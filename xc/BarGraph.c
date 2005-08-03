/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*************************************************************************
 * Program: BarGraph.c                                                   *
 * Author : Paul D. Johnston, Mark Anderson, DMW Software.               *
 * Mods   : 1.0 Original                                                 *
 *          ?       - Integration with MEDM.                             *
 *          June 94 - Added bar graph fill from center.                  *
 *************************************************************************/

#define DEBUG_BAR 0
#define DEBUG_DRAW 0

#include <stdio.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/IntrinsicP.h>

#include "Xc.h"
#include "Control.h"
#include "Value.h"
#include "BarGraphP.h"
#include "cvtFast.h"

#ifndef MIN
# define  MIN(a,b)    (((a) < (b)) ? (a) :  (b))
#endif
#ifndef MAX
#  define  MAX(a,b)    (((a) > (b)) ? (a) :  (b))
#endif

/* Macro redefinition for offset. */
#define offset(field) XtOffset(BarGraphWidget, field)

/* Declare widget methods */
static void ClassInitialize(void);
static void Initialize(Widget request, Widget new,
  ArgList args, Cardinal *nargs);
static void Redisplay(Widget w, XEvent *event, Region region);
static Boolean SetValues(Widget cur, Widget req,
  Widget new, ArgList args, Cardinal *nargs);
static void Destroy(Widget w);
static void Resize(Widget w);
static XtGeometryResult QueryGeometry(Widget w, XtWidgetGeometry *proposed,
  XtWidgetGeometry *answer);

/* Declare functions and variables private to this widget */
static void Get_value(XtPointer client_data, XtIntervalId *id);
static void Draw_display(Widget w, Display *display,
  Drawable drawable, GC gc);
static void Print_bounds(Widget w, char *upper, char *lower);

/* Define the widget's resource list */
static XtResource resources[] = {
    {
	XcNorient,
	XcCOrient,
	XcROrient,
	sizeof(XcOrient),
	offset(barGraph.orient),
	XtRString,
	"vertical"
    },
    {
	XcNfillmod,
	XcCFillmod,
	XcRFillmod,
	sizeof(XcFillmod),
	offset(barGraph.fillmod),
	XtRString,
	"from edge"
    },
    {
	XcNbarForeground,
	XtCColor,
	XtRPixel,
	sizeof(Pixel),
	offset(barGraph.bar_foreground),
	XtRString,
	XtDefaultForeground
    },
    {
	XcNbarBackground,
	XtCColor,
	XtRPixel,
	sizeof(Pixel),
	offset(barGraph.bar_background),
	XtRString,
	XtDefaultBackground
    },
    {
	XcNscaleColor,
	XtCColor,
	XtRPixel,
	sizeof(Pixel),
	offset(barGraph.scale_pixel),
	XtRString,
	XtDefaultForeground
    },
    {
	XcNscaleSegments,
	XcCScaleSegments,
	XtRInt,
	sizeof(int),
	offset(barGraph.num_segments),
	XtRImmediate,
	(XtPointer)7
    },
    {
	XcNvalueVisible,
	XtCBoolean,
	XtRBoolean,
	sizeof(Boolean),
	offset(barGraph.value_visible),
	XtRString,
	"True"
    },
    {
	XcNdecorations,
	XtCBoolean,
	XtRBoolean,
	sizeof(Boolean),
	offset(barGraph.decorations),
	XtRString,
	"True"
    },
    {
	XcNinterval,
	XcCInterval,
	XtRInt,
	sizeof(int),
	offset(barGraph.interval),
	XtRImmediate,
	(XtPointer)0
    },
    {
	XcNupdateCallback,
	XtCCallback,
	XtRCallback,
	sizeof(XtPointer),
	offset(barGraph.update_callback),
	XtRCallback,
	NULL
    },
    {
	XcNdoubleBuffer,
	XtCBoolean,
	XtRBoolean,
	sizeof(Boolean),
	offset(barGraph.double_buffer),
	XtRString,
	"False"
    },
};

/* Widget Class Record initialization */
BarGraphClassRec barGraphClassRec = {
    {
      /* core_class part */
        (WidgetClass) &valueClassRec,  /* superclass */
        "BarGraph",                    /* class_name */
        sizeof(BarGraphRec),           /* widget_size */
        ClassInitialize,               /* class_initialize */
        NULL,                          /* class_part_initialize */
        FALSE,                         /* class_inited */
        Initialize,                    /* initialize */
        NULL,                          /* initialize_hook */
        XtInheritRealize,              /* realize */
        NULL,                          /* actions */
        0,                             /* num_actions */
        resources,                     /* resources */
        XtNumber(resources),           /* num_resources */
        NULLQUARK,                     /* xrm_class */
        TRUE,                          /* compress_motion */
        TRUE,                          /* compress_exposure */
        TRUE,                          /* compress_enterleave */
        TRUE,                          /* visible_interest */
        Destroy,                       /* destroy */
        Resize,                        /* resize */
        Redisplay,                     /* expose */
        SetValues,                     /* set_values */
        NULL,                          /* set_values_hook */
        XtInheritSetValuesAlmost,      /* set_values_almost */
        NULL,                          /* get_values_hook */
        NULL,                          /* accept_focus */
        XtVersion,                     /* version */
        NULL,                          /* callback_private */
        NULL,                          /* tm_table */
        QueryGeometry,                 /* query_geometry */
        NULL,                          /* display_accelerator */
        NULL,                          /* extension */
    },
  /* Control class part, value class part, barGraph class part */
    { 0, }, { 0, }, { 0, }
};

WidgetClass xcBarGraphWidgetClass = (WidgetClass)&barGraphClassRec;

static void ClassInitialize(void) {
  /*************************************************************************
   * ClassInitialize: This method initializes the BarGraph widget class.   *
   *   It registers resource value converter functions with Xt.            *
   *************************************************************************/
    XtAddConverter(XtRString, XcROrient, CvtStringToOrient, NULL, 0);
}

static void Initialize(Widget request, Widget new,
  ArgList args, Cardinal *nargs)
  /*************************************************************************
   * Initialize: This is the initialize method for the BarGraph widget.    *
   *   It validates user-modifiable instance resources and initializes     *
   *   private widget variables and structures.  This function also        *
   *   creates any server resources (i.e., GCs, fonts, Pixmaps, etc.) used *
   *   by this widget.  This method is called by Xt when the application   *
   *   calls XtCreateWidget().                                             *
   *************************************************************************/
{
    BarGraphWidget wnew = (BarGraphWidget)new;
    DPRINTF(("BarGraph: executing Initialize...\n"));


  /* Validate public instance variable settings.
	  Check orientation resource setting. */
    if((wnew->barGraph.orient != XcVert)
      && (wnew->barGraph.orient != XcHoriz)
      && (wnew->barGraph.orient != XcVertDown)
      && (wnew->barGraph.orient != XcHorizLeft)){
	XtWarning("BarGraph: invalid orientation setting");
	wnew->barGraph.orient = XcVert;
    }

  /* Check the interval resource setting. */
    if(wnew->barGraph.interval > 0) {
	wnew->barGraph.interval_id =
	  XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)new),
	    wnew->barGraph.interval, Get_value, new);
    }

  /* Check the scaleSegments resource setting. */
    if(wnew->barGraph.num_segments < MIN_SCALE_SEGS) {
	XtWarning("BarGraph: invalid number of scale segments");
	wnew->barGraph.num_segments = MIN_SCALE_SEGS;
    }
    else if(wnew->barGraph.num_segments > MAX_SCALE_SEGS) {
	XtWarning("BarGraph: invalid number of scale segments");
	wnew->barGraph.num_segments = MAX_SCALE_SEGS;
    }

  /* Check the valueVisible resource setting. */
    if((wnew->barGraph.value_visible != True) &&
      (wnew->barGraph.value_visible != False)) {
	XtWarning("BarGraph: invalid valueVisible setting");
	wnew->barGraph.value_visible = True;
    }

  /* Check the decorations resource setting. */
    if((wnew->barGraph.decorations != True) &&
      (wnew->barGraph.decorations != False)) {
	XtWarning("BarGraph: invalid decorations setting");
	wnew->barGraph.decorations = True;
    }

  /* Initialize the BarGraph width and height. */
    if(wnew->barGraph.decorations == True) {
	if(wnew->core.width < MIN_BG_WIDTH) wnew->core.width = MIN_BG_WIDTH;
	if(wnew->core.height < MIN_BG_HEIGHT) wnew->core.height = MIN_BG_HEIGHT;
    }

  /* Set the initial geometry of the BarGraph elements. */
    Resize(new);

    DPRINTF(("BarGraph: done Initialize\n"));
}

static void Redisplay(Widget w, XEvent *event, Region region)
  /*************************************************************************
   * Redisplay : This function is the BarGraph's Expose method.  It redraws*
   *   the BarGraph's 3D rectangle background, Value Box, label, Bar       *
   *   indicator, and the Scale.  All drawing takes place within the       *
   *   widget's window (no need for an off-screen pixmap).                 *
   *************************************************************************/
{
    BarGraphWidget wb = (BarGraphWidget)w;
    int j;
    char upper[30], lower[30];

#if DEBUG_BAR
    printf("BarGraph Redisplay:\n");
    printf("  core: x=%d y=%d width=%u height=%u border_width=%u\n",
      wb->core.x,wb->core.y,
      wb->core.width,wb->core.height,
      wb->core.border_width);
    printf("  barGraph.bar: x=%d y=%d width=%u height=%u\n",
      wb->barGraph.bar.x,wb->barGraph.bar.y,
      wb->barGraph.bar.width,wb->barGraph.bar.height);
#endif

  /* Check to see whether or not the widget's window is mapped */
#ifdef USE_CORE_VISIBLE
    if(!XtIsRealized(w) || !wb->core.visible) return;
#else
    if(!XtIsRealized(w)) return;
#endif
    DPRINTF(("BarGraph: executing Redisplay\n"));

#if 1
  /* KE: Why is this commented out? */
  /* Draw the 3D rectangle background for the BarGraph */
    if(wb->barGraph.decorations == True) {
	XSetClipMask(XtDisplay(w), wb->control.gc, None);
	Rect3d(w, XtDisplay(w), XtWindow(w), wb->control.gc,
	  0, 0, wb->core.width, wb->core.height, RAISED);
    }
#endif

  /* Draw the Label string */
    if(wb->barGraph.decorations == True) {
	XSetClipRectangles(XtDisplay(w), wb->control.gc, 0, 0,
	  &(wb->barGraph.face), 1, Unsorted);
	XSetForeground(XtDisplay(w), wb->control.gc, wb->control.label_pixel);
	XDrawString(XtDisplay(w), XtWindow(w), wb->control.gc,
	  wb->barGraph.lbl.x, wb->barGraph.lbl.y,
	  wb->control.label, strlen(wb->control.label));
    }

  /* Draw the Scale */
    if(wb->barGraph.decorations  == True && wb->barGraph.num_segments > 0) {
	XSetForeground(XtDisplay(w), wb->control.gc, wb->barGraph.scale_pixel);
	XDrawLine(XtDisplay(w), XtWindow(w), wb->control.gc,
	  wb->barGraph.scale_line.x1, wb->barGraph.scale_line.y1,
	  wb->barGraph.scale_line.x2, wb->barGraph.scale_line.y2);

      /* Draw the max and min value segments */
	if(wb->barGraph.orient == XcVert || wb->barGraph.orient == XcVertDown) {
	} else {
	    XDrawLine(XtDisplay(w), XtWindow(w), wb->control.gc,
	      wb->barGraph.scale_line.x1,
	      wb->barGraph.scale_line.y1 - wb->barGraph.seg_length,
	      wb->barGraph.scale_line.x1, wb->barGraph.scale_line.y1);
	    XDrawLine(XtDisplay(w), XtWindow(w), wb->control.gc,
	      wb->barGraph.scale_line.x2,
	      wb->barGraph.scale_line.y2 - wb->barGraph.seg_length,
	      wb->barGraph.scale_line.x2, wb->barGraph.scale_line.y2);
	}

      /* Now draw the rest of the Scale segments */
	for(j = 0; j < wb->barGraph.num_segments; j++) {
	    if(wb->barGraph.orient == XcVert ||
	      wb->barGraph.orient == XcVertDown)
	      XDrawLine(XtDisplay(w), XtWindow(w), wb->control.gc,
		wb->barGraph.segs[j].x, wb->barGraph.segs[j].y,
		wb->barGraph.scale_line.x1, wb->barGraph.segs[j].y);
	    else
	      XDrawLine(XtDisplay(w), XtWindow(w), wb->control.gc,
		wb->barGraph.segs[j].x, wb->barGraph.segs[j].y,
		wb->barGraph.segs[j].x, wb->barGraph.scale_line.y1);
	}

      /* Draw the max and min value string indicators */
	Print_bounds(w, upper, lower);
	if(wb->barGraph.orient == XcVert ||  wb->barGraph.orient == XcHoriz) {
	    XDrawString(XtDisplay(w), XtWindow(w), wb->control.gc,
	      wb->barGraph.max_val.x, wb->barGraph.max_val.y, upper,
	      strlen(upper));
	    XDrawString(XtDisplay(w), XtWindow(w), wb->control.gc,
	      wb->barGraph.min_val.x, wb->barGraph.min_val.y, lower,
	      strlen(lower));
        } else {
	    XDrawString(XtDisplay(w), XtWindow(w), wb->control.gc,
	      wb->barGraph.max_val.x, wb->barGraph.max_val.y, lower,
	      strlen(lower));
	    XDrawString(XtDisplay(w), XtWindow(w), wb->control.gc,
	      wb->barGraph.min_val.x, wb->barGraph.min_val.y, upper,
	      strlen(upper));
        }
    }


  /* Draw the Bar indicator border */
    if(wb->barGraph.decorations == True) {
	Rect3d(w, XtDisplay(w), XtWindow(w), wb->control.gc,
	  wb->barGraph.bar.x - wb->control.shade_depth,
	  wb->barGraph.bar.y - wb->control.shade_depth,
	  wb->barGraph.bar.width + (2 * wb->control.shade_depth),
	  wb->barGraph.bar.height + (2 * wb->control.shade_depth), DEPRESSED);
    }

  /* Draw the Value Box */
    if(wb->barGraph.decorations == True && wb->barGraph.value_visible == True)
      Rect3d(w, XtDisplay(w), XtWindow(w), wb->control.gc,
	wb->value.value_box.x - wb->control.shade_depth,
	wb->value.value_box.y - wb->control.shade_depth,
	wb->value.value_box.width + (2 * wb->control.shade_depth),
	wb->value.value_box.height + (2 * wb->control.shade_depth), DEPRESSED);

  /* Draw the new values of Bar indicator and the value string */
    Draw_display(w, XtDisplay(w), XtWindow(w), wb->control.gc);
    DPRINTF(("BarGraph: done Redisplay\n"));
}

/*******************************************************************
 NAME:		SetValues.
 DESCRIPTION:
   This is the set_values method for this widget. It validates resource
 settings set with XtSetValues. If a resource is changed that would
 require re-drawing the widget, return True.
*******************************************************************/
static Boolean SetValues(Widget cur, Widget req,
  Widget new, ArgList args, Cardinal *nargs)
  /* KE: req, args, nargs is not used */
{
    BarGraphWidget wnew = (BarGraphWidget)new;
    BarGraphWidget wcur = (BarGraphWidget)cur;
    Boolean do_redisplay = False, do_resize = False;

    DPRINTF(("BarGraph: executing SetValues\n"));

  /* Check widget color resource settings. */
    if((wnew->barGraph.bar_foreground != wcur->barGraph.bar_foreground) ||
      (wnew->barGraph.bar_background != wcur->barGraph.bar_background) ||
      (wnew->barGraph.scale_pixel != wcur->barGraph.scale_pixel))
      do_redisplay = True;

  /* Check orientation resource setting. */
    if(wnew->barGraph.orient != wcur->barGraph.orient) {
	do_redisplay = True;
	if((wnew->barGraph.orient != XcVert) &&
	  (wnew->barGraph.orient != XcHoriz)) {
	    XtWarning("BarGraph: invalid orientation setting");
	    wnew->barGraph.orient = XcVert;
	}
    }

  /* Check the interval resource setting. */
    if(wnew->barGraph.interval != wcur->barGraph.interval) {
	if(wcur->barGraph.interval > 0)
	  XtRemoveTimeOut (wcur->barGraph.interval_id);
	if(wnew->barGraph.interval > 0)
	  wnew->barGraph.interval_id =
	    XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)new),
	      wnew->barGraph.interval, Get_value, new);
    }

  /* Check the scaleSegments resource setting. */
    if(wnew->barGraph.num_segments != wcur->barGraph.num_segments) {
	if(wnew->barGraph.num_segments < MIN_SCALE_SEGS) {
	    XtWarning("BarGraph: invalid number of scale segments");
	    wnew->barGraph.num_segments = MIN_SCALE_SEGS;
	} else if(wnew->barGraph.num_segments > MAX_SCALE_SEGS) {
	    XtWarning("BarGraph: invalid number of scale segments");
	    wnew->barGraph.num_segments = MAX_SCALE_SEGS;
	}
    }

  /* Check the valueVisible resource setting. */
    if(wnew->barGraph.value_visible != wcur->barGraph.value_visible) {
	do_redisplay = True;
	if((wnew->barGraph.value_visible != True) &&
	  (wnew->barGraph.value_visible != False)) {
	    XtWarning("BarGraph: invalid valueVisible setting");
	    wnew->barGraph.value_visible = True;
	}
    }

  /* Check the decorations resource setting. */
    if(wnew->barGraph.decorations != wcur->barGraph.decorations) {
	do_redisplay = True;
	if((wnew->barGraph.decorations != True) &&
	  (wnew->barGraph.decorations != False)) {
	    XtWarning("BarGraph: invalid decorations setting");
	    wnew->barGraph.decorations = True;
	}
	do_resize = True;
    }

  /* Check to see if the value has changed. */
    if((((wnew->value.datatype == XcLval) || (wnew->value.datatype == XcHval)) &&
      (wnew->value.val.lval != wcur->value.val.lval)) ||
      ((wnew->value.datatype == XcFval) &&
	(wnew->value.val.fval != wcur->value.val.fval))) {
	do_redisplay = True;
    }

  /* (MDA) want to force resizing if min/max changed  or decimals setting */
    if(wnew->value.decimals != wcur->value.decimals) do_resize = True;

    if(((wnew->value.datatype == XcLval) || (wnew->value.datatype == XcHval)) &&
      ((wnew->value.lower_bound.lval != wcur->value.lower_bound.lval) ||
	(wnew->value.upper_bound.lval != wcur->value.upper_bound.lval) )) {
	do_resize = True;
    } else if((wnew->value.datatype == XcFval) &&
      ( (wnew->value.lower_bound.fval != wcur->value.lower_bound.fval) ||
	(wnew->value.upper_bound.fval != wcur->value.upper_bound.fval) )) {
	do_resize = True;
    }

  /* Resize and set do_display if do_resize is true */
    if(do_resize) {
	Resize(new);
	do_redisplay = True;
    }

    DPRINTF(("BarGraph: done SetValues\n"));
    return do_redisplay;
}

static void Resize(Widget w)
  /*************************************************************************
   * Resize: This is the resize method of the BarGraph widget. It re-sizes *
   *   the BarGraph's graphics based on the new width and height of the    *
   *   widget's window.                                                    *
   *************************************************************************/
{
    BarGraphWidget wb = (BarGraphWidget)w;
    int j, max_val_width, min_val_width, max_width;
    int seg_spacing, font_center, font_height;
    char upper[30], lower[30];
    Boolean displayValue, displayLabel;

    DPRINTF(("BarGraph: executing Resize\n"));

  /* For numbers, usually safe to ignore descent to save space */
    font_height = (wb->control.font)->ascent;

  /* Set the widgets new width and height. */
    if(wb->barGraph.decorations == True) {
	wb->barGraph.face.x = wb->barGraph.face.y = wb->control.shade_depth;
	wb->barGraph.face.width = wb->core.width - (2*wb->control.shade_depth);
	wb->barGraph.face.height = wb->core.height - (2*wb->control.shade_depth);
    } else {
	wb->barGraph.face.x = wb->barGraph.face.y = 0;
	wb->barGraph.face.width = wb->core.width;
	wb->barGraph.face.height = wb->core.height;
    }

  /* Calculate min/max string attributes */
    Print_bounds(w, upper, lower);
    if(wb->barGraph.orient == XcVert || wb->barGraph.orient == XcHoriz)	{
	max_val_width = XTextWidth(wb->control.font, upper,
	  strlen(upper));
	min_val_width = XTextWidth(wb->control.font, lower,
	  strlen(lower));
    } else {
	max_val_width = XTextWidth(wb->control.font, lower,
	  strlen(lower));
	min_val_width = XTextWidth(wb->control.font, upper,
	  strlen(upper));
    }
    max_width = MAX(min_val_width,max_val_width) +
      2*wb->control.shade_depth;
    if(wb->barGraph.num_segments == 0) {
	max_width = wb->barGraph.face.width - 2*wb->control.shade_depth;
    }

  /* Establish the new Value Box geometry. */
    if(wb->barGraph.value_visible == True) {
	displayValue = True;
	if(wb->barGraph.orient == XcVert || wb->barGraph.orient == XcVertDown) {
	    wb->value.value_box.x = wb->core.width/2 - max_width/2;
	    wb->value.value_box.y = wb->core.height - font_height -
	      3*wb->control.shade_depth;
	    wb->value.value_box.width = max_width;
	    wb->value.value_box.height = font_height;
	} else {
	    wb->value.value_box.x = wb->core.width/2 - max_width/2;
	    wb->value.value_box.y = wb->core.height - font_height -
	      2*wb->control.shade_depth - 2;
	    wb->value.value_box.width = max_width;
	    wb->value.value_box.height = font_height;
	}
      /* Set the position of the displayed value within the Value Box. */
	Position_val(w);
    } else {
	displayValue = False;
	wb->value.value_box.x = 0;
	wb->value.value_box.y = wb->core.height - 2*wb->control.shade_depth;
	wb->value.value_box.width = 0;
	wb->value.value_box.height = 0;
    }

  /* Set the new label location. */

    if(strlen(wb->control.label) > 1 ||
      (strlen(wb->control.label) == 1 && wb->control.label[0] != ' ')) {
	displayLabel = True;
	wb->barGraph.lbl.x = (short)((wb->core.width/2) -
	  (XTextWidth(wb->control.font, wb->control.label,
	    strlen(wb->control.label))/2));
	wb->barGraph.lbl.y = (short)(wb->barGraph.face.y +
	  wb->control.font->ascent + 1);
    } else {
	displayLabel = False;
	wb->barGraph.lbl.x = wb->barGraph.face.x;
	wb->barGraph.lbl.y = wb->barGraph.face.y;
    }

  /* Resize the Bar indicator */
    if(wb->barGraph.decorations == True) {
	if(wb->barGraph.orient == XcVert || wb->barGraph.orient == XcVertDown) {
	    wb->barGraph.bar.x = (short)(wb->barGraph.face.x +
	      + (wb->barGraph.num_segments > 0 ? max_width +
		(wb->barGraph.face.width/32) : 0) + 2*wb->control.shade_depth);
	    wb->barGraph.bar.y = (short)(wb->barGraph.lbl.y + wb->control.font->descent
	      + 2*wb->control.shade_depth);
	    wb->barGraph.bar.width = (unsigned short) MAX(0,
	      (int)(wb->barGraph.face.width - wb->barGraph.bar.x -
		wb->control.shade_depth));
	    wb->barGraph.bar.height = (unsigned short)
	      (wb->value.value_box.y - wb->barGraph.bar.y - (displayValue == True
		? font_height - wb->control.font->descent : 2*wb->control.shade_depth));
	} else {
	    wb->barGraph.bar.x = wb->barGraph.face.x +
	      (short)(wb->barGraph.face.width/16) +
	      wb->control.shade_depth;
	    wb->barGraph.bar.y = wb->barGraph.lbl.y
	      + (wb->barGraph.num_segments > 0 ?
		font_height/2 +
		wb->barGraph.face.height/(unsigned short)8 + 1 :
		0)
	      + 2*wb->control.shade_depth;
	    wb->barGraph.bar.width = (short)((9*(int)wb->barGraph.face.width)/10) -
	      2*wb->control.shade_depth;
	    wb->barGraph.bar.height = MAX(1,wb->value.value_box.y - wb->barGraph.bar.y
	      - (displayValue == True ? font_height : 0)
	      - wb->control.shade_depth);
	}
    } else {
	wb->barGraph.bar.x = wb->barGraph.bar.y = 0;
	wb->barGraph.bar.width = wb->barGraph.face.width;
	wb->barGraph.bar.height = wb->barGraph.face.height;
    }

  /* Resize the Scale line. */
    if(wb->barGraph.orient == XcVert ||
      wb->barGraph.orient == XcVertDown) {
	wb->barGraph.scale_line.x1 = wb->barGraph.bar.x -
	  wb->control.shade_depth -
	  (wb->barGraph.face.width/32);
	wb->barGraph.scale_line.y1 = wb->barGraph.bar.y;
	wb->barGraph.scale_line.x2 = wb->barGraph.scale_line.x1;
	wb->barGraph.scale_line.y2 = wb->barGraph.bar.y + wb->barGraph.bar.height;
    } else {
	wb->barGraph.scale_line.x1 = wb->barGraph.bar.x;
	wb->barGraph.scale_line.y1 = wb->barGraph.bar.y - wb->control.shade_depth -
	  (wb->barGraph.face.height/32);
	wb->barGraph.scale_line.x2 = wb->barGraph.bar.x + wb->barGraph.bar.width;
	wb->barGraph.scale_line.y2 = wb->barGraph.scale_line.y1;
    }

  /* Now, resize Scale line segments */
    if(wb->barGraph.num_segments > 0) {
	if(wb->barGraph.orient == XcVert || wb->barGraph.orient == XcVertDown) {
	    wb->barGraph.seg_length = (wb->barGraph.face.width/16);
	    seg_spacing = (wb->barGraph.bar.height /
	      (unsigned short)(wb->barGraph.num_segments + 1));
	    for(j = 0; j < wb->barGraph.num_segments; j++) {
		wb->barGraph.segs[j].x = wb->barGraph.scale_line.x1 -
		  wb->barGraph.seg_length;
		wb->barGraph.segs[j].y = wb->barGraph.scale_line.y1 +
		  ((j+1) * seg_spacing);
	    }
	} else {
	    wb->barGraph.seg_length = (wb->barGraph.face.height/16);
	    seg_spacing = (wb->barGraph.bar.width /
	      (unsigned short)(wb->barGraph.num_segments + 1));
	    for(j = 0; j < wb->barGraph.num_segments; j++) {
		wb->barGraph.segs[j].x = wb->barGraph.scale_line.x1 +
		  ((j+1) * seg_spacing);
		wb->barGraph.segs[j].y = wb->barGraph.scale_line.y1 -
		  wb->barGraph.seg_length;
	    }
	}
    }

  /* Set the position of the max and min value strings */
    if(wb->barGraph.orient == XcVert ||
      wb->barGraph.orient == XcVertDown) {
	font_center = ((wb->control.font->ascent +
	  wb->control.font->descent)/2)
	  - wb->control.font->descent;
	wb->barGraph.max_val.x = MAX(wb->control.shade_depth,
	  wb->barGraph.scale_line.x2 - (max_val_width));

	wb->barGraph.max_val.y = wb->barGraph.scale_line.y1 + font_center;

	wb->barGraph.min_val.x = MAX(wb->control.shade_depth,
	  wb->barGraph.scale_line.x2 - (min_val_width));
	wb->barGraph.min_val.y = wb->barGraph.scale_line.y2 + font_center;
    } else {
	wb->barGraph.max_val.x = MIN(
	  (int)(wb->barGraph.face.width + wb->control.shade_depth
	    - max_val_width),
	  (wb->barGraph.scale_line.x2 - (max_val_width/2)));
	wb->barGraph.max_val.y = wb->barGraph.min_val.y =
	  wb->barGraph.scale_line.y1 - wb->barGraph.seg_length - 1;
	wb->barGraph.min_val.x = MAX(wb->control.shade_depth,
	  wb->barGraph.scale_line.x1 - (min_val_width/2));
    }

    DPRINTF(("BarGraph: done Resize\n"));
}

/*******************************************************************
 NAME:		QueryGeometry.
 DESCRIPTION:
   This function is the widget's query_geometry method.  It simply
 checks the proposed size and returns the appropriate value based on
 the proposed size.  If the proposed size is greater than the maximum
 appropriate size for this widget, QueryGeometry returns the recommended
 size.
*******************************************************************/

static XtGeometryResult QueryGeometry(Widget w, XtWidgetGeometry *proposed,
  XtWidgetGeometry *answer)
{
    BarGraphWidget wb = (BarGraphWidget)w;

  /* Set the request mode mask for the returned answer. */
    answer->request_mode = CWWidth | CWHeight;

  /* Set the recommended size. */
    answer->width = (wb->core.width > MAX_BG_WIDTH)
      ? MAX_BG_WIDTH : wb->core.width;
    answer->height = (wb->core.height > MAX_BG_HEIGHT)
      ? MAX_BG_HEIGHT : wb->core.height;

  /*
   * Check the proposed dimensions. If the proposed size is larger than
   * appropriate, return the recommended size.
   */
    if(((proposed->request_mode & (CWWidth | CWHeight)) == (CWWidth | CWHeight))
      && proposed->width == answer->width
      && proposed->height == answer->height)
      return XtGeometryYes;
    else if(answer->width == wb->core.width && answer->height == wb->core.height)
      return XtGeometryNo;
    else
      return XtGeometryAlmost;

}

/*******************************************************************
 NAME:		Destroy.
 DESCRIPTION:
   This function is the widget's destroy method.  It simply releases
 any server resources acquired during the life of the widget.

*******************************************************************/
static void Destroy(Widget w)
{
    BarGraphWidget wb = (BarGraphWidget)w;

    if(wb->barGraph.interval > 0)
      XtRemoveTimeOut(wb->barGraph.interval_id);
}

/*******************************************************************
 NAME:		Get_value.
 DESCRIPTION:
   This function is the time out procedure called at XcNinterval
 intervals.  It calls the application registered callback function to
 get the latest value and updates the BarGraph display accordingly.
*******************************************************************/
static void Get_value(XtPointer client_data, XtIntervalId *id)
{
    static XcCallData call_data;
    Widget w = (Widget)client_data;
    BarGraphWidget wb = (BarGraphWidget)client_data;

  /* Get the new value by calling the application's callback if it exists */
    if(wb->barGraph.update_callback == NULL) return;

  /* Re-register this TimeOut procedure for the next interval */
    if(wb->barGraph.interval > 0)
      wb->barGraph.interval_id =
	XtAppAddTimeOut(XtWidgetToApplicationContext(w),
	  wb->barGraph.interval, Get_value, client_data);

  /* Set the widget's current value and datatype before calling the callback */
    call_data.dtype = wb->value.datatype;
    call_data.decimals = wb->value.decimals;
    if((wb->value.datatype == XcLval) || (wb->value.datatype == XcHval))
      call_data.value.lval = wb->value.val.lval;
    else if(wb->value.datatype == XcFval)
      call_data.value.fval = wb->value.val.fval;
    XtCallCallbacks(w, XcNupdateCallback, &call_data);

  /* Update the new value, update the BarGraph display */
    if((wb->value.datatype == XcLval) || (wb->value.datatype == XcHval))
      wb->value.val.lval = call_data.value.lval;
    else if(wb->value.datatype == XcFval)
      wb->value.val.fval = call_data.value.fval;

    if(XtIsRealized(w))
      Draw_display(w, XtDisplay(w), XtWindow(w), wb->control.gc);
}

void XcBGUpdateValue(Widget w, XcVType *value)
  /*************************************************************************
   * Xc Bar Graph Update Value:  This convenience function is called by the*
   *   application in order to update the value (a little quicker than     *
   *   using XtSetArg/XtSetValue).  The application passes the new value   *
   *   to be updated with.                                                 *
   *************************************************************************/
{
    BarGraphWidget wb = (BarGraphWidget)w;

  /* Update the new value, then update the BarGraph display. */
    if(value != NULL) {
	if((wb->value.datatype == XcLval) || (wb->value.datatype == XcHval))
	  wb->value.val.lval = value->lval;
	else if(wb->value.datatype == XcFval)
	  wb->value.val.fval = value->fval;

	if(XtIsRealized(w))
	  Draw_display(w, XtDisplay(w), XtWindow(w), wb->control.gc);
    }
}

/*******************************************************************
 NAME:		XcBGUpdateBarForeground.
 DESCRIPTION:
   This convenience function is called by the application in order to
 update the value (a little quicker than using XtSetArg/XtSetValue).
 The application passes the new value to be updated with.

*******************************************************************/

void XcBGUpdateBarForeground(Widget w, Pixel pixel)
{
    BarGraphWidget wb = (BarGraphWidget)w;

#ifdef USE_CORE_VISIBLE
    if(!wb->core.visible) return;
#endif

  /* Update the new value, then update the BarGraph display. */
    if(wb->barGraph.bar_foreground != pixel) {
	wb->barGraph.bar_foreground = pixel;
	if(XtIsRealized(w))
	  Draw_display(w, XtDisplay(w), XtWindow(w), wb->control.gc);
    }
}

static void Draw_display(Widget w, Display *display,
  Drawable drawable, GC gc)
  /*************************************************************************
   * Draw Display: This function redraws the Bar indicator and the value   *
   *   string in the Value Box.                                            *
   *************************************************************************/
{
    BarGraphWidget wb = (BarGraphWidget)w;
    Drawable tempDrawable;
    Pixmap pixmap = (Pixmap)0;
    Position x0, y0, x = 0, y = 0;
    Dimension width = 0, height = 0;
    int  len, mid, d;
    float dim = 0.0;
    char *temp;

  /* Reset any clip mask */
    XSetClipMask(display, gc, None);

  /* Handle double buffer */
    if(wb->barGraph.double_buffer) {
	pixmap = XCreatePixmap(XtDisplay(w), XtWindow(w),
	  wb->barGraph.bar.width, wb->barGraph.bar.height,
	  XDefaultDepth(XtDisplay(w), DefaultScreen(XtDisplay(w))));
	if(pixmap) tempDrawable = pixmap;
	else tempDrawable = drawable;
	x0 = 0;
	y0 = 0;
    } else {
	x0 = wb->barGraph.bar.x;
	y0 = wb->barGraph.bar.y;
	tempDrawable = drawable;
    }

  /* Draw the Bar indicator, fill the Bar with its background color */
    XSetForeground(display, gc, wb->barGraph.bar_background);
    XFillRectangle(display, tempDrawable, gc, x0, y0,
      wb->barGraph.bar.width, wb->barGraph.bar.height);

  /* Determine the range depending on horizontal or vertical */
    if(wb->barGraph.orient == XcVert || wb->barGraph.orient == XcVertDown)
      len = wb->barGraph.bar.height;
    else len = wb->barGraph.bar.width;

  /* Calculate the dim value (length of val-min in pixels) */
    if((wb->value.datatype == XcLval) || (wb->value.datatype == XcHval))
      dim = Correlate(((float)(wb->value.val.lval)
	- (float)(wb->value.lower_bound.lval)),
	((float)(wb->value.upper_bound.lval) -
	  (float)(wb->value.lower_bound.lval)), (float)len);
    else if(wb->value.datatype == XcFval)
      dim = Correlate((wb->value.val.fval - wb->value.lower_bound.fval),
	(wb->value.upper_bound.fval - wb->value.lower_bound.fval), (float)len);

  /* Determine the x, y, width, and height of the bar depending on the
     properties */
    XSetForeground(display, gc, wb->barGraph.bar_foreground);
    mid = len/2;
  /* KE: The following roundoff uses .49 instead of .5 so that bars
     with limits (-L,L) in XcCenter mode show no bar when the val is
     0. This is because it needs to round down when dim is nnnn.5 as
     happens when len is odd. Note that the original implementation
     did not round, just took the integer part.  */
    d = (int)(dim + .49);
  /* KE: Formerly the minimum value of d was 1, not 0.  This left a
     trace of a bar showing when the value was at the lower limit. */
    if(d < 0) d = 0;
    if(d > len) d = len;
    switch(wb->barGraph.orient) {
    case XcVert:
	x = x0;
	width = wb->barGraph.bar.width;
	if(wb->barGraph.fillmod == XcCenter) {
	    if(d > mid) {
		y = y0 + len - d;
		height = d - mid;

	    } else {
		y = y0 + mid;
		height = mid - d;
	    }
	} else {
	    y = y0 + len - d;
	    height = d;
	}
	break;
    case XcVertDown:
	x = x0;
	width = wb->barGraph.bar.width;
	if(wb->barGraph.fillmod == XcCenter) {
	    if(d > mid) {
		y = y0 + mid;
		height = d - mid;

	    } else {
		y = y0 + d;
		height = mid - d;
	    }
	} else {
		y = y0;
		height = d;
	}
	break;
    case XcHoriz:
	y = y0;
	height = wb->barGraph.bar.height;
	if(wb->barGraph.fillmod == XcCenter) {
	    if(d > mid) {
		x = x0 + mid;
		width =  d - mid;

	    } else {
		x = x0 + d;
		width = mid - d;
	    }
	} else {
		x = x0;
		width = d;
	}
	break;
    case XcHorizLeft:
	y = y0;
	height = wb->barGraph.bar.height;
	if(wb->barGraph.fillmod == XcCenter) {
	    if(d > mid) {
		x = x0 + len - d;
		width = d - mid;

	    } else {
		x = x0 + mid;
		width = mid - d;
	    }
	} else {
		x = x0 + len - d;
		width = d;
	}
	break;
    }

  /* Draw the bar */
    if(width > 0 && height > 0)
      XFillRectangle(display, tempDrawable, gc, x, y, width, height);

  /* If double buffered, copy the pixmap */
    if(wb->barGraph.double_buffer && pixmap) {
	XCopyArea(XtDisplay(w), tempDrawable, drawable, gc, 0, 0,
	  wb->barGraph.bar.width, wb->barGraph.bar.height,
	  wb->barGraph.bar.x, wb->barGraph.bar.y);
	XFreePixmap(XtDisplay(w), pixmap);
    }

#if DEBUG_DRAW
    printf("Draw_display: low=%f val=%f high=%f dim=%f\n",
      wb->value.lower_bound.fval,
      wb->value.val.fval,
      wb->value.lower_bound.fval,
      dim);
    printf("  len=%d mid=%d d=%d\n",len,mid,d);
#endif

  /* If no decorations, return */
    if(wb->barGraph.decorations == False) return;

  /*If the value string is supposed to be displayed, draw it */
    if(wb->barGraph.value_visible == True) {
      /* Clear the Value Box by re-filling it with its background color */
	XSetForeground(display, gc, wb->value.value_bg_pixel);
	XFillRectangle(display, drawable, gc,
	  wb->value.value_box.x, wb->value.value_box.y,
	  wb->value.value_box.width, wb->value.value_box.height);

      /* Now draw the value string in its foreground color, clipped by
         the Value Box */
	XSetForeground(display, gc, wb->value.value_fg_pixel);
	XSetClipRectangles(display, gc, 0, 0,
	  &(wb->value.value_box), 1, Unsorted);

	temp = Print_value(wb->value.datatype, &wb->value.val,
	  wb->value.decimals);

	Position_val(w);

	XDrawString(display, drawable, gc,
	  wb->value.vp.x, wb->value.vp.y, temp, strlen(temp));
    }

  /* Reset the clip_mask to no clipping */
    XSetClipMask(display, gc, None);
}

/*******************************************************************
 NAME:		Print_bounds.
 DESCRIPTION:
   This is a utility function used by the Redisplay and Resize methods to
 print the upper and lower bound values as strings for displaying and resizing
 purposes.
*******************************************************************/

static void Print_bounds(Widget w, char *upper, char *lower)
{
    BarGraphWidget wb = (BarGraphWidget)w;

    if(wb->value.datatype == XcLval) {
	cvtLongToString(wb->value.upper_bound.lval, upper);
	cvtLongToString(wb->value.lower_bound.lval, lower);
    } else if(wb->value.datatype == XcHval) {
	cvtLongToHexString(wb->value.upper_bound.lval, upper);
	cvtLongToHexString(wb->value.lower_bound.lval, lower);
    } else if(wb->value.datatype == XcFval) {
	cvtFloatToString(wb->value.upper_bound.fval, upper,
	  (unsigned short)wb->value.decimals);
	cvtFloatToString(wb->value.lower_bound.fval, lower,
	  (unsigned short)wb->value.decimals);
    }
}
