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

/****************************************************************************
 * resourcePalette.c - Resource Palette                                     *
 * Mods: MDA - Creation                                                     *
 *       DMW - Tells resource palette which global resources Byte needs     *
 ****************************************************************************/
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
# define N_MAIN_MENU_ELES 3
# define N_FILE_MENU_ELES 5
# define FILE_BTN_POSN 0
# define FILE_OPEN_BTN	 0
# define FILE_SAVE_BTN	 1
# define FILE_SAVE_AS_BTN 2
# define FILE_CLOSE_BTN	 3
#else
# define N_MAIN_MENU_ELES 2
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

#define RD_APPLY_BTN	0
#define RD_CLOSE_BTN	1

#define CMD_APPLY_BTN	0
#define CMD_CLOSE_BTN	1

#define CP_XDATA_COLUMN		0
#define CP_YDATA_COLUMN		1
#define CP_COLOR_COLUMN		2

#define CP_APPLY_BTN	0
#define CP_CLOSE_BTN	1

#define SC_CHANNEL_COLUMN	0
#define SC_COLOR_COLUMN		1	

#define SC_APPLY_BTN	0
#define SC_CLOSE_BTN	1

#ifdef EXTENDED_INTERFACE
static Widget resourceFilePDM;
static Widget resourceBundlePDM, openFSD;
#endif
static Widget bundlesRB;
static Dimension maxLabelWidth = 0;
static Dimension maxLabelHeight = 0;

XmString xmstringSelect;

/****************************************************************************
 * CARTESIAN PLOT DATA
 *********************************************************************/
static Widget cpMatrix = NULL, cpForm = NULL;
static String cpColumnLabels[] = {"X Data","Y Data","Color",};
static int cpColumnMaxLengths[] = {MAX_TOKEN_LENGTH-1,MAX_TOKEN_LENGTH-1,6,};
static short cpColumnWidths[] = {36,36,6,};
static unsigned char cpColumnLabelAlignments[] = {XmALIGNMENT_CENTER,
					XmALIGNMENT_CENTER,XmALIGNMENT_CENTER,};
/* and the cpCells array of strings (filled in from globalResourceBundle...) */
static String cpRows[MAX_TRACES][3];
static String *cpCells[MAX_TRACES];
static String dashes = "******";

static Pixel cpColorRows[MAX_TRACES][3];
static Pixel *cpColorCells[MAX_TRACES];


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


/*********************************************************************
 * RELATED DISLAY DATA
 *********************************************************************/
static Widget rdMatrix = NULL, rdForm = NULL;
static String rdColumnLabels[] = {"Display Label","Display File","Arguments",};
static int rdColumnMaxLengths[] = {MAX_TOKEN_LENGTH-1,MAX_TOKEN_LENGTH-1,
					MAX_TOKEN_LENGTH-1,};
static short rdColumnWidths[] = {36,36,36,};
static unsigned char rdColumnLabelAlignments[] = {XmALIGNMENT_CENTER,
					XmALIGNMENT_CENTER,XmALIGNMENT_CENTER,};
/* and the rdCells array of strings (filled in from globalResourceBundle...) */
static String rdRows[MAX_RELATED_DISPLAYS][3];
static String *rdCells[MAX_RELATED_DISPLAYS];


/*********************************************************************
 * CARTESIAN PLOT AXIS DATA
 *********************************************************************/

/*
 * for the Cartesian Plot Axis Dialog, use the following static globals
 *   (N.B. This dialog has semantics for both EDIT and EXECUTE time
 *    operation)
 */

/* Widget cpAxisForm defined in medm.h since execute-time needs it too */

/* define array of widgets (for X, Y1, Y2) */
static Widget axisRangeMenu[3];			/* X_AXIS_ELEMENT =0 */
static Widget axisStyleMenu[3];			/* Y1_AXIS_ELEMENT=1 */
static Widget axisRangeMin[3], axisRangeMax[3];	/* Y2_AXIS_ELEMENT=2 */
static Widget axisRangeMinRC[3], axisRangeMaxRC[3];

#define CP_AXIS_STYLE	0
#define CP_RANGE_STYLE	(CP_AXIS_STYLE + 3)
#define CP_RANGE_MIN	(CP_RANGE_STYLE + 3)
#define CP_RANGE_MAX	(CP_RANGE_MIN + 3)

#define MAX_CP_AXIS_ELEMENTS	20
#define MAX_CP_AXIS_BUTTONS	MAX(NUM_CARTESIAN_PLOT_RANGE_STYLES,\
				    NUM_CARTESIAN_PLOT_AXIS_STYLES)


static void createResourceEntries(Widget entriesSW);
static void createEntryRC(Widget parent, int rcType);
static void initializeResourcePaletteElements();
static void createResourceBundles(Widget bundlesSW);
static void createBundleTB(Widget bundlesRB, char *name);

/****************************************************************************
 ****************************************************************************/

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
static void pushButtonActivateCallback(Widget w, XtPointer cd, XtPointer) {
#else
static void pushButtonActivateCallback(Widget w, XtPointer cd, XtPointer cbs) {
#endif
  int rcType = (int) cd;

    switch (rcType) {
      case RDDATA_RC:
	if (relatedDisplayS == NULL) {
          relatedDisplayS = createRelatedDisplayDataDialog(w);
	}
	/* update related display data from globalResourceBundle */
	updateRelatedDisplayDataDialog();
	XtManageChild(rdForm);
	XtPopup(relatedDisplayS,XtGrabNone);
	break;
      case SHELLDATA_RC:
	if (shellCommandS == NULL) {
	   shellCommandS = createShellCommandDataDialog(w);
	}
	/* update shell command data from globalResourceBundle */
	updateShellCommandDataDialog();
	XtManageChild(cmdForm);
	XtPopup(shellCommandS,XtGrabNone);
	break;
      case CPDATA_RC:
	if (cartesianPlotS == NULL) {
	   cartesianPlotS = createCartesianPlotDataDialog(w);
	}
	/* update cartesian plot data from globalResourceBundle */
	updateCartesianPlotDataDialog();
	XtManageChild(cpForm);
	XtPopup(cartesianPlotS,XtGrabNone);
	break;
      case SCDATA_RC:
	if (stripChartS == NULL) {
	   stripChartS = createStripChartDataDialog(w);
	}
	/* update strip chart data from globalResourceBundle */
	updateStripChartDataDialog();
	XtManageChild(scForm);
	XtPopup(stripChartS,XtGrabNone);
	break;
     case CPAXIS_RC:
	if (cartesianPlotAxisS == NULL) {
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
static void optionMenuSimpleCallback(Widget w, XtPointer cd, XtPointer) {
#else
static void optionMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs) {
#endif
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
    case CLRMOD_RC: 
       globalResourceBundle.clrmod = (ColorMode) (FIRST_COLOR_MODE + buttonId);
       break;
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

    default:
       medmPrintf("\noptionMenuSimpleCallback: unknown rcType = %d",rcType);
       break;
  }

/****** Update elements (this is overkill, but okay for now)
 *	-- not as efficient as it should be (don't update EVERYTHING if only
 *	   one item changed!) */
    if (currentDisplayInfo != NULL) {

/****** Unhighlight (since objects may move) */
    unhighlightSelectedElements();

    for (i = 0; i < currentDisplayInfo->numSelectedElements; i++) {
      elementPtr = currentDisplayInfo->selectedElementsArray[i];
      updateElementFromGlobalResourceBundle(elementPtr);
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
static void cpAxisOptionMenuSimpleCallback(Widget w, XtPointer cd, XtPointer) {
#else
static void cpAxisOptionMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs) {
#endif
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
    if (executeTimeCartesianPlotWidget != NULL) {
      XtSetArg(args[0],XmNuserData,&userData);
      XtGetValues(executeTimeCartesianPlotWidget,args,1);
      pcp = (CartesianPlot *) userData;
      if (pcp)
	  dlCartesianPlot = (DlCartesianPlot *) pcp->dlCartesianPlot;
    }
  }

/* rcType (and therefore which option menu...) is stored in userData */
  XtVaGetValues(XtParent(w),XmNuserData,&rcType,NULL);
  n = 0;
  switch (rcType/3) {
    case CP_AXIS_STYLE/3: 
       globalResourceBundle.axis[rcType%3].axisStyle
		= (CartesianPlotAxisStyle) (FIRST_CARTESIAN_PLOT_AXIS_STYLE + buttonId);
       if (globalDisplayListTraversalMode == DL_EXECUTE) {
	 switch(globalResourceBundle.axis[rcType%3].axisStyle) {
	    case LINEAR_AXIS:
		if (rcType%3 == X_AXIS_ELEMENT) {
		    XtSetArg(args[n],XtNxrtXAxisLogarithmic,False); n++;
		} else if (rcType%3 == Y1_AXIS_ELEMENT) {
		    XtSetArg(args[n],XtNxrtYAxisLogarithmic,False); n++;
		} else if (rcType%3 == Y2_AXIS_ELEMENT) {
		    XtSetArg(args[n],XtNxrtY2AxisLogarithmic,False); n++;
		}
		break;
	    case LOG10_AXIS:
		if (rcType%3 == X_AXIS_ELEMENT) {
		    XtSetArg(args[n],XtNxrtXAxisLogarithmic,True); n++;
		} else if (rcType%3 == Y1_AXIS_ELEMENT) {
		    XtSetArg(args[n],XtNxrtYAxisLogarithmic,True); n++;
		} else if (rcType%3 == Y2_AXIS_ELEMENT) {
		    XtSetArg(args[n],XtNxrtY2AxisLogarithmic,True); n++;
		}
		break;
	 }
       }
       break;

    case CP_RANGE_STYLE/3: 
       globalResourceBundle.axis[rcType%3].rangeStyle
		= (CartesianPlotRangeStyle) (FIRST_CARTESIAN_PLOT_RANGE_STYLE + buttonId);

       if (globalResourceBundle.axis[rcType%3].rangeStyle
					== USER_SPECIFIED_RANGE) {

	  XtSetSensitive(axisRangeMinRC[rcType%3],True);
	  XtSetSensitive(axisRangeMaxRC[rcType%3],True);
	  if (globalDisplayListTraversalMode == DL_EXECUTE) {
	     if (dlCartesianPlot != NULL) /* get min from element if possible */
		minF.fval = dlCartesianPlot->axis[rcType%3].minRange;
	     else
		minF.fval = globalResourceBundle.axis[rcType%3].minRange;

	     sprintf(string,"%f",minF.fval);
	     XmTextFieldSetString(axisRangeMin[rcType%3],string);
	     switch(rcType%3) {
		case X_AXIS_ELEMENT: resourceName = XtNxrtXMin; break;
		case Y1_AXIS_ELEMENT: resourceName = XtNxrtYMin; break;
		case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2Min; break;
	     }
	     XtSetArg(args[n],resourceName,minF.lval); n++;
	     if (dlCartesianPlot != NULL) /* get max from element if possible */
		maxF.fval = dlCartesianPlot->axis[rcType%3].maxRange;
	     else
		maxF.fval = globalResourceBundle.axis[rcType%3].maxRange;

	     sprintf(string,"%f",maxF.fval);
	     XmTextFieldSetString(axisRangeMax[rcType%3],string);
	     switch(rcType%3) {
		case X_AXIS_ELEMENT: resourceName = XtNxrtXMax; break;
		case Y1_AXIS_ELEMENT: resourceName = XtNxrtYMax; break;
		case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2Max; break;
	     }
	     XtSetArg(args[n],resourceName,maxF.lval); n++;
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
	     sprintf(string,"%f",tickF.fval);
	     k = strlen(string)-1;
	     while (string[k] == '0') k--;	/* strip off trailing zeroes */
	     iPrec = k;
	     while (string[k] != '.' && k >= 0) k--;
	     iPrec = iPrec - k;
	     XtSetArg(args[n],resourceName,iPrec); n++;
	  }
	  if (pcp) pcp->axisRange[rcType%3].isCurrentlyFromChannel = False;
       } else if (globalResourceBundle.axis[rcType%3].rangeStyle
					== CHANNEL_RANGE) {

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
	      case X_AXIS_ELEMENT: resourceName = XtNxrtXMin; break;
	      case Y1_AXIS_ELEMENT: resourceName = XtNxrtYMin; break;
	      case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2Min; break;
	  }
	  XtSetArg(args[n],resourceName,minF.lval); n++;
	  switch(rcType%3) {
	      case X_AXIS_ELEMENT: resourceName = XtNxrtXMax; break;
	      case Y1_AXIS_ELEMENT: resourceName = XtNxrtYMax; break;
	      case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2Max; break;
	  }
	  XtSetArg(args[n],resourceName,maxF.lval); n++;
	  if (pcp) pcp->axisRange[rcType%3].isCurrentlyFromChannel = True;
	  switch(rcType%3) {
		case X_AXIS_ELEMENT: resourceName = XtNxrtXTickUseDefault;
							break;
		case Y1_AXIS_ELEMENT: resourceName = XtNxrtYTickUseDefault;
							break;
		case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2TickUseDefault;
							break;
	  }
	  XtSetArg(args[n],resourceName,True); n++;
	  switch(rcType%3) {
		case X_AXIS_ELEMENT: resourceName = XtNxrtXNumUseDefault; break;
		case Y1_AXIS_ELEMENT: resourceName = XtNxrtYNumUseDefault;
							break;
		case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2NumUseDefault;
							break;
	  }
	  XtSetArg(args[n],resourceName,True); n++;
	  switch(rcType%3) {
		case X_AXIS_ELEMENT: resourceName = XtNxrtXPrecisionUseDefault;
							break;
		case Y1_AXIS_ELEMENT: resourceName = XtNxrtYPrecisionUseDefault;
							break;
		case Y2_AXIS_ELEMENT: resourceName =XtNxrtY2PrecisionUseDefault;
							 break;
	  }
	  XtSetArg(args[n],resourceName,True); n++;

       } else if (globalResourceBundle.axis[rcType%3].rangeStyle
					== AUTO_SCALE_RANGE) {
	  XtSetSensitive(axisRangeMinRC[rcType%3],False);
	  XtSetSensitive(axisRangeMaxRC[rcType%3],False);
	  if (globalDisplayListTraversalMode == DL_EXECUTE) {
	     switch(rcType%3) {
		case X_AXIS_ELEMENT: resourceName = XtNxrtXMinUseDefault;
			break;
		case Y1_AXIS_ELEMENT: resourceName = XtNxrtYMinUseDefault;
			break;
		case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2MinUseDefault;
			break;
	     }
	     XtSetArg(args[n],resourceName,True); n++;
	     switch(rcType%3) {
		case X_AXIS_ELEMENT: resourceName = XtNxrtXMaxUseDefault;
			break;
		case Y1_AXIS_ELEMENT: resourceName = XtNxrtYMaxUseDefault;
			break;
		case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2MaxUseDefault;
			break;
	     }
	     XtSetArg(args[n],resourceName,True); n++;

	     switch(rcType%3) {
		case X_AXIS_ELEMENT: resourceName = XtNxrtXTickUseDefault;
							break;
		case Y1_AXIS_ELEMENT: resourceName = XtNxrtYTickUseDefault;
							break;
		case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2TickUseDefault;
							break;
	     }
	     XtSetArg(args[n],resourceName,True); n++;
	     switch(rcType%3) {
		case X_AXIS_ELEMENT: resourceName = XtNxrtXNumUseDefault; break;
		case Y1_AXIS_ELEMENT: resourceName = XtNxrtYNumUseDefault;
							break;
		case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2NumUseDefault;
							break;
	     }
	     XtSetArg(args[n],resourceName,True); n++;
	     switch(rcType%3) {
		case X_AXIS_ELEMENT: resourceName = XtNxrtXPrecisionUseDefault;
							break;
		case Y1_AXIS_ELEMENT: resourceName = XtNxrtYPrecisionUseDefault;
							break;
		case Y2_AXIS_ELEMENT: resourceName =XtNxrtY2PrecisionUseDefault;
							 break;
	     }
	     XtSetArg(args[n],resourceName,True); n++;
	  }
	  if (pcp) pcp->axisRange[rcType%3].isCurrentlyFromChannel = False;
       }
       break;
    default:
       medmPrintf("\ncpAxisptionMenuSimpleCallback: unknown rcType = %d",rcType/3);
       break;
  }

/****** Update for EDIT or EXECUTE mode */

  switch(globalDisplayListTraversalMode) {

    case DL_EDIT:
      if (currentDisplayInfo != NULL) {
/*
 * update elements (this is overkill, but okay for now)
 *	-- not as efficient as it should be (don't update EVERYTHING if only
 *	   one item changed!)
 */
/****** Unhighlight (since objects may move) */
      unhighlightSelectedElements();
      for (i = 0; i < currentDisplayInfo->numSelectedElements; i++) {
        elementPtr = currentDisplayInfo->selectedElementsArray[i];
        updateElementFromGlobalResourceBundle(elementPtr);
      }
      dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
/****** Highlight */
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
static void colorSelectCallback(Widget, XtPointer cd, XtPointer) {
#else
static void colorSelectCallback(Widget w, XtPointer cd, XtPointer cbs) {
#endif
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
static void fileOpenCallback(
  Widget w,
  int btn,
  XmAnyCallbackStruct *call_data) {
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
static void fileMenuSimpleCallback(Widget, XtPointer cd, XtPointer) {
#else
static void fileMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs) {
#endif
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

#ifdef __cplusplus
static void helpMenuSimpleCallback(Widget, XtPointer cd, XtPointer) {
#else
static void helpMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs) {
#endif
    int buttonNumber = (int) cd;
    Arg args[10];

    switch(buttonNumber) {
    }
}

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
void textFieldFloatVerifyCallback(Widget w, XtPointer, XtPointer pcbs) {
#else
void textFieldFloatVerifyCallback(Widget w, XtPointer cd, XtPointer pcbs) {
#endif
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
void scaleCallback(Widget, XtPointer cd, XtPointer pcbs) {
#else
void scaleCallback(Widget w, XtPointer cd, XtPointer pcbs) {
#endif
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
/* unhighlight */
      unhighlightSelectedElements();
      for (i = 0; i < currentDisplayInfo->numSelectedElements; i++) {
        updateElementFromGlobalResourceBundle(
        currentDisplayInfo->selectedElementsArray[i]);
      }
      dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
/* highlight */
      highlightSelectedElements();
    }
}

#ifdef __cplusplus
void textFieldActivateCallback(Widget w, XtPointer cd, XtPointer) {
#else
void textFieldActivateCallback(Widget w, XtPointer cd, XtPointer cbs) {
#endif
  int rcType = (int) cd;
  char *stringValue;
  DlElement *dyn;
  int i;

  stringValue = XmTextFieldGetString(w);
/*
 * for the strcpy() calls, note that the textField has a maxLength resource
 *	set such that the strcpy always succeeds
 */
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
	strcpy(globalResourceBundle.rdbk,stringValue);
	break;
     case CTRL_RC:
	strcpy(globalResourceBundle.ctrl,stringValue);
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
	if (strlen(stringValue) > 0) {
          XtSetSensitive(resourceEntryRC[CLRMOD_RC],True);
          XtSetSensitive(resourceEntryRC[VIS_RC],True);
	} else {
          XtSetSensitive(resourceEntryRC[CLRMOD_RC],False);
          XtSetSensitive(resourceEntryRC[VIS_RC],False);
          if (currentDisplayInfo != NULL) {
            if (currentDisplayInfo->numSelectedElements == 1) {
              dyn = lookupDynamicAttributeElement(
	        currentDisplayInfo->selectedElementsArray[0]);
	      if (dyn != NULL) 
                deleteAndFreeElementAndStructure(currentDisplayInfo,dyn);
            }
          }
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
        if (strlen(stringValue) > 0) {
          XtSetSensitive(resourceEntryRC[ERASE_MODE_RC],True);
        } else {
          XtSetSensitive(resourceEntryRC[ERASE_MODE_RC],False);
        }
	break;
  }
  XtFree(stringValue);

/****** Update elements (this is overkill, but okay for now) */
/* unhighlight (since objects may move) */
  if (currentDisplayInfo != NULL) {
    unhighlightSelectedElements();
    for (i = 0; i < currentDisplayInfo->numSelectedElements; i++) {
      updateElementFromGlobalResourceBundle(
			currentDisplayInfo->selectedElementsArray[i]);
    }
    dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
    /* highlight */
    highlightSelectedElements();
    if (currentDisplayInfo->hasBeenEditedButNotSaved == False) 
      medmMarkDisplayBeingEdited(currentDisplayInfo);
  }
}

#ifdef __cplusplus
void cpAxisTextFieldActivateCallback(Widget w, XtPointer cd, XtPointer) {
#else
void cpAxisTextFieldActivateCallback(Widget w, XtPointer cd, XtPointer cbs) {
#endif
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
    switch(rcType/3) {
      case CP_RANGE_MIN/3:
	globalResourceBundle.axis[rcType%3].minRange= atof(stringValue);
	if (globalDisplayListTraversalMode == DL_EXECUTE) {
	    valF.fval = globalResourceBundle.axis[rcType%3].minRange;
	    switch(rcType%3) {
		case X_AXIS_ELEMENT: resourceName = XtNxrtXMin; break;
		case Y1_AXIS_ELEMENT: resourceName = XtNxrtYMin; break;
		case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2Min; break;
	    }
	    XtSetArg(args[n],resourceName,valF.lval); n++;
	}
	break;
     case CP_RANGE_MAX/3:
	globalResourceBundle.axis[rcType%3].maxRange= atof(stringValue);
	if (globalDisplayListTraversalMode == DL_EXECUTE) {
	    valF.fval = globalResourceBundle.axis[rcType%3].maxRange;
	    switch(rcType%3) {
		case X_AXIS_ELEMENT: resourceName = XtNxrtXMax; break;
		case Y1_AXIS_ELEMENT: resourceName = XtNxrtYMax; break;
		case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2Max; break;
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
    /* unhighlight (since objects may move) */
      unhighlightSelectedElements();
      for (i = 0; i < currentDisplayInfo->numSelectedElements; i++) {
      updateElementFromGlobalResourceBundle(
			currentDisplayInfo->selectedElementsArray[i]);
      }
      dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
    /* highlight */
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
void textFieldLosingFocusCallback(Widget, XtPointer cd, XtPointer) {
#else
void textFieldLosingFocusCallback(Widget w, XtPointer cd, XtPointer cbs) {
#endif
    int rcType = (int) cd;
    char string[MAX_TOKEN_LENGTH], *currentString;
    int tail;

/** losing focus - make sure that the text field remains accurate
    wrt globalResourceBundle */
  switch(rcType) {
     case X_RC:
	sprintf(string,"%d",globalResourceBundle.x);
	currentString = XmTextFieldGetString(resourceEntryElement[X_RC]);
	if (strcmp(string,currentString))
	   XmTextFieldSetString(resourceEntryElement[X_RC],string);
	XtFree(currentString);
	break;
     case Y_RC:
	sprintf(string,"%d",globalResourceBundle.y);
	currentString = XmTextFieldGetString(resourceEntryElement[Y_RC]);
	if (strcmp(string,currentString))
	   XmTextFieldSetString(resourceEntryElement[Y_RC],string);
	XtFree(currentString);
	break;
     case WIDTH_RC:
	sprintf(string,"%d",globalResourceBundle.width);
	currentString = XmTextFieldGetString(resourceEntryElement[WIDTH_RC]);
	if (strcmp(string,currentString))
	   XmTextFieldSetString(resourceEntryElement[WIDTH_RC],string);
	XtFree(currentString);
	break;
     case HEIGHT_RC:
	sprintf(string,"%d",globalResourceBundle.height);
	currentString = XmTextFieldGetString(resourceEntryElement[HEIGHT_RC]);
	if (strcmp(string,currentString))
	   XmTextFieldSetString(resourceEntryElement[HEIGHT_RC],string);
	XtFree(currentString);
	break;
     case LINEWIDTH_RC:
	sprintf(string,"%d",globalResourceBundle.lineWidth);
	currentString =XmTextFieldGetString(resourceEntryElement[LINEWIDTH_RC]);
	if (strcmp(string,currentString))
	   XmTextFieldSetString(resourceEntryElement[LINEWIDTH_RC],string);
	XtFree(currentString);
	break;

     case RDBK_RC:
	currentString = XmTextFieldGetString(resourceEntryElement[RDBK_RC]);
	if (strcmp(globalResourceBundle.rdbk,currentString))
	    XmTextFieldSetString(resourceEntryElement[RDBK_RC],
			globalResourceBundle.rdbk);
	XtFree(currentString);
	break;
     case CTRL_RC:
	currentString = XmTextFieldGetString(resourceEntryElement[CTRL_RC]);
	if (strcmp(globalResourceBundle.ctrl,currentString))
	   XmTextFieldSetString(resourceEntryElement[CTRL_RC],
		     globalResourceBundle.ctrl);
	XtFree(currentString);
	break;
     case TITLE_RC:
	currentString = XmTextFieldGetString(resourceEntryElement[TITLE_RC]);
	if (strcmp(globalResourceBundle.title,currentString))
	   XmTextFieldSetString(resourceEntryElement[TITLE_RC],
		     globalResourceBundle.title);
	XtFree(currentString);
	break;
     case XLABEL_RC:
	currentString = XmTextFieldGetString(resourceEntryElement[XLABEL_RC]);
	if (strcmp(globalResourceBundle.xlabel,currentString))
	   XmTextFieldSetString(resourceEntryElement[XLABEL_RC],
		     globalResourceBundle.xlabel);
	XtFree(currentString);
	break;
     case YLABEL_RC:
	currentString = XmTextFieldGetString(resourceEntryElement[YLABEL_RC]);
	if (strcmp(globalResourceBundle.ylabel,currentString))
	   XmTextFieldSetString(resourceEntryElement[YLABEL_RC],
		     globalResourceBundle.ylabel);
	XtFree(currentString);
	break;
     case CHAN_RC:
	currentString = XmTextFieldGetString(resourceEntryElement[CHAN_RC]);
	if (strcmp(globalResourceBundle.chan,currentString))
	   XmTextFieldSetString(resourceEntryElement[CHAN_RC],
			globalResourceBundle.chan);
	XtFree(currentString);
	break;
     case DIS_RC:
	sprintf(string,"%d",globalResourceBundle.dis);
	currentString = XmTextFieldGetString(resourceEntryElement[DIS_RC]);
	if (strcmp(string,currentString))
	   XmTextFieldSetString(resourceEntryElement[DIS_RC],string);
	XtFree(currentString);
	break;
     case XYANGLE_RC:
	sprintf(string,"%d",globalResourceBundle.xyangle);
	currentString = XmTextFieldGetString(resourceEntryElement[XYANGLE_RC]);
	if (strcmp(string,currentString))
	   XmTextFieldSetString(resourceEntryElement[XYANGLE_RC],string);
	XtFree(currentString);
	break;
     case ZANGLE_RC:
	sprintf(string,"%d",globalResourceBundle.zangle);
	currentString = XmTextFieldGetString(resourceEntryElement[ZANGLE_RC]);
	if (strcmp(string,currentString))
	   XmTextFieldSetString(resourceEntryElement[ZANGLE_RC],string);
	XtFree(currentString);
	break;
     case PERIOD_RC:
        cvtDoubleToString(globalResourceBundle.period,string,0);
	currentString = XmTextFieldGetString(resourceEntryElement[PERIOD_RC]);
	if (strcmp(string,currentString))
	   XmTextFieldSetString(resourceEntryElement[PERIOD_RC],string);
	XtFree(currentString);
	break;
     case COUNT_RC:
	sprintf(string,"%d",globalResourceBundle.count);
	currentString = XmTextFieldGetString(resourceEntryElement[COUNT_RC]);
	if (strcmp(string,currentString))
	   XmTextFieldSetString(resourceEntryElement[COUNT_RC],string);
	XtFree(currentString);
	break;
     case TEXTIX_RC:
	currentString = XmTextFieldGetString(resourceEntryElement[TEXTIX_RC]);
	if (strcmp(globalResourceBundle.textix,currentString))
	   XmTextFieldSetString(resourceEntryElement[TEXTIX_RC],
			globalResourceBundle.textix);
	XtFree(currentString);
	break;
     case MSG_LABEL_RC:
	currentString = XmTextFieldGetString(
				resourceEntryElement[MSG_LABEL_RC]);
	if (strcmp(globalResourceBundle.messageLabel,currentString))
	   XmTextFieldSetString(resourceEntryElement[MSG_LABEL_RC],
		     globalResourceBundle.messageLabel);
	XtFree(currentString);
	break;
     case PRESS_MSG_RC:
	currentString = XmTextFieldGetString(
				resourceEntryElement[PRESS_MSG_RC]);
	if (strcmp(globalResourceBundle.press_msg,currentString))
	   XmTextFieldSetString(resourceEntryElement[PRESS_MSG_RC],
		     globalResourceBundle.press_msg);
	XtFree(currentString);
	break;
     case RELEASE_MSG_RC:
	currentString = XmTextFieldGetString(
          resourceEntryElement[RELEASE_MSG_RC]);
	if (strcmp(globalResourceBundle.release_msg,currentString))
	  XmTextFieldSetString(resourceEntryElement[RELEASE_MSG_RC],
          globalResourceBundle.release_msg);
	XtFree(currentString);
	break;
     case IMAGENAME_RC:
	currentString=XmTextFieldGetString(resourceEntryElement[IMAGENAME_RC]);
	if (strcmp(globalResourceBundle.imageName,currentString))
          XmTextFieldSetString(resourceEntryElement[IMAGENAME_RC],
          globalResourceBundle.imageName);
	XtFree(currentString);
	break;
     case DATA_RC:
	currentString = XmTextFieldGetString(resourceEntryElement[DATA_RC]);
	if (strcmp(globalResourceBundle.data,currentString))
	  XmTextFieldSetString(resourceEntryElement[DATA_RC],
          globalResourceBundle.data);
	XtFree(currentString);
	break;
     case CMAP_RC:
	currentString = XmTextFieldGetString(resourceEntryElement[CMAP_RC]);
	if (strcmp(globalResourceBundle.cmap,currentString))
	   XmTextFieldSetString(resourceEntryElement[CMAP_RC],
			globalResourceBundle.cmap);
	XtFree(currentString);
	break;
     case PRECISION_RC:
	sprintf(string,"%f",globalResourceBundle.dPrecision);
	/* strip trailing zeroes */
	tail = strlen(string);
	while (string[--tail] == '0') string[tail] = '\0';
	currentString = XmTextFieldGetString(
			resourceEntryElement[PRECISION_RC]);
	if (strcmp(string,currentString))
	   XmTextFieldSetString(resourceEntryElement[PRECISION_RC],string);
	XtFree(currentString);
	break;
     case SBIT_RC:
	sprintf(string,"%d",globalResourceBundle.sbit);
	currentString = XmTextFieldGetString(resourceEntryElement[SBIT_RC]);
	if (strcmp(string,currentString))
	   XmTextFieldSetString(resourceEntryElement[SBIT_RC],string);
	XtFree(currentString);
	break;
     case EBIT_RC:
	sprintf(string,"%d",globalResourceBundle.ebit);
	currentString = XmTextFieldGetString(resourceEntryElement[EBIT_RC]);
	if (strcmp(string,currentString))
	   XmTextFieldSetString(resourceEntryElement[EBIT_RC],string);
	XtFree(currentString);
	break;
     case TRIGGER_RC:
	currentString = XmTextFieldGetString(resourceEntryElement[TRIGGER_RC]);
	if (strcmp(globalResourceBundle.trigger,currentString))
          XmTextFieldSetString(resourceEntryElement[TRIGGER_RC],globalResourceBundle.trigger);
	XtFree(currentString);
	break;
     case ERASE_RC :
	currentString = XmTextFieldGetString(resourceEntryElement[ERASE_RC]);
	if (strcmp(globalResourceBundle.erase,currentString))
	  XmTextFieldSetString(resourceEntryElement[ERASE_RC],globalResourceBundle.erase);
	XtFree(currentString);
	break;
  }
}

#ifdef __cplusplus
void cpAxisTextFieldLosingFocusCallback(Widget, XtPointer cd, XtPointer) {
#else
void cpAxisTextFieldLosingFocusCallback(Widget w, XtPointer cd, XtPointer cbs) {
#endif
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

  switch(rcType/3) {
     case CP_RANGE_MIN/3:
	sprintf(string,"%f", minF[rcType%3].fval);
	/* strip trailing zeroes */
	tail = strlen(string);
	while (string[--tail] == '0') string[tail] = '\0';
	currentString = XmTextFieldGetString(axisRangeMin[rcType%3]);
	if (strcmp(string,currentString))
	   XmTextFieldSetString(axisRangeMin[rcType%3],string);
	XtFree(currentString);
	break;
     case CP_RANGE_MAX/3:
	sprintf(string,"%f", maxF[rcType%3].fval);
	/* strip trailing zeroes */
	tail = strlen(string);
	while (string[--tail] == '0') string[tail] = '\0';
	currentString = XmTextFieldGetString(axisRangeMax[rcType%3]);
	if (strcmp(string,currentString))
	   XmTextFieldSetString(axisRangeMax[rcType%3],string);
	XtFree(currentString);
	break;
     default:
	fprintf(stderr,
	"\ncpAxisTextFieldLosingFocusCallback: unknown rcType = %d",
		rcType/3);
	break;
  }
}

#ifdef EXTENDED_INTERFACE
static void bundleCallback(
/****************************************************************************
 * Bundle Call-back                                                         *
 ****************************************************************************/
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
 globalResourceBundle.rdbk[0] = '\0';
 globalResourceBundle.ctrl[0] = '\0';
 globalResourceBundle.title[0] = '\0';
 globalResourceBundle.xlabel[0] = '\0';
 globalResourceBundle.ylabel[0] = '\0';
 if (currentDisplayInfo != NULL) {
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
 globalResourceBundle.direction = UP;
 globalResourceBundle.clrmod = STATIC;
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
  globalResourceBundle.axis[X_AXIS_ELEMENT].axisStyle = LINEAR_AXIS;
  globalResourceBundle.axis[X_AXIS_ELEMENT].rangeStyle = CHANNEL_RANGE;
  globalResourceBundle.axis[X_AXIS_ELEMENT].minRange = 0.0;
  globalResourceBundle.axis[X_AXIS_ELEMENT].maxRange = 1.0;
/* structure copy for other two axis definitions */
  globalResourceBundle.axis[Y1_AXIS_ELEMENT]
	= globalResourceBundle.axis[X_AXIS_ELEMENT];
  globalResourceBundle.axis[Y2_AXIS_ELEMENT]
	= globalResourceBundle.axis[X_AXIS_ELEMENT];
  globalResourceBundle.trigger[0] = '\0';
  globalResourceBundle.erase[0] = '\0';
  globalResourceBundle.eraseMode = ERASE_IF_NOT_ZERO;
}

static void initializeXmStringValueTables() {
/****************************************************************************
 * Initialize XmStrintg Value Tables: ResourceBundle and related widgets.   *
 ****************************************************************************/
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

void createResource() {
/****************************************************************************
 * Create Resource: Create and initialize the resourcePalette,              *
 *   resourceBundle and related widgets.                                    *
 ****************************************************************************/
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
  buttons[2] = XmStringCreateSimple("Help");
  keySyms[1] = 'B';
  keySyms[2] = 'H';
#else
  buttons[1] = XmStringCreateSimple("Help");
  keySyms[1] = 'H';
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
      XmVaCASCADEBUTTON, buttons[1], 'H',
      NULL);

/****** Color resourceMB properly (force so VUE doesn't interfere) */
    colorMenuBar(resourceMB,defaultForeground,defaultBackground);


/****** set the Help cascade button in the menu bar */
#ifdef EXTENDED_INTERFACE
  menuHelpWidget = XtNameToWidget(resourceMB,"*button_2");
#else
  menuHelpWidget = XtNameToWidget(resourceMB,"*button_1");
#endif

    XtVaSetValues(resourceMB,XmNmenuHelpWidget,menuHelpWidget, NULL);
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

static void createResourceEntries(Widget entriesSW) {
/****************************************************************************
 * Create Resource Entries: Create resource entries in scrolled window      *
 ****************************************************************************/
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

static void createEntryRC( Widget parent, int rcType) {
/****************************************************************************
 * Create Entry RC: Create the various row-columns for each resource entry  *
 * rcType = {X_RC,Y_RC,...}.                                                *
 ****************************************************************************/
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
			textFieldActivateCallback,
			(XtPointer)rcType);
	XtAddCallback(localElement,XmNlosingFocusCallback,
			textFieldLosingFocusCallback,
			(XtPointer)rcType);
	XtAddCallback(localElement,XmNmodifyVerifyCallback,
			textFieldNumericVerifyCallback,
			(XtPointer)rcType);
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


/* color types */
     case CLR_RC:
     case BCLR_RC:
     case DATA_CLR_RC:
	n = 0;
	if (rcType == CLR_RC) {
	    XtSetArg(args[n],XmNbackground,
		(currentDisplayInfo == NULL) ? BlackPixel(display,screenNum) :
			currentDisplayInfo->dlColormap[
			currentDisplayInfo->drawingAreaForegroundColor]); n++;
	} else {
	    XtSetArg(args[n],XmNbackground,
		(currentDisplayInfo == NULL) ? WhitePixel(display,screenNum) :
		currentDisplayInfo->dlColormap[
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

static void initializeResourcePaletteElements() {
/****************************************************************************
 * Initialize Resource Palette Elements: Initialize resourcePaletteElements *
 *   array to reflect the appropriate resource entries for the selected     *
 *   display element type                                                   *
 ****************************************************************************/
    int i, j, index;

/****** initialize the resourcePaletteElements array */
    for (index = 0; index < NUM_DL_ELEMENT_TYPES; index++) {
      for (i = 0; i < MAX_RESOURCES_FOR_DL_ELEMENT; i++)  {
        resourcePaletteElements[index].childIndexRC[i] = 0;
        resourcePaletteElements[index].children[i] = NULL;
      }
      resourcePaletteElements[index].numChildren = 0;
    }

/****** now do the element-specific stuff */

 index = DL_Display - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CMAP_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 index = DL_Valuator - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CTRL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = LABEL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = DIRECTION_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = PRECISION_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 index = DL_ChoiceButton - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CTRL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = STACKING_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 index = DL_MessageButton - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CTRL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = MSG_LABEL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = PRESS_MSG_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = RELEASE_MSG_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 index = DL_TextEntry - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CTRL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = FORMAT_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 index = DL_Menu - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CTRL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 /* DL_Meter */
 index = DL_Meter - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = RDBK_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = LABEL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 index = DL_TextUpdate - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = RDBK_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = ALIGN_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = FORMAT_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 index = DL_Bar - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = RDBK_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = LABEL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = DIRECTION_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = FILLMOD_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 index = DL_Byte - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = RDBK_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = SBIT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = EBIT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = DIRECTION_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 /* DL_Indicator */
 index = DL_Indicator - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = RDBK_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = LABEL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = DIRECTION_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 /* DL_StripChart */
 index = DL_StripChart - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = TITLE_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = XLABEL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = YLABEL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = PERIOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = UNITS_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = SCDATA_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 /* DL_CartesianPlot */
 index = DL_CartesianPlot - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = TITLE_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = XLABEL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = YLABEL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CSTYLE_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = ERASE_OLDEST_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = COUNT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CPDATA_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CPAXIS_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = TRIGGER_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = ERASE_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = ERASE_MODE_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 /* DL_SurfacePlot */
 index = DL_SurfacePlot - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = TITLE_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = XLABEL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = YLABEL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = DATA_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = DATA_CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = DIS_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = XYANGLE_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = ZANGLE_RC; i++;
 resourcePaletteElements[index].numChildren = i;

/************/

 /* DL_Rectangle */
 index = DL_Rectangle - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = STYLE_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = FILL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = LINEWIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = VIS_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CHAN_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 /* DL_Oval */
 index = DL_Oval - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = STYLE_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = FILL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = LINEWIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = VIS_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CHAN_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 /* DL_Arc */
 index = DL_Arc - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BEGIN_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = PATH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = STYLE_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = FILL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = LINEWIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = VIS_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CHAN_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 /* DL_Text */
 index = DL_Text - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = TEXTIX_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = ALIGN_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = VIS_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CHAN_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 /* DL_File */
 /* DL_Colormap */
 /* DL_BasicAttribute */
 /* DL_DynamicAttribute */
 /* DL_RelatedDisplay */
 index = DL_RelatedDisplay - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
/* MDA - add dynamics to Related Display at some point
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = VIS_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CHAN_RC; i++;
*/
 resourcePaletteElements[index].childIndexRC[i] = RDDATA_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 /* DL_ShellCommand */
 index = DL_ShellCommand - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = BCLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = SHELLDATA_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 /* DL_Image */
 index = DL_Image - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = IMAGETYPE_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = IMAGENAME_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 /* DL_Composite */
 index = DL_Composite - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
/* MDA - turn these back on to finish composite visibility!!!
 resourcePaletteElements[index].childIndexRC[i] = VIS_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CHAN_RC; i++;
*/
 resourcePaletteElements[index].numChildren = i;

 /* DL_Line */
 index = DL_Line - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = STYLE_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = LINEWIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = VIS_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CHAN_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 /* DL_Polyline*/
 index = DL_Polyline - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = STYLE_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = LINEWIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = VIS_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CHAN_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 /* DL_Polygon */
 index = DL_Polygon - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = STYLE_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = FILL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = LINEWIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = VIS_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CHAN_RC; i++;
 resourcePaletteElements[index].numChildren = i;

 /* DL_BezierCurve */
 index = DL_BezierCurve - MIN_DL_ELEMENT_TYPE;
 i = 0;
 resourcePaletteElements[index].childIndexRC[i] = X_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = Y_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = WIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = HEIGHT_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLR_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = STYLE_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = FILL_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = LINEWIDTH_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CLRMOD_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = VIS_RC; i++;
 resourcePaletteElements[index].childIndexRC[i] = CHAN_RC; i++;
 resourcePaletteElements[index].numChildren = i;

/****** Now set the widgets for Manage/Unmange Children */
    for (i = 0; i < NUM_DL_ELEMENT_TYPES; i++) {
      for (j = 0; j < resourcePaletteElements[i].numChildren; j++) {
	resourcePaletteElements[i].children[j] =
          resourceEntryRC[resourcePaletteElements[i].childIndexRC[j]];
      }
    }
}

static void createResourceBundles(Widget bundlesSW) {
/****************************************************************************
 * Create Resource Bundles : Create resource bundles in scrolled window.    *
 ****************************************************************************/
    Arg args[10];
    int n;

    n = 0;
    bundlesRB = XmCreateRadioBox(bundlesSW,"bundlesRB",args,n);

/****** create the default bundle (Current) */
    createBundleTB(bundlesRB,"Current");
    XtManageChild(bundlesRB);
}

static void createBundleTB(Widget bundlesRB, char *name) {
/****************************************************************************
 * Create Bundle Bundles : Create resource bundles in scrolled window       *
 ****************************************************************************/
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
static void relatedDisplayActivate(Widget, XtPointer cd, XtPointer) {
#else
static void relatedDisplayActivate(Widget w, XtPointer cd, XtPointer cbs) {
#endif
  int buttonType = (int) cd;
  String **newCells;
  int i;

  switch (buttonType) {

   case RD_APPLY_BTN:
  /* commit changes in matrix to global matrix array data */
     XbaeMatrixCommitEdit(rdMatrix,False);
     XtVaGetValues(rdMatrix,XmNcells,&newCells,NULL);
  /* now update globalResourceBundle...*/
     for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
	strcpy(globalResourceBundle.rdData[i].label, newCells[i][0]);
	strcpy(globalResourceBundle.rdData[i].name, newCells[i][1]);
	strcpy(globalResourceBundle.rdData[i].args, newCells[i][2]);
     }
  /* and update the elements (since this level of "Apply" is analogous
   *	to changing text in a text field in the resource palette
   *	(don't need to traverse the display list since these changes
   *	 aren't visible at the first level)
   */
     if (currentDisplayInfo != NULL) {
       for (i = 0; i < currentDisplayInfo->numSelectedElements; i++) {
	if (currentDisplayInfo->selectedElementsArray[i]->type == 
		DL_RelatedDisplay) updateElementFromGlobalResourceBundle(
			currentDisplayInfo->selectedElementsArray[i]);
       }
     }
     XtPopdown(relatedDisplayS);
     if (currentDisplayInfo->hasBeenEditedButNotSaved == False) 
       medmMarkDisplayBeingEdited(currentDisplayInfo);
     break;

   case RD_CLOSE_BTN:
     XtPopdown(relatedDisplayS);
     break;
  }
}

/*
 * create related display data dialog
 */
Widget createRelatedDisplayDataDialog (Widget parent) {
  Widget shell, applyButton, closeButton;
  Dimension cWidth, cHeight, aWidth, aHeight;
  Arg args[12];
  XmString xmString;
  int i, j, n;
  static Boolean first = True;

/* initialize those file-scoped globals */
  if (first) {
    first = False;
    for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
      for (j = 0; j < 3; j++) rdRows[i][j] = NULL;
      rdCells[i] = &rdRows[i][0];
    }
  }

/*
 * now create the interface
 *
 *	       label | name | args
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
  rdForm = XmCreateFormDialog(parent,"relatedDisplayDataF",args,n);
  shell = XtParent(rdForm);
  n = 0;
  XtSetArg(args[n],XmNtitle,"Related Display Data"); n++;
  XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
  XtSetValues(shell,args,n);

  XmAddWMProtocolCallback(shell,WM_DELETE_WINDOW,
		relatedDisplayActivate,(XtPointer)RD_CLOSE_BTN);
  n = 0;
  XtSetArg(args[n],XmNrows,MAX_RELATED_DISPLAYS); n++;
  XtSetArg(args[n],XmNcolumns,3); n++;
  XtSetArg(args[n],XmNcolumnMaxLengths,rdColumnMaxLengths); n++;
  XtSetArg(args[n],XmNcolumnWidths,rdColumnWidths); n++;
  XtSetArg(args[n],XmNcolumnLabels,rdColumnLabels); n++;
  XtSetArg(args[n],XmNcolumnMaxLengths,rdColumnMaxLengths); n++;
  XtSetArg(args[n],XmNcolumnWidths,rdColumnWidths); n++;
  XtSetArg(args[n],XmNcolumnLabelAlignments,rdColumnLabelAlignments); n++;
  XtSetArg(args[n],XmNboldLabels,False); n++;
  rdMatrix = XtCreateManagedWidget("rdMatrix",
			xbaeMatrixWidgetClass,rdForm,args,n);


  xmString = XmStringCreateSimple("Close");
  n = 0;
  XtSetArg(args[n],XmNlabelString,xmString); n++;
  closeButton = XmCreatePushButton(rdForm,"closeButton",args,n);
  XtAddCallback(closeButton,XmNactivateCallback,
		relatedDisplayActivate,(XtPointer)RD_CLOSE_BTN);
  XtManageChild(closeButton);
  XmStringFree(xmString);

  xmString = XmStringCreateSimple("Apply");
  n = 0;
  XtSetArg(args[n],XmNlabelString,xmString); n++;
  applyButton = XmCreatePushButton(rdForm,"applyButton",args,n);
  XtAddCallback(applyButton,XmNactivateCallback,
		relatedDisplayActivate,(XtPointer)RD_APPLY_BTN);
  XtManageChild(applyButton);
  XmStringFree(xmString);

/* make APPLY and CLOSE buttons same size */
  XtVaGetValues(closeButton,XmNwidth,&cWidth,XmNheight,&cHeight,NULL);
  XtVaGetValues(applyButton,XmNwidth,&aWidth,XmNheight,&aHeight,NULL);
  XtVaSetValues(closeButton,XmNwidth,MAX(cWidth,aWidth),
			XmNheight,MAX(cHeight,aHeight),NULL);

/* and make the APPLY button the default for the form */
  XtVaSetValues(rdForm,XmNdefaultButton,applyButton,NULL);

/*
 * now do form layout 
 */

/* rdMatrix */
  n = 0;
  XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
  XtSetValues(rdMatrix,args,n);
/* apply */
  n = 0;
  XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
  XtSetArg(args[n],XmNtopWidget,rdMatrix); n++;
  XtSetArg(args[n],XmNtopOffset,12); n++;
  XtSetArg(args[n],XmNleftAttachment,XmATTACH_POSITION); n++;
  XtSetArg(args[n],XmNleftPosition,30); n++;
  XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n],XmNbottomOffset,12); n++;
  XtSetValues(applyButton,args,n);
/* close */
  n = 0;
  XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
  XtSetArg(args[n],XmNtopWidget,rdMatrix); n++;
  XtSetArg(args[n],XmNtopOffset,12); n++;
  XtSetArg(args[n],XmNrightAttachment,XmATTACH_POSITION); n++;
  XtSetArg(args[n],XmNrightPosition,70); n++;
  XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
  XtSetArg(args[n],XmNbottomOffset,12); n++;
  XtSetValues(closeButton,args,n);

  XtManageChild(rdForm);

  return shell;
}

/*
 * access function (for file-scoped globals, etc) to udpate the
 *	related display data dialog with the values currently in
 *	globalResourceBundle
 */
void updateRelatedDisplayDataDialog()
{
  int i;

  for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
    rdRows[i][0] = globalResourceBundle.rdData[i].label;
    rdRows[i][1] = globalResourceBundle.rdData[i].name;
    rdRows[i][2] = globalResourceBundle.rdData[i].args;
  }
  if (rdMatrix != NULL) XtVaSetValues(rdMatrix,XmNcells,rdCells,NULL);
  
}


#ifdef __cplusplus
static void shellCommandActivate(Widget, XtPointer cd, XtPointer)
#else
static void shellCommandActivate(Widget w, XtPointer cd, XtPointer cbs)
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
  /* and update the elements (since this level of "Ok" is analogous
   *	to changing text in a text field in the resource palette
   *	(don't need to traverse the display list since these changes
   *	 aren't visible at the first level)
   */
     if (currentDisplayInfo != NULL) {
      for (i = 0; i < currentDisplayInfo->numSelectedElements; i++) {
	if (currentDisplayInfo->selectedElementsArray[i]->type == 
		DL_ShellCommand) updateElementFromGlobalResourceBundle(
			currentDisplayInfo->selectedElementsArray[i]);
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
     if (currentDisplayInfo != NULL) {
       for (i = 0; i < currentDisplayInfo->numSelectedElements; i++) {
	if (currentDisplayInfo->selectedElementsArray[i]->type == 
		DL_CartesianPlot) updateElementFromGlobalResourceBundle(
			currentDisplayInfo->selectedElementsArray[i]);
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
  if (cpMatrix != NULL) XtVaSetValues(cpMatrix,XmNcells,cpCells,NULL);
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
	globalResourceBundle.scData[i].clr = (int) scColorRows[i][SC_COLOR_COLUMN];
     }
  /* and update the elements (since this level of "Apply" is analogous
   *	to changing text in a text field in the resource palette
   *	(don't need to traverse the display list since these changes
   *	 aren't visible at the first level)
   */
     if (currentDisplayInfo != NULL) {
       for (i = 0; i < currentDisplayInfo->numSelectedElements; i++) {
	if (currentDisplayInfo->selectedElementsArray[i]->type == 
		DL_StripChart) updateElementFromGlobalResourceBundle(
			currentDisplayInfo->selectedElementsArray[i]);
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
Widget createStripChartDataDialog(
  Widget parent)
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
		axisMaxXmString, frameLabelXmString;
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

/* Axis Style */
  /* create element RC */
    n = 0;
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_NONE); n++;
    localRC = XmCreateRowColumn(parentRC,"entryRC",args,n);
    XtManageChild(localRC);

  /* create the label element */
    n = 0;
    XtSetArg(args[n],XmNalignment,XmALIGNMENT_END); n++;
    XtSetArg(args[n],XmNlabelString,axisStyleXmString); n++;
    XtSetArg(args[n],XmNrecomputeSize,False); n++;
    localLabel = XmCreateLabel(localRC,"localLabel",args,n);
    entryLabel[counter] = localLabel;

  /* create the state or "value" element */
    n = 0;
    XtSetArg(args[n],XmNbuttonType,buttonType); n++;
    XtSetArg(args[n],XmNbuttons,
		&(xmStringValueTable[FIRST_CARTESIAN_PLOT_AXIS_STYLE])); n++;
    XtSetArg(args[n],XmNbuttonCount,NUM_CARTESIAN_PLOT_AXIS_STYLES); n++;
    XtSetArg(args[n],XmNsimpleCallback,
		cpAxisOptionMenuSimpleCallback); n++;
    XtSetArg(args[n],XmNuserData,(XtPointer)(CP_AXIS_STYLE+i)); n++;
    localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
    XtUnmanageChild(XmOptionLabelGadget(localElement));
    entryElement[counter] = localElement;
    axisStyleMenu[i] = localElement;
    counter++;

/* Range Style */
  /* create element RC */
    n = 0;
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_NONE); n++;
    localRC = XmCreateRowColumn(parentRC,"entryRC",args,n);
    XtManageChild(localRC);

  /* create the label element */
    n = 0;
    XtSetArg(args[n],XmNalignment,XmALIGNMENT_END); n++;
    XtSetArg(args[n],XmNlabelString,axisRangeXmString); n++;
    XtSetArg(args[n],XmNrecomputeSize,False); n++;
    localLabel = XmCreateLabel(localRC,"localLabel",args,n);
    entryLabel[counter] = localLabel;

  /* create the state or "value" element */
    n = 0;
    XtSetArg(args[n],XmNbuttonType,buttonType); n++;
    XtSetArg(args[n],XmNbuttons,
		&(xmStringValueTable[FIRST_CARTESIAN_PLOT_RANGE_STYLE])); n++;
    XtSetArg(args[n],XmNbuttonCount,NUM_CARTESIAN_PLOT_RANGE_STYLES); n++;
    XtSetArg(args[n],XmNsimpleCallback,
		cpAxisOptionMenuSimpleCallback); n++;
    XtSetArg(args[n],XmNuserData,CP_RANGE_STYLE+i); n++;
    localElement = XmCreateSimpleOptionMenu(localRC,"localElement",args,n);
    XtUnmanageChild(XmOptionLabelGadget(localElement));
    entryElement[counter] = localElement;
    axisRangeMenu[i] = localElement;
    counter++;

/* Min */
  /* create element RC */
    n = 0;
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_NONE); n++;
    localRC = XmCreateRowColumn(parentRC,"entryRC",args,n);
    XtManageChild(localRC);
    axisRangeMinRC[i] = localRC;

  /* create the label element */
    n = 0;
    XtSetArg(args[n],XmNalignment,XmALIGNMENT_END); n++;
    XtSetArg(args[n],XmNlabelString,axisMinXmString); n++;
    XtSetArg(args[n],XmNrecomputeSize,False); n++;
    localLabel = XmCreateLabel(localRC,"localLabel",args,n);
    entryLabel[counter] = localLabel;

  /* create the state or "value" element */
    n = 0;
    XtSetArg(args[n],XmNmaxLength,MAX_TOKEN_LENGTH-1); n++;
    localElement = XmCreateTextField(localRC,"localElement",args,n);
    entryElement[counter] = localElement;
    axisRangeMin[i] = localElement;
    XtAddCallback(localElement,XmNactivateCallback,
		cpAxisTextFieldActivateCallback,
		(XtPointer)(CP_RANGE_MIN+i));
    XtAddCallback(localElement,XmNlosingFocusCallback,
		cpAxisTextFieldLosingFocusCallback,
		(XtPointer)(CP_RANGE_MIN+i));
    XtAddCallback(localElement,XmNmodifyVerifyCallback,
		textFieldFloatVerifyCallback,
		(XtPointer)NULL);
    counter++;

/* Max */
  /* create element RC */
    n = 0;
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_NONE); n++;
    localRC = XmCreateRowColumn(parentRC,"entryRC",args,n);
    XtManageChild(localRC);
    axisRangeMaxRC[i] = localRC;

  /* create the label element */
    n = 0;
    XtSetArg(args[n],XmNalignment,XmALIGNMENT_END); n++;
    XtSetArg(args[n],XmNlabelString,axisMaxXmString); n++;
    XtSetArg(args[n],XmNrecomputeSize,False); n++;
    localLabel = XmCreateLabel(localRC,"localLabel",args,n);
    entryLabel[counter] = localLabel;

  /* create the state or "value" element */
    n = 0;
    XtSetArg(args[n],XmNmaxLength,MAX_TOKEN_LENGTH-1); n++;
    localElement = XmCreateTextField(localRC,"localElement",args,n);
    entryElement[counter] = localElement;
    axisRangeMax[i] = localElement;
    XtAddCallback(localElement,XmNactivateCallback,
		cpAxisTextFieldActivateCallback,
		(XtPointer)(CP_RANGE_MAX+i));
    XtAddCallback(localElement,XmNlosingFocusCallback,
		cpAxisTextFieldLosingFocusCallback,
		(XtPointer)(CP_RANGE_MAX+i));
    XtAddCallback(localElement,XmNmodifyVerifyCallback,
		textFieldFloatVerifyCallback,
		(XtPointer)NULL);
    counter++;

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
  XcVType xMinF, xMaxF, y1MinF, y1MaxF, y2MinF, y2MaxF;
  Arg args[2];

  if (globalDisplayListTraversalMode == DL_EXECUTE) {
     if (cp != NULL) {
        XtSetArg(args[0],XmNuserData,&userData);
        XtGetValues(cp,args,1);
        pcp = (CartesianPlot *) userData;
     }
  }
  if (pcp != NULL) {
    xIsCurrentlyFromChannel =
        pcp->axisRange[X_AXIS_ELEMENT].isCurrentlyFromChannel;
    y1IsCurrentlyFromChannel =
        pcp->axisRange[Y1_AXIS_ELEMENT].isCurrentlyFromChannel;
    y2IsCurrentlyFromChannel =
        pcp->axisRange[Y2_AXIS_ELEMENT].isCurrentlyFromChannel;
  }

  XtVaGetValues(cp, XtNxrtXAxisLogarithmic,&xAxisIsLog,
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
                    XtNxrtY2MinUseDefault,&y2MinUseDef, NULL);


/* X Axis */
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
    if (!xMinUseDef && !xIsCurrentlyFromChannel) {
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

