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
 * .03  09-12-95        vong    conform to c++ syntax
 *
 *****************************************************************************
*/

#include "medm.h"

#include <Xm/ToggleBG.h>
#include <Xm/MwmUtil.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <cvtFast.h>
#ifdef __cplusplus
}
#endif

#ifdef EXTENDED_INTERFACE
# define N_MAIN_MENU_ELES 3
# define N_OPTIONS_MENU_ELES 1
# define OPTIONS_BTN_POSN 1
# define HELP_BTN_POSN 2
#else
# define N_MAIN_MENU_ELES 2
# define HELP_BTN_POSN 1
#endif

#define N_FILE_MENU_ELES 1
#define FILE_BTN_POSN 0
#define FILE_CLOSE_BTN 0
#define N_HELP_MENU_ELES 1




typedef struct {
  char *         pixmapName;
  Widget         widget;
  XtCallbackProc callback;
  XtPointer      clientData;
} buttons_t;

static void objectToggleCallback(Widget, XtPointer, XtPointer);

static XtCallbackProc objectToggleSelectCallback(
	   Widget, XtPointer, XmToggleButtonCallbackStruct *);

buttons_t paletteGraphicsButton[] = {
  {"rectangle25",NULL,objectToggleCallback,(XtPointer) DL_Rectangle},
  {"oval25",NULL,objectToggleCallback,(XtPointer) DL_Oval},
  {"arc25",NULL,objectToggleCallback,(XtPointer) DL_Arc},
  {"text25",NULL,objectToggleCallback,(XtPointer) DL_Text},
  {"polyline25",NULL,objectToggleCallback,(XtPointer) DL_Polyline},
  {"line25",NULL,objectToggleCallback,(XtPointer) DL_Line},
  {"polygon25",NULL,objectToggleCallback,(XtPointer) DL_Polygon},
  {"image25",NULL,objectToggleCallback,(XtPointer) DL_Image},
  NULL};
/* not implement
  {"bezierCurve25",NULL,objectToggleCallback,(XtPointer) DL_BezierCurve},
*/
buttons_t paletteMonitorButton[] = {
  {"meter25",NULL,objectToggleCallback,(XtPointer) DL_Meter},
  {"bar25",NULL,objectToggleCallback,(XtPointer) DL_Bar},
  {"stripChart25",NULL,objectToggleCallback,(XtPointer) DL_StripChart},
  {"textUpdate25",NULL,objectToggleCallback,(XtPointer) DL_TextUpdate},
  {"indicator25",NULL,objectToggleCallback,(XtPointer) DL_Indicator},
  {"cartesianPlot25",NULL,objectToggleCallback,(XtPointer) DL_CartesianPlot},
/* not implement
  {"surfacePlot25",NULL,objectToggleCallback,(XtPointer) DL_SurfacePlot},
*/
  {"byte25",NULL,objectToggleCallback,(XtPointer) DL_Byte},
  NULL};

buttons_t paletteControllerButton[] = {
  {"choiceButton25",NULL, objectToggleCallback,(XtPointer) DL_ChoiceButton},
  {"textEntry25",NULL, objectToggleCallback,(XtPointer) DL_TextEntry},
  {"messageButton25",NULL, objectToggleCallback,(XtPointer) DL_MessageButton},
  {"menu25",NULL, objectToggleCallback,(XtPointer) DL_Menu},
  {"valuator25",NULL, objectToggleCallback,(XtPointer) DL_Valuator},
  {"relatedDisplay25",NULL, objectToggleCallback,(XtPointer) DL_RelatedDisplay},
  {"shellCommand25",NULL, objectToggleCallback,(XtPointer) DL_ShellCommand},
  NULL};

buttons_t paletteMiscButton[] = {
  {"select25",NULL, objectToggleCallback,NULL},
  NULL};

buttons_t *buttonList[] = {
  paletteGraphicsButton,
  paletteMonitorButton,
  paletteControllerButton,
  NULL};

static Widget lastButton = NULL;

Widget objectFilePDM;
extern Widget importFSD;

extern XmString xmstringSelect;

/* last mouse position of the display before popup the menu */
extern XButtonPressedEvent lastEvent;


/*
 * global widget for ObjectPalette's SELECT toggle button (needed for
 *	programmatic toggle/untoggle of SELECT/CREATE modes
 */
Widget objectPaletteSelectToggleButton;

/********************************************
 **************** Callbacks *****************
 ********************************************/


/*
 * object palette's state transition callback - updates resource palette
 */
#ifdef __cplusplus
 void objectMenuCallback(
  Widget,
  XtPointer clientData,
  XtPointer)
#else
 void objectMenuCallback(
  Widget w,
  XtPointer clientData,
  XtPointer callbackStruct)
#endif
{
  DlElementType elementType = (DlElementType) clientData;
  DisplayInfo *di;

  /* move the pointer back the original location */
  di = currentDisplayInfo;
  XWarpPointer(display,None,XtWindow(di->drawingArea),0,0,
    0,0,lastEvent.x, lastEvent.y);

/* unhighlight and unselect any selected elements */
  unhighlightSelectedElements();
  unselectSelectedElements();
  clearResourcePaletteEntries();

/* set global action (from object palette) to CREATE, & global element type */
  currentActionType = CREATE_ACTION;
  currentElementType = elementType;
  setResourcePaletteEntries();
  di = displayInfoListHead->next;
  while(di != NULL) {
    XDefineCursor(display,XtWindow(di->drawingArea),crosshairCursor);
    di = di->next;
  }

  if (objectS) {
    int i,j;

    /* allow only one button pressed at a time */
    if (lastButton)
      XmToggleButtonGadgetSetState(lastButton,False,False);
    for (i = 0; buttonList[i] != NULL; i++) {
      for (j = 0; buttonList[i][j].pixmapName != NULL; j++) {
        if (buttonList[i][j].clientData == (XtPointer) elementType) {
	  lastButton = buttonList[i][j].widget;
  	  XmToggleButtonGadgetSetState(lastButton,True,False);
	  return;
        }
      }
    }
  }

  return;
}

void setActionToSelect() {

  DisplayInfo *di;
/* set global action (from object palette) to SELECT, not CREATE... */
  currentActionType = SELECT_ACTION;

/* since currentElementType is not really reset yet (don't know what is
 *	selected yet), clearResourcePaletteEntries() may not popdown
 *	these associated shells  - therefore use brute force */
  if (relatedDisplayS != NULL) XtPopdown(relatedDisplayS);
  if (cartesianPlotS != NULL) XtPopdown(cartesianPlotS);
  if (cartesianPlotAxisS != NULL) XtPopdown(cartesianPlotAxisS);
  if (stripChartS != NULL) XtPopdown(stripChartS);

/* clear out the resource palette to reflect empty/unselected object */
  if (currentDisplayInfo == NULL) {
    clearResourcePaletteEntries();
  } else {
    if (currentDisplayInfo->numSelectedElements != 1)
      clearResourcePaletteEntries();
  }

  if (objectS) 
    XDefineCursor(display,XtWindow(objectS),rubberbandCursor);
  di = displayInfoListHead->next;
  while(di != NULL) {
    XDefineCursor(display,XtWindow(di->drawingArea),rubberbandCursor);
    di = di->next;
  }
  return;
}

/*
 * object palette's state transition callback - updates resource palette
 */
static void objectToggleCallback(
  Widget w,
  XtPointer clientData,
  XtPointer callbackStruct)
{
  DisplayInfo *di;
  DlElementType elementType = (DlElementType) clientData;
  XmToggleButtonCallbackStruct *call_data = (XmToggleButtonCallbackStruct *) callbackStruct;

  /* pushing one of these toggles implies create object of this type,
   *      and MB in a display now takes on CREATE semantics
   */

  if (call_data->set == False) return;

  /* allow only one button pressed at a time */
  if ((lastButton) && (lastButton != w)) {
    XmToggleButtonGadgetSetState(lastButton,False,False);
    lastButton = w;
  }

  if (w == objectPaletteSelectToggleButton) {
    setActionToSelect();
  } else {
    /* unhighlight and unselect any selected elements */
    unhighlightSelectedElements();
    unselectSelectedElements();
    clearResourcePaletteEntries();

    /* set global action (from object palette) to CREATE, & global element type */
    currentActionType = CREATE_ACTION;
    currentElementType = elementType;
    setResourcePaletteEntries();
    XDefineCursor(display,XtWindow(objectS),crosshairCursor);
    di = displayInfoListHead->next;
    while(di != NULL) {
      XDefineCursor(display,XtWindow(di->drawingArea),crosshairCursor);
      di = di->next;
    }
  }
  return;
}


#ifdef __cplusplus
static void fileMenuSimpleCallback(
  Widget,
  XtPointer clientData,
  XtPointer)
#else
static void fileMenuSimpleCallback(
  Widget w,
  XtPointer clientData,
  XtPointer callbackStruct)
#endif
{
  int buttonNumber = (int) clientData;
  switch(buttonNumber) {
    case FILE_CLOSE_BTN:
      XtPopdown(objectS);
  }
}




#ifdef EXTENDED_INTERFACE
static XtCallbackProc optionsMenuSimpleCallback(
  Widget w,
  int buttonNumber,
  XmAnyCallbackStruct *call_data)
{
    switch(buttonNumber) {
	    case OPTIONS_USER_PALETTE_BTN:
		break;
    }
}
#endif


Widget createRadioButtonPanel(Widget parent, char* name, buttons_t *b) {
  Widget background, label, rowCol;
  Pixel fg, bg;
  int i, col;

  background = XtVaCreateWidget("background",
	    xmRowColumnWidgetClass, parent,
	    XmNpacking, XmPACK_TIGHT,
	    NULL);

  label = XtVaCreateWidget(name,
	    xmLabelWidgetClass, background,
	    NULL);

  rowCol = XtVaCreateWidget("rowCol",
	    xmRowColumnWidgetClass, background,
	    XmNorientation, XmVERTICAL,
	    XmNpacking, XmPACK_COLUMN,
	    NULL);

  XtVaGetValues(rowCol,
    XmNforeground, &fg,
    XmNbackground, &bg,
    NULL);
  for (i=0; b[i].pixmapName != NULL; i++) {
    Pixmap pixmap;

    pixmap = XmGetPixmap(XtScreen(rowCol),
	       b[i].pixmapName,
	       fg, bg);
    b[i].widget = XtVaCreateManagedWidget(b[i].pixmapName,
	xmToggleButtonGadgetClass, rowCol,
        XmNlabelType, XmPIXMAP,
	XmNmarginTop, 0,
	XmNmarginBottom, 0,
	XmNmarginLeft, 0,
	XmNmarginRight, 0,
	XmNmarginWidth, 0,
	XmNmarginHeight, 0,
	XmNwidth, 29,
	XmNheight, 29,
	XmNpushButtonEnabled, True,
	XmNhighlightThickness, 0,
	XmNalignment, XmALIGNMENT_CENTER,
	XmNlabelPixmap, pixmap,
	XmNindicatorOn, False,
	XmNrecomputeSize, False,
	NULL);
    XtAddCallback(b[i].widget,XmNvalueChangedCallback,b[i].callback,b[i].clientData);
  }
  if (((i>>1)<<1) == i) { /* check for odd/even */
    col = i >> 1;
  } else {
    col = (i >> 1) + 1;
  }
  if (col <= 0) col = 1;
  XtVaSetValues(rowCol, XmNnumColumns, col, NULL);
  XtManageChild(rowCol);
  XtManageChild(background);
  XtManageChild(label);
  return rowCol;
}

static menuEntry_t fileMenu[] = {
  { "Close",  &xmPushButtonGadgetClass, 'C', NULL, NULL, NULL,
    fileMenuSimpleCallback, (XtPointer) FILE_CLOSE_BTN,  NULL},
  NULL,
};

#ifdef EXTENDED_INTERFACE
static menuEntry_t optionMenu[] = {
  { "User Palette...",  &xmPushButtonGadgetClass, 'U', NULL, NULL, NULL,
    fileMenuSimpleCallback, (XtPointer) OPTION_USER_PALETTE_BTN,  NULL},
  NULL,
};
#endif

static menuEntry_t helpMenu[] = {
  { "On Object Palette...",  &xmPushButtonGadgetClass, 'O', NULL, NULL, NULL,
    NULL, NULL, NULL},
  NULL,
};

void createObject()
{
  Widget objectRC;
  Widget graphicsRC, monitorRC, controllerRC, miscRC;
  Widget objectMB;
  Widget objectHelpPDM;

 char name[20];
 Arg args[10];


/*
 * initialize local static globals
 */
 importFSD = NULL;

/*
 * create a MainWindow in a shell, and then the palette radio box
 */
/* map window manager menu Close function to application close... */

 objectS = XtVaCreatePopupShell("objectS",
	     topLevelShellWidgetClass,mainShell,
	     XtNiconName,"Objects",
	     XtNtitle,"Object Palette",
	     XtNallowShellResize,TRUE,
	     XmNkeyboardFocusPolicy,XmEXPLICIT,
	     XmNdeleteResponse,XmDO_NOTHING,
	     XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH,
	     NULL);

 XmAddWMProtocolCallback(objectS,WM_DELETE_WINDOW,
		(XtCallbackProc)wmCloseCallback,(XtPointer)OTHER_SHELL);

 objectMW = XmCreateMainWindow(objectS,"objectMW",NULL,0);


/*
 * create the menu bar
 */
  objectMB = XmCreateMenuBar(objectMW, "objectMB",NULL,0);

/* color objectMB properly (force so VUE doesn't interfere) */
  colorMenuBar(objectMB,defaultForeground,defaultBackground);

/*
 * create the file pulldown menu pane
 */
  objectFilePDM = buildMenu(objectMB,XmMENU_PULLDOWN,
		    "File", 'F', fileMenu);


#ifdef EXTENDED_INTERFACE
/*
 * create the options pulldown menu pane
 */
  objectOptionPDM = buildMenu(objectMB,XmMENU_PULLDOWN,
		    "Option", 'O', optionMenu);
#endif


/*
 * create the help pulldown menu pane
 */
  objectHelpPDM = buildMenu(objectMB,XmMENU_PULLDOWN,
		    "Help", 'H', helpMenu);
  XtVaSetValues(objectMB, XmNmenuHelpWidget, objectHelpPDM, NULL);
  /* (MDA) for now, disable this menu */
  XtSetSensitive(objectHelpPDM,False);

/*
 * create work area Row Column
 */
  objectRC = XtVaCreateWidget("objectRC",
		 xmRowColumnWidgetClass, objectMW,
		 XmNorientation, XmHORIZONTAL,
		 NULL);

/* set main window areas */
  XmMainWindowSetAreas(objectMW,objectMB,NULL,NULL,NULL,objectRC);

  graphicsRC = createRadioButtonPanel(objectRC,"Graphics",paletteGraphicsButton);
  monitorRC = createRadioButtonPanel(objectRC,"Monitors",paletteMonitorButton);
  controllerRC = createRadioButtonPanel(objectRC,"Controllers",paletteControllerButton);
  miscRC = createRadioButtonPanel(objectRC,"Misc",paletteMiscButton);

  objectPaletteSelectToggleButton = paletteMiscButton[0].widget;
  lastButton = objectPaletteSelectToggleButton;
  XtVaSetValues(objectPaletteSelectToggleButton,
     XmNset, True,
     NULL);
     
  XtManageChild(objectMB);
  XtManageChild(objectRC);
  XtManageChild(objectMW);
}





/*
 * clear current resourcePalette entries
 */
void clearResourcePaletteEntries()
{

/* if no resource palette yet, simply return */
  if (resourceMW == NULL) return;

/* popdown any of the associated shells */
  if (relatedDisplayS != NULL) XtPopdown(relatedDisplayS);
  if (shellCommandS != NULL) XtPopdown(shellCommandS);
  if (cartesianPlotS != NULL) XtPopdown(cartesianPlotS);
  if (cartesianPlotAxisS != NULL) XtPopdown(cartesianPlotAxisS);
  if (stripChartS != NULL) XtPopdown(stripChartS);

/*
 * unsetting the current button: unmanage previous resource entries
 */

/* update current element type label in resourceMW  (to Select...) by default */
  XtVaSetValues(resourceElementTypeLabel,XmNlabelString,xmstringSelect,NULL);

/* must normalize back to 0 as index into array for element type */
  if (currentElementType >= MIN_DL_ELEMENT_TYPE &&
	currentElementType <= MAX_DL_ELEMENT_TYPE)
	XtUnmanageChildren(resourcePaletteElements[currentElementType 
				- MIN_DL_ELEMENT_TYPE].children,
		     resourcePaletteElements[currentElementType
				- MIN_DL_ELEMENT_TYPE].numChildren);
}






/*
 * set resourcePalette entries based on current type
 */
void setResourcePaletteEntries()
{
/* must normalize back to 0 as index into array for element type */
  XmString buttons[NUM_IMAGE_TYPES-1];
  XmButtonType buttonType[NUM_IMAGE_TYPES-1];
  Arg args[10];
  Boolean objectDataOnly;
  DlElementType displayType;


/* if no resource palette yet, create it */
  if (resourceMW == NULL) createResource();

/* make sure the resource palette shell is popped-up */
  XtPopup(resourceS,XtGrabNone);

/* make these sensitive in case they are managed */
  XtSetSensitive(resourceEntryRC[VIS_RC],True);
  XtSetSensitive(resourceEntryRC[CLRMOD_RC],True);
#ifdef __COLOR_RULE_H__
  XtSetSensitive(resourceEntryRC[COLOR_RULE_RC],True);
#endif

/* setting the new button: manage new resource entries */
    XtManageChildren(
      resourcePaletteElements[currentElementType -
				MIN_DL_ELEMENT_TYPE].children,
      resourcePaletteElements[currentElementType -
				MIN_DL_ELEMENT_TYPE].numChildren);

/* update current element type label in resourceMW */

  /* if polyline with 2 points display Line as label, not Polyline */
    displayType = currentElementType;
    if (currentElementType == DL_Polyline &&
	currentDisplayInfo->numSelectedElements == 1 &&
	currentDisplayInfo->selectedElementsArray[0]
		->structure.polyline->nPoints == 2) displayType = DL_Line;
    XtVaSetValues(resourceElementTypeLabel,
	XmNlabelString,elementXmStringTable[displayType -MIN_DL_ELEMENT_TYPE],
	NULL);

    if (currentDisplayInfo->selectedElementsArray == NULL) {

  /* restore globalResourceBundle and resource palette
   *	x/y/width/height to defaults (as in initializeResourceBundle)
   */
	resetGlobalResourceBundleAndResourcePalette();

    } else {

  /* in display edit  ---  update ALL of globalResourceBundle & res. Palette */
	objectDataOnly = False;
	updateGlobalResourceBundleAndResourcePalette(objectDataOnly);

    }

/* if not a monitor or controller type object, and no  dynamics channel
 *  specified, then insensitize the related entries */
    if (strlen(globalResourceBundle.chan) == 0) {
	XtSetSensitive(resourceEntryRC[VIS_RC],False);
	if ( (!ELEMENT_HAS_WIDGET(currentElementType)) &&
	     (currentElementType != DL_TextUpdate))
	      XtSetSensitive(resourceEntryRC[CLRMOD_RC],False);
#ifdef __COLOR_RULE_H__
        if (globalResourceBundle.clrmod != DISCRETE)
          XtSetSensitive(resourceEntryRC[COLOR_RULE_RC],False);
#endif
    }

  /* make these sensitive in case they are managed */
  if (strlen(globalResourceBundle.erase) == 0)
    XtSetSensitive(resourceEntryRC[ERASE_MODE_RC],False);
  else
    XtSetSensitive(resourceEntryRC[ERASE_MODE_RC],True);
}




/*
 * function to update the global resource bundle data structure
 * N.B. - must keep this in sync with
 *	updateGlobalResourceBundleAndResourcePalette()
 */

void updateGlobalResourceBundleFromElement(DlElement *elementPtr)
{
  DlElement *dyn, *basic;
  char string[MAX_TOKEN_LENGTH];
  int i;



/* simply return if not valid to update */
  if (elementPtr == NULL) return;


  if (ELEMENT_IS_RENDERABLE(elementPtr->type)) {

/* get object data: must have object entry  - use rectangle type (arbitrary) */
    globalResourceBundle.x = elementPtr->structure.rectangle->object.x;
    globalResourceBundle.y = elementPtr->structure.rectangle->object.y;
    globalResourceBundle.width = elementPtr->structure.rectangle->object.width;
    globalResourceBundle.height= elementPtr->structure.rectangle->object.height;


/***************************************************************************/

/* only worry about basic and dynamic attributes for objects without widgets */

   if (! ELEMENT_HAS_WIDGET(elementPtr->type) 
			&& elementPtr->type != DL_TextUpdate
			&& elementPtr->type != DL_Composite) {

/* get basicAttribute data */
    basic = lookupBasicAttributeElement(elementPtr);

    if (basic != NULL) {
      globalResourceBundle.clr = basic->structure.basicAttribute->attr.clr;
      globalResourceBundle.style = basic->structure.basicAttribute->attr.style;
      globalResourceBundle.fill = basic->structure.basicAttribute->attr.fill;
      globalResourceBundle.lineWidth =
		basic->structure.basicAttribute->attr.width;
    }

/* get dynamicAttribute data */
    dyn = lookupDynamicAttributeElement(elementPtr);

    if (dyn != NULL) {
      globalResourceBundle.clrmod =
		dyn->structure.dynamicAttribute->attr.mod.clr;
      globalResourceBundle.vis = dyn->structure.dynamicAttribute->attr.mod.vis;
#ifdef __COLOR_RULE_H__
      globalResourceBundle.colorRule = dyn->structure.dynamicAttribute->attr.mod.colorRule;
#endif
      strcpy(globalResourceBundle.chan,
		dyn->structure.dynamicAttribute->attr.param.chan);
    } else { 	/* reset to defaults for dynamics */
      globalResourceBundle.clrmod = STATIC;
      globalResourceBundle.vis = V_STATIC;
#ifdef __COLOR_RULE_H__
      globalResourceBundle.colorRule = 0;
#endif
      globalResourceBundle.chan[0] = '\0';
/* insensitize the other dyn fields since don't make sense if no channel
   specified */
      XtSetSensitive(resourceEntryRC[CLRMOD_RC],False);
      XtSetSensitive(resourceEntryRC[VIS_RC],False);
#ifdef __COLOR_RULE_H__
      XtSetSensitive(resourceEntryRC[COLOR_RULE_RC],False);
#endif
    }
   }
  }

  switch (elementPtr->type) {
      case DL_File:
	strcpy(globalResourceBundle.name,elementPtr->structure.file->name);
	break;

      case DL_Display:
	globalResourceBundle.clr = elementPtr->structure.display->clr;
	globalResourceBundle.bclr = elementPtr->structure.display->bclr;
	strcpy(globalResourceBundle.cmap,elementPtr->structure.display->cmap);
	break;

      case DL_Colormap:
	medmPrintf(
	"\nupdateGlobalResourceBundleFromElement:  in DL_Colormap type??");
	break;

      case DL_BasicAttribute:
      case DL_DynamicAttribute:
      /* (hopefully) taken care of in ELEMENT_IS_RENDERABLE part up there */
	break;

      case DL_Valuator:
	strcpy(globalResourceBundle.ctrl,
		elementPtr->structure.valuator->control.ctrl);
	globalResourceBundle.clr = elementPtr->structure.valuator->control.clr;
	globalResourceBundle.bclr =
		elementPtr->structure.valuator->control.bclr;
	globalResourceBundle.label = elementPtr->structure.valuator->label;
	globalResourceBundle.clrmod = elementPtr->structure.valuator->clrmod;
	globalResourceBundle.direction =
		elementPtr->structure.valuator->direction;
	globalResourceBundle.dPrecision =
		elementPtr->structure.valuator->dPrecision;
	break;

      case DL_ChoiceButton:
	strcpy(globalResourceBundle.ctrl,
		elementPtr->structure.choiceButton->control.ctrl);
	globalResourceBundle.clr =
		elementPtr->structure.choiceButton->control.clr;
	globalResourceBundle.bclr =
		elementPtr->structure.choiceButton->control.bclr;
	globalResourceBundle.clrmod =
		elementPtr->structure.choiceButton->clrmod;
	globalResourceBundle.stacking =
		elementPtr->structure.choiceButton->stacking;
	break;

      case DL_MessageButton:
	strcpy(globalResourceBundle.ctrl,
		elementPtr->structure.messageButton->control.ctrl);
	globalResourceBundle.clr =
		elementPtr->structure.messageButton->control.clr;
	globalResourceBundle.bclr =
		elementPtr->structure.messageButton->control.bclr;
	strcpy(globalResourceBundle.messageLabel,
		elementPtr->structure.messageButton->label);
	strcpy(globalResourceBundle.press_msg,
		elementPtr->structure.messageButton->press_msg);
	strcpy(globalResourceBundle.release_msg,
		elementPtr->structure.messageButton->release_msg);
	globalResourceBundle.clrmod =
		elementPtr->structure.messageButton->clrmod;
	break;

      case DL_TextEntry:
	strcpy(globalResourceBundle.ctrl,
		elementPtr->structure.textEntry->control.ctrl);
	globalResourceBundle.clr =
		elementPtr->structure.textEntry->control.clr;
	globalResourceBundle.bclr =
		elementPtr->structure.textEntry->control.bclr;
	globalResourceBundle.clrmod =
		elementPtr->structure.textEntry->clrmod;
	globalResourceBundle.format =
		elementPtr->structure.textEntry->format;
	break;

      case DL_Menu:
	strcpy(globalResourceBundle.ctrl,
		elementPtr->structure.menu->control.ctrl);
	globalResourceBundle.clr =
		elementPtr->structure.menu->control.clr;
	globalResourceBundle.bclr =
		elementPtr->structure.menu->control.bclr;
	globalResourceBundle.clrmod =
		elementPtr->structure.menu->clrmod;
	break;

      case DL_Meter:
	strcpy(globalResourceBundle.rdbk,
		elementPtr->structure.meter->monitor.rdbk);
	globalResourceBundle.clr =
		elementPtr->structure.meter->monitor.clr;
	globalResourceBundle.bclr =
		elementPtr->structure.meter->monitor.bclr;
	globalResourceBundle.label =
		elementPtr->structure.meter->label;
	globalResourceBundle.clrmod =
		elementPtr->structure.meter->clrmod;
	break;

      case DL_TextUpdate:
	strcpy(globalResourceBundle.rdbk,
		elementPtr->structure.textUpdate->monitor.rdbk);
	globalResourceBundle.clr =
		elementPtr->structure.textUpdate->monitor.clr;
	globalResourceBundle.bclr =
		elementPtr->structure.textUpdate->monitor.bclr;
	globalResourceBundle.clrmod =
		elementPtr->structure.textUpdate->clrmod;
	globalResourceBundle.align =
		elementPtr->structure.textUpdate->align;
	globalResourceBundle.format =
		elementPtr->structure.textUpdate->format;
	break;

      case DL_Bar:
	strcpy(globalResourceBundle.rdbk,
		elementPtr->structure.bar->monitor.rdbk);
	globalResourceBundle.clr =
		elementPtr->structure.bar->monitor.clr;
	globalResourceBundle.bclr =
		elementPtr->structure.bar->monitor.bclr;
	globalResourceBundle.label =
		elementPtr->structure.bar->label;
	globalResourceBundle.clrmod =
		elementPtr->structure.bar->clrmod;
	globalResourceBundle.direction =
		elementPtr->structure.bar->direction;
	globalResourceBundle.fillmod =
		elementPtr->structure.bar->fillmod;
	break;
      case DL_Byte:
        strcpy(globalResourceBundle.rdbk,
          elementPtr->structure.byte->monitor.rdbk);
        globalResourceBundle.clr = elementPtr->structure.byte->monitor.clr;
        globalResourceBundle.bclr = elementPtr->structure.byte->monitor.bclr;
        globalResourceBundle.clrmod = elementPtr->structure.byte->clrmod;
        globalResourceBundle.direction = elementPtr->structure.byte->direction;
        globalResourceBundle.sbit = elementPtr->structure.byte->sbit;
        globalResourceBundle.ebit = elementPtr->structure.byte->ebit;
        break;
      case DL_Indicator:
	strcpy(globalResourceBundle.rdbk,
		elementPtr->structure.indicator->monitor.rdbk);
	globalResourceBundle.clr =
		elementPtr->structure.indicator->monitor.clr;
	globalResourceBundle.bclr =
		elementPtr->structure.indicator->monitor.bclr;
	globalResourceBundle.label =
		elementPtr->structure.indicator->label;
	globalResourceBundle.clrmod =
		elementPtr->structure.indicator->clrmod;
	globalResourceBundle.direction =
		elementPtr->structure.indicator->direction;
	break;

      case DL_StripChart:
	strcpy(globalResourceBundle.title,
		elementPtr->structure.stripChart->plotcom.title);
	strcpy(globalResourceBundle.xlabel,
		elementPtr->structure.stripChart->plotcom.xlabel);
	strcpy(globalResourceBundle.ylabel,
		elementPtr->structure.stripChart->plotcom.ylabel);
	globalResourceBundle.clr =
		elementPtr->structure.stripChart->plotcom.clr;
	globalResourceBundle.bclr =
		elementPtr->structure.stripChart->plotcom.bclr;
	globalResourceBundle.period = 
		elementPtr->structure.stripChart->period;
	globalResourceBundle.units =
		elementPtr->structure.stripChart->units;
	for (i = 0; i < MAX_PENS; i++){
	  strcpy(globalResourceBundle.scData[i].chan,
		elementPtr->structure.stripChart->pen[i].chan);  
	  globalResourceBundle.scData[i].clr = 
		elementPtr->structure.stripChart->pen[i].clr;
	}
	break;

      case DL_CartesianPlot:
	strcpy(globalResourceBundle.title,
		elementPtr->structure.cartesianPlot->plotcom.title);
	strcpy(globalResourceBundle.xlabel,
		elementPtr->structure.cartesianPlot->plotcom.xlabel);
	strcpy(globalResourceBundle.ylabel,
		elementPtr->structure.cartesianPlot->plotcom.ylabel);
	globalResourceBundle.clr =
		elementPtr->structure.cartesianPlot->plotcom.clr;
	globalResourceBundle.bclr =
		elementPtr->structure.cartesianPlot->plotcom.bclr;
	globalResourceBundle.count = 
		elementPtr->structure.cartesianPlot->count;
	globalResourceBundle.cStyle =
		elementPtr->structure.cartesianPlot->style;
	globalResourceBundle.erase_oldest =
		elementPtr->structure.cartesianPlot->erase_oldest;
	for (i = 0; i < MAX_TRACES; i++){
	  strcpy(globalResourceBundle.cpData[i].xdata,
		elementPtr->structure.cartesianPlot->trace[i].xdata);  
	  strcpy(globalResourceBundle.cpData[i].ydata,
		elementPtr->structure.cartesianPlot->trace[i].ydata);  
	  globalResourceBundle.cpData[i].data_clr = 
		elementPtr->structure.cartesianPlot->trace[i].data_clr;
	}
	for (i = X_AXIS_ELEMENT; i <= Y2_AXIS_ELEMENT; i++) {
	  globalResourceBundle.axis[i].axisStyle =
		elementPtr->structure.cartesianPlot->axis[i].axisStyle;
	  globalResourceBundle.axis[i].rangeStyle =
		elementPtr->structure.cartesianPlot->axis[i].rangeStyle;
	  globalResourceBundle.axis[i].minRange = 
		elementPtr->structure.cartesianPlot->axis[i].minRange;
	  globalResourceBundle.axis[i].maxRange =
		elementPtr->structure.cartesianPlot->axis[i].maxRange;
	}
	strcpy(globalResourceBundle.trigger,
		elementPtr->structure.cartesianPlot->trigger);
	strcpy(globalResourceBundle.erase,
		elementPtr->structure.cartesianPlot->erase);
	globalResourceBundle.eraseMode =
		elementPtr->structure.cartesianPlot->eraseMode;
	break;

      case DL_SurfacePlot:
	strcpy(globalResourceBundle.title,
		elementPtr->structure.surfacePlot->plotcom.title);
	strcpy(globalResourceBundle.xlabel,
		elementPtr->structure.surfacePlot->plotcom.xlabel);
	strcpy(globalResourceBundle.ylabel,
		elementPtr->structure.surfacePlot->plotcom.ylabel);
	globalResourceBundle.clr =
		elementPtr->structure.surfacePlot->plotcom.clr;
	globalResourceBundle.bclr =
		elementPtr->structure.surfacePlot->plotcom.bclr;
	strcpy(globalResourceBundle.data,
		elementPtr->structure.surfacePlot->data);
	globalResourceBundle.data_clr = 
		elementPtr->structure.surfacePlot->data_clr;
	globalResourceBundle.dis = 
		elementPtr->structure.surfacePlot->dis;
	globalResourceBundle.xyangle = 
		elementPtr->structure.surfacePlot->xyangle;
	globalResourceBundle.zangle = 
		elementPtr->structure.surfacePlot->zangle;
	break;

      case DL_Rectangle:
      case DL_Oval:
	/* handled in prelude (since only DlObject description) */
	break;

      case DL_Arc:
	globalResourceBundle.begin = 
		elementPtr->structure.arc->begin;
	globalResourceBundle.path = 
		elementPtr->structure.arc ->path;
	break;

      case DL_Text:
	strcpy(globalResourceBundle.textix,
		elementPtr->structure.text->textix);
	globalResourceBundle.align = elementPtr->structure.text->align;
	break;

      case DL_RelatedDisplay:
	globalResourceBundle.clr =
		elementPtr->structure.relatedDisplay->clr;
	globalResourceBundle.bclr =
		elementPtr->structure.relatedDisplay->bclr;
	for (i = 0; i < MAX_RELATED_DISPLAYS; i++){
	  strcpy(globalResourceBundle.rdData[i].label,
		elementPtr->structure.relatedDisplay->display[i].label);  
	  strcpy(globalResourceBundle.rdData[i].name,
		elementPtr->structure.relatedDisplay->display[i].name);  
	  strcpy(globalResourceBundle.rdData[i].args,
		elementPtr->structure.relatedDisplay->display[i].args);  
	}
	break;

      case DL_ShellCommand:
	globalResourceBundle.clr =
		elementPtr->structure.shellCommand->clr;
	globalResourceBundle.bclr =
		elementPtr->structure.shellCommand->bclr;
	for (i = 0; i < MAX_SHELL_COMMANDS; i++){
	  strcpy(globalResourceBundle.cmdData[i].label,
		elementPtr->structure.shellCommand->command[i].label);  
	  strcpy(globalResourceBundle.cmdData[i].command,
		elementPtr->structure.shellCommand->command[i].command);  
	  strcpy(globalResourceBundle.cmdData[i].args,
		elementPtr->structure.shellCommand->command[i].args);  
	}
	break;


      case DL_Image:
	globalResourceBundle.imageType =
		elementPtr->structure.image->imageType;
	strcpy(globalResourceBundle.imageName,
		elementPtr->structure.image->imageName);
	break;

      case DL_Composite:
/* need to finish this if we want named groups
	strcpy(globalResourceBundle.compositeName,
		elementPtr->structure.composite->compositeName);
 */
        globalResourceBundle.vis = elementPtr->structure.composite->vis;
        strcpy(globalResourceBundle.chan,elementPtr->structure.composite->chan);
	break;
      case DL_Line:
	break;
      case DL_Polyline:
	break;
      case DL_Polygon:
	break;
      case DL_BezierCurve:
	break;

      default:
	medmPrintf(   
	"\n updateGlobalResourceBundleFromElement: unknown element type %d",
		elementPtr->type);
	break;

  }


}



/*
 * function to update the global resource bundle data structure
 *	and set those resource values in the resourcePalette widgets
 * N.B. - must keep this in sync with updateGlobalResourceBundleFromElement()
 */

void updateGlobalResourceBundleAndResourcePalette(Boolean objectDataOnly)
{
  DlElement *elementPtr, *dyn, *basic;
  char string[MAX_TOKEN_LENGTH];
  int i, tail;



/* simply return if not valid to update */
  if (currentDisplayInfo->numSelectedElements != 1 ||
	currentDisplayInfo->selectedElementsArray == NULL) return;

  elementPtr = currentDisplayInfo->selectedElementsArray[0];

/* if no resource palette yet, create it */
  if (resourceMW == NULL) {
     currentElementType = elementPtr->type;
     setResourcePaletteEntries();
     return;
  }

    

  if (ELEMENT_IS_RENDERABLE(elementPtr->type) ) {

/* get object data: must have object entry  - use rectangle type (arbitrary) */
    globalResourceBundle.x = elementPtr->structure.rectangle->object.x;
    sprintf(string,"%d",globalResourceBundle.x);
    XmTextFieldSetString(resourceEntryElement[X_RC],string);
    globalResourceBundle.y = elementPtr->structure.rectangle->object.y;
    sprintf(string,"%d",globalResourceBundle.y);
    XmTextFieldSetString(resourceEntryElement[Y_RC],string);
    globalResourceBundle.width = elementPtr->structure.rectangle->object.width;
    sprintf(string,"%d",globalResourceBundle.width);
    XmTextFieldSetString(resourceEntryElement[WIDTH_RC],string);
    globalResourceBundle.height= elementPtr->structure.rectangle->object.height;
    sprintf(string,"%d",globalResourceBundle.height);
    XmTextFieldSetString(resourceEntryElement[HEIGHT_RC],string);



/* if only updating OBJECT data (x,y,width,height) we're done */
    if (objectDataOnly) return;


/***************************************************************************/

/* only worry about basic and dynamic attributes for objects without widgets */

   if (! ELEMENT_HAS_WIDGET(elementPtr->type) 
			&& elementPtr->type != DL_TextUpdate
			&& elementPtr->type != DL_Composite) {

/* get basicAttribute data */
    basic = lookupBasicAttributeElement(elementPtr);

    if (basic != NULL) {
      globalResourceBundle.clr = basic->structure.basicAttribute->attr.clr;
      XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
      globalResourceBundle.style = basic->structure.basicAttribute->attr.style;
      optionMenuSet(resourceEntryElement[STYLE_RC],
		globalResourceBundle.style - FIRST_EDGE_STYLE);
      globalResourceBundle.fill = basic->structure.basicAttribute->attr.fill;
      optionMenuSet(resourceEntryElement[FILL_RC],
		globalResourceBundle.fill - FIRST_FILL_STYLE);
      globalResourceBundle.lineWidth =
		basic->structure.basicAttribute->attr.width;
      sprintf(string,"%d",globalResourceBundle.lineWidth);
      XmTextFieldSetString(resourceEntryElement[LINEWIDTH_RC],string);
    }

/* get dynamicAttribute data */
    dyn = lookupDynamicAttributeElement(elementPtr);

    if (dyn != NULL) {
      globalResourceBundle.clrmod =
			dyn->structure.dynamicAttribute->attr.mod.clr;
      optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
      globalResourceBundle.vis = dyn->structure.dynamicAttribute->attr.mod.vis;
      optionMenuSet(resourceEntryElement[VIS_RC],
		globalResourceBundle.vis - FIRST_VISIBILITY_MODE);
#ifdef __COLOR_RULE_H__
      globalResourceBundle.colorRule = dyn->structure.dynamicAttribute->attr.mod.colorRule;
      optionMenuSet(resourceEntryElement[COLOR_RULE_RC],
                globalResourceBundle.colorRule);
#endif
      strcpy(globalResourceBundle.chan,
		dyn->structure.dynamicAttribute->attr.param.chan);
      XmTextFieldSetString(resourceEntryElement[CHAN_RC],
		globalResourceBundle.chan);
    } else { 	/* reset to defaults for dynamics */
      globalResourceBundle.clrmod = STATIC;
      optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
      globalResourceBundle.vis = V_STATIC;
      optionMenuSet(resourceEntryElement[VIS_RC],
		globalResourceBundle.vis - FIRST_VISIBILITY_MODE);
#ifdef __COLOR_RULE_H__
      globalResourceBundle.colorRule = 0;
      optionMenuSet(resourceEntryElement[COLOR_RULE_RC],
                globalResourceBundle.colorRule);
#endif
      globalResourceBundle.chan[0] = '\0';
      XmTextFieldSetString(resourceEntryElement[CHAN_RC],
		globalResourceBundle.chan);
/* insensitize the other dyn fields since don't make sense if no channel
 * specified */
      XtSetSensitive(resourceEntryRC[CLRMOD_RC],False);
      XtSetSensitive(resourceEntryRC[VIS_RC],False);
#ifdef __COLOR_RULE_H__
      XtSetSensitive(resourceEntryRC[COLOR_RULE_RC],False);
#endif
    }
   }
  }

  switch (elementPtr->type) {
      case DL_File:
	strcpy(globalResourceBundle.name,elementPtr->structure.file->name);
	XmTextFieldSetString(resourceEntryElement[NAME_RC],
		globalResourceBundle.name);
	break;

      case DL_Display:
	globalResourceBundle.clr = elementPtr->structure.display->clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr = elementPtr->structure.display->bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
	strcpy(globalResourceBundle.cmap,elementPtr->structure.display->cmap);
	XmTextFieldSetString(resourceEntryElement[CMAP_RC],
		globalResourceBundle.cmap);
	break;

      case DL_Colormap:
	medmPrintf(
  "\nupdateGlobalResourceBundleAndResourcePalette:  in DL_Colormap type??");
	break;

      case DL_BasicAttribute:
      case DL_DynamicAttribute:
      /* (hopefully) taken care of in ELEMENT_IS_RENDERABLE part up there */
	break;

      case DL_Valuator:
	strcpy(globalResourceBundle.ctrl,
		elementPtr->structure.valuator->control.ctrl);
	XmTextFieldSetString(resourceEntryElement[CTRL_RC],
		globalResourceBundle.ctrl);
	globalResourceBundle.clr = elementPtr->structure.valuator->control.clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr =
		elementPtr->structure.valuator->control.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
	globalResourceBundle.label = elementPtr->structure.valuator->label;
        optionMenuSet(resourceEntryElement[LABEL_RC],
		globalResourceBundle.label - FIRST_LABEL_TYPE);
	globalResourceBundle.clrmod = elementPtr->structure.valuator->clrmod;
        optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
	globalResourceBundle.direction =
		elementPtr->structure.valuator->direction;
        optionMenuSet(resourceEntryElement[DIRECTION_RC],
		globalResourceBundle.direction - FIRST_DIRECTION);
	globalResourceBundle.dPrecision =
		elementPtr->structure.valuator->dPrecision;
	sprintf(string,"%f",globalResourceBundle.dPrecision);
	/* strip trailing zeroes */
        tail = strlen(string);
        while (string[--tail] == '0') string[tail] = '\0';
	XmTextFieldSetString(resourceEntryElement[PRECISION_RC],string);
	break;

      case DL_ChoiceButton:
	strcpy(globalResourceBundle.ctrl,
		elementPtr->structure.choiceButton->control.ctrl);
	XmTextFieldSetString(resourceEntryElement[CTRL_RC],
		globalResourceBundle.ctrl);
	globalResourceBundle.clr =
		elementPtr->structure.choiceButton->control.clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr =
		elementPtr->structure.choiceButton->control.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
	globalResourceBundle.clrmod =
		elementPtr->structure.choiceButton->clrmod;
        optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
	globalResourceBundle.stacking =
		elementPtr->structure.choiceButton->stacking;
        optionMenuSet(resourceEntryElement[STACKING_RC],
		globalResourceBundle.stacking - FIRST_STACKING);
	break;

      case DL_MessageButton:
	strcpy(globalResourceBundle.ctrl,
		elementPtr->structure.messageButton->control.ctrl);
	XmTextFieldSetString(resourceEntryElement[CTRL_RC],
		globalResourceBundle.ctrl);
	globalResourceBundle.clr =
		elementPtr->structure.messageButton->control.clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr =
		elementPtr->structure.messageButton->control.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
	strcpy(globalResourceBundle.messageLabel,
		elementPtr->structure.messageButton->label);
	XmTextFieldSetString(resourceEntryElement[MSG_LABEL_RC],
		globalResourceBundle.messageLabel);
	strcpy(globalResourceBundle.press_msg,
		elementPtr->structure.messageButton->press_msg);
	XmTextFieldSetString(resourceEntryElement[PRESS_MSG_RC],
		globalResourceBundle.press_msg);
	strcpy(globalResourceBundle.release_msg,
		elementPtr->structure.messageButton->release_msg);
	XmTextFieldSetString(resourceEntryElement[RELEASE_MSG_RC],
		globalResourceBundle.release_msg);
	globalResourceBundle.clrmod =
		elementPtr->structure.messageButton->clrmod;
        optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
	break;

      case DL_TextEntry:
	strcpy(globalResourceBundle.ctrl,
		elementPtr->structure.textEntry->control.ctrl);
	XmTextFieldSetString(resourceEntryElement[CTRL_RC],
		globalResourceBundle.ctrl);
	globalResourceBundle.clr =
		elementPtr->structure.textEntry->control.clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr =
		elementPtr->structure.textEntry->control.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
	globalResourceBundle.clrmod =
		elementPtr->structure.textEntry->clrmod;
        optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
	globalResourceBundle.format =
		elementPtr->structure.textEntry->format;
        optionMenuSet(resourceEntryElement[FORMAT_RC],
		globalResourceBundle.format - FIRST_TEXT_FORMAT);
	break;

      case DL_Menu:
	strcpy(globalResourceBundle.ctrl,
		elementPtr->structure.menu->control.ctrl);
	XmTextFieldSetString(resourceEntryElement[CTRL_RC],
		globalResourceBundle.ctrl);
	globalResourceBundle.clr =
		elementPtr->structure.menu->control.clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr =
		elementPtr->structure.menu->control.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
	globalResourceBundle.clrmod =
		elementPtr->structure.menu->clrmod;
        optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
	break;

      case DL_Meter:
	strcpy(globalResourceBundle.rdbk,
		elementPtr->structure.meter->monitor.rdbk);
	XmTextFieldSetString(resourceEntryElement[RDBK_RC],
		globalResourceBundle.rdbk);
	globalResourceBundle.clr =
		elementPtr->structure.meter->monitor.clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr =
		elementPtr->structure.meter->monitor.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
	globalResourceBundle.label =
		elementPtr->structure.meter->label;
        optionMenuSet(resourceEntryElement[LABEL_RC],
		globalResourceBundle.label - FIRST_LABEL_TYPE);
	globalResourceBundle.clrmod =
		elementPtr->structure.meter->clrmod;
        optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
	break;

      case DL_TextUpdate:
	strcpy(globalResourceBundle.rdbk,
		elementPtr->structure.textUpdate->monitor.rdbk);
	XmTextFieldSetString(resourceEntryElement[RDBK_RC],
		globalResourceBundle.rdbk);
	globalResourceBundle.clr =
		elementPtr->structure.textUpdate->monitor.clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr =
		elementPtr->structure.textUpdate->monitor.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
	globalResourceBundle.clrmod =
		elementPtr->structure.textUpdate->clrmod;
        optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
	globalResourceBundle.align =
		elementPtr->structure.textUpdate->align;
        optionMenuSet(resourceEntryElement[ALIGN_RC],
		globalResourceBundle.align - FIRST_TEXT_ALIGN);
	globalResourceBundle.format =
		elementPtr->structure.textUpdate->format;
        optionMenuSet(resourceEntryElement[FORMAT_RC],
		globalResourceBundle.format - FIRST_TEXT_FORMAT);
	break;

      case DL_Bar:
	strcpy(globalResourceBundle.rdbk,
		elementPtr->structure.bar->monitor.rdbk);
	XmTextFieldSetString(resourceEntryElement[RDBK_RC],
		globalResourceBundle.rdbk);
	globalResourceBundle.clr =
		elementPtr->structure.bar->monitor.clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr =
		elementPtr->structure.bar->monitor.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
	globalResourceBundle.label =
		elementPtr->structure.bar->label;
        optionMenuSet(resourceEntryElement[LABEL_RC],
		globalResourceBundle.label - FIRST_LABEL_TYPE);
	globalResourceBundle.clrmod =
		elementPtr->structure.bar->clrmod;
        optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
	globalResourceBundle.direction =
		elementPtr->structure.bar->direction;
        optionMenuSet(resourceEntryElement[DIRECTION_RC],
		globalResourceBundle.direction - FIRST_DIRECTION);
	globalResourceBundle.fillmod =
		elementPtr->structure.bar->fillmod;
        optionMenuSet(resourceEntryElement[FILLMOD_RC],
		globalResourceBundle.fillmod - FIRST_FILL_MODE);
	break;
      case DL_Byte:
        strcpy(globalResourceBundle.rdbk,
          elementPtr->structure.byte->monitor.rdbk);
        XmTextFieldSetString(resourceEntryElement[RDBK_RC],
          globalResourceBundle.rdbk);
        globalResourceBundle.clr = elementPtr->structure.byte->monitor.clr;
        XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
          currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
        globalResourceBundle.bclr = elementPtr->structure.byte->monitor.bclr;
        XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
          currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
        globalResourceBundle.clrmod = elementPtr->structure.byte->clrmod;
        optionMenuSet(resourceEntryElement[CLRMOD_RC],
          globalResourceBundle.clrmod - FIRST_COLOR_MODE);
        globalResourceBundle.direction = elementPtr->structure.byte->direction;
        optionMenuSet(resourceEntryElement[DIRECTION_RC],
          globalResourceBundle.direction - FIRST_DIRECTION);
        globalResourceBundle.sbit = elementPtr->structure.byte->sbit;
        sprintf(string,"%d",globalResourceBundle.sbit);
        XmTextFieldSetString(resourceEntryElement[SBIT_RC],string);
        globalResourceBundle.ebit = elementPtr->structure.byte->ebit;
        sprintf(string,"%d",globalResourceBundle.ebit);
        XmTextFieldSetString(resourceEntryElement[EBIT_RC],string);
        break;
      case DL_Indicator:
	strcpy(globalResourceBundle.rdbk,
		elementPtr->structure.indicator->monitor.rdbk);
	XmTextFieldSetString(resourceEntryElement[RDBK_RC],
		globalResourceBundle.rdbk);
	globalResourceBundle.clr =
		elementPtr->structure.indicator->monitor.clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr =
		elementPtr->structure.indicator->monitor.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
	globalResourceBundle.label =
		elementPtr->structure.indicator->label;
        optionMenuSet(resourceEntryElement[LABEL_RC],
		globalResourceBundle.label - FIRST_LABEL_TYPE);
	globalResourceBundle.clrmod =
		elementPtr->structure.indicator->clrmod;
        optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
	globalResourceBundle.direction =
		elementPtr->structure.indicator->direction;
        optionMenuSet(resourceEntryElement[DIRECTION_RC],
		globalResourceBundle.direction - FIRST_DIRECTION);
	break;

      case DL_StripChart:
	strcpy(globalResourceBundle.title,
		elementPtr->structure.stripChart->plotcom.title);
	XmTextFieldSetString(resourceEntryElement[TITLE_RC],
		globalResourceBundle.title);
	strcpy(globalResourceBundle.xlabel,
		elementPtr->structure.stripChart->plotcom.xlabel);
	XmTextFieldSetString(resourceEntryElement[XLABEL_RC],
		globalResourceBundle.xlabel);
	strcpy(globalResourceBundle.ylabel,
		elementPtr->structure.stripChart->plotcom.ylabel);
	XmTextFieldSetString(resourceEntryElement[YLABEL_RC],
		globalResourceBundle.ylabel);
	globalResourceBundle.clr =
		elementPtr->structure.stripChart->plotcom.clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr =
		elementPtr->structure.stripChart->plotcom.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
	globalResourceBundle.period = 
		elementPtr->structure.stripChart->period;
        cvtDoubleToString(globalResourceBundle.period,string,0);
	XmTextFieldSetString(resourceEntryElement[PERIOD_RC],string);
	globalResourceBundle.units =
		elementPtr->structure.stripChart->units;
        optionMenuSet(resourceEntryElement[UNITS_RC],
		globalResourceBundle.units - FIRST_TIME_UNIT);
	for (i = 0; i < MAX_PENS; i++){
	  strcpy(globalResourceBundle.scData[i].chan,
		elementPtr->structure.stripChart->pen[i].chan);  
	  globalResourceBundle.scData[i].clr = 
		elementPtr->structure.stripChart->pen[i].clr;
	}
	break;

      case DL_CartesianPlot:
	strcpy(globalResourceBundle.title,
		elementPtr->structure.cartesianPlot->plotcom.title);
	XmTextFieldSetString(resourceEntryElement[TITLE_RC],
		globalResourceBundle.title);
	strcpy(globalResourceBundle.xlabel,
		elementPtr->structure.cartesianPlot->plotcom.xlabel);
	XmTextFieldSetString(resourceEntryElement[XLABEL_RC],
		globalResourceBundle.xlabel);
	strcpy(globalResourceBundle.ylabel,
		elementPtr->structure.cartesianPlot->plotcom.ylabel);
	XmTextFieldSetString(resourceEntryElement[YLABEL_RC],
		globalResourceBundle.ylabel);
	globalResourceBundle.clr =
		elementPtr->structure.cartesianPlot->plotcom.clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr =
		elementPtr->structure.cartesianPlot->plotcom.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
	globalResourceBundle.count = 
		elementPtr->structure.cartesianPlot->count;
	sprintf(string,"%d",globalResourceBundle.count);
	XmTextFieldSetString(resourceEntryElement[COUNT_RC],string);
	globalResourceBundle.cStyle =
		elementPtr->structure.cartesianPlot->style;
        optionMenuSet(resourceEntryElement[CSTYLE_RC],
		globalResourceBundle.cStyle - FIRST_CARTESIAN_PLOT_STYLE);
	globalResourceBundle.erase_oldest =
		elementPtr->structure.cartesianPlot->erase_oldest;
        optionMenuSet(resourceEntryElement[ERASE_OLDEST_RC],
		globalResourceBundle.erase_oldest - FIRST_ERASE_OLDEST);
	for (i = 0; i < MAX_TRACES; i++){
	  strcpy(globalResourceBundle.cpData[i].xdata,
		elementPtr->structure.cartesianPlot->trace[i].xdata);  
	  strcpy(globalResourceBundle.cpData[i].ydata,
		elementPtr->structure.cartesianPlot->trace[i].ydata);  
	  globalResourceBundle.cpData[i].data_clr = 
		elementPtr->structure.cartesianPlot->trace[i].data_clr;
	}
	for (i = X_AXIS_ELEMENT; i <= Y2_AXIS_ELEMENT; i++) {
	  globalResourceBundle.axis[i].axisStyle =
		elementPtr->structure.cartesianPlot->axis[i].axisStyle;
	  globalResourceBundle.axis[i].rangeStyle =
		elementPtr->structure.cartesianPlot->axis[i].rangeStyle;
	  globalResourceBundle.axis[i].minRange = 
		elementPtr->structure.cartesianPlot->axis[i].minRange;
	  globalResourceBundle.axis[i].maxRange =
		elementPtr->structure.cartesianPlot->axis[i].maxRange;
	}
	strcpy(globalResourceBundle.trigger,
		elementPtr->structure.cartesianPlot->trigger);
	XmTextFieldSetString(resourceEntryElement[TRIGGER_RC],
		globalResourceBundle.trigger);
	strcpy(globalResourceBundle.erase,
		elementPtr->structure.cartesianPlot->erase);
	XmTextFieldSetString(resourceEntryElement[ERASE_RC],
		globalResourceBundle.erase);
	globalResourceBundle.eraseMode = 
		elementPtr->structure.cartesianPlot->eraseMode;
	optionMenuSet(resourceEntryElement[ERASE_MODE_RC],
		globalResourceBundle.eraseMode - FIRST_ERASE_MODE);
	break;

      case DL_SurfacePlot:
	strcpy(globalResourceBundle.title,
		elementPtr->structure.surfacePlot->plotcom.title);
	XmTextFieldSetString(resourceEntryElement[TITLE_RC],
		globalResourceBundle.title);
	strcpy(globalResourceBundle.xlabel,
		elementPtr->structure.surfacePlot->plotcom.xlabel);
	XmTextFieldSetString(resourceEntryElement[XLABEL_RC],
		globalResourceBundle.xlabel);
	strcpy(globalResourceBundle.ylabel,
		elementPtr->structure.surfacePlot->plotcom.ylabel);
	XmTextFieldSetString(resourceEntryElement[YLABEL_RC],
		globalResourceBundle.ylabel);
	globalResourceBundle.clr =
		elementPtr->structure.surfacePlot->plotcom.clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr =
		elementPtr->structure.surfacePlot->plotcom.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
	strcpy(globalResourceBundle.data,
		elementPtr->structure.surfacePlot->data);
	XmTextFieldSetString(resourceEntryElement[DATA_RC],
		globalResourceBundle.data);
	globalResourceBundle.data_clr = 
		elementPtr->structure.surfacePlot->data_clr;
	XtVaSetValues(resourceEntryElement[DATA_CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.data_clr],
		NULL);
	globalResourceBundle.dis = 
		elementPtr->structure.surfacePlot->dis;
	sprintf(string,"%d",globalResourceBundle.dis);
	XmTextFieldSetString(resourceEntryElement[DIS_RC],string);
	globalResourceBundle.xyangle = 
		elementPtr->structure.surfacePlot->xyangle;
	sprintf(string,"%d",globalResourceBundle.xyangle);
	XmTextFieldSetString(resourceEntryElement[XYANGLE_RC],string);
	globalResourceBundle.zangle = 
		elementPtr->structure.surfacePlot->zangle;
	sprintf(string,"%d",globalResourceBundle.zangle);
	XmTextFieldSetString(resourceEntryElement[ZANGLE_RC],string);
	break;

      case DL_Rectangle:
      case DL_Oval:
	/* handled in prelude (since only DlObject description) */
	break;

      case DL_Arc:
/* want user to see degrees, but internally use degrees*64 as Xlib requires */
	globalResourceBundle.begin = 
		elementPtr->structure.arc->begin;
	XmScaleSetValue(resourceEntryElement[BEGIN_RC],
			globalResourceBundle.begin/64);
	globalResourceBundle.path = 
		elementPtr->structure.arc ->path;
	XmScaleSetValue(resourceEntryElement[PATH_RC],
			globalResourceBundle.path/64);
	break;

      case DL_Text:
	strcpy(globalResourceBundle.textix,
		elementPtr->structure.text->textix);
	XmTextFieldSetString(resourceEntryElement[TEXTIX_RC],
		globalResourceBundle.textix);
	globalResourceBundle.align = elementPtr->structure.text->align;
        optionMenuSet(resourceEntryElement[ALIGN_RC],
		globalResourceBundle.align - FIRST_TEXT_ALIGN);
	break;

      case DL_RelatedDisplay:
	globalResourceBundle.clr =
		elementPtr->structure.relatedDisplay->clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr =
		elementPtr->structure.relatedDisplay->bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
	for (i = 0; i < MAX_RELATED_DISPLAYS; i++){
	  strcpy(globalResourceBundle.rdData[i].label,
		elementPtr->structure.relatedDisplay->display[i].label);  
	  strcpy(globalResourceBundle.rdData[i].name,
		elementPtr->structure.relatedDisplay->display[i].name);  
	  strcpy(globalResourceBundle.rdData[i].args,
		elementPtr->structure.relatedDisplay->display[i].args);  
	/* update the related display dialog (matrix of values) if appr. */
	  updateRelatedDisplayDataDialog();
	}
	break;

      case DL_ShellCommand:
	globalResourceBundle.clr =
		elementPtr->structure.shellCommand->clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr =
		elementPtr->structure.shellCommand->bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
	for (i = 0; i < MAX_SHELL_COMMANDS; i++){
	  strcpy(globalResourceBundle.cmdData[i].label,
		elementPtr->structure.shellCommand->command[i].label);  
	  strcpy(globalResourceBundle.cmdData[i].command,
		elementPtr->structure.shellCommand->command[i].command);  
	  strcpy(globalResourceBundle.cmdData[i].args,
		elementPtr->structure.shellCommand->command[i].args);  
	/* update the shell command dialog (matrix of values) if appr. */
	  updateShellCommandDataDialog();
	}
	break;


      case DL_Image:
	globalResourceBundle.imageType =
		elementPtr->structure.image->imageType;
        optionMenuSet(resourceEntryElement[IMAGETYPE_RC],
		globalResourceBundle.imageType - FIRST_IMAGE_TYPE);
	strcpy(globalResourceBundle.imageName,
		elementPtr->structure.image->imageName);
	XmTextFieldSetString(resourceEntryElement[IMAGENAME_RC],
		globalResourceBundle.imageName);
	break;

      case DL_Composite:
        globalResourceBundle.vis = elementPtr->structure.composite->vis;
        optionMenuSet(resourceEntryElement[VIS_RC],
		globalResourceBundle.vis - FIRST_VISIBILITY_MODE);
        strcpy(globalResourceBundle.chan,elementPtr->structure.composite->chan);
	XmTextFieldSetString(resourceEntryElement[CHAN_RC],
		globalResourceBundle.chan);
/* need to add this entry to widgetDM.h and finish this if we want named
 *  groups
	strcpy(globalResourceBundle.compositeName,
		 elementPtr->structure.composite->compositeName);
 */
	break;
      case DL_Line:
	break;
      case DL_Polyline:
	break;
      case DL_Polygon:
	break;
      case DL_BezierCurve:
	break;

      default:
	medmPrintf(    
"\n updateGlobalResourceBundleAndResourcePalette: unknown element type %d",
	    elementPtr->type);
	break;

  }


}



/*
 * function to update element from global resource bundle
 *
 *	note: simple elements (graphic elements) are not traversed
 *	but complex elements (monitors/controllers - objects which have
 *	associated widgets) are "traversed" or updated
 */
void updateElementFromGlobalResourceBundle(
  DlElement *elementPtr)
{
  Widget widget;
  DlElement *basic, *newBasic, *dyn;
  int i, j;
  int xOffset = 0, yOffset = 0, deltaWidth = 0, deltaHeight = 0;
  float sX, sY;
  Boolean moveWidgets;
  DlPolyline *dlPolyline;
  DlPolygon *dlPolygon;
  int minX, minY, maxX, maxY;

/* simply return if not valid to update */
  if (elementPtr == NULL || currentDisplayInfo == NULL) return;

/* (MDA)
 * can update elements in display list and retraverse whole thing,
 *	or update WIDGETS separately and do partial traversal ?
 */

  widget = (Widget) NULL;

/* this is common to all - therefore put here */
  if (ELEMENT_IS_RENDERABLE(elementPtr->type) ) {

  /* first lookup the widget based on current position/dimension */
    if (ELEMENT_HAS_WIDGET(elementPtr->type))
	widget = lookupElementWidget(currentDisplayInfo,
                        &(elementPtr->structure.rectangle->object));

  /* special handling if Composite  - move or resize */
    if (elementPtr->type == DL_Composite) {
      xOffset = globalResourceBundle.x 
			- elementPtr->structure.rectangle->object.x; 
      yOffset = globalResourceBundle.y 
			- elementPtr->structure.rectangle->object.y; 
      deltaWidth = globalResourceBundle.width -
		elementPtr->structure.rectangle->object.width;
      deltaHeight = globalResourceBundle.height -
		elementPtr->structure.rectangle->object.height;

      moveWidgets = True;
      if (xOffset != 0 || yOffset != 0)
	 moveCompositeChildren(currentDisplayInfo,elementPtr,xOffset,yOffset,
		moveWidgets);
      if (deltaWidth != 0 || deltaHeight != 0) {
         sX = (float) ((float)globalResourceBundle.width/
			(float)((int)globalResourceBundle.width - deltaWidth));
         sY = (float) ((float)globalResourceBundle.height/
			(float)((int)globalResourceBundle.height- deltaHeight));
	 resizeCompositeChildren(currentDisplayInfo,elementPtr,
			elementPtr,sX,sY);
      }
    } else if (elementPtr->type == DL_Polyline) {
      xOffset = globalResourceBundle.x 
			- elementPtr->structure.polyline->object.x; 
      yOffset = globalResourceBundle.y 
			- elementPtr->structure.polyline->object.y; 
      for (j = 0; j < elementPtr->structure.polyline->nPoints; j++) {
/* this works because really only changing one of x/y/w/h at a time */
	elementPtr->structure.polyline->points[j].x += xOffset;
	elementPtr->structure.polyline->points[j].y += yOffset;
	elementPtr->structure.polyline->points[j].x =
		elementPtr->structure.polyline->object.x +
		(elementPtr->structure.polyline->points[j].x -
		elementPtr->structure.polyline->object.x) *
/* avoid divide by 0 */
		(elementPtr->structure.polyline->object.width <= 0 ? 1 :
		   (short)(((float)globalResourceBundle.width/
		   (float)elementPtr->structure.polyline->object.width)));
	elementPtr->structure.polyline->points[j].y =
		elementPtr->structure.polyline->object.y +
		(elementPtr->structure.polyline->points[j].y -
		elementPtr->structure.polyline->object.y) *
/* avoid divide by 0 */
		(elementPtr->structure.polyline->object.height <= 0 ? 1 :
		   (short)(((float)globalResourceBundle.height/
		   (float)elementPtr->structure.polyline->object.height)));
      }
    } else if (elementPtr->type == DL_Polygon) {
      xOffset = globalResourceBundle.x 
			- elementPtr->structure.polygon->object.x; 
      yOffset = globalResourceBundle.y 
			- elementPtr->structure.polygon->object.y; 
      for (j = 0; j < elementPtr->structure.polygon->nPoints; j++) {
/* this works because really only changing one of x/y/w/h at a time */
	elementPtr->structure.polygon->points[j].x += xOffset;
	elementPtr->structure.polygon->points[j].y += yOffset;
	elementPtr->structure.polygon->points[j].x =
		elementPtr->structure.polygon->object.x +
		(elementPtr->structure.polygon->points[j].x -
		elementPtr->structure.polygon->object.x) *
/* avoid divide by 0 */
		(elementPtr->structure.polygon->object.width == 0 ? 1 :
		   (short)(((float)globalResourceBundle.width/
		   (float)elementPtr->structure.polygon->object.width)));
	elementPtr->structure.polygon->points[j].y =
		elementPtr->structure.polygon->object.y +
		(elementPtr->structure.polygon->points[j].y -
		elementPtr->structure.polygon->object.y) *
/* avoid divide by 0 */
		(elementPtr->structure.polygon->object.height == 0 ? 1 :
		   (short)(((float)globalResourceBundle.height/
		   (float)elementPtr->structure.polygon->object.height)));
      }
    }


  /* now update object entry in display list (if not DL_Display since
   *   letting resize callback handle that - but let X/Y be updated here) */
    if (elementPtr->type != DL_Display) {
      elementPtr->structure.rectangle->object.x = globalResourceBundle.x;
      elementPtr->structure.rectangle->object.y = globalResourceBundle.y;
      elementPtr->structure.rectangle->object.width=globalResourceBundle.width;
      elementPtr->structure.rectangle->object.height =
				globalResourceBundle.height;
    } else {
  /* for DL_Display update x/y here and let resize CB handle w/h */
      elementPtr->structure.rectangle->object.x = globalResourceBundle.x;
      elementPtr->structure.rectangle->object.y = globalResourceBundle.y;
    }

/*
 * (MDA) careful here - ASSUMPTION: elements that have widgets associated with
 *	them don't rely on basic or dynamic attributes
 */
   if ( ! ELEMENT_HAS_WIDGET(elementPtr->type) &&
		elementPtr->type != DL_TextUpdate &&
		elementPtr->type != DL_Composite) {
/*** 
 *** Basic Attribute
 ***/
     basic = lookupPrivateBasicAttributeElement(elementPtr);
     if (basic != NULL) {
   /* has a private basic attribute, just update it */
       basic->structure.basicAttribute->attr.clr = globalResourceBundle.clr;
       basic->structure.basicAttribute->attr.style = globalResourceBundle.style;
       basic->structure.basicAttribute->attr.fill = globalResourceBundle.fill;
       basic->structure.basicAttribute->attr.width =
		globalResourceBundle.lineWidth;
     } else {
   /* doesn't have a private basic attribute, copy and allocate one */
       basic = lookupBasicAttributeElement(elementPtr);
       if (basic != NULL) {
     /*
      * fairly complicated, since multiple elements can share a basic attribute:
      *  + structure copy to make clone of previous basic attribute
      *  + move copy of basic attribute to just after this element
      *  + now create new basic attr. for this element based on resourceBundle
      */
         newBasic =  createDlBasicAttribute(currentDisplayInfo);
         newBasic->structure.basicAttribute = basic->structure.basicAttribute;
         moveElementAfter(currentDisplayInfo,NULL,newBasic,elementPtr);
         newBasic =  createDlBasicAttribute(currentDisplayInfo);
         moveElementAfter(currentDisplayInfo,NULL,newBasic,elementPtr->prev);
       } else {
     /* didn't find a basic attribute, but need one */
	  basic = createDlBasicAttribute(currentDisplayInfo);
	  moveElementAfter(currentDisplayInfo,NULL,basic,elementPtr->prev);
       }
     }
/***
 *** Dynamic Attribute (NB: dynamic attribute must immediately precede the
 ***	object)
 ***/
     dyn = lookupDynamicAttributeElement(elementPtr);
     if (dyn != NULL) {
   /* found a dynamic attribute - update it */
       dyn->structure.dynamicAttribute->attr.mod.clr=
		globalResourceBundle.clrmod;
#ifdef __COLOR_RULE_H__
       dyn->structure.dynamicAttribute->attr.mod.colorRule = globalResourceBundle.colorRule;
#endif
       dyn->structure.dynamicAttribute->attr.mod.vis = globalResourceBundle.vis;
       strcpy(dyn->structure.dynamicAttribute->attr.param.chan,
		globalResourceBundle.chan);
     } else {
       if (strlen(globalResourceBundle.chan) > (size_t) 0) {
     /* didn't find a dynamic attribute, but need one since a channel
	has been specified for dynamics */
	dyn = createDlDynamicAttribute(currentDisplayInfo);
	moveElementAfter(currentDisplayInfo,NULL,dyn,elementPtr->prev);
       }
     }
   }
  }


  switch (elementPtr->type) {
      case DL_File:
	strcpy(elementPtr->structure.file->name,globalResourceBundle.name);
	break;

      case DL_Display:
	elementPtr->structure.display->clr = globalResourceBundle.clr;
	elementPtr->structure.display->bclr = globalResourceBundle.bclr;
	strcpy(elementPtr->structure.display->cmap,globalResourceBundle.cmap);
	currentDisplayInfo->drawingAreaBackgroundColor =
					globalResourceBundle.bclr;
	currentDisplayInfo->drawingAreaForegroundColor =
					globalResourceBundle.clr;
/* and resize the shell */
	XtVaSetValues(currentDisplayInfo->shell,
		XmNx,globalResourceBundle.x,
		XmNy,globalResourceBundle.y,
		XmNwidth,globalResourceBundle.width,
		XmNheight,globalResourceBundle.height,NULL);
	break;

      case DL_Valuator:
	strcpy(elementPtr->structure.valuator->control.ctrl,
		globalResourceBundle.ctrl);
	elementPtr->structure.valuator->control.clr = 
		globalResourceBundle.clr;
	elementPtr->structure.valuator->control.bclr = 
		globalResourceBundle.bclr;
	elementPtr->structure.valuator->label = globalResourceBundle.label;
	elementPtr->structure.valuator->clrmod = globalResourceBundle.clrmod;
	elementPtr->structure.valuator->direction = 
		globalResourceBundle.direction;
	elementPtr->structure.valuator->dPrecision = 
		globalResourceBundle.dPrecision;
	break;

      case DL_ChoiceButton:
	strcpy(elementPtr->structure.choiceButton->control.ctrl,
		globalResourceBundle.ctrl);
	elementPtr->structure.choiceButton->control.clr =
		globalResourceBundle.clr;
	elementPtr->structure.choiceButton->control.bclr = 
		globalResourceBundle.bclr;
	elementPtr->structure.choiceButton->clrmod = 
		globalResourceBundle.clrmod;
	elementPtr->structure.choiceButton->stacking =
		globalResourceBundle.stacking;
	break;

      case DL_MessageButton:
	strcpy(elementPtr->structure.messageButton->control.ctrl,
		globalResourceBundle.ctrl);
	elementPtr->structure.messageButton->control.clr =
		globalResourceBundle.clr;
	elementPtr->structure.messageButton->control.bclr =
		globalResourceBundle.bclr;
	strcpy(elementPtr->structure.messageButton->label,
		globalResourceBundle.messageLabel);
	strcpy(elementPtr->structure.messageButton->press_msg,
		globalResourceBundle.press_msg);
	strcpy(elementPtr->structure.messageButton->release_msg,
		globalResourceBundle.release_msg);
	elementPtr->structure.messageButton->clrmod = 
		globalResourceBundle.clrmod;
	break;

      case DL_TextEntry:
	strcpy(elementPtr->structure.textEntry->control.ctrl,
		globalResourceBundle.ctrl);
	elementPtr->structure.textEntry->control.clr =
		globalResourceBundle.clr;
	elementPtr->structure.textEntry->control.bclr =
		globalResourceBundle.bclr;
	elementPtr->structure.textEntry->clrmod =
		globalResourceBundle.clrmod;
	elementPtr->structure.textEntry->format =
		globalResourceBundle.format;
	break;

      case DL_Menu:
	strcpy(elementPtr->structure.menu->control.ctrl,
		globalResourceBundle.ctrl);
	elementPtr->structure.menu->control.clr =
		globalResourceBundle.clr;
	elementPtr->structure.menu->control.bclr =
		globalResourceBundle.bclr;
	elementPtr->structure.menu->clrmod =
		globalResourceBundle.clrmod;
	break;

      case DL_Meter:
	strcpy(elementPtr->structure.meter->monitor.rdbk,
		globalResourceBundle.rdbk);
	elementPtr->structure.meter->monitor.clr =
		globalResourceBundle.clr;
	elementPtr->structure.meter->monitor.bclr =
		globalResourceBundle.bclr;
	elementPtr->structure.meter->label =
		globalResourceBundle.label;
	elementPtr->structure.meter->clrmod =
		globalResourceBundle.clrmod;
	break;

      case DL_TextUpdate:
	strcpy(elementPtr->structure.textUpdate->monitor.rdbk,
		globalResourceBundle.rdbk);
	elementPtr->structure.textUpdate->monitor.clr =
		globalResourceBundle.clr;
	elementPtr->structure.textUpdate->monitor.bclr =
		globalResourceBundle.bclr;
	elementPtr->structure.textUpdate->clrmod =
		globalResourceBundle.clrmod;
	elementPtr->structure.textUpdate->align =
		globalResourceBundle.align;
	elementPtr->structure.textUpdate->format =
		globalResourceBundle.format;
	break;

      case DL_Bar:
	strcpy(elementPtr->structure.bar->monitor.rdbk,
		globalResourceBundle.rdbk);
	elementPtr->structure.bar->monitor.clr =
		globalResourceBundle.clr;
	elementPtr->structure.bar->monitor.bclr =
		globalResourceBundle.bclr;
	elementPtr->structure.bar->label =
		globalResourceBundle.label;
	elementPtr->structure.bar->clrmod =
		globalResourceBundle.clrmod;
	elementPtr->structure.bar->direction =
		globalResourceBundle.direction;
	elementPtr->structure.bar->fillmod =
		globalResourceBundle.fillmod;
	break;
      case DL_Byte:
        strcpy(elementPtr->structure.byte->monitor.rdbk,
                globalResourceBundle.rdbk);
        elementPtr->structure.byte->monitor.clr = globalResourceBundle.clr;
        elementPtr->structure.byte->monitor.bclr = globalResourceBundle.bclr;
        elementPtr->structure.byte->clrmod = globalResourceBundle.clrmod;
        elementPtr->structure.byte->direction = globalResourceBundle.direction;
        elementPtr->structure.byte->sbit = globalResourceBundle.sbit;
        elementPtr->structure.byte->ebit = globalResourceBundle.ebit;
        break;
      case DL_Indicator:
	strcpy(elementPtr->structure.indicator->monitor.rdbk,
		globalResourceBundle.rdbk);
	elementPtr->structure.indicator->monitor.clr =
		globalResourceBundle.clr;
	elementPtr->structure.indicator->monitor.bclr =
		globalResourceBundle.bclr;
	elementPtr->structure.indicator->label =
		globalResourceBundle.label;
	elementPtr->structure.indicator->clrmod =
		globalResourceBundle.clrmod;
	elementPtr->structure.indicator->direction =
		globalResourceBundle.direction;
	break;

      case DL_StripChart:
	strcpy(elementPtr->structure.stripChart->plotcom.title,
		globalResourceBundle.title);
	strcpy(elementPtr->structure.stripChart->plotcom.xlabel,
		globalResourceBundle.xlabel);
	strcpy(elementPtr->structure.stripChart->plotcom.ylabel,
		globalResourceBundle.ylabel);
	elementPtr->structure.stripChart->plotcom.clr =
		globalResourceBundle.clr;
	elementPtr->structure.stripChart->plotcom.bclr =
		globalResourceBundle.bclr;
	elementPtr->structure.stripChart->period =
		globalResourceBundle.period;
	elementPtr->structure.stripChart->units =
		globalResourceBundle.units;
	for (i = 0; i < MAX_PENS; i++){
	  strcpy(elementPtr->structure.stripChart->pen[i].chan,
		globalResourceBundle.scData[i].chan);
	  elementPtr->structure.stripChart->pen[i].clr =
		globalResourceBundle.scData[i].clr;
	}
	break;


      case DL_CartesianPlot:
	strcpy(elementPtr->structure.cartesianPlot->plotcom.title,
		globalResourceBundle.title);
	strcpy(elementPtr->structure.cartesianPlot->plotcom.xlabel,
		globalResourceBundle.xlabel);
	strcpy(elementPtr->structure.cartesianPlot->plotcom.ylabel,
		globalResourceBundle.ylabel);
	elementPtr->structure.cartesianPlot->plotcom.clr =
		globalResourceBundle.clr;
	elementPtr->structure.cartesianPlot->plotcom.bclr =
		globalResourceBundle.bclr;
	elementPtr->structure.cartesianPlot->count =
		globalResourceBundle.count;
	elementPtr->structure.cartesianPlot->style =
		globalResourceBundle.cStyle;
	elementPtr->structure.cartesianPlot->erase_oldest =
		globalResourceBundle.erase_oldest;
	for (i = 0; i < MAX_TRACES; i++){
	  strcpy(elementPtr->structure.cartesianPlot->trace[i].xdata,
		globalResourceBundle.cpData[i].xdata);
	  strcpy(elementPtr->structure.cartesianPlot->trace[i].ydata,
		globalResourceBundle.cpData[i].ydata);
	  elementPtr->structure.cartesianPlot->trace[i].data_clr =
		globalResourceBundle.cpData[i].data_clr;
	}
	elementPtr->structure.cartesianPlot->axis[X_AXIS_ELEMENT].axisStyle =
		globalResourceBundle.axis[X_AXIS_ELEMENT].axisStyle;
	elementPtr->structure.cartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle =
		globalResourceBundle.axis[X_AXIS_ELEMENT].rangeStyle;
	elementPtr->structure.cartesianPlot->axis[X_AXIS_ELEMENT].minRange =
		globalResourceBundle.axis[X_AXIS_ELEMENT].minRange;
	elementPtr->structure.cartesianPlot->axis[X_AXIS_ELEMENT].maxRange =
		globalResourceBundle.axis[X_AXIS_ELEMENT].maxRange;
	elementPtr->structure.cartesianPlot->axis[Y1_AXIS_ELEMENT].axisStyle =
		globalResourceBundle.axis[Y1_AXIS_ELEMENT].axisStyle;
	elementPtr->structure.cartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle =
		globalResourceBundle.axis[Y1_AXIS_ELEMENT].rangeStyle;
	elementPtr->structure.cartesianPlot->axis[Y1_AXIS_ELEMENT].minRange =
		globalResourceBundle.axis[Y1_AXIS_ELEMENT].minRange;
	elementPtr->structure.cartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange =
		globalResourceBundle.axis[Y1_AXIS_ELEMENT].maxRange;
	elementPtr->structure.cartesianPlot->axis[Y2_AXIS_ELEMENT].axisStyle =
		globalResourceBundle.axis[Y2_AXIS_ELEMENT].axisStyle;
	elementPtr->structure.cartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle =
		globalResourceBundle.axis[Y2_AXIS_ELEMENT].rangeStyle;
	elementPtr->structure.cartesianPlot->axis[Y2_AXIS_ELEMENT].minRange =
		globalResourceBundle.axis[Y2_AXIS_ELEMENT].minRange;
	elementPtr->structure.cartesianPlot->axis[Y2_AXIS_ELEMENT].maxRange =
		globalResourceBundle.axis[Y2_AXIS_ELEMENT].maxRange;
	strcpy(elementPtr->structure.cartesianPlot->trigger,
		globalResourceBundle.trigger);
	strcpy(elementPtr->structure.cartesianPlot->erase,
		globalResourceBundle.erase);
	elementPtr->structure.cartesianPlot->eraseMode =
		globalResourceBundle.eraseMode;
	break;

      case DL_SurfacePlot:
	strcpy(elementPtr->structure.surfacePlot->plotcom.title,
		globalResourceBundle.title);
	strcpy(elementPtr->structure.surfacePlot->plotcom.xlabel,
		globalResourceBundle.xlabel);
	strcpy(elementPtr->structure.surfacePlot->plotcom.ylabel,
		globalResourceBundle.ylabel);
	elementPtr->structure.surfacePlot->plotcom.clr =
		globalResourceBundle.clr;
	elementPtr->structure.surfacePlot->plotcom.bclr =
		globalResourceBundle.bclr;
	strcpy(elementPtr->structure.surfacePlot->data,
		globalResourceBundle.data);
	elementPtr->structure.surfacePlot->data_clr =
		globalResourceBundle.data_clr;
	elementPtr->structure.surfacePlot->dis =
		globalResourceBundle.dis;
	elementPtr->structure.surfacePlot->xyangle =
		globalResourceBundle.xyangle;
	elementPtr->structure.surfacePlot->zangle =
		globalResourceBundle.zangle;
	break;


      case DL_Rectangle:
      case DL_Oval:
	/* handled in prelude (since only DlObject description) */
	break;

      case DL_Arc:
	elementPtr->structure.arc->begin =
		globalResourceBundle.begin;
	elementPtr->structure.arc ->path =
		globalResourceBundle.path;
	break;

      case DL_Text:
	strcpy(elementPtr->structure.text->textix,globalResourceBundle.textix);
	elementPtr->structure.text->align = globalResourceBundle.align;
	break;

      case DL_RelatedDisplay:
	elementPtr->structure.relatedDisplay->clr =
		globalResourceBundle.clr;
	elementPtr->structure.relatedDisplay->bclr =
		globalResourceBundle.bclr;
	for (i = 0; i < MAX_RELATED_DISPLAYS; i++){
	  strcpy(elementPtr->structure.relatedDisplay->display[i].label,
		globalResourceBundle.rdData[i].label);
	  strcpy(elementPtr->structure.relatedDisplay->display[i].name,
		globalResourceBundle.rdData[i].name);
	  strcpy(elementPtr->structure.relatedDisplay->display[i].args,
		globalResourceBundle.rdData[i].args);
	}
	break;

      case DL_ShellCommand:
	elementPtr->structure.shellCommand->clr =
		globalResourceBundle.clr;
	elementPtr->structure.shellCommand->bclr =
		globalResourceBundle.bclr;
	for (i = 0; i < MAX_SHELL_COMMANDS; i++){
	  strcpy(elementPtr->structure.shellCommand->command[i].label,
		globalResourceBundle.cmdData[i].label);
	  strcpy(elementPtr->structure.shellCommand->command[i].command,
		globalResourceBundle.cmdData[i].command);
	  strcpy(elementPtr->structure.shellCommand->command[i].args,
		globalResourceBundle.cmdData[i].args);
	}
	break;

      case DL_Image:
	elementPtr->structure.image->imageType =
		globalResourceBundle.imageType;
	strcpy(elementPtr->structure.image->imageName,
		globalResourceBundle.imageName);
	break;

      case DL_Composite:
	elementPtr->structure.composite->vis = globalResourceBundle.vis;
	strcpy(elementPtr->structure.composite->chan,
		globalResourceBundle.chan);
	break;

      case DL_Line: /* maps to Polyline (new) or Rising/Falling Line (old)*/
	break;

/* handle change in lineWidth if Polyline, or Polygon w/fill==F_OUTLINE */
      case DL_Polyline:
	minX = INT_MAX; minY = INT_MAX; maxX = INT_MIN; maxY = INT_MIN;
	dlPolyline = elementPtr->structure.polyline;
	for (j = 0; j < elementPtr->structure.polyline->nPoints; j++) {
	  minX = MIN(minX,dlPolyline->points[j].x);
	  maxX = MAX(maxX,dlPolyline->points[j].x);
	  minY = MIN(minY,dlPolyline->points[j].y);
	  maxY = MAX(maxY,dlPolyline->points[j].y);
	}
	dlPolyline->object.x =  minX - globalResourceBundle.lineWidth/2;
	dlPolyline->object.y =  minY - globalResourceBundle.lineWidth/2;
	dlPolyline->object.width =  maxX - minX
					+ globalResourceBundle.lineWidth;
	dlPolyline->object.height =  maxY - minY
					+ globalResourceBundle.lineWidth;
	break;
      case DL_Polygon:
	minX = INT_MAX; minY = INT_MAX; maxX = INT_MIN; maxY = INT_MIN;
	dlPolygon = elementPtr->structure.polygon;
	for (j = 0; j < elementPtr->structure.polygon->nPoints; j++) {
	  minX = MIN(minX,dlPolygon->points[j].x);
	  maxX = MAX(maxX,dlPolygon->points[j].x);
	  minY = MIN(minY,dlPolygon->points[j].y);
	  maxY = MAX(maxY,dlPolygon->points[j].y);
	}
	if (globalResourceBundle.fill == F_SOLID) {
	  dlPolygon->object.x =  minX;
	  dlPolygon->object.y =  minY;
	  dlPolygon->object.width =  maxX - minX;
	  dlPolygon->object.height =  maxY - minY;
	} else {	/* F_OUTLINE, therfore lineWidth is a factor */
	  dlPolygon->object.x =  minX - globalResourceBundle.lineWidth/2;
	  dlPolygon->object.y =  minY - globalResourceBundle.lineWidth/2;
	  dlPolygon->object.width =  maxX - minX
					+ globalResourceBundle.lineWidth;
	  dlPolygon->object.height =  maxY - minY
					+ globalResourceBundle.lineWidth;
	}
	break;


      case DL_BezierCurve:
	break;

      default:
	medmPrintf(
	   "\n updateElementFromResourceBundle: unknown element type %d",
	    elementPtr->type);
	break;

  }

/* now update the element's widget by recreating the widget... */
    if (ELEMENT_HAS_WIDGET(elementPtr->type)) {
        if (widget != NULL) {
	   if (widget != currentDisplayInfo->drawingArea) {
		destroyElementWidget(currentDisplayInfo,widget);
		(*elementPtr->dmExecute)((XtPointer) currentDisplayInfo,
				(XtPointer) elementPtr->structure.file,FALSE);
	   }
        }
    }



}




/*
 * function to clear/reset the global resource bundle data structure
 *	and to put those resource values into the resourcePalette
 *	elements (for the specified element type)
 */

void resetGlobalResourceBundleAndResourcePalette()
{
  char string[MAX_TOKEN_LENGTH];


  if (ELEMENT_IS_RENDERABLE(currentElementType) ) {

/* get object data: must have object entry  - use rectangle type (arbitrary) */
   globalResourceBundle.x = 0;
   globalResourceBundle.y = 0;

/*
 * special case for text -
 *   since can type to input, want to inherit width/height
 */
   if (currentElementType != DL_Text) {
     globalResourceBundle.width = 10;
     globalResourceBundle.height = 10;
   }

   sprintf(string,"%d",globalResourceBundle.x);
   XmTextFieldSetString(resourceEntryElement[X_RC],string);
   sprintf(string,"%d",globalResourceBundle.y);
   XmTextFieldSetString(resourceEntryElement[Y_RC],string);
   sprintf(string,"%d",globalResourceBundle.width);
   XmTextFieldSetString(resourceEntryElement[WIDTH_RC],string);
   sprintf(string,"%d",globalResourceBundle.height);
   XmTextFieldSetString(resourceEntryElement[HEIGHT_RC],string);

  }

}
