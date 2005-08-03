/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
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


/* Function prototypes for public widget methods */
void XcIndUpdateValue(Widget w, XcVType *value);
void XcIndUpdateIndicatorForeground(Widget w, Pixel pixel);
#ifdef ACM
void XcIndUpdateLowerBound(Widget w, XcVType *value);
void XcIndUpdateUpperBound(Widget w, XcVType *value);
#endif

#endif
