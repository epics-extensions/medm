/*************************************************************************
 * Program : Byte.c                                                      *
 * Author  : David M. Wetherholt - DMW Software 1994                     *
 * Lab     : Continuous Electron Beam Accelerator Facility               *
 * Modified: 7 July 94                                                   *
 * Mods    : 1.0 Original                                                *
 *************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>

#include "Xc.h"
#include "Control.h"
#include "Value.h"
#include "ByteP.h"
#include "cvtFast.h"

#ifndef MIN
# define  MIN(a,b)    (((a) < (b)) ? (a) :  (b))
#endif
#ifndef MAX
#  define  MAX(a,b)    (((a) > (b)) ? (a) :  (b))
#endif

/****** Macro redefinition for offset. */
#define offset(field) XtOffset(ByteWidget, field)

/****** Declare widget methods */
static void ClassInitialize();
static void Initialize();
static void Redisplay();
static void Destroy();
static void Resize();
static XtGeometryResult QueryGeometry();
static Boolean SetValues();

/* Declare functions and variables private to this widget */
static void Update_value();
static void Draw_display();
static void Get_value();
static void Print_bounds();

/* Define the widget's resource list */
static XtResource resources[] = {
    {
	XcNorient,
	XcCOrient,
	XcROrient,
	sizeof(XcOrient),
	offset(byte.orient),
	XtRString,
	"vertical"
    },
    {
	XcNbyteForeground,
	XtCColor,
	XtRPixel,
	sizeof(Pixel),
	offset(byte.byte_foreground),
	XtRString,
	XtDefaultForeground
    },
    {
	XcNbyteBackground,
	XtCColor,
	XtRPixel,
	sizeof(Pixel),
	offset(byte.byte_background),
	XtRString,
	XtDefaultBackground
    },
    {
	XcNinterval,
	XcCInterval,
	XtRInt,
	sizeof(int),
	offset(byte.interval),
	XtRImmediate,
	(XtPointer)0
    },
    {
	XcNsBit,
	XcCsBit,
	XtRInt,
	sizeof(int),
	offset(byte.sbit),
	XtRImmediate,
	(XtPointer)0
    },
    {
	XcNeBit,
	XcCeBit,
	XtRInt,
	sizeof(int),
	offset(byte.ebit),
	XtRImmediate,
	(XtPointer)15
    },
    {
	XcNupdateCallback,
	XtCCallback,
	XtRCallback,
	sizeof(XtPointer),
	offset(byte.update_callback),
	XtRCallback,
	NULL
    },
};

/* Widget Class Record initialization */
ByteClassRec byteClassRec = {
    {
	(WidgetClass) &valueClassRec,		/* superclass */
	"Byte",					/* class_name */
	sizeof(ByteRec),                            /* widget_size */
	ClassInitialize,  NULL, FALSE,              /* class_initialize */
	Initialize,	      NULL,                     /* initialize, hook */
	XtInheritRealize, NULL, 0,                  /* realize, action, nactions */
	resources,					/* resources */
	XtNumber(resources),			/* num_resources */
	NULLQUARK,					/* xrm_class */
	TRUE,					/* compress_motion */
	TRUE,					/* compress_exposure */
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
  /****** Control class part, value class part, byte class part */
    { 0, }, { 0, }, { 0, }
};

WidgetClass xcByteWidgetClass = (WidgetClass)&byteClassRec;

static void ClassInitialize() {
  /*************************************************************************
   * ClassInitialize: This method initializes the Byte widget class.       *
   *   It registers resource value converter functions with Xt.            *
   *************************************************************************/
    XtAddConverter(XtRString, XcROrient, CvtStringToOrient, NULL, 0);
}

static void Initialize(request, new)
  /*************************************************************************
 * Initialize: This is the initialize method for the Byte widget.  It    *
 *   validates user-modifiable instance resources and initializes private*
 *   widget variables and structures.  This function also creates any    *
 *   server resources (i.e., GCs, fonts, Pixmaps, etc.) used by this     *
 *   widget.  This method is called by Xt when the application calls     *
 *   XtCreateWidget().                                                   *
 *************************************************************************/
    ByteWidget request, new;
{
    Display *display = XtDisplay(new);
    DPRINTF(("Byte: executing Initialize...\n"));
  /* printf("BY: executing Initialize1\n"); */

  /****** Validate public instance variable settings.
    Check orientation resource setting. */
    if ((new->byte.orient != XcVert) && (new->byte.orient != XcHoriz)){
	XtWarning("Byte: invalid orientation setting");
	new->byte.orient = XcHoriz;
    }

  /****** Check the interval resource setting. */
    if (new->byte.interval >0) {
	new->byte.interval_id = 
	  XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)new),
	    new->byte.interval, Get_value, new);
    }

  /****** Initialize the Byte width and height. */
    if (new->core.width < MIN_BY_WIDTH) new->core.width = MIN_BY_WIDTH; 
    if (new->core.height < MIN_BY_HEIGHT) new->core.height = MIN_BY_HEIGHT; 

  /****** Set the initial geometry of the Byte elements. */
    Resize(new);

    DPRINTF(("Byte: done Initialize\n"));
}

static void Resize(w)
  /*************************************************************************
 * Resize: This is the resize method of the Byte widget. It re-sizes the *
 *   Byte's graphics based on the new width and height of the widget's   *
 *   window.                                                             *
 *************************************************************************/
    ByteWidget w;
{

    int j, max_val_width, min_val_width, max_width;
    int seg_spacing, font_center, font_height;
    char upper[30], lower[30];
    Boolean displayValue, displayLabel;

    DPRINTF(("Byte: executing Resize\n"));
  /* printf("BY: executing Resize1\n"); */

  /****** For numbers, usually safe to ignore descent to save space */
    font_height = (w->control.font)->ascent;

  /****** Set the widgets new width and height. */
    w->byte.face.x = w->byte.face.y = 0;
    w->byte.face.width = w->core.width;
    w->byte.face.height = w->core.height;

  /****** Calculate min/max string attributes
    Print_bounds(w, upper, lower);
    max_val_width = XTextWidth(w->control.font, upper, strlen(upper));
    min_val_width = XTextWidth(w->control.font, lower, strlen(lower));
    max_width = MAX(min_val_width,max_val_width) + 2*w->control.shade_depth;
    max_width = w->byte.face.width - 2*w->control.shade_depth; */
   
    DPRINTF(("Byte: done Resize\n"));
}

static void Redisplay(w, event)
  /*************************************************************************
 * Redisplay : This function is the Byte's Expose method.  It redraws    *
 *   the Byte's 3D rectangle background, Value Box, label, Bar           *
 *   indicator, and the Scale.  All drawing takes place within the       *
 *   widget's window (no need for an off-screen pixmap).                 *
 *************************************************************************/
    ByteWidget w;
    XExposeEvent *event;
{
    int j;
    char upper[30], lower[30];

  /****** Check to see whether or not the widget's window is mapped */
    if (!XtIsRealized((Widget)w) || !w->core.visible) return;
    DPRINTF(("Byte: executing Redisplay\n"));
  /* printf("BY: executing Redisplay1\n"); */

  /****** Draw the new values of Bar indicator and the value string */
    Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);
    DPRINTF(("Byte: done Redisplay\n"));
}

static Boolean SetValues(cur, req, new)
  /****************************************************************************
 * Set Values: This is the set_values method for this widget. It validates  *
 *   resource settings set with XtSetValues. If a resource is changed that  *
 *   would require re-drawing the widget, return True.                      *
 ****************************************************************************/
    ByteWidget cur, req, new;
{
    Boolean do_redisplay = False, do_resize = False;

    DPRINTF(("Byte: executing SetValues\n"));

  /****** Check widget color resource settings */
    if ((new->byte.byte_foreground != cur->byte.byte_foreground) ||
      (new->byte.byte_background != cur->byte.byte_background))
      do_redisplay = True;

  /****** Check orientation resource setting */
    if (new->byte.orient != cur->byte.orient) {
	do_redisplay = True;
	if ((new->byte.orient != XcVert) && (new->byte.orient != XcHoriz)) {
	    XtWarning("Byte: invalid orientation setting");
	    new->byte.orient = XcHoriz;
	}
    }

  /****** Check the interval resource setting */
    if (new->byte.interval != cur->byte.interval) {
	if (cur->byte.interval > 0) XtRemoveTimeOut (cur->byte.interval_id);
	if (new->byte.interval > 0)
	  new->byte.interval_id = 
	    XtAppAddTimeOut( XtWidgetToApplicationContext((Widget)new), 
	      new->byte.interval, Get_value, new);
    }

  /****** Check the valueVisible resource setting
    if (new->byte.value_visible != cur->byte.value_visible) {
    do_redisplay = True;
    if ((new->byte.value_visible != True) &&
    (new->byte.value_visible != False)) {
    XtWarning("Byte: invalid valueVisible setting");
    new->byte.value_visible = True;
    }
    } */

  /****** Check to see if the value has changed. */
    if ((((new->value.datatype == XcLval) || (new->value.datatype == XcHval)) 
      && (new->value.val.lval != cur->value.val.lval)) ||
      ((new->value.datatype == XcFval) && 
	(new->value.val.fval != cur->value.val.fval))) {
	do_redisplay = True;
    }

  /* (MDA) want to force resizing if min/max changed  or decimals setting */
    if (new->value.decimals != cur->value.decimals) do_resize = True;

    if (((new->value.datatype == XcLval) || (new->value.datatype == XcHval)) &&
      ((new->value.lower_bound.lval != cur->value.lower_bound.lval) ||
	(new->value.upper_bound.lval != cur->value.upper_bound.lval) )) {
	do_resize = True;
    }
    else if ((new->value.datatype == XcFval) &&
      ((new->value.lower_bound.fval != cur->value.lower_bound.fval) ||
	(new->value.upper_bound.fval != cur->value.upper_bound.fval) )) {
	do_resize = True;
    }
    if (do_resize) {
	Resize(new);
	do_redisplay = True;
    }

    DPRINTF(("Byte: done SetValues\n"));
    return do_redisplay;
}

static XtGeometryResult QueryGeometry(w, proposed, answer)
  /*************************************************************************
 * Query Geometry: This function is the widget's query_geometry method.  *
 *   It simply checks the proposed size and returns the appropriate value*
 *   based on the proposed size.  If the proposed size is greater than   *
 *   the maximum appropriate size for this widget, QueryGeometry returns *
 *   the recommended size.                                               *
 *************************************************************************/
    ByteWidget w;
    XtWidgetGeometry *proposed, *answer;
{
  /****** Set the request mode mask for the returned answer */
    answer->request_mode = CWWidth | CWHeight;

  /****** Set the recommended size */
    answer->width=(w->core.width > MAX_BY_WIDTH) ? MAX_BY_WIDTH : w->core.width;
    answer->height=(w->core.height > MAX_BY_HEIGHT) 
      ? MAX_BY_HEIGHT : w->core.height;

  /****** Check the proposed dimensions. If the proposed size is larger than
    appropriate, return the recommended size.  */
    if (((proposed->request_mode & (CWWidth | CWHeight)) == (CWWidth | CWHeight))
      && proposed->width == answer->width && proposed->height == answer->height)
      return XtGeometryYes;
    else if (answer->width == w->core.width && answer->height == w->core.height)
      return XtGeometryNo;
    else
      return XtGeometryAlmost;
}

static void Destroy(w)
  /*************************************************************************
 * Destroy: This function is the widget's destroy method.  It simply     *
 *   releases any server resources acquired during the life of the widget*
 *************************************************************************/
    ByteWidget w;
{

    if (w->byte.interval > 0) XtRemoveTimeOut (w->byte.interval_id);
}

static void Get_value(client_data, id)
  /*************************************************************************
 * Get Value: This function is the time out procedure called at          *
 *   XcNinterval intervals.  It calls the application registered callback*
 *   function to get the latest value and updates the Byte display   *
 *   accordingly.                                                        *
 *************************************************************************/
    XtPointer client_data;
    XtIntervalId *id;
{
    static XcCallData call_data;
    ByteWidget w = (ByteWidget)client_data;
   
  /****** Get the new value by calling the application's callback if it exists */
    if (w->byte.update_callback == NULL) return;

  /****** Re-register this TimeOut procedure for the next interval */
    if (w->byte.interval > 0) w->byte.interval_id = 
				XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)w),
				  w->byte.interval, Get_value, client_data);

  /****** Set the current value and datatype before calling the callback */
    call_data.dtype = w->value.datatype;
    call_data.decimals = w->value.decimals;
    if ((w->value.datatype == XcLval) || (w->value.datatype == XcHval))  
      call_data.value.lval = w->value.val.lval;
    else if (w->value.datatype == XcFval)
      call_data.value.fval = w->value.val.fval;
    XtCallCallbacks((Widget)w, XcNupdateCallback, &call_data);

  /****** Update the new value, update the Byte display */
    if ((w->value.datatype == XcLval) || (w->value.datatype == XcHval))  
      w->value.val.lval = call_data.value.lval;
    else if (w->value.datatype == XcFval)
      w->value.val.fval = call_data.value.fval;

    if (XtIsRealized((Widget)w))
      Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);
}

void XcBYUpdateValue(w, value)
  /*************************************************************************
 * Xc Byte Update Value:  This convenience function is called by the     *
 *   application in order to update the value (a little quicker than     *
 *   using XtSetArg/XtSetValue).  The application passes the new value   *
 *   to be updated with.                                                 *
 *************************************************************************/
    ByteWidget w;
    XcVType *value;
{

    if (!w->core.visible) return;
    
  /****** Update the new value, then update the Byte display. */
    if (value != NULL) {
	if ((w->value.datatype == XcLval) || (w->value.datatype == XcHval))  
	  w->value.val.lval = value->lval;
	else if (w->value.datatype == XcFval)
	  w->value.val.fval = value->fval;

	if (XtIsRealized((Widget)w))
	  Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);
    }
}

void XcBYUpdateByteForeground(w, pixel)
  /*************************************************************************
 * Xc Byte Update Byte Foreground: This convenience function is called   *
 *   by the application in order to update the value (a little quicker   *
 *   than using XtSetArg/XtSetValue).  The application passes the new    *
 *   value to be updated with.                                           *
 *************************************************************************/
    ByteWidget w;
    unsigned long pixel;
{

  /* Local variables */
    if (!w->core.visible) return;
    
  /* Update the new value, then update the Byte display. */
    if (w->byte.byte_foreground != pixel) {
	w->byte.byte_foreground = pixel;
	if (XtIsRealized((Widget)w))
	  Draw_display(w, XtDisplay(w), XtWindow(w), w->control.gc);
    }
}

static void Draw_display( w, display, drawable, gc)
  /*************************************************************************
 * Draw Display: This function redraws the Bar indicator and the value   *
 *   string in the Value Box.                                            *
 *************************************************************************/
    ByteWidget w;
    Display *display;
    Drawable drawable;
    GC gc;
{
    int  back_fg=0, dec_val, i, j, segn;
    float seg;

  /* printf("BY: Draw display2\n"); */

  /****** Draw the Byte indicator, fill the Bar with its background color */
    XSetForeground(display, gc, w->byte.byte_background); 
    XFillRectangle(display, drawable, gc, w->byte.face.x, w->byte.face.y, 
      w->byte.face.width, w->byte.face.height);
    segn = w->byte.ebit - w->byte.sbit;
    if (segn < 0) back_fg = 1;
    segn = abs(segn) + 1;
    dec_val = (int)w->value.val.fval;
    XSetForeground(display, gc, XBlackPixel(display, 0));

  /****** Hi->Low Byte */
    if (back_fg) {
      /****** Vertical hi->low byte */
	if (w->byte.orient == XcVert) {
	    seg = ((float)w->byte.face.height / segn);
	    for (i=w->byte.ebit; i < w->byte.sbit+1; i++) {
		j = (int)(seg * (float)(w->byte.sbit - i));
		if ((dec_val) & ((int)(pow(2.0, (float)i)))) {
		    XSetForeground(display, gc, w->byte.byte_foreground); 
		    XFillRectangle(display, drawable, gc, w->byte.face.x, j,
		      w->byte.face.x + w->byte.face.width, (int)(seg-1.0));
		    XSetForeground(display, gc, XBlackPixel(display, 0));
		}
		XDrawLine(display, drawable, gc, w->byte.face.x, j-1, 
		  w->byte.face.x + w->byte.face.width, j-1);
	    }
	} else {
	  /****** Horizontal hi->low byte */
	    seg = ((float)(w->byte.face.width + 2) / segn);
	    for (i=w->byte.ebit; i < w->byte.sbit+1; i++) {
		j = w->byte.face.width - (int)(seg * (float)(i - w->byte.ebit)) 
		  - (int)(seg + 1.0);
		if ((dec_val) & ((int)(pow(2.0, (float)i)))) {
		    XSetForeground(display, gc, w->byte.byte_foreground); 
		    XFillRectangle(display, drawable, gc, j, w->byte.face.y, 
		      (int)seg-1, w->byte.face.height);
		    XSetForeground(display, gc, XBlackPixel(display, 0));
		}
		XDrawLine(display, drawable, gc, j-1, w->byte.face.y, 
		  j-1, w->byte.face.height);
	    }
	}

      /****** Low->Hi Byte */
    } else {
      /****** Vertical low->hi byte */
	if (w->byte.orient == XcVert) {
	    seg = ((float)w->byte.face.height / segn);
	    for (i=w->byte.sbit; i < w->byte.ebit+1; i++) {
		j = (int)(seg * (float)i);
		if ((dec_val) & ((int)(pow(2.0, (float)i)))) {
		    XSetForeground(display, gc, w->byte.byte_foreground);
		    XFillRectangle(display, drawable, gc, w->byte.face.x, j,
		      w->byte.face.x + w->byte.face.width, (int)seg);
		    XSetForeground(display, gc, XBlackPixel(display, 0));
		}
		XDrawLine(display, drawable, gc, w->byte.face.x, j-1,
		  w->byte.face.x + w->byte.face.width, j-1);
	    }
	} else {
	  /****** Horizontal low->hi byte */
	    seg = ((float)(w->byte.face.width + 2) / segn);
	    for (i=w->byte.sbit; i < w->byte.ebit+1; i++) {
		j = (int)(seg * (float)(i - w->byte.sbit));
		if ((dec_val) & ((int)(pow(2.0, (float)i)))) {
		    XSetForeground(display, gc, w->byte.byte_foreground); 
		    XFillRectangle(display, drawable, gc, j, w->byte.face.y, 
		      (int)seg, w->byte.face.height);
		    XSetForeground(display, gc, XBlackPixel(display, 0));
		}
	      /****** Draw bit separator lines */
		XDrawLine(display, drawable, gc, j-1, w->byte.face.y, 
		  j-1, w->byte.face.height);
	    }
	}
    }
}

static void Print_bounds(w, upper, lower)
  /*************************************************************************
 * Print Bounds: This is a utility function used by the Redisplay and    *
 *   Resize methods to print the upper and lower bound values as strings *
 *   for displaying and resizing purposes.                               *
 *************************************************************************/
    ByteWidget w;
    char *upper, *lower;
{

    if (w->value.datatype == XcLval) {
	cvtLongToString(w->value.upper_bound.lval, upper);
	cvtLongToString(w->value.lower_bound.lval, lower);
    }
    else if (w->value.datatype == XcHval) {
	cvtLongToHexString(w->value.upper_bound.lval, upper);
	cvtLongToHexString(w->value.lower_bound.lval, lower);
    }
    else if (w->value.datatype == XcFval) {
	cvtFloatToString(w->value.upper_bound.fval, upper,
	  (unsigned short)w->value.decimals);
	cvtFloatToString(w->value.lower_bound.fval, lower,
	  (unsigned short)w->value.decimals);
    }
}
