/*******************************************************************
 FILE:		Meter.c
 CONTENTS:	Definitions for structures, methods, and actions of the 
		Meter widget.
 AUTHOR:	Mark Anderson, derived from Paul D. Johnston's BarGraf widget
********************************************************************/

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


/* Declare widget methods */
static void ClassInitialize();
static void Initialize(MeterWidget request, MeterWidget new);
static void Redisplay(MeterWidget w, XExposeEvent *event, Region region);
static void Destroy(MeterWidget w);
static void Resize(MeterWidget w);
static XtGeometryResult QueryGeometry(
  MeterWidget w,
  XtWidgetGeometry *proposed, XtWidgetGeometry *answer);
static Boolean SetValues(
  MeterWidget cur,
  MeterWidget req,
  MeterWidget new);


/* Declare functions and variables private to this widget */
static void Draw_display(
  MeterWidget w,
  Display *display,
  Drawable drawable,
  GC gc);
static void Get_value(XtPointer client_data,XtIntervalId *id);
static void Print_bounds(MeterWidget w, char *upper, char *lower);



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
	(WidgetClass) &valueClassRec,		/* superclass */
	"Meter",				/* class_name */
	sizeof(MeterRec),			/* widget_size */
	ClassInitialize,				/* class_initialize */
	NULL,					/* class_part_initialize */
	FALSE,					/* class_inited */
	Initialize,					/* initialize */
	NULL,					/* initialize_hook */
	XtInheritRealize,				/* realize */
	NULL,					/* actions */
	0,						/* num_actions */
	resources,					/* resources */
	XtNumber(resources),			/* num_resources */
	NULLQUARK,					/* xrm_class */
	TRUE,					/* compress_motion */
	XtExposeCompressMaximal,			/* compress_exposure */
	TRUE,					/* compress_enterleave */
	TRUE,					/* visible_interest */
	Destroy,					/* destroy */
	Resize,					/* resize */
	Redisplay,					/* expose */
	SetValues,					/* set_values */
	NULL,					/* set_values_hook */
	XtInheritSetValuesAlmost,			/* set_values_almost */
	NULL,					/* get_values_hook */
	NULL,					/* accept_focus */
	XtVersion,					/* version */
	NULL,					/* callback_private */
	NULL,					/* tm_table */
	QueryGeometry,				/* query_geometry */
	NULL,					/* display_accelerator */
	NULL,					/* extension */
    },
    {
      /* Control class part */
	0,						/* dummy_field */
    },
    {
      /* Value class part */
	0,						/* dummy_field */
    },
    {
      /* Meter class part */
	0,						/* dummy_field */
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

static void ClassInitialize()
{

    XtAddConverter(XtRString, XcROrient, CvtStringToOrient, NULL, 0);

}  /* end of ClassInitialize */






/*******************************************************************
 NAME:		Initialize.		
 DESCRIPTION:
   This is the initialize method for the Meter widget.  It 
validates user-modifiable instance resources and initializes private 
widget variables and structures.  This function also creates any server 
resources (i.e., GCs, fonts, Pixmaps, etc.) used by this widget.  This
method is called by Xt when the application calls XtCreateWidget().

*******************************************************************/

static void Initialize(MeterWidget request, MeterWidget new)
{

    DPRINTF(("Meter: executing Initialize...\n"));

  /*
   * Validate public instance variable settings.
   */
  /* Check the interval resource setting. */
    if (new->meter.interval >0)
	{
	    new->meter.interval_id = 
	      XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)new),
		new->meter.interval, Get_value, new);
	}

  /* Check the scaleSegments resource setting. */
    if (new->meter.num_segments < MIN_SCALE_SEGS)
	{
	    XtWarning("Meter: invalid number of scale segments");
	    new->meter.num_segments = MIN_SCALE_SEGS;
	}
    else if (new->meter.num_segments > MAX_SCALE_SEGS)
	{
	    XtWarning("Meter: invalid number of scale segments");
	    new->meter.num_segments = MAX_SCALE_SEGS;
	}
   
  /* Check the valueVisible resource setting. */
    if ((new->meter.value_visible != True) &&
      (new->meter.value_visible != False))
	{
	    XtWarning("Meter: invalid valueVisible setting");
	    new->meter.value_visible = True;
	}

  /* Initialize the Meter width and height. */
    if (new->core.width < MIN_METER_WIDTH)
      new->core.width = MIN_METER_WIDTH; 
    if (new->core.height < MIN_METER_HEIGHT)
      new->core.height = MIN_METER_HEIGHT; 
   
  /* Initialize private instance variables.  */

  /* Set the initial geometry of the Meter elements. */
    Resize(new);


    DPRINTF(("Meter: done Initialize\n"));

}  /* end of Initialize */





/*******************************************************************
 NAME:		Redisplay.	
 DESCRIPTION:
   This function is the Meter's Expose method.  It redraws the
Meter's 3D rectangle background, Value Box, label, Bar meter,
and the Scale.  All drawing takes place within the widget's window 
(no need for an off-screen pixmap).

*******************************************************************/

static void Redisplay(MeterWidget w, XExposeEvent *event, Region region)
{
  /* Local variables */
    char upper[30], lower[30];
    int shift, diameter;

  /*
 * Check to see whether or not the widget's window is mapped.  You can't
 * draw into a window that is not mapped.  Realizing a widget doesn't 
 * mean its mapped, but this call will work for most Window Managers.
 */
    if (!XtIsRealized((Widget)w) || !w->core.visible)
      return;

    DPRINTF(("Meter: executing Redisplay\n"));

  /* Draw the 3D rectangle background for the Meter. */
    XSetClipMask(XtDisplay(w), w->control.gc, None);
    Rect3d(w, XtDisplay(w), XtWindow(w), w->control.gc,
      0, 0, w->core.width, w->core.height, RAISED);

  /* Draw the Label string. */
    XSetClipRectangles(XtDisplay(w), w->control.gc, 0, 0, 
      &(w->meter.face), 1, Unsorted); 
    XSetForeground(XtDisplay(w), w->control.gc,
      w->control.label_pixel);
    XDrawString(XtDisplay(w), XtWindow(w), w->control.gc,
      w->meter.lbl.x, w->meter.lbl.y, 
      w->control.label, strlen(w->control.label));
   
  /* Draw the inner rectangle which houses the meter */
#if 0    
    Rect3d(w, XtDisplay(w), XtWindow(w), w->control.gc,
      w->meter.meter_center.x - w->meter.meter.width/2,
      w->meter.meter_center.y - w->meter.meter.height,
      w->meter.meter.width, w->meter.meter.height,
      DEPRESSED);
#else
  /* Adjust for changes to Rect3d */
    Rect3d(w, XtDisplay(w), XtWindow(w), w->control.gc,
      w->meter.meter_center.x - w->meter.meter.width/2,
      w->meter.meter_center.y - w->meter.meter.height,
      w->meter.meter.width+w->control.shade_depth,
      w->meter.meter.height+w->control.shade_depth,
      DEPRESSED);
#endif    
   
  /* Now draw the Scale segments */
    XSetForeground(XtDisplay(w), w->control.gc, w->meter.scale_pixel);
    XDrawSegments(XtDisplay(w), XtWindow(w), w->control.gc,
      w->meter.segs,w->meter.num_segments);
   
  /* Draw the max and min value strings */
    Print_bounds(w, upper, lower);
    XDrawString(XtDisplay(w), XtWindow(w), w->control.gc,
      w->meter.max_val.x, w->meter.max_val.y,upper, strlen(upper)); 
    XDrawString(XtDisplay(w), XtWindow(w), w->control.gc,
      w->meter.min_val.x, w->meter.min_val.y, lower, strlen(lower));
 
   
  /* Draw the Value Box */
    if (w->meter.value_visible == True)
      Rect3d(w, XtDisplay(w), XtWindow(w), w->control.gc,
	w->value.value_box.x - w->control.shade_depth, 
	w->value.value_box.y - w->control.shade_depth,
	w->value.value_box.width + (2 * w->control.shade_depth),  
	w->value.value_box.height + (2 * w->control.shade_depth), 
	DEPRESSED);

  /* Draw the new value represented by the Bar meter and the value string */
    Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);


    DPRINTF(("Meter: done Redisplay\n"));

}  /* end of Redisplay */





/*******************************************************************
 NAME:		SetValues.
 DESCRIPTION:
   This is the set_values method for this widget. It validates resource
settings set with XtSetValues. If a resource is changed that would
require re-drawing the widget, return True.

*******************************************************************/

static Boolean SetValues(
  MeterWidget cur, MeterWidget req, MeterWidget new)
{
  /* Local variables */
    Boolean do_redisplay = False, do_resize = False;


    DPRINTF(("Meter: executing SetValues \n"));

  /* Validate new resource settings. */

/* Check widget color resource settings. */
    if ((new->meter.meter_foreground != cur->meter.meter_foreground) ||
      (new->meter.meter_background != cur->meter.meter_background) ||
      (new->meter.scale_pixel != cur->meter.scale_pixel)) 
      do_redisplay = True;

/* Check the interval resource setting. */
    if (new->meter.interval != cur->meter.interval) 
	{
	    if (cur->meter.interval > 0)
	      XtRemoveTimeOut (cur->meter.interval_id);
	    if (new->meter.interval > 0)
	      new->meter.interval_id = 
		XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)new),
		  new->meter.interval, Get_value, new);
	}

  /* Check the scaleSegments resource setting. */
    if (new->meter.num_segments != cur->meter.num_segments)
	{
	    if (new->meter.num_segments < MIN_SCALE_SEGS)
		{
		    XtWarning("Meter: invalid number of scale segments");
		    new->meter.num_segments = MIN_SCALE_SEGS;
		}
	    else if (new->meter.num_segments > MAX_SCALE_SEGS)
		{
		    XtWarning("Meter: invalid number of scale segments");
		    new->meter.num_segments = MAX_SCALE_SEGS;
		}
	}

  /* Check the valueVisible resource setting. */
    if (new->meter.value_visible != cur->meter.value_visible)
	{
	    do_redisplay = True;
	    if ((new->meter.value_visible != True) &&
	      (new->meter.value_visible != False))
		{
		    XtWarning("Meter: invalid valueVisible setting");
		    new->meter.value_visible = True;
		}
	}

  /* Check to see if the value has changed. */
    if ((((new->value.datatype == XcLval) || (new->value.datatype == XcHval)) && 
      (new->value.val.lval != cur->value.val.lval))
      || ((new->value.datatype == XcFval) && 
	(new->value.val.fval != cur->value.val.fval)))
	{
	    do_redisplay = True;
	}

  /* (MDA) want to force resizing if min/max changed  or decimals setting */
    if (new->value.decimals != cur->value.decimals) do_resize = True;

    if ( ((new->value.datatype == XcLval) || (new->value.datatype == XcHval)) &&
      ( (new->value.lower_bound.lval != cur->value.lower_bound.lval) ||
	(new->value.upper_bound.lval != cur->value.upper_bound.lval) ))
	{
	    do_resize = True;
	}
    else if ( (new->value.datatype == XcFval) &&
      ( (new->value.lower_bound.fval != cur->value.lower_bound.fval) ||
	(new->value.upper_bound.fval != cur->value.upper_bound.fval) ))
	{
	    do_resize = True;
	}
    if (do_resize) {
	Resize(new);
        do_redisplay = True;
    }


    DPRINTF(("Meter: done SetValues\n"));
    return do_redisplay;


}  /* end of SetValues */




/*******************************************************************
 NAME:		Resize.
 DESCRIPTION:
   This is the resize method of the Meter widget. It resizes the
Meter's graphics based on the new width and height of the widget's
window.  

*******************************************************************/

static void Resize(MeterWidget w)
{
  /* Local variables */
    int j;
    int max_val_width, min_val_width, max_width;
    int font_center, font_height, marker_width, center_x, center_y;
    char upper[30], lower[30];
    double angle, cosine, sine;

    DPRINTF(("Meter: executing Resize\n"));

  /* (MDA) for numbers, usually safe to ignore descent to save space */
    font_height = (w->control.font)->ascent;
   
  /* Set the widgets new width and height. */
    w->meter.face.x = w->meter.face.y = w->control.shade_depth;
    w->meter.face.width = w->core.width - (2*w->control.shade_depth);
    w->meter.face.height = w->core.height - (2*w->control.shade_depth);

  /* Calculate min/max string attributes */
    Print_bounds(w, upper, lower);
    max_val_width = XTextWidth(w->control.font, upper, strlen(upper));
    min_val_width = XTextWidth(w->control.font, lower, strlen(lower));
    max_width = MAX(min_val_width,max_val_width) + 2*w->control.shade_depth;

/* Establish the new Value Box geometry. */
    if (w->meter.value_visible == True)
	{
	    w->value.value_box.x = w->core.width/2 - max_width/2;
	    w->value.value_box.y = w->core.height - font_height -
	      2*w->control.shade_depth - 2;
	    w->value.value_box.width = max_width;
	    w->value.value_box.height = font_height;
	  /* Set the position of the displayed value within the Value Box. */
	    Position_val((ValueWidget)w);
	}
    else
	{
	    w->value.value_box.x = 0;
	    w->value.value_box.y = w->core.height;
	    w->value.value_box.width = 0;
	    w->value.value_box.height = 0;         
	}



  /* Set the new label location. */
    if (strlen(w->control.label) > 1 ||
      (strlen(w->control.label) == 1 && w->control.label[0] != ' '))
	{
	    w->meter.lbl.x = (w->core.width / 2) -
	      (XTextWidth(w->control.font, w->control.label, 
		strlen(w->control.label)) / 2); 
	    w->meter.lbl.y = w->meter.face.y + w->control.font->ascent;
	}
    else
	{
	    w->meter.lbl.x = w->meter.lbl.y = w->control.shade_depth;
	}


  /* Resize the Meter */
    w->meter.meter.width = w->meter.face.width - (2*w->control.shade_depth);
    w->meter.meter.height =  MAX(0,(w->value.value_box.y - font_height -
      w->meter.lbl.y));
  /* now make rectangular (w=2*h)*/
    w->meter.meter.height = MIN((unsigned short)(w->meter.meter.width/2),
      w->meter.meter.height);
    w->meter.meter.width = MIN(w->meter.meter.width,
      (unsigned short)(2*w->meter.meter.height));
    w->meter.meter.x = w->core.width/2 - w->meter.meter.width/2;
    w->meter.meter.y = w->meter.lbl.y + (w->control.font)->descent + 2;
    w->meter.meter_center.y = w->meter.meter.y + w->meter.meter.height 
      - w->control.shade_depth;
    w->meter.meter_center.x = w->core.width/2;
    marker_width = MAX(3,(int)w->meter.meter.width/15);
    w->meter.inner_radius = w->meter.meter.height - 2*w->control.shade_depth
      - marker_width;
    w->meter.outer_radius = w->meter.inner_radius + marker_width;

  /* Now, resize line segments */
    if (w->meter.num_segments > 0)
	{
	    center_x = w->meter.meter_center.x;
	    center_y = w->meter.meter_center.y - w->control.shade_depth;
	    for (j = 0; j < w->meter.num_segments; j++)
		{
		  /* note:  we are going counter-clockwise for min to max hence subtract */
		    angle = RADIANS(MAX_ANGLE - 
		      ((float)j/(float)(w->meter.num_segments-1))
		      *(MAX_ANGLE - MIN_ANGLE));
		    cosine = cos(angle);
		    sine = sin(angle);
		    w->meter.segs[j].x1 = center_x + w->meter.outer_radius*cosine;
		    w->meter.segs[j].y1 = center_y - w->meter.outer_radius*sine;
		    w->meter.segs[j].x2 = center_x + w->meter.inner_radius*cosine; 
		    w->meter.segs[j].y2 = center_y - w->meter.inner_radius*sine; 
		}
	}

  /* Set the position of the max and min value strings */
    w->meter.max_val.x = MIN((short)(w->core.width - w->control.shade_depth
      - max_val_width),
      w->meter.segs[w->meter.num_segments-1].x1);
    w->meter.min_val.x = MAX(w->control.shade_depth,w->meter.segs[0].x1
      - min_val_width);
    w->meter.max_val.y = w->meter.min_val.y =
      w->meter.meter.y + w->meter.meter.height + font_height + 1;

    DPRINTF(("Meter: done Resize\n"));

}  /* end of Resize */




/*******************************************************************
 NAME:		QueryGeometry.		
 DESCRIPTION:
   This function is the widget's query_geometry method.  It simply
checks the proposed size and returns the appropriate value based on
the proposed size.  If the proposed size is greater than the maximum
appropriate size for this widget, QueryGeometry returns the recommended
size.

*******************************************************************/

static XtGeometryResult QueryGeometry(
  MeterWidget w,
  XtWidgetGeometry *proposed, XtWidgetGeometry *answer)
{
  /* Set the request mode mask for the returned answer. */
    answer->request_mode = CWWidth | CWHeight;

/* Set the recommended size. */
    answer->width = (w->core.width > MAX_METER_WIDTH)
      ? MAX_METER_WIDTH : w->core.width;
    answer->height = (w->core.height > MAX_METER_HEIGHT)
      ? MAX_METER_HEIGHT : w->core.height;

/* 
 * Check the proposed dimensions. If the proposed size is larger than
 * appropriate, return the recommended size.
 */
    if (((proposed->request_mode & (CWWidth | CWHeight)) == (CWWidth | CWHeight))
      && proposed->width == answer->width 
      && proposed->height == answer->height)
      return XtGeometryYes;
    else if (answer->width == w->core.width && answer->height == w->core.height)
      return XtGeometryNo;
    else
      return XtGeometryAlmost;

}  /* end of QueryGeometry */




/*******************************************************************
 NAME:		Destroy.
 DESCRIPTION:
   This function is the widget's destroy method.  It simply releases
any server resources acquired during the life of the widget.

*******************************************************************/

static void Destroy(MeterWidget w)
{

    if (w->meter.interval > 0)
      XtRemoveTimeOut (w->meter.interval_id);

}  /* end of Destroy */





/* Widget action functions. */

/*******************************************************************
 NAME:		Get_value.		
 DESCRIPTION:
   This function is the time out procedure called at XcNinterval 
intervals.  It calls the application registered callback function to
get the latest value and updates the Meter display accordingly.

*******************************************************************/

static void Get_value(
  XtPointer client_data,
  XtIntervalId *id)		/* unused */
{
  /* Local variables */
    static XcCallData call_data;
    MeterWidget w = (MeterWidget)client_data;
   
  /* Get the new value by calling the application's callback if it exists. */
    if (w->meter.update_callback == NULL)
      return;

/* Re-register this TimeOut procedure for the next interval. */
    if (w->meter.interval > 0)
      w->meter.interval_id = 
	XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)w),
	  w->meter.interval, Get_value, client_data);

  /* Set the widget's current value and datatype before calling the callback. */
    call_data.dtype = w->value.datatype;
    call_data.decimals = w->value.decimals;
    if ((w->value.datatype == XcLval) || (w->value.datatype == XcHval))  
      call_data.value.lval = w->value.val.lval;
    else if (w->value.datatype == XcFval)
      call_data.value.fval = w->value.val.fval;
    XtCallCallbacks((Widget)w, XcNupdateCallback, &call_data);


  /* Update the new value, update the Meter display. */
    if ((w->value.datatype == XcLval) || (w->value.datatype == XcHval))  
      w->value.val.lval = call_data.value.lval;
    else if (w->value.datatype == XcFval)
      w->value.val.fval = call_data.value.fval;

    if (XtIsRealized((Widget)w))
      Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);
   

}  /* end of Get_value */




/*******************************************************************
 NAME:		XcMeterUpdateValue.		
 DESCRIPTION:
   This convenience function is called by the application in order to
update the value (a little quicker than using XtSetArg/XtSetValue).
The application passes the new value to be updated with.

*******************************************************************/

void XcMeterUpdateValue(Widget widget, XcVType *value)
{
  /* Local variables */
    MeterWidget w = (MeterWidget) widget;
   
/* Update the new value, then update the Meter display. */
    if (value != NULL)
	{
	    if ((w->value.datatype == XcLval) || (w->value.datatype == XcHval))  
	      w->value.val.lval = value->lval;
	    else if (w->value.datatype == XcFval)
	      w->value.val.fval = value->fval;

	    if (XtIsRealized((Widget)w))
	      Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);

	}
   
}  /* end of XcMeterUpdateValue */



/*******************************************************************
 NAME:		XcMeterUpdateMeterForeground.		
 DESCRIPTION:
   This convenience function is called by the application in order to
update the meter foreground (needle) (a little quicker than using
XtSetArg/XtSetValue).
The application passes the new value to be updated with.

*******************************************************************/

void XcMeterUpdateMeterForeground(Widget widget, unsigned long pixel)
{
  /* Local variables */
    MeterWidget w = (MeterWidget) widget;

    if (!w->core.visible) return;
   
  /* Update the new value, then update the Meter display. */
    if (w->meter.meter_foreground != pixel) {
	w->meter.meter_foreground = pixel;
	if (XtIsRealized((Widget)w))
	  Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);
    }
   
}  /* end of XcMeterUpdateMeterForeground */


/*******************************************************************
 NAME:		Draw_display.		
 DESCRIPTION:
   This function redraws the Bar meter and the value string in the 
Value Box.

*******************************************************************/

static void Draw_display(
  MeterWidget w,
  Display *display,
  Drawable drawable,
  GC gc)
{
  /* Local variables */
    char *temp;
    float range, dim;
    int shift, diameter, radius, center_x, center_y;
    unsigned int lineWidth, meter_size;
    double angle, sine, cosine, base_multiplier;
    XPoint point, triangle[3];
    XRectangle clipRect[1];

    if (!w->core.visible) return;
  /* Clear meter */
  /* Fill the interior of the Meter with its background color */
    XSetForeground(XtDisplay(w), w->control.gc, w->meter.meter_background); 
    shift = w->meter.outer_radius - w->meter.inner_radius + 1;
  /* fill as whole circle */
    diameter = w->meter.meter.width - 2*(w->control.shade_depth + shift);
    radius = w->meter.meter.height - 2*(w->control.shade_depth + shift);
    clipRect[0].x = w->meter.meter.x + w->control.shade_depth;
    clipRect[0].y = w->meter.meter.y + w->control.shade_depth;
    clipRect[0].width = w->meter.meter.width - 2*w->control.shade_depth;
    clipRect[0].height = w->meter.meter.height - 3*w->control.shade_depth;
    XSetClipRectangles(display, gc, 0, 0, clipRect,1,Unsorted);
    XFillArc(XtDisplay(w),XtWindow(w),w->control.gc,
      w->meter.meter_center.x - diameter/2 + w->control.shade_depth,
      w->meter.meter_center.y - radius - shift,
      (unsigned int)diameter - w->control.shade_depth,
      (unsigned int)diameter,
      0, 180*64);

  /* Draw the Meter  in its foreground color according to the value. */
    range = MAX_ANGLE - MIN_ANGLE;
    if ((w->value.datatype == XcLval) || (w->value.datatype == XcHval))
      dim = Correlate(((float)(w->value.val.lval)
	- (float)(w->value.lower_bound.lval)),
	((float)(w->value.upper_bound.lval) -
	  (float)(w->value.lower_bound.lval)), range);
    else if (w->value.datatype == XcFval)
      dim = Correlate((w->value.val.fval - w->value.lower_bound.fval),
	(w->value.upper_bound.fval -
	  w->value.lower_bound.fval), range);

  /* Draw meter's "needle" */
    XSetForeground(display, gc, w->meter.meter_foreground);
    angle = RADIANS(MAX_ANGLE - dim);
    cosine = cos(angle);
    sine = sin(angle);
    center_x = w->meter.meter_center.x;
    center_y = w->meter.meter_center.y - w->control.shade_depth;
    point.x = center_x + (w->meter.inner_radius-4)*cosine;
    point.y = center_y - (w->meter.inner_radius-4)*sine;
    base_multiplier = (double) MAX(3,w->meter.inner_radius/15);
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
    if (w->meter.value_visible == True)
	{
	  /* Clear the Value Box by re-filling it with its background color. */
	    XSetForeground(display, gc, w->value.value_bg_pixel); 
	    XFillRectangle(display, drawable, gc,
	      w->value.value_box.x, w->value.value_box.y, 
	      w->value.value_box.width, w->value.value_box.height); 

	  /*
    * Now draw the value string in its foreground color, clipped by the
    * Value Box.
    */
	    XSetForeground(display, gc, w->value.value_fg_pixel); 
	    XSetClipRectangles(display, gc, 0, 0, 
	      &(w->value.value_box), 1, Unsorted); 

	    temp = Print_value(w->value.datatype, &w->value.val, w->value.decimals);

	    Position_val((ValueWidget)w);

	    XDrawString(display, drawable, gc,
	      w->value.vp.x, w->value.vp.y, temp, strlen(temp)); 
	}

  /* Reset the clip_mask to no clipping. */
    XSetClipMask(display, gc, None);

}  /* end of Draw_display */




/*******************************************************************
 NAME:		Print_bounds.		
 DESCRIPTION:
   This is a utility function used by the Redisplay and Resize methods to
print the upper and lower bound values as strings for displaying and resizing
purposes.

*******************************************************************/

static void Print_bounds(
  MeterWidget w,
  char *upper, char *lower)
{

    if (w->value.datatype == XcLval)  
	{
	    cvtLongToString(w->value.upper_bound.lval,upper);
	    cvtLongToString(w->value.lower_bound.lval,lower);
	} 
    else if (w->value.datatype == XcHval)  
	{
	    cvtLongToHexString(w->value.upper_bound.lval,upper);
	    cvtLongToHexString(w->value.lower_bound.lval,lower);
	} 
    else if (w->value.datatype == XcFval)  
	{
	    cvtFloatToString(w->value.upper_bound.fval,upper,
	      (unsigned short)w->value.decimals);
	    cvtFloatToString(w->value.lower_bound.fval,lower,
	      (unsigned short)w->value.decimals);
	} 

}  /* end of Print_bounds */


/* end of Meter.c */
