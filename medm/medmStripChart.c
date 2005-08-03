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

#define DEBUG_STRIP_CHART 0
#define DEBUG_SC 0
#define DEBUG_DIALOG 0
#define DEBUG_RECORDS 0
#define DEBUG_COLOR 0
#define DEBUG_TIMEOUT 0
#define DEBUG_MEM 0
#define DEBUG_ACCESS 0

#include "medm.h"
#include <Xm/MwmUtil.h>

#include <Xm/DrawingAP.h>
#include <Xm/DrawP.h>
#include <X11/keysym.h>
#include <cvtFast.h>

#define SC_COLS             6
#define SC_COLOR_COLUMN     1
#define STRIP_MARGIN        0.18
#define SC_DEFAULT_CLR      4     /* grey */
#define SC_DEFAULT_PERIOD   60.0
#define SC_DEFAULT_UNITS    SECONDS

#define OMIT_TEXT           0
#define DO_TEXT             1

/* These must start with 3, 0-2 are for option menu buttons.  If any
   of LoperSrc, HoprSrc, or Units have more values, 3 must change to
   accomodate the largest. */
#define SC_CHAN_BTN         3
#define SC_CLR_BTN          4
#define SC_LOPR_SRC_BTN     5
#define SC_LOPR_BTN         6
#define SC_HOPR_SRC_BTN     7
#define SC_HOPR_BTN         8
#define SC_PREC_SRC_BTN     9
#define SC_PREC_BTN         10
#define SC_PERIOD_BTN       11
#define SC_UNITS_BTN        12
#define SC_APPLY_BTN        13
#define SC_CANCEL_BTN       14
#define SC_HELP_BTN         15

#define SC_PREC(pL)         ((short)((pL)->prec>2?(pL)->prec:2))
#define SETHIGH(x) ((x) << 16)
#define GETHIGH(x) ((x) >> 16)
#define GETLOW(x)  ((x) & 0xFF)

/* Structures */

typedef struct _MedmStripChart {
    DlElement       *dlElement;        /* Must be first */
    UpdateTask      *updateTask;       /* Must be second */
    Record          *record[MAX_PENS]; /* array of data */
  /* Strip chart data */
    int             nChannels;         /* number of channels ( <= MAX_PENS) */
    Boolean         updateEnable;      /* strip chart update enable */
    Dimension w;
    Dimension h;
  /* These (X0,Y0), (X1,Y1) are relative to main window/pixmap */
    unsigned int    dataX0;            /* upper left corner - data region X */
    unsigned int    dataY0;            /* upper left corner - data region Y */
    unsigned int    dataX1;            /* lower right corner - data region X */
    unsigned int    dataY1;            /* lower right corner - data region Y */
    unsigned int    dataWidth;         /* width of data region */
    unsigned int    dataHeight;        /* height of data region */

    int             shadowThickness;
    double          timeInterval;      /* time for each pixel */
    double          nextAdvanceTime;   /* time for advance one pixel */
    Pixmap          pixmap;
    GC              gc;
    double          value[MAX_PENS];
    double          maxVal[MAX_PENS];
    double          minVal[MAX_PENS];
    int             nextSlot;
    XtIntervalId    timerid;
} MedmStripChart;

typedef struct {
    double hi;
    double lo;
    unsigned int numDot;
    unsigned int mask;
    char format;
    int decimal;
    int width;
    double value;
    double step;
} Range;

typedef struct {
    int axisLabelFont;
    int axisLabelFontHeight;
    int xAxisLabelWidth;
    int xAxisLabelHeight;
    int yAxisLabelWidth;
    int yAxisLabelHeight;
    int titleFont;
    int titleFontHeight;
    int margin;
    int markerHeight;
    int numYAxisLabel;
    int lineSpace;
    int yAxisLabelOffset;
    int yAxisLabelTextWidth;
    int shadowThickness;
} StripChartConfigData;

/* Function prototypes */

static void stripChartConfig(MedmStripChart *psc);
static void stripChartDraw(XtPointer cd);
static void stripChartUpdateTaskCb(XtPointer cd);
static void stripChartUpdateValueCb(XtPointer cd);
static void stripChartUpdateGraphicalInfoCb(XtPointer cd);
static void redisplayStrip(Widget, XtPointer, XtPointer);
static void stripChartUpdateGraph(XtPointer);
static MedmStripChart *stripChartAlloc(DisplayInfo *, DlElement *);
static void freeStripChart(XtPointer);
static void stripChartGetRecord(XtPointer, Record **, int *);
static void configStripChart(XtPointer, XtIntervalId *);
static void stripChartInheritValues(ResourceBundle *pRCB, DlElement *p);
static void stripChartGetLimits(DlElement *pE, DlLimits **ppL, char **pN);
static void stripChartSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void stripChartSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void stripChartGetValues(ResourceBundle *pRCB, DlElement *p);
static void linear_scale(double xmin, double xmax, int n,
  double *xminp, double *xmaxp, double *dist);

static void stripChartDialogCreateDialog(void);
static void stripChartDialogCb(Widget w, XtPointer cd , XtPointer cbs);
static void stripChartDialogUpdateElement(void);
static void stripChartDialogReset(void);
static void stripChartDialogStoreTextEntries(void);
#if DEBUG_DIALOG
static void printPens(DlPen *pen, char *title);
#endif

static DlDispatchTable stripChartDlDispatchTable = {
    createDlStripChart,
    NULL,
    executeDlStripChart,
    hideDlStripChart,
    writeDlStripChart,
    stripChartGetLimits,
    stripChartGetValues,
    stripChartInheritValues,
    stripChartSetBackgroundColor,
    stripChartSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

/* Global variables */

static Widget scMatrixW = NULL;
static DlPen tempPen[MAX_PENS];
static double tempPeriod;
static TimeUnits tempUnits;
static String scColumnLabels[] = {"Channel", "Color", "Low Source",  "Low Limit",
 "High Source", "High Limit"};
static short scColumnWidths[] = {36, 6, 8, 8, 8, 8};
static Widget scForm = NULL;
static Widget table[MAX_PENS][SC_COLS];
static Widget scPeriodLabelW, scPeriodW, scUnitsW, scExecuteControlsW;
static Widget scActionAreaW, scApplyButtonW, scCancelButtonW;
static Range range[MAX_PENS];
static Range nullRange = {0.0, 0.0, 0, 0, 0, 0, 0, 0.0, 0.0};
static StripChartConfigData sccd;
char *stripChartWidgetName = "stripChart";
static char* titleStr = "Strip Chart";

static int calcLabelFontSize(MedmStripChart *psc) {
    int width;
    int fontHeight;
    int i;

    width = (psc->w < psc->h) ? psc->w : psc->h;
    if(width > 1000) {
	fontHeight = 18;
    } else if(width > 900) {
	fontHeight = 16;
    } else if(width > 750) {
	fontHeight = 14;
    } else if(width > 600) {
	fontHeight = 12;
    } else if(width > 400) {
	fontHeight = 10;
    } else {
	fontHeight = 8;
    }
    for(i = MAX_FONTS - 1; i >= 0 ; i--) {
	if((fontTable[i]->ascent + fontTable[i]->descent) <= fontHeight) {
	    break;
	}
    }
    if(i<0) i = 0;
    return i;
}

static int calcTitleFontSize(MedmStripChart *psc) {
    int width;
    int fontHeight;
    int i;

    width = (psc->w < psc->h) ? psc->w : psc->h;
    if(width > 1000) {
	fontHeight = 26;
    } else if(width > 900) {
	fontHeight = 24;
    } else if(width > 750) {
	fontHeight = 22;
    } else if(width > 600) {
	fontHeight = 20;
    } else if(width > 500) {
	fontHeight = 18;
    } else if(width > 400) {
	fontHeight = 16;
    } else if(width > 300) {
	fontHeight = 14;
    } else if(width > 250) {
	fontHeight = 12;
    } else if(width > 200) {
	fontHeight = 10;
    } else {
	fontHeight = 8;
    }
    for(i = MAX_FONTS - 1; i >= 0 ; i--) {
	if((fontTable[i]->ascent + fontTable[i]->descent) <= fontHeight) {
	    break;
	}
    }
    if(i<0) i = 0;
    return i;
}

static void calcFormat(double value, char *format, int *decimal, int *width)
{
    double order = 0.0;
    if(value == 0.0) {
	*format = 'f';
	*decimal = 1;
	*width = 3;
	return;
    } else {
	order = log10(fabs(value));
    }
    if(order > 5.0 || order < -4.0) {
	*format = 'e';
	*decimal = 1;
    } else {
	*format = 'f';
	if(order < 0.0) {
	    *decimal = (int)(order) * -1 + 2;
	} else {
	    *decimal = 1;
	}
    }
    if(order >= 4.0) {
	*width = 7;
    } else if(order >= 3.0) {
	*width = 6;
    } else if(order >= 2.0) {
	*width = 5;
    } else if(order >= 1.0) {
	*width = 4;
    } else if(order >= 0.0) {
	*width = 3;
    } else if(order >= -1.0) {
	*width = 4;
    } else if(order >= -2.0) {
	*width = 5;
    } else if(order >= -3.0) {
	*width = 6;
    } else {
	*width = 7;
    }
}

static int calcMargin(MedmStripChart *psc) {
    int width;
    int margin;
    width = (psc->h < psc->w) ? psc->h : psc->w;
    if(width > 1000) {
	margin = 6;
    } else if(width > 800) {
	margin = 5;
    } else if(width > 600) {
	margin = 4;
    } else if(width > 400) {
	margin = 3;
    } else if(width > 300) {
	margin = 2;
    } else if(width > 200) {
	margin = 1;
    } else {
	margin = 0;
    }
    return margin;
}

static int calcMarkerHeight(MedmStripChart *psc) {
    int width;
    int markerHeight;
    width = (psc->h < psc->w) ? psc->h : psc->w;
    if(width > 1000) {
	markerHeight = 6;
    } else if(width > 800) {
	markerHeight = 5;
    } else if(width > 600) {
	markerHeight = 4;
    } else if(width > 400) {
	markerHeight = 3;
    } else if(width > 300) {
	markerHeight = 2;
    } else {
	markerHeight = 1;
    }
    return markerHeight;
}

static void calcYAxisLabelWidth(MedmStripChart *psc) {
    DlStripChart *dlStripChart = psc->dlElement->structure.stripChart;
    int i;
    int cnt;
    int maxWidth = 0;
    int maxDot = 0;
    double lopr, hopr;

    sccd.axisLabelFont = calcLabelFontSize(psc);
    sccd.axisLabelFontHeight = fontTable[sccd.axisLabelFont]->ascent +
      fontTable[sccd.axisLabelFont]->descent;
    sccd.markerHeight = calcMarkerHeight(psc);
    sccd.lineSpace = 3;
    sccd.margin = calcMargin(psc);;
    sccd.shadowThickness = 3;

  /* clear ranges */
    for(i = 0; i<psc->nChannels; i++) {
	range[i] = nullRange;
    }

  /* remove any duplicated settings */
    cnt = 0;
    for(i = 0; i < psc->nChannels; i++) {
	int found = 0;
	int j;

	updatePvLimits(&dlStripChart->pen[i].limits);
	lopr = dlStripChart->pen[i].limits.lopr;
	hopr = dlStripChart->pen[i].limits.hopr;

	for(j=0; j<cnt; j++) {
	    if(hopr == range[j].hi && lopr == range[j].lo) {
		found = 1;
		range[j].numDot++;
		range[j].mask = range[j].mask | (0x0001 << i);
		break;
	    }
	}

	if(!found) {
	    char f1, f2;
	    int  d1, d2;
	    int  w1, w2;

	    range[cnt].hi = hopr;
	    range[cnt].lo = lopr;
	    range[cnt].mask = range[cnt].mask | (0x0001 << i);
	    range[cnt].numDot = 1;
	    calcFormat(range[cnt].hi, &f1, &d1, &w1);
	    calcFormat(range[cnt].lo, &f2, &d2, &w2);
	    range[cnt].format = (f1 == 'e' || f2 == 'e') ? 'e' : 'f';
	    range[cnt].decimal = (d1 > d2) ? d1 : d2;
	    if(range[cnt].hi < 0.0) w1++;
	    if(range[cnt].lo < 0.0) w2++;
	    range[cnt].width = (w1 > w2) ? w1 : w2;
	    cnt++;
	}
    }
    sccd.numYAxisLabel = cnt;
    if(sccd.numYAxisLabel == 1) {
	range[0].numDot = 0;
    }
    for(i = 0; i < sccd.numYAxisLabel; i++) {
	int width;
	char *text = "-8.8888888888";

	width = XTextWidth(fontTable[sccd.axisLabelFont], text, range[i].width);
	if(width > maxWidth)
	  maxWidth = width;
	if((int)range[i].numDot > maxDot) maxDot = range[i].numDot;
    }
    sccd.yAxisLabelWidth = maxWidth + (maxDot) * sccd.lineSpace;
    sccd.yAxisLabelTextWidth = maxWidth;
}

static void calcTitleHeight(MedmStripChart *psc) {
    sccd.titleFont = calcTitleFontSize(psc);
    sccd.titleFontHeight = fontTable[sccd.titleFont]->ascent +
      fontTable[sccd.titleFont]->descent;
}

static void calcXAxisLabelWidth(MedmStripChart *psc) {
    char format;
    int decimal;
    int width;

    calcFormat(psc->dlElement->structure.stripChart->period,
      &format, &decimal, &width);
    width = width + 1;
    sccd.xAxisLabelWidth = XTextWidth(fontTable[sccd.axisLabelFont],
      "-0.0000000", width) + psc->dataWidth;
}

static void calcYAxisLabelHeight(MedmStripChart *psc) {
    sccd.yAxisLabelHeight = sccd.axisLabelFontHeight *sccd.numYAxisLabel
      + psc->dataHeight;
}

static MedmStripChart *stripChartAlloc(DisplayInfo *displayInfo,
  DlElement *dlElement) {
    MedmStripChart *psc;
    DlStripChart *dlStripChart = dlElement->structure.stripChart;

    psc = (MedmStripChart *)calloc(1, sizeof(MedmStripChart));
    dlElement->data = (void *)psc;
#if DEBUG_MEM
    print("stripChartAlloc: dlElement=%x dlElement->data=%x\n",
      dlElement, dlElement->data);
#endif
    if(psc == NULL) {
	medmPrintf(1, "\nstripChartAlloc: Memory allocation error\n");
	return psc;
    }
  /* Pre-initialize */
    psc->updateTask = NULL;
    psc->dlElement = dlElement;

    psc->w = dlStripChart->object.width;
    psc->h = dlStripChart->object.height;
    psc->dataX0 =  (int)(STRIP_MARGIN*psc->w);
    psc->dataY0 =  (int)(STRIP_MARGIN*psc->h);
    psc->dataX1 =  (int)((1.0 - STRIP_MARGIN)*psc->w);
    psc->dataY1 =  (int)((1.0 - STRIP_MARGIN)*psc->h);
    psc->dataWidth = psc->dataX1 - psc->dataX0;
    psc->dataHeight = psc->dataY1 - psc->dataY0;

    psc->updateEnable = False;
    psc->pixmap = (Pixmap)0;
    psc->nextAdvanceTime = medmTime();
    psc->timerid = (XtIntervalId)0;
    switch(dlStripChart->units) {
    case MILLISECONDS:
	psc->timeInterval =
	  dlStripChart->period * 0.001 / (double) psc->dataWidth;
	break;
    case SECONDS:
	psc->timeInterval =
	  dlStripChart->period / (double)psc->dataWidth;
	break;
    case MINUTES:
	psc->timeInterval =
	  dlStripChart->period * 60 / (double) psc->dataWidth;
	break;
    default:
	medmPrintf(1, "\nstripChartAlloc: Unknown time unit\n");
	psc->timeInterval = 60/ (double) psc->dataWidth;
	break;
    }
    psc->updateTask =
      updateTaskAddTask(displayInfo, &(dlStripChart->object),
	stripChartUpdateTaskCb,
	(XtPointer)psc);
#if DEBUG_MEM
    {
	MedmElement *pe = (MedmElement *)dlElement->data;
	print("  dlElement = %x updateTask=%x\n",
	  pe->dlElement, pe->updateTask);
    }
#endif
    if(psc->updateTask == NULL) {
	medmPrintf(1, "\nstripChartAlloc: Memory allocation error\n");
    } else {
	updateTaskAddDestroyCb(psc->updateTask, freeStripChart);
	updateTaskAddNameCb(psc->updateTask, stripChartGetRecord);
    }
    return psc;
}

static void freeStripChart(XtPointer cd) {
    MedmStripChart *psc = (MedmStripChart *)cd;
    int i;

#if DEBUG_STRIP_CHART
    fprintf(stderr, "freeStripChart: psc->timerid=%x\n",
      psc->timerid);
#endif
#if DEBUG_MEM
    {
	DlElement *dlElement = psc->dlElement;
	MedmElement *pe = (MedmElement *)dlElement->data;

	print("freeStripChart: dlElement=%x dlElement->data=%x\n",
	  dlElement, dlElement->data);
	print("  dlElement = %x updateTask=%x(pe) or %x(psc)\n",
	  pe->dlElement, pe->updateTask, psc->updateTask);
    }
#endif

    if(psc) {
	if(psc->timerid) {
	    XtRemoveTimeOut(psc->timerid);
	    psc->timerid = (XtIntervalId)0;
	}
	for(i = 0; i < psc->nChannels; i++) {
	    medmDestroyRecord(psc->record[i]);
	}
	if(psc->pixmap) {
	    XFreePixmap(display, psc->pixmap);
	    psc->pixmap = (Pixmap) NULL;
	    XFreeGC(display, psc->gc);
	    psc->gc = NULL;
	}
	if(psc->dlElement) psc->dlElement->data = NULL;
	free((char *)psc);
	psc = NULL;
    }
}


static void stripChartConfig(MedmStripChart *psc)
{
    DlStripChart *dlStripChart = psc->dlElement->structure.stripChart;
    DisplayInfo *displayInfo = psc->updateTask->displayInfo;
    Widget widget = psc->dlElement->widget;
    GC gc;
    int i;
    int width;
    int height;
    int dropXAxisUnitLabel = False;
    int dropYAxisUnitLabel = False;
    int dropTitleLabel = False;
    int squeezeSpace = False;
    int widthExt;
    int heightExt;

#if DEBUG_STRIP_CHART
    fprintf(stderr,"stripChartConfig: "
      "psc=%x psc->dlElement=%x psc->timerid=%x\n",
      &psc,psc->dlElement,psc->timerid);
#endif


    calcYAxisLabelWidth(psc);

  /* use the width of y-axis label to set all margins */
    width = sccd.shadowThickness + sccd.yAxisLabelWidth +
      (sccd.margin) * 2 + sccd.markerHeight + 1;
    psc->dataX0 = width;
    psc->dataY0 = width;
    psc->dataX1 = psc->w - psc->dataX0 - 1;
    psc->dataY1 = psc->h - psc->dataY0 - 1;
    psc->dataWidth = psc->dataX1 - psc->dataX0 + 1;
    psc->dataHeight = psc->dataY1 - psc->dataY0 + 1;

    calcTitleHeight(psc);

  /* set the height of the top margin, shrink the height of right and bottom
   *   if applicable. */
    height = sccd.shadowThickness + sccd.titleFontHeight +
      sccd.axisLabelFontHeight + sccd.margin * 3 + 1;
    psc->dataY0 = height;
    psc->dataHeight = psc->dataY1 - psc->dataY0 + 1;
    if(psc->dataY0 < psc->dataX0) {
      /* shrink the margins */
	psc->dataX1 = psc->w - psc->dataY0 - 1;
	psc->dataY1 = psc->h - psc->dataY0;
	psc->dataWidth = psc->dataX1 - psc->dataX0 + 1;
	psc->dataHeight = psc->dataY1 - psc->dataY0 + 1;
    }

  /* calc the height of the bottom margin, expands if necessary */
    height = sccd.shadowThickness + sccd.axisLabelFontHeight * 2 +
      sccd.margin * 3 + sccd.markerHeight + 2;
    if((int)(psc->h - psc->dataY1 - 1) < height) {
	psc->dataY1 = psc->h - height - 1;
	psc->dataHeight = psc->dataY1 - psc->dataY0 + 1;
    }
    if((int)(psc->w - psc->dataX1 - 1) < height) {
	psc->dataX1 = psc->w - height - 1;
	psc->dataWidth = psc->dataX1 - psc->dataX0 + 1;
    }

    calcYAxisLabelHeight(psc);

  /* make sure the height of y-axis label won't bigger than the allowed height
   *   of the graph */
    heightExt = (sccd.yAxisLabelHeight - psc->dataHeight + 1) / 2;
    if(heightExt + sccd.shadowThickness > (int)psc->dataY0) {
	psc->dataY0 = heightExt + sccd.shadowThickness;
	psc->dataHeight = psc->dataY1 - psc->dataY0 + 1;
    }
    if(heightExt + sccd.shadowThickness > (int)(psc->h - psc->dataY1)) {
	psc->dataY1 = psc->h - heightExt - sccd.shadowThickness - 1;
	psc->dataHeight = psc->dataY1 - psc->dataY0 + 1;
    }

    calcXAxisLabelWidth(psc);

  /* make sure the x-axis and y-axis labels not overlapping each other */
    widthExt = (sccd.xAxisLabelWidth - psc->dataWidth + 1) / 2;

    if((widthExt > (sccd.margin + sccd.markerHeight))
      && (height > (sccd.margin + sccd.markerHeight))) {
      /* shrink the graph */
	sccd.yAxisLabelOffset = widthExt - sccd.margin - sccd.markerHeight;
	psc->dataX0 = psc->dataX0 + sccd.yAxisLabelOffset ;
	psc->dataWidth = psc->dataX1 - psc->dataX0 + 1;
    }

  /* make sure the width of x-axis label won't bigger than the allowed width
   *   of the graph */
    if(widthExt + sccd.shadowThickness > (int)(psc->w - psc->dataX1 - 1)) {
	psc->dataX1 = psc->w - widthExt - sccd.shadowThickness - 1;
	psc->dataWidth = psc->dataX1 - psc->dataX0 + 1;
    }

  /* do the second pass */
  /* if the graph is too small do somethings */
    if(psc->dataHeight < psc->h/2U) {
	squeezeSpace = True;
	if((dlStripChart->plotcom.title == NULL)
	  || (strlen(dlStripChart->plotcom.title) == 0)) {
	    dropTitleLabel = True;
	    psc->dataY0 -= sccd.titleFontHeight;
	    psc->dataHeight = psc->dataY1 - psc->dataY0 + 1;
	    if((psc->w - psc->dataX1 - 1) > psc->dataY0) {
		psc->dataX1 = psc->w - psc->dataY0 - 1;
		psc->dataWidth = psc->dataX1 - psc->dataX0 + 1;
	    }
	}
	if((dlStripChart->plotcom.xlabel == NULL)
	  || (strlen(dlStripChart->plotcom.xlabel) == 0)) {
	    dropXAxisUnitLabel = True;
	    psc->dataY1 += sccd.axisLabelFontHeight;
	    psc->dataHeight = psc->dataY1 - psc->dataY0 + 1;
	    if((psc->w - psc->dataX1 - 1) > (psc->h - psc->dataY1 - 1)) {
		psc->dataX1 = psc->w - (psc->h - psc->dataY1 - 1) - 1;
		psc->dataWidth = psc->dataX1 - psc->dataX0 + 1;
	    }
	}
	if((dlStripChart->plotcom.ylabel == NULL)
	  || (strlen(dlStripChart->plotcom.ylabel) == 0)) {
	    dropYAxisUnitLabel = True;
	    psc->dataY0 -= sccd.axisLabelFontHeight;
	    psc->dataHeight = psc->dataY1 - psc->dataY0 + 1;
	    if((psc->w - psc->dataX1 - 1) > psc->dataY0) {
		psc->dataX1 = psc->w - psc->dataY0 - 1;
		psc->dataWidth = psc->dataX1 - psc->dataX0 + 1;
	    }
	}
      /* make sure the height of y-axis label won't be bigger than the allowed
       *   height of the graph */
	if(heightExt + sccd.shadowThickness > (int)psc->dataY0) {
	    psc->dataY0 = heightExt + sccd.shadowThickness;
	    psc->dataHeight = psc->dataY1 - psc->dataY0 + 1;
	}
	if(heightExt + sccd.shadowThickness > (int)(psc->h - psc->dataY1)) {
	    psc->dataY1 = psc->h - heightExt- sccd.shadowThickness - 1;
	    psc->dataHeight = psc->dataY1 - psc->dataY0 + 1;
	}
      /* make sure the width of x-axis label won't be bigger than the allowed
       *   width of the graph */
	if(widthExt + sccd.shadowThickness > (int)(psc->w - psc->dataX1 - 1)) {
	    psc->dataX1 = psc->w - widthExt - sccd.shadowThickness - 1;
	    psc->dataWidth = psc->dataX1 - psc->dataX0 + 1;
	}
    }

  /* make sure the size is bigger than zero */
    if(psc->dataX1 <= psc->dataX0) {
	psc->dataWidth = 2;
	psc->dataX1 = psc->dataX0 + 1;
    }
    if(psc->dataY1 <= psc->dataY0) {
	psc->dataHeight = 2;
	psc->dataY1 = psc->dataY0 + 1;
    }

  /* Destroy any existing pixmap and create one */
    if(psc->pixmap) {
	XFreePixmap(display,psc->pixmap);
    }
    psc->pixmap =
	XCreatePixmap(display, XtWindow(widget),
	psc->w, psc->h,
	DefaultDepth(display,
	  DefaultScreen(display)));
    if(!psc->gc) {
	psc->gc = XCreateGC(display, XtWindow(widget),
	  (unsigned long)NULL, NULL);
      /* Eliminate events that we do not handle anyway */
	XSetGraphicsExposures(display, psc->gc, False);
    }
    gc = psc->gc;

  /* fill the background */
    XSetForeground(display, gc, displayInfo->colormap[dlStripChart->plotcom.bclr]);
    XFillRectangle(display, psc->pixmap, gc, 0, 0,
      psc->w, psc->h);

  /* draw the rectangle which encloses the graph */
    XSetForeground(display, gc, displayInfo->colormap[dlStripChart->plotcom.clr]);
    XSetLineAttributes(display, gc, 0, LineSolid, CapButt, JoinMiter);
    XDrawRectangle(display, psc->pixmap, gc,
      psc->dataX0-1, psc->dataY0-1,
      psc->dataWidth+1, psc->dataHeight+1);

  /* draw title */
    if(dlStripChart->plotcom.title) {
	int textWidth;
	char *title = dlStripChart->plotcom.title;
	int strLen = strlen(title);

	textWidth = XTextWidth(fontTable[sccd.titleFont], title, strLen);
	XSetForeground(display, gc, displayInfo->colormap[dlStripChart->plotcom.clr]);
	XSetFont(display, gc, fontTable[sccd.titleFont]->fid);
	XDrawString(display, psc->pixmap, gc,
	  ((int)psc->w - textWidth)/2,
	  sccd.shadowThickness + sccd.margin + fontTable[sccd.titleFont]->ascent,
	  title, strLen);
    }

  /* draw y-axis label */
    if(dlStripChart->plotcom.ylabel) {
	int textWidth;
	char *label = dlStripChart->plotcom.ylabel;
	int strLen = strlen(label);
	int x, y;

	textWidth = XTextWidth(fontTable[sccd.axisLabelFont], label, strLen);
	XSetForeground(display, gc, displayInfo->colormap[dlStripChart->plotcom.clr]);
	XSetFont(display, gc, fontTable[sccd.axisLabelFont]->fid);
	x = psc->dataX0;
	y = sccd.shadowThickness + sccd.margin*2 + sccd.titleFontHeight +
	  fontTable[sccd.axisLabelFont]->ascent;
	XDrawString(display, psc->pixmap, gc, x, y, label, strLen);
    }

  /* draw y-axis scale */
    XSetFont(display, gc, fontTable[sccd.axisLabelFont]->fid);
    {
	int i;
	int labelHeight;
	int nDiv;
	Pixel fg;
	double interval;
	double nextTick;
	double verticalSpacing;

      /* calculate how many divisions are needed */
	if(squeezeSpace) {
	    verticalSpacing = 1.0;
	} else {
	    verticalSpacing = 2.0;
	}
	labelHeight = (int)(((double)sccd.numYAxisLabel + verticalSpacing)
	  * (double) sccd.axisLabelFontHeight);
	nDiv = (psc->dataHeight - 1) / labelHeight;
	if(nDiv > 10) {
	    nDiv = 10;
	} else if(nDiv == 9) {
	    nDiv = 8;
	} else if(nDiv == 7) {
	    nDiv = 6;
	}

	interval = (double)(psc->dataHeight - 1) / (double)nDiv;
	nextTick = 0.0;

	for(i=0; i<sccd.numYAxisLabel; i++) {
	    range[i].step = (range[i].hi - range[i].lo)/(double)nDiv;
	    range[i].value = range[i].hi;
	}
	fg = displayInfo->colormap[dlStripChart->plotcom.clr];
	for(i=0; i< nDiv + 1; i++) {
	    int j;
	    for(j = 0; j<sccd.numYAxisLabel; j++) {
		double yoffset;
		int w;
		int x, y;
		char text[20];
		int strLen;

		if(i == nDiv) {
		    range[j].value = range[j].lo;
		}
		if(range[j].format == 'e') {
		    sprintf(text, "%.1e", range[j].value);
		} else {
		    sprintf(text, "%.*f", range[j].decimal, range[j].value);
		}
		strLen = strlen(text);
		XSetForeground(display, gc, fg);
		w = XTextWidth(fontTable[sccd.axisLabelFont], text, strLen);
		x = psc->dataX0 - 1 - sccd.markerHeight - sccd.yAxisLabelOffset
		  - sccd.margin - w;
		yoffset = nextTick - (double) labelHeight / 2 +
		  ((double)j + verticalSpacing/2.0) * sccd.axisLabelFontHeight +
                  (double) fontTable[sccd.axisLabelFont]->ascent;
		y = (int)psc->dataY0 + (int)yoffset;
		XDrawString(display, psc->pixmap, gc, x, y, text, strLen);
		range[j].value = range[j].value - range[j].step;

		if(range[j].numDot > 0) {
		    int k;
		    int count = 0;
		    for(k = MAX_PENS - 1; k >= 0; k--) {
			if(range[j].mask & (0x1 << k)) {
			    XSetForeground(display, gc,
			      displayInfo->colormap[dlStripChart->pen[k].clr]);
			    x = psc->dataX0 - sccd.yAxisLabelTextWidth -
			      sccd.markerHeight
			      - sccd.margin - sccd.yAxisLabelOffset;
			    XFillRectangle(display, psc->pixmap, gc,
			      x-(count+1)*sccd.lineSpace,
			      y-fontTable[sccd.axisLabelFont]->ascent,
			      2, fontTable[sccd.axisLabelFont]->ascent);
#if 0
			    XDrawLine(display, psc->pixmap, gc,
			      x-count*sccd.lineSpace-1,
			      y-fontTable[sccd.axisLabelFont]->ascent,
			      x-count*sccd.lineSpace-1, y);
#endif

			    count++;
			}
		    }
		}
	    }
	    XSetForeground(display, gc, fg);
	    XDrawLine(display, psc->pixmap, gc,
	      psc->dataX0 - 2 - (sccd.markerHeight - 1),
	      (int)psc->dataY0 + (int)nextTick,
	      psc->dataX0 - 2, (int)psc->dataY0 + (int)nextTick);
	    nextTick = nextTick + interval;
	}
    }

  /* calculate the value time/pixel */
    switch (dlStripChart->units) {
    case MILLISECONDS:
	psc->timeInterval =
	  dlStripChart->period * 0.001 / (double) psc->dataWidth;
	break;
    case SECONDS:
	psc->timeInterval =
	  dlStripChart->period / (double)psc->dataWidth;
	break;
    case MINUTES:
	psc->timeInterval =
	  dlStripChart->period * 60 / (double) psc->dataWidth;
	break;
    default:
	medmPrintf(1, "\nstripChartAlloc: Unknown time unit\n");
	psc->timeInterval = 60/ (double) psc->dataWidth;
	break;
    }

  /* draw x-axis label and scale */
    {
	int i;
	char text[10];
	int  tw;
	char format;
	int  decimal;
	int  width;
	int  nDiv;
	double step;
	double value;
	double nextTick;
	int x, y;

	calcFormat(dlStripChart->period, &format, &decimal, &width);
	width = width + 1;   /* add the plus size */
	tw = XTextWidth(fontTable[sccd.axisLabelFont], "-0.000000", width);
	nDiv = (psc->dataWidth - 1)/tw;
	if(nDiv > 10) {
	    nDiv = 10;
	} else if(nDiv == 9) {
	    nDiv = 8;
	} else if(nDiv == 7) {
	    nDiv = 6;
	}
	nextTick = 0.0;

	step = dlStripChart->period/nDiv;
	value = 0.0;
	y = psc->dataY1 + 1 + sccd.axisLabelFontHeight + sccd.margin +
	  sccd.markerHeight + 1;

	XSetForeground(display, gc,
	  displayInfo->colormap[dlStripChart->plotcom.clr]);
	for(i=0; i< nDiv + 1; i++) {
	    double xoffset;
	    int w;
	    int strLen;
	    nextTick = (double) (psc->dataWidth - 1) *
	      ((double) i / (double) nDiv);
	    if(format == 'e') {
		sprintf(text, "%.1e", value);
	    } else {
		sprintf(text, "%.*f", decimal, value);
	    }
	    strLen = strlen(text);
	    w = XTextWidth(fontTable[sccd.axisLabelFont], text, strLen);
	    xoffset = nextTick + (double) w / 2.0;
	    x = (int)psc->dataX1 - (int)xoffset;
	    XDrawLine(display, psc->pixmap, gc,
	      psc->dataX1 - (int)nextTick, psc->dataY1 + 2,
	      psc->dataX1 - (int)nextTick, psc->dataY1 + 2 + sccd.markerHeight);
	    XDrawString(display, psc->pixmap, gc, x, y, text, strLen);
	    value = value - step;
	}

      /* determine the x-axis label */
	if(dropXAxisUnitLabel == False) {
	    int strLen;
	    int textWidth;
	    char *label;

	    if((dlStripChart->plotcom.xlabel)
	      && (strlen(dlStripChart->plotcom.xlabel) > (size_t) 0)) {
		label = dlStripChart->plotcom.xlabel;
	    } else {
		switch (dlStripChart->units) {
		case MILLISECONDS :
		    label = "time (ms)";
		    break;
		case SECONDS :
		    label = "time (sec)";
		    break;
		case MINUTES :
		    label = "time (min)";
		    break;
		default :
		    label = "time (sec)";
		    break;
		}
	    }
	    strLen = strlen(label);
	    textWidth = XTextWidth(fontTable[sccd.axisLabelFont], label, strLen);
	    x = psc->dataX0 + (psc->dataWidth - textWidth)/2;
	    y = psc->dataY1 + 1 + sccd.axisLabelFontHeight + sccd.margin * 2 +
	      sccd.markerHeight +
	      fontTable[sccd.axisLabelFont]->ascent + 1;
	    XDrawString(display, psc->pixmap, gc, x, y, label, strLen);
	}


#if 0
	if(!((dlStripChart->plotcom.xlabel) &&
	  (strlen(dlStripChart->plotcom.xlabel) > 0))) {
	    int strLen;
	    int textWidth;
	  /* print the timeInterval */
	    sprintf(text, "time/pixel = %.3f sec", psc->timeInterval);
	    strLen = strlen(text);
	    textWidth = XTextWidth(fontTable[sccd.axisLabelFont], text, strLen);
	    x = psc->dataX0 + (psc->dataWidth - textWidth)/2;
	    y = psc->dataY1 + sccd.axisLabelFontHeight + sccd.margin * 2 +
	      sccd.markerHeight +
	      fontTable[sccd.axisLabelFont]->ascent;
	    XDrawString(display, psc->pixmap, gc, x, y, text, strLen);
	}
#endif
    }

    {  /* draw the shadow */
	GC topGC = ((struct _XmDrawingAreaRec *)widget)->manager.top_shadow_GC;
	GC bottomGC =
	  ((struct _XmDrawingAreaRec *)widget)->manager.bottom_shadow_GC;
	_XmDrawShadows(display, psc->pixmap, topGC, bottomGC, 0, 0,
	  (Dimension)dlStripChart->object.width,
	  (Dimension)dlStripChart->object.height,
	  (Dimension)(sccd.shadowThickness-1), XmSHADOW_OUT);
    }
    XCopyArea(display, psc->pixmap, XtWindow(widget), gc,
      0, 0, psc->w, psc->h, 0, 0);
    for(i = 0; i < psc->nChannels; i++) {
	Record *tmpd = psc->record[i];
	psc->maxVal[i] = tmpd->value;
	psc->minVal[i] = tmpd->value;
	psc->value[i] = tmpd->value;
    }

    psc->nextAdvanceTime = medmTime() + psc->timeInterval;

  /* specify the time interval */
    updateTaskSetScanRate(psc->updateTask, psc->timeInterval);
    psc->updateEnable = True;
}

static void redisplayFakeStrip(Widget widget, XtPointer cd, XtPointer cbs)
{

    DlStripChart *dlStripChart = (DlStripChart *) cd;
    int usedWidth, usedHeight;
    int i;
    DisplayInfo *displayInfo;

    UNREFERENCED(cbs);

    if(!(displayInfo = dmGetDisplayInfoFromWidget(widget)))
      return;

    if(dlStripChart) {
	GC topGC;
	GC bottomGC;
	Dimension shadowThickness;
	int x, y, w, h;
	int textWidth;
	int strLen = strlen(titleStr);

	XSetLineAttributes(display, displayInfo->gc, 0,
	  LineSolid, CapButt, JoinMiter);
	XSetForeground(display, displayInfo->gc,
	  displayInfo->colormap[dlStripChart->plotcom.bclr]);
	XFillRectangle(display, XtWindow(widget), displayInfo->gc,
	  0, 0, dlStripChart->object.width, dlStripChart->object.height);
	XSetForeground(display, displayInfo->gc,
	  displayInfo->colormap[dlStripChart->plotcom.clr]);
	x = (int)(STRIP_MARGIN*dlStripChart->object.width);
	y = (int)(STRIP_MARGIN*dlStripChart->object.height);
	w = (unsigned int)((1-STRIP_MARGIN*2)*dlStripChart->object.width);
	h = (unsigned int)((1-STRIP_MARGIN*2)*dlStripChart->object.height);
	XDrawRectangle(display, XtWindow(widget), displayInfo->gc, x, y, w, h);
	i = dmGetBestFontWithInfo(fontTable, MAX_FONTS, titleStr, y, w,
	  &usedHeight, &usedWidth, TRUE);
	textWidth = XTextWidth(fontTable[i], titleStr, strLen);
	x = (dlStripChart->object.width - textWidth)/2;
	XSetFont(display, displayInfo->gc, fontTable[i]->fid);
	XDrawString(display, XtWindow(widget), displayInfo->gc,
	  x, (int)(0.15*dlStripChart->object.height), titleStr, strLen);
	XtVaGetValues(widget,
	  XmNshadowThickness, &shadowThickness,
	  NULL);
	topGC = ((struct _XmDrawingAreaRec *)widget)->manager.top_shadow_GC;
	bottomGC =
	  ((struct _XmDrawingAreaRec *)widget)->manager.bottom_shadow_GC;
	_XmDrawShadows(display, XtWindow(widget), topGC, bottomGC, 0, 0,
	  (Dimension)dlStripChart->object.width,
	  (Dimension)dlStripChart->object.height,
	  shadowThickness, XmSHADOW_OUT);
    }
}


void executeDlStripChart(DisplayInfo *displayInfo, DlElement *dlElement)
{
    int n;
    Arg args[15];
    Widget localWidget;
    DlStripChart *dlStripChart = dlElement->structure.stripChart;

  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(!dlElement->widget) {
      /* create the drawing widget for the strip chart */
	n = 0;
	XtSetArg(args[n], XmNx, (Position)dlStripChart->object.x); n++;
	XtSetArg(args[n], XmNy, (Position)dlStripChart->object.y); n++;
	XtSetArg(args[n], XmNwidth, (Dimension)dlStripChart->object.width); n++;
	XtSetArg(args[n], XmNheight, (Dimension)dlStripChart->object.height); n++;
	XtSetArg(args[n], XmNmarginWidth, 0); n++;
	XtSetArg(args[n], XmNmarginHeight, 0); n++;
	XtSetArg(args[n], XmNshadowThickness, 2); n++;
	XtSetArg(args[n], XmNforeground, (Pixel)
	  displayInfo->colormap[dlStripChart->plotcom.clr]); n++;
	XtSetArg(args[n], XmNbackground, (Pixel)
	  displayInfo->colormap[dlStripChart->plotcom.bclr]); n++;
	XtSetArg(args[n], XmNtraversalOn, False); n++;
	localWidget = XmCreateDrawingArea(displayInfo->drawingArea,
	  stripChartWidgetName, args, n);
	dlElement->widget = localWidget;

      /* if execute mode, create stripChart and setup all the channels */
	if(displayInfo->traversalMode == DL_EXECUTE) {
	    int i, j;
	    MedmStripChart *psc;

	    if(dlElement->data) {
		psc = (MedmStripChart *)dlElement->data;
	    } else {
		psc = stripChartAlloc(displayInfo, dlElement);
		if(psc == NULL) {
		    medmPrintf(1, "\nexecuteDlStripChart: Memory allocation error\n");
		    return;
		}

	      /* connect channels */
		j = 0;
		for(i = 0; i < MAX_PENS; i++) {
		    if(dlStripChart->pen[i].chan[0] != '\0') {
			psc->record[j] = NULL;
			psc->record[j] =
			  medmAllocateRecord(dlStripChart->pen[i].chan,
			    stripChartUpdateValueCb,
			    stripChartUpdateGraphicalInfoCb,
			    (XtPointer)psc);
			j++;
		    }
		}
	      /* record the number of channels in the strip chart */
		psc->nChannels = j;

		if(psc->nChannels == 0) {
		  /* if no channel, create a fake channel */
		    psc->nChannels = 1;
		    psc->record[0] = NULL;
		    psc->record[0] = medmAllocateRecord(" ",
		      NULL,
		      stripChartUpdateGraphicalInfoCb,
		      (XtPointer)psc);
		}
	    }

	    XtVaSetValues(localWidget, XmNuserData, (XtPointer) psc, NULL);
	    XtAddCallback(localWidget, XmNexposeCallback, redisplayStrip,
	      (XtPointer)psc);
#if 0
	    XtAddCallback(localWidget, XmNdestroyCallback,
	      (XtCallbackProc)monitorDestroy, (XtPointer)psc);
#endif
	} else if(displayInfo->traversalMode == DL_EDIT) {
	  /* Add expose callback for EDIT mode */
	    XtAddCallback(localWidget, XmNexposeCallback, redisplayFakeStrip,
	      dlStripChart);
	  /* Add handlers */
	    addCommonHandlers(localWidget, displayInfo);
	    XtManageChild(localWidget);
	}
    } else {
	DlObject *po = &(dlElement->structure.stripChart->object);
	XtVaSetValues(dlElement->widget,
	  XmNx, (Position) po->x,
	  XmNy, (Position) po->y,
	  XmNwidth, (Dimension) po->width,
	  XmNheight, (Dimension) po->height,
	  NULL);
    }
}

void hideDlStripChart(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element with a widget */
    hideWidgetElement(displayInfo, dlElement);
}

static void stripChartUpdateGraphicalInfoCb(XtPointer cd) {
    Record *pR = (Record *)cd;
    MedmStripChart *psc = (MedmStripChart *)pR->clientData;
    DlStripChart *dlStripChart = psc->dlElement->structure.stripChart;
    Widget widget = psc->dlElement->widget;
    XcVType hopr, lopr, val;
    short precision;
    int i, row;


#if DEBUG_ACCESS
    print("stripChartUpdateGraphicalInfoCb\n");
#endif

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! This is a temporary work around !!!!! */
  /* !!!!! for the reconnection.           !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  /* KE: This causes it to not do anything for the reconnection */
    medmRecordAddGraphicalInfoCb(pR, NULL);

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! End work around                 !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  /* Find out which record this is */
    row = -1;
    for(i = 0; i < psc->nChannels; i++) {
	if(pR == psc->record[i]) {
	    row = i;
	    break;
	}
    }
    if(row < 0) {
	medmPostMsg(1, "stripChartUpdateGraphicalInfoCb:\n"
	  "  Cannot find this record in this Strip Chart\n");
	return;
    }

    switch (pR->dataType) {
    case DBF_STRING :
	medmPostMsg(1, "stripChartUpdateGraphicalInfoCb:\n"
	  "  Illegal channel type for %s\n"
	  "  Cannot attach stripChart\n",
	  dlStripChart->pen[row].chan);
	return;
    case DBF_ENUM :
    case DBF_CHAR :
    case DBF_INT :
    case DBF_LONG :
    case DBF_FLOAT :
    case DBF_DOUBLE :
	hopr.fval = (float)pR->hopr;
	lopr.fval = (float)pR->lopr;
	val.fval = (float)pR->value;
	precision = pR->precision;
	break;
    default :
	medmPostMsg(1, "stripChartUpdateGraphicalInfoCb:\n"
	  "  Unknown channel type for %s\n"
	  "  Cannot attach stripChart\n",
	  dlStripChart->pen[row].chan);
	break;
    }
    if((hopr.fval == 0.0) && (lopr.fval == 0.0)) {
	hopr.fval += 1.0;
    }

  /* Set the limits */
    if(widget) {
#if DEBUG_RECORDS
	print("%d lopr=%g hopr=%g precion=%hd %s\n",
	  row,lopr.fval,hopr.fval, precision,dlStripChart->pen[row].chan);
#endif
      /* Set Channel and User limits (if apparently not set yet) */
	dlStripChart->pen[row].limits.loprChannel = lopr.fval;
	if(dlStripChart->pen[row].limits.loprSrc != PV_LIMITS_USER &&
	  dlStripChart->pen[row].limits.loprUser == LOPR_DEFAULT) {
	    dlStripChart->pen[row].limits.loprUser = lopr.fval;
	}
	dlStripChart->pen[row].limits.hoprChannel = hopr.fval;
	if(dlStripChart->pen[row].limits.hoprSrc != PV_LIMITS_USER &&
	  dlStripChart->pen[row].limits.hoprUser == HOPR_DEFAULT) {
	    dlStripChart->pen[row].limits.hoprUser = hopr.fval;
	}
	dlStripChart->pen[row].limits.precChannel = precision;
	if(dlStripChart->pen[row].limits.precSrc != PV_LIMITS_USER &&
	  dlStripChart->pen[row].limits.precUser == PREC_DEFAULT) {
	    dlStripChart->pen[row].limits.precUser = precision;
	}
    }

  /* Make sure all the channels get their operating ranges before
     proceeding */
    for(i = 0; i < psc->nChannels; i++) {
	if(psc->record[i]->precision < 0) {
	  /* Wait for other channels */
	    return;
	}
    }

    if(XtIsManaged(widget) == False) {
	addCommonHandlers(widget, psc->updateTask->displayInfo);
	XtManageChild(widget);
	if(!psc->timerid) psc->timerid=XtAppAddTimeOut(
	  XtWidgetToApplicationContext(widget),
	  100, configStripChart, psc);
#if DEBUG_STRIP_CHART
	fprintf(stderr, "stripChartUpdateGraphicalInfoCb(1): psc->timerid=%x\n",
	  psc->timerid);
#endif
    } else if(XtIsRealized(widget)) {
	if(psc->pixmap == (Pixmap)0) {
	    stripChartConfig(psc);
	}
    } else {
      /* do it half second later */
	if(!psc->timerid) psc->timerid=XtAppAddTimeOut(
	  XtWidgetToApplicationContext(widget),
	  100, configStripChart, psc);
#if DEBUG_STRIP_CHART
	fprintf(stderr, "stripChartUpdateGraphicalInfoCb(2): psc->timerid=%x\n",
	  psc->timerid);
#endif
    }
}

static void configStripChart(XtPointer cd,  XtIntervalId *id)
{
    MedmStripChart *psc = (MedmStripChart *) cd;
    Widget widget = psc->dlElement->widget;

    UNREFERENCED(id);

#if DEBUG_STRIP_CHART
    fprintf(stderr,"configStripChart: "
      "psc=%x psc->dlElement=%x widget=%x psc->timerid=%x\n",
      &psc,psc->dlElement,widget,psc->timerid);
    fprintf(stderr,"                  psc->pixmap=%x XtIsRealized(widget)=%d\n",
      psc->pixmap,XtIsRealized(widget));
#endif

    psc->timerid=(XtIntervalId)0;
    if(XtIsRealized(widget)) {
	if(psc->pixmap == (Pixmap)0) {
	    stripChartConfig(psc);
	}
    } else {
      /* do it half second later */
	psc->timerid=XtAppAddTimeOut(XtWidgetToApplicationContext(widget),
	  100, configStripChart, psc);
#if DEBUG_STRIP_CHART
	fprintf(stderr,"configStripChart: psc->timerid=%x\n",
	  psc->timerid);
#endif
    }
}

static void redisplayStrip(Widget widget, XtPointer cd, XtPointer cbs)
{
    MedmStripChart *psc = (MedmStripChart *) cd;
    DlElement *dlElement = psc->dlElement;
    GC gc = psc->gc;

    UNREFERENCED(cbs);

  /*                    STRIP CHART GEOMETRY

			(0,0)
			*--------------------------------------------------|
			|                                                  |
			|                                                  |
			|                                                  |
			|                      TOP                         |
			|                                                  |
			|  (dataX0-1, dataY0-1)                            |
			|----------*---------------------------------------|
			|          |                   ^        |          |
			|          |< - - - dataWidth -+- - - ->|          |
			|          |                   |        |          |
			|          |                   |        |          |
			|          |                            |          |
			|   LEFT   |               dataHeight   |  RIGHT   |
			|          |                            |          |
			|          |                   |        |          |
			|          |                   |        |          |
			|          |                   |        |          |
			|          |                   V        |          |
			|---------------------------------------*----------|
			|                            (dataX1+1,dataY1+1)   |
			|                                                  |
			|                     BOTTOM                       |
			|                                                  |
			|                                                  |
			|                                                  |
			|--------------------------------------------------*
			(w,h)
			*/

#if DEBUG_ACCESS
    print("redisplayStrip\n");
#endif

  /* Check if hidden */
    if(dlElement->hidden) {
	if(widget && XtIsManaged(widget)) {
	    XtUnmanageChild(widget);
	}
	return;
    }

    if(psc->pixmap) {
      /* copy the top region */
	XCopyArea(display, psc->pixmap, XtWindow(widget), gc, 0, 0,
	  psc->w,psc->dataY0, 0, 0);
      /* copy the left region */
	XCopyArea(display, psc->pixmap, XtWindow(widget), gc, 0,
	  psc->dataY0, psc->dataX0, psc->dataHeight, 0, psc->dataY0);
      /* copy the bottom region */
	XCopyArea(display, psc->pixmap, XtWindow(widget), gc, 0,
	  psc->dataY1+1, psc->w, (psc->h - psc->dataY1), 0, psc->dataY1+1);
      /* copy the right region */
	XCopyArea(display, psc->pixmap, XtWindow(widget), gc,
	  psc->dataX1+1, psc->dataY0, (psc->w - psc->dataX1),
	  psc->dataHeight, psc->dataX1+1, psc->dataY0);
      /* draw graph */
	if(psc->updateEnable)
	  stripChartDraw((XtPointer)psc);
    }
}

static void stripChartUpdateValueCb(XtPointer cd) {
    Record *pR = (Record *)cd;
    MedmStripChart *psc = (MedmStripChart *)pR->clientData;
    Boolean connected = True;
    Boolean readAccess = True;
    Boolean validPrecision = True;
    int i;
    Widget widget = psc->dlElement->widget;

    for(i = 0; i< psc->nChannels; i++) {
	Record *ptmp = psc->record[i];
	if(!ptmp->connected) {
	    connected = False;
	    break;
	}
	if(!ptmp->readAccess) {
	    readAccess = False;
	}
	if(ptmp->precision < 0) {
	    validPrecision = False;
	}
    }


#if DEBUG_ACCESS
    print("stripChartUpdateValueCb: connected=%s readAccess=%s"
      " validPrecision=%s\n",
      connected?"True":"False",
      readAccess?"True":"False",
      validPrecision?"True":"False");
#endif

    if(connected) {
	if(readAccess) {
	    if(widget && validPrecision) {
		if(XtIsManaged(widget) == False) {
		    addCommonHandlers(widget, psc->updateTask->displayInfo);
		    XtManageChild(widget);
		  /* specified the time interval */
		    updateTaskSetScanRate(psc->updateTask, psc->timeInterval);
		}
		if(psc->updateEnable)
		  stripChartUpdateGraph((XtPointer)cd);
	    }
	} else {
	    if(widget && XtIsManaged(widget)) {
		XtUnmanageChild(widget);
	      /* Stop periodic update */
		updateTaskSetScanRate(psc->updateTask, 0.0);
	    }
#if DEBUG_ACCESS
	    print("  drawBlackRectangle\n");
#endif
	    drawBlackRectangle(psc->updateTask);
	  /* The black rectangle is drawn on the updatePixmap.  If
	    this routine is called as a result of the
	    ca_replace_access_rights_event call, then the updatePixmap
	    may not be copied. So mark it for update. If you don't do
	    this it may end up white, rather than black.  */
	    updateTaskMarkUpdate(psc->updateTask);
	}
    } else {
	if(widget && XtIsManaged(widget)) {
	    XtUnmanageChild(widget);
	  /* Stop periodic update */
	    updateTaskSetScanRate(psc->updateTask, 0.0);
	}
#if DEBUG_ACCESS
	print("  drawWhiteRectangle\n");
#endif
	drawWhiteRectangle(psc->updateTask);
    }
}

static void stripChartUpdateTaskCb(XtPointer cd) {
    MedmStripChart *psc = (MedmStripChart *) cd;
    Boolean connected = True;
    Boolean readAccess = True;
    Boolean validPrecision = True;
    Widget widget = psc->dlElement->widget;
    int i;

    for(i = 0; i< psc->nChannels; i++) {
	Record *ptmp = psc->record[i];
	if(!ptmp->connected) {
	    connected = False;
	    break;
	}
	if(!ptmp->readAccess) {
	    readAccess = False;
	}
	if(ptmp->precision < 0) {
	    validPrecision = False;
	}
    }

#if DEBUG_ACCESS
    print("stripChartUpdateTaskCb: connected=%s readAccess=%s"
      " validPrecision=%s\n",
      connected?"True":"False",
      readAccess?"True":"False",
      validPrecision?"True":"False");
#endif

    if(connected) {
	if(readAccess) {
	    if(widget && validPrecision) {
		if(XtIsManaged(widget) == False) {
		    addCommonHandlers(widget, psc->updateTask->displayInfo);
		    XtManageChild(widget);
		  /* Specifiy the time interval */
		    updateTaskSetScanRate(psc->updateTask, psc->timeInterval);
		}
		if(psc->updateEnable)
		  stripChartDraw((XtPointer)psc);
	    }
	} else {
	    if(widget && XtIsManaged(widget)) {
		XtUnmanageChild(widget);
	      /* Stop periodic update */
		updateTaskSetScanRate(psc->updateTask, 0.0);
	    }
#if DEBUG_ACCESS
	    print("  drawBlackRectangle\n");
#endif
	    drawBlackRectangle(psc->updateTask);
	}
    } else {
	if(widget && XtIsManaged(widget)) {
	    XtUnmanageChild(widget);
	  /* Stop periodic update */
	    updateTaskSetScanRate(psc->updateTask, 0.0);
	}
#if DEBUG_ACCESS
	print("  drawWhiteRectangle\n");
#endif
	drawWhiteRectangle(psc->updateTask);
    }
}

static void stripChartUpdateGraph(XtPointer cd) {
    Record *pR = (Record *)cd;
    MedmStripChart *psc = (MedmStripChart *)pR->clientData;
    double currentTime, lopr, hopr;
    int n = 0;

  /* get the time */
    currentTime = medmTime();

    while(psc->record[n] != pR) n++;

  /* See whether it is time to advance the graph */
    if(currentTime > psc->nextAdvanceTime) {
      /* update the graph */
	GC gc = psc->gc;
	DlStripChart *dlStripChart = psc->dlElement->structure.stripChart;
	DisplayInfo *displayInfo  = psc->updateTask->displayInfo;
	int i;
	int totalPixel;    /* number of pixel needed to be drawn */


      /* Calculate how many pixels need to be drawn */
	totalPixel = 1 + (int)((currentTime - psc->nextAdvanceTime) /
	  psc->timeInterval);
	psc->nextAdvanceTime += psc->timeInterval * totalPixel;
#if 0
	while(psc->nextAdvanceTime <= currentTime) {
	    psc->nextAdvanceTime += psc->timeInterval;
	    totalPixel++;
	}
#endif
	if(totalPixel > 0) {
	    XRectangle rectangle;
	    rectangle.x = psc->dataX0;
	    rectangle.y = psc->dataY0;
	    rectangle.width = psc->dataWidth;
	    rectangle.height = psc->dataHeight;
	    XSetClipRectangles(display, gc, 0, 0, &rectangle, 1, YXBanded);
	  /* draw the background */
	    XSetForeground(display, gc,
	      displayInfo->colormap[dlStripChart->plotcom.bclr]);
	    if(totalPixel <= 1) {
		XDrawLine(display, psc->pixmap, gc,
		  psc->nextSlot + psc->dataX0, psc->dataY0,
		  psc->nextSlot + psc->dataX0, psc->dataY1);
	    } else {
		int limit = psc->dataWidth + psc->dataX0;
		int nextSlot = psc->nextSlot + psc->dataX0;

		if((nextSlot + totalPixel) > limit) {
		  /* if wrapped, two fills are needed */
		    XFillRectangle(display, psc->pixmap, gc,
		      nextSlot, psc->dataY0, (limit-nextSlot), psc->dataHeight);
		    XFillRectangle(display, psc->pixmap, gc,
		      psc->dataX0, psc->dataY0, totalPixel-limit+nextSlot,
		      psc->dataHeight);
		} else {
		  /* if not wrapped, do one fill */
		    XFillRectangle(display, psc->pixmap, gc,
		      nextSlot, psc->dataY0, totalPixel, psc->dataHeight);
		}
	    }

	  /* draw each pen */
	    for(i=0; i< psc->nChannels; i++) {
		int y1, y2;
		double base;
		int nextSlot = psc->nextSlot + psc->dataX0;
		int limit = psc->dataWidth + psc->dataX0;

		updatePvLimits(&dlStripChart->pen[i].limits);
		lopr = dlStripChart->pen[i].limits.lopr;
		hopr = dlStripChart->pen[i].limits.hopr;

	      /* plot data */
		base = hopr - lopr;
		y1 =  psc->dataY0 + (int)((psc->dataHeight - 1) *
		  (1.0 - ((psc->minVal[i] - lopr) / base)));
		y2 =  psc->dataY0 + (int)((psc->dataHeight - 1) *
		  (1.0 - ((psc->maxVal[i] - lopr) / base)));

		XSetForeground(display, gc,
		  displayInfo->colormap[dlStripChart->pen[i].clr]);
		XDrawLine(display, psc->pixmap, gc, nextSlot, y1, nextSlot, y2);

		if(totalPixel > 1) {
		    int y;
		    y =  psc->dataY0 + (int)((psc->dataHeight - 1) *
		      (1.0 - ((psc->value[i] - lopr) / base)));
		  /* fill the gap in between */
		    if((nextSlot + totalPixel) > limit) {
		      /* if wraped, two lines are needed */
			XDrawLine(display, psc->pixmap, gc,
			  nextSlot, y, limit, y);
			XDrawLine(display, psc->pixmap, gc,
			  psc->dataX0, y, totalPixel-limit+nextSlot+psc->dataX0, y);
		    } else {
		      /* not wraped, one line only */
			XDrawLine(display, psc->pixmap, gc,
			  nextSlot, y, (nextSlot + totalPixel), y);
		    }
		}
	      /* reset max, min to the last pen position */
		psc->maxVal[i] = psc->minVal[i] = psc->value[i];
	    }
	    XSetClipOrigin(display, gc, 0, 0);
	    XSetClipMask(display, gc, None);
	    psc->nextSlot += totalPixel;
	    if(psc->dataWidth > 0) {
		while(psc->nextSlot >= (int)psc->dataWidth)
		  psc->nextSlot -= psc->dataWidth;
	    } else {
		psc->nextSlot = 0;
	    }
	}
    }
  /* remember the min and max and current pen position */
    psc->value[n] = pR->value;
    if(pR->value > psc->maxVal[n]) {
	psc->maxVal[n] = pR->value;
    }
    if(pR->value < psc->minVal[n]) {
	psc->minVal[n] = pR->value;
    }
    return;
}

static void stripChartDraw(XtPointer cd) {
    MedmStripChart *psc = (MedmStripChart *) cd;
    DlElement *dlElement = psc->dlElement;
    Widget widget = dlElement->widget;
    Window window = XtWindow(widget);
    GC gc = psc->gc;
    int startPos;

#if DEBUG_ACCESS
    print("stripChartDraw\n");
#endif

  /* Check if hidden */
    if(dlElement->hidden) {
	if(widget && XtIsManaged(widget)) {
	    XtUnmanageChild(widget);
	}
	return;
    }

  /* Make sure the plot is up to date */
    stripChartUpdateValueCb((XtPointer)psc->record[0]);
    startPos = psc->nextSlot;
    if(startPos > (int)psc->dataWidth) {
	startPos = 0;
    }
#if DEBUG_TIMEOUT
    {
	static double lastTime = 0;
	double now=medmTime();
	double delta = now-lastTime;
	print("stripChartDraw: delta=%f\n"
	  " nextSlot=%d dataWidth=%d startPos=%d\n",
	  delta,psc->nextSlot,psc->dataWidth,startPos);
	lastTime=now;
    }
#endif
    if(startPos) {
	int x, w;
      /* copy the first half */
	x = startPos + psc->dataX0;
	w = psc->dataWidth - startPos;
	XCopyArea(display, psc->pixmap, window, gc,
	  x, psc->dataY0, w, psc->dataHeight,
	  psc->dataX0, psc->dataY0);
      /* copy the second half */
	x = w + psc->dataX0;
	XCopyArea(display, psc->pixmap, window, gc,
	  psc->dataX0, psc->dataY0, startPos, psc->dataHeight,
	  x, psc->dataY0);
    } else {
	XCopyArea(display, psc->pixmap, window, gc,
	  psc->dataX0, psc->dataY0, psc->dataWidth, psc->dataHeight,
	  psc->dataX0, psc->dataY0);
    }
}

static void stripChartGetRecord(XtPointer cd, Record **record, int *count)
{
    MedmStripChart *psc = (MedmStripChart *) cd;
    int i;

    *count = psc->nChannels;
    for(i = 0; i < psc->nChannels; i++) {
	record[i] = psc->record[i];
    }
}

DlElement *createDlStripChart(DlElement *p)
{
    DlStripChart *dlStripChart;
    DlElement *dlElement;
    int penNumber;


    dlStripChart = (DlStripChart *)malloc(sizeof(DlStripChart));
    if(!dlStripChart) return 0;
    if(p) {
	*dlStripChart = *p->structure.stripChart;
    } else {
	objectAttributeInit(&(dlStripChart->object));
	plotcomAttributeInit(&(dlStripChart->plotcom));
	dlStripChart->period = SC_DEFAULT_PERIOD;
	dlStripChart->units = SC_DEFAULT_UNITS;
#if 1
      /* For backward compatibility */
	dlStripChart->delay = -1.0;
	dlStripChart->oldUnits = SC_DEFAULT_UNITS;
#endif
	for(penNumber = 0; penNumber < MAX_PENS; penNumber++) {
	    penAttributeInit(&(dlStripChart->pen[penNumber]));
	}
    }

    if(!(dlElement = createDlElement(DL_StripChart,
      (XtPointer)dlStripChart,
      &stripChartDlDispatchTable))) {
	free(dlStripChart);
    }

    return(dlElement);
}

DlElement *parseStripChart(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlStripChart *dlStripChart;
    DlElement *dlElement = createDlStripChart(NULL);
    int penNumber;
    int isVersion2_1_x = False;

    if(!dlElement) return 0;
    dlStripChart = dlElement->structure.stripChart;

    do {
	switch( (tokenType=getToken(displayInfo, token)) ) {
	case T_WORD:
	    if(!strcmp(token, "object"))
	      parseObject(displayInfo, &(dlStripChart->object));
	    else if(!strcmp(token, "plotcom"))
	      parsePlotcom(displayInfo, &(dlStripChart->plotcom));
	    else if(!strcmp(token, "period")) {
		getToken(displayInfo, token);
		getToken(displayInfo, token);
		dlStripChart->period = atof(token);
		isVersion2_1_x = True;
	    } else if(!strcmp(token, "delay")) {
		getToken(displayInfo, token);
		getToken(displayInfo, token);
		dlStripChart->delay = atoi(token);
	    } else if(!strcmp(token, "units")) {
		getToken(displayInfo, token);
		getToken(displayInfo, token);
		if(!strcmp(token, "minute"))
		  dlStripChart->units = MINUTES;
		else if(!strcmp(token, "second"))
		  dlStripChart->units = SECONDS;
		else if(!strcmp(token, "milli second"))
		  dlStripChart->units = MILLISECONDS;
		else if(!strcmp(token, "milli-second"))
		  dlStripChart->units = MILLISECONDS;
		else
		  medmPostMsg(1, "parseStripChart: Illegal units %s\n"
		    "  Using: %s\n", token, stringValueTable[SC_DEFAULT_UNITS]);
	    } else if(!strncmp(token, "pen", 3)) {
		penNumber = MIN(token[4] - '0', MAX_PENS-1);
		parsePen(displayInfo, &(dlStripChart->pen[penNumber]));
	    }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++;
	    break;
	case T_RIGHT_BRACE:
	    nestingLevel--;
	    break;
	default:
	    break;
	}
    } while( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );

    if(isVersion2_1_x) {
	dlStripChart->delay = -1.0;  /* -1.0 is used as a indicator to save
					as new format */
    } else
      if(dlStripChart->delay > 0) {
	  double val, dummy1, dummy2;
	  switch (dlStripChart->units) {
	  case MILLISECONDS:
	      dummy1 = -0.060 * (double) dlStripChart->delay;
	      break;
	  case SECONDS:
	      dummy1 = -60 * (double) dlStripChart->delay;
	      break;
	  case MINUTES:
	      dummy1 = -3600.0 * (double) dlStripChart->delay;
	      break;
	  default:
	      dummy1 = -60 * (double) dlStripChart->delay;
	      break;
	  }

	  linear_scale(dummy1, 0.0, 2, &val, &dummy1, &dummy2);
	  dlStripChart->period = -val;
	  dlStripChart->oldUnits = dlStripChart->units;
	  dlStripChart->units = SC_DEFAULT_UNITS;
      }

    return dlElement;

}

void writeDlStripChart( FILE *stream, DlElement *dlElement, int level) {
    int i;
    char indent[16];
    DlStripChart *dlStripChart = dlElement->structure.stripChart;

#if 1
  /* for the compatibility */

    if(dlStripChart->delay > 0.0) {
	double val, dummy1, dummy2;
	switch (dlStripChart->oldUnits) {
	case MILLISECONDS:
	    dummy1 = -0.060 * (double) dlStripChart->delay;
	    break;
	case SECONDS:
	    dummy1 = -60 * (double) dlStripChart->delay;
	    break;
	case MINUTES:
	    dummy1 = -3600.0 * (double) dlStripChart->delay;
	    break;
	default:
	    dummy1 = -60 * (double) dlStripChart->delay;
	    break;
	}

	linear_scale(dummy1, 0.0, 2, &val, &dummy1, &dummy2);
	if(dlStripChart->period != -val  || dlStripChart->units != SC_DEFAULT_UNITS) {
	    dlStripChart->delay = -1;
	}
    }
#endif


    memset(indent, '\t', level);
    indent[level] = '\0';

    fprintf(stream, "\n%s\"strip chart\" {", indent);
    writeDlObject(stream, &(dlStripChart->object), level+1);
    writeDlPlotcom(stream, &(dlStripChart->plotcom), level+1);
    if(dlStripChart->delay < 0.0) {
	if(dlStripChart->period != SC_DEFAULT_PERIOD)
	  fprintf(stream, "\n%s\tperiod=%f", indent, dlStripChart->period);
#ifdef SUPPORT_0201XX_FILE_FORMAT
	if(MedmUseNewFileFormat) {
#endif
	    if(dlStripChart->units != SC_DEFAULT_UNITS)
	      fprintf(stream, "\n%s\tunits=\"%s\"", indent,
		stringValueTable[dlStripChart->units]);
#ifdef SUPPORT_0201XX_FILE_FORMAT
	} else {
	    fprintf(stream, "\n%s\tunits=\"%s\"", indent,
	      stringValueTable[dlStripChart->units]);
	}
#endif
    } else {
#if 1
      /* for the compatibility */
	fprintf(stream, "\n%s\tdelay=%f", indent, dlStripChart->delay);
	fprintf(stream, "\n%s\tunits=\"%s\"", indent,
	  stringValueTable[dlStripChart->oldUnits]);
#endif
    }
    for(i = 0; i < MAX_PENS; i++) {
	writeDlPen(stream, &(dlStripChart->pen[i]), i, level+1);
    }
    fprintf(stream, "\n%s}", indent);
}

static void stripChartInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlStripChart *dlStripChart = p->structure.stripChart;
    medmGetValues(pRCB,
      CLR_RC,        &(dlStripChart->plotcom.clr),
      BCLR_RC,       &(dlStripChart->plotcom.bclr),
      -1);
}

static void stripChartGetLimits(DlElement *pE, DlLimits **ppL, char **pN)
{
    DlStripChart *dlStripChart = pE->structure.stripChart;
    int i;

  /* The user needs to have given us an array of MAX_PENS pointers */
    if(!ppL && !pN) return;

    for(i = 0; i < MAX_PENS; i++) {
	ppL[i] = &(dlStripChart->pen[i].limits);
	pN[i] = dlStripChart->pen[i].chan;
    }
}

static void stripChartGetValues(ResourceBundle *pRCB, DlElement *p)
{
    DlStripChart *dlStripChart = p->structure.stripChart;

    medmGetValues(pRCB,
      X_RC,          &(dlStripChart->object.x),
      Y_RC,          &(dlStripChart->object.y),
      WIDTH_RC,      &(dlStripChart->object.width),
      HEIGHT_RC,     &(dlStripChart->object.height),
      TITLE_RC,      &(dlStripChart->plotcom.title),
      XLABEL_RC,     &(dlStripChart->plotcom.xlabel),
      YLABEL_RC,     &(dlStripChart->plotcom.ylabel),
      CLR_RC,        &(dlStripChart->plotcom.clr),
      BCLR_RC,       &(dlStripChart->plotcom.bclr),
      PERIOD_RC,     &(dlStripChart->period),
      UNITS_RC,      &(dlStripChart->units),
      SCDATA_RC,     &(dlStripChart->pen),
      -1);
}

static void stripChartSetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlStripChart *dlStripChart = p->structure.stripChart;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlStripChart->plotcom.bclr),
      -1);
}

static void stripChartSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlStripChart *dlStripChart = p->structure.stripChart;
    medmGetValues(pRCB,
      CLR_RC,        &(dlStripChart->plotcom.clr),
      -1);
}

/* KE: Formerly in graphX/algorithms.c */
static void linear_scale(double xmin, double xmax, int n,
  double *xminp, double *xmaxp, double *dist)
{
    static double vint[4] = { 1.0, 2.0, 5.0, 10.0, };
    static double sqr[3]  = { 1.414214, 3.162278, 7.071068, };
    double del = 0.0000002;		/* account for round-off errors */
    int nal, i, m1, m2;
    double fn, a, al, b, fm1, fm2;

  /* Check whether proper input values were supplied */
    if(!(xmin <= xmax && n > 0)) {
	fprintf(stderr,"\nlinear_scale: improper input values supplied!");
	return;
    }

  /* Provide 10% spread if graph is parallel to an axis */
    if(xmax == xmin) {
	xmax *= 1.05;
	xmin *= 0.95;
    }
    fn = n;

  /* Find approximate interval size */
    a = (xmax - xmin)/(double)fn;
    al = log10(a);
    nal = (int) al;

    if(a < 1.0) nal -= 1;

  /* a is scaled into variable named b betwen 1 and 10 */
    b = a / ((double)pow(10.0, (double) nal));

  /* The closest permissible value for b is found */
    for(i = 1; i < 4; i++) {
	if (b < sqr[i-1]) goto label30;
    }
    i = 4;

  /* The interval size is computed */
  label30:
    *dist = vint[i-1]*((double)pow(10.0, (double)nal));
    fm1 = xmin/(*dist);
    m1 = (int)fm1;
    if(fm1 < 0.0) m1 -= 1;
    if(fabs(((double) m1) + 1.0 - ((double) fm1)) < ((double)del))
      m1 += 1;

  /* The new min and max limits are found */
    *xminp = (*dist) * ((double) m1);
    fm2 = xmax/(*dist);
    m2 = ((int)fm2) + 1;
    if(fm2 < -1.0) m2 -= 1;
    if( fabs((((double)fm2) + 1.0 - ((double)m2))) < del)
      m2 -= 1;
    *xmaxp = (*dist)*((double)m2);

  /* Adjust limits to account for round-off if necessary */
    if((double)(*xminp) > xmin)
      *xminp = xmin;
    if((double)(*xmaxp) < xmax)
      *xmaxp = xmax;

}

/* Creates the dialog if needed, fills in the values, and pops it up */
void popupStripChartDataDialog(void)
{
    DlStripChart *dlStripChart;
    int i;

  /* Create the dialog if necessary */
    if(!stripChartS) {
	if(!stripChartS) {
	    stripChartDialogCreateDialog();
	    if(!stripChartS) {
		medmPostMsg(1, "popupStripChartDataDialog: "
		  "Cannot create Strip Chart data dialog\n");
		return;
	    }
	}
    }

  /* Fill in the tempPen depending on EDIT or EXECUTE */
    if(globalDisplayListTraversalMode == DL_EDIT) {
      /* EDIT */
	for (i = 0; i < MAX_PENS; i++) {
	    tempPen[i] = globalResourceBundle.scData[i];
	}
    } else {
      /* Check if executeTimeStripChartElement is still valid
       *   It should be, but be safe */
	if(executeTimeStripChartElement == NULL) {
	    medmPostMsg(1, "stripChartDialogCb: Element is no longer valid\n");
	    XtPopdown(stripChartS);
	    return;
	}
	dlStripChart = executeTimeStripChartElement->structure.stripChart;
	for (i = 0; i < MAX_PENS; i++) {
	    tempPen[i] = dlStripChart->pen[i];
	}
	tempPeriod = dlStripChart->period;
	tempUnits = dlStripChart->units;
    }

#if DEBUG_DIALOG
    printPens(tempPen,"popupStripChartDataDialog (1):");
#endif

  /* Fill in the values */
    stripChartDialogReset();

#if DEBUG_DIALOG
    printPens(tempPen,"popupStripChartDataDialog (2):");
#endif

  /* Pop it up */
    XtSetSensitive(stripChartS, True);
    XtPopup(stripChartS, XtGrabNone);

  /* Show the execute controls only in EXECUTE mode */
    if(globalDisplayListTraversalMode == DL_EDIT) {
	XtUnmanageChild(scExecuteControlsW);
	XtSetSensitive(scExecuteControlsW, False);
#if 0
	XtUnmanageChild(scPeriodLabelW);
	XtUnmanageChild(scPeriodW);
	XtUnmanageChild(scUnitsW);
#endif
    } else {
	XtSetSensitive(scExecuteControlsW, True);
	XtManageChild(scExecuteControlsW);
#if 0
	XtManageChild(scPeriodLabelW);
	XtManageChild(scPeriodW);
	XtManageChild(scUnitsW);
#endif
    }
}

/* Udates the colors in the COLOR_COLUMN of the matrix */
void stripChartUpdateMatrixColors(int clr, int row)
{
    Widget w;
    Pixel pixel;
    Pixel *colormap =currentDisplayInfo->colormap;

    tempPen[row].clr = clr;
    w = table[row][SC_COLOR_COLUMN];
    pixel = colormap[tempPen[row].clr];
#if DEBUG_COLOR
    print("stripChartUpdateMatrixColors: clr=%d row=%d pixel=%x w=%x\n",
      clr,row,pixel,w);
#endif
    if(w) XtVaSetValues(w, XmNbackground, pixel, NULL);
}

/* Creates the strip chart data dialog */
static void stripChartDialogCreateDialog(void)
{
    Widget w, wparent;
    Widget columns[SC_COLS], labels[SC_COLS];
    XmString label, opt1, opt2, opt3;
    int i, j;
    static Boolean first = True;

    if(stripChartS != NULL || mainShell == NULL) return;

    stripChartS = XtVaCreatePopupShell("stripChartS",
      topLevelShellWidgetClass, mainShell,
      XmNtitle, "Strip Chart Data",
      XmNdeleteResponse, XmDO_NOTHING,
#if OMIT_RESIZE_HANDLES
      XmNmwmDecorations, MWM_DECOR_ALL|MWM_DECOR_RESIZEH,
    /* KE: The following is necessary for Exceed, which turns off the
       resize function with the handles.  It should not be necessary */
      XmNmwmFunctions, MWM_FUNC_ALL,
#endif
    /* This appears to be necessary since apparently the scMatrix does
       some resizing on its own and the shell doesn't adjust on some
       platforms */
      XmNallowShellResize, True,
      NULL);
    XmAddWMProtocolCallback(stripChartS, WM_DELETE_WINDOW,
      wmCloseCallback, (XtPointer)OTHER_SHELL);

  /* Initialize globals that store the temporary values until apply */
    if(first) {
	first = False;
	for (i = 0; i < MAX_PENS; i++) {
	    for (j = 0; j < SC_COLS; j++) table[i][j] = NULL;
	    stripChartUpdateMatrixColors(SC_DEFAULT_CLR, i);
	}
	tempPeriod = SC_DEFAULT_PERIOD;
	tempUnits = SC_DEFAULT_UNITS;
    }

  /*
   * Create the interface
   *
   *        scMatrix
   *	       channel | color | losrc | loval | hisrc | hival
   *	       -----------------------------------------------
   *	    1 |   0        1       2       3       4       5
   *	    2 |
   *	    3 |
   *		     ...
   *        scExecuteControls
   *		 Period    Units
   *        scActionArea
   *		 OK     CANCEL    HELP
   */

  /* Create a form for everything (scForm) */
    scForm = XtVaCreateWidget("scForm",
      xmFormWidgetClass, stripChartS,
      NULL);

  /* Create a row column for the matrix (scMatrix) */
    scMatrixW = XtVaCreateWidget("scMatrix",
      xmRowColumnWidgetClass, scForm,
      XmNpacking, XmPACK_TIGHT,
      XmNorientation, XmHORIZONTAL,
      XmNtopAttachment, XmATTACH_FORM,
      XmNleftAttachment, XmATTACH_FORM,
      XmNrightAttachment, XmATTACH_FORM,
      NULL);

    for(j=0; j < SC_COLS; j++) {
      /* Create a form for each column */
	w = XtVaCreateManagedWidget("columnF",
	  xmFormWidgetClass, scMatrixW,
	  XmNfractionBase, MAX_PENS + 1,
	  NULL);
	columns[j] = wparent = w;

      /* Create a column label */
	w = XtVaCreateManagedWidget(scColumnLabels[j],
	  xmLabelWidgetClass, wparent,
	  XmNcolumns, scColumnWidths[j],
	  XmNalignment, XmALIGNMENT_CENTER,
	  XmNtopAttachment, XmATTACH_POSITION,
	  XmNbottomAttachment, XmATTACH_POSITION,
	  XmNleftAttachment, XmATTACH_FORM,
	  XmNrightAttachment, XmATTACH_FORM,
	  XmNtopPosition, 0,
	  XmNbottomPosition, 1,
	  NULL);
	labels[j] = w;

	for(i=0; i < MAX_PENS; i++) {
	    switch(j) {
	    case 0:
		w = XtVaCreateManagedWidget("channel",
		  xmTextFieldWidgetClass, wparent,
		  XmNmaxLength, MAX_TOKEN_LENGTH-1,
		  XmNcolumns, scColumnWidths[0],
		  XmNtopAttachment, XmATTACH_POSITION,
		  XmNbottomAttachment, XmATTACH_POSITION,
		  XmNleftAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_FORM,
		  XmNtopPosition, i + 1,
		  XmNbottomPosition, i + 2,
		  NULL);
		XtAddCallback(w, XmNactivateCallback, stripChartDialogCb,
		  (XtPointer)(SC_CHAN_BTN + SETHIGH(i)));
		table[i][0] = w;
		break;
	    case 1:
		w = XtVaCreateManagedWidget("color",
		  xmDrawnButtonWidgetClass, wparent,
		  XmNshadowType, XmSHADOW_IN,
		  XmNtopAttachment, XmATTACH_POSITION,
		  XmNbottomAttachment, XmATTACH_POSITION,
		  XmNleftAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_FORM,
		  XmNtopPosition, i + 1,
		  XmNbottomPosition, i + 2,
		  NULL);
		XtAddCallback(w, XmNactivateCallback, stripChartDialogCb,
		  (XtPointer)(SC_CLR_BTN + SETHIGH(i)));
		table[i][1] = w;
		break;
	    case 2:
		label = XmStringCreateLocalized("Source");
		opt1 = XmStringCreateLocalized("Channel");
		opt2 = XmStringCreateLocalized("Default");
		opt3 = XmStringCreateLocalized("User Specified");
		w = XmVaCreateSimpleOptionMenu(wparent, "lowSource",
		  label, '\0', 0, stripChartDialogCb,
		  XmVaPUSHBUTTON, opt1, '\0', NULL, NULL,
		  XmVaPUSHBUTTON, opt2, '\0', NULL, NULL,
		  XmVaPUSHBUTTON, opt3, '\0', NULL, NULL,
		  XmNuserData, (XtPointer)(SC_LOPR_SRC_BTN + SETHIGH(i)),
		  XmNtopAttachment, XmATTACH_POSITION,
		  XmNbottomAttachment, XmATTACH_POSITION,
		  XmNleftAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_FORM,
		  XmNtopPosition, i + 1,
		  XmNbottomPosition, i + 2,
		  NULL);
		XtManageChild(w);
		optionMenuRemoveLabel(w);
		XmStringFree(label);
		XmStringFree(opt1);
		XmStringFree(opt2);
		XmStringFree(opt3);
		table[i][2] = w;
		break;
	    case 3:
		w = XtVaCreateManagedWidget("lowValue",
		  xmTextFieldWidgetClass, wparent,
		  XmNmaxLength, MAX_TOKEN_LENGTH-1,
		  XmNcolumns, scColumnWidths[3],
		  XmNtopAttachment, XmATTACH_POSITION,
		  XmNbottomAttachment, XmATTACH_POSITION,
		  XmNleftAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_FORM,
		  XmNtopPosition, i + 1,
		  XmNbottomPosition, i + 2,
		  NULL);
		XtAddCallback(w, XmNactivateCallback, stripChartDialogCb,
		  (XtPointer)(SC_LOPR_BTN + SETHIGH(i)));
		XtAddCallback(w, XmNmodifyVerifyCallback,
		  textFieldFloatVerifyCallback, NULL);
		table[i][3] = w;
		break;
	    case 4:
		label = XmStringCreateLocalized("Source:");
		opt1 = XmStringCreateLocalized("Channel");
		opt2 = XmStringCreateLocalized("Default");
		opt3 = XmStringCreateLocalized("User Specified");
		w = XmVaCreateSimpleOptionMenu(wparent, "highSource",
		  label, '\0', 0, stripChartDialogCb,
		  XmVaPUSHBUTTON, opt1, '\0', NULL, NULL,
		  XmVaPUSHBUTTON, opt2, '\0', NULL, NULL,
		  XmVaPUSHBUTTON, opt3, '\0', NULL, NULL,
		  XmNuserData, (XtPointer)(SC_HOPR_SRC_BTN + SETHIGH(i)),
		  XmNtopAttachment, XmATTACH_POSITION,
		  XmNbottomAttachment, XmATTACH_POSITION,
		  XmNleftAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_FORM,
		  XmNtopPosition, i + 1,
		  XmNbottomPosition, i + 2,
		  NULL);
		XtManageChild(w);
		optionMenuRemoveLabel(w);
		XmStringFree(label);
		XmStringFree(opt1);
		XmStringFree(opt2);
		XmStringFree(opt3);
		table[i][4] = w;
		break;
	    case 5:
		w = XtVaCreateManagedWidget("highValue",
		  xmTextFieldWidgetClass, wparent,
		  XmNmaxLength, MAX_TOKEN_LENGTH-1,
		  XmNcolumns, scColumnWidths[5],
		  XmNtopAttachment, XmATTACH_POSITION,
		  XmNbottomAttachment, XmATTACH_POSITION,
		  XmNleftAttachment, XmATTACH_FORM,
		  XmNrightAttachment, XmATTACH_FORM,
		  XmNtopPosition, i + 1,
		  XmNbottomPosition, i + 2,
		  NULL);
		XtAddCallback(w, XmNactivateCallback, stripChartDialogCb,
		  (XtPointer)(SC_HOPR_BTN + SETHIGH(i)));
		XtAddCallback(w, XmNmodifyVerifyCallback,
		  textFieldFloatVerifyCallback, NULL);
		table[i][5] = w;
	    }
	}
    }

  /* Create execute controls */
    w = XtVaCreateWidget("scExecuteControls",
      xmRowColumnWidgetClass, scForm,
      XmNpacking, XmPACK_TIGHT,
      XmNorientation, XmHORIZONTAL,
      XmNtopAttachment, XmATTACH_WIDGET,
      XmNtopWidget, scMatrixW,
      XmNleftAttachment, XmATTACH_FORM,
      XmNrightAttachment, XmATTACH_FORM,
      NULL);
    scExecuteControlsW = wparent = w;

  /* Period label */
    w = XtVaCreateManagedWidget("Period:",
      xmLabelWidgetClass, wparent,
      NULL);
    scPeriodLabelW = w;

  /* Period entry */
    w = XtVaCreateManagedWidget("period",
      xmTextFieldWidgetClass, wparent,
      XmNmaxLength, MAX_TOKEN_LENGTH-1,
      NULL);
    XtAddCallback(w, XmNactivateCallback, stripChartDialogCb,
      (XtPointer)SC_PERIOD_BTN);
    XtAddCallback(w, XmNmodifyVerifyCallback,
      textFieldNumericVerifyCallback, NULL);
    scPeriodW = w;

  /* Units */
    label = XmStringCreateLocalized("Units");
    opt1 = XmStringCreateLocalized(stringValueTable[FIRST_TIME_UNIT]);
    opt2 = XmStringCreateLocalized(stringValueTable[FIRST_TIME_UNIT+1]);
    opt3 = XmStringCreateLocalized(stringValueTable[FIRST_TIME_UNIT+2]);
    w = XmVaCreateSimpleOptionMenu(wparent, "units",
      label, '\0', 0, stripChartDialogCb,
      XmVaPUSHBUTTON, opt1, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt2, '\0', NULL, NULL,
      XmVaPUSHBUTTON, opt3, '\0', NULL, NULL,
      XmNuserData, (XtPointer)SC_UNITS_BTN,
      NULL);
    XtManageChild(w);
    XmStringFree(label);
    XmStringFree(opt1);
    XmStringFree(opt2);
    XmStringFree(opt3);
    scUnitsW = w;

  /* Create action area */
    w = XtVaCreateWidget("scActionArea",
      xmFormWidgetClass, scForm,
      XmNshadowThickness, 0,
      XmNfractionBase, 7,
      XmNtopAttachment, XmATTACH_WIDGET,
      XmNtopWidget, scExecuteControlsW,
      XmNleftAttachment, XmATTACH_FORM,
      XmNrightAttachment, XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      NULL);
    scActionAreaW = wparent = w;

    w = XtVaCreateManagedWidget("Apply",
      xmPushButtonWidgetClass, wparent,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     1,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    2,
      NULL);
    XtAddCallback(w, XmNactivateCallback,
      stripChartDialogCb, (XtPointer)SC_APPLY_BTN);
    scApplyButtonW = w;

    w = XtVaCreateManagedWidget("Cancel",
      xmPushButtonWidgetClass, wparent,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     3,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    4,
      NULL);
    XtAddCallback(w, XmNactivateCallback,
      stripChartDialogCb, (XtPointer)SC_CANCEL_BTN);
    scCancelButtonW = w;

    w = XtVaCreateManagedWidget("Help",
      xmPushButtonWidgetClass, wparent,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     5,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    6,
      NULL);
    XtAddCallback(w, XmNactivateCallback,
      stripChartDialogCb, (XtPointer)SC_HELP_BTN);

  /* Manage the managers */
    XtManageChild(scMatrixW);
    XtManageChild(scExecuteControlsW);
    XtManageChild(scActionAreaW);
    XtManageChild(scForm);

    return;
}

/* Sets values into the dialog from the tempPen */
static void stripChartDialogReset()
{
    Widget w;
    Pixel pixel;
    int i;
    DlPen *pPen;
    DlLimits *pL;
    char string[MAX_TOKEN_LENGTH];
    Pixel *colormap =currentDisplayInfo->colormap;

  /* Loop over rows */
    for (i = 0; i < MAX_PENS; i++) {
      /*  Set the pointers */
	pPen = &tempPen[i];
	pL = &pPen->limits;

      /* Update the limits to reflect current src's */
	updatePvLimits(pL);

      /* Channel */
	w = table[i][0];
	XmTextFieldSetString(table[i][0], pPen->chan);
	if(globalDisplayListTraversalMode == DL_EDIT) {
	    XtVaSetValues(w, XmNeditable, True, NULL);
	} else {
	    XtVaSetValues(w, XmNeditable, False, NULL);
	}

      /* Color */
	w = table[i][1];
	if(*tempPen[i].chan) {
	  /* Use the specified color */
	    pixel = colormap[tempPen[i].clr];
	} else {
	  /* Use the background color of the stripChartS */
	    XtVaGetValues(stripChartS, XmNbackground, &pixel, NULL);
	}
	if(w) XtVaSetValues(w, XmNbackground, pixel, NULL);

      /* LOPR */
	switch(pL->loprSrc) {
	case PV_LIMITS_CHANNEL:
	case PV_LIMITS_DEFAULT:
	case PV_LIMITS_USER:
	    if(globalDisplayListTraversalMode == DL_EDIT) {
		if(pL->loprSrc == PV_LIMITS_USER) {
		    pL->loprSrc = PV_LIMITS_DEFAULT;
		    pL->lopr = pL->loprDefault;
		}
	    }
	    w = table[i][2];
	    XtSetSensitive(w, True);
	    optionMenuSet(w, pL->loprSrc-FIRST_PV_LIMITS_SRC);
	    XtSetSensitive(XtParent(table[i][3]), True);
	    w = table[i][3];
	    if(globalDisplayListTraversalMode == DL_EDIT) {
		XtVaSetValues(w, XmNeditable,
		  (pL->loprSrc == PV_LIMITS_DEFAULT)?True:False, NULL);
	    } else {
		XtVaSetValues(w, XmNeditable,
		  (pL->loprSrc == PV_LIMITS_USER)?True:False, NULL);
	    }
	    cvtDoubleToString(pL->lopr, string, SC_PREC(pL));
	    XmTextFieldSetString(w, string);
	    break;
	case PV_LIMITS_UNUSED:
	    w = table[i][2];
	    optionMenuSet(w, PV_LIMITS_CHANNEL-FIRST_PV_LIMITS_SRC);
	    XtSetSensitive(w, False);
	    w = table[i][2];
	    cvtDoubleToString(pL->loprChannel, string, SC_PREC(pL));
	    XmTextFieldSetString(w, string);
	    XtSetSensitive(XtParent(w), False);
	    break;
	}

      /* HOPR */
	switch(pL->hoprSrc) {
	case PV_LIMITS_CHANNEL:
	case PV_LIMITS_DEFAULT:
	case PV_LIMITS_USER:
	    if(globalDisplayListTraversalMode == DL_EDIT) {
		if(pL->hoprSrc == PV_LIMITS_USER) {
		    pL->hoprSrc = PV_LIMITS_DEFAULT;
		    pL->hopr = pL->hoprDefault;
		}
	    }
	    w = table[i][4];
	    XtSetSensitive(w, True);
	    optionMenuSet(w, pL->hoprSrc-FIRST_PV_LIMITS_SRC);
	    w = table[i][5];
	    XtSetSensitive(XtParent(w), True);
	    if(globalDisplayListTraversalMode == DL_EDIT) {
		XtVaSetValues(w, XmNeditable,
		  (pL->hoprSrc == PV_LIMITS_DEFAULT)?True:False, NULL);
	    } else {
		XtVaSetValues(w, XmNeditable,
		  (pL->hoprSrc == PV_LIMITS_USER)?True:False, NULL);
	    }
	    cvtDoubleToString(pL->hopr, string, SC_PREC(pL));
	    XmTextFieldSetString(w, string);
	    break;
	case PV_LIMITS_UNUSED:
	    w = table[i][4];
	    optionMenuSet(w, PV_LIMITS_CHANNEL-FIRST_PV_LIMITS_SRC);
	    XtSetSensitive(w, False);
	    w = table[i][5];
	    cvtDoubleToString(pL->hoprChannel, string, SC_PREC(pL));
	    XmTextFieldSetString(w, string);
	    XtSetSensitive(XtParent(w), False);
	    break;
	}
    }

  /* Execute controls */
    if(globalDisplayListTraversalMode == DL_EXECUTE) {
      /* Period */
	cvtDoubleToString(tempPeriod,string,0);
	XmTextFieldSetString(scPeriodW,string);

      /* Units */
	optionMenuSet(scUnitsW, tempUnits - FIRST_TIME_UNIT);
    }
}

/* Sets the values in the tempPen when controls in the dialog are
   activated.  The row is stored in the high word of the cd and the
   type is stored in the low word.  If the type is less than 3, it is
   a button in the option menu, and the type is stored in the userData
   of the row column parent of the button.  Otherwise the type in the
   cd is valid. */
static void stripChartDialogCb(Widget w, XtPointer cd , XtPointer cbs)
{
  /* The type is in the low word of the cd and the row is in the high word */
    int type = GETLOW((int)cd);
    int row = GETHIGH((int)cd);
    int button, src;
    double val;
    DlLimits *pL = NULL;
    char *string;

#if DEBUG_SC
    print("\nstripChartDialogCb: w=%x cd=%d type=%d row=%d\n",
      w,(int)cd,type,row);
#endif
  /* If the type is less than 3, the callback comes from an option menu button
   *   Find the real type from the userData of the RC parent of the button */
    if(type < 3) {
	XtPointer userData;

	button=type;
	XtVaGetValues(XtParent(w), XmNuserData, &userData, NULL);
	type = GETLOW((int)userData);
	row = GETHIGH((int)userData);
    }

  /* Check */
    if(row < 0 || row > MAX_PENS) {
	medmPostMsg(1, "stripChartDialogCb: Invalid row (%d)\n", row);
	return;
    }

  /* Put all the text values into tempPen since we do not get a
     callback when they are entered */
    stripChartDialogStoreTextEntries();

  /* Get limits pointer */
    pL = &tempPen[row].limits;

#if DEBUG_SC
    print("  Final values: type=%d row=%d pL=%x\n",type,row,pL);
#endif

    switch(type) {
    case SC_CLR_BTN:
	if(globalDisplayListTraversalMode == DL_EDIT) {
	    setCurrentDisplayColorsInColorPalette(SCDATA_RC,row);
	    XtPopup(colorS, XtGrabNone);
	} else {
	    setCurrentDisplayColorsInColorPalette(SCDATA_RC,row);
	  /* Make the color palette sensitive. It usually isn't in
	     EXECUTE mode. */
	    if(colorS) {
		XtSetSensitive(colorS, True);
		XtPopup(colorS, XtGrabNone);
	    } else {
		medmPostMsg(1, "stripChartDialogCb:\n"
		  "  Cannot pop up color palette\n");
	    }
	}
	break;
    case SC_LOPR_SRC_BTN:
	src = (PvLimitsSrc_t)(FIRST_PV_LIMITS_SRC + button);
	if(globalDisplayListTraversalMode == DL_EDIT) {
	    if(button == 2) {
	      /* Don't allow setting user-specified, revert to channel */
		optionMenuSet(table[row][2], 0);
		src = (PvLimitsSrc_t)(FIRST_PV_LIMITS_SRC + 0);
		XBell(display, 50);
	    }
	    pL->loprSrc0 = src;
	}
	pL->loprSrc = src;
	break;
    case SC_HOPR_SRC_BTN:
	src = (PvLimitsSrc_t)(FIRST_PV_LIMITS_SRC + button);
	if(globalDisplayListTraversalMode == DL_EDIT) {
	    if(button == 2) {
	      /* Don't allow setting user-specified, revert to channel */
		optionMenuSet(table[row][4], 0);
		src = (PvLimitsSrc_t)(FIRST_PV_LIMITS_SRC + 0);
		XBell(display, 50);
	    }
	    pL->hoprSrc0 = src;
	}
	pL->hoprSrc = src;
	break;
    case SC_LOPR_BTN:
	string = XmTextFieldGetString(table[row][3]);
	val = atof(string);
	XtFree(string);
	src = pL->loprSrc;
	if(globalDisplayListTraversalMode == DL_EDIT) {
	    if(src == PV_LIMITS_DEFAULT) {
		pL->loprDefault = val;
		pL->lopr = val;
	    } else {
		XBell(display, 50);
		return;
	    }
	} else {
	    if(src == PV_LIMITS_USER) {
		pL->loprUser = val;
		pL->lopr = val;
	    } else {
		XBell(display, 50);
		return;
	    }
	}
	break;
    case SC_HOPR_BTN:
	string = XmTextFieldGetString(table[row][5]);
	val = atof(string);
	XtFree(string);
	src = pL->hoprSrc;
	if(globalDisplayListTraversalMode == DL_EDIT) {
	    if(src == PV_LIMITS_DEFAULT) {
		pL->hoprDefault = val;
		pL->hopr = val;
	    } else {
		XBell(display, 50);
		return;
	    }
	} else {
	    if(src == PV_LIMITS_USER) {
		pL->hoprUser = val;
		pL->hopr = val;
	    } else {
		XBell(display, 50);
		return;
	    }
	}
	break;
    case SC_PERIOD_BTN:
	if(globalDisplayListTraversalMode == DL_EXECUTE) {
	    string = XmTextFieldGetString(scPeriodW);
	    tempPeriod = atof(string);
	    XtFree(string);
	}
	break;
    case SC_UNITS_BTN:
	if(globalDisplayListTraversalMode == DL_EXECUTE) {
	    tempUnits = (TimeUnits)(FIRST_TIME_UNIT + button);
	}
	break;
    case SC_APPLY_BTN:
      /* pL is not meaningful here.  We are going to do all rows. */
	pL = NULL;
	stripChartDialogUpdateElement();
	break;
    case SC_CANCEL_BTN:
	executeTimeStripChartElement = NULL;
	XtPopdown(stripChartS);
      /* Pop the color palette down and set it to insensitive in
         EXECUTE mode */
	if(globalDisplayListTraversalMode == DL_EXECUTE && colorS) {
	    XtPopdown(colorS);
	    XtSetSensitive(colorS, False);
	}
	return;
    case SC_HELP_BTN:
	callBrowser(medmHelpPath,"#StripChartDataDialog");
	return;
    }

  /* Reset the dialog box */
    stripChartDialogReset();
}

/* Reads the text entries and stores the values in the tempPen.  There
   is no indication when a value is entered in a text entry, so we
   cannot do this when the value is changed.  (Perhaps we could, but
   we don't.)  */
static void stripChartDialogStoreTextEntries(void)
{
    int i, src;
    double val;
    DlLimits *pL = NULL;
    char *string;

  /* Copy text values from dialog to tempPens.  Other values
     should be up to date. */
    for(i = 0; i < MAX_PENS; i++) {
	pL = &tempPen[i].limits;

      /* Update the limits to reflect current src's */
	updatePvLimits(pL);

	string = XmTextFieldGetString(table[i][0]);
	if(string) {
	    strcpy(tempPen[i].chan, string);
	    XtFree(string);
	}
	string = XmTextFieldGetString(table[i][3]);
	if(string) {
	    val = atof(string);
	    XtFree(string);
	    src = pL->loprSrc;
	    if(globalDisplayListTraversalMode == DL_EDIT) {
		if(src == PV_LIMITS_DEFAULT) {
		    pL->loprDefault = val;
		    pL->lopr = val;
		} else {
		}
	    } else {
		if(src == PV_LIMITS_USER) {
		    pL->loprUser = val;
		    pL->lopr = val;
		} else {
		  /* Error message here? */
		}
	    }
	}
	string = XmTextFieldGetString(table[i][5]);
	if(string) {
	    val = atof(string);
	    XtFree(string);
	    src = pL->hoprSrc;
	    if(globalDisplayListTraversalMode == DL_EDIT) {
		if(src == PV_LIMITS_DEFAULT) {
		    pL->hoprDefault = val;
		    pL->hopr = val;
		} else {
		}
	    } else {
		if(src == PV_LIMITS_USER) {
		    pL->hoprUser = val;
		    pL->hopr = val;
		} else {
		  /* Error message here? */
		}
	    }
	}
    }

  /* Execute controls */
    if(globalDisplayListTraversalMode == DL_EXECUTE) {
      /* Period */
	string = XmTextFieldGetString(scPeriodW);
	if(string) {
	    tempPeriod = atof(string);
	}
    }
}

/* Implements the Apply button.  Copies the tempPen to the
   globalResourceBundle or the executeTimeStripChartElement.  Until
   then the user can cancel without affecting real data.  */
static void stripChartDialogUpdateElement(void)
{
    int i;
    DlStripChart *dlStripChart;
    DlElement *pE = executeTimeStripChartElement;

  /* Read and store the text entries */
    stripChartDialogStoreTextEntries();

  /* Copy the tempPen to the globalResourceBundle or the
     executeTimeStripChartElement */
    for(i = 0; i < MAX_PENS; i++) {
	if(globalDisplayListTraversalMode == DL_EDIT) {
	    globalResourceBundle.scData[i] = tempPen[i];
	} else {
	    dlStripChart = pE->structure.stripChart;
	    dlStripChart->pen[i] = tempPen[i];
	}
    }

  /* Execute controls */
    if(globalDisplayListTraversalMode == DL_EXECUTE) {
      /* Period */
	dlStripChart->period = tempPeriod;
      /* Units */
	dlStripChart->units = tempUnits;
    }

  /* Update element */
    if(globalDisplayListTraversalMode == DL_EDIT) {
	DisplayInfo *cdi=currentDisplayInfo;

	DlElement *dlElement = FirstDlElement(
	  cdi->selectedDlElementList);
	unhighlightSelectedElements();
	while(dlElement) {
	    updateElementFromGlobalResourceBundle(
	      dlElement->structure.element);
	    dlElement = dlElement->next;
	}
	dmTraverseNonWidgetsInDisplayList(cdi);
	highlightSelectedElements();
	medmMarkDisplayBeingEdited(cdi);
    } else {
	MedmStripChart *psc = (MedmStripChart *)pE->data;
	DisplayInfo *cdi=currentDisplayInfo;

	if(pE && pE->data) {
	    psc = (MedmStripChart *)pE->data;
	    stripChartConfig(psc);
	}
	if(pE && pE->run && pE->run->execute) {
	    pE->run->execute(cdi, pE);
	}
      /* Make sure it starts over right */
	psc->nextSlot = 0;
#if DEBUG_TIMEOUT
	{
	    double currentTime=medmTime();

	    print("stripChartDialogUpdateElement: \n"
	      "  timeInterval=%f\n"
	      "  nextAdvanceTime(delta)=%f\n"
	      "  nextSlot\n",
	      psc->timeInterval,
	      psc->nextAdvanceTime-currentTime,
	      psc->nextSlot);
	    print("updateTask: \n"
	      "  timeInterval=%f\n"
	      "  nextExecuteTime(delta)=%f\n"
	      "  executeRequestsPendingCount=%d\n"
	      "  disabled=%s\n",
	      psc->updateTask->timeInterval,
	      psc->updateTask->nextExecuteTime-currentTime,
	      psc->updateTask->executeRequestsPendingCount,
	      psc->updateTask->disabled?"True":"False");
	    print("displayInfo: \n"
	      "  periodicTaskcount=%d\n"
	      "  nextExecuteTime(delta)=%f\n"
	      "  periodicTaskCount=%d\n",
	      cdi->periodicTaskCount,
	      cdi->updateTaskListHead.nextExecuteTime-currentTime,
	      cdi->periodicTaskCount);
	}
#endif
    }
}

#if 0
/* Use for debugging */
static void printPens(DlPen *pens, char *title)
{
    int i;

    print("%s pen=%x\n", title, pens);
    for(i=0; i < MAX_PENS; i++) {
	print("%d %-34s %2d %-7s %6.2f %-7s %6.2f limits=%x\n",
	  i, pens[i].chan, pens[i].clr,
	  stringValueTable[pens[i].limits.loprSrc], pens[i].limits.lopr,
	  stringValueTable[pens[i].limits.hoprSrc], pens[i].limits.hopr,
	  &pens[i].limits);
    }
}
#endif
