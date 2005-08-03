/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/* This header file is used to implement the Cartesian Plot using Xrt/Graph */

#ifndef __MEDMXRTGRAPH_H__
#define __MEDMXRTGRAPH_H__

#include <XrtGraph.h>
#if XRT_VERSION > 2
#ifdef XRT_EXTENSIONS
#include <XrtGraphProp.h>
#endif
#endif

#define CpDataType XrtDataType
#define CpDataHandle XrtDataHandle
#define CpData XrtData

#define CP_GENERAL XRT_GENERAL

#if XRT_VERSION > 2

#ifdef XRT_EXTENSIONS
static void destroyXrtPropertyEditor(Widget w, XtPointer, XtPointer);
#endif

#define CpDataCreate(widget, type, nsets, npoints) \
  XrtDataCreate(type, nsets, npoints)
#define CpDataGetXElement(hData, set, point) \
  XrtDataGetXElement(hData, set, point)
#define CpDataGetYElement(hData, set, point) \
  XrtDataGetYElement(hData, set, point)
#define CpDataDestroy(hData) \
  XrtDataDestroy(hData)
#define CpDataSetHole(hData, hole) \
  XrtDataSetHole(hData, hole)
#define CpDataSetXElement(hData, set, point, x) \
  XrtDataSetXElement(hData, set, point, x)
#define CpDataSetYElement(hData, set, point, y) \
  XrtDataSetYElement(hData, set, point, y)

#else      /* #if XRT_VERSION > 2 */

typedef CpData * XrtDataHandle;

#endif     /* #if XRT_VERSION > 2 */

#endif  /* __MEDMXRTGRAPH_H__ */
