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

#include <Xm/ToggleB.h>
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
# define N_OPTIONS_MENU_ELES 2
# define OPTIONS_BTN_POSN 1
# define HELP_BTN_POSN 3
#else
# define N_MAIN_MENU_ELES 3
# define HELP_BTN_POSN 2
#endif

#define N_FILE_MENU_ELES 1
#define FILE_BTN_POSN 0
#define FILE_CLOSE_BTN 0

#define HELP_OBJECT_PALETTE_BTN 0
#define HELP_OBJECT_OBJECTS_BTN 1

typedef struct {
    char *         pixmapName;
    Widget         widget;
    XtCallbackProc callback;
    XtPointer      clientData;
    char *         bubbleHelpString;
} buttons_t;

static void helpObjectCallback(Widget,XtPointer,XtPointer);
static void objectToggleCallback(Widget, XtPointer, XtPointer);

static XtCallbackProc objectToggleSelectCallback(
  Widget, XtPointer, XmToggleButtonCallbackStruct *);

buttons_t paletteGraphicsButton[] = {
    {"rectangle25",NULL,objectToggleCallback,(XtPointer)DL_Rectangle,"Rectangle"},
    {"oval25",NULL,objectToggleCallback,(XtPointer)DL_Oval,"Oval"},
    {"arc25",NULL,objectToggleCallback,(XtPointer)DL_Arc,"Arc"},
    {"text25",NULL,objectToggleCallback,(XtPointer)DL_Text,"Text"},
    {"polyline25",NULL,objectToggleCallback,(XtPointer)DL_Polyline,"Polyline"},
    {"line25",NULL,objectToggleCallback,(XtPointer)DL_Line,"Line"},
    {"polygon25",NULL,objectToggleCallback,(XtPointer)DL_Polygon,"Polygon"},
    {"image25",NULL,objectToggleCallback,(XtPointer)DL_Image,"Image"},
    NULL};
/* Not implemented
  {"bezierCurve25",NULL,objectToggleCallback,(XtPointer)DL_BezierCurve},
*/
buttons_t paletteMonitorButton[] = {
    {"meter25",NULL,objectToggleCallback,(XtPointer)DL_Meter,"Meter"},
    {"bar25",NULL,objectToggleCallback,(XtPointer)DL_Bar,"Bar Monitor"},
    {"stripChart25",NULL,objectToggleCallback,(XtPointer)DL_StripChart,"Strip Chart"},
    {"textUpdate25",NULL,objectToggleCallback,(XtPointer)DL_TextUpdate,"Text Monitor"},
    {"indicator25",NULL,objectToggleCallback,(XtPointer)DL_Indicator,"Scale Monitor"},
    {"cartesianPlot25",NULL,objectToggleCallback,(XtPointer)DL_CartesianPlot,"Cartesian Plot"},
/* Not implemented
   {"surfacePlot25",NULL,objectToggleCallback,(XtPointer)DL_SurfacePlot,"Surface Plot"},
   */
    {"byte25",NULL,objectToggleCallback,(XtPointer)DL_Byte,"Byte Monitor"},
    NULL};

buttons_t paletteControllerButton[] = {
    {"choiceButton25",NULL, objectToggleCallback,(XtPointer)DL_ChoiceButton,"Choice Button"},
    {"textEntry25",NULL, objectToggleCallback,(XtPointer)DL_TextEntry,"Text Entry"},
    {"messageButton25",NULL, objectToggleCallback,(XtPointer)DL_MessageButton,"Message Button"},
    {"menu25",NULL, objectToggleCallback,(XtPointer)DL_Menu,"Menu"},
    {"valuator25",NULL, objectToggleCallback,(XtPointer)DL_Valuator,"Slider"},
    {"relatedDisplay25",NULL, objectToggleCallback,(XtPointer)DL_RelatedDisplay,"Related Display"},
    {"shellCommand25",NULL, objectToggleCallback,(XtPointer)DL_ShellCommand,"Shell Command"},
    NULL};

buttons_t paletteMiscButton[] = {
    {"select25",NULL, objectToggleCallback,NULL,"Select"},
    NULL};

buttons_t *buttonList[] = {
    paletteGraphicsButton,
    paletteMonitorButton,
    paletteControllerButton,
    NULL};

static Widget lastButton = NULL;

Widget objectFilePDM;
extern Widget imageNameFSD;

/*
 * global widget for ObjectPalette's SELECT toggle button (needed for
 *	programmatic toggle/untoggle of SELECT/CREATE modes
 */
Widget objectPaletteSelectToggleButton;

extern XButtonPressedEvent lastEvent;

/********************************************
 **************** Callbacks *****************
 ********************************************/

/*
 * object palette's state transition callback - updates resource palette
 */
void objectMenuCallback(Widget w, XtPointer clientData, XtPointer cbs)
{
    DlElementType type = (DlElementType) clientData;
    DisplayInfo *di;

    UNREFERENCED(w);
    UNREFERENCED(cbs);

  /* Move the pointer back the original location */
    di = currentDisplayInfo;
    XWarpPointer(display,None,XtWindow(di->drawingArea),0,0,
      0,0,lastEvent.x, lastEvent.y);

  /* Unselect any selected elements */
    unselectElementsInDisplay();

  /* Set global action (from object palette) to CREATE, & global element type */
    currentActionType = CREATE_ACTION;
    currentElementType = type;
    setResourcePaletteEntries();
    di = displayInfoListHead->next;
    while(di != NULL) {
	XDefineCursor(display,XtWindow(di->drawingArea),crosshairCursor);
	di = di->next;
    }

    if (objectS) {
	int i,j;

      /* Allow only one button pressed at a time */
	if (lastButton)
	  XmToggleButtonSetState(lastButton,False,False);
	for (i = 0; buttonList[i] != NULL; i++) {
	    for (j = 0; buttonList[i][j].pixmapName != NULL; j++) {
		if (buttonList[i][j].clientData == (XtPointer)type) {
		    lastButton = buttonList[i][j].widget;
		    XmToggleButtonSetState(lastButton,True,False);
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

    clearResourcePaletteEntries();

    if (objectS) 
      XDefineCursor(display,XtWindow(objectS),rubberbandCursor);
    di = displayInfoListHead->next;
    while(di) {
	XDefineCursor(display,XtWindow(di->drawingArea),rubberbandCursor);
	di = di->next;
    }
    return;
}

/*
 * object palette's state transition callback - updates resource palette
 */
static void objectToggleCallback(Widget w, XtPointer clientData,
  XtPointer callbackStruct)
{
    DisplayInfo *di;
    DlElementType type = (DlElementType) clientData;     /* KE: Not used, Possibly not valid  */
    XmToggleButtonCallbackStruct *call_data = (XmToggleButtonCallbackStruct *) callbackStruct;

  /* Pushing one of these toggles implies create object of this type,
   *   and MB in a display now takes on CREATE semantics */

    if (call_data->set == False) return;

  /* Allow only one button pressed at a time */
    if ((lastButton) && (lastButton != w)) {
	XmToggleButtonSetState(lastButton,False,False);
	lastButton = w;
    }

    if (w == objectPaletteSelectToggleButton) {
	setActionToSelect();
    } else {
      /* Unselect any selected elements */
	unselectElementsInDisplay();

      /* Set global action (from object palette) to CREATE, & global element type */
	currentActionType = CREATE_ACTION;
	currentElementType = type;
	setResourcePaletteEntries();
	XDefineCursor(display,XtWindow(objectS),crosshairCursor);
	di = displayInfoListHead->next;
	while(di) {
	    XDefineCursor(display,XtWindow(di->drawingArea),crosshairCursor);
	    di = di->next;
	}
    }
    return;
}

static void fileMenuSimpleCallback(Widget w, XtPointer clientData,
  XtPointer cbs)
{
    int buttonNumber = (int) clientData;
    
    UNREFERENCED(w);
    UNREFERENCED(cbs);

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

/* Used to create radio buttons in the object palette */
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

	pixmap = XmGetPixmap(XtScreen(rowCol),b[i].pixmapName,fg,bg);
	b[i].widget = XtVaCreateManagedWidget(b[i].pixmapName,
	  xmToggleButtonWidgetClass, rowCol,     /* Must be a widget for event handler */
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
	XtAddEventHandler(b[i].widget,EnterWindowMask|LeaveWindowMask,False,
	  (XtEventHandler)handleBubbleHelp,(XtPointer)b[i].bubbleHelpString);
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
      fileMenuSimpleCallback, (XtPointer)FILE_CLOSE_BTN,  NULL},
    NULL,
};

#ifdef EXTENDED_INTERFACE
static menuEntry_t optionMenu[] = {
    { "User Palette...",  &xmPushButtonGadgetClass, 'U', NULL, NULL, NULL,
      fileMenuSimpleCallback, (XtPointer)OPTION_USER_PALETTE_BTN,  NULL},
    NULL,
};
#endif

static menuEntry_t helpMenu[] = {
    { "On Object Palette",  &xmPushButtonGadgetClass, 'P', NULL, NULL, NULL,
      helpObjectCallback, (XtPointer)HELP_OBJECT_PALETTE_BTN, NULL},
    { "Object Index", &xmPushButtonGadgetClass, 'I', NULL, NULL, NULL,
      helpObjectCallback, (XtPointer)HELP_OBJECT_OBJECTS_BTN, NULL},
    NULL,
};

void createObject()
{
    Widget objectRC;
    Widget graphicsRC, monitorRC, controllerRC, miscRC;
    Widget objectMB;
    Widget objectHelpPDM;

/*
 * initialize local static globals
 */
    imageNameFSD = NULL;

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
  /* KE: The following is necessary for Exceed, which turns off the
     resize function with the handles.  It should not be necessary */
      XmNmwmFunctions, MWM_FUNC_ALL,
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
/*     XtSetSensitive(objectHelpPDM,False); */

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

static void helpObjectCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    int buttonNumber = (int)cd;
    XmAnyCallbackStruct *call_data = (XmAnyCallbackStruct *)cbs;
    
    UNREFERENCED(w);

    switch(buttonNumber) {
    case HELP_OBJECT_PALETTE_BTN:
	callBrowser(medmHelpPath,"#ObjectPalette");
	break;
    case HELP_OBJECT_OBJECTS_BTN:
	callBrowser(medmHelpPath,"#ObjectIndex");
	break;
    }
}
