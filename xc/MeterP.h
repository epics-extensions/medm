/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*******************************************************************
 FILE:		MeterP.h
 CONTENTS:	Private definitions for the Meter widget.
 AUTHOR:	Mark Anderson, derived from work of Paul D. Johnston
********************************************************************/

#ifndef __XC_METERP_H
#define __XC_METERP_H

/*
 * Include the superclass private headers.
 */
#include <math.h>
#include <X11/CoreP.h>
#include "Xc.h"
#include "ValueP.h"

/*
 * Include this widget class public header.
 */
#include "Meter.h"


/* Private declarations and definitions. */
#define MIN_METER_WIDTH		20
#define MIN_METER_HEIGHT	20
#define MAX_METER_WIDTH		500
#define MAX_METER_HEIGHT	500

/* Max number of Scale segments. */
#define MIN_SCALE_SEGS		0
#define MAX_SCALE_SEGS		21

/* Angle definitions */
#define MIN_ANGLE 0.0
#define MAX_ANGLE 180.0
#if !defined( M_PI )
#define M_PI 3.14159265358979323846
#endif
#define RADIANS(x) (M_PI * 2.0 * (x) /360.)
/*
 * Class part.
 */
typedef struct
{
    int dummy;	/* Minimum of one member required. */
} MeterClassPart;

/*
 * Class record.
 */
typedef struct _MeterClassRec
{
    CoreClassPart core_class;
    ControlClassPart control_class;
    ValueClassPart value_class;
    MeterClassPart meter_class;
} MeterClassRec;

/*
 * Declare the widget class record as external for use in the widget source
 * file.
 */
extern MeterClassRec meterClassRec;



/*
 * Instance part.
 */
typedef struct
{
  /* Public instance variables. */
    int interval;			/* Time interval for updateCallback */
    XtCallbackList update_callback;	/* The updateCallback function. */
    Pixel meter_background;		/* Background color of the meter. */
    Pixel meter_foreground;		/* Foreground color of the meter. */
    Pixel scale_pixel;			/* Color of the Scale. */
    int num_segments;			/* Number of segments in the Scale */
    Boolean value_visible;		/* Enable/Disable display of the
					 * value in the Value Box.
					 */

   /* Private instance variables. */
    XRectangle face;			/* Geometry of the Meter face */
    XPoint lbl;				/* Location of the Label string */
    XRectangle meter;			/* Rectangle for the Meter. */
    XSegment segs[MAX_SCALE_SEGS+1];	/* Line segments for the Scale. */
    XPoint max_val;			/* Point at which to draw the max
					 * value string on the Scale.
					 */
    XPoint min_val;			/* Point at which to draw the min
					 * value string on the Scale.
					 */
    XPoint meter_center;			/* center of meter radius  */
    int inner_radius;			/* inner radius length     */
    int outer_radius;			/* outer radius length	   */
    int interval_id;			/* Xt TimeOut interval ID. */

} MeterPart;

/*
 * Instance record.
 */
typedef struct _MeterRec
{
    CorePart core;
    ControlPart control;
    ValuePart value;
    MeterPart meter;
} MeterRec;


/* Declare widget class functions here. */


#endif  /* __XC_METERP_H */
