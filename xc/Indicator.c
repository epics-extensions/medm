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


/* Declare widget methods */
static void ClassInitialize();
static void Initialize(IndicatorWidget request, IndicatorWidget new);
static void Redisplay(IndicatorWidget w, XExposeEvent *event, Region region);
static void Destroy(IndicatorWidget w);
static void Resize(IndicatorWidget w);
static XtGeometryResult QueryGeometry(
  IndicatorWidget w,
  XtWidgetGeometry *proposed, XtWidgetGeometry *answer);
static Boolean SetValues(
  IndicatorWidget cur,
  IndicatorWidget req,
  IndicatorWidget new);



/* Declare functions and variables private to this widget */
static void Draw_display(
  IndicatorWidget w,
  Display *display,
  Drawable drawable,
  GC gc);
static void Get_value(XtPointer client_data,XtIntervalId *id);
static void Print_bounds(IndicatorWidget w, char *upper, char *lower);




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
    (WidgetClass) &valueClassRec,		/* superclass */
    "Indicator",				/* class_name */
    sizeof(IndicatorRec),			/* widget_size */
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
  /* Indicator class part */
    0,						/* dummy_field */
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

static void ClassInitialize()
{

   XtAddConverter(XtRString, XcROrient, CvtStringToOrient, NULL, 0);

}  /* end of ClassInitialize */






/*******************************************************************
 NAME:		Initialize.		
 DESCRIPTION:
   This is the initialize method for the Indicator widget.  It 
validates user-modifiable instance resources and initializes private 
widget variables and structures.  This function also creates any server 
resources (i.e., GCs, fonts, Pixmaps, etc.) used by this widget.  This
method is called by Xt when the application calls XtCreateWidget().

*******************************************************************/

static void Initialize(
  IndicatorWidget request, IndicatorWidget new)
{
/* Local variables */
Display *display = XtDisplay(new);


DPRINTF(("Indicator: executing Initialize...\n"));

/*
 * Validate public instance variable settings.
 */
/* Check orientation resource setting. */
   if ((new->indicator.orient != XcVert) && (new->indicator.orient != XcHoriz))
   {
      XtWarning("Indicator: invalid orientation setting");
      new->indicator.orient = XcVert;
   }

/* Check the interval resource setting. */
   if (new->indicator.interval >0)
   {
      new->indicator.interval_id = 
		XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)new),
				new->indicator.interval, Get_value, new);
   }

/* Check the scaleSegments resource setting. */
   if (new->indicator.num_segments < MIN_SCALE_SEGS)
   {
      XtWarning("Indicator: invalid number of scale segments");
      new->indicator.num_segments = MIN_SCALE_SEGS;
   }
   else if (new->indicator.num_segments > MAX_SCALE_SEGS)
   {
      XtWarning("Indicator: invalid number of scale segments");
      new->indicator.num_segments = MAX_SCALE_SEGS;
   }
   
/* Check the valueVisible resource setting. */
   if ((new->indicator.value_visible != True) &&
		(new->indicator.value_visible != False))
   {
      XtWarning("Indicator: invalid valueVisible setting");
      new->indicator.value_visible = True;
   }

/* Initialize the Indicator width and height. */
   if (new->core.width < MIN_INDICATOR_WIDTH)
      new->core.width = MIN_INDICATOR_WIDTH; 
   if (new->core.height < MIN_INDICATOR_HEIGHT)
      new->core.height = MIN_INDICATOR_HEIGHT; 
   
/* Initialize private instance variables.  */

/* Set the initial geometry of the Indicator elements. */
   Resize(new);


DPRINTF(("Indicator: done Initialize\n"));

}  /* end of Initialize */





/*******************************************************************
 NAME:		Redisplay.	
 DESCRIPTION:
   This function is the Indicator's Expose method.  It redraws the
Indicator's 3D rectangle background, Value Box, label, Bar indicator,
and the Scale.  All drawing takes place within the widget's window 
(no need for an off-screen pixmap).

*******************************************************************/

static void Redisplay(IndicatorWidget w, XExposeEvent *event, Region region)
{
/* Local variables */
int j;
char upper[30], lower[30];

/*
 * Check to see whether or not the widget's window is mapped.  You can't
 * draw into a window that is not mapped.  Realizing a widget doesn't 
 * mean its mapped, but this call will work for most Window Managers.
 */
   if (!XtIsRealized((Widget)w) || !w->core.visible)
      return;

DPRINTF(("Indicator: executing Redisplay\n"));

/* Draw the 3D rectangle background for the Indicator. */
   XSetClipMask(XtDisplay(w), w->control.gc, None);
   Rect3d(w, XtDisplay(w), XtWindow(w), w->control.gc,
		0, 0, w->core.width, w->core.height, RAISED);

/* Draw the Label string. */
   XSetClipRectangles(XtDisplay(w), w->control.gc, 0, 0, 
  			&(w->indicator.face), 1, Unsorted); 
   XSetForeground(XtDisplay(w), w->control.gc,
                        w->control.label_pixel);
   XDrawString(XtDisplay(w), XtWindow(w), w->control.gc,
	w->indicator.lbl.x, w->indicator.lbl.y, 
	w->control.label, strlen(w->control.label));

/* Draw the Scale */
   if (w->indicator.num_segments > 0) {
      XSetForeground(XtDisplay(w), w->control.gc,
                        w->indicator.scale_pixel);
      XDrawLine(XtDisplay(w), XtWindow(w), w->control.gc,
	w->indicator.scale_line.x1, w->indicator.scale_line.y1,
	w->indicator.scale_line.x2, w->indicator.scale_line.y2);

/* Draw the max and min value segments. */
      if (w->indicator.orient == XcVert) {
      } else {
	XDrawLine(XtDisplay(w), XtWindow(w), w->control.gc,
		w->indicator.scale_line.x1, 
		w->indicator.scale_line.y1 - w->indicator.seg_length, 
		w->indicator.scale_line.x1, w->indicator.scale_line.y1);
	XDrawLine(XtDisplay(w), XtWindow(w), w->control.gc,
		w->indicator.scale_line.x2, 
		w->indicator.scale_line.y2 - w->indicator.seg_length, 
		w->indicator.scale_line.x2, w->indicator.scale_line.y2);
      }

/* Now draw the rest of the Scale segments. */
      for (j = 0; j < w->indicator.num_segments; j++) {
         if (w->indicator.orient == XcVert)
	    XDrawLine(XtDisplay(w), XtWindow(w), w->control.gc,
		w->indicator.segs[j].x, w->indicator.segs[j].y,
		w->indicator.scale_line.x1, w->indicator.segs[j].y);
         else
	    XDrawLine(XtDisplay(w), XtWindow(w), w->control.gc,
		w->indicator.segs[j].x, w->indicator.segs[j].y,
		w->indicator.segs[j].x, w->indicator.scale_line.y1);
      }

/* Draw the max and min value string indicators */
     Print_bounds(w, upper, lower);
     XDrawString(XtDisplay(w), XtWindow(w), w->control.gc,
    	w->indicator.max_val.x, w->indicator.max_val.y,
						upper, strlen(upper)); 
     XDrawString(XtDisplay(w), XtWindow(w), w->control.gc,
    	w->indicator.min_val.x, w->indicator.min_val.y, 
						lower, strlen(lower)); 
   }


/* Draw the Bar indicator border */
   Rect3d(w, XtDisplay(w), XtWindow(w), w->control.gc,
	w->indicator.indicator.x - w->control.shade_depth, 
	w->indicator.indicator.y - w->control.shade_depth,
	w->indicator.indicator.width + (2*w->control.shade_depth),  
	w->indicator.indicator.height + (2*w->control.shade_depth), DEPRESSED);

/* Draw the Value Box */
   if (w->indicator.value_visible == True)
      Rect3d(w, XtDisplay(w), XtWindow(w), w->control.gc,
	w->value.value_box.x - w->control.shade_depth, 
	w->value.value_box.y - w->control.shade_depth,
	w->value.value_box.width + (2*w->control.shade_depth),  
	w->value.value_box.height + (2*w->control.shade_depth), DEPRESSED);

/* Draw the new value represented by the Bar indicator and the value string */
   Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);


DPRINTF(("Indicator: done Redisplay\n"));

}  /* end of Redisplay */





/*******************************************************************
 NAME:		SetValues.
 DESCRIPTION:
   This is the set_values method for this widget. It validates resource
settings set with XtSetValues. If a resource is changed that would
require re-drawing the widget, return True.

*******************************************************************/

static Boolean SetValues(
IndicatorWidget cur, IndicatorWidget req, IndicatorWidget new)
{
/* Local variables */
Boolean do_redisplay = False, do_resize = True;


DPRINTF(("Indicator: executing SetValues \n"));

/* Validate new resource settings. */

/* Check widget color resource settings. */
   if ((new->indicator.indicator_foreground 
	!= cur->indicator.indicator_foreground) ||
	(new->indicator.indicator_background 
	!= cur->indicator.indicator_background) ||
	(new->indicator.scale_pixel != cur->indicator.scale_pixel)) 
      do_redisplay = True;

/* Check orientation resource setting. */
   if (new->indicator.orient != cur->indicator.orient)
   {
      do_redisplay = True;
      if ((new->indicator.orient != XcVert) 
	&& (new->indicator.orient != XcHoriz))
      {
         XtWarning("Indicator: invalid orientation setting");
         new->indicator.orient = XcVert;
      }
   }

/* Check the interval resource setting. */
   if (new->indicator.interval != cur->indicator.interval) 
   {
      if (cur->indicator.interval > 0)
	    XtRemoveTimeOut (cur->indicator.interval_id);
      if (new->indicator.interval > 0)
         new->indicator.interval_id = 
		XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)new),
					new->indicator.interval, Get_value, new);
   }

/* Check the scaleSegments resource setting. */
   if (new->indicator.num_segments != cur->indicator.num_segments)
   {
      if (new->indicator.num_segments < MIN_SCALE_SEGS)
      {
         XtWarning("Indicator: invalid number of scale segments");
         new->indicator.num_segments = MIN_SCALE_SEGS;
      }
      else if (new->indicator.num_segments > MAX_SCALE_SEGS)
      {
         XtWarning("Indicator: invalid number of scale segments");
         new->indicator.num_segments = MAX_SCALE_SEGS;
      }
   }

/* Check the valueVisible resource setting. */
   if (new->indicator.value_visible != cur->indicator.value_visible)
   {
      do_redisplay = True;
      if ((new->indicator.value_visible != True) &&
		(new->indicator.value_visible != False))
      {
         XtWarning("Indicator: invalid valueVisible setting");
         new->indicator.value_visible = True;
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


DPRINTF(("Indicator: done SetValues\n"));
   return do_redisplay;


}  /* end of SetValues */




/*******************************************************************
 NAME:		Resize.
 DESCRIPTION:
   This is the resize method of the Indicator widget. It resizes the
Indicator's graphics based on the new width and height of the widget's
window.  

*******************************************************************/

static void Resize(IndicatorWidget w)
{
/* Local variables */
int j;
int seg_spacing;
int max_val_width, min_val_width, max_width;
int font_center, font_height;
char upper[30], lower[30];
Boolean displayValue, displayLabel;

DPRINTF(("Indicator: executing Resize\n"));

/* (MDA) for numbers, usually safe to ignore descent to save space */
   font_height = (w->control.font)->ascent;

/* Set the widgets new width and height. */
   w->indicator.face.x = w->indicator.face.y = w->control.shade_depth;
   w->indicator.face.width = w->core.width - (2*w->control.shade_depth);
   w->indicator.face.height = w->core.height - (2*w->control.shade_depth);

/* Calculate min/max string attributes */
   Print_bounds(w, upper, lower);
   max_val_width = XTextWidth(w->control.font, upper, strlen(upper));
   min_val_width = XTextWidth(w->control.font, lower, strlen(lower));
   max_width = MAX(min_val_width,max_val_width) + 2*w->control.shade_depth;
   if (w->indicator.num_segments == 0) max_width = w->indicator.face.width
                - 2*w->control.shade_depth;

   
/* Establish the new Value Box geometry. */
   if (w->indicator.value_visible == True) {
     displayValue = True;
     if (w->indicator.orient == XcVert) {
      w->value.value_box.x = w->core.width/2 - max_width/2;
      w->value.value_box.y = w->core.height - font_height -
				3*w->control.shade_depth;
      w->value.value_box.width = max_width;
      w->value.value_box.height = font_height;
     } else {
      w->value.value_box.x = w->core.width/2 - max_width/2;
      w->value.value_box.y = w->core.height - font_height -
				2*w->control.shade_depth -2;
      w->value.value_box.width = max_width;
      w->value.value_box.height = font_height;
     }
   /* Set the position of the displayed value within the Value Box. */
     Position_val((ValueWidget)w);

   } else {
      displayValue = False;
      w->value.value_box.x = 0;
      w->value.value_box.y = w->core.height - 2*w->control.shade_depth;
      w->value.value_box.width = 0;
      w->value.value_box.height = 0;   
   }


/* Set the new label location. */
   
   if (strlen(w->control.label) > 1 ||
       		(strlen(w->control.label) == 1 && w->control.label[0] != ' '))
   {
	displayLabel = True;
	w->indicator.lbl.x = (w->core.width / 2) -
		(XTextWidth(w->control.font, w->control.label, 
			strlen(w->control.label)) / 2); 
	w->indicator.lbl.y = w->indicator.face.y + w->control.font->ascent + 1;
   }
   else
   {
	displayLabel = False;
	w->indicator.lbl.x = w->indicator.face.x;
	w->indicator.lbl.y = w->indicator.face.y;
   }   

/* Resize the Bar indicator */
   if (w->indicator.orient == XcVert)
   {
      w->indicator.indicator.x = w->indicator.face.x
			+ (w->indicator.num_segments > 0 ? max_width +
				 + (w->indicator.face.width / 32) : 0)
			+ 2*w->control.shade_depth;	 
      w->indicator.indicator.y = w->indicator.lbl.y  + w->control.font->descent
			+ 2*w->control.shade_depth;
      w->indicator.indicator.width = MAX(0,(int)(w->indicator.face.width 
			- w->indicator.indicator.x - w->control.shade_depth));
      w->indicator.indicator.height = w->value.value_box.y 
			- w->indicator.indicator.y
			- (displayValue == True
				? font_height - w->control.font->descent 
				: 2*w->control.shade_depth);
   }
   else
   {
      w->indicator.indicator.x = w->indicator.face.x +
			(short)(w->indicator.face.width / 16) +
				w->control.shade_depth;
      w->indicator.indicator.y =  w->indicator.lbl.y
				+ (w->indicator.num_segments > 0 ?
				font_height/2 + w->indicator.face.height/8 + 1 :
				0)
				+ 2*w->control.shade_depth;
      w->indicator.indicator.width = (short)
	   ((9 * (int)w->indicator.face.width)/10) - 2*w->control.shade_depth;
      w->indicator.indicator.height = MAX(1,
			w->value.value_box.y - w->indicator.indicator.y
			- (displayValue == True ? font_height : 0)
			- w->control.shade_depth);
   }


/* Resize the Scale line. */
   if (w->indicator.orient == XcVert)
   {
      w->indicator.scale_line.x1 = w->indicator.indicator.x 
	- w->control.shade_depth - (w->indicator.face.width / 32);
      w->indicator.scale_line.y1 = w->indicator.indicator.y;
      w->indicator.scale_line.x2 = w->indicator.scale_line.x1;
      w->indicator.scale_line.y2 = w->indicator.indicator.y 
	+ w->indicator.indicator.height;
   }
   else
   {
      w->indicator.scale_line.x1 = w->indicator.indicator.x;
      w->indicator.scale_line.y1 = w->indicator.indicator.y 
	- w->control.shade_depth - (w->indicator.face.height / 32);
      w->indicator.scale_line.x2 = w->indicator.indicator.x 
	+ w->indicator.indicator.width;
      w->indicator.scale_line.y2 = w->indicator.scale_line.y1;
   }

/* Now, resize Scale line segments */
   if (w->indicator.num_segments > 0)
   {
      if (w->indicator.orient == XcVert)
      {
         w->indicator.seg_length = (w->indicator.face.width / 16);
         seg_spacing = ((int)w->indicator.indicator.height / 
				(w->indicator.num_segments + 1));
         for (j = 0; j < w->indicator.num_segments; j++)
         {
	    w->indicator.segs[j].x = w->indicator.scale_line.x1 -
					w->indicator.seg_length;
	    w->indicator.segs[j].y = w->indicator.scale_line.y1 + 
					((j+1) * seg_spacing);
         }
      }
      else
      {
         w->indicator.seg_length = (w->indicator.face.height / 16);
         seg_spacing = ((int)w->indicator.indicator.width / 
				(w->indicator.num_segments + 1));
         for (j = 0; j < w->indicator.num_segments; j++)
         {
	    w->indicator.segs[j].x = w->indicator.scale_line.x1 + 
						((j+1) * seg_spacing);
	    w->indicator.segs[j].y = w->indicator.scale_line.y1 - 
						w->indicator.seg_length;
         }
      }
   }

/* Set the position of the max and min value strings */
   if (w->indicator.orient == XcVert)
   {
      font_center = ((w->control.font->ascent + 
			w->control.font->descent) / 2) 
				- w->control.font->descent;
      w->indicator.max_val.x = MAX(w->control.shade_depth,
			w->indicator.scale_line.x2 - (max_val_width));
      w->indicator.max_val.y = w->indicator.scale_line.y1 + font_center;
      w->indicator.min_val.x = MAX(w->control.shade_depth,
			w->indicator.scale_line.x2 - (min_val_width));
      w->indicator.min_val.y = w->indicator.scale_line.y2 + font_center;
   }
   else
   {
      w->indicator.max_val.x = MIN(
			(int)(w->indicator.face.width + w->control.shade_depth
				  - max_val_width),
			w->indicator.scale_line.x2 - (max_val_width / 2));
      w->indicator.max_val.y = w->indicator.min_val.y =
      		w->indicator.scale_line.y1 - w->indicator.seg_length - 1;
      w->indicator.min_val.x = MAX(w->control.shade_depth,
			w->indicator.scale_line.x1 - (min_val_width / 2));
   }


DPRINTF(("Indicator: done Resize\n"));

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
IndicatorWidget w,
XtWidgetGeometry *proposed, XtWidgetGeometry *answer)
{
/* Set the request mode mask for the returned answer. */
   answer->request_mode = CWWidth | CWHeight;

/* Set the recommended size. */
   answer->width = (w->core.width > MAX_INDICATOR_WIDTH)
	? MAX_INDICATOR_WIDTH : w->core.width;
   answer->height = (w->core.height > MAX_INDICATOR_HEIGHT)
	? MAX_INDICATOR_HEIGHT : w->core.height;

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

static void Destroy(IndicatorWidget w)
{

   if (w->indicator.interval > 0)
      XtRemoveTimeOut (w->indicator.interval_id);

}  /* end of Destroy */





/* Widget action functions. */

/*******************************************************************
 NAME:		Get_value.		
 DESCRIPTION:
   This function is the time out procedure called at XcNinterval 
intervals.  It calls the application registered callback function to
get the latest value and updates the Indicator display accordingly.

*******************************************************************/

static void Get_value(
  XtPointer client_data,
  XtIntervalId *id)		/* unused */
{
/* Local variables */
static XcCallData call_data;
IndicatorWidget w = (IndicatorWidget)client_data;
   
/* Get the new value by calling the application's callback if it exists. */
   if (w->indicator.update_callback == NULL)
       return;

/* Re-register this TimeOut procedure for the next interval. */
   if (w->indicator.interval > 0)
      w->indicator.interval_id = 
	XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)w),
		       w->indicator.interval, Get_value, client_data);

/* Set the widget's current value and datatype before calling the callback. */
   call_data.dtype = w->value.datatype;
   call_data.decimals = w->value.decimals;
   if ((w->value.datatype == XcLval) || (w->value.datatype == XcHval))  
      call_data.value.lval = w->value.val.lval;
   else if (w->value.datatype == XcFval)
      call_data.value.fval = w->value.val.fval;
   XtCallCallbacks((Widget)w, XcNupdateCallback, &call_data);


/* Update the new value, update the Indicator display. */
   if ((w->value.datatype == XcLval) || (w->value.datatype == XcHval))  
	 w->value.val.lval = call_data.value.lval;
   else if (w->value.datatype == XcFval)
	 w->value.val.fval = call_data.value.fval;

   if (XtIsRealized((Widget)w))
      Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);
   

}  /* end of Get_value */




/*******************************************************************
 NAME:		XcIndUpdateValue.		
 DESCRIPTION:
   This convenience function is called by the application in order to
update the value (a little quicker than using XtSetArg/XtSetValue).
The application passes the new value to be updated with.

*******************************************************************/

void XcIndUpdateValue(Widget widget, XcVType *value)
{
/* Local variables */

  IndicatorWidget w = (IndicatorWidget) widget;

    if (!w->core.visible) return;
    
/* Update the new value, then update the Indicator display. */
   if (value != NULL)
   {
      if ((w->value.datatype == XcLval) || (w->value.datatype == XcHval))  
	 w->value.val.lval = value->lval;
      else if (w->value.datatype == XcFval)
	 w->value.val.fval = value->fval;

      if (XtIsRealized((Widget)w))
         Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);

   }
   
}  /* end of XcIndUpdateValue */



/*******************************************************************
 NAME:		XcIndUpdateIndicatorForeground.		
 DESCRIPTION:
   This convenience function is called by the application in order to
   update the indicator foreground color (a little quicker than using
   XtSetArg/XtSetValue).
   The application passes the new value to be updated with.

*******************************************************************/

void XcIndUpdateIndicatorForeground(Widget widget, unsigned long pixel)
{
/* Local variables */
  IndicatorWidget w = (IndicatorWidget) widget;

    if (!w->core.visible) return;
    
/* Update the new value, then update the Indicator display. */
   if (w->indicator.indicator_foreground != pixel) {
      w->indicator.indicator_foreground = pixel;
      if (XtIsRealized((Widget)w))
         Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);
   }
   
}  /* end of XcIndUpdateIndicatorForeground */



/*******************************************************************
 NAME:		Draw_display.		
 DESCRIPTION:
   This function redraws the Bar indicator and the value string in the 
Value Box.

*******************************************************************/

static void Draw_display(
  IndicatorWidget w,
  Display *display,
  Drawable drawable,
  GC gc)
{
/* Local variables */
XRectangle clipRect[1];
char *temp;
float range, dim;
unsigned int indicator_size;
XPoint points[4];
int nPoints = 4;

/* Draw the Bar indicator */
/* Fill the Bar with its background color. */
   XSetForeground(display, gc, w->indicator.indicator_background); 
   XFillRectangle(display, drawable, gc,
  	w->indicator.indicator.x, w->indicator.indicator.y, 
  	w->indicator.indicator.width, w->indicator.indicator.height); 

/* Draw the Bar in its foreground color according to the value. */
   if (w->indicator.orient == XcVert)
      range = (float)(w->indicator.indicator.height);
   else
      range = (float)(w->indicator.indicator.width);

   if ((w->value.datatype == XcLval) || (w->value.datatype == XcHval))
      dim = Correlate(((float)(w->value.val.lval)
				- (float)(w->value.lower_bound.lval)),
			((float)(w->value.upper_bound.lval) -
			 (float)(w->value.lower_bound.lval)), range);
   else if (w->value.datatype == XcFval)
      dim = Correlate((w->value.val.fval
				- w->value.lower_bound.fval),
			(w->value.upper_bound.fval -
		 	 w->value.lower_bound.fval), range);

   if (w->indicator.orient == XcVert) {
      indicator_size = MAX(5,(int)w->indicator.indicator.height/10);
   } else {
      indicator_size = MAX(5,(int)w->indicator.indicator.width/10);
   }

   if ((int)dim < 1)
      dim = 1;
   XSetForeground(display, gc, w->indicator.indicator_foreground); 
   clipRect[0].x = w->indicator.indicator.x;
   clipRect[0].y = w->indicator.indicator.y;
   clipRect[0].width = w->indicator.indicator.width;
   clipRect[0].height = w->indicator.indicator.height;
   XSetClipRectangles(display,gc,0,0,clipRect,1,Unsorted);
   if (w->indicator.orient == XcVert) {
      points[0].x = w->indicator.indicator.x;
      points[0].y = w->indicator.indicator.y
		+ w->indicator.indicator.height - (int)dim;
      points[1].x = points[0].x + w->indicator.indicator.width/2;
      points[1].y = points[0].y - indicator_size/2;

      points[2].x = points[1].x + (points[1].x - points[0].x);
      points[2].y = points[0].y;
      points[3].x = points[1].x;
      points[3].y = points[0].y + (points[0].y - points[1].y);
   } else {
      points[0].x = w->indicator.indicator.x + (int)dim - indicator_size/2 - 1;
      points[0].y = w->indicator.indicator.y + w->indicator.indicator.height/2;
      points[1].x = points[0].x + indicator_size/2;
      points[1].y = points[0].y - w->indicator.indicator.height/2;

      points[2].x = points[1].x + (points[1].x - points[0].x);
      points[2].y = points[0].y;
      points[3].x = points[1].x;
      points[3].y = points[0].y + (points[0].y - points[1].y);
   }

   XFillPolygon(display,drawable,gc,points,nPoints,Convex,CoordModeOrigin);

   XSetClipMask(display,gc,None);


/* If the value string is supposed to be displayed, draw it. */
   if (w->indicator.value_visible == True)
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
  IndicatorWidget w,
  char *upper, char *lower)
{

   if (w->value.datatype == XcLval)  
   {
      cvtLongToString(w->value.upper_bound.lval, upper); 
      cvtLongToString(w->value.lower_bound.lval, lower); 
   } 
   else if (w->value.datatype == XcHval)  
   {
      cvtLongToHexString(w->value.upper_bound.lval, upper); 
      cvtLongToHexString(w->value.lower_bound.lval, lower); 
   } 
   else if (w->value.datatype == XcFval)  
   {
      cvtFloatToString(w->value.upper_bound.fval, upper,
				(unsigned short)w->value.decimals); 
      cvtFloatToString(w->value.lower_bound.fval, lower,
				(unsigned short)w->value.decimals); 
   } 

}  /* end of Print_bounds */


/* end of Indicator.c */

