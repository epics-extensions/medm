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

#include "medm.h"

#include <limits.h>


/* for modal dialogs - defined in utils.c and referenced here in callbacks.c */
extern Boolean modalGrab;
extern Widget mainMW;

extern char *stripChartWidgetName;

void executePopupMenuCallback(Widget  w, XtPointer cd, XtPointer cbs)
{
    int buttonNumber = (int) cd;
    XmAnyCallbackStruct *call_data = (XmAnyCallbackStruct *) cbs;
    Arg args[3];
    XtPointer data;
    DisplayInfo *displayInfo;

/* button's parent (menuPane) has the displayInfo pointer */
    XtSetArg(args[0],XmNuserData,&data);
    XtGetValues(XtParent(w),args,1);
    displayInfo = (DisplayInfo *) data;

    if (buttonNumber == EXECUTE_POPUP_MENU_PRINT_ID) {

	utilPrint(XtDisplay(displayInfo->drawingArea),
	  XtWindow(displayInfo->drawingArea),DISPLAY_XWD_FILE);

    } else if (buttonNumber == EXECUTE_POPUP_MENU_CLOSE_ID) {
	closeDisplay(w);
    }

}


void drawingAreaCallback(
  Widget  w,
  DisplayInfo *displayInfo,
  XmDrawingAreaCallbackStruct *call_data)
{
    int x, y;
    unsigned int uiw, uih;
    Dimension width, height, goodWidth, goodHeight, oldWidth, oldHeight;
    Boolean resized;
    XKeyEvent *key;
    Modifiers modifiers;
    KeySym keysym;
    Boolean objectDataOnly;
    Arg args[4];
    XtPointer userData;
    DlElement *elementPtr;
    float aspectRatio, newAspectRatio;

    Window root, child;
    int rootX,rootY,winX,winY;
    unsigned int mask;


    if (call_data->reason == XmCR_EXPOSE) {

	x = call_data->event->xexpose.x;
	y = call_data->event->xexpose.y;
	uiw = call_data->event->xexpose.width;
	uih = call_data->event->xexpose.height;

	if (displayInfo->drawingAreaPixmap != (Pixmap)NULL &&
	  displayInfo->pixmapGC != (GC)NULL && 
	  displayInfo->drawingArea != (Widget)NULL) {

	    XCopyArea(display,displayInfo->drawingAreaPixmap,XtWindow(w),
	      displayInfo->pixmapGC,x,y,uiw,uih,x,y);
	    if (globalDisplayListTraversalMode == DL_EXECUTE) {
		Display *display = XtDisplay(displayInfo->drawingArea);
		GC gc = displayInfo->gc;

		XPoint points[4];
		Region region;
		XRectangle clipRect;

		points[0].x = x;
		points[0].y = y;
		points[1].x = x + uiw;
		points[1].y = y;
		points[2].x = x + uiw;
		points[2].y = y + uih;
		points[3].x = x;
		points[3].y = y + uih;
		region = XPolygonRegion(points,4,EvenOddRule);
		if (region == NULL) {
		    medmPrintf("medmRepaintRegion : XPolygonRegion() return NULL\n");
		    return;
		}

	      /* clip the region */
		clipRect.x = x;
		clipRect.y = y;
		clipRect.width = uiw;
		clipRect.height = uih;

		XSetClipRectangles(display,gc,0,0,&clipRect,1,YXBanded);

		updateTaskRepaintRegion(displayInfo,&region);
	      /* release the clipping region */
		XSetClipOrigin(display,gc,0,0);
		XSetClipMask(display,gc,None);
		if (region) XDestroyRegion(region);
	    }
	}
	return;
    } else if (call_data->reason == XmCR_RESIZE) {

/* RESIZE */
	XtSetArg(args[0],XmNwidth,&width);
	XtSetArg(args[1],XmNheight,&height);
	XtSetArg(args[2],XmNuserData,&userData);
	XtGetValues(w,args,3);

	if (globalDisplayListTraversalMode == DL_EDIT) {
	    unhighlightSelectedElements();
	    resized = dmResizeSelectedElements(displayInfo,width,height);
	    if (displayInfo->hasBeenEditedButNotSaved == False)
	      medmMarkDisplayBeingEdited(displayInfo);

	} else {

/* in EXECUTE mode - resize all elements */

/* since calling for resize in resize handler - use this flag to ignore
 *   derived resize */
	    XQueryPointer(display,RootWindow(display,screenNum),&root,&child,
	      &rootX,&rootY,&winX,&winY,&mask);

	    if (userData != NULL || !(mask & ShiftMask) ) {

		XtSetArg(args[0],XmNuserData,(XtPointer)NULL);
		XtSetValues(w,args,1);
		goodWidth = width;
		goodHeight = height;

	    } else {

	      /* constrain resizes to original aspect ratio, call for resize, then return */

		elementPtr = FirstDlElement(displayInfo->dlElementList);
	      /* get to DL_Display type which has old x,y,width,height */
		while (elementPtr->type != DL_Display) {elementPtr = elementPtr->next;}
		oldWidth = elementPtr->structure.display->object.width;
		oldHeight = elementPtr->structure.display->object.height;
		aspectRatio = (float)oldWidth/(float)oldHeight;
		newAspectRatio = (float)width/(float)height;
		if (newAspectRatio > aspectRatio) {
		  /* w too big; derive w=f(h) */
		    goodWidth = (unsigned short) (aspectRatio*(float)height);
		    goodHeight = height;
		} else {
		  /* h too big; derive h=f(w) */
		    goodWidth = width;
		    goodHeight = (Dimension)((float)width/aspectRatio);
		}
	      /* change width/height of DA */
	      /* use DA's userData to signify a "forced" resize which can be ignored */
		XtSetArg(args[0],XmNwidth,goodWidth);
		XtSetArg(args[1],XmNheight,goodHeight);
		XtSetArg(args[2],XmNuserData,(XtPointer)1);
		XtSetValues(w,args,3);

		return;
	    }

	    resized = dmResizeDisplayList(displayInfo,goodWidth,goodHeight);
	}

/* (MDA) should always cleanup before traversing!! */
	if (resized) {
	    clearResourcePaletteEntries();	/* clear any selected entries */
	    dmCleanupDisplayInfo(displayInfo,FALSE);
#if 0
	    XtAppAddTimeOut(appContext,1000,traverseDisplayLater,displayInfo); 
#else
	    dmTraverseDisplayList(displayInfo);
#endif
	}
    }
    else {
	if (call_data->reason == XmCR_INPUT) {
	  /* INPUT */
	  /* left/right/up/down for movement of selected elements */
	    if (currentActionType == SELECT_ACTION &&
	      !IsEmpty(currentDisplayInfo->selectedDlElementList)) {
		
		key = &(call_data->event->xkey);
		
		if (key->type == KeyPress ) {
		    XtTranslateKeycode(display,key->keycode,(Modifiers)NULL,
		      &modifiers,&keysym);
		    if (keysym == osfXK_Left || keysym == osfXK_Right  ||
		      keysym == osfXK_Up   || keysym == osfXK_Down) {
			switch (keysym) {
			case osfXK_Left:
			    updateDraggedElements(1,0,0,0);
			    break;
			case osfXK_Right:
			    updateDraggedElements(0,0,1,0);
			    break;
			case osfXK_Up:
			    updateDraggedElements(0,1,0,0);
			    break;
			case osfXK_Down:
			    updateDraggedElements(0,0,0,1);
			    break;
			default:
			    break;
			}
			if (currentDisplayInfo->selectedDlElementList->count == 1) {
			    objectDataOnly = True;
			    updateGlobalResourceBundleAndResourcePalette(objectDataOnly);
			}
			if (currentDisplayInfo->hasBeenEditedButNotSaved == False) 
			  medmMarkDisplayBeingEdited(currentDisplayInfo);
		    }
		}
	    }
	}
    }
}
