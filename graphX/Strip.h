/*
 * Strip structure definition
 */

#ifndef __STRIP_H__
#define __STRIP_H__


#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include "GraphXMacros.h"

#define STRIP_MARGIN            0.18    /* window fraction for text, &c */
#define STRIP_DRAWINGAREA       0.64     /* 2*MARGIN + DRAWINGAREA == 1 */



/* must define the filename of the temporary xwd file for printing */

#ifndef XWD_TEMP_FILE_STRING
#  define XWD_TEMP_FILE_STRING    "./strip.xwd"
#endif



#ifndef MAXCURVES
#  define MAXCURVES 10		/* maximum number of curves/traces per plot */
#endif


typedef enum {
    StripInternal,		/* make private copies of all strip data */
    StripExternal		/* use the passed-in storage for all data */
} StripStorageMode;

 
typedef struct {
    double minVal;
    double maxVal;
} StripRange;


 
struct _Strip {
    Display	   *display;
    int		    screen;
    Window	    window;
    Pixmap	    pixmap;
    GC		    gc;
    Boolean	    axesAlreadyDrawn;
    Boolean	    clipRectanglesSet;
    StripStorageMode storageMode;

    int		    nChannels;		/* the # of channels in this strip */
    double	  **dataBuffer;		/* an array of data buffers */
    int             bufferSize;
    unsigned long  *bufferPtr;		/* an array of data buffer pointers */
    int		   *nValues;		/* an array of data buffer counters */
    XPoint	   **pointArray;	/* an array of XPoint arrays */

    StripRange      valueRange[MAXCURVES];

    double	    samplingInterval;	/* sampling interval in secs */
    double	 (**getValue)();	/* array of ptrs. to fns. 
					   returning sampled value */
    BOOLEAN	    paused;		/* state variable: is strip paused? */

    /* these (X0,Y0), (X1,Y1) are relative to main window/pixmap */
    unsigned long    dataX0;		/* upper left corner - data region X */
    unsigned long    dataY0;		/* upper left corner - data region Y */
    unsigned long    dataX1;		/* lower right corner - data region X */
    unsigned long    dataY1;		/* lower right corner - data region Y */
    unsigned long    dataWidth;		/* width of data region */
    unsigned long    dataHeight;	/* height of data region */

    double	    adjustedInterval;	/* adjusted sampling interval in secs */

    BOOLEAN	    setOwnColors;	/* if TRUE user will fill in pixels */
    int		    shadowThickness;	/* # of pixels to not draw in */
    unsigned long   forePixel; 
    unsigned long   backPixel;
    unsigned long   *dataPixel;		/* array of pixel values for data */

    unsigned long   w;
    unsigned long   h;

    BOOLEAN	    setOwnFonts;	/* if TRUE user will fill in fonts */
    char	   *title;
    char	   *titleFont;
    XFontStruct	   *titleFontStruct;
    int		    titleFontHeight;
    int		    titleFontWidth;
    char	   *xAxisLabel;
    char	   *yAxisLabel;
    char	   *axesFont;
    XFontStruct	   *axesFontStruct;
    int		    axesFontHeight;
    int		    axesFontWidth;

    BOOLEAN         legend;
    char           *legendTitle;
    char          **legendArray;
    Window          legendWindow;

    XtPointer	    userData;		/* pointer to arbitrary user data */

    XtAppContext    appContext;		/* default or user-created AppContext */
    XtIntervalId    timeoutId;

    };
typedef struct _Strip Strip;


extern Strip *stripInit(Display *, int, Window);
extern void stripDraw(Strip *);
extern void stripPrint(Strip *);
extern void stripRefresh(Strip *);
extern void stripResize(Strip *);
extern void stripSet(Strip *, int, int, double **, StripRange *, double,
	double(*[])(), char *, char *, char *, char *, char *,
	char *, char *, char **, StripStorageMode);
extern void stripSetDataColor(Strip *, int, char *);
extern void stripSetRange(Strip *, int, double, double);
extern void stripSetInterval(Strip *, double);
extern void stripPause(Strip *);
extern void stripResume(Strip *);
extern void stripTerm(Strip *);
extern void stripSetLegend(Strip *, char *, char **);
extern void stripDrawLegend(Strip *);
extern void stripSetAppContext(Strip *, XtAppContext);


#endif  /* __STRIP_H__ */
