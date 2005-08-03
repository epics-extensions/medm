/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*******************************************************************
 FILE:		Meter.h
 CONTENTS:	Public header file for the Meter widget.
 AUTHOR:	Mark Anderson, derived from Paul D. Johnston's BarGraf widget
********************************************************************/

#ifndef __XC_METER_H
#define __XC_METER_H

/* Superclass header */
#include "Value.h"


/*
 * Define widget resource names, classes, and representation types.
 * Use these resource strings in your resource files.
 */
#define XcNmeterBackground	"meterBackground"
#define XcNmeterForeground	"meterForeground"
#define XcNscaleColor		"scaleColor"
#define XcNscaleSegments	"scaleSegments"
#define XcCScaleSegments	"ScaleSegments"
#define XcNvalueVisible		"valueVisible"



/* Class record declarations */

extern WidgetClass xcMeterWidgetClass;

typedef struct _MeterClassRec *MeterWidgetClass;
typedef struct _MeterRec *MeterWidget;


/* Widget functions */
extern void XcMeterUpdateValue(Widget w, XcVType *value);
extern void XcMeterUpdateMeterForeground(Widget w, Pixel pixel);

#endif
