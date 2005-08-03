/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*******************************************************************
 FILE:		Meter.c
 CONTENTS:	Definitions for structures, methods, and actions of the
		Meter widget.
 AUTHOR:	Mark Anderson, derived from Paul D. Johnston's BarGraf widget
********************************************************************/

#define DEBUG_REDRAW 0

#include <stdio.h>
#include <math.h>

/* Xlib includes */
#include <X11/Xlib.h>

/* Xt includes */
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>

/* Widget includes */
#include "Xc.h"
#include "Control.h"
#include "Value.h"
#include "MeterP.h"	/* (includes Meter.h also) */

#include "cvtFast.h"

#ifndef MIN
# define  MIN(a,b)    (((a) < (b)) ? (a) :  (b))
#endif
#ifndef MAX
#  define  MAX(a,b)    (((a) > (b)) ? (a) :  (b))
#endif

/* Macro redefinition for offset. */
#define offset(field) XtOffset(MeterWidget, field)

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
	XcNmeterForeground,
	XtCColor,
	XtRPixel,
	sizeof(Pixel),
	offset(meter.meter_foreground),
	XtRString,
	XtDefaultForeground
    },
    {
	XcNmeterBackground,
	XtCColor,
	XtRPixel,
	sizeof(Pixel),
	offset(meter.meter_background),
	XtRString,
	XtDefaultBackground
    },
    {
	XcNscaleColor,
	XtCColor,
	XtRPixel,
	sizeof(Pixel),
	offset(meter.scale_pixel),
	XtRString,
	XtDefaultForeground
    },
    {
	XcNscaleSegments,
	XcCScaleSegments,
	XtRInt,
	sizeof(int),
	offset(meter.num_segments),
	XtRImmediate,
	(XtPointer)11
    },
    {
	XcNvalueVisible,
	XtCBoolean,
	XtRBoolean,
	sizeof(Boolean),
	offset(meter.value_visible),
	XtRString,
	"True"
    },
    {
	XcNinterval,
	XcCInterval,
	XtRInt,
	sizeof(int),
	offset(meter.interval),
	XtRImmediate,
	(XtPointer)0
    },
    {
	XcNupdateCallback,
	XtCCallback,
	XtRCallback,
	sizeof(XtPointer),
	offset(meter.update_callback),
	XtRCallback,
	NULL
    },
};




/* Widget Class Record initialization */
MeterClassRec meterClassRec =
{
    {
      /* core_class part */
        (WidgetClass) &valueClassRec,  /* superclass */
        "Meter",                       /* class_name */
        sizeof(MeterRec),              /* widget_size */
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
      /* Meter class part */
        0,                             /* dummy_field */
    }
};

WidgetClass xcMeterWidgetClass = (WidgetClass)&meterClassRec;


/* Widget method function definitions */

/*******************************************************************
 NAME:		ClassInitialize.
 DESCRIPTION:
   This method initializes the Meter widget class. Specifically,
 it registers resource value converter functions with Xt.
*******************************************************************/

static void ClassInitialize(void)
{
    XtAddConverter(XtRString, XcROrient, CvtStringToOrient, NULL, 0);
}

/*******************************************************************
 NAME:		Initialize.
 DESCRIPTION:
   This is the initialize method for the Meter widget.  It
 validates user-modifiable instance resources and initializes private
 widget variables and structures.  This function also creates any server
 resources (i.e., GCs, fonts, Pixmaps, etc.) used by this widget.  This
 method is called by Xt when the application calls XtCreateWidget().

*******************************************************************/

static void Initialize(Widget request, Widget new,
  ArgList args, Cardinal *nargs)
{
    MeterWidget wnew = (MeterWidget)new;
    DPRINTF(("Meter: executing Initialize...\n"));

  /*
   * Validate public instance variable settings.
   */
  /* Check the interval resource setting. */
    if (wnew->meter.interval >0) {
	wnew->meter.interval_id =
	  XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)new),
	    wnew->meter.interval, Get_value, new);
    }

  /* Check the scaleSegments resource setting. */
    if (wnew->meter.num_segments < MIN_SCALE_SEGS) {
	XtWarning("Meter: invalid number of scale segments");
	wnew->meter.num_segments = MIN_SCALE_SEGS;
    } else if (wnew->meter.num_segments > MAX_SCALE_SEGS) {
	XtWarning("Meter: invalid number of scale segments");
	wnew->meter.num_segments = MAX_SCALE_SEGS;
    }

  /* Check the valueVisible resource setting. */
    if ((wnew->meter.value_visible != True) &&
      (wnew->meter.value_visible != False)) {
	XtWarning("Meter: invalid valueVisible setting");
	wnew->meter.value_visible = True;
    }

  /* Initialize the Meter width and height. */
    if (wnew->core.width < MIN_METER_WIDTH)
      wnew->core.width = MIN_METER_WIDTH;
    if (wnew->core.height < MIN_METER_HEIGHT)
      wnew->core.height = MIN_METER_HEIGHT;

  /* Initialize private instance variables.  */

  /* Set the initial geometry of the Meter elements. */
    Resize(new);

    DPRINTF(("Meter: done Initialize\n"));
}

/*******************************************************************
 NAME:		Redisplay.
 DESCRIPTION:
   This function is the Meter's Expose method.  It redraws the
 Meter's 3D rectangle background, Value Box, label, Bar meter,
 and the Scale.  All drawing takes place within the widget's window
 (no need for an off-screen pixmap).
*******************************************************************/

static void Redisplay(Widget w, XEvent *event, Region region)
{
    MeterWidget wm = (MeterWidget)w;
    char upper[30], lower[30];

  /*
   * Check to see whether or not the widget's window is mapped.  You can't
   * draw into a window that is not mapped.  Realizing a widget doesn't
   * mean its mapped, but this call will work for most Window Managers.
   */
#if DEBUG_REDRAW
    printf("Redisplay: w=%x Realized=%s core.visible=%d\n",
      w,XtIsRealized(w)?"Yes":"No",wm->core.visible);
#endif
#ifdef USE_CORE_VISIBLE
    if (!XtIsRealized((Widget)w) || !wm->core.visible) return;
#else
    if (!XtIsRealized((Widget)w)) return;
#endif

    DPRINTF(("Meter: executing Redisplay\n"));

  /* Draw the 3D rectangle background for the Meter. */
    XSetClipMask(XtDisplay(w), wm->control.gc, None);
    Rect3d(w, XtDisplay(w), XtWindow(w), wm->control.gc,
      0, 0, wm->core.width, wm->core.height, RAISED);

  /* Draw the Label string. */
    XSetClipRectangles(XtDisplay(w), wm->control.gc, 0, 0,
      &(wm->meter.face), 1, Unsorted);
    XSetForeground(XtDisplay(w), wm->control.gc,
      wm->control.label_pixel);
    XDrawString(XtDisplay(w), XtWindow(w), wm->control.gc,
      wm->meter.lbl.x, wm->meter.lbl.y,
      wm->control.label, strlen(wm->control.label));

  /* Draw the inner rectangle which houses the meter */
#if 0
    Rect3d(w, XtDisplay(w), XtWindow(w), wm->control.gc,
      wm->meter.meter_center.x - wm->meter.meter.width/2,
      wm->meter.meter_center.y - wm->meter.meter.height,
      wm->meter.meter.width, wm->meter.meter.height,
      DEPRESSED);
#else
  /* Adjust for changes to Rect3d */
    Rect3d(w, XtDisplay(w), XtWindow(w), wm->control.gc,
      wm->meter.meter_center.x - wm->meter.meter.width/2,
      wm->meter.meter_center.y - wm->meter.meter.height,
      wm->meter.meter.width,
      wm->meter.meter.height+1,
      DEPRESSED);
#endif

  /* Now draw the Scale segments */
    XSetForeground(XtDisplay(w), wm->control.gc, wm->meter.scale_pixel);
    XDrawSegments(XtDisplay(w), XtWindow(w), wm->control.gc,
      wm->meter.segs,wm->meter.num_segments);

  /* Draw the max and min value strings */
    Print_bounds(w, upper, lower);
    XDrawString(XtDisplay(w), XtWindow(w), wm->control.gc,
      wm->meter.max_val.x, wm->meter.max_val.y,upper, strlen(upper));
    XDrawString(XtDisplay(w), XtWindow(w), wm->control.gc,
      wm->meter.min_val.x, wm->meter.min_val.y, lower, strlen(lower));

  /* Draw the Value Box */
    if (wm->meter.value_visible == True)
      Rect3d(w, XtDisplay(w), XtWindow(w), wm->control.gc,
	wm->value.value_box.x - wm->control.shade_depth,
	wm->value.value_box.y - wm->control.shade_depth,
	wm->value.value_box.width + (2 * wm->control.shade_depth),
	wm->value.value_box.height + (2 * wm->control.shade_depth),
	DEPRESSED);

  /* Draw the new value represented by the Bar meter and the value string */
    Draw_display(w, XtDisplay(w), XtWindow(w), wm->control.gc);

    DPRINTF(("Meter: done Redisplay\n"));
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
    MeterWidget wnew = (MeterWidget)new;
    MeterWidget wcur = (MeterWidget)cur;
    Boolean do_redisplay = False, do_resize = False;

    DPRINTF(("Meter: executing SetValues \n"));

  /* Validate new resource settings. */

  /* Check widget color resource settings. */
    if ((wnew->meter.meter_foreground != wcur->meter.meter_foreground) ||
      (wnew->meter.meter_background != wcur->meter.meter_background) ||
      (wnew->meter.scale_pixel != wcur->meter.scale_pixel))
      do_redisplay = True;

  /* Check the interval resource setting. */
    if (wnew->meter.interval != wcur->meter.interval) {
	if (wcur->meter.interval > 0)
	  XtRemoveTimeOut (wcur->meter.interval_id);
	if (wnew->meter.interval > 0)
	  wnew->meter.interval_id =
	    XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)new),
	      wnew->meter.interval, Get_value, new);
    }

  /* Check the scaleSegments resource setting. */
    if (wnew->meter.num_segments != wcur->meter.num_segments) {
	if (wnew->meter.num_segments < MIN_SCALE_SEGS) {
	    XtWarning("Meter: invalid number of scale segments");
	    wnew->meter.num_segments = MIN_SCALE_SEGS;
	} else if (wnew->meter.num_segments > MAX_SCALE_SEGS) {
	    XtWarning("Meter: invalid number of scale segments");
	    wnew->meter.num_segments = MAX_SCALE_SEGS;
	}
    }

  /* Check the valueVisible resource setting. */
    if (wnew->meter.value_visible != wcur->meter.value_visible) {
	do_redisplay = True;
	if ((wnew->meter.value_visible != True) &&
	      (wnew->meter.value_visible != False)) {
	    XtWarning("Meter: invalid valueVisible setting");
	    wnew->meter.value_visible = True;
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

    DPRINTF(("Meter: done SetValues\n"));
    return do_redisplay;
}

/*******************************************************************
 NAME:		Resize.
 DESCRIPTION:
   This is the resize method of the Meter widget. It resizes the
 Meter's graphics based on the new width and height of the widget's
 window.

*******************************************************************/

static void Resize(Widget w)
{
    MeterWidget wm = (MeterWidget)w;
    int j;
    int max_val_width, min_val_width, max_width;
    int font_height, marker_width, center_x, center_y;
    char upper[30], lower[30];
    double angle, cosine, sine;

    DPRINTF(("Meter: executing Resize\n"));

  /* (MDA) for numbers, usually safe to ignore descent to save space */
    font_height = (wm->control.font)->ascent;

  /* Set the widgets new width and height. */
    wm->meter.face.x = wm->meter.face.y = wm->control.shade_depth;
    wm->meter.face.width = wm->core.width - (2*wm->control.shade_depth);
    wm->meter.face.height = wm->core.height - (2*wm->control.shade_depth);

  /* Calculate min/max string attributes */
    Print_bounds(w, upper, lower);
    max_val_width = XTextWidth(wm->control.font, upper, strlen(upper));
    min_val_width = XTextWidth(wm->control.font, lower, strlen(lower));
    max_width = MAX(min_val_width,max_val_width) + 2*wm->control.shade_depth;

  /* Establish the new Value Box geometry. */
    if (wm->meter.value_visible == True) {
	wm->value.value_box.x = wm->core.width/2 - max_width/2;
	wm->value.value_box.y = wm->core.height - font_height -
	  2*wm->control.shade_depth - 2;
	wm->value.value_box.width = max_width;
	wm->value.value_box.height = font_height;
      /* Set the position of the displayed value within the Value Box. */
	Position_val(w);
    } else {
	wm->value.value_box.x = 0;
	wm->value.value_box.y = wm->core.height;
	wm->value.value_box.width = 0;
	wm->value.value_box.height = 0;
    }

  /* Set the new label location. */
    if (strlen(wm->control.label) > 1 ||
      (strlen(wm->control.label) == 1 && wm->control.label[0] != ' ')) {
	wm->meter.lbl.x = (wm->core.width / 2) -
	  (XTextWidth(wm->control.font, wm->control.label,
	    strlen(wm->control.label)) / 2);
	wm->meter.lbl.y = wm->meter.face.y + wm->control.font->ascent;
    } else {
	wm->meter.lbl.x = wm->meter.lbl.y = wm->control.shade_depth;
    }

  /* Resize the Meter */
    wm->meter.meter.width = wm->meter.face.width - (2*wm->control.shade_depth);
    wm->meter.meter.height =  MAX(0,(wm->value.value_box.y - font_height -
      wm->meter.lbl.y));
  /* Now make rectangular (w=2*h)*/
    wm->meter.meter.height = MIN((unsigned short)(wm->meter.meter.width/2),
      wm->meter.meter.height);
    wm->meter.meter.width = MIN(wm->meter.meter.width,
      (unsigned short)(2*wm->meter.meter.height));
    wm->meter.meter.x = wm->core.width/2 - wm->meter.meter.width/2;
    wm->meter.meter.y = wm->meter.lbl.y + (wm->control.font)->descent + 2;
    wm->meter.meter_center.y = wm->meter.meter.y + wm->meter.meter.height
      - wm->control.shade_depth;
    wm->meter.meter_center.x = wm->core.width/2;
    marker_width = MAX(3,(int)wm->meter.meter.width/15);
    wm->meter.inner_radius = wm->meter.meter.height - 2*wm->control.shade_depth
      - marker_width;
    wm->meter.outer_radius = wm->meter.inner_radius + marker_width;

  /* Now, resize line segments */
    if (wm->meter.num_segments > 0) {
	center_x = wm->meter.meter_center.x;
	center_y = wm->meter.meter_center.y - wm->control.shade_depth;
	for (j = 0; j < wm->meter.num_segments; j++) {
	  /* note:  we are going counter-clockwise for min to max hence subtract */
	    angle = RADIANS(MAX_ANGLE -
	      ((float)j/(float)(wm->meter.num_segments-1))
	      *(MAX_ANGLE - MIN_ANGLE));
	    cosine = cos(angle);
	    sine = sin(angle);
	    wm->meter.segs[j].x1 =
	      (short)(center_x + wm->meter.outer_radius*cosine);
	    wm->meter.segs[j].y1 =
	      (short)(center_y - wm->meter.outer_radius*sine);
	    wm->meter.segs[j].x2 =
	      (short)(center_x + wm->meter.inner_radius*cosine);
	    wm->meter.segs[j].y2 =
	      (short)(center_y - wm->meter.inner_radius*sine);
	}
    }

  /* Set the position of the max and min value strings */
    wm->meter.max_val.x = MIN((short)(wm->core.width - wm->control.shade_depth
      - max_val_width),
      wm->meter.segs[wm->meter.num_segments-1].x1);
    wm->meter.min_val.x = MAX(wm->control.shade_depth,wm->meter.segs[0].x1
      - min_val_width);
    wm->meter.max_val.y = wm->meter.min_val.y =
      wm->meter.meter.y + wm->meter.meter.height + font_height + 1;

    DPRINTF(("Meter: done Resize\n"));
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
    MeterWidget wm = (MeterWidget)w;

  /* Set the request mode mask for the returned answer. */
    answer->request_mode = CWWidth | CWHeight;

  /* Set the recommended size. */
    answer->width = (wm->core.width > MAX_METER_WIDTH)
      ? MAX_METER_WIDTH : wm->core.width;
    answer->height = (wm->core.height > MAX_METER_HEIGHT)
      ? MAX_METER_HEIGHT : wm->core.height;

  /*
   * Check the proposed dimensions. If the proposed size is larger than
   * appropriate, return the recommended size.
   */
    if (((proposed->request_mode & (CWWidth | CWHeight)) == (CWWidth | CWHeight))
      && proposed->width == answer->width
      && proposed->height == answer->height)
      return XtGeometryYes;
    else if (answer->width == wm->core.width && answer->height == wm->core.height)
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
    MeterWidget wm = (MeterWidget)w;

    if (wm->meter.interval > 0)
      XtRemoveTimeOut (wm->meter.interval_id);
}
/* Widget action functions. */

/*******************************************************************
 NAME:		Get_value.
 DESCRIPTION:
   This function is the time out procedure called at XcNinterval
 intervals.  It calls the application registered callback function to
 get the latest value and updates the Meter display accordingly.
*******************************************************************/

static void Get_value(XtPointer client_data, XtIntervalId *id)
{
  /* Local variables */
    static XcCallData call_data;
    Widget w = (Widget)client_data;
    MeterWidget wm = (MeterWidget)client_data;

  /* Get the new value by calling the application's callback if it exists. */
    if (wm->meter.update_callback == NULL)
      return;

  /* Re-register this TimeOut procedure for the next interval. */
    if (wm->meter.interval > 0)
      wm->meter.interval_id =
	XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)w),
	  wm->meter.interval, Get_value, client_data);

  /* Set the widget's current value and datatype before calling the callback. */
    call_data.dtype = wm->value.datatype;
    call_data.decimals = wm->value.decimals;
    if ((wm->value.datatype == XcLval) || (wm->value.datatype == XcHval))
      call_data.value.lval = wm->value.val.lval;
    else if (wm->value.datatype == XcFval)
      call_data.value.fval = wm->value.val.fval;
    XtCallCallbacks(w, XcNupdateCallback, &call_data);

  /* Update the new value, update the Meter display. */
    if ((wm->value.datatype == XcLval) || (wm->value.datatype == XcHval))
      wm->value.val.lval = call_data.value.lval;
    else if (wm->value.datatype == XcFval)
      wm->value.val.fval = call_data.value.fval;

    if (XtIsRealized((Widget)w))
      Draw_display(w, XtDisplay(w), XtWindow(w), wm->control.gc);
}

/*******************************************************************
 NAME:		XcMeterUpdateValue.
 DESCRIPTION:
   This convenience function is called by the application in order to
 update the value (a little quicker than using XtSetArg/XtSetValue).
 The application passes the new value to be updated with.

*******************************************************************/

void XcMeterUpdateValue(Widget w, XcVType *value)
{
  /* Local variables */
    MeterWidget wm = (MeterWidget)w;

  /* Update the new value, then update the Meter display. */
    if (value != NULL) {
	if ((wm->value.datatype == XcLval) || (wm->value.datatype == XcHval))
	  wm->value.val.lval = value->lval;
	else if (wm->value.datatype == XcFval)
	  wm->value.val.fval = value->fval;

	if (XtIsRealized(w))
	  Draw_display(w, XtDisplay(w), XtWindow(w), wm->control.gc);
    }
}

/*******************************************************************
 NAME:		XcMeterUpdateMeterForeground.
 DESCRIPTION:
   This convenience function is called by the application in order to
 update the meter foreground (needle) (a little quicker than using
 XtSetArg/XtSetValue).
 The application passes the new value to be updated with.

*******************************************************************/

void XcMeterUpdateMeterForeground(Widget w, Pixel pixel)
{
    MeterWidget wm = (MeterWidget)w;

#ifdef USE_CORE_VISIBLE
    if (!wm->core.visible) return;
#endif

  /* Update the new value, then update the Meter display. */
    if (wm->meter.meter_foreground != pixel) {
	wm->meter.meter_foreground = pixel;
	if (XtIsRealized((Widget)w))
	  Draw_display(w, XtDisplay(w), XtWindow(w), wm->control.gc);
    }
}

/*******************************************************************
 NAME:		Draw_display.
 DESCRIPTION:
   This function redraws the Bar meter and the value string in the
 Value Box.
*******************************************************************/

static void Draw_display(Widget w, Display *display,
  Drawable drawable, GC gc)
{
    MeterWidget wm = (MeterWidget)w;
    char *temp;
    float range, dim = 0.0;
    int shift, diameter, radius, center_x, center_y;
    double angle, sine, cosine, base_multiplier;
    XPoint point, triangle[3];
    XRectangle clipRect[1];

#ifdef USE_CORE_VISIBLE
    if (!wm->core.visible) return;
#endif
  /* Clear meter */
  /* Fill the interior of the Meter with its background color */
    XSetForeground(XtDisplay(w), wm->control.gc, wm->meter.meter_background);
    shift = wm->meter.outer_radius - wm->meter.inner_radius + 1;
  /* fill as whole circle */
    diameter = wm->meter.meter.width - 2*(wm->control.shade_depth + shift);
    radius = wm->meter.meter.height - 2*(wm->control.shade_depth + shift);
    clipRect[0].x = wm->meter.meter.x + wm->control.shade_depth;
    clipRect[0].y = wm->meter.meter.y + wm->control.shade_depth;
    clipRect[0].width = wm->meter.meter.width - 2*wm->control.shade_depth;
    clipRect[0].height = wm->meter.meter.height - 3*wm->control.shade_depth;
    XSetClipRectangles(display, gc, 0, 0, clipRect,1,Unsorted);
    XFillArc(XtDisplay(w),XtWindow(w),wm->control.gc,
      wm->meter.meter_center.x - diameter/2 + wm->control.shade_depth,
      wm->meter.meter_center.y - radius - shift,
      (unsigned int)diameter - wm->control.shade_depth,
      (unsigned int)diameter,
      0, 180*64);

  /* Draw the Meter  in its foreground color according to the value. */
    range = MAX_ANGLE - MIN_ANGLE;
    if ((wm->value.datatype == XcLval) || (wm->value.datatype == XcHval))
      dim = Correlate(((float)(wm->value.val.lval)
	- (float)(wm->value.lower_bound.lval)),
	((float)(wm->value.upper_bound.lval) -
	  (float)(wm->value.lower_bound.lval)), range);
    else if (wm->value.datatype == XcFval)
      dim = Correlate((wm->value.val.fval - wm->value.lower_bound.fval),
	(wm->value.upper_bound.fval -
	  wm->value.lower_bound.fval), range);

  /* Draw meter's "needle" */
    XSetForeground(display, gc, wm->meter.meter_foreground);
    angle = RADIANS(MAX_ANGLE - dim);
    cosine = cos(angle);
    sine = sin(angle);
    center_x = wm->meter.meter_center.x;
    center_y = wm->meter.meter_center.y - wm->control.shade_depth;
    point.x = (short)(center_x + (wm->meter.inner_radius-4)*cosine);
    point.y = (short)(center_y - (wm->meter.inner_radius-4)*sine);
    base_multiplier = (short)MAX(3,wm->meter.inner_radius/15);
    triangle[0].x = point.x;
    triangle[0].y = point.y;
    triangle[1].x = center_x - (int)(base_multiplier*sine);
    triangle[1].y = center_y - 2 - (int)(base_multiplier*cosine);
    triangle[2].x = center_x + (int)(base_multiplier*sine);
    triangle[2].y = center_y - 2 + (int)(base_multiplier*cosine);
    XFillPolygon(display,drawable,gc,triangle,3,Convex,CoordModeOrigin);

  /* Reset the clip_mask to no clipping. */
    XSetClipMask(display, gc, None);

  /* If the value string is supposed to be displayed, draw it. */
    if (wm->meter.value_visible == True) {
      /* Clear the Value Box by re-filling it with its background color. */
	XSetForeground(display, gc, wm->value.value_bg_pixel);
	XFillRectangle(display, drawable, gc,
	  wm->value.value_box.x, wm->value.value_box.y,
	  wm->value.value_box.width, wm->value.value_box.height);

      /*
       * Now draw the value string in its foreground color, clipped by the
       * Value Box.
       */
	XSetForeground(display, gc, wm->value.value_fg_pixel);
	XSetClipRectangles(display, gc, 0, 0,
	  &(wm->value.value_box), 1, Unsorted);

	temp = Print_value(wm->value.datatype, &wm->value.val, wm->value.decimals);

	Position_val(w);

	XDrawString(display, drawable, gc,
	  wm->value.vp.x, wm->value.vp.y, temp, strlen(temp));
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
    MeterWidget wm = (MeterWidget)w;

    if (wm->value.datatype == XcLval) {
	cvtLongToString(wm->value.upper_bound.lval,upper);
	cvtLongToString(wm->value.lower_bound.lval,lower);
    } else if (wm->value.datatype == XcHval) {
	cvtLongToHexString(wm->value.upper_bound.lval,upper);
	cvtLongToHexString(wm->value.lower_bound.lval,lower);
    } else if (wm->value.datatype == XcFval) {
	cvtFloatToString(wm->value.upper_bound.fval,upper,
	  (unsigned short)wm->value.decimals);
	cvtFloatToString(wm->value.lower_bound.fval,lower,
	  (unsigned short)wm->value.decimals);
    }
}
