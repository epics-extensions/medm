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

#define DEFAULT_TIME 1
#define ANIMATE_TIME(gif) \
  ((gif)->frames[CURFRAME(gif)]->DelayTime ? \
  ((gif)->frames[CURFRAME(gif)]->DelayTime)/100. : DEFAULT_TIME)

#include "postfix.h"
#include "medm.h"
#include "xgif.h"

#include <X11/keysym.h>

typedef struct _Image {
    DisplayInfo *displayInfo;
    DlElement *dlElement;
    Record **records;
    UpdateTask *updateTask;
    Boolean validCalc;
    char post[MAX_TOKEN_LENGTH];
} MedmImage;

Widget importFSD;
XmString gifDirMask;

/* Function prototypes */

static void destroyDlImage(DlElement *dlElement);
static void drawImage(MedmImage *pi);
static void imageDestroyCb(XtPointer cd);
static void imageDraw(XtPointer cd);
static void imageGetRecord(XtPointer cd, Record **record, int *count);
static void imageGetValues(ResourceBundle *pRCB, DlElement *p);
static void imageGetValues(ResourceBundle *pRCB, DlElement *p);
static void imageInheritValues(ResourceBundle *pRCB, DlElement *p);
static void imageUpdateGraphicalInfoCb(XtPointer cd);
static void imageUpdateValueCb(XtPointer cd);
static void imageUpdateValueCb(XtPointer cd);
static void imageScale(DlElement *dlElement, int xOffset, int yOffset);
static void importCallback(Widget w, XtPointer cd, XtPointer cbs);

static DlDispatchTable imageDlDispatchTable = {
    createDlImage,
    destroyDlImage,
    executeDlImage,
    writeDlImage,
    NULL,
    imageGetValues,
    imageInheritValues,
    NULL,
    NULL,
    genericMove,
    imageScale,
    genericOrient,
    NULL,
    NULL};

#if 0
static void drawImage(MedmImage *pi)
{
    unsigned int lineWidth;
    DisplayInfo *displayInfo = pi->updateTask->displayInfo;
    Widget widget = pi->updateTask->displayInfo->drawingArea;
    Display *display = XtDisplay(widget);
    DlImage *dlImage = pi->dlElement->structure.image;

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
}
#endif    

void executeDlImage(DisplayInfo *displayInfo, DlElement *dlElement)
{
    GIFData *gif;
    DlImage *dlImage = dlElement->structure.image;

  /* Get the image */
    switch(dlImage->imageType) {
    case GIF_IMAGE:
	if (dlImage->privateData == NULL) {
	  /* Not initialized */
	    if(!initializeGIF(displayInfo,dlImage)) {
	      /* Something failed - bail out! */
		if(dlImage->privateData != NULL) freeGIF(dlImage);
	    }
	    gif = (GIFData *)dlImage->privateData;
#if DEBUG_ANIMATE
	    print("executeDlImage: %s dlImage=%x gif=%x nFrames=%d\n",
	      gif->imageName,dlImage,gif,gif?gif->nFrames:-1);
#endif
	} else {
	  /* Already initialized */
	    gif = (GIFData *)dlImage->privateData;
	  /* Check if filename has changed */
	    if(strcmp(gif->imageName,dlImage->imageName)) {
		if(!initializeGIF(displayInfo,dlImage)) {
		  /* Something failed - bail out! */
		    if(dlImage->privateData != NULL) freeGIF(dlImage);
		}
		gif = (GIFData *)dlImage->privateData;
	    }
	}
	break;
    case NO_IMAGE:
    case TIFF_IMAGE:
	if(dlImage->privateData != NULL) freeGIF(dlImage);
	gif = NULL;
    }

  /* Allocate and fill in MedmImage struct */
    if (displayInfo->traversalMode == DL_EXECUTE) {
      /* EXECUTE mode */
	MedmImage *pi;
	
	pi = (MedmImage *)malloc(sizeof(MedmImage));
	pi->displayInfo = displayInfo;
	pi->dlElement = dlElement;
	pi->records = NULL;
	pi->updateTask = updateTaskAddTask(displayInfo, &(dlImage->object),
	  imageDraw, (XtPointer)pi);
	pi->validCalc = False;
	pi->post[0] = '\0';
	if (pi->updateTask == NULL) {
	    medmPrintf(1,"\nexecuteDlImage: Memory allocation error\n");
	} else {
	    updateTaskAddDestroyCb(pi->updateTask,imageDestroyCb);
	    updateTaskAddNameCb(pi->updateTask,imageGetRecord);
	    pi->updateTask->opaque = False;
	}
	pi->records = NULL;
	if(*dlImage->dynAttr.chan[0]) {
	    long status;
	    short errnum;
	    
	  /* A channel is defined */
	    pi->records = medmAllocateDynamicRecords(&dlImage->dynAttr,
	      imageUpdateValueCb,
	      imageUpdateGraphicalInfoCb,
	      (XtPointer)pi);
	  /* Calculate the postfix for calc */
	    status=postfix(dlImage->calc, pi->post, &errnum);
	    if(status) {
		medmPostMsg(1,"executeDlImage:\n"
		  "  Invalid calc expression [error %d]: %s\n  for %s\n",
		  errnum, dlImage->calc, dlImage->dynAttr.chan);
		pi->validCalc=False;
	    } else {
		pi->validCalc=True;
	    }
	  /* Draw initial white rectangle */
	    drawWhiteRectangle(pi->updateTask);
	} else {
	  /* No channel */
	    if(gif) {
		if(gif->nFrames > 1) {
		    updateTaskSetScanRate(pi->updateTask, ANIMATE_TIME(gif));
		} else {
		  /* Draw the first frame */
		    drawGIF(displayInfo,dlImage);
		}
	    }
	}
    } else {
      /* EDIT mode */
	if (gif != NULL) {
	    gif->curFrame=0;
	    if (dlImage->object.width == gif->currentWidth &&
	      dlImage->object.height == gif->currentHeight) {
		drawGIF(displayInfo,dlImage);
	    } else {
		resizeGIF(dlImage);
		drawGIF(displayInfo,dlImage);
	    }
	}
    }
}

/* This routine is the update task.  It is called for updates and for
   drawing area exposures */
static void imageDraw(XtPointer cd)
{
    MedmImage *pi = (MedmImage *)cd;
    DisplayInfo *displayInfo = pi->updateTask->displayInfo;
    Record *pr = pi->records?pi->records[0]:NULL;
    DlImage *dlImage = pi->dlElement->structure.image;
    GIFData *gif = (GIFData *)dlImage->privateData;

  /* Branch on whether there is a channel or not */
    if(*dlImage->dynAttr.chan[0]) {
      /* A channel is defined */
	updateTaskSetScanRate(pi->updateTask, 0.0);
	if(!pr) return;
	if (pr->connected) {
	    if (pr->readAccess) {
		double result;
		long status;

	      /* Determine the result of the calculation */
		if(!*dlImage->calc) {
		  /* calc string is empty */
		    result=pr->value;
		    status = 0;
		} else if(!pi->validCalc) {
		  /* calc string is invalid */
		    status = 1;
		} else {
		  /* Perform the calculation */
		    status = calcPerform(&pr->value, &result, pi->post);
		}
	      /* Draw depending on status */
		if(!status) {
		  /* Result is valid, convert to frame number */
		    if(gif) {
			if(result < 0.0) gif->curFrame = 0;
			else if(result > gif->nFrames-1) gif->curFrame=gif->nFrames-1;
			else gif->curFrame = result +.5;
		      /* Draw the frame */
			drawGIF(pi->displayInfo, pi->dlElement->structure.image);
		    }
		} else {
		  /* Result is invalid */
		    draw3DPane(pi->updateTask,BlackPixel(display,screenNum));
		}
	    } else {
		pi->updateTask->opaque = False;
		draw3DQuestionMark(pi->updateTask);
	    }
	} else {
	    drawWhiteRectangle(pi->updateTask);
	}
      /* Update the drawing objects above */
	redrawElementsAbove(displayInfo, (DlElement *)dlImage);
    } else {
      /* No channel */
#if DEBUG_ANIMATE
	print("imageDraw: time=%.3f interval=%.3f name=%s\n",
	  medmElapsedTime(),ANIMATE_TIME(gif),gif->imageName);
#endif
	if(gif) {
	    if(gif->nFrames > 1) {
	      /* Draw the next image */
		if(++gif->curFrame >= gif->nFrames) gif->curFrame = 0;
		drawGIF(pi->displayInfo, pi->dlElement->structure.image);
		
	      /* Reset the time */
		updateTaskSetScanRate(pi->updateTask, ANIMATE_TIME(gif));
	    } else {
	      /* Draw the image */
		drawGIF(pi->displayInfo, pi->dlElement->structure.image);
	    }
	}
      /* Don't update the drawing objects above (Could get to be
       * expensive) */
    }
}

static void imageDestroyCb(XtPointer cd) {
    MedmImage *pi = (MedmImage *)cd;

    if (pi) {
	Record **records = pi->records;
	
	updateTaskSetScanRate(pi->updateTask, 0.0);
	if(records) {
	    int i;
	    for(i=0; i < MAX_CALC_RECORDS; i++) {
		if(records[i]) medmDestroyRecord(records[i]);
	    }
	    free((char *)records);
	}
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

static void imageGetRecord(XtPointer cd, Record **record, int *count) {
    MedmImage *pi = (MedmImage *)cd;
    int i;
    
    *count = MAX_CALC_RECORDS;
    for(i=0; i < MAX_CALC_RECORDS; i++) {
	record[i] = pi->records[i];
    }
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

static void imageUpdateGraphicalInfoCb(XtPointer cd)
{
    Record *pr = (Record *)cd;
    MedmImage *pi = (MedmImage *)pr->clientData;
    DlImage *dlImage = pi->dlElement->structure.image;

    switch (pr->dataType) {
    case DBF_STRING :
	medmPostMsg(1,"imageUpdateGraphicalInfoCb:\n"
	  "  Illegal channel type for %s\n"
	  "  Cannot attach image\n",
	  dlImage->dynAttr.chan);
	return;
    case DBF_ENUM :
    case DBF_CHAR :
    case DBF_INT :
    case DBF_LONG :
    case DBF_FLOAT :
    case DBF_DOUBLE :
	break;
    default :
	medmPostMsg(1,"imageUpdateGraphicalInfoCb:\n"
	  "  Unknown channel type for %s\n"
	  "  Cannot attach image\n",
	  dlImage->dynAttr.chan);
	break;
    }
#if 0    
    updateTaskMarkUpdate(pi->updateTask);
#endif    
}

static void imageScale(DlElement *dlElement, int xOffset, int yOffset)
{
    DlImage *dlImage = dlElement->structure.image;
    int width, height, oldWidth, oldHeight;

    oldWidth = dlImage->object.width;
    width = MAX(oldWidth + xOffset, 1);
    dlImage->object.width = width;
    oldHeight = dlImage->object.height;
    height = MAX(oldHeight + yOffset, 1);
    dlImage->object.height = height;
    if(width != oldWidth || height != oldHeight) {
	resizeGIF(dlImage);
    }
}

DlElement *createDlImage(DlElement *p)
{
    DlImage *dlImage;
    DlElement *dlElement;

    dlImage = (DlImage *)malloc(sizeof(DlImage));
    if (!dlImage) return 0;
    if (p) {
	*dlImage = *p->structure.image;
      /* Make copies of the data pointed to by privateData */
	copyGIF(dlImage,dlImage);
    } else {
	objectAttributeInit(&(dlImage->object));
	dynamicAttributeInit(&(dlImage->dynAttr));
	dlImage->calc[0] = '\0';
	dlImage->calc[0] = '\0';
	dlImage->imageType = NO_IMAGE;
	dlImage->imageName[0] = '\0';
	dlImage->privateData = NULL;
    }

    if (!(dlElement = createDlElement(DL_Image, (XtPointer)dlImage,
      &imageDlDispatchTable))) {
	free(dlImage);
    }

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
	    } else if (!strcmp(token,"dynamic attribute")) {
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
	    } else if(!strcmp(token,"image name")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlImage->imageName,token);
	    } else if(!strcmp(token,"calc")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlImage->calc,token);
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
    fprintf(stream,"\n%s\ttype=\"%s\"",indent,
      stringValueTable[dlImage->imageType]);
    if(*dlImage->imageName)
      fprintf(stream,"\n%s\t\"image name\"=\"%s\"",indent,dlImage->imageName);
    if(*dlImage->calc)
      fprintf(stream,"\n%s\tcalc=\"%s\"",indent,dlImage->calc);
    writeDlDynamicAttribute(stream,&(dlImage->dynAttr),level+1);
    fprintf(stream,"\n%s}",indent);
}

static void imageInheritValues(ResourceBundle *pRCB, DlElement *p)
{
    DlImage *dlImage = p->structure.image;
    medmGetValues(pRCB,
      IMAGETYPE_RC,  &(dlImage->imageType),
      IMAGENAME_RC,  &(dlImage->imageName),
      CALC_RC,       &(dlImage->calc),
      CLRMOD_RC,     &(dlImage->dynAttr.clr),
      VIS_RC,        &(dlImage->dynAttr.vis),
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlImage->dynAttr.colorRule),
#endif
      CHAN_RC,       &(dlImage->dynAttr.chan),
      -1);
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
      CALC_RC,       &(dlImage->calc),
      CLRMOD_RC,     &(dlImage->dynAttr.clr),
      VIS_RC,        &(dlImage->dynAttr.vis),
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlImage->dynAttr.colorRule),
#endif
      CHAN_RC,       &(dlImage->dynAttr.chan),
      -1);
}

