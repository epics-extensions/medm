
#include "medm.h"


extern Widget resourceMW, resourceS;
extern Widget objectPaletteSelectToggleButton;


/*
 * event handlers
 */


XtEventHandler popupMenu(
  Widget w,
  DisplayInfo *displayInfo,
  XEvent *event)
{
  XButtonEvent *xEvent = (XButtonEvent *)event;
  Widget widget;
  DlElement *element;

  if (displayInfo != currentDisplayInfo) {
    currentDisplayInfo = displayInfo;
    currentColormap = currentDisplayInfo->dlColormap;
    currentColormapSize = currentDisplayInfo->dlColormapSize;

  }
/* and always make sure the window is on top and has input focus */
  XRaiseWindow(display,XtWindow(displayInfo->shell));
  XSetInputFocus(display,XtWindow(displayInfo->shell),
		RevertToParent,CurrentTime);

/* MB3 = Menu */
  if (xEvent->button == Button3) {
     if (globalDisplayListTraversalMode == DL_EDIT) {
  /* edit menu doesn't have valid/unique displayInfo ptr, hence use current */
	XmMenuPosition(currentDisplayInfo->editPopupMenu,
		(XButtonPressedEvent *)event);
	XtManageChild(currentDisplayInfo->editPopupMenu);

     } else {

/*
 * in EXECUTE mode, MB3 can also mean things based on where it occurs,
 *   hence, lookup to see if MB3 occured in an object that cares
 */
	element = lookupElement(displayInfo->dlElementListTail,
		(Position)xEvent->x, (Position)xEvent->y);
	if (element != NULL) {
	   switch(element->type) {
		case DL_Valuator:
		   widget = lookupElementWidget(displayInfo,
			&(element->structure.rectangle->object));
		   if (widget != NULL) {
			popupValuatorKeyboardEntry(widget,displayInfo,event);
			XUngrabPointer(display,CurrentTime);
		   }
		   break;

		case DL_CartesianPlot:
		   widget = lookupElementWidget(displayInfo,
			&(element->structure.rectangle->object));
		   if (widget != NULL) {
		/* update globalResourceBundle with this element's info */
			executeTimeCartesianPlotWidget = widget;
			updateGlobalResourceBundleFromElement(element);
			if (cartesianPlotAxisS == NULL) {
			   cartesianPlotAxisS = createCartesianPlotAxisDialog(
							mainShell);
			} else {
			   XtSetSensitive(cartesianPlotAxisS,True);
			}
		/* update cartesian plot axis data from globalResourceBundle */
 			updateCartesianPlotAxisDialogFromWidget(widget);
			XtManageChild(cpAxisForm);
			XtPopup(cartesianPlotAxisS,XtGrabNone);
 
			XUngrabPointer(display,CurrentTime);
		   }
		   break;

		default:
		   XmMenuPosition(displayInfo->executePopupMenu,
			(XButtonPressedEvent *)event);
		   XtManageChild(displayInfo->executePopupMenu);
		   break;
	   }
	} else {
  /* execute menu does have valid/unique displayInfo ptr, hence use it */
	   XmMenuPosition(displayInfo->executePopupMenu,
		(XButtonPressedEvent *)event);
	   XtManageChild(displayInfo->executePopupMenu);
	}
     }
  }
}


XtEventHandler popdownMenu(
  Widget w,
  DisplayInfo *displayInfo,
  XEvent *event)
{
  XButtonEvent *xEvent = (XButtonEvent *)event;

/* MB3 = Menu */
  if (xEvent->button == Button3) {
     if (globalDisplayListTraversalMode == DL_EDIT) {
  /* edit menu doesn't have valid/unique displayInfo ptr, hence use current */
	XtUnmanageChild(currentDisplayInfo->editPopupMenu);
     } else {
  /* execute menu does have valid/unique displayInfo ptr, hence use it */
	XtUnmanageChild(displayInfo->executePopupMenu);
     }
  }
}



XtEventHandler handleEnterWindow(
  Widget w,
  DisplayInfo *displayInfo,
  XEvent *event)
{
  pointerInDisplayInfo = displayInfo;
}



XtEventHandler handleButtonPress(
  Widget w,
  DisplayInfo *displayInfo,
  XEvent *event)
{
  XButtonEvent *xEvent = (XButtonEvent *)event;
  int j, k;
  Position x0, y0, x1, y1, initialX0, initialY0;
  Dimension dx0, dy0, daWidth, daHeight;
  DlElement *element;
  Boolean unselectedOne, validDrag, validResize;
  int numSelected, minSize;
  DlElement **array;
  int numElements, arraySize;
  XEvent localEvent, newEvent;
  Boolean objectDataOnly, foundVertex = False, foundPoly;
  int newEventType;
  Arg args[2];
  DisplayInfo *di, *nextDI;



/* if in execute mode - update currentDisplayInfo and simply return */
  if (globalDisplayListTraversalMode == DL_EXECUTE) {
     currentDisplayInfo = displayInfo;
     return;
  }

  /* loop over all displays, unhighlight and unselect previously
	selected elements */
  di = displayInfoListHead->next;
  while (di != NULL) {
     nextDI = di->next;
     if (di != displayInfo) {
       currentDisplayInfo = di;
       unhighlightSelectedElements();
       unselectSelectedElements();
     }
     di = nextDI;
  }

  currentDisplayInfo = displayInfo;
  currentColormap = currentDisplayInfo->dlColormap;
  currentColormapSize = currentDisplayInfo->dlColormapSize;

/* and always make sure the window is on top and has input focus */
  XRaiseWindow(display,XtWindow(displayInfo->shell));
  XSetInputFocus(display,XtWindow(displayInfo->shell),
		RevertToParent,CurrentTime);

  if (w != displayInfo->drawingArea) {
  /* other widget trapped the button press, therefore "normalize" */
    XtVaGetValues(w,XmNx,&dx0,XmNy,&dy0,NULL);
    dx0 += event->xbutton.x;
    dy0 += event->xbutton.y;
    x0 = dx0;
    y0 = dy0;
  } else {
    x0 = event->xbutton.x;
    y0 = event->xbutton.y;
  }

/*
 * handle differently, based on currentActionType
 */

/* change drawingArea's cursor back to the appropriate cursor */
  XDefineCursor(display,XtWindow(currentDisplayInfo->drawingArea),
		(currentActionType == SELECT_ACTION ?
		 rubberbandCursor : crosshairCursor));


/**************************************************************************
 ***			SELECT_ACTION					***
 **************************************************************************/

 if (currentActionType == SELECT_ACTION) {

/****************************************
 * MB1       =  select (rubberband)	*
 * shift-MB1 =  multi-select		*
 * MB2       =  adjust (drag)		*
 * ctrl-MB2  =  adjust( resize)		*
 * MB3       =  menu			*
 ****************************************/


    switch (xEvent->button) {

/**************************************************************/
/**************************************************************/
      case Button1:

	if (xEvent->state & ControlMask) {
 /**********************/
 /* Ctrl-MB1 = NOTHING */
 /**********************/
	  break;
	}

  /* see if this initial MB1-press is on a polyline/polygon vertex */
	array = selectedElementsLookup(displayInfo->dlElementListTail,
			x0,y0,x0,y0,&arraySize,&numElements);
  /* handle point grabs of polyline/polygon if already selected */
	foundPoly = False;
	for (k = 0; k < displayInfo->numSelectedElements; k++) {
	   if (displayInfo->selectedElementsArray[k] == array[0] &&
	       (array[0]->type == DL_Polyline || array[0]->type == DL_Polygon)){
		foundPoly = True;
		break;	/* out of for */
	   }
	}
	if (foundPoly) {
	  if (array[0]->type == DL_Polyline) {	
#define EPSILON 6
	    for (j = 0; j < array[0]->structure.polyline->nPoints; j++) {
		if (array[0]->structure.polyline->points[j].x >= x0 - EPSILON &&
		    array[0]->structure.polyline->points[j].x <= x0 + EPSILON &&
		    array[0]->structure.polyline->points[j].y >= y0 - EPSILON &&
		    array[0]->structure.polyline->points[j].y <= y0 + EPSILON) {
			handlePolylineVertexManipulation(
				array[0]->structure.polyline,j);
			foundVertex = True;
			break;	/* to exit the for loop */
		}
	    }
	    if (foundVertex) break;	/* exit the case */
	  } else if (array[0]->type == DL_Polygon) {	
	    for (j = 0; j < array[0]->structure.polyline->nPoints; j++) {
		if (array[0]->structure.polyline->points[j].x >= x0 - EPSILON &&
		    array[0]->structure.polyline->points[j].x <= x0 + EPSILON &&
		    array[0]->structure.polyline->points[j].y >= y0 - EPSILON &&
		    array[0]->structure.polyline->points[j].y <= y0 + EPSILON) {
			handlePolygonVertexManipulation(
				array[0]->structure.polygon,j);
			foundVertex = True;
			break;	/* to exit the for loop */
		}
	    }
#undef EPSILON
	    if (foundVertex) break;	/* exit the case */
	  }
	} else {
/* not polyline/polygon pick, generate a region... */

	  if (array != NULL) free((char *)array);
	  numElements = 0;

	  doRubberbanding(XtWindow(displayInfo->drawingArea),&x0,&y0,&x1,&y1);
	  array = selectedElementsLookup(displayInfo->dlElementListTail,
			x0,y0,x1,y1,&arraySize,&numElements);
	  if (array == NULL) break;	/* exit this case */



	}




	if (xEvent->state & ShiftMask) {
 /*******************************************/
 /* Shift-MB1 = EXTENDED (AUGMENTED) SELECT */
 /*******************************************/
	   numSelected = highlightAndAugmentSelectedElements(array,
			arraySize,numElements);
	} else {
 /**********************************************************************/
 /* MB1 = SET/RESET SELECT (if element already selected - deselect it) */
 /**********************************************************************/
	   unselectedOne = False;

/* if found vertex for editing, don't deselect */
	   if (!foundVertex) {
	     if (numElements == 1 && displayInfo->numSelectedElements >= 1) {
	   /* see if need to de-select something */
	       unselectedOne = unhighlightAndUnselectElement(array[0],
			&numSelected);
	     }
	     if (!unselectedOne) {
	       numSelected = highlightAndSetSelectedElements(array,arraySize,
			numElements);
	     }
	   }
	}

	if (!foundVertex) {
	  clearResourcePaletteEntries();
	  if (numSelected == 1) {
	    currentElementType = displayInfo->selectedElementsArray[0]->type;
	    setResourcePaletteEntries();
	  }
	}

	break;




/**************************************************************/
/**************************************************************/
      case Button2:

	if (xEvent->state & ShiftMask) {
 /***********************/
 /* Shift-MB2 = NOTHING */
 /***********************/
	  break;
	}

	array = selectedElementsLookup(displayInfo->dlElementListTail,
			x0,y0,x0,y0,&arraySize,&numElements);
	if (array == NULL) break;	/* exit this case */


	if (xEvent->state & ControlMask) {

  /*********************/
  /* Ctrl-MB2 = RESIZE */
  /*********************/

	 if (alreadySelected(array[0])) {

	 /* element already selected - resize it and any others */
	 /* (MDA) ?? separate resize of display here? */

	  /* unhighlight currently selected elements */
	    unhighlightSelectedElements();
	  /* this element already selected: resize all selected elements */
	    validResize = doResizing(XtWindow(displayInfo->drawingArea),
						x0,y0,&x1,&y1);
	    if (validResize) updateResizedElements(x0,y0,x1,y1);
	  /* highlight currently selected elements */
	    numSelected = highlightSelectedElements();
	  /* if only one selected, use first one (only) in list */
	    if (numSelected == 1) {
		objectDataOnly = True;
		updateGlobalResourceBundleAndResourcePalette(objectDataOnly);
	    }

	 } else {

	/* this element not already selected, deselect others and resize it */
	  unhighlightSelectedElements();
	  unselectSelectedElements();
	  clearResourcePaletteEntries();
	  numSelected = highlightAndSetSelectedElements(array,arraySize,
			numElements);
	/* unhighlight currently selected elements */
	  unhighlightSelectedElements();
	  validResize = doResizing(XtWindow(displayInfo->drawingArea),
						x0,y0,&x1,&y1);
	  if (validResize) updateResizedElements(x0,y0,x1,y1);
	/* highlight currently selected elements */
	  numSelected = highlightSelectedElements();
	/* if only one selected, use first one (only) in list */
	  if (numSelected == 1) {
	     objectDataOnly = True;
	     updateGlobalResourceBundleAndResourcePalette(objectDataOnly);
	  }
	 }

	} else {
  /************************/
  /* MB2 = ADJUST (MOVE)  */
  /************************/

	 XtSetArg(args[0],XmNwidth,&daWidth);
	 XtSetArg(args[1],XmNheight,&daHeight);
	 XtGetValues(displayInfo->drawingArea,args,2);
	 if (alreadySelected(array[0])) {

	 /* element already selected - move it and any others */
	    /* unhighlight currently selected elements */
	    unhighlightSelectedElements();
	    /* this element already selected: move all selected elements */
	    validDrag = doDragging(XtWindow(displayInfo->drawingArea),
					daWidth,daHeight,x0,y0,&x1,&y1);
	    if (validDrag) updateDraggedElements(x0,y0,x1,y1);
	    /* highlight currently selected elements */
	    numSelected = highlightSelectedElements();
	    /* if only one selected, use first one (only) in list */
	    if (numSelected == 1) {
	       objectDataOnly = True;
	       updateGlobalResourceBundleAndResourcePalette(objectDataOnly);
	    }

	 } else {

	  /* this element not already selected, deselect others and move it */
	    unhighlightSelectedElements();
	    unselectSelectedElements();
	    numSelected = highlightAndSetSelectedElements(array,arraySize,
			numElements);
	    /* unhighlight currently selected elements */
	    unhighlightSelectedElements();
	    validDrag = doDragging(XtWindow(displayInfo->drawingArea),
					daWidth,daHeight,x0,y0,&x1,&y1);
	    if (validDrag) updateDraggedElements(x0,y0,x1,y1);
	    /* highlight currently selected elements */
	    numSelected = highlightSelectedElements();
	    /* if only one selected, use first one (only) in list */
	    if (numSelected ==1 ) {
	       objectDataOnly = True;
	       updateGlobalResourceBundleAndResourcePalette(objectDataOnly);
	    }
	 }
	}

	break;


      case Button3:
  /***************************************************************************/
  /* MB3 = MENU  --- this is really handled in popupMenu/popdownMenu handler */
  /***************************************************************************/
	break;
    }




/**************************************************************************
 ***			CREATE_ACTION					***
 **************************************************************************/

 } else if (currentActionType == CREATE_ACTION) {

/************************************************
 * MB1       =  create (rubberband)		*
 * MB2       =  adjust (drag)     {as above}	*
 * ctrl-MB2  =  adjust( resize)			*
 * MB3       =  menu				*
 ************************************************/

    switch (xEvent->button) {

      case Button1:
  /*****************************/
  /* MB1 = CREATE (Rubberband) */
  /*****************************/
  /* see how to handle text - either rectangular-create or type-create */
	XWindowEvent(display,XtWindow(displayInfo->drawingArea),
		ButtonReleaseMask|Button1MotionMask,&newEvent);
	newEventType = newEvent.type;
	XPutBackEvent(display,&newEvent);

	if (currentElementType == DL_Text && newEventType == ButtonRelease) {
	  handleTextCreate(x0,y0);
	} else if (currentElementType == DL_Polyline) {
	  handlePolylineCreate(x0,y0,(Boolean)False);
	} else if (currentElementType == DL_Line) {
	  handlePolylineCreate(x0,y0,(Boolean)True);
	} else if (currentElementType == DL_Polygon) {
	  handlePolygonCreate(x0,y0);
	} else {
  /* everybody else has "rectangular" creates */
	  initialX0 = x0;
	  initialY0 = y0;
	  doRubberbanding(XtWindow(displayInfo->drawingArea),
		&x0,&y0,&x1,&y1);
	  globalResourceBundle.x = x0;
	  globalResourceBundle.y = y0;
	  /* pick some semi-sensible size for widget type elements */
	  if (ELEMENT_HAS_WIDGET(currentElementType)) minSize = 12;
	  else minSize = 2;
	  globalResourceBundle.width = MAX(minSize,x1 - x0);
	  globalResourceBundle.height = MAX(minSize,y1 - y0);
	/* actually create elements */
	  handleRectangularCreates(currentElementType);
	}
	break;

      case Button2:
  /****************/
  /* MB2 = ADJUST */
  /****************/
	break;

      case Button3:
  /***************************************************************************/
  /* MB3 = MENU  --- this is really handled in popupMenu/popdownMenu handler */
  /***************************************************************************/
	break;
    }
    /* now toggle back to SELECT_ACTION from CREATE_ACTION */
    XmToggleButtonSetState(objectPaletteSelectToggleButton,True,True);

 }

}



/*
 * set value (with implicit redraw of value) for valuator
 */
void valuatorSetValue(ChannelAccessMonitorData *monitorData, double forcedValue,
			Boolean force)
{
  int iValue;
  double dValue;
  ChannelAccessControllerData *cData;
  Arg args[1];

/* if we got here "too soon" simply return */
  if (monitorData->self == NULL) return;

  if (monitorData->hopr != monitorData->lopr) {
    if (force)
	dValue = forcedValue;
    else
	dValue = monitorData->value;

/* to make reworked event handling for Valuator work */
    cData = (ChannelAccessControllerData *) monitorData->controllerData;
    cData->value = dValue;

/* update scale widget */
    iValue = VALUATOR_MIN + ((dValue - monitorData->lopr)
		/(monitorData->hopr - monitorData->lopr))
		*((double)(VALUATOR_MAX - VALUATOR_MIN));
    monitorData->oldIntegerValue = iValue;
    XtSetArg(args[0],XmNvalue,iValue);
    XtSetValues(monitorData->self,args,1);

/* update and render string value, use monitorData (ignore other stuff) */
    valuatorRedrawValue(monitorData,forcedValue,force,
	monitorData->displayInfo,monitorData->self,
	(DlValuator *)monitorData->specifics);

  }
}



/*
 * redraw value for valuator
 */
void valuatorRedrawValue(ChannelAccessMonitorData *monitorData,
	double forcedValue, Boolean force, DisplayInfo *displayInfo, Widget w,
	DlValuator *dlValuator)
{
  unsigned long foreground, background, valueForeground;
  Dimension scaleWidth, scaleHeight;
  int useableWidth, useableHeight, textHeight, textWidth, startX, startY;
  int n, nChars;
  Arg args[4];
  XFontStruct *font;
  char stringValue[40];
  unsigned long gcValueMask;
  XGCValues gcValues;

  int localPrecision;
  double localValue;

/* return if no window for widget yet, or if displayInfo == NULL, or ... */
  if (XtWindow(w) == (Window)NULL || displayInfo == (DisplayInfo *)NULL ||
      dlValuator == (DlValuator *)NULL) return;

/* simply return if no value to render */
  if (!(dlValuator->label == LIMITS || dlValuator->label == CHANNEL)) return;

  if (monitorData == NULL) {
/* faking values */
    localPrecision = 0;
    localValue = 0.0;
  } else {
/* use real values */
    localPrecision = monitorData->precision;
    localValue = monitorData->value;
  }

  foreground = displayInfo->dlColormap[dlValuator->control.clr];
  background = displayInfo->dlColormap[dlValuator->control.bclr];
  font = fontTable[valuatorFontListIndex(dlValuator)];
  textHeight = font->ascent + font->descent;

  switch (dlValuator->direction) {
      case UP: case DOWN:
	n = 0;
	XtSetArg(args[n],XmNscaleWidth,&scaleWidth); n++;
	XtSetArg(args[n],XmNforeground,&valueForeground); n++;
	XtGetValues(w,args,n);
	useableWidth = dlValuator->object.width - scaleWidth;
	if (force)
	    cvtDoubleToString(forcedValue,stringValue,localPrecision);
	else
	    cvtDoubleToString(localValue,stringValue, localPrecision);
	nChars = strlen(stringValue);
	textWidth = XTextWidth(font,stringValue,nChars);
	startX = MAX(1,useableWidth - textWidth);
	startY = dlValuator->object.height/2 - font->ascent/2;

	gcValueMask = GCForeground | GCFont | GCBackground | GCFunction;
	gcValues.function = GXcopy;
	gcValues.foreground = background;
	gcValues.background = background;
	gcValues.font = font->fid;
	XChangeGC(display, displayInfo->gc, gcValueMask, &gcValues);
	if (monitorData != NULL) {
/* reuse monitorData->fontIndex (int) for storing Valuator's max text width */
	   XFillRectangle(display,XtWindow(w),displayInfo->gc,
		MAX(1,useableWidth - monitorData->fontIndex),
		startY - font->ascent,
		monitorData->fontIndex,font->ascent+font->descent);
	   monitorData->fontIndex = textWidth;
	}


	if (dlValuator->clrmod == ALARM && monitorData != NULL) {
	   XSetForeground(display,displayInfo->gc,
			alarmColorPixel[monitorData->severity]);
	} else {
	   XSetForeground(display,displayInfo->gc,foreground);
	}
	XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,
			stringValue,nChars);
	break;


      case LEFT: case RIGHT:	/* but we know it's really only RIGHT */
	n = 0;
	XtSetArg(args[n],XmNscaleHeight,&scaleHeight); n++;
	XtSetArg(args[n],XmNforeground,&valueForeground); n++;
	XtGetValues(w,args,n);
	useableHeight = dlValuator->object.height - scaleHeight;
	if (force)
	    cvtDoubleToString(forcedValue,stringValue,localPrecision);
	else
	    cvtDoubleToString(localValue,stringValue,localPrecision);
	nChars = strlen(stringValue);
	textWidth = XTextWidth(font,stringValue,nChars);
	startX = dlValuator->object.width/2 - textWidth/2;
	startY = useableHeight - font->descent;

	gcValueMask = GCForeground | GCFont | GCBackground | GCFunction;
	gcValues.function = GXcopy;
	gcValues.foreground = background;
	gcValues.background = background;
	gcValues.font = font->fid;
	XChangeGC(display, displayInfo->gc, gcValueMask, &gcValues);
	if (monitorData != NULL) {
/* reuse monitorData->fontIndex (int) for storing Valuator's max text width */
	   XFillRectangle(display,XtWindow(w),displayInfo->gc,
		dlValuator->object.width/2 - monitorData->fontIndex/2,
		startY - font->ascent,
		monitorData->fontIndex,font->ascent+font->descent);
	   monitorData->fontIndex = textWidth;
	}

	if (dlValuator->clrmod == ALARM && monitorData != NULL) {
	   XSetForeground(display,displayInfo->gc,
			alarmColorPixel[monitorData->severity]);
	} else {
	   XSetForeground(display,displayInfo->gc,foreground);
	}
	XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,
						stringValue,nChars);
	break;
  }  /* end switch() */

}


/*
 * thanks to complicated valuator interactions, need to rely on
 *  Key/ButtonRelease events to re-enable updates for dlValuator display
 */
XtEventHandler handleValuatorRelease(
  Widget w,
  XtPointer passedData,
  XEvent *event)
{
  ChannelAccessControllerData *cData;
  DlValuator *dlValuator;

  if (passedData != NULL) {
/* then valid controllerData exists */
     cData = (ChannelAccessControllerData *)passedData;
     if (cData->monitorData != NULL) {
       dlValuator = (DlValuator *)cData->monitorData->specifics;
       switch(event->type) {
	 case ButtonRelease:
	 case KeyRelease:
	    dlValuator->enableUpdates = True;
	/* don't reset ->dragging: let valuatorValueChanged() do that */
	    break;
       }
     }
  }
}


XtEventHandler handleValuatorExpose(
  Widget w,
  XtPointer passedData,
  XExposeEvent *event)
{
  DlValuator *dlValuator;
  unsigned long foreground, background, valueForeground, alarmColor;
  Dimension scaleWidth, scaleHeight;
  int useableWidth, useableHeight, textHeight, textWidth, startX, startY;
  int n, nChars;
  Arg args[4];
  XFontStruct *font;
  char stringValue[40];
  unsigned long gcValueMask;
  XGCValues gcValues;
  ChannelAccessMonitorData *monitorData;
  DisplayInfo *displayInfo;
  ChannelAccessControllerData *controllerData;
  XtPointer userData;
  double localLopr, localHopr;
  char *localTitle;
  int localPrecision;


  if (event->count > 0) return;

  if (passedData != NULL) {
/* then valid controllerData exists */
     controllerData = (ChannelAccessControllerData *)passedData;
     if (controllerData == NULL) return;
     monitorData = controllerData->monitorData;
     if (monitorData == NULL) return;
     displayInfo = monitorData->displayInfo;
     if (displayInfo == NULL) return;
     dlValuator = (DlValuator *)monitorData->specifics;
     if (dlValuator == NULL) return;
     localLopr = monitorData->lopr;
     localHopr = monitorData->hopr;
     localPrecision = monitorData->precision;
     if (monitorData->chid != NULL)
	localTitle = ca_name(monitorData->chid);
     else localTitle = NULL;

  } else {
/* no controller data, therefore userData = dlValuator */
     XtSetArg(args[0],XmNuserData,&userData);
     XtGetValues(w,args,1);
     dlValuator = (DlValuator *)userData;
     if (dlValuator == NULL) return;
     localLopr = 0.0;
     localHopr = 0.0;
     localPrecision = 0;
     localTitle = dlValuator->control.ctrl;
     displayInfo = dmGetDisplayInfoFromWidget(w);
     if (displayInfo == NULL) return;

  }

/* since XmScale doesn't really do the right things, we'll do it by hand */

  if (dlValuator->label != LABEL_NONE) {

    foreground = displayInfo->dlColormap[dlValuator->control.clr];
    background = displayInfo->dlColormap[dlValuator->control.bclr];
    font = fontTable[valuatorFontListIndex(dlValuator)];
    textHeight = font->ascent + font->descent;

    gcValueMask = GCForeground | GCFont | GCBackground | GCFunction;
    gcValues.function = GXcopy;
    gcValues.foreground = foreground;
    gcValues.background = background;
    gcValues.font = font->fid;
    XChangeGC(display, displayInfo->gc, gcValueMask, &gcValues);

    switch (dlValuator->direction) {
      case UP: case DOWN:	/* but we know it's really only UP */
	n = 0;
	XtSetArg(args[n],XmNscaleWidth,&scaleWidth); n++;
	XtSetArg(args[n],XmNforeground,&valueForeground); n++;
	XtGetValues(w,args,n);
	useableWidth = dlValuator->object.width - scaleWidth;
	if (dlValuator->label == OUTLINE || dlValuator->label == LIMITS 
					|| dlValuator->label == CHANNEL) {
/* LOPR */   cvtDoubleToString(localLopr,stringValue,localPrecision);
	     if (stringValue != NULL) {
	       nChars = strlen(stringValue);
	       textWidth = XTextWidth(font,stringValue,nChars);
	       startX = MAX(1,useableWidth - textWidth);
	       startY = dlValuator->object.height - font->descent - 3;
	       XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,
						stringValue,nChars);
	     }
/* HOPR */   cvtDoubleToString(localHopr,stringValue,localPrecision);
	     if (stringValue != NULL) {
	       nChars = strlen(stringValue);
	       textWidth = XTextWidth(font,stringValue,nChars);
	       startX = MAX(1,useableWidth - textWidth);
	       if (dlValuator->label == CHANNEL) {
		/* need room for label above */
		startY = 1.3*(font->ascent + font->descent)
				+ font->ascent;
	       } else {
		startY = font->ascent + 3;
	       }
	       XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,
						stringValue,nChars);
	     }
	}
	if (dlValuator->label == CHANNEL) {
/* TITLE */
	     if (localTitle != NULL) {
	       nChars = strlen(localTitle);
	       textWidth = XTextWidth(font,localTitle,nChars);
	       startX = MAX(1,useableWidth - textWidth);
	       startY = font->ascent + 2;
	       XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,
					localTitle,nChars);
	     }
	}
	break;



      case LEFT: case RIGHT:	/* but we know it's really only RIGHT */
	n = 0;
	XtSetArg(args[n],XmNscaleHeight,&scaleHeight); n++;
	XtSetArg(args[n],XmNforeground,&valueForeground); n++;
	XtGetValues(w,args,n);
	useableHeight = dlValuator->object.height - scaleHeight;

	if (dlValuator->label == OUTLINE || dlValuator->label == LIMITS 
					|| dlValuator->label == CHANNEL) {
/* LOPR */   cvtDoubleToString(localLopr,stringValue,localPrecision);
	     if (stringValue != NULL) {
	       nChars = strlen(stringValue);
	       startX = 2;
	       startY = useableHeight - font->descent;/* NB: descent=0 for #s */
	       XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,
						stringValue,nChars);
	     }
/* HOPR */   cvtDoubleToString(localHopr,stringValue,localPrecision);
	     if (stringValue != NULL) {
	       nChars = strlen(stringValue);
	       textWidth = XTextWidth(font,stringValue,nChars);
	       startX = dlValuator->object.width - textWidth - 2;
	       startY = useableHeight - font->descent;
	       XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,
						stringValue,nChars);
	     }
	}
	if (dlValuator->label == CHANNEL) {
/* TITLE */
	     if (localTitle != NULL) {
	       nChars = strlen(localTitle);
	       textWidth = XTextWidth(font,localTitle,nChars);
	       startX = 2;
	       startY = font->ascent + 2;
	       XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,
				localTitle,nChars);
	     }
	}
	break;
    }
    if (passedData != NULL) {
/* real data */
      valuatorRedrawValue(monitorData,0.0,False,
	    monitorData->displayInfo,monitorData->self,
		(DlValuator *)monitorData->specifics);
	    

    } else {
/* fake data */
      valuatorRedrawValue(NULL,0.0,False,currentDisplayInfo,w,dlValuator);
    }
  }  /* end --  if (dlValuator->label != LABEL_NONE) */


}




/*
 *  function to highlight the currently selected elements
 *	RETURNS: the current TOTAL number of elements selected
 */
int highlightSelectedElements()
{
  int i;
  DisplayInfo *cdi; /* abbreviation for currentDisplayInfo for clarity */
/*
 * as usual, the type of the union is unimportant as long as object
 *	is the first entry in all element structures
 *
 *	N.B.: need to highlight in both window and pixmap in case
 *	there are expose events...
 */

/* simply return if no selected display */
 if (currentDisplayInfo == NULL) return;
 
 cdi = currentDisplayInfo;

 if (cdi->selectedElementsArray != NULL) {

  for (i = 0; i < cdi->numSelectedElements; i++) {

/* draw the highlight (note that this could draw or undraw it actually) */
   if (cdi->selectedElementsArray[i] != NULL) {

    if (cdi->selectedElementsArray[i]->type == DL_Display) {
  /* if DL_Display draw inside  - NB: x/y are for position of shell only
   *  hence the drawing is always relative to 0,0,w,h */
      XDrawRectangle(display,XtWindow(cdi->drawingArea),highlightGC,
					0 + HIGHLIGHT_LINE_THICKNESS,
					0 + HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.display->object.width -
					2*HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.display->object.height -
					2*HIGHLIGHT_LINE_THICKNESS);
      XDrawRectangle(display,cdi->drawingAreaPixmap,highlightGC,
					0 + HIGHLIGHT_LINE_THICKNESS,
					0 + HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.display->object.width -
					2*HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.display->object.height -
					2*HIGHLIGHT_LINE_THICKNESS);
    } else {
  /* else draw outside */
      XDrawRectangle(display,XtWindow(cdi->drawingArea), highlightGC,
	cdi->selectedElementsArray[i]->structure.rectangle->object.x -
					HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.rectangle->object.y -
					HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.rectangle->object.width +
					2*HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.rectangle->object.height +
					2*HIGHLIGHT_LINE_THICKNESS);
      XDrawRectangle(display,cdi->drawingAreaPixmap, highlightGC,
	cdi->selectedElementsArray[i]->structure.rectangle->object.x -
					HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.rectangle->object.y -
					HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.rectangle->object.width +
					2*HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.rectangle->object.height + 
					2*HIGHLIGHT_LINE_THICKNESS);
    }
   }
  }
 }

 if (cdi->numSelectedElements > 0) cdi->selectedElementsAreHighlighted = True;
 return (cdi->numSelectedElements);
}




/*
 *  function to unhighlight the currently highlighted (and  therfore
 *	selected) elements
 */
void unhighlightSelectedElements()
{
  int i;
  DisplayInfo *cdi; /* abbreviation for currentDisplayInfo for clarity */
/*
 * as usual, the type of the union is unimportant as long as object
 *	is the first entry in all element structures
 *
 *	N.B.: need to highlight in both window and pixmap in case
 *	there are expose events...
 */

/* simply return if no selected display */
 if (currentDisplayInfo == NULL) return;
 
 cdi = currentDisplayInfo;
/*
 * if elements are not highlighted, there's nothing to unhighlight,
 *	simply return
 */
 if (! cdi->selectedElementsAreHighlighted) return;

 cdi->selectedElementsAreHighlighted = FALSE;

 if (cdi->selectedElementsArray != NULL) {
  for (i = 0; i < cdi->numSelectedElements; i++) {

/* undraw the highlight */
   if (cdi->selectedElementsArray[i] != NULL) {

    if (cdi->selectedElementsArray[i]->type == DL_Display) {
  /* if DL_Display draw inside  - NB: x/y are for position of shell only
   *  hence the drawing is always relative to 0,0,w,h */
      XDrawRectangle(display,XtWindow(cdi->drawingArea),highlightGC,
					0 + HIGHLIGHT_LINE_THICKNESS,
					0 + HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.display->object.width -
					2*HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.display->object.height -
					2*HIGHLIGHT_LINE_THICKNESS);
      XDrawRectangle(display,cdi->drawingAreaPixmap,highlightGC,
					0 + HIGHLIGHT_LINE_THICKNESS,
					0 + HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.display->object.width -
					2*HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.display->object.height -
					2*HIGHLIGHT_LINE_THICKNESS);
    } else {
  /* else draw outside */
      XDrawRectangle(display,XtWindow(cdi->drawingArea), highlightGC,
	cdi->selectedElementsArray[i]->structure.rectangle->object.x -
					HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.rectangle->object.y -
					HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.rectangle->object.width +
					2*HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.rectangle->object.height +
					2*HIGHLIGHT_LINE_THICKNESS);
      XDrawRectangle(display,cdi->drawingAreaPixmap, highlightGC,
	cdi->selectedElementsArray[i]->structure.rectangle->object.x -
					HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.rectangle->object.y -
					HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.rectangle->object.width +
					2*HIGHLIGHT_LINE_THICKNESS,
	cdi->selectedElementsArray[i]->structure.rectangle->object.height + 
					2*HIGHLIGHT_LINE_THICKNESS);
    }
   }
  }
 }
}



/*
 * function to unselect any selected elements
 */
void unselectSelectedElements()
{
 if (currentDisplayInfo != NULL)  {
   if (currentDisplayInfo->selectedElementsArray != NULL) {
     currentDisplayInfo->numSelectedElements = 0;
     free ((char *) currentDisplayInfo->selectedElementsArray);
     currentDisplayInfo->selectedElementsArray = NULL;
     currentDisplayInfo->selectedElementsArraySize = 0;
   }
 }
}



/*
 *  function to set and highlight selected elements
 *	first undraw the old element highlights (if there are any)
 *	then draw the new element highlights (and update selected
 *	element array)
 *	RETURNS: the current TOTAL number of elements selected
 */
int highlightAndSetSelectedElements(
  DlElement **array,
  int arraySize, 
  int numElements)
{
  int i;
  DisplayInfo *cdi;
/*
 * as usual, the type of the union is unimportant as long as object
 *	is the first entry in all element structures
 *
 *	N.B.: need to highlight in both window and pixmap in case
 *	there are expose events...
 */


/* if no current display, simply return */
 if (currentDisplayInfo == NULL) return;

 cdi = currentDisplayInfo;

/* undraw the old highlights */
 unhighlightSelectedElements();

/* unselect any selected elements */
 unselectSelectedElements();

 if (array != NULL) {
  for (i = 0; i < numElements; i++) {

/* draw the new highlight */
  if (array[i] != NULL && cdi != NULL) {

    if (array[i]->type == DL_Display) { /* if display draw inside */
      XDrawRectangle(display,XtWindow(cdi->drawingArea), highlightGC,
	0 + HIGHLIGHT_LINE_THICKNESS, 0 + HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.display->object.width-2*HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.display->object.height - 
				2*HIGHLIGHT_LINE_THICKNESS);
      XDrawRectangle(display,cdi->drawingAreaPixmap,highlightGC,
	0 + HIGHLIGHT_LINE_THICKNESS, 0 + HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.display->object.width-2*HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.display->object.height -
				2*HIGHLIGHT_LINE_THICKNESS);
    } else {				/* else draw outside */
      XDrawRectangle(display,XtWindow(cdi->drawingArea),highlightGC,
	array[i]->structure.rectangle->object.x-HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.y-HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.width+2*HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.height + 
				2*HIGHLIGHT_LINE_THICKNESS);
      XDrawRectangle(display,cdi->drawingAreaPixmap,highlightGC,
	array[i]->structure.rectangle->object.x - HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.y - HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.width+2*HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.height + 
				2*HIGHLIGHT_LINE_THICKNESS);
    }
   }
  }
  cdi->selectedElementsArray = array;
  cdi->selectedElementsArraySize = arraySize;
  cdi->numSelectedElements = numElements;
  if (cdi->numSelectedElements > 0) cdi->selectedElementsAreHighlighted = TRUE;
 }

 return(cdi->numSelectedElements);
}







/*
 *  function to augment and highlight selected elements
 *	draw the new element highlights (and update selected element array)
 *	RETURNS: the current TOTAL number of elements selected
 */
int highlightAndAugmentSelectedElements(
  DlElement **array,
  int arraySize,
  int numElements)
{
  int i;
  DisplayInfo *cdi;
/*
 * as usual, the type of the union is unimportant as long as object
 *	is the first entry in all element structures
 *
 *	N.B.: need to highlight in both window and pixmap in case
 *	there are expose events...
 */


/* if no current display, simply return */
 if (currentDisplayInfo == NULL) return;

 cdi = currentDisplayInfo;

 if (array != NULL && numElements != 0) {

  for (i = 0; i < numElements; i++) {

/* draw the new highlight */
  if (array[i] != NULL) {

    if (array[i]->type == DL_Display) { /* if display draw inside */

      XDrawRectangle(display,XtWindow(cdi->drawingArea),highlightGC,
	0 + HIGHLIGHT_LINE_THICKNESS, 0 + HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.display->object.width-2*HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.display->object.height - 
				2*HIGHLIGHT_LINE_THICKNESS);
      XDrawRectangle(display,cdi->drawingAreaPixmap,highlightGC,
	0 + HIGHLIGHT_LINE_THICKNESS, 0 + HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.display->object.width-2*HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.display->object.height -
				2*HIGHLIGHT_LINE_THICKNESS);

    } else {				/* else draw outside */

      XDrawRectangle(display,XtWindow(cdi->drawingArea),highlightGC,
	array[i]->structure.rectangle->object.x-HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.y-HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.width+2*HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.height + 
				2*HIGHLIGHT_LINE_THICKNESS);
      XDrawRectangle(display,cdi->drawingAreaPixmap,highlightGC,
	array[i]->structure.rectangle->object.x - HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.y - HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.width+2*HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.height + 
				2*HIGHLIGHT_LINE_THICKNESS);
    }
   }
  }

  if ( cdi->selectedElementsArray == NULL ||
      (cdi->numSelectedElements+numElements) > cdi->selectedElementsArraySize){
  /* realloc behaves like malloc if 1st arg (ptr) is NULL under XPG3
     (but apparently not SUNOS 4.1.2) */
     if (cdi->selectedElementsArray != NULL) {
	/* this implies cdi->numSelectedElements == 0 */
        cdi->selectedElementsArray = (DlElement **) realloc(
		cdi->selectedElementsArray,
		(cdi->numSelectedElements + numElements)*sizeof(DlElement *));
        cdi->selectedElementsArraySize = cdi->numSelectedElements + numElements;
     } else {
        cdi->selectedElementsArray = (DlElement **) malloc(
		(size_t) (numElements*sizeof(DlElement *)));
        cdi->selectedElementsArraySize = numElements;
     }
  }

  for (i = 0; i < numElements; i++)
     cdi->selectedElementsArray[cdi->numSelectedElements + i] = array[i];

/* now free temporary array, and update numSelectedElements  */
  free ((char *) array);
  cdi->numSelectedElements = cdi->numSelectedElements + numElements;

 }

 return(cdi->numSelectedElements);

}




/*
 *  function to determine if specified element is already in selected list
 *	and unhighlight and unselect if it is...
 *	RETURNS: Boolean - True if an element was found and unselected,
 *		False if none found...
 *	also - the TOTAL number of elements selected is returned in
 *		numSelected  argument
 */
Boolean unhighlightAndUnselectElement(
  DlElement *element,
  int *numSelected)
{
  int i, j, elementIndex;
  DlElement **array;
  DisplayInfo *cdi;
/*
 * as usual, the type of the union is unimportant as long as object
 *	is the first entry in all element structures
 *
 *	N.B.: need to highlight in both window and pixmap in case
 *	there are expose events...
 */

/* simply return if no current display */
 if (currentDisplayInfo == NULL) return;

 cdi = currentDisplayInfo;
 elementIndex = -1;
 if (cdi->selectedElementsArray != NULL) {
   for (i = 0; i < cdi->numSelectedElements; i++) {
	if (cdi->selectedElementsArray[i] == element) elementIndex = i;
   }
 }

 /* element not found - no changes */
 if (elementIndex < 0) {
	*numSelected = cdi->numSelectedElements;
	return (False);
 }


/* for convenience of notation below: */
  array = cdi->selectedElementsArray;
  i = elementIndex;

/* undraw the highlight */
  if (array[i] != NULL) {

    if (array[i]->type == DL_Display) { /* if display draw inside */

      XDrawRectangle(display,XtWindow(cdi->drawingArea),highlightGC,
	0 + HIGHLIGHT_LINE_THICKNESS, 0 + HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.display->object.width-2*HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.display->object.height - 
				2*HIGHLIGHT_LINE_THICKNESS);
      XDrawRectangle(display,cdi->drawingAreaPixmap,highlightGC,
	0 + HIGHLIGHT_LINE_THICKNESS, 0 + HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.display->object.width-2*HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.display->object.height -
				2*HIGHLIGHT_LINE_THICKNESS);

    } else {				/* else draw outside */

      XDrawRectangle(display,XtWindow(cdi->drawingArea),highlightGC,
	array[i]->structure.rectangle->object.x-HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.y-HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.width+2*HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.height + 
				2*HIGHLIGHT_LINE_THICKNESS);
      XDrawRectangle(display,cdi->drawingAreaPixmap,highlightGC,
	array[i]->structure.rectangle->object.x - HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.y - HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.width+2*HIGHLIGHT_LINE_THICKNESS,
	array[i]->structure.rectangle->object.height + 
				2*HIGHLIGHT_LINE_THICKNESS);
    }
   }


/*
 * now remove the element from the list/array and move everybody
 *	downstream up one element
 */
  for (j = elementIndex; j < cdi->numSelectedElements-1; j++) {
      cdi->selectedElementsArray[j] = cdi->selectedElementsArray[j+1];
  }
  cdi->selectedElementsArray[cdi->numSelectedElements-1] = NULL;
  cdi->numSelectedElements--;

  if (cdi->numSelectedElements == 0) {
    free ((char *)cdi->selectedElementsArray);
    cdi->selectedElementsArray = NULL;
  }

 *numSelected = cdi->numSelectedElements;
 return(True);

}



/*
 * recursive function to move Composite objects (and all children, which
 *	may be Composite objects)
 */
void moveCompositeChildren(DisplayInfo *cdi, DlElement *element,
	int xOffset, int yOffset, Boolean moveWidgets)
{
  DlElement *ele;
  Widget widget;
  int j;

  if (element->type == DL_Composite) {
   ele = ((DlElement *)element->structure.composite->dlElementListHead)->next;
   while (ele != NULL) {
    if (ELEMENT_IS_RENDERABLE(ele->type)) {
      if (moveWidgets) {
        if (ELEMENT_HAS_WIDGET(ele->type)) {
	  widget = lookupElementWidget(cdi,
		&(ele->structure.rectangle->object));
  	  if (widget != NULL) XtMoveWidget(widget,
		(Position) (ele->structure.rectangle->object.x + xOffset),
		(Position) (ele->structure.rectangle->object.y + yOffset));
        }
      }
      ele->structure.rectangle->object.x += xOffset;
      ele->structure.rectangle->object.y += yOffset;
      if (ele->type == DL_Composite) {
	  moveCompositeChildren(cdi,ele,xOffset,yOffset,moveWidgets);
      } else if (ele->type == DL_Polyline) {
	for (j = 0; j < ele->structure.polyline->nPoints; j++) {
	   ele->structure.polyline->points[j].x += xOffset;
	   ele->structure.polyline->points[j].y += yOffset;
	}
      } else if (ele->type == DL_Polygon) {
	for (j = 0; j < ele->structure.polygon->nPoints; j++) {
	   ele->structure.polygon->points[j].x += xOffset;
	   ele->structure.polygon->points[j].y += yOffset;
	}
      }
    }
    ele = ele->next;
   }
  }
}


/*
 * update (move) all currently selected elements and rerender
 */
void updateDraggedElements(Position x0, Position y0, Position x1, Position y1)
{
  int i, j;
  int xOffset, yOffset;
  DisplayInfo *cdi;
  Widget widget;
  DlElement *ele;
  Boolean moveWidgets;

/* if no current display or selected elements array, simply return */
  if (currentDisplayInfo == NULL) return;
  if (currentDisplayInfo->selectedElementsArray == NULL) return;

  cdi = currentDisplayInfo;

  xOffset = x1 - x0;
  yOffset = y1 - y0;

/* as usual, type in union unimportant as long as object is 1st thing...*/
  for (i = 0; i < cdi->numSelectedElements; i++) {
    if (cdi->selectedElementsArray[i]->type != DL_Display) {
     if (ELEMENT_HAS_WIDGET(cdi->selectedElementsArray[i]->type)) {
        widget = lookupElementWidget(cdi,
		&(cdi->selectedElementsArray[i]->structure.rectangle->object));
        if (widget != NULL) XtMoveWidget(widget,
	   (Position)
		(cdi->selectedElementsArray[i]->structure.rectangle->object.x
					+ xOffset),
	   (Position)
		(cdi->selectedElementsArray[i]->structure.rectangle->object.y
					+ yOffset));
     }
     cdi->selectedElementsArray[i]->structure.rectangle->object.x += xOffset;
     cdi->selectedElementsArray[i]->structure.rectangle->object.y += yOffset;

/* if moving composite, update all "children" too */
     moveWidgets = True;
     if (cdi->selectedElementsArray[i]->type == DL_Composite) {
	moveCompositeChildren(cdi,cdi->selectedElementsArray[i],
		xOffset,yOffset,moveWidgets);

/* if moving polyline/polygon, update all points */
     } else if (cdi->selectedElementsArray[i]->type == DL_Polyline) {
	for (j = 0; j < cdi->selectedElementsArray[i]
				->structure.polyline->nPoints; j++) {
	   cdi->selectedElementsArray[i]->structure.polyline->points[j].x
		+= xOffset;
	   cdi->selectedElementsArray[i]->structure.polyline->points[j].y
		+= yOffset;
	}
     } else if (cdi->selectedElementsArray[i]->type == DL_Polygon) {
	for (j = 0; j < cdi->selectedElementsArray[i]
				->structure.polygon->nPoints; j++) {
	   cdi->selectedElementsArray[i]->structure.polygon->points[j].x
		+= xOffset;
	   cdi->selectedElementsArray[i]->structure.polygon->points[j].y
		+= yOffset;
	}
     }

    }
  }


/* (MDA) could use a new element-lookup based on region (write routine
 *	which returns all elements which intersect rather than are
 *	bounded by a given region) and do partial traversal based on
 *	those elements in start and end regions.  this could be much
 *	more efficient and not suffer from the "flash" updates
 */
  dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
}





/*
 * recursive function to resize Composite objects (and all children, which
 *	may be Composite objects)
 *	N.B.  this is relative to outermost composite, not parent composite
 */
void resizeCompositeChildren(DisplayInfo *cdi, DlElement *outerComposite,
	DlElement *composite, float scaleX, float scaleY)
{
  DlElement *ele, *child;
  Widget widget = (Widget)NULL, childWidget = (Widget)NULL;
  int deltaX, deltaY, dX, dY;
  int j, minX, maxX, minY, maxY;


  if (composite->type == DL_Composite) {

   ele = ((DlElement *)composite->structure.composite->dlElementListHead)->next;

   while (ele != NULL) {

    if (ELEMENT_IS_RENDERABLE(ele->type)) {

/* lookup widget now, before resized */
     if (ELEMENT_HAS_WIDGET(ele->type)) {
	widget = lookupElementWidget(cdi,
		&(ele->structure.rectangle->object));
     }

/* since relative resize within larger composite, x/y resize is relative... */
     if (ele->type == DL_Composite) {
	resizeCompositeChildren(cdi, outerComposite, ele, scaleX, scaleY);
     } else {
       deltaX = (scaleX*(ele->structure.rectangle->object.x
		- outerComposite->structure.composite->object.x));
       deltaY = (scaleY*(ele->structure.rectangle->object.y
		- outerComposite->structure.composite->object.y));

/* extra work for polyline/polygon - resize/rescale constituent points */
       if (ele->type == DL_Polyline ) {
	    for (j = 0; j < ele->structure.polyline->nPoints; j++) {
		dX = (scaleX*(ele->structure.polyline->points[j].x
			- outerComposite->structure.composite->object.x));
		dY = (scaleY*(ele->structure.polyline->points[j].y
			- outerComposite->structure.composite->object.y));
	        ele->structure.polyline->points[j].x = dX
			+ outerComposite->structure.composite->object.x;
	        ele->structure.polyline->points[j].y =  dY
			+ outerComposite->structure.composite->object.y;
	    }
       } else if (ele->type == DL_Polygon ) {
	    for (j = 0; j < ele->structure.polygon->nPoints; j++) {
		dX = (scaleX*(ele->structure.polygon->points[j].x
			- outerComposite->structure.composite->object.x));
		dY = (scaleY*(ele->structure.polygon->points[j].y
			- outerComposite->structure.composite->object.y));
	        ele->structure.polygon->points[j].x = dX
			+ outerComposite->structure.composite->object.x;
	        ele->structure.polygon->points[j].y =  dY
			+ outerComposite->structure.composite->object.y;
	    }
       }
       ele->structure.rectangle->object.x = (Position)
		(deltaX + outerComposite->structure.composite->object.x);
       ele->structure.rectangle->object.y = (Position)
		(deltaY + outerComposite->structure.composite->object.y);
       ele->structure.rectangle->object.width = (Dimension)
		(scaleX*ele->structure.rectangle->object.width+0.5);
       ele->structure.rectangle->object.height = (Dimension)
		(scaleY*ele->structure.rectangle->object.height+0.5);
     }

/*
 * Use expensive but reliable destroy-update-recreate sequence to get
 *	resizing right.  (XtResizeWidget mostly worked here, except for
 *	aggregate types like menu which had children which really defined
 *	it's parent's size.)
 * One additional advantage - this method guarantees WYSIWYG wrt fonts, etc
 *	but only do this if in EDIT mode - for execute mode this is handled
 *	by full re-traversal elsewhere
 */
     if (widget != NULL && globalDisplayListTraversalMode == DL_EDIT) {
/* destroy old widget */
	  destroyElementWidget(cdi,widget);
/* create new widget */
	  (*ele->dmExecute)(cdi,ele->structure.file,FALSE);
     }

    }
    ele = ele->next;
   }

 /* recalculate composite's dimensions/position */
    minX = INT_MAX; minY = INT_MAX;
    maxX = INT_MIN; maxY = INT_MIN;
    ele =((DlElement *)composite->structure.composite->dlElementListHead)->next;
    while (ele != NULL) {
      if (ELEMENT_IS_RENDERABLE(ele->type)) {
	minX = MIN(minX,ele->structure.rectangle->object.x);
	maxX = MAX(maxX,ele->structure.rectangle->object.x +
                (int)ele->structure.rectangle->object.width);
	minY = MIN(minY,ele->structure.rectangle->object.y);
	maxY = MAX(maxY,ele->structure.rectangle->object.y +
                (int)ele->structure.rectangle->object.height);
      }
      ele = ele->next;
    }
    composite->structure.composite->object.x = minX;
    composite->structure.composite->object.y = minY;
    composite->structure.composite->object.width = maxX - minX;
    composite->structure.composite->object.height = maxY - minY;

  }
}



/*
 * update all currently selected elements and rerender
 *	note: elements are only resized if their size remains physical 
 *		
 */
void updateResizedElements(Position x0, Position y0, Position x1, Position y1)
{
  int i, j, xOffset, yOffset, width, height, oldWidth, oldHeight;
  DisplayInfo *cdi;
  Widget widget;
  float sX, sY;
  DlElement *ele;

/* if no current display or selected elements array, simply return */
  if (currentDisplayInfo == NULL) return;
  if (currentDisplayInfo->selectedElementsArray == NULL) return;

  cdi = currentDisplayInfo;

  xOffset = x1 - x0;
  yOffset = y1 - y0;

/*
 * Use expensive but reliable destroy-update-recreate sequence to get
 *	resizing right.  (XtResizeWidget mostly worked here, except for
 *	aggregate types like menu which had children which really defined
 *	it's parent's size.)
 * One additional advantage - this method guarantees WYSIWYG wrt fonts, etc
 */

/* as usual, type in union unimportant as long as object is 1st thing...*/
  for (i = 0; i < cdi->numSelectedElements; i++) {

    ele = cdi->selectedElementsArray[i];

    if (ele->type != DL_Display && ELEMENT_IS_RENDERABLE(ele->type) ) {

     if (ele->type == DL_Composite) {

/* simple element resize for composite itself, then resize of children */
      oldWidth = ele->structure.composite->object.width;
      oldHeight = ele->structure.composite->object.height;
      width = (ele->structure.composite->object.width + xOffset);
      width = MAX(1,width);
      height = (ele->structure.composite->object.height + yOffset);
      height = MAX(1,height);
      sX = (float) ((float)width/(float)oldWidth);
      sY = (float) ((float)height/(float)oldHeight);
      resizeCompositeChildren(cdi,ele,ele,sX,sY);

      ele->structure.composite->object.width = (Dimension) width;
      ele->structure.composite->object.height = (Dimension) height;

     } else {

      if (!ELEMENT_HAS_WIDGET(ele->type)) {
/* extra work for polyline/polygon - resize/rescale constituent points */
	  if (ele->type == DL_Polyline ) {
	    sX = (float)((float)(ele->structure.polyline->object.width
		+ xOffset) / (float)(ele->structure.polyline->object.width));
	    sY = (float)((float)(ele->structure.polyline->object.height
		+ yOffset) / (float)(ele->structure.polyline->object.height));
	    for (j = 0; j < ele->structure.polyline->nPoints; j++) {
	        ele->structure.polyline->points[j].x = 
		   ele->structure.polyline->object.x +
		   	sX*(ele->structure.polyline->points[j].x
			- ele->structure.polyline->object.x);
	        ele->structure.polyline->points[j].y = 
		   ele->structure.polyline->object.y +
		   	sY*(ele->structure.polyline->points[j].y
			- ele->structure.polyline->object.y);
	    }
	  } else if (cdi->selectedElementsArray[i]->type == DL_Polygon ) {
	    sX = (float)((float)(ele->structure.polygon->object.width
		+ xOffset) / (float)(ele->structure.polygon->object.width));
	    sY = (float)((float)(ele->structure.polygon->object.height
		+ yOffset) / (float)(ele->structure.polygon->object.height));
	    for (j = 0; j < ele->structure.polygon->nPoints; j++) {
	        ele->structure.polygon->points[j].x = 
		   ele->structure.polyline->object.x +
			sX*(ele->structure.polygon->points[j].x
			- ele->structure.polygon->object.x);
	        ele->structure.polygon->points[j].y = 
		   ele->structure.polyline->object.y +
			sY*(ele->structure.polygon->points[j].y
			- ele->structure.polygon->object.y);
	    }
	  }
/* simple element resize */
	  width = (
	     cdi->selectedElementsArray[i]->structure.rectangle->object.width +
	     xOffset);
	  width = MAX(1,width);
	  cdi->selectedElementsArray[i]->structure.rectangle->object.width =
	     (Dimension) width;
	  height = (
	    cdi->selectedElementsArray[i]->structure.rectangle->object.height +
	    yOffset);
	  height = MAX(1,height);
	  cdi->selectedElementsArray[i]->structure.rectangle->object.height =
	    height;

      } else {
/* element has widget - more complicated resize */
       widget = lookupElementWidget(cdi,
		&(cdi->selectedElementsArray[i]->structure.rectangle->object));
       if (widget != NULL) {
	if (widget != cdi->drawingArea) {
/* resize */
	  width = (
	     cdi->selectedElementsArray[i]->structure.rectangle->object.width +
	     xOffset);
	  width = MAX(1,width);
	  cdi->selectedElementsArray[i]->structure.rectangle->object.width =
	     (Dimension) width;
	  height = (
	    cdi->selectedElementsArray[i]->structure.rectangle->object.height +
	    yOffset);
	  height = MAX(1,height);
	  cdi->selectedElementsArray[i]->structure.rectangle->object.height =
	    height;

	  if (widget != NULL && globalDisplayListTraversalMode == DL_EDIT) {
/* destroy old widget */
	    destroyElementWidget(cdi,widget);
/* create new widget */
	    (*cdi->selectedElementsArray[i]->dmExecute)(cdi,
	      cdi->selectedElementsArray[i]->structure.file,FALSE);
	  }
	}
       }
      }
     }
    }
  }


/* (MDA) could use a new element-lookup based on region (write routine
 *	which returns all elements which intersect rather than are
 *	bounded by a given region) and do partial traversal based on
 *	those elements in start and end regions.  this could be much
 *	more efficient and not suffer from the "flash" updates
 */
  dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
}





/* 
 * handle creates (based on filled in globalResourceBundle values and
 *	currentElementType);
 *
 */
void handleRectangularCreates(
  DlElementType elementType)
{
  DlElement *element, **array;
  Widget widget;


    element = (DlElement *) NULL;

/*
 *  if element could have basic or dynamic attributes, then create them if appr.
 */
    if (!ELEMENT_HAS_WIDGET(elementType) && elementType != DL_TextUpdate) {
/* create a basic attribute */
      element = createDlBasicAttribute(currentDisplayInfo);
      (*element->dmExecute)(currentDisplayInfo,
			element->structure.basicAttribute,FALSE);
/* create a dynamic attribute if appropriate */
      if (strlen(globalResourceBundle.chan) > 0 &&
        globalResourceBundle.vis != V_STATIC) {
        element = createDlDynamicAttribute(currentDisplayInfo);
        (*element->dmExecute)(currentDisplayInfo,
			element->structure.dynamicAttribute,FALSE);
      }
    }

/* now create the actual element */
    switch(elementType) {

	  case DL_Image:
	    handleImageCreate();
	    break;

/* others are more straight-forward */
	  case DL_Valuator:
	    element = createDlValuator(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.valuator,FALSE);
	    break;
	  case DL_ChoiceButton:
	    element = createDlChoiceButton(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.choiceButton,FALSE);
	    break;
	  case DL_MessageButton:
	    element = createDlMessageButton(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.messageButton,FALSE);
	    break;
	  case DL_TextEntry:
	    element = createDlTextEntry(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.textEntry,FALSE);
	    break;
	  case DL_Menu:
	    element = createDlMenu(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.menu,FALSE);
	    break;
	  case DL_Meter:
	    element = createDlMeter(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.meter,FALSE);
	    break;
	  case DL_TextUpdate:
	    element = createDlTextUpdate(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.textUpdate,FALSE);
	    break;
	  case DL_Bar:
	    element = createDlBar(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.bar,FALSE);
	    break;
	  case DL_Indicator:
	    element = createDlIndicator(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.indicator,FALSE);
	    break;
	  case DL_StripChart:
	    element = createDlStripChart(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.stripChart,FALSE);
	    break;
	  case DL_CartesianPlot:
	    element = createDlCartesianPlot(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.cartesianPlot,FALSE);
	    break;
	  case DL_SurfacePlot:
	    element = createDlSurfacePlot(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.surfacePlot,FALSE);
	    break;
	  case DL_Rectangle:
	    element = createDlRectangle(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.rectangle,FALSE);
	    break;
	  case DL_Oval:
	    element = createDlOval(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.oval,FALSE);
	    break;
	  case DL_Arc:
	    element = createDlArc(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.arc,FALSE);
	    break;
	  case DL_RelatedDisplay:
	    element = createDlRelatedDisplay(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.relatedDisplay,FALSE);
	    break;
	  case DL_ShellCommand:
	    element = createDlShellCommand(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.shellCommand,FALSE);
	    break;
	  case DL_Text:
	  /* for rectangular create of text, clear the field *
	   *  (like direct typing style entry)		     */
	    globalResourceBundle.textix[0] = '\0';
	    element = createDlText(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.text,FALSE);
	    break;

/*** (MDA) ***
	  case DL_BezierCurve:
	    element = createDlBezierCurve(currentDisplayInfo);
	    (*element->dmExecute)(currentDisplayInfo,
			element->structure.bezierCurve,FALSE);
	    break;

 ***/

	  default:
	    fprintf(stderr,"handleRectangularCreates: CREATE - invalid type %d",
		elementType);
	    break;


    }


    if (element != NULL) {
       unhighlightSelectedElements();
       unselectSelectedElements();
       clearResourcePaletteEntries();
       array = (DlElement **) malloc(1*sizeof(DlElement *));
       array[0] = element;
       highlightAndSetSelectedElements(array,1,1);
       setResourcePaletteEntries();
    }
}
