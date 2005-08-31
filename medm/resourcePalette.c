/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*****************************************************************************
 *
 *     Original Author : Mark Anderson
 *     Second Author   : Frederick Vong
 *     Third Author    : Kenneth Evans, Jr.
 *
 *****************************************************************************
*/

/****************************************************************************
 * resourcePalette.c - Resource Palette                                     *
 * Mods: MDA - Creation                                                     *
 *       DMW - Tells resource palette which global resources Byte needs     *
 ***************************************************************************/

#define DEBUG_RESOURCE 0
#define DEBUG_TEXT_VERIFY 0
#define DEBUG_LOSING_FOCUS 0
#define DEBUG_RELATED_DISPLAY 0

#include <ctype.h>
#include "medm.h"
#include <Xm/MwmUtil.h>
#ifndef MEDM_CDEV
#include "dbDefs.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif
#include <cvtFast.h>
#ifdef __cplusplus
	   }
#endif

#define N_MAX_MENU_ELES 5
#ifdef EXTENDED_INTERFACE
# define N_MAIN_MENU_ELES 2
# define N_FILE_MENU_ELES 5
# define FILE_BTN_POSN 0
# define FILE_OPEN_BTN	 0
# define FILE_SAVE_BTN	 1
# define FILE_SAVE_AS_BTN 2
# define FILE_CLOSE_BTN	 3
#else
# define N_MAIN_MENU_ELES 1
# define N_FILE_MENU_ELES 1
# define FILE_BTN_POSN 0
# define FILE_CLOSE_BTN	 0
#endif

#ifdef EXTENDED_INTERFACE
# define N_BUNDLE_MENU_ELES 3
# define BUNDLE_BTN_POSN 1
# define BUNDLE_CREATE_BTN	0
# define BUNDLE_DELETE_BTN	1
# define BUNDLE_RENAME_BTN	2
#endif

#define N_HELP_MENU_ELES 1
#ifdef EXTENDED_INTERFACE
# define HELP_BTN_POSN 2
#else
# define HELP_BTN_POSN 1
#endif

#define HELP_RESOURCE_PALETTE_BTN 0

/* Function prototypes */

static void helpResourceCallback(Widget,XtPointer,XtPointer);

static menuEntry_t helpMenu[] = {
    { "On Resource Palette",  &xmPushButtonGadgetClass, 'P', NULL, NULL, NULL,
      helpResourceCallback, (XtPointer)HELP_RESOURCE_PALETTE_BTN, NULL},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
};

#ifdef EXTENDED_INTERFACE
static Widget resourceFilePDM;
static Widget resourceBundlePDM, openFSD;
#endif

XmString xmstringSelect;

static void createResourceEntries(Widget entriesSW);
static void initializeResourcePaletteElements();
static void createEntryRC( Widget parent, int rcType);

static void createBundleButtons( Widget messageF) {
  /****************************************************************************
   * Create Bundle Buttons: Create the control panel at bottom of resource    *
   *   and bundle editor.                                                     *
   ****************************************************************************/
    Widget separator;
    Arg args[4];
    int n;

    n = 0;
    XtSetArg(args[n],XmNlabelString,xmstringSelect); n++;
    resourceElementTypeLabel = XmCreateLabel(messageF,
      "resourceElementTypeLabel",args,n);

    n = 0;
    XtSetArg(args[n],XmNseparatorType,XmNO_LINE); n++;
    XtSetArg(args[n],XmNshadowThickness,0); n++;
    XtSetArg(args[n],XmNheight,1); n++;
    separator = XmCreateSeparator(messageF,"separator",args,n);

 /****** Label - message */
    XtVaSetValues(resourceElementTypeLabel,XmNtopAttachment,XmATTACH_FORM,
      XmNleftAttachment,XmATTACH_FORM,XmNrightAttachment,XmATTACH_FORM, NULL);

  /****** Separator*/
    XtVaSetValues(separator,XmNtopAttachment,XmATTACH_WIDGET,
      XmNtopWidget,resourceElementTypeLabel,
      XmNbottomAttachment,XmATTACH_FORM, XmNleftAttachment,XmATTACH_FORM,
      XmNrightAttachment,XmATTACH_FORM, NULL);

    XtManageChild(resourceElementTypeLabel);
    XtManageChild(separator);
}

static void pushButtonActivateCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *cdi = currentDisplayInfo;
    int rcType = (int) cd;

    UNREFERENCED(cbs);

    switch(rcType) {
    case RDDATA_RC:
	relatedDisplayDataDialogPopup(w);
	break;
    case SHELLDATA_RC:
	if(!shellCommandS) {
	    shellCommandS = createShellCommandDataDialog(w);
	}
      /* update shell command data from globalResourceBundle */
	updateShellCommandDataDialog();
	XtManageChild(cmdForm);
	XtPopup(shellCommandS,XtGrabNone);
	break;
    case CPDATA_RC:
#ifdef CARTESIAN_PLOT
	if(!cartesianPlotS) {
	    cartesianPlotS = createCartesianPlotDataDialog(w);
	}
      /* update cartesian plot data from globalResourceBundle */
	updateCartesianPlotDataDialog();
	XtManageChild(cpForm);
	XtPopup(cartesianPlotS,XtGrabNone);
#endif     /* #ifdef CARTESIAN_PLOT */
	break;
    case SCDATA_RC:
	popupStripChartDataDialog();
	break;
    case CPAXIS_RC:
#ifdef CARTESIAN_PLOT
	if(!cartesianPlotAxisS) {
	    cartesianPlotAxisS = createCartesianPlotAxisDialog(w);
	}
      /* update cartesian plot axis data from globalResourceBundle */
	updateCartesianPlotAxisDialog();
	medmMarkDisplayBeingEdited(cdi);
	XtManageChild(cpAxisForm);
	XtPopup(cartesianPlotAxisS,XtGrabNone);
#endif     /* #ifdef CARTESIAN_PLOT */
	break;
    case LIMITS_RC:
	medmMarkDisplayBeingEdited(cdi);
	popupPvLimits(cdi);
	break;
    default:
	medmPrintf(1,"\npushButtonActivate: Invalid type = %d\n",rcType);
	break;
    }
}

static void optionMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *cdi = currentDisplayInfo;
    int buttonId = (int)cd;
    int rcType;
    DlElement *elementPtr;

    UNREFERENCED(cbs);

  /****** rcType (which option menu) is stored in userData */
    XtVaGetValues(XtParent(w),XmNuserData,&rcType,NULL);
    switch(rcType) {
    case ALIGN_RC:
	globalResourceBundle.align = (TextAlign)(FIRST_TEXT_ALIGN + buttonId);
	break;
    case FORMAT_RC:
	globalResourceBundle.format = (TextFormat)(FIRST_TEXT_FORMAT + buttonId);
	break;
    case LABEL_RC:
	globalResourceBundle.label = (LabelType)(FIRST_LABEL_TYPE + buttonId);
	break;
    case DIRECTION_RC:
	globalResourceBundle.direction = (Direction)(FIRST_DIRECTION + buttonId);
	break;
    case CLRMOD_RC:
	globalResourceBundle.clrmod = (ColorMode)(FIRST_COLOR_MODE + buttonId);
	break;
    case FILLMOD_RC:
	globalResourceBundle.fillmod = (FillMode)(FIRST_FILL_MODE + buttonId);
	break;
    case STYLE_RC:
	globalResourceBundle.style = (EdgeStyle)(FIRST_EDGE_STYLE + buttonId);
	break;
    case FILL_RC:
	globalResourceBundle.fill = (FillStyle)(FIRST_FILL_STYLE + buttonId);
	break;
    case VIS_RC:
	globalResourceBundle.vis = (VisibilityMode)(FIRST_VISIBILITY_MODE + buttonId);
	break;
    case UNITS_RC:
	globalResourceBundle.units = (TimeUnits)(FIRST_TIME_UNIT + buttonId);
	break;
    case CSTYLE_RC:
	globalResourceBundle.cStyle = (CartesianPlotStyle)(FIRST_CARTESIAN_PLOT_STYLE + buttonId);
	break;
    case ERASE_OLDEST_RC:
	globalResourceBundle.erase_oldest = (EraseOldest)(FIRST_ERASE_OLDEST + buttonId);
	break;
    case STACKING_RC:
	globalResourceBundle.stacking = (Stacking)(FIRST_STACKING + buttonId);
	break;
    case IMAGE_TYPE_RC:
	globalResourceBundle.imageType = (ImageType)(FIRST_IMAGE_TYPE + buttonId);
	break;
    case ERASE_MODE_RC:
	globalResourceBundle.eraseMode = (eraseMode_t)(FIRST_ERASE_MODE + buttonId);
	break;
    case RD_VISUAL_RC:
	globalResourceBundle.rdVisual =
	  (relatedDisplayVisual_t)(FIRST_RD_VISUAL + buttonId);
	break;
    case GRID_ON_RC:
	globalResourceBundle.gridOn = buttonId;
	break;
    case GRID_SNAP_RC:
	globalResourceBundle.snapToGrid = buttonId;
	break;
    default:
	medmPrintf(1,"\noptionMenuSimpleCallback: Unknown rcType = %d\n",rcType);
	break;
    }

  /****** Update elements (this is overkill, but okay for now)
	*	-- not as efficient as it should be (don't update EVERYTHING if only
	*	   one item changed!) */
    if(cdi) {
	DlElement *dlElement = FirstDlElement(cdi->selectedDlElementList);

	unhighlightSelectedElements();
	while(dlElement) {
	    elementPtr = dlElement->structure.element;
	    updateElementFromGlobalResourceBundle(elementPtr);
	    dlElement = dlElement->next;
	}

	dmTraverseNonWidgetsInDisplayList(cdi);
	medmMarkDisplayBeingEdited(cdi);
	highlightSelectedElements();
    }
}

static void colorSelectCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    int rcType = (int) cd;

    UNREFERENCED(w);
    UNREFERENCED(cbs);

    if(colorMW != NULL) {
	setCurrentDisplayColorsInColorPalette(rcType,0);
	XtPopup(colorS,XtGrabNone);
    } else {
	createColor();
	setCurrentDisplayColorsInColorPalette(rcType,0);
	XtPopup(colorS,XtGrabNone);
    }
}

#ifdef EXTENDED_INTERFACE
static void fileOpenCallback(Widget w, int btn, XmAnyCallbackStruct *call_data)
{
    switch(call_data->reason){
    case XmCR_CANCEL:
	XtUnmanageChild(w);
	break;
    case XmCR_OK:
	XtUnmanageChild(w);
	break;
    }
}
#endif

static void fileMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    int buttonNumber = (int) cd;

    UNREFERENCED(w);
    UNREFERENCED(cbs);

    switch(buttonNumber) {
#ifdef EXTENDED_INTERFACE
    case FILE_OPEN_BTN:
	if(openFSD == NULL) {
	    n = 0;
	    label = XmStringCreateLocalized(RESOURCE_DIALOG_MASK);
	    XtSetArg(args[n],XmNdirMask,label); n++;
	    XtSetArg(args[n],XmNdialogStyle,
	      XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
	    openFSD = XmCreateFileSelectionDialog(resourceFilePDM,
	      "openFSD",args,n);
	  /* Make Filter text field insensitive to prevent user
             hand-editing dirMask */
	    textField = XmFileSelectionBoxGetChild(openFSD,
	      XmDIALOG_FILTER_TEXT);
	    XtSetSensitive(textField,FALSE);
	    XtAddCallback(openFSD,XmNokCallback,
	      fileOpenCallback,
	      (XtPointer)FILE_OPEN_BTN);
	    XtAddCallback(openFSD,XmNcancelCallback,
	      fileOpenCallback,
	      (XtPointer)FILE_OPEN_BTN);
	    XmStringFree(label);
	    XtManageChild(openFSD);
	} else {
	    XtManageChild(openFSD);
	}
	break;
    case FILE_SAVE_BTN:
	break;
    case FILE_SAVE_AS_BTN:
	break;
#endif
    case FILE_CLOSE_BTN:
	XtPopdown(resourceS);
	break;
    }
}

#ifdef EXTENDED_INTERFACE
static void bundleMenuSimpleCallback(Widget w, int buttonNumber,
  XmAnyCallbackStruct *call_data)
{
    XmString label;
    int n;
    Arg args[10];
    Widget textField;

    switch(buttonNumber) {
    }
}
#endif

/* Used to verify int input as it happens
 *  Allowed characters:
 *    digits   Anywhere
 *    +,-      First Position only
 * (Same as textFieldFloatVerifyCallback below except for .)
 */
void textFieldNumericVerifyCallback(Widget w, XtPointer clientData, XtPointer callData)
{
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)callData;
    int i,abort;

    UNREFERENCED(clientData);

#if DEBUG_TEXT_VERIFY
    {
	int i;

	print("\ntextFieldNumericVerifyCallback: Entered\n");
	print("  event: %x  cbs->text->ptr: %x\n"
	  "  startPos: %d endPos: %d currInsert: %d newInsert: %d\n",
	  cbs->event,cbs->text->ptr,
	  cbs->startPos,cbs->endPos,cbs->currInsert,cbs->newInsert);
	if(cbs->text->length) {
	    print("  length=%d: \"",cbs->text->length);
	    for(i=0; i < cbs->text->length; i++) print("%c",cbs->text->ptr[i]);
	    print("\"\n");
	} else {
	    print("  length=0\n");
	}
    }
#endif
  /* Is a deletion */
    if(!cbs->text->length) return;

  /* Check the new characters, character by character */
    abort = 0;
    for (i = 0; i < cbs->text->length && !abort; i++) {
      /* Digits are OK, check non-digits */
	if(!isdigit(cbs->text->ptr[i])) {
	    switch(cbs->text->ptr[i]) {
	    case '+':
	    case '-':
	      /* Abort if this is not the first new char */
		if(i) {
		    abort = 1;
		    break;
		}
	      /* OK if insertion point is at 0 */
		if(!cbs->currInsert) break;
	      /* OK if insertion point is after a replacement starting at 0 */
		if(cbs->startPos == 0 && cbs->startPos != cbs->endPos &&
		  cbs->endPos == cbs->currInsert) break;
	      /* Else abort */
		abort = 1;
		break;
	    default:
		abort = 4;
	    }
	}
    }
    if(abort) cbs->doit = False;
#if DEBUG_TEXT_VERIFY
    print("  doit: %d",cbs->doit);
    if(!abort) print("\n");
    else if(abort == 1) print("  +/- not in first position\n");
    else if(abort == 2) print("  More than one new dot\n");
    else if(abort == 3) print("  Already have a dot\n");
    else if(abort == 4) print("  Invalid character\n");
#endif
}

/* Used to verify float input as it happens
 *  Allowed characters:
 *    digits   Anywhere
 *    +,-      First Position only
 *    .        Anywhere, but only one
 * (Same as textFieldNumericVerifyCallback above except for .)
 */
void textFieldFloatVerifyCallback(Widget w, XtPointer clientData, XtPointer callData)
{
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *)callData;
    int i,j,len,newDot,replace,abort;
    char *curString;

    UNREFERENCED(clientData);

#if DEBUG_TEXT_VERIFY
    {
	int i;

	print("\ntextFieldFloatVerifyCallback: Entered\n");
	print("  event: %x  cbs->text->ptr: %x\n"
	  "  startPos: %d endPos: %d currInsert: %d newInsert: %d\n",
	  cbs->event,cbs->text->ptr,
	  cbs->startPos,cbs->endPos,cbs->currInsert,cbs->newInsert);
	if(cbs->text->length) {
	    print("  length=%d: \"",cbs->text->length);
	    for(i=0; i < cbs->text->length; i++) print("%c",cbs->text->ptr[i]);
	    print("\"\n");
	} else {
	    print("  length=0\n");
	}
    }
#endif
  /* Is a deletion */
    if(!cbs->text->length) return;

  /* Check the new characters, character by character */
    newDot = replace = abort = 0;
    for (i = 0; i < cbs->text->length && !abort; i++) {
      /* Digits are OK, check non-digits */
	if(!isdigit(cbs->text->ptr[i])) {
	    switch(cbs->text->ptr[i]) {
	    case '+':
	    case '-':
	      /* Abort if this is not the first new char */
		if(i) {
		    abort = 1;
		    break;
		}
	      /* OK if insertion point is at 0 */
		if(!cbs->currInsert) break;
	      /* OK if insertion point is after a replacement starting at 0 */
		if(cbs->startPos == 0 && cbs->startPos != cbs->endPos &&
		  cbs->endPos == cbs->currInsert) break;
	      /* Else abort */
		abort = 1;
		break;
	    case '.':
	      /* Abort if already have a new dot */
		if(newDot) {
		    abort = 2;
		    break;
		}
		newDot = 1;
	      /* Get the current string */
		curString = XmTextFieldGetString(w);
		len = strlen(curString);
	      /* Check if this is a replacement */
		if(cbs->startPos != cbs->endPos) replace = 1;
	      /* Check character by character outside the replacement */
		for (j = 0; j < len; j++) {
		    if(replace && j >= cbs->startPos && j < cbs->endPos) {
			continue;
		    }
		  /* Abort if there is already a . */
		    if(curString[j] == '.') {
			abort = 3;
			break;
		    }
		}
		XtFree(curString);
		break;
	    default:
		abort = 4;
	    }
	}
    }
    if(abort) cbs->doit = False;
#if DEBUG_TEXT_VERIFY
    print("  doit: %d",cbs->doit);
    if(!abort) print("\n");
    else if(abort == 1) print("  +/- not in first position\n");
    else if(abort == 2) print("  More than one new dot\n");
    else if(abort == 3) print("  Already have a dot\n");
    else if(abort == 4) print("  Invalid character\n");
#endif
}

void scaleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *cdi = currentDisplayInfo;
    int rcType = (int) cd;  /* the resource element type */
    XmScaleCallbackStruct *scbs = (XmScaleCallbackStruct *)cbs;

    UNREFERENCED(w);

  /****** Show users degrees, but internally use degrees*64 as Xlib requires */
    switch(rcType) {
    case BEGIN_RC:
	globalResourceBundle.begin = 64*scbs->value;
	medmMarkDisplayBeingEdited(cdi);
	break;
    case PATH_RC:
	globalResourceBundle.path = 64*scbs->value;
	medmMarkDisplayBeingEdited(cdi);
	break;
    default:
	break;
    }

  /****** Update elements (this is overkill, but okay for now) */
    if(cdi != NULL) {
	DlElement *dlElement = FirstDlElement(
	  cdi->selectedDlElementList);
	unhighlightSelectedElements();
	while(dlElement) {
	    updateElementFromGlobalResourceBundle(dlElement->structure.element);
	    dlElement = dlElement->next;
	}
	dmTraverseNonWidgetsInDisplayList(cdi);
	highlightSelectedElements();
    }
}

void textFieldActivateCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *cdi = currentDisplayInfo;
    int rcType = (int)cd;
    char *stringValue;
    int redoDisplay = 0;
    int clearComposite = 0;
    int updateResourcePalette = 0;

    UNREFERENCED(cbs);

    stringValue = XmTextFieldGetString(w);
    switch(rcType) {
    case X_RC:
	globalResourceBundle.x = atoi(stringValue);
	break;
    case Y_RC:
	globalResourceBundle.y = atoi(stringValue);
	break;
    case WIDTH_RC:
	globalResourceBundle.width = atoi(stringValue);
	break;
    case HEIGHT_RC:
	globalResourceBundle.height = atoi(stringValue);
	break;
    case RDBK_RC:
	strcpy(globalResourceBundle.chan[0],stringValue);
	break;
    case CTRL_RC:
	strcpy(globalResourceBundle.chan[0],stringValue);
	break;
    case TITLE_RC:
	strcpy(globalResourceBundle.title,stringValue);
	break;
    case XLABEL_RC:
	strcpy(globalResourceBundle.xlabel,stringValue);
	break;
    case YLABEL_RC:
	strcpy(globalResourceBundle.ylabel,stringValue);
	break;
    case LINEWIDTH_RC:
	globalResourceBundle.lineWidth = atoi(stringValue);
      /* This may cause the bounding box to change for Polyline and
         Polygon */
	updateResourcePalette = 1;
	break;
    case SBIT_RC:
    {
	int value = atoi(stringValue);
	if(value >= 0 && value <= 31) {
	    globalResourceBundle.sbit = value;
	} else {
	    char tmp[32];
	    sprintf(tmp,"%d",globalResourceBundle.sbit);
	    XmTextFieldSetString(w,tmp);
	}
	break;
    }
    case EBIT_RC:
    {
	int value = atoi(stringValue);
	if(value >= 0 && value <= 31) {
	    globalResourceBundle.ebit = value;
	} else {
	    char tmp[32];
	    sprintf(tmp,"%d",globalResourceBundle.ebit);
	    XmTextFieldSetString(w,tmp);
	}
	break;
    }
    case GRID_SPACING_RC:
    {
	int value = atoi(stringValue);
	char tmp[32];

	if(value < 2) value = 2;
	globalResourceBundle.gridSpacing = value;
	sprintf(tmp,"%d",globalResourceBundle.gridSpacing);
	XmTextFieldSetString(w,tmp);
	break;
    }

    case VIS_CALC_RC:
	strcpy(globalResourceBundle.visCalc,stringValue);
	break;
    case CHAN_A_RC:
	strcpy(globalResourceBundle.chan[0],stringValue);
      /* A non-NULL string value for the dynamics channel means that VIS
       * and CLRMOD must be visible */
	if(*stringValue != '\0') {
	    XtSetSensitive(resourceEntryRC[CLRMOD_RC],True);
	    XtSetSensitive(resourceEntryRC[VIS_RC],True);
	    XtSetSensitive(resourceEntryRC[VIS_CALC_RC],True);
	} else {
	    XtSetSensitive(resourceEntryRC[CLRMOD_RC],False);
	    XtSetSensitive(resourceEntryRC[VIS_RC],False);
	    XtSetSensitive(resourceEntryRC[VIS_CALC_RC],False);
	}
	break;
    case CHAN_B_RC:
	strcpy(globalResourceBundle.chan[1],stringValue);
	break;
    case CHAN_C_RC:
	strcpy(globalResourceBundle.chan[2],stringValue);
	break;
    case CHAN_D_RC:
	strcpy(globalResourceBundle.chan[3],stringValue);
	break;
    case DIS_RC:
	globalResourceBundle.dis = atoi(stringValue);
	break;
    case XYANGLE_RC:
	globalResourceBundle.xyangle = atoi(stringValue);
	break;
    case ZANGLE_RC:
	globalResourceBundle.zangle = atoi(stringValue);
	break;
    case PERIOD_RC:
	globalResourceBundle.period = atof(stringValue);
	break;
    case COUNT_RC:
	strcpy(globalResourceBundle.countPvName,stringValue);
	break;
    case TEXTIX_RC:
	strcpy(globalResourceBundle.textix,stringValue);
	break;
    case MSG_LABEL_RC:
	strcpy(globalResourceBundle.messageLabel,stringValue);
	break;
    case PRESS_MSG_RC:
	strcpy(globalResourceBundle.press_msg,stringValue);
	break;
    case RELEASE_MSG_RC:
	strcpy(globalResourceBundle.release_msg,stringValue);
	break;
    case IMAGE_NAME_RC:
	strcpy(globalResourceBundle.imageName,stringValue);
	break;
    case IMAGE_CALC_RC:
	strcpy(globalResourceBundle.imageCalc,stringValue);
	break;
    case DATA_RC:
	strcpy(globalResourceBundle.data,stringValue);
	break;
    case CMAP_RC:
      /* Free the colormap, so when we call run->execute (in
         updateElementFromGlobalResourceBundle) for the display, it
         will use the new one */
	if(cdi && cdi->dlColormap) {
	    free((char *)cdi->dlColormap);
	    cdi->dlColormap = NULL;
	}
	strcpy(globalResourceBundle.cmap,stringValue);
	redoDisplay = 1;
	break;
    case COMPOSITE_FILE_RC:
      /* If the filename is not blank, set a flag to clear the element
         list.  When we call run->execute (in
         updateElementFromGlobalResourceBundle) for the display, it
         will recreate it from the file */
	if(*stringValue) clearComposite = 1;
	strcpy(globalResourceBundle.compositeFile,stringValue);
	break;
    case PRECISION_RC:
	globalResourceBundle.dPrecision = atof(stringValue);
	break;
    case TRIGGER_RC:
	strcpy(globalResourceBundle.trigger,stringValue);
	break;
    case ERASE_RC:
	strcpy(globalResourceBundle.erase,stringValue);
	if(strlen(stringValue) > (size_t) 0) {
	    XtSetSensitive(resourceEntryRC[ERASE_MODE_RC],True);
	} else {
	    XtSetSensitive(resourceEntryRC[ERASE_MODE_RC],False);
	}
	break;
    case RD_LABEL_RC:
	strcpy(globalResourceBundle.rdLabel,stringValue);
	break;
    case WS_FORMAT_RC:
	strcpy(globalResourceBundle.wsFormat,stringValue);
	break;
    }
    XtFree(stringValue);

  /* Update elements (this is overkill, but okay for now) */
  /* KE: Supposedly only one element is selected when this routine is
     called */
    if(cdi != NULL) {
	DlElement *dlElement = FirstDlElement(cdi->selectedDlElementList);

	unhighlightSelectedElements();
	while(dlElement) {
	    DlElement *pE = dlElement->structure.element;

	  /* Clear the composite element list if a composite */
	    if(clearComposite && pE->type == DL_Composite) {
		DlComposite *dlComposite = pE->structure.composite;

	      /* Use removeDlDisplayListElementsExceptDisplay instead
                 of clearDlDisplayList because it also destroys the
                 widgets and there should not be a display in the
                 composite element list anyway */
		removeDlDisplayListElementsExceptDisplay(cdi,
		  dlComposite->dlElementList);
	    }

	  /* Update the element */
	    updateElementFromGlobalResourceBundle(pE);

	  /* Update the resource palette if other element data may
             have changed.  Currently after LineWidth change for
             Polyline and Polygon */
	    if(updateResourcePalette &&
	      (pE->type == DL_Polyline || pE->type == DL_Polygon)) {
	      /* Use objectDataOnly = True */
		updateGlobalResourceBundleAndResourcePalette(True);
	    }

	    dlElement = dlElement->next;
	}
	dmTraverseNonWidgetsInDisplayList(cdi);
	highlightSelectedElements();
	medmMarkDisplayBeingEdited(cdi);
    }

  /* Redo the display if indicated.  Is necessary after a colormap
     change to see the change. */
    if(redoDisplay) {
	dmCleanupDisplayInfo(cdi,False);
	dmTraverseDisplayList(cdi);
    }
}

void textFieldLosingFocusCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    int rcType = (int) cd;
    char string[MAX_TOKEN_LENGTH], *newString;
    int tail;

    UNREFERENCED(w);
    UNREFERENCED(cbs);

#if DEBUG_LOSING_FOCUS
    print("\ntextFieldLosingFocusCallback: rcType=%d[%s]\n",
      rcType,
      (rcType >= 0 && rcType < MAX_RESOURCE_ENTRY)?
      resourceEntryStringTable[rcType]:"Unknown");
#endif

    newString = string;
  /** losing focus - make sure that the text field remains accurate
      wrt globalResourceBundle */
    switch(rcType) {
    case X_RC:
	sprintf(string,"%d",globalResourceBundle.x);
	break;
    case Y_RC:
	sprintf(string,"%d",globalResourceBundle.y);
	break;
    case WIDTH_RC:
	sprintf(string,"%d",globalResourceBundle.width);
	break;
    case HEIGHT_RC:
	sprintf(string,"%d",globalResourceBundle.height);
	break;
    case LINEWIDTH_RC:
	sprintf(string,"%d",globalResourceBundle.lineWidth);
	break;
    case RDBK_RC:
	newString = globalResourceBundle.chan[0];
	break;
    case CTRL_RC:
	newString = globalResourceBundle.chan[0];
	break;
    case TITLE_RC:
	newString = globalResourceBundle.title;
	break;
    case XLABEL_RC:
	newString = globalResourceBundle.xlabel;
	break;
    case YLABEL_RC:
	newString = globalResourceBundle.ylabel;
	break;
    case VIS_CALC_RC:
	newString = globalResourceBundle.visCalc;
	break;
    case CHAN_A_RC:
	newString = globalResourceBundle.chan[0];
	break;
    case CHAN_B_RC:
	newString = globalResourceBundle.chan[1];
	break;
    case CHAN_C_RC:
	newString = globalResourceBundle.chan[2];
	break;
    case CHAN_D_RC:
	newString = globalResourceBundle.chan[3];
	break;
    case DIS_RC:
	sprintf(string,"%d",globalResourceBundle.dis);
	break;
    case XYANGLE_RC:
	sprintf(string,"%d",globalResourceBundle.xyangle);
	break;
    case ZANGLE_RC:
	sprintf(string,"%d",globalResourceBundle.zangle);
	break;
    case PERIOD_RC:
	cvtDoubleToString(globalResourceBundle.period,string,0);
	break;
    case COUNT_RC:
	newString= globalResourceBundle.countPvName;
	break;
    case TEXTIX_RC:
	newString = globalResourceBundle.textix;
	break;
    case MSG_LABEL_RC:
	newString = globalResourceBundle.messageLabel;
	break;
    case PRESS_MSG_RC:
	newString = globalResourceBundle.press_msg;
	break;
    case RELEASE_MSG_RC:
	newString = globalResourceBundle.release_msg;
	break;
    case IMAGE_NAME_RC:
	newString = globalResourceBundle.imageName;
	break;
    case IMAGE_CALC_RC:
	newString = globalResourceBundle.imageCalc;
	break;
    case DATA_RC:
	newString = globalResourceBundle.data;
	break;
    case CMAP_RC:
	newString = globalResourceBundle.cmap;
	break;
    case COMPOSITE_FILE_RC:
	newString = globalResourceBundle.compositeFile;
	break;
    case PRECISION_RC:
	sprintf(string,"%f",globalResourceBundle.dPrecision);
      /* strip trailing zeroes */
	tail = strlen(string);
	while(string[--tail] == '0') string[tail] = '\0';
	break;
    case SBIT_RC:
	sprintf(string,"%d",globalResourceBundle.sbit);
	break;
    case EBIT_RC:
	sprintf(string,"%d",globalResourceBundle.ebit);
	break;
    case GRID_SPACING_RC:
	sprintf(string,"%d",globalResourceBundle.gridSpacing);
	break;
    case TRIGGER_RC:
	newString= globalResourceBundle.trigger;
	break;
    case ERASE_RC:
	newString= globalResourceBundle.erase;
	break;
    case WS_FORMAT_RC:
	newString= globalResourceBundle.wsFormat;
	break;
    }
    XmTextFieldSetString(resourceEntryElement[rcType],newString);
}

#ifdef EXTENDED_INTERFACE
/****************************************************************************
 * Bundle Call-back                                                         *
 ****************************************************************************/
static void bundleCallback(Widget w, int bundleId,
  XmToggleButtonCallbackStruct *call_data)
{

  /** Since both on & off will invoke this callback, only care about transition
   * of one to ON
   */

    if(call_data->set == False) return;

}
#endif

/*
 * initialize globalResourceBundle with (semi-arbitrary) values
 */
void initializeGlobalResourceBundle()
{
    DisplayInfo *cdi = currentDisplayInfo;
    int i;

    globalResourceBundle.x = 0;
    globalResourceBundle.y = 0;
    globalResourceBundle.width = 10;
    globalResourceBundle.height = 10;
    globalResourceBundle.sbit = 15;
    globalResourceBundle.ebit = 0;
    globalResourceBundle.rdLabel[0] = '\0';
    globalResourceBundle.rdVisual = RD_MENU;
#if 0
    globalResourceBundle.rdbk[0] = '\0';
    globalResourceBundle.ctrl[0] = '\0';
#endif
    globalResourceBundle.title[0] = '\0';
    globalResourceBundle.xlabel[0] = '\0';
    globalResourceBundle.ylabel[0] = '\0';
    globalResourceBundle.wsFormat[0]='\0';

    if(cdi) {
      /*
       * (MDA) hopefully this will work in the general case (with displays being
       *	made current and un-current)
       */
	globalResourceBundle.clr = cdi->drawingAreaForegroundColor;
	globalResourceBundle.bclr = cdi->drawingAreaBackgroundColor;
    } else {
      /*
       * (MDA) this isn't safe if the non-standard colormap is loaded, but this
       *	shouldn't get called unless starting from scratch...
       */
	globalResourceBundle.clr = 14;	/* black */
	globalResourceBundle.bclr = 4;	/* grey  */
    }
    globalResourceBundle.begin = 0;
    globalResourceBundle.path = 64*90;		/* arc in first quadrant */
    globalResourceBundle.align= HORIZ_LEFT;
    globalResourceBundle.format = MEDM_DECIMAL;
    globalResourceBundle.label = LABEL_NONE;
    globalResourceBundle.direction = RIGHT;
    globalResourceBundle.clrmod = STATIC;
    globalResourceBundle.fillmod = FROM_EDGE;
    globalResourceBundle.style = SOLID;
    globalResourceBundle.fill = F_SOLID;
    globalResourceBundle.lineWidth = 0;
    globalResourceBundle.dPrecision = 1.;
    globalResourceBundle.vis = V_STATIC;
    globalResourceBundle.visCalc[0] = '\0';
    globalResourceBundle.chan[0][0] = '\0';
    globalResourceBundle.chan[1][0] = '\0';
    globalResourceBundle.chan[2][0] = '\0';
    globalResourceBundle.chan[3][0] = '\0';
    globalResourceBundle.data_clr = 0;
    globalResourceBundle.dis = 10;
    globalResourceBundle.xyangle = 45;
    globalResourceBundle.zangle = 45;
    globalResourceBundle.period = 60.0;
    globalResourceBundle.units = SECONDS;
    globalResourceBundle.cStyle = POINT_PLOT;
    globalResourceBundle.erase_oldest = ERASE_OLDEST_OFF;
    globalResourceBundle.countPvName[0] = '\0';
    globalResourceBundle.stacking = ROW;
    globalResourceBundle.imageType= NO_IMAGE;
    globalResourceBundle.name[0] = '\0';
    globalResourceBundle.textix[0] = '\0';
    globalResourceBundle.messageLabel[0] = '\0';
    globalResourceBundle.press_msg[0] = '\0';
    globalResourceBundle.release_msg[0] = '\0';
    globalResourceBundle.imageName[0] = '\0';
    globalResourceBundle.imageCalc[0] = '\0';
    globalResourceBundle.compositeName[0] = '\0';
    globalResourceBundle.compositeFile[0] = '\0';
    globalResourceBundle.data[0] = '\0';
    globalResourceBundle.cmap[0] = '\0';
    for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
	globalResourceBundle.rdData[i].label[0] = '\0';
	globalResourceBundle.rdData[i].name[0] = '\0';
	globalResourceBundle.rdData[i].args[0] = '\0';
	globalResourceBundle.rdData[i].mode = ADD_NEW_DISPLAY;
    }
    for (i = 0; i < MAX_SHELL_COMMANDS; i++) {
	globalResourceBundle.cmdData[i].label[0] = '\0';
	globalResourceBundle.cmdData[i].command[0] = '\0';
	globalResourceBundle.cmdData[i].args[0] = '\0';
    }
    for (i = 0; i < MAX_TRACES; i++) {
	globalResourceBundle.cpData[i].xdata[0] = '\0';
	globalResourceBundle.cpData[i].ydata[0] = '\0';
	globalResourceBundle.cpData[i].data_clr = 0;
    }
    for (i = 0; i < MAX_PENS; i++) {
	globalResourceBundle.scData[i].chan[0] = '\0';
	globalResourceBundle.scData[i].clr = 0;
	limitsAttributeInit(&globalResourceBundle.scData[i].limits);
    }
    plotAxisDefinitionInit(&(globalResourceBundle.axis[X_AXIS_ELEMENT]));
  /* structure copy for other two axis definitions */
    globalResourceBundle.axis[Y1_AXIS_ELEMENT]
      = globalResourceBundle.axis[X_AXIS_ELEMENT];
    globalResourceBundle.axis[Y2_AXIS_ELEMENT]
      = globalResourceBundle.axis[X_AXIS_ELEMENT];
    globalResourceBundle.trigger[0] = '\0';
    globalResourceBundle.erase[0] = '\0';
    globalResourceBundle.eraseMode = ERASE_IF_NOT_ZERO;
    limitsAttributeInit(&globalResourceBundle.limits);
}

/****************************************************************************
 * Initialize XmString Value Tables: ResourceBundle and related widgets.    *
 ****************************************************************************/
void initializeXmStringValueTables()
{
    int i;
    static Boolean initialized = False;

  /****** Initialize XmString table for element types */
    if(!initialized) {
	initialized = True;
	for (i = 0; i <NUM_DL_ELEMENT_TYPES; i++) {
	    elementXmStringTable[i] = XmStringCreateLocalized(elementStringTable[i]);
	}

      /****** Initialize XmString table for value types (format, alignment types) */
	for (i = 0; i < NUMBER_STRING_VALUES; i++) {
	    xmStringValueTable[i] = XmStringCreateLocalized(stringValueTable[i]);
	}
    }
}

/****************************************************************************
 * Create Resource: Create and initialize the resourcePalette,              *
 *   resourceBundle and related widgets.                                    *
 ****************************************************************************/
void createResource()
{
    DisplayInfo *cdi = currentDisplayInfo;
    Widget entriesSW, resourceMB, messageF, resourceHelpPDM;
    XmString buttons[N_MAX_MENU_ELES];
    KeySym keySyms[N_MAX_MENU_ELES];
    XmButtonType buttonType[N_MAX_MENU_ELES];
    int i, n;
    Arg args[10];

  /****** If resource palette has already been created, simply return */
    if(resourceMW != NULL) return;

  /****** This make take a second... give user some indication */
    if(cdi != NULL) XDefineCursor(display,
      XtWindow(cdi->drawingArea), watchCursor);

  /****** Initialize XmString tables */
    initializeXmStringValueTables();
    xmstringSelect = XmStringCreateLocalized("Select...");

#ifdef EXTENDED_INTERFACE
    openFSD = NULL;
#endif

  /****** Create a main window in a dialog */
    n = 0;
    XtSetArg(args[n],XtNiconName,"Resources"); n++;
    XtSetArg(args[n],XmNautoUnmanage,False); n++;
    XtSetArg(args[n],XtNtitle,"Resource Palette"); n++;
    XtSetArg(args[n],XtNallowShellResize,TRUE); n++;
    XtSetArg(args[n],XmNkeyboardFocusPolicy,XmEXPLICIT); n++;

  /****** Map window manager menu Close function to application close... */
    XtSetArg(args[n],XmNdeleteResponse,XmDO_NOTHING); n++;
#if OMIT_RESIZE_HANDLES
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
  /* KE: The following is necessary for Exceed, which turns off the
     resize function with the handles.  It should not be necessary */
    XtSetArg(args[n],XmNmwmFunctions, MWM_FUNC_ALL); n++;
#endif
    resourceS = XtCreatePopupShell("resourceS",topLevelShellWidgetClass,
      mainShell,args,n);

    XmAddWMProtocolCallback(resourceS,WM_DELETE_WINDOW,
      wmCloseCallback,(XtPointer)OTHER_SHELL);

    resourceMW = XmCreateMainWindow(resourceS,"resourceMW",NULL,0);

  /****** Create the menu bar */
    buttons[0] = XmStringCreateLocalized("File");

#ifdef EXTENDED_INTERFACE
    buttons[1] = XmStringCreateLocalized("Bundle");
    keySyms[1] = 'B';
#endif

    keySyms[0] = 'F';
    n = 0;
#if 0
    XtSetArg(args[n],XmNbuttonCount,N_MAIN_MENU_ELES); n++;
    XtSetArg(args[n],XmNbuttons,buttons); n++;
    XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
    XtSetArg(args[n],XmNforeground,defaultForeground); n++;
    XtSetArg(args[n],XmNbackground,defaultBackground); n++;
    resourceMB = XmCreateSimpleMenuBar(resourceMW, "resourceMB",args,n);
#endif

    resourceMB = XmVaCreateSimpleMenuBar(resourceMW, "resourceMB",
      XmVaCASCADEBUTTON, buttons[0], 'F',
      NULL);

#if EXPLICITLY_OVERWRITE_CDE_COLORS
  /* Color menu bar explicitly to avoid CDE interference */
    colorMenuBar(resourceMB,defaultForeground,defaultBackground);
#endif

  /* Free strings */
    for (i = 0; i < N_MAIN_MENU_ELES; i++) XmStringFree(buttons[i]);

  /****** create the file pulldown menu pane */
#ifdef EXTENDED_INTERFACE
    buttons[0] = XmStringCreateLocalized("Open...");
    buttons[1] = XmStringCreateLocalized("Save");
    buttons[2] = XmStringCreateLocalized("Save As...");
    buttons[3] = XmStringCreateLocalized("Separator");
    buttons[4] = XmStringCreateLocalized("Close");
    keySyms[0] = 'O';
    keySyms[1] = 'S';
    keySyms[2] = 'A';
    keySyms[3] = ' ';
    keySyms[4] = 'C';
    buttonType[0] = XmPUSHBUTTON;
    buttonType[1] = XmPUSHBUTTON;
    buttonType[2] = XmPUSHBUTTON;
    buttonType[3] = XmSEPARATOR;
    buttonType[4] = XmPUSHBUTTON;
#else
    buttons[0] = XmStringCreateLocalized("Close");
    keySyms[0] = 'C';
    buttonType[0] = XmPUSHBUTTON;
#endif

    n = 0;
    XtSetArg(args[n],XmNbuttonCount,N_FILE_MENU_ELES); n++;
    XtSetArg(args[n],XmNbuttons,buttons); n++;
    XtSetArg(args[n],XmNbuttonType,buttonType); n++;
    XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
    XtSetArg(args[n],XmNpostFromButton,FILE_BTN_POSN); n++;
    XtSetArg(args[n],XmNsimpleCallback,
      fileMenuSimpleCallback); n++;
#ifdef EXTENDED_INTERFACE
    resourceFilePDM = XmCreateSimplePulldownMenu(resourceMB,"resourceFilePDM",
      args,n);
#else
    XmCreateSimplePulldownMenu(resourceMB,"resourceFilePDM", args,n);
#endif
    for (i = 0; i < N_FILE_MENU_ELES; i++) XmStringFree(buttons[i]);

  /** create the bundle pulldown menu pane */
#ifdef EXTENDED_INTERFACE
    buttons[0] = XmStringCreateLocalized("Create...");
    buttons[1] = XmStringCreateLocalized("Delete");
    buttons[2] = XmStringCreateLocalized("Rename...");
    keySyms[0] = 'C';
    keySyms[1] = 'D';
    keySyms[2] = 'R';
    buttonType[0] = XmPUSHBUTTON;
    buttonType[1] = XmPUSHBUTTON;
    buttonType[2] = XmPUSHBUTTON;
    n = 0;
    XtSetArg(args[n],XmNbuttonCount,N_BUNDLE_MENU_ELES); n++;
    XtSetArg(args[n],XmNbuttons,buttons); n++;
    XtSetArg(args[n],XmNbuttonType,buttonType); n++;
    XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
    XtSetArg(args[n],XmNpostFromButton,BUNDLE_BTN_POSN); n++;
    XtSetArg(args[n],XmNsimpleCallback,
      bundleMenuSimpleCallback); n++;
    resourceBundlePDM = XmCreateSimplePulldownMenu(resourceMB,"resourceBundlePDM",
      args,n);
    for (i = 0; i < N_BUNDLE_MENU_ELES; i++) XmStringFree(buttons[i]);
#endif

    resourceHelpPDM = buildMenu(resourceMB,XmMENU_PULLDOWN,
      "Help", 'H', helpMenu);
    XtVaSetValues(resourceMB, XmNmenuHelpWidget, resourceHelpPDM, NULL);
#if 0
  /* (MDA) for now, disable this menu */
    XtSetSensitive(resourceHelpPDM,False);
#endif

#if 0
  /****** create the help pulldown menu pane */
    buttons[0] = XmStringCreateLocalized("On Resource Palette...");
    keySyms[0] = 'C';
    buttonType[0] = XmPUSHBUTTON;
    n = 0;
    XtSetArg(args[n],XmNbuttonCount,N_HELP_MENU_ELES); n++;
    XtSetArg(args[n],XmNbuttons,buttons); n++;
    XtSetArg(args[n],XmNbuttonType,buttonType); n++;
    XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
    XtSetArg(args[n],XmNpostFromButton,HELP_BTN_POSN); n++;
    resourceHelpPDM = XmCreateSimplePulldownMenu(resourceMB,
      "resourceHelpPDM",args,n);
    XmStringFree(buttons[0]);
  /* (MDA) for now, disable this menu */
    XtSetSensitive(resourceHelpPDM,False);
#endif

#ifdef UNNECESSARY
  /****** Add the resource bundle scrolled window and contents */
    n = 0;
    XtSetArg(args[n],XmNscrollingPolicy,XmAUTOMATIC); n++;
    XtSetArg(args[n],XmNscrollBarDisplayPolicy,XmAS_NEEDED); n++;
    bundlesSW = XmCreateScrolledWindow(resourceMW,"bundlesSW",args,n);
    createResourceBundles(bundlesSW);
#endif

  /****** Add the resource entry scrolled window and contents */
    n = 0;
    XtSetArg(args[n],XmNscrollingPolicy,XmAUTOMATIC); n++;
    XtSetArg(args[n],XmNscrollBarDisplayPolicy,XmAS_NEEDED); n++;
    entriesSW = XmCreateScrolledWindow(resourceMW,"entriesSW",args,n);
    createResourceEntries(entriesSW);

  /* add a message/status and dispatch area (this is clumsier than need-be,
   *	but perhaps necessary (at least for now)) */
    n = 0;
    XtSetArg(args[n],XmNtopOffset,0); n++;
    XtSetArg(args[n],XmNbottomOffset,0); n++;
    XtSetArg(args[n],XmNshadowThickness,0); n++;
    messageF = XmCreateForm(resourceMW,"messageF",args,n);
    createBundleButtons(messageF);

  /****** Manage the composites */
    XtManageChild(messageF);
    XtManageChild(resourceMB);
#ifdef UNNECESSARY
    XtManageChild(bundlesSW);
#endif
    XtManageChild(entriesSW);
    XtManageChild(resourceMW);

    XmMainWindowSetAreas(resourceMW,resourceMB,NULL,NULL,NULL,entriesSW);
#ifdef UNNECESSARY
    XtVaSetValues(resourceMW,XmNmessageWindow,messageF,XmNcommandWindow,
      bundlesSW, NULL);
#else
    XtVaSetValues(resourceMW,XmNmessageWindow,messageF,NULL);
#endif

  /****** Now popup the dialog and restore cursor */
    XtPopup(resourceS,XtGrabNone);

  /* Change drawingArea's cursor back to the appropriate cursor */
    if(cdi != NULL)
      XDefineCursor(display,XtWindow(cdi->drawingArea),
	(currentActionType == SELECT_ACTION ? rubberbandCursor: crosshairCursor));
}

/****************************************************************************
 * Create Resource Entries: Create resource entries in scrolled window      *
 ****************************************************************************/
static void createResourceEntries(Widget entriesSW)
{
    Widget entriesRC;
    Arg args[12];
    int i, n;
    short maxCols;

    n = 0;
    XtSetArg(args[n],XmNnumColumns,1); n++;
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_COLUMN); n++;
    entriesRC = XmCreateRowColumn(entriesSW,"entriesRC",args,n);

  /* Create the row-columns which are entries into overall row-column
   *   these entries are specific to resource bundle elements, and
   *   are managed/unmanaged according to the selected widgets being
   *   edited...  (see WidgetDM.h for details on this) */
    for (i = MIN_RESOURCE_ENTRY; i < MAX_RESOURCE_ENTRY; i++) {
	createEntryRC(entriesRC,i);
    }
    initializeResourcePaletteElements();

  /* Define the column width for PV names, etc. For 3.13 base this
     used to be PVNAME_STRINGSZ+FLDNAME_SZ+1=33.  For 3.14
     PVNAME_STRINGSZ is much larger and FLDNAME_SZ is undefined and
     unlimited.  Use 33 (number of characters excluding space and 1
     less than before) always. */
    maxCols=33;

  /* Resize the labels and elements (to maximum's width) for uniform appearance */
    XtSetArg(args[0],XmNwidth,maxLabelWidth);
    XtSetArg(args[1],XmNheight,maxLabelHeight);
    XtSetArg(args[2],XmNrecomputeSize,False);
    XtSetArg(args[3],XmNalignment,XmALIGNMENT_END);

    XtSetArg(args[4],XmNx,(Position)maxLabelWidth);
    XtSetArg(args[5],XmNwidth,maxLabelWidth);
    XtSetArg(args[6],XmNheight,maxLabelHeight);
    XtSetArg(args[7],XmNrecomputeSize,False);
    XtSetArg(args[8],XmNresizeWidth,False);
    XtSetArg(args[9],XmNmarginWidth,0);

    for (i = MIN_RESOURCE_ENTRY; i < MAX_RESOURCE_ENTRY; i++) {
      /* Set label */
	XtSetValues(resourceEntryLabel[i],args,4);

      /* Set element */
	if(XtClass(resourceEntryElement[i]) == xmRowColumnWidgetClass) {
	  /* must be option menu - unmanage label widget */
	    XtUnmanageChild(XmOptionLabelGadget(resourceEntryElement[i]));
	    XtSetValues(XmOptionButtonGadget(resourceEntryElement[i]),
	      &(args[4]),6);
	}
	XtSetValues(resourceEntryElement[i],&(args[4]),6);
      /* KE: Why not do this in createEntryRC */
      /* Restrict size of CA PV name entry */
	if(i == CHAN_A_RC || i == CHAN_B_RC || i == CHAN_C_RC || i == CHAN_D_RC
	  || i == RDBK_RC || i == CTRL_RC
	  || i == TRIGGER_RC || i == ERASE_RC || i == COUNT_RC) {
	  /* Since can have macro-substituted strings, need longer length */
	    XtVaSetValues(resourceEntryElement[i],
	      XmNcolumns,maxCols,
	      XmNmaxLength,(int)MAX_TOKEN_LENGTH-1,NULL);
	} else if(i == VIS_CALC_RC || i == IMAGE_CALC_RC) {
	  /* calc in calcRecord is limited to MAX_STRING_SIZE=40
             characters including NULL */
	    XtVaSetValues(resourceEntryElement[i],
	      XmNcolumns,maxCols,
	      XmNmaxLength,(int)MAX_STRING_SIZE-1,NULL);
	} else if(i == MSG_LABEL_RC || i == PRESS_MSG_RC
	  || i == RELEASE_MSG_RC || i == TEXTIX_RC
	  || i == TITLE_RC || i == XLABEL_RC || i == YLABEL_RC
	  || i == IMAGE_NAME_RC) {
	  /* Use size of CA PV name entry for these text-oriented fields */
	    XtVaSetValues(resourceEntryElement[i],
	      XmNcolumns,maxCols,NULL);
	}
    }

    XtManageChild(entriesRC);
}

/****************************************************************************
 * Create Entry RC: Create the various row-columns for each resource entry  *
 * rcType = {X_RC,Y_RC,...}.                                                *
 ****************************************************************************/
static void createEntryRC( Widget parent, int rcType)
{
    DisplayInfo *cdi = currentDisplayInfo;
    Widget localRC, localLabel, localElement;
    XmString labelString;
    Dimension width, height;
    Arg args[6];
    int n;
    static Boolean first = True;
    static XmButtonType buttonType[MAX_OPTIONS];

    if(first) {
	first = False;
	for (n = 0; n < MAX_OPTIONS; n++) {
	    buttonType[n] = XmPUSHBUTTON;
	}
    }

    n = 0;
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_NONE); n++;
    localRC = XmCreateRowColumn(parent,"entryRC",args,n);

  /****** Create the label element */
    n = 0;
    labelString = XmStringCreateLocalized(resourceEntryStringTable[rcType]);
    XtSetArg(args[n],XmNalignment,XmALIGNMENT_END); n++;
    XtSetArg(args[n],XmNlabelString,labelString); n++;
    XtSetArg(args[n],XmNrecomputeSize,False); n++;
    localLabel = XmCreateLabel(localRC,"localLabel",args,n);
    XmStringFree(labelString);

  /****** Create the selection element (text entry, option menu, etc) */
    switch(rcType) {

      /* numeric text field types */
    case X_RC:
    case Y_RC:
    case WIDTH_RC:
    case HEIGHT_RC:
    case SBIT_RC:
    case EBIT_RC:
    case DIS_RC:
    case XYANGLE_RC:
    case ZANGLE_RC:
    case PERIOD_RC:
    case LINEWIDTH_RC:
    case GRID_SPACING_RC:
	n = 0;
	XtSetArg(args[n],XmNmaxLength,MAX_TOKEN_LENGTH-1); n++;
	localElement = XmCreateTextField(localRC,"localElement",args,n);
	XtAddCallback(localElement,XmNactivateCallback,
	  textFieldActivateCallback,(XtPointer)rcType);
	XtAddCallback(localElement,XmNlosingFocusCallback,
	  textFieldLosingFocusCallback,(XtPointer)rcType);
	XtAddCallback(localElement,XmNmodifyVerifyCallback,
	  textFieldNumericVerifyCallback,(XtPointer)rcType);
	break;
    case PRECISION_RC:
	n = 0;
	XtSetArg(args[n],XmNmaxLength,MAX_TOKEN_LENGTH-1); n++;
	localElement = XmCreateTextField(localRC,"localElement",args,n);
	XtAddCallback(localElement,XmNactivateCallback,
	  textFieldActivateCallback,(XtPointer)rcType);
	XtAddCallback(localElement,XmNlosingFocusCallback,
	  textFieldLosingFocusCallback,(XtPointer)rcType);
	XtAddCallback(localElement,XmNmodifyVerifyCallback,
	  textFieldFloatVerifyCallback,(XtPointer)NULL);
	break;
      /* alpha-numeric text field types */
    case RDBK_RC:
    case CTRL_RC:
    case TITLE_RC:
    case XLABEL_RC:
    case YLABEL_RC:
    case VIS_CALC_RC:
    case CHAN_A_RC:
    case CHAN_B_RC:
    case CHAN_C_RC:
    case CHAN_D_RC:
    case TEXTIX_RC:
    case MSG_LABEL_RC:
    case PRESS_MSG_RC:
    case RELEASE_MSG_RC:
    case IMAGE_NAME_RC:
    case IMAGE_CALC_RC:
    case DATA_RC:
    case CMAP_RC:
    case NAME_RC:
    case COMPOSITE_FILE_RC:
    case TRIGGER_RC:
    case ERASE_RC:
    case COUNT_RC:
    case RD_LABEL_RC:
    case WS_FORMAT_RC:
	n = 0;
	XtSetArg(args[n],XmNmaxLength,MAX_TOKEN_LENGTH-1); n++;
	localElement = XmCreateTextField(localRC,"localElement",args,n);
	XtAddCallback(localElement,XmNactivateCallback,
	  textFieldActivateCallback,(XtPointer)rcType);
	XtAddCallback(localElement,XmNlosingFocusCallback,
	  textFieldLosingFocusCallback,(XtPointer)rcType);
	break;

      /* scale (slider) types */
    case BEGIN_RC:
    case PATH_RC:
	n = 0;
	XtSetArg(args[n],XmNminimum,0); n++;
	XtSetArg(args[n],XmNmaximum,360); n++;
	XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
	XtSetArg(args[n],XmNshowValue,True); n++;
	XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
	XtSetArg(args[n],XmNscaleMultiple,15); n++;
	localElement = XmCreateScale(localRC,"localElement",args,n);
	XtAddCallback(localElement,XmNvalueChangedCallback,
	  scaleCallback,(XtPointer)rcType);
	XtAddCallback(localElement,XmNdragCallback,
	  scaleCallback,(XtPointer)rcType);
	break;


      /* option menu types */
    case ALIGN_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,
	  &(xmStringValueTable[FIRST_TEXT_ALIGN])); n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_TEXT_ALIGNS); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;

    case FORMAT_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,
	  &(xmStringValueTable[FIRST_TEXT_FORMAT])); n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_TEXT_FORMATS); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;

    case LABEL_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,
	  &(xmStringValueTable[FIRST_LABEL_TYPE])); n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_LABEL_TYPES); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;

    case DIRECTION_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,
	  &(xmStringValueTable[FIRST_DIRECTION])); n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_DIRECTIONS); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;

    case CLRMOD_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,
	  &(xmStringValueTable[FIRST_COLOR_MODE])); n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_COLOR_MODES); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;

    case FILLMOD_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,
	  &(xmStringValueTable[FIRST_FILL_MODE])); n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_FILL_MODES); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;

    case STYLE_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,
	  &(xmStringValueTable[FIRST_EDGE_STYLE])); n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_EDGE_STYLES); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;

    case FILL_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,
	  &(xmStringValueTable[FIRST_FILL_STYLE])); n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_FILL_STYLES); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;

    case VIS_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,
	  &(xmStringValueTable[FIRST_VISIBILITY_MODE])); n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_VISIBILITY_MODES); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;

    case UNITS_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,
	  &(xmStringValueTable[FIRST_TIME_UNIT])); n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_TIME_UNITS); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;

    case CSTYLE_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,
	  &(xmStringValueTable[FIRST_CARTESIAN_PLOT_STYLE])); n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_CARTESIAN_PLOT_STYLES); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;

    case ERASE_OLDEST_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,&(xmStringValueTable[FIRST_ERASE_OLDEST]));
	n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_ERASE_OLDESTS); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;

    case ERASE_MODE_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,&(xmStringValueTable[FIRST_ERASE_MODE]));
	n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_ERASE_MODES); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;

    case STACKING_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,&(xmStringValueTable[FIRST_STACKING])); n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_STACKINGS); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;

    case IMAGE_TYPE_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,
	  &(xmStringValueTable[FIRST_IMAGE_TYPE])); n++;
      /* MDA - when TIFF is implemented:
	 XtSetArg(args[n],XmNbuttonCount,NUM_IMAGE_TYPES); n++;
      */
	XtSetArg(args[n],XmNbuttonCount,2); n++;

	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;
    case RD_VISUAL_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,&(xmStringValueTable[FIRST_RD_VISUAL]));
	n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_RD_VISUAL); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;
    case GRID_ON_RC:
    case GRID_SNAP_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,&(xmStringValueTable[FIRST_BOOLEAN]));
	n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_BOOLEAN); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;

      /* Color types */
    case CLR_RC:
    case BCLR_RC:
    case DATA_CLR_RC:
	n = 0;
	if(rcType == CLR_RC) {
	    XtSetArg(args[n],XmNbackground,
	      (cdi == NULL) ?
	      BlackPixel(display,screenNum) :
	      cdi->colormap[cdi->drawingAreaForegroundColor]); n++;
	} else {
	    XtSetArg(args[n],XmNbackground,
	      (cdi == NULL) ?
	      WhitePixel(display,screenNum) :
	      cdi->colormap[cdi->drawingAreaBackgroundColor]); n++;
	}
	XtSetArg(args[n],XmNshadowType,XmSHADOW_IN); n++;
	localElement = XmCreateDrawnButton(localRC,"localElement",args,n);
	XtAddCallback(localElement,XmNactivateCallback,
	  colorSelectCallback,(XtPointer)rcType);
	break;


    case RDDATA_RC:
    case CPDATA_RC:
    case SCDATA_RC:
    case SHELLDATA_RC:
    case CPAXIS_RC:
    case LIMITS_RC:
	n = 0;
	XtSetArg(args[n],XmNlabelString,dlXmStringMoreToComeSymbol); n++;
	XtSetArg(args[n],XmNalignment,XmALIGNMENT_CENTER); n++;
	XtSetArg(args[n],XmNrecomputeSize,False); n++;
	localElement = XmCreatePushButton(localRC,"localElement",args,n);
	XtAddCallback(localElement,XmNactivateCallback,
	  pushButtonActivateCallback,(XtPointer)rcType);
	break;

    default:
	medmPrintf(1,"\ncreateEntryRC(): Unknown rcType (%d)\n",
	  rcType);

    }

    XtVaGetValues(localLabel,XmNwidth,&width,XmNheight,&height,NULL);
    maxLabelWidth = MAX(maxLabelWidth,width);
    maxLabelHeight = MAX(maxLabelHeight,height);
    XtVaGetValues(localElement,XmNwidth,&width,XmNheight,&height,NULL);
    maxLabelWidth = MAX(maxLabelWidth,width);
    maxLabelHeight = MAX(maxLabelHeight,height);

    XtManageChild(localLabel);
    XtManageChild(localElement);

  /* update global variables */
    resourceEntryRC[rcType] = localRC;
    resourceEntryLabel[rcType] = localLabel;
    resourceEntryElement[rcType] = localElement;

}

/* The following determines what items appear in the Resource Palette
 *   Row-Column.  The order they appear depends on the values of the xxx_RC  */
static int resourceTable[] = {
    DL_Display,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, BCLR_RC, CMAP_RC,
    GRID_SPACING_RC, GRID_ON_RC, GRID_SNAP_RC,
    -1,
    DL_ChoiceButton,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CTRL_RC, CLR_RC, BCLR_RC, CLRMOD_RC,
    STACKING_RC,
    -1,
    DL_Menu,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CTRL_RC, CLR_RC, BCLR_RC, CLRMOD_RC,
    -1,
    DL_MessageButton,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CTRL_RC, CLR_RC, BCLR_RC, MSG_LABEL_RC,
    PRESS_MSG_RC, RELEASE_MSG_RC, CLRMOD_RC,
    -1,
    DL_Valuator,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CTRL_RC, LIMITS_RC, CLR_RC, BCLR_RC,
    LABEL_RC, CLRMOD_RC, DIRECTION_RC, PRECISION_RC,
    -1,
    DL_WheelSwitch,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CTRL_RC, LIMITS_RC, CLR_RC, BCLR_RC,
    CLRMOD_RC, WS_FORMAT_RC,
    -1,
    DL_TextEntry,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CTRL_RC, LIMITS_RC, CLR_RC, BCLR_RC,
    CLRMOD_RC, FORMAT_RC,
    -1,
    DL_Meter,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, RDBK_RC, LIMITS_RC, CLR_RC, BCLR_RC,
    LABEL_RC, CLRMOD_RC,
    -1,
    DL_TextUpdate,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, RDBK_RC, LIMITS_RC, CLR_RC, BCLR_RC,
    CLRMOD_RC, ALIGN_RC, FORMAT_RC,
    -1,
    DL_Bar,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, RDBK_RC, LIMITS_RC, CLR_RC, BCLR_RC,
    LABEL_RC, CLRMOD_RC, DIRECTION_RC, FILLMOD_RC,
    -1,
    DL_Byte,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, RDBK_RC, CLR_RC, BCLR_RC, SBIT_RC,
    EBIT_RC, CLRMOD_RC, DIRECTION_RC,
    -1,
    DL_Indicator,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, RDBK_RC, LIMITS_RC, CLR_RC, BCLR_RC,
    LABEL_RC, CLRMOD_RC, DIRECTION_RC,
    -1,
    DL_StripChart,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, TITLE_RC, XLABEL_RC, YLABEL_RC, CLR_RC,
    BCLR_RC, PERIOD_RC, UNITS_RC, SCDATA_RC,
    -1,
    DL_CartesianPlot,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, TITLE_RC, XLABEL_RC, YLABEL_RC, CLR_RC,
    BCLR_RC, CSTYLE_RC, ERASE_OLDEST_RC, COUNT_RC, CPDATA_RC, CPAXIS_RC,
    TRIGGER_RC, ERASE_RC, ERASE_MODE_RC,
    -1,
    DL_Rectangle,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, STYLE_RC, FILL_RC, LINEWIDTH_RC,
    CLRMOD_RC, VIS_RC, VIS_CALC_RC, CHAN_A_RC, CHAN_B_RC, CHAN_C_RC, CHAN_D_RC,
    -1,
    DL_Oval,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, STYLE_RC, FILL_RC, LINEWIDTH_RC,
    CLRMOD_RC, VIS_RC, VIS_CALC_RC, CHAN_A_RC, CHAN_B_RC, CHAN_C_RC, CHAN_D_RC,
    -1,
    DL_Arc,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, BEGIN_RC, PATH_RC, CLR_RC, STYLE_RC,
    FILL_RC, LINEWIDTH_RC, CLRMOD_RC,
    CLRMOD_RC, VIS_RC, VIS_CALC_RC, CHAN_A_RC, CHAN_B_RC, CHAN_C_RC, CHAN_D_RC,
    -1,
    DL_Text,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, TEXTIX_RC, ALIGN_RC, CLR_RC,
    CLRMOD_RC, VIS_RC, VIS_CALC_RC, CHAN_A_RC, CHAN_B_RC,
    CHAN_C_RC, CHAN_D_RC,
    -1,
    DL_RelatedDisplay,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, BCLR_RC,
    RD_LABEL_RC, RD_VISUAL_RC, RDDATA_RC,
    -1,
    DL_ShellCommand,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, BCLR_RC,
    RD_LABEL_RC, SHELLDATA_RC,
    -1,
    DL_Image,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, IMAGE_TYPE_RC, IMAGE_NAME_RC, IMAGE_CALC_RC,
    VIS_RC, VIS_CALC_RC, CHAN_A_RC, CHAN_B_RC, CHAN_C_RC, CHAN_D_RC,
    -1,
    DL_Composite,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, BCLR_RC, COMPOSITE_FILE_RC,
    VIS_RC, VIS_CALC_RC, CHAN_A_RC, CHAN_B_RC, CHAN_C_RC, CHAN_D_RC,
    -1,
    DL_Line,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, STYLE_RC, LINEWIDTH_RC,
    CLRMOD_RC, VIS_RC, VIS_CALC_RC, CHAN_A_RC, CHAN_B_RC, CHAN_C_RC, CHAN_D_RC,
    -1,
    DL_Polyline,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, STYLE_RC, LINEWIDTH_RC,
    CLRMOD_RC, VIS_RC, VIS_CALC_RC, CHAN_A_RC, CHAN_B_RC, CHAN_C_RC, CHAN_D_RC,
    -1,
    DL_Polygon,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, STYLE_RC, FILL_RC, LINEWIDTH_RC,
    CLRMOD_RC, VIS_RC, VIS_CALC_RC, CHAN_A_RC, CHAN_B_RC, CHAN_C_RC, CHAN_D_RC,
    -1,
};

static void initializeResourcePaletteElements()
{
    int i, j, index;
    int tableSize = sizeof(resourceTable)/sizeof(int);

    index = -1;
    for (i=0; i<tableSize; i++) {
	if(index < 0) {
	  /* Start a new element, get the new index */
	    index = resourceTable[i] - MIN_DL_ELEMENT_TYPE;
	    j = 0;
	} else {
	    if(resourceTable[i] >= 0) {
	      /* Copy RC resource from resourceTable until it reaches -1 */
		resourcePaletteElements[index].childIndexRC[j] = resourceTable[i];
		resourcePaletteElements[index].children[j] =
		  resourceEntryRC[resourceTable[i]];
		j++;
	    } else {
		int k;
	      /* Reset the index, fill the rest with zeros */
		for (k = j; k < MAX_RESOURCES_FOR_DL_ELEMENT; k++) {
		    resourcePaletteElements[index].childIndexRC[k] = 0;
		    resourcePaletteElements[index].children[k] = NULL;
		}
		resourcePaletteElements[index].numChildren = j;
		index = -1;
	    }
	}
    }
}

#ifdef UNNECESSARY
/****************************************************************************
 * Create Resource Bundles : Create resource bundles in scrolled window.    *
 ****************************************************************************/
static void createResourceBundles(Widget bundlesSW)
{
    Arg args[10];
    int n;

    n = 0;
    bundlesRB = XmCreateRadioBox(bundlesSW,"bundlesRB",args,n);

  /****** create the default bundle (Current) */
    createBundleTB(bundlesRB,"Current");
    XtManageChild(bundlesRB);
}

/****************************************************************************
 * Create Bundle Bundles : Create resource bundles in scrolled window       *
 ****************************************************************************/
static void createBundleTB(Widget bundlesRB, char *name)
{
    Widget bundlesTB;
    Arg args[10];
    int n;
    XmString xmString;

    n = 0;
    xmString = XmStringCreateLocalized(name);
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    if(resourceBundleCounter == SELECTION_BUNDLE) {
	XtSetArg(args[n],XmNset,True); n++;
    }
    bundlesTB = XmCreateToggleButton(bundlesRB,"bundlesTB",args,n);

#ifdef EXTENDED_INTERFACE
    XtAddCallback(bundlesTB,XmNvalueChangedCallback,
      bundleCallback,(XtPointer)resourceBundleCounter);
#endif

    resourceBundleCounter++;
    XmStringFree(xmString);

    XtManageChild(bundlesTB);
}
#endif

void medmGetValues(ResourceBundle *pRB, ...)
{
    va_list ap;
    int arg;
    va_start(ap, pRB);
    arg = va_arg(ap,int);
    while(arg >= 0) {
	switch(arg) {
	case X_RC: {
	  /* KE: Position, not int ??? */
	  /*   (Check others ? */
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->x;
	    break;
	}
	case Y_RC: {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->y;
	    break;
	}
	case WIDTH_RC: {
	    unsigned int *pvalue = va_arg(ap,unsigned int *);
	    *pvalue = pRB->width;
	    break;
	}
	case HEIGHT_RC: {
	    unsigned int *pvalue = va_arg(ap,unsigned int *);
	    *pvalue = pRB->height;
	    break;
	}
	case RDBK_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->chan[0]);
	    break;
	}
	case CTRL_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->chan[0]);
	    break;
	}
	case LIMITS_RC: {
	    DlLimits *plimits = va_arg(ap,DlLimits *);
	    *plimits = pRB->limits;
	    break;
	}
	case TITLE_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->title);
	    break;
	}
	case XLABEL_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->xlabel);
	    break;
	}
	case YLABEL_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->ylabel);
	    break;
	}
	case CLR_RC: {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->clr;
	    break;
	}
	case BCLR_RC: {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->bclr;
	    break;
	}
	case BEGIN_RC: {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->begin;
	    break;
	}
	case PATH_RC: {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->path;
	    break;
	}
	case ALIGN_RC: {
	    TextAlign *pvalue = va_arg(ap,TextAlign *);
	    *pvalue = pRB->align;
	    break;
	}
	case FORMAT_RC: {
	    TextFormat *pvalue = va_arg(ap,TextFormat *);
	    *pvalue = pRB->format;
	    break;
	}
	case LABEL_RC: {
	    LabelType *pvalue = va_arg(ap,LabelType *);
	    *pvalue = pRB->label;
	    break;
	}
	case DIRECTION_RC: {
	    Direction *pvalue = va_arg(ap,Direction *);
	    *pvalue = pRB->direction;
	    break;
	}
	case FILLMOD_RC: {
	    FillMode *pvalue = va_arg(ap,FillMode *);
	    *pvalue = pRB->fillmod;
	    break;
	}
	case STYLE_RC: {
	    EdgeStyle *pvalue = va_arg(ap,EdgeStyle *);
	    *pvalue = pRB->style;
	    break;
	}
	case FILL_RC: {
	    FillStyle *pvalue = va_arg(ap,FillStyle *);
	    *pvalue = pRB->fill;
	    break;
	}
	case CLRMOD_RC: {
	    ColorMode *pvalue = va_arg(ap,ColorMode *);
	    *pvalue = pRB->clrmod;
	    break;
	}
	case VIS_RC: {
	    VisibilityMode *pvalue = va_arg(ap,VisibilityMode *);
	    *pvalue = pRB->vis;
	    break;
	}
	case VIS_CALC_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->visCalc);
	    break;
	}
	case CHAN_A_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->chan[0]);
	    break;
	}
	case CHAN_B_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->chan[1]);
	    break;
	}
	case CHAN_C_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->chan[2]);
	    break;
	}
	case CHAN_D_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->chan[3]);
	    break;
	}
	case DATA_CLR_RC: {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->data_clr;
	    break;
	}
	case DIS_RC: {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->dis;
	    break;
	}
	case XYANGLE_RC: {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->xyangle;
	    break;
	}
	case ZANGLE_RC: {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->zangle;
	    break;
	}
	case PERIOD_RC: {
	    double *pvalue = va_arg(ap,double *);
	    *pvalue = pRB->period;
	    break;
	}
	case UNITS_RC: {
	    TimeUnits *pvalue = va_arg(ap,TimeUnits *);
	    *pvalue = pRB->units;
	    break;
	}
	case CSTYLE_RC: {
	    CartesianPlotStyle *pvalue = va_arg(ap,CartesianPlotStyle *);
	    *pvalue = pRB->cStyle;
	    break;
	}
	case ERASE_OLDEST_RC: {
	    EraseOldest *pvalue = va_arg(ap,EraseOldest *);
	    *pvalue = pRB->erase_oldest;
	    break;
	}
	case COUNT_RC: {
	    char* pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->countPvName);
	    break;
	}
	case STACKING_RC: {
	    Stacking *pvalue = va_arg(ap,Stacking *);
	    *pvalue = pRB->stacking;
	    break;
	}
	case IMAGE_TYPE_RC: {
	    ImageType *pvalue = va_arg(ap,ImageType *);
	    *pvalue = pRB->imageType;
	    break;
	}
	case TEXTIX_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->textix);
	    break;
	}
	case MSG_LABEL_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->messageLabel);
	    break;
	}
	case PRESS_MSG_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->press_msg);
	    break;
	}
	case RELEASE_MSG_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->release_msg);
	    break;
	}
	case IMAGE_NAME_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->imageName);
	    break;
	}
	case IMAGE_CALC_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->imageCalc);
	    break;
	}
	case DATA_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->data);
	    break;
	}
	case CMAP_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->cmap);
	    break;
	}
	case COMPOSITE_FILE_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->compositeFile);
	    break;
	}
	case NAME_RC: {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->name);
	    break;
	}
	case LINEWIDTH_RC: {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->lineWidth;
	    break;
	}
	case PRECISION_RC: {
	    double *pvalue = va_arg(ap,double *);
	    *pvalue = pRB->dPrecision;
	    break;
	}
	case SBIT_RC: {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->sbit;
	    break;
	}
	case EBIT_RC: {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->ebit;
	    break;
	}
	case RD_LABEL_RC: {
	    char* pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->rdLabel);
	    break;
	}
	case RD_VISUAL_RC: {
	    relatedDisplayVisual_t *pvalue = va_arg(ap,relatedDisplayVisual_t *);
	    *pvalue = pRB->rdVisual;
	    break;
	}
	case WS_FORMAT_RC: {
	    char* pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->wsFormat);
	    break;
	}
	case RDDATA_RC: {
	    DlRelatedDisplayEntry *pDisplay = va_arg(ap,DlRelatedDisplayEntry *);
	    int i;
	    for (i = 0; i < MAX_RELATED_DISPLAYS; i++){
		strcpy(pDisplay[i].label,pRB->rdData[i].label);
		strcpy(pDisplay[i].name,pRB->rdData[i].name);
		strcpy(pDisplay[i].args,pRB->rdData[i].args);
		pDisplay[i].mode = pRB->rdData[i].mode;
	    }
	    break;
	}
	case CPDATA_RC: {
	    DlTrace* ptrace = va_arg(ap,DlTrace *);
	    int i;
	    for (i = 0; i < MAX_TRACES; i++){
		strcpy(ptrace[i].xdata,pRB->cpData[i].xdata);
		strcpy(ptrace[i].ydata,pRB->cpData[i].ydata);
		ptrace[i].data_clr = pRB->cpData[i].data_clr;
	    }
	    break;
	}
	case SCDATA_RC: {
	    DlPen *pPen = va_arg(ap, DlPen *);
	    int i;
	    for (i = 0; i < MAX_PENS; i++){
		strcpy(pPen[i].chan,pRB->scData[i].chan);
		pPen[i].clr = pRB->scData[i].clr;
		pPen[i].limits = pRB->scData[i].limits;
	    }
	    break;
	}
	case SHELLDATA_RC: {
	    DlShellCommandEntry *pCommand = va_arg(ap, DlShellCommandEntry *);
	    int i;
	    for (i = 0; i < MAX_SHELL_COMMANDS; i++){
		strcpy(pCommand[i].label,pRB->cmdData[i].label);
		strcpy(pCommand[i].command,pRB->cmdData[i].command);
		strcpy(pCommand[i].args,pRB->cmdData[i].args);
	    }
	    break;
	}
	case CPAXIS_RC: {
	    DlPlotAxisDefinition *paxis = va_arg(ap,DlPlotAxisDefinition *);
	    paxis[X_AXIS_ELEMENT] = pRB->axis[X_AXIS_ELEMENT];
	    paxis[Y1_AXIS_ELEMENT] = pRB->axis[Y1_AXIS_ELEMENT];
	    paxis[Y2_AXIS_ELEMENT] = pRB->axis[Y2_AXIS_ELEMENT];
	    break;
	}
	case TRIGGER_RC: {
	    char* pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->trigger);
	    break;
	}
	case ERASE_RC: {
	    char* pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->erase);
	    break;
	}
	case ERASE_MODE_RC: {
	    eraseMode_t *pvalue = va_arg(ap,eraseMode_t *);
	    *pvalue = pRB->eraseMode;
	    break;
	}
	case GRID_SPACING_RC: {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->gridSpacing;
	    break;
	}
	case GRID_ON_RC: {
	    Boolean *pvalue = va_arg(ap,Boolean *);
	    *pvalue = pRB->gridOn;
	    break;
	}
	case GRID_SNAP_RC: {
	    Boolean *pvalue = va_arg(ap,Boolean *);
	    *pvalue = pRB->snapToGrid;
	    break;
	}
	default:
	    break;
	}
	arg = va_arg(ap,int);
    }
    va_end(ap);
    return;
}

static void helpResourceCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    int buttonNumber = (int)cd;

    UNREFERENCED(w);
    UNREFERENCED(cbs);

    switch(buttonNumber) {
    case HELP_RESOURCE_PALETTE_BTN:
	callBrowser(medmHelpPath,"#ResourcePalette");
	break;
    }
}

void updateGlobalResourceBundleObjectAttribute(DlObject *object)
{
    globalResourceBundle.x = object->x;
    globalResourceBundle.y = object->y;
    globalResourceBundle.width = object->width;
    globalResourceBundle.height= object->height;
}

void updateGlobalResourceBundleLimitsAttribute(DlLimits *limits)
{
    globalResourceBundle.limits = *limits;
}

void updateElementObjectAttribute(DlObject *object)
{
    object->x = globalResourceBundle.x;
    object->y = globalResourceBundle.y;
    object->width = globalResourceBundle.width;
    object->height = globalResourceBundle.height;
}

void updateResourcePaletteObjectAttribute()
{
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

void updateGlobalResourceBundleBasicAttribute(DlBasicAttribute *attr)
{
    globalResourceBundle.clr = attr->clr;
    globalResourceBundle.style = attr->style;
    globalResourceBundle.fill = attr->fill;
    globalResourceBundle.lineWidth = attr->width;
}

void updateElementBasicAttribute(DlBasicAttribute *attr)
{
    attr->clr = globalResourceBundle.clr;
    attr->style = globalResourceBundle.style;
    attr->fill = globalResourceBundle.fill;
    attr->width = globalResourceBundle.lineWidth;
}

void updateResourcePaletteBasicAttribute()
{
    DisplayInfo *cdi = currentDisplayInfo;
    char string[MAX_TOKEN_LENGTH];

    XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
      cdi->colormap[globalResourceBundle.clr],NULL);
    optionMenuSet(resourceEntryElement[STYLE_RC],
      globalResourceBundle.style - FIRST_EDGE_STYLE);
    optionMenuSet(resourceEntryElement[FILL_RC],
      globalResourceBundle.fill - FIRST_FILL_STYLE);
    sprintf(string,"%d",globalResourceBundle.lineWidth);
    XmTextFieldSetString(resourceEntryElement[LINEWIDTH_RC],string);
}

void updateGlobalResourceBundleDynamicAttribute(DlDynamicAttribute *dynAttr)
{
    int i;

    globalResourceBundle.clrmod = dynAttr->clr;
    globalResourceBundle.vis = dynAttr->vis;
    strcpy(globalResourceBundle.visCalc,dynAttr->calc);
    for(i=0; i < MAX_CALC_RECORDS; i++) {
	strcpy(globalResourceBundle.chan[i],dynAttr->chan[i]);
    }
}

void updateElementDynamicAttribute(DlDynamicAttribute *dynAttr)
{
    int i;

    dynAttr->clr = globalResourceBundle.clrmod;
    dynAttr->vis = globalResourceBundle.vis;
    strcpy(dynAttr->calc,globalResourceBundle.visCalc);
    for(i=0; i < MAX_CALC_RECORDS; i++) {
	strcpy(dynAttr->chan[i],globalResourceBundle.chan[i]);
    }
}

void updateResourcePaletteDynamicAttribute()
{
    optionMenuSet(resourceEntryElement[CLRMOD_RC],
      globalResourceBundle.clrmod - FIRST_COLOR_MODE);
    optionMenuSet(resourceEntryElement[VIS_RC],
      globalResourceBundle.vis - FIRST_VISIBILITY_MODE);
    XmTextFieldSetString(resourceEntryElement[VIS_CALC_RC],
      globalResourceBundle.visCalc);
    XmTextFieldSetString(resourceEntryElement[CHAN_A_RC],
      globalResourceBundle.chan[0]);
    XmTextFieldSetString(resourceEntryElement[CHAN_B_RC],
      globalResourceBundle.chan[1]);
    XmTextFieldSetString(resourceEntryElement[CHAN_C_RC],
      globalResourceBundle.chan[2]);
    XmTextFieldSetString(resourceEntryElement[CHAN_D_RC],
      globalResourceBundle.chan[3]);
    if(globalResourceBundle.chan[0][0] != '\0') {
	XtSetSensitive(resourceEntryRC[CLRMOD_RC],True);
	XtSetSensitive(resourceEntryRC[VIS_RC],True);
	XtSetSensitive(resourceEntryRC[VIS_CALC_RC],True);
    } else {
	XtSetSensitive(resourceEntryRC[CLRMOD_RC],False);
	XtSetSensitive(resourceEntryRC[VIS_RC],False);
	XtSetSensitive(resourceEntryRC[VIS_CALC_RC],False);
    }
}

void updateGlobalResourceBundleControlAttribute(DlControl *control)
{
    strcpy(globalResourceBundle.chan[0], control->ctrl);
    globalResourceBundle.clr = control->clr;
    globalResourceBundle.bclr = control->bclr;
}

void updateElementControlAttribute(DlControl *control)
{
    strcpy(control->ctrl, globalResourceBundle.chan[0]);
    control->clr = globalResourceBundle.clr;
    control->bclr = globalResourceBundle.bclr;
}

void updateResourcePaletteControlAttribute()
{
    DisplayInfo *cdi = currentDisplayInfo;

    XmTextFieldSetString(resourceEntryElement[CTRL_RC],
      globalResourceBundle.chan[0]);
    XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
      cdi->colormap[globalResourceBundle.clr],NULL);
    XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
      cdi->colormap[globalResourceBundle.bclr],NULL);
}

void updateGlobalResourceBundleMonitorAttribute(DlMonitor *monitor)
{
    strcpy(globalResourceBundle.chan[0], monitor->rdbk);
    globalResourceBundle.clr = monitor->clr;
    globalResourceBundle.bclr = monitor->bclr;
}

void updateElementMonitorAttribute(DlMonitor *monitor)
{
    strcpy(monitor->rdbk, globalResourceBundle.chan[0]);
    monitor->clr = globalResourceBundle.clr;
    monitor->bclr = globalResourceBundle.bclr;
}

void updateResourcePaletteMonitorAttribute()
{
    DisplayInfo *cdi = currentDisplayInfo;

    XmTextFieldSetString(resourceEntryElement[RDBK_RC],
      globalResourceBundle.chan[0]);
    XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
      cdi->colormap[globalResourceBundle.clr],NULL);
    XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
      cdi->colormap[globalResourceBundle.bclr],NULL);
}

/* Clear all the entries in the resource palette */
void clearResourcePaletteEntries()
{
#if DEBUG_RESOURCE
    print("In clearResourcePaletteEntries\n");
    if(currentElementType >= MIN_DL_ELEMENT_TYPE &&
      currentElementType <= MAX_DL_ELEMENT_TYPE) {
	print("  currentElementType: %s (%d)\n",
	  elementType(currentElementType),currentElementType);
    } else {
	print("  currentElementType: (%d) Valid Types [%d-%d]\n",
	  currentElementType,MIN_DL_ELEMENT_TYPE,MAX_DL_ELEMENT_TYPE);
    }
#endif

  /* If no resource palette yet, simply return */
    if(!resourceMW) return;

  /* Popdown any of the associated shells */
    if(relatedDisplayS)    XtPopdown(relatedDisplayS);
    if(shellCommandS)      XtPopdown(shellCommandS);
    if(cartesianPlotS)     XtPopdown(cartesianPlotS);
    if(cartesianPlotAxisS) {
	executeTimeCartesianPlotWidget = NULL;
	  XtPopdown(cartesianPlotAxisS);
    }
    if(pvLimitsS) {
	executeTimePvLimitsElement = NULL;
	XtPopdown(pvLimitsS);
    }
    if(stripChartS) {
	executeTimeStripChartElement = NULL;
	XtPopdown(stripChartS);
    }

  /* Unset the current button and set label in resourceMW to Select... */
    XtVaSetValues(resourceElementTypeLabel,XmNlabelString,xmstringSelect,NULL);

  /* Unmanage items in resource palette */
    if(currentElementType >= MIN_DL_ELEMENT_TYPE &&
      currentElementType <= MAX_DL_ELEMENT_TYPE) {
	int i = currentElementType-MIN_DL_ELEMENT_TYPE;
	XtUnmanageChildren(
	  resourcePaletteElements[i].children,
	  resourcePaletteElements[i].numChildren);
    }
}

/* Set entries in the resource palette based on current type */
void setResourcePaletteEntries()
{
    DisplayInfo *cdi = currentDisplayInfo;
    Boolean objectDataOnly;
    DlElementType displayType;

#if DEBUG_RESOURCE
    print("In setResourcePaletteEntries\n");
    if(currentElementType >= MIN_DL_ELEMENT_TYPE &&
      currentElementType <= MAX_DL_ELEMENT_TYPE) {
	print("  currentElementType: %s (%d)\n",
	  elementType(currentElementType),currentElementType);
    } else {
	print("  currentElementType: (%d) Valid Types [%d-%d]\n",
	  currentElementType,MIN_DL_ELEMENT_TYPE,MAX_DL_ELEMENT_TYPE);
    }
#endif

  /* If no resource palette yet, create it */
    if(!resourceMW) createResource();

  /* Make sure the resource palette shell is popped-up */
    XtPopup(resourceS,XtGrabNone);

  /* Check if this is a valid element type */
    if(currentElementType < MIN_DL_ELEMENT_TYPE ||
      currentElementType > MAX_DL_ELEMENT_TYPE ||
      IsEmpty(cdi->selectedDlElementList)) {
	clearResourcePaletteEntries();
/* 	resetGlobalResourceBundleAndResourcePalette(); */
	return;
    }

  /* Make these sensitive by default in case they are managed */
    XtSetSensitive(resourceEntryRC[VIS_RC],True);
    XtSetSensitive(resourceEntryRC[VIS_CALC_RC],False);
    XtSetSensitive(resourceEntryRC[CLRMOD_RC],True);

  /* Manage new resource entries */
    XtManageChildren(
      resourcePaletteElements[currentElementType -
	MIN_DL_ELEMENT_TYPE].children,
      resourcePaletteElements[currentElementType -
	MIN_DL_ELEMENT_TYPE].numChildren);

  /* If polyline with 2 points display Line as label, not Polyline */
    displayType = currentElementType;
    if((cdi->selectedDlElementList->count == 1) &&
      (currentElementType == DL_Polyline) &&
      (FirstDlElement(cdi->selectedDlElementList)->
	structure.element->structure.polyline->nPoints == 2))
      displayType = DL_Line;
    XtVaSetValues(resourceElementTypeLabel,
      XmNlabelString,elementXmStringTable[displayType-MIN_DL_ELEMENT_TYPE],
      NULL);

  /* Update all resource palette parameters */
    objectDataOnly = False;
    updateGlobalResourceBundleAndResourcePalette(objectDataOnly);

  /* If not a monitor, controller, or dynamic attribute channel, then
   * insensitize the related entries */
    if(globalResourceBundle.chan[0][0] == '\0') {
	XtSetSensitive(resourceEntryRC[VIS_RC],False);
	XtSetSensitive(resourceEntryRC[VIS_CALC_RC],False);
	if((!ELEMENT_HAS_WIDGET(currentElementType)) &&
	  (currentElementType != DL_TextUpdate))
	  XtSetSensitive(resourceEntryRC[CLRMOD_RC],False);
    }

  /* Make these sensitive in case they are managed */
    if(strlen(globalResourceBundle.erase) == 0)
      XtSetSensitive(resourceEntryRC[ERASE_MODE_RC],False);
    else
      XtSetSensitive(resourceEntryRC[ERASE_MODE_RC],True);
}

void updateElementFromGlobalResourceBundle(DlElement *element)
{
    DisplayInfo *cdi = currentDisplayInfo;

#if DEBUG_RESOURCE
    print("In updateElementFromGlobalResourceBundle\n");
#endif

  /* Simply return if not valid to update */
    if(!element || !cdi) return;

  /* Copy (all) values from resource palette to element */
    if(element->run->getValues) {
	element->run->getValues(&globalResourceBundle,element);
    }
    if(element->widget) {
      /* Need to destroy, then create to get it right */
	destroyElementWidgets(element);
	(element->run->execute)(cdi,element);
    } else if(element->type == DL_Display) {
      /* Need to execute the display though it doesn't have a widget
       *   (Is at least necessary to resize the Pixmap) */
	(element->run->execute)(cdi,element);
    }
}

void updateElementBackgroundColorFromGlobalResourceBundle(DlElement *element)
{
    DlElement *childE;
    DisplayInfo *cdi = currentDisplayInfo;

#if DEBUG_RESOURCE
    print("In updateElementBackgroundColorFromGlobalResourceBundle\n");
#endif

  /* Simply return if not valid to update */
    if(!element || !cdi) return;

  /* Check if composite */
    if(element->type == DL_Composite) {
      /* Composite, loop over contained elements */
	DlComposite *compE = element->structure.composite;

	childE = FirstDlElement(compE->dlElementList);
	while(childE) {
	    if(childE->run->setBackgroundColor) {
		childE->run->setBackgroundColor(&globalResourceBundle,childE);
	    }
	    if(childE->widget) {
		XtVaSetValues(childE->widget,XmNbackground,
		  currentColormap[globalResourceBundle.bclr],NULL);
	      /* Need to destroy, then create to get it right */
		destroyElementWidgets(childE);
		(childE->run->execute)(cdi,childE);
	    }
	  /* If drawingArea: update drawingAreaForegroundColor */
	    if(childE->type == DL_Display) {
		cdi->drawingAreaBackgroundColor = globalResourceBundle.bclr;
	    }
	    childE = childE->next;
	}
    } else {
      /* Not composite */
	if(element->run->setBackgroundColor) {
	    element->run->setBackgroundColor(&globalResourceBundle,element);
	}
	if(element->widget) {
	    XtVaSetValues(element->widget,XmNbackground,
	      currentColormap[globalResourceBundle.bclr],NULL);
	  /* Need to destroy, then create to get it right */
	    destroyElementWidgets(element);
	    (element->run->execute)(cdi,element);
	}
      /* If drawingArea: update drawingAreaForegroundColor */
	if(element->type == DL_Display) {
	    cdi->drawingAreaBackgroundColor = globalResourceBundle.bclr;
	}
    }
}

void updateElementForegroundColorFromGlobalResourceBundle(DlElement *element)
{
    DlElement *childE;
    DisplayInfo *cdi = currentDisplayInfo;

#if DEBUG_RESOURCE
    print("In updateElementForegroundColorFromGlobalResourceBundle\n");
#endif

  /* Simply return if not valid to update */
    if(!element || !cdi) return;

  /* Check if composite */
    if(element->type == DL_Composite) {
      /* Composite, loop over contained elements */
	DlComposite *compE = element->structure.composite;

	childE = FirstDlElement(compE->dlElementList);
	while(childE) {
	    if(childE->run->setForegroundColor) {
		childE->run->setForegroundColor(&globalResourceBundle,childE);
	    }
	    if(childE->widget) {
		XtVaSetValues(childE->widget,XmNforeground,
		  currentColormap[globalResourceBundle.clr],NULL);
	      /* Need to destroy, then create to get it right */
		destroyElementWidgets(childE);
		(childE->run->execute)(cdi,childE);
	    }
	  /* If drawingArea: update drawingAreaForegroundColor */
	    if(childE->type == DL_Display) {
		cdi->drawingAreaForegroundColor = globalResourceBundle.clr;
	    }
	    childE = childE->next;
	}
    } else {
      /* Not composite */
	if(element->run->setForegroundColor) {
	    element->run->setForegroundColor(&globalResourceBundle,element);
	}
	if(element->widget) {
	    XtVaSetValues(element->widget,XmNforeground,
	      currentColormap[globalResourceBundle.clr],NULL);
	  /* Need to destroy, then create to get it right */
	    destroyElementWidgets(element);
	    (element->run->execute)(cdi,element);
	  /* If drawingArea: update drawingAreaForegroundColor */
	    if(element->type == DL_Display) {
		cdi->drawingAreaForegroundColor = globalResourceBundle.clr;
	    }
	}
    }
}

/* Only used for Cartesian Plot */
void updateGlobalResourceBundleFromElement(DlElement *element)
{
    DlCartesianPlot *p;
    int i;

#if DEBUG_RESOURCE
    print("In updateGlobalResourceBundleFromElement\n");
#endif

    if(!element || (element->type != DL_CartesianPlot)) return;
    p = element->structure.cartesianPlot;

    for (i = X_AXIS_ELEMENT; i <= Y2_AXIS_ELEMENT; i++) {
	globalResourceBundle.axis[i].axisStyle = p->axis[i].axisStyle;
	globalResourceBundle.axis[i].rangeStyle = p->axis[i].rangeStyle;
	globalResourceBundle.axis[i].minRange = p->axis[i].minRange;
	globalResourceBundle.axis[i].maxRange = p->axis[i].maxRange;
	if(i == X_AXIS_ELEMENT) {
	    globalResourceBundle.axis[i].timeFormat = p->axis[i].timeFormat;
	}
    }
}

/*
 * function to clear/reset the global resource bundle data structure
 *	and to put those resource values into the resourcePalette
 *	elements (for the specified element type)
 */
void updateGlobalResourceBundleAndResourcePalette(Boolean objectDataOnly)
{
    DisplayInfo *cdi = currentDisplayInfo;
    DlElement *elementPtr;
    char string[MAX_TOKEN_LENGTH];
    int i, tail;

#if DEBUG_RESOURCE
    print("In updateGlobalResourceBundleAndResourcePaletteo\n");
#endif

  /* Simply return if not valid to update */
    if(cdi->selectedDlElementList->count != 1) return;

    elementPtr = FirstDlElement(cdi->selectedDlElementList);
    elementPtr = elementPtr->structure.element;

  /* If no resource palette yet, create it */
    if(!resourceMW) {
	currentElementType = elementPtr->type;
	setResourcePaletteEntries();
	return;
    }

    switch(elementPtr->type) {
    case DL_Display: {
	DlDisplay *p = elementPtr->structure.display;
	Arg args[2];
	int nargs;
	Position x, y;

      /* Get the current values */
	nargs = 0;
	XtSetArg(args[nargs],XmNx,&x); nargs++;
	XtSetArg(args[nargs],XmNy,&y); nargs++;
	XtGetValues(cdi->shell,args,nargs);
#if DEBUG_RELATED_DISPLAY
	{
	    Position x, y;

	    XtSetArg(args[0],XmNx,&x);
	    XtSetArg(args[1],XmNy,&y);
	    XtGetValues(cdi->shell,args,2);
	    print("updateGlobalResourceBundleAndResourcePalette:"
	      " x=%d y=%d\n",x,y);
	    print("  XtIsRealized=%s XtIsManaged=%s\n",
	      XtIsRealized(cdi->shell)?"True":"False",
	      XtIsManaged(cdi->shell)?"True":"False");
	}
#endif

      /* Set the a and y attributes in case they haven't been set */
	if(p->object.x != x) {
	    p->object.x = x;
	    medmMarkDisplayBeingEdited(cdi);
	}
	if(p->object.y != y) {
	    p->object.y = y;
	    medmMarkDisplayBeingEdited(cdi);
	}

	updateGlobalResourceBundleObjectAttribute(
	  &(elementPtr->structure.display->object));
	updateResourcePaletteObjectAttribute();
	if(objectDataOnly) return;
	globalResourceBundle.clr = elementPtr->structure.display->clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
	  cdi->colormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr = elementPtr->structure.display->bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
	  cdi->colormap[globalResourceBundle.bclr],NULL);
	strcpy(globalResourceBundle.cmap,elementPtr->structure.display->cmap);
	XmTextFieldSetString(resourceEntryElement[CMAP_RC],
	  globalResourceBundle.cmap);
	globalResourceBundle.gridSpacing = p->grid.gridSpacing;
	sprintf(string,"%d",globalResourceBundle.gridSpacing);
	XmTextFieldSetString(resourceEntryElement[GRID_SPACING_RC],string);
	globalResourceBundle.gridOn = p->grid.gridOn;
	optionMenuSet(resourceEntryElement[GRID_ON_RC],
	  (int)globalResourceBundle.gridOn);
	globalResourceBundle.snapToGrid = p->grid.snapToGrid;
	optionMenuSet(resourceEntryElement[GRID_SNAP_RC],
	  (int)globalResourceBundle.snapToGrid);
	break;
    }
    case DL_Valuator: {
	DlValuator *p = elementPtr->structure.valuator;

	updateGlobalResourceBundleObjectAttribute(&(p->object));
	updateResourcePaletteObjectAttribute();
	if(objectDataOnly) return;

	updateGlobalResourceBundleControlAttribute(&(p->control));
	updateResourcePaletteControlAttribute();
	updateGlobalResourceBundleLimitsAttribute(&(p->limits));
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
	while(string[--tail] == '0') string[tail] = '\0';
	XmTextFieldSetString(resourceEntryElement[PRECISION_RC],string);
	break;
    }
    case DL_WheelSwitch: {
	DlWheelSwitch *p = elementPtr->structure.wheelSwitch;

	updateGlobalResourceBundleObjectAttribute(&(p->object));
	updateResourcePaletteObjectAttribute();
	if(objectDataOnly) return;

	updateGlobalResourceBundleControlAttribute(&(p->control));
	updateResourcePaletteControlAttribute();
	updateGlobalResourceBundleLimitsAttribute(&(p->limits));
	globalResourceBundle.clrmod = p->clrmod;
	optionMenuSet(resourceEntryElement[CLRMOD_RC],
	  globalResourceBundle.clrmod - FIRST_COLOR_MODE);
	globalResourceBundle.dPrecision = p->dPrecision;
	sprintf(string,"%f",globalResourceBundle.dPrecision);
      /* strip trailing zeroes */
	tail = strlen(string);
	while(string[--tail] == '0') string[tail] = '\0';
	XmTextFieldSetString(resourceEntryElement[PRECISION_RC],string);
	strcpy(globalResourceBundle.wsFormat,p->format);
	XmTextFieldSetString(resourceEntryElement[WS_FORMAT_RC],
          globalResourceBundle.wsFormat);
	break;
    }
    case DL_ChoiceButton: {
	DlChoiceButton *p = elementPtr->structure.choiceButton;

	updateGlobalResourceBundleObjectAttribute(&(p->object));
	updateResourcePaletteObjectAttribute();
	if(objectDataOnly) return;

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
	if(objectDataOnly) return;

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
	if(objectDataOnly) return;

	updateGlobalResourceBundleControlAttribute(&(p->control));
	updateResourcePaletteControlAttribute();
	updateGlobalResourceBundleLimitsAttribute(&(p->limits));
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
	if(objectDataOnly) return;

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
	if(objectDataOnly) return;

	updateGlobalResourceBundleMonitorAttribute(&(p->monitor));
	updateResourcePaletteMonitorAttribute();
	updateGlobalResourceBundleLimitsAttribute(&(p->limits));
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
	if(objectDataOnly) return;

	updateGlobalResourceBundleMonitorAttribute(&(p->monitor));
	updateResourcePaletteMonitorAttribute();
	updateGlobalResourceBundleLimitsAttribute(&(p->limits));
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
	if(objectDataOnly) return;

	updateGlobalResourceBundleMonitorAttribute(&(p->monitor));
	updateResourcePaletteMonitorAttribute();
	updateGlobalResourceBundleLimitsAttribute(&(p->limits));
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
	if(objectDataOnly) return;

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
	if(objectDataOnly) return;

	updateGlobalResourceBundleMonitorAttribute(&(p->monitor));
	updateResourcePaletteMonitorAttribute();
	updateGlobalResourceBundleLimitsAttribute(&(p->limits));
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
	if(objectDataOnly) return;

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
	  cdi->colormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr = p->plotcom.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
	  cdi->colormap[globalResourceBundle.bclr],NULL);
	globalResourceBundle.period = p->period;
	cvtDoubleToString(globalResourceBundle.period,string,0);
	XmTextFieldSetString(resourceEntryElement[PERIOD_RC],string);
	globalResourceBundle.units = p->units;
	optionMenuSet(resourceEntryElement[UNITS_RC],
	  globalResourceBundle.units - FIRST_TIME_UNIT);
	for (i = 0; i < MAX_PENS; i++){
	    strcpy(globalResourceBundle.scData[i].chan,p->pen[i].chan);
	    globalResourceBundle.scData[i].clr = p->pen[i].clr;
	    globalResourceBundle.scData[i].limits = p->pen[i].limits;
	}
	break;
    }
    case DL_CartesianPlot: {
	DlCartesianPlot *p = elementPtr->structure.cartesianPlot;

	updateGlobalResourceBundleObjectAttribute(&(p->object));
	updateResourcePaletteObjectAttribute();
	if(objectDataOnly) return;

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
	  cdi->colormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr = p->plotcom.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
	  cdi->colormap[globalResourceBundle.bclr],NULL);
	strcpy(globalResourceBundle.countPvName, p->countPvName);
	XmTextFieldSetString(resourceEntryElement[COUNT_RC],
	  globalResourceBundle.countPvName);
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
	    globalResourceBundle.axis[i] = p->axis[i];
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
	if(objectDataOnly) return;

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
	if(objectDataOnly) return;

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
	if(objectDataOnly) return;

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

#if DEBUG_RESOURCE
        print("\n[updateGlobalResourceBundleAndResourcePalette] selectedDlElementList:\n");
        dumpDlElementList(cdi->selectedDlElementList);
#endif

#if 0
	if(objectDataOnly) {
	    updateGlobalResourceBundleObjectAttribute(&(p->object));
	    updateResourcePaletteObjectAttribute();
	} else {
	    elementPtr->setValues(&globaleResourceBundle,elementPtr);
	    updateResourceBundle(&globalResourceBundle);
	}
#else
	updateGlobalResourceBundleObjectAttribute(&(p->object));
	updateResourcePaletteObjectAttribute();
	if(objectDataOnly) return;

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
	if(objectDataOnly) return;

	globalResourceBundle.clr = p->clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
          cdi->colormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr = p->bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
          cdi->colormap[globalResourceBundle.bclr],NULL);
	strcpy(globalResourceBundle.rdLabel,p->label);
	XmTextFieldSetString(resourceEntryElement[RD_LABEL_RC],
          globalResourceBundle.rdLabel);
	globalResourceBundle.rdVisual = p->visual;
	optionMenuSet(resourceEntryElement[RD_VISUAL_RC],
          globalResourceBundle.rdVisual - FIRST_RD_VISUAL);
	for (i = 0; i < MAX_RELATED_DISPLAYS; i++){
	    strcpy(globalResourceBundle.rdData[i].label, p->display[i].label);
	    strcpy(globalResourceBundle.rdData[i].name, p->display[i].name);
	    strcpy(globalResourceBundle.rdData[i].args, p->display[i].args);
	    globalResourceBundle.rdData[i].mode = p->display[i].mode;
	  /* update the related display dialog (matrix of values) if appr. */
	    updateRelatedDisplayDataDialog();
	}
	break;
    }
    case DL_ShellCommand: {
	DlShellCommand *p = elementPtr->structure.shellCommand;

	updateGlobalResourceBundleObjectAttribute(&(p->object));
	updateResourcePaletteObjectAttribute();
	if(objectDataOnly) return;

	globalResourceBundle.clr = p->clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
	  cdi->colormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr = p->bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
	  cdi->colormap[globalResourceBundle.bclr],NULL);
	strcpy(globalResourceBundle.rdLabel,p->label);
	XmTextFieldSetString(resourceEntryElement[RD_LABEL_RC],
          globalResourceBundle.rdLabel);
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
	if(objectDataOnly) return;

	updateGlobalResourceBundleDynamicAttribute(&(p->dynAttr));
	updateResourcePaletteDynamicAttribute();
	globalResourceBundle.imageType = p->imageType;
	optionMenuSet(resourceEntryElement[IMAGE_TYPE_RC],
	  globalResourceBundle.imageType - FIRST_IMAGE_TYPE);
	strcpy(globalResourceBundle.imageName, p->imageName);
	XmTextFieldSetString(resourceEntryElement[IMAGE_NAME_RC],
	  globalResourceBundle.imageName);
	strcpy(globalResourceBundle.imageCalc, p->calc);
	XmTextFieldSetString(resourceEntryElement[IMAGE_CALC_RC],
	  globalResourceBundle.imageCalc);
	break;
    }
    case DL_Composite: {
	DlComposite *p = elementPtr->structure.composite;

	updateGlobalResourceBundleObjectAttribute(&(p->object));
	updateResourcePaletteObjectAttribute();
	if(objectDataOnly) return;

	updateGlobalResourceBundleDynamicAttribute(&(p->dynAttr));
	updateResourcePaletteDynamicAttribute();

      /* Set colors explicitly */
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
	  defaultBackground,NULL);
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
	  defaultBackground,NULL);
#if 0
      /* Need to add this entry to medmWidget.h and finish this if we
         want named groups */
	strcpy(globalResourceBundle.compositeName,p->compositeName);
#endif
	strcpy(globalResourceBundle.compositeFile, p->compositeFile);
	XmTextFieldSetString(resourceEntryElement[COMPOSITE_FILE_RC],
	  globalResourceBundle.compositeFile);
	break;
    }
    case DL_Polyline: {
	DlPolyline *p = elementPtr->structure.polyline;

	updateGlobalResourceBundleObjectAttribute(&(p->object));
	updateResourcePaletteObjectAttribute();
	if(objectDataOnly) return;

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
	if(objectDataOnly) return;

	updateGlobalResourceBundleBasicAttribute(&(p->attr));
	updateResourcePaletteBasicAttribute();
	updateGlobalResourceBundleDynamicAttribute(&(p->dynAttr));
	updateResourcePaletteDynamicAttribute();
	break;
    }
    default:
	medmPrintf(1,"\nupdateGlobalResourceBundleAndResourcePalette: "
	  "Unknown element type %d\n",
	  elementPtr->type);
	break;

    }
}

void resetGlobalResourceBundleAndResourcePalette()
{
    char string[MAX_TOKEN_LENGTH];

#if DEBUG_RESOURCE
    print("In resetGlobalResourceBundleAndResourcePalette\n");
#endif

    if(ELEMENT_IS_RENDERABLE(currentElementType) ) {

      /* Get object data: must have object entry - use rectangle type
         (arbitrary) */
	globalResourceBundle.x = 0;
	globalResourceBundle.y = 0;

      /* Special case for text - since can type to input, want to
          inherit width/height */
	if(currentElementType != DL_Text) {
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
