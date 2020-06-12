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

/* KE: Formerly defined in GraphXMacros.h */
#define TITLE_SCALE_FACTOR 20
#define AXES_SCALE_FACTOR 27

#ifndef CARTESIAN_PLOT_H
#  define CARTESIAN_PLOT_H

#  define CP_XDATA_COLUMN 0
#  define CP_YDATA_COLUMN 1
#  define CP_COLOR_COLUMN 2
#  define CP_YAXIS_COLUMN 3

#  define CP_X 0
#  define CP_Y 1
#  define CP_Y2 2
#  define CP_Y3 3
#  define CP_Y4 4

#  define CP_LINE_SOLID 0
#  define CP_LINE_NONE 1
#  define CP_LINE_STEP 2

#  define CP_FAST 0
#  define CP_FULL 1

typedef struct
{
  float axisMin;
  float axisMax;
  Boolean isCurrentlyFromChannel;
} CartesianPlotAxisRange;

typedef enum
  {
    CP_XYScalar,
    CP_XScalar,
    CP_YScalar,
    CP_XVector,
    CP_YVector,
    CP_XVectorYScalar,
    CP_YVectorXScalar,
    CP_XYVector
  } XYChannelTypeEnum;

typedef struct
{
  struct _MedmCartesianPlot *cartesianPlot;
  CpDataHandle hcp;
  int trace;
  Record *recordX;
  Record *recordY;
  XYChannelTypeEnum type;
} XYTrace;

typedef struct _MedmCartesianPlot
{
  DlElement *dlElement;   /* Must be first */
  UpdateTask *updateTask; /* Must be second */
  XYTrace xyTrace[MAX_TRACES];
  XYTrace eraseCh;
  XYTrace triggerCh;
  XYTrace countCh;
  int nTraces;                         /* number of traces (<=MAX_TRACES) */
  int nTracesY1;                       /* number of traces (<=MAX_TRACES) */
  int nTracesY2;                       /* number of traces (<=MAX_TRACES) */
  int nTracesY3;                       /* number of traces (<=MAX_TRACES) */
  int nTracesY4;                       /* number of traces (<=MAX_TRACES) */
  int nPoints;                         /* number of points in a trace */
  CpDataHandle hcp1, hcp2, hcp3, hcp4; /* CpData handles */
  /* Used for channel-based range determination (filled in at connect) */
  CartesianPlotAxisRange axisRange[5]; /* X, Y, Y2, Y3, Y4 _AXIS_ELEMENT          */
  eraseMode_t eraseMode;               /* erase mode */
  Boolean dirty1;                      /* cpData1 needs screen update */
  Boolean dirty2;                      /* cpData2 needs screen update */
  Boolean dirty3;                      /* cpData3 needs screen update */
  Boolean dirty4;                      /* cpData4 needs screen update */
  TS_STAMP startTime;
  Boolean timeScale;
} MedmCartesianPlot;

/* Function prototypes for generic plotting routines
 * These should be the same for any plot package */

CpDataHandle CpDataCreate(Widget w, CpDataType type, int nsets, int npoints, int axis);
void CpDataDeleteCurves(Widget w, CpDataHandle hData);
int CpDataGetPointsUsed(CpDataHandle hData, int set);
double CpDataGetXElement(CpDataHandle hData, int set, int point);
double CpDataGetYElement(CpDataHandle hData, int set, int point);
void CpDataDestroy(CpDataHandle hData);
int CpDataSetHole(CpDataHandle hData, double hole);
int CpDataSetPointsUsed(Widget w, CpDataHandle hData, int set, int npoints);
int CpDataSetXElement(CpDataHandle hData, int set, int point, double x);
int CpDataSetYElement(CpDataHandle hData, int set, int point, double y);

void CpEraseData(Widget w, int axis, CpDataHandle hData);
void CpGetAxisInfo(Widget w,
                   XtPointer *userData, Boolean *xAxisIsTime, char **timeFormat,
                   Boolean *xAxisIsLog, Boolean *yAxisIsLog, Boolean *y2AxisIsLog, Boolean *y3AxisIsLog, Boolean *y4AxisIsLog,
                   Boolean *xAxisIsAuto, Boolean *yAxisIsAuto, Boolean *y2AxisIsAuto, Boolean *y3AxisIsAuto, Boolean *y4AxisIsAuto,
                   XcVType *xMaxF, XcVType *yMaxF, XcVType *y2MaxF, XcVType *y3MaxF, XcVType *y4MaxF,
                   XcVType *xMinF, XcVType *yMinF, XcVType *y2MinF, XcVType *y3MinF, XcVType *y4MinF);
void CpGetAxisMaxMin(Widget w, int axis, XcVType *maxF, XcVType *minF);
#  if defined(SCIPLOT)
void CpSetAxisStyle(Widget w, CpDataHandle hData, int trace, int lineType,
                    int fillType, XColor color, int pointSize, int Yaxis, int nYaxis, int Yside);
#  else
void CpSetAxisStyle(Widget w, CpDataHandle hData, int trace, int lineType,
                    int fillType, XColor color, int pointSize, int Yaxis, int nYaxis);
#  endif
void CpSetAxisAll(Widget w, int axis, XcVType max, XcVType min,
                  XcVType tick, XcVType num, int precision);
void CpSetAxisMax(Widget w, int axis, XcVType max, XcVType tick,
                  XcVType num, int precision);
void CpSetAxisMin(Widget w, int axis, XcVType min, XcVType tick,
                  XcVType num, int precision);
void CpSetAxisAuto(Widget w, int axis);
void CpSetAxisChannel(Widget w, int axis, XcVType max, XcVType min);
void CpSetAxisLinear(Widget w, int axis);
void CpSetAxisLog(Widget w, int axis);
void CpSetAxisMaxMin(Widget w, int axis, XcVType max, XcVType min);
void CpSetAxisTime(Widget w, int axis, time_t base, char *format);
void CpSetData(Widget w, int axis, CpDataHandle hData);
void CpSetTimeBase(Widget w, time_t base);
void CpSetTimeFormat(Widget w, char *format);
void CpUpdateWidget(Widget w, int full);
Widget CpCreateCartesianPlot(DisplayInfo *displayInfo,
                             DlCartesianPlot *dlCartesianPlot, MedmCartesianPlot *pcp);

#  define CP_X_AXIS_STYLE 0
#  define CP_Y_AXIS_STYLE 1
#  define CP_Y2_AXIS_STYLE 2
#  define CP_Y3_AXIS_STYLE 3
#  define CP_Y4_AXIS_STYLE 4
#  define CP_X_RANGE_STYLE 5
#  define CP_Y_RANGE_STYLE 6
#  define CP_Y2_RANGE_STYLE 7
#  define CP_Y3_RANGE_STYLE 8
#  define CP_Y4_RANGE_STYLE 9
#  define CP_X_RANGE_MIN 10
#  define CP_Y_RANGE_MIN 11
#  define CP_Y2_RANGE_MIN 12
#  define CP_Y3_RANGE_MIN 13
#  define CP_Y4_RANGE_MIN 14
#  define CP_X_RANGE_MAX 15
#  define CP_Y_RANGE_MAX 16
#  define CP_Y2_RANGE_MAX 17
#  define CP_Y3_RANGE_MAX 18
#  define CP_Y4_RANGE_MAX 19
#  define CP_X_TIME_FORMAT 20

#  define MAX_CP_AXIS_ELEMENTS 30
/* The following should be the largest of NUM_CP_TIME_FORMAT,
 *   NUM_CARTESIAN_PLOT_RANGE_STYLES, NUM_CARTESIAN_PLOT_AXIS_STYLES */
#  define MAX_CP_AXIS_BUTTONS NUM_CP_TIME_FORMAT

EXTERN char *cpTimeFormatString[NUM_CP_TIME_FORMAT];

#endif /* #ifndef CARTESIAN_PLOT_H */
