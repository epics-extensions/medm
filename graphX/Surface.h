/*
 *  Surface structure definition
 */

#ifndef __SURFACE_H__
#define __SURFACE_H__


#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include "GraphXMacros.h"

#include "3D.h"


/* must define the filename of the temporary xwd file for printing */

#ifndef XWD_TEMP_FILE_STRING
#  define XWD_TEMP_FILE_STRING    "./surface.xwd"
#endif



typedef enum {
    SurfaceInternal,               /* make private copies of all surface data */
    SurfaceExternal                /* use the passed-in storage for all data */
} SurfaceStorageMode;

typedef enum {
    SurfaceSolid = SOLID,	   /* solid model, hidden line removed */
    SurfaceShaded = SHADED	   /* shaded model */
} SurfaceRenderMode;



struct _Surface {
    Display	   *display;
    int		    screen;
    Window	    window;
    Pixmap	    pixmap;
    GC		    gc;

    SurfaceStorageMode storageMode;
    double	   *mesh, *x, *y;
    int		    nx, ny;

    ThreeD	    *s3d;	    /* pointer to structure for 3D stuff */

    double	    minX;	    /* need these to go from normalized  */
    double	    maxX;	    /*  data [0,1] and back again        */
    double	    deltaX;
    double	    minY;
    double	    maxY;
    double	    deltaY;
    double	    minZ;
    double	    maxZ;
    double	    deltaZ;

    BOOLEAN	    setOwnColors;	/* if TRUE user will fill in pixels */
    unsigned long   forePixel, backPixel, dataPixel;

    unsigned long   w;
    unsigned long   h;

    BOOLEAN	    setOwnFonts;	/* if TRUE user will fill in fonts */
    char	   *title;
    char	   *titleFont;
    XFontStruct	   *titleFontStruct;
    int		    titleFontHeight;
    int		    titleFontWidth;


    XtPointer	    userData;		/* pointer to arbitrary user data */

    };


typedef struct _Surface Surface;


#ifdef _NO_PROTO
extern Surface *surfaceInit();
extern void surfaceDraw();
extern void surfacePrint();
extern void surfaceRefresh();
extern void surfaceResize();
extern void surfaceSet();
extern void surfaceSetRenderMode();
extern void surfaceSetView();
extern void surfaceTerm();
#else
extern Surface *surfaceInit(Display *, int, Window);
extern void surfaceDraw(Surface *);
extern void surfacePrint(Surface *);
extern void surfaceRefresh(Surface *);
extern void surfaceResize(Surface *);
extern void surfaceSet(Surface *, double *, double *, double *, int, int,
	char *, char *, char *, char *, char*, SurfaceStorageMode);
extern void surfaceSetRenderMode(Surface *, SurfaceRenderMode);
extern void surfaceSetView(Surface *, float, float, float);
extern void surfaceTerm(Surface *);
#endif

#endif  /* __SURFACE_H__ */
