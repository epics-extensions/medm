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
#define DEBUG_CALC 0

#define DEFAULT_TIME 1
#define ANIMATE_TIME(gif) \
  ((gif)->frames[CURFRAME(gif)]->DelayTime ? \
  ((gif)->frames[CURFRAME(gif)]->DelayTime)/100. : DEFAULT_TIME)

#include <ctype.h>
#include <postfix.h>
#include "medm.h"
#include "xgif.h"

#include <X11/keysym.h>

typedef struct _Image {
    DlElement *dlElement;     /* Must be first */
    Record **records;
    UpdateTask *updateTask;
    DisplayInfo *displayInfo;
    Boolean validCalc;
    Boolean animate;
    char post[MAX_TOKEN_LENGTH];
} MedmImage;

/* Function prototypes */

static void destroyDlImage(DisplayInfo *displayInfo, DlElement *pE);
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
static void imageFileSelectionCb(Widget w, XtPointer cd, XtPointer cbs);

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

Widget imageNameFSD;     /* Cannot be static - is initialized elsewhere */
static char imageName[MAX_TOKEN_LENGTH];

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
	char *pC, upstring[MAX_TOKEN_LENGTH];
	int i;
	
	pi = (MedmImage *)malloc(sizeof(MedmImage));
	pi->displayInfo = displayInfo;
	pi->dlElement = dlElement;
	pi->records = NULL;
	pi->updateTask = updateTaskAddTask(displayInfo, &(dlImage->object),
	  imageDraw, (XtPointer)pi);
	pi->validCalc = False;
	pi->animate = False;
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
	    
	  /* Calculate the postfix for visbilitiy calc */
	    calcPostfix(&dlImage->dynAttr);
	    setMonitorChanged(&dlImage->dynAttr, pi->records);
	  /* Calculate the postfix for the image calc */
	  /* First, check for ANIMATE in the string */
	    for(i=0; i < 4; i++) {
		upstring[i] = toupper(dlImage->calc[i]);
	    }
	    if(!strncmp(upstring,"ANIM",4)) {
	      /* Calc is set to animate */
		pi->animate = True;
		pi->validCalc = False;
	    } else {
		pi->animate = False;
		status=postfix(dlImage->calc, pi->post, &errnum);
		if(status) {
		    medmPostMsg(1,"executeDlImage:\n"
		      "  Invalid calc expression [error %d]: %s\n  for %s\n",
		      errnum, dlImage->calc, dlImage->dynAttr.chan);
		    pi->validCalc = False;
		} else {
		    pi->validCalc = True;
		}
	    }
	  /* Override the visibility monitorChanged paramaters if
             there is a valid image calc */
	    if(pi->validCalc) {
		for(i=0; i < MAX_CALC_RECORDS; i++) {
		    pi->records[i]->monitorValueChanged = True;
		}
	    }
#if 0	    
	  /* Draw initial white rectangle */
	  /* KE: Check if this is necessary */
	    drawWhiteRectangle(pi->updateTask);
#endif	    
	} else {
	  /* No channel */
	    if(gif) {
		if(gif->nFrames > 1) {
		    pi->animate = True;
		    updateTaskSetScanRate(pi->updateTask, ANIMATE_TIME(gif));
		} else {
		    pi->animate = False;
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
    int i;

  /* Branch on whether there is a channel or not */
    if(*dlImage->dynAttr.chan[0]) {
      /* A channel is defined */
	updateTaskSetScanRate(pi->updateTask, 0.0);
	if(!pr) return;
	if (pr->connected) {
	    if (pr->readAccess) {
		double result;
		long status;

		if(!pi->animate) {
		  /* Determine the result of the calculation */
		    if(!*dlImage->calc) {
		      /* calc string is empty */
			result=pr->value;
			status = 0;
		    } else if(!pi->validCalc) {
		      /* calc string is invalid */
			status = 1;
		    } else {
			double valueArray[MAX_CALC_INPUTS];
			Record **records = pi->records;
			DlDynamicAttribute *attr = &dlImage->dynAttr;;
			
		      /* Fill in the input array */
			for(i=0; i < MAX_CALC_RECORDS; i++) {
			    if(*attr->chan[i] && records[i]->connected) {
				valueArray[i] = records[i]->value;
			    } else {
				valueArray[i] = 0.0;
			    }
			}
			valueArray[4] = 0.0;              /* E: Reserved */
			valueArray[5] = 0.0;              /* F: Reserved */
			valueArray[6] = pr->elementCount; /* G: count */
			valueArray[7] = pr->hopr;         /* H: hopr */
			valueArray[8] = pr->status;       /* I: status */
			valueArray[9] = pr->severity;     /* J: severity */
			valueArray[10] = pr->precision;   /* K: precision */
			valueArray[11] = pr->lopr;        /* L: lopr */
			
		      /* Perform the calculation */
			status = calcPerform(valueArray, &result, pi->post);
			if(!status) {
			  /* Result is valid, convert to frame number */
			    if(gif) {
				if(result < 0.0) gif->curFrame = 0;
				else if(result > gif->nFrames-1)
				  gif->curFrame=gif->nFrames-1;
				else gif->curFrame = result +.5;
			    }
			}
		    }
#if DEBUG_CALC
		    print("imageDraw: calc=%s result=%g sevr=%d animate=%d\n",
		      dlImage->calc,result,pr->severity,pi->animate);
#endif		    
		} else {
		  /* Result is always valid for animate */
		    status = 0;
		}
	      /* Draw depending on visibility */
		if(status == 0) {
		    if(calcVisibility(&dlImage->dynAttr, pi->records)) {
			drawImage(pi);
		    } else {
			drawColoredRectangle(pi->updateTask,
			  displayInfo->colormap[
			    displayInfo->drawingAreaBackgroundColor]);
		    }
		} else {
		  /* Result is invalid */
		    drawColoredRectangle(pi->updateTask,
		      BlackPixel(display,screenNum));
		}
		if (pr->readAccess) {
		    if (!pi->updateTask->overlapped &&
		      dlImage->dynAttr.vis == V_STATIC) {
			pi->updateTask->opaque = True;
		    }
		} else {
		    pi->updateTask->opaque = False;
		    draw3DQuestionMark(pi->updateTask);
		}
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
	drawImage(pi);
      /* Don't update the drawing objects above (Could get to be
       * expensive) */
    }
}

static void drawImage(MedmImage *pi)
{
    DisplayInfo *displayInfo = pi->updateTask->displayInfo;
    DlImage *dlImage = pi->dlElement->structure.image;
    GIFData *gif = (GIFData *)dlImage->privateData;
    
    if(gif) {
	if(pi->animate) {
	  /* Draw the next image */
	    if(++gif->curFrame >= gif->nFrames) gif->curFrame = 0;
	    drawGIF(displayInfo, dlImage);
	  /* Reset the time */
	    updateTaskSetScanRate(pi->updateTask, ANIMATE_TIME(gif));
	} else {
	  /* Draw the image */
	    drawGIF(displayInfo, dlImage);
	  /* Reset the time */
	    updateTaskSetScanRate(pi->updateTask, 0.0);
	}
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

static void destroyDlImage(DisplayInfo *displayInfo, DlElement *pE)
{
    freeGIF(pE->structure.image);
    genericDestroy(displayInfo, pE);
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

/* Function which handles creation (and initial display) of images.
   It pops up file selection dialog and waits for the response (0 = No
   response, 1 = OK, 2 = Cancel) */
DlElement* handleImageCreate()
{
    DlElement *dlElement = NULL;
    XEvent event;
    static int response = 0;     /* Keep it off the stack */
    
    if(!(dlElement = createDlImage(NULL))) return dlElement;

  /* Create or manage the file selection dialog */
    if(imageNameFSD == NULL) {
	int n;
	Arg args[10];
	XmString gifDirMask = XmStringCreateLocalized("*.gif");
	
	n = 0;
	XtSetArg(args[n],XmNdirMask,gifDirMask); n++;
	XtSetArg(args[n],XmNdialogStyle,XmDIALOG_FULL_APPLICATION_MODAL); n++;
	imageNameFSD = XmCreateFileSelectionDialog(resourceMW,"imageNameFSD",args,n);
	XtAddCallback(imageNameFSD,XmNokCallback,imageFileSelectionCb,&response);
	XtAddCallback(imageNameFSD,XmNcancelCallback,imageFileSelectionCb,&response);
	XtManageChild(imageNameFSD);
	XmStringFree(gifDirMask);
    } else {
	XtManageChild(imageNameFSD);
    }

  /* This routine is called by handleRectangularCreates. We need to
     stay in this routine until the filename is chosen so the
     succeeding processing will be meaningful */
    while (!response || XtAppPending(appContext)) {
	XtAppNextEvent(appContext,&event);
	XtDispatchEvent(&event);
    }

  /* Check response */
    if(response == 2) {
      /* Cancel */
	destroyDlImage(currentDisplayInfo, dlElement);
	dlElement = NULL;
    } else {
      /* OK */
	DlImage *dlImage = dlElement->structure.image;

      /* Set the globalResourceBundle so inheritValues will work in
         the succeeding processing */
	dlImage->imageType = GIF_IMAGE;
	strcpy(dlImage->imageName, imageName);
    }
    
    return dlElement;
}

static void imageFileSelectionCb(Widget w, XtPointer clientData,
  XtPointer callData)
{
    int *response = (int *)clientData;
    XmFileSelectionBoxCallbackStruct *cbs =
      (XmFileSelectionBoxCallbackStruct *)callData;

    switch(cbs->reason){
    case XmCR_CANCEL:
	XtUnmanageChild(w);
	*response = 2;
	break;
    case XmCR_OK:
	if(cbs->value != NULL) {
	    char *fullPathName, *dirName;
	    int dirLength;
	    
	    *response = 1;
	    XmStringGetLtoR(cbs->value, XmFONTLIST_DEFAULT_TAG, &fullPathName);
	    XmStringGetLtoR(cbs->dir, XmSTRING_DEFAULT_CHARSET, &dirName);
	    dirLength = strlen(dirName);
#ifndef VMS
	  /* Copy the file name starting after the directory part */
	    strncpy(imageName, fullPathName + dirLength, MAX_TOKEN_LENGTH);
#else
	  /* Copy the whole file name */
	    strncpy(imageName, fullPathName, MAX_TOKEN_LENGTH);
#endif
	    imageName[MAX_TOKEN_LENGTH-1] = '\0';
	    XtFree(fullPathName);
	    XtUnmanageChild(w);
	} else {
	    *response = 2;
	}
	break;
    default:
	*response = 2;
	break;
    }
}

static void imageUpdateGraphicalInfoCb(XtPointer cd)
{
    Record *pr = (Record *)cd;
    MedmImage *pi = (MedmImage *)pr->clientData;
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
      IMAGE_CALC_RC, &(dlImage->calc),
      VIS_RC,        &(dlImage->dynAttr.vis),
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlImage->dynAttr.colorRule),
#endif
      VIS_CALC_RC,   &(dlImage->dynAttr.calc),
      CHAN_A_RC,     &(dlImage->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlImage->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlImage->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlImage->dynAttr.chan[3]),
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
      IMAGE_TYPE_RC, &(dlImage->imageType),
      IMAGE_NAME_RC, &(dlImage->imageName),
      IMAGE_CALC_RC, &(dlImage->calc),
      VIS_RC,        &(dlImage->dynAttr.vis),
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlImage->dynAttr.colorRule),
#endif
      VIS_CALC_RC,   &(dlImage->dynAttr.calc),
      CHAN_A_RC,     &(dlImage->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlImage->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlImage->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlImage->dynAttr.chan[3]),
      -1);
}

