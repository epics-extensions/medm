/*
 * Seql structure definition
 */

#ifndef __SEQL_H__
#define __SEQL_H__


#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include "GraphXMacros.h"


#define SEQL_MARGIN		0.15	/* window fraction for text, &c */
#define SEQL_DRAWINGAREA	0.7	/* 2*MARGIN + DRAWINGAREA == 1 */


/* must define the filename of the temporary xwd file for printing */

#ifndef XWD_TEMP_FILE_STRING
#  define XWD_TEMP_FILE_STRING	"./seql.xwd"
#endif


#ifndef MAXCURVES
#  define MAXCURVES 10	/* maximum number of traces or curves per plot */
#endif


typedef enum {
    SeqlPolygon,	/* sequential data filled polygon type */
    SeqlLine,		/* sequential data line type */
    SeqlPoint		/* sequential data point type */
} SeqlType;

typedef enum {
    SeqlInternal,	/* make private copies of all sequential data */
    SeqlExternal	/* use the passed-in storage for all data */
} SeqlStorageMode;

typedef enum {
    SeqlDefaultRange,	/* do own range calculation: based on data */
    SeqlSpecRange	/* use user specified ranges */
} SeqlRangeType;

typedef struct {
    double minVal;
    double maxVal;
} SeqlRange;



struct _seqlInteraction {

        unsigned long crosshairPixel;
        BOOLEAN first;
        int saveX0, saveY0, saveX1, saveY1;
        int useX0, useY0, useX1, useY1;
        GC xorGC;
        int bufNum;

};
typedef struct _seqlInteraction SeqlInteraction;



struct _Seql {
    Display	    *display;
    int		    screen;
    Window	    window;
    Pixmap	    pixmap;
    GC		    gc;
    SeqlStorageMode storageMode;

    int             nBuffers;		/* the # of "plots" in this seql */
    double	  **dataBuffer;		/* an array of data buffers */
    int		    bufferSize;
    int            *nValues;		/* an array of data buffer counters */

    SeqlRangeType   xRangeType[MAXCURVES];
    SeqlRange	    xRange[MAXCURVES];
    SeqlRangeType   yRangeType[MAXCURVES];
    SeqlRange	    yRange[MAXCURVES];

    SeqlType	    type;		/* the enumerated type of Seq'l plot */
    XPoint	   *devicePoints;	/* device coordinates for dataPoints */

    BOOLEAN	    setOwnColors;	/* if TRUE user will fill in pixels */
    unsigned long   forePixel; 
    unsigned long   backPixel;
    unsigned long  *dataPixel;		/* an array of pixel values for data */

    BOOLEAN	    dontLinearScaleX;	/* if TRUE don't make nice X scales */
    BOOLEAN	    dontLinearScaleY;	/* if TRUE don't make nice Y scales */
    unsigned long   w;
    unsigned long   h;

    /* these (X0,Y0), (X1,Y1) are relative to main window/pixmap */
    unsigned long    dataX0;		/* upper left corner - data region X */
    unsigned long    dataY0;		/* upper left corner - data region Y */
    unsigned long    dataX1;		/* lower right corner - data region X */
    unsigned long    dataY1;		/* lower right corner - data region Y */

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

    BOOLEAN	    legend;
    char	   *legendTitle;
    char          **legendArray;
    Window	    legendWindow;

    XtPointer	    userData;		/* pointer to arbitrary user data */

    SeqlInteraction *interactive;

  };

typedef struct _Seql Seql;


#ifdef _NO_PROTO
extern Seql *seqlInit();
extern void seqlDraw();
extern void seqlPrint();
extern void seqlRefresh();
extern void seqlResize();
extern void seqlSet();
extern void seqlSetRange();
extern void seqlSetRangeDefault();
extern void seqlSetDisplayType();
extern void seqlSetDataColor();
extern void seqlTerm();
extern void seqlSetLegend();
extern void seqlDrawLegend();
extern void seqlSetInteractive();
#else
extern Seql *seqlInit(Display *, int, Window);
extern void seqlDraw(Seql *);
extern void seqlPrint(Seql *);
extern void seqlRefresh(Seql *);
extern void seqlResize(Seql *);
extern void seqlSet(Seql *, int, int, double **, int *, SeqlType,
	char *, char *, char *, char *, char *, char *, char *, char **,
	SeqlStorageMode);
extern void seqlSetRange(Seql *, int, char, double, double);
extern void seqlSetRangeDefault(Seql *, int, char);
extern void seqlSetDisplayType(Seql *, SeqlType);
extern void seqlSetDataColor(Seql *, int, char *);
extern void seqlTerm(Seql *);
extern void seqlSetLegend(Seql *,  char *, char **);
extern void seqlDrawLegend(Seql *);
extern void seqlSetInteractive(Seql *, Boolean);
#endif

#endif  /*__SEQL_H__ */
