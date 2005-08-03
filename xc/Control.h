/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*******************************************************************
 FILE:		Control.h
 CONTENTS:	Public header file for the Control widget class.
 AUTHOR:	Paul D. Johnston
 HISTORY:
 Date		Action
 ---------	------------------------------------
 5/23/92	Changed the widget class name so that it is preceded
		by 'xc' with the first major word capitalized.
 3/11/92	Created.

********************************************************************/

#ifndef __XC_CONTROL_H
#define __XC_CONTROL_H


/* Debug printf statements for Control Panel widget development. */
#ifdef XCDEBUG
#define DPRINTF(statement)  printf statement
#else
#define DPRINTF(statement)
#endif

/*
 * This macro is used by the resource string conversion routines to
 * assign the converted type elements to the XrmValue struct.
 */
#define CvtDone(type, address) \
  toVal->size = sizeof(type); \
  toVal->addr = (XtPointer)address; \
  return

/*
 * This enumeration defines the possible 3D rectangle types, used in the
 * call to Rect3d().
 */
typedef enum rect3d_t
{
    RAISED,
    DEPRESSED
} Type3d;

/* Type used to indicate the Arrow direction (i.e., up, down, etc.) */
typedef enum
{
    UP,
    DOWN,
    LEFT,
    RIGHT
} ArrowType;


/* Class record declarations */

extern WidgetClass xcControlWidgetClass;

typedef struct _ControlClassRec *ControlWidgetClass;
typedef struct _ControlRec *ControlWidget;


/* Function prototypes for widget methods */
Boolean Point_In_Rect(int x, int y, int rect_x, int rect_y,
  int width, int height);
void Rect3d(Widget w, Display *display, Drawable drawable,
  GC gc, int x, int y, unsigned int width, unsigned int height,
  Type3d type);
void ToLower(char *source, char *dest);
void CvtStringToOrient(XrmValuePtr args, Cardinal *nargs,
  XrmValuePtr fromVal, XrmValuePtr toVal);
void Arrow3d(Widget w, Display *display,Drawable drawable, GC gc,
  XRectangle *bounds, ArrowType orientation,Type3d type);

#endif /* __XC_CONTROL_H */
