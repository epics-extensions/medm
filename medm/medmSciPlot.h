/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/* This header file is used to implement the Cartesian Plot using SciPLot */

#ifndef __MEDMSCIPLOT_H__
#define __MEDMSCIPLOT_H__

#include "SciPlot.h"

#define CP_GENERAL 0

#define INVALID_LISTID -1

/* Structures */

typedef struct {
    int npoints;
    int pointsUsed;
    int listid;
    float *xp;
    float *yp;
} CpDataSet;

typedef struct {
    int       nsets;
    CpDataSet *data;
} CpData, *CpDataHandle;

typedef int CpDataType;

#endif  /* __MEDMSCIPLOT_H__ */
