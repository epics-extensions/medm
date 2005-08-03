/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*******************************************************************
 FILE:		Indicator.c
 CONTENTS:	Definitions for structures, methods, and actions of the
		Indicator widget.
 AUTHOR:	Mark Anderson, derived from Paul D. Johnston's BarGraf widget
********************************************************************/

#include <stdio.h>

/* Xlib includes */
#include <X11/Xlib.h>

/* Xt includes */
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>


/* Widget includes */
#include "Xc.h"
#include "Control.h"
#include "Value.h"
#include "IndicatorP.h"

#include "cvtFast.h"

#ifndef MIN
# define  MIN(a,b)    (((a) < (b)) ? (a) :  (b))
#endif
#ifndef MAX
#  define  MAX(a,b)    (((a) > (b)) ? (a) :  (b))
#endif

/* Macro redefinition for offset. */
#define offset(field) XtOffset(IndicatorWidget, field)

/* Function prototypes for widget methods */
static void ClassInitialize(void);
static void Initialize(Widget request, Widget new,
  ArgList args, Cardinal *nargs);
static void Redisplay(Widget w, XEvent *event, Region region);
static Boolean SetValues(Widget cur, Widget req,
  Widget new, ArgList args, Cardinal *nargs);
static void Resize(Widget w);
static XtGeometryResult QueryGeometry(Widget w, XtWidgetGeometry *proposed,
  XtWidgetGeometry *answer);
static void Destroy(Widget w);
static void Get_value(XtPointer client_data, XtIntervalId *id);
static void Draw_display(Widget w, Display *display,
  Drawable drawable, GC gc);
static void Print_bounds(Widget w, char *upper, char *lower);

/* Define the widget's resource list */
static XtResource resources[] =
{
    {
	XcNorient,
	XcCOrient,
	XcROrient,
	sizeof(XcOrient),
	offset(indicator.orient),
	XtRString,
	"vertical"
    },
    {
	XcNindicatorForeground,
	XtCColor,
	XtRPixel,
	sizeof(Pixel),
	offset(indicator.indicator_foreground),
	XtRString,
	XtDefaultForeground
    },
    {
	XcNindicatorBackground,
	XtCColor,
	XtRPixel,
	sizeof(Pixel),
	offset(indicator.indicator_background),
	XtRString,
	XtDefaultBackground
    },
    {
	XcNscaleColor,
	XtCColor,
	XtRPixel,
	sizeof(Pixel),
	offset(indicator.scale_pixel),
	XtRString,
	XtDefaultForeground
    },
    {
	XcNscaleSegments,
	XcCScaleSegments,
	XtRInt,
	sizeof(int),
	offset(indicator.num_segments),
	XtRImmediate,
	(XtPointer)7
    },
    {
	XcNvalueVisible,
	XtCBoolean,
	XtRBoolean,
	sizeof(Boolean),
	offset(indicator.value_visible),
	XtRString,
	"True"
    },
    {
	XcNinterval,
	XcCInterval,
	XtRInt,
	sizeof(int),
	offset(indicator.interval),
	XtRImmediate,
	(XtPointer)0
    },
    {
	XcNupdateCallback,
	XtCCallback,
	XtRCallback,
	sizeof(XtPointer),
	offset(indicator.update_callback),
	XtRCallback,
	NULL
    },
};




/* Widget Class Record initialization */
IndicatorClassRec indicatorClassRec =
{
    {
      /* core_class part */
        (WidgetClass) &valueClassRec,    /* superclass */
        "Indicator",                   /* class_name */
        sizeof(IndicatorRec),          /* widget_size */
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
        XtExposeCompressMaximal,       /* compress_exposure */
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
    {
      /* Control class part */
        0,                             /* dummy_field */
    },
    {
      /* Value class part */
        0,                             /* dummy_field */
    },
    {
      /* Indicator class part */
        0,                                              /* dummy_field */
    }
};

WidgetClass xcIndicatorWidgetClass = (WidgetClass)&indicatorClassRec;


/* Widget method function definitions */

/*******************************************************************
 NAME:		ClassInitialize.
 DESCRIPTION:
   This method initializes the Indicator widget class. Specifically,
 it registers resource value converter functions with Xt.

*******************************************************************/

static void ClassInitialize(void)
{
    XtAddConverter(XtRString, XcROrient, CvtStringToOrient, NULL, 0);
}

/*******************************************************************
 NAME:		Initialize.
 DESCRIPTION:
   This is the initialize method for the Indicator widget.  It
 validates user-modifiable instance resources and initializes private
 widget variables and structures.  This function also creates any server
 resources (i.e., GCs, fonts, Pixmaps, etc.) used by this widget.  This
 method is called by Xt when the application calls XtCreateWidget().

*******************************************************************/

static void Initialize(Widget request, Widget new,
  ArgList args, Cardinal *nargs)
  /* KE: request, args, nargs are not used */
  /* KE: The definition of XtInitProc seems to be wrong in
   *   O'Reilly Vol. 4, p. 494 */
{
    IndicatorWidget wnew = (IndicatorWidget)new;

    DPRINTF(("Indicator: executing Initialize...\n"));

  /*
   * Validate public instance variable settings.
   */
  /* Check orientation resource setting. */
    if ((wnew->indicator.orient != XcVert) &&
      (wnew->indicator.orient != XcHoriz)) {
	XtWarning("Indicator: invalid orientation setting");
	wnew->indicator.orient = XcVert;
    }

  /* Check the interval resource setting. */
    if (wnew->indicator.interval >0) {
	wnew->indicator.interval_id =
	  XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)new),
	    wnew->indicator.interval, Get_value, new);
    }

  /* Check the scaleSegments resource setting. */
    if (wnew->indicator.num_segments < MIN_SCALE_SEGS) {
	XtWarning("Indicator: invalid number of scale segments");
	wnew->indicator.num_segments = MIN_SCALE_SEGS;
    } else if (wnew->indicator.num_segments > MAX_SCALE_SEGS) {
	XtWarning("Indicator: invalid number of scale segments");
	wnew->indicator.num_segments = MAX_SCALE_SEGS;
    }

  /* Check the valueVisible resource setting. */
    if ((wnew->indicator.value_visible != True) &&
      (wnew->indicator.value_visible != False))	{
	XtWarning("Indicator: invalid valueVisible setting");
	wnew->indicator.value_visible = True;
    }

  /* Initialize the Indicator width and height. */
    if (wnew->core.width < MIN_INDICATOR_WIDTH)
      wnew->core.width = MIN_INDICATOR_WIDTH;
    if (wnew->core.height < MIN_INDICATOR_HEIGHT)
      wnew->core.height = MIN_INDICATOR_HEIGHT;

  /* Initialize private instance variables.  */

  /* Set the initial geometry of the Indicator elements. */
    Resize(new);

    DPRINTF(("Indicator: done Initialize\n"));
}

/*******************************************************************
 NAME:		Redisplay.
 DESCRIPTION:
   This function is the Indicator's Expose method.  It redraws the
 Indicator's 3D rectangle background, Value Box, label, Bar indicator,
 and the Scale.  All drawing takes place within the widget's window
 (no need for an off-screen pixmap).
*******************************************************************/

static void Redisplay(Widget w, XEvent *event, Region region)
{
    IndicatorWidget wi = (IndicatorWidget)w;
    int j;
    char upper[30], lower[30];

  /*
   * Check to see whether or not the widget's window is mapped.  You can't
   * draw into a window that is not mapped.  Realizing a widget doesn't
   * mean its mapped, but this call will work for most Window Managers.
   */
#ifdef USE_CORE_VISIBLE
    if (!XtIsRealized((Widget)w) || !wi->core.visible) return;
#else
    if (!XtIsRealized((Widget)w)) return;
#endif

    DPRINTF(("Indicator: executing Redisplay\n"));

  /* Draw the 3D rectangle background for the Indicator. */
    XSetClipMask(XtDisplay(w), wi->control.gc, None);
    Rect3d(w, XtDisplay(w), XtWindow(w), wi->control.gc,
      0, 0, wi->core.width, wi->core.height, RAISED);

  /* Draw the Label string. */
    XSetClipRectangles(XtDisplay(w), wi->control.gc, 0, 0,
      &(wi->indicator.face), 1, Unsorted);
    XSetForeground(XtDisplay(w), wi->control.gc,
      wi->control.label_pixel);
    XDrawString(XtDisplay(w), XtWindow(w), wi->control.gc,
      wi->indicator.lbl.x, wi->indicator.lbl.y,
      wi->control.label, strlen(wi->control.label));

  /* Draw the Scale */
    if (wi->indicator.num_segments > 0) {
	XSetForeground(XtDisplay(w), wi->control.gc,
	  wi->indicator.scale_pixel);
	XDrawLine(XtDisplay(w), XtWindow(w), wi->control.gc,
	  wi->indicator.scale_line.x1, wi->indicator.scale_line.y1,
	  wi->indicator.scale_line.x2, wi->indicator.scale_line.y2);

      /* Draw the max and min value segments. */
	if (wi->indicator.orient == XcVert) {
	} else {
	    XDrawLine(XtDisplay(w), XtWindow(w), wi->control.gc,
	      wi->indicator.scale_line.x1,
	      wi->indicator.scale_line.y1 - wi->indicator.seg_length,
	      wi->indicator.scale_line.x1, wi->indicator.scale_line.y1);
	    XDrawLine(XtDisplay(w), XtWindow(w), wi->control.gc,
	      wi->indicator.scale_line.x2,
	      wi->indicator.scale_line.y2 - wi->indicator.seg_length,
	      wi->indicator.scale_line.x2, wi->indicator.scale_line.y2);
	}

      /* Now draw the rest of the Scale segments. */
	for (j = 0; j < wi->indicator.num_segments; j++) {
	    if (wi->indicator.orient == XcVert)
	      XDrawLine(XtDisplay(w), XtWindow(w), wi->control.gc,
		wi->indicator.segs[j].x, wi->indicator.segs[j].y,
		wi->indicator.scale_line.x1, wi->indicator.segs[j].y);
	    else
	      XDrawLine(XtDisplay(w), XtWindow(w), wi->control.gc,
		wi->indicator.segs[j].x, wi->indicator.segs[j].y,
		wi->indicator.segs[j].x, wi->indicator.scale_line.y1);
	}

      /* Draw the max and min value string indicators */
	Print_bounds(w, upper, lower);
	XDrawString(XtDisplay(w), XtWindow(w), wi->control.gc,
	  wi->indicator.max_val.x, wi->indicator.max_val.y,
	  upper, strlen(upper));
	XDrawString(XtDisplay(w), XtWindow(w), wi->control.gc,
	  wi->indicator.min_val.x, wi->indicator.min_val.y,
	  lower, strlen(lower));
    }

  /* Draw the Bar indicator border */
    Rect3d(w, XtDisplay(w), XtWindow(w), wi->control.gc,
      wi->indicator.indicator.x - wi->control.shade_depth,
      wi->indicator.indicator.y - wi->control.shade_depth,
      wi->indicator.indicator.width + (2*wi->control.shade_depth),
      wi->indicator.indicator.height + (2*wi->control.shade_depth), DEPRESSED);

  /* Draw the Value Box */
    if (wi->indicator.value_visible == True)
      Rect3d(w, XtDisplay(w), XtWindow(w), wi->control.gc,
	wi->value.value_box.x - wi->control.shade_depth,
	wi->value.value_box.y - wi->control.shade_depth,
	wi->value.value_box.width + (2*wi->control.shade_depth),
	wi->value.value_box.height + (2*wi->control.shade_depth), DEPRESSED);

  /* Draw the new value represented by the Bar indicator and the value string */
    Draw_display(w, XtDisplay(w), XtWindow(w), wi->control.gc);

    DPRINTF(("Indicator: done Redisplay\n"));

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
    IndicatorWidget wnew = (IndicatorWidget)new;
    IndicatorWidget wcur = (IndicatorWidget)cur;
    Boolean do_redisplay = False, do_resize = False;

    DPRINTF(("Indicator: executing SetValues \n"));

  /* Validate new resource settings. */

  /* Check widget color resource settings. */
    if ((wnew->indicator.indicator_foreground
      != wcur->indicator.indicator_foreground) ||
      (wnew->indicator.indicator_background
	!= wcur->indicator.indicator_background) ||
      (wnew->indicator.scale_pixel != wcur->indicator.scale_pixel))
      do_redisplay = True;

  /* Check orientation resource setting. */
    if (wnew->indicator.orient != wcur->indicator.orient) {
	do_redisplay = True;
	if ((wnew->indicator.orient != XcVert)
	  && (wnew->indicator.orient != XcHoriz)) {
	    XtWarning("Indicator: invalid orientation setting");
	    wnew->indicator.orient = XcVert;
	}
    }

  /* Check the interval resource setting. */
    if (wnew->indicator.interval != wcur->indicator.interval) {
	if (wcur->indicator.interval > 0)
	  XtRemoveTimeOut (wcur->indicator.interval_id);
	if (wnew->indicator.interval > 0)
	  wnew->indicator.interval_id =
	    XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)new),
	      wnew->indicator.interval, Get_value, new);
    }

  /* Check the scaleSegments resource setting. */
    if (wnew->indicator.num_segments != wcur->indicator.num_segments) {
	if (wnew->indicator.num_segments < MIN_SCALE_SEGS) {
	    XtWarning("Indicator: invalid number of scale segments");
	    wnew->indicator.num_segments = MIN_SCALE_SEGS;
	} else if (wnew->indicator.num_segments > MAX_SCALE_SEGS) {
	    XtWarning("Indicator: invalid number of scale segments");
	    wnew->indicator.num_segments = MAX_SCALE_SEGS;
	}
    }

  /* Check the valueVisible resource setting. */
    if (wnew->indicator.value_visible != wcur->indicator.value_visible) {
	do_redisplay = True;
	if ((wnew->indicator.value_visible != True) &&
	  (wnew->indicator.value_visible != False)) {
	    XtWarning("Indicator: invalid valueVisible setting");
	    wnew->indicator.value_visible = True;
	}
    }

  /* Check to see if the value has changed. */
    if ((((wnew->value.datatype == XcLval) || (wnew->value.datatype == XcHval)) &&
      (wnew->value.val.lval != wcur->value.val.lval))
      || ((wnew->value.datatype == XcFval) &&
	(wnew->value.val.fval != wcur->value.val.fval))) {
	do_redisplay = True;
    }

  /* (MDA) want to force resizing if min/max changed  or decimals setting */
    if (wnew->value.decimals != wcur->value.decimals) do_resize = True;

    if ( ((wnew->value.datatype == XcLval) || (wnew->value.datatype == XcHval)) &&
      ( (wnew->value.lower_bound.lval != wcur->value.lower_bound.lval) ||
	(wnew->value.upper_bound.lval != wcur->value.upper_bound.lval) )) {
	do_resize = True;
    } else if ( (wnew->value.datatype == XcFval) &&
      ( (wnew->value.lower_bound.fval != wcur->value.lower_bound.fval) ||
	(wnew->value.upper_bound.fval != wcur->value.upper_bound.fval) )) {
	do_resize = True;
    }
    if (do_resize) {
        Resize(new);
        do_redisplay = True;
    }

    DPRINTF(("Indicator: done SetValues\n"));
    return do_redisplay;
}

/*******************************************************************
 NAME:		Resize.
 DESCRIPTION:
   This is the resize method of the Indicator widget. It resizes the
 Indicator's graphics based on the new width and height of the widget's
 window.
*******************************************************************/

static void Resize(Widget w)
{
    IndicatorWidget wi = (IndicatorWidget)w;
    int j;
    int seg_spacing;
    int max_val_width, min_val_width, max_width;
    int font_center, font_height;
    char upper[30], lower[30];
    Boolean displayValue, displayLabel;

    DPRINTF(("Indicator: executing Resize\n"));

  /* (MDA) for numbers, usually safe to ignore descent to save space */
    font_height = (wi->control.font)->ascent;

  /* Set the widgets new width and height. */
    wi->indicator.face.x = wi->indicator.face.y = wi->control.shade_depth;
    wi->indicator.face.width = wi->core.width - (2*wi->control.shade_depth);
    wi->indicator.face.height = wi->core.height - (2*wi->control.shade_depth);

  /* Calculate min/max string attributes */
    Print_bounds(w, upper, lower);
    max_val_width = XTextWidth(wi->control.font, upper, strlen(upper));
    min_val_width = XTextWidth(wi->control.font, lower, strlen(lower));
    max_width = MAX(min_val_width,max_val_width) + 2*wi->control.shade_depth;
    if (wi->indicator.num_segments == 0) max_width = wi->indicator.face.width
					   - 2*wi->control.shade_depth;

  /* Establish the new Value Box geometry. */
    if (wi->indicator.value_visible == True) {
	displayValue = True;
	if (wi->indicator.orient == XcVert) {
	    wi->value.value_box.x = wi->core.width/2 - max_width/2;
	    wi->value.value_box.y = wi->core.height - font_height -
	      3*wi->control.shade_depth;
	    wi->value.value_box.width = max_width;
	    wi->value.value_box.height = font_height;
	} else {
	    wi->value.value_box.x = wi->core.width/2 - max_width/2;
	    wi->value.value_box.y = wi->core.height - font_height -
	      2*wi->control.shade_depth -2;
	    wi->value.value_box.width = max_width;
	    wi->value.value_box.height = font_height;
	}
      /* Set the position of the displayed value within the Value Box. */
	Position_val(w);
    } else {
	displayValue = False;
	wi->value.value_box.x = 0;
	wi->value.value_box.y = wi->core.height - 2*wi->control.shade_depth;
	wi->value.value_box.width = 0;
	wi->value.value_box.height = 0;
    }

  /* Set the new label location. */
    if (strlen(wi->control.label) > 1 ||
      (strlen(wi->control.label) == 1 && wi->control.label[0] != ' ')) {
	displayLabel = True;
	wi->indicator.lbl.x = (wi->core.width / 2) -
	  (XTextWidth(wi->control.font, wi->control.label,
	    strlen(wi->control.label)) / 2);
	wi->indicator.lbl.y = wi->indicator.face.y + wi->control.font->ascent + 1;
    } else {
	displayLabel = False;
	wi->indicator.lbl.x = wi->indicator.face.x;
	wi->indicator.lbl.y = wi->indicator.face.y;
    }

  /* Resize the Bar indicator */
    if (wi->indicator.orient == XcVert) {
	wi->indicator.indicator.x = wi->indicator.face.x
	  + (wi->indicator.num_segments > 0 ? max_width +
	    + (wi->indicator.face.width / 32) : 0)
	  + 2*wi->control.shade_depth;
	wi->indicator.indicator.y = wi->indicator.lbl.y  + wi->control.font->descent
	  + 2*wi->control.shade_depth;
	wi->indicator.indicator.width = MAX(0,(int)(wi->indicator.face.width
	  - wi->indicator.indicator.x - wi->control.shade_depth));
	wi->indicator.indicator.height = wi->value.value_box.y
	  - wi->indicator.indicator.y
	  - (displayValue == True
	    ? font_height - wi->control.font->descent
	    : 2*wi->control.shade_depth);
    } else {
	wi->indicator.indicator.x = wi->indicator.face.x +
	  (short)(wi->indicator.face.width / 16) +
	  wi->control.shade_depth;
	wi->indicator.indicator.y =  wi->indicator.lbl.y
	  + (wi->indicator.num_segments > 0 ?
	    font_height/2 + wi->indicator.face.height/8 + 1 :
	    0)
	  + 2*wi->control.shade_depth;
	wi->indicator.indicator.width = (short)
	  ((9 * (int)wi->indicator.face.width)/10) - 2*wi->control.shade_depth;
	wi->indicator.indicator.height = MAX(1,
	  wi->value.value_box.y - wi->indicator.indicator.y
	  - (displayValue == True ? font_height : 0)
	  - wi->control.shade_depth);
    }

  /* Resize the Scale line. */
    if (wi->indicator.orient == XcVert)	{
	wi->indicator.scale_line.x1 = wi->indicator.indicator.x
	  - wi->control.shade_depth - (wi->indicator.face.width / 32);
	wi->indicator.scale_line.y1 = wi->indicator.indicator.y;
	wi->indicator.scale_line.x2 = wi->indicator.scale_line.x1;
	wi->indicator.scale_line.y2 = wi->indicator.indicator.y
	  + wi->indicator.indicator.height;
    } else {
	wi->indicator.scale_line.x1 = wi->indicator.indicator.x;
	wi->indicator.scale_line.y1 = wi->indicator.indicator.y
	  - wi->control.shade_depth - (wi->indicator.face.height / 32);
	wi->indicator.scale_line.x2 = wi->indicator.indicator.x
	  + wi->indicator.indicator.width;
	wi->indicator.scale_line.y2 = wi->indicator.scale_line.y1;
    }

  /* Now, resize Scale line segments */
    if (wi->indicator.num_segments > 0)	{
	if (wi->indicator.orient == XcVert) {
	    wi->indicator.seg_length = (wi->indicator.face.width / 16);
	    seg_spacing = ((int)wi->indicator.indicator.height /
	      (wi->indicator.num_segments + 1));
	    for (j = 0; j < wi->indicator.num_segments; j++) {
		wi->indicator.segs[j].x = wi->indicator.scale_line.x1 -
		  wi->indicator.seg_length;
		wi->indicator.segs[j].y = wi->indicator.scale_line.y1 +
		  ((j+1) * seg_spacing);
	    }
	} else {
	    wi->indicator.seg_length = (wi->indicator.face.height / 16);
	    seg_spacing = ((int)wi->indicator.indicator.width /
	      (wi->indicator.num_segments + 1));
	    for (j = 0; j < wi->indicator.num_segments; j++) {
		wi->indicator.segs[j].x = wi->indicator.scale_line.x1 +
		  ((j+1) * seg_spacing);
		wi->indicator.segs[j].y = wi->indicator.scale_line.y1 -
		  wi->indicator.seg_length;
	    }
	}
    }

  /* Set the position of the max and min value strings */
    if (wi->indicator.orient == XcVert)	{
	font_center = ((wi->control.font->ascent +
	  wi->control.font->descent) / 2)
	  - wi->control.font->descent;
	wi->indicator.max_val.x = MAX(wi->control.shade_depth,
	  wi->indicator.scale_line.x2 - (max_val_width));
	wi->indicator.max_val.y = wi->indicator.scale_line.y1 + font_center;
	wi->indicator.min_val.x = MAX(wi->control.shade_depth,
	  wi->indicator.scale_line.x2 - (min_val_width));
	wi->indicator.min_val.y = wi->indicator.scale_line.y2 + font_center;
    } else {
	wi->indicator.max_val.x = MIN(
	  (int)(wi->indicator.face.width + wi->control.shade_depth
	    - max_val_width),
	  wi->indicator.scale_line.x2 - (max_val_width / 2));
	wi->indicator.max_val.y = wi->indicator.min_val.y =
	  wi->indicator.scale_line.y1 - wi->indicator.seg_length - 1;
	wi->indicator.min_val.x = MAX(wi->control.shade_depth,
	  wi->indicator.scale_line.x1 - (min_val_width / 2));
    }

    DPRINTF(("Indicator: done Resize\n"));
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
    IndicatorWidget wi = (IndicatorWidget)w;

  /* Set the request mode mask for the returned answer. */
    answer->request_mode = CWWidth | CWHeight;

  /* Set the recommended size. */
    answer->width = (wi->core.width > MAX_INDICATOR_WIDTH)
      ? MAX_INDICATOR_WIDTH : wi->core.width;
    answer->height = (wi->core.height > MAX_INDICATOR_HEIGHT)
      ? MAX_INDICATOR_HEIGHT : wi->core.height;

  /*
   * Check the proposed dimensions. If the proposed size is larger than
   * appropriate, return the recommended size.
   */
    if (((proposed->request_mode & (CWWidth | CWHeight)) == (CWWidth | CWHeight))
      && proposed->width == answer->width
      && proposed->height == answer->height)
      return XtGeometryYes;
    else if (answer->width == wi->core.width && answer->height == wi->core.height)
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
    IndicatorWidget wi = (IndicatorWidget)w;

    if (wi->indicator.interval > 0)
      XtRemoveTimeOut (wi->indicator.interval_id);
}
/* Widget action functions. */

/*******************************************************************
 NAME:		Get_value.
 DESCRIPTION:
   This function is the time out procedure called at XcNinterval
 intervals.  It calls the application registered callback function to
 get the latest value and updates the Indicator display accordingly.
*******************************************************************/

static void Get_value(XtPointer client_data, XtIntervalId *id)
{
  /* Local variables */
    static XcCallData call_data;
    Widget w = (Widget)client_data;
    IndicatorWidget wi = (IndicatorWidget)client_data;

  /* Get the new value by calling the application's callback if it exists. */
    if (wi->indicator.update_callback == NULL)
      return;

  /* Re-register this TimeOut procedure for the next interval. */
    if (wi->indicator.interval > 0)
      wi->indicator.interval_id =
	XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)w),
	  wi->indicator.interval, Get_value, client_data);

  /* Set the widget's current value and datatype before calling the callback. */
    call_data.dtype = wi->value.datatype;
    call_data.decimals = wi->value.decimals;
    if ((wi->value.datatype == XcLval) || (wi->value.datatype == XcHval))
      call_data.value.lval = wi->value.val.lval;
    else if (wi->value.datatype == XcFval)
      call_data.value.fval = wi->value.val.fval;
    XtCallCallbacks((Widget)w, XcNupdateCallback, &call_data);


  /* Update the new value, update the Indicator display. */
    if ((wi->value.datatype == XcLval) || (wi->value.datatype == XcHval))
      wi->value.val.lval = call_data.value.lval;
    else if (wi->value.datatype == XcFval)
      wi->value.val.fval = call_data.value.fval;

    if (XtIsRealized((Widget)w))
      Draw_display(w, XtDisplay(w), XtWindow(w), wi->control.gc);
}

/*******************************************************************
 NAME:		XcIndUpdateValue.
 DESCRIPTION:
   This convenience function is called by the application in order to
update the value (a little quicker than using XtSetArg/XtSetValue).
The application passes the new value to be updated with.

*******************************************************************/

void XcIndUpdateValue(Widget w, XcVType *value)
{
    IndicatorWidget wi = (IndicatorWidget)w;

#ifdef USE_CORE_VISIBLE
    if (!wi->core.visible) return;
#endif

  /* Update the new value, then update the Indicator display. */
    if (value != NULL) {
	if ((wi->value.datatype == XcLval) || (wi->value.datatype == XcHval))
	  wi->value.val.lval = value->lval;
	else if (wi->value.datatype == XcFval)
	  wi->value.val.fval = value->fval;

	if (XtIsRealized((Widget)w))
	  Draw_display(w, XtDisplay(w), XtWindow(w), wi->control.gc);
    }

}

/*******************************************************************
 NAME:		XcIndUpdateIndicatorForeground.
 DESCRIPTION:
   This convenience function is called by the application in order to
   update the indicator foreground color (a little quicker than using
   XtSetArg/XtSetValue).
   The application passes the new value to be updated with.

*******************************************************************/

void XcIndUpdateIndicatorForeground(Widget w, Pixel pixel)
{
    IndicatorWidget wi = (IndicatorWidget)w;

#ifdef USE_CORE_VISIBLE
    if (!wi->core.visible) return;
#endif

  /* Update the new value, then update the Indicator display. */
    if (wi->indicator.indicator_foreground != pixel) {
	wi->indicator.indicator_foreground = pixel;
	if (XtIsRealized((Widget)w))
	  Draw_display(w, XtDisplay(w), XtWindow(w), wi->control.gc);
    }
}

/*******************************************************************
 NAME:		Draw_display.
 DESCRIPTION:
   This function redraws the Bar indicator and the value string in the
Value Box.

*******************************************************************/

static void Draw_display(Widget w, Display *display,
  Drawable drawable, GC gc)
{
    IndicatorWidget wi = (IndicatorWidget)w;
    XRectangle clipRect[1];
    char *temp;
    float range, dim = 0.0;
    unsigned int indicator_size;
    XPoint points[4];
    int nPoints = 4;

  /* Draw the Bar indicator */
  /* Fill the Bar with its background color. */
    XSetForeground(display, gc, wi->indicator.indicator_background);
    XFillRectangle(display, drawable, gc,
      wi->indicator.indicator.x, wi->indicator.indicator.y,
      wi->indicator.indicator.width, wi->indicator.indicator.height);

  /* Draw the Bar in its foreground color according to the value. */
    if (wi->indicator.orient == XcVert)
      range = (float)(wi->indicator.indicator.height);
    else
      range = (float)(wi->indicator.indicator.width);

    if ((wi->value.datatype == XcLval) || (wi->value.datatype == XcHval))
      dim = Correlate(((float)(wi->value.val.lval)
	- (float)(wi->value.lower_bound.lval)),
	((float)(wi->value.upper_bound.lval) -
	  (float)(wi->value.lower_bound.lval)), range);
    else if (wi->value.datatype == XcFval)
      dim = Correlate((wi->value.val.fval
	- wi->value.lower_bound.fval),
	(wi->value.upper_bound.fval -
	  wi->value.lower_bound.fval), range);

    if (wi->indicator.orient == XcVert) {
	indicator_size = MAX(5,(int)wi->indicator.indicator.height/10);
    } else {
	indicator_size = MAX(5,(int)wi->indicator.indicator.width/10);
    }

    if ((int)dim < 1)
      dim = 1;
    XSetForeground(display, gc, wi->indicator.indicator_foreground);
    clipRect[0].x = wi->indicator.indicator.x;
    clipRect[0].y = wi->indicator.indicator.y;
    clipRect[0].width = wi->indicator.indicator.width;
    clipRect[0].height = wi->indicator.indicator.height;
    XSetClipRectangles(display,gc,0,0,clipRect,1,Unsorted);

    if (wi->indicator.orient == XcVert) {
#ifndef INDICATOR_NOT_DIAMOND
	points[0].x = wi->indicator.indicator.x;
	points[0].y = wi->indicator.indicator.y
	  + wi->indicator.indicator.height - (int)dim;
	points[1].x = points[0].x + wi->indicator.indicator.width/2;
	points[1].y = points[0].y - indicator_size/2;

	points[2].x = points[1].x + (points[1].x - points[0].x);
	points[2].y = points[0].y;
	points[3].x = points[1].x;
	points[3].y = points[0].y + (points[0].y - points[1].y);
#else
      /* ACM: I do not like the diamond at all */
	points[0].x = wi->indicator.indicator.x;
	points[0].y = wi->indicator.indicator.y
	               + wi->indicator.indicator.height - (int)dim -3;
	points[1].x = points[0].x + wi->indicator.indicator.width;
	points[1].y = points[0].y;

	points[2].x = points[1].x;
	points[2].y = points[1].y +6;
	points[3].x = points[0].x;
	points[3].y = points[2].y;
#endif
    } else {
#ifndef INDICATOR_NOT_DIAMOND
	points[0].x = wi->indicator.indicator.x + (int)dim - indicator_size/2 - 1;
	points[0].y = wi->indicator.indicator.y + wi->indicator.indicator.height/2;
	points[1].x = points[0].x + indicator_size/2;
	points[1].y = points[0].y - wi->indicator.indicator.height/2;

	points[2].x = points[1].x + (points[1].x - points[0].x);
	points[2].y = points[0].y;
	points[3].x = points[1].x;
	points[3].y = points[0].y + (points[0].y - points[1].y);
#else
      /* ACM: I do not like the diamond at all */
	points[0].x = wi->indicator.indicator.x + (int)dim - 3;
	points[0].y = wi->indicator.indicator.y;
	points[1].x = points[0].x;
	points[1].y = points[0].y + wi->indicator.indicator.height;

	points[2].x = points[0].x + 6;
	points[2].y = points[1].y;
	points[3].x = points[2].x;
	points[3].y = points[0].y;
#endif
    }

    XFillPolygon(display,drawable,gc,points,nPoints,Convex,CoordModeOrigin);

    XSetClipMask(display,gc,None);


  /* If the value string is supposed to be displayed, draw it. */
    if (wi->indicator.value_visible == True) {
      /* Clear the Value Box by re-filling it with its background color. */
	XSetForeground(display, gc, wi->value.value_bg_pixel);
	XFillRectangle(display, drawable, gc,
	  wi->value.value_box.x, wi->value.value_box.y,
	  wi->value.value_box.width, wi->value.value_box.height);

      /*
       * Now draw the value string in its foreground color, clipped by the
       * Value Box.
       */
	XSetForeground(display, gc, wi->value.value_fg_pixel);
	XSetClipRectangles(display, gc, 0, 0,
	  &(wi->value.value_box), 1, Unsorted);

	temp = Print_value(wi->value.datatype, &wi->value.val, wi->value.decimals);

	Position_val(w);

	XDrawString(display, drawable, gc,
	  wi->value.vp.x, wi->value.vp.y, temp, strlen(temp));
    }

  /* Reset the clip_mask to no clipping. */
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
    IndicatorWidget wi = (IndicatorWidget)w;

    if (wi->value.datatype == XcLval) {
	cvtLongToString(wi->value.upper_bound.lval, upper);
	cvtLongToString(wi->value.lower_bound.lval, lower);
    } else if (wi->value.datatype == XcHval) {
	cvtLongToHexString(wi->value.upper_bound.lval, upper);
	cvtLongToHexString(wi->value.lower_bound.lval, lower);
    } else if (wi->value.datatype == XcFval) {
	cvtFloatToString(wi->value.upper_bound.fval, upper,
	  (unsigned short)wi->value.decimals);
	cvtFloatToString(wi->value.lower_bound.fval, lower,
	  (unsigned short)wi->value.decimals);
    }
}
