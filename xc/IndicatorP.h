/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*******************************************************************
 FILE:		IndicatorP.h
 CONTENTS:	Private definitions for the Indicator widget.
 AUTHOR:	Paul D. Johnston
 HISTORY:
 Date		Action
 ---------	------------------------------------
 4/15/91	Created.

********************************************************************/

#ifndef __XC_INDICATORP_H
#define __XC_INDICATORP_H

/*
 * Include the superclass private headers.
 */
#include <X11/CoreP.h>
#include "Xc.h"
#include "ValueP.h"

/*
 * Include this widget class public header.
 */
#include "Indicator.h"


/* Private declarations and definitions. */
#define MIN_INDICATOR_WIDTH		20
#define MIN_INDICATOR_HEIGHT		20
#define MAX_INDICATOR_WIDTH		500
#define MAX_INDICATOR_HEIGHT		500

/* Max number of Scale segments. */
#define MIN_SCALE_SEGS		0
#define MAX_SCALE_SEGS		21


/*
 * Class part.
 */
typedef struct
{
    int dummy;	/* Minimum of one member required. */
} IndicatorClassPart;

/*
 * Class record.
 */
typedef struct _IndicatorClassRec
{
    CoreClassPart core_class;
    ControlClassPart control_class;
    ValueClassPart value_class;
    IndicatorClassPart indicator_class;
} IndicatorClassRec;

/*
 * Declare the widget class record as external for use in the widget source
 * file.
 */
extern IndicatorClassRec indicatorClassRec;



/*
 * Instance part.
 */
typedef struct
{
  /* Public instance variables. */
    XcOrient orient;			/* Indicator's orientation: vertical
					 * or horizontal.
					 */
    int interval;			/* Time interval for updateCallback */
    XtCallbackList update_callback;	/* The updateCallback function. */
    Pixel indicator_background;		/* Background color of the indicator. */
    Pixel indicator_foreground;		/* Foreground color of the indicator. */
    Pixel scale_pixel;			/* Color of the Scale indicator. */
    int num_segments;			/* Number of segments in the Scale */
    Boolean value_visible;		/* Enable/Disable display of the
					 * value in the Value Box.
					 */

   /* Private instance variables. */
    XRectangle face;			/* Geometry of the Indicator face */
    XPoint lbl;				/* Location of the Label string */
    XRectangle indicator;		/* Rectangle for the Bar indicator. */
    XSegment scale_line;			/* Scale line along Bar indicator. */
    XPoint segs[MAX_SCALE_SEGS+1];	/* Line segments for the Scale. */
    int seg_length;			/* Length of Scale line segments. */
    XPoint max_val;			/* Point at which to draw the max
					 * value string on the Scale.
					 */
    XPoint min_val;			/* Point at which to draw the min
					 * value string on the Scale.
					 */
    int interval_id;			/* Xt TimeOut interval ID. */

} IndicatorPart;

/*
 * Instance record.
 */
typedef struct _IndicatorRec
{
    CorePart core;
    ControlPart control;
    ValuePart value;
    IndicatorPart indicator;
} IndicatorRec;


/* Declare widget class functions here. */


#endif  /* INDICATORP_H */
