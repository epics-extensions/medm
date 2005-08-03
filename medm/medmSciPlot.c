/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/

/* Routines used to implement the Cartesian Plot using SciPlot */

/* KE: Note that MEDM uses the union XcVType (of a float and a long) to convert
 *   float values to the values needed for XRT float resources.  This is not
 *   needed for SciPlot, but the mechanism is still used */

#define DEBUG_CARTESIAN_PLOT 0
#define DEBUG_USER_DATA 0
#define DEBUG_FONTS 0
#define DEBUG_MEMORY 0

#define SPECIFY_RESOURCES 0

#define MAX(a,b)  ((a)>(b)?(a):(b))
#define MIN_FONT_HEIGHT 8

#include "medm.h"
#include "medmSciPlot.h"
#include "medmCartesianPlot.h"

/* Static Function prototypes */

/* Implementations */

CpDataHandle CpDataCreate(Widget w, CpDataType type, int nsets, int npoints)
{
    CpDataHandle hData;
    CpDataSet *ds;
    int i;

  /* Allocate structure  */
    hData = (CpDataHandle)calloc(1,sizeof(CpData));
    if(!hData) return NULL;

  /* Initialize */
    hData->nsets = nsets;
    hData->data = NULL;
    if(nsets <= 0) return hData;

  /* Allocate data sets */
    hData->data = (CpDataSet *)calloc(nsets, sizeof(CpDataSet));
    if(!hData) {
	CpDataDestroy(hData);
	return NULL;
    }

  /* Initialize data sets */
    for(i=0; i < nsets; i++) {
	ds = hData->data+i;
	ds->npoints = npoints;
	ds->pointsUsed = MAX(npoints,0);
	ds->listid = INVALID_LISTID;
	ds->xp = NULL;
	ds->yp = NULL;
      /* Allocate x and y arrays */
	if(npoints > 0) {
	    ds->xp = (float *)calloc(npoints,sizeof(float));
	    ds->yp = (float *)calloc(npoints,sizeof(float));
	    if(!ds->xp || !ds->yp) {
		CpDataDestroy(hData);
		return NULL;
	    }
	}
      /* Determine a listid now to allow setting axis parameters, etc.  */
	if(w) {
	    ds->listid = SciPlotListCreateFloat(w, ds->npoints,
	      ds->xp, ds->yp, "");
	}
    }

#if DEBUG_MEMORY
    if(hData) {
	print("CpDataCreate: hData=%x nsets=%d data=%x\n",hData,hData->nsets,hData->data);
	for(i=0; i < hData->nsets; i++) {
	    ds = hData->data+i;
	    print("  Set %d: ds=%x npoints=%d pointsUsed=%d listid=%d xp=%x yp=%x\n",
	      i,ds,ds->npoints,ds->pointsUsed,ds->listid,ds->xp,ds->yp);
	}
    } else {
	print("CpDataCreate: hData=NULL\n");
    }
#endif

    return hData;
}

void CpDataDeleteCurves(Widget w, CpDataHandle hData)
{
    int i, listid, nsets;

  /* Return if handle is NULL */
    if(!hData) return;

  /* Loop over sets */
    nsets = hData->nsets;
    for(i=0; i < nsets; i++) {
	listid = hData->data[i].listid;
	if(listid != INVALID_LISTID) {
	    SciPlotListDelete(w, listid);
	    hData->data[i].listid=INVALID_LISTID;
	}
    }
}

int CpDataGetPointsUsed(CpDataHandle hData, int set) {
    return hData->data[set].pointsUsed;
}

double CpDataGetXElement(CpDataHandle hData, int set, int point) {
    return hData->data[set].xp[point];
}

double CpDataGetYElement(CpDataHandle hData, int set, int point) {
    return hData->data[set].yp[point];
}

void CpDataDestroy(CpDataHandle hData)
{
    CpDataSet *ds;
    int i;

#if DEBUG_MEMORY
    if(hData) {
	print("CpDataCreate: hData=%x nsets=%d data=%x\n",hData,hData->nsets,hData->data);
	for(i=0; i < hData->nsets; i++) {
	    ds = hData->data+i;
	    print("  Set %d: ds=%x npoints=%d pointsUsed=%d listid=%d xp=%x yp=%x\n",
	      i,ds,ds->npoints,ds->pointsUsed,ds->listid,ds->xp,ds->yp);
	}
    } else {
	print("CpDataCreate: hData=NULL\n");
    }
#endif

    if(hData) {
	if(hData->data) {
	    for(i=0; i < hData->nsets; i++) {
		ds = hData->data+i;
		if (ds->xp) free((char *)ds->xp);
		if (ds->yp) free((char *)ds->yp);
	    }
	    free((char *)hData->data);
	}
	free(hData);
	hData = NULL;
    }
}

int CpDataSetHole(CpDataHandle hData, double hole) {
  /* Do nothing, hole is always SCIPLOT_SKIP_VAL */
    return 1;
}

int CpDataSetPointsUsed(Widget w, CpDataHandle hData, int set, int point) {
    int listid;

    hData->data[set].pointsUsed = MAX(point,0);
    listid = hData->data[set].listid;
    SciPlotListUpdateFloat(w, listid, hData->data[set].pointsUsed,
      hData->data[set].xp, hData->data[set].yp);
    SciPlotUpdate(w);
    return 1;
}

int CpDataSetXElement(CpDataHandle hData, int set, int point, double x) {
    hData->data[set].xp[point] = (float)x;
    return 1;
}

int CpDataSetYElement(CpDataHandle hData, int set, int point, double y) {
    hData->data[set].yp[point] = (float)y;
    return 1;
}

void CpGetAxisInfo(Widget w,
  XtPointer *userData, Boolean *xAxisIsTime, char **timeFormat,
  Boolean *xAxisIsLog, Boolean *yAxisIsLog, Boolean *y2AxisIsLog,
  Boolean *xAxisIsAuto, Boolean *yAxisIsAuto, Boolean *y2AxisIsAuto,
  XcVType *xMaxF, XcVType *yMaxF, XcVType *y2MaxF,
  XcVType *xMinF, XcVType *yMinF, XcVType *y2MinF)
{
    float xMin, yMin, xMax, yMax;

  /* Get available information from SciPlot */
    SciPlotGetXAxisInfo(w, &xMin, &xMax, xAxisIsLog, xAxisIsAuto);
    SciPlotGetYAxisInfo(w, &yMin, &yMax, yAxisIsLog, yAxisIsAuto);

  /* Convert */
    xMinF->fval = xMin;
    yMinF->fval = yMin;
    y2MinF->fval = 0;         /* Not implemented */
    xMaxF->fval = xMax;
    yMaxF->fval = yMax;
    y2MaxF->fval = 0;         /* Not implemented */

  /* Get userData */
    XtVaGetValues(w, XmNuserData, userData, NULL);

  /* Define the rest */
    *xAxisIsTime = False;      /* Not implemented */
    *timeFormat = NULL;        /* Not implemented */
    *y2AxisIsLog = False;      /* Not implemented */
    *y2AxisIsAuto = True;      /* Not implemented */
}

void CpGetAxisMaxMin(Widget w, int axis, XcVType *maxF, XcVType *minF)
{
    float min, max;

    switch(axis) {
    case CP_X:
	SciPlotGetXScale(w, &min, &max);
	break;
    case CP_Y:
	SciPlotGetYScale(w, &min, &max);
	break;
    case CP_Y2:
      /* Not implemented */
	min=0.0;
	max=0.0;
	break;
    }

  /* Convert */
    minF->fval = min;
    maxF->fval = max;
}

void CpSetAxisStyle(Widget w, CpDataHandle hData, int trace, int lineType,
  int fillType, XColor color, int pointSize)
  /* Fill type is not supported */
{
    int listid, colorid, pointStyle, lineStyle, set;

  /* Return if handle is NULL */
    if(!hData) return;

  /* Determine which dataset (First trace is y1 axis, others are y2 axis))*/
    set = (trace)?trace-1:0;
    if(set >= hData->nsets) return;

  /* Return if an invalid id (list not defined yet) */
    listid = hData->data[set].listid;
    if(listid == INVALID_LISTID) return;

  /* Convert color to an index */
    colorid = SciPlotStoreAllocatedColor(w, color.pixel);

  /* Line style */
    if(lineType == CP_LINE_NONE) lineStyle = XtLINE_NONE;
    else lineStyle = XtLINE_SOLID;

  /* Point style (depends on trace not data set) */
    if(trace == 0) {
	pointStyle = XtMARKER_FCIRCLE;
    } else {
	switch(trace%8) {
	case 0:
	    pointStyle = XtMARKER_FCIRCLE;
	    break;
	case 1:
	    pointStyle = XtMARKER_FSQUARE;
	    pointSize = pointSize*3/4;
	    break;
	case 2:
	    pointStyle = XtMARKER_FUTRIANGLE;
	    break;
	case 3:
	    pointStyle = XtMARKER_FDIAMOND;
	    break;
	case 4:
	    pointStyle = XtMARKER_FDTRIANGLE;
	    break;
	case 5:
	    pointStyle = XtMARKER_FRTRIANGLE;;
	    break;
	case 6:
	    pointStyle = XtMARKER_FLTRIANGLE;;
	    break;
	case 7:
	    pointStyle = XtMARKER_FBOWTIE;
	    break;
	default:
	    pointStyle = XtMARKER_NONE;
	    break;
	}
    }

  /* Set the styles */
    SciPlotListSetStyle(w, listid, colorid, pointStyle, colorid, lineStyle);
    SciPlotListSetMarkerSize(w, listid, (float)(pointSize*2./3.));
    SciPlotUpdate(w);
}

void CpSetAxisAll(Widget w, int axis, XcVType max, XcVType min,
  XcVType tick, XcVType num, int precision)
  /* Ticks parameters cannot be changed */
{
    switch(axis) {
    case CP_X:
	SciPlotSetXUserScale(w, min.fval, max.fval);
	break;
    case CP_Y:
	SciPlotSetYUserScale(w, min.fval, max.fval);
	break;
    case CP_Y2:
      /* Not supported */
	break;
    }
    SciPlotUpdate(w);
}

void CpSetAxisAuto(Widget w, int axis)
{
    switch(axis) {
    case CP_X:
	SciPlotSetXAutoScale(w);
	break;
    case CP_Y:
	SciPlotSetYAutoScale(w);
	break;
    case CP_Y2:
      /* Not supported */
	break;
    }
    SciPlotUpdate(w);
}

void CpSetAxisChannel(Widget w, int axis, XcVType max, XcVType min)
{
    switch(axis) {
    case CP_X:
	SciPlotSetXUserScale(w, min.fval, max.fval);
	break;
    case CP_Y:
	SciPlotSetYUserScale(w, min.fval, max.fval);
	break;
    case CP_Y2:
      /* Not supported */
	break;
    }
    SciPlotUpdate(w);
}

void CpSetAxisLinear(Widget w, int axis)
{
    Arg args[1];
    int nargs;

    nargs=0;
    switch(axis) {
    case CP_X:
	XtSetArg(args[nargs],XtNxLog,False); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNyLog,False); nargs++;
	break;
    case CP_Y2:
      /* Not supported */
	break;
    }
    XtSetValues(w,args,nargs);

  /* Not supposed to be necessary */
    SciPlotUpdate(w);
}

void CpSetAxisLog(Widget w, int axis)
{
    Arg args[1];
    int nargs;

    nargs=0;
    switch(axis) {
    case CP_X:
	XtSetArg(args[nargs],XtNxLog,True); nargs++;
	break;
    case CP_Y:
	XtSetArg(args[nargs],XtNyLog,True); nargs++;
	break;
    case CP_Y2:
      /* Not supported */
	break;
    }
    XtSetValues(w,args,nargs);

  /* Not supposed to be necessary */
    SciPlotUpdate(w);
}

void CpSetAxisMaxMin(Widget w, int axis, XcVType max, XcVType min)
{
    switch(axis) {
    case CP_X:
	SciPlotSetXUserScale(w, min.fval, max.fval);
	break;
    case CP_Y:
	SciPlotSetYUserScale(w, min.fval, max.fval);
	break;
    case CP_Y2:
      /* Not supported */
	break;
    }
    SciPlotUpdate(w);
}

void CpSetAxisMax(Widget w, int axis, XcVType max, XcVType tick,
  XcVType num, int precision)
  /* Ticks parameters cannot be changed */
{
    switch(axis) {
    case CP_X:
	SciPlotSetXUserMax(w, max.fval);
	break;
    case CP_Y:
	SciPlotSetYUserMax(w, max.fval);
	break;
    case CP_Y2:
      /* Not supported */
	break;
    }
    SciPlotUpdate(w);
}

void CpSetAxisMin(Widget w, int axis, XcVType min, XcVType tick,
  XcVType num, int precision)
  /* Ticks parameters cannot be changed */
{
    switch(axis) {
    case CP_X:
	SciPlotSetXUserMin(w, min.fval);
	break;
    case CP_Y:
	SciPlotSetYUserMin(w, min.fval);
	break;
    case CP_Y2:
      /* Not supported */
	break;
    }
    SciPlotUpdate(w);
}

void CpSetAxisTime(Widget w, int axis, time_t base, char * format)
  /* Not supported */
{
}

void CpSetData(Widget w, int axis, CpDataHandle hData)
  /* Axis (CP_X, CP_Y, CP_Y2) doesn't matter */
{
    int i, listid, npoints, nsets;

  /* Return if handle is NULL */
    if(!hData) return;

  /* Loop over sets */
    nsets = hData->nsets;
    for(i=0; i < nsets; i++) {
	listid = hData->data[i].listid;
	npoints = hData->data[i].npoints;
#if 0
	if(npoints <= 0) continue;
#endif
	if(listid == INVALID_LISTID) {
	  /* Not set yet, set with npoints */
	    listid = SciPlotListCreateFloat(w, hData->data[i].npoints,
	      hData->data[i].xp, hData->data[i].yp, "");
	}
      /* Update with pointsUsed */
	SciPlotListUpdateFloat(w, listid, hData->data[i].pointsUsed,
	  hData->data[i].xp, hData->data[i].yp);
    }
  /* Don't do SciPlotUpdate here for efficiency */
}

void CpEraseData(Widget w, int axis, CpDataHandle hData)
  /* Axis (CP_X, CP_Y, CP_Y2) doesn't matter */
{
    int i, listid, npoints, nsets;

  /* Return if handle is NULL */
    if(!hData) return;

  /* Loop over sets */
    nsets = hData->nsets;
    for(i=0; i < nsets; i++) {
	listid = hData->data[i].listid;
	npoints = hData->data[i].npoints;
#if 0
	if(npoints <= 0) continue;
#endif
	if(listid == INVALID_LISTID) {
	  /* Not set yet, set with npoints */
	    listid = SciPlotListCreateFloat(w, hData->data[i].npoints,
	      hData->data[i].xp, hData->data[i].yp, "");
	}
      /* Update with zero */
	SciPlotListUpdateFloat(w, listid, 0,
	  hData->data[i].xp, hData->data[i].yp);
    }
  /* Don't do SciPlotUpdate here for efficiency */
}

void CpSetTimeBase(Widget w, time_t base)
  /* Not supported */
{
}

void CpUpdateWidget(Widget w, int full)
{
    if(full) {
	SciPlotUpdate(w);
    } else {
      /* Try data only, do full if data out of bounds */
	if(SciPlotQuickUpdate(w))
	  SciPlotUpdate(w);
    }
}

void CpSetTimeFormat(Widget w, char *format)
  /* Not supported */
{
}

Widget CpCreateCartesianPlot(DisplayInfo *displayInfo,
  DlCartesianPlot *dlCartesianPlot, MedmCartesianPlot *pcp)
{
    Arg args[75];     /* Count later */
    int nargs;

    int fgcolorid, bgcolorid;
    int preferredHeight;
    XcVType minF, maxF;
    Widget w;
#if 0
    int validTraces = pcp ? pcp->nTraces : 0;
#endif

  /* Set widget args from the dlCartesianPlot structure */
    nargs = 0;
    XtSetArg(args[nargs],XmNx,(Position)dlCartesianPlot->object.x); nargs++;
    XtSetArg(args[nargs],XmNy,(Position)dlCartesianPlot->object.y); nargs++;
    XtSetArg(args[nargs],XmNwidth,(Dimension)dlCartesianPlot->object.width); nargs++;
    XtSetArg(args[nargs],XmNheight,(Dimension)dlCartesianPlot->object.height); nargs++;
    XtSetArg(args[nargs],XmNshadowType,XmSHADOW_OUT); nargs++;
#if SPECIFY_RESOURCES
    XtSetArg(args[nargs],XmNborderWidth,0); nargs++;
    XtSetArg(args[nargs],XmNhighlightThickness,0); nargs++;
#endif
    preferredHeight = MIN(dlCartesianPlot->object.width,
      dlCartesianPlot->object.height)/TITLE_SCALE_FACTOR;
#if DEBUG_FONTS
    print("CpCreateCartesianPlot: Axis and Label Font\n"
      "  dlCartesianPlot->object.width=%d\n"
      "  dlCartesianPlot->object.height=%d\n"
      "  TITLE_SCALE_FACTOR=%d\n"
      "  preferredHeight=%d\n",
      dlCartesianPlot->object.width,dlCartesianPlot->object.height,
      TITLE_SCALE_FACTOR,preferredHeight);

#endif
    XtSetArg(args[nargs],XtNtitleFont,XtFONT_HELVETICA|
      MAX(preferredHeight,MIN_FONT_HEIGHT)); nargs++;
    if (strlen(dlCartesianPlot->plotcom.title) > 0) {
	XtSetArg(args[nargs],XtNplotTitle,dlCartesianPlot->plotcom.title); nargs++;
	XtSetArg(args[nargs],XtNshowTitle,True); nargs++;
    } else {
	XtSetArg(args[nargs],XtNshowTitle,False); nargs++;
    }
    if (strlen(dlCartesianPlot->plotcom.xlabel) > 0) {
	XtSetArg(args[nargs],XtNxLabel,dlCartesianPlot->plotcom.xlabel); nargs++;
	XtSetArg(args[nargs],XtNshowXLabel,True); nargs++;
    } else {
	XtSetArg(args[nargs],XtNshowXLabel,False); nargs++;
    }
    if (strlen(dlCartesianPlot->plotcom.ylabel) > 0) {
	XtSetArg(args[nargs],XtNyLabel,dlCartesianPlot->plotcom.ylabel); nargs++;
	XtSetArg(args[nargs],XtNshowYLabel,True); nargs++;
    } else {
	XtSetArg(args[nargs],XtNshowYLabel,False); nargs++;
    }
    preferredHeight = MIN(dlCartesianPlot->object.width,
      dlCartesianPlot->object.height)/AXES_SCALE_FACTOR;
#if DEBUG_FONTS
    print("CpCreateCartesianPlot: Axis and Label Font\n"
      "  dlCartesianPlot->object.width=%d\n"
      "  dlCartesianPlot->object.height=%d\n"
      "  AXES_SCALE_FACTOR=%d\n"
      "  preferredHeight=%d\n",
      dlCartesianPlot->object.width,dlCartesianPlot->object.height,
      AXES_SCALE_FACTOR,preferredHeight);

#endif
    XtSetArg(args[nargs],XtNaxisFont,XtFONT_HELVETICA|
      MAX(preferredHeight,MIN_FONT_HEIGHT)); nargs++;
    XtSetArg(args[nargs],XtNlabelFont,XtFONT_HELVETICA|
      MAX(preferredHeight,MIN_FONT_HEIGHT)); nargs++;

  /* SciPlot-specific */
#if SPECIFY_RESOURCES
    XtSetArg(args[nargs],XtNtitleMargin,5); nargs++;
    XtSetArg(args[nargs],XtNshowLegend,False); nargs++;
    XtSetArg(args[nargs],XtNdrawMajor,False); nargs++;
    XtSetArg(args[nargs],XtNdrawMinor,False); nargs++;
#endif
  /* Not supported */
    XtSetArg(args[nargs],XtNdragX,False); nargs++;
    XtSetArg(args[nargs],XtNdragY,False); nargs++;
#if 0
  /* KE: These extend the origin to include zero
   *   They don't make the axes cross at zero */
    XtSetArg(args[nargs],XtNxOrigin,True); nargs++;
    XtSetArg(args[nargs],XtNyOrigin,True); nargs++;
#endif

#if 0
  /* Not implemented or not implemented this way */
  /* Set the plot type */
  /* (Use the default of Cartesian) */
    switch (dlCartesianPlot->style) {
    case POINT_PLOT:
    case LINE_PLOT:
    case FILL_UNDER_PLOT:
	XtSetArg(args[nargs],XtchartType,XtCartesian); nargs++;
	break;
    }
  /* Second y axis is not supported */
    if (validTraces > 1) {
	XtSetArg(args[nargs],XtNxrtY2AxisShow,TRUE); nargs++;
    }
#endif

  /* X Axis Style */
    switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].axisStyle) {
    case LINEAR_AXIS:
	XtSetArg(args[nargs],XtNxLog,False); nargs++;
	break;
    case LOG10_AXIS:
	XtSetArg(args[nargs],XtNxLog,True); nargs++;
	break;
    case TIME_AXIS: {
      /* Not supported */
	if(pcp) pcp->timeScale = True;
    }
    break;
    default:
	medmPrintf(1,"\nCpCreateCartesianPlot: Unknown X axis style\n");
	break;
    }

  /* Y1 Axis Style */
    switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].axisStyle) {
    case LINEAR_AXIS:
	XtSetArg(args[nargs],XtNyLog,False); nargs++;
	break;
    case LOG10_AXIS:
	XtSetArg(args[nargs],XtNyLog,True); nargs++;
	break;
    default:
	medmPrintf(1,"\nCpCreateCartesianPlot: Unknown Y1 axis style\n");
	break;
    }

#if 0
  /* Not implemented */
  /* Y2 Axis Style */
    switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].axisStyle) {
    case LINEAR_AXIS:
	XtSetArg(args[nargs],XtNyLog,False); nargs++;
	break;
    case LOG10_AXIS:
	XtSetArg(args[nargs],XtNyLog,True); nargs++;
	break;
    default:
	medmPrintf(1,"\nCpCreateCartesianPlot: Unknown Y1 axis style\n");
	break;
    }
#endif

  /* Add pointer to MedmCartesianPlot struct as userData to widget */
    XtSetArg(args[nargs], XmNuserData, (XtPointer)pcp); nargs++;

  /* Set miscellaneous  args */
#if SPECIFY_RESOURCES
    XtSetArg(args[nargs],XmNtraversalOn,False); nargs++;
#endif

  /* Create the widget */
    w = XtCreateWidget("cartesianPlot", sciplotWidgetClass,
      displayInfo->drawingArea, args, nargs);

  /* Have to realize the widget before setting the rest */
    XtRealizeWidget(w);

#if DEBUG_USER_DATA
    print("CpCreateCartesianPlot: widget=%x pcp=%x\n",w,pcp);
#endif

  /* Set foreground and background */
    fgcolorid = SciPlotStoreAllocatedColor(w,
      displayInfo->colormap[dlCartesianPlot->plotcom.clr]);
    bgcolorid = SciPlotStoreAllocatedColor(w,
      displayInfo->colormap[dlCartesianPlot->plotcom.bclr]);
    SciPlotSetForegroundColor(w, fgcolorid);
    SciPlotSetBackgroundColor(w, bgcolorid);

  /* Set X Axis Range */
    switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle) {
    case CHANNEL_RANGE:		/* handle as default until connected */
    case AUTO_SCALE_RANGE:
	SciPlotSetXAutoScale(w);
	break;
    case USER_SPECIFIED_RANGE:
	minF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].minRange;
	maxF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].maxRange;
	SciPlotSetXUserScale(w, minF.fval, maxF.fval);
	break;
    default:
	medmPrintf(1,"\nCpCreateCartesianPlot: Unknown X range style\n");
	break;
    }

  /* Set Y1 Axis Range */
    switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle) {
    case CHANNEL_RANGE:		/* handle as default until connected */
    case AUTO_SCALE_RANGE:
	SciPlotSetYAutoScale(w);
	break;
    case USER_SPECIFIED_RANGE:
	minF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].minRange;
	maxF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange;
	SciPlotSetYUserScale(w, minF.fval, maxF.fval);
	break;
    default:
	medmPrintf(1,"\nCpCreateCartesianPlot: Unknown Y1 range style\n");
	break;
    }

#if 0
  /* Not implemented  */
  /* Set Y2 Axis Range (Unsupported) */
    switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle) {
    case CHANNEL_RANGE:		/* handle as default until connected */
    case AUTO_SCALE_RANGE:
	SciPlotSetYAutoScale(w);
	break;
    case USER_SPECIFIED_RANGE:
	minF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].minRange;
	maxF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange;
	SciPlotSetYUserScale(w, minF.fval, max.fval);
	break;
    default:
	medmPrintf(1,"\nCpCreateCartesianPlot: Unknown Y1 range style\n");
	break;
    }
#endif

    SciPlotUpdate(w);
    return w;
}

#if 1
/* Set DEBUG_CARTESIAN_PLOT in eventHandlers.c to do this with Btn3 */
void dumpCartesianPlot(Widget w)
{
    Arg args[30];
    int n=0;

    Dimension height;
    Dimension width;
    Dimension borderWidth;
    Dimension shadowThickness;
    int defaultMarkerSize;
    int margin;
    int titleMargin;
    Boolean drawMajor;
    Boolean drawMinor;
    Boolean drawMajorTics;
    Boolean drawMinorTics;
    Boolean showLegend;
    Boolean showTitle;
    Boolean showXLabel;
    Boolean showYLabel;
    Boolean xLog;
    Boolean yLog;
    Boolean xOrigin;
    Boolean yOrigin;
    Boolean xAxisNumbers;
    Boolean yAxisNumbers;
    String plotTitle;
    String xLabel;
    String yLabel;
    XtPointer userData;
    int titleFont;
    int axisFont;
    int labelFont;

    print("\nSciPlot Widget (%x)\n",w);

    XtSetArg(args[n],XtNwidth,&width); n++;
    XtSetArg(args[n],XtNheight,&height); n++;
    XtSetArg(args[n],XtNdefaultMarkerSize,&defaultMarkerSize); n++;
    XtSetArg(args[n],XtNborderWidth,&borderWidth); n++;
    XtSetArg(args[n],XmNshadowThickness,&shadowThickness); n++;
    XtSetArg(args[n],XtNtitleMargin,&titleMargin); n++;
    XtSetArg(args[n],XtNmargin,&margin); n++;
    XtSetArg(args[n],XtNdrawMajor,&drawMajor); n++;
    XtSetArg(args[n],XtNdrawMinor,&drawMinor); n++;
    XtSetArg(args[n],XtNdrawMajorTics,&drawMajorTics); n++;
    XtSetArg(args[n],XtNdrawMinorTics,&drawMinorTics); n++;
    XtSetArg(args[n],XtNshowLegend,&showLegend); n++;
    XtSetArg(args[n],XtNshowTitle,&showTitle); n++;
    XtSetArg(args[n],XtNshowXLabel,&showXLabel); n++;
    XtSetArg(args[n],XtNshowYLabel,&showYLabel); n++;
    XtSetArg(args[n],XtNxLog,&xLog); n++;
    XtSetArg(args[n],XtNyLog,&yLog); n++;
    XtSetArg(args[n],XtNxOrigin,&xOrigin); n++;
    XtSetArg(args[n],XtNyOrigin,&yOrigin); n++;
    XtSetArg(args[n],XtNxAxisNumbers,&xAxisNumbers); n++;
    XtSetArg(args[n],XtNyAxisNumbers,&yAxisNumbers); n++;
    XtSetArg(args[n],XtNplotTitle,&plotTitle); n++;
    XtSetArg(args[n],XtNxLabel,&xLabel); n++;
    XtSetArg(args[n],XtNyLabel,&yLabel); n++;
    XtSetArg(args[n],XtNtitleFont,&titleFont); n++;
    XtSetArg(args[n],XtNaxisFont,&axisFont); n++;
    XtSetArg(args[n],XtNlabelFont,&labelFont); n++;
    XtSetArg(args[n],XmNuserData,&userData); n++;
    XtGetValues(w,args,n);

    print("  plotTitle: %s\n",plotTitle);
    print("  xLabel: %s\n",xLabel);
    print("  yLabel: %s\n",yLabel);
    print("  width: %d\n",width);
    print("  height: %d\n",height);
    print("  defaultMarkerSize: %d\n",defaultMarkerSize);
    print("  borderWidth: %d\n",borderWidth);
    print("  shadowThickness: %d\n",shadowThickness);
    print("  margin: %d\n",margin);
    print("  titleMargin: %d\n",titleMargin);
    print("  titleFont: %x Size: %d Name: %x Attribute: %x\n",
      titleFont,titleFont&XtFONT_SIZE_MASK,titleFont&XtFONT_NAME_MASK,
      titleFont&XtFONT_ATTRIBUTE_MASK);
    print("  axisFont: %x Size: %d Name: %x Attribute: %x\n",
      axisFont,axisFont&XtFONT_SIZE_MASK,axisFont&XtFONT_NAME_MASK,
      axisFont&XtFONT_ATTRIBUTE_MASK);
    print("  labelFont: %x Size: %d Name: %x Attribute: %x\n",
      labelFont,labelFont&XtFONT_SIZE_MASK,labelFont&XtFONT_NAME_MASK,
      labelFont&XtFONT_ATTRIBUTE_MASK);
    print("    Times=%x Helvetica=%x\n",XtFONT_TIMES,XtFONT_HELVETICA);
    print("    Bold=%x Italic=%x BoldItalic=%x\n",XtFONT_BOLD,
       XtFONT_ITALIC,XtFONT_BOLD_ITALIC);
    print("  drawMajor: %s\n",drawMajor?"True":"False");
    print("  drawMinor: %s\n",drawMinor?"True":"False");
    print("  drawMajorTics: %s\n",drawMajorTics?"True":"False");
    print("  drawMinorTics: %s\n",drawMinorTics?"True":"False");
    print("  showLegend: %s\n",showLegend?"True":"False");
    print("  showTitle: %s\n",showTitle?"True":"False");
    print("  showXLabel: %s\n",showXLabel?"True":"False");
    print("  showYLabel: %s\n",showYLabel?"True":"False");
    print("  xLog: %s\n",xLog?"True":"False");
    print("  yLog: %s\n",yLog?"True":"False");
    print("  xOrigin: %s\n",xOrigin?"True":"False");
    print("  yOrigin: %s\n",yOrigin?"True":"False");
    print("  xAxisNumbers: %s\n",xAxisNumbers?"True":"False");
    print("  yAxisNumbers: %s\n",yAxisNumbers?"True":"False");
    print("  userData: %x\n",userData);

#if 1
    SciPlotPrintStatistics(w);
    SciPlotPrintMetrics(w);
    SciPlotPrintAxisInfo(w);
#else
#endif
}
#endif /* dumpCartesianPlot */
