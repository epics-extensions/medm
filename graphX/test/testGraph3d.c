/**
 ** toplevel Graph3d Plotter test program
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
 


#define WIDTH   250
#define HEIGHT  250


/* some surface related stuff - for comparing surface and graph3d */
#define X_SIZE 30
#define Y_SIZE 30

#define NPTS X_SIZE*Y_SIZE 


static XtWorkProc animateGraph3d();

static Boolean animate = FALSE;
static Boolean booleanSleep = FALSE;
static XtWorkProcId animateId = NULL;
static Graph3d *graph3d;
static XtAppContext app;




/* 
 * TRANSLATIONS
 */


static void printPlot(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
  graph3dPrint(graph3d);
}



static void toggleGraph3dAnimation(w,event,params,num_params)
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
      animateId = XtAppAddWorkProc(app,(XtWorkProc)animateGraph3d,NULL);
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


static void gridType(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
   graph3dSetDisplayType(graph3d,Graph3dGrid);
   graph3dDraw(graph3d);
}


static void pointType(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
   graph3dSetDisplayType(graph3d,Graph3dPoint);
   graph3dDraw(graph3d);
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
  graph3dTerm(graph3d);

  exit(0);
}




/*
 * CALLBACKS 
 */


static void redisplayGraph3d(w,graph3d,call_data)
  Widget w;
  Graph3d **graph3d;  /* double indirection since filling in actual */
  XtPointer call_data;  /* address of Graph3d structure after XtAddCallback */
{

  graph3dRefresh(*graph3d);	/* copy pixmap onto window (this is quick) */	
}


static void resizeGraph3d(w,graph3d,call_data)
  Widget w;
  Graph3d **graph3d;  /* double indirection since filling in actual */
  XtPointer call_data;  /* address of Graph3d structure after XtAddCallback */
{

  graph3dResize(*graph3d);
  graph3dDraw(*graph3d);

}




static XtWorkProc animateGraph3d()
{
int i;
double temp;

if (booleanSleep)sleep(1);

for (i=0; i<graph3d->nPoints; i++){
   temp = (  (((double) random())/pow(2.0,31.0) > 0.5) ?
         - 0.2*((double) random())/pow(2.0,31.0) :
           0.2*((double) random())/pow(2.0,31.0) );

   graph3d->dataPoints[i].x += temp;
   graph3d->dataPoints[i].y += temp;
   graph3d->dataPoints[i].z += temp;

}

   graph3dDraw(graph3d);

/* return FALSE so this WorkProc keeps getting called (just in case...) */
return FALSE;
}


static char defaultTranslations[] = 
		"Ctrl<Key>p: printPlot()    \n\
		 <Key>a: toggleGraph3dAnimation()    \n\
		 <Key>s: toggleSleep()    \n\
		 <Key>g: gridType()    \n\
		 <Key>p: pointType()    \n\
		 <Key>q: quit() ";

static XtActionsRec actionsTable[] = {
	{"toggleGraph3dAnimation", (XtActionProc)toggleGraph3dAnimation},
	{"toggleSleep", (XtActionProc)toggleSleep},
	{"gridType", (XtActionProc)gridType},
	{"pointType", (XtActionProc)pointType},
	{"printPlot", (XtActionProc)printPlot},
	{"quit", (XtActionProc)quit},
};




main(argc,argv)
  int   argc;
  char *argv[];
{
  Widget topLevel2, canvas2;
  Arg args[5];
  int n;

  XtTranslations transTable;

  int i, j, k;
  XYZdataPoint data[NPTS];

  Display *display = NULL;
  int screen;
  Window window2;


/* more surface related stuff */
  double mesh[X_SIZE*Y_SIZE], x[X_SIZE], y[Y_SIZE];
  double r;

/* these are filled in by graphXGetBestFont() calls */
  char *titleFont, *axesFont;
  int   titleFontSize, axesFontSize;


  printf("\n\t enter g to display on grid\n\t enter p to display as points\n");

/*
 * try application contexts approach to shells
 */
   XtToolkitInitialize();
   app = XtCreateApplicationContext();

   display = XtOpenDisplay(app,NULL,argv[0], 
		"application context", NULL,0,&argc,argv);
   if (display == NULL) {
      fprintf(stderr,"\ntestGraph3d: Can't open display!\n");
      exit(1);
   }

   XtSetArg(args[0],XtNiconName,"XYZ plot");
   topLevel2 = XtAppCreateShell("Test", "shell2", applicationShellWidgetClass,
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
   canvas2 = XmCreateDrawingArea(topLevel2,"drawingArea",args,n);
   XtManageChild(canvas2);

/*
 * merge program-defined translations with existing translations
 */
   XtOverrideTranslations(canvas2,transTable);
   XtOverrideTranslations(topLevel2,transTable);

/*
 * add the expose and resize callbacks, passing pointer to graph3d as 
 *   the data of the callback
 */
   XtAddCallback(canvas2, XmNexposeCallback,
	(XtCallbackProc)redisplayGraph3d, &graph3d);
   XtAddCallback(canvas2, XmNresizeCallback,
	(XtCallbackProc)resizeGraph3d, &graph3d);


/*
 *  need to realize the widgets for windows to be created...
 */

   XtRealizeWidget(topLevel2);

   screen  = DefaultScreen(display);
   window2  = XtWindow(canvas2);


/*
 * now proceed with graph3d generation
 */

   graph3d = graph3dInit(display,screen,window2);

/* surface version of data */
   for (i=0; i<X_SIZE; i++) x[i] = i*(1.0/(X_SIZE-1));
   for (j=0; j<Y_SIZE; j++) y[j] = j*(1.0/(Y_SIZE-1));
   for (i=0; i<X_SIZE; i++)
      for(j=0; j<Y_SIZE; j++) {
        r = sqrt( (x[i] - 0.5) * (x[i] - 0.5) + (y[j] - 0.5) * (y[j] - 0.5));
        *(mesh + i + j * X_SIZE) = (r == 0) ? 1.0
                        : x[i]*y[j]*sin(8.0*r) / (8.0 * r);
      }

/* Graph3d version of data */
   k = 0;
   for (i=0; i<X_SIZE; i++)
      for(j=0; j<Y_SIZE; j++) {
        r = sqrt( (x[i] - 0.5) * (x[i] - 0.5) + (y[j] - 0.5) * (y[j] - 0.5));
        *(mesh + i + j * X_SIZE) = (r == 0) ? 1.0
                        : x[i]*y[j]*sin(8.0*r) / (8.0 * r);
	data[k].x = x[i];
	data[k].y = y[j];
	data[k].z = *(mesh + i + j * X_SIZE);
/*** (MDA) for debugging purposes
	printf("\n\tx=%f,\ty=%f,\tz=%f",data[k].x,data[k].y,data[k].z);
 ***/
	k++;
   }


   titleFontSize = GraphX_TitleFontSize(graph3d);
   titleFont = graphXGetBestFont(display,"times","bold","r", titleFontSize);
   axesFontSize = GraphX_AxesFontSize(graph3d);
   axesFont = graphXGetBestFont(display,"times","medium","r",axesFontSize);


   /* do a normal graph3d/XYZ plot with data external to graph3d */
   graph3dSet(graph3d, data, NPTS, Graph3dGrid, "An Animated XYZ Plot", 
     titleFont,
     "X Axis", "Y Axis",
     axesFont,
     "white", "black", "green", Graph3dExternal);

   /* do the drawing so that expose event can map pixmap onto window */
   graph3dSetBins(graph3d,'X',20);
   graph3dSetBins(graph3d,'Y',20);
   graph3dDraw(graph3d);

   XtAppMainLoop(app);
}



