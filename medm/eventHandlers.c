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

#include "medm.h"
#include <X11/IntrinsicP.h>


extern Widget resourceMW, resourceS;
extern Widget objectPaletteSelectToggleButton;
extern XButtonPressedEvent lastEvent;

void toggleSelectedElementHighlight(DlElement *element);

static DlList *tmpDlElementList = NULL;

int initEventHandlers() {
    if (tmpDlElementList) return 0;
    if (tmpDlElementList = createDlList()) {
	return 0;
    } else {
	return -1;
    }
}

/*
 * event handlers
 */

#ifdef __cplusplus
void popupMenu(
  Widget,
  XtPointer cd,
  XEvent *event,
  Boolean *)
#else
void popupMenu(
  Widget w,
  XtPointer cd,
  XEvent *event,
  Boolean *ctd)
#endif
{
    DisplayInfo *displayInfo = (DisplayInfo *) cd;
    XButtonEvent *xEvent = (XButtonEvent *)event;
    Widget widget;
    DlElement *element;

    if (displayInfo != currentDisplayInfo) {
	currentDisplayInfo = displayInfo;
	currentColormap = currentDisplayInfo->colormap;
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
	    lastEvent = *((XButtonPressedEvent *)event);
	    XmMenuPosition(currentDisplayInfo->editPopupMenu,
	      (XButtonPressedEvent *)event);
	    XtManageChild(currentDisplayInfo->editPopupMenu);
	    XtPopup(XtParent(currentDisplayInfo->editPopupMenu),XtGrabNone);
	    XRaiseWindow(display,XtWindow(currentDisplayInfo->editPopupMenu));
        
	} else {
	  /*
	   * in EXECUTE mode, MB3 can also mean things based on where it occurs,
	   *   hence, lookup to see if MB3 occured in an object that cares
	   */
	    element = lookupElement(displayInfo->dlElementList,
	      (Position)xEvent->x, (Position)xEvent->y);
	    if (element) {
		switch(element->type) {
		case DL_Valuator:
		    if (widget = element->widget) {
			popupValuatorKeyboardEntry(widget,displayInfo,event);
			XUngrabPointer(display,CurrentTime);
		    }
		    break;

		case DL_CartesianPlot:
		    if (widget = element->widget) {
		      /* update globalResourceBundle with this element's info */
			executeTimeCartesianPlotWidget = widget;
			updateGlobalResourceBundleFromElement(element);
			if (!cartesianPlotAxisS) {
			    cartesianPlotAxisS = createCartesianPlotAxisDialog(mainShell);
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
		    XtPopup(XtParent(currentDisplayInfo->executePopupMenu),XtGrabNone);
		    XRaiseWindow(display,XtWindow(currentDisplayInfo->executePopupMenu));
		    break;
		}
	    } else {
	      /* execute menu does have valid/unique displayInfo ptr, hence use it */
		XmMenuPosition(displayInfo->executePopupMenu,(XButtonPressedEvent *)event);
		XtManageChild(displayInfo->executePopupMenu);
	    }
	}
    } else 
      if (xEvent->button == Button1 &&
	globalDisplayListTraversalMode == DL_EXECUTE) {
	  if (element = lookupElement(displayInfo->dlElementList,
	    (Position)xEvent->x, (Position)xEvent->y)) {
	      if (element->type == DL_RelatedDisplay &&
		element->structure.relatedDisplay->visual == RD_HIDDEN_BTN) {
		  relatedDisplayCreateNewDisplay(displayInfo,
		    &(element->structure.relatedDisplay->display[0]));
	      }
	  }
      }
}

#ifdef __cplusplus
void popdownMenu(
  Widget,
  XtPointer cd,
  XEvent *event,
  Boolean *)
#else
void popdownMenu(
  Widget w,
  XtPointer cd,
  XEvent *event,
  Boolean *ctd)
#endif
{
    DisplayInfo *displayInfo = (DisplayInfo *) cd;
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



#ifdef __cplusplus
void handleEnterWindow(
  Widget,
  XtPointer cd,
  XEvent *,
  Boolean *)
#else
void handleEnterWindow(
  Widget w,
  XtPointer cd,
  XEvent *event,
  Boolean *ctd)
#endif
{
    pointerInDisplayInfo = (DisplayInfo *) cd;
}


#ifdef __cplusplus
void handleButtonPress(
  Widget w,
  XtPointer clientData,
  XEvent *event,
  Boolean *)
#else
void handleButtonPress(
  Widget w,
  XtPointer clientData,
  XEvent *event,
  Boolean *continueToDispatch)
#endif
{
    DisplayInfo *displayInfo = (DisplayInfo *) clientData;
    XButtonEvent *xEvent = (XButtonEvent *)event;
    int j, k;
    Position x0, y0, x1, y1;
    Dimension daWidth, daHeight;
    Boolean validDrag, validResize;
    int minSize;
    XEvent newEvent;
    Boolean objectDataOnly, foundVertex = False, foundPoly = False;
    int newEventType;
    DisplayInfo *di;
    DlElement *dlElement;

  /* if in execute mode - update currentDisplayInfo and simply return */
    if (globalDisplayListTraversalMode == DL_EXECUTE) {
	currentDisplayInfo = displayInfo;
	return;
    }

  /* loop over all displays, unhighlight and unselect previously
   * selected elements
   */

    di = displayInfoListHead->next;
    while (di) {
	if (di != displayInfo) {
	    currentDisplayInfo = di;
	    unhighlightSelectedElements();
	    destroyDlDisplayList(di->selectedDlElementList);
	}
	di = di->next;
    }

    currentDisplayInfo = displayInfo;
    currentColormap = currentDisplayInfo->colormap;
    currentColormapSize = currentDisplayInfo->dlColormapSize;

  /* and always make sure the window is on top and has input focus */
    XRaiseWindow(display,XtWindow(displayInfo->shell));
    XSetInputFocus(display,XtWindow(displayInfo->shell),
      RevertToParent,CurrentTime);

    x0 = event->xbutton.x;
    y0 = event->xbutton.y;
    if (w != displayInfo->drawingArea) {
	Dimension dx0, dy0;
      /* other widget trapped the button press, therefore "normalize" */
	XtVaGetValues(w,XmNx,&dx0,XmNy,&dy0,NULL);
	x0 += dx0;
	y0 += dy0;
    }

  /*
   * handle differently, based on currentActionType
   */

  /* change drawingArea's cursor back to the appropriate cursor */
    XDefineCursor(display,XtWindow(currentDisplayInfo->drawingArea),
      (currentActionType == SELECT_ACTION ?
	rubberbandCursor : crosshairCursor));


  /**********************************
  ***       SELECT_ACTION        ***
  **********************************/

    if (currentActionType == SELECT_ACTION) {

      /****************************************
       * MB1       =  select (rubberband)     *
       * shift-MB1 =  multi-select            *
       * MB2       =  adjust (drag)           *
       * ctrl-MB2  =  adjust( resize)         *
       * MB3       =  menu                    *
       ****************************************/

	switch (xEvent->button) {
	case Button1:
	    if (xEvent->state & ControlMask) {
	      /* Ctrl-MB1 = NOTHING */
		break;
	    } else
	      if (xEvent->state & ShiftMask) {
		/* Shift-MB1 = toggle and append selections */
		  doRubberbanding(XtWindow(displayInfo->drawingArea),&x0,&y0,&x1,&y1);
		  selectedElementsLookup(displayInfo->dlElementList,
		    x0,y0,x1,y1,tmpDlElementList);
		  if (!IsEmpty(tmpDlElementList)) {
		      DlElement *pE = FirstDlElement(tmpDlElementList);
		      int found = False;
		      unhighlightSelectedElements();
		      while (pE) {
			  DlElement *pT =
			    FirstDlElement(displayInfo->selectedDlElementList);
			  int found = False;
			/* if found, remove it from the selected list */
			  while (pT) {
			      if (pT->structure.element == pE->structure.element) {
				  removeDlElement(displayInfo->selectedDlElementList,pT);
				  destroyDlElement(pT);
				  found = True;
				  break;
			      }
			      pT = pT->next;
			  }
			/* if not found, add it to the selected list */
			  if (!found) {
			      DlElement *pF = pE;
			      pE = pE->next;
			      removeDlElement(tmpDlElementList,pF);
			      appendDlElement(displayInfo->selectedDlElementList,pF);
			  } else {
			      pE = pE->next;
			  }
		      }
		      highlightSelectedElements();
		      destroyDlDisplayList(tmpDlElementList);
		  }
	      } else {
		/* handle the MB1 */
		/* see whether this is a vertex edit */
		  selectedElementsLookup(displayInfo->dlElementList,
		    x0,y0,x0,y0,tmpDlElementList);
		  if (NumberOfDlElement(displayInfo->selectedDlElementList) == 1) {
		      DlElement *dlElement =
			FirstDlElement(displayInfo->selectedDlElementList)
			->structure.element;
		      if (dlElement->run->editVertex) {
			  unhighlightSelectedElements();
			  if (dlElement->run->editVertex(dlElement,x0,y0)) {
			      foundVertex = True;
			  }
			  highlightSelectedElements();
		      }
		  }
		  destroyDlDisplayList(tmpDlElementList);
		/* if this is not a vertex edit */
		  if (!foundVertex) {
		      doRubberbanding(XtWindow(displayInfo->drawingArea),&x0,&y0,&x1,&y1);
		      selectedElementsLookup(displayInfo->dlElementList,
			x0,y0,x1,y1,tmpDlElementList);
		      if (!IsEmpty(tmpDlElementList)) {
			  unhighlightSelectedElements();
			  destroyDlDisplayList(displayInfo->selectedDlElementList);
			  appendDlList(displayInfo->selectedDlElementList,tmpDlElementList);
			  highlightSelectedElements();
			  destroyDlDisplayList(tmpDlElementList);
		      }
		  }
	      }
	    if (!foundVertex) {
		clearResourcePaletteEntries();
		if (NumberOfDlElement(currentDisplayInfo->selectedDlElementList)==1){
		    currentElementType =
		      FirstDlElement(currentDisplayInfo->selectedDlElementList)
                      ->structure.element->type;
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
	    selectedElementsLookup(displayInfo->dlElementList,
	      x0,y0,x0,y0,tmpDlElementList);
	    if (IsEmpty(tmpDlElementList)) break;

	    if (xEvent->state & ControlMask) {
	      /*********************/
	      /* Ctrl-MB2 = RESIZE */
	      /*********************/
		if (alreadySelected(FirstDlElement(tmpDlElementList))) {
		  /* element already selected - resize it and any others */
		  /* (MDA) ?? separate resize of display here? */
		  /* unhighlight currently selected elements */
		    unhighlightSelectedElements();
		  /* this element already selected: resize all selected elements */
		    validResize = doResizing(XtWindow(displayInfo->drawingArea),
		      x0,y0,&x1,&y1);
		    if (validResize) {
			updateResizedElements(x0,y0,x1,y1);
			if (currentDisplayInfo->hasBeenEditedButNotSaved == False) {
			    medmMarkDisplayBeingEdited(currentDisplayInfo);
			}
		    }
		  /* highlight currently selected elements */
		    highlightSelectedElements();
		  /* if only one selected, use first one (only) in list */
		    if (currentDisplayInfo->selectedDlElementList->count == 1) {
			objectDataOnly = True;
			updateGlobalResourceBundleAndResourcePalette(objectDataOnly);
		    }
		} else {
		  /* this element not already selected,
		     deselect others and resize it */
		    unhighlightSelectedElements();
		    destroyDlDisplayList(currentDisplayInfo->selectedDlElementList);
		    appendDlList(currentDisplayInfo->selectedDlElementList,
		      tmpDlElementList);
		    validResize = doResizing(XtWindow(displayInfo->drawingArea),
		      x0,y0,&x1,&y1);
		    if (validResize) {
			updateResizedElements(x0,y0,x1,y1);
			if (currentDisplayInfo->hasBeenEditedButNotSaved == False) {
			    medmMarkDisplayBeingEdited(currentDisplayInfo);
			}
		    }
		  /* highlight currently selected elements */
		    highlightSelectedElements();
		  /* if only one selected, use first one (only) in list */
		    clearResourcePaletteEntries();
		    if (currentDisplayInfo->selectedDlElementList->count == 1) {
			objectDataOnly = True;
			updateGlobalResourceBundleAndResourcePalette(objectDataOnly);
		    } 
		}
	    } else {
	      /************************/
	      /* MB2 = ADJUST (MOVE)  */
	      /************************/
		XtVaGetValues(displayInfo->drawingArea,
		  XmNwidth,&daWidth,
		  XmNheight,&daHeight,
		  NULL);
#if -1
		printf("\nTemp. element list :\n");
		dumpDlElementList(tmpDlElementList);
#endif
		if (alreadySelected(FirstDlElement(tmpDlElementList))) {
		  /* element already selected - move it and any others */
		  /* unhighlight currently selected elements */
		    unhighlightSelectedElements();
		  /* this element already selected: move all selected elements */
		    validDrag = doDragging(XtWindow(displayInfo->drawingArea),
		      daWidth,daHeight,x0,y0,&x1,&y1);
		    if (validDrag) {
			updateDraggedElements(x0,y0,x1,y1);
			if (currentDisplayInfo->hasBeenEditedButNotSaved == False) {
			    medmMarkDisplayBeingEdited(currentDisplayInfo);
			}
		    }
		  /* highlight currently selected elements */
		    highlightSelectedElements();
		  /* if only one selected, use first one (only) in list */
		    if (currentDisplayInfo->selectedDlElementList->count == 1) {
			objectDataOnly = True;
			updateGlobalResourceBundleAndResourcePalette(objectDataOnly);
		    }
		} else {
		  /* this element not already selected,
		     deselect others and move it */
		    unhighlightSelectedElements();
		    destroyDlDisplayList(currentDisplayInfo->selectedDlElementList);
		    appendDlList(currentDisplayInfo->selectedDlElementList,
		      tmpDlElementList);
		    validDrag = doDragging(XtWindow(displayInfo->drawingArea),
		      daWidth,daHeight,x0,y0,&x1,&y1);
		    printf("x0=%d, y0=%d, x1=%d, y1=%d\n",x0,y0,x1,y1);
		    if (validDrag) {
			updateDraggedElements(x0,y0,x1,y1);
			if (currentDisplayInfo->hasBeenEditedButNotSaved == False) {
			    medmMarkDisplayBeingEdited(currentDisplayInfo);
			}
		    }
		  /* highlight currently selected elements */
		    highlightSelectedElements();
		  /* if only one selected, use first one (only) in list */
		    if (currentDisplayInfo->selectedDlElementList->count ==1 ) {
			objectDataOnly = True;
			updateGlobalResourceBundleAndResourcePalette(objectDataOnly);
		    }
		}
	    }
	    destroyDlDisplayList(tmpDlElementList);
	    break;

	case Button3:
	  /************************************************************/
	  /* MB3 = MENU  --- this is really handled in                */
	  /*                 popupMenu/popdownMenu handler            */
	  /************************************************************/
	    break;
	}
    } else
      if (currentActionType == CREATE_ACTION) {
	/**********************************
	***			CREATE_ACTION					***
	**********************************/
	  DlElement *dlElement = 0;
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
	    /* see how to handle text -
	       either rectangular-create or type-create */
	      XWindowEvent(display,XtWindow(displayInfo->drawingArea),
		ButtonReleaseMask|Button1MotionMask,&newEvent);
	      newEventType = newEvent.type;
	      XPutBackEvent(display,&newEvent);

	      if (currentElementType == DL_Text &&
		newEventType == ButtonRelease) {
		  dlElement = handleTextCreate(x0,y0);
	      } else if (currentElementType == DL_Polyline) {
		  dlElement = handlePolylineCreate(x0,y0,(Boolean)False);
	      } else if (currentElementType == DL_Line) {
		  dlElement = handlePolylineCreate(x0,y0,(Boolean)True);
	      } else if (currentElementType == DL_Polygon) {
		  dlElement = handlePolygonCreate(x0,y0);
	      } else {
		  if (ELEMENT_HAS_WIDGET(currentElementType))
		    minSize = 12;
		  else
		    minSize = 2;
		/* actually create elements */
		  dlElement = handleRectangularCreates(currentElementType, x0, y0,
		    MAX(minSize,x1 - x0),MAX(minSize,y1 - y0));
	      }
	      if (dlElement) {
		  DlElement *pSE = 0;
		  if (currentDisplayInfo->hasBeenEditedButNotSaved == False) {
		      medmMarkDisplayBeingEdited(currentDisplayInfo);
		  }
		  appendDlElement(currentDisplayInfo->dlElementList,dlElement);
		  (*dlElement->run->execute)(currentDisplayInfo,dlElement);
		  
		  unhighlightSelectedElements();
		  destroyDlDisplayList(displayInfo->selectedDlElementList);
		  pSE = createDlElement(DL_Element,(XtPointer)dlElement,NULL);
		  if (pSE) {
		      appendDlElement(displayInfo->selectedDlElementList,pSE);
		  }
		  currentElementType =
		    FirstDlElement(currentDisplayInfo->selectedDlElementList)
		    ->structure.element->type;
		  highlightSelectedElements();
	      }
	      break;
	  case Button2:
	    /****************/
	    /* MB2 = ADJUST */
	    /****************/
	      break;

	  case Button3:
	    /********************************************/
	    /* MB3 = MENU  --- this is really           */
	    /* handled in popupMenu/popdownMenu handler */
	    /********************************************/
	      break;
	  }
#if -1
	  printf("\nselected element list :\n");
	  dumpDlElementList(displayInfo->selectedDlElementList);
#endif
	/* now toggle back to SELECT_ACTION from CREATE_ACTION */
	  if (objectS != NULL) {
	      XmToggleButtonSetState(objectPaletteSelectToggleButton,True,True);
	  }
	  setActionToSelect();
	  clearResourcePaletteEntries();
	  setResourcePaletteEntries();
      }
}

void highlightSelectedElements()
{
    DisplayInfo *cdi; /* abbreviation for currentDisplayInfo for clarity */
    DlElement *dlElement;

    if (!currentDisplayInfo) return;
    cdi = currentDisplayInfo;
    if (IsEmpty(cdi->selectedDlElementList)) return;
    if (cdi->selectedElementsAreHighlighted) return;
    cdi->selectedElementsAreHighlighted = True;
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while (dlElement) {
	toggleSelectedElementHighlight(dlElement->structure.element);
	dlElement = dlElement->next;
    }
}

/*
 *  function to unhighlight the currently highlighted (and  therfore
 *	selected) elements
 */
void unhighlightSelectedElements()
{
    DisplayInfo *pDI; /* abbreviation for currentDisplayInfo for clarity */
    DlElement *dlElement;

    if (!currentDisplayInfo) return;
    pDI = currentDisplayInfo;
    if (IsEmpty(pDI->selectedDlElementList)) return;
    if (!pDI->selectedElementsAreHighlighted) return;
    pDI->selectedElementsAreHighlighted = False;
    dlElement = FirstDlElement(pDI->selectedDlElementList);
    while (dlElement) {
	toggleSelectedElementHighlight(dlElement->structure.element);
	dlElement = dlElement->next;
    }
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
    int j;
    DisplayInfo *cdi;
    DlElement *dlElement = 0;

  /*
   *	N.B.: need to highlight in both window and pixmap in case
   *	there are expose events...
   */

  /* simply return if no current display */
    if (!currentDisplayInfo) return False;
    cdi = currentDisplayInfo;
    if (!IsEmpty(cdi->selectedDlElementList)) {
	DlElement *pE = FirstDlElement(cdi->selectedDlElementList);
	while (pE && !dlElement) {
	    if (pE->structure.element == element) dlElement = pE;
	    pE = pE->next;
	}
    }

  /* element not found - no changes */
    if (!dlElement) {
	return (False);
    }

  /* undraw the highlight */
    toggleSelectedElementHighlight(dlElement->structure.element);

    removeDlElement(cdi->selectedDlElementList,dlElement);
    dlElement->run->destroy(dlElement);
    return(True);
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
    Boolean moveWidgets;
    DlElement *pE;
    Display *display;
    GC gc;

  /* if no current display or selected elements array, simply return */
    if (!currentDisplayInfo) return;
    if (IsEmpty(currentDisplayInfo->selectedDlElementList)) return;

    cdi = currentDisplayInfo;
    display = XtDisplay(cdi->drawingArea);
    gc = cdi->gc;

    xOffset = x1 - x0;
    yOffset = y1 - y0;

    unhighlightSelectedElements();
  /* as usual, type in union unimportant as long as object is 1st thing...*/
    pE = FirstDlElement(currentDisplayInfo->selectedDlElementList);
    while (pE) {
	DlElement *dlElement = pE->structure.element;
	if (dlElement->run->move) 
	  dlElement->run->move(dlElement,xOffset,yOffset);
	if (dlElement->widget) 
	  dlElement->run->execute(currentDisplayInfo,dlElement);
	pE = pE->next;
    }
    dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
    highlightSelectedElements();
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
    DlElement *dlElement, *ele;

/* if no current display or selected elements array, simply return */
    if (!currentDisplayInfo) return;
    if (IsEmpty(currentDisplayInfo->selectedDlElementList)) return;

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
    dlElement = FirstDlElement(currentDisplayInfo->selectedDlElementList);
    while (dlElement) {
	DlElement *pE = dlElement->structure.element;
	if (pE->run->scale) {
	    pE->run->scale(pE,xOffset,yOffset);
	}
	if (pE->widget) {
	    pE->run->execute(currentDisplayInfo,pE);
	}
	dlElement = dlElement->next;
    }
    dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
}





/* 
 * handle creates (based on filled in globalResourceBundle values and
 *	currentElementType);
 *
 */
DlElement *handleRectangularCreates(
  DlElementType elementType,
  int x,
  int y,
  unsigned int width,
  unsigned int height)
{
    DlElement *element = 0, **array;

    element = (DlElement *) NULL;

/* now create the actual element */
    switch(elementType) {
    case DL_Image:
	element = handleImageCreate();
	break;
/* others are more straight-forward */
    case DL_Valuator:
	element = createDlValuator(NULL);
	break;
    case DL_ChoiceButton:
	element = createDlChoiceButton(NULL);
	break;
    case DL_MessageButton:
	element = createDlMessageButton(NULL);
	break;
    case DL_TextEntry:
	element = createDlTextEntry(NULL);
	break;
    case DL_Menu:
	element = createDlMenu(NULL);
	break;
    case DL_Meter:
	element = createDlMeter(NULL);
	break;
    case DL_TextUpdate:
	element = createDlTextUpdate(NULL);
	break;
    case DL_Bar:
	element = createDlBar(NULL);
	break;
    case DL_Indicator:
	element = createDlIndicator(NULL);
	break;
    case DL_Byte:
	element = createDlByte(NULL);
	break;
    case DL_StripChart:
	element = createDlStripChart(NULL);
	break;
    case DL_CartesianPlot:
	element = createDlCartesianPlot(NULL);
	break;
    case DL_Rectangle:
	element = createDlRectangle(NULL);
	break;
    case DL_Oval:
	element = createDlOval(NULL);
	break;
    case DL_Arc:
	element = createDlArc(NULL);
	break;
    case DL_RelatedDisplay:
	element = createDlRelatedDisplay(NULL);
	break;
    case DL_ShellCommand:
	element = createDlShellCommand(NULL);
	break;
    case DL_Text:
	element = createDlText(NULL);
	break;
    default:
	fprintf(stderr,"handleRectangularCreates: CREATE - invalid type %d",
	  elementType);
	break;
    }

    if (element) {
	if (element->run->inheritValues) {
	    element->run->inheritValues(&globalResourceBundle,element);
	}
	objectAttributeSet(&(element->structure.rectangle->object),x,y,width,height);
    }
    return element;
}

void toggleSelectedElementHighlight(DlElement *dlElement) {
    DisplayInfo *cdi = currentDisplayInfo;
    int x, y, width, height; 
    DlObject *po = &(dlElement->structure.display->object);

    if (dlElement->type == DL_Display) {
	x = HIGHLIGHT_LINE_THICKNESS;
	y = HIGHLIGHT_LINE_THICKNESS;
	width = po->width - 2*HIGHLIGHT_LINE_THICKNESS;
	height = po->height - 2*HIGHLIGHT_LINE_THICKNESS;
    } else {
	x = po->x-HIGHLIGHT_LINE_THICKNESS;
	y = po->y-HIGHLIGHT_LINE_THICKNESS;
	width = po->width + 2*HIGHLIGHT_LINE_THICKNESS;
	height = po->height + 2*HIGHLIGHT_LINE_THICKNESS;
    }
#if 0
    XSetForeground(display,highlightGC,cdi->drawingAreaBackgroundColor);
    XSetBackground(display,highlightGC,cdi->drawingAreaForegroundColor);
#endif
    XDrawRectangle(display,XtWindow(cdi->drawingArea),highlightGC,
      x, y, width, height);
    XDrawRectangle(display,cdi->drawingAreaPixmap,highlightGC,
      x, y, width, height);
}

void unselectSelectedElements() {
    return;
}
