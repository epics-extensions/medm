/**
 ** Application programmer Layer to graphX library
 **
 **   This layer provides easy use of graphX X plotting functions, and
 **   requires no particular familiarity with X.  Display connections
 **   and widget/window creation with some default translations and
 **   callbacks is provided.
 **
 **   Since it would be foolish to do otherwise, the Intrinsics, along
 **   with the Motif widget set, are used.
 **/

#ifndef __GRAPH_APP_H__
#define __GRAPH_APP_H__

#include <stdio.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <Xm/Xm.h>
#include <Xm/DrawingA.h>


/* 
 *  include the graphX type definitions 
 */
#include "GraphX.h"


#undef XWD_TEMP_FILE_STRING
#define XWD_TEMP_FILE_STRING "./graphApp.xwd"


#define GraphAppInfo(a,b) (a)->display,(a)->screen,((a)->shellInfo[(b)].window)

#define GraphAppDisplay(a) (a)->display


#define MAX_SHELLS	20


typedef enum {
	SEQL_TYPE,
	GRAPH_TYPE,
	STRIP_TYPE,
	GRAPH3D_TYPE,
	SURFACE_TYPE
} GraphXType;


typedef struct {
	Widget    shell;
	Widget    canvas;
	Window    window;
	GraphXType graphType;
	XtPointer   graphicPtr;		/* pointer to arbitrary graphX type */
} ShellInfo;

typedef struct {
	XtAppContext	appContext;
	Display		*display;
	int		screen;
	ShellInfo	shellInfo[MAX_SHELLS];
	int		shellPtr;
} GraphApp;


#ifdef _NO_PROTO
extern GraphApp *graphAppInit();
extern int graphAppInitShell();
extern void graphAppRegisterGraphic();
extern void graphAppLoop();
extern void graphAppTermShell();
extern void graphAppTerm();
#else
extern GraphApp *graphAppInit(char *, int *, char **);
extern int graphAppInitShell(GraphApp *, int, int, char *);
extern void graphAppRegisterGraphic(GraphApp *, int, GraphXType, XtPointer);
extern void graphAppLoop(GraphApp *);
extern void graphAppTermShell(GraphApp *, int);
extern void graphAppTerm(GraphApp *);
#endif

#endif  /*__GRAPH_APP_H__ */
