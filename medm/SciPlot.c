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

#define DEBUG_SCIPLOT 0
#define DEBUG_SCIPLOT_VTEXT 0
#define DEBUG_SCIPLOT_TEXT 0
#define DEBUG_SCIPLOT_LINE 0
#define DEBUG_SCIPLOT_AXIS 0
#define DEBUG_SCIPLOT_ALLOC 0
#define DEBUG_SCIPLOT_MLK 0

/* KE: Use SCIPLOT_EPS to avoid roundoff in floor and ceil */
#define SCIPLOT_EPS .0001

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#ifdef MOTIF
#include <Xm/Xm.h>
#endif

#ifdef _MSC_VER
/* Eliminate Microsoft C++ warnings about converting to float */
#pragma warning (disable: 4244 4305)
#endif

#ifdef WIN32
/* Hummingbird extra functions including lprintf
 *   Needs to be included after Intrinsic.h for Exceed 5 */
# include <X11/XlibXtra.h>
/* The following is done in Exceed 6 but not in Exceed 5
 *   Need it to define printf as lprintf for Windows
 *   (as opposed to Console) apps */
# ifdef _WINDOWS
#  ifndef printf
#   define printf lprintf
#  endif
# endif
#endif /* #ifdef WIN32 */

#include "SciPlotP.h"

enum sci_drag_states {NOT_DRAGGING,
		      DRAGGING_NOTHING,
		      DRAGGING_TOP, DRAGGING_BOTTOM,
		      DRAGGING_BOTTOM_AND_LEFT,
		      DRAGGING_LEFT, DRAGGING_RIGHT, DRAGGING_DATA
};

enum sci_draw_axis_type {DRAW_ALL,
			 DRAW_AXIS_ONLY,
			 DRAW_NO_LABELS
};

#define SCIPLOT_ZOOM_FACTOR 0.25

#define offset(field) XtOffsetOf(SciPlotRec, plot.field)
static XtResource resources[] =
{
  {XtNchartType, XtCMargin, XtRInt, sizeof(int),
    offset(ChartType), XtRImmediate, (XtPointer) XtCARTESIAN},
  {XtNdegrees, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(Degrees), XtRImmediate, (XtPointer) True},
  {XtNdrawMajor, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(DrawMajor), XtRImmediate, (XtPointer) True},
  {XtNdrawMajorTics, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(DrawMajorTics), XtRImmediate, (XtPointer) True},
  {XtNdrawMinor, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(DrawMinor), XtRImmediate, (XtPointer) True},
  {XtNdrawMinorTics, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(DrawMinorTics), XtRImmediate, (XtPointer) True},
  {XtNmonochrome, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(Monochrome), XtRImmediate, (XtPointer) False},
  {XtNshowLegend, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(ShowLegend), XtRImmediate, (XtPointer) True},
  {XtNshowTitle, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(ShowTitle), XtRImmediate, (XtPointer) True},
  {XtNshowXLabel, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(ShowXLabel), XtRImmediate, (XtPointer) True},
  {XtNshowYLabel, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(ShowYLabel), XtRImmediate, (XtPointer) True},
  {XtNxLabel, XtCString, XtRString, sizeof(String),
    offset(TransientXLabel), XtRString, "X Axis"},
  {XtNyLabel, XtCString, XtRString, sizeof(String),
    offset(TransientYLabel), XtRString, "Y Axis"},
  {XtNplotTitle, XtCString, XtRString, sizeof(String),
    offset(TransientPlotTitle), XtRString, "Plot"},
  {XtNmargin, XtCMargin, XtRInt, sizeof(int),
    offset(Margin), XtRImmediate, (XtPointer) 5},
  {XtNtitleMargin, XtCMargin, XtRInt, sizeof(int),
    offset(TitleMargin), XtRImmediate, (XtPointer) 16},
  {XtNlegendLineSize, XtCMargin, XtRInt, sizeof(int),
    offset(LegendLineSize), XtRImmediate, (XtPointer) 16},
  {XtNdefaultMarkerSize, XtCMargin, XtRInt, sizeof(int),
    offset(DefaultMarkerSize), XtRImmediate, (XtPointer) 3},
  {XtNlegendMargin, XtCMargin, XtRInt, sizeof(int),
    offset(LegendMargin), XtRImmediate, (XtPointer) 3},
  {XtNlegendThroughPlot, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(LegendThroughPlot), XtRImmediate, (XtPointer) False},
  {XtNtitleFont, XtCMargin, XtRInt, sizeof(int),
    offset(TitleFont), XtRImmediate, (XtPointer) (XtFONT_HELVETICA | 24)},
  {XtNlabelFont, XtCMargin, XtRInt, sizeof(int),
    offset(LabelFont), XtRImmediate, (XtPointer) (XtFONT_TIMES | 18)},
  {XtNaxisFont, XtCMargin, XtRInt, sizeof(int),
    offset(AxisFont), XtRImmediate, (XtPointer) (XtFONT_TIMES | 10)},
  {XtNxAutoScale, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(XAutoScale), XtRImmediate, (XtPointer) True},
  {XtNyAutoScale, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(YAutoScale), XtRImmediate, (XtPointer) True},
  {XtNxAxisNumbers, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(XAxisNumbers), XtRImmediate, (XtPointer) True},
  {XtNyAxisNumbers, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(YAxisNumbers), XtRImmediate, (XtPointer) True},
  {XtNxLog, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(XLog), XtRImmediate, (XtPointer) False},
  {XtNyLog, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(YLog), XtRImmediate, (XtPointer) False},
  {XtNxOrigin, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(XOrigin), XtRImmediate, (XtPointer) False},
  {XtNyOrigin, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(YOrigin), XtRImmediate, (XtPointer) False},
  {XtNyNumbersHorizontal, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(YNumHorz), XtRImmediate, (XtPointer) True},
  {XtNdragX, XtCBoolean, XtRBoolean, sizeof(Boolean),
   offset(DragX), XtRImmediate, (XtPointer) True},
  {XtNdragY, XtCBoolean, XtRBoolean, sizeof(Boolean),
   offset(DragY), XtRImmediate, (XtPointer) False},
  {XtNtrackPointer, XtCBoolean, XtRBoolean, sizeof(Boolean),
   offset(TrackPointer), XtRImmediate, (XtPointer) False},
  {XtNxNoCompMinMax, XtCBoolean, XtRBoolean, sizeof(Boolean),
   offset(XNoCompMinMax), XtRImmediate, (XtPointer) False},
  {XtNdragXCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
   offset(drag_x_callback), XtRCallback, NULL},
  {XtNdragYCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
   offset(drag_y_callback), XtRCallback, NULL},
  {XtNpointerValCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
   offset(pointer_val_callback), XtRCallback, NULL},
  {XtNbtn1ClickCallback, XtCCallback, XtRCallback, sizeof(XtPointer),
   offset(btn1_callback), XtRCallback, NULL},
  {XtNxFixedLeftRight, XtCBoolean, XtRBoolean, sizeof(Boolean),
    offset(XFixedLR), XtRImmediate, (XtPointer) False},
  {XtNxLeftSpace,  XtCMargin, XtRInt, sizeof(int),
   offset(XLeftSpace), XtRImmediate, (XtPointer) (100)},
  {XtNxRightSpace,  XtCMargin, XtRInt, sizeof(int),
   offset(XRightSpace), XtRImmediate, (XtPointer) (100)},
/* KE: (From XmFrame.c) */
#ifdef MOTIF
  {XmNshadowType, XmCShadowType, XmRShadowType, sizeof(unsigned char),
   offset(ShadowType), XmRImmediate, (XtPointer)XmINVALID_DIMENSION},
#endif
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
static void Redisplay (Widget w, XEvent *event, Region region);
static void Resize (Widget w);
static Boolean SetValues (Widget current,
			  Widget request, Widget new,
			  ArgList args, Cardinal *nargs);
static void GetValuesHook (Widget w, ArgList args,
			   Cardinal *num_args);
static void Initialize (Widget treq, Widget tnew,
			ArgList args, Cardinal *num);
static void Realize (Widget aw, XtValueMask * value_mask,
		     XSetWindowAttributes * attributes);
static void Destroy (Widget w);

static void ComputeAll (SciPlotWidget w, int type);
static void ComputeAllDimensions (SciPlotWidget w, int type);
static void DrawAll (SciPlotWidget w);
static void ItemDrawAll (SciPlotWidget w);
static void ItemDraw (SciPlotWidget w, SciPlotItem *item);
static void EraseAll (SciPlotWidget w);
static void FontInit (SciPlotWidget w, SciPlotFont *pf);
static int ColorStore (SciPlotWidget w, Pixel color);
static int FontStore (SciPlotWidget w, int flag);
static int FontnumReplace (SciPlotWidget w, int fontnum, int flag);
static void SciPlotPrintf(const char *fmt, ...);

/* translation manager routines */
static void sciPlotMotionAP (SciPlotWidget w,
  XEvent *event, char *args, int n_args);
static void sciPlotBtnUpAP (SciPlotWidget w,
  XEvent *event, char *args, int n_args);
static void sciPlotClick (SciPlotWidget w,
  XEvent *event, char *args, int n_args);
static void sciPlotTrackPointer (SciPlotWidget w,
  XEvent *event, char *args, int n_args);

/* KE: */
#ifdef MOTIF
static void DrawShadow (SciPlotWidget w, Boolean raised, real x, real y,
  real width, real height);
#endif

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
    {"TrackPointer", (XtActionProc)sciPlotTrackPointer}
};


SciPlotClassRec sciplotClassRec =
{
  {
    /* core_class fields        */
#ifdef MOTIF
    /* superclass               */ (WidgetClass) & xmPrimitiveClassRec,
#else
    /* superclass               */ (WidgetClass) & widgetClassRec,
#endif
    /* class_name               */ "SciPlot",
    /* widget_size              */ sizeof(SciPlotRec),
    /* class_initialize         */ NULL,
    /* class_part_initialize    */ NULL,
    /* class_inited             */ False,
    /* initialize               */ Initialize,
    /* initialize_hook          */ NULL,
    /* realize                  */ Realize,
    /* actions                  */ sciPlotActionsList,
    /* num_actions              */ XtNumber (sciPlotActionsList),
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
    /* extension                */ NULL
  },
#ifdef MOTIF
  {
    /* primitive_class fields   */
    /* border_highlight         */ (XtWidgetProc)_XtInherit,
    /* border_unhighligh        */ (XtWidgetProc)_XtInherit,
    /* translations             */ XtInheritTranslations,
    /* arm_and_activate         */ (XtActionProc)_XtInherit,
    /* syn_resources            */ NULL,
    /* num_syn_resources        */ 0,
    /* extension                */ NULL
  },
#endif
  {
    /* plot_class fields        */
    /* dummy                    */ 0
    /* (some stupid compilers barf on empty structures) */
  }
};

WidgetClass sciplotWidgetClass = (WidgetClass) & sciplotClassRec;

static void
Initialize(Widget treq, Widget tnew, ArgList args, Cardinal *num)
{
  SciPlotWidget new;

  new = (SciPlotWidget) tnew;

  new->plot.plotlist = NULL;
  new->plot.alloc_plotlist = 0;
  new->plot.num_plotlist = 0;

  new->plot.alloc_drawlist = NUMPLOTITEMALLOC;
  new->plot.drawlist = (SciPlotItem *) XtCalloc(new->plot.alloc_drawlist,
    sizeof(SciPlotItem));
  new->plot.num_drawlist = 0;

  new->plot.cmap = DefaultColormap(XtDisplay(new),
				   DefaultScreen(XtDisplay(new)));

  new->plot.xlabel = (char *) XtMalloc(strlen(new->plot.TransientXLabel) + 1);
  strcpy(new->plot.xlabel, new->plot.TransientXLabel);
  new->plot.ylabel = (char *) XtMalloc(strlen(new->plot.TransientYLabel) + 1);
  strcpy(new->plot.ylabel, new->plot.TransientYLabel);
  new->plot.plotTitle = (char *) XtMalloc(strlen(new->plot.TransientPlotTitle) + 1);
  strcpy(new->plot.plotTitle, new->plot.TransientPlotTitle);
  new->plot.TransientXLabel=NULL;
  new->plot.TransientYLabel=NULL;
  new->plot.TransientPlotTitle=NULL;

  new->plot.colors = NULL;
  new->plot.num_colors = 0;
  new->plot.fonts = NULL;
  new->plot.num_fonts = 0;

  new->plot.update = FALSE;
  new->plot.UserMin.x = new->plot.UserMin.y = 0.0;
  new->plot.UserMax.x = new->plot.UserMax.y = 10.0;

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
#ifdef MOTIF
/* KE: Primitive should keep track of colors and GC.  We don't need to
 * do that. */
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
#endif

  mask = GCLineStyle | GCLineWidth | GCFillStyle | GCForeground | GCBackground;
  new->plot.defaultGC = XCreateGC(XtDisplay(new),XtWindow(new), mask, &values);
/* Eliminate events that we do not handle anyway */
  XSetGraphicsExposures(XtDisplay(new), new->plot.defaultGC, False);

  values.foreground = colorsave;
  values.line_style = LineOnOffDash;
  new->plot.dashGC = XCreateGC(XtDisplay(new),XtWindow(new), mask, &values);
/* Eliminate events that we do not handle anyway */
  XSetGraphicsExposures(XtDisplay(new), new->plot.dashGC, False);
}

static void
Realize(Widget aw, XtValueMask * value_mask, XSetWindowAttributes * attributes)
{
  SciPlotWidget w = (SciPlotWidget) aw;

#define	superclass	(&widgetClassRec)
  (*superclass->core_class.realize) (aw, value_mask, attributes);
#undef	superclass

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
  XtFree((char *) w->plot.xlabel);
  XtFree((char *) w->plot.ylabel);
  XtFree((char *) w->plot.plotTitle);

  for (i = 0; i < w->plot.num_fonts; i++) {
    pf = &w->plot.fonts[i];
    XFreeFont(XtDisplay((Widget) w), pf->font);
  }
  XtFree((char *) w->plot.fonts);

  XtFree((char *) w->plot.colors);

#if DEBUG_SCIPLOT_ALLOC
  SciPlotPrintf("Destroy: alloc_plotlist=%d num_plotlist=%d\n",
    w->plot.alloc_plotlist,w->plot.num_plotlist);
  for (i = 0; i < w->plot.alloc_plotlist; i++) {
    p = w->plot.plotlist + i;
    SciPlotPrintf("  List %d: allocated=%d legend=%x markertext=%x\n",
      i, p->allocated, p->legend, p->markertext);
  }
#endif

  for (i = 0; i < w->plot.alloc_plotlist; i++) {
    p = w->plot.plotlist + i;
    if (p->allocated > 0)
      XtFree((char *) p->data);
    if (p->legend)
      XtFree(p->legend);
    if (p->markertext) {
      for (j = 0; j < p->number; j++) {
	if (p->markertext[j])
	  XtFree (p->markertext[j]);
      }
    }
  }
  if (w->plot.alloc_plotlist > 0)
    XtFree((char *) w->plot.plotlist);

  EraseAll(w);
  XtFree((char *) w->plot.drawlist);
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
  else if (current->plot.YLog != new->plot.YLog)
    redisplay = TRUE;
  else if (current->plot.XOrigin != new->plot.XOrigin)
    redisplay = TRUE;
  else if (current->plot.YOrigin != new->plot.YOrigin)
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
  else if (current->plot.ShowLegend != new->plot.ShowLegend)
    redisplay = TRUE;
  else if (current->plot.ShowTitle != new->plot.ShowTitle)
    redisplay = TRUE;
  else if (current->plot.ShowXLabel != new->plot.ShowXLabel)
    redisplay = TRUE;
  else if (current->plot.ShowYLabel != new->plot.ShowYLabel)
    redisplay = TRUE;
  else if (current->plot.ShowTitle != new->plot.ShowTitle)
    redisplay = TRUE;
  else if (current->plot.Monochrome != new->plot.Monochrome)
    redisplay = TRUE;
  else if (current->plot.XFixedLR != new->plot.XFixedLR)
    redisplay = TRUE;
  else if ((current->plot.XLeftSpace != new->plot.XLeftSpace ||
	   current->plot.XRightSpace != new->plot.XRightSpace) &&
	   new->plot.XFixedLR)
    redisplay = TRUE;


  if (new->plot.TransientXLabel) {
    if (current->plot.TransientXLabel != new->plot.TransientXLabel ||
        strcmp(new->plot.TransientXLabel,current->plot.xlabel)!=0) {
      redisplay = TRUE;
      XtFree(current->plot.xlabel);
      new->plot.xlabel = (char *) XtMalloc(strlen(new->plot.TransientXLabel) + 1);
      strcpy(new->plot.xlabel, new->plot.TransientXLabel);
      new->plot.TransientXLabel=NULL;
    }
  }
  if (new->plot.TransientYLabel) {
    if (current->plot.TransientYLabel != new->plot.TransientYLabel ||
        strcmp(new->plot.TransientYLabel,current->plot.ylabel)!=0) {
      redisplay = TRUE;
      XtFree(current->plot.ylabel);
      new->plot.ylabel = (char *) XtMalloc(strlen(new->plot.TransientYLabel) + 1);
      strcpy(new->plot.ylabel, new->plot.TransientYLabel);
      new->plot.TransientYLabel=NULL;
    }
  }
  if (new->plot.TransientPlotTitle) {
    if (current->plot.TransientPlotTitle != new->plot.TransientPlotTitle ||
        strcmp(new->plot.TransientPlotTitle,current->plot.plotTitle)!=0) {
      redisplay = TRUE;
      XtFree(current->plot.plotTitle);
      new->plot.plotTitle = (char *) XtMalloc(strlen(new->plot.TransientPlotTitle) + 1);
      strcpy(new->plot.plotTitle, new->plot.TransientPlotTitle);
      new->plot.TransientPlotTitle=NULL;
    }
  }

  if (current->plot.AxisFont != new->plot.AxisFont) {
    redisplay = TRUE;
    FontnumReplace(new, new->plot.axisFont, new->plot.AxisFont);
  }
  if (current->plot.TitleFont != new->plot.TitleFont) {
    redisplay = TRUE;
    FontnumReplace(new, new->plot.titleFont, new->plot.TitleFont);
  }
  if (current->plot.LabelFont != new->plot.LabelFont) {
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

  for (i=0; i < (int)*num_args; i++) {
    loc=(char **)args[i].value;
    if (strcmp(args[i].name,XtNplotTitle)==0)
      *loc=w->plot.plotTitle;
    else if (strcmp(args[i].name,XtNxLabel)==0)
      *loc=w->plot.xlabel;
    else if (strcmp(args[i].name,XtNyLabel)==0)
      *loc=w->plot.ylabel;

  }
}


static void
Redisplay(Widget ws, XEvent *event, Region region)
  /* KE: event, region not used */
{
  SciPlotWidget w = (SciPlotWidget)ws;
  if (!XtIsRealized((Widget)w))
    return;

  if (w->plot.update) {
    Resize(ws);
    w->plot.update = FALSE;
  }
  else {
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
  if (w->plot.XNoCompMinMax)
    ComputeAll(w, NO_COMPUTE_MIN_MAX_X);
  else
    ComputeAll(w, COMPUTE_MIN_MAX);
  DrawAll(w);
}


/*
 * Private SciPlot utility functions
 */


static int
ColorStore (SciPlotWidget w, Pixel color)
{
    int i;

  /* Check if it is there */
    for(i=0; i < w->plot.num_colors; i++) {
	if(w->plot.colors[i] == color) return i;
    }

  /* Not found, add it */
    w->plot.num_colors++;
    w->plot.colors = (Pixel *) XtRealloc((char *) w->plot.colors,
      sizeof(Pixel) * w->plot.num_colors);
    w->plot.colors[w->plot.num_colors - 1] = color;
    return w->plot.num_colors - 1;
}

static void
FontnumStore (SciPlotWidget w, int fontnum, int flag)
{
  SciPlotFont *pf;
  int fontflag, sizeflag, attrflag;

  pf = &w->plot.fonts[fontnum];

  fontflag = flag & XtFONT_NAME_MASK;
  sizeflag = flag & XtFONT_SIZE_MASK;
  attrflag = flag & XtFONT_ATTRIBUTE_MASK;

  switch (fontflag) {
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

  switch (attrflag) {
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
FontnumReplace (SciPlotWidget w, int fontnum, int flag)
{
  SciPlotFont *pf;

  pf = &w->plot.fonts[fontnum];
  XFreeFont(XtDisplay(w), pf->font);

  FontnumStore(w, fontnum, flag);

  return fontnum;
}

static int
FontStore (SciPlotWidget w, int flag)
{
  int fontnum;

  w->plot.num_fonts++;
  w->plot.fonts = (SciPlotFont *) XtRealloc((char *) w->plot.fonts,
    sizeof(SciPlotFont) * w->plot.num_fonts);
  fontnum = w->plot.num_fonts - 1;

  FontnumStore(w, fontnum, flag);

  return fontnum;
}

static SciPlotFontDesc *
FontDescLookup (int flag)
{
  SciPlotFontDesc *pfd;

  pfd = font_desc_table;
  while (pfd->flag >= 0) {
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
FontnumPostScriptString (SciPlotWidget w, int fontnum, char *str)
{
  char temp[128];
  int flag, bold, italic;
  SciPlotFontDesc *pfd;

  flag = w->plot.fonts[fontnum].id;
  pfd = FontDescLookup(flag);
  if (pfd) {
    strcpy(temp, pfd->PostScript);
    bold = False;
    italic = False;
    if (flag & XtFONT_BOLD) {
      bold = True;
      strcat(temp, "-Bold");
    }
    if (flag & XtFONT_ITALIC) {
      italic = True;
      if (!bold)
	strcat(temp, "-");
      if (pfd->PSUsesOblique)
	strcat(temp, "Oblique");
      else
	strcat(temp, "Italic");
    }
    if (!bold && !italic && pfd->PSUsesRoman) {
      strcat(temp, "-Roman");
    }

    sprintf(str, "/%s findfont %d scalefont",
      temp,
      (flag & XtFONT_SIZE_MASK));
  }
  else
    sprintf(str, "/Courier findfont 10 scalefont");
}

static void
FontX11String (int flag, char *str)
{
  SciPlotFontDesc *pfd;

  pfd = FontDescLookup(flag);
  if (pfd) {
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
FontInit (SciPlotWidget w, SciPlotFont *pf)
{
  char str[256], **list;
  int num;

  FontX11String(pf->id, str);
  list = XListFonts(XtDisplay(w), str, 100, &num);
#if DEBUG_SCIPLOT
  if (1) {
    int i;

    i = 0;
    while (i < num) {
      SciPlotPrintf("FontInit: Found font: %s\n", list[i]);
      i++;
    }
  }
#endif
  if (num <= 0) {
    pf->id &= ~XtFONT_ATTRIBUTE_MASK;
    pf->id |= XtFONT_ATTRIBUTE_DEFAULT;
    FontX11String(pf->id, str);
    list = XListFonts(XtDisplay(w), str, 100, &num);
#if DEBUG_SCIPLOT
    if (1) {
      int i;

      i = 0;
      while (i < num) {
	SciPlotPrintf("FontInit: Attr reset: found: %s\n", list[i]);
	i++;
      }
    }
#endif
  }
  if (num <= 0) {
    pf->id &= ~XtFONT_NAME_MASK;
    pf->id |= XtFONT_NAME_DEFAULT;
    FontX11String(pf->id, str);
    list = XListFonts(XtDisplay(w), str, 100, &num);
#if DEBUG_SCIPLOT
    if (1) {
      int i;

      i = 0;
      while (i < num) {
	SciPlotPrintf("FontInit: Name reset: found: %s\n", list[i]);
	i++;
      }
    }
#endif
  }
  if (num <= 0) {
    pf->id &= ~XtFONT_SIZE_MASK;
    pf->id |= XtFONT_SIZE_DEFAULT;
    FontX11String(pf->id, str);
    list = XListFonts(XtDisplay(w), str, 100, &num);
#if DEBUG_SCIPLOT
    if (1) {
      int i;

      i = 0;
      while (i < num) {
	SciPlotPrintf("FontInit: Size reset: found: %s\n", list[i]);
	i++;
      }
    }
#endif
  }
#if DEBUG_SCIPLOT
  if (1) {
    XFontStruct *f;
    int i;

    i = 0;
    while (i < num) {
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
  if (num > 0) {
  /* Use the first one in the list */
  /* KE: Used to use str instead of list[0] but this failed on Linux
   * Enterprise for some unknown reason */
    pf->font = XLoadQueryFont(XtDisplay(w), list[0]);
    XFreeFontNames(list);
  } else {
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
FontFromFontnum (SciPlotWidget w, int fontnum)
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
  return (real) (f->max_bounds.ascent + f->max_bounds.descent);
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
  return (real) (f->max_bounds.descent);
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
  return (real) (f->max_bounds.ascent);
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
  return (real) XTextWidth(f, c, strlen(c));
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
_ListNew (SciPlotWidget w)
{
  int index;
  SciPlotList *p;
  Boolean found;

/* First check to see if there is any free space in the index */
  found = FALSE;
  for (index = 0; index < w->plot.num_plotlist; index++) {
    p = w->plot.plotlist + index;
    if (!p->used) {
      found = TRUE;
      break;
    }
  }

/* If no space is found, increase the size of the index */
  if (!found) {
    w->plot.num_plotlist++;
    if (w->plot.alloc_plotlist == 0) {
      w->plot.alloc_plotlist = NUMPLOTLINEALLOC;
      w->plot.plotlist = (SciPlotList *) XtCalloc(w->plot.alloc_plotlist, sizeof(SciPlotList));
      if (!w->plot.plotlist) {
	SciPlotPrintf("Can't calloc memory for SciPlotList\n");
	exit(1);
      }
      w->plot.alloc_plotlist = NUMPLOTLINEALLOC;
    }
    else if (w->plot.num_plotlist > w->plot.alloc_plotlist) {
      w->plot.alloc_plotlist += NUMPLOTLINEALLOC;
      w->plot.plotlist = (SciPlotList *) XtRealloc((char *) w->plot.plotlist,
	w->plot.alloc_plotlist * sizeof(SciPlotList));
      if (!w->plot.plotlist) {
	SciPlotPrintf("Can't realloc memory for SciPlotList\n");
	exit(1);
      }
    /* KE: XtRealloc does not zero memory as does the original XtCalloc
     *   This will cause problems in Destroy for data, legend, markertest, etc.*/
	p = w->plot.plotlist +  w->plot.alloc_plotlist - NUMPLOTLINEALLOC;
	memset(p, '\0', NUMPLOTLINEALLOC * sizeof(SciPlotList));
    }
    index = w->plot.num_plotlist - 1;
    p = w->plot.plotlist + index;
  }

/* KE: Since the lists have been zeroed, only the non-zero ones
 *   really need to be set */
  p->LineStyle = p->LineColor = p->PointStyle = p->PointColor = 0;
  p->number = p->allocated = 0;
  p->data = NULL;
  p->legend = NULL;
  p->draw = p->used = TRUE;
  p->markersize = (real) w->plot.DefaultMarkerSize;
  p->markertext = NULL;

#if DEBUG_SCIPLOT_ALLOC
  SciPlotPrintf("_ListNew: alloc_plotlist=%d num_plotlist=%d\n",
    w->plot.alloc_plotlist,w->plot.num_plotlist);
#endif

  return index;
}

static void
_ListDelete (SciPlotList *p)
{
  int i;

  if (p->markertext) {
    for (i = 0; i < p->number; i++) {
      if (p->markertext[i]) {
	XtFree (p->markertext[i]);
	p->markertext[i] = NULL;
      }
    }
    XtFree ((char *)p->markertext);
    p->markertext = NULL;
  }

  p->draw = p->used = FALSE;
  p->number = p->allocated = 0;
  if (p->data)
    XtFree((char *) p->data);
  p->data = NULL;
  if (p->legend)
    XtFree((char *) p->legend);
  p->legend = NULL;

#if DEBUG_SCIPLOT_ALLOC
  SciPlotPrintf("_ListDelete:\n");
#endif
}

static SciPlotList *
_ListFind (SciPlotWidget w, int id)
{
  SciPlotList *p;

  if ((id >= 0) && (id < w->plot.num_plotlist)) {
    p = w->plot.plotlist + id;
    if (p->used)
      return p;
  }
  return NULL;
}

static void
_ListSetStyle (SciPlotList *p, int pcolor, int pstyle, int lcolor, int lstyle)
{
/* Note!  Do checks in here later on... */

  if (lstyle >= 0)
    p->LineStyle = lstyle;
  if (lcolor >= 0)
    p->LineColor = lcolor;
  if (pstyle >= 0)
    p->PointStyle = pstyle;
  if (pcolor >= 0)
    p->PointColor = pcolor;
}

static void
_ListSetLegend (SciPlotList *p, char *legend)
{
/* Note!  Do checks in here later on... */

  p->legend = (char *) XtMalloc(strlen(legend) + 1);
  strcpy(p->legend, legend);
}

static void
_ListAllocData (SciPlotList *p, int num)
{
  if (p->data) {
    XtFree((char *) p->data);
    p->allocated = 0;
  }
  p->allocated = num + NUMPLOTDATAEXTRA;
  p->data = (realpair *) XtCalloc(p->allocated, sizeof(realpair));
  if (!p->data) {
    p->number = p->allocated = 0;
  }
}

static void
_ListReallocData (SciPlotList *p, int more)
{
  if (!p->data) {
    _ListAllocData(p, more);
  }
  else if (p->number + more > p->allocated) {
    p->allocated += more + NUMPLOTDATAEXTRA;
    p->data = (realpair *) XtRealloc((char *) p->data, p->allocated * sizeof(realpair));
    if (!p->data) {
      p->number = p->allocated = 0;
    }
  }

}

static void
_ListAddReal (SciPlotList *p, int num, real *xlist, real *ylist)
{
  int i;

  _ListReallocData(p, num);
  if (p->data) {
    for (i = 0; i < num; i++) {
      p->data[i + p->number].x = xlist[i];
      p->data[i + p->number].y = ylist[i];
    }
    p->number += num;
  }
}

static void
_ListAddFloat (SciPlotList *p, int num, float *xlist, float *ylist)
{
  int i;

  _ListReallocData(p, num);
  if (p->data) {
    for (i = 0; i < num; i++) {
      p->data[i + p->number].x = xlist[i];
      p->data[i + p->number].y = ylist[i];
    }
    p->number += num;
  }
}

static void
_ListAddDouble (SciPlotList *p, int num, double *xlist, double *ylist)
{
  int i;

  _ListReallocData(p, num);
  if (p->data) {
    for (i = 0; i < num; i++) {
      p->data[i + p->number].x = xlist[i];
      p->data[i + p->number].y = ylist[i];
    }
    p->number += num;
  }
}

static void
_ListSetReal(SciPlotList *p, int num, real *xlist, real *ylist)
{
  if ((!p->data) || (p->allocated < num))
    _ListAllocData(p, num);
  p->number = 0;
  _ListAddReal(p, num, xlist, ylist);
}

static void
_ListSetFloat (SciPlotList *p, int num, float *xlist, float *ylist)
{
  if ((!p->data) || (p->allocated < num))
    _ListAllocData(p, num);
  p->number = 0;
  _ListAddFloat(p, num, xlist, ylist);
}

static void
_ListSetDouble (SciPlotList *p, int num, double *xlist, double *ylist)
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
XDrawVString (Display *display, Window win, GC gc, int x, int y, char *str, int len, XFontStruct *f)
{
  XImage *before, *after;
  char *dest, *source, *source0, *source1;
  int xloop, yloop, xdest, ydest;
  Pixmap pix, rotpix;
  int width, height;
  GC drawGC;

  width = (int) FontTextWidth(f, str);
  height = (int) FontHeight(f);

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

  XDrawImageString(display, pix, drawGC, 0, (int) FontAscent(f),
    str, strlen(str));

  source0 = (char *) calloc((((width + 7) / 8) * height), 1);
  before = XCreateImage(display, DefaultVisual(display, DefaultScreen(display)),
    1, XYPixmap, 0, source0, width, height, 8, 0);
  before->byte_order = before->bitmap_bit_order = MSBFirst;
  XGetSubImage(display, pix, 0, 0, width, height, 1L, XYPixmap, before, 0, 0);

  source1 = (char *) calloc((((height + 7) / 8) * width), 1);
  after = XCreateImage(display, DefaultVisual(display, DefaultScreen(display)),
    1, XYPixmap, 0, source1, height, width, 8, 0);
  after->byte_order = after->bitmap_bit_order = MSBFirst;

  for (yloop = 0; yloop < height; yloop++) {
    for (xloop = 0; xloop < width; xloop++) {
      source = before->data + (xloop / 8) +
	(yloop * before->bytes_per_line);
      if (*source & (128 >> (xloop % 8))) {
	dest = after->data + (yloop / 8) +
	  ((width - 1 - xloop) * after->bytes_per_line);
	*dest |= (128 >> (yloop % 8));
      }
    }
  }

#if DEBUG_SCIPLOT_VTEXT
  if (1) {
    char sourcebit;

    for (yloop = 0; yloop < before->height; yloop++) {
      for (xloop = 0; xloop < before->width; xloop++) {
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

    for (yloop = 0; yloop < after->height; yloop++) {
      for (xloop = 0; xloop < after->width; xloop++) {
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

  xdest = x - (int) FontAscent(f);
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
ItemGetGC (SciPlotWidget w, SciPlotItem *item)
{
  GC gc;
  short color;

  switch (item->kind.any.style) {
  case XtLINE_SOLID:
    gc = w->plot.defaultGC;
    break;
  case XtLINE_DOTTED:
    XSetDashes(XtDisplay(w), w->plot.dashGC, 0, &dots[1],
      (int) dots[0]);
    gc = w->plot.dashGC;
    break;
  case XtLINE_WIDEDOT:
    XSetDashes(XtDisplay(w), w->plot.dashGC, 0, &widedots[1],
      (int) widedots[0]);
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
ItemGetFontGC (SciPlotWidget w, SciPlotItem *item)
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
ItemDraw (SciPlotWidget w, SciPlotItem *item)
{
  XPoint point[8];
  XSegment seg;
  XRectangle rect;
  int i;
  GC gc;

  if (!XtIsRealized((Widget) w))
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
      item->type,SciPlotFALSE,SciPlotPoint,SciPlotLine);
#endif
  switch (item->type) {
  case SciPlotLine:
    seg.x1 = (short) item->kind.line.x1;
    seg.y1 = (short) item->kind.line.y1;
    seg.x2 = (short) item->kind.line.x2;
    seg.y2 = (short) item->kind.line.y2;
    XDrawSegments(XtDisplay(w), XtWindow(w), gc,
      &seg, 1);
    break;
  case SciPlotRect:
    XDrawRectangle(XtDisplay(w), XtWindow(w), gc,
      (int) (item->kind.rect.x),
      (int) (item->kind.rect.y),
      (unsigned int) (item->kind.rect.w),
      (unsigned int) (item->kind.rect.h));
    break;
  case SciPlotFRect:
    XFillRectangle(XtDisplay(w), XtWindow(w), gc,
      (int) (item->kind.rect.x),
      (int) (item->kind.rect.y),
      (unsigned int) (item->kind.rect.w),
      (unsigned int) (item->kind.rect.h));
    XDrawRectangle(XtDisplay(w), XtWindow(w), gc,
      (int) (item->kind.rect.x),
      (int) (item->kind.rect.y),
      (unsigned int) (item->kind.rect.w),
      (unsigned int) (item->kind.rect.h));
    break;
  case SciPlotPoly:
    i = 0;
    while (i < item->kind.poly.count) {
      point[i].x = (int) item->kind.poly.x[i];
      point[i].y = (int) item->kind.poly.y[i];
      i++;
    }
    point[i].x = (int) item->kind.poly.x[0];
    point[i].y = (int) item->kind.poly.y[0];
    XDrawLines(XtDisplay(w), XtWindow(w), gc,
      point, i + 1, CoordModeOrigin);
    break;
  case SciPlotFPoly:
    i = 0;
    while (i < item->kind.poly.count) {
      point[i].x = (int) item->kind.poly.x[i];
      point[i].y = (int) item->kind.poly.y[i];
      i++;
    }
    point[i].x = (int) item->kind.poly.x[0];
    point[i].y = (int) item->kind.poly.y[0];
    XFillPolygon(XtDisplay(w), XtWindow(w), gc,
      point, i + 1, Complex, CoordModeOrigin);
    XDrawLines(XtDisplay(w), XtWindow(w), gc,
      point, i + 1, CoordModeOrigin);
    break;
  case SciPlotCircle:
    XDrawArc(XtDisplay(w), XtWindow(w), gc,
      (int) (item->kind.circ.x - item->kind.circ.r),
      (int) (item->kind.circ.y - item->kind.circ.r),
      (unsigned int) (item->kind.circ.r * 2),
      (unsigned int) (item->kind.circ.r * 2),
      0 * 64, 360 * 64);
    break;
  case SciPlotFCircle:
    XFillArc(XtDisplay(w), XtWindow(w), gc,
      (int) (item->kind.circ.x - item->kind.circ.r),
      (int) (item->kind.circ.y - item->kind.circ.r),
      (unsigned int) (item->kind.circ.r * 2),
      (unsigned int) (item->kind.circ.r * 2),
      0 * 64, 360 * 64);
    break;
  case SciPlotText:
    XDrawString(XtDisplay(w), XtWindow(w), gc,
      (int) (item->kind.text.x), (int) (item->kind.text.y),
      item->kind.text.text,
      (int) item->kind.text.length);
    break;
  case SciPlotVText:
    XDrawVString(XtDisplay(w), XtWindow(w), gc,
      (int) (item->kind.text.x), (int) (item->kind.text.y),
      item->kind.text.text,
      (int) item->kind.text.length,
      FontFromFontnum(w, item->kind.text.font));
    break;
  case SciPlotClipRegion:
    rect.x = (short) item->kind.line.x1;
    rect.y = (short) item->kind.line.y1;
    rect.width = (short) item->kind.line.x2;
    rect.height = (short) item->kind.line.y2;
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
ItemDrawAll (SciPlotWidget w)
{
  SciPlotItem *item;
  int i;

  if (!XtIsRealized((Widget) w))
    return;
  item = w->plot.drawlist;
  i = 0;
  while (i < w->plot.num_drawlist) {
    ItemDraw(w, item);
    i++;
    item++;
  }
}



/*
 * PostScript (r) functions ------------------------------------------------
 *
 */
typedef struct {
  char *command;
  char *prolog;
} PScommands;

static PScommands psc[] =
{
  {"ma", "moveto"},
  {"da", "lineto stroke newpath"},
  {"la", "lineto"},
  {"poly", "closepath stroke newpath"},
  {"fpoly", "closepath fill newpath"},
  {"box", "1 index 0 rlineto 0 exch rlineto neg 0 rlineto closepath stroke newpath"},
  {"fbox", "1 index 0 rlineto 0 exch rlineto neg 0 rlineto closepath fill newpath"},
  {"clipbox", "gsave 1 index 0 rlineto 0 exch rlineto neg 0 rlineto closepath clip newpath"},
  {"unclip", "grestore newpath"},
  {"cr", "0 360 arc stroke newpath"},
  {"fcr", "0 360 arc fill newpath"},
  {"vma", "gsave moveto 90 rotate"},
  {"norm", "grestore"},
  {"solid", "[] 0 setdash"},
  {"dot", "[.25 2] 0 setdash"},
  {"widedot", "[.25 8] 0 setdash"},
  {"rgb", "setrgbcolor"},
  {NULL, NULL}
};

enum PSenums {
  PSmoveto, PSlineto,
  PSpolyline, PSendpoly, PSendfill,
  PSbox, PSfbox,
  PSclipbox, PSunclip,
  PScircle, PSfcircle,
  PSvmoveto, PSnormal,
  PSsolid, PSdot, PSwidedot,
  PSrgb
};

static void
ItemPSDrawAll (SciPlotWidget w, FILE *fd, float yflip, int usecolor)
{
  int i, loopcount;
  SciPlotItem *item;
  XcmsColor currentcolor;
  int previousfont, previousline, currentfont, currentline, previouscolor;

  item = w->plot.drawlist;
  loopcount = 0;
  previousfont = 0;
  previouscolor = -1;
  previousline = XtLINE_SOLID;
  while (loopcount < w->plot.num_drawlist) {

/* 2 switch blocks:  1st sets up defaults, 2nd actually draws things. */
    currentline = previousline;
    currentfont = previousfont;
    switch (item->type) {
    case SciPlotLine:
    case SciPlotCircle:
      currentline = item->kind.any.style;
      break;
    default:
      break;
    }
    if (currentline != XtLINE_NONE) {
      if (currentline != previousline) {
	switch (item->kind.any.style) {
	case XtLINE_SOLID:
	  fprintf(fd, "%s ", psc[PSsolid].command);
	  break;
	case XtLINE_DOTTED:
	  fprintf(fd, "%s ", psc[PSdot].command);
	  break;
	case XtLINE_WIDEDOT:
	  fprintf(fd, "%s ", psc[PSwidedot].command);
	  break;
	}
	previousline = currentline;
      }

      if (usecolor && item->kind.any.color != previouscolor) {

          /* Get Pixel index */
        currentcolor.pixel = w->plot.colors[item->kind.any.color];
#ifdef WIN32
      /* Exceed 5 does not have Xcms routines
       *   Only want to get colors as XcmsFloat=double in range [0.0,1.0]
       *   So use  XQueryColors and convert */
       {
	 XColor xcolor;

	 XQueryColor( XtDisplay(w), w->plot.cmap, &xcolor);
	 currentcolor.spec.RGBi.red = (double)xcolor.red/65535.;
	 currentcolor.spec.RGBi.green = (double)xcolor.green/65535.;
	 currentcolor.spec.RGBi.blue = (double)xcolor.blue/65535.;
	 currentcolor.format = XcmsRGBiFormat;
       }
#else
          /* Get RGBi components [0.0,1.0] */
        XcmsQueryColor( XtDisplay(w), w->plot.cmap, &currentcolor,
          XcmsRGBiFormat );
#endif
          /* output PostScript command */
        fprintf(fd, "%f %f %f %s ", currentcolor.spec.RGBi.red,
          currentcolor.spec.RGBi.green, currentcolor.spec.RGBi.blue,
          psc[PSrgb].command);

        previouscolor=item->kind.any.color;

      }

      switch (item->type) {
      case SciPlotLine:
	fprintf(fd, "%.2f %.2f %s %.2f %.2f %s\n",
	  item->kind.line.x1, yflip - item->kind.line.y1,
	  psc[PSmoveto].command,
	  item->kind.line.x2, yflip - item->kind.line.y2,
	  psc[PSlineto].command);
	break;
      case SciPlotRect:
	fprintf(fd, "%.2f %.2f %s %.2f %.2f %s\n",
	  item->kind.rect.x,
	  yflip - item->kind.rect.y - (item->kind.rect.h - 1.0),
	  psc[PSmoveto].command,
	  item->kind.rect.w - 1.0, item->kind.rect.h - 1.0,
	  psc[PSbox].command);
	break;
      case SciPlotFRect:
	fprintf(fd, "%.2f %.2f %s %.2f %.2f %s\n",
	  item->kind.rect.x,
	  yflip - item->kind.rect.y - (item->kind.rect.h - 1.0),
	  psc[PSmoveto].command,
	  item->kind.rect.w - 1.0, item->kind.rect.h - 1.0,
	  psc[PSfbox].command);
	break;
      case SciPlotPoly:
	fprintf(fd, "%.2f %.2f %s ",
	  item->kind.poly.x[0], yflip - item->kind.poly.y[0],
	  psc[PSmoveto].command);
	for (i = 1; i < item->kind.poly.count; i++) {
	  fprintf(fd, "%.2f %.2f %s ",
	    item->kind.poly.x[i],
	    yflip - item->kind.poly.y[i],
	    psc[PSpolyline].command);
	}
	fprintf(fd, "%s\n", psc[PSendpoly].command);
	break;
      case SciPlotFPoly:
	fprintf(fd, "%.2f %.2f %s ",
	  item->kind.poly.x[0], yflip - item->kind.poly.y[0],
	  psc[PSmoveto].command);
	for (i = 1; i < item->kind.poly.count; i++) {
	  fprintf(fd, "%.2f %.2f %s ",
	    item->kind.poly.x[i],
	    yflip - item->kind.poly.y[i],
	    psc[PSpolyline].command);
	}
	fprintf(fd, "%s\n", psc[PSendfill].command);
	break;
      case SciPlotCircle:
	fprintf(fd, "%.2f %.2f %.2f %s\n",
	  item->kind.circ.x, yflip - item->kind.circ.y,
	  item->kind.circ.r,
	  psc[PScircle].command);
	break;
      case SciPlotFCircle:
	fprintf(fd, "%.2f %.2f %.2f %s\n",
	  item->kind.circ.x, yflip - item->kind.circ.y,
	  item->kind.circ.r,
	  psc[PSfcircle].command);
	break;
      case SciPlotText:
	fprintf(fd, "font-%d %.2f %.2f %s (%s) show\n",
	  item->kind.text.font,
	  item->kind.text.x, yflip - item->kind.text.y,
	  psc[PSmoveto].command,
	  item->kind.text.text);
	break;
      case SciPlotVText:
	fprintf(fd, "font-%d %.2f %.2f %s (%s) show %s\n",
	  item->kind.text.font,
	  item->kind.text.x, yflip - item->kind.text.y,
	  psc[PSvmoveto].command,
	  item->kind.text.text,
	  psc[PSnormal].command);
	break;
      case SciPlotClipRegion:
	fprintf(fd, "%.2f %.2f %s %.2f %.2f %s\n",
	  item->kind.line.x1,
	  yflip - item->kind.line.y1 - item->kind.line.y2,
	  psc[PSmoveto].command,
	  item->kind.line.x2, item->kind.line.y2,
	  psc[PSclipbox].command);
	break;
      case SciPlotClipClear:
	fprintf(fd, "%s\n", psc[PSunclip].command);
	break;
      default:
	break;
      }
    }
    loopcount++;
    item++;
  }
}

FILE*
SciPlotPSCreateHeader (Widget wi, char *filename, float width, float height)
{
  float scale, xoff, yoff, xmax, ymax, yflip, aspect, border, titlefontsize;
  int i;
  PScommands *p;
  char fontname[128];
  SciPlotWidget w = (SciPlotWidget)wi;
  FILE *fd;

  if (!(fd = fopen(filename, "w"))) {
    XtWarning("SciPlotPSCreateHeader: Unable to open postscript file.");
    return 0;
  }

  aspect = width /height;
  border = 36.0;
  if (aspect > (612.0 / 792.0)) {
    scale = (612.0 - (2 * border)) / width;
    xoff = border;
    yoff = (792.0 - (2 * border) - scale * height) / 2.0;
    xmax = xoff + scale * (float) width;
    ymax = yoff + scale * (float) height;
  }
  else {
    scale = (792.0 - (2 * border)) / height;
    yoff = border;
    xoff = (612.0 - (2 * border) - scale * width) / 2.0;
    xmax = xoff + scale * width;
    ymax = yoff + scale * height;
  }
  yflip = height;
  fprintf(fd, "%s\n%s %.2f  %s\n%s %f %f %f %f\n%s\n",
    "%!PS-ADOBE-3.0 EPSF-3.0",
    "%%Creator: SciPlot Widget",
    _SCIPLOT_WIDGET_VERSION,
    "Copyright (c) 1997 Jie Chen",
    "%%BoundingBox:", xoff, yoff, xmax, ymax,
    "%%EndComments");

  p = psc;
  while (p->command) {
    fprintf(fd, "/%s {%s} bind def\n", p->command, p->prolog);
    p++;
  }


  for (i = 0; i < w->plot.num_fonts; i++) {
    FontnumPostScriptString(w, i, fontname);
    fprintf(fd, "/font-%d {%s setfont} bind def\n",
      i, fontname);
  }
  titlefontsize = 10.0;
  fprintf(fd, "/font-title {/%s findfont %f scalefont setfont} bind def\n",
	  "Times-Roman", titlefontsize);
  fprintf(fd, "%f setlinewidth\n", 0.001);
  fprintf(fd, "newpath gsave\n%f %f translate %f %f scale\n",
    xoff, yoff, scale, scale);

  return fd;
}

float
SciPlotPSAdd (Widget wi, FILE* fd, float currht, int usecolor)
{
  SciPlotWidget w = (SciPlotWidget)wi;

  ItemPSDrawAll(w, fd, currht, usecolor);

  return (float)(w->core.height);
}

int
SciPlotPSFinish (FILE *fd, int drawborder, char* titles, int usecolor)
{
  float border = 36.0;
  float titlefontsize = 10.0;

  fprintf(fd, "grestore\n");

  if (drawborder) {
    fprintf(fd, "%.2f %.2f %s %.2f %.2f %s\n",
      border, border,
      psc[PSmoveto].command,
      612.0 - 2.0 * border, 792.0 - 2.0 * border,
      psc[PSbox].command);
  }
  if (titles) {
    char *ptr;
    char buf[256];
    int len, i, j;
    float x, y;

    x = border + titlefontsize;
    y = 792.0 - border - (2.0 * titlefontsize);
    len = strlen(titles);
    ptr = titles;
    i = 0;
    while (i < len) {
      j = 0;
      while ((*ptr != '\n') && (i < len)) {
	if ((*ptr == '(') || (*ptr == ')'))
	  buf[j++] = '\\';
	buf[j++] = *ptr;
	ptr++;
	i++;
      }
      buf[j] = '\0';
      ptr++;
      i++;
      fprintf(fd, "font-title %.2f %.2f %s (%s) show\n",
	x, y, psc[PSmoveto].command, buf);
      y -= titlefontsize * 1.5;
    }
    if (border) {
      y += titlefontsize * 0.5;
      fprintf(fd, "%.2f %.2f %s %.2f %.2f %s\n",
	border, y,
	psc[PSmoveto].command,
	612.0 - border, y,
	psc[PSlineto].command);
    }
  }

  fprintf(fd, "showpage\n");
  fclose(fd);
  return 1;
}


int
SciPlotPSCreateFancy (SciPlotWidget w, char *filename, int drawborder, char *titles, int usecolor)
{
  FILE *fd;
  float scale, xoff, yoff, xmax, ymax, yflip, aspect, border, titlefontsize;
  int i;
  PScommands *p;
  char fontname[128];

  if (!(fd = fopen(filename, "w"))) {
    XtWarning("SciPlotPSCreate: Unable to open postscript file.");
    return 0;
  }
  DrawAll(w);

  aspect = (float) w->core.width / (float) w->core.height;
  border = 36.0;
  if (aspect > (612.0 / 792.0)) {
    scale = (612.0 - (2 * border)) / (float) w->core.width;
    xoff = border;
    yoff = (792.0 - (2 * border) - scale * (float) w->core.height) / 2.0;
    xmax = xoff + scale * (float) w->core.width;
    ymax = yoff + scale * (float) w->core.height;
  }
  else {
    scale = (792.0 - (2 * border)) / (float) w->core.height;
    yoff = border;
    xoff = (612.0 - (2 * border) - scale * (float) w->core.width) / 2.0;
    xmax = xoff + scale * (float) w->core.width;
    ymax = yoff + scale * (float) w->core.height;
  }
  yflip = w->core.height;
  fprintf(fd, "%s\n%s %.2f  %s\n%s %f %f %f %f\n%s\n",
    "%!PS-ADOBE-3.0 EPSF-3.0",
    "%%Creator: SciPlot Widget",
    _SCIPLOT_WIDGET_VERSION,
    "Copyright (c) 1995 Robert W. McMullen",
    "%%BoundingBox:", xoff, yoff, xmax, ymax,
    "%%EndComments");

  p = psc;
  while (p->command) {
    fprintf(fd, "/%s {%s} bind def\n", p->command, p->prolog);
    p++;
  }

  for (i = 0; i < w->plot.num_fonts; i++) {
    FontnumPostScriptString(w, i, fontname);
    fprintf(fd, "/font-%d {%s setfont} bind def\n",
      i, fontname);
  }
  titlefontsize = 10.0;
  fprintf(fd, "/font-title {/%s findfont %f scalefont setfont} bind def\n",
    "Times-Roman", titlefontsize);
  fprintf(fd, "%f setlinewidth\n", 0.001);
  fprintf(fd, "newpath gsave\n%f %f translate %f %f scale\n",
    xoff, yoff, scale, scale);

  ItemPSDrawAll(w, fd, yflip, usecolor);

  fprintf(fd, "grestore\n");

  if (drawborder) {
    fprintf(fd, "%.2f %.2f %s %.2f %.2f %s\n",
      border, border,
      psc[PSmoveto].command,
      612.0 - 2.0 * border, 792.0 - 2.0 * border,
      psc[PSbox].command);
  }
  if (titles) {
    char *ptr;
    char buf[256];
    int len, i, j;
    float x, y;

    x = border + titlefontsize;
    y = 792.0 - border - (2.0 * titlefontsize);
    len = strlen(titles);
    ptr = titles;
    i = 0;
    while (i < len) {
      j = 0;
      while ((*ptr != '\n') && (i < len)) {
	if ((*ptr == '(') || (*ptr == ')'))
	  buf[j++] = '\\';
	buf[j++] = *ptr;
	ptr++;
	i++;
      }
      buf[j] = '\0';
      ptr++;
      i++;
      fprintf(fd, "font-title %.2f %.2f %s (%s) show\n",
	x, y, psc[PSmoveto].command, buf);
      y -= titlefontsize * 1.5;
    }
    if (border) {
      y += titlefontsize * 0.5;
      fprintf(fd, "%.2f %.2f %s %.2f %.2f %s\n",
	border, y,
	psc[PSmoveto].command,
	612.0 - border, y,
	psc[PSlineto].command);
    }
  }

  fprintf(fd, "showpage\n");
  fclose(fd);
  return 1;
}

int
SciPlotPSCreate (Widget wi, char *filename)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi)) {
    XtWarning("SciPlotPSCreate: Not a SciPlot widget.");
    return 1;
  }

  w = (SciPlotWidget) wi;
  return SciPlotPSCreateFancy(w, filename, False, NULL, 0);
}

int
SciPlotPSCreateColor (Widget wi, char *filename)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi)) {
    XtWarning("SciPlotPSCreate: Not a SciPlot widget.");
    return 0;
  }

  w = (SciPlotWidget) wi;
  return SciPlotPSCreateFancy(w, filename, False, NULL, 1);
}


/*
 * Private device independent drawing functions
 */

static void
EraseClassItems (SciPlotWidget w, SciPlotDrawingEnum drawing)
{
  SciPlotItem *item;
  int i;

  if (!XtIsRealized((Widget) w))
    return;
  item = w->plot.drawlist;
  i = 0;
  while (i < w->plot.num_drawlist) {
    if (item->drawing_class == drawing) {
      item->kind.any.color = 0;
      item->kind.any.style = XtLINE_SOLID;
      ItemDraw(w, item);
    }
    i++;
    item++;
  }
}

static void
EraseAllItems (SciPlotWidget w)
{
  SciPlotItem *item;
  int i;

  item = w->plot.drawlist;
  i = 0;
  while (i < w->plot.num_drawlist) {
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
EraseAll (SciPlotWidget w)
{
  EraseAllItems(w);
  if (XtIsRealized((Widget) w))
    XClearWindow(XtDisplay(w), XtWindow(w));
}

static SciPlotItem *
ItemGetNew (SciPlotWidget w)
{
  SciPlotItem *item;

  w->plot.num_drawlist++;
  if (w->plot.num_drawlist >= w->plot.alloc_drawlist) {
    w->plot.alloc_drawlist += NUMPLOTITEMEXTRA;
    w->plot.drawlist = (SciPlotItem *) XtRealloc((char *) w->plot.drawlist,
      w->plot.alloc_drawlist * sizeof(SciPlotItem));
    if (!w->plot.drawlist) {
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
  item->kind.any.color = (short) color;
  item->kind.any.style = (short) style;
  item->kind.line.x1 = (real) x1;
  item->kind.line.y1 = (real) y1;
  item->kind.line.x2 = (real) x2;
  item->kind.line.y2 = (real) y2;
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
  item->kind.any.color = (short) color;
  item->kind.any.style = (short) style;
  item->kind.rect.x = (real) x;
  item->kind.rect.y = (real) y;
  item->kind.rect.w = (real) width;
  item->kind.rect.h = (real) height;
  item->type = SciPlotRect;
  ItemDraw(w, item);
}

static void
FilledRectSet (SciPlotWidget w, real x1, real y1, real x2, real y2, int color, int style)
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
  item->kind.any.color = (short) color;
  item->kind.any.style = (short) style;
  item->kind.rect.x = (real) x;
  item->kind.rect.y = (real) y;
  item->kind.rect.w = (real) width;
  item->kind.rect.h = (real) height;
  item->type = SciPlotFRect;
  ItemDraw(w, item);
}

static void
TriSet (SciPlotWidget w, real x1, real y1, real x2, real y2, real x3, real y3, int color, int style)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short) color;
  item->kind.any.style = (short) style;
  item->kind.poly.count = 3;
  item->kind.poly.x[0] = (real) x1;
  item->kind.poly.y[0] = (real) y1;
  item->kind.poly.x[1] = (real) x2;
  item->kind.poly.y[1] = (real) y2;
  item->kind.poly.x[2] = (real) x3;
  item->kind.poly.y[2] = (real) y3;
  item->type = SciPlotPoly;
  ItemDraw(w, item);
}

static void
FilledTriSet (SciPlotWidget w, real x1, real y1, real x2, real y2, real x3, real y3, int color, int style)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short) color;
  item->kind.any.style = (short) style;
  item->kind.poly.count = 3;
  item->kind.poly.x[0] = (real) x1;
  item->kind.poly.y[0] = (real) y1;
  item->kind.poly.x[1] = (real) x2;
  item->kind.poly.y[1] = (real) y2;
  item->kind.poly.x[2] = (real) x3;
  item->kind.poly.y[2] = (real) y3;
  item->type = SciPlotFPoly;
  ItemDraw(w, item);
}

static void
QuadSet (SciPlotWidget w, real x1, real y1, real x2, real y2, real x3, real y3, real x4, real y4, int color, int style)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short) color;
  item->kind.any.style = (short) style;
  item->kind.poly.count = 4;
  item->kind.poly.x[0] = (real) x1;
  item->kind.poly.y[0] = (real) y1;
  item->kind.poly.x[1] = (real) x2;
  item->kind.poly.y[1] = (real) y2;
  item->kind.poly.x[2] = (real) x3;
  item->kind.poly.y[2] = (real) y3;
  item->kind.poly.x[3] = (real) x4;
  item->kind.poly.y[3] = (real) y4;
  item->type = SciPlotPoly;
  ItemDraw(w, item);
}

static void
FilledQuadSet (SciPlotWidget w, real x1, real y1, real x2, real y2, real x3, real y3, real x4, real y4, int color, int style)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short) color;
  item->kind.any.style = (short) style;
  item->kind.poly.count = 4;
  item->kind.poly.x[0] = (real) x1;
  item->kind.poly.y[0] = (real) y1;
  item->kind.poly.x[1] = (real) x2;
  item->kind.poly.y[1] = (real) y2;
  item->kind.poly.x[2] = (real) x3;
  item->kind.poly.y[2] = (real) y3;
  item->kind.poly.x[3] = (real) x4;
  item->kind.poly.y[3] = (real) y4;
  item->type = SciPlotFPoly;
  ItemDraw(w, item);
}

static void
CircleSet (SciPlotWidget w, real x, real y, real r, int color, int style)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short) color;
  item->kind.any.style = (short) style;
  item->kind.circ.x = (real) x;
  item->kind.circ.y = (real) y;
  item->kind.circ.r = (real) r;
  item->type = SciPlotCircle;
  ItemDraw(w, item);
}

static void
FilledCircleSet (SciPlotWidget w, real x, real y, real r, int color, int style)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short) color;
  item->kind.any.style = (short) style;
  item->kind.circ.x = (real) x;
  item->kind.circ.y = (real) y;
  item->kind.circ.r = (real) r;
  item->type = SciPlotFCircle;
  ItemDraw(w, item);
}

static void
TextSet (SciPlotWidget w, real x, real y, char *text, int color, int font)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short) color;
  item->kind.any.style = 0;
  item->kind.text.x = (real) x;
  item->kind.text.y = (real) y;
  item->kind.text.length = strlen(text);
  item->kind.text.text = XtMalloc((int) item->kind.text.length + 1);
#if DEBUG_SCIPLOT_MLK
  SciPlotPrintf("TextSet: item->kind.text.text=%p\n",item->kind.text.text);
#endif
  item->kind.text.font = font;
  strcpy(item->kind.text.text, text);
  item->type = SciPlotText;
  ItemDraw(w, item);
#if DEBUG_SCIPLOT_TEXT
  if (1) {
    real x1, y1;

    y -= FontnumAscent(w, font);
    y1 = y + FontnumHeight(w, font) - 1.0;
    x1 = x + FontnumTextWidth(w, font, text) - 1.0;
    RectSet(w, x, y, x1, y1, color, XtLINE_SOLID);
  }
#endif
}

static void
TextCenter (SciPlotWidget w, real x, real y, char *text, int color, int font)
{
  x -= FontnumTextWidth(w, font, text) / 2.0;
  y += FontnumHeight(w, font) / 2.0 - FontnumDescent(w, font);
  TextSet(w, x, y, text, color, font);
}

static void
VTextSet (SciPlotWidget w, real x, real y, char *text, int color, int font)
{
  SciPlotItem *item;

  item = ItemGetNew(w);
  item->kind.any.color = (short) color;
  item->kind.any.style = 0;
  item->kind.text.x = (real) x;
  item->kind.text.y = (real) y;
  item->kind.text.length = strlen(text);
  item->kind.text.text = XtMalloc((int) item->kind.text.length + 1);
#if DEBUG_SCIPLOT_MLK
  SciPlotPrintf("VTextSet: item->kind.text.text=%p\n",item->kind.text.text);
#endif
  item->kind.text.font = font;
  strcpy(item->kind.text.text, text);
  item->type = SciPlotVText;
  ItemDraw(w, item);
#if DEBUG_SCIPLOT_TEXT
  if (1) {
    real x1, y1;

    x += FontnumDescent(w, font);
    x1 = x - FontnumHeight(w, font) - 1.0;
    y1 = y - FontnumTextWidth(w, font, text) - 1.0;
    RectSet(w, x, y, x1, y1, color, XtLINE_SOLID);
  }
#endif
}

static void
VTextCenter (SciPlotWidget w, real x, real y, char *text, int color, int font)
{
  x += FontnumHeight(w, font) / 2.0 - FontnumDescent(w, font);
  y += FontnumTextWidth(w, font, text) / 2.0;
  VTextSet(w, x, y, text, color, font);
}

static void
ClipSet (SciPlotWidget w)
{
  SciPlotItem *item;

  if (w->plot.ChartType == XtCARTESIAN) {
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
      item->kind.line.y2
      );
#endif
    item->type = SciPlotClipRegion;
    ItemDraw(w, item);
  }
}

static void
ClipClear (SciPlotWidget w)
{
  SciPlotItem *item;

  if (w->plot.ChartType == XtCARTESIAN) {
    item = ItemGetNew(w);
    item->kind.any.style = XtLINE_SOLID;
    item->kind.any.color = 1;
    item->type = SciPlotClipClear;
    ItemDraw(w, item);
  }
}


/*
 * Private data point to screen location converters
 */

static real
PlotX (SciPlotWidget w, real xin)
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
PlotY (SciPlotWidget w, real yin)
{
  real yout;

  if (w->plot.YLog)
    yout = w->plot.y.Origin + w->plot.y.Size -
      ((log10(yin) - log10(w->plot.y.DrawOrigin)) *
      (w->plot.y.Size / w->plot.y.DrawSize));
  else
    yout = w->plot.y.Origin + w->plot.y.Size -
      ((yin - w->plot.y.DrawOrigin) *
      (w->plot.y.Size / w->plot.y.DrawSize));
  return yout;
}

static void
PlotRTRadians (SciPlotWidget w, real r, real t, real *xout, real *yout)
{
  *xout = w->plot.x.Center + (r * (real) cos(t) /
    w->plot.PolarScale * w->plot.x.Size / 2.0);
  *yout = w->plot.y.Center + (-r * (real) sin(t) /
    w->plot.PolarScale * w->plot.x.Size / 2.0);
}

static void
PlotRTDegrees (SciPlotWidget w, real r, real t, real *xout, real *yout)
{
  t *= (float)DEG2RAD;
  PlotRTRadians(w, r, t, xout, yout);
}

static void
PlotRT (SciPlotWidget w, real r, real t, real *xout, real *yout)
{
  if (w->plot.Degrees)
    t *= (float)DEG2RAD;
  PlotRTRadians(w, r, t, xout, yout);
}


/*
 * Private calculation utilities for axes
 */

#define NUMBER_MINOR	8
#define MAX_MAJOR	8
static float CAdeltas[8] =
{0.1, 0.2, 0.25, 0.5, 1.0, 2.0, 2.5, 5.0};
static int CAdecimals[8] =
{0, 0, 1, 0, 0, 0, 1, 0};
static int CAminors[8] =
{4, 4, 4, 5, 4, 4, 4, 5};

static void
ComputeAxis (SciPlotAxis *axis, real min, real max, Boolean log, int type)
{
  real range, rnorm, delta, calcmin, calcmax;
  int nexp, majornum, minornum, majordecimals, decimals, i;

#if DEBUG_SCIPLOT || DEBUG_SCIPLOT_AXIS
    SciPlotPrintf("\nComputeAxis: min=%f max=%f\n", min, max);
#endif
  range = max - min;
  if (log) {
    if (range==0.0) {
      calcmin = powi(10.0, (int) floor(log10(min) + SCIPLOT_EPS));
      calcmax = 10.0*calcmin;
    }
    else {
      calcmin = powi(10.0, (int) floor(log10(min) + SCIPLOT_EPS));
      calcmax = powi(10.0, (int) ceil(log10(max) - SCIPLOT_EPS));
    }

    /*
    SciPlotPrintf("calcmin=%e min=%e   calcmax=%e max=%e\n",calcmin,min,
	   calcmax,max); */

    delta = 10.0;

    if (type == COMPUTE_MIN_MAX) {
      axis->DrawOrigin = calcmin;
      axis->DrawMax = calcmax;
    }
    else if (type == COMPUTE_MIN_ONLY) {
      axis->DrawOrigin = calcmin;
      calcmax = axis->DrawMax;
    }
    else if (type == COMPUTE_MAX_ONLY) {
      calcmin = axis->DrawOrigin;
      axis->DrawMax = calcmax;
    }
    else if (type == NO_COMPUTE_MIN_MAX) {
      axis->DrawOrigin = calcmin;
      axis->DrawMax = calcmax;
    }

    axis->DrawSize = log10(calcmax) - log10(calcmin);
    axis->MajorInc = delta;
    axis->MajorNum = (int) (log10(calcmax) - log10(calcmin)) + 1;
    axis->MinorNum = 10;
    axis->Precision = -(int) (log10(calcmin) * 1.0001);
#if DEBUG_SCIPLOT || DEBUG_SCIPLOT_AXIS
    SciPlotPrintf("calcmin=%e log=%e (int)log=%d  Precision=%d\n",
      calcmin, log10(calcmin), (int) (log10(calcmin) * 1.0001), axis->Precision);
#endif
    if (axis->Precision < 0)
      axis->Precision = 0;
  }
  else {
    if (range==0.0) nexp=0;
    else nexp = (int) floor(log10(range) + SCIPLOT_EPS);
    rnorm = range / powi(10.0, nexp);
    for (i = 0; i < NUMBER_MINOR; i++) {
      delta = CAdeltas[i];
      minornum = CAminors[i];
      majornum = (int) ((rnorm + 0.9999 * delta) / delta);
      majordecimals = CAdecimals[i];
      if (majornum <= MAX_MAJOR)
	break;
    }
    delta *= powi(10.0, nexp);
#if DEBUG_SCIPLOT || DEBUG_SCIPLOT_AXIS
    SciPlotPrintf("nexp=%d range=%f rnorm=%f delta=%f\n", nexp, range, rnorm, delta);
#endif

    if (min < 0.0)
      calcmin = ((float) ((int) ((min - .9999 * delta) / delta))) * delta;
    else if ((min > 0.0) && (min < 1.0))
      calcmin = ((float) ((int) ((1.0001 * min) / delta))) * delta;
    else if (min >= 1.0)
      calcmin = ((float) ((int) ((.9999 * min) / delta))) * delta;
    else
      calcmin = min;
    if (max < 0.0)
      calcmax = ((float) ((int) ((.9999 * max) / delta))) * delta;
    else if (max > 0.0)
      calcmax = ((float) ((int) ((max + .9999 * delta) / delta))) * delta;
    else
      calcmax = max;

    if (type == COMPUTE_MIN_MAX) {
      axis->DrawOrigin = calcmin;
      axis->DrawMax = calcmax;
    }
    else if (type == COMPUTE_MIN_ONLY) {
      axis->DrawOrigin = calcmin;
      calcmax = axis->DrawMax;
    }
    else if (type == COMPUTE_MAX_ONLY) {
      calcmin = axis->DrawOrigin;
      axis->DrawMax = calcmax;
    }
    else if (type == NO_COMPUTE_MIN_MAX) {
      calcmin = min;
      calcmax = max;
      axis->DrawOrigin = calcmin;
      axis->DrawMax = calcmax;
    }

    axis->DrawSize = calcmax - calcmin;
    axis->MajorInc = delta;
    axis->MajorNum = majornum;
    axis->MinorNum = minornum;

    delta = log10(axis->MajorInc);
    if (delta > 0.0)
      decimals = -(int) floor(delta + SCIPLOT_EPS) + majordecimals;
    else
      decimals = (int) ceil(-delta - SCIPLOT_EPS) + majordecimals;
    if (decimals < 0)
      decimals = 0;
#if DEBUG_SCIPLOT || DEBUG_SCIPLOT_AXIS
    SciPlotPrintf("delta=%f majordecimals=%d decimals=%d\n",
      delta, majordecimals, decimals);
#endif
    axis->Precision = decimals;
  }

#if DEBUG_SCIPLOT || DEBUG_SCIPLOT_AXIS
  SciPlotPrintf("Tics: min=%f max=%f size=%f  major inc=%f #major=%d #minor=%d decimals=%d\n",
    axis->DrawOrigin, axis->DrawMax, axis->DrawSize,
    axis->MajorInc, axis->MajorNum, axis->MinorNum, axis->Precision);
#endif
}

static void
ComputeDrawingRange (SciPlotWidget w, int type)
{
  /* when we are dragging along one direction, we do not need to caculate the drawing */
  /* range of the other axis                                                          */
  if (w->plot.ChartType == XtCARTESIAN) {
    if (w->plot.drag_state == NOT_DRAGGING || w->plot.drag_state == DRAGGING_NOTHING) {
      if (type == NO_COMPUTE_MIN_MAX_X) {
	ComputeAxis(&w->plot.x, w->plot.Min.x, w->plot.Max.x,
		    w->plot.XLog, NO_COMPUTE_MIN_MAX);
	ComputeAxis(&w->plot.y, w->plot.Min.y, w->plot.Max.y,
		    w->plot.YLog, COMPUTE_MIN_MAX);
      }
      else if (type == NO_COMPUTE_MIN_MAX_Y) {
	ComputeAxis(&w->plot.x, w->plot.Min.x, w->plot.Max.x,
		    w->plot.XLog, COMPUTE_MIN_MAX);
	ComputeAxis(&w->plot.y, w->plot.Min.y, w->plot.Max.y,
		    w->plot.YLog, NO_COMPUTE_MIN_MAX);
      }
      else {
	ComputeAxis(&w->plot.x, w->plot.Min.x, w->plot.Max.x,
		    w->plot.XLog, type);
	ComputeAxis(&w->plot.y, w->plot.Min.y, w->plot.Max.y,
		    w->plot.YLog, type);
      }
    }
    else if (w->plot.drag_state == DRAGGING_LEFT || w->plot.drag_state == DRAGGING_RIGHT ||
	     w->plot.drag_state == DRAGGING_BOTTOM_AND_LEFT ||
	     w->plot.drag_state == DRAGGING_DATA)
      ComputeAxis(&w->plot.x, w->plot.Min.x, w->plot.Max.x,
		  w->plot.XLog, type);
    else if (w->plot.drag_state == DRAGGING_TOP || w->plot.drag_state == DRAGGING_BOTTOM)
      ComputeAxis(&w->plot.y, w->plot.Min.y, w->plot.Max.y,
		  w->plot.YLog, type);
  }
  else {
    ComputeAxis(&w->plot.x, (real) 0.0, w->plot.Max.x,
      (Boolean) FALSE, type);
    w->plot.PolarScale = w->plot.x.DrawMax;
  }
}

static Boolean
CheckMinMax (SciPlotWidget w, int type)
{
  register int i, j;
  register SciPlotList *p;
  register real val;

  if (type == COMPUTE_MIN_MAX) {
    if (w->plot.ChartType == XtCARTESIAN) {
      for (i = 0; i < w->plot.num_plotlist; i++) {
	p = w->plot.plotlist + i;
	if (p->draw) {
	  for (j = 0; j < p->number; j++) {

            /* Don't count the "break in line segment" flag for Min/Max */
	    if (p->data[j].x > SCIPLOT_SKIP_VAL &&
		p->data[j].y > SCIPLOT_SKIP_VAL) {

	      val = p->data[j].x;
	      if (val > w->plot.x.DrawMax || val < w->plot.x.DrawOrigin)
		return True;
	      val = p->data[j].y;
	      if (val > w->plot.y.DrawMax || val < w->plot.y.DrawOrigin)
		return True;
	    }
	  }
	}
      }
    }
    else {
      for (i = 0; i < w->plot.num_plotlist; i++) {
	p = w->plot.plotlist + i;
	if (p->draw) {
	  for (j = 0; j < p->number; j++) {
	    val = p->data[j].x;
	    if (val > w->plot.Max.x || val < w->plot.Min.x)
	      return True;
	  }
	}
      }
    }
  }
  else if (type == NO_COMPUTE_MIN_MAX_X) {
    if (w->plot.ChartType == XtCARTESIAN) {
      for (i = 0; i < w->plot.num_plotlist; i++) {
	p = w->plot.plotlist + i;
	if (p->draw) {
	  for (j = 0; j < p->number; j++) {

            /* Don't count the "break in line segment" flag for Min/Max */
	    if (p->data[j].y > SCIPLOT_SKIP_VAL) {
	      val = p->data[j].y;
	      if (val > w->plot.y.DrawMax || val < w->plot.y.DrawOrigin)
		return True;
	    }
	  }
	}
      }
    }
  }
  return False;
}

static void
ComputeMinMax (SciPlotWidget w, int type)
{
  register int i, j;
  register SciPlotList *p;
  register real val;
  Boolean firstx, firsty;

  if (type != NO_COMPUTE_MIN_MAX_X && type != NO_COMPUTE_MIN_MAX_Y) {
    w->plot.Min.x = w->plot.Min.y = w->plot.Max.x = w->plot.Max.y = 1.0;
    firstx = True;
    firsty = True;

    for (i = 0; i < w->plot.num_plotlist; i++) {
      p = w->plot.plotlist + i;
      if (p->draw) {
	for (j = 0; j < p->number; j++) {

          /* Don't count the "break in line segment" flag for Min/Max */
	  if (p->data[j].x > SCIPLOT_SKIP_VAL &&
	      p->data[j].y > SCIPLOT_SKIP_VAL) {

	    val = p->data[j].x;
	    if (!w->plot.XLog || (w->plot.XLog && (val > 0.0))) {
	      if (firstx) {
		w->plot.Min.x = w->plot.Max.x = val;
		firstx = False;
	      }
	      else {
		if (val > w->plot.Max.x)
		  w->plot.Max.x = val;
		else if (val < w->plot.Min.x)
		  w->plot.Min.x = val;
	      }
	    }

	    val = p->data[j].y;
	    if (!w->plot.YLog || (w->plot.YLog && (val > 0.0))) {
	      if (firsty) {
		w->plot.Min.y = w->plot.Max.y = val;
		firsty = False;
	      }
	      else {
		if (val > w->plot.Max.y)
		  w->plot.Max.y = val;
		else if (val < w->plot.Min.y)
		  w->plot.Min.y = val;
	      }
	    }
	  }

	}
      }
    }

    /* fix defaults if there is only one point. */
    if (firstx) {
      if (w->plot.XLog) {
	w->plot.Min.x = 1.0;
	w->plot.Max.x = 10.0;
      }
      else {
	w->plot.Min.x = 0.0;
	w->plot.Max.x = 10.0;
      }
    }
    if (firsty) {
      if (w->plot.YLog) {
	w->plot.Min.y = 1.0;
	w->plot.Max.y = 10.0;
      }
      else {
	w->plot.Min.y = 0.0;
	w->plot.Max.y = 10.0;
      }
    }

    /* real x and y range */
    w->plot.startx = w->plot.Min.x;
    w->plot.starty = w->plot.Min.y;
    w->plot.endx   = w->plot.Max.x;
    w->plot.endy   = w->plot.Max.y;

    /* now calculate plotting range */

    if (w->plot.ChartType == XtCARTESIAN) {
      if (!w->plot.XLog) {
	if (!w->plot.XAutoScale) {
	  w->plot.Min.x = w->plot.UserMin.x;
	  w->plot.Max.x = w->plot.UserMax.x;
	}
	else if (w->plot.XOrigin) {
	  if (w->plot.Min.x > 0.0)
	    w->plot.Min.x = 0.0;
	  if (w->plot.Max.x < 0.0)
	    w->plot.Max.x = 0.0;
	}
	if (fabs(w->plot.Min.x - w->plot.Max.x) < 1.e-10) {
	  w->plot.Min.x -= .5;
	  w->plot.Max.x += .5;
	}
      }
      else {
	if (!w->plot.XAutoScale && w->plot.UserMin.x > 0 &&
	    w->plot.UserMax.x > 0  &&
	    w->plot.UserMax.x >= w->plot.UserMin.x) {
	  w->plot.Min.x = w->plot.UserMin.x;
	  w->plot.Max.x = w->plot.UserMax.x;
	}
      }

      if (!w->plot.YLog) {
	if (!w->plot.YAutoScale) {
	  w->plot.Min.y = w->plot.UserMin.y;
	  w->plot.Max.y = w->plot.UserMax.y;
	}
	else if (w->plot.YOrigin) {
	  if (w->plot.Min.y > 0.0)
	    w->plot.Min.y = 0.0;
	  if (w->plot.Max.y < 0.0)
	    w->plot.Max.y = 0.0;
	}
	if (fabs(w->plot.Min.y - w->plot.Max.y) < 1.e-10) {
	  w->plot.Min.y -= .5;
	  w->plot.Max.y += .5;
	}
      }
      else {
	if (!w->plot.YAutoScale && w->plot.UserMin.y > 0 &&
	    w->plot.UserMax.y > 0  &&
	    w->plot.UserMax.y >= w->plot.UserMin.y) {
	  w->plot.Min.y = w->plot.UserMin.y;
	  w->plot.Max.y = w->plot.UserMax.y;
	}
      }

    }
    else {
      if (fabs(w->plot.Min.x) > fabs(w->plot.Max.x))
	w->plot.Max.x = fabs(w->plot.Min.x);
    }
  }
  else if (type == NO_COMPUTE_MIN_MAX_X) {
    w->plot.Min.y = w->plot.Max.y = 1.0;
    firsty = True;

    for (i = 0; i < w->plot.num_plotlist; i++) {
      p = w->plot.plotlist + i;
      if (p->draw) {
	for (j = 0; j < p->number; j++) {
	  /* Don't count the "break in line segment" flag for Min/Max */
	  if (p->data[j].y > SCIPLOT_SKIP_VAL) {
	    val = p->data[j].y;
	    if (!w->plot.YLog || (w->plot.YLog && (val > 0.0))) {
	      if (firsty) {
		w->plot.Min.y = w->plot.Max.y = val;
		firsty = False;
	      }
	      else {
		if (val > w->plot.Max.y)
		  w->plot.Max.y = val;
		else if (val < w->plot.Min.y)
		  w->plot.Min.y = val;
	      }

	    }

	  }
	}
      }
    }

    if (firsty) {
      if (w->plot.YLog) {
	w->plot.Min.y = 1.0;
	w->plot.Max.y = 10.0;
      }
      else {
	w->plot.Min.y = 0.0;
	w->plot.Max.y = 10.0;
      }
    }

    /* real x and y range */
    w->plot.starty = w->plot.Min.y;
    w->plot.endy   = w->plot.Max.y;

    /* now calculate plotting range */

    if (w->plot.ChartType == XtCARTESIAN) {
      if (!w->plot.XLog) {
	if (!w->plot.XAutoScale) {
	  w->plot.Min.x = w->plot.UserMin.x;
	  w->plot.Max.x = w->plot.UserMax.x;
	}
      }
      else {
	if (!w->plot.XAutoScale && w->plot.UserMin.x > 0 &&
	    w->plot.UserMax.x > 0  &&
	    w->plot.UserMax.x >= w->plot.UserMin.x) {
	  w->plot.Min.x = w->plot.UserMin.x;
	  w->plot.Max.x = w->plot.UserMax.x;
	}
      }

      if (!w->plot.YLog) {
	if (!w->plot.YAutoScale) {
	  w->plot.Min.y = w->plot.UserMin.y;
	  w->plot.Max.y = w->plot.UserMax.y;
	}
	else if (w->plot.YOrigin) {
	  if (w->plot.Min.y > 0.0)
	    w->plot.Min.y = 0.0;
	  if (w->plot.Max.y < 0.0)
	    w->plot.Max.y = 0.0;
	}
	if (fabs(w->plot.Min.y - w->plot.Max.y) < 1.e-10) {
	  w->plot.Min.y -= .5;
	  w->plot.Max.y += .5;
	}
      }
      else {
	if (!w->plot.YAutoScale && w->plot.UserMin.y > 0 &&
	    w->plot.UserMax.y > 0  &&
	    w->plot.UserMax.y >= w->plot.UserMin.y) {
	  w->plot.Min.y = w->plot.UserMin.y;
	  w->plot.Max.y = w->plot.UserMax.y;
	}
      }

    }
  }
  else if (type == NO_COMPUTE_MIN_MAX_Y) {
    w->plot.Min.x = w->plot.Max.x = 1.0;
    firstx = True;

    for (i = 0; i < w->plot.num_plotlist; i++) {
      p = w->plot.plotlist + i;
      if (p->draw) {
	for (j = 0; j < p->number; j++) {

          /* Don't count the "break in line segment" flag for Min/Max */
	  if (p->data[j].x > SCIPLOT_SKIP_VAL)   {
	    val = p->data[j].x;
	    if (!w->plot.XLog || (w->plot.XLog && (val > 0.0))) {
	      if (firstx) {
		w->plot.Min.x = w->plot.Max.x = val;
		firstx = False;
	      }
	      else {
		if (val > w->plot.Max.x)
		  w->plot.Max.x = val;
		else if (val < w->plot.Min.x)
		  w->plot.Min.x = val;
	      }
	    }

	  }

	}
      }
    }

    /* fix defaults if there is only one point. */
    if (firstx) {
      if (w->plot.XLog) {
	w->plot.Min.x = 1.0;
	w->plot.Max.x = 10.0;
      }
      else {
	w->plot.Min.x = 0.0;
	w->plot.Max.x = 10.0;
      }
    }

    /* real x and y range */
    w->plot.startx = w->plot.Min.x;
    w->plot.endx   = w->plot.Max.x;

    /* now calculate plotting range */

    if (w->plot.ChartType == XtCARTESIAN) {
      if (!w->plot.XLog) {
	if (!w->plot.XAutoScale) {
	  w->plot.Min.x = w->plot.UserMin.x;
	  w->plot.Max.x = w->plot.UserMax.x;
	}
	else if (w->plot.XOrigin) {
	  if (w->plot.Min.x > 0.0)
	    w->plot.Min.x = 0.0;
	  if (w->plot.Max.x < 0.0)
	    w->plot.Max.x = 0.0;
	}
	if (fabs(w->plot.Min.x - w->plot.Max.x) < 1.e-10) {
	  w->plot.Min.x -= .5;
	  w->plot.Max.x += .5;
	}
      }
      else {
	if (!w->plot.XAutoScale && w->plot.UserMin.x > 0 &&
	    w->plot.UserMax.x > 0  &&
	    w->plot.UserMax.x >= w->plot.UserMin.x) {
	  w->plot.Min.x = w->plot.UserMin.x;
	  w->plot.Max.x = w->plot.UserMax.x;
	}
      }

      if (!w->plot.YLog) {
	if (!w->plot.YAutoScale) {
	  w->plot.Min.y = w->plot.UserMin.y;
	  w->plot.Max.y = w->plot.UserMax.y;
	}
      }
      else {
	if (!w->plot.YAutoScale && w->plot.UserMin.y > 0 &&
	    w->plot.UserMax.y > 0  &&
	    w->plot.UserMax.y >= w->plot.UserMin.y) {
	  w->plot.Min.y = w->plot.UserMin.y;
	  w->plot.Max.y = w->plot.UserMax.y;
	}
      }
    }
    else {
      if (fabs(w->plot.Min.x) > fabs(w->plot.Max.x))
	w->plot.Max.x = fabs(w->plot.Min.x);
    }
  }


#if DEBUG_SCIPLOT
  SciPlotPrintf("Min: (%f,%f)\tMax: (%f,%f)\n",
    w->plot.Min.x, w->plot.Min.y,
    w->plot.Max.x, w->plot.Max.y);
#endif
}

static void
ComputeLegendDimensions (SciPlotWidget w)
{
  real current, xmax, ymax;
  int i;
  SciPlotList *p;

  if (!w->plot.XFixedLR) {
    if (w->plot.ShowLegend) {
      xmax = 0.0;
      ymax = 2.0 * (real) w->plot.LegendMargin;

      for (i = 0; i < w->plot.num_plotlist; i++) {
	p = w->plot.plotlist + i;
	if (p->draw) {
	  current = (real) w->plot.Margin +
	    (real) w->plot.LegendMargin * 3.0 +
	    (real) w->plot.LegendLineSize +
	    FontnumTextWidth(w, w->plot.axisFont, p->legend);
	  if (current > xmax)
	    xmax = current;
	  ymax += FontnumHeight(w, w->plot.axisFont);
	}
      }

      w->plot.x.LegendSize = xmax;
      w->plot.x.LegendPos = (real) w->plot.Margin;
      w->plot.y.LegendSize = ymax;
      w->plot.y.LegendPos = 0.0;
    }
    else {
      w->plot.x.LegendSize =
	w->plot.x.LegendPos =
	w->plot.y.LegendSize =
	w->plot.y.LegendPos = 0.0;
    }
  }
  else {
    if (w->plot.ShowLegend) {
      xmax = 0.0;
      ymax = 2.0 * (real) w->plot.LegendMargin;

      for (i = 0; i < w->plot.num_plotlist; i++) {
	p = w->plot.plotlist + i;
	if (p->draw)
	  ymax += FontnumHeight(w, w->plot.axisFont);
      }
      w->plot.x.LegendSize = w->plot.XRightSpace - w->plot.LegendMargin -
	w->plot.Margin;
      w->plot.x.LegendPos = w->plot.Margin;
      w->plot.y.LegendSize = ymax;
      w->plot.y.LegendPos = 0.0;
    }
    else {
      w->plot.x.LegendSize =
	w->plot.x.LegendPos =
	w->plot.y.LegendSize =
	w->plot.y.LegendPos = 0.0;
    }
  }
}

static void
ComputeDimensions (SciPlotWidget w)
{
  real x, y, width, height, axisnumbersize, axisXlabelsize, axisYlabelsize;

  if (!w->plot.XFixedLR) {
    /* x,y is the origin of the upper left corner of the drawing area inside
     * the widget.  Doesn't necessarily have to be (Margin,Margin)
     * as it is now.
     */
    x = (real) w->plot.Margin;
    y = (real) w->plot.Margin;

    /* width = (real)w->core.width - (real)w->plot.Margin - x -
     * **           legendwidth - AxisFontHeight
     */
    width = (real) w->core.width - (real) w->plot.Margin - x -
      w->plot.x.LegendSize;

    /* height = (real)w->core.height - (real)w->plot.Margin - y
     *           - Height of axis numbers (including margin)
     *           - Height of axis label (including margin)
     *           - Height of Title (including margin)
     */
    height = (real) w->core.height - (real) w->plot.Margin - y;

#ifdef MOTIF
    width -= w->primitive.shadow_thickness;
    height -= w->primitive.shadow_thickness;
#endif

    w->plot.x.Origin = x;
    w->plot.y.Origin = y;

    /* Adjust the size depending upon what sorts of text are visible. */
    if (w->plot.ShowTitle)
      height -= (real) w->plot.TitleMargin +
	FontnumHeight(w, w->plot.titleFont);

    if (w->plot.ChartType == XtCARTESIAN) {
      axisnumbersize = (real) w->plot.Margin +
	FontnumHeight(w, w->plot.axisFont);
      if (w->plot.XAxisNumbers) {
	height -= axisnumbersize;
      }
      if (w->plot.YAxisNumbers) {
	width -= axisnumbersize;
	w->plot.x.Origin += axisnumbersize;
      }

      if (w->plot.ShowXLabel) {
	axisXlabelsize = (real) w->plot.Margin +
	  FontnumHeight(w, w->plot.labelFont);
	height -= axisXlabelsize;
      }
      if (w->plot.ShowYLabel) {
	axisYlabelsize = (real) w->plot.Margin +
	  FontnumHeight(w, w->plot.labelFont);
	width -= axisYlabelsize;
	w->plot.x.Origin += axisYlabelsize;
      }
    }

    w->plot.x.Size = width;
    w->plot.y.Size = height;

  }
  else {
    y = (real) w->plot.Margin;
    height = (real) w->core.height - (real) w->plot.Margin - y;
    width = (real) w->core.width - (real) w->plot.XLeftSpace -
      (real)w->plot.XRightSpace;

#ifdef MOTIF
    width -= w->primitive.shadow_thickness;
    height -= w->primitive.shadow_thickness;
#endif

    w->plot.x.Origin = w->plot.XLeftSpace;
    w->plot.y.Origin = y;

    /* Adjust the size depending upon what sorts of text are visible. */
    if (w->plot.ShowTitle)
      height -= (real) w->plot.TitleMargin +
	FontnumHeight(w, w->plot.titleFont);


    if (w->plot.ChartType == XtCARTESIAN) {
      axisnumbersize = (real) w->plot.Margin +
	FontnumHeight(w, w->plot.axisFont);
      if (w->plot.XAxisNumbers) {
	height -= axisnumbersize;
      }

      if (w->plot.ShowXLabel) {
	axisXlabelsize = (real) w->plot.Margin +
	  FontnumHeight(w, w->plot.labelFont);
	height -= axisXlabelsize;
      }

      w->plot.x.Size = width;
      w->plot.y.Size = height;
    }
  }
  /* Adjust parameters for polar plot */
  if (w->plot.ChartType == XtPOLAR) {
    if (height < width)
      w->plot.x.Size = height;
  }
  w->plot.x.Center = w->plot.x.Origin + (width / 2.0);
  w->plot.y.Center = w->plot.y.Origin + (height / 2.0);

}

static void
AdjustDimensionsCartesian (SciPlotWidget w)
{
  real xextra, yextra, val, xhorz;
  real x, y, width, height, axisnumbersize, axisXlabelsize, axisYlabelsize;
  char numberformat[16], label[16];
  int precision;

  /* Compute xextra and yextra, which are the extra distances that the text
   * labels on the axes stick outside of the graph.
   */
  xextra = yextra = 0.0;
  if (w->plot.XAxisNumbers) {
    precision = w->plot.x.Precision;
    if (w->plot.XLog) {
      val = w->plot.x.DrawMax;
      precision -= w->plot.x.MajorNum;
      if (precision < 0)
	precision = 0;
    }
    else
      val = w->plot.x.DrawOrigin + floor(w->plot.x.DrawSize /
	w->plot.x.MajorInc + SCIPLOT_EPS) * w->plot.x.MajorInc;

    x = PlotX(w, val);
    sprintf(numberformat, "%%.%df", precision);
    sprintf(label, numberformat, val);
    x += FontnumTextWidth(w, w->plot.axisFont, label);
    if ((int) x > w->core.width) {
      xextra = ceil(x - w->core.width + w->plot.Margin - SCIPLOT_EPS);
      if (xextra < 0.0)
	xextra = 0.0;
    }
  }

  yextra=xhorz=0.0;
  if (w->plot.YAxisNumbers) {
    precision = w->plot.y.Precision;
    if (w->plot.YLog) {
      int p1,p2;

      p1=precision;
      val = w->plot.y.DrawOrigin;
      if (p1 > 0)
	p1--;

      val = w->plot.y.DrawMax;
      p2 = precision - w->plot.y.MajorNum;
      if (p2 < 0)
	p2 = 0;

      if (p1>p2) precision=p1;
      else precision=p2;
    }
    else
      val = w->plot.y.DrawOrigin + floor(w->plot.y.DrawSize /
		 w->plot.y.MajorInc + SCIPLOT_EPS) * w->plot.y.MajorInc;
    y = PlotY(w, val);
    sprintf(numberformat, "%%.%df", precision);
    sprintf(label, numberformat, val);
#if DEBUG_SCIPLOT
    SciPlotPrintf("ylabel=%s\n", label);
#endif
    if (w->plot.YNumHorz) {
      yextra=FontnumHeight(w, w->plot.axisFont)/2.0;
      xhorz=FontnumTextWidth(w, w->plot.axisFont, label)
	+ (real)w->plot.Margin;
    }
    else {
      y -= FontnumTextWidth(w, w->plot.axisFont, label);
      if ((int) y <= 0) {
	yextra = ceil(w->plot.Margin - y - SCIPLOT_EPS);
	if (yextra < 0.0)
	  yextra = 0.0;
      }
    }
  }
#if DEBUG_SCIPLOT
    SciPlotPrintf("xextra=%f  yextra=%f\n",xextra,yextra);
#endif

  if (!w->plot.XFixedLR) {
    /* x,y is the origin of the upper left corner of the drawing area inside
     * the widget.  Doesn't necessarily have to be (Margin,Margin)
     * as it is now.
     */
    x = (real) w->plot.Margin + xhorz;
    y = (real) w->plot.Margin + yextra;

    /* width = (real)w->core.width - (real)w->plot.Margin - x -
     *          legendwidth - AxisFontHeight
     */
    width = (real) w->core.width - (real) w->plot.Margin - x - xextra;


    /* height = (real)w->core.height - (real)w->plot.Margin - y
     *           - Height of axis numbers (including margin)
     *           - Height of axis label (including margin)
     *           - Height of Title (including margin)
     */
    height = (real) w->core.height - (real) w->plot.Margin - y;

#ifdef MOTIF
    width -= w->primitive.shadow_thickness;
    height -= w->primitive.shadow_thickness;
#endif

    w->plot.x.Origin = x;
    w->plot.y.Origin = y;

    /* Adjust the size depending upon what sorts of text are visible. */
    if (w->plot.ShowTitle)
      height -= (real) w->plot.TitleMargin +
	FontnumHeight(w, w->plot.titleFont);

    axisXlabelsize = 0.0;
    axisYlabelsize = 0.0;
    axisnumbersize = (real) w->plot.Margin +
      FontnumHeight(w, w->plot.axisFont);
    if (w->plot.XAxisNumbers) {
      height -= axisnumbersize;
    }
    if (w->plot.YAxisNumbers && !w->plot.YNumHorz) {
      width -= axisnumbersize;
      w->plot.x.Origin += axisnumbersize;
    }

    if (w->plot.ShowXLabel) {
      axisXlabelsize = (real) w->plot.Margin +
	FontnumHeight(w, w->plot.labelFont);
      height -= axisXlabelsize;
    }
    if (w->plot.ShowYLabel) {
      axisYlabelsize = (real) w->plot.Margin +
	FontnumHeight(w, w->plot.labelFont);
      width -= axisYlabelsize;
      w->plot.x.Origin += axisYlabelsize;
    }

    /* Move legend position to the right of the plot */
    if (w->plot.LegendThroughPlot) {
      w->plot.x.LegendPos += w->plot.x.Origin + width - w->plot.x.LegendSize;
      w->plot.y.LegendPos += w->plot.y.Origin;
    }
    else {
      width -= w->plot.x.LegendSize;
      w->plot.x.LegendPos += w->plot.x.Origin + width;
      w->plot.y.LegendPos += w->plot.y.Origin;
    }

    w->plot.x.Size = width;
    w->plot.y.Size = height;

    w->plot.y.AxisPos = w->plot.y.Origin + w->plot.y.Size +
      (real) w->plot.Margin +
      FontnumAscent(w, w->plot.axisFont);
    if (w->plot.YNumHorz) {
      w->plot.x.AxisPos = w->plot.x.Origin - (real) w->plot.Margin;
    }
    else {
      w->plot.x.AxisPos = w->plot.x.Origin -
	(real) w->plot.Margin -
	FontnumDescent(w, w->plot.axisFont);
    }

    w->plot.y.LabelPos = w->plot.y.Origin + w->plot.y.Size +
      (real) w->plot.Margin + (FontnumHeight(w, w->plot.labelFont) / 2.0);
    if (w->plot.XAxisNumbers)
      w->plot.y.LabelPos += axisnumbersize;
    if (w->plot.YAxisNumbers) {
      if (w->plot.YNumHorz) {
	w->plot.x.LabelPos = w->plot.x.Origin -
	  xhorz - (real) w->plot.Margin -
	  (FontnumHeight(w, w->plot.labelFont) / 2.0);
      }
      else {
	w->plot.x.LabelPos = w->plot.x.Origin -
	  axisnumbersize - (real) w->plot.Margin -
	  (FontnumHeight(w, w->plot.labelFont) / 2.0);
      }
    }
    else {
      w->plot.x.LabelPos = w->plot.x.Origin - (real) w->plot.Margin -
        (FontnumHeight(w, w->plot.labelFont) / 2.0);
    }

    w->plot.y.TitlePos = (real) w->core.height - (real) w->plot.Margin;
    w->plot.x.TitlePos = (real) w->plot.Margin;
  }
  else {
    x = (real) w->plot.XLeftSpace;
    y = (real) w->plot.Margin + yextra;

    /* width = (real)w->core.width - (real)w->plot.Margin - x -
     *          legendwidth - AxisFontHeight
     */
    width = (real) w->core.width - (real) w->plot.XLeftSpace -
      (real) w->plot.XRightSpace;


    /* height = (real)w->core.height - (real)w->plot.Margin - y
     *           - Height of axis numbers (including margin)
     *           - Height of axis label (including margin)
     *           - Height of Title (including margin)
     */
    height = (real) w->core.height - (real) w->plot.Margin - y;

#ifdef MOTIF
    width -= w->primitive.shadow_thickness;
    height -= w->primitive.shadow_thickness;
#endif

    w->plot.x.Origin = x;
    w->plot.y.Origin = y;

    /* Adjust the size depending upon what sorts of text are visible. */
    if (w->plot.ShowTitle)
      height -= (real) w->plot.TitleMargin +
	FontnumHeight(w, w->plot.titleFont);

    axisXlabelsize = 0.0;
    axisYlabelsize = 0.0;
    axisnumbersize = (real) w->plot.Margin +
      FontnumHeight(w, w->plot.axisFont);
    if (w->plot.XAxisNumbers) {
      height -= axisnumbersize;
    }

    if (w->plot.ShowXLabel) {
      axisXlabelsize = (real) w->plot.Margin +
	FontnumHeight(w, w->plot.labelFont);
      height -= axisXlabelsize;
    }

    /* Move legend position to the right of the plot */
    if (w->plot.LegendThroughPlot) {
      w->plot.x.LegendPos += w->plot.x.Origin + width - w->plot.x.LegendSize;
      w->plot.y.LegendPos += w->plot.y.Origin;
    }
    else {
      w->plot.x.LegendPos += w->plot.x.Origin + width;
      w->plot.y.LegendPos += w->plot.y.Origin;
    }

    w->plot.x.Size = width;
    w->plot.y.Size = height;

    w->plot.y.AxisPos = w->plot.y.Origin + w->plot.y.Size +
      (real) w->plot.Margin +
      FontnumAscent(w, w->plot.axisFont);

    /* this is ynumber positions */
    if (w->plot.YNumHorz) {
      w->plot.x.AxisPos = w->plot.x.Origin - (real) w->plot.Margin;
    }
    else {
      w->plot.x.AxisPos = w->plot.x.Origin -
	(real) w->plot.Margin -
	FontnumDescent(w, w->plot.axisFont);
    }


    w->plot.y.LabelPos = w->plot.y.Origin + w->plot.y.Size +
      (real) w->plot.Margin + (FontnumHeight(w, w->plot.labelFont) / 2.0);
    if (w->plot.XAxisNumbers)
      w->plot.y.LabelPos += axisnumbersize;

    if (w->plot.YAxisNumbers) {
      if (w->plot.YNumHorz) {
	w->plot.x.LabelPos = w->plot.x.Origin -
	  xhorz - (real) w->plot.Margin -
	  (FontnumHeight(w, w->plot.labelFont) / 2.0);
      }
      else {
	w->plot.x.LabelPos = w->plot.x.Origin -
	  axisnumbersize - (real) w->plot.Margin -
	  (FontnumHeight(w, w->plot.labelFont) / 2.0);
      }
    }
    else {
      w->plot.x.LabelPos = w->plot.x.Origin - (real) w->plot.Margin -
        (FontnumHeight(w, w->plot.labelFont) / 2.0);
    }

    w->plot.y.TitlePos = (real) w->core.height - (real) w->plot.Margin;
    w->plot.x.TitlePos = (real) w->plot.Margin;
  }

#if DEBUG_SCIPLOT
  SciPlotPrintf("y.Origin:             %f\n", w->plot.y.Origin);
  SciPlotPrintf("y.Size:               %f\n", w->plot.y.Size);
  SciPlotPrintf("axisnumbersize:       %f\n", axisnumbersize);
  SciPlotPrintf("y.axisLabelSize:      %f\n", axisYlabelsize);
  SciPlotPrintf("y.TitleSize:          %f\n",
    (real) w->plot.TitleMargin + FontnumHeight(w, w->plot.titleFont));
  SciPlotPrintf("y.Margin:             %f\n", (real) w->plot.Margin);
  SciPlotPrintf("total-----------------%f\n", w->plot.y.Origin + w->plot.y.Size +
    axisnumbersize + axisYlabelsize + (real) w->plot.Margin +
    (real) w->plot.TitleMargin + FontnumHeight(w, w->plot.titleFont));
  SciPlotPrintf("total should be-------%f\n", (real) w->core.height);
#endif
}

static void
AdjustDimensionsPolar (SciPlotWidget w)
{
  real x, y, xextra, yextra, val;
  real width, height, size;
  char numberformat[16], label[16];

/* Compute xextra and yextra, which are the extra distances that the text
 * labels on the axes stick outside of the graph.
 */
  xextra = yextra = 0.0;
  val = w->plot.PolarScale;
  PlotRTDegrees(w, val, 0.0, &x, &y);
  sprintf(numberformat, "%%.%df", w->plot.x.Precision);
  sprintf(label, numberformat, val);
  x += FontnumTextWidth(w, w->plot.axisFont, label);
  if ((int) x > w->core.width) {
    xextra = x - w->core.width + w->plot.Margin;
    if (xextra < 0.0)
      xextra = 0.0;
  }
  yextra = 0.0;


  /* x,y is the origin of the upper left corner of the drawing area inside
   * the widget.  Doesn't necessarily have to be (Margin,Margin)
   * as it is now.
   */
  w->plot.x.Origin = (real) w->plot.Margin;
  w->plot.y.Origin = (real) w->plot.Margin;

  /* width = (real)w->core.width - (real)w->plot.Margin - x -
   *          legendwidth - AxisFontHeight
   */
  width = (real) w->core.width - (real) w->plot.Margin - w->plot.x.Origin - xextra;

  /* height = (real)w->core.height - (real)w->plot.Margin - y
   *           - Height of axis numbers (including margin)
   *           - Height of axis label (including margin)
   *           - Height of Title (including margin)
   */
  height = (real) w->core.height - (real) w->plot.Margin - w->plot.y.Origin - yextra;

#ifdef MOTIF
    width -= w->primitive.shadow_thickness;
    height -= w->primitive.shadow_thickness;
#endif

  /* Adjust the size depending upon what sorts of text are visible. */
  if (w->plot.ShowTitle)
    height -= (real) w->plot.TitleMargin +
      FontnumHeight(w, w->plot.titleFont);

  /* Only need to carry one number for the size, (since it is a circle!) */
  if (height < width)
    size = height;
  else
    size = width;

  /* Assign some preliminary values */
  w->plot.x.Center = w->plot.x.Origin + (width / 2.0);
  w->plot.y.Center = w->plot.y.Origin + (height / 2.0);
  w->plot.x.LegendPos += width - w->plot.x.LegendSize;
  w->plot.y.LegendPos += w->plot.y.Origin;

  /*
   * Check and see if the legend can fit in the blank space in the upper right
   *
   * To fit, the legend must:
   *   1) be less than half the width/height of the plot
   *   2) hmmm.
   */
  if (!w->plot.LegendThroughPlot) {
    real radius = size / 2.0;
    real dist;

    x = w->plot.x.LegendPos - w->plot.x.Center;
    y = (w->plot.y.LegendPos + w->plot.y.LegendSize) - w->plot.y.Center;

    dist = sqrt(x * x + y * y);
    /*       SciPlotPrintf("rad=%f dist=%f: legend=(%f,%f) center=(%f,%f)\n", */
    /*              radius,dist,w->plot.x.LegendPos,w->plot.y.LegendPos, */
    /*              w->plot.x.Center,w->plot.y.Center); */

    /* It doesn't fit if this check is true.  Make the plot smaller */

    /* This is a first cut horrible algorithm.  My calculus is a bit
     * rusty tonight--can't seem to figure out how to maximize a circle
     * in a rectangle with a rectangular chunk out of it. */
    if (dist < radius) {
      width -= w->plot.x.LegendSize;
      height -= w->plot.y.LegendSize;

      /* readjust some parameters */
      w->plot.x.Center = w->plot.x.Origin + width / 2.0;
      w->plot.y.Center = w->plot.y.Origin + w->plot.y.LegendSize + height / 2.0;
      if (height < width)
	size = height;
      else
	size = width;
      }

  }


  /* OK, customization is finished when we reach here. */
  w->plot.x.Size = w->plot.y.Size = size;

  w->plot.y.TitlePos = w->plot.y.Center + w->plot.y.Size / 2.0 +
    (real) w->plot.TitleMargin +
    FontnumAscent(w, w->plot.titleFont);
  w->plot.x.TitlePos = w->plot.x.Origin;
}

static void
AdjustDimensions (SciPlotWidget w)
{
  if (w->plot.ChartType == XtCARTESIAN) {
    AdjustDimensionsCartesian(w);
  }
  else {
    AdjustDimensionsPolar(w);
  }
}

static void
ComputeAllDimensions (SciPlotWidget w, int type)
{
  if (type == COMPUTE_MIN_MAX || type == NO_COMPUTE_MIN_MAX_X ||
      type == NO_COMPUTE_MIN_MAX_Y) {
    ComputeLegendDimensions(w);
    ComputeDimensions(w);
    ComputeDrawingRange(w, type);
    AdjustDimensions(w);
  }
  else {
    /* we are in the middle of dragging, nothing is changed */
    ComputeDrawingRange(w, type);
  }
}

static void
ComputeAll (SciPlotWidget w, int type)
{
  ComputeMinMax(w, type);
  ComputeAllDimensions(w, type);
}


/*
 * Private drawing routines
 */

static void
DrawMarker (SciPlotWidget w, real xpaper, real ypaper, real size,
	    int color, int style)
{
  real sizex, sizey;

  switch (style) {
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
DrawLegend (SciPlotWidget w)
{
  real x, y, len, height, height2, len2, ascent;
  int i;
  SciPlotList *p;

  w->plot.current_id = SciPlotDrawingLegend;
  if (w->plot.ShowLegend) {
    x = w->plot.x.LegendPos;
    y = w->plot.y.LegendPos;
    len = (real) w->plot.LegendLineSize;
    len2 = len / 2.0;
    height = FontnumHeight(w, w->plot.axisFont);
    height2 = height / 2.0;
    ascent = FontnumAscent(w, w->plot.axisFont);
    RectSet(w, x, y,
      x + w->plot.x.LegendSize - 1.0 - (real) w->plot.Margin,
      y + w->plot.y.LegendSize - 1.0,
      w->plot.ForegroundColor, XtLINE_SOLID);
    x += (real) w->plot.LegendMargin;
    y += (real) w->plot.LegendMargin;

    for (i = 0; i < w->plot.num_plotlist; i++) {
      p = w->plot.plotlist + i;
      if (p->draw) {
	LineSet(w, x, y + height2, x + len, y + height2,
	  p->LineColor, p->LineStyle);
	DrawMarker(w, x + len2, y + height2, p->markersize,
	  p->PointColor, p->PointStyle);
	TextSet(w, x + len + (real) w->plot.LegendMargin,
	  y + ascent,
	  p->legend, w->plot.ForegroundColor,
	  w->plot.axisFont);
	y += height;
      }
    }
  }
}

static void
DrawCartesianXLabelAndTitle  (SciPlotWidget w)
{
  w->plot.current_id = SciPlotDrawingXLabelTitle;
  if (w->plot.ShowTitle)
    TextSet(w, w->plot.x.TitlePos, w->plot.y.TitlePos,
      w->plot.plotTitle, w->plot.ForegroundColor,
      w->plot.titleFont);
  if (w->plot.ShowXLabel)
    TextCenter(w, w->plot.x.Origin + (w->plot.x.Size / 2.0),
      w->plot.y.LabelPos, w->plot.xlabel,
      w->plot.ForegroundColor, w->plot.labelFont);
}

static void
DrawCartesianYLabelAndTitle  (SciPlotWidget w)
{
  w->plot.current_id = SciPlotDrawingYLabelTitle;
  if (w->plot.ShowYLabel)
    VTextCenter(w, w->plot.x.LabelPos,
      w->plot.y.Origin + (w->plot.y.Size / 2.0),
      w->plot.ylabel, w->plot.ForegroundColor,
      w->plot.labelFont);
}

static void
DrawCartesianXMajorAndMinor (SciPlotWidget w)
{
  real x, x1, y1, x2, y2, tic, val, majorval;
  int j;

  if (!w->plot.DrawMajor && !w->plot.DrawMinor)
    return;

  w->plot.current_id = SciPlotDrawingXMajorMinor;
  x1 = PlotX(w, w->plot.x.DrawOrigin);
  y1 = PlotY(w, w->plot.y.DrawOrigin);
  x2 = PlotX(w, w->plot.x.DrawMax);
  y2 = PlotY(w, w->plot.y.DrawMax);

  if (w->plot.XLog) {
    val = w->plot.x.DrawOrigin;
  }
  else {
    val = w->plot.x.DrawOrigin;
  }


  majorval = val;
  while ((majorval * 1.0001) < w->plot.x.DrawMax) {
    if (w->plot.XLog) {

/* Hack to make sure that 9.99999e? still gets interpreted as 10.0000e? */
      if (majorval * 1.1 > w->plot.x.DrawMax)
	break;
      tic = majorval;
      if (w->plot.DrawMinor) {
	for (j = 2; j < w->plot.x.MinorNum; j++) {
	  val = tic * (real) j;

	  if (val <= w->plot.x.DrawMax) {
	    x = PlotX(w, val);
	    LineSet(w, x, y1, x, y2,
		    w->plot.ForegroundColor,
		    XtLINE_WIDEDOT);
	  }
	}
      }
      val = tic * (real) w->plot.x.MinorNum;
    }
    else {
      tic = majorval;
      if (w->plot.DrawMinor) {
	for (j = 1; j < w->plot.x.MinorNum; j++) {
	  val = tic + w->plot.x.MajorInc * (real) j /
	    w->plot.x.MinorNum;

	  if (val <= w->plot.x.DrawMax) {
	    x = PlotX(w, val);
	    LineSet(w, x, y1, x, y2,
		    w->plot.ForegroundColor,
		    XtLINE_WIDEDOT);
	  }
	}
      }
      val = tic + w->plot.x.MajorInc;
    }


    if (val <= w->plot.x.DrawMax) {
      x = PlotX(w, val);
      if (w->plot.DrawMajor)
	LineSet(w, x, y1, x, y2, w->plot.ForegroundColor,
		XtLINE_DOTTED);
      else if (w->plot.DrawMinor)
	LineSet(w, x, y1, x, y2, w->plot.ForegroundColor,
		XtLINE_WIDEDOT);
    }
    majorval = val;
  }
}


static void
DrawCartesianYMajorAndMinor (SciPlotWidget w)
{
  real y, x1, y1, x2, y2, tic, val, majorval;
  int j;

  if (!w->plot.DrawMajor && !w->plot.DrawMinor)
    return;

  w->plot.current_id = SciPlotDrawingYMajorMinor;
  x1 = PlotX(w, w->plot.x.DrawOrigin);
  y1 = PlotY(w, w->plot.y.DrawOrigin);
  x2 = PlotX(w, w->plot.x.DrawMax);
  y2 = PlotY(w, w->plot.y.DrawMax);

  /* draw y direction */
  if (w->plot.YLog) {
    val = w->plot.y.DrawOrigin;
  }
  else
    val = w->plot.y.DrawOrigin;

  majorval = val;

/* majorval*1.0001 is a fudge to get rid of rounding errors that seem to
 * occur when continuing to add the major axis increment.
 */
  while ((majorval * 1.0001) < w->plot.y.DrawMax) {
    if (w->plot.YLog) {

/* Hack to make sure that 9.99999e? still gets interpreted as 10.0000e? */
      if (majorval * 1.1 > w->plot.y.DrawMax)
	break;
      tic = majorval;
      if (w->plot.DrawMinor) {
	for (j = 2; j < w->plot.y.MinorNum; j++) {
	  val = tic * (real) j;

	  if (val <= w->plot.y.DrawMax) {
	    y = PlotY(w, val);
	    LineSet(w, x1, y, x2, y,
		    w->plot.ForegroundColor,
		    XtLINE_WIDEDOT);
	  }
	}
      }
      val = tic * (real) w->plot.y.MinorNum;
    }
    else {
      tic = majorval;
      if (w->plot.DrawMinor) {
	for (j = 1; j < w->plot.y.MinorNum; j++) {
	  val = tic + w->plot.y.MajorInc * (real) j /
	    w->plot.y.MinorNum;

	  if (val <= w->plot.y.DrawMax) {
	    y = PlotY(w, val);
	    if (w->plot.DrawMinor)
	      LineSet(w, x1, y, x2, y,
		      w->plot.ForegroundColor,
		      XtLINE_WIDEDOT);
	  }
	}
      }
      val = tic + w->plot.y.MajorInc;
    }

    if (val <= w->plot.y.DrawMax) {
      y = PlotY(w, val);
      if (w->plot.DrawMajor)
	LineSet(w, x1, y, x2, y, w->plot.ForegroundColor,
		XtLINE_DOTTED);
      else if (w->plot.DrawMinor)
	LineSet(w, x1, y, x2, y, w->plot.ForegroundColor,
		XtLINE_WIDEDOT);
    }
    majorval = val;
  }
}

static void
DrawCartesianXAxis (SciPlotWidget w, int type)
{
  real x, x1, y1, x2, y2, tic, val, height, majorval;
  int j, precision;
  char numberformat[16], label[16];

  w->plot.current_id = SciPlotDrawingXAxis;
  height = FontnumHeight(w, w->plot.labelFont);
  x1 = PlotX(w, w->plot.x.DrawOrigin);
  y1 = PlotY(w, w->plot.y.DrawOrigin);
  x2 = PlotX(w, w->plot.x.DrawMax);
  y2 = PlotY(w, w->plot.y.DrawMax);
  LineSet(w, x1, y1, x2, y1, w->plot.ForegroundColor, XtLINE_SOLID);

#if DEBUG_SCIPLOT_AXIS
  SciPlotPrintf("\nDrawCartesianXAxis: x1=%f y1=%f x2=%f y2=%f\n"
    "  Width=%d Height=%d\n"
    "  MajorInc=%f MajorNum=%d MinorNum=%d Precision=%d\n"
    "  Origin=%f Size=%f Center=%f AxisPos=%f\n"
    "  DrawOrigin=%f DrawSize=%f DrawMax=%f\n",
    x1,y1,x2,y2,
    w->core.width,w->core.height,
    w->plot.x.MajorInc,w->plot.x.MajorNum,w->plot.x.MinorNum,w->plot.x.Precision,
    w->plot.x.Origin,w->plot.x.Size,w->plot.x.Center,w->plot.x.AxisPos,
    w->plot.x.DrawOrigin,w->plot.x.DrawSize,w->plot.x.DrawMax);
#endif

  precision = w->plot.x.Precision;
  sprintf(numberformat, "%%.%df", precision);
  if (w->plot.XLog) {
    val = w->plot.x.DrawOrigin;
    if (precision > 0)
      precision--;
  }
  else {
    val = w->plot.x.DrawOrigin;
  }
  x = PlotX(w, val);
  if (w->plot.DrawMajorTics)
    LineSet(w, x, y1 + 5, x, y1 - 5, w->plot.ForegroundColor, XtLINE_SOLID);
  if (w->plot.XAxisNumbers) {
    sprintf(label, numberformat, val);
#if DEBUG_SCIPLOT_AXIS
    SciPlotPrintf("  TextSet: x=%f y=%f val=%f label=%s\n",
      x,w->plot.y.AxisPos,val,label);
#endif
    TextSet(w, x, w->plot.y.AxisPos, label, w->plot.ForegroundColor,
      w->plot.axisFont);
  }

  majorval = val;
  while ((majorval * 1.0001) < w->plot.x.DrawMax) {
    if (w->plot.XLog) {

/* Hack to make sure that 9.99999e? still gets interpreted as 10.0000e? */
      if (majorval * 1.1 > w->plot.x.DrawMax)
	break;
      tic = majorval;
      if (w->plot.DrawMinorTics) {
	for (j = 2; j < w->plot.x.MinorNum; j++) {
	  val = tic * (real) j;

	  if (val < w->plot.x.DrawMax) {
	    x = PlotX(w, val);
	    if (w->plot.DrawMinorTics)
	      LineSet(w, x, y1, x, y1 - 3,
		      w->plot.ForegroundColor,
		      XtLINE_SOLID);
	  }
	}
      }
      val = tic * (real) w->plot.x.MinorNum;
      sprintf(numberformat, "%%.%df", precision);
      if (precision > 0)
	precision--;
    }
    else {
      tic = majorval;
      if (w->plot.DrawMinorTics) {
	for (j = 1; j < w->plot.x.MinorNum; j++) {
	  val = tic + w->plot.x.MajorInc * (real) j /
	    w->plot.x.MinorNum;
	  if (val <= w->plot.x.DrawMax) {
	    x = PlotX(w, val);
	    if (w->plot.DrawMinorTics)
	      LineSet(w, x, y1, x, y1 - 3,
		      w->plot.ForegroundColor,
		      XtLINE_SOLID);
	  }
	}
      }
      val = tic + w->plot.x.MajorInc;
    }

    if (val <= w->plot.x.DrawMax) {
      x = PlotX(w, val);
      if (w->plot.DrawMajorTics)
	LineSet(w, x, y1 + 5, x, y1 - 5, w->plot.ForegroundColor,
		XtLINE_SOLID);
      if (w->plot.XAxisNumbers) {
	sprintf(label, numberformat, val);
#if DEBUG_SCIPLOT_AXIS
	SciPlotPrintf("  TextSet: x=%f y=%f val=%f label=%s\n",
	  x,w->plot.y.AxisPos,val,label);
#endif
	TextSet(w, x, w->plot.y.AxisPos, label, w->plot.ForegroundColor,
		w->plot.axisFont);
      }
    }
    majorval = val;
  }
  if (type == DRAW_ALL) {
    DrawCartesianXMajorAndMinor (w);
    DrawCartesianXLabelAndTitle (w);
  }
  else if (type == DRAW_NO_LABELS)
    DrawCartesianXMajorAndMinor (w);
}

static void
DrawCartesianYAxis (SciPlotWidget w, int type)
{
  real y, x1, y1, x2, y2, tic, val, height, majorval;
  int j, precision;
  char numberformat[16], label[16];

  w->plot.current_id = SciPlotDrawingYAxis;
  height = FontnumHeight(w, w->plot.labelFont);
  x1 = PlotX(w, w->plot.x.DrawOrigin);
  y1 = PlotY(w, w->plot.y.DrawOrigin);
  x2 = PlotX(w, w->plot.x.DrawMax);
  y2 = PlotY(w, w->plot.y.DrawMax);
  LineSet(w, x1, y1, x1, y2, w->plot.ForegroundColor, XtLINE_SOLID);

  precision = w->plot.y.Precision;
  sprintf(numberformat, "%%.%df", precision);
  if (w->plot.YLog) {
    val = w->plot.y.DrawOrigin;
    if (precision > 0)
      precision--;
  }
  else {
    val = w->plot.y.DrawOrigin;
  }
  y = PlotY(w, val);
  if (w->plot.DrawMajorTics)
    LineSet(w, x1 + 5, y, x1 - 5, y, w->plot.ForegroundColor, XtLINE_SOLID);
  if (w->plot.YAxisNumbers) {
    sprintf(label, numberformat, val);
    if (w->plot.YNumHorz) {
      y+=FontnumHeight(w, w->plot.axisFont)/2.0 -
        FontnumDescent(w, w->plot.axisFont);
      TextSet(w,
        w->plot.x.AxisPos - FontnumTextWidth(w, w->plot.axisFont, label),
        y, label, w->plot.ForegroundColor,
        w->plot.axisFont);
    }
    else {
      VTextSet(w, w->plot.x.AxisPos, y, label, w->plot.ForegroundColor,
        w->plot.axisFont);
    }
  }
  majorval = val;

/* majorval*1.0001 is a fudge to get rid of rounding errors that seem to
 * occur when continuing to add the major axis increment.
 */
  while ((majorval * 1.0001) < w->plot.y.DrawMax) {
    if (w->plot.YLog) {

/* Hack to make sure that 9.99999e? still gets interpreted as 10.0000e? */
      if (majorval * 1.1 > w->plot.y.DrawMax)
	break;
      tic = majorval;
      if (w->plot.DrawMinorTics) {
	for (j = 2; j < w->plot.y.MinorNum; j++) {
	  val = tic * (real) j;

	  if (val <= w->plot.y.DrawMax) {
	    y = PlotY(w, val);
	    if (w->plot.DrawMinorTics)
	      LineSet(w, x1, y, x1 + 3, y,
		      w->plot.ForegroundColor,
		      XtLINE_SOLID);
	  }
	}
      }
      val = tic * (real) w->plot.y.MinorNum;
      sprintf(numberformat, "%%.%df", precision);
      if (precision > 0)
	precision--;
    }
    else {
      tic = majorval;
      if (w->plot.DrawMinorTics) {
	for (j = 1; j < w->plot.y.MinorNum; j++) {
	  val = tic + w->plot.y.MajorInc * (real) j /
	    w->plot.y.MinorNum;

	  if (val <= w->plot.y.DrawMax) {
	    y = PlotY(w, val);
	    if (w->plot.DrawMinorTics)
	      LineSet(w, x1, y, x1 + 3, y,
		      w->plot.ForegroundColor,
		      XtLINE_SOLID);
	  }
	}
      }
      val = tic + w->plot.y.MajorInc;
    }

    if (val <= w->plot.y.DrawMax) {
      y = PlotY(w, val);
      if (w->plot.DrawMajorTics)
	LineSet(w, x1 - 5, y, x1 + 5, y, w->plot.ForegroundColor,
		XtLINE_SOLID);
      if (w->plot.YAxisNumbers) {
	sprintf(label, numberformat, val);
	if (w->plot.YNumHorz) {
	  y+=FontnumHeight(w, w->plot.axisFont)/2.0 -
	    FontnumDescent(w, w->plot.axisFont);
	  TextSet(w,
		  w->plot.x.AxisPos - FontnumTextWidth(w, w->plot.axisFont, label),
		  y, label, w->plot.ForegroundColor,
		  w->plot.axisFont);
	}
	else {
	  VTextSet(w, w->plot.x.AxisPos, y, label, w->plot.ForegroundColor,
		   w->plot.axisFont);
	}
      }
    }
    majorval = val;
  }

  if (type == DRAW_ALL) {
    DrawCartesianYMajorAndMinor (w);
    DrawCartesianYLabelAndTitle (w);
  }
  else if (type == DRAW_NO_LABELS)
    DrawCartesianYMajorAndMinor (w);
}


static void
DrawCartesianAxes (SciPlotWidget w, int type)
{
  DrawCartesianXAxis (w, type);
  DrawCartesianYAxis (w, type);
}

static void
DrawCartesianPlot (SciPlotWidget w)
{
  int i, j, jstart;
  SciPlotList *p;

  w->plot.current_id = SciPlotDrawingAny;
  ClipSet(w);
  w->plot.current_id = SciPlotDrawingLine;
  for (i = 0; i < w->plot.num_plotlist; i++) {
    p = w->plot.plotlist + i;
    if (p->draw) {
      real x1, y1, x2, y2;
      Boolean skipnext=False;

      jstart = 0;
      while ((jstart < p->number) &&
        (((p->data[jstart].x <= SCIPLOT_SKIP_VAL ||
           p->data[jstart].y <= SCIPLOT_SKIP_VAL) ||
          (w->plot.XLog && (p->data[jstart].x <= 0.0)) ||
          (w->plot.YLog && (p->data[jstart].y <= 0.0)))))
        jstart++;
      if (jstart < p->number) {
        x1 = PlotX(w, p->data[jstart].x);
        y1 = PlotY(w, p->data[jstart].y);
      }
      for (j = jstart; j < p->number; j++) {
        if (p->data[j].x <= SCIPLOT_SKIP_VAL ||
            p->data[j].y <= SCIPLOT_SKIP_VAL) {
          skipnext=True;
          continue;
        }

	if (!((w->plot.XLog && (p->data[j].x <= 0.0)) ||
	    (w->plot.YLog && (p->data[j].y <= 0.0)))) {
	  x2 = PlotX(w, p->data[j].x);
	  y2 = PlotY(w, p->data[j].y);
          if (!skipnext)
            LineSet(w, x1, y1, x2, y2, p->LineColor, p->LineStyle);
	  x1 = x2;
	  y1 = y2;
	}

        skipnext=False;
      }
    }
  }
  w->plot.current_id = SciPlotDrawingAny;
  ClipClear(w);
  w->plot.current_id = SciPlotDrawingLine;
  for (i = 0; i < w->plot.num_plotlist; i++) {
    p = w->plot.plotlist + i;
    if (p->draw) {
      real x2, y2;

      for (j = 0; j < p->number; j++) {
	if (!((w->plot.XLog && (p->data[j].x <= 0.0)) ||
	    (w->plot.YLog && (p->data[j].y <= 0.0)) ||
             p->data[j].x <= SCIPLOT_SKIP_VAL ||
             p->data[j].y <= SCIPLOT_SKIP_VAL )) {
	  x2 = PlotX(w, p->data[j].x);
	  y2 = PlotY(w, p->data[j].y);
	  if ((x2 >= w->plot.x.Origin) &&
	    (x2 <= w->plot.x.Origin + w->plot.x.Size) &&
	    (y2 >= w->plot.y.Origin) &&
	    (y2 <= w->plot.y.Origin + w->plot.y.Size)) {

	    DrawMarker(w, x2, y2,
	      p->markersize,
	      p->PointColor,
	      p->PointStyle);

	    if (p->PointStyle == XtMARKER_HTEXT && p->markertext) {
	      if (p->markertext[j])
		TextSet (w, x2, y2, p->markertext[j], p->PointColor,
			 w->plot.axisFont);
	    }
	    else if (p->PointStyle == XtMARKER_VTEXT && p->markertext) {
	      if (p->markertext[j])
		VTextSet (w, x2, y2, p->markertext[j], p->PointColor,
			  w->plot.axisFont);
	    }
	  }

	}
      }
    }
  }
}

static void
DrawPolarAxes (SciPlotWidget w)
{
  real x1, y1, x2, y2, max, tic, val, height;
  int i, j;
  char numberformat[16], label[16];

  w->plot.current_id = SciPlotDrawingPAxis;
  sprintf(numberformat, "%%.%df", w->plot.x.Precision);
  height = FontnumHeight(w, w->plot.labelFont);
  max = w->plot.PolarScale;
  PlotRTDegrees(w, 0.0, 0.0, &x1, &y1);
  PlotRTDegrees(w, max, 0.0, &x2, &y2);
  LineSet(w, x1, y1, x2, y2, 1, XtLINE_SOLID);
  for (i = 45; i < 360; i += 45) {
    PlotRTDegrees(w, max, (real) i, &x2, &y2);
    LineSet(w, x1, y1, x2, y2, w->plot.ForegroundColor, XtLINE_DOTTED);
  }
  for (i = 1; i <= w->plot.x.MajorNum; i++) {
    tic = w->plot.PolarScale *
      (real) i / (real) w->plot.x.MajorNum;
    if (w->plot.DrawMinor || w->plot.DrawMinorTics) {
      for (j = 1; j < w->plot.x.MinorNum; j++) {
	val = tic - w->plot.x.MajorInc * (real) j /
	  w->plot.x.MinorNum;
	PlotRTDegrees(w, val, 0.0, &x2, &y2);
	if (w->plot.DrawMinor)
	  CircleSet(w, x1, y1, x2 - x1,
	    w->plot.ForegroundColor, XtLINE_WIDEDOT);
	if (w->plot.DrawMinorTics)
	  LineSet(w, x2, y2 - 2.5, x2, y2 + 2.5,
	    w->plot.ForegroundColor, XtLINE_SOLID);
      }
    }
    PlotRTDegrees(w, tic, 0.0, &x2, &y2);
    if (w->plot.DrawMajor)
      CircleSet(w, x1, y1, x2 - x1, w->plot.ForegroundColor, XtLINE_DOTTED);
    if (w->plot.DrawMajorTics)
      LineSet(w, x2, y2 - 5.0, x2, y2 + 5.0, w->plot.ForegroundColor, XtLINE_SOLID);
    if (w->plot.XAxisNumbers) {
      sprintf(label, numberformat, tic);
      TextSet(w, x2, y2 + height, label, w->plot.ForegroundColor, w->plot.axisFont);
    }
  }

  if (w->plot.ShowTitle)
    TextSet(w, w->plot.x.TitlePos, w->plot.y.TitlePos,
      w->plot.plotTitle, w->plot.ForegroundColor, w->plot.titleFont);
}

static void
DrawPolarPlot (SciPlotWidget w)
{
  int i, j;
  SciPlotList *p;

  w->plot.current_id = SciPlotDrawingLine;
  for (i = 0; i < w->plot.num_plotlist; i++) {
    p = w->plot.plotlist + i;
    if (p->draw) {
      int jstart;
      real x1, y1, x2, y2;
      Boolean skipnext=False;

      jstart = 0;
      while ((jstart < p->number) &&
        (p->data[jstart].x <= SCIPLOT_SKIP_VAL ||
          p->data[jstart].y <= SCIPLOT_SKIP_VAL))
        jstart++;
      if (jstart < p->number) {
        PlotRT(w, p->data[0].x, p->data[0].y, &x1, &y1);
      }
      for (j = jstart; j < p->number; j++) {
        if (p->data[j].x <= SCIPLOT_SKIP_VAL ||
            p->data[j].y <= SCIPLOT_SKIP_VAL) {
          skipnext=True;
          continue;
        }

	PlotRT(w, p->data[j].x, p->data[j].y, &x2, &y2);
        if (!skipnext) {
          LineSet(w, x1, y1, x2, y2,
            p->LineColor, p->LineStyle);
          DrawMarker(w, x1, y1, p->markersize,
            p->PointColor, p->PointStyle);
          DrawMarker(w, x2, y2, p->markersize,
            p->PointColor, p->PointStyle);
        }
	x1 = x2;
	y1 = y2;

        skipnext=False;
      }
    }
  }
}

static void
DrawAll (SciPlotWidget w)
{
#ifdef MOTIF
  if(w->primitive.shadow_thickness > 0) {
  /* Just support shadow in, out or none (not etched in or out) */
    if(w->plot.ShadowType == XmSHADOW_OUT) {
      DrawShadow(w, True, 0, 0, w->core.width, w->core.height);
    } else if (w->plot.ShadowType == XmSHADOW_IN) {
      DrawShadow(w, False, 0, 0, w->core.width, w->core.height);
    }
  }
#endif
  if (w->plot.ChartType == XtCARTESIAN) {
    DrawCartesianAxes(w, DRAW_ALL);
    DrawLegend(w);
    DrawCartesianPlot(w);
  }
  else {
    DrawPolarAxes(w);
    DrawLegend(w);
    DrawPolarPlot(w);
  }
}

static Boolean
DrawQuick (SciPlotWidget w)
{
  Boolean range_check;

  if (w->plot.XNoCompMinMax)
    range_check = CheckMinMax(w, NO_COMPUTE_MIN_MAX_X);
  else
    range_check = CheckMinMax(w, COMPUTE_MIN_MAX);
  EraseClassItems(w, SciPlotDrawingLine);
  EraseAllItems(w);
  DrawAll(w);

  return range_check;
}


/*
 * Public Plot functions
 */

int
SciPlotAllocNamedColor (Widget wi, char *name)
{
  XColor used, exact;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return -1;

  w = (SciPlotWidget) wi;

  if (!XAllocNamedColor(XtDisplay(w), w->plot.cmap, name, &used, &exact))
    return 1;
  return ColorStore(w, used.pixel);
}

int
SciPlotAllocRGBColor (Widget wi, int r, int g, int b)
{
  XColor used;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return -1;

  w = (SciPlotWidget) wi;

  used.pixel = 0;
  r *= 256;
  g *= 256;
  b *= 256;
  if (r > 65535)
    r = 65535;
  if (g > 65535)
    g = 65535;
  if (b > 65535)
    b = 65535;
  used.red = (unsigned short) r;
  used.green = (unsigned short) g;
  used.blue = (unsigned short) b;
  if (!XAllocColor(XtDisplay(w), w->plot.cmap, &used))
    return 1;
  return ColorStore(w, used.pixel);
}

int
SciPlotAllocColor (Widget wi, Pixel pix)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return -1;

  w = (SciPlotWidget) wi;

  return ColorStore(w, pix);
}


void
SciPlotSetBackgroundColor (Widget wi, int color)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;
  if (color < w->plot.num_colors) {
    w->plot.BackgroundColor = color;
    w->core.background_pixel = w->plot.colors[color];
    XSetWindowBackground( XtDisplay(w), XtWindow(w), w->core.background_pixel);
#ifdef MOTIF
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
#endif
  }
}

void
SciPlotSetForegroundColor (Widget wi, int color)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;
  if (color < w->plot.num_colors)
    w->plot.ForegroundColor = color;
}

int
NumberAllocatedColors (Widget wi)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return 0;

  w = (SciPlotWidget)wi;
  return w->plot.num_colors;
}

Pixel
PixelValue (Widget wi, int color)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return 0;

  w = (SciPlotWidget)wi;

  if (color < 0 || color > w->plot.num_colors)
    return 0;

  return w->plot.colors[color];
}

void
SciPlotListDelete (Widget wi, int idnum)
{
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;

  p = _ListFind(w, idnum);
  if (p)
    _ListDelete(p);
}

int
SciPlotListCreateFromData (Widget wi, int num, real *xlist, real *ylist, char *legend, int pcolor, int pstyle, int lcolor, int lstyle)
{
  int idnum;
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return -1;

  w = (SciPlotWidget) wi;

  idnum = _ListNew(w);
  p = w->plot.plotlist + idnum;
  _ListSetReal(p, num, xlist, ylist);
  _ListSetLegend(p, legend);
  _ListSetStyle(p, pcolor, pstyle, lcolor, lstyle);
  return idnum;
}

int
SciPlotListCreateFloat (Widget wi, int num, float *xlist, float *ylist, char *legend)
{
  int idnum;
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return -1;

  w = (SciPlotWidget) wi;

  idnum = _ListNew(w);
  p = w->plot.plotlist + idnum;
  _ListSetFloat(p, num, xlist, ylist);
  _ListSetLegend(p, legend);
  _ListSetStyle(p, 1, XtMARKER_CIRCLE, 1, XtLINE_SOLID);
  return idnum;
}

void
SciPlotListUpdateFloat (Widget wi, int idnum, int num, float *xlist, float *ylist)
{
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;

  p = _ListFind(w, idnum);
  if (p)
    _ListSetFloat(p, num, xlist, ylist);
}

void
SciPlotListAddFloat (Widget wi, int idnum, int num, float *xlist, float *ylist)
{
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;

  p = _ListFind(w, idnum);
  if (p)
    _ListAddFloat(p, num, xlist, ylist);
}

int
SciPlotListCreateDouble (Widget wi, int num, double *xlist, double *ylist, char *legend)
{
  int idnum;
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return -1;

  w = (SciPlotWidget) wi;

  idnum = _ListNew(w);
  p = w->plot.plotlist + idnum;
  _ListSetDouble(p, num, xlist, ylist);
  _ListSetLegend(p, legend);
  _ListSetStyle(p, 1, XtMARKER_CIRCLE, 1, XtLINE_SOLID);
  return idnum;
}

void
SciPlotListUpdateDouble (Widget wi, int idnum, int num, double *xlist, double *ylist)
{
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;

  p = _ListFind(w, idnum);
  if (p)
    _ListSetDouble(p, num, xlist, ylist);
}

void
SciPlotListAddDouble (Widget wi, int idnum, int num, double *xlist, double *ylist)
{
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;

  p = _ListFind(w, idnum);
  if (p)
    _ListAddDouble(p, num, xlist, ylist);
}

void
SciPlotListSetStyle (Widget wi, int idnum, int pcolor, int pstyle, int lcolor, int lstyle)
{
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;

  p = _ListFind(w, idnum);
  if (p)
    _ListSetStyle(p, pcolor, pstyle, lcolor, lstyle);
}

void
SciPlotListSetMarkerSize (Widget wi, int idnum, float size)
{
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;

  p = _ListFind(w, idnum);
  if (p)
    p->markersize=size;
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

void
SciPlotSetXAutoScale (Widget wi)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;
  w->plot.XAutoScale = True;
}

void
SciPlotSetXUserScale (Widget wi, double min, double max)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;
  if (min < max) {
    w->plot.XAutoScale = False;
    w->plot.UserMin.x = (real) min;
    w->plot.UserMax.x = (real) max;
  }
}

void
SciPlotSetXUserMin (Widget wi, double min)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;
  w->plot.XAutoScale = False;
  w->plot.UserMin.x = (real) min;
}

void
SciPlotSetXUserMax (Widget wi, double max)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;
  w->plot.XAutoScale = False;
  w->plot.UserMax.x = (real) max;
}

void
SciPlotSetYAutoScale (Widget wi)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;
  w->plot.YAutoScale = True;
}

void
SciPlotSetYUserScale (Widget wi, double min, double max)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;
  if (min < max) {
    w->plot.YAutoScale = False;
    w->plot.UserMin.y = (real) min;
    w->plot.UserMax.y = (real) max;
  }
}

void
SciPlotSetYUserMin (Widget wi, double min)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;
  w->plot.YAutoScale = False;
  w->plot.UserMin.y = (real) min;
}

void
SciPlotSetYUserMax (Widget wi, double max)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;
  w->plot.YAutoScale = False;
  w->plot.UserMax.y = (real) max;
}

void
SciPlotGetXScale (Widget wi, float* min, float* max)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  if (w->plot.XAutoScale) {
    *min = w->plot.Min.x;
    *max = w->plot.Max.x;
  }
  else {
    *min = w->plot.UserMin.x;
    *max = w->plot.UserMax.x;
  }
}

void
SciPlotGetYScale (Widget wi, float* min, float* max)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  if (w->plot.YAutoScale) {
    *min = w->plot.Min.y;
    *max = w->plot.Max.y;
  }
  else {
    *min = w->plot.UserMin.y;
    *max = w->plot.UserMax.y;
  }
}



void
SciPlotPrintStatistics (Widget wi)
{
  int i, j;
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;

  SciPlotPrintf("Title=%s\nxlabel=%s\tylabel=%s\n",
    w->plot.plotTitle, w->plot.xlabel, w->plot.ylabel);
  SciPlotPrintf("ChartType=%d\n", w->plot.ChartType);
  SciPlotPrintf("Degrees=%d\n", w->plot.Degrees);
  SciPlotPrintf("XLog=%d\tYLog=%d\n", w->plot.XLog, w->plot.YLog);
  SciPlotPrintf("XAutoScale=%d\tYAutoScale=%d\n",
    w->plot.XAutoScale, w->plot.YAutoScale);
  for (i = 0; i < w->plot.num_plotlist; i++) {
    p = w->plot.plotlist + i;
    if (p->draw) {
      SciPlotPrintf("\nLegend=%s\n", p->legend);
      SciPlotPrintf("Styles: point=%d line=%d  Color: point=%d line=%d\n",
	p->PointStyle, p->LineStyle, p->PointColor, p->LineColor);
      for (j = 0; j < p->number; j++)
	SciPlotPrintf("%f\t%f\n", p->data[j].x, p->data[j].y);
      SciPlotPrintf("\n");
    }
  }
}

void
SciPlotExportData (Widget wi, FILE *fd)
{
  int i, j;
  SciPlotList *p;
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;

  fprintf(fd, "Title=\"%s\"\n", w->plot.plotTitle);
  fprintf(fd, "Xaxis=\"%s\"\n", w->plot.xlabel);
  fprintf(fd, "Yaxis=\"%s\"\n\n", w->plot.ylabel);
  for (i = 0; i < w->plot.num_plotlist; i++) {
    p = w->plot.plotlist + i;
    if (p->draw) {
      fprintf(fd, "Line=\"%s\"\n",p->legend);
      for (j = 0; j < p->number; j++)
	fprintf(fd, "%e\t%e\n", p->data[j].x, p->data[j].y);
      fprintf(fd, "\n");
    }
  }
}

int
SciPlotStoreAllocatedColor(Widget wi, Pixel p)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return -1;

  w = (SciPlotWidget) wi;

  return ColorStore(w, p);
}

void
SciPlotUpdate (Widget wi)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi)) {
    return;
  }

  w = (SciPlotWidget) wi;
  EraseAll(w);

#if DEBUG_SCIPLOT
  SciPlotPrintStatistics(wi);
#endif
  if (w->plot.XNoCompMinMax) {
    ComputeAll(w, NO_COMPUTE_MIN_MAX_X);
  }
  else {
    ComputeAll(w, COMPUTE_MIN_MAX);
  }
  DrawAll(w);
}

void
SciPlotUpdateSimple (Widget wi, int type)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget) wi;
  EraseAll(w);
#if DEBUG_SCIPLOT
  SciPlotPrintStatistics(wi);
#endif
  ComputeAll(w, type);
  DrawAll(w);
}

Boolean
SciPlotQuickUpdate (Widget wi)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return False;

  w = (SciPlotWidget) wi;
  return DrawQuick(w);
}

/****************************************************************************
 *            Addtional functions
 ***************************************************************************/

/***************************************************************************
 *      Private data conversion from screen to data coordination
 ***************************************************************************/
static real
OutputX (SciPlotWidget w, real xin)
{
  real xout;

  if (w->plot.XLog)
    xout = pow (10.0, log10(w->plot.x.DrawOrigin) +
		 (xin - w->plot.x.Origin) *
		 (w->plot.x.DrawSize/ w->plot.x.Size));
  else
    xout = w->plot.x.DrawOrigin +
      ((xin - w->plot.x.Origin) *
       (w->plot.x.DrawSize/ w->plot.x.Size));
  return xout;
}

static real
OutputY (SciPlotWidget w, real yin)
{
  real yout;

  if (w->plot.YLog)
    yout = pow (10.0, log10(w->plot.y.DrawOrigin) +
		 (w->plot.y.Origin + w->plot.y.Size - yin) *
		 (w->plot.y.DrawSize/ w->plot.y.Size));
  else
    yout = w->plot.y.DrawOrigin +
      ((w->plot.y.Origin + w->plot.y.Size - yin) *
       (w->plot.y.DrawSize/ w->plot.y.Size));
  return yout;
}

#if 0
/* Not used */
static void
ComputeXMinMax_i (SciPlotWidget w, real* xmin, real* xmax)
{
  register int i, j;
  register SciPlotList *p;
  register real val;
  Boolean firstx;
  real     tmin = 1.0;
  real     tmax = 1.0;


  firstx = True;

  for (i = 0; i < w->plot.num_plotlist; i++) {
    p = w->plot.plotlist + i;
    if (p->draw) {
      for (j = 0; j < p->number; j++) {

          /* Don't count the "break in line segment" flag for Min/Max */
        if (p->data[j].x > SCIPLOT_SKIP_VAL &&
            p->data[j].y > SCIPLOT_SKIP_VAL) {

          val = p->data[j].x;
          if (!w->plot.XLog || (w->plot.XLog && (val > 0.0))) {
            if (firstx) {
              tmin = tmax = val;
              firstx = False;
            }
            else {
              if (val > tmax)
                tmax = val;
              else if (val < tmin)
                tmin = val;
            }
          }

	}
      }
    }
  }

    /* fix defaults if there is only one point. */
  if (firstx) {
    if (w->plot.XLog) {
      tmin = 1.0;
      tmax = 10.0;
    }
    else {
      tmin  = 0.0;
      tmax = 10.0;
    }
  }

  if (w->plot.ChartType == XtCARTESIAN) {
    if (!w->plot.XLog) {
      if (!w->plot.XAutoScale) {
	tmin = w->plot.UserMin.x;
	tmax = w->plot.UserMax.x;
      }
      else if (w->plot.XOrigin) {
	if (tmin > 0.0)
	  tmin = 0.0;
	if (tmax < 0.0)
	  tmax = 0.0;
      }
      if (fabs(tmin - tmax) < 1.e-10) {
	tmin -= .5;
	tmax += .5;
      }
    }
  }
  else {
    if (tmin > tmax)
      tmax = fabs(tmin);
  }
  *xmin = tmin;
  *xmax = tmax;
}
#endif

static void
ComputeAxis_i (SciPlotAxis *axis, real min, real max, Boolean log,
	       real* drawOrigin, real* drawMax)
{
  real range, rnorm, delta, calcmin, calcmax;
  int nexp, majornum, minornum, majordecimals, i;

  range = max - min;
  if (log) {
    if (range==0.0) {
      calcmin = powi(10.0, (int) floor(log10(min) + SCIPLOT_EPS));
      calcmax = 10.0*calcmin;
    }
    else {
      calcmin = powi(10.0, (int) floor(log10(min) + SCIPLOT_EPS));
      calcmax = powi(10.0, (int) ceil(log10(max) - SCIPLOT_EPS));
    }

    /*
    SciPlotPrintf("calcmin=%e min=%e   calcmax=%e max=%e\n",calcmin,min,
	   calcmax,max); */

    delta = 10.0;

    *drawOrigin = calcmin;
    *drawMax = calcmax;
  }
  else {
    if (range==0.0) nexp=0;
    else nexp = (int) floor(log10(range) + SCIPLOT_EPS);
    rnorm = range / powi(10.0, nexp);
    for (i = 0; i < NUMBER_MINOR; i++) {
      delta = CAdeltas[i];
      minornum = CAminors[i];
      majornum = (int) ((rnorm + 0.9999 * delta) / delta);
      majordecimals = CAdecimals[i];
      if (majornum <= MAX_MAJOR)
	break;
    }
    delta *= powi(10.0, nexp);
#if DEBUG_SCIPLOT
    SciPlotPrintf("nexp=%d range=%f rnorm=%f delta=%f\n", nexp, range, rnorm, delta);
#endif

    if (min < 0.0)
      calcmin = ((float) ((int) ((min - .9999 * delta) / delta))) * delta;
    else if ((min > 0.0) && (min < 1.0))
      calcmin = ((float) ((int) ((1.0001 * min) / delta))) * delta;
    else if (min >= 1.0)
      calcmin = ((float) ((int) ((.9999 * min) / delta))) * delta;
    else
      calcmin = min;
    if (max < 0.0)
      calcmax = ((float) ((int) ((.9999 * max) / delta))) * delta;
    else if (max > 0.0)
      calcmax = ((float) ((int) ((max + .9999 * delta) / delta))) * delta;
    else
      calcmax = max;

    *drawOrigin = calcmin;
    *drawMax = calcmax;
  }
}

static void
ComputeXDrawingRange_i (SciPlotWidget w, real xmin,
			real xmax, real* drawOrigin, real* drawMax)
{
  if (w->plot.ChartType == XtCARTESIAN)
    ComputeAxis_i(&w->plot.x, xmin, xmax,
		  w->plot.XLog, drawOrigin, drawMax);
  else
    ComputeAxis_i(&w->plot.x, (real) 0.0, xmax,
		  (Boolean) FALSE, drawOrigin, drawMax);
}


static void
ComputeYDrawingRange_i (SciPlotWidget w, real ymin,
			real ymax, real* drawOrigin, real* drawMax)
{
  if (w->plot.ChartType == XtCARTESIAN)
    ComputeAxis_i(&w->plot.y, ymin, ymax,
		  w->plot.YLog, drawOrigin, drawMax);
  else
    ComputeAxis_i(&w->plot.y, (real) 0.0, ymax,
		  (Boolean) FALSE, drawOrigin, drawMax);
}



static void
sciPlotMotionAP (SciPlotWidget w,
		 XEvent *event, char *args, int n_args)
{
  int x, y;
  int xorig, xend;
  int yorig, yend;
  real xscale, yscale;
  real minxlim, minylim, maxxlim, maxylim;  /* view port */
  real minxdat, minydat, maxxdat, maxydat;  /* data port */
  real newlim;
  real txmin, txmax;
  real tymin, tymax;
  real xoffset;
  real newlim0, newlim1;

#ifdef MOTIF
  if (event->type == ButtonPress)
    XmProcessTraversal((Widget)w, XmTRAVERSE_CURRENT);
#endif

  if (w->plot.ChartType == XtCARTESIAN) {  /* let's figure out cart plot */

    /* get boundary information */
    if (w->plot.XLog) {
      minxlim = log10(w->plot.x.DrawOrigin);
      maxxlim = log10(w->plot.x.DrawOrigin) + w->plot.x.DrawSize;
      minxdat = log10(w->plot.Min.x);
      maxxdat = log10(w->plot.Max.x);
    }
    else {
      minxlim = w->plot.x.DrawOrigin;
      maxxlim = w->plot.x.DrawOrigin + w->plot.x.DrawSize;
      minxdat = w->plot.Min.x;
      maxxdat = w->plot.Max.x;
    }

    if (w->plot.YLog) {
      minylim = log10(w->plot.y.DrawOrigin);
      maxylim = log10(w->plot.y.DrawOrigin) + w->plot.y.DrawSize;
      minydat = log10(w->plot.Min.y);
      maxydat = log10(w->plot.Max.y);
    }
    else {
      minylim = w->plot.y.DrawOrigin;
      maxylim = w->plot.y.DrawOrigin + w->plot.y.DrawSize;
      minydat = w->plot.Min.y;
      maxydat = w->plot.Max.y;
    }

    /* get scale factor for converting data coords to screen coords */
    xscale = (maxxlim - minxlim)/(w->plot.x.Size);
    yscale = (maxylim - minylim)/(w->plot.y.Size);

    xorig = PlotX(w, w->plot.x.DrawOrigin);
    yorig = PlotY(w, w->plot.y.DrawOrigin);
    xend  = PlotX(w, w->plot.x.DrawMax);
    yend  = PlotY(w, w->plot.y.DrawMax);


    x = event->xbutton.x;
    y = event->xbutton.y;

    switch (w->plot.drag_state) {
    case NOT_DRAGGING:
      if (x >= xorig && x <= xend && y <= yorig && y >= yend)
	w->plot.drag_state = DRAGGING_DATA;
      else if (x < xorig && y > yorig)
	w->plot.drag_state = DRAGGING_BOTTOM_AND_LEFT;
      else if (x < xorig && y <= yorig && y >= yend) {
	if (y < yend + (yorig - yend)/2)
	  w->plot.drag_state = DRAGGING_TOP;
	else
	  w->plot.drag_state = DRAGGING_BOTTOM;
      }
      else if (x >= xorig && x <= xend && y > yorig) {
	if (x < xorig + (xend - xorig)/2)
	  w->plot.drag_state = DRAGGING_LEFT;
	else
	  w->plot.drag_state = DRAGGING_RIGHT;
      }
      else
	w->plot.drag_state = DRAGGING_NOTHING;
      w->plot.drag_start.x = minxlim + (real)(x - w->plot.x.Origin)*xscale;
      /* y origin is at the top */
      w->plot.drag_start.y = minylim +
	(real)(w->plot.y.Origin + w->plot.y.Size - y)*yscale;
      break;
    case DRAGGING_DATA:
      if (w->plot.DragX) {
	if (!w->plot.XLog) {  /* not dragging for x log axis yet */
	  /* only allow x direction dragging data */
	  xoffset = minxlim + (x - w->plot.x.Origin)*xscale -
	    w->plot.drag_start.x;

	  if (xoffset != 0.0) {
	    newlim1 = maxxlim - xoffset;
	    newlim0 = minxlim - xoffset;

	    /* calculate maximum drawng range */
	    ComputeXDrawingRange_i (w, w->plot.startx, w->plot.endx,
				    &txmin, &txmax);

	    if (newlim1 <= txmax && newlim0 >= txmin)
		w->plot.reach_limit = 0;
	    else if (newlim1 > txmax)
	      newlim1 = txmax;
	    else if (newlim0 < txmin)
	      newlim0 = txmin;

	    if (!w->plot.reach_limit) {
	    /* KE: Did not use SCIPLOT_EPS here since UserMax could be smaller
	     *  than SCIPLOT_EPS.  Don't understand why ceil is used with
	     *  these values either.  Might want to use * (1.0 - SCIPLOT_EPS) */
	      w->plot.UserMax.x = ceil (newlim1);

	      if (w->plot.UserMax.x >= txmax) {
		w->plot.UserMax.x = txmax;
		w->plot.reach_limit = 1;
	      }

	      w->plot.UserMin.x = w->plot.UserMax.x - w->plot.x.DrawSize;
	      if (w->plot.UserMin.x <= txmin) {
		w->plot.UserMin.x = txmin;
		w->plot.UserMax.x = txmin + w->plot.x.DrawSize;
		w->plot.reach_limit = 1;
	      }

	      w->plot.XAutoScale = False;

	      EraseClassItems(w, SciPlotDrawingLine);
	      EraseClassItems(w, SciPlotDrawingXAxis);
	      EraseClassItems(w, SciPlotDrawingYAxis);

	      ComputeAll(w, NO_COMPUTE_MIN_MAX);

	      DrawCartesianXAxis (w, DRAW_AXIS_ONLY);
	      DrawCartesianYAxis (w, DRAW_AXIS_ONLY);
	      DrawCartesianPlot(w);
	    }
	  }
	}
      }
      break;
    case DRAGGING_RIGHT:
      if (w->plot.DragX) {
	if (x > w->plot.x.Origin)
	  newlim = minxlim + (w->plot.drag_start.x - minxlim)*
	    (w->plot.x.Size)/(x - w->plot.x.Origin);

	if (w->plot.XLog) {
	  if (x <= w->plot.x.Origin || newlim > log10 (w->plot.endx))
	    newlim = log10 (w->plot.endx);
	}
	else {
	  if (x <= w->plot.x.Origin || newlim > w->plot.endx)
	    newlim = w->plot.endx;
	}

	if (w->plot.XLog)
	  newlim = pow (10.0, newlim);

	ComputeXDrawingRange_i (w, w->plot.Min.x, newlim,
				&txmin, &txmax);

	if (txmax != w->plot.x.DrawMax) {
	  w->plot.UserMax.x = newlim;
	  w->plot.UserMin.x = w->plot.Min.x;
	  w->plot.XAutoScale = False;

	  EraseClassItems (w, SciPlotDrawingLine);
	  EraseClassItems (w, SciPlotDrawingXAxis);
	  EraseClassItems (w, SciPlotDrawingXMajorMinor);

	  ComputeAll(w, COMPUTE_MAX_ONLY);
	  DrawCartesianXAxis (w, DRAW_NO_LABELS);
	  DrawCartesianPlot(w);
	}
	else {
	  w->plot.XAutoScale = True;
	  ComputeMinMax (w, COMPUTE_MIN_MAX);
	}
      }
      break;
    case DRAGGING_LEFT:
      if (w->plot.DragX) {
	if (x < w->plot.x.Origin + w->plot.x.Size)
	  newlim = maxxlim - (maxxlim - w->plot.drag_start.x)*
	    (w->plot.x.Size)/(w->plot.x.Origin + w->plot.x.Size - x);

	if (w->plot.XLog) {
	  if (x >= w->plot.x.Origin + w->plot.x.Size
	      || newlim < log10 (w->plot.startx))
	    newlim = log10 (w->plot.startx);
	}
	else {
	  if (x >= w->plot.x.Origin + w->plot.x.Size
	      || newlim < w->plot.startx)
	    newlim = w->plot.startx;
	}

	if (w->plot.XLog)
	  newlim = pow (10.0, newlim);

	ComputeXDrawingRange_i (w, newlim, w->plot.Max.x,
				&txmin, &txmax);

	if (txmin != w->plot.x.DrawOrigin) {
	  w->plot.UserMin.x = newlim;
	  w->plot.UserMax.x = w->plot.Max.x;
	  w->plot.XAutoScale = False;

	  EraseAll(w);
	  ComputeAll(w, COMPUTE_MIN_ONLY);

	  DrawAll (w);
	}
	else {
	  w->plot.XAutoScale = True;
	  ComputeMinMax (w, COMPUTE_MIN_MAX);
	}
      }
      break;
    case DRAGGING_TOP:
      if (w->plot.DragY) {
	/* origin is at top */
	if (y < w->plot.y.Origin + w->plot.y.Size)
	  newlim = minylim + (w->plot.drag_start.y - minylim)*
	    (w->plot.y.Size)/(w->plot.y.Origin + w->plot.y.Size - y);

	if (w->plot.YLog) {
	  if (y >= w->plot.y.Origin + w->plot.y.Size ||
	      newlim > log10 (w->plot.endy))
	    newlim = log10 (w->plot.endy);
	}
	else {
	  if (y >= w->plot.y.Origin + w->plot.y.Size
	      || newlim > w->plot.endy)
	    newlim = w->plot.endy;
	}

	if (w->plot.YLog)
	  newlim = pow (10.0, newlim);

	ComputeYDrawingRange_i (w, w->plot.Min.y, newlim,
				&tymin, &tymax);

	if (tymax != w->plot.y.DrawMax) {
	  w->plot.UserMax.y = newlim;
	  w->plot.UserMin.y = w->plot.Min.y;
	  w->plot.YAutoScale = False;

	  EraseClassItems (w, SciPlotDrawingLine);
	  EraseClassItems (w, SciPlotDrawingYAxis);
	  EraseClassItems (w, SciPlotDrawingYMajorMinor);

	  ComputeAll(w, COMPUTE_MAX_ONLY);
	  DrawCartesianYAxis (w, DRAW_NO_LABELS);
	  DrawCartesianPlot(w);
	}
	else {
	  w->plot.YAutoScale = True;
	  ComputeMinMax (w, COMPUTE_MIN_MAX);
	}
      }
      break;
    case DRAGGING_BOTTOM:
      if (w->plot.DragY) {
	/* origin is at top */
	if (y > w->plot.y.Origin)
	  newlim = maxylim - (maxylim - w->plot.drag_start.y)*
	    (w->plot.y.Size)/(y - w->plot.y.Origin);

	if (w->plot.YLog) {
	  if (y < w->plot.y.Origin || newlim < log10 (w->plot.starty))
	    newlim = log10 (w->plot.starty);
	}
	else {
	  if (y < w->plot.y.Origin|| newlim < w->plot.starty)
	    newlim = w->plot.starty;
	}

	if (w->plot.YLog)
	  newlim = pow (10.0, newlim);

	ComputeYDrawingRange_i (w, newlim, w->plot.Max.y,
				&tymin, &tymax);
	if (tymin != w->plot.y.DrawOrigin) {
	  w->plot.UserMax.y = w->plot.Max.y;
	  w->plot.UserMin.y = newlim;
	  w->plot.YAutoScale = False;

	  EraseClassItems (w, SciPlotDrawingLine);
	  EraseClassItems (w, SciPlotDrawingYAxis);
	  EraseClassItems (w, SciPlotDrawingYMajorMinor);

	  ComputeAll(w, COMPUTE_MIN_ONLY);
	  DrawCartesianYAxis (w, DRAW_NO_LABELS);
	  DrawCartesianPlot(w);
	}
	else {
	  w->plot.YAutoScale = True;
	  ComputeMinMax (w, COMPUTE_MIN_MAX);
	}
      }
      break;
    default:
      break;
    }
  }

}

static void sciPlotBtnUpAP (SciPlotWidget w,
			    XEvent *event, char *args, int n_args)
{
  SciPlotDragCbkStruct cbk;

  if (w->plot.DragX) {
    if (w->plot.drag_state == DRAGGING_LEFT ||
	w->plot.drag_state == DRAGGING_RIGHT ||
	w->plot.drag_state == DRAGGING_DATA) {
      cbk.action = w->plot.drag_state;
      cbk.event = event;
      cbk.drawMin = w->plot.x.DrawOrigin;
      cbk.drawMax = w->plot.x.DrawMax;
      XtCallCallbacks ((Widget)w, XtNdragXCallback, &cbk);
    }
  }

  if (w->plot.DragY) {
    if (w->plot.drag_state == DRAGGING_TOP ||
	w->plot.drag_state == DRAGGING_BOTTOM) {
      cbk.action = w->plot.drag_state;
      cbk.event = event;
      cbk.drawMin = w->plot.x.DrawOrigin;
      cbk.drawMax = w->plot.x.DrawMax;
      XtCallCallbacks ((Widget)w, XtNdragYCallback, &cbk);
    }
  }

  w->plot.drag_state = NOT_DRAGGING;
  w->plot.drag_start.x = 0.0;
  w->plot.drag_start.y = 0.0;
  w->plot.reach_limit = 0;

}

static int get_plotlist_id (SciPlotWidget w, real x, real y)
{
  register int i, j;
  register SciPlotList *p;
  register real xval, yval;
  real     dmin = 0.0;
  real     d;
  int      id = -1;
  int      first = 1;

  for (i = 0; i < w->plot.num_plotlist; i++) {
    p = w->plot.plotlist + i;
    if (p->draw) {
      for (j = 0; j < p->number; j++) {

          /* Don't count the "break in line segment" flag for Min/Max */
        if (p->data[j].x > SCIPLOT_SKIP_VAL &&
            p->data[j].y > SCIPLOT_SKIP_VAL) {
          xval = p->data[j].x;
          yval = p->data[j].y;

	  if (first) {
	    dmin = (x - xval)*(x - xval) + (y - yval) * (y - yval);
	    first = 0;
	    id = i;
	  }
	  else {
	    d = (x - xval)*(x - xval) + (y - yval) * (y - yval);
	    if (d < dmin) {
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

static void sciPlotClick (SciPlotWidget w,
			  XEvent *event, char *args, int n_args)
{
  SciPlotBtn1ClickCbkStruct cbs;

  if (w->plot.btn1_callback != NULL) {
    cbs.event = event;
    cbs.action = 0;
    cbs.x = OutputX (w, event->xbutton.x);
    cbs.y = OutputY (w, event->xbutton.y);
    cbs.plotid = get_plotlist_id (w, cbs.x, cbs.y);
    XtCallCallbacks ((Widget)w, XtNbtn1ClickCallback, &cbs);
  }
}


static void sciPlotTrackPointer (SciPlotWidget w,
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
  cbs.x = OutputX (w, x);
  cbs.y = OutputY (w, y);
  XtCallCallbacks ((Widget)w, XtNpointerValCallback, &cbs);
}

int
SciPlotZoomIn (Widget wi)
{
  SciPlotWidget w = (SciPlotWidget)wi;
  real xoffset, yoffset;
  real xmin, xmax, ymin, ymax;

  xmin = w->plot.x.DrawOrigin;
  xmax = w->plot.x.DrawMax;
  ymin = w->plot.y.DrawOrigin;
  ymax = w->plot.y.DrawMax;

  /* calculate offsets for limits of displayed data */
  xoffset = (xmax - xmin) * SCIPLOT_ZOOM_FACTOR/2;
  yoffset = (ymax - ymin) * SCIPLOT_ZOOM_FACTOR/2;

  /* Narrow the display range by the above offsets */
  xmax -= xoffset;
  xmin += xoffset;
  ymin += yoffset;
  ymax -= yoffset;

  w->plot.UserMin.x = xmin;
  w->plot.UserMin.y = ymin;
  w->plot.UserMax.x = xmax;
  w->plot.UserMax.y = ymax;
  w->plot.XAutoScale = False;
  w->plot.YAutoScale = False;

  /* redraw everything */
  EraseAll(w);
#if DEBUG_SCIPLOT
  SciPlotPrintStatistics(wi);
#endif
  ComputeAll(w, NO_COMPUTE_MIN_MAX);
  /* ComputeAll(w, COMPUTE_MIN_MAX);*/
  DrawAll(w);

  return 0;
}

int
SciPlotZoomInX (Widget wi)
{
  SciPlotWidget w = (SciPlotWidget)wi;
  real xoffset;
  real xmin, xmax;

  xmin = w->plot.x.DrawOrigin;
  xmax = w->plot.x.DrawMax;


  /* calculate offsets for limits of displayed data */
  xoffset = (xmax - xmin) * SCIPLOT_ZOOM_FACTOR/2;

  /* Narrow the display range by the above offsets */
  xmax -= xoffset;
  xmin += xoffset;

  w->plot.UserMin.x = xmin;
  w->plot.UserMax.x = xmax;
  w->plot.XAutoScale = False;

  /* redraw everything */
  EraseAll(w);
#if DEBUG_SCIPLOT
  SciPlotPrintStatistics(wi);
#endif
  ComputeAll(w, NO_COMPUTE_MIN_MAX);
  /* ComputeAll(w, COMPUTE_MIN_MAX);*/
  DrawAll(w);
  return 0;
}

int
SciPlotZoomOut (Widget wi)
{
  SciPlotWidget w = (SciPlotWidget)wi;
  real xoffset, yoffset;
  real xmin, xmax, ymin, ymax;
  real newxmin, newxmax, newymin, newymax;

  xmin = w->plot.x.DrawOrigin;
  xmax = w->plot.x.DrawMax;
  ymin = w->plot.y.DrawOrigin;
  ymax = w->plot.y.DrawMax;

  /* calculate zoom factor to reverse a zoom by ZOOM_FACTOR */
  xoffset = (xmax - xmin) * (SCIPLOT_ZOOM_FACTOR/(1.0 -SCIPLOT_ZOOM_FACTOR))/2;
  yoffset = (ymax - ymin) * (SCIPLOT_ZOOM_FACTOR/(1.0 -SCIPLOT_ZOOM_FACTOR))/2;

  xmin -= xoffset;
  xmax += xoffset;
  ymin -= yoffset;
  ymax += yoffset;

  if (xmin <= w->plot.startx)
    xmin = w->plot.startx;

  if (xmax >= w->plot.endx)
    xmax = w->plot.endx;

  if (ymin <= w->plot.starty)
    ymin = w->plot.starty;

  if (ymax >= w->plot.endy)
    ymax = w->plot.endy;

  /* calculate real drawing range */
  ComputeXDrawingRange_i (w, xmin, xmax, &newxmin, &newxmax);
  ComputeYDrawingRange_i (w, ymin, ymax, &newymin, &newymax);

  if (newxmin != w->plot.x.DrawOrigin || newxmax != w->plot.x.DrawMax ||
      newymin != w->plot.y.DrawOrigin || newymax != w->plot.y.DrawMax) {
    w->plot.UserMin.x = xmin;
    w->plot.UserMin.y = ymin;
    w->plot.UserMax.x = xmax;
    w->plot.UserMax.y = ymax;
    w->plot.XAutoScale = False;
    w->plot.YAutoScale = False;

    /* redraw everything */
    EraseAll(w);
#if DEBUG_SCIPLOT
    SciPlotPrintStatistics(wi);
#endif
    /* ComputeAll(w, COMPUTE_MIN_MAX);*/
    ComputeAll(w, NO_COMPUTE_MIN_MAX);
    DrawAll(w);
    return 0;
  }
  else
    return 1;
}

int
SciPlotZoomOutX (Widget wi)
{
  SciPlotWidget w = (SciPlotWidget)wi;
  real xoffset;
  real xmin, xmax;
  real newxmin, newxmax;

  xmin = w->plot.x.DrawOrigin;
  xmax = w->plot.x.DrawMax;

  /* calculate zoom factor to reverse a zoom by ZOOM_FACTOR */
  xoffset = (xmax - xmin) * (SCIPLOT_ZOOM_FACTOR/(1.0 -SCIPLOT_ZOOM_FACTOR))/2;

  xmin -= xoffset;
  xmax += xoffset;

  if (xmin <= w->plot.startx)
    xmin = w->plot.startx;

  if (xmax >= w->plot.endx)
    xmax = w->plot.endx;

  /* calculate real drawing range */
  ComputeXDrawingRange_i (w, xmin, xmax, &newxmin, &newxmax);

  if (newxmin != w->plot.x.DrawOrigin || newxmax != w->plot.x.DrawMax) {
    w->plot.UserMin.x = xmin;
    w->plot.UserMax.x = xmax;
    w->plot.XAutoScale = False;

    /* redraw everything */
    EraseAll(w);
#if DEBUG_SCIPLOT
    SciPlotPrintStatistics(wi);
#endif
    /* ComputeAll(w, COMPUTE_MIN_MAX);*/
    ComputeAll(w, NO_COMPUTE_MIN_MAX);
    DrawAll(w);
    return 0;
  }
  return 1;
}


void
SciPlotXDrawingRange (Widget wi, float xmin, float xmax,
		      float* rxmin, float* rxmax)
{
  SciPlotWidget w = (SciPlotWidget)wi;

  /* calculate real drawing range */
  ComputeXDrawingRange_i (w, xmin, xmax, rxmin, rxmax);
}

/* KE: Added routines */

void
SciPlotGetXAxisInfo(Widget wi, float *min, float *max, Boolean *isLog,
  Boolean *isAuto)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  *isLog = w->plot.XLog;
  if (w->plot.XAutoScale) {
    *min = w->plot.Min.x;
    *max = w->plot.Max.x;
    *isAuto = True;
  } else {
    *min = w->plot.UserMin.x;
    *max = w->plot.UserMax.x;
    *isAuto = False;
  }
}

void
SciPlotGetYAxisInfo(Widget wi, float *min, float *max, Boolean *isLog,
  Boolean *isAuto)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi))
    return;

  w = (SciPlotWidget)wi;
  *isLog = w->plot.YLog;
  if (w->plot.YAutoScale) {
    *min = w->plot.Min.y;
    *max = w->plot.Max.y;
    *isAuto = True;
  } else {
    *min = w->plot.UserMin.y;
    *max = w->plot.UserMax.y;
    *isAuto = False;
  }
}

/* This function prints widget metrics */
void
SciPlotPrintMetrics(Widget wi)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi)) return;
  w = (SciPlotWidget)wi;

  SciPlotPrintf("\nPlot Metrics Information\n");
  SciPlotPrintf("  Width:           %d\n",w->core.width);
  SciPlotPrintf("  Height:          %d\n",w->core.height);
  SciPlotPrintf("  Margin:          %d\n",w->plot.Margin);
  SciPlotPrintf("  XFixedLR         %s\n",w->plot.XFixedLR?"True":"False");
  SciPlotPrintf("  LegendMargin     %d\n",w->plot.LegendMargin);
  SciPlotPrintf("  LegendLineSize   %d\n",w->plot.LegendLineSize);
  SciPlotPrintf("  TitleMargin      %d\n",w->plot.TitleMargin);
  SciPlotPrintf("  TitleHeight      %g\n",FontnumHeight(w,w->plot.titleFont));
  SciPlotPrintf("  TitleAscent      %g\n",FontnumAscent(w,w->plot.titleFont));
  SciPlotPrintf("  TitleDescent     %g\n",FontnumDescent(w,w->plot.titleFont));
  SciPlotPrintf("  AxisLabelHeight  %g\n",FontnumHeight(w,w->plot.labelFont));
  SciPlotPrintf("  AxisLabelAscent  %g\n",FontnumAscent(w,w->plot.labelFont));
  SciPlotPrintf("  AxisLabelDescent %g\n",FontnumDescent(w,w->plot.labelFont));
  SciPlotPrintf("  AxisNumHeight    %g\n",FontnumHeight(w,w->plot.axisFont));
  SciPlotPrintf("  AxisNumAscent    %g\n",FontnumAscent(w,w->plot.axisFont));
  SciPlotPrintf("  AxisNumDescent   %g\n",FontnumDescent(w,w->plot.axisFont));
  SciPlotPrintf("  x.Origin:        %g\n",w->plot.x.Origin);
  SciPlotPrintf("  x.Size:          %g\n",w->plot.x.Size);
  SciPlotPrintf("  x.Center:        %g\n",w->plot.x.Center);
  SciPlotPrintf("  x.TitlePos:      %g\n",w->plot.x.TitlePos);
  SciPlotPrintf("  x.AxisPos:       %g\n",w->plot.x.AxisPos);
  SciPlotPrintf("  x.LabelPos:      %g\n",w->plot.x.LabelPos);
  SciPlotPrintf("  x.LegendPos:     %g\n",w->plot.x.LegendPos);
  SciPlotPrintf("  x.LegendSize     %g\n",w->plot.x.LegendSize);
  SciPlotPrintf("  y.Origin:        %g\n",w->plot.y.Origin);
  SciPlotPrintf("  y.Size:          %g\n",w->plot.y.Size);
  SciPlotPrintf("  y.Center:        %g\n",w->plot.y.Center);
  SciPlotPrintf("  y.TitlePos:      %g\n",w->plot.y.TitlePos);
  SciPlotPrintf("  y.AxisPos:       %g\n",w->plot.y.AxisPos);
  SciPlotPrintf("  y.LabelPos:      %g\n",w->plot.y.LabelPos);
  SciPlotPrintf("  y.LegendPos:     %g\n",w->plot.y.LegendPos);
  SciPlotPrintf("  y.LegendSize     %g\n",w->plot.y.LegendSize);
}

/* This function prints axis info */
void
SciPlotPrintAxisInfo(Widget wi)
{
  SciPlotWidget w;

  if (!XtIsSciPlot(wi)) return;
  w = (SciPlotWidget)wi;

  SciPlotPrintf("\nPlot Axis Information\n");
  SciPlotPrintf("  XLog:            %s\n",w->plot.XLog?"True":"False");
  SciPlotPrintf("  XAutoScale:      %s\n",w->plot.XAutoScale?"True":"False");
  SciPlotPrintf("  XNoCompMinMax:   %s\n",w->plot.XNoCompMinMax?"True":"False");
  SciPlotPrintf("  XFixedLR:        %s\n",w->plot.XFixedLR?"True":"False");
  SciPlotPrintf("  XLeftSpace:      %d\n",w->plot.XLeftSpace);
  SciPlotPrintf("  XRightSpace:     %d\n",w->plot.XRightSpace);
  SciPlotPrintf("  Min.x:           %g\n",w->plot.Min.x);
  SciPlotPrintf("  Max.x:           %g\n",w->plot.Max.x);
  SciPlotPrintf("  UserMin.x:       %g\n",w->plot.UserMin.x);
  SciPlotPrintf("  UserMax.x:       %g\n",w->plot.UserMax.x);
  SciPlotPrintf("  x.Origin:        %g\n",w->plot.x.Origin);
  SciPlotPrintf("  x.Size:          %g\n",w->plot.x.Size);
  SciPlotPrintf("  x.Center:        %g\n",w->plot.x.Center);
  SciPlotPrintf("  x.TitlePos:      %g\n",w->plot.x.TitlePos);
  SciPlotPrintf("  x.AxisPos:       %g\n",w->plot.x.AxisPos);
  SciPlotPrintf("  x.LabelPos:      %g\n",w->plot.x.LabelPos);
  SciPlotPrintf("  x.LegendPos:     %g\n",w->plot.x.LegendPos);
  SciPlotPrintf("  x.LegendSize     %g\n",w->plot.x.LegendSize);
  SciPlotPrintf("  x.DrawOrigin:    %g\n",w->plot.x.DrawOrigin);
  SciPlotPrintf("  x.DrawSize:      %g\n",w->plot.x.DrawSize);
  SciPlotPrintf("  x.DrawMax:       %g\n",w->plot.x.DrawMax);
  SciPlotPrintf("  x.MajorInc:      %g\n",w->plot.x.MajorInc);
  SciPlotPrintf("  x.MajorNum:      %d\n",w->plot.x.MajorNum);
  SciPlotPrintf("  x.MinorNum:      %d\n",w->plot.x.MinorNum);
  SciPlotPrintf("  x.Precision:     %d\n",w->plot.x.Precision);
  SciPlotPrintf("  x.FixedMargin:   %d\n",w->plot.x.FixedMargin);
  SciPlotPrintf("  x.Left:          %g\n",w->plot.x.Left);
  SciPlotPrintf("  x.Right:         %g\n",w->plot.x.Right);

  SciPlotPrintf("  YLog:            %s\n",w->plot.YLog?"True":"False");
  SciPlotPrintf("  YAutoScale:      %s\n",w->plot.YAutoScale?"True":"False");
  SciPlotPrintf("  Min.y:           %g\n",w->plot.Min.y);
  SciPlotPrintf("  Max.y:           %g\n",w->plot.Max.y);
  SciPlotPrintf("  UserMin.y:       %g\n",w->plot.UserMin.y);
  SciPlotPrintf("  UserMax.y:       %g\n",w->plot.UserMax.y);
  SciPlotPrintf("  y.Origin:        %g\n",w->plot.y.Origin);
  SciPlotPrintf("  y.Size:          %g\n",w->plot.y.Size);
  SciPlotPrintf("  y.Center:        %g\n",w->plot.y.Center);
  SciPlotPrintf("  y.TitlePos:      %g\n",w->plot.y.TitlePos);
  SciPlotPrintf("  y.AxisPos:       %g\n",w->plot.y.AxisPos);
  SciPlotPrintf("  y.LabelPos:      %g\n",w->plot.y.LabelPos);
  SciPlotPrintf("  y.LegendPos:     %g\n",w->plot.y.LegendPos);
  SciPlotPrintf("  y.LegendSize     %g\n",w->plot.y.LegendSize);
  SciPlotPrintf("  y.DrawOrigin:    %g\n",w->plot.y.DrawOrigin);
  SciPlotPrintf("  y.DrawSize:      %g\n",w->plot.y.DrawSize);
  SciPlotPrintf("  y.DrawMax:       %g\n",w->plot.y.DrawMax);
  SciPlotPrintf("  y.MajorInc:      %g\n",w->plot.y.MajorInc);
  SciPlotPrintf("  y.MajorNum:      %d\n",w->plot.y.MajorNum);
  SciPlotPrintf("  y.MinorNum:      %d\n",w->plot.y.MinorNum);
  SciPlotPrintf("  y.Precision:     %d\n",w->plot.y.Precision);
  SciPlotPrintf("  y.FixedMargin:   %d\n",w->plot.y.FixedMargin);
  SciPlotPrintf("  y.Left:          %g\n",w->plot.y.Left);
  SciPlotPrintf("  y.Right:         %g\n",w->plot.y.Right);
}

#ifdef MOTIF
/* KE: Could use XmeDrawShadows here.  Primitive should keep track of
 * colors and GCs */
static void
DrawShadow (SciPlotWidget w, Boolean raised, real x, real y,
  real width, real height)
{
  int i, color1, color2, color;

  if(w->primitive.shadow_thickness <= 0) return;

  w->plot.current_id = SciPlotDrawingShadows;

/* Reduce the height and width by 1 for the algorithm */
  if(height > 0) height--;
  if(width > 0) width--;

  color1 = w->plot.ShadowColor1;
  color2 = w->plot.ShadowColor2;

  for (i = 0; i < w->primitive.shadow_thickness; i++) {
    color = raised ? color1 : color2;
    LineSet(w, x+i, y+i, x+i, y+height-i, color, XtLINE_SOLID);
    LineSet(w, x+i, y+i, x+width-i, y+i, color, XtLINE_SOLID);

    color = raised ? color2 : color1;
    LineSet(w, x+i, y+height-i, x+width-i, y+height-i, color, XtLINE_SOLID);
    LineSet(w, x+width-i, y+i, x+width-i, y+height-i,color, XtLINE_SOLID);
  }
}
#endif

/* This function handles various problems with printing under Windows
 *   using Exceed */
static void
SciPlotPrintf(const char *fmt, ...)
{
    va_list vargs;
    static char lstring[1024];  /* DANGER: Fixed buffer size */

    va_start(vargs,fmt);
    vsprintf(lstring,fmt,vargs);
    va_end(vargs);

    if(lstring[0] != '\0') {
#ifdef WIN32
	lprintf("%s",lstring);
#else
	printf("%s",lstring);
#endif
    }
}

/* **************************** Emacs Editing Sequences ***************** */
/* Local Variables: */
/* c-basic-offset: 2 */
/* End: */
