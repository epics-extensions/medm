/***
 ***   GRAPH3D Plot routines & data structures
 ***
 ***	(MDA) 15 June 1990
 ***
 ***/

#include <X11/Xlib.h>


#include <math.h>
#include <stdio.h>

#include "Graph3d.h"


#define MINBORDER 3		/* minimum distance from window edge (pixels) */

#define RELATIVETICKSIZE 0.015	/* size of tick mark relative to window */
#define NTICKS 4		/* `preferred' number of ticks for axes */

#define DATALINEWIDTH 1		/* line width for data lines (pixels) */
#define BORDERLINEWIDTH 1	/* line width for lines (pixels) */


#define DEFAULTNBINS	10
#define DEFAULTLOWVAL	0.0
#define DEFAULTHIGHVAL	10.0

/*
 * define constants for XListFontsWithInfo call
 */
#define FONTWILDCARD "*"
#define MAXFONTS 1




/*
 * Initialize the Graph3d
 * function:  Graph3d *graph3dInit()
 *
 *   input parameters:
 *
 *   display	 - pointer to Display structure
 *   screen	 - integer screen number
 *   window	 - Window to output graphics into
 */

Graph3d *graph3dInit( display, screen, window)
  Display *display;
  int     screen;
  Window  window;
{
Graph3d *graph3d;
XWindowAttributes windowAttributes;
int     i, w, h;

XColor color, ignore;
unsigned long recentGoodColor;

#define DEFAULTMAXCOLORINDEX 10

/*
 * there should be  DEFAULTMAXCOLORINDEX  of these 
*/
static char *colorArray[] = {	"black",
				"violet",
				"violet red",
				"red",
				"orange red",
				"orange",
				"gold",
				"wheat",
				"yellow",
				"white",};



graph3d = (Graph3d *) malloc( sizeof( Graph3d ) );
if (!graph3d) return;

XGetWindowAttributes(display, window, &windowAttributes);
w = windowAttributes.width;
h = windowAttributes.height;

/* 
 * initialize some useful fields
 */
graph3d->display = display;
graph3d->screen = screen;
graph3d->window = window;

/* 
 * description of extent of graph3d in pixel coordinates...
 */
graph3d->w      = w;
graph3d->h      = h;

/*
 * create a pixmap
 */
graph3d->pixmap = XCreatePixmap(graph3d->display,graph3d->window,
			graph3d->w,graph3d->h,
			DefaultDepth(graph3d->display,graph3d->screen));

/* 
 * create a default, writable GC
 */
graph3d->gc = XCreateGC(graph3d->display,DefaultRootWindow(graph3d->display),
		NULL,NULL);

/*
 * and clear all the other fields (since some compilers do, some don't...)
 */
graph3d->dataPoints = NULL;
graph3d->titleFont = NULL;
graph3d->axesFont = NULL;
graph3d->title = NULL;
graph3d->xAxisLabel = NULL;
graph3d->yAxisLabel = NULL;
graph3d->devicePoints = NULL;

graph3d->xRangeType = Graph3dDefaultRange;
graph3d->xRange.minVal = DEFAULTLOWVAL;
graph3d->xRange.maxVal = DEFAULTHIGHVAL;
graph3d->yRangeType = Graph3dDefaultRange;
graph3d->yRange.minVal = DEFAULTLOWVAL;
graph3d->yRange.maxVal = DEFAULTHIGHVAL;
graph3d->zRangeType = Graph3dDefaultRange;
graph3d->zRange.minVal = DEFAULTLOWVAL;
graph3d->zRange.maxVal = DEFAULTHIGHVAL;

graph3d->xBins = DEFAULTNBINS;
graph3d->yBins = DEFAULTNBINS;
graph3d->zBins = DEFAULTMAXCOLORINDEX;

graph3d->zPixels = (unsigned long *) 
	malloc(graph3d->zBins*sizeof(unsigned long));

/* and initialize color table */
for (i = 0; i < DEFAULTMAXCOLORINDEX; i++) {
    if(!XAllocNamedColor(graph3d->display,
       DefaultColormap(graph3d->display,graph3d->screen),
       colorArray[i],&color,&ignore)) {
         fprintf(stderr,"\ngraph3dInit:  couldn't allocate color %s\n", 
		colorArray[i]);
         graph3d->zPixels[i] = recentGoodColor; 
    } else {
        graph3d->zPixels[i] = color.pixel;
	recentGoodColor = color.pixel;
    }
}

graph3d->setOwnColors = FALSE;	/* will user fill in own pixel values later? */
graph3d->setOwnFonts = FALSE;	/* will user fill in own font values later? */


return(graph3d);

}



/*
 * Terminate the Graph3d
 * function:  void graph3dTerm()
 */

void graph3dTerm(graph3d)
  Graph3d *graph3d;
{
/***
 *** free the space used and referenced by the graph3d
 ***/

/* free the pixmap */
  XFreePixmap(graph3d->display,graph3d->pixmap);

/* likewise, free the graphics context and font structures */
  XFreeGC(graph3d->display,graph3d->gc);
  if (!graph3d->setOwnFonts) {
    XFreeFont(graph3d->display,graph3d->titleFontStruct);
    XFreeFont(graph3d->display,graph3d->axesFontStruct);
  }

/* and finally free the graph3d space itself */
  free( (char *) graph3d->devicePoints);
  free((char *) graph3d->zPixels);

  if (graph3d->storageMode == Graph3dInternal) {
      free( (char *) graph3d->dataPoints);
      free( (char *) graph3d->titleFont);
      free( (char *) graph3d->axesFont);
      free( (char *) graph3d->title);
      free( (char *) graph3d->xAxisLabel);
      free( (char *) graph3d->yAxisLabel);
  }

  free( (char *) graph3d);

}




/*
 * Resize the Graph3d
 * function: void graph3dResize()
 */

void graph3dResize(graph3d)
  Graph3d  *graph3d;
{
XWindowAttributes windowAttributes;


/*
 * check to see if window has changed size
 *   if so, then Free the pixmap, fetch a new one and continue
 */
  XGetWindowAttributes(graph3d->display, graph3d->window, &windowAttributes);
  if (graph3d->w != windowAttributes.width  ||
      graph3d->h != windowAttributes.height){

      graph3d->w = windowAttributes.width;
      graph3d->h = windowAttributes.height;
      XFreePixmap(graph3d->display,graph3d->pixmap);
      graph3d->pixmap = XCreatePixmap(graph3d->display,graph3d->window,
                graph3d->w,graph3d->h,
                DefaultDepth(graph3d->display,graph3d->screen));
  }
}




/*
 * Draw the Graph3d
 * function: void graph3dDraw()
 */

void graph3dDraw(graph3d)
  Graph3d  *graph3d;
{

int i, j, textWidth;
double xScale, yScale;
int xScaledValue, yScaledValue;
int prevX, prevY;
double xMaxValue = (double)(-HUGE_VAL), xMinValue = (double)(HUGE_VAL);
double yMaxValue = (double)(-HUGE_VAL), yMinValue = (double)(HUGE_VAL);
double zMaxValue = (double)(-HUGE_VAL), zMinValue = (double)(HUGE_VAL);
double zBinMax = (double)(-HUGE_VAL), zBinMin = (double)(HUGE_VAL), zBinRange;
double xValueScale, yValueScale, xInterval, yInterval;
double xIntervalBins, yIntervalBins;
int tickPosition, xPosition, yPosition, tempPosition;
int nDisplayPoints;
char *valueString = "            ";	/* a 12 char blank string */
double **binnedData;			/* an array of pointers to doubles */
					/* will become  binnedData[][]  */
int offset, xBinNum, yBinNum, binColorIndex;
double xBinWidth, yBinWidth;



XSetForeground(graph3d->display,graph3d->gc,graph3d->backPixel);
XFillRectangle( graph3d->display,graph3d->pixmap,graph3d->gc,0,0,
		graph3d->w,graph3d->h);



/* 
 * calculate pixel to value scaling
 *
 *   examine range specifiers to see what limits the user wants
 */

if (graph3d->xRangeType == Graph3dDefaultRange) {     /* go with default */
    for (i = 0; i < graph3d->nPoints; i++) {
       xMaxValue = max(xMaxValue,(graph3d->dataPoints)[i].x);
       xMinValue = min(xMinValue,(graph3d->dataPoints)[i].x);
    }
} else {
    xMaxValue = graph3d->xRange.maxVal;
    xMinValue = graph3d->xRange.minVal;
}

if (graph3d->yRangeType == Graph3dDefaultRange) {     /* go with default */
    for (i = 0; i < graph3d->nPoints; i++) {
       yMaxValue = max(yMaxValue,(graph3d->dataPoints)[i].y);
       yMinValue = min(yMinValue,(graph3d->dataPoints)[i].y);
    }
} else {
    yMaxValue = graph3d->yRange.maxVal;
    yMinValue = graph3d->yRange.minVal;
}

/* 
 *   first loop to determine number of displayable points;
 *    and then cut on Z (if appropriate)
 */
nDisplayPoints = 0;
if (graph3d->zRangeType == Graph3dDefaultRange) {     /* go with default */
    for (i = 0; i < graph3d->nPoints; i++) {
       if ((graph3d->dataPoints[i].x <= xMaxValue &&
            graph3d->dataPoints[i].x >= xMinValue) && 
           (graph3d->dataPoints[i].y <= yMaxValue &&
            graph3d->dataPoints[i].y >= yMinValue)) {
		zMaxValue = max(zMaxValue,(graph3d->dataPoints)[i].z);
		zMinValue = min(zMinValue,(graph3d->dataPoints)[i].z);
		nDisplayPoints++;
       }
    }
} else {
    for (i = 0; i < graph3d->nPoints; i++) {
       if ((graph3d->dataPoints[i].x <= xMaxValue &&
            graph3d->dataPoints[i].x >= xMinValue) && 
           (graph3d->dataPoints[i].y <= yMaxValue &&
            graph3d->dataPoints[i].y >= yMinValue)) {
		nDisplayPoints++;
       }
    }
    zMaxValue = graph3d->zRange.maxVal;
    zMinValue = graph3d->zRange.minVal;
}



/* 
 * perform linear scaling (get nice values) for X and Y axes
 */
linear_scale(xMinValue,xMaxValue,NTICKS,&xMinValue,&xMaxValue,&xInterval);
linear_scale(yMinValue,yMaxValue,NTICKS,&yMinValue,&yMaxValue,&yInterval);

if (graph3d->type == Graph3dGrid){
   xIntervalBins = (xMaxValue - xMinValue) / (double) graph3d->xBins;
   yIntervalBins = (yMaxValue - yMinValue) / (double) graph3d->yBins;
}


/* 
 * Draw axes first
 */

XSetForeground( graph3d->display, graph3d->gc, graph3d->forePixel);
/* left side axis */
XDrawLine(graph3d->display,graph3d->pixmap,graph3d->gc,
		(int)(GRAPH3D_MARGIN*graph3d->w),
		(int)(GRAPH3D_MARGIN*graph3d->h),
		(int)(GRAPH3D_MARGIN*graph3d->w),
		(int)(GRAPH3D_DRAWINGAREA*graph3d->h) +
			(int)(GRAPH3D_MARGIN*graph3d->h));
/* right side axis */
XDrawLine(graph3d->display,graph3d->pixmap,graph3d->gc,
		(int)((1.0-GRAPH3D_MARGIN)*graph3d->w),
		(int)(GRAPH3D_MARGIN*graph3d->h),
		(int)((1.0-GRAPH3D_MARGIN)*graph3d->w),
		(int)(GRAPH3D_DRAWINGAREA*graph3d->h) +
			(int)(GRAPH3D_MARGIN*graph3d->h));
/* top line */
XDrawLine(graph3d->display,graph3d->pixmap,graph3d->gc,
		(int)(GRAPH3D_MARGIN*graph3d->w),
		(int)(GRAPH3D_MARGIN*graph3d->h),
		(int)((GRAPH3D_DRAWINGAREA+GRAPH3D_MARGIN)*graph3d->w),
		(int)(GRAPH3D_MARGIN*graph3d->h));
/* bottom line */
XDrawLine(graph3d->display,graph3d->pixmap,graph3d->gc,
		(int)(GRAPH3D_MARGIN*graph3d->w),
		(int)((1.0-GRAPH3D_MARGIN)*graph3d->h),
		(int)((GRAPH3D_DRAWINGAREA+GRAPH3D_MARGIN)*graph3d->w),
		(int)((1.0-GRAPH3D_MARGIN)*graph3d->h));






/*
 * now put up the data based on type
 */


XSetForeground( graph3d->display, graph3d->gc, graph3d->dataPixel);

xScale = (GRAPH3D_DRAWINGAREA * graph3d->w) / (xMaxValue - xMinValue);
yScale = (GRAPH3D_DRAWINGAREA * graph3d->h)/(yMaxValue - yMinValue);



switch(graph3d->type) {

case Graph3dGrid: 
/* 
 * loop over points, put in proper bins, then put up colored rectangles
 */
  xBinWidth = (GRAPH3D_DRAWINGAREA*graph3d->w)/((double) graph3d->xBins);
  yBinWidth = (GRAPH3D_DRAWINGAREA*graph3d->h)/((double) graph3d->yBins);

/*
 * create our 2D array of doubles
 */
  binnedData = (double **) (malloc(graph3d->xBins*sizeof(double *)));
  for (i = 0; i < graph3d->xBins; i++)
        binnedData[i] = (double *) 
		malloc((unsigned) ( graph3d->yBins * sizeof(double)));


  zBinMin = zMinValue;
  zBinMax = zMaxValue;
  zBinRange = zBinMax - zBinMin;


  for (i = 0; i < graph3d->xBins; i++)
     for (j = 0; j < graph3d->yBins; j++)
	  binnedData[i][j] = zBinMin;


  for (i = 0; i < graph3d->nPoints; i++) {

     if ((graph3d->dataPoints[i].x <= xMaxValue &&
         graph3d->dataPoints[i].x >= xMinValue) &&
        (graph3d->dataPoints[i].y <= yMaxValue &&
         graph3d->dataPoints[i].y >= yMinValue)) {

	/* since arrays from [0]-[nBins-1] : make sure they're in that range */
	   xBinNum = max(0,min( graph3d->xBins-1,
			  (int) ((graph3d->dataPoints[i].x - xMinValue) / 
				xIntervalBins)));
	   yBinNum = max(0,min( graph3d->yBins-1,
			  (int) ((yMaxValue - graph3d->dataPoints[i].y) / 
				yIntervalBins)));

	/* set value to maximum of current value or old stored value */
	   binnedData[xBinNum][yBinNum] = max(graph3d->dataPoints[i].z,
				binnedData[xBinNum][yBinNum]);

     }
  }



  for (i = 0; i < graph3d->xBins; i++) {
     for (j = 0; j < graph3d->yBins; j++) {
   /*
    * color the bin/rectangle based on it's value:
    *   binColor now an index between 0 and graph3d->zBins
    *
    *   note: clip z values at zBinMin,zBinMax
    */
	if (binnedData[i][j] > zBinMax) binnedData[i][j] = zBinMax;
	if (binnedData[i][j] < zBinMin) binnedData[i][j] = zBinMin;

	binColorIndex =  floor(graph3d->zBins*(binnedData[i][j]-zBinMin)/
					zBinRange);
        binColorIndex = min(binColorIndex,graph3d->zBins-1);

	XSetForeground(graph3d->display, graph3d->gc, 
				graph3d->zPixels[binColorIndex]);
	XFillRectangle( graph3d->display,graph3d->pixmap , graph3d->gc,
                        (int) ((graph3d->w*GRAPH3D_MARGIN) + i*xBinWidth),
                        (int) ((graph3d->h*GRAPH3D_MARGIN)+ j*yBinWidth),
			(int) xBinWidth, (int) yBinWidth );
     }
  }

  free(binnedData);
  break;



case Graph3dPoint:
/* 
 * loop over points and put up as points 
 */

  XSetForeground(graph3d->display,graph3d->gc,graph3d->dataPixel);

  for (i = 0; i < graph3d->nPoints; i++) {
     if ((graph3d->dataPoints[i].x <= xMaxValue &&
         graph3d->dataPoints[i].x >= xMinValue) ||
         (graph3d->dataPoints[i].y <= yMaxValue &&
         graph3d->dataPoints[i].y >= yMinValue)) {
	    if (zBinMin > graph3d->dataPoints[i].z) 
			zBinMin = graph3d->dataPoints[i].z;
	    if (zBinMax < graph3d->dataPoints[i].z) 
			zBinMax = graph3d->dataPoints[i].z;
     }
  }
  zBinRange = zBinMax - zBinMin;


  j = 0;

  for (i = 0; i < graph3d->nPoints; i++) {
     if ((graph3d->dataPoints[i].x <= xMaxValue &&
         graph3d->dataPoints[i].x >= xMinValue) ||
         (graph3d->dataPoints[i].y <= yMaxValue &&
         graph3d->dataPoints[i].y >= yMinValue)) {

      /* xScaledValue is more sensible than yScaledValue, it is # of pixels */
	xScaledValue = (int) (((graph3d->dataPoints)[i].x - xMinValue)*xScale);

      /* yScaledValue is sort of inverse since upper left is (0,0), */
      /*   this value is actually the number of pixels NOT in bar/rectangle */
	yScaledValue = (int) ((yMaxValue - (graph3d->dataPoints)[i].y)*yScale);

	graph3d->devicePoints[j].x = (int) ((graph3d->w*GRAPH3D_MARGIN) + xScaledValue);
	graph3d->devicePoints[j].y = (int) ((graph3d->h*GRAPH3D_MARGIN) + yScaledValue);


	binColorIndex =  floor(graph3d->zBins*
				(graph3d->dataPoints[i].z - zBinMin)/zBinRange);
        binColorIndex = min(binColorIndex,graph3d->zBins-1);

	XSetForeground(graph3d->display, graph3d->gc, 
				graph3d->zPixels[binColorIndex]);
	XDrawPoint( graph3d->display, graph3d->pixmap , graph3d->gc,
			graph3d->devicePoints[j].x,graph3d->devicePoints[j].y);

	j++;
     }
  }

  break;

case Graph3dContour: 
/* 
 * loop over points, put in proper bins, then put up colored rectangles
 */
  xBinWidth = (GRAPH3D_DRAWINGAREA*graph3d->w)/((double) graph3d->xBins);
  yBinWidth = (GRAPH3D_DRAWINGAREA*graph3d->h)/((double) graph3d->yBins);

/*
 * create our 2D array of doubles
 */
  binnedData = (double **) (malloc(graph3d->xBins*sizeof(double *)));
  for (i = 0; i < graph3d->xBins; i++)
        binnedData[i] = (double *) 
		malloc((unsigned) ( graph3d->yBins * sizeof(double)));


  zBinMin = zMinValue;
  zBinMax = zMaxValue;
  zBinRange = zBinMax - zBinMin;


  for (i = 0; i < graph3d->xBins; i++)
     for (j = 0; j < graph3d->yBins; j++)
	  binnedData[i][j] = zBinMin;


  for (i = 0; i < graph3d->nPoints; i++) {

     if ((graph3d->dataPoints[i].x <= xMaxValue &&
         graph3d->dataPoints[i].x >= xMinValue) &&
        (graph3d->dataPoints[i].y <= yMaxValue &&
         graph3d->dataPoints[i].y >= yMinValue)) {

	/* since arrays from [0]-[nBins-1] : make sure they're in that range */
	   xBinNum = max(0,min( graph3d->xBins-1,
			  (int) ((graph3d->dataPoints[i].x - xMinValue) / 
				xIntervalBins)));
	   yBinNum = max(0,min( graph3d->yBins-1,
			  (int) ((yMaxValue - graph3d->dataPoints[i].y) / 
				yIntervalBins)));

	/* set value to maximum of current value or old stored value */
	   binnedData[xBinNum][yBinNum] = max(graph3d->dataPoints[i].z,
				binnedData[xBinNum][yBinNum]);

     }
  }



  for (i = 0; i < graph3d->xBins; i++) {
     for (j = 0; j < graph3d->yBins; j++) {
   /*
    * color the bin/rectangle based on it's value:
    *   binColor now an index between 0 and graph3d->zBins
    *
    *   note: clip z values at zBinMin,zBinMax
    */
	if (binnedData[i][j] > zBinMax) binnedData[i][j] = zBinMax;
	if (binnedData[i][j] < zBinMin) binnedData[i][j] = zBinMin;

	binColorIndex =  floor(graph3d->zBins*(binnedData[i][j]-zBinMin)/
					zBinRange);
        binColorIndex = min(binColorIndex,graph3d->zBins-1);

	XSetForeground(graph3d->display, graph3d->gc, 
				graph3d->zPixels[binColorIndex]);
	XFillRectangle( graph3d->display,graph3d->pixmap , graph3d->gc,
                        (int) ((graph3d->w*GRAPH3D_MARGIN) + i*xBinWidth),
                        (int) ((graph3d->h*GRAPH3D_MARGIN)+ j*yBinWidth),
			(int) xBinWidth, (int) yBinWidth );
     }
  }

  free(binnedData);
  break;


}





/* 
 * Draw axes
 */

XSetForeground( graph3d->display, graph3d->gc, graph3d->forePixel);
/* left side axis */
XDrawLine(graph3d->display,graph3d->pixmap,graph3d->gc,
		(int)(GRAPH3D_MARGIN*graph3d->w),
		(int)(GRAPH3D_MARGIN*graph3d->h),
		(int)(GRAPH3D_MARGIN*graph3d->w),
		(int)(GRAPH3D_DRAWINGAREA*graph3d->h) +
			(int)(GRAPH3D_MARGIN*graph3d->h));
/* right side axis */
XDrawLine(graph3d->display,graph3d->pixmap,graph3d->gc,
		(int)((1.0-GRAPH3D_MARGIN)*graph3d->w),
		(int)(GRAPH3D_MARGIN*graph3d->h),
		(int)((1.0-GRAPH3D_MARGIN)*graph3d->w),
		(int)(GRAPH3D_DRAWINGAREA*graph3d->h) +
			(int)(GRAPH3D_MARGIN*graph3d->h));
/* top line */
XDrawLine(graph3d->display,graph3d->pixmap,graph3d->gc,
		(int)(GRAPH3D_MARGIN*graph3d->w),
		(int)(GRAPH3D_MARGIN*graph3d->h),
		(int)((GRAPH3D_DRAWINGAREA+GRAPH3D_MARGIN)*graph3d->w),
		(int)(GRAPH3D_MARGIN*graph3d->h));
/* bottom line */
XDrawLine(graph3d->display,graph3d->pixmap,graph3d->gc,
		(int)(GRAPH3D_MARGIN*graph3d->w),
		(int)((1.0-GRAPH3D_MARGIN)*graph3d->h),
		(int)((GRAPH3D_DRAWINGAREA+GRAPH3D_MARGIN)*graph3d->w),
		(int)((1.0-GRAPH3D_MARGIN)*graph3d->h));



/* 
 * Put up Value axis tick marks and labels
 */

XSetBackground( graph3d->display, graph3d->gc, graph3d->backPixel);
XSetForeground( graph3d->display, graph3d->gc, graph3d->forePixel);


for (yValueScale=yMinValue; yValueScale<=yMaxValue; yValueScale+=yInterval){

   tickPosition = (int)((yMaxValue-yValueScale)*yScale) + 
					(int)(graph3d->h*GRAPH3D_MARGIN);

  /* left side ticks (could do grids right around here...) */
   XDrawLine(graph3d->display,graph3d->pixmap,graph3d->gc,
		(int)(GRAPH3D_MARGIN*graph3d->w) 
			- (int)(RELATIVETICKSIZE
			* (min(graph3d->w,graph3d->h))/2),
		tickPosition,
		(int)(GRAPH3D_MARGIN*graph3d->w) 
			+ (int)(RELATIVETICKSIZE
			* (min(graph3d->w,graph3d->h))/2),
		tickPosition);

  /* right side ticks */
   XDrawLine(graph3d->display,graph3d->pixmap,graph3d->gc,
		(int)((1.0-GRAPH3D_MARGIN)*graph3d->w) 
		    - (int)(RELATIVETICKSIZE*(min(graph3d->w,graph3d->h))/2),
		tickPosition,
		(int)((1.0-GRAPH3D_MARGIN)*graph3d->w) 
		    + (int)(RELATIVETICKSIZE*(min(graph3d->w,graph3d->h))/2),
		tickPosition);

  /* 
   * tick labelling (left side only)
   */
   sprintf(valueString,"%.3g",yValueScale);
   textWidth = XTextWidth(graph3d->axesFontStruct,valueString,
			strlen(valueString));
   XSetFont(graph3d->display,graph3d->gc,graph3d->axesFontStruct->fid);
   XDrawString(graph3d->display,graph3d->pixmap,graph3d->gc,
	(int)(GRAPH3D_MARGIN*graph3d->w)-textWidth
		-(int)(RELATIVETICKSIZE*(min(graph3d->w,graph3d->h))),
	tickPosition,
	valueString,strlen(valueString));

}



/* 
 * Put up independent variable axis (X) tick marks and labelling
 */

for (xValueScale=xMinValue; xValueScale<=xMaxValue; xValueScale+=xInterval){

      tickPosition = (int)((xValueScale - xMinValue)*xScale)
	+ (int)(graph3d->w*GRAPH3D_MARGIN);

     /* bottom ticks (could do grids right around here...) */
      XDrawLine(graph3d->display,graph3d->pixmap,graph3d->gc,
		tickPosition,
		(int)((1.0-GRAPH3D_MARGIN)*graph3d->h) 
			- (int)(RELATIVETICKSIZE
			* (min(graph3d->h,graph3d->w))/2),
		tickPosition,
		(int)((1.0-GRAPH3D_MARGIN)*graph3d->h) 
			+ (int)(RELATIVETICKSIZE
			* (min(graph3d->h,graph3d->w))/2));

     /* top ticks */
      XDrawLine(graph3d->display,graph3d->pixmap,graph3d->gc,
		tickPosition,
		(int)(GRAPH3D_MARGIN*graph3d->h) 
		    - (int)(RELATIVETICKSIZE*(min(graph3d->h,graph3d->w))/2),
		tickPosition,
		(int)(GRAPH3D_MARGIN*graph3d->h) 
		    + (int)(RELATIVETICKSIZE*(min(graph3d->h,graph3d->w))/2));

    /* 
     * tick labelling (bottom only)
     */
      sprintf(valueString,"%.3g",xValueScale);
      textWidth = XTextWidth(graph3d->axesFontStruct,valueString,
			strlen(valueString));
      XSetFont(graph3d->display,graph3d->gc,graph3d->axesFontStruct->fid);
      XDrawString(graph3d->display,graph3d->pixmap,graph3d->gc,
	tickPosition - textWidth/2,
	(int)((1.0-GRAPH3D_MARGIN)*graph3d->h) 
	   + (int)(RELATIVETICKSIZE*(min(graph3d->h,graph3d->w)))/2
	   + graph3d->axesFontHeight,
	valueString,strlen(valueString));
}

     
     

/*
 * put down title
 */
if (graph3d->title != NULL) {
  textWidth = XTextWidth(graph3d->titleFontStruct,graph3d->title,
			strlen(graph3d->title));
  XSetFont(graph3d->display,graph3d->gc,graph3d->titleFontStruct->fid);
  XDrawString(graph3d->display,graph3d->pixmap,graph3d->gc,
	((graph3d->w)/2 -  textWidth/2),
	(int)(1.5*graph3d->titleFontHeight),graph3d->title,
	strlen(graph3d->title));
}


/*
 * label axes
 */
XSetFont(graph3d->display,graph3d->gc,graph3d->axesFontStruct->fid);

/*
 * put down x axis label
 */
if (graph3d->xAxisLabel != NULL) {
  textWidth = XTextWidth(graph3d->axesFontStruct,graph3d->xAxisLabel,
			strlen(graph3d->xAxisLabel));
  tempPosition = ((int)((1.0-GRAPH3D_MARGIN)*graph3d->w));
  xPosition = tempPosition + (graph3d->w - tempPosition)/2 - textWidth/2;
  xPosition = (xPosition+textWidth > graph3d->w - MINBORDER 
		? graph3d->w - MINBORDER - textWidth : xPosition);
  tempPosition = ((int)((1.0-GRAPH3D_MARGIN)*graph3d->h));
/* (MDA) move X axis title down 10% */
  yPosition = tempPosition + 3*(graph3d->h - tempPosition)/5 + 
		graph3d->axesFontHeight/2;
  XDrawString(graph3d->display,graph3d->pixmap,graph3d->gc,
	xPosition, yPosition,
	graph3d->xAxisLabel,strlen(graph3d->xAxisLabel));
}

/*
 * put down y axis label
 */
if (graph3d->yAxisLabel != NULL) {
  textWidth = XTextWidth(graph3d->axesFontStruct,graph3d->yAxisLabel,
			strlen(graph3d->yAxisLabel));
  tempPosition = ((int)((GRAPH3D_MARGIN)*graph3d->w));
  xPosition = tempPosition/2 - textWidth/2;
  xPosition = (xPosition < MINBORDER ? MINBORDER : xPosition);
  tempPosition = (int)(GRAPH3D_MARGIN*graph3d->h);
/* (MDA) move Y axis title up 7% */
  yPosition = 3*tempPosition/7  + graph3d->axesFontHeight/2;
  XDrawString(graph3d->display,graph3d->pixmap,graph3d->gc,
	xPosition, yPosition,
	graph3d->yAxisLabel,strlen(graph3d->yAxisLabel));
}


XCopyArea(graph3d->display,graph3d->pixmap,graph3d->window,graph3d->gc,0,0,
		graph3d->w,graph3d->h,0,0);

XFlush(graph3d->display);


}





/*
 * Refresh the display
 * (don't recalcuate, just remap the pixmap onto the window
 */

void graph3dRefresh(graph3d)
  Graph3d  *graph3d;
{
  XCopyArea(graph3d->display,graph3d->pixmap,graph3d->window,graph3d->gc,0,0,
		graph3d->w,graph3d->h,0,0);
  XFlush(graph3d->display);
}




/*
 *   Add a graph3d plot
 *
 *   input parameters:
 *
 *   graph3d	 - pointer to Graph3d data structure
 *   dataPoints  - array of XYZdataPoint-s (floating point x,y,z pairs)
 *   nPoints     - number of points
 *   type	 - graph3d type (Graph3dType): 
 *		   	Graph3dGrid, Graph3dPoint, Graph3dLine
 *   title	 - character string (ptr) for title
 *   titleFont	 - charaacter string (ptr) for font to use for title
 *   xAxisLabel	 - character string (ptr) for X axis label
 *   yAxisLabel	 - character string (ptr) for Y axis label
 *   axesFont	 - character string (ptr) for font to use for axes labels
 *   foreColor	 - character string (ptr) for color to use for foreground
 *   backColor	 - character string (ptr) for color to use for background
 *   dataColor	 - character string (ptr) for color to use for data
 *   storageMode -  graph3d data storage mode: Graph3dInternal, Graph3dExternal
 *			if Graph3dInternal, make a copy of the data; 
 *			otherwise (Graph3dExternal), just copy the pointers
 */

void graph3dSet( graph3d, dataPoints, nPoints, type, title, titleFont, 
			xAxisLabel, yAxisLabel,
			axesFont, foreColor, backColor, dataColor, storageMode)
  Graph3d		*graph3d;
  XYZdataPoint	*dataPoints;
  int             nPoints;
  Graph3dType	type;
  char		*title, *titleFont, *xAxisLabel, *yAxisLabel, *axesFont;
  char		*foreColor, *backColor, *dataColor;
  Graph3dStorageMode storageMode;
{
int stringLength = 0;
XColor color, ignore;
int fontCount;
char **fontNames;
XFontStruct *fontInfo;


if (storageMode == Graph3dExternal) {
    graph3d->storageMode = Graph3dExternal;
    graph3d->dataPoints    = dataPoints;
    graph3d->titleFont = titleFont;
    graph3d->axesFont = axesFont;
    graph3d->title = title;
    graph3d->xAxisLabel = xAxisLabel;
    graph3d->yAxisLabel = yAxisLabel;
} else {
    graph3d->storageMode = Graph3dInternal;
    if (graph3d->dataPoints) free( (char *) graph3d->dataPoints );
    graph3d->dataPoints = (XYZdataPoint *) malloc((unsigned)
			(nPoints*sizeof(XYZdataPoint)));
    memcpy((void *)graph3d->dataPoints,(void *)dataPoints,
		(size_t)(nPoints * sizeof(XYZdataPoint)) );

    if (graph3d->titleFont) free( (char *) graph3d->titleFont);
    if (titleFont != NULL) {
      stringLength = strlen(titleFont) + 1;
      graph3d->titleFont = (char *) malloc(stringLength);
      memcpy((void *)graph3d->titleFont,(void *)titleFont,(size_t)stringLength);
      graph3d->titleFont[stringLength-1] = '\0';
    }

    if (graph3d->axesFont) free( (char *) graph3d->axesFont);
    if (axesFont != NULL) {
      stringLength = strlen(axesFont) + 1;
      graph3d->axesFont = (char *) malloc(stringLength);
      memcpy((void *)graph3d->axesFont,(void *)axesFont,(size_t)stringLength);
      graph3d->axesFont[stringLength-1] = '\0';
    }

    if (graph3d->title) free( (char *) graph3d->title);
    if (title != NULL) {
      stringLength = strlen(title) + 1;
      graph3d->title = (char *) malloc(stringLength);
      memcpy((void *)graph3d->title,(void *)title,(size_t)stringLength);
      graph3d->title[stringLength-1] = '\0';
    }

    if (graph3d->xAxisLabel) free( (char *) graph3d->xAxisLabel);
    if (xAxisLabel != NULL) {
      stringLength = strlen(xAxisLabel) + 1;
      graph3d->xAxisLabel = (char *) malloc(stringLength);
      memcpy((void *)graph3d->xAxisLabel,(void *)xAxisLabel,
		(size_t)stringLength);
      graph3d->xAxisLabel[stringLength-1] = '\0';
    }

    if (graph3d->yAxisLabel) free( (char *) graph3d->yAxisLabel);
    if (yAxisLabel != NULL) {
      stringLength = strlen(yAxisLabel) + 1;
      graph3d->yAxisLabel = (char *) malloc(stringLength);
      memcpy((void *)graph3d->yAxisLabel,(void *)yAxisLabel,
		(size_t)stringLength);
      graph3d->yAxisLabel[stringLength-1] = '\0';
    }

}
graph3d->nPoints    = nPoints;
graph3d->type	 = type;


if (graph3d->devicePoints) free( (char *) graph3d->devicePoints );
graph3d->devicePoints = (XPoint *) malloc((unsigned)
			(nPoints*sizeof(XPoint)));


/* set foreground, background and data colors based on passed-in strings */

/* use a color for the foreground if possible */
if ((DisplayCells(graph3d->display,graph3d->screen) > 2) 
		&& (!graph3d->setOwnColors)) {
    if(!XAllocNamedColor(graph3d->display,
        DefaultColormap(graph3d->display,graph3d->screen),
        foreColor,&color,&ignore)){
        fprintf(stderr,"\ngraph3dSet:  couldn't allocate color %s",foreColor);
        graph3d->forePixel  = WhitePixel(graph3d->display,graph3d->screen);
    } else {
        graph3d->forePixel = color.pixel;
    }

    if(!XAllocNamedColor(graph3d->display,
        DefaultColormap(graph3d->display,graph3d->screen),
        backColor,&color,&ignore)) {
        fprintf(stderr,"\ngraph3dSet:  couldn't allocate color %s",backColor);
        graph3d->backPixel  = BlackPixel(graph3d->display,graph3d->screen);
    } else {
        graph3d->backPixel = color.pixel;
    }

    if(!XAllocNamedColor(graph3d->display,
        DefaultColormap(graph3d->display,graph3d->screen),
        dataColor,&color,&ignore)) {
        fprintf(stderr,"\ngraph3dSet:  couldn't allocate color %s",dataColor);
        graph3d->dataPixel  = WhitePixel(graph3d->display,graph3d->screen);
    } else {
        graph3d->dataPixel = color.pixel;
    }

} else {
    graph3d->forePixel  = WhitePixel(graph3d->display,graph3d->screen);
    graph3d->backPixel  = BlackPixel(graph3d->display,graph3d->screen);
    graph3d->dataPixel  = WhitePixel(graph3d->display,graph3d->screen);
}


/*
 * load the fonts
 */
if (!graph3d->setOwnFonts) {
   if ((graph3d->titleFontStruct=XLoadQueryFont(graph3d->display,
		graph3d->titleFont)) == NULL) {
      fprintf(stderr,"display %s doesn't know font %s\n",
                DisplayString(graph3d->display),graph3d->titleFont);
      fprintf(stderr,"  using first available font\n");
      fontNames = XListFontsWithInfo(graph3d->display,FONTWILDCARD,
		MAXFONTS,&fontCount,&fontInfo);
      graph3d->titleFontStruct = XLoadQueryFont(graph3d->display,fontNames[0]);
      XFreeFontInfo(fontNames,fontInfo,fontCount);
      XFreeFontNames(fontNames);
   }

   graph3d->titleFontHeight = graph3d->titleFontStruct->ascent + 
				graph3d->titleFontStruct->descent;
   graph3d->titleFontWidth = graph3d->titleFontStruct->max_bounds.rbearing - 
				graph3d->titleFontStruct->min_bounds.lbearing;

   if ((graph3d->axesFontStruct=XLoadQueryFont(graph3d->display,
		graph3d->axesFont)) == NULL) {
      fprintf(stderr,"display %s doesn't know font %s\n",
                DisplayString(graph3d->display),graph3d->axesFont);
      fprintf(stderr,"  using first available font\n");
      graph3d->axesFontStruct = graph3d->titleFontStruct;

   }

   graph3d->axesFontHeight = graph3d->axesFontStruct->ascent + 
				graph3d->axesFontStruct->descent;
   graph3d->axesFontWidth = graph3d->axesFontStruct->max_bounds.rbearing - 
				graph3d->axesFontStruct->min_bounds.lbearing;

}

}





/*
 *   Set the  axis display range of a Graph3d plot
 *
 *   input parameters:
 *
 *   graph3d        - pointer to Graph3d data structure
 *   axis        - character specifying X or Y axis
 *   minVal      - minimum display value
 *   maxVal      - maximum display value
 */

#ifdef _NO_PROTO
void graph3dSetRange(graph3d,axis,minVal,maxVal)
  Graph3d *graph3d;
  char axis;
  double minVal,maxVal;
#else
void graph3dSetRange(Graph3d *graph3d, char axis, double minVal, double maxVal)
#endif
{
  if (axis == 'X' || axis == 'x') {
     if (minVal >= maxVal) {
        graph3d->xRangeType = Graph3dDefaultRange;
        fprintf(stderr,
         "\ngraph3dSetRange: improper range specified, using defaults\n");
        return;
     }
     graph3d->xRangeType = Graph3dSpecRange;
     graph3d->xRange.minVal = minVal;
     graph3d->xRange.maxVal = maxVal;
  } else {
    if (axis == 'Y' || axis == 'y') {
     if (minVal >=  maxVal) {
        graph3d->yRangeType = Graph3dDefaultRange;
        fprintf(stderr,
         "\ngraph3dSetRange: improper range specified, using defaults\n");
        return;
     }
     graph3d->yRangeType = Graph3dSpecRange;
     graph3d->yRange.minVal = minVal;
     graph3d->yRange.maxVal = maxVal;
    }
  }
}




/*
 *   Set the  axis display range of a Graph3d plot to DEFAULT
 *
 *   input parameters:
 *
 *   graph3d        - pointer to Graph3d data structure
 *   axis	    - character ( X or Y or Z ) denoting axis
 */
#ifdef _NO_PROTO
void graph3dSetRangeDefault(graph3d,axis)
  Graph3d *graph3d;
  char axis;
#else
void graph3dSetRangeDefault(Graph3d *graph3d, char axis)
#endif
{
  if (axis == 'X' || axis == 'x') {
     graph3d->xRangeType = Graph3dDefaultRange;
  } else if (axis == 'Y' || axis == 'y') {
     graph3d->yRangeType = Graph3dDefaultRange;
  } else if (axis == 'Z' || axis == 'z') {
     graph3d->zRangeType = Graph3dDefaultRange;
  }

}



/*
 *   Set the  X & Y axis binning parameters for a Bin type of Graph3d...
 *
 *   input parameters:
 *
 *   graph3d     - pointer to Graph3d data structure
 *   axis	  - character ( X or Y  ) denoting axis
 *   nBins	 - number of bins to divide the range into
 */

#ifdef _NO_PROTO
void graph3dSetBins(graph3d,axis,nBins)
  Graph3d *graph3d;
  char axis;
  int nBins;
#else
void graph3dSetBins(Graph3d *graph3d, char axis, int nBins)
#endif
{
  if (graph3d->type != Graph3dGrid) {
      fprintf(stderr,
	"\ngraph3dSetBins: this graph3d not of type Graph3dGrid\n");
      return;
  }

  if (axis == 'X' || axis == 'x') {
     graph3d->xBins= nBins;
  } else if (axis == 'Y' || axis == 'y') {
     graph3d->yBins = nBins;
  }

}




/*
 *   Set the  Z axis interval (bin) parameters and colors for a 
 *   Bin type of Graph3d...
 *
 *   input parameters:
 *
 *   graph3d     - pointer to Graph3d data structure
 *   nBins	 - number of intervals (bins) to divide the range into
 *   colorArray	 - an array of strings (colors) for the ranges specified by 
 *		   nBins such that array element 0 color is for background, 
 *		   ... the last element is for maximum intensity (maximum Z)
 */

void graph3dSetZBins(graph3d,nBins,colorArray)
  Graph3d *graph3d;
  int nBins;
  char **colorArray;
{
  int i;
  XColor color, ignore;
  unsigned long recentGoodColor;


  if (graph3d->type != Graph3dGrid) {
      fprintf(stderr,
	"\ngraph3dSetZBins: this graph3d not of type Graph3dGrid\n");
      return;
  }

  graph3d->zBins = nBins;

  free((char *)graph3d->zPixels);
  graph3d->zPixels = (unsigned long *) malloc(nBins*sizeof(unsigned long));

/* now reset color table */
for (i = 0; i < nBins; i++) {
    if(!XAllocNamedColor(graph3d->display,
       DefaultColormap(graph3d->display,graph3d->screen),
       colorArray[i],&color,&ignore)) {
         fprintf(stderr,
	   "\ngraph3dSetZBins:  couldn't allocate color %s\n", colorArray[i]);
         graph3d->zPixels[i] = recentGoodColor;
    } else {
        graph3d->zPixels[i] = color.pixel;
        recentGoodColor = color.pixel;
    }
}


}





/*
 *   Set the display type of a Graph3d plot
 *
 *   input parameters:
 *
 *   graph3d     - pointer to Graph3d data structure
 *   type        - display type
 */
void graph3dSetDisplayType(graph3d,type)
  Graph3d *graph3d;
  Graph3dType type;
{
  if (type == Graph3dPoint) {
        graph3d->type = Graph3dPoint;
  } else if (type == Graph3dGrid) {
        graph3d->type = Graph3dGrid;
  } else {
        fprintf(stderr,
		"\ngraph3dSetDisplayType: invalid display type\n");
  }
}


/*
 *   Print (dump in xwd format, then pipe to `xpr -device ps' then pipe
 *   to printer defined by PSPRINTER environment variable...)
 *
 *   input parameters:
 *
 *   graph3d        - pointer to Graph3d data structure
 */
void graph3dPrint(graph3d)
  Graph3d *graph3d;
{

  utilPrint(graph3d->display, graph3d->window, XWD_TEMP_FILE_STRING);

}
