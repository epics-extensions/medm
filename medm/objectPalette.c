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

void updateGlobalResourceBundleObjectAttribute(DlObject *object) {
  globalResourceBundle.x = object->x;
  globalResourceBundle.y = object->y;
  globalResourceBundle.width = object->width;
  globalResourceBundle.height= object->height;
}

void updateElementObjectAttribute(DlObject *object) {
  object->x = globalResourceBundle.x;
  object->y = globalResourceBundle.y;
  object->width = globalResourceBundle.width;
  object->height = globalResourceBundle.height;
}

void updateResourcePaletteObjectAttribute() {
  char string[MAX_TOKEN_LENGTH];
  sprintf(string,"%d",globalResourceBundle.x);
  XmTextFieldSetString(resourceEntryElement[X_RC],string);
  sprintf(string,"%d",globalResourceBundle.y);
  XmTextFieldSetString(resourceEntryElement[Y_RC],string);
  sprintf(string,"%d",globalResourceBundle.width);
  XmTextFieldSetString(resourceEntryElement[WIDTH_RC],string);
  sprintf(string,"%d",globalResourceBundle.height);
  XmTextFieldSetString(resourceEntryElement[HEIGHT_RC],string);
}

void updateGlobalResourceBundleBasicAttribute(DlBasicAttribute *attr) {
  globalResourceBundle.clr = attr->clr;
  globalResourceBundle.style = attr->style;
  globalResourceBundle.fill = attr->fill;
  globalResourceBundle.lineWidth = attr->width;
}

void updateElementBasicAttribute(DlBasicAttribute *attr) {
  attr->clr = globalResourceBundle.clr;
  attr->style = globalResourceBundle.style;
  attr->fill = globalResourceBundle.fill;
  attr->width = globalResourceBundle.lineWidth;
}

void updateResourcePaletteBasicAttribute() {
  char string[MAX_TOKEN_LENGTH];
  XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
    currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
  optionMenuSet(resourceEntryElement[STYLE_RC],
    globalResourceBundle.style - FIRST_EDGE_STYLE);
  optionMenuSet(resourceEntryElement[FILL_RC],
    globalResourceBundle.fill - FIRST_FILL_STYLE);
  sprintf(string,"%d",globalResourceBundle.lineWidth);
  XmTextFieldSetString(resourceEntryElement[LINEWIDTH_RC],string);
}

void updateGlobalResourceBundleDynamicAttribute(DlDynamicAttribute *dynAttr) {
  globalResourceBundle.clrmod = dynAttr->clr;
  globalResourceBundle.vis = dynAttr->vis;
#ifdef __COLOR_RULE_H__
  globalResourceBundle.colorRule = dynAttr->colorRule;
#endif
  strcpy(globalResourceBundle.chan,dynAttr->chan);
}

void updateElementDynamicAttribute(DlDynamicAttribute *dynAttr) {
  dynAttr->clr = globalResourceBundle.clrmod;
  dynAttr->vis = globalResourceBundle.vis;
#ifdef __COLOR_RULE_H__
  dynAttr->colorRule = globalResourceBundle.colorRule;
#endif
  strcpy(dynAttr->chan,globalResourceBundle.chan);
}

void updateResourcePaletteDynamicAttribute() {
  optionMenuSet(resourceEntryElement[CLRMOD_RC],
    globalResourceBundle.clrmod - FIRST_COLOR_MODE);
  optionMenuSet(resourceEntryElement[VIS_RC],
    globalResourceBundle.vis - FIRST_VISIBILITY_MODE);
#ifdef __COLOR_RULE_H__
  optionMenuSet(resourceEntryElement[COLOR_RULE_RC],
    globalResourceBundle.colorRule);
#endif
  XmTextFieldSetString(resourceEntryElement[CHAN_RC],
    globalResourceBundle.chan);
  if (globalResourceBundle.chan[0] != '\0') {
    XtSetSensitive(resourceEntryRC[CLRMOD_RC],True);
    XtSetSensitive(resourceEntryRC[VIS_RC],True);
#ifdef __COLOR_RULE_H__
    XtSetSensitive(resourceEntryRC[COLOR_RULE_RC],True);
#endif
  } else {
    XtSetSensitive(resourceEntryRC[CLRMOD_RC],False);
    XtSetSensitive(resourceEntryRC[VIS_RC],False);
#ifdef __COLOR_RULE_H__
    XtSetSensitive(resourceEntryRC[COLOR_RULE_RC],False);
#endif
  }
}

void updateGlobalResourceBundleControlAttribute(DlControl *control) {
  strcpy(globalResourceBundle.chan, control->ctrl);
  globalResourceBundle.clr = control->clr;
  globalResourceBundle.bclr = control->bclr;
}

void updateElementControlAttribute(DlControl *control) {
  strcpy(control->ctrl, globalResourceBundle.chan);
  control->clr = globalResourceBundle.clr;
  control->bclr = globalResourceBundle.bclr;
}

void updateResourcePaletteControlAttribute() {
  XmTextFieldSetString(resourceEntryElement[CTRL_RC],globalResourceBundle.chan);
  XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
  XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
}

void updateGlobalResourceBundleMonitorAttribute(DlMonitor *monitor) {
  strcpy(globalResourceBundle.chan, monitor->rdbk);
  globalResourceBundle.clr = monitor->clr;
  globalResourceBundle.bclr = monitor->bclr;
}

void updateElementMonitorAttribute(DlMonitor *monitor) {
  strcpy(monitor->rdbk, globalResourceBundle.chan);
  monitor->clr = globalResourceBundle.clr;
  monitor->bclr = globalResourceBundle.bclr;
}

void updateResourcePaletteMonitorAttribute() {
  XmTextFieldSetString(resourceEntryElement[RDBK_RC],globalResourceBundle.chan);
  XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
	currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
  XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
	currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
}
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
  if (!resourceMW) createResource();

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

void updateGlobalResourceBundleFromElement(DlElement *element) {
  DlCartesianPlot *p;
  int i;
  if (!element || (element->type != DL_CartesianPlot)) return;
  p = element->structure.cartesianPlot;

  for (i = X_AXIS_ELEMENT; i <= Y2_AXIS_ELEMENT; i++) {
    globalResourceBundle.axis[i].axisStyle = p->axis[i].axisStyle;
    globalResourceBundle.axis[i].rangeStyle = p->axis[i].rangeStyle;
    globalResourceBundle.axis[i].minRange = p->axis[i].minRange;
    globalResourceBundle.axis[i].maxRange = p->axis[i].maxRange;
  }
}

void updateGlobalResourceBundleAndResourcePalette(Boolean objectDataOnly) {
  DlElement *elementPtr;
  char string[MAX_TOKEN_LENGTH];
  int i, tail;

/* simply return if not valid to update */
  if (currentDisplayInfo->numSelectedElements != 1 ||
	currentDisplayInfo->selectedElementsArray == NULL) return;

  elementPtr = currentDisplayInfo->selectedElementsArray[0];

/* if no resource palette yet, create it */
  if (!resourceMW) {
     currentElementType = elementPtr->type;
     setResourcePaletteEntries();
     return;
  }

  switch (elementPtr->type) {
    case DL_File:
      strcpy(globalResourceBundle.name,elementPtr->structure.file->name);
      XmTextFieldSetString(resourceEntryElement[NAME_RC],
	globalResourceBundle.name);
      break;

    case DL_Colormap:
      medmPrintf(
      "\nupdateGlobalResourceBundleAndResourcePalette:  in DL_Colormap type??");
      break;

    case DL_Display:
      updateGlobalResourceBundleObjectAttribute(
                &(elementPtr->structure.display->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;
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

    case DL_Valuator: {
      DlValuator *p = elementPtr->structure.valuator;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleControlAttribute(&(p->control));
      updateResourcePaletteControlAttribute();
      globalResourceBundle.label = p->label;
      optionMenuSet(resourceEntryElement[LABEL_RC],
		globalResourceBundle.label - FIRST_LABEL_TYPE);
      globalResourceBundle.clrmod = p->clrmod;
      optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
      globalResourceBundle.direction = p->direction;
      optionMenuSet(resourceEntryElement[DIRECTION_RC],
		globalResourceBundle.direction - FIRST_DIRECTION);
      globalResourceBundle.dPrecision = p->dPrecision;
      sprintf(string,"%f",globalResourceBundle.dPrecision);
      /* strip trailing zeroes */
      tail = strlen(string);
      while (string[--tail] == '0') string[tail] = '\0';
      XmTextFieldSetString(resourceEntryElement[PRECISION_RC],string);
      break;
    }
    case DL_ChoiceButton: {
      DlChoiceButton *p = elementPtr->structure.choiceButton;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleControlAttribute(&(p->control));
      updateResourcePaletteControlAttribute();
      globalResourceBundle.clrmod = p->clrmod;
      optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
      globalResourceBundle.stacking = p->stacking;
      optionMenuSet(resourceEntryElement[STACKING_RC],
		globalResourceBundle.stacking - FIRST_STACKING);
      break;
    }
    case DL_MessageButton: {
      DlMessageButton *p = elementPtr->structure.messageButton;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleControlAttribute(&(p->control));
      updateResourcePaletteControlAttribute();
      strcpy(globalResourceBundle.messageLabel, p->label);
      XmTextFieldSetString(resourceEntryElement[MSG_LABEL_RC],
		globalResourceBundle.messageLabel);
      strcpy(globalResourceBundle.press_msg, p->press_msg);
      XmTextFieldSetString(resourceEntryElement[PRESS_MSG_RC],
		globalResourceBundle.press_msg);
      strcpy(globalResourceBundle.release_msg, p->release_msg);
      XmTextFieldSetString(resourceEntryElement[RELEASE_MSG_RC],
		globalResourceBundle.release_msg);
      globalResourceBundle.clrmod = p->clrmod;
      optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
      break;
    }
    case DL_TextEntry: {
      DlTextEntry *p = elementPtr->structure.textEntry;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleControlAttribute(&(p->control));
      updateResourcePaletteControlAttribute();
      globalResourceBundle.clrmod = p->clrmod;
      optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
      globalResourceBundle.format = p->format;
      optionMenuSet(resourceEntryElement[FORMAT_RC],
		globalResourceBundle.format - FIRST_TEXT_FORMAT);
      break;
    }
    case DL_Menu: {
      DlMenu *p = elementPtr->structure.menu;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleControlAttribute(&(p->control));
      updateResourcePaletteControlAttribute();

      globalResourceBundle.clrmod = p->clrmod;
      optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
      break;
    }
    case DL_Meter: {
      DlMeter *p = elementPtr->structure.meter;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleMonitorAttribute(&(p->monitor));
      updateResourcePaletteMonitorAttribute();

      globalResourceBundle.label = p->label;
      optionMenuSet(resourceEntryElement[LABEL_RC],
	globalResourceBundle.label - FIRST_LABEL_TYPE);
      globalResourceBundle.clrmod = p->clrmod;
      optionMenuSet(resourceEntryElement[CLRMOD_RC],
	globalResourceBundle.clrmod - FIRST_COLOR_MODE);
      break;
    }
    case DL_TextUpdate: {
      DlTextUpdate *p = elementPtr->structure.textUpdate;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleMonitorAttribute(&(p->monitor));
      updateResourcePaletteMonitorAttribute();

      globalResourceBundle.clrmod = p->clrmod;
      optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
      globalResourceBundle.align = p->align;
      optionMenuSet(resourceEntryElement[ALIGN_RC],
		globalResourceBundle.align - FIRST_TEXT_ALIGN);
      globalResourceBundle.format = p->format;
      optionMenuSet(resourceEntryElement[FORMAT_RC],
		globalResourceBundle.format - FIRST_TEXT_FORMAT);
      break;
    }
    case DL_Bar: {
      DlBar *p = elementPtr->structure.bar;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleMonitorAttribute(&(p->monitor));
      updateResourcePaletteMonitorAttribute();

      globalResourceBundle.label = p->label;
      optionMenuSet(resourceEntryElement[LABEL_RC],
		globalResourceBundle.label - FIRST_LABEL_TYPE);
      globalResourceBundle.clrmod = p->clrmod;
      optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
      globalResourceBundle.direction = p->direction;
      optionMenuSet(resourceEntryElement[DIRECTION_RC],
		globalResourceBundle.direction - FIRST_DIRECTION);
      globalResourceBundle.fillmod = p->fillmod;
      optionMenuSet(resourceEntryElement[FILLMOD_RC],
		globalResourceBundle.fillmod - FIRST_FILL_MODE);
      break;
    }
    case DL_Byte: {
      DlByte *p = elementPtr->structure.byte;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleMonitorAttribute(&(p->monitor));
      updateResourcePaletteMonitorAttribute();

      globalResourceBundle.clrmod = p->clrmod;
      optionMenuSet(resourceEntryElement[CLRMOD_RC],
          globalResourceBundle.clrmod - FIRST_COLOR_MODE);
      globalResourceBundle.direction = p->direction;
      optionMenuSet(resourceEntryElement[DIRECTION_RC],
          globalResourceBundle.direction - FIRST_DIRECTION);
      globalResourceBundle.sbit = p->sbit;
      sprintf(string,"%d",globalResourceBundle.sbit);
      XmTextFieldSetString(resourceEntryElement[SBIT_RC],string);
      globalResourceBundle.ebit = p->ebit;
      sprintf(string,"%d",globalResourceBundle.ebit);
      XmTextFieldSetString(resourceEntryElement[EBIT_RC],string);
      break;
    }
    case DL_Indicator: {
      DlIndicator *p = elementPtr->structure.indicator;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleMonitorAttribute(&(p->monitor));
      updateResourcePaletteMonitorAttribute();

      globalResourceBundle.label = p->label;
      optionMenuSet(resourceEntryElement[LABEL_RC],
		globalResourceBundle.label - FIRST_LABEL_TYPE);
      globalResourceBundle.clrmod = p->clrmod;
      optionMenuSet(resourceEntryElement[CLRMOD_RC],
		globalResourceBundle.clrmod - FIRST_COLOR_MODE);
      globalResourceBundle.direction = p->direction;
      optionMenuSet(resourceEntryElement[DIRECTION_RC],
		globalResourceBundle.direction - FIRST_DIRECTION);
      break;
    }
    case DL_StripChart: {
      DlStripChart *p = elementPtr->structure.stripChart;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      strcpy(globalResourceBundle.title, p->plotcom.title);
      XmTextFieldSetString(resourceEntryElement[TITLE_RC],
		globalResourceBundle.title);
      strcpy(globalResourceBundle.xlabel, p->plotcom.xlabel);
      XmTextFieldSetString(resourceEntryElement[XLABEL_RC],
		globalResourceBundle.xlabel);
      strcpy(globalResourceBundle.ylabel, p->plotcom.ylabel);
      XmTextFieldSetString(resourceEntryElement[YLABEL_RC],
		globalResourceBundle.ylabel);
      globalResourceBundle.clr = p->plotcom.clr;
      XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
      globalResourceBundle.bclr = p->plotcom.bclr;
      XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
      globalResourceBundle.period = p->period;
      cvtDoubleToString(globalResourceBundle.period,string,0);
      XmTextFieldSetString(resourceEntryElement[PERIOD_RC],string);
      globalResourceBundle.units = p->units;
      optionMenuSet(resourceEntryElement[UNITS_RC],
		globalResourceBundle.units - FIRST_TIME_UNIT);
      for (i = 0; i < MAX_PENS; i++){
        strcpy(globalResourceBundle.scData[i].chan,p->pen[i].chan);  
	globalResourceBundle.scData[i].clr = p->pen[i].clr;
      }
      break;
    }
    case DL_CartesianPlot: {
      DlCartesianPlot *p = elementPtr->structure.cartesianPlot;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      strcpy(globalResourceBundle.title, p->plotcom.title);
      XmTextFieldSetString(resourceEntryElement[TITLE_RC],
		globalResourceBundle.title);
      strcpy(globalResourceBundle.xlabel, p->plotcom.xlabel);
      XmTextFieldSetString(resourceEntryElement[XLABEL_RC],
		globalResourceBundle.xlabel);
      strcpy(globalResourceBundle.ylabel, p->plotcom.ylabel);
      XmTextFieldSetString(resourceEntryElement[YLABEL_RC],
		globalResourceBundle.ylabel);
      globalResourceBundle.clr = p->plotcom.clr;
      XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
	  currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
      globalResourceBundle.bclr = p->plotcom.bclr;
      XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
	  currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
      globalResourceBundle.count = p->count;
      sprintf(string,"%d",globalResourceBundle.count);
      XmTextFieldSetString(resourceEntryElement[COUNT_RC],string);
      globalResourceBundle.cStyle = p->style;
      optionMenuSet(resourceEntryElement[CSTYLE_RC],
		globalResourceBundle.cStyle - FIRST_CARTESIAN_PLOT_STYLE);
      globalResourceBundle.erase_oldest = p->erase_oldest;
      optionMenuSet(resourceEntryElement[ERASE_OLDEST_RC],
		globalResourceBundle.erase_oldest - FIRST_ERASE_OLDEST);
      for (i = 0; i < MAX_TRACES; i++){
        strcpy(globalResourceBundle.cpData[i].xdata, p->trace[i].xdata);  
	strcpy(globalResourceBundle.cpData[i].ydata, p->trace[i].ydata);  
	globalResourceBundle.cpData[i].data_clr = p->trace[i].data_clr;
      }
      for (i = X_AXIS_ELEMENT; i <= Y2_AXIS_ELEMENT; i++) {
        globalResourceBundle.axis[i].axisStyle = p->axis[i].axisStyle;
	globalResourceBundle.axis[i].rangeStyle = p->axis[i].rangeStyle;
	globalResourceBundle.axis[i].minRange = p->axis[i].minRange;
	globalResourceBundle.axis[i].maxRange = p->axis[i].maxRange;
      }
      strcpy(globalResourceBundle.trigger, p->trigger);
      XmTextFieldSetString(resourceEntryElement[TRIGGER_RC],
		globalResourceBundle.trigger);
      strcpy(globalResourceBundle.erase, p->erase);
      XmTextFieldSetString(resourceEntryElement[ERASE_RC],
		globalResourceBundle.erase);
      globalResourceBundle.eraseMode = p->eraseMode;
      optionMenuSet(resourceEntryElement[ERASE_MODE_RC],
		globalResourceBundle.eraseMode - FIRST_ERASE_MODE);
      break;
    }
    case DL_Rectangle: {
      DlRectangle *p = elementPtr->structure.rectangle;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleBasicAttribute(&(p->attr));
      updateResourcePaletteBasicAttribute();
      updateGlobalResourceBundleDynamicAttribute(&(p->dynAttr));
      updateResourcePaletteDynamicAttribute();
      break;
    }
    case DL_Oval: {
      DlOval *p = elementPtr->structure.oval;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleBasicAttribute(&(p->attr));
      updateResourcePaletteBasicAttribute();
      updateGlobalResourceBundleDynamicAttribute(&(p->dynAttr));
      updateResourcePaletteDynamicAttribute();
      break;
    }
    case DL_Arc: {
      DlArc *p = elementPtr->structure.arc;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleBasicAttribute(&(p->attr));
      updateResourcePaletteBasicAttribute();
      updateGlobalResourceBundleDynamicAttribute(&(p->dynAttr));
      updateResourcePaletteDynamicAttribute();

      /* want user to see degrees, but internally use
       * degrees*64 as Xlib requires
       */
      globalResourceBundle.begin = p->begin;
      XmScaleSetValue(resourceEntryElement[BEGIN_RC],
			globalResourceBundle.begin/64);
      globalResourceBundle.path = p->path;
      XmScaleSetValue(resourceEntryElement[PATH_RC],
			globalResourceBundle.path/64);
      break;
    }
    case DL_Text: {
      DlText *p = elementPtr->structure.text;

#if 0
      if (objectDataOnly) {
      	updateGlobalResourceBundleObjectAttribute(&(p->object));
      	updateResourcePaletteObjectAttribute();
			} else {
        elementPtr->setValues(&globaleResourceBundle,elementPtr);
				updateResourceBundle(&globalResourceBundle);
			}
#else
      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleBasicAttribute(&(p->attr));
      updateResourcePaletteBasicAttribute();
      updateGlobalResourceBundleDynamicAttribute(&(p->dynAttr));
      updateResourcePaletteDynamicAttribute();

      strcpy(globalResourceBundle.textix, p->textix);
      XmTextFieldSetString(resourceEntryElement[TEXTIX_RC],
		globalResourceBundle.textix);
      globalResourceBundle.align = p->align;
      optionMenuSet(resourceEntryElement[ALIGN_RC],
		globalResourceBundle.align - FIRST_TEXT_ALIGN);
#endif
      break;
    }
    case DL_RelatedDisplay: {
      DlRelatedDisplay *p = elementPtr->structure.relatedDisplay;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      globalResourceBundle.clr = p->clr;
      XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
      globalResourceBundle.bclr = p->bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
      for (i = 0; i < MAX_RELATED_DISPLAYS; i++){
	strcpy(globalResourceBundle.rdData[i].label, p->display[i].label);  
	strcpy(globalResourceBundle.rdData[i].name, p->display[i].name);  
	strcpy(globalResourceBundle.rdData[i].args, p->display[i].args);  
	/* update the related display dialog (matrix of values) if appr. */
	updateRelatedDisplayDataDialog();
      }
      break;
    }
    case DL_ShellCommand: {
      DlShellCommand *p = elementPtr->structure.shellCommand;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      globalResourceBundle.clr = p->clr;
      XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.clr],NULL);
      globalResourceBundle.bclr = p->bclr;
      XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
		currentDisplayInfo->dlColormap[globalResourceBundle.bclr],NULL);
      for (i = 0; i < MAX_SHELL_COMMANDS; i++){
        strcpy(globalResourceBundle.cmdData[i].label, p->command[i].label);  
	strcpy(globalResourceBundle.cmdData[i].command, p->command[i].command);
	strcpy(globalResourceBundle.cmdData[i].args, p->command[i].args);  
	/* update the shell command dialog (matrix of values) if appr. */
	updateShellCommandDataDialog();
      }
      break;
    }
    case DL_Image: {
      DlImage *p = elementPtr->structure.image;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      globalResourceBundle.imageType = p->imageType;
      optionMenuSet(resourceEntryElement[IMAGETYPE_RC],
		globalResourceBundle.imageType - FIRST_IMAGE_TYPE);
      strcpy(globalResourceBundle.imageName, p->imageName);
      XmTextFieldSetString(resourceEntryElement[IMAGENAME_RC],
		globalResourceBundle.imageName);
      break;
    }
    case DL_Composite: {
      DlComposite *p = elementPtr->structure.composite;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      globalResourceBundle.vis = p->vis;
      optionMenuSet(resourceEntryElement[VIS_RC],
		globalResourceBundle.vis - FIRST_VISIBILITY_MODE);
      strcpy(globalResourceBundle.chan,p->chan);
      XmTextFieldSetString(resourceEntryElement[CHAN_RC],
		globalResourceBundle.chan);
/* need to add this entry to widgetDM.h and finish this if we want named
 *  groups
      strcpy(globalResourceBundle.compositeName,p->compositeName);
 */
	break;
    }
    case DL_Polyline: {
      DlPolyline *p = elementPtr->structure.polyline;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleBasicAttribute(&(p->attr));
      updateResourcePaletteBasicAttribute();
      updateGlobalResourceBundleDynamicAttribute(&(p->dynAttr));
      updateResourcePaletteDynamicAttribute();
      break;
    }
    case DL_Polygon: {
      DlPolygon *p = elementPtr->structure.polygon;

      updateGlobalResourceBundleObjectAttribute(&(p->object));
      updateResourcePaletteObjectAttribute();
      if (objectDataOnly) return;

      updateGlobalResourceBundleBasicAttribute(&(p->attr));
      updateResourcePaletteBasicAttribute();
      updateGlobalResourceBundleDynamicAttribute(&(p->dynAttr));
      updateResourcePaletteDynamicAttribute();
      break;
    }
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
  int minX, minY, maxX, maxY;

  /* simply return if not valid to update */
  if (!elementPtr || !currentDisplayInfo) return;

  widget = (Widget) NULL;

  /* this is common to all - therefore put here */
  if (ELEMENT_IS_RENDERABLE(elementPtr->type) ) {

    /* first lookup the widget based on current position/dimension */
    if (ELEMENT_HAS_WIDGET(elementPtr->type))
	widget = lookupElementWidget(currentDisplayInfo,
                        &(elementPtr->structure.rectangle->object));
  }


  switch (elementPtr->type) {
    case DL_File:
      strcpy(elementPtr->structure.file->name,globalResourceBundle.name);
      break;

    case DL_Display:
      updateElementObjectAttribute(&(elementPtr->structure.display->object));
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
    case DL_Valuator: {
      DlValuator *p = elementPtr->structure.valuator;
      updateElementObjectAttribute(&(p->object));
      updateElementControlAttribute(&(p->control));
      p->label = globalResourceBundle.label;
      p->clrmod = globalResourceBundle.clrmod;
      p->direction = globalResourceBundle.direction;
      p->dPrecision = globalResourceBundle.dPrecision;
      break;
    }
    case DL_ChoiceButton: {
      DlChoiceButton *p = elementPtr->structure.choiceButton;
      updateElementObjectAttribute(&(p->object));
      updateElementControlAttribute(&(p->control));
      p->clrmod = globalResourceBundle.clrmod;
      p->stacking = globalResourceBundle.stacking;
      break;
    }
    case DL_MessageButton: {
      DlMessageButton *p = elementPtr->structure.messageButton;
      updateElementObjectAttribute(&(p->object));
      updateElementControlAttribute(&(p->control));
      strcpy(p->label,globalResourceBundle.messageLabel);
      strcpy(p->press_msg,globalResourceBundle.press_msg);
      strcpy(p->release_msg,globalResourceBundle.release_msg);
      p->clrmod = globalResourceBundle.clrmod;
      break;
    }
    case DL_TextEntry: {
      DlTextEntry *p = elementPtr->structure.textEntry;
      updateElementObjectAttribute(&(p->object));
      updateElementControlAttribute(&(p->control));
      p->clrmod = globalResourceBundle.clrmod;
      p->format = globalResourceBundle.format;
      break;
    }
    case DL_Menu: {
      DlMenu *p = elementPtr->structure.menu;
      updateElementObjectAttribute(&(p->object));
      updateElementControlAttribute(&(p->control));
      p->clrmod = globalResourceBundle.clrmod;
      break;
    }
    case DL_Meter: {
      DlMeter *p = elementPtr->structure.meter;
      updateElementObjectAttribute(&(p->object));
      updateElementMonitorAttribute(&(p->monitor));
      p->label = globalResourceBundle.label;
      p->clrmod = globalResourceBundle.clrmod;
      break;
    }
    case DL_TextUpdate: {
      DlTextUpdate *p = elementPtr->structure.textUpdate;
      updateElementObjectAttribute(&(p->object));
      updateElementMonitorAttribute(&(p->monitor));
      p->clrmod = globalResourceBundle.clrmod;
      p->align = globalResourceBundle.align;
      p->format = globalResourceBundle.format;
      break;
    }
    case DL_Bar: {
      DlBar *p = elementPtr->structure.bar;
      updateElementObjectAttribute(&(p->object));
      updateElementMonitorAttribute(&(p->monitor));
      p->label = globalResourceBundle.label;
      p->clrmod = globalResourceBundle.clrmod;
      p->direction = globalResourceBundle.direction;
      p->fillmod = globalResourceBundle.fillmod;
      break;
    }
    case DL_Byte: {
      DlByte *p = elementPtr->structure.byte;
      updateElementObjectAttribute(&(p->object));
      updateElementMonitorAttribute(&(p->monitor));
      p->clrmod = globalResourceBundle.clrmod;
      p->direction = globalResourceBundle.direction;
      p->sbit = globalResourceBundle.sbit;
      p->ebit = globalResourceBundle.ebit;
      break;
    }
    case DL_Indicator: {
      DlIndicator *p = elementPtr->structure.indicator;
      updateElementObjectAttribute(&(p->object));
      updateElementMonitorAttribute(&(p->monitor));
      p->label = globalResourceBundle.label;
      p->clrmod = globalResourceBundle.clrmod;
      p->direction = globalResourceBundle.direction;
      break;
    }
    case DL_StripChart: {
      DlStripChart *p = elementPtr->structure.stripChart;
      updateElementObjectAttribute(&(p->object));
      strcpy(p->plotcom.title,globalResourceBundle.title);
      strcpy(p->plotcom.xlabel,globalResourceBundle.xlabel);
      strcpy(p->plotcom.ylabel,globalResourceBundle.ylabel);
      p->plotcom.clr = globalResourceBundle.clr;
      p->plotcom.bclr = globalResourceBundle.bclr;
      p->period = globalResourceBundle.period;
      p->units = globalResourceBundle.units;
      for (i = 0; i < MAX_PENS; i++){
	strcpy(p->pen[i].chan,globalResourceBundle.scData[i].chan);
	p->pen[i].clr = globalResourceBundle.scData[i].clr;
      }
      break;
    }
    case DL_CartesianPlot: {
      DlCartesianPlot *p = elementPtr->structure.cartesianPlot;
      updateElementObjectAttribute(&(p->object));
      strcpy(p->plotcom.title,globalResourceBundle.title);
      strcpy(p->plotcom.xlabel,globalResourceBundle.xlabel);
      strcpy(p->plotcom.ylabel,globalResourceBundle.ylabel);
      p->plotcom.clr = globalResourceBundle.clr;
      p->plotcom.bclr = globalResourceBundle.bclr;
      p->count = globalResourceBundle.count;
      p->style = globalResourceBundle.cStyle;
      p->erase_oldest = globalResourceBundle.erase_oldest;
      for (i = 0; i < MAX_TRACES; i++){
        strcpy(p->trace[i].xdata,globalResourceBundle.cpData[i].xdata);
        strcpy(p->trace[i].ydata,globalResourceBundle.cpData[i].ydata);
	p->trace[i].data_clr = globalResourceBundle.cpData[i].data_clr;
      }
      p->axis[X_AXIS_ELEMENT].axisStyle =
		globalResourceBundle.axis[X_AXIS_ELEMENT].axisStyle;
      p->axis[X_AXIS_ELEMENT].rangeStyle =
		globalResourceBundle.axis[X_AXIS_ELEMENT].rangeStyle;
      p->axis[X_AXIS_ELEMENT].minRange =
		globalResourceBundle.axis[X_AXIS_ELEMENT].minRange;
      p->axis[X_AXIS_ELEMENT].maxRange =
		globalResourceBundle.axis[X_AXIS_ELEMENT].maxRange;
      p->axis[Y1_AXIS_ELEMENT].axisStyle =
		globalResourceBundle.axis[Y1_AXIS_ELEMENT].axisStyle;
      p->axis[Y1_AXIS_ELEMENT].rangeStyle =
		globalResourceBundle.axis[Y1_AXIS_ELEMENT].rangeStyle;
      p->axis[Y1_AXIS_ELEMENT].minRange =
		globalResourceBundle.axis[Y1_AXIS_ELEMENT].minRange;
      p->axis[Y1_AXIS_ELEMENT].maxRange =
		globalResourceBundle.axis[Y1_AXIS_ELEMENT].maxRange;
      p->axis[Y2_AXIS_ELEMENT].axisStyle =
		globalResourceBundle.axis[Y2_AXIS_ELEMENT].axisStyle;
      p->axis[Y2_AXIS_ELEMENT].rangeStyle =
		globalResourceBundle.axis[Y2_AXIS_ELEMENT].rangeStyle;
      p->axis[Y2_AXIS_ELEMENT].minRange =
		globalResourceBundle.axis[Y2_AXIS_ELEMENT].minRange;
      p->axis[Y2_AXIS_ELEMENT].maxRange =
		globalResourceBundle.axis[Y2_AXIS_ELEMENT].maxRange;
      strcpy(p->trigger,globalResourceBundle.trigger);
      strcpy(p->erase,globalResourceBundle.erase);
      p->eraseMode = globalResourceBundle.eraseMode;
      break;
    }
    case DL_Rectangle: {
      DlRectangle *p = elementPtr->structure.rectangle;
      updateElementObjectAttribute(&(p->object));
      updateElementBasicAttribute(&(p->attr));
      updateElementDynamicAttribute(&(p->dynAttr));
      break;
    }
    case DL_Oval: {
      DlOval *p = elementPtr->structure.oval;
      updateElementObjectAttribute(&(p->object));
      updateElementBasicAttribute(&(p->attr));
      updateElementDynamicAttribute(&(p->dynAttr));
      break;
    }
    case DL_Arc: {
      DlArc *p = elementPtr->structure.arc;
      updateElementObjectAttribute(&(p->object));
      updateElementBasicAttribute(&(p->attr));
      updateElementDynamicAttribute(&(p->dynAttr));
      p->begin = globalResourceBundle.begin;
      p->path = globalResourceBundle.path;
      break;
    }
    case DL_Text: {
      DlText *p = elementPtr->structure.text;
      updateElementObjectAttribute(&(p->object));
      updateElementBasicAttribute(&(p->attr));
      updateElementDynamicAttribute(&(p->dynAttr));
      strcpy(p->textix,globalResourceBundle.textix);
      p->align = globalResourceBundle.align;
      break;
    }
    case DL_RelatedDisplay: {
      DlRelatedDisplay *p = elementPtr->structure.relatedDisplay;
      updateElementObjectAttribute(&(p->object));
      p->clr = globalResourceBundle.clr;
      p->bclr = globalResourceBundle.bclr;
      for (i = 0; i < MAX_RELATED_DISPLAYS; i++){
        strcpy(p->display[i].label,globalResourceBundle.rdData[i].label);
	strcpy(p->display[i].name,globalResourceBundle.rdData[i].name);
	strcpy(p->display[i].args,globalResourceBundle.rdData[i].args);
      }
      break;
    }
    case DL_ShellCommand: {
      DlShellCommand *p = elementPtr->structure.shellCommand;
      updateElementObjectAttribute(&(p->object));
      p->clr = globalResourceBundle.clr;
      p->bclr = globalResourceBundle.bclr;
      for (i = 0; i < MAX_SHELL_COMMANDS; i++){
        strcpy(p->command[i].label,globalResourceBundle.cmdData[i].label);
	strcpy(p->command[i].command,globalResourceBundle.cmdData[i].command);
	strcpy(p->command[i].args,globalResourceBundle.cmdData[i].args);
      }
      break;
    }
    case DL_Image: {
      DlImage *p = elementPtr->structure.image;
      updateElementObjectAttribute(&(p->object));
      p->imageType = globalResourceBundle.imageType;
      strcpy(p->imageName,globalResourceBundle.imageName);
      break;
    }
    case DL_Composite: {
      DlComposite *p = elementPtr->structure.composite;
      DlObject obj = p->object;
      float xScale, yScale;

      updateElementObjectAttribute(&(p->object));
      if (memcmp(&(p->object),&obj,sizeof(DlObject))) {
        xOffset = p->object.x - obj.x; 
        yOffset = p->object.y - obj.y;
        xScale = (obj.width > 0) ?
               ((float)p->object.width / (float)obj.width) : 1;
        yScale = (obj.height > 0) ?
               ((float)p->object.height / (float)obj.height) : 1;

        moveWidgets = True;
        if (xOffset != 0 || yOffset != 0) {
	   moveCompositeChildren(currentDisplayInfo,elementPtr,
              xOffset,yOffset,moveWidgets);
        }
        if (xScale != 1.0 || deltaHeight != 1.0) {
	 resizeCompositeChildren(currentDisplayInfo,elementPtr,
			elementPtr,sX,sY);
        }
      }
      p->vis = globalResourceBundle.vis;
      strcpy(p->chan,globalResourceBundle.chan);
      break;
    }
/* handle change in lineWidth if Polyline, or Polygon w/fill==F_OUTLINE */
    case DL_Polyline: {
      DlPolyline *p = elementPtr->structure.polyline;
      DlObject obj = p->object;
      float xScale, yScale;
      int reCalDimension = 0;
      unsigned int width = p->attr.width;

      minX = INT_MAX; minY = INT_MAX; maxX = INT_MIN; maxY = INT_MIN;
      updateElementObjectAttribute(&(p->object));
      updateElementBasicAttribute(&(p->attr));
      updateElementDynamicAttribute(&(p->dynAttr));
      if (p->attr.width != obj.width) reCalDimension = 1;
      if (memcmp(&(p->object),&obj,sizeof(DlObject))) {
        xOffset = p->object.x - obj.x; 
        yOffset = p->object.y - obj.y;
        xScale = (obj.width > 0) ?
               ((float)p->object.width / (float)obj.width) : 1;
        yScale = (obj.height > 0) ?
               ((float)p->object.height / (float)obj.height) : 1;
        for (j = 0; j < p->nPoints; j++) {
          /* this works because really only changing one of x/y/w/h at a time */
	  p->points[j].x += xOffset;
	  p->points[j].y += yOffset;
	  p->points[j].x =
                  p->object.x + (p->points[j].x - p->object.x) * xScale;
	  p->points[j].y =
                  p->object.y + (p->points[j].y - p->object.y) * yScale;
        }
        reCalDimension = 1;
      }
      if (reCalDimension) {
        for (j = 0; j < p->nPoints; j++) {
	  minX = MIN(minX,p->points[j].x);
	  maxX = MAX(maxX,p->points[j].x);
	  minY = MIN(minY,p->points[j].y);
	  maxY = MAX(maxY,p->points[j].y);
        }
        p->object.x =  minX - p->attr.width/2;
        p->object.y =  minY - p->attr.width/2;
        p->object.width =  maxX - minX + p->attr.width;
        p->object.height =  maxY - minY + p->attr.width;
      }
      break;
    }
    case DL_Polygon: {
      DlPolygon *p = elementPtr->structure.polygon;
      DlObject obj = p->object;
      float xScale, yScale;
      int reCalDimension = 0;
      unsigned int width = p->attr.width;

      minX = INT_MAX; minY = INT_MAX; maxX = INT_MIN; maxY = INT_MIN;
      updateElementObjectAttribute(&(p->object));
      updateElementBasicAttribute(&(p->attr));
      updateElementDynamicAttribute(&(p->dynAttr));
      if (p->attr.width != obj.width) reCalDimension = 1;
      if (memcmp(&(p->object),&obj,sizeof(DlObject))) {
        xOffset = p->object.x - obj.x;
        yOffset = p->object.y - obj.y;
        xScale = (obj.width > 0) ?
               ((float)p->object.width / (float)obj.width) : 1;
        yScale = (obj.height > 0) ?
               ((float)p->object.height / (float)obj.height) : 1;
        for (j = 0; j < p->nPoints; j++) {
        /* this works because really only changing one of x/y/w/h at a time */
          p->points[j].x += xOffset;
          p->points[j].y += yOffset;
          p->points[j].x =
                  p->object.x + (p->points[j].x - p->object.x) * xScale;
         p->points[j].y =
                  p->object.y + (p->points[j].y - p->object.y) * yScale;
       }
        reCalDimension = 1;
      }
      if (reCalDimension) {
        for (j = 0; j < p->nPoints; j++) {
	  minX = MIN(minX,p->points[j].x);
	  maxX = MAX(maxX,p->points[j].x);
	  minY = MIN(minY,p->points[j].y);
	  maxY = MAX(maxY,p->points[j].y);
        }
	if (globalResourceBundle.fill == F_SOLID) {
	  p->object.x =  minX;
	  p->object.y =  minY;
	  p->object.width =  maxX - minX;
	  p->object.height =  maxY - minY;
	} else {	/* F_OUTLINE, therfore lineWidth is a factor */
          p->object.x =  minX - p->attr.width/2;
          p->object.y =  minY - p->attr.width/2;
          p->object.width =  maxX - minX + p->attr.width;
          p->object.height =  maxY - minY + p->attr.width;
	}
      }
      break;
    }
    default:
	medmPrintf(
	   "\n updateElementFromResourceBundle: unknown element type %d",
	    elementPtr->type);
	break;

  }
/* now update the element's widget by recreating the widget... */
  if (ELEMENT_HAS_WIDGET(elementPtr->type)) {
    if (widget) {
      if (widget != currentDisplayInfo->drawingArea) {
        destroyElementWidget(currentDisplayInfo,widget);
					(*elementPtr->dmExecute)(currentDisplayInfo,
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
