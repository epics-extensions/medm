/*************************************************************************\
* Copyright (c) 2004 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2004 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
/* Based on                 */
/* WheelSwitch widget class */
/* F. Di Maio - CERN 1990   */

#ifndef _PswWheelSwitch
#define _PswWheelSwitch

/* New arguments for this widget */
#define XmNcallbackDelay       "callbackDelay"
#define XmNconformToContent    "conformToContent"
#define XmNcurrentValue        "currentValue"
#define XmNcurrentValues       "currentValues"
#define XmNfontPattern         "fontPattern"
#define XmNformat              "format"
#define XmNmaxValue            "maxValue"
#define XmNminValue            "minValue"
#define XmNrepeatInterval      "repeatInterval"

/* New argument Classes for this Widget */
#define XmCCallbackDelay       "CallbackDelay"
#define XmCFloat               "Float"
#define XmCReapeatInterval     "ReapeatInterval"
#define XmCfontPattern         "fontPattern"
#define XmCformat              "format"

/* Class record pointer */
extern WidgetClass wheelSwitchWidgetClass;

/* New data structure */
typedef struct {
    int reason;
    XEvent * event;
    double *value;
} XmWheelSwitchCallbackStruct;

/* New class method entry points */

extern void XmWheelSwitchSetValue(Widget wsw, double *value);

/* New creation entry points */

extern Widget XmCreateWheelSwitch(Widget parent_widget, char *name,
  ArgList arglist, int num_args);
extern Widget XmWheelSwitch(Widget parent_widget, char *name, Position x,
  Position y, double *value, char *format, double *min_value, double *max_value,
  XtCallbackList callback, XtCallbackList help_callback);

#endif /* #ifndef _PswWheelSwitch */
