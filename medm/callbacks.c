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
#define DEBUG_FONTS 0
#define DEBUG_PIXMAP 1
#define DEBUG_EXPOSE 1

#include "medm.h"

#include <limits.h>


/* For modal dialogs - defined in utils.c and referenced here in callbacks.c */
/* KE: neither of these seems to be used */
extern Boolean modalGrab;
extern Widget mainMW;

extern char *stripChartWidgetName;

#if DEBUG_PIXMAP
int getNumberOfCompositeElements(DlElement *dlElement, int indent, int n)
{
    int nE,i;
    DlElement *pE;
    DlComposite *dlComposite = dlElement->structure.composite;

    nE = 0;
    pE = FirstDlElement(dlComposite->dlElementList);
    while(pE) {
	nE++;
	for(i=0; i<indent; i++) print("  ");
	print("%2d %s\n",nE+n,elementType(pE->type));
	if(pE->type == DL_Composite) {
	    indent++;
	    nE+=getNumberOfCompositeElements(pE,indent,n+nE);
	    indent--;
	}
	pE = pE->next;
    }

    return nE;
}
#endif

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
    case EXECUTE_POPUP_MENU_PVLIMITS_ID:
	popupPvLimits(displayInfo);
	break;
    case EXECUTE_POPUP_MENU_DISPLAY_LIST_ID:
    {
#if DEBUG_PIXMAP
      /* Use this in debugging to show the drawing area pixmap */
	int c,i,n,nE,indent;
	Arg args[2];
	Dimension width, height;
	Pixmap pixmap;
	WidgetList children;
	Cardinal numChildren;
	DlElement *pE;

	n=0;
	XtSetArg(args[n],XmNwidth,&width); n++;
	XtSetArg(args[n],XmNheight,&height); n++;
	XtGetValues(displayInfo->drawingArea,args,n);
	
	pixmap=XCreatePixmap(display,RootWindow(display,screenNum),
	  width,height,
	  DefaultDepth(display,screenNum));
	XCopyArea(display,XtWindow(displayInfo->drawingArea),
	  pixmap,
	  displayInfo->pixmapGC,0,0,width,height,0,0);
	XFlush(display);
	
	for(i=0; i < 1; i++) {
	    print("%d Background is window\n",i+1);
	    print("Hit CR to draw pixmap\n");
	    c=getc(stdin);
	    
	    XCopyArea(display,displayInfo->drawingAreaPixmap,
	      XtWindow(displayInfo->drawingArea),
	      displayInfo->pixmapGC,0,0,width,height,0,0);
	    XFlush(display);
	    
	    print("%d Background is pixmap\n",i+1);
	    print("Hit CR to draw window\n");
	    c=getc(stdin);
	    
	    XCopyArea(display,pixmap,
	      XtWindow(displayInfo->drawingArea),
	      displayInfo->pixmapGC,0,0,width,height,0,0);
	    XFlush(display);
	}
	XFreePixmap(display,pixmap);
	print("Done: Background is window\n");

	print("  Element List:\n");
	indent=2;
	nE=0;
	pE = SecondDlElement(displayInfo->dlElementList);
	while(pE) {
	    nE++;
	    for(i=0; i<indent; i++) print("  ");
	    print("%2d %s\n",nE,elementType(pE->type));
	    if(pE->type == DL_Composite) {
		indent++;
		nE+=getNumberOfCompositeElements(pE,indent,nE);
		indent--;
	    }
	    pE = pE->next;
	}
	print("  Display has %d elements\n",nE);

	print("  Update Task List:\n");
	dumpUpdatetaskList(displayInfo);

	XtVaGetValues(displayInfo->drawingArea,XmNnumChildren,&numChildren,
	  XmNchildren,&children,NULL);
	print("  Drawing area has %d children\n",numChildren);
	for(i=0; i < (int)numChildren; i++) {
	    print("  %2d %x %s\n",i+1,children[i],XtName(children[i]));
	}
#else
        popupDisplayListDlg();
#endif    
	break;
    }
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
    Position oldx, oldy;
    Boolean resized;
    Arg args[4];
    int nargs;
    XtPointer userData;
    DlElement *pE;
    float aspectRatio, newAspectRatio;

    Window root, child;
    int rootX,rootY,winX,winY;
    unsigned int mask;

#if DEBUG_EVENTS > 1
	print("\ndrawingAreaCallback(Entered):\n");
#endif
    if (cbs->reason == XmCR_EXPOSE) {
      /* EXPOSE */
#if DEBUG_EVENTS > 1 || DEBUG_EXPOSE
	print("drawingAreaCallback(XmCR_EXPOSE):\n");
#endif
	x = cbs->event->xexpose.x;
	y = cbs->event->xexpose.y;
	uiw = cbs->event->xexpose.width;
	uih = cbs->event->xexpose.height;

	if (displayInfo->drawingAreaPixmap != (Pixmap)NULL &&
	  displayInfo->pixmapGC != (GC)NULL && 
	  displayInfo->drawingArea != (Widget)NULL) {
	    
#if DEBUG_EVENTS > 1 || DEBUG_EXPOSE
	    print("  w=%x DA=%x x=%d y=%d width=%d height=%d\n",
	      w,displayInfo->drawingArea,x,y,uiw,uih);
#endif
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
#if DEBUG_EVENTS > 1 || DEBUG_FONTS
	printf("drawingAreaCallback(XmCR_RESIZE):\n");
#endif
	nargs=0;
 	XtSetArg(args[nargs],XmNwidth,&width); nargs++;
	XtSetArg(args[nargs],XmNheight,&height); nargs++;
	XtSetArg(args[nargs],XmNuserData,&userData); nargs++;
	XtGetValues(w,args,nargs);

	if (globalDisplayListTraversalMode == DL_EDIT) {
	  /* In EDIT mode - resize selected elements */
	    unhighlightSelectedElements();
	    resized = dmResizeSelectedElements(displayInfo,width,height);
	    if (displayInfo->hasBeenEditedButNotSaved == False)
	      medmMarkDisplayBeingEdited(displayInfo);
	} else {
	  /* In EXECUTE mode - resize all elements
	   * If user resized with Shift-Click
	   *     Redefine values to maintain aspect ratio
	   *     Use userData to make this happen only once */

	  /* Check for Shift-Click */
	    XQueryPointer(display,RootWindow(display,screenNum),&root,&child,
	      &rootX,&rootY,&winX,&winY,&mask);

	    if (userData != NULL || !(mask & ShiftMask) ) {
	      /* Either not Shift-Click or second time through */

#if DEBUG_FONTS
		printf(" Upper branch: userData =%d (mask & ShiftMask)=%d\n",
		  userData,mask & ShiftMask);
		printf(" width=%d height=%d userData=%d\n",
		  (int)width,(int)height,(int)userData);
#endif
	      /* Reset userData */
		nargs=0;
		XtSetArg(args[nargs],XmNuserData,(XtPointer)NULL); nargs++;
		XtSetValues(w,args,nargs);
	      /* Use current width */
		goodWidth = width;
		goodHeight = height;
	    } else {
	      /* Shift-Click, first time through
	       *   Redefine height and/or width */
#if DEBUG_FONTS
		printf(" Lower branch: userData =%d (mask & ShiftMask)=%d\n",
		  userData,mask & ShiftMask);
		printf(" width=%d height=%d userData=%d\n",
		  (int)width,(int)height,(int)userData);
#endif
	      /* Look at the display, which is the first element */
		pE = FirstDlElement(displayInfo->dlElementList);
		oldWidth = pE->structure.display->object.width;
		oldHeight = pE->structure.display->object.height;
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
	      /* Change the width and/or height and set userData */
		nargs=0;
		XtSetArg(args[nargs],XmNwidth,goodWidth); nargs++;
		XtSetArg(args[nargs],XmNheight,goodHeight); nargs++;
		XtSetArg(args[nargs],XmNuserData,(XtPointer)1); nargs++;
		XtSetValues(w,args,nargs);
	      /* Return -- Display will be resized next time */		
		return;
	    }
	    
	  /* Set width and height in all DLObject's including display */
	    resized = dmResizeDisplayList(displayInfo,goodWidth,goodHeight);
#if DEBUG_EVENTS > 1 || DEBUG_FONTS
	    printf(" width=%d goodWidth=%d height=%d goodHeight=%d resized=%s\n",
	      (int)width,(int)goodWidth,(int)height,(int)goodHeight,resized?"True":"False");
#endif
	}

      /* Get the current values of x and y */
	nargs=0;
	XtSetArg(args[nargs],XmNx,&oldx); nargs++;
	XtSetArg(args[nargs],XmNy,&oldy); nargs++;
	XtGetValues(displayInfo->shell,args,nargs);

      /* Change DlObject values for x and y to be the same as the original */
	pE = FirstDlElement(displayInfo->dlElementList);
	pE->structure.display->object.x = oldx;
	pE->structure.display->object.y = oldy;

      /* Implement resized values */
	if (resized) {
	  /* Clear any selected entries */
	    unselectElementsInDisplay();
	    clearResourcePaletteEntries();
	  /* Clean up the displayInfo */
	    dmCleanupDisplayInfo(displayInfo,FALSE);
	  /* Execute the elements except the display */
	    dmTraverseDisplayList(displayInfo);
	}
    }
}

#ifdef __cplusplus 
void wmCloseCallback(Widget w, XtPointer cd, XtPointer)
#else
void wmCloseCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    ShellType shellType = (ShellType) cd;
  /*
   * handle WM Close functions like all the separate dialog close functions,
   *   dispatch based upon widget value that the callback is called with
   */
    switch (shellType) {
    case DISPLAY_SHELL:
	closeDisplay(w);
	break;
    case OTHER_SHELL:
      /* it's one of the permanent shells */
	if (w == mainShell) {
	    medmExit();
	} else if (w == objectS) {
	    XtPopdown(objectS);
	} else if (w == resourceS) {
	    XtPopdown(resourceS);
	} else if (w == colorS) {
	    XtPopdown(colorS);
	} else if (w == channelS) {
	    XtPopdown(channelS);
	} else if (w == helpS) {
	    XtPopdown(helpS);
	} else if (w == editHelpS) {
	    XtPopdown(editHelpS);
	} else if (w == pvInfoS) {
	    XtPopdown(pvInfoS);
	} else if (w == errMsgS) {
	    XtPopdown(errMsgS);
	} else if (w == errMsgSendS) {
	    XtPopdown(errMsgSendS);
	} else if (w == caStudyS) {
	    XtPopdown(caStudyS);
	} else if (w == displayListS) {
	    XtPopdown(displayListS);
	}
	break;
    }
}

