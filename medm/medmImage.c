/*
*****************************************************************
                          COPYRIGHT NOTIFICATION
*****************************************************************

THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
AND IN ALL SOURCE LISTINGS OF THE CODE.

(C)  COPYRIGHT 1993 UNIVERSITY OF CHICAGO

Argonne National Laboratory (ANL), with facilities in the States of
Illinois and Idaho, is owned by the United States Government, and
operated by the University of Chicago under provision of a contract
with the Department of Energy.

Portions of this material resulted from work developed under a U.S.
Government contract and are subject to the following license:  For
a period of five years from March 30, 1993, the Government is
granted for itself and others acting on its behalf a paid-up,
nonexclusive, irrevocable worldwide license in this computer
software to reproduce, prepare derivative works, and perform
publicly and display publicly.  With the approval of DOE, this
period may be renewed for two additional five year periods.
Following the expiration of this period or periods, the Government
is granted for itself and others acting on its behalf, a paid-up,
nonexclusive, irrevocable worldwide license in this computer
software to reproduce, prepare derivative works, distribute copies
to the public, perform publicly and display publicly, and to permit
others to do so.

*****************************************************************
                                DISCLAIMER
*****************************************************************

NEITHER THE UNITED STATES GOVERNMENT NOR ANY AGENCY THEREOF, NOR
THE UNIVERSITY OF CHICAGO, NOR ANY OF THEIR EMPLOYEES OR OFFICERS,
MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL
LIABILITY OR RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR
USEFULNESS OF ANY INFORMATION, APPARATUS, PRODUCT, OR PROCESS
DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT INFRINGE PRIVATELY
OWNED RIGHTS.

*****************************************************************
LICENSING INQUIRIES MAY BE DIRECTED TO THE INDUSTRIAL TECHNOLOGY
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (630-252-2000).
*/
/*****************************************************************************
 *
 *     Original Author : Mark Anderson
 *     Second Author   : Frederick Vong
 *     Third Author    : Kenneth Evans, Jr.
 *
 *****************************************************************************
*/

#define DEBUG_ANIMATE 0

#define DEFAULT_TIME 100
#define ANIMATE_TIME(gif) \
  ((gif)->frames[CURFRAME(gif)]->DelayTime ? \
  ((gif)->frames[CURFRAME(gif)]->DelayTime)*10 : DEFAULT_TIME)

#include "medm.h"

#include "xgif.h"

#include <X11/keysym.h>

typedef struct _Image {
    DisplayInfo   *displayInfo;
    DlElement     *dlElement;
    Record        *record;
    UpdateTask    *updateTask;
    XtIntervalId  timerid;
} MedmImage;

Widget importFSD;
XmString gifDirMask;
static void destroyDlImage(DlElement *);
static void imageGetValues(ResourceBundle *pRCB, DlElement *p);
static DlDispatchTable imageDlDispatchTable = {
    createDlImage,
    NULL,
    executeDlImage,
    writeDlImage,
    NULL,
    imageGetValues,
    NULL,
    NULL,
    NULL,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

/* Function prototypes */

static void animateImage(XtPointer cd, XtIntervalId *id);
static void destroyDlImage(DlElement *dlElement);
static void drawImage(MedmImage *pi);
static void imageGetValues(ResourceBundle *pRCB, DlElement *p);
static void imageUpdateValueCb(XtPointer cd);
static void imageUpdateValueCb(XtPointer cd);
static void importCallback(Widget w, XtPointer cd, XtPointer cbs);
static void imageDraw(XtPointer cd);
static void imageDestroyCb(XtPointer cd);
static void imageGetRecord(XtPointer cd, Record **record, int *count);

static void drawImage(MedmImage *pi)
{
    unsigned int lineWidth;
    DisplayInfo *displayInfo = pi->updateTask->displayInfo;
    Widget widget = pi->updateTask->displayInfo->drawingArea;
    Display *display = XtDisplay(widget);
    DlImage *dlImage = pi->dlElement->structure.image;

#if 0
    lineWidth = (dlImage->attr.width+1)/2;
    if (dlImage->attr.fill == F_SOLID) {
	XFillImage(display,XtWindow(widget),displayInfo->gc,
          dlImage->object.x,dlImage->object.y,
          dlImage->object.width,dlImage->object.height);
    } else if (dlImage->attr.fill == F_OUTLINE) {
	XDrawImage(display,XtWindow(widget),displayInfo->gc,
	  dlImage->object.x + lineWidth,
	  dlImage->object.y + lineWidth,
	  dlImage->object.width - 2*lineWidth,
	  dlImage->object.height - 2*lineWidth);
    }
#endif    
}

void executeDlImage(DisplayInfo *displayInfo, DlElement *dlElement)
{
    GIFData *gif;
    DlImage *dlImage = dlElement->structure.image;

  /* From the DlImage structure, we've got the image's dimensions */
    switch (dlImage->imageType) {
    case GIF_IMAGE:
      /* KE: GIF is the only type */
	if (dlImage->privateData == NULL) {
	    if (!initializeGIF(displayInfo,dlImage)) {
	      /* Something failed - bail out! */
		if(dlImage->privateData != NULL) {
		    free((char *)dlImage->privateData);
		    dlImage->privateData = NULL;
		}
	    }
	    gif = (GIFData *)dlImage->privateData;
#if DEBUG_ANIMATE
	    fprintf(stderr,"executeDlImage (1): dlImage=%x gif=%x nFrames=%d\n",
	      dlImage,gif,gif?gif->nFrames:-1);
#endif
	} else {
	    gif = (GIFData *)dlImage->privateData;
	    if (gif != NULL) {
		gif->curFrame=0;
		if (dlImage->object.width == gif->currentWidth &&
		  dlImage->object.height == gif->currentHeight) {
		    drawGIF(displayInfo,dlImage);
		} else {
		    resizeGIF(dlImage);
		    drawGIF(displayInfo,dlImage);
		}
#if DEBUG_ANIMATE
		fprintf(stderr,"executeDlImage (2): dlImage=%x gif=%x nFrames=%d\n",
		  dlImage,gif,gif?gif->nFrames:-1);
#endif
	    }
	}
	break;
    }

  /* Allocate and fill in MedmImage struct */
    if (displayInfo->traversalMode == DL_EXECUTE) {
	MedmImage *pi;
	pi = (MedmImage *)malloc(sizeof(MedmImage));
	pi->displayInfo = displayInfo;
	pi->dlElement = dlElement;
	pi->record = NULL;
	pi->timerid = (XtIntervalId)0;
	pi->updateTask = updateTaskAddTask(displayInfo, &(dlImage->object),
	  imageDraw, (XtPointer)pi);
	if (pi->updateTask == NULL) {
	    medmPrintf(1,"\nexecuteDlImage: Memory allocation error\n");
	} else {
	    updateTaskAddDestroyCb(pi->updateTask,imageDestroyCb);
	    updateTaskAddNameCb(pi->updateTask,imageGetRecord);
	    pi->updateTask->opaque = False;
	}
	if(*dlImage->dynAttr.chan) {
	    pi->record = medmAllocateRecord(dlImage->dynAttr.chan,
	      imageUpdateValueCb,NULL,(XtPointer) pi);
	    pi->record->monitorValueChanged = False;

#ifdef __COLOR_RULE_H__
	    switch (dlImage->dynAttr.clr) {
	    case STATIC:
		pi->record->monitorValueChanged = False;
		pi->record->monitorSeverityChanged = False;
		break;
	    case ALARM:
		pi->record->monitorValueChanged = False;
		break;
	    case DISCRETE:
		pi->record->monitorSeverityChanged = False;
		break;
	    }
#else
	    pi->record->monitorValueChanged = False;
	    if (dlImage->dynAttr.clr != ALARM ) {
		pi->record->monitorSeverityChanged = False;
	    }
#endif
	    if (dlImage->dynAttr.vis == V_STATIC ) {
		pi->record->monitorZeroAndNoneZeroTransition = False;
	    }
	} else {
	  /* No channel */
	    if(gif && gif->nFrames > 1) {
#if DEBUG_ANIMATE
		fprintf(stderr,"executeDlImage (3): pi=%x dlElement=%x "
		  "gif=%x nFrames=%d pi->timerid=%x\n",
		  &pi,pi->dlElement,gif,gif->nFrames,pi->timerid);
#endif
		pi->timerid=XtAppAddTimeOut(appContext,
		  ANIMATE_TIME(gif), animateImage, (XtPointer)pi);
	    }
	}
    } else {
#if DEBUG_ANIMATE
	fprintf(stderr,"executeDlImage: dlElement=%x dlImage=%x clr=%d "
	  "width=%d\n",
	  dlElement,dlImage,dlImage->attr.clr,dlImage->attr.width);
#endif
	executeDlBasicAttribute(displayInfo,&(dlImage->attr));
    }
}

static void animateImage(XtPointer cd, XtIntervalId *id)
{
    MedmImage *pi = (MedmImage *)cd;
    GIFData *gif = (GIFData *)pi->dlElement->structure.image->privateData;

#if DEBUG_ANIMATE
    fprintf(stderr,"animateImage: pi=%x dlElement=%x "
      "gif=%x nFrames=%d curFrame=%d pi->timerid=%x\n",
      &pi,pi->dlElement,gif,gif->nFrames,gif->nFrames,pi->timerid);
#endif
  /* Display the next image */
    if(++gif->curFrame >= gif->nFrames) gif->curFrame=0;
    drawGIF(pi->displayInfo, pi->dlElement->structure.image);

  /* Reinstall the timeout */
    pi->timerid=XtAppAddTimeOut(appContext, ANIMATE_TIME(gif),
      animateImage, (XtPointer)pi);
}

static void imageDraw(XtPointer cd)
{
    MedmImage *pi = (MedmImage *)cd;
    Record *pr = pi->record;
    DisplayInfo *displayInfo = pi->updateTask->displayInfo;
    XGCValues gcValues;
    unsigned long gcValueMask;
    Display *display = XtDisplay(pi->updateTask->displayInfo->drawingArea);
    DlImage *dlImage = pi->dlElement->structure.image;
    
    if(!pr) return;
    if (pr->connected) {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	switch (dlImage->dynAttr.clr) {
#ifdef __COLOR_RULE_H__
	case STATIC :
	    gcValues.foreground = displayInfo->colormap[dlImage->attr.clr];
	    break;
	case DISCRETE:
	    gcValues.foreground = extractColor(displayInfo,
	      pr->value,
	      dlImage->dynAttr.colorRule,
	      dlImage->attr.clr);
	    break;
#else
	case STATIC :
	case DISCRETE:
	    gcValues.foreground = displayInfo->colormap[dlImage->attr.clr];
	    break;
	case ALARM :
	    gcValues.foreground = alarmColor(pr->severity);
	    break;
#endif
	}
	gcValues.line_width = dlImage->attr.width;
	gcValues.line_style = ( (dlImage->attr.style == SOLID) ? LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

	switch (dlImage->dynAttr.vis) {
	case V_STATIC:
	    drawImage(pi);
	    break;
	case IF_NOT_ZERO:
	    if (pr->value != 0.0)
	      drawImage(pi);
	    break;
	case IF_ZERO:
	    if (pr->value == 0.0)
	      drawImage(pi);
	    break;
	default :
	    medmPrintf(1,"\nimageUpdateValueCb: Unknown visibility\n");
	    break;
	}
	if (pr->readAccess) {
	    if (!pi->updateTask->overlapped && dlImage->dynAttr.vis == V_STATIC) {
		pi->updateTask->opaque = True;
	    }
	} else {
	    pi->updateTask->opaque = False;
	    draw3DQuestionMark(pi->updateTask);
	}
    } else {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	gcValues.foreground = WhitePixel(display,DefaultScreen(display));
	gcValues.line_width = dlImage->attr.width;
	gcValues.line_style = ((dlImage->attr.style == SOLID) ? LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
	drawImage(pi);
    }
}

static void imageDestroyCb(XtPointer cd)
{
    MedmImage *pi = (MedmImage *)cd;
    if (pi) {
	if(pi->timerid) {
	    XtRemoveTimeOut(pi->timerid);
	    pi->timerid=0;
	}
	medmDestroyRecord(pi->record);
	free((char *)pi);
    }
    return;
}

static void destroyDlImage(DlElement *dlElement)
{
    freeGIF(dlElement->structure.image);
    free((char*)dlElement->structure.composite);
    destroyDlElement(dlElement);
}

static void imageGetRecord(XtPointer cd, Record **record, int *count)
{
    MedmImage *pi = (MedmImage *)cd;
    *count = 1;
    record[0] = pi->record;
}

static void imageUpdateValueCb(XtPointer cd)
{
    MedmImage *pi = (MedmImage *)((Record *) cd)->clientData;
    updateTaskMarkUpdate(pi->updateTask);
}

static void importCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    XmFileSelectionBoxCallbackStruct *call_data =
      (XmFileSelectionBoxCallbackStruct *) cbs;
    char *fullPathName, *dirName;
    int dirLength;

    switch(call_data->reason){
    case XmCR_CANCEL:
	XtUnmanageChild(w);
	break;

    case XmCR_OK:
	if (call_data->value != NULL && call_data->dir != NULL) {
	    DlElement *dlElement = *((DlElement **) cd);
	    DlImage *dlImage;
	    if (!dlElement) return;
	    dlImage = dlElement->structure.image;

	    XmStringGetLtoR(call_data->value,XmSTRING_DEFAULT_CHARSET,&fullPathName);
	    XmStringGetLtoR(call_data->dir,XmSTRING_DEFAULT_CHARSET,&dirName);
	    dirLength = strlen(dirName);
	    XtUnmanageChild(w);
#ifndef VMS
	    strcpy(dlImage->imageName, &(fullPathName[dirLength]));
#else
	    strcpy(dlImage->imageName, &(fullPathName[0]));
#endif
	    dlImage->imageType = GIF_IMAGE;
	    (dlElement->run->execute)(currentDisplayInfo, dlElement);
	  /* Unselect any selected elements */
	    unselectElementsInDisplay();
	    
	    setResourcePaletteEntries();
	}
	XtFree(fullPathName);
	XtFree(dirName);
	break;
    }
}

/* Function which handles creation (and initial display) of images */
DlElement* handleImageCreate()
{
    int n;
    Arg args[10];
    static DlElement *dlElement = 0;
    if (!(dlElement = createDlImage(NULL))) return 0;

    if (importFSD == NULL) {
	gifDirMask = XmStringCreateLocalized("*.gif");
	globalResourceBundle.imageType = GIF_IMAGE;
	n = 0;
	XtSetArg(args[n],XmNdirMask,gifDirMask); n++;
	XtSetArg(args[n],XmNdialogStyle,XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
	importFSD = XmCreateFileSelectionDialog(resourceMW,"importFSD",args,n);
	XtAddCallback(importFSD,XmNokCallback,importCallback,&dlElement);
	XtAddCallback(importFSD,XmNcancelCallback,importCallback,NULL);
	XtManageChild(importFSD);
    } else {
	XtManageChild(importFSD);
    }
    return dlElement;
}

DlElement *createDlImage(DlElement *p)
{
    DlImage *dlImage;
    DlElement *dlElement;

    dlImage = (DlImage *)malloc(sizeof(DlImage));
    if (!dlImage) return 0;
    if (p) {
	*dlImage = *p->structure.image;
    } else {
	objectAttributeInit(&(dlImage->object));
	basicAttributeInit(&(dlImage->attr));
	dynamicAttributeInit(&(dlImage->dynAttr));
	dlImage->imageType = NO_IMAGE;
	dlImage->imageName[0] = '\0';
	dlImage->privateData = NULL;
    }

    if (!(dlElement = createDlElement(DL_Image,
      (XtPointer)      dlImage,
      &imageDlDispatchTable))) {
	free(dlImage);
    }

#if DEBUG_ANIMATE
    fprintf(stderr,"createDlImage: dlElement=%x dlImage=%x clr=%d width=%d\n",
      dlElement,dlImage,dlImage->attr.clr,dlImage->attr.width);
#endif
    
    return(dlElement);
}

DlElement *parseImage(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlImage *dlImage = 0;
    DlElement *dlElement = createDlImage(NULL);

    if (!dlElement) return 0;
    dlImage = dlElement->structure.image;
    do {
        switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlImage->object));
	    } else if (!strcmp(token,"basic attribute")) {
		parseBasicAttribute(displayInfo,&(dlImage->attr));
	    } else if(!strcmp(token,"dynamic attribute")) {
		parseDynamicAttribute(displayInfo,&(dlImage->dynAttr));
	    } else if (!strcmp(token,"type")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"none"))
		  dlImage->imageType = NO_IMAGE;
		else if (!strcmp(token,"gif"))
		  dlImage->imageType = GIF_IMAGE;
		else if (!strcmp(token,"tiff"))
		/* KE: There is no TIFF capability */
		  dlImage->imageType = TIFF_IMAGE;
	    } else if (!strcmp(token,"image name")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlImage->imageName,token);
	    }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
        }
    } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );
    
  /* Just to be safe, initialize the privateData member separately */
    dlImage->privateData = NULL;

    return dlElement;
}

void writeDlImage(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
    int i;
    char indent[16];
    DlImage *dlImage = dlElement->structure.image;
    
    for (i = 0; i < level; i++) indent[i] = '\t';
    indent[i] = '\0';
    
    fprintf(stream,"\n%simage {",indent);
    writeDlObject(stream,&(dlImage->object),level+1);
    writeDlBasicAttribute(stream,&(dlImage->attr),level+1);
    writeDlDynamicAttribute(stream,&(dlImage->dynAttr),level+1);
    fprintf(stream,"\n%s\ttype=\"%s\"",indent,
      stringValueTable[dlImage->imageType]);
    fprintf(stream,"\n%s\t\"image name\"=\"%s\"",indent,dlImage->imageName);
    fprintf(stream,"\n%s}",indent);
}

static void imageGetValues(ResourceBundle *pRCB, DlElement *p)
{
    DlImage *dlImage = p->structure.image;
    medmGetValues(pRCB,
      X_RC,          &(dlImage->object.x),
      Y_RC,          &(dlImage->object.y),
      WIDTH_RC,      &(dlImage->object.width),
      HEIGHT_RC,     &(dlImage->object.height),
      IMAGETYPE_RC,  &(dlImage->imageType),
      IMAGENAME_RC,  &(dlImage->imageName),
      -1);
}
