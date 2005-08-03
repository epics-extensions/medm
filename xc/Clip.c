/*
 * Copyright(c) 1992 Bell Communications Research, Inc. (Bellcore)
 *                        All rights reserved
 * Permission to use, copy, modify and distribute this material for
 * any purpose and without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies, and that the name of Bellcore not be used in advertising
 * or publicity pertaining to this material without the specific,
 * prior written permission of an authorized representative of
 * Bellcore.
 *
 * BELLCORE MAKES NO REPRESENTATIONS AND EXTENDS NO WARRANTIES, EX-
 * PRESS OR IMPLIED, WITH RESPECT TO THE SOFTWARE, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR ANY PARTICULAR PURPOSE, AND THE WARRANTY AGAINST IN-
 * FRINGEMENT OF PATENTS OR OTHER INTELLECTUAL PROPERTY RIGHTS.  THE
 * SOFTWARE IS PROVIDED "AS IS", AND IN NO EVENT SHALL BELLCORE OR
 * ANY OF ITS AFFILIATES BE LIABLE FOR ANY DAMAGES, INCLUDING ANY
 * LOST PROFITS OR OTHER INCIDENTAL OR CONSEQUENTIAL DAMAGES RELAT-
 * ING TO THE SOFTWARE.
 *
 * ClipWidget Author: Andrew Wason, Bellcore, aw@bae.bellcore.com
 */

/*
 * Clip.c - private child of Matrix - used to clip Matrix's textField child
 */

#include <X11/StringDefs.h>
#include <Xm/XmP.h>
#include "ClipP.h"

static char defaultTranslations[] =
"<FocusIn>:			FocusIn()";

static XtResource resources[] = {
    { XmNexposeProc, XmCFunction, XtRFunction, sizeof(XtExposeProc),
      XtOffsetOf(XbaeClipRec, clip.expose_proc),
      XtRFunction, (XtPointer) NULL },
    { XmNfocusCallback, XmCCallback, XmRCallback, sizeof(XtCallbackList),
      XtOffsetOf(XbaeClipRec, clip.focus_callback),
      XtRImmediate, (XtPointer) NULL },
};

/*
 * Declaration of methods
 */
static void ClassPartInitialize(WidgetClass wc);
static void Realize(Widget w, XtValueMask *valueMask,
  XSetWindowAttributes *attributes);
static void Redisplay(Widget w, XEvent *event, Region region);
static void Redraw(Widget w);

/*
 * Public convenience function
 */
void XbaeClipRedraw(Widget w);

/*
 * Clip actions
 */
static void FocusInACT(Widget w, XEvent * event, String *params,
  Cardinal *nparams);

static XtActionsRec actions[] =
{
    {"FocusIn", FocusInACT},
};

XbaeClipClassRec xbaeClipClassRec = {
    {
      /* core_class fields */
      /* superclass        */ (WidgetClass) &xmPrimitiveClassRec,
    /* class_name          */ "XbaeClip",
    /* widget_size         */ sizeof(XbaeClipRec),
    /* class_initialize    */ NULL,
    /* class_part_initialize */ ClassPartInitialize,
    /* class_inited        */ False,
    /* initialize          */ NULL,
    /* initialize_hook     */ NULL,
    /* realize             */ Realize,
    /* actions             */ actions,
    /* num_actions         */ XtNumber(actions),
    /* resources           */ resources,
    /* num_resources       */ XtNumber(resources),
    /* xrm_class           */ NULLQUARK,
    /* compress_motion     */ True,
    /* compress_exposure   */ XtExposeCompressSeries |
      XtExposeGraphicsExpose |
      XtExposeNoExpose,
    /* compress_enterleave */ True,
    /* visible_interest    */ False,
    /* destroy             */ NULL,
    /* resize              */ NULL,
    /* expose              */ Redisplay,
    /* set_values          */ NULL,
    /* set_values_hook     */ NULL,
    /* set_values_almost   */ XtInheritSetValuesAlmost,
    /* get_values_hook     */ NULL,
    /* accept_focus        */ NULL,
    /* version             */ XtVersion,
    /* callback_private    */ NULL,
    /* tm_table            */ defaultTranslations,
    /* query_geometry      */ NULL,
    /* display_accelerator */ NULL,
    /* extension           */ NULL
    },
  /* primitive_class fields */
    {
      /* border_highlight  */ NULL,
    /* border_unhighlight  */ NULL,
    /* translations        */ NULL,
    /* arm_and_activate    */ NULL,
    /* syn_resources       */ NULL,
    /* num_syn_resources   */ 0,
    /* extension           */ NULL
    },
  /* clip_class fields */
    {
      /* redraw            */ Redraw,
    /* extension           */ NULL,
    }
};

WidgetClass xbaeClipWidgetClass = (WidgetClass) & xbaeClipClassRec;


static void
ClassPartInitialize(WidgetClass wc)
{
    XbaeClipWidgetClass cwc = (XbaeClipWidgetClass)wc;
    register XbaeClipWidgetClass super =
      (XbaeClipWidgetClass) cwc->core_class.superclass;

  /*
   * Allow subclasses to inherit our redraw method
   */
    if (cwc->clip_class.redraw == XbaeInheritRedraw)
      cwc->clip_class.redraw = super->clip_class.redraw;
}

static void
Realize(Widget w, XtValueMask *valueMask, XSetWindowAttributes *attributes)
{
    XbaeClipWidget cw = (XbaeClipWidget)w;
  /*
   * Don't call our superclasses realize method, because Primitive sets
   * bit_gravity and do_not_propagate
   */
    XtCreateWindow((Widget)cw, InputOutput, CopyFromParent,
      *valueMask, attributes);
}

static void
Redisplay(Widget w, XEvent *event, Region region)
{
    XbaeClipWidget cw = (XbaeClipWidget)w;

    if (cw->clip.expose_proc)
      cw->clip.expose_proc((Widget)cw, event, region);
}

/*
 * Clip redraw method
 */
static void
Redraw(Widget w)
{
  /*
   * Clear the window generating Expose events.
   * XXX It might be more efficient to fake up an Expose event
   * and call Redisplay directly
   */
    if (XtIsRealized(w))
      XClearArea(XtDisplay(w), XtWindow(w),
	0, 0,
	0 /*Full Width*/, 0 /*Full Height*/,
	True);
}

/*
 * Public interface to redraw method
 */
void
XbaeClipRedraw(Widget w)
{
  /*
   * Make sure w is a Clip or a subclass
   */
    XtCheckSubclass(w, xbaeClipWidgetClass, NULL);

  /*
   * Call the redraw method
   */
    if (XtIsRealized(w))
      (*((XbaeClipWidgetClass) XtClass(w))->clip_class.redraw)
	((Widget)w);
}

static void
FocusInACT(Widget w, XEvent * event, String *params, Cardinal *nparams)
{
    XbaeClipWidget cw = (XbaeClipWidget)w;

    if (event->xany.type != FocusIn || !event->xfocus.send_event)
      return;

    if (cw->clip.focus_callback)
      XtCallCallbackList((Widget)cw, cw->clip.focus_callback, NULL);
}
