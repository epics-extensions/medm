/***
 ***   GRAPH Plot routines & data structures
 ***
 ***	(MDA) 7 May 1990
 ***
 ***/


#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "Graph.h"


#define BINFRACTION 0.8		/* fraction of bin width for Fill Rectangle */
#define MINBORDER 3		/* minimum distance from window edge (pixels) */

#define RELATIVETICKSIZE 0.02	/* size of tick mark relative to window */
#define NTICKS 2		/* `preferred' number of ticks for axes */
#define MAXTICKS NTICKS+4	/* `maximum' number of ticks based on NTICKS */

#define DATALINEWIDTH 1		/* line width for data lines (pixels) */
#define BORDERLINEWIDTH 1	/* line width for lines (pixels) */

#define LEGENDOFFSET 1          /* amount of legend window to overlap parent */

/*
 * define constants for XListFontsWithInfo call
 */
#define FONTWILDCARD "*"
#define MAXFONTS 1



#ifdef _NO_PROTO
extern int getOKStringPosition();
extern int getOKStringPositionXY();
#else
extern int getOKStringPosition(int, int *, int *, int);
extern int getOKStringPositionXY(int *, int *, int *, int *, int, int);
#endif


/*
 * Initialize the Graph
 * function:  Graph *graphInit()
 *
 *   input parameters:
 *
 *   display	 - pointer to Display structure
 *   screen	 - integer screen number
 *   window	 - Window to output graphics into
 */

Graph *graphInit( display, screen, window)
  Display *display;
  int     screen;
  Window  window;
{
Graph *graph;
XWindowAttributes windowAttributes;
int     i, w, h;

graph = (Graph *) malloc( sizeof( Graph ) );
if (!graph) return(NULL);

XGetWindowAttributes(display, window, &windowAttributes);
w = windowAttributes.width;
h = windowAttributes.height;

/* 
 * initialize some useful fields
 */
graph->display = display;
graph->screen = screen;
graph->window = window;

/* 
 * description of extent of graph in pixel coordinates...
 */
graph->w      = w;
graph->h      = h;
graph->dataX0 =  GRAPH_MARGIN*w;
graph->dataY0 =  GRAPH_MARGIN*h;
graph->dataX1 =  (1.0 - GRAPH_MARGIN)*w;
graph->dataY1 =  (1.0 - GRAPH_MARGIN)*h;


/*
 * create a pixmap
 */
graph->pixmap = XCreatePixmap(graph->display,graph->window,graph->w,graph->h,
			DefaultDepth(graph->display,graph->screen));

/* 
 * create a default, writable GC
 */
graph->gc = XCreateGC(graph->display,
			DefaultRootWindow(graph->display),(unsigned long)NULL,
			NULL);

/*
 * and clear all the other fields (since some compilers do, some don't...)
 */
graph->dataBuffer = NULL;
graph->nBuffers = 0;
graph->bufferSize = 0;
graph->titleFont = NULL;
graph->axesFont = NULL;
graph->title = NULL;
graph->xAxisLabel = NULL;
graph->yAxisLabel = NULL;
graph->devicePoints = NULL;

for (i=0; i< MAXCURVES; i++) {
  graph->xRangeType[i] = GraphDefaultRange;
  graph->xRange[i].minVal = 0.0;
  graph->xRange[i].maxVal = 10.0;
  graph->yRangeType[i] = GraphDefaultRange;
  graph->yRange[i].minVal = 0.0;
  graph->yRange[i].maxVal = 10.0;
  graph->nBins[i] = 100;		/* useful only for GraphBar types */
}

graph->nValues = NULL;
graph->dataPixel = NULL;


/*
 * and initialize the Legend fields
 */
graph->legend = FALSE;
graph->legendTitle = NULL;
graph->legendArray = NULL;

graph->setOwnColors = FALSE;	/* will user fill in own pixel values later? */
graph->setOwnFonts = FALSE;	/* will user fill in own font structs later? */
graph->dontLinearScaleX = FALSE; /* should we make nice scales, or use */
graph->dontLinearScaleY = FALSE; /* exact ones as specified? */

/*
 * and initialize the Interactive fields
 */
graph->interactive = NULL;

return(graph);

}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Set/Reset the data color for the Graph
 * function:  void graphSetDataColor()
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void graphSetDataColor(graph,channelNumber,colorString)
  Graph *graph;
  int channelNumber;
  char *colorString;
{
  XColor color, ignore;

    if (channelNumber > graph->nBuffers-1 || channelNumber < 0) {
       fprintf(stderr,"\ngraphSetDataColor:  invalid Buffer number %d",
                channelNumber);
       return;
    }

    if(!XAllocNamedColor(graph->display,
        DefaultColormap(graph->display,graph->screen),
        colorString,&color,&ignore)) {
        fprintf(stderr, "\ngraphSetDataColor:  couldn't allocate color %s",
                colorString);
        graph->dataPixel[channelNumber]  = WhitePixel(graph->display,
                graph->screen);
    } else {
        graph->dataPixel[channelNumber] = color.pixel;
    }

}





/*
 * Terminate the Graph
 * function:  void graphTerm()
 */

void graphTerm(graph)
  Graph *graph;
{
int i;
/***
 *** free the space used and referenced by the graph
 ***/

/* free the pixmap */
  XFreePixmap(graph->display,graph->pixmap);

/* likewise, free the graphics context and font structures */
  XFreeGC(graph->display,graph->gc);
  if (!graph->setOwnFonts) {
    XFreeFont(graph->display,graph->titleFontStruct);
    XFreeFont(graph->display,graph->axesFontStruct);
  }

/* and finally free the graph space itself */
  free( (char *) graph->devicePoints);

  if (graph->storageMode == GraphInternal) {
      for (i=0; i<graph->nBuffers; i++)
         free( (char *) graph->dataBuffer[i]);
      free( (char *) graph->dataBuffer);
      free( (char *) graph->titleFont);
      free( (char *) graph->axesFont);
      free( (char *) graph->title);
      free( (char *) graph->xAxisLabel);
      free( (char *) graph->yAxisLabel);
  }

  free( (char *) graph->nValues);
  free( (char *) graph->dataPixel);

/* free the legend space */
  if (graph->legend) {
    free( (char *) graph->legendTitle);
    for (i=0; i<graph->nBuffers; i++)
       free( (char *) graph->legendArray[i]);
    free( (char *) graph->legendArray);
    XDestroyWindow(graph->display, graph->legendWindow);
    graph->legend = FALSE;
  }


/* turn off interactive and free the interactive structure if there is one */
  graphSetInteractive(graph,FALSE);

  free( (char *) graph);

}





/*
 * Resize the Graph
 * function: void graphResize()
 */

void graphResize(graph)
  Graph  *graph;
{
XWindowAttributes windowAttributes;

int i, legendWidth = 0, legendHeight= 0;
XWindowAttributes parentWindowAttributes;
XSetWindowAttributes setWindowAttributes;
int rootX, rootY;
Window child;

/*
 * check to see if window has changed size
 *   if so, then Free the pixmap, fetch a new one and continue
 */
  XGetWindowAttributes(graph->display, graph->window, &windowAttributes);
  if (graph->w != windowAttributes.width  ||
      graph->h != windowAttributes.height){

      graph->w = windowAttributes.width;
      graph->h = windowAttributes.height;
      graph->dataX0 = GRAPH_MARGIN*windowAttributes.width;
      graph->dataY0 = GRAPH_MARGIN*windowAttributes.height;
      graph->dataX1 = (1.0 - GRAPH_MARGIN)*windowAttributes.width;
      graph->dataY1 = (1.0 - GRAPH_MARGIN)*windowAttributes.height;

      XFreePixmap(graph->display,graph->pixmap);
      graph->pixmap = XCreatePixmap(graph->display,graph->window,
                graph->w,graph->h,
                DefaultDepth(graph->display,graph->screen));

      /* Relocate legend window, if it exists */
      if (graph->legend) {
	 XDestroyWindow(graph->display,graph->legendWindow);

      /*
       * loop to determine required size of legend window
       */
	 legendWidth = XTextWidth(graph->titleFontStruct,graph->legendTitle,
			strlen(graph->legendTitle)) + 2*MINBORDER;
	 for (i=0; i<graph->nBuffers; i++) {
	      legendWidth = max(legendWidth,
			XTextWidth(graph->axesFontStruct,graph->legendArray[i],
			strlen(graph->legendArray[i]))+graph->axesFontHeight+
			4*MINBORDER);
	 }
	 legendHeight = 2*MINBORDER + graph->nBuffers*MINBORDER + MINBORDER +
			graph->titleFontHeight +
			graph->nBuffers*graph->axesFontHeight;
      /*
       * create a window butting up against graph window;
       * legendWindow is created as child of graph wndow.
       */
	 graph->legendWindow = XCreateSimpleWindow(graph->display,
				graph->window,
				graph->w-legendWidth
					-LEGENDOFFSET-(2*BORDERLINEWIDTH),
				LEGENDOFFSET, legendWidth, legendHeight,
				BORDERLINEWIDTH,
				graph->forePixel, graph->backPixel);
      /*
       * Set backing store attribute from parent window
       */
	 XGetWindowAttributes(graph->display, graph->window,
				&parentWindowAttributes);
	 setWindowAttributes.backing_store = 
				parentWindowAttributes.backing_store;
	 XChangeWindowAttributes(graph->display, graph->legendWindow,
				CWBackingStore, &setWindowAttributes);
	 XMapRaised(graph->display,graph->legendWindow);
      }
  }
}






/*
 * Draw the Graph
 * function: void graphDraw()
 */

void graphDraw(graph)
  Graph  *graph;
{

int i, j, textWidth;
double xScale[MAXCURVES], yScale[MAXCURVES];
int xScaledValue, yScaledValue;
int prevX, prevY, bufferNumber;
double binWidth[MAXCURVES], binMin;

double xInterval[MAXCURVES], yInterval[MAXCURVES];
double xIntervalBins[MAXCURVES];

double xValueScale, yValueScale;
int tickPosition, xPosition, yPosition, tempPosition;
int goodNumberOfTicks, nDisplayPoints;
char *valueString = "            ";	/* a 12 char blank string */

int usedSpots[(NTICKS+1)*MAXCURVES], usedCtr;
int stringPosition, stringPositionX, stringPositionY;
int used[MAXTICKS], numUsed[MAXTICKS];

double *binnedData;
int binNum, bufNum;

XRectangle clipRect[2];



XSetForeground(graph->display,graph->gc,graph->backPixel);
XFillRectangle( graph->display,graph->pixmap,graph->gc,0,0,graph->w,graph->h);


/* 
 * calculate pixel to value scaling FOR EACH CURVE
 *
 *   examine range specifiers to see what limits the user wants
 */

for (i = 0; i < graph->nBuffers; i++) {

  if (graph->xRangeType[i] == GraphDefaultRange) {     /* go with default */
     graph->xRange[i].maxVal = (double)(-HUGE_VAL);
     graph->xRange[i].minVal = (double)(HUGE_VAL);
     for (j = 0; j < graph->nValues[i]; j++) {
       graph->xRange[i].maxVal = max(graph->xRange[i].maxVal,
		(graph->dataBuffer)[i][j].x);
       graph->xRange[i].minVal = min(graph->xRange[i].minVal,
		(graph->dataBuffer)[i][j].x);
     }
  }

  if (graph->yRangeType[i] == GraphDefaultRange) {     /* go with default */
     graph->yRange[i].maxVal = (double)(-HUGE_VAL);
     graph->yRange[i].minVal = (double)(HUGE_VAL);
     for (j = 0; j < graph->nValues[i]; j++) {
       graph->yRange[i].maxVal = max(graph->yRange[i].maxVal,
		(graph->dataBuffer)[i][j].y);
       graph->yRange[i].minVal = min(graph->yRange[i].minVal,
		(graph->dataBuffer)[i][j].y);
     }
   }


/* 
 * perform linear scaling (get nice values) for X and Y axes
 */
  if ( graph->dontLinearScaleX )
     xInterval[i] = graph->xRange[i].maxVal - graph->xRange[i].minVal;
  else
     linear_scale(graph->xRange[i].minVal,graph->xRange[i].maxVal,NTICKS,
	&graph->xRange[i].minVal,&graph->xRange[i].maxVal,&xInterval[i]);

  if ( graph->dontLinearScaleY )
     yInterval[i] = graph->yRange[i].maxVal - graph->yRange[i].minVal;
  else
     linear_scale(graph->yRange[i].minVal,graph->yRange[i].maxVal,NTICKS,
	&graph->yRange[i].minVal,&graph->yRange[i].maxVal,&yInterval[i]);


  if (graph->type == GraphBar)
    xIntervalBins[i] = (graph->xRange[i].maxVal - graph->xRange[i].minVal) 
		/ (double) graph->nBins[i];


/*
 * now put up the data based on type
 */

  xScale[i] = (graph->dataX1 - graph->dataX0)  / (graph->xRange[i].maxVal - 
		graph->xRange[i].minVal);
  yScale[i] = (graph->dataY1 - graph->dataY0) /(graph->yRange[i].maxVal - 
		graph->yRange[i].minVal);


/* 
 * for all types other than GraphBar:  
 *   first loop to determine number of displayable points;
 *   if either X or Y coordinate is valid, then rely on clipping
 *   to do the right thing (rather than throwing the whole point away)
 */
  if (graph->type == GraphBar) {
    /* calculate binWidth for GraphBar types */
     binWidth[i] = (graph->dataX1 - graph->dataX0)/((double) graph->nBins[i]);
  }

}

/* 
 * Draw axes first
 */
XSetForeground( graph->display, graph->gc, graph->forePixel);

/* left side axis */
XDrawLine(graph->display,graph->pixmap,graph->gc,(int)(graph->dataX0),
		(int)(graph->dataY0), (int)(graph->dataX0),
		(int)(graph->dataY1 - graph->dataY0)+(int)(graph->dataY0));
/* right side axis */
XDrawLine(graph->display,graph->pixmap,graph->gc, (int)(graph->dataX1), 
		(int)(graph->dataY0), (int)(graph->dataX1),
		(int)(graph->dataY1 - graph->dataY0)+(int)(graph->dataY0));
/* top line */
XDrawLine(graph->display,graph->pixmap,graph->gc,
		(int)(graph->dataX0), (int)(graph->dataY0),
		(int)(graph->dataX1), (int)(graph->dataY0));
/* bottom line */
XDrawLine(graph->display,graph->pixmap,graph->gc,
		(int)(graph->dataX0), (int)(graph->dataY1),
		(int)(graph->dataX1), (int)(graph->dataY1) );


/*
 * set clipping rectangle
 */
clipRect[0].x = graph->dataX0;
clipRect[0].y = graph->dataY0;
clipRect[0].width = graph->dataX1 - graph->dataX0;
clipRect[0].height = graph->dataY1 - graph->dataY0;
XSetClipRectangles(graph->display, graph->gc, 0, 0, clipRect, 1, YXBanded);



switch(graph->type) {

case GraphBar: 
/* 
 * loop over points, put in proper bins, then put up rectangles
 *
 *    not using polyline/polypoints/polyrectangles until I can map
 *    this cleanly onto the graph->devicePoints XPoint array
 */


  for (i = 0; i < graph->nBuffers; i++) {

    binnedData = (double *) (malloc(graph->nBins[i] * sizeof(double)));

    XSetForeground( graph->display, graph->gc, graph->dataPixel[i]);

    for (j = 0; j < graph->nBins[i]; j++)
	binnedData[j] = 0.0;

    binMin = HUGE_VAL;
    if (graph->yRangeType[i] == GraphDefaultRange) {     /* go with default */
     for (j = 0; j < graph->nValues[i]; j++) {
       if (graph->dataBuffer[i][j].x <= graph->xRange[i].maxVal &&
         graph->dataBuffer[i][j].x >= graph->xRange[i].minVal) {
	   binMin = min(binMin,graph->dataBuffer[i][j].y);
       }
     }
    } else {
      binMin = graph->yRange[i].minVal;
    }

    for (j = 0; j < graph->nValues[i]; j++) {
     if (graph->dataBuffer[i][j].x <= graph->xRange[i].maxVal &&
         graph->dataBuffer[i][j].x >= graph->xRange[i].minVal) {
	   binNum = (int)((graph->dataBuffer[i][j].x-graph->xRange[i].minVal)/
		xIntervalBins[i]);
	   binnedData[binNum] = binMin;
     }
    }

    for (j = 0; j < graph->nValues[i]; j++) {
     if (graph->dataBuffer[i][j].x <= graph->xRange[i].maxVal &&
         graph->dataBuffer[i][j].x >= graph->xRange[i].minVal) {
	   binNum = (int)((graph->dataBuffer[i][j].x-graph->xRange[i].minVal)/
		xIntervalBins[i]);
	   binnedData[binNum] = max(graph->dataBuffer[i][j].y,
			binnedData[binNum]);
     }
    }



    for (j = 0; j < graph->nBins[i]; j++) {

     /* yScaledValue is sort of inverse since upper left is (0,0), */
     /*   this value is actually the number of pixels NOT in bar/rectangle */
      yScaledValue = (int) ((graph->yRange[i].maxVal - binnedData[j])*yScale[i]);

      if ((int)(binWidth[i]*BINFRACTION) < 1) {
         XDrawLine( graph->display,graph->pixmap , graph->gc,
			(int) (graph->dataX0 + j*binWidth[i]),
			(int)(graph->dataY0 + yScaledValue),
			(int) (graph->dataX0 + j*binWidth[i]),
			graph->dataY1 );

      } else {
         XFillRectangle( graph->display,graph->pixmap , graph->gc,
			(int) ((graph->dataX0) + j*binWidth[i]),
			((int)(graph->dataY0))+yScaledValue,
			((int)(binWidth[i]*BINFRACTION) < 1
				? 1 : (int)(binWidth[i]*BINFRACTION)),
			(int)(graph->dataY1 - graph->dataY0) - yScaledValue);
      }

    }

  free(binnedData);

  }

  break;



case GraphLine:
/* 
 * loop over points and put up polyline 
 */

  XSetLineAttributes( graph->display, graph->gc,
			DATALINEWIDTH,LineSolid,CapButt,JoinMiter);

  for (i = 0; i < graph->nBuffers; i++) {

    nDisplayPoints = 0;
    XSetForeground( graph->display, graph->gc, graph->dataPixel[i]);

    for (j = 0; j < graph->nValues[i]; j++) {

     if ((graph->dataBuffer[i][j].x <= graph->xRange[i].maxVal &&
         graph->dataBuffer[i][j].x >= graph->xRange[i].minVal) ||
         (graph->dataBuffer[i][j].y <= graph->yRange[i].maxVal &&
         graph->dataBuffer[i][j].y >= graph->yRange[i].minVal)) {

      /* xScaledValue is more sensible than yScaledValue, it is # of pixels */
        xScaledValue = (int) (((graph->dataBuffer)[i][j].x - 
		graph->xRange[i].minVal) *xScale[i]);

      /* yScaledValue is sort of inverse since upper left is (0,0), */
      /*   this value is actually the number of pixels NOT in bar/rectangle */
        yScaledValue = (int) ((graph->yRange[i].maxVal - 
		(graph->dataBuffer)[i][j].y)*yScale[i]);


        graph->devicePoints[nDisplayPoints].x = 
			(int) ((graph->dataX0) + xScaledValue);
        graph->devicePoints[nDisplayPoints].y = 
			(int) ((graph->dataY0) + yScaledValue);
	nDisplayPoints++;
     }

    }
    XDrawLines( graph->display,graph->pixmap , graph->gc,
			graph->devicePoints, nDisplayPoints, CoordModeOrigin);
  }

  XSetLineAttributes( graph->display, graph->gc,
			BORDERLINEWIDTH,LineSolid,CapButt,JoinMiter);
  break;



case GraphPoint:
/* 
 * loop over points and put up as polypoints 
 */


  for (i = 0; i < graph->nBuffers; i++) {

    nDisplayPoints = 0;

    XSetForeground( graph->display, graph->gc, graph->dataPixel[i]);

    for (j = 0; j < graph->nValues[i]; j++) {
     if ((graph->dataBuffer[i][j].x <= graph->xRange[i].maxVal &&
         graph->dataBuffer[i][j].x >= graph->xRange[i].minVal) ||
         (graph->dataBuffer[i][j].y <= graph->yRange[i].maxVal &&
         graph->dataBuffer[i][j].y >= graph->yRange[i].minVal)) {

      /* xScaledValue is more sensible than yScaledValue, it is # of pixels */
	xScaledValue = (int) (((graph->dataBuffer)[i][j].x 
		- graph->xRange[i].minVal)*xScale[i]);

      /* yScaledValue is sort of inverse since upper left is (0,0), */
      /*   this value is actually the number of pixels NOT in bar/rectangle */
	yScaledValue = (int) ((graph->yRange[i].maxVal - 
		(graph->dataBuffer)[i][j].y)*yScale[i]);

	graph->devicePoints[nDisplayPoints].x = 
			(int) ((graph->dataX0) + xScaledValue);
	graph->devicePoints[nDisplayPoints].y = 
			(int) ((graph->dataY0) + yScaledValue);

	nDisplayPoints++;
     }
    }
    XDrawPoints( graph->display,graph->pixmap , graph->gc,
			graph->devicePoints, nDisplayPoints, CoordModeOrigin);

  }

  break;


}


/* 
 *and now restore the gc to no-clipping
 */
XSetClipMask(graph->display, graph->gc, None);



/* 
 * Put up Value axis tick marks and labelling
 */

XSetForeground( graph->display, graph->gc, graph->forePixel);

usedCtr = 0;
for (i=0; i<MAXTICKS; i++) {
   used[i] = 0;
   numUsed[i]=0;
}

for (bufNum = 0; bufNum < graph->nBuffers; bufNum++) {

 for (yValueScale=graph->yRange[bufNum].minVal; 
		yValueScale<=graph->yRange[bufNum].maxVal; 
		yValueScale+=yInterval[bufNum]){

   XSetForeground( graph->display, graph->gc, graph->forePixel);
   tickPosition = (int)((graph->yRange[bufNum].maxVal-yValueScale)*
		yScale[bufNum])+ (int)(graph->dataY0);

  /* left side ticks (could do grids right around here...) */
   XDrawLine(graph->display,graph->pixmap,graph->gc,
		(int)(graph->dataX0) 
			- (int)(RELATIVETICKSIZE*(min(graph->w,graph->h))/2),
		tickPosition,
		(int)(graph->dataX0) 
			+ (int)(RELATIVETICKSIZE*(min(graph->w,graph->h))/2),
		tickPosition);

  /* right side ticks */
   XDrawLine(graph->display,graph->pixmap,graph->gc,
		(int)(graph->dataX1) 
		    - (int)(RELATIVETICKSIZE*(min(graph->w,graph->h))/2),
		tickPosition,
		(int)(graph->dataX1) 
		    + (int)(RELATIVETICKSIZE*(min(graph->w,graph->h))/2),
		tickPosition);

  /* 
   * tick labelling (left side only)
   */
   sprintf(valueString,"%.3g",yValueScale);
   textWidth = XTextWidth(graph->axesFontStruct,valueString,
			strlen(valueString));
   XSetFont(graph->display,graph->gc,graph->axesFontStruct->fid);

   stringPosition = getOKStringPosition(tickPosition, usedSpots,
                        &usedCtr, graph->axesFontHeight);
 
   XDrawString(graph->display,graph->pixmap,graph->gc,
	(int)(graph->dataX0)-textWidth
		-(int)(RELATIVETICKSIZE*(min(graph->w,graph->h))),
	stringPosition,
	valueString,strlen(valueString));

/* put up a little colored circles next to the scale */
   XSetForeground( graph->display, graph->gc, graph->dataPixel[bufNum]);
   XFillArc(graph->display,graph->pixmap,graph->gc,
        (int)(graph->dataX0) - textWidth
                - (int)(RELATIVETICKSIZE*(min(graph->w,graph->h)))
                - MINBORDER - graph->axesFontHeight/3,
        stringPosition - graph->axesFontHeight/3,
       graph->axesFontHeight/3, graph->axesFontHeight/3, 0, 64*360);
 }
}



/* 
 * Put up independent variable tick marks and labels
 */

XSetForeground( graph->display, graph->gc, graph->forePixel);

/*
 * now X axis labelling...
 */

usedCtr = 0;
for (i=0; i<MAXTICKS; i++) {
   used[i] = 0;
   numUsed[i]=0;
}


for (bufNum = 0; bufNum < graph->nBuffers; bufNum++) {

 for (xValueScale=graph->xRange[bufNum].minVal; 
		xValueScale<=graph->xRange[bufNum].maxVal; 
		xValueScale+=xInterval[bufNum]){

      XSetForeground( graph->display, graph->gc, graph->forePixel);
      tickPosition = (int)((xValueScale - graph->xRange[bufNum].minVal)*
		xScale[bufNum]) + (int)(graph->dataX0);

     /* bottom ticks (could do grids right around here...) */
      XDrawLine(graph->display,graph->pixmap,graph->gc,
		tickPosition,
		(int)(graph->dataY1)
			- (int)(RELATIVETICKSIZE*(min(graph->h,graph->w))/2),
		tickPosition,
		(int)(graph->dataY1)
			+ (int)(RELATIVETICKSIZE*(min(graph->h,graph->w))/2));

     /* top ticks */
      XDrawLine(graph->display,graph->pixmap,graph->gc,
		tickPosition,
		(int)(graph->dataY0) 
		    - (int)(RELATIVETICKSIZE*(min(graph->h,graph->w))/2),
		tickPosition,
		(int)(graph->dataY0) 
		    + (int)(RELATIVETICKSIZE*(min(graph->h,graph->w))/2));

    /* 
     * tick labelling (bottom only)
     */
      sprintf(valueString,"%.3g",xValueScale);
      textWidth = XTextWidth(graph->axesFontStruct,valueString,
			strlen(valueString));
      XSetFont(graph->display,graph->gc,graph->axesFontStruct->fid);

      stringPositionX = tickPosition;
      stringPositionY = (int)(graph->dataY1
           + (int)(RELATIVETICKSIZE*(min(graph->h,graph->w)))/2
           + graph->axesFontHeight);
      getOKStringPositionXY(&stringPositionX, &stringPositionY, used, numUsed,
                MAXTICKS, graph->axesFontHeight);

      XDrawString(graph->display,graph->pixmap,graph->gc,
	stringPositionX - textWidth/2 + MINBORDER + graph->axesFontHeight/2,
	stringPositionY,
	valueString,strlen(valueString));

/* put up a little colored rectangle next to the scale */
      XSetForeground( graph->display, graph->gc, graph->dataPixel[bufNum]);
      XFillArc(graph->display,graph->pixmap,graph->gc,
        stringPositionX - textWidth/2,
        stringPositionY - graph->axesFontHeight/3,
        graph->axesFontHeight/3, graph->axesFontHeight/3, 0, 64*360);
 
  }
}
     
     

/*
 * put down title
 */
XSetForeground( graph->display, graph->gc, graph->forePixel);
if (graph->title != NULL) {
   textWidth = XTextWidth(graph->titleFontStruct,graph->title,
			strlen(graph->title));
   XSetFont(graph->display,graph->gc,graph->titleFontStruct->fid);
   XDrawString(graph->display,graph->pixmap,graph->gc,
	(graph->w/2 - textWidth/2),
	(graph->dataY0/2), graph->title,strlen(graph->title));
}


/*
 * label axes
 */
XSetFont(graph->display,graph->gc,graph->axesFontStruct->fid);

/*
 * put down x axis label
 */
if (graph->xAxisLabel != NULL) {
  textWidth = XTextWidth(graph->axesFontStruct,graph->xAxisLabel,
			strlen(graph->xAxisLabel));
  tempPosition = (int)(graph->dataX1);
  xPosition = tempPosition + (graph->w - tempPosition)/2 - textWidth/2;
  xPosition = (xPosition+textWidth > graph->w - MINBORDER 
		? graph->w - MINBORDER - textWidth : xPosition);
  tempPosition = (int)(graph->dataY1);
  yPosition = tempPosition + 3*(graph->h - tempPosition)/5 + 
		graph->axesFontHeight/2;
  XDrawString(graph->display,graph->pixmap,graph->gc,
	xPosition,
	yPosition,
	graph->xAxisLabel,strlen(graph->xAxisLabel));
}

/*
 * put down y axis label
 */
if (graph->yAxisLabel != NULL) {
  textWidth = XTextWidth(graph->axesFontStruct,graph->yAxisLabel,
			strlen(graph->yAxisLabel));
  tempPosition = (int)(graph->dataX0);
  xPosition = tempPosition/2 - textWidth/2;
  xPosition = (xPosition < MINBORDER ? MINBORDER : xPosition);
  tempPosition = (int)(graph->dataY0);
  yPosition = 3*tempPosition/7  + graph->axesFontHeight/2;
  XDrawString(graph->display,graph->pixmap,graph->gc,
	xPosition,
	yPosition,
	graph->yAxisLabel,strlen(graph->yAxisLabel));
}


XCopyArea(graph->display,graph->pixmap,graph->window,graph->gc,0,0,
		graph->w,graph->h,0,0);

XFlush(graph->display);


}





/*
 * Refresh the display
 * (don't recalcuate, just remap the pixmap onto the window
 */

void graphRefresh(graph)
  Graph  *graph;
{
  XCopyArea(graph->display,graph->pixmap,graph->window,graph->gc,0,0,
		graph->w,graph->h,0,0);
  XFlush(graph->display);
}




/*
 *   Add a graph plot
 *
 *   input parameters:
 *
 *   graph	 - pointer to Graph data structure
 *   nBuffers    - number of "plots" to be made in same frame
 *   bufferSize  - size of buffers
 *   dataBuffer  - array of array of XYdataPoint (double x,y pairs)
 *   nValues     - number of points per buffer (above)
 *   type	 - graph type (GraphType): 
 *		   	GraphBar, GraphPoint, GraphLine
 *   title	 - character string (ptr) for title
 *   titleFont	 - charaacter string (ptr) for font to use for title
 *   xAxisLabel	 - character string (ptr) for X axis label
 *   yAxisLabel	 - character string (ptr) for Y axis label
 *   axesFont	 - character string (ptr) for font to use for axes labels
 *   foreColor	 - character string (ptr) for color to use for foreground
 *   backColor	 - character string (ptr) for color to use for background
 *   dataColor	 - array of character strings (ptr) for color to use for data
 *			if NULL, then do own color allocation via color cells
 *   storageMode -  graph data storage mode: GraphInternal, GraphExternal
 *			if GraphInternal, make a copy of the data; 
 *			otherwise (GraphExternal), just copy the pointers
 */

void graphSet( graph, nBuffers, bufferSize, dataBuffer, nValues, type, 
			title, titleFont, 
			xAxisLabel, yAxisLabel,
			axesFont, foreColor, backColor, dataColor, storageMode)
  Graph		*graph;
  int		 nBuffers;
  int		 bufferSize;
  XYdataPoint	*dataBuffer[];
  int           *nValues;
  GraphType	 type;
  char		*title, *titleFont, *xAxisLabel, *yAxisLabel, *axesFont;
  char		*foreColor, *backColor, *dataColor[];
  GraphStorageMode storageMode;
{
int i, j, k,  stringLength = 0;
XColor color, ignore;
double r, g, b;
int fontCount;
char **fontNames;
XFontStruct *fontInfo;


graph->nBuffers = nBuffers;
graph->bufferSize = bufferSize;
graph->type = type;

if (storageMode == GraphExternal) {
    graph->storageMode = GraphExternal;
    graph->dataBuffer    = dataBuffer;
    graph->titleFont = titleFont;
    graph->axesFont = axesFont;
    graph->title = title;
    graph->xAxisLabel = xAxisLabel;
    graph->yAxisLabel = yAxisLabel;
    graph->nValues = nValues;
} else {
    graph->storageMode = GraphInternal;
    if (graph->dataBuffer) {
        for (i = 0; i < nBuffers; i++)
            free( (char *) graph->dataBuffer[i] ); 
	free( (char *) graph->dataBuffer );
    }
    graph->dataBuffer = (XYdataPoint **) malloc((unsigned)
                        (nBuffers*sizeof(XYdataPoint *)));
    for (i = 0; i < nBuffers; i++) {
        graph->dataBuffer[i] = (XYdataPoint *)
             malloc((unsigned) bufferSize*sizeof(XYdataPoint));
        memcpy( (void *)graph->dataBuffer[i], (void *)dataBuffer[i],
		(size_t)(bufferSize*sizeof(XYdataPoint)) );
    }

    if (graph->titleFont) free( (char *) graph->titleFont);
    if (titleFont != NULL) {
      stringLength = strlen(titleFont) + 1;
      graph->titleFont = (char *) malloc(stringLength);
      memcpy( (void *)graph->titleFont,(void *)titleFont,(size_t)stringLength);
      graph->titleFont[stringLength-1] = '\0';
    }

    if (graph->axesFont) free( (char *) graph->axesFont);
    if (axesFont != NULL) {
      stringLength = strlen(axesFont) + 1;
      graph->axesFont = (char *) malloc(stringLength);
      memcpy( (void *)graph->axesFont, (void *)axesFont, (size_t)stringLength);
      graph->axesFont[stringLength-1] = '\0';
    }

    if (graph->title) free( (char *) graph->title);
    if (title != NULL) {
      stringLength = strlen(title) + 1;
      graph->title = (char *) malloc(stringLength);
      memcpy( (void *)graph->title, (void *)title, (size_t)stringLength);
      graph->title[stringLength-1] = '\0';
    }

    if (graph->xAxisLabel) free( (char *) graph->xAxisLabel);
    if (xAxisLabel != NULL) {
      stringLength = strlen(xAxisLabel) + 1;
      graph->xAxisLabel = (char *) malloc(stringLength);
      memcpy((void *)graph->xAxisLabel,(void *)xAxisLabel,(size_t)stringLength);
      graph->xAxisLabel[stringLength-1] = '\0';
    }

    if (graph->yAxisLabel) free( (char *) graph->yAxisLabel);
    if (yAxisLabel != NULL) {
      stringLength = strlen(yAxisLabel) + 1;
      graph->yAxisLabel = (char *) malloc(stringLength);
      memcpy((void *)graph->yAxisLabel,(void *)yAxisLabel,(size_t)stringLength);
      graph->yAxisLabel[stringLength-1] = '\0';
    }

    if (graph->nValues) free( (char *) graph->nValues);
    graph->nValues = (int *) malloc(nBuffers*sizeof(int *));
    for (i = 0; i < nBuffers; i++) graph->nValues[i] = nValues[i];

}

graph->type	 = type;


if (graph->dataPixel) free( (char *) graph->dataPixel);
graph->dataPixel = (unsigned long *) malloc((unsigned)
                        ((nBuffers)*sizeof(unsigned long)));

if (graph->devicePoints) free( (char *) graph->devicePoints );
graph->devicePoints = (XPoint *) malloc((unsigned)
			(bufferSize*sizeof(XPoint)));


/* set foreground, background and data colors based on passed-in strings */

/* use a color for the foreground if possible */
if ((DisplayCells(graph->display,graph->screen) > 2) && !(graph->setOwnColors))
 {
    if(!XAllocNamedColor(graph->display,
        DefaultColormap(graph->display,graph->screen),
        foreColor,&color,&ignore)){
        fprintf(stderr,"\ngraphSet:  couldn't allocate color %s",foreColor);
        graph->forePixel  = WhitePixel(graph->display,graph->screen);
    } else {
        graph->forePixel = color.pixel;
    }

    if(!XAllocNamedColor(graph->display,
        DefaultColormap(graph->display,graph->screen),
        backColor,&color,&ignore)) {
        fprintf(stderr,"\ngraphSet:  couldn't allocate color %s",backColor);
        graph->backPixel  = BlackPixel(graph->display,graph->screen);
    } else {
        graph->backPixel = color.pixel;
    }


  /* if dataColor pointer is NULL, then do own color allocation */
    if (dataColor == NULL) {
      if (XAllocColorCells(graph->display,
                DefaultColormap(graph->display,graph->screen), FALSE,
                NULL, 0, graph->dataPixel,nBuffers)) {

        for (k = 0; k < nBuffers; k++) {
/* (MDA) - rainbow( (double)(k+1)/(double)nBuffers, 1.0, 1.0, &r, &g, &b); */
            r = ((double) rand())/pow(2.0,31.0);
            g = ((double) rand())/pow(2.0,31.0);
            b = ((double) rand())/pow(2.0,31.0);
            r *= 65535; g *= 65535; b *= 65535;
            color.red = (unsigned int) r;
            color.green = (unsigned int) g;
            color.blue = (unsigned int) b;
            color.pixel = graph->dataPixel[k];
            color.flags = DoRed | DoGreen | DoBlue;
            XStoreColor(graph->display,
                DefaultColormap(graph->display,graph->screen), &color);
        }

      } else {
        fprintf(stderr,"\ngraphSet:  couldn't allocate color cells");
      }
    } else {
      for (i = 0; i < nBuffers; i++) {
       if(!XAllocNamedColor(graph->display,
        DefaultColormap(graph->display,graph->screen),
        dataColor[i],&color,&ignore)) {
        fprintf(stderr,"\ngraphSet:  couldn't allocate color %s",dataColor);
        graph->dataPixel[i]  = WhitePixel(graph->display,graph->screen);
       } else {
        graph->dataPixel[i] = color.pixel;
       }
      }
    }

} else {
    graph->forePixel  = WhitePixel(graph->display,graph->screen);
    graph->backPixel  = BlackPixel(graph->display,graph->screen);
    for (i = 0; i < nBuffers; i++)
       graph->dataPixel[i]  = WhitePixel(graph->display,graph->screen);
}


/*
 * load the fonts
 */
if (!graph->setOwnFonts) {
   if ((graph->titleFontStruct=XLoadQueryFont(graph->display,graph->titleFont)) 
						== NULL) {
      fprintf(stderr,"display %s doesn't know font %s\n",
                DisplayString(graph->display),graph->titleFont);
      fprintf(stderr,"  using first available font\n");
      fontNames = XListFontsWithInfo(graph->display,FONTWILDCARD,
		MAXFONTS,&fontCount, &fontInfo);
      graph->titleFontStruct = XLoadQueryFont(graph->display,fontNames[0]);
      XFreeFontInfo(fontNames,fontInfo,fontCount);
      XFreeFontNames(fontNames);
   }
   graph->titleFontHeight = graph->titleFontStruct->ascent + 
				graph->titleFontStruct->descent;
   graph->titleFontWidth = graph->titleFontStruct->max_bounds.rbearing - 
				graph->titleFontStruct->min_bounds.lbearing;

   if ((graph->axesFontStruct=XLoadQueryFont(graph->display,graph->axesFont)) 
						== NULL) {
      fprintf(stderr,"display %s doesn't know font %s\n",
                DisplayString(graph->display),graph->axesFont);
      fprintf(stderr,"  using first available font\n");
      graph->axesFontStruct = graph->titleFontStruct;
   }

   graph->axesFontHeight = graph->axesFontStruct->ascent + 
				graph->axesFontStruct->descent;
   graph->axesFontWidth = graph->axesFontStruct->max_bounds.rbearing - 
				graph->axesFontStruct->min_bounds.lbearing;

}

}





/*
 *   Set the  axis display range of a Graph plot
 *
 *   input parameters:
 *
 *   graph        - pointer to Graph data structure
 *   axis        - character specifying X or Y axis
 *   minVal      - minimum display value
 *   maxVal      - maximum display value
 */

#ifdef _NO_PROTO
void graphSetRange(graph,curveNum,axis,minVal,maxVal)
  Graph *graph;
  int curveNum;
  char axis;
  double minVal,maxVal;
#else 
void graphSetRange(Graph *graph, int curveNum, char axis,
	 double minVal, double maxVal)
#endif
{
  if (curveNum < 0 || curveNum >= graph->nBuffers) {
        fprintf(stderr,
         "\ngraphSetRange: illegal curve (number %d) specified\n",
		curveNum);
	return;
  }

  if (axis == 'X' || axis == 'x') {
     if (minVal >= maxVal) {
        graph->xRangeType[curveNum] = GraphDefaultRange;
        fprintf(stderr,
         "\ngraphSetRange: improper range specified, using defaults\n");
        return;
     }
     graph->xRangeType[curveNum] = GraphSpecRange;
     graph->xRange[curveNum].minVal = minVal;
     graph->xRange[curveNum].maxVal = maxVal;
  } else {
    if (axis == 'Y' || axis == 'y') {
     if (minVal >=  maxVal) {
        graph->yRangeType[curveNum] = GraphDefaultRange;
        fprintf(stderr,
         "\ngraphSetRange: improper range specified, using defaults\n");
        return;
     }
     graph->yRangeType[curveNum] = GraphSpecRange;
     graph->yRange[curveNum].minVal = minVal;
     graph->yRange[curveNum].maxVal = maxVal;
    }
  }
}




/*
 *   Set the  axis display range of a Graph plot to DEFAULT
 *
 *   input parameters:
 *
 *   graph        - pointer to Graph data structure
 */
#ifdef _NO_PROTO
void graphSetRangeDefault(graph,curveNum,axis)
  Graph *graph;
  int curveNum;
  char axis;
#else
void graphSetRangeDefault( Graph *graph, int curveNum, char axis)
#endif
{
  if (curveNum < 0 || curveNum >= graph->nBuffers) {
        fprintf(stderr,
         "\ngraphSetRangeDefault: illegal curve (number %d) specified\n",
		curveNum);
        return;
  }

  if (axis == 'X' || axis == 'x') {
     graph->xRangeType[curveNum] = GraphDefaultRange;
  } else if (axis == 'Y' || axis == 'y') {
     graph->yRangeType[curveNum] = GraphDefaultRange;
  }

}



/*
 *   Set the  X axis binning parameters for a Histogram type of Graph...
 *
 *   input parameters:
 *
 *   graph       - pointer to Graph data structure
 *   curveNum	 - curve/data set number whose binning is being set
 *   nBins	 - number of bins to divide the range into
 */

void graphSetBins(graph,curveNum,nBins)
  Graph *graph;
  int curveNum;
  int nBins;
{
  if (graph->type != GraphBar) {
      fprintf(stderr,"\ngraphSetBins: this graph not of type GraphBar\n");
      return;
  }

  if (curveNum < 0 || curveNum >= graph->nBuffers) {
      fprintf(stderr,"\ngraphSetBins: illegal curve (number %d) specified\n",
		curveNum);
      return;
  }
  graph->nBins[curveNum] = nBins;

}


/*
 *   Set the display type of a Graph plot
 *
 *   input parameters:
 *
 *   graph       - pointer to Graph data structure
 *   type        - display type
 */
void graphSetDisplayType(graph,type)
  Graph *graph;
  GraphType type;
{
  if (type == GraphBar) {
        graph->type = GraphBar;
  } else if (type == GraphLine) {
        graph->type = GraphLine;
  } else if (type == GraphPoint)  {
        graph->type = GraphPoint;
  } else {
        fprintf(stderr,"\ngraphSetDisplayType: invalid display type\n");
  }
}




/*
 *   Print (dump in xwd format, then pipe to `xpr -device ps' then pipe
 *   to printer defined by PSPRINTER environment variable...)
 *
 *   input parameters:
 *
 *   graph        - pointer to Graph data structure
 */
void graphPrint(graph)
  Graph *graph;
{

  utilPrint(graph->display, graph->window, XWD_TEMP_FILE_STRING);

}




/*
 *   Set the legend parameters
 *
 *   input parameters:
 *
 *   graph        - pointer to Graph data structure
 *   legendTitle - title string
 *   legendArray - pointer to array of legend entry strings
 */
void graphSetLegend(graph, legendTitle, legendArray)
  Graph *graph;
  char *legendTitle;
  char **legendArray;
{
  int i, legendWidth = 0, legendHeight= 0;
  XWindowAttributes parentWindowAttributes;
  XSetWindowAttributes windowAttributes;

  int rootX, rootY;
  Window child;


  if (graph->legend) XDestroyWindow(graph->display,graph->legendWindow);

  graph->legend = TRUE;

  if (graph->legendTitle != NULL) free( (char *) graph->legendTitle);
  graph->legendTitle = malloc(strlen(legendTitle)+1);
  strcpy(graph->legendTitle,legendTitle);

  if (graph->legendArray != NULL) {
     for (i=0; i<graph->nBuffers; i++) free( (char *) graph->legendArray[i]);
     free( (char *) graph->legendArray);
  }
  graph->legendArray = (char **) malloc(graph->nBuffers*sizeof(char *));
  for (i=0; i<graph->nBuffers; i++){
     graph->legendArray[i] = malloc(strlen(legendArray[i])+1);
     strcpy(graph->legendArray[i],legendArray[i]);
  }


/*
 * loop to determine required size of legend window
 */
  legendWidth = XTextWidth(graph->titleFontStruct,legendTitle,
                        strlen(legendTitle)) + 2*MINBORDER;
  for (i=0; i<graph->nBuffers; i++) {
      legendWidth = max(legendWidth,
                        XTextWidth(graph->axesFontStruct,legendArray[i],
                        strlen(legendArray[i]))+graph->axesFontHeight+
                        4*MINBORDER);

  }

  legendHeight = 2*MINBORDER + graph->nBuffers*MINBORDER + MINBORDER +
                        graph->titleFontHeight +
                        graph->nBuffers*graph->axesFontHeight;

/*
 * create a window butting up against graph window;
 * legendWindow is created as child of graph wndow. 
 */
  graph->legendWindow = XCreateSimpleWindow(graph->display,
                                graph->window,
                                graph->w-legendWidth
                                        -LEGENDOFFSET-(2*BORDERLINEWIDTH),
                                LEGENDOFFSET,
                                legendWidth, legendHeight,
                                BORDERLINEWIDTH,
                                graph->forePixel, graph->backPixel);

/*
 * Set backing store attribute from parent window
 *  (Xt and XView differ on the default)
 */
  XGetWindowAttributes(graph->display, graph->window, &parentWindowAttributes);
  windowAttributes.backing_store = parentWindowAttributes.backing_store;
  XChangeWindowAttributes(graph->display, graph->legendWindow,
        CWBackingStore, &windowAttributes);

  XMapRaised(graph->display,graph->legendWindow);


}


/*
 *   Draw the legend
 *
 *   input parameters:
 *
 *   graph        - pointer to Graph data structure
 */
void graphDrawLegend(graph)
  Graph *graph;
{
  int i;
  Window child;


  if (!graph->legend) return;

/* title */
  XSetFont(graph->display,graph->gc,graph->titleFontStruct->fid);
  XSetForeground( graph->display, graph->gc, graph->forePixel);
  XDrawString(graph->display,graph->legendWindow,graph->gc,
        MINBORDER, graph->titleFontHeight + MINBORDER,
        graph->legendTitle,strlen(graph->legendTitle));

/* legend color-key rectangles */
/* (use axesFontHeight as both Cell width and height */
  XSetFont(graph->display,graph->gc,graph->axesFontStruct->fid);
  for (i=0; i<graph->nBuffers; i++) {
     XSetForeground( graph->display, graph->gc, graph->dataPixel[i]);
     XFillRectangle( graph->display, graph->legendWindow, graph->gc, MINBORDER,
        3*MINBORDER + graph->titleFontHeight +
           i*(graph->axesFontHeight+MINBORDER),
        graph->axesFontHeight, graph->axesFontHeight);
  }

/* legend strings */
  XSetForeground( graph->display, graph->gc, graph->forePixel);
  for (i=0; i<graph->nBuffers; i++) {
     XDrawString(graph->display,graph->legendWindow,graph->gc,
        graph->axesFontHeight + 2*MINBORDER,
        3*MINBORDER + graph->titleFontHeight + 2*graph->axesFontHeight/3 +
           i*(graph->axesFontHeight+MINBORDER),
        graph->legendArray[i],strlen(graph->legendArray[i]));
  }

  XRaiseWindow(graph->display,graph->legendWindow);

}



