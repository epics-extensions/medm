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
 *                              - add version number into the FILE object
 * .03  09-08-95        vong    conform to c++ syntax
 *
 *****************************************************************************
*/

#include "medm.h"
#include <Xm/MwmUtil.h>

#define RD_APPLY_BTN  0
#define RD_CLOSE_BTN  1
 
static Widget rdMatrix = NULL, rdForm = NULL;
static Widget table[MAX_RELATED_DISPLAYS][4];
static Pixmap stipple = NULL;

void relatedDisplayCreateNewDisplay(DisplayInfo *displayInfo,
                                    DlRelatedDisplayEntry *pEntry);

static void relatedDisplayInheritValues(ResourceBundle *pRCB, DlElement *p);
static void relatedDisplayGetValues(ResourceBundle *pRCB, DlElement *p);
static void relatedDisplayButtonPressedCb(Widget, XtPointer, XtPointer);

static DlDispatchTable relatedDisplayDlDispatchTable = {
         createDlRelatedDisplay,
         NULL,
         executeDlRelatedDisplay,
         writeDlRelatedDisplay,
         NULL,
         relatedDisplayGetValues,
         relatedDisplayInheritValues,
         NULL,
         NULL,
         genericMove,
         genericScale,
         NULL,
         NULL};

#ifdef __cplusplus
static void freePixmapCallback(Widget w, XtPointer cd, XtPointer)
#else
static void freePixmapCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
  Pixmap pixmap = (Pixmap) cd;
  if (pixmap != (Pixmap)NULL) XmDestroyPixmap(XtScreen(w),pixmap);
}

/*
 * local function to render the related display icon into a pixmap
 */
static void renderRelatedDisplayPixmap(Display *display, Pixmap pixmap,
        Pixel fg, Pixel bg, Dimension width, Dimension height)
{
  typedef struct { float x; float y;} XY;
/* icon is based on the 25 pixel (w & h) bitmap relatedDisplay25 */
  static float rectangleX = 4./25., rectangleY = 4./25.,
        rectangleWidth = 13./25., rectangleHeight = 14./25.;
  static XY segmentData[] = {
        {16./25.,9./25.},
        {22./25.,9./25.},
        {22./25.,22./25.},
        {10./25.,22./25.},
        {10./25.,18./25.},
  };
  GC gc;
  XSegment segments[4];

  gc = XCreateGC(display,pixmap,0,NULL);
  XSetForeground(display,gc,bg);
  XFillRectangle(display,pixmap,gc,0,0,width,height);
  XSetForeground(display,gc,fg);

  segments[0].x1 = (short)(segmentData[0].x*width);
  segments[0].y1 = (short)(segmentData[0].y*height);
  segments[0].x2 = (short)(segmentData[1].x*width);
  segments[0].y2 = (short)(segmentData[1].y*height);

  segments[1].x1 = (short)(segmentData[1].x*width);
  segments[1].y1 = (short)(segmentData[1].y*height);
  segments[1].x2 = (short)(segmentData[2].x*width);
  segments[1].y2 = (short)(segmentData[2].y*height);

  segments[2].x1 = (short)(segmentData[2].x*width);
  segments[2].y1 = (short)(segmentData[2].y*height);
  segments[2].x2 = (short)(segmentData[3].x*width);
  segments[2].y2 = (short)(segmentData[3].y*height);

  segments[3].x1 = (short)(segmentData[3].x*width);
  segments[3].y1 = (short)(segmentData[3].y*height);
  segments[3].x2 = (short)(segmentData[4].x*width);
  segments[3].y2 = (short)(segmentData[4].y*height);

  XDrawSegments(display,pixmap,gc,segments,4);

/* erase any out-of-bounds edges due to roundoff error by blanking out
 *  area of top rectangle */
  XSetForeground(display,gc,bg);
  XFillRectangle(display,pixmap,gc,
        (int)(rectangleX*width),
        (int)(rectangleY*height),
        (unsigned int)(rectangleWidth*width),
        (unsigned int)(rectangleHeight*height));

/* and draw the top rectangle */
  XSetForeground(display,gc,fg);
  XDrawRectangle(display,pixmap,gc,
        (int)(rectangleX*width),
        (int)(rectangleY*height),
        (unsigned int)(rectangleWidth*width),
        (unsigned int)(rectangleHeight*height));

  XFreeGC(display,gc);
}


#ifdef __cplusplus
int relatedDisplayFontListIndex(
  DlRelatedDisplay *dlRelatedDisplay,
  int numButtons,
  int)
#else
int relatedDisplayFontListIndex(
  DlRelatedDisplay *dlRelatedDisplay,
  int numButtons,
  int maxChars)
#endif
{
  int i, useNumButtons;
  short sqrtNumButtons;
 
#define SHADOWS_SIZE 4    /* each Toggle Button has 2 shadows...*/
 
/* more complicated calculation based on orientation, etc */
  for (i = MAX_FONTS-1; i >=  0; i--) {
    switch (dlRelatedDisplay->visual) {
    case RD_COL_OF_BTN:
      if ( (int)(dlRelatedDisplay->object.height/MAX(1,numButtons)
          - SHADOWS_SIZE) >=
       (fontTable[i]->ascent + fontTable[i]->descent))
      return(i);
      break;
    case RD_ROW_OF_BTN:
      if ( (int)(dlRelatedDisplay->object.height - SHADOWS_SIZE) >=
       (fontTable[i]->ascent + fontTable[i]->descent))
      return(i);
      break;
    }
  }
  return (0);
}

void executeDlRelatedDisplay(DisplayInfo *displayInfo, DlElement *dlElement)
{
  Widget localMenuBar, tearOff;
  Arg args[30];
  int n;
  int i, displayNumber=0;
  char *name, *argsString;
  char **nameArgs;
  XmString xmString;
  Pixmap relatedDisplayPixmap;
  unsigned int pixmapSize;
  int iNumberOfDisplays = 0;
  DlRelatedDisplay *dlRelatedDisplay = dlElement->structure.relatedDisplay;

/*
 * these are widget ids, but they are recorded in the otherChild widget list
 *   as well, for destruction when new displays are selected at the top level
 */
  Widget relatedDisplayPulldownMenu, relatedDisplayMenuButton;
#if 1
  Widget widget;
#endif

  if (dlElement->widget) {
    XtDestroyWidget(dlElement->widget);
    dlElement->widget = NULL;
  }
  /* count number of displays */
  for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
    if (dlRelatedDisplay->display[i].label[0] != '\0') {
      iNumberOfDisplays++;
    }
  } 

  if (iNumberOfDisplays == 1 && dlRelatedDisplay->visual != RD_HIDDEN_BTN) {
    Pixmap pixmap = 0;
    char *label = 0;
    if (dlRelatedDisplay->label[0] != '\0') {
      label = dlRelatedDisplay->label;
    } else {
      Dimension pixmapSize = MIN(dlRelatedDisplay->object.width,
                                 dlRelatedDisplay->object.height);
      /* allowing for shadows etc */
      pixmapSize = (unsigned int) MAX(1,(int)pixmapSize - 8);
 
      /* create relatedDisplay icon (render to appropriate size) */
      pixmap = XCreatePixmap(display,
          RootWindow(display,screenNum), pixmapSize, pixmapSize,
          XDefaultDepth(display,screenNum));
      renderRelatedDisplayPixmap(display,pixmap,
        displayInfo->colormap[dlRelatedDisplay->clr],
        displayInfo->colormap[dlRelatedDisplay->bclr],
        pixmapSize, pixmapSize);
    }
    n = 0;
    dlElement->widget = createPushButton(
                          displayInfo->drawingArea,
                          &(dlRelatedDisplay->object),
                          displayInfo->colormap[dlRelatedDisplay->clr],
                          displayInfo->colormap[dlRelatedDisplay->bclr],
                          pixmap,
                          label,
                          (XtPointer) displayInfo);
    if (globalDisplayListTraversalMode == DL_EDIT) { 
      /* remove all translations if in edit mode */
      XtUninstallTranslations(dlElement->widget);
      /*
       * add button press handlers too
       */
      XtAddEventHandler(dlElement->widget,ButtonPressMask, False,
                      handleButtonPress,(XtPointer)displayInfo);
    } else {
      /* add the callbacks for bring up menu */
      XtAddCallback(dlElement->widget,XmNarmCallback,
                    relatedDisplayButtonPressedCb,
                    (XtPointer) &(dlRelatedDisplay->display[0]));
    }
    XtManageChild(dlElement->widget);
  } else
  if (dlRelatedDisplay->visual == RD_MENU) {
    XmString str;
    n = 0;
    XtSetArg(args[n],XmNbackground,(Pixel)
        displayInfo->colormap[dlRelatedDisplay->bclr]); n++;
    XtSetArg(args[n],XmNforeground,(Pixel)
        displayInfo->colormap[dlRelatedDisplay->clr]); n++;
    XtSetArg(args[n],XmNhighlightThickness,1); n++;
    XtSetArg(args[n],XmNwidth,dlRelatedDisplay->object.width); n++;
    XtSetArg(args[n],XmNheight,dlRelatedDisplay->object.height); n++;
    XtSetArg(args[n],XmNmarginHeight,0); n++;
    XtSetArg(args[n],XmNmarginWidth,0); n++;
    XtSetArg(args[n],XmNresizeHeight,(Boolean)FALSE); n++;
    XtSetArg(args[n],XmNresizeWidth,(Boolean)FALSE); n++;
    XtSetArg(args[n],XmNspacing,0); n++;
    XtSetArg(args[n],XmNx,(Position)dlRelatedDisplay->object.x); n++;
    XtSetArg(args[n],XmNy,(Position)dlRelatedDisplay->object.y); n++;
    XtSetArg(args[n],XmNhighlightOnEnter,TRUE); n++;
    XtSetArg(args[n],XmNtearOffModel,XmTEAR_OFF_DISABLED); n++;
    localMenuBar =
       XmCreateMenuBar(displayInfo->drawingArea,"relatedDisplayMenuBar",args,n);
    XtManageChild(localMenuBar);
    dlElement->widget = localMenuBar;
  
    colorMenuBar(localMenuBar,
        (Pixel)displayInfo->colormap[dlRelatedDisplay->clr],
        (Pixel)displayInfo->colormap[dlRelatedDisplay->bclr]);

    relatedDisplayPulldownMenu = XmCreatePulldownMenu(
        localMenuBar,"relatedDisplayPulldownMenu",args,2);
    tearOff = XmGetTearOffControl(relatedDisplayPulldownMenu);
    if (tearOff) {
      XtVaSetValues(tearOff,
        XmNforeground,(Pixel) displayInfo->colormap[dlRelatedDisplay->clr],
        XmNbackground,(Pixel) displayInfo->colormap[dlRelatedDisplay->bclr],
        XmNtearOffModel,XmTEAR_OFF_DISABLED,
        NULL);
    }

    n = 7;
    XtSetArg(args[n],XmNrecomputeSize,(Boolean)False); n++;
    if (dlRelatedDisplay->label[0] == '\0') {
      pixmapSize = MIN(dlRelatedDisplay->object.width,
                        dlRelatedDisplay->object.height);
      /* allowing for shadows etc */
      pixmapSize = (unsigned int) MAX(1,(int)pixmapSize - 8);

      /* create relatedDisplay icon (render to appropriate size) */
      relatedDisplayPixmap = XCreatePixmap(display,
          RootWindow(display,screenNum), pixmapSize, pixmapSize,
          XDefaultDepth(display,screenNum));
      renderRelatedDisplayPixmap(display,relatedDisplayPixmap,
        displayInfo->colormap[dlRelatedDisplay->clr],
        displayInfo->colormap[dlRelatedDisplay->bclr],
        pixmapSize, pixmapSize);
      XtSetArg(args[n],XmNlabelPixmap,relatedDisplayPixmap); n++;
      XtSetArg(args[n],XmNlabelType,XmPIXMAP); n++;
    } else {
      XtSetArg(args[n],XmNlabelType,XmSTRING); n++;
      XtSetArg(args[n],XmNfontList,fontListTable[
        messageButtonFontListIndex(dlRelatedDisplay->object.height)]); n++;
    }
    XtSetArg(args[n],XmNsubMenuId,relatedDisplayPulldownMenu); n++;
    XtSetArg(args[n],XmNhighlightOnEnter,TRUE); n++;
    XtSetArg(args[n],XmNalignment,XmALIGNMENT_BEGINNING); n++;

    XtSetArg(args[n],XmNmarginLeft,0); n++;
    XtSetArg(args[n],XmNmarginRight,0); n++;
    XtSetArg(args[n],XmNmarginTop,0); n++;
    XtSetArg(args[n],XmNmarginBottom,0); n++;
    XtSetArg(args[n],XmNmarginWidth,0); n++;
    XtSetArg(args[n],XmNmarginHeight,0); n++;

    widget = XtCreateManagedWidget("relatedDisplayMenuLabel",
           xmCascadeButtonGadgetClass,
           dlElement->widget, args, n);

    str = XmStringCreateSimple(dlRelatedDisplay->label);
    XtVaSetValues(widget,XmNlabelString,str,NULL);
    XmStringFree(str);

    XtAddCallback(widget, XmNdestroyCallback,freePixmapCallback,
        (XtPointer)relatedDisplayPixmap);

    for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
      if (strlen(dlRelatedDisplay->display[i].name) > (size_t)1) {
         xmString = XmStringCreateSimple(dlRelatedDisplay->display[i].label);
         XtSetArg(args[3], XmNlabelString,xmString);
         XtSetArg(args[4], XmNuserData, displayInfo);
         relatedDisplayMenuButton = XtCreateManagedWidget("relatedButton",
                xmPushButtonWidgetClass, relatedDisplayPulldownMenu, args, 5);
         XtAddCallback(relatedDisplayMenuButton,XmNactivateCallback,
                relatedDisplayButtonPressedCb,&(dlRelatedDisplay->display[i]));
         XmStringFree(xmString);
      }
    }


  /* add event handlers to relatedDisplay... */
    if (displayInfo->traversalMode == DL_EDIT) {

  /* remove all translations if in edit mode */
      XtUninstallTranslations(localMenuBar);

      XtAddEventHandler(localMenuBar,ButtonPressMask,False,
                handleButtonPress, (XtPointer)displayInfo);
    }
  } else
  if (dlRelatedDisplay->visual == RD_ROW_OF_BTN || 
      dlRelatedDisplay->visual == RD_COL_OF_BTN) {
    Arg wargs[20];
    int i, n, maxChars, usedWidth, usedHeight;
    short sqrtEntries;
    double dSqrt;
    XmFontList fontList;
    Pixel fg, bg;
    Widget widget;
 
    maxChars = 0;
    for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
      maxChars = MAX((size_t) maxChars,
                     strlen(dlRelatedDisplay->display[i].label));
    }
    fg = displayInfo->colormap[dlRelatedDisplay->clr];
    bg = displayInfo->colormap[dlRelatedDisplay->bclr];
    n = 0;
    XtSetArg(wargs[n],XmNx,(Position)dlRelatedDisplay->object.x); n++;
    XtSetArg(wargs[n],XmNy,(Position)dlRelatedDisplay->object.y); n++;
    XtSetArg(wargs[n],XmNwidth,(Dimension)dlRelatedDisplay->object.width); n++;
    XtSetArg(wargs[n],XmNheight,(Dimension)dlRelatedDisplay->object.height); n++;
    XtSetArg(wargs[n],XmNforeground,fg); n++;
    XtSetArg(wargs[n],XmNbackground,bg); n++;
    XtSetArg(wargs[n],XmNindicatorOn,(Boolean)FALSE); n++;
    XtSetArg(wargs[n],XmNmarginWidth,0); n++;
    XtSetArg(wargs[n],XmNmarginHeight,0); n++;
    XtSetArg(wargs[n],XmNresizeWidth,(Boolean)FALSE); n++;
    XtSetArg(wargs[n],XmNresizeHeight,(Boolean)FALSE); n++;
    XtSetArg(wargs[n],XmNspacing,0); n++;
    XtSetArg(wargs[n],XmNrecomputeSize,(Boolean)FALSE); n++;
    XtSetArg(wargs[n],XmNuserData,displayInfo); n++;
    switch (dlRelatedDisplay->visual) {
    case RD_COL_OF_BTN:
      XtSetArg(wargs[n],XmNorientation,XmVERTICAL); n++;
      usedWidth = dlRelatedDisplay->object.width;
      usedHeight = (int) (dlRelatedDisplay->object.height/
                          MAX(1,iNumberOfDisplays));
      break;
    case RD_ROW_OF_BTN:
      XtSetArg(wargs[n],XmNorientation,XmHORIZONTAL); n++;
      usedWidth = (int) (dlRelatedDisplay->object.width/
                        MAX(1,iNumberOfDisplays));
      usedHeight = dlRelatedDisplay->object.height;
      break;
    default:
      break;
    }
    widget = XmCreateRowColumn(displayInfo->drawingArea,"radioBox",wargs,n);
    dlElement->widget = widget;
 
    /* now make push-in type radio buttons of the correct size */
    fontList = fontListTable[relatedDisplayFontListIndex(
      dlRelatedDisplay,iNumberOfDisplays,maxChars)];
    n = 0;
    XtSetArg(wargs[n],XmNindicatorOn,False); n++;
    XtSetArg(wargs[n],XmNshadowThickness,2); n++;
    XtSetArg(wargs[n],XmNhighlightThickness,1); n++;
    XtSetArg(wargs[n],XmNrecomputeSize,(Boolean)FALSE); n++;
    XtSetArg(wargs[n],XmNwidth,(Dimension)usedWidth); n++;
    XtSetArg(wargs[n],XmNheight,(Dimension)usedHeight); n++;
    XtSetArg(wargs[n],XmNfontList,fontList); n++;
    XtSetArg(wargs[n],XmNalignment,XmALIGNMENT_CENTER); n++;
    XtSetArg(wargs[n],XmNindicatorOn,False); n++;
    XtSetArg(wargs[n],XmNindicatorSize,0); n++;
    XtSetArg(wargs[n],XmNspacing,0); n++;
    XtSetArg(wargs[n],XmNvisibleWhenOff,False); n++;
    XtSetArg(wargs[n],XmNforeground,fg); n++;
    XtSetArg(wargs[n],XmNbackground,bg); n++;
    XtSetArg(wargs[n],XmNalignment,XmALIGNMENT_CENTER); n++;
    XtSetArg(wargs[n],XmNuserData,displayInfo); n++;
    for (i = 0; i < iNumberOfDisplays; i++) {
      XmString xmStr;
      Widget   toggleButton;
      xmStr = XmStringCreateSimple(dlRelatedDisplay->display[i].label);
      XtSetArg(wargs[n],XmNlabelString,xmStr);
      /* use gadgets here so that changing foreground of
         radioBox changes buttons */
      toggleButton = XmCreatePushButtonGadget(widget,"toggleButton",
          wargs,n+1);
      if (globalDisplayListTraversalMode == DL_EXECUTE) {
        XtAddCallback(toggleButton,XmNarmCallback,relatedDisplayButtonPressedCb,
           (XtPointer) &(dlRelatedDisplay->display[i]));
      }  else {
        XtSetSensitive(toggleButton,False);
        XtUninstallTranslations(toggleButton);
      }
      /* MDA - for some reason, need to do this after the fact for gadgets... */
      XtVaSetValues(toggleButton,XmNalignment,XmALIGNMENT_CENTER,NULL);
      XtManageChild(toggleButton);
    }
    if (globalDisplayListTraversalMode == DL_EDIT) {
      /* add button press handlers for editing */
      XtAddEventHandler(widget, ButtonPressMask, False,
                      handleButtonPress,displayInfo);
    } else {
      /* add in drag/drop translations */
      XtOverrideTranslations(widget,parsedTranslations);
    }
    XtManageChild(widget);
  } else
  if (dlRelatedDisplay->visual == RD_HIDDEN_BTN) {
    unsigned long gcValueMask;
    XGCValues gcValues;
    Display *display = XtDisplay(displayInfo->drawingArea);
    gcValueMask = GCForeground | GCBackground | GCFillStyle | GCStipple;
    gcValues.foreground = displayInfo->colormap[dlRelatedDisplay->clr];
    gcValues.background = displayInfo->colormap[dlRelatedDisplay->bclr];
    gcValues.fill_style = FillStippled;
    if (!stipple) {
      static char stipple_bitmap[] = {0x03, 0x03, 0x0c, 0x0c};

      stipple = XCreateBitmapFromData(display,
                  RootWindow(display, DefaultScreen(display)),
                  stipple_bitmap, 4, 4);
    }
    gcValues.stipple = stipple;
    XChangeGC(XtDisplay(displayInfo->drawingArea),
              displayInfo->gc,
							gcValueMask, &gcValues);
    XFillRectangle(XtDisplay(displayInfo->drawingArea),
          XtWindow(displayInfo->drawingArea),displayInfo->gc,
          dlRelatedDisplay->object.x,dlRelatedDisplay->object.y,
          dlRelatedDisplay->object.width,dlRelatedDisplay->object.height);
    XFillRectangle(XtDisplay(displayInfo->drawingArea),
          displayInfo->drawingAreaPixmap,displayInfo->gc,
          dlRelatedDisplay->object.x,dlRelatedDisplay->object.y,
          dlRelatedDisplay->object.width,dlRelatedDisplay->object.height);
  }
}

#ifdef __cplusplus
static void createDlRelatedDisplayEntry(
  DlRelatedDisplayEntry *relatedDisplay,
  int displayNumber)
#else
static void createDlRelatedDisplayEntry(
  DlRelatedDisplayEntry *relatedDisplay,
  int displayNumber)
#endif
{
/* structure copy */
  *relatedDisplay = globalResourceBundle.rdData[displayNumber];
}

DlElement *createDlRelatedDisplay(DlElement *p)
{
  DlRelatedDisplay *dlRelatedDisplay;
  DlElement *dlElement;
  int displayNumber;

  dlRelatedDisplay = (DlRelatedDisplay *) malloc(sizeof(DlRelatedDisplay));
  if (!dlRelatedDisplay) return 0;
  if (p) {
    *dlRelatedDisplay = *p->structure.relatedDisplay;
  } else {
    objectAttributeInit(&(dlRelatedDisplay->object));
    for (displayNumber = 0; displayNumber < MAX_RELATED_DISPLAYS;
         displayNumber++)
	  createDlRelatedDisplayEntry(
			&(dlRelatedDisplay->display[displayNumber]),
			displayNumber );

    dlRelatedDisplay->clr = globalResourceBundle.clr;
    dlRelatedDisplay->bclr = globalResourceBundle.bclr;
    dlRelatedDisplay->label[0] = '\0';
    dlRelatedDisplay->visual = RD_MENU;
  }

  if (!(dlElement = createDlElement(DL_RelatedDisplay,
                    (XtPointer)      dlRelatedDisplay,
                    &relatedDisplayDlDispatchTable))) {
    free(dlRelatedDisplay);
  }

  return(dlElement);
}

void parseRelatedDisplayEntry(DisplayInfo *displayInfo, DlRelatedDisplayEntry *relatedDisplay)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;

  do {
    switch( (tokenType=getToken(displayInfo,token)) ) {
    case T_WORD:
      if (!strcmp(token,"label")) {
        getToken(displayInfo,token);
        getToken(displayInfo,token);
        strcpy(relatedDisplay->label,token);
      } else if (!strcmp(token,"name")) {
        getToken(displayInfo,token);
        getToken(displayInfo,token);
        strcpy(relatedDisplay->name,token);
      } else if (!strcmp(token,"args")) {
        getToken(displayInfo,token);
        getToken(displayInfo,token);
        strcpy(relatedDisplay->args,token);
      } else if (!strcmp(token,"policy")) {
        getToken(displayInfo,token);
        getToken(displayInfo,token);
        if (!strcmp(token,stringValueTable[REPLACE_DISPLAY]))
          relatedDisplay->mode = REPLACE_DISPLAY;
      }
      break;
    case T_LEFT_BRACE:
      nestingLevel++; break;
    case T_RIGHT_BRACE:
      nestingLevel--; break;
    }
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
                && (tokenType != T_EOF) );
}

DlElement *parseRelatedDisplay(DisplayInfo *displayInfo)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlRelatedDisplay *dlRelatedDisplay = 0;
  DlElement *dlElement = createDlRelatedDisplay(NULL);
  int displayNumber;

  if (!dlElement) return 0;
  dlRelatedDisplay = dlElement->structure.relatedDisplay;

  do {
    switch( (tokenType=getToken(displayInfo,token)) ) {
    case T_WORD:
      if (!strcmp(token,"object")) {
        parseObject(displayInfo,&(dlRelatedDisplay->object));
      } else if (!strncmp(token,"display",7)) {
/*
 * compare the first 7 characters to see if a display entry.
 *   if more than one digit is allowed for the display index, then change
 *   the following code to pick up all the digits (can't use atoi() unless
 *   we get a null-terminated string
 */
        displayNumber = MIN(token[8] - '0', MAX_RELATED_DISPLAYS-1);
        parseRelatedDisplayEntry(displayInfo,
            &(dlRelatedDisplay->display[displayNumber]) );
      } else if (!strcmp(token,"clr")) {
        getToken(displayInfo,token);
        getToken(displayInfo,token);
        dlRelatedDisplay->clr = atoi(token) % DL_MAX_COLORS;
      } else if (!strcmp(token,"bclr")) {
        getToken(displayInfo,token);
        getToken(displayInfo,token);
        dlRelatedDisplay->bclr = atoi(token) % DL_MAX_COLORS;
      } else if (!strcmp(token,"label")) {
        getToken(displayInfo,token);
        getToken(displayInfo,token);
        strcpy(dlRelatedDisplay->label,token);
      } else if (!strcmp(token,"visual")) {
        getToken(displayInfo,token);
        getToken(displayInfo,token);
        if (!strcmp(token,stringValueTable[FIRST_RD_VISUAL+1])) {
          dlRelatedDisplay->visual = RD_ROW_OF_BTN;
        } else
        if (!strcmp(token,stringValueTable[FIRST_RD_VISUAL+2])) {
          dlRelatedDisplay->visual = RD_COL_OF_BTN;
        } else
        if (!strcmp(token,stringValueTable[FIRST_RD_VISUAL+3])) {
          dlRelatedDisplay->visual = RD_HIDDEN_BTN;
        }
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

  return dlElement;

}

void writeDlRelatedDisplayEntry(
  FILE *stream,
  DlRelatedDisplayEntry *entry,
  int index,
  int level)
{
  char indent[16];

  memset(indent,'\t',level);
  indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
  if (MedmUseNewFileFormat) {
#endif
		fprintf(stream,"\n%sdisplay[%d] {",indent,index);
		if (entry->label[0] != '\0')
			fprintf(stream,"\n%s\tlabel=\"%s\"",indent,entry->label);
		if (entry->name[0] != '\0')
			fprintf(stream,"\n%s\tname=\"%s\"",indent,entry->name);
		if (entry->args[0] != '\0')
			fprintf(stream,"\n%s\targs=\"%s\"",indent,entry->args);
    if (entry->mode != ADD_NEW_DISPLAY)
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

void writeDlRelatedDisplay(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
  int i;
  char indent[16];
  DlRelatedDisplay *dlRelatedDisplay = dlElement->structure.relatedDisplay;

  memset(indent,'\t',level);
  indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
  if (MedmUseNewFileFormat) {
#endif
		fprintf(stream,"\n%s\"related display\" {",indent);
		writeDlObject(stream,&(dlRelatedDisplay->object),level+1);
		for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
		if ((dlRelatedDisplay->display[i].label[0] != '\0') ||
			(dlRelatedDisplay->display[i].name[0] != '\0') ||
			(dlRelatedDisplay->display[i].args[0] != '\0'))
			writeDlRelatedDisplayEntry(stream,&(dlRelatedDisplay->display[i]),i,level+1);
		}
		fprintf(stream,"\n%s\tclr=%d",indent,dlRelatedDisplay->clr);
		fprintf(stream,"\n%s\tbclr=%d",indent,dlRelatedDisplay->bclr);
    if (dlRelatedDisplay->label[0] != '\0') 
      fprintf(stream,"\n%s\tlabel=%s",indent,dlRelatedDisplay->label);
    if (dlRelatedDisplay->visual != RD_MENU)
      fprintf(stream,"\n%s\tvisual=\"%s\"",
              indent,stringValueTable[dlRelatedDisplay->visual]);
		fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
	} else {
		fprintf(stream,"\n%s\"related display\" {",indent);
		writeDlObject(stream,&(dlRelatedDisplay->object),level+1);
		for (i = 0; i < MAX_RELATED_DISPLAYS; i++)
			writeDlRelatedDisplayEntry(stream,&(dlRelatedDisplay->display[i]),i,level+1);
		fprintf(stream,"\n%s\tclr=%d",indent,dlRelatedDisplay->clr);
		fprintf(stream,"\n%s\tbclr=%d",indent,dlRelatedDisplay->bclr);
		fprintf(stream,"\n%s}",indent);
	}
#endif
}

static void relatedDisplayInheritValues(ResourceBundle *pRCB, DlElement *p) {
  DlRelatedDisplay *dlRelatedDisplay = p->structure.relatedDisplay;
  medmGetValues(pRCB,
    CLR_RC,        &(dlRelatedDisplay->clr),
    BCLR_RC,       &(dlRelatedDisplay->bclr),
    -1);
}

static void relatedDisplayGetValues(ResourceBundle *pRCB, DlElement *p) {
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

static void relatedDisplayButtonPressedCb(Widget w,
                XtPointer clientData,
                XtPointer callbackData) {
  DlRelatedDisplayEntry *pEntry = (DlRelatedDisplayEntry *) clientData;
  DisplayInfo *displayInfo = 0;
  XtVaGetValues(w,XmNuserData,&displayInfo,NULL);
  relatedDisplayCreateNewDisplay(displayInfo,pEntry);
}

void relatedDisplayCreateNewDisplay(DisplayInfo *displayInfo,
                                    DlRelatedDisplayEntry *pEntry) { 
  char *filename, *argsString, *newFilename, token[MAX_TOKEN_LENGTH];
  FILE *filePtr;
  char *adlPtr;
  char processedArgs[2*MAX_TOKEN_LENGTH];
  int suffixLength, prefixLength;
  filename = pEntry->name;
  argsString = pEntry->args;
/*
 * if we want to be able to have RD's inherit their parent's
 *   macro-substitutions, then we must perform any macro substitution on
 *   this argument string in this displayInfo's context before passing
 *   it to the created child display
 */
  if (globalDisplayListTraversalMode == DL_EXECUTE) {
    performMacroSubstitutions(displayInfo,argsString,processedArgs,
          2*MAX_TOKEN_LENGTH);
    filePtr = dmOpenUseableFile(filename);
    if (filePtr == NULL) {
      newFilename = STRDUP(filename);
      adlPtr = strstr(filename,DISPLAY_FILE_ASCII_SUFFIX);
      if (adlPtr != NULL) {
       /* ascii name */
        suffixLength = strlen(DISPLAY_FILE_ASCII_SUFFIX);
      } else {
        /* binary name */
        suffixLength = strlen(DISPLAY_FILE_BINARY_SUFFIX);
      }
      prefixLength = strlen(newFilename) - suffixLength;
      newFilename[prefixLength] = '\0';
      sprintf(token,
         "Can't open related display:\n\n        %s%s\n\n%s",
          newFilename, DISPLAY_FILE_ASCII_SUFFIX,
          "--> check EPICS_DISPLAY_PATH ");
      dmSetAndPopupWarningDialog(displayInfo,token,"Ok",NULL,NULL);
      fprintf(stderr,"\n%s",token);
      free(newFilename);
    } else {
      if (pEntry->mode == REPLACE_DISPLAY) {
        dmDisplayListParse(displayInfo,filePtr,processedArgs,
                          filename,NULL,(Boolean)True);
        fclose(filePtr);
      } else {
        dmDisplayListParse(NULL,filePtr,processedArgs,filename,NULL,(Boolean)True);
        fclose(filePtr);
      }
    }
  }
}

#ifdef __cplusplus
static void relatedDisplayActivate(Widget, XtPointer cd, XtPointer) {
#else
static void relatedDisplayActivate(Widget w, XtPointer cd, XtPointer cbs) {
#endif
  int buttonType = (int) cd;
  String **newCells;
  int i;

  switch (buttonType) {
 
    case RD_APPLY_BTN:
      /* commit changes in matrix to global matrix array data */
      for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
        char *tmp = NULL;
        if (tmp = XmTextFieldGetString(table[i][0])) {
          strcpy(globalResourceBundle.rdData[i].label, tmp);
          XtFree(tmp);
        }
        if (tmp = XmTextFieldGetString(table[i][1])) {
          strcpy(globalResourceBundle.rdData[i].name, tmp);
          XtFree(tmp);
        }
        if (tmp = XmTextFieldGetString(table[i][2])) {
          strcpy(globalResourceBundle.rdData[i].args, tmp);
          XtFree(tmp);
        }
        if (XmToggleButtonGetState(table[i][3])) {
          globalResourceBundle.rdData[i].mode = REPLACE_DISPLAY;
        } else {
          globalResourceBundle.rdData[i].mode = ADD_NEW_DISPLAY;
        }
      }
      if (currentDisplayInfo) {
        DlElement *dlElement =
                   FirstDlElement(currentDisplayInfo->selectedDlElementList);
        while (dlElement) {
          if (dlElement->structure.element->type == DL_RelatedDisplay) {
            updateElementFromGlobalResourceBundle(
                dlElement->structure.element);
          }
          dlElement = dlElement->next;
        }
      }
      if (currentDisplayInfo->hasBeenEditedButNotSaved == False)
        medmMarkDisplayBeingEdited(currentDisplayInfo);
      break;
 
    case RD_CLOSE_BTN:
      if (XtClass(w) == xmPushButtonWidgetClass) {
        XtPopdown(relatedDisplayS);
        XtUnmanageChild(relatedDisplayS);
      }
      break;
  }
}
 
/*
 * create related display data dialog
 */
Widget createRelatedDisplayDataDialog (Widget parent) {
  Widget form, shell, applyButton, closeButton;
  Dimension cWidth, cHeight, aWidth, aHeight;
  Arg args[12];
  XmString xmString;
  int i, j, n;
 
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
 
  n = 0;
  XtSetArg(args[n],XmNautoUnmanage,False); n++;
  XtSetArg(args[n],XmNmarginHeight,8); n++;
  XtSetArg(args[n],XmNmarginWidth,8); n++;
  rdForm = XmCreateFormDialog(parent,"relatedDisplayDataF",args,n);
  shell = XtParent(rdForm);
  XtVaSetValues(shell,
      XmNtitle,"Related Display Data",
      XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH,
      NULL);
 
  XmAddWMProtocolCallback(shell,WM_DELETE_WINDOW,
    relatedDisplayActivate,(XtPointer)RD_CLOSE_BTN);
  n = 0;
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
  for (i=0; i<MAX_RELATED_DISPLAYS; i++) {
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

  closeButton = XtVaCreateWidget("Close",
                xmPushButtonWidgetClass, rdForm,
                NULL);
  XtAddCallback(closeButton,XmNactivateCallback,
    relatedDisplayActivate,(XtPointer)RD_CLOSE_BTN);
  XtManageChild(closeButton);

  applyButton = XtVaCreateWidget("Apply",
                xmPushButtonWidgetClass, rdForm,
                NULL); 
  XtAddCallback(applyButton,XmNactivateCallback,
    relatedDisplayActivate,(XtPointer)RD_APPLY_BTN);
  XtManageChild(applyButton);
 
/* make APPLY and CLOSE buttons same size */
  XtVaGetValues(closeButton,XmNwidth,&cWidth,XmNheight,&cHeight,NULL);
  XtVaGetValues(applyButton,XmNwidth,&aWidth,XmNheight,&aHeight,NULL);
  XtVaSetValues(closeButton,XmNwidth,MAX(cWidth,aWidth),
      XmNheight,MAX(cHeight,aHeight),NULL);
 
/*
 * now do form layout
 */
 
/* rdMatrix */
  n = 0;
  XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
  XtSetValues(rdMatrix,args,n);
/* apply */
  n = 0;
  XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
  XtSetArg(args[n],XmNtopWidget,rdMatrix); n++;
  XtSetArg(args[n],XmNtopOffset,12); n++;
  XtSetArg(args[n],XmNleftAttachment,XmATTACH_POSITION); n++;
  XtSetArg(args[n],XmNleftPosition,30); n++;
  XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n],XmNbottomOffset,12); n++;
  XtSetValues(applyButton,args,n);
/* close */
  n = 0;
  XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
  XtSetArg(args[n],XmNtopWidget,rdMatrix); n++;
  XtSetArg(args[n],XmNtopOffset,12); n++;
  XtSetArg(args[n],XmNrightAttachment,XmATTACH_POSITION); n++;
  XtSetArg(args[n],XmNrightPosition,70); n++;
  XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n],XmNbottomOffset,12); n++;
  XtSetValues(closeButton,args,n);
 
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

  if (rdMatrix) { 
    for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
      XmTextFieldSetString(table[i][0],globalResourceBundle.rdData[i].label);
      XmTextFieldSetString(table[i][1],globalResourceBundle.rdData[i].name);
      XmTextFieldSetString(table[i][2],globalResourceBundle.rdData[i].args);
      if (globalResourceBundle.rdData[i].mode == REPLACE_DISPLAY) {
        XmToggleButtonSetState(table[i][3],True,False);
      } else {
        XmToggleButtonSetState(table[i][3],False,False);
      }
    }
  }
}

void relatedDisplayDataDialogPopup(Widget w) {
  if (relatedDisplayS == NULL) {
          relatedDisplayS = createRelatedDisplayDataDialog(w);
  }
  /* update related display data from globalResourceBundle */
  updateRelatedDisplayDataDialog();
  XtManageChild(rdForm);
  XtPopup(relatedDisplayS,XtGrabNone);
}
