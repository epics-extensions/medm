/*
 * Graph structure definition
 */

#ifndef __GRAPH_H__
#define __GRAPH_H__

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include "GraphXMacros.h"


#define GRAPH_MARGIN		0.15	/* window fraction for text, &c */
#define GRAPH_DRAWINGAREA	0.7	/* 2*MARGIN + DRAWINGAREA == 1 */


/* must define the filename of the temporary xwd file for printing */

#ifndef XWD_TEMP_FILE_STRING
#  define XWD_TEMP_FILE_STRING    "./graph.xwd"
#endif


#ifndef MAXCURVES
#  define MAXCURVES 10	/* maximum number of traces or curves per plot */
#endif

typedef enum {
    GraphLine,		/* XY plot (conected line sequence) type */
    GraphPoint,		/* XY plot (scatter or point) type */
    GraphBar		/* 1D bar chart (vertical - with `binning' ) type */
} GraphType;

typedef enum {
    GraphInternal,		/* make private copies of all  data */
    GraphExternal		/* use the passed-in storage for all data */
} GraphStorageMode;

typedef struct {
    double	   x;
    double	   y;
} XYdataPoint;

typedef enum {
    GraphDefaultRange,	/* do own range calculation: based on data */
    GraphSpecRange	/* use user specified ranges */
} GraphRangeType;

typedef struct {
    double minVal;
    double maxVal;
} GraphRange;


struct _graphInteraction {

        unsigned long crosshairPixel;
        BOOLEAN first;
        int saveX0, saveY0, saveX1, saveY1;
        int useX0, useY0, useX1, useY1;
        GC xorGC;
        int bufNum;

};
typedef struct _graphInteraction GraphInteraction;



struct _Graph {
    Display	    *display;
    int		     screen;
    Window	     window;
    Pixmap	     pixmap;
    GC		     gc;
    GraphStorageMode storageMode;

    int		     nBuffers;		/* the # of "plots" in this graph */
    XYdataPoint    **dataBuffer;	/* an array of data buffers */
    int		     bufferSize;
    int             *nValues;		/* an arry of data buffer counters */

    GraphRangeType   xRangeType[MAXCURVES];
    GraphRange       xRange[MAXCURVES];
    GraphRangeType   yRangeType[MAXCURVES];
    GraphRange       yRange[MAXCURVES];

    GraphType	     type;		/* the enumerated type of Graph */
    int		     nBins[MAXCURVES];	/* nBins to use (for GraphBar only)*/
    XPoint	    *devicePoints;	/* device coordinates for dataPoints */

    BOOLEAN	     setOwnColors;	/* if TRUE user will fill in pixels */
    unsigned long    forePixel; 
    unsigned long    backPixel;
    unsigned long   *dataPixel;		/* an arry of pixel values for data */

    BOOLEAN	     dontLinearScaleX;   /* if TRUE don't make nice X scales */
    BOOLEAN	     dontLinearScaleY;   /* if TRUE don't make nice Y scales */
    unsigned long    w;
    unsigned long    h;

    /* these (X0,Y0), (X1,Y1) are relative to main window/pixmap */
    unsigned long    dataX0;            /* upper left corner - data region X */
    unsigned long    dataY0;            /* upper left corner - data region Y */
    unsigned long    dataX1;            /* lower right corner - data region X */
    unsigned long    dataY1;            /* lower right corner - data region Y */


    BOOLEAN	     setOwnFonts;	/* if TRUE user will fill in fonts */
    char	    *title;
    char	    *titleFont;
    XFontStruct	    *titleFontStruct;
    int		     titleFontHeight;
    int		     titleFontWidth;
    char	    *xAxisLabel;
    char	    *yAxisLabel;
    char	    *axesFont;
    XFontStruct	    *axesFontStruct;
    int		     axesFontHeight;
    int		     axesFontWidth;

    BOOLEAN         legend;
    char           *legendTitle;
    char          **legendArray;
    Window          legendWindow;

    XtPointer	    userData;		/* pointer to arbitrary user data */

    GraphInteraction *interactive;

    };
typedef struct _Graph Graph;


#ifdef _NO_PROTO
extern Graph *graphInit();
extern void graphDraw();
extern void graphPrint();
extern void graphRefresh();
extern void graphResize();
extern void graphSet();
extern void graphSetRange();
extern void graphSetRangeDefault();
extern void graphSetBins();
extern void graphSetDisplayType();
extern void graphSetDataColor();
extern void graphTerm();
extern void graphSetLegend();
extern void graphDrawLegend();
extern void graphSetInteractive();
#else
extern Graph *graphInit(Display *, int, Window);
extern void graphDraw(Graph *);
extern void graphPrint(Graph *);
extern void graphRefresh(Graph *);
extern void graphResize(Graph *);
extern void graphSet(Graph *, int, int, XYdataPoint **, int *, GraphType,
	char *, char *,  char *, char *, char *, char *, char *, char **,
	GraphStorageMode);
extern void graphSetRange(Graph *, int, char, double, double);
extern void graphSetRangeDefault(Graph *, int, char);
extern void graphSetBins(Graph *, int, int);
extern void graphSetDisplayType(Graph *, GraphType);
extern void graphSetDataColor(Graph *, int, char *);
extern void graphTerm(Graph *);
extern void graphSetLegend(Graph *, char *, char **);
extern void graphDrawLegend(Graph *);
extern void graphSetInteractive(Graph *, Boolean);
#endif

#endif /*__GRAPH_H__ */
