/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/****************************************************************************
 FILE:		BarGraphP.h
 CONTENTS:	Private definitions for the BarGraph widget.
 AUTHOR:	Paul D. Johnston
 Date		Action
 ---------	------------------------------------
 15 Apr 91	Created.
 21 Jun 94      Modified to handle different fill modes
 ****************************************************************************/

#ifndef __XC_BARGRAPHP_H
#define __XC_BARGRAPHP_H

/****** Include the superclass private headers */
#include <X11/CoreP.h>
#include "Xc.h"
#include "ValueP.h"
#include "BarGraph.h"

/****** Private declarations and definitions */
#define MIN_BG_WIDTH		20
#define MIN_BG_HEIGHT		20
#define MAX_BG_WIDTH		500
#define MAX_BG_HEIGHT		500
#define MIN_SCALE_SEGS		0
#define MAX_SCALE_SEGS		21

/****** Class part - minimum of one member required */
typedef struct {
    int dummy;
} BarGraphClassPart;

/****** Class record */
typedef struct _BarGraphClassRec {
    CoreClassPart core_class;
    ControlClassPart control_class;
    ValueClassPart value_class;
    BarGraphClassPart barGraph_class;
} BarGraphClassRec;

/****** Declare the widget class record as external for widget source file */
extern BarGraphClassRec barGraphClassRec;

/****** Instance part */
typedef struct {
    XcOrient orient;			/* BarGraph's orientation */
    XcFillmod fillmod;                  /* BarGraph's fill mode */
    int interval;			/* Time interval for updateCallback */
    XtCallbackList update_callback;	/* The updateCallback function */
    Pixel bar_background;		/* Background color of the bar */
    Pixel bar_foreground;		/* Foreground color of the bar */
    Pixel scale_pixel;			/* Color of the Scale indicator */
    int num_segments;			/* Number of segments in the Scale */
    Boolean value_visible;		/* Enable/Disable display of the
					 * value in the Value Box */
    Boolean double_buffer;		/* Enable/Disable double buffering
					 * to avoid flashing */
    Boolean decorations;		/* Enable/Disable decorations */
  /******* Private instance variables */
    XRectangle face;			/* Geometry of the BarGraph face */
    XPoint lbl;				/* Location of the Label string */
    XRectangle bar;			/* Rectangle for the Bar indicator */
    XSegment scale_line; 		/* Scale line along Bar indicator */
    XPoint segs[MAX_SCALE_SEGS+1];	/* Line segments for the Scale */
    int seg_length;			/* Length of Scale line segments */
    XPoint max_val;			/* Point at which to draw the max
					 * value string on the Scale */
    XPoint min_val;			/* Point at which to draw the min
					 * value string on the Scale */
    int interval_id;			/* Xt TimeOut interval ID */
} BarGraphPart;

/****** Instance record */
typedef struct _BarGraphRec {
    CorePart core;
    ControlPart control;
    ValuePart value;
    BarGraphPart barGraph;
} BarGraphRec;

/****** Declare widget class functions here */

#endif  /* BARGRAPHP_H */
