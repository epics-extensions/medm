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

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN_FONT_HEIGHT 8

#include "medm.h"
#include "medmSciPlot.h"
#include "medmCartesianPlot.h"

/* Static Function prototypes */

/* Implementations */

CpDataHandle CpDataCreate(Widget w, CpDataType type, int nsets, int npoints, int axis)
{
  CpDataHandle hData;
  CpDataSet *ds;
  int i;

  /* Allocate structure  */
  hData = (CpDataHandle)calloc(1, sizeof(CpData));
  if (!hData)
    return NULL;

  /* Initialize */
  hData->nsets = nsets;
  hData->data = NULL;
  if (nsets <= 0)
    return hData;

  /* Allocate data sets */
  hData->data = (CpDataSet *)calloc(nsets, sizeof(CpDataSet));
  if (!hData)
    {
      CpDataDestroy(hData);
      return NULL;
    }

  /* Initialize data sets */
  for (i = 0; i < nsets; i++)
    {
      ds = hData->data + i;
      ds->npoints = npoints;
      ds->pointsUsed = MAX(npoints, 0);
      ds->listid = INVALID_LISTID;
      ds->xp = NULL;
      ds->yp = NULL;
      /* Allocate x and y arrays */
      if (npoints > 0)
        {
          ds->xp = (double *)calloc(npoints, sizeof(double));
          ds->yp = (double *)calloc(npoints, sizeof(double));
          if (!ds->xp || !ds->yp)
            {
              CpDataDestroy(hData);
              return NULL;
            }
        }
      /* Determine a listid now to allow setting axis parameters, etc.  */
      if (w)
        {
          ds->listid = SciPlotListCreateDouble(w, ds->npoints,
                                               ds->xp, ds->yp, axis);
        }
    }

#if DEBUG_MEMORY
  if (hData)
    {
      print("CpDataCreate: hData=%x nsets=%d data=%x\n", hData, hData->nsets, hData->data);
      for (i = 0; i < hData->nsets; i++)
        {
          ds = hData->data + i;
          print("  Set %d: ds=%x npoints=%d pointsUsed=%d listid=%d xp=%x yp=%x\n",
                i, ds, ds->npoints, ds->pointsUsed, ds->listid, ds->xp, ds->yp);
        }
    }
  else
    {
      print("CpDataCreate: hData=NULL\n");
    }
#endif

  return hData;
}

void CpDataDeleteCurves(Widget w, CpDataHandle hData)
{
  int i, listid, nsets;

  /* Return if handle is NULL */
  if (!hData)
    return;

  /* Loop over sets */
  nsets = hData->nsets;
  for (i = 0; i < nsets; i++)
    {
      listid = hData->data[i].listid;
      if (listid != INVALID_LISTID)
        {
          SciPlotListDelete(w, listid);
          hData->data[i].listid = INVALID_LISTID;
        }
    }
}

int CpDataGetPointsUsed(CpDataHandle hData, int set)
{
  return hData->data[set].pointsUsed;
}

double CpDataGetXElement(CpDataHandle hData, int set, int point)
{
  return hData->data[set].xp[point];
}

double CpDataGetYElement(CpDataHandle hData, int set, int point)
{
  return hData->data[set].yp[point];
}

void CpDataDestroy(CpDataHandle hData)
{
  CpDataSet *ds;
  int i;

#if DEBUG_MEMORY
  if (hData)
    {
      print("CpDataCreate: hData=%x nsets=%d data=%x\n", hData, hData->nsets, hData->data);
      for (i = 0; i < hData->nsets; i++)
        {
          ds = hData->data + i;
          print("  Set %d: ds=%x npoints=%d pointsUsed=%d listid=%d xp=%x yp=%x\n",
                i, ds, ds->npoints, ds->pointsUsed, ds->listid, ds->xp, ds->yp);
        }
    }
  else
    {
      print("CpDataCreate: hData=NULL\n");
    }
#endif

  if (hData)
    {
      if (hData->data)
        {
          for (i = 0; i < hData->nsets; i++)
            {
              ds = hData->data + i;
              if (ds->xp)
                free((char *)ds->xp);
              if (ds->yp)
                free((char *)ds->yp);
            }
          free((char *)hData->data);
        }
      free(hData);
      hData = NULL;
    }
}

int CpDataSetHole(CpDataHandle hData, double hole)
{
  /* Do nothing, hole is always SCIPLOT_SKIP_VAL */
  return 1;
}

int CpDataSetPointsUsed(Widget w, CpDataHandle hData, int set, int point)
{
  int listid;

  hData->data[set].pointsUsed = MAX(point, 0);
  listid = hData->data[set].listid;
  SciPlotListUpdateDouble(w, listid, hData->data[set].pointsUsed,
                          hData->data[set].xp, hData->data[set].yp);
  return 1;
}

int CpDataSetXElement(CpDataHandle hData, int set, int point, double x)
{
  hData->data[set].xp[point] = x;
  return 1;
}

int CpDataSetYElement(CpDataHandle hData, int set, int point, double y)
{
  hData->data[set].yp[point] = y;
  return 1;
}

void CpGetAxisInfo(Widget w,
                   XtPointer *userData, Boolean *xAxisIsTime, char **timeFormat,
                   Boolean *xAxisIsLog, Boolean *yAxisIsLog, Boolean *y2AxisIsLog, Boolean *y3AxisIsLog, Boolean *y4AxisIsLog,
                   Boolean *xAxisIsAuto, Boolean *yAxisIsAuto, Boolean *y2AxisIsAuto, Boolean *y3AxisIsAuto, Boolean *y4AxisIsAuto,
                   XcVType *xMaxF, XcVType *yMaxF, XcVType *y2MaxF, XcVType *y3MaxF, XcVType *y4MaxF,
                   XcVType *xMinF, XcVType *yMinF, XcVType *y2MinF, XcVType *y3MinF, XcVType *y4MinF)
{
  double xMin, yMin, xMax, yMax, y2Min, y2Max, y3Min, y3Max, y4Min, y4Max;

  /* Get available information from SciPlot */
  SciPlotGetXAxisInfo(w, &xMin, &xMax, xAxisIsLog, xAxisIsAuto, xAxisIsTime, timeFormat);
  SciPlotGetYAxisInfo(w, &yMin, &yMax, yAxisIsLog, yAxisIsAuto, CP_Y);
  SciPlotGetYAxisInfo(w, &y2Min, &y2Max, y2AxisIsLog, y2AxisIsAuto, CP_Y2);
  SciPlotGetYAxisInfo(w, &y3Min, &y3Max, y3AxisIsLog, y3AxisIsAuto, CP_Y3);
  SciPlotGetYAxisInfo(w, &y4Min, &y4Max, y4AxisIsLog, y4AxisIsAuto, CP_Y4);

  /* Convert */
  xMinF->fval = xMin;
  yMinF->fval = yMin;
  y2MinF->fval = y2Min;
  y3MinF->fval = y3Min;
  y4MinF->fval = y4Min;
  xMaxF->fval = xMax;
  yMaxF->fval = yMax;
  y2MaxF->fval = y2Max;
  y3MaxF->fval = y3Max;
  y4MaxF->fval = y4Max;

  /* Get userData */
  XtVaGetValues(w, XmNuserData, userData, NULL);

  /* Define the rest */
  *xAxisIsTime = False; /* Not implemented */
  *timeFormat = NULL;   /* Not implemented */
                        //*y2AxisIsLog = False;      /* Not implemented */
    //*y2AxisIsAuto = True;      /* Not implemented */
    }

void CpGetAxisMaxMin(Widget w, int axis, XcVType *maxF, XcVType *minF)
{
  double min = 0, max = 0;

  switch (axis)
    {
    case CP_X:
      SciPlotGetXScale(w, &min, &max);
      break;
    case CP_Y:
    case CP_Y2:
    case CP_Y3:
    case CP_Y4:
      SciPlotGetYScale(w, &min, &max, axis);
      break;
    }

  /* Convert */
  minF->fval = min;
  maxF->fval = max;
}

void CpSetAxisStyle(Widget w, CpDataHandle hData, int trace, int lineType,
                    int fillType, XColor color, int pointSize, int Yaxis, int nYaxis, int Yside)
/* Fill type is not supported */
/* Yaxis, nYaxis are unused. Yaxis Y1=0, Y2=1, Y3=2, Y4=3, nYaxis starts at 0 */
{
  int colorid, pointStyle, lineStyle, fillStyle;
  int listid, set;

  /* Return if handle is NULL */
  if (!hData)
    return;

  set = nYaxis;
  //  set = (trace) ? trace - 1 : 0;
  //  if (set >= hData->nsets)
  //    return;

  /* Return if an invalid id (list not defined yet) */
  listid = hData->data[set].listid;
  if (listid == INVALID_LISTID)
    return;

  /* Convert color to an index */
  colorid = SciPlotStoreAllocatedColor(w, color.pixel);

  /* Line style */
  if (lineType == CP_LINE_NONE)
    {
      lineStyle = XtLINE_NONE;
      /* Point style (depends on trace not data set) */
      if (trace == 0)
        {
          pointStyle = XtMARKER_FCIRCLE;
        }
      else
        {
          switch (trace % 8)
            {
            case 0:
              pointStyle = XtMARKER_FCIRCLE;
              break;
            case 1:
              pointStyle = XtMARKER_FSQUARE;
              pointSize = pointSize * 3 / 4;
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
              pointStyle = XtMARKER_FRTRIANGLE;
              break;
            case 6:
              pointStyle = XtMARKER_FLTRIANGLE;
              break;
            case 7:
              pointStyle = XtMARKER_FBOWTIE;
              break;
            default:
              pointStyle = XtMARKER_NONE;
              break;
            }
        }
    }
  else if (lineType == CP_LINE_STEP)
    {
      lineStyle = XtLINE_STEP;
      pointStyle = XtMARKER_NONE;
      pointSize = 0;
    }
  else
    {
      lineStyle = XtLINE_SOLID;
      pointStyle = XtMARKER_NONE;
      pointSize = 0;
    }

  /* Fill style */
  if (fillType == CP_LINE_NONE)
    fillStyle = XtLINE_NONE;
  else
    {
      fillStyle = XtLINE_SOLID;
    }
  /* Set the styles */
  SciPlotListSetStyle(w, listid, colorid, pointStyle, colorid, lineStyle, fillStyle, Yaxis + 1, Yside);
  SciPlotListSetMarkerSize(w, listid, (double)(pointSize * 2. / 3.));
  SciPlotUpdate(w);
}

void CpSetAxisAll(Widget w, int axis, XcVType max, XcVType min,
                  XcVType tick, XcVType num, int precision)
/* Ticks parameters cannot be changed */
{
  switch (axis)
    {
    case CP_X:
      SciPlotSetXUserScale(w, min.fval, max.fval);
      break;
    case CP_Y:
    case CP_Y2:
    case CP_Y3:
    case CP_Y4:
      SciPlotSetYUserScale(w, min.fval, max.fval, axis);
      break;
    }
  SciPlotUpdate(w);
}

void CpSetAxisAuto(Widget w, int axis)
{
  switch (axis)
    {
    case CP_X:
      SciPlotSetXAutoScale(w);
      break;
    case CP_Y:
    case CP_Y2:
    case CP_Y3:
    case CP_Y4:
      SciPlotSetYAutoScale(w, axis);
      break;
    }
  SciPlotUpdate(w);
}

void CpSetAxisChannel(Widget w, int axis, XcVType max, XcVType min)
{
  switch (axis)
    {
    case CP_X:
      SciPlotSetXUserScale(w, min.fval, max.fval);
      break;
    case CP_Y:
    case CP_Y2:
    case CP_Y3:
    case CP_Y4:
      SciPlotSetYUserScale(w, min.fval, max.fval, axis);
      break;
    }
  SciPlotUpdate(w);
}

void CpSetAxisLinear(Widget w, int axis)
{
  Arg args[1];
  int nargs;

  nargs = 0;
  switch (axis)
    {
    case CP_X:
      XtSetArg(args[nargs], XtNxLog, False);
      nargs++;
      break;
    case CP_Y:
      XtSetArg(args[nargs], XtNyLog, False);
      nargs++;
      break;
    case CP_Y2:
      XtSetArg(args[nargs], XtNy2Log, False);
      nargs++;
      break;
    case CP_Y3:
      XtSetArg(args[nargs], XtNy3Log, False);
      nargs++;
      break;
    case CP_Y4:
      XtSetArg(args[nargs], XtNy4Log, False);
      nargs++;
      break;
    }
  XtSetValues(w, args, nargs);

  /* Not supposed to be necessary */
  SciPlotUpdate(w);
}

void CpSetAxisLog(Widget w, int axis)
{
  Arg args[1];
  int nargs;

  nargs = 0;
  switch (axis)
    {
    case CP_X:
      XtSetArg(args[nargs], XtNxLog, True);
      nargs++;
      break;
    case CP_Y:
      XtSetArg(args[nargs], XtNyLog, True);
      nargs++;
      break;
    case CP_Y2:
      XtSetArg(args[nargs], XtNy2Log, True);
      nargs++;
      break;
    case CP_Y3:
      XtSetArg(args[nargs], XtNy3Log, True);
      nargs++;
      break;
    case CP_Y4:
      XtSetArg(args[nargs], XtNy4Log, True);
      nargs++;
      break;
    }
  XtSetValues(w, args, nargs);

  /* Not supposed to be necessary */
  SciPlotUpdate(w);
}

void CpSetAxisMaxMin(Widget w, int axis, XcVType max, XcVType min)
{
  switch (axis)
    {
    case CP_X:
      SciPlotSetXUserScale(w, min.fval, max.fval);
      break;
    case CP_Y:
    case CP_Y2:
    case CP_Y3:
    case CP_Y4:
      SciPlotSetYUserScale(w, min.fval, max.fval, axis);
      break;
    }
  SciPlotUpdate(w);
}

void CpSetAxisMax(Widget w, int axis, XcVType max, XcVType tick,
                  XcVType num, int precision)
/* Ticks parameters cannot be changed */
{
  switch (axis)
    {
    case CP_X:
      SciPlotSetXUserMax(w, max.fval);
      break;
    case CP_Y:
    case CP_Y2:
    case CP_Y3:
    case CP_Y4:
      SciPlotSetYUserMax(w, max.fval, axis);
      break;
    }
  SciPlotUpdate(w);
}

void CpSetAxisMin(Widget w, int axis, XcVType min, XcVType tick,
                  XcVType num, int precision)
/* Ticks parameters cannot be changed */
{
  switch (axis)
    {
    case CP_X:
      SciPlotSetXUserMin(w, min.fval);
      break;
    case CP_Y:
    case CP_Y2:
    case CP_Y3:
    case CP_Y4:
      SciPlotSetYUserMin(w, min.fval, axis);
      break;
    }
  SciPlotUpdate(w);
}

void CpSetAxisTime(Widget w, int axis, time_t base, char *format)
{
  Arg args[4];
  int nargs;

  nargs = 0;
  switch (axis)
    {
    case CP_X:
      XtSetArg(args[nargs], XtNxLog, False);
      nargs++;
      XtSetArg(args[nargs], XtNxTime, True);
      nargs++;
      XtSetArg(args[nargs], XtNxTimeBase, base);
      nargs++;
      XtSetArg(args[nargs], XtNxTimeFormat, format);
      nargs++;
      break;
    }
  XtSetValues(w, args, nargs);
}

void CpSetData(Widget w, int axis, CpDataHandle hData)
/* Axis (CP_X, CP_Y, CP_Y2) doesn't matter */
{
  int i, listid, nsets;

  /* Return if handle is NULL */
  if (!hData)
    return;

  /* Loop over sets */
  nsets = hData->nsets;
  for (i = 0; i < nsets; i++)
    {
      listid = hData->data[i].listid;
#if 0
      if(hData->data[i].npoints <= 0) continue;
#endif
      if (listid == INVALID_LISTID)
        {
          /* Not set yet, set with npoints */
          listid = SciPlotListCreateDouble(w, hData->data[i].npoints,
                                           hData->data[i].xp, hData->data[i].yp, axis);
        }
      /* Update with pointsUsed */
      SciPlotListUpdateDouble(w, listid, hData->data[i].pointsUsed,
                              hData->data[i].xp, hData->data[i].yp);
    }
  /* Don't do SciPlotUpdate here for efficiency */
}

void CpEraseData(Widget w, int axis, CpDataHandle hData)
/* Axis (CP_X, CP_Y, CP_Y2) doesn't matter */
{
  int i, listid, nsets;

  /* Return if handle is NULL */
  if (!hData)
    return;

  /* Loop over sets */
  nsets = hData->nsets;
  for (i = 0; i < nsets; i++)
    {
      listid = hData->data[i].listid;
#if 0
      if(hData->data[i].npoints <= 0) continue;
#endif
      if (listid == INVALID_LISTID)
        {
          /* Not set yet, set with npoints */
          listid = SciPlotListCreateDouble(w, hData->data[i].npoints,
                                           hData->data[i].xp, hData->data[i].yp, axis);
        }
      /* Update with zero */
      SciPlotListUpdateDouble(w, listid, 0,
                              hData->data[i].xp, hData->data[i].yp);
    }
  /* Don't do SciPlotUpdate here for efficiency */
}

void CpSetTimeBase(Widget w, time_t base)
{
  Arg args[1];
  int nargs;

  nargs = 0;
  XtSetArg(args[nargs], XtNxTimeBase, base);
  nargs++;
  XtSetValues(w, args, nargs);
}

void CpUpdateWidget(Widget w, int full)
{
  if (full)
    {
      SciPlotUpdate(w);
    }
  else
    {
      /* Try data only, do full if data out of bounds */
      if (SciPlotQuickUpdate(w))
        SciPlotUpdate(w);
    }
}

void CpSetTimeFormat(Widget w, char *format)
{
  Arg args[1];
  int nargs;

  nargs = 0;
  XtSetArg(args[nargs], XtNxTimeFormat, format);
  nargs++;
  XtSetValues(w, args, nargs);
}

Widget CpCreateCartesianPlot(DisplayInfo *displayInfo,
                             DlCartesianPlot *dlCartesianPlot, MedmCartesianPlot *pcp)
{
  Arg args[75]; /* Count later */
  int nargs;

  int fgcolorid, bgcolorid;
  int preferredHeight;
  XcVType minF, maxF;
  Widget w;

  /* Set widget args from the dlCartesianPlot structure */
  nargs = 0;
  XtSetArg(args[nargs], XmNx, (Position)dlCartesianPlot->object.x);
  nargs++;
  XtSetArg(args[nargs], XmNy, (Position)dlCartesianPlot->object.y);
  nargs++;
  XtSetArg(args[nargs], XmNwidth, (Dimension)dlCartesianPlot->object.width);
  nargs++;
  XtSetArg(args[nargs], XmNheight, (Dimension)dlCartesianPlot->object.height);
  nargs++;
  XtSetArg(args[nargs], XmNshadowType, XmSHADOW_OUT);
  nargs++;
#if SPECIFY_RESOURCES
  XtSetArg(args[nargs], XmNborderWidth, 0);
  nargs++;
  XtSetArg(args[nargs], XmNhighlightThickness, 0);
  nargs++;
#endif
  preferredHeight = MIN(dlCartesianPlot->object.width,
                        dlCartesianPlot->object.height) /
    TITLE_SCALE_FACTOR;
  /* The Helvetica font here at the APS only have select sizes that look normal */
  switch (preferredHeight)
    {
    case 9:
      preferredHeight = 10;
      break;
    case 13:
      preferredHeight = 14;
      break;
    case 15:
    case 16:
      preferredHeight = 17;
      break;
    case 19:
      preferredHeight = 20;
      break;
    case 21:
    case 22:
    case 23:
      preferredHeight = 24;
      break;
    case 26:
    case 27:
    case 28:
    case 29:
      preferredHeight = 25;
      break;
    case 30:
    case 31:
    case 32:
    case 33:
      preferredHeight = 34;
      break;
    }
#if DEBUG_FONTS
  print("CpCreateCartesianPlot: Axis and Label Font\n"
        "  dlCartesianPlot->object.width=%d\n"
        "  dlCartesianPlot->object.height=%d\n"
        "  TITLE_SCALE_FACTOR=%d\n"
        "  preferredHeight=%d\n",
        dlCartesianPlot->object.width, dlCartesianPlot->object.height,
        TITLE_SCALE_FACTOR, preferredHeight);

#endif
  XtSetArg(args[nargs], XtNtitleFont, XtFONT_HELVETICA | MAX(preferredHeight, MIN_FONT_HEIGHT));
  nargs++;
  if (strlen(dlCartesianPlot->plotcom.title) > 0)
    {
      XtSetArg(args[nargs], XtNplotTitle, dlCartesianPlot->plotcom.title);
      nargs++;
      XtSetArg(args[nargs], XtNshowTitle, True);
      nargs++;
    }
  else
    {
      XtSetArg(args[nargs], XtNshowTitle, False);
      nargs++;
    }
  if (strlen(dlCartesianPlot->plotcom.xlabel) > 0)
    {
      XtSetArg(args[nargs], XtNxLabel, dlCartesianPlot->plotcom.xlabel);
      nargs++;
      XtSetArg(args[nargs], XtNshowXLabel, True);
      nargs++;
    }
  else
    {
      XtSetArg(args[nargs], XtNshowXLabel, False);
      nargs++;
    }
  if (strlen(dlCartesianPlot->plotcom.ylabel) > 0)
    {
      XtSetArg(args[nargs], XtNyLabel, dlCartesianPlot->plotcom.ylabel);
      nargs++;
      XtSetArg(args[nargs], XtNshowYLabel, True);
      nargs++;
    }
  else
    {
      XtSetArg(args[nargs], XtNshowYLabel, False);
      nargs++;
    }
  if (strlen(dlCartesianPlot->plotcom.y2label) > 0)
    {
      XtSetArg(args[nargs], XtNy2Label, dlCartesianPlot->plotcom.y2label);
      nargs++;
      XtSetArg(args[nargs], XtNshowY2Label, True);
      nargs++;
    }
  else
    {
      XtSetArg(args[nargs], XtNshowY2Label, False);
      nargs++;
    }
  if (strlen(dlCartesianPlot->plotcom.y3label) > 0)
    {
      XtSetArg(args[nargs], XtNy3Label, dlCartesianPlot->plotcom.y3label);
      nargs++;
      XtSetArg(args[nargs], XtNshowY3Label, True);
      nargs++;
    }
  else
    {
      XtSetArg(args[nargs], XtNshowY3Label, False);
      nargs++;
    }
  if (strlen(dlCartesianPlot->plotcom.y4label) > 0)
    {
      XtSetArg(args[nargs], XtNy4Label, dlCartesianPlot->plotcom.y4label);
      nargs++;
      XtSetArg(args[nargs], XtNshowY4Label, True);
      nargs++;
    }
  else
    {
      XtSetArg(args[nargs], XtNshowY4Label, False);
      nargs++;
    }
  preferredHeight = MIN(dlCartesianPlot->object.width,
                        dlCartesianPlot->object.height) /
    AXES_SCALE_FACTOR;
  /* The Helvetica font here at the APS only have select sizes that look normal */
  switch (preferredHeight)
    {
    case 9:
      preferredHeight = 10;
      break;
    case 13:
      preferredHeight = 14;
      break;
    case 15:
    case 16:
      preferredHeight = 17;
      break;
    case 19:
      preferredHeight = 20;
      break;
    case 21:
    case 22:
    case 23:
      preferredHeight = 24;
      break;
    case 26:
    case 27:
    case 28:
    case 29:
      preferredHeight = 25;
      break;
    case 30:
    case 31:
    case 32:
    case 33:
      preferredHeight = 34;
      break;
    }
#if DEBUG_FONTS
  print("CpCreateCartesianPlot: Axis and Label Font\n"
        "  dlCartesianPlot->object.width=%d\n"
        "  dlCartesianPlot->object.height=%d\n"
        "  AXES_SCALE_FACTOR=%d\n"
        "  preferredHeight=%d\n",
        dlCartesianPlot->object.width, dlCartesianPlot->object.height,
        AXES_SCALE_FACTOR, preferredHeight);

#endif
  XtSetArg(args[nargs], XtNaxisFont, XtFONT_HELVETICA | MAX(preferredHeight, MIN_FONT_HEIGHT));
  nargs++;
  XtSetArg(args[nargs], XtNlabelFont, XtFONT_HELVETICA | MAX(preferredHeight, MIN_FONT_HEIGHT));
  nargs++;

  /* SciPlot-specific */
#if SPECIFY_RESOURCES
  /*
    XtSetArg(args[nargs], XtNtitleMargin, 5);
    nargs++;
    XtSetArg(args[nargs], XtNdrawMajor, False);
    nargs++;
    XtSetArg(args[nargs], XtNdrawMinor, False);
    nargs++;
  */
#endif
  /* Not supported */
  XtSetArg(args[nargs], XtNdragX, False);
  nargs++;
  XtSetArg(args[nargs], XtNdragY, False);
  nargs++;
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
#endif
  if ((pcp) && (pcp->nTracesY1 > 0))
    {
      XtSetArg(args[nargs], XtNyAxisShow, TRUE);
      nargs++;
    }
  else
    {
      XtSetArg(args[nargs], XtNyAxisShow, FALSE);
      nargs++;
    }
  if ((pcp) && (pcp->nTracesY2 > 0))
    {
      XtSetArg(args[nargs], XtNy2AxisShow, TRUE);
      nargs++;
    }
  if ((pcp) && (pcp->nTracesY3 > 0))
    {
      XtSetArg(args[nargs], XtNy3AxisShow, TRUE);
      nargs++;
    }
  if ((pcp) && (pcp->nTracesY4 > 0))
    {
      XtSetArg(args[nargs], XtNy4AxisShow, TRUE);
      nargs++;
    }

  /* X Axis Style */
  switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].axisStyle)
    {
    case LINEAR_AXIS:
      XtSetArg(args[nargs], XtNxLog, False);
      nargs++;
      break;
    case LOG10_AXIS:
      XtSetArg(args[nargs], XtNxLog, True);
      nargs++;
      break;
    case TIME_AXIS:
      {
        XtSetArg(args[nargs], XtNxLog, False);
        nargs++;
        XtSetArg(args[nargs], XtNxTime, True);
        nargs++;
        XtSetArg(args[nargs], XtNxTimeBase, time900101);
        nargs++;
        XtSetArg(args[nargs], XtNxTimeFormat,
                 cpTimeFormatString[dlCartesianPlot->axis[0].timeFormat - FIRST_CP_TIME_FORMAT]);
        nargs++;
        if (pcp)
          {
            pcp->timeScale = True;
          }
      }
      break;
    default:
      medmPrintf(1, "\nCpCreateCartesianPlot: Unknown X axis style\n");
      break;
    }

  /* Y1 Axis Style */
  switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].axisStyle)
    {
    case LINEAR_AXIS:
      XtSetArg(args[nargs], XtNyLog, False);
      nargs++;
      break;
    case LOG10_AXIS:
      XtSetArg(args[nargs], XtNyLog, True);
      nargs++;
      break;
    default:
      medmPrintf(1, "\nCpCreateCartesianPlot: Unknown Y1 axis style\n");
      break;
    }

  /* Y2 Axis Style */
  switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].axisStyle)
    {
    case LINEAR_AXIS:
      XtSetArg(args[nargs], XtNy2Log, False);
      nargs++;
      break;
    case LOG10_AXIS:
      XtSetArg(args[nargs], XtNy2Log, True);
      nargs++;
      break;
    default:
      medmPrintf(1, "\nCpCreateCartesianPlot: Unknown Y2 axis style\n");
      break;
    }

  /* Y3 Axis Style */
  switch (dlCartesianPlot->axis[Y3_AXIS_ELEMENT].axisStyle)
    {
    case LINEAR_AXIS:
      XtSetArg(args[nargs], XtNy3Log, False);
      nargs++;
      break;
    case LOG10_AXIS:
      XtSetArg(args[nargs], XtNy3Log, True);
      nargs++;
      break;
    default:
      medmPrintf(1, "\nCpCreateCartesianPlot: Unknown Y3 axis style\n");
      break;
    }

  /* Y4 Axis Style */
  switch (dlCartesianPlot->axis[Y4_AXIS_ELEMENT].axisStyle)
    {
    case LINEAR_AXIS:
      XtSetArg(args[nargs], XtNy4Log, False);
      nargs++;
      break;
    case LOG10_AXIS:
      XtSetArg(args[nargs], XtNy4Log, True);
      nargs++;
      break;
    default:
      medmPrintf(1, "\nCpCreateCartesianPlot: Unknown Y4 axis style\n");
      break;
    }

  /* Add pointer to MedmCartesianPlot struct as userData to widget */
  XtSetArg(args[nargs], XmNuserData, (XtPointer)pcp);
  nargs++;

  /* Set miscellaneous  args */
#if SPECIFY_RESOURCES
  XtSetArg(args[nargs], XmNtraversalOn, False);
  nargs++;
#endif
  /* Create the widget */
  w = XtCreateWidget("cartesianPlot", sciplotWidgetClass,
                     displayInfo->drawingArea, args, nargs);

  /* Have to realize the widget before setting the rest */
  XtRealizeWidget(w);

#if DEBUG_USER_DATA
  print("CpCreateCartesianPlot: widget=%x pcp=%x\n", w, pcp);
#endif
  SciPlotSetFileVersion(w, displayInfo->dlFile->versionNumber);
  /* Set foreground and background */
  fgcolorid = SciPlotStoreAllocatedColor(w,
                                         displayInfo->colormap[dlCartesianPlot->plotcom.clr]);
  bgcolorid = SciPlotStoreAllocatedColor(w,
                                         displayInfo->colormap[dlCartesianPlot->plotcom.bclr]);
  SciPlotSetForegroundColor(w, fgcolorid);
  SciPlotSetBackgroundColor(w, bgcolorid);

  /* Set X Axis Range */
  switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle)
    {
    case CHANNEL_RANGE: /* handle as default until connected */
    case AUTO_SCALE_RANGE:
      SciPlotSetXAutoScale(w);
      break;
    case USER_SPECIFIED_RANGE:
      minF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].minRange;
      maxF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].maxRange;
      SciPlotSetXUserScale(w, minF.fval, maxF.fval);
      break;
    default:
      medmPrintf(1, "\nCpCreateCartesianPlot: Unknown X range style\n");
      break;
    }

  /* Set Y1 Axis Range */
  switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle)
    {
    case CHANNEL_RANGE: /* handle as default until connected */
    case AUTO_SCALE_RANGE:
      SciPlotSetYAutoScale(w, CP_Y);
      break;
    case USER_SPECIFIED_RANGE:
      minF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].minRange;
      maxF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange;
      SciPlotSetYUserScale(w, minF.fval, maxF.fval, CP_Y);
      break;
    default:
      medmPrintf(1, "\nCpCreateCartesianPlot: Unknown Y1 range style\n");
      break;
    }

  /* Set Y2 Axis Range */
  switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle)
    {
    case CHANNEL_RANGE: /* handle as default until connected */
    case AUTO_SCALE_RANGE:
      SciPlotSetYAutoScale(w, CP_Y2);
      break;
    case USER_SPECIFIED_RANGE:
      minF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].minRange;
      maxF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].maxRange;
      SciPlotSetYUserScale(w, minF.fval, maxF.fval, CP_Y2);
      break;
    default:
      medmPrintf(1, "\nCpCreateCartesianPlot: Unknown Y2 range style\n");
      break;
    }

  /* Set Y3 Axis Range */
  switch (dlCartesianPlot->axis[Y3_AXIS_ELEMENT].rangeStyle)
    {
    case CHANNEL_RANGE: /* handle as default until connected */
    case AUTO_SCALE_RANGE:
      SciPlotSetYAutoScale(w, CP_Y3);
      break;
    case USER_SPECIFIED_RANGE:
      minF.fval = dlCartesianPlot->axis[Y3_AXIS_ELEMENT].minRange;
      maxF.fval = dlCartesianPlot->axis[Y3_AXIS_ELEMENT].maxRange;
      SciPlotSetYUserScale(w, minF.fval, maxF.fval, CP_Y3);
      break;
    default:
      medmPrintf(1, "\nCpCreateCartesianPlot: Unknown Y3 range style\n");
      break;
    }

  /* Set Y4 Axis Range */
  switch (dlCartesianPlot->axis[Y4_AXIS_ELEMENT].rangeStyle)
    {
    case CHANNEL_RANGE: /* handle as default until connected */
    case AUTO_SCALE_RANGE:
      SciPlotSetYAutoScale(w, CP_Y4);
      break;
    case USER_SPECIFIED_RANGE:
      minF.fval = dlCartesianPlot->axis[Y4_AXIS_ELEMENT].minRange;
      maxF.fval = dlCartesianPlot->axis[Y4_AXIS_ELEMENT].maxRange;
      SciPlotSetYUserScale(w, minF.fval, maxF.fval, CP_Y4);
      break;
    default:
      medmPrintf(1, "\nCpCreateCartesianPlot: Unknown Y4 range style\n");
      break;
    }

  SciPlotUpdate(w);
  return w;
}

#if 1
/* Set DEBUG_CARTESIAN_PLOT in eventHandlers.c to do this with Btn3 */
void dumpCartesianPlot(Widget w)
{
  Arg args[39];
  int n = 0;

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
  Boolean showTitle;
  Boolean showXLabel;
  Boolean showYLabel;
  Boolean showY2Label;
  Boolean showY3Label;
  Boolean showY4Label;
  Boolean xLog;
  Boolean yLog;
  Boolean y2Log;
  Boolean y3Log;
  Boolean y4Log;
  Boolean xOrigin;
  Boolean yOrigin;
  Boolean y2Origin;
  Boolean y3Origin;
  Boolean y4Origin;
  Boolean xAxisNumbers;
  Boolean yAxisNumbers;
  String plotTitle;
  String xLabel;
  String yLabel;
  String y2Label;
  String y3Label;
  String y4Label;
  XtPointer userData;
  int titleFont;
  int axisFont;
  int labelFont;

  print("\nSciPlot Widget (%x)\n", w);

  XtSetArg(args[n], XtNwidth, &width);
  n++;
  XtSetArg(args[n], XtNheight, &height);
  n++;
  XtSetArg(args[n], XtNdefaultMarkerSize, &defaultMarkerSize);
  n++;
  XtSetArg(args[n], XtNborderWidth, &borderWidth);
  n++;
  XtSetArg(args[n], XmNshadowThickness, &shadowThickness);
  n++;
  XtSetArg(args[n], XtNtitleMargin, &titleMargin);
  n++;
  XtSetArg(args[n], XtNmargin, &margin);
  n++;
  XtSetArg(args[n], XtNdrawMajor, &drawMajor);
  n++;
  XtSetArg(args[n], XtNdrawMinor, &drawMinor);
  n++;
  XtSetArg(args[n], XtNdrawMajorTics, &drawMajorTics);
  n++;
  XtSetArg(args[n], XtNdrawMinorTics, &drawMinorTics);
  n++;
  XtSetArg(args[n], XtNshowTitle, &showTitle);
  n++;
  XtSetArg(args[n], XtNshowXLabel, &showXLabel);
  n++;
  XtSetArg(args[n], XtNshowYLabel, &showYLabel);
  n++;
  XtSetArg(args[n], XtNshowY2Label, &showY2Label);
  n++;
  XtSetArg(args[n], XtNshowY3Label, &showY3Label);
  n++;
  XtSetArg(args[n], XtNshowY4Label, &showY4Label);
  n++;
  XtSetArg(args[n], XtNxLog, &xLog);
  n++;
  XtSetArg(args[n], XtNyLog, &yLog);
  n++;
  XtSetArg(args[n], XtNy2Log, &y2Log);
  n++;
  XtSetArg(args[n], XtNy3Log, &y3Log);
  n++;
  XtSetArg(args[n], XtNy4Log, &y4Log);
  n++;
  XtSetArg(args[n], XtNxOrigin, &xOrigin);
  n++;
  XtSetArg(args[n], XtNyOrigin, &yOrigin);
  n++;
  XtSetArg(args[n], XtNy2Origin, &y2Origin);
  n++;
  XtSetArg(args[n], XtNy3Origin, &y3Origin);
  n++;
  XtSetArg(args[n], XtNy4Origin, &y4Origin);
  n++;
  XtSetArg(args[n], XtNxAxisNumbers, &xAxisNumbers);
  n++;
  XtSetArg(args[n], XtNyAxisNumbers, &yAxisNumbers);
  n++;
  XtSetArg(args[n], XtNplotTitle, &plotTitle);
  n++;
  XtSetArg(args[n], XtNxLabel, &xLabel);
  n++;
  XtSetArg(args[n], XtNyLabel, &yLabel);
  n++;
  XtSetArg(args[n], XtNy2Label, &y2Label);
  n++;
  XtSetArg(args[n], XtNy3Label, &y3Label);
  n++;
  XtSetArg(args[n], XtNy4Label, &y4Label);
  n++;
  XtSetArg(args[n], XtNtitleFont, &titleFont);
  n++;
  XtSetArg(args[n], XtNaxisFont, &axisFont);
  n++;
  XtSetArg(args[n], XtNlabelFont, &labelFont);
  n++;
  XtSetArg(args[n], XmNuserData, &userData);
  n++;
  XtGetValues(w, args, n);

  print("  plotTitle: %s\n", plotTitle);
  print("  xLabel: %s\n", xLabel);
  print("  yLabel: %s\n", yLabel);
  print("  y2Label: %s\n", y2Label);
  print("  y3Label: %s\n", y3Label);
  print("  y4Label: %s\n", y4Label);
  print("  width: %d\n", width);
  print("  height: %d\n", height);
  print("  defaultMarkerSize: %d\n", defaultMarkerSize);
  print("  borderWidth: %d\n", borderWidth);
  print("  shadowThickness: %d\n", shadowThickness);
  print("  margin: %d\n", margin);
  print("  titleMargin: %d\n", titleMargin);
  print("  titleFont: %x Size: %d Name: %x Attribute: %x\n",
        titleFont, titleFont & XtFONT_SIZE_MASK, titleFont & XtFONT_NAME_MASK,
        titleFont & XtFONT_ATTRIBUTE_MASK);
  print("  axisFont: %x Size: %d Name: %x Attribute: %x\n",
        axisFont, axisFont & XtFONT_SIZE_MASK, axisFont & XtFONT_NAME_MASK,
        axisFont & XtFONT_ATTRIBUTE_MASK);
  print("  labelFont: %x Size: %d Name: %x Attribute: %x\n",
        labelFont, labelFont & XtFONT_SIZE_MASK, labelFont & XtFONT_NAME_MASK,
        labelFont & XtFONT_ATTRIBUTE_MASK);
  print("    Times=%x Helvetica=%x\n", XtFONT_TIMES, XtFONT_HELVETICA);
  print("    Bold=%x Italic=%x BoldItalic=%x\n", XtFONT_BOLD,
        XtFONT_ITALIC, XtFONT_BOLD_ITALIC);
  print("  drawMajor: %s\n", drawMajor ? "True" : "False");
  print("  drawMinor: %s\n", drawMinor ? "True" : "False");
  print("  drawMajorTics: %s\n", drawMajorTics ? "True" : "False");
  print("  drawMinorTics: %s\n", drawMinorTics ? "True" : "False");
  print("  showTitle: %s\n", showTitle ? "True" : "False");
  print("  showXLabel: %s\n", showXLabel ? "True" : "False");
  print("  showYLabel: %s\n", showYLabel ? "True" : "False");
  print("  showY2Label: %s\n", showY2Label ? "True" : "False");
  print("  showY3Label: %s\n", showY3Label ? "True" : "False");
  print("  showY4Label: %s\n", showY4Label ? "True" : "False");
  print("  xLog: %s\n", xLog ? "True" : "False");
  print("  yLog: %s\n", yLog ? "True" : "False");
  print("  y2Log: %s\n", y2Log ? "True" : "False");
  print("  y3Log: %s\n", y3Log ? "True" : "False");
  print("  y4Log: %s\n", y4Log ? "True" : "False");
  print("  xOrigin: %s\n", xOrigin ? "True" : "False");
  print("  yOrigin: %s\n", yOrigin ? "True" : "False");
  print("  xAxisNumbers: %s\n", xAxisNumbers ? "True" : "False");
  print("  yAxisNumbers: %s\n", yAxisNumbers ? "True" : "False");
  print("  userData: %x\n", userData);

#  if 1
  SciPlotPrintStatistics(w);
  SciPlotPrintMetrics(w);
  SciPlotPrintAxisInfo(w);
#  else
#  endif
}
#endif /* dumpCartesianPlot */
