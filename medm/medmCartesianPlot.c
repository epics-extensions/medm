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

#define DEBUG_CARTESIAN_PLOT 0
#define DEBUG_CARTESIAN_PLOT_BORDER 0
#define DEBUG_CARTESIAN_PLOT_UPDATE 0
#define DEBUG_TIME 0
#define DEBUG_XRT 0

#define CHECK_NAN

#ifdef CHECK_NAN
/* Note that for Solaris isnand and isnanf are defined in ieeefp.h.
 *  isnan is the same as isnand and is supposed to be defined in math.h
 *    but it isn't.
 * For Sun4 there is no isnand or isnanf and isnan is defined in math.h.
 * Consequently we use isnan and prototype it explicitly.
 * This should work on all systems and be safe.
 */
#ifdef WIN32
#include <float.h>
#define isnan(a) _isnan(a)  /* For some reason MS has leading _ */
#else
#include <math.h>
extern int isnan(double);     /* Because it is not in math.h as it should be */
#endif /* WIN32 */
#define NAN_SUBSTITUTE 0.0
#define SAFEFLOAT(x) (safeFloat(x))
#else
#define SAFEFLOAT(x) ((float)(x))
#endif     /* End #ifdef CHECK_NAN */

#include "medm.h"
#include <Xm/MwmUtil.h>
#include "medmCartesianPlot.h"

/* Function for generic plotting routines */

CpDataHandle CpDataCreate(CpDataType hData, int nsets, int npoints);
int CpDataGetLastPoint(CpDataHandle hData, int set);
double CpDataGetXElement(CpDataHandle hData, int set, int point);
double CpDataGetYElement(CpDataHandle hData, int set, int point);
void CpDataDestroy(CpDataHandle hData);
int CpDataSetHole(CpDataHandle hData, double hole);
int CpDataSetLastPoint(CpDataHandle hData, int set, int npoints);
int CpDataSetXElement(CpDataHandle hData, int set, int point, double x);
int CpDataSetYElement(CpDataHandle hData, int set, int point, double y);
void CpSetNthDataStyle(Widget graph, int index, CpDataStyle *ds);
void CpSetNthDataStyle2(Widget graph, int index, CpDataStyle *ds);
  
/* Function prototypes */

void cartesianPlotCreateRunTimeInstance(DisplayInfo *, DlElement *);
void cartesianPlotCreateEditInstance(DisplayInfo *, DlElement *);
static void cartesianPlotUpdateGraphicalInfoCb(XtPointer cd);
static void cartesianPlotUpdateScreenFirstTime(XtPointer cd);
static void cartesianPlotDraw(XtPointer cd);
static void cartesianPlotUpdateValueCb(XtPointer cd);
static void cartesianPlotDestroyCb(XtPointer cd);
static void cartesianPlotGetRecord(XtPointer, Record **, int *);
static void cartesianPlotInheritValues(ResourceBundle *pRCB, DlElement *p);
static void cartesianPlotSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void cartesianPlotSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void cartesianPlotGetValues(ResourceBundle *pRCB, DlElement *p);

static void cartesianPlotAxisActivate(Widget w, XtPointer cd, XtPointer cbs);

static CpDataHandle hcpNullData = (CpDataHandle)0;
Widget cpMatrix = NULL, cpForm = NULL;

char *timeFormatString[NUM_CP_TIME_FORMAT] = {
    "%H:%M:%S",
    "%H:%M",
    "%H:00",
    "%b %d, %Y",
    "%b %d",
    "%b %d %H:00",
    "%a %H:00"};

static DlDispatchTable cartesianPlotDlDispatchTable = {
    createDlCartesianPlot,
    NULL,
    executeDlCartesianPlot,
    writeDlCartesianPlot,
    NULL,
    cartesianPlotGetValues,
    cartesianPlotInheritValues,
    cartesianPlotSetBackgroundColor,
    cartesianPlotSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

#ifdef CHECK_NAN
float safeFloat(double x) {
    static int nerrs = 0;
    static int print = 1;
    
    if (isnan(x)) {
	if(print) {
	    if( nerrs < 25) {
		nerrs++;
		medmPostMsg(0,"CartesianPlot: Value is NaN, using %g\n",NAN_SUBSTITUTE);
		if(nerrs >= 25) {
		    medmPrintf(0,"\nCartesianPlot: Suppressing further NaN error messages\n");
		    print = 0;
		}
	    }
	}
	return NAN_SUBSTITUTE;
    } else {
	return (float)x;
    }
}
#endif

void cartesianPlotCreateRunTimeInstance(DisplayInfo *displayInfo,
  DlElement *dlElement)
{
    CartesianPlot *pcp;
    int i, k, n, validTraces, iPrec;
    Arg args[46];
    Widget localWidget;
    XColor xColors[2];
    char *headerStrings[2];
    char rgb[2][16], string[24];
    int usedHeight, usedCharWidth, bestSize, preferredHeight;
    XcVType minF, maxF, tickF;
    DlCartesianPlot *dlCartesianPlot = dlElement->structure.cartesianPlot;

  /* Initialize hcpNullData if not done */
    if(!hcpNullData) {
	hcpNullData = CpDataCreate(CP_GENERAL,1,1);
	CpDataSetHole(hcpNullData,0.0);
	CpDataSetLastPoint(hcpNullData,0,0);
	CpDataSetXElement(hcpNullData,0,0,0.0);
	CpDataSetYElement(hcpNullData,0,0,0.0);
    }

  /* Allocate a CartesianPlot and fill part of it in */
    pcp = (CartesianPlot *) malloc(sizeof(CartesianPlot));
    pcp->hcp1 = pcp->hcp2 = NULL;
    pcp->dirty1 = pcp->dirty2 = False;
    pcp->timeScale = False;
    pcp->startTime.secPastEpoch = 0;
    pcp->startTime.nsec = 0;
    pcp->eraseMode = dlCartesianPlot->eraseMode;
    pcp->dlElement = dlElement;
    pcp->updateTask = updateTaskAddTask(displayInfo,
      &(dlCartesianPlot->object),
      cartesianPlotDraw,
      (XtPointer)pcp);
    if (pcp->updateTask == NULL) {
	medmPrintf(1,"\ncartesianPlotCreateRunTimeInstance: Memory allocation error\n");
    } else {
	updateTaskAddDestroyCb(pcp->updateTask,cartesianPlotDestroyCb);
	updateTaskAddNameCb(pcp->updateTask,cartesianPlotGetRecord);
    }

  /* Allocate (or set to NULL) the Records in the xyTrace's XYTraces */
  /*   Note: Set the updateValueCb for the time being to
   *     cartesianPlotUpdateScreenFirstTime(), which will change it
   *     to cartesianPlotUpdateValueCb() when it is finished */
    validTraces = 0;
    for (i = 0; i < MAX_TRACES; i++) {
	Boolean validTrace = False;
      /* X data */
	if (dlCartesianPlot->trace[i].xdata[0] != '\0') {
	    pcp->xyTrace[validTraces].recordX =
              medmAllocateRecord(dlCartesianPlot->trace[i].xdata,
                cartesianPlotUpdateScreenFirstTime,
                cartesianPlotUpdateGraphicalInfoCb,
                (XtPointer) &(pcp->xyTrace[validTraces]));
	    validTrace = True;
	} else {
	    pcp->xyTrace[validTraces].recordX = NULL;
	}
      /* Y data */
	if (dlCartesianPlot->trace[i].ydata[0] != '\0') {
	    pcp->xyTrace[validTraces].recordY =
              medmAllocateRecord(dlCartesianPlot->trace[i].ydata,
                cartesianPlotUpdateScreenFirstTime,
                cartesianPlotUpdateGraphicalInfoCb,
                (XtPointer) &(pcp->xyTrace[validTraces]));
	    validTrace = True;
	} else {
	    pcp->xyTrace[validTraces].recordY = NULL;
	}
	if (validTrace) {
	    pcp->xyTrace[validTraces].cartesianPlot = pcp;
	    validTraces++;
	}
    }

  /* If no xyTraces, create one fake one */
    if (validTraces == 0) {
	validTraces = 1;
	pcp->xyTrace[0].recordX = medmAllocateRecord(" ",
	  cartesianPlotUpdateScreenFirstTime,
	  cartesianPlotUpdateGraphicalInfoCb,
	  (XtPointer) &(pcp->xyTrace[0]));
	pcp->xyTrace[0].recordY = NULL;
    }

  /* Record the number of traces in the cartesian plot */
    pcp->nTraces = validTraces;

  /* Allocate (or set to NULL) the X Record in the eraseCh XYTrace */
    if ((dlCartesianPlot->erase[0] != '\0') && (validTraces > 0)) {
	pcp->eraseCh.recordX =
	  medmAllocateRecord(dlCartesianPlot->erase,
	    cartesianPlotUpdateScreenFirstTime,
	    cartesianPlotUpdateGraphicalInfoCb,
	    (XtPointer) &(pcp->eraseCh));
	pcp->eraseCh.cartesianPlot = pcp;
    } else {
	pcp->eraseCh.recordX = NULL;
    }

  /* Allocate (or set to NULL) the X Record in the triggerCh XYTrace */
    if ((dlCartesianPlot->trigger[0] != '\0') && (validTraces > 0)) {
	pcp->triggerCh.recordX = 
	  medmAllocateRecord(dlCartesianPlot->trigger,
	    cartesianPlotUpdateScreenFirstTime,
	    cartesianPlotUpdateGraphicalInfoCb,
	    (XtPointer) &(pcp->triggerCh));
	pcp->triggerCh.cartesianPlot = pcp;
    } else {
	pcp->triggerCh.recordX = NULL;
    }

  /* Note: Only the Record and CartesianPlot parts of the XYTrace's are filled
   *   in now, rest is filled in in cartesianPlotUpdateGraphicalInfoCb() */

    drawWhiteRectangle(pcp->updateTask);

  /* Set widget args from the dlCartesianPlot structure */
    n = 0;
    XtSetArg(args[n],XmNx,(Position)dlCartesianPlot->object.x); n++;
    XtSetArg(args[n],XmNy,(Position)dlCartesianPlot->object.y); n++;
    XtSetArg(args[n],XmNwidth,(Dimension)dlCartesianPlot->object.width); n++;
    XtSetArg(args[n],XmNheight,(Dimension)dlCartesianPlot->object.height); n++;
    XtSetArg(args[n],XmNhighlightThickness,0); n++;

#if DEBUG_CARTESIAN_PLOT_BORDER    
    printf("dlCartesianPlot->object.width: %d\n",dlCartesianPlot->object.width);
    printf("dlCartesianPlot->object.height: %d\n",dlCartesianPlot->object.height);
#endif    

  /* long way around for color handling... but XRT/Graph insists on strings! */
    xColors[0].pixel = displayInfo->colormap[dlCartesianPlot->plotcom.clr];
    xColors[1].pixel = displayInfo->colormap[dlCartesianPlot->plotcom.bclr];
    XQueryColors(display,cmap,xColors,2);
    sprintf(rgb[0],"#%2.2x%2.2x%2.2x",xColors[0].red>>8, xColors[0].green>>8,
      xColors[0].blue>>8);
    sprintf(rgb[1],"#%2.2x%2.2x%2.2x",xColors[1].red>>8, xColors[1].green>>8,
      xColors[1].blue>>8);
    XtSetArg(args[n],XtNxrtGraphForegroundColor,rgb[0]); n++;
    XtSetArg(args[n],XtNxrtGraphBackgroundColor,rgb[1]); n++;
    XtSetArg(args[n],XtNxrtForegroundColor,rgb[0]); n++;
    XtSetArg(args[n],XtNxrtBackgroundColor,rgb[1]); n++;
    preferredHeight = MIN(dlCartesianPlot->object.width,
      dlCartesianPlot->object.height)/TITLE_SCALE_FACTOR;
    bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
      preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
#if XRT_VERSION > 2
    XtSetArg(args[n],XtNxrtHeaderFont,fontListTable[bestSize]); n++;
#else
    XtSetArg(args[n],XtNxrtHeaderFont,fontTable[bestSize]->fid); n++;
#endif    
    if (strlen(dlCartesianPlot->plotcom.title) > 0) {
	headerStrings[0] = dlCartesianPlot->plotcom.title;
    } else {
	headerStrings[0] = NULL;
    }
    headerStrings[1] = NULL;
    XtSetArg(args[n],XtNxrtHeaderStrings,headerStrings); n++;
    if (strlen(dlCartesianPlot->plotcom.xlabel) > 0) {
	XtSetArg(args[n],XtNxrtXTitle,dlCartesianPlot->plotcom.xlabel); n++;
    } else {
	XtSetArg(args[n],XtNxrtXTitle,NULL); n++;
    }
    if (strlen(dlCartesianPlot->plotcom.ylabel) > 0) {
	XtSetArg(args[n],XtNxrtYTitle,dlCartesianPlot->plotcom.ylabel); n++;
    } else {
	XtSetArg(args[n],XtNxrtYTitle,NULL); n++;
    }
    preferredHeight = MIN(dlCartesianPlot->object.width,
      dlCartesianPlot->object.height)/AXES_SCALE_FACTOR;
    bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
      preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
#if XRT_VERSION > 2
    XtSetArg(args[n],XtNxrtAxisFont,fontListTable[bestSize]); n++;
#else
    XtSetArg(args[n],XtNxrtAxisFont,fontTable[bestSize]->fid); n++;
#endif    
    switch (dlCartesianPlot->style) {
    case POINT_PLOT:
    case LINE_PLOT:
	XtSetArg(args[n],XtNxrtType,XRT_PLOT); n++;
	if (validTraces > 1) {
	    XtSetArg(args[n],XtNxrtType2,XRT_PLOT); n++;
	}
	break;
    case FILL_UNDER_PLOT:
	XtSetArg(args[n],XtNxrtType,XRT_AREA); n++;
	if (validTraces > 1) {
	    XtSetArg(args[n],XtNxrtType2,XRT_AREA); n++;
	}
	break;
    }
    if (validTraces > 1) {
	XtSetArg(args[n],XtNxrtY2AxisShow,TRUE); n++;
    }

  /* X Axis Style */
    switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].axisStyle) {
    case LINEAR_AXIS:
	XtSetArg(args[n],XtNxrtXAnnotationMethod,XRT_ANNO_VALUES); n++;
	XtSetArg(args[n],XtNxrtXAxisLogarithmic,False); n++;
	break;
    case LOG10_AXIS:
	XtSetArg(args[n],XtNxrtXAnnotationMethod,XRT_ANNO_VALUES); n++;
	XtSetArg(args[n],XtNxrtXAxisLogarithmic,True); n++;
	break;
    case TIME_AXIS: {
	XtSetArg(args[n],XtNxrtXAxisLogarithmic,False); n++;
	XtSetArg(args[n],XtNxrtXAnnotationMethod,XRT_ANNO_TIME_LABELS); n++;
	XtSetArg(args[n],XtNxrtTimeBase,time900101); n++;
	XtSetArg(args[n],XtNxrtTimeUnit,XRT_TMUNIT_SECONDS); n++;
	XtSetArg(args[n],XtNxrtTimeFormatUseDefault,False); n++;
	XtSetArg(args[n],XtNxrtTimeFormat,
	  timeFormatString[dlCartesianPlot->axis[0].timeFormat
	    -FIRST_CP_TIME_FORMAT]); n++;
	pcp->timeScale = True;
    }
    break;
    default:
	medmPrintf(1,"\ncartesianPlotCreateRunTimeInstance: Unknown X axis style\n");
	break;
    }

  /* X Axis Range */
    switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle) {
    case CHANNEL_RANGE:		/* handle as default until connected */
    case AUTO_SCALE_RANGE:
	XtSetArg(args[n],XtNxrtXNumUseDefault,True); n++;
	XtSetArg(args[n],XtNxrtXTickUseDefault,True); n++;
	XtSetArg(args[n],XtNxrtXPrecisionUseDefault,True); n++;
	break;
    case USER_SPECIFIED_RANGE:
	minF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].minRange;
	maxF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].maxRange;
	tickF.fval = (maxF.fval - minF.fval)/4.0;
	XtSetArg(args[n],XtNxrtXMin,minF.lval); n++;
	XtSetArg(args[n],XtNxrtXMax,maxF.lval); n++;
	XtSetArg(args[n],XtNxrtXTick,tickF.lval); n++;
	XtSetArg(args[n],XtNxrtXNum,tickF.lval); n++;
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while (string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while (string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	XtSetArg(args[n],XtNxrtXPrecision,iPrec); n++;
	break;
    default:
	medmPrintf(1,"\ncartesianPlotCreateRunTimeInstance: Unknown X range style\n");
	break;
    }

  /* Y1 Axis Style */
    switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].axisStyle) {
    case LINEAR_AXIS:
	break;
    case LOG10_AXIS:
	XtSetArg(args[n],XtNxrtYAxisLogarithmic,True); n++;
	break;
    default:
	medmPrintf(1,"\ncartesianPlotCreateRunTimeInstance: Unknown Y1 axis style\n");
	break;
    }

  /* Y1 Axis Range */
    switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle) {
    case CHANNEL_RANGE:		/* handle as default until connected */
    case AUTO_SCALE_RANGE:
	XtSetArg(args[n],XtNxrtYNumUseDefault,True); n++;
	XtSetArg(args[n],XtNxrtYTickUseDefault,True); n++;
	XtSetArg(args[n],XtNxrtYPrecisionUseDefault,True); n++;
	break;
    case USER_SPECIFIED_RANGE:
	minF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].minRange;
	maxF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange;
	tickF.fval = (maxF.fval - minF.fval)/4.0;
	XtSetArg(args[n],XtNxrtYMin,minF.lval); n++;
	XtSetArg(args[n],XtNxrtYMax,maxF.lval); n++;
	XtSetArg(args[n],XtNxrtYTick,tickF.lval); n++;
	XtSetArg(args[n],XtNxrtYNum,tickF.lval); n++;
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while (string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while (string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	XtSetArg(args[n],XtNxrtYPrecision,iPrec); n++;
	break;
    default:
	medmPrintf(1,"\ncartesianPlotCreateRunTimeInstance: Unknown Y1 range style\n");
	break;
    }

  /* Y2 Axis Style */
    switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].axisStyle) {
    case LINEAR_AXIS:
	break;
    case LOG10_AXIS:
	XtSetArg(args[n],XtNxrtY2AxisLogarithmic,True); n++;
	break;
    default:
	medmPrintf(1,
	  "\ncartesianPlotCreateRunTimeInstance: Unknown Y2 axis style\n");
	break;
    }

  /* Y2 Axis Range */
    switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle) {
    case CHANNEL_RANGE:		/* handle as default until connected */
    case AUTO_SCALE_RANGE:
	XtSetArg(args[n],XtNxrtY2NumUseDefault,True); n++;
	XtSetArg(args[n],XtNxrtY2TickUseDefault,True); n++;
	XtSetArg(args[n],XtNxrtY2PrecisionUseDefault,True); n++;
	break;
    case USER_SPECIFIED_RANGE:
	minF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].minRange;
	maxF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].maxRange;
	tickF.fval = (maxF.fval - minF.fval)/4.0;
	XtSetArg(args[n],XtNxrtY2Min,minF.lval); n++;
	XtSetArg(args[n],XtNxrtY2Max,maxF.lval); n++;
	XtSetArg(args[n],XtNxrtY2Tick,tickF.lval); n++;
	XtSetArg(args[n],XtNxrtY2Num,tickF.lval); n++;
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while (string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while (string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	XtSetArg(args[n],XtNxrtY2Precision,iPrec); n++;
	break;
    default:
	medmPrintf(1,
	  "\ncartesianPlotCreateRunTimeInstance: Unknown Y2 range style\n");
	break;
    }

  /* Add pointer to CartesianPlot struct as userData to widget */
    XtSetArg(args[n], XmNuserData, (XtPointer)pcp); n++;

  /* Set miscellaneous  args */
    XtSetArg(args[n],XmNtraversalOn,False); n++;
    XtSetArg(args[n], XtNxrtDoubleBuffer, True); n++;

  /* Create the widget */
    localWidget = XtCreateWidget("cartesianPlot",xtXrtGraphWidgetClass,
      displayInfo->drawingArea, args, n);
    dlElement->widget = localWidget;

  /* Add destroy callback */
#if XRT_VERSION > 2    
#ifdef XRT_EXTENSIONS
    XtAddCallback(localWidget,XmNdestroyCallback,
      (XtCallbackProc)destroyXrtPropertyEditor, NULL);
#endif
#endif
}

void cartesianPlotCreateEditInstance(DisplayInfo *displayInfo,
  DlElement *dlElement)
{
    Channel *triggerCh;
    CartesianPlot *pcp;
    int k, n, validTraces, iPrec;
    Arg args[42];
    Widget localWidget;
    XColor xColors[2];
    char *headerStrings[2];
    char rgb[2][16], string[24];
    int usedHeight, usedCharWidth, bestSize, preferredHeight;
    XcVType minF, maxF, tickF;
    DlCartesianPlot *dlCartesianPlot = dlElement->structure.cartesianPlot;

    pcp = NULL;
    triggerCh = (Channel *)NULL;

    validTraces = 0;

  /* Set widget args from the dlCartesianPlot structure */
    n = 0;
    XtSetArg(args[n],XmNx,(Position)dlCartesianPlot->object.x); n++;
    XtSetArg(args[n],XmNy,(Position)dlCartesianPlot->object.y); n++;
    XtSetArg(args[n],XmNwidth,(Dimension)dlCartesianPlot->object.width); n++;
    XtSetArg(args[n],XmNheight,(Dimension)dlCartesianPlot->object.height); n++;
    XtSetArg(args[n],XmNhighlightThickness,0); n++;

  /* long way around for color handling... but XRT/Graph insists on strings! */
    xColors[0].pixel = displayInfo->colormap[dlCartesianPlot->plotcom.clr];
    xColors[1].pixel = displayInfo->colormap[dlCartesianPlot->plotcom.bclr];
    XQueryColors(display,cmap,xColors,2);
    sprintf(rgb[0],"#%2.2x%2.2x%2.2x",xColors[0].red>>8, xColors[0].green>>8,
      xColors[0].blue>>8);
    sprintf(rgb[1],"#%2.2x%2.2x%2.2x",xColors[1].red>>8, xColors[1].green>>8,
      xColors[1].blue>>8);
    XtSetArg(args[n],XtNxrtGraphForegroundColor,rgb[0]); n++;
    XtSetArg(args[n],XtNxrtGraphBackgroundColor,rgb[1]); n++;
    XtSetArg(args[n],XtNxrtForegroundColor,rgb[0]); n++;
    XtSetArg(args[n],XtNxrtBackgroundColor,rgb[1]); n++;
    preferredHeight = MIN(dlCartesianPlot->object.width,
      dlCartesianPlot->object.height)/TITLE_SCALE_FACTOR;
    bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
      preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
#if XRT_VERSION > 2
    XtSetArg(args[n],XtNxrtHeaderFont,fontListTable[bestSize]); n++;
#else
    XtSetArg(args[n],XtNxrtHeaderFont,fontTable[bestSize]->fid); n++;
#endif    
    if (strlen(dlCartesianPlot->plotcom.title) > 0) {
	headerStrings[0] = dlCartesianPlot->plotcom.title;
    } else {
	headerStrings[0] = NULL;
    }
    headerStrings[1] = NULL;
    XtSetArg(args[n],XtNxrtHeaderStrings,headerStrings); n++;
    if (strlen(dlCartesianPlot->plotcom.xlabel) > 0) {
	XtSetArg(args[n],XtNxrtXTitle,dlCartesianPlot->plotcom.xlabel); n++;
    } else {
	XtSetArg(args[n],XtNxrtXTitle,NULL); n++;
    }
    if (strlen(dlCartesianPlot->plotcom.ylabel) > 0) {
	XtSetArg(args[n],XtNxrtYTitle,dlCartesianPlot->plotcom.ylabel); n++;
    } else {
	XtSetArg(args[n],XtNxrtYTitle,NULL); n++;
    }
    preferredHeight = MIN(dlCartesianPlot->object.width,
      dlCartesianPlot->object.height)/AXES_SCALE_FACTOR;
    bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
      preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
#if XRT_VERSION > 2
    XtSetArg(args[n],XtNxrtAxisFont,fontListTable[bestSize]); n++;
#else
    XtSetArg(args[n],XtNxrtAxisFont,fontTable[bestSize]->fid); n++;
#endif    
    switch (dlCartesianPlot->style) {
    case POINT_PLOT:
    case LINE_PLOT:
	XtSetArg(args[n],XtNxrtType,XRT_PLOT); n++;
	if (validTraces > 1) {
	    XtSetArg(args[n],XtNxrtType2,XRT_PLOT); n++;
	}
	break;
    case FILL_UNDER_PLOT:
	XtSetArg(args[n],XtNxrtType,XRT_AREA); n++;
	if (validTraces > 1) {
	    XtSetArg(args[n],XtNxrtType2,XRT_AREA); n++;
	}
	break;
    }
    if (validTraces > 1) {
	XtSetArg(args[n],XtNxrtY2AxisShow,TRUE); n++;
    }

  /* X Axis Style */
    switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].axisStyle) {
    case LINEAR_AXIS:
	break;
    case LOG10_AXIS:
	XtSetArg(args[n],XtNxrtYAxisLogarithmic,True); n++;
	break;
    case TIME_AXIS:
	break;
    default:
	medmPrintf(1,"\ncartesianPlotCreateEditInstance: Unknown X axis style\n"); 
	break;
    }

  /* X Axis Range */
    switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle) {
    case CHANNEL_RANGE:		/* handle as default until connected */
    case AUTO_SCALE_RANGE:
	XtSetArg(args[n],XtNxrtXNumUseDefault,True); n++;
	XtSetArg(args[n],XtNxrtXTickUseDefault,True); n++;
	XtSetArg(args[n],XtNxrtXPrecisionUseDefault,True); n++;
	break;
    case USER_SPECIFIED_RANGE:
	minF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].minRange;
	maxF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].maxRange;
	tickF.fval = (maxF.fval - minF.fval)/4.0;
	XtSetArg(args[n],XtNxrtXMin,minF.lval); n++;
	XtSetArg(args[n],XtNxrtXMax,maxF.lval); n++;
	XtSetArg(args[n],XtNxrtXTick,tickF.lval); n++;
	XtSetArg(args[n],XtNxrtXNum,tickF.lval); n++;
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while (string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while (string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	XtSetArg(args[n],XtNxrtXPrecision,iPrec); n++;
	break;
    default:
	medmPrintf(1,
	  "\ncartesianPlotCreateEditInstance: Unknown X range style\n");
	break;
    }

    /* Y1 Axis Style */
    switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].axisStyle) {
    case LINEAR_AXIS:
	break;
    case LOG10_AXIS:
	XtSetArg(args[n],XtNxrtYAxisLogarithmic,True); n++;
	break;
    default:
	medmPrintf(1,
	  "\ncartesianPlotCreateEditInstance: Unknown Y1 axis style\n");
	break;
    }

    /* Y1 Axis Range */
    switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle) {
    case CHANNEL_RANGE:		/* handle as default until connected */
    case AUTO_SCALE_RANGE:
	XtSetArg(args[n],XtNxrtYNumUseDefault,True); n++;
	XtSetArg(args[n],XtNxrtYTickUseDefault,True); n++;
	XtSetArg(args[n],XtNxrtYPrecisionUseDefault,True); n++;
	break;
    case USER_SPECIFIED_RANGE:
	minF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].minRange;
	maxF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange;
	tickF.fval = (maxF.fval - minF.fval)/4.0;
	XtSetArg(args[n],XtNxrtYMin,minF.lval); n++;
	XtSetArg(args[n],XtNxrtYMax,maxF.lval); n++;
	XtSetArg(args[n],XtNxrtYTick,tickF.lval); n++;
	XtSetArg(args[n],XtNxrtYNum,tickF.lval); n++;
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while (string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while (string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	XtSetArg(args[n],XtNxrtYPrecision,iPrec); n++;
	break;
    default:
	medmPrintf(1,
	  "\ncartesianPlotCreateEditInstance: Unknown Y1 range style\n");
	break;
    }

  /* Y2 Axis Style */
    switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].axisStyle) {
    case LINEAR_AXIS:
	break;
    case LOG10_AXIS:
	XtSetArg(args[n],XtNxrtY2AxisLogarithmic,True); n++;
	break;
    default:
	medmPrintf(1,
	  "\ncartesianPlotCreateEditInstance: Unknown Y2 axis style\n");
	break;
    }

  /* Y2 Axis Range */
    switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle) {
    case CHANNEL_RANGE:		/* handle as default until connected */
    case AUTO_SCALE_RANGE:
	XtSetArg(args[n],XtNxrtY2NumUseDefault,True); n++;
	XtSetArg(args[n],XtNxrtY2TickUseDefault,True); n++;
	XtSetArg(args[n],XtNxrtY2PrecisionUseDefault,True); n++;
	break;
    case USER_SPECIFIED_RANGE:
	minF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].minRange;
	maxF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].maxRange;
	tickF.fval = (maxF.fval - minF.fval)/4.0;
	XtSetArg(args[n],XtNxrtY2Min,minF.lval); n++;
	XtSetArg(args[n],XtNxrtY2Max,maxF.lval); n++;
	XtSetArg(args[n],XtNxrtY2Tick,tickF.lval); n++;
	XtSetArg(args[n],XtNxrtY2Num,tickF.lval); n++;
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while (string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while (string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	XtSetArg(args[n],XtNxrtY2Precision,iPrec); n++;
	break;
    default:
	medmPrintf(1,
	  "\ncartesianPlotCreateEditInstance: Unknown Y2 range style\n");
	break;
    }

  /*Add pointer to CartesianPlot struct as userData to widget */
    XtSetArg(args[n], XmNuserData, (XtPointer) pcp); n++;

  /* Set miscellaneous  args */
    XtSetArg(args[n],XmNtraversalOn,False); n++;
    XtSetArg(args[n], XtNxrtDoubleBuffer, True); n++;

  /* Create the widget */
    localWidget = XtCreateWidget("cartesianPlot",xtXrtGraphWidgetClass,
      displayInfo->drawingArea, args, n);
    dlElement->widget = localWidget;

  /* Add handlers */
    addCommonHandlers(localWidget, displayInfo);

    XtManageChild(localWidget);
}

void executeDlCartesianPlot(DisplayInfo *displayInfo, DlElement *dlElement)
{
    if (dlElement->widget) {
	DlObject *po = &(dlElement->structure.cartesianPlot->object);
	XtVaSetValues(dlElement->widget,
	  XmNx, (Position) po->x,
	  XmNy, (Position) po->y,
	  XmNwidth, (Dimension) po->width,
	  XmNheight, (Dimension) po->height,
	  NULL);
    } else if (displayInfo->traversalMode == DL_EXECUTE) {
	cartesianPlotCreateRunTimeInstance(displayInfo, dlElement);
    } else if (displayInfo->traversalMode == DL_EDIT) {
	    cartesianPlotCreateEditInstance(displayInfo, dlElement);
    }
}

static void cartesianPlotUpdateGraphicalInfoCb(XtPointer cd) {
    Record *pr = (Record *)cd;
    XYTrace *pt = (XYTrace *)pr->clientData;
    CartesianPlot *pcp = pt->cartesianPlot;
    DisplayInfo *displayInfo = pcp->updateTask->displayInfo;
    DlCartesianPlot *dlCartesianPlot = pcp->dlElement->structure.cartesianPlot;
    Widget widget = pcp->dlElement->widget;
    XColor xColors[MAX_TRACES];
    char rgb[MAX_TRACES][16];
    CpDataHandle hcp1, hcp2;
    CpDataStyle myds1[MAX_TRACES], myds2[MAX_TRACES];
    float minX, maxX, minY, maxY, minY2, maxY2;
    XcVType minXF, maxXF, minYF, maxYF, minY2F, maxY2F, tickF;
    char string[24];
    int iPrec, kk;

    int maxElements = 0;
    int i,j;
    int n;
    Arg widgetArgs[20];

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! This is a temporary work around !!!!! */
  /* !!!!! for the reconnection.           !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
    
  /* KE: This removes the updateGraphicalInfoCb in the Record */
    medmRecordAddGraphicalInfoCb(pr,NULL);

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! End work around                 !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  /* Return until all channels get their graphical information */
    for (i = 0; i < pcp->nTraces; i++) {
	XYTrace *t = &(pcp->xyTrace[i]);
	if (t->recordX) {
	    if (t->recordX->precision < 0) return;
	}

	if (t->recordY) {
	    if (t->recordY->precision < 0) return;
	}
    }

  /* Get the colors for all the possible traces */
    for (i = 0; i < pcp->nTraces; i++)
      xColors[i].pixel = displayInfo->colormap[dlCartesianPlot->trace[i].data_clr];
    XQueryColors(XtDisplay(widget),cmap,xColors,pcp->nTraces);
    for (i = 0; i < pcp->nTraces; i++) {
	sprintf(rgb[i],"#%2.2x%2.2x%2.2x", xColors[i].red>>8,
	  xColors[i].green>>8, xColors[i].blue>>8);
    }

  /* Set the maximum count (for allocating data structures) to be the
   *   largest of any of the Channel Access counts and the specified
       count (from the DlCartesianPlot) */
    maxElements = dlCartesianPlot->count;
    for (i = 0; i < pcp->nTraces; i++) {
	XYTrace *t = &(pcp->xyTrace[i]);
	if (t->recordX)
	  maxElements = MAX(maxElements,(int)t->recordX->elementCount);
	if (t->recordY)
	  maxElements = MAX(maxElements,(int)t->recordY->elementCount);
    }

  /* Allocate the CP data structures with size maxElements */
    hcp1 = hcp2 = NULL;
    hcp1 = CpDataCreate(CP_GENERAL,1,maxElements);
    if (pcp->nTraces > 1) {
	hcp2 = CpDataCreate(CP_GENERAL,pcp->nTraces-1,maxElements);
    }

  /* Loop over all the traces */
    minX = minY = minY2 = FLT_MAX;
    maxX = maxY = maxY2 = -0.99*FLT_MAX;
    for (i = 0; i < pcp->nTraces; i++) {
	XYTrace *t = &(pcp->xyTrace[i]);

      /* Set as uninitialized (Used for incrementing last point) */
	 t->init=0;

      /* Determine data type (based on type (scalar or vector) of data) */
	if (t->recordX && t->recordY) {
	    if ( t->recordX->elementCount > 1 && t->recordY->elementCount > 1 ) {
		t->type = CP_XYVector;
	    } else if ( t->recordX->elementCount > 1 && t->recordY->elementCount <= 1 ) {
		t->type = CP_XVectorYScalar;
	    } else if ( t->recordY->elementCount > 1 && t->recordX->elementCount <= 1 ) {
		t->type = CP_YVectorXScalar;
	    } else if ( t->recordX->elementCount <= 1 && t->recordY->elementCount <= 1 ) {
		t->type = CP_XYScalar;
	    }
	  /* Put initial data in */
	    if (i <= 0) {
		CpDataSetLastPoint(hcp1,i,0);
		minY = MIN(minY,t->recordY->lopr);
		maxY = MAX(maxY,t->recordY->hopr);
	    } else {
		CpDataSetLastPoint(hcp2,i-1,0);
		minY2 = MIN(minY2,t->recordY->lopr);
		maxY2 = MAX(maxY2,t->recordY->hopr);
	    }
	    minX = MIN(minX,t->recordX->lopr);
	    maxX = MAX(maxX,t->recordX->hopr);
	} else if (t->recordX) {
	  /* X channel - supporting scalar or waveform */
	    if (t->recordX->elementCount > 1) {
	      /* Vector/waveform */
		t->type = CP_XVector;
		if (i <= 0) {
		    for (j = 0; j < (int)t->recordX->elementCount; j++)
		      CpDataSetYElement(hcp1,i,j,(float)j);
		    minY = MIN(minY,0.);
		    maxY = MAX(maxY,(float)((int)t->recordX->elementCount-1));
		} else {
		    for(j = 0; j < (int)t->recordX->elementCount; j++)
		     CpDataSetYElement(hcp2,i-1,j,(float)j);
		    minY2 = MIN(minY2,0.);
		    maxY2 = MAX(maxY2,(float)t->recordX->elementCount-1);
		}
		minX = MIN(minX,t->recordX->lopr);
		maxX = MAX(maxX,t->recordX->hopr);
	    } else {
	      /* Scalar */
		t->type = CP_XScalar;
		if (i <= 0) {
		    for(j = 0; j < dlCartesianPlot->count; j++)
		      CpDataSetYElement(hcp1,i,j,(float)j);
		    CpDataSetXElement(hcp1,i,0,(float)t->recordX->value);
		    minY = MIN(minY,0.);
		    maxY = MAX(maxY,(float)dlCartesianPlot->count);
		} else {
		    for(j = 0; j < dlCartesianPlot->count; j++)
		      CpDataSetYElement(hcp2,i-1,j,(float)j);
		    CpDataSetXElement(hcp2,i-1,0,(float)t->recordX->value);
		    minY2 = MIN(minY2,0.);
		    maxY2 = MAX(maxY2,(float)dlCartesianPlot->count);
		}
		minX = MIN(minX,t->recordX->lopr);
		maxX = MAX(maxX,t->recordX->hopr);
	    }
	} else if (t->recordY) {
	  /* Y channel - supporting scalar or waveform */
	    if (t->recordY->elementCount > 1) {
	      /* Vector/waveform */
		t->type = CP_YVector;
		if (i <= 0) {
		    for(j = 0; j < t->recordY->elementCount; j++)
		      CpDataSetYElement(hcp1,i,j,(float)j);
		    minY = MIN(minY,t->recordY->lopr);
		    maxY = MAX(maxY,t->recordY->hopr);
		} else {
		    for(j = 0; j < t->recordY->elementCount; j++)
		      CpDataSetXElement(hcp2,i-1,j,(float)j);
		    minY2 = MIN(minY2,t->recordY->lopr);
		    maxY2 = MAX(maxY2,t->recordY->hopr);
		}
		minX = MIN(minX,0.);
		maxX = MAX(maxX,(float)(t->recordY->elementCount-1));
	    } else {
	      /* Scalar */
		t->type = CP_YScalar;
		if (i <= 0) {
		    for(j = 0; j < dlCartesianPlot->count; j++)
		      CpDataSetXElement(hcp1,0,j,(float)j);
		    CpDataSetYElement(hcp1,0,0,(float)t->recordY->value);
		    CpDataSetLastPoint(hcp1,0,0);
		    minY = MIN(minY,t->recordY->lopr);
		    maxY = MAX(maxY,t->recordY->hopr);
		} else {
		    for(j = 0; j < dlCartesianPlot->count; j++)
		      CpDataSetXElement(hcp2,0,j,(float)j);
		    CpDataSetYElement(hcp2,0,0,(float)t->recordY->value);
		    CpDataSetLastPoint(hcp2,0,0);
		    minY2 = MIN(minY2,t->recordY->lopr);
		    maxY2 = MAX(maxY2,t->recordY->hopr);
		}
		minX = MIN(minX,0.);
		maxX = MAX(maxX,(float)dlCartesianPlot->count);
	    }
	}
    }     /* End for loop over traces */
    
  /* Loop over traces and initialize CpDataStyle array */
    for (i = 0; i < MAX_TRACES; i++) {
      /* there is really only going to be one myds1 in a graph */
	myds1[i].lpat = XRT_LPAT_SOLID;
	myds1[i].fpat = XRT_FPAT_SOLID;
	myds1[i].color = rgb[i];
	myds1[i].width = 1;
	myds1[i].point = XRT_POINT_DOT;
	myds1[i].pcolor = rgb[i];
	myds1[i].psize = MAX(2,dlCartesianPlot->object.height/70);
	myds1[i].res1 = 0;
	myds1[i].res1 = 0;

	myds2[i].lpat = XRT_LPAT_SOLID;
	myds2[i].fpat = (XrtFillPattern) (XRT_FPAT_SOLID + i);
	myds2[i].color = rgb[i];
	myds2[i].width = 1;
	myds2[i].point = (XrtPoint)(XRT_POINT_BOX+i);
	myds2[i].pcolor = rgb[i];
	myds2[i].psize = MAX(2,dlCartesianPlot->object.height/70);
	myds2[i].res1 = 0;
	myds2[i].res1 = 0;
    }


  /* Loop over traces and set CpDataStyle array */
    for (i = 0; i < pcp->nTraces; i++) {
	switch(dlCartesianPlot->style) {
	case POINT_PLOT:
	    if (i <= 0) {
		myds1[i].lpat = XRT_LPAT_NONE;
		myds1[i].fpat = XRT_FPAT_NONE;
		myds1[i].color = rgb[i];
		myds1[i].pcolor = rgb[i];
		myds1[i].psize = MAX(2,dlCartesianPlot->object.height/70);
		XrtSetNthDataStyle(widget,i,&myds1[i]);
	    } else {
		myds2[i-1].lpat = XRT_LPAT_NONE;
		myds2[i-1].fpat = XRT_FPAT_NONE;
		myds2[i-1].color = rgb[i];
		myds2[i-1].pcolor = rgb[i];
		myds2[i-1].psize = MAX(2, dlCartesianPlot->object.height/70);
		XrtSetNthDataStyle2(widget, i-1,&myds2[i-1]);
	    }
	    break;
	case LINE_PLOT:
	    if (i <= 0) {
		myds1[i].fpat = XRT_FPAT_NONE;
		myds1[i].color = rgb[i];
		myds1[i].pcolor = rgb[i];
		myds1[i].psize = MAX(2, dlCartesianPlot->object.height/70);
		CpSetNthDataStyle(widget,i,&myds1[i]);
	    } else {
		myds2[i-1].fpat = XRT_FPAT_NONE;
		myds2[i-1].color = rgb[i];
		myds2[i-1].pcolor = rgb[i];
		myds2[i-1].psize = MAX(2, dlCartesianPlot->object.height/70);
		CpSetNthDataStyle2(widget, i-1,&myds2[i-1]);
	    }
	    break;
	case FILL_UNDER_PLOT:
	    if (i <= 0) {
		myds1[i].color = rgb[i];
		myds1[i].pcolor = rgb[i];
		myds1[i].psize = MAX(2, dlCartesianPlot->object.height/70);
		CpSetNthDataStyle(widget,i,&myds1[i]);
	    } else {
		myds2[i-1].color = rgb[i];
		myds2[i-1].pcolor = rgb[i];
		myds2[i-1].psize = MAX(2, dlCartesianPlot->object.height/70);
		CpSetNthDataStyle2(widget, i-1,&myds2[i-1]);
	    }
	    break;
	}
    }

  /* Record the trace number and set the data pointers in the XYTrace */
    for (i = 0; i < pcp->nTraces; i++) {
	XYTrace *t = &(pcp->xyTrace[i]);

	if ( i <= 0) {
	    t->hcp = hcp1;
	    t->trace = i;
	} else {
	    t->hcp = hcp2;
	    t->trace = i-1;
	}
    }

  /* Set the data pointers in the CartesianPlot */
    pcp->hcp1 = hcp1;
    pcp->hcp2 = hcp2;

  /* Fill in connect-time channel-based range specifications
   *   This is different than the min/max stored in the display element */
    pcp->axisRange[X_AXIS_ELEMENT].axisMin = minX;
    pcp->axisRange[X_AXIS_ELEMENT].axisMax = maxX;
    pcp->axisRange[Y1_AXIS_ELEMENT].axisMin = minY;
    pcp->axisRange[Y1_AXIS_ELEMENT].axisMax = maxY;
    pcp->axisRange[Y2_AXIS_ELEMENT].axisMin = minY2;
    pcp->axisRange[Y2_AXIS_ELEMENT].axisMax = maxY2;
    for (kk = X_AXIS_ELEMENT; kk <= Y2_AXIS_ELEMENT; kk++) {
	if (dlCartesianPlot->axis[kk].rangeStyle==CHANNEL_RANGE) {
	    pcp->axisRange[kk].isCurrentlyFromChannel= True;
	} else {
	    pcp->axisRange[kk].isCurrentlyFromChannel = False;
	}
    }
    minXF.fval = minX;
    maxXF.fval = maxX;
    minYF.fval = minY;
    maxYF.fval = maxY;
    minY2F.fval = minY2;
    maxY2F.fval = maxY2;
    n = 0;
    if (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle
      == CHANNEL_RANGE) {
	XtSetArg(widgetArgs[n],XtNxrtXMin,minXF.lval); n++;
	XtSetArg(widgetArgs[n],XtNxrtXMax,maxXF.lval); n++;
    } else if (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle
      == USER_SPECIFIED_RANGE) {
	int k;

	minXF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].minRange;
	maxXF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].maxRange;
	
	tickF.fval = (maxXF.fval - minXF.fval)/4.0;
	XtSetArg(widgetArgs[n],XtNxrtXMin,minXF.lval); n++;
	XtSetArg(widgetArgs[n],XtNxrtXMax,maxXF.lval); n++;
	XtSetArg(widgetArgs[n],XtNxrtXTick,tickF.lval); n++;
	XtSetArg(widgetArgs[n],XtNxrtXNum,tickF.lval); n++;
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while (string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while (string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	XtSetArg(widgetArgs[n],XtNxrtXPrecision,iPrec); n++;
    }

  /* Set axis parameters for Y1 */
     if (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle
       == CHANNEL_RANGE) {
	 XtSetArg(widgetArgs[n],XtNxrtYMin,minYF.lval); n++;
	 XtSetArg(widgetArgs[n],XtNxrtYMax,maxYF.lval); n++;
     } else if (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle
       == USER_SPECIFIED_RANGE) {
	 int k;
	 minYF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].minRange;
	 maxYF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange;
	 
	 tickF.fval = (maxYF.fval - minYF.fval)/4.0;
	 XtSetArg(widgetArgs[n],XtNxrtYMin,minYF.lval); n++;
	 XtSetArg(widgetArgs[n],XtNxrtYMax,maxYF.lval); n++;
	 XtSetArg(widgetArgs[n],XtNxrtYTick,tickF.lval); n++;
	 XtSetArg(widgetArgs[n],XtNxrtYNum,tickF.lval); n++;
	 sprintf(string,"%f",tickF.fval);
	 k = strlen(string)-1;
	 while (string[k] == '0') k--; /* strip off trailing zeroes */
	 iPrec = k;
	 while (string[k] != '.' && k >= 0) k--;
	    iPrec = iPrec - k;
	    XtSetArg(widgetArgs[n],XtNxrtYPrecision,iPrec); n++;
     }

  /* Set axis parameters for Y2 */
    if (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle
      == CHANNEL_RANGE) {
	XtSetArg(widgetArgs[n],XtNxrtY2Min,minY2F.lval); n++;
	XtSetArg(widgetArgs[n],XtNxrtY2Max,maxY2F.lval); n++;

    } else if (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle
      == USER_SPECIFIED_RANGE) {
	int k;
	minY2F.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].minRange;
	maxY2F.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].maxRange;
	
	tickF.fval = (maxY2F.fval - minY2F.fval)/4.0;
	XtSetArg(widgetArgs[n],XtNxrtY2Min,minY2F.lval); n++;
	XtSetArg(widgetArgs[n],XtNxrtY2Max,maxY2F.lval); n++;
	XtSetArg(widgetArgs[n],XtNxrtY2Tick,tickF.lval); n++;
	XtSetArg(widgetArgs[n],XtNxrtY2Num,tickF.lval); n++;
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while (string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while (string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	XtSetArg(widgetArgs[n],XtNxrtY2Precision,iPrec); n++;
    }
    
  /* Set the values in the widget */
    XtSetValues(widget,widgetArgs,n);
    
#if 0
  /* Add destroy callback */
    XtAddCallback(widget,XmNdestroyCallback,
      (XtCallbackProc)monitorDestroy, (XtPointer)pcp);
#endif

  /* Now call the plot update routine */
    cartesianPlotUpdateScreenFirstTime(cd);
}

void cartesianPlotUpdateTrace(XtPointer cd) {
    Record *pr = (Record *) cd;

    XYTrace *pt = (XYTrace *) pr->clientData;
    CartesianPlot *pcp = pt->cartesianPlot;
    DlCartesianPlot *dlCartesianPlot = pcp->dlElement->structure.cartesianPlot;
    int nextPoint, j;
    Arg args[20];

#if DEBUG_CARTESIAN_PLOT_UPDATE
    printf("cartesianPlotUpdateTrace:\n");
#endif    
    switch(pt->type) {
    case CP_XYScalar:
      /* x,y channels specified - scalars, up to dlCartesianPlot->count pairs */
	nextPoint = CpDataGetLastPoint(pt->hcp,pt->trace);
	if(pt->init) nextPoint++;
	else pt->init=1;
	if (nextPoint < dlCartesianPlot->count) {
	    CpDataSetXElement(pt->hcp,pt->trace,nextPoint,
	      SAFEFLOAT(pt->recordX->value));
	    CpDataSetYElement(pt->hcp,pt->trace,nextPoint,
	      SAFEFLOAT(pt->recordY->value));
	    CpDataSetLastPoint(pt->hcp,pt->trace,nextPoint);
	} else {
	    if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
	      /* All done, don't add any more points */
	    } else if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
	      /* Shift everybody down one, add at end */
		int j;
		for (j = 1; j < dlCartesianPlot->count; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j-1,
		      CpDataGetXElement(pt->hcp,pt->trace,j));
		    CpDataSetYElement(pt->hcp,pt->trace,j-1,
		      CpDataGetYElement(pt->hcp,pt->trace,j));
		}
		CpDataSetXElement(pt->hcp,pt->trace,j-1,
		  SAFEFLOAT(pt->recordX->value));
		CpDataSetYElement(pt->hcp,pt->trace,j-1,
		  SAFEFLOAT(pt->recordY->value));
	    }
	}
#if 0     /* Use with cpCP_XYScalar.adl or delete these lines */
	printf("nextPoint=%d dlCartesianPlot->count=%d x=%g y=%g\n",
	  nextPoint,dlCartesianPlot->count,
	  pt->recordX->value,pt->recordY->value);
	printf("x1=%g x2=%g x3=%g\n",
	  CpDataGetXElement(pt->hcp,pt->trace,0),
	  CpDataGetXElement(pt->hcp,pt->trace,1),
	  CpDataGetXElement(pt->hcp,pt->trace,2));
	printf("y1=%g y2=%g y3=%g\n\n",
	  CpDataGetYElement(pt->hcp,pt->trace,0),
	  CpDataGetYElement(pt->hcp,pt->trace,1),
	  CpDataGetYElement(pt->hcp,pt->trace,2));
#endif		
	break;

    case CP_XScalar:
      /* x channel scalar, up to dlCartesianPlot->count pairs */
	nextPoint = CpDataGetLastPoint(pt->hcp,pt->trace);
	if(pt->init) nextPoint++;
	else pt->init=1;
	if (nextPoint < dlCartesianPlot->count) {
	    CpDataSetXElement(pt->hcp,pt->trace,nextPoint,
	      SAFEFLOAT(pt->recordX->value));
	    CpDataSetYElement(pt->hcp,pt->trace,nextPoint,(float)nextPoint);
	    CpDataSetLastPoint(pt->hcp,pt->trace,nextPoint);
	} else {
	    if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
	      /* All done, don't add any more points */
	    } else if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
	      /* Shift everybody down one, add at end */
		int j;
		for (j = 1; j < dlCartesianPlot->count; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j-1,
		      CpDataGetXElement(pt->hcp,pt->trace,j));
		    CpDataSetYElement(pt->hcp,pt->trace,j-1,(float)(j-1));
		}
		CpDataSetXElement(pt->hcp,pt->trace,j-1,
		  SAFEFLOAT(pt->recordX->value));
		CpDataSetYElement(pt->hcp,pt->trace,j-1,(float)(j-1));
	    }
	}
	break;


    case CP_XVector:
      /* x channel vector, ca_element_count(chid) elements */
	CpDataSetLastPoint(pt->hcp,pt->trace,MAX(pt->recordX->elementCount-1,0));
	switch(pt->recordX->dataType) {
	case DBF_STRING:
	{
	    for (j = 0; j < pt->recordX->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,0.0);
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_INT:
	{
	    short *pShort = (short *) pt->recordX->array;
	    for (j = 0; j < pt->recordX->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)pShort[j]);
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_FLOAT:
	{
	    float *pFloat = (float *) pt->recordX->array;
	    for (j = 0; j < pt->recordX->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,SAFEFLOAT(pFloat[j]));
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_ENUM:
	{
	    unsigned short *pUShort = (unsigned short *) pt->recordX->array;
	    for (j = 0; j < pt->recordX->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)pUShort[j]);
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_CHAR:
	{
	    unsigned char *pUChar = (unsigned char *) pt->recordX->array;
	    for (j = 0; j < pt->recordX->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)pUChar[j]);
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_LONG:
	{
	    dbr_long_t *pLong = (dbr_long_t *) pt->recordX->array;
	    for (j = 0; j < pt->recordX->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)pLong[j]);
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_DOUBLE:
	{
	    double *pDouble = (double *) pt->recordX->array;
	    for (j = 0; j < pt->recordX->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,SAFEFLOAT(pDouble[j]));
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	    }
	}
	break;

    case CP_YScalar:
      /* y channel scalar, up to dlCartesianPlot->count pairs */
	nextPoint = CpDataGetLastPoint(pt->hcp,pt->trace);
	if(pt->init) nextPoint++;
	else pt->init=1;
#if DEBUG_CARTESIAN_PLOT_UPDATE
	printf("  nextPoint=%d dlCartesianPlot->count=%d\n  XRT_HUGE_VAL=%f\n",
	  nextPoint,
	  dlCartesianPlot->count,
	  XRT_HUGE_VAL);
#endif    
	if (nextPoint < dlCartesianPlot->count) {
	    if (!nextPoint && pcp->timeScale) {
		time_t tval;
		tval = timeOffset + (time_t) pt->recordY->time.secPastEpoch;
		XtVaSetValues(pcp->dlElement->widget,
		  XtNxrtTimeBase,tval,
		  NULL);
		pcp->startTime = pt->recordY->time;
	    }
	    CpDataSetXElement(pt->hcp,pt->trace,nextPoint,(pcp->timeScale) ?
	      (float)(pt->recordY->time.secPastEpoch -
		pcp->startTime.secPastEpoch) :
	      (float) nextPoint);
	    CpDataSetYElement(pt->hcp,pt->trace,nextPoint,
	      (float)pt->recordY->value);
	    CpDataSetLastPoint(pt->hcp,pt->trace,nextPoint);
	} else {
	    if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
	      /* All done, don't add any more points */
#if DEBUG_CARTESIAN_PLOT_UPDATE
		printf("  Array full\n");
#endif    
	    } else if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
	      /* Shift everybody down one, add at end */
		int j;

#if DEBUG_CARTESIAN_PLOT_UPDATE
		printf("  Shifting\n");
#endif    
		if (pcp->timeScale) {
		    for (j = 1; j < dlCartesianPlot->count; j++) {
			CpDataSetXElement(pt->hcp,pt->trace,j-1,
			  CpDataGetXElement(pt->hcp,pt->trace,j));
			CpDataSetYElement(pt->hcp,pt->trace,j-1,
			  CpDataGetYElement(pt->hcp,pt->trace,j));
		    }
		    CpDataSetXElement(pt->hcp,pt->trace,j-1,
		      (float)(pt->recordY->time.secPastEpoch -
		      pcp->startTime.secPastEpoch)); 
		    CpDataSetYElement(pt->hcp,pt->trace,j-1,
		      (float)pt->recordY->value);
		} else {
		    for (j = 1; j < dlCartesianPlot->count; j++) {
			CpDataSetYElement(pt->hcp,pt->trace,j-1,
			  CpDataGetYElement(pt->hcp,pt->trace,j));
		    }
		    CpDataSetYElement(pt->hcp,pt->trace,j-1,
		      (float)pt->recordY->value);
		} 
	    }
	}
#if DEBUG_CARTESIAN_PLOT_UPDATE
	{
	    int nextPoint1=(nextPoint < dlCartesianPlot->count)?nextPoint:
	      dlCartesianPlot->count-1;
	    static char filename[]="xrtdata.data";

	    if(nextPoint == 10) {
		XrtDataSaveToFile(pt->hcp,filename,NULL);
		printf("Data saved to %s\n",filename);
	    }
	    
	    printf("  nextPoint=%d x=       %f y=       %f\n",
	      nextPoint,
	      (pcp->timeScale) ?
	      (float)(pt->recordY->time.secPastEpoch -
		pcp->startTime.secPastEpoch) :
	      (float)nextPoint1,
	      (float)pt->recordY->value);
	    printf("  nextPoint=%d XElement=%f YElement=%f\n",
	      CpDataGetLastPoint(pt->hcp,pt->trace),
	      CpDataGetXElement(pt->hcp,pt->trace,nextPoint1),
	      CpDataGetYElement(pt->hcp,pt->trace,nextPoint1));
	}
#endif    
	break;

    case CP_YVector:
      /* plot first "count" elements of vector per dlCartesianPlot */
	CpDataSetLastPoint(pt->hcp,pt->trace,pt->recordY->elementCount-1);
	switch(pt->recordY->dataType) {
	case DBF_STRING:
	{
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetYElement(pt->hcp,pt->trace,j,0.0);
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_INT:
	{
	    short *pShort = (short *) pt->recordY->array;
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)pShort[j]);
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_FLOAT:
	{
	    float *pFloat = (float *) pt->recordY->array;
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetYElement(pt->hcp,pt->trace,j,SAFEFLOAT(pFloat[j]));
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_ENUM:
	{
	    unsigned short *pUShort = (unsigned short *) pt->recordY->array;
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)pUShort[j]);
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_CHAR:
	{
	    unsigned char *pUChar = (unsigned char *) pt->recordY->array;
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)pUChar[j]);
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_LONG:
	{
	    dbr_long_t *pLong = (dbr_long_t *) pt->recordY->array;
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)pLong[j]);
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_DOUBLE:
	{
	    double *pDouble = (double *) pt->recordY->array;
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetYElement(pt->hcp,pt->trace,j,SAFEFLOAT(pDouble[j]));
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	}
	break;


    case CP_XVectorYScalar:
	CpDataSetLastPoint(pt->hcp,pt->trace,MAX(pt->recordX->elementCount-1,0));
	if (pr == pt->recordX) {
	  /* plot first "count" elements of vector per dlCartesianPlot */
	    switch(pt->recordX->dataType) {
	    case DBF_STRING:
	    {
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j,0.0);
		}
		break;
	    }
	    case DBF_INT:
	    {
		short *pShort = (short *) pt->recordX->array;
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j,(float)pShort[j]);
		}
		break;
	    }
	    case DBF_FLOAT:
	    {
		float *pFloat = (float *) pt->recordX->array;
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j,SAFEFLOAT(pFloat[j]));
		}
		break;
	    }
	    case DBF_ENUM:
	    {
		unsigned short *pUShort = (unsigned short *) pt->recordX->array;
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j,(float)pUShort[j]);
		}
		break;
	    }
	    case DBF_CHAR:
	    {
		unsigned char *pUChar = (unsigned char *) pt->recordX->array;
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j,(float)pUChar[j]);
		}
		break;
	    }
	    case DBF_LONG:
	    {
		dbr_long_t *pLong = (dbr_long_t *) pt->recordX->array;
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j,(float)pLong[j]);
		}
		break;
	    }
	    case DBF_DOUBLE:
	    {
		double *pDouble = (double *) pt->recordX->array;
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j,
		      SAFEFLOAT(pDouble[j]));
		}
		break;
	    }
	    }
	} else {
	    if (pr == pt->recordY) {
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,
		      SAFEFLOAT(pt->recordY->value));
		}
	    }
	}
	break;

    case CP_YVectorXScalar:
	CpDataSetLastPoint(pt->hcp,pt->trace,MAX(pt->recordY->elementCount-1,0));
	if (pr == pt->recordY) {
	  /* plot first "count" elements of vector per dlCartesianPlot */
	    switch(pt->recordY->dataType) {
	    case DBF_STRING:
	    {
		for (j = 0; j < pt->recordY->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,0.0);
		}
		break;
	    }
	    case DBF_INT:
	    {
		short *pShort = (short *) pt->recordY->array;
		for (j = 0; j < pt->recordY->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,(float)pShort[j]);
		}
		break;
	    }
	    case DBF_FLOAT:
	    {
		float *pFloat = (float *) pt->recordY->array;
		for (j = 0; j < pt->recordY->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,
		      SAFEFLOAT(pFloat[j]));
		}
		break;
	    }
	    case DBF_ENUM:
	    {
		unsigned short *pUShort = (unsigned short *) pt->recordY->array;
		for (j = 0; j < pt->recordY->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,(float)pUShort[j]);
		}
		break;
	    }
	    case DBF_CHAR:
	    {
		unsigned char *pUChar = (unsigned char *) pt->recordY->array;
		for (j = 0; j < pt->recordY->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,(float)pUChar[j]);
		}
		break;
	    }
	    case DBF_LONG:
	    {
		dbr_long_t *pLong = (dbr_long_t *) pt->recordY->array;
		for (j = 0; j < pt->recordY->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,(float)pLong[j]);
		}
		break;
	    }
	    case DBF_DOUBLE:
	    {
		double *pDouble = (double *) pt->recordY->array;
		for (j = 0; j < pt->recordY->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,
		      SAFEFLOAT(pDouble[j]));
		}
		break;
	    }
	    }
	} else  if (pr == pt->recordX) {
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,
		  SAFEFLOAT(pt->recordX->value));
	    }
	}
	break;

    case CP_XYVector: {
	int dox;
	int count;

	count = MIN(pt->recordX->elementCount, pt->recordY->elementCount);
	CpDataSetLastPoint(pt->hcp,pt->trace,MAX(count-1,0));

	if (pr == pt->recordX) {
	    dox = 1;
	} else  if (pr == pt->recordY) {
	    dox = 0;
	} else {
	  /* don't do anything */
	    break;
	}
	if (pcp->timeScale && pr == pt->recordX) {
	    if (pcp->startTime.nsec) {
	    } else {
		pcp->startTime.secPastEpoch = (int) pr->value;
		pcp->startTime.nsec = 1;
		XtVaSetValues(pcp->dlElement->widget,
		  XtNxrtTimeBase, timeOffset + pcp->startTime.secPastEpoch,
		  NULL);
#if DEBUG_TIME		
		printf("pcp->startTime = %d\n",pcp->startTime.secPastEpoch);
#endif		
	    }
	    switch(pr->dataType) {
	    case DBF_LONG:
	    {
		dbr_long_t *pLong = (dbr_long_t *) pr->array;
		dbr_long_t offset = (dbr_long_t) pcp->startTime.secPastEpoch;
		for (j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,(float)(pLong[j] - offset));
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,(float)(pLong[j] - offset));
		}
		break;
	    }
	    case DBF_DOUBLE:
	    {
		double *pDouble = (double *) pr->array;
		double offset = (double) pcp->startTime.secPastEpoch;
		for (j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,SAFEFLOAT(pDouble[j] - offset));
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,SAFEFLOAT(pDouble[j] - offset));
		}
		break;
	    }
	    default :
		break;
	    }
	} else {
	    switch(pr->dataType) {
	    case DBF_STRING:
		for (j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,0.0);
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,0.0);
		}
		break;
	    case DBF_INT:
	    {
		short *pShort = (short *) pr->array;
		for (j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,(float)pShort[j]);
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,(float)pShort[j]);
		}
		break;
	    }
	    case DBF_FLOAT:
	    {
		float *pFloat = (float *) pr->array;
		for (j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,
			SAFEFLOAT(pFloat[j]));
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,
			SAFEFLOAT(pFloat[j]));
		}
		break;
	    }
	    case DBF_ENUM:
	    {
		unsigned short *pUShort = (unsigned short *) pr->array;
		for (j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,(float)pUShort[j]);
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,(float)pUShort[j]);
		}
		break;
	    }
	    case DBF_CHAR:
	    {
		unsigned char *pUChar = (unsigned char *) pr->array;
		for (j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,(float)pUChar[j]);
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,(float)pUChar[j]);
		}
		break;
	    }
	    case DBF_LONG:
	    {
		dbr_long_t *pLong = (dbr_long_t *) pr->array;
		for (j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,(float)pLong[j]);
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,(float)pLong[j]);
		}
		break;
	    }
	    case DBF_DOUBLE:
	    {
		double *pDouble = (double *) pr->array;
		for (j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,
			SAFEFLOAT(pDouble[j]));
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,
			SAFEFLOAT(pDouble[j]));
		}
		break;
	    }
	    }
	}
	break;
    }
    default:
	medmPrintf(1,"\ncartesianPlotUpdateTrace: Unknown dataType\n");
    }
}

void cartesianPlotUpdateScreenFirstTime(XtPointer cd) {
    Record *pr = (Record *) cd;
    XYTrace *pt = (XYTrace *) pr->clientData;
    CartesianPlot *pcp = pt->cartesianPlot;
    Widget widget = pcp->dlElement->widget;
    int i;
    Arg args[20];
    Boolean clearDataSet1 = True;
    Boolean clearDataSet2 = True;

#if DEBUG_CARTESIAN_PLOT_UPDATE
    printf("cartesianPlotUpdateScreenFirstTime: nTraces=%d\n",
      pcp->nTraces);
#endif    
  /* Return until all channels get their graphical information */
    for (i = 0; i < pcp->nTraces; i++) {
	XYTrace *t = &(pcp->xyTrace[i]);
#if DEBUG_CARTESIAN_PLOT_UPDATE
	printf("  recordX=%x recordY=%x hcp=%x\n",
	  t->recordX,t->recordY,t->hcp);
	if(t->recordX) {
	    printf("  x: array=%x precision=%d\n",
	      t->recordX->array,t->recordX->precision);
	}
	if(t->recordY) {
	    printf("  y: array=%x precision=%d\n",
	      t->recordY->array,t->recordY->precision);
	}
#endif    
	if ((t->recordX) || (t->recordY)) {
	    if (t->hcp == NULL) return;
	    if ((t->recordX) &&
	      ((!t->recordX->array) || (t->recordX->precision < 0))) return;
	    if ((t->recordY) &&
	      ((!t->recordY->array) || (t->recordY->precision < 0))) return;
	}
    }
    if (pcp->triggerCh.recordX) {
	if (pcp->triggerCh.recordX->precision < 0) return;
    }
    if (pcp->eraseCh.recordX) {
	if (pcp->eraseCh.recordX->precision < 0) return;
    }

  /* Draw all traces once */
    for (i = 0; i < pcp->nTraces; i++) {
	XYTrace *t = &(pcp->xyTrace[i]);
	if (t->recordX && t->recordY) {
	    cartesianPlotUpdateTrace((XtPointer)t->recordX);
	    cartesianPlotUpdateTrace((XtPointer)t->recordY);
	} else if (t->recordX) {
	    cartesianPlotUpdateTrace((XtPointer)t->recordX);
	} else if (t->recordY) {
	    cartesianPlotUpdateTrace((XtPointer)t->recordY);
	} else {
	    continue;
	}
	if (t->hcp == pcp->hcp1)
	  XtVaSetValues(widget,XtNxrtData,t->hcp,NULL);
	else if (t->hcp == pcp->hcp2) {
	    XtVaSetValues(widget,XtNxrtData2,t->hcp,NULL);
	}
    }
  /* Erase the plot */
    if (pcp->eraseCh.recordX) {
	Record *pr = pcp->eraseCh.recordX;
	if (((pr->value == 0.0) && (pcp->eraseMode == ERASE_IF_ZERO))
	  || ((pr->value != 0.0) && (pcp->eraseMode == ERASE_IF_NOT_ZERO))) {
	    for (i = 0; i < pcp->nTraces; i++) {
		XYTrace *t = &(pcp->xyTrace[i]);
		if ((t->recordX) || (t->recordY)) {
		    int n = CpDataGetLastPoint(t->hcp,t->trace);
		    if (n > 0) {
			if ((t->hcp == pcp->hcp1) && (clearDataSet1)) {
			    XtVaSetValues(widget,XtNxrtData,hcpNullData,NULL);
			    clearDataSet1 = False;
			} else if ((t->hcp == pcp->hcp2) && (clearDataSet2)) {
			    XtVaSetValues(widget,XtNxrtData2,hcpNullData,NULL);
			    clearDataSet2 = False;
			}
			CpDataSetLastPoint(t->hcp,t->trace,0);
		    }
		} 
	    }
	}
    }

  /* Switch to the regular update routine from now on */
    for (i = 0; i < pcp->nTraces; i++) {
	XYTrace *t = &(pcp->xyTrace[i]);
	if (t->recordX) {
	    medmRecordAddUpdateValueCb(t->recordX,cartesianPlotUpdateValueCb);
	}
	if (t->recordY) {
	    medmRecordAddUpdateValueCb(t->recordY,cartesianPlotUpdateValueCb);
	}
    }
    if (pcp->eraseCh.recordX) {
	medmRecordAddUpdateValueCb(pcp->eraseCh.recordX,cartesianPlotUpdateValueCb);
    }
    if (pcp->triggerCh.recordX) {
	medmRecordAddUpdateValueCb(pcp->triggerCh.recordX,cartesianPlotUpdateValueCb);
    }
    updateTaskMarkUpdate(pcp->updateTask);
}


void cartesianPlotUpdateValueCb(XtPointer cd) {
    Record *pr = (Record *) cd;
    XYTrace *pt = (XYTrace *) pr->clientData;
    CartesianPlot *pcp = pt->cartesianPlot;
    Widget widget = pcp->dlElement->widget;
    int i;
    Arg args[20];

#if DEBUG_CARTESIAN_PLOT_UPDATE
    printf("cartesianPlotUpdateValueCb:\n");
#endif    
  /* If this is an erase channel, erase screen */
    if (pr == pcp->eraseCh.recordX) {
	Boolean clearDataSet1 = True;
	Boolean clearDataSet2 = True;

      /* not the right value, return */
	if (((pr->value == 0) && (pcp->eraseMode == ERASE_IF_NOT_ZERO))
	  ||((pr->value != 0) && (pcp->eraseMode == ERASE_IF_ZERO))) {
	    return;
	}

      /* erase */
	for (i = 0; i < pcp->nTraces; i++) {
	    XYTrace *t = &(pcp->xyTrace[i]);
	    if ((t->recordX) || (t->recordY)) {
		int n = CpDataGetLastPoint(t->hcp,t->trace);
		if (n > 0) {
		    if ((t->hcp == pcp->hcp1) && (clearDataSet1)) {
			XtVaSetValues(widget,XtNxrtData,hcpNullData,NULL);
			clearDataSet1 = False;
			pcp->dirty1 = False;
		    } else if ((t->hcp == pcp->hcp2) && (clearDataSet2)) {
			XtVaSetValues(widget,XtNxrtData2,hcpNullData,NULL);
			clearDataSet2 = False;
			pcp->dirty2 = False;
		    }
		    CpDataSetLastPoint(t->hcp,t->trace,0);
		}
	    }
	}
	updateTaskMarkUpdate(pcp->updateTask);
	return;
    }

  /* If there is a trigger channel, but this is not the one, return
       and do not update */
    if (pcp->triggerCh.recordX) {
	if (pr != pcp->triggerCh.recordX)
	  return;
    }

  /* Check if there is a trigger channel */
    if (pcp->triggerCh.recordX) {
  /* There is a trigger channel, update appropriate plots */
	for (i = 0; i < pcp->nTraces; i++) {
	    XYTrace *t = &(pcp->xyTrace[i]);
	    if ((t->recordX == NULL) && (t->recordY == NULL)) continue;
	    if ((t->recordX) && (t->hcp)) {
		cartesianPlotUpdateTrace((XtPointer)t->recordX);
		if (t->type == CP_XYVector)
		  cartesianPlotUpdateTrace((XtPointer)t->recordY);
		if (t->hcp == pcp->hcp1)
		  pcp->dirty1 = True;
		else if (t->hcp == pcp->hcp2)
		  pcp->dirty2 = True;
	    } else if ((t->recordY) && (t->hcp)) {
		cartesianPlotUpdateTrace((XtPointer)t->recordY);
		if (t->hcp == pcp->hcp1)
		  pcp->dirty1 = True;
		else if (t->hcp == pcp->hcp2)
		  pcp->dirty2 = True;
	    }
	}
    } else {
      /* No trigger channel, proceed as normal */
	cartesianPlotUpdateTrace((XtPointer)pr);
	if (pt->hcp == pcp->hcp1) {
	    pcp->dirty1 = True;
	} else if (pt->hcp == pcp->hcp2) {
	    pcp->dirty2 = True;
	} else {
	    medmPrintf(1,"\ncartesianPlotUpdateValueCb: Illegal cpDataSet specified\n");
	}
    }
    updateTaskMarkUpdate(pcp->updateTask);
}

void cartesianPlotDestroyCb(XtPointer cd) {
    CartesianPlot *pcp = (CartesianPlot *) cd;
    if (executeTimeCartesianPlotWidget == pcp->dlElement->widget) {
	executeTimeCartesianPlotWidget = NULL;
	XtSetSensitive(cartesianPlotAxisS,False);
    }
    if (pcp) {
	int i;
	for (i = 0; i < pcp->nTraces; i++) {
	    Record *pr;
	    if (pr = pcp->xyTrace[i].recordX) {
		medmDestroyRecord(pr);
	    }
	    if (pr = pcp->xyTrace[i].recordY) {
		medmDestroyRecord(pr);
	    }
	}
	if (pcp->triggerCh.recordX)
	  medmDestroyRecord(pcp->triggerCh.recordX);
	if (pcp->eraseCh.recordX)
	  medmDestroyRecord(pcp->eraseCh.recordX);
	if (pcp->hcp1) CpDataDestroy(pcp->hcp1);
	if (pcp->hcp2) CpDataDestroy(pcp->hcp2);
	free((char *)pcp);
    }
    return;
}

void cartesianPlotDraw(XtPointer cd) {
    CartesianPlot *pcp = (CartesianPlot *) cd;
    Widget widget = pcp->dlElement->widget;
    int i;
    Boolean connected = True;
    Boolean readAccess = True;

  /* Check for connection */
    for (i = 0; i < pcp->nTraces; i++) {
	Record *pr;
	if (pr = pcp->xyTrace[i].recordX) {
	    if (!pr->connected) {
		connected = False;
		break;
	    } else if (!pr->readAccess)
	      readAccess = False;
	}
	if (pr = pcp->xyTrace[i].recordY) {
	    if (!pr->connected) {
		connected = False;
		break;
	    } else if (!pr->readAccess)
	      readAccess = False;
	}
    }
    if (pcp->triggerCh.recordX) {
	Record *pr = pcp->triggerCh.recordX;
	if (!pr->connected) {
	    connected = False;
	} else if (!pr->readAccess) {
	    readAccess = False;
	}
    }
    if (pcp->eraseCh.recordX) {
	Record *pr = pcp->eraseCh.recordX;
	if (!pr->connected) {
	    connected = False;
	} else if (!pr->readAccess) {
	    readAccess = False;
	}
    }
    if (connected) {
	if (readAccess) {
	    if (widget) {
		if (pcp->dirty1) {
		    pcp->dirty1 = False;
		    XtVaSetValues(widget,XtNxrtData,pcp->hcp1,NULL);
		}
		if (pcp->dirty2) {
		    pcp->dirty2 = False;
		    XtVaSetValues(widget,XtNxrtData2,pcp->hcp2,NULL);
		}
		addCommonHandlers(widget, pcp->updateTask->displayInfo);
		XtManageChild(widget);
	    }
	} else {
	    if (widget) {
		XtUnmanageChild(widget);
	    }
	    draw3DPane(pcp->updateTask,
	      pcp->updateTask->displayInfo->colormap[
		pcp->dlElement->structure.cartesianPlot->plotcom.bclr]);
	    draw3DQuestionMark(pcp->updateTask);
	}
    } else {
	if (widget) {
	    XtUnmanageChild(widget);
	}
	drawWhiteRectangle(pcp->updateTask);
    }
}

static void cartesianPlotGetRecord(XtPointer cd, Record **record, int *count)
{
    CartesianPlot *pcp = (CartesianPlot *) cd;
    int i, j;

    j = 0;
    for (i = 0; i < pcp->nTraces; i++) {
	XYTrace *pt = &(pcp->xyTrace[i]);
	record[j++] = pt->recordX;
	record[j++] = pt->recordY;
    }
    if (pcp->triggerCh.recordX) {
	record[j++] = pcp->triggerCh.recordX;
    } else {     /* Count these even if NULL */
	record[j++] = NULL;
    }
    if (pcp->eraseCh.recordX) {
	record[j++] = pcp->eraseCh.recordX;
    } else {     /* Count these even if NULL */
	record[j++] = NULL;
    }

  /* 200 means two columns */
    *count = j + 200;
}

DlElement *createDlCartesianPlot(DlElement *p)
{
    DlCartesianPlot *dlCartesianPlot;
    DlElement *dlElement;
    int traceNumber;

    dlCartesianPlot = (DlCartesianPlot *) malloc(sizeof(DlCartesianPlot));
    if (!dlCartesianPlot) return 0;
    if (p) {
	*dlCartesianPlot = *p->structure.cartesianPlot;
    } else {
	objectAttributeInit(&(dlCartesianPlot->object));
	plotcomAttributeInit(&(dlCartesianPlot->plotcom));
	dlCartesianPlot->count = 1;
	dlCartesianPlot->style = POINT_PLOT;
	dlCartesianPlot->erase_oldest = ERASE_OLDEST_OFF;
	for (traceNumber = 0; traceNumber < MAX_TRACES; traceNumber++)
	  traceAttributeInit(&(dlCartesianPlot->trace[traceNumber]));
	plotAxisDefinitionInit(&(dlCartesianPlot->axis[X_AXIS_ELEMENT]));
	plotAxisDefinitionInit(&(dlCartesianPlot->axis[Y1_AXIS_ELEMENT]));
	plotAxisDefinitionInit(&(dlCartesianPlot->axis[Y2_AXIS_ELEMENT]));
	dlCartesianPlot->trigger[0] = '\0';
	dlCartesianPlot->erase[0] = '\0';
	dlCartesianPlot->eraseMode = ERASE_IF_NOT_ZERO;
    }

    if (!(dlElement = createDlElement(DL_CartesianPlot,
      (XtPointer)      dlCartesianPlot,
      &cartesianPlotDlDispatchTable))) {
	free(dlCartesianPlot);
    }

    return(dlElement);
}

DlElement *parseCartesianPlot(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlCartesianPlot *dlCartesianPlot;
    DlElement *dlElement = createDlCartesianPlot(NULL);
    int traceNumber;

    if (!dlElement) return 0;
    dlCartesianPlot = dlElement->structure.cartesianPlot;
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlCartesianPlot->object));
	    else if (!strcmp(token,"plotcom"))
	      parsePlotcom(displayInfo,&(dlCartesianPlot->plotcom));
	    else if (!strcmp(token,"count")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlCartesianPlot->count= atoi(token);
	    } else if (!strcmp(token,"style")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"point plot")) 
		  dlCartesianPlot->style = POINT_PLOT;
		else if (!strcmp(token,"point")) 
		  dlCartesianPlot->style = POINT_PLOT;
		else if (!strcmp(token,"line plot")) 
		  dlCartesianPlot->style = LINE_PLOT;
		else if (!strcmp(token,"line")) 
		  dlCartesianPlot->style = LINE_PLOT;
		else if (!strcmp(token,"fill under")) 
		  dlCartesianPlot->style = FILL_UNDER_PLOT;
		else if (!strcmp(token,"fill-under")) 
		  dlCartesianPlot->style = FILL_UNDER_PLOT;
	    } else if (!strcmp(token,"erase_oldest")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"on")) 
		  dlCartesianPlot->erase_oldest = ERASE_OLDEST_ON;
		else if (!strcmp(token,"off")) 
		  dlCartesianPlot->erase_oldest = ERASE_OLDEST_OFF;
		else if (!strcmp(token,"plot last n pts")) 
		  dlCartesianPlot->erase_oldest = ERASE_OLDEST_ON;
		else if (!strcmp(token,"plot n pts & stop")) 
		  dlCartesianPlot->erase_oldest = ERASE_OLDEST_OFF;
	    } else if (!strncmp(token,"trace",5)) {
		traceNumber = MIN(token[6] - '0', MAX_TRACES - 1);
		parseTrace(displayInfo,
		  &(dlCartesianPlot->trace[traceNumber]));
	    } else if (!strcmp(token,"x_axis")) {
		parsePlotAxisDefinition(displayInfo,
		  &(dlCartesianPlot->axis[X_AXIS_ELEMENT]));
	    } else if (!strcmp(token,"y1_axis")) {
		parsePlotAxisDefinition(displayInfo,
		  &(dlCartesianPlot->axis[Y1_AXIS_ELEMENT]));
	    } else if (!strcmp(token,"y2_axis")) {
		parsePlotAxisDefinition(displayInfo,
		  &(dlCartesianPlot->axis[Y2_AXIS_ELEMENT]));
	    } else if (!strcmp(token,"trigger")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlCartesianPlot->trigger,token);
	    } else if (!strcmp(token,"erase")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlCartesianPlot->erase,token);
	    } else if (!strcmp(token,"eraseMode")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"if not zero"))
		  dlCartesianPlot->eraseMode = ERASE_IF_NOT_ZERO;
		else if (!strcmp(token,"if zero"))
		  dlCartesianPlot->eraseMode = ERASE_IF_ZERO;
	    }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
	}
    } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );

    return dlElement;

}

void writeDlCartesianPlot(FILE *stream, DlElement *dlElement, int level)
{
    int i;
    char indent[16];
    DlCartesianPlot *dlCartesianPlot = dlElement->structure.cartesianPlot;

    for (i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%s\"cartesian plot\" {",indent);
    writeDlObject(stream,&(dlCartesianPlot->object),level+1);
    writeDlPlotcom(stream,&(dlCartesianPlot->plotcom),level+1);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
	if (dlCartesianPlot->style != POINT_PLOT)
	  fprintf(stream,"\n%s\tstyle=\"%s\"",indent,
	    stringValueTable[dlCartesianPlot->style]);
	if (dlCartesianPlot->erase_oldest != ERASE_OLDEST_OFF)
	  fprintf(stream,"\n%s\terase_oldest=\"%s\"",indent,
	    stringValueTable[dlCartesianPlot->erase_oldest]);
	if (dlCartesianPlot->count != 1)
	  fprintf(stream,"\n%s\tcount=\"%d\"",indent,dlCartesianPlot->count);
	for (i = 0; i < MAX_TRACES; i++) {
	    writeDlTrace(stream,&(dlCartesianPlot->trace[i]),i,level+1);
	}
	writeDlPlotAxisDefinition(stream,&(dlCartesianPlot->axis[X_AXIS_ELEMENT]),
	  X_AXIS_ELEMENT,level+1);
	writeDlPlotAxisDefinition(stream,&(dlCartesianPlot->axis[Y1_AXIS_ELEMENT]),
	  Y1_AXIS_ELEMENT,level+1);
	writeDlPlotAxisDefinition(stream,&(dlCartesianPlot->axis[Y2_AXIS_ELEMENT]),
	  Y2_AXIS_ELEMENT,level+1);
	if (dlCartesianPlot->trigger[0] != '\0')
	  fprintf(stream,"\n%s\ttrigger=\"%s\"",indent,dlCartesianPlot->trigger);
	if (dlCartesianPlot->erase[0] != '\0')
	  fprintf(stream,"\n%s\terase=\"%s\"",indent,dlCartesianPlot->erase);
	if (dlCartesianPlot->eraseMode != ERASE_IF_NOT_ZERO)
	  fprintf(stream,"\n%s\teraseMode=\"%s\"",indent,
	    stringValueTable[dlCartesianPlot->eraseMode]);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%s\tstyle=\"%s\"",indent,
	  stringValueTable[dlCartesianPlot->style]);
	fprintf(stream,"\n%s\terase_oldest=\"%s\"",indent,
	  stringValueTable[dlCartesianPlot->erase_oldest]);
	fprintf(stream,"\n%s\tcount=\"%d\"",indent,dlCartesianPlot->count);
	for (i = 0; i < MAX_TRACES; i++) {
	    writeDlTrace(stream,&(dlCartesianPlot->trace[i]),i,level+1);
	}
	writeDlPlotAxisDefinition(stream,&(dlCartesianPlot->axis[X_AXIS_ELEMENT]),
	  X_AXIS_ELEMENT,level+1);
	writeDlPlotAxisDefinition(stream,&(dlCartesianPlot->axis[Y1_AXIS_ELEMENT]),
	  Y1_AXIS_ELEMENT,level+1);
	writeDlPlotAxisDefinition(stream,&(dlCartesianPlot->axis[Y2_AXIS_ELEMENT]),
	  Y2_AXIS_ELEMENT,level+1);
	fprintf(stream,"\n%s\ttrigger=\"%s\"",indent,dlCartesianPlot->trigger);
	fprintf(stream,"\n%s\terase=\"%s\"",indent,dlCartesianPlot->erase);
	fprintf(stream,"\n%s\teraseMode=\"%s\"",indent,
	  stringValueTable[dlCartesianPlot->eraseMode]);
    }
#endif
    fprintf(stream,"\n%s}",indent);
}

static void cartesianPlotInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlCartesianPlot *dlCartesianPlot = p->structure.cartesianPlot;
    medmGetValues(pRCB,
      CLR_RC,        &(dlCartesianPlot->plotcom.clr),
      BCLR_RC,       &(dlCartesianPlot->plotcom.bclr),
      -1);
}

static void cartesianPlotGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlCartesianPlot *dlCartesianPlot = p->structure.cartesianPlot;
    medmGetValues(pRCB,
      X_RC,          &(dlCartesianPlot->object.x),
      Y_RC,          &(dlCartesianPlot->object.y),
      WIDTH_RC,      &(dlCartesianPlot->object.width),
      HEIGHT_RC,     &(dlCartesianPlot->object.height),
      TITLE_RC,      &(dlCartesianPlot->plotcom.title),
      XLABEL_RC,     &(dlCartesianPlot->plotcom.xlabel),
      YLABEL_RC,     &(dlCartesianPlot->plotcom.ylabel),
      CLR_RC,        &(dlCartesianPlot->plotcom.clr),
      BCLR_RC,       &(dlCartesianPlot->plotcom.bclr),
      COUNT_RC,      &(dlCartesianPlot->count),
      CSTYLE_RC,     &(dlCartesianPlot->style),
      ERASE_OLDEST_RC,&(dlCartesianPlot->erase_oldest),
      CPDATA_RC,     &(dlCartesianPlot->trace),
      CPAXIS_RC,     &(dlCartesianPlot->axis),
      TRIGGER_RC,    &(dlCartesianPlot->trigger),
      ERASE_RC,      &(dlCartesianPlot->erase),
      ERASE_MODE_RC, &(dlCartesianPlot->eraseMode),
      -1);
}

static void cartesianPlotSetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlCartesianPlot *dlCartesianPlot = p->structure.cartesianPlot;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlCartesianPlot->plotcom.bclr),
      -1);
}

static void cartesianPlotSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlCartesianPlot *dlCartesianPlot = p->structure.cartesianPlot;
    medmGetValues(pRCB,
      CLR_RC,        &(dlCartesianPlot->plotcom.clr),
      -1);
}

#if XRT_VERSION > 2
#ifdef XRT_EXTENSIONS
#ifdef __cplusplus
static void destroyXrtPropertyEditor(Widget w, XtPointer, XtPointer)
#else
static void destroyXrtPropertyEditor(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
  /* False means do not destroy the dialog */
    XrtPopdownPropertyEditor(w,False);
}
#endif			    
#endif    

/*
 * Set Cartesian Plot Axis attributes
 * (complex - has to handle both EDIT and EXECUTE time interactions)
 */
#ifdef __cplusplus
static void cpAxisOptionMenuSimpleCallback(Widget w, XtPointer cd, XtPointer)
#else
static void cpAxisOptionMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{     
    DisplayInfo *cdi=currentDisplayInfo;
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

  /* Get current cartesian plot */
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
    case CP_Y2_AXIS_STYLE:
    {
	CartesianPlotAxisStyle style 
	  = (CartesianPlotAxisStyle)(FIRST_CARTESIAN_PLOT_AXIS_STYLE+buttonId);
	
	globalResourceBundle.axis[rcType - CP_X_AXIS_STYLE].axisStyle = style;
	switch (rcType) {
	case CP_X_AXIS_STYLE:
	    switch (style) {
	    case LINEAR_AXIS:
		XtSetArg(args[n],XtNxrtXAnnotationMethod,XRT_ANNO_VALUES); n++;
		XtSetArg(args[n],XtNxrtXAxisLogarithmic,False); n++;
		XtSetSensitive(axisTimeFormat,False);
		if(pcp) pcp->timeScale = False;
		break;
	    case LOG10_AXIS:
		XtSetArg(args[n],XtNxrtXAnnotationMethod,XRT_ANNO_VALUES); n++;
		XtSetArg(args[n],XtNxrtXAxisLogarithmic,True); n++;
		XtSetSensitive(axisTimeFormat,False);
		if(pcp) pcp->timeScale = False;
		break;
	    case TIME_AXIS:
		XtSetSensitive(axisTimeFormat,True);
		XtSetArg(args[n],XtNxrtXAxisLogarithmic,False); n++;
		XtSetArg(args[n],XtNxrtXAnnotationMethod,XRT_ANNO_TIME_LABELS); n++;
		XtSetArg(args[n],XtNxrtTimeBase,time900101); n++;
		XtSetArg(args[n],XtNxrtTimeUnit,XRT_TMUNIT_SECONDS); n++;
		XtSetArg(args[n],XtNxrtTimeFormatUseDefault,False); n++;
		XtSetArg(args[n],XtNxrtTimeFormat,
		  timeFormatString[(int)globalResourceBundle.axis[0].timeFormat -
		      FIRST_CP_TIME_FORMAT]); n++;
		if(pcp) pcp->timeScale = True;
	    }
	    break;
	case CP_Y_AXIS_STYLE:
	    XtSetArg(args[n],XtNxrtYAxisLogarithmic,
	      (style == LOG10_AXIS) ? True : False); n++;
	    break;
	case CP_Y2_AXIS_STYLE:
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
	  = (CartesianPlotRangeStyle)(FIRST_CARTESIAN_PLOT_RANGE_STYLE
	    + buttonId);
	switch(globalResourceBundle.axis[rcType%3].rangeStyle) {
	case USER_SPECIFIED_RANGE:
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
	case CHANNEL_RANGE: 
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
	case AUTO_SCALE_RANGE:
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
	default:
	    break;
	}
	break;
    case CP_X_TIME_FORMAT:
	globalResourceBundle.axis[0].timeFormat =
	  (CartesianPlotTimeFormat_t)(FIRST_CP_TIME_FORMAT + buttonId);
	XtSetArg(args[n],XtNxrtTimeFormat,
	  timeFormatString[(int)globalResourceBundle.axis[0].timeFormat -
	    FIRST_CP_TIME_FORMAT]); n++;
	break;
    default:
	medmPrintf(1,"\ncpAxisOptionMenuSimpleCallback: Unknown rcType = %d\n",rcType/3);
	break;
    }
    
  /* Update for EDIT or EXECUTE mode */
    switch(globalDisplayListTraversalMode) {
    case DL_EDIT:
	if (cdi) {
	    DlElement *dlElement = FirstDlElement(
	      cdi->selectedDlElementList);
	    unhighlightSelectedElements();
	    while (dlElement) {
		updateElementFromGlobalResourceBundle(dlElement->structure.element);
		dlElement = dlElement->next;
	    }
	    dmTraverseNonWidgetsInDisplayList(cdi);
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
void cpAxisTextFieldActivateCallback(Widget w, XtPointer cd, XtPointer)
#else
void cpAxisTextFieldActivateCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    DisplayInfo *cdi=currentDisplayInfo;
    int rcType = (int)cd;
    char *stringValue, string[24];
    int i, k, n, iPrec;
    Arg args[10];
    XcVType valF, minF, maxF, tickF;
    String resourceName;

#if DEBUG_XRT
    print("\ncpAxisTextFieldActivateCallback: Entered\n");
#endif    
    stringValue = XmTextFieldGetString(w);

  /* Determine axis max or min
   *   Note: For the strcpy() calls, note that the textField has a maxLength 
	  resource set such that the strcpy always succeeds) */
    n = 0;
    switch(rcType) {
    case CP_X_RANGE_MIN:
    case CP_Y_RANGE_MIN:
    case CP_Y2_RANGE_MIN:
	globalResourceBundle.axis[rcType%3].minRange= atof(stringValue);
	if (globalDisplayListTraversalMode == DL_EXECUTE) {
	    valF.fval = globalResourceBundle.axis[rcType%3].minRange;
	    switch(rcType%3) {
	    case X_AXIS_ELEMENT: resourceName = XtNxrtXMin; break;
	    case Y1_AXIS_ELEMENT: resourceName = XtNxrtYMin; break;
	    case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2Min; break;
	    default:
		medmPrintf(1,"\ncpAxisTextFieldActivateCallback (MIN): "
		  "Unknown rcType%%3 = %d\n",rcType%3);
		return;
	    }
	    XtSetArg(args[n],resourceName,valF.lval); n++;
#if DEBUG_XRT
	    print("\ncpAxisTextFieldActivateCallback [MIN]: "
	      "valF.fval =%g valF.lval=%ld Converted: %d\n",
	      valF.fval,valF.lval,XrtFloatToArgVal(valF.fval));
#endif    
	}
	break;
    case CP_X_RANGE_MAX:
    case CP_Y_RANGE_MAX:
    case CP_Y2_RANGE_MAX:
	globalResourceBundle.axis[rcType%3].maxRange= atof(stringValue);
	if (globalDisplayListTraversalMode == DL_EXECUTE) {
	    valF.fval = globalResourceBundle.axis[rcType%3].maxRange;
	    switch(rcType%3) {
	    case X_AXIS_ELEMENT: resourceName = XtNxrtXMax; break;
	    case Y1_AXIS_ELEMENT: resourceName = XtNxrtYMax; break;
	    case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2Max; break;
	    default:
		medmPrintf(1,"\ncpAxisTextFieldActivateCallback (MAX): "
		  "Unknown rcType%%3 = %d\n",rcType%3);
		return;
	    }
	    XtSetArg(args[n],resourceName,valF.lval); n++;
#if DEBUG_XRT
	    print("\ncpAxisTextFieldActivateCallback [MAX]: "
	      "valF.fval =%g valF.lval=%ld Converted: %d\n",
	      valF.fval,valF.lval,XrtFloatToArgVal(valF.fval));
#endif    
	}
	break;
    default:
	medmPrintf(1,"\ncpAxisTextFieldActivateCallback: "
	  "Unknown rcType = %d\n",rcType);
	return;
    }
    XtFree(stringValue);

  /* Recalculate ticks */
    minF.fval = globalResourceBundle.axis[rcType%3].minRange;
    maxF.fval = globalResourceBundle.axis[rcType%3].maxRange;
    tickF.fval = (maxF.fval - minF.fval)/4.0;
#if DEBUG_XRT
    print("cpAxisTextFieldActivateCallback: "
      "minF.fval =%g minF.lval=%ld Converted: %d\n",
      minF.fval,minF.lval,XrtFloatToArgVal(minF.fval));
    print("cpAxisTextFieldActivateCallback: "
      "maxF.fval =%g maxF.lval=%ld Converted: %d\n",
      maxF.fval,maxF.lval,XrtFloatToArgVal(maxF.fval));
    print("cpAxisTextFieldActivateCallback: "
      "tickF.fval =%g tickF.lval=%ld Converted: %d\n",
      tickF.fval,tickF.lval,XrtFloatToArgVal(tickF.fval));
#endif    

  /* Increment between axis ticks */
    switch(rcType%3) {
    case X_AXIS_ELEMENT: resourceName = XtNxrtXTick; break;
    case Y1_AXIS_ELEMENT: resourceName = XtNxrtYTick; break;
    case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2Tick; break;
    default:
	medmPrintf(1,"\ncpAxisTextFieldActivateCallback (Tick): "
	  "Unknown rcType%%3 = %d\n",rcType%3);
	return;
    }
    XtSetArg(args[n],resourceName,tickF.lval); n++;

  /* Increment between axis numbering (set to tick value) */
    switch(rcType%3) {
    case X_AXIS_ELEMENT: resourceName = XtNxrtXNum; break;
    case Y1_AXIS_ELEMENT: resourceName = XtNxrtYNum; break;
    case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2Num; break;
    default:
	medmPrintf(1,"\ncpAxisTextFieldActivateCallback (Num): "
	  "Unknown rcType%%3 = %d\n",rcType%3);
	return;
    }

  /* Digits after the decimal point */
    XtSetArg(args[n],resourceName,tickF.lval); n++;
    switch(rcType%3) {
    case X_AXIS_ELEMENT: resourceName = XtNxrtXPrecision; break;
    case Y1_AXIS_ELEMENT: resourceName = XtNxrtYPrecision; break;
    case Y2_AXIS_ELEMENT: resourceName = XtNxrtY2Precision; break;
    default:
	medmPrintf(1,"\ncpAxisTextFieldActivateCallback (Precision): "
	  "Unknown rcType%%3 = %d\n",rcType%3);
	return;
    }
#if 0
  /* KE: Doesn't make sense and is redone below */
    XtSetArg(args[n],resourceName,tickF.lval); n++;
#endif    
    sprintf(string,"%f",tickF.fval);
    k = strlen(string)-1;
    while (string[k] == '0') k--;	/* strip off trailing zeroes */
    iPrec = k;
    while (string[k] != '.' && k >= 0) k--;
    iPrec = iPrec - k;
    XtSetArg(args[n],resourceName,iPrec); n++;
    
  /* Update for EDIT or EXECUTE mode  */
    switch(globalDisplayListTraversalMode) {
    case DL_EDIT:
      /*
       * update elements (this is overkill, but okay for now)
       *	-- not as efficient as it should be (don't update EVERYTHING if only
       *	   one item changed!)
       */
	if (cdi != NULL) {
	    DlElement *dlElement = FirstDlElement(
	      cdi->selectedDlElementList);
	    unhighlightSelectedElements();
	    while (dlElement) {
		updateElementFromGlobalResourceBundle(dlElement->structure.element);
		dlElement = dlElement->next;
	    }
	    dmTraverseNonWidgetsInDisplayList(cdi);
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
void cpAxisTextFieldLosingFocusCallback(Widget w, XtPointer cd, XtPointer)
#else
void cpAxisTextFieldLosingFocusCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
  /* Note: Losing focus happens when cursor leaves cartesianPlotAxisS, too */
{
    int rcType = (int) cd;
    char string[MAX_TOKEN_LENGTH], *currentString;
    int tail;
    XcVType minF[3], maxF[3];

#if DEBUG_XRT
    print("\ncpAxisTextFieldLosingFocusCallback: Entered\n");
    print("  executeTimeCartesianPlotWidget: %d\n",
      executeTimeCartesianPlotWidget);
    print("  rcType: %d  rcType%%3: %d\n",rcType,rcType%3);
    print("  axisRangeMin[rcType%%3]: %d  axisRangeMax[rcType%%3]: %d  "
      "w: %d\n",axisRangeMin[rcType%3],axisRangeMax[rcType%3],w);
#endif    
  /* Losing focus - make sure that the text field remains accurate wrt 
   *   values stored in widget (not necessarily what is in globalResourceBundle) */
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
	medmPostMsg(1,"cpAxisTextFieldLosingFocusCallback: Unknown rcType = %d",
	  rcType/3);
	return;
    }
  /* strip trailing zeroes */
    tail = strlen(string);
    while (string[--tail] == '0') string[tail] = '\0';
    currentString = XmTextFieldGetString(w);
    if (strcmp(string,currentString))
      XmTextFieldSetString(w,string);
    XtFree(currentString);
}

/*
 * Menu entry support routine for the Cartesian Plot Axis Dialog...
 */
void createCartesianPlotAxisDialogMenuEntry(
  Widget parentRC,
  XmString axisLabelXmString,
  Widget *label,
  Widget *menu,
  XmString *menuLabelXmStrings,
  XmButtonType *buttonType,
  int numberOfLabels,
  XtPointer clientData)
{
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

/*
 * Text entry support routine for the Cartesian Plot Axis Dialog...
 */
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
  /* Create a row column widget to hold the label and textfield widget */
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_NONE); n++;
    *rowColumn = XmCreateRowColumn(parentRC,"entryRC",args,n);
 
  /* Create the label */
    n = 0;
    XtSetArg(args[n],XmNalignment,XmALIGNMENT_END); n++;
    XtSetArg(args[n],XmNlabelString,axisLabelXmString); n++;
    XtSetArg(args[n],XmNrecomputeSize,False); n++;
    *label = XmCreateLabel(*rowColumn,"localLabel",args,n);
 
  /* Create the text field */
    n = 0;
    XtSetArg(args[n],XmNmaxLength,MAX_TOKEN_LENGTH-1); n++;
    *text = XmCreateTextField(*rowColumn,"localElement",args,n);
    XtAddCallback(*text,XmNactivateCallback,cpAxisTextFieldActivateCallback,
      clientData);
    XtAddCallback(*text,XmNlosingFocusCallback,cpAxisTextFieldLosingFocusCallback,
      clientData);
    XtAddCallback(*text,XmNmodifyVerifyCallback,textFieldFloatVerifyCallback,
      NULL);
    XtManageChild(*rowColumn);
}
     

/*
 * Create Cartesian Plot axis dialog box
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
  /* For keeping list of widgets around */
    Widget entryLabel[MAX_CP_AXIS_ELEMENTS], entryElement[MAX_CP_AXIS_ELEMENTS];
    Dimension width, height;
    static int maxWidth = 0, maxHeight = 0;

  /* Indexed like dlCartesianPlot->axis[]: X_ELEMENT_AXIS, Y1_ELEMENT_AXIS... */
    static char *frameLabelString[3] = {"X Axis", "Y1 Axis", "Y2 Axis",};

  /* Initialize XmString value tables (since this can be edit or execute time) */
    initializeXmStringValueTables();

  /* Set buttons to be push button */
    for (i = 0; i < MAX_CP_AXIS_BUTTONS; i++) buttonType[i] = XmPUSHBUTTON;

  /*
   * Create the interface
   *		     ...
   *		 OK     CANCEL
   */
    
  /* Shell */
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

  /* Form */
    n = 0;
    XtSetArg(args[n],XmNautoUnmanage,False); n++;
    XtSetArg(args[n],XmNmarginHeight,8); n++;
    XtSetArg(args[n],XmNmarginWidth,8); n++;
    XtSetArg(args[n],XmNdialogStyle,XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
    cpAxisForm = XmCreateForm(shell,"cartesianPlotAxisF",args,n);

  /* RowColumn */
    n = 0;
    XtSetArg(args[n],XmNnumColumns,1); n++;
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_COLUMN); n++;
    entriesRC = XmCreateRowColumn(cpAxisForm,"entriesRC",args,n);

    axisStyleXmString = XmStringCreateLocalized("Axis Style");
    axisRangeXmString = XmStringCreateLocalized("Axis Range");
    axisMinXmString = XmStringCreateLocalized("Minimum Value");
    axisMaxXmString = XmStringCreateLocalized("Maximum Value");
    axisTimeFmtXmString = XmStringCreateLocalized("Time format");

  /* Loop over major elements */
    counter = 0;
    for (i = X_AXIS_ELEMENT /* 0 */; i <= Y2_AXIS_ELEMENT /* 2 */; i++) {
      /* Frame */
	n = 0;
	XtSetArg(args[n],XmNshadowType,XmSHADOW_ETCHED_IN); n++;
	frame = XmCreateFrame(entriesRC,"frame",args,n);
	XtManageChild(frame);

      /* Label */
	n = 0;
	frameLabelXmString = XmStringCreateLocalized(frameLabelString[i]);
	XtSetArg(args[n],XmNlabelString,frameLabelXmString); n++;
	XtSetArg(args[n],XmNmarginWidth,0); n++;
	XtSetArg(args[n],XmNmarginHeight,0); n++;
	XtSetArg(args[n],XmNchildType,XmFRAME_TITLE_CHILD); n++;
      /* (Use font calculation for textField (which uses ~90% of height)) */
	XtSetArg(args[n],XmNfontList,fontListTable[textFieldFontListIndex(24)]);n++;
	localLabel = XmCreateLabel(frame,"label",args,n);
	XtManageChild(localLabel);
	XmStringFree(frameLabelXmString);

      /* RC within frame */
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
	createCartesianPlotAxisDialogMenuEntry(parentRC,
	  axisRangeXmString,
	  &(entryLabel[counter]),
	  &(entryElement[counter]),
	  &(xmStringValueTable[FIRST_CARTESIAN_PLOT_RANGE_STYLE]),
	  buttonType,
	  NUM_CARTESIAN_PLOT_RANGE_STYLES,
	  (XtPointer)(CP_X_RANGE_STYLE+i));
	axisRangeMenu[i] =  entryElement[counter];
	counter++;

      /* Create Min text field entry */
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
	  &(entryElement[counter]), (XtPointer)(CP_X_RANGE_MAX+i));
	axisRangeMax[i] = entryElement[counter];
	counter++;

      /* Create time format menu entry for X axis only */
	if (i == X_AXIS_ELEMENT) {
	    createCartesianPlotAxisDialogMenuEntry(
	      parentRC,
	      axisTimeFmtXmString,
	      &(entryLabel[counter]),
	      &(entryElement[counter]),
	      &(xmStringValueTable[FIRST_CP_TIME_FORMAT]),
	      buttonType,
	      NUM_CP_TIME_FORMAT,
	      (XtPointer)(CP_X_TIME_FORMAT));
	    axisTimeFormat = entryElement[counter];
	    counter++;
	}
    }

  /* Get max sizes and manage */
    for (i = 0; i < counter; i++) {
	XtVaGetValues(entryLabel[i],XmNwidth,&width,XmNheight,&height,NULL);
	maxLabelWidth = MAX(maxLabelWidth,width);
	maxLabelHeight = MAX(maxLabelHeight,height);
	XtVaGetValues(entryElement[i],XmNwidth,&width,XmNheight,&height,NULL);
	maxLabelWidth = MAX(maxLabelWidth,width);
	maxLabelHeight = MAX(maxLabelHeight,height);
#if 0
      /* Is done below */
	XtManageChild(entryLabel[i]);
	XtManageChild(entryElement[i]);
#endif	
    }

   /* Resize the labels and elements (to max width) for uniform appearance */
    for (i = 0; i < counter; i++) {
      /* Set label */
	XtVaSetValues(entryLabel[i],XmNwidth,maxLabelWidth,
	  XmNheight,maxLabelHeight,XmNrecomputeSize,False,
	  XmNalignment,XmALIGNMENT_END,NULL);

      /* Set element */
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

  /* Free strings */
    XmStringFree(axisStyleXmString);
    XmStringFree(axisRangeXmString);
    XmStringFree(axisMinXmString);
    XmStringFree(axisMaxXmString);
    XmStringFree(axisTimeFmtXmString);
    
  /* Set values for entriesRC (After resizing) */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
    XtSetValues(entriesRC,args,n);

  /* Manage the RC */
    XtManageChild(entriesRC);
    
  /* Close button */
    n = 0;
    xmString = XmStringCreateLocalized("Close");
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,entriesRC); n++;
    XtSetArg(args[n],XmNtopOffset,12); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomOffset,12); n++;
  /* HACK - approximate centering button by putting at 43% of form width */
    XtSetArg(args[n],XmNleftPosition,(Position)43); n++;
    closeButton = XmCreatePushButton(cpAxisForm,"closeButton",args,n);
    XtAddCallback(closeButton,XmNactivateCallback,
      cartesianPlotAxisActivate, (XtPointer)CP_CLOSE_BTN);
    XtManageChild(closeButton);
    XmStringFree(xmString);

  /* Return the shell */
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
    int i, tail, buttonId;
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
    char *timeFormat;

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
      XtNxrtTimeFormat,&timeFormat,
      NULL);

    if (pcp = (CartesianPlot *)userData ) {
	xIsCurrentlyFromChannel =
	  pcp->axisRange[X_AXIS_ELEMENT].isCurrentlyFromChannel;
	y1IsCurrentlyFromChannel =
	  pcp->axisRange[Y1_AXIS_ELEMENT].isCurrentlyFromChannel;
	y2IsCurrentlyFromChannel =
	  pcp->axisRange[Y2_AXIS_ELEMENT].isCurrentlyFromChannel;
    }

  /* X Axis */
    if (xAnnoMethod == XRT_ANNO_TIME_LABELS)  {
      /* Style */
	optionMenuSet(axisStyleMenu[X_AXIS_ELEMENT],
	  TIME_AXIS - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
      /* Time format */
	buttonId = 0;     /* Use for  default */
	for(i = 0; i < NUM_CP_TIME_FORMAT; i++) {
	    if(!strcmp(timeFormatString[i],timeFormat)) {
		buttonId = i;
		break;
	    }
	}
	optionMenuSet(axisTimeFormat, buttonId);
	XtSetSensitive(axisTimeFormat, True);
    } else {
      /* Style */
	optionMenuSet(axisStyleMenu[X_AXIS_ELEMENT],
	  (xAxisIsLog ? LOG10_AXIS : LINEAR_AXIS)
	  - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
	XtSetSensitive(axisTimeFormat, False);
    }
  /* Range */
    buttonId = (xIsCurrentlyFromChannel ? CHANNEL_RANGE :
      (xMinUseDef ? AUTO_SCALE_RANGE : USER_SPECIFIED_RANGE)
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
    buttonId = (y1IsCurrentlyFromChannel ? CHANNEL_RANGE :
      (y1MinUseDef ? AUTO_SCALE_RANGE : USER_SPECIFIED_RANGE)
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
      (y2AxisIsLog ? LOG10_AXIS : LINEAR_AXIS)
      - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
    buttonId = (y2IsCurrentlyFromChannel ? CHANNEL_RANGE :
      (y2MinUseDef ? AUTO_SCALE_RANGE : USER_SPECIFIED_RANGE)
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

#ifdef __cplusplus
static void cartesianPlotActivate(Widget, XtPointer cd, XtPointer)
#else
static void cartesianPlotActivate(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    DisplayInfo *cdi=currentDisplayInfo;
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
	if (cdi) {
	    DlElement *dlElement = FirstDlElement(
	      cdi->selectedDlElementList);
	    unhighlightSelectedElements();
	    while (dlElement) {
		if (dlElement->structure.element->type = DL_CartesianPlot)
		  updateElementFromGlobalResourceBundle(dlElement->structure.element);
		dlElement = dlElement->next;
	    }
	}
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	XtPopdown(cartesianPlotS);
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
Widget createCartesianPlotDataDialog(Widget parent)
{
    Widget shell, applyButton, closeButton;
    Dimension cWidth, cHeight, aWidth, aHeight;
    Arg args[12];
    XmString xmString;
    int i, j, n;
    static Boolean first = True;


  /* Initialize file-scoped globals */
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
   * Create the interface
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


    xmString = XmStringCreateLocalized("Cancel");
    n = 0;
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    closeButton = XmCreatePushButton(cpForm,"closeButton",args,n);
    XtAddCallback(closeButton,XmNactivateCallback,
      cartesianPlotActivate,(XtPointer)CP_CLOSE_BTN);
    XtManageChild(closeButton);
    XmStringFree(xmString);

    xmString = XmStringCreateLocalized("Apply");
    n = 0;
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    applyButton = XmCreatePushButton(cpForm,"applyButton",args,n);
    XtAddCallback(applyButton,XmNactivateCallback,
      cartesianPlotActivate,(XtPointer)CP_APPLY_BTN);
    XtManageChild(applyButton);
    XmStringFree(xmString);

  /* Make APPLY and CLOSE buttons same size */
    XtVaGetValues(closeButton,XmNwidth,&cWidth,XmNheight,&cHeight,NULL);
    XtVaGetValues(applyButton,XmNwidth,&aWidth,XmNheight,&aHeight,NULL);
    XtVaSetValues(closeButton,XmNwidth,MAX(cWidth,aWidth),
      XmNheight,MAX(cHeight,aHeight),NULL);

  /* Make the APPLY button the default for the form */
    XtVaSetValues(cpForm,XmNdefaultButton,applyButton,NULL);

  /*
   * Do form layout 
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

#if DEBUG_CARTESIAN_PLOT
#include "medmCartesianPlot.h"
void dumpCartesianPlot(void)
{
    Arg args[20];
    int n=0;
			    
    Dimension footerHeight;
    Dimension footerWidth;
    Dimension footerBorderWidth;
    Dimension borderWidth;
    Dimension shadowThickness;
    Dimension highlightThickness;
    Dimension headerBorderWidth;
    Dimension headerHeight;
    Dimension headerWidth;
    Dimension graphBorderWidth;
    Dimension graphWidth;
    Dimension graphHeight;
    Dimension height;
    Dimension width;
    Dimension legendBorderWidth;
    Dimension legendHeight;
    Dimension legendWidth;
    unsigned char unitType;
    time_t timeBase;


    XtSetArg(args[n],XtNxrtLegendWidth,&legendWidth); n++;
    XtSetArg(args[n],XtNxrtLegendHeight,&legendHeight); n++;
    XtSetArg(args[n],XtNxrtLegendBorderWidth,&legendBorderWidth); n++;
    XtSetArg(args[n],XmNwidth,&width); n++;
    XtSetArg(args[n],XmNheight,&height); n++;
    XtSetArg(args[n],XtNxrtGraphBorderWidth,&graphBorderWidth); n++;
    XtSetArg(args[n],XtNxrtGraphWidth,&graphWidth); n++;
    XtSetArg(args[n],XtNxrtGraphHeight,&graphHeight); n++;
    XtSetArg(args[n],XtNxrtHeaderWidth,&headerWidth); n++;
    XtSetArg(args[n],XtNxrtHeaderHeight,&headerHeight); n++;
    XtSetArg(args[n],XtNxrtHeaderBorderWidth,&headerBorderWidth); n++;
    XtSetArg(args[n],XmNhighlightThickness,&highlightThickness); n++;
    XtSetArg(args[n],XmNshadowThickness,&shadowThickness); n++;
    XtSetArg(args[n],XmNborderWidth,&borderWidth); n++;
    XtSetArg(args[n],XtNxrtFooterBorderWidth,&footerBorderWidth); n++;
    XtSetArg(args[n],XtNxrtFooterHeight,&footerHeight); n++;
    XtSetArg(args[n],XtNxrtFooterWidth,&footerWidth); n++;
    XtSetArg(args[n],XtNxrtFooterBorderWidth,&footerBorderWidth); n++;
    XtSetArg(args[n],XmNunitType,&unitType); n++;
    XtSetArg(args[n],XtNxrtTimeBase,&timeBase); n++;
    XtGetValues(widget,args,n);
			      
    print(,"width: %d\n",width);
    print(,"height: %d\n",height);
    print(,"highlightThickness: %d\n",highlightThickness);
    print(,"shadowThickness: %d\n",shadowThickness);
    print(,"borderWidth: %d\n",borderWidth);
    print(,"graphBorderWidth: %d\n",graphBorderWidth);
    print(,"graphWidth: %d\n",graphWidth);
    print(,"graphHeight: %d\n",graphHeight);
    print(,"headerBorderWidth: %d\n",headerBorderWidth);
    print(,"headerWidth: %d\n",headerWidth);
    print(,"headerHeight: %d\n",headerHeight);
    print(,"footerWidth: %d\n",footerBorderWidth);
    print(,"footerWidth: %d\n",footerWidth);
    print(,"footerHeight: %d\n",footerHeight);
    print(,"legendBorderWidth: %d\n",legendBorderWidth);
    print(,"legendWidth: %d\n",legendWidth);
    print(,"legendHeight: %d\n",legendHeight);
    print(,"unitType: %d (PIXELS %d, MM %d, IN %d, PTS %d, FONT %d)\n",unitType,
      XmPIXELS,Xm100TH_MILLIMETERS,Xm1000TH_INCHES,Xm100TH_POINTS,Xm100TH_FONT_UNITS);
    print(,"timeBase: %d\n",timeBase);
}
#endif

