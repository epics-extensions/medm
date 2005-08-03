/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/* Routines used to implement the Cartesian Plot using Xrt/Graph */

/* KE: Note that MEDM uses the union XcVType (of a float and a long) to convert
 *   float values to the values needed for XRT float resources.  This currently
 *   gives the same results as using the XRT recommended XrtFloatToArg(), but
 *   probably isn't guaranteed */

#define DEBUG_CARTESIAN_PLOT 0
#define DEBUG_XRT 0
#define DEBUG_AXIS 0
#define DEBUG_COUNT 0

#define MAX(a,b) ((a)>(b)?(a):(b))

#include "medm.h"
#include "medmXrtGraph.h"
#include "medmCartesianPlot.h"

static CpDataHandle hcpNullData = (CpDataHandle)0;

/* Function prototypes */
static void destroyXrtPropertyEditor(Widget w, XtPointer cd, XtPointer cbs);

#if XRT_VERSION < 3
/* Routines to make XRT/graph backward compatible from Version 3.0 */

CpDataHandle CpDataCreate(Widget w, CpDataType type, int nsets, int npoints) {
    return XrtMakeData(type,nsets,npoints,True);
}

int CpDataGetPointsUsed(CpDataHandle hData, int set) {
    return (hData->g.data[set].npoints);
}

double CpDataGetXElement(CpDataHandle hData, int set, int point) {
    return hData->g.data[set].xp[point];
}

double CpDataGetYElement(CpDataHandle hData, int set, int point) {
    return hData->g.data[set].yp[point];
}

void CpDataDestroy(CpDataHandle hData) {
    if(hData) XrtDestroyData(hData,True);
}

int CpDataSetHole(CpDataHandle hData, double hole) {
    hData->g.hole = hole;
    return 1;
}

int CpDataSetPointsUsed(Widget w, CpDataHandle hData, int set, int point) {
  /* w is unused */
    hData->g.data[set].npoints = MAX(point,0);
    return 1;
}

int CpDataSetXElement(CpDataHandle hData, int set, int point, double x) {
    hData->g.data[set].xp[point] = x;
    return 1;
}

int CpDataSetYElement(CpDataHandle hData, int set, int point, double y) {
    hData->g.data[set].yp[point] = y;
    return 1;
}
#endif

#if XRT_VERSION > 2
#ifdef XRT_EXTENSIONS
static void destroyXrtPropertyEditor(Widget w, XtPointer cd, XtPointer cbs)
{
    UNREFERENCED(cd);
    UNREFERENCED(cbs);

  /* False means do not destroy the dialog */
    XrtPopdownPropertyEditor(w,False);
}
#endif
#endif

void CpDataDeleteCurves(Widget w, CpDataHandle hData) {
  /* Not necessary */
}

void CpGetAxisInfo(Widget w,
  XtPointer *userData, Boolean *xAxisIsTime, char **timeFormat,
  Boolean *xAxisIsLog, Boolean *yAxisIsLog, Boolean *y2AxisIsLog,
  Boolean *xAxisIsAuto, Boolean *yAxisIsAuto, Boolean *y2AxisIsAuto,
  XcVType *xMaxF, XcVType *yMaxF, XcVType *y2MaxF,
  XcVType *xMinF, XcVType *yMinF, XcVType *y2MinF)
{
    XrtAnnoMethod xAnnoMethod;
    int xMin, yMin, y2Min, xMax, yMax, y2Max;
    Arg args[15];
    int nargs;

#if DEBUG_AXIS
    print("CpGetAxisInfo: Before XtGetValues\n");
#endif
    nargs=0;
    XtSetArg(args[nargs],XmNuserData, userData); nargs++;
    XtSetArg(args[nargs],XtNxrtXAnnotationMethod, &xAnnoMethod); nargs++;
    XtSetArg(args[nargs],XtNxrtXAxisLogarithmic, xAxisIsLog); nargs++;
    XtSetArg(args[nargs],XtNxrtYAxisLogarithmic, yAxisIsLog); nargs++;
    XtSetArg(args[nargs],XtNxrtY2AxisLogarithmic, y2AxisIsLog); nargs++;
    XtSetArg(args[nargs],XtNxrtXMin, &xMin); nargs++;
    XtSetArg(args[nargs],XtNxrtYMin, &yMin); nargs++;
    XtSetArg(args[nargs],XtNxrtY2Min, &y2Min); nargs++;
    XtSetArg(args[nargs],XtNxrtXMax, &xMax); nargs++;
    XtSetArg(args[nargs],XtNxrtYMax, &yMax); nargs++;
    XtSetArg(args[nargs],XtNxrtY2Max, &y2Max); nargs++;
  /* Axis is auto if using default for (min and max) */
    XtSetArg(args[nargs],XtNxrtXMinUseDefault, xAxisIsAuto); nargs++;
    XtSetArg(args[nargs],XtNxrtYMinUseDefault, yAxisIsAuto); nargs++;
    XtSetArg(args[nargs],XtNxrtY2MinUseDefault, y2AxisIsAuto); nargs++;
    XtSetArg(args[nargs],XtNxrtTimeFormat, timeFormat); nargs++;
    XtGetValues(w,args,nargs);
#if DEBUG_AXIS
    print("CpGetAxisInfo: After XtGetValues nargs=%d\n",nargs);
#endif

    *xAxisIsTime = (xAnnoMethod == XRT_ANNO_TIME_LABELS)?True:False;
    xMinF->lval = xMin;
    yMinF->lval = yMin;
    y2MinF->lval = y2Min;
    xMaxF->lval = xMax;
    yMaxF->lval = yMax;
    y2MaxF->lval = y2Max;
}

void CpGetAxisMaxMin(Widget w, int axis, XcVType *maxF, XcVType *minF)
{
    Arg args[2];
    int nargs;
    int min, max;

    nargs=0;
    switch(axis) {
    case CP_X:
	XtSetArg(args[nargs],XtNxrtXMax,&max); nargs++;
	XtSetArg(args[nargs],XtNxrtXMin,&min); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNxrtYMax,&max); nargs++;
	XtSetArg(args[nargs],XtNxrtYMin,&min); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNxrtY2Max,&max); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Min,&min); nargs++;
	break;
    }
    XtGetValues(w,args,nargs);

    minF->lval = min;
    maxF->lval = max;
}

void CpSetAxisStyle(Widget w, CpDataHandle hData, int trace, int lineType,
  int fillType, XColor color, int pointSize)
  /* hData is unused */
{
    char rgb[16];
    XrtDataStyle myds;

  /* Convert color to a color name */
    sprintf(rgb,"#%2.2x%2.2x%2.2x", color.red>>8,
      color.green>>8, color.blue>>8);

  /* Zero the XrtDataStyle struct (should not be necessary, but is consistent) */
    memset(&myds,0,sizeof(XrtDataStyle));

  /* Fill in the XrtDataStyle struct */
    if(lineType == CP_LINE_NONE) myds.lpat = XRT_LPAT_NONE;
    else myds.lpat = XRT_LPAT_SOLID;
    if(fillType == CP_LINE_NONE) myds.fpat = XRT_FPAT_NONE;
    else myds.fpat = XRT_FPAT_SOLID;
    myds.color = rgb;
    myds.width = 1;
    myds.point = XRT_POINT_DOT;
    myds.pcolor = rgb;
    myds.psize = pointSize;

#if DEBUG_XRT
    print("CpSetAxisStyle: w=%x trace=%d lineType=%d fillType=%d pointSize=%d\n",
      w,trace,lineType,fillType,pointSize);
    print("  rgb=%s red=%hx green=%hx blue=%hx pixel=%lx\n",
      rgb,color.red,color.green,color.blue,color.pixel);
#endif

  /* First trace is y1 axis, others are y2 axis */
    if(trace == 0) {
	myds.point = XRT_POINT_DOT;
	XrtSetNthDataStyle(w, trace, &myds);
    } else {
	switch(trace%8) {
	case 0:
	    myds.point = (XrtPoint)(XRT_POINT_DOT);
	    break;
	case 1:
	    myds.point = (XrtPoint)(XRT_POINT_BOX);
	    break;
	case 2:
	    myds.point = (XrtPoint)(XRT_POINT_TRI);
	    break;
	case 3:
	    myds.point = (XrtPoint)(XRT_POINT_DIAMOND);
	    break;
	case 4:
	    myds.point = (XrtPoint)(XRT_POINT_STAR);
	    break;
	case 5:
	    myds.point = (XrtPoint)(XRT_POINT_CROSS);
	    break;
	case 6:
	    myds.point = (XrtPoint)(XRT_POINT_CIRCLE);
	    break;
	case 7:
	    myds.point = (XrtPoint)(XRT_POINT_SQUARE);
	    break;
	default:
	    myds.point = (XrtPoint)(XRT_POINT_NONE);
	    break;
	}
	XrtSetNthDataStyle2(w, trace-1, &myds);
    }
}

void CpSetAxisAll(Widget w, int axis, XcVType max, XcVType min,
  XcVType tick, XcVType num, int precision)
{
    Arg args[5];
    int nargs;

    nargs=0;
    switch(axis) {
    case CP_X:
	XtSetArg(args[nargs],XtNxrtXMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXMin,min.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXTick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXNum,num.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXPrecision,precision); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNxrtYMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYMin,min.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYTick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYNum,num.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYPrecision,precision); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNxrtY2Max,max.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Min,min.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Tick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Num,num.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Precision,precision); nargs++;
	break;
    }
    XtSetValues(w,args,nargs);
}

void CpSetAxisAuto(Widget w, int axis)
{
    Arg args[5];
    int nargs;

    nargs=0;
    switch(axis) {
    case CP_X:
	XtSetArg(args[nargs],XtNxrtXMaxUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtXMinUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtXTickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtXNumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtXPrecisionUseDefault,True); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNxrtYMaxUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtYMinUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtYTickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtYNumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtYPrecisionUseDefault,True); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNxrtY2MaxUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtY2MinUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtY2TickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtY2NumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtY2PrecisionUseDefault,True); nargs++;
	break;
    }
    XtSetValues(w,args,nargs);
}

void CpSetAxisChannel(Widget w, int axis, XcVType max, XcVType min)
{
    Arg args[5];
    int nargs;

    nargs=0;
    switch(axis) {
    case CP_X:
	XtSetArg(args[nargs],XtNxrtXMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXMin,min.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXTickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtXNumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtXPrecisionUseDefault,True); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNxrtYMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYMin,min.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYTickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtYNumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtYPrecisionUseDefault,True); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNxrtY2Max,max.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Min,min.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2TickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtY2NumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtY2PrecisionUseDefault,True); nargs++;
	break;
    }
    XtSetValues(w,args,nargs);
}

void CpSetAxisLinear(Widget w, int axis)
{
    Arg args[2];
    int nargs;

    nargs=0;
    switch(axis) {
    case CP_X:
	XtSetArg(args[nargs],XtNxrtXAnnotationMethod,XRT_ANNO_VALUES); nargs++;
	XtSetArg(args[nargs],XtNxrtXAxisLogarithmic,False); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNxrtYAxisLogarithmic,False); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNxrtY2AxisLogarithmic,False); nargs++;
	break;
    }
    XtSetValues(w,args,nargs);
}

void CpSetAxisLog(Widget w, int axis)
{
    Arg args[15];
    int nargs;

    nargs=0;
    switch(axis) {
    case CP_X:
	XtSetArg(args[nargs],XtNxrtXAnnotationMethod,XRT_ANNO_VALUES); nargs++;
	XtSetArg(args[nargs],XtNxrtXAxisLogarithmic,True); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNxrtYAxisLogarithmic,True); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNxrtY2AxisLogarithmic,True); nargs++;
	break;
    }
    XtSetValues(w,args,nargs);
}

void CpSetAxisMaxMin(Widget w, int axis, XcVType max, XcVType min)
{
    Arg args[2];
    int nargs;

    nargs=0;
    switch(axis) {
    case CP_X:
	XtSetArg(args[nargs],XtNxrtXMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXMin,min.lval); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNxrtYMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYMin,min.lval); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNxrtY2Max,max.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Min,min.lval); nargs++;
	break;
    }
    XtSetValues(w,args,nargs);
}

void CpSetAxisMax(Widget w, int axis, XcVType max, XcVType tick,
  XcVType num, int precision)
{
    Arg args[4];
    int nargs;

    nargs=0;
    switch(axis) {
    case CP_X:
	XtSetArg(args[nargs],XtNxrtXMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXTick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXNum,num.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXPrecision,precision); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNxrtYMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYTick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYNum,num.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYPrecision,precision); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNxrtY2Max,max.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Tick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Num,num.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Precision,precision); nargs++;
	break;
    }
    XtSetValues(w,args,nargs);
}

void CpSetAxisMin(Widget w, int axis, XcVType min, XcVType tick,
  XcVType num, int precision)
{
    Arg args[4];
    int nargs;

    nargs=0;
    switch(axis) {
    case CP_X:
	XtSetArg(args[nargs],XtNxrtXMin,min.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXTick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXNum,num.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXPrecision,precision); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNxrtYMin,min.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYTick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYNum,num.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYPrecision,precision); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNxrtY2Min,min.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Tick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Num,num.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Precision,precision); nargs++;
	break;
    }
    XtSetValues(w,args,nargs);
}

void CpSetAxisTime(Widget w, int axis, time_t base, char * format)
{
    Arg args[6];
    int nargs;

    nargs=0;
    switch(axis) {
    case CP_X:
	XtSetArg(args[nargs],XtNxrtXAxisLogarithmic,False); nargs++;
	XtSetArg(args[nargs],XtNxrtXAnnotationMethod,XRT_ANNO_TIME_LABELS); nargs++;
	XtSetArg(args[nargs],XtNxrtTimeBase,base); nargs++;
	XtSetArg(args[nargs],XtNxrtTimeUnit,XRT_TMUNIT_SECONDS); nargs++;
	XtSetArg(args[nargs],XtNxrtTimeFormatUseDefault,False); nargs++;
	XtSetArg(args[nargs],XtNxrtTimeFormat,format); nargs++;
	break;
    }
    XtSetValues(w,args,nargs);
}

void CpSetData(Widget w, int axis, CpDataHandle hData)
{
    Arg args[1];
    int nargs;

    nargs=0;
    if(axis == CP_Y) {
	XtSetArg(args[nargs],XtNxrtData,hData); nargs++;
    } else {
	XtSetArg(args[nargs],XtNxrtData2,hData); nargs++;
    }
    XtSetValues(w,args,nargs);
}

void CpEraseData(Widget w, int axis, CpDataHandle hData)
{
    Arg args[1];
    int nargs;

  /* Initialize hcpNullData if not done */
    if(!hcpNullData) {
	hcpNullData = CpDataCreate((Widget)0,CP_GENERAL,1,1);
	CpDataSetHole(hcpNullData,0.0);
	CpDataSetPointsUsed((Widget)0,hcpNullData,0,0);
	CpDataSetXElement(hcpNullData,0,0,0.0);
	CpDataSetYElement(hcpNullData,0,0,0.0);
    }

    nargs=0;
    if(axis == CP_Y) {
	XtSetArg(args[nargs],XtNxrtData,hcpNullData); nargs++;
    } else {
	XtSetArg(args[nargs],XtNxrtData2,hcpNullData); nargs++;
    }
    XtSetValues(w,args,nargs);
}

void CpSetTimeBase(Widget w, time_t base)
{
    Arg args[1];
    int nargs;

    nargs=0;
    XtSetArg(args[nargs],XtNxrtTimeBase,base); nargs++;
    XtSetValues(w,args,nargs);
}

void CpSetTimeFormat(Widget w, char *format)
{
    Arg args[15];
    int nargs;

    nargs=0;
    XtSetArg(args[nargs],XtNxrtTimeFormat,format); nargs++;
    XtSetValues(w,args,nargs);
}

void CpUpdateWidget(Widget w, int full)
  /* Not necessary */
{
}

Widget CpCreateCartesianPlot(DisplayInfo *displayInfo,
  DlCartesianPlot *dlCartesianPlot, MedmCartesianPlot *pcp)
{
    Arg args[50];
    int nargs;
    XColor xColors[2];
    char rgb[2][16], string[24];
    int usedHeight, usedCharWidth, bestSize, preferredHeight;
    char *headerStrings[2];
    int i, k, iPrec;
    XcVType minF, maxF, tickF;
    Widget w;
    int validTraces = pcp ? pcp->nTraces : 0;

  /* Set widget args from the dlCartesianPlot structure */
    nargs = 0;
    XtSetArg(args[nargs],XmNx,(Position)dlCartesianPlot->object.x); nargs++;
    XtSetArg(args[nargs],XmNy,(Position)dlCartesianPlot->object.y); nargs++;
    XtSetArg(args[nargs],XmNwidth,(Dimension)dlCartesianPlot->object.width); nargs++;
    XtSetArg(args[nargs],XmNheight,(Dimension)dlCartesianPlot->object.height); nargs++;
    XtSetArg(args[nargs],XmNhighlightThickness,0); nargs++;
    XtSetArg(args[nargs],XmNhighlightThickness,0); nargs++;

#if DEBUG_CARTESIAN_PLOT_BORDER
    printf("dlCartesianPlot->object.width: %d\n",dlCartesianPlot->object.width);
    printf("dlCartesianPlot->object.height: %d\n",dlCartesianPlot->object.height);
#endif

#if 1
    {
	Pixel topShadow;
	Pixel bottomShadow;
	Pixel foreground;
	Pixel select;

	XmGetColors(XtScreen(displayInfo->drawingArea),cmap,
	  displayInfo->colormap[dlCartesianPlot->plotcom.bclr],
	    &foreground,&topShadow,&bottomShadow,&select);

/*  	    XtSetArg(args[nargs],XmNtopShadowColor,topShadow); nargs++; */
	    XtSetArg(args[nargs],XmNtopShadowColor,BlackPixel(display,screenNum)); nargs++;
	    XtSetArg(args[nargs],XmNbottomShadowColor,bottomShadow); nargs++;
	    XtSetArg(args[nargs],XmNselectColor,select); nargs++;
    }
#endif

  /* long way around for color handling... but XRT/Graph insists on strings! */
    xColors[0].pixel = displayInfo->colormap[dlCartesianPlot->plotcom.clr];
    xColors[1].pixel = displayInfo->colormap[dlCartesianPlot->plotcom.bclr];
    XQueryColors(display,cmap,xColors,2);
    sprintf(rgb[0],"#%2.2x%2.2x%2.2x",xColors[0].red>>8, xColors[0].green>>8,
      xColors[0].blue>>8);
    sprintf(rgb[1],"#%2.2x%2.2x%2.2x",xColors[1].red>>8, xColors[1].green>>8,
      xColors[1].blue>>8);
    XtSetArg(args[nargs],XtNxrtGraphForegroundColor,rgb[0]); nargs++;
    XtSetArg(args[nargs],XtNxrtGraphBackgroundColor,rgb[1]); nargs++;
    XtSetArg(args[nargs],XtNxrtForegroundColor,rgb[0]); nargs++;
    XtSetArg(args[nargs],XtNxrtBackgroundColor,rgb[1]); nargs++;
    preferredHeight = MIN(dlCartesianPlot->object.width,
      dlCartesianPlot->object.height)/TITLE_SCALE_FACTOR;
    bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
      preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
#if XRT_VERSION > 2
    XtSetArg(args[nargs],XtNxrtHeaderFont,fontListTable[bestSize]); nargs++;
#else
    XtSetArg(args[nargs],XtNxrtHeaderFont,fontTable[bestSize]->fid); nargs++;
#endif
    if (strlen(dlCartesianPlot->plotcom.title) > 0) {
	headerStrings[0] = dlCartesianPlot->plotcom.title;
    } else {
	headerStrings[0] = NULL;
    }
    headerStrings[1] = NULL;
    XtSetArg(args[nargs],XtNxrtHeaderStrings,headerStrings); nargs++;
    if (strlen(dlCartesianPlot->plotcom.xlabel) > 0) {
	XtSetArg(args[nargs],XtNxrtXTitle,dlCartesianPlot->plotcom.xlabel); nargs++;
    } else {
	XtSetArg(args[nargs],XtNxrtXTitle,NULL); nargs++;
    }
    if (strlen(dlCartesianPlot->plotcom.ylabel) > 0) {
	XtSetArg(args[nargs],XtNxrtYTitle,dlCartesianPlot->plotcom.ylabel); nargs++;
    } else {
	XtSetArg(args[nargs],XtNxrtYTitle,NULL); nargs++;
    }
    preferredHeight = MIN(dlCartesianPlot->object.width,
      dlCartesianPlot->object.height)/AXES_SCALE_FACTOR;
    bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
      preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
#if XRT_VERSION > 2
    XtSetArg(args[nargs],XtNxrtAxisFont,fontListTable[bestSize]); nargs++;
#else
    XtSetArg(args[nargs],XtNxrtAxisFont,fontTable[bestSize]->fid); nargs++;
#endif
    switch (dlCartesianPlot->style) {
    case POINT_PLOT:
    case LINE_PLOT:
	XtSetArg(args[nargs],XtNxrtType,XRT_PLOT); nargs++;
	if (validTraces > 1) {
	    XtSetArg(args[nargs],XtNxrtType2,XRT_PLOT); nargs++;
	}
	break;
    case FILL_UNDER_PLOT:
	XtSetArg(args[nargs],XtNxrtType,XRT_AREA); nargs++;
	if (validTraces > 1) {
	    XtSetArg(args[nargs],XtNxrtType2,XRT_AREA); nargs++;
	}
	break;
    }
    if (validTraces > 1) {
	XtSetArg(args[nargs],XtNxrtY2AxisShow,TRUE); nargs++;
    }

  /* X Axis Style */
    switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].axisStyle) {
    case LINEAR_AXIS:
	XtSetArg(args[nargs],XtNxrtXAnnotationMethod,XRT_ANNO_VALUES); nargs++;
	XtSetArg(args[nargs],XtNxrtXAxisLogarithmic,False); nargs++;
	break;
    case LOG10_AXIS:
	XtSetArg(args[nargs],XtNxrtXAnnotationMethod,XRT_ANNO_VALUES); nargs++;
	XtSetArg(args[nargs],XtNxrtXAxisLogarithmic,True); nargs++;
	break;
    case TIME_AXIS: {
	XtSetArg(args[nargs],XtNxrtXAxisLogarithmic,False); nargs++;
	XtSetArg(args[nargs],XtNxrtXAnnotationMethod,XRT_ANNO_TIME_LABELS); nargs++;
	XtSetArg(args[nargs],XtNxrtTimeBase,time900101); nargs++;
	XtSetArg(args[nargs],XtNxrtTimeUnit,XRT_TMUNIT_SECONDS); nargs++;
	XtSetArg(args[nargs],XtNxrtTimeFormatUseDefault,False); nargs++;
	XtSetArg(args[nargs],XtNxrtTimeFormat,
	  cpTimeFormatString[dlCartesianPlot->axis[0].timeFormat
	    -FIRST_CP_TIME_FORMAT]); nargs++;
	if(pcp) pcp->timeScale = True;
    }
    break;
    default:
	medmPrintf(1,"\nCpCreateCartesianPlot: Unknown X axis style\n");
	break;
    }

  /* X Axis Range */
    switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle) {
    case CHANNEL_RANGE:		/* handle as default until connected */
    case AUTO_SCALE_RANGE:
	XtSetArg(args[nargs],XtNxrtXNumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtXTickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtXPrecisionUseDefault,True); nargs++;
	break;
    case USER_SPECIFIED_RANGE:
	minF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].minRange;
	maxF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].maxRange;
	tickF.fval = (maxF.fval - minF.fval)/4.0;
	XtSetArg(args[nargs],XtNxrtXMin,minF.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXMax,maxF.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXTick,tickF.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtXNum,tickF.lval); nargs++;
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while (string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while (string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	XtSetArg(args[nargs],XtNxrtXPrecision,iPrec); nargs++;
	break;
    default:
	medmPrintf(1,"\nCpCreateCartesianPlot: Unknown X range style\n");
	break;
    }

  /* Y1 Axis Style */
    switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].axisStyle) {
    case LINEAR_AXIS:
	break;
    case LOG10_AXIS:
	XtSetArg(args[nargs],XtNxrtYAxisLogarithmic,True); nargs++;
	break;
    default:
	medmPrintf(1,"\nCpCreateCartesianPlot: Unknown Y1 axis style\n");
	break;
    }

  /* Y1 Axis Range */
    switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle) {
    case CHANNEL_RANGE:		/* handle as default until connected */
    case AUTO_SCALE_RANGE:
	XtSetArg(args[nargs],XtNxrtYNumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtYTickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtYPrecisionUseDefault,True); nargs++;
	break;
    case USER_SPECIFIED_RANGE:
	minF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].minRange;
	maxF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange;
	tickF.fval = (maxF.fval - minF.fval)/4.0;
	XtSetArg(args[nargs],XtNxrtYMin,minF.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYMax,maxF.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYTick,tickF.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtYNum,tickF.lval); nargs++;
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while (string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while (string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	XtSetArg(args[nargs],XtNxrtYPrecision,iPrec); nargs++;
	break;
    default:
	medmPrintf(1,"\nCpCreateCartesianPlot: Unknown Y1 range style\n");
	break;
    }

  /* Y2 Axis Style */
    switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].axisStyle) {
    case LINEAR_AXIS:
	break;
    case LOG10_AXIS:
	XtSetArg(args[nargs],XtNxrtY2AxisLogarithmic,True); nargs++;
	break;
    default:
	medmPrintf(1,
	  "\nCpCreateCartesianPlot: Unknown Y2 axis style\n");
	break;
    }

  /* Y2 Axis Range */
    switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle) {
    case CHANNEL_RANGE:		/* handle as default until connected */
    case AUTO_SCALE_RANGE:
	XtSetArg(args[nargs],XtNxrtY2NumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtY2TickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNxrtY2PrecisionUseDefault,True); nargs++;
	break;
    case USER_SPECIFIED_RANGE:
	minF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].minRange;
	maxF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].maxRange;
	tickF.fval = (maxF.fval - minF.fval)/4.0;
	XtSetArg(args[nargs],XtNxrtY2Min,minF.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Max,maxF.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Tick,tickF.lval); nargs++;
	XtSetArg(args[nargs],XtNxrtY2Num,tickF.lval); nargs++;
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while (string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while (string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	XtSetArg(args[nargs],XtNxrtY2Precision,iPrec); nargs++;
	break;
    default:
	medmPrintf(1,
	  "\nCpCreateCartesianPlot: Unknown Y2 range style\n");
	break;
    }

  /* Don't show outlines in filled plots */
    XtSetArg(args[nargs], XtNxrtGraphShowOutlines, False); nargs++;

  /* Set miscellaneous args */
    XtSetArg(args[nargs],XmNtraversalOn,False); nargs++;
    XtSetArg(args[nargs], XtNxrtDoubleBuffer, True); nargs++;

  /* Add pointer to MedmCartesianPlot struct as userData to widget */
    XtSetArg(args[nargs], XmNuserData, (XtPointer)pcp); nargs++;

  /* Create the widget */
    w = XtCreateWidget("cartesianPlot", xtXrtGraphWidgetClass,
      displayInfo->drawingArea, args, nargs);

  /* Add destroy callback for property editor */
#if XRT_VERSION > 2
#ifdef XRT_EXTENSIONS
    XtAddCallback(w,XmNdestroyCallback,
      (XtCallbackProc)destroyXrtPropertyEditor, NULL);
#endif
#endif

    return w;
}

#if 1
/* Set DEBUG_CARTESIAN_PLOT in eventHandlers.c to do this with Btn3 */
void dumpCartesianPlot(Widget w)
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
    XtGetValues(w,args,n);

    print("\nXRT/Graph Widget (%x)\n");
    print("  width: %d\n",width);
    print("  height: %d\n",height);
    print("  highlightThickness: %d\n",highlightThickness);
    print("  shadowThickness: %d\n",shadowThickness);
    print("  borderWidth: %d\n",borderWidth);
    print("  graphBorderWidth: %d\n",graphBorderWidth);
    print("  graphWidth: %d\n",graphWidth);
    print("  graphHeight: %d\n",graphHeight);
    print("  headerBorderWidth: %d\n",headerBorderWidth);
    print("  headerWidth: %d\n",headerWidth);
    print("  headerHeight: %d\n",headerHeight);
    print("  footerWidth: %d\n",footerBorderWidth);
    print("  footerWidth: %d\n",footerWidth);
    print("  footerHeight: %d\n",footerHeight);
    print("  legendBorderWidth: %d\n",legendBorderWidth);
    print("  legendWidth: %d\n",legendWidth);
    print("  legendHeight: %d\n",legendHeight);
    print("  unitType: %d (PIXELS %d, MM %d, IN %d, PTS %d, FONT %d)\n",unitType,
      XmPIXELS,Xm100TH_MILLIMETERS,Xm1000TH_INCHES,Xm100TH_POINTS,Xm100TH_FONT_UNITS);
    print("timeBase: %d\n",timeBase);
}

int CpDataSetPointsUsed(Widget w, CpDataHandle hData, int set, int npoints)
{
    if(npoints > 0) {
	XrtDataSetDisplay(hData, set, XRT_DISPLAY_SHOW);
    } else {
	XrtDataSetDisplay(hData, set, XRT_DISPLAY_HIDE);
    }
#if DEBUG_COUNT
    print("CpDataSetPointsUsed: hData=%p set=%d npoints=%d\n",
      hData,set,npoints);
#endif
    return XrtDataSetLastPoint(hData, set, MAX(npoints-1,0));
}

int CpDataGetPointsUsed(CpDataHandle hData, int set)
{
    int retVal;
    XrtDisplay display = XrtDataGetDisplay(hData, set);
    if(display == XRT_DISPLAY_HIDE) {
	retVal=0;
    }else {
	retVal = XrtDataGetLastPoint(hData, set) + 1;
    }
#if DEBUG_COUNT
    print("CpDataGetPointsUsed: hData=%p set=%d retVal=%d\n",
      hData,set,retVal);
#endif
    return retVal;
}

#endif
