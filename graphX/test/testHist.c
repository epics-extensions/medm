/**
 ** toplevel Graph Plotter test program
 **/


#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/PushB.h>
#include <Xm/BulletinB.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/Scale.h>
#include <Xm/RowColumn.h>
#include <Xm/List.h>

#include <stdio.h>

#include <math.h>		/* very Very VERY IMPORTANT ! */

#include "GraphX.h"             /* overall GraphX include file
                                   defines data types, plot types,
                                   functions and macros            */
 


#define WIDTH   400
#define HEIGHT  400

#define NBUFFERS 1
#define NPTS 20 
#define TWOPI 6.28


/*** 
 *** unfortunately have to keep this global for now (for the toggleAnimation
 *** routine to be able to update graph...)
 ***/

static XtWorkProc animateGraph();

static Graph *graph1;
static Graph *graph2;
static Graph *graph3;

/* for the application contexts... */
static XtAppContext app;

static Boolean animate = FALSE;
static Boolean zoomBoolean = FALSE;
static int numUpdate = 1;
static Boolean booleanSleep = FALSE;
static XtWorkProcId animateId = NULL;


/* 
 * TRANSLATIONS
 */


static void toggleGraphAnimation(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{


   if (animate) {
      animate = FALSE;
      XtRemoveWorkProc(animateId);
   } else {
      animate = TRUE;
      /* must call this after XtInitialize() */
      animateId = XtAppAddWorkProc(app,(XtWorkProc)animateGraph,NULL);
   }
}


static void toggleSleep(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
   booleanSleep = (!booleanSleep);
}


static void zoom(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
   if (zoomBoolean) {
      zoomBoolean = FALSE;
      graphSetRangeDefault(graph1,0,'X');
      graphSetRangeDefault(graph1,0,'Y');
      graphSetRangeDefault(graph2,0,'X');
      graphSetRangeDefault(graph2,0,'Y');
   } else {
      zoomBoolean = TRUE;
      graphSetRange(graph1,0,'X',-5.0,5.0);
      graphSetRange(graph1,0,'Y',-0.5,0.5);
      graphSetRange(graph2,0,'X',-5.0,5.0);
      graphSetRange(graph2,0,'Y',-0.5,0.5);
   }
   graphDraw(graph1);
   graphDraw(graph2);

}


static void printPlot(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
  graphPrint(graph1);
  graphPrint(graph2);
  graphPrint(graph3);
}


static void update1(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
   numUpdate = 1;
}


static void update2(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
   numUpdate = 2;
}


static void update3(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
   numUpdate = 3;
}



static void quit(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
/*
 * clean up our space like a good program
 */
  graphTerm(graph1);
  graphTerm(graph2);
  graphTerm(graph3);

  exit(0);
}




/*
 * CALLBACKS 
 */


static void redisplayGraph(w,graph,call_data)
  Widget w;
  Graph **graph;	/* double indirection since filling in actual */
  XtPointer call_data;	/* address of Graph structure after XtAddCallback */
{

  graphRefresh(*graph);	/* copy pixmap onto window (this is quick) */	

}


static void resizeGraph(w,graph,call_data)
  Widget w;
  Graph **graph;	/* double indirection since filling in actual */
  XtPointer call_data;	/* address of Graph structure after XtAddCallback */
{

  graphResize(*graph);
  graphDraw(*graph);

}




static XtWorkProc animateGraph()
{
int i;
double temp;

if (booleanSleep) sleep(1);

for (i=0; i<graph1->nValues[0]; i++){
   temp = (  (((double) random())/pow(2.0,31.0) > 0.5) ?
   	 - 0.2*((double) random())/pow(2.0,31.0) :
   	  0.2*((double) random())/pow(2.0,31.0) );

/* this will affect both since same storage used by both */
   graph1->dataBuffer[0][i].y += temp;
}

if (numUpdate == 1) {
   graphDraw(graph1);
} else if (numUpdate == 2) {
   graphDraw(graph1);
   graphDraw(graph2);
} else {
   graphDraw(graph1);
   graphDraw(graph2);
   graphDraw(graph3);
}

/* return FALSE so this WorkProc keeps getting called (just in case...) */
return FALSE;

}

static char defaultTranslations[] = 
		"Ctrl<Key>p: printPlot()    \n\
		 <Key>a: toggleGraphAnimation()    \n\
		 <Key>s: toggleSleep()    \n\
		 <Key>1: update1()    \n\
		 <Key>2: update2()    \n\
		 <Key>3: update3()    \n\
		 <Key>z: zoom()    \n\
		 <Key>q: quit() ";

static XtActionsRec actionsTable[] = {
	{"toggleGraphAnimation", (XtActionProc)toggleGraphAnimation},
	{"toggleSleep", (XtActionProc)toggleSleep},
	{"printPlot", (XtActionProc)printPlot},
	{"update1", (XtActionProc)update1},
	{"update2", (XtActionProc)update2},
	{"update3", (XtActionProc)update3},
	{"zoom", (XtActionProc)zoom},
	{"quit", (XtActionProc)quit},
};




main(argc,argv)
  int   argc;
  char *argv[];
{
  Widget topLevel1, topLevel2, topLevel3,  canvas1, canvas2, canvas3;
  Arg args[5];
  int n;

  XtTranslations transTable;

  int i, j;

  int nValues[NBUFFERS];
  XYdataPoint *data[NBUFFERS];

  Display *display = NULL;
  int screen;
  Window window1, window2, window3;

  static char *dataColor[] = {"magenta",};

/* these are filled in by graphXGetBestFont() calls */
  char *titleFont, *axesFont;
  int   titleFontSize, axesFontSize;



/*
 * first initialize the data buffer
 */
  for (i=0; i<NBUFFERS; i++)
       data[i] = (XYdataPoint *) malloc((unsigned) NPTS * sizeof(XYdataPoint));


/*
 * Initialize the Intrinsics right away
 * and make another toplevel shell (same application context)
 */

/*
 * try application contexts approach to shells
 */
   XtToolkitInitialize();
   app = XtCreateApplicationContext();

   display = XtOpenDisplay(app,NULL,argv[0], 
		"application context", NULL,0,&argc,argv);
   if (display == NULL) {
      fprintf(stderr,"\ntestGraph: Can't open display!\n");
      exit(1);
   }

   XtSetArg(args[0],XtNiconName,"Plot");
   topLevel1 = XtAppCreateShell("Test", "shell1", applicationShellWidgetClass,
		display,args,1);

   XtSetArg(args[0],XtNiconName,"Another plot");
   topLevel2 = XtAppCreateShell("Test", "shell2", applicationShellWidgetClass,
		display,args,1);

   XtSetArg(args[0],XtNiconName,"Yet Another plot");
   topLevel3 = XtAppCreateShell("Test", "shell3", applicationShellWidgetClass,
		display,args,1);



/*
 * Register new actions and compile translations table
 */
   XtAppAddActions(app,actionsTable,XtNumber(actionsTable));
   transTable = XtParseTranslationTable(defaultTranslations);


/*
 * setup a canvas widget to draw in
 */
   n = 0;
   XtSetArg(args[n], XmNwidth, WIDTH); n++;
   XtSetArg(args[n], XmNheight, HEIGHT); n++;

   canvas1 = XtCreateManagedWidget("canvas1", xmDrawingAreaWidgetClass,
					topLevel1, args, n);

   canvas2 = XtCreateManagedWidget("canvas2", xmDrawingAreaWidgetClass,
					topLevel2, args, n);

   canvas3 = XtCreateManagedWidget("canvas3", xmDrawingAreaWidgetClass,
					topLevel3, args, n);

/*
 * merge program-defined translations with existing translations
 */
   XtOverrideTranslations(canvas1,transTable);
   XtOverrideTranslations(canvas2,transTable);
   XtOverrideTranslations(canvas3,transTable);
   XtOverrideTranslations(topLevel1,transTable);
   XtOverrideTranslations(topLevel2,transTable);
   XtOverrideTranslations(topLevel3,transTable);

/*
 * add the expose and resize callbacks, passing pointer to graph as the data of 
 *   the callback
 */
   XtAddCallback(canvas1, XmNexposeCallback,
	(XtCallbackProc)redisplayGraph, &graph1);
   XtAddCallback(canvas1, XmNresizeCallback,
	(XtCallbackProc)resizeGraph, &graph1);
   XtAddCallback(canvas2, XmNexposeCallback,
	(XtCallbackProc)redisplayGraph, &graph2);
   XtAddCallback(canvas2, XmNresizeCallback,
	(XtCallbackProc)resizeGraph, &graph2);
   XtAddCallback(canvas3, XmNexposeCallback,
	(XtCallbackProc)redisplayGraph, &graph3);
   XtAddCallback(canvas3, XmNresizeCallback,
	(XtCallbackProc)resizeGraph, &graph3);


/*
 *  need to realize the widgets for windows to be created...
 */

   XtRealizeWidget(topLevel1);
   XtRealizeWidget(topLevel2);
   XtRealizeWidget(topLevel3);

   screen  = DefaultScreen(display);
   window1  = XtWindow(canvas1);
   window2  = XtWindow(canvas2);
   window3  = XtWindow(canvas3);


/*
 * now proceed with graph generation
 */

   graph1 = graphInit(display,screen,window1);
   graph2 = graphInit(display,screen,window2);
   graph3 = graphInit(display,screen,window3);

/*
 * (MDA) add zoom type event handling
 */
   graphSetInteractive(graph1,True);
   graphSetInteractive(graph2,True);
   graphSetInteractive(graph3,True);

   for (i=0; i<NBUFFERS; i++)
      for (j=0; j<NPTS; j++) {
	data[i][j].x = (double) (-TWOPI + 2.0*TWOPI*j/((NPTS-1)));
	data[i][j].y = cos(data[i][j].x);
        nValues[i] = NPTS;
      }


/* all the same size */
   titleFontSize = GraphX_TitleFontSize(graph1);
   titleFont = graphXGetBestFont(display,"times","bold","r", titleFontSize);
   axesFontSize = GraphX_AxesFontSize(graph1);
   axesFont = graphXGetBestFont(display,"times","medium","r",axesFontSize);


   /* setup the Graph, use the external-to-graph version of variables */
   graphSet(graph1, NBUFFERS, NPTS, data, nValues, GraphLine, 
     "An Animated Line Plot", 
     titleFont,
     "X Axis", "Y Axis",
     axesFont,
     "white", "black", dataColor, GraphExternal);

   graphSet(graph2, NBUFFERS, NPTS, data, nValues, GraphBar, 
     "An Animated Histogram", 
     titleFont,
     "X Axis", "Y Axis",
     axesFont,
     "white", "black", dataColor, GraphExternal);

   graphSet(graph3, NBUFFERS, NPTS, data, nValues, GraphBar, 
     "An Animated Zoomed Histogram", 
     titleFont,
     "X Axis", "Y Axis",
     axesFont,
     "white", "black", NULL, GraphExternal);



   /* do the drawing so that expose event can map pixmap onto window */
   graphSetRange(graph1,0,'X',-10.0,10.0);
   graphDraw(graph1);

   /* set the binning for the display of this data as a histogram.. */
   graphSetBins(graph2,0,20);
   graphDraw(graph2);

   graphSetBins(graph3,0,20);
   graphSetRange(graph3,0,'Y', 0.0, 1.5);
   graphDraw(graph3);

   XtAppMainLoop(app);
}

