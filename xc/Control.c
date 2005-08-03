/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*******************************************************************
 FILE:		Control.c
 CONTENTS:	Definitions for structures, methods, and actions of the
		Control widget.
 AUTHOR:	Paul D. Johnston
 HISTORY:
 Date		Action
 ---------	------------------------------------
 5/23/92	Changed the widget class name so that it is preceded
		by 'xc' with the first major word capitalized.
 5/18/92	Changed Label resource processing so that it copies a " "
		to the label pointer instead of "NO LABEL".
 10/22/91	Created.
********************************************************************/

#include <stdio.h>

/* Xlib includes */
#include <X11/Xlib.h>

/* Xt includes */
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#ifndef _HPUX_SOURCE
#ifdef X11R3
#include <X11/Xmu.h>
#else
#include <X11/Xmu/Xmu.h>
#endif
#endif

#if !defined(NICE_SHADES) && !defined(EASY_SHADES)
/* Use Motif colors */
#include <Xm/Xm.h>
#endif

/* Widget includes */
#include "ControlP.h"	/* (includes Control.h also) */

/* Macro redefinition for offset. */
#define offset(field) XtOffset(ControlWidget, field)

/* Constants used in 3D rectangle color generation. */
#define MAX_RGB	65280
#define SHADE_INTENSITY	((unsigned short)(MAX_RGB / 4))

/* Data for the bitmap used for the 3D shading effect. */
#define shade_width 2
#define shade_height 2

/* Function prototypes for internal widget methods */
static void Initialize(Widget request, Widget new,
  ArgList args, Cardinal *nargs);
static Boolean SetValues(Widget cur, Widget req,
  Widget new, ArgList args, Cardinal *nargs);
static void Destroy(Widget w);

/* Define the widget's resource list */
static XtResource resources[] =
{
    {
	XcNcontrolBackground,
	XtCColor,
	XtRPixel,
	sizeof(Pixel),
	offset(control.background_pixel),
	XtRString,
	"gray"
    },
    {
	XcNlabelColor,
	XtCColor,
	XtRPixel,
	sizeof(Pixel),
	offset(control.label_pixel),
	XtRString,
	XtDefaultForeground
    },
    {
	XcNlabel,
	XcCLabel,
	XtRString,
	sizeof(String),
	offset(control.label),
	XtRString,
	"  "
    },
    {
	XtNfont,
	XtCFont,
	XtRFontStruct,
	sizeof(XFontStruct *),
	offset(control.font),
	XtRString,
	XtDefaultFont
    },
    {
	XcNshadeDepth,
	XcCShadeDepth,
	XtRInt,
	sizeof(int),
	offset(control.shade_depth),
	XtRString,
	"2"
    },

};

/* Widget Class Record initialization */
ControlClassRec controlClassRec =
{
    {
      /* core_class part */
        (WidgetClass) &widgetClassRec,   /* superclass */
        "Control",                       /* class_name */
        sizeof(ControlRec),              /* widget_size */
        NULL,                            /* class_initialize */
        NULL,                            /* class_part_initialize */
        FALSE,                           /* class_inited */
        Initialize,                      /* initialize */
        NULL,                            /* initialize_hook */
        XtInheritRealize,                /* realize */
        NULL,                            /* actions */
        0,                               /* num_actions */
        resources,                       /* resources */
        XtNumber(resources),             /* num_resources */
        NULLQUARK,                       /* xrm_class */
        TRUE,                            /* compress_motion */
        TRUE,                            /* compress_exposure */
        TRUE,                            /* compress_enterleave */
        TRUE,                            /* visible_interest */
        Destroy,                         /* destroy */
        NULL,                            /* resize */
        NULL,                            /* expose */
        SetValues,                       /* set_values */
        NULL,                            /* set_values_hook */
        XtInheritSetValuesAlmost,        /* set_values_almost */
        NULL,                            /* get_values_hook */
        NULL,                            /* accept_focus */
        XtVersion,                       /* version */
        NULL,                            /* callback_private */
        NULL,                            /* tm_table */
        NULL,                            /* query_geometry */
        NULL,                            /* display_accelerator */
        NULL,                            /* extension */
    },
    {
      /* Control class part */
        0,                               /* dummy_field */
    },
};

WidgetClass xcControlWidgetClass = (WidgetClass)&controlClassRec;

/* Widget method function definitions */

/*******************************************************************
 NAME:		Initialize.
 DESCRIPTION:
   This is the initialize method for the Control widget.  It
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
    ControlWidget wnew = (ControlWidget)new;
    Display *display = XtDisplay(new);
    XGCValues values;
    XtGCMask mask;
    static char dash_list[2] = { 1, 1 };


#if defined(NICE_SHADES)
  /* These macros are used for calculating the 3D shade colors. */
#define COLOR_ADD(member) \
  if (((int)bg.member + (int)SHADE_INTENSITY) <= MAX_RGB) \
    wnew->control.shade1.member = bg.member + SHADE_INTENSITY; \
  else  wnew->control.shade1.member = MAX_RGB;

#define COLOR_SUBTRACT(member) \
  if (((int)bg.member - (int)SHADE_INTENSITY) >= 0) \
    wnew->control.shade2.member = bg.member - SHADE_INTENSITY; \
  else  wnew->control.shade2.member = 0;

  /*
   * Determine whether or not a backgroud color resource has been set for
   * this widget, and if so, what its RGB values are. If no color has been
   * established, set gray as its background by default.
   */
    if (wnew->control.background_pixel)
	{
	    bg.pixel = wnew->control.background_pixel;
	    XQueryColor(display, wnew->core.colormap, &bg);
	}
    else
	{
	    if (XAllocNamedColor(display, wnew->core.colormap,
	      "gray", &bg, &exact))
	      wnew->control.background_pixel = bg.pixel;
	    else
	      XtWarning("Control: unable to alloc default bg\n");
	}

  /*
   * Calculate the 3D rectangle shades based on the background color RGB
   * values, and allocate the corresponding colors.
   */
    COLOR_ADD(red);
    COLOR_ADD(green);
    COLOR_ADD(blue);
    COLOR_SUBTRACT(red);
    COLOR_SUBTRACT(green);
    COLOR_SUBTRACT(blue);
    if (XAllocColor(display, wnew->core.colormap, &(wnew->control.shade1)) == 0)
	{
	    XtWarning("Control: unable to alloc shade 1 color\n");
	    wnew->control.shade1.pixel = WhitePixel(display, DefaultScreen(display));
	}
    if (XAllocColor(display, wnew->core.colormap, &(wnew->control.shade2)) == 0)
	{
	    XtWarning("Control: unable to alloc shade 2 color\n");
	    wnew->control.shade2.pixel = BlackPixel(display, DefaultScreen(display));
	}
#elif defined(EASY_SHADES)
#else
  /* Use Motif colors */
    Pixel fg, select;

    XmGetColors(DefaultScreenOfDisplay(display), wnew->core.colormap, wnew->core.background_pixel,
      &fg, &wnew->control.shade1, &wnew->control.shade2, &select);
#endif	/* NICE_SHADES */


  /*
   * Validate public instance variable settings.
   */
    if (strlen(wnew->control.label) == 0)
	{
	    XtWarning("Control: invalid or missing label string.");
	    strcpy(wnew->control.label, " ");
	}

    if ((wnew->control.shade_depth < MIN_SHADE_DEPTH) ||
      (wnew->control.shade_depth > MAX_SHADE_DEPTH))
	{
	    XtWarning("Control: invalid shadeDepth specification.");
	    wnew->control.shade_depth = 3;
	}


  /* Create the GC used by all subclasses for drawing in this widget. */
    values.graphics_exposures = False;
    values.foreground = wnew->control.label_pixel;
    values.background = wnew->control.background_pixel;
    values.font = wnew->control.font->fid;
    mask = GCForeground | GCBackground | GCFont | GCGraphicsExposures;
    if ((wnew->control.gc = XCreateGC(display,
      RootWindowOfScreen(XtScreen(wnew)),
      mask, &values)) == NULL)
      XtWarning("Control: couldn't create GC");

  /* Set basic line attributes used in drawing this widget. */
    XSetLineAttributes(display, wnew->control.gc, 0, LineSolid,
      CapButt, JoinRound);
    XSetDashes(display, wnew->control.gc, 0, dash_list, 2);


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
    ControlWidget wnew = (ControlWidget)new;
    ControlWidget wcur = (ControlWidget)cur;
    Boolean do_redisplay = False;


  /* Validate new resource settings. */
  /* Check to see if the labelColor has changed. */
    if (wnew->control.label_pixel != wcur->control.label_pixel)
      do_redisplay = True;

  /* Check for valid shadeDepth setting. */
    if (wnew->control.shade_depth != wcur->control.shade_depth) {
	do_redisplay = True;
	if ((wnew->control.shade_depth < MIN_SHADE_DEPTH) ||
	  (wnew->control.shade_depth > MAX_SHADE_DEPTH)) {
	    XtWarning("Control: invalid shadeDepth setting.");
	    wnew->control.shade_depth = 3;
	}
    }

  /* Check for a valid label */
    if (strcmp(wnew->control.label, wcur->control.label) != 0) {
	if (strlen(wnew->control.label) == 0) {
	    XtWarning("Control: invalid Label setting.");
	    if (wnew->control.label != NULL)
	      strcpy(wnew->control.label, " ");
	}
	do_redisplay = True;
    }

    return do_redisplay;
}

/*******************************************************************
 NAME:		Destroy.
 DESCRIPTION:
   This function is the widget's destroy method.  It simply releases
 any server resources acquired during the life of the widget.
*******************************************************************/

static void Destroy(Widget w)
{
    ControlWidget wc = (ControlWidget)w;

    if (wc->control.gc)
      XFreeGC(XtDisplay(wc), wc->control.gc);
}

/* Functions shared by widget subclasses. */

/***************************************************************************
 FUNCTION:	Point_In_Rect.
 DESCRIPTION:
   This function simply checks to see if the given point is within the
 given rectangle and returns TRUE if it is, or FALSE if it isn't within
 the rectangle.
***************************************************************************/

Boolean Point_In_Rect(int x, int y, int rect_x, int rect_y,
  int width, int height)
/*
 * x       INPUT: The x coordinate of the point
 * y       INPUT: The y coordinate of the point
 * rect_x  INPUT: The x coordinate of the rectangle
 * rect_y  INPUT: The y coordinate of the rectangle
 * width,  INPUT: The rectangle's width
 * height  INPUT: The rectangle's height
 *
 * RETURN: True or False; identifies whether or not the point is in the
 * 	   rectangle.
 */
{
  /* Check x coordinate first */
    if ((x < rect_x) || (x > (rect_x + width)))
      return False;

  /* Check y coordinate next. If its within the rectangle, return True. */
    if ((y < rect_y) || (y > (rect_y + height)))
      return False;

    return True;
}

/*******************************************************************
 NAME:		Rect3d.
 DESCRIPTION:
   This function generates a filled rectangle with a 3D effect in
 the given drawable.  It uses the widget's background color and the
 derived shades (calculated in the Initialize method) to produce the
 3D rectangle.
*******************************************************************/

void Rect3d(Widget w, Display *display, Drawable drawable,
  GC gc, int x, int y, unsigned int width, unsigned int height,
  Type3d type)
{
    ControlWidget wc = (ControlWidget)w;
    int j;
    unsigned long shade1, shade2;
    unsigned int depth = wc->control.shade_depth;

  /* Set basic GC attributes used in drawing the 3D rectangle. */
#if defined(NICE_SHADES)
    shade1 = wc->control.shade1.pixel;
    shade2 = wc->control.shade2.pixel;
#elif defined(EASY_SHADES)
    shade1 = WhitePixel(display, DefaultScreen(display));
    shade2 = BlackPixel(display, DefaultScreen(display));
    XSetLineAttributes(display, gc, 0, LineOnOffDash, CapButt, JoinRound);
#else
    shade1 = wc->control.shade1;
    shade2 = wc->control.shade2;
#endif

  /* Reduce the height and width by 1 to use in drawing lines */
    if(height > 0) height--;
    if(width > 0) width--;

  /* Draw the shadow lines */
    for (j = 0; j < (int)depth; j++) {
	if (type == RAISED)
	  XSetForeground(display, gc, shade1);
	else
	  XSetForeground(display, gc, shade2);
	XDrawLine(display, drawable, gc, x+j, y+j, x+j, y+height-j);
	XDrawLine(display, drawable, gc, x+j, y+j, x+width-j, y+j);

	if (type == RAISED)
	  XSetForeground(display, gc, shade2);
	else
	  XSetForeground(display, gc, shade1);
	XDrawLine(display, drawable, gc, x+j, y+height-j, x+width-j, y+height-j);
	XDrawLine(display, drawable, gc, x+width-j, y+j, x+width-j, y+height-j);
    }

  /* Fill in the background. */
    XSetFillStyle(display, gc, FillSolid);
#if defined(EASY_SHADES)
    XSetLineAttributes(display, gc, 0, LineSolid, CapButt, JoinRound);
#endif
    XSetForeground(display, gc, wc->control.background_pixel);
    if ((width > (2 * depth)) &&  (height > (2 * depth))) {
	XFillRectangle(display, drawable, gc, x+depth,
	  y+depth, width-(2 * depth),
	  height-(2 * depth));
    }
}

/*******************************************************************
 NAME:		ToLower.
 DESCRIPTION:
   Converts a character string to all lowercase for use in the
 resource string conversion routines.
*******************************************************************/

void ToLower(char *source, char *dest)
{
    char ch;

    for (; (ch = *source) != 0; source++, dest++) {
	if ('A' <= ch && ch <= 'Z')
	  *dest = ch - 'A' + 'a';
	else
	  *dest = ch;
    }
    *dest = '\0';

}

/*******************************************************************
 NAME:		CvtStringToOrient.
 DESCRIPTION:
   This function converts resource settings in string form to the
 XcROrient representation type.
*******************************************************************/

void CvtStringToOrient(XrmValuePtr args, Cardinal *nargs,
  XrmValuePtr fromVal, XrmValuePtr toVal)
  /* Args and nargs are unused */
{
    static XcOrient orient;
    char lowerstring[100];

  /* Convert the resource string to lower case for quick comparison */
    ToLower((char *)fromVal->addr, lowerstring);

  /*
   * Compare resource string with valid XcOrient strings and assign to
   * datatype.
   */
    if (strcmp(lowerstring, XcEvert) == 0) {
	orient = XcVert;
	CvtDone(XcOrient, &orient);
    } else if (strcmp(lowerstring, XcEhoriz) == 0) {
	orient = XcHoriz;
	CvtDone(XcOrient, &orient);
    }

  /*
   * If the string is not valid for this resource type, print a warning
   * and do not make the conversion.
   */
    XtStringConversionWarning(fromVal->addr, "XcOrient");
    toVal->addr = NULL;
    toVal->size = 0;
}

/*******************************************************************
 NAME:		Arrow3d.
 DESCRIPTION:
   This function generates a filled arrow polygon with a 3D effect in
 the given drawable.  It uses the widget's background color and the
 derived shades (calculated in Control's Initialize method) to produce the
 3D effect.
*******************************************************************/

void Arrow3d(Widget w, Display *display,Drawable drawable, GC gc,
  XRectangle *bounds, ArrowType orientation,Type3d type)
{
    ControlWidget wc = (ControlWidget)w;
    int j;
    XPoint points[4];
    unsigned long shade1, shade2;

  /* Set basic line attributes used in drawing the 3D arrow. */
#if defined(NICE_SHADES)
    shade1 = wc->control.shade1.pixel;
    shade2 = wc->control.shade2.pixel;
#elif defined(EASY_SHADES)
    shade1 = WhitePixel(display, DefaultScreen(display));
    shade2 = BlackPixel(display, DefaultScreen(display));
    XSetLineAttributes(display, gc, 0, LineOnOffDash, CapButt, JoinRound);
#else
    shade1 = wc->control.shade1;
    shade2 = wc->control.shade2;
#endif

  /* This macro is used to determine the shading color to use. */
#define SET_SHADE( sh1, sh2 ) \
  if (type == RAISED) XSetForeground(display, gc, sh1); \
  else XSetForeground(display, gc, sh2); \

  /*
   * Calculate the arrow vertices based on the bounding rectangle and
   * orientation.
   */
    if (orientation == UP) {
      /* left corner */
	points[0].x = bounds->x;
	points[0].y = bounds->y + bounds->height;
      /* arrow tip */
	points[1].x = (short)(points[0].x + (bounds->width / 2 + 0.5));
	points[1].y = bounds->y;
      /* right corner */
	points[2].x = points[0].x + bounds->width;
	points[2].y = points[0].y;
    } else if (orientation == DOWN) {
      /* left corner */
	points[0].x = bounds->x;
	points[0].y = bounds->y;
      /* arrow tip */
	points[1].x = (short)(points[0].x + (bounds->width / 2 + 0.5));
	points[1].y = bounds->y + bounds->height;
      /* right corner */
	points[2].x = points[0].x + bounds->width;
	points[2].y = points[0].y;
    } else if (orientation == LEFT) {
      /* left corner */
	points[0].x = bounds->x + bounds->width;
	points[0].y = bounds->y + bounds->height;
      /* arrow tip */
	points[1].x = bounds->x;
	points[1].y = (short)(bounds->y + (bounds->height / 2 + 0.5));
      /* right corner */
	points[2].x = points[0].x;
	points[2].y = bounds->y;
    } else if (orientation == RIGHT) {
      /* left corner */
	points[0].x = bounds->x;
	points[0].y = bounds->y;
      /* arrow tip */
	points[1].x = bounds->x + bounds->width;
	points[1].y = (short)(bounds->y + (bounds->height / 2 + 0.5));
      /* right corner */
	points[2].x = points[0].x;
	points[2].y = bounds->y + bounds->height;
    }

  /* left corner */
    points[3].x = points[0].x;
    points[3].y = points[0].y;

  /* Draw the shadow lines */
    for (j = 0; j < wc->control.shade_depth; j++) {
	if (orientation == UP) {
	    SET_SHADE(shade1, shade2);
	    XDrawLine(display, drawable, gc, points[0].x+j, points[0].y-j,
	      points[1].x, points[1].y+j);
	    XDrawLine(display, drawable, gc, points[1].x, points[1].y+j,
	      points[2].x-j, points[2].y-j);
	    SET_SHADE(shade2, shade1);
	    XDrawLine(display, drawable, gc, points[2].x-j, points[2].y-j,
	      points[3].x+j, points[3].y-j);
	} else if (orientation == DOWN) {
	    SET_SHADE(shade2, shade1);
	    XDrawLine(display, drawable, gc, points[0].x+j, points[0].y+j,
	      points[1].x, points[1].y-j);
	    XDrawLine(display, drawable, gc, points[1].x, points[1].y-j,
	      points[2].x-j, points[2].y+j);
	    SET_SHADE(shade1, shade2);
	    XDrawLine(display, drawable, gc, points[2].x-j, points[3].y+j,
	      points[3].x+j, points[3].y+j);
	} else if (orientation == LEFT) {
	    SET_SHADE(shade2, shade1);
	    XDrawLine(display, drawable, gc, points[0].x-j, points[0].y-j,
	      points[1].x+j, points[1].y);
	    SET_SHADE(shade1, shade2);
	    XDrawLine(display, drawable, gc, points[1].x+j, points[1].y,
	      points[2].x-j, points[2].y+j);
	    SET_SHADE(shade2, shade1);
	    XDrawLine(display, drawable, gc, points[2].x-j, points[2].y+j,
	      points[3].x-j, points[3].y-j);
	} else if (orientation == RIGHT) {
	    SET_SHADE(shade1, shade2);
	    XDrawLine(display, drawable, gc, points[0].x+j, points[0].y+j,
	      points[1].x-j, points[1].y);
	    SET_SHADE(shade2, shade1);
	    XDrawLine(display, drawable, gc, points[1].x-j, points[1].y,
	      points[2].x+j, points[2].y-j);
	    SET_SHADE(shade1, shade2);
	    XDrawLine(display, drawable, gc, points[2].x+j, points[2].y-j,
	      points[3].x+j, points[3].y+j);
	}
    }

#if defined(EASY_SHADES)
    XSetLineAttributes(display, gc, 0, LineSolid, CapButt, JoinRound);
#endif
    XSetFillStyle(display, gc, FillSolid);
}
