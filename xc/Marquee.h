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

