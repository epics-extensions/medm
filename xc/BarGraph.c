/*******************************************************************
 FILE:		BarGraph.c
 CONTENTS:	Definitions for structures, methods, and actions of the 
		BarGraph widget.
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
static void ClassInitialize();
static void Initialize(BarGraphWidget request, BarGraphWidget new);
static void Redisplay(BarGraphWidget w, XExposeEvent *event, Region region);
static void Destroy(BarGraphWidget w);
static void Resize(BarGraphWidget w);
static XtGeometryResult QueryGeometry(
  BarGraphWidget w,
  XtWidgetGeometry *proposed, XtWidgetGeometry *answer);
static Boolean SetValues(
  BarGraphWidget cur,
  BarGraphWidget req,
  BarGraphWidget new);



/* Declare functions and variables private to this widget */
static void Draw_display(
  BarGraphWidget w,
  Display *display,
  Drawable drawable,
  GC gc);
static void Get_value(XtPointer client_data,XtIntervalId *id);
static void Print_bounds(BarGraphWidget w, char *upper, char *lower);




/* Define the widget's resource list */
static XtResource resources[] =
{
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
};




/* Widget Class Record initialization */
BarGraphClassRec barGraphClassRec =
{
  {
  /* core_class part */
    (WidgetClass) &valueClassRec,		/* superclass */
    "BarGraph",					/* class_name */
    sizeof(BarGraphRec),			/* widget_size */
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
  /* BarGraph class part */
    0,						/* dummy_field */
  }
};

WidgetClass xcBarGraphWidgetClass = (WidgetClass)&barGraphClassRec;


/* Widget method function definitions */

/*******************************************************************
 NAME:		ClassInitialize.	
 DESCRIPTION:
   This method initializes the BarGraph widget class. Specifically,
it registers resource value converter functions with Xt.

*******************************************************************/

static void ClassInitialize()
{

   XtAddConverter(XtRString, XcROrient, CvtStringToOrient, NULL, 0);

}  /* end of ClassInitialize */






/*******************************************************************
 NAME:		Initialize.		
 DESCRIPTION:
   This is the initialize method for the BarGraph widget.  It 
validates user-modifiable instance resources and initializes private 
widget variables and structures.  This function also creates any server 
resources (i.e., GCs, fonts, Pixmaps, etc.) used by this widget.  This
method is called by Xt when the application calls XtCreateWidget().

*******************************************************************/

static void Initialize(BarGraphWidget request, BarGraphWidget new)
{
/* Local variables */
Display *display = XtDisplay(new);


DPRINTF(("BarGraph: executing Initialize...\n"));

/*
 * Validate public instance variable settings.
 */
/* Check orientation resource setting. */
   if ((new->barGraph.orient != XcVert) && (new->barGraph.orient != XcHoriz))
   {
      XtWarning("BarGraph: invalid orientation setting");
      new->barGraph.orient = XcVert;
   }

/* Check the interval resource setting. */
   if (new->barGraph.interval >0)
   {
      new->barGraph.interval_id = 
		XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)new),
				new->barGraph.interval, Get_value, new);
   }

/* Check the scaleSegments resource setting. */
   if (new->barGraph.num_segments < MIN_SCALE_SEGS)
   {
      XtWarning("BarGraph: invalid number of scale segments");
      new->barGraph.num_segments = MIN_SCALE_SEGS;
   }
   else if (new->barGraph.num_segments > MAX_SCALE_SEGS)
   {
      XtWarning("BarGraph: invalid number of scale segments");
      new->barGraph.num_segments = MAX_SCALE_SEGS;
   }
   
/* Check the valueVisible resource setting. */
   if ((new->barGraph.value_visible != True) &&
		(new->barGraph.value_visible != False))
   {
      XtWarning("BarGraph: invalid valueVisible setting");
      new->barGraph.value_visible = True;
   }

/* Initialize the BarGraph width and height. */
   if (new->core.width < MIN_BG_WIDTH)
      new->core.width = MIN_BG_WIDTH; 
   if (new->core.height < MIN_BG_HEIGHT)
      new->core.height = MIN_BG_HEIGHT; 
   
/* Initialize private instance variables.  */

/* Set the initial geometry of the BarGraph elements. */
   Resize(new);


DPRINTF(("BarGraph: done Initialize\n"));

}  /* end of Initialize */





/*******************************************************************
 NAME:		Redisplay.	
 DESCRIPTION:
   This function is the BarGraph's Expose method.  It redraws the
BarGraph's 3D rectangle background, Value Box, label, Bar indicator,
and the Scale.  All drawing takes place within the widget's window 
(no need for an off-screen pixmap).

*******************************************************************/

static void Redisplay(BarGraphWidget w, XExposeEvent *event, Region region)
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

DPRINTF(("BarGraph: executing Redisplay\n"));

/* Draw the 3D rectangle background for the BarGraph. */
   XSetClipMask(XtDisplay(w), w->control.gc, None);
   Rect3d(w, XtDisplay(w), XtWindow(w), w->control.gc,
		0, 0, w->core.width, w->core.height, RAISED);

/* Draw the Label string. */
   XSetClipRectangles(XtDisplay(w), w->control.gc, 0, 0, 
  			&(w->barGraph.face), 1, Unsorted); 
   XSetForeground(XtDisplay(w), w->control.gc,
                        w->control.label_pixel);
   XDrawString(XtDisplay(w), XtWindow(w), w->control.gc,
	w->barGraph.lbl.x, w->barGraph.lbl.y, 
	w->control.label, strlen(w->control.label));

/* Draw the Scale */
   if (w->barGraph.num_segments > 0) {
      XSetForeground(XtDisplay(w), w->control.gc,
                        w->barGraph.scale_pixel);
      XDrawLine(XtDisplay(w), XtWindow(w), w->control.gc,
	w->barGraph.scale_line.x1, w->barGraph.scale_line.y1,
	w->barGraph.scale_line.x2, w->barGraph.scale_line.y2);

/* Draw the max and min value segments. */
      if (w->barGraph.orient == XcVert) {
      } else {
        XDrawLine(XtDisplay(w), XtWindow(w), w->control.gc,
		w->barGraph.scale_line.x1, 
		w->barGraph.scale_line.y1 - w->barGraph.seg_length, 
		w->barGraph.scale_line.x1, w->barGraph.scale_line.y1);
        XDrawLine(XtDisplay(w), XtWindow(w), w->control.gc,
		w->barGraph.scale_line.x2, 
		w->barGraph.scale_line.y2 - w->barGraph.seg_length, 
		w->barGraph.scale_line.x2, w->barGraph.scale_line.y2);
      }

/* Now draw the rest of the Scale segments. */
      for (j = 0; j < w->barGraph.num_segments; j++) {
         if (w->barGraph.orient == XcVert)
	    XDrawLine(XtDisplay(w), XtWindow(w), w->control.gc,
		w->barGraph.segs[j].x, w->barGraph.segs[j].y,
		w->barGraph.scale_line.x1, w->barGraph.segs[j].y);
         else
	    XDrawLine(XtDisplay(w), XtWindow(w), w->control.gc,
		w->barGraph.segs[j].x, w->barGraph.segs[j].y,
		w->barGraph.segs[j].x, w->barGraph.scale_line.y1);
      }

/* Draw the max and min value string indicators */
      Print_bounds(w, upper, lower);
      XDrawString(XtDisplay(w), XtWindow(w), w->control.gc,
    	w->barGraph.max_val.x, w->barGraph.max_val.y,
						upper, strlen(upper)); 
      XDrawString(XtDisplay(w), XtWindow(w), w->control.gc,
    	w->barGraph.min_val.x, w->barGraph.min_val.y, 
						lower, strlen(lower)); 
   }


/* Draw the Bar indicator border */
   Rect3d(w, XtDisplay(w), XtWindow(w), w->control.gc,
	w->barGraph.bar.x - w->control.shade_depth, 
	w->barGraph.bar.y - w->control.shade_depth,
	w->barGraph.bar.width + (2 * w->control.shade_depth),  
	w->barGraph.bar.height + (2 * w->control.shade_depth), DEPRESSED);

/* Draw the Value Box */
   if (w->barGraph.value_visible == True)
      Rect3d(w, XtDisplay(w), XtWindow(w), w->control.gc,
	w->value.value_box.x - w->control.shade_depth, 
	w->value.value_box.y - w->control.shade_depth,
	w->value.value_box.width + (2 * w->control.shade_depth),  
	w->value.value_box.height + (2 * w->control.shade_depth), 
							DEPRESSED);

/* Draw the new value represented by the Bar indicator and the value string */
   Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);


DPRINTF(("BarGraph: done Redisplay\n"));

}  /* end of Redisplay */





/*******************************************************************
 NAME:		SetValues.
 DESCRIPTION:
   This is the set_values method for this widget. It validates resource
settings set with XtSetValues. If a resource is changed that would
require re-drawing the widget, return True.

*******************************************************************/

static Boolean SetValues(
  BarGraphWidget cur,
  BarGraphWidget req,
  BarGraphWidget new)
{
/* Local variables */
Boolean do_redisplay = False, do_resize = False;


DPRINTF(("BarGraph: executing SetValues \n"));

/* Validate new resource settings. */

/* Check widget color resource settings. */
   if ((new->barGraph.bar_foreground != cur->barGraph.bar_foreground) ||
	(new->barGraph.bar_background != cur->barGraph.bar_background) ||
	(new->barGraph.scale_pixel != cur->barGraph.scale_pixel)) 
      do_redisplay = True;

/* Check orientation resource setting. */
   if (new->barGraph.orient != cur->barGraph.orient)
   {
      do_redisplay = True;
      if ((new->barGraph.orient != XcVert) && (new->barGraph.orient != XcHoriz))
      {
         XtWarning("BarGraph: invalid orientation setting");
         new->barGraph.orient = XcVert;
      }
   }

/* Check the interval resource setting. */
   if (new->barGraph.interval != cur->barGraph.interval) 
   {
      if (cur->barGraph.interval > 0)
	    XtRemoveTimeOut (cur->barGraph.interval_id);
      if (new->barGraph.interval > 0)
         new->barGraph.interval_id = 
		XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)new),
					new->barGraph.interval, Get_value, new);
   }

/* Check the scaleSegments resource setting. */
   if (new->barGraph.num_segments != cur->barGraph.num_segments)
   {
      if (new->barGraph.num_segments < MIN_SCALE_SEGS)
      {
         XtWarning("BarGraph: invalid number of scale segments");
         new->barGraph.num_segments = MIN_SCALE_SEGS;
      }
      else if (new->barGraph.num_segments > MAX_SCALE_SEGS)
      {
         XtWarning("BarGraph: invalid number of scale segments");
         new->barGraph.num_segments = MAX_SCALE_SEGS;
      }
   }

/* Check the valueVisible resource setting. */
   if (new->barGraph.value_visible != cur->barGraph.value_visible)
   {
      do_redisplay = True;
      if ((new->barGraph.value_visible != True) &&
		(new->barGraph.value_visible != False))
      {
         XtWarning("BarGraph: invalid valueVisible setting");
         new->barGraph.value_visible = True;
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


DPRINTF(("BarGraph: done SetValues\n"));
   return do_redisplay;


}  /* end of SetValues */




/*******************************************************************
 NAME:		Resize.
 DESCRIPTION:
   This is the resize method of the BarGraph widget. It resizes the
BarGraph's graphics based on the new width and height of the widget's
window.  

*******************************************************************/

static void Resize(BarGraphWidget w)
{
/* Local variables */
int j;
int seg_spacing;
int max_val_width, min_val_width, max_width;
int font_center, font_height;
char upper[30], lower[30];
Boolean displayValue, displayLabel;

DPRINTF(("BarGraph: executing Resize\n"));

/* (MDA) for numbers, usually safe to ignore descent to save space */
   font_height = (w->control.font)->ascent;

/* Set the widgets new width and height. */
   w->barGraph.face.x = w->barGraph.face.y = w->control.shade_depth;
   w->barGraph.face.width = w->core.width - (2*w->control.shade_depth);
   w->barGraph.face.height = w->core.height - (2*w->control.shade_depth);

/* Calculate min/max string attributes */
   Print_bounds(w, upper, lower);
   max_val_width = XTextWidth(w->control.font, upper, strlen(upper));
   min_val_width = XTextWidth(w->control.font, lower, strlen(lower));
   max_width = MAX(min_val_width,max_val_width) + 2*w->control.shade_depth;
   if (w->barGraph.num_segments == 0) max_width = w->barGraph.face.width 
		- 2*w->control.shade_depth;
   
/* Establish the new Value Box geometry. */
   if (w->barGraph.value_visible == True) {
     displayValue = True;
     if (w->barGraph.orient == XcVert) {
      w->value.value_box.x = w->core.width/2 - max_width/2;
      w->value.value_box.y = w->core.height - font_height -
				3*w->control.shade_depth;
      w->value.value_box.width = max_width;
      w->value.value_box.height = font_height;
     } else {
      w->value.value_box.x = w->core.width/2 - max_width/2;
      w->value.value_box.y = w->core.height - font_height -
				2*w->control.shade_depth - 2;
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
	w->barGraph.lbl.x = (short)((w->core.width/2) -
		(XTextWidth(w->control.font, w->control.label, 
			strlen(w->control.label))/2)); 
	w->barGraph.lbl.y = (short)(w->barGraph.face.y +
			w->control.font->ascent + 1);
   }
   else
   {
	displayLabel = False;
	w->barGraph.lbl.x = w->barGraph.face.x;
	w->barGraph.lbl.y = w->barGraph.face.y;
   }   


/* Resize the Bar indicator */
   if (w->barGraph.orient == XcVert)
   {
      w->barGraph.bar.x = (short)(w->barGraph.face.x + 
			+ (w->barGraph.num_segments > 0 ? max_width
				+ (w->barGraph.face.width/32) : 0)
			+ 2*w->control.shade_depth);
      w->barGraph.bar.y = (short)(w->barGraph.lbl.y + w->control.font->descent
			+ 2*w->control.shade_depth);
      w->barGraph.bar.width = (unsigned short)
			MAX(0,
			(int)(w->barGraph.face.width - w->barGraph.bar.x
			- w->control.shade_depth));
      w->barGraph.bar.height = (unsigned short)
			(w->value.value_box.y - w->barGraph.bar.y 
			- (displayValue == True 
				? font_height - w->control.font->descent
				: 2*w->control.shade_depth));
   }
   else
   {
      w->barGraph.bar.x = w->barGraph.face.x +
			(short)(w->barGraph.face.width/16) +
				w->control.shade_depth;
      w->barGraph.bar.y = w->barGraph.lbl.y
			+ (w->barGraph.num_segments > 0 ?
				font_height/2 +
				w->barGraph.face.height/(unsigned short)8 + 1 :
				0)
			+ 2*w->control.shade_depth;
      w->barGraph.bar.width = (short)((9*(int)w->barGraph.face.width)/10) -
					2*w->control.shade_depth;
      w->barGraph.bar.height = MAX(1,w->value.value_box.y - w->barGraph.bar.y 
			- (displayValue == True ? font_height : 0)
			- w->control.shade_depth);
   }


/* Resize the Scale line. */
   if (w->barGraph.orient == XcVert)
   {
      w->barGraph.scale_line.x1 = w->barGraph.bar.x - w->control.shade_depth -
					(w->barGraph.face.width/32);
      w->barGraph.scale_line.y1 = w->barGraph.bar.y;
      w->barGraph.scale_line.x2 = w->barGraph.scale_line.x1;
      w->barGraph.scale_line.y2 = w->barGraph.bar.y + w->barGraph.bar.height;
   }
   else
   {
      w->barGraph.scale_line.x1 = w->barGraph.bar.x;
      w->barGraph.scale_line.y1 = w->barGraph.bar.y - w->control.shade_depth -
					(w->barGraph.face.height/32);
      w->barGraph.scale_line.x2 = w->barGraph.bar.x + w->barGraph.bar.width;
      w->barGraph.scale_line.y2 = w->barGraph.scale_line.y1;
   }

/* Now, resize Scale line segments */
   if (w->barGraph.num_segments > 0)
   {
      if (w->barGraph.orient == XcVert)
      {
         w->barGraph.seg_length = (w->barGraph.face.width/16);
         seg_spacing = (w->barGraph.bar.height / 
				(unsigned short)(w->barGraph.num_segments + 1));
         for (j = 0; j < w->barGraph.num_segments; j++)
         {
	    w->barGraph.segs[j].x = w->barGraph.scale_line.x1 - 
				w->barGraph.seg_length;
	    w->barGraph.segs[j].y = w->barGraph.scale_line.y1 + 
				((j+1) * seg_spacing);
         }
      }
      else
      {
         w->barGraph.seg_length = (w->barGraph.face.height/16);
         seg_spacing = (w->barGraph.bar.width / 
				(unsigned short)(w->barGraph.num_segments + 1));
         for (j = 0; j < w->barGraph.num_segments; j++)
         {
	    w->barGraph.segs[j].x = w->barGraph.scale_line.x1 + 
						((j+1) * seg_spacing);
	    w->barGraph.segs[j].y = w->barGraph.scale_line.y1 - 
						w->barGraph.seg_length;
         }
      }
   }

/* Set the position of the max and min value strings */
   if (w->barGraph.orient == XcVert)
   {
      font_center = ((w->control.font->ascent + 
				w->control.font->descent)/2) 
				- w->control.font->descent;
      w->barGraph.max_val.x = MAX(w->control.shade_depth,
			      w->barGraph.scale_line.x2 - (max_val_width));

      w->barGraph.max_val.y = w->barGraph.scale_line.y1 + font_center;

      w->barGraph.min_val.x = MAX(w->control.shade_depth,
			      w->barGraph.scale_line.x2 - (min_val_width));
      w->barGraph.min_val.y = w->barGraph.scale_line.y2 + font_center;
   }
   else
   {
      w->barGraph.max_val.x = MIN(
			(int)(w->barGraph.face.width + w->control.shade_depth
				  - max_val_width),
			(w->barGraph.scale_line.x2 - (max_val_width/2)));
      w->barGraph.max_val.y = w->barGraph.min_val.y =
      		w->barGraph.scale_line.y1 - w->barGraph.seg_length - 1;
      w->barGraph.min_val.x = MAX(w->control.shade_depth,
			w->barGraph.scale_line.x1 - (min_val_width/2));
   }


DPRINTF(("BarGraph: done Resize\n"));

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
  BarGraphWidget w,
  XtWidgetGeometry *proposed, XtWidgetGeometry *answer)
{
/* Set the request mode mask for the returned answer. */
   answer->request_mode = CWWidth | CWHeight;

/* Set the recommended size. */
   answer->width = (w->core.width > MAX_BG_WIDTH)
	? MAX_BG_WIDTH : w->core.width;
   answer->height = (w->core.height > MAX_BG_HEIGHT)
	? MAX_BG_HEIGHT : w->core.height;

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

static void Destroy(BarGraphWidget w)
{

   if (w->barGraph.interval > 0)
      XtRemoveTimeOut (w->barGraph.interval_id);

}  /* end of Destroy */





/* Widget action functions. */

/*******************************************************************
 NAME:		Get_value.		
 DESCRIPTION:
   This function is the time out procedure called at XcNinterval 
intervals.  It calls the application registered callback function to
get the latest value and updates the BarGraph display accordingly.

*******************************************************************/

static void Get_value(XtPointer client_data,
XtIntervalId *id)		/* unused */
{
/* Local variables */
static XcCallData call_data;
BarGraphWidget w = (BarGraphWidget)client_data;
   
/* Get the new value by calling the application's callback if it exists. */
   if (w->barGraph.update_callback == NULL)
       return;

/* Re-register this TimeOut procedure for the next interval. */
   if (w->barGraph.interval > 0)
      w->barGraph.interval_id = 
	XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)w),
		       w->barGraph.interval, Get_value, client_data);

/* Set the widget's current value and datatype before calling the callback. */
   call_data.dtype = w->value.datatype;
   call_data.decimals = w->value.decimals;
   if ((w->value.datatype == XcLval) || (w->value.datatype == XcHval))  
      call_data.value.lval = w->value.val.lval;
   else if (w->value.datatype == XcFval)
      call_data.value.fval = w->value.val.fval;
   XtCallCallbacks((Widget)w, XcNupdateCallback, &call_data);


/* Update the new value, update the BarGraph display. */
   if ((w->value.datatype == XcLval) || (w->value.datatype == XcHval))  
	w->value.val.lval = call_data.value.lval;
   else if (w->value.datatype == XcFval)
	w->value.val.fval = call_data.value.fval;

   if (XtIsRealized((Widget)w))
      Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);
   

}  /* end of Get_value */




/*******************************************************************
 NAME:		XcBGUpdateValue.		
 DESCRIPTION:
   This convenience function is called by the application in order to
update the value (a little quicker than using XtSetArg/XtSetValue).
The application passes the new value to be updated with.

*******************************************************************/

void XcBGUpdateValue(
  Widget widget,
  XcVType *value)
{
/* Local variables */

  BarGraphWidget w = (BarGraphWidget) widget;

    if (!w->core.visible) return;
    
/* Update the new value, then update the BarGraph display. */
   if (value != NULL)
   {
      if ((w->value.datatype == XcLval) || (w->value.datatype == XcHval))  
	 w->value.val.lval = value->lval;
      else if (w->value.datatype == XcFval)
	 w->value.val.fval = value->fval;

      if (XtIsRealized((Widget)w))
         Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);

   }
   
}  /* end of XcBGUpdateValue */



/*******************************************************************
 NAME:		XcBGUpdateBarForeground.		
 DESCRIPTION:
   This convenience function is called by the application in order to
update the value (a little quicker than using XtSetArg/XtSetValue).
The application passes the new value to be updated with.

*******************************************************************/

void XcBGUpdateBarForeground(
  Widget widget,
  unsigned long pixel)
{
/* Local variables */

  BarGraphWidget w = (BarGraphWidget) widget;

    if (!w->core.visible) return;
    
/* Update the new value, then update the BarGraph display. */
   if (w->barGraph.bar_foreground != pixel) {
      w->barGraph.bar_foreground = pixel;
      if (XtIsRealized((Widget)w))
         Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);
   }
   
}  /* end of XcBGUpdateBarForeground */



/*******************************************************************
 NAME:		Draw_display.		
 DESCRIPTION:
   This function redraws the Bar indicator and the value string in the 
Value Box.

*******************************************************************/

static void Draw_display(
  BarGraphWidget w,
  Display *display,
  Drawable drawable,
  GC gc)
{
/* Local variables */
char *temp;
float range, dim;

/* Draw the Bar indicator */
/* Fill the Bar with its background color. */
   XSetForeground(display, gc, w->barGraph.bar_background); 
   XFillRectangle(display, drawable, gc,
  	w->barGraph.bar.x, w->barGraph.bar.y, 
  	w->barGraph.bar.width, w->barGraph.bar.height); 

/* Draw the Bar in its foreground color according to the value. */
   if (w->barGraph.orient == XcVert)
      range = (float)(w->barGraph.bar.height);
   else
      range = (float)(w->barGraph.bar.width);

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

   if ((int)dim < 1)
      dim = 1;
   XSetForeground(display, gc, w->barGraph.bar_foreground); 
   if (w->barGraph.orient == XcVert)
      XFillRectangle(display, drawable, gc, w->barGraph.bar.x,
		(w->barGraph.bar.y + w->barGraph.bar.height - (int)dim),
		w->barGraph.bar.width, (int)dim);
   else
      XFillRectangle(display, drawable, gc, w->barGraph.bar.x,
		w->barGraph.bar.y, (int)dim, w->barGraph.bar.height);


/* If the value string is supposed to be displayed, draw it. */
   if (w->barGraph.value_visible == True)
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
  BarGraphWidget w,
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


/* end of BarGraph.c */

