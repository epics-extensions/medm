/**
 ** toplevel Graph Plotter test program
 **	using the graphApp layer of code, for several different plots
 **/


#include <stdio.h>
#include <math.h>

#include "GraphApp.h"		/* this includes all graphX type definitions */

#define NBUFFERS 4
#define NPTS  100
#define TWOPI 6.28
#define X_SIZE 15
#define Y_SIZE 15


/* 
 * functions to call to get next value for strip chart;
 *  note we are not using either of the arguments passed
 */
static double getValue1ForStripChart(channelNumber, userData)
  int channelNumber;
  XtPointer userData;
{
  return ((double)  
	 4.0*(random()/pow(2.0,31.0)) - 4.0*(random()/pow(2.0,31.0)) );
}
static double getValue2ForStripChart(channelNumber, userData)
  int channelNumber;
  XtPointer userData;
{
  return ((double)  
	 2.0*(random()/pow(2.0,31.0)) - 4.0*(random()/pow(2.0,31.0)) );
}

#define NCHANNELS 2
static double (*getChannelValues[])() = { getValue1ForStripChart,
			getValue2ForStripChart,};



main(argc,argv)
  int   argc;
  char *argv[];
{
  GraphApp *graphApp;
  Graph *graph, *hist;
  Surface *surface;
  Strip *strip;
  Seql *seql;
  int i, j, shellNumber;
  double r;

  int seqlValues[NBUFFERS], graphValues[NBUFFERS];

  double *seqlData[NBUFFERS];		/* for Seql plot */
  XYdataPoint *graphData[NBUFFERS];	/* for Graph plots */
  XYZdataPoint graph3dData[NPTS];	/* for Graph3d plot */
  double mesh[X_SIZE*Y_SIZE], 		/* for Surface plot */
	 x[X_SIZE], y[Y_SIZE];
  double stripData[NPTS];		/* for Strip chart */
  StripRange stripRange[NBUFFERS];

  static char *legendArray[] = {"curve 1", "curve 2", "curve 3", "curve 4",};
  static char *dataColor[] = {"red", "green", "blue", "yellow",};

/* these are filled in by graphXGetBestFont() calls */
  char *titleFont, *axesFont;
  int   titleFontSize, axesFontSize;


/*
 * initiate an X connection and create an application context
 */
   graphApp = graphAppInit(NULL,&argc,argv);


/* ----------------------------------------------------------------------- */
   for (i=0; i<NBUFFERS; i++)
     graphData[i] = (XYdataPoint *) malloc((unsigned)NPTS*sizeof(XYdataPoint));

   shellNumber = graphAppInitShell(graphApp,350,350,"Graph shell");
   graph = graphInit(GraphAppInfo(graphApp,shellNumber));

   for (i=0; i<NBUFFERS; i++) {
      graphValues[i] = 0;
      for (j=0; j<NPTS; j++) {
	graphData[i][j].x = (double) (-TWOPI + 2.0*TWOPI*j/((NPTS-1)) +
				0.5 - 0.1*((double) random())/pow(2.0,31.0));
	graphData[i][j].y = cos(tan(graphData[i][j].x)) + 0.5 - 
			0.1*((double) random())/pow(2.0,31.0);
        graphValues[i]++;
      }
   }

   titleFontSize = GraphX_TitleFontSize(graph);
   titleFont = graphXGetBestFont(GraphAppDisplay(graphApp),
		"courier","bold","r", titleFontSize);
   axesFontSize = GraphX_AxesFontSize(graph);
   axesFont = graphXGetBestFont(GraphAppDisplay(graphApp),
		"courier","medium","r",axesFontSize);

   graphSet(graph, NBUFFERS, NPTS, graphData, graphValues, GraphPoint, 
     "An XY Point Plot", 
     titleFont,
     "X Axis", "Y Axis",
     axesFont,
     "white", "black", NULL, GraphInternal);
   graphDraw(graph);
   graphAppRegisterGraphic(graphApp,shellNumber,GRAPH_TYPE,graph);
/* ----------------------------------------------------------------------- */
   for (i=0; i<NBUFFERS; i++)
     seqlData[i] = (double *) malloc((unsigned)NPTS*sizeof(double));

   shellNumber = graphAppInitShell(graphApp,400,400,"Seql shell");
   seql = seqlInit(GraphAppInfo(graphApp,shellNumber));

   for (i=0; i<NBUFFERS; i++) {
      seqlValues[i] = 0;
      for (j=0; j<NPTS; j++) {
        seqlData[i][j] = (double) (-TWOPI + 2.0*TWOPI*j/((NPTS-1)));
        seqlData[i][j] = ((double)(i+1)) * sin(seqlData[i][j]) 
		* cos(seqlData[i][j]);
        seqlValues[i]++;
      }
   }

   titleFontSize = GraphX_TitleFontSize(seql);
   titleFont = graphXGetBestFont(GraphAppDisplay(graphApp),
		"new century schoolbook","bold","r", titleFontSize);
   axesFontSize = GraphX_AxesFontSize(seql);
   axesFont = graphXGetBestFont(GraphAppDisplay(graphApp),
		"times","medium","r",axesFontSize);

   seqlSet(seql, NBUFFERS, NPTS, seqlData, seqlValues, SeqlLine,
     "A Sequential Data Plot",
     titleFont,
     "X Axis", "Y Axis",
     axesFont,
     "white", "black", NULL, SeqlInternal);
   seqlDraw(seql);
   seqlSetLegend(seql," Seql data ", legendArray);
   seqlDrawLegend(seql);
   graphAppRegisterGraphic(graphApp,shellNumber,SEQL_TYPE,seql);
/* ----------------------------------------------------------------------- */
   shellNumber = graphAppInitShell(graphApp,350,350,"Histogram shell");
   hist = graphInit(GraphAppInfo(graphApp,shellNumber));

   for (i=0; i<NBUFFERS; i++) {
      graphValues[i] = 0;
      for (j=0; j<NPTS; j++) {
	graphData[i][j].x = (double) (-TWOPI + 2.0*TWOPI*j)/(NPTS-1);
	graphData[i][j].y = cos(graphData[i][j].x);
        graphValues[i]++;
      }
   }

   titleFontSize = GraphX_TitleFontSize(hist);
   titleFont = graphXGetBestFont(GraphAppDisplay(graphApp),
		"times","bold","r", titleFontSize);
   axesFontSize = GraphX_AxesFontSize(hist);
   axesFont = graphXGetBestFont(GraphAppDisplay(graphApp),
		"times","medium","r",axesFontSize);

   graphSet(hist, NBUFFERS, NPTS, graphData, graphValues, GraphBar, 
     "A Bar Chart",
     titleFont,
     "X Axis", "Y Axis",
     axesFont,
     "white", "black", NULL, GraphInternal);
   graphDraw(hist);
   graphSetLegend(hist," Legend ", legendArray);
   graphDrawLegend(hist);
   graphAppRegisterGraphic(graphApp,shellNumber,GRAPH_TYPE,hist);
/* ----------------------------------------------------------------------- */
   for (i=0; i<X_SIZE; i++) x[i] = 4.0*(i*(1.0/(X_SIZE-1)));
   for (j=0; j<Y_SIZE; j++) y[j] = 4.0*(j*(1.0/(Y_SIZE-1)));
   for (i=0; i<X_SIZE; i++)
      for(j=0; j<Y_SIZE; j++) {
        r = sin(x[i]*y[j]);
        *(mesh + i + j * X_SIZE) =  r;
      }

   shellNumber = graphAppInitShell(graphApp,350,350,"Surface shell");
   surface = surfaceInit(GraphAppInfo(graphApp,shellNumber));

   titleFontSize = GraphX_TitleFontSize(surface);
   titleFont = graphXGetBestFont(GraphAppDisplay(graphApp),
		"times","bold","r", titleFontSize);

   surfaceSet(surface, mesh, x, y, X_SIZE, Y_SIZE, "Surface Plot",
                titleFont, "white", "black", "gray", SurfaceInternal);
   surfaceSetRenderMode(surface, SurfaceShaded);
   surfaceSetView(surface, 45.0, 0.0, 45.0);


   surfaceDraw(surface);
   graphAppRegisterGraphic(graphApp,shellNumber,SURFACE_TYPE,surface);
/* ----------------------------------------------------------------------- */
   shellNumber = graphAppInitShell(graphApp,300,300,"Strip shell");
   strip = stripInit(GraphAppInfo(graphApp,shellNumber));

   stripRange[0].minVal = -10.0; stripRange[0].maxVal = 10.0;
   stripRange[1].minVal = -5.0;  stripRange[1].maxVal = 5.0;

   titleFontSize = GraphX_TitleFontSize(strip);
   titleFont = graphXGetBestFont(GraphAppDisplay(graphApp),
		"times","bold","r", titleFontSize);
   axesFontSize = GraphX_AxesFontSize(strip);
   axesFont = graphXGetBestFont(GraphAppDisplay(graphApp),
		"times","medium","r",axesFontSize);

   stripSet(strip, NCHANNELS, NPTS, stripData,
     stripRange, 0.25, getChannelValues,
     "A StripChart", 
     titleFont,
     "Time Axis (secs)", "Value Axis", 
     axesFont,
     "white", "black", dataColor, StripInternal);

   stripDraw(strip);
   stripSetLegend(strip,"Strip Chart channel info", legendArray);
   stripDrawLegend(strip);
   graphAppRegisterGraphic(graphApp,shellNumber,STRIP_TYPE,strip);
/* ----------------------------------------------------------------------- */



   graphAppLoop(graphApp);

}



