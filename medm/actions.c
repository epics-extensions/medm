
#include "medm.h"
#include <Xm/MwmUtil.h>

#include "cvtFast.h"

extern char *stripChartWidgetName;

/*
 * action routines (XtActionProc) and some associated callbacks
 */


static XtCallbackProc sendKeyboardValue(
  Widget  w,
  ChannelAccessControllerData *channelAccessControllerData,
  XmSelectionBoxCallbackStruct *call_data)
{
  double value;
  char *stringValue;

  if (channelAccessControllerData == NULL) return;

  XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET, &stringValue);

  if (stringValue != NULL) {
     value = atof(stringValue);
/* update controller data value for book keeping purposes */
     channelAccessControllerData->value = value;

/* set modifed on monitorData so that at next traversal, the valuator
 * is guaranteed to be visually correct
 */
     globalModifiedFlag = True;
     if (channelAccessControllerData->monitorData != NULL)
	 channelAccessControllerData->monitorData->modified = PRIMARY_MODIFIED;

/* move/redraw valuator & value, but force use of user-selected value */
     valuatorSetValue(channelAccessControllerData->monitorData,value,True);
     if (ca_state(channelAccessControllerData->chid) == cs_conn) {
       SEVCHK(ca_put(DBR_DOUBLE,channelAccessControllerData->chid,&value),
	"sendKeyboardValue: error in ca_put");
       ca_flush_io();
     } else {
       fprintf(stderr,"\nsendKeyboardValue: %s not connected",
		ca_name(channelAccessControllerData->chid));
     }
     XtFree(stringValue);
  }
  XtDestroyWidget(XtParent(w));

}



static XtCallbackProc destroyDialog(
  Widget  w,
  XtPointer client_data,
  XmSelectionBoxCallbackStruct *call_data)
{
  XtDestroyWidget(XtParent(w));
}


static XtCallbackProc precisionToggleChangedCallback(
  Widget w,
  XtPointer client_data,
  XmToggleButtonCallbackStruct *call_data)
{
  Widget widget;
  long longValue;
  short shortValue;
  ChannelAccessControllerData *data;
  XtPointer userData;
  DlValuator *valuator;
  Arg args[2];

/* only respond to the button actually set */
  if (call_data->event != NULL && call_data->set == True) {

    longValue = (long)client_data;
    shortValue = (short)longValue;

    XtSetArg(args[0],XmNuserData,&userData);
    XtGetValues(w,args,1);
    data = (ChannelAccessControllerData *)userData;
/*
 * now set the prec field in the valuator data structure, and update
 * the valuator (scale) resources
 */
    if (data != NULL) {
      valuator = (DlValuator *)data->monitorData->specifics;
      valuator->dPrecision = pow(10.,(double)shortValue);
    }

/* hierarchy = TB<-RB<-Frame<-SelectionBox<-Dialog */
    widget = w;
    while (XtClass(widget) != xmDialogShellWidgetClass) {
      widget = XtParent(widget);
    }

    XtDestroyWidget(widget);
  }

}



/*
 * text field processing callback
 */
static XtCallbackProc precTextFieldActivateCallback(
  Widget w,
  DlValuator *dlValuator,
  XmTextVerifyCallbackStruct *cbs)
{
  char *stringValue;
  int i;
  Widget widget;

  stringValue = XmTextFieldGetString(w);
  dlValuator->dPrecision = atof(stringValue);
  XtFree(stringValue);

/* hierarchy = TB<-RB<-Frame<-SelectionBox<-Dialog */
    widget = w;
    while (XtClass(widget) != xmDialogShellWidgetClass) {
      widget = XtParent(widget);
    }
    XtDestroyWidget(widget);
}



/*
 * text field losing focus callback
 */
static XtCallbackProc precTextFieldLosingFocusCallback(
  Widget w,
  DlValuator *dlValuator,
  XmTextVerifyCallbackStruct *cbs)
{
  char string[MAX_TOKEN_LENGTH];
  int tail;

/*
 * losing focus - make sure that the text field remains accurate
 *	wrt dlValuator
 */
  sprintf(string,"%f",dlValuator->dPrecision);
  /* strip trailing zeroes */
  tail = strlen(string);
  while (string[--tail] == '0') string[tail] = '\0';
  XmTextFieldSetString(w,string);

}



/* since passing client_data didn't seem to work... */
static char *channelName;


static Boolean DragConvertProc(w,selection,target,typeRtn,valueRtn,
				lengthRtn,formatRtn,max_lengthRtn,
				client_data,
				request_id)
  Widget w;
  Atom *selection, *target, *typeRtn;
  XtPointer *valueRtn;
  unsigned long *lengthRtn;
  int *formatRtn;
  unsigned long *max_lengthRtn;
  XtPointer client_data;
  XtRequestId *request_id;
{
  XmString cString;
  char *cText, *passText;

  if (channelName != NULL) {
    if (*target != COMPOUND_TEXT) return(False);
    cString = XmStringCreateSimple(channelName);
    cText = XmCvtXmStringToCT(cString);
    passText = XtMalloc(strlen(cText)+1);
    memcpy(passText,cText,strlen(cText)+1);
   /* probably need this too */
    XmStringFree(cString);

   /* format the value for return */
    *typeRtn = COMPOUND_TEXT;
    *valueRtn = (XtPointer)passText;
    *lengthRtn = strlen(passText);
    *formatRtn = 8;	/* from example - related to #bits for data elements */
    return(True);
  } else {
/* monitorData not found */
    return(False);
  }

}


/*
 * cleanup after drag/drop
 */
static void dragDropFinish(
  Widget w,
  XtPointer client,
  XtPointer call)
{
  Widget sourceIcon;
  Pixmap pixmap;
  Arg args[2];

/* perform cleanup at conclusion of DND */
  XtSetArg(args[0],XmNsourcePixmapIcon,&sourceIcon);
  XtGetValues(w,args,1);

  XtSetArg(args[0],XmNpixmap,&pixmap);
  XtGetValues(sourceIcon,args,1);

  XFreePixmap(display,pixmap);
  XtDestroyWidget(sourceIcon);
}

static XtCallbackRec dragDropFinishCB[] = {
        {dragDropFinish,NULL},
        {NULL,NULL}
};




void StartDrag(
  Widget w,
  XEvent *event)
{
  Arg args[8];
  Cardinal n;
  Atom exportList[1];
  Widget sourceIcon;
  ChannelAccessMonitorData *mData, *md, *mdArray[MAX(MAX_PENS,MAX_TRACES)][2];
  int dir, asc, desc, maxWidth, maxHeight, maxAsc, maxDesc, maxTextWidth;
  int nominalY, i, j, liveChannels, liveTraces;
  XCharStruct overall;
  unsigned long fg, bg;
  Widget searchWidget;
  XButtonEvent *xbutton;
  XtPointer userData;
  StripChartData *stripChartData;
  CartesianPlotData *cartesianPlotData;
  XGCValues gcValues;
  unsigned long gcValueMask;
  Boolean newTrace, haveXY;
  DisplayInfo *displayInfo;

  static char *channelNames[MAX(MAX_PENS,MAX_TRACES)][2];
  Pixmap sourcePixmap = (Pixmap)NULL;
  static GC gc = NULL;

/* a nice sized font */
#define FONT_TABLE_INDEX 6
/* move the text over... */
#define X_SHIFT 8
#define MARGIN  2

  
/* (MDA) since widget doing drag could be toggleButton or optionMenu button
 *   (which has more than just flat, single parent),
 *   find the widget that has a parent that is the drawing area and
 *   search based on that *   (since that is what is rooted in the display)
 * - NB if drawing areas as children of the main drawing area are allowed
 *   as parents of controllers/monitors, this logic must change...
 */
  searchWidget = w;
  if (XtClass(searchWidget) == xmDrawingAreaWidgetClass
		&& strcmp(XtName(searchWidget),stripChartWidgetName)) {
    /* starting search from a DrawingArea which is not a StripChart 
     *  (i.e., DL_Display) therefore lookup "graphic" (non-widget) elements 
     *  ---get data from position
     */
    displayInfo = dmGetDisplayInfoFromWidget(searchWidget);
    xbutton = (XButtonEvent *)event;
    mData = dmGetChannelAccessMonitorDataFromPosition(displayInfo,
		xbutton->x,xbutton->y);
  } else {
   /* ---get data from widget */
    while (XtClass(XtParent(searchWidget)) != xmDrawingAreaWidgetClass)
	searchWidget = XtParent(searchWidget);
    mData = dmGetChannelAccessMonitorDataFromWidget(searchWidget);
  }


  if (mData != NULL) {

/* always color according to severity (to convey channel name and severity) */
    bg = BlackPixel(display,screenNum);

    switch (mData->monitorType) {

	case DL_StripChart:
		channelName = NULL;
		maxAsc = 0; maxDesc = 0; maxWidth = 0;
		liveChannels = 0;
		for (i = 0; i < MAX_PENS; i++) {
		   channelNames[i][0]= NULL;
		   mdArray[i][0] = NULL;
		}
		if (XtClass(searchWidget) == xmDrawingAreaWidgetClass
		    && strcmp(XtName(searchWidget),stripChartWidgetName)) {
		/* if we got here via parent DA (not StripChart), get widget */
		   searchWidget = mData->self;
		}
		XtVaGetValues(searchWidget,XmNuserData,&userData,NULL);
		stripChartData = (StripChartData *)userData;
		for (i = 0; i < stripChartData->nChannels; i++) {
		  if (stripChartData->monitors[i] != NULL){
		    md = (ChannelAccessMonitorData *)
				stripChartData->monitors[i];
		    mdArray[liveChannels][0] = md;
		    if ( md->chid != NULL) {
		      channelNames[liveChannels][0] = ca_name(md->chid);
 		      XTextExtents(fontTable[FONT_TABLE_INDEX],
			channelNames[liveChannels][0],
			strlen(channelNames[liveChannels][0]),
			&dir,&asc,&desc,&overall);
		      maxWidth = MAX(maxWidth,overall.width);
		      maxAsc = MAX(maxAsc,asc);
		      maxDesc = MAX(maxDesc,desc);
		      liveChannels++;
		    }
		  }
		}
		/* since most information for StripCharts is from connect...*/
		if (liveChannels > 0) {
		  maxWidth += X_SHIFT + MARGIN;
		  maxHeight = liveChannels*(maxAsc+maxDesc+MARGIN);
		  nominalY = maxAsc + MARGIN;
		  sourcePixmap = XCreatePixmap(display,RootWindow(display,
			screenNum),maxWidth,maxHeight,
			DefaultDepth(display,screenNum));
		  if (gc == NULL) gc = XCreateGC(display,sourcePixmap,0,NULL);
		  gcValueMask = GCForeground|GCBackground|GCFunction|GCFont;
		  gcValues.foreground = bg;
		  gcValues.background = bg;
		  gcValues.function = GXcopy;
		  gcValues.font = fontTable[FONT_TABLE_INDEX]->fid;
		  XChangeGC(display,gc,gcValueMask,&gcValues);
		  XFillRectangle(display,sourcePixmap,gc,0,0,maxWidth,
			maxHeight);
		  for (i = 0; i < liveChannels; i++) {
		    if (mdArray[i][0] != NULL && channelNames[i][0] != NULL) {
		      fg = alarmColorPixel[mdArray[i][0]->severity];
		      XSetForeground(display,gc,fg);
		      XDrawString(display,sourcePixmap,gc,X_SHIFT,
			(i+1)*nominalY,channelNames[i][0],
			strlen(channelNames[i][0]));
		    }
		  }
		}
		break;

	case DL_CartesianPlot:
		channelName = NULL;
		maxAsc = 0; maxDesc = 0; maxWidth = 0;
		liveTraces = 0;
		for (i = 0; i < MAX_PENS; i++)
		  for (j = 0; j <= 1; j++) {
		     channelNames[i][j] = NULL;
		     mdArray[i][j] = NULL;
		  }
		if (XtClass(searchWidget) == xmDrawingAreaWidgetClass) {
		/* if we got here via parent DA (not CP), get widget */
		   searchWidget = mData->self;
		}
		XtVaGetValues(searchWidget,XmNuserData,&userData,NULL);
		cartesianPlotData = (CartesianPlotData *)userData;
		for (i = 0; i < cartesianPlotData->nTraces; i++) {
		  newTrace = False;
		  for (j = 0; j <= 1; j++) {
		    if (cartesianPlotData->monitors[i][j] != NULL){
		      md = (ChannelAccessMonitorData *)
				cartesianPlotData->monitors[i][j];
		      newTrace = True;
		      mdArray[liveTraces][j] = md;
		      if ( md->chid != NULL) {
		        channelNames[liveTraces][j] = ca_name(md->chid);
 		        XTextExtents(fontTable[FONT_TABLE_INDEX],
			  channelNames[liveTraces][j],
			  strlen(channelNames[liveTraces][j]),
			  &dir,&asc,&desc,&overall);
		        maxWidth = MAX(maxWidth,overall.width);
		        maxAsc = MAX(maxAsc,asc);
		        maxDesc = MAX(maxDesc,desc);
		      }
		    }
		  }
		  if (newTrace) liveTraces++;
		}
	     /* since most information for CartesianPlots is from connect..*/
		if (liveTraces > 0) {
		/* see if we have XY plot; if so make room for x,y in pixmap */
		  haveXY = False;
		  for (i = 0; i < liveTraces; i++) {
		    if (mdArray[i][0] != NULL && mdArray[i][1] != NULL)
			haveXY = True;
		  }
		  maxTextWidth = maxWidth;
		  if (haveXY) {
		    maxWidth += maxWidth + X_SHIFT + 4*MARGIN;
		  } else {
		    maxWidth += X_SHIFT + MARGIN;
		  }
		  maxHeight = liveTraces*(maxAsc+maxDesc+MARGIN);
		  nominalY = maxAsc + MARGIN;
		  sourcePixmap = XCreatePixmap(display,RootWindow(display,
			screenNum),maxWidth,maxHeight,
			DefaultDepth(display,screenNum));
		  if (gc == NULL) gc = XCreateGC(display,sourcePixmap,0,NULL);
		  gcValueMask = GCForeground|GCBackground|GCFunction|GCFont;
		  gcValues.foreground = bg;
		  gcValues.background = bg;
		  gcValues.function = GXcopy;
		  gcValues.font = fontTable[FONT_TABLE_INDEX]->fid;
		  XChangeGC(display,gc,gcValueMask,&gcValues);
		  XFillRectangle(display,sourcePixmap,gc,0,0,maxWidth,
			maxHeight);
		  for (i = 0; i < liveTraces; i++) {
		    for (j = 0; j <= 1; j++) {
		      if (mdArray[i][j] != NULL && channelNames[i][j] != NULL) {
		        fg = alarmColorPixel[mdArray[i][j]->severity];
		        XSetForeground(display,gc,fg);
		        XDrawString(display,sourcePixmap,gc,
			  (j == 0 ? X_SHIFT :
				maxTextWidth + 3*MARGIN + X_SHIFT),
			  (i+1)*nominalY,
			  channelNames[i][j],strlen(channelNames[i][j]));
		      }
		    }
		  }
		}
		break;

	default:
		fg = alarmColorPixel[mData->severity];
		overall.width = 0;
		asc = 0; desc = 0;
		channelName = NULL;
		if (mData->chid != NULL) channelName = ca_name(mData->chid);
		if (channelName != NULL) {
 		  XTextExtents(fontTable[FONT_TABLE_INDEX],channelName,
			strlen(channelName),&dir,&asc,&desc,&overall);
		  overall.width += X_SHIFT + MARGIN;
		  maxWidth = overall.width;
		  maxHeight = asc+desc+2*MARGIN;
		  sourcePixmap = XCreatePixmap(display,RootWindow(display,
			screenNum),maxWidth,maxHeight,
			DefaultDepth(display,screenNum));
		  if (gc == NULL) gc = XCreateGC(display,sourcePixmap,0,NULL);
		  gcValueMask = GCForeground|GCBackground|GCFunction|GCFont;
		  gcValues.foreground = bg;
		  gcValues.background = bg;
		  gcValues.function = GXcopy;
		  gcValues.font = fontTable[FONT_TABLE_INDEX]->fid;
		  XChangeGC(display,gc,gcValueMask,&gcValues);
		  XFillRectangle(display,sourcePixmap,gc,0,0,maxWidth,
			maxHeight);
		  XSetForeground(display,gc,fg);
		  XDrawString(display,sourcePixmap,gc,X_SHIFT,asc+MARGIN,
			channelName,strlen(channelName));
		}
		break;
    }

  } else {

    channelName = NULL;
    return;

  }


  if (sourcePixmap != (Pixmap)NULL) {

/* use source widget as parent - can inherit visual attributes that way */
    n = 0;
    XtSetArg(args[n],XmNpixmap,sourcePixmap); n++;
    XtSetArg(args[n],XmNwidth,maxWidth); n++;
    XtSetArg(args[n],XmNheight,maxHeight); n++;
    XtSetArg(args[n],XmNdepth,DefaultDepth(display,screenNum)); n++;
    sourceIcon = XmCreateDragIcon(XtParent(searchWidget),"sourceIcon",args,n);

/* establish list of valid target types */
    exportList[0] = COMPOUND_TEXT;

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







void popupValuatorKeyboardEntry(
  Widget w,
  DisplayInfo *displayInfo,
  XEvent *event)
{
#define MAX_TOGGLES 20
  Widget keyboardDialog;
  char valueLabel[MAX_TOKEN_LENGTH + 8];
  XmString xmTitle, xmValueLabel, valueXmString;
  char valueString[40];
  char *channel;
  Arg args[8];
  int n;
  ChannelAccessControllerData *channelAccessControllerData;
  ChannelAccessMonitorData *channelAccessMonitorData;

  Widget frame, frameLabel, radioBox, toggles[MAX_TOGGLES];
  Widget form, textField;
  XmString frameXmString, toggleXmString;
  double hoprLoprAbs;
  short numColumns, numPlusColumns, numMinusColumns, shortValue;
  char toggleString[4];
  int i, count, tail;
  long longValue;
  DlValuator *dlValuator;

  XButtonEvent *xEvent = (XButtonEvent *)event;

  if (globalDisplayListTraversalMode == DL_EDIT) {
  /* do nothing */
  } else {

    if (xEvent->button != Button3) return;

    XtSetArg(args[0],XmNuserData,&channelAccessControllerData);
    XtGetValues(w,args,1);
    if (channelAccessControllerData != NULL) {
      if (channelAccessControllerData->chid != NULL) {
	channel = ca_name(channelAccessControllerData->chid);
	if (channelAccessControllerData->connected == TRUE &&
				strlen(channel) > 0) {
/* create selection box/prompt dialog */
	  strcpy(valueLabel,"VALUE: ");
	  strcat(valueLabel,channel);
	  xmValueLabel = XmStringCreateSimple(valueLabel);
	  xmTitle = XmStringCreateSimple(channel);
	  channelAccessMonitorData = channelAccessControllerData->monitorData;
	  dlValuator = (DlValuator *)channelAccessMonitorData->specifics;
	  cvtDoubleToString(channelAccessMonitorData->value,valueString,
			channelAccessMonitorData->precision);
	  valueXmString = XmStringCreateSimple(valueString);
	  n = 0;
	  XtSetArg(args[n],XmNdialogStyle,
				XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
	  XtSetArg(args[n],XmNselectionLabelString,xmValueLabel); n++;
	  XtSetArg(args[n],XmNdialogTitle,xmTitle); n++;
	  XtSetArg(args[n],XmNtextString,valueXmString); n++;
	  keyboardDialog = XmCreatePromptDialog(w,channel,args,n);

/* remove resize handles from shell */
	  XtSetArg(args[0],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH);
	  XtSetValues(XtParent(keyboardDialog),args,1);


	  XtAddCallback(keyboardDialog,XmNokCallback,
		(XtCallbackProc)sendKeyboardValue, channelAccessControllerData);
	  XtAddCallback(keyboardDialog,XmNcancelCallback,
		(XtCallbackProc)destroyDialog,NULL);

/* create frame/radiobox/toggles for precision selection */
	  hoprLoprAbs = fabs(channelAccessMonitorData->hopr);
	  hoprLoprAbs = MAX(hoprLoprAbs,fabs(channelAccessMonitorData->lopr));
	/* log10 + 1 */
	  numPlusColumns =  (short)log10(hoprLoprAbs) + 1;
	  numMinusColumns = (short)channelAccessMonitorData->precision;
	/* leave room for decimal point */
	  numColumns = numPlusColumns + 1 + numMinusColumns;
	  if (numColumns > MAX_TOGGLES) {
		fprintf(stderr,
		"\npopupValuatorKeyboardEntry: maximum # of toggles exceeded");
		numColumns = MAX_TOGGLES;
	  }
	  n = 0;
	  frame = XmCreateFrame(keyboardDialog,"frame",args,n);
	  frameXmString = XmStringCreateSimple("VALUATOR PRECISION (10^X)");
	  XtSetArg(args[n],XmNlabelString,frameXmString); n++;
	  XtSetArg(args[n],XmNchildType,XmFRAME_TITLE_CHILD); n++;
	  frameLabel = XmCreateLabel(frame,"frameLabel",args,n);
	  XtManageChild(frameLabel);

	  n = 0;
	  XtSetArg(args[n],XmNchildType,XmFRAME_WORKAREA_CHILD); n++;
	  XtSetArg(args[n],XmNshadowThickness,0); n++;
	  form = XmCreateForm(frame,"form",args,n);

/* radio box */
	  n = 0;
	  XtSetArg(args[n],XmNnumColumns,numColumns); n++;
	  XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
	  XtSetArg(args[n],XmNadjustLast,False); n++;
	  XtSetArg(args[n],XmNspacing,0); n++;
	  radioBox = XmCreateRadioBox(form,"radioBox",args,n);

	  toggleXmString = (XmString)NULL;
	  XtSetArg(args[0],XmNindicatorOn,False);
/* digits to the left of the decimal point */
	  count = 0;
	  for (i = numPlusColumns - 1; i >= 0; i--) {
	     if (toggleXmString != NULL) XmStringFree(toggleXmString);
	     shortValue = (short)i;
	     cvtShortToString(shortValue,toggleString);
	     toggleXmString = XmStringCreateSimple(toggleString);
	     XtSetArg(args[1],XmNlabelString,toggleXmString);
	     XtSetArg(args[2],XmNuserData,(XtPointer)
					channelAccessControllerData);
	     if (log10(dlValuator->dPrecision) == (double)i) {
		XtSetArg(args[3],XmNset,True);
	     }
	     toggles[count++] = XmCreateToggleButton(radioBox,"toggles",args,
			(log10(dlValuator->dPrecision) == (double)i ? 4 : 3));
	     longValue = (long)shortValue;
	     XtAddCallback(toggles[count-1],XmNvalueChangedCallback,
			(XtCallbackProc)precisionToggleChangedCallback,
			(XtPointer)longValue);
	  }
/* the decimal point */
	  if (toggleXmString != NULL) XmStringFree(toggleXmString);
	  toggleString[0] = '.'; toggleString[1] = '\0';
	  toggleXmString = XmStringCreateSimple(toggleString);
	  XtSetArg(args[1],XmNlabelString,toggleXmString);
	  XtSetArg(args[2],XmNshadowThickness,0);
	  toggles[count++] = XmCreateToggleButton(radioBox,"toggles",args,3);
	  XtSetSensitive(toggles[count-1],False);

/* digits to the right of the decimal point */
	  for (i = 1; i <= numMinusColumns; i++) {
	     if (toggleXmString != NULL) XmStringFree(toggleXmString);
	     shortValue = (short)-i;
	     cvtShortToString(shortValue,toggleString);
	     toggleXmString = XmStringCreateSimple(toggleString);
	     XtSetArg(args[1],XmNlabelString,toggleXmString);
	     XtSetArg(args[2],XmNuserData,(XtPointer)
					channelAccessControllerData);
	     if (log10(dlValuator->dPrecision) == (double)-i) {
		XtSetArg(args[3],XmNset,True);
	     }
	     toggles[count++] = XmCreateToggleButton(radioBox,"toggles",args,
			(log10(dlValuator->dPrecision) == (double)-i ? 4 : 3));
	     longValue = (long)shortValue;
	     XtAddCallback(toggles[count-1],XmNvalueChangedCallback,
			(XtCallbackProc)precisionToggleChangedCallback,
			(XtPointer)longValue);
	  }

/* text field */
	  n = 0;
	  textField = XmCreateTextField(form,"textField",args,n);
	  XtAddCallback(textField,XmNactivateCallback,
			(XtCallbackProc)precTextFieldActivateCallback,
			(XtPointer)dlValuator);
	  XtAddCallback(textField,XmNlosingFocusCallback,
			(XtCallbackProc)precTextFieldLosingFocusCallback,
			(XtPointer)dlValuator);
	  XtAddCallback(textField,XmNmodifyVerifyCallback,
			(XtCallbackProc)textFieldFloatVerifyCallback,
			(XtPointer)NULL);
	  sprintf(valueString,"%f",dlValuator->dPrecision);
	  /* strip trailing zeroes */
	  tail = strlen(valueString);
	  while (valueString[--tail] == '0') valueString[tail] = '\0';
	  XmTextFieldSetString(textField,valueString);


/* now specify attatchments of radio box and text field in form */
	  n = 0;
	  XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
	  XtSetValues(radioBox,args,n);
	  n = 0;
	  XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
	  XtSetArg(args[n],XmNtopWidget,radioBox); n++;
	  XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
	  XtSetValues(textField,args,n);


	  XtManageChildren(toggles,numColumns);
	  XtManageChild(radioBox);
	  XtManageChild(textField);
	  XtManageChild(form);
	  XtManageChild(frame);
	  if (toggleXmString != NULL) XmStringFree(toggleXmString);
	  XmStringFree(frameXmString);

	  XtManageChild(keyboardDialog);
	  XmStringFree(xmValueLabel);
	  XmStringFree(xmTitle);
	  XmStringFree(valueXmString);
	} else {
	  fprintf(stderr,
		"\npopupValuatorKeyboardEntry: illegal/unconnected channel");
	}
      }
    }
  }
}

