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
extern void XcMeterUpdateMeterForeground(Widget w, unsigned long pixel);

#endif

