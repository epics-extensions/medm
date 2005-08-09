/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
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
#define PRINT_SETUP_PRINT_BTN      6
#define PRINT_SETUP_HELP_BTN       7
#define PRINT_SETUP_CMD_FIELD      8
#define PRINT_SETUP_ORIENT_BTN     9
#define PRINT_SETUP_SIZE_BTN       10
#define PRINT_SETUP_TITLE_BTN      11
#define PRINT_SETUP_TITLE_FIELD    12
#define PRINT_SETUP_WIDTH_FIELD    13
#define PRINT_SETUP_HEIGHT_FIELD   14
#define PRINT_SETUP_FILENAME_FIELD 15
#define PRINT_SETUP_TOFILE_BTN     16
#define PRINT_SETUP_DATE_BTN       17
#define PRINT_SETUP_TIME_BTN       18

#ifdef WIN32
/* For getpid() */
#include <process.h>
#endif
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
static void updatePrintSetupFromDialog();
static void updatePrintSetupDlg();

/* Global variables */
static Widget displayListBox1 = NULL;
#if DEBUG_DISPLAY_LIST
static Widget displayListBox2 = NULL;
#endif
static Widget pvLimitsName, pvLimitsLopr, pvLimitsHopr, pvLimitsPrec;
static Widget pvLimitsLoprSrc, pvLimitsHoprSrc, pvLimitsPrecSrc;
static Widget printSetupCommandTF, printSetupFileTF, printSetupTitleTF;
static Widget printSetupWidthTF, printSetupHeightTF, printSetupPrintToFileTB;
static Widget printSetupPrintToFileTB;
static Widget printSetupPrintDateTB, printSetupPrintTimeTB;
static Widget printSetupOrientationMenu, printSetupSizeMenu, printSetupTitleMenu;
static int printSetupModified;

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
	callBrowser(medmHelpPath,"#PVInfoDialogBox");
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
    int i;

    pE = NULL;
  /* Choose the object with the process variable */
    widget = XmTrackingEvent(displayInfo->drawingArea,
      pvCursor, True, &event);
    XFlush(display);    /* For debugger */
    if(!widget) {
	medmPostMsg(1,"getPvInfoFromDisplay: Choosing object failed\n");
	dmSetAndPopupWarningDialog(displayInfo,
	  "getPvInfoFromDisplay: "
	  "Did not find object","OK",NULL,NULL);
	return NULL;
    }

  /* Find the element corresponding to the button press */
    x = event.xbutton.x;
    y = event.xbutton.y;
    *ppE = pE = findSmallestTouchedExecuteElement(widget, displayInfo,
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
	medmPostMsg(1,"getPvInfoFromDisplay: Not on an object with a PV\n");
	dmSetAndPopupWarningDialog(displayInfo,
	  "getPvInfoFromDisplay: "
	  "Not on an object with a PV", "OK", NULL, NULL);
	return NULL;
    }

  /* Get the update task */
    pT = getUpdateTaskFromElement(pE);
    if(!pT || !pT->getRecord) {
	medmPostMsg(1,"getPvInfoFromDisplay: "
	  "No process variable associated with object\n");
	return NULL;
    }
#if 0
  /* KE: Shouldn't happen now */
  /* Refine the return element.  The former one may have been composite */
    medmElement = (MedmElement *)(pT->clientData);
    if(medmElement) *ppE = medmElement->dlElement;
#endif

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

/* Creates the dialog if needed, fills in the values, and pops it up */
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
      /* Do the Strip Chart differently */
	if(pE->type == DL_StripChart) {
	    executeTimeStripChartElement = pE;
	    popupStripChartDataDialog();
	    return;
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

/* Resets members of the limits struct argument.  Only affects the
   limits struct. */
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

/* Fills in the dialog from values in limits and pvName arguments */
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

/* Creates pvLimitsS */
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
#if OMIT_RESIZE_HANDLES
      XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH,
    /* KE: The following is necessary for Exceed, which turns off the
       resize function with the handles.  It should not be necessary */
      XmNmwmFunctions, MWM_FUNC_ALL,
#endif
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

/* Sets the values in the globalResourceBundle or the
   executeTimePvLimitsElement when controls in the dialog are
   activated */
static void pvLimitsDialogCallback(Widget w, XtPointer cd , XtPointer cbs)
{
    int type = (int)cd;
    int button, src;
    double val;
    short sval;
    DlElement *pE;
    DlLimits *pL = NULL;
    char *pvName;
    char *string;

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
	XtVaGetValues(XtParent(w), XmNuserData, &type,NULL);
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
	string = XmTextFieldGetString(pvLimitsLopr);
	val = atof(string);
	XtFree(string);
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
	string = XmTextFieldGetString(pvLimitsHopr);
	val = atof(string);
	XtFree(string);
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
	string =XmTextFieldGetString(pvLimitsPrec);
	sval = atoi(string);
	XtFree(string);
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
	callBrowser(medmHelpPath,"#PVLimitsDialogBox");
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

	if(pE) {
	    if(pE->updateType == DYNAMIC_GRAPHIC) {
	      /* The textUpdate is the only case -- make it update */
		UpdateTask *pT=getUpdateTaskFromElement(pE);

		if(pT) updateTaskMarkUpdate(pT);
	    } else {
	      /* It should be a widget -- execute it */
		if(pE->run && pE->run->execute) pE->run->execute(cdi, pE);
	    }
	}
    }
}

/* Resets the text entries from the globalResourceBundle or the
   executeTimePvLimitsElement */
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
    char string[80];
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
    sprintf(string,"DISPLAYS (MEDM PID: %d Window ID: 0x%x)",
      getpid(),(unsigned int)XtWindow(mainShell));
    xmString = XmStringCreateLocalized(string);
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
    if(displayListS) {
	refreshDisplayListDlg();
	XtSetSensitive(displayListS, True);
      /* Make sure it comes up on the current workspace */
      /* KE: This appears to work and deiconify if iconic */
	if(XtIsRealized(displayListS)) {
	    XMapRaised(display, XtWindow(displayListS));
	}
      /* Pop it up */
	XtPopup(displayListS, XtGrabNone);
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
#if 1
			    if(di && di->shell && XtIsRealized(di->shell)) {
				XMapRaised(display, XtWindow(di->shell));
			    }
#else
			  /* KE: Doesn't work on WIN32 */
			    XtPopdown(di->shell);
			    XtPopup(di->shell, XtGrabNone);
#endif
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
	callBrowser(medmHelpPath,"#DisplayListDialogBox");
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
    updatePrintSetupDlg();

  /* Pop it up */
    XtSetSensitive(printSetupS, True);
    XtPopup(printSetupS, XtGrabNone);
}

void createPrintSetupDlg(void)
{
    Widget w, wparent;
    Widget pane, control;
    Widget actionArea, containerRC0, containerRC1;
    Widget okButton, helpButton, cancelButton, printButton;
    XmString label, opt1, opt2, opt3, opt4;
    char string[80];

    if(printSetupS != NULL) return;

    if(mainShell == NULL) return;

    printSetupS = XtVaCreatePopupShell("printSetupS",
      topLevelShellWidgetClass, mainShell,
      XmNtitle, "Print Setup",
      XmNdeleteResponse, XmDO_NOTHING,
#if OMIT_RESIZE_HANDLES
      XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH,
    /* KE: The following is necessary for Exceed, which turns off the
       resize function with the handles.  It should not be necessary */
      XmNmwmFunctions, MWM_FUNC_ALL,
#endif
      NULL);
    XmAddWMProtocolCallback(printSetupS,WM_DELETE_WINDOW,
      wmCloseCallback,(XtPointer)OTHER_SHELL);

  /* Paned window for everything */
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
    wparent = control;
    label = XmStringCreateLocalized("Orientation:");
    opt1 = XmStringCreateLocalized(printerOrientationTable[PRINT_PORTRAIT]);
    opt2 = XmStringCreateLocalized(printerOrientationTable[PRINT_LANDSCAPE]);
    w = XmVaCreateSimpleOptionMenu(wparent, "optionMenu",
      label, '\0', printOrientation, printSetupDialogCallback,
      XmVaPUSHBUTTON, opt1, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt2, '\0', NULL, NULL,
      XmNuserData, PRINT_SETUP_ORIENT_BTN,
      NULL);
    XtManageChild(w);
    printSetupOrientationMenu = w;
    XmStringFree(label);
    XmStringFree(opt1);
    XmStringFree(opt2);

  /* Size */
    wparent = control;
    label = XmStringCreateLocalized("Paper Size:");
    opt1 = XmStringCreateLocalized(printerSizeTable[PRINT_A]);
    opt2 = XmStringCreateLocalized(printerSizeTable[PRINT_B]);
    opt3 = XmStringCreateLocalized(printerSizeTable[PRINT_A3]);
    opt4 = XmStringCreateLocalized(printerSizeTable[PRINT_A4]);
    w = XmVaCreateSimpleOptionMenu(wparent, "optionMenu",
      label, '\0', printOrientation, printSetupDialogCallback,
      XmVaPUSHBUTTON, opt1, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt2, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt3, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt4, '\0', NULL, NULL,
      XmNuserData, PRINT_SETUP_SIZE_BTN,
      NULL);
    XtManageChild(w);
    printSetupSizeMenu = w;
    XmStringFree(label);
    XmStringFree(opt1);
    XmStringFree(opt2);
    XmStringFree(opt3);
    XmStringFree(opt4);

  /* Title */
    wparent = control;
    label = XmStringCreateLocalized("Title Option:");
    opt1 = XmStringCreateLocalized(printerTitleTable[PRINT_TITLE_NONE]);
    opt2 = XmStringCreateLocalized(printerTitleTable[PRINT_TITLE_SHORT_NAME]);
    opt3 = XmStringCreateLocalized(printerTitleTable[PRINT_TITLE_LONG_NAME]);
    opt4 = XmStringCreateLocalized(printerTitleTable[PRINT_TITLE_SPECIFIED]);
    w = XmVaCreateSimpleOptionMenu(wparent, "optionMenu",
      label, '\0', printOrientation, printSetupDialogCallback,
      XmVaPUSHBUTTON, opt1, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt2, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt3, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt4, '\0', NULL, NULL,
      XmNuserData, PRINT_SETUP_TITLE_BTN,
      NULL);
    XtManageChild(w);
    printSetupTitleMenu = w;
    XmStringFree(label);
    XmStringFree(opt1);
    XmStringFree(opt2);
    XmStringFree(opt3);
    XmStringFree(opt4);

  /* Row column to hold height and width */
    wparent = control;
    containerRC0 = XtVaCreateManagedWidget("rowCol",
      xmRowColumnWidgetClass, wparent,
      XmNorientation, XmHORIZONTAL,
      NULL);

  /* Width */
    wparent = containerRC0;
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
    wparent = containerRC0;
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

  /* Row column to hold toggle buttons */
    wparent = control;
    containerRC1 = XtVaCreateManagedWidget("rowCol",
      xmRowColumnWidgetClass, wparent,
      XmNorientation, XmHORIZONTAL,
      NULL);

  /* Print date */
    wparent = containerRC1;
    label = XmStringCreateLocalized(
      "Print Date");
    w = XtVaCreateManagedWidget("toggleButton",
      xmToggleButtonWidgetClass, wparent,
      XmNlabelString, label,
      XmNset, (Boolean)printDate,
      NULL);
    XmStringFree(label);
    XtAddCallback(w, XmNvalueChangedCallback, printSetupDialogCallback,
      (XtPointer)PRINT_SETUP_DATE_BTN);
    printSetupPrintDateTB = w;

  /* Print time */
    wparent = containerRC1;
    label = XmStringCreateLocalized(
      "Print Time");
    w =  XtVaCreateManagedWidget("toggleButton",
      xmToggleButtonWidgetClass, wparent,
      XmNlabelString, label,
      XmNset, (Boolean)printTime,
      NULL);
    XmStringFree(label);
    XtAddCallback(w, XmNvalueChangedCallback, printSetupDialogCallback,
      (XtPointer)PRINT_SETUP_TIME_BTN);
    printSetupPrintTimeTB = w;

  /* Print to file */
    wparent = containerRC1;
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

  /* Title string */
    wparent = control;
    w = XtVaCreateManagedWidget("Title:",
      xmLabelWidgetClass, wparent,
      NULL);

    w = XtVaCreateManagedWidget("textField",
      xmTextFieldWidgetClass, wparent,
      XmNvalue, printTitleString,
      XmNmaxLength, PRINT_BUF_SIZE-1,
      XmNcolumns, 40,
      NULL);
    XtAddCallback(w, XmNactivateCallback, printSetupDialogCallback,
      (XtPointer)PRINT_SETUP_TITLE_FIELD);
    printSetupTitleTF = w;

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

  /* Action area */
    actionArea = XtVaCreateWidget("actionArea",
      xmFormWidgetClass, pane,
      XmNshadowThickness, 0,
      XmNfractionBase, 9,
      XmNskipAdjust, True,
      NULL);

    okButton = XtVaCreateManagedWidget("Apply",
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

    printButton = XtVaCreateManagedWidget("Print",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     5,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    6,
      NULL);
    XtAddCallback(printButton,XmNactivateCallback,printSetupDialogCallback,
      (XtPointer)PRINT_SETUP_PRINT_BTN);

    helpButton = XtVaCreateManagedWidget("Help",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     7,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    8,
      NULL);
    XtAddCallback(helpButton,XmNactivateCallback,printSetupDialogCallback,
      (XtPointer)PRINT_SETUP_HELP_BTN);

    XtManageChild(actionArea);
    XtManageChild(pane);

    printSetupModified=FALSE;
}

static void updatePrintSetupDlg()
{
    char string[80];
    Widget menuWidget;
    WidgetList children;
    Cardinal numChildren;

  /* Command */
    XmTextFieldSetString(printSetupCommandTF,printCommand);
    XmTextFieldSetCursorPosition(printSetupCommandTF, 0);

  /* Orientation */
    XtVaGetValues(printSetupOrientationMenu,
      XmNsubMenuId,&menuWidget,
      NULL);
    XtVaGetValues(menuWidget,
      XmNchildren,&children,
      XmNnumChildren,&numChildren,
      NULL);
    switch(printOrientation) {
    case PRINT_PORTRAIT:
	XtVaSetValues(printSetupOrientationMenu,
	  XmNmenuHistory, children[PRINT_PORTRAIT],
	  NULL);
	break;
    case PRINT_LANDSCAPE:
	XtVaSetValues(printSetupOrientationMenu,
	  XmNmenuHistory, children[PRINT_LANDSCAPE],
	  NULL);
	break;
    default:
	XtVaSetValues(printSetupOrientationMenu,
	  XmNmenuHistory, children[PRINT_PORTRAIT],
	  NULL);
	break;
    }

  /* Size */
    XtVaGetValues(printSetupSizeMenu,
      XmNsubMenuId,&menuWidget,
      NULL);
    XtVaGetValues(menuWidget,
      XmNchildren,&children,
      XmNnumChildren,&numChildren,
      NULL);
    switch(printSize) {
    case PRINT_A:
	XtVaSetValues(printSetupSizeMenu,
	  XmNmenuHistory, children[PRINT_A],
	  NULL);
	break;
    case PRINT_B:
	XtVaSetValues(printSetupSizeMenu,
	  XmNmenuHistory, children[PRINT_B],
	  NULL);
	break;
    case PRINT_A3:
	XtVaSetValues(printSetupSizeMenu,
	  XmNmenuHistory, children[PRINT_A3],
	  NULL);
	break;
    case PRINT_A4:
	XtVaSetValues(printSetupSizeMenu,
	  XmNmenuHistory, children[PRINT_A4],
	  NULL);
	break;
    default:
	XtVaSetValues(printSetupSizeMenu,
	  XmNmenuHistory, children[PRINT_A],
	  NULL);
	break;
    }

  /* Title */
    XtVaGetValues(printSetupTitleMenu,
      XmNsubMenuId,&menuWidget,
      NULL);
    XtVaGetValues(menuWidget,
      XmNchildren,&children,
      XmNnumChildren,&numChildren,
      NULL);
    switch(printTitle) {
    case PRINT_TITLE_NONE:
	XtVaSetValues(printSetupTitleMenu,
	  XmNmenuHistory, children[PRINT_TITLE_NONE],
	  NULL);
	break;
    case PRINT_TITLE_SHORT_NAME:
	XtVaSetValues(printSetupTitleMenu,
	  XmNmenuHistory, children[PRINT_TITLE_SHORT_NAME],
	  NULL);
	break;
    case PRINT_TITLE_LONG_NAME:
	XtVaSetValues(printSetupTitleMenu,
	  XmNmenuHistory, children[PRINT_TITLE_LONG_NAME],
	  NULL);
	break;
    case PRINT_TITLE_SPECIFIED:
	XtVaSetValues(printSetupTitleMenu,
	  XmNmenuHistory, children[PRINT_TITLE_SPECIFIED],
	  NULL);
	break;
    default:
	XtVaSetValues(printSetupTitleMenu,
	  XmNmenuHistory, children[PRINT_TITLE_LONG_NAME],
	  NULL);
	break;
    }

  /* Width */
    sprintf(string, "%.2f", printWidth);
    XmTextFieldSetString(printSetupWidthTF, string);
    XmTextFieldSetCursorPosition(printSetupWidthTF, 0);

  /* Height */
    sprintf(string, "%.2f", printHeight);
    XmTextFieldSetString(printSetupHeightTF, string);
    XmTextFieldSetCursorPosition(printSetupHeightTF, 0);

  /* Title string */
    XmTextFieldSetString(printSetupFileTF, printTitleString);
    XmTextFieldSetCursorPosition(printSetupTitleTF, 0);

  /* Filename */
    XmTextFieldSetString(printSetupFileTF, printFile);
    XmTextFieldSetCursorPosition(printSetupFileTF, 0);

  /* To file */
    XmToggleButtonSetState(printSetupPrintToFileTB,(Boolean)printToFile,False);

  /* Date */
    XmToggleButtonSetState(printSetupPrintDateTB,(Boolean)printDate,False);

  /* Time */
    XmToggleButtonSetState(printSetupPrintTimeTB,(Boolean)printTime,False);

    printSetupModified = FALSE;
}

static void updatePrintSetupFromDialog()
{
    Widget menuWidget;
    Widget menuHistory;
    WidgetList children;
    Cardinal numChildren;
    double fval;
    char *string;

  /* Command */
    string = XmTextFieldGetString(printSetupCommandTF);
    sprintf(printCommand, string);
    XtFree(string);
    XmTextFieldSetCursorPosition(printSetupCommandTF, 0);

  /* Orientation */
    XtVaGetValues(printSetupOrientationMenu,
      XmNsubMenuId,&menuWidget,
      XmNmenuHistory,&menuHistory,
      NULL);
    XtVaGetValues(menuWidget,
      XmNchildren,&children,
      XmNnumChildren,&numChildren,
      NULL);
    if(menuHistory == children[PRINT_PORTRAIT]) {
	printOrientation = PRINT_PORTRAIT;
    } else if(menuHistory == children[PRINT_LANDSCAPE]) {
	printOrientation = PRINT_LANDSCAPE;
    } else {
	printOrientation = PRINT_PORTRAIT;
    }

  /* Size */
    XtVaGetValues(printSetupSizeMenu,
      XmNsubMenuId,&menuWidget,
      XmNmenuHistory,&menuHistory,
      NULL);
    XtVaGetValues(menuWidget,
      XmNchildren,&children,
      XmNnumChildren,&numChildren,
      NULL);
    if(menuHistory == children[PRINT_A]) {
	printSize = PRINT_A;
    } else if(menuHistory == children[PRINT_B]) {
	printSize = PRINT_B;
    } else if(menuHistory == children[PRINT_A3]) {
	printSize = PRINT_A3;
    } else if(menuHistory == children[PRINT_A4]) {
	printSize = PRINT_A4;
    } else {
	printSize = PRINT_A;
    }

  /* Title */
    XtVaGetValues(printSetupTitleMenu,
      XmNsubMenuId,&menuWidget,
      XmNmenuHistory,&menuHistory,
      NULL);
    XtVaGetValues(menuWidget,
      XmNchildren,&children,
      XmNnumChildren,&numChildren,
      NULL);
    if(menuHistory == children[PRINT_TITLE_NONE]) {
	printTitle = PRINT_TITLE_NONE;
    } else if(menuHistory == children[PRINT_TITLE_SHORT_NAME]) {
	printTitle = PRINT_TITLE_SHORT_NAME;
    } else if(menuHistory == children[PRINT_TITLE_LONG_NAME]) {
	printTitle = PRINT_TITLE_LONG_NAME;
    } else if(menuHistory == children[PRINT_TITLE_SPECIFIED]) {
	printTitle = PRINT_TITLE_SPECIFIED;
    } else {
	printTitle = PRINT_TITLE_LONG_NAME;
    }

  /* Width */
    string = XmTextFieldGetString(printSetupWidthTF);
    fval = atof(string);
    XtFree(string);
    if(fval < 0.0) {
	fval = 0;
	XmTextFieldSetString(printSetupWidthTF, "0.00");
	XBell(display,50);
    }
    XmTextFieldSetCursorPosition(printSetupWidthTF, 0);
    printWidth = fval;

  /* Height */
    string = XmTextFieldGetString(printSetupHeightTF);
    fval = atof(string);
    XtFree(string);
    if(fval < 0.0) {
	fval = 0;
	XmTextFieldSetString(printSetupHeightTF, "0.00");
	XBell(display,50);
    }
    XmTextFieldSetCursorPosition(printSetupHeightTF, 0);
    printHeight = fval;

  /* Title string */
    string = XmTextFieldGetString(printSetupTitleTF);
    sprintf(printTitleString, string);
    XtFree(string);
    XmTextFieldSetCursorPosition(printSetupTitleTF, 0);

  /* Filename */
    string = XmTextFieldGetString(printSetupFileTF);
    sprintf(printFile, string);
    XtFree(string);
    XmTextFieldSetCursorPosition(printSetupFileTF, 0);

  /* To file */
    printToFile = XmToggleButtonGetState(printSetupPrintToFileTB)?1:0;

  /* Date */
    printDate = XmToggleButtonGetState(printSetupPrintDateTB)?1:0;

  /* Time */
    printTime = XmToggleButtonGetState(printSetupPrintTimeTB)?1:0;
}

static void printSetupDialogCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    int type = (int)cd;
    int button;

  /* If the type is less than 4, the callback comes from an option
   *   menu button.  Find the real type from the userData of the RC
   *   parent of the button */
    if(type < 4) {
	button=type;
	XtVaGetValues(XtParent(w), XmNuserData, &type, NULL);
    }

    switch(type) {
    case PRINT_SETUP_OK_BTN:
	updatePrintSetupFromDialog();
	updatePrintSetupDlg();
	break;
    case PRINT_SETUP_CANCEL_BTN:
	XtPopdown(printSetupS);
	break;
    case PRINT_SETUP_PRINT_BTN:
      /* Check if it has been updated */
	if(printSetupModified && currentDisplayInfo) {
	    int cancel=0;

	    dmSetAndPopupQuestionDialog(currentDisplayInfo,
	      "Apply your changes first?", "Yes", "No", "Cancel");
	    switch(currentDisplayInfo->questionDialogAnswer) {
	    case 1:
		updatePrintSetupFromDialog();
		updatePrintSetupDlg();
		break;
	    case 3:
		cancel=1;
		break;
	    }
	  /* Pop the print dialog back on top */
	    XtPopup(printSetupS, XtGrabNone);
	    if(cancel) break;
	}
      /* Call the print routine from the mainFileMenuSimpleCallback */
	mainFileMenuSimpleCallback(NULL, (XtPointer)MAIN_FILE_PRINT_BTN, NULL);
      /* Pop the print dialog back on top */
	XtPopup(printSetupS, XtGrabNone);
	break;
    case PRINT_SETUP_HELP_BTN:
	callBrowser(medmHelpPath,"#PrintSetupDialogBox");
	break;
    case PRINT_SETUP_CMD_FIELD:
    case PRINT_SETUP_ORIENT_BTN:
    case PRINT_SETUP_SIZE_BTN:
    case PRINT_SETUP_WIDTH_FIELD:
    case PRINT_SETUP_HEIGHT_FIELD:
    case PRINT_SETUP_FILENAME_FIELD:
    case PRINT_SETUP_TOFILE_BTN:
    case PRINT_SETUP_TITLE_BTN:
    case PRINT_SETUP_DATE_BTN:
    case PRINT_SETUP_TIME_BTN:
	printSetupModified=TRUE;
	break;
    }
}
