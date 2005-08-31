/*************************************************************************\
* Copyright (c) 2002 Southeastern Universities Research Association, as
*     Operator of Thomas Jefferson National Accelerator Facility.
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*************************************************************************
 * Program : Byte.c                                                      *
 * Author  : David M. Wetherholt                                         *
 * Lab     : Continuous Electron Beam Accelerator Facility               *
 * Modified: 10 Apr 98                                                   *
 * Mods    : 1.0 Original                                                *
 *************************************************************************/
#define DEBUG_SIZE 0

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
static void ClassInitialize(void);
static void Initialize(Widget request, Widget new,
  ArgList args, Cardinal *nargs);
static void Resize(Widget w);
static void Redisplay(Widget w, XEvent *event, Region region);
static Boolean SetValues(Widget cur, Widget req,
  Widget new, ArgList args, Cardinal *nargs);
static XtGeometryResult QueryGeometry(Widget w, XtWidgetGeometry *proposed,
  XtWidgetGeometry *answer);
static void Destroy(Widget w);

/* Declare functions and variables private to this widget */
static void Get_value(XtPointer client_data, XtIntervalId *id);
static void Draw_display(Widget w, Display *display,
  Drawable drawable, GC gc);
#if 0
static void Print_bounds(Widget w, char *upper, char *lower);
#endif

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
        (WidgetClass) &valueClassRec,  /* superclass */
        "Byte",                        /* class_name */
        sizeof(ByteRec),               /* widget_size */
        ClassInitialize,  NULL, FALSE, /* class_initialize */
        Initialize,           NULL,    /* initialize, hook */
        XtInheritRealize, NULL, 0,     /* realize, action, nactions */
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
  /****** Control class part, value class part, byte class part */
    { 0, }, { 0, }, { 0, }
};

WidgetClass xcByteWidgetClass = (WidgetClass)&byteClassRec;

static void ClassInitialize(void)
  /*************************************************************************
   * ClassInitialize: This method initializes the Byte widget class.       *
   *   It registers resource value converter functions with Xt.            *
   *************************************************************************/
{
    XtAddConverter(XtRString, XcROrient, CvtStringToOrient, NULL, 0);
}

static void Initialize(Widget request, Widget new,
  ArgList args, Cardinal *nargs)
  /*************************************************************************
   * Initialize: This is the initialize method for the Byte widget.  It    *
   *   validates user-modifiable instance resources and initializes private*
   *   widget variables and structures.  This function also creates any    *
   *   server resources (i.e., GCs, fonts, Pixmaps, etc.) used by this     *
   *   widget.  This method is called by Xt when the application calls     *
   *   XtCreateWidget().                                                   *
   *************************************************************************/
{
    ByteWidget wnew = (ByteWidget)new;
    DPRINTF(("Byte: executing Initialize...\n"));
  /* printf("BY: executing Initialize1\n"); */

  /****** Validate public instance variable settings.
	  Check orientation resource setting. */
    if ((wnew->byte.orient != XcVert) && (wnew->byte.orient != XcHoriz)) {
	XtWarning("Byte: invalid orientation setting");
	wnew->byte.orient = XcHoriz;
    }

  /****** Check the interval resource setting. */
    if (wnew->byte.interval >0) {
	wnew->byte.interval_id =
	  XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)new),
	    wnew->byte.interval, Get_value, new);
    }

  /****** Initialize the Byte width and height. */
    if (wnew->core.width < MIN_BY_WIDTH) wnew->core.width = MIN_BY_WIDTH;
    if (wnew->core.height < MIN_BY_HEIGHT) wnew->core.height = MIN_BY_HEIGHT;

  /****** Set the initial geometry of the Byte elements. */
    Resize(new);

    DPRINTF(("Byte: done Initialize\n"));
}

static void Resize(Widget w)
  /*************************************************************************
   * Resize: This is the resize method of the Byte widget. It re-sizes the *
   *   Byte's graphics based on the new width and height of the widget's   *
   *   window.                                                             *
   *************************************************************************/
{
    ByteWidget wb = (ByteWidget)w;
    int font_height;

    DPRINTF(("Byte: executing Resize\n"));
  /* printf("BY: executing Resize1\n"); */

  /****** For numbers, usually safe to ignore descent to save space */
    font_height = (wb->control.font)->ascent;

  /****** Set the widgets new width and height. */
    wb->byte.face.x = wb->byte.face.y = 0;
    wb->byte.face.width = wb->core.width;
    wb->byte.face.height = wb->core.height;

  /****** Calculate min/max string attributes
	  Print_bounds(w, upper, lower);
	  max_val_width = XTextWidth(wb->control.font, upper, strlen(upper));
	  min_val_width = XTextWidth(wb->control.font, lower, strlen(lower));
	  max_width = MAX(min_val_width,max_val_width) + 2*wb->control.shade_depth;
	  max_width = wb->byte.face.width - 2*wb->control.shade_depth; */

    DPRINTF(("Byte: done Resize\n"));
}

static void Redisplay(Widget w, XEvent *event, Region region)
/***********************************************************************
 * Redisplay : This function is the Byte's Expose method.  It redraws  *
 *   the Byte.  All drawing takes place within the widget's window     *
 *   (no need for an off-screen pixmap).                               *
 ***********************************************************************/
{
    ByteWidget wb = (ByteWidget)w;

  /****** Check to see whether or not the widget's window is mapped */
#ifdef USE_CORE_VISIBLE
    if (!XtIsRealized((Widget)w) || !wb->core.visible) return;
#else
    if (!XtIsRealized((Widget)w)) return;
#endif
    DPRINTF(("Byte: executing Redisplay\n"));
  /* printf("BY: executing Redisplay1\n"); */

  /****** Draw the new values of the Byte */
    Draw_display(w, XtDisplay(w), XtWindow(w), wb->control.gc);
    DPRINTF(("Byte: done Redisplay\n"));
}

static Boolean SetValues(Widget cur, Widget req,
  Widget new, ArgList args, Cardinal *nargs)
  /* KE: req, args, nargs is not used */
  /****************************************************************************
 * Set Values: This is the set_values method for this widget. It validates  *
 *   resource settings set with XtSetValues. If a resource is changed that  *
 *   would require re-drawing the widget, return True.                      *
 ****************************************************************************/
{
    ByteWidget wnew = (ByteWidget)new;
    ByteWidget wcur = (ByteWidget)cur;
    Boolean do_redisplay = False, do_resize = False;

    DPRINTF(("Byte: executing SetValues\n"));

  /****** Check widget color resource settings */
    if ((wnew->byte.byte_foreground != wcur->byte.byte_foreground) ||
      (wnew->byte.byte_background != wcur->byte.byte_background))
      do_redisplay = True;

  /****** Check orientation resource setting */
    if (wnew->byte.orient != wcur->byte.orient) {
	do_redisplay = True;
	if ((wnew->byte.orient != XcVert) && (wnew->byte.orient != XcHoriz)) {
	    XtWarning("Byte: invalid orientation setting");
	    wnew->byte.orient = XcHoriz;
	}
    }

  /****** Check the interval resource setting */
    if (wnew->byte.interval != wcur->byte.interval) {
	if (wcur->byte.interval > 0) XtRemoveTimeOut (wcur->byte.interval_id);
	if (wnew->byte.interval > 0)
	  wnew->byte.interval_id =
	    XtAppAddTimeOut( XtWidgetToApplicationContext((Widget)new),
	      wnew->byte.interval, Get_value, new);
    }

  /****** Check the valueVisible resource setting
	  if (wnew->byte.value_visible != wcur->byte.value_visible) {
	  do_redisplay = True;
	  if ((wnew->byte.value_visible != True) &&
	  (wnew->byte.value_visible != False)) {
	  XtWarning("Byte: invalid valueVisible setting");
	  wnew->byte.value_visible = True;
	  }
	  } */

  /****** Check to see if the value has changed. */
    if ((((wnew->value.datatype == XcLval) || (wnew->value.datatype == XcHval))
      && (wnew->value.val.lval != wcur->value.val.lval)) ||
      ((wnew->value.datatype == XcFval) &&
	(wnew->value.val.fval != wcur->value.val.fval))) {
	do_redisplay = True;
    }

  /* (MDA) want to force resizing if min/max changed  or decimals setting */
    if (wnew->value.decimals != wcur->value.decimals) do_resize = True;

    if (((wnew->value.datatype == XcLval) || (wnew->value.datatype == XcHval)) &&
      ((wnew->value.lower_bound.lval != wcur->value.lower_bound.lval) ||
	(wnew->value.upper_bound.lval != wcur->value.upper_bound.lval) )) {
	do_resize = True;
    }
    else if ((wnew->value.datatype == XcFval) &&
      ((wnew->value.lower_bound.fval != wcur->value.lower_bound.fval) ||
	(wnew->value.upper_bound.fval != wcur->value.upper_bound.fval) )) {
	do_resize = True;
    }
    if (do_resize) {
	Resize(new);
	do_redisplay = True;
    }

    DPRINTF(("Byte: done SetValues\n"));
    return do_redisplay;
}

static XtGeometryResult QueryGeometry(Widget w, XtWidgetGeometry *proposed,
  XtWidgetGeometry *answer)
  /*************************************************************************
   * Query Geometry: This function is the widget's query_geometry method.  *
   *   It simply checks the proposed size and returns the appropriate value*
   *   based on the proposed size.  If the proposed size is greater than   *
   *   the maximum appropriate size for this widget, QueryGeometry returns *
   *   the recommended size.                                               *
   *************************************************************************/
{
    ByteWidget wb = (ByteWidget)w;
  /****** Set the request mode mask for the returned answer */
    answer->request_mode = CWWidth | CWHeight;

  /****** Set the recommended size */
    answer->width=(wb->core.width > MAX_BY_WIDTH) ? MAX_BY_WIDTH : wb->core.width;
    answer->height=(wb->core.height > MAX_BY_HEIGHT)
      ? MAX_BY_HEIGHT : wb->core.height;

  /****** Check the proposed dimensions. If the proposed size is larger than
	  appropriate, return the recommended size.  */
    if (((proposed->request_mode & (CWWidth | CWHeight)) == (CWWidth | CWHeight))
      && proposed->width == answer->width && proposed->height == answer->height)
      return XtGeometryYes;
    else if (answer->width == wb->core.width && answer->height == wb->core.height)
      return XtGeometryNo;
    else
      return XtGeometryAlmost;
}

static void Destroy(Widget w)
  /*************************************************************************
 * Destroy: This function is the widget's destroy method.  It simply     *
 *   releases any server resources acquired during the life of the widget*
 *************************************************************************/
{
    ByteWidget wb = (ByteWidget)w;

    if (wb->byte.interval > 0) XtRemoveTimeOut (wb->byte.interval_id);
}

static void Get_value(XtPointer client_data, XtIntervalId *id)
  /*************************************************************************
   * Get Value: This function is the time out procedure called at          *
   *   XcNinterval intervals.  It calls the application registered callback*
   *   function to get the latest value and updates the Byte display   *
   *   accordingly.                                                        *
   *************************************************************************/
{
    static XcCallData call_data;
    Widget w = (Widget)client_data;
    ByteWidget wb = (ByteWidget)client_data;

  /****** Get the new value by calling the application's callback if it exists */
    if (wb->byte.update_callback == NULL) return;

  /****** Re-register this TimeOut procedure for the next interval */
    if (wb->byte.interval > 0)
      wb->byte.interval_id =
	XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)w),
	  wb->byte.interval, Get_value, client_data);

  /****** Set the current value and datatype before calling the callback */
    call_data.dtype = wb->value.datatype;
    call_data.decimals = wb->value.decimals;
    if ((wb->value.datatype == XcLval) || (wb->value.datatype == XcHval))
      call_data.value.lval = wb->value.val.lval;
    else if (wb->value.datatype == XcFval)
      call_data.value.fval = wb->value.val.fval;
    XtCallCallbacks((Widget)w, XcNupdateCallback, &call_data);

  /****** Update the new value, update the Byte display */
    if ((wb->value.datatype == XcLval) || (wb->value.datatype == XcHval))
      wb->value.val.lval = call_data.value.lval;
    else if (wb->value.datatype == XcFval)
      wb->value.val.fval = call_data.value.fval;

    if (XtIsRealized((Widget)w))
      Draw_display(w, XtDisplay(w), XtWindow(w), wb->control.gc);
}

void XcBYUpdateValue(Widget w, XcVType *value)
  /*************************************************************************
   * Xc Byte Update Value:  This convenience function is called by the     *
   *   application in order to update the value (a little quicker than     *
   *   using XtSetArg/XtSetValue).  The application passes the new value   *
   *   to be updated with.                                                 *
   *************************************************************************/
{
    ByteWidget wb = (ByteWidget)w;

#ifdef USE_CORE_VISIBLE
    if (!wb->core.visible) return;
#endif

  /****** Update the new value, then update the Byte display. */
    if (value != NULL) {
	if ((wb->value.datatype == XcLval) || (wb->value.datatype == XcHval))
	  wb->value.val.lval = value->lval;
	else if (wb->value.datatype == XcFval)
	  wb->value.val.fval = value->fval;

	if (XtIsRealized((Widget)w))
	  Draw_display(w, XtDisplay(w), XtWindow(w), wb->control.gc);
    }
}

void XcBYUpdateByteForeground(Widget w, Pixel pixel)
  /*************************************************************************
   * Xc Byte Update Byte Foreground: This convenience function is called   *
   *   by the application in order to update the value (a little quicker   *
   *   than using XtSetArg/XtSetValue).  The application passes the new    *
   *   value to be updated with.                                           *
   *************************************************************************/
{
    ByteWidget wb = (ByteWidget)w;

  /* Local variables */
#ifdef USE_CORE_VISIBLE
    if (!wb->core.visible) return;
#endif

  /* Update the new value, then update the Byte display. */
    if (wb->byte.byte_foreground != pixel) {
	wb->byte.byte_foreground = pixel;
	if (XtIsRealized((Widget)w))
	  Draw_display(w, XtDisplay(w), XtWindow(w), wb->control.gc);
    }
}

static void Draw_display(Widget w, Display *display,
  Drawable drawable, GC gc)
  /*************************************************************************
   * Draw Display: This function redraws the Byte.                         *
   *************************************************************************/
  /* KE: XtNborderWidth is in core and core will draw a border
   positioned at x,y with width = core->width+2*borderWidth and height
   = core->height+2*borderWidth.  This means the actual widget size is
   different from the core size, causing problems in MEDM.  So we draw
   the border ourselves, rather than using borderWidth.
   XtNborderWidth should be set to 0. (Its default is 1.)  The lines
   in Byte are always 1 pixel, independent of borderWidth. */
{
    ByteWidget wb = (ByteWidget)w;
    Boolean reverse;
    int  iVal, i, nSeg, iBit, isSet;
    double delta;
    int x1, y1, x2, y2, i1, i2, iDelta;
    unsigned int width, height;

  /* printf("BY: Draw display2\n"); */
#if DEBUG_SIZE
    printf("Draw_display: x=%d y=%d width=%u height=%u\n"
      "  Face: x=%d y=%d width=%u height=%u\n",
      wb->core.x,wb->core.y,wb->core.width,wb->core.height,
      wb->byte.face.x,wb->byte.face.y,
      wb->byte.face.width,wb->byte.face.height);
#endif

    
  /* Define some parameters */
    width = wb->byte.face.width;
    height = wb->byte.face.height;
    x1 = wb->byte.face.x + 1;
    y1 = wb->byte.face.y + 1;
    x2 = width-1;
    y2 = height-1;
    if(wb->byte.ebit > wb->byte.sbit) {
	reverse = True;
	nSeg = wb->byte.ebit - wb->byte.sbit +1;
    } else {
	reverse = False;
	nSeg = wb->byte.sbit - wb->byte.ebit +1;
    }

  /* Check if large enough to be meaningful */
    if((wb->byte.orient == XcHoriz && ((int)width <= nSeg + 1 || height <= 3)) ||
      ((wb->byte.orient == XcVert && ((int)height <= nSeg + 1 || width <= 3)))) {
      /* Only enough room for the lines, fill with black and return */
	XSetForeground(display, gc, XBlackPixel(display, 0));
	XFillRectangle(display, drawable, gc, x1, y1, width, height);
	return;
    }
    
  /* Fill the background */
    XSetForeground(display, gc, wb->byte.byte_background);
    XFillRectangle(display, drawable, gc, x1, y1, width, height);

    /* Get the value as an integer */
    if ((wb->value.datatype == XcLval) || (wb->value.datatype == XcHval)) {
	iVal = wb->value.val.lval;
    } else if (wb->value.datatype == XcFval) {
	iVal = (int)wb->value.val.fval;
    }

  /* Draw the segments */
    if (wb->byte.orient == XcVert) {
      /* Vertical */
	i1 = 0;
	delta = (double)(height-1)/((double)nSeg);
	for(i=0; i < nSeg; i++) {
	    i2 = (int)((i + 1) * delta + .5);
	    if(i2 > (int)height) i2 = height;
	  /* Draw the foreground color if the bit is 1 */
	    if(reverse) iBit = wb->byte.sbit + i;
	    else iBit = wb->byte.sbit - i;
	    isSet = iVal & (1<<iBit);
	    iDelta = i2 - i1 - 1;
	    if(isSet && iDelta > 0) {
		XSetForeground(display, gc, wb->byte.byte_foreground);
		XFillRectangle(display, drawable, gc, x1, i1+1, width-2, iDelta);
	    }
	  /* Draw the line at the end of the segment */
	    if(i < nSeg - 1) {
		XSetForeground(display, gc, XBlackPixel(display, 0));
		XDrawLine(display, drawable, gc, x1, i2, x2, i2);
	    }
	    i1 = i2;
	}
    } else {
      /* Horizontal */
	i1 = 0;
	delta = (double)(width-1)/((double)nSeg);
	for(i=0; i < nSeg; i++) {
	    i2 = (int)((i + 1) * delta + .5);
	    if(i2 > (int)width) i2 = width;
	  /* Draw the foreground color if the bit is 1 */
	    if(reverse) iBit = wb->byte.sbit + i;
	    else iBit = wb->byte.sbit - i;
	    isSet = iVal & (1<<iBit);
	    iDelta = i2 - i1 - 1;
	    if(isSet && iDelta > 0) {
		XSetForeground(display, gc, wb->byte.byte_foreground);
		XFillRectangle(display, drawable, gc, i1+1, y1, iDelta, height-2);
	    }
	  /* Draw the line at the end of the segment */
	    if(i < nSeg - 1) {
		XSetForeground(display, gc, XBlackPixel(display, 0));
		XDrawLine(display, drawable, gc, i2, y1, i2, y2);
	    }
	    i1 = i2;
	}
    }
    
  /* Draw the border */
    XSetForeground(display, gc, XBlackPixel(display, 0));
    XDrawRectangle(display, drawable, gc, 0, 0, x2, y2);
}

#if 0
/* Not used */
static void Print_bounds(Widget w, char *upper, char *lower)
  /*************************************************************************
   * Print Bounds: This is a utility function used by the Redisplay and    *
   *   Resize methods to print the upper and lower bound values as strings *
   *   for displaying and resizing purposes.                               *
   *************************************************************************/
{
    ByteWidget wb = (ByteWidget)w;

    if (wb->value.datatype == XcLval) {
	cvtLongToString(wb->value.upper_bound.lval, upper);
	cvtLongToString(wb->value.lower_bound.lval, lower);
    }
    else if (wb->value.datatype == XcHval) {
	cvtLongToHexString(wb->value.upper_bound.lval, upper);
	cvtLongToHexString(wb->value.lower_bound.lval, lower);
    }
    else if (wb->value.datatype == XcFval) {
	cvtFloatToString(wb->value.upper_bound.fval, upper,
	  (unsigned short)wb->value.decimals);
	cvtFloatToString(wb->value.lower_bound.fval, lower,
	  (unsigned short)wb->value.decimals);
    }
}
#endif
