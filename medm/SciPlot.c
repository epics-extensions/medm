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
 *
 * Modified for DAKOTA by Mike Eldred (see MSE notations below).
 *
 */

#define DEBUG_SCIPLOT 0
#define DEBUG_SCIPLOT_VTEXT 0
#define DEBUG_SCIPLOT_TEXT 0
#define DEBUG_SCIPLOT_LINE 0
#define DEBUG_SCIPLOT_AXIS 0
#define DEBUG_SCIPLOT_ALLOC 0
#define DEBUG_SCIPLOT_MLK 0
#define CP_Y 1
#define CP_Y2 2
#define CP_Y3 3
#define CP_Y4 4
/* KE: Use SCIPLOT_EPS to avoid roundoff in floor and ceil */
#define SCIPLOT_EPS .0001

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <float.h>
#include "medm.h"
/* NOTE:  float.h is required by POSIX */
#define SCIPLOT_SKIP_VAL (-FLT_MAX)

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xm/Xm.h>

#ifdef _MSC_VER
/* Eliminate Microsoft C++ warnings about converting to float */
#  pragma warning(disable : 4244 4305)
#endif

#ifdef WIN32
/* Hummingbird extra functions including lprintf
 *   Needs to be included after Intrinsic.h for Exceed 5 */
#  include <X11/XlibXtra.h>
/* The following is done in Exceed 6 but not in Exceed 5
 *   Need it to define printf as lprintf for Windows
 *   (as opposed to Console) apps */
#  ifdef _WINDOWS
#    ifndef printf
#      define printf lprintf
#    endif
#  endif
#endif /* #ifdef WIN32 */

#include "SciPlotP.h"

/* #define DEBUG_SCIPLOT */

enum sci_drag_states
  {
    NOT_DRAGGING,
    DRAGGING_NOTHING,
    DRAGGING_TOP,
    DRAGGING_BOTTOM,
    DRAGGING_BOTTOM_AND_LEFT,
    DRAGGING_LEFT,
    DRAGGING_RIGHT,
    DRAGGING_DATA
  };

enum sci_draw_axis_type
  {
    DRAW_ALL,
    DRAW_AXIS_ONLY,
    DRAW_NO_LABELS
  };

#define SCIPLOT_ZOOM_FACTOR 0.25

#define offset(field) XtOffsetOf(SciPlotRec, plot.field)
static XtResource resources[] =
  {
    {XtNchartType, XtCMargin, XtRInt, sizeof(int),
     offset(ChartType), XtRImmediate, (XtPointer)XtCARTESIAN},
    {XtNdegrees, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(Degrees), XtRImmediate, (XtPointer)True},
    {XtNdrawMajor, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(DrawMajor), XtRImmediate, (XtPointer)False},
    {XtNdrawMajorTics, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(DrawMajorTics), XtRImmediate, (XtPointer)True},
    {XtNdrawMinor, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(DrawMinor), XtRImmediate, (XtPointer)False},
    {XtNdrawMinorTics, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(DrawMinorTics), XtRImmediate, (XtPointer)True},
    {XtNmonochrome, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(Monochrome), XtRImmediate, (XtPointer)False},
    {XtNshowTitle, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(ShowTitle), XtRImmediate, (XtPointer)False},
    {XtNshowXLabel, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(ShowXLabel), XtRImmediate, (XtPointer)False},
    {XtNshowYLabel, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(ShowYLabel), XtRImmediate, (XtPointer)False},
    {XtNshowY2Label, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(ShowY2Label), XtRImmediate, (XtPointer)False},
    {XtNshowY3Label, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(ShowY3Label), XtRImmediate, (XtPointer)False},
    {XtNshowY4Label, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(ShowY4Label), XtRImmediate, (XtPointer)False},
    {XtNxLabel, XtCString, XtRString, sizeof(String),
     offset(TransientXLabel), XtRString, ""},
    {XtNyLabel, XtCString, XtRString, sizeof(String),
     offset(TransientYLabel), XtRString, ""},
    {XtNy2Label, XtCString, XtRString, sizeof(String),
     offset(TransientY2Label), XtRString, ""},
    {XtNy3Label, XtCString, XtRString, sizeof(String),
     offset(TransientY3Label), XtRString, ""},
    {XtNy4Label, XtCString, XtRString, sizeof(String),
     offset(TransientY4Label), XtRString, ""},
    {XtNplotTitle, XtCString, XtRString, sizeof(String),
     offset(TransientPlotTitle), XtRString, ""},
    {XtNmargin, XtCMargin, XtRInt, sizeof(int),
     offset(Margin), XtRImmediate, (XtPointer)5},
    {XtNtitleMargin, XtCMargin, XtRInt, sizeof(int),
     offset(TitleMargin), XtRImmediate, (XtPointer)16},
    {XtNdefaultMarkerSize, XtCMargin, XtRInt, sizeof(int),
     offset(DefaultMarkerSize), XtRImmediate, (XtPointer)3},
    {XtNtitleFont, XtCMargin, XtRInt, sizeof(int),
     offset(TitleFont), XtRImmediate, (XtPointer)(XtFONT_HELVETICA | 24)},
    {XtNlabelFont, XtCMargin, XtRInt, sizeof(int),
     offset(LabelFont), XtRImmediate, (XtPointer)(XtFONT_TIMES | 18)},
    {XtNaxisFont, XtCMargin, XtRInt, sizeof(int),
     offset(AxisFont), XtRImmediate, (XtPointer)(XtFONT_TIMES | 10)},
    {XtNxAutoScale, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(XAutoScale), XtRImmediate, (XtPointer)True},
    {XtNyAutoScale, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(YAutoScale), XtRImmediate, (XtPointer)True},
    {XtNy2AutoScale, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(Y2AutoScale), XtRImmediate, (XtPointer)True},
    {XtNy3AutoScale, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(Y3AutoScale), XtRImmediate, (XtPointer)True},
    {XtNy4AutoScale, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(Y4AutoScale), XtRImmediate, (XtPointer)True},
    {XtNyAxisShow, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(YAxisShow), XtRImmediate, (XtPointer)True},
    {XtNy2AxisShow, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(Y2AxisShow), XtRImmediate, (XtPointer)False},
    {XtNy3AxisShow, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(Y3AxisShow), XtRImmediate, (XtPointer)False},
    {XtNy4AxisShow, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(Y4AxisShow), XtRImmediate, (XtPointer)False},
    {XtNxAxisNumbers, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(XAxisNumbers), XtRImmediate, (XtPointer)True},
    {XtNyAxisNumbers, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(YAxisNumbers), XtRImmediate, (XtPointer)True},
    {XtNxTime, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(XTime), XtRImmediate, (XtPointer)False},
    {XtNxTimeBase, XtCMargin, XtRInt, sizeof(int),
     offset(XTimeBase), XtRImmediate, (XtPointer)0},
    {XtNxTimeFormat, XtCString, XtRString, sizeof(String),
     offset(XTimeFormat), XtRString, ""},
    {XtNxLog, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(XLog), XtRImmediate, (XtPointer)False},
    {XtNyLog, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(YLog), XtRImmediate, (XtPointer)False},
    {XtNy2Log, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(Y2Log), XtRImmediate, (XtPointer)False},
    {XtNy3Log, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(Y3Log), XtRImmediate, (XtPointer)False},
    {XtNy4Log, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(Y4Log), XtRImmediate, (XtPointer)False},
    {XtNxOrigin, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(XOrigin), XtRImmediate, (XtPointer)False},
    {XtNyOrigin, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(YOrigin), XtRImmediate, (XtPointer)False},
    {XtNy2Origin, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(Y2Origin), XtRImmediate, (XtPointer)False},
    {XtNy3Origin, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(Y3Origin), XtRImmediate, (XtPointer)False},
    {XtNy4Origin, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(Y4Origin), XtRImmediate, (XtPointer)False},
    {XtNyNumbersHorizontal, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(YNumHorz), XtRImmediate, (XtPointer)True},
    {XtNdragX, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(DragX), XtRImmediate, (XtPointer)True},
    {XtNdragY, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(DragY), XtRImmediate, (XtPointer)False},
    {XtNtrackPointer, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(TrackPointer), XtRImmediate, (XtPointer)False},
    {XtNxNoCompMinMax, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(XNoCompMinMax), XtRImmediate, (XtPointer)False},
    {XtNdragXCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
     offset(drag_x_callback), XtRCallback, NULL},
    {XtNdragYCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
     offset(drag_y_callback), XtRCallback, NULL},
    {XtNpointerValCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
     offset(pointer_val_callback), XtRCallback, NULL},
    {XtNbtn1ClickCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
     offset(btn1_callback), XtRCallback, NULL},
    {XtNxFixedLeftRight, XtCBoolean, XtRBoolean, sizeof(Boolean),
     offset(XFixedLR), XtRImmediate, (XtPointer)False},
    {XtNxLeftSpace, XtCMargin, XtRInt, sizeof(int),
     offset(XLeftSpace), XtRImmediate, (XtPointer)(100)},
    {XtNxRightSpace, XtCMargin, XtRInt, sizeof(int),
     offset(XRightSpace), XtRImmediate, (XtPointer)(100)},
    {XmNshadowType, XmCShadowType, XmRShadowType, sizeof(unsigned char),
     offset(ShadowType), XmRImmediate, (XtPointer)XmINVALID_DIMENSION},
  };

static SciPlotFontDesc font_desc_table[] =
  {
    {XtFONT_TIMES, "Times", "times", False, True},
    {XtFONT_COURIER, "Courier", "courier", True, False},
    {XtFONT_HELVETICA, "Helvetica", "helvetica", True, False},
    {XtFONT_LUCIDA, "Lucida", "lucidabright", False, False},
    {XtFONT_LUCIDASANS, "LucidaSans", "lucida", False, False},
    {XtFONT_NCSCHOOLBOOK, "NewCenturySchlbk",
     "new century schoolbook", False, True},
    {-1, NULL, NULL, False, False},
  };

/*
 * Private function declarations
 */
static void Redisplay(Widget w, XEvent *event, Region region);
static void Resize(Widget w);
static Boolean SetValues(Widget current,
                         Widget request, Widget new,
                         ArgList args, Cardinal *nargs);
static void GetValuesHook(Widget w, ArgList args,
                          Cardinal *num_args);
static void Initialize(Widget treq, Widget tnew,
                       ArgList args, Cardinal *num);
static void Realize(Widget aw, XtValueMask *value_mask,
                    XSetWindowAttributes *attributes);
static void Destroy(Widget w);

static void ComputeAll(SciPlotWidget w, int type);
static void DrawAll(SciPlotWidget w);
static void ItemDrawAll(SciPlotWidget w);
static void ItemDraw(SciPlotWidget w, SciPlotItem *item);
static void EraseAll(SciPlotWidget w);
static void FontInit(SciPlotWidget w, SciPlotFont *pf);
static int ColorStore(SciPlotWidget w, Pixel color);
static int FontStore(SciPlotWidget w, int flag);
static int FontnumReplace(SciPlotWidget w, int fontnum, int flag);
static void SciPlotPrintf(const char *fmt, ...);

/* translation manager routines */
static void sciPlotMotionAP(SciPlotWidget w,
                            XEvent *event, char *args, int n_args);
static void sciPlotBtnUpAP(SciPlotWidget w,
                           XEvent *event, char *args, int n_args);
static void sciPlotClick(SciPlotWidget w,
                         XEvent *event, char *args, int n_args);
static void sciPlotTrackPointer(SciPlotWidget w,
                                XEvent *event, char *args, int n_args);

static void DrawShadow(SciPlotWidget w, Boolean raised, real x, real y,
                       real width, real height);

static char sciPlotTranslations[] =
#ifndef _MEDM
  "<Btn2Motion>: Motion()\n\
     <Btn2Down>: Motion()\n\
     <Btn2Up>: BtnUp()\n\
     <Btn1Up>: Btn1Up()\n\
     <Motion>: TrackPointer()";
#else
"Shift<Btn1Motion>: Motion()\n\
     Shift<Btn2Motion>: Motion()\n\
     Shift<Btn1Down>: Motion()\n\
     Shift<Btn1Up>: BtnUp()\n\
     Shift<Btn2Down>: Motion()\n\
     Shift<Btn2Up>: BtnUp()\n\
     <Btn1Up>: Btn1Up()\n\
     <Motion>: TrackPointer()";
#endif

static XtActionsRec sciPlotActionsList[] = {
  {"Motion", (XtActionProc)sciPlotMotionAP},
  {"BtnUp", (XtActionProc)sciPlotBtnUpAP},
  {"Btn1Up", (XtActionProc)sciPlotClick},
  {"TrackPointer", (XtActionProc)sciPlotTrackPointer}};

SciPlotClassRec sciplotClassRec =
  {
    {/* core_class fields        */
      /* superclass               */ (WidgetClass)&xmPrimitiveClassRec,
      /* class_name               */ "SciPlot",
      /* widget_size              */ sizeof(SciPlotRec),
      /* class_initialize         */ NULL,
      /* class_part_initialize    */ NULL,
      /* class_inited             */ False,
      /* initialize               */ Initialize,
      /* initialize_hook          */ NULL,
      /* realize                  */ Realize,
      /* actions                  */ sciPlotActionsList,
      /* num_actions              */ XtNumber(sciPlotActionsList),
      /* resources                */ resources,
      /* num_resources            */ XtNumber(resources),
      /* xrm_class                */ NULLQUARK,
      /* compress_motion          */ True,
      /* compress_exposure        */ XtExposeCompressMultiple,
      /* compress_enterleave      */ True,
      /* visible_interest         */ True,
      /* destroy                  */ Destroy,
      /* resize                   */ Resize,
      /* expose                   */ Redisplay,
      /* set_values               */ SetValues,
      /* set_values_hook          */ NULL,
      /* set_values_almost        */ XtInheritSetValuesAlmost,
      /* get_values_hook          */ GetValuesHook,
      /* accept_focus             */ NULL,
      /* version                  */ XtVersion,
      /* callback_private         */ NULL,
      /* tm_table                 */ sciPlotTranslations,
      /* query_geometry           */ NULL,
      /* display_accelerator      */ XtInheritDisplayAccelerator,
      /* extension                */ NULL},
    {/* primitive_class fields   */
      /* border_highlight         */ (XtWidgetProc)_XtInherit,
      /* border_unhighligh        */ (XtWidgetProc)_XtInherit,
      /* translations             */ XtInheritTranslations,
      /* arm_and_activate         */ (XtActionProc)_XtInherit,
      /* syn_resources            */ NULL,
      /* num_syn_resources        */ 0,
      /* extension                */ NULL},
    {
      /* plot_class fields        */
      /* dummy                    */ 0
      /* (some stupid compilers barf on empty structures) */
    }};

WidgetClass sciplotWidgetClass = (WidgetClass)&sciplotClassRec;

static void
Initialize(Widget treq, Widget tnew, ArgList args, Cardinal *num)
{
  SciPlotWidget new;

  new = (SciPlotWidget)tnew;

  new->plot.plotlist = NULL;
  new->plot.alloc_plotlist = 0;
  new->plot.num_plotlist = 0;

  new->plot.alloc_drawlist = NUMPLOTITEMALLOC;
  new->plot.drawlist = (SciPlotItem *)XtCalloc(new->plot.alloc_drawlist,
                                               sizeof(SciPlotItem));
  new->plot.num_drawlist = 0;

  new->plot.cmap = DefaultColormap(XtDisplay(new),
                                   DefaultScreen(XtDisplay(new)));

  new->plot.xlabel = (char *)XtMalloc(strlen(new->plot.TransientXLabel) + 1);
  strcpy(new->plot.xlabel, new->plot.TransientXLabel);
  new->plot.ylabel = (char *)XtMalloc(strlen(new->plot.TransientYLabel) + 1);
  strcpy(new->plot.ylabel, new->plot.TransientYLabel);
  new->plot.y2label = (char *)XtMalloc(strlen(new->plot.TransientY2Label) + 1);
  strcpy(new->plot.y2label, new->plot.TransientY2Label);
  new->plot.y3label = (char *)XtMalloc(strlen(new->plot.TransientY3Label) + 1);
  strcpy(new->plot.y3label, new->plot.TransientY3Label);
  new->plot.y4label = (char *)XtMalloc(strlen(new->plot.TransientY4Label) + 1);
  strcpy(new->plot.y4label, new->plot.TransientY4Label);
  new->plot.plotTitle = (char *)XtMalloc(strlen(new->plot.TransientPlotTitle) + 1);
  strcpy(new->plot.plotTitle, new->plot.TransientPlotTitle);
  new->plot.TransientXLabel = NULL;
  new->plot.TransientYLabel = NULL;
  new->plot.TransientY2Label = NULL;
  new->plot.TransientY3Label = NULL;
  new->plot.TransientY4Label = NULL;
  new->plot.TransientPlotTitle = NULL;

  new->plot.colors = NULL;
  new->plot.num_colors = 0;
  new->plot.fonts = NULL;
  new->plot.num_fonts = 0;

  new->plot.update = FALSE;
  new->plot.UserMin.x = new->plot.UserMin.y = 0.0;
  new->plot.UserMax.x = new->plot.UserMax.y = 10.0;
  new->plot.UserMin.y2 = 0.0;
  new->plot.UserMax.y2 = 10.0;
  new->plot.UserMin.y3 = 0.0;
  new->plot.UserMax.y3 = 10.0;
  new->plot.UserMin.y4 = 0.0;
  new->plot.UserMax.y4 = 10.0;

  new->plot.titleFont = FontStore(new, new->plot.TitleFont);
  new->plot.labelFont = FontStore(new, new->plot.LabelFont);
  new->plot.axisFont = FontStore(new, new->plot.AxisFont);

  /* set initial drag state to drag_none */
  new->plot.drag_state = NOT_DRAGGING;
  new->plot.drag_start.x = 0.0;
  new->plot.drag_start.y = 0.0;
  new->plot.startx = 0.0;
  new->plot.endx = 0.0;
  new->plot.starty = 0.0;
  new->plot.endy = 0.0;
  new->plot.reach_limit = 0;
}

static void
GCInitialize(SciPlotWidget new)
{
  XGCValues values;
  XtGCMask mask;
  long colorsave;
  values.line_style = LineSolid;
  values.line_width = 0;
  values.fill_style = FillSolid;
  values.background = WhitePixelOfScreen(XtScreen(new));
  values.background = new->core.background_pixel;
  new->plot.BackgroundColor = ColorStore(new, values.background);
  /* KE: Moved this up here 1-11-01 so 1 would be the foreground color
     as in the documentation */
  values.foreground = colorsave = BlackPixelOfScreen(XtScreen(new));
  new->plot.ForegroundColor = ColorStore(new, values.foreground);
  new->core.background_pixel = values.background;
  {
    Pixel fg, select, shade1, shade2;

    XmGetColors(XtScreen(new), new->core.colormap,
                new->core.background_pixel, &fg, &shade1, &shade2, &select);
    new->plot.ShadowColor1 = ColorStore(new, shade1);
    new->plot.ShadowColor2 = ColorStore(new, shade2);
    new->primitive.top_shadow_color = shade1;
    new->primitive.bottom_shadow_color = shade2;
  }

  mask = GCLineStyle | GCLineWidth | GCFillStyle | GCForeground | GCBackground;
  new->plot.defaultGC = XCreateGC(XtDisplay(new), XtWindow(new), mask, &values);
  /* Eliminate events that we do not handle anyway */
  XSetGraphicsExposures(XtDisplay(new), new->plot.defaultGC, False);

  values.foreground = colorsave;
  values.line_style = LineOnOffDash;
  new->plot.dashGC = XCreateGC(XtDisplay(new), XtWindow(new), mask, &values);
  /* Eliminate events that we do not handle anyway */
  XSetGraphicsExposures(XtDisplay(new), new->plot.dashGC, False);
}

static void
Realize(Widget aw, XtValueMask *value_mask, XSetWindowAttributes *attributes)
{
  SciPlotWidget w = (SciPlotWidget)aw;

#define superclass (&widgetClassRec)
  (*superclass->core_class.realize)(aw, value_mask, attributes);
#undef superclass

  GCInitialize(w);
}

static void
Destroy(Widget ws)
{
  SciPlotWidget w = (SciPlotWidget)ws;
  int i, j;
  SciPlotFont *pf;
  SciPlotList *p;

  XFreeGC(XtDisplay(w), w->plot.defaultGC);
  XFreeGC(XtDisplay(w), w->plot.dashGC);
  XtFree((char *)w->plot.xlabel);
  XtFree((char *)w->plot.ylabel);
  XtFree((char *)w->plot.y2label);
  XtFree((char *)w->plot.y3label);
  XtFree((char *)w->plot.y4label);
  XtFree((char *)w->plot.plotTitle);

  for (i = 0; i < w->plot.num_fonts; i++)
    {
      pf = &w->plot.fonts[i];
      XFreeFont(XtDisplay((Widget)w), pf->font);
    }
  XtFree((char *)w->plot.fonts);

  XtFree((char *)w->plot.colors);

  for (i = 0; i < w->plot.alloc_plotlist; i++)
    {
      p = w->plot.plotlist + i;
      if (p->allocated > 0)
        XtFree((char *)p->data);
      if (p->markertext)
        {
          for (j = 0; j < p->number; j++)
            {
              if (p->markertext[j])
                XtFree(p->markertext[j]);
            }
        }
    }
  if (w->plot.alloc_plotlist > 0)
    XtFree((char *)w->plot.plotlist);

  EraseAll(w);
  XtFree((char *)w->plot.drawlist);
}

static Boolean
SetValues(Widget currents, Widget requests, Widget news,
          ArgList args, Cardinal *nargs)
/* KE: requests, args, nargs are not used */
{
  SciPlotWidget current = (SciPlotWidget)currents;
  SciPlotWidget new = (SciPlotWidget)news;
  Boolean redisplay = FALSE;

  if (current->plot.XLog != new->plot.XLog)
    redisplay = TRUE;
  else if (current->plot.XTime != new->plot.XTime)
    redisplay = TRUE;
  else if (current->plot.YLog != new->plot.YLog)
    redisplay = TRUE;
  else if (current->plot.Y2Log != new->plot.Y2Log)
    redisplay = TRUE;
  else if (current->plot.Y3Log != new->plot.Y3Log)
    redisplay = TRUE;
  else if (current->plot.Y4Log != new->plot.Y4Log)
    redisplay = TRUE;
  else if (current->plot.XOrigin != new->plot.XOrigin)
    redisplay = TRUE;
  else if (current->plot.YOrigin != new->plot.YOrigin)
    redisplay = TRUE;
  else if (current->plot.Y2Origin != new->plot.Y2Origin)
    redisplay = TRUE;
  else if (current->plot.Y3Origin != new->plot.Y3Origin)
    redisplay = TRUE;
  else if (current->plot.Y4Origin != new->plot.Y4Origin)
    redisplay = TRUE;
  else if (current->plot.XAxisNumbers != new->plot.XAxisNumbers)
    redisplay = TRUE;
  else if (current->plot.YAxisNumbers != new->plot.YAxisNumbers)
    redisplay = TRUE;
  else if (current->plot.DrawMajor != new->plot.DrawMajor)
    redisplay = TRUE;
  else if (current->plot.DrawMajorTics != new->plot.DrawMajorTics)
    redisplay = TRUE;
  else if (current->plot.DrawMinor != new->plot.DrawMinor)
    redisplay = TRUE;
  else if (current->plot.DrawMinorTics != new->plot.DrawMinorTics)
    redisplay = TRUE;
  else if (current->plot.ChartType != new->plot.ChartType)
    redisplay = TRUE;
  else if (current->plot.Degrees != new->plot.Degrees)
    redisplay = TRUE;
  else if (current->plot.ShowTitle != new->plot.ShowTitle)
    redisplay = TRUE;
  else if (current->plot.ShowXLabel != new->plot.ShowXLabel)
    redisplay = TRUE;
  else if (current->plot.ShowYLabel != new->plot.ShowYLabel)
    redisplay = TRUE;
  else if (current->plot.ShowY2Label != new->plot.ShowY2Label)
    redisplay = TRUE;
  else if (current->plot.ShowY3Label != new->plot.ShowY3Label)
    redisplay = TRUE;
  else if (current->plot.ShowY4Label != new->plot.ShowY4Label)
    redisplay = TRUE;
  else if (current->plot.Monochrome != new->plot.Monochrome)
    redisplay = TRUE;
  else if (current->plot.XFixedLR != new->plot.XFixedLR)
    redisplay = TRUE;
  else if ((current->plot.XLeftSpace != new->plot.XLeftSpace ||
            current->plot.XRightSpace != new->plot.XRightSpace) &&
           new->plot.XFixedLR)
    redisplay = TRUE;

  if (new->plot.TransientXLabel)
    {
      if (current->plot.TransientXLabel != new->plot.TransientXLabel ||
          strcmp(new->plot.TransientXLabel, current->plot.xlabel) != 0)
        {
          redisplay = TRUE;
          XtFree(current->plot.xlabel);
          new->plot.xlabel = (char *)XtMalloc(strlen(new->plot.TransientXLabel) + 1);
          strcpy(new->plot.xlabel, new->plot.TransientXLabel);
          new->plot.TransientXLabel = NULL;
        }
    }
  if (new->plot.TransientYLabel)
    {
      if (current->plot.TransientYLabel != new->plot.TransientYLabel ||
          strcmp(new->plot.TransientYLabel, current->plot.ylabel) != 0)
        {
          redisplay = TRUE;
          XtFree(current->plot.ylabel);
          new->plot.ylabel = (char *)XtMalloc(strlen(new->plot.TransientYLabel) + 1);
          strcpy(new->plot.ylabel, new->plot.TransientYLabel);
          new->plot.TransientYLabel = NULL;
        }
    }
  if (new->plot.TransientY2Label)
    {
      if (current->plot.TransientY2Label != new->plot.TransientY2Label ||
          strcmp(new->plot.TransientY2Label, current->plot.y2label) != 0)
        {
          redisplay = TRUE;
          XtFree(current->plot.y2label);
          new->plot.y2label = (char *)XtMalloc(strlen(new->plot.TransientY2Label) + 1);
          strcpy(new->plot.y2label, new->plot.TransientY2Label);
          new->plot.TransientY2Label = NULL;
        }
    }
  if (new->plot.TransientY3Label)
    {
      if (current->plot.TransientY3Label != new->plot.TransientY3Label ||
          strcmp(new->plot.TransientY3Label, current->plot.y3label) != 0)
        {
          redisplay = TRUE;
          XtFree(current->plot.y3label);
          new->plot.y3label = (char *)XtMalloc(strlen(new->plot.TransientY3Label) + 1);
          strcpy(new->plot.y3label, new->plot.TransientY3Label);
          new->plot.TransientY3Label = NULL;
        }
    }
  if (new->plot.TransientY4Label)
    {
      if (current->plot.TransientY4Label != new->plot.TransientY4Label ||
          strcmp(new->plot.TransientY4Label, current->plot.y4label) != 0)
        {
          redisplay = TRUE;
          XtFree(current->plot.y4label);
          new->plot.y4label = (char *)XtMalloc(strlen(new->plot.TransientY4Label) + 1);
          strcpy(new->plot.y4label, new->plot.TransientY4Label);
          new->plot.TransientY4Label = NULL;
        }
    }
  if (new->plot.TransientPlotTitle)
    {
      if (current->plot.TransientPlotTitle != new->plot.TransientPlotTitle ||
          strcmp(new->plot.TransientPlotTitle, current->plot.plotTitle) != 0)
        {
          redisplay = TRUE;
          XtFree(current->plot.plotTitle);
          new->plot.plotTitle = (char *)XtMalloc(strlen(new->plot.TransientPlotTitle) + 1);
          strcpy(new->plot.plotTitle, new->plot.TransientPlotTitle);
          new->plot.TransientPlotTitle = NULL;
        }
    }

  if (current->plot.AxisFont != new->plot.AxisFont)
    {
      redisplay = TRUE;
      FontnumReplace(new, new->plot.axisFont, new->plot.AxisFont);
    }
  if (current->plot.TitleFont != new->plot.TitleFont)
    {
      redisplay = TRUE;
      FontnumReplace(new, new->plot.titleFont, new->plot.TitleFont);
    }
  if (current->plot.LabelFont != new->plot.LabelFont)
    {
      redisplay = TRUE;
      FontnumReplace(new, new->plot.labelFont, new->plot.LabelFont);
    }

  new->plot.update = redisplay;

  return redisplay;
}

static void
GetValuesHook(Widget ws, ArgList args, Cardinal *num_args)
{
  SciPlotWidget w = (SciPlotWidget)ws;
  int i;
  char **loc;

  for (i = 0; i < (int)*num_args; i++)
    {
      loc = (char **)args[i].value;
      if (strcmp(args[i].name, XtNplotTitle) == 0)
        *loc = w->plot.plotTitle;
      else if (strcmp(args[i].name, XtNxLabel) == 0)
        *loc = w->plot.xlabel;
      else if (strcmp(args[i].name, XtNyLabel) == 0)
        *loc = w->plot.ylabel;
      else if (strcmp(args[i].name, XtNy2Label) == 0)
        *loc = w->plot.y2label;
      else if (strcmp(args[i].name, XtNy3Label) == 0)
        *loc = w->plot.y3label;
      else if (strcmp(args[i].name, XtNy4Label) == 0)
        *loc = w->plot.y4label;
    }
}

static void
Redisplay(Widget ws, XEvent *event, Region region)
/* KE: event, region not used */
{
  SciPlotWidget w = (SciPlotWidget)ws;
  if (!XtIsRealized((Widget)w))
    return;
  if (w->plot.update)
    {
      Resize(ws);
      w->plot.update = FALSE;
    }
  else
    {
      ItemDrawAll(w);
    }
}

static void
Resize(Widget ws)
{
  SciPlotWidget w = (SciPlotWidget)ws;
  if (!XtIsRealized((Widget)w))
    return;

  EraseAll(w);
  ComputeAll(w, COMPUTE_MIN_MAX);
  DrawAll(w);
}

/*
 * Private SciPlot utility functions
 */

static int
ColorStore(SciPlotWidget w, Pixel color)
{
  int i;

  /* Check if it is there */
  for (i = 0; i < w->plot.num_colors; i++)
    {
      if (w->plot.colors[i] == color)
        return i;
    }

  /* Not found, add it */
  w->plot.num_colors++;
  w->plot.colors = (Pixel *)XtRealloc((char *)w->plot.colors,
                                      sizeof(Pixel) * w->plot.num_colors);
  w->plot.colors[w->plot.num_colors - 1] = color;
  return w->plot.num_colors - 1;
}

static void
FontnumStore(SciPlotWidget w, int fontnum, int flag)
{
  SciPlotFont *pf;
  int fontflag, sizeflag, attrflag;

  pf = &w->plot.fonts[fontnum];

  fontflag = flag & XtFONT_NAME_MASK;
  sizeflag = flag & XtFONT_SIZE_MASK;
  attrflag = flag & XtFONT_ATTRIBUTE_MASK;

  switch (fontflag)
    {
    case XtFONT_TIMES:
    case XtFONT_COURIER:
    case XtFONT_HELVETICA:
    case XtFONT_LUCIDA:
    case XtFONT_LUCIDASANS:
    case XtFONT_NCSCHOOLBOOK:
      break;
    default:
      fontflag = XtFONT_NAME_DEFAULT;
      break;
    }

  if (sizeflag < 1)
    sizeflag = XtFONT_SIZE_DEFAULT;

  switch (attrflag)
    {
    case XtFONT_BOLD:
    case XtFONT_ITALIC:
    case XtFONT_BOLD_ITALIC:
      break;
    default:
      attrflag = XtFONT_ATTRIBUTE_DEFAULT;
      break;
    }
  pf->id = flag;
  FontInit(w, pf);
}

static int
FontnumReplace(SciPlotWidget w, int fontnum, int flag)
{
  SciPlotFont *pf;

  pf = &w->plot.fonts[fontnum];
  XFreeFont(XtDisplay(w), pf->font);

  FontnumStore(w, fontnum, flag);

  return fontnum;
}

static int
FontStore(SciPlotWidget w, int flag)
{
  int fontnum;

  w->plot.num_fonts++;
  w->plot.fonts = (SciPlotFont *)XtRealloc((char *)w->plot.fonts,
                                           sizeof(SciPlotFont) * w->plot.num_fonts);
  fontnum = w->plot.num_fonts - 1;

  FontnumStore(w, fontnum, flag);

  return fontnum;
}

static SciPlotFontDesc *
FontDescLookup(int flag)
{
  SciPlotFontDesc *pfd;

  pfd = font_desc_table;
  while (pfd->flag >= 0)
    {
#if DEBUG_SCIPLOT
      SciPlotPrintf("FontDescLookup: checking if %d == %d (font %s)\n",
                    flag & XtFONT_NAME_MASK, pfd->flag, pfd->PostScript);
#endif
      if ((flag & XtFONT_NAME_MASK) == pfd->flag)
        return pfd;
      pfd++;
    }
  return NULL;
}

static void
FontX11String(int flag, char *str)
{
  SciPlotFontDesc *pfd;

  pfd = FontDescLookup(flag);
  if (pfd)
    {
      sprintf(str, "-*-%s-%s-%s-*-*-%d-*-*-*-*-*-*-*",
              pfd->X11,
              (flag & XtFONT_BOLD ? "bold" : "medium"),
              (flag & XtFONT_ITALIC ? (pfd->PSUsesOblique ? "o" : "i") : "r"),
              (flag & XtFONT_SIZE_MASK));
    }
  else
    sprintf(str, "fixed");
#if DEBUG_SCIPLOT
  SciPlotPrintf("FontX11String: font string=%s\n", str);
#endif
}

static void
FontInit(SciPlotWidget w, SciPlotFont *pf)
{
  char str[256], **list;
  int num;

  FontX11String(pf->id, str);
  list = XListFonts(XtDisplay(w), str, 100, &num);
#if DEBUG_SCIPLOT
  if (1)
    {
      int i;

      i = 0;
      while (i < num)
        {
          SciPlotPrintf("FontInit: Found font: %s\n", list[i]);
          i++;
        }
    }
#endif
  if (num <= 0)
    {
      pf->id &= ~XtFONT_ATTRIBUTE_MASK;
      pf->id |= XtFONT_ATTRIBUTE_DEFAULT;
      FontX11String(pf->id, str);
      list = XListFonts(XtDisplay(w), str, 100, &num);
#if DEBUG_SCIPLOT
      if (1)
        {
          int i;

          i = 0;
          while (i < num)
            {
              SciPlotPrintf("FontInit: Attr reset: found: %s\n", list[i]);
              i++;
            }
        }
#endif
    }
  if (num <= 0)
    {
      pf->id &= ~XtFONT_NAME_MASK;
      pf->id |= XtFONT_NAME_DEFAULT;
      FontX11String(pf->id, str);
      list = XListFonts(XtDisplay(w), str, 100, &num);
#if DEBUG_SCIPLOT
      if (1)
        {
          int i;

          i = 0;
          while (i < num)
            {
              SciPlotPrintf("FontInit: Name reset: found: %s\n", list[i]);
              i++;
            }
        }
#endif
    }
  if (num <= 0)
    {
      pf->id &= ~XtFONT_SIZE_MASK;
      pf->id |= XtFONT_SIZE_DEFAULT;
      FontX11String(pf->id, str);
      list = XListFonts(XtDisplay(w), str, 100, &num);
#if DEBUG_SCIPLOT
      if (1)
        {
          int i;

          i = 0;
          while (i < num)
            {
              SciPlotPrintf("FontInit: Size reset: found: %s\n", list[i]);
              i++;
            }
        }
#endif
    }
#if DEBUG_SCIPLOT
  if (1)
    {
      XFontStruct *f;
      int i;

      i = 0;
      while (i < num)
        {
          SciPlotPrintf("FontInit: Properties for: %s\n", list[i]);
          f = XLoadQueryFont(XtDisplay(w), list[i]);
          SciPlotPrintf("  Height: %3d   Ascent: %d   Descent: %d\n",
                        f->max_bounds.ascent + f->max_bounds.descent,
                        f->max_bounds.ascent, f->max_bounds.descent);
          XFreeFont(XtDisplay(w), f);
          i++;
        }
    }
#endif
  if (num > 0)
    {
      /* Use the first one in the list */
      /* KE: Used to use str instead of list[0] but this failed on Linux
       * Enterprise for some unknown reason */
      pf->font = XLoadQueryFont(XtDisplay(w), list[0]);
      XFreeFontNames(list);
    }
  else
    {
      /* Use fixed */
      pf->font = XLoadQueryFont(XtDisplay(w), "fixed");
    }
#if DEBUG_SCIPLOT
  SciPlotPrintf("FontInit: Finally ran XLoadQueryFont using: %s\n", str);
  SciPlotPrintf("  Height: %3d   Ascent: %d   Descent: %d\n",
                pf->font->max_bounds.ascent + pf->font->max_bounds.descent,
                pf->font->max_bounds.ascent, pf->font->max_bounds.descent);
#endif
}

static XFontStruct *
FontFromFontnum(SciPlotWidget w, int fontnum)
{
  XFontStruct *f;

  if (fontnum >= w->plot.num_fonts)
    fontnum = 0;
  f = w->plot.fonts[fontnum].font;
  return f;
}

static real
FontHeight(XFontStruct *f)
{
  return (real)(f->max_bounds.ascent + f->max_bounds.descent);
}

static real
FontnumHeight(SciPlotWidget w, int fontnum)
{
  XFontStruct *f;

  f = FontFromFontnum(w, fontnum);
  return FontHeight(f);
}

static real
FontDescent(XFontStruct *f)
{
  return (real)(f->max_bounds.descent);
}

static real
FontnumDescent(SciPlotWidget w, int fontnum)
{
  XFontStruct *f;

  f = FontFromFontnum(w, fontnum);
  return FontDescent(f);
}

static real
FontAscent(XFontStruct *f)
{
  return (real)(f->max_bounds.ascent);
}

static real
FontnumAscent(SciPlotWidget w, int fontnum)
{
  XFontStruct *f;

  f = FontFromFontnum(w, fontnum);
  return FontAscent(f);
}

static real
FontTextWidth(XFontStruct *f, char *c)
{
  return (real)XTextWidth(f, c, strlen(c));
}

static real
FontnumTextWidth(SciPlotWidget w, int fontnum, char *c)
{
  XFontStruct *f;

  f = FontFromFontnum(w, fontnum);
  return FontTextWidth(f, c);
}

/*
 * Private List functions
 */

static int
_ListNew(SciPlotWidget w)
{
  int index;
  SciPlotList *p;
  Boolean found;

  /* First check to see if there is any free space in the index */
  found = FALSE;
  for (index = 0; index < w->plot.num_plotlist; index++)
    {
      p = w->plot.plotlist + index;
      if (!p->used)
        {
          found = TRUE;
          break;
        }
    }

  /* If no space is found, increase the size of the index */
  if (!found)
    {
      w->plot.num_plotlist++;
      if (w->plot.alloc_plotlist == 0)
        {
          w->plot.alloc_plotlist = NUMPLOTLINEALLOC;
          w->plot.plotlist = (SciPlotList *)XtCalloc(w->plot.alloc_plotlist, sizeof(SciPlotList));
          if (!w->plot.plotlist)
            {
              SciPlotPrintf("Can't calloc memory for SciPlotList\n");
              exit(1);
            }
          w->plot.alloc_plotlist = NUMPLOTLINEALLOC;
        }
      else if (w->plot.num_plotlist > w->plot.alloc_plotlist)
        {
          w->plot.alloc_plotlist += NUMPLOTLINEALLOC;
          w->plot.plotlist = (SciPlotList *)XtRealloc((char *)w->plot.plotlist,
                                                      w->plot.alloc_plotlist * sizeof(SciPlotList));
          if (!w->plot.plotlist)
            {
              SciPlotPrintf("Can't realloc memory for SciPlotList\n");
              exit(1);
            }
          /* KE: XtRealloc does not zero memory as does the original XtCalloc
           *   This will cause problems in Destroy for data, legend, markertest, etc.*/
          p = w->plot.plotlist + w->plot.alloc_plotlist - NUMPLOTLINEALLOC;
          memset(p, '\0', NUMPLOTLINEALLOC * sizeof(SciPlotList));
        }
      index = w->plot.num_plotlist - 1;
      p = w->plot.plotlist + index;
    }

  /* KE: Since the lists have been zeroed, only the non-zero ones
   *   really need to be set */
  p->LineStyle = p->FillStyle = p->LineColor = p->PointStyle = p->PointColor = 0;
  p->number = p->allocated = 0;
  p->data = NULL;
  p->draw = p->used = TRUE;
  p->markersize = (real)w->plot.DefaultMarkerSize;
  p->markertext = NULL;

#if DEBUG_SCIPLOT_ALLOC
  SciPlotPrintf("_ListNew: alloc_plotlist=%d num_plotlist=%d\n",
                w->plot.alloc_plotlist, w->plot.num_plotlist);
#endif

  return index;
}

static void
_ListDelete(SciPlotList *p)
{
  int i;

  if (p->markertext)
    {
      for (i = 0; i < p->number; i++)
        {
          if (p->markertext[i])
            {
              XtFree(p->markertext[i]);
              p->markertext[i] = NULL;
            }
        }
      XtFree((char *)p->markertext);
      p->markertext = NULL;
    }

  p->draw = p->used = FALSE;
  p->number = p->allocated = 0;
  if (p->data)
    XtFree((char *)p->data);
  p->data = NULL;

#if DEBUG_SCIPLOT_ALLOC
  SciPlotPrintf("_ListDelete:\n");
#endif
}

static SciPlotList *
_ListFind(SciPlotWidget w, int id)
{
  SciPlotList *p;

  if ((id >= 0) && (id < w->plot.num_plotlist))
    {
      p = w->plot.plotlist + id;
      if (p->used)
        return p;
    }
  return NULL;
}

static void
_ListSetStyle(SciPlotList *p, int pcolor, int pstyle, int lcolor, int lstyle, int fstyle)
{
  /* Note!  Do checks in here later on... */

  if (lstyle >= 0)
    p->LineStyle = lstyle;
  if (fstyle >= 0)
    p->FillStyle = fstyle;
  if (lcolor >= 0)
    p->LineColor = lcolor;
  if (pstyle >= 0)
    p->PointStyle = pstyle;
  if (pcolor >= 0)
    p->PointColor = pcolor;
}

static void
_ListAllocData(SciPlotList *p, int num)
{
  if (p->data)
    {
      XtFree((char *)p->data);
      p->allocated = 0;
    }
  p->allocated = num + NUMPLOTDATAEXTRA;
  p->data = (realpair *)XtCalloc(p->allocated, sizeof(realpair));
  if (!p->data)
    {
      p->number = p->allocated = 0;
    }
}

static void
_ListReallocData(SciPlotList *p, int more)
{
  if (!p->data)
    {
      _ListAllocData(p, more);
    }
  else if (p->number + more > p->allocated)
    {
      /* MSE, 5/16/03: changed to 2x growth for efficiency with larger arrays */
      /* p->allocated += more + NUMPLOTDATAEXTRA;   old: constant growth */
      p->allocated = 2 * (p->number + more); /* new: factor of 2 growth */
      p->data = (realpair *)XtRealloc((char *)p->data, p->allocated * sizeof(realpair));
      if (!p->data)
        {
          p->number = p->allocated = 0;
        }
    }
}

static void
_ListAddDouble(SciPlotList *p, int num, double *xlist, double *ylist)
{
  int i;

  _ListReallocData(p, num);
  if (p->data)
    {
      for (i = 0; i < num; i++)
        {
          p->data[i + p->number].x = xlist[i];
          p->data[i + p->number].y = ylist[i];
        }
      p->number += num;
    }
}

static void
_ListSetDouble(SciPlotList *p, int num, double *xlist, double *ylist)
{
  if ((!p->data) || (p->allocated < num))
    _ListAllocData(p, num);
  p->number = 0;
  _ListAddDouble(p, num, xlist, ylist);
}

/*
 * Private SciPlot functions
 */

/*
 * The following vertical text drawing routine uses the "Fill Stippled" idea
 * found in xvertext-5.0, by Alan Richardson (mppa3@syma.sussex.ac.uk).
 *
 * The following code is my interpretation of his idea, including some
 * hacked together excerpts from his source.  The credit for the clever bits
 * belongs to him.
 *
 * To be complete, portions of the subroutine XDrawVString are
 * Copyright (c) 1993 Alan Richardson (mppa3@syma.sussex.ac.uk)
 */
static void
XDrawVString(Display *display, Window win, GC gc, int x, int y, char *str, int len, XFontStruct *f)
{
  XImage *before, *after;
  char *dest, *source, *source0, *source1;
  int xloop, yloop, xdest, ydest;
  Pixmap pix, rotpix;
  int width, height;
  GC drawGC;

  width = (int)FontTextWidth(f, str);
  height = (int)FontHeight(f);

  pix = XCreatePixmap(display, win, width, height, 1);
  rotpix = XCreatePixmap(display, win, height, width, 1);

  drawGC = XCreateGC(display, pix, 0L, NULL);
  XSetBackground(display, drawGC, 0);
  XSetFont(display, drawGC, f->fid);
  XSetForeground(display, drawGC, 0);
  XFillRectangle(display, pix, drawGC, 0, 0, width, height);
  XFillRectangle(display, rotpix, drawGC, 0, 0, height, width);
  XSetForeground(display, drawGC, 1);
  /* Eliminate events that we do not handle anyway */
  XSetGraphicsExposures(display, drawGC, False);

  XDrawImageString(display, pix, drawGC, 0, (int)FontAscent(f),
                   str, strlen(str));

  source0 = (char *)calloc((((width + 7) / 8) * height), 1);
  before = XCreateImage(display, DefaultVisual(display, DefaultScreen(display)),
                        1, XYPixmap, 0, source0, width, height, 8, 0);
  before->byte_order = before->bitmap_bit_order = MSBFirst;
  XGetSubImage(display, pix, 0, 0, width, height, 1L, XYPixmap, before, 0, 0);

  source1 = (char *)calloc((((height + 7) / 8) * width), 1);
  after = XCreateImage(display, DefaultVisual(display, DefaultScreen(display)),
                       1, XYPixmap, 0, source1, height, width, 8, 0);
  after->byte_order = after->bitmap_bit_order = MSBFirst;

  for (yloop = 0; yloop < height; yloop++)
    {
      for (xloop = 0; xloop < width; xloop++)
        {
          source = before->data + (xloop / 8) +
            (yloop * before->bytes_per_line);
          if (*source & (128 >> (xloop % 8)))
            {
              dest = after->data + (yloop / 8) +
                ((width - 1 - xloop) * after->bytes_per_line);
              *dest |= (128 >> (yloop % 8));
            }
        }
    }

#if DEBUG_SCIPLOT_VTEXT
  if (1)
    {
      char sourcebit;

      for (yloop = 0; yloop < before->height; yloop++)
        {
          for (xloop = 0; xloop < before->width; xloop++)
            {
              source = before->data + (xloop / 8) +
                (yloop * before->bytes_per_line);
              sourcebit = *source & (128 >> (xloop % 8));
              if (sourcebit)
                putchar('X');
              else
                putchar('.');
            }
          putchar('\n');
        }

      for (yloop = 0; yloop < after->height; yloop++)
        {
          for (xloop = 0; xloop < after->width; xloop++)
            {
              source = after->data + (xloop / 8) +
                (yloop * after->bytes_per_line);
              sourcebit = *source & (128 >> (xloop % 8));
              if (sourcebit)
                putchar('X');
              else
                putchar('.');
            }
          putchar('\n');
        }
    }
#endif

  xdest = x - (int)FontAscent(f);
  if (xdest < 0)
    xdest = 0;
  ydest = y - width;

  XPutImage(display, rotpix, drawGC, after, 0, 0, 0, 0,
            after->width, after->height);

  XSetFillStyle(display, gc, FillStippled);
  XSetStipple(display, gc, rotpix);
  XSetTSOrigin(display, gc, xdest, ydest);
  XFillRectangle(display, win, gc, xdest, ydest, after->width, after->height);
  XSetFillStyle(display, gc, FillSolid);

  XFreeGC(display, drawGC);

  /* Free image data ourselves and mark it as null in the XImage */
  free(source0);
  free(source1);
  before->data = 0;
  after->data = 0;

  XDestroyImage(before);
  XDestroyImage(after);
  XFreePixmap(display, pix);
  XFreePixmap(display, rotpix);
}

static char dots[] =
  {2, 1, 1};
static char widedots[] =
  {2, 1, 4};

static GC
ItemGetGC(SciPlotWidget w, SciPlotItem *item)
{
  GC gc;
  short color;

  switch (item->kind.any.style)
    {
    case XtLINE_SOLID:
      gc = w->plot.defaultGC;
      break;
    case XtLINE_DOTTED:
      XSetDashes(XtDisplay(w), w->plot.dashGC, 0, &dots[1],
                 (int)dots[0]);
      gc = w->plot.dashGC;
      break;
    case XtLINE_WIDEDOT:
      XSetDashes(XtDisplay(w), w->plot.dashGC, 0, &widedots[1],
                 (int)widedots[0]);
      gc = w->plot.dashGC;
      break;
    default:
      return NULL;
    }
  if (w->plot.Monochrome)
    if (item->kind.any.color > 0)
      color = w->plot.ForegroundColor;
    else
      color = w->plot.BackgroundColor;
  else if (item->kind.any.color >= w->plot.num_colors)
    color = w->plot.ForegroundColor;
  else if (item->kind.any.color <= 0)
    color = w->plot.BackgroundColor;
  else
    color = item->kind.any.color;
  XSetForeground(XtDisplay(w), gc, w->plot.colors[color]);
  return gc;
}

static GC
ItemGetFontGC(SciPlotWidget w, SciPlotItem *item)
{
  GC gc;
  short color, fontnum;

  gc = w->plot.dashGC;
  if (w->plot.Monochrome)
    if (item->kind.any.color > 0)
      color = w->plot.ForegroundColor;
    else
      color = w->plot.BackgroundColor;
  else if (item->kind.any.color >= w->plot.num_colors)
    color = w->plot.ForegroundColor;
  else if (item->kind.any.color <= 0)
    color = w->plot.BackgroundColor;
  else
    color = item->kind.any.color;
  XSetForeground(XtDisplay(w), gc, w->plot.colors[color]);
  if (item->kind.text.font >= w->plot.num_fonts)
    fontnum = 0;
  else
    fontnum = item->kind.text.font;

  /*
   * fontnum==0 hack:  0 is supposed to be the default font, but the program
   * can't seem to get the default font ID from the GC for some reason.  So,
   * use a different GC where the default font still exists.
   */
  XSetFont(XtDisplay(w), gc, w->plot.fonts[fontnum].font->fid);
  return gc;
}

static void
ItemDraw(SciPlotWidget w, SciPlotItem *item)
{
  XPoint point[8];
  XSegment seg;
  XRectangle rect;
  int i;
  GC gc;

  if (!XtIsRealized((Widget)w))
    return;
  if ((item->type > SciPlotStartTextTypes) && (item->type < SciPlotEndTextTypes))
    gc = ItemGetFontGC(w, item);
  else
    gc = ItemGetGC(w, item);
  if (!gc)
    return;
#if DEBUG_SCIPLOT_LINE
  SciPlotPrintf("ItemDraw: item->type=%d\n"
                "  SciPlotFALSE=%d SciPlotPoint=%d  SciPlotLine=%d \n",
                item->type, SciPlotFALSE, SciPlotPoint, SciPlotLine);
#endif
  switch (item->type)
    {
    case SciPlotLine:
      seg.x1 = (short)item->kind.line.x1;
      seg.y1 = (short)item->kind.line.y1;
      seg.x2 = (short)item->kind.line.x2;
      seg.y2 = (short)item->kind.line.y2;
      XDrawSegments(XtDisplay(w), XtWindow(w), gc,
                    &seg, 1);
      break;
    case SciPlotRect:
      XDrawRectangle(XtDisplay(w), XtWindow(w), gc,
                     (int)(item->kind.rect.x),
                     (int)(item->kind.rect.y),
                     (unsigned int)(item->kind.rect.w),
                     (unsigned int)(item->kind.rect.h));
      break;
    case SciPlotFRect:
      XFillRectangle(XtDisplay(w), XtWindow(w), gc,
                     (int)(item->kind.rect.x),
                     (int)(item->kind.rect.y),
                     (unsigned int)(item->kind.rect.w),
                     (unsigned int)(item->kind.rect.h));
      XDrawRectangle(XtDisplay(w), XtWindow(w), gc,
                     (int)(item->kind.rect.x),
                     (int)(item->kind.rect.y),
                     (unsigned int)(item->kind.rect.w),
                     (unsigned int)(item->kind.rect.h));
      break;
    case SciPlotPoly:
      i = 0;
      while (i < item->kind.poly.count)
        {
          point[i].x = (int)item->kind.poly.x[i];
          point[i].y = (int)item->kind.poly.y[i];
          i++;
        }
      point[i].x = (int)item->kind.poly.x[0];
      point[i].y = (int)item->kind.poly.y[0];
      XDrawLines(XtDisplay(w), XtWindow(w), gc,
                 point, i + 1, CoordModeOrigin);
      break;
    case SciPlotFPoly:
      i = 0;
      while (i < item->kind.poly.count)
        {
          point[i].x = (int)item->kind.poly.x[i];
          point[i].y = (int)item->kind.poly.y[i];
          i++;
        }
      point[i].x = (int)item->kind.poly.x[0];
      point[i].y = (int)item->kind.poly.y[0];
      XFillPolygon(XtDisplay(w), XtWindow(w), gc,
                   point, i + 1, Complex, CoordModeOrigin);
      XDrawLines(XtDisplay(w), XtWindow(w), gc,
                 point, i + 1, CoordModeOrigin);
      break;
    case SciPlotCircle:
      XDrawArc(XtDisplay(w), XtWindow(w), gc,
               (int)(item->kind.circ.x - item->kind.circ.r),
               (int)(item->kind.circ.y - item->kind.circ.r),
               (unsigned int)(item->kind.circ.r * 2),
               (unsigned int)(item->kind.circ.r * 2),
               0 * 64, 360 * 64);
      break;
    case SciPlotFCircle:
      XFillArc(XtDisplay(w), XtWindow(w), gc,
               (int)(item->kind.circ.x - item->kind.circ.r),
               (int)(item->kind.circ.y - item->kind.circ.r),
               (unsigned int)(item->kind.circ.r * 2),
               (unsigned int)(item->kind.circ.r * 2),
               0 * 64, 360 * 64);
      break;
    case SciPlotText:
      XDrawString(XtDisplay(w), XtWindow(w), gc,
                  (int)(item->kind.text.x), (int)(item->kind.text.y),
                  item->kind.text.text,
                  (int)item->kind.text.length);
      break;
    case SciPlotVText:
      XDrawVString(XtDisplay(w), XtWindow(w), gc,
                   (int)(item->kind.text.x), (int)(item->kind.text.y),
                   item->kind.text.text,
                   (int)item->kind.text.length,
                   FontFromFontnum(w, item->kind.text.font));
      break;
    case SciPlotClipRegion:
      rect.x = (short)item->kind.line.x1;
      rect.y = (short)item->kind.line.y1;
      rect.width = (short)item->kind.line.x2;
      rect.height = (short)item->kind.line.y2;
      XSetClipRectangles(XtDisplay(w), w->plot.dashGC, 0, 0, &rect, 1, Unsorted);
      XSetClipRectangles(XtDisplay(w), w->plot.defaultGC, 0, 0, &rect, 1, Unsorted);
      break;
    case SciPlotClipClear:
      XSetClipMask(XtDisplay(w), w->plot.dashGC, None);
      XSetClipMask(XtDisplay(w), w->plot.defaultGC, None);
      break;
    default:
#if DEBUG_SCIPLOT_LINE
      SciPlotPrintf("ItemDraw: default case\n");
#endif
      break;
    }
}

static void
ItemDrawAll(SciPlotWidget w)
{
  SciPlotItem *item;
  int i;

  if (!XtIsRealized((Widget)w))
    return;
  item = w->plot.drawlist;
  i = 0;
  while (i < w->plot.num_drawlist)
    {
      ItemDraw(w, item);
      i++;
      item++;
    }
}

/*
 * Private device independent drawing functions
 */

static void
EraseClassItems(SciPlotWidget w, SciPlotDrawingEnum drawing)
{
  SciPlotItem *item;
  int i;

  if (!XtIsRealized((Widget)w))
    return;
  item = w->plot.drawlist;
  i = 0;
  while (i < w->plot.num_drawlist)
    {
      if (item->drawing_class == drawing)
        {
          item->kind.any.color = 0;
          item->kind.any.style = XtLINE_SOLID;
          ItemDraw(w, item);
        }
      i++;
      item++;
    }
}

static void
EraseAllItems(SciPlotWidget w)
{
  SciPlotItem *item;
  int i;

  item = w->plot.drawlist;
  i = 0;
  while (i < w->plot.num_drawlist)
    {
      if ((item->type > SciPlotStartTextTypes) &&
          (item->type < SciPlotEndTextTypes))
#if DEBUG_SCIPLOT_MLK
        SciPlotPrintf("EraseAllItems: item->kind.text.text=%p\n",
                      item->kind.text.text);
#endif
      XtFree(item->kind.text.text);
      i++;
      item++;
    }
  w->plot.num_drawlist = 0;
}

static void
EraseAll(SciPlotWidget w)
{
  EraseAllItems(w);
  if (XtIsRealized((Widget)w))
    XClearWindow(XtDisplay(w), XtWindow(w));
}

static SciPlotItem *
ItemGetNew(SciPlotWidget w)
{
  SciPlotItem *item;

  w->plot.num_drawlist++;
  if (w->plot.num_drawlist >= w->plot.alloc_drawlist)
    {
      w->plot.alloc_drawlist += NUMPLOTITEMEXTRA;
      w->plot.drawlist = (SciPlotItem *)XtRealloc((char *)w->plot.drawlist,
                                                  w->plot.alloc_drawlist * sizeof(SciPlotItem));
      if (!w->plot.drawlist)
        {
          SciPlotPrintf("Can't realloc memory for SciPlotItem list\n");
          exit(1);
        }
#if DEBUG_SCIPLOT
      SciPlotPrintf("Alloced #%d for drawlist\n", w->plot.alloc_drawlist);
#endif
    }
  item = w->plot.drawlist + (w->plot.num_drawlist - 1);
  item->type = SciPlotFALSE;
  item->drawing_class = w->plot.current_id;
  return item;
}

static void
LineSet(SciPlotWidget w, real x1, real y1, real x2, real y2,
        int color, int style)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short)color;
  item->kind.any.style = (short)style;
  item->kind.line.x1 = (real)x1;
  item->kind.line.y1 = (real)y1;
  item->kind.line.x2 = (real)x2;
  item->kind.line.y2 = (real)y2;
  item->type = SciPlotLine;
  ItemDraw(w, item);
}

static void
RectSet(SciPlotWidget w, real x1, real y1, real x2, real y2,
        int color, int style)
{
  SciPlotItem *item;
  real x, y, width, height;

  if (x1 < x2)
    x = x1, width = (x2 - x1 + 1);
  else
    x = x2, width = (x1 - x2 + 1);
  if (y1 < y2)
    y = y1, height = (y2 - y1 + 1);
  else
    y = y2, height = (y1 - y2 + 1);

  item = ItemGetNew(w);
  item->kind.any.color = (short)color;
  item->kind.any.style = (short)style;
  item->kind.rect.x = (real)x;
  item->kind.rect.y = (real)y;
  item->kind.rect.w = (real)width;
  item->kind.rect.h = (real)height;
  item->type = SciPlotRect;
  ItemDraw(w, item);
}

static void
FilledRectSet(SciPlotWidget w, real x1, real y1, real x2, real y2, int color, int style)
{
  SciPlotItem *item;
  real x, y, width, height;

  if (x1 < x2)
    x = x1, width = (x2 - x1 + 1);
  else
    x = x2, width = (x1 - x2 + 1);
  if (y1 < y2)
    y = y1, height = (y2 - y1 + 1);
  else
    y = y2, height = (y1 - y2 + 1);

  item = ItemGetNew(w);
  item->kind.any.color = (short)color;
  item->kind.any.style = (short)style;
  item->kind.rect.x = (real)x;
  item->kind.rect.y = (real)y;
  item->kind.rect.w = (real)width;
  item->kind.rect.h = (real)height;
  item->type = SciPlotFRect;
  ItemDraw(w, item);
}

static void
TriSet(SciPlotWidget w, real x1, real y1, real x2, real y2, real x3, real y3, int color, int style)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short)color;
  item->kind.any.style = (short)style;
  item->kind.poly.count = 3;
  item->kind.poly.x[0] = (real)x1;
  item->kind.poly.y[0] = (real)y1;
  item->kind.poly.x[1] = (real)x2;
  item->kind.poly.y[1] = (real)y2;
  item->kind.poly.x[2] = (real)x3;
  item->kind.poly.y[2] = (real)y3;
  item->type = SciPlotPoly;
  ItemDraw(w, item);
}

static void
FilledTriSet(SciPlotWidget w, real x1, real y1, real x2, real y2, real x3, real y3, int color, int style)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short)color;
  item->kind.any.style = (short)style;
  item->kind.poly.count = 3;
  item->kind.poly.x[0] = (real)x1;
  item->kind.poly.y[0] = (real)y1;
  item->kind.poly.x[1] = (real)x2;
  item->kind.poly.y[1] = (real)y2;
  item->kind.poly.x[2] = (real)x3;
  item->kind.poly.y[2] = (real)y3;
  item->type = SciPlotFPoly;
  ItemDraw(w, item);
}

static void
QuadSet(SciPlotWidget w, real x1, real y1, real x2, real y2, real x3, real y3, real x4, real y4, int color, int style)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short)color;
  item->kind.any.style = (short)style;
  item->kind.poly.count = 4;
  item->kind.poly.x[0] = (real)x1;
  item->kind.poly.y[0] = (real)y1;
  item->kind.poly.x[1] = (real)x2;
  item->kind.poly.y[1] = (real)y2;
  item->kind.poly.x[2] = (real)x3;
  item->kind.poly.y[2] = (real)y3;
  item->kind.poly.x[3] = (real)x4;
  item->kind.poly.y[3] = (real)y4;
  item->type = SciPlotPoly;
  ItemDraw(w, item);
}

static void
FilledQuadSet(SciPlotWidget w, real x1, real y1, real x2, real y2, real x3, real y3, real x4, real y4, int color, int style)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short)color;
  item->kind.any.style = (short)style;
  item->kind.poly.count = 4;
  item->kind.poly.x[0] = (real)x1;
  item->kind.poly.y[0] = (real)y1;
  item->kind.poly.x[1] = (real)x2;
  item->kind.poly.y[1] = (real)y2;
  item->kind.poly.x[2] = (real)x3;
  item->kind.poly.y[2] = (real)y3;
  item->kind.poly.x[3] = (real)x4;
  item->kind.poly.y[3] = (real)y4;
  item->type = SciPlotFPoly;
  ItemDraw(w, item);
}

static void
CircleSet(SciPlotWidget w, real x, real y, real r, int color, int style)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short)color;
  item->kind.any.style = (short)style;
  item->kind.circ.x = (real)x;
  item->kind.circ.y = (real)y;
  item->kind.circ.r = (real)r;
  item->type = SciPlotCircle;
  ItemDraw(w, item);
}

static void
FilledCircleSet(SciPlotWidget w, real x, real y, real r, int color, int style)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short)color;
  item->kind.any.style = (short)style;
  item->kind.circ.x = (real)x;
  item->kind.circ.y = (real)y;
  item->kind.circ.r = (real)r;
  item->type = SciPlotFCircle;
  ItemDraw(w, item);
}

static void
TextSet(SciPlotWidget w, real x, real y, char *text, int color, int font)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short)color;
  item->kind.any.style = 0;
  item->kind.text.x = (real)x;
  item->kind.text.y = (real)y;
  item->kind.text.length = strlen(text);
  item->kind.text.text = XtMalloc((int)item->kind.text.length + 1);
#if DEBUG_SCIPLOT_MLK
  SciPlotPrintf("TextSet: item->kind.text.text=%p\n", item->kind.text.text);
#endif
  item->kind.text.font = font;
  strcpy(item->kind.text.text, text);
  item->type = SciPlotText;
  ItemDraw(w, item);
#if DEBUG_SCIPLOT_TEXT
  if (1)
    {
      real x1, y1;

      y -= FontnumAscent(w, font);
      y1 = y + FontnumHeight(w, font) - 1.0;
      x1 = x + FontnumTextWidth(w, font, text) - 1.0;
      RectSet(w, x, y, x1, y1, color, XtLINE_SOLID);
    }
#endif
}

static void
TextCenter(SciPlotWidget w, real x, real y, char *text, int color, int font)
{
  x -= FontnumTextWidth(w, font, text) / 2.0;
  y += FontnumHeight(w, font) / 2.0 - FontnumDescent(w, font);
  TextSet(w, x, y, text, color, font);
}

static void
VTextSet(SciPlotWidget w, real x, real y, char *text, int color, int font)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short)color;
  item->kind.any.style = 0;
  item->kind.text.x = (real)x;
  item->kind.text.y = (real)y;
  item->kind.text.length = strlen(text);
  item->kind.text.text = XtMalloc((int)item->kind.text.length + 1);
#if DEBUG_SCIPLOT_MLK
  SciPlotPrintf("VTextSet: item->kind.text.text=%p\n", item->kind.text.text);
#endif
  item->kind.text.font = font;
  strcpy(item->kind.text.text, text);
  item->type = SciPlotVText;
  ItemDraw(w, item);
#if DEBUG_SCIPLOT_TEXT
  if (1)
    {
      real x1, y1;

      x += FontnumDescent(w, font);
      x1 = x - FontnumHeight(w, font) - 1.0;
      y1 = y - FontnumTextWidth(w, font, text) - 1.0;
      RectSet(w, x, y, x1, y1, color, XtLINE_SOLID);
    }
#endif
}

static void
VTextCenter(SciPlotWidget w, real x, real y, char *text, int color, int font)
{
  x += FontnumHeight(w, font) / 2.0 - FontnumDescent(w, font);
  y += FontnumTextWidth(w, font, text) / 2.0;
  VTextSet(w, x, y, text, color, font);
}

static void
ClipSet(SciPlotWidget w)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.style = XtLINE_SOLID;
  item->kind.any.color = 1;
  item->kind.line.x1 = w->plot.x.Origin;
  item->kind.line.x2 = w->plot.x.Size;
  item->kind.line.y1 = w->plot.y.Origin;
  item->kind.line.y2 = w->plot.y.Size;
#if DEBUG_SCIPLOT
  SciPlotPrintf("clipping region: x=%f y=%f w=%f h=%f\n",
                item->kind.line.x1,
                item->kind.line.y1,
                item->kind.line.x2,
                item->kind.line.y2);
#endif
  item->type = SciPlotClipRegion;
  ItemDraw(w, item);
}

static void
ClipClear(SciPlotWidget w)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.style = XtLINE_SOLID;
  item->kind.any.color = 1;
  item->type = SciPlotClipClear;
  ItemDraw(w, item);
}

/*
 * Private data point to screen location converters
 */

static real
PlotX(SciPlotWidget w, real xin)
{
  real xout;

  if (w->plot.XLog)
    xout = w->plot.x.Origin +
      ((log10(xin) - log10(w->plot.x.DrawOrigin)) *
       (w->plot.x.Size / w->plot.x.DrawSize));
  else
    xout = w->plot.x.Origin +
      ((xin - w->plot.x.DrawOrigin) *
       (w->plot.x.Size / w->plot.x.DrawSize));
  return xout;
}

static real
PlotY(SciPlotWidget w, real yin, int axis)
{
  real yout, drawOrigin = 0, drawSize = 0;
  Boolean log = 0;
  switch (axis)
    {
    case CP_Y:
      drawOrigin = w->plot.y.DrawOrigin;
      drawSize = w->plot.y.DrawSize;
      log = w->plot.YLog;
      break;
    case CP_Y2:
      drawOrigin = w->plot.y2.DrawOrigin;
      drawSize = w->plot.y2.DrawSize;
      log = w->plot.Y2Log;
      break;
    case CP_Y3:
      drawOrigin = w->plot.y3.DrawOrigin;
      drawSize = w->plot.y3.DrawSize;
      log = w->plot.Y3Log;
      break;
    case CP_Y4:
      drawOrigin = w->plot.y4.DrawOrigin;
      drawSize = w->plot.y4.DrawSize;
      log = w->plot.Y4Log;
      break;
    }
  if (log)
    yout = w->plot.y.Origin + w->plot.y.Size - ((log10(yin) - log10(drawOrigin)) * (w->plot.y.Size / drawSize));
  else
    yout = w->plot.y.Origin + w->plot.y.Size - ((yin - drawOrigin) * (w->plot.y.Size / drawSize));

  return yout;
}

/*
 * Private calculation utilities for axes
 * More spacing needed for horizontal x axis labels than 
 horizontal y axis labels */
/* #define MAX_MAJOR 8 */
#define MAX_MAJOR_X 6
#define MAX_MAJOR_Y 8
#define NUMBER_MINOR 8
static real CAdeltas[8] =
  {0.1, 0.2, 0.25, 0.5, 1.0, 2.0, 2.5, 5.0};
static int CAdecimals[8] =
  {0, 0, 1, 0, 0, 0, 1, 0};
static int CAminors[8] =
  {4, 4, 4, 5, 4, 4, 4, 5};

static void
ComputeAxis(SciPlotAxis *axis, real min, real max, Boolean log, Boolean timeaxis, int type, int max_major)
{
  real range, rel_range, rnorm, min_mag, max_mag, mag, delta, delta_order, calcmin, calcmax;
  int i, nexp, minornum = 0, majornum, majordecimals = 0, prec = 0;
  real deltaN, calcminN, calcmaxN;
  int majornumN;
  /* MSE: reworked to fix problems observed when an actual range is
     not (yet) defined by the data set. */
  if (timeaxis)
    { //Try to get the x-axis to start exactly on the second so the label is in the right spot.
      min = floor(min);
      max = ceil(max);
    }
  min_mag = fabs(min);
  max_mag = fabs(max);
  mag = (max_mag > min_mag) ? max_mag : min_mag;
  range = max - min;
  rel_range = (mag > 0.) ? range / mag : range;

#if DEBUG_SCIPLOT || DEBUG_SCIPLOT_AXIS
  SciPlotPrintf("\nComputeAxis: min=%f max=%f\n", min, max);
#endif

  if (log)
    {
      if (rel_range < DBL_EPSILON)
        {
          calcmin = (min > 0.) ? powi(10.0, (int)floor(log10(min))) : 1;
          calcmax = 10.0 * calcmin;
        }
      else
        {
          calcmin = powi(10.0, (int)floor(log10(min)));
          calcmax = powi(10.0, (int)ceil(log10(max)));
        }

      delta = 10.0;

      if (type == COMPUTE_MIN_MAX)
        {
          axis->DrawOrigin = calcmin;
          axis->DrawMax = calcmax;
        }
      else if (type == COMPUTE_MIN_ONLY)
        {
          axis->DrawOrigin = calcmin;
          calcmax = axis->DrawMax;
        }
      else if (type == COMPUTE_MAX_ONLY)
        {
          calcmin = axis->DrawOrigin;
          axis->DrawMax = calcmax;
        }
      else if (type == NO_COMPUTE_MIN_MAX)
        {
          //axis->DrawOrigin = calcmin;
          //axis->DrawMax = calcmax;
          calcmin = axis->DrawOrigin;
          calcmax = axis->DrawMax;
        }

      axis->DrawSize = log10(calcmax) - log10(calcmin);
      axis->MajorInc = delta;
      axis->MajorNum = (int)log10(calcmax / calcmin * 1.0001);
      axis->MinorNum = 10;
      axis->Precision = -(int)(log10(calcmin) * 1.0001);
#if DEBUG_SCIPLOT || DEBUG_SCIPLOT_AXIS
      SciPlotPrintf("calcmin=%e log=%e (int)log=%d  Precision=%d\n",
                    calcmin, log10(calcmin), (int)(log10(calcmin) * 1.0001), axis->Precision);
#endif
      if (axis->Precision < 0)
        axis->Precision = 0;
    }
  else
    {
      if (rel_range < DBL_EPSILON)
        {
          /* For min == max == 0, use a +/-.5 range (common when no data yet).
             For nonzero min == max, use a range of +/-4% of max magnitude. */
          real half_range = (mag > 0.) ? mag * .02 : .5;
          range = 2. * half_range;
          min -= half_range;
          max += half_range;
        }
      nexp = (int)floor(log10(range));
      rnorm = range / powi(10.0, nexp);
      i = 0;
      while (i < NUMBER_MINOR)
        {
          delta = CAdeltas[i];
          majornum = (int)ceil(rnorm / delta);
          if (majornum <= max_major)
            {
              majordecimals = CAdecimals[i];
              minornum = CAminors[i];
              i++;
              break;
            }
          i++;
        }
      delta *= pow(10.0, nexp);
#if DEBUG_SCIPLOT || DEBUG_SCIPLOT_AXIS
      SciPlotPrintf("nexp=%d range=%f rnorm=%f delta=%f\n", nexp, range, rnorm, delta);
#endif

      /* MSE: simplified formulas. */
      if (timeaxis)
        {
          calcmax = max;
          calcmin = min;
        }
      else
        {
          calcmax = ceil(max / delta) * delta;
          calcmin = floor(min / delta) * delta;
        }

      //Check next CAdelta to see if it is a better fit
      majornumN = (int)ceil(rnorm / CAdeltas[i]);
      if (majornumN > 2)
        {
          deltaN = CAdeltas[i] * pow(10.0, nexp);
          calcmaxN = ceil(max / deltaN) * deltaN;
          calcminN = floor(min / deltaN) * deltaN;
          if ((calcmax - calcmin) > (calcmaxN - calcminN))
            {
              delta = deltaN;
              majornum = majornumN;
              majordecimals = CAdecimals[i];
              minornum = CAminors[i];
              if (!timeaxis)
                {
                  calcmax = calcmaxN;
                  calcmin = calcminN;
                }
            }
        }

      if (type == COMPUTE_MIN_MAX)
        {
          axis->DrawOrigin = calcmin;
          axis->DrawMax = calcmax;
        }
      else if (type == COMPUTE_MIN_ONLY)
        {
          axis->DrawOrigin = calcmin;
          calcmax = axis->DrawMax;
        }
      else if (type == COMPUTE_MAX_ONLY)
        {
          calcmin = axis->DrawOrigin;
          axis->DrawMax = calcmax;
        }
      else if (type == NO_COMPUTE_MIN_MAX)
        {
          calcmin = min;
          calcmax = max;
          axis->DrawOrigin = calcmin;
          axis->DrawMax = calcmax;
        }

      /* NOTE: round(x) =~ floor(x+.5) =~ ceil(x-.5) where floor()/ceil() are
         portable and round()/rint()/trunc()/nearbyint() are not. */
      axis->DrawSize = calcmax - calcmin;
      axis->MajorNum = (int)floor((calcmax - calcmin) / delta + .5);

      axis->MinorNum = minornum;
      axis->MajorInc = delta;

      /* This code assumes %f floating point output since the magnitude of the
         values (delta) masks majordecimals (taken from the CAdecimals
         incrementing strategies) in calculating the output precision.  A better
         approach would be to calculate #decimal difference between increment and
         max value (to properly capture precision required when Inc << Total, e.g.
         a range of 1000000 to 1000005) plus majordecimals (additional precision
         required for specific increment patterns, e.g. 1000001.25, 1000002.5,
         1000003.75, 1000005). */
      delta_order = log10(delta);
      prec = (delta_order > 0.0) ? -(int)floor(delta_order) + majordecimals
        : (int)ceil(-delta_order) + majordecimals;
      axis->Precision = (prec > 0) ? prec : 0;
    }

  //axis->DrawOrigin = calcmin;
  //axis->DrawMax = calcmax;
  //axis->MajorInc = delta;
  //axis->Precision = (prec > 0) ? prec : 0;

#if DEBUG_SCIPLOT
  SciPlotPrintf("Tics: min=%f max=%f size=%f major inc=%f #major=%d #minor=%d prec=%d\n",
                axis->DrawOrigin, axis->DrawMax, axis->DrawSize, axis->MajorInc,
                axis->MajorNum, axis->MinorNum, axis->Precision);
#endif
}

static void
ComputeDrawingRange(SciPlotWidget w, int type)
{
  /* when we are dragging along one direction, we do not need to caculate the drawing */
  /* range of the other axis                                                          */
  if (w->plot.drag_state == NOT_DRAGGING || w->plot.drag_state == DRAGGING_NOTHING)
    {
      ComputeAxis(&w->plot.x, w->plot.Min.x, w->plot.Max.x,
                  w->plot.XLog, w->plot.XTime, type, MAX_MAJOR_X);
      ComputeAxis(&w->plot.y, w->plot.Min.y, w->plot.Max.y,
                  w->plot.YLog, FALSE, type, MAX_MAJOR_Y);
      if (w->plot.Y2AxisShow)
        {
          ComputeAxis(&w->plot.y2, w->plot.Min.y2, w->plot.Max.y2,
                      w->plot.Y2Log, FALSE, type, MAX_MAJOR_Y);
        }
      if (w->plot.Y3AxisShow)
        {
          ComputeAxis(&w->plot.y3, w->plot.Min.y3, w->plot.Max.y3,
                      w->plot.Y3Log, FALSE, type, MAX_MAJOR_Y);
        }
      if (w->plot.Y4AxisShow)
        {
          ComputeAxis(&w->plot.y4, w->plot.Min.y4, w->plot.Max.y4,
                      w->plot.Y4Log, FALSE, type, MAX_MAJOR_Y);
        }
    }
  else if (w->plot.drag_state == DRAGGING_LEFT || w->plot.drag_state == DRAGGING_RIGHT ||
           w->plot.drag_state == DRAGGING_BOTTOM_AND_LEFT ||
           w->plot.drag_state == DRAGGING_DATA)
    ComputeAxis(&w->plot.x, w->plot.Min.x, w->plot.Max.x,
                w->plot.XLog, w->plot.XTime, type, MAX_MAJOR_X);
  else if (w->plot.drag_state == DRAGGING_TOP || w->plot.drag_state == DRAGGING_BOTTOM)
    {
      ComputeAxis(&w->plot.y, w->plot.Min.y, w->plot.Max.y,
                  w->plot.YLog, FALSE, type, MAX_MAJOR_Y);
      if (w->plot.Y2AxisShow)
        {
          ComputeAxis(&w->plot.y2, w->plot.Min.y2, w->plot.Max.y2,
                      w->plot.Y2Log, FALSE, type, MAX_MAJOR_Y);
        }
      if (w->plot.Y3AxisShow)
        {
          ComputeAxis(&w->plot.y3, w->plot.Min.y3, w->plot.Max.y3,
                      w->plot.Y3Log, FALSE, type, MAX_MAJOR_Y);
        }
      if (w->plot.Y4AxisShow)
        {
          ComputeAxis(&w->plot.y4, w->plot.Min.y4, w->plot.Max.y4,
                      w->plot.Y4Log, FALSE, type, MAX_MAJOR_Y);
        }
    }
}

static Boolean
CheckMinMax(SciPlotWidget w, int type)
{
  register int i, j;
  register SciPlotList *p;
  register real val;

  if (type == COMPUTE_MIN_MAX)
    {
      for (i = 0; i < w->plot.num_plotlist; i++)
        {
          p = w->plot.plotlist + i;
          if (p->draw)
            {
              for (j = 0; j < p->number; j++)
                {

                  /* Don't count the "break in line segment" flag for Min/Max */
                  if (p->data[j].x > SCIPLOT_SKIP_VAL &&
                      p->data[j].y > SCIPLOT_SKIP_VAL)
                    {

                      val = p->data[j].x;
                      if (val > w->plot.x.DrawMax || val < w->plot.x.DrawOrigin)
                        return True;
                      val = p->data[j].y;
                      switch (p->axis)
                        {
                        case CP_Y:
                          if (val > w->plot.y.DrawMax || val < w->plot.y.DrawOrigin)
                            return True;
                          break;
                        case CP_Y2:
                          if (val > w->plot.y2.DrawMax || val < w->plot.y2.DrawOrigin)
                            return True;
                          break;
                        case CP_Y3:
                          if (val > w->plot.y3.DrawMax || val < w->plot.y3.DrawOrigin)
                            return True;
                          break;
                        case CP_Y4:
                          if (val > w->plot.y4.DrawMax || val < w->plot.y4.DrawOrigin)
                            return True;
                          break;
                        }
                    }
                }
            }
        }
    }
  return False;
}

static void
ComputeMinMax(SciPlotWidget w)
{
  register int i, j;
  register SciPlotList *p;
  register real val;
  Boolean firstx = True, firsty = True, firsty2 = True, firsty3 = True, firsty4 = True;

  /* w->plot.Min.x = w->plot.Min.y = w->plot.Max.x = w->plot.Max.y = 1.0;*/

  /* MSE: these defaults are for the case of no plot data (p->number == 0 for 
     all drawn lists), but they are not seen in current DAKOTA use since the
     y min/max marker lists always have 1 data point each (exception: markers
     with zero value are ignored if log scale is used). */
  w->plot.Max.x = w->plot.Max.y = w->plot.Max.y2 = w->plot.Max.y3 = w->plot.Max.y4 = 10.0;
  w->plot.Min.x = (w->plot.XLog) ? 1.0 : 0.0;
  w->plot.Min.y = (w->plot.YLog) ? 1.0 : -10.0;
  w->plot.Min.y2 = (w->plot.Y2Log) ? 1.0 : -10.0;
  w->plot.Min.y3 = (w->plot.Y3Log) ? 1.0 : -10.0;
  w->plot.Min.y4 = (w->plot.Y4Log) ? 1.0 : -10.0;

  for (i = 0; i < w->plot.num_plotlist; i++)
    {
      p = w->plot.plotlist + i;
      if (p->draw)
        { /* all drawn lists (xy plot data and y min/max markers) */
          for (j = 0; j < p->number; j++)
            {

              /* Don't count the "break in line segment" flag for Min/Max */
              if (p->data[j].x > SCIPLOT_SKIP_VAL &&
                  p->data[j].y > SCIPLOT_SKIP_VAL)
                {

                  val = p->data[j].x;
                  if (!w->plot.XLog || (w->plot.XLog && (val > 0.0)))
                    {
                      if (firstx)
                        {
                          w->plot.Min.x = w->plot.Max.x = val;
                          firstx = False;
                        }
                      else
                        {
                          if (val > w->plot.Max.x)
                            w->plot.Max.x = val;
                          else if (val < w->plot.Min.x)
                            w->plot.Min.x = val;
                        }
                    }

                  val = p->data[j].y;
                  if (p->axis == CP_Y)
                    {
                      if (!w->plot.YLog || (w->plot.YLog && (val > 0.0)))
                        {
                          if (firsty)
                            {
                              w->plot.Min.y = w->plot.Max.y = val;
                              firsty = False;
                            }
                          else
                            {
                              if (val > w->plot.Max.y)
                                w->plot.Max.y = val;
                              else if (val < w->plot.Min.y)
                                w->plot.Min.y = val;
                            }
                        }
                    }
                  else if (p->axis == CP_Y2)
                    {
                      if (!w->plot.Y2Log || (w->plot.Y2Log && (val > 0.0)))
                        {
                          if (firsty2)
                            {
                              w->plot.Min.y2 = w->plot.Max.y2 = val;
                              firsty2 = False;
                            }
                          else
                            {
                              if (val > w->plot.Max.y2)
                                w->plot.Max.y2 = val;
                              else if (val < w->plot.Min.y2)
                                w->plot.Min.y2 = val;
                            }
                        }
                    }
                  else if (p->axis == CP_Y3)
                    {
                      if (!w->plot.Y3Log || (w->plot.Y3Log && (val > 0.0)))
                        {
                          if (firsty3)
                            {
                              w->plot.Min.y3 = w->plot.Max.y3 = val;
                              firsty3 = False;
                            }
                          else
                            {
                              if (val > w->plot.Max.y3)
                                w->plot.Max.y3 = val;
                              else if (val < w->plot.Min.y3)
                                w->plot.Min.y3 = val;
                            }
                        }
                    }
                  else if (p->axis == CP_Y4)
                    {
                      if (!w->plot.Y4Log || (w->plot.Y4Log && (val > 0.0)))
                        {
                          if (firsty4)
                            {
                              w->plot.Min.y4 = w->plot.Max.y4 = val;
                              firsty4 = False;
                            }
                          else
                            {
                              if (val > w->plot.Max.y4)
                                w->plot.Max.y4 = val;
                              else if (val < w->plot.Min.y4)
                                w->plot.Min.y4 = val;
                            }
                        }
                    }
                }
            }
        }
    }
  /* if (!w->plot.XLog) { MSE: allow range restrictions in log scale */
  if (!w->plot.XAutoScale)
    {
      w->plot.Min.x = w->plot.UserMin.x;
      w->plot.Max.x = w->plot.UserMax.x;
    }
  else if (!w->plot.XLog && w->plot.XOrigin)
    { /* MSE: add log check here */
      if (w->plot.Min.x > 0.0)
        w->plot.Min.x = 0.0;
      if (w->plot.Max.x < 0.0)
        w->plot.Max.x = 0.0;
    }

  /* if (!w->plot.YLog) { MSE: allow range restrictions in log scale */
  if (!w->plot.YAutoScale)
    { /* user override of auto-scaling */
      w->plot.Min.y = w->plot.UserMin.y;
      w->plot.Max.y = w->plot.UserMax.y;
    }
  else if (w->plot.YLog && w->plot.YOrigin)
    { /* MSE: add log check here */
      if (w->plot.Min.y > 0.0)
        w->plot.Min.y = 0.0;
      if (w->plot.Max.y < 0.0)
        w->plot.Max.y = 0.0;
    }
  if (!w->plot.Y2AutoScale)
    { /* user override of auto-scaling */
      w->plot.Min.y2 = w->plot.UserMin.y2;
      w->plot.Max.y2 = w->plot.UserMax.y2;
    }
  else if (w->plot.Y2Log && w->plot.Y2Origin)
    { /* MSE: add log check here */
      if (w->plot.Min.y2 > 0.0)
        w->plot.Min.y2 = 0.0;
      if (w->plot.Max.y2 < 0.0)
        w->plot.Max.y2 = 0.0;
    }
  if (!w->plot.Y3AutoScale)
    { /* user override of auto-scaling */
      w->plot.Min.y3 = w->plot.UserMin.y3;
      w->plot.Max.y3 = w->plot.UserMax.y3;
    }
  else if (w->plot.Y3Log && w->plot.Y3Origin)
    { /* MSE: add log check here */
      if (w->plot.Min.y3 > 0.0)
        w->plot.Min.y3 = 0.0;
      if (w->plot.Max.y3 < 0.0)
        w->plot.Max.y3 = 0.0;
    }
  if (!w->plot.Y4AutoScale)
    { /* user override of auto-scaling */
      w->plot.Min.y4 = w->plot.UserMin.y4;
      w->plot.Max.y4 = w->plot.UserMax.y4;
    }
  else if (w->plot.Y4Log && w->plot.Y4Origin)
    { /* MSE: add log check here */
      if (w->plot.Min.y4 > 0.0)
        w->plot.Min.y4 = 0.0;
      if (w->plot.Max.y4 < 0.0)
        w->plot.Max.y4 = 0.0;
    }

#if DEBUG_SCIPLOT
  SciPlotPrintf("Min: (%f,%f)\tMax: (%f,%f)\n",
                w->plot.Min.x, w->plot.Min.y,
                w->plot.Max.x, w->plot.Max.y);
#endif
}

static void
ComputeDimensions(SciPlotWidget w)
{
  real xextra, yextra, val, labelval, label_width, max_label_width=0, max_label_width2 = 0, max_label_width3 = 0, max_label_width4 = 0,
    y_major_incr, x, y, width, height, axisnumbersize, axisXlabelsize,
    axisYlabelsize;
  char label[16]; /* , numberformat[16]; */
  int i, num_y_major, xmargin, ymargin;
  Boolean AxisOnLeft = FALSE;

  /* Compute xextra and yextra, which are the extra distances that the text
   * labels on the axes stick outside of the graph.
   */
  xextra = yextra = 0.;
  if (w->plot.XTime)
    {
      sprintf(label, "08:00:00");
    }
  else
    {
      val = w->plot.x.DrawMax;
      sprintf(label, "%.6g", val);
    }
  //Half the Xmax number on the xaxis will go beyond the xaxis. xextra is that size.
  //xextra = ceil(PlotX(w, val) + FontnumTextWidth(w, w->plot.axisFont, label) / 2.) + (real)w->plot.Margin - (real)w->core.width;
  xextra = FontnumTextWidth(w, w->plot.axisFont, label) / 2.;
  if (xextra < 0.)
    xextra = 0.;
  //Half the Ymax number on the yaxis will go beyond the yaxis. yextra is that size.
  yextra = FontnumHeight(w, w->plot.axisFont) / 2.0;

  /* loop over each increment and save max label width */
  if (w->plot.YAxisShow)
    {
      max_label_width = 0.0;
      num_y_major = w->plot.y.MajorNum;
      y_major_incr = w->plot.y.MajorInc;
      val = w->plot.y.DrawOrigin;
      for (i = 0; i <= num_y_major; i++)
        { /* both ends */
          if (i)
            val = (w->plot.YLog) ? val * y_major_incr : val + y_major_incr;
          /* Make sure label is not trying to resolve more precision
             than that indicated by MajorInc (especially at y = 0.) */
          labelval = (w->plot.YLog) ? val : floor(val / y_major_incr + .5) * y_major_incr;
          if (fabs(labelval) < DBL_MIN)
            labelval = 0.; /* hack to remove "-0" */
          sprintf(label, "%.6g", labelval);
          label_width = FontnumTextWidth(w, w->plot.axisFont, label);
          if (label_width > max_label_width)
            max_label_width = label_width;
        }
    }
  if (w->plot.Y2AxisShow)
    {
      max_label_width2 = 0.0;
      num_y_major = w->plot.y2.MajorNum;
      y_major_incr = w->plot.y2.MajorInc;
      val = w->plot.y2.DrawOrigin;
      for (i = 0; i <= num_y_major; i++)
        { /* both ends */
          if (i)
            val = (w->plot.Y2Log) ? val * y_major_incr : val + y_major_incr;
          /* Make sure label is not trying to resolve more precision
             than that indicated by MajorInc (especially at y = 0.) */
          labelval = (w->plot.Y2Log) ? val : floor(val / y_major_incr + .5) * y_major_incr;
          if (fabs(labelval) < DBL_MIN)
            labelval = 0.; /* hack to remove "-0" */
          sprintf(label, "%.6g", labelval);
          label_width = FontnumTextWidth(w, w->plot.axisFont, label);
          if (label_width > max_label_width2)
            max_label_width2 = label_width;
        }
    }
  if (w->plot.Y3AxisShow)
    {
      max_label_width3 = 0.0;
      num_y_major = w->plot.y3.MajorNum;
      y_major_incr = w->plot.y3.MajorInc;
      val = w->plot.y3.DrawOrigin;
      for (i = 0; i <= num_y_major; i++)
        { /* both ends */
          if (i)
            val = (w->plot.Y3Log) ? val * y_major_incr : val + y_major_incr;
          /* Make sure label is not trying to resolve more precision
             than that indicated by MajorInc (especially at y = 0.) */
          labelval = (w->plot.Y3Log) ? val : floor(val / y_major_incr + .5) * y_major_incr;
          if (fabs(labelval) < DBL_MIN)
            labelval = 0.; /* hack to remove "-0" */
          sprintf(label, "%.6g", labelval);
          label_width = FontnumTextWidth(w, w->plot.axisFont, label);
          if (label_width > max_label_width3)
            max_label_width3 = label_width;
        }
    }
  if (w->plot.Y4AxisShow)
    {
      max_label_width4 = 0.0;
      num_y_major = w->plot.y4.MajorNum;
      y_major_incr = w->plot.y4.MajorInc;
      val = w->plot.y4.DrawOrigin;
      for (i = 0; i <= num_y_major; i++)
        { /* both ends */
          if (i)
            val = (w->plot.Y4Log) ? val * y_major_incr : val + y_major_incr;
          /* Make sure label is not trying to resolve more precision
             than that indicated by MajorInc (especially at y = 0.) */
          labelval = (w->plot.Y4Log) ? val : floor(val / y_major_incr + .5) * y_major_incr;
          if (fabs(labelval) < DBL_MIN)
            labelval = 0.; /* hack to remove "-0" */
          sprintf(label, "%.6g", labelval);
          label_width = FontnumTextWidth(w, w->plot.axisFont, label);
          if (label_width > max_label_width4)
            max_label_width4 = label_width;
        }
    }

#if DEBUG_SCIPLOT
  SciPlotPrintf("xextra=%f  yextra=%f\n", xextra, yextra);
#endif

  if ((w->plot.YAxisShow && w->plot.YAxisLeft) ||
      (w->plot.Y2AxisShow && w->plot.Y2AxisLeft) ||
      (w->plot.Y3AxisShow && w->plot.Y3AxisLeft) ||
      (w->plot.Y4AxisShow && w->plot.Y4AxisLeft))
    {
      AxisOnLeft = TRUE;
    }
  if (w->core.width <= 250) // tiny plot
    {
      xmargin = 0;
      ymargin = w->plot.Margin;
      xextra = 0;
    }
  else
    {
      xmargin = w->plot.Margin;
      ymargin = w->plot.Margin;
    }
  if (AxisOnLeft == TRUE)
    {
      x = 0;
    }
  else
    {
      x = (real)xmargin * 2 + xextra;
    }
  //The idea here is to match the old margins from the old xrt graph code if the .adl file has not been updated.
  if ((w->plot.medmFileVersion <= 30114) && (w->core.width > 250))
    x += (.035 * (real)w->core.width);
  y = (real)ymargin + yextra;

  /* width = (real)w->core.width - (real)xmargin - x -
   *          legendwidth - AxisFontHeight
   */
  width = (real)w->core.width - (real)xmargin - x - xextra - (real)xmargin;
  if ((w->plot.medmFileVersion <= 30114) && (w->core.width > 250))
    width -= (.032 * (real)w->core.width);

  /* height = (real)w->core.height - (real)ymargin - y
   *           - Height of axis numbers (including margin)
   *           - Height of axis label (including margin)
   *           - Height of Title (including margin)
   */
  height = (real)w->core.height - y;

  width -= w->primitive.shadow_thickness;
  height -= w->primitive.shadow_thickness;

  w->plot.x.Origin = x;
  w->plot.y.Origin = y;

  /* Adjust the size depending upon what sorts of text are visible. */
  if (w->plot.ShowTitle)
    height -= FontnumHeight(w, w->plot.titleFont);

  axisXlabelsize = 0.0;
  axisYlabelsize = 0.0;
  axisnumbersize = (real)ymargin + FontnumHeight(w, w->plot.axisFont);
  height -= axisnumbersize;

  if (w->plot.ShowXLabel)
    {
      axisXlabelsize = FontnumHeight(w, w->plot.labelFont);
      height -= axisXlabelsize;
    }
  axisYlabelsize = (real)xmargin + FontnumHeight(w, w->plot.labelFont);

  if (w->plot.Y4AxisShow)
    {
      width -= axisYlabelsize;
      if (w->plot.Y4AxisLeft)
        {
          width -= max_label_width4 + 2 * (real)xmargin;
          w->plot.x.Origin += axisYlabelsize + max_label_width4 + 2 * (real)xmargin;
          w->plot.y4.AxisPos = w->plot.x.Origin;
          w->plot.y4.LabelPos = w->plot.y4.AxisPos - max_label_width4 - (real)xmargin - (FontnumHeight(w, w->plot.labelFont) / 2.0);
        }
      else
        {
          width -= max_label_width4;
          w->plot.y4.AxisPos = w->plot.x.Origin + width;
          w->plot.y4.LabelPos = w->plot.y4.AxisPos + max_label_width4 + 3 * (real)xmargin;
          width -= 2 * (real)xmargin;
        }
    }
  if (w->plot.Y3AxisShow)
    {
      width -= axisYlabelsize;
      if (w->plot.Y3AxisLeft)
        {
          width -= max_label_width3 + 2 * (real)xmargin;
          w->plot.x.Origin += axisYlabelsize + max_label_width3 + 2 * (real)xmargin;
          w->plot.y3.AxisPos = w->plot.x.Origin;
          w->plot.y3.LabelPos = w->plot.y3.AxisPos - max_label_width3 - (real)xmargin - (FontnumHeight(w, w->plot.labelFont) / 2.0);
        }
      else
        {
          width -= max_label_width3;
          w->plot.y3.AxisPos = w->plot.x.Origin + width;
          w->plot.y3.LabelPos = w->plot.y3.AxisPos + max_label_width3 + 3 * (real)xmargin;
          width -= 2 * (real)xmargin;
        }
    }
  if (w->plot.Y2AxisShow)
    {
      width -= axisYlabelsize;
      if (w->plot.Y2AxisLeft)
        {
          width -= max_label_width2 + 2 * (real)xmargin;
          w->plot.x.Origin += axisYlabelsize + max_label_width2 + 2 * (real)xmargin;
          w->plot.y2.AxisPos = w->plot.x.Origin;
          w->plot.y2.LabelPos = w->plot.y2.AxisPos - max_label_width2 - (real)xmargin - (FontnumHeight(w, w->plot.labelFont) / 2.0);
        }
      else
        {
          width -= max_label_width2;
          w->plot.y2.AxisPos = w->plot.x.Origin + width;
          w->plot.y2.LabelPos = w->plot.y2.AxisPos + max_label_width2 + 3 * (real)xmargin;
          width -= 2 * (real)xmargin;
        }
    }
  if (w->plot.YAxisShow)
    {
      width -= axisYlabelsize;
      if (w->plot.YAxisLeft)
        {
          width -= max_label_width + 2 * (real)xmargin;
          w->plot.x.Origin += axisYlabelsize + max_label_width + 2 * (real)xmargin;
          // axisYlabelsize = (real)xmargin + FontnumHeight(w, w->plot.labelFont);
          w->plot.x.AxisPos = w->plot.x.Origin;
          w->plot.x.LabelPos = w->plot.x.AxisPos - max_label_width - (real)xmargin - (FontnumHeight(w, w->plot.labelFont) / 2.0);
        }
      else
        {
          width -= max_label_width;
          w->plot.x.AxisPos = w->plot.x.Origin + width;
          w->plot.x.LabelPos = w->plot.x.AxisPos + max_label_width + 3 * (real)xmargin;
        }
    }

  w->plot.x.Size = width;
  w->plot.y.Size = height;

  w->plot.y.AxisPos = w->plot.y.Origin + w->plot.y.Size + (real)ymargin + FontnumAscent(w, w->plot.axisFont);
  w->plot.y.LabelPos = w->plot.y.Origin + w->plot.y.Size + (real)ymargin + axisnumbersize;
  if (w->plot.ShowTitle)
    {
      w->plot.y.TitlePos = (real)w->core.height - (real)ymargin;
      w->plot.x.TitlePos = (real)xmargin;
      if (w->plot.medmFileVersion <= 30114)
        w->plot.x.TitlePos += (.035 * (real)w->core.width);
    }

#if DEBUG_SCIPLOT
  SciPlotPrintf("y.Origin:             %f\n", w->plot.y.Origin);
  SciPlotPrintf("y.Size:               %f\n", w->plot.y.Size);
  SciPlotPrintf("axisnumbersize:       %f\n", axisnumbersize);
  SciPlotPrintf("y.axisLabelSize:      %f\n", axisYlabelsize);
  SciPlotPrintf("y.TitleSize:          %f\n",
                (real)w->plot.TitleMargin + FontnumHeight(w, w->plot.titleFont));
  SciPlotPrintf("y.Margin:             %f\n", (real)ymargin);
  SciPlotPrintf("total-----------------%f\n", w->plot.y.Origin + w->plot.y.Size +
                axisnumbersize + axisYlabelsize + (real)margin +
                (real)w->plot.TitleMargin + FontnumHeight(w, w->plot.titleFont));
  SciPlotPrintf("total should be-------%f\n", (real)w->core.height);
#endif
}

static void
ComputeAll(SciPlotWidget w, int type)
{
  ComputeMinMax(w);
  ComputeDrawingRange(w, type);
  if (type == COMPUTE_MIN_MAX)
    ComputeDimensions(w);
}

/*
 * Private drawing routines
 */

static void
DrawMarker(SciPlotWidget w, real xpaper, real ypaper, real size,
           int color, int style)
{
  real sizex, sizey;

  switch (style)
    {
    case XtMARKER_CIRCLE:
      CircleSet(w, xpaper, ypaper, size, color, XtLINE_SOLID);
      break;
    case XtMARKER_FCIRCLE:
      FilledCircleSet(w, xpaper, ypaper, size, color, XtLINE_SOLID);
      break;
    case XtMARKER_SQUARE:
      size -= .5;
      RectSet(w, xpaper - size, ypaper - size,
              xpaper + size, ypaper + size,
              color, XtLINE_SOLID);
      break;
    case XtMARKER_FSQUARE:
      size -= .5;
      FilledRectSet(w, xpaper - size, ypaper - size,
                    xpaper + size, ypaper + size,
                    color, XtLINE_SOLID);
      break;
    case XtMARKER_UTRIANGLE:
      sizex = size * .866;
      sizey = size / 2.0;
      TriSet(w, xpaper, ypaper - size,
             xpaper + sizex, ypaper + sizey,
             xpaper - sizex, ypaper + sizey,
             color, XtLINE_SOLID);
      break;
    case XtMARKER_FUTRIANGLE:
      sizex = size * .866;
      sizey = size / 2.0;
      FilledTriSet(w, xpaper, ypaper - size,
                   xpaper + sizex, ypaper + sizey,
                   xpaper - sizex, ypaper + sizey,
                   color, XtLINE_SOLID);
      break;
    case XtMARKER_DTRIANGLE:
      sizex = size * .866;
      sizey = size / 2.0;
      TriSet(w, xpaper, ypaper + size,
             xpaper + sizex, ypaper - sizey,
             xpaper - sizex, ypaper - sizey,
             color, XtLINE_SOLID);
      break;
    case XtMARKER_FDTRIANGLE:
      sizex = size * .866;
      sizey = size / 2.0;
      FilledTriSet(w, xpaper, ypaper + size,
                   xpaper + sizex, ypaper - sizey,
                   xpaper - sizex, ypaper - sizey,
                   color, XtLINE_SOLID);
      break;
    case XtMARKER_RTRIANGLE:
      sizey = size * .866;
      sizex = size / 2.0;
      TriSet(w, xpaper + size, ypaper,
             xpaper - sizex, ypaper + sizey,
             xpaper - sizex, ypaper - sizey,
             color, XtLINE_SOLID);
      break;
    case XtMARKER_FRTRIANGLE:
      sizey = size * .866;
      sizex = size / 2.0;
      FilledTriSet(w, xpaper + size, ypaper,
                   xpaper - sizex, ypaper + sizey,
                   xpaper - sizex, ypaper - sizey,
                   color, XtLINE_SOLID);
      break;
    case XtMARKER_LTRIANGLE:
      sizey = size * .866;
      sizex = size / 2.0;
      TriSet(w, xpaper - size, ypaper,
             xpaper + sizex, ypaper + sizey,
             xpaper + sizex, ypaper - sizey,
             color, XtLINE_SOLID);
      break;
    case XtMARKER_FLTRIANGLE:
      sizey = size * .866;
      sizex = size / 2.0;
      FilledTriSet(w, xpaper - size, ypaper,
                   xpaper + sizex, ypaper + sizey,
                   xpaper + sizex, ypaper - sizey,
                   color, XtLINE_SOLID);
      break;
    case XtMARKER_DIAMOND:
      QuadSet(w, xpaper, ypaper - size,
              xpaper + size, ypaper,
              xpaper, ypaper + size,
              xpaper - size, ypaper,
              color, XtLINE_SOLID);
      break;
    case XtMARKER_FDIAMOND:
      FilledQuadSet(w, xpaper, ypaper - size,
                    xpaper + size, ypaper,
                    xpaper, ypaper + size,
                    xpaper - size, ypaper,
                    color, XtLINE_SOLID);
      break;
    case XtMARKER_HOURGLASS:
      QuadSet(w, xpaper - size, ypaper - size,
              xpaper + size, ypaper - size,
              xpaper - size, ypaper + size,
              xpaper + size, ypaper + size,
              color, XtLINE_SOLID);
      break;
    case XtMARKER_FHOURGLASS:
      FilledQuadSet(w, xpaper - size, ypaper - size,
                    xpaper + size, ypaper - size,
                    xpaper - size, ypaper + size,
                    xpaper + size, ypaper + size,
                    color, XtLINE_SOLID);
      break;
    case XtMARKER_BOWTIE:
      QuadSet(w, xpaper - size, ypaper - size,
              xpaper - size, ypaper + size,
              xpaper + size, ypaper - size,
              xpaper + size, ypaper + size,
              color, XtLINE_SOLID);
      break;
    case XtMARKER_FBOWTIE:
      FilledQuadSet(w, xpaper - size, ypaper - size,
                    xpaper - size, ypaper + size,
                    xpaper + size, ypaper - size,
                    xpaper + size, ypaper + size,
                    color, XtLINE_SOLID);
      break;
    case XtMARKER_DOT:
      FilledCircleSet(w, xpaper, ypaper, 1.5, color, XtLINE_SOLID);
      break;
    default:
      break;
    }
}

static void
DrawCartesianXLabelAndTitle(SciPlotWidget w)
{
  w->plot.current_id = SciPlotDrawingXLabelTitle;
  if (w->plot.ShowTitle)
    TextSet(w, w->plot.x.TitlePos, w->plot.y.TitlePos, w->plot.plotTitle,
            w->plot.ForegroundColor, w->plot.titleFont);
  if (w->plot.ShowXLabel)
    TextCenter(w, w->plot.x.Origin + w->plot.x.Size / 2.0, w->plot.y.LabelPos,
               w->plot.xlabel, w->plot.ForegroundColor, w->plot.labelFont);
}

static void
DrawCartesianYLabel(SciPlotWidget w, int axis)
{
  int i, width, j;
  SciPlotList *p;
  j = 4;
  switch (axis)
    {
    case CP_Y:
      w->plot.current_id = SciPlotDrawingYLabel;
      if (!w->plot.ShowYLabel)
        {
          width = -10;
        }
      else
        {
          VTextCenter(w, w->plot.x.LabelPos, w->plot.y.Origin + w->plot.y.Size / 2.0,
                      w->plot.ylabel, w->plot.ForegroundColor, w->plot.labelFont);
          width = FontnumTextWidth(w, w->plot.labelFont, w->plot.ylabel);
        }
      if (w->plot.Y2AxisShow || w->plot.Y3AxisShow || w->plot.Y4AxisShow)
        {
          for (i = 0; i < w->plot.num_plotlist; i++)
            {
              p = w->plot.plotlist + i;
              if (p->draw && (p->axis == axis) && (w->core.width > 250))
                {
                  LineSet(w, w->plot.x.LabelPos + j, w->plot.y.Origin, w->plot.x.LabelPos + j, w->plot.y.Origin + ((w->plot.y.Size - width) / 2.0) - 5, p->LineColor, XtLINE_SOLID);
                  LineSet(w, w->plot.x.LabelPos + j, w->plot.y.Origin + w->plot.y.Size, w->plot.x.LabelPos + j, w->plot.y.Origin + ((w->plot.y.Size + width) / 2.0) + 5, p->LineColor, XtLINE_SOLID);
                  j = j - 2;
                }
            }
        }
      break;
    case CP_Y2:
      w->plot.current_id = SciPlotDrawingY2Label;
      if (!w->plot.ShowY2Label)
        {
          width = -10;
        }
      else
        {
          VTextCenter(w, w->plot.y2.LabelPos,
                      w->plot.y.Origin + w->plot.y.Size / 2.0,
                      w->plot.y2label,
                      w->plot.ForegroundColor,
                      w->plot.labelFont);
          width = FontnumTextWidth(w, w->plot.labelFont, w->plot.y2label);
        }
      if (w->plot.YAxisShow || w->plot.Y3AxisShow || w->plot.Y4AxisShow)
        {
          for (i = 0; i < w->plot.num_plotlist; i++)
            {
              p = w->plot.plotlist + i;
              if (p->draw && (p->axis == axis) && (w->core.width > 250))
                {
                  LineSet(w, w->plot.y2.LabelPos + j, w->plot.y.Origin, w->plot.y2.LabelPos + j, w->plot.y.Origin + ((w->plot.y.Size - width) / 2.0) - 5, p->LineColor, XtLINE_SOLID);
                  LineSet(w, w->plot.y2.LabelPos + j, w->plot.y.Origin + w->plot.y.Size, w->plot.y2.LabelPos + j, w->plot.y.Origin + ((w->plot.y.Size + width) / 2.0) + 5, p->LineColor, XtLINE_SOLID);
                  j = j - 2;
                }
            }
        }
      break;
    case CP_Y3:
      w->plot.current_id = SciPlotDrawingY3Label;
      if (!w->plot.ShowY3Label)
        {
          width = -10;
        }
      else
        {
          VTextCenter(w, w->plot.y3.LabelPos,
                      w->plot.y.Origin + w->plot.y.Size / 2.0,
                      w->plot.y3label,
                      w->plot.ForegroundColor,
                      w->plot.labelFont);
          width = FontnumTextWidth(w, w->plot.labelFont, w->plot.y3label);
        }
      if (w->plot.YAxisShow || w->plot.Y2AxisShow || w->plot.Y4AxisShow)
        {
          for (i = 0; i < w->plot.num_plotlist; i++)
            {
              p = w->plot.plotlist + i;
              if (p->draw && (p->axis == axis) && (w->core.width > 250))
                {
                  LineSet(w, w->plot.y3.LabelPos + j, w->plot.y.Origin, w->plot.y3.LabelPos + j, w->plot.y.Origin + ((w->plot.y.Size - width) / 2.0) - 5, p->LineColor, XtLINE_SOLID);
                  LineSet(w, w->plot.y3.LabelPos + j, w->plot.y.Origin + w->plot.y.Size, w->plot.y3.LabelPos + j, w->plot.y.Origin + ((w->plot.y.Size + width) / 2.0) + 5, p->LineColor, XtLINE_SOLID);
                  j = j - 2;
                }
            }
        }
      break;
    case CP_Y4:
      w->plot.current_id = SciPlotDrawingY4Label;
      if (!w->plot.ShowY4Label)
        {
          width = -10;
        }
      else
        {
          VTextCenter(w, w->plot.y4.LabelPos,
                      w->plot.y.Origin + w->plot.y.Size / 2.0,
                      w->plot.y4label,
                      w->plot.ForegroundColor,
                      w->plot.labelFont);
          width = FontnumTextWidth(w, w->plot.labelFont, w->plot.y4label);
        }
      if (w->plot.YAxisShow || w->plot.Y2AxisShow || w->plot.Y3AxisShow)
        {
          for (i = 0; i < w->plot.num_plotlist; i++)
            {
              p = w->plot.plotlist + i;
              if (p->draw && (p->axis == axis) && (w->core.width > 250))
                {
                  LineSet(w, w->plot.y4.LabelPos + j, w->plot.y.Origin, w->plot.y4.LabelPos + j, w->plot.y.Origin + ((w->plot.y.Size - width) / 2.0) - 5, p->LineColor, XtLINE_SOLID);
                  LineSet(w, w->plot.y4.LabelPos + j, w->plot.y.Origin + w->plot.y.Size, w->plot.y4.LabelPos + j, w->plot.y.Origin + ((w->plot.y.Size + width) / 2.0) + 5, p->LineColor, XtLINE_SOLID);
                  j = j - 2;
                }
            }
        }
      break;
    }
}

static void
DrawCartesianXMajorAndMinor(SciPlotWidget w)
{
  real x, y0_pos, x1_pos, y1_pos, x1_val, y1_val, x_major_incr,
    minorval, majorval;
  int i, j, num_x_major;

  w->plot.current_id = SciPlotDrawingXMajorMinor;
  FontnumHeight(w, w->plot.axisFont);
  if (w->plot.XTime)
    {
      num_x_major = w->plot.x.MajorNum + 1;
      x_major_incr = ceil(w->plot.x.MajorInc);
    }
  else
    {
      num_x_major = w->plot.x.MajorNum;
      x_major_incr = w->plot.x.MajorInc;
    }
  x1_val = w->plot.x.DrawOrigin;
  y1_val = w->plot.y.DrawOrigin;
  x1_pos = PlotX(w, x1_val);
  y1_pos = PlotY(w, y1_val, CP_Y);
  /*
    y0_pos = PlotY(w, 0.0, CP_Y);
    if (!w->plot.YAxisShow)
    {
    if (w->plot.Y2AxisShow)
    {
    y0_pos = PlotY(w, 0.0, CP_Y2);
    }
    else if (w->plot.Y3AxisShow)
    {
    y0_pos = PlotY(w, 0.0, CP_Y3);
    }
    else if (w->plot.Y3AxisShow)
    {
    y0_pos = PlotY(w, 0.0, CP_Y4);
    }
    }
    if (y0_pos > y1_pos)
    {
    y0_pos = y1_pos;
    }
  */
  y0_pos = y1_pos;
  //w->plot.y.AxisPos = y0_pos + (real)w->plot.Margin + FontnumAscent(w, w->plot.axisFont);

  LineSet(w, x1_pos, y0_pos + 4, x1_pos, y0_pos - 4, w->plot.ForegroundColor,
          XtLINE_SOLID);

  majorval = x1_val;
  for (i = 0; i < num_x_major; i++)
    {
      /* minor x axis ticks and grid lines */
      if (w->plot.XLog)
        {
          for (j = 2; j < w->plot.x.MinorNum; j++)
            {
              minorval = majorval * (real)j;
              x = PlotX(w, minorval);
              LineSet(w, x, y0_pos + 2, x, y0_pos - 2, w->plot.ForegroundColor,
                      XtLINE_SOLID);
            }
          majorval *= x_major_incr;
        }
      else
        {
          for (j = 1; j < w->plot.x.MinorNum; j++)
            {
              minorval = majorval + x_major_incr * (real)j / w->plot.x.MinorNum;
              x = PlotX(w, minorval);
              if (w->plot.XTime)
                {
                  if (minorval > w->plot.x.DrawMax)
                    {
                      break;
                    }
                }
              LineSet(w, x, y0_pos + 2, x, y0_pos - 2, w->plot.ForegroundColor,
                      XtLINE_SOLID);
            }
          majorval += x_major_incr;
        }
      /* major x axis ticks and grid lines */
      x = PlotX(w, majorval);
      if (w->plot.XTime)
        {
          if (majorval > w->plot.x.DrawMax)
            {
              break;
            }
        }
      LineSet(w, x, y0_pos + 4, x, y0_pos - 4, w->plot.ForegroundColor, XtLINE_SOLID);
    }
}

static void
DrawCartesianYMajorAndMinor(SciPlotWidget w, int axis)
{
  real y, y1_pos, y1_val=0,
    y_major_incr=0, minorval, majorval, x = 0;
  int i, j, num_y_major = 0, num_y_minor = 0, log = 0, offset = 0;
  Boolean leftside = TRUE;

  switch (axis)
    {
    case CP_Y:
      num_y_minor = w->plot.y.MinorNum;
      num_y_major = w->plot.y.MajorNum;
      y_major_incr = w->plot.y.MajorInc;
      y1_val = w->plot.y.DrawOrigin;
      x = w->plot.x.AxisPos;
      log = w->plot.YLog;
      leftside = w->plot.YAxisLeft;
      break;
    case CP_Y2:
      num_y_minor = w->plot.y2.MinorNum;
      num_y_major = w->plot.y2.MajorNum;
      y_major_incr = w->plot.y2.MajorInc;
      y1_val = w->plot.y2.DrawOrigin;
      x = w->plot.y2.AxisPos;
      log = w->plot.Y2Log;
      leftside = w->plot.Y2AxisLeft;
      break;
    case CP_Y3:
      num_y_minor = w->plot.y3.MinorNum;
      num_y_major = w->plot.y3.MajorNum;
      y_major_incr = w->plot.y3.MajorInc;
      y1_val = w->plot.y3.DrawOrigin;
      x = w->plot.y3.AxisPos;
      log = w->plot.Y3Log;
      leftside = w->plot.Y3AxisLeft;
      break;
    case CP_Y4:
      num_y_minor = w->plot.y4.MinorNum;
      num_y_major = w->plot.y4.MajorNum;
      y_major_incr = w->plot.y4.MajorInc;
      y1_val = w->plot.y4.DrawOrigin;
      x = w->plot.y4.AxisPos;
      log = w->plot.Y4Log;
      leftside = w->plot.Y4AxisLeft;
      break;
    }
  y1_pos = PlotY(w, w->plot.y.DrawOrigin, CP_Y);

  LineSet(w, x + 5, y1_pos, x - 5, y1_pos, w->plot.ForegroundColor, XtLINE_SOLID);

  majorval = y1_val;
  for (i = 0; i < num_y_major; i++)
    {
      /* minor x axis ticks and grid lines */
      if (leftside)
        {
          offset = 3;
        }
      else
        {
          offset = -3;
        }
      if (log)
        {
          for (j = 2; j < num_y_minor; j++)
            {
              minorval = majorval * (real)j;
              y = PlotY(w, minorval, axis);
              LineSet(w, x, y, x + offset, y, w->plot.ForegroundColor, XtLINE_SOLID);
            }
          majorval *= y_major_incr;
        }
      else
        {
          for (j = 1; j < num_y_minor; j++)
            {
              minorval = majorval + y_major_incr * (real)j / num_y_minor;
              y = PlotY(w, minorval, axis);
              LineSet(w, x, y, x + offset, y, w->plot.ForegroundColor, XtLINE_SOLID);
            }
          majorval += y_major_incr;
        }

      /* major y axis ticks and grid lines */
      y = PlotY(w, majorval, axis);
      LineSet(w, x - 5, y, x + 5, y, w->plot.ForegroundColor, XtLINE_SOLID);
    }
}

static void
DrawCartesianXAxis(SciPlotWidget w, int type)
{
  real x, xx = 0, x1_pos, y1_pos, x2_pos, y0_pos, x1_val, y1_val, x_major_incr,
    majorval, labelval, xpos = 0;
  int i, num_x_major;
  char label[80];
  time_t tval;
  struct tm *ts;

  w->plot.current_id = SciPlotDrawingXAxis;
  FontnumHeight(w, w->plot.axisFont);
  if (w->plot.XTime)
    {
      num_x_major = w->plot.x.MajorNum + 1;
      x_major_incr = ceil(w->plot.x.MajorInc);
    }
  else
    {
      num_x_major = w->plot.x.MajorNum;
      x_major_incr = w->plot.x.MajorInc;
    }
  x1_val = w->plot.x.DrawOrigin;
  y1_val = w->plot.y.DrawOrigin;
  x1_pos = PlotX(w, x1_val);
  y1_pos = PlotY(w, y1_val, CP_Y);
  x2_pos = PlotX(w, w->plot.x.DrawMax);
  /*
    y0_pos = PlotY(w, 0.0, CP_Y);
    if (!w->plot.YAxisShow)
    {
    if (w->plot.Y2AxisShow)
    {
    y0_pos = PlotY(w, 0.0, CP_Y2);
    }
    else if (w->plot.Y3AxisShow)
    {
    y0_pos = PlotY(w, 0.0, CP_Y3);
    }
    else if (w->plot.Y3AxisShow)
    {
    y0_pos = PlotY(w, 0.0, CP_Y4);
    }
    }
    if (y0_pos > y1_pos)
    {
    y0_pos = y1_pos;
    }
  */
  y0_pos = y1_pos;
  w->plot.y.AxisPos = y0_pos + (real)w->plot.Margin + FontnumAscent(w, w->plot.axisFont);
  /* draw x axis line */
  LineSet(w, x1_pos, y0_pos, x2_pos, y0_pos, w->plot.ForegroundColor,
          XtLINE_SOLID); /* x axis line */

  if (y0_pos == y1_pos)
    {
      if (w->plot.XTime)
        {
          tval = (time_t)x1_val;
          ts = localtime(&tval);
          strftime(label, sizeof(label), w->plot.XTimeFormat, ts);
        }
      else
        {
          sprintf(label, "%.6g", x1_val);
        }
      TextSet(w, ceil(x1_pos - FontnumTextWidth(w, w->plot.axisFont, label) / 2.),
              w->plot.y.AxisPos, label, w->plot.ForegroundColor,
              w->plot.axisFont);
    }

  majorval = x1_val;
  for (i = 0; i < num_x_major; i++)
    {
      if (w->plot.XLog)
        {
          majorval *= x_major_incr;
        }
      else
        {
          majorval += x_major_incr;
        }
      /* major x axis ticks and grid lines */
      x = PlotX(w, majorval);
      if (w->plot.XTime)
        {
          if (majorval > w->plot.x.DrawMax)
            {
              break;
            }
          labelval = majorval;
          if (fabs(labelval) < DBL_MIN)
            labelval = 0.; /* hack to remove "-0" */
          tval = (time_t)labelval;
          ts = localtime(&tval);
          strftime(label, sizeof(label), w->plot.XTimeFormat, ts);
        }
      else
        {
          labelval = (w->plot.XLog) ? majorval : floor(majorval / x_major_incr + .5) * x_major_incr;
          if (fabs(labelval) < DBL_MIN)
            labelval = 0.; /* hack to remove "-0" */
          sprintf(label, "%.6g", labelval);
        }
      xx = FontnumTextWidth(w, w->plot.axisFont, label) / 2.;
      if ((y0_pos == y1_pos) || ((ceil(x + xx) < x2_pos) && (ceil(x - xx) > x1_pos)))
        {
          xpos = ceil(x - xx);
          TextSet(w, xpos,
                  w->plot.y.AxisPos, label, w->plot.ForegroundColor,
                  w->plot.axisFont);
        }
    }
  if (w->plot.XTime && (xpos > 0))
    {
      Boolean AxisOnRight = FALSE;
      if ((w->plot.YAxisShow && !w->plot.YAxisLeft) ||
          (w->plot.Y2AxisShow && !w->plot.Y2AxisLeft) ||
          (w->plot.Y3AxisShow && !w->plot.Y3AxisLeft) ||
          (w->plot.Y4AxisShow && !w->plot.Y4AxisLeft))
        {
          AxisOnRight = TRUE;
        }

      if (((xpos + xx * 2.) < (x2_pos - xx)) && (AxisOnRight == FALSE))
        {
          tval = (time_t)w->plot.x.DrawMax;
          ts = localtime(&tval);
          strftime(label, sizeof(label), w->plot.XTimeFormat, ts);
          TextSet(w, ceil(x2_pos - FontnumTextWidth(w, w->plot.axisFont, label) / 2.),
                  w->plot.y.AxisPos, label, w->plot.ForegroundColor,
                  w->plot.axisFont);
        }
    }
  if (type == DRAW_ALL)
    {
      DrawCartesianXMajorAndMinor(w);
      DrawCartesianXLabelAndTitle(w);
    }
  else if (type == DRAW_NO_LABELS)
    {
      DrawCartesianXMajorAndMinor(w);
    }
}

static void
DrawCartesianYAxis(SciPlotWidget w, int type, int axis)
{
  real y, y1_pos, y2_pos, y1_val = 0,
    y_major_incr = 1, majorval, labelval, height, x = 0;
  int i, num_y_major = 0, offset = 0, log = 0;
  char label[16];
  Boolean leftside = TRUE;

  height = FontnumHeight(w, w->plot.axisFont);
  switch (axis)
    {
    case CP_Y:
      w->plot.current_id = SciPlotDrawingYAxis;
      num_y_major = w->plot.y.MajorNum;
      y_major_incr = w->plot.y.MajorInc;
      y1_val = w->plot.y.DrawOrigin;
      x = w->plot.x.AxisPos;
      log = w->plot.YLog;
      leftside = w->plot.YAxisLeft;
      break;
    case CP_Y2:
      w->plot.current_id = SciPlotDrawingY2Axis;
      num_y_major = w->plot.y2.MajorNum;
      y_major_incr = w->plot.y2.MajorInc;
      y1_val = w->plot.y2.DrawOrigin;
      x = w->plot.y2.AxisPos;
      log = w->plot.Y2Log;
      leftside = w->plot.Y2AxisLeft;
      break;
    case CP_Y3:
      w->plot.current_id = SciPlotDrawingY3Axis;
      num_y_major = w->plot.y3.MajorNum;
      y_major_incr = w->plot.y3.MajorInc;
      y1_val = w->plot.y3.DrawOrigin;
      x = w->plot.y3.AxisPos;
      log = w->plot.Y3Log;
      leftside = w->plot.Y3AxisLeft;
      break;
    case CP_Y4:
      w->plot.current_id = SciPlotDrawingY4Axis;
      num_y_major = w->plot.y4.MajorNum;
      y_major_incr = w->plot.y4.MajorInc;
      y1_val = w->plot.y4.DrawOrigin;
      x = w->plot.y4.AxisPos;
      log = w->plot.Y4Log;
      leftside = w->plot.Y4AxisLeft;
      break;
    }
  y1_pos = PlotY(w, w->plot.y.DrawOrigin, CP_Y);
  y2_pos = PlotY(w, w->plot.y.DrawMax, CP_Y);

  /* draw y axis line */
  LineSet(w, x, y1_pos, x, y2_pos, w->plot.ForegroundColor, XtLINE_SOLID);

  sprintf(label, "%.6g", y1_val);
  if (leftside)
    {
      offset = -5 - FontnumTextWidth(w, w->plot.axisFont, label);
    }
  else
    {
      offset = 8;
    }
  y = y1_pos + height / 2.0 - FontnumDescent(w, w->plot.axisFont);
  TextSet(w, x + offset, y, label, w->plot.ForegroundColor, w->plot.axisFont);

  majorval = y1_val;
  for (i = 0; i < num_y_major; i++)
    {
      /* minor x axis ticks and grid lines */
      if (log)
        {
          majorval *= y_major_incr;
        }
      else
        {
          majorval += y_major_incr;
        }

      /* major y axis ticks and grid lines */
      y = PlotY(w, majorval, axis);
      labelval = (log) ? majorval : floor(majorval / y_major_incr + .5) * y_major_incr;
      if (fabs(labelval) < DBL_MIN)
        labelval = 0.; /* hack to remove "-0" */
      sprintf(label, "%.6g", labelval);
      if (leftside)
        {
          offset = -5 - FontnumTextWidth(w, w->plot.axisFont, label);
        }
      else
        {
          offset = 8;
        }
      y += height / 2.0 - FontnumDescent(w, w->plot.axisFont);
      TextSet(w, x + offset, y, label, w->plot.ForegroundColor, w->plot.axisFont);
    }
  if (type == DRAW_ALL)
    {
      DrawCartesianYMajorAndMinor(w, axis);
      DrawCartesianYLabel(w, axis);
    }
  else if (type == DRAW_NO_LABELS)
    {
      DrawCartesianYMajorAndMinor(w, axis);
    }
}

static void
DrawCartesianAxes(SciPlotWidget w, int type)
{
  DrawCartesianXAxis(w, type);
  if (w->plot.YAxisShow)
    {
      DrawCartesianYAxis(w, type, CP_Y);
    }
  if (w->plot.Y2AxisShow)
    {
      DrawCartesianYAxis(w, type, CP_Y2);
    }
  if (w->plot.Y3AxisShow)
    {
      DrawCartesianYAxis(w, type, CP_Y3);
    }
  if (w->plot.Y4AxisShow)
    {
      DrawCartesianYAxis(w, type, CP_Y4);
    }
}

static void
DrawCartesianPlot(SciPlotWidget w)
{
  int i, j, jstart;
  SciPlotList *p;
  Boolean AxisOnRight = FALSE;

  w->plot.current_id = SciPlotDrawingAny;
  ClipSet(w);
  w->plot.current_id = SciPlotDrawingLine;

  if ((w->plot.YAxisShow && !w->plot.YAxisLeft) ||
      (w->plot.Y2AxisShow && !w->plot.Y2AxisLeft) ||
      (w->plot.Y3AxisShow && !w->plot.Y3AxisLeft) ||
      (w->plot.Y4AxisShow && !w->plot.Y4AxisLeft))
    {
      AxisOnRight = TRUE;
    }

  for (i = 0; i < w->plot.num_plotlist; i++)
    {
      p = w->plot.plotlist + i;
      if (p->draw)
        {
          real x1 = 0, y1 = 0, x2 = 0, y2 = 0, y0 = 0;
          Boolean skipnext = False;

          jstart = 0;
          while ((jstart < p->number) &&
                 (((p->data[jstart].x <= SCIPLOT_SKIP_VAL ||
                    p->data[jstart].y <= SCIPLOT_SKIP_VAL) ||
                   (w->plot.XLog && (p->data[jstart].x <= 0.0)) ||
                   ((p->axis == CP_Y) && w->plot.YLog && (p->data[jstart].y <= 0.0)) ||
                   ((p->axis == CP_Y2) && w->plot.Y2Log && (p->data[jstart].y <= 0.0)) ||
                   ((p->axis == CP_Y3) && w->plot.Y3Log && (p->data[jstart].y <= 0.0)) ||
                   ((p->axis == CP_Y4) && w->plot.Y4Log && (p->data[jstart].y <= 0.0)))))
            jstart++;
          if (jstart < p->number)
            {
              x1 = PlotX(w, p->data[jstart].x);
              y1 = PlotY(w, p->data[jstart].y, p->axis);
            }
          for (j = jstart; j < p->number; j++)
            {
              if (p->data[j].x <= SCIPLOT_SKIP_VAL ||
                  p->data[j].y <= SCIPLOT_SKIP_VAL)
                {
                  skipnext = True;
                  continue;
                }

              if (!((w->plot.XLog && (p->data[j].x <= 0.0)) ||
                    ((p->axis == CP_Y) && w->plot.YLog && (p->data[j].y <= 0.0)) ||
                    ((p->axis == CP_Y2) && w->plot.Y2Log && (p->data[j].y <= 0.0)) ||
                    ((p->axis == CP_Y3) && w->plot.Y3Log && (p->data[j].y <= 0.0)) ||
                    ((p->axis == CP_Y4) && w->plot.Y4Log && (p->data[j].y <= 0.0))))
                {
                  x2 = PlotX(w, p->data[j].x);
                  y2 = PlotY(w, p->data[j].y, p->axis);
                  if (!skipnext)
                    {
                      if (p->FillStyle == XtLINE_NONE)
                        {
                          if (p->LineStyle == XtLINE_STEP)
                            {
                              LineSet(w, x1, y1, x2, y1, p->LineColor, XtLINE_SOLID);
                              if (AxisOnRight && (p->data[j].x == w->plot.x.DrawMax))
                                { //avoid drawing directly on the y2 axis
                                  x2--;
                                }
                              LineSet(w, x2, y1, x2, y2, p->LineColor, XtLINE_SOLID);
                              if (j + 1 == p->number)
                                { //Extrapolate the last bit to the current time
                                  x1 = PlotX(w, w->plot.x.DrawMax);
                                  LineSet(w, x2, y2, x1, y2, p->LineColor, XtLINE_SOLID);
                                }
                            }
                          else
                            {
                              LineSet(w, x1, y1, x2, y2, p->LineColor, p->LineStyle);
                            }
                        }
                      else
                        {
                          switch (p->axis)
                            {
                            case CP_Y:
                              if (w->plot.y.DrawOrigin > 0)
                                y0 = w->plot.y.DrawOrigin;
                              else if (w->plot.y.DrawOrigin + w->plot.y.DrawSize < 0)
                                y0 = w->plot.y.DrawOrigin + w->plot.y.DrawSize;
                              else
                                y0 = 0;
                              y0 = PlotY(w, y0, p->axis);
                              break;
                            case CP_Y2:
                              if (w->plot.y2.DrawOrigin > 0)
                                y0 = w->plot.y2.DrawOrigin;
                              else if (w->plot.y2.DrawOrigin + w->plot.y2.DrawSize < 0)
                                y0 = w->plot.y2.DrawOrigin + w->plot.y2.DrawSize;
                              else
                                y0 = 0;
                              y0 = PlotY(w, y0, p->axis);
                              break;
                            case CP_Y3:
                              if (w->plot.y3.DrawOrigin > 0)
                                y0 = w->plot.y3.DrawOrigin;
                              else if (w->plot.y3.DrawOrigin + w->plot.y3.DrawSize < 0)
                                y0 = w->plot.y3.DrawOrigin + w->plot.y3.DrawSize;
                              else
                                y0 = 0;
                              y0 = PlotY(w, y0, p->axis);
                              break;
                            case CP_Y4:
                              if (w->plot.y4.DrawOrigin > 0)
                                y0 = w->plot.y4.DrawOrigin;
                              else if (w->plot.y4.DrawOrigin + w->plot.y4.DrawSize < 0)
                                y0 = w->plot.y4.DrawOrigin + w->plot.y4.DrawSize;
                              else
                                y0 = 0;
                              y0 = PlotY(w, y0, p->axis);
                              break;
                            }
                          FilledQuadSet(w, x1, y1, x2, y2, x2, y0, x1, y0, p->LineColor, p->LineStyle);
                        }
                    }
                  x1 = x2;
                  y1 = y2;
                }
              skipnext = False;
            }
        }
    }
  w->plot.current_id = SciPlotDrawingAny;
  ClipClear(w);
  w->plot.current_id = SciPlotDrawingLine;
  for (i = 0; i < w->plot.num_plotlist; i++)
    {
      p = w->plot.plotlist + i;
      if (p->draw)
        {
          real x2, y2;

          for (j = 0; j < p->number; j++)
            {
              if (!((w->plot.XLog && (p->data[j].x <= 0.0)) ||
                    ((p->axis == CP_Y) && w->plot.YLog && (p->data[j].y <= 0.0)) ||
                    ((p->axis == CP_Y2) && w->plot.Y2Log && (p->data[j].y <= 0.0)) ||
                    ((p->axis == CP_Y3) && w->plot.Y3Log && (p->data[j].y <= 0.0)) ||
                    ((p->axis == CP_Y4) && w->plot.Y4Log && (p->data[j].y <= 0.0)) ||
                    p->data[j].x <= SCIPLOT_SKIP_VAL ||
                    p->data[j].y <= SCIPLOT_SKIP_VAL))
                {
                  x2 = PlotX(w, p->data[j].x);
                  y2 = PlotY(w, p->data[j].y, p->axis);
                  if ((x2 >= w->plot.x.Origin) &&
                      (x2 <= w->plot.x.Origin + w->plot.x.Size) &&
                      (y2 >= w->plot.y.Origin) &&
                      (y2 <= w->plot.y.Origin + w->plot.y.Size))
                    {

                      DrawMarker(w, x2, y2,
                                 p->markersize,
                                 p->PointColor,
                                 p->PointStyle);

                      if (p->PointStyle == XtMARKER_HTEXT && p->markertext)
                        {
                          if (p->markertext[j])
                            TextSet(w, x2, y2, p->markertext[j], p->PointColor,
                                    w->plot.axisFont);
                        }
                      else if (p->PointStyle == XtMARKER_VTEXT && p->markertext)
                        {
                          if (p->markertext[j])
                            VTextSet(w, x2, y2, p->markertext[j], p->PointColor,
                                     w->plot.axisFont);
                        }
                    }
                }
            }
        }
    }
}

static void
DrawAll(SciPlotWidget w)
{
  if (w->primitive.shadow_thickness > 0)
    {
      /* Just support shadow in, out or none (not etched in or out) */
      if (w->plot.ShadowType == XmSHADOW_OUT)
        {
          DrawShadow(w, True, 0, 0, w->core.width, w->core.height);
        }
      else if (w->plot.ShadowType == XmSHADOW_IN)
        {
          DrawShadow(w, False, 0, 0, w->core.width, w->core.height);
        }
    }
  DrawCartesianPlot(w);
  DrawCartesianAxes(w, DRAW_ALL);
  //DrawCartesianPlot(w); //draw axes last because fillunder covers it up.
}

static Boolean
DrawQuick(SciPlotWidget w)
{
  Boolean range_check;
  range_check = CheckMinMax(w, COMPUTE_MIN_MAX);
  EraseClassItems(w, SciPlotDrawingLine);
  EraseAllItems(w);
  DrawAll(w);

  return range_check;
}

/*
 * Public Plot functions
 */

void SciPlotSetFileVersion(Widget wi, int version)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  w->plot.medmFileVersion = version;
}

void SciPlotSetBackgroundColor(Widget wi, int color)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  if (color < w->plot.num_colors)
    {
      w->plot.BackgroundColor = color;
      w->core.background_pixel = w->plot.colors[color];
      XSetWindowBackground(XtDisplay(w), XtWindow(w), w->core.background_pixel);
      /* Redefine the shadow colors */
      {
        Pixel fg, select, shade1, shade2;

        XmGetColors(XtScreen(w), w->core.colormap,
                    w->core.background_pixel, &fg, &shade1, &shade2, &select);
        w->plot.ShadowColor1 = ColorStore(w, shade1);
        w->plot.ShadowColor2 = ColorStore(w, shade2);
        w->primitive.top_shadow_color = shade1;
        w->primitive.bottom_shadow_color = shade2;
      }
    }
}

void SciPlotSetForegroundColor(Widget wi, int color)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  if (color < w->plot.num_colors)
    w->plot.ForegroundColor = color;
}

void SciPlotListDelete(Widget wi, int idnum)
{
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;

  p = _ListFind(w, idnum);
  if (p)
    _ListDelete(p);
}

int SciPlotListCreateDouble(Widget wi, int num, double *xlist, double *ylist, int axis)
{
  int idnum;
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return -1;

  w = (SciPlotWidget)wi;

  idnum = _ListNew(w);
  p = w->plot.plotlist + idnum;
  _ListSetDouble(p, num, xlist, ylist);
  _ListSetStyle(p, 1, XtMARKER_CIRCLE, 1, XtLINE_SOLID, XtLINE_NONE);
  p->axis = axis;

  return idnum;
}

void SciPlotListUpdateDouble(Widget wi, int idnum, int num, double *xlist, double *ylist)
{
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;

  p = _ListFind(w, idnum);
  if (p)
    _ListSetDouble(p, num, xlist, ylist);
}

void SciPlotListSetStyle(Widget wi, int idnum, int pcolor, int pstyle, int lcolor, int lstyle, int fstyle, int axis, int yside)
{
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;

  p = _ListFind(w, idnum);
  if (p)
    {
      _ListSetStyle(p, pcolor, pstyle, lcolor, lstyle, fstyle);
      p->yside = yside;
      if (yside == 0)
        {
          switch (axis)
            {
            case CP_Y:
              w->plot.YAxisLeft = TRUE;
              break;
            case CP_Y2:
              w->plot.Y2AxisLeft = TRUE;
              break;
            case CP_Y3:
              w->plot.Y3AxisLeft = TRUE;
              break;
            case CP_Y4:
              w->plot.Y4AxisLeft = TRUE;
              break;
            }
        }
      else
        {
          switch (axis)
            {
            case CP_Y:
              w->plot.YAxisLeft = FALSE;
              break;
            case CP_Y2:
              w->plot.Y2AxisLeft = FALSE;
              break;
            case CP_Y3:
              w->plot.Y3AxisLeft = FALSE;
              break;
            case CP_Y4:
              w->plot.Y4AxisLeft = FALSE;
              break;
            }
        }
    }
}

void SciPlotListSetMarkerSize(Widget wi, int idnum, double size)
{
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;

  p = _ListFind(w, idnum);
  if (p)
    p->markersize = size;
}

#if 0
/* KE: p is not initialized in this routine
 *   The routine needs to be done properly if it is to be used */
void
SciPlotListSetMarkerText (Widget wi, int idnum, char** text, int num,
                          int style)
{
  SciPlotList *p;
  SciPlotWidget w;
  int realnum;
  int i;

  if (!XtIsSciPlot(wi))
    return;

  if (style != XtMARKER_HTEXT && style != XtMARKER_VTEXT)
    return;

  if (text == 0 || num == 0) {  /* erase existing marker */
    if (p->markertext) {
      for (i = 0; i < p->number; i++) {
        if (p->markertext[i])
          free (p->markertext[i]);
      }
      free (p->markertext);
    }
    p->markertext = 0;
    return;
  }

  w = (SciPlotWidget) wi;

  p = _ListFind(w, idnum);

  if (p) {
    realnum = (num < p->number) ? num : p->number;
    if (p->markertext) {
      for (i = 0; i < p->number; i++) {
        if (p->markertext[i])
          free (p->markertext[i]);
      }
      free (p->markertext);
    }

    /* allocate memory */
    p->markertext = (char **)malloc (p->number*sizeof (char *));
    if (!p->markertext)
      return;

    for (i = 0; i < realnum; i++) {
      p->markertext[i] =(char *)malloc((strlen (text[i]) + 1)*sizeof (char));
      if (p->markertext[i])
        strcpy (p->markertext[i], text[i]);
    }
    for (i = realnum; i < p->number; i++)
      p->markertext[i] = NULL;


    p->PointStyle = style;
  }
}
#endif

void SciPlotSetXAutoScale(Widget wi)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  w->plot.XAutoScale = True;
}

void SciPlotSetXUserScale(Widget wi, double min, double max)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  if (min < max)
    {
      w->plot.XAutoScale = False;
      w->plot.UserMin.x = (real)min;
      w->plot.UserMax.x = (real)max;
    }
}

void SciPlotSetXUserMin(Widget wi, double min)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  w->plot.XAutoScale = False;
  w->plot.UserMin.x = (real)min;
}

void SciPlotSetXUserMax(Widget wi, double max)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  w->plot.XAutoScale = False;
  w->plot.UserMax.x = (real)max;
}

void SciPlotSetYAutoScale(Widget wi, int axis)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  switch (axis)
    {
    case CP_Y:
      w->plot.YAutoScale = True;
      break;
    case CP_Y2:
      w->plot.Y2AutoScale = True;
      break;
    case CP_Y3:
      w->plot.Y3AutoScale = True;
      break;
    case CP_Y4:
      w->plot.Y4AutoScale = True;
      break;
    }
  w->plot.YAutoScale = True;
}

void SciPlotSetYUserScale(Widget wi, double min, double max, int axis)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  if (min < max)
    {
      switch (axis)
        {
        case CP_Y:
          w->plot.YAutoScale = False;
          w->plot.UserMin.y = (real)min;
          w->plot.UserMax.y = (real)max;
          break;
        case CP_Y2:
          w->plot.Y2AutoScale = False;
          w->plot.UserMin.y2 = (real)min;
          w->plot.UserMax.y2 = (real)max;
          break;
        case CP_Y3:
          w->plot.Y3AutoScale = False;
          w->plot.UserMin.y3 = (real)min;
          w->plot.UserMax.y3 = (real)max;
          break;
        case CP_Y4:
          w->plot.Y4AutoScale = False;
          w->plot.UserMin.y4 = (real)min;
          w->plot.UserMax.y4 = (real)max;
          break;
        }
    }
}

void SciPlotSetYUserMin(Widget wi, double min, int axis)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  switch (axis)
    {
    case CP_Y:
      w->plot.YAutoScale = False;
      w->plot.UserMin.y = (real)min;
      break;
    case CP_Y2:
      w->plot.Y2AutoScale = False;
      w->plot.UserMin.y2 = (real)min;
      break;
    case CP_Y3:
      w->plot.Y3AutoScale = False;
      w->plot.UserMin.y3 = (real)min;
      break;
    case CP_Y4:
      w->plot.Y4AutoScale = False;
      w->plot.UserMin.y4 = (real)min;
      break;
    }
}

void SciPlotSetYUserMax(Widget wi, double max, int axis)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  switch (axis)
    {
    case CP_Y:
      w->plot.YAutoScale = False;
      w->plot.UserMax.y = (real)max;
      break;
    case CP_Y2:
      w->plot.Y2AutoScale = False;
      w->plot.UserMax.y2 = (real)max;
      break;
    case CP_Y3:
      w->plot.Y3AutoScale = False;
      w->plot.UserMax.y3 = (real)max;
      break;
    case CP_Y4:
      w->plot.Y4AutoScale = False;
      w->plot.UserMax.y4 = (real)max;
      break;
    }
}

void SciPlotGetXScale(Widget wi, double *min, double *max)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  if (w->plot.XAutoScale)
    {
      *min = w->plot.Min.x;
      *max = w->plot.Max.x;
    }
  else
    {
      *min = w->plot.UserMin.x;
      *max = w->plot.UserMax.x;
    }
}

void SciPlotGetYScale(Widget wi, double *min, double *max, int axis)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  switch (axis)
    {
    case CP_Y:
      if (w->plot.YAutoScale)
        {
          *min = w->plot.Min.y;
          *max = w->plot.Max.y;
        }
      else
        {
          *min = w->plot.UserMin.y;
          *max = w->plot.UserMax.y;
        }
      break;
    case CP_Y2:
      if (w->plot.Y2AutoScale)
        {
          *min = w->plot.Min.y2;
          *max = w->plot.Max.y2;
        }
      else
        {
          *min = w->plot.UserMin.y2;
          *max = w->plot.UserMax.y2;
        }
      break;
    case CP_Y3:
      if (w->plot.Y3AutoScale)
        {
          *min = w->plot.Min.y3;
          *max = w->plot.Max.y3;
        }
      else
        {
          *min = w->plot.UserMin.y3;
          *max = w->plot.UserMax.y3;
        }
      break;
    case CP_Y4:
      if (w->plot.Y4AutoScale)
        {
          *min = w->plot.Min.y4;
          *max = w->plot.Max.y4;
        }
      else
        {
          *min = w->plot.UserMin.y4;
          *max = w->plot.UserMax.y4;
        }
      break;
    }
}

void SciPlotPrintStatistics(Widget wi)
{
  int i, j;
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;

  SciPlotPrintf("Title=%s\nxlabel=%s\tylabel=%s\n",
                w->plot.plotTitle, w->plot.xlabel, w->plot.ylabel);
  SciPlotPrintf("Degrees=%d\n", w->plot.Degrees);
  SciPlotPrintf("XLog=%d\tYLog=%d\n", w->plot.XLog, w->plot.YLog);
  SciPlotPrintf("XAutoScale=%d\tYAutoScale=%d\n",
                w->plot.XAutoScale, w->plot.YAutoScale);
  for (i = 0; i < w->plot.num_plotlist; i++)
    {
      p = w->plot.plotlist + i;
      if (p->draw)
        {
          SciPlotPrintf("Styles: point=%d line=%d  Color: point=%d line=%d\n",
                        p->PointStyle, p->LineStyle, p->PointColor, p->LineColor);
          for (j = 0; j < p->number; j++)
            SciPlotPrintf("%f\t%f\n", p->data[j].x, p->data[j].y);
          SciPlotPrintf("\n");
        }
    }
}

int SciPlotStoreAllocatedColor(Widget wi, Pixel p)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return -1;

  w = (SciPlotWidget)wi;

  return ColorStore(w, p);
}

void SciPlotUpdate(Widget wi)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    {
      return;
    }

  w = (SciPlotWidget)wi;
  EraseAll(w);

#if DEBUG_SCIPLOT
  SciPlotPrintStatistics(wi);
#endif
  ComputeAll(w, COMPUTE_MIN_MAX);
  DrawAll(w);
}

Boolean
SciPlotQuickUpdate(Widget wi)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return False;

  w = (SciPlotWidget)wi;
  return DrawQuick(w);
}

/****************************************************************************
 *            Addtional functions
 ***************************************************************************/

/***************************************************************************
 *      Private data conversion from screen to data coordination
 ***************************************************************************/
static real
OutputX(SciPlotWidget w, real xin)
{
  real xout;

  if (w->plot.XLog)
    xout = pow(10.0, log10(w->plot.x.DrawOrigin) +
               (xin - w->plot.x.Origin) *
               (w->plot.x.DrawSize / w->plot.x.Size));
  else
    xout = w->plot.x.DrawOrigin +
      ((xin - w->plot.x.Origin) *
       (w->plot.x.DrawSize / w->plot.x.Size));
  return xout;
}

static real
OutputY(SciPlotWidget w, real yin)
{
  real yout;

  if (w->plot.YLog)
    yout = pow(10.0, log10(w->plot.y.DrawOrigin) +
               (w->plot.y.Origin + w->plot.y.Size - yin) *
               (w->plot.y.DrawSize / w->plot.y.Size));
  else
    yout = w->plot.y.DrawOrigin +
      ((w->plot.y.Origin + w->plot.y.Size - yin) *
       (w->plot.y.DrawSize / w->plot.y.Size));
  return yout;
}

static void
ComputeAxis_i(SciPlotAxis *axis, real min, real max, Boolean log,
              real *drawOrigin, real *drawMax, int max_major)
{
  real range, rnorm, delta, calcmin, calcmax;
  int nexp, majornum, i;
  //int majordecimals, minornum;
  range = max - min;

  if (log)
    {
      if (range == 0.0)
        {
          calcmin = powi(10.0, (int)floor(log10(min) + SCIPLOT_EPS));
          calcmax = 10.0 * calcmin;
        }
      else
        {
          calcmin = powi(10.0, (int)floor(log10(min) + SCIPLOT_EPS));
          calcmax = powi(10.0, (int)ceil(log10(max) - SCIPLOT_EPS));
        }

      /*
        SciPlotPrintf("calcmin=%e min=%e   calcmax=%e max=%e\n",calcmin,min,
        calcmax,max); */

      delta = 10.0;

      *drawOrigin = calcmin;
      *drawMax = calcmax;
    }
  else
    {
      if (range == 0.0)
        nexp = 0;
      else
        nexp = (int)floor(log10(range) + SCIPLOT_EPS);
      rnorm = range / powi(10.0, nexp);
      for (i = 0; i < NUMBER_MINOR; i++)
        {
          delta = CAdeltas[i];
          //minornum = CAminors[i];
          majornum = (int)((rnorm + 0.9999 * delta) / delta);
          //majordecimals = CAdecimals[i];
          if (majornum <= max_major)
            break;
        }
      delta *= powi(10.0, nexp);
#if DEBUG_SCIPLOT
      SciPlotPrintf("nexp=%d range=%f rnorm=%f delta=%f\n", nexp, range, rnorm, delta);
#endif

      if (min < 0.0)
        calcmin = ((double)((int)((min - .9999 * delta) / delta))) * delta;
      else if ((min > 0.0) && (min < 1.0))
        calcmin = ((double)((int)((1.0001 * min) / delta))) * delta;
      else if (min >= 1.0)
        calcmin = ((double)((int)((.9999 * min) / delta))) * delta;
      else
        calcmin = min;
      if (max < 0.0)
        calcmax = ((double)((int)((.9999 * max) / delta))) * delta;
      else if (max > 0.0)
        calcmax = ((double)((int)((max + .9999 * delta) / delta))) * delta;
      else
        calcmax = max;

      *drawOrigin = calcmin;
      *drawMax = calcmax;
    }
}

static void
ComputeXDrawingRange_i(SciPlotWidget w, real xmin,
                       real xmax, real *drawOrigin, real *drawMax)
{
  ComputeAxis_i(&w->plot.x, xmin, xmax,
                w->plot.XLog, drawOrigin, drawMax, MAX_MAJOR_X);
}

static void
ComputeYDrawingRange_i(SciPlotWidget w, real ymin,
                       real ymax, real *drawOrigin, real *drawMax)
{
  ComputeAxis_i(&w->plot.y, ymin, ymax,
                w->plot.YLog, drawOrigin, drawMax, MAX_MAJOR_Y);
}

static void
sciPlotMotionAP(SciPlotWidget w,
                XEvent *event, char *args, int n_args)
{
  int x, y;
  int xorig, xend;
  int yorig, yend;
  real xscale, yscale;
  real minxlim, minylim, maxxlim, maxylim; /* view port */
  //real minxdat, minydat, maxxdat, maxydat;  /* data port */
  real newlim = 0.0;
  real txmin, txmax;
  real tymin, tymax;
  real xoffset;
  real newlim0, newlim1;

  if (event->type == ButtonPress)
    XmProcessTraversal((Widget)w, XmTRAVERSE_CURRENT);

  /* get boundary information */
  if (w->plot.XLog)
    {
      minxlim = log10(w->plot.x.DrawOrigin);
      maxxlim = log10(w->plot.x.DrawOrigin) + w->plot.x.DrawSize;
      //minxdat = log10(w->plot.Min.x);
      //maxxdat = log10(w->plot.Max.x);
    }
  else
    {
      minxlim = w->plot.x.DrawOrigin;
      maxxlim = w->plot.x.DrawOrigin + w->plot.x.DrawSize;
      //minxdat = w->plot.Min.x;
      //maxxdat = w->plot.Max.x;
    }

  if (w->plot.YLog)
    {
      minylim = log10(w->plot.y.DrawOrigin);
      maxylim = log10(w->plot.y.DrawOrigin) + w->plot.y.DrawSize;
      //minydat = log10(w->plot.Min.y);
      //maxydat = log10(w->plot.Max.y);
    }
  else
    {
      minylim = w->plot.y.DrawOrigin;
      maxylim = w->plot.y.DrawOrigin + w->plot.y.DrawSize;
      //minydat = w->plot.Min.y;
      //maxydat = w->plot.Max.y;
    }

  /* get scale factor for converting data coords to screen coords */
  xscale = (maxxlim - minxlim) / (w->plot.x.Size);
  yscale = (maxylim - minylim) / (w->plot.y.Size);
  xorig = PlotX(w, w->plot.x.DrawOrigin);
  yorig = PlotY(w, w->plot.y.DrawOrigin, CP_Y);
  xend = PlotX(w, w->plot.x.DrawMax);
  yend = PlotY(w, w->plot.y.DrawMax, CP_Y);

  x = event->xbutton.x;
  y = event->xbutton.y;

  switch (w->plot.drag_state)
    {
    case NOT_DRAGGING:
      if (x >= xorig && x <= xend && y <= yorig && y >= yend)
        w->plot.drag_state = DRAGGING_DATA;
      else if (x < xorig && y > yorig)
        w->plot.drag_state = DRAGGING_BOTTOM_AND_LEFT;
      else if (x < xorig && y <= yorig && y >= yend)
        {
          if (y < yend + (yorig - yend) / 2)
            w->plot.drag_state = DRAGGING_TOP;
          else
            w->plot.drag_state = DRAGGING_BOTTOM;
        }
      else if (x >= xorig && x <= xend && y > yorig)
        {
          if (x < xorig + (xend - xorig) / 2)
            w->plot.drag_state = DRAGGING_LEFT;
          else
            w->plot.drag_state = DRAGGING_RIGHT;
        }
      else
        w->plot.drag_state = DRAGGING_NOTHING;
      w->plot.drag_start.x = minxlim + (real)(x - w->plot.x.Origin) * xscale;
      /* y origin is at the top */
      w->plot.drag_start.y = minylim +
        (real)(w->plot.y.Origin + w->plot.y.Size - y) * yscale;
      break;
    case DRAGGING_DATA:
      if (w->plot.DragX)
        {
          if (!w->plot.XLog)
            { /* not dragging for x log axis yet */
              /* only allow x direction dragging data */
              xoffset = minxlim + (x - w->plot.x.Origin) * xscale -
                w->plot.drag_start.x;

              if (xoffset != 0.0)
                {
                  newlim1 = maxxlim - xoffset;
                  newlim0 = minxlim - xoffset;

                  /* calculate maximum drawng range */
                  ComputeXDrawingRange_i(w, w->plot.startx, w->plot.endx,
                                         &txmin, &txmax);

                  if (newlim1 <= txmax && newlim0 >= txmin)
                    w->plot.reach_limit = 0;
                  else if (newlim1 > txmax)
                    newlim1 = txmax;
                  else if (newlim0 < txmin)
                    newlim0 = txmin;

                  if (!w->plot.reach_limit)
                    {
                      /* KE: Did not use SCIPLOT_EPS here since UserMax could be smaller
                       *  than SCIPLOT_EPS.  Don't understand why ceil is used with
                       *  these values either.  Might want to use * (1.0 - SCIPLOT_EPS) */
                      w->plot.UserMax.x = ceil(newlim1);

                      if (w->plot.UserMax.x >= txmax)
                        {
                          w->plot.UserMax.x = txmax;
                          w->plot.reach_limit = 1;
                        }

                      w->plot.UserMin.x = w->plot.UserMax.x - w->plot.x.DrawSize;
                      if (w->plot.UserMin.x <= txmin)
                        {
                          w->plot.UserMin.x = txmin;
                          w->plot.UserMax.x = txmin + w->plot.x.DrawSize;
                          w->plot.reach_limit = 1;
                        }

                      w->plot.XAutoScale = False;

                      EraseClassItems(w, SciPlotDrawingLine);
                      EraseClassItems(w, SciPlotDrawingXAxis);
                      if (w->plot.YAxisShow)
                        {
                          EraseClassItems(w, SciPlotDrawingYAxis);
                        }
                      if (w->plot.Y2AxisShow)
                        {
                          EraseClassItems(w, SciPlotDrawingY2Axis);
                        }
                      if (w->plot.Y3AxisShow)
                        {
                          EraseClassItems(w, SciPlotDrawingY3Axis);
                        }
                      if (w->plot.Y4AxisShow)
                        {
                          EraseClassItems(w, SciPlotDrawingY4Axis);
                        }
                      ComputeAll(w, NO_COMPUTE_MIN_MAX);

                      DrawCartesianPlot(w);
                      DrawCartesianXAxis(w, DRAW_AXIS_ONLY);
                      if (w->plot.YAxisShow)
                        {
                          DrawCartesianYAxis(w, DRAW_AXIS_ONLY, CP_Y);
                        }
                      if (w->plot.Y2AxisShow)
                        {
                          DrawCartesianYAxis(w, DRAW_AXIS_ONLY, CP_Y2);
                        }
                      if (w->plot.Y3AxisShow)
                        {
                          DrawCartesianYAxis(w, DRAW_AXIS_ONLY, CP_Y3);
                        }
                      if (w->plot.Y4AxisShow)
                        {
                          DrawCartesianYAxis(w, DRAW_AXIS_ONLY, CP_Y4);
                        }
                      //DrawCartesianPlot(w);
                    }
                }
            }
        }
      break;
    case DRAGGING_RIGHT:
      if (w->plot.DragX)
        {
          if (x > w->plot.x.Origin)
            newlim = minxlim + (w->plot.drag_start.x - minxlim) *
              (w->plot.x.Size) / (x - w->plot.x.Origin);

          if (w->plot.XLog)
            {
              if (x <= w->plot.x.Origin || newlim > log10(w->plot.endx))
                newlim = log10(w->plot.endx);
            }
          else
            {
              if (x <= w->plot.x.Origin || newlim > w->plot.endx)
                newlim = w->plot.endx;
            }

          if (w->plot.XLog)
            newlim = pow(10.0, newlim);

          ComputeXDrawingRange_i(w, w->plot.Min.x, newlim,
                                 &txmin, &txmax);

          if (txmax != w->plot.x.DrawMax)
            {
              w->plot.UserMax.x = newlim;
              w->plot.UserMin.x = w->plot.Min.x;
              w->plot.XAutoScale = False;

              EraseClassItems(w, SciPlotDrawingLine);
              EraseClassItems(w, SciPlotDrawingXAxis);
              EraseClassItems(w, SciPlotDrawingXMajorMinor);

              ComputeAll(w, COMPUTE_MAX_ONLY);
              DrawCartesianPlot(w);
              DrawCartesianXAxis(w, DRAW_NO_LABELS);
              //DrawCartesianPlot(w);
            }
          else
            {
              w->plot.XAutoScale = True;
              ComputeMinMax(w);
            }
        }
      break;
    case DRAGGING_LEFT:
      if (w->plot.DragX)
        {
          if (x < w->plot.x.Origin + w->plot.x.Size)
            newlim = maxxlim - (maxxlim - w->plot.drag_start.x) *
              (w->plot.x.Size) / (w->plot.x.Origin + w->plot.x.Size - x);

          if (w->plot.XLog)
            {
              if (x >= w->plot.x.Origin + w->plot.x.Size || newlim < log10(w->plot.startx))
                newlim = log10(w->plot.startx);
            }
          else
            {
              if (x >= w->plot.x.Origin + w->plot.x.Size || newlim < w->plot.startx)
                newlim = w->plot.startx;
            }

          if (w->plot.XLog)
            newlim = pow(10.0, newlim);

          ComputeXDrawingRange_i(w, newlim, w->plot.Max.x,
                                 &txmin, &txmax);

          if (txmin != w->plot.x.DrawOrigin)
            {
              w->plot.UserMin.x = newlim;
              w->plot.UserMax.x = w->plot.Max.x;
              w->plot.XAutoScale = False;

              EraseAll(w);
              ComputeAll(w, COMPUTE_MIN_ONLY);

              DrawAll(w);
            }
          else
            {
              w->plot.XAutoScale = True;
              ComputeMinMax(w);
            }
        }
      break;
    case DRAGGING_TOP:
      if (w->plot.DragY)
        {
          /* origin is at top */
          if (y < w->plot.y.Origin + w->plot.y.Size)
            newlim = minylim + (w->plot.drag_start.y - minylim) *
              (w->plot.y.Size) / (w->plot.y.Origin + w->plot.y.Size - y);

          if (w->plot.YLog)
            {
              if (y >= w->plot.y.Origin + w->plot.y.Size ||
                  newlim > log10(w->plot.endy))
                newlim = log10(w->plot.endy);
            }
          else
            {
              if (y >= w->plot.y.Origin + w->plot.y.Size || newlim > w->plot.endy)
                newlim = w->plot.endy;
            }

          if (w->plot.YLog)
            newlim = pow(10.0, newlim);

          ComputeYDrawingRange_i(w, w->plot.Min.y, newlim,
                                 &tymin, &tymax);

          if (tymax != w->plot.y.DrawMax)
            {
              w->plot.UserMax.y = newlim;
              w->plot.UserMin.y = w->plot.Min.y;
              w->plot.YAutoScale = False;

              EraseClassItems(w, SciPlotDrawingLine);
              if (w->plot.YAxisShow)
                {
                  EraseClassItems(w, SciPlotDrawingYAxis);
                }
              if (w->plot.Y2AxisShow)
                {
                  EraseClassItems(w, SciPlotDrawingY2Axis);
                }
              if (w->plot.Y3AxisShow)
                {
                  EraseClassItems(w, SciPlotDrawingY3Axis);
                }
              if (w->plot.Y4AxisShow)
                {
                  EraseClassItems(w, SciPlotDrawingY4Axis);
                }

              ComputeAll(w, COMPUTE_MAX_ONLY);
              DrawCartesianPlot(w);
              if (w->plot.YAxisShow)
                {
                  DrawCartesianYAxis(w, DRAW_NO_LABELS, CP_Y);
                }
              if (w->plot.Y2AxisShow)
                {
                  DrawCartesianYAxis(w, DRAW_NO_LABELS, CP_Y2);
                }
              if (w->plot.Y3AxisShow)
                {
                  DrawCartesianYAxis(w, DRAW_NO_LABELS, CP_Y3);
                }
              if (w->plot.Y4AxisShow)
                {
                  DrawCartesianYAxis(w, DRAW_NO_LABELS, CP_Y4);
                }
              //DrawCartesianPlot(w);
            }
          else
            {
              w->plot.YAutoScale = True;
              ComputeMinMax(w);
            }
        }
      break;
    case DRAGGING_BOTTOM:
      if (w->plot.DragY)
        {
          /* origin is at top */
          if (y > w->plot.y.Origin)
            newlim = maxylim - (maxylim - w->plot.drag_start.y) *
              (w->plot.y.Size) / (y - w->plot.y.Origin);

          if (w->plot.YLog)
            {
              if (y < w->plot.y.Origin || newlim < log10(w->plot.starty))
                newlim = log10(w->plot.starty);
            }
          else
            {
              if (y < w->plot.y.Origin || newlim < w->plot.starty)
                newlim = w->plot.starty;
            }

          if (w->plot.YLog)
            newlim = pow(10.0, newlim);

          ComputeYDrawingRange_i(w, newlim, w->plot.Max.y,
                                 &tymin, &tymax);
          if (tymin != w->plot.y.DrawOrigin)
            {
              w->plot.UserMax.y = w->plot.Max.y;
              w->plot.UserMin.y = newlim;
              w->plot.YAutoScale = False;

              EraseClassItems(w, SciPlotDrawingLine);
              if (w->plot.YAxisShow)
                {
                  EraseClassItems(w, SciPlotDrawingYAxis);
                }
              if (w->plot.Y2AxisShow)
                {
                  EraseClassItems(w, SciPlotDrawingY2Axis);
                }
              if (w->plot.Y3AxisShow)
                {
                  EraseClassItems(w, SciPlotDrawingY3Axis);
                }
              if (w->plot.Y4AxisShow)
                {
                  EraseClassItems(w, SciPlotDrawingY4Axis);
                }

              ComputeAll(w, COMPUTE_MIN_ONLY);
              DrawCartesianPlot(w);
              if (w->plot.YAxisShow)
                {
                  DrawCartesianYAxis(w, DRAW_NO_LABELS, CP_Y);
                }
              if (w->plot.Y2AxisShow)
                {
                  DrawCartesianYAxis(w, DRAW_NO_LABELS, CP_Y2);
                }
              if (w->plot.Y3AxisShow)
                {
                  DrawCartesianYAxis(w, DRAW_NO_LABELS, CP_Y3);
                }
              if (w->plot.Y4AxisShow)
                {
                  DrawCartesianYAxis(w, DRAW_NO_LABELS, CP_Y4);
                }
              //DrawCartesianPlot(w);
            }
          else
            {
              w->plot.YAutoScale = True;
              ComputeMinMax(w);
            }
        }
      break;
    default:
      break;
    }
}

static void sciPlotBtnUpAP(SciPlotWidget w,
                           XEvent *event, char *args, int n_args)
{
  SciPlotDragCbkStruct cbk;

  if (w->plot.DragX)
    {
      if (w->plot.drag_state == DRAGGING_LEFT ||
          w->plot.drag_state == DRAGGING_RIGHT ||
          w->plot.drag_state == DRAGGING_DATA)
        {
          cbk.action = w->plot.drag_state;
          cbk.event = event;
          cbk.drawMin = w->plot.x.DrawOrigin;
          cbk.drawMax = w->plot.x.DrawMax;
          XtCallCallbacks((Widget)w, XtNdragXCallback, &cbk);
        }
    }

  if (w->plot.DragY)
    {
      if (w->plot.drag_state == DRAGGING_TOP ||
          w->plot.drag_state == DRAGGING_BOTTOM)
        {
          cbk.action = w->plot.drag_state;
          cbk.event = event;
          cbk.drawMin = w->plot.x.DrawOrigin;
          cbk.drawMax = w->plot.x.DrawMax;
          XtCallCallbacks((Widget)w, XtNdragYCallback, &cbk);
        }
    }

  w->plot.drag_state = NOT_DRAGGING;
  w->plot.drag_start.x = 0.0;
  w->plot.drag_start.y = 0.0;
  w->plot.reach_limit = 0;
}

static int get_plotlist_id(SciPlotWidget w, real x, real y)
{
  register int i, j;
  register SciPlotList *p;
  register real xval, yval;
  real dmin = 0.0;
  real d;
  int id = -1;
  int first = 1;

  for (i = 0; i < w->plot.num_plotlist; i++)
    {
      p = w->plot.plotlist + i;
      if (p->draw)
        {
          for (j = 0; j < p->number; j++)
            {

              /* Don't count the "break in line segment" flag for Min/Max */
              if (p->data[j].x > SCIPLOT_SKIP_VAL &&
                  p->data[j].y > SCIPLOT_SKIP_VAL)
                {
                  xval = p->data[j].x;
                  yval = p->data[j].y;

                  if (first)
                    {
                      dmin = (x - xval) * (x - xval) + (y - yval) * (y - yval);
                      first = 0;
                      id = i;
                    }
                  else
                    {
                      d = (x - xval) * (x - xval) + (y - yval) * (y - yval);
                      if (d < dmin)
                        {
                          dmin = d;
                          id = i;
                        }
                    }
                }
            }
        }
    }

  return id;
}

static void sciPlotClick(SciPlotWidget w,
                         XEvent *event, char *args, int n_args)
{
  SciPlotBtn1ClickCbkStruct cbs;

  if (w->plot.btn1_callback != NULL)
    {
      cbs.event = event;
      cbs.action = 0;
      cbs.x = OutputX(w, event->xbutton.x);
      cbs.y = OutputY(w, event->xbutton.y);
      cbs.plotid = get_plotlist_id(w, cbs.x, cbs.y);
      XtCallCallbacks((Widget)w, XtNbtn1ClickCallback, &cbs);
    }
}

static void sciPlotTrackPointer(SciPlotWidget w,
                                XEvent *event, char *args, int n_args)
{
  int x, y;
  SciPlotPointerCbkStruct cbs;

  if (!w->plot.TrackPointer)
    return;

  x = event->xbutton.x;
  y = event->xbutton.y;

  cbs.event = event;
  cbs.action = 0;
  cbs.x = OutputX(w, x);
  cbs.y = OutputY(w, y);
  XtCallCallbacks((Widget)w, XtNpointerValCallback, &cbs);
}

/* KE: Added routines */

void SciPlotGetXAxisInfo(Widget wi, double *min, double *max, Boolean *isLog,
                         Boolean *isAuto, Boolean *isTime, char **timeFormat)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  *isLog = w->plot.XLog;
  *isTime = w->plot.XTime;
  *timeFormat = w->plot.XTimeFormat;
  if (w->plot.XAutoScale)
    {
      *min = w->plot.Min.x;
      *max = w->plot.Max.x;
      *isAuto = True;
    }
  else
    {
      *min = w->plot.UserMin.x;
      *max = w->plot.UserMax.x;
      *isAuto = False;
    }
}

void SciPlotGetYAxisInfo(Widget wi, double *min, double *max, Boolean *isLog,
                         Boolean *isAuto, int axis)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  switch (axis)
    {
    case CP_Y:
      *isLog = w->plot.YLog;
      if (w->plot.YAutoScale)
        {
          *min = w->plot.Min.y;
          *max = w->plot.Max.y;
          *isAuto = True;
        }
      else
        {
          *min = w->plot.UserMin.y;
          *max = w->plot.UserMax.y;
          *isAuto = False;
        }
      break;
    case CP_Y2:
      *isLog = w->plot.Y2Log;
      if (w->plot.Y2AutoScale)
        {
          *min = w->plot.Min.y2;
          *max = w->plot.Max.y2;
          *isAuto = True;
        }
      else
        {
          *min = w->plot.UserMin.y2;
          *max = w->plot.UserMax.y2;
          *isAuto = False;
        }
      break;
    case CP_Y3:
      *isLog = w->plot.Y3Log;
      if (w->plot.Y3AutoScale)
        {
          *min = w->plot.Min.y3;
          *max = w->plot.Max.y3;
          *isAuto = True;
        }
      else
        {
          *min = w->plot.UserMin.y3;
          *max = w->plot.UserMax.y3;
          *isAuto = False;
        }
      break;
    case CP_Y4:
      *isLog = w->plot.Y4Log;
      if (w->plot.Y4AutoScale)
        {
          *min = w->plot.Min.y4;
          *max = w->plot.Max.y4;
          *isAuto = True;
        }
      else
        {
          *min = w->plot.UserMin.y4;
          *max = w->plot.UserMax.y4;
          *isAuto = False;
        }
      break;
    }
}

/* This function prints widget metrics */
void SciPlotPrintMetrics(Widget wi)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;
  w = (SciPlotWidget)wi;

  SciPlotPrintf("\nPlot Metrics Information\n");
  SciPlotPrintf("  Width:           %d\n", w->core.width);
  SciPlotPrintf("  Height:          %d\n", w->core.height);
  SciPlotPrintf("  Margin:          %d\n", w->plot.Margin);
  SciPlotPrintf("  TitleMargin      %d\n", w->plot.TitleMargin);
  SciPlotPrintf("  TitleHeight      %g\n", FontnumHeight(w, w->plot.titleFont));
  SciPlotPrintf("  TitleAscent      %g\n", FontnumAscent(w, w->plot.titleFont));
  SciPlotPrintf("  TitleDescent     %g\n", FontnumDescent(w, w->plot.titleFont));
  SciPlotPrintf("  AxisLabelHeight  %g\n", FontnumHeight(w, w->plot.labelFont));
  SciPlotPrintf("  AxisLabelAscent  %g\n", FontnumAscent(w, w->plot.labelFont));
  SciPlotPrintf("  AxisLabelDescent %g\n", FontnumDescent(w, w->plot.labelFont));
  SciPlotPrintf("  AxisNumHeight    %g\n", FontnumHeight(w, w->plot.axisFont));
  SciPlotPrintf("  AxisNumAscent    %g\n", FontnumAscent(w, w->plot.axisFont));
  SciPlotPrintf("  AxisNumDescent   %g\n", FontnumDescent(w, w->plot.axisFont));
  SciPlotPrintf("  x.Origin:        %g\n", w->plot.x.Origin);
  SciPlotPrintf("  x.Size:          %g\n", w->plot.x.Size);
  SciPlotPrintf("  x.TitlePos:      %g\n", w->plot.x.TitlePos);
  SciPlotPrintf("  x.AxisPos:       %g\n", w->plot.x.AxisPos);
  SciPlotPrintf("  x.LabelPos:      %g\n", w->plot.x.LabelPos);
  SciPlotPrintf("  y.Origin:        %g\n", w->plot.y.Origin);
  SciPlotPrintf("  y.Size:          %g\n", w->plot.y.Size);
  SciPlotPrintf("  y.TitlePos:      %g\n", w->plot.y.TitlePos);
  SciPlotPrintf("  y.AxisPos:       %g\n", w->plot.y.AxisPos);
  SciPlotPrintf("  y.LabelPos:      %g\n", w->plot.y.LabelPos);
}

/*====================================================*/

/* This function prints axis info */
void SciPlotPrintAxisInfo(Widget wi)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;
  w = (SciPlotWidget)wi;

  SciPlotPrintf("\nPlot Axis Information\n");
  SciPlotPrintf("  XLog:            %s\n", w->plot.XLog ? "True" : "False");
  SciPlotPrintf("  XAutoScale:      %s\n", w->plot.XAutoScale ? "True" : "False");
  SciPlotPrintf("  XNoCompMinMax:   %s\n", w->plot.XNoCompMinMax ? "True" : "False");
  SciPlotPrintf("  XLeftSpace:      %d\n", w->plot.XLeftSpace);
  SciPlotPrintf("  XRightSpace:     %d\n", w->plot.XRightSpace);
  SciPlotPrintf("  Min.x:           %g\n", w->plot.Min.x);
  SciPlotPrintf("  Max.x:           %g\n", w->plot.Max.x);
  SciPlotPrintf("  UserMin.x:       %g\n", w->plot.UserMin.x);
  SciPlotPrintf("  UserMax.x:       %g\n", w->plot.UserMax.x);
  SciPlotPrintf("  x.Origin:        %g\n", w->plot.x.Origin);
  SciPlotPrintf("  x.Size:          %g\n", w->plot.x.Size);
  SciPlotPrintf("  x.TitlePos:      %g\n", w->plot.x.TitlePos);
  SciPlotPrintf("  x.AxisPos:       %g\n", w->plot.x.AxisPos);
  SciPlotPrintf("  x.LabelPos:      %g\n", w->plot.x.LabelPos);
  SciPlotPrintf("  x.DrawOrigin:    %g\n", w->plot.x.DrawOrigin);
  SciPlotPrintf("  x.DrawSize:      %g\n", w->plot.x.DrawSize);
  SciPlotPrintf("  x.DrawMax:       %g\n", w->plot.x.DrawMax);
  SciPlotPrintf("  x.MajorInc:      %g\n", w->plot.x.MajorInc);
  SciPlotPrintf("  x.MajorNum:      %d\n", w->plot.x.MajorNum);
  SciPlotPrintf("  x.MinorNum:      %d\n", w->plot.x.MinorNum);
  SciPlotPrintf("  x.Precision:     %d\n", w->plot.x.Precision);
  SciPlotPrintf("  x.FixedMargin:   %d\n", w->plot.x.FixedMargin);
  SciPlotPrintf("  x.Left:          %g\n", w->plot.x.Left);
  SciPlotPrintf("  x.Right:         %g\n", w->plot.x.Right);

  SciPlotPrintf("  YLog:            %s\n", w->plot.YLog ? "True" : "False");
  SciPlotPrintf("  YAutoScale:      %s\n", w->plot.YAutoScale ? "True" : "False");
  SciPlotPrintf("  Min.y:           %g\n", w->plot.Min.y);
  SciPlotPrintf("  Max.y:           %g\n", w->plot.Max.y);
  SciPlotPrintf("  UserMin.y:       %g\n", w->plot.UserMin.y);
  SciPlotPrintf("  UserMax.y:       %g\n", w->plot.UserMax.y);
  SciPlotPrintf("  y.Origin:        %g\n", w->plot.y.Origin);
  SciPlotPrintf("  y.Size:          %g\n", w->plot.y.Size);
  SciPlotPrintf("  y.TitlePos:      %g\n", w->plot.y.TitlePos);
  SciPlotPrintf("  y.AxisPos:       %g\n", w->plot.y.AxisPos);
  SciPlotPrintf("  y.LabelPos:      %g\n", w->plot.y.LabelPos);
  SciPlotPrintf("  y.DrawOrigin:    %g\n", w->plot.y.DrawOrigin);
  SciPlotPrintf("  y.DrawSize:      %g\n", w->plot.y.DrawSize);
  SciPlotPrintf("  y.DrawMax:       %g\n", w->plot.y.DrawMax);
  SciPlotPrintf("  y.MajorInc:      %g\n", w->plot.y.MajorInc);
  SciPlotPrintf("  y.MajorNum:      %d\n", w->plot.y.MajorNum);
  SciPlotPrintf("  y.MinorNum:      %d\n", w->plot.y.MinorNum);
  SciPlotPrintf("  y.Precision:     %d\n", w->plot.y.Precision);
  SciPlotPrintf("  y.FixedMargin:   %d\n", w->plot.y.FixedMargin);
  SciPlotPrintf("  y.Left:          %g\n", w->plot.y.Left);
  SciPlotPrintf("  y.Right:         %g\n", w->plot.y.Right);
}

static void
DrawShadow(SciPlotWidget w, Boolean raised, real x, real y,
           real width, real height)
{
  int i, color1, color2, color;

  if (w->primitive.shadow_thickness <= 0)
    return;

  w->plot.current_id = SciPlotDrawingShadows;

  /* Reduce the height and width by 1 for the algorithm */
  if (height > 0)
    height--;
  if (width > 0)
    width--;

  color1 = w->plot.ShadowColor1;
  color2 = w->plot.ShadowColor2;

  for (i = 0; i < w->primitive.shadow_thickness; i++)
    {
      color = raised ? color1 : color2;
      LineSet(w, x + i, y + i, x + i, y + height - i, color, XtLINE_SOLID);
      LineSet(w, x + i, y + i, x + width - i, y + i, color, XtLINE_SOLID);

      color = raised ? color2 : color1;
      LineSet(w, x + i, y + height - i, x + width - i, y + height - i, color, XtLINE_SOLID);
      LineSet(w, x + width - i, y + i, x + width - i, y + height - i, color, XtLINE_SOLID);
    }
}

/* This function handles various problems with printing under Windows
 *   using Exceed */
static void
SciPlotPrintf(const char *fmt, ...)
{
  va_list vargs;
  static char lstring[1024]; /* DANGER: Fixed buffer size */

  va_start(vargs, fmt);
  vsprintf(lstring, fmt, vargs);
  va_end(vargs);

  if (lstring[0] != '\0')
    {
#ifdef WIN32
      lprintf("%s", lstring);
#else
      printf("%s", lstring);
#endif
    }
}

/* **************************** Emacs Editing Sequences ***************** */
/* Local Variables: */
/* c-basic-offset: 2 */
/* End: */
