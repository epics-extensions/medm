#ifndef CARTESIAN_PLOT_H
#define CARTESIAN_PLOT_H

typedef struct {
        float axisMin;
        float axisMax;
        Boolean isCurrentlyFromChannel;
} CartesianPlotAxisRange;

typedef enum {
    CP_XYScalar,
    CP_XScalar,         CP_YScalar,
    CP_XVector,         CP_YVector,
    CP_XVectorYScalar,
    CP_YVectorXScalar,
    CP_XYVector
} XYChannelTypeEnum;

typedef struct {
  struct _CartesianPlot *cartesianPlot;
  XrtData               *xrtData;
  int                   trace;
  Record                *recordX;
  Record                *recordY;
  XYChannelTypeEnum     type;
} XYTrace;

typedef struct _CartesianPlot {
        Widget          widget;
        DlCartesianPlot *dlCartesianPlot;
        XYTrace         xyTrace[MAX_TRACES];
        XYTrace         eraseCh;
        XYTrace         triggerCh;
        UpdateTask      *updateTask;
        int             nTraces;        /* number of traces ( <= MAX_TRACES) */
        XrtData         *xrtData1, *xrtData2;    /* XrtData                  */
        /* used for channel-based range determination - filled in at connect */
        CartesianPlotAxisRange  axisRange[3];    /* X, Y, Y2 _AXIS_ELEMENT   */
        eraseMode_t     eraseMode;               /* erase mode               */
        Boolean         dirty1;                  /* xrtData1 needs screen update */
        Boolean         dirty2;                  /* xrtData2 needs screen update */
} CartesianPlot;

#endif
