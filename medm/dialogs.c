/* Dialog box routines */

#define PV_INFO_CLOSE_BTN 0
#define PV_INFO_HELP_BTN  1

/* These must start with 3, 0-2 are for option menu buttons */
#define PV_LIMITS_NAME_BTN         3
#define PV_LIMITS_LOPR_SRC_BTN     4
#define PV_LIMITS_LOPR_BTN         5
#define PV_LIMITS_HOPR_SRC_BTN     6
#define PV_LIMITS_HOPR_BTN         7
#define PV_LIMITS_PREC_SRC_BTN     8
#define PV_LIMITS_PREC_BTN         9
#define PV_LIMITS_CLOSE_BTN        10
#define PV_LIMITS_HELP_BTN         11

#define PV_LIMITS_EDIT_NAME "EDIT Mode Limits"

#define PV_LIMITS_INDENT 25

#define DISPLAY_LIST_RAISE_BTN     0
#define DISPLAY_LIST_CLOSE1_BTN    1
#define DISPLAY_LIST_CLOSE_BTN     2
#define DISPLAY_LIST_REFRESH_BTN   3
#define DISPLAY_LIST_HELP_BTN      4

/* These must start with 4, 0-3 are for orientation menu buttons */
#define PRINT_SETUP_OK_BTN         4
#define PRINT_SETUP_CANCEL_BTN     5
#define PRINT_SETUP_HELP_BTN       6
#define PRINT_SETUP_CMD_FIELD      7
#define PRINT_SETUP_ORIENT_BTN     8
#define PRINT_SETUP_WIDTH_FIELD    9
#define PRINT_SETUP_HEIGHT_FIELD   10
#define PRINT_SETUP_FILENAME_FIELD 11
#define PRINT_SETUP_TOFILE_BTN     12

#include <string.h>
#include <time.h>
#include <ctype.h>
#include <X11/IntrinsicP.h>
#include <Xm/MwmUtil.h>
#include <Xm/List.h>
#include <cvtFast.h>
#include <postfix.h>
#include "medm.h"

/* Function prototypes */

static void createPrintSetupDlg(void);
static void printSetupDialogCallback(Widget, XtPointer, XtPointer cbs);
static void pvInfoDialogCallback(Widget, XtPointer, XtPointer cbs);
static void pvLimitsDialogCallback(Widget, XtPointer, XtPointer cbs);
static void pvLimitsLosingFocusCallback(Widget w, XtPointer, XtPointer);
static void createPvLimitsDlg(void);
static void displayListDlgCb(Widget w, XtPointer clientData,
  XtPointer callData);
static void resetPvLimitsDlg(DlLimits *limits, char *pvName, Boolean doName);

/* Global variables */
static Widget displayListBox1 = NULL, displayListBox2 = NULL;
static Widget pvLimitsName, pvLimitsLopr, pvLimitsHopr, pvLimitsPrec;
static Widget pvLimitsLoprSrc, pvLimitsHoprSrc, pvLimitsPrecSrc;
static Widget printSetupCommandTF, printSetupFileTF, printSetupTitleTF;
static Widget printSetupWidthTF, printSetupHeightTF, printSetupPrintToFileTB;
/*** PV Info routines ***/

void createPvInfoDlg(void)
{
    Widget pane;
    Widget actionArea;
    Widget closeButton, helpButton;
    Arg args[10];
    int n;

    if(pvInfoS != NULL) {
	return;
    }

    if(mainShell == NULL) return;

    pvInfoS = XtVaCreatePopupShell("pvInfoS",
      topLevelShellWidgetClass, mainShell,
      XmNtitle, "PV Info",
      XmNdeleteResponse, XmDO_NOTHING,
      NULL);
    XmAddWMProtocolCallback(pvInfoS,WM_DELETE_WINDOW,
      wmCloseCallback,(XtPointer)OTHER_SHELL);

    pane = XtVaCreateWidget("panel",
      xmPanedWindowWidgetClass, pvInfoS,
      XmNsashWidth, 1,
      XmNsashHeight, 1,
      NULL);
    n = 0;
    XtSetArg(args[n], XmNrows, 28); n++;
    XtSetArg(args[n], XmNcolumns, 38); n++;
    XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
    XtSetArg(args[n], XmNeditable, False); n++;
    pvInfoMessageBox = XmCreateScrolledText(pane,"text",args,n);
    XtManageChild(pvInfoMessageBox);

    actionArea = XtVaCreateWidget("actionArea",
      xmFormWidgetClass, pane,
      XmNshadowThickness, 0,
      XmNfractionBase, 5,
      XmNskipAdjust, True,
      NULL);

    closeButton = XtVaCreateManagedWidget("Close",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     1,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    2,
#if 0      
      XmNshowAsDefault,    True,
      XmNdefaultButtonShadowThickness, 1,
#endif      
      NULL);
    XtAddCallback(closeButton,XmNactivateCallback,pvInfoDialogCallback,
      (XtPointer)PV_INFO_CLOSE_BTN);
    
#if 0
    printButton = XtVaCreateManagedWidget("Print",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     3,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    4,
      NULL);
    XtAddCallback(printButton,XmNactivateCallback,pvInfoDialogCallback,
      (XtPointer)PV_INFO_PRINT_BTN);
#endif    

    helpButton = XtVaCreateManagedWidget("Help",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     3,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    4,
      NULL);
    XtAddCallback(helpButton,XmNactivateCallback,pvInfoDialogCallback,
      (XtPointer)PV_INFO_HELP_BTN);
    XtManageChild(actionArea);
    XtManageChild(pane);
}

static void pvInfoDialogCallback(Widget w, XtPointer cd , XtPointer cbs)
{
    int i = (int)cd;
    switch(i) {
    case  PV_INFO_CLOSE_BTN:
	XtPopdown(pvInfoS);
	break;
    case  PV_INFO_HELP_BTN:
	callBrowser(MEDM_HELP_PATH"#PVInfoDialogBox");
	break;
    }
}

/* Prompts the user with a crosshair cursor for an object in the
 *   display and returns an array of Records associated
 *   with the object and the number of them in count
 * The returned array must be freed by the calling routine
 */
Record **getPvInfoFromDisplay(DisplayInfo *displayInfo, int *count,
  DlElement **ppE)
{
#if((2*MAX_TRACES)+2) > MAX_PENS
#define MAX_COUNT 2*MAX_TRACES+2
#else
#define MAX_COUNT MAX_PENS
#endif
    Widget widget;
    XEvent event;
    Record *records[MAX_COUNT];
    Record **retRecords;
    UpdateTask *pT;
    Position x, y;
    DlElement *pE;
    MedmElement *medmElement;
    int i;

    pE = NULL;
  /* Choose the object with the process variable */
    widget = XmTrackingEvent(displayInfo->drawingArea,
      pvCursor, True, &event);
    XFlush(display);    /* For debugger */
    if(!widget) {
	medmPostMsg(1,"getPvInfoFromDisplay: Choosing object failed\n");
	dmSetAndPopupWarningDialog(displayInfo,
	  "getPvInfoFromDispla: "
	  "Did not find object","OK",NULL,NULL);
	return NULL;
    }

  /* Find the element corresponding to the button press */
    x = event.xbutton.x;
    y = event.xbutton.y;
    *ppE = pE = findSmallestTouchedExecuteElementFromWidget(widget, displayInfo,
      &x, &y, True);
#if DEBUG_PVINFO
    print("getPvInfoFromDisplay: Element: %s\n",elementType(pE->type));
    print("  x: %4d  event.xbutton.x: %4d\n",x,event.xbutton.x);
    print("  y: %4d  event.xbutton.y: %4d\n",y,event.xbutton.y);
#endif    
#if DEBUG_PVLIMITS
    print("\ngetPvInfoFromDisplay: pE = %x\n",pE);
#endif    
    if(!pE) {
	medmPostMsg(1,"getPvInfoFromDisplay: Not on an object\n");
	dmSetAndPopupWarningDialog(displayInfo,
	  "getPvInfoFromDisplay: "
	  "Not on an object","OK",NULL,NULL);
	return NULL;
    }
		
  /* Get the update task */
    pT = getUpdateTaskFromElement(pE);
    if(!pT || !pT->getRecord) {
	medmPostMsg(1,"getPvInfoFromDisplay: "
	  "No process variable associated with object\n");
	return NULL;
    }
  /* Refine the return element.  The former one may have been composite */
    medmElement = (MedmElement *)(pT->clientData);
    if(medmElement) *ppE = medmElement->dlElement;
		
  /* Run the element's updateTask's getRecord procedure */
    pT->getRecord(pT->clientData, records, count);
    (*count)%=100;
    if(*count > MAX_COUNT) {
	medmPostMsg(1,"getPvInfoFromDisplay: Maximum count exceeded\n"
	  "  Programming Error: Please notify person in charge of MEDM\n");
	return NULL;
    }

  /* Allocate the return array */
    if(count) {
	retRecords = (Record **)calloc(*count,sizeof(Record *));
	for(i=0; i < *count; i++) {
	    retRecords[i] = records[i];
	}
    } else {
	retRecords = NULL;
    }
    return retRecords;
#undef MAX_COUNT
}

/*** PV Limits routines ***/

void popupPvLimits(DisplayInfo *displayInfo)
{
    Record **records = NULL;
    DlElement *pE;
    DlLimits *pL;
    char *pvName;
    int count;

  /* Create the dialog box if it has not been created */
    if(!pvLimitsS) {
	createPvLimitsDlg();
	if(!pvLimitsS) {
	    medmPostMsg(1,"popupPvLimits: Cannot create PV Limits dialog box\n");
	    return;
	}
    }
    executeTimePvLimitsElement = NULL;

  /* Branch on EDIT or EXECUTE */
    if(globalDisplayListTraversalMode == DL_EDIT) {
      /* Update the dialog box from the globalResourceBundle */
	pL = &globalResourceBundle.limits;
	resetPvLimitsDlg(pL, PV_LIMITS_EDIT_NAME, True);
    } else {
      /* Let the user pick the widget */
	records = getPvInfoFromDisplay(displayInfo, &count, &pE);
#if DEBUG_PVLIMITS
	print("\npopupPvLimits: pE = %x\n",pE);
#endif    
      /* Don't need the records, free them */
	if(records) {
	    free(records);
	    records = NULL;
	} else {
	    resetPvLimitsDlg(NULL, NULL, True);
	    return;   /* (Error messages have been sent) */
	}
      /* Check if element is valid or if there is a getLimits method */
	executeTimePvLimitsElement = pE;
	pvName = NULL;
	pL = NULL;
	if(pE && pE->run && pE->run->getLimits) {
	    pE->run->getLimits(pE, &pL, &pvName);
	}
	resetPvLimitsDlg(pL, pvName, True);
    }
	
      /* Pop it up */
    XtSetSensitive(pvLimitsS, True);
    XtPopup(pvLimitsS, XtGrabNone);
}

void updatePvLimits(DlLimits *limits)
{
  /* LOPR */
    if(globalDisplayListTraversalMode == DL_EDIT) {
	limits->loprSrc = limits->loprSrc0;
    }
    switch(limits->loprSrc) {
    case PV_LIMITS_CHANNEL:
	limits->lopr = limits->loprChannel;
	break;
    case PV_LIMITS_DEFAULT:
	limits->lopr = limits->loprDefault;
	break;
    case PV_LIMITS_USER:
	limits->lopr = limits->loprUser;
	break;
    }
  /* HOPR */
    if(globalDisplayListTraversalMode == DL_EDIT) {
	limits->hoprSrc = limits->hoprSrc0;
    }
    switch(limits->hoprSrc) {
    case PV_LIMITS_CHANNEL:
	limits->hopr = limits->hoprChannel;
	break;
    case PV_LIMITS_DEFAULT:
	limits->hopr = limits->hoprDefault;
	break;
    case PV_LIMITS_USER:
	limits->hopr = limits->hoprUser;
	break;
    }
  /* PREC */
    if(globalDisplayListTraversalMode == DL_EDIT) {
	limits->precSrc = limits->precSrc0;
    }
    switch(limits->precSrc) {
    case PV_LIMITS_CHANNEL:
	limits->prec = limits->precChannel;
	break;
    case PV_LIMITS_DEFAULT:
	limits->prec = limits->precDefault;
	break;
    case PV_LIMITS_USER:
	limits->prec = limits->precUser;
	break;
    }
}

static void resetPvLimitsDlg(DlLimits *limits, char *pvName, Boolean doName)
{
    char string[1024];     /* Danger: Fixed length */
    XmString xmString;
    
#if DEBUG_PVLIMITS
    print("\nresetPvLimitsDlg: limits=%x  pvName = |%s|\n",
      limits, pvName?pvName:"NULL");
    if(limits) {
	print(" loprSrc=    %10s  hoprSrc=    %10s  precSrc=    %10s\n",
	  stringValueTable[limits->loprSrc],
	  stringValueTable[limits->hoprSrc],
	  stringValueTable[limits->precSrc]);
	print(" lopr=       %10g  hopr=       %10g  prec=       %10hd\n",
	  limits->lopr,limits->hopr,limits->prec);
	print(" loprChannel=%10g  hoprChannel=%10g  precChannel=%10hd\n",
	  limits->loprChannel,limits->hoprChannel,limits->precChannel);
	print(" loprDefault=%10g  hoprDefault=%10g  precDefault=%10hd\n",
	  limits->loprDefault,limits->hoprDefault,limits->precDefault);
	print(" loprUser=   %10g  hoprUser=   %10g  precUser=   %10hd\n",
	  limits->loprUser,limits->hoprUser,limits->precUser);
    }
#endif

  /* Fill in the dialog box parts */
    if(limits) {
      /* PvName */
	if(doName) {
	    if(pvName) {
		xmString = XmStringCreateLocalized(pvName);
	    } else {
		xmString = XmStringCreateLocalized("Unknown");
	    }
	    XtVaSetValues(pvLimitsName,XmNlabelString,xmString,NULL);
	    XmStringFree(xmString);
	}
      /* LOPR */
	switch(limits->loprSrc) {
	case PV_LIMITS_CHANNEL:
	case PV_LIMITS_DEFAULT:
	case PV_LIMITS_USER:
	    if(globalDisplayListTraversalMode == DL_EDIT) {
		if(limits->loprSrc == PV_LIMITS_USER) {
		    limits->loprSrc = PV_LIMITS_DEFAULT;
		    limits->lopr = limits->loprDefault;
		}
	    }
	    XtSetSensitive(pvLimitsLoprSrc, True);
	    optionMenuSet(pvLimitsLoprSrc, limits->loprSrc-FIRST_PV_LIMITS_SRC);
	    XtSetSensitive(XtParent(pvLimitsLopr), True);
	    if(globalDisplayListTraversalMode == DL_EDIT) {	    
		XtVaSetValues(pvLimitsLopr,XmNeditable,
		  (limits->loprSrc == PV_LIMITS_DEFAULT)?True:False,NULL);
	    } else {
		XtVaSetValues(pvLimitsLopr,XmNeditable,
		  (limits->loprSrc == PV_LIMITS_USER)?True:False,NULL);
	    }
	    cvtDoubleToString(limits->lopr, string, limits->prec);
	    XmTextFieldSetString(pvLimitsLopr, string);
	    break;
	case PV_LIMITS_UNUSED:
	    optionMenuSet(pvLimitsLoprSrc, PV_LIMITS_CHANNEL-FIRST_PV_LIMITS_SRC);
	    XtSetSensitive(pvLimitsLoprSrc, False);
	    cvtDoubleToString(limits->loprChannel, string, limits->prec);
	    XmTextFieldSetString(pvLimitsLopr, string);
	    XtSetSensitive(XtParent(pvLimitsLopr), False);
	    break;
	}
      /* HOPR */
	switch(limits->hoprSrc) {
	case PV_LIMITS_CHANNEL:
	case PV_LIMITS_DEFAULT:
	case PV_LIMITS_USER:
	    if(globalDisplayListTraversalMode == DL_EDIT) {
		if(limits->hoprSrc == PV_LIMITS_USER) {
		    limits->hoprSrc = PV_LIMITS_DEFAULT;
		    limits->hopr = limits->hoprDefault;
		}
	    }
	    XtSetSensitive(pvLimitsHoprSrc, True);
	    optionMenuSet(pvLimitsHoprSrc, limits->hoprSrc-FIRST_PV_LIMITS_SRC);
	    XtSetSensitive(XtParent(pvLimitsHopr), True);
	    if(globalDisplayListTraversalMode == DL_EDIT) {	    
		XtVaSetValues(pvLimitsHopr,XmNeditable,
		  (limits->hoprSrc == PV_LIMITS_DEFAULT)?True:False,NULL);
	    } else {
		XtVaSetValues(pvLimitsHopr,XmNeditable,
		  (limits->hoprSrc == PV_LIMITS_USER)?True:False,NULL);
	    }
	    cvtDoubleToString(limits->hopr, string, limits->prec);
	    XmTextFieldSetString(pvLimitsHopr, string);
	    break;
	case PV_LIMITS_UNUSED:
	    optionMenuSet(pvLimitsHoprSrc, PV_LIMITS_CHANNEL-FIRST_PV_LIMITS_SRC);
	    XtSetSensitive(pvLimitsHoprSrc, False);
	    cvtDoubleToString(limits->hoprChannel, string, limits->prec);
	    XmTextFieldSetString(pvLimitsHopr, string);
	    XtSetSensitive(XtParent(pvLimitsHopr), False);
	    break;
	}
      /* PREC */
	switch(limits->precSrc) {
	case PV_LIMITS_CHANNEL:
	case PV_LIMITS_DEFAULT:
	case PV_LIMITS_USER:
	    if(globalDisplayListTraversalMode == DL_EDIT) {
		if(limits->precSrc == PV_LIMITS_USER) {
		    limits->precSrc = PV_LIMITS_DEFAULT;
		    limits->prec = limits->precDefault;
		}
	    }
	    XtSetSensitive(pvLimitsPrecSrc, True);
	    optionMenuSet(pvLimitsPrecSrc, limits->precSrc-FIRST_PV_LIMITS_SRC);
	    XtSetSensitive(XtParent(pvLimitsPrec), True);
	    if(globalDisplayListTraversalMode == DL_EDIT) {	    
		XtVaSetValues(pvLimitsPrec,XmNeditable,
		  (limits->precSrc == PV_LIMITS_DEFAULT)?True:False,NULL);
	    } else {
		XtVaSetValues(pvLimitsPrec,XmNeditable,
		  (limits->precSrc == PV_LIMITS_USER)?True:False,NULL);
	    }
	    cvtLongToString((long)limits->prec, string);
	    XmTextFieldSetString(pvLimitsPrec, string);
	    break;
	case PV_LIMITS_UNUSED:
	    optionMenuSet(pvLimitsPrecSrc, PV_LIMITS_CHANNEL-FIRST_PV_LIMITS_SRC);
	    XtSetSensitive(pvLimitsPrecSrc, False);
	    cvtLongToString((long)limits->precChannel, string);
	    XmTextFieldSetString(pvLimitsPrec, string);
	    XtSetSensitive(XtParent(pvLimitsPrec), False);
	    break;
	}
    } else {
      /* Standard default */
      /* PvName */
	if(doName) {
	    if(pvName) {
		xmString = XmStringCreateLocalized(pvName);
	    } else {
		xmString = XmStringCreateLocalized("Not Available");
	    }
	}
	XtVaSetValues(pvLimitsName,XmNlabelString,xmString,NULL);
	XmStringFree(xmString);
      /* LOPR */
	optionMenuSet(pvLimitsLoprSrc, PV_LIMITS_CHANNEL-FIRST_PV_LIMITS_SRC);
	XtSetSensitive(pvLimitsLoprSrc, False);
	XtSetSensitive(XtParent(pvLimitsLopr), False);
	XmTextFieldSetString(pvLimitsLopr, "");	
      /* HOPR */
	optionMenuSet(pvLimitsHoprSrc, PV_LIMITS_CHANNEL-FIRST_PV_LIMITS_SRC);
	XtSetSensitive(pvLimitsHoprSrc, False);
	XmTextFieldSetString(pvLimitsHopr, "");
	XtSetSensitive(XtParent(pvLimitsHopr), False);
      /* PREC */
	optionMenuSet(pvLimitsPrecSrc, PV_LIMITS_CHANNEL-FIRST_PV_LIMITS_SRC);
	XtSetSensitive(pvLimitsPrecSrc, False);
	XmTextFieldSetString(pvLimitsPrec, "");
	XtSetSensitive(XtParent(pvLimitsPrec), False);
    }
}

static void createPvLimitsDlg(void)
{
    Widget pane, w, wparent, wsave;
    XmString label, opt1, opt2, opt3;

    if(pvLimitsS != NULL) {
	return;
    }

    if(mainShell == NULL) return;

    pvLimitsS = XtVaCreatePopupShell("pvLimitsS",
      topLevelShellWidgetClass, mainShell,
      XmNtitle, "PV Limits",
      XmNdeleteResponse, XmDO_NOTHING,
      NULL);
    XmAddWMProtocolCallback(pvLimitsS,WM_DELETE_WINDOW,
      wmCloseCallback,(XtPointer)OTHER_SHELL);

    pane = XtVaCreateWidget("panel",
      xmPanedWindowWidgetClass, pvLimitsS,
      XmNseparatorOn, True,
      XmNsashWidth, 1,
      XmNsashHeight, 1,
      NULL);

  /* PV name */
    w = XtVaCreateManagedWidget("label",
      xmLabelWidgetClass, pane,
      XmNalignment,XmALIGNMENT_CENTER,
      NULL);
    pvLimitsName = w;

  /* LOPR */
    wparent = XtVaCreateManagedWidget("rowCol",
      xmRowColumnWidgetClass, pane,
      XmNorientation, XmVERTICAL,
      NULL);

    w = XtVaCreateManagedWidget("Low Limit (LOPR)",
      xmLabelWidgetClass, wparent,
      NULL);

    label = XmStringCreateLocalized("Source:");
    opt1 = XmStringCreateLocalized("Channel");
    opt2 = XmStringCreateLocalized("Default");
    opt3 = XmStringCreateLocalized("User Specified");
    w = XmVaCreateSimpleOptionMenu(wparent, "optionMenu",
      label, '\0', 0, pvLimitsDialogCallback,
      XmVaPUSHBUTTON, opt1, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt2, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt3, '\0', NULL, NULL,
      XmNmarginWidth, PV_LIMITS_INDENT,
      XmNuserData, PV_LIMITS_LOPR_SRC_BTN,
      NULL);
    XtManageChild(w);
    XmStringFree(label);
    XmStringFree(opt1);
    XmStringFree(opt2);
    XmStringFree(opt3);
    pvLimitsLoprSrc = w;

    wsave = wparent;
    w = XtVaCreateManagedWidget("rowCol",
      xmRowColumnWidgetClass, wparent,
      XmNorientation, XmHORIZONTAL,
      XmNmarginWidth, PV_LIMITS_INDENT,
      NULL);
    wparent = w;

    w = XtVaCreateManagedWidget("Value:",
      xmLabelWidgetClass, wparent,
      NULL);

    w = XtVaCreateManagedWidget("textField",
      xmTextFieldWidgetClass, wparent,
      XmNcolumns, 20,
      NULL);
    XtAddCallback(w, XmNactivateCallback, pvLimitsDialogCallback,
      (XtPointer)PV_LIMITS_LOPR_BTN);
    XtAddCallback(w, XmNmodifyVerifyCallback, textFieldFloatVerifyCallback,
      NULL);
    XtAddCallback(w, XmNlosingFocusCallback, pvLimitsLosingFocusCallback,
      NULL);
      pvLimitsLopr = w;

  /* HOPR */
    wparent = XtVaCreateManagedWidget("rowCol",
      xmRowColumnWidgetClass, pane,
      XmNorientation, XmVERTICAL,
      NULL);

    w = XtVaCreateManagedWidget("High Limit (HOPR)",
      xmLabelWidgetClass, wparent,
      NULL);

    label = XmStringCreateLocalized("Source:");
    opt1 = XmStringCreateLocalized("Channel");
    opt2 = XmStringCreateLocalized("Default");
    opt3 = XmStringCreateLocalized("User Specified");
    w = XmVaCreateSimpleOptionMenu(wparent, "optionMenu",
      label, '\0', 0, pvLimitsDialogCallback,
      XmVaPUSHBUTTON, opt1, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt2, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt3, '\0', NULL, NULL,
      XmNmarginWidth, PV_LIMITS_INDENT,
      XmNuserData, PV_LIMITS_HOPR_SRC_BTN,
      NULL);
    XtManageChild(w);
    XmStringFree(label);
    XmStringFree(opt1);
    XmStringFree(opt2);
    XmStringFree(opt3);
    pvLimitsHoprSrc = w;

    wsave = wparent;
    w = XtVaCreateManagedWidget("rowCol",
      xmRowColumnWidgetClass, wparent,
      XmNorientation, XmHORIZONTAL,
      XmNmarginWidth, PV_LIMITS_INDENT,
      NULL);
    wparent = w;

    w = XtVaCreateManagedWidget("Value:",
      xmLabelWidgetClass, wparent,
      NULL);

    w = XtVaCreateManagedWidget("textField",
      xmTextFieldWidgetClass, wparent,
      XmNcolumns, 20,
      NULL);
    XtAddCallback(w, XmNactivateCallback, pvLimitsDialogCallback,
      (XtPointer)PV_LIMITS_HOPR_BTN);
    XtAddCallback(w, XmNmodifyVerifyCallback, textFieldFloatVerifyCallback,
      NULL);
    XtAddCallback(w, XmNlosingFocusCallback, pvLimitsLosingFocusCallback,
      NULL);
    pvLimitsHopr = w;

  /* PREC */
    wparent = XtVaCreateManagedWidget("rowCol",
      xmRowColumnWidgetClass, pane,
      XmNorientation, XmVERTICAL,
      NULL);

    w = XtVaCreateManagedWidget("Precision (PREC)",
      xmLabelWidgetClass, wparent,
      NULL);

    label = XmStringCreateLocalized("Source:");
    opt1 = XmStringCreateLocalized("Channel");
    opt2 = XmStringCreateLocalized("Default");
    opt3 = XmStringCreateLocalized("User Specified");
    w = XmVaCreateSimpleOptionMenu(wparent, "optionMenu",
      label, '\0', 0, pvLimitsDialogCallback,
      XmVaPUSHBUTTON, opt1, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt2, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt3, '\0', NULL, NULL,
      XmNmarginWidth, PV_LIMITS_INDENT,
      XmNuserData, PV_LIMITS_PREC_SRC_BTN,
      NULL);
    XtManageChild(w);
    XmStringFree(label);
    XmStringFree(opt1);
    XmStringFree(opt2);
    XmStringFree(opt3);
    pvLimitsPrecSrc = w;

    wsave = wparent;
    w = XtVaCreateManagedWidget("rowCol",
      xmRowColumnWidgetClass, wparent,
      XmNorientation, XmHORIZONTAL,
      XmNmarginWidth, PV_LIMITS_INDENT,
      NULL);
    wparent = w;

    w = XtVaCreateManagedWidget("Value:",
      xmLabelWidgetClass, wparent,
      NULL);

    w = XtVaCreateManagedWidget("textField",
      xmTextFieldWidgetClass, wparent,
      XmNcolumns, 20,
      NULL);
    XtAddCallback(w, XmNactivateCallback, pvLimitsDialogCallback,
      (XtPointer)PV_LIMITS_PREC_BTN);
    XtAddCallback(w, XmNmodifyVerifyCallback, textFieldFloatVerifyCallback,
      NULL);
    XtAddCallback(w, XmNlosingFocusCallback, pvLimitsLosingFocusCallback,
      NULL);
    pvLimitsPrec = w;

  /* Action area */
    wparent = XtVaCreateManagedWidget("actionArea",
      xmFormWidgetClass, pane,
      XmNshadowThickness, 0,
      XmNfractionBase, 5,
      XmNskipAdjust, True,
      NULL);

    w = XtVaCreateManagedWidget("Close",
      xmPushButtonWidgetClass, wparent,
      XmNtopAttachment, XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment, XmATTACH_POSITION,
      XmNleftPosition, 1,
      XmNrightAttachment, XmATTACH_POSITION,
      XmNrightPosition, 2,
      NULL);
    XtAddCallback(w,XmNactivateCallback,pvLimitsDialogCallback,
      (XtPointer)PV_LIMITS_CLOSE_BTN);
    
    w = XtVaCreateManagedWidget("Help",
      xmPushButtonWidgetClass, wparent,
      XmNtopAttachment, XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment, XmATTACH_POSITION,
      XmNleftPosition, 3,
      XmNrightAttachment, XmATTACH_POSITION,
      XmNrightPosition, 4,
      NULL);
    XtAddCallback(w,XmNactivateCallback,pvLimitsDialogCallback,
      (XtPointer)PV_LIMITS_HELP_BTN);

    XtManageChild(pane);
}

static void pvLimitsDialogCallback(Widget w, XtPointer cd , XtPointer cbs)
{
    int type = (int)cd;
    int button, src;
    double val;
    short sval;
    DlElement *pE;
    DlLimits *pL = NULL;
    char *pvName;

#if DEBUG_PVLIMITS
    print("\npvLimitsDialogCallback: type=%d\n",type);
#endif

  /* Get limits pointer */
    if(globalDisplayListTraversalMode == DL_EDIT) {
	pL = &globalResourceBundle.limits;
    } else {
      /* Check if executeTimePvLimitsElement is still valid
       *   It should be, but be safe */
	if(executeTimePvLimitsElement == NULL) {
	    medmPostMsg(1,"pvLimitsDialogCallback: Element is no longer valid\n");
	    XtPopdown(pvLimitsS);
	    return;
	}
	pE = executeTimePvLimitsElement;
	if(pE && pE->run && pE->run->getLimits) {
	    pE->run->getLimits(pE, &pL, &pvName);
	    if(!pL) {
		medmPostMsg(1,"pvLimitsDialogCallback: Element does not have limits\n");
		XtPopdown(pvLimitsS);
		return;
	    }
	}
    }
    
  /* If the type is less than 3, the callback comes from an option menu button
   *   Find the real type from the userData of the RC parent of the button */
    if(type < 3) {
	button=type;
	XtVaGetValues(XtParent(w),XmNuserData,&type,NULL);
#if DEBUG_PVLIMITS
	print("  Type is really %d, button is %d\n",type,button);
#endif
    }

    switch(type) {
    case PV_LIMITS_LOPR_SRC_BTN:
	src = (PvLimitsSrc_t)(FIRST_PV_LIMITS_SRC + button);
	if(globalDisplayListTraversalMode == DL_EDIT) {
	    if(button == 2) {
	      /* Don't allow setting user-specified, revert to channel */
		optionMenuSet(pvLimitsLoprSrc, 0);
		src = (PvLimitsSrc_t)(FIRST_PV_LIMITS_SRC + 0);
		XBell(display,50);
	    }
	    pL->loprSrc0 = src;
	}
	pL->loprSrc = src;
	break;
    case PV_LIMITS_HOPR_SRC_BTN:
	src = (PvLimitsSrc_t)(FIRST_PV_LIMITS_SRC + button);
	if(globalDisplayListTraversalMode == DL_EDIT) {
	    if(button == 2) {
	      /* Don't allow setting user-specified, revert to channel */
		optionMenuSet(pvLimitsHoprSrc, 0);
		src = (PvLimitsSrc_t)(FIRST_PV_LIMITS_SRC + 0);
		XBell(display,50);
	    }
	    pL->hoprSrc0 = src;
	}
	pL->hoprSrc = src;
	break;
    case PV_LIMITS_PREC_SRC_BTN:
	src = (PvLimitsSrc_t)(FIRST_PV_LIMITS_SRC + button);
	if(globalDisplayListTraversalMode == DL_EDIT) {
	    if(button == 2) {
	      /* Don't allow setting user-specified, revert to channel */
		optionMenuSet(pvLimitsPrecSrc, 0);
		src = (PvLimitsSrc_t)(FIRST_PV_LIMITS_SRC + 0);
		XBell(display,50);
	    }
	    pL->precSrc0 = src;
	}
	pL->precSrc = src;
	break;
    case PV_LIMITS_LOPR_BTN:
	val = atof(XmTextFieldGetString(pvLimitsLopr));
	src = pL->loprSrc;
	if(globalDisplayListTraversalMode == DL_EDIT) {
	    if(src == PV_LIMITS_DEFAULT) {
		pL->loprDefault = val;
		pL->lopr = val;
	    } else {
		XBell(display,50);
		return;
	    }
	} else {
	    if(src == PV_LIMITS_USER) {
		pL->loprUser = val;
		pL->lopr = val;
	    } else {
		XBell(display,50);
		return;
	    }
	}
	break;
    case PV_LIMITS_HOPR_BTN:
	val = atof(XmTextFieldGetString(pvLimitsHopr));
	src = pL->hoprSrc;
	if(globalDisplayListTraversalMode == DL_EDIT) {
	    if(src == PV_LIMITS_DEFAULT) {
		pL->hoprDefault = val;
		pL->hopr = val;
	    } else {
		XBell(display,50);
		return;
	    }
	} else {
	    if(src == PV_LIMITS_USER) {
		pL->hoprUser = val;
		pL->hopr = val;
	    } else {
		XBell(display,50);
		return;
	    }
	}
	break;
    case PV_LIMITS_PREC_BTN:
	sval = atoi(XmTextFieldGetString(pvLimitsPrec));
	if(sval < 0) {
	    sval = 0;
	    XBell(display,50);
	} else if(sval > 17) {
	    sval = 17;
	    XBell(display,50);
	}
	src = pL->precSrc;
	if(globalDisplayListTraversalMode == DL_EDIT) {
	    if(src == PV_LIMITS_DEFAULT) {
		pL->precDefault = sval;
		pL->prec = sval;
	    } else {
		XBell(display,50);
		return;
	    }
	} else {
	    if(src == PV_LIMITS_USER) {
		pL->precUser = sval;
		pL->prec = sval;
	    } else {
		XBell(display,50);
		return;
	    }
	}
	break;

      /* These are the buttons at the bottom */
    case  PV_LIMITS_CLOSE_BTN:
	executeTimePvLimitsElement = NULL;
	XtPopdown(pvLimitsS);
	return;
    case  PV_LIMITS_HELP_BTN:
	callBrowser(MEDM_HELP_PATH"#PVLimitsDialogBox");
	return;
    }

  /* Reset the dialog box */
    updatePvLimits(pL);
    resetPvLimitsDlg(pL, NULL, False);

  /* Update from globalResourceBundle for EDIT mode
   *   EXECUTE mode already done */
    if(globalDisplayListTraversalMode == DL_EDIT) {
	DisplayInfo *cdi=currentDisplayInfo;
	
	DlElement *dlElement = FirstDlElement(
	  cdi->selectedDlElementList);
	unhighlightSelectedElements();
	while(dlElement) {
	    updateElementFromGlobalResourceBundle(
	      dlElement->structure.element);
	    dlElement = dlElement->next;
	}
	dmTraverseNonWidgetsInDisplayList(cdi);
	highlightSelectedElements();
    } else {
	DisplayInfo *cdi=currentDisplayInfo;

	if(pE && pE->run && pE->run->execute) pE->run->execute(cdi, pE);
    }
}

static void pvLimitsLosingFocusCallback(Widget w, XtPointer cd , XtPointer cbs)
{
    char string[1024];     /* Danger: Fixed length */
    DlElement *pE;
    DlLimits *pL;
    char *pvName;

#if DEBUG_PVLIMITS
    print("\npvLimitsLosingFocusCallback: w=%x pvLimitsLopr=%x "
      "pvLimitsHopr=%x pvLimitsPrec=%x\n",
      pvLimitsLopr,pvLimitsHopr,pvLimitsPrec);
#endif
  /* Get limits pointer */
    if(globalDisplayListTraversalMode == DL_EDIT) {
	pL = &globalResourceBundle.limits;
    } else {
      /* Check if executeTimePvLimitsElement is still valid
       *   It should be, but be safe */
	if(executeTimePvLimitsElement == NULL) {
	    return;
	}
	pE = executeTimePvLimitsElement;
	if(pE && pE->run && pE->run->getLimits) {
	    pE->run->getLimits(pE, &pL, &pvName);
	    if(!pL) {
		return;
	    }
	}
    }

  /* Reset the text entries */
    if(w == pvLimitsLopr) {
	cvtDoubleToString(pL->lopr, string, pL->prec);
	XmTextFieldSetString(pvLimitsLopr, string);
    } else if(w == pvLimitsHopr) {
	cvtDoubleToString(pL->hopr, string, pL->prec);
	XmTextFieldSetString(pvLimitsHopr, string);
    } else if(w == pvLimitsPrec) {
	cvtLongToString((long)pL->prec, string);
	XmTextFieldSetString(pvLimitsPrec, string);
    }
}

/*** Display list dialog routines ***/

void createDisplayListDlg(void)
{
    Widget w, pane, actionArea;
    XmString xmString;
    Arg args[10];
    int nargs;

    if(displayListS) return;
    if(mainShell == NULL) return;
    
    displayListS = XtVaCreatePopupShell("displayListS",
      topLevelShellWidgetClass, mainShell,
      XmNtitle, "Display List",
      XmNdeleteResponse, XmDO_NOTHING,
      NULL);
    XmAddWMProtocolCallback(displayListS,WM_DELETE_WINDOW,
      wmCloseCallback,(XtPointer)OTHER_SHELL);

  /* Create paned window */
    pane = XtVaCreateManagedWidget("panel",
      xmPanedWindowWidgetClass, displayListS,
      XmNsashWidth, 1,
      XmNsashHeight, 1,
      NULL);

  /* Create label 1 */
    xmString = XmStringCreateLocalized("DISPLAYS");    
    w = XtVaCreateManagedWidget("label",
      xmLabelWidgetClass, pane,
      XmNlabelString, xmString,
      XmNskipAdjust, True,
      NULL);
    XmStringFree(xmString);

  /* Create list box 1 */
    nargs = 0;
    XtSetArg(args[nargs], XmNvisibleItemCount,  5); nargs++;
    XtSetArg(args[nargs], XmNskipAdjust, False); nargs++;
    XtSetArg(args[nargs], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE); nargs++;
    XtSetArg(args[nargs], XmNselectionPolicy, XmMULTIPLE_SELECT); nargs++;
    displayListBox1 = XmCreateScrolledList(pane, "displayListBox1",
      args, nargs);
    XtManageChild(displayListBox1);

  /* Create action area 1 */
    actionArea = XtVaCreateManagedWidget("actionArea",
      xmFormWidgetClass, pane,
      XmNshadowThickness, 0,
      XmNfractionBase, 5,
      XmNskipAdjust, True,
      NULL);

    w = XtVaCreateManagedWidget("Raise Display",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     1,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    2,
      NULL);
    XtAddCallback(w, XmNactivateCallback, displayListDlgCb,
      (XtPointer)DISPLAY_LIST_RAISE_BTN);

    w = XtVaCreateManagedWidget("Close Display",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     3,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    4,
      NULL);
    XtAddCallback(w, XmNactivateCallback, displayListDlgCb,
      (XtPointer)DISPLAY_LIST_CLOSE1_BTN);

#if DEBUG_DISPLAY_LIST
  /* Create label 2 */
    xmString = XmStringCreateLocalized("SAVED DISPLAYS");    
    w = XtVaCreateManagedWidget("label",
      xmLabelWidgetClass, pane,
      XmNlabelString, xmString,
      XmNskipAdjust, True,
      NULL);
    XmStringFree(xmString);

  /* Create list box 2 */
    nargs = 0;
    XtSetArg(args[nargs], XmNvisibleItemCount,  5); nargs++;
    XtSetArg(args[nargs], XmNskipAdjust, True); nargs++;
    XtSetArg(args[nargs], XmNlistSizePolicy, XmRESIZE_IF_POSSIBLE); nargs++;
    XtSetArg(args[nargs], XmNselectionPolicy, XmMULTIPLE_SELECT); nargs++;
    displayListBox2 = XmCreateScrolledList(pane, "displayListBox2",
      args, nargs);
    XtManageChild(displayListBox2);

  /* Create dummy action area */
    actionArea = XtVaCreateManagedWidget("actionArea",
      xmFormWidgetClass, pane,
      XmNshadowThickness, 0,
      XmNfractionBase, 7,
      XmNskipAdjust, True,
      NULL);
#endif

#if 0
  /* Create dummy action area */
    actionArea = XtVaCreateManagedWidget("actionArea",
      xmFormWidgetClass, pane,
      XmNshadowThickness, 0,
      XmNfractionBase, 7,
      XmNskipAdjust, True,
      NULL);
#endif    

  /* Create action area */
    actionArea = XtVaCreateManagedWidget("actionArea",
      xmFormWidgetClass, pane,
      XmNshadowThickness, 0,
      XmNfractionBase, 7,
      XmNskipAdjust, True,
      NULL);

    w = XtVaCreateManagedWidget("Close",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     1,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    2,
      NULL);
    XtAddCallback(w, XmNactivateCallback, displayListDlgCb,
      (XtPointer)DISPLAY_LIST_CLOSE_BTN);

    w = XtVaCreateManagedWidget("Refresh",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     3,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    4,
      NULL);
    XtAddCallback(w, XmNactivateCallback, displayListDlgCb,
      (XtPointer)DISPLAY_LIST_REFRESH_BTN);

    w = XtVaCreateManagedWidget("Help",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     5,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    6,
      NULL);
    XtAddCallback(w, XmNactivateCallback, displayListDlgCb,
      (XtPointer)DISPLAY_LIST_HELP_BTN);

  /* Initialize */
    refreshDisplayListDlg();
}

void refreshDisplayListDlg(void)
{
#define MAX_LENGTH 256
    DisplayInfo *di;
    XmString xmString;
    int i, len;
    char string[MAX_LENGTH];     /* Danger fixed length */

    if(!displayListS) return;
    
  /* Display list */
    XmListDeleteAllItems(displayListBox1);
    di = displayInfoListHead->next;
    while(di) {
	strcpy(string,di->dlFile->name);
	for(i=0; i < di->numNameValues; i++) {
	    len = strlen(string) +
	      strlen(di->nameValueTable[i].name) +
	      strlen(di->nameValueTable[i].value) + 2;
	    if(len >= MAX_LENGTH) break;
	    sprintf(string,"%s %s=%s", string,
	      di->nameValueTable[i].name,  di->nameValueTable[i].value);
	}
	xmString = XmStringCreateLocalized(string);
	XmListAddItemUnselected(displayListBox1, xmString, 0);
	XmStringFree(xmString);
	di = di->next;
    }	

#if DEBUG_DISPLAY_LIST
  /* Display save list */
    XmListDeleteAllItems(displayListBox2);
    di = displayInfoSaveListHead->next;
    while(di) {
	strcpy(string,di->dlFile->name);
	for(i=0; i < di->numNameValues; i++) {
	    len = strlen(string) +
	      strlen(di->nameValueTable[i].name) +
	      strlen(di->nameValueTable[i].value) + 2;
	    if(len >= MAX_LENGTH) break;
	    sprintf(string,"%s %s=%s", string,
	      di->nameValueTable[i].name,  di->nameValueTable[i].value);
	}
	xmString = XmStringCreateLocalized(string);
	XmListAddItemUnselected(displayListBox2, xmString, 0);
	XmStringFree(xmString);
	di = di->next;
    }	
#endif
#undef MAX_LENGTH
}

void popupDisplayListDlg(void)
{
  /* Create the dialog box if it has not been created */
    if(!displayListS) {
	createDisplayListDlg();
    }

  /* Refresh and pop it up */
    if(displayListS) {XtPopup(displayListS,XtGrabNone);
	refreshDisplayListDlg();
	XtPopup(displayListS,XtGrabNone);
    }
}

static void displayListDlgCb(Widget w, XtPointer clientData,
  XtPointer callData)
{
    int button = (int)clientData;
    DisplayInfo *di, *diNext;
    Boolean status;
    int *positionList = NULL;
    int i, j, positionCount;    

    switch(button) {
    case DISPLAY_LIST_RAISE_BTN:
    case DISPLAY_LIST_CLOSE1_BTN:
	status = XmListGetSelectedPos(displayListBox1, &positionList,
	  &positionCount);
	if(status) {
	    di = displayInfoListHead->next;
	    i = 1;
	    while(di) {
		diNext = di->next;
		for(j=0; j < positionCount; j++) {
		    if(i == positionList[j]) {
		      /* Item is selected */
			switch(button) {
			case DISPLAY_LIST_RAISE_BTN:
			  /* Make sure it changes workspaces, too */
			    XtPopdown(di->shell);
			    XtPopup(di->shell, XtGrabNone);
			    break;
			case DISPLAY_LIST_CLOSE1_BTN:
			  /* KE: Use closeDisplay instead of
			   *   dmRemoveDisplayInfo(di) same as for menu items */
			    closeDisplay(di->shell);
			    break;
			}
		    }
		}
		di = diNext; i++;
	    }
	    if(positionList) XFree(positionList);
	    refreshDisplayListDlg();
	} else {
	    XBell(display,50); XBell(display,50); XBell(display,50);	    
	}
	break;
    case DISPLAY_LIST_CLOSE_BTN:
	if(displayListS != NULL) {
	    XtPopdown(displayListS);
	}
	break;
    case DISPLAY_LIST_REFRESH_BTN:
	refreshDisplayListDlg();
	break;
    case DISPLAY_LIST_HELP_BTN:
	callBrowser(MEDM_HELP_PATH"#DisplayListDialogBox");
	break;
    }
}

/*** Print Setup ***/

void popupPrintSetup(void)
{
  /* Create the dialog box if it has not been created */
    if(!printSetupS) {
	createPrintSetupDlg();
	if(!printSetupS) {
	    medmPostMsg(1,"popupPrintSetup: Cannot create PV Limits dialog box\n");
	    return;
	}
    }

  /* Pop it up */
    XtSetSensitive(printSetupS, True);
    XtPopup(printSetupS, XtGrabNone);
}

void createPrintSetupDlg(void)
{
    Widget w, wparent, wsave;
    Widget pane, control, hw;
    Widget actionArea;
    Widget okButton, helpButton, cancelButton;
    XmString label, opt1, opt2;
    char string[80];

    if(printSetupS != NULL) return;

    if(mainShell == NULL) return;

    printSetupS = XtVaCreatePopupShell("printSetupS",
      topLevelShellWidgetClass, mainShell,
      XmNtitle, "Print Setup",
      XmNdeleteResponse, XmDO_NOTHING,
      NULL);
    XmAddWMProtocolCallback(printSetupS,WM_DELETE_WINDOW,
      wmCloseCallback,(XtPointer)OTHER_SHELL);

    pane = XtVaCreateWidget("panel",
      xmPanedWindowWidgetClass, printSetupS,
      XmNsashWidth, 1,
      XmNsashHeight, 1,
      NULL);

  /* Vertical row column to hold controls */
    control = XtVaCreateManagedWidget("rowCol",
      xmRowColumnWidgetClass, pane,
      XmNorientation, XmVERTICAL,
      NULL);

  /* Print command */
    wparent = control;
    w = XtVaCreateManagedWidget("Print Command:",
      xmLabelWidgetClass, wparent,
      NULL);

    w = XtVaCreateManagedWidget("textField",
      xmTextFieldWidgetClass, wparent,
      XmNvalue, printCommand,
      XmNmaxLength, PRINT_BUF_SIZE-1,
      XmNcolumns, 40,
      NULL);
    XtAddCallback(w, XmNactivateCallback, printSetupDialogCallback,
      (XtPointer)PRINT_SETUP_CMD_FIELD);
    printSetupCommandTF = w;

  /* Orientation */
    label = XmStringCreateLocalized("Orientation:");
    opt1 = XmStringCreateLocalized(printerOrientationTable[0]);
    opt2 = XmStringCreateLocalized(printerOrientationTable[1]);
    w = XmVaCreateSimpleOptionMenu(wparent, "optionMenu",
      label, '\0', printOrientation, printSetupDialogCallback,
      XmVaPUSHBUTTON, opt1, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt2, '\0', NULL, NULL,
      XmNuserData, PRINT_SETUP_ORIENT_BTN,
      NULL);
    XtManageChild(w);
    XmStringFree(label);
    XmStringFree(opt1);
    XmStringFree(opt2);

  /* Height and width */
    hw = XtVaCreateManagedWidget("rowCol",
      xmRowColumnWidgetClass, wparent,
      XmNorientation, XmHORIZONTAL,
      NULL);
    wparent = hw;

  /* Width */
    wsave = wparent;
    w = XtVaCreateManagedWidget("rowCol",
      xmRowColumnWidgetClass, wparent,
      XmNorientation, XmHORIZONTAL,
      NULL);
    wparent = w;

    w = XtVaCreateManagedWidget("Width:",
      xmLabelWidgetClass, wparent,
      NULL);

    sprintf(string, "%.2f", printWidth);
    w = XtVaCreateManagedWidget("textField",
      xmTextFieldWidgetClass, wparent,
      XmNvalue, string,
      XmNmaxLength, 15,
      XmNcolumns, 15,
      NULL);
    XtAddCallback(w, XmNactivateCallback, printSetupDialogCallback,
      (XtPointer)PRINT_SETUP_WIDTH_FIELD);
    XtAddCallback(w, XmNmodifyVerifyCallback,
      textFieldFloatVerifyCallback,(XtPointer)NULL);
    printSetupWidthTF = w;

  /* Height */
    wparent = hw;
    w = XtVaCreateManagedWidget("rowCol",
      xmRowColumnWidgetClass, wparent,
      XmNorientation, XmHORIZONTAL,
      NULL);
    wparent = w;

    w = XtVaCreateManagedWidget("Height:",
      xmLabelWidgetClass, wparent,
      NULL);

    sprintf(string, "%.2f", printHeight);
    w = XtVaCreateManagedWidget("textField",
      xmTextFieldWidgetClass, wparent,
      XmNvalue, string,
      XmNmaxLength, 15,
      XmNcolumns, 15,
      NULL);
    XtAddCallback(w, XmNactivateCallback, printSetupDialogCallback,
      (XtPointer)PRINT_SETUP_HEIGHT_FIELD);
    XtAddCallback(w, XmNmodifyVerifyCallback,
      textFieldFloatVerifyCallback,(XtPointer)NULL);
    printSetupHeightTF = w;

  /* File name */
    wparent = control;
    w = XtVaCreateManagedWidget("File Name:",
      xmLabelWidgetClass, wparent,
      NULL);

    w = XtVaCreateManagedWidget("textField",
      xmTextFieldWidgetClass, wparent,
      XmNvalue, printFile,
      XmNmaxLength, PRINT_BUF_SIZE-1,
      XmNcolumns, 40,
      NULL);
    XtAddCallback(w, XmNactivateCallback, printSetupDialogCallback,
      (XtPointer)PRINT_SETUP_FILENAME_FIELD);
    printSetupFileTF = w;

  /* Print to file */
    wparent = control;
    label = XmStringCreateLocalized(
      "Print to File");
    w =  XtVaCreateManagedWidget("toggleButton",
      xmToggleButtonWidgetClass, wparent,
      XmNlabelString, label,
      XmNset, (Boolean)printToFile,
      NULL);
    XmStringFree(label);
    XtAddCallback(w, XmNvalueChangedCallback, printSetupDialogCallback,
      (XtPointer)PRINT_SETUP_TOFILE_BTN);
    printSetupPrintToFileTB = w;

  /* Action area */
    actionArea = XtVaCreateWidget("actionArea",
      xmFormWidgetClass, pane,
      XmNshadowThickness, 0,
      XmNfractionBase, 7,
      XmNskipAdjust, True,
      NULL);

    okButton = XtVaCreateManagedWidget("OK",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     1,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    2,
#if 0      
      XmNshowAsDefault,    True,
      XmNdefaultButtonShadowThickness, 1,
#endif      
      NULL);
    XtAddCallback(okButton,XmNactivateCallback,printSetupDialogCallback,
      (XtPointer)PRINT_SETUP_OK_BTN);
    
    cancelButton = XtVaCreateManagedWidget("Cancel",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     3,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    4,
      NULL);
    XtAddCallback(cancelButton,XmNactivateCallback,printSetupDialogCallback,
      (XtPointer)PRINT_SETUP_CANCEL_BTN);

    helpButton = XtVaCreateManagedWidget("Help",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     5,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    6,
      NULL);
    XtAddCallback(helpButton,XmNactivateCallback,printSetupDialogCallback,
      (XtPointer)PRINT_SETUP_HELP_BTN);
    XtManageChild(actionArea);
    XtManageChild(pane);
}

static void printSetupDialogCallback(Widget w, XtPointer cd , XtPointer cbs)
{
    int type = (int)cd;
    int button;
    double fval;
    
  /* If the type is less than 4, the callback comes from an option
   *   menu button.  Find the real type from the userData of the RC
   *   parent of the button */
    if(type < 4) {
	button=type;
	XtVaGetValues(XtParent(w), XmNuserData, &type, NULL);
    }

    switch(type) {
    case PRINT_SETUP_ORIENT_BTN:
	switch(button) {
	case 0:
	  /* Option button for portrait */
	    printOrientation = PRINT_PORTRAIT;
	    break;
	case 1:
	  /* Option button for landscape */
	    printOrientation = PRINT_LANDSCAPE;
	    break;
	}
	break;
    case PRINT_SETUP_OK_BTN:
	XtPopdown(printSetupS);
	break;
    case PRINT_SETUP_CANCEL_BTN:
	XtPopdown(printSetupS);
	break;
    case PRINT_SETUP_HELP_BTN:
	callBrowser(MEDM_HELP_PATH"#PrintSetupDialogBox");
	break;
    case PRINT_SETUP_CMD_FIELD:
	sprintf(printCommand, XmTextFieldGetString(printSetupCommandTF));
	XmTextFieldSetCursorPosition(printSetupCommandTF, 0);
	break;
    case PRINT_SETUP_WIDTH_FIELD:
	fval = atoi(XmTextFieldGetString(printSetupWidthTF));
	if(fval < 0.0) {
	    fval = 0;
	    XmTextFieldSetString(printSetupWidthTF, "0.00");
	    XBell(display,50);
	}
	XmTextFieldSetCursorPosition(printSetupWidthTF, 0);
	printWidth = fval;
	break;
    case PRINT_SETUP_HEIGHT_FIELD:
	fval = atoi(XmTextFieldGetString(printSetupHeightTF));
	if(fval < 0.0) {
	    fval = 0;
	    XmTextFieldSetString(printSetupHeightTF, "0.00");
	    XBell(display,50);
	}
	XmTextFieldSetCursorPosition(printSetupHeightTF, 0);
	printHeight = fval;
	break;
    case PRINT_SETUP_FILENAME_FIELD:
	sprintf(printFile, XmTextFieldGetString(printSetupFileTF));
	XmTextFieldSetCursorPosition(printSetupFileTF, 0);
	break;
    case PRINT_SETUP_TOFILE_BTN:
	printToFile = XmToggleButtonGetState(printSetupPrintToFileTB)?1:0;
	break;
    }
}
