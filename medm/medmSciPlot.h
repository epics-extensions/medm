/* This header file is used to implement the Cartesian Plot using SciPLot */

#ifndef __MEDMSCIPLOT_H__
#define __MEDMSCIPLOT_H__

/* SciPlot needs MOTIF defined */
#ifndef MOTIF
#define MOTIF
#endif

#include <SciPlot.h>

#define CP_GENERAL 0

#define INVALID_LISTID -1

/* Structures */

typedef struct {
    int npoints;
    int lastPoint;
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
