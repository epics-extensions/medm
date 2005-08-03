/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/* Marquee.c -- Source file for Marquee widget */

#define DEBUG_GEN 0
#define DEBUG_BG 0
#define DEBUG_GC 0

#include <Xm/XmP.h>
#include <X11/StringDefs.h>
#include <X11/extensions/shape.h>
#include <X11/Xmu/Converters.h>
#include <stdio.h>
#include <math.h>

#include "MarqueeP.h"

#define offset(field) XtOffsetOf(MarqueeRec, field)

static XtResource resources[] = {
    {XtNblinkTime, XtCBlinkTime, XtRInt, sizeof(int),
     offset(marquee.blink_time), XtRImmediate, (XtPointer)500},
    {XtNtransparent, XtCTransparent, XtRBoolean, sizeof(Boolean),
     offset(marquee.transparent), XtRImmediate, (XtPointer)TRUE},
};

#undef offset

/* Function prototypes */

static void Initialize(Widget greq, Widget gnew,
  ArgList args, Cardinal *nargs);
static void Resize(Widget gw);
static void Realize(Widget gw, XtValueMask *valueMask,
  XSetWindowAttributes *attrs);
static void Destroy(Widget gw);
static Boolean SetValues(Widget gcur, Widget greq,
  Widget gnew, ArgList args, Cardinal *nargs);

/* Core methods not needed by Marquee:
 *    static void ClassInitialize();
 *    static void Resize(Widget gw);  [Timer proc redraws]
 */

static void makeGC(Widget gw);
static void drawMarquee(XtPointer clientData, XtIntervalId *id);

MarqueeClassRec marqueeClassRec = {
    {
      /* Core fields */
	(WidgetClass)&xmPrimitiveClassRec, /* superclass   */
	"Marquee",                /* class_name            */
	sizeof(MarqueeRec),       /* size                  */
	NULL,                     /* class_initialize      */
	NULL,                     /* class_part_initialize */
	FALSE,                    /* class_inited          */
	Initialize,               /* initialize            */
	NULL,                     /* initialize_hook       */
	Realize,                  /* realize               */
	NULL,                     /* actions               */
	0,                        /* num_actions           */
	resources,                /* resources             */
	XtNumber(resources),      /* num_resources         */
	NULLQUARK,                /* xrm_class             */
	TRUE,                     /* compress_motion       */
	TRUE,                     /* compress_exposure     */
	TRUE,                     /* compress_enterleave   */
	FALSE,                    /* visible_interest      */
	Destroy,                  /* destroy               */
	Resize,                   /* resize                */
	NULL,                     /* expose                */
	SetValues,                /* set_values            */
	NULL,                     /* set_values_hook       */
	NULL,                     /* set_values_almost     */
	NULL,                     /* get_values_hook       */
	NULL,                     /* accept_focus          */
	XtVersion,                /* version               */
	NULL,                     /* callback_private      */
	NULL,                     /* tm_table              */
	XtInheritQueryGeometry,   /* query_geometry        */
    },
    {
      /* Primitive class fields */
	(XtWidgetProc)_XtInherit, /* border_highlight   */
	(XtWidgetProc)_XtInherit, /* border_unhighlight */
	XtInheritTranslations,    /* translations       */
#if 1
	NULL,                     /* arm_and_activate   */
#else
	(XtWidgetProc)_XtInherit, /* arm_and_activate   */
#endif
	NULL,                     /* syn resources      */
	0,                        /* num_syn_resources  */
	NULL,                     /* extension          */
    },
    {
      /* Marquee class fields */
	0,                        /* dummy              */
    },
};

WidgetClass marqueeWidgetClass = (WidgetClass)&marqueeClassRec;

#if 0
/* Not needed for Marquee */
static void ClassInitialize()
{
    XtAddConverter(XtRString, XtRBackingStore, XmuCvtStringToBackingStore,
      NULL, 0 );
}
#endif

static void Initialize(Widget greq, Widget gnew,
  ArgList args, Cardinal *nargs)
{
    MarqueeWidget w = (MarqueeWidget)gnew;
    int shape_event_base, shape_error_base;

#if DEBUG_BG > 1
    MarqueeWidget wreq = (MarqueeWidget)greq;
    printf("Initialize: req: bg=%8x fg=%8x\n"
      "            new: bg=%8x fg=%8x\n",
      wreq->core.background_pixel,wreq->primitive.foreground,
      w->core.background_pixel,w->primitive.foreground);
#endif

  /* Make the GCs */
    makeGC(gnew);

    w->marquee.interval_id = 0;
    w->marquee.first_state = True;

  /* Override transparent if the shape extension is not available */
    if(w->marquee.transparent && !XShapeQueryExtension(XtDisplay(w),
      &shape_event_base,
      &shape_error_base)) {
	w->marquee.transparent = False;
    }
#if DEBUG_GEN
    printf("Initialize: BorderColor=%d BorderWidth=%d\n",
      w->core.border_pixel,w->core.border_width);
    printf("            background=%x foreground=%x\n",
      w->core.background_pixel,w->primitive.foreground);
#endif
}

static void Resize(Widget gw)
{
    MarqueeWidget w = (MarqueeWidget)gw;
    XGCValues xgcv;
    GC gc;
    Pixmap pixmap;
    int width, height, pwidth, pheight, border;
    int i;

    if(XtIsRealized(gw)) {
	XClearWindow(XtDisplay(w), XtWindow(w));
	if(w->marquee.transparent) {
	    border = w->core.border_width;
	    pwidth = w->core.width + 2*border;
	    pheight = w->core.height + 2*border;

	  /* Create a temporary pixmap of depth 1 for the mask */
	    pixmap = XCreatePixmap(XtDisplay(w), XtWindow(w),
	      (Dimension)pwidth, (Dimension)pheight, 1);

	  /* Create a GC */
	    gc = XCreateGC(XtDisplay(w), pixmap, 0, &xgcv);

	  /* Fill the pixmap with 0's */
	    XSetForeground(XtDisplay(w), gc, 0);
	    XFillRectangle(XtDisplay(w), pixmap,
	      gc, 0, 0,  (Dimension)pwidth, (Dimension)pheight);

	  /* Set 1's where we want things to show */
	    XSetForeground(XtDisplay(w), gc, 1);
	    width = w->core.width - 1;
	    height = w->core.height - 1;
	  /* Marquee */
	    if(width > 0 && height > 0) {
		XDrawRectangle(XtDisplay(w), pixmap, gc,
		  border, border, (Dimension)width, (Dimension)height);
	    }
	  /* Border */
	    for(i=0; i < border; i++) {
		int bwidth = pwidth - 2*i - 1;
		int bheight = pheight - 2*i - 1;

		if(bwidth > 0 && bheight > 0) {
		    XDrawRectangle(XtDisplay(w), pixmap, gc,
		      i, i, pwidth - 2*i - 1, pheight - 2*i - 1);
		}
	    }

	  /* Combine the mask */
	    XShapeCombineMask(XtDisplay(w), XtWindow(w), ShapeBounding,
	      -border, -border, pixmap, ShapeSet);

	  /* Free the pixmap and gc */
	    XFreePixmap(XtDisplay(w), pixmap);
	    XFreeGC(XtDisplay(w), gc);
	}
    }
#if DEBUG_GEN
    printf("Resize: BorderColor=%d BorderWidth=%d\n",
      w->core.border_pixel,w->core.border_width);
    printf("            background=%x foreground=%x\n",
      w->core.background_pixel,w->primitive.foreground);
#endif
}

static void Realize(Widget gw, XtValueMask *valueMask,
  XSetWindowAttributes *attrs)
{
    MarqueeWidget w = (MarqueeWidget)gw;

    XtCreateWindow(gw, (unsigned)InputOutput, (Visual *)CopyFromParent,
      *valueMask, attrs );
    Resize(gw);
  /* Call the timer proc with a zero timeout.  It will reinstall
     itself with the blinkTime timeout.  */
    if(w->marquee.blink_time > 0) {
	w->marquee.interval_id =
	  XtAppAddTimeOut(XtWidgetToApplicationContext(gw),
	    0, drawMarquee, (XtPointer)gw);
    }
}

static void Destroy(Widget gw)
{
    MarqueeWidget w = (MarqueeWidget)gw;

    if(w->marquee.interval_id)
      XtRemoveTimeOut(w->marquee.interval_id);
    XtReleaseGC(gw, w->marquee.gc1);
    XtReleaseGC(gw, w->marquee.gc2);
}

#if 0
/* Not needed for Marquee */
static void Redisplay(Widget gw, XEvent event, Region region)
{
    MarqueeWidget w;

    w = (MarqueeWidget)gw;
#if DEBUG_GEN
    printf("Redisplay: BorderColor=%d BorderWidth=%d\n",
      w->core.border_pixel,w->core.border_width);
    printf("            background=%x foreground=%x\n",
      w->core.background_pixel,w->primitive.foreground);
    printf("            width=%d height=%d\n",
      w->core.width,w->core.height);
#endif
}
#endif

static Boolean SetValues(Widget gcur, Widget greq,
  Widget gnew, ArgList args, Cardinal *nargs)
{
    MarqueeWidget wcur = (MarqueeWidget)gcur;
    MarqueeWidget wnew = (MarqueeWidget)gnew;
    Boolean do_redisplay = False;


  /* KE: These have not been tested */

  /* foreground and background */
    if(wcur->primitive.foreground != wnew->primitive.foreground ||
      wcur->core.background_pixel != wnew->core.background_pixel) {
        XtReleaseGC(gcur, wcur->marquee.gc1);
        XtReleaseGC(gcur, wcur->marquee.gc2);
	makeGC(gnew);
        do_redisplay = True;
    }

  /* blinkTime */
    if(wcur->marquee.blink_time != wnew->marquee.blink_time) {
	if(wcur->marquee.interval_id) {
	    XtRemoveTimeOut(wcur->marquee.interval_id);
	    wnew->marquee.interval_id = 0;
	}
	if(wnew->marquee.blink_time > 0) {
	    wnew->marquee.interval_id =
	      XtAppAddTimeOut(XtWidgetToApplicationContext(gnew),
		wnew->marquee.blink_time, drawMarquee, (XtPointer)gnew);
	}
    }

  /* transparent */
    if(wcur->marquee.transparent != wnew->marquee.transparent) {
	Resize(gnew);
        do_redisplay = True;
    }

  /* borderWidth */
    if(wcur->core.border_width != wnew->core.border_width) {
	Resize(gnew);
        do_redisplay = True;
    }

#if DEBUG_GEN
    printf("SetValues: Cur: Background=%d Foreground=%d gc1=%x gc2=%x\n",
      wcur->core.background_pixel,wcur->primitive.foreground,
      wcur->marquee.gc1,wcur->marquee.gc2);
    printf("           New: Background=%d Foreground=%d gc1=%x gc2=%x\n",
      wnew->core.background_pixel,wnew->primitive.foreground,
      wnew->marquee.gc1,wnew->marquee.gc2);
#endif

    return do_redisplay;
}

/* Routine to make GC's */
static void makeGC(Widget gw)
{
    MarqueeWidget w = (MarqueeWidget)gw;
    XtGCMask valuemask;
    XGCValues gcv;

    gcv.foreground = w->primitive.foreground;
    gcv.background = w->core.background_pixel;
    gcv.line_style = LineDoubleDash;
    gcv.line_width = 1;     /* Insures uniformity.  WIN32 needs it. */
    valuemask = GCForeground | GCBackground | GCLineStyle | GCLineWidth;

    w->marquee.gc1 = XtGetGC(gw, valuemask, &gcv);

  /* Reverse the foreground and background for gc2 */
    gcv.foreground = w->core.background_pixel;
    gcv.background = w->primitive.foreground;
    w->marquee.gc2 = XtGetGC(gw, valuemask, &gcv);
#if DEBUG_GC
    {
	XtGCMask valuemask;
	XGCValues gcv1,gcv2;

	valuemask = GCForeground | GCBackground | GCLineStyle;
	XGetGCValues(XtDisplay(w),w->marquee.gc1,valuemask,&gcv1);
	XGetGCValues(XtDisplay(w),w->marquee.gc2,valuemask,&gcv2);
	printf("makeGC: gc1=%x fg=%x bg=%x "
	  "lineStyle=%d [LineDoubleDash=%d]\n",
	  w->marquee.gc1,gcv1.foreground,gcv1.background,
	  gcv1.line_style,LineDoubleDash);
	printf("        gc2=%x fg=%x bg=%x "
	  "lineStyle=%d [LineDoubleDash=%d]\n",
	  w->marquee.gc2,gcv2.foreground,gcv2.background,
	  gcv2.line_style,LineDoubleDash);
    }
#endif
#if DEBUG_BG
    printf("makeGC: w=%x bg=%8x fg=%8x gc1=%x gc2=%x\n",
      w,w->core.background_pixel,w->primitive.foreground,
      w->marquee.gc1,w->marquee.gc2);

#endif
}

/* Timer proc */
static void drawMarquee(XtPointer clientData, XtIntervalId *id)
{
    MarqueeWidget w = (MarqueeWidget)clientData;
    Dimension width, height;

#if DEBUG_GC
    {
	XtGCMask valuemask;
	XGCValues gcv1,gcv2;

	valuemask = GCForeground | GCBackground | GCLineStyle;
	XGetGCValues(XtDisplay(w),w->marquee.gc1,valuemask,&gcv1);
	XGetGCValues(XtDisplay(w),w->marquee.gc2,valuemask,&gcv2);
	printf("drawMarquee: gc1=%x fg=%x bg=%x "
	  "lineStyle=%d [LineDoubleDash=%d]\n",
	  w->marquee.gc1,gcv1.foreground,gcv1.background,
	  gcv1.line_style,LineDoubleDash);
	printf("             gc2=%x fg=%x bg=%x "
	  "lineStyle=%d [LineDoubleDash=%d]\n",
	  w->marquee.gc2,gcv2.foreground,gcv2.background,
	  gcv2.line_style,LineDoubleDash);
    }
#endif
    if(XtIsRealized((Widget)w)) {
	width = w->core.width?w->core.width-1:0;
	height = w->core.height?w->core.height-1:0;
	if(w->marquee.first_state) {
	    XDrawRectangle(XtDisplay(w), XtWindow(w), w->marquee.gc1,
	      0, 0, width, height);
	} else {
	    XDrawRectangle(XtDisplay(w), XtWindow(w), w->marquee.gc2,
	      0, 0, width, height);
	}
	w->marquee.first_state = !w->marquee.first_state;
    }

  /* Call the timer proc again */
    if(w->marquee.blink_time > 0) {
	w->marquee.interval_id =
	  XtAppAddTimeOut(XtWidgetToApplicationContext((Widget) w),
	    w->marquee.blink_time, drawMarquee, (XtPointer)w);
    }
}
