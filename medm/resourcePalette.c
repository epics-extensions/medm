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

/****************************************************************************
 * resourcePalette.c - Resource Palette                                     *
 * Mods: MDA - Creation                                                     *
 *       DMW - Tells resource palette which global resources Byte needs     *
 ***************************************************************************/

#define DEBUG_RESOURCE 0

#include <ctype.h>
#include "medm.h"
#include <Xm/MwmUtil.h>
#include "dbDefs.h"
#include "medmCartesianPlot.h"

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

#define CMD_APPLY_BTN	0
#define CMD_CLOSE_BTN	1

#define SC_CHANNEL_COLUMN	0
#define SC_COLOR_COLUMN		1	

#define SC_APPLY_BTN	0
#define SC_CLOSE_BTN	1

/* Function prototypes */

static void helpResourceCallback(Widget,XtPointer,XtPointer);

static menuEntry_t helpMenu[] = {
    { "On Resource Palette",  &xmPushButtonGadgetClass, 'P', NULL, NULL, NULL,
      helpResourceCallback, (XtPointer)HELP_RESOURCE_PALETTE_BTN, NULL},
    NULL,
};

#ifdef EXTENDED_INTERFACE
static Widget resourceFilePDM;
static Widget resourceBundlePDM, openFSD;
#endif
static Widget bundlesRB;
static Dimension maxLabelWidth = 0;
static Dimension maxLabelHeight = 0;

XmString xmstringSelect;

/*********************************************************************
 * STRIP CHART DATA
 *********************************************************************/
static Widget scMatrix = NULL, scForm;
static String scColumnLabels[] = {"Channel","Color",};
static int scColumnMaxLengths[] = {MAX_TOKEN_LENGTH-1,6,};
static short scColumnWidths[] = {36,6,};
static unsigned char scColumnLabelAlignments[] = {XmALIGNMENT_CENTER,
						  XmALIGNMENT_CENTER};
/* and the scCells array of strings (filled in from globalResourceBundle...) */
static String scRows[MAX_PENS][2];
static String *scCells[MAX_PENS];
static Pixel scColorRows[MAX_PENS][2];
static Pixel *scColorCells[MAX_PENS];


/*********************************************************************
 * SHELL COMMAND DATA
 *********************************************************************/
static Widget cmdMatrix = NULL, cmdForm = NULL;
static String cmdColumnLabels[] = {"Command Label","Command","Arguments",};
static int cmdColumnMaxLengths[] = {MAX_TOKEN_LENGTH-1,MAX_TOKEN_LENGTH-1,
				    MAX_TOKEN_LENGTH-1,};
static short cmdColumnWidths[] = {36,36,36,};
static unsigned char cmdColumnLabelAlignments[] = {XmALIGNMENT_CENTER,
						   XmALIGNMENT_CENTER,XmALIGNMENT_CENTER,};
/* and the cmdCells array of strings (filled in from globalResourceBundle...) */
static String cmdRows[MAX_SHELL_COMMANDS][3];
static String *cmdCells[MAX_SHELL_COMMANDS];

static void createResourceEntries(Widget entriesSW);
static void initializeResourcePaletteElements();
static void createResourceBundles(Widget bundlesSW);
static void createBundleTB(Widget bundlesRB, char *name);
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

#ifdef __cplusplus
static void pushButtonActivateCallback(Widget w, XtPointer cd, XtPointer)
#else
    static void pushButtonActivateCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int rcType = (int) cd;

    switch (rcType) {
    case RDDATA_RC:
	relatedDisplayDataDialogPopup(w);
	break;
    case SHELLDATA_RC:
	if (!shellCommandS) {
	    shellCommandS = createShellCommandDataDialog(w);
	}
      /* update shell command data from globalResourceBundle */
	updateShellCommandDataDialog();
	XtManageChild(cmdForm);
	XtPopup(shellCommandS,XtGrabNone);
	break;
    case CPDATA_RC:
	if (!cartesianPlotS) {
	    cartesianPlotS = createCartesianPlotDataDialog(w);
	}
      /* update cartesian plot data from globalResourceBundle */
	updateCartesianPlotDataDialog();
	XtManageChild(cpForm);
	XtPopup(cartesianPlotS,XtGrabNone);
	break;
    case SCDATA_RC:
	if (!stripChartS) {
	    stripChartS = createStripChartDataDialog(w);
	}
      /* update strip chart data from globalResourceBundle */
	updateStripChartDataDialog();
	XtManageChild(scForm);
	XtPopup(stripChartS,XtGrabNone);
	break;
    case CPAXIS_RC:
	if (!cartesianPlotAxisS) {
	    cartesianPlotAxisS = createCartesianPlotAxisDialog(w);
	}
      /* update cartesian plot axis data from globalResourceBundle */
	updateCartesianPlotAxisDialog();
	if (currentDisplayInfo->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(currentDisplayInfo);
	XtManageChild(cpAxisForm);
	XtPopup(cartesianPlotAxisS,XtGrabNone);
	break;
    default:
	medmPrintf("\npushButtonActivate...: invalid type = %d",rcType);
	break;
    }
}

#ifdef __cplusplus
static void optionMenuSimpleCallback(Widget w, XtPointer cd, XtPointer)
#else
    static void optionMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int buttonId = (int) cd;
    int i, rcType;
    DlElement *elementPtr;

  /****** rcType (which option menu) is stored in userData */
    XtVaGetValues(XtParent(w),XmNuserData,&rcType,NULL);
    switch (rcType) {
    case ALIGN_RC: 
	globalResourceBundle.align = (TextAlign) (FIRST_TEXT_ALIGN + buttonId);
	break;
    case FORMAT_RC: 
	globalResourceBundle.format = (TextFormat) (FIRST_TEXT_FORMAT + buttonId);
	break;
    case LABEL_RC: 
	globalResourceBundle.label = (LabelType) (FIRST_LABEL_TYPE + buttonId);
	break;
    case DIRECTION_RC: 
	globalResourceBundle.direction = (Direction) (FIRST_DIRECTION + buttonId);
	break;
#ifdef __COLOR_RULE_H__
    case CLRMOD_RC:
	globalResourceBundle.clrmod = (ColorMode) (FIRST_COLOR_MODE + buttonId);
	if (globalResourceBundle.clrmod == DISCRETE) {
	    XtSetSensitive(resourceEntryRC[COLOR_RULE_RC],True);
	} else {
	    XtSetSensitive(resourceEntryRC[COLOR_RULE_RC],False);
	}
	break;
    case COLOR_RULE_RC:
	globalResourceBundle.colorRule = buttonId;
	break;
#else
    case CLRMOD_RC: 
	globalResourceBundle.clrmod = (ColorMode) (FIRST_COLOR_MODE + buttonId);
	break;
#endif
    case FILLMOD_RC: 
	globalResourceBundle.fillmod = (FillMode) (FIRST_FILL_MODE + buttonId);
	break;
    case STYLE_RC: 
	globalResourceBundle.style = (EdgeStyle) (FIRST_EDGE_STYLE + buttonId);
	break;
    case FILL_RC: 
	globalResourceBundle.fill = (FillStyle) (FIRST_FILL_STYLE + buttonId);
	break;
    case VIS_RC: 
	globalResourceBundle.vis = (VisibilityMode) (FIRST_VISIBILITY_MODE + buttonId);
	break;
    case UNITS_RC: 
	globalResourceBundle.units = (TimeUnits) (FIRST_TIME_UNIT + buttonId);
	break;
    case CSTYLE_RC: 
	globalResourceBundle.cStyle = (CartesianPlotStyle) (FIRST_CARTESIAN_PLOT_STYLE + buttonId);
	break;
    case ERASE_OLDEST_RC:
	globalResourceBundle.erase_oldest = (EraseOldest) (FIRST_ERASE_OLDEST + buttonId);
	break;
    case STACKING_RC: 
	globalResourceBundle.stacking = (Stacking) (FIRST_STACKING + buttonId);
	break;
    case IMAGETYPE_RC: 
	globalResourceBundle.imageType = (ImageType) (FIRST_IMAGE_TYPE + buttonId);
	break;
    case ERASE_MODE_RC:
	globalResourceBundle.eraseMode = (eraseMode_t) (FIRST_ERASE_MODE + buttonId);
	break;
    case RD_VISUAL_RC :
	globalResourceBundle.rdVisual = 
	  (relatedDisplayVisual_t) (FIRST_RD_VISUAL + buttonId);
	break;

    default:
	medmPrintf("\noptionMenuSimpleCallback: unknown rcType = %d",rcType);
	break;
    }

  /****** Update elements (this is overkill, but okay for now)
	*	-- not as efficient as it should be (don't update EVERYTHING if only
	*	   one item changed!) */
    if (currentDisplayInfo) {
	DlElement *dlElement = FirstDlElement(currentDisplayInfo->selectedDlElementList);
      /****** Unhighlight (since objects may move) */
	unhighlightSelectedElements();

	while (dlElement) {
	    elementPtr = dlElement->structure.element;
	    updateElementFromGlobalResourceBundle(elementPtr);
	    dlElement = dlElement->next;
	}

	dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
	if (currentDisplayInfo->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(currentDisplayInfo);
      /* highlight */
	highlightSelectedElements();
    }
}


/** set Cartesian Plot Axis attributes
 * (complex - has to handle both EDIT and EXECUTE time interactions)
 */
#ifdef __cplusplus
static void cpAxisOptionMenuSimpleCallback(Widget w, XtPointer cd, XtPointer)
#else
    static void cpAxisOptionMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{     
    int buttonId = (int) cd;
    int i, k, n, rcType, iPrec;
    char string[24];
    DlElement *elementPtr;
    Arg args[10];
    String resourceName;
    XcVType minF, maxF, tickF;
    XtPointer userData;
    CartesianPlot *pcp = NULL;
    DlCartesianPlot *dlCartesianPlot = NULL;

  /****** Get current cartesian plot */
    if (globalDisplayListTraversalMode == DL_EXECUTE) {
	if (executeTimeCartesianPlotWidget) {
	    XtVaGetValues(executeTimeCartesianPlotWidget,
	      XmNuserData, &userData, NULL);
	    if (pcp = (CartesianPlot *) userData)
	      dlCartesianPlot = (DlCartesianPlot *) 
		pcp->dlElement->structure.cartesianPlot;
	}
    }

  /* rcType (and therefore which option menu...) is stored in userData */
    XtVaGetValues(XtParent(w),XmNuserData,&rcType,NULL);
    n = 0;
    switch (rcType) {
    case CP_X_AXIS_STYLE: 
    case CP_Y_AXIS_STYLE: 
    case CP_Y2_AXIS_STYLE: {
	CartesianPlotAxisStyle style 
	  = (CartesianPlotAxisStyle) (FIRST_CARTESIAN_PLOT_AXIS_STYLE+buttonId);
	globalResourceBundle.axis[rcType - CP_X_AXIS_STYLE].axisStyle = style;
	switch (rcType) {
	case CP_X_AXIS_STYLE :
	    if (style == TIME_AXIS) {
		XtSetSensitive(axisTimeFormat,True);
	    } else {
		XtSetArg(args[n],XtNxrtXAxisLogarithmic,
		  (style == LOG10_AXIS) ? True : False); n++;
		XtSetSensitive(axisTimeFormat,False);
	    }
	    break;
	case CP_Y_AXIS_STYLE :
	    XtSetArg(args[n],XtNxrtYAxisLogarithmic,
	      (style == LOG10_AXIS) ? True : False); n++;
	    break;
	case CP_Y2_AXIS_STYLE :
	    XtSetArg(args[n],XtNxrtY2AxisLogarithmic,
	      (style == LOG10_AXIS) ? True : False); n++;
	    break;
	}
	break;
    }
    case CP_X_RANGE_STYLE:
    case CP_Y_RANGE_STYLE:
    case CP_Y2_RANGE_STYLE:
	globalResourceBundle.axis[rcType-CP_X_RANGE_STYLE].rangeStyle
	  = (CartesianPlotRangeStyle) (FIRST_CARTESIAN_PLOT_RANGE_STYLE
	    + buttonId);
	switch(globalResourceBundle.axis[rcType%3].rangeStyle) {
	    USER_SPECIFIED_RANGE :
	      XtSetSensitive(axisRangeMinRC[rcType%3],True);
	    XtSetSensitive(axisRangeMaxRC[rcType%3],True);
	    if (globalDisplayListTraversalMode == DL_EXECUTE) {
		if (dlCartesianPlot) /* get min from element if possible */
		  minF.fval = dlCartesianPlot->axis[rcType%3].minRange;
		else
		  minF.fval = globalResourceBundle.axis[rcType%3].minRange;
		sprintf(string,"%f",minF.fval);
		XmTextFieldSetString(axisRangeMin[rcType%3],string);
		if (dlCartesianPlot) /* get max from element if possible */
		  maxF.fval = dlCartesianPlot->axis[rcType%3].maxRange;
		else
		  maxF.fval = globalResourceBundle.axis[rcType%3].maxRange;
		sprintf(string,"%f",maxF.fval);
		XmTextFieldSetString(axisRangeMax[rcType%3],string);
		tickF.fval = (maxF.fval - minF.fval)/4.0;
		sprintf(string,"%f",tickF.fval);
		k = strlen(string)-1;
		while (string[k] == '0') k--;	/* strip off trailing zeroes */
		iPrec = k;
		while (string[k] != '.' && k >= 0) k--;
		iPrec = iPrec - k;
		switch(rcType%3) {
		case X_AXIS_ELEMENT:
		    XtSetArg(args[n],XtNxrtXMin,minF.lval); n++;
		    XtSetArg(args[n],XtNxrtXMax,maxF.lval); n++;
		    XtSetArg(args[n],XtNxrtXTick,tickF.lval); n++;
		    XtSetArg(args[n],XtNxrtXNum,tickF.lval); n++;
		    XtSetArg(args[n],XtNxrtXPrecision,iPrec); n++;
		    break;
		case Y1_AXIS_ELEMENT:
		    XtSetArg(args[n],XtNxrtYMin,minF.lval); n++;
		    XtSetArg(args[n],XtNxrtYMax,maxF.lval); n++;
		    XtSetArg(args[n],XtNxrtYTick,tickF.lval); n++;
		    XtSetArg(args[n],XtNxrtYNum,tickF.lval); n++;
		    XtSetArg(args[n],XtNxrtYPrecision,iPrec); n++;
		    break;
		case Y2_AXIS_ELEMENT:
		    XtSetArg(args[n],XtNxrtY2Min,minF.lval); n++;
		    XtSetArg(args[n],XtNxrtY2Max,maxF.lval); n++;
		    XtSetArg(args[n],XtNxrtY2Tick,tickF.lval); n++;
		    XtSetArg(args[n],XtNxrtY2Num,tickF.lval); n++;
		    XtSetArg(args[n],XtNxrtY2Precision,iPrec); n++;
		    break;
		}
	    }
	    if (pcp) pcp->axisRange[rcType%3].isCurrentlyFromChannel = False;
	    break;
	    CHANNEL_X_RANGE : 
	      CHANNEL_Y_RANGE : 
	      CHANNEL_Y2_RANGE : 
	      XtSetSensitive(axisRangeMinRC[rcType%3],False);
	    XtSetSensitive(axisRangeMaxRC[rcType%3],False);
	    if (pcp) {
	      /* get channel-based range specifiers - NB: these are
	       *   different than the display element version of these
	       *   which are the user-specified values
	       */
		minF.fval = pcp->axisRange[rcType%3].axisMin;
		maxF.fval = pcp->axisRange[rcType%3].axisMax;
	    }
	    switch(rcType%3) {
	    case X_AXIS_ELEMENT:
		XtSetArg(args[n],XtNxrtXMin,minF.lval); n++;
		XtSetArg(args[n],XtNxrtXMax,maxF.lval); n++;
		XtSetArg(args[n],XtNxrtXTickUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtXNumUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtXPrecisionUseDefault,True); n++;
		break;
	    case Y1_AXIS_ELEMENT:
		XtSetArg(args[n],XtNxrtYMin,minF.lval); n++;
		XtSetArg(args[n],XtNxrtYMax,maxF.lval); n++;
		XtSetArg(args[n],XtNxrtYTickUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtYNumUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtYPrecisionUseDefault,True); n++;
		break;
	    case Y2_AXIS_ELEMENT:
		XtSetArg(args[n],XtNxrtY2Min,minF.lval); n++;
		XtSetArg(args[n],XtNxrtY2Max,maxF.lval); n++;
		XtSetArg(args[n],XtNxrtY2TickUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtY2NumUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtY2PrecisionUseDefault,True); n++;
		break;
	    }
	    if (pcp) pcp->axisRange[rcType%3].isCurrentlyFromChannel = True;
	    break;
	    AUTO_SCALE_RANGE :
	      XtSetSensitive(axisRangeMinRC[rcType%3],False);
	    XtSetSensitive(axisRangeMaxRC[rcType%3],False);
	    if (globalDisplayListTraversalMode == DL_EXECUTE) {
		switch(rcType%3) {
		case X_AXIS_ELEMENT:
		    XtSetArg(args[n],XtNxrtXMinUseDefault,True);n++;
		    XtSetArg(args[n],XtNxrtXMaxUseDefault,True);n++;
		    XtSetArg(args[n],XtNxrtXTickUseDefault,True);n++;
		    XtSetArg(args[n],XtNxrtXNumUseDefault,True);n++;
		    XtSetArg(args[n],XtNxrtXPrecisionUseDefault,True);n++;
		    break;
		case Y1_AXIS_ELEMENT:
		    XtSetArg(args[n],XtNxrtYMinUseDefault,True);n++;
		    XtSetArg(args[n],XtNxrtYMaxUseDefault,True);n++;
		    XtSetArg(args[n],XtNxrtYTickUseDefault,True);n++;
		    XtSetArg(args[n],XtNxrtYNumUseDefault,True);n++;
		    XtSetArg(args[n],XtNxrtYPrecisionUseDefault,True);n++;
		    break;
		case Y2_AXIS_ELEMENT:
		    XtSetArg(args[n],XtNxrtY2MinUseDefault,True);n++;
		    XtSetArg(args[n],XtNxrtY2MaxUseDefault,True);n++;
		    XtSetArg(args[n],XtNxrtY2TickUseDefault,True);n++;
		    XtSetArg(args[n],XtNxrtY2NumUseDefault,True);n++;
		    XtSetArg(args[n],XtNxrtY2PrecisionUseDefault,True);n++;
		    break;
		}
	    }
	    if (pcp) pcp->axisRange[rcType%3].isCurrentlyFromChannel = False;
	    break;
	    DEFAULT :
	      break;
	}
	break;
    case CP_X_TIME_FORMAT :
	globalResourceBundle.axis[0].timeFormat =
	  (CartesianPlotTimeFormat_t) (FIRST_CP_TIME_FORMAT + buttonId);
	break;
    default:
	medmPrintf("\ncpAxisptionMenuSimpleCallback: unknown rcType = %d",rcType/3);
	break;
    }

  /****** Update for EDIT or EXECUTE mode */
    switch(globalDisplayListTraversalMode) {
    case DL_EDIT:
	if (currentDisplayInfo) {
	    DlElement *dlElement = FirstDlElement(
	      currentDisplayInfo->selectedDlElementList);
	    unhighlightSelectedElements();
	    while (dlElement) {
		updateElementFromGlobalResourceBundle(dlElement->structure.element);
		dlElement = dlElement->next;
	    }
	    dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
	    highlightSelectedElements();
	}
	break;
    case DL_EXECUTE:
	if (executeTimeCartesianPlotWidget)
	  XtSetValues(executeTimeCartesianPlotWidget,args,n);
	break;
    }
}

#ifdef __cplusplus
static void colorSelectCallback(Widget, XtPointer cd, XtPointer)
#else
static void colorSelectCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{     
    int rcType = (int) cd;

    if (colorMW != NULL) {
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

#ifdef __cplusplus
static void fileMenuSimpleCallback(Widget, XtPointer cd, XtPointer)
#else
static void fileMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int buttonNumber = (int) cd;
    Arg args[10];

    switch(buttonNumber) {
#ifdef EXTENDED_INTERFACE
    case FILE_OPEN_BTN:
	if (openFSD == NULL) {
	    n = 0;
	    label = XmStringCreateSimple(RESOURCE_DIALOG_MASK);
	    XtSetArg(args[n],XmNdirMask,label); n++;
	    XtSetArg(args[n],XmNdialogStyle,
	      XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
	    openFSD = XmCreateFileSelectionDialog(resourceFilePDM,
	      "openFSD",args,n);
	  /* make Filter text field insensitive to prevent user hand-editing dirMask */
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
static void bundleMenuSimpleCallback(
  Widget w,
  int buttonNumber,
  XmAnyCallbackStruct *call_data) {
    XmString label;
    int n;
    Arg args[10];
    Widget textField;

    switch(buttonNumber) {
    }
}
#endif

/****** Text field verify callback  (verify numeric input) */
void textFieldNumericVerifyCallback(
  Widget w,
  XtPointer clientData,
  XtPointer callbackData)
{
    int rcType = (int) clientData;
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *) callbackData;
    int len;

    len = 0;
    if (cbs->startPos < cbs->currInsert) return;

    len = (int) XmTextFieldGetLastPosition(w);

  /****** If no (new) data {e.g., setting to NULL string}, simply return */
    if (cbs->text->ptr == NULL) return;

  /****** Check for leading sign (+/-) */
    if (len == 0) {	/* nothing there yet... therefore can add sign */
	if ((!isdigit(cbs->text->ptr[0]) && cbs->text->ptr[0] != '+'
	  && cbs->text->ptr[0] != '-') ||
	/****** Not a digit or +/-, move all chars down one and decrement length */
	  (!isdigit(cbs->text->ptr[0]) && ((rcType == X_RC || rcType == Y_RC)
	    && cbs->text->ptr[0] == '-')) ) {
	    int i;
	    for (i = len; (i+1) < cbs->text->length; i++)
	      cbs->text->ptr[i] = cbs->text->ptr[i+1];
	    cbs->text->length--;
	    len--;
	}
    } else {
      /****** Already a first character, therefore only digits allowed */
	for (len = 0; len < cbs->text->length; len++) {
	  /****** Not a digit - move all chars down one and decrement length */
	    if (!isdigit(cbs->text->ptr[len])) {
		int i;
		for (i = len; (i+1) < cbs->text->length; i++)
		  cbs->text->ptr[i] = cbs->text->ptr[i+1];
		cbs->text->length--;
		len--;
	    }
	}
    }
    if (cbs->text->length <= 0)
      cbs->doit = False;
}

#ifdef __cplusplus
void textFieldFloatVerifyCallback(Widget w, XtPointer, XtPointer pcbs)
#else
void textFieldFloatVerifyCallback(Widget w, XtPointer cd, XtPointer pcbs)
#endif
{
    XmTextVerifyCallbackStruct *cbs = (XmTextVerifyCallbackStruct *) pcbs;
    
    int len;
    Boolean haveDecimalPoint;
    char *textString;

    len = 0;
    if (cbs->startPos < cbs->currInsert) return;

    len = (int) XmTextFieldGetLastPosition(w);

  /* see if there is already a decimal point in the string */
    textString = XmTextFieldGetString(w);
    if (strchr(textString,(int)'.'))
      haveDecimalPoint = True;
    else
      haveDecimalPoint = False;
    XtFree(textString);

  /* odd behavior for programmatic reset - if NULL event, then programmatic
     and bypass previous determinations regarding decimal point, etc */
    if (cbs->event == NULL) {
	len = 0;
	haveDecimalPoint = False;
    }

  /* if no (new) data {e.g., setting to NULL string, simply return */
    if (cbs->text->ptr == NULL) return;

  /* check for leading sign (+/-) */
    if (len == 0) {	/* nothing there yet... therefore can add sign */
	if (!isdigit(cbs->text->ptr[0]) && cbs->text->ptr[0] != '+'
	  && cbs->text->ptr[0] != '-' && cbs->text->ptr[0] != '.') {
	  /* not a digit or +/-/.,  move all chars down one and decrement length */
	    int i;
	    for (i = len; (i+1) < cbs->text->length; i++)
	      cbs->text->ptr[i] = cbs->text->ptr[i+1];
	    cbs->text->length--;
	    len--;
	} else if (cbs->text->ptr[0] != '.') {
	    haveDecimalPoint = True;
	}
    } else {
      /* already a first character, therefore only digits or potential
       * decimal point allowed */

	for (len = 0; len < cbs->text->length; len++) {
	  /* make sure all additions are digits (or the first decimal point) */
	    if (!isdigit(cbs->text->ptr[len])) {
		if (cbs->text->ptr[len] == '.' && !haveDecimalPoint) {
		    haveDecimalPoint = True;
		} else {
		  /* not a digit (or is another decimal point)  
		     - move all chars down one and decrement length */
		    int i;
		    for (i = len; (i+1) < cbs->text->length; i++)
		      cbs->text->ptr[i] = cbs->text->ptr[i+1];
		    cbs->text->length--;
		    len--;
		}
	    }
	}
    }
    if (cbs->text->length <= 0)
      cbs->doit = False;
}

#ifdef __cplusplus
void scaleCallback(Widget, XtPointer cd, XtPointer pcbs)
#else
void scaleCallback(Widget w, XtPointer cd, XtPointer pcbs)
#endif
{
    int rcType = (int) cd;  /* the resource element type */
    XmScaleCallbackStruct *cbs = (XmScaleCallbackStruct *) pcbs;
    int i;

  /****** Show users degrees, but internally use degrees*64 as Xlib requires */
    switch(rcType) {
    case BEGIN_RC:
	globalResourceBundle.begin = 64*cbs->value;
	if (currentDisplayInfo->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(currentDisplayInfo);
	break;
    case PATH_RC:
	globalResourceBundle.path = 64*cbs->value;
	if (currentDisplayInfo->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(currentDisplayInfo);
	break;
    default:
	break;
    }

  /****** Update elements (this is overkill, but okay for now) */
    if (currentDisplayInfo != NULL) {
	DlElement *dlElement = FirstDlElement(
	  currentDisplayInfo->selectedDlElementList);
	unhighlightSelectedElements();
	while (dlElement) {
	    updateElementFromGlobalResourceBundle(dlElement->structure.element);
	    dlElement = dlElement->next;
	}
	dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
	highlightSelectedElements();
    }
}

#ifdef __cplusplus
void textFieldActivateCallback(Widget w, XtPointer cd, XtPointer)
#else
void textFieldActivateCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int rcType = (int) cd;
    char *stringValue;
    DlElement *dyn;
    int i;

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
	strcpy(globalResourceBundle.chan,stringValue);
	break;
    case CTRL_RC:
	strcpy(globalResourceBundle.chan,stringValue);
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
	break;
    case SBIT_RC: 
    {
	int value = atoi(stringValue);
	if (value >= 0 && value <= 15) {
	    globalResourceBundle.sbit = value;
	} else {
	    char tmp[32];
	    sprintf(tmp,"%d",globalResourceBundle.sbit);
	    XmTextFieldSetString(w,tmp);
	}
    }
    break;
    case EBIT_RC:
    {
	int value = atoi(stringValue);
	if (value >= 0 && value <= 15) {
	    globalResourceBundle.ebit = value;
	} else {
	    char tmp[32];
	    sprintf(tmp,"%d",globalResourceBundle.ebit);
	    XmTextFieldSetString(w,tmp);
	}
    }
    break;

  /****** Since a non-NULL string value for the dynamics channel means that VIS 
	  and CLRMOD must be visible */
    case CHAN_RC:
	strcpy(globalResourceBundle.chan,stringValue);
	if (strlen(stringValue) > (size_t) 0) {
	    XtSetSensitive(resourceEntryRC[CLRMOD_RC],True);
	    XtSetSensitive(resourceEntryRC[VIS_RC],True);
#ifdef __COLOR_RULE_H__
	    if (globalResourceBundle.clrmod == DISCRETE) {
		XtSetSensitive(resourceEntryRC[COLOR_RULE_RC],True);
	    } else {
		XtSetSensitive(resourceEntryRC[COLOR_RULE_RC],False);
	    }
#endif
	} else {
	    XtSetSensitive(resourceEntryRC[CLRMOD_RC],False);
	    XtSetSensitive(resourceEntryRC[VIS_RC],False);
#ifdef __COLOR_RULE_H__
	    XtSetSensitive(resourceEntryRC[COLOR_RULE_RC],False);
#endif
	}
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
	globalResourceBundle.count = atoi(stringValue);
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
    case IMAGENAME_RC:
	strcpy(globalResourceBundle.imageName,stringValue);
	break;
    case DATA_RC:
	strcpy(globalResourceBundle.data,stringValue);
	break;
    case CMAP_RC:
	strcpy(globalResourceBundle.cmap,stringValue);
	break;
    case PRECISION_RC:
	globalResourceBundle.dPrecision = atof(stringValue);
	break;
    case TRIGGER_RC:
	strcpy(globalResourceBundle.trigger,stringValue);
	break;
    case ERASE_RC:
	strcpy(globalResourceBundle.erase,stringValue);
	if (strlen(stringValue) > (size_t) 0) {
	    XtSetSensitive(resourceEntryRC[ERASE_MODE_RC],True);
	} else {
	    XtSetSensitive(resourceEntryRC[ERASE_MODE_RC],False);
	}
	break;
    case RD_LABEL_RC:
	strcpy(globalResourceBundle.rdLabel,stringValue);
	break;
    }
    XtFree(stringValue);

  /****** Update elements (this is overkill, but okay for now) */
  /* unhighlight (since objects may move) */
    if (currentDisplayInfo != NULL) {
	DlElement *dlElement = FirstDlElement(
	  currentDisplayInfo->selectedDlElementList);
	unhighlightSelectedElements();
	while (dlElement) {
	    updateElementFromGlobalResourceBundle(dlElement->structure.element);
	    dlElement = dlElement->next;
	}
	dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
      /* highlight */
	highlightSelectedElements();
	if (currentDisplayInfo->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(currentDisplayInfo);
    }
}

#ifdef __cplusplus
void cpAxisTextFieldActivateCallback(Widget w, XtPointer cd, XtPointer)
#else
void cpAxisTextFieldActivateCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int rcType = (int) cd;
    char *stringValue, string[24];
    int i, k, n, iPrec;
    Arg args[6];
    XcVType valF, minF, maxF, tickF;
    String resourceName;

    stringValue = XmTextFieldGetString(w);

  /****** For the strcpy() calls, note that the textField has a maxLength 
	  resource set such that the strcpy always succeeds */
    n = 0;
    switch(rcType) {
    case CP_X_RANGE_MIN:
    case CP_Y_RANGE_MIN:
    case CP_Y2_RANGE_MIN:
	globalResourceBundle.axis[rcType%3].minRange= atof(stringValue);
	if (globalDisplayListTraversalMode == DL_EXECUTE) {
	    valF.fval = globalResourceBundle.axis[rcType%3].minRange;
	    switch(rcType%3) {
	    case CP_X_RANGE_MIN: resourceName = XtNxrtXMin; break;
	    case CP_Y_RANGE_MIN: resourceName = XtNxrtYMin; break;
	    case CP_Y2_RANGE_MIN: resourceName = XtNxrtY2Min; break;
	    }
	    XtSetArg(args[n],resourceName,valF.lval); n++;
	}
	break;
    case CP_X_RANGE_MAX:
    case CP_Y_RANGE_MAX:
    case CP_Y2_RANGE_MAX:
	globalResourceBundle.axis[rcType%3].maxRange= atof(stringValue);
	if (globalDisplayListTraversalMode == DL_EXECUTE) {
	    valF.fval = globalResourceBundle.axis[rcType%3].maxRange;
	    switch(rcType%3) {
	    case CP_X_RANGE_MAX: resourceName = XtNxrtXMax; break;
	    case CP_Y_RANGE_MAX: resourceName = XtNxrtYMax; break;
	    case CP_Y2_RANGE_MAX: resourceName = XtNxrtY2Max; break;
	    }
	    XtSetArg(args[n],resourceName,valF.lval); n++;
	}
	break;
    default:
	medmPrintf("\ncpAxisTextFieldActivateCallback: unknown rcType = %d",rcType/3);
	break;

    }
    minF.fval = globalResourceBundle.axis[rcType%3].minRange;
    maxF.fval = globalResourceBundle.axis[rcType%3].maxRange;
    tickF.fval = (maxF.fval - minF.fval)/4.0;
    switch(rcType%3) {
    case X_AXIS_ELEMENT: resourceName = XtNxrtXTick; break;
    case Y1_AXIS_ELEMENT: resourceName = XtNxrtYTick; break;
    case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2Tick; break;
    }
    XtSetArg(args[n],resourceName,tickF.lval); n++;
    switch(rcType%3) {
    case X_AXIS_ELEMENT: resourceName = XtNxrtXNum; break;
    case Y1_AXIS_ELEMENT: resourceName = XtNxrtYNum; break;
    case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2Num; break;
    }
    XtSetArg(args[n],resourceName,tickF.lval); n++;
    switch(rcType%3) {
    case X_AXIS_ELEMENT: resourceName = XtNxrtXPrecision; break;
    case Y1_AXIS_ELEMENT: resourceName = XtNxrtYPrecision; break;
    case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2Precision; break;
    }
    XtSetArg(args[n],resourceName,tickF.lval); n++;
    sprintf(string,"%f",tickF.fval);
    k = strlen(string)-1;
    while (string[k] == '0') k--;	/* strip off trailing zeroes */
    iPrec = k;
    while (string[k] != '.' && k >= 0) k--;
    iPrec = iPrec - k;
    XtSetArg(args[n],resourceName,iPrec); n++;

    XtFree(stringValue);

  /*
   *  update for EDIT or EXECUTE mode
   */

    switch(globalDisplayListTraversalMode) {

    case DL_EDIT:
      /*
       * update elements (this is overkill, but okay for now)
       *	-- not as efficient as it should be (don't update EVERYTHING if only
       *	   one item changed!)
       */
	if (currentDisplayInfo != NULL) {
	    DlElement *dlElement = FirstDlElement(
	      currentDisplayInfo->selectedDlElementList);
	    unhighlightSelectedElements();
	    while (dlElement) {
		updateElementFromGlobalResourceBundle(dlElement->structure.element);
		dlElement = dlElement->next;
	    }
	    dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
	    highlightSelectedElements();
	}
	break;

    case DL_EXECUTE:
	if (executeTimeCartesianPlotWidget != NULL)
	  XtSetValues(executeTimeCartesianPlotWidget,args,n);
	break;
    }
}

#ifdef __cplusplus
void textFieldLosingFocusCallback(Widget, XtPointer cd, XtPointer)
#else
void textFieldLosingFocusCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int rcType = (int) cd;
    char string[MAX_TOKEN_LENGTH], *newString;
    int tail;
  
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
	newString = globalResourceBundle.chan;
	break;
    case CTRL_RC:
	newString = globalResourceBundle.chan;
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
    case CHAN_RC:
	newString = globalResourceBundle.chan;
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
	sprintf(string,"%d",globalResourceBundle.count);
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
    case IMAGENAME_RC:
	newString = globalResourceBundle.imageName;
	break;
    case DATA_RC:
	newString = globalResourceBundle.data;
	break;
    case CMAP_RC:
	newString = globalResourceBundle.cmap;
	break;
    case PRECISION_RC:
	sprintf(string,"%f",globalResourceBundle.dPrecision);
      /* strip trailing zeroes */
	tail = strlen(string);
	while (string[--tail] == '0') string[tail] = '\0';
	break;
    case SBIT_RC:
	sprintf(string,"%d",globalResourceBundle.sbit);
	break;
    case EBIT_RC:
	sprintf(string,"%d",globalResourceBundle.ebit);
	break;
    case TRIGGER_RC:
	newString= globalResourceBundle.trigger;
	break;
    case ERASE_RC :
	newString= globalResourceBundle.erase;
	break;
    }
    XmTextFieldSetString(resourceEntryElement[rcType],newString);
}

#ifdef __cplusplus
void cpAxisTextFieldLosingFocusCallback(Widget, XtPointer cd, XtPointer)
#else
void cpAxisTextFieldLosingFocusCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int rcType = (int) cd;
    char string[MAX_TOKEN_LENGTH], *currentString;
    int tail;
    XcVType minF[3], maxF[3];
    Widget cp = NULL;

  /****** Losing focus - make sure that the text field remains accurate wrt 
	  values stored in widget (not necessarily what is in 
  globalResourceBundle) */
    if (executeTimeCartesianPlotWidget != NULL)
      XtVaGetValues(executeTimeCartesianPlotWidget,
	XtNxrtXMin,&minF[X_AXIS_ELEMENT].lval,
	XtNxrtYMin,&minF[Y1_AXIS_ELEMENT].lval,
	XtNxrtY2Min,&minF[Y2_AXIS_ELEMENT].lval,
	XtNxrtXMax,&maxF[X_AXIS_ELEMENT].lval,
	XtNxrtYMax,&maxF[Y1_AXIS_ELEMENT].lval,
	XtNxrtY2Max,&maxF[Y2_AXIS_ELEMENT].lval, NULL);
    else
      return;
  /*
   * losing focus - make sure that the text field remains accurate
   *	wrt values stored in widget (not necessarily what is in
   *	globalResourceBundle)
   */

    switch(rcType) {
    case CP_X_RANGE_MIN:
    case CP_Y_RANGE_MIN:
    case CP_Y2_RANGE_MIN:
	sprintf(string,"%f", minF[rcType%3].fval);
	break;
    case CP_X_RANGE_MAX:
    case CP_Y_RANGE_MAX:
    case CP_Y2_RANGE_MAX:
	sprintf(string,"%f", maxF[rcType%3].fval);
	break;
    default:
	fprintf(stderr,
	  "\ncpAxisTextFieldLosingFocusCallback: unknown rcType = %d",
	  rcType/3);
	return;
    }
  /* strip trailing zeroes */
    tail = strlen(string);
    while (string[--tail] == '0') string[tail] = '\0';
    currentString = XmTextFieldGetString(axisRangeMin[rcType%3]);
    if (strcmp(string,currentString))
      XmTextFieldSetString(axisRangeMin[rcType%3],string);
    XtFree(currentString);
}

#ifdef EXTENDED_INTERFACE
/****************************************************************************
 * Bundle Call-back                                                         *
 ****************************************************************************/
static void bundleCallback(
  Widget w,
  int bundleId,
  XmToggleButtonCallbackStruct *call_data) {

  /** Since both on & off will invoke this callback, only care about transition
   * of one to ON
   */

    if (call_data->set == False) return;

}
#endif

/*
 * initialize globalResourceBundle with (semi-arbitrary) values
 */
void initializeGlobalResourceBundle()
{
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
    if (currentDisplayInfo) {
      /*
       * (MDA) hopefully this will work in the general case (with displays being
       *	made current and un-current)
       */
	globalResourceBundle.clr = currentDisplayInfo->drawingAreaForegroundColor;
	globalResourceBundle.bclr = currentDisplayInfo->drawingAreaBackgroundColor;
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
    globalResourceBundle.format = DECIMAL;
    globalResourceBundle.label = LABEL_NONE;
    globalResourceBundle.direction = RIGHT;
    globalResourceBundle.clrmod = STATIC;
#ifdef __COLOR_RULE_H__
    globalResourceBundle.colorRule = 0;
#endif
    globalResourceBundle.fillmod = FROM_EDGE;
    globalResourceBundle.style = SOLID;
    globalResourceBundle.fill = F_SOLID;
    globalResourceBundle.lineWidth = 0;
    globalResourceBundle.dPrecision = 1.;
    globalResourceBundle.vis = V_STATIC;
    globalResourceBundle.chan[0] = '\0';
    globalResourceBundle.data_clr = 0;
    globalResourceBundle.dis = 10;
    globalResourceBundle.xyangle = 45;
    globalResourceBundle.zangle = 45;
    globalResourceBundle.period = 60.0;
    globalResourceBundle.units = SECONDS;
    globalResourceBundle.cStyle = POINT_PLOT;
    globalResourceBundle.erase_oldest = ERASE_OLDEST_OFF;
    globalResourceBundle.count = 1;
    globalResourceBundle.stacking = ROW;
    globalResourceBundle.imageType= NO_IMAGE;
    globalResourceBundle.name[0] = '\0';
    globalResourceBundle.textix[0] = '\0';
    globalResourceBundle.messageLabel[0] = '\0';
    globalResourceBundle.press_msg[0] = '\0';
    globalResourceBundle.release_msg[0] = '\0';
    globalResourceBundle.imageName[0] = '\0';
    globalResourceBundle.compositeName[0] = '\0';
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
}

/****************************************************************************
 * Initialize XmStrintg Value Tables: ResourceBundle and related widgets.   *
 ****************************************************************************/
static void initializeXmStringValueTables() {
    int i;
    static Boolean initialized = False;

  /****** Initialize XmString table for element types */
    if (!initialized) {
	initialized = True;
	for (i = 0; i <NUM_DL_ELEMENT_TYPES; i++) {
	    elementXmStringTable[i] = XmStringCreateSimple(elementStringTable[i]);
	}

      /****** Initialize XmString table for value types (format, alignment types) */
	for (i = 0; i < NUMBER_STRING_VALUES; i++) {
	    xmStringValueTable[i] = XmStringCreateSimple(stringValueTable[i]);
	}
    }
}

/****************************************************************************
 * Create Resource: Create and initialize the resourcePalette,              *
 *   resourceBundle and related widgets.                                    *
 ****************************************************************************/
void createResource() {
    Widget entriesSW, bundlesSW, resourceMB, messageF, resourceHelpPDM,
      menuHelpWidget;
    XmString buttons[N_MAX_MENU_ELES];
    KeySym keySyms[N_MAX_MENU_ELES];
    XmButtonType buttonType[N_MAX_MENU_ELES];
    int i, n;
    char name[20];
    Arg args[10];

  /****** If resource palette has already been created, simply return */
    if (resourceMW != NULL) return;

  /****** This make take a second... give user some indication */
    if (currentDisplayInfo != NULL) XDefineCursor(display,
      XtWindow(currentDisplayInfo->drawingArea), watchCursor);

  /****** Initialize XmString tables */
    initializeXmStringValueTables();
    xmstringSelect = XmStringCreateSimple("Select...");

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
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
    resourceS = XtCreatePopupShell("resourceS",topLevelShellWidgetClass,
      mainShell,args,n);

    XmAddWMProtocolCallback(resourceS,WM_DELETE_WINDOW,
      wmCloseCallback,(XtPointer)OTHER_SHELL);

    resourceMW = XmCreateMainWindow(resourceS,"resourceMW",NULL,0);

  /****** Create the menu bar */
    buttons[0] = XmStringCreateSimple("File");

#ifdef EXTENDED_INTERFACE
    buttons[1] = XmStringCreateSimple("Bundle");
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

  /* Color resourceMB properly (force so VUE doesn't interfere) */
    colorMenuBar(resourceMB,defaultForeground,defaultBackground);

  /* Free strings */
    for (i = 0; i < N_MAIN_MENU_ELES; i++) XmStringFree(buttons[i]);

  /****** create the file pulldown menu pane */
#ifdef EXTENDED_INTERFACE
    buttons[0] = XmStringCreateSimple("Open...");
    buttons[1] = XmStringCreateSimple("Save");
    buttons[2] = XmStringCreateSimple("Save As...");
    buttons[3] = XmStringCreateSimple("Separator");
    buttons[4] = XmStringCreateSimple("Close");
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
    buttons[0] = XmStringCreateSimple("Close");
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
    buttons[0] = XmStringCreateSimple("Create...");
    buttons[1] = XmStringCreateSimple("Delete");
    buttons[2] = XmStringCreateSimple("Rename...");
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
  /* (MDA) for now, disable this menu */
  /*     XtSetSensitive(resourceHelpPDM,False); */

#if 0
  /****** create the help pulldown menu pane */
    buttons[0] = XmStringCreateSimple("On Resource Palette...");
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

  /****** Add the resource bundle scrolled window and contents */
    n = 0;
    XtSetArg(args[n],XmNscrollingPolicy,XmAUTOMATIC); n++;
    XtSetArg(args[n],XmNscrollBarDisplayPolicy,XmAS_NEEDED); n++;
    bundlesSW = XmCreateScrolledWindow(resourceMW,"bundlesSW",args,n);
    createResourceBundles(bundlesSW);

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
    XtManageChild(bundlesSW);
    XtManageChild(entriesSW);
    XtManageChild(resourceMW);

    XmMainWindowSetAreas(resourceMW,resourceMB,NULL,NULL,NULL,entriesSW);
    XtVaSetValues(resourceMW,XmNmessageWindow,messageF,XmNcommandWindow,
      bundlesSW, NULL);

  /****** Now popup the dialog and restore cursor */
    XtPopup(resourceS,XtGrabNone);

  /* change drawingArea's cursor back to the appropriate cursor */
    if (currentDisplayInfo != NULL)
      XDefineCursor(display,XtWindow(currentDisplayInfo->drawingArea),
	(currentActionType == SELECT_ACTION ? rubberbandCursor: crosshairCursor));
}

/****************************************************************************
 * Create Resource Entries: Create resource entries in scrolled window      *
 ****************************************************************************/
static void createResourceEntries(Widget entriesSW) {
    Widget entriesRC;
    Arg args[12];
    int i, n;

    n = 0;
    XtSetArg(args[n],XmNnumColumns,1); n++;
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_COLUMN); n++;
    entriesRC = XmCreateRowColumn(entriesSW,"entriesRC",args,n);

  /** Create the row-columns which are entries into overall row-column
    *	these entries are specific to resource bundle elements, and
    *	are managed/unmanaged according to the selected widgets being
    *	editted...  (see WidgetDM.h for details on this) */
    for (i = MIN_RESOURCE_ENTRY; i < MAX_RESOURCE_ENTRY; i++) {
	createEntryRC(entriesRC,i);
    }
    initializeResourcePaletteElements();

  /** now resize the labels and elements (to maximum's width)
    *	for uniform appearance */
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
      /****** Set label */
	XtSetValues(resourceEntryLabel[i],args,4);

      /****** Set element */
	if (XtClass(resourceEntryElement[i]) == xmRowColumnWidgetClass) {
	  /* must be option menu - unmanage label widget */
	    XtUnmanageChild(XmOptionLabelGadget(resourceEntryElement[i]));
	    XtSetValues(XmOptionButtonGadget(resourceEntryElement[i]),
	      &(args[4]),6);
	}
	XtSetValues(resourceEntryElement[i],&(args[4]),6);
      /* restrict size of CA PV name entry */
	if (i == CHAN_RC || i == RDBK_RC || i == CTRL_RC){

	    XtVaSetValues(resourceEntryElement[i],
	      XmNcolumns,(short)(PVNAME_STRINGSZ + FLDNAME_SZ+1),
	    /* since can have macro-substituted strings, need longer length */
	      XmNmaxLength,(int)MAX_TOKEN_LENGTH-1,NULL);

	} else if (i == MSG_LABEL_RC || i == PRESS_MSG_RC
	  || i == RELEASE_MSG_RC || i == TEXTIX_RC
	  || i == TITLE_RC || i == XLABEL_RC || i == YLABEL_RC) {
	  /* use size of CA PV name entry for other text-oriented fields */
	    XtVaSetValues(resourceEntryElement[i],
	      XmNcolumns,(short)(PVNAME_STRINGSZ + FLDNAME_SZ+1),NULL);
	}
    }

    XtManageChild(entriesRC);

}

/****************************************************************************
 * Create Entry RC: Create the various row-columns for each resource entry  *
 * rcType = {X_RC,Y_RC,...}.                                                *
 ****************************************************************************/
static void createEntryRC( Widget parent, int rcType) {
    Widget localRC, localLabel, localElement;
    XmString labelString;
    Dimension width, height;
    Arg args[6];
    int n;
    static Boolean first = True;
    static XmButtonType buttonType[MAX_OPTIONS];

    if (first) {
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
    labelString = XmStringCreateSimple(resourceEntryStringTable[rcType]);
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
    case COUNT_RC:
    case LINEWIDTH_RC:
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
    case CHAN_RC:
    case TEXTIX_RC:
    case MSG_LABEL_RC:
    case PRESS_MSG_RC:
    case RELEASE_MSG_RC:
    case IMAGENAME_RC:
    case DATA_RC:
    case CMAP_RC:
    case NAME_RC:
    case TRIGGER_RC:
    case ERASE_RC:
    case RD_LABEL_RC:
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

#ifdef __COLOR_RULE_H__
    case COLOR_RULE_RC:
	n = 0;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,
	  &(xmStringValueTable[FIRST_COLOR_RULE])); n++;
	XtSetArg(args[n],XmNbuttonCount,NUM_COLOR_RULE); n++;
	XtSetArg(args[n],XmNsimpleCallback,
	  (XtCallbackProc)optionMenuSimpleCallback); n++;
	XtSetArg(args[n],XmNuserData,rcType); n++;
	localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
	break;
#endif

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

    case IMAGETYPE_RC:
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

      /* color types */
    case CLR_RC:
    case BCLR_RC:
    case DATA_CLR_RC:
	n = 0;
	if (rcType == CLR_RC) {
	    XtSetArg(args[n],XmNbackground,
	      (currentDisplayInfo == NULL) ?
	      BlackPixel(display,screenNum) :
	      currentDisplayInfo->colormap[
		currentDisplayInfo->drawingAreaForegroundColor]); n++;
	} else {
	    XtSetArg(args[n],XmNbackground,
	      (currentDisplayInfo == NULL) ?
	      WhitePixel(display,screenNum) :
	      currentDisplayInfo->colormap[
		currentDisplayInfo->drawingAreaBackgroundColor]); n++;
	}
	localElement = XmCreateDrawnButton(localRC,"localElement",args,n);
	XtAddCallback(localElement,XmNactivateCallback,
	  colorSelectCallback,(XtPointer)rcType);
	break;


    case RDDATA_RC:
    case CPDATA_RC:
    case SCDATA_RC:
    case SHELLDATA_RC:
    case CPAXIS_RC:
	n = 0;
	XtSetArg(args[n],XmNlabelString,dlXmStringMoreToComeSymbol); n++;
	XtSetArg(args[n],XmNalignment,XmALIGNMENT_CENTER); n++;
	XtSetArg(args[n],XmNrecomputeSize,False); n++;
	localElement = XmCreatePushButton(localRC,"localElement",args,n);
	XtAddCallback(localElement,XmNactivateCallback,
	  pushButtonActivateCallback,(XtPointer)rcType);
	break;

    default:
	printf("\n------MISSED TYPE  %d  IN createEntryRC()!!--------",rcType);

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

static int table[] = {
    DL_Display,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, BCLR_RC, CMAP_RC, -1,
    DL_ChoiceButton,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CTRL_RC, CLR_RC, BCLR_RC, CLRMOD_RC,
    STACKING_RC, -1,
    DL_Menu,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CTRL_RC, CLR_RC, BCLR_RC, CLRMOD_RC, -1,
    DL_MessageButton,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CTRL_RC, CLR_RC, BCLR_RC, MSG_LABEL_RC,
    PRESS_MSG_RC, RELEASE_MSG_RC, CLRMOD_RC, -1,
    DL_Valuator,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CTRL_RC, CLR_RC, BCLR_RC, LABEL_RC,
    CLRMOD_RC, DIRECTION_RC, PRECISION_RC, -1,
    DL_TextEntry,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CTRL_RC, CLR_RC, BCLR_RC, CLRMOD_RC,
    FORMAT_RC, -1,
    DL_Meter,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, RDBK_RC, CLR_RC, BCLR_RC, LABEL_RC,
    CLRMOD_RC, -1,
    DL_TextUpdate,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, RDBK_RC, CLR_RC, BCLR_RC, CLRMOD_RC,
    ALIGN_RC, FORMAT_RC, -1,
    DL_Bar,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, RDBK_RC, CLR_RC, BCLR_RC, LABEL_RC,
    CLRMOD_RC, DIRECTION_RC, FILLMOD_RC, -1,
    DL_Byte,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, RDBK_RC, CLR_RC, BCLR_RC, SBIT_RC,
    EBIT_RC, CLRMOD_RC, DIRECTION_RC, -1,
    DL_Indicator,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, RDBK_RC, CLR_RC, BCLR_RC, LABEL_RC,
    CLRMOD_RC< DIRECTION_RC, -1,
    DL_StripChart,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, TITLE_RC, XLABEL_RC, YLABEL_RC, CLR_RC,
    BCLR_RC, PERIOD_RC, UNITS_RC, SCDATA_RC, -1,
    DL_CartesianPlot,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, TITLE_RC, XLABEL_RC, YLABEL_RC, CLR_RC,
    BCLR_RC, CSTYLE_RC, ERASE_OLDEST_RC, COUNT_RC, CPDATA_RC, CPAXIS_RC,
    TRIGGER_RC, ERASE_RC, ERASE_MODE_RC, -1,
    DL_Rectangle,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, STYLE_RC, FILL_RC, LINEWIDTH_RC,
#ifdef __COLOR_RULE_H__
    CLRMOD_RC, COLOR_RULE_RC, VIS_RC, CHAN_RC, -1,
#else
    CLRMOD_RC, VIS_RC, CHAN_RC, -1,
#endif
    DL_Oval,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, STYLE_RC, FILL_RC, LINEWIDTH_RC,
#ifdef __COLOR_RULE_H__
    CLRMOD_RC, COLOR_RULE_RC, VIS_RC, CHAN_RC, -1,
#else
    CLRMOD_RC, VIS_RC, CHAN_RC, -1,
#endif
    DL_Arc,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, BEGIN_RC, PATH_RC, CLR_RC, STYLE_RC,
#ifdef __COLOR_RULE_H__
    FILL_RC, LINEWIDTH_RC, CLRMOD_RC, COLOR_RULE_RC, VIS_RC, CHAN_RC, -1,
#else
    FILL_RC, LINEWIDTH_RC, CLRMOD_RC, VIS_RC, CHAN_RC, -1,
#endif
    DL_Text,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, TEXTIX_RC, ALIGN_RC, CLR_RC,
#ifdef __COLOR_RULE_H__
    CLRMOD_RC, COLOR_RULE_RC, VIS_RC, CHAN_RC, -1,
#else
    CLRMOD_RC, VIS_RC, CHAN_RC, -1,
#endif
    DL_RelatedDisplay,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, BCLR_RC, CLRMOD_RC,
#ifdef __COLOR_RULE_H__
    COLOR_RULE_RC, VIS_RC, CHAN_RC, RD_LABEL_RC, RD_VISUAL_RC, RDDATA_RC, -1,
#else
    VIS_RC, CHAN_RC, RD_LABEL_RC, RD_VISUAL_RC, RDDATA_RC, -1,
#endif
    DL_ShellCommand,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, BCLR_RC, SHELLDATA_RC, -1,
    DL_Image,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, IMAGETYPE_RC, IMAGENAME_RC, -1,
    DL_Composite,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, BCLR_RC, -1,
    DL_Line,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, STYLE_RC, LINEWIDTH_RC,
#ifdef __COLOR_RULE_H__
    CLRMOD_RC, COLOR_RULE_RC, VIS_RC, CHAN_RC, -1,
#else
    CLRMOD_RC, VIS_RC, CHAN_RC, -1,
#endif
    DL_Polyline,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, STYLE_RC, LINEWIDTH_RC,
#ifdef __COLOR_RULE_H__
    CLRMOD_RC, COLOR_RULE_RC, VIS_RC, CHAN_RC, -1,
#else
    CLRMOD_RC, VIS_RC, CHAN_RC, -1,
#endif
    DL_Polygon,
    X_RC, Y_RC, WIDTH_RC, HEIGHT_RC, CLR_RC, STYLE_RC, FILL_RC, LINEWIDTH_RC,
#ifdef __COLOR_RULE_H__
    CLRMOD_RC, COLOR_RULE_RC, VIS_RC, CHAN_RC, -1,
#else
    CLRMOD_RC, VIS_RC, CHAN_RC, -1,
#endif
};

static void initializeResourcePaletteElements() {
    int i, j, index;
    int tableSize = sizeof(table)/sizeof(int);

    index = -1;
    for (i=0; i<tableSize; i++) {
	if (index < 0) {
	  /* start a new element, get the new index */
	    index = table[i] - MIN_DL_ELEMENT_TYPE;
	    j = 0;
	} else {
	    if (table[i] >= 0) {
	      /* copy RC resource from table until it meet -1 */
		resourcePaletteElements[index].childIndexRC[j] = table[i];
		resourcePaletteElements[index].children[j] =
		  resourceEntryRC[table[i]];
		j++;
	    } else {
		int k;
	      /* reset the index, fill the rest with zero */
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

/****************************************************************************
 * Create Resource Bundles : Create resource bundles in scrolled window.    *
 ****************************************************************************/
static void createResourceBundles(Widget bundlesSW) {
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
static void createBundleTB(Widget bundlesRB, char *name) {
    Widget bundlesTB;
    Arg args[10];
    int n;
    XmString xmString;

    n = 0;
    xmString = XmStringCreateSimple(name);
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    if (resourceBundleCounter == SELECTION_BUNDLE) {
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

#ifdef __cplusplus
static void shellCommandActivate(Widget, XtPointer cd, XtPointer)
#else
static void shellCommandActivate(Widget w, XtPointer cd, XtPointer cb)
#endif
{
    int buttonType = (int) cd;
    String **newCells;
    int i;

    switch (buttonType) {
    case CMD_APPLY_BTN:
      /* commit changes in matrix to global matrix array data */
	XbaeMatrixCommitEdit(cmdMatrix,False);
	XtVaGetValues(cmdMatrix,XmNcells,&newCells,NULL);
      /* now update globalResourceBundle...*/
	for (i = 0; i < MAX_SHELL_COMMANDS; i++) {
	    strcpy(globalResourceBundle.cmdData[i].label, newCells[i][0]);
	    strcpy(globalResourceBundle.cmdData[i].command, newCells[i][1]);
	    strcpy(globalResourceBundle.cmdData[i].args, newCells[i][2]);
	}
      /* and update the elements (since this level of "OK" is analogous
       *	to changing text in a text field in the resource palette
       *	(don't need to traverse the display list since these changes
       *	 aren't visible at the first level)
       */
	if (currentDisplayInfo) {
	    DlElement *dlElement = FirstDlElement(
	      currentDisplayInfo->selectedDlElementList);
	    unhighlightSelectedElements();
	    while (dlElement) {
		if (dlElement->structure.element->type = DL_ShellCommand)
		  updateElementFromGlobalResourceBundle(dlElement->structure.element);
		dlElement = dlElement->next;
	    }
	}
	XtPopdown(shellCommandS);
	if (currentDisplayInfo->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(currentDisplayInfo);
	break;

    case CMD_CLOSE_BTN:
	XtPopdown(shellCommandS);
	break;
    }
}

/*
 * create shell command data dialog
 */
Widget createShellCommandDataDialog(
  Widget parent)
{
    Widget shell, applyButton, closeButton;
    Dimension cWidth, cHeight, aWidth, aHeight;
    Arg args[12];
    XmString xmString;
    int i, j, n;
    static Boolean first = True;


  /* initialize those file-scoped globals */
    if (first) {
	first = False;
	for (i = 0; i < MAX_SHELL_COMMANDS; i++) {
	    for (j = 0; j < 3; j++) cmdRows[i][j] = NULL;
	    cmdCells[i] = &cmdRows[i][0];
	}
    }

  /*
   * now create the interface
   *
   *	       label | cmd | args
   *	       -------------------
   *	    1 |  A      B      C
   *	    2 | 
   *	    3 | 
   *		     ...
   *		 OK     CANCEL
   */

    n = 0;
    XtSetArg(args[n],XmNautoUnmanage,False); n++;
    XtSetArg(args[n],XmNmarginHeight,8); n++;
    XtSetArg(args[n],XmNmarginWidth,8); n++;
    cmdForm = XmCreateFormDialog(parent,"shellCommandDataF",args,n);
    shell = XtParent(cmdForm);
    n = 0;
    XtSetArg(args[n],XmNtitle,"Shell Command Data"); n++;
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
    XtSetValues(shell,args,n);
    XmAddWMProtocolCallback(shell,WM_DELETE_WINDOW,
      shellCommandActivate,(XtPointer)CMD_CLOSE_BTN);
    n = 0;
    XtSetArg(args[n],XmNrows,MAX_RELATED_DISPLAYS); n++;
    XtSetArg(args[n],XmNcolumns,3); n++;
    XtSetArg(args[n],XmNcolumnMaxLengths,cmdColumnMaxLengths); n++;
    XtSetArg(args[n],XmNcolumnWidths,cmdColumnWidths); n++;
    XtSetArg(args[n],XmNcolumnLabels,cmdColumnLabels); n++;
    XtSetArg(args[n],XmNcolumnMaxLengths,cmdColumnMaxLengths); n++;
    XtSetArg(args[n],XmNcolumnWidths,cmdColumnWidths); n++;
    XtSetArg(args[n],XmNcolumnLabelAlignments,cmdColumnLabelAlignments); n++;
    XtSetArg(args[n],XmNboldLabels,False); n++;
    cmdMatrix = XtCreateManagedWidget("cmdMatrix",
      xbaeMatrixWidgetClass,cmdForm,args,n);


    xmString = XmStringCreateSimple("Close");
    n = 0;
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    closeButton = XmCreatePushButton(cmdForm,"closeButton",args,n);
    XtAddCallback(closeButton,XmNactivateCallback,
      shellCommandActivate,(XtPointer)CMD_CLOSE_BTN);
    XtManageChild(closeButton);
    XmStringFree(xmString);

    xmString = XmStringCreateSimple("Apply");
    n = 0;
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    applyButton = XmCreatePushButton(cmdForm,"applyButton",args,n);
    XtAddCallback(applyButton,XmNactivateCallback,
      shellCommandActivate,(XtPointer)CMD_APPLY_BTN);
    XtManageChild(applyButton);
    XmStringFree(xmString);

  /* make APPLY and CLOSE buttons same size */
    XtVaGetValues(closeButton,XmNwidth,&cWidth,XmNheight,&cHeight,NULL);
    XtVaGetValues(applyButton,XmNwidth,&aWidth,XmNheight,&aHeight,NULL);
    XtVaSetValues(closeButton,XmNwidth,MAX(cWidth,aWidth),
      XmNheight,MAX(cHeight,aHeight),NULL);

  /* and make the APPLY button the default for the form */
    XtVaSetValues(cmdForm,XmNdefaultButton,applyButton,NULL);

  /*
   * now do form layout 
   */

  /* cmdMatrix */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
    XtSetValues(cmdMatrix,args,n);
  /* apply */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,cmdMatrix); n++;
    XtSetArg(args[n],XmNtopOffset,12); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNleftPosition,30); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomOffset,12); n++;
    XtSetValues(applyButton,args,n);
  /* close */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,cmdMatrix); n++;
    XtSetArg(args[n],XmNtopOffset,12); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNrightPosition,70); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomOffset,12); n++;
    XtSetValues(closeButton,args,n);

    XtManageChild(cmdForm);

    return shell;
}

/*
 * access function (for file-scoped globals, etc) to udpate the
 *	shell command data dialog with the values currently in
 *	globalResourceBundle
 */
void updateShellCommandDataDialog() {
    int i;

    for (i = 0; i < MAX_SHELL_COMMANDS; i++) {
	cmdRows[i][0] = globalResourceBundle.cmdData[i].label;
	cmdRows[i][1] = globalResourceBundle.cmdData[i].command;
	cmdRows[i][2] = globalResourceBundle.cmdData[i].args;
    }
    if (cmdMatrix != NULL) XtVaSetValues(cmdMatrix,XmNcells,cmdCells,NULL);
  
}

#ifdef __cplusplus
static void cartesianPlotActivate(Widget, XtPointer cd, XtPointer)
#else
static void cartesianPlotActivate(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int buttonType = (int) cd;
    String **newCells;
    int i;

    switch (buttonType) {
    case CP_APPLY_BTN:
      /* commit changes in matrix to global matrix array data */
	XbaeMatrixCommitEdit(cpMatrix,False);
	XtVaGetValues(cpMatrix,XmNcells,&newCells,NULL);
      /* now update globalResourceBundle...*/
	for (i = 0; i < MAX_TRACES; i++) {
	    strcpy(globalResourceBundle.cpData[i].xdata,
	      newCells[i][CP_XDATA_COLUMN]);
	    strcpy(globalResourceBundle.cpData[i].ydata,
	      newCells[i][CP_YDATA_COLUMN]);
	    globalResourceBundle.cpData[i].data_clr = 
	      (int) cpColorRows[i][CP_COLOR_COLUMN];
	}
      /* and update the elements (since this level of "Apply" is analogous
       *	to changing text in a text field in the resource palette
       *	(don't need to traverse the display list since these changes
       *	 aren't visible at the first level)
       */
	if (currentDisplayInfo) {
	    DlElement *dlElement = FirstDlElement(
	      currentDisplayInfo->selectedDlElementList);
	    unhighlightSelectedElements();
	    while (dlElement) {
		if (dlElement->structure.element->type = DL_CartesianPlot)
		  updateElementFromGlobalResourceBundle(dlElement->structure.element);
		dlElement = dlElement->next;
	    }
	}
	XtPopdown(cartesianPlotS);
	if (currentDisplayInfo->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(currentDisplayInfo);
	break;

    case CP_CLOSE_BTN:
	XtPopdown(cartesianPlotS);
	break;
    }
}


#ifdef __cplusplus
static void cartesianPlotAxisActivate(Widget, XtPointer cd, XtPointer)
#else
static void cartesianPlotAxisActivate(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int buttonType = (int) cd;

    switch (buttonType) {

    case CP_CLOSE_BTN:
	XtPopdown(cartesianPlotAxisS);
      /* since done with CP Axis dialog, reset that selected widget */
	executeTimeCartesianPlotWidget  = NULL;
	break;
    }
}

/*
 * function to handle cell selection in the matrix
 *	mostly it passes through for the text field entry
 *	but pops up the color editor for the color field selection
 */
#ifdef __cplusplus
void cpEnterCellCallback(Widget, XtPointer, XtPointer cbs)
#else
void cpEnterCellCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    XbaeMatrixEnterCellCallbackStruct *call_data = (XbaeMatrixEnterCellCallbackStruct *) cbs;
    int row;
    if (call_data->column == CP_COLOR_COLUMN) {
      /* set this cell non-editable */
	call_data->doit = False;
      /* update the color palette, set index of the color vector element to set */
	row = call_data->row;
	setCurrentDisplayColorsInColorPalette(CPDATA_RC,row);
	XtPopup(colorS,XtGrabNone);
    }
}


/*
 * function to actually update the colors in the COLOR_COLUMN of the matrix
 */
void cpUpdateMatrixColors()
{
    int i;

  /* XmNcolors needs pixel values */
    for (i = 0; i < MAX_TRACES; i++) {
	cpColorRows[i][CP_COLOR_COLUMN] = currentColormap[
	  globalResourceBundle.cpData[i].data_clr];
    }
    if (cpMatrix != NULL) XtVaSetValues(cpMatrix,XmNcolors,cpColorCells,NULL);

  /* but for resource editing cpData should contain indexes into colormap */
  /* this resource is copied, hence this is okay to do */
    for (i = 0; i < MAX_TRACES; i++) {
	cpColorRows[i][CP_COLOR_COLUMN] = globalResourceBundle.cpData[i].data_clr;
    }
}

/*
 * create data dialog
 */
Widget createCartesianPlotDataDialog(
  Widget parent)
{
    Widget shell, applyButton, closeButton;
    Dimension cWidth, cHeight, aWidth, aHeight;
    Arg args[12];
    XmString xmString;
    int i, j, n;
    static Boolean first = True;


  /* initialize those file-scoped globals */
    if (first) {
	first = False;
	for (i = 0; i < MAX_TRACES; i++) {
	    for (j = 0; j < 2; j++) cpRows[i][j] = NULL;
	    cpRows[i][2] = dashes;
	    cpCells[i] = &cpRows[i][0];
	    cpColorCells[i] = &cpColorRows[i][0];
	}
    }

  /*
   * now create the interface
   *
   *	       xdata | ydata | color
   *	       ---------------------
   *	    1 |  A      B      C
   *	    2 | 
   *	    3 | 
   *		     ...
   *		 OK     CANCEL
   */
    
    n = 0;
    XtSetArg(args[n],XmNautoUnmanage,False); n++;
    XtSetArg(args[n],XmNmarginHeight,8); n++;
    XtSetArg(args[n],XmNmarginWidth,8); n++;
    XtSetArg(args[n],XmNdialogStyle,XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
    cpForm = XmCreateFormDialog(parent,"cartesianPlotDataF",args,n);
    shell = XtParent(cpForm);
    n = 0;
    XtSetArg(args[n],XmNtitle,"Cartesian Plot Data"); n++;
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
    XtSetValues(shell,args,n);
    XmAddWMProtocolCallback(shell,WM_DELETE_WINDOW,
      cartesianPlotActivate,(XtPointer)CP_CLOSE_BTN);
    n = 0;
    XtSetArg(args[n],XmNrows,MAX_TRACES); n++;
    XtSetArg(args[n],XmNcolumns,3); n++;
    XtSetArg(args[n],XmNcolumnMaxLengths,cpColumnMaxLengths); n++;
    XtSetArg(args[n],XmNcolumnWidths,cpColumnWidths); n++;
    XtSetArg(args[n],XmNcolumnLabels,cpColumnLabels); n++;
    XtSetArg(args[n],XmNcolumnMaxLengths,cpColumnMaxLengths); n++;
    XtSetArg(args[n],XmNcolumnWidths,cpColumnWidths); n++;
    XtSetArg(args[n],XmNcolumnLabelAlignments,cpColumnLabelAlignments); n++;
    XtSetArg(args[n],XmNboldLabels,False); n++;
    cpMatrix = XtCreateManagedWidget("cpMatrix",
      xbaeMatrixWidgetClass,cpForm,args,n);
    cpUpdateMatrixColors();
    XtAddCallback(cpMatrix,XmNenterCellCallback,
      cpEnterCellCallback,(XtPointer)NULL);


    xmString = XmStringCreateSimple("Close");
    n = 0;
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    closeButton = XmCreatePushButton(cpForm,"closeButton",args,n);
    XtAddCallback(closeButton,XmNactivateCallback,
      cartesianPlotActivate,(XtPointer)CP_CLOSE_BTN);
    XtManageChild(closeButton);
    XmStringFree(xmString);

    xmString = XmStringCreateSimple("Apply");
    n = 0;
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    applyButton = XmCreatePushButton(cpForm,"applyButton",args,n);
    XtAddCallback(applyButton,XmNactivateCallback,
      cartesianPlotActivate,(XtPointer)CP_APPLY_BTN);
    XtManageChild(applyButton);
    XmStringFree(xmString);

  /* make APPLY and CLOSE buttons same size */
    XtVaGetValues(closeButton,XmNwidth,&cWidth,XmNheight,&cHeight,NULL);
    XtVaGetValues(applyButton,XmNwidth,&aWidth,XmNheight,&aHeight,NULL);
    XtVaSetValues(closeButton,XmNwidth,MAX(cWidth,aWidth),
      XmNheight,MAX(cHeight,aHeight),NULL);

  /* and make the APPLY button the default for the form */
    XtVaSetValues(cpForm,XmNdefaultButton,applyButton,NULL);

  /*
   * now do form layout 
   */

  /* cpMatrix */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
    XtSetValues(cpMatrix,args,n);
  /* apply */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,cpMatrix); n++;
    XtSetArg(args[n],XmNtopOffset,12); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNleftPosition,25); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomOffset,12); n++;
    XtSetValues(applyButton,args,n);
  /* close */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,cpMatrix); n++;
    XtSetArg(args[n],XmNtopOffset,12); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNrightPosition,75); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomOffset,12); n++;
    XtSetValues(closeButton,args,n);


    XtManageChild(cpForm);

    return shell;
}

/*
 * access function (for file-scoped globals, etc) to udpate the
 *	cartesian plot data dialog with the values currently in
 *	globalResourceBundle
 */
void updateCartesianPlotDataDialog()
{
    int i;

    for (i = 0; i < MAX_TRACES; i++) {
	cpRows[i][0] = globalResourceBundle.cpData[i].xdata;
	cpRows[i][1] = globalResourceBundle.cpData[i].ydata;
	cpRows[i][2] =  dashes;
    }
  /* handle data_clr in here */
    cpUpdateMatrixColors();
    if (cpMatrix)
      XtVaSetValues(cpMatrix,XmNcells,cpCells,NULL);
}

#ifdef __cplusplus
static void stripChartActivate(Widget, XtPointer cd, XtPointer) 
#else
static void stripChartActivate(Widget w, XtPointer cd, XtPointer cbs) 
#endif
{
    int buttonType = (int) cd;
    String **newCells;
    int i;

    switch (buttonType) {

    case SC_APPLY_BTN:
      /* commit changes in matrix to global matrix array data */
	XbaeMatrixCommitEdit(scMatrix,False);
	XtVaGetValues(scMatrix,XmNcells,&newCells,NULL);
      /* now update globalResourceBundle...*/
	for (i = 0; i < MAX_PENS; i++) {
	    strcpy(globalResourceBundle.scData[i].chan,
	      newCells[i][SC_CHANNEL_COLUMN]);
	    globalResourceBundle.scData[i].clr =
	      (int) scColorRows[i][SC_COLOR_COLUMN];
	}
      /* and update the elements (since this level of "Apply" is analogous
       *	to changing text in a text field in the resource palette
       *	(don't need to traverse the display list since these changes
       *	 aren't visible at the first level)
       */
	if (currentDisplayInfo != NULL) {
	    DlElement *dlElement = FirstDlElement(
	      currentDisplayInfo->selectedDlElementList);
	    unhighlightSelectedElements();
	    while (dlElement) {
		if (dlElement->structure.element->type = DL_StripChart)
		  updateElementFromGlobalResourceBundle(dlElement->structure.element);
		dlElement = dlElement->next;
	    }
	}
	XtPopdown(stripChartS);
	if (currentDisplayInfo->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(currentDisplayInfo);
	break;
    case SC_CLOSE_BTN:
	XtPopdown(stripChartS);
	break;
    }
}

/*
 * function to handle cell selection in the matrix
 *	mostly it passes through for the text field entry
 *	but pops up the color editor for the color field selection
 */
#ifdef __cplusplus
void scEnterCellCallback(Widget, XtPointer, XtPointer cbs)
#else
void scEnterCellCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    XbaeMatrixEnterCellCallbackStruct *call_data = (XbaeMatrixEnterCellCallbackStruct *) cbs;
    int row;
    if (call_data->column == SC_COLOR_COLUMN) {
      /* set this cell non-editable */
	call_data->doit = False;
      /* update the color palette, set index of the color vector element to set */
	row = call_data->row;
	setCurrentDisplayColorsInColorPalette(SCDATA_RC,row);
	XtPopup(colorS,XtGrabNone);
    }
}


/*
 * function to actually update the colors in the COLOR_COLUMN of the matrix
 */
void scUpdateMatrixColors()
{
    int i;

  /* XmNcolors needs pixel values */
    for (i = 0; i < MAX_PENS; i++) {
	scColorRows[i][SC_COLOR_COLUMN] =
	  currentColormap[globalResourceBundle.scData[i].clr];
    }
    if (scMatrix != NULL) XtVaSetValues(scMatrix,XmNcolors,scColorCells,NULL);

  /* but for resource editing scData should contain indexes into colormap */
  /* this resource is copied, hence this is okay to do */
    for (i = 0; i < MAX_PENS; i++) {
	scColorRows[i][SC_COLOR_COLUMN] = globalResourceBundle.scData[i].clr;
    }

}

/*
 * create strip chart data dialog
 */
Widget createStripChartDataDialog(Widget parent)
{
    Widget shell, applyButton, closeButton;
    Dimension cWidth, cHeight, aWidth, aHeight;
    Arg args[14];
    XmString xmString;
    int i, n;
    static Boolean first = True;
    Pixel foreground;

    XtVaGetValues(parent,XmNforeground,&foreground,NULL);

  /* initialize those file-scoped globals */
    if (first) {
	first = False;
	for (i = 0; i < MAX_PENS; i++) {
	    scRows[i][0] = NULL;
	    scRows[i][1] = NULL;
	    scColorRows[i][SC_CHANNEL_COLUMN] = foreground;
	    scColorRows[i][SC_COLOR_COLUMN] = foreground;
	    scRows[i][1] = dashes;
	    scCells[i] = &scRows[i][0];
	    scColorCells[i] = &scColorRows[i][0];
	}
    }


  /*
   * now create the interface
   *
   *	       channel | color
   *	       ---------------
   *	    1 |  A         B  
   *	    2 | 
   *	    3 | 
   *		     ...
   *		 OK     CANCEL
   */
    
    n = 0;
    XtSetArg(args[n],XmNautoUnmanage,False); n++;
    XtSetArg(args[n],XmNmarginHeight,8); n++;
    XtSetArg(args[n],XmNmarginWidth,8); n++;
    XtSetArg(args[n],XmNdialogStyle,XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
    scForm = XmCreateFormDialog(parent,"stripChartDataF",args,n);
    shell = XtParent(scForm);
    XmAddWMProtocolCallback(shell,WM_DELETE_WINDOW,
      stripChartActivate,(XtPointer)SC_CLOSE_BTN);
    n = 0;
    XtSetArg(args[n],XmNtitle,"Strip Chart Data"); n++;
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
    XtSetValues(shell,args,n);
    n = 0;
    XtSetArg(args[n],XmNrows,MAX_PENS); n++;
    XtSetArg(args[n],XmNcolumns,2); n++;
    XtSetArg(args[n],XmNcolumnMaxLengths,scColumnMaxLengths); n++;
    XtSetArg(args[n],XmNcolumnWidths,scColumnWidths); n++;
    XtSetArg(args[n],XmNcolumnLabels,scColumnLabels); n++;
    XtSetArg(args[n],XmNcolumnMaxLengths,scColumnMaxLengths); n++;
    XtSetArg(args[n],XmNcolumnWidths,scColumnWidths); n++;
    XtSetArg(args[n],XmNcolumnLabelAlignments,scColumnLabelAlignments); n++;
    XtSetArg(args[n],XmNboldLabels,False); n++;
    scMatrix = XtCreateManagedWidget("scMatrix",
      xbaeMatrixWidgetClass,scForm,args,n);
    scUpdateMatrixColors();

    XtAddCallback(scMatrix,XmNenterCellCallback,
      scEnterCellCallback,(XtPointer)NULL);

    xmString = XmStringCreateSimple("Close");
    n = 0;
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    closeButton = XmCreatePushButton(scForm,"closeButton",args,n);
    XtAddCallback(closeButton,XmNactivateCallback,
      stripChartActivate,(XtPointer)SC_CLOSE_BTN);
    XtManageChild(closeButton);
    XmStringFree(xmString);

    xmString = XmStringCreateSimple("Apply");
    n = 0;
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    applyButton = XmCreatePushButton(scForm,"applyButton",args,n);
    XtAddCallback(applyButton,XmNactivateCallback,
      stripChartActivate,(XtPointer)SC_APPLY_BTN);
    XtManageChild(applyButton);
    XmStringFree(xmString);

  /* make APPLY and CLOSE buttons same size */
    XtVaGetValues(closeButton,XmNwidth,&cWidth,XmNheight,&cHeight,NULL);
    XtVaGetValues(applyButton,XmNwidth,&aWidth,XmNheight,&aHeight,NULL);
    XtVaSetValues(closeButton,XmNwidth,MAX(cWidth,aWidth),
      XmNheight,MAX(cHeight,aHeight),NULL);

  /* and make the APPLY button the default for the form */
    XtVaSetValues(scForm,XmNdefaultButton,applyButton,NULL);

  /*
   * now do form layout 
   */

  /* scMatrix */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
    XtSetValues(scMatrix,args,n);
  /* apply */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,scMatrix); n++;
    XtSetArg(args[n],XmNtopOffset,12); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNleftPosition,20); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomOffset,12); n++;
    XtSetValues(applyButton,args,n);
  /* close */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,scMatrix); n++;
    XtSetArg(args[n],XmNtopOffset,12); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNrightPosition,80); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomOffset,12); n++;
    XtSetValues(closeButton,args,n);


    XtManageChild(scForm);

    return shell;
}

/*
 * access function (for file-scoped globals, etc) to udpate the
 *	strip chart data dialog with the values currently in
 *	globalResourceBundle
 */
void updateStripChartDataDialog()
{
    int i;

    for (i = 0; i < MAX_PENS; i++) {
	scRows[i][0] = globalResourceBundle.scData[i].chan;
	cpRows[i][1] =  dashes;
    }
  /* handle clr in here */
    scUpdateMatrixColors();
    if (scMatrix != NULL) XtVaSetValues(scMatrix,XmNcells,scCells,NULL);
  
}

/*****************
 * for the Cartesian Plot Axis Dialog...
 *****************/
void createCartesianPlotAxisDialogMenuEntry(
  Widget         parentRC,
  XmString       axisLabelXmString,
  Widget *       label,
  Widget *       menu,
  XmString *     menuLabelXmStrings,
  XmButtonType * buttonType,
  int            numberOfLabels,
  XtPointer      clientData) {
    Arg args[10];
    int n = 0;
    Widget rowColumn;

  /* create rowColumn widget to hold the label and menu widgets */
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_NONE); n++;
    rowColumn = XmCreateRowColumn(parentRC,"entryRC",args,n);
 
  /* create the label widget */
    n = 0;
    XtSetArg(args[n],XmNalignment,XmALIGNMENT_END); n++;
    XtSetArg(args[n],XmNlabelString,axisLabelXmString); n++;
    XtSetArg(args[n],XmNrecomputeSize,False); n++;
    *label = XmCreateLabel(rowColumn,"localLabel",args,n);
 
  /* create the text widget */
    n = 0;
    XtSetArg(args[n],XmNbuttonType,buttonType); n++;
    XtSetArg(args[n],XmNbuttons,menuLabelXmStrings); n++;
    XtSetArg(args[n],XmNbuttonCount,numberOfLabels); n++;
    XtSetArg(args[n],XmNsimpleCallback,cpAxisOptionMenuSimpleCallback); n++;
    XtSetArg(args[n],XmNuserData,clientData); n++;
    *menu = XmCreateSimpleOptionMenu(rowColumn,"localElement",args,n);
    XtUnmanageChild(XmOptionLabelGadget(*menu));
    XtManageChild(rowColumn);
}

void createCartesianPlotAxisDialogTextEntry(
  Widget parentRC,
  XmString axisLabelXmString,
  Widget *rowColumn,
  Widget *label,
  Widget *text,
  XtPointer clientData)
{
    Arg args[10];
    int n = 0;
  /* create a row column widget to hold the label and textfield widget */
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_NONE); n++;
    *rowColumn = XmCreateRowColumn(parentRC,"entryRC",args,n);
 
  /* create the label */
    n = 0;
    XtSetArg(args[n],XmNalignment,XmALIGNMENT_END); n++;
    XtSetArg(args[n],XmNlabelString,axisLabelXmString); n++;
    XtSetArg(args[n],XmNrecomputeSize,False); n++;
    *label = XmCreateLabel(*rowColumn,"localLabel",args,n);
 
  /* create the text field */
    n = 0;
    XtSetArg(args[n],XmNmaxLength,MAX_TOKEN_LENGTH-1); n++;
    *text = XmCreateTextField(*rowColumn,"localElement",args,n);
    XtAddCallback(*text,XmNactivateCallback,cpAxisTextFieldActivateCallback,
      clientData);
    XtAddCallback(*text,XmNlosingFocusCallback,cpAxisTextFieldLosingFocusCallback,
      clientData);
    XtAddCallback(*text,XmNmodifyVerifyCallback, textFieldFloatVerifyCallback,
      NULL);
    XtManageChild(*rowColumn);
}
     

/*
 * create axis dialog
 */
#ifdef __cplusplus
Widget createCartesianPlotAxisDialog(Widget)
#else
Widget createCartesianPlotAxisDialog(Widget parent)
#endif
{
    Widget shell, closeButton;
    Arg args[12];
    int counter;
    XmString xmString, axisStyleXmString, axisRangeXmString, axisMinXmString,
      axisMaxXmString, axisTimeFmtXmString, frameLabelXmString;
    int i, n;
    static Boolean first = True;
    XmButtonType buttonType[MAX_CP_AXIS_BUTTONS];
    Widget entriesRC, frame, localRC, localLabel, localElement, parentRC;
  /* for keeping list of widgets around */
    Widget entryLabel[MAX_CP_AXIS_ELEMENTS], entryElement[MAX_CP_AXIS_ELEMENTS];

    Dimension width, height;
    static int maxWidth = 0, maxHeight = 0;

  /* indexed like dlCartesianPlot->axis[]: X_ELEMENT_AXIS, Y1_ELEMENT_AXIS... */
    static char *frameLabelString[3] = {"X Axis", "Y1 Axis", "Y2 Axis",};

  /* initialize XmString value tables (since this can be edit or execute time) */
    initializeXmStringValueTables();

    for (i = 0; i < MAX_CP_AXIS_BUTTONS; i++) buttonType[i] = XmPUSHBUTTON;

  /*
   * now create the interface
   *		     ...
   *		 OK     CANCEL
   */
    
    n = 0;
    XtSetArg(args[n],XmNdeleteResponse,XmDO_NOTHING); n++;
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
    XtSetArg(args[n],XmNautoUnmanage,False); n++;
    XtSetArg(args[n],XmNtitle,"Cartesian Plot Axis Data"); n++;
    shell = XtCreatePopupShell("cartesianPlotAxisS",
      topLevelShellWidgetClass,mainShell,args,n);
    XmAddWMProtocolCallback(shell,WM_DELETE_WINDOW,
      cartesianPlotAxisActivate,
      (XtPointer)CP_CLOSE_BTN);

    n = 0;
    XtSetArg(args[n],XmNautoUnmanage,False); n++;
    XtSetArg(args[n],XmNmarginHeight,8); n++;
    XtSetArg(args[n],XmNmarginWidth,8); n++;
    XtSetArg(args[n],XmNdialogStyle,XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
    cpAxisForm = XmCreateForm(shell,"cartesianPlotAxisF",args,n);

    n = 0;
    XtSetArg(args[n],XmNnumColumns,1); n++;
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_COLUMN); n++;
    entriesRC = XmCreateRowColumn(cpAxisForm,"entriesRC",args,n);

    axisStyleXmString = XmStringCreateSimple("Axis Style");
    axisRangeXmString = XmStringCreateSimple("Axis Range");
    axisMinXmString = XmStringCreateSimple("Minimum Value");
    axisMaxXmString = XmStringCreateSimple("Maximum Value");
    axisTimeFmtXmString = XmStringCreateSimple("Time format");

    counter = 0;
  /* ---------------------------------------------------------------- */
    for (i = X_AXIS_ELEMENT /* 0 */; i <= Y2_AXIS_ELEMENT /* 2 */; i++) {

	n = 0;
	XtSetArg(args[n],XmNshadowType,XmSHADOW_ETCHED_IN); n++;
	frame = XmCreateFrame(entriesRC,"frame",args,n);
	XtManageChild(frame);

	frameLabelXmString = XmStringCreateSimple(frameLabelString[i]);
	n = 0;
	XtSetArg(args[n],XmNlabelString,frameLabelXmString); n++;
	XtSetArg(args[n],XmNmarginWidth,0); n++;
	XtSetArg(args[n],XmNmarginHeight,0); n++;
	XtSetArg(args[n],XmNchildType,XmFRAME_TITLE_CHILD); n++;
      /* use font calculation for textField (which uses ~90% of height) */
	XtSetArg(args[n],XmNfontList,fontListTable[textFieldFontListIndex(24)]);n++;
	localLabel = XmCreateLabel(frame,"label",args,n);
	XtManageChild(localLabel);
	XmStringFree(frameLabelXmString);

      /* parent RC within frame */
	n = 0;
	XtSetArg(args[n],XmNnumColumns,1); n++;
	XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
	XtSetArg(args[n],XmNpacking,XmPACK_COLUMN); n++;
	parentRC = XmCreateRowColumn(frame,"parentRC",args,n);
	XtManageChild(parentRC);

      /* Create Axis Style Entry */
	createCartesianPlotAxisDialogMenuEntry(
	  parentRC,
	  axisStyleXmString,
	  &(entryLabel[counter]),
	  &(entryElement[counter]),
	  &(xmStringValueTable[FIRST_CARTESIAN_PLOT_AXIS_STYLE]),
	  buttonType,
	  (!i)?NUM_CARTESIAN_PLOT_AXIS_STYLES:NUM_CARTESIAN_PLOT_AXIS_STYLES-1,
	  (XtPointer)(CP_X_AXIS_STYLE+i));
	axisStyleMenu[i] =  entryElement[counter];
	counter++;

      /* Create Range Style Entry */
	createCartesianPlotAxisDialogMenuEntry(
	  parentRC,
	  axisRangeXmString,
	  &(entryLabel[counter]),
	  &(entryElement[counter]),
	  &(xmStringValueTable[FIRST_CARTESIAN_PLOT_RANGE_STYLE]),
	  buttonType,
	  NUM_CARTESIAN_PLOT_RANGE_STYLES,
	  (XtPointer)(CP_X_RANGE_STYLE+i));
	axisRangeMenu[i] =  entryElement[counter];
	counter++;

      /* create Min text field entry */
	createCartesianPlotAxisDialogTextEntry(
	  parentRC, axisMinXmString,
	  &(axisRangeMinRC[i]), &(entryLabel[counter]),
	  &(entryElement[counter]), (XtPointer)(CP_X_RANGE_MIN+i));
	axisRangeMin[i] = entryElement[counter];
	counter++;
 
      /* Create Max text field entry */
	createCartesianPlotAxisDialogTextEntry(
	  parentRC, axisMaxXmString,
	  &(axisRangeMaxRC[i]), &(entryLabel[counter]),
	  &(entryElement[counter]), (XtPointer)(CP_X_RANGE_MIN+i));
	axisRangeMax[i] = entryElement[counter];
	counter++;

	if (i == X_AXIS_ELEMENT) {
	  /* create time format menu entry */

	    createCartesianPlotAxisDialogMenuEntry(
	      parentRC,
	      axisTimeFmtXmString,
	      &(entryLabel[counter]),
	      &(entryElement[counter]),
	      &(xmStringValueTable[FIRST_CP_TIME_FORMAT]),
	      buttonType,
	      NUM_CP_TIME_FORMAT,
	      (XtPointer)(CP_X_TIME_FORMAT));
	    axisTimeFormat =  entryElement[counter];
	    counter++;
	}
    }

    for (i = 0; i < counter; i++) {
	XtVaGetValues(entryLabel[i],XmNwidth,&width,XmNheight,&height,NULL);
	maxLabelWidth = MAX(maxLabelWidth,width);
	maxLabelHeight = MAX(maxLabelHeight,height);
	XtVaGetValues(entryElement[i],XmNwidth,&width,XmNheight,&height,NULL);
	maxLabelWidth = MAX(maxLabelWidth,width);
	maxLabelHeight = MAX(maxLabelHeight,height);
	XtManageChild(entryLabel[i]);
	XtManageChild(entryElement[i]);
    }

  /*
   * now resize the labels and elements (to maximum's width)
   *      for uniform appearance
   */
    for (i = 0; i < counter; i++) {

      /* set label */
	XtVaSetValues(entryLabel[i],XmNwidth,maxLabelWidth,
	  XmNheight,maxLabelHeight,XmNrecomputeSize,False,
	  XmNalignment,XmALIGNMENT_END,NULL);

      /* set element */
	if (XtClass(entryElement[i]) == xmRowColumnWidgetClass) {
	  /* must be option menu - unmanage label widget */
	    XtVaSetValues(XmOptionButtonGadget(entryElement[i]),
	      XmNx,(Position)maxLabelWidth, XmNwidth,maxLabelWidth,
	      XmNheight,maxLabelHeight,
	      XmNrecomputeSize,False, XmNresizeWidth,True,
	      XmNmarginWidth,0,
	      NULL);
	}

	XtVaSetValues(entryElement[i],
	  XmNx,(Position)maxLabelWidth, XmNwidth,maxLabelWidth,
	  XmNheight,maxLabelHeight,
	  XmNrecomputeSize,False, XmNresizeWidth,True,
	  XmNmarginWidth,0,
	  NULL);
	XtManageChild(entryLabel[i]);
	XtManageChild(entryElement[i]);
    }
    XtManageChild(entriesRC);

  /* ---------------------------------------------------------------- */
    XmStringFree(axisStyleXmString);
    XmStringFree(axisRangeXmString);
    XmStringFree(axisMinXmString);
    XmStringFree(axisMaxXmString);


    xmString = XmStringCreateSimple("Close");
    n = 0;
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    closeButton = XmCreatePushButton(cpAxisForm,"closeButton",args,n);
    XtAddCallback(closeButton,XmNactivateCallback,
      cartesianPlotAxisActivate, (XtPointer)CP_CLOSE_BTN);
    XtManageChild(closeButton);
    XmStringFree(xmString);

  /*
   * now do form layout 
   */
    
  /* entriesRC */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
    XtSetValues(entriesRC,args,n);

  /* close */

    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,entriesRC); n++;
    XtSetArg(args[n],XmNtopOffset,12); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomOffset,12); n++;
  /* HACK - approximate centering button by putting at 43% of form width */
    XtSetArg(args[n],XmNleftPosition,(Position)43); n++;
    XtSetValues(closeButton,args,n);


    return (shell);
}

/*
 * access function (for file-scoped globals, etc) to udpate the
 *	cartesian plot axis dialog with the values currently in
 *	globalResourceBundle
 */
void updateCartesianPlotAxisDialog()
{
    int i, tail;
    char string[MAX_TOKEN_LENGTH];

    for (i = X_AXIS_ELEMENT; i <= Y2_AXIS_ELEMENT; i++) {
	optionMenuSet(axisStyleMenu[i], globalResourceBundle.axis[i].axisStyle
	  - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
	optionMenuSet(axisRangeMenu[i], globalResourceBundle.axis[i].rangeStyle
	  - FIRST_CARTESIAN_PLOT_RANGE_STYLE);
	if (globalResourceBundle.axis[i].rangeStyle == USER_SPECIFIED_RANGE) {
	    sprintf(string,"%f",globalResourceBundle.axis[i].minRange);
	  /* strip trailing zeroes */
	    tail = strlen(string);
	    while (string[--tail] == '0') string[tail] = '\0';
	    XmTextFieldSetString(axisRangeMin[i],string);
	    sprintf(string,"%f",globalResourceBundle.axis[i].maxRange);
	  /* strip trailing zeroes */
	    tail = strlen(string);
	    while (string[--tail] == '0') string[tail] = '\0';
	    XmTextFieldSetString(axisRangeMax[i],string);
	    XtSetSensitive(axisRangeMinRC[i],True);
	    XtSetSensitive(axisRangeMaxRC[i],True);
	} else {
	    XtSetSensitive(axisRangeMinRC[i],False);
	    XtSetSensitive(axisRangeMaxRC[i],False);
	}
    }
    if (globalResourceBundle.axis[0].axisStyle == TIME_AXIS) {
	XtSetSensitive(axisTimeFormat,True);
	optionMenuSet(axisTimeFormat,globalResourceBundle.axis[0].timeFormat
	  - FIRST_CP_TIME_FORMAT);
    } else {
	XtSetSensitive(axisTimeFormat,False);
    }
}

/*
 * access function (for file-scoped globals, etc) to udpate the
 *      cartesian plot axis dialog with the values currently in
 *      the subject cartesian plot
 */
void updateCartesianPlotAxisDialogFromWidget(Widget cp)
{
    int tail, buttonId;
    char string[MAX_TOKEN_LENGTH];
    CartesianPlot *pcp;
    XtPointer userData;
    Boolean xAxisIsLog, y1AxisIsLog, y2AxisIsLog,
      xMinUseDef, y1MinUseDef, y2MinUseDef,
      xIsCurrentlyFromChannel, y1IsCurrentlyFromChannel,
      y2IsCurrentlyFromChannel;
    XrtAnnoMethod xAnnoMethod;
    XcVType xMinF, xMaxF, y1MinF, y1MaxF, y2MinF, y2MaxF;
    Arg args[2];

    if (globalDisplayListTraversalMode != DL_EXECUTE) return;
    XtVaGetValues(cp,
      XmNuserData,&userData,
      XtNxrtXAnnotationMethod, &xAnnoMethod,
      XtNxrtXAxisLogarithmic,&xAxisIsLog,
      XtNxrtYAxisLogarithmic,&y1AxisIsLog,
      XtNxrtY2AxisLogarithmic,&y2AxisIsLog,
      XtNxrtXMin,&xMinF.lval,
      XtNxrtYMin,&y1MinF.lval,
      XtNxrtY2Min,&y2MinF.lval,
      XtNxrtXMax,&xMaxF.lval,
      XtNxrtYMax,&y1MaxF.lval,
      XtNxrtY2Max,&y2MaxF.lval,
      XtNxrtXMinUseDefault,&xMinUseDef,
      XtNxrtYMinUseDefault,&y1MinUseDef,
      XtNxrtY2MinUseDefault,&y2MinUseDef,
      NULL);

    if (pcp = (CartesianPlot *) userData ) {
	xIsCurrentlyFromChannel =
	  pcp->axisRange[X_AXIS_ELEMENT].isCurrentlyFromChannel;
	y1IsCurrentlyFromChannel =
	  pcp->axisRange[Y1_AXIS_ELEMENT].isCurrentlyFromChannel;
	y2IsCurrentlyFromChannel =
	  pcp->axisRange[Y2_AXIS_ELEMENT].isCurrentlyFromChannel;
    }

  /* X Axis */
    if (xAnnoMethod == XRT_ANNO_TIME_LABELS)  {
	optionMenuSet(axisStyleMenu[X_AXIS_ELEMENT],
	  TIME_AXIS - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
	optionMenuSet(axisRangeMenu[X_AXIS_ELEMENT],
	  AUTO_SCALE_RANGE - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
	XtSetSensitive(axisTimeFormat,True);
    } else {
	optionMenuSet(axisStyleMenu[X_AXIS_ELEMENT],
	  (xAxisIsLog ? LOG10_AXIS : LINEAR_AXIS)
	  - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
	buttonId = (xMinUseDef ? AUTO_SCALE_RANGE :
	  (xIsCurrentlyFromChannel ? CHANNEL_RANGE : USER_SPECIFIED_RANGE)
	  - FIRST_CARTESIAN_PLOT_RANGE_STYLE);
	optionMenuSet(axisRangeMenu[X_AXIS_ELEMENT], buttonId);
	if (buttonId == USER_SPECIFIED_RANGE - FIRST_CARTESIAN_PLOT_RANGE_STYLE) {
	    sprintf(string,"%f",xMinF.fval);
	  /* strip trailing zeroes */
	    tail = strlen(string);
	    while (string[--tail] == '0') string[tail] = '\0';
	    XmTextFieldSetString(axisRangeMin[X_AXIS_ELEMENT],string);
	    sprintf(string,"%f",xMaxF.fval);
	  /* strip trailing zeroes */
	    tail = strlen(string);
	    while (string[--tail] == '0') string[tail] = '\0';
	    XmTextFieldSetString(axisRangeMax[X_AXIS_ELEMENT],string);
	}
	XtSetSensitive(axisTimeFormat,False);
    }
    if ((!xMinUseDef && !xIsCurrentlyFromChannel) ||
      (xAnnoMethod == XRT_ANNO_TIME_LABELS)) {
	XtSetSensitive(axisRangeMinRC[X_AXIS_ELEMENT],True);
	XtSetSensitive(axisRangeMaxRC[X_AXIS_ELEMENT],True);
    } else {
	XtSetSensitive(axisRangeMinRC[X_AXIS_ELEMENT],False);
	XtSetSensitive(axisRangeMaxRC[X_AXIS_ELEMENT],False);
    }


  /* Y1 Axis */
    optionMenuSet(axisStyleMenu[Y1_AXIS_ELEMENT],
      (y1AxisIsLog ? LOG10_AXIS : LINEAR_AXIS)
      - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
    buttonId =  (y1MinUseDef ? AUTO_SCALE_RANGE :
      (y1IsCurrentlyFromChannel ? CHANNEL_RANGE : USER_SPECIFIED_RANGE)
      - FIRST_CARTESIAN_PLOT_RANGE_STYLE);
    optionMenuSet(axisRangeMenu[Y1_AXIS_ELEMENT], buttonId);
    if (buttonId == USER_SPECIFIED_RANGE - FIRST_CARTESIAN_PLOT_RANGE_STYLE) {
	sprintf(string,"%f",y1MinF.fval);
      /* strip trailing zeroes */
	tail = strlen(string);
	while (string[--tail] == '0') string[tail] = '\0';
	XmTextFieldSetString(axisRangeMin[Y1_AXIS_ELEMENT],string);
	sprintf(string,"%f",y1MaxF.fval);
      /* strip trailing zeroes */
	tail = strlen(string);
	while (string[--tail] == '0') string[tail] = '\0';
	XmTextFieldSetString(axisRangeMax[Y1_AXIS_ELEMENT],string);
    }
    if (!y1MinUseDef && !y1IsCurrentlyFromChannel) {
	XtSetSensitive(axisRangeMinRC[Y1_AXIS_ELEMENT],True);
	XtSetSensitive(axisRangeMaxRC[Y1_AXIS_ELEMENT],True);
    } else {
	XtSetSensitive(axisRangeMinRC[Y1_AXIS_ELEMENT],False);
	XtSetSensitive(axisRangeMaxRC[Y1_AXIS_ELEMENT],False);
    }


  /* Y2 Axis */
    optionMenuSet(axisStyleMenu[Y2_AXIS_ELEMENT],
      (y1AxisIsLog ? LOG10_AXIS : LINEAR_AXIS)
      - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
    buttonId = (y2MinUseDef ? AUTO_SCALE_RANGE :
      (y2IsCurrentlyFromChannel ? CHANNEL_RANGE : USER_SPECIFIED_RANGE)
      - FIRST_CARTESIAN_PLOT_RANGE_STYLE);
    optionMenuSet(axisRangeMenu[Y2_AXIS_ELEMENT], buttonId);
    if (buttonId == USER_SPECIFIED_RANGE - FIRST_CARTESIAN_PLOT_RANGE_STYLE) {
	sprintf(string,"%f",y2MinF.fval);
      /* strip trailing zeroes */
	tail = strlen(string);
	while (string[--tail] == '0') string[tail] = '\0';
	XmTextFieldSetString(axisRangeMin[Y2_AXIS_ELEMENT],string);
	sprintf(string,"%f",y2MaxF.fval);
      /* strip trailing zeroes */
	tail = strlen(string);
	while (string[--tail] == '0') string[tail] = '\0';
	XmTextFieldSetString(axisRangeMax[Y2_AXIS_ELEMENT],string);
    }
    if (!y2MinUseDef && !y2IsCurrentlyFromChannel) {
	XtSetSensitive(axisRangeMinRC[Y2_AXIS_ELEMENT],True);
	XtSetSensitive(axisRangeMaxRC[Y2_AXIS_ELEMENT],True);
    } else {
	XtSetSensitive(axisRangeMinRC[Y2_AXIS_ELEMENT],False);
	XtSetSensitive(axisRangeMaxRC[Y2_AXIS_ELEMENT],False);
    }
}

void medmGetValues(ResourceBundle *pRB, ...) {
    va_list ap;
    int arg;
    va_start(ap, pRB);
    arg = va_arg(ap,int);
    while (arg >= 0) {
	switch (arg) {
	case X_RC : {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->x;
	    break;
	}
	case Y_RC : {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->y;
	    break;
	}
	case WIDTH_RC : {
	    unsigned int *pvalue = va_arg(ap,unsigned int *);
	    *pvalue = pRB->width;
	    break;
	}
	case HEIGHT_RC : {
	    unsigned int *pvalue = va_arg(ap,unsigned int *);
	    *pvalue = pRB->height;
	    break;
	}
	case RDBK_RC : {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->chan);
	    break;
	}
	case CTRL_RC : {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->chan);
	    break;
	}
	case TITLE_RC : {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->title);
	    break;
	}
	case XLABEL_RC : {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->xlabel);
	    break;
	}
	case YLABEL_RC : {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->ylabel);
	    break;
	}
	case CLR_RC : {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->clr;
	    break;
	}
	case BCLR_RC : {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->bclr;
	    break;
	}
	case BEGIN_RC : {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->begin;
	    break;
	}
	case PATH_RC : {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->path;
	    break;
	}
	case ALIGN_RC : {
	    TextAlign *pvalue = va_arg(ap,TextAlign *);
	    *pvalue = pRB->align;
	    break;
	}
	case FORMAT_RC : {
	    TextFormat *pvalue = va_arg(ap,TextFormat *);
	    *pvalue = pRB->format;
	    break;
	}
	case LABEL_RC : {
	    LabelType *pvalue = va_arg(ap,LabelType *);
	    *pvalue = pRB->label;
	    break;
	}
	case DIRECTION_RC : {
	    Direction *pvalue = va_arg(ap,Direction *);
	    *pvalue = pRB->direction;
	    break;
	}
	case FILLMOD_RC : {
	    FillMode *pvalue = va_arg(ap,FillMode *);
	    *pvalue = pRB->fillmod;
	    break;
	}
	case STYLE_RC : {
	    EdgeStyle *pvalue = va_arg(ap,EdgeStyle *);
	    *pvalue = pRB->style;
	    break;
	}
	case FILL_RC : {
	    FillStyle *pvalue = va_arg(ap,FillStyle *);
	    *pvalue = pRB->fill;
	    break;
	}
	case CLRMOD_RC : {
	    ColorMode *pvalue = va_arg(ap,ColorMode *);
	    *pvalue = pRB->clrmod;
	    break;
	}
#ifdef __COLOR_RULE_H__
	case COLOR_RULE_RC : {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->colorRule;
	    break;
	}
#endif
	case VIS_RC : {
	    VisibilityMode *pvalue = va_arg(ap,VisibilityMode *);
	    *pvalue = pRB->vis;
	    break;
	}
	case CHAN_RC : {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->chan);
	    break;
	}
	case DATA_CLR_RC : {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->data_clr;
	    break;
	}
	case DIS_RC : {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->dis;
	    break;
	}
	case XYANGLE_RC : {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->xyangle;
	    break;
	}
	case ZANGLE_RC : {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->zangle;
	    break;
	}
	case PERIOD_RC : {
	    double *pvalue = va_arg(ap,double *);
	    *pvalue = pRB->period;
	    break;
	}
	case UNITS_RC : {
	    TimeUnits *pvalue = va_arg(ap,TimeUnits *);
	    *pvalue = pRB->units;
	    break;
	}
	case CSTYLE_RC : {
	    CartesianPlotStyle *pvalue = va_arg(ap,CartesianPlotStyle *);
	    *pvalue = pRB->cStyle;
	    break;
	}
	case ERASE_OLDEST_RC : {
	    EraseOldest *pvalue = va_arg(ap,EraseOldest *);
	    *pvalue = pRB->erase_oldest;
	    break;
	}
	case COUNT_RC : {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->count;
	    break;
	}
	case STACKING_RC : {
	    Stacking *pvalue = va_arg(ap,Stacking *);
	    *pvalue = pRB->stacking;
	    break;
	}
	case IMAGETYPE_RC : {
	    ImageType *pvalue = va_arg(ap,ImageType *);
	    *pvalue = pRB->imageType;
	    break;
	}
	case TEXTIX_RC : {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->textix);
	    break;
	}
	case MSG_LABEL_RC : {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->messageLabel);
	    break;
	}
	case PRESS_MSG_RC : {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->press_msg);
	    break;
	}
	case RELEASE_MSG_RC : {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->release_msg);
	    break;
	}
	case IMAGENAME_RC : {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->imageName);
	    break;
	}
	case DATA_RC : {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->data);
	    break;
	}
	case CMAP_RC : {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->cmap);
	    break;
	}
	case NAME_RC : {
	    char *pvalue = va_arg(ap,char *);
	    strcpy(pvalue,pRB->name);
	    break;
	}
	case LINEWIDTH_RC : {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->lineWidth;
	    break;
	}
	case PRECISION_RC : {
	    double *pvalue = va_arg(ap,double *);
	    *pvalue = pRB->dPrecision;
	    break;
	}
	case SBIT_RC : {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->sbit;
	    break;
	}
	case EBIT_RC : {
	    int *pvalue = va_arg(ap,int *);
	    *pvalue = pRB->ebit;
	    break;
	}
	case RD_LABEL_RC : {
	    char* pvalue = va_arg(ap,char *);
	    strcpy(pvalue,globalResourceBundle.rdLabel);
	    break;
	}
	case RD_VISUAL_RC : {
	    relatedDisplayVisual_t *pvalue = va_arg(ap,relatedDisplayVisual_t *);
	    *pvalue = globalResourceBundle.rdVisual;
	    break;
	}
	case RDDATA_RC : {
	    DlRelatedDisplayEntry *pDisplay = va_arg(ap,DlRelatedDisplayEntry *);
	    int i;
	    for (i = 0; i < MAX_RELATED_DISPLAYS; i++){
		strcpy(pDisplay[i].label,globalResourceBundle.rdData[i].label);
		strcpy(pDisplay[i].name,globalResourceBundle.rdData[i].name);
		strcpy(pDisplay[i].args,globalResourceBundle.rdData[i].args);
		pDisplay[i].mode = globalResourceBundle.rdData[i].mode;
	    }
	    break;
	}
	case CPDATA_RC : {
	    DlTrace* ptrace = va_arg(ap,DlTrace *);
	    int i;
	    for (i = 0; i < MAX_TRACES; i++){
		strcpy(ptrace[i].xdata,globalResourceBundle.cpData[i].xdata);
		strcpy(ptrace[i].ydata,globalResourceBundle.cpData[i].ydata);
		ptrace[i].data_clr = globalResourceBundle.cpData[i].data_clr;
	    }
	    break;
	}
	case SCDATA_RC : {
	    DlPen *pPen = va_arg(ap, DlPen *);
	    int i;
	    for (i = 0; i < MAX_PENS; i++){
		strcpy(pPen[i].chan,pRB->scData[i].chan);
		pPen[i].clr = pRB->scData[i].clr;
	    }
	    break;
	}
	case SHELLDATA_RC : {
	    DlShellCommandEntry *pCommand = va_arg(ap, DlShellCommandEntry *);
	    int i;
	    for (i = 0; i < MAX_SHELL_COMMANDS; i++){
		strcpy(pCommand[i].label,globalResourceBundle.cmdData[i].label);
		strcpy(pCommand[i].command,globalResourceBundle.cmdData[i].command);
		strcpy(pCommand[i].args,globalResourceBundle.cmdData[i].args);
	    }
	    break;
	}
	case CPAXIS_RC : {
	    DlPlotAxisDefinition *paxis = va_arg(ap,DlPlotAxisDefinition *);
	    paxis[X_AXIS_ELEMENT] = globalResourceBundle.axis[X_AXIS_ELEMENT];
	    paxis[Y1_AXIS_ELEMENT] = globalResourceBundle.axis[Y1_AXIS_ELEMENT];
	    paxis[Y2_AXIS_ELEMENT] = globalResourceBundle.axis[Y2_AXIS_ELEMENT];
	    break;
	}
	case TRIGGER_RC : {
	    char* pvalue = va_arg(ap,char *);
	    strcpy(pvalue,globalResourceBundle.trigger);
	    break;
	}
	case ERASE_RC : {
	    char* pvalue = va_arg(ap,char *);
	    strcpy(pvalue,globalResourceBundle.erase);
	    break;
	}
	case ERASE_MODE_RC : {
	    eraseMode_t *pvalue = va_arg(ap,eraseMode_t *);
	    *pvalue = globalResourceBundle.eraseMode;
	    break;
	}
	default :
	    break;
	}
	arg = va_arg(ap,int);
    }
    va_end(ap);
    return;
}

#ifdef __cplusplus
static void helpResourceCallback(Widget, XtPointer cd, XtPointer cbs)
#else
static void helpResourceCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int buttonNumber = (int)cd;
    XmAnyCallbackStruct *call_data = (XmAnyCallbackStruct *)cbs;
    Widget widget;
    XEvent event;
    
    switch(buttonNumber) {
    case HELP_RESOURCE_PALETTE_BTN:
	callBrowser(MEDM_HELP_PATH"/MEDM.html#ResourcePalette");
	break;
    }
}

/* ********************************************************************
 * Routines formerly in objectPalette.c
 * ********************************************************************/

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
      currentDisplayInfo->colormap[globalResourceBundle.clr],NULL);
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
    if (dynAttr->chan) {
	strcpy(globalResourceBundle.chan,dynAttr->chan);
    } else {
	globalResourceBundle.chan[0] = '\0';
    }
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
      currentDisplayInfo->colormap[globalResourceBundle.clr],NULL);
    XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
      currentDisplayInfo->colormap[globalResourceBundle.bclr],NULL);
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
      currentDisplayInfo->colormap[globalResourceBundle.clr],NULL);
    XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
      currentDisplayInfo->colormap[globalResourceBundle.bclr],NULL);
}

/*
 * Clear resourcePalette dialog box
 */
void clearResourcePaletteEntries()
{
#if DEBUG_RESOURCE
    printf("In clearResourcePaletteEntries\n");
    if (currentElementType >= MIN_DL_ELEMENT_TYPE &&
      currentElementType <= MAX_DL_ELEMENT_TYPE) {
	printf("  currentElementType: %s (%d)\n",
	  elementType(currentElementType),currentElementType);
    } else {
	printf("  currentElementType: (%d) Valid Types [%d-%d]\n",
	  currentElementType,MIN_DL_ELEMENT_TYPE,MAX_DL_ELEMENT_TYPE);
    }
#endif
    
  /* If no resource palette yet, simply return */
    if (!resourceMW) return;
 
  /* Popdown any of the associated shells */
    if (relatedDisplayS)    XtPopdown(relatedDisplayS);
    if (shellCommandS)      XtPopdown(shellCommandS);
    if (cartesianPlotS)     XtPopdown(cartesianPlotS);
    if (cartesianPlotAxisS) XtPopdown(cartesianPlotAxisS);
    if (stripChartS)        XtPopdown(stripChartS);
 
  /* Unset the current button and set label in resourceMW to Select... */
    XtVaSetValues(resourceElementTypeLabel,XmNlabelString,xmstringSelect,NULL);
 
  /* Unmanage items in resource palette */
    if (currentElementType >= MIN_DL_ELEMENT_TYPE &&
      currentElementType <= MAX_DL_ELEMENT_TYPE) {
	int i = currentElementType-MIN_DL_ELEMENT_TYPE;
	XtUnmanageChildren(
	  resourcePaletteElements[i].children,
	  resourcePaletteElements[i].numChildren);
    }
}

/*
 * Set resourcePalette entries based on current type
 */
void setResourcePaletteEntries()
{
  /* Must normalize back to 0 as index into array for element type */
    XmString buttons[NUM_IMAGE_TYPES-1];
    XmButtonType buttonType[NUM_IMAGE_TYPES-1];
    Arg args[10];
    Boolean objectDataOnly;
    DlElementType displayType;

#if DEBUG_RESOURCE
    printf("In setResourcePaletteEntries\n");
    if (currentElementType >= MIN_DL_ELEMENT_TYPE &&
      currentElementType <= MAX_DL_ELEMENT_TYPE) {
	printf("  currentElementType: %s (%d)\n",
	  elementType(currentElementType),currentElementType);
    } else {
	printf("  currentElementType: (%d) Valid Types [%d-%d]\n",
	  currentElementType,MIN_DL_ELEMENT_TYPE,MAX_DL_ELEMENT_TYPE);
    }
#endif
    
  /* If no resource palette yet, create it */
    if (!resourceMW) createResource();
    
  /* Make sure the resource palette shell is popped-up */
    XtPopup(resourceS,XtGrabNone);

  /* Check if this is a valid element type */
    if (currentElementType < MIN_DL_ELEMENT_TYPE ||
      currentElementType > MAX_DL_ELEMENT_TYPE ||
      IsEmpty(currentDisplayInfo->selectedDlElementList)) {
	clearResourcePaletteEntries();
/* 	resetGlobalResourceBundleAndResourcePalette(); */
	return;
    }

  /* Make these sensitive in case they are managed */
    XtSetSensitive(resourceEntryRC[VIS_RC],True);
    XtSetSensitive(resourceEntryRC[CLRMOD_RC],True);
#ifdef __COLOR_RULE_H__
    XtSetSensitive(resourceEntryRC[COLOR_RULE_RC],True);
#endif

  /* Setting the new button: manage new resource entries */
    XtManageChildren(
      resourcePaletteElements[currentElementType -
	MIN_DL_ELEMENT_TYPE].children,
      resourcePaletteElements[currentElementType -
	MIN_DL_ELEMENT_TYPE].numChildren);

  /* If polyline with 2 points display Line as label, not Polyline */
    displayType = currentElementType;
    if ((currentDisplayInfo->selectedDlElementList->count == 1) &&
      (currentElementType == DL_Polyline) &&
      (FirstDlElement(currentDisplayInfo->selectedDlElementList)->
	structure.element->structure.polyline->nPoints == 2))
      displayType = DL_Line;
    XtVaSetValues(resourceElementTypeLabel,
      XmNlabelString,elementXmStringTable[displayType-MIN_DL_ELEMENT_TYPE],
      NULL);

  /* Update all resource palette parameters */
    objectDataOnly = False;
    updateGlobalResourceBundleAndResourcePalette(objectDataOnly);

  /* If not a monitor or controller type object, and no  dynamics channel
   * specified, then insensitize the related entries */
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

  /* Make these sensitive in case they are managed */
    if (strlen(globalResourceBundle.erase) == 0)
      XtSetSensitive(resourceEntryRC[ERASE_MODE_RC],False);
    else
      XtSetSensitive(resourceEntryRC[ERASE_MODE_RC],True);
}

void updateElementFromGlobalResourceBundle(DlElement *element)
{
    DlElement *childE;
    DisplayInfo *cdi = currentDisplayInfo;

#if DEBUG_RESOURCE
    printf("In updateElementFromGlobalResourceBundle\n");
#endif
    
  /* Simply return if not valid to update */
    if (!element || !cdi) return;
    
  /* Copy (all) vales from resource palette to element */
    if (element->run->getValues) {
	element->run->getValues(&globalResourceBundle,element);
    }
    if (element->widget) {
      /* Need to destroy, then create to get it right */
	destroyElementWidgets(element);
	(element->run->execute)(cdi,element);
    } else if (element->type == DL_Display) {
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
    printf("In updateElementBackgroundColorFromGlobalResourceBundle\n");
#endif
    
  /* Simply return if not valid to update */
    if (!element || !cdi) return;
    
  /* Check if composite */
    if(element->type == DL_Composite) {
      /* Composite, loop over contained elements */
	DlComposite *compE = element->structure.composite;
	
	childE = FirstDlElement(compE->dlElementList);
	while (childE) {
	    if (childE->run->setBackgroundColor) {
		childE->run->setBackgroundColor(&globalResourceBundle,childE);
	    }
	    if (childE->widget) {
		XtVaSetValues(childE->widget,XmNbackground,
		  currentColormap[globalResourceBundle.bclr],NULL);
	      /* Need to destroy, then create to get it right */
		destroyElementWidgets(childE);
		(childE->run->execute)(cdi,childE);
	    }
	  /* If drawingArea: update drawingAreaForegroundColor */
	    if (childE->type == DL_Display) {
		cdi->drawingAreaBackgroundColor = globalResourceBundle.bclr;
	    }
	    childE = childE->next;
	}
    } else {
      /* Not composite */
	if (element->run->setBackgroundColor) {
	    element->run->setBackgroundColor(&globalResourceBundle,element);
	}
	if (element->widget) {
	    XtVaSetValues(element->widget,XmNbackground,
	      currentColormap[globalResourceBundle.bclr],NULL);
	  /* Need to destroy, then create to get it right */
	    destroyElementWidgets(element);
	    (element->run->execute)(cdi,element);
	}
      /* If drawingArea: update drawingAreaForegroundColor */
	if (element->type == DL_Display) {
	    cdi->drawingAreaBackgroundColor = globalResourceBundle.bclr;
	}
    }
}

void updateElementForegroundColorFromGlobalResourceBundle(DlElement *element)
{
    DlElement *childE;
    DisplayInfo *cdi = currentDisplayInfo;
    
#if DEBUG_RESOURCE
    printf("In updateElementForegroundColorFromGlobalResourceBundle\n");
#endif
    
  /* Simply return if not valid to update */
    if (!element || !cdi) return;
    
  /* Check if composite */
    if(element->type == DL_Composite) {
      /* Composite, loop over contained elements */
	DlComposite *compE = element->structure.composite;
	
	childE = FirstDlElement(compE->dlElementList);
	while (childE) {
	    if (childE->run->setForegroundColor) {
		childE->run->setForegroundColor(&globalResourceBundle,childE);
	    }
	    if (childE->widget) {
		XtVaSetValues(childE->widget,XmNforeground,
		  currentColormap[globalResourceBundle.clr],NULL);
	      /* Need to destroy, then create to get it right */
		destroyElementWidgets(childE);
		(childE->run->execute)(currentDisplayInfo,childE);
	    }
	  /* If drawingArea: update drawingAreaForegroundColor */
	    if (childE->type == DL_Display) {
		cdi->drawingAreaForegroundColor = globalResourceBundle.clr;
	    }
	    childE = childE->next;
	}
    } else {
      /* Not composite */
	if (element->run->setForegroundColor) {
	    element->run->setForegroundColor(&globalResourceBundle,element);
	}
	if (element->widget) {
	    XtVaSetValues(element->widget,XmNforeground,
	      currentColormap[globalResourceBundle.clr],NULL);
	  /* Need to destroy, then create to get it right */
	    destroyElementWidgets(element);
	    (element->run->execute)(currentDisplayInfo,element);
	  /* If drawingArea: update drawingAreaForegroundColor */
	    if (element->type == DL_Display) {
		cdi->drawingAreaForegroundColor = globalResourceBundle.clr;
	    }
	}
    }
}

void updateGlobalResourceBundleFromElement(DlElement *element) {
    DlCartesianPlot *p;
    int i;

#if DEBUG_RESOURCE
    printf("In updateGlobalResourceBundleFromElement\n");
#endif

    if (!element || (element->type != DL_CartesianPlot)) return;
    p = element->structure.cartesianPlot;

    for (i = X_AXIS_ELEMENT; i <= Y2_AXIS_ELEMENT; i++) {
	globalResourceBundle.axis[i].axisStyle = p->axis[i].axisStyle;
	globalResourceBundle.axis[i].rangeStyle = p->axis[i].rangeStyle;
	globalResourceBundle.axis[i].minRange = p->axis[i].minRange;
	globalResourceBundle.axis[i].maxRange = p->axis[i].maxRange;
    }
}

/*
 * function to clear/reset the global resource bundle data structure
 *	and to put those resource values into the resourcePalette
 *	elements (for the specified element type)
 */

void updateGlobalResourceBundleAndResourcePalette(Boolean objectDataOnly) {
    DlElement *elementPtr;
    char string[MAX_TOKEN_LENGTH];
    int i, tail;

#if DEBUG_RESOURCE
    printf("In updateGlobalResourceBundleAndResourcePaletteo\n");
#endif

  /* simply return if not valid to update */
    if (currentDisplayInfo->selectedDlElementList->count != 1) return;

    elementPtr = FirstDlElement(currentDisplayInfo->selectedDlElementList);
    elementPtr = elementPtr->structure.element;

  /* if no resource palette yet, create it */
    if (!resourceMW) {
	currentElementType = elementPtr->type;
	setResourcePaletteEntries();
	return;
    }

    switch (elementPtr->type) {
    case DL_Display: {
	DlDisplay *p = elementPtr->structure.display;
	Arg args[2];
	int nargs;
	Position x, y;
	
      /* Get the current values */
	nargs=0;
	XtSetArg(args[nargs],XmNx,&x); nargs++;
	XtSetArg(args[nargs],XmNy,&y); nargs++;
	XtGetValues(currentDisplayInfo->shell,args,nargs);

      /* Set the attributes in case they haven't been set */
	p->object.x = x;
	p->object.y = y;

	updateGlobalResourceBundleObjectAttribute(
	  &(elementPtr->structure.display->object));
	updateResourcePaletteObjectAttribute();
	if (objectDataOnly) return;
	globalResourceBundle.clr = elementPtr->structure.display->clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
	  currentDisplayInfo->colormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr = elementPtr->structure.display->bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
	  currentDisplayInfo->colormap[globalResourceBundle.bclr],NULL);
	strcpy(globalResourceBundle.cmap,elementPtr->structure.display->cmap);
	XmTextFieldSetString(resourceEntryElement[CMAP_RC],
	  globalResourceBundle.cmap);
	break;
    }
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
	  currentDisplayInfo->colormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr = p->plotcom.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
	  currentDisplayInfo->colormap[globalResourceBundle.bclr],NULL);
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
	  currentDisplayInfo->colormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr = p->plotcom.bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
	  currentDisplayInfo->colormap[globalResourceBundle.bclr],NULL);
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

#if DEBUG_RESOURCE
        printf("\n[updateGlobalResourceBundleAndResourcePalette] selectedDlElementList:\n");
        dumpDlElementList(currentDisplayInfo->selectedDlElementList);
#endif

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
          currentDisplayInfo->colormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr = p->bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
          currentDisplayInfo->colormap[globalResourceBundle.bclr],NULL);
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
	if (objectDataOnly) return;

	globalResourceBundle.clr = p->clr;
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
	  currentDisplayInfo->colormap[globalResourceBundle.clr],NULL);
	globalResourceBundle.bclr = p->bclr;
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
	  currentDisplayInfo->colormap[globalResourceBundle.bclr],NULL);
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

      /* Set colors explicitly */
#if 0	  
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
	  BlackPixel(display,screenNum),NULL);
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
	  WhitePixel(display,screenNum),NULL);
#else	
	XtVaSetValues(resourceEntryElement[CLR_RC],XmNbackground,
	  defaultBackground,NULL);
	XtVaSetValues(resourceEntryElement[BCLR_RC],XmNbackground,
	  defaultBackground,NULL);
#endif	
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

void resetGlobalResourceBundleAndResourcePalette()
{
    char string[MAX_TOKEN_LENGTH];


#if DEBUG_RESOURCE
    printf("In resetGlobalResourceBundleAndResourcePalette\n");
#endif
    
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
