/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/* Marquee.h -- Header file for Marquee widget */

#ifndef _XtMarquee_h
#define _XtMarquee_h

#if 0
/* The public header file for the immediate superclass normally must
 * be included.  However, not in this case because the public header
 * file for Primitive is in Xm.h, which is already included in all
 * Motif applications.  */
#include <Xm/Superclass.h>
#endif

#define XtNtransparent "transparent"
#define XtNblinkTime "blinkTime"

#define XtCTransparent "Transparent"
#define XtCBlinkTime "BlinkTime"

/* Class record constants */

extern WidgetClass marqueeWidgetClass;

typedef struct _MarqueeClassRec *MarqueeWidgetClass;
typedef struct _MarqueeRec      *MarqueeWidget;

#endif /* _XtMarquee_h */
/* DON'T ADD STUFF AFTER THIS #endif */

