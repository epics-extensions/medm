/**
 ** Overall Include for graphX library
 **
 **/

#ifndef __GRAPH_X_H__
#define __GRAPH_X_H__

/* 
 *  include the graphX type definitions 
 */
#include "GraphXMacros.h"

#include "Seql.h"
#include "Graph.h"
#include "Strip.h"
#include "Graph3d.h"
#include "Surface.h"

#ifdef _NO_PROTO
extern char *graphXGetBestFont();
#else
extern char *graphXGetBestFont(Display *, char *, char *, char *, int);
#endif

#endif  /*__GRAPH_X_H__ */
