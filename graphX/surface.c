/*
    Surface Plot routines & data structures
 */

#include <X11/Xlib.h>


#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "Surface.h"


#define DRAW_FRACTION 0.8		/* fraction of window to draw surface in */

/*
 * define the x and y positions of the angle strings
 */

#define ANGLE_X 10
#define ANGLE_Y 20

/*
 * define constants for XListFontsWithInfo call
 */
#define FONTWILDCARD "*"
#define MAXFONTS 1



/*
 * Initialize the Surface, allocate transformation matrices
 * function:  Surface *surfaceInit()
 * 
 *   input parameters:
 *
 *   display     - pointer to Display structure
 *   screen      - integer screen number
 *   window      - Window to output graphics into
 */

Surface *surfaceInit(display, screen, window )
    Display *display;
    int     screen;
    Window  window;
{
    Surface *surface;
    XWindowAttributes windowAttributes;
    int w, h;

    surface = (Surface *) malloc( sizeof( Surface ) );
    if (!surface) return(NULL);

    XGetWindowAttributes(display, window, &windowAttributes);
    w = windowAttributes.width;
    h = windowAttributes.height;

  /* allocate the 3d structure */
    surface->s3d = init3d();

    surface->mesh   = (double *)0;

  /* initialize some useful fields */
    surface->display = display;
    surface->screen = screen;
    surface->window = window;

  /* description of extent of surface in Pixel coordinates... */
    surface->w      = w;
    surface->h      = h;

  /*
   * create a pixmap
   */
    surface->pixmap = XCreatePixmap(surface->display,surface->window,
      surface->w,surface->h,
      DefaultDepth(surface->display,surface->screen));

  /*
   * create a default, writable GC
   */
    surface->gc = XCreateGC(surface->display,
      DefaultRootWindow(surface->display),NULL,NULL);

  /*
   * and initialize other fields
   */
    surface->titleFont = NULL;
    surface->title = NULL;


    surface->setOwnColors = FALSE;	/* will user fill in pixel values later? */
    surface->setOwnFonts = FALSE;	/* will user fill in font values later? */

    return(surface);

}






/*
 * Resize the Surface
 * function: void surfaceResize()
 */

void surfaceResize(surface)
    Surface  *surface;
{
    XWindowAttributes windowAttributes;


  /*
   * check to see if window has changed size
   *   if so, then Free the pixmap, fetch a new one and continue
   */
    XGetWindowAttributes(surface->display, surface->window, &windowAttributes);
    if (surface->w != windowAttributes.width  ||
      surface->h != windowAttributes.height){

	surface->w = windowAttributes.width;
	surface->h = windowAttributes.height;
	XFreePixmap(surface->display,surface->pixmap);
	surface->pixmap = XCreatePixmap(surface->display,surface->window,
	  surface->w,surface->h,
	  DefaultDepth(surface->display,surface->screen));

	setPixmap3d(surface->s3d,surface->pixmap);
    }
}






/*
 * Draw the Surface
 * function: void surfaceDraw()
 */

void surfaceDraw(surface)
    Surface  *surface;
{

    char angleString[16];
    int textWidth;

    int i;



  /*
   * do the 3d drawing
   */
    draw3d(surface->s3d);


  /*
   * put down title and current display angle information
   */

    XSetForeground( surface->display, surface->gc, surface->forePixel);
    if (surface->title != NULL) {
	textWidth = XTextWidth(surface->titleFontStruct,surface->title,
	  strlen(surface->title));

	XSetFont(surface->display,surface->gc,surface->titleFontStruct->fid);
	XDrawString(surface->display,surface->pixmap,surface->gc,
	  ((surface->w)/2 -  textWidth/2),
	  surface->h - (int)(1.1*surface->titleFontHeight),
	  surface->title, strlen(surface->title));
    }


    sprintf(angleString, "x=%6.2f",RTD*surface->s3d->rotation[0]);
    XDrawString(surface->display,surface->pixmap,surface->gc,
      ANGLE_X, ANGLE_Y,angleString,8);
    sprintf(angleString, "y=%6.2f",RTD*surface->s3d->rotation[1]);
    XDrawString(surface->display,surface->pixmap,surface->gc,
      ANGLE_X, ANGLE_Y + surface->titleFontHeight, 
      angleString,8);
    sprintf(angleString, "z=%6.2f",RTD*surface->s3d->rotation[2]);
    XDrawString(surface->display,surface->pixmap,surface->gc,
      ANGLE_X, ANGLE_Y + 2*surface->titleFontHeight, 
      angleString,8);



    XCopyArea(surface->display,surface->pixmap,surface->window,
      surface->gc,0,0,surface->w,surface->h,0,0);


    XFlush( surface->display );

}




/*
 * Refresh the display
 * (don't recalcuate, just remap the pixmap onto the window)
 */

void surfaceRefresh(surface)
    Surface *surface;
{
    XCopyArea(surface->display,surface->pixmap,surface->window,surface->gc,0,0,
      surface->w,surface->h,0,0);
    XFlush(surface->display);
}




/*
 * Set the viewing angles of the Surface
 */
#ifdef _NO_PROTO
void surfaceSetView(surface, x, y, z)
    Surface *surface;
    float x, y, z;
#else
    void surfaceSetView(Surface *surface, float x, float y, float z)
#endif
{
    rotate3d(surface->s3d,x,y,z);
}


/*
 * Set the rendering mode for the surface
 * function: surfaceSetRenderMode()
 */
void surfaceSetRenderMode( surface, mode)
    Surface *surface;
    SurfaceRenderMode mode;
{

    setStyle3d(surface->s3d,mode);

}




/*
 *   Add a surface plot
 *
 *   input parameters:
 *
 *   surface - pointer to Surface data structure
 *   mesh    - mesh of data
 *   x, y    - locations of mesh lines
 *   nx, ny  - number of points in each direction
 *   foreColor, backColor, dataColor - strings for colors for
 *			foreground, background and data respectively
 *   storageMode - surface data storage mode: SurfaceInternal, SurfaceExternal
 *                      if SurfaceInternal, make a copy of the data;
 *                      otherwise (SurfaceExternal), just copy the pointers
 */

void surfaceSet( surface, mesh, x, y, nx, ny, title, titleFont,
  foreColor, backColor, dataColor, storageMode)
    Surface		*surface;
    double          *mesh, *x, *y;
    int              nx, ny;
    char		*title, *titleFont;
    char		*foreColor, *backColor, *dataColor;
    SurfaceStorageMode storageMode;
{

    XColor color, ignore;
    int i, stringLength;
    int fontCount;
    char **fontNames;
    XFontStruct *fontInfo;

    if (storageMode == SurfaceExternal) {
	surface->storageMode = SurfaceExternal;
	surface->mesh = mesh;
	surface->x    = x;
	surface->y    = y;
	surface->titleFont = titleFont;
	surface->title = title;
    }
    else {
	surface->storageMode = SurfaceInternal;
	if (surface->mesh) {
	    free( (char *) surface->mesh );
	    free( (char *) surface->x );
	    free( (char *) surface->y );
        }
	surface->mesh = (double *) malloc( (unsigned)( nx * ny * sizeof(double) ) );
	surface->x    = (double *) malloc( (unsigned)( nx * sizeof(double) ) );
	surface->y    = (double *) malloc( (unsigned)( ny * sizeof(double) ) );
	memcpy( (void *)surface->mesh, (void *)mesh,
	  (size_t)(nx * ny * sizeof(double)) );
	memcpy( (void *)surface->x, (void *)x,  (size_t)(nx * sizeof(double)) );
	memcpy( (void *)surface->y, (void *)y,  (size_t)(ny * sizeof(double)) );


	if (surface->titleFont) free( (char *) surface->titleFont);
	if (titleFont != NULL) {
	    stringLength = strlen(titleFont) + 1;
	    surface->titleFont = (char *) malloc(stringLength);
	    memcpy((void *)surface->titleFont,(void *)titleFont,(size_t)stringLength);
	    surface->titleFont[stringLength-1] = '\0';
	}

	if (surface->title) free( (char *) surface->title);
	if (title != NULL) {
	    stringLength = strlen(title) + 1;
	    surface->title = (char *) malloc(stringLength);
	    memcpy((void *)surface->title,(void *)title,(size_t)stringLength);
	    surface->title[stringLength-1] = '\0';
	}
    }
    
    surface->nx         = nx;
    surface->ny         = ny;



  /* set foreground, background and data colors based on passed-in strings */

  /* use a color for the foreground if possible */
    if ((DisplayCells(surface->display,surface->screen) > 2) &&
      (!surface->setOwnColors)) {
	if(!XAllocNamedColor(surface->display,
	  DefaultColormap(surface->display,surface->screen),
	  foreColor,&color,&ignore)){
	    fprintf(stderr,"\nsurfaceSet:  couldn't allocate color %s",foreColor);
	    surface->forePixel  = WhitePixel(surface->display,surface->screen);
	} else {
	    surface->forePixel = color.pixel;
	}

	if(!XAllocNamedColor(surface->display,
	  DefaultColormap(surface->display,surface->screen),
	  backColor,&color,&ignore)) {
	    fprintf(stderr,"\nsurfaceSet:  couldn't allocate color %s",backColor);
	    surface->backPixel  = BlackPixel(surface->display,surface->screen);
	} else {
	    surface->backPixel = color.pixel;
	}


	if(!XAllocNamedColor(surface->display,
	  DefaultColormap(surface->display,surface->screen),
	  dataColor,&color,&ignore)) {
	    fprintf(stderr,"\nsurfaceSet:  couldn't allocate color %s",dataColor);
	    surface->dataPixel  = WhitePixel(surface->display,surface->screen);
	} else {
	    surface->dataPixel = color.pixel;
	}

    } else {
	surface->forePixel  = WhitePixel(surface->display,surface->screen);
	surface->backPixel  = BlackPixel(surface->display,surface->screen);
	surface->dataPixel  = WhitePixel(surface->display,surface->screen);
    }

  /*
   * set the 3d parameters
   */
    meshToFacets3d(surface->s3d,surface->nx,surface->ny,
      surface->mesh,surface->x,surface->y);
    set3d(surface->s3d,surface->display,surface->window,
      surface->pixmap,surface->gc,surface->forePixel,
      surface->backPixel, surface->dataPixel);


  /*
   * load the fonts
   */
    if (!surface->setOwnFonts) {
	if ((surface->titleFontStruct=XLoadQueryFont(surface->display,
	  surface->titleFont)) == NULL) {
	    fprintf(stderr,"surfaceInit: display %s doesn't know font %s\n",
	      DisplayString(surface->display),surface->titleFont);
	    fprintf(stderr,"\t using first available font\n");
	    fontNames = XListFontsWithInfo(surface->display,FONTWILDCARD,
	      MAXFONTS,&fontCount,&fontInfo);
	    surface->titleFontStruct = XLoadQueryFont(surface->display,fontNames[0]);
	    XFreeFontInfo(fontNames,fontInfo,fontCount);
	    XFreeFontNames(fontNames);
	}

	surface->titleFontHeight = surface->titleFontStruct->ascent +
	  surface->titleFontStruct->descent;
	surface->titleFontWidth = surface->titleFontStruct->max_bounds.rbearing -
	  surface->titleFontStruct->min_bounds.lbearing;

    }

}


/*
 * Terminate the Surface
 * function:  void surfaceTerm()
 */

void surfaceTerm(surface)
    Surface *surface;
{
  /***
  *** free the space used and referenced by the surface
  ***/

  /*
   * if we don't free this pixmap, the program will mysteriously
   * remember the last pixmap and use it (the next call to XCreatePixmap
   * of a given size digs up that last one (or part of it if not same size)
   * and uses it
   */
    XFreePixmap(surface->display,surface->pixmap);

  /* likewise, free the graphics context and font structures */
    XFreeGC(surface->display,surface->gc);
    if (!surface->setOwnFonts)
      XFreeFont(surface->display,surface->titleFontStruct);


  /* and free these guys if SurfaceInternal data storage */
    if (surface->storageMode == SurfaceInternal) {
	free( (char *) surface->titleFont);
	free( (char *) surface->title);
	free( (char *) surface->mesh);
	free( (char *) surface->x);
	free( (char *) surface->y);
    }

  /* free storage used by 3d structures */
    term3d(surface->s3d);

    free( (char *) surface);

}



/*
 *   Print (dump in xwd format, then pipe to `xpr -device ps' then pipe
 *   to printer defined by PSPRINTER environment variable...)
 *
 *   input parameters:
 *
 *   surface        - pointer to Surface data structure
 */
void surfacePrint(surface)
    Surface *surface;
{

    utilPrint(surface->display, surface->window, XWD_TEMP_FILE_STRING);

}
