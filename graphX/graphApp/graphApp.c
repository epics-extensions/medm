/**
 ** Application programmer Layer to graphX library
 **
 **   This layer provides easy use of graphX X plotting functions, and
 **   requires no particular familiarity with X.  Display connections
 **   and widget/window creation with some default translations and
 **   callbacks is provided.
 **
 **   Since it would be foolish to do otherwise, the Intrinsics, along
 **   with the Motif widget set, are utilized.
 **/


#include "GraphApp.h"
#include <sys/time.h>



/* 
 * TRANSLATIONS
 */

static void printPlot(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
/*
 *  do the internal xwd/xwd2ps pair...
 */
  char *commandBuffer;
  char labelString[28];
  struct timeval tp;
  struct timezone tzp;

  utilPrint(XtDisplay(w),XtWindow(w),XWD_TEMP_FILE_STRING);

}


static void quit(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
  exit(0);
}




/*
 * CALLBACKS 
 */


static void redisplayGraphAppSeql(w,seql,call_data)
  Widget w;
  Seql *seql;
  XtPointer call_data;
{
  seqlRefresh(seql);	
  seqlDrawLegend(seql);
}
static void resizeGraphAppSeql(w,seql,call_data)
  Widget w;
  Seql *seql;
  XtPointer call_data;
{
  seqlResize(seql);
  seqlDraw(seql);
  seqlDrawLegend(seql);
}
static void destroyGraphAppSeql(w,seql,call_data)
  Widget w;
  Seql *seql;
  XtPointer call_data;
{
  seqlTerm(seql);
}



static void redisplayGraphAppGraph(w,graph,call_data)
  Widget w;
  Graph *graph;
  XtPointer call_data;
{
  graphRefresh(graph);	
  graphDrawLegend(graph);
}
static void resizeGraphAppGraph(w,graph,call_data)
  Widget w;
  Graph *graph;
  XtPointer call_data;
{
  graphResize(graph);
  graphDraw(graph);
  graphDrawLegend(graph);
}
static void destroyGraphAppGraph(w,graph,call_data)
  Widget w;
  Graph *graph;
  XtPointer call_data;
{
  graphTerm(graph);
}



static void redisplayGraphAppStrip(w,strip,call_data)
  Widget w;
  Strip *strip;
  XtPointer call_data;
{
  stripRefresh(strip);	
  stripDrawLegend(strip);
}
static void resizeGraphAppStrip(w,strip,call_data)
  Widget w;
  Strip *strip;
  XtPointer call_data;
{
  stripResize(strip);
  stripDraw(strip);
  stripDrawLegend(strip);
}
static void destroyGraphAppStrip(w,strip,call_data)
  Widget w;
  Strip *strip;
  XtPointer call_data;
{
  stripTerm(strip);
}



static void redisplayGraphAppGraph3d(w,graph3d,call_data)
  Widget w;
  Graph3d *graph3d;
  XtPointer call_data;
{
  graph3dRefresh(graph3d);	
}
static void resizeGraphAppGraph3d(w,graph3d,call_data)
  Widget w;
  Graph3d *graph3d;
  XtPointer call_data;
{
  graph3dResize(graph3d);
  graph3dDraw(graph3d);
}
static void destroyGraphAppGraph3d(w,graph3d,call_data)
  Widget w;
  Graph3d *graph3d;
  XtPointer call_data;
{
  graph3dTerm(graph3d);
}



static void redisplayGraphAppSurface(w,surface,call_data)
  Widget w;
  Surface *surface;
  XtPointer call_data;
{
  surfaceRefresh(surface);	
}
static void resizeGraphAppSurface(w,surface,call_data)
  Widget w;
  Surface *surface;
  XtPointer call_data;
{
  surfaceResize(surface);
  surfaceDraw(surface);
}
static void destroyGraphAppSurface(w,surface,call_data)
  Widget w;
  Surface *surface;
  XtPointer call_data;
{
  surfaceTerm(surface);
}


static char defaultTranslations[] = 
		"Ctrl<Key>p: printPlot()    \n\
		 Ctrl<Key>q: quit() ";

static XtActionsRec actionsTable[] = {
	{"printPlot", (XtActionProc)printPlot},
	{"quit", (XtActionProc)quit},
};





/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Initialize the GraphApp structure and connect to the specified 
 *    or default X server
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


GraphApp *graphAppInit(displayName,argc,argv)
  char *displayName;
  int *argc;			/* pointer to argc from main */
  char **argv;			/* the argv from main */
{

int zero = 0;
GraphApp *graphApp;


/*
 * Allocate an instance of GraphApp
 */
   graphApp = (GraphApp *) malloc(sizeof(GraphApp));
   if (!graphApp) return(NULL);


/*
 * Initialize the Intrinsics right away
 *   and try application contexts approach to shells
 */
   XtToolkitInitialize();
   graphApp->appContext = XtCreateApplicationContext();
   graphApp->display = XtOpenDisplay(graphApp->appContext,displayName,
		"graphApp", "GraphApp", (XrmOptionDescRec *)NULL,
		0,argc,argv);
   if (graphApp->display == NULL) {
      fprintf(stderr,"\ngraphAppInit: Can't open display - %s\n",displayName);
      return(NULL);
   }
   graphApp->screen  = DefaultScreen(graphApp->display);
   graphApp->shellPtr = 0;

   return(graphApp);
}




/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Allocate a shell and register the appropriate translations
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


int graphAppInitShell(graphApp, height, width, shellName)
  GraphApp *graphApp;
  int height, width;
  char *shellName;
{
  Arg args[5];
  int n;
  XtTranslations transTable;

  if (graphApp == NULL) return(-1);

   if (graphApp->shellPtr >= MAX_SHELLS) {
	fprintf(stderr,
	  "\ngraphAppInitShell: unable to allocate more shells!");
	fprintf (stderr,
	  "\n                   maximum of %d already allocated!\n",
	  graphApp->shellPtr);
	return (-1);
   }

   XtSetArg(args[0],XtNiconName,shellName);
   graphApp->shellInfo[graphApp->shellPtr].shell = 
		XtAppCreateShell("graphApp", "GraphApp", 
			applicationShellWidgetClass,
			graphApp->display,args,1);



/*
 * Register new actions and compile translations table
 */
   XtAppAddActions(graphApp->appContext,
			actionsTable,XtNumber(actionsTable));
   transTable = XtParseTranslationTable(defaultTranslations);


/*
 * setup a canvas widget to draw in
 */
   n = 0;
   XtSetArg(args[n], XmNwidth, width); n++;
   XtSetArg(args[n], XmNheight, height); n++;

   graphApp->shellInfo[graphApp->shellPtr].canvas = 
	XtCreateManagedWidget("canvas", xmDrawingAreaWidgetClass,
			 	graphApp->shellInfo[graphApp->shellPtr].shell,
			 	args, n);

/*
 * merge program-defined translations with existing translations
 */
   XtOverrideTranslations(graphApp->shellInfo[graphApp->shellPtr].canvas,
				transTable);
   XtOverrideTranslations(graphApp->shellInfo[graphApp->shellPtr].shell,
				transTable);

/*
 *  need to realize the widgets for windows to be created...
 */
   XtRealizeWidget(graphApp->shellInfo[graphApp->shellPtr].shell);

   graphApp->shellInfo[graphApp->shellPtr].window = 
	XtWindow(graphApp->shellInfo[graphApp->shellPtr].canvas);


   graphApp->shellPtr++;

   return(graphApp->shellPtr-1);

}



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Register the type of and pointer to graphic in the specified graphApp
 *
 *   based on the specified type, the expose and resize callbacks can
 *   now be registered
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


void graphAppRegisterGraphic(graphApp, shellNumber, graphType, graphicPtr)
  GraphApp *graphApp;
  int shellNumber;
  GraphXType graphType;
  XtPointer graphicPtr;
{
  if (graphApp == NULL) return;

  graphApp->shellInfo[shellNumber].graphType = graphType;
  graphApp->shellInfo[shellNumber].graphicPtr = graphicPtr;


/*
 * this is sort of ugly and verbose, but most other solutions require
 *   some assumptions about the way the compiler handles the enumerated
 *   types, and we really don't want to rely on that...
 */

  switch(graphType) {

     case SEQL_TYPE:
   	   XtAddCallback(graphApp->shellInfo[shellNumber].canvas, 
			XmNexposeCallback,
			(XtCallbackProc)redisplayGraphAppSeql, graphicPtr);
	   XtAddCallback(graphApp->shellInfo[shellNumber].canvas,
			XmNresizeCallback,
			(XtCallbackProc)resizeGraphAppSeql, graphicPtr);
	   XtAddCallback(graphApp->shellInfo[shellNumber].canvas,
			XmNdestroyCallback,
			(XtCallbackProc)destroyGraphAppSeql, graphicPtr);
	   break;


     case GRAPH_TYPE:
   	   XtAddCallback(graphApp->shellInfo[shellNumber].canvas, 
			XmNexposeCallback,
			(XtCallbackProc)redisplayGraphAppGraph, graphicPtr);
	   XtAddCallback(graphApp->shellInfo[shellNumber].canvas,
			XmNresizeCallback,
			(XtCallbackProc)resizeGraphAppGraph, graphicPtr);
	   XtAddCallback(graphApp->shellInfo[shellNumber].canvas,
			XmNdestroyCallback,
			(XtCallbackProc)destroyGraphAppGraph, graphicPtr);
	   break;


     case STRIP_TYPE:
	   stripSetAppContext(graphicPtr,graphApp->appContext);
   	   XtAddCallback(graphApp->shellInfo[shellNumber].canvas, 
			XmNexposeCallback,
			(XtCallbackProc)redisplayGraphAppStrip, graphicPtr);
	   XtAddCallback(graphApp->shellInfo[shellNumber].canvas,
			XmNresizeCallback,
			(XtCallbackProc)resizeGraphAppStrip, graphicPtr);
	   XtAddCallback(graphApp->shellInfo[shellNumber].canvas,
			XmNdestroyCallback,
			(XtCallbackProc)destroyGraphAppStrip, graphicPtr);
	   break;


     case GRAPH3D_TYPE:
   	   XtAddCallback(graphApp->shellInfo[shellNumber].canvas, 
			XmNexposeCallback,
			(XtCallbackProc)redisplayGraphAppGraph3d,graphicPtr);
	   XtAddCallback(graphApp->shellInfo[shellNumber].canvas,
			XmNresizeCallback,
			(XtCallbackProc)resizeGraphAppGraph3d, graphicPtr);
	   XtAddCallback(graphApp->shellInfo[shellNumber].canvas,
			XmNdestroyCallback,
			(XtCallbackProc)destroyGraphAppGraph3d, graphicPtr);
	   break;


     case SURFACE_TYPE:
   	   XtAddCallback(graphApp->shellInfo[shellNumber].canvas, 
			XmNexposeCallback,
			(XtCallbackProc)redisplayGraphAppSurface,graphicPtr);
	   XtAddCallback(graphApp->shellInfo[shellNumber].canvas,
			XmNresizeCallback,
			(XtCallbackProc)resizeGraphAppSurface, graphicPtr);
	   XtAddCallback(graphApp->shellInfo[shellNumber].canvas,
			XmNdestroyCallback,
			(XtCallbackProc)destroyGraphAppSurface, graphicPtr);
	   break;


  }

}






/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Do the XtAppMainLoop call with graphApp->appContext
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


void graphAppLoop(graphApp)
  GraphApp *graphApp;
{

    if (graphApp == NULL) return;
    XtAppMainLoop(graphApp->appContext);


}



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * De-allocate the specified shell and window
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


void graphAppTermShell(graphApp, shellNumber)
  GraphApp *graphApp;
  int shellNumber;
{
   if (graphApp == NULL) return;
   XtDestroyWidget(graphApp->shellInfo[shellNumber].shell);
}




/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *
 * Terminate the application and the X connection
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


void graphAppTerm(graphApp)
  GraphApp *graphApp;
{
  if (graphApp == NULL) return;
  XtDestroyApplicationContext(graphApp->appContext);
  XtCloseDisplay(graphApp->display);
  free ( (char *) graphApp);
}



