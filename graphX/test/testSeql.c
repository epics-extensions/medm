/**
 ** toplevel Seql Plotter test program
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
#include <math.h>

#include "GraphX.h"		/* overall GraphX include file 
				   defines data types, plot types,
				   functions and macros            */



#define NBUFFERS 4
#define NPTS 100 
#define TWOPI 6.28

static XtWorkProc animateSeql();

static Boolean animate = FALSE;
static Boolean zoomBoolean = FALSE;
static int numUpdate = 1;
static Boolean booleanSleep = FALSE;
static XtWorkProcId animateId = NULL;
static Seql *seql1;
static Seql *seql2;
static XtAppContext app;

/* 
 * TRANSLATIONS
 */


static void toggleSeqlAnimation(w,event,params,num_params)
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
      animateId = XtAppAddWorkProc(app,(XtWorkProc)animateSeql,NULL);
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

static void seqlPolygon(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
  seqlSetDisplayType(seql1,SeqlPolygon);
  seqlDraw(seql1);
  seqlSetDisplayType(seql2,SeqlPolygon);
  seqlDraw(seql2);
}

static void seqlPoint(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
  seqlSetDisplayType(seql1,SeqlPoint);
  seqlDraw(seql1);
  seqlSetDisplayType(seql2,SeqlPoint);
  seqlDraw(seql2);
}

static void seqlLine(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
  seqlSetDisplayType(seql1,SeqlLine);
  seqlDraw(seql1);

  seqlSetDisplayType(seql2,SeqlLine);
  seqlDraw(seql2);
}



static void seqlPrintPlot(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
  seqlPrint(seql1);
  seqlPrint(seql2);
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
   seqlSetRangeDefault(seql1,0,'X');
   seqlSetRangeDefault(seql1,0,'Y');
}



static void zoom(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{

   if (zoomBoolean) {
      zoomBoolean = FALSE;
      seqlSetRangeDefault(seql2,0,'X');
      seqlSetRangeDefault(seql2,0,'Y');
   } else {
      zoomBoolean = TRUE;
      seqlSetRange(seql2,0,'X',20.,80.);
      seqlSetRange(seql2,0,'Y',-2.,2.);
   }
   seqlDraw(seql2);
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
  seqlTerm(seql1);
  seqlTerm(seql2);

  exit(0);
}



/*
 * CALLBACKS 
 */

static void redisplaySeql(w,seql,call_data)
  Widget w;
  Seql **seql;		/* double indirection since filling in actual */
  XtPointer call_data;	/* address of Seql structure after XtAddCallback */
{

  seqlRefresh(*seql);	/* copy pixmap onto window (this is quick) */	

  seqlDrawLegend(*seql);

}


static void resizeSeql(w,seql,call_data)
  Widget w;
  Seql **seql;		/* double indirection since filling in actual */
  XtPointer call_data;	/* address of Seql structure after XtAddCallback */
{

  seqlResize(*seql);
  seqlDraw(*seql);

  seqlDrawLegend(*seql);

}




static XtWorkProc animateSeql()
{
int i,j;
double temp;

if (booleanSleep) sleep(1);

for (i = 0; i<seql1->nBuffers; i++)
  for (j=0; j<seql1->nValues[i]; j++){
   temp = (  (((double) random())/pow(2.0,31.0) > 0.5) ?
   	 - 0.2*((double) random())/pow(2.0,31.0) :
   	  0.2*((double) random())/pow(2.0,31.0) );

/* this will affect both since same storage used by both */
   seql1->dataBuffer[i][j] += temp;
  }

if (numUpdate == 1) {
   seqlDraw(seql1);
} else {
   seqlDraw(seql1);
   seqlDraw(seql2);
}

/* return FALSE so this WorkProc keeps getting called (just in case...) */
return FALSE;
}


static char defaultTranslations[] = 
		"Ctrl<Key>p: seqlPrintPlot()    \n\
		 <Key>a: toggleSeqlAnimation()    \n\
		 <Key>s: toggleSleep()    \n\
		 <Key>g: seqlPolygon()    \n\
		 <Key>l: seqlLine()    \n\
		 <Key>p: seqlPoint()    \n\
		 <Key>1: update1()    \n\
		 <Key>2: update2()    \n\
		 <Key>z: zoom()    \n\
		 <Key>q: quit() ";

static XtActionsRec actionsTable[] = {
	{"toggleSeqlAnimation", (XtActionProc)toggleSeqlAnimation},
	{"toggleSleep", (XtActionProc)toggleSleep},
	{"seqlLine", (XtActionProc)seqlLine},
	{"seqlPolygon", (XtActionProc)seqlPolygon},
	{"seqlPoint", (XtActionProc)seqlPoint},
	{"seqlPrintPlot", (XtActionProc)seqlPrintPlot},
	{"update1", (XtActionProc)update1},
	{"update2", (XtActionProc)update2},
	{"zoom", (XtActionProc)zoom},
	{"quit", (XtActionProc)quit},
};




main(argc,argv)
  int   argc;
  char *argv[];
{
  Widget topLevel1, topLevel2, canvas1, canvas2;
  Arg args[5];
  int n;
  int nValues[NBUFFERS];

  int i, j, width, height;
  double *data[NBUFFERS];

  Display *display = NULL;
  int screen;
  Window window1, window2;
  XtTranslations transTable;

/* these are filled in by graphXGetBestFont() calls */
  char *titleFont, *axesFont;	
  int   titleFontSize, axesFontSize;

  static char *dataColors[] = {"gray", "sandybrown", "green yellow", 
			"cadet blue",};
  static char *legendArray[] = {"curve 1", "curve 2", "curve 3", "curve 4",};

  static char *legendArray2[] = {"trace #1", "trace #2", "trace #3", 
			"trace #4",};

/*
 * first let's initialize our data buffers 
 */
  for (i = 0; i < NBUFFERS; i++)
     data[i] = (double *) malloc((unsigned) NPTS * sizeof(double));



  printf("\n\t enter g to display as polygon\n\t l = line\n\t p = point\n");

/*
 * Initialize the Intrinsics right away
 * try application contexts approach to shells
 */
   XtToolkitInitialize();
   app = XtCreateApplicationContext();

   display = XtOpenDisplay(app,NULL,argv[0], 
		"application context", NULL,0,&argc,argv);
   if (display == NULL) {
      fprintf(stderr,"\ntestSeql: Can't open display!\n");
      exit(1);
   }

   XtSetArg(args[0],XtNiconName,"Sequential data plot");
   topLevel1 = XtAppCreateShell("Test", "shell1", applicationShellWidgetClass,
		display,args,1);

   XtSetArg(args[0],XtNiconName,"Sequential data plot");
   topLevel2 = XtAppCreateShell("Test", "shell2", applicationShellWidgetClass,
		display,args,1);



   printf("\nEnter width: ");  scanf("%d",&width);
   printf("\nEnter height: "); scanf("%d",&height);

/*
 * Register new actions and compile translations table
 */
   XtAppAddActions(app,actionsTable,XtNumber(actionsTable));
   transTable = XtParseTranslationTable(defaultTranslations);


/*
 * setup a canvas widget to draw in
 */
   n = 0;
   XtSetArg(args[n], XtNwidth, width); n++;
   XtSetArg(args[n], XtNheight, height); n++;

   canvas1 = XtCreateManagedWidget("canvas1", xmDrawingAreaWidgetClass,
					topLevel1, args, n);

   n = 0;
   XtSetArg(args[n], XtNwidth, width); n++;
   XtSetArg(args[n], XtNheight, height); n++;

   canvas2 = XtCreateManagedWidget("canvas2", xmDrawingAreaWidgetClass,
					topLevel2, args, n);

/*
 * merge program-defined translations with existing translations
 */
   XtOverrideTranslations(canvas1,transTable);
   XtOverrideTranslations(canvas2,transTable);
   XtOverrideTranslations(topLevel1,transTable);
   XtOverrideTranslations(topLevel2,transTable);

/*
 * add the expose and resize callbacks, passing pointer to seql as the data of 
 *   the callback
 */
   XtAddCallback(canvas1, XmNexposeCallback,
	(XtCallbackProc)redisplaySeql, &seql1);
   XtAddCallback(canvas1, XmNresizeCallback,
	(XtCallbackProc)resizeSeql, &seql1);
   XtAddCallback(canvas2, XmNexposeCallback,
	(XtCallbackProc)redisplaySeql, &seql2);
   XtAddCallback(canvas2, XmNresizeCallback,
	(XtCallbackProc)resizeSeql, &seql2);


/*
 *  need to realize the widgets for windows to be created...
 */

   XtRealizeWidget(topLevel1);
   XtRealizeWidget(topLevel2);

   screen  = DefaultScreen(display);
   window1  = XtWindow(canvas1);
   window2  = XtWindow(canvas2);


/*
 * now proceed with seql generation
 */

   seql1 = seqlInit(display,screen,window1);
   seql2 = seqlInit(display,screen,window2);

   seqlSetInteractive(seql1,True);
   seqlSetInteractive(seql2,True);

   titleFontSize = GraphX_TitleFontSize(seql1);  
{
char fontFamily[20];
printf("\n enter font family: ");
scanf("%s",fontFamily);
   titleFont = graphXGetBestFont(display,fontFamily,"bold","r", titleFontSize);
   axesFontSize = GraphX_AxesFontSize(seql1);
   axesFont = graphXGetBestFont(display,fontFamily,"medium","r",axesFontSize);
}

   for (i=0; i<NBUFFERS/2; i++){
     nValues[i] = 0;
     for (j=0; j<NPTS; j++) {
        data[i][j] = (double) (-4.0 + 2.0*4.0*j/((NPTS-1)));
	data[i][j] =  cos(data[i][j]) * (i+1);
     }
     nValues[i] = j-1;
   }

   for (i=NBUFFERS/2; i<NBUFFERS; i++){
     nValues[i] = 0;
     for (j=0; j<NPTS/4; j++) {
        data[i][j] = (double) (-4.0 + 2.0*4.0*j/((NPTS-1)));
	data[i][j] =  cos(data[i][j]) * (i+1);
     }
     nValues[i] = j-1;
   }


   /* setup the Seql, use the external-to-seql version of variables */
   /*   -- pass NULL as dataColor ==> do own color allocation */

   seqlSet(seql1, NBUFFERS, NPTS, data, nValues, SeqlPolygon, 
     "A Sequential Data Plot", 
     titleFont,
     "X Axis", "Y Axis",
     axesFont,
     "white", "black", NULL, SeqlExternal);


   /* and do a point version */

   seqlSet(seql2, NBUFFERS, NPTS, data, nValues, SeqlLine, 
     "Another Sequential Data Plot", 
     titleFont,
     "X Axis", "Y Axis",
     axesFont,
     "white", "black", dataColors, SeqlExternal);

   /* do the drawing so that expose event can map pixmap onto window */

   seqlDraw(seql1);
   seqlSetLegend(seql1,"  Legend  ", legendArray);
   seqlDrawLegend(seql1);


   seqlSetRange(seql2,1,'X',10.0,20.0);
   seqlSetRange(seql2,1,'Y',-50.0,50.0);

   seqlDraw(seql2);
   seqlSetLegend(seql2,"   Traces   ", legendArray2);
   seqlDrawLegend(seql2);



   XtAppMainLoop(app);
}



