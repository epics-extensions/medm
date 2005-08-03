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

#ifndef __WHEELSWITCH_H
#define __WHEELSWITCH_H

/* New arguments for this widget */
#define XmNcallbackDelay       "callbackDelay"
#define XmNconformToContent    "conformToContent"
#define XmNcurrentValue        "currentValue"
#define XmNmaxValue            "maxValue"
#define XmNminValue            "minValue"
#define XmNfontPattern         "fontPattern"
#define XmNformat              "format"
#define XmNrepeatInterval      "repeatInterval"
#define XmNdisableInput        "disableInput"

/* New argument Classes for this Widget */
#define XmCCallbackDelay       "CallbackDelay"
#define XmCCurrentValue        "CurrentValue"
#define XmCMinValue            "MinValue"
#ifndef XmCMaxValue
# define XmCMaxValue           "MaxValue"
#endif
#define XmCRepeatInterval      "RepeatInterval"
#define XmCfontPattern         "FontPattern"
#define XmCformat              "Format"
#define XmCDisableInput        "DisableInput"

/* Class record pointer */
extern WidgetClass wheelSwitchWidgetClass;

/* New data structure */
typedef struct {
    int reason;
    XEvent *event;
    double *value;
} XmWheelSwitchCallbackStruct;

/* Convenience functions */

extern Widget XmCreateWheelSwitch(Widget parent_widget, char *name,
  ArgList arglist, int num_args);
extern Widget XmWheelSwitch(Widget parent_widget, char *name, Position x,
  Position y, double *value, char *format, double *min_value, double *max_value,
  XtCallbackList callback, XtCallbackList help_callback);
extern void XmWheelSwitchSetValue(Widget w, double *value);
extern void XmWheelSwitchSetForeground(Widget w, Pixel foreground);
extern void XmWheelSwitchSetBackground(Widget w, Pixel background);

#endif /* #ifndef __WHEELSWITCH_H */
