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

#define DEBUG_EVENTS 0

#include "medm.h"

#include <limits.h>


/* For modal dialogs - defined in utils.c and referenced here in callbacks.c */
/* KE: neither of these seems to be used */
extern Boolean modalGrab;
extern Widget mainMW;

extern char *stripChartWidgetName;

void executePopupMenuCallback(Widget  w, XtPointer cd, XtPointer cbs)
{
    int buttonNumber = (int) cd;
    XmAnyCallbackStruct *call_data = (XmAnyCallbackStruct *) cbs;
    Arg args[1];
    XtPointer data;
    DisplayInfo *displayInfo;
    
  /* Button's parent (menuPane) has the displayInfo pointer */
    XtSetArg(args[0],XmNuserData,&data);
    XtGetValues(XtParent(w),args,1);
    displayInfo = (DisplayInfo *)data;

    switch(buttonNumber) {
    case EXECUTE_POPUP_MENU_PRINT_ID:
#ifdef WIN32
	dmSetAndPopupWarningDialog(displayInfo,
	  "Printing from MEDM is not available for WIN32\n"
	  "You can use Alt+PrintScreen to copy the window to the clipboard",
	  "OK", NULL, NULL);
#else	
	utilPrint(XtDisplay(displayInfo->drawingArea),
	  XtWindow(displayInfo->drawingArea),DISPLAY_XWD_FILE);
#endif	
	break;
    case EXECUTE_POPUP_MENU_CLOSE_ID:
	closeDisplay(w);
	break;
    case EXECUTE_POPUP_MENU_PVINFO_ID:
	popupPvInfo(displayInfo);
	break;
    case EXECUTE_POPUP_MENU_DISPLAY_LIST_ID:
	popupDisplayListDlg();
	break;
#if 0	
    case EXECUTE_POPUP_MENU_EXECUTE_ID:
	break;
#endif	
    }
}

void executeMenuCallback(Widget  w, XtPointer cd, XtPointer cbs)
{
    int buttonNumber = (int) cd;
    XmAnyCallbackStruct *call_data = (XmAnyCallbackStruct *)cbs;
    char *cmd;
    int nargs;
    Arg args[1];
    XtPointer data;
    DisplayInfo *displayInfo;

  /* Button's parent (menuPane) has the displayInfo pointer */
    nargs=0;
    XtSetArg(args[nargs],XmNuserData,&data); nargs++;
    XtGetValues(XtParent(w),args,nargs);
    displayInfo = (DisplayInfo *)data;

  /* Parse and execute command */
    cmd = execMenuCommandList[buttonNumber];
    parseAndExecCommand(displayInfo,cmd);
}

void drawingAreaCallback(Widget w, XtPointer clientData, XtPointer callData)
{
    DisplayInfo *displayInfo = (DisplayInfo *)clientData;
    XmDrawingAreaCallbackStruct *cbs = (XmDrawingAreaCallbackStruct *)callData;    
    int x, y;
    unsigned int uiw, uih;
    Dimension width, height, goodWidth, goodHeight, oldWidth, oldHeight;
    Boolean resized;
    XKeyEvent *key;
    Modifiers modifiers;
    KeySym keysym;
    Arg args[4];
    XtPointer userData;
    DlElement *elementPtr;
    float aspectRatio, newAspectRatio;

    Window root, child;
    int rootX,rootY,winX,winY;
    unsigned int mask;

#if DEBUG_EVENTS > 1
	fprintf(stderr,"\ndrawingAreaCallback(Entered): \n");
#endif
    if (cbs->reason == XmCR_EXPOSE) {
      /* EXPOSE */
#if DEBUG_EVENTS > 1
	fprintf(stderr,"drawingAreaCallback(XmCR_EXPOSE): \n");
#endif
	x = cbs->event->xexpose.x;
	y = cbs->event->xexpose.y;
	uiw = cbs->event->xexpose.width;
	uih = cbs->event->xexpose.height;

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
		    medmPostMsg(0,"drawingAreaCallback: XPolygonRegion is NULL\n");
		    return;
		}

	      /* Clip the region */
		clipRect.x = x;
		clipRect.y = y;
		clipRect.width = uiw;
		clipRect.height = uih;

		XSetClipRectangles(display,gc,0,0,&clipRect,1,YXBanded);
		updateTaskRepaintRegion(displayInfo,&region);
		
	      /* Release the clipping region */
		XSetClipOrigin(display,gc,0,0);
		XSetClipMask(display,gc,None);
		if (region) XDestroyRegion(region);
	    }
	}
	return;
    } else if (cbs->reason == XmCR_RESIZE) {
      /* RESIZE */
#if DEBUG_EVENTS > 1
	fprintf(stderr,"drawingAreaCallback(XmCR_RESIZE): \n");
#endif
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

	  /* in EXECUTE mode - resize all elements
	   * Since calling for resize in resize handler - use this flag to ignore
	   *   derived resize */
	    XQueryPointer(display,RootWindow(display,screenNum),&root,&child,
	      &rootX,&rootY,&winX,&winY,&mask);

	    if (userData != NULL || !(mask & ShiftMask) ) {

		XtSetArg(args[0],XmNuserData,(XtPointer)NULL);
		XtSetValues(w,args,1);
		goodWidth = width;
		goodHeight = height;

	    } else {

	      /* Constrain resizes to original aspect ratio, call for resize, then return */

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
	      /* Change width/height of DA */
	      /* Use DA's userData to signify a "forced" resize which can be ignored */
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
	    clearResourcePaletteEntries();	/* Clear any selected entries */
	    dmCleanupDisplayInfo(displayInfo,FALSE);
#if 0
	    XtAppAddTimeOut(appContext,1000,traverseDisplayLater,displayInfo); 
#else
	    dmTraverseDisplayList(displayInfo);
#endif
	}
    }
#if 0    
    else if (cbs->reason == XmCR_INPUT) {
      /* INPUT */
	XEvent *xEvent = (XEvent *)cbs->event;
	Boolean ctd = True;
	
#if DEBUG_EVENTS
	fprintf(stderr,"\ndrawingAreaCallback(XmCR_INPUT): \n");
	switch(xEvent->xany.type) {
	case ButtonPress:
	    printf("  ButtonPress\n");
	    break;
	case ButtonRelease:
	    printf("  ButtonRelease\n");
	    break;
	case KeyPress:
	    printf("  KeyPress\n");
	    break;
	case KeyRelease:
	    printf("  KeyRelease\n");
	    break;
	}
#endif
	switch(xEvent->xany.type) {
	case KeyPress:
	  /* Call the keypress handler */
	    handleEditKeyPress(w,(XtPointer)displayInfo,
	      (XEvent *)&cbs->event->xkey,&ctd);
	    break;
	}
    }
#endif    
}
