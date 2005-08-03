/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/* Routines used to implement the Cartesian Plot using
 * Jpt (JeffersonLab Plotting Toolkit) */

/* KE: Note that MEDM uses the union XcVType (of a float and a long) to convert
 *   float values to the values needed for JPT float resources.  This currently
 *   gives the same results as using the JPT recommended PlotFloatToArg(), but
 *   probably isn't guaranteed */

#define DEBUG_CARTESIAN_PLOT 0
#define DEBUG_JPT 0
#define DEBUG_JPTA 0

#define MAX(a,b)  ((a)>(b)?(a):(b))

#include "medm.h"
#include "medmJpt.h"
#include "medmCartesianPlot.h"

static CpDataHandle hcpNullData = (CpDataHandle)0;

/* Function prototypes */

void CpDataDeleteCurves(Widget w, CpDataHandle hData) {
  /* Not necessary */
}

int CpDataGetPointsUsed(CpDataHandle hData, int set) {
    return (hData->g.data[set].npoints);
}
int CpDataSetPointsUsed(Widget w, CpDataHandle hData, int set, int point) {
  /* w is unused */
    hData->g.data[set].npoints = MAX(point,0);
    return 1;
}

void CpGetAxisInfo(Widget w,
  XtPointer *userData, Boolean *xAxisIsTime, char **timeFormat,
  Boolean *xAxisIsLog, Boolean *yAxisIsLog, Boolean *y2AxisIsLog,
  Boolean *xAxisIsAuto, Boolean *yAxisIsAuto, Boolean *y2AxisIsAuto,
  XcVType *xMaxF, XcVType *yMaxF, XcVType *y2MaxF,
  XcVType *xMinF, XcVType *yMinF, XcVType *y2MinF)
{
    PlotAnnoMethod xAnnoMethod;
    int xMin, yMin, y2Min, xMax, yMax, y2Max;
    Arg args[15];
    int nargs;

#if DEBUG_JPTA
    print("CpGetAxisInfo: Before XtGetValues\n");
#endif
    nargs=0;
    XtSetArg(args[nargs],XmNuserData, userData); nargs++;
    XtSetArg(args[nargs],XtNplotXAnnotationMethod, &xAnnoMethod); nargs++;
    XtSetArg(args[nargs],XtNplotXAxisLogarithmic, xAxisIsLog); nargs++;
    XtSetArg(args[nargs],XtNplotYAxisLogarithmic, yAxisIsLog); nargs++;
    XtSetArg(args[nargs],XtNplotY2AxisLogarithmic, y2AxisIsLog); nargs++;
    XtSetArg(args[nargs],XtNplotXMin, &xMin); nargs++;
    XtSetArg(args[nargs],XtNplotYMin, &yMin); nargs++;
    XtSetArg(args[nargs],XtNplotY2Min, &y2Min); nargs++;
    XtSetArg(args[nargs],XtNplotXMax, &xMax); nargs++;
    XtSetArg(args[nargs],XtNplotYMax, &yMax); nargs++;
    XtSetArg(args[nargs],XtNplotY2Max, &y2Max); nargs++;
  /* Axis is auto if using default for (min and max) */
    XtSetArg(args[nargs],XtNplotXMinUseDefault, xAxisIsAuto); nargs++;
    XtSetArg(args[nargs],XtNplotYMinUseDefault, yAxisIsAuto); nargs++;
    XtSetArg(args[nargs],XtNplotY2MinUseDefault, y2AxisIsAuto); nargs++;
    XtSetArg(args[nargs],XtNplotTimeFormat, timeFormat); nargs++;
    XtGetValues(w,args,nargs);
#if DEBUG_JPTA
    print("CpGetAxisInfo: After XtGetValues nargs=%d\n",nargs);
#endif

    *xAxisIsTime = (xAnnoMethod == PLOT_ANNO_TIME_LABELS)?True:False;
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
	XtSetArg(args[nargs],XtNplotXMax,&max); nargs++;
	XtSetArg(args[nargs],XtNplotXMin,&min); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNplotYMax,&max); nargs++;
	XtSetArg(args[nargs],XtNplotYMin,&min); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNplotY2Max,&max); nargs++;
	XtSetArg(args[nargs],XtNplotY2Min,&min); nargs++;
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
    PlotDataStyle myds;

  /* Convert color to a color name */
    sprintf(rgb,"#%2.2x%2.2x%2.2x", color.red>>8,
      color.green>>8, color.blue>>8);

  /* Zero the PlotDataStyle struct (should not be necessary, but is consistent) */
    memset(&myds,0,sizeof(PlotDataStyle));

  /* Fill in the PlotDataStyle struct */
    if(lineType == CP_LINE_NONE) myds.lpat = AtLineNONE;
    else myds.lpat = AtLineSOLID;
    if(fillType == CP_LINE_NONE) myds.fpat = PLOT_FPAT_NONE;
    else myds.fpat = PLOT_FPAT_SOLID;
    myds.color = rgb;
    myds.width = 1;
    myds.point = AtMarkDOT;
    myds.pcolor = rgb;
    myds.psize = pointSize;

#if DEBUG_JPT
    print("CpSetAxisStyle: w=%x trace=%d lineType=%d fillType=%d pointSize=%d\n",
      w,trace,lineType,fillType,pointSize);
    print("  rgb=%s red=%hx green=%hx blue=%hx pixel=%lx\n",
      rgb,color.red,color.green,color.blue,color.pixel);
#endif

  /* First trace is y1 axis, others are y2 axis */
    if(trace == 0) {
	myds.point = AtMarkDOT;
	PlotSetNthDataStyle(w, trace, &myds);
    } else {
	switch(trace%8) {
	case 0:
	    myds.point = (AtPlotMarkType)(AtMarkDOT);
	    break;
	case 1:
	    myds.point = (AtPlotMarkType)(AtMarkRECTANGLE);
	    break;
	case 2:
	    myds.point = (AtPlotMarkType)(AtMarkTRIANGLE1);
	    break;
	case 3:
	    myds.point = (AtPlotMarkType)(AtMarkDIAMOND);
	    break;
	case 4:
	    myds.point = (AtPlotMarkType)(AtMarkSTAR);
	    break;
	case 5:
	    myds.point = (AtPlotMarkType)(AtMarkCROSS);
	    break;
	case 6:
	    myds.point = (AtPlotMarkType)(AtMarkCIRCLE);
	    break;
	case 7:
	    myds.point = (AtPlotMarkType)(AtMarkRECTANGLE);
	    break;
	default:
	    myds.point = (AtPlotMarkType)(AtMarkNONE);
	    break;
	}
	PlotSetNthDataStyle2(w, trace-1, &myds);
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
	XtSetArg(args[nargs],XtNplotXMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXMin,min.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXTick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXNum,num.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXPrecision,precision); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNplotYMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYMin,min.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYTick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYNum,num.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYPrecision,precision); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNplotY2Max,max.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2Min,min.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2Tick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2Num,num.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2Precision,precision); nargs++;
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
	XtSetArg(args[nargs],XtNplotXMaxUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotXMinUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotXTickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotXNumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotXPrecisionUseDefault,True); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNplotYMaxUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotYMinUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotYTickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotYNumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotYPrecisionUseDefault,True); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNplotY2MaxUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotY2MinUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotY2TickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotY2NumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotY2PrecisionUseDefault,True); nargs++;
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
	XtSetArg(args[nargs],XtNplotXMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXMin,min.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXTickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotXNumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotXPrecisionUseDefault,True); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNplotYMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYMin,min.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYTickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotYNumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotYPrecisionUseDefault,True); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNplotY2Max,max.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2Min,min.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2TickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotY2NumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotY2PrecisionUseDefault,True); nargs++;
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
	XtSetArg(args[nargs],XtNplotXAnnotationMethod,PLOT_ANNO_VALUES); nargs++;
	XtSetArg(args[nargs],XtNplotXAxisLogarithmic,False); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNplotYAxisLogarithmic,False); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNplotY2AxisLogarithmic,False); nargs++;
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
	XtSetArg(args[nargs],XtNplotXAnnotationMethod,PLOT_ANNO_VALUES); nargs++;
	XtSetArg(args[nargs],XtNplotXAxisLogarithmic,True); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNplotYAxisLogarithmic,True); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNplotY2AxisLogarithmic,True); nargs++;
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
	XtSetArg(args[nargs],XtNplotXMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXMin,min.lval); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNplotYMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYMin,min.lval); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNplotY2Max,max.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2Min,min.lval); nargs++;
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
	XtSetArg(args[nargs],XtNplotXMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXTick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXNum,num.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXPrecision,precision); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNplotYMax,max.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYTick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYNum,num.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYPrecision,precision); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNplotY2Max,max.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2Tick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2Num,num.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2Precision,precision); nargs++;
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
	XtSetArg(args[nargs],XtNplotXMin,min.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXTick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXNum,num.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXPrecision,precision); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNplotYMin,min.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYTick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYNum,num.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYPrecision,precision); nargs++;
	break;
    case CP_Y2:
	XtSetArg(args[nargs],XtNplotY2Min,min.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2Tick,tick.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2Num,num.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2Precision,precision); nargs++;
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
	XtSetArg(args[nargs],XtNplotXAxisLogarithmic,False); nargs++;
	XtSetArg(args[nargs],XtNplotXAnnotationMethod,PLOT_ANNO_TIME_LABELS); nargs++;
	XtSetArg(args[nargs],XtNplotTimeBase,base); nargs++;
	XtSetArg(args[nargs],XtNplotTimeUnit,PLOT_TMUNIT_SECONDS); nargs++;
	XtSetArg(args[nargs],XtNplotTimeFormatUseDefault,False); nargs++;
	XtSetArg(args[nargs],XtNplotTimeFormat,format); nargs++;
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
	XtSetArg(args[nargs],XtNplotData,hData); nargs++;
    } else {
	XtSetArg(args[nargs],XtNplotData2,hData); nargs++;
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
	CpDataSetPointsUsed(w,hcpNullData,0,0);
	CpDataSetXElement(hcpNullData,0,0,0.0);
	CpDataSetYElement(hcpNullData,0,0,0.0);
    }

    nargs=0;
    if(axis == CP_Y) {
	XtSetArg(args[nargs],XtNplotData,hcpNullData); nargs++;
    } else {
	XtSetArg(args[nargs],XtNplotData2,hcpNullData); nargs++;
    }
    XtSetValues(w,args,nargs);
}

void CpSetTimeBase(Widget w, time_t base)
{
    Arg args[1];
    int nargs;

    nargs=0;
    XtSetArg(args[nargs],XtNplotTimeBase,base); nargs++;
    XtSetValues(w,args,nargs);
}

void CpSetTimeFormat(Widget w, char *format)
{
    Arg args[15];
    int nargs;

    nargs=0;
    XtSetArg(args[nargs],XtNplotTimeFormat,format); nargs++;
    XtSetValues(w,args,nargs);
}

void CpUpdateWidget(Widget w, int full)
  /* Not necessary */
{
}

Widget CpCreateCartesianPlot(DisplayInfo *displayInfo,
  DlCartesianPlot *dlCartesianPlot, MedmCartesianPlot *pcp)
{
    Arg args[47];
    int nargs;
    XColor xColors[2];
    char rgb[2][16], string[24];
    int usedHeight, usedCharWidth, bestSize, preferredHeight;
    char *headerStrings[2];
    int k, iPrec;
    XcVType minF, maxF, tickF;
    Widget w;
    int validTraces = pcp ? pcp->nTraces : 0;

  /* Set widget args from the dlCartesianPlot structure */
    nargs = 0;
    XtSetArg(args[nargs],XmNx,(Position)dlCartesianPlot->object.x); nargs++;
    XtSetArg(args[nargs],XmNy,(Position)dlCartesianPlot->object.y); nargs++;
    XtSetArg(args[nargs],XmNwidth,(Dimension)dlCartesianPlot->object.width); nargs++;
    XtSetArg(args[nargs],XmNborderWidth,0); nargs++;
    XtSetArg(args[nargs],XmNheight,(Dimension)dlCartesianPlot->object.height); nargs++;
    XtSetArg(args[nargs],XmNhighlightThickness,0); nargs++;

  /* JPT uses strings for color names */
    xColors[0].pixel = displayInfo->colormap[dlCartesianPlot->plotcom.clr];
    xColors[1].pixel = displayInfo->colormap[dlCartesianPlot->plotcom.bclr];
    XQueryColors(display,cmap,xColors,2);
    sprintf(rgb[0],"#%2.2x%2.2x%2.2x",xColors[0].red>>8, xColors[0].green>>8,
      xColors[0].blue>>8);
    sprintf(rgb[1],"#%2.2x%2.2x%2.2x",xColors[1].red>>8, xColors[1].green>>8,
      xColors[1].blue>>8);
    XtSetArg(args[nargs],XtNplotGraphForegroundColor,rgb[0]); nargs++;
    XtSetArg(args[nargs],XtNplotGraphBackgroundColor,rgb[1]); nargs++;
    XtSetArg(args[nargs],XtNplotForegroundColor,rgb[0]); nargs++;
    XtSetArg(args[nargs],XtNplotBackgroundColor,rgb[1]); nargs++;
    preferredHeight = MIN(dlCartesianPlot->object.width,
      dlCartesianPlot->object.height)/TITLE_SCALE_FACTOR;
    bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
      preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
    XtSetArg(args[nargs],XtNplotHeaderFont,fontTable[bestSize]->fid); nargs++;
    if (strlen(dlCartesianPlot->plotcom.title) > 0) {
	headerStrings[0] = dlCartesianPlot->plotcom.title;
    } else {
	headerStrings[0] = NULL;
    }
    headerStrings[1] = NULL;
    XtSetArg(args[nargs],XtNplotHeaderStrings,headerStrings); nargs++;
    if (strlen(dlCartesianPlot->plotcom.xlabel) > 0) {
	XtSetArg(args[nargs],XtNplotXTitle,dlCartesianPlot->plotcom.xlabel); nargs++;
    } else {
	XtSetArg(args[nargs],XtNplotXTitle,NULL); nargs++;
    }
    if (strlen(dlCartesianPlot->plotcom.ylabel) > 0) {
	XtSetArg(args[nargs],XtNplotYTitle,dlCartesianPlot->plotcom.ylabel); nargs++;
    } else {
	XtSetArg(args[nargs],XtNplotYTitle,NULL); nargs++;
    }
    preferredHeight = MIN(dlCartesianPlot->object.width,
      dlCartesianPlot->object.height)/AXES_SCALE_FACTOR;
    bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
      preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
    XtSetArg(args[nargs],XtNplotAxisFont,fontTable[bestSize]->fid); nargs++;
    switch (dlCartesianPlot->style) {
    case POINT_PLOT:
    case LINE_PLOT:
	XtSetArg(args[nargs],XtNplotType,PLOT_PLOT); nargs++;
	if (validTraces > 1) {
	    XtSetArg(args[nargs],XtNplotType2,PLOT_PLOT); nargs++;
	}
	break;
    case FILL_UNDER_PLOT:
	XtSetArg(args[nargs],XtNplotType,PLOT_AREA); nargs++;
	if (validTraces > 1) {
	    XtSetArg(args[nargs],XtNplotType2,PLOT_AREA); nargs++;
	}
	break;
    }
    if (validTraces > 1) {
	XtSetArg(args[nargs],XtNplotY2AxisShow,TRUE); nargs++;
    }

  /* X Axis Style */
    switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].axisStyle) {
    case LINEAR_AXIS:
	XtSetArg(args[nargs],XtNplotXAnnotationMethod,PLOT_ANNO_VALUES); nargs++;
	XtSetArg(args[nargs],XtNplotXAxisLogarithmic,False); nargs++;
	break;
    case LOG10_AXIS:
	XtSetArg(args[nargs],XtNplotXAnnotationMethod,PLOT_ANNO_VALUES); nargs++;
	XtSetArg(args[nargs],XtNplotXAxisLogarithmic,True); nargs++;
	break;
    case TIME_AXIS: {
	XtSetArg(args[nargs],XtNplotXAxisLogarithmic,False); nargs++;
	XtSetArg(args[nargs],XtNplotXAnnotationMethod,PLOT_ANNO_TIME_LABELS); nargs++;
	XtSetArg(args[nargs],XtNplotTimeBase,time900101); nargs++;
	XtSetArg(args[nargs],XtNplotTimeUnit,PLOT_TMUNIT_SECONDS); nargs++;
	XtSetArg(args[nargs],XtNplotTimeFormatUseDefault,False); nargs++;
	XtSetArg(args[nargs],XtNplotTimeFormat,
	  cpTimeFormatString[dlCartesianPlot->axis[0].timeFormat
	    -FIRST_CP_TIME_FORMAT]); nargs++;
	if(pcp) pcp->timeScale = True;
    }
    break;
    default:
	medmPrintf(1,"\nCpCreateRunTimeCartesianPlot: Unknown X axis style\n");
	break;
    }

  /* X Axis Range */
    switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle) {
    case CHANNEL_RANGE:		/* handle as default until connected */
    case AUTO_SCALE_RANGE:
	XtSetArg(args[nargs],XtNplotXNumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotXTickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotXPrecisionUseDefault,True); nargs++;
	break;
    case USER_SPECIFIED_RANGE:
	minF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].minRange;
	maxF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].maxRange;
	tickF.fval = (float)((maxF.fval - minF.fval)/4.0);
	XtSetArg(args[nargs],XtNplotXMin,minF.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXMax,maxF.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXTick,tickF.lval); nargs++;
	XtSetArg(args[nargs],XtNplotXNum,tickF.lval); nargs++;
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while (string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while (string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	XtSetArg(args[nargs],XtNplotXPrecision,iPrec); nargs++;
	break;
    default:
	medmPrintf(1,"\nCpCreateRunTimeCartesianPlot: Unknown X range style\n");
	break;
    }

  /* Y1 Axis Style */
    switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].axisStyle) {
    case LINEAR_AXIS:
	break;
    case LOG10_AXIS:
	XtSetArg(args[nargs],XtNplotYAxisLogarithmic,True); nargs++;
	break;
    default:
	medmPrintf(1,"\nCpCreateRunTimeCartesianPlot: Unknown Y1 axis style\n");
	break;
    }

  /* Y1 Axis Range */
    switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle) {
    case CHANNEL_RANGE:		/* handle as default until connected */
    case AUTO_SCALE_RANGE:
	XtSetArg(args[nargs],XtNplotYNumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotYTickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotYPrecisionUseDefault,True); nargs++;
	break;
    case USER_SPECIFIED_RANGE:
	minF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].minRange;
	maxF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange;
	tickF.fval = (float)((maxF.fval - minF.fval)/4.0);
	XtSetArg(args[nargs],XtNplotYMin,minF.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYMax,maxF.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYTick,tickF.lval); nargs++;
	XtSetArg(args[nargs],XtNplotYNum,tickF.lval); nargs++;
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while (string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while (string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	XtSetArg(args[nargs],XtNplotYPrecision,iPrec); nargs++;
	break;
    default:
	medmPrintf(1,"\nCpCreateRunTimeCartesianPlot: Unknown Y1 range style\n");
	break;
    }

  /* Y2 Axis Style */
    switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].axisStyle) {
    case LINEAR_AXIS:
	break;
    case LOG10_AXIS:
	XtSetArg(args[nargs],XtNplotY2AxisLogarithmic,True); nargs++;
	break;
    default:
	medmPrintf(1,
	  "\nCpCreateRunTimeCartesianPlot: Unknown Y2 axis style\n");
	break;
    }

  /* Y2 Axis Range */
    switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle) {
    case CHANNEL_RANGE:		/* handle as default until connected */
    case AUTO_SCALE_RANGE:
	XtSetArg(args[nargs],XtNplotY2NumUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotY2TickUseDefault,True); nargs++;
	XtSetArg(args[nargs],XtNplotY2PrecisionUseDefault,True); nargs++;
	break;
    case USER_SPECIFIED_RANGE:
	minF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].minRange;
	maxF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].maxRange;
	tickF.fval = (float)((maxF.fval - minF.fval)/4.0);
	XtSetArg(args[nargs],XtNplotY2Min,minF.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2Max,maxF.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2Tick,tickF.lval); nargs++;
	XtSetArg(args[nargs],XtNplotY2Num,tickF.lval); nargs++;
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while (string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while (string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	XtSetArg(args[nargs],XtNplotY2Precision,iPrec); nargs++;
	break;
    default:
	medmPrintf(1,
	  "\nCpCreateRunTimeCartesianPlot: Unknown Y2 range style\n");
	break;
    }

  /* Don't show outlines in filled plots */
    XtSetArg(args[nargs], XtNplotGraphShowOutlines, False); nargs++;

  /* Set miscellaneous args */
    XtSetArg(args[nargs],XmNtraversalOn,False); nargs++;
    XtSetArg(args[nargs], XtNplotDoubleBuffer, True); nargs++;

  /* Add pointer to CartesianPlot struct as userData to widget */
    XtSetArg(args[nargs], XmNuserData, (XtPointer)pcp); nargs++;

  /* Create the widget */
    w = XtCreateWidget("cartesianPlot", atPlotterWidgetClass,
      displayInfo->drawingArea, args, nargs);

    return(w);
}

#if 1
/* Set DEBUG_CARTESIAN_PLOT in eventHandlers.c to do this with Btn3 */
void dumpCartesianPlot(Widget w)
{
    Arg args[20];
    int n=0;

    Dimension borderWidth;
    Dimension shadowThickness;
    Dimension highlightThickness;
    int headerBorderWidth;
    int headerHeight;
    int headerWidth;
    int graphBorderWidth;
    Dimension height;
    Dimension width;
    int legendBorderWidth;
    Dimension legendWidth;
    time_t timeBase;

    XtSetArg(args[n],XtNplotLegendWidth,&legendWidth); n++;
    /*XtSetArg(args[n],XtNplotLegendHeight,&legendHeight); n++;*/
    XtSetArg(args[n],XtNplotLegendBorderWidth,&legendBorderWidth); n++;
    XtSetArg(args[n],XmNwidth,&width); n++;
    XtSetArg(args[n],XmNheight,&height); n++;
    XtSetArg(args[n],XtNplotGraphBorderWidth,&graphBorderWidth); n++;
    /*XtSetArg(args[n],XtNplotGraphWidth,&graphWidth); n++;
    XtSetArg(args[n],XtNplotGraphHeight,&graphHeight); n++;*/
    XtSetArg(args[n],XtNplotHeaderWidth,&headerWidth); n++;
    XtSetArg(args[n],XtNplotHeaderHeight,&headerHeight); n++;
    XtSetArg(args[n],XtNplotHeaderBorderWidth,&headerBorderWidth); n++;
    XtSetArg(args[n],XmNhighlightThickness,&highlightThickness); n++;
    XtSetArg(args[n],XmNshadowThickness,&shadowThickness); n++;
    XtSetArg(args[n],XmNborderWidth,&borderWidth); n++;
    /*XtSetArg(args[n],XtNplotFooterBorderWidth,&footerBorderWidth); n++;
    XtSetArg(args[n],XtNplotFooterHeight,&footerHeight); n++;
    XtSetArg(args[n],XtNplotFooterWidth,&footerWidth); n++;
    XtSetArg(args[n],XtNplotFooterBorderWidth,&footerBorderWidth); n++;
    XtSetArg(args[n],XmNunitType,&unitType); n++;*/
    XtSetArg(args[n],XtNplotTimeBase,&timeBase); n++;
    XtGetValues(w,args,n);

    print("\nJPT Widget (%x)\n");
    print("  width: %d\n",width);
    print("  height: %d\n",height);
    print("  highlightThickness: %d\n",highlightThickness);
    print("  shadowThickness: %d\n",shadowThickness);
    print("  borderWidth: %d\n",borderWidth);
    print("  graphBorderWidth: %d\n",graphBorderWidth);
    /*print("  graphWidth: %d\n",graphWidth);
    print("  graphHeight: %d\n",graphHeight);*/
    print("  headerBorderWidth: %d\n",headerBorderWidth);
    print("  headerWidth: %d\n",headerWidth);
    print("  headerHeight: %d\n",headerHeight);
    /*print("  footerWidth: %d\n",footerBorderWidth);
    print("  footerWidth: %d\n",footerWidth);
    print("  footerHeight: %d\n",footerHeight);*/
    print("  legendBorderWidth: %d\n",legendBorderWidth);
    print("  legendWidth: %d\n",legendWidth);
    /*print("  legendHeight: %d\n",legendHeight);*/
    /*print("  unitType: %d (PIXELS %d, MM %d, IN %d, PTS %d, FONT %d)\n",unitType,
      XmPIXELS,Xm100TH_MILLIMETERS,Xm1000TH_INCHES,Xm100TH_POINTS,Xm100TH_FONT_UNITS);*/
    print("timeBase: %d\n",timeBase);
}
#endif
