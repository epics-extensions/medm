/*******************************************************************
 FILE:		Indicator.h
 CONTENTS:	Public header file for the Indicator widget.
 AUTHOR:	Paul D. Johnston
 HISTORY:
 Date		Action
 ---------	------------------------------------
 5/23/92	Changed the widget class name so that it is preceded
		by 'xc' with the first major word capitalized.
 4/15/92	Created.

********************************************************************/

#ifndef __XC_INDICATOR_H
#define __XC_INDICATOR_H

/* Superclass header */
#include "Value.h"


/*
 * Define widget resource names, classes, and representation types.
 * Use these resource strings in your resource files.
 */
#define XcNindicatorBackground	"indicatorBackground"
#define XcNindicatorForeground	"indicatorForeground"
#define XcNscaleColor		"scaleColor"
#define XcNscaleSegments	"scaleSegments"
#define XcCScaleSegments	"ScaleSegments"
#define XcNvalueVisible		"valueVisible"



/* Class record declarations */

extern WidgetClass xcIndicatorWidgetClass;

typedef struct _IndicatorClassRec *IndicatorWidgetClass;
typedef struct _IndicatorRec *IndicatorWidget;


/* Widget functions */
extern void XcIndUpdateValue(Widget w, XcVType *value);
extern void XcIndUpdateIndicatorForeground(Widget w,
	unsigned long pixel);


#endif

