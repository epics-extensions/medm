/*
 *
 *  NOTE:  Xlib seems to do a pretty good job of only changing things and
 *		generating protocol when necessary.
 *	   Hence:  the XSetForeground()/XSetBackground()/XChangeGC()/...
 *		calls actually check for real change before generating
 *		protocol packets or more work.  Therefore, it's probably
 *		safe for me to rely on this mechanism, rather than
 *		doing my own checking.  Reason: somebody may change
 *		my GC behind my back and any internal buffering/checking
 *		in this reoutine may get out-of-sync wrt the current value.
 *		Performance penalty - cost of function calls instead of
 *		logical tests.  But this mechanism is guaranteed to be
 *		accurate.
 *
 *	   Alternative:  judicious use of XGetGCValues() with own checking
 *		in this routine, to avoid many function call penalties
 */

#include "medm.h"

/* optimized replacements for numeric sprintf()'s... */
#include "cvtFast.h"


static void localCvtDoubleToExpNotationString(
  double value,
  char *textField,
  unsigned short precision)
{
  double absVal, newVal;
  Boolean minus;
  int exp, k, l;
  char TF[MAX_TEXT_UPDATE_WIDTH];

  absVal = fabs(value);
  minus = (value < 0.0 ? True : False);
  newVal = absVal;

  if (absVal < 1.) {

/* absVal < 1. */
    exp = 0;
    if (absVal != 0.) {		/* really ought to test against some epsilon */
      do {
	newVal *= 1000.0;
	exp += 3;
      } while (newVal < 1.);
    }
    cvtDoubleToString(newVal,TF,precision);
    k = 0; l = 0;
    if (minus) textField[k++] = '-';
    while (TF[l] != '\0') textField[k++] = TF[l++];
    textField[k++] = 'e';
    if (exp == 0) {
	textField[k++] = '+';	/* want e+00 for consistency with norms */
    } else {
	textField[k++] = '-';
    }
    textField[k++] = '0' + exp/10;
    textField[k++] = '0' + exp%10;
    textField[k++] = '\0';

  } else {

/* absVal >= 1. */
    exp = 0;
    while (newVal >= 1000.) {
	newVal *= 0.001; /* since multiplying is usually faster than dividing */
	exp += 3;
    }
    cvtDoubleToString(newVal,TF,precision);
    k = 0; l = 0;
    if (minus) textField[k++] = '-';
    while (TF[l] != '\0') textField[k++] = TF[l++];
    textField[k++] = 'e';
    textField[k++] = '+';
    textField[k++] = '0' + exp/10;
    textField[k++] = '0' + exp%10;
    textField[k++] = '\0';
  }
}



void traverseMonitorList(
  Boolean forcedTraversal,
  DisplayInfo *displayInfo,
  int regionX, 
  int regionY,
  unsigned int regionWidth, 
  unsigned int regionHeight)
{
  ChannelAccessMonitorData *data, *mData;
  DlTextUpdate *dlTextUpdate;
  DlTextEntry *dlTextEntry;
  DlBar *dlBar;
  DlIndicator *dlIndicator;
  DlRectangle *dlRectangle;
  DlOval *dlOval;
  DlArc *dlArc;
  DlText *dlText;
  DlFallingLine *dlFallingLine;
  DlRisingLine *dlRisingLine;
  DlComposite *dlComposite;
  DlPolyline *dlPolyline;
  DlPolygon *dlPolygon;
  DlValuator *dlValuator;
  XcVType current;

  Arg args[5];
  char textField[MAX_TEXT_UPDATE_WIDTH];
  int i, n, usedWidth, usedHeight, iFraction;
  int iValue, prevValue;
  unsigned long alarmColor;
  float f_value, displayedF_value;
  double fraction, displayedValue;
  WidgetList children;
  Cardinal numChildren;
  Boolean alarmColorChanged;

  Arg tmpArgs[4];
  Widget menuWidget;
  XRectangle clipRect[1];
  Region region;
  XPoint points[4];
  XGCValues gcValues;
  unsigned long gcValueMask;
  XExposeEvent exposeEvent;
  CartesianPlotData *cartesianPlotData;
  XtPointer userData;


/* don't do any monitor list traversal if in edit mode or !globalModifiedFlag */
  if (globalDisplayListTraversalMode == DL_EDIT) return;
  if (!forcedTraversal && !globalModifiedFlag) return;

  globalModifiedFlag = False;
  region = (Region) NULL;

/* if a displayInfo was passed in, then do region clipping */
  if (displayInfo != NULL) {
    points[0].x = regionX;
    points[0].y = regionY;
    points[1].x = regionX + regionWidth;
    points[1].y = regionY;
    points[2].x = regionX + regionWidth;
    points[2].y = regionY + regionHeight;
    points[3].x = regionX;
    points[3].y = regionY + regionHeight;
    region = XPolygonRegion(points,4,EvenOddRule);

/* since dynamic objects which fall in the above region can contain
 *	smaller dynamic objects which don't, we need to clip the output
 *	to the region
 */
    clipRect[0].x = regionX;
    clipRect[0].y = regionY;
    clipRect[0].width  = regionWidth;
    clipRect[0].height =  regionHeight;
    XSetClipRectangles(display,displayInfo->gc,0,0,clipRect,1,YXBanded);
    XSetClipRectangles(display,displayInfo->pixmapGC,0,0,clipRect,1,YXBanded);
  }


/* start at first element past placeholder (list head) */
  data = channelAccessMonitorListHead->next;

  if (data != NULL) {

      do {


/*
 * if displayInfo is NULL, then traverse all monitors on all
 *   displays, else, only traverse if monitor's displayInfo matches
 *   passed-in displayInfo, and the object in the display list
 *   intersects the update region
 */
      if ( (displayInfo == (DisplayInfo *)NULL)
			|| (data->displayInfo == displayInfo))

	if ( (displayInfo != (DisplayInfo *)NULL) &&
		(XRectInRegion(region,
			((DlRectangle *)data->specifics)->object.x,
			((DlRectangle *)data->specifics)->object.y,
			((DlRectangle *)data->specifics)->object.width,
			((DlRectangle *)data->specifics)->object.height) 
				== RectangleOut) )
	{ 
	/* since the continue sends us to the end of the do while{} without */
	/*  the prerequisite data=data->next, we need to update data here   */
	    data = data->next;
	    continue;

	} else if ( data->modified == PRIMARY_MODIFIED ||
		(forcedTraversal && (ELEMENT_IS_STATIC(data->monitorType)
				     || data->monitorType == DL_TextUpdate))) {

/* don't reset ->modifed if strip chart or cartesian plot: multiple channels
 *  may need to connect. strip charts operate more or less independently
 *  of the traversal mechanism anyway...  and cartesian plots set
 *  ->modified based on more conditions since complicated object
 *
 *  but for all others = reset ->modified to indicate we've processed this
 *  and we're graphically "up to date"
 */
	  if (data->monitorType != DL_StripChart &&
	      data->monitorType != DL_CartesianPlot)
				data->modified = NOT_MODIFIED;

	  alarmColor =  alarmColorPixel[data->severity];

	  switch (data->monitorType) {

/***
 ***  DYNAMIC ELEMENTS (dynamic versions of "statics")
 ***/
	     case DL_Rectangle:
	     case DL_Oval:
	     case DL_Arc:
	     case DL_Text:
	     case DL_FallingLine:
	     case DL_RisingLine:
	     case DL_Polyline:
	     case DL_Polygon:

	/* always traverse the dynamic static elements if value changed */
		prevValue = data->displayedValue;
		data->displayedValue = data->value;
		

	/* as usual, as long as OBJECT is the first part of any structure, then
	 *   this works */
		dlRectangle = (DlRectangle *)data->specifics;

	/* synthesize an expose event for later processing to get proper
	 *   "Painter's Algorithm" HLHSR behavior
	 * (this doesn't generate synthetic events in response to Expose)
	 *
	 * the event is synthesized for significant transistions only:
	 *   (0) <--> (not 0)  NB: if visibility is generalized (beyond
	 *   the zero/not zero  cases, then this code must be modified
	 */
	      if (data->dlDyn != NULL)
		if (!forcedTraversal && data->dlDyn->attr.mod.vis != V_STATIC
			&& ( (data->value != 0.0 && prevValue == 0.0) ||
			   (data->value == 0.0 && prevValue != 0.0) ) ||
			   (ca_state(data->chid)!=cs_conn && !data->displayed))
		{
			data->displayed = True;
			exposeEvent.type = Expose;
			exposeEvent.serial = 0;
			exposeEvent.send_event = True;
			exposeEvent.display = display;
			exposeEvent.window = XtWindow(
					data->displayInfo->drawingArea);
			if (data->monitorType == DL_RisingLine
			  || data->monitorType == DL_FallingLine
			  || data->monitorType == DL_Polyline) {
		     /* if falling/rising line, since width/height can == 0 */
		     /* special case since lines span x -> x+width          */
			  exposeEvent.x = dlRectangle->object.x-1;
			  exposeEvent.y = dlRectangle->object.y-1;
			  exposeEvent.width = MAX(1,
				dlRectangle->object.width+1)+1;
			  exposeEvent.height = MAX(1,
				dlRectangle->object.height+1)+1;
			} else {
			  exposeEvent.x = dlRectangle->object.x;
			  exposeEvent.y = dlRectangle->object.y;
			  exposeEvent.width = dlRectangle->object.width;
			  exposeEvent.height = dlRectangle->object.height;
			}
			exposeEvent.count = 0;

		/* MDA - this could perhaps be optimized to gather near dynamic
		 *   objects into singe larger expose event */
			XSendEvent(display,XtWindow(
				data->displayInfo->drawingArea),
				False,ExposureMask,(XEvent *)&exposeEvent);

		     /* done processing this one, go to next */
			data = data->next;
			continue;
		}


	/* if we are really going to render: update the basic attributes */
		data->displayInfo->attribute = *data->dlAttr;
		data->displayed = True;

		switch (data->dlDyn->attr.mod.clr) {
		    case STATIC :
		    case DISCRETE:
			gcValueMask = GCForeground|GCBackground|GCLineWidth
					|GCLineStyle;
			if (ca_state(data->chid) == cs_conn) {
			   gcValues.foreground = data->displayInfo->dlColormap[
				data->dlAttr->clr];
			} else {
			   gcValues.foreground = WhitePixel(display,screenNum);
			}
			gcValues.background = data->displayInfo->dlColormap[
				data->displayInfo->drawingAreaBackgroundColor];
			gcValues.line_width = data->dlAttr->width;
			gcValues.line_style = ( (data->dlAttr->style == SOLID)
						? LineSolid : LineOnOffDash);
			XChangeGC(display,data->displayInfo->gc,
				gcValueMask,&gcValues);
			break;
		    case ALARM :
			gcValueMask = GCForeground|GCBackground|GCLineWidth;
			if (ca_state(data->chid) == cs_conn) {
			   gcValues.foreground = alarmColor;
			} else {
			   gcValues.foreground = WhitePixel(display,screenNum);
			}
			gcValues.background = data->displayInfo->dlColormap[
				data->displayInfo->drawingAreaBackgroundColor];
			gcValues.line_width = data->dlAttr->width;
			gcValues.line_style = ( (data->dlAttr->style == SOLID)
						? LineSolid : LineOnOffDash);
			XChangeGC(display,data->displayInfo->gc,
				gcValueMask,&gcValues);
			break;
		}


	/* display the elements, in "forced" mode */
		switch (data->monitorType) {
		   case DL_Rectangle:
			/* correct for lineWidths > 0 */
			dlRectangle = (DlRectangle *)data->specifics;
			switch (data->dlDyn->attr.mod.vis) {
			   case V_STATIC:
				executeDlRectangle(data->displayInfo,
					dlRectangle,TRUE);
				break;
			   case IF_NOT_ZERO:
				if (data->value != 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlRectangle(data->displayInfo,
					dlRectangle,TRUE);
				break;
			   case IF_ZERO:
				if (data->value == 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlRectangle(data->displayInfo,
					dlRectangle,TRUE);
				break;
			}
			break;

		   case DL_Oval:
			dlOval = (DlOval *)data->specifics;
			switch (data->dlDyn->attr.mod.vis) {
			   case V_STATIC:
				executeDlOval(data->displayInfo,
					dlOval,TRUE);
				break;
			   case IF_NOT_ZERO:
				if (data->value != 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlOval(data->displayInfo,
					dlOval,TRUE);
				break;
			   case IF_ZERO:
				if (data->value == 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlOval(data->displayInfo,
					dlOval,TRUE);
				break;
			}
			break;

		   case DL_Arc:
			dlArc = (DlArc *) data->specifics;
			switch (data->dlDyn->attr.mod.vis) {
			   case V_STATIC:
				executeDlArc(data->displayInfo,
					dlArc,TRUE);
				break;
			   case IF_NOT_ZERO:
				if (data->value != 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlArc(data->displayInfo,
					dlArc,TRUE);
				break;
			   case IF_ZERO:
				if (data->value == 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlArc(data->displayInfo,
					dlArc,TRUE);
				break;
			}
			break;

		   case DL_Text:
			dlText = (DlText *) data->specifics;
			switch (data->dlDyn->attr.mod.vis) {
			   case V_STATIC:
				executeDlText(data->displayInfo,
					dlText,TRUE); break;
			   case IF_NOT_ZERO:
				if (data->value != 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlText(data->displayInfo,
					dlText,TRUE);
				break;
			   case IF_ZERO:
				if (data->value == 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlText(data->displayInfo,
					dlText,TRUE);
				break;
			}
			break;

		   case DL_FallingLine:
			dlFallingLine = (DlFallingLine *) data->specifics;
			switch (data->dlDyn->attr.mod.vis) {
			   case V_STATIC:
				executeDlFallingLine(data->displayInfo,
					dlFallingLine,TRUE);
				break;
			   case IF_NOT_ZERO:
				if (data->value != 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlFallingLine(data->displayInfo,
					dlFallingLine,TRUE);
				break;
			   case IF_ZERO:
				if (data->value == 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlFallingLine(data->displayInfo,
					dlFallingLine,TRUE);
				break;
			}
			break;

		   case DL_RisingLine:
			dlRisingLine = (DlRisingLine *) data->specifics;
			switch (data->dlDyn->attr.mod.vis) {
			   case V_STATIC:
				executeDlRisingLine(data->displayInfo,
					dlRisingLine,TRUE);
				break;
			   case IF_NOT_ZERO:
				if (data->value != 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlRisingLine(data->displayInfo,
					dlRisingLine,TRUE);
				break;
			   case IF_ZERO:
				if (data->value == 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlRisingLine(data->displayInfo,
					dlRisingLine,TRUE);
				break;
			}
			break;

		   case DL_Polyline:
			dlPolyline = (DlPolyline *)data->specifics;
			switch (data->dlDyn->attr.mod.vis) {
			   case V_STATIC:
				executeDlPolyline(data->displayInfo,
					dlPolyline,TRUE);
				break;
			   case IF_NOT_ZERO:
				if (data->value != 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlPolyline(data->displayInfo,
					dlPolyline,TRUE);
				break;
			   case IF_ZERO:
				if (data->value == 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlPolyline(data->displayInfo,
					dlPolyline,TRUE);
				break;
			}
			break;

		   case DL_Polygon:
			dlPolygon = (DlPolygon *)data->specifics;
			switch (data->dlDyn->attr.mod.vis) {
			   case V_STATIC:
				executeDlPolygon(data->displayInfo,
					dlPolygon,TRUE);
				break;
			   case IF_NOT_ZERO:
				if (data->value != 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlPolygon(data->displayInfo,
					dlPolygon,TRUE);
				break;
			   case IF_ZERO:
				if (data->value == 0.0
					|| ca_state(data->chid) != cs_conn)
				    executeDlPolygon(data->displayInfo,
					dlPolygon,TRUE);
				break;
			}
			break;

		}
		break;


/***
 ***  COMPOSITE:
 ***/
	     case DL_Composite:	

/****** NB - this is all commented out! *******/

/* careful here - composite visibility not fully implemented - this
 *  requires the synthetic Expose events like above to work properly
 *
		prevValue = data->displayedValue;
		data->displayedValue = data->value;
		dlComposite = (DlComposite *) data->specifics;
		data->displayed = True;
		switch (dlComposite->vis) {
		   case V_STATIC:
			dlComposite->visible = True;
			break;
		   case IF_NOT_ZERO:
			if (data->value != 0.0)
			    dlComposite->visible = True;
			else
			    dlComposite->visible = False;
			executeDlComposite(data->displayInfo,dlComposite,TRUE);
			break;
		   case IF_ZERO:
			if (data->value == 0.0)
			    dlComposite->visible = True;
			else
			    dlComposite->visible = False;
			executeDlComposite(data->displayInfo,dlComposite,TRUE);
			break;
		}
 *
 */
		break;


/***
 ***  STRIP CHART:
 ***/
	     case DL_StripChart:	/* handled automatically, not here */
		break;

/***
 ***  CARTESIAN PLOT:
 ***/
	     case DL_CartesianPlot:
		if (data->self == NULL) {
		  data->modified = NOT_MODIFIED;
		  break;
		}
		if (data->xrtData != NULL) {
/* not in CP related to trigger channel, proceed as normal */
		  if (data->xrtDataSet == 1) {
		      XtSetArg(args[0],XtNxrtData,data->xrtData);
		      XtSetValues(data->self,args,1);
		      data->modified = NOT_MODIFIED;
		  } else if (data->xrtDataSet == 2) {
		      XtSetArg(args[0],XtNxrtData2,data->xrtData);
		      XtSetValues(data->self,args,1);
		      data->modified = NOT_MODIFIED;
		  } else fprintf(stderr,
		     "\ntraverseMonitorList: illegal xrtDataSet specified (%d)",
		      data->xrtDataSet);
		} else {
/* trigger channel monitor, update appropriate plots */
		  XtSetArg(args[0],XmNuserData,&userData);
		  XtGetValues((Widget)data->self,args,1);
		  cartesianPlotData = (CartesianPlotData *) userData;
		  if (cartesianPlotData != NULL) {
		   if (data == (ChannelAccessMonitorData *)
				cartesianPlotData->triggerMonitorData) {
		     for (i = 0; i < cartesianPlotData->nTraces; i++) {
			mData = (ChannelAccessMonitorData *)
					cartesianPlotData->monitors[i][0];
			if (mData != NULL) {
			 if (mData->xrtData != NULL) {
			  if (mData->modified != NOT_MODIFIED &&
					mData->xrtDataSet == 1) {
			    XtSetArg(args[0],XtNxrtData,mData->xrtData);
			    XtSetValues(mData->self,args,1);
 			    mData->modified = NOT_MODIFIED;
			  } else if (mData->modified != NOT_MODIFIED &&
					mData->xrtDataSet == 2) {
			    XtSetArg(args[0],XtNxrtData2,mData->xrtData);
			    XtSetValues(mData->self,args,1);
			    mData->modified = NOT_MODIFIED;
			  }
			 }
			}
		     }
		   }
		  }
		}
		/* this resets modified for the trigger and everybody... */
		data->modified = NOT_MODIFIED;
		break;

/****************************************************************************
 * NOTE:
 * For all controllers: don't test for value != displayedValue because
 *  controllers by their nature can have a visual state which is not
 *  the same as the displayedValue.  Hence rely on modified bit alone.
 ****************************************************************************/

/***
 ***  MENU:
 ***/
	     case DL_Menu:

		if (data->self == NULL) break;
		iValue = (int) data->value;
		data->displayedValue = data->value;
		data->displayed = TRUE;

		XtSetArg(tmpArgs[0],XmNsubMenuId,&menuWidget);
		XtGetValues(data->self,tmpArgs,1);
		XtSetArg(tmpArgs[0],XmNchildren,&children);
		XtSetArg(tmpArgs[1],XmNnumChildren,&numChildren);
		XtGetValues(menuWidget,tmpArgs,2);
		if (iValue >= 0 && iValue < numChildren) {
		  XtSetArg(args[0],XmNmenuHistory,children[iValue]);
		  XtSetValues(data->self,args,1);
		} else {
		  fprintf(stderr,
			"\ntraverseMonitorList: invalid menuHistory child");
		}
		switch (data->clrmod) {
		    case STATIC :
		    case DISCRETE:
			break;
		    case ALARM:
			if (data->severity != data->oldSeverity) {
			    XtSetArg(args[0],XmNforeground,alarmColor);
/* set fg color for option menu here (need to do it separately from above) */
			    XtSetValues(data->self,args,1);
/* also set fg color for pulldown menupane for coloring pushButtonGadgets */
			    XtSetValues(menuWidget,args,1);
			}
			break;
		}
		break;

/***
 ***  VALUATOR:
 ***/
	     case DL_Valuator:

		if (data->self == NULL ) break;
		dlValuator = (DlValuator *)data->specifics;
		if (dlValuator == NULL) break;

/* valuator is only controller/monitor which can have updates disabled */

		if (dlValuator->enableUpdates == True) {
		  valuatorSetValue(data,(double)data->value,False);
		  data->displayedValue = data->value;
		  data->displayed = True;
		}
		break;


/***
 ***  MESSAGE BUTTON:
 ***/
	     case DL_MessageButton:

   /* message button only changes foreground color to reflect alarm severity */
		if (data->self == NULL) break;
		n = 0;
		switch (data->clrmod) {
		    case STATIC :
		    case DISCRETE:
			break;
		    case ALARM:
			if (data->severity != data->oldSeverity) {
			    XtSetArg(args[0],XmNforeground,alarmColor);
			    XtSetValues(data->self,args,1);
			}
			break;
		}
		break;



/***
 ***  CHOICE BUTTON:
 ***/
	     case DL_ChoiceButton:

		if (data->self == NULL) break;
		n = 0;
		switch (data->clrmod) {
		    case STATIC :
		    case DISCRETE:
			break;
		    case ALARM:
/* since choice button is radio box with toggleButton gadgets, this suffices */
			if (data->severity != data->oldSeverity) {
			    XtSetArg(args[0],XmNforeground,alarmColor);
			    XtSetValues(data->self,args,1);
			}
			break;
		}
		iValue = (int) data->value; 

		XtSetArg(args[0],XmNchildren,&children);
		XtSetArg(args[1],XmNnumChildren,&numChildren);
		XtGetValues(data->self,args,2);
		if (iValue >= 0 && iValue < numChildren) {
		    XmToggleButtonGadgetSetState(children[iValue],
			(Boolean)True,(Boolean)True);
		} else {
		   fprintf(stderr,"traverseMonitorList: %s %d",
			"illegal value for choice button:",iValue);
		}
		data->displayedValue = data->value;
		data->displayed = True;
		break;



/***
 ***  METER
 ***/
	     case DL_Meter:
		if (data->self == NULL) break;
		n = 0;
		f_value = (float) data->value;
		displayedF_value = (float) data->displayedValue;
		if (f_value != displayedF_value || !data->displayed
			|| forcedTraversal) {
		    current.fval = f_value;
		    XcMeterUpdateValue(data->self,&current);
		    data->displayedValue = data->value;
		    data->displayed = True;
		}
		switch (data->clrmod) {
		    case STATIC :
		    case DISCRETE:
			break;
		    case ALARM:
			if (data->severity != data->oldSeverity)
			    XcMeterUpdateMeterForeground(data->self,
					alarmColor);
			break;
		}
		break;


/***
 ***  TEXT UPDATE
 ***/
	     case DL_TextUpdate:
		dlTextUpdate = (DlTextUpdate *) data->specifics;
		if (ca_field_type(data->chid) == DBF_STRING) {
			strncpy(textField,data->stringValue,
				MAX_TEXT_UPDATE_WIDTH-1);
			textField[MAX_TEXT_UPDATE_WIDTH-1] = '\0';
			data->displayed = False;
		} else if (ca_field_type(data->chid) == DBF_ENUM &&
			data->numberStateStrings > 0) {
/* getting values of -1 for data->value for invalid connections */
			i = (int) data->value;
			if (i >= 0 && i < data->numberStateStrings) {
			    strcpy(textField,data->stateStrings[i]);
			} else {
			    textField[0] = ' '; textField[1] = '\0';
			}
		} else {
/* using optimized numeric-to-string routines (instead of generic sprintf()) */
			switch (dlTextUpdate->format) {
			   case DECIMAL:
				cvtDoubleToString(data->value,textField,
					(unsigned short)data->precision);
				break;
			   case EXPONENTIAL:
				cvtDoubleToExpString(data->value,textField,
					(unsigned short)data->precision);
				break;
			   case ENGR_NOTATION:
				localCvtDoubleToExpNotationString(data->value,
					textField,
					(unsigned short)data->precision);
				break;
			   case COMPACT:
				cvtDoubleToCompactString(data->value,
					textField,
					(unsigned short)data->precision);
				break;
			   case TRUNCATED:
				cvtLongToString((long)data->value,textField);
				break;
			   case HEXADECIMAL:
				cvtLongToHexString((long)data->value,
					textField);
				break;
			   case OCTAL:
				cvtLongToOctalString((long)data->value,
					textField);
				break;
			}
		}

		data->displayedValue = data->value;
		data->displayed = True;
		i = data->fontIndex;

/* clip to bounding box */
		clipRect[0].x = dlTextUpdate->object.x;
		clipRect[0].y = dlTextUpdate->object.y;
		clipRect[0].width  = dlTextUpdate->object.width;
		clipRect[0].height =  dlTextUpdate->object.height;
		XSetClipRectangles(display,data->displayInfo->gc,0,0,
			clipRect,1,YXBanded);
			

		XSetForeground(display,data->displayInfo->gc,
			data->displayInfo->dlColormap[
			dlTextUpdate->monitor.bclr]);
		XFillRectangle(display,
			XtWindow(data->displayInfo->drawingArea),
			data->displayInfo->gc,
			dlTextUpdate->object.x,dlTextUpdate->object.y,
			dlTextUpdate->object.width,
			dlTextUpdate->object.height);

		
		switch (dlTextUpdate->clrmod) {
		    case STATIC :
		    case DISCRETE:
			gcValueMask = GCForeground | GCBackground;
			if (ca_state(data->chid) == cs_conn) {
			    gcValues.foreground =
				data->displayInfo->dlColormap[
				  dlTextUpdate->monitor.clr];
			} else {
			    gcValues.foreground=WhitePixel(display,screenNum);
			}
			gcValues.background = data->displayInfo->dlColormap[
				dlTextUpdate->monitor.bclr];
			XChangeGC(display,data->displayInfo->gc,
				gcValueMask,&gcValues);
			break;
		    case ALARM :
			gcValueMask = GCForeground | GCBackground;
			if (ca_state(data->chid) == cs_conn) {
			    gcValues.foreground = alarmColor;
			} else {
			    gcValues.foreground=WhitePixel(display,screenNum);
			}
			gcValues.background = data->displayInfo->dlColormap[
				dlTextUpdate->monitor.bclr];
			XChangeGC(display,data->displayInfo->gc,
				gcValueMask,&gcValues);
			break;
		}
		switch (dlTextUpdate->align) {
		  case HORIZ_LEFT:
		    XSetFont(display,data->displayInfo->gc,fontTable[i]->fid);
		    XDrawString(display,XtWindow(
			data->displayInfo->drawingArea),data->displayInfo->gc,
			dlTextUpdate->object.x,
			dlTextUpdate->object.y + fontTable[i]->ascent,
			textField,strlen(textField));
		    break;
		  case HORIZ_CENTER:
		    i = dmGetBestFontWithInfo(fontTable,MAX_FONTS,textField,
			dlTextUpdate->object.height,dlTextUpdate->object.width,
			&usedHeight,&usedWidth,TRUE);
		    XSetFont(display,data->displayInfo->gc,fontTable[i]->fid);
		    XDrawString(display,XtWindow(
				data->displayInfo->drawingArea),
			data->displayInfo->gc,
			dlTextUpdate->object.x 
				+ (dlTextUpdate->object.width - usedWidth)/2,
			dlTextUpdate->object.y + fontTable[i]->ascent,
			textField,strlen(textField));
		    break;
		  case HORIZ_RIGHT:
		    i = dmGetBestFontWithInfo(fontTable,MAX_FONTS,textField,
			dlTextUpdate->object.height,dlTextUpdate->object.width,
			&usedHeight,&usedWidth,TRUE);
		    XSetFont(display,data->displayInfo->gc,fontTable[i]->fid);
		    XDrawString(display,XtWindow(
			data->displayInfo->drawingArea),data->displayInfo->gc,
			dlTextUpdate->object.x 
			  + dlTextUpdate->object.width - usedWidth,
			dlTextUpdate->object.y + fontTable[i]->ascent,
			textField,strlen(textField));
		    break;
		  case VERT_TOP:
		    XSetFont(display,data->displayInfo->gc,fontTable[i]->fid);
		    XDrawString(display,XtWindow(
			data->displayInfo->drawingArea),data->displayInfo->gc,
			dlTextUpdate->object.x,
			dlTextUpdate->object.y + fontTable[i]->ascent,
			textField,strlen(textField));
		    break;
		  case VERT_BOTTOM:
		    i = dmGetBestFontWithInfo(fontTable,MAX_FONTS,textField,
			dlTextUpdate->object.height,dlTextUpdate->object.width,
			&usedHeight,&usedWidth,TRUE);
		    XSetFont(display,data->displayInfo->gc,fontTable[i]->fid);
		    XDrawString(display,XtWindow(
				data->displayInfo->drawingArea),
			data->displayInfo->gc,
			dlTextUpdate->object.x 
				+ (dlTextUpdate->object.width - usedWidth)/2,
			dlTextUpdate->object.y + fontTable[i]->ascent,
			textField,strlen(textField));
		    break;
		  case VERT_CENTER:
		    i = dmGetBestFontWithInfo(fontTable,MAX_FONTS,textField,
			dlTextUpdate->object.height,dlTextUpdate->object.width,
			&usedHeight,&usedWidth,TRUE);
		    XSetFont(display,data->displayInfo->gc,fontTable[i]->fid);
		    XDrawString(display,XtWindow(
				data->displayInfo->drawingArea),
			data->displayInfo->gc,
			dlTextUpdate->object.x 
				+ (dlTextUpdate->object.width - usedWidth)/2,
			dlTextUpdate->object.y + fontTable[i]->ascent,
			textField,strlen(textField));
		    break;
		}
/* disable clipping on data's GC */
		XSetClipMask(display,data->displayInfo->gc,None);

/* restore clipping to region */
		if (displayInfo != NULL) {
		    clipRect[0].x = regionX;
		    clipRect[0].y = regionY;
		    clipRect[0].width  = regionWidth;
		    clipRect[0].height =  regionHeight;
		    XSetClipRectangles(display,displayInfo->gc,0,0,
			clipRect,1,YXBanded);
		}
		break;

/***
 ***  INDICATOR
 ***/
	     case DL_Indicator:
		if (data->self == NULL) break;
		n = 0;
		f_value = (float) data->value;
		displayedF_value = (float) data->displayedValue;
		if (f_value != displayedF_value || !data->displayed
			|| forcedTraversal) {
		    current.fval = f_value;
		    XcIndUpdateValue(data->self,&current);
		    data->displayedValue = data->value;
		    data->displayed = True;
		}
		switch (data->clrmod) {
		    case STATIC :
		    case DISCRETE:
			break;
		    case ALARM:
			if (data->severity != data->oldSeverity)
			    XcIndUpdateIndicatorForeground(data->self,
				alarmColor);
			break;
		}
		break;

/***
 ***  Bar
 ***/
	     case DL_Bar:
		if (data->self == NULL) break;
		n = 0;
		f_value = (float) data->value;
		displayedF_value = (float) data->displayedValue;
		if (f_value != displayedF_value || !data->displayed
			|| forcedTraversal) {
		    current.fval = f_value;
		    XcBGUpdateValue(data->self,&current);
		    data->displayedValue = data->value;
		    data->displayed = True;
		}
		switch (data->clrmod) {
		    case STATIC :
		    case DISCRETE:
			break;
		    case ALARM:
			if (data->severity != data->oldSeverity)
			    XcBGUpdateBarForeground(data->self,alarmColor);
			break;
		}
		break;

/***
 ***  TEXT ENTRY
 ***/
	     case DL_TextEntry:
		if (data->self == NULL) break;
		dlTextEntry = (DlTextEntry *) data->specifics;
		switch (ca_field_type(data->chid)) {
		   case DBF_STRING:
			XmTextFieldSetString(data->self,data->stringValue);
			break;

		   default:
			data->displayedValue = data->value;
			if (ca_field_type(data->chid) == DBF_ENUM &&
				data->numberStateStrings > 0) {
/* getting values of -1 for data->value for invalid connections */
			  i = (int) data->value;
			  if (i >= 0 && i < data->numberStateStrings) {
			    strcpy(textField,data->stateStrings[i]);
			  } else {
			    textField[0] = ' '; textField[1] = '\0';
			  }
			} else {
/* using optimized numeric-to-string routines (instead of generic sprintf()) */
			  switch (dlTextEntry->format) {
			     case DECIMAL:
				cvtDoubleToString(data->value,textField,
					(unsigned short)data->precision);
				break;
			     case EXPONENTIAL:
				cvtDoubleToExpString(data->value,textField,
					(unsigned short)data->precision);
				break;
			   case ENGR_NOTATION:
				localCvtDoubleToExpNotationString(data->value,
					textField,
					(unsigned short)data->precision);
				break;
			     case COMPACT:
				cvtDoubleToCompactString(data->value,
					textField,
					(unsigned short)data->precision);
				break;
			     case TRUNCATED:
				cvtLongToString((long)data->value,textField);
				break;
			     case HEXADECIMAL:
				cvtLongToHexString((long)data->value,
					textField);
				break;
			     case OCTAL:
				cvtLongToOctalString((long)data->value,
					textField);
				break;
			  }
			}
			XmTextFieldSetString(data->self,textField);
		}

		data->displayed = True;
		dlTextEntry = (DlTextEntry *) data->specifics;
		switch (data->clrmod) {
		    case STATIC :
		    case DISCRETE:
			break;
		    case ALARM:
			if (data->severity != data->oldSeverity) {
			    XtSetArg(args[0],XmNforeground,alarmColor);
			    XtSetValues(data->self,args,1);
			}
			break;
		}
		break;


	  }
	}

	data = data->next;

      } while (data != NULL);

/* reset the background color on exit */
      data = channelAccessMonitorListHead->next;
      XSetBackground(display,data->displayInfo->gc,
	data->displayInfo->dlColormap[
	data->displayInfo->drawingAreaBackgroundColor]);
  }
  
/* destroy the region if one is created, and turn off clipping */
  if (displayInfo != (DisplayInfo *)NULL) {
      XSetClipMask(display,displayInfo->gc,None);
      XSetClipMask(display,displayInfo->pixmapGC,None);
      if (region != (Region)NULL) XDestroyRegion(region);
  }
	
}
