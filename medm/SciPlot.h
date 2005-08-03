/*----------------------------------------------------------------------------
 * SciPlot      A entended plotting widget
 *
 * CopyRight (c) 1997 Jie Chen
 *
 * Free software. Allow interactive zoom and pan to existing plotting widget
 */
/*----------------------------------------------------------------------------
 * SciPlot	A generalized plotting widget
 *
 * Copyright (c) 1996 Robert W. McMullen
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Author: Rob McMullen <rwmcm@mail.ae.utexas.edu>
 *         http://www.ae.utexas.edu/~rwmcm
 */

#ifndef _SCIPLOT_H
#define _SCIPLOT_H

#include <stdio.h>
#include <X11/Core.h>
#include <math.h>
#include <float.h>

#ifdef __cplusplus
extern "C"
{
#endif

#define _SCIPLOT_WIDGET_VERSION	1.36

#ifndef XtIsSciPlot
#define XtIsSciPlot(w) XtIsSubclass((Widget)w, sciplotWidgetClass)
#endif


  /* NOTE:  float.h is required by POSIX */
#define SCIPLOT_SKIP_VAL (-FLT_MAX)

typedef float real;

typedef struct {
	real x,y;
} realpair;


#define XtNchartType	          "chartType"          /* cartesian or polar */
#define XtNdegrees	          "degrees"            /* degrees            */
#define XtNdefaultMarkerSize      "defaultMarkerSize"  /* draw marker size   */
#define XtNdrawMajor	          "drawMajor"          /* major line         */
#define XtNdrawMajorTics	  "drawMajorTics"      /* major tick marks   */
#define XtNdrawMinor	          "drawMinor"          /* minor line         */
#define XtNdrawMinorTics	  "drawMinorTics"      /* minor tick marks   */
#define XtNxAutoScale	          "xAutoScale"         /* autoscale x        */
#define XtNyAutoScale	          "yAutoScale"         /* autoscale y        */
#define XtNxAxisNumbers	          "xAxisNumbers"       /* show x-axis num    */
#define XtNyAxisNumbers	          "yAxisNumbers"       /* show y-axis num    */
#define XtNxLog		          "xLog"               /* log scale x        */
#define XtNyLog		          "yLog"               /* log scale y        */
#define XtNxOrigin	          "xOrigin"            /* show x origin      */
#define XtNyOrigin	          "yOrigin"            /* show y origin      */
#define XtNxLabel	          "xLabel"             /* x label            */
#define XtNyLabel	          "yLabel"             /* y label            */
#define XtNplotTitle	          "plotTitle"          /* plot title         */
#define XtNmargin	          "margin"             /* pltting margin     */
#define XtNmonochrome	          "monochrome"         /* just two colors    */
#define XtNtitleMargin	          "titleMargin"        /* title margin       */
#define XtNshowLegend	          "showLegend"         /* show legend ?      */
#define XtNshowTitle	          "showTitle"          /* draw title?        */
#define XtNshowXLabel	          "showXLabel"         /* draw X label?      */
#define XtNshowYLabel	          "showYLabel"         /* draw y label?      */
#define XtNlegendLineSize	  "legendLineSize"     /* lengend line size  */
#define XtNlegendMargin	          "legendMargin"       /* legend margin      */
#define XtNlegendThroughPlot	  "legendThroughPlot"  /* legend extend      */
#define XtNtitleFont	          "titleFont"          /* font for title     */
#define XtNlabelFont	          "labelFont"          /* label font         */
#define XtNaxisFont	          "axisFont"           /* axis  font         */
#define XtNyNumbersHorizontal	  "yNumbersHorizontal" /* horizontal num y   */
#define XtNdragX                  "dragX"              /* allow drag x axis  */
#define XtNdragY                  "dragY"              /* allow drag y axis  */
#define XtNdragXCallback          "dragXCallback"      /* callback           */
#define XtNdragYCallback          "dragYCallback"      /* callback           */
#define XtNtrackPointer           "trackPointer"       /* track pointer pos  */
#define XtNpointerValCallback     "pointerValCallback" /* track pointer cbk  */
#define XtNbtn1ClickCallback      "btn1ClickCallback"  /* button 1 click cbk */

/* the following are for the fixed left and right or top and bottom space    */
/* It is very useful for multiple drawings to align in the windows           */
/* these are for cartesian plot only, polar plots are coming                 */
#define XtNxFixedLeftRight        "xFixedLeftRight"
#define XtNxLeftSpace             "xLeftSpace"
#define XtNxRightSpace            "xRightSpace"

/* the following is for precalculated drawing range. So the widget will      */
/* not calculate drawing range again                                         */
#define XtNxNoCompMinMax           "xNoCompMinMax"

#define XtPOLAR		0
#define XtCARTESIAN	1

#define XtMARKER_NONE		0
#define XtMARKER_CIRCLE		1
#define XtMARKER_SQUARE		2
#define XtMARKER_UTRIANGLE	3
#define XtMARKER_DTRIANGLE	4
#define XtMARKER_LTRIANGLE	5
#define XtMARKER_RTRIANGLE	6
#define XtMARKER_DIAMOND	7
#define XtMARKER_HOURGLASS	8
#define XtMARKER_BOWTIE		9
#define XtMARKER_FCIRCLE	10
#define XtMARKER_FSQUARE	11
#define XtMARKER_FUTRIANGLE	12
#define XtMARKER_FDTRIANGLE	13
#define XtMARKER_FLTRIANGLE	14
#define XtMARKER_FRTRIANGLE	15
#define XtMARKER_FDIAMOND	16
#define XtMARKER_FHOURGLASS	17
#define XtMARKER_FBOWTIE	18
#define XtMARKER_DOT		19
#define XtMARKER_HTEXT          20   /* draw horizontal text               */
#define XtMARKER_VTEXT          21   /* draw vertial    text               */

#define XtFONT_SIZE_MASK	0xff
#define XtFONT_SIZE_DEFAULT	12

#define XtFONT_NAME_MASK	0xf00
#define XtFONT_TIMES		0x000
#define XtFONT_COURIER		0x100
#define XtFONT_HELVETICA	0x200
#define XtFONT_LUCIDA		0x300
#define XtFONT_LUCIDASANS	0x400
#define XtFONT_NCSCHOOLBOOK	0x500
#define XtFONT_NAME_DEFAULT	XtFONT_TIMES

#define XtFONT_ATTRIBUTE_MASK	0xf000
#define XtFONT_BOLD		0x1000
#define XtFONT_ITALIC		0x2000
#define XtFONT_BOLD_ITALIC	0x3000
#define XtFONT_ATTRIBUTE_DEFAULT 0


#define XtLINE_NONE	0
#define XtLINE_SOLID	1
#define XtLINE_DOTTED	2
#define XtLINE_WIDEDOT	3
#define XtLINE_USERDASH	4

/* drag callback structure */
typedef struct _SciPlotDragCbkStruct
{
  int            action;
  XEvent         *event;
  real           drawMin;        /* x or y direction drawing min and max  */
  real           drawMax;        /* in user coordinates                   */
}SciPlotDragCbkStruct;

typedef struct _SciPlotPointerCbkStruct
{
  int            action;
  XEvent         *event;
  real           x;              /* current x and y coordinates in user  */
  real           y;              /* space                                */
}SciPlotPointerCbkStruct;


typedef struct _SciPlotBtn1ClickCbkStruct
{
  int            action;
  XEvent         *event;
  real           x;             /* current x and y coordinates in user   */
  real           y;             /* space                                 */
  int            plotid;        /* closest plot: -1 means no plots       */
}SciPlotBtn1ClickCbkStruct;


extern WidgetClass sciplotWidgetClass;

typedef struct _SciPlotClassRec *SciPlotWidgetClass;
typedef struct _SciPlotRec      *SciPlotWidget;


/*
** Public function declarations
*/

/* Old compatibility functions */
#define SciPlotListCreateFromFloat SciPlotListCreateFloat
#define SciPlotListUpdateFromFloat SciPlotListUpdateFloat
#define SciPlotListCreateFromDouble SciPlotListCreateDouble
#define SciPlotListUpdateFromDouble SciPlotListUpdateDouble

#define COMPUTE_MIN_MAX 0
#define COMPUTE_MIN_ONLY 1
#define COMPUTE_MAX_ONLY 2
#define NO_COMPUTE_MIN_MAX 3
#define NO_COMPUTE_MIN_MAX_X 4
#define NO_COMPUTE_MIN_MAX_Y 5

/* functions decleration */
extern FILE* SciPlotPSCreateHeader            (Widget plot,
					       char* filename,
					       float width, float height);
extern float SciPlotPSAdd                     (Widget plot, FILE* fd, float ht,
					       int usecolor);
extern int   SciPlotPSFinish                  (FILE* fd, int drawborder,
					       char* title, int usecolor);

extern int     SciPlotPSCreate                (Widget wi, char *filename);
extern int     SciPlotPSCreateColor           (Widget wi, char *filename);
extern int SciPlotAllocNamedColor             (Widget wi, char *name);
extern int SciPlotAllocRGBColor               (Widget wi, int r, int g, int b);
extern int SciPlotAllocColor                  (Widget wi, Pixel p);
extern void SciPlotSetBackgroundColor         (Widget wi, int color);
extern void SciPlotSetForegroundColor         (Widget wi, int color);
extern int  NumberAllocatedColors             (Widget wi);
extern Pixel PixelValue                       (Widget wi, int color);
extern void SciPlotListDelete                 (Widget wi, int idnum);
extern int SciPlotListCreateFromData          (Widget wi, int num, real *xlist,
					       real *ylist, char *legend,
					       int pcolor, int pstyle,
					       int lcolor, int lstyle);
extern int SciPlotListCreateFloat             (Widget wi, int num,
					       float *xlist, float *ylist, char *legend);
extern void SciPlotListUpdateFloat            (Widget wi, int idnum,
					       int num, float *xlist, float *ylist);
extern void SciPlotListAddFloat               (Widget wi, int idnum,
					       int num, float *xlist, float *ylist);
extern int SciPlotListCreateDouble            (Widget wi, int num,
					       double *xlist, double *ylist, char *legend);
extern void SciPlotListUpdateDouble           (Widget wi, int idnum,
					       int num, double *xlist, double *ylist);
extern void SciPlotListAddDouble              (Widget wi, int idnum,
					       int num, double *xlist, double *ylist);
extern void SciPlotListSetStyle               (Widget wi, int idnum,
					       int pcolor, int pstyle,
					       int lcolor, int lstyle);
extern void SciPlotListSetMarkerText          (Widget wi, int idnum,
					       char** text, int num,
					       int style);
extern void SciPlotListSetMarkerSize          (Widget wi, int idnum, float size);
extern void SciPlotSetXAutoScale              (Widget wi);
extern void SciPlotSetXUserScale              (Widget wi, double min, double max);
extern void SciPlotSetXUserMin                (Widget wi, double min);
extern void SciPlotSetXUserMax                (Widget wi, double max);
extern void SciPlotSetYAutoScale              (Widget wi);
extern void SciPlotSetYUserScale              (Widget wi, double min, double max);
extern void SciPlotSetYUserMin                (Widget wi, double min);
extern void SciPlotSetYUserMax                (Widget wi, double max);

extern void SciPlotGetXScale                  (Widget wi,
					       float* min, float* max);
extern void SciPlotGetYScale                  (Widget wi,
					       float* min, float* max);
extern void SciPlotPrintStatistics            (Widget wi);
extern void SciPlotExportData                 (Widget wi, FILE *fd);
extern void SciPlotUpdate                     (Widget wi);
extern void SciPlotUpdateSimple               (Widget wi, int type);
extern Boolean SciPlotQuickUpdate             (Widget wi);
extern void SciPlotXDrawingRange              (Widget wi, float xmin, float xmax,
					       float* rxmin, float*  rxmax);
extern int  SciPlotZoomIn                     (Widget wi);
extern int  SciPlotZoomOut                    (Widget wi);
extern int  SciPlotZoomInX                    (Widget wi);
extern int  SciPlotZoomOutX                   (Widget wi);

/* KE: Missing prototypes */

int SciPlotStoreAllocatedColor(Widget wi, Pixel p);

/* KE: Prototypes for additional SciPlot functions */

void SciPlotGetXAxisInfo(Widget wi, float *min, float *max, Boolean *isLog,
  Boolean *isAuto);
void SciPlotGetYAxisInfo(Widget wi, float *min, float *max, Boolean *isLog,
  Boolean *isAuto);
void SciPlotPrintMetrics(Widget wi);
void SciPlotPrintAxisInfo(Widget wi);

#ifdef __cplusplus
}
#endif

#endif /* _SCIPLOT_H */

/* **************************** Emacs Editing Sequences ***************** */
/* Local Variables: */
/* c-basic-offset: 2 */
/* End: */
