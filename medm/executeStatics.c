
#include "medm.h"

#include <X11/keysym.h>
#include <Xm/MwmUtil.h>

static XtCallbackProc freePixmapCallback(
  Widget widget,
  Pixmap pixmap,
  XmAnyCallbackStruct *call_data)
{
  if (pixmap != (Pixmap)NULL) XmDestroyPixmap(XtScreen(widget),pixmap);
}


void executeDlFile(DisplayInfo *displayInfo, DlFile *dlFile, Boolean dummy)
{
  displayInfo->displayFileName = dlFile->name;
}



/*
 * this execute... function is a bit unique in that it can properly handle
 *	being called on extant widgets (drawingArea's/shells) - all other
 *	execute... functions want to always create new widgets (since there
 *	is no direct record of the widget attached to the object/element)
 *	{this can be fixed to save on widget create/destroy cycles later}
 */
void executeDlDisplay(DisplayInfo *displayInfo, DlDisplay *dlDisplay,
		Boolean dummy)
{
  DlColormap *dlColormap;
  Arg args[12];
  int i, k, n, startPos;


/* set the displayInfo useDynamicAttribute flag FALSE initially */
  displayInfo->useDynamicAttribute = FALSE;

/* set the display's foreground and background colors */
  displayInfo->drawingAreaBackgroundColor = dlDisplay->bclr;
  displayInfo->drawingAreaForegroundColor = dlDisplay->clr;

/* from the DlDisplay structure, we've got drawingArea's dimensions */
  n = 0;
  XtSetArg(args[n],XmNwidth,(Dimension)dlDisplay->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlDisplay->object.height); n++;
  XtSetArg(args[n],XmNborderWidth,(Dimension)0); n++;
  XtSetArg(args[n],XmNmarginWidth,(Dimension)0); n++;
  XtSetArg(args[n],XmNmarginHeight,(Dimension)0); n++;
  XtSetArg(args[n],XmNshadowThickness,(Dimension)0); n++;
  XtSetArg(args[n],XmNresizePolicy,XmRESIZE_NONE); n++;
/* N.B.: don't use userData resource since it is used later on for aspect
 *   ratio-preserving resizes */

  if (displayInfo->drawingArea == NULL) {

      displayInfo->drawingArea = XmCreateDrawingArea(displayInfo->shell,
		"displayDA",args,n);
    /* add expose & resize  & input callbacks for drawingArea */
     XtAddCallback(displayInfo->drawingArea,XmNexposeCallback,
		(XtCallbackProc)drawingAreaCallback,(XtPointer)displayInfo);
     XtAddCallback(displayInfo->drawingArea,XmNresizeCallback,
		(XtCallbackProc)drawingAreaCallback,(XtPointer)displayInfo);
     XtManageChild(displayInfo->drawingArea);


   /*
    * and if in EDIT mode...
    */
    if (displayInfo->traversalMode == DL_EDIT) {

   /* handle input (arrow keys) */
	XtAddCallback(displayInfo->drawingArea,XmNinputCallback,
		(XtCallbackProc)drawingAreaCallback,(XtPointer)displayInfo);
   /* and handle button presses and enter windows */
	XtAddEventHandler(displayInfo->drawingArea,ButtonPressMask,False,
		(XtEventHandler)handleButtonPress,(XtPointer)displayInfo);
	XtAddEventHandler(displayInfo->drawingArea,EnterWindowMask,False,
		(XtEventHandler)handleEnterWindow,(XtPointer)displayInfo);

    } else if (displayInfo->traversalMode == DL_EXECUTE) {

/*
 *  MDA --- HACK to fix DND visuals problem with SUN server
 *   Note: This call is in here strictly to satisfy some defect in
 *		the MIT and other X servers for SUNOS machines
 *	   This is completely unnecessary for HP, DEC, NCD, ...
 */
XtSetArg(args[0],XmNdropSiteType,XmDROP_SITE_COMPOSITE);
XmDropSiteRegister(displayInfo->drawingArea,args,1);


   /* add in drag/drop translations */
	XtOverrideTranslations(displayInfo->drawingArea,parsedTranslations);


    }

  } else  {
      XtSetValues(displayInfo->drawingArea,args,n);
  }



  if (XtIsShell(displayInfo->shell)) {
  /***
   *** if parent ("shell") is not a shell, then doing embedded MEDM,
   ***  and most of this is inappropriate
   ***
   *** otherwise, a real shell, and do normal MEDM
   ***/

/* wait to realize the shell... */
    n = 0;
    if (!XtIsRealized(displayInfo->shell)) {	/* only position first time */
      XtSetArg(args[n],XmNx,(Position)dlDisplay->object.x); n++;
      XtSetArg(args[n],XmNy,(Position)dlDisplay->object.y); n++;
    }
    XtSetArg(args[n],XmNallowShellResize,(Boolean)TRUE); n++;
    XtSetArg(args[n],XmNwidth,(Dimension)dlDisplay->object.width); n++;
    XtSetArg(args[n],XmNheight,(Dimension)dlDisplay->object.height); n++;
/* just give filename as title */
    startPos = 0;
    for (k = 0; k < strlen(displayInfo->displayFileName); k++) {
      if (displayInfo->displayFileName[k] == '/') startPos = k;
    }
    XtSetArg(args[n],XmNtitle,&(displayInfo->displayFileName[
	startPos > 0 ? startPos+1 : 0])); n++;
    XtSetArg(args[n],XmNiconName,displayInfo->displayFileName); n++;
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
    XtSetValues(displayInfo->shell,args,n);

    XtRealizeWidget(displayInfo->shell);

  }


/* if there is an external colormap file specification, parse/execute it now */


  if (strlen(dlDisplay->cmap) > 1)  {

      dlColormap = parseAndExtractExternalColormap(displayInfo,
				dlDisplay->cmap);
      if (dlColormap != NULL) {
	  executeDlColormap(displayInfo,dlColormap,False);
      } else {
	 fprintf(stderr,
	 "\nexecuteDlDisplay: can't parse and execute external colormap %s",
	  dlDisplay->cmap);
	  dmTerminateCA();
	  dmTerminateX();
	  exit(-1);
      }
  }

}



/*
 * this function gets called for executing the colormap section of
 *  each display list, for new display creation and for edit <-> execute
 *  transitions.  we could be more clever here for the edit/execute
 *  transitions and not re-execute the colormap info, but that would
 *  require changes to the cleanup code, etc...  hence let us leave
 *  this as is (since the colors are properly being freed (ref-count
 *  being decremented) and performance seems fine...
 *  -- for edit <-> execute type running, performance is not a big issue
 *  anyway and there is no additional cost incurred for the straight
 *  execute time running --
 */
void executeDlColormap(DisplayInfo *displayInfo, DlColormap *dlColormap,
			Boolean dummy)
{
  Arg args[10];
  int i, n;
  Dimension width, height;
  XtGCMask valueMask;
  XGCValues values;
  XColor color;

  if (displayInfo == NULL) return;
/* already have a colormap - don't allow a second one! */
  if (displayInfo->dlColormap != NULL) return;


  displayInfo->dlColormap = (unsigned long *) malloc(
		dlColormap->ncolors * sizeof(unsigned long));
  displayInfo->dlColormapSize = dlColormap->ncolors;

/**** allocate the X colormap from dlColormap data ****/

  for (i = 0; i < dlColormap->ncolors; i++) {

/* scale [0,255] to [0,65535] */
    color.red   = (unsigned short) COLOR_SCALE*(dlColormap->dl_color[i].r); 
    color.green = (unsigned short) COLOR_SCALE*(dlColormap->dl_color[i].g); 
    color.blue  = (unsigned short) COLOR_SCALE*(dlColormap->dl_color[i].b); 
/* allocate a shareable color cell with closest RGB value */

    if (XAllocColor(display,cmap,&color)) {

     displayInfo->dlColormap[displayInfo->dlColormapCounter] = color.pixel;

    } else {

     fprintf(stderr,"\nexecuteDlColormap: couldn't allocate requested color");
        displayInfo->dlColormap[displayInfo->dlColormapCounter] 
		= unphysicalPixel;
    }

    if (displayInfo->dlColormapCounter < displayInfo->dlColormapSize) 
	displayInfo->dlColormapCounter++;
    else fprintf(stderr,"\nexecuteDlColormap:  too many colormap entries");
    	/* just keep rewriting that last colormap entry */
  }


/*
 * set the foreground and background of the display 
 */
  XtSetArg(args[0],XmNbackground,(Pixel)
	displayInfo->dlColormap[displayInfo->drawingAreaBackgroundColor]);
  XtSetValues(displayInfo->drawingArea,args,1);

/* and create the drawing area pixmap */
  n = 0;
  XtSetArg(args[n],XmNwidth,(Dimension *)&width); n++;
  XtSetArg(args[n],XmNheight,(Dimension *)&height); n++;
  XtGetValues(displayInfo->drawingArea,args,n);
  if (displayInfo->drawingAreaPixmap == (Pixmap)NULL)
      displayInfo->drawingAreaPixmap = XCreatePixmap(display,
	RootWindow(display,screenNum),MAX(1,width),MAX(1,height),
	DefaultDepth(display,screenNum));
  else {
      XFreePixmap(display,displayInfo->drawingAreaPixmap);
      displayInfo->drawingAreaPixmap = XCreatePixmap(display,
	RootWindow(display,screenNum),MAX(1,width),MAX(1,height),
	DefaultDepth(display,screenNum));
  }

/* create the pixmap GC */
  valueMask = GCForeground | GCBackground ;
  values.foreground = displayInfo->dlColormap[
	displayInfo->drawingAreaBackgroundColor];
  values.background = displayInfo->dlColormap[
	displayInfo->drawingAreaBackgroundColor];
  if (displayInfo->pixmapGC == NULL)
     displayInfo->pixmapGC = XCreateGC(display,
	XtWindow(displayInfo->drawingArea),valueMask,&values); 
  else {
     XFreeGC(display,displayInfo->pixmapGC);
     displayInfo->pixmapGC = XCreateGC(display,
	XtWindow(displayInfo->drawingArea),valueMask,&values);
  }
/* (MDA) don't generate GraphicsExpose events on XCopyArea() */
  XSetGraphicsExposures(display,displayInfo->pixmapGC,FALSE);

  XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
	0,0,width,height);
  XSetForeground(display,displayInfo->pixmapGC,
	displayInfo->dlColormap[displayInfo->drawingAreaForegroundColor]);

/* create the initial display GC */
  valueMask = GCForeground | GCBackground ;
  values.foreground = 
	displayInfo->dlColormap[displayInfo->drawingAreaForegroundColor];
  values.background = 
	displayInfo->dlColormap[displayInfo->drawingAreaBackgroundColor];
  if (displayInfo->gc == NULL)
     displayInfo->gc = XCreateGC(display,XtWindow(displayInfo->drawingArea),
	valueMask,&values);
  else {
     XFreeGC(display,displayInfo->gc);
     displayInfo->gc = XCreateGC(display,XtWindow(displayInfo->drawingArea),
	valueMask,&values);
  }

}



void executeDlBasicAttribute(DisplayInfo *displayInfo,
			DlBasicAttribute *dlBasicAttribute, Boolean dummy)
{
  int lineStyle;
  unsigned long gcValueMask;
  XGCValues gcValues;

/* MDA - I think Basic Attributes should turn off Dynamics, since Dynamics
 *	should always immediately precede the object to be made dynamic
 */
  displayInfo->useDynamicAttribute = FALSE;	/* MDA ??? */


/*
 * update the global (current) gc to reflect this attribute data
 * (structure copy), and then batch all XChangeGC's
 */
  displayInfo->attribute = dlBasicAttribute->attr;
 
  gcValueMask = GCForeground | GCBackground | GCLineStyle | GCLineWidth |
			GCCapStyle | GCJoinStyle | GCFillStyle;
  gcValues.foreground = displayInfo->dlColormap[displayInfo->attribute.clr];
  gcValues.background = displayInfo->dlColormap[displayInfo->attribute.clr];

  if (displayInfo->attribute.style == SOLID) lineStyle = LineSolid;
  else if (displayInfo->attribute.style == DASH) lineStyle = LineOnOffDash;
/* handle confused case to avoid X protocol error */
  else lineStyle = LineSolid; 

  gcValues.line_style = lineStyle;
  gcValues.line_width = displayInfo->attribute.width;
  gcValues.cap_style = CapButt;
  gcValues.join_style = JoinRound;

/* F_SOLID and F_OUTLINE supported, but rendered differently (e.g.,
 *  XDrawRectangle vs. XFillRectangle) hence Xlib's FillStyle
 *  always FillSolid
 *
  if (displayInfo->attribute.fill == F_SOLID)  fillStyle = FillSolid;
  else fillStyle = FillSolid;
 */
  gcValues.fill_style = FillSolid;

  XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

}



void executeDlDynamicAttribute(DisplayInfo *displayInfo,
			DlDynamicAttribute *dlDynamicAttribute, Boolean dummy)
{
/* structure copy */
  displayInfo->dynamicAttribute = *dlDynamicAttribute;
  displayInfo->useDynamicAttribute = TRUE;
}





void executeDlRectangle(DisplayInfo *displayInfo, DlRectangle *dlRectangle,
				Boolean forcedDisplayToWindow)
{
  Drawable drawable;
  FillStyle fillStyle;
  unsigned int lineWidth;


  fillStyle = displayInfo->attribute.fill;
  lineWidth = (displayInfo->attribute.width+1)/2;

  if (displayInfo->useDynamicAttribute == FALSE ||
			displayInfo->traversalMode == DL_EDIT) {

/* from the dlRectangle structure, we've got rectangles's dimensions */
  /* always render to window */

    drawable = XtWindow(displayInfo->drawingArea);
    if (fillStyle == F_SOLID)
	XFillRectangle(display,drawable,displayInfo->gc,
	  dlRectangle->object.x,dlRectangle->object.y,
	  dlRectangle->object.width,dlRectangle->object.height);
    else if (fillStyle == F_OUTLINE)
	XDrawRectangle(display,drawable,displayInfo->gc,
	  dlRectangle->object.x + lineWidth,
	  dlRectangle->object.y + lineWidth,
	  dlRectangle->object.width - 2*lineWidth,
	  dlRectangle->object.height - 2*lineWidth);


    if (!forcedDisplayToWindow) {

  /* also render to pixmap if not part of the dynamics stuff and not
   * member of composite */
      drawable = displayInfo->drawingAreaPixmap;
      if (fillStyle == F_SOLID)
	XFillRectangle(display,drawable,displayInfo->gc,
	  dlRectangle->object.x,dlRectangle->object.y,
	  dlRectangle->object.width,dlRectangle->object.height);
      else if (fillStyle == F_OUTLINE)
	XDrawRectangle(display,drawable,displayInfo->gc,
	  dlRectangle->object.x + lineWidth,
	  dlRectangle->object.y + lineWidth,
	  dlRectangle->object.width - 2*lineWidth,
	  dlRectangle->object.height - 2*lineWidth);
    }


  } else {


/* Dynamic Attribute to be used: setup monitor */

    if (displayInfo->traversalMode == DL_EXECUTE) {

      DlAttribute *dlAttr = (DlAttribute *) malloc(sizeof(DlAttribute));
      DlDynamicAttribute *dlDyn = (DlDynamicAttribute *)
	malloc(sizeof(DlDynamicAttribute));
      ChannelAccessMonitorData *channelAccessMonitorData = 
			allocateChannelAccessMonitorData(displayInfo);
      *dlAttr = displayInfo->attribute;
      *dlDyn = displayInfo->dynamicAttribute;
      channelAccessMonitorData->monitorType = DL_Rectangle;
      channelAccessMonitorData->specifics = (XtPointer) dlRectangle;
      channelAccessMonitorData->dlAttr = dlAttr;
      channelAccessMonitorData->dlDyn  = dlDyn;

      SEVCHK(CA_BUILD_AND_CONNECT(dlDyn->attr.param.chan,TYPENOTCONN,0,
        &(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
        NULL),
        "executeDlRectangle: error in CA_BUILD_AND_CONNECT");
      if (channelAccessMonitorData->chid != NULL)
	    ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;
    }
  }

  displayInfo->useDynamicAttribute = FALSE;
}


void executeDlOval(DisplayInfo *displayInfo, DlOval *dlOval,
			Boolean forcedDisplayToWindow)
{
  Drawable drawable;
  FillStyle fillStyle;
  unsigned int lineWidth;

  fillStyle = displayInfo->attribute.fill;
  lineWidth = (displayInfo->attribute.width+1)/2;

  if (displayInfo->useDynamicAttribute == FALSE ||
			displayInfo->traversalMode == DL_EDIT) {

/* 
 * from the DlOval structure, we've got oval's dimensions
 *  for circle: 0 = start angle,  360*64 = complete circle
 */

  /* always render to window */
    drawable = XtWindow(displayInfo->drawingArea);
    if (fillStyle == F_SOLID) XFillArc(display,drawable,displayInfo->gc,
	dlOval->object.x,dlOval->object.y,
	dlOval->object.width,dlOval->object.height,0,360*64);
    else if (fillStyle == F_OUTLINE) XDrawArc(display,drawable,displayInfo->gc,
	dlOval->object.x + lineWidth,
	dlOval->object.y + lineWidth,
	dlOval->object.width - 2*lineWidth,
	dlOval->object.height - 2*lineWidth, 0,360*64);

  /* if not part of dynamics,also  render to pixmap */
    if (!forcedDisplayToWindow) {
       drawable = displayInfo->drawingAreaPixmap;
       if (fillStyle == F_SOLID) XFillArc(display,drawable,displayInfo->gc,
	 dlOval->object.x,dlOval->object.y,
	 dlOval->object.width,dlOval->object.height,0,360*64);
       else if (fillStyle == F_OUTLINE) XDrawArc(display,drawable,
	displayInfo->gc,
	dlOval->object.x + lineWidth,
	dlOval->object.y + lineWidth,
	dlOval->object.width - 2*lineWidth,
	dlOval->object.height - 2*lineWidth, 0,360*64);
    }

  } else {

/* Dynamic Attribute to be used: setup monitor */

    if (displayInfo->traversalMode == DL_EXECUTE) {

      DlAttribute *dlAttr = (DlAttribute *) malloc(sizeof(DlAttribute));
      DlDynamicAttribute *dlDyn = (DlDynamicAttribute *)
	malloc(sizeof(DlDynamicAttribute));
      ChannelAccessMonitorData *channelAccessMonitorData = 
			allocateChannelAccessMonitorData(displayInfo);
      *dlAttr = displayInfo->attribute;
      *dlDyn = displayInfo->dynamicAttribute;
      channelAccessMonitorData->monitorType = DL_Oval;
      channelAccessMonitorData->specifics = (XtPointer) dlOval;
      channelAccessMonitorData->dlAttr = dlAttr;
      channelAccessMonitorData->dlDyn  = dlDyn;

      SEVCHK(CA_BUILD_AND_CONNECT(dlDyn->attr.param.chan,TYPENOTCONN,0,
        &(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
        NULL),
        "executeDlOval: error in CA_BUILD_AND_CONNECT");
      if (channelAccessMonitorData->chid != NULL)
	    ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;
    }
  }

  displayInfo->useDynamicAttribute = FALSE;

}


void executeDlArc(DisplayInfo *displayInfo, DlArc*dlArc,
			Boolean forcedDisplayToWindow)
{
  Drawable drawable;
  FillStyle fillStyle;
  unsigned int lineWidth;

  fillStyle = displayInfo->attribute.fill;
  lineWidth = (displayInfo->attribute.width+1)/2;

  if (displayInfo->useDynamicAttribute == FALSE || 
		displayInfo->traversalMode == DL_EDIT) {
/* 
 * from the DlArc structure, we've got arc's dimensions
 */
 /* always render to window */
    drawable = XtWindow(displayInfo->drawingArea);
    if (fillStyle == F_SOLID) XFillArc(display,drawable,displayInfo->gc,
	dlArc->object.x,dlArc->object.y,
	dlArc->object.width,dlArc->object.height,dlArc->begin,dlArc->path);
    else if (fillStyle == F_OUTLINE) XDrawArc(display,drawable,displayInfo->gc,
	dlArc->object.x + lineWidth,
	dlArc->object.y + lineWidth,
	dlArc->object.width - 2*lineWidth,
	dlArc->object.height - 2*lineWidth,dlArc->begin,dlArc->path);

  /* if not associated with dynamic stuff, also render to pixmap */
    if (!forcedDisplayToWindow) {
      drawable = displayInfo->drawingAreaPixmap;
      if (fillStyle == F_SOLID) XFillArc(display,drawable,displayInfo->gc,
	dlArc->object.x,dlArc->object.y,
	dlArc->object.width,dlArc->object.height,dlArc->begin,dlArc->path);
      else if (fillStyle == F_OUTLINE) XDrawArc(display,drawable,
	displayInfo->gc,
	dlArc->object.x + lineWidth,
	dlArc->object.y + lineWidth,
	dlArc->object.width - 2*lineWidth,
	dlArc->object.height - 2*lineWidth,dlArc->begin,dlArc->path);
    }

  } else {

/* Dynamic Attribute to be used: setup monitor */

    if (displayInfo->traversalMode == DL_EXECUTE) {

      DlAttribute *dlAttr = (DlAttribute *) malloc(sizeof(DlAttribute));
      DlDynamicAttribute *dlDyn = (DlDynamicAttribute *)
	malloc(sizeof(DlDynamicAttribute));
      ChannelAccessMonitorData *channelAccessMonitorData = 
			allocateChannelAccessMonitorData(displayInfo);
      *dlAttr = displayInfo->attribute;
      *dlDyn = displayInfo->dynamicAttribute;
      channelAccessMonitorData->monitorType = DL_Arc;
      channelAccessMonitorData->specifics = (XtPointer) dlArc;
      channelAccessMonitorData->dlAttr = dlAttr;
      channelAccessMonitorData->dlDyn  = dlDyn;

      SEVCHK(CA_BUILD_AND_CONNECT(dlDyn->attr.param.chan,TYPENOTCONN,0,
        &(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
        NULL),
        "executeDlArc: error in CA_BUILD_AND_CONNECT");
      if (channelAccessMonitorData->chid != NULL)
	    ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;
    }
  }

  displayInfo->useDynamicAttribute = FALSE;
}



void executeDlText(DisplayInfo *displayInfo, DlText *dlText,
			Boolean forcedDisplayToWindow)
{
  Drawable drawable;
  int i = 0, usedWidth, usedHeight;
  size_t nChars;


  if (displayInfo->useDynamicAttribute == FALSE || 
		displayInfo->traversalMode == DL_EDIT) {

    nChars = strlen(dlText->textix);
    i = dmGetBestFontWithInfo(fontTable,MAX_FONTS,dlText->textix,
	dlText->object.height,dlText->object.width,&usedHeight,&usedWidth,
	FALSE);
    usedWidth = XTextWidth(fontTable[i],dlText->textix,nChars);


    XSetFont(display,displayInfo->gc,fontTable[i]->fid);

  /* always render to window */
    drawable = XtWindow(displayInfo->drawingArea);
    switch (dlText->align) {
      case HORIZ_LEFT:
      case VERT_TOP:
	XDrawString(display,drawable,displayInfo->gc,
		    dlText->object.x,dlText->object.y + fontTable[i]->ascent,
		    dlText->textix,nChars);
	break;
      case HORIZ_CENTER:
      case VERT_CENTER:
	XDrawString(display,drawable,displayInfo->gc,
		    dlText->object.x + (dlText->object.width - usedWidth)/2, 
		    dlText->object.y + fontTable[i]->ascent,
		    dlText->textix,nChars);
	break;
      case HORIZ_RIGHT:
      case VERT_BOTTOM:
	XDrawString(display,drawable,displayInfo->gc,
		    dlText->object.x + dlText->object.width - usedWidth,
		    dlText->object.y + fontTable[i]->ascent, 
		    dlText->textix,nChars);
	break;
    }

  /* if not associated with dynamics, also render to pixmap */
    if (!forcedDisplayToWindow) {

     drawable = displayInfo->drawingAreaPixmap;
     switch (dlText->align) {
       case HORIZ_LEFT:
	 XDrawString(display,drawable,displayInfo->gc,
		    dlText->object.x,dlText->object.y + fontTable[i]->ascent,
		    dlText->textix,nChars);
	 break;
       case HORIZ_CENTER:
	 XDrawString(display,drawable,displayInfo->gc,
		    dlText->object.x + (dlText->object.width - usedWidth)/2, 
		    dlText->object.y + fontTable[i]->ascent,
		    dlText->textix,nChars);
	 break;
       case HORIZ_RIGHT:
	 XDrawString(display,drawable,displayInfo->gc,
		    dlText->object.x + dlText->object.width - usedWidth,
		    dlText->object.y + fontTable[i]->ascent, 
		    dlText->textix,nChars);
	 break;
      case VERT_TOP:
	 XDrawString(display,drawable,displayInfo->gc,
		    dlText->object.x,dlText->object.y + fontTable[i]->ascent,
		    dlText->textix,nChars);
	 break;
      case VERT_CENTER:
	 XDrawString(display,drawable,displayInfo->gc,
		    dlText->object.x + (dlText->object.width - usedWidth)/2, 
		    dlText->object.y + fontTable[i]->ascent,
		    dlText->textix,nChars);
	 break;
      case VERT_BOTTOM:
	 XDrawString(display,drawable,displayInfo->gc,
		    dlText->object.x + dlText->object.width - usedWidth,
		    dlText->object.y + fontTable[i]->ascent, 
		    dlText->textix,nChars);
	 break;
     }
    }


  } else {

/* Dynamic Attribute to be used: setup monitor */

    if (displayInfo->traversalMode == DL_EXECUTE) {

      DlAttribute *dlAttr = (DlAttribute *) malloc(sizeof(DlAttribute));
      DlDynamicAttribute *dlDyn = (DlDynamicAttribute *)
	malloc(sizeof(DlDynamicAttribute));
      ChannelAccessMonitorData *channelAccessMonitorData = 
			allocateChannelAccessMonitorData(displayInfo);
      *dlAttr = displayInfo->attribute;
      *dlDyn = displayInfo->dynamicAttribute;
      channelAccessMonitorData->monitorType = DL_Text;
      channelAccessMonitorData->specifics = (XtPointer) dlText;
      channelAccessMonitorData->dlAttr = dlAttr;
      channelAccessMonitorData->dlDyn  = dlDyn;

      SEVCHK(CA_BUILD_AND_CONNECT(dlDyn->attr.param.chan,TYPENOTCONN,0,
        &(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
        NULL),
        "executeDlText: error in CA_BUILD_AND_CONNECT");
      if (channelAccessMonitorData->chid != NULL)
	    ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;
    }
  }

  displayInfo->useDynamicAttribute = FALSE;
}


void executeDlFallingLine(DisplayInfo *displayInfo,
	DlFallingLine *dlFallingLine, Boolean forcedDisplayToWindow)
{
  Drawable drawable;
  unsigned int lineWidth;

  lineWidth = displayInfo->attribute.width/2;

  if (displayInfo->useDynamicAttribute == FALSE || 
			displayInfo->traversalMode == DL_EDIT) {
/* 
 * from the DlFallingLine structure, we've got line's dimensions
 */

 /* always render to window */
    drawable = XtWindow(displayInfo->drawingArea);
    XDrawLine(display,drawable,displayInfo->gc,
	dlFallingLine->object.x + lineWidth,
	dlFallingLine->object.y + lineWidth,
	dlFallingLine->object.x + dlFallingLine->object.width - lineWidth,
	dlFallingLine->object.y + dlFallingLine->object.height - lineWidth);

    if (!forcedDisplayToWindow) {
 /* also render to pixmap */
      drawable = displayInfo->drawingAreaPixmap;
      XDrawLine(display,drawable,displayInfo->gc,
	dlFallingLine->object.x + lineWidth,
	dlFallingLine->object.y + lineWidth,
	dlFallingLine->object.x + dlFallingLine->object.width - lineWidth,
	dlFallingLine->object.y + dlFallingLine->object.height - lineWidth); 
    }

  } else {

/* Dynamic Attribute to be used: setup monitor */

    if (displayInfo->traversalMode == DL_EXECUTE) {

      DlAttribute *dlAttr = (DlAttribute *) malloc(sizeof(DlAttribute));
      DlDynamicAttribute *dlDyn = (DlDynamicAttribute *)
	malloc(sizeof(DlDynamicAttribute));
      ChannelAccessMonitorData *channelAccessMonitorData = 
			allocateChannelAccessMonitorData(displayInfo);
      *dlAttr = displayInfo->attribute;
      *dlDyn = displayInfo->dynamicAttribute;
      channelAccessMonitorData->monitorType = DL_FallingLine;
      channelAccessMonitorData->specifics = (XtPointer) dlFallingLine;
      channelAccessMonitorData->dlAttr = dlAttr;
      channelAccessMonitorData->dlDyn  = dlDyn;

      SEVCHK(CA_BUILD_AND_CONNECT(dlDyn->attr.param.chan,TYPENOTCONN,0,
        &(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
        NULL),
        "executeDlFallingLine: error in CA_BUILD_AND_CONNECT");
      if (channelAccessMonitorData->chid != NULL)
	    ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;
    }
  }

  displayInfo->useDynamicAttribute = FALSE;
}


void executeDlRisingLine(DisplayInfo *displayInfo,
		DlRisingLine *dlRisingLine, Boolean forcedDisplayToWindow)
{
  Drawable drawable;
  unsigned int lineWidth;

  lineWidth = displayInfo->attribute.width/2;

  if (displayInfo->useDynamicAttribute == FALSE || 
			displayInfo->traversalMode == DL_EDIT) {

/* 
 * from the DlRisingLine structure, we've got line's dimensions
 */
 /* always render to window */
    drawable = XtWindow(displayInfo->drawingArea);
    XDrawLine(display,drawable,displayInfo->gc,
	dlRisingLine->object.x + lineWidth,
	dlRisingLine->object.y + dlRisingLine->object.height 
		- displayInfo->attribute.width,
	dlRisingLine->object.x + dlRisingLine->object.width 
		- displayInfo->attribute.width,
	dlRisingLine->object.y - lineWidth);

    if (!forcedDisplayToWindow) {
  /* if not related to dynamics, also render to pixmap */
      drawable = displayInfo->drawingAreaPixmap;
      XDrawLine(display,drawable,displayInfo->gc,
	dlRisingLine->object.x + lineWidth,  
	dlRisingLine->object.y + dlRisingLine->object.height 
		- displayInfo->attribute.width,
	dlRisingLine->object.x + dlRisingLine->object.width 
		- displayInfo->attribute.width,
	dlRisingLine->object.y - lineWidth);
    }

  } else {

/* Dynamic Attribute to be used: setup monitor */

    if (displayInfo->traversalMode == DL_EXECUTE) {

      DlAttribute *dlAttr = (DlAttribute *) malloc(sizeof(DlAttribute));
      DlDynamicAttribute *dlDyn = (DlDynamicAttribute *)
	malloc(sizeof(DlDynamicAttribute));
      ChannelAccessMonitorData *channelAccessMonitorData = 
			allocateChannelAccessMonitorData(displayInfo);
      *dlAttr = displayInfo->attribute;
      *dlDyn = displayInfo->dynamicAttribute;
      channelAccessMonitorData->monitorType = DL_RisingLine;
      channelAccessMonitorData->specifics = (XtPointer) dlRisingLine;
      channelAccessMonitorData->dlAttr = dlAttr;
      channelAccessMonitorData->dlDyn  = dlDyn;

      SEVCHK(CA_BUILD_AND_CONNECT(dlDyn->attr.param.chan,TYPENOTCONN,0,
        &(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
        NULL),
        "executeDlRisingLine: error in CA_BUILD_AND_CONNECT");
      if (channelAccessMonitorData->chid != NULL)
	    ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;
    }
  }

  displayInfo->useDynamicAttribute = FALSE;
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




void executeDlRelatedDisplay(DisplayInfo *displayInfo,
			DlRelatedDisplay *dlRelatedDisplay, Boolean dummy)
{
  Widget localMenuBar, tearOff;
  Arg args[20];
  int i, n, displayNumber=0;
  char *name, *argsString;
  char **nameArgs;
  XmString xmString;
  Pixmap relatedDisplayPixmap;
  unsigned int pixmapSize;

/* 
 * these are widget ids, but they are recorded in the otherChild widget list
 *   as well, for destruction when new displays are selected at the top level
 */
  Widget relatedDisplayPulldownMenu, relatedDisplayMenuButton;


  displayInfo->useDynamicAttribute = FALSE;

/*** 
 *** from the DlRelatedDisplay structure, we've got specifics
 *** (MDA)  create a pulldown menu with the following related display menu
 ***   entries in it...  --  careful with the XtSetArgs here (special)
 ***/
  XtSetArg(args[0],XmNbackground,(Pixel)
	displayInfo->dlColormap[dlRelatedDisplay->bclr]);
  XtSetArg(args[1],XmNforeground,(Pixel)
	displayInfo->dlColormap[dlRelatedDisplay->clr]);
  XtSetArg(args[2],XmNhighlightThickness,1);
  XtSetArg(args[3],XmNwidth,dlRelatedDisplay->object.width);
  XtSetArg(args[4],XmNheight,dlRelatedDisplay->object.height);
  XtSetArg(args[5],XmNmarginHeight,0);
  XtSetArg(args[6],XmNmarginWidth,0);
  XtSetArg(args[7],XmNresizeHeight,(Boolean)FALSE);
  XtSetArg(args[8],XmNresizeWidth,(Boolean)FALSE);
  XtSetArg(args[9],XmNspacing,0);
  XtSetArg(args[10],XmNx,(Position)dlRelatedDisplay->object.x);
  XtSetArg(args[11],XmNy,(Position)dlRelatedDisplay->object.y);
  XtSetArg(args[12],XmNhighlightOnEnter,TRUE);
  localMenuBar = 
     XmCreateMenuBar(displayInfo->drawingArea,"relatedDisplayMenuBar",args,13);
  XtManageChild(localMenuBar);
  displayInfo->child[displayInfo->childCount++] = localMenuBar;

  colorMenuBar(localMenuBar,
	(Pixel)displayInfo->dlColormap[dlRelatedDisplay->clr],
	(Pixel)displayInfo->dlColormap[dlRelatedDisplay->bclr]);

  relatedDisplayPulldownMenu = XmCreatePulldownMenu(
	displayInfo->child[displayInfo->childCount-1],
		"relatedDisplayPulldownMenu",args,2);
  displayInfo->otherChild[displayInfo->otherChildCount++] = 
	relatedDisplayPulldownMenu;

  tearOff = XmGetTearOffControl(relatedDisplayPulldownMenu);
  if (tearOff != NULL) {
  /* sensitive to ordering of Args above - want background as args[0] */
    XtSetValues(tearOff,args,1);
  }

  pixmapSize = MIN(dlRelatedDisplay->object.width,
			dlRelatedDisplay->object.height);
/* allowing for shadows etc */
  pixmapSize = (unsigned int) MAX(1,(int)pixmapSize - 8);

/* create relatedDisplay icon (render to appropriate size) */
  relatedDisplayPixmap = XCreatePixmap(display,RootWindow(display,screenNum),
	pixmapSize, pixmapSize, XDefaultDepth(display,screenNum));
  renderRelatedDisplayPixmap(display,relatedDisplayPixmap,
	displayInfo->dlColormap[dlRelatedDisplay->clr],
	displayInfo->dlColormap[dlRelatedDisplay->bclr],
	pixmapSize, pixmapSize);

  XtSetArg(args[7],XmNrecomputeSize,(Boolean)False);
  XtSetArg(args[8],XmNlabelPixmap,relatedDisplayPixmap); 
  XtSetArg(args[9],XmNlabelType,XmPIXMAP); 
  XtSetArg(args[10],XmNsubMenuId,relatedDisplayPulldownMenu);
  XtSetArg(args[11],XmNhighlightOnEnter,TRUE);
  XtSetArg(args[12],XmNalignment,XmALIGNMENT_BEGINNING);

  XtSetArg(args[13],XmNmarginLeft,0);
  XtSetArg(args[14],XmNmarginRight,0);
  XtSetArg(args[15],XmNmarginTop,0);
  XtSetArg(args[16],XmNmarginBottom,0);
  XtSetArg(args[17],XmNmarginWidth,0);
  XtSetArg(args[18],XmNmarginHeight,0);

  displayInfo->otherChild[displayInfo->otherChildCount++] = 
	XtCreateManagedWidget("relatedDisplayMenuLabel",
		xmCascadeButtonGadgetClass, 
		displayInfo->child[displayInfo->childCount-1], args, 19);

/* add destroy callback to free pixmap from pixmap cache */
  XtAddCallback(displayInfo->otherChild[displayInfo->otherChildCount-1],
	XmNdestroyCallback,(XtCallbackProc)freePixmapCallback,
	(XtPointer)relatedDisplayPixmap);

  for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
     if (strlen(dlRelatedDisplay->display[i].name) > 1) {
	xmString = XmStringCreateSimple(dlRelatedDisplay->display[i].label);
        XtSetArg(args[3], XmNlabelString,xmString);
	name = STRDUP(dlRelatedDisplay->display[i].name);
	argsString = STRDUP(dlRelatedDisplay->display[i].args);
	nameArgs = (char **)malloc(
		RELATED_DISPLAY_FILENAME_AND_ARGS_SIZE*sizeof(char *));
	nameArgs[RELATED_DISPLAY_FILENAME_INDEX] = name;
	nameArgs[RELATED_DISPLAY_ARGS_INDEX] = argsString;
	XtSetArg(args[4], XmNuserData, nameArgs);
        relatedDisplayMenuButton = XtCreateManagedWidget("relatedButton",
		xmPushButtonWidgetClass, relatedDisplayPulldownMenu, args, 5);
	displayInfo->otherChild[displayInfo->otherChildCount++] = 
		relatedDisplayMenuButton;
	XtAddCallback(relatedDisplayMenuButton,XmNactivateCallback,
		(XtCallbackProc)dmCreateRelatedDisplay,(XtPointer)displayInfo);
	XtAddCallback(relatedDisplayMenuButton,XmNdestroyCallback,
		(XtCallbackProc)relatedDisplayMenuButtonDestroy,
		(XtPointer)nameArgs);
	XmStringFree(xmString);
     }
  }


/* add event handlers to relatedDisplay... */
  if (displayInfo->traversalMode == DL_EDIT) {

/* remove all translations if in edit mode */
    XtUninstallTranslations(localMenuBar);

    XtAddEventHandler(localMenuBar,ButtonPressMask,False,
		(XtEventHandler)handleButtonPress, (XtPointer)displayInfo);
  }

}



/*
 * local function to render the related display icon into a pixmap
 */
static void renderShellCommandPixmap(Display *display, Pixmap pixmap,
	Pixel fg, Pixel bg, Dimension width, Dimension height)
{
  typedef struct { float x; float y;} XY;
/* icon is based on the 25 pixel (w & h) bitmap shellCommand25 */
  static float rectangleX = 12./25., rectangleY = 4./25.,
	rectangleWidth = 3./25., rectangleHeight = 14./25.;
  static float dotX = 12./25., dotY = 20./25.,
	dotWidth = 3./25., dotHeight = 3./25.;
  GC gc;

  gc = XCreateGC(display,pixmap,0,NULL);
  XSetForeground(display,gc,bg);
  XFillRectangle(display,pixmap,gc,0,0,width,height);
  XSetForeground(display,gc,fg);
  
  XFillRectangle(display,pixmap,gc,
	(int)(rectangleX*width),
	(int)(rectangleY*height),
	(unsigned int)(MAX(1,(unsigned int)(rectangleWidth*width))),
	(unsigned int)(MAX(1,(unsigned int)(rectangleHeight*height))) );
	
  XFillRectangle(display,pixmap,gc,
	(int)(dotX*width),
	(int)(dotY*height),
	(unsigned int)(MAX(1,(unsigned int)(dotWidth*width))),
	(unsigned int)(MAX(1,(unsigned int)(dotHeight*height))) );

  XFreeGC(display,gc);
}



void executeDlShellCommand(DisplayInfo *displayInfo,
			DlShellCommand *dlShellCommand, Boolean dummy)
{
  Widget localMenuBar;
  Arg args[16];
  int i, n, shellNumber=0;
  char *command;
  XmString xmString;
  Pixmap shellCommandPixmap;
  unsigned int pixmapSize;
/* 
 * these are widget ids, but they are recorded in the otherChild widget list
 *   as well, for destruction when new shells are selected at the top level
 */
  Widget shellCommandPulldownMenu, shellCommandMenuButton;


  displayInfo->useDynamicAttribute = FALSE;

/*** 
 *** from the DlShellCommand structure, we've got specifics
 *** (MDA)  create a pulldown menu with the following related shell menu
 ***   entries in it...  --  careful with the XtSetArgs here (special)
 ***/
  XtSetArg(args[0],XmNforeground,(Pixel)
	displayInfo->dlColormap[dlShellCommand->clr]);
  XtSetArg(args[1],XmNbackground,(Pixel)
	displayInfo->dlColormap[dlShellCommand->bclr]);
  XtSetArg(args[2],XmNhighlightThickness,1);
  XtSetArg(args[3],XmNwidth,dlShellCommand->object.width);
  XtSetArg(args[4],XmNheight,dlShellCommand->object.height);
  XtSetArg(args[5],XmNmarginHeight,0);
  XtSetArg(args[6],XmNmarginWidth,0);
  XtSetArg(args[7],XmNresizeHeight,(Boolean)FALSE);
  XtSetArg(args[8],XmNresizeWidth,(Boolean)FALSE);
  XtSetArg(args[9],XmNspacing,0);
  XtSetArg(args[10],XmNx,(Position)dlShellCommand->object.x);
  XtSetArg(args[11],XmNy,(Position)dlShellCommand->object.y);
  XtSetArg(args[12],XmNhighlightOnEnter,TRUE);
  localMenuBar = 
     XmCreateMenuBar(displayInfo->drawingArea,"shellCommandMenuBar",args,13);
  XtManageChild(localMenuBar);
  displayInfo->child[displayInfo->childCount++] = localMenuBar;

  colorMenuBar(localMenuBar,
	(Pixel)displayInfo->dlColormap[dlShellCommand->clr],
	(Pixel)displayInfo->dlColormap[dlShellCommand->bclr]);

  shellCommandPulldownMenu = XmCreatePulldownMenu(
	displayInfo->child[displayInfo->childCount-1],
		"shellCommandPulldownMenu",args,2);
  displayInfo->otherChild[displayInfo->otherChildCount++] = 
	shellCommandPulldownMenu;

  pixmapSize = MIN(dlShellCommand->object.width,dlShellCommand->object.height);
/* allowing for shadows etc */
  pixmapSize = (unsigned int) MAX(1,(int)pixmapSize - 8);

/* create shellCommand icon (render to appropriate size) */
  shellCommandPixmap = XCreatePixmap(display,RootWindow(display,screenNum),
	pixmapSize,pixmapSize,XDefaultDepth(display,screenNum));
  renderShellCommandPixmap(display,shellCommandPixmap,
	displayInfo->dlColormap[dlShellCommand->clr],
	displayInfo->dlColormap[dlShellCommand->bclr],
	pixmapSize,pixmapSize);

  XtSetArg(args[7],XmNrecomputeSize,(Boolean)False);
  XtSetArg(args[8],XmNlabelPixmap,shellCommandPixmap); 
  XtSetArg(args[9],XmNlabelType,XmPIXMAP); 
  XtSetArg(args[10],XmNsubMenuId,shellCommandPulldownMenu);
  XtSetArg(args[11],XmNhighlightOnEnter,TRUE);
  displayInfo->otherChild[displayInfo->otherChildCount++] = 
	XtCreateManagedWidget("shellCommandMenuLabel",
		xmCascadeButtonGadgetClass, 
		displayInfo->child[displayInfo->childCount-1], args, 12);

/* add destroy callback to free pixmap from pixmap cache */
  XtAddCallback(displayInfo->otherChild[displayInfo->otherChildCount-1],
	XmNdestroyCallback,(XtCallbackProc)freePixmapCallback,
	(XtPointer)shellCommandPixmap);

  for (i = 0; i < MAX_SHELL_COMMANDS; i++) {
     if (strlen(dlShellCommand->command[i].command) > 0) {
	xmString = XmStringCreateSimple(dlShellCommand->command[i].label);
        XtSetArg(args[3], XmNlabelString,xmString);
	/* set the displayInfo as the button's userData */
        XtSetArg(args[4], XmNuserData,(XtPointer)displayInfo);
        shellCommandMenuButton = XtCreateManagedWidget("relatedButton",
		xmPushButtonWidgetClass, shellCommandPulldownMenu, args, 5);
	displayInfo->otherChild[displayInfo->otherChildCount++] = 
		shellCommandMenuButton;
	XtAddCallback(shellCommandMenuButton,XmNactivateCallback,
		(XtCallbackProc)dmExecuteShellCommand,
		(XtPointer)&(dlShellCommand->command[i]));
	XmStringFree(xmString);
     }
  }


/* add event handlers to shellCommand... */
  if (displayInfo->traversalMode == DL_EDIT) {

/* remove all translations if in edit mode */
    XtUninstallTranslations(localMenuBar);

    XtAddEventHandler(localMenuBar,ButtonPressMask,False,
		(XtEventHandler)handleButtonPress, (XtPointer)displayInfo);
  }

}

