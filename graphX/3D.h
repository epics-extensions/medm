/*
 *
 * 7 Jan 1990 - MDA
 *
 * algorithm and shaded polygon drawing code adapted from: 
 *		Digital Equipment Corporation
 *  		Nick Williams, IED/Reading
 *
 *
 */	

#ifndef __3D_H__
#define __3D_H__

#include	<stdio.h>
#include	<math.h>

#include	<X11/X.h>
#include	<X11/Xlib.h>
#include	<X11/Xutil.h>
#include	<X11/Intrinsic.h>

#ifndef min
#  define min(a,b) ((a) < (b) ? a : b)
#endif
#ifndef max
#  define max(a,b) ((a) > (b) ? a : b)
#endif

#define	MAX_COLORS	100
#define	RGB_DEFAULT	100
#define	RGB_SCALE_MAX	100
#define	MAXCORNERS	4	/* max vertices per facet (quadrilateral mesh)*/
#define	MAXDISTANCE	100.0
#define	PI		3.14159
#define	DTR		PI / 180.0
#define	RTD		180.0 / PI
#define SOLID		0
#define SHADED		1


/* function macros */
#define 	ABS(A)		((A) >= 0.0 ? (A) : -(A))


/* structures for 3D */
 
struct depths {				/* structure  used to depth */
	float zmax;			/* sort facets		    */
	int facet;
	};


struct _3d {

  Display *display;
  Window window;
  XWindowAttributes wind_struct;
  GC gc;
  Pixmap pixmap;			/* pixmap backing store */

  unsigned long forePixel, backPixel, dataPixel;

  float xMin, xMax, xDelta;		/* need these to go from normalized  */
  float yMin, yMax, yDelta;		/*  data [0,1] and back again        */
  float zMin, zMax, zDelta;


/* 3D data structures */

  int nverts;				/* total number of vertices */
  int nfacets;				/* total number of facets */
  int draw_mode;			/* SOLID or SHADED */
  int **facets;				/* vertices in facet */
  int *ncorners;			/* number of vertices / facet */
  float **vertices;			/* 3D vertex positions */
  float **out_verts;			/* xformed 3D vertex positions */
  float obj_box[2][3];			/* object bounding box */
  float screen_box[2][3];		/* screen space bounding box */

  struct depths *facet_depth;


/* colour map and parameters */

  Colormap map;
  XColor *colours;
  int red_level;
  int blue_level;
  int green_level;
  int num_colours;
  unsigned long pixels[MAX_COLORS];


/* Matrix which maps world coordinates to ndc coordinates */
  float world_to_ndc[4][4];

/* Matrix which maps normalised device coor. to  screen pixel coor. */
  float ndc_to_screen[4][4];	

/* Matrix product of ndc_to_screen x world_to_ndc 
	(maps world coordinates to screen pixels */
  float world_to_screen[4][4];

/* Concatenated viewing transformation - maps world coordinates to screen 
	pixels i.e. the whole viewing pipeline */
  float cvt[4][4];

/* Current transformation matrix - operates on world coordinates producing 
	transformed world coordinates */
  float ctm[4][4];

/*  parameters for rotation, translation, and scaling parameters */
  float	rotation[3];
  float	translation[3];
  float	scaling[3];

};

typedef struct _3d ThreeD;


#ifdef _NO_PROTO
extern ThreeD *init3d();
extern void meshToFacets3d();
extern void rotate3d();
extern void setPixmap3d();
extern void setStyle3d();
extern void set3d();
extern void draw3d();
extern void term3d();
#else
extern ThreeD *init3d(void);
extern void meshToFacets3d(ThreeD *, int , int , double *, double *, double *);
extern void rotate3d(ThreeD *, float, float, float);
extern void setPixmap3d(ThreeD *, Pixmap);
extern void setStyle3d(ThreeD *, int);
extern void set3d(ThreeD *, Display *, Window, Pixmap, GC, unsigned long,
	unsigned long, unsigned long);
extern void draw3d(ThreeD *);
extern void term3d(ThreeD *);
#endif

#endif /*__3D_H__ */
