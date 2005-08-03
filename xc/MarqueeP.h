/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/* MarqueeP.h -- Private header file for Marquee widget */

#ifndef _XtMarqueeP_h
#define _XtMarqueeP_h

/* Include private header file of superclass */
#include <Xm/PrimitiveP.h>

/* Include public header file for this widget */
#include "Marquee.h"

/* New fields for the Marquee widget class record */
typedef struct {
    int dummy;      /* keep compiler happy */
} MarqueeClassPart;

/* Full class record declaration */
typedef struct _MarqueeClassRec {
    CoreClassPart        core_class;
    XmPrimitiveClassPart primitive_class;
    MarqueeClassPart     Marquee_class;
} MarqueeClassRec;

extern MarqueeClassRec marqueeClassRec;

/* New fields for the Marquee widget record */
typedef struct {
  /* Resources */
    int blink_time;            /* Duration of one blink in ms */
    Boolean transparent;       /* Whether widget is transparent */
  /* Private */
    Boolean first_state;       /* Is in first state */
    GC gc1;                    /* GC for first state of the marquee */
    GC gc2;                    /* GC for second state of the marquee */
    XtIntervalId interval_id;  /* Timer ID */
} MarqueePart;

/* Full instance record declaration */
typedef struct _MarqueeRec {
    CorePart         core;
    XmPrimitivePart  primitive;
    MarqueePart      marquee;
} MarqueeRec;

#endif /* _XtMarqueeP_h */
