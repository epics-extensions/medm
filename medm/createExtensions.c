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
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (708-252-2000).
*/
/*****************************************************************************
 *
 *     Original Author : Mark Andersion
 *     Current Author  : Frederick Vong
 *
 * Modification Log:
 * -----------------
 * .01  03-01-95        vong    2.0.0 release
 * .02  09-05-95        vong    2.1.0 release
 *                              correct the falling line and rising line to
 *                              polyline geometry calculation
 *
 *****************************************************************************
*/

#include "../medm/medm.h"


/* from utils.c - get XOR GC */
extern GC xorGC;

Widget importFSD;
XmString gifDirMask, tifDirMask;


#define GIF_BTN  0
#define TIFF_BTN 1

extern Widget objectFilePDM;
extern XmString xmstringSelect;




static void imageTypeCallback(
  Widget w,
  int buttonNumber,
  XmToggleButtonCallbackStruct *call_data)
{
  Widget fsb;
  Arg args[4];
  int n;

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



static XtCallbackProc importCallback(
  Widget w,
  XtPointer client_data,
  XmFileSelectionBoxCallbackStruct *call_data)
{
  char *fullPathName, *dirName;
  Dimension width, height;
  DlElement *element, **array;
  int dirLength;


  element = (DlElement *) NULL;

  switch(call_data->reason){

	case XmCR_CANCEL:
		XtUnmanageChild(w);
		break;

	case XmCR_OK:
		if (call_data->value != NULL && call_data->dir != NULL) {
		  XmStringGetLtoR(call_data->value,XmSTRING_DEFAULT_CHARSET,
			&fullPathName);
		  XmStringGetLtoR(call_data->dir,XmSTRING_DEFAULT_CHARSET,
			&dirName);
		  dirLength = strlen(dirName);
		  XtUnmanageChild(w);
		  switch (globalResourceBundle.imageType) {
		    case GIF_IMAGE:
			if (currentDisplayInfo == NULL) {
			   /* doing and implicit main File "New" */
			    currentDisplayInfo = createDisplay();
			    XtManageChild(currentDisplayInfo->drawingArea);
			} else {
			    /* update globalResourceBundle and execute */
			    strcpy(globalResourceBundle.imageName,
					&(fullPathName[dirLength]));
			    globalResourceBundle.imageType = GIF_IMAGE;
			    element = createDlImage(currentDisplayInfo);
			    (*element->dmExecute)(currentDisplayInfo,
						element->structure.image,FALSE);

			    /* now select this element for resource edits */
			    highlightAndSetSelectedElements(NULL,0,0);
			    clearResourcePaletteEntries();
			    array = (DlElement **)malloc(1*sizeof(DlElement *));
			    array[0] = element;
			    highlightAndSetSelectedElements(array,1,1);
			    setResourcePaletteEntries();

			}
			XtFree(fullPathName);
			XtFree(dirName);
			break;

		    case TIFF_IMAGE:
			printf("\nI'm a TIFF file");
			break;
		  }
		}
		break;
  }

}






DlElement *createDlImage(
  DisplayInfo *displayInfo)
{
  DlImage *dlImage;
  DlElement *dlElement;


  dlImage = (DlImage *) calloc(1,sizeof(DlImage));
  createDlObject(displayInfo,&(dlImage->object));
  dlImage->imageType = globalResourceBundle.imageType;
  strcpy(dlImage->imageName,globalResourceBundle.imageName);

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Image;
  dlElement->structure.image = dlImage;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlImage;
  dlElement->dmWrite = (void(*)())writeDlImage;

  return(dlElement);
}




/*
 * function which handles creation (and initial display) of images
 */
void handleImageCreate()
{
  XmString buttons[NUM_IMAGE_TYPES-1];
  XmButtonType buttonType[NUM_IMAGE_TYPES-1];
  Widget radioBox, form, frame, typeLabel;
  int i, n;
  Arg args[10];

  if (importFSD == NULL) {
/* since GIF is the default, need dirMask to match */
    gifDirMask = XmStringCreateSimple("*.gif");
    tifDirMask = XmStringCreateSimple("*.tif");
    globalResourceBundle.imageType = GIF_IMAGE;
    n = 0;
    XtSetArg(args[n],XmNdirMask,gifDirMask); n++;
    XtSetArg(args[n],XmNdialogStyle,
    XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
    importFSD = XmCreateFileSelectionDialog(resourceMW,"importFSD",args,n);
    XtAddCallback(importFSD,XmNokCallback,
		(XtCallbackProc)importCallback,NULL);
    XtAddCallback(importFSD,XmNcancelCallback,
		(XtCallbackProc)importCallback,NULL);
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
    XtSetArg(args[n],XmNsimpleCallback,(XtCallbackProc)imageTypeCallback); n++;
    radioBox = XmCreateSimpleRadioBox(frame,"radioBox",args,n);
    XtManageChild(radioBox);
    for (i = 0; i < 2; i++) XmStringFree(buttons[i]);
    XtManageChild(importFSD);
  } else {
    XtManageChild(importFSD);
 }

}



/*
 * createDlComposite function - really doing the "Group" function on existing
 *	display list elements (and doing implicit unselect of previously
 *	selected elements and select of the newly created Composite)
 */
DlElement *createDlComposite(DisplayInfo *displayInfo)
{
  DlComposite *dlComposite;
  DlElement *dlElement, *elementPtr;
  int i, minX, minY, maxX, maxY;
  DlElement **array;
  DlElement *basic, *dyn, *newCompositePosition;
  Boolean foundFirstSelected;

/* composites only exist if members exist:  numSelectedElements must be > 0 */
  if (displayInfo->numSelectedElements <= 0) return ((DlElement *)NULL);


  dlComposite = (DlComposite *) malloc(sizeof(DlComposite));
  strcpy(dlComposite->compositeName,globalResourceBundle.compositeName);
  strcpy(dlComposite->chan,globalResourceBundle.chan);

/*  for now - only support static visibility of composites
  dlComposite->vis = globalResourceBundle.vis;
*/
  dlComposite->vis = V_STATIC;

  dlComposite->visible = True;
  dlComposite->monitorAlreadyAdded = False;
  dlComposite->dlElementListHead = (XtPointer)calloc((size_t)1,
					sizeof(DlElement));
  ((DlElement *)(dlComposite->dlElementListHead))->next = NULL;
  dlComposite->dlElementListTail = dlComposite->dlElementListHead;


  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Composite;
  dlElement->structure.composite = dlComposite;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlComposite;
  dlElement->dmWrite = (void(*)())writeDlComposite;

/*
 * Save the position of first element in "visibility space" (but not 
 *  necessarily the last element in the selected elements array);
 *  since selects can be multiple selects appended to by Shift-MB1, the
 *  relative order in the selectedElementsArray[] can be not "proper"
 *  -- start at dlColormap element and move forward 'til first selected
 *      element is found
 */
  newCompositePosition = displayInfo->dlColormapElement->next;
  foundFirstSelected = False;
  while (!foundFirstSelected) {
    for (i = 0; i < displayInfo->numSelectedElements; i++) {
      if (newCompositePosition == displayInfo->selectedElementsArray[i]) {
	foundFirstSelected = True;
	break;	/* out of for loop */
      }
    }
    if (!foundFirstSelected) newCompositePosition = newCompositePosition->next;
  }
  newCompositePosition = newCompositePosition->prev;
  while( (newCompositePosition->type == DL_DynamicAttribute ||
	  newCompositePosition->type == DL_BasicAttribute) &&
	  newCompositePosition->type != DL_Colormap)
	newCompositePosition = newCompositePosition->prev;






/*
 *  now loop over all selected elements and and determine x/y/width/height
 *    of the newly created composite
 */
  minX = INT_MAX; minY = INT_MAX;
  maxX = INT_MIN; maxY = INT_MIN;


  for (i = displayInfo->numSelectedElements - 1; i >= 0; i--) {
/* get object data: must have object entry  - use rectangle type (arbitrary) */
    elementPtr = displayInfo->selectedElementsArray[i];

  /* don't group in display */
    if (elementPtr->type != DL_Display) {

      minX = MIN(minX,elementPtr->structure.rectangle->object.x);
      maxX = MAX(maxX,elementPtr->structure.rectangle->object.x +
		(int)elementPtr->structure.rectangle->object.width);
      minY = MIN(minY,elementPtr->structure.rectangle->object.y);
      maxY = MAX(maxY,elementPtr->structure.rectangle->object.y +
		(int)elementPtr->structure.rectangle->object.height);

      if (! ELEMENT_HAS_WIDGET(elementPtr->type) &&
	elementPtr->type != DL_TextUpdate && elementPtr->type != DL_Composite){
/* get basicAttribute data */
        basic = lookupBasicAttributeElement(elementPtr);
        if (basic != NULL) {
	  moveElementAfter(displayInfo,dlElement->structure.composite,basic,
		(DlElement *)dlElement->structure.composite->dlElementListTail);
        }

/* get dynamicAttribute data */
        dyn = lookupDynamicAttributeElement(elementPtr);
        if (dyn != NULL) {
	  moveElementAfter(displayInfo,dlElement->structure.composite,dyn,
		(DlElement *)dlElement->structure.composite->dlElementListTail);
        }
      }
      moveElementAfter(displayInfo,dlElement->structure.composite,elementPtr,
		(DlElement *)dlElement->structure.composite->dlElementListTail);

    }
  }

  dlComposite->object.x = minX;
  dlComposite->object.y = minY;
  dlComposite->object.width = maxX - minX;
  dlComposite->object.height = maxY - minY;


/* move the newly created element to just before the first child element */
  moveElementAfter(displayInfo,NULL,dlElement,newCompositePosition);

/*
 * now select this newly created Composite/group (this unselects the previously
 *	selected children, etc)
 */
  highlightAndSetSelectedElements(NULL,0,0);
  clearResourcePaletteEntries();
  array = (DlElement **)malloc(1*sizeof(DlElement *));
  array[0] = dlElement;
  highlightAndSetSelectedElements(array,1,1);
  currentActionType = SELECT_ACTION;
  currentElementType = DL_Composite;
  setResourcePaletteEntries();


  return(dlElement);

}




DlElement *createDlPolyline(
  DisplayInfo *displayInfo)
{
  DlPolyline *dlPolyline;
  DlElement *dlElement;


  dlPolyline = (DlPolyline *) calloc(1,sizeof(DlPolyline));
  createDlObject(displayInfo,&(dlPolyline->object));
  dlPolyline->points = NULL;
  dlPolyline->nPoints = 0;
  dlPolyline->isFallingOrRisingLine = False;

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Polyline;
  dlElement->structure.polyline = dlPolyline;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlPolyline;
  dlElement->dmWrite = (void(*)())writeDlPolyline;

  return(dlElement);
}


/* for use in handlePoly*Create() functions */
const double okRadiansTable[24]  =    { 0.,
    					1.*M_PI/4., 1.*M_PI/4.,
    					2.*M_PI/4., 2.*M_PI/4.,
    					3.*M_PI/4., 3.*M_PI/4.,
    					4.*M_PI/4., 4.*M_PI/4.,
    					5.*M_PI/4., 5.*M_PI/4.,
    					6.*M_PI/4., 6.*M_PI/4.,
    					7.*M_PI/4., 7.*M_PI/4.,
    					0.};


/*
 * create a polyline - if Boolean simpleLine is True then want a simple
 *  (2 point) line, else create and add points to the polyline until
 *  the user enters a double click
 */

DlElement *handlePolylineCreate(
  int x0, int y0, Boolean simpleLine)
{
  XEvent event, newEvent;
  int newEventType;
  Window window;
  DlPolyline dlPolyline;
  DlElement *element, **array;
#define INITIAL_NUM_POINTS 16
  int pointsArraySize = INITIAL_NUM_POINTS;
  int i, minX, maxX, minY, maxY;
  int x01, y01;

  int deltaX, deltaY, okIndex;
  double radians, okRadians, length;

/* create a basic attribute */
  element = createDlBasicAttribute(currentDisplayInfo);
  (*element->dmExecute)(currentDisplayInfo,
                        element->structure.basicAttribute,FALSE);
/* create a dynamic attribute if appropriate */
  if (strlen(globalResourceBundle.chan) > 0 &&
      globalResourceBundle.vis != V_STATIC) {
    element = createDlDynamicAttribute(currentDisplayInfo);
    (*element->dmExecute)(currentDisplayInfo,
                        element->structure.dynamicAttribute,FALSE);
  }


  window = XtWindow(currentDisplayInfo->drawingArea);
  element = (DlElement *) NULL;
  dlPolyline.object.x = x0;
  dlPolyline.object.y = y0;
  dlPolyline.object.width = 0;
  dlPolyline.object.height = 0;

  globalResourceBundle.x = x0;
  globalResourceBundle.y = y0;

  minX = INT_MAX; maxX = INT_MIN; minY = INT_MAX; maxY = INT_MIN;
  
/* first click is first point... */
  dlPolyline.nPoints = 1;
  if (simpleLine) {
    dlPolyline.points = (XPoint *)malloc(2*sizeof(XPoint));
  } else {
    dlPolyline.points = (XPoint *)malloc((pointsArraySize+1)*sizeof(XPoint));
  }
  dlPolyline.points[0].x = (short)x0;
  dlPolyline.points[0].y = (short)y0;
  x01 = x0; y01 = y0;

  XGrabPointer(display,window,FALSE,
	PointerMotionMask|ButtonMotionMask|ButtonPressMask|ButtonReleaseMask,
	GrabModeAsync,GrabModeAsync,None,None,CurrentTime);
  XGrabServer(display);


/* now loop until button is double-clicked (or until 2 points if simpleLine) */
  while (TRUE) {

      XtAppNextEvent(appContext,&event);

      switch(event.type) {

	case ButtonPress:
	  if (!simpleLine) {
	/* need double click to end */
	    XWindowEvent(display,XtWindow(currentDisplayInfo->drawingArea),
		ButtonPressMask|Button1MotionMask|PointerMotionMask,&newEvent);
	    newEventType = newEvent.type;
	  } else {
	/* just need second point, set type so we can terminate wi/ 2 points */
	    newEventType = ButtonPress;
	  }
	  if (newEventType == ButtonPress) {

  /* -> double click... add last point and leave here */
	    if (event.xbutton.state & ShiftMask) {
/* constrain to 45 degree increments */
	      deltaX = event.xbutton.x -
			dlPolyline.points[dlPolyline.nPoints-1].x;
	      deltaY = event.xbutton.y -
			dlPolyline.points[dlPolyline.nPoints-1].y;
	      length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
	      radians = atan2((double)(deltaY),(double)(deltaX));
	/* use positive radians */
	      if (radians < 0.) radians = 2*M_PI + radians;
	      okIndex = (int)((radians*8.0)/M_PI);
	      okRadians = okRadiansTable[okIndex];
	      x01 = (int) (cos(okRadians)*length)
			 + dlPolyline.points[dlPolyline.nPoints-1].x;
	      y01 = (int) (sin(okRadians)*length)
			 + dlPolyline.points[dlPolyline.nPoints-1].y;
	      dlPolyline.nPoints++;
	      dlPolyline.points[dlPolyline.nPoints-1].x = x01;
	      dlPolyline.points[dlPolyline.nPoints-1].y = y01;
	    } else {
/* unconstrained */
	      dlPolyline.nPoints++;
	      dlPolyline.points[dlPolyline.nPoints-1].x = event.xbutton.x;
	      dlPolyline.points[dlPolyline.nPoints-1].y = event.xbutton.y;
	    }
	    XUngrabPointer(display,CurrentTime);
	    XUngrabServer(display);
	    for (i = 0; i < dlPolyline.nPoints; i++) {
	      minX = MIN(minX,dlPolyline.points[i].x);
	      maxX = MAX(maxX,dlPolyline.points[i].x);
	      minY = MIN(minY,dlPolyline.points[i].y);
	      maxY = MAX(maxY,dlPolyline.points[i].y);
	    }
	    dlPolyline.object.x = minX - globalResourceBundle.lineWidth/2;
	    dlPolyline.object.y = minY - globalResourceBundle.lineWidth/2;
	    dlPolyline.object.width = maxX - minX
					+ globalResourceBundle.lineWidth;
	    dlPolyline.object.height = maxY - minY
					+ globalResourceBundle.lineWidth;
	  /* update global resource bundle  - then do create out of it */
	    globalResourceBundle.x = dlPolyline.object.x;
	    globalResourceBundle.y = dlPolyline.object.y;
	    globalResourceBundle.width = dlPolyline.object.width;
	    globalResourceBundle.height = dlPolyline.object.height;
	    element = createDlPolyline(currentDisplayInfo);
	  /* update nPoints and assign the points array to the polyline */
	    element->structure.polyline->nPoints = dlPolyline.nPoints;
	    element->structure.polyline->points = dlPolyline.points;
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.polyline,FALSE);
	    XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
			XtWindow(currentDisplayInfo->drawingArea),
			currentDisplayInfo->pixmapGC,
			dlPolyline.object.x, dlPolyline.object.y,
			dlPolyline.object.width, dlPolyline.object.height,
			dlPolyline.object.x, dlPolyline.object.y);
	    XBell(display,50); XBell(display,50);
	/* now select and highlight this element */
	    highlightAndSetSelectedElements(NULL,0,0);
	    clearResourcePaletteEntries();
	    array = (DlElement **) malloc(1*sizeof(DlElement *));
	    array[0] = element;
	    highlightAndSetSelectedElements(array,1,1);
	    setResourcePaletteEntries();
	    return (element);

	  } else {

/* not last point: more points to add.. */
	/* undraw old last line segment */
	    XDrawLine(display,window,xorGC,
		dlPolyline.points[dlPolyline.nPoints-1].x,
		dlPolyline.points[dlPolyline.nPoints-1].y,
		x01, y01);
	/* new line segment added: update coordinates */
	    if (dlPolyline.nPoints >= pointsArraySize) {
	    /* reallocate the points array: enlarge by 4X, etc */
		pointsArraySize *= 4;
		dlPolyline.points = (XPoint *)realloc(dlPolyline.points,
					(pointsArraySize+1)*sizeof(XPoint));
	    }

	    if (event.xbutton.state & ShiftMask) {
/* constrain to 45 degree increments */
	      deltaX = event.xbutton.x -
			dlPolyline.points[dlPolyline.nPoints-1].x;
	      deltaY = event.xbutton.y -
			dlPolyline.points[dlPolyline.nPoints-1].y;
	      length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
	      radians = atan2((double)(deltaY),(double)(deltaX));
	/* use positive radians */
	      if (radians < 0.) radians = 2*M_PI + radians;
	      okIndex = (int)((radians*8.0)/M_PI);
	      okRadians = okRadiansTable[okIndex];
	      x01 = (int) (cos(okRadians)*length)
			 + dlPolyline.points[dlPolyline.nPoints-1].x;
	      y01 = (int) (sin(okRadians)*length)
			 + dlPolyline.points[dlPolyline.nPoints-1].y;
	      dlPolyline.nPoints++;
	      dlPolyline.points[dlPolyline.nPoints-1].x = x01;
	      dlPolyline.points[dlPolyline.nPoints-1].y = y01;
	    } else {
/* unconstrained */
	      dlPolyline.nPoints++;
	      dlPolyline.points[dlPolyline.nPoints-1].x = event.xbutton.x;
	      dlPolyline.points[dlPolyline.nPoints-1].y = event.xbutton.y;
	      x01 = event.xbutton.x; y01 = event.xbutton.y;
	    }
	/* draw new line segment */
	    XDrawLine(display,window,xorGC,
		dlPolyline.points[dlPolyline.nPoints-2].x,
		dlPolyline.points[dlPolyline.nPoints-2].y,
		dlPolyline.points[dlPolyline.nPoints-1].x,
		dlPolyline.points[dlPolyline.nPoints-1].y);
	  }
	  break;

	case MotionNotify:
	/* undraw old last line segment */
	  XDrawLine(display,window,xorGC,
		dlPolyline.points[dlPolyline.nPoints-1].x,
		dlPolyline.points[dlPolyline.nPoints-1].y,
		x01, y01);

	  if (event.xmotion.state & ShiftMask) {
/* constrain redraw to 45 degree increments */
	    deltaX = event.xmotion.x -
			dlPolyline.points[dlPolyline.nPoints-1].x;
	    deltaY = event.xmotion.y -
			dlPolyline.points[dlPolyline.nPoints-1].y;
	    length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
	    radians = atan2((double)(deltaY),(double)(deltaX));
	/* use positive radians */
	    if (radians < 0.) radians = 2*M_PI + radians;
	    okIndex = (int)((radians*8.0)/M_PI);
	    okRadians = okRadiansTable[okIndex];
	    x01 = (int) (cos(okRadians)*length)
			 + dlPolyline.points[dlPolyline.nPoints-1].x;
	    y01 = (int) (sin(okRadians)*length)
			 + dlPolyline.points[dlPolyline.nPoints-1].y;
	  } else {
/* unconstrained */
	    x01 = event.xmotion.x;
	    y01 = event.xmotion.y;
	  }
	/* draw new last line segment */
	  XDrawLine(display,window,xorGC,
		dlPolyline.points[dlPolyline.nPoints-1].x,
		dlPolyline.points[dlPolyline.nPoints-1].y,
		x01,y01);
	  break;

	default:
	  XtDispatchEvent(&event);
	  break;
     }
  }

}




DlElement *createDlPolygon(
  DisplayInfo *displayInfo)
{
  DlPolygon *dlPolygon;
  DlElement *dlElement;


  dlPolygon = (DlPolygon *) calloc(1,sizeof(DlPolygon));
  createDlObject(displayInfo,&(dlPolygon->object));
  dlPolygon->points = NULL;
  dlPolygon->nPoints = 0;

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Polygon;
  dlElement->structure.polygon = dlPolygon;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlPolygon;
  dlElement->dmWrite = (void(*)())writeDlPolygon;

  return(dlElement);
}





DlElement *handlePolygonCreate(
  int x0, int y0)
{
  XEvent event, newEvent;
  int newEventType;
  Window window;
  DlPolygon dlPolygon;
  DlElement *element, **array;
#define INITIAL_NUM_POINTS 16
  int pointsArraySize = INITIAL_NUM_POINTS;
  int i, minX, maxX, minY, maxY;
  int x01, y01;

  int deltaX, deltaY, okIndex;
  double radians, okRadians, length;

/* create a basic attribute */
  element = createDlBasicAttribute(currentDisplayInfo);
  (*element->dmExecute)(currentDisplayInfo,
                        element->structure.basicAttribute,FALSE);
/* create a dynamic attribute if appropriate */
  if (strlen(globalResourceBundle.chan) > 0 &&
      globalResourceBundle.vis != V_STATIC) {
    element = createDlDynamicAttribute(currentDisplayInfo);
    (*element->dmExecute)(currentDisplayInfo,
                        element->structure.dynamicAttribute,FALSE);
  }


  window = XtWindow(currentDisplayInfo->drawingArea);
  element = (DlElement *) NULL;
  dlPolygon.object.x = x0;
  dlPolygon.object.y = y0;
  dlPolygon.object.width = 0;
  dlPolygon.object.height = 0;

  globalResourceBundle.x = x0;
  globalResourceBundle.y = y0;

  minX = INT_MAX; maxX = INT_MIN; minY = INT_MAX; maxY = INT_MIN;
  
/* first click is first point... */
  dlPolygon.nPoints = 1;
  dlPolygon.points = (XPoint *)malloc((pointsArraySize+3)*sizeof(XPoint));
  dlPolygon.points[0].x = (short)x0;
  dlPolygon.points[0].y = (short)y0;
  x01 = x0; y01 = y0;

  XGrabPointer(display,window,FALSE,
	PointerMotionMask|ButtonMotionMask|ButtonPressMask|ButtonReleaseMask,
	GrabModeAsync,GrabModeAsync,None,None,CurrentTime);
  XGrabServer(display);


  /* now loop until button is double-clicked */
  while (TRUE) {

      XtAppNextEvent(appContext,&event);

      switch(event.type) {

	case ButtonPress:
	  XWindowEvent(display,XtWindow(currentDisplayInfo->drawingArea),
		ButtonPressMask|Button1MotionMask|PointerMotionMask,&newEvent);
	  newEventType = newEvent.type;
	  if (newEventType == ButtonPress) {

  /* -> double click... add last point and leave here */
	    if (event.xbutton.state & ShiftMask) {
/* constrain to 45 degree increments */
	      deltaX = event.xbutton.x -
			dlPolygon.points[dlPolygon.nPoints-1].x;
	      deltaY = event.xbutton.y -
			dlPolygon.points[dlPolygon.nPoints-1].y;
	      length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
	      radians = atan2((double)(deltaY),(double)(deltaX));
	/* use positive radians */
	      if (radians < 0.) radians = 2*M_PI + radians;
	      okIndex = (int)((radians*8.0)/M_PI);
	      okRadians = okRadiansTable[okIndex];
	      x01 = (int) (cos(okRadians)*length)
			 + dlPolygon.points[dlPolygon.nPoints-1].x;
	      y01 = (int) (sin(okRadians)*length)
			 + dlPolygon.points[dlPolygon.nPoints-1].y;
	      dlPolygon.nPoints++;
	      dlPolygon.points[dlPolygon.nPoints-1].x = x01;
	      dlPolygon.points[dlPolygon.nPoints-1].y = y01;
	    } else {
/* unconstrained */
	      dlPolygon.nPoints++;
	      dlPolygon.points[dlPolygon.nPoints-1].x = event.xbutton.x;
	      dlPolygon.points[dlPolygon.nPoints-1].y = event.xbutton.y;
	    }
	    XUngrabPointer(display,CurrentTime);
	    XUngrabServer(display);
	    for (i = 0; i < dlPolygon.nPoints; i++) {
	      minX = MIN(minX,dlPolygon.points[i].x);
	      maxX = MAX(maxX,dlPolygon.points[i].x);
	      minY = MIN(minY,dlPolygon.points[i].y);
	      maxY = MAX(maxY,dlPolygon.points[i].y);
	    }
	/* to ensure closure, make sure last point = first point */
	    if(!(dlPolygon.points[0].x ==
		dlPolygon.points[dlPolygon.nPoints-1].x &&
		dlPolygon.points[0].y ==
		dlPolygon.points[dlPolygon.nPoints-1].y)) {
	      dlPolygon.points[dlPolygon.nPoints] = dlPolygon.points[0];
	      dlPolygon.nPoints++;
            }

	    if (globalResourceBundle.fill == F_SOLID) {
		dlPolygon.object.x = minX;
 		dlPolygon.object.y = minY;
		dlPolygon.object.width = maxX - minX;
		dlPolygon.object.height = maxY - minY;
	    } else {            /* F_OUTLINE, therfore lineWidth is a factor */
		dlPolygon.object.x = minX - globalResourceBundle.lineWidth/2;
		dlPolygon.object.y = minY - globalResourceBundle.lineWidth/2;
		dlPolygon.object.width = maxX - minX
					+ globalResourceBundle.lineWidth;
		dlPolygon.object.height = maxY - minY
					+ globalResourceBundle.lineWidth;
	    }

	  /* update global resource bundle  - then do create out of it */
	    globalResourceBundle.x = dlPolygon.object.x;
	    globalResourceBundle.y = dlPolygon.object.y;
	    globalResourceBundle.width = dlPolygon.object.width;
	    globalResourceBundle.height = dlPolygon.object.height;
	    element = createDlPolygon(currentDisplayInfo);
	  /* update nPoints and assign the points array to the polygon */
	    element->structure.polygon->nPoints = dlPolygon.nPoints;
	    element->structure.polygon->points = dlPolygon.points;
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.polygon,FALSE);
	    XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
			XtWindow(currentDisplayInfo->drawingArea),
			currentDisplayInfo->pixmapGC,
			dlPolygon.object.x, dlPolygon.object.y,
			dlPolygon.object.width, dlPolygon.object.height,
			dlPolygon.object.x, dlPolygon.object.y);
	    XBell(display,50); XBell(display,50);
	/* now select and highlight this element */
	    highlightAndSetSelectedElements(NULL,0,0);
	    clearResourcePaletteEntries();
	    array = (DlElement **) malloc(1*sizeof(DlElement *));
	    array[0] = element;
	    highlightAndSetSelectedElements(array,1,1);
	    setResourcePaletteEntries();
	    return (element);

	  } else {

/* not last point: more points to add.. */
	/* undraw old line segments */
	    XDrawLine(display,window,xorGC,
		dlPolygon.points[dlPolygon.nPoints-1].x,
		dlPolygon.points[dlPolygon.nPoints-1].y,
		x01, y01);
	    if (dlPolygon.nPoints > 1) XDrawLine(display,window,xorGC,
			dlPolygon.points[0].x, dlPolygon.points[0].y, x01, y01);

	/* new line segment added: update coordinates */
	    if (dlPolygon.nPoints >= pointsArraySize) {
	    /* reallocate the points array: enlarge by 4X, etc */
		pointsArraySize *= 4;
		dlPolygon.points = (XPoint *)realloc(dlPolygon.points,
					(pointsArraySize+3)*sizeof(XPoint));
	    }

	    if (event.xbutton.state & ShiftMask) {
/* constrain to 45 degree increments */
	      deltaX = event.xbutton.x -
			dlPolygon.points[dlPolygon.nPoints-1].x;
	      deltaY = event.xbutton.y -
			dlPolygon.points[dlPolygon.nPoints-1].y;
	      length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
	      radians = atan2((double)(deltaY),(double)(deltaX));
	/* use positive radians */
	      if (radians < 0.) radians = 2*M_PI + radians;
	      okIndex = (int)((radians*8.0)/M_PI);
	      okRadians = okRadiansTable[okIndex];
	      x01 = (int) (cos(okRadians)*length)
			 + dlPolygon.points[dlPolygon.nPoints-1].x;
	      y01 = (int) (sin(okRadians)*length)
			 + dlPolygon.points[dlPolygon.nPoints-1].y;
	      dlPolygon.nPoints++;
	      dlPolygon.points[dlPolygon.nPoints-1].x = x01;
	      dlPolygon.points[dlPolygon.nPoints-1].y = y01;
	    } else {
/* unconstrained */
	      dlPolygon.nPoints++;
	      dlPolygon.points[dlPolygon.nPoints-1].x = event.xbutton.x;
	      dlPolygon.points[dlPolygon.nPoints-1].y = event.xbutton.y;
	      x01 = event.xbutton.x; y01 = event.xbutton.y;
	    }
	/* draw new line segments */
	    XDrawLine(display,window,xorGC,
		dlPolygon.points[dlPolygon.nPoints-2].x,
		dlPolygon.points[dlPolygon.nPoints-2].y,
		dlPolygon.points[dlPolygon.nPoints-1].x,
		dlPolygon.points[dlPolygon.nPoints-1].y);
	    if (dlPolygon.nPoints > 1) XDrawLine(display,window,xorGC,
			dlPolygon.points[0].x,dlPolygon.points[0].y,x01,y01);
	  }
	  break;

	case MotionNotify:
	/* undraw old line segments */
	  XDrawLine(display,window,xorGC,
		dlPolygon.points[dlPolygon.nPoints-1].x,
		dlPolygon.points[dlPolygon.nPoints-1].y,
		x01, y01);
	  if (dlPolygon.nPoints > 1) XDrawLine(display,window,xorGC,
			dlPolygon.points[0].x,dlPolygon.points[0].y, x01, y01);


	  if (event.xmotion.state & ShiftMask) {
/* constrain redraw to 45 degree increments */
	    deltaX = event.xmotion.x -
			dlPolygon.points[dlPolygon.nPoints-1].x;
	    deltaY = event.xmotion.y -
			dlPolygon.points[dlPolygon.nPoints-1].y;
	    length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
	    radians = atan2((double)(deltaY),(double)(deltaX));
	/* use positive radians */
	    if (radians < 0.) radians = 2*M_PI + radians;
	    okIndex = (int)((radians*8.0)/M_PI);
	    okRadians = okRadiansTable[okIndex];
	    x01 = (int) (cos(okRadians)*length)
			 + dlPolygon.points[dlPolygon.nPoints-1].x;
	    y01 = (int) (sin(okRadians)*length)
			 + dlPolygon.points[dlPolygon.nPoints-1].y;
	  } else {
/* unconstrained */
	    x01 = event.xmotion.x;
	    y01 = event.xmotion.y;
	  }
	/* draw new last line segment */
	  XDrawLine(display,window,xorGC,
		dlPolygon.points[dlPolygon.nPoints-1].x,
		dlPolygon.points[dlPolygon.nPoints-1].y,
		x01,y01);
	  if (dlPolygon.nPoints > 1) XDrawLine(display,window,xorGC,
			dlPolygon.points[0].x,dlPolygon.points[0].y, x01,y01);

	  break;

	default:
	  XtDispatchEvent(&event);
	  break;
     }
  }

}




/*
 * manipulate a polyline vertex
 */

void handlePolylineVertexManipulation(
  DlPolyline *dlPolyline,
  int pointIndex)
{
  XEvent event, newEvent;
  int newEventType;
  Window window;
  int i, minX, maxX, minY, maxY;
  int x01, y01;

  int deltaX, deltaY, okIndex;
  double radians, okRadians, length;

  window = XtWindow(currentDisplayInfo->drawingArea);
  minX = INT_MAX; maxX = INT_MIN; minY = INT_MAX; maxY = INT_MIN;
  
  x01 = dlPolyline->points[pointIndex].x;
  y01 = dlPolyline->points[pointIndex].y;

  XGrabPointer(display,window,FALSE,
	PointerMotionMask|ButtonMotionMask|ButtonReleaseMask,
	GrabModeAsync,GrabModeAsync,None,None,CurrentTime);
  XGrabServer(display);


/* now loop until button is released */
  while (TRUE) {

      XtAppNextEvent(appContext,&event);

      switch(event.type) {

	case ButtonRelease:

  /* modify point and leave here */
	    if (event.xbutton.state & ShiftMask) {
/* constrain to 45 degree increments */
	      deltaX = event.xmotion.x - dlPolyline->points[
		    (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].x;
	      deltaY = event.xmotion.y - dlPolyline->points[
		    (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].y;
	      length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
	      radians = atan2((double)(deltaY),(double)(deltaX));
	/* use positive radians */
	      if (radians < 0.) radians = 2*M_PI + radians;
	      okIndex = (int)((radians*8.0)/M_PI);
	      okRadians = okRadiansTable[okIndex];
	      x01 = (int) (cos(okRadians)*length) + dlPolyline->points[
		    (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].x;
	      y01 = (int) (sin(okRadians)*length) + dlPolyline->points[
		    (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].y;
	      dlPolyline->points[pointIndex].x = x01;
	      dlPolyline->points[pointIndex].y = y01;
	    } else {
/* unconstrained */
	      dlPolyline->points[pointIndex].x = event.xbutton.x;
	      dlPolyline->points[pointIndex].y = event.xbutton.y;
	    }
	    XUngrabPointer(display,CurrentTime);
	    XUngrabServer(display);
	    for (i = 0; i < dlPolyline->nPoints; i++) {
	      minX = MIN(minX,dlPolyline->points[i].x);
	      maxX = MAX(maxX,dlPolyline->points[i].x);
	      minY = MIN(minY,dlPolyline->points[i].y);
	      maxY = MAX(maxY,dlPolyline->points[i].y);
	    }
	    dlPolyline->object.x = minX - globalResourceBundle.lineWidth/2;
	    dlPolyline->object.y = minY - globalResourceBundle.lineWidth/2;
	    dlPolyline->object.width = maxX - minX
					+ globalResourceBundle.lineWidth;
	    dlPolyline->object.height = maxY - minY
					+ globalResourceBundle.lineWidth;
	  /* update global resource bundle  - then do create out of it */
	    globalResourceBundle.x = dlPolyline->object.x;
	    globalResourceBundle.y = dlPolyline->object.y;
	    globalResourceBundle.width = dlPolyline->object.width;
	    globalResourceBundle.height = dlPolyline->object.height;

	    dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
/* since dmTraverseNonWidgets... clears the window, redraw highlights */
	    highlightSelectedElements();

	    return;
	  break;


	case MotionNotify:
	/* undraw old line segments */
	  if (pointIndex > 0)
	      XDrawLine(display,window,xorGC,
		dlPolyline->points[pointIndex-1].x,
		dlPolyline->points[pointIndex-1].y, x01,y01);
	  if (pointIndex < dlPolyline->nPoints-1)
	      XDrawLine(display,window,xorGC,
		dlPolyline->points[pointIndex+1].x,
		dlPolyline->points[pointIndex+1].y, x01,y01);

	  if (event.xmotion.state & ShiftMask) {
/* constrain redraw to 45 degree increments */
	    deltaX = event.xmotion.x - dlPolyline->points[
		    (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].x;
	    deltaY = event.xmotion.y - dlPolyline->points[
		    (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].y;
	    length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
	    radians = atan2((double)(deltaY),(double)(deltaX));
	/* use positive radians */
	    if (radians < 0.) radians = 2*M_PI + radians;
	    okIndex = (int)((radians*8.0)/M_PI);
	    okRadians = okRadiansTable[okIndex];
	    x01 = (int) (cos(okRadians)*length) + dlPolyline->points[
		    (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].x;
	    y01 = (int) (sin(okRadians)*length) + dlPolyline->points[
		    (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].y;
	  } else {
/* unconstrained */
	    x01 = event.xmotion.x;
	    y01 = event.xmotion.y;
	  }
	/* draw new line segments */
	  if (pointIndex > 0)
	      XDrawLine(display,window,xorGC,
		dlPolyline->points[pointIndex-1].x,
		dlPolyline->points[pointIndex-1].y, x01,y01);
	  if (pointIndex < dlPolyline->nPoints-1)
	      XDrawLine(display,window,xorGC,
		dlPolyline->points[pointIndex+1].x,
		dlPolyline->points[pointIndex+1].y, x01,y01);

	  break;

	default:
	  XtDispatchEvent(&event);
	  break;
     }
  }

}




/*
 * manipulate a polygon vertex
 */

void handlePolygonVertexManipulation(
  DlPolygon *dlPolygon,
  int pointIndex)
{
  XEvent event, newEvent;
  int newEventType;
  Window window;
  int i, minX, maxX, minY, maxY;
  int x01, y01;

  int deltaX, deltaY, okIndex;
  double radians, okRadians, length;

  window = XtWindow(currentDisplayInfo->drawingArea);
  minX = INT_MAX; maxX = INT_MIN; minY = INT_MAX; maxY = INT_MIN;
  
  x01 = dlPolygon->points[pointIndex].x;
  y01 = dlPolygon->points[pointIndex].y;

  XGrabPointer(display,window,FALSE,
	PointerMotionMask|ButtonMotionMask|ButtonReleaseMask,
	GrabModeAsync,GrabModeAsync,None,None,CurrentTime);
  XGrabServer(display);


/* now loop until button is released */
  while (TRUE) {

      XtAppNextEvent(appContext,&event);

      switch(event.type) {

	case ButtonRelease:

  /* modify point and leave here */
	    if (event.xbutton.state & ShiftMask) {
/* constrain to 45 degree increments */
	      deltaX = event.xbutton.x - dlPolygon->points[
		     (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].x;
	      deltaY = event.xbutton.y - dlPolygon->points[
		     (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].y;
	      length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
	      radians = atan2((double)(deltaY),(double)(deltaX));
	/* use positive radians */
	      if (radians < 0.) radians = 2*M_PI + radians;
	      okIndex = (int)((radians*8.0)/M_PI);
	      okRadians = okRadiansTable[okIndex];
	      x01 = (int) (cos(okRadians)*length) + dlPolygon->points[
		    (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].x;
	      y01 = (int) (sin(okRadians)*length) + dlPolygon->points[
		    (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].y;
	      dlPolygon->points[pointIndex].x = x01;
	      dlPolygon->points[pointIndex].y = y01;
	    } else {
/* unconstrained */
	      dlPolygon->points[pointIndex].x = event.xbutton.x;
	      dlPolygon->points[pointIndex].y = event.xbutton.y;
	    }

	/* also update 0 or nPoints-1 point to keep order of polygon same */
	    if (pointIndex == 0) {
	      dlPolygon->points[dlPolygon->nPoints-1] = dlPolygon->points[0];
	    } else if (pointIndex == dlPolygon->nPoints-1) {
	      dlPolygon->points[0] = dlPolygon->points[dlPolygon->nPoints-1];
	    }

	    XUngrabPointer(display,CurrentTime);
	    XUngrabServer(display);
	    for (i = 0; i < dlPolygon->nPoints; i++) {
	      minX = MIN(minX,dlPolygon->points[i].x);
	      maxX = MAX(maxX,dlPolygon->points[i].x);
	      minY = MIN(minY,dlPolygon->points[i].y);
	      maxY = MAX(maxY,dlPolygon->points[i].y);
	    }
	    dlPolygon->object.x = minX - globalResourceBundle.lineWidth/2;
	    dlPolygon->object.y = minY - globalResourceBundle.lineWidth/2;
	    dlPolygon->object.width = maxX - minX
					+ globalResourceBundle.lineWidth;
	    dlPolygon->object.height = maxY - minY
					+ globalResourceBundle.lineWidth;
	  /* update global resource bundle  - then do create out of it */
	    globalResourceBundle.x = dlPolygon->object.x;
	    globalResourceBundle.y = dlPolygon->object.y;
	    globalResourceBundle.width = dlPolygon->object.width;
	    globalResourceBundle.height = dlPolygon->object.height;

	    dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
/* since dmTraverseNonWidgets... clears the window, redraw highlights */
	    highlightSelectedElements();

	    return;
	  break;


	case MotionNotify:
	/* undraw old line segments */
	  if (pointIndex > 0)
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[pointIndex-1].x,
		dlPolygon->points[pointIndex-1].y, x01,y01);
	  else 
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[MAX(dlPolygon->nPoints-2,0)].x,
		dlPolygon->points[MAX(dlPolygon->nPoints-2,0)].y, x01,y01);

	  if (pointIndex < dlPolygon->nPoints-1)
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[pointIndex+1].x,
		dlPolygon->points[pointIndex+1].y, x01,y01);
	  else
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[0].x,dlPolygon->points[0].y, x01,y01);

	  if (event.xmotion.state & ShiftMask) {
/* constrain redraw to 45 degree increments */
	    deltaX =  event.xmotion.x - dlPolygon->points[
		     (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].x;
	    deltaY = event.xmotion.y - dlPolygon->points[
		     (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].y;
	    length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
	    radians = atan2((double)(deltaY),(double)(deltaX));
	/* use positive radians */
	    if (radians < 0.) radians = 2*M_PI + radians;
	    okIndex = (int)((radians*8.0)/M_PI);
	    okRadians = okRadiansTable[okIndex];
	    x01 = (int) (cos(okRadians)*length) + dlPolygon->points[
		     (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].x;
	    y01 = (int) (sin(okRadians)*length) + dlPolygon->points[
		     (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].y;
	  } else {
/* unconstrained */
	    x01 = event.xmotion.x;
	    y01 = event.xmotion.y;
	  }
	/* draw new line segments */
	  if (pointIndex > 0)
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[pointIndex-1].x,
		dlPolygon->points[pointIndex-1].y, x01,y01);
	  else 
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[MAX(dlPolygon->nPoints-2,0)].x,
		dlPolygon->points[MAX(dlPolygon->nPoints-2,0)].y, x01,y01);
	  if (pointIndex < dlPolygon->nPoints-1)
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[pointIndex+1].x,
		dlPolygon->points[pointIndex+1].y, x01,y01);
	  else
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[0].x,dlPolygon->points[0].y, x01,y01);

	  break;

	default:
	  XtDispatchEvent(&event);
	  break;
     }
  }

}


