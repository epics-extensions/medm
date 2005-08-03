/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/****************************************************************************
 * File    : Byte.h                                                         *
 * Author  : David M. Wetherholt - DMW Software 1994                        *
 * Lab     : Continuous Electron Beam Accelerator Facility                  *
 * Modified: 7 July 94                                                      *
 * Mods    : 1.0 Created                                                    *
 ****************************************************************************/

#ifndef __XC_BYTE_H
#define __XC_BYTE_H

/****** Superclass header */
#include "Value.h"

/****** Define widget resource names, classes, and representation types.
        Use these resource strings in your resource files */
#define XcNbyteBackground	"byteBackground"
#define XcNbyteForeground	"byteForeground"
#define XcNsBit          	"startBit"
#define XcCsBit          	"startBit"
#define XcNeBit          	"endBit"
#define XcCeBit          	"endBit"

/****** Class record declarations */
extern WidgetClass xcByteWidgetClass;
typedef struct _ByteClassRec *ByteWidgetClass;
typedef struct _ByteRec *ByteWidget;

/****** Widget functions */
void XcBYUpdateValue(Widget w, XcVType *value);
void XcBYUpdateByteForeground(Widget w, Pixel pixel);

#endif

