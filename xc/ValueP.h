/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*******************************************************************
 FILE:		ValueP.h
 CONTENTS:	Private definitions for the Value widget class.
 AUTHOR:	Paul D. Johnston
 HISTORY:
 Date		Action
 ---------	------------------------------------
 3/11/92	Created.

********************************************************************/

#ifndef __XC_VALUEP_H
#define __XC_VALUEP_H

/*
 * Include the superclass private header.
 */
#include <X11/CoreP.h>
#include "Xc.h"
#include "ControlP.h"

/*
 * Include this widget class public header.
 */
#include "Value.h"


/* Maximum and minimum decimal precision settings. */
#define MIN_DECIMALS		0
#define MAX_DECIMALS		8

/*
 * Class part.
 */
typedef struct
{
    int dummy;	/* Minimum of one member required. */
} ValueClassPart;

/*
 * Class record.
 */
typedef struct _ValueClassRec
{
    CoreClassPart core_class;
    ControlClassPart control_class;
    ValueClassPart value_class;
} ValueClassRec;

/*
 * Declare the widget class record as external for use in the widget source
 * file.
 */
extern ValueClassRec valueClassRec;

/*
 * Instance part.
 */
typedef struct
{
  /* Public instance variables. */
    Pixel value_fg_pixel;               /* Value string foreground color */
    Pixel value_bg_pixel;               /* Value Box background color */
    XtCallbackList callback;            /* Widget callbacks */
    XcDType datatype;                   /* Value's data type */
    int decimals;                       /* No. of decimal points (if Fval). */
    XcVType increment;                  /* Widget's increment. */
    XcVType upper_bound;                /* Upper range limit. */
    XcVType lower_bound;                /* Lower range limit. */
    XcVType val;                        /* Storage for the current value
                                         * manipulated by the widget. */
    XcValueJustify justify;             /* Left, Right, or Center justification
                                         * of the value within the Value Box. */
    XtPointer userData;                 /* userData field (arbitrary ptr). */

  /* Private instance variables. */
    XRectangle value_box;               /* Value Box to display value in. */
    XPoint vp;                          /* Place in Value Box to draw value. */
} ValuePart;

/*
 * Instance record.
 */
typedef struct _ValueRec
{
    CorePart core;
    ControlPart control;
    ValuePart value;
} ValueRec;

#endif  /* __XC_VALUEP_H */
