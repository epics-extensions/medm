
#include "medm.h"

extern Boolean isCurrentMonitor(ChannelAccessMonitorData *monitorData);


/*
 * NULL event handler for channels being searched (not yet connected)
 *      but now invalidated because of execute->edit state change
 */
void nullControllerConnectionEventHandler(struct connection_handler_args args)
{
#ifdef DEBUG
fprintf(stderr,"\n nullControllerConnectionEventHandler: %s",
		ca_name(args.chid));
#endif
  return;
}



/*
 * function to return a double which is the number of seconds between
 *   samples.  input an integer delay and unit specifier, and return
 *   the corresponding number of seconds
 */

static double stripCalculateInterval(int delay, TimeUnits units)
{
  double returnValue;

  switch (units) {
    case MILLISECONDS:
	returnValue = SECS_PER_MILLISEC * delay;
	break;
    case SECONDS:
	returnValue = delay;
	break;
    case MINUTES:
	returnValue = SECS_PER_MIN * delay;
	break;

    default:
	fprintf(stderr,"\nstripCalculateInterval: unknown UNITS specified");
	returnValue = delay;
	break;
  }
  return (returnValue);

}



static double stripGetValue(int channelNumber, StripChartData *stripChartData)
{
  ChannelAccessMonitorData *monitorData;

  monitorData = (ChannelAccessMonitorData *) 
	stripChartData->monitors[channelNumber];

  return (monitorData->value);


}



/***
 *** channel access routines
 ***/



/******************************************************************************
 *  This function initializes channel access
 ******************************************************************************/
void dmInitializeCA()
{

/*
 * add CA's fd to X
 */
  SEVCHK(ca_add_fd_registration(dmRegisterCA,NULL),
	"\ndmInitializeCA: error adding CA's fd to X");

}



/******************************************************************************
 *  This function terminates channel access
 *	and un-registers CA's fd with X... 
 ******************************************************************************/
void dmTerminateCA()
{

   /* cancel registration of the CA file descriptors */
      SEVCHK(ca_add_fd_registration(dmRegisterCA,NULL),
                "\ndmTerminateCA:  error removing CA's fd from X");
   /* and close channel access */
      ca_pend_event(20.0*CA_PEND_EVENT_TIME);	/* don't allow early returns */


      SEVCHK(ca_task_exit(),"\ndmTerminateCA: error exiting CA");

}


/******************************************************************************
 * functions to register Channel Access' file descriptor with Xt
 *      and perform CA handling when events exist on that event stream
 ******************************************************************************/

void dmRegisterCA(
  void *dummy,
  int fd,
  int condition)
{
  int currentNumInps;

#define NUM_INITIAL_FDS 100
typedef struct {
	XtInputId inputId;
	int fd;
} InputIdAndFd;
 static InputIdAndFd *inp = NULL;
 static int maxInps = 0, numInps = 0;
 int i, j, k;



 if (inp == NULL && maxInps == 0) { 
/* first time through */
   inp = (InputIdAndFd *) calloc(1,NUM_INITIAL_FDS*sizeof(InputIdAndFd));
   maxInps = NUM_INITIAL_FDS;
   numInps = 0;
 }

 if (condition) {
/*
 * add new fd
 */
   if (numInps < maxInps-1) {

	inp[numInps].fd = fd;
	inp[numInps].inputId  = XtAppAddInput(appContext,fd,
			(XtPointer)XtInputReadMask,
			(XtInputCallbackProc)dmProcessCA,(XtPointer)NULL);
	numInps++;

   } else {

fprintf(stderr,"\ndmRegisterCA: info: realloc-ing input fd's array");

	maxInps = 2*maxInps;
	inp = (InputIdAndFd *) realloc(inp,maxInps*sizeof(InputIdAndFd));
	inp[numInps].fd = fd;
	inp[numInps].inputId  = XtAppAddInput(appContext,fd,
			(XtPointer)XtInputReadMask,
			(XtInputCallbackProc)dmProcessCA,(XtPointer)NULL);
	numInps++;
   }

 } else {

  currentNumInps = numInps;

/*
 * remove old fd/inputId
 */
   for (i = 0; i < numInps; i++) {
	if (inp[i].fd == fd) {
	   XtRemoveInput(inp[i].inputId);
	   inp[i].inputId = (XtInputId)NULL;
	   inp[i].fd = (int)NULL; 
	   currentNumInps--;
	}
   }

/* now remove holes in the array */
   i = 0;
   while (i < numInps) {
	if (inp[i].inputId == (XtInputId)NULL) {
	   j = i+1;
	   k = 0;
	   while(inp[j].inputId != (XtInputId)NULL) {
	      inp[i+k].inputId = inp[j].inputId;
	      inp[i+k].fd = inp[j].fd;
	      j++;
	      k++;
	   }
	   i = j-1;
	}
	i++;
   }
   numInps = currentNumInps;

 }

#ifdef DEBUG
fprintf(stderr,"\ndmRegisterCA: numInps = %d\n\t",numInps);
for (i = 0; i < maxInps; i++)
    fprintf(stderr,"%d ",inp[i].fd);
fprintf(stderr,"\n");
#endif

}




XtInputCallbackProc dmProcessCA()
{
  ca_pend_event(CA_PEND_EVENT_TIME);	/* don't allow early return */
  if (globalDisplayListTraversalMode == DL_EXECUTE) {
    traverseMonitorList(FALSE,NULL,0,0,0,0);
  }
}






/******************************************************************************
 * Channel Access callback and event handling functions...
 ******************************************************************************/



/***
 ***  Monitor routines
/***

/*
 * monitor connection event handler - when the channel connects,
 *   register a GR type get_callback() to get the graphics
 *   information for the monitor object/widget
 */
void processMonitorConnectionEvent(struct connection_handler_args args)
{
  chtype channelType;
  XExposeEvent exposeEvent;
  DlRectangle *dlRectangle;
  ChannelAccessMonitorData *data =
			(ChannelAccessMonitorData *) ca_puser(args.chid);


  if (globalDisplayListTraversalMode != DL_EXECUTE || data == NULL) return;
  globalModifiedFlag = True;

/* use displayInfo->drawingArea == NULL as sign of "in cleanup", hence ignore */
  if (data->displayInfo == NULL) return;
  if (data->displayInfo->drawingArea == NULL) return;


  if (data->self != NULL) {
	if (args.op != CA_OP_CONN_UP) /* unmanage if not connected */
		XtUnmanageChild(data->self);
	else
		XtManageChild(data->self);
  }
  if (args.op != CA_OP_CONN_UP) {
  /* connection down */
	  data->modified = PRIMARY_MODIFIED;
  } else {
  /* connection up */
      if (ELEMENT_IS_STATIC(data->monitorType)) {
	if (data->dlDyn != NULL)
	   if (data->dlDyn->attr.mod.vis != V_STATIC) {
	/* as usual, as long as OBJECT is the first part of any structure, then
	 *   this works */
                dlRectangle = (DlRectangle *)data->specifics;
	/*
	 * synthesize an expose event to do refresh to take care of VIS
	 */
		exposeEvent.type = Expose;
		exposeEvent.serial = 0;
		exposeEvent.send_event = True;
		exposeEvent.display = display;
		exposeEvent.window = XtWindow(data->displayInfo->drawingArea);
		if (data->monitorType == DL_RisingLine
		  || data->monitorType == DL_FallingLine
		  || data->monitorType == DL_Polyline) {
	     /* if falling/rising line, since width/height can == 0 */
	     /* special case since lines span x -> x+width	  */
		  exposeEvent.x = dlRectangle->object.x-1;
		  exposeEvent.y = dlRectangle->object.y-1;
		  exposeEvent.width = MAX(1, dlRectangle->object.width+1)+1;
		  exposeEvent.height = MAX(1, dlRectangle->object.height+1)+1;
		} else {
		  exposeEvent.x = dlRectangle->object.x;
		  exposeEvent.y = dlRectangle->object.y;
		  exposeEvent.width = dlRectangle->object.width;
		  exposeEvent.height = dlRectangle->object.height;
		}
		exposeEvent.count = 0;

	/* MDA - this could perhaps be optimized to gather near dynamic
	 *   objects into singe larger expose event */
		XSendEvent(display,XtWindow(data->displayInfo->drawingArea),
			False,ExposureMask,(XEvent *)&exposeEvent);
	   }
      }
  }

  if (args.op != CA_OP_CONN_UP || data->previouslyConnected) return;

  if (!ELEMENT_IS_CONTROLLER(data->monitorType))
    data->previouslyConnected = True;

  switch(ca_field_type(args.chid)) {
	case DBF_STRING:
		channelType = DBR_GR_STRING; break;
	case DBF_INT:
/*	case DBF_SHORT: -- INT and SHORT are same thing in db_access.h */
		channelType = DBR_GR_INT; break;
	case DBF_FLOAT:
		channelType = DBR_GR_FLOAT; break;
	case DBF_ENUM:
		channelType = DBR_GR_ENUM; break;
	case DBF_CHAR:
		channelType = DBR_GR_CHAR; break;
	case DBF_LONG:
		channelType = DBR_GR_LONG; break;
	case DBF_DOUBLE:
		channelType = DBR_GR_DOUBLE; break;
	default:
		fprintf(stderr,
		  "\nprocessMonitorConnectionEvent: unknown field type = %d",
		   ca_field_type(args.chid));
		return;
  }
  SEVCHK(ca_get_callback(channelType,args.chid,
	processMonitorGrGetCallback,NULL),
	"\nprocessMonitorConnectionEvent: error in ca_get_callback");

}


/*
 * monitor graphics get callback - when the graphics information is available,
 *   store it and register a STS type event handler
 */
void processMonitorGrGetCallback(struct event_handler_args args)
{
  ChannelAccessMonitorData *channelAccessMonitorData = ca_puser(args.chid);
  chtype stsChannelType;
  struct dbr_sts_string  *sstData;
  struct dbr_gr_int    *ciData;
  struct dbr_gr_float  *cfData;
  struct dbr_gr_enum   *ceData;
  struct dbr_gr_char   *ccData;
  struct dbr_gr_long   *clData;
  struct dbr_gr_double *cdData;
  XtPointer data;
  int i, j, k, n, iValue;
  Boolean iBoolean;
  XcVType minVal, maxVal, currentVal;
  float f_hopr, f_lopr, f_value;
  Arg widgetArgs[12];
  WidgetList children;
  Cardinal numChildren;

  XtPointer userData;
  Boolean allConnected;
  ChannelAccessMonitorData *monitorData, *monitorDataX, *monitorDataY,
				*savedMonitorData;
  DlMeter *dlMeter;
  DlIndicator *dlIndicator;
  DlBar *dlBar;
  DlTextEntry *dlTextEntry;
  DlValuator *dlValuator;

/* Cartesian Plot variables */
  Widget widget;
  DisplayInfo *displayInfo;
  CartesianPlotData *cartesianPlotData;
  DlCartesianPlot *dlCartesianPlot;
  chid xChid, yChid;
  Boolean fail;
  XColor xColors[MAX_TRACES];
  char rgb[MAX_TRACES][16];
/* xrtData1 for 1st trace, xrtData2 for 2nd -> nTraces */
  XrtData *xrtData1, *xrtData2;
  XrtDataStyle myds1[MAX_TRACES], myds2[MAX_TRACES];
  float minX, maxX, minY, maxY, minY2, maxY2;
  XcVType minXF, maxXF, minYF, maxYF, minY2F, maxY2F, tickF;
  char string[24];
  int iPrec, kk;

/* strip chart variables */
  StripChartData *stripChartData;
  DlStripChart *dlStripChart;
  StripChartList *stripElement;
  StripRange stripRange[MAX_PENS];
  Dimension shadowThickness;
  XRectangle clipRect[1];
  int ii, usedHeight, usedCharWidth, bestSize, preferredHeight;
  double (*stripGetValueArray[MAX_PENS])();
  double stripGetValue();
  int maxElements;


/* (MDA) events can still be firing when we can't process them,
 *	thanks to asynchronicity
 */
  if (globalDisplayListTraversalMode != DL_EXECUTE
	|| channelAccessMonitorData == NULL) return;
  globalModifiedFlag = True;

/* use displayInfo->drawingArea == NULL as sign of "in cleanup", hence ignore */
  if (channelAccessMonitorData->displayInfo == NULL) return;
  if (channelAccessMonitorData->displayInfo->drawingArea == NULL) return;

/*
 * if this get callback returns after the monitor is invalidated, return
 */
  if (!isCurrentMonitor(channelAccessMonitorData))
    return;


  channelAccessMonitorData->modified = PRIMARY_MODIFIED;
  channelAccessMonitorData->precision = 0;
  channelAccessMonitorData->oldSeverity = INVALID_ALARM;

/* store status information in ChannelAccessData structure or widget */
  switch(args.type) {
	case DBR_GR_STRING:
		sstData = (struct dbr_sts_string *) args.dbr;
		strcpy(channelAccessMonitorData->oldStringValue,
			channelAccessMonitorData->stringValue);
		strncpy(channelAccessMonitorData->stringValue,sstData->value,
			MAX_STRING_SIZE-1);
		channelAccessMonitorData->stringValue[MAX_STRING_SIZE-1] = '\0';
		channelAccessMonitorData->status = sstData->status;
		channelAccessMonitorData->severity = sstData->severity;
		stsChannelType = DBR_STS_STRING;
		break;
	case DBR_GR_INT:
/*	case DBR_GR_SHORT: -- INT and SHORT are same thing in db_access.h */
		ciData = (struct dbr_gr_int *) args.dbr;
		channelAccessMonitorData->lopr = 
			(double) ciData->lower_disp_limit;
		channelAccessMonitorData->hopr = 
			(double) ciData->upper_disp_limit;
		channelAccessMonitorData->oldValue = 
			channelAccessMonitorData->value;
		channelAccessMonitorData->value = (double) ciData->value;
		channelAccessMonitorData->status = ciData->status;
		channelAccessMonitorData->severity =  ciData->severity;
		stsChannelType = DBR_STS_INT;
		break;
	case DBR_GR_FLOAT:
		cfData = (struct dbr_gr_float *) args.dbr;
		channelAccessMonitorData->lopr = 
			(double) cfData->lower_disp_limit;
		channelAccessMonitorData->hopr = 
			(double) cfData->upper_disp_limit;
		channelAccessMonitorData->precision = cfData->precision;
		channelAccessMonitorData->oldValue =
			channelAccessMonitorData->value;
		channelAccessMonitorData->value = (double) cfData->value;
		channelAccessMonitorData->status = cfData->status;
		channelAccessMonitorData->severity =  cfData->severity;
		stsChannelType = DBR_STS_FLOAT;
		break;
	case DBR_GR_ENUM:
		ceData = (struct dbr_gr_enum *) args.dbr;
		channelAccessMonitorData->oldValue =
			channelAccessMonitorData->value;
		channelAccessMonitorData->value = (double) ceData->value;
		channelAccessMonitorData->status = ceData->status;
		channelAccessMonitorData->severity =  ceData->severity;
		channelAccessMonitorData->lopr = (double) 0.0;
		/* high is number of enum values -1 */
		channelAccessMonitorData->hopr = (double)
			(MAX(0,ceData->no_str-1));
/* some enum-specific code: allocate and copy the state strings */
		channelAccessMonitorData->numberStateStrings = ceData->no_str;
		channelAccessMonitorData->stateStrings = (char **)
			malloc(MAX(1,ceData->no_str)*sizeof(char *));
		channelAccessMonitorData->xmStateStrings = (XmString *)
			malloc(MAX(1,ceData->no_str)*sizeof(XmString));
		for (i = 0; i < ceData->no_str; i++) {
			channelAccessMonitorData->stateStrings[i] = (char *)
				malloc((MAX_STATE_STRING_SIZE+1)*sizeof(char));
			strncpy(channelAccessMonitorData->stateStrings[i],
				ceData->strs[i],MAX_STATE_STRING_SIZE-1);
			channelAccessMonitorData->
				stateStrings[i][MAX_STATE_STRING_SIZE] = '\0';
			channelAccessMonitorData->xmStateStrings[i]=
				XmStringCreateSimple(
				   channelAccessMonitorData->stateStrings[i]);
		}

		stsChannelType = DBR_STS_ENUM;
		break;
	case DBR_GR_CHAR:
		ccData = (struct dbr_gr_char *) args.dbr;
		channelAccessMonitorData->lopr = 
			(double) ccData->lower_disp_limit;
		channelAccessMonitorData->hopr = 
			(double) ccData->upper_disp_limit;
		channelAccessMonitorData->oldValue =
			channelAccessMonitorData->value;
		channelAccessMonitorData->value = (double) ccData->value;
		channelAccessMonitorData->status = ccData->status;
		channelAccessMonitorData->severity =  ccData->severity;
		stsChannelType = DBR_STS_CHAR;
		break;
	case DBR_GR_LONG:
		clData = (struct dbr_gr_long *) args.dbr;
		channelAccessMonitorData->lopr = 
			(double) clData->lower_disp_limit;
		channelAccessMonitorData->hopr = 
			(double) clData->upper_disp_limit;
		channelAccessMonitorData->oldValue =
			channelAccessMonitorData->value;
		channelAccessMonitorData->value = (double) clData->value;
		channelAccessMonitorData->status = clData->status;
		channelAccessMonitorData->severity =  clData->severity;
		stsChannelType = DBR_STS_LONG;
		break;
	case DBR_GR_DOUBLE:
		cdData = (struct dbr_gr_double *) args.dbr;
		channelAccessMonitorData->lopr = 
			(double) cdData->lower_disp_limit;
		channelAccessMonitorData->hopr = 
			(double) cdData->upper_disp_limit;
		channelAccessMonitorData->precision = cdData->precision;
		channelAccessMonitorData->oldValue =
			channelAccessMonitorData->value;
		channelAccessMonitorData->value = (double) cdData->value;
		channelAccessMonitorData->status = cdData->status;
		channelAccessMonitorData->severity =  cdData->severity;
		stsChannelType = DBR_STS_DOUBLE;
		break;

	default:
		fprintf(stderr,
		  "\nprocessMonitorGrGetCallback: unknown field type = %d",
		   args.type);
		return;
  }

/* note these define range for both monitors and controllers */
#define DOUBLE_EPSILON  1E-10

/*
 *  safety net: fix up hopr and lopr to do something sensible
 */
  channelAccessMonitorData->lopr = MIN(channelAccessMonitorData->lopr,
					channelAccessMonitorData->hopr);
  channelAccessMonitorData->hopr = MAX(channelAccessMonitorData->lopr,
					channelAccessMonitorData->hopr);
  if ( (fabs(channelAccessMonitorData->lopr-channelAccessMonitorData->hopr)
        < DOUBLE_EPSILON) && args.type != DBR_GR_STRING) {

    if (channelAccessMonitorData->self != NULL) {

	switch(channelAccessMonitorData->monitorType) {
	    case DL_Valuator:
	    case DL_Meter:
	    case DL_Bar:
	    case DL_Indicator:
	    case DL_StripChart:
	    case DL_CartesianPlot:
	    case DL_SurfacePlot:
	/* hopr/lopr do matter for monitors which have a range to display */
		fprintf(stderr,
"\n..MonitorGrGetCallback:    %s hopr=lopr=%f, -> hopr=lopr+1.0",
		  ca_name(args.chid),
		  channelAccessMonitorData->lopr);
		channelAccessMonitorData->hopr = channelAccessMonitorData->lopr
							 + 1.0;
		break;

	/* absense of hopr/lopr don't really matter for other types */
	    default:
		break;
	}
    }
  }


/* set the value limits on the monitor object */

  switch(channelAccessMonitorData->monitorType) {

    case DL_Meter:
	if (args.type == DBR_GR_STRING) {
	    fprintf(stderr,"\nprocessMonitorGrGetCallback: %s %s %s",
		"illegal channel type for",ca_name(args.chid),
		": cannot attach Meter");
	} else {
	    f_hopr = (float) channelAccessMonitorData->hopr;
	    f_lopr = (float) channelAccessMonitorData->lopr;
	    f_value = (float) channelAccessMonitorData->value;
	    minVal.fval = f_lopr;
	    maxVal.fval = f_hopr;
	    currentVal.fval = f_value;
	    XtSetArg(widgetArgs[0],XcNlowerBound,minVal.lval);
	    XtSetArg(widgetArgs[1],XcNupperBound,maxVal.lval);
	    dlMeter = (DlMeter *)channelAccessMonitorData->specifics;
	    XtSetArg(widgetArgs[2],XcNmeterForeground,
		(channelAccessMonitorData->clrmod == ALARM ?
			alarmColorPixel[channelAccessMonitorData->severity] :
			channelAccessMonitorData->displayInfo->dlColormap[
			dlMeter->monitor.clr]));
	    XtSetArg(widgetArgs[3],XcNdecimals,
				(int)channelAccessMonitorData->precision);
	    if (channelAccessMonitorData->self != NULL) {
	      XtSetValues(channelAccessMonitorData->self,widgetArgs,4);
	      XcMeterUpdateValue(channelAccessMonitorData->self,&currentVal);
	    }
	}
	break;


    case DL_Indicator:
	if (args.type == DBR_GR_STRING) {
	    fprintf(stderr,"\nprocessMonitorGrGetCallback: %s %s %s",
		"illegal channel type for",ca_name(args.chid),
		": cannot attach Indicator");
	} else {
	    f_hopr = (float) channelAccessMonitorData->hopr;
	    f_lopr = (float) channelAccessMonitorData->lopr;
	    f_value = (float) channelAccessMonitorData->value;
	    minVal.fval = f_lopr;
	    maxVal.fval = f_hopr;
	    currentVal.fval = f_value;
	    XtSetArg(widgetArgs[0],XcNlowerBound,minVal.lval);
	    XtSetArg(widgetArgs[1],XcNupperBound,maxVal.lval);
	    dlIndicator = (DlIndicator *)channelAccessMonitorData->specifics;
	    XtSetArg(widgetArgs[2],XcNindicatorForeground,
		(channelAccessMonitorData->clrmod == ALARM ?
			alarmColorPixel[channelAccessMonitorData->severity] :
			channelAccessMonitorData->displayInfo->dlColormap[
			dlIndicator->monitor.clr]));
	    XtSetArg(widgetArgs[3],XcNdecimals,
				(int)channelAccessMonitorData->precision);
	    if (channelAccessMonitorData->self != NULL) {
	      XtSetValues(channelAccessMonitorData->self,widgetArgs,4);
	      XcIndUpdateValue(channelAccessMonitorData->self,&currentVal);
	    }
	}
	break;


    case DL_Bar:
	if (args.type == DBR_GR_STRING) {
	    fprintf(stderr,"\nprocessMonitorGrGetCallback: %s %s %s",
		"illegal channel type for",ca_name(args.chid),
		": cannot attach Indicator");
	} else {
	    f_hopr = (float) channelAccessMonitorData->hopr;
	    f_lopr = (float) channelAccessMonitorData->lopr;
	    f_value = (float) channelAccessMonitorData->value;
	    minVal.fval = f_lopr;
	    maxVal.fval = f_hopr;
	    currentVal.fval = f_value;
	    XtSetArg(widgetArgs[0],XcNlowerBound,minVal.lval);
	    XtSetArg(widgetArgs[1],XcNupperBound,maxVal.lval);
	    dlBar = (DlBar *)channelAccessMonitorData->specifics;
	    XtSetArg(widgetArgs[2],XcNbarForeground,
		(channelAccessMonitorData->clrmod == ALARM ?
			alarmColorPixel[channelAccessMonitorData->severity] :
			channelAccessMonitorData->displayInfo->dlColormap[
			dlBar->monitor.clr]));
	    XtSetArg(widgetArgs[3],XcNdecimals,
				(int)channelAccessMonitorData->precision);
	    if (channelAccessMonitorData->self != NULL) {
	      XtSetValues(channelAccessMonitorData->self,widgetArgs,4);
	      XcBGUpdateValue(channelAccessMonitorData->self,&currentVal);
	    }
	}
	break;



/******************/
/*  STRIP CHART   */
/******************/
/* lots of special handling for strip chart */
    case DL_StripChart:

	XtSetArg(widgetArgs[0],XmNuserData,&userData);
	XtGetValues(channelAccessMonitorData->self,widgetArgs,1);
	stripChartData = (StripChartData *) userData;
/*
 *  check to see if all component channels of strip chart have connected.
 *  if so, proceed with strip chart initialization
 */
	allConnected = TRUE;
	for (i = 0; i < stripChartData->nChannels; i++) {
	   monitorData = (ChannelAccessMonitorData*)stripChartData->monitors[i];
	   if (monitorData != NULL) {
	       allConnected = (allConnected &
			(monitorData->modified == PRIMARY_MODIFIED));
	       savedMonitorData = monitorData;
	   }
	}
	monitorData = savedMonitorData;

	if (allConnected) {

/* initialize strip chart */
	   dlStripChart = (DlStripChart *)monitorData->specifics;
	   stripChartData->strip = stripInit(display,screenNum,	
		XtWindow(monitorData->self));
/* set a clip rectangle in strip's GC to prevent drawing over shadows */
	   XtSetArg(widgetArgs[0],XmNshadowThickness,&shadowThickness);
	   XtGetValues(channelAccessMonitorData->self,widgetArgs,1);
	   clipRect[0].x = shadowThickness;
	   clipRect[0].y = shadowThickness;
	   clipRect[0].width  = dlStripChart->object.width - 2*shadowThickness;
	   clipRect[0].height = dlStripChart->object.height - 2*shadowThickness;
	   XSetClipRectangles(display,stripChartData->strip->gc,
		0,0,clipRect,1,YXBanded);

/* allocate a node on displayInfo's strip list */
	   stripElement = (StripChartList *) malloc(sizeof(StripChartList));
	   if (monitorData->displayInfo->stripChartListHead == NULL)
	/* first on list */
		monitorData->displayInfo->stripChartListHead = stripElement;
	   else
		monitorData->displayInfo->stripChartListTail->next=stripElement;
	   stripElement->strip = stripChartData->strip;
	   stripElement->next = NULL;
	   monitorData->displayInfo->stripChartListTail = stripElement;
/* get ranges for each channel; get proper lopr/hopr one way or another */
	   for (i = 0; i < stripChartData->nChannels; i++) {
		monitorData = (ChannelAccessMonitorData *)
			stripChartData->monitors[i];
		stripRange[i].minVal = MIN(monitorData->lopr,monitorData->hopr);
		stripRange[i].maxVal = MAX(monitorData->hopr,monitorData->lopr);
		if (stripRange[i].minVal == stripRange[i].maxVal) {
			stripRange[i].maxVal = stripRange[i].minVal + 1.0;
		}
		stripGetValueArray[i] = stripGetValue;
	   }
/* do own font and color handling */
	    stripChartData->strip->setOwnFonts  = TRUE;
	    stripChartData->strip->shadowThickness = shadowThickness;

	    preferredHeight = GraphX_TitleFontSize(stripChartData->strip);
	    bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
		preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
	    stripChartData->strip->titleFontStruct = fontTable[bestSize];
	    stripChartData->strip->titleFontHeight = usedHeight;
	    stripChartData->strip->titleFontWidth = usedCharWidth;

	    preferredHeight = GraphX_AxesFontSize(stripChartData->strip);
	    bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
		preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
	    stripChartData->strip->axesFontStruct = fontTable[bestSize];
	    stripChartData->strip->axesFontHeight = usedHeight;
	    stripChartData->strip->axesFontWidth = usedCharWidth;

	    stripChartData->strip->setOwnColors = TRUE;
/* all the monitors point to same DlStripChart structure, so pick arbitrarily */
	    monitorData = savedMonitorData;
	    dlStripChart = (DlStripChart *)monitorData->specifics;
	    stripChartData->strip->forePixel = 
		monitorData->displayInfo->dlColormap[dlStripChart->plotcom.clr];
	    stripChartData->strip->backPixel = 
		monitorData->displayInfo->dlColormap[
			dlStripChart->plotcom.bclr];
/* let user data be the stripChartData aux. structure */
	    stripChartData->strip->userData = (XtPointer) stripChartData;

	    stripSet(stripChartData->strip,stripChartData->nChannels,
		STRIP_NSAMPLES,NULL,stripRange,
		stripCalculateInterval(dlStripChart->delay,dlStripChart->units),
		stripGetValueArray,
		(dlStripChart->plotcom.title == NULL
			? " " : dlStripChart->plotcom.title),NULL,
		(dlStripChart->plotcom.xlabel == NULL
			? " " : dlStripChart->plotcom.xlabel),
		(dlStripChart->plotcom.ylabel == NULL
			? " " : dlStripChart->plotcom.ylabel),
		NULL,NULL,NULL,NULL,StripInternal);
	    stripSetAppContext(stripChartData->strip,appContext);
	    for (i = 0; i < stripChartData->nChannels; i++)
		stripChartData->strip->dataPixel[i] = 
			monitorData->displayInfo->dlColormap[
				dlStripChart->pen[i].clr];
	    stripDraw(stripChartData->strip);
/* add expose and destroy callbacks */
	    XtAddCallback(monitorData->self,XmNexposeCallback,
		(XtCallbackProc)redisplayStrip,
		(XtPointer)&stripChartData->strip);
	    
	  for (i = 0; i < stripChartData->nChannels; i++) {
	   monitorData = (ChannelAccessMonitorData*)stripChartData->monitors[i];
	   if (monitorData != NULL)
/* register a STS type event handler */
		SEVCHK(ca_add_event(stsChannelType,
		    monitorData->chid,
		    processMonitorEvent,NULL,
		    &(monitorData->evid)),
		    "\nprocessMonitorGrGetCallback: error in ca_add_event");
	  }
/* add destroy callback */
/* not freeing monitorData here, rather using monitorData for
   it's information */
	  XtAddCallback(savedMonitorData->self,XmNdestroyCallback,
		(XtCallbackProc)monitorDestroy, (XtPointer)stripChartData);
	}

	break;



/******************/
/* CARTESIAN PLOT */
/******************/
/* lots of special handling for cartesian plot */
    case DL_CartesianPlot:

	xrtData1 = xrtData2 = NULL;

	widget = (Widget)channelAccessMonitorData->self;
	XtSetArg(widgetArgs[0],XmNuserData,&userData);
	XtGetValues(widget,widgetArgs,1);
	cartesianPlotData = (CartesianPlotData *) userData;

/* MDA - okay I hate to do this but the nesting of ifs is getting out of hand */
	if (cartesianPlotData == (CartesianPlotData *)NULL) {
	  fprintf(stderr,
	 "\nprocessMonitorGrGetCallback: cartesianPlotData == NULL, returning");
	  return;
	}

/* if a trigger channel was specified and this is it, then add monitor for it
   and break out */
	if (cartesianPlotData->triggerMonitorData != NULL) {
	  if (channelAccessMonitorData ==
	    (ChannelAccessMonitorData *)cartesianPlotData->triggerMonitorData) {
	    SEVCHK(ca_add_event(stsChannelType,channelAccessMonitorData->chid,
		processMonitorEvent,NULL,&(channelAccessMonitorData->evid)),
		"\nprocessMonitorGrGetCallback: error in ca_add_array_event");
	    return;
	  }
	}

/*
 *  check to see if all component channels of cartesian plot have connected.
 *  if so, proceed with plot initialization (keep a valid monitorData too)
 */
	allConnected = TRUE;
	for (i = 0; i < cartesianPlotData->nTraces; i++) {
	   monitorData = (ChannelAccessMonitorData *)
			cartesianPlotData->monitors[i][0];
	   if (monitorData != NULL) {
		allConnected = (allConnected & 
			(monitorData->modified == PRIMARY_MODIFIED));
		savedMonitorData = monitorData;
	   }

	   monitorData = (ChannelAccessMonitorData*)
			cartesianPlotData->monitors[i][1];
	   if (monitorData != NULL) {
		allConnected = (allConnected & 
			(monitorData->modified == PRIMARY_MODIFIED));
		savedMonitorData = monitorData;
	   }
	}
/* we came in only knowing about one monitor/channel, but since all
   are connected, we can proceed, now knowing everything via 
   cartesianPlotData->monitors[][] (since, for instance chid is in there)
 */
	monitorData = savedMonitorData;
	if (allConnected) {
/* break out if no valid monitorData's found */
	    if (monitorData == NULL) break;

/* initialize min/max to some other-end-of-the-range values */
	    minX = minY = minY2 = FLT_MAX;
	    maxX = maxY = maxY2 = -0.99*FLT_MAX;

	    dlCartesianPlot = (DlCartesianPlot *)monitorData->specifics;
	    for (i = 0; i < cartesianPlotData->nTraces; i++)
		xColors[i].pixel = monitorData->displayInfo->dlColormap[
			dlCartesianPlot->trace[i].data_clr];
	    XQueryColors(display,cmap,xColors,cartesianPlotData->nTraces);
	    for (i = 0; i < cartesianPlotData->nTraces; i++) {
		sprintf(rgb[i],"#%2.2x%2.2x%2.2x", xColors[i].red>>8,
				xColors[i].green>>8, xColors[i].blue>>8);
	    }
/*
 * now create XrtData
 */
/* allocate XrtData structures */

/* loop over monitors, extracting information */
	    maxElements = dlCartesianPlot->count;
	    for (i = 0; i < cartesianPlotData->nTraces; i++) {
	/* trace i, X data */
	       monitorDataX = (ChannelAccessMonitorData *)
			cartesianPlotData->monitors[i][0];
	       if (monitorDataX != NULL) xChid = monitorDataX->chid;
	       else xChid = NULL;
	/* trace i, Y data */
	       monitorDataY = (ChannelAccessMonitorData*)
			cartesianPlotData->monitors[i][1];
	       if (monitorDataY != NULL) yChid = monitorDataY->chid;
	       else yChid = NULL;

	       if (xChid != NULL)
		   maxElements = MAX(maxElements,(int)ca_element_count(xChid));
	       if (yChid != NULL)
		   maxElements = MAX(maxElements,(int)ca_element_count(yChid));
	    }

	    xrtData1 = XrtMakeData(XRT_GENERAL,1,maxElements,TRUE);
/* now 2nd xrt data set if appropriate */
	    if (cartesianPlotData->nTraces > 1) {
	      xrtData2 = XrtMakeData(XRT_GENERAL,cartesianPlotData->nTraces-1,
			maxElements,TRUE);
	    }
	    

/* loop over monitors, extracting information */
	    for (i = 0; i < cartesianPlotData->nTraces; i++) {
	/* trace i, X data */
	       monitorDataX = (ChannelAccessMonitorData *)
			cartesianPlotData->monitors[i][0];
	       if (monitorDataX != NULL) xChid = monitorDataX->chid;
	       else xChid = NULL;
	/* trace i, Y data */
	       monitorDataY = (ChannelAccessMonitorData*)
			cartesianPlotData->monitors[i][1];
	       if (monitorDataY != NULL) yChid = monitorDataY->chid;
	       else yChid = NULL;
	/*
	 * now process data type (based on type (scalar or vector) of data)
	 */
	       if (xChid != NULL && yChid != NULL) {

		 if ( ca_element_count(xChid) > 1 &&
					ca_element_count(yChid) > 1 ) {
		     monitorDataX->xyChannelType = CP_XYVectorX;
		     monitorDataY->xyChannelType = CP_XYVectorY;
		 } else if ( ca_element_count(xChid) > 1 &&
					ca_element_count(yChid) <= 1 ) {
		     monitorDataX->xyChannelType = CP_XVectorYScalarX;
		     monitorDataY->xyChannelType = CP_XVectorYScalarY;
		 } else if ( ca_element_count(yChid) > 1 &&
					ca_element_count(xChid) <= 1 ) {
		     monitorDataX->xyChannelType = CP_YVectorXScalarX;
		     monitorDataY->xyChannelType = CP_YVectorXScalarY;
		 } else if ( ca_element_count(xChid) <= 1 &&
					ca_element_count(yChid) <= 1 ) {
		     monitorDataX->xyChannelType = CP_XYScalarX;
		     monitorDataY->xyChannelType = CP_XYScalarY;
		 }

		 monitorDataX->other = monitorDataY;
		 monitorDataY->other = monitorDataX;
	    /* put initial data in */
		 if (i <= 0) {
		   xrtData1->g.data[i].npoints = 0;
		   minY = MIN(minY,monitorDataY->lopr);
		   maxY = MAX(maxY,monitorDataY->hopr);
		 } else {
		   xrtData2->g.data[i-1].npoints = 0;
		   minY2 = MIN(minY2,monitorDataY->lopr);
		   maxY2 = MAX(maxY2,monitorDataY->hopr);
		 }
		 minX = MIN(minX,monitorDataX->lopr);
		 maxX = MAX(maxX,monitorDataX->hopr);

	       } else if (xChid != NULL && yChid == NULL) {
	/* X channel - supporting scalar or waveform */
		 if (ca_element_count(xChid) > 1) {
		/* vector/waveform */
		  monitorDataX->xyChannelType = CP_XVector;
		  if (i <= 0) {
		     for(j = 0; j < (int)ca_element_count(xChid); j++)
			 	xrtData1->g.data[i].yp[j]= (float)j;
		     minY = MIN(minY,0.);
		     maxY = MAX(maxY,(float)((int)ca_element_count(xChid)-1));
		  } else {
		     for(j = 0; j < (int)ca_element_count(xChid); j++)
				xrtData2->g.data[i-1].yp[j]= (float)j;
		     minY2 = MIN(minY2,0.);
		     maxY2 = MAX(maxY2,(float)ca_element_count(xChid));
		  }
		  minX = MIN(minX,monitorDataX->lopr);
		  maxX = MAX(maxX,monitorDataX->hopr);
		 } else {
		/* scalar */
		  monitorDataX->xyChannelType = CP_XScalar;
		  if (i <= 0) {
		     for(j = 0; j < dlCartesianPlot->count; j++)
				xrtData1->g.data[i].yp[j]= (float)j;
		     xrtData1->g.data[i].xp[0]= (float)monitorDataX->value;
		     minY = MIN(minY,0.);
		     maxY = MAX(maxY,(float)dlCartesianPlot->count);
		  } else {
		     for(j = 0; j < dlCartesianPlot->count; j++)
				xrtData2->g.data[i-1].yp[j]= (float)j;
		     xrtData2->g.data[i-1].xp[0]= (float)monitorDataX->value;
		     minY2 = MIN(minY2,0.);
		     maxY2 = MAX(maxY2,(float)dlCartesianPlot->count);
		  }
		  minX = MIN(minX,monitorDataX->lopr);
		  maxX = MAX(maxX,monitorDataX->hopr);
		 }
	       } else if (xChid == NULL && yChid != NULL) {
	/* Y channel - supporting scalar or waveform */
		 if (ca_element_count(yChid) > 1) {
		/* vector/waveform */
		  monitorDataY->xyChannelType = CP_YVector;
		  if (i <= 0) {
		     for(j = 0; j < (int)ca_element_count(yChid); j++)
				xrtData1->g.data[i].xp[j]= (float)j;
		     minY = MIN(minY,monitorDataY->lopr);
		     maxY = MAX(maxY,monitorDataY->hopr);
		  } else {
		     for(j = 0; j < (int)ca_element_count(yChid); j++)
				xrtData2->g.data[i-1].xp[j]= (float)j;
		     minY2 = MIN(minY2,monitorDataY->lopr);
		     maxY2 = MAX(maxY2,monitorDataY->hopr);
		  }
		  minX = MIN(minX,0.);
		  maxX = MAX(maxX,(float)((int)ca_element_count(yChid)-1));
		 } else {
		/* scalar */
		  monitorDataY->xyChannelType = CP_YScalar;
		  if (i <= 0) {
		     for(j = 0; j < dlCartesianPlot->count; j++)
				xrtData1->g.data[i].xp[j]= (float)j;
		     xrtData1->g.data[i].yp[0]= (float)monitorDataY->value;
		     minY = MIN(minY,monitorDataY->lopr);
		     maxY = MAX(maxY,monitorDataY->hopr);
		  } else {
		     for(j = 0; j < dlCartesianPlot->count; j++)
				xrtData2->g.data[i-1].xp[j]= (float)j;
		     xrtData2->g.data[i-1].yp[0]= (float)monitorDataY->value;
		     minY2 = MIN(minY2,monitorDataY->lopr);
		     maxY2 = MAX(maxY2,monitorDataY->hopr);
		  }
		  minX = MIN(minX,0.);
		  maxX = MAX(maxX,(float)dlCartesianPlot->count);
		 }
	       }	/* end for */
	    }
	    monitorData = savedMonitorData;

/*
 * now set XrtDataStyle
 */

/* initialize XrtDataStyle array */
	    for (i = 0; i < MAX_TRACES; i++) {
	/* there is really only going to be one myds1 in a graph */
		myds1[i].lpat = XRT_LPAT_SOLID;
		myds1[i].fpat = XRT_FPAT_SOLID;
		myds1[i].color = rgb[i];
		myds1[i].width = 1;
		myds1[i].point = XRT_POINT_DOT;
		myds1[i].pcolor = rgb[i];
		myds1[i].psize = MAX(2,dlCartesianPlot->object.height/70);
		myds1[i].res1 = 0;
		myds1[i].res1 = 0;

		myds2[i].lpat = XRT_LPAT_SOLID;
		myds2[i].fpat = XRT_FPAT_SOLID + i;
		myds2[i].color = rgb[i];
		myds2[i].width = 1;
		myds2[i].point = XRT_POINT_BOX+i;
		myds2[i].pcolor = rgb[i];
		myds2[i].psize = MAX(2,dlCartesianPlot->object.height/70);
		myds2[i].res1 = 0;
		myds2[i].res1 = 0;
	    }


/* loop over each trace and set xrtData attributes */
	    for (i = 0; i < cartesianPlotData->nTraces; i++) {
		monitorData = (ChannelAccessMonitorData *)
		  cartesianPlotData->monitors[i][0];
		if (monitorData == NULL) monitorData =
		  (ChannelAccessMonitorData*)cartesianPlotData->monitors[i][1];
		if (monitorData != NULL) {

		  switch(dlCartesianPlot->style) {
		    case POINT_PLOT:
			if (i <= 0) {
			  myds1[i].lpat = XRT_LPAT_NONE;
			  myds1[i].fpat = XRT_FPAT_NONE;
			  myds1[i].color = rgb[i];
			  myds1[i].pcolor = rgb[i];
			  myds1[i].psize = MAX(2,
				dlCartesianPlot->object.height/70);
			  XrtSetNthDataStyle(monitorData->self,i,&myds1[i]);
			} else {
			  myds2[i-1].lpat = XRT_LPAT_NONE;
			  myds2[i-1].fpat = XRT_FPAT_NONE;
			  myds2[i-1].color = rgb[i];
			  myds2[i-1].pcolor = rgb[i];
			  myds2[i-1].psize = MAX(2,
				dlCartesianPlot->object.height/70);
			  XrtSetNthDataStyle2(monitorData->self,
				i-1,&myds2[i-1]);
			}
			break;
		    case LINE_PLOT:
			if (i <= 0) {
			  myds1[i].fpat = XRT_FPAT_NONE;
			  myds1[i].color = rgb[i];
			  myds1[i].pcolor = rgb[i];
			  myds1[i].psize = MAX(2,
				dlCartesianPlot->object.height/70);
			  XrtSetNthDataStyle(monitorData->self,i,&myds1[i]);
			} else {
			  myds2[i-1].fpat = XRT_FPAT_NONE;
			  myds2[i-1].color = rgb[i];
			  myds2[i-1].pcolor = rgb[i];
			  myds2[i-1].psize = MAX(2,
				dlCartesianPlot->object.height/70);
			  XrtSetNthDataStyle2(monitorData->self,
				i-1,&myds2[i-1]);
			}
			break;
		    case FILL_UNDER_PLOT:
			if (i <= 0) {
			  myds1[i].color = rgb[i];
			  myds1[i].pcolor = rgb[i];
			  myds1[i].psize = MAX(2,
				dlCartesianPlot->object.height/70);
			  XrtSetNthDataStyle(monitorData->self,i,&myds1[i]);
			} else {
			  myds2[i-1].color = rgb[i];
			  myds2[i-1].pcolor = rgb[i];
			  myds2[i-1].psize = MAX(2,
				dlCartesianPlot->object.height/70);
			  XrtSetNthDataStyle2(monitorData->self,
				i-1,&myds2[i-1]);
			}
			break;
		  }
		}
	    }

	    monitorData = savedMonitorData;

/*
 * now register event handlers for vectors of channels/monitors
 */
	   for (i = 0; i < cartesianPlotData->nTraces; i++) {
		monitorData = (ChannelAccessMonitorData *)
			cartesianPlotData->monitors[i][0];
		if (monitorData != NULL) {
		    if ( i <= 0) {
			monitorData->xrtData = xrtData1;
			monitorData->xrtDataSet = 1;
			monitorData->trace = i;
		    } else {
			monitorData->xrtData = xrtData2;
			monitorData->xrtDataSet = 2;
			monitorData->trace = i-1;
		    }
		    SEVCHK(ca_add_array_event(stsChannelType,
			ca_element_count(monitorData->chid),
			monitorData->chid,
			processMonitorEvent,
			NULL,
			0.0,0.0,0.0,
			&(monitorData->evid)),
		"\nprocessMonitorGrGetCallback: error in ca_add_array_event");
		}
		monitorData = (ChannelAccessMonitorData*)
			cartesianPlotData->monitors[i][1];
		if (monitorData != NULL) {
		    if ( i <= 0) {
			monitorData->xrtData = xrtData1;
			monitorData->xrtDataSet = 1;
			monitorData->trace = i;
		    } else {
			monitorData->xrtData = xrtData2;
			monitorData->xrtDataSet = 2;
			monitorData->trace = i-1;
		    }
		    SEVCHK(ca_add_array_event(stsChannelType,
			ca_element_count(monitorData->chid),
			monitorData->chid,
			processMonitorEvent,
			NULL,
			0.0,0.0,0.0,
			&(monitorData->evid)),
		"\nprocessMonitorGrGetCallback: error in ca_add_array_event");
		}
	   }

	/* fill in connect-time channel-based range specifications - NB:
	 *  this is different than the min/max stored in the display element
	 */
	   cartesianPlotData->axisRange[X_AXIS_ELEMENT].axisMin = minX;
	   cartesianPlotData->axisRange[X_AXIS_ELEMENT].axisMax = maxX;
	   cartesianPlotData->axisRange[Y1_AXIS_ELEMENT].axisMin = minY;
	   cartesianPlotData->axisRange[Y1_AXIS_ELEMENT].axisMax = maxY;
	   cartesianPlotData->axisRange[Y2_AXIS_ELEMENT].axisMin = minY2;
	   cartesianPlotData->axisRange[Y2_AXIS_ELEMENT].axisMax = maxY2;
	   for (kk = X_AXIS_ELEMENT; kk <= Y2_AXIS_ELEMENT; kk++) {
	    if (dlCartesianPlot->axis[kk].rangeStyle==CHANNEL_RANGE) {
	      cartesianPlotData->axisRange[kk].isCurrentlyFromChannel= True;
	    } else {
	      cartesianPlotData->axisRange[kk].isCurrentlyFromChannel = False;
	    }
	   }
	   minXF.fval = minX;
	   maxXF.fval = maxX;
	   minYF.fval = minY;
	   maxYF.fval = maxY;
	   minY2F.fval = minY2;
	   maxY2F.fval = maxY2;
	   n = 0;
	   if (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle
					== CHANNEL_RANGE) {

	     XtSetArg(widgetArgs[n],XtNxrtXMin,minXF.lval); n++;
	     XtSetArg(widgetArgs[n],XtNxrtXMax,maxXF.lval); n++;

	   } else if (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle
					== USER_SPECIFIED_RANGE) {

	     minXF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].minRange;
	     maxXF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].maxRange;

	     tickF.fval = (maxXF.fval - minXF.fval)/4.0;
	     XtSetArg(widgetArgs[n],XtNxrtXMin,minXF.lval); n++;
	     XtSetArg(widgetArgs[n],XtNxrtXMax,maxXF.lval); n++;
	     XtSetArg(widgetArgs[n],XtNxrtXTick,tickF.lval); n++;
	     XtSetArg(widgetArgs[n],XtNxrtXNum,tickF.lval); n++;
	     sprintf(string,"%f",tickF.fval);
	     k = strlen(string)-1;
	     while (string[k] == '0') k--; /* strip off trailing zeroes */
	     iPrec = k;
	     while (string[k] != '.' && k >= 0) k--;
	     iPrec = iPrec - k;
	     XtSetArg(widgetArgs[n],XtNxrtXPrecision,iPrec); n++;

	   }

	   if (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle
					== CHANNEL_RANGE) {

	     XtSetArg(widgetArgs[n],XtNxrtYMin,minYF.lval); n++;
	     XtSetArg(widgetArgs[n],XtNxrtYMax,maxYF.lval); n++;

	   } else if (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle
					== USER_SPECIFIED_RANGE) {

	     minYF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].minRange;
	     maxYF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange;

	     tickF.fval = (maxYF.fval - minYF.fval)/4.0;
	     XtSetArg(widgetArgs[n],XtNxrtYMin,minYF.lval); n++;
	     XtSetArg(widgetArgs[n],XtNxrtYMax,maxYF.lval); n++;
	     XtSetArg(widgetArgs[n],XtNxrtYTick,tickF.lval); n++;
	     XtSetArg(widgetArgs[n],XtNxrtYNum,tickF.lval); n++;
	     sprintf(string,"%f",tickF.fval);
	     k = strlen(string)-1;
	     while (string[k] == '0') k--; /* strip off trailing zeroes */
	     iPrec = k;
	     while (string[k] != '.' && k >= 0) k--;
	     iPrec = iPrec - k;
	     XtSetArg(widgetArgs[n],XtNxrtYPrecision,iPrec); n++;

	   }

	   if (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle
					== CHANNEL_RANGE) {
	     XtSetArg(widgetArgs[n],XtNxrtY2Min,minY2F.lval); n++;
	     XtSetArg(widgetArgs[n],XtNxrtY2Max,maxY2F.lval); n++;

	   } else if (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle
					== USER_SPECIFIED_RANGE) {

	     minY2F.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].minRange;
	     maxY2F.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].maxRange;

	     tickF.fval = (maxY2F.fval - minY2F.fval)/4.0;
	     XtSetArg(widgetArgs[n],XtNxrtY2Min,minY2F.lval); n++;
	     XtSetArg(widgetArgs[n],XtNxrtY2Max,maxY2F.lval); n++;
	     XtSetArg(widgetArgs[n],XtNxrtY2Tick,tickF.lval); n++;
	     XtSetArg(widgetArgs[n],XtNxrtY2Num,tickF.lval); n++;
	     sprintf(string,"%f",tickF.fval);
	     k = strlen(string)-1;
	     while (string[k] == '0') k--; /* strip off trailing zeroes */
	     iPrec = k;
	     while (string[k] != '.' && k >= 0) k--;
	     iPrec = iPrec - k;
	     XtSetArg(widgetArgs[n],XtNxrtY2Precision,iPrec); n++;

	   }


	   XtSetValues(savedMonitorData->self,widgetArgs,n);

/* add destroy callback */
/* not freeing monitorData here, rather using monitorData for
   it's information */
	  cartesianPlotData->xrtData1 = xrtData1;
	  cartesianPlotData->xrtData2 = xrtData2;
	  XtAddCallback(savedMonitorData->self,XmNdestroyCallback,
		(XtCallbackProc)monitorDestroy, (XtPointer)cartesianPlotData);
/*
 * don't set XrtData until monitors fire and call for update
 */
	    
	} /* end if (allConnected) ?  {hopefully!}  */
	
	break;

    default:
	break;
  }


/* if a scalar monitor type (e.g., not StripChart or Cartesian Plot) then
 *  use this common place to add monitor
 */
  if (channelAccessMonitorData->monitorType != DL_StripChart &&
      channelAccessMonitorData->monitorType != DL_CartesianPlot) {
/* register a STS type event handler */
    SEVCHK(ca_add_event(stsChannelType,
	channelAccessMonitorData->chid,
	processMonitorEvent,NULL,
	&(channelAccessMonitorData->evid)),
	"\nprocessMonitorGrGetCallback: error in ca_add_array_event");
  }

}  

     

/*
 * status event handler - when the channel changes, store the new value
 *   and alarm data and update the modified field in the 
 *   ChannelAccessMonitorData structure
 */
void processMonitorEvent(struct event_handler_args args)
{
  ChannelAccessMonitorData *channelAccessMonitorData = ca_puser(args.chid);
  struct dbr_sts_string  *sstData;
  struct dbr_sts_int    *ciData;
  struct dbr_sts_float  *cfData;
  struct dbr_sts_enum   *ceData;
  struct dbr_sts_char   *ccData;
  struct dbr_sts_long   *clData;
  struct dbr_sts_double *cdData;
  DlCartesianPlot	*dlCartesianPlot;
  float f_value;
  int j, trace, count, loopCount;
  unsigned char *pChar;
  short *pShort;
  unsigned short *pUnsignedShort;
  long *pLong;
  float *pFloat;
  double *pDouble;

  Arg widgetArgs[2];
  XtPointer userData;
  CartesianPlotData *cartesianPlotData;


if (globalDisplayListTraversalMode != DL_EXECUTE) return;
globalModifiedFlag = True;
if (channelAccessMonitorData == NULL) return;
/* use displayInfo->drawingArea == NULL as sign of "in cleanup", hence ignore */
if (channelAccessMonitorData->displayInfo == NULL) return;
if (channelAccessMonitorData->displayInfo->drawingArea == NULL) return;

channelAccessMonitorData->modified = PRIMARY_MODIFIED;

/*
 *  cartesian plot should be the only element type that requested vector data
 *   therefore all scalar data should be worth updating  in *MonitorData 
 *	(note that even for cartesian plots where scalars are plotted, this
 *	 mechanism should work - since vector vs. scalar is determined at
 *	 run-time alone)
 */

  if (args.count == 1) {

/* store value and alarm data in ChannelAccessMonitorData structure */
    switch(args.type) {
	case DBR_STS_STRING:
		sstData = (struct dbr_sts_string *) args.dbr;
		strcpy(channelAccessMonitorData->oldStringValue,
			channelAccessMonitorData->stringValue);
		strcpy(channelAccessMonitorData->stringValue,sstData->value);
		channelAccessMonitorData->status = sstData->status;
		channelAccessMonitorData->oldSeverity = 
			channelAccessMonitorData->severity;
		channelAccessMonitorData->severity = sstData->severity;
		break;
	case DBR_STS_INT:
/*	case DBR_STS_SHORT: -- INT and SHORT are same thing in db_access.h */
		ciData = (struct dbr_sts_int *) args.dbr;
		channelAccessMonitorData->oldValue =
			channelAccessMonitorData->value;
		channelAccessMonitorData->value = (double) ciData->value;
		channelAccessMonitorData->status = ciData->status;
		channelAccessMonitorData->oldSeverity =
			channelAccessMonitorData->severity;
		channelAccessMonitorData->severity =  ciData->severity;
		break;
	case DBR_STS_FLOAT:
		cfData = (struct dbr_sts_float *) args.dbr;
		channelAccessMonitorData->oldValue =
			channelAccessMonitorData->value;
		channelAccessMonitorData->value = (double) cfData->value;
		channelAccessMonitorData->status = cfData->status;
		channelAccessMonitorData->oldSeverity =
			channelAccessMonitorData->severity;
		channelAccessMonitorData->severity =  cfData->severity;
		break;
	case DBR_STS_ENUM:
		ceData = (struct dbr_sts_enum *) args.dbr;
		channelAccessMonitorData->oldValue =
			channelAccessMonitorData->value;
		channelAccessMonitorData->value = (double) ceData->value;
		channelAccessMonitorData->status = ceData->status;
		channelAccessMonitorData->oldSeverity = 
			channelAccessMonitorData->severity;
		channelAccessMonitorData->severity =  ceData->severity;
		break;
	case DBR_STS_CHAR:
		ccData = (struct dbr_sts_char *) args.dbr;
		channelAccessMonitorData->oldValue =
			channelAccessMonitorData->value;
		channelAccessMonitorData->value = (double) ccData->value;
		channelAccessMonitorData->status = ccData->status;
		channelAccessMonitorData->oldSeverity =
			channelAccessMonitorData->severity;
		channelAccessMonitorData->severity =  ccData->severity;
		break;
	case DBR_STS_LONG:
		clData = (struct dbr_sts_long *) args.dbr;
		channelAccessMonitorData->oldValue =
			channelAccessMonitorData->value;
		channelAccessMonitorData->value = (double) clData->value;
		channelAccessMonitorData->status = clData->status;
		channelAccessMonitorData->oldSeverity = 
			channelAccessMonitorData->severity;
		channelAccessMonitorData->severity =  clData->severity;
		break;
	case DBR_STS_DOUBLE:
		cdData = (struct dbr_sts_double *) args.dbr;
		channelAccessMonitorData->oldValue =
			channelAccessMonitorData->value;
		channelAccessMonitorData->value = (double) cdData->value;
		channelAccessMonitorData->status = cdData->status;
		channelAccessMonitorData->oldSeverity = 
			channelAccessMonitorData->severity;
		channelAccessMonitorData->severity =  cdData->severity;
		break;

	default:
		fprintf(stderr,
		  "\nprocessMonitorEvent: unknown field type = %d",args.type);
		return;
    }
  }


/****
 **** now put value into vectors for cartesian plots
 ****/

  if (channelAccessMonitorData->monitorType == DL_CartesianPlot) {

/*
 * MDA - handle trigger channel here - only set modified on trigger if trigger
 *	is used, but always update the data for the other channels
 */
    XtSetArg(widgetArgs[0],XmNuserData,&userData);
    XtGetValues((Widget)channelAccessMonitorData->self,widgetArgs,1);
    cartesianPlotData = (CartesianPlotData *) userData;
    if (cartesianPlotData == (CartesianPlotData *)NULL) {
	fprintf(stderr,"\nprocessMonitorEvent: cartesianPlotData == NULL");
	return;
    }

    if (cartesianPlotData->triggerMonitorData != NULL) {
      if (channelAccessMonitorData ==
	  (ChannelAccessMonitorData *)cartesianPlotData->triggerMonitorData) {
	  channelAccessMonitorData->modified = PRIMARY_MODIFIED;
/* since no other work to do for this monitor - data is being filled in
   elsewhere and this channel only used for triggering update... */
	  return;
      } else
/* since this monitor fired but there is a trigger channel, set modified
 *   for this data to let us know it needs to be updated, but that we
 *   are waiting on the trigger channel to do the update
 */
	  channelAccessMonitorData->modified = SECONDARY_MODIFIED;
    }


    dlCartesianPlot = (DlCartesianPlot *)channelAccessMonitorData->specifics;

    switch(channelAccessMonitorData->xyChannelType) {
      case CP_XYScalarX:
 /* X: x & y channels specified - scalars, up to dlCartesianPlot->count pairs */
	trace = channelAccessMonitorData->trace;
	count = channelAccessMonitorData->xrtData->g.data[trace].npoints;
	if (count <= dlCartesianPlot->count-1) {
	  channelAccessMonitorData->xrtData->g.data[trace].xp[count] =
		(float) channelAccessMonitorData->value;
	  channelAccessMonitorData->xrtData->g.data[trace].yp[count] =
		(float) channelAccessMonitorData->other->value;
	  channelAccessMonitorData->xrtData->g.data[trace].npoints++;
	} else if (count > dlCartesianPlot->count-1) {
	  if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
	/* cancel monitors for both channels */
	      if (channelAccessMonitorData->evid != NULL) {
		SEVCHK(ca_clear_event(channelAccessMonitorData->evid),
			"\nprocessMonitorEvent: error in ca_clear_event");
		channelAccessMonitorData->evid = NULL;
	      }
	      if (channelAccessMonitorData->other->evid != NULL) {
		SEVCHK(ca_clear_event(channelAccessMonitorData->other->evid),
			"\nprocessMonitorEvent: error in ca_clear_event");
		channelAccessMonitorData->other->evid = NULL;
	      }
	  } else if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
	/* shift everybody down one, add at end */
	      for (j = 1; j < dlCartesianPlot->count; j++) {
		channelAccessMonitorData->xrtData->g.data[trace].xp[j-1] =
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j];
		channelAccessMonitorData->xrtData->g.data[trace].yp[j-1] =
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j];
	      }
	      channelAccessMonitorData->xrtData->g.data[trace].xp[j-1] =
		     (float) channelAccessMonitorData->value;
	      channelAccessMonitorData->xrtData->g.data[trace].yp[j-1] =
		     (float) channelAccessMonitorData->other->value;
	      channelAccessMonitorData->xrtData->g.data[trace].npoints =
		     dlCartesianPlot->count;
	  }
	}
	break;

      case CP_XYScalarY:
 /* Y: x & y channels specified - scalars, up to dlCartesianPlot->count pairs */
	trace = channelAccessMonitorData->trace;
	count = channelAccessMonitorData->xrtData->g.data[trace].npoints;
	if (count <= dlCartesianPlot->count-1) {
	  channelAccessMonitorData->xrtData->g.data[trace].yp[count] =
		(float) channelAccessMonitorData->value;
	  channelAccessMonitorData->xrtData->g.data[trace].xp[count] =
		(float) channelAccessMonitorData->other->value;
	  channelAccessMonitorData->xrtData->g.data[trace].npoints++;
	} else if (count > dlCartesianPlot->count-1) {
	  if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
	/* cancel monitors for both channels */
	      if (channelAccessMonitorData->evid != NULL) {
		SEVCHK(ca_clear_event(channelAccessMonitorData->evid),
			"\nprocessMonitorEvent: error in ca_clear_event");
		channelAccessMonitorData->evid = NULL;
	      }
	      if (channelAccessMonitorData->other->evid != NULL) {
		SEVCHK(ca_clear_event(channelAccessMonitorData->other->evid),
		"\nprocessMonitorEvent: error in ca_clear_event");
		channelAccessMonitorData->other->evid = NULL;
	      }
	  } else if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
	/* shift everybody down one, add at end */
	      for (j = 1; j < dlCartesianPlot->count; j++) {
		channelAccessMonitorData->xrtData->g.data[trace].yp[j-1] =
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j];
		channelAccessMonitorData->xrtData->g.data[trace].xp[j-1] =
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j];
	      }
	      channelAccessMonitorData->xrtData->g.data[trace].yp[j-1] =
		     (float) channelAccessMonitorData->value;
	      channelAccessMonitorData->xrtData->g.data[trace].xp[j-1] =
		     (float) channelAccessMonitorData->other->value;
	  }
	}
	break;



      case CP_XScalar:
 /* x channel scalar, up to dlCartesianPlot->count pairs */
	trace = channelAccessMonitorData->trace;
	count = channelAccessMonitorData->xrtData->g.data[trace].npoints;
	if (count <= dlCartesianPlot->count-1) {
	  channelAccessMonitorData->xrtData->g.data[trace].xp[count] =
		(float) channelAccessMonitorData->value;
	  channelAccessMonitorData->xrtData->g.data[trace].yp[count] =
		(float) count;
	  channelAccessMonitorData->xrtData->g.data[trace].npoints++;
	} else if (count > dlCartesianPlot->count-1) {
	  if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
	/* cancel monitors for this channels */
	      if (channelAccessMonitorData->evid != NULL) {
	        SEVCHK(ca_clear_event(channelAccessMonitorData->evid),
			"\nprocessMonitorEvent: error in ca_clear_event");
		channelAccessMonitorData->evid = NULL;
	      }
	  } else if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
	/* shift everybody down one, add at end */
	      for (j = 1; j < dlCartesianPlot->count; j++) {
		channelAccessMonitorData->xrtData->g.data[trace].xp[j-1] =
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j];
		channelAccessMonitorData->xrtData->g.data[trace].yp[j-1] =
		     (float) (j-1);
	      }
	      channelAccessMonitorData->xrtData->g.data[trace].xp[j-1] =
		     (float) channelAccessMonitorData->value;
	      channelAccessMonitorData->xrtData->g.data[trace].yp[j-1] =
		     (float) (j-1);
	  }
	}
	break;


      case CP_XVector:
 /* x channel vector, ca_element_count(chid) elements */
	trace = channelAccessMonitorData->trace;
	loopCount = ca_element_count(channelAccessMonitorData->chid);

	/* plot first "count" elements of vector per dlCartesianPlot*/
	switch(args.type) {
	    case DBR_STS_STRING:
		  fprintf(stderr,"\nprocessMonitorEvent: %s: %s",
			"illegal field type for Cartesian Plot channel",
			ca_name(channelAccessMonitorData->chid));
		  f_value = 0.0;
		  for (j = 0; j < loopCount; j++) {
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			(float) j;
		  }
		  break;
	    case DBR_STS_INT:
/*	case DBR_STS_SHORT: -- INT and SHORT are same thing in db_access.h */
		  ciData = (struct dbr_sts_int *) args.dbr;
		  pShort = &ciData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pShort[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			(float) j;
		  }
		  break;
	    case DBR_STS_FLOAT:
		  cfData = (struct dbr_sts_float *) args.dbr;
		  pFloat = &cfData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pFloat[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			(float) j;
		  }
		  break;
	    case DBR_STS_ENUM:
		  ceData = (struct dbr_sts_enum *) args.dbr;
		  pUnsignedShort = &ceData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pUnsignedShort[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			(float) j;
		  }
		  break;
	    case DBR_STS_CHAR:
		  ccData = (struct dbr_sts_char *) args.dbr;
		  pChar = &ccData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pChar[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			(float) j;
		  }
		  break;
	    case DBR_STS_LONG:
		  clData = (struct dbr_sts_long *) args.dbr;
		  pLong = &clData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pLong[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
		 	f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			(float) j;
		  }
		  break;
	    case DBR_STS_DOUBLE:
		  cdData = (struct dbr_sts_double *) args.dbr;
		  pDouble = &cdData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pDouble[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			(float) j;
		  }
		  break;
	}
	channelAccessMonitorData->xrtData->g.data[trace].npoints = loopCount;
	break;


      case CP_YScalar:
 /* y channel scalar, up to dlCartesianPlot->count pairs */
	trace = channelAccessMonitorData->trace;
	count = channelAccessMonitorData->xrtData->g.data[trace].npoints;
	if (count <= dlCartesianPlot->count-1) {
	  channelAccessMonitorData->xrtData->g.data[trace].yp[count] =
		(float) channelAccessMonitorData->value;
	  channelAccessMonitorData->xrtData->g.data[trace].xp[count] =
		(float) count;
	  channelAccessMonitorData->xrtData->g.data[trace].npoints++;
	} else if (count > dlCartesianPlot->count-1) {
	  if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
	/* cancel monitors for this channels */
	      if (channelAccessMonitorData->evid != NULL) {
	        SEVCHK(ca_clear_event(channelAccessMonitorData->evid),
			"\nprocessMonitorEvent: error in ca_clear_event");
	        channelAccessMonitorData->evid = NULL;
	      }
	  } else if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
	/* shift everybody down one, add at end */
	      for (j = 1; j < dlCartesianPlot->count; j++) {
		channelAccessMonitorData->xrtData->g.data[trace].yp[j-1] =
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j];
		channelAccessMonitorData->xrtData->g.data[trace].xp[j-1] =
		     (float) (j-1);
	      }
	      channelAccessMonitorData->xrtData->g.data[trace].yp[j-1] =
		     (float) channelAccessMonitorData->value;
	      channelAccessMonitorData->xrtData->g.data[trace].xp[j-1] =
		     (float) (j-1);
	  }
	}
	break;

      case CP_YVector:
 /* y channel vector, ca_element_count(chid) elements */

	trace = channelAccessMonitorData->trace;
	loopCount = ca_element_count(channelAccessMonitorData->chid);

	/* plot first "count" elements of vector per dlCartesianPlot*/
	switch(args.type) {
	    case DBR_STS_STRING:
		  fprintf(stderr,"\nprocessMonitorEvent: %s: %s",
			"illegal field type for Cartesian Plot channel",
			ca_name(channelAccessMonitorData->chid));
		  f_value = 0.0;
		  for (j = 0; j < loopCount; j++) {
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			(float) j;
		  }
		  break;
	    case DBR_STS_INT:
/*	case DBR_STS_SHORT: -- INT and SHORT are same thing in db_access.h */
		  ciData = (struct dbr_sts_int *) args.dbr;
		  pShort = &ciData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pShort[j];
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			(float) j;
		  }
		  break;
	    case DBR_STS_FLOAT:
		  cfData = (struct dbr_sts_float *) args.dbr;
		  pFloat = &cfData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pFloat[j];
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			(float) j;
		  }
		  break;
	    case DBR_STS_ENUM:
		  ceData = (struct dbr_sts_enum *) args.dbr;
		  pUnsignedShort = &ceData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pUnsignedShort[j];
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			(float) j;
		  }
		  break;
	    case DBR_STS_CHAR:
		  ccData = (struct dbr_sts_char *) args.dbr;
		  pChar = &ccData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pChar[j];
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			(float) j;
		  }
		  break;
	    case DBR_STS_LONG:
		  clData = (struct dbr_sts_long *) args.dbr;
		  pLong = &clData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pLong[j];
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
		 	f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			(float) j;
		  }
		  break;
	    case DBR_STS_DOUBLE:
		  cdData = (struct dbr_sts_double *) args.dbr;
		  pDouble = &cdData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pDouble[j];
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			(float) j;
		  }
		  break;
	}
	channelAccessMonitorData->xrtData->g.data[trace].npoints = loopCount;
	break;


      case CP_XVectorYScalarX:
 /* x channel vector, ca_element_count(chid) elements */
	trace = channelAccessMonitorData->trace;
	loopCount = ca_element_count(channelAccessMonitorData->chid);
	channelAccessMonitorData->xrtData->g.data[trace].npoints = loopCount;

	/* plot first "count" elements of vector per dlCartesianPlot*/
	switch(args.type) {
	    case DBR_STS_STRING:
		  fprintf(stderr,"\nprocessMonitorEvent: %s: %s",
			"illegal field type for Cartesian Plot channel",
			ca_name(channelAccessMonitorData->chid));
		  f_value = 0.0;
		  for (j = 0; j < loopCount; j++) {
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			(float) channelAccessMonitorData->other->value;
		  }
		  break;
	    case DBR_STS_INT:
/*	case DBR_STS_SHORT: -- INT and SHORT are same thing in db_access.h */
		  ciData = (struct dbr_sts_int *) args.dbr;
		  pShort = &ciData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pShort[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			(float) channelAccessMonitorData->other->value;
		  }
		  break;
	    case DBR_STS_FLOAT:
		  cfData = (struct dbr_sts_float *) args.dbr;
		  pFloat = &cfData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pFloat[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			(float) channelAccessMonitorData->other->value;
		  }
		  break;
	    case DBR_STS_ENUM:
		  ceData = (struct dbr_sts_enum *) args.dbr;
		  pUnsignedShort = &ceData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pUnsignedShort[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			(float) channelAccessMonitorData->other->value;
		  }
		  break;
	    case DBR_STS_CHAR:
		  ccData = (struct dbr_sts_char *) args.dbr;
		  pChar = &ccData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pChar[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			(float) channelAccessMonitorData->other->value;
		  }
		  break;
	    case DBR_STS_LONG:
		  clData = (struct dbr_sts_long *) args.dbr;
		  pLong = &clData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pLong[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
		 	f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			(float) channelAccessMonitorData->other->value;
		  }
		  break;
	    case DBR_STS_DOUBLE:
		  cdData = (struct dbr_sts_double *) args.dbr;
		  pDouble = &cdData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pDouble[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			(float) channelAccessMonitorData->other->value;
		  }
		  break;
	}
	channelAccessMonitorData->xrtData->g.data[trace].npoints = loopCount;
	break;


      case CP_YVectorXScalarY:
 /* y channel vector, ca_element_count(chid) elements */
	trace = channelAccessMonitorData->trace;
	loopCount = ca_element_count(channelAccessMonitorData->chid);
	channelAccessMonitorData->xrtData->g.data[trace].npoints = loopCount;

	/* plot first "count" elements of vector per dlCartesianPlot*/
	switch(args.type) {
	    case DBR_STS_STRING:
		  fprintf(stderr,"\nprocessMonitorEvent: %s: %s",
			"illegal field type for Cartesian Plot channel",
			ca_name(channelAccessMonitorData->chid));
		  f_value = 0.0;
		  for (j = 0; j < loopCount; j++) {
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			(float) channelAccessMonitorData->other->value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		  }
		  break;
	    case DBR_STS_INT:
/*	case DBR_STS_SHORT: -- INT and SHORT are same thing in db_access.h */
		  ciData = (struct dbr_sts_int *) args.dbr;
		  pShort = &ciData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pShort[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			(float) channelAccessMonitorData->other->value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		  }
		  break;
	    case DBR_STS_FLOAT:
		  cfData = (struct dbr_sts_float *) args.dbr;
		  pFloat = &cfData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pFloat[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			(float) channelAccessMonitorData->other->value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		  }
		  break;
	    case DBR_STS_ENUM:
		  ceData = (struct dbr_sts_enum *) args.dbr;
		  pUnsignedShort = &ceData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pUnsignedShort[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			(float) channelAccessMonitorData->other->value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		  }
		  break;
	    case DBR_STS_CHAR:
		  ccData = (struct dbr_sts_char *) args.dbr;
		  pChar = &ccData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pChar[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			(float) channelAccessMonitorData->other->value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		  }
		  break;
	    case DBR_STS_LONG:
		  clData = (struct dbr_sts_long *) args.dbr;
		  pLong = &clData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pLong[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			(float) channelAccessMonitorData->other->value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
		 	f_value;
		  }
		  break;
	    case DBR_STS_DOUBLE:
		  cdData = (struct dbr_sts_double *) args.dbr;
		  pDouble = &cdData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pDouble[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			(float) channelAccessMonitorData->other->value;
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		  }
		  break;
	}
	channelAccessMonitorData->xrtData->g.data[trace].npoints = loopCount;
	break;

      case CP_XVectorYScalarY:
	trace = channelAccessMonitorData->trace;
	loopCount = ca_element_count(channelAccessMonitorData->other->chid);
	channelAccessMonitorData->xrtData->g.data[trace].npoints = loopCount;
	for (j = 0; j < loopCount; j++) {
	  channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
		(float) channelAccessMonitorData->value;
	}
	break;

      case CP_YVectorXScalarX:
	trace = channelAccessMonitorData->trace;
	loopCount = ca_element_count(channelAccessMonitorData->other->chid);
	channelAccessMonitorData->xrtData->g.data[trace].npoints = loopCount;
	for (j = 0; j < loopCount; j++) {
	  channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
		(float) channelAccessMonitorData->value;
	}
	break;

/**** N.B. - not handling update of other vector in this call, relying
 ****		upon old value information until it is also updated
 ****		by monitor events.  Note that this is probabilistically
 ****		equivalent to the other more complicated monitor-occurred-
 ****		on-this-channel-but-fill-in-other-channel-from-its-current-
 ****		value
 ****/
      case CP_XYVectorX:
 /* x channel vector, ca_element_count(chid) elements */
	trace = channelAccessMonitorData->trace;
	loopCount = ca_element_count(channelAccessMonitorData->chid);
	loopCount = MIN(loopCount,
		(int)ca_element_count(channelAccessMonitorData->other->chid));
	channelAccessMonitorData->xrtData->g.data[trace].npoints = loopCount;

	/* plot first "count" elements of vector per dlCartesianPlot*/
	switch(args.type) {
	    case DBR_STS_STRING:
		  fprintf(stderr,"\nprocessMonitorEvent: %s: %s",
			"illegal field type for Cartesian Plot channel",
			ca_name(channelAccessMonitorData->chid));
		  f_value = 0.0;
		  for (j = 0; j < loopCount; j++) {
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		  }
		  break;
	    case DBR_STS_INT:
/*	case DBR_STS_SHORT: -- INT and SHORT are same thing in db_access.h */
		  ciData = (struct dbr_sts_int *) args.dbr;
		  pShort = &ciData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pShort[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		  }
		  break;
	    case DBR_STS_FLOAT:
		  cfData = (struct dbr_sts_float *) args.dbr;
		  pFloat = &cfData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pFloat[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		  }
		  break;
	    case DBR_STS_ENUM:
		  ceData = (struct dbr_sts_enum *) args.dbr;
		  pUnsignedShort = &ceData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pUnsignedShort[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		  }
		  break;
	    case DBR_STS_CHAR:
		  ccData = (struct dbr_sts_char *) args.dbr;
		  pChar = &ccData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pChar[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		  }
		  break;
	    case DBR_STS_LONG:
		  clData = (struct dbr_sts_long *) args.dbr;
		  pLong = &clData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pLong[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
		 	f_value;
		  }
		  break;
	    case DBR_STS_DOUBLE:
		  cdData = (struct dbr_sts_double *) args.dbr;
		  pDouble = &cdData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pDouble[j];
		     channelAccessMonitorData->xrtData->g.data[trace].xp[j] =
			f_value;
		  }
		  break;
	}
	channelAccessMonitorData->xrtData->g.data[trace].npoints = loopCount;
	break;


      case CP_XYVectorY:
 /* y channel vector, ca_element_count(chid) elements */
	trace = channelAccessMonitorData->trace;
	loopCount = ca_element_count(channelAccessMonitorData->chid);
	loopCount = MIN(loopCount,
		(int)ca_element_count(channelAccessMonitorData->other->chid));
	channelAccessMonitorData->xrtData->g.data[trace].npoints = loopCount;

	/* plot first "count" elements of vector per dlCartesianPlot*/
	switch(args.type) {
	    case DBR_STS_STRING:
		  fprintf(stderr,"\nprocessMonitorEvent: %s: %s",
			"illegal field type for Cartesian Plot channel",
			ca_name(channelAccessMonitorData->chid));
		  f_value = 0.0;
		  for (j = 0; j < loopCount; j++) {
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			(float) f_value;
		  }
		  break;
	    case DBR_STS_INT:
/*	case DBR_STS_SHORT: -- INT and SHORT are same thing in db_access.h */
		  ciData = (struct dbr_sts_int *) args.dbr;
		  pShort = &ciData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pShort[j];
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		  }
		  break;
	    case DBR_STS_FLOAT:
		  cfData = (struct dbr_sts_float *) args.dbr;
		  pFloat = &cfData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pFloat[j];
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		  }
		  break;
	    case DBR_STS_ENUM:
		  ceData = (struct dbr_sts_enum *) args.dbr;
		  pUnsignedShort = &ceData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pUnsignedShort[j];
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		  }
		  break;
	    case DBR_STS_CHAR:
		  ccData = (struct dbr_sts_char *) args.dbr;
		  pChar = &ccData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pChar[j];
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		  }
		  break;
	    case DBR_STS_LONG:
		  clData = (struct dbr_sts_long *) args.dbr;
		  pLong = &clData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pLong[j];
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
		 	f_value;
		  }
		  break;
	    case DBR_STS_DOUBLE:
		  cdData = (struct dbr_sts_double *) args.dbr;
		  pDouble = &cdData->value;
		  for (j = 0; j < loopCount; j++) {
		     f_value = (float) pDouble[j];
		     channelAccessMonitorData->xrtData->g.data[trace].yp[j] =
			f_value;
		  }
		  break;
	}
	channelAccessMonitorData->xrtData->g.data[trace].npoints = loopCount;
	break;


      default:
	fprintf(stderr,"\n processMonitorEvent: unhandled CP case: %d",
			channelAccessMonitorData->xyChannelType);
    }
  }

}  

     


/***
 ***  Controller routines
/***

/*
 * controller connection event handler - when the channel connects,
 *   register a GR type get_callback() to get the control
 *   information for the controller object/widget
 */
void processControllerConnectionEvent(struct connection_handler_args args)
{
  chtype channelType;
  ChannelAccessControllerData *channelAccessControllerData =
			(ChannelAccessControllerData *) ca_puser(args.chid);


  if (globalDisplayListTraversalMode != DL_EXECUTE ||
			channelAccessControllerData == NULL) return;
  globalModifiedFlag = True;

/* use displayInfo->drawingArea == NULL as sign of "in cleanup", hence ignore */
   if (channelAccessControllerData->monitorData == NULL) return;
   if (channelAccessControllerData->monitorData->displayInfo == NULL) return;
   if (channelAccessControllerData->monitorData->displayInfo->drawingArea
	== NULL) return;

/* let the monitor side handle the management/unmanagement */

  if (args.op != CA_OP_CONN_UP) return;

  if (channelAccessControllerData->monitorData->previouslyConnected) return;


  switch(ca_field_type(args.chid)) {
	case DBF_STRING:
		channelType = DBR_GR_STRING; break;
	case DBF_INT:
/*	case DBF_SHORT: -- INT and SHORT are same thing in db_access.h */
		channelType = DBR_GR_INT; break;
	case DBF_FLOAT:
		channelType = DBR_GR_FLOAT; break;
	case DBF_ENUM:
		channelType = DBR_GR_ENUM; break;
	case DBF_CHAR:
		channelType = DBR_GR_CHAR; break;
	case DBF_LONG:
		channelType = DBR_GR_LONG; break;
	case DBF_DOUBLE:
		channelType = DBR_GR_DOUBLE; break;

	default:
		fprintf(stderr,
		  "\nprocessControllerConnectionEvent: unknown field type = %d",
		   ca_field_type(args.chid));
		return;
  }
  SEVCHK(ca_get_callback(channelType,args.chid,
	processControllerGrGetCallback,NULL),
	"\nprocessControllerConnectionEvent: error in ca_get_callback");

/* turn off this connection event handler, install null handler */
  ca_change_connection_event(args.chid,nullControllerConnectionEventHandler);

}



/*
 * controller graphics get callback - when the graphics information is
 *   available, store it
 */
void processControllerGrGetCallback(struct event_handler_args args)
{
  ChannelAccessControllerData *controllerData = ca_puser(args.chid);
  ChannelAccessMonitorData *mData;
  DlMenu *dlMenu;
  DlChoiceButton *dlChoiceButton;
  DlValuator *dlValuator;
  DlMessageButton *dlMessageButton;
  DlTextEntry *dlTextEntry;
  WidgetList children;
  Cardinal numChildren;
  Widget menu, *buttons;
  XmFontList fontList;
  struct dbr_sts_string  *sstData;
  struct dbr_gr_int    *ciData;
  struct dbr_gr_float  *cfData;
  struct dbr_gr_enum   *ceData;
  struct dbr_gr_char   *ccData;
  struct dbr_gr_long   *clData;
  struct dbr_gr_double *cdData;

  Arg wargs[28];
  OptionMenuData *optionMenuData;
  int i, n, iLopr, iHopr, iValue, iDecimal, iScale;
  double power, range;
  int usedWidth, usedHeight;
  short sqrtEntries;
  double dSqrt;
  int maxChars;
  Widget toggleButton, radioBox;


/* (MDA) events can still be firing when we can't process them,
 *	thanks to asynchronicity
 */
  if (globalDisplayListTraversalMode != DL_EXECUTE || controllerData == NULL)
    return;
  globalModifiedFlag = True;
  if (controllerData->monitorData == NULL) return;
  if (controllerData->monitorData->displayInfo == NULL) return;
/* use displayInfo->drawingArea == NULL as sign of "in cleanup */
  if (controllerData->monitorData->displayInfo->drawingArea == NULL) return;

/*
 * if this get callback returns after the monitor is invalidated, return
 */
  if (!isCurrentMonitor(controllerData->monitorData))
    return;


/* store status information in ChannelAccessControllerData structure/widget */
  switch(args.type) {
	case DBR_GR_STRING:
		sstData = (struct dbr_sts_string *) args.dbr;
		strcpy(controllerData->stringValue,sstData->value);
		break;
	case DBR_GR_INT:
/*	case DBR_GR_SHORT: -- INT and SHORT are same thing in db_access.h */
		ciData = (struct dbr_gr_int *) args.dbr;
		controllerData->lopr = (double) ciData->lower_disp_limit;
		controllerData->hopr = (double) ciData->upper_disp_limit;
		controllerData->value = (double) ciData->value;
		break;
	case DBR_GR_FLOAT:
		cfData = (struct dbr_gr_float *) args.dbr;
		controllerData->lopr = (double) cfData->lower_disp_limit;
		controllerData->hopr = (double) cfData->upper_disp_limit;
		controllerData->value = (double) cfData->value;
		break;
	case DBR_GR_ENUM:
		ceData = (struct dbr_gr_enum *) args.dbr;
		controllerData->lopr = (double) 0.0;
		/* high is number of enum values -1 */
		controllerData->hopr = (double) (MAX(0,ceData->no_str-1));
		controllerData->value = (double) ceData->value;
		break;
	case DBR_GR_CHAR:
		ccData = (struct dbr_gr_char *) args.dbr;
		controllerData->lopr = (double) ccData->lower_disp_limit;
		controllerData->hopr = (double) ccData->upper_disp_limit;
		controllerData->value = (double) ccData->value;
		break;
	case DBR_GR_LONG:
		clData = (struct dbr_gr_long *) args.dbr;
		controllerData->lopr = (double) clData->lower_disp_limit;
		controllerData->hopr = (double) clData->upper_disp_limit;
		controllerData->value = (double) clData->value;
		break;
	case DBR_GR_DOUBLE:
		cdData = (struct dbr_gr_double *) args.dbr;
		controllerData->lopr = (double) cdData->lower_disp_limit;
		controllerData->hopr = (double) cdData->upper_disp_limit;
		controllerData->value = (double) cdData->value;
		break;

	default:
		fprintf(stderr,
		  "\nprocessControllerGrGetCallback: unknown field type = %d",
		   args.type);
		return;
  }

  controllerData->connected = TRUE;

/*
 *  safety net: fix hopr and lopr to do something sensible 
 */
  controllerData->lopr = MIN(controllerData->lopr,controllerData->hopr);
  controllerData->hopr = MAX(controllerData->lopr,controllerData->hopr);
  if ( (fabs(controllerData->lopr - controllerData->hopr)
        < DOUBLE_EPSILON) && args.type != DBR_GR_STRING) {

    switch(controllerData->controllerType) {
	case DL_Valuator:
		fprintf(stderr,
"\n..ControllerGrGetCallback: %s hopr=lopr=%f, -> hopr=lopr+1.0",
		  ca_name(args.chid),
		  controllerData->lopr);
		controllerData->hopr = controllerData->lopr + 1.0;
	    break;
	default:
	    break;
    }
  }
  controllerData->monitorData->lopr = controllerData->lopr;
  controllerData->monitorData->hopr = controllerData->hopr;


/* set the value limits on the controller object (if appropriate) */


  switch(controllerData->controllerType) {
    case DL_Valuator:
      if (args.type == DBR_GR_STRING) {
	    fprintf(stderr,"\nprocessControllerGrGetCallback: %s %s %s",
		"illegal channel type for",ca_name(args.chid),
		": cannot attach Valuator");
      } else {
	/* this guy handles all updates including alarm color */
	    valuatorSetValue(controllerData->monitorData,
				controllerData->value,False);
      }
      break;



    case DL_Menu:
/*
 * this one is tricky:  most of the information is hidden in CA... hence
 * widget creation (option menu) has been deferred 'til now
 */
      if (args.type != DBR_GR_ENUM) {
	    fprintf(stderr,"\nprocessControllerGrGetCallback: %s %s %s",
		"illegal channel type for",ca_name(args.chid),
		": cannot attach Menu ");
	    controllerData->connected = FALSE;
	    controllerData->monitorData->modified = NOT_MODIFIED;
	    if (controllerData->monitorData->evid != NULL) {
		SEVCHK(ca_clear_event(controllerData->monitorData->evid),
		"\nprocessControllerGrGetCallback: error in ca_clear_event");
		controllerData->monitorData->evid = NULL;
	    }
	
      } else {

/* (MDA) need to manage this malloc-ed memory carefully */
	optionMenuData = (OptionMenuData *) malloc(sizeof(OptionMenuData));
	optionMenuData->nButtons = ceData->no_str;
	optionMenuData->buttonType = (XmButtonType *) 
		malloc(MAX(1,ceData->no_str)*sizeof(XmButtonType));
	optionMenuData->buttons = (XmString *) 
		malloc(MAX(1,ceData->no_str)*sizeof(XmString));
	buttons = (Widget *) malloc(MAX(1,ceData->no_str)*sizeof(Widget));
	for (i = 0; i < ceData->no_str; i++) {
	    optionMenuData->buttonType[i] = XmPUSHBUTTON;
	    optionMenuData->buttons[i] = XmStringCreateSimple(ceData->strs[i]);
	}
	optionMenuData->controllerData = (XtPointer) controllerData;
	
	mData = (ChannelAccessMonitorData *) controllerData->monitorData;
	dlMenu = (DlMenu *) mData->specifics;
	fontList = fontListTable[menuFontListIndex(dlMenu->object.height)];
/*
 *  take a guess here - keep this constant the same is in executeControllers.c
 *	this takes out the extra space needed for the cascade pixmap, etc
 */
#define OPTION_MENU_SUBTRACTIVE_WIDTH 23
	if (dlMenu->object.width > OPTION_MENU_SUBTRACTIVE_WIDTH)
	   XtSetArg(wargs[0],XmNwidth,(Dimension) (dlMenu->object.width
		- OPTION_MENU_SUBTRACTIVE_WIDTH));
	else
	   XtSetArg(wargs[0],XmNwidth,(Dimension)dlMenu->object.width);
	XtSetArg(wargs[1],XmNheight,(Dimension)dlMenu->object.height);
	XtSetArg(wargs[2],XmNforeground,(Pixel)
		(mData->clrmod == ALARM ? alarmColorPixel[mData->severity] :
			mData->displayInfo->dlColormap[
			dlMenu->control.clr]));
	XtSetArg(wargs[3],XmNbackground,(Pixel)
		mData->displayInfo->dlColormap[dlMenu->control.bclr]);
	XtSetArg(wargs[4],XmNrecomputeSize,False);
	XtSetArg(wargs[5],XmNfontList,fontList);
	XtSetArg(wargs[6],XmNuserData,optionMenuData);
	XtSetArg(wargs[7],XmNtearOffModel,XmTEAR_OFF_DISABLED);
	XtSetArg(wargs[8],XmNentryAlignment,XmALIGNMENT_CENTER);
	XtSetArg(wargs[9],XmNisAligned,True);
	menu = XmCreatePulldownMenu(mData->displayInfo->drawingArea,"menu",
			wargs,10);
	XtSetArg(wargs[7],XmNalignment,XmALIGNMENT_CENTER);
	for (i = 0; i < ceData->no_str; i++) {
	    XtSetArg(wargs[8],XmNlabelString,optionMenuData->buttons[i]);
    /* use gadgets here so that changing foreground of menu changes buttons */
	    buttons[i] = XmCreatePushButtonGadget(menu,"menuButtons",wargs,9);
	    XtAddCallback(buttons[i],XmNactivateCallback,
			(XtCallbackProc)simpleOptionMenuCallback,(XtPointer)i);
	}
	XtManageChildren(buttons,ceData->no_str);
	n = 7;
	XtSetArg(wargs[n],XmNx,(Position)dlMenu->object.x); n++;
	XtSetArg(wargs[n],XmNy,(Position)dlMenu->object.y); n++;
	XtSetArg(wargs[n],XmNmarginWidth,0); n++;
	XtSetArg(wargs[n],XmNmarginHeight,0); n++;
	XtSetArg(wargs[n],XmNsubMenuId,menu); n++;
	XtSetArg(wargs[n],XmNtearOffModel,XmTEAR_OFF_DISABLED); n++;
	mData->displayInfo->child[mData->displayInfo->childCount++] =
		XmCreateOptionMenu(mData->displayInfo->drawingArea,
			"optionMenu",wargs,n);
	controllerData->self =
		mData->displayInfo->child[mData->displayInfo->childCount-1];
	controllerData->monitorData->self = controllerData->self;


/* unmanage the option label gadget, manage the option menu */
	XtUnmanageChild(XmOptionLabelGadget(controllerData->self));
	XtManageChild(controllerData->self);

/* add the callback for destroy (free memory) */
	XtAddCallback(controllerData->self,XmNdestroyCallback,
		(XtCallbackProc)controllerDestroy,(XtPointer)controllerData);
/* add in drag/drop translations */
	XtOverrideTranslations(controllerData->self,parsedTranslations);

      }

      break;


    case DL_ChoiceButton:

/*
 * this one is tricky:  most of the information is hidden in CA... hence
 * widget creation (radio box/toggle buttons) has been deferred 'til now
 */
      if (args.type != DBR_GR_ENUM) {
	    fprintf(stderr,"\nprocessControllerGrGetCallback: %s %s %s",
		"illegal channel type for",ca_name(args.chid),
		": cannot attach Choice Button");
	    controllerData->connected = FALSE;
	    controllerData->monitorData->modified = NOT_MODIFIED;

	    if (controllerData->monitorData->evid != NULL) {
		SEVCHK(ca_clear_event(controllerData->monitorData->evid),
		"\nprocessControllerGrGetCallback: error in ca_clear_event");
		controllerData->monitorData->evid = NULL;
	   }
	
      } else {

/* (MDA) need to manage this malloc-ed memory carefully */
	optionMenuData = (OptionMenuData *) malloc(sizeof(OptionMenuData));
	optionMenuData->nButtons = ceData->no_str;
	optionMenuData->buttonType = (XmButtonType *) 
		malloc(MAX(1,ceData->no_str)*sizeof(XmButtonType));
	optionMenuData->buttons = (XmString *) 
		malloc(MAX(1,ceData->no_str)*sizeof(XmString));
	maxChars = 0;
	for (i = 0; i < ceData->no_str; i++) {
	    optionMenuData->buttonType[i] = XmRADIOBUTTON;
	    optionMenuData->buttons[i] = XmStringCreateSimple(ceData->strs[i]);
	    maxChars = MAX(maxChars,strlen(ceData->strs[i]));
	}
	optionMenuData->controllerData = (XtPointer) controllerData;
	
	mData = (ChannelAccessMonitorData *) controllerData->monitorData;
	dlChoiceButton = (DlChoiceButton *) mData->specifics;
	n = 0;
	XtSetArg(wargs[n],XmNx,(Position)dlChoiceButton->object.x); n++;
	XtSetArg(wargs[n],XmNy,(Position)dlChoiceButton->object.y); n++;
	XtSetArg(wargs[n],XmNwidth,(Dimension)dlChoiceButton->object.width);
		n++;
	XtSetArg(wargs[n],XmNheight,(Dimension)dlChoiceButton->object.height);
		n++;
	XtSetArg(wargs[n],XmNforeground,(Pixel)
		(mData->clrmod == ALARM ? alarmColorPixel[mData->severity] :
			mData->displayInfo->dlColormap[
			dlChoiceButton->control.clr]));
		n++;
	XtSetArg(wargs[n],XmNbackground,(Pixel)
		mData->displayInfo->dlColormap[dlChoiceButton->control.bclr]);
		n++;
	XtSetArg(wargs[n],XmNindicatorOn,(Boolean)FALSE); n++;
	XtSetArg(wargs[n],XmNmarginWidth,0); n++;
	XtSetArg(wargs[n],XmNmarginHeight,0); n++;
	XtSetArg(wargs[n],XmNresizeWidth,(Boolean)FALSE); n++;
	XtSetArg(wargs[n],XmNresizeHeight,(Boolean)FALSE); n++;
	XtSetArg(wargs[n],XmNspacing,0); n++;
	XtSetArg(wargs[n],XmNrecomputeSize,(Boolean)FALSE); n++;
	XtSetArg(wargs[n],XmNuserData,optionMenuData); n++;
	switch (dlChoiceButton->stacking) {
	  case ROW:
	    XtSetArg(wargs[n],XmNorientation,XmVERTICAL); n++;
	    usedWidth = dlChoiceButton->object.width;
	    usedHeight = dlChoiceButton->object.height/MAX(1,ceData->no_str);
	    break;
	  case COLUMN:
	    XtSetArg(wargs[n],XmNorientation,XmHORIZONTAL); n++;
	    usedWidth = dlChoiceButton->object.width/MAX(1,ceData->no_str);
	    usedHeight = dlChoiceButton->object.height;
	    break;
	  case ROW_COLUMN:
	    XtSetArg(wargs[n],XmNorientation,XmVERTICAL); n++;
	    dSqrt = ceil(sqrt((double)ceData->no_str));
	    sqrtEntries = MAX(2,(short)dSqrt);
	    XtSetArg(wargs[n],XmNnumColumns,sqrtEntries); n++;
	    usedWidth = dlChoiceButton->object.width/sqrtEntries;
	    usedHeight = dlChoiceButton->object.height/sqrtEntries;
	    break;
	  default:
	    fprintf(stderr,
	      "\nprocessControllerGrGetCallback: unknown stacking = %d",
	       dlChoiceButton->stacking);
	    break;
	}
	radioBox = XmCreateRadioBox(mData->displayInfo->drawingArea,
			"radioBox",wargs,n);
	mData->displayInfo->child[mData->displayInfo->childCount++] = radioBox;
	controllerData->self = radioBox;
	controllerData->monitorData->self = controllerData->self;

/* now make push-in type radio buttons of the correct size */
	fontList = fontListTable[choiceButtonFontListIndex(
			dlChoiceButton,ceData->no_str,maxChars)];
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
	XtSetArg(wargs[n],XmNforeground,(Pixel)
		(mData->clrmod == ALARM ? alarmColorPixel[mData->severity] :
			mData->displayInfo->dlColormap[
			dlChoiceButton->control.clr])); n++;
	XtSetArg(wargs[n],XmNbackground,(Pixel)
	    mData->displayInfo->dlColormap[dlChoiceButton->control.bclr]); n++;
	XtSetArg(wargs[n],XmNalignment,XmALIGNMENT_CENTER); n++;
	for (i = 0; i < ceData->no_str; i++) {
	   XtSetArg(wargs[n],XmNlabelString,optionMenuData->buttons[i]);
 /* use gadgets here so that changing foreground of radioBox changes buttons */
	   toggleButton = XmCreateToggleButtonGadget(radioBox,"toggleButton",
					wargs,n+1);
	   if (i==ceData->value)XmToggleButtonGadgetSetState(toggleButton,
			True,True);
	   XtAddCallback(toggleButton,XmNvalueChangedCallback,
			(XtCallbackProc)simpleRadioBoxCallback,(XtPointer)i);

/* MDA - for some reason, need to do this after the fact for gadgets... */
	   XtSetArg(wargs[0],XmNalignment,XmALIGNMENT_CENTER);
	   XtSetValues(toggleButton,wargs,1);

	   XtManageChild(toggleButton);
	}
/* add in drag/drop translations */
	XtOverrideTranslations(radioBox,parsedTranslations);


	XtManageChild(controllerData->self);


/* add the callback for destroy (free memory) */
	XtAddCallback(controllerData->self,XmNdestroyCallback,
		(XtCallbackProc)controllerDestroy,(XtPointer)controllerData);
      }

      break;


    case DL_MessageButton:
      dlMessageButton = (DlMessageButton *)
				controllerData->monitorData->specifics;
      XtSetArg(wargs[0],XmNforeground,
		(controllerData->monitorData->clrmod == ALARM ?
		alarmColorPixel[controllerData->monitorData->severity] :
			controllerData->monitorData->displayInfo->dlColormap[
			dlMessageButton->control.clr]));
      if (controllerData->self != NULL)
	  XtSetValues(controllerData->self,wargs,1);
      break;


    case DL_TextEntry:
      dlTextEntry = (DlTextEntry *) controllerData->monitorData->specifics;
      XtSetArg(wargs[0],XmNforeground,
		(controllerData->monitorData->clrmod == ALARM ?
		alarmColorPixel[controllerData->monitorData->severity] :
			controllerData->monitorData->displayInfo->dlColormap[
			dlTextEntry->control.clr]));

      if (controllerData->self != NULL)
	  XtSetValues(controllerData->self,wargs,1);
      break;

    default:
	fprintf(stderr,
		"\nprocessControllerGrGetCallback: unprocessed controller %d",
		 controllerData->controllerType);
	break;
  }

}  


