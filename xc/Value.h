/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
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
float Correlate(float from_val, float from_range, float to_range);
char *Print_value(XcDType datatype, XcVType *value, int decimals);
void Position_val(Widget w);

#endif
