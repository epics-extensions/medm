/*******************************************************************
 FILE:		Value.h
 CONTENTS:	Public header file for the Value widget class.
 AUTHOR:	Paul D. Johnston
 HISTORY:
 Date		Action
 ---------	------------------------------------
 5/23/92	Changed the widget class name so that it is preceded
		by 'xc' with the first major word capitalized.
 3/11/92	Created.

********************************************************************/

#ifndef __XC_VALUE_H
#define __XC_VALUE_H




/* Class record declarations */

extern WidgetClass xcValueWidgetClass;

typedef struct _ValueClassRec *ValueWidgetClass;
typedef struct _ValueRec *ValueWidget;


/* Widget class functions. */

extern float Correlate(float from_val, float from_range, float to_range);
extern char *Print_value( XcDType datatype, XcVType *value, int decimals);
extern void Position_val(ValueWidget w);


#endif /* __XC_VALUE_H */

