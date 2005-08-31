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

#define DEBUG_COMPOSITE 0
#define DEBUG_FONTS 0

/* KE: May need to set this to 0 for WIN32 to improve performance */
#define USE_MARQUEE 1

#if 0     /* Not using _NTSDK anymore */
#ifdef WIN32
  /* math.h for WIN32 with _NTSDK defined defines y1 as _y1, also y0
   *  as _y0 for Bessel functions.  This screws up XSegment
   *  Exceed 5 includes math.h differently than Exceed 6
   *  Include math.h here and undefine y1 since we don't use it
   *  Should work for both Exceed 5 and Exceed 6 */
#include <math.h>
#undef y1
#endif
#endif

#include "medm.h"
#include "Marquee.h"
#include <Xm/MwmUtil.h>

#define RD_APPLY_BTN  0
#define RD_CLOSE_BTN  1

#define MARKER_ON_TIME 5000     /* msec */
#define MARKER_OFF_TIME 1000     /* msec */
#define MARKER_COUNT 1

static Widget rdMatrix = NULL, rdForm = NULL;
static Widget table[MAX_RELATED_DISPLAYS][4];
static Pixmap stipple = (Pixmap)0;

static void relatedDisplayInheritValues(ResourceBundle *pRCB, DlElement *p);
static void relatedDisplaySetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void relatedDisplaySetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void relatedDisplayGetValues(ResourceBundle *pRCB, DlElement *p);
static void relatedDisplayButtonPressedCb(Widget, XtPointer, XtPointer);
static void renderRelatedDisplayPixmap(Display *display, Pixmap pixmap,
  Pixel fg, Pixel bg, Dimension width, Dimension height,
  XFontStruct *font, int icon, char *label);
static int countHiddenButtons(DlElement *dlElement);
static void createMarkerWidgets(DisplayInfo *displayInfo, DlElement *dlElement);
static void createMarkerWidget(DisplayInfo *displayInfo, DlElement *dlElement);
static void markerWidgetsDestroy(DisplayInfo *displayInfo);

static DlDispatchTable relatedDisplayDlDispatchTable = {
    createDlRelatedDisplay,
    NULL,
    executeDlRelatedDisplay,
    hideDlRelatedDisplay,
    writeDlRelatedDisplay,
    NULL,
    relatedDisplayGetValues,
    relatedDisplayInheritValues,
    relatedDisplaySetBackgroundColor,
    relatedDisplaySetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

/* Local function to render the related display icon into a pixmap */
static void renderRelatedDisplayPixmap(Display *display, Pixmap pixmap,
  Pixel fg, Pixel bg, Dimension width, Dimension height,
  XFontStruct *font, int icon, char *label)
{
  /* Icon is based on the 25 pixel (w & h) bitmap relatedDisplay25 */
    typedef struct { float x; float y;} XY;
    static float rectangleX = (float)(4./25.), rectangleY = (float)(4./25.),
      rectangleWidth = (float)(13./25.), rectangleHeight = (float)(14./25.);
    static XY segmentData[] = {
        {(float)(16./25.),(float)(9./25.)},
        {(float)(22./25.),(float)(9./25.)},
        {(float)(22./25.),(float)(22./25.)},
        {(float)(10./25.),(float)(22./25.)},
        {(float)(10./25.),(float)(18./25.)},
    };
    XSegment segments[4];
    GC gc = XCreateGC(display,pixmap,0,NULL);

  /* Eliminate events that we do not handle anyway */
    XSetGraphicsExposures(display,gc,False);

#if 0
    print("renderRelatedDisplayPixmap: width=%d height=%d label=|%s|\n",
      width,height,label?label:"NULL");
#endif

  /* Draw the background */
    XSetForeground(display,gc,bg);
    XFillRectangle(display,pixmap,gc,0,0,width,height);
    XSetForeground(display,gc,fg);

  /* Draw the icon */
    if(icon) {
	segments[0].x1 = (short)(segmentData[0].x*height);
	segments[0].y1 = (short)(segmentData[0].y*height);
	segments[0].x2 = (short)(segmentData[1].x*height);
	segments[0].y2 = (short)(segmentData[1].y*height);

	segments[1].x1 = (short)(segmentData[1].x*height);
	segments[1].y1 = (short)(segmentData[1].y*height);
	segments[1].x2 = (short)(segmentData[2].x*height);
	segments[1].y2 = (short)(segmentData[2].y*height);

	segments[2].x1 = (short)(segmentData[2].x*height);
	segments[2].y1 = (short)(segmentData[2].y*height);
	segments[2].x2 = (short)(segmentData[3].x*height);
	segments[2].y2 = (short)(segmentData[3].y*height);

	segments[3].x1 = (short)(segmentData[3].x*height);
	segments[3].y1 = (short)(segmentData[3].y*height);
	segments[3].x2 = (short)(segmentData[4].x*height);
	segments[3].y2 = (short)(segmentData[4].y*height);

	XDrawSegments(display,pixmap,gc,segments,4);

      /* Erase any out-of-bounds edges due to roundoff error by blanking out
       *  area of top rectangle */
	XSetForeground(display,gc,bg);
	XFillRectangle(display,pixmap,gc,
	  (int)(rectangleX*height),
	  (int)(rectangleY*height),
	  (unsigned int)(rectangleWidth*height),
	  (unsigned int)(rectangleHeight*height));

      /* Draw the top rectangle */
	XSetForeground(display,gc,fg);
	XDrawRectangle(display,pixmap,gc,
	  (int)(rectangleX*height),
	  (int)(rectangleY*height),
	  (unsigned int)(rectangleWidth*height),
	  (unsigned int)(rectangleHeight*height));
    }

  /* Draw the label */
    if(label && *label) {
	int base;

	XSetFont(display,gc,font->fid);
	base=(height+font->ascent-font->descent)/2;
	XDrawString(display,pixmap,gc,
	  icon?height:0,base,label,strlen(label));
    }

  /* Free the GC */
    XFreeGC(display,gc);
}

int relatedDisplayFontListIndex(DlRelatedDisplay *dlRelatedDisplay,
  int numButtons, int maxChars)
{
    int i;

    UNREFERENCED(maxChars);

#define SHADOWS_SIZE 4    /* each Toggle Button has 2 shadows...*/

/* more complicated calculation based on orientation, etc */
    for(i = MAX_FONTS-1; i >= 0; i--) {
	switch(dlRelatedDisplay->visual) {
	case RD_COL_OF_BTN:
	    if((int)(dlRelatedDisplay->object.height/MAX(1,numButtons)
	      - SHADOWS_SIZE) >=
	      (fontTable[i]->ascent + fontTable[i]->descent))
	      return(i);
	    break;
	case RD_ROW_OF_BTN:
	    if((int)(dlRelatedDisplay->object.height - SHADOWS_SIZE) >=
	      (fontTable[i]->ascent + fontTable[i]->descent))
	      return(i);
	    break;
	default:
	    break;
	}
    }
    return(0);
}

void executeDlRelatedDisplay(DisplayInfo *displayInfo, DlElement *dlElement)
{
    Widget localMenuBar, tearOff;
    Arg args[30];
    int nargs;
    int i, index, icon;
    char *label;
    XmString xmString;
    Pixmap pixmap;
    Dimension pixmapH, pixmapW;
    int iNumberOfDisplays = 0;
    DlRelatedDisplay *dlRelatedDisplay = dlElement->structure.relatedDisplay;

  /* These are widget ids, but they are recorded in the otherChild
   *  widget list as well, for destruction when new displays are
   *  selected at the top level */
    Widget relatedDisplayPulldownMenu, relatedDisplayMenuButton;
    Widget widget;

  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(dlElement->widget) {
	if(displayInfo->traversalMode == DL_EDIT) {
	  /* In EDIT mode destroy it */
	    XtDestroyWidget(dlElement->widget);
	    dlElement->widget = NULL;
	} else {
	  /* In EXECUTE manage it if necessary */
	    if(!XtIsManaged(dlElement->widget)) {
		XtManageChild(dlElement->widget);
	    }
	    return;
	}
    }

  /* Count number of displays with non-NULL labels */
    for(i = 0; i < MAX_RELATED_DISPLAYS; i++) {
	if(dlRelatedDisplay->display[i].label[0] != '\0') {
	    iNumberOfDisplays++;
	}
    }

  /* One display, not hidden */
    if(iNumberOfDisplays <= 1 && dlRelatedDisplay->visual != RD_HIDDEN_BTN) {
      /* Case 1 0f 4 */
      /* One item, any type but hidden */
      /* Get size for graphic part of pixmap */
	pixmapH = MIN(dlRelatedDisplay->object.width,
	  dlRelatedDisplay->object.height);
      /* Allow for shadows, etc. */
	pixmapH = (unsigned int)MAX(1,(int)pixmapH - 8);
      /* Create the pixmap */
	if(dlRelatedDisplay->label[0] == '\0') {
	    label=NULL;
	    index=0;
	    icon=1;
	    pixmapW=pixmapH;
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	} else if(dlRelatedDisplay->label[0] == '-') {
	    int usedWidth;

	    label=dlRelatedDisplay->label+1;
	    index=messageButtonFontListIndex(dlRelatedDisplay->object.height);
	    icon=0;
	    usedWidth=XTextWidth(fontTable[index],label,strlen(label));
	    pixmapW=MAX(usedWidth,1);
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	} else {
	    int usedWidth;

	    label=dlRelatedDisplay->label;
	    index=messageButtonFontListIndex(dlRelatedDisplay->object.height);
	    icon=1;
	    usedWidth=XTextWidth(fontTable[index],label,strlen(label));
	    pixmapW=pixmapH+usedWidth;
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	}
      /* Draw the pixmap */
	renderRelatedDisplayPixmap(display,pixmap,
	  displayInfo->colormap[dlRelatedDisplay->clr],
	  displayInfo->colormap[dlRelatedDisplay->bclr],
	  pixmapW, pixmapH, fontTable[index], icon, label);
      /* Create a push button */
	nargs = 0;
	dlElement->widget = createPushButton(
	  displayInfo->drawingArea,
	  &(dlRelatedDisplay->object),
	  displayInfo->colormap[dlRelatedDisplay->clr],
	  displayInfo->colormap[dlRelatedDisplay->bclr],
	  pixmap,
	  NULL,     /* There a pixmap, not a label on the button */
	  (XtPointer)displayInfo);
      /* Add the callbacks for bringing up the menu */
	if(globalDisplayListTraversalMode == DL_EXECUTE) {
	    int i;

	  /* Check the display array to find the first non-empty one */
	    for(i=0; i < MAX_RELATED_DISPLAYS; i++) {
		if(*(dlRelatedDisplay->display[i].name)) {
		    XtAddCallback(dlElement->widget,XmNactivateCallback,
		      relatedDisplayButtonPressedCb,
		      (XtPointer)&(dlRelatedDisplay->display[i]));
		    break;
		}
	    }
	}
      /* Add handlers */
	addCommonHandlers(dlElement->widget, displayInfo);
	XtManageChild(dlElement->widget);
    } else if(dlRelatedDisplay->visual == RD_MENU) {
      /* Case 2 of 4 */
      /* Single column menu, more than 1 item */
	nargs = 0;
	XtSetArg(args[nargs],XmNbackground,(Pixel)
	  displayInfo->colormap[dlRelatedDisplay->bclr]); nargs++;
	XtSetArg(args[nargs],XmNforeground,
	  (Pixel)displayInfo->colormap[dlRelatedDisplay->clr]); nargs++;
	XtSetArg(args[nargs],XmNhighlightThickness,0); nargs++;
	XtSetArg(args[nargs],XmNwidth,dlRelatedDisplay->object.width); nargs++;
	XtSetArg(args[nargs],XmNheight,dlRelatedDisplay->object.height); nargs++;
	XtSetArg(args[nargs],XmNmarginHeight,0); nargs++;
	XtSetArg(args[nargs],XmNmarginWidth,0); nargs++;
	XtSetArg(args[nargs],XmNresizeHeight,(Boolean)FALSE); nargs++;
	XtSetArg(args[nargs],XmNresizeWidth,(Boolean)FALSE); nargs++;
	XtSetArg(args[nargs],XmNspacing,0); nargs++;
	XtSetArg(args[nargs],XmNx,(Position)dlRelatedDisplay->object.x); nargs++;
	XtSetArg(args[nargs],XmNy,(Position)dlRelatedDisplay->object.y); nargs++;
      /* KE: Beware below if changing the number of args here */
	localMenuBar =
	  XmCreateMenuBar(displayInfo->drawingArea, "relatedDisplayMenuBar",
	    args, nargs);
	dlElement->widget = localMenuBar;
      /* Add handlers */
	addCommonHandlers(dlElement->widget, displayInfo);
	XtManageChild(dlElement->widget);

#if EXPLICITLY_OVERWRITE_CDE_COLORS
      /* Color menu bar explicitly to avoid CDE interference */
	colorMenuBar(localMenuBar,
	  (Pixel)displayInfo->colormap[dlRelatedDisplay->clr],
	  (Pixel)displayInfo->colormap[dlRelatedDisplay->bclr]);
#endif

      /* Create the pulldown menu */
	nargs = 0;
	XtSetArg(args[nargs],XmNbackground,(Pixel)
	  displayInfo->colormap[dlRelatedDisplay->bclr]); nargs++;
	XtSetArg(args[nargs],XmNforeground,
	  (Pixel)displayInfo->colormap[dlRelatedDisplay->clr]); nargs++;
#if 0
	XtSetArg(args[nargs], XmNtearOffModel, XmTEAR_OFF_DISABLED); nargs++;
#endif
	relatedDisplayPulldownMenu = XmCreatePulldownMenu(
	  localMenuBar, "relatedDisplayPulldownMenu", args, nargs);
      /* Make the tear off colors right */
	tearOff = XmGetTearOffControl(relatedDisplayPulldownMenu);
	if(tearOff) {
	    XtVaSetValues(tearOff,
	      XmNforeground,(Pixel)displayInfo->colormap[dlRelatedDisplay->clr],
	      XmNbackground,(Pixel)displayInfo->colormap[dlRelatedDisplay->bclr],
	      NULL);
	}

      /* Get size for graphic part of pixmap */
	pixmapH = MIN(dlRelatedDisplay->object.width,
	  dlRelatedDisplay->object.height);
      /* Allow for shadows, etc. */
	pixmapH = (unsigned int)MAX(1,(int)pixmapH - 8);
      /* Create the pixmap */
	if(dlRelatedDisplay->label[0] == '\0') {
	    label=NULL;
	    index=0;
	    icon=1;
	    pixmapW=pixmapH;
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	} else if(dlRelatedDisplay->label[0] == '-') {
	    int usedWidth;

	    label=dlRelatedDisplay->label+1;
	    index=messageButtonFontListIndex(dlRelatedDisplay->object.height);
	    icon=0;
	    usedWidth=XTextWidth(fontTable[index],label,strlen(label));
	    pixmapW=MAX(usedWidth,1);
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	} else {
	    int usedWidth;

	    label=dlRelatedDisplay->label;
	    index=messageButtonFontListIndex(dlRelatedDisplay->object.height);
	    icon=1;
	    usedWidth=XTextWidth(fontTable[index],label,strlen(label));
	    pixmapW=pixmapH+usedWidth;
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	}
      /* Draw the pixmap */
	renderRelatedDisplayPixmap(display,pixmap,
	  displayInfo->colormap[dlRelatedDisplay->clr],
	  displayInfo->colormap[dlRelatedDisplay->bclr],
	  pixmapW, pixmapH, fontTable[index], icon, label);
      /* Create a cascade button */
	nargs = 0;
	XtSetArg(args[nargs],XmNbackground,(Pixel)
	  displayInfo->colormap[dlRelatedDisplay->bclr]); nargs++;
	XtSetArg(args[nargs],XmNforeground,
	  (Pixel)displayInfo->colormap[dlRelatedDisplay->clr]); nargs++;
	XtSetArg(args[nargs],XmNhighlightThickness,0); nargs++;
	XtSetArg(args[nargs],XmNwidth,dlRelatedDisplay->object.width); nargs++;
	XtSetArg(args[nargs],XmNheight,dlRelatedDisplay->object.height); nargs++;
	XtSetArg(args[nargs],XmNmarginHeight,0); nargs++;
	XtSetArg(args[nargs],XmNmarginWidth,0); nargs++;
	XtSetArg(args[nargs],XmNrecomputeSize,(Boolean)False); nargs++;
	XtSetArg(args[nargs],XmNlabelPixmap,pixmap); nargs++;
	XtSetArg(args[nargs],XmNlabelType,XmPIXMAP); nargs++;
	XtSetArg(args[nargs],XmNsubMenuId,relatedDisplayPulldownMenu); nargs++;
	XtSetArg(args[nargs],XmNalignment,XmALIGNMENT_BEGINNING); nargs++;

	XtSetArg(args[nargs],XmNmarginLeft,0); nargs++;
	XtSetArg(args[nargs],XmNmarginRight,0); nargs++;
	XtSetArg(args[nargs],XmNmarginTop,0); nargs++;
	XtSetArg(args[nargs],XmNmarginBottom,0); nargs++;
	XtSetArg(args[nargs],XmNmarginWidth,0); nargs++;
	XtSetArg(args[nargs],XmNmarginHeight,0); nargs++;

	widget = XtCreateManagedWidget("relatedDisplayMenuLabel",
	  xmCascadeButtonGadgetClass,
	  dlElement->widget, args, nargs);

#if 0
      /* KE: Can't do this, it points to the stack
       *   and shouldn't be necessary */
	XtAddCallback(widget, XmNdestroyCallback,freePixmapCallback,
	  (XtPointer)relatedDisplayPixmap);
#endif

	for(i = 0; i < MAX_RELATED_DISPLAYS; i++) {
	    if(strlen(dlRelatedDisplay->display[i].name) > (size_t)1) {
		xmString = XmStringCreateLocalized(
		  dlRelatedDisplay->display[i].label);
		nargs = 0;
		XtSetArg(args[nargs],XmNbackground,(Pixel)
		  displayInfo->colormap[dlRelatedDisplay->bclr]); nargs++;
		XtSetArg(args[nargs],XmNforeground,
		  (Pixel)displayInfo->colormap[dlRelatedDisplay->clr]); nargs++;
		XtSetArg(args[nargs],XmNhighlightThickness,0); nargs++;
		XtSetArg(args[nargs], XmNlabelString,xmString); nargs++;
		XtSetArg(args[nargs], XmNuserData, displayInfo); nargs++;
		relatedDisplayMenuButton =
		  XtCreateManagedWidget("relatedDisplayButton",
		    xmPushButtonWidgetClass, relatedDisplayPulldownMenu,
		    args, nargs);
		XtAddCallback(relatedDisplayMenuButton,XmNactivateCallback,
		  relatedDisplayButtonPressedCb,&(dlRelatedDisplay->display[i]));
		XmStringFree(xmString);
	    }
	}
    } else if(dlRelatedDisplay->visual == RD_ROW_OF_BTN ||
      dlRelatedDisplay->visual == RD_COL_OF_BTN) {
      /* Case 3 of 4 */
      /* Rows or columns of buttons */
	Arg wargs[20];
	int i, maxChars, usedWidth, usedHeight;
	XmFontList fontList;
	Pixel fg, bg;
	Widget widget;

	maxChars = 0;
	for(i = 0; i < MAX_RELATED_DISPLAYS; i++) {
	    maxChars = MAX((size_t) maxChars,
	      strlen(dlRelatedDisplay->display[i].label));
	}
	fg = displayInfo->colormap[dlRelatedDisplay->clr];
	bg = displayInfo->colormap[dlRelatedDisplay->bclr];
	nargs = 0;
	XtSetArg(wargs[nargs],XmNx,(Position)dlRelatedDisplay->object.x);
	nargs++;
	XtSetArg(wargs[nargs],XmNy,(Position)dlRelatedDisplay->object.y);
	nargs++;
	XtSetArg(wargs[nargs],XmNwidth,
	  (Dimension)dlRelatedDisplay->object.width); nargs++;
	XtSetArg(wargs[nargs],XmNheight,
	  (Dimension)dlRelatedDisplay->object.height); nargs++;
	XtSetArg(wargs[nargs],XmNforeground,fg); nargs++;
	XtSetArg(wargs[nargs],XmNbackground,bg); nargs++;
	XtSetArg(wargs[nargs],XmNindicatorOn,(Boolean)FALSE); nargs++;
	XtSetArg(wargs[nargs],XmNmarginWidth,0); nargs++;
	XtSetArg(wargs[nargs],XmNmarginHeight,0); nargs++;
	XtSetArg(wargs[nargs],XmNresizeWidth,(Boolean)FALSE); nargs++;
	XtSetArg(wargs[nargs],XmNresizeHeight,(Boolean)FALSE); nargs++;
	XtSetArg(wargs[nargs],XmNspacing,0); nargs++;
	XtSetArg(wargs[nargs],XmNrecomputeSize,(Boolean)FALSE); nargs++;
	XtSetArg(wargs[nargs],XmNuserData,displayInfo); nargs++;
	switch(dlRelatedDisplay->visual) {
	case RD_COL_OF_BTN:
	    XtSetArg(wargs[nargs],XmNorientation,XmVERTICAL); nargs++;
	    usedWidth = dlRelatedDisplay->object.width;
	    usedHeight = (int)(dlRelatedDisplay->object.height/
	      MAX(1,iNumberOfDisplays));
	    break;
	case RD_ROW_OF_BTN:
	    XtSetArg(wargs[nargs],XmNorientation,XmHORIZONTAL); nargs++;
	    usedWidth = (int)(dlRelatedDisplay->object.width/
	      MAX(1,iNumberOfDisplays));
	    usedHeight = dlRelatedDisplay->object.height;
	    break;
	default:
	    break;
	}
	widget = XmCreateRowColumn(displayInfo->drawingArea,"radioBox",
	  wargs,nargs);
	dlElement->widget = widget;

      /* Make push-in type radio buttons of the correct size */
	fontList = fontListTable[relatedDisplayFontListIndex(
	  dlRelatedDisplay,iNumberOfDisplays,maxChars)];
	nargs = 0;
	XtSetArg(wargs[nargs],XmNindicatorOn,False); nargs++;
	XtSetArg(wargs[nargs],XmNshadowThickness,2); nargs++;
	XtSetArg(wargs[nargs],XmNhighlightThickness,0); nargs++;
	XtSetArg(wargs[nargs],XmNrecomputeSize,(Boolean)FALSE); nargs++;
	XtSetArg(wargs[nargs],XmNwidth,(Dimension)usedWidth); nargs++;
	XtSetArg(wargs[nargs],XmNheight,(Dimension)usedHeight); nargs++;
	XtSetArg(wargs[nargs],XmNfontList,fontList); nargs++;
	XtSetArg(wargs[nargs],XmNalignment,XmALIGNMENT_CENTER); nargs++;
	XtSetArg(wargs[nargs],XmNindicatorOn,False); nargs++;
	XtSetArg(wargs[nargs],XmNindicatorSize,0); nargs++;
	XtSetArg(wargs[nargs],XmNspacing,0); nargs++;
	XtSetArg(wargs[nargs],XmNvisibleWhenOff,False); nargs++;
	XtSetArg(wargs[nargs],XmNforeground,fg); nargs++;
	XtSetArg(wargs[nargs],XmNbackground,bg); nargs++;
	XtSetArg(wargs[nargs],XmNalignment,XmALIGNMENT_CENTER); nargs++;
	XtSetArg(wargs[nargs],XmNuserData,displayInfo); nargs++;
	for(i = 0; i < iNumberOfDisplays; i++) {
	    Widget toggleButton;

	    xmString = XmStringCreateLocalized(
	      dlRelatedDisplay->display[i].label);
	    XtSetArg(wargs[nargs],XmNlabelString,xmString); nargs++;
	  /* Use gadgets here so that changing foreground of
	   *   radioBox changes buttons */
	    toggleButton = XmCreatePushButtonGadget(widget,"toggleButton",
	      wargs,nargs);
	    if(globalDisplayListTraversalMode == DL_EXECUTE) {
		XtAddCallback(toggleButton,XmNactivateCallback,
		  relatedDisplayButtonPressedCb,
		  (XtPointer)&(dlRelatedDisplay->display[i]));
	    }  else {
		XtSetSensitive(toggleButton,False);
	    }
	  /* MDA: For some reason, need to do this after the fact for gadgets */
	    XtVaSetValues(toggleButton,XmNalignment,XmALIGNMENT_CENTER,NULL);
	    XtManageChild(toggleButton);
	    XmStringFree(xmString);
	}
      /* Add handlers */
	addCommonHandlers(dlElement->widget, displayInfo);
	XtManageChild(dlElement->widget);
    } else if(dlRelatedDisplay->visual == RD_HIDDEN_BTN) {
      /* Case 4 of 4 */
      /* Hidden button: No widget. No callbacks. Just draw a stippled
       *  rectangle.  Handle in executeModeButtonPress, not
       *  relatedDisplayButtonPressedCb */
	unsigned long gcValueMask;
	XGCValues gcValues;
	Drawable drawable=updateInProgress?
	  displayInfo->updatePixmap:displayInfo->drawingAreaPixmap;

	dlElement->updateType = STATIC_GRAPHIC;

	gcValueMask = GCForeground | GCBackground | GCFillStyle | GCStipple;
	gcValues.foreground = displayInfo->colormap[dlRelatedDisplay->clr];
	gcValues.background = displayInfo->colormap[dlRelatedDisplay->bclr];
	gcValues.fill_style = FillStippled;
	if(!stipple) {
	    static char stipple_bitmap[] = {0x03, 0x03, 0x0c, 0x0c};

	    stipple = XCreateBitmapFromData(display,
	      RootWindow(display, DefaultScreen(display)),
	      stipple_bitmap, 4, 4);
	}
	gcValues.stipple = stipple;
	XChangeGC(display, displayInfo->gc, gcValueMask, &gcValues);
	XFillRectangle(display, drawable, displayInfo->gc,
	  dlRelatedDisplay->object.x, dlRelatedDisplay->object.y,
	  dlRelatedDisplay->object.width, dlRelatedDisplay->object.height);

      /* Restore GC */
	XSetFillStyle(display, displayInfo->gc, FillSolid);
    }
}

void hideDlRelatedDisplay(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element with a widget */
    dlElement->updateType = STATIC_GRAPHIC;

    hideWidgetElement(displayInfo, dlElement);
}

static void createDlRelatedDisplayEntry(DlRelatedDisplayEntry *relatedDisplay,
  int displayNumber)
{
  /* Structure copy */
    *relatedDisplay = globalResourceBundle.rdData[displayNumber];
}

DlElement *createDlRelatedDisplay(DlElement *p)
{
    DlRelatedDisplay *dlRelatedDisplay;
    DlElement *dlElement;
    int displayNumber;

    dlRelatedDisplay = (DlRelatedDisplay *)malloc(sizeof(DlRelatedDisplay));
    if(!dlRelatedDisplay) return 0;
    if(p) {
	*dlRelatedDisplay = *p->structure.relatedDisplay;
    } else {
	objectAttributeInit(&(dlRelatedDisplay->object));
	for(displayNumber = 0; displayNumber < MAX_RELATED_DISPLAYS;
	     displayNumber++)
	  createDlRelatedDisplayEntry(
	    &(dlRelatedDisplay->display[displayNumber]),
	    displayNumber );

	dlRelatedDisplay->clr = globalResourceBundle.clr;
	dlRelatedDisplay->bclr = globalResourceBundle.bclr;
	dlRelatedDisplay->label[0] = '\0';
	dlRelatedDisplay->visual = RD_MENU;
    }

    if(!(dlElement = createDlElement(DL_RelatedDisplay,
      (XtPointer)dlRelatedDisplay,
      &relatedDisplayDlDispatchTable))) {
	free(dlRelatedDisplay);
    }

    return(dlElement);
}

void parseRelatedDisplayEntry(DisplayInfo *displayInfo,
  DlRelatedDisplayEntry *relatedDisplay)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;

    do {
	switch(tokenType=getToken(displayInfo,token)) {
	case T_WORD:
	    if(!strcmp(token,"label")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(relatedDisplay->label,token);
	    } else if(!strcmp(token,"name")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(relatedDisplay->name,token);
	    } else if(!strcmp(token,"args")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(relatedDisplay->args,token);
	    } else if(!strcmp(token,"policy")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,stringValueTable[REPLACE_DISPLAY]))
		  relatedDisplay->mode = REPLACE_DISPLAY;
	    }
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++;
	    break;
	case T_RIGHT_BRACE:
	    nestingLevel--;
	    break;
	default:
	    break;
	}
    } while((tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF));
}

DlElement *parseRelatedDisplay(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlRelatedDisplay *dlRelatedDisplay = 0;
    DlElement *dlElement = createDlRelatedDisplay(NULL);
    int displayNumber;
    int rc;

    if(!dlElement) return 0;
    dlRelatedDisplay = dlElement->structure.relatedDisplay;

    do {
	switch(tokenType=getToken(displayInfo,token)) {
	case T_WORD:
	    if(!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlRelatedDisplay->object));
	    } else if(!strncmp(token,"display",7)) {
	      /* Get the display number */
		displayNumber=MAX_RELATED_DISPLAYS-1;
		rc=sscanf(token,"display[%d]",&displayNumber);
		if(rc == 0 || rc == EOF || displayNumber < 0 ||
		  displayNumber > MAX_RELATED_DISPLAYS-1) {
		    displayNumber=MAX_RELATED_DISPLAYS-1;
		}
		parseRelatedDisplayEntry(displayInfo,
		  &(dlRelatedDisplay->display[displayNumber]) );
	    } else if(!strcmp(token,"clr")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlRelatedDisplay->clr = atoi(token) % DL_MAX_COLORS;
	    } else if(!strcmp(token,"bclr")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlRelatedDisplay->bclr = atoi(token) % DL_MAX_COLORS;
	    } else if(!strcmp(token,"label")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlRelatedDisplay->label,token);
	    } else if(!strcmp(token,"visual")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,stringValueTable[FIRST_RD_VISUAL+1])) {
		    dlRelatedDisplay->visual = RD_ROW_OF_BTN;
		} else
		  if(!strcmp(token,stringValueTable[FIRST_RD_VISUAL+2])) {
		      dlRelatedDisplay->visual = RD_COL_OF_BTN;
		  } else
		    if(!strcmp(token,stringValueTable[FIRST_RD_VISUAL+3])) {
			dlRelatedDisplay->visual = RD_HIDDEN_BTN;
		    }
	    }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++;
	    break;
	case T_RIGHT_BRACE:
	    nestingLevel--;
	    break;
	default:
	    break;
	}
    } while((tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF));

    return dlElement;

}

void writeDlRelatedDisplayEntry(FILE *stream, DlRelatedDisplayEntry *entry,
  int index, int level)
{
    char indent[16];

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
	fprintf(stream,"\n%sdisplay[%d] {",indent,index);
	if(entry->label[0] != '\0')
	  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,entry->label);
	if(entry->name[0] != '\0')
	  fprintf(stream,"\n%s\tname=\"%s\"",indent,entry->name);
	if(entry->args[0] != '\0')
	  fprintf(stream,"\n%s\targs=\"%s\"",indent,entry->args);
	if(entry->mode != ADD_NEW_DISPLAY)
	  fprintf(stream,"\n%s\tpolicy=\"%s\"",
	    indent,stringValueTable[entry->mode]);
	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%sdisplay[%d] {",indent,index);
	fprintf(stream,"\n%s\tlabel=\"%s\"",indent,entry->label);
	fprintf(stream,"\n%s\tname=\"%s\"",indent,entry->name);
	fprintf(stream,"\n%s\targs=\"%s\"",indent,entry->args);
	fprintf(stream,"\n%s}",indent);
    }
#endif
}

void writeDlRelatedDisplay(FILE *stream, DlElement *dlElement, int level)
{
    int i;
    char indent[16];
    DlRelatedDisplay *dlRelatedDisplay = dlElement->structure.relatedDisplay;

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
	fprintf(stream,"\n%s\"related display\" {",indent);
	writeDlObject(stream,&(dlRelatedDisplay->object),level+1);
	for(i = 0; i < MAX_RELATED_DISPLAYS; i++) {
	    if((dlRelatedDisplay->display[i].label[0] != '\0') ||
	      (dlRelatedDisplay->display[i].name[0] != '\0') ||
	      (dlRelatedDisplay->display[i].args[0] != '\0')) {
		writeDlRelatedDisplayEntry(stream,
		  &(dlRelatedDisplay->display[i]),i,level+1);
	    }
	}
	fprintf(stream,"\n%s\tclr=%d",indent,dlRelatedDisplay->clr);
	fprintf(stream,"\n%s\tbclr=%d",indent,dlRelatedDisplay->bclr);
	if(dlRelatedDisplay->label[0] != '\0')
	  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,dlRelatedDisplay->label);
	if(dlRelatedDisplay->visual != RD_MENU)
	  fprintf(stream,"\n%s\tvisual=\"%s\"",
	    indent,stringValueTable[dlRelatedDisplay->visual]);
	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%s\"related display\" {",indent);
	writeDlObject(stream,&(dlRelatedDisplay->object),level+1);
	for(i = 0; i < MAX_RELATED_DISPLAYS; i++) {
	    writeDlRelatedDisplayEntry(stream,
	      &(dlRelatedDisplay->display[i]),i,level+1);
	}
	fprintf(stream,"\n%s\tclr=%d",indent,dlRelatedDisplay->clr);
	fprintf(stream,"\n%s\tbclr=%d",indent,dlRelatedDisplay->bclr);
	fprintf(stream,"\n%s}",indent);
    }
#endif
}

static void relatedDisplayButtonPressedCb(Widget w, XtPointer clientData,
  XtPointer callbackData)
{
    DlRelatedDisplayEntry *pEntry = (DlRelatedDisplayEntry *)clientData;
    DisplayInfo *displayInfo = 0;
    XEvent *event = ((XmPushButtonCallbackStruct *)callbackData)->event;
    Boolean replace = False;

  /* See if it was a ctrl-click indicating replace */
  /* KE: Replace = with == in next line */
    if(event->type == ButtonPress  &&
      ((XButtonEvent *)event)->state & ControlMask) {
	replace = True;
    }

#if DEBUG_FONTS
    print("\nrelatedDisplayButtonPressedCb: displayInfo=%x replace=%s\n",
      displayInfo,replace?"True":"False");
#endif

  /* Create the new display */
    XtVaGetValues(w, XmNuserData, &displayInfo, NULL);
    relatedDisplayCreateNewDisplay(displayInfo, pEntry, replace);
}

void relatedDisplayCreateNewDisplay(DisplayInfo *displayInfo,
  DlRelatedDisplayEntry *pEntry, Boolean replaceDisplay)
{
    DisplayInfo *existingDisplayInfo;
    FILE *filePtr;
    char *argsString, token[MAX_TOKEN_LENGTH];
    char filename[MAX_TOKEN_LENGTH];
    char processedArgs[2*MAX_TOKEN_LENGTH];

    strncpy(filename, pEntry->name, MAX_TOKEN_LENGTH);
    filename[MAX_TOKEN_LENGTH-1] = '\0';
    argsString = pEntry->args;

#if DEBUG_FONTS
    print("relatedDisplayCreateNewDisplay: displayInfo=%x replaceDisplay=%s\n"
      "  filename=%s\n",
      displayInfo,replaceDisplay?"True":"False",filename);
#endif

  /*
   * if we want to be able to have RD's inherit their parent's
   *   macro-substitutions, then we must perform any macro substitution on
   *   this argument string in this displayInfo's context before passing
   *   it to the created child display
   */
    if(globalDisplayListTraversalMode == DL_EXECUTE) {
	performMacroSubstitutions(displayInfo, argsString, processedArgs,
          2*MAX_TOKEN_LENGTH);
      /* Check for an existing display */
	existingDisplayInfo = NULL;
	if(popupExistingDisplay) {
	    existingDisplayInfo = findDisplay(filename, processedArgs,
	      displayInfo->dlFile->name);
	    if(existingDisplayInfo) {
		DisplayInfo *cdi;

	      /* Remove the old one if appropriate */
		if(replaceDisplay || (pEntry->mode == REPLACE_DISPLAY &&
		  displayInfo != existingDisplayInfo)) {
		    closeDisplay(displayInfo->shell);
		}

	      /* Make the existing one be the current one */
		cdi = currentDisplayInfo = existingDisplayInfo;
#if 0
	      /* KE: Doesn't work on WIN32 */
		XtPopdown(currentDisplayInfo->shell);
		XtPopup(currentDisplayInfo->shell,XtGrabNone);
#else
		if(cdi && cdi->shell && XtIsRealized(cdi->shell)) {
		    XMapRaised(display, XtWindow(cdi->shell));
		}
#endif
	      /* Refresh the display list dialog box */
		refreshDisplayListDlg();
		return;
	    }
	}

      /* There is no existing display to use.  Try to find a file,
         passing the parent's path. */
	filePtr = dmOpenUsableFile(filename, displayInfo->dlFile->name);
	if(filePtr == NULL) {
	    sprintf(token,
	      "Cannot open related display:\n  %s\nCheck %s\n",
	      filename, "EPICS_DISPLAY_PATH");
	    dmSetAndPopupWarningDialog(displayInfo,token,"OK",NULL,NULL);
	    medmPostMsg(1,token);
	} else {
	    if(replaceDisplay || pEntry->mode == REPLACE_DISPLAY) {
	      /* Don't look for an existing one.  Just reparse this one. */
		dmDisplayListParse(displayInfo,filePtr,processedArgs,
		  filename,NULL,(Boolean)True);
	    } else {
		dmDisplayListParse(NULL,filePtr,processedArgs,
		  filename,NULL,(Boolean)True);
	    }
	    fclose(filePtr);

	  /* Refresh the display list dialog box */
	    refreshDisplayListDlg();
	}
    }
}

static void relatedDisplayActivate(Widget w, XtPointer cd, XtPointer cbs)
{
    int buttonType = (int)cd;
    int i;

    UNREFERENCED(w);
    UNREFERENCED(cbs);

    switch(buttonType) {
    case RD_APPLY_BTN:
      /* Commit changes in matrix to global matrix array data */
	for(i = 0; i < MAX_RELATED_DISPLAYS; i++) {
	    char *tmp = NULL;
	    tmp = XmTextFieldGetString(table[i][0]);
	    if(tmp) {
		strcpy(globalResourceBundle.rdData[i].label, tmp);
		XtFree(tmp);
	    }
	    tmp = XmTextFieldGetString(table[i][1]);
	    if(tmp) {
		strcpy(globalResourceBundle.rdData[i].name, tmp);
		XtFree(tmp);
	    }
	    tmp = XmTextFieldGetString(table[i][2]);
	    if(tmp) {
		strcpy(globalResourceBundle.rdData[i].args, tmp);
		XtFree(tmp);
	    }
	    if(XmToggleButtonGetState(table[i][3])) {
		globalResourceBundle.rdData[i].mode = REPLACE_DISPLAY;
	    } else {
		globalResourceBundle.rdData[i].mode = ADD_NEW_DISPLAY;
	    }
	}
	if(currentDisplayInfo) {
	    DlElement *dlElement =
	      FirstDlElement(currentDisplayInfo->selectedDlElementList);
	    while(dlElement) {
		if(dlElement->structure.element->type == DL_RelatedDisplay) {
		    updateElementFromGlobalResourceBundle(
		      dlElement->structure.element);
		}
		dlElement = dlElement->next;
	    }
	}
	medmMarkDisplayBeingEdited(currentDisplayInfo);
	XtPopdown(relatedDisplayS);
	break;
    case RD_CLOSE_BTN:
	if(XtClass(w) == xmPushButtonWidgetClass) {
	    XtPopdown(relatedDisplayS);
	}
	break;
    }
}

/* Create related display data dialog */
Widget createRelatedDisplayDataDialog(Widget parent)
{
    Widget shell, applyButton, closeButton;
    Dimension cWidth, cHeight, aWidth, aHeight;
    Arg args[12];
    int i, nargs;

/*
 * now create the interface
 *
 *         label | name | args | mode
 *         --------------------------
 *      1 |  A      B      C      D
 *      2 |
 *      3 |
 *         ...
 *     OK     CANCEL
 */

  /* Create form dialog (rdForm) */
    nargs = 0;
    XtSetArg(args[nargs],XmNautoUnmanage,False); nargs++;
    XtSetArg(args[nargs],XmNmarginHeight,8); nargs++;
    XtSetArg(args[nargs],XmNmarginWidth,8); nargs++;
    XtSetArg(args[nargs],XmNdialogStyle,XmDIALOG_PRIMARY_APPLICATION_MODAL);
    nargs++;
    rdForm = XmCreateFormDialog(parent,"relatedDisplayDataF",args,nargs);

  /* Set values for the shell parent of the form */
    shell = XtParent(rdForm);
    XtVaSetValues(shell,
      XmNtitle,"Related Display Data",
#if OMIT_RESIZE_HANDLES
      XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH,
    /* KE: The following is necessary for Exceed, which turns off the
       resize function with the handles.  It should not be necessary */
      XmNmwmFunctions, MWM_FUNC_ALL,
#endif
      NULL);
    XmAddWMProtocolCallback(shell,WM_DELETE_WINDOW,
      relatedDisplayActivate,(XtPointer)RD_CLOSE_BTN);
#if DEBUG_COMPOSITE
    {
	Widget w=rdForm;
	Window win=XtWindow(w);

	int i=0;

	while(1) {
	    print("%4d w=%x win=%x",i++,w,win);
	    if(w == mainShell) {
		print(" (mainShell)\n");
		break;
	    } else if(w == shell) {
		print(" (shell)\n");
	    } else if(w == rdForm) {
		print(" (rdForm)\n");
	    } else if(w == parent) {
		print(" (parent)\n");
	    } else {
		print("\n");
	    }
	    w=XtParent(w);
	    win=XtWindow(w);
	}
    }
#endif

  /* rdMatrix */
    nargs = 0;
    rdMatrix = XtVaCreateManagedWidget("rdMatrix",
      xmRowColumnWidgetClass,rdForm,
      XmNpacking, XmPACK_COLUMN,
      XmNorientation,XmHORIZONTAL,
      XmNnumColumns, MAX_RELATED_DISPLAYS + 1,
      NULL);
  /* create column label */
    XtVaCreateManagedWidget("Display Label",
      xmLabelWidgetClass, rdMatrix,
      XmNalignment, XmALIGNMENT_CENTER,
      NULL);
    XtVaCreateManagedWidget("Display File",
      xmLabelWidgetClass, rdMatrix,
      XmNalignment, XmALIGNMENT_CENTER,
      NULL);
    XtVaCreateManagedWidget("Arguments",
      xmLabelWidgetClass, rdMatrix,
      XmNalignment, XmALIGNMENT_CENTER,
      NULL);
    XtVaCreateManagedWidget("Policy",
      xmLabelWidgetClass, rdMatrix,
      XmNalignment, XmALIGNMENT_CENTER,
      NULL);
    for(i=0; i<MAX_RELATED_DISPLAYS; i++) {
	table[i][0] = XtVaCreateManagedWidget("label",
	  xmTextFieldWidgetClass, rdMatrix, NULL);
	table[i][1] = XtVaCreateManagedWidget("display",
	  xmTextFieldWidgetClass, rdMatrix, NULL);
	table[i][2] = XtVaCreateManagedWidget("arguments",
	  xmTextFieldWidgetClass, rdMatrix, NULL);
	table[i][3] = XtVaCreateManagedWidget("Remove Parent Display",
	  xmToggleButtonWidgetClass, rdMatrix,
	  XmNshadowThickness, 0,
	  NULL);
    }

  /* Close Button */
    closeButton = XtVaCreateWidget("Cancel",
      xmPushButtonWidgetClass, rdForm,
      NULL);
    XtAddCallback(closeButton,XmNactivateCallback,
      relatedDisplayActivate,(XtPointer)RD_CLOSE_BTN);
    XtManageChild(closeButton);

  /* Apply Button */
    applyButton = XtVaCreateWidget("Apply",
      xmPushButtonWidgetClass, rdForm,
      NULL);
    XtAddCallback(applyButton,XmNactivateCallback,
      relatedDisplayActivate,(XtPointer)RD_APPLY_BTN);
    XtManageChild(applyButton);

  /* Make APPLY and CLOSE buttons same size */
    XtVaGetValues(closeButton,XmNwidth,&cWidth,XmNheight,&cHeight,NULL);
    XtVaGetValues(applyButton,XmNwidth,&aWidth,XmNheight,&aHeight,NULL);
    XtVaSetValues(closeButton,XmNwidth,MAX(cWidth,aWidth),
      XmNheight,MAX(cHeight,aHeight),NULL);

  /* Make the APPLY button the default for the form */
    XtVaSetValues(rdForm,XmNdefaultButton,applyButton,NULL);

  /* Do form layout */

  /* rdMatrix */
    nargs = 0;
    XtSetArg(args[nargs],XmNtopAttachment,XmATTACH_FORM); nargs++;
    XtSetArg(args[nargs],XmNleftAttachment,XmATTACH_FORM); nargs++;
    XtSetArg(args[nargs],XmNrightAttachment,XmATTACH_FORM); nargs++;
    XtSetValues(rdMatrix,args,nargs);

  /* Apply */
    nargs = 0;
    XtSetArg(args[nargs],XmNtopAttachment,XmATTACH_WIDGET); nargs++;
    XtSetArg(args[nargs],XmNtopWidget,rdMatrix); nargs++;
    XtSetArg(args[nargs],XmNtopOffset,12); nargs++;
    XtSetArg(args[nargs],XmNleftAttachment,XmATTACH_POSITION); nargs++;
    XtSetArg(args[nargs],XmNleftPosition,30); nargs++;
    XtSetArg(args[nargs],XmNbottomAttachment,XmATTACH_FORM); nargs++;
    XtSetArg(args[nargs],XmNbottomOffset,12); nargs++;
    XtSetValues(applyButton,args,nargs);

  /* Close */
    nargs = 0;
    XtSetArg(args[nargs],XmNtopAttachment,XmATTACH_WIDGET); nargs++;
    XtSetArg(args[nargs],XmNtopWidget,rdMatrix); nargs++;
    XtSetArg(args[nargs],XmNtopOffset,12); nargs++;
    XtSetArg(args[nargs],XmNrightAttachment,XmATTACH_POSITION); nargs++;
    XtSetArg(args[nargs],XmNrightPosition,70); nargs++;
    XtSetArg(args[nargs],XmNbottomAttachment,XmATTACH_FORM); nargs++;
    XtSetArg(args[nargs],XmNbottomOffset,12); nargs++;
    XtSetValues(closeButton,args,nargs);

    XtManageChild(rdForm);

    return shell;
}

/*
 * access function (for file-scoped globals, etc) to udpate the
 *  related display data dialog with the values currently in
 *  globalResourceBundle
 */
void updateRelatedDisplayDataDialog()
{
    int i;

    if(rdMatrix) {
	for(i = 0; i < MAX_RELATED_DISPLAYS; i++) {
	    XmTextFieldSetString(table[i][0],
	      globalResourceBundle.rdData[i].label);
	    XmTextFieldSetString(table[i][1],
	      globalResourceBundle.rdData[i].name);
	    XmTextFieldSetString(table[i][2],
	      globalResourceBundle.rdData[i].args);
	    if(globalResourceBundle.rdData[i].mode == REPLACE_DISPLAY) {
		XmToggleButtonSetState(table[i][3],True,False);
	    } else {
		XmToggleButtonSetState(table[i][3],False,False);
	    }
	}
    }
}

void relatedDisplayDataDialogPopup(Widget w)
{
    if(relatedDisplayS == NULL) {
	relatedDisplayS = createRelatedDisplayDataDialog(w);
    }
  /* update related display data from globalResourceBundle */
    updateRelatedDisplayDataDialog();
    XtManageChild(rdForm);
    XtPopup(relatedDisplayS,XtGrabNone);
}

static void relatedDisplayInheritValues(ResourceBundle *pRCB, DlElement *p)
{
    DlRelatedDisplay *dlRelatedDisplay = p->structure.relatedDisplay;
    medmGetValues(pRCB,
      CLR_RC,        &(dlRelatedDisplay->clr),
      BCLR_RC,       &(dlRelatedDisplay->bclr),
      -1);
}

static void relatedDisplayGetValues(ResourceBundle *pRCB, DlElement *p)
{
    DlRelatedDisplay *dlRelatedDisplay = p->structure.relatedDisplay;
    medmGetValues(pRCB,
      X_RC,          &(dlRelatedDisplay->object.x),
      Y_RC,          &(dlRelatedDisplay->object.y),
      WIDTH_RC,      &(dlRelatedDisplay->object.width),
      HEIGHT_RC,     &(dlRelatedDisplay->object.height),
      CLR_RC,        &(dlRelatedDisplay->clr),
      BCLR_RC,       &(dlRelatedDisplay->bclr),
      RD_LABEL_RC,   &(dlRelatedDisplay->label),
      RD_VISUAL_RC,  &(dlRelatedDisplay->visual),
      RDDATA_RC,     &(dlRelatedDisplay->display),
      -1);
}

static void relatedDisplaySetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlRelatedDisplay *dlRelatedDisplay = p->structure.relatedDisplay;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlRelatedDisplay->bclr),
      -1);
}

static void relatedDisplaySetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlRelatedDisplay *dlRelatedDisplay = p->structure.relatedDisplay;
    medmGetValues(pRCB,
      CLR_RC,        &(dlRelatedDisplay->clr),
      -1);
}

/*** Hidden button marking routines ***/

void markHiddenButtons(DisplayInfo *displayInfo)
{
    DlElement *pE;

  /* Don't do anything if the displayInfo is invalid */
    if(!displayInfo) return;

  /* This is a toggle */
    if(displayInfo->nMarkerWidgets) {
      /* They are on.  Turn them off */
	markerWidgetsDestroy(displayInfo);
    } else {
      /* They are off.  Turn them on */
      /* Count the number of hidden buttons */
	pE = FirstDlElement(displayInfo->dlElementList);
	while(pE) {
	    displayInfo->nMarkerWidgets += countHiddenButtons(pE);

	    pE = pE->next;
	}

      /* Popup a dialog and return if there are no hidden buttons */
	if(!displayInfo->nMarkerWidgets) {
	    char token[MAX_TOKEN_LENGTH];

	    sprintf(token,
	      "There are no hidden buttons in this display.");
	    dmSetAndPopupWarningDialog(displayInfo, token, "OK", NULL, NULL);
	    return;
	}

      /* Allocate space for the list of widgets */
	displayInfo->markerWidgetList =
	  (Widget *)malloc(displayInfo->nMarkerWidgets*sizeof(Widget));
	if(!displayInfo->markerWidgetList) {
	    displayInfo->nMarkerWidgets = 0;
	    medmPrintf(1,"\nmarkHiddenButtons: Cannot create widget list\n");
	    return;
	}

      /* Create the marker widgets */
#ifdef WIN32
      /* Seems to take a long time on WIN32 */
	XDefineCursor(display,XtWindow(displayInfo->drawingArea),watchCursor);
	XFlush(display);
#endif
	displayInfo->nMarkerWidgets = 0;
	pE = FirstDlElement(displayInfo->dlElementList);
	while(pE) {
	    createMarkerWidgets(displayInfo, pE);

	    pE = pE->next;
	}
#ifdef WIN32
	XUndefineCursor(display,XtWindow(displayInfo->drawingArea));
#endif
    }
}

static int countHiddenButtons(DlElement *dlElement)
{
    DlElement *pE;
    int nargs = 0;

    if(dlElement->type == DL_Composite) {
	pE = FirstDlElement(dlElement->structure.composite->dlElementList);
	while(pE) {
	    if(pE->type == DL_Composite) {
		nargs+=countHiddenButtons(pE);
	    } else if(pE->type == DL_RelatedDisplay &&
	      pE->structure.relatedDisplay->visual == RD_HIDDEN_BTN) {
		nargs++;
	    }
	    pE = pE->next;
	}
    } else if(dlElement->type == DL_RelatedDisplay &&
      dlElement->structure.relatedDisplay->visual == RD_HIDDEN_BTN) {
	nargs++;
    }

    return(nargs);
}

static void createMarkerWidgets(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlElement *pE;

    if(dlElement->type == DL_Composite) {
	pE = FirstDlElement(dlElement->structure.composite->dlElementList);
	while(pE) {
	    if(pE->type == DL_Composite) {
		createMarkerWidgets(displayInfo, pE);
	    } else if(pE->type == DL_RelatedDisplay &&
	      pE->structure.relatedDisplay->visual == RD_HIDDEN_BTN) {
		createMarkerWidget(displayInfo, pE);
	    }
	    pE = pE->next;
	}
    } else if(dlElement->type == DL_RelatedDisplay &&
      dlElement->structure.relatedDisplay->visual == RD_HIDDEN_BTN) {
	createMarkerWidget(displayInfo, dlElement);
    }
}

static void createMarkerWidget(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlObject *pO = &(dlElement->structure.composite->object);
    int nargs;
    Arg args[10];
    Widget w;
    int x, y, width, height;

  /* Make the marquee surround the hidden button */
    x = pO->x - 1;
    y = pO->y - 1;
    width = pO->width + 2;
    height = pO->height + 2;

    nargs = 0;
    XtSetArg(args[nargs], XmNx, x); nargs++;
    XtSetArg(args[nargs], XmNy, y); nargs++;
    XtSetArg(args[nargs], XmNwidth, width); nargs++;
    XtSetArg(args[nargs], XmNheight, height); nargs++;
    XtSetArg(args[nargs], XmNforeground, WhitePixel(display,screenNum));
    nargs++;
    XtSetArg(args[nargs], XmNbackground, BlackPixel(display,screenNum));
    nargs++;
    XtSetArg(args[nargs], XtNborderWidth, 0); nargs++;
#if !USE_MARQUEE
    XtSetArg(args[nargs], XtNblinkTime, 0); nargs++;
    XtSetArg(args[nargs], XtNtransparent, False); nargs++;
#endif
    w=XtCreateManagedWidget("marquee", marqueeWidgetClass,
      displayInfo->drawingArea, args, nargs);
    displayInfo->markerWidgetList[displayInfo->nMarkerWidgets++] = w;
}

static void markerWidgetsDestroy(DisplayInfo *displayInfo)
{
    int i;
    int nWidgets = displayInfo->nMarkerWidgets;
    Widget *widgets = displayInfo->markerWidgetList;

#ifdef WIN32
      /* Seems to take a long time on WIN32 */
	XDefineCursor(display,XtWindow(displayInfo->drawingArea),watchCursor);
	XFlush(display);
#endif
  /* Unmap them to be sure */
    for(i=0; i < nWidgets; i++) {
	XtUnmapWidget(widgets[i]);
    }
    XFlush(display);

  /* Destroy them */
    for(i=0; i < nWidgets; i++) {
	XtDestroyWidget(widgets[i]);
    }
    if(widgets) {
	free((char *)widgets);
	displayInfo->markerWidgetList = NULL;
    }
    displayInfo->nMarkerWidgets = 0;
#ifdef WIN32
	XUndefineCursor(display,XtWindow(displayInfo->drawingArea));
#endif
}
