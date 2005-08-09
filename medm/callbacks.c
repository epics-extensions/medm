/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
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
#define DEBUG_PIXMAP 0
#define DEBUG_EXPOSE 0
#define DEBUG_EXECUTE_MENU 0
#define DEBUG_RELATED_DISPLAY 0
#define DEBUG_POSITION 0

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
    int buttonNumber = (int)cd;
    Arg args[2];
    XtPointer data;
    DisplayInfo *displayInfo;
    int status;
    char *adlName;

  /* w is the button pushed.  Its parent is the row column, which has
     the displayInfo pointer as userData */
    XtSetArg(args[0],XmNuserData,&data);
    XtGetValues(XtParent(w),args,1);
    displayInfo = (DisplayInfo *)data;

    switch(buttonNumber) {
    case EXECUTE_POPUP_MENU_PRINT_ID:
#if 0
#ifdef WIN32
	if(!printToFile) {
	    dmSetAndPopupWarningDialog(displayInfo,
	      "Printing from MEDM is not available for WIN32\n"
	      "You can use Alt+PrintScreen to copy the window to the clipboard",
	      "OK", NULL, NULL);
	}
	break;
#endif
#endif
	if(printTitle == PRINT_TITLE_SHORT_NAME) {
	    adlName = shortName(displayInfo->dlFile->name);
	} else {
	    adlName = displayInfo->dlFile->name;
	}
	status = utilPrint(display, displayInfo->drawingArea,
	  xwdFile, adlName);
	if(!status) {
	    medmPrintf(1,"\nexecutePopupMenuCallback: "
	      "Print was not successful for\n  %s\n",
	      displayInfo->dlFile->name);
	}
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
    case EXECUTE_POPUP_MENU_MAIN_ID:
      /* KE: This appears to work and deiconify if iconic */
	XMapRaised(display, XtWindow(mainShell));
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
	  displayInfo->gc,0,0,width,height,0,0);
	XFlush(display);

	for(i=0; i < 1; i++) {
	    print("%d Background is window\n",i+1);
	    print("Hit CR to draw pixmap\n");
	    c=getc(stdin);

	    XCopyArea(display,displayInfo->drawingAreaPixmap,
	      XtWindow(displayInfo->drawingArea),
	      displayInfo->gc,0,0,width,height,0,0);
	    XFlush(display);

	    print("%d Background is pixmap\n",i+1);
	    print("Hit CR to draw window\n");
	    c=getc(stdin);

	    XCopyArea(display,pixmap,
	      XtWindow(displayInfo->drawingArea),
	      displayInfo->gc,0,0,width,height,0,0);
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
#if 0
	XSetForeground(display, displayInfo->gc,
	  displayInfo->colormap[20]);
	XFillRectangle(display, XtWindow(displayInfo->drawingArea),
	  displayInfo->gc, 0, 0, width, height);
#endif
#else
        popupDisplayListDlg();
#endif
	break;
    }
    case EXECUTE_POPUP_MENU_FLASH_HIDDEN_ID:
	markHiddenButtons(displayInfo);
	break;
    case EXECUTE_POPUP_MENU_REFRESH_ID:
	refreshDisplay(displayInfo);
	break;
    case EXECUTE_POPUP_MENU_RETRY_ID:
	retryConnections();
	break;
#if DEBUG_EXECUTE_MENU
    case EXECUTE_POPUP_MENU_EXECUTE_ID:
    {
	Arg args[10];
	int nargs;
	XtPointer userData=NULL;
	Widget subMenuId=NULL;
	WidgetList children=NULL;
	Cardinal numChildren=111;
	Cardinal postFromCount=111;
	unsigned char rowColumnType=111;
	Position x=111,y=111;

	print("executePopupMenuCallback: EXECUTE_POPUP_MENU_EXECUTE_ID=%d\n",
	  EXECUTE_POPUP_MENU_EXECUTE_ID);
	nargs = 0;
	XtSetArg(args[nargs],XmNx,&x); nargs++;
	XtSetArg(args[nargs],XmNy,&y); nargs++;
	XtSetArg(args[nargs],XmNuserData,&userData); nargs++;
	XtSetArg(args[nargs],XmNsubMenuId,&subMenuId); nargs++;
	XtSetArg(args[nargs],XmNnumChildren,&numChildren); nargs++;
	XtSetArg(args[nargs],XmNchildren,&children); nargs++;
	XtSetArg(args[nargs],XmNpostFromCount,&postFromCount); nargs++;
	XtSetArg(args[nargs],XmNrowColumnType,&rowColumnType); nargs++;
	XtGetValues(XtParent(w),args,nargs);

	print("  w=%x\n",w);
	print("  XtParent(w)=%x\n",XtParent(w));
	print("  displayInfo=%x\n",displayInfo);
	print("  x=%d\n",x);
	print("  y=%d\n",y);
	print("  userData=%x\n",userData);
	print("  subMenuId=%x\n",subMenuId);
	print("  numChildren=%d\n",numChildren);
	print("  postFromCount=%d\n",postFromCount);
	print("  rowColumnType=%d\n",(int)rowColumnType);
	print("    XmWORK_AREA=%d XmMENU_BAR=%d\n",
	  (int)XmWORK_AREA,(int)XmMENU_BAR);
	print("    XmMENU_POPUP=%d XmMENU_PULLDOWN=%d XmMENU_OPTION=%d\n",
	  (int)XmMENU_POPUP,(int)XmMENU_PULLDOWN,(int)XmMENU_OPTION);
	break;
    }
#endif
    }
}

void executeMenuCallback(Widget  w, XtPointer cd, XtPointer cbs)
{
    int buttonNumber = (int) cd;
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
    if(!displayInfo) return;

    if(cbs->reason == XmCR_EXPOSE) {
      /* EXPOSE */
#if DEBUG_EVENTS > 1 || DEBUG_EXPOSE
	print("drawingAreaCallback(XmCR_EXPOSE):\n");
#endif

      /* Move window to be consistent with object.x,y */
	if(displayInfo->positionDisplay) {
	    int status;

#if DEBUG_POSITION
	    {
		Position x,y;
		DlElement *pE = FirstDlElement(displayInfo->dlElementList);
		DlDisplay *dlDisplay = pE->structure.display;

		XtSetArg(args[0],XmNx,&x);
		XtSetArg(args[1],XmNy,&y);
		XtGetValues(displayInfo->shell,args,2);

		print("drawingAreaCallback:\n"
		  "  Shell(Before): object.x=%d object.y=%d xShell=%d yShell=%d\n",
		  dlDisplay->object.x,dlDisplay->object.y,x,y);
	    }
#endif
	    status = repositionDisplay(displayInfo);
	  /* Turn switch off if successful (Currently always) */
	    if(!status) displayInfo->positionDisplay = False;

#if DEBUG_POSITION
	    {
		Position x,y;
		DlElement *pE = FirstDlElement(displayInfo->dlElementList);
		DlDisplay *dlDisplay = pE->structure.display;

		XtSetArg(args[0],XmNx,&x);
		XtSetArg(args[1],XmNy,&y);
		XtGetValues(displayInfo->shell,args,2);

		print("drawingAreaCallback:\n"
		  "  Shell(After): object.x=%d object.y=%d xShell=%d yShell=%d\n",
		  dlDisplay->object.x,dlDisplay->object.y,x,y);
	    }
#endif
	}

    /* Handle exposure */
	x = cbs->event->xexpose.x;
	y = cbs->event->xexpose.y;
	uiw = cbs->event->xexpose.width;
	uih = cbs->event->xexpose.height;

	if(displayInfo->drawingAreaPixmap != (Pixmap)NULL &&
	  displayInfo->gc != (GC)NULL &&
	  displayInfo->drawingArea != (Widget)NULL) {

#if DEBUG_EVENTS > 1 || DEBUG_EXPOSE
	    print("  w=%x DA=%x x=%d y=%d width=%d height=%d\n",
	      w,displayInfo->drawingArea,x,y,uiw,uih);
#endif
#if DEBUG_EXPOSE > 1
	    if(globalDisplayListTraversalMode == DL_EXECUTE) {
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
		  displayInfo->gc,0,0,width,height,0,0);
		XFlush(display);

		for(i=0; i < 1; i++) {
		    print("%d Background is window\n",i+1);
		    print("Hit CR to draw pixmap\n");
		    c=getc(stdin);

		    XCopyArea(display,displayInfo->drawingAreaPixmap,
		      XtWindow(displayInfo->drawingArea),
		      displayInfo->gc,0,0,width,height,0,0);
		    XFlush(display);

		    print("%d Background is pixmap\n",i+1);
		    print("Hit CR to draw window\n");
		    c=getc(stdin);

		    XCopyArea(display,pixmap,
		      XtWindow(displayInfo->drawingArea),
		      displayInfo->gc,0,0,width,height,0,0);
		    XFlush(display);
		}
		XFreePixmap(display,pixmap);
		print("Done: Background is window\n");
	    }
#endif
	    if(globalDisplayListTraversalMode == DL_EXECUTE) {
	      /* EXECUTE mode */
		XRectangle clipRect;

	      /* Define the clip rectangle */
		clipRect.x = x;
		clipRect.y = y;
		clipRect.width = uiw;
		clipRect.height = uih;

	      /* Repaint the region */
		updateTaskRepaintRect(displayInfo, &clipRect, False);
	    } else {
	      /* EDIT mode */
		XCopyArea(display,displayInfo->drawingAreaPixmap,XtWindow(w),
		  displayInfo->gc,x,y,uiw,uih,x,y);
	    }
	}
#if DEBUG_RELATED_DISPLAY
	{
	    Position x, y;

	    XtSetArg(args[0],XmNx,&x);
	    XtSetArg(args[1],XmNy,&y);
	    XtGetValues(displayInfo->shell,args,2);
	    print("drawingAreaCallback: x=%d y=%d\n",x,y);
	    print("  XtIsRealized=%s XtIsManaged=%s\n",
	      XtIsRealized(displayInfo->shell)?"True":"False",
	      XtIsManaged(displayInfo->shell)?"True":"False");
	}
#endif
#if DEBUG_EXPOSE
	print("drawingAreaCallback(XmCR_EXPOSE): END\n\n");
#endif
	return;
    } else if(cbs->reason == XmCR_RESIZE) {
      /* RESIZE */
#if DEBUG_EVENTS > 1 || DEBUG_FONTS
	printf("drawingAreaCallback(XmCR_RESIZE):\n");
#endif
	nargs=0;
 	XtSetArg(args[nargs],XmNwidth,&width); nargs++;
	XtSetArg(args[nargs],XmNheight,&height); nargs++;
	XtSetArg(args[nargs],XmNuserData,&userData); nargs++;
	XtGetValues(w,args,nargs);

	if(globalDisplayListTraversalMode == DL_EDIT) {
	  /* In EDIT mode - resize selected elements */
	    unhighlightSelectedElements();
	    resized = dmResizeSelectedElements(displayInfo,width,height);
	    if(resized) medmMarkDisplayBeingEdited(displayInfo);
	} else {
	  /* In EXECUTE mode - resize all elements
	   * If user resized with Shift-Click
	   *     Redefine values to maintain aspect ratio
	   *     Use userData to make this happen only once */

	  /* Check for Shift-Click */
	    XQueryPointer(display,RootWindow(display,screenNum),&root,&child,
	      &rootX,&rootY,&winX,&winY,&mask);

	    if(userData != NULL || !(mask & ShiftMask) ) {
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
		if(newAspectRatio > aspectRatio) {
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
	if(resized) {
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

void wmCloseCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    ShellType shellType = (ShellType) cd;

    UNREFERENCED(cbs);

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
	if(w == mainShell) {
	    medmExit();
	} else if(w == objectS) {
	    XtPopdown(objectS);
	} else if(w == resourceS) {
	    XtPopdown(resourceS);
	} else if(w == colorS) {
	    XtPopdown(colorS);
	} else if(w == channelS) {
	    XtPopdown(channelS);
	} else if(w == helpS) {
	    XtPopdown(helpS);
	} else if(w == editHelpS) {
	    XtPopdown(editHelpS);
	} else if(w == pvInfoS) {
	    XtPopdown(pvInfoS);
	} else if(w == printSetupS) {
	    XtPopdown(printSetupS);
	} else if(w == errMsgS) {
	    XtPopdown(errMsgS);
	} else if(w == errMsgSendS) {
	    XtPopdown(errMsgSendS);
	} else if(w == caStudyS) {
	    XtPopdown(caStudyS);
	} else if(w == displayListS) {
	    XtPopdown(displayListS);
	} else {
	  /* KE: Cpould use this for all of them */
	    XtPopdown(w);
	}
	break;
    }
}

