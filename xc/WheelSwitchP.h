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

#ifndef __WHEELSWITCHP_H
#define __WHEELSWITCHP_H

#include <Xm/Xm.h>
#include <Xm/XmP.h>
#include <Xm/ManagerP.h>

/* C widget type definition */

typedef struct _WswRec *WswWidget;

typedef struct {
    WswWidget widget;
    XEvent *event;
} WswCallData;

/* New fields for the widget record */

typedef struct {
  /* Settable through resources */
    Boolean conform_to_content;
    Boolean disable_input;
    Boolean highlight_on_enter;
    short margin_width;   /* Margin around widget inside shadow */
    short margin_height;

  /* Shadows, in addition to what is in Manager */
    unsigned int shadow_type;

    char *format;
    char *font_pattern;
    XmFontList font_list;

    double *value;
    double *min_value;
    double *max_value;

    int repeat_interval;
    int callback_delay;

    XtCallbackList value_changed_callback;

  /* Derived and private */
    double format_min_value;
    double format_max_value;
    GC foreground_GC;   /* GC for text */
    Pixmap pixmap;
    XRectangle clip_rect;
    Boolean GC_inited;
    short nb_digit;
    Boolean user_font;
    XFontStruct *font;

    double *increments;
    int digit_number;
    int prefix_size;
    int digit_size;
    int format_size;
    int postfix_size;
    int point_position;
    Widget *up_buttons;
    Widget *down_buttons;
    Boolean has_focus;  /* wsw has focus */
    int selected_digit;
    XtIntervalId pending_timeout_id;
    XtIntervalId callback_timeout_id;
    Position prefix_x;
    Position digit_x;
    Position postfix_x;
    Position up_button_y;
    Position digit_y;
    Position string_base_y;
    Position down_button_y;
    Dimension digit_width;

    char *kbd_format;
    char *kbd_value;
    XtIntervalId blink_timeout_id;
    Boolean to_clear;
} WswPart;

/* New instance declaration */

typedef struct _WswRec {
    CorePart core;
    CompositePart composite;
    ConstraintPart constraint;
    XmManagerPart manager;
    WswPart wsw;
} WswRec;

/* New fields for the widget class record */

typedef struct {
    XtPointer extension;
} WswClassPart;

/* Full class record declaration */
typedef struct _WswClassRec {
    CoreClassPart core_class;
    CompositeClassPart composite_class;
    ConstraintClassPart constraint_class;
    XmManagerClassPart manager_class;
    WswClassPart wsw_class;
} WswClassRec;

/* Class record variable */
extern  WswClassRec wswClassRec;

#endif  /* #ifndef __WHEELSWITCHP_H */
