/***
 ***   SEQL Plot routines & data structures
 ***
 ***	(MDA) 7 May 1990
 ***
 ***/


#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "Seql.h"


#define BINFRACTION 0.8		/* fraction of bin width for Fill Rectangle */
#define MINBORDER 3		/* minimum distance from window edge (pixels) */

#define RELATIVETICKSIZE 0.02	/* size of tick mark relative to window */
#define NTICKS 2		/* `preferred' number of ticks for axes */
#define MAXTICKS NTICKS+4	/* `maximum' number of ticks based on NTICKS */

#define DATALINEWIDTH 1		/* line width for data lines (pixels) */
#define BORDERLINEWIDTH 1	/* line width for lines (pixels) */

#define LEGENDOFFSET 1		/* amount of legend window to overlap parent */

/*
 * define constants for XListFontsWithInfo call
 */
#define FONTWILDCARD "*"
#define MAXFONTS 1


#ifdef _NO_PROTO
extern int getOKStringPosition();
extern void getOKStringPositionXY();
#else
extern int getOKStringPosition(int, int *, int *, int);
extern void getOKStringPositionXY(int *, int *, int *, int *, int, int);
#endif


/*
 * Initialize the Seql
 * function:  Seql *seqlInit()
 *
 *   input parameters:
 *
 *   display	 - pointer to Display structure
 *   screen	 - integer screen number
 *   window	 - Window to output graphics into
 */

Seql *seqlInit( display, screen, window)
    Display *display;
    int     screen;
    Window  window;
{
    Seql *seql;
    XWindowAttributes windowAttributes;
    int   i, w, h;

    seql = (Seql *) malloc( sizeof( Seql ) );
    if (!seql) return;

    XGetWindowAttributes(display, window, &windowAttributes);
    w = windowAttributes.width;
    h = windowAttributes.height;

  /* 
   * initialize some useful fields
   */
    seql->display = display;
    seql->screen = screen;
    seql->window = window;

  /* 
   * description of extent of seql in pixel coordinates...
   */
    seql->w      = w;
    seql->h      = h;
    seql->dataX0 =  SEQL_MARGIN*w;
    seql->dataY0 =  SEQL_MARGIN*h;
    seql->dataX1 =  (1.0 - SEQL_MARGIN)*w;
    seql->dataY1 =  (1.0 - SEQL_MARGIN)*h;


  /*
   * create a pixmap
   */
    seql->pixmap = XCreatePixmap(seql->display,seql->window,seql->w,seql->h,
      DefaultDepth(seql->display,seql->screen));

  /* 
   * create a default, writable GC
   */
    seql->gc = XCreateGC(seql->display,DefaultRootWindow(seql->display),
      (unsigned long)NULL,NULL);

  /*
   * and clear all the other fields (since some compilers do, some don't...)
   */
    seql->dataBuffer = NULL;
    seql->nBuffers = 0;
    seql->bufferSize = 0;
    seql->titleFont = NULL;
    seql->axesFont = NULL;
    seql->title = NULL;
    seql->xAxisLabel = NULL;
    seql->yAxisLabel = NULL;
    seql->devicePoints = NULL;

    for (i=0; i<MAXCURVES; i++) {
	seql->xRangeType[i] = SeqlDefaultRange;
	seql->xRange[i].minVal = 0.0;
	seql->xRange[i].maxVal = 10.0;
	seql->xRangeType[i] = SeqlDefaultRange;
	seql->yRange[i].minVal = 0.0;
	seql->yRange[i].maxVal = 10.0;
    }

    seql->nValues = NULL;
    seql->dataPixel = NULL;


  /*
   * and initialize the Legend fields
   */
    seql->legend = FALSE;
    seql->legendTitle = NULL;
    seql->legendArray = NULL;

    seql->setOwnColors = FALSE;	/* will user fill in own pixel values? */
    seql->setOwnFonts = FALSE;	/* will user fill in own font values? */
    seql->dontLinearScaleX = FALSE;	/* should we make nice scales, or use */
    seql->dontLinearScaleY = FALSE;	/* exact ones as specified? */

  /*
   * and initialize the Interactive field
   */
    seql->interactive = NULL;

    return(seql);

}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Set/Reset the data color for the Seql
 * function:  void seqlSetDataColor()
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void seqlSetDataColor(seql,channelNumber,colorString)
    Seql *seql;
    int channelNumber;
    char *colorString;
{
    XColor color, ignore;

    if (channelNumber > seql->nBuffers-1 || channelNumber < 0) {
	fprintf(stderr,"\nseqlSetDataColor:  invalid buffer number %d",
	  channelNumber);
	return;
    }

    if(!XAllocNamedColor(seql->display,
      DefaultColormap(seql->display,seql->screen),
      colorString,&color,&ignore)) {
        fprintf(stderr, "\nseqlSetDataColor:  couldn't allocate color %s",
	  colorString);
        seql->dataPixel[channelNumber]  = WhitePixel(seql->display,
	  seql->screen);
    } else {
        seql->dataPixel[channelNumber] = color.pixel;
    }

}





/*
 * Terminate the Seql
 * function:  void seqlTerm()
 */

void seqlTerm(seql)
    Seql *seql;
{
    int i;
  /***
  *** free the space used and referenced by the sequential data plot
  ***/

  /* free the pixmap */
    XFreePixmap(seql->display,seql->pixmap);

  /* likewise, free the graphics context and font structures */
    XFreeGC(seql->display,seql->gc);
    if (!seql->setOwnFonts) {
	XFreeFont(seql->display,seql->titleFontStruct);
	XFreeFont(seql->display,seql->axesFontStruct);
    }

  /* and finally free the sequential data plot space itself */
    free( (char *) seql->devicePoints);

    if ( seql->storageMode == SeqlInternal) {
	for (i=0; i<seql->nBuffers; i++)
	  free( (char *) seql->dataBuffer[i]);
	free( (char *) seql->dataBuffer);
	free( (char *) seql->titleFont);
	free( (char *) seql->axesFont);
	free( (char *) seql->title);
	free( (char *) seql->xAxisLabel);
	free( (char *) seql->yAxisLabel);
    }

    free( (char *) seql->nValues);
    free( (char *) seql->dataPixel);

  /* free the legend space */
    if (seql->legend) {
	free( (char *) seql->legendTitle);
	for (i=0; i<seql->nBuffers; i++)
	  free( (char *) seql->legendArray[i]);
	free( (char *) seql->legendArray);
	XDestroyWindow(seql->display, seql->legendWindow);
	seql->legend = FALSE;
 
    }

  /* turn off interactive and free the interactive structure if there is one */
    seqlSetInteractive(seql,FALSE);

    free( (char *) seql);

}





/*
 * Resize the Seql
 * function: void seqlResize()
 */

void seqlResize(seql)
    Seql  *seql;
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
    XGetWindowAttributes(seql->display, seql->window, &windowAttributes);
    if (seql->w != windowAttributes.width  ||
      seql->h != windowAttributes.height){

	seql->w = windowAttributes.width;
	seql->h = windowAttributes.height;
	seql->dataX0 = SEQL_MARGIN*windowAttributes.width;
	seql->dataY0 = SEQL_MARGIN*windowAttributes.height;
	seql->dataX1 = (1.0 - SEQL_MARGIN)*windowAttributes.width;
	seql->dataY1 = (1.0 - SEQL_MARGIN)*windowAttributes.height;

	XFreePixmap(seql->display,seql->pixmap);
	seql->pixmap = XCreatePixmap(seql->display,seql->window,
	  seql->w,seql->h,
	  DefaultDepth(seql->display,seql->screen));

      /* Relocate legend window, if it exists */
	if (seql->legend) {
	    XDestroyWindow(seql->display,seql->legendWindow);

	  /*
	   * loop to determine required size of legend window
	   */
	    legendWidth = XTextWidth(seql->titleFontStruct,seql->legendTitle,
	      strlen(seql->legendTitle)) + 2*MINBORDER;
	    for (i=0; i<seql->nBuffers; i++) {
		legendWidth = max(legendWidth,
		  XTextWidth(seql->axesFontStruct,seql->legendArray[i],
		    strlen(seql->legendArray[i]))+seql->axesFontHeight+
		  4*MINBORDER);
	    }
	    legendHeight = 2*MINBORDER + seql->nBuffers*MINBORDER + MINBORDER +
	      seql->titleFontHeight +
	      seql->nBuffers*seql->axesFontHeight;
	  /*
	   * create a window butting up against graph window;
	   * legendWindow is created as child of graph wndow.
	   */
	    seql->legendWindow = XCreateSimpleWindow(seql->display,
	      seql->window,
	      seql->w-legendWidth
	      -LEGENDOFFSET-(2*BORDERLINEWIDTH),
	      LEGENDOFFSET,
	      legendWidth, legendHeight,
	      BORDERLINEWIDTH,
	      seql->forePixel, seql->backPixel);
	  /*
	   * Set backing store attribute from parent window
	   */
	    XGetWindowAttributes(seql->display, seql->window,
	      &parentWindowAttributes);
	    setWindowAttributes.backing_store =
	      parentWindowAttributes.backing_store;
	    XChangeWindowAttributes(seql->display, seql->legendWindow,
	      CWBackingStore, &setWindowAttributes);
	    XMapRaised(seql->display,seql->legendWindow);
	}
    }
}






/*
 * Draw the Seql
 * function: void seqlDraw()
 */

void seqlDraw(seql)
    Seql  *seql;
{

    XColor color;

    int i, j, textWidth;

    double xCurveScale[MAXCURVES], yCurveScale[MAXCURVES];

    int xScaledValue, yScaledValue;
    int prevX, prevY, bufNum;

    double xCurveInterval[MAXCURVES], yCurveInterval[MAXCURVES];


    double xValueScale, yValueScale, xInterval, yInterval;
    int tickPosition, xPosition, yPosition, tempPosition;
    int goodNumberOfTicks, nDisplayPoints;
    char *valueString = "            ";	/* a 12 char blank string */

    int usedSpots[(NTICKS+1)*MAXCURVES], usedCtr;
    int stringPosition, stringPositionX, stringPositionY;

    int  used[MAXTICKS], numUsed[MAXTICKS];
        

    XSetForeground(seql->display, seql->gc, seql->backPixel);
    XFillRectangle( seql->display, seql->pixmap, seql->gc, 0, 0, seql->w, seql->h);


  /* 
   * calculate pixel to value scaling for EACH CURVE
   *
   *   examine range specifiers to see what limits the user wants
   *
   */
    for (i=0; i<min(MAXCURVES,seql->nBuffers); i++) {
	if (seql->xRangeType[i] == SeqlDefaultRange) {	/* go with default */
	    seql->xRange[i].maxVal = (double) seql->nValues[i];
	    seql->xRange[i].minVal = 0.0;
	} 

	if (seql->yRangeType[i] == SeqlDefaultRange) {	/* go with default */
	    seql->yRange[i].maxVal = (double)(-HUGE_VAL);
	    seql->yRange[i].minVal = (double)(HUGE_VAL);
	    for (j = 0; j < seql->nValues[i]; j++) {
		seql->yRange[i].maxVal = max(seql->yRange[i].maxVal,
		  (seql->dataBuffer)[i][j]);
		seql->yRange[i].minVal = min(seql->yRange[i].minVal,
		  (seql->dataBuffer)[i][j]);
	    }
	}

	if ( seql->dontLinearScaleX )
	  xCurveInterval[i] = seql->xRange[i].maxVal - 
	    seql->xRange[i].minVal;
	else
	  linear_scale(seql->xRange[i].minVal,seql->xRange[i].maxVal,
	    NTICKS, &(seql->xRange[i].minVal),&(seql->xRange[i].maxVal),
	    &xCurveInterval[i]);

	if ( seql->dontLinearScaleY )
	  yCurveInterval[i] = seql->yRange[i].maxVal - 
	    seql->yRange[i].minVal;
	else
	  linear_scale(seql->yRange[i].minVal,seql->yRange[i].maxVal,
	    NTICKS, &(seql->yRange[i].minVal),&(seql->yRange[i].maxVal),
	    &yCurveInterval[i]);


	xCurveScale[i] = (seql->dataX1 - seql->dataX0) / (seql->xRange[i].maxVal 
	  - seql->xRange[i].minVal);
	yCurveScale[i] = (seql->dataY1 - seql->dataY0) /(seql->yRange[i].maxVal 
	  - seql->yRange[i].minVal);
    }



  /*
   * Draw axes first
   */
    XSetForeground( seql->display, seql->gc, seql->forePixel);
  /* left side axis */
    XDrawLine(seql->display,seql->pixmap,seql->gc,
      seql->dataX0, seql->dataY0, seql->dataX0, seql->dataY1);
  /* right side axis */
    XDrawLine(seql->display,seql->pixmap,seql->gc,
      seql->dataX1, seql->dataY0, seql->dataX1, seql->dataY1);
  /* top line */
    XDrawLine(seql->display,seql->pixmap,seql->gc,
      seql->dataX0, seql->dataY0, seql->dataX1, seql->dataY0);
  /* bottom line */
    XDrawLine(seql->display,seql->pixmap,seql->gc,
      seql->dataX0, seql->dataY1, seql->dataX1, seql->dataY1);


  /*
   * now put up the data based on type
   */



    switch(seql->type) {


    case SeqlPolygon:
      /* 
       * loop over points and put up as filled polygon
       */

	for (bufNum = 0; bufNum < seql->nBuffers; bufNum++) {

	    XSetForeground( seql->display, seql->gc, seql->dataPixel[bufNum]);

	  /* special case for polygon: need to start at origins... */
	    seql->devicePoints[0].x = seql->dataX0;
	    seql->devicePoints[0].y = seql->dataY1;

	    nDisplayPoints = 1;
	    for (i = ((int) seql->xRange[bufNum].minVal); 
		 i < min( seql->xRange[bufNum].maxVal,
		   seql->nValues[bufNum]); i++){
	      /*
	       * do own data clipping: if Y value in range then display
	       */
		nDisplayPoints++;

	      /* xScaledValue is more sensible than yScaledValue, it is # of pixels */
		xScaledValue = (int) ((i - seql->xRange[bufNum].minVal)
		  *xCurveScale[bufNum]);

	      /* yScaledValue is sort of inverse since upper left is (0,0), */
	      /*   this value is actually the number of pixels NOT in bar/rectangle */
		if ((seql->dataBuffer)[bufNum][i] >= 
		  seql->yRange[bufNum].maxVal) {
		    yScaledValue = (int) ((seql->yRange[bufNum].maxVal 
		      - seql->yRange[bufNum].maxVal)*
		      yCurveScale[bufNum]);
		} else if ((seql->dataBuffer)[bufNum][i] <= 
		  seql->yRange[bufNum].minVal) {
		    yScaledValue = (int) ((seql->yRange[bufNum].maxVal 
		      - seql->yRange[bufNum].minVal)*
		      yCurveScale[bufNum]);
		} else {
		    yScaledValue = (int) ((seql->yRange[bufNum].maxVal - 
		      (seql->dataBuffer)[bufNum][i])*yCurveScale[bufNum]);
		}

		seql->devicePoints[nDisplayPoints-1].x = seql->dataX0 + xScaledValue;
		seql->devicePoints[nDisplayPoints-1].y = seql->dataY0 + yScaledValue;
	    }

	  /* special case for polygon: need to end at bottom of last point... */
	    seql->devicePoints[nDisplayPoints].x = 
	      seql->devicePoints[nDisplayPoints-1].x;
	    seql->devicePoints[nDisplayPoints].y = seql->dataY1;

	    XFillPolygon( seql->display,seql->pixmap , seql->gc,
	      seql->devicePoints, nDisplayPoints+1, 
	      Nonconvex, CoordModeOrigin);
	}
	break;



    case SeqlPoint:

      /* 
       * loop over points and put up as polypoints 
       */

	for (bufNum = 0; bufNum < seql->nBuffers; bufNum++) {

	    XSetForeground( seql->display, seql->gc, seql->dataPixel[bufNum]);

	    nDisplayPoints = 0;
	    for (i = ((int) seql->xRange[i].minVal); 
		 i < min((int)seql->xRange[bufNum].maxVal,
		   seql->nValues[bufNum]); i++) {

	      /*
	       * do own data clipping: if Y value in range then display
	       */

	      /* xScaledValue is more sensible than yScaledValue, it is # of pixels */
		xScaledValue = (int) ((i - seql->xRange[bufNum].minVal)
		  *xCurveScale[bufNum]);

	      /* yScaledValue is sort of inverse since upper left is (0,0), */
	      /*   this value is actually the number of pixels NOT in bar/rectangle */
		yScaledValue = (int) ((seql->yRange[bufNum].maxVal - 
		  (seql->dataBuffer)[bufNum][i])*yCurveScale[bufNum]);

		seql->devicePoints[nDisplayPoints].x = seql->dataX0 + xScaledValue;
		seql->devicePoints[nDisplayPoints].y = seql->dataY0 + yScaledValue;
		nDisplayPoints++;
	    }
	    XDrawPoints( seql->display,seql->pixmap , seql->gc,
	      seql->devicePoints, nDisplayPoints, CoordModeOrigin);
	}

	break;


    case SeqlLine:
      /* 
       * loop over points and put up as polyline
       */
	for (bufNum = 0; bufNum < seql->nBuffers; bufNum++) {

	    XSetForeground( seql->display, seql->gc, seql->dataPixel[bufNum]);

	    XSetLineAttributes( seql->display, seql->gc,
	      DATALINEWIDTH,LineSolid,CapButt,JoinMiter);

	    nDisplayPoints = 0;
	    for (i = ((int) seql->xRange[bufNum].minVal); 
		 i < min( (int)seql->xRange[bufNum].maxVal,
		   seql->nValues[bufNum]); i++) {
	      /*
	       * do own data clipping: if Y value in range then display
	       */ 

	      /* xScaledValue is more sensible than yScaledValue, it is # of pixels */
		xScaledValue = (int) ((i - seql->xRange[bufNum].minVal)
		  *xCurveScale[bufNum]);

	      /* yScaledValue is sort of inverse since upper left is (0,0), */
	      /*   this value is actually the number of pixels NOT in bar/rectangle */
		if ((seql->dataBuffer)[bufNum][i] >= seql->yRange[bufNum].maxVal) {
		    yScaledValue = (int) ((seql->yRange[bufNum].maxVal 
		      - seql->yRange[bufNum].maxVal)*yCurveScale[bufNum]);
		} else if ((seql->dataBuffer)[bufNum][i]<=
		  seql->yRange[bufNum].minVal) {
		    yScaledValue = (int) ((seql->yRange[bufNum].maxVal 
		      - seql->yRange[bufNum].minVal)*yCurveScale[bufNum]);
		} else {
		    yScaledValue = (int) ((seql->yRange[bufNum].maxVal - 
		      (seql->dataBuffer)[bufNum][i])*yCurveScale[bufNum]);
		}

		seql->devicePoints[nDisplayPoints].x = seql->dataX0 + xScaledValue;
		seql->devicePoints[nDisplayPoints].y = seql->dataY0 + yScaledValue;
		nDisplayPoints++;
	    }


	    XDrawLines( seql->display,seql->pixmap , seql->gc,
	      seql->devicePoints, nDisplayPoints, CoordModeOrigin);

	    XSetLineAttributes( seql->display, seql->gc,
	      BORDERLINEWIDTH,LineSolid,CapButt,JoinMiter);
	}

	break;


    }


  /* 
   * Put up Value axis and tick marks
   */

    XSetForeground( seql->display, seql->gc, seql->forePixel);


    usedCtr = 0;
    for (i=0; i<MAXTICKS; i++) {
	used[i] = 0;
	numUsed[i]=0;
    }

    for (bufNum = 0; bufNum < seql->nBuffers; bufNum++) {

	for (yValueScale=seql->yRange[bufNum].minVal; yValueScale <=
	       seql->yRange[bufNum].maxVal; yValueScale+=yCurveInterval[bufNum]){

	    XSetForeground( seql->display, seql->gc, seql->forePixel);

	    tickPosition = (int)((seql->yRange[bufNum].maxVal-yValueScale) *
	      yCurveScale[bufNum])+ seql->dataY0;

	  /* left side ticks (could do grids right around here...) */
	    XDrawLine(seql->display,seql->pixmap,seql->gc,
	      seql->dataX0
	      - (int)(RELATIVETICKSIZE*(min(seql->w,seql->h))/2),
	      tickPosition,
	      seql->dataX0
	      + (int)(RELATIVETICKSIZE*(min(seql->w,seql->h))/2),
	      tickPosition);

	  /* right side ticks */
	    XDrawLine(seql->display,seql->pixmap,seql->gc,
	      seql->dataX1
	      - (int)(RELATIVETICKSIZE*(min(seql->w,seql->h))/2),
	      tickPosition,
	      seql->dataX1
	      + (int)(RELATIVETICKSIZE*(min(seql->w,seql->h))/2),
	      tickPosition);

	  /* 
	   * tick labelling (left side only)
	   *   (move label slightly if there's already a string in that area)
	   */

	    sprintf(valueString,"%.3g",yValueScale);
	    textWidth = XTextWidth(seql->axesFontStruct,valueString,
	      strlen(valueString));
	    XSetFont(seql->display,seql->gc,seql->axesFontStruct->fid);

	    stringPosition = getOKStringPosition(tickPosition, usedSpots,
	      &usedCtr, seql->axesFontHeight);

	    XDrawString(seql->display,seql->pixmap,seql->gc,
	      seql->dataX0 - textWidth
	      -(int)(RELATIVETICKSIZE*(min(seql->w,seql->h))),
	      stringPosition,
	      valueString,strlen(valueString));

	  /* put up a little colored circles next to the scale */
	    XSetForeground( seql->display, seql->gc, seql->dataPixel[bufNum]);
	    XFillArc(seql->display,seql->pixmap,seql->gc,
	      seql->dataX0 - textWidth
	      - (int)(RELATIVETICKSIZE*(min(seql->w,seql->h)))
	      - MINBORDER - seql->axesFontHeight/3,
	      stringPosition - seql->axesFontHeight/3,
	      seql->axesFontHeight/3, seql->axesFontHeight/3, 0, 64*360);
	}
    }



  /* 
   * Put up independent variable axis (X) tick marks and labels
   */

    XSetForeground( seql->display, seql->gc, seql->forePixel);

  /*
   * now X axis labelling...
   */
    usedCtr = 0;
    for (i=0; i<MAXTICKS; i++) {
	used[i] = 0;
	numUsed[i]=0;
    }

    for (bufNum = 0; bufNum < seql->nBuffers; bufNum++) {


	for (xValueScale=seql->xRange[i].minVal; xValueScale <=
	       seql->xRange[bufNum].maxVal; xValueScale+=xCurveInterval[bufNum]){

	    XSetForeground( seql->display, seql->gc, seql->forePixel);

	    tickPosition = (int)((xValueScale - seql->xRange[i].minVal)
	      *xCurveScale[bufNum]) + seql->dataX0;


	  /* bottom ticks (could do grids right around here...) */
	    XDrawLine(seql->display,seql->pixmap,seql->gc,
	      tickPosition,
	      seql->dataY1
	      - (int)(RELATIVETICKSIZE*(min(seql->h,seql->w))/2),
	      tickPosition,
	      seql->dataY1
	      + (int)(RELATIVETICKSIZE*(min(seql->h,seql->w))/2));

	  /* top ticks */
	    XDrawLine(seql->display,seql->pixmap,seql->gc,
	      tickPosition,
	      seql->dataY0
	      - (int)(RELATIVETICKSIZE*(min(seql->h,seql->w))/2),
	      tickPosition,
	      seql->dataY0
	      + (int)(RELATIVETICKSIZE*(min(seql->h,seql->w))/2));

	  /*
	   * tick labelling (bottom only)
	   */

	    sprintf(valueString,"%.3g",xValueScale);
	    textWidth = XTextWidth(seql->axesFontStruct,valueString,
	      strlen(valueString));
	    XSetFont(seql->display,seql->gc,seql->axesFontStruct->fid);

	    stringPositionX = tickPosition;
	    stringPositionY = seql->dataY1
	      + (int)(RELATIVETICKSIZE*(min(seql->h,seql->w)))/2
	      + seql->axesFontHeight;
	    getOKStringPositionXY(&stringPositionX, &stringPositionY, used, numUsed, 
	      MAXTICKS, seql->axesFontHeight);

	    XDrawString(seql->display,seql->pixmap,seql->gc,
	      stringPositionX - textWidth/2 + MINBORDER + seql->axesFontHeight/2,
	      stringPositionY,
	      valueString,strlen(valueString));

	  /* put up a little colored rectangle next to the scale */
	    XSetForeground( seql->display, seql->gc, seql->dataPixel[bufNum]);
	    XFillArc(seql->display,seql->pixmap,seql->gc,
	      stringPositionX - textWidth/2,
	      stringPositionY - seql->axesFontHeight/3,
	      seql->axesFontHeight/3, seql->axesFontHeight/3, 0, 64*360);
	}
    }




    XSetForeground( seql->display, seql->gc, seql->forePixel);


  /*
   * put down title
   */
    if (seql->title != NULL) {
	textWidth = XTextWidth(seql->titleFontStruct,seql->title,
	  strlen(seql->title));
	XSetFont(seql->display,seql->gc,seql->titleFontStruct->fid);
	XDrawString(seql->display,seql->pixmap,seql->gc,
	  (seql->w/2 - textWidth/2),
	  (seql->dataY0/2), seql->title,strlen(seql->title));
    }


  /*
   * label axes
   */
    XSetFont(seql->display,seql->gc,seql->axesFontStruct->fid);

  /*
   * put down x axis label
   */
    if (seql->xAxisLabel != NULL) {
	textWidth = XTextWidth(seql->axesFontStruct,seql->xAxisLabel,
	  strlen(seql->xAxisLabel));
	tempPosition = seql->dataX1;
	xPosition = tempPosition + (seql->w - tempPosition)/2 - textWidth/2;
	xPosition = (xPosition+textWidth > seql->w - MINBORDER 
	  ? seql->w - MINBORDER - textWidth : xPosition);
	tempPosition = seql->dataY1;
	yPosition = tempPosition ;
	XDrawString(seql->display,seql->pixmap,seql->gc,
	  xPosition,
	  yPosition,
	  seql->xAxisLabel,strlen(seql->xAxisLabel));
    }

  /*
   * put down y axis label
   */
    if (seql->yAxisLabel != NULL) {
	textWidth = XTextWidth(seql->axesFontStruct,seql->yAxisLabel,
	  strlen(seql->yAxisLabel));
	tempPosition = seql->dataX0;
	xPosition = tempPosition/2 - textWidth/2;
	xPosition = (xPosition < MINBORDER ? MINBORDER : xPosition);
	tempPosition = seql->dataY0;
	yPosition = 4*tempPosition/7  + seql->axesFontHeight/2;
	XDrawString(seql->display,seql->pixmap,seql->gc,
	  xPosition,
	  yPosition,
	  seql->yAxisLabel,strlen(seql->yAxisLabel));
    }


    XCopyArea(seql->display,seql->pixmap,seql->window,seql->gc,0,0,
      seql->w,seql->h,0,0);

    XFlush(seql->display);


}





/*
 * Refresh the display
 * (don't recalcuate, just remap the pixmap onto the window
 */

void seqlRefresh(seql)
    Seql  *seql;
{
    XCopyArea(seql->display,seql->pixmap,seql->window,seql->gc,0,0,
      seql->w,seql->h,0,0);
    XFlush(seql->display);
}




/*
 *   Add a seql plot
 *
 *   input parameters:
 *
 *   seql	 - pointer to Seql data structure
 *   nBuffers    - number of "plots" to be made in same frame
 *   bufferSize  - size of buffers
 *   dataBuffer  - array of array of doubles dataBuffer[nBuffers][bufferSize]
 *   nValues     - array of number of points per buffer (above)
 *   type	 - sequential data plot type (SeqlType): 
 *		   	SeqlPolygon, SeqlPoint, SeqlLine
 *   title	 - character string (ptr) for title
 *   titleFont	 - charaacter string (ptr) for font to use for title
 *   xAxisLabel	 - character string (ptr) for X axis label
 *   yAxisLabel	 - character string (ptr) for Y axis label
 *   axesFont	 - character string (ptr) for font to use for axes labels
 *   foreColor	 - character string (ptr) for color to use for foreground
 *   backColor	 - character string (ptr) for color to use for background
 *   dataColor	 - array of character strings (ptr) for color to use for data
 *			if NULL, then do own color allocation via color cell
 *   storageMode -  sequential data plot data storage mode: 
 *		      SeqlInternal, SeqlExternal
 *			if SeqlInternal, make a copy of the data; 
 *			otherwise (SeqlExternal), just copy the pointers
 */

void seqlSet( seql, nBuffers, bufferSize, dataBuffer, nValues, type, 
  title, titleFont, 
  xAxisLabel, yAxisLabel,
  axesFont, foreColor, backColor, dataColor, storageMode)
    Seql		*seql;
    int		 nBuffers;
    int		 bufferSize;
    double	*dataBuffer[];
    int           *nValues;
    SeqlType	 type;
    char		*title, *titleFont, *xAxisLabel, *yAxisLabel, *axesFont;
    char		*foreColor, *backColor, *dataColor[];
    SeqlStorageMode storageMode;
{
    int i, k, stringLength = 0;
    XColor color, ignore;
    double r, g, b;
    int fontCount;
    char **fontNames;
    XFontStruct *fontInfo;



    seql->nBuffers = nBuffers;
    seql->bufferSize = bufferSize;
    seql->type = type;

    if (storageMode == SeqlExternal) {
	seql->storageMode = SeqlExternal;
	seql->dataBuffer    = dataBuffer;
	seql->titleFont = titleFont;
	seql->axesFont = axesFont;
	seql->title = title;
	seql->xAxisLabel = xAxisLabel;
	seql->yAxisLabel = yAxisLabel;
	seql->nValues = nValues;
    } else {
	seql->storageMode = SeqlInternal;
	if (seql->dataBuffer) {
	    for (i = 0; i < nBuffers; i++)
	      free( (char *) seql->dataBuffer[i] );
	    free( (char *) seql->dataBuffer );

	}
	seql->dataBuffer = (double **) malloc((unsigned)
	  (nBuffers*sizeof(double)));
	for (i = 0; i < nBuffers; i++) {
	    seql->dataBuffer[i] = (double *) 
	      malloc((unsigned) bufferSize*sizeof(double));
	    memcpy((void *)seql->dataBuffer[i],(void *)dataBuffer[i],
	      (size_t)(bufferSize*sizeof(double)) );
	}

	if (seql->titleFont) free( (char *) seql->titleFont);
	if (titleFont != NULL) {
	    stringLength = strlen(titleFont) + 1;
	    seql->titleFont = (char *) malloc(stringLength);
	    memcpy((void *)seql->titleFont,(void *)titleFont,(size_t)stringLength);
	    seql->titleFont[stringLength-1] = '\0';
	}

	if (seql->axesFont) free( (char *) seql->axesFont);
	if (axesFont != NULL) {
	    stringLength = strlen(axesFont) + 1;
	    seql->axesFont = (char *) malloc(stringLength);
	    memcpy((void *)seql->axesFont,(void *)axesFont,(size_t)stringLength);
	    seql->axesFont[stringLength-1] = '\0';
	}

	if (seql->title) free( (char *) seql->title);
	if (title != NULL) {
	    stringLength = strlen(title) + 1;
	    seql->title = (char *) malloc(stringLength);
	    memcpy((void *)seql->title,(void *)title,(size_t)stringLength);
	    seql->title[stringLength-1] = '\0';
	}

	if (seql->xAxisLabel) free( (char *) seql->xAxisLabel);
	if (xAxisLabel != NULL) {
	    stringLength = strlen(xAxisLabel) + 1;
	    seql->xAxisLabel = (char *) malloc(stringLength);
	    memcpy((void *)seql->xAxisLabel,(void *)xAxisLabel,(size_t)stringLength);
	    seql->xAxisLabel[stringLength-1] = '\0';
	}

	if (seql->yAxisLabel) free( (char *) seql->yAxisLabel);
	if (yAxisLabel != NULL) {
	    stringLength = strlen(yAxisLabel) + 1;
	    seql->yAxisLabel = (char *) malloc(stringLength);
	    memcpy((void *)seql->yAxisLabel,(void *)yAxisLabel,(size_t)stringLength);
	    seql->yAxisLabel[stringLength-1] = '\0';
	}

	if (seql->nValues) free( (char *) seql->nValues);
	seql->nValues = (int *) malloc(nBuffers*sizeof(int *));
	for (i = 0; i < nBuffers; i++) seql->nValues[i] = nValues[i];
    }


    if (seql->dataPixel) free( (char *) seql->dataPixel);
    seql->dataPixel = (unsigned long *) malloc((unsigned)
      ((nBuffers)*sizeof(unsigned long)));


    if (seql->devicePoints) free( (char *) seql->devicePoints );
  /* devicePoints = nPts + 2 since for filled polygon need x0,y0 and x1,y0 also */
    seql->devicePoints = (XPoint *) malloc((unsigned)
      ((bufferSize+2)*sizeof(XPoint)));


  /* set foreground, background and data colors based on passed-in strings */

  /* use a color for the foreground if possible */
    if ((DisplayCells(seql->display,seql->screen) > 2) && (!(seql->setOwnColors))) {
	if(!XAllocNamedColor(seql->display,
	  DefaultColormap(seql->display,seql->screen),
	  foreColor,&color,&ignore)){
	    fprintf(stderr,"\nseqlSet:  couldn't allocate color %s",foreColor);
	    seql->forePixel  = WhitePixel(seql->display,seql->screen);
	} else {
	    seql->forePixel = color.pixel;
	}

	if(!XAllocNamedColor(seql->display,
	  DefaultColormap(seql->display,seql->screen),
	  backColor,&color,&ignore)) {
	    fprintf(stderr,"\nseqlSet:  couldn't allocate color %s",backColor);
	    seql->backPixel  = BlackPixel(seql->display,seql->screen);
	} else {
	    seql->backPixel = color.pixel;
	}

      /* if dataColor pointer is NULL, then do own color allocation */
	if (dataColor == NULL) {
	    if (XAllocColorCells(seql->display, 
	      DefaultColormap(seql->display,seql->screen), FALSE,
	      NULL, 0, seql->dataPixel,nBuffers)) {

		for (k = 0; k < nBuffers; k++) {
		  /* (MDA) - rainbow( (double)(k+1)/(double)nBuffers, 1.0, 1.0, &r, &g, &b); */
		    r = ((double) rand())/pow(2.0,31.0);
		    g = ((double) rand())/pow(2.0,31.0);
		    b = ((double) rand())/pow(2.0,31.0);
		    r *= 65535; g *= 65535; b *= 65535;
		    color.red = (unsigned int) r;
		    color.green = (unsigned int) g;
		    color.blue = (unsigned int) b;
		    color.pixel = seql->dataPixel[k];
		    color.flags = DoRed | DoGreen | DoBlue;
		    XStoreColor(seql->display,
		      DefaultColormap(seql->display,seql->screen), &color);
		}

	    } else {
		fprintf(stderr,"\nseqlSet:  couldn't allocate color cells");
	    }
	} else {
	    for (i = 0; i < nBuffers; i++) {
		if(!XAllocNamedColor(seql->display,
		  DefaultColormap(seql->display,seql->screen),
		  dataColor[i],&color,&ignore)) {
		    fprintf(stderr,"\nseqlSet:  couldn't allocate color %s",dataColor);
		    seql->dataPixel[i]  = WhitePixel(seql->display,seql->screen);
		} else {
		    seql->dataPixel[i] = color.pixel;
		}
	    }
	}


    } else {
	seql->forePixel  = WhitePixel(seql->display,seql->screen);
	seql->backPixel  = BlackPixel(seql->display,seql->screen);
	for (i = 0; i < nBuffers; i++)
	  seql->dataPixel[i]  = WhitePixel(seql->display,seql->screen);
    }


  /*
   * load the fonts
   */
    if (!seql->setOwnFonts) {
	if ((seql->titleFontStruct=XLoadQueryFont(seql->display,seql->titleFont)) 
	  == NULL) {
	    fprintf(stderr,"display %s doesn't know font %s\n",
	      DisplayString(seql->display),seql->titleFont);
	    fprintf(stderr,"  using first available font\n");
	    fontNames = XListFontsWithInfo(seql->display,FONTWILDCARD,
	      MAXFONTS,&fontCount, &fontInfo);
	    seql->titleFontStruct=XLoadQueryFont(seql->display,fontNames[0]);
	    XFreeFontInfo(fontNames,fontInfo,fontCount);
	    XFreeFontNames(fontNames);
	}

	seql->titleFontHeight = seql->titleFontStruct->ascent + 
	  seql->titleFontStruct->descent;
	seql->titleFontWidth = seql->titleFontStruct->max_bounds.rbearing - 
	  seql->titleFontStruct->min_bounds.lbearing;

	if ((seql->axesFontStruct=XLoadQueryFont(seql->display,seql->axesFont)) 
	  == NULL) {
	    fprintf(stderr,"display %s doesn't know font %s\n",
	      DisplayString(seql->display),seql->axesFont);
	    fprintf(stderr,"  using first available font\n");
	    seql->axesFontStruct = seql->titleFontStruct;
	}

	seql->axesFontHeight = seql->axesFontStruct->ascent + 
	  seql->axesFontStruct->descent;
	seql->axesFontWidth = seql->axesFontStruct->max_bounds.rbearing - 
	  seql->axesFontStruct->min_bounds.lbearing;


    }

}






/*
 *   Set the  axis display range for a single curve of a Seql plot
 *
 *   input parameters:
 *
 *   seql        - pointer to Seql data structure
 *   curveNum    - curve or data set number
 *   axis        - character specifying X or Y axis
 *   minVal      - minimum display value
 *   maxVal      - maximum display value
 */

#ifdef _NO_PROTO
void seqlSetRange(seql,curveNum,axis,minVal,maxVal)
    Seql *seql;
    int  curveNum;
    char axis;
    double minVal,maxVal;
#else
    void seqlSetRange(Seql *seql, int curveNum, char axis,
      double minVal, double maxVal)
#endif
{
    if (curveNum < 0 || curveNum >= seql->nBuffers) {
	fprintf(stderr,
	  "\nseqlSetRange: illegal curve (number %d) specified\n",
	  curveNum);
	return;
    }


    if (axis == 'X' || axis == 'x') {
	if ( (minVal >= maxVal) || (minVal < 0.0) ||
	  (maxVal > (double) (seql->bufferSize)) ) {
	    seql->xRangeType[curveNum] = SeqlDefaultRange;
	    fprintf(stderr,
	      "\nseqlSetRange: improper range specified, using defaults\n");
	    return;
	}
	seql->xRangeType[curveNum] = SeqlSpecRange;
	seql->xRange[curveNum].minVal = minVal;
	seql->xRange[curveNum].maxVal = maxVal;
    } else {
	if (axis == 'Y' || axis == 'y') {
	    if (minVal >=  maxVal) {
		seql->yRangeType[curveNum] = SeqlDefaultRange;
		fprintf(stderr,
		  "\nseqlSetRange: improper range specified, using defaults\n");
		return;
	    }
	    seql->yRangeType[curveNum] = SeqlSpecRange;
	    seql->yRange[curveNum].minVal = minVal;
	    seql->yRange[curveNum].maxVal = maxVal;
	}
    }
}



/*
 *   Set the axis display range for a single curve of a Seql plot to DEFAULT
 *
 *   input parameters:
 *
 *   seql        - pointer to Seql data structure
 *   curveNum    - curve or data set number
 *   axis        - axis (X or Y)
 */
#ifdef _NO_PROTO
void seqlSetRangeDefault(seql,curveNum,axis)
    Seql *seql;
    int  curveNum;
    char axis;
#else
    void seqlSetRangeDefault(Seql *seql, int curveNum, char axis)
#endif
{
    if (curveNum < 0 || curveNum >= seql->nBuffers) {
	fprintf(stderr,
	  "\nseqlSetRangeDefault: illegal curve (number %d) specified\n",
	  curveNum);
	return;
    }
    if (axis == 'X' || axis == 'x') {
	seql->xRangeType[curveNum] = SeqlDefaultRange;
    } else if (axis == 'Y' || axis == 'y') {
	seql->yRangeType[curveNum] = SeqlDefaultRange;
    }

}



/*
 *   Set the display type of a Seql plot 
 *
 *   input parameters:
 *
 *   seql	 - pointer to Seql data structure
 *   type	 - display type
 */
void seqlSetDisplayType(seql,type)
    Seql *seql;
    SeqlType type;
{
    if (type == SeqlPolygon) {
	seql->type = SeqlPolygon;
    } else if (type == SeqlLine) {
	seql->type = SeqlLine;
    } else if (type == SeqlPoint)  {
	seql->type = SeqlPoint;
    } else {
	fprintf(stderr,"\nseqlSetDisplayType: invalid display type\n");
    }

}




/*
 *   Print (dump in xwd format, then pipe to `xpr -device ps' then pipe
 *   to printer defined by PSPRINTER environment variable...)
 *
 *   input parameters:
 *
 *   seql	 - pointer to Seql data structure
 */
void seqlPrint(seql)
    Seql *seql;
{

    utilPrint(seql->display, seql->window, XWD_TEMP_FILE_STRING);

}




/*
 *   Set the legend parameters
 *
 *   input parameters:
 *
 *   seql	 - pointer to Seql data structure
 *   legendTitle - title string	
 *   legendArray - pointer to array of legend entry strings
 */
void seqlSetLegend(seql, legendTitle, legendArray)
    Seql *seql;
    char *legendTitle;
    char **legendArray;
{
    int i, legendWidth = 0, legendHeight= 0;
    XWindowAttributes parentWindowAttributes;
    XSetWindowAttributes windowAttributes;


    if (seql->legend) XDestroyWindow(seql->display,seql->legendWindow);

    seql->legend = TRUE;

    if (seql->legendTitle != NULL) free( (char *) seql->legendTitle);
    seql->legendTitle = malloc(strlen(legendTitle)+1);
    strcpy(seql->legendTitle,legendTitle);

    if (seql->legendArray != NULL) {
	for (i=0; i<seql->nBuffers; i++) free( (char *) seql->legendArray[i]);
	free( (char *) seql->legendArray);
    }
    seql->legendArray = (char **) malloc(seql->nBuffers*sizeof(char *));
    for (i=0; i<seql->nBuffers; i++){
	seql->legendArray[i] = malloc(strlen(legendArray[i])+1);
	strcpy(seql->legendArray[i],legendArray[i]);
    }


  /*
   * loop to determine required size of legend window
   */
    legendWidth = XTextWidth(seql->titleFontStruct,legendTitle,
      strlen(legendTitle)) + 2*MINBORDER;
    for (i=0; i<seql->nBuffers; i++) {
	legendWidth = max(legendWidth, 
	  XTextWidth(seql->axesFontStruct,legendArray[i],
	    strlen(legendArray[i]))+seql->axesFontHeight+
	  4*MINBORDER);

    }

    legendHeight = 2*MINBORDER + seql->nBuffers*MINBORDER + MINBORDER +
      seql->titleFontHeight + 
      seql->nBuffers*seql->axesFontHeight;

  /*
   * create a window butting up against seql window.
   * legendWindow is created as child of seql window.
   */
    seql->legendWindow = XCreateSimpleWindow(seql->display,
      seql->window,
      seql->w-legendWidth
      -LEGENDOFFSET-(2*BORDERLINEWIDTH),
      LEGENDOFFSET,
      legendWidth, legendHeight,
      BORDERLINEWIDTH,
      seql->forePixel, seql->backPixel);


  /*
   * Set backing store attribute from parent window
   *  (Xt and XView differ on the default)
   */
    XGetWindowAttributes(seql->display, seql->window, &parentWindowAttributes);
    windowAttributes.backing_store = parentWindowAttributes.backing_store;
    XChangeWindowAttributes(seql->display, seql->legendWindow,
      CWBackingStore, &windowAttributes);

    XMapRaised(seql->display,seql->legendWindow);

			
}


/*
 *   Draw the legend
 *
 *   input parameters:
 *
 *   seql	 - pointer to Seql data structure
 */
void seqlDrawLegend(seql)
    Seql *seql;
{
    int i;

    if (!seql->legend) return;

  /* title */
    XSetFont(seql->display,seql->gc,seql->titleFontStruct->fid);
    XSetForeground( seql->display, seql->gc, seql->forePixel);
    XDrawString(seql->display,seql->legendWindow,seql->gc,
      MINBORDER, seql->titleFontHeight + MINBORDER, 
      seql->legendTitle,strlen(seql->legendTitle));

  /* legend color-key rectangles */
  /* (use axesFontHeight as both Cell width and height */
    XSetFont(seql->display,seql->gc,seql->axesFontStruct->fid);
    for (i=0; i<seql->nBuffers; i++) {
	XSetForeground( seql->display, seql->gc, seql->dataPixel[i]);
	XFillRectangle( seql->display, seql->legendWindow, seql->gc, MINBORDER,
	  3*MINBORDER + seql->titleFontHeight + 
	  i*(seql->axesFontHeight+MINBORDER),
	  seql->axesFontHeight, seql->axesFontHeight);
    }

  /* legend strings */
    XSetForeground( seql->display, seql->gc, seql->forePixel);
    for (i=0; i<seql->nBuffers; i++) {
	XDrawString(seql->display,seql->legendWindow,seql->gc,
	  seql->axesFontHeight + 2*MINBORDER,
	  3*MINBORDER + seql->titleFontHeight + 2*seql->axesFontHeight/3 +
	  i*(seql->axesFontHeight+MINBORDER),
	  seql->legendArray[i],strlen(seql->legendArray[i]));
    }

    XRaiseWindow(seql->display,seql->legendWindow);

}
