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

#define CHECK_NAN
#define DEBUG_CARTESIAN_PLOT_BORDER 0
#define DEBUG_CARTESIAN_PLOT_UPDATE 0
#define DEBUG_TIME 0

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
#include "medmCartesianPlot.h"

#include <XrtGraph.h>
#if XRT_VERSION > 2
#ifdef XRT_EXTENSIONS
#include <XrtGraphProp.h>
#endif
#endif

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

#if XRT_VERSION > 2
#ifdef XRT_EXTENSIONS
static void destroyXrtPropertyEditor(Widget w, XtPointer, XtPointer);
#endif
#endif

#if XRT_VERSION < 3
/* Routines to make XRT/graph backward compatible from Version 3.0 */

XrtDataHandle XrtDataCreate(XrtDataType hData, int nsets, int npoints);
int XrtDataGetLastPoint(XrtDataHandle hData, int set);
double XrtDataGetXElement(XrtDataHandle hData, int set, int point);
double XrtDataGetYElement(XrtDataHandle hData, int set, int point);
int XrtDataDestroy(XrtDataHandle hData);
int XrtDataSetHole(XrtDataHandle hData, double hole);
int XrtDataSetLastPoint(XrtDataHandle hData, int set, int npoints);
int XrtDataSetXElement(XrtDataHandle hData, int set, int point, double x);
int XrtDataSetYElement(XrtDataHandle hData, int set, int point, double y);

XrtDataHandle XrtDataCreate(XrtDataType type, int nsets, int npoints) {
    return XrtMakeData(type,nsets,npoints,True);
}

int XrtDataGetLastPoint(XrtDataHandle hData, int set) {
    return (MAX(hData->g.data[set].npoints-1,0));
}

double XrtDataGetXElement(XrtDataHandle hData, int set, int point) {
    return hData->g.data[set].xp[point];
}

double XrtDataGetYElement(XrtDataHandle hData, int set, int point) {
    return hData->g.data[set].yp[point];
}

int XrtDataDestroy(XrtDataHandle hData) {
    if(hData) XrtDestroyData(hData,True);
    return 1;
}

int XrtDataSetHole(XrtDataHandle hData, double hole) {
    hData->g.hole = hole;
    return 1;
}

int XrtDataSetLastPoint(XrtDataHandle hData, int set, int point) {
    hData->g.data[0].npoints = point+1;
    return 1;
}

int XrtDataSetXElement(XrtDataHandle hData, int set, int point, double x) {
    hData->g.data[set].xp[point] = x;
    return 1;
}

int XrtDataSetYElement(XrtDataHandle hData, int set, int point, double y) {
    hData->g.data[set].yp[point] = y;
    return 1;
}
#endif

static XrtDataHandle hxrtNullData = (XrtDataHandle)0;

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

  /* Initialize hxrtNullData if not done */
    if(!hxrtNullData) {
	hxrtNullData = XrtDataCreate(XRT_GENERAL,1,1);
	XrtDataSetHole(hxrtNullData,0.0);
	XrtDataSetLastPoint(hxrtNullData,0,0);
	XrtDataSetXElement(hxrtNullData,0,0,0.0);
	XrtDataSetYElement(hxrtNullData,0,0,0.0);
    }

  /* Allocate a CartesianPlot and fill part of it in */
    pcp = (CartesianPlot *) malloc(sizeof(CartesianPlot));
    pcp->hxrt1 = pcp->hxrt2 = NULL;
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
    XrtDataHandle hxrt1, hxrt2;
    XrtDataStyle myds1[MAX_TRACES], myds2[MAX_TRACES];
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

  /* Allocate the XRT data structures with size maxElements */
    hxrt1 = hxrt2 = NULL;
    hxrt1 = XrtDataCreate(XRT_GENERAL,1,maxElements);
    if (pcp->nTraces > 1) {
	hxrt2 = XrtDataCreate(XRT_GENERAL,pcp->nTraces-1,maxElements);
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
		XrtDataSetLastPoint(hxrt1,i,0);
		minY = MIN(minY,t->recordY->lopr);
		maxY = MAX(maxY,t->recordY->hopr);
	    } else {
		XrtDataSetLastPoint(hxrt2,i-1,0);
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
		      XrtDataSetYElement(hxrt1,i,j,(float)j);
		    minY = MIN(minY,0.);
		    maxY = MAX(maxY,(float)((int)t->recordX->elementCount-1));
		} else {
		    for(j = 0; j < (int)t->recordX->elementCount; j++)
		     XrtDataSetYElement(hxrt2,i-1,j,(float)j);
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
		      XrtDataSetYElement(hxrt1,i,j,(float)j);
		    XrtDataSetXElement(hxrt1,i,0,(float)t->recordX->value);
		    minY = MIN(minY,0.);
		    maxY = MAX(maxY,(float)dlCartesianPlot->count);
		} else {
		    for(j = 0; j < dlCartesianPlot->count; j++)
		      XrtDataSetYElement(hxrt2,i-1,j,(float)j);
		    XrtDataSetXElement(hxrt2,i-1,0,(float)t->recordX->value);
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
		      XrtDataSetYElement(hxrt1,i,j,(float)j);
		    minY = MIN(minY,t->recordY->lopr);
		    maxY = MAX(maxY,t->recordY->hopr);
		} else {
		    for(j = 0; j < t->recordY->elementCount; j++)
		      XrtDataSetXElement(hxrt2,i-1,j,(float)j);
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
		      XrtDataSetXElement(hxrt1,0,j,(float)j);
		    XrtDataSetYElement(hxrt1,0,0,(float)t->recordY->value);
		    XrtDataSetLastPoint(hxrt1,0,0);
		    minY = MIN(minY,t->recordY->lopr);
		    maxY = MAX(maxY,t->recordY->hopr);
		} else {
		    for(j = 0; j < dlCartesianPlot->count; j++)
		      XrtDataSetXElement(hxrt2,0,j,(float)j);
		    XrtDataSetYElement(hxrt2,0,0,(float)t->recordY->value);
		    XrtDataSetLastPoint(hxrt2,0,0);
		    minY2 = MIN(minY2,t->recordY->lopr);
		    maxY2 = MAX(maxY2,t->recordY->hopr);
		}
		minX = MIN(minX,0.);
		maxX = MAX(maxX,(float)dlCartesianPlot->count);
	    }
	}
    }     /* End for loop over traces */
    
  /* Loop over traces and initialize XrtDataStyle array */
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


  /* Loop over traces and set XrtDataStyle array */
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
		XrtSetNthDataStyle(widget,i,&myds1[i]);
	    } else {
		myds2[i-1].fpat = XRT_FPAT_NONE;
		myds2[i-1].color = rgb[i];
		myds2[i-1].pcolor = rgb[i];
		myds2[i-1].psize = MAX(2, dlCartesianPlot->object.height/70);
		XrtSetNthDataStyle2(widget, i-1,&myds2[i-1]);
	    }
	    break;
	case FILL_UNDER_PLOT:
	    if (i <= 0) {
		myds1[i].color = rgb[i];
		myds1[i].pcolor = rgb[i];
		myds1[i].psize = MAX(2, dlCartesianPlot->object.height/70);
		XrtSetNthDataStyle(widget,i,&myds1[i]);
	    } else {
		myds2[i-1].color = rgb[i];
		myds2[i-1].pcolor = rgb[i];
		myds2[i-1].psize = MAX(2, dlCartesianPlot->object.height/70);
		XrtSetNthDataStyle2(widget, i-1,&myds2[i-1]);
	    }
	    break;
	}
    }

  /* Record the trace number and set the data pointers in the XYTrace */
    for (i = 0; i < pcp->nTraces; i++) {
	XYTrace *t = &(pcp->xyTrace[i]);

	if ( i <= 0) {
	    t->hxrt = hxrt1;
	    t->trace = i;
	} else {
	    t->hxrt = hxrt2;
	    t->trace = i-1;
	}
    }

  /* Set the data pointers in the CartesianPlot */
    pcp->hxrt1 = hxrt1;
    pcp->hxrt2 = hxrt2;

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
	nextPoint = XrtDataGetLastPoint(pt->hxrt,pt->trace);
	if(pt->init) nextPoint++;
	else pt->init=1;
	if (nextPoint < dlCartesianPlot->count) {
	    XrtDataSetXElement(pt->hxrt,pt->trace,nextPoint,
	      SAFEFLOAT(pt->recordX->value));
	    XrtDataSetYElement(pt->hxrt,pt->trace,nextPoint,
	      SAFEFLOAT(pt->recordY->value));
	    XrtDataSetLastPoint(pt->hxrt,pt->trace,nextPoint);
	} else {
	    if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
	      /* All done, don't add any more points */
	    } else if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
	      /* Shift everybody down one, add at end */
		int j;
		for (j = 1; j < dlCartesianPlot->count; j++) {
		    XrtDataSetXElement(pt->hxrt,pt->trace,j-1,
		      XrtDataGetXElement(pt->hxrt,pt->trace,j));
		    XrtDataSetYElement(pt->hxrt,pt->trace,j-1,
		      XrtDataGetYElement(pt->hxrt,pt->trace,j));
		}
		XrtDataSetXElement(pt->hxrt,pt->trace,j-1,
		  SAFEFLOAT(pt->recordX->value));
		XrtDataSetYElement(pt->hxrt,pt->trace,j-1,
		  SAFEFLOAT(pt->recordY->value));
	    }
	}
#if 0     /* Use with cpCP_XYScalar.adl or delete these lines */
	printf("nextPoint=%d dlCartesianPlot->count=%d x=%g y=%g\n",
	  nextPoint,dlCartesianPlot->count,
	  pt->recordX->value,pt->recordY->value);
	printf("x1=%g x2=%g x3=%g\n",
	  XrtDataGetXElement(pt->hxrt,pt->trace,0),
	  XrtDataGetXElement(pt->hxrt,pt->trace,1),
	  XrtDataGetXElement(pt->hxrt,pt->trace,2));
	printf("y1=%g y2=%g y3=%g\n\n",
	  XrtDataGetYElement(pt->hxrt,pt->trace,0),
	  XrtDataGetYElement(pt->hxrt,pt->trace,1),
	  XrtDataGetYElement(pt->hxrt,pt->trace,2));
#endif		
	break;

    case CP_XScalar:
      /* x channel scalar, up to dlCartesianPlot->count pairs */
	nextPoint = XrtDataGetLastPoint(pt->hxrt,pt->trace);
	if(pt->init) nextPoint++;
	else pt->init=1;
	if (nextPoint < dlCartesianPlot->count) {
	    XrtDataSetXElement(pt->hxrt,pt->trace,nextPoint,
	      SAFEFLOAT(pt->recordX->value));
	    XrtDataSetYElement(pt->hxrt,pt->trace,nextPoint,(float)nextPoint);
	    XrtDataSetLastPoint(pt->hxrt,pt->trace,nextPoint);
	} else {
	    if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
	      /* All done, don't add any more points */
	    } else if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
	      /* Shift everybody down one, add at end */
		int j;
		for (j = 1; j < dlCartesianPlot->count; j++) {
		    XrtDataSetXElement(pt->hxrt,pt->trace,j-1,
		      XrtDataGetXElement(pt->hxrt,pt->trace,j));
		    XrtDataSetYElement(pt->hxrt,pt->trace,j-1,(float)(j-1));
		}
		XrtDataSetXElement(pt->hxrt,pt->trace,j-1,
		  SAFEFLOAT(pt->recordX->value));
		XrtDataSetYElement(pt->hxrt,pt->trace,j-1,(float)(j-1));
	    }
	}
	break;


    case CP_XVector:
      /* x channel vector, ca_element_count(chid) elements */
	XrtDataSetLastPoint(pt->hxrt,pt->trace,MAX(pt->recordX->elementCount-1,0));
	switch(pt->recordX->dataType) {
	case DBF_STRING:
	{
	    for (j = 0; j < pt->recordX->elementCount; j++) {
		XrtDataSetXElement(pt->hxrt,pt->trace,j,0.0);
		XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_INT:
	{
	    short *pShort = (short *) pt->recordX->array;
	    for (j = 0; j < pt->recordX->elementCount; j++) {
		XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)pShort[j]);
		XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_FLOAT:
	{
	    float *pFloat = (float *) pt->recordX->array;
	    for (j = 0; j < pt->recordX->elementCount; j++) {
		XrtDataSetXElement(pt->hxrt,pt->trace,j,SAFEFLOAT(pFloat[j]));
		XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_ENUM:
	{
	    unsigned short *pUShort = (unsigned short *) pt->recordX->array;
	    for (j = 0; j < pt->recordX->elementCount; j++) {
		XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)pUShort[j]);
		XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_CHAR:
	{
	    unsigned char *pUChar = (unsigned char *) pt->recordX->array;
	    for (j = 0; j < pt->recordX->elementCount; j++) {
		XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)pUChar[j]);
		XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_LONG:
	{
	    dbr_long_t *pLong = (dbr_long_t *) pt->recordX->array;
	    for (j = 0; j < pt->recordX->elementCount; j++) {
		XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)pLong[j]);
		XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_DOUBLE:
	{
	    double *pDouble = (double *) pt->recordX->array;
	    for (j = 0; j < pt->recordX->elementCount; j++) {
		XrtDataSetXElement(pt->hxrt,pt->trace,j,SAFEFLOAT(pDouble[j]));
		XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)j);
	    }
	    break;
	    }
	}
	break;

    case CP_YScalar:
      /* y channel scalar, up to dlCartesianPlot->count pairs */
	nextPoint = XrtDataGetLastPoint(pt->hxrt,pt->trace);
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
	    XrtDataSetXElement(pt->hxrt,pt->trace,nextPoint,(pcp->timeScale) ?
	      (float)(pt->recordY->time.secPastEpoch -
		pcp->startTime.secPastEpoch) :
	      (float) nextPoint);
	    XrtDataSetYElement(pt->hxrt,pt->trace,nextPoint,
	      (float)pt->recordY->value);
	    XrtDataSetLastPoint(pt->hxrt,pt->trace,nextPoint);
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
			XrtDataSetXElement(pt->hxrt,pt->trace,j-1,
			  XrtDataGetXElement(pt->hxrt,pt->trace,j));
			XrtDataSetYElement(pt->hxrt,pt->trace,j-1,
			  XrtDataGetYElement(pt->hxrt,pt->trace,j));
		    }
		    XrtDataSetXElement(pt->hxrt,pt->trace,j-1,
		      (float)(pt->recordY->time.secPastEpoch -
		      pcp->startTime.secPastEpoch)); 
		    XrtDataSetYElement(pt->hxrt,pt->trace,j-1,
		      (float)pt->recordY->value);
		} else {
		    for (j = 1; j < dlCartesianPlot->count; j++) {
			XrtDataSetYElement(pt->hxrt,pt->trace,j-1,
			  XrtDataGetYElement(pt->hxrt,pt->trace,j));
		    }
		    XrtDataSetYElement(pt->hxrt,pt->trace,j-1,
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
		XrtDataSaveToFile(pt->hxrt,filename,NULL);
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
	      XrtDataGetLastPoint(pt->hxrt,pt->trace),
	      XrtDataGetXElement(pt->hxrt,pt->trace,nextPoint1),
	      XrtDataGetYElement(pt->hxrt,pt->trace,nextPoint1));
	}
#endif    
	break;

    case CP_YVector:
      /* plot first "count" elements of vector per dlCartesianPlot */
	XrtDataSetLastPoint(pt->hxrt,pt->trace,pt->recordY->elementCount-1);
	switch(pt->recordY->dataType) {
	case DBF_STRING:
	{
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		XrtDataSetYElement(pt->hxrt,pt->trace,j,0.0);
		XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_INT:
	{
	    short *pShort = (short *) pt->recordY->array;
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)pShort[j]);
		XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_FLOAT:
	{
	    float *pFloat = (float *) pt->recordY->array;
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		XrtDataSetYElement(pt->hxrt,pt->trace,j,SAFEFLOAT(pFloat[j]));
		XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_ENUM:
	{
	    unsigned short *pUShort = (unsigned short *) pt->recordY->array;
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)pUShort[j]);
		XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_CHAR:
	{
	    unsigned char *pUChar = (unsigned char *) pt->recordY->array;
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)pUChar[j]);
		XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_LONG:
	{
	    dbr_long_t *pLong = (dbr_long_t *) pt->recordY->array;
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)pLong[j]);
		XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_DOUBLE:
	{
	    double *pDouble = (double *) pt->recordY->array;
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		XrtDataSetYElement(pt->hxrt,pt->trace,j,SAFEFLOAT(pDouble[j]));
		XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)j);
	    }
	    break;
	}
	}
	break;


    case CP_XVectorYScalar:
	XrtDataSetLastPoint(pt->hxrt,pt->trace,MAX(pt->recordX->elementCount-1,0));
	if (pr == pt->recordX) {
	  /* plot first "count" elements of vector per dlCartesianPlot */
	    switch(pt->recordX->dataType) {
	    case DBF_STRING:
	    {
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    XrtDataSetXElement(pt->hxrt,pt->trace,j,0.0);
		}
		break;
	    }
	    case DBF_INT:
	    {
		short *pShort = (short *) pt->recordX->array;
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)pShort[j]);
		}
		break;
	    }
	    case DBF_FLOAT:
	    {
		float *pFloat = (float *) pt->recordX->array;
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    XrtDataSetXElement(pt->hxrt,pt->trace,j,SAFEFLOAT(pFloat[j]));
		}
		break;
	    }
	    case DBF_ENUM:
	    {
		unsigned short *pUShort = (unsigned short *) pt->recordX->array;
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)pUShort[j]);
		}
		break;
	    }
	    case DBF_CHAR:
	    {
		unsigned char *pUChar = (unsigned char *) pt->recordX->array;
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)pUChar[j]);
		}
		break;
	    }
	    case DBF_LONG:
	    {
		dbr_long_t *pLong = (dbr_long_t *) pt->recordX->array;
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)pLong[j]);
		}
		break;
	    }
	    case DBF_DOUBLE:
	    {
		double *pDouble = (double *) pt->recordX->array;
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    XrtDataSetXElement(pt->hxrt,pt->trace,j,
		      SAFEFLOAT(pDouble[j]));
		}
		break;
	    }
	    }
	} else {
	    if (pr == pt->recordY) {
		for (j = 0; j < pt->recordX->elementCount; j++) {
		    XrtDataSetYElement(pt->hxrt,pt->trace,j,
		      SAFEFLOAT(pt->recordY->value));
		}
	    }
	}
	break;

    case CP_YVectorXScalar:
	XrtDataSetLastPoint(pt->hxrt,pt->trace,MAX(pt->recordY->elementCount-1,0));
	if (pr == pt->recordY) {
	  /* plot first "count" elements of vector per dlCartesianPlot */
	    switch(pt->recordY->dataType) {
	    case DBF_STRING:
	    {
		for (j = 0; j < pt->recordY->elementCount; j++) {
		    XrtDataSetYElement(pt->hxrt,pt->trace,j,0.0);
		}
		break;
	    }
	    case DBF_INT:
	    {
		short *pShort = (short *) pt->recordY->array;
		for (j = 0; j < pt->recordY->elementCount; j++) {
		    XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)pShort[j]);
		}
		break;
	    }
	    case DBF_FLOAT:
	    {
		float *pFloat = (float *) pt->recordY->array;
		for (j = 0; j < pt->recordY->elementCount; j++) {
		    XrtDataSetYElement(pt->hxrt,pt->trace,j,
		      SAFEFLOAT(pFloat[j]));
		}
		break;
	    }
	    case DBF_ENUM:
	    {
		unsigned short *pUShort = (unsigned short *) pt->recordY->array;
		for (j = 0; j < pt->recordY->elementCount; j++) {
		    XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)pUShort[j]);
		}
		break;
	    }
	    case DBF_CHAR:
	    {
		unsigned char *pUChar = (unsigned char *) pt->recordY->array;
		for (j = 0; j < pt->recordY->elementCount; j++) {
		    XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)pUChar[j]);
		}
		break;
	    }
	    case DBF_LONG:
	    {
		dbr_long_t *pLong = (dbr_long_t *) pt->recordY->array;
		for (j = 0; j < pt->recordY->elementCount; j++) {
		    XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)pLong[j]);
		}
		break;
	    }
	    case DBF_DOUBLE:
	    {
		double *pDouble = (double *) pt->recordY->array;
		for (j = 0; j < pt->recordY->elementCount; j++) {
		    XrtDataSetYElement(pt->hxrt,pt->trace,j,
		      SAFEFLOAT(pDouble[j]));
		}
		break;
	    }
	    }
	} else  if (pr == pt->recordX) {
	    for (j = 0; j < pt->recordY->elementCount; j++) {
		XrtDataSetXElement(pt->hxrt,pt->trace,j,
		  SAFEFLOAT(pt->recordX->value));
	    }
	}
	break;

    case CP_XYVector: {
	int dox;
	int count;

	count = MIN(pt->recordX->elementCount, pt->recordY->elementCount);
	XrtDataSetLastPoint(pt->hxrt,pt->trace,MAX(count-1,0));

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
		      XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)(pLong[j] - offset));
		    else
		      XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)(pLong[j] - offset));
		}
		break;
	    }
	    case DBF_DOUBLE:
	    {
		double *pDouble = (double *) pr->array;
		double offset = (double) pcp->startTime.secPastEpoch;
		for (j = 0; j < count; j++) {
		    if(dox)
		      XrtDataSetXElement(pt->hxrt,pt->trace,j,SAFEFLOAT(pDouble[j] - offset));
		    else
		      XrtDataSetYElement(pt->hxrt,pt->trace,j,SAFEFLOAT(pDouble[j] - offset));
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
		      XrtDataSetXElement(pt->hxrt,pt->trace,j,0.0);
		    else
		      XrtDataSetYElement(pt->hxrt,pt->trace,j,0.0);
		}
		break;
	    case DBF_INT:
	    {
		short *pShort = (short *) pr->array;
		for (j = 0; j < count; j++) {
		    if(dox)
		      XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)pShort[j]);
		    else
		      XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)pShort[j]);
		}
		break;
	    }
	    case DBF_FLOAT:
	    {
		float *pFloat = (float *) pr->array;
		for (j = 0; j < count; j++) {
		    if(dox)
		      XrtDataSetXElement(pt->hxrt,pt->trace,j,
			SAFEFLOAT(pFloat[j]));
		    else
		      XrtDataSetYElement(pt->hxrt,pt->trace,j,
			SAFEFLOAT(pFloat[j]));
		}
		break;
	    }
	    case DBF_ENUM:
	    {
		unsigned short *pUShort = (unsigned short *) pr->array;
		for (j = 0; j < count; j++) {
		    if(dox)
		      XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)pUShort[j]);
		    else
		      XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)pUShort[j]);
		}
		break;
	    }
	    case DBF_CHAR:
	    {
		unsigned char *pUChar = (unsigned char *) pr->array;
		for (j = 0; j < count; j++) {
		    if(dox)
		      XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)pUChar[j]);
		    else
		      XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)pUChar[j]);
		}
		break;
	    }
	    case DBF_LONG:
	    {
		dbr_long_t *pLong = (dbr_long_t *) pr->array;
		for (j = 0; j < count; j++) {
		    if(dox)
		      XrtDataSetXElement(pt->hxrt,pt->trace,j,(float)pLong[j]);
		    else
		      XrtDataSetYElement(pt->hxrt,pt->trace,j,(float)pLong[j]);
		}
		break;
	    }
	    case DBF_DOUBLE:
	    {
		double *pDouble = (double *) pr->array;
		for (j = 0; j < count; j++) {
		    if(dox)
		      XrtDataSetXElement(pt->hxrt,pt->trace,j,
			SAFEFLOAT(pDouble[j]));
		    else
		      XrtDataSetYElement(pt->hxrt,pt->trace,j,
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
	printf("  recordX=%x recordY=%x hxrt=%x\n",
	  t->recordX,t->recordY,t->hxrt);
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
	    if (t->hxrt == NULL) return;
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
	if (t->hxrt == pcp->hxrt1)
	  XtVaSetValues(widget,XtNxrtData,t->hxrt,NULL);
	else if (t->hxrt == pcp->hxrt2) {
	    XtVaSetValues(widget,XtNxrtData2,t->hxrt,NULL);
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
		    int n = XrtDataGetLastPoint(t->hxrt,t->trace);
		    if (n > 0) {
			if ((t->hxrt == pcp->hxrt1) && (clearDataSet1)) {
			    XtVaSetValues(widget,XtNxrtData,hxrtNullData,NULL);
			    clearDataSet1 = False;
			} else if ((t->hxrt == pcp->hxrt2) && (clearDataSet2)) {
			    XtVaSetValues(widget,XtNxrtData2,hxrtNullData,NULL);
			    clearDataSet2 = False;
			}
			XrtDataSetLastPoint(t->hxrt,t->trace,0);
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
		int n = XrtDataGetLastPoint(t->hxrt,t->trace);
		if (n > 0) {
		    if ((t->hxrt == pcp->hxrt1) && (clearDataSet1)) {
			XtVaSetValues(widget,XtNxrtData,hxrtNullData,NULL);
			clearDataSet1 = False;
			pcp->dirty1 = False;
		    } else if ((t->hxrt == pcp->hxrt2) && (clearDataSet2)) {
			XtVaSetValues(widget,XtNxrtData2,hxrtNullData,NULL);
			clearDataSet2 = False;
			pcp->dirty2 = False;
		    }
		    XrtDataSetLastPoint(t->hxrt,t->trace,0);
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
	    if ((t->recordX) && (t->hxrt)) {
		cartesianPlotUpdateTrace((XtPointer)t->recordX);
		if (t->type == CP_XYVector)
		  cartesianPlotUpdateTrace((XtPointer)t->recordY);
		if (t->hxrt == pcp->hxrt1)
		  pcp->dirty1 = True;
		else if (t->hxrt == pcp->hxrt2)
		  pcp->dirty2 = True;
	    } else if ((t->recordY) && (t->hxrt)) {
		cartesianPlotUpdateTrace((XtPointer)t->recordY);
		if (t->hxrt == pcp->hxrt1)
		  pcp->dirty1 = True;
		else if (t->hxrt == pcp->hxrt2)
		  pcp->dirty2 = True;
	    }
	}
    } else {
      /* No trigger channel, proceed as normal */
	cartesianPlotUpdateTrace((XtPointer)pr);
	if (pt->hxrt == pcp->hxrt1) {
	    pcp->dirty1 = True;
	} else if (pt->hxrt == pcp->hxrt2) {
	    pcp->dirty2 = True;
	} else {
	    medmPrintf(1,"\ncartesianPlotUpdateValueCb: Illegal xrtDataSet specified\n");
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
	if (pcp->hxrt1) XrtDataDestroy(pcp->hxrt1);
	if (pcp->hxrt2) XrtDataDestroy(pcp->hxrt2);
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
		    XtVaSetValues(widget,XtNxrtData,pcp->hxrt1,NULL);
		}
		if (pcp->dirty2) {
		    pcp->dirty2 = False;
		    XtVaSetValues(widget,XtNxrtData2,pcp->hxrt2,NULL);
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
