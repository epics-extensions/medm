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

/****** Class record declarations */
extern WidgetClass xcValueWidgetClass;
typedef struct _ValueClassRec *ValueWidgetClass;
typedef struct _ValueRec *ValueWidget;

/****** Widget class functions. */
extern float Correlate();
extern char *Print_value();
extern void Position_val();

#endif

