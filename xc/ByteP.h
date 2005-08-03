/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/****************************************************************************
 * File    : ByteP.h                                                        *
 * Author  : David M. Wetherholt - DMW Software 1994.                       *
 * Lab     : Continuous Electron Beam Accelerator Facility                  *
 * Modified: 7 July 94                                                      *
 * Mods    : 1.0 Created                                                    *
 ****************************************************************************/

#ifndef __XC_BYTEP_H
#define __XC_BYTEP_H

/****** Include the superclass private headers */
#include <X11/CoreP.h>
#include "Xc.h"
#include "ValueP.h"
#include "Byte.h"

/****** Private declarations and definitions */
#define MIN_BY_WIDTH		10
#define MIN_BY_HEIGHT		10
#define MAX_BY_WIDTH		500
#define MAX_BY_HEIGHT		500

/****** Class part - minimum of one member required */
typedef struct {
    int dummy;
} ByteClassPart;

/****** Class record */
typedef struct _ByteClassRec {
    CoreClassPart core_class;
    ControlClassPart control_class;
    ValueClassPart value_class;
    ByteClassPart byte_class;
} ByteClassRec;

/****** Declare the widget class record as external for widget source file */
extern ByteClassRec byteClassRec;

/****** Instance part */
typedef struct {
    XcOrient orient;                     /* Byte orientation */
    int   interval;			/* Time interval for updateCallback */
    XtCallbackList update_callback;	/* The updateCallback function */
    Pixel byte_background;		/* Background color of the byte */
    Pixel byte_foreground;		/* Foreground color of the byte */
    XRectangle face;                     /* Geometry of the Byte face */
    int   sbit, ebit;                    /* Starting, Ending bit */
    int   interval_id;			/* Xt TimeOut interval ID */
} BytePart;

/****** Instance record */
typedef struct _ByteRec {
    CorePart core;
    ControlPart control;
    ValuePart value;
    BytePart byte;
} ByteRec;

/****** Declare widget class functions here */

#endif
