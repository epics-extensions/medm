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

#define DEBUG_DRAGDROP 0
#define DEBUG_DRAGDROPDELAY 0
#define DEBUG_SELECTION 0

#define USE_SOURCE_PIXMAP_MASK 0

/* Number of export types supported */
#define NTARGETS 3

#include "medm.h"
#include <Xm/MwmUtil.h>
#include <Xm/CutPaste.h>

#include "cvtFast.h"

/* Function prototypes */

static Boolean selectionConvertProc(Widget w, Atom *selection, Atom *target,
  Atom *typeRtn, XtPointer *valueRtn, unsigned long *lengthRtn,
  int *formatRtn);
static Boolean dragConvertProc(Widget w, Atom *selection, Atom *target,
  Atom *typeRtn, XtPointer *valueRtn, unsigned long *lengthRtn,
  int *formatRtn, unsigned long *max_lengthRtn, XtPointer client_data,
  XtRequestId *request_id);
static void dragDropFinish(Widget w, XtPointer clientData, XtPointer callData);
#if DEBUG_DRAGDROP
/* From TMprint.c */
String _XtPrintXlations(Widget w, XtTranslations xlations,
  Widget accelWidget, _XtBoolean includeRHS);
#endif

/* Global variables */

extern char *stripChartWidgetName;
/* Since passing client_data didn't seem to work... */
static char *channelNames;
/* Used to cleanup damage to Choice Button */
static Widget dragDropWidget = (Widget)0;

static Boolean selectionConvertProc(Widget w, Atom *selection, Atom *target,
  Atom *typeRtn, XtPointer *valueRtn, unsigned long *lengthRtn,
  int *formatRtn)
{
  /* Call the drag convert proc with dummy unused arguments */
    return dragConvertProc(w,selection,target,typeRtn,valueRtn,lengthRtn,
      formatRtn,0,NULL,0);
}

static Boolean dragConvertProc(Widget w, Atom *selection, Atom *target,
  Atom *typeRtn, XtPointer *valueRtn, unsigned long *lengthRtn,
  int *formatRtn, unsigned long *max_lengthRtn, XtPointer client_data,
  XtRequestId *request_id)
{
    XmString cString;
    char *passText;

    UNREFERENCED(w);
    UNREFERENCED(selection);
    UNREFERENCED(max_lengthRtn);
    UNREFERENCED(client_data);
    UNREFERENCED(request_id);

    if(!channelNames) return(False);

  /* Note that the requestor frees the data, not us */

    if (*target == compoundTextAtom) {
      /* COMPOUND_TEXT */
	cString = XmStringCreateLocalized(channelNames);
#if 0
	char *cText;
      /* KE: Seems unnecessary and cText is not freed */
	cText = XmCvtXmStringToCT(cString);
	XmStringFree(cString);
	if(!cText) return(False);
	passText = XtMalloc(strlen(cText)+1);
	if(!passText) return(False);
	memcpy(passText,cText,strlen(cText)+1);
#else
	passText = XmCvtXmStringToCT(cString);
	XmStringFree(cString);
	if(!passText) return(False);
#endif

	*typeRtn = *target;
	*valueRtn = (XtPointer)passText;
      /* KE: See comment under STRING case */
	*lengthRtn = strlen(passText);
	*formatRtn = 8;	/* Bits per element of the array */
	return(True);
    } else if(*target == XA_STRING || *target == textAtom) {
      /* STRING */
      /* TEXT */
	passText=XtNewString(channelNames);
	if(!passText) return(False);

	*typeRtn = *target;
	*valueRtn = (XtPointer)passText;
      /* KE: Using strlen(passText)+1 here results in a ^@ (NULL)
         appearing in the selection.  The documentation is unclear
         about what to use */
	*lengthRtn = strlen(passText);
	*formatRtn = 8; /* Bits per element of the array */
	return(True);
    } else {
	return(False);
    }
}


/*
 * Cleanup after drag/drop
 */
static void dragDropFinish(Widget w, XtPointer clientData, XtPointer callData)
{
    Widget sourceIcon;
    Pixmap pixmap;
    Arg args[2];

    UNREFERENCED(clientData);
    UNREFERENCED(callData);

#if DEBUG_DRAGDROPDELAY
    print("%8.3f dragDropFinish: Start\n",getTimerDouble());
#endif
  /* KE: The following is a kludge to repair Btn2 drag and drop leaving toggle
   *   buttons down in the Choice Menu.  The buttons only appear to be down.
   *   No callbacks are called and the values of XmNset are correct.
   *   (This seems to be a Motif bug and violates radio box behavior.)
   * A number of other kludges did not work:
   *   Unamanging, then managing radioBox and/or toggleButtons
   *   Sending VisibilityChange and Exposure events to radioBox
   *   XmToggleButtonSetState(children[i],getState,False); by itself
   * Note that the Choice Menu is a radioBox with toggleButton children */
    if(dragDropWidget && !strcmp(XtName(dragDropWidget),"radioBox")) {
	int i;
	WidgetList children;
	Cardinal numChildren;
	Boolean getState;

	XtVaGetValues(dragDropWidget,
	  XmNchildren,&children, XmNnumChildren,&numChildren,
	  NULL);
	for(i=0; i < (int)numChildren; i++) {
	    if(!strcmp(XtName(children[i]),"toggleButton")) {
		getState=XmToggleButtonGetState(children[i]);
#ifdef WIN32
	      /* Boolean is size 1, Logical operations convert to size 4 */
		XmToggleButtonSetState(children[i],(Boolean)(!getState),False);
#else
		XmToggleButtonSetState(children[i],!getState,False);
#endif
		XmToggleButtonSetState(children[i],getState,False);
	    }
	}
    }

#if DEBUG_DRAGDROP && 0
    if(dragDropWidget && !strcmp(XtName(dragDropWidget),"radioBox")) {
	int i;
	Boolean set,vis,radioBehavior;
	unsigned char indicatorType;
	WidgetList children;
	Cardinal numChildren;

	XtVaGetValues(dragDropWidget,
	  XmNchildren,&children, XmNnumChildren,&numChildren,
	  XmNradioBehavior,&radioBehavior,
	  NULL);
	printf("\ndragDropFinish: XmNradioBehavior=%d  XmNindicatorType: [XmONE_OF_MANY=%d]\n",
	  radioBehavior,XmONE_OF_MANY);
	for(i=0; i < (int)numChildren; i++) {
	    Boolean getState;

	    if(!strcmp(XtName(children[i]),"toggleButton")) {
		getState=XmToggleButtonGetState(children[i]);
		XtVaGetValues(children[i],
		  XmNset,&set,
		  XmNindicatorType,&indicatorType,
		  XmNvisibleWhenOff,&vis,
		  NULL);
		printf("Button %2d:  State=%d  XmNset=%d  XmNindicatorType=%d  "
		  "XmNvisibleWhenOff=%d\n",i,getState,set,indicatorType,vis);
	    }
	}
    }
#endif

  /* Perform cleanup at conclusion of drag and drop */
    XtSetArg(args[0],XmNsourcePixmapIcon,&sourceIcon);
    XtGetValues(w,args,1);

    XtSetArg(args[0],XmNpixmap,&pixmap);
    XtGetValues(sourceIcon,args,1);

    XFreePixmap(display,pixmap);
    XtDestroyWidget(sourceIcon);

#if USE_SOURCE_PIXMAP_MASK
  /* Implement cleanup for XmNmask here */
#endif

#if 0
  /* KE: Causes a lot of flashing.  Better to leave it trashed. */
#ifdef WIN32
    {
	DisplayInfo *displayInfo;

      /* Refresh the display for WIN32, since Exceed trashes it */
	if(dragDropWidget) {
	    displayInfo = dmGetDisplayInfoFromWidget(dragDropWidget);
	    if(displayInfo) refreshDisplay(displayInfo);
	}
    }
#endif
#endif
#if DEBUG_DRAGDROPDELAY
    print("%8.3f dragDropFinish: End\n",getTimerDouble());
#endif
}

static XtCallbackRec dragDropFinishCB[] = {
    {dragDropFinish,NULL},
    {NULL,NULL}
};


#define FONT_TABLE_INDEX 6  /* A nice sized font */
#define X_SHIFT 8
#define MARGIN  2
#if ((2*MAX_TRACES)+2) > MAX_PENS
#define MAX_COUNT 2*MAX_TRACES+2
#define MAX_COL 2
#else
#define MAX_COUNT MAX_PENS
#define MAX_COL 1
#endif

void StartDrag(Widget w, XEvent *event)
{
    Arg args[8];
    Cardinal n;
    Atom exportList[NTARGETS];
    Widget sourceIcon;
    UpdateTask *pT;
    int textWidth, maxWidth, maxHeight, fontHeight, ascent;
    unsigned long fg, bg;
    Widget searchWidget;
    Position x, y;
    XGCValues gcValues;
    unsigned long gcValueMask;
    DisplayInfo *displayInfo;
    DlElement *pE;
#if 0
    static char *channelNamesArray[MAX(MAX_PENS,MAX_TRACES)][2];
#endif
    Pixmap sourcePixmap = (Pixmap)NULL;
    static GC gc = NULL;
    Record *record[MAX_COUNT];
    int count;
    int column;
    int row;
#if USE_SOURCE_PIXMAP_MASK
  /* KE: Use this if a mask is implemented.  See comments below. */
    int doMask = 0;
    Pixmap maskPixmap = (Pixmap)NULL;
#endif

#if DEBUG_DRAGDROPDELAY
    print("%8.3f StartDrag: Start\n",getTimerDouble());
#endif

    if(!w) return;
    displayInfo = dmGetDisplayInfoFromWidget(w);
    if(!displayInfo) return;

  /* Find the element corresponding to the coordinates */
    dragDropWidget=(Widget)0;     /* Used in cleanup */
    x = event->xbutton.x;
    y = event->xbutton.y;
    pE = findSmallestTouchedExecuteElement(w, displayInfo,
      &x, &y, True);
    if(!pE) return;

  /* Get the update task */
    pT = getUpdateTaskFromElement(pE);
    if(!pT) return;

  /* Get the widget (may be different from w) or use the drawing area
     if none */
    if(pE->widget) searchWidget = pE->widget;
    else searchWidget = displayInfo->drawingArea;

#if DEBUG_DRAGDROP
    print("start drag : [%s] widget=0x%08x pT=0x%08x\n",
      elementType(pE->type),searchWidget,pT);
#if 1
    {
	static int first=1;
	XtTranslations xlations=NULL;
	String xString=NULL;

	XtVaGetValues(searchWidget,XtNtranslations,&xlations,NULL);
	print("  translations=0x%08x parsedTranslations=0x%08x\n",
	  xlations,parsedTranslations);
	print("\n");

	if(first) {
	  /* Note: widget argument is needed, even if the
             parsedTranslations are independent of it */
	    xString= _XtPrintXlations(searchWidget,parsedTranslations,NULL,True);
	    print("parsedTranslations:\n");
	    print("%s\n",xString);
	    XtFree(xString);
#if 0
       	    first=0;
#endif
	}

	xString= _XtPrintXlations(searchWidget,xlations,NULL,True);
	print("dragDropWidget translations:\n");
	print("%s\n",xString);
	XtFree(xString);
    }
#endif
#endif

  /* Call the getRecord procedure, if there is one */
    if(!pT->getRecord) return;
    pT->getRecord(pT->clientData, record, &count);

    column = count / 100;
    if (column == 0) column = 1;
    if (column > MAX_COL) {
	column = MAX_COL;
	medmPostMsg(1,"StartDrag: Maximum number of columns exceeded\n"
	  "  Programming Error: Please notify person in charge of MEDM\n");
	return;
    }
    row = 0;
    count = count % 100;
    if (count > MAX_COUNT) {
	medmPostMsg(1,"StartDrag: Maximum count exceeded\n"
	  "  Programming Error: Please notify person in charge of MEDM\n");
	return;
    }

  /* Make the source icon pixmap */
    bg = BlackPixel(display,screenNum);
    fg = WhitePixel(display,screenNum);
    ascent = fontTable[FONT_TABLE_INDEX]->ascent;
    fontHeight = ascent + fontTable[FONT_TABLE_INDEX]->descent;
  /* If the channelNames exists, free it */
    if(channelNames) {
	XtFree(channelNames);
	channelNames = NULL;
    }
    if (count == 0) {
	return;
    } else {
	int i, j;
	int x, y;

      /* Determine the dimensions of the pixmap */
	i = 0; j = 0;
	textWidth = 0;
	while (i < count) {
	    if (record[i] && record[i]->name) {
#ifdef MEDM_CDEV
	      /* Use fullname instead of name for CDEV */
		textWidth = MAX(textWidth,XTextWidth(
		  fontTable[FONT_TABLE_INDEX],record[i]->fullname,
		  strlen(record[i]->fullname)));
#else
		textWidth = MAX(textWidth,XTextWidth(
		  fontTable[FONT_TABLE_INDEX],record[i]->name,
		  strlen(record[i]->name)));
#endif
	    }
	    j++;
	    if (j >= column) {
		j = 0;
		row++;
	    }
	    i++;
	}

      /* KE: When the source pixmap is not extended below the state and
       *  operation icons, garbage pixels occur in this region.  Extending
       *  the row to 2 seems to eliminate this problem (at the expense of
       *  making the source pixmap larger than necessary).  Trying to use
       *  a mask gives Bad Match errors as the icon is created and/or
       *  dragged.  Note: If the mask is implemented it should be freed in
       *  dragDropFinish */
	if(row < 2) {
#if USE_SOURCE_PIXMAP_MASK
	    doMask = 1;
#endif
	    row = 2;
	}
	maxWidth = X_SHIFT + (textWidth + MARGIN) * column;
	maxHeight = row*fontHeight + 2*MARGIN;
	sourcePixmap = XCreatePixmap(display,
	  RootWindow(display,screenNum),maxWidth,maxHeight,
	  DefaultDepth(display,screenNum));
	if (gc == NULL) gc = XCreateGC(display,sourcePixmap,0,NULL);
	gcValueMask = GCForeground|GCBackground|GCFunction|GCFont;
	gcValues.foreground = bg;
	gcValues.background = bg;
	gcValues.function = GXcopy;
	gcValues.font = fontTable[FONT_TABLE_INDEX]->fid;
	XChangeGC(display,gc,gcValueMask,&gcValues);
      /* Eliminate events that we do not handle anyway */
	XSetGraphicsExposures(display,gc,False);
	XFillRectangle(display,sourcePixmap,gc,0,0,maxWidth,maxHeight);
	i = 0; j = 0;
	x = X_SHIFT;
	y = ascent + MARGIN;
	while (i < count) {
	    if (record[i] && record[i]->name) {
		char *name=NULL;
		XSetForeground(display,gc,
		  alarmColor(record[i]->severity));
#ifdef MEDM_CDEV
	      /* Use fullname insead of name for CDEV */
		name=record[i]->fullname;
#else
		name=record[i]->name;
#endif
		XDrawString(display,sourcePixmap,gc,x,y,name,strlen(name));
	      /* Build the channelNames for the selection and drop */
		if(!channelNames) {
		    int len=strlen(name)+1;
		    channelNames=XtMalloc(len);
		    if(channelNames) {
			strcpy(channelNames,name);
		    }
		} else {
		    int len=strlen(channelNames)+strlen(name)+2;
		    channelNames=XtRealloc(channelNames,len);
		    if(channelNames) {
			strcat(channelNames," ");
			strcat(channelNames,name);
		    }
		}
	    }
	    j++;
	    if (j < column) {
		x += textWidth + MARGIN;
	    } else {
		j = 0;
		x = X_SHIFT;
		y += fontHeight;
	    }
	    i++;
	}
#if USE_SOURCE_PIXMAP_MASK
	if(doMask) {
	    maskPixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum),maxWidth,maxHeight,
	      DefaultDepth(display,screenNum));
	    XSetFunction(display,gc,GXset);
	    XFillRectangle(display,maskPixmap,gc,0,0,maxWidth,maxHeight);
	    XSetFunction(display,gc,GXclear);
	    XFillRectangle(display,maskPixmap,gc,0,fontHeight+MARGIN+1,
	      maxWidth,maxHeight);
	}
#endif
#if DEBUG_DRAGDROPDELAY
    print("%8.3f StartDrag: Start put names in CLIPBOARD\n",getTimerDouble());
#endif
    }

  /* Put the channelNames in the CLIPBOARD.  Based on O'Reilly
     Vol. 6A.  This does not put it in the PRIMARY selection used by
     most X clients. */
    if(channelNames) {
	Window window=XtWindow(mainShell);
	long itemID=0;
	XmString clipLabel;
	int status;
	int privateID=0;

#if DEBUG_SELECTION
	print("MAX_COUNT=%d\n",MAX_COUNT);
	print("Copying|%s| to clipboard\n",channelNames);
	print("  Success=%d Fail=%d Locked=%d\n",
	  ClipboardSuccess,ClipboardFail,ClipboardLocked);
#endif

      /* Start the copy, try until unlocked */
	clipLabel=XmStringCreateLocalized("PV Name");
	do {
	  /* Don't use CurrentTime here.  Get the time from the event,
	     which must be a button press event, according to the man
	     page. */
#if DEBUG_DRAGDROPDELAY
	    print("%8.3f StartDrag: Start XmClipboardStartCopy\n",getTimerDouble());
#endif
	    status=XmClipboardStartCopy(display,window,clipLabel,
	      (event->type == ButtonPress)?((XButtonEvent *)event)->time:CurrentTime,
	      NULL,NULL,&itemID);
#if DEBUG_SELECTION
	    print("XmClipboardStartCopy: status=%d \n",status);
#endif
	} while(status == ClipboardLocked);
	XmStringFree(clipLabel);

      /* Copy the data, try until unlocked */
	do {
	  /* KE: Using strlen(channelNames)+1e results in ^@ appearing
             in the clipboard. */
#if DEBUG_DRAGDROPDELAY
	    print("%8.3f StartDrag: Start XmClipboardCopy\n",getTimerDouble());
#endif
	    status=XmClipboardCopy(display,window,itemID,"STRING",
	      channelNames,strlen(channelNames),privateID,NULL);
#if DEBUG_SELECTION
	    print("XmClipboardCopy: status=%d \n",status);
#endif
	} while(status == ClipboardLocked);

      /* End the copy, try until unlocked */
	do {
#if DEBUG_DRAGDROPDELAY
	    print("%8.3f StartDrag: Start XmClipboardEndCopy\n",getTimerDouble());
#endif
	    status=XmClipboardEndCopy(display,window,itemID);
#if DEBUG_SELECTION
	    print("XmClipboardEndCopy: status=%d \n",status);
#endif
	} while(status == ClipboardLocked);

    }

  /* Make the channelNames available in the PRIMARY selection so X
     clients like XTerm can use it.  Based on O'Reilly Vol. 4.  Should
     possibly use the more elaborate convert routine, but this seems
     to work.  The CLIPBOARD could possibly be done this way also, but
     note that XA_CLIPBOARD is not predefined. */
#if DEBUG_DRAGDROPDELAY
    print("%8.3f StartDrag: Start put names in PRIMARY\n",getTimerDouble());
#endif
    if(channelNames) {
	Boolean status;
      /* Don't use CurrentTime here.  Get the time from the event,
         which must be a button press event, according to the man
         page. */
	status=XtOwnSelection(mainShell,XA_PRIMARY,
	  (event->type == ButtonPress)?((XButtonEvent *)event)->time:CurrentTime,
	  selectionConvertProc,NULL,NULL);
#if DEBUG_SELECTION
	print("XtOwnSelection: |%s| status=%s\n"
	  "  event->type=%d [ButtonPress=%d]\n",
	  channelNames,status?"True":"False",
	  event->type,ButtonPress);
#endif
    }

  /* Create the drag icon and start the drag */
#if DEBUG_DRAGDROPDELAY
    print("%8.3f StartDrag: Start create drag icon\n",getTimerDouble());
#endif
    if (sourcePixmap != (Pixmap)NULL) {
      /* Use source widget as parent.  Can inherit visual attributes
         that way */
	n = 0;
	XtSetArg(args[n],XmNpixmap,sourcePixmap); n++;
	XtSetArg(args[n],XmNwidth,maxWidth); n++;
	XtSetArg(args[n],XmNheight,maxHeight); n++;
	XtSetArg(args[n],XmNdepth,DefaultDepth(display,screenNum)); n++;
#if USE_SOURCE_PIXMAP_MASK
	if(doMask) {
	    XtSetArg(args[n],XmNmask,maskPixmap); n++;
	}
#endif
	sourceIcon = XmCreateDragIcon(searchWidget, "sourceIcon",
	  args,n);

      /* Establish list of valid target types (Atoms must be interned, and this is done in medm.c) */

	exportList[0] = compoundTextAtom;
	exportList[1] = XA_STRING;    /* Doesn't have to be interned */
	exportList[2] = textAtom;

      /* Start the drag */
#if DEBUG_DRAGDROPDELAY
    print("%8.3f StartDrag: Start drag\n",getTimerDouble());
#endif
	dragDropWidget = searchWidget;     /* Save widget for cleanup */
	n = 0;
	XtSetArg(args[n],XmNexportTargets,exportList); n++;
	XtSetArg(args[n],XmNnumExportTargets,1); n++;
	XtSetArg(args[n],XmNdragOperations,XmDROP_COPY); n++;
	XtSetArg(args[n],XmNconvertProc,dragConvertProc); n++;
	XtSetArg(args[n],XmNsourcePixmapIcon,sourceIcon); n++;
	XtSetArg(args[n],XmNcursorForeground,fg); n++;
	XtSetArg(args[n],XmNcursorBackground,bg); n++;
	XtSetArg(args[n],XmNdragDropFinishCallback,dragDropFinishCB); n++;
	XmDragStart(searchWidget,event,args,n);
    }
#if DEBUG_DRAGDROPDELAY
    print("%8.3f StartDrag: End\n",getTimerDouble());
#endif
}
