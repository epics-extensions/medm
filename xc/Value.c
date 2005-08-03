/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*******************************************************************
 FILE:		Value.c
 CONTENTS:	Definitions for structures, methods, and actions of the
		Value widget.
 AUTHOR:	Paul D. Johnston
********************************************************************/

#include <stdio.h>

/* Xlib includes */
#include <X11/Xlib.h>

/* Xt includes */
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>

/* Xc includes */
#include "Xc.h"
#include "Control.h"

/* Widget includes */
#include "ValueP.h"	/* (includes Value.h also) */

/* other includes */
#include "cvtFast.h"

#ifndef MIN
# define  MIN(a,b)    (((a) < (b)) ? (a) :  (b))
#endif
#ifndef MAX
#  define  MAX(a,b)    (((a) > (b)) ? (a) :  (b))
#endif


/* Macro redefinition for offset. */
#define offset(field) XtOffset(ValueWidget, field)

/* Widget method function prototypes */
static void ClassInitialize(void);
static void Initialize(Widget request, Widget new,
  ArgList args, Cardinal *nargs);
static Boolean SetValues(Widget cur, Widget req,
  Widget new, ArgList args, Cardinal *nargs);
static void CvtStringToDType(XrmValue *args, Cardinal *nargs,
  XrmValue *fromVal, XrmValue *toVal);
static void CvtStringToVType(XrmValue *args, Cardinal *nargs,
  XrmValue *fromVal, XrmValue *toVal);
static void CvtStringToValueJustify(XrmValue *args, Cardinal *nargs,
  XrmValue *fromVal, XrmValue *toVal);

/* Define the widget's resource list */
static XtResource resources[] =
{
    {
	XcNvalueForeground,
	XtCColor,
	XtRPixel,
	sizeof(Pixel),
	offset(value.value_fg_pixel),
	XtRString,
	XtDefaultForeground
    },
    {
	XcNvalueBackground,
	XtCColor,
	XtRPixel,
	sizeof(Pixel),
	offset(value.value_bg_pixel),
	XtRString,
	"white"
    },
    {
	XcNvalueCallback,
	XtCCallback,
	XtRCallback,
	sizeof(XtPointer),
	offset(value.callback),
	XtRCallback,
	NULL
    },
    {
	XcNdataType,
	XcCDataType,
	XcRDType,
	sizeof(XcDType),
	offset(value.datatype),
	XtRString,
	"lval"
    },
    {
	XcNdecimals,
	XcCDecimals,
	XtRInt,
	sizeof(int),
	offset(value.decimals),
	XtRImmediate,
	(XtPointer)2
    },
    {
	XcNincrement,
	XcCIncrement,
	XcRVType,
	sizeof(XcVType),
	offset(value.increment),
	XtRString,
	"1"
    },
    {
	XcNupperBound,
	XcCUpperBound,
	XcRVType,
	sizeof(XcVType),
	offset(value.upper_bound),
	XtRString,
	"100"
    },
    {
	XcNlowerBound,
	XcCLowerBound,
	XcRVType,
	sizeof(XcVType),
	offset(value.lower_bound),
	XtRString,
	"0"
    },
    {
	XcNvalueJustify,
	XcCValueJustify,
	XcRValueJustify,
	sizeof(XcValueJustify),
	offset(value.justify),
	XtRString,
	"justifycenter"
    },
    {
	XcNvalue,
	XcCValue,
	XcRVType,
	sizeof(XcVType),
	offset(value.val),
	XtRString,
	"0"
    },
    {
	XcNuserData,
	XcCUserData,
	XtRPointer,
	sizeof(XtPointer),
	offset(value.userData),
	XtRPointer,
	NULL
    },
};


/* Widget Class Record initialization */
ValueClassRec valueClassRec =
{
    {
      /* core_class part */
        (WidgetClass) &controlClassRec,  /* superclass */
        "Value",                         /* class_name */
        sizeof(ValueRec),                /* widget_size */
        ClassInitialize,                 /* class_initialize */
        NULL,                            /* class_part_initialize */
        FALSE,                           /* class_inited */
        Initialize,                      /* initialize */
        NULL,                            /* initialize_hook */
        XtInheritRealize,                /* realize */
        NULL,                            /* actions */
        0,                               /* num_actions */
        resources,                       /* resources */
        XtNumber(resources),             /* num_resources */
        NULLQUARK,                       /* xrm_class */
        TRUE,                            /* compress_motion */
        TRUE,                            /* compress_exposure */
        TRUE,                            /* compress_enterleave */
        TRUE,                            /* visible_interest */
        NULL,                            /* destroy */
        NULL,                            /* resize */
        NULL,                            /* expose */
        SetValues,                       /* set_values */
        NULL,                            /* set_values_hook */
        XtInheritSetValuesAlmost,        /* set_values_almost */
        NULL,                            /* get_values_hook */
        NULL,                            /* accept_focus */
        XtVersion,                       /* version */
        NULL,                            /* callback_private */
        NULL,                            /* tm_table */
        NULL,                            /* query_geometry */
        NULL,                            /* display_accelerator */
        NULL,                            /* extension */
    },
    {
      /* Control class part */
        0,                               /* dummy_field */
    },
    {
      /* Value class part */
        0,                               /* dummy_field */
    },
};

WidgetClass xcValueWidgetClass = (WidgetClass)&valueClassRec;

/*******************************************************************
 NAME:		ClassInitialize.
 DESCRIPTION:
   This method initializes the Value widget class. Specifically,
 it registers resource value converter functions with Xt.
*******************************************************************/

static void ClassInitialize()
{
    XtAddConverter(XtRString, XcRDType, CvtStringToDType, NULL, 0);
    XtAddConverter(XtRString, XcRVType, CvtStringToVType, NULL, 0);
    XtAddConverter(XtRString, XcRValueJustify, CvtStringToValueJustify, NULL,0);
}

/*******************************************************************
 NAME:		Initialize.
 DESCRIPTION:
   This is the initialize method for the Value widget.  It
 validates user-modifiable instance resources and initializes private
 widget variables and structures.
*******************************************************************/

static void Initialize(Widget request, Widget new,
  ArgList args, Cardinal *nargs)
{
    ValueWidget wnew = (ValueWidget)new;

    DPRINTF(("Value: executing Initialize \n"));
  /*
   * Validate public instance variable settings.
   */
    if ((wnew->value.datatype != XcLval) &&
      (wnew->value.datatype != XcHval) &&
      (wnew->value.datatype != XcFval)) {
	XtWarning("Value: invalid datatype setting.");
	wnew->value.datatype = XcLval;
    }

  /* Check decimals setting */
    if ((wnew->value.datatype == XcFval) &&
      ((wnew->value.decimals < MIN_DECIMALS) ||
	(wnew->value.decimals > MAX_DECIMALS))) {
	XtWarning("Value: invalid decimals setting.");
	wnew->value.decimals = 2;
    }

  /* Check increment setting */
    if (((wnew->value.datatype == XcLval) || (wnew->value.datatype == XcHval))
      && (wnew->value.increment.lval <= 0)) {
	XtWarning("Value: invalid increment setting.");
	wnew->value.increment.lval = 1L;
    } else if ((wnew->value.datatype == XcFval) &&
      (wnew->value.increment.fval <= 0.0)) {
	XtWarning("Value: invalid increment setting.");
	wnew->value.increment.fval = 1.0;
    }

  /* Check lower/upper bound setting */
    if (((wnew->value.datatype == XcLval) ||
      (wnew->value.datatype == XcHval)) &&
      (wnew->value.lower_bound.lval >=
	wnew->value.upper_bound.lval)) {
	XtWarning("Value: invalid lowerBound/upperBound setting.");
	wnew->value.lower_bound.lval =
	  (wnew->value.upper_bound.lval - wnew->value.increment.lval);
    } else if ((wnew->value.datatype == XcFval) &&
      (wnew->value.lower_bound.fval >=
	wnew->value.upper_bound.fval)) {
	XtWarning("Value: invalid lowerBound/upperBound setting.");
	wnew->value.lower_bound.fval =
	  (wnew->value.upper_bound.fval - wnew->value.increment.fval);
    }

  /* Check the initial value setting */
    if ((wnew->value.datatype == XcLval)  || (wnew->value.datatype == XcHval)) {
	if (wnew->value.val.lval < wnew->value.lower_bound.lval)
	  wnew->value.val.lval = wnew->value.lower_bound.lval;
	else if (wnew->value.val.lval > wnew->value.upper_bound.lval)
	  wnew->value.val.lval = wnew->value.upper_bound.lval;
    } else if (wnew->value.datatype == XcFval) {
	if (wnew->value.val.fval < wnew->value.lower_bound.fval)
	  wnew->value.val.fval = wnew->value.lower_bound.fval;
	else if (wnew->value.val.fval > wnew->value.upper_bound.fval)
	  wnew->value.val.fval = wnew->value.upper_bound.fval;
    }

  /* Check the initial valueJustify setting. */
    if ((wnew->value.justify != XcJustifyLeft) &&
      (wnew->value.justify != XcJustifyRight) &&
      (wnew->value.justify != XcJustifyCenter)) {
	XtWarning("Value: invalid valueJustify setting.");
	wnew->value.justify = XcJustifyCenter;
    }

    DPRINTF(("Value: done Initialize \n"));
}

/*******************************************************************
 NAME:		SetValues.
 DESCRIPTION:
   This is the set_values method for this widget. It validates resource
 settings set with XtSetValues. If a resource is changed that would
 require re-drawing the widget, return True.

*******************************************************************/

static Boolean SetValues(Widget cur, Widget req,
  Widget new, ArgList args, Cardinal *nargs)
  /* KE: req, args, nargs is not used */
{
    ValueWidget wnew = (ValueWidget)new;
    ValueWidget wcur = (ValueWidget)cur;
    Boolean do_redisplay = False;

    DPRINTF(("Value: executing SetValues \n"));

  /* Validate new resource settings. */

  /* Check the widget's color resources. */
    if ((wnew->value.value_fg_pixel != wcur->value.value_fg_pixel) ||
      (wnew->value.value_fg_pixel != wcur->value.value_fg_pixel))
      do_redisplay = True;

  /* Check the datatype */
    if (wnew->value.datatype != wcur->value.datatype) {
	do_redisplay = True;
	if ((wnew->value.datatype != XcLval) &&
	  (wnew->value.datatype != XcHval) &&
	  (wnew->value.datatype != XcFval)) {
	    XtWarning("Value: invalid datatype setting.");
	    wnew->value.datatype = XcLval;
	}
    }

  /* Check the decimals setting */
    if (wnew->value.decimals != wcur->value.decimals) {
	do_redisplay = True;
	if ((wnew->value.decimals < MIN_DECIMALS) ||
	  (wnew->value.decimals > MAX_DECIMALS)) {
	    XtWarning("Value: invalid decimals setting.");
	    wnew->value.decimals = 2;
	}
    }

  /* Check the increment setting */
    if (((wnew->value.datatype == XcLval) || (wnew->value.datatype == XcHval))
      && (wnew->value.increment.lval <= 0)) {
	XtWarning("Value: invalid increment setting.");
	wnew->value.increment.lval = 1L;
    } else if ((wnew->value.datatype == XcFval) &&
      (wnew->value.increment.fval <= 0.0)) {
	XtWarning("Value: invalid increment setting.");
	wnew->value.increment.fval = 1.0;
    }

  /* Check the lowerBound/upperBound setting */
    if (((wnew->value.datatype == XcLval) ||
      (wnew->value.datatype == XcHval)) &&
      (wnew->value.lower_bound.lval >=
	wnew->value.upper_bound.lval)) {
	XtWarning("Value: invalid lowerBound/upperBound setting.");
	wnew->value.lower_bound.lval =
	  (wnew->value.upper_bound.lval - wnew->value.increment.lval);
    } else if ((wnew->value.datatype == XcFval) &&
      (wnew->value.lower_bound.fval >=
	wnew->value.upper_bound.fval)) {
	XtWarning("Value: invalid lowerBound/upperBound setting.");
	wnew->value.lower_bound.fval =
	  (wnew->value.upper_bound.fval - wnew->value.increment.fval);
    }

  /* Check the new value setting */
    if (((wnew->value.datatype == XcLval) || (wnew->value.datatype == XcHval)) &&
      (wnew->value.val.lval != wcur->value.val.lval)) {
	do_redisplay = True;
	if (wnew->value.val.lval < wnew->value.lower_bound.lval)
	  wnew->value.val.lval = wnew->value.lower_bound.lval;
	else if (wnew->value.val.lval > wnew->value.upper_bound.lval)
	  wnew->value.val.lval = wnew->value.upper_bound.lval;
    }
    if ((wnew->value.datatype == XcFval)  &&
      (wnew->value.val.fval != wcur->value.val.fval)) {
	do_redisplay = True;
	if (wnew->value.val.fval < wnew->value.lower_bound.fval)
	  wnew->value.val.fval = wnew->value.lower_bound.fval;
	else if (wnew->value.val.fval > wnew->value.upper_bound.fval)
	  wnew->value.val.fval = wnew->value.upper_bound.fval;
    }

  /* Check the valueJustify setting. */
    if (wnew->value.justify != wcur->value.justify) {
	do_redisplay = True;
	if ((wnew->value.justify != XcJustifyLeft) &&
	  (wnew->value.justify != XcJustifyRight) &&
	  (wnew->value.justify != XcJustifyCenter)) {
	    XtWarning("Value: invalid valueJustify setting.");
	    wnew->value.justify = XcJustifyCenter;
	}
    }

    DPRINTF(("Value: done SetValues\n"));

    return do_redisplay;
}

/* Widget class functions */

/*******************************************************************
 NAME:		CvtStringToDType.
 DESCRIPTION:
   This function converts resource settings in string form to the
XtRDType representation type.
*******************************************************************/

static void CvtStringToDType(XrmValue *args, Cardinal *nargs,
  XrmValue *fromVal, XrmValue *toVal)
{
  /* Local variables */
    static XcDType datatype;
    char lowerstring[100];

  /* Convert the resource string to lower case for quick comparison */
    ToLower((char *)fromVal->addr, lowerstring);

  /*
   * Compare resource string with valid datatype strings and assign to
   * datatype.
   */
    if (strcmp(lowerstring, XcElval) == 0) {
	datatype = XcLval;
	CvtDone(XcDType, &datatype);
    } else if (strcmp(lowerstring, XcEhval) == 0) {
	datatype = XcHval;
	CvtDone(XcDType, &datatype);
    } else if (strcmp(lowerstring, XcEfval) == 0) {
	datatype = XcFval;
	CvtDone(XcDType, &datatype);
    }

  /*
   * If the string is not valid for this resource type, print a warning
   * and do not make the conversion.
   */
    XtStringConversionWarning(fromVal->addr, "XcDType");
    toVal->addr = NULL;
    toVal->size = 0;
}

/*******************************************************************
 NAME:		CvtStringToVType.
 DESCRIPTION:
   This function converts resource settings in string form to the
 XcRVType representation type.
*******************************************************************/

static void CvtStringToVType(XrmValue *args, Cardinal *nargs,
  XrmValue *fromVal, XrmValue *toVal)
{
    long num;
    static XcVType value;
    char temp[30], temp2[30];

  /*
   * Make sure the string is a numeric constant. If it isn't, print
   * a warning, and NULL the destination value.
   */
    if ((num = sscanf((char *)fromVal->addr,
      "%[-012345689ABCDEFabcdef.]", temp)) == (int)NULL) {
	XtStringConversionWarning(fromVal->addr, "XcVType");
	CvtDone(char, NULL);
    }

  /* If the number contains a decimal point, convert it into a float. */
    if (strchr(temp, '.') != NULL) {
	sscanf(temp, "%f", &value.fval);
	CvtDone(XcVType, &value.fval);
    }

  /* If it contains any hexadecimal characters, convert it to a long hex int */
    if (sscanf(temp, "%[ABCDEFabcdef]", temp2) != (int)NULL) {
	sscanf(temp, "%lX", (unsigned long *)&value.lval);
	CvtDone(XcVType, &value.lval);
    }

  /* Otherwise, it's an integer type. Convert the string to a numeric constant. */
    sscanf(temp, "%ld", &value.lval);
    CvtDone(XcVType, &value.lval);
}

/*******************************************************************
 NAME:		CvtStringToValueJustify.
 DESCRIPTION:
   This function converts resource settings in string form to the
 XcRValueJustify representation type.

*******************************************************************/

static void CvtStringToValueJustify(XrmValue *args, Cardinal *nargs,
  XrmValue *fromVal, XrmValue *toVal)
{
    static XcValueJustify val_justify;
    char lowerstring[100];

  /* Convert the resource string to lower case for quick comparison */
    ToLower((char *)fromVal->addr, lowerstring);

  /*
   * Compare resource string with valid XcValueJustify strings and assign to
   * datatype.
   */
    if (strcmp(lowerstring, XcEjustifyLeft) == 0) {
	val_justify = XcJustifyLeft;
	CvtDone(XcValueJustify, &val_justify);
    } else if (strcmp(lowerstring, XcEjustifyRight) == 0) {
	val_justify = XcJustifyRight;
	CvtDone(XcValueJustify, &val_justify);
    } else if (strcmp(lowerstring, XcEjustifyCenter) == 0) {
	val_justify = XcJustifyCenter;
	CvtDone(XcValueJustify, &val_justify);
    }

  /*
   * If the string is not valid for this resource type, print a warning
   * and do not make the conversion.
   */
    XtStringConversionWarning(fromVal->addr, XcRValueJustify);
    toVal->addr = NULL;
    toVal->size = 0;
}


float Correlate(float from_val, float from_range, float to_range)
  /*************************************************************************
 * Correlate: This function takes the given value between the given      *
 *   range and correlates it to the destination range, returning the     *
 *   correlated value.                                                   *
 *************************************************************************/
{
    double percent;
    float result;

    percent = ((from_val * 100.0) / from_range);

  /****** Clip to 0 and 100 */
    percent = MAX(0.,percent);
    percent = MIN(100.,percent);

    result = (float)(to_range * (percent / 100.0));

    return result;
}

/*********************************************************************
 FUNCTION:	Print_value.
 DESCRIPTION:
  Simply converts a widget's value into a string based on the value's
 datatype and returns a pointer to it.

*********************************************************************/

char *Print_value(XcDType datatype, XcVType *value, int decimals)
{
    static char string[40];

    if (datatype == XcLval)
      cvtLongToString(value->lval,string);
    else if (datatype == XcHval)
      cvtLongToHexString(value->lval,string);
    else if (datatype == XcFval)
      cvtFloatToString(value->fval,string,(unsigned short)decimals);

    return &string[0];

}

/*********************************************************************
 FUNCTION:	Position_val.
 DESCRIPTION:
  This function positions the displayed value within the Value Box based
 on the XcNvalueJustify resource setting.

*********************************************************************/

void Position_val(Widget w)
{
    ValueWidget wv = (ValueWidget)w;
    char *val_string;
    int text_width;

  /* Establish the position of the value string within the Value Box. */
    val_string = Print_value(wv->value.datatype, &wv->value.val,
      wv->value.decimals);
    text_width = XTextWidth(wv->control.font, val_string, strlen(val_string));

    if (wv->value.justify == XcJustifyLeft)
      wv->value.vp.x = wv->value.value_box.x +
	(wv->control.font->max_bounds.rbearing / 2);
    else if (wv->value.justify == XcJustifyRight)
      wv->value.vp.x = wv->value.value_box.x +
	wv->value.value_box.width - text_width -
	(wv->control.font->max_bounds.rbearing / 2);
    else if (wv->value.justify == XcJustifyCenter)
      wv->value.vp.x = wv->value.value_box.x +
	(wv->value.value_box.width / 2) -
	(text_width / 2);

    wv->value.vp.y = wv->value.value_box.y +
      (int)(wv->value.value_box.height -
	(wv->control.font->ascent + wv->control.font->descent))/2
      + wv->control.font->ascent + 1;
}
