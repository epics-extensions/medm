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


/***************************************************************************
 ****                        executeExtensions.c                        ****
 ***************************************************************************/


#include "medm.h"

#include "xgif.h"

#include <X11/keysym.h>

#define GIF_BTN  0
#define TIFF_BTN 1

Widget importFSD;
XmString gifDirMask, tifDirMask;
static void destroyDlImage(DlElement *);
static void imageGetValues(ResourceBundle *pRCB, DlElement *p);
static DlDispatchTable imageDlDispatchTable = {
    createDlImage,
    destroyDlImage,
    executeDlImage,
    writeDlImage,
    NULL,
    imageGetValues,
    NULL,
    NULL,
    NULL,
    genericMove,
    genericScale,
    NULL,
    NULL};

void executeDlImage(DisplayInfo *displayInfo, DlElement *dlElement)
{
    Arg args[10];
    GIFData *gif;
    DlImage *dlImage = dlElement->structure.image;

/* from the DlImage structure, we've got the image's dimensions */
    switch (dlImage->imageType) {
    case GIF_IMAGE:
	if (dlImage->privateData == NULL) {
	    if (!initializeGIF(displayInfo,dlImage)) {
	      /* something failed in there - bail out! */
		if (dlImage->privateData != NULL) {
		    free((char *)dlImage->privateData);
		    dlImage->privateData = NULL;
		}
	    }
	} else {
	    gif = (GIFData *) dlImage->privateData;
	    if (gif != NULL) {
		if (dlImage->object.width == gif->currentWidth &&
		  dlImage->object.height == gif->currentHeight) {
		    drawGIF(displayInfo,dlImage);
		} else {
		    resizeGIF(displayInfo,dlImage);
		    drawGIF(displayInfo,dlImage);
		}
	    }
	}
	break;
    }


}

static void imageTypeCallback(
  Widget w,
  int buttonNumber,
  XmToggleButtonCallbackStruct *call_data)
{
    Widget fsb;
    Arg args[4];

/* since both on & off will invoke this callback, only care about the
   transition of one to ON */
    if (call_data->set == False) return;

    fsb = XtParent(XtParent(XtParent(XtParent(w))));
    switch(buttonNumber) {
    case GIF_BTN:
	XtSetArg(args[0],XmNdirMask,gifDirMask);
	XtSetValues(fsb,args,1);
	globalResourceBundle.imageType = GIF_IMAGE;
	break;
    case TIFF_BTN:
	XtSetArg(args[0],XmNdirMask,tifDirMask);
	XtSetValues(fsb,args,1);
	globalResourceBundle.imageType = TIFF_IMAGE;
	break;
    }
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
	    DlElement **array;
	    DlImage *dlImage;
	    if (!dlElement) return;
	    dlImage = dlElement->structure.image;

	    XmStringGetLtoR(call_data->value,XmSTRING_DEFAULT_CHARSET,&fullPathName);
	    XmStringGetLtoR(call_data->dir,XmSTRING_DEFAULT_CHARSET,&dirName);
	    dirLength = strlen(dirName);
	    XtUnmanageChild(w);
	    strcpy(dlImage->imageName, &(fullPathName[dirLength]));
	    dlImage->imageType = GIF_IMAGE;
	    (dlElement->run->execute)(currentDisplayInfo, dlElement);
	  /* now select this element for resource edits */
#if 0
	    highlightAndSetSelectedElements(NULL,0,0);
#endif
	    clearResourcePaletteEntries();
	    array = (DlElement **)malloc(1*sizeof(DlElement *));
	    array[0] = dlElement;
#if 0
	    highlightAndSetSelectedElements(array,1,1);
#endif
	    setResourcePaletteEntries();

	}
	XtFree(fullPathName);
	XtFree(dirName);
	break;
    }

}

/*
 * function which handles creation (and initial display) of images
 */
DlElement* handleImageCreate()
{
    XmString buttons[NUM_IMAGE_TYPES-1];
    XmButtonType buttonType[NUM_IMAGE_TYPES-1];
    Widget radioBox, form, frame, typeLabel;
    int i, n;
    Arg args[10];
    static DlElement *dlElement = 0;
    if (!(dlElement = createDlImage(NULL))) return 0;

    if (importFSD == NULL) {
/* since GIF is the default, need dirMask to match */
	gifDirMask = XmStringCreateSimple("*.gif");
	tifDirMask = XmStringCreateSimple("*.tif");
	globalResourceBundle.imageType = GIF_IMAGE;
	n = 0;
	XtSetArg(args[n],XmNdirMask,gifDirMask); n++;
	XtSetArg(args[n],XmNdialogStyle,XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
	importFSD = XmCreateFileSelectionDialog(resourceMW,"importFSD",args,n);
	XtAddCallback(importFSD,XmNokCallback,importCallback,&dlElement);
	XtAddCallback(importFSD,XmNcancelCallback,importCallback,NULL);
	form = XmCreateForm(importFSD,"form",NULL,0);
	XtManageChild(form);
	n = 0;
	XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
	XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM); n++;
	typeLabel = XmCreateLabel(form,"typeLabel",args,n);
	XtManageChild(typeLabel);
	n = 0;
	XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
	XtSetArg(args[n],XmNleftAttachment,XmATTACH_WIDGET); n++;
	XtSetArg(args[n],XmNleftWidget,typeLabel); n++;
	frame = XmCreateFrame(form,"frame",args,n);
	XtManageChild(frame);

	buttons[0] = XmStringCreateSimple("GIF");
	buttons[1] = XmStringCreateSimple("TIFF");
	n = 0;
/* MDA this will be 2 when TIFF is implemented
   XtSetArg(args[n],XmNbuttonCount,2); n++;
   */
	XtSetArg(args[n],XmNbuttonCount,1); n++;
	XtSetArg(args[n],XmNbuttons,buttons); n++;
	XtSetArg(args[n],XmNbuttonSet,GIF_BTN); n++;
	XtSetArg(args[n],XmNsimpleCallback,imageTypeCallback); n++;
	radioBox = XmCreateSimpleRadioBox(frame,"radioBox",args,n);
	XtManageChild(radioBox);
	for (i = 0; i < 2; i++) XmStringFree(buttons[i]);
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

    dlImage = (DlImage *) malloc(sizeof(DlImage));
    if (!dlImage) return 0;
    if (p) {
	*dlImage = *p->structure.image;
	dlImage->privateData = NULL;
    } else {
	objectAttributeInit(&(dlImage->object));
	dlImage->imageType = NO_IMAGE;
	dlImage->imageName[0] = '\0';
	dlImage->privateData = 0;
    }


    if (!(dlElement = createDlElement(DL_Image,
      (XtPointer)      dlImage,
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
	    if (!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlImage->object));
	    } else if (!strcmp(token,"type")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"none"))
		  dlImage->imageType = NO_IMAGE;
		else if (!strcmp(token,"gif"))
		  dlImage->imageType = GIF_IMAGE;
		else if (!strcmp(token,"tiff"))
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

  /* just to be safe, initialize privateData member separately */
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
    fprintf(stream,"\n%s}",indent);
}

static void imageGetValues(ResourceBundle *pRCB, DlElement *p) {
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

static void destroyDlImage(DlElement *dlElement) {
    freeGIF(dlElement->structure.image);
    free((char*)dlElement->structure.composite);
    destroyDlElement(dlElement);
}
