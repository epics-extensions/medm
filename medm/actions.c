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

#define DEBUG_DRAG 0
#define USE_SOURCE_PIXMAP_MASK 0

#include "medm.h"
#include <Xm/MwmUtil.h>

#include "cvtFast.h"

extern char *stripChartWidgetName;

/* Since passing client_data didn't seem to work... */
static char *channelName;

/* Used to cleanup damage to Choice Button */
static Widget dragDropWidget = (Widget)0;

static Boolean DragConvertProc(Widget w, Atom *selection, Atom *target,
  Atom *typeRtn, XtPointer *valueRtn, unsigned long *lengthRtn,
  int *formatRtn, unsigned long *max_lengthRtn, XtPointer client_data,
  XtRequestId *request_id)
{
    XmString cString;
    char *cText, *passText;

    UNREFERENCED(w);
    UNREFERENCED(selection);
    UNREFERENCED(max_lengthRtn);
    UNREFERENCED(client_data);
    UNREFERENCED(request_id);

    if (channelName != NULL) {
	if (*target != COMPOUND_TEXT) return(False);
	cString = XmStringCreateLocalized(channelName);
	cText = XmCvtXmStringToCT(cString);
	passText = XtMalloc(strlen(cText)+1);
	memcpy(passText,cText,strlen(cText)+1);
      /* Probably need this too */
	XmStringFree(cString);

      /* Format the value for return */
	*typeRtn = COMPOUND_TEXT;
	*valueRtn = (XtPointer)passText;
	*lengthRtn = strlen(passText);
	*formatRtn = 8;	/* from example - related to #bits for data elements */
	return(True);
    } else {
      /* Monitordata not found */
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

#if DEBUG_DRAG
    if(dragDropWidget && !strcmp(XtName(dragDropWidget),"radioBox")) {
	int i;
	Boolean set,vis,radioBehavior;
	unsigned char indicatorType;
	
	XtVaGetValues(dragDropWidget,
	  XmNradioBehavior,&radioBehavior,NULL);
	printf("\ndragDropFinish: XmNradioBehavior=%d  XmNindicatorType: [XmONE_OF_MANY=%d]\n",
	  radioBehavior,XmONE_OF_MANY);
	for(i=0; i < (int)numChildren; i++) {
	    Boolean getState;

	    if(!strcmp(XtName(children[i]),"toggleButton")) {
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
    Atom exportList[1];
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
    static char *channelNames[MAX(MAX_PENS,MAX_TRACES)][2];
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

#if DEBUG_DRAG
    printf("start drag : 0x%08x\n",pT);
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
    if (count == 0) {
	channelName = NULL;
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
	      /* Use fullname insead of name for CDEV */
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
	XFillRectangle(display,sourcePixmap,gc,0,0,maxWidth,maxHeight);
	i = 0; j = 0;
	x = X_SHIFT;
	y = ascent + MARGIN;
	while (i < count) {
	    if (record[i] && record[i]->name) {
		XSetForeground(display,gc,
		  alarmColor(record[i]->severity));
#ifdef MEDM_CDEV
	      /* Use fullname insead of name for CDEV */
		XDrawString(display,sourcePixmap,gc,x,y,record[i]->fullname,
		  strlen(record[i]->fullname));
		channelName = record[i]->fullname;
#else
		XDrawString(display,sourcePixmap,gc,x,y,record[i]->name,
		  strlen(record[i]->name));
		channelName = record[i]->name;
#endif
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
	    XFillRectangle(display,maskPixmap,gc,0,fontHeight+MARGIN+1,maxWidth,maxHeight);
	}
#endif	    
    } 

  /* Create the drag icon and start the drag */
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

      /* Establish list of valid target types */
	exportList[0] = COMPOUND_TEXT;
	
      /* Start the drag */
	dragDropWidget = searchWidget;     /* Save widget for cleanup */
	n = 0;
	XtSetArg(args[n],XmNexportTargets,exportList); n++;
	XtSetArg(args[n],XmNnumExportTargets,1); n++;
	XtSetArg(args[n],XmNdragOperations,XmDROP_COPY); n++;
	XtSetArg(args[n],XmNconvertProc,DragConvertProc); n++;
	XtSetArg(args[n],XmNsourcePixmapIcon,sourceIcon); n++;
	XtSetArg(args[n],XmNcursorForeground,fg); n++;
	XtSetArg(args[n],XmNcursorBackground,bg); n++;
	XtSetArg(args[n],XmNdragDropFinishCallback,dragDropFinishCB); n++;
	XmDragStart(searchWidget,event,args,n);
    }
}
