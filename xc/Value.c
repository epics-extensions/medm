/*******************************************************************
 FILE:		Value.c
 CONTENTS:	Definitions for structures, methods, and actions of the 
		Value widget.
 AUTHOR:	Paul D. Johnston
 HISTORY:
 Date		Action
 ---------	------------------------------------
 01/12/93	MDA - use cvt[Long/Float]ToString routines instead of sprintf()
 10/13/92	MDA - add userData field
 5/23/92	Changed the widget class name so that it is preceded
		by 'xc' with the first major word capitalized.
 10/22/91	Created.

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


/* Declare widget methods */
static void ClassInitialize();
static void Initialize();
static Boolean SetValues();

/* Declare widget class functions */
static void CvtStringToDType();
static void CvtStringToVType();
static void CvtStringToValueJustify();



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
    (WidgetClass) &controlClassRec,		/* superclass */
    "Value",					/* class_name */
    sizeof(ValueRec),				/* widget_size */
    ClassInitialize,				/* class_initialize */
    NULL,					/* class_part_initialize */
    FALSE,					/* class_inited */
    Initialize,					/* initialize */
    NULL,					/* initialize_hook */
    XtInheritRealize,				/* realize */
    NULL,					/* actions */
    0,						/* num_actions */
    resources,					/* resources */
    XtNumber(resources),			/* num_resources */
    NULLQUARK,					/* xrm_class */
    TRUE,					/* compress_motion */
    TRUE,					/* compress_exposure */
    TRUE,					/* compress_enterleave */
    TRUE,					/* visible_interest */
    NULL,					/* destroy */
    NULL,					/* resize */
    NULL,					/* expose */
    SetValues,					/* set_values */
    NULL,					/* set_values_hook */
    XtInheritSetValuesAlmost,			/* set_values_almost */
    NULL,					/* get_values_hook */
    NULL,					/* accept_focus */
    XtVersion,					/* version */
    NULL,					/* callback_private */
    NULL,					/* tm_table */
    NULL,					/* query_geometry */
    NULL,					/* display_accelerator */
    NULL,					/* extension */
  },
  {
  /* Control class part */
    0,						/* dummy_field */
  },
  {
  /* Value class part */
    0,						/* dummy_field */
  },
};

WidgetClass xcValueWidgetClass = (WidgetClass)&valueClassRec;


/* Widget method function definitions */

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

}  /* end of ClassInitialize */




/*******************************************************************
 NAME:		Initialize.		
 DESCRIPTION:
   This is the initialize method for the Value widget.  It 
validates user-modifiable instance resources and initializes private 
widget variables and structures.  

*******************************************************************/

static void Initialize(request, new)
ValueWidget request, new;
{

DPRINTF(("Value: executing Initialize \n"));
/*
 * Validate public instance variable settings.
 */
   if ((new->value.datatype != XcLval) &&
       (new->value.datatype != XcHval) &&
       (new->value.datatype != XcFval))
   {
      XtWarning("Value: invalid datatype setting.");
      new->value.datatype = XcLval;
   }

/* Check decimals setting */
   if ((new->value.datatype == XcFval) && 
        ((new->value.decimals < MIN_DECIMALS) ||
         (new->value.decimals > MAX_DECIMALS)))
   {
      XtWarning("Value: invalid decimals setting.");
      new->value.decimals = 2;
   }

/* Check increment setting */
   if (((new->value.datatype == XcLval) || (new->value.datatype == XcHval))
				&& (new->value.increment.lval <= 0))
   {
      XtWarning("Value: invalid increment setting.");
      new->value.increment.lval = 1L;
   } 
   else if ((new->value.datatype == XcFval) &&  
			(new->value.increment.fval <= 0.0)) 
   {
      XtWarning("Value: invalid increment setting.");
      new->value.increment.fval = 1.0;
   } 

/* Check lower/upper bound setting */
   if (((new->value.datatype == XcLval) || 
       (new->value.datatype == XcHval)) && 
	(new->value.lower_bound.lval >= 
		new->value.upper_bound.lval)) 
   {
      XtWarning("Value: invalid lowerBound/upperBound setting.");
      new->value.lower_bound.lval = 
	(new->value.upper_bound.lval - new->value.increment.lval);
   } 
   else if ((new->value.datatype == XcFval) && 
	(new->value.lower_bound.fval >= 
		new->value.upper_bound.fval)) 
   {
      XtWarning("Value: invalid lowerBound/upperBound setting.");
      new->value.lower_bound.fval = 
	(new->value.upper_bound.fval - new->value.increment.fval);
   } 


/* Check the initial value setting */
   if ((new->value.datatype == XcLval)  || (new->value.datatype == XcHval))
   {
      if (new->value.val.lval < new->value.lower_bound.lval)
         new->value.val.lval = new->value.lower_bound.lval;
      else if (new->value.val.lval > new->value.upper_bound.lval)
         new->value.val.lval = new->value.upper_bound.lval; 
   }
   else if (new->value.datatype == XcFval) 
   {
      if (new->value.val.fval < new->value.lower_bound.fval)
         new->value.val.fval = new->value.lower_bound.fval;
      else if (new->value.val.fval > new->value.upper_bound.fval)
         new->value.val.fval = new->value.upper_bound.fval; 
   }

/* Check the initial valueJustify setting. */
   if ((new->value.justify != XcJustifyLeft) &&
          (new->value.justify != XcJustifyRight) &&
          (new->value.justify != XcJustifyCenter))
   {
      XtWarning("Value: invalid valueJustify setting.");
      new->value.justify = XcJustifyCenter;
   }

DPRINTF(("Value: done Initialize \n"));

}  /* end of Initialize */





/*******************************************************************
 NAME:		SetValues.
 DESCRIPTION:
   This is the set_values method for this widget. It validates resource
settings set with XtSetValues. If a resource is changed that would
require re-drawing the widget, return True.

*******************************************************************/

static Boolean SetValues(cur, req, new)
ValueWidget cur, req, new;
{
/* Local variables */
Boolean do_redisplay = False;


DPRINTF(("Value: executing SetValues \n"));

/* Validate new resource settings. */

/* Check the widget's color resources. */
   if ((new->value.value_fg_pixel != cur->value.value_fg_pixel) ||
       (new->value.value_fg_pixel != cur->value.value_fg_pixel)) 
      do_redisplay = True;

/* Check the datatype */
   if (new->value.datatype != cur->value.datatype)
   {
      do_redisplay = True;
      if ((new->value.datatype != XcLval) &&
          (new->value.datatype != XcHval) &&
          (new->value.datatype != XcFval))
      {
         XtWarning("Value: invalid datatype setting.");
         new->value.datatype = XcLval;
      }
   }
   
/* Check the decimals setting */
   if (new->value.decimals != cur->value.decimals)
   {
      do_redisplay = True;
      if ((new->value.decimals < MIN_DECIMALS) || 
		(new->value.decimals > MAX_DECIMALS))
      {
	 XtWarning("Value: invalid decimals setting.");
	 new->value.decimals = 2;
      }
   }

/* Check the increment setting */
   if (((new->value.datatype == XcLval) || (new->value.datatype == XcHval))
				&& (new->value.increment.lval <= 0))
   {
      XtWarning("Value: invalid increment setting.");
      new->value.increment.lval = 1L;
   } 
   else if ((new->value.datatype == XcFval) &&  
			(new->value.increment.fval <= 0.0)) 
   {
      XtWarning("Value: invalid increment setting.");
      new->value.increment.fval = 1.0;
   } 

/* Check the lowerBound/upperBound setting */
   if (((new->value.datatype == XcLval) || 
       (new->value.datatype == XcHval)) && 
	(new->value.lower_bound.lval >= 
		new->value.upper_bound.lval)) 
   {
      XtWarning("Value: invalid lowerBound/upperBound setting.");
      new->value.lower_bound.lval = 
	(new->value.upper_bound.lval - new->value.increment.lval);
   }
   else if ((new->value.datatype == XcFval) && 
	(new->value.lower_bound.fval >= 
		new->value.upper_bound.fval)) 
   {
      XtWarning("Value: invalid lowerBound/upperBound setting.");
      new->value.lower_bound.fval = 
	(new->value.upper_bound.fval - new->value.increment.fval);
   }


/* Check the new value setting */
   if (((new->value.datatype == XcLval) || (new->value.datatype == XcHval)) && 
		(new->value.val.lval != cur->value.val.lval))
   {
      do_redisplay = True;
      if (new->value.val.lval < new->value.lower_bound.lval)
         new->value.val.lval = new->value.lower_bound.lval;
      else if (new->value.val.lval > new->value.upper_bound.lval)
         new->value.val.lval = new->value.upper_bound.lval; 
   }
   if ((new->value.datatype == XcFval)  &&
		(new->value.val.fval != cur->value.val.fval))
   {
      do_redisplay = True;
      if (new->value.val.fval < new->value.lower_bound.fval)
         new->value.val.fval = new->value.lower_bound.fval;
      else if (new->value.val.fval > new->value.upper_bound.fval)
         new->value.val.fval = new->value.upper_bound.fval; 
   }

/* Check the valueJustify setting. */
   if (new->value.justify != cur->value.justify)
   {
      do_redisplay = True;
      if ((new->value.justify != XcJustifyLeft) &&
          (new->value.justify != XcJustifyRight) &&
          (new->value.justify != XcJustifyCenter))
      {
         XtWarning("Value: invalid valueJustify setting.");
         new->value.justify = XcJustifyCenter;
      }
   }

DPRINTF(("Value: done SetValues\n"));

   return do_redisplay;


}  /* end of SetValues */




/* Widget class functions */

/*******************************************************************
 NAME:		CvtStringToDType.		
 DESCRIPTION:
   This function converts resource settings in string form to the
XtRDType representation type.

*******************************************************************/

static void CvtStringToDType(args, num_args, fromVal, toVal)
XrmValuePtr args;		/* unused */
Cardinal *num_args;		/* unused */
XrmValuePtr fromVal;
XrmValuePtr toVal;
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
   if (strcmp(lowerstring, XcElval) == 0) 
   {
      datatype = XcLval;
      CvtDone(XcDType, &datatype);
   }
   else if (strcmp(lowerstring, XcEhval) == 0) 
   {
      datatype = XcHval;
      CvtDone(XcDType, &datatype);
   }
   else if (strcmp(lowerstring, XcEfval) == 0) 
   {
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

}  /* end of CvtStringToDType */




/*******************************************************************
 NAME:		CvtStringToVType.		
 DESCRIPTION:
   This function converts resource settings in string form to the
XtRVType representation type.

*******************************************************************/

static void CvtStringToVType(args, num_args, fromVal, toVal)
XrmValuePtr args;		/* unused */
Cardinal *num_args;		/* unused */
XrmValuePtr fromVal;
XrmValuePtr toVal;
{
/* Local variables */
long num;
static XcVType value;
char temp[30], temp2[30];

/*
 * Make sure the string is a numeric constant. If it isn't, print
 * a warning, and NULL the destination value.
 */
   if ((num = sscanf((char *)fromVal->addr, 
			"%[-012345689ABCDEFabcdef.]", temp)) == (int)NULL)
   {
      XtStringConversionWarning(fromVal->addr, "XcVType");
      CvtDone(char, NULL);
   }

/* If the number contains a decimal point, convert it into a float. */
   if (strchr(temp, '.') != NULL)
   {
      sscanf(temp, "%f", &value.fval); 
      CvtDone(XcVType, &value.fval);
   }

/* If it contains any hexadecimal characters, convert it to a long hex int */
   if (sscanf(temp, "%[ABCDEFabcdef]", temp2) != (int)NULL)
   {
      sscanf(temp, "%lX", &value.lval); 
      CvtDone(XcVType, &value.lval);
   }


/* Otherwise, it's an integer type. Convert the string to a numeric constant. */
   sscanf(temp, "%ld", &value.lval); 
   CvtDone(XcVType, &value.lval);
 
}  /* end of CvtStringToVType */




/*******************************************************************
 NAME:		CvtStringToValueJustify.		
 DESCRIPTION:
   This function converts resource settings in string form to the
XcRValueJustify representation type.

*******************************************************************/

static void CvtStringToValueJustify(args, num_args, fromVal, toVal)
XrmValuePtr args;		/* unused */
Cardinal *num_args;		/* unused */
XrmValuePtr fromVal;
XrmValuePtr toVal;
{
/* Local variables */
static XcValueJustify val_justify;
char lowerstring[100];

/* Convert the resource string to lower case for quick comparison */
   ToLower((char *)fromVal->addr, lowerstring);

/*
 * Compare resource string with valid XcValueJustify strings and assign to
 * datatype.
 */
   if (strcmp(lowerstring, XcEjustifyLeft) == 0) 
   {
      val_justify = XcJustifyLeft;
      CvtDone(XcValueJustify, &val_justify);
   }
   else if (strcmp(lowerstring, XcEjustifyRight) == 0) 
   {
      val_justify = XcJustifyRight;
      CvtDone(XcValueJustify, &val_justify);
   }
   else if (strcmp(lowerstring, XcEjustifyCenter) == 0) 
   {
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

}  /* end of CvtStringToValueJustify */


float Correlate(from_val, from_range, to_range)
/*************************************************************************
 * Correlate: This function takes the given value between the given      *
 *   range and correlates it to the destination range, returning the     *
 *   correlated value.                                                   *
 *************************************************************************/
  float from_val, from_range, to_range; {
/* Local variables */
float percent, result;

   percent = ((from_val * 100.0) / from_range);

/****** Clip to 0 and 100 */
   percent = MAX(0.,percent);
   percent = MIN(100.,percent);

   result = (to_range * (percent / 100));

   return result;

}


/*********************************************************************
 FUNCTION:	Print_value.
 DESCRIPTION:
  Simply converts a widget's value into a string based on the value's
datatype and returns a pointer to it.

*********************************************************************/

char *Print_value( XcDType datatype, XcVType *value, int decimals)
{
static char string[40];

   if (datatype == XcLval)
      cvtLongToString(value->lval,string);
   else if (datatype == XcHval)
      cvtLongToHexString(value->lval,string);
   else if (datatype == XcFval)
      cvtFloatToString(value->fval,string,(unsigned short)decimals);

   return &string[0];

}  /* end of Print_value */




/*********************************************************************
 FUNCTION:	Position_val.
 DESCRIPTION:
  This function positions the displayed value within the Value Box based
on the XcNvalueJustify resource setting.

*********************************************************************/

void Position_val(w)
  ValueWidget w;
{
/* Local variables */
char *val_string;
int text_width;

/* Establish the position of the value string within the Value Box. */
   val_string = Print_value(w->value.datatype, &w->value.val, 
							w->value.decimals);
   text_width = XTextWidth(w->control.font, val_string, strlen(val_string)); 

   if (w->value.justify == XcJustifyLeft)
      w->value.vp.x = w->value.value_box.x +
			(w->control.font->max_bounds.rbearing / 2);
   else if (w->value.justify == XcJustifyRight)
      w->value.vp.x = w->value.value_box.x +
      		w->value.value_box.width - text_width -
			(w->control.font->max_bounds.rbearing / 2);
   else if (w->value.justify == XcJustifyCenter)
      w->value.vp.x = w->value.value_box.x +
                (w->value.value_box.width / 2) -
                            (text_width / 2); 

   w->value.vp.y = w->value.value_box.y + 
		(int)(w->value.value_box.height -
		  (w->control.font->ascent + w->control.font->descent))/2
		   + w->control.font->ascent + 1;

}  /* end of Position_val */


/* end of Value.c */

