/* Routines used to implement the Cartesian Plot using Xrt/Graph */

/* KE: Note that MEDM uses the union XcVType (of a float and a long) to convert
 *   float values to the values needed for XRT float resources.  This currently
 *   gives the same results as using the XRT recommended XrtFloatToArg(), but
 *   probably isn't guaranteed */

#define MAX(a,b)  ((a)>(b)?(a):(b))

#include "medmXrtGraph.h"

#if XRT_VERSION < 3
/* Routines to make XRT/graph backward compatible from Version 3.0 */

CpDataHandle CpDataCreate(CpDataType type, int nsets, int npoints) {
    return XrtMakeData(type,nsets,npoints,True);
}

int CpDataGetLastPoint(CpDataHandle hData, int set) {
    return (MAX(hData->g.data[set].npoints-1,0));
}

double CpDataGetXElement(CpDataHandle hData, int set, int point) {
    return hData->g.data[set].xp[point];
}

double CpDataGetYElement(CpDataHandle hData, int set, int point) {
    return hData->g.data[set].yp[point];
}

void CpDataDestroy(CpDataHandle hData) {
    if(hData) XrtDestroyData(hData,True);
}

int CpDataSetHole(CpDataHandle hData, double hole) {
    hData->g.hole = hole;
    return 1;
}

int CpDataSetLastPoint(CpDataHandle hData, int set, int point) {
    hData->g.data[0].npoints = point+1;
    return 1;
}

int CpDataSetXElement(CpDataHandle hData, int set, int point, double x) {
    hData->g.data[set].xp[point] = x;
    return 1;
}

int CpDataSetYElement(CpDataHandle hData, int set, int point, double y) {
    hData->g.data[set].yp[point] = y;
    return 1;
}
#endif
