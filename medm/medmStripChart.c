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

#define DEBUG_STRIP_CHART 0

#include "medm.h"

#include <Xm/DrawingAP.h>
#include <Xm/DrawP.h>
#include <X11/keysym.h>

#define STRIP_MARGIN 0.18

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

extern void linear_scale(double xmin, double xmax, int n,
  double *xminp, double *xmaxp, double *dist);

static void stripChartDraw(XtPointer cd);
static void stripChartUpdateTaskCb(XtPointer cd);
static void stripChartUpdateValueCb(XtPointer cd);
static void stripChartUpdateGraphicalInfoCb(XtPointer cd);
static void stripChartDestroyCb(XtPointer cd);
static void redisplayStrip(Widget, XtPointer, XtPointer);
static void stripChartUpdateGraph(XtPointer);
static MedmStripChart *stripChartAlloc(DisplayInfo *,DlElement *);
static void freeStripChart(XtPointer);
static void stripChartGetRecord(XtPointer, Record **, int *);
static void configStripChart(XtPointer, XtIntervalId *);
static void stripChartInheritValues(ResourceBundle *pRCB, DlElement *p);
static void stripChartSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void stripChartSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void stripChartGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable stripChartDlDispatchTable = {
    createDlStripChart,
    NULL,
    executeDlStripChart,
    hideDlStripChart,
    writeDlStripChart,
    NULL,
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

char *stripChartWidgetName = "stripChart";
static char* titleStr = "Strip Chart";
static Range range[MAX_PENS];
static Range nullRange = {0.0, 0.0, 0, 0, 0, 0, 0, 0.0, 0.0};
static StripChartConfigData sccd;

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
    int i;
    int cnt;
    int maxWidth = 0;
    int maxDot = 0;

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

	for(j=0; j<cnt; j++) {
	    if((psc->record[i]->hopr == range[j].hi) &&
	      (psc->record[i]->lopr == range[j].lo)) {
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

	    range[cnt].hi = psc->record[i]->hopr;
	    range[cnt].lo = psc->record[i]->lopr;
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

	width = XTextWidth(fontTable[sccd.axisLabelFont],text,range[i].width);
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
      "-0.0000000",width) + psc->dataWidth;
}

static void calcYAxisLabelHeight(MedmStripChart *psc) {
    sccd.yAxisLabelHeight = sccd.axisLabelFontHeight * sccd.numYAxisLabel
      + psc->dataHeight;
}

static MedmStripChart *stripChartAlloc(DisplayInfo *displayInfo,
  DlElement *dlElement) {
    MedmStripChart *psc;
    DlStripChart *dlStripChart = dlElement->structure.stripChart;

    psc = (MedmStripChart *)calloc(1,sizeof(MedmStripChart));
    dlElement->data = (void *)psc;
    if(psc == NULL) return psc;
    psc->w = dlStripChart->object.width;
    psc->h = dlStripChart->object.height;
    psc->dataX0 =  (int)(STRIP_MARGIN*psc->w);
    psc->dataY0 =  (int)(STRIP_MARGIN*psc->h);
    psc->dataX1 =  (int)((1.0 - STRIP_MARGIN)*psc->w);
    psc->dataY1 =  (int)((1.0 - STRIP_MARGIN)*psc->h);
    psc->dataWidth = psc->dataX1 - psc->dataX0;
    psc->dataHeight = psc->dataY1 - psc->dataY0;

    psc->updateEnable = False;
    psc->dlElement = dlElement;
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
	medmPrintf(1,"\nstripChartAlloc: Unknown time unit\n");
	psc->timeInterval = 60/ (double) psc->dataWidth;
	break;
    }
    psc->updateTask =
      updateTaskAddTask(displayInfo, &(dlStripChart->object),
	stripChartUpdateTaskCb,
	(XtPointer)psc);
    if(psc->updateTask == NULL) {
	medmPrintf(1,"\nstripChartAlloc: Memory allocation error\n");
    } else {
	updateTaskAddDestroyCb(psc->updateTask,freeStripChart);
	updateTaskAddNameCb(psc->updateTask,stripChartGetRecord);
    }
    return psc;
}

static void freeStripChart(XtPointer cd) {
    MedmStripChart *psc = (MedmStripChart *) cd;
    int i;

#if(DEBUG_STRIP_CHART)
    fprintf(stderr,"freeStripChart: psc->timerid=%x\n",
      psc->timerid);
#endif

    if(psc == NULL) return;
    if(psc->timerid) {
	XtRemoveTimeOut(psc->timerid);
	psc->timerid=0;
    }
    for(i = 0; i < psc->nChannels; i++) {
	medmDestroyRecord(psc->record[i]);
    }
    if(psc->pixmap) {
	XFreePixmap(display,psc->pixmap);
	psc->pixmap = (Pixmap) NULL;
	XFreeGC(display,psc->gc);
	psc->gc = NULL;
    }
    free((char *)psc);
    psc = NULL;
}


static void stripChartConfig(MedmStripChart *psc) {
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

    psc->pixmap =
      XCreatePixmap(display,XtWindow(widget),
	psc->w, psc->h,
	DefaultDepth(display,
	  DefaultScreen(display)));
    psc->gc = XCreateGC(display,XtWindow(widget),
      (unsigned long)NULL,NULL);
    gc = psc->gc;

  /* fill the background */
    XSetForeground(display,gc,displayInfo->colormap[dlStripChart->plotcom.bclr]);
    XFillRectangle(display,psc->pixmap,gc,0,0,
      psc->w, psc->h);
  /* draw the rectangle which enclosed the graph */
    XSetForeground(display,gc,displayInfo->colormap[dlStripChart->plotcom.clr]);
    XSetLineAttributes(display,gc,0,LineSolid,CapButt,JoinMiter);
    XDrawRectangle(display,psc->pixmap,gc,
      psc->dataX0-1, psc->dataY0-1,
      psc->dataWidth+1,psc->dataHeight+1);

  /* draw title */
    if(dlStripChart->plotcom.title) {
	int textWidth;
	char *title = dlStripChart->plotcom.title;
	int strLen = strlen(title);

	textWidth = XTextWidth(fontTable[sccd.titleFont],title,strLen);
	XSetForeground(display,gc,displayInfo->colormap[dlStripChart->plotcom.clr]);
	XSetFont(display,gc,fontTable[sccd.titleFont]->fid);
	XDrawString(display,psc->pixmap,gc,
	  ((int)psc->w - textWidth)/2,
	  sccd.shadowThickness + sccd.margin + fontTable[sccd.titleFont]->ascent,
	  title,strLen);
    }

  /* draw y-axis label */
    if(dlStripChart->plotcom.ylabel) {
	int textWidth;
	char *label = dlStripChart->plotcom.ylabel;
	int strLen = strlen(label);
	int x, y;

	textWidth = XTextWidth(fontTable[sccd.axisLabelFont],label,strLen);
	XSetForeground(display,gc,displayInfo->colormap[dlStripChart->plotcom.clr]);
	XSetFont(display,gc,fontTable[sccd.axisLabelFont]->fid);
	x = psc->dataX0;
	y = sccd.shadowThickness + sccd.margin*2 + sccd.titleFontHeight +
	  fontTable[sccd.axisLabelFont]->ascent;
	XDrawString(display,psc->pixmap,gc,x,y,label,strLen);
    }

  /* draw y-axis scale */
    XSetFont(display,gc,fontTable[sccd.axisLabelFont]->fid);
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
		    sprintf(text,"%.1e",range[j].value);
		} else {
		    sprintf(text,"%.*f",range[j].decimal,range[j].value);
		}
		strLen = strlen(text);
		XSetForeground(display, gc, fg);
		w = XTextWidth(fontTable[sccd.axisLabelFont],text,strLen);
		x = psc->dataX0 - 1 - sccd.markerHeight - sccd.yAxisLabelOffset
		  - sccd.margin - w;
		yoffset = nextTick - (double) labelHeight / 2 +
		  ((double)j + verticalSpacing/2.0) * sccd.axisLabelFontHeight +
                  (double) fontTable[sccd.axisLabelFont]->ascent;
		y = (int)psc->dataY0 + (int)yoffset;
		XDrawString(display,psc->pixmap,gc,x,y,text,strLen);
		range[j].value = range[j].value - range[j].step;

		if(range[j].numDot > 0) {   
		    int k;
		    int count = 0;
		    for(k = MAX_PENS - 1; k >= 0; k--) {
			if(range[j].mask & (0x1 << k)) {
			    XSetForeground(display,gc,
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
	medmPrintf(1,"\nstripChartAlloc: Unknown time unit\n");
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
	tw = XTextWidth(fontTable[sccd.axisLabelFont],"-0.000000",width);
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
		sprintf(text,"%.1e",value);
	    } else {
		sprintf(text,"%.*f",decimal,value);
	    }
	    strLen = strlen(text);
	    w = XTextWidth(fontTable[sccd.axisLabelFont],text,strLen);
	    xoffset = nextTick + (double) w / 2.0;
	    x = (int)psc->dataX1 - (int)xoffset;
	    XDrawLine(display, psc->pixmap, gc,
	      psc->dataX1 - (int)nextTick, psc->dataY1 + 2,
	      psc->dataX1 - (int)nextTick, psc->dataY1 + 2 + sccd.markerHeight);
	    XDrawString(display,psc->pixmap,gc,x,y,text,strLen);
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
	    textWidth = XTextWidth(fontTable[sccd.axisLabelFont],label,strLen);
	    x = psc->dataX0 + (psc->dataWidth - textWidth)/2;
	    y = psc->dataY1 + 1 + sccd.axisLabelFontHeight + sccd.margin * 2 +
	      sccd.markerHeight +
	      fontTable[sccd.axisLabelFont]->ascent + 1;
	    XDrawString(display,psc->pixmap,gc,x,y,label,strLen);
	}

    
#if 0
	if(!((dlStripChart->plotcom.xlabel) &&
	  (strlen(dlStripChart->plotcom.xlabel) > 0))) {
	    int strLen;
	    int textWidth;
	  /* print the timeInterval */
	    sprintf(text,"time/pixel = %.3f sec",psc->timeInterval);
	    strLen = strlen(text);
	    textWidth = XTextWidth(fontTable[sccd.axisLabelFont],text,strLen);
	    x = psc->dataX0 + (psc->dataWidth - textWidth)/2;
	    y = psc->dataY1 + sccd.axisLabelFontHeight + sccd.margin * 2 +
	      sccd.markerHeight +
	      fontTable[sccd.axisLabelFont]->ascent;
	    XDrawString(display,psc->pixmap,gc,x,y,text,strLen);
	}
#endif
    }

    {  /* draw the shadow */
	GC topGC = ((struct _XmDrawingAreaRec *)widget)->manager.top_shadow_GC;
	GC bottomGC =
	  ((struct _XmDrawingAreaRec *)widget)->manager.bottom_shadow_GC;
	_XmDrawShadows(display,psc->pixmap,topGC,bottomGC,0,0,
	  (Dimension)dlStripChart->object.width,
	  (Dimension)dlStripChart->object.height,
	  (Dimension)(sccd.shadowThickness-1),XmSHADOW_OUT);
    }
    XCopyArea(display,psc->pixmap,XtWindow(widget), gc,
      0,0,psc->w,psc->h,0,0);
    for(i = 0; i < psc->nChannels; i++) {
	Record *tmpd = psc->record[i];
	psc->maxVal[i] = tmpd->value;
	psc->minVal[i] = tmpd->value;
	psc->value[i] = tmpd->value;
    }

    psc->nextAdvanceTime = medmTime() + psc->timeInterval;

  /* specify the time interval */
    updateTaskSetScanRate(psc->updateTask,psc->timeInterval);
    psc->updateEnable = True;
}

#ifdef __cplusplus
static void redisplayFakeStrip(Widget widget, XtPointer cd, XtPointer)
#else
static void redisplayFakeStrip(Widget widget, XtPointer cd, XtPointer cbs)
#endif
{

    DlStripChart *dlStripChart = (DlStripChart *) cd;
    int usedWidth, usedHeight;
    int i;
    DisplayInfo *displayInfo;

    if(!(displayInfo = dmGetDisplayInfoFromWidget(widget)))
      return;

    if(dlStripChart) {
	GC topGC;
	GC bottomGC;
	Dimension shadowThickness;
	int x, y, w, h;
	int textWidth;
	int strLen = strlen(titleStr);

	XSetLineAttributes(display,displayInfo->gc,0,
	  LineSolid,CapButt,JoinMiter);
	XSetForeground(display,displayInfo->gc,
	  displayInfo->colormap[dlStripChart->plotcom.bclr]);
	XFillRectangle(display,XtWindow(widget),displayInfo->gc,
	  0,0,dlStripChart->object.width,dlStripChart->object.height);
	XSetForeground(display,displayInfo->gc,
	  displayInfo->colormap[dlStripChart->plotcom.clr]);
	x = (int)(STRIP_MARGIN*dlStripChart->object.width);
	y = (int)(STRIP_MARGIN*dlStripChart->object.height);
	w = (unsigned int)((1-STRIP_MARGIN*2)*dlStripChart->object.width);
	h = (unsigned int)((1-STRIP_MARGIN*2)*dlStripChart->object.height); 
	XDrawRectangle(display,XtWindow(widget),displayInfo->gc,x,y,w,h);
	i = dmGetBestFontWithInfo(fontTable,MAX_FONTS,titleStr,y,w,
	  &usedHeight,&usedWidth,TRUE);
	textWidth = XTextWidth(fontTable[i],titleStr,strLen);
	x = (dlStripChart->object.width - textWidth)/2;
	XSetFont(display,displayInfo->gc,fontTable[i]->fid);
	XDrawString(display,XtWindow(widget),displayInfo->gc,
	  x, (int)(0.15*dlStripChart->object.height),titleStr,strLen);
	XtVaGetValues(widget,
	  XmNshadowThickness,&shadowThickness,
	  NULL);
	topGC = ((struct _XmDrawingAreaRec *)widget)->manager.top_shadow_GC;
	bottomGC =
	  ((struct _XmDrawingAreaRec *)widget)->manager.bottom_shadow_GC;
	_XmDrawShadows(display,XtWindow(widget),topGC,bottomGC,0,0,
	  (Dimension)dlStripChart->object.width,
	  (Dimension)dlStripChart->object.height,
	  shadowThickness,XmSHADOW_OUT);
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
	XtSetArg(args[n],XmNx,(Position)dlStripChart->object.x); n++;
	XtSetArg(args[n],XmNy,(Position)dlStripChart->object.y); n++;
	XtSetArg(args[n],XmNwidth,(Dimension)dlStripChart->object.width); n++;
	XtSetArg(args[n],XmNheight,(Dimension)dlStripChart->object.height); n++;
	XtSetArg(args[n],XmNmarginWidth,0); n++;
	XtSetArg(args[n],XmNmarginHeight,0); n++;
	XtSetArg(args[n],XmNshadowThickness,2); n++;
	XtSetArg(args[n],XmNforeground,(Pixel)
	  displayInfo->colormap[dlStripChart->plotcom.clr]); n++;
	XtSetArg(args[n],XmNbackground,(Pixel)
	  displayInfo->colormap[dlStripChart->plotcom.bclr]); n++;
	XtSetArg(args[n],XmNtraversalOn,False); n++;
	localWidget = XmCreateDrawingArea(displayInfo->drawingArea,
	  stripChartWidgetName,args,n);
	dlElement->widget = localWidget;
	
      /* if execute mode, create stripChart and setup all the channels */
	if(displayInfo->traversalMode == DL_EXECUTE) {
	    int i,j;
	    MedmStripChart *psc;
	    
	    if(dlElement->data) {
		psc = (MedmStripChart *)dlElement->data;
	    } else {
		psc = stripChartAlloc(displayInfo,dlElement);
		if(psc == NULL) {
		    medmPrintf(1,"\nexecuteDlStripChart: Memory allocation error\n");
		    return;
		}
		
	      /* connect channels */
		j = 0;
		for(i = 0; i < MAX_PENS; i++) {
		    if(dlStripChart->pen[i].chan[0] != '\0') {
			psc->record[j] =
			  medmAllocateRecord(dlStripChart->pen[i].chan,
			    stripChartUpdateValueCb,
			    stripChartUpdateGraphicalInfoCb,
			    (XtPointer) psc);
			j++;
		    }
		}
	      /* record the number of channels in the strip chart */
		psc->nChannels = j;
		
		if(psc->nChannels == 0) {
		  /* if no channel, create a fake channel */
		    psc->nChannels = 1;
		    psc->record[0] = medmAllocateRecord(" ",
		      NULL,
		      stripChartUpdateGraphicalInfoCb,
		      (XtPointer) psc);
		}
	    }
	    
	    XtVaSetValues(localWidget, XmNuserData, (XtPointer) psc, NULL);
	    XtAddCallback(localWidget,XmNexposeCallback,redisplayStrip,
	      (XtPointer)psc);
#if 0
	    XtAddCallback(localWidget,XmNdestroyCallback,
	      (XtCallbackProc)monitorDestroy, (XtPointer)psc);
#endif
	} else if(displayInfo->traversalMode == DL_EDIT) {
	  /* Add expose callback for EDIT mode */
	    XtAddCallback(localWidget,XmNexposeCallback,redisplayFakeStrip,
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
    Record *pd = (Record *) cd;
    MedmStripChart *psc = (MedmStripChart *) pd->clientData;
    Widget widget = psc->dlElement->widget;
    int i;


  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! This is a temporary work around !!!!! */
  /* !!!!! for the reconnection.           !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  /* KE: This causes it to not do anything for the reconnection */
    medmRecordAddGraphicalInfoCb(pd, NULL);

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! End work around                 !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  /* make sure all the channel get their operating ranges */
    for(i = 0; i < psc->nChannels; i++) {
	if(psc->record[i]->precision < 0)
	  return;  /* wait for other channels */
    }

    if(XtIsManaged(widget) == False) {
	addCommonHandlers(widget, psc->updateTask->displayInfo);
	XtManageChild(widget);
	if(!psc->timerid) psc->timerid=XtAppAddTimeOut(
	  XtWidgetToApplicationContext(widget),
	  100,configStripChart,psc);
#if(DEBUG_STRIP_CHART)
	fprintf(stderr,"stripChartUpdateGraphicalInfoCb(1): psc->timerid=%x\n",
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
#if(DEBUG_STRIP_CHART)
	fprintf(stderr,"stripChartUpdateGraphicalInfoCb(2): psc->timerid=%x\n",
	  psc->timerid);
#endif
    }
}

#ifdef __cplusplus
static void configStripChart(XtPointer cd, XtIntervalId *)
#else
static void configStripChart(XtPointer cd, XtIntervalId *id)
#endif
{
    MedmStripChart *psc = (MedmStripChart *) cd;
    Widget widget = psc->dlElement->widget;

#if(DEBUG_STRIP_CHART)
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
	  100,configStripChart,psc);
#if(DEBUG_STRIP_CHART)
	fprintf(stderr,"configStripChart: psc->timerid=%x\n",
	  psc->timerid);
#endif
    }
}

#ifdef __cplusplus
static void redisplayStrip(Widget widget, XtPointer cd, XtPointer)
#else
static void redisplayStrip(Widget widget, XtPointer cd, XtPointer cbs)
#endif
{
    MedmStripChart *psc = (MedmStripChart *) cd;
    DlElement *dlElement = psc->dlElement;
    DlStripChart *dlStripChart = dlElement->structure.stripChart;
    DisplayInfo *displayInfo = psc->updateTask->displayInfo;
    GC gc = psc->gc;
    
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
	  psc->w,psc->dataY0,0,0);
      /* copy the left region */
	XCopyArea(display, psc->pixmap, XtWindow(widget), gc, 0,
	  psc->dataY0, psc->dataX0,psc->dataHeight,0,psc->dataY0);
      /* copy the bottom region */
	XCopyArea(display, psc->pixmap, XtWindow(widget), gc, 0,
	  psc->dataY1+1,psc->w,(psc->h - psc->dataY1),0,psc->dataY1+1);
      /* copy the right region */
	XCopyArea(display, psc->pixmap, XtWindow(widget), gc,
	  psc->dataX1+1,psc->dataY0, (psc->w - psc->dataX1),
	  psc->dataHeight, psc->dataX1+1,psc->dataY0);
      /* draw graph */
	if(psc->updateEnable)
	  stripChartDraw((XtPointer)psc);
    }
}

/* KE: This is not used */
#ifdef __cplusplus
static void stripChartDestroyCb(XtPointer)
#else
static void stripChartDestroyCb(XtPointer cd)
#endif
{
    return;
}

static void stripChartUpdateValueCb(XtPointer cd) {
    Record *pd = (Record *) cd;
    MedmStripChart *psc = (MedmStripChart *) pd->clientData;
    Boolean connected = True;
    Boolean readAccess = True;
    int i;
    Widget widget = psc->dlElement->widget;
    DlStripChart *dlStripChart = psc->dlElement->structure.stripChart;

    for(i = 0; i< psc->nChannels; i++) {
	Record *ptmp = psc->record[i];
	if(!ptmp->connected) {
	    connected = False;
	    break;
	}
	if(!ptmp->readAccess) {
	    readAccess = False;
	}
    }

    if(connected) {
	if(readAccess) {
	    if(widget) {
		if(XtIsManaged(widget) == False) {
		    addCommonHandlers(widget, psc->updateTask->displayInfo);
		    XtManageChild(widget);
		  /* specified the time interval */
		    updateTaskSetScanRate(psc->updateTask,psc->timeInterval);
		}
		if(psc->updateEnable) 
		  stripChartUpdateGraph((XtPointer)cd);
	    }
	} else {
	    if(widget) {
		if(XtIsManaged(widget)) {
		    XtUnmanageChild(widget);
		  /* stop periodic update */
		    updateTaskSetScanRate(psc->updateTask,0.0);
		}
	    }
	    draw3DPane(psc->updateTask,
	      psc->updateTask->displayInfo->
	      colormap[dlStripChart->plotcom.bclr]);
	    draw3DQuestionMark(psc->updateTask);
	}
    } else {
	if((widget) && (XtIsManaged(widget))) {
	    XtUnmanageChild(widget);
	  /* stop periodic update */
	    updateTaskSetScanRate(psc->updateTask,0.0);
	}
	drawWhiteRectangle(psc->updateTask);
    }
}

static void stripChartUpdateTaskCb(XtPointer cd) {
    MedmStripChart *psc = (MedmStripChart *) cd;
    Boolean connected = True;
    Boolean readAccess = True;
    Widget widget = psc->dlElement->widget;
    DlStripChart *dlStripChart = psc->dlElement->structure.stripChart;
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
    }

    if(connected) {
	if(readAccess) {
	    if(widget) {
		if(XtIsManaged(widget) == False) {
		    addCommonHandlers(widget, psc->updateTask->displayInfo);
		    XtManageChild(widget);
		  /* specified the time interval */
		    updateTaskSetScanRate(psc->updateTask,psc->timeInterval);
		}
		if(psc->updateEnable) 
		  stripChartDraw((XtPointer)psc);
	    }
	} else {
	    if(widget) {
		if(XtIsManaged(widget)) {
		    XtUnmanageChild(widget);
		  /* stop periodic update */
		    updateTaskSetScanRate(psc->updateTask,0.0);
		}
	    }
	    draw3DPane(psc->updateTask,
	      psc->updateTask->displayInfo->
	      colormap[dlStripChart->plotcom.bclr]);
	    draw3DQuestionMark(psc->updateTask);
	}
    } else {
	if((widget) && (XtIsManaged(widget))) {
	    XtUnmanageChild(widget);
	  /* stop periodic update */
	    updateTaskSetScanRate(psc->updateTask,0.0);
	}
	drawWhiteRectangle(psc->updateTask);
    }
}

static void stripChartUpdateGraph(XtPointer cd) {
    Record *pd = (Record *) cd;
    MedmStripChart *psc = (MedmStripChart *) pd->clientData;
    double currentTime;
    int n = 0;

  /* get the time */
    currentTime = medmTime();

    while(psc->record[n] != pd) n++;

  /* see whether is time to advance the graph */
    if(currentTime > psc->nextAdvanceTime) {
      /* update the graph */
	Window window = XtWindow(psc->dlElement->widget);
	GC gc = psc->gc;
	DlStripChart *dlStripChart = psc->dlElement->structure.stripChart;
	DisplayInfo *displayInfo  = psc->updateTask->displayInfo;
	int i;
	int totalPixel;    /* number of pixel needed to be drawn */


      /* calculate how many pixel needed to draw */
	if(psc->nextAdvanceTime > currentTime) {
	    totalPixel = 0;
	} else {
	    totalPixel = 1 + (int)((currentTime - psc->nextAdvanceTime) /
	      psc->timeInterval);
	    psc->nextAdvanceTime += psc->timeInterval * totalPixel;
	}
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
	    XSetClipRectangles(display,gc,0,0,&rectangle,1,YXBanded);
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
		  /* if wraped, two filled is needed */
		    XFillRectangle(display, psc->pixmap, gc,
		      nextSlot, psc->dataY0, (limit-nextSlot), psc->dataHeight);
		    XFillRectangle(display, psc->pixmap, gc,
		      psc->dataX0, psc->dataY0, totalPixel-limit+nextSlot,
		      psc->dataHeight);
		} else {
		  /* if not wraped, do one fill */
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
		Record *p = psc->record[i];

	      /* plot data */
		base = p->hopr - p->lopr;
		y1 =  psc->dataY0 + (int)((psc->dataHeight - 1) *
		  (1.0 - ((psc->minVal[i] - p->lopr) / base)));
		y2 =  psc->dataY0 + (int)((psc->dataHeight - 1) *
		  (1.0 - ((psc->maxVal[i] - p->lopr) / base)));
  
		XSetForeground(display, gc,
		  displayInfo->colormap[dlStripChart->pen[i].clr]);
		XDrawLine(display, psc->pixmap, gc, nextSlot, y1, nextSlot, y2);
  
		if(totalPixel > 1) {
		    int y;
		    y =  psc->dataY0 + (int)((psc->dataHeight - 1) *
		      (1.0 - ((psc->value[i] - p->lopr) / base)));
		  /* fill the gap in between */
		    if((nextSlot + totalPixel) > limit) {
		      /* if wraped, two lines are needed */
			XDrawLine(display, psc->pixmap, gc,
			  nextSlot, y, limit, y);
			XDrawLine(display, psc->pixmap, gc,
			  psc->dataX0,y,totalPixel-limit+nextSlot+psc->dataX0,y);
		    } else {
		      /* not wraped, one line only */
			XDrawLine(display, psc->pixmap, gc,
			  nextSlot, y, (nextSlot + totalPixel), y);
		    }
		}
	      /* reset max, min to the last pen position */
		psc->maxVal[i] = psc->minVal[i] = psc->value[i];
	    }
	    XSetClipOrigin(display,gc,0,0);
	    XSetClipMask(display,gc,None);
	    psc->nextSlot += totalPixel;
	    if(psc->nextSlot >= (int)psc->dataWidth)
	      psc->nextSlot -= psc->dataWidth;
	}
    }
  /* remember the min and max and current pen position */
    psc->value[n] = pd->value;
    if(pd->value > psc->maxVal[n]) {
	psc->maxVal[n] = pd->value;
    }
    if(pd->value < psc->minVal[n]) {
	psc->minVal[n] = pd->value;
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
    if(startPos) {
	int x, w;
      /* copy the first half */
	x = startPos + psc->dataX0;
	w = psc->dataWidth - startPos;
	XCopyArea(display,psc->pixmap,window,gc,
	  x, psc->dataY0, w, psc->dataHeight,
	  psc->dataX0, psc->dataY0);
      /* copy the second half */
	x = w + psc->dataX0;
	XCopyArea(display,psc->pixmap,window,gc,
	  psc->dataX0, psc->dataY0, startPos, psc->dataHeight,
	  x, psc->dataY0);
    } else {
	XCopyArea(display,psc->pixmap,window,gc,
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
	dlStripChart->period = 60.0;
	dlStripChart->units = SECONDS;
#if 1
      /* For backward compatibility */
	dlStripChart->delay = -1.0;
	dlStripChart->oldUnits = SECONDS;
#endif
	for(penNumber = 0; penNumber < MAX_PENS; penNumber++)
	  penAttributeInit(&(dlStripChart->pen[penNumber]));
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
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlStripChart->object));
	    else if(!strcmp(token,"plotcom"))
	      parsePlotcom(displayInfo,&(dlStripChart->plotcom));
	    else if(!strcmp(token,"period")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlStripChart->period = atof(token);
		isVersion2_1_x = True;
	    } else if(!strcmp(token,"delay")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlStripChart->delay = atoi(token);
	    } else if(!strcmp(token,"units")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"minute")) 
		  dlStripChart->units = MINUTES;
		else if(!strcmp(token,"second")) 
		  dlStripChart->units = SECONDS;
		else if(!strcmp(token,"milli second")) 
		  dlStripChart->units = MILLISECONDS;
		else if(!strcmp(token,"milli-second")) 
		  dlStripChart->units = MILLISECONDS;
		else
		  medmPostMsg(1,"parseStripChart: Illegal units %s\n"
		    "  Using SECONDS\n",token);
	    } else if(!strncmp(token,"pen",3)) {
		penNumber = MIN(token[4] - '0', MAX_PENS-1);
		parsePen(displayInfo,&(dlStripChart->pen[penNumber]));
	    }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
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
	  dlStripChart->units = SECONDS;
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
	if(dlStripChart->period != -val  || dlStripChart->units != SECONDS) {
	    dlStripChart->delay = -1;
	}
    }
#endif


    memset(indent,'\t',level);
    indent[level] = '\0';

    fprintf(stream,"\n%s\"strip chart\" {",indent);
    writeDlObject(stream,&(dlStripChart->object),level+1);
    writeDlPlotcom(stream,&(dlStripChart->plotcom),level+1);
    if(dlStripChart->delay < 0.0) {
	if(dlStripChart->period != 60.0) 
	  fprintf(stream,"\n%s\tperiod=%f",indent,dlStripChart->period);
#ifdef SUPPORT_0201XX_FILE_FORMAT
	if(MedmUseNewFileFormat) {
#endif
	    if(dlStripChart->units != SECONDS)
	      fprintf(stream,"\n%s\tunits=\"%s\"",indent,
		stringValueTable[dlStripChart->units]);
#ifdef SUPPORT_0201XX_FILE_FORMAT
	} else {
	    fprintf(stream,"\n%s\tunits=\"%s\"",indent,
	      stringValueTable[dlStripChart->units]);
	}
#endif
    } else {
#if 1
      /* for the compatibility */
	fprintf(stream,"\n%s\tdelay=%f",indent,dlStripChart->delay);
	fprintf(stream,"\n%s\tunits=\"%s\"",indent,
	  stringValueTable[dlStripChart->oldUnits]);
#endif
    }
    for(i = 0; i < MAX_PENS; i++) {
	writeDlPen(stream,&(dlStripChart->pen[i]),i,level+1);
    }
    fprintf(stream,"\n%s}",indent);
}

static void stripChartInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlStripChart *dlStripChart = p->structure.stripChart;
    medmGetValues(pRCB,
      CLR_RC,        &(dlStripChart->plotcom.clr),
      BCLR_RC,       &(dlStripChart->plotcom.bclr),
      -1);
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
