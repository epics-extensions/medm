/*
 * Graph3d structure definition (allows display of (x,y,z) triples)
 */

#ifndef __GRAPH3D_H__
#define __GRAPH3D_H__

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include "GraphXMacros.h"


#define GRAPH3D_MARGIN		0.1	/* window fraction for text, &c */
#define GRAPH3D_DRAWINGAREA	0.8	/* 2*MARGIN + DRAWINGAREA == 1 */


/* must define the filename of the temporary xwd file for printing */

#ifndef XWD_TEMP_FILE_STRING
#  define XWD_TEMP_FILE_STRING    "./graph3d.xwd"
#endif



typedef enum {
    Graph3dGrid,		/* XYZ plot (XY defined on grid, color) type */
    Graph3dPoint,		/* XYZ plot (simple (x,y,color)) type */
    Graph3dContour		/* XYZ plot (contour) type */
} Graph3dType;

typedef enum {
    Graph3dInternal,		/* make private copies of all  data */
    Graph3dExternal		/* use the passed-in storage for all data */
} Graph3dStorageMode;

typedef struct {
    double	   x;
    double	   y;
    double	   z;
} XYZdataPoint;

typedef enum {
    Graph3dDefaultRange,	/* do own range calculation: based on data */
    Graph3dSpecRange	/* use user specified ranges */
} Graph3dRangeType;

typedef struct {
    double minVal;
    double maxVal;
} Graph3dRange;



struct _Graph3d {
    Display	    *display;
    int		     screen;
    Window	     window;
    Pixmap	     pixmap;
    GC		     gc;
    Graph3dStorageMode storageMode;

    XYZdataPoint       *dataPoints;
    int              nPoints;

    Graph3dRangeType xRangeType;	/* enumerated type for display range */
    Graph3dRange     xRange;		/* the range itself */
    Graph3dRangeType yRangeType;
    Graph3dRange     yRange;
    Graph3dRangeType zRangeType;
    Graph3dRange     zRange;

    Graph3dType	     type;		/* the enumerated type of Graph3d */
    int		     xBins;		/* the number of X bins (Graph3dBin) */
    int		     yBins;		/* the number of Y bins (Graph3dBin) */
    int		     zBins;		/* the number of Z bins (Graph3dBin) */

    XPoint	    *devicePoints;	/* device coordinates for dataPoints */

    BOOLEAN	     setOwnColors;	/* if TRUE user will fill in pixels */
    unsigned long    forePixel; 	/* foreground color */
    unsigned long    backPixel;		/* background color */
    unsigned long    dataPixel;		/* primary data color */

    unsigned long    *zPixels;		/* color table for Z */

    unsigned long    w;
    unsigned long    h;

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

    XtPointer	     userData;		/* pointer to arbitrary user data */

    };
typedef struct _Graph3d Graph3d;


#ifdef _NO_PROTO
extern Graph3d *graph3dInit();
extern void graph3dDraw();
extern void graph3dPrint();
extern void graph3dRefresh();
extern void graph3dResize();
extern void graph3dSet();
extern void graph3dSetRange();
extern void graph3dSetRangeDefault();
extern void graph3dSetBins();
extern void graph3dSetZBins();
extern void graph3dSetDisplayType();
extern void graph3dTerm();
#else
extern Graph3d *graph3dInit(Display *, int, Window);
extern void graph3dDraw(Graph3d *);
extern void graph3dPrint(Graph3d *);
extern void graph3dRefresh(Graph3d *);
extern void graph3dResize(Graph3d *);
extern void graph3dSet(Graph3d *, XYZdataPoint *, int, Graph3dType,
	char *, char *, char *, char *, char *, char *, char *, char *,
	Graph3dStorageMode);
extern void graph3dSetRange(Graph3d *, char, double, double);
extern void graph3dSetRangeDefault(Graph3d *, char);
extern void graph3dSetBins(Graph3d *, char, int);
extern void graph3dSetZBins(Graph3d *, int, char **);
extern void graph3dSetDisplayType(Graph3d *, Graph3dType);
extern void graph3dTerm(Graph3d *);
#endif

#endif  /*__GRAPH3D_H__ */
