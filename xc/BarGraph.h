/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/****************************************************************************
 FILE:		BarGraph.h
 CONTENTS:	Public header file for the BarGraph widget.
 AUTHOR:	Paul D. Johnston
 Date		Action
 ---------	------------------------------------
 5/23/92	Changed the widget class name so that it is preceded
		by 'xc' with the first major word capitalized.
 4/15/92	Created.
 ****************************************************************************/

#ifndef __XC_BARGRAPH_H
#define __XC_BARGRAPH_H

/****** Superclass header */
#include "Value.h"

/****** Define widget resource names, classes, and representation types.
        Use these resource strings in your resource files */
#define XcNbarBackground "barBackground"
#define XcNbarForeground "barForeground"
#define XcNscaleColor    "scaleColor"
#define XcNscaleSegments "scaleSegments"
#define XcCScaleSegments "ScaleSegments"
#define XcNvalueVisible  "valueVisible"
#define XcNdecorations   "decorations"
#define XcNdoubleBuffer  "doubleBuffer"

/****** Class record declarations */
extern WidgetClass xcBarGraphWidgetClass;

typedef struct _BarGraphClassRec *BarGraphWidgetClass;
typedef struct _BarGraphRec *BarGraphWidget;

/* Function prototypes for widget methods */
extern void XcBGUpdateValue(Widget w, XcVType *value);
extern void XcBGUpdateBarForeground(Widget w, Pixel pixel);

#endif
