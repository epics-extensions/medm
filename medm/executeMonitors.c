

#include "medm.h"

#include <Xm/DrawingAP.h>

#include <X11/keysym.h>

char *stripChartWidgetName = "stripChart";


ChannelAccessMonitorData *allocateChannelAccessMonitorData(
  DisplayInfo *displayInfo)
{
  ChannelAccessMonitorData *channelAccessMonitorData = 
	(ChannelAccessMonitorData *) malloc(sizeof(ChannelAccessMonitorData));
  channelAccessMonitorData->modified = NOT_MODIFIED;
  channelAccessMonitorData->previouslyConnected = FALSE;
  channelAccessMonitorData->chid = NULL;
  channelAccessMonitorData->evid = NULL;
  channelAccessMonitorData->self = NULL;
  channelAccessMonitorData->value = 0.0;
  channelAccessMonitorData->oldValue = 0.0;
  channelAccessMonitorData->displayedValue = 0.0;
  strcpy(channelAccessMonitorData->stringValue," ");
  strcpy(channelAccessMonitorData->oldStringValue," ");
  channelAccessMonitorData->displayed = FALSE;
  channelAccessMonitorData->numberStateStrings = 0;
  channelAccessMonitorData->stateStrings = NULL;
  channelAccessMonitorData->hopr = 0.0;
  channelAccessMonitorData->lopr = 0.0;
  channelAccessMonitorData->precision = 0;
  channelAccessMonitorData->oldIntegerValue = 0;
  channelAccessMonitorData->status = 0;
  channelAccessMonitorData->severity = NO_ALARM;
  channelAccessMonitorData->oldSeverity = NO_ALARM;
  channelAccessMonitorData->next = NULL;
  channelAccessMonitorData->prev = channelAccessMonitorListTail;
  channelAccessMonitorData->displayInfo = displayInfo;
  channelAccessMonitorData->clrmod = STATIC;
  channelAccessMonitorData->label = LABEL_NONE;
  channelAccessMonitorData->fontIndex = 0;
  channelAccessMonitorData->dlAttr = NULL;
  channelAccessMonitorData->dlDyn  = NULL;
  channelAccessMonitorData->xrtData = NULL;
  channelAccessMonitorData->xrtDataSet = 1;
  channelAccessMonitorData->trace = 0;
  channelAccessMonitorData->xyChannelType = CP_XYScalarX;
  channelAccessMonitorData->other = NULL;
  channelAccessMonitorData->controllerData = NULL;

/* add this ca monitor data node into monitor list */
  channelAccessMonitorListTail->next = channelAccessMonitorData;
  channelAccessMonitorListTail= channelAccessMonitorData;

  return (channelAccessMonitorData);
}




/***
 *** Meter
 ***/



void executeDlMeter(DisplayInfo *displayInfo, DlMeter *dlMeter, Boolean dummy)
{
  ChannelAccessMonitorData *channelAccessMonitorData;
  Arg args[24];
  int n;
  int usedHeight, usedCharWidth, bestSize, preferredHeight;
  Widget localWidget;

  displayInfo->useDynamicAttribute = FALSE;

  if (displayInfo->traversalMode == DL_EXECUTE) {

    channelAccessMonitorData = allocateChannelAccessMonitorData(displayInfo);
    channelAccessMonitorData->monitorType = DL_Meter;
    channelAccessMonitorData->specifics = (XtPointer) dlMeter;
    channelAccessMonitorData->clrmod = dlMeter->clrmod;
    channelAccessMonitorData->label = dlMeter->label;

/* put up white rectangle so that unconnected channels are obvious */
    XSetForeground(display,displayInfo->pixmapGC,WhitePixel(display,screenNum));
    XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
	dlMeter->object.x,dlMeter->object.y,
	dlMeter->object.width,dlMeter->object.height);
    XSetForeground(display,displayInfo->gc,WhitePixel(display,screenNum));
    XFillRectangle(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	dlMeter->object.x,dlMeter->object.y,
	dlMeter->object.width,dlMeter->object.height);

    SEVCHK(CA_BUILD_AND_CONNECT(dlMeter->monitor.rdbk,TYPENOTCONN,0,
	&(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
	NULL), 
	"executeDlMeter: error in CA_BUILD_AND_CONNECT");
    if (channelAccessMonitorData->chid != NULL)
	ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;
  }

/* from the meter structure, we've got Meter's specifics */
  n = 0;
  XtSetArg(args[n],XtNx,(Position)dlMeter->object.x); n++;
  XtSetArg(args[n],XtNy,(Position)dlMeter->object.y); n++;
  XtSetArg(args[n],XtNwidth,(Dimension)dlMeter->object.width); n++;
  XtSetArg(args[n],XtNheight,(Dimension)dlMeter->object.height); n++;
  XtSetArg(args[n],XcNdataType,XcFval); n++;
  XtSetArg(args[n],XcNscaleSegments,
		(dlMeter->object.width > METER_OKAY_SIZE ? 11 : 5) ); n++;
  switch (dlMeter->label) {
     case LABEL_NONE:
	XtSetArg(args[n],XcNvalueVisible,FALSE); n++;
	XtSetArg(args[n],XcNlabel," "); n++;
	break;
     case OUTLINE:
	XtSetArg(args[n],XcNvalueVisible,FALSE); n++;
	XtSetArg(args[n],XcNlabel," "); n++;
	break;
     case LIMITS:
	XtSetArg(args[n],XcNvalueVisible,TRUE); n++;
	XtSetArg(args[n],XcNlabel," "); n++;
	break;
     case CHANNEL:
	XtSetArg(args[n],XcNvalueVisible,TRUE); n++;
	XtSetArg(args[n],XcNlabel,dlMeter->monitor.rdbk); n++;
	break;
  }
  preferredHeight = dlMeter->object.height/METER_FONT_DIVISOR;
  bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
	preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
  XtSetArg(args[n],XtNfont,fontTable[bestSize]); n++;
  XtSetArg(args[n],XcNmeterForeground,(Pixel)
	displayInfo->dlColormap[dlMeter->monitor.clr]); n++;
  XtSetArg(args[n],XcNmeterBackground,(Pixel)
	displayInfo->dlColormap[dlMeter->monitor.bclr]); n++;
  XtSetArg(args[n],XtNbackground,(Pixel)
	displayInfo->dlColormap[dlMeter->monitor.bclr]); n++;
  XtSetArg(args[n],XcNcontrolBackground,(Pixel)
	displayInfo->dlColormap[dlMeter->monitor.bclr]); n++;
/*
 * add the pointer to the ChannelAccessMonitorData structure as userData 
 *  to widget
 */
  XtSetArg(args[n],XcNuserData,(XtPointer)channelAccessMonitorData); n++;
  localWidget = XtCreateWidget("meter", 
		xcMeterWidgetClass, displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] = localWidget;

  if (displayInfo->traversalMode == DL_EXECUTE) {

/* record the widget that this structure belongs to */
    channelAccessMonitorData->self = localWidget;

/* add in drag/drop translations */
    XtOverrideTranslations(localWidget,parsedTranslations);


  } else if (displayInfo->traversalMode == DL_EDIT) {

/* add button press handlers */
    XtAddEventHandler(localWidget,ButtonPressMask,False,
		(XtEventHandler)handleButtonPress,(XtPointer)displayInfo);

    XtManageChild(localWidget);
  }

}



/***
 *** Bar
 ***/


void executeDlBar(DisplayInfo *displayInfo, DlBar *dlBar, Boolean dummy)
{
  ChannelAccessMonitorData *channelAccessMonitorData;
  Arg args[30];
  int n;
  int usedHeight, usedCharWidth, bestSize, preferredHeight;
  Widget localWidget;


  displayInfo->useDynamicAttribute = FALSE;

  if (displayInfo->traversalMode == DL_EXECUTE) {
    channelAccessMonitorData = allocateChannelAccessMonitorData(displayInfo);
    channelAccessMonitorData->monitorType = DL_Bar;
    channelAccessMonitorData->specifics = (XtPointer) dlBar;
    channelAccessMonitorData->clrmod = dlBar->clrmod;
    channelAccessMonitorData->label = dlBar->label;

/* put up white rectangle so that unconnected channels are obvious */
    XSetForeground(display,displayInfo->pixmapGC,WhitePixel(display,screenNum));
    XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
	dlBar->object.x,dlBar->object.y,
	dlBar->object.width,dlBar->object.height);
    XSetForeground(display,displayInfo->gc,WhitePixel(display,screenNum));
    XFillRectangle(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	dlBar->object.x,dlBar->object.y,
	dlBar->object.width,dlBar->object.height);

    SEVCHK(CA_BUILD_AND_CONNECT(dlBar->monitor.rdbk,TYPENOTCONN,0,
	&(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
	NULL), 
	"executeDlBar: error in CA_BUILD_AND_CONNECT");
    if (channelAccessMonitorData->chid != NULL)
	ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;
  }

/* from the bar structure, we've got Bar's specifics */
  n = 0;
  XtSetArg(args[n],XtNx,(Position)dlBar->object.x); n++;
  XtSetArg(args[n],XtNy,(Position)dlBar->object.y); n++;
  XtSetArg(args[n],XtNwidth,(Dimension)dlBar->object.width); n++;
  XtSetArg(args[n],XtNheight,(Dimension)dlBar->object.height); n++;
  XtSetArg(args[n],XcNdataType,XcFval); n++;
  switch (dlBar->label) {
     case LABEL_NONE:
	XtSetArg(args[n],XcNvalueVisible,FALSE); n++;
	XtSetArg(args[n],XcNlabel," "); n++;
	break;
     case OUTLINE:
	XtSetArg(args[n],XcNvalueVisible,FALSE); n++;
	XtSetArg(args[n],XcNlabel," "); n++;
	break;
     case LIMITS:
	XtSetArg(args[n],XcNvalueVisible,TRUE); n++;
	XtSetArg(args[n],XcNlabel," "); n++;
	break;
     case CHANNEL:
	XtSetArg(args[n],XcNvalueVisible,TRUE); n++;
	XtSetArg(args[n],XcNlabel,dlBar->monitor.rdbk); n++;
	break;
  }
  switch (dlBar->direction) {
/*
 * note that this is  "direction of increase" for Bar
 */
     case LEFT:
	fprintf(stderr,"\nexecuteDlBar: LEFT direction BARS not supported");
     case RIGHT:
	XtSetArg(args[n],XcNorient,XcHoriz); n++;
	XtSetArg(args[n],XcNscaleSegments,
		(dlBar->object.width>INDICATOR_OKAY_SIZE ? 11 : 5)); n++;
	if (dlBar->label == LABEL_NONE) {
		XtSetArg(args[n],XcNscaleSegments, 0); n++;
	}
	break;

     case DOWN:
	fprintf(stderr,"\nexecuteDlBar: DOWN direction BARS not supported");
     case UP:
	XtSetArg(args[n],XcNorient,XcVert); n++;
	XtSetArg(args[n],XcNscaleSegments,
		(dlBar->object.height>INDICATOR_OKAY_SIZE ? 11 : 5)); n++;
	if (dlBar->label == LABEL_NONE) {
		XtSetArg(args[n],XcNscaleSegments, 0); n++;
	}
	break;
  }
  preferredHeight = dlBar->object.height/INDICATOR_FONT_DIVISOR;
  bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
	preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
  XtSetArg(args[n],XtNfont,fontTable[bestSize]); n++;

  XtSetArg(args[n],XcNbarForeground,(Pixel)
	displayInfo->dlColormap[dlBar->monitor.clr]); n++;
  XtSetArg(args[n],XcNbarBackground,(Pixel)
	displayInfo->dlColormap[dlBar->monitor.bclr]); n++;
  XtSetArg(args[n],XtNbackground,(Pixel)
	displayInfo->dlColormap[dlBar->monitor.bclr]); n++;
  XtSetArg(args[n],XcNcontrolBackground,(Pixel)
	displayInfo->dlColormap[dlBar->monitor.bclr]); n++;
/*
 * add the pointer to the ChannelAccessMonitorData structure as userData 
 *  to widget
 */
  XtSetArg(args[n],XcNuserData,(XtPointer)channelAccessMonitorData); n++;
  localWidget = XtCreateWidget("bar", 
		xcBarGraphWidgetClass, displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] = localWidget;

  if (displayInfo->traversalMode == DL_EXECUTE) {

/* record the widget that this structure belongs to */
    channelAccessMonitorData->self = localWidget;

/* add in drag/drop translations */
    XtOverrideTranslations(localWidget,parsedTranslations);

  } else if (displayInfo->traversalMode == DL_EDIT) {

/* add button press handlers */
    XtAddEventHandler(localWidget,ButtonPressMask,False,
		(XtEventHandler)handleButtonPress,(XtPointer)displayInfo);

    XtManageChild(localWidget);
  }

}



/***
 *** Indicator
 ***/


void executeDlIndicator(DisplayInfo *displayInfo, DlIndicator *dlIndicator,
				Boolean dummy)
{
  ChannelAccessMonitorData *channelAccessMonitorData;
  Arg args[30];
  int n;
  int usedHeight, usedCharWidth, bestSize, preferredHeight;
  Widget localWidget;


  displayInfo->useDynamicAttribute = FALSE;

  if (displayInfo->traversalMode == DL_EXECUTE) {
    channelAccessMonitorData = allocateChannelAccessMonitorData(displayInfo);
    channelAccessMonitorData->monitorType = DL_Indicator;
    channelAccessMonitorData->specifics = (XtPointer) dlIndicator;
    channelAccessMonitorData->clrmod = dlIndicator->clrmod;
    channelAccessMonitorData->label = dlIndicator->label;

/* put up white rectangle so that unconnected channels are obvious */
    XSetForeground(display,displayInfo->pixmapGC,WhitePixel(display,screenNum));
    XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
	dlIndicator->object.x,dlIndicator->object.y,
	dlIndicator->object.width,dlIndicator->object.height);
    XSetForeground(display,displayInfo->gc,WhitePixel(display,screenNum));
    XFillRectangle(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	dlIndicator->object.x,dlIndicator->object.y,
	dlIndicator->object.width,dlIndicator->object.height);

    SEVCHK(CA_BUILD_AND_CONNECT(dlIndicator->monitor.rdbk,TYPENOTCONN,0,
	&(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
	NULL), 
	"executeDlIndicator: error in CA_BUILD_AND_CONNECT");
    if (channelAccessMonitorData->chid != NULL)
	ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;
  }

/* from the indicator structure, we've got Indicator's specifics */
  n = 0;
  XtSetArg(args[n],XtNx,(Position)dlIndicator->object.x); n++;
  XtSetArg(args[n],XtNy,(Position)dlIndicator->object.y); n++;
  XtSetArg(args[n],XtNwidth,(Dimension)dlIndicator->object.width); n++;
  XtSetArg(args[n],XtNheight,(Dimension)dlIndicator->object.height); n++;
  XtSetArg(args[n],XcNdataType,XcFval); n++;
  switch (dlIndicator->label) {
     case LABEL_NONE:
	XtSetArg(args[n],XcNvalueVisible,FALSE); n++;
	XtSetArg(args[n],XcNlabel," "); n++;
	break;
     case OUTLINE:
	XtSetArg(args[n],XcNvalueVisible,FALSE); n++;
	XtSetArg(args[n],XcNlabel," "); n++;
	break;
     case LIMITS:
	XtSetArg(args[n],XcNvalueVisible,TRUE); n++;
	XtSetArg(args[n],XcNlabel," "); n++;
	break;
     case CHANNEL:
	XtSetArg(args[n],XcNvalueVisible,TRUE); n++;
	XtSetArg(args[n],XcNlabel,dlIndicator->monitor.rdbk); n++;
	break;
  }

  switch (dlIndicator->direction) {
/*
 * note that this is  "direction of increase"
 */
     case DOWN:
	fprintf(stderr,
	    "\nexecuteDlIndicator: DOWN direction INDICATORS not supported");
     case UP:
	XtSetArg(args[n],XcNscaleSegments,
		(dlIndicator->object.width >INDICATOR_OKAY_SIZE ? 11 : 5)); n++;
	XtSetArg(args[n],XcNorient,XcVert); n++;
	if (dlIndicator->label == LABEL_NONE) {
		XtSetArg(args[n],XcNscaleSegments,0); n++;
	}
	break;

     case LEFT:
	fprintf(stderr,
	    "\nexecuteDlIndicator: LEFT direction INDICATORS not supported");
     case RIGHT:
	XtSetArg(args[n],XcNscaleSegments,
		(dlIndicator->object.height>INDICATOR_OKAY_SIZE ? 11 : 5)); n++;
	XtSetArg(args[n],XcNorient,XcHoriz); n++;
	if (dlIndicator->label == LABEL_NONE) {
		XtSetArg(args[n],XcNscaleSegments,0); n++;
	}
	break;
  }
  preferredHeight = dlIndicator->object.height/INDICATOR_FONT_DIVISOR;
  bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
	preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
  XtSetArg(args[n],XtNfont,fontTable[bestSize]); n++;

  XtSetArg(args[n],XcNindicatorForeground,(Pixel)
	displayInfo->dlColormap[dlIndicator->monitor.clr]); n++;
  XtSetArg(args[n],XcNindicatorBackground,(Pixel)
	displayInfo->dlColormap[dlIndicator->monitor.bclr]); n++;
  XtSetArg(args[n],XtNbackground,(Pixel)
	displayInfo->dlColormap[dlIndicator->monitor.bclr]); n++;
  XtSetArg(args[n],XcNcontrolBackground,(Pixel)
	displayInfo->dlColormap[dlIndicator->monitor.bclr]); n++;
/*
 * add the pointer to the ChannelAccessMonitorData structure as userData 
 *  to widget
 */
  XtSetArg(args[n],XcNuserData,(XtPointer)channelAccessMonitorData); n++;
  localWidget = XtCreateWidget("indicator", 
		xcIndicatorWidgetClass, displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] = localWidget;

  if (displayInfo->traversalMode == DL_EXECUTE) {

/* record the widget that this structure belongs to */
    channelAccessMonitorData->self = localWidget;

/* add in drag/drop translations */
    XtOverrideTranslations(localWidget,parsedTranslations);

  } else if (displayInfo->traversalMode == DL_EDIT) {

/* add button press handlers */
    XtAddEventHandler(localWidget,ButtonPressMask,False,
		(XtEventHandler)handleButtonPress,(XtPointer)displayInfo);

    XtManageChild(localWidget);
  }

}





/***
 *** Text Update
 ***/


void executeDlTextUpdate(DisplayInfo *displayInfo, DlTextUpdate *dlTextUpdate,
				Boolean dummy)
{
  ChannelAccessMonitorData *channelAccessMonitorData;
  XRectangle clipRect[1];
  int usedHeight, usedWidth;
  int localFontIndex;
  size_t nChars;


  displayInfo->useDynamicAttribute = FALSE;

  if (displayInfo->traversalMode == DL_EXECUTE) {
    channelAccessMonitorData = allocateChannelAccessMonitorData(displayInfo);
    channelAccessMonitorData->monitorType = DL_TextUpdate;
    channelAccessMonitorData->specifics = (XtPointer) dlTextUpdate;
    channelAccessMonitorData->clrmod = dlTextUpdate->clrmod;
    channelAccessMonitorData->label = LABEL_NONE;
    channelAccessMonitorData->fontIndex = dmGetBestFontWithInfo(fontTable,
	MAX_FONTS,DUMMY_TEXT_FIELD,
	dlTextUpdate->object.height, dlTextUpdate->object.width, 
	&usedHeight, &usedWidth, FALSE);	/* don't use width */

    SEVCHK(CA_BUILD_AND_CONNECT(dlTextUpdate->monitor.rdbk,TYPENOTCONN,0,
	&(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
	NULL), 
	"executeDlTextUpdate: error in CA_BUILD_AND_CONNECT");
    if (channelAccessMonitorData->chid != NULL)
	ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;

  } else {

  /* since no ca callbacks to put up text, put up dummy region */
    XSetForeground(display,displayInfo->gc,
	displayInfo->dlColormap[ dlTextUpdate->monitor.bclr]);
    XFillRectangle(display, XtWindow(displayInfo->drawingArea),
	displayInfo->gc,
	dlTextUpdate->object.x,dlTextUpdate->object.y,
	dlTextUpdate->object.width, dlTextUpdate->object.height);
    XFillRectangle(display,displayInfo->drawingAreaPixmap,
	displayInfo->gc,
	dlTextUpdate->object.x,dlTextUpdate->object.y,
	dlTextUpdate->object.width, dlTextUpdate->object.height);

    XSetForeground(display,displayInfo->gc,
	displayInfo->dlColormap[dlTextUpdate->monitor.clr]);
    XSetBackground(display,displayInfo->gc,
	displayInfo->dlColormap[dlTextUpdate->monitor.bclr]);
    nChars = strlen(dlTextUpdate->monitor.rdbk);
    localFontIndex = dmGetBestFontWithInfo(fontTable,
	MAX_FONTS,dlTextUpdate->monitor.rdbk,
	dlTextUpdate->object.height, dlTextUpdate->object.width, 
	&usedHeight, &usedWidth, FALSE);	/* don't use width */
    usedWidth = XTextWidth(fontTable[localFontIndex],dlTextUpdate->monitor.rdbk,
		nChars);

/* clip to bounding box (especially for text) */
    clipRect[0].x = dlTextUpdate->object.x;
    clipRect[0].y = dlTextUpdate->object.y;
    clipRect[0].width  = dlTextUpdate->object.width;
    clipRect[0].height =  dlTextUpdate->object.height;
    XSetClipRectangles(display,displayInfo->gc,0,0,clipRect,1,YXBanded);

    XSetFont(display,displayInfo->gc,fontTable[localFontIndex]->fid);
    switch(dlTextUpdate->align) {
      case HORIZ_LEFT:
      case VERT_TOP:
	XDrawString(display,displayInfo->drawingAreaPixmap,
	  displayInfo->gc,
	  dlTextUpdate->object.x,dlTextUpdate->object.y +
			fontTable[localFontIndex]->ascent,
	  dlTextUpdate->monitor.rdbk,strlen(dlTextUpdate->monitor.rdbk));
	XDrawString(display,XtWindow(displayInfo->drawingArea),
	  displayInfo->gc,
	  dlTextUpdate->object.x,dlTextUpdate->object.y +
			fontTable[localFontIndex]->ascent,
	  dlTextUpdate->monitor.rdbk,strlen(dlTextUpdate->monitor.rdbk));
	break;
      case HORIZ_CENTER:
      case VERT_CENTER:
	XDrawString(display,displayInfo->drawingAreaPixmap,
	  displayInfo->gc,
	  dlTextUpdate->object.x + (dlTextUpdate->object.width - usedWidth)/2,
	  dlTextUpdate->object.y + fontTable[localFontIndex]->ascent,
	  dlTextUpdate->monitor.rdbk,strlen(dlTextUpdate->monitor.rdbk));
	XDrawString(display,XtWindow(displayInfo->drawingArea),
	  displayInfo->gc,
	  dlTextUpdate->object.x + (dlTextUpdate->object.width - usedWidth)/2,
	  dlTextUpdate->object.y + fontTable[localFontIndex]->ascent,
	  dlTextUpdate->monitor.rdbk,strlen(dlTextUpdate->monitor.rdbk));
	break;
      case HORIZ_RIGHT:
      case VERT_BOTTOM:
	XDrawString(display,displayInfo->drawingAreaPixmap,
	  displayInfo->gc,
	  dlTextUpdate->object.x + dlTextUpdate->object.width - usedWidth,
	  dlTextUpdate->object.y + fontTable[localFontIndex]->ascent,
	  dlTextUpdate->monitor.rdbk,strlen(dlTextUpdate->monitor.rdbk));
	XDrawString(display,XtWindow(displayInfo->drawingArea),
	  displayInfo->gc,
	  dlTextUpdate->object.x + dlTextUpdate->object.width - usedWidth,
	  dlTextUpdate->object.y + fontTable[localFontIndex]->ascent,
	  dlTextUpdate->monitor.rdbk,strlen(dlTextUpdate->monitor.rdbk));
	break;
    }


/* and turn off clipping on exit */
    XSetClipMask(display,displayInfo->gc,None);
  }

}





/***
 ***  Strip Chart
 ***/

static XtCallbackProc drawShadows(
  XmDrawingAreaWidget w,
  XtPointer client_data,
  XtPointer call_data)
{
  Arg args[8];
  int n;
  Dimension width, height, shadowThickness;
  GC topGC;
  GC bottomGC;

/* trying to force DrawingArea to draw a shadow */
  topGC = ((struct _XmDrawingAreaRec *)w)->manager.top_shadow_GC;
  bottomGC = ((struct _XmDrawingAreaRec *)w)->manager.bottom_shadow_GC;
  
  n = 0;
  XtSetArg(args[n],XmNwidth,&width); n++;
  XtSetArg(args[n],XmNheight,&height); n++;
  XtSetArg(args[n],XmNshadowThickness,&shadowThickness); n++;
  XtGetValues((Widget)w,args,n);
  _XmDrawShadows(XtDisplay(w),XtWindow(w),topGC,bottomGC,0,0,
	width,height,shadowThickness,XmSHADOW_OUT);
}


static XtCallbackProc redisplayFakeStrip(
  Widget w,
  DlStripChart *dlStripChart,
  XtPointer call_data)
{
  if (currentDisplayInfo != NULL && dlStripChart != NULL) {
    XSetLineAttributes(display,currentDisplayInfo->gc,0,
				LineSolid,CapButt,JoinMiter);
    XSetForeground(display,currentDisplayInfo->gc,
	currentDisplayInfo->dlColormap[dlStripChart->plotcom.bclr]);
    XFillRectangle(display,XtWindow(w),currentDisplayInfo->gc,
	0,0,dlStripChart->object.width,dlStripChart->object.height);
    XSetForeground(display,currentDisplayInfo->gc,
	currentDisplayInfo->dlColormap[dlStripChart->plotcom.clr]);
    XDrawRectangle(display,XtWindow(w),currentDisplayInfo->gc,
	(int)(0.2*dlStripChart->object.width),
	(int)(0.2*dlStripChart->object.height),
	(unsigned int) (0.6*dlStripChart->object.width),
	(unsigned int) (0.6*dlStripChart->object.height));
    XDrawString(display,XtWindow(w),currentDisplayInfo->gc,
	(int)(0.2*dlStripChart->object.width),
	(int)(0.15*dlStripChart->object.height),"Strip Chart", 11);
  }
}



void executeDlStripChart(DisplayInfo *displayInfo, DlStripChart *dlStripChart,
				Boolean dummy)
{
  ChannelAccessMonitorData *channelAccessMonitorData, *monitorData;
  StripChartData *stripChartData;
  Strip *strip;
  StripRange stripRange[1];
  int i, j, n;
  Arg args[15];
  Widget localWidget;


  displayInfo->useDynamicAttribute = FALSE;

  stripChartData = NULL;

  if (displayInfo->traversalMode == DL_EXECUTE) {
    stripChartData = (StripChartData *) calloc(1,sizeof(StripChartData));
    j = 0;
    for (i = 0; i < MAX_PENS; i++) {
      if (dlStripChart->pen[i].chan[0] != '\0') {

       channelAccessMonitorData = allocateChannelAccessMonitorData(displayInfo);
       channelAccessMonitorData->monitorType = DL_StripChart;
       channelAccessMonitorData->specifics = (XtPointer) dlStripChart;

       SEVCHK(CA_BUILD_AND_CONNECT(dlStripChart->pen[i].chan,TYPENOTCONN,0,
	&(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
	NULL), 
	"executeDlStripChart: error in CA_BUILD_AND_CONNECT");
       if (channelAccessMonitorData->chid != NULL)
	ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;

/* add pointer to this monitor data structure to the strip chart structure */
       stripChartData->monitors[j] = (XtPointer) channelAccessMonitorData;
       j++;

      }
    }
  }

/* put up white rectangle so that unconnected channels are obvious */
  XSetForeground(display,displayInfo->pixmapGC,WhitePixel(display,screenNum));
  XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
	dlStripChart->object.x,dlStripChart->object.y,
	dlStripChart->object.width,dlStripChart->object.height);
  XSetForeground(display,displayInfo->gc,WhitePixel(display,screenNum));
  XFillRectangle(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	dlStripChart->object.x,dlStripChart->object.y,
	dlStripChart->object.width,dlStripChart->object.height);


/* from the stripChart structure, we've got some of StripChart's specifics */
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlStripChart->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlStripChart->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlStripChart->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlStripChart->object.height); n++;
  XtSetArg(args[n],XmNmarginWidth,0); n++;
  XtSetArg(args[n],XmNmarginHeight,0); n++;
  XtSetArg(args[n],XmNshadowThickness,2); n++;
  XtSetArg(args[n],XmNforeground,(Pixel)
	displayInfo->dlColormap[dlStripChart->plotcom.clr]); n++;
  XtSetArg(args[n],XmNbackground,(Pixel)
	displayInfo->dlColormap[dlStripChart->plotcom.bclr]); n++;
  XtSetArg(args[n],XmNtraversalOn,False); n++;
/*
 * add the pointer to the StripChartData structure as userData to widget
 */
  XtSetArg(args[n], XmNuserData, (XtPointer) stripChartData); n++;

  localWidget = XmCreateDrawingArea(displayInfo->drawingArea,
	stripChartWidgetName,args,n);
  displayInfo->child[displayInfo->childCount++] = localWidget;

/* add expose callback to force shadows drawn (DrawingArea should do this!) */
  XtAddCallback(localWidget,XmNexposeCallback,(XtCallbackProc)drawShadows,NULL);

  if (displayInfo->traversalMode == DL_EXECUTE) {

/* record the number of channels in the strip chart */
    stripChartData->nChannels = j;

/* record the widget that the monitor structures belongs to */
    for (i = 0; i < stripChartData->nChannels; i++) {
      monitorData = (ChannelAccessMonitorData *) stripChartData->monitors[i];
      monitorData->self = localWidget;
    }
/* add in drag/drop translations */
    XtOverrideTranslations(localWidget,parsedTranslations);


  } else if (displayInfo->traversalMode == DL_EDIT) {

    XtManageChild(localWidget);

    XSetForeground(display,displayInfo->gc,
	displayInfo->dlColormap[dlStripChart->plotcom.clr]);
    XDrawRectangle(display,XtWindow(localWidget),displayInfo->gc,
			(int)(0.2*dlStripChart->object.width),
			(int)(0.2*dlStripChart->object.height),
			(unsigned int) (0.6*dlStripChart->object.width),
			(unsigned int) (0.6*dlStripChart->object.height));
    XDrawString(display,XtWindow(localWidget),displayInfo->gc,
			(int)(0.1*dlStripChart->object.width),
			(int)(0.1*dlStripChart->object.height),
			 "Strip Chart...", 14);
	
    XtAddCallback(localWidget,XmNexposeCallback,
			(XtCallbackProc)redisplayFakeStrip,dlStripChart);

/* add button press handlers */
    XtAddEventHandler(localWidget,
	ButtonPressMask,False,(XtEventHandler)handleButtonPress,
	(XtPointer)displayInfo);


  }

/* add expose callback to force shadows drawn (DrawingArea should do this!) */
/*   important to add this last therefore shadows always drawn! */
  XtAddCallback(localWidget,XmNexposeCallback,(XtCallbackProc)drawShadows,NULL);

}





/***
 ***  Cartesian Plot
 ***/


void executeDlCartesianPlot(DisplayInfo *displayInfo,
			DlCartesianPlot *dlCartesianPlot, Boolean dummy)
{
  ChannelAccessMonitorData *channelAccessMonitorData, *monitorData,
	*triggerMonitorData;
  CartesianPlotData *cartesianPlotData;
  int i, j, k, n, validTraces, iPrec;
  Arg args[42];
  Widget localWidget;
  XColor xColors[2];
  char *headerStrings[2];
  char rgb[2][16], string[24];
  Boolean validTrace;
  int usedHeight, usedCharWidth, bestSize, preferredHeight;
  XcVType minF, maxF, tickF;
  int precision, log10Precision;


  displayInfo->useDynamicAttribute = FALSE;

  cartesianPlotData = NULL;
  triggerMonitorData = (ChannelAccessMonitorData *)NULL;

  validTraces = 0;
  if (displayInfo->traversalMode == DL_EXECUTE) {
    cartesianPlotData = (CartesianPlotData *)
				calloc(1,sizeof(CartesianPlotData));
    cartesianPlotData->xrtData1 = cartesianPlotData->xrtData2 = NULL;
    cartesianPlotData->triggerMonitorData = (XtPointer) NULL;

    for (i = 0; i < MAX_TRACES; i++) {
      validTrace = False;
/* X data */
      if (dlCartesianPlot->trace[validTraces].xdata[0] != '\0') {
       channelAccessMonitorData = allocateChannelAccessMonitorData(displayInfo);
       channelAccessMonitorData->monitorType = DL_CartesianPlot;
       channelAccessMonitorData->specifics = (XtPointer) dlCartesianPlot;
       SEVCHK(CA_BUILD_AND_CONNECT(dlCartesianPlot->trace[validTraces].xdata,
	TYPENOTCONN,
	0,&(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
	NULL), 
	"executeDlCartesianPlot: error in CA_BUILD_AND_CONNECT");
       if (channelAccessMonitorData->chid != NULL)
          ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;
/* add pointer to this monitor data structure to the cartesian plot structure */
       cartesianPlotData->monitors[validTraces][0] = (XtPointer)
	channelAccessMonitorData;
       validTrace = True;
      }
/* Y data */
      if (dlCartesianPlot->trace[validTraces].ydata[0] != '\0') {
       channelAccessMonitorData = allocateChannelAccessMonitorData(displayInfo);
       channelAccessMonitorData->monitorType = DL_CartesianPlot;
       channelAccessMonitorData->specifics = (XtPointer) dlCartesianPlot;
       SEVCHK(CA_BUILD_AND_CONNECT(dlCartesianPlot->trace[validTraces].ydata,
	TYPENOTCONN,
	0,&(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
	NULL), 
	"executeDlCartesianPlot: error in CA_BUILD_AND_CONNECT");
       if (channelAccessMonitorData->chid != NULL)
          ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;
/* add pointer to this monitor data structure to the cartesian plot structure */
       cartesianPlotData->monitors[validTraces][1] = (XtPointer)
	channelAccessMonitorData;
       validTrace = True;
      }
      if (validTrace) validTraces++;
    }

/* now add monitor for trigger channel if appropriate */
    if (dlCartesianPlot->trigger[0] != '\0') {
       channelAccessMonitorData = allocateChannelAccessMonitorData(displayInfo);
       channelAccessMonitorData->monitorType = DL_CartesianPlot;
       channelAccessMonitorData->specifics = (XtPointer) dlCartesianPlot;
       SEVCHK(CA_BUILD_AND_CONNECT(dlCartesianPlot->trigger, TYPENOTCONN,
	0,&(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
	NULL), 
	"executeDlCartesianPlot: error in CA_BUILD_AND_CONNECT");
       if (channelAccessMonitorData->chid != NULL)
          ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;
/* add ptr. to trigger monitor data structure to the cartesian plot structure */
       cartesianPlotData->triggerMonitorData =
		(XtPointer) channelAccessMonitorData;
       triggerMonitorData = channelAccessMonitorData;
    }
  }


/* put up white rectangle so that unconnected channels are obvious */
  XSetForeground(display,displayInfo->pixmapGC,WhitePixel(display,screenNum));
  XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
	dlCartesianPlot->object.x,dlCartesianPlot->object.y,
	dlCartesianPlot->object.width,dlCartesianPlot->object.height);
  XSetForeground(display,displayInfo->gc,WhitePixel(display,screenNum));
  XFillRectangle(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	dlCartesianPlot->object.x,dlCartesianPlot->object.y,
	dlCartesianPlot->object.width,dlCartesianPlot->object.height);


/* from the cartesianPlot structure, we've got CartesianPlot's specifics */
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlCartesianPlot->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlCartesianPlot->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlCartesianPlot->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlCartesianPlot->object.height); n++;
  XtSetArg(args[n],XmNhighlightThickness,0); n++;

/* long way around for color handling... but XRT/Graph insists on strings! */
  xColors[0].pixel = displayInfo->dlColormap[dlCartesianPlot->plotcom.clr];
  xColors[1].pixel = displayInfo->dlColormap[dlCartesianPlot->plotcom.bclr];
  XQueryColors(display,cmap,xColors,2);
  sprintf(rgb[0],"#%2.2x%2.2x%2.2x",xColors[0].red>>8, xColors[0].green>>8,
		xColors[0].blue>>8);
  sprintf(rgb[1],"#%2.2x%2.2x%2.2x",xColors[1].red>>8, xColors[1].green>>8,
		xColors[1].blue>>8);
  XtSetArg(args[n],XtNxrtGraphForegroundColor,rgb[0]); n++;
  XtSetArg(args[n],XtNxrtGraphBackgroundColor,rgb[1]); n++;
  XtSetArg(args[n],XtNxrtForegroundColor,rgb[0]); n++;
  XtSetArg(args[n],XtNxrtBackgroundColor,rgb[1]); n++;
  preferredHeight = MIN(dlCartesianPlot->object.width,
		dlCartesianPlot->object.height)/TITLE_SCALE_FACTOR;
  bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
		preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
  XtSetArg(args[n],XtNxrtHeaderFont,fontTable[bestSize]->fid); n++;
  if (strlen(dlCartesianPlot->plotcom.title) > 0) {
    headerStrings[0] = dlCartesianPlot->plotcom.title;
  } else {
    headerStrings[0] = NULL;
  }
  headerStrings[1] = NULL;
  XtSetArg(args[n],XtNxrtHeaderStrings,headerStrings); n++;
  if (strlen(dlCartesianPlot->plotcom.xlabel) > 0) {
    XtSetArg(args[n],XtNxrtXTitle,dlCartesianPlot->plotcom.xlabel); n++;
  } else {
    XtSetArg(args[n],XtNxrtXTitle,NULL); n++;
  }
  if (strlen(dlCartesianPlot->plotcom.ylabel) > 0) {
    XtSetArg(args[n],XtNxrtYTitle,dlCartesianPlot->plotcom.ylabel); n++;
  } else {
    XtSetArg(args[n],XtNxrtYTitle,NULL); n++;
  }
  preferredHeight = MIN(dlCartesianPlot->object.width,
		dlCartesianPlot->object.height)/AXES_SCALE_FACTOR;
  bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
		preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
  XtSetArg(args[n],XtNxrtAxisFont,fontTable[bestSize]->fid); n++;
  switch (dlCartesianPlot->style) {
    case POINT_PLOT:
    case LINE_PLOT:
	XtSetArg(args[n],XtNxrtType,XRT_PLOT); n++;
	if (validTraces > 1) {
	    XtSetArg(args[n],XtNxrtType2,XRT_PLOT); n++;
	}
	break;
    case FILL_UNDER_PLOT:
	XtSetArg(args[n],XtNxrtType,XRT_AREA); n++;
	if (validTraces > 1) {
	    XtSetArg(args[n],XtNxrtType2,XRT_AREA); n++;
	}
	break;
  }
  if (validTraces > 1) {
    XtSetArg(args[n],XtNxrtY2AxisShow,TRUE); n++;
  }

/******* Axis Definitions *******/
/* X Axis definition */
  switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].axisStyle) {
	case LINEAR_AXIS:
		break;
	case LOG10_AXIS:
		XtSetArg(args[n],XtNxrtYAxisLogarithmic,True); n++;
		break;
	default:
		fprintf(stderr,
		"\nexecuteDlCartesianPlot: unknown X axis style"); break;
		break;
  }
  switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle) {
	case CHANNEL_RANGE:		/* handle as default until connected */
	case AUTO_SCALE_RANGE:
		XtSetArg(args[n],XtNxrtXNumUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtXTickUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtXPrecisionUseDefault,True); n++;
		break;
	case USER_SPECIFIED_RANGE:
		minF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].minRange;
		maxF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].maxRange;
		tickF.fval = (maxF.fval - minF.fval)/4.0;
		XtSetArg(args[n],XtNxrtXMin,minF.lval); n++;
		XtSetArg(args[n],XtNxrtXMax,maxF.lval); n++;
		XtSetArg(args[n],XtNxrtXTick,tickF.lval); n++;
		XtSetArg(args[n],XtNxrtXNum,tickF.lval); n++;
		sprintf(string,"%f",tickF.fval);
		k = strlen(string)-1;
		while (string[k] == '0') k--; /* strip off trailing zeroes */
		iPrec = k;
		while (string[k] != '.' && k >= 0) k--;
		iPrec = iPrec - k;
		XtSetArg(args[n],XtNxrtXPrecision,iPrec); n++;
		break;
	default:
		fprintf(stderr,
		"\nexecuteDlCartesianPlot: unknown X range style"); break;
		break;
  }
/* Y1 Axis definition */
  switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].axisStyle) {
	case LINEAR_AXIS:
		break;
	case LOG10_AXIS:
		XtSetArg(args[n],XtNxrtYAxisLogarithmic,True); n++;
		break;
	default:
		fprintf(stderr,
		"\nexecuteDlCartesianPlot: unknown Y1 axis style"); break;
		break;
  }
  switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle) {
	case CHANNEL_RANGE:		/* handle as default until connected */
	case AUTO_SCALE_RANGE:
		XtSetArg(args[n],XtNxrtYNumUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtYTickUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtYPrecisionUseDefault,True); n++;
		break;
	case USER_SPECIFIED_RANGE:
		minF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].minRange;
		maxF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange;
		tickF.fval = (maxF.fval - minF.fval)/4.0;
		XtSetArg(args[n],XtNxrtYMin,minF.lval); n++;
		XtSetArg(args[n],XtNxrtYMax,maxF.lval); n++;
		XtSetArg(args[n],XtNxrtYTick,tickF.lval); n++;
		XtSetArg(args[n],XtNxrtYNum,tickF.lval); n++;
		sprintf(string,"%f",tickF.fval);
		k = strlen(string)-1;
		while (string[k] == '0') k--; /* strip off trailing zeroes */
		iPrec = k;
		while (string[k] != '.' && k >= 0) k--;
		iPrec = iPrec - k;
		XtSetArg(args[n],XtNxrtYPrecision,iPrec); n++;
		break;
	default:
		fprintf(stderr,
		"\nexecuteDlCartesianPlot: unknown Y1 range style"); break;
		break;
  }
/* Y2 Axis definition */
  switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].axisStyle) {
	case LINEAR_AXIS:
		break;
	case LOG10_AXIS:
		XtSetArg(args[n],XtNxrtY2AxisLogarithmic,True); n++;
		break;
	default:
		fprintf(stderr,
		"\nexecuteDlCartesianPlot: unknown Y2 axis style"); break;
		break;
  }
  switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle) {
	case CHANNEL_RANGE:		/* handle as default until connected */
	case AUTO_SCALE_RANGE:
		XtSetArg(args[n],XtNxrtY2NumUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtY2TickUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtY2PrecisionUseDefault,True); n++;
		break;
	case USER_SPECIFIED_RANGE:
		minF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].minRange;
		maxF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].maxRange;
		tickF.fval = (maxF.fval - minF.fval)/4.0;
		XtSetArg(args[n],XtNxrtY2Min,minF.lval); n++;
		XtSetArg(args[n],XtNxrtY2Max,maxF.lval); n++;
		XtSetArg(args[n],XtNxrtY2Tick,tickF.lval); n++;
		XtSetArg(args[n],XtNxrtY2Num,tickF.lval); n++;
		sprintf(string,"%f",tickF.fval);
		k = strlen(string)-1;
		while (string[k] == '0') k--; /* strip off trailing zeroes */
		iPrec = k;
		while (string[k] != '.' && k >= 0) k--;
		iPrec = iPrec - k;
		XtSetArg(args[n],XtNxrtY2Precision,iPrec); n++;
		break;
	default:
		fprintf(stderr,
		"\nexecuteDlCartesianPlot: unknown Y2 range style"); break;
		break;
  }
  


  XtSetArg(args[n],XmNtraversalOn,False); n++;
/*
 * add the pointer to the CartesianPlotData structure as userData to widget
 */
  XtSetArg(args[n], XmNuserData, (XtPointer) cartesianPlotData); n++;
  XtSetArg(args[n], XtNxrtDoubleBuffer, True); n++;
  localWidget = XtCreateWidget("cartesianPlot",xtXrtGraphWidgetClass,
		displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] = localWidget;

  if (displayInfo->traversalMode == DL_EXECUTE) {

/* record the number of traces in the cartesian plot */
    cartesianPlotData->nTraces = validTraces;

/* record the widget that the monitor structures belong to */
    if (triggerMonitorData != NULL) triggerMonitorData->self = localWidget;
    for (i = 0; i < cartesianPlotData->nTraces; i++) {
      for (j = 0; j <= 1; j++) {
         monitorData = (ChannelAccessMonitorData *)
		cartesianPlotData->monitors[i][j];
	 if (monitorData != NULL)
		monitorData->self = localWidget;
      }
    }
/* add in drag/drop translations */
    XtOverrideTranslations(localWidget,parsedTranslations);


  } else if (displayInfo->traversalMode == DL_EDIT) {

/* add button press handlers */
    XtAddEventHandler(localWidget,
	ButtonPressMask,False,(XtEventHandler)handleButtonPress,
	(XtPointer)displayInfo);

    XtManageChild(localWidget);
  }

}




/***
 ***  Surface Plot
 ***/


void executeDlSurfacePlot(DisplayInfo *displayInfo,
			DlSurfacePlot *dlSurfacePlot, Boolean dummy)
{
  ChannelAccessMonitorData *channelAccessMonitorData;
  int n;
  Arg args[12];
  Widget localWidget;


  displayInfo->useDynamicAttribute = FALSE;

  if (displayInfo->traversalMode == DL_EXECUTE) {
    channelAccessMonitorData = allocateChannelAccessMonitorData(displayInfo);
    channelAccessMonitorData->monitorType = DL_SurfacePlot;
    channelAccessMonitorData->specifics = (XtPointer) dlSurfacePlot;
  }

/* put up white rectangle so that unconnected channels are obvious */
  XSetForeground(display,displayInfo->pixmapGC,WhitePixel(display,screenNum));
  XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
	dlSurfacePlot->object.x,dlSurfacePlot->object.y,
	dlSurfacePlot->object.width,dlSurfacePlot->object.height);
  XSetForeground(display,displayInfo->gc,WhitePixel(display,screenNum));
  XFillRectangle(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	dlSurfacePlot->object.x,dlSurfacePlot->object.y,
	dlSurfacePlot->object.width,dlSurfacePlot->object.height);


/* from the surfacePlot structure, we've got SurfacePlot's specifics */
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlSurfacePlot->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlSurfacePlot->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlSurfacePlot->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlSurfacePlot->object.height); n++;
  XtSetArg(args[n],XmNhighlightThickness,0); n++;
  XtSetArg(args[n],XmNforeground,(Pixel)
	displayInfo->dlColormap[dlSurfacePlot->plotcom.clr]); n++;
  XtSetArg(args[n],XmNbackground,(Pixel)
	displayInfo->dlColormap[dlSurfacePlot->plotcom.bclr]); n++;
  XtSetArg(args[n],XmNtraversalOn,False); n++;
/*
 * add the pointer to the ChannelAccessMonitorData structure as userData 
 *  to widget
 */
  XtSetArg(args[n],XmNuserData,(XtPointer)channelAccessMonitorData); n++;
  localWidget = XtCreateWidget("surfacePlot", xmDrawingAreaWidgetClass, 
		displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] =  localWidget;

  if (displayInfo->traversalMode == DL_EXECUTE) {

/* record the widget that this structure belongs to */
    channelAccessMonitorData->self = localWidget;

  } else if (displayInfo->traversalMode == DL_EDIT) {

/* add button press handlers */
  XtAddEventHandler(localWidget,
	ButtonPressMask,False,(XtEventHandler)handleButtonPress,
	(XtPointer)displayInfo);
    XtManageChild(localWidget);
  }
}



