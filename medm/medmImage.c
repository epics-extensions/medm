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

#include "medm.h"

#include "xgif.h"

#include <X11/keysym.h>

typedef struct _Image {
    DisplayInfo   *displayInfo;
    DlElement     *dlElement;
    Record        *record;
    UpdateTask    *updateTask;
} MedmImage;

Widget importFSD;
XmString gifDirMask;

/* Function prototypes */
static void animateImage(MedmImage *pi);
static void destroyDlImage(DlElement *);
static void destroyDlImage(DlElement *dlElement);
static void drawImage(MedmImage *pi);
static void imageDestroyCb(XtPointer cd);
static void imageDraw(XtPointer cd);
static void imageGetLimits(DlElement *pE, DlLimits **ppL, char **pN);
static void imageGetRecord(XtPointer cd, Record **record, int *count);
static void imageGetValues(ResourceBundle *pRCB, DlElement *p);
static void imageGetValues(ResourceBundle *pRCB, DlElement *p);
static void imageInheritValues(ResourceBundle *pRCB, DlElement *p);
static void imageUpdateGraphicalInfoCb(XtPointer cd);
static void imageUpdateValueCb(XtPointer cd);
static void imageUpdateValueCb(XtPointer cd);
static void importCallback(Widget w, XtPointer cd, XtPointer cbs);

static DlDispatchTable imageDlDispatchTable = {
    createDlImage,
    NULL,
    executeDlImage,
    writeDlImage,
    imageGetLimits,
    imageGetValues,
    imageInheritValues,
    NULL,
    NULL,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

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

  /* Get the image */
    switch (dlImage->imageType) {
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
	pi->record = NULL;
	pi->updateTask = updateTaskAddTask(displayInfo, &(dlImage->object),
	  imageDraw, (XtPointer)pi);
	if (pi->updateTask == NULL) {
	    medmPrintf(1,"\nexecuteDlImage: Memory allocation error\n");
	} else {
	    updateTaskAddDestroyCb(pi->updateTask,imageDestroyCb);
	    updateTaskAddNameCb(pi->updateTask,imageGetRecord);
	    pi->updateTask->opaque = False;
	}
	pi->record = NULL;
	if(*dlImage->monitor.rdbk) {
	  /* A channel is defined */
	    pi->record = medmAllocateRecord(dlImage->monitor.rdbk,
	      imageUpdateValueCb,
	      imageUpdateGraphicalInfoCb,
	      (XtPointer)pi);
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

  /* Update the limits to reflect current src's */
    updatePvLimits(&dlImage->limits);
    
}

#if 0
static void animateImage(MedmImage *pi)
{
    GIFData *gif = (GIFData *)pi->dlElement->structure.image->privateData;

#if DEBUG_ANIMATE
	print("animateImage: time=%.3f interval=%.3f name=%s\n",
	  medmElapsedTime(),ANIMATE_TIME(gif),gif->imageName);
#endif    
  /* Display the next image */
    if(++gif->curFrame >= gif->nFrames) gif->curFrame=0;
    drawGIF(pi->displayInfo, pi->dlElement->structure.image);

  /* Reset the time */
    updateTaskSetScanRate(pi->updateTask, ANIMATE_TIME(gif));
}
#endif

/* This routine is the update task.  It is called for updates and for
   drawing area exposures */
static void imageDraw(XtPointer cd)
{
    MedmImage *pi = (MedmImage *)cd;
    Record *pr = pi->record;
    DlImage *dlImage = pi->dlElement->structure.image;

  /* Branch on whether there is a channel or not */
    if(*dlImage->monitor.rdbk) {
      /* A channel is defined */
	if(!pr) return;
	if (pr->connected) {
	    if (pr->readAccess) {
	    } else {
		draw3DPane(pi->updateTask,
		  pi->updateTask->displayInfo->colormap[dlImage->monitor.bclr]);
		draw3DQuestionMark(pi->updateTask);
	    }
	} else {
	    drawWhiteRectangle(pi->updateTask);
	}
    } else {
      /* No channel */
	GIFData *gif = (GIFData *)pi->dlElement->structure.image->privateData;
	
#if DEBUG_ANIMATE
	print("imageDraw: time=%.3f interval=%.3f name=%s\n",
	  medmElapsedTime(),ANIMATE_TIME(gif),gif->imageName);
#endif
	if(gif->nFrames > 1) {
	  /* Draw the next image */
	    if(++gif->curFrame >= gif->nFrames) gif->curFrame=0;
	    drawGIF(pi->displayInfo, pi->dlElement->structure.image);
	    
	  /* Reset the time */
	    updateTaskSetScanRate(pi->updateTask, ANIMATE_TIME(gif));
	} else {
	  /* Draw the image */
	    drawGIF(pi->displayInfo, pi->dlElement->structure.image);
	}
    }
}

static void imageDestroyCb(XtPointer cd)
{
    MedmImage *pi = (MedmImage *)cd;
    if (pi) {
	updateTaskSetScanRate(pi->updateTask, 0.0);
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

static void imageUpdateGraphicalInfoCb(XtPointer cd)
{
#if 0
    Record *pr = (Record *) cd;
    Meter *pm = (Meter *) pr->clientData;
    DlMeter *dlMeter = pm->dlElement->structure.meter;
    Pixel pixel;
    Widget widget = pm->dlElement->widget;
    XcVType hopr, lopr, val;
    short precision;

    switch (pr->dataType) {
    case DBF_STRING :
	medmPostMsg(1,"meterUpdateGraphicalInfoCb:\n"
	  "  Illegal channel type for %s\n"
	  "  Cannot attach meter\n",
	  dlMeter->monitor.rdbk);
	return;
    case DBF_ENUM :
    case DBF_CHAR :
    case DBF_INT :
    case DBF_LONG :
    case DBF_FLOAT :
    case DBF_DOUBLE :
	hopr.fval = (float) pr->hopr;
	lopr.fval = (float) pr->lopr;
	val.fval = (float) pr->value;
	precision = pr->precision;
	break;
    default :
	medmPostMsg(1,"meterUpdateGraphicalInfoCb:\n"
	  "  Unknown channel type for %s\n"
	  "  Cannot attach meter\n",
	  dlMeter->monitor.rdbk);
	break;
    }
    if ((hopr.fval == 0.0) && (lopr.fval == 0.0)) {
	hopr.fval += 1.0;
    }
    if (widget != NULL) {
      /* Set foreground pixel according to alarm */
	pixel = (dlMeter->clrmod == ALARM) ?
	  alarmColor(pr->severity) :
	  pm->updateTask->displayInfo->colormap[dlMeter->monitor.clr];
	XtVaSetValues(widget, XcNmeterForeground,pixel, NULL);

      /* Set Channel and User limits (if apparently not set yet) */
	dlMeter->limits.loprChannel = lopr.fval;
	if(dlMeter->limits.loprSrc != PV_LIMITS_USER &&
	  dlMeter->limits.loprUser == LOPR_DEFAULT) {
	    dlMeter->limits.loprUser = lopr.fval;
	}
	dlMeter->limits.hoprChannel = hopr.fval;
	if(dlMeter->limits.hoprSrc != PV_LIMITS_USER &&
	  dlMeter->limits.hoprUser == HOPR_DEFAULT) {
	    dlMeter->limits.hoprUser = hopr.fval;
	}
	dlMeter->limits.precChannel = precision;
	if(dlMeter->limits.precSrc != PV_LIMITS_USER &&
	  dlMeter->limits.precUser == PREC_DEFAULT) {
	    dlMeter->limits.precUser = precision;
	}

      /* Set values in the widget if src is Channel */
	if(dlMeter->limits.loprSrc == PV_LIMITS_CHANNEL) {
	    dlMeter->limits.lopr = lopr.fval;
	    XtVaSetValues(widget, XcNlowerBound,lopr.lval, NULL);
	}
	if(dlMeter->limits.hoprSrc == PV_LIMITS_CHANNEL) {
	    dlMeter->limits.hopr = hopr.fval;
	    XtVaSetValues(widget, XcNupperBound,hopr.lval, NULL);
	}
	if(dlMeter->limits.precSrc == PV_LIMITS_CHANNEL) {
	    dlMeter->limits.prec = precision;
	    XtVaSetValues(widget, XcNdecimals, (int)precision, NULL);
	}
	XcMeterUpdateValue(widget,&val);
    }
#endif
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
	monitorAttributeInit(&(dlImage->monitor));
	limitsAttributeInit(&(dlImage->limits));
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
	    } else if (!strcmp(token,"monitor")) {
		parseMonitor(displayInfo,&(dlImage->monitor));
	    } else if (!strcmp(token,"limits")) {
		parseLimits(displayInfo,&(dlImage->limits));
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
    fprintf(stream,"\n%s\ttype=\"%s\"",indent,
      stringValueTable[dlImage->imageType]);
    fprintf(stream,"\n%s\t\"image name\"=\"%s\"",indent,dlImage->imageName);
    writeDlMonitor(stream,&(dlImage->monitor),level+1);
    writeDlLimits(stream,&(dlImage->limits),level+1);
    fprintf(stream,"\n%s}",indent);
}

static void imageInheritValues(ResourceBundle *pRCB, DlElement *p)
{
    DlImage *dlImage = p->structure.image;
    medmGetValues(pRCB,
      RDBK_RC,       &(dlImage->monitor.rdbk),
      CLR_RC,        &(dlImage->monitor.clr),
      BCLR_RC,       &(dlImage->monitor.bclr),
      IMAGETYPE_RC,  &(dlImage->imageType),
      IMAGENAME_RC,  &(dlImage->imageName),
      LIMITS_RC,     &(dlImage->limits),
      -1);
}

static void imageGetLimits(DlElement *pE, DlLimits **ppL, char **pN)
{
    DlImage *dlImage = pE->structure.image;
    
    *(ppL) = &(dlImage->limits);
    *(pN) = dlImage->monitor.rdbk;
}

static void imageGetValues(ResourceBundle *pRCB, DlElement *p)
{
    DlImage *dlImage = p->structure.image;
    medmGetValues(pRCB,
      X_RC,          &(dlImage->object.x),
      Y_RC,          &(dlImage->object.y),
      WIDTH_RC,      &(dlImage->object.width),
      HEIGHT_RC,     &(dlImage->object.height),
      RDBK_RC,       &(dlImage->monitor.rdbk),
      CLR_RC,        &(dlImage->monitor.clr),
      BCLR_RC,       &(dlImage->monitor.bclr),
      IMAGETYPE_RC,  &(dlImage->imageType),
      IMAGENAME_RC,  &(dlImage->imageName),
      LIMITS_RC,     &(dlImage->limits),
      -1);
}

