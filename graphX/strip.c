/***
 ***   STRIP CHART Plot routines & data structures
 ***
 ***	(MDA) 11 May 1990
 ***
 ***/

#include <X11/Xlib.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include "Strip.h"

#ifndef MAX
# define MAX(a,b)  ((a)>(b)?(a):(b))
#endif

#define MINBORDER 3		/* minimum distance from window edge (Pixels) */

#define RELATIVETICKSIZE 0.02	/* size of tick mark relative to window */
#define NTICKS 2		/* `preferred' number of ticks for axes */
#define MAXTICKS NTICKS+4	/* `maximum' number of tick based on NTICKS */

#define DATALINEWIDTH 1		/* line width for data lines (Pixels) */
#define BORDERLINEWIDTH 1	/* line width for lines (Pixels) */
#define ALMOSTZERO 1.0e-07	/* close enough to zero */

#define MAXCHANNELS 100		/* largest number of channels to sample */

#define LEGENDOFFSET 1          /* amount of legend window to overlap parent */


/*
 * modified Strip Chart code to subtract out execution time of update/draw
 *   from user-specified interval.  This more accurately defines sampling
 *   intervals
 */
#include <sys/times.h>
#include <unistd.h>


/*
 * define constants for XListFontsWithInfo call
 */
#define FONTWILDCARD "*"
#define MAXFONTS 1


extern int getOKStringPosition(int, int *, int *, int);
extern int getOKStringPositionXY(int *, int *, int*, int*, int, int);




/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * here's the timeout routine (updates)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

static void updateStrip(strip,timeoutId)
  Strip *strip;
  XtIntervalId *timeoutId;
{
  int i;
  double currentValue;

  static time_t  startTime = 0;
  static time_t  endTime = 0;
  static struct tms  tms;

  static BOOLEAN takeTimes = TRUE; 

  if (strip->paused)
	return;


/* we want the time spent in user and system space */
  if ((takeTimes) && (startTime == 0)) {
     times(&tms); 
     startTime = tms.tms_utime + tms.tms_stime;
  }


  for (i = 0; i < strip->nChannels; i++) {

/* pass the channel number just in case the same function is registered
   for all channels, and the user wants to just do a switch on channel # */

     currentValue = (*(strip->getValue[i]))(i,strip->userData);

     if (strip->nValues[i] < strip->bufferSize) {
        strip->dataBuffer[i][strip->bufferPtr[i] + strip->nValues[i]] 
		= currentValue;
        strip->nValues[i]++;
     } else {
        strip->bufferPtr[i]++;
        strip->bufferPtr[i] %= strip->bufferSize;
/* (MDA) fix for the "not resetting 1st point in buffer" problem */
        strip->dataBuffer[i][(strip->bufferPtr[i]+strip->bufferSize-1) 
			% strip->bufferSize ] = currentValue; 
     }
  }

  stripDraw(strip);

/*
 * and reregister the timeout 
 *
 */

/*
 *(add some machine performance dependent stuff
 *   use an effective update interval to subtract out client times and
 *   more accurately sample at the specified frequency/interval
 */

  if ((takeTimes) && (startTime != 0)) {
     times(&tms); 
     endTime = tms.tms_utime + tms.tms_stime;
     strip->adjustedInterval = (double) (endTime - startTime);
     strip->adjustedInterval /= sysconf(_SC_CLK_TCK);
     startTime = 0;
     takeTimes = FALSE;
  }


/*
 * using Xt timeout facility: this function wants milliseconds
 */
if (strip->appContext == NULL) {
  strip->timeoutId = XtAddTimeOut( 
       max(0,(int)(1000.0*(strip->samplingInterval - strip->adjustedInterval))),
       (XtTimerCallbackProc)updateStrip,strip);
} else {
  strip->timeoutId = XtAppAddTimeOut( strip->appContext,
       max(0,(int)(1000.0*(strip->samplingInterval - strip->adjustedInterval))),
       (XtTimerCallbackProc)updateStrip,strip);
}


}




/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Initialize the Strip
 * function:  Strip *stripInit()
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */ 

Strip *stripInit( display, screen, window)
  Display *display;
  int     screen;
  Window  window;
{
Strip *strip;
XWindowAttributes windowAttributes;
int i, w, h;

strip = (Strip *) malloc( sizeof( Strip ) );
if (!strip) return(NULL);

XGetWindowAttributes(display, window, &windowAttributes);
w = windowAttributes.width;
h = windowAttributes.height;

/* 
 * initialize some useful fields
 */
strip->display = display;
strip->screen = screen;
strip->window = window;
strip->axesAlreadyDrawn = False;
strip->clipRectanglesSet = False;

/* 
 * description of extent of strip in Pixel coordinates...
 */
strip->w      = w;
strip->h      = h;

strip->dataX0 =  STRIP_MARGIN*w;
strip->dataY0 =  STRIP_MARGIN*h;
strip->dataX1 =  (1.0 - STRIP_MARGIN)*w;
strip->dataY1 =  (1.0 - STRIP_MARGIN)*h;
strip->dataWidth = strip->dataX1 - strip->dataX0;
strip->dataHeight = strip->dataY1 - strip->dataY0;


/*
 * create the pixmaps
 */
strip->pixmap = XCreatePixmap(strip->display,strip->window,strip->w,strip->h,
			DefaultDepth(strip->display,strip->screen));

/* 
 * create a default, writable GC
 */
strip->gc = XCreateGC(strip->display,DefaultRootWindow(strip->display),
			NULL,NULL);

/*
 * and clear the empty fields
 */

strip->appContext = (XtAppContext) NULL; 
		/* use default AppContext unless user specifies one */
strip->dataBuffer = NULL;
strip->pointArray = NULL;
strip->titleFont = NULL;
strip->axesFont = NULL;
strip->title = NULL;
strip->xAxisLabel = NULL;
strip->yAxisLabel = NULL;
strip->bufferPtr = NULL;
strip->nValues = NULL;
strip->dataPixel = NULL;
strip->getValue = NULL;
strip->timeoutId = NULL;

for (i=0; i<MAXCURVES; i++) {
  strip->valueRange[i].minVal = 0.0;
  strip->valueRange[i].maxVal = 10.0;
}


/*
 * and initialize the Legend fields
 */
strip->legend = FALSE;
strip->legendTitle = NULL;
strip->legendArray = NULL;

strip->setOwnColors = FALSE;    /* will user fill in own pixel values? */
strip->setOwnFonts = FALSE;	/* will user fill in own font values? */
strip->shadowThickness = 0;	/* size of any shadows to not draw in */

return(strip);

}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Set the application context to use for the Strip
 * function:  void stripSetAppContext()
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */ 

void stripSetAppContext(strip,appContext)
  Strip *strip;
  XtAppContext appContext;
{

/*
 * update the  application context  field
 */
  strip->appContext = appContext;
  if (strip->timeoutId != NULL) XtRemoveTimeOut(strip->timeoutId);
  strip->timeoutId = XtAppAddTimeOut( strip->appContext,
       max(0,(int)(1000.0*(strip->samplingInterval - strip->adjustedInterval))),
       (XtTimerCallbackProc)updateStrip,strip);

}



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Set/Reset the Interval (Timeout) for the Strip
 * function:  void stripSetInterval()
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */ 

void stripSetInterval(strip,newInterval)
  Strip *strip;
  double newInterval;
{
/*
 * update the sampling Interval field
 */
  strip->samplingInterval  = newInterval;

/*
 * register the timeout to reflect the current sampling interval
 */
  if (strip->timeoutId != NULL) XtRemoveTimeOut(strip->timeoutId);
 
  if (strip->appContext == NULL) {
     strip->timeoutId = XtAddTimeOut( 
       max(0,(int)(1000.0*(strip->samplingInterval - strip->adjustedInterval))),
       (XtTimerCallbackProc)updateStrip,strip);
  } else {
     strip->timeoutId = XtAppAddTimeOut( strip->appContext,
       max(0,(int)(1000.0*(strip->samplingInterval - strip->adjustedInterval))),
       (XtTimerCallbackProc)updateStrip,strip);
  }

}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Set/Reset the data color for the Strip
 * function:  void stripSetDataColor()
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */ 

void stripSetDataColor(strip,channelNumber,colorString)
  Strip *strip;
  int channelNumber;
  char *colorString;
{
  XColor color, ignore;

    if (channelNumber > strip->nChannels-1 || channelNumber < 0) {
       fprintf(stderr,"\nstripSetDataColor:  invalid channel number %d", 
		channelNumber);
       return;
    }

    if(!XAllocNamedColor(strip->display,
        DefaultColormap(strip->display,strip->screen),
        colorString,&color,&ignore)) {
        fprintf(stderr, "\nstripSetDataColor:  couldn't allocate color %s",
		colorString);
        strip->dataPixel[channelNumber]  = WhitePixel(strip->display,
		strip->screen);
    } else {
        strip->dataPixel[channelNumber] = color.pixel;
    }

}


/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Terminate the Strip
 * function:  void stripTerm()
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */ 

void stripTerm(strip)
  Strip *strip;
{
int i;
/***
 *** free the space used and referenced by the strip chart
 ***/

/*
 * free the pixmaps
 */
  XFreePixmap(strip->display,strip->pixmap);

/* likewise, free the graphics context and font structures */
  XFreeGC(strip->display,strip->gc);
  if (!strip->setOwnFonts) {
    XFreeFont(strip->display,strip->titleFontStruct);
    XFreeFont(strip->display,strip->axesFontStruct);
  }

/* and finally free the strip chart space itself */

  if (strip->storageMode == StripInternal) {
      for (i=0; i<strip->nChannels; i++) {
         free( (char *) strip->dataBuffer[i]);
         free( (char *) strip->pointArray[i]);
      }
      free( (char *) strip->dataBuffer);
      free( (char *) strip->pointArray);
      free( (char *) strip->titleFont);
      free( (char *) strip->axesFont);
      free( (char *) strip->title);
      free( (char *) strip->xAxisLabel);
      free( (char *) strip->yAxisLabel);
  }

  free( (char *) strip->bufferPtr);
  free( (char *) strip->nValues);
  free( (char *) strip->dataPixel);
  free( (char *) strip->getValue);

/* free the legend space */
  if (strip->legend) {
    free( (char *) strip->legendTitle);
    for (i=0; i<strip->nChannels; i++)
       free( (char *) strip->legendArray[i]);
    free( (char *) strip->legendArray);
    XDestroyWindow(strip->display, strip->legendWindow);
    strip->legend = FALSE;

  }


  free( (char *) strip);

}









/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Resize the Strip (e.g., change sizes: free old pixmaps, create new ones)
 * function: void stripResize()
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */ 

void stripResize(strip)
  Strip  *strip;
{
XWindowAttributes windowAttributes;

int i, legendWidth = 0, legendHeight= 0;
XWindowAttributes parentWindowAttributes;
XSetWindowAttributes setWindowAttributes;
int rootX, rootY;
Window child;

/*
 * check to see if window has changed size
 *   if so, then Free the pixmaps, fetch new ones, and continue
 */
 XGetWindowAttributes(strip->display, strip->window, &windowAttributes);

 if (strip->w != windowAttributes.width  ||
     strip->h != windowAttributes.height){

     XSetForeground(strip->display, strip->gc, strip->backPixel);
     strip->w = windowAttributes.width;
     strip->h = windowAttributes.height;
     strip->dataX0 = STRIP_MARGIN*windowAttributes.width;
     strip->dataY0 = STRIP_MARGIN*windowAttributes.height;
     strip->dataX1 = (1.0 - STRIP_MARGIN)*windowAttributes.width;
     strip->dataY1 = (1.0 - STRIP_MARGIN)*windowAttributes.height;
     strip->dataWidth = strip->dataX1 - strip->dataX0;
     strip->dataHeight = strip->dataY1 - strip->dataY0;

     if  ( strip->pixmap ) XFreePixmap(strip->display,strip->pixmap);
     strip->pixmap = XCreatePixmap(strip->display,strip->window,
                strip->w,strip->h,
                DefaultDepth(strip->display,strip->screen));
     strip->axesAlreadyDrawn = False;
     strip->clipRectanglesSet = False;


      /* Relocate legend window, if it exists */
     if (strip->legend) {
	XDestroyWindow(strip->display,strip->legendWindow);

      /*
       * loop to determine required size of legend window
       */
	legendWidth = XTextWidth(strip->titleFontStruct,strip->legendTitle,
			strlen(strip->legendTitle)) + 2*MINBORDER;
	for (i=0; i<strip->nChannels; i++) {
	    legendWidth = max(legendWidth,
			XTextWidth(strip->axesFontStruct,strip->legendArray[i],
			strlen(strip->legendArray[i]))+strip->axesFontHeight+
			4*MINBORDER);
	}
	legendHeight = 2*MINBORDER + strip->nChannels*MINBORDER + MINBORDER +
			strip->titleFontHeight +
			strip->nChannels*strip->axesFontHeight;
      /*
       * create a window butting up against graph window;
       * legendWindow is created as child of graph wndow.
       */
	strip->legendWindow = XCreateSimpleWindow(strip->display,
				strip->window,
				strip->w-legendWidth
                                        -LEGENDOFFSET-(2*BORDERLINEWIDTH),
				LEGENDOFFSET,
				legendWidth, legendHeight,
				BORDERLINEWIDTH,
				strip->forePixel, strip->backPixel);
      /*
       * Set backing store attribute from parent window
       */
	XGetWindowAttributes(strip->display, strip->window,
				&parentWindowAttributes);
	setWindowAttributes.backing_store = 
				parentWindowAttributes.backing_store;
	XChangeWindowAttributes(strip->display, strip->legendWindow,
				CWBackingStore, &setWindowAttributes);
	XMapRaised(strip->display,strip->legendWindow);
      }
 }
}







/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * Draw the Strip
 * function: void stripDraw()
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */ 

void stripDraw(strip)
  Strip  *strip;
{

int i, ii, j, textWidth, maxTextWidth;
double timeScale, valueScale[MAXCURVES];
int timeScaledValue, valueScaledValue, ww, hh;
double binWidth = ((STRIP_DRAWINGAREA*strip->w)/((double)(strip->bufferSize)));
double timeValueScale, valueValueScale, timeInterval, valueInterval[MAXCURVES];
double valueMin[MAXCURVES], valueMax[MAXCURVES], timeMin, timeMax;
int tickPosition, xPosition, yPosition, tempPosition;
int goodNumberOfTicks;
char valueString[16];

XRectangle clipRect[2];
int buf, newBuf, usedSpots[(NTICKS+1)*MAXCURVES], usedCtr;
int stringPosition, stringPositionX, stringPositionY, blobPositionY;
int used[MAXTICKS], numUsed[MAXTICKS];
Boolean displayTick;


 if (strip->axesAlreadyDrawn && !strip->clipRectanglesSet) {
    clipRect[0].x = strip->dataX0+1;
    clipRect[0].y = strip->dataY0+1;
    clipRect[0].width = strip->dataWidth-1;
    clipRect[0].height = strip->dataHeight-1;
    XSetClipRectangles(strip->display,strip->gc,0,0,clipRect,1,YXBanded);
    strip->clipRectanglesSet = True;
 }
 XSetForeground(strip->display, strip->gc, strip->backPixel);
 XFillRectangle( strip->display, strip->pixmap, strip->gc,
		0, 0, strip->w, strip->h);

/* 
 * calculate Pixel to value scaling
 */

/* 
 * perform linear scaling (get nice values) for Time and Value axes
 */

 for (i = 0; i < strip->nChannels; i++) {
   linear_scale(strip->valueRange[i].minVal,strip->valueRange[i].maxVal,NTICKS,
		&valueMin[i], &valueMax[i], &valueInterval[i]);
 }

 if (strip->samplingInterval != 0.0) {
    linear_scale(-1.0*strip->samplingInterval * strip->bufferSize, 
		0.0, NTICKS,
		&timeMin, &timeMax, &timeInterval);
 } else {
    timeMin = 0.0;
    timeMax = (double) strip->bufferSize;
    timeInterval = (double) (strip->bufferSize / NTICKS);
 }


/*
 * now put up the data 
 */
  if (!strip->axesAlreadyDrawn) {	/* only set this if necessary */
    XSetLineAttributes( strip->display, strip->gc,
			DATALINEWIDTH,LineSolid,CapButt,JoinMiter);
  }
  timeScale = (STRIP_DRAWINGAREA * strip->w) / (timeMax - timeMin);

  for (i = 0; i < strip->nChannels; i++)
    valueScale[i] = (STRIP_DRAWINGAREA * strip->h)/(valueMax[i] - valueMin[i]);


/* 
 *  strategy: use XCopyArea only for double buffering
 */

  for (i = 0; i < strip->nChannels; i++)
     if (strip->nValues[i] < 2)
	return;			/* bail out if no points to plot */


  ww = strip->w*(1.0-STRIP_MARGIN);
  hh = strip->h*STRIP_MARGIN;

  for (j = 0; j < strip->nChannels; j++) {

    for (i = strip->nValues[j]-1; i >= 0;  i--) {

       ii = (strip->bufferPtr[j] + i) % strip->bufferSize;
    /* timeScaledValue is more sensible than valueScaledValue, 
	it is # of Pixels */
       timeScaledValue = (int) (-1 * (((strip->nValues[j]-i-1)*binWidth)));
    /* valueScaledValue is sort of inverse since upper left is (0,0), */
       valueScaledValue = (int) ((valueMax[j] - strip->dataBuffer[j][ii])
		*valueScale[j]);
       strip->pointArray[j][i].x = ww + timeScaledValue;
       strip->pointArray[j][i].y = hh + valueScaledValue;
    }
  }

  /* draw lines*/
  for (j = 0; j < strip->nChannels; j++){
      XSetForeground( strip->display, strip->gc, strip->dataPixel[j]);
      XDrawLines( strip->display, strip->pixmap, strip->gc,
		strip->pointArray[j], strip->nValues[j],CoordModeOrigin);
  }




if (!strip->axesAlreadyDrawn) {

  XSetClipMask(strip->display,strip->gc,None);
  strip->clipRectanglesSet = False;
  XSetLineAttributes( strip->display, strip->gc,
			BORDERLINEWIDTH,LineSolid,CapButt,JoinMiter);
/* 
 * Put up Value axis and tick marks
 */

 XSetForeground( strip->display, strip->gc, strip->forePixel);

/* left side axis */
 XDrawLine(strip->display,strip->pixmap,strip->gc,
		strip->dataX0, strip->dataY0, strip->dataX0, strip->dataY1);
/* right side axis */
 XDrawLine(strip->display,strip->pixmap,strip->gc,
		strip->dataX1, strip->dataY0, strip->dataX1, strip->dataY1);


 usedCtr = 0;
 for (i=0; i<MAXTICKS; i++) {
  used[i] = 0; numUsed[i] = 0;
 }

/* calculate maxTextWidth for blob positioning */
 maxTextWidth = 0;
 for (buf = 0; buf < strip->nChannels; buf++) {
  for (valueValueScale=valueMin[buf]; valueValueScale<=valueMax[buf]; 
		valueValueScale+=valueInterval[buf]){
      sprintf(valueString,"%.3g",valueValueScale);
      maxTextWidth = MAX(maxTextWidth,
	XTextWidth(strip->axesFontStruct,valueString, strlen(valueString)));
  }
 }


 for (buf = 0; buf < strip->nChannels; buf++) {

/* don't put up multiple ticks or labels if sharing min/max */
  displayTick = True;
  for (newBuf = 0; newBuf < buf; newBuf++)
     if (valueMin[buf] == valueMin[newBuf] && valueMax[buf] == valueMax[newBuf])
	displayTick = False;

  for (valueValueScale=valueMin[buf]; valueValueScale<=valueMax[buf]; 
		valueValueScale+=valueInterval[buf]){

    tickPosition = (int)((valueMax[buf]-valueValueScale)*valueScale[buf])
		+ strip->dataY0;
    stringPosition = getOKStringPosition(tickPosition,usedSpots,&usedCtr,
			strip->axesFontHeight);
    blobPositionY = tickPosition;

    if (displayTick) {

      blobPositionY = stringPosition;
      XSetForeground(strip->display, strip->gc, strip->forePixel);

  /* left side ticks (could do grids right around here...) */
      XDrawLine(strip->display,strip->pixmap,strip->gc,
		strip->dataX0 
			- (int)(RELATIVETICKSIZE*(min(strip->w,strip->h))),
		tickPosition, strip->dataX0, tickPosition);

  /* right side ticks */
      XDrawLine(strip->display,strip->pixmap,strip->gc,
		strip->dataX1, tickPosition, strip->dataX1 
		    + (int)(RELATIVETICKSIZE*(min(strip->w,strip->h))),
		tickPosition);

  /* 
   * tick labelling (left side only)
   *  (move label slightly if there's already a string in that area)
   */
      valueString[0] = '\0';
      sprintf(valueString,"%.3g",valueValueScale);
      textWidth = XTextWidth(strip->axesFontStruct,valueString,
			strlen(valueString));
      XSetFont(strip->display,strip->gc,strip->axesFontStruct->fid);
      XDrawString(strip->display,strip->pixmap,strip->gc,
	strip->dataX0 - textWidth
		-(int)(RELATIVETICKSIZE*(min(strip->w,strip->h))),
	stringPosition,
	valueString,strlen(valueString));
    }

/* put up a little colored rectangle next to the scale */
    XSetForeground( strip->display, strip->gc, strip->dataPixel[buf]);
    XFillRectangle(strip->display,strip->pixmap,strip->gc,
        (int)(STRIP_MARGIN*strip->w) - maxTextWidth
                - (int)(RELATIVETICKSIZE*(min(strip->w,strip->h)))
                - MINBORDER - buf*strip->axesFontHeight/2-3,
	blobPositionY - strip->axesFontHeight/2,
        strip->axesFontHeight/2-1, strip->axesFontHeight/2-1);
 }

 }



/* 
 * Put up independent variable axis (X) and tick marks
 */

XSetForeground( strip->display, strip->gc, strip->forePixel);

/* top line */
XDrawLine(strip->display,strip->pixmap,strip->gc,
		strip->dataX0, strip->dataY0, strip->dataX1, strip->dataY0);

/* bottom line */
XDrawLine(strip->display,strip->pixmap,strip->gc,
		strip->dataX0, strip->dataY1, strip->dataX1, strip->dataY1);

/*
 * now X axis labelling...
 */

   timeScale = strip->dataWidth / (timeMax - timeMin);

   for (timeValueScale=timeMin; timeValueScale<=timeMax; 
		timeValueScale+=timeInterval){

    /* problem near zero, check if `close enough' */
      if (timeValueScale > -ALMOSTZERO && timeValueScale < ALMOSTZERO)
	  timeValueScale = 0.0;

      tickPosition = (int)((timeValueScale - timeMin)*timeScale)
	+ strip->dataX0;
     /* bottom ticks (could do grids right around here...) */
      XDrawLine(strip->display,strip->pixmap,strip->gc,
		tickPosition, strip->dataY1, tickPosition,
		strip->dataY1 
			+ (int)(RELATIVETICKSIZE*(min(strip->h,strip->w))));
     /* top ticks */
      XDrawLine(strip->display,strip->pixmap,strip->gc,
		tickPosition, strip->dataY0, tickPosition,
		strip->dataY0 
		    - (int)(RELATIVETICKSIZE*(min(strip->h,strip->w))));
    /* 
     * tick labelling (bottom only)
     */
      sprintf(valueString,"%.3g",timeValueScale);
      textWidth = XTextWidth(strip->axesFontStruct,valueString,
			strlen(valueString));
      XSetFont(strip->display,strip->gc,strip->axesFontStruct->fid);
      XDrawString(strip->display,strip->pixmap,strip->gc,
	tickPosition - textWidth/2,
	strip->dataY1 
	   + (int)(RELATIVETICKSIZE*(min(strip->h,strip->w)))/2
	   + strip->axesFontHeight,
	valueString,strlen(valueString));
   }

/*
 * put down title
 */
 if (strip->title != NULL) {
  textWidth = XTextWidth(strip->titleFontStruct,
	strip->title,strlen(strip->title));
  XSetFont(strip->display,strip->gc,strip->titleFontStruct->fid);
  XDrawString(strip->display,strip->pixmap,strip->gc,
	(strip->w/2 -  textWidth/2),
	(strip->dataY0/2) + 2, strip->title,strlen(strip->title));
 }


/*
 * label axes
 */
 XSetFont(strip->display,strip->gc,strip->axesFontStruct->fid);

/*
 * put down x axis label
 */
 if (strip->xAxisLabel != NULL) {
  textWidth = XTextWidth(strip->axesFontStruct,strip->xAxisLabel,
			strlen(strip->xAxisLabel));
  tempPosition = strip->dataX1;
  xPosition = tempPosition + (strip->w - tempPosition)/2 - textWidth/2
		- (int)(binWidth/2.0);
  xPosition = (xPosition+textWidth > strip->w - MINBORDER 
		? strip->w - MINBORDER - textWidth : xPosition);
  tempPosition = strip->dataY1+(int)(RELATIVETICKSIZE*(min(strip->h,strip->w)))
			+ strip->axesFontHeight;
  yPosition = min( (tempPosition + 
		   3*(strip->h - tempPosition)/5 + strip->axesFontHeight/2),
		 (strip->h - strip->axesFontHeight/2) );

  XDrawString(strip->display,strip->pixmap,strip->gc,
	xPosition, yPosition,
	strip->xAxisLabel,strlen(strip->xAxisLabel));
 }

/*
 * put down y axis label
 */
 if (strip->yAxisLabel != NULL) {
  textWidth = XTextWidth(strip->axesFontStruct,strip->yAxisLabel,
			strlen(strip->yAxisLabel));
  tempPosition = strip->dataX0;
  xPosition = tempPosition/2 - textWidth/2;
  xPosition = (xPosition < MINBORDER ? MINBORDER : xPosition);
  tempPosition = strip->dataY0 - strip->axesFontHeight;
  yPosition = tempPosition/2  + strip->axesFontHeight/2 + 2;
  XDrawString(strip->display,strip->pixmap,strip->gc,
	xPosition, yPosition,
	strip->yAxisLabel,strlen(strip->yAxisLabel));
 }

/* end with proper line attributes for fast rendering of data */
 XSetLineAttributes( strip->display, strip->gc,
			DATALINEWIDTH,LineSolid,CapButt,JoinMiter);

} /* end if (!axesAlreadyDrawn) */

/*
 * from pixmap to window
 */
if (strip->axesAlreadyDrawn) {
  XCopyArea(strip->display,strip->pixmap,strip->window,strip->gc,
		strip->dataX0+1,strip->dataY0+1,
		strip->dataWidth-2,strip->dataHeight-2,
		strip->dataX0+1,strip->dataY0+1);
} else {
  strip->axesAlreadyDrawn = True;
/* since we don't want to draw over the shadows of any Motif DA... */
  XCopyArea(strip->display,strip->pixmap,strip->window,strip->gc,
		strip->shadowThickness,strip->shadowThickness,
		strip->w-(2*strip->shadowThickness),
		strip->h-(2*strip->shadowThickness),
		strip->shadowThickness,strip->shadowThickness);
}

XFlush(strip->display);


}





/*
 * Refresh the display
 * (don't recalcuate, just remap the pixmap onto the window)
 */

void stripRefresh(strip)
  Strip  *strip;
{
  XSetClipMask(strip->display,strip->gc,None);
  strip->clipRectanglesSet = False;
/* since we don't want to draw over the shadows of any Motif DA... */
  XCopyArea(strip->display,strip->pixmap,strip->window,strip->gc,
		strip->shadowThickness,strip->shadowThickness,
		strip->w-(2*strip->shadowThickness),
		strip->h-(2*strip->shadowThickness),
		strip->shadowThickness,strip->shadowThickness);
  XFlush(strip->display);
}




/*
 *   Add a strip chart 
 *
 *   input parameters:
 *
 *   strip	- the strip chart to be described (as returned from stripInit())
 *   nChannels  - number of channels to be plotted (number of dataBuffers, etc)
 *   bufferSize - size of buffers
 *   dataBuffer - array of array of doubles: dataBuffer[nChannels][bufferSize]
 *   valueMin   - minimum Value scale
 *   valueMax   - maximum Value scale
 *   samplingInterval - sampling freq: how long to wait between updates (sec)
 *   getValue   - array of pointers to functions returning a double 
 *			(is called every samplingInterval period of time)
 *   title	- character string title
 *   titleFont	- character string title font
 *   xAxisLabel	- character string X axis label
 *   yAxisLabel	- character string Y axis label
 *   axesFont	- character string axes font
 *   foreColor  - character string foreground color
 *   backColor  - character string background color
 *   dataColor  - array of character strings:  data colors
 *   storageMode - if StripInternal, make a copy of the data; 
 *			otherwise (StripExternal), just copy the pointers
 */

void stripSet( strip, nChannels, bufferSize, dataBuffer,
			valueRange, samplingInterval, getValue,
			title, titleFont, 
			xAxisLabel, yAxisLabel,
			axesFont, foreColor, backColor, dataColor, storageMode)
  Strip		*strip;
  int		 nChannels;
  int		 bufferSize;
  double	*dataBuffer[];
  StripRange	*valueRange;
  double	 samplingInterval;
  double	(*getValue[])();
  char		*title, *titleFont, *xAxisLabel, *yAxisLabel, *axesFont;
  char		*foreColor, *backColor, *dataColor[];
  StripStorageMode storageMode;
{
int i, j, k;
double r, g, b;
int stringLength = 0;
XColor color, ignore;
int fontCount;
char **fontNames;
XFontStruct *fontInfo;


if (storageMode == StripExternal) {
    strip->storageMode = StripExternal;
    strip->dataBuffer = dataBuffer;
    strip->titleFont = titleFont;
    strip->axesFont = axesFont;
    strip->title = title;
    strip->xAxisLabel = xAxisLabel;
    strip->yAxisLabel = yAxisLabel;
} else {
    strip->storageMode = StripInternal;
    if (strip->dataBuffer) {
        for (i = 0; i < nChannels; i++)
           free( (char *) strip->dataBuffer[i] );
        free( (char *) strip->dataBuffer );
    }
    strip->dataBuffer = (double **) malloc((unsigned) nChannels 
	* sizeof( double *));
    for (i = 0; i < nChannels; i++)
        strip->dataBuffer[i] = (double *)
	    malloc((unsigned) ( bufferSize * sizeof(double)));

    if (strip->titleFont) free( (char *) strip->titleFont);
    if (titleFont != NULL && !strip->setOwnFonts) {
       stringLength = strlen(titleFont) + 1;
       strip->titleFont = (char *) malloc(stringLength);
       memcpy((void *)strip->titleFont,(void *)titleFont,(size_t)stringLength);
       strip->titleFont[stringLength-1] = '\0';
    }

    if (strip->axesFont) free( (char *) strip->axesFont);
    if (axesFont != NULL && !strip->setOwnFonts) {
      stringLength = strlen(axesFont) + 1;
      strip->axesFont = (char *) malloc(stringLength);
      memcpy((void *)strip->axesFont,(void *)axesFont,(size_t)stringLength);
      strip->axesFont[stringLength-1] = '\0';
    }

    if (strip->title) free( (char *) strip->title);
    if (title != NULL) {
       stringLength = strlen(title) + 1;
       strip->title = (char *) malloc(stringLength);
       memcpy((void *)strip->title, (void *)title, (size_t)stringLength);
       strip->title[stringLength-1] = '\0';
    }

    if (strip->xAxisLabel) free( (char *) strip->xAxisLabel);
    if (xAxisLabel != NULL) {
       stringLength = strlen(xAxisLabel) + 1;
       strip->xAxisLabel = (char *) malloc(stringLength);
       memcpy((void *)strip->xAxisLabel,(void *)xAxisLabel,
		(size_t)stringLength);
       strip->xAxisLabel[stringLength-1] = '\0';
    }

    if (strip->yAxisLabel) free( (char *) strip->yAxisLabel);
    if (yAxisLabel != NULL) {
       stringLength = strlen(yAxisLabel) + 1;
       strip->yAxisLabel = (char *) malloc(stringLength);
       memcpy((void *)strip->yAxisLabel,(void *)yAxisLabel,
		(size_t)stringLength);
       strip->yAxisLabel[stringLength-1] = '\0';
    }

}

/* allocate the array of XPoint arrays for rendering */
if (strip->pointArray) {
   for (i = 0; i < nChannels; i++)
           free( (char *) strip->pointArray[i] );
   free( (char *) strip->pointArray);
}
strip->pointArray = (XPoint **) malloc((unsigned)nChannels*sizeof(XPoint *));
for (i = 0; i < nChannels; i++)
  strip->pointArray[i] = (XPoint*)malloc((unsigned)(bufferSize*sizeof(XPoint)));


strip->nChannels = nChannels;
strip->bufferSize = bufferSize;

if (strip->bufferPtr) free( (char *) strip->bufferPtr);
strip->bufferPtr = (unsigned long *) malloc((unsigned) nChannels 
	* sizeof(unsigned long));

if (strip->nValues) free( (char *) strip->nValues);
strip->nValues = (int *) malloc((unsigned) nChannels * sizeof(int));

if (strip->dataPixel) free( (char *) strip->dataPixel);
strip->dataPixel = (unsigned long *) malloc((unsigned) nChannels * 
	sizeof(unsigned long));

if (strip->getValue) free( (char *) strip->getValue);
strip->getValue = (double (**)()) malloc((unsigned) nChannels 
	* sizeof( double (**)() ) );

for (i = 0; i< nChannels; i++) {
   strip->bufferPtr[i] = 0;
   strip->nValues[i]   = 0;
   strip->getValue[i]  = getValue[i];

   strip->valueRange[i].minVal = valueRange[i].minVal;
   strip->valueRange[i].maxVal = valueRange[i].maxVal;
}

strip->samplingInterval = samplingInterval;
strip->adjustedInterval = 0.0;



/* set foreground, background and data colors based on passed-in strings */

/* use a color for the foreground if possible */
if (!strip->setOwnColors) {
  if (DisplayCells(strip->display,strip->screen)>2) {
    if(!XAllocNamedColor(strip->display,
	DefaultColormap(strip->display,strip->screen),
	foreColor,&color,&ignore)){
        fprintf(stderr,"\nstripSet:  couldn't allocate color %s",foreColor);
        strip->forePixel  = WhitePixel(strip->display,strip->screen);
    } else {
        strip->forePixel = color.pixel;
    }

    if(!XAllocNamedColor(strip->display,
	DefaultColormap(strip->display,strip->screen),
	backColor,&color,&ignore)) {
        fprintf(stderr,"\nstripSet:  couldn't allocate color %s",backColor);
        strip->backPixel  = BlackPixel(strip->display,strip->screen);
    } else {
        strip->backPixel = color.pixel;
    }

  /* if dataColor pointer is NULL, then do own color allocation */
    if (dataColor == NULL) {
      if (XAllocColorCells(strip->display,
                DefaultColormap(strip->display,strip->screen), FALSE,
                NULL, 0, strip->dataPixel,nChannels)) {

        for (k = 0; k < nChannels; k++) {
/* (MDA) - rainbow( (double)(k+1)/(double)nChannels, 1.0, 1.0, &r, &g, &b); */
            r = ((double) rand())/pow(2.0,31.0);
            g = ((double) rand())/pow(2.0,31.0);
            b = ((double) rand())/pow(2.0,31.0);
            r *= 65535; g *= 65535; b *= 65535;
            color.red = (unsigned int) r;
            color.green = (unsigned int) g;
            color.blue = (unsigned int) b;
            color.pixel = strip->dataPixel[k];
            color.flags = DoRed | DoGreen | DoBlue;
            XStoreColor(strip->display,
                DefaultColormap(strip->display,strip->screen), &color);
        }

      } else {
        fprintf(stderr,"\nstripSet:  couldn't allocate color cells");
      }
    } else {
      for (i = 0; i < nChannels; i++) {
       if(!XAllocNamedColor(strip->display,
        DefaultColormap(strip->display,strip->screen),
        dataColor[i],&color,&ignore)) {
        fprintf(stderr,"\nstripSet:  couldn't allocate color %s",dataColor);
        strip->dataPixel[i]  = WhitePixel(strip->display,strip->screen);
       } else {
        strip->dataPixel[i] = color.pixel;
       }
      }
    }

  } else {
    strip->forePixel  = WhitePixel(strip->display,strip->screen);
    strip->backPixel  = BlackPixel(strip->display,strip->screen);
    for (i = 0; i < nChannels; i++)
       strip->dataPixel[i]  = WhitePixel(strip->display,strip->screen);
  }
}


/*
 * load the fonts
 */
if (!strip->setOwnFonts) {
   if ((strip->titleFontStruct=XLoadQueryFont(strip->display,strip->titleFont)) 
						== NULL) {
      fprintf(stderr,"display %s doesn't know font %s\n",
                DisplayString(strip->display),strip->titleFont);
      fprintf(stderr,"\t using first available font\n");
      fontNames = XListFontsWithInfo(strip->display,FONTWILDCARD,
		MAXFONTS,&fontCount,&fontInfo);
      strip->titleFontStruct = XLoadQueryFont(strip->display,fontNames[0]);
      XFreeFontInfo(fontNames,fontInfo,fontCount);
      XFreeFontNames(fontNames);
   }
   strip->titleFontHeight = strip->titleFontStruct->ascent + 
				strip->titleFontStruct->descent;
   strip->titleFontWidth = strip->titleFontStruct->max_bounds.rbearing - 
				strip->titleFontStruct->min_bounds.lbearing;

   if ((strip->axesFontStruct=XLoadQueryFont(strip->display,strip->axesFont)) 
						== NULL) {
      fprintf(stderr,"stripSet: display %s doesn't know font %s\n",
                DisplayString(strip->display),strip->axesFont);
      fprintf(stderr,"\t using first available font\n");
      strip->axesFontStruct = strip->titleFontStruct;
   }
   strip->axesFontHeight = strip->axesFontStruct->ascent + 
				strip->axesFontStruct->descent;
   strip->axesFontWidth = strip->axesFontStruct->max_bounds.rbearing - 
				strip->axesFontStruct->min_bounds.lbearing;

}

/* 
 * initialize (to background colour) the two pixmaps
 */
XSetForeground(strip->display, strip->gc, strip->backPixel);

XFillRectangle( strip->display, strip->pixmap, strip->gc, 
			0, 0, strip->w, strip->h);

/***
 *** IMPORTANT:  the Intrinsics must have been initialized for the
 ***	Strip Chart to work, since it relies on TimeOut's
 ***/

/*
 * register the initial (unadjusted) TimeOut here
 */

if (strip->appContext == NULL) {
  strip->timeoutId = XtAddTimeOut(
	(int) (1000.0 * strip->samplingInterval),
	(XtTimerCallbackProc)updateStrip, strip );
} else {
  strip->timeoutId = XtAppAddTimeOut(strip->appContext,
	(int) (1000.0 * strip->samplingInterval),
	(XtTimerCallbackProc)updateStrip, strip );
}


strip->paused = FALSE;

}


/*
 *   Set the  value axis display range of a Strip chart plot
 *
 *   input parameters:
 *
 *   strip       - pointer to Strip data structure
 *   curveNum	 - curve or data set number
 *   minVal      - minimum display value
 *   maxVal      - maximum display value
 */

void stripSetRange(strip,curveNum,minVal,maxVal)
  Strip *strip;
  int curveNum;
  double minVal,maxVal;
{
     if (curveNum < 0 || curveNum >= strip->nChannels) {
        fprintf(stderr,
         "\nstripSetRange: illegal curve (number %d) specified\n",
		curveNum);
        return;
     }

     if (minVal >= maxVal) {
        fprintf(stderr,
         "\nstripSetRange: improper range specified, using defaults\n");
        return;
     }
     strip->valueRange[curveNum].minVal = minVal;
     strip->valueRange[curveNum].maxVal = maxVal;
}




/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * here's the Pause strip chart routine
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void stripPause(strip)
  Strip *strip;
{
  if (strip->timeoutId != NULL) XtRemoveTimeOut(strip->timeoutId);
  strip->timeoutId = NULL;
  strip->paused = TRUE;

}




/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
 * and the Resume strip chart routine
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

void stripResume(strip)
  Strip *strip;
{
 /* reregister the timeout */
  if (strip->paused) {
    strip->paused = FALSE;
    if (strip->appContext == NULL) {
      strip->timeoutId = XtAddTimeOut( 
       max(0,(int)
	(1000.0*(strip->samplingInterval - strip->adjustedInterval))),
	(XtTimerCallbackProc)updateStrip,strip);
    } else {
      strip->timeoutId = XtAppAddTimeOut( strip->appContext,
	max(0,(int)(1000.0*(strip->samplingInterval-strip->adjustedInterval))),
	(XtTimerCallbackProc)updateStrip,strip);
    }
  }
}



/*
 *   Print (dump in xwd format, then pipe to `xpr -device ps' then pipe
 *   to printer defined by PSPRINTER environment variable...)
 *
 *   input parameters:
 *
 *   strip        - pointer to Strip data structure
 */
void stripPrint(strip)
  Strip *strip;
{

  utilPrint(strip->display, strip->window, XWD_TEMP_FILE_STRING);

}




/*
 *   Set the legend parameters
 *
 *   input parameters:
 *
 *   strip        - pointer to Strip data structure
 *   legendTitle - title string
 *   legendArray - pointer to array of legend entry strings
 */
void stripSetLegend(strip, legendTitle, legendArray)
  Strip *strip;
  char *legendTitle;
  char **legendArray;
{
  int i, legendWidth = 0, legendHeight= 0;
  XWindowAttributes parentWindowAttributes;
  XSetWindowAttributes windowAttributes;


  if (strip->legend) XDestroyWindow(strip->display,strip->legendWindow);

  strip->legend = TRUE;

  if (strip->legendTitle != NULL) free( (char *) strip->legendTitle);
  strip->legendTitle = malloc(strlen(legendTitle)+1);
  strcpy(strip->legendTitle,legendTitle);

  if (strip->legendArray != NULL) {
     for (i=0; i<strip->nChannels; i++) free( (char *) strip->legendArray[i]);
     free( (char *) strip->legendArray);
  }
  strip->legendArray = (char **) malloc(strip->nChannels*sizeof(char *));
  for (i=0; i<strip->nChannels; i++){
     strip->legendArray[i] = malloc(strlen(legendArray[i])+1);
     strcpy(strip->legendArray[i],legendArray[i]);
  }


/*
 * loop to determine required size of legend window
 */
  legendWidth = XTextWidth(strip->titleFontStruct,legendTitle,
                       strlen(legendTitle)) + 2*MINBORDER;
  for (i=0; i<strip->nChannels; i++) {
      legendWidth = max(legendWidth,
                        XTextWidth(strip->axesFontStruct,legendArray[i],
                        strlen(legendArray[i]))+strip->axesFontHeight+
                        4*MINBORDER);

  }

  legendHeight = 2*MINBORDER + strip->nChannels*MINBORDER + MINBORDER +
                        strip->titleFontHeight +
                        strip->nChannels*strip->axesFontHeight;

/*
 * create a window butting up against strip window.
 * legendWindow is created as child of strip window.
 */
  strip->legendWindow = XCreateSimpleWindow(strip->display,
                                strip->window,
                                strip->w-legendWidth
                                        -LEGENDOFFSET-(2*BORDERLINEWIDTH),
                                LEGENDOFFSET,
                                legendWidth, legendHeight,
                                BORDERLINEWIDTH,
                                strip->forePixel, strip->backPixel);

/*
 * Set backing store attribute from parent window
 *  (Xt and XView differ on the default)
 */
  XGetWindowAttributes(strip->display, strip->window, &parentWindowAttributes);
  windowAttributes.backing_store = parentWindowAttributes.backing_store;
  XChangeWindowAttributes(strip->display, strip->legendWindow,
        CWBackingStore, &windowAttributes);

  XMapRaised(strip->display,strip->legendWindow);
 

}


/*
 *   Draw the legend
 *
 *   input parameters:
 *
 *   strip        - pointer to Strip data structure
 */
void stripDrawLegend(strip)
  Strip *strip;
{
  int i;

  if (!strip->legend) return;

/* title */
  XSetFont(strip->display,strip->gc,strip->titleFontStruct->fid);
  XSetForeground( strip->display, strip->gc, strip->forePixel);
  XDrawString(strip->display,strip->legendWindow,strip->gc,
        MINBORDER, strip->titleFontHeight + MINBORDER,
        strip->legendTitle,strlen(strip->legendTitle));

/* legend color-key rectangles */
/* (use axesFontHeight as both Cell width and height */
  XSetFont(strip->display,strip->gc,strip->axesFontStruct->fid);
  for (i=0; i<strip->nChannels; i++) {
     XSetForeground( strip->display, strip->gc, strip->dataPixel[i]);
     XFillRectangle( strip->display, strip->legendWindow, strip->gc, MINBORDER,
        3*MINBORDER + strip->titleFontHeight +
           i*(strip->axesFontHeight+MINBORDER),
        strip->axesFontHeight, strip->axesFontHeight);
  }

/* legend strings */
  XSetForeground( strip->display, strip->gc, strip->forePixel);
  for (i=0; i<strip->nChannels; i++) {
     XDrawString(strip->display,strip->legendWindow,strip->gc,
        strip->axesFontHeight + 2*MINBORDER,
        3*MINBORDER + strip->titleFontHeight + 2*strip->axesFontHeight/3 +
           i*(strip->axesFontHeight+MINBORDER),
        strip->legendArray[i],strlen(strip->legendArray[i]));
  }

  XRaiseWindow(strip->display,strip->legendWindow);

}


