/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*******************************************************************
 FILE:		Xc.h
 CONTENTS:	Public header file for the Xc Control Panel Widget Set.
 AUTHOR:	Paul D. Johnston
 HISTORY:
 Date		Action
 ---------	------------------------------------
 3/11/92	Created.

********************************************************************/

#ifndef __XC_H
#define __XC_H

/****** Defined Xc widget resource names, classes, and representation types.
        Use these resource strings in your resource files */
#define XcNcontrolBackground	"controlBackground"

/****** The Control widget's color resource strings */
#define XcNlabelColor	"labelColor"

/****** The Control widget's label string */
#define XcNlabel	"label"
#define XcCLabel	"Label"

/****** The Control widget's shade depth for the 3D effect */
#define XcNshadeDepth	"shadeDepth"
#define XcCShadeDepth	"ShadeDepth"

/****** Max and min shadeDepth resource settings */
#define MIN_SHADE_DEPTH		1
#define MAX_SHADE_DEPTH		3

/* These strings are used in specifying a Value widget's value datatype */
#define XcNdataType	"dataType"
#define XcCDataType	"DataType"
#define XcRDType	"XcDType"
#define XcElval		"lval"
#define XcEhval		"hval"
#define XcEfval		"fval"

/****** Widget's value resource */
#define XcNvalue	"value"
#define XcCValue	"Value"

/****** Widget's userData resource */
#define XcNuserData	"userData"
#define XcCUserData	"UserData"


/****** Value foreground/background color resources */
#define XcNvalueForeground	"valueForeground"
#define XcNvalueBackground	"valueBackground"

/****** Resource that specifies what to increment a widget's value by */
#define XcNincrement	"increment"
#define XcCIncrement	"Increment"

/****** Number of decimal positions for floating point values */
#define XcNdecimals	"decimals"
#define XcCDecimals	"Decimals"

/****** Range of a widget's value */
#define XcNupperBound	"upperBound"
#define XcCUpperBound	"UpperBound"
#define XcNlowerBound	"lowerBound"
#define XcCLowerBound	"LowerBound"

/****** Widget orientation: vertical or horizontal */
#define XcNorient	"orient"
#define XcCOrient	"Orient"
#define XcROrient	"XcOrient"
#define XcEvert		"vertical"
#define XcEhoriz	"horizontal"

/****** Widget fill mode: from edge or from center */
#define XcNfillmod      "fillmod"
#define XcCFillmod      "Fillmod"
#define XcRFillmod      "XcFillmod"
#define XcEedge         "edge"
#define XcEcenter       "center"

/* Time interval between update callbacks used by the indicator style
   widgets */
#define XcNinterval	"interval"
#define XcCInterval	"Interval"

/* Callback function called by the button style widgets when the user
   activates the button */
#define XcNactivateCallback	"activateCallback"

/*
 * Callback function called by the Value style widgets when the user
 * has changed its value.  A pointer to the XcData structure is passed to
 * the callback function containing the new value, it's datatype, and
 * number of decimal places if applicable.
 */
#define XcNvalueCallback	"valueCallback"

/*
 * Callback function called by the indicator style widgets at timed
 * intervals.
 */
#define XcNupdateCallback	"updateCallback"


/****** Justification of a widget's value within the Value Box. */
#define XcNvalueJustify		"valueJustify"
#define XcCValueJustify		"ValueJustify"
#define XcRValueJustify		"XcValueJustify"
#define XcEjustifyLeft		"justifyleft"
#define XcEjustifyRight		"justifyright"
#define XcEjustifyCenter	"justifycenter"


/****** Type for a widget's orientation, and fill mode */
typedef enum {
    XcVert = 1,
    XcHoriz,
    XcVertDown,
    XcHorizLeft
} XcOrient;

typedef enum {
    XcEdge = 1,
    XcCenter
} XcFillmod;

/* Data type of the value manipulated with a Xc widget subclass. */
typedef enum {
    XcLval = 1,
    XcFval,
    XcHval
} XcDType;

/* Type for a widget's valueJustify resource setting. */
typedef enum
{
    XcJustifyLeft = 1,
    XcJustifyRight,
    XcJustifyCenter
} XcValueJustify;

/* This respresentation type is a union for the possible Xc datatypes */
#define XcRVType	"XcVType"

/* Type used for holding values manipulated with an Xc widget subclass */
typedef union v_t {
    long lval;
    float fval;
} XcVType;

/* This structure is used by Xc widget action functions to communicate
 * important values to the application's callback functions.
 * The application should use the XcData data type for the call_data
 * parameter.
 */
typedef struct xc_stuff {
    XcDType dtype;
    XcVType value;
    int decimals;
} XcCallData, *XcData;

#endif  /* __XC_H */
