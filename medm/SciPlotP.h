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

#ifndef _SCIPLOTP_H
#  define _SCIPLOTP_H

#  ifdef __cplusplus
extern "C"
{
#  endif

#  include <X11/CoreP.h>
#  if XmVersion == 1001
#    include <Xm/XmP.h>
#  else
#    include <Xm/PrimitiveP.h>
#  endif

#  include "SciPlot.h"
#  include <X11/Xcms.h>

#  define powi(a, i) (real) pow(a, (double)((int)i))

#  define NUMPLOTLINEALLOC 5
#  define NUMPLOTDATAEXTRA 25
#  define NUMPLOTITEMALLOC 256
#  define NUMPLOTITEMEXTRA 64
#  define DEG2RAD (3.1415926535897931160E0 / 180.0)

  typedef struct
  {
    int dummy; /* keep compiler happy with dummy field */
  } SciPlotClassPart;

  typedef struct _SciPlotClassRec
  {
    CoreClassPart core_class;
    XmPrimitiveClassPart primitive_class;
    SciPlotClassPart plot_class;
  } SciPlotClassRec;

  extern SciPlotClassRec sciplotClassRec;

  typedef enum
    {
      SciPlotFALSE,
      SciPlotPoint,
      SciPlotLine,
      SciPlotRect,
      SciPlotFRect,
      SciPlotCircle,
      SciPlotFCircle,
      SciPlotStartTextTypes,
      SciPlotText,
      SciPlotVText,
      SciPlotEndTextTypes,
      SciPlotPoly,
      SciPlotFPoly,
      SciPlotClipRegion,
      SciPlotClipClear,
      SciPlotENDOFLIST
    } SciPlotTypeEnum;

  typedef enum
    {
      SciPlotDrawingAny,
      SciPlotDrawingShadows,
      SciPlotDrawingPAxis,
      SciPlotDrawingXAxis,
      SciPlotDrawingXMajorMinor,
      SciPlotDrawingXLabelTitle,
      SciPlotDrawingYAxis,
      SciPlotDrawingY2Axis,
      SciPlotDrawingY3Axis,
      SciPlotDrawingY4Axis,
      SciPlotDrawingYMajorMinor,
      SciPlotDrawingYLabel,
      SciPlotDrawingY2Label,
      SciPlotDrawingY3Label,
      SciPlotDrawingY4Label,
      SciPlotDrawingLegend,
      SciPlotDrawingLine
    } SciPlotDrawingEnum;

  typedef struct _SciPlotItem
  {
    SciPlotTypeEnum type;
    SciPlotDrawingEnum drawing_class;
    union
    {
      struct
      {
        short color;
        short style;
        real x, y;
      } pt;
      struct
      {
        short color;
        short style;
        real x1, y1, x2, y2;
      } line;
      struct
      {
        short color;
        short style;
        real x, y, w, h;
      } rect;
      struct
      {
        short color;
        short style;
        real x, y, r;
      } circ;
      struct
      {
        short color;
        short style;
        short count;
        real x[4], y[4];
      } poly;
      struct
      {
        short color;
        short style;
        short font;
        short length;
        real x, y;
        char *text;
      } text;
      struct
      {
        short color;
        short style;
      } any;

    } kind;
    short individually_allocated;
    struct _SciPlotItem *next;
  } SciPlotItem;

  typedef struct
  {
    int LineStyle;
    int FillStyle;
    int LineColor;
    int PointStyle;
    int PointColor;
    int number;
    int allocated;
    realpair *data;
    char *legend;
    real markersize;
    realpair min, max;
    Boolean draw, used;
    char **markertext;
    int axis;
    int yside;
  } SciPlotList;

  typedef struct
  {
    real Origin;
    real Size;
    real Center;
    real TitlePos;
    real AxisPos;
    real LabelPos;
    real LegendPos;
    real LegendSize;
    real DrawOrigin;
    real DrawSize;
    real DrawMax;
    real MajorInc;
    int MajorNum;
    int MinorNum;
    int Precision;
    int FixedMargin;
    real Left;  /* in y direction will be top */
    real Right; /* in y direction will be bottom */
  } SciPlotAxis;

  typedef struct
  {
    int id;
    XFontStruct *font;
  } SciPlotFont;

  typedef struct
  {
    int flag;
    char *PostScript;
    char *X11;
    Boolean PSUsesOblique;
    Boolean PSUsesRoman;
  } SciPlotFontDesc;

  typedef struct
  {
    /* Public stuff ... */
    char *TransientPlotTitle;
    char *TransientXLabel;
    char *TransientYLabel;
    char *TransientY2Label;
    char *TransientY3Label;
    char *TransientY4Label;
    int Margin;
    int TitleMargin;
    int LegendMargin;
    int LegendLineSize;
    int MajorTicSize;
    int DefaultMarkerSize;
    int ChartType;
    Boolean ScaleToFit;
    Boolean Degrees;
    Boolean XTime;
    time_t XTimeBase;
    char *XTimeFormat;
    Boolean XLog;
    Boolean YLog;
    Boolean Y2Log;
    Boolean Y3Log;
    Boolean Y4Log;
    Boolean XAutoScale;
    Boolean YAutoScale;
    Boolean Y2AutoScale;
    Boolean Y3AutoScale;
    Boolean Y4AutoScale;
    Boolean YAxisShow;
    Boolean Y2AxisShow;
    Boolean Y3AxisShow;
    Boolean Y4AxisShow;
    Boolean YAxisLeft;
    Boolean Y2AxisLeft;
    Boolean Y3AxisLeft;
    Boolean Y4AxisLeft;
    Boolean XAxisNumbers;
    Boolean YAxisNumbers;
    Boolean XOrigin;
    Boolean YOrigin;
    Boolean Y2Origin;
    Boolean Y3Origin;
    Boolean Y4Origin;
    Boolean DrawMajor;
    Boolean DrawMinor;
    Boolean DrawMajorTics;
    Boolean DrawMinorTics;
    Boolean ShowLegend;
    Boolean ShowTitle;
    Boolean ShowXLabel;
    Boolean ShowYLabel;
    Boolean ShowY2Label;
    Boolean ShowY3Label;
    Boolean ShowY4Label;
    Boolean YNumHorz;
    Boolean LegendThroughPlot;
    Boolean Monochrome;
    Boolean DragX;
    Boolean DragY;
    Boolean TrackPointer;
    Boolean XNoCompMinMax;
    Font TitleFID;
    int TitleFont;
    int LabelFont;
    int AxisFont;
    int BackgroundColor;
    int ForegroundColor;

    int ShadowColor1;
    int ShadowColor2;
    unsigned char ShadowType;

    int medmFileVersion;
    /*   Fixed X left & right space */
    Boolean XFixedLR;
    int XLeftSpace;
    int XRightSpace;

    /* Private stuff ... */
    char *plotTitle;
    char *xlabel;
    char *ylabel;
    char *y2label;
    char *y3label;
    char *y4label;
    realset Min, Max;
    realset UserMin, UserMax;
    SciPlotAxis x, y, y2, y3, y4;
    int titleFont;
    int labelFont;
    int axisFont;

    GC defaultGC;
    GC dashGC;
    Colormap cmap;
    Pixel *colors;
    int num_colors;
    SciPlotFont *fonts;
    int num_fonts;

    int alloc_plotlist;
    int num_plotlist;
    SciPlotList *plotlist;
    int alloc_drawlist;
    int num_drawlist;
    SciPlotItem *drawlist;
    SciPlotDrawingEnum current_id; /* id of current item.  Used for erasing */
    Boolean update;

    /* drag and resize stuff */
    int drag_state;
    realpair drag_start;                 /* data coord position of start of drag   */
    real startx, endx;                   /* real x value range                     */
    real starty, endy;                   /* real y value range                     */
    int reach_limit;                     /* reach drawing limit                    */
    XtCallbackList drag_x_callback;      /* callback list for dragging x           */
    XtCallbackList drag_y_callback;      /* callback list for dragging y           */
    XtCallbackList pointer_val_callback; /* callback list for tracking pointer*/
    XtCallbackList btn1_callback;        /* callback assciated with btn1 click     */
  } SciPlotPart;

  typedef struct _SciPlotRec
  {
    CorePart core;
    XmPrimitivePart primitive;
    SciPlotPart plot;
  } SciPlotRec;

#  ifdef __cplusplus
};
#  endif

#endif /* _SCIPLOTP_H */

/* **************************** Emacs Editing Sequences ***************** */
/* Local Variables: */
/* c-basic-offset: 2 */
/* End: */
