/*
 *
 *   7 Jan 1990 - MDA
 *
 *   algorithm and shaded polygon drawing code adapted from: 
 *
 *  		Nick Williams, IED/Reading
 *		Digital Equipment Corporation
 *
 *
 */	

#include	<float.h>
#include	"3D.h"



ThreeD *init3d()
{
    ThreeD *s3d;

    s3d = (ThreeD *) malloc( (unsigned) sizeof(ThreeD));

    s3d->draw_mode = SOLID;
    s3d->num_colours = MAX_COLORS;

    s3d->rotation[0] = 45.0;
    s3d->rotation[1] = 0.0;
    s3d->rotation[2] = 45.0;

    s3d->translation[0] = 0.0;
    s3d->translation[1] = 0.0;
    s3d->translation[2] = 0.0;

    s3d->scaling[0] = 0.6;
    s3d->scaling[1] = 0.6;
    s3d->scaling[2] = 0.6;

    return(s3d);
}




void setStyle3d(s3d,style)
    ThreeD *s3d;
    int style;
{
    s3d->draw_mode = style;
}



#ifdef _NO_PROTO
void rotate3d(s3d,x,y,z)
    ThreeD *s3d;
    float x,y,z;
#else
    void rotate3d(ThreeD *s3d, float x, float y, float z)
#endif
{
    s3d->rotation[0] = DTR * x;
    s3d->rotation[1] = DTR * y;
    s3d->rotation[2] = DTR * z;

}


void meshToFacets3d(s3d,xSize,ySize,mesh,xArray,yArray)
    ThreeD *s3d;
    int xSize, ySize;
    double *mesh, *xArray, *yArray;
{
    int i, j, k;

  /* according to XPG2's values.h */
    s3d->xMin = FLT_MAX; s3d->xMax = FLT_MIN;
    s3d->yMin = FLT_MAX; s3d->yMax = FLT_MIN;
    s3d->zMin = FLT_MAX; s3d->zMax = FLT_MIN;

  /*
   * # vertices, # facets         (simple since quadrilateral mesh..)
   */
    s3d->nverts = xSize*ySize;
    s3d->nfacets = (xSize-1)*(ySize-1);

    s3d->facets = (int **) malloc((unsigned) s3d->nfacets * sizeof(int *));
    for (i = 0; i < s3d->nfacets; i++)
      s3d->facets[i] = (int *) malloc((unsigned) (MAXCORNERS*sizeof(int)));

    s3d->ncorners = (int *) malloc((unsigned) s3d->nfacets * sizeof(int));

    s3d->vertices = (float **) malloc((unsigned) s3d->nverts * sizeof(float *));
    for (i = 0; i < s3d->nverts; i++)
      s3d->vertices[i] = (float *) malloc((unsigned) 3 * sizeof(float));
  
    s3d->out_verts = (float **) malloc((unsigned) s3d->nverts * sizeof(float *));
    for (i = 0; i < s3d->nverts; i++)
      s3d->out_verts[i] = (float *) malloc((unsigned) 3 * sizeof(float));

#define FACET_FACTOR 2
    s3d->facet_depth = (struct depths *) malloc((unsigned) 
      FACET_FACTOR*s3d->nfacets * sizeof(struct depths *));



  /*
   * vertices
   */

  /*  normalise the x,y,z values for the vertices: find the ranges */

    for (i = 0; i < xSize; i++) {
        s3d->xMin = min(s3d->xMin, xArray[i]); 
	s3d->xMax = max(s3d->xMax, xArray[i]);
    }
    for (j = 0; j < ySize; j++) {
        s3d->yMin = min(s3d->yMin, yArray[j]); 
	s3d->yMax = max(s3d->yMax, yArray[j]);
    }
    for (i = 0; i < xSize; i++)
      for (j = 0; j < ySize; j++) {
	  s3d->zMin = min(s3d->zMin, mesh[i + j*xSize]);
	  s3d->zMax = max(s3d->zMax, mesh[i + j*xSize]);
      }
    s3d->xDelta = s3d->xMax - s3d->xMin; 
    s3d->yDelta = s3d->yMax - s3d->yMin; 
    s3d->zDelta = s3d->zMax - s3d->zMin;


  /* and use the "normalised" vertices */
    k = 0;
    for (i = 0; i < xSize; i++)
      for (j = 0; j < ySize; j++) {
	  s3d->vertices[k][0] = (xArray[i]-s3d->xMin)/s3d->xDelta;
	  s3d->vertices[k][1] = (yArray[j]-s3d->yMin)/s3d->yDelta;
	  s3d->vertices[k][2] = (mesh[i + j*xSize]-s3d->zMin)/s3d->zDelta;
	  k++;
      }


  /*
   * facets
   */

    k = 0;

    if (((xSize+ySize)%2) == 0) {			/* Rows+Columns == Even */
	for (i = 0; i < xSize-1; i++)
	  for (j = 0; j < ySize-1; j++) {
	      s3d->ncorners[k] = MAXCORNERS;		/* since quadrilateral mesh */
	      s3d->facets[k][0]  = i + j*xSize;
	      s3d->facets[k][1] = (i+1) + j*xSize,
		s3d->facets[k][2] = (i+1) + (j+1)*xSize,
		s3d->facets[k][3]  = i + (j+1)*xSize;
	      k++;
	  };
    } else {                                      /* Rows+Columns == Odd */
	for (i = 0; i < xSize-1; i++)
	  for (j = 0; j < ySize-1; j++) {
	      s3d->ncorners[k] = MAXCORNERS;		/* since quadrilateral mesh */
	      s3d->facets[k][0]  = j + i*ySize;
	      s3d->facets[k][1] = (j+1) + i*ySize;
	      s3d->facets[k][2] = (j+1) + (i+1)*ySize;
	      s3d->facets[k][3] = j + (i+1)*ySize;
	      k++;
	  }

    }

}



void setPixmap3d(s3d,pixmap)
    ThreeD *s3d;
    Pixmap pixmap;
{

    s3d->pixmap = pixmap;

    get_obj_box(s3d,s3d->nverts,s3d->vertices,s3d->obj_box);

  /*
   *  fetch the window attributes
   */
    if( XGetWindowAttributes(s3d->display, s3d->window, &(s3d->wind_struct)) == 0)
      fprintf(stderr,"Can't get attributes for drawing window\n");
  
  /*
   *  Initialise our 3D graphics transforms
   *  based on size of drawing window  
   */
    set_screen_box(s3d,s3d->screen_box);

  /* 
   * set up the necessary transformation matrices
   */
    initialise_3d(s3d,s3d->obj_box, s3d->screen_box);

}


void set3d(s3d,display,drawWindow,pixmap,gc,forePixel,backPixel,dataPixel)
    ThreeD *s3d;
    Display *display;
    Window drawWindow;
    Pixmap pixmap;
    GC gc;
    unsigned long forePixel, backPixel, dataPixel;
{

    s3d->window = drawWindow;
    s3d->display = display;
    s3d->forePixel = forePixel;
    s3d->backPixel = backPixel;
    s3d->dataPixel = dataPixel;


  /* initialise colour data structures etc */
    init_colour(s3d,display,drawWindow,&(s3d->map));
        
  /* Set up the colour map */
    load_colour_luts(s3d,display, s3d->map, &(s3d->num_colours));

    s3d->red_level = s3d->green_level = s3d->blue_level = RGB_DEFAULT;
    new_colour_luts(s3d,display,s3d->map,s3d->num_colours);


    s3d->gc = gc;

  /*
   * set the pixmap and related transformations
   */
    setPixmap3d(s3d,pixmap);

}




/* set size of screen 'space' using window dimensions */

set_screen_box(s3d,box)
    ThreeD *s3d;
    float box[2][3];
{
    s3d->screen_box[0][0] = 0;				/* min */
    s3d->screen_box[1][0] = s3d->wind_struct.width;		/* max */

  /* note origin forced to bottom (left) */
    s3d->screen_box[0][1] = s3d->wind_struct.height;
    s3d->screen_box[1][1] = 0;

  /* default Z value is window height */
    s3d->screen_box[0][2] = 0;
    s3d->screen_box[1][2] = s3d->wind_struct.height;
}





/*
 *  Get the 3D extent of a given set of vectors
 *  Note the box is stored as box[min-max][X-Y-Z] 
 */


get_obj_box(s3d,nverts,vertices,box)
    ThreeD *s3d;
    int nverts;
    float vertices[][3],box[2][3];
{
    int i,j;

  /* initial min,max values */
    for(j=0;j<3;j++){
	box[0][j] = box[1][j] = vertices[0][j];
    }

    for(i=0;i<nverts;i++){
	for(j=0;j<3;j++){
	  /* get minimum */
	    if(vertices[i][j] < box[0][j])
	      box[0][j] = vertices[i][j];
	  /* get maximum */
	    if(vertices[i][j] > box[1][j])
	      box[1][j] = vertices[i][j];
	}
    }

}






/*
 *  Perform simple hidden surface elimination by depth
 *  sorting the polygons to be drawn
 */

draw_hidden(s3d)
    ThreeD *s3d;
{
    int i, j, facet_id, vert_id, x1, y1, x2, y2;
    int npoints, mode, shape;
    int compare();
    float in_vec[4];
    float out_vec[4];
    float max_depth;
    XPoint points[100];

  /* clear the pixmap prior to drawing */
    XSetForeground(s3d->display, s3d->gc, s3d->backPixel);
    XFillRectangle(s3d->display, s3d->pixmap, s3d->gc, 0, 0,
      s3d->wind_struct.width, s3d->wind_struct.height);

  
    transform_vertices(s3d->nverts, s3d->cvt, s3d->vertices, s3d->out_verts);

    mode = CoordModeOrigin;

  /* simple shading for b/w screens */
    XSetFillStyle(s3d->display, s3d->gc, FillSolid);
  
  /* assume all poygons are non-convex for safety */  
    shape = Nonconvex;

  /* get the maximum depth for each facet */
    for(i = 0; i < s3d->nfacets; i++){

      /* get the number of corners for current facet */
	npoints = s3d->ncorners[i];
	max_depth = -10000.0;

      /* get the coordinates for vertices of this facet */
	for(j = 0; j < npoints; j++){
	    vert_id = s3d->facets[i][j];
	    if(s3d->out_verts[vert_id][2] > max_depth) 
	      max_depth = s3d->out_verts[vert_id][2];
	}

      /* record facet depth and number */
	s3d->facet_depth[i].zmax = max_depth;
	s3d->facet_depth[i].facet = i;
    }

  /* depth sort the facets */
    qsort(s3d->facet_depth, s3d->nfacets, sizeof(struct depths), compare);

  /* draw each facet in turn */
    for(i = 0; i < s3d->nfacets; i++){
  
      /* pick the facet from the sorted list */
	facet_id = s3d->facet_depth[i].facet;

      /* get the number of corners for current facet */
	npoints = s3d->ncorners[facet_id];

      /* get the coordinates for vertices of this facet */
	for(j = 0; j < npoints; j++){
 
	    vert_id = s3d->facets[facet_id][j];
  
	  /* map to 2D */  
	    points[j].x = s3d->out_verts[vert_id][0];
	    points[j].y = s3d->out_verts[vert_id][1];
	}

      /* replicate first point to join end to start */
	points[npoints].x = points[0].x;
	points[npoints].y = points[0].y;


      /* draw polygon interior */  
	XSetForeground(s3d->display, s3d->gc, s3d->backPixel);
	XFillPolygon(s3d->display, s3d->pixmap, s3d->gc, points, 
	  npoints + 1, shape, mode);

      /* draw in the boundary */
	XSetForeground(s3d->display, s3d->gc, s3d->dataPixel);
	XDrawLines(s3d->display, s3d->pixmap, s3d->gc, points,
	  npoints + 1, mode);

    }


}

/*
 *  Draw hidden surface model with shaded surface
 */

draw_shaded(s3d)
    ThreeD *s3d;
{
    int i, j, facet_id, x1, y1, x2, y2;
    int vert_id;
    int vert_ids[MAXCORNERS];
    int npoints, mode, shape;
    int compare();
    int icolour;
    float in_vec[4];
    float out_vec[4];
    float max_depth;
    float norm[3];
    float lvec[3];
    float dotp;
    float c_amb;
    float c_diff;
    XPoint points[100];
    unsigned long colour;

    float dot_product();


    lvec[0] = 0;
    lvec[1] = 0;
    lvec[2] = -1;

    c_amb = 0.3;
    c_diff = 0.7;


  /* clear the pixmap prior to drawing */
    XSetForeground(s3d->display, s3d->gc, s3d->backPixel);
    XFillRectangle(s3d->display, s3d->pixmap, s3d->gc, 0, 0,
      s3d->wind_struct.width, s3d->wind_struct.height);

  
    transform_vertices(s3d->nverts, s3d->cvt, s3d->vertices, s3d->out_verts);

    mode = CoordModeOrigin;

    XSetFillStyle(s3d->display, s3d->gc, FillSolid);

  /* assume all poygons are non-convex for safety */  
    shape = Nonconvex;

  /* get the maximum depth for each facet */
    for(i = 0; i < s3d->nfacets; i++){

      /* get the number of corners for current facet */
  
	npoints = s3d->ncorners[i];

	max_depth = -10000.0;
      /* get the coordinates for s3d->vertices of this facet */
	for(j = 0; j < npoints; j++){
 
	    vert_id = s3d->facets[i][j];
  
	    if(s3d->out_verts[vert_id][2] > max_depth)
	      max_depth = s3d->out_verts[vert_id][2];
	}

      /* record facet depth and number */
	s3d->facet_depth[i].zmax = max_depth;
	s3d->facet_depth[i].facet = i;
    }

  /* depth sort the facets */
    qsort(s3d->facet_depth, s3d->nfacets, sizeof(struct depths), compare);



  /* draw each facet in turn */
    for(i = 0; i < s3d->nfacets; i++){
  
      /* pick the facet from the sorted list */
	facet_id = s3d->facet_depth[i].facet;

      /* get the number of corners for current facet */
	npoints = s3d->ncorners[facet_id];

      /* get the coordinates for vertices of this facet */
	for(j = 0; j < npoints; j++){
 
	    vert_id = vert_ids[j] = s3d->facets[facet_id][j];
  
	  /* map to 2D */  
	    points[j].x = s3d->out_verts[vert_id][0];
	    points[j].y = s3d->out_verts[vert_id][1];
	}

      /* replicate first point to join end back to start */
	points[npoints].x = points[0].x;
	points[npoints].y = points[0].y;

      /* 
       *  use the first three points on this facet to
       *  determine the surface normal
       */


	surf_normal(&(s3d->out_verts[vert_ids[0]][0]),
	  &(s3d->out_verts[vert_ids[1]][0]),
	  &(s3d->out_verts[vert_ids[2]][0]),
	  norm);


      /* convert to  unit vector */
	normalise(norm);  
    
      /* Lambertian surface shading */
	dotp = dot_product(lvec,norm);  



      /* simple Lambertian shading model */
	colour = s3d->num_colours * (ABS(dotp) * c_diff + c_amb);  

	XSetForeground(s3d->display, s3d->gc, colour);

	XFillPolygon(s3d->display, s3d->pixmap, s3d->gc, points, 
	  npoints + 1, shape, mode);



    }

  /* finally copy the buffer into the s3d->window */  

  /*** (MDA)
    XCopyArea(s3d->display, s3d->pixmap, s3d->window, s3d->gc,
    0, 0,
    s3d->wind_struct.width, s3d->wind_struct.height,
    0, 0);
    ***/

}


/*
 *   Comparison procedure used by quicksort to sort facet depths
 */

int compare(a,b)
    struct depths *a,*b;
{

    if(a->zmax < b->zmax)
      return(-1);
    else if(a->zmax > b->zmax)
      return(1);
    else
      return(0);
}


/*
 *  Transform vertex list using given transformation matrix
 */

transform_vertices(nverts, mat, in_verts, out_verts)
    int nverts;
    float mat[4][4], **in_verts, **out_verts;
{
    int i;
    float in_vec[4], out_vec[4];

    for(i = 0; i < nverts; i++){
 
	in_vec[0] = in_verts[i][0];
	in_vec[1] = in_verts[i][1];
	in_vec[2] = in_verts[i][2];
	in_vec[3] = 1.0;
  
      /* apply the transformation */  
	vec_mat(in_vec, mat, out_vec);
      
	out_verts[i][0] = out_vec[0];
	out_verts[i][1] = out_vec[1];
	out_verts[i][2] = out_vec[2];
      
    }

}

/*
 *  Initialise the 3D graphics transforms
 *  Maps a space enclosing box to screen
 *  pixels. The box usually enloses the
 *  object to be displayed which is then
 *  mapped to pixels in the range
 *  (sxmin,sxmax) and (symin,symax)
 */

initialise_3d(s3d, box, screen_box)
    ThreeD *s3d;
    float box[2][3];
    float screen_box[2][3];
{

    get_world_to_ndc_matrix(box, s3d->world_to_ndc);
    get_ndc_to_screen_matrix(screen_box, s3d->ndc_to_screen);
    mat_mult(4,s3d->ndc_to_screen, s3d->world_to_ndc, s3d->world_to_screen);

  /* initally viewing transform is == world_to_screen */
    mat_copy(4, 4, s3d->world_to_screen, s3d->cvt);

  /* current transformation is null */
    mat_id(4, s3d->ctm);
}


/*
 *  Translate bounding box to 0,0,0. Then
 *  scale to ndc space (+1,-1) along X,Y,Z
 *  axes. Note that all dimensions are
 *  scaled by the same factor - this avoids
 *  stretching/contraction along one axis.
 */

get_world_to_ndc_matrix(box, mat)
    float box[2][3], mat[4][4];
{
    float box_centre[3], side, scaling[3], transl[3];
    float scale_mat[4][4];
    float trans_mat[4][4];
    float max_side;
    static float ndc_size = 2.0;
    int i;


    max_side = box[1][0] - box[0][0];
    for(i=0;i<3;i++){
  
	box_centre[i] = (box[0][i] + box[1][i] ) * 0.5;
	transl[i] = -box_centre[i];

	side = box[1][i] - box[0][i];  
	if(side > max_side)
	  max_side = side;
  
    }
  
    if(max_side != 0.0){
	scaling[0] = scaling[1]  = scaling[2] = ndc_size / max_side;
    }
    else{
	scaling[0] = scaling[1]  = scaling[2] = 1.0; 
    }


    xform_3D_translate(transl[0],transl[1],transl[2], trans_mat);
    xform_3D_scale(scaling[0],scaling[1],scaling[2], scale_mat);

    mat_mult(4,trans_mat, scale_mat, mat);

}


/*
 *  Get the ndc to screen (pixel matrix). Note that for consistency,
 *  the screen is treated as a 3D object - i.e. a space rather than
 *  a plane. This allows 3D operations (e.g. rendering) to be defined 
 *  on 'screen coordinates, if desired.
 *
 *  Parameters define screen dimensions
 *  Z coordinate of screen assumed to be
 *  at 0.0 (i.e. at origin)
 */

get_ndc_to_screen_matrix(screen_box,mat)
    float screen_box[2][3];  /* [min,max][X,Y,Z] */
    float mat[4][4];
{
    int i;
    float scaling[3],transl[3];
    float scale_mat[4][4];
    float trans_mat[4][4];
    static float ndc_size = 2.0;


    for(i=0;i<3;i++){
      /* work out scaling to blow ndc space up to screen size */

	scaling[i] = (screen_box[1][i] - screen_box[0][i]) /
	  ndc_size;
    
      /* centre of screen - new origin */
	transl[i] = (screen_box[1][i] + screen_box[0][i]) /
	  2.0;
    }

    xform_3D_scale(scaling[0],scaling[1],scaling[2], scale_mat);
    xform_3D_translate(transl[0],transl[1],transl[2], trans_mat);

    mat_mult(4, scale_mat, trans_mat, mat);
  
}




/*
 *  Calculate numerical values to insert into colour tables
 */

new_colour_luts(s3d,display,colorMap,num_colours)
    ThreeD *s3d;
    Display *display;
    Colormap colorMap;
    int num_colours;
{

    static int allcolours = DoRed | DoGreen | DoBlue;
    int i;
    float fac;

    if(num_colours <= 0){
	fprintf(stderr,"new_colour_luts: num_colours <= 0\n");
	return;
    }

    fac = (65535 / (float)num_colours) / (float)RGB_SCALE_MAX;

    for (i=0; i < num_colours ; i++) {
	s3d->colours[i].flags = allcolours;
	s3d->colours[i].red = s3d->red_level * fac * i; 
	s3d->colours[i].green = s3d->green_level * fac * i;
	s3d->colours[i].blue = s3d->blue_level * fac * i;
    }

    XStoreColors(display,colorMap,s3d->colours,num_colours);
}


/*
 *  Set up the colour look-up tables
 */

init_colour(s3d,display,window,colorMap)
    ThreeD *s3d;
    Display *display;
    Window window;
    Colormap *colorMap;
{
    int nVis;
    XVisualInfo *pVisualInfo;
    XVisualInfo  VisualInfo_dummy;


    VisualInfo_dummy.visualid = (DefaultVisual(display,0))->visualid;
  
    pVisualInfo = XGetVisualInfo(display,
      VisualIDMask,&(VisualInfo_dummy),&(nVis) );

    if (pVisualInfo->class != PseudoColor &&
      pVisualInfo->class != DirectColor &&
      pVisualInfo->class != GrayScale) {

	fprintf(stderr,"Unable to use this visual class\n");
	exit(0);
    }

    if (s3d->num_colours > pVisualInfo->colormap_size)
      s3d->num_colours = pVisualInfo->colormap_size-2;
  
    *colorMap = XDefaultColormap(display, DefaultScreen(display));

    XSetWindowColormap(display,window,*colorMap);
    s3d->colours = (XColor *) malloc((unsigned) 
      sizeof(*s3d->colours) * s3d->num_colours);

}


  
/* 
 *  reserve colour cells for use by this application 
 *   pixel values are returned in 'pixels' 
 */

load_colour_luts(s3d,display,colorMap,count)
    ThreeD *s3d;
    Display *display;
    Colormap colorMap;
    int *count;
{
    unsigned int i,j,fullcount;
    static unsigned int nplanes = 0;
    float h,r,g,b;
    int rx,gx,bx;
    unsigned long dummy;
    int black;
    int white;
    unsigned int ncells;
  

    ncells = *count;

  /* try to allocate requested number of colours */
  /* if too many, decrement, and try again */

    while (!XAllocColorCells(display,
      s3d->map, 0, &dummy, nplanes, s3d->pixels, ncells) && ncells >= 1) {
	ncells--;
    }

    if (ncells < 1) {
	fprintf(stderr,"Could not allocate enough colours");
	exit(0);
    }

  /* find out if any of the colours include WhitePixel || BlackPixel */
  /* if they do, re-allocate */

    fullcount = ncells;

    black = BlackPixel(display, DefaultScreen(display));
    white = WhitePixel(display, DefaultScreen(display));

    for (j=0,i=0; i < fullcount; i++) {

	if (s3d->pixels[i] == black || s3d->pixels[i] == white) {

	    if (XAllocColorCells(display,
	      s3d->map,0,&dummy,0,&(s3d->pixels[i]),(unsigned int)1)){
		i--; /*Got a replacement value; try it again*/
	    }
	    else{
		ncells--; /*No replacement; forget this one*/
	    }  
	}
        else {
	    s3d->colours[j++].pixel = s3d->pixels[i];
	}
    }

    if (ncells < 1) {
	fprintf(stderr,"Could not allocate enough colours");
	exit(0);
    }

  /*This loop fills in the remaining colours*/


    for (i=0; i<ncells ; i++) {
	s3d->colours[i].flags = DoRed | DoGreen | DoBlue;
	s3d->colours[i].red =   0;
	s3d->colours[i].green = 0;
	s3d->colours[i].blue =  0;
    }
    
    XStoreColors(display,colorMap,s3d->colours,ncells);

    new_colour_luts(s3d,display,colorMap,ncells);

    *count = ncells;
}






/*
 *  The viewing transformations are applied in NDC space
 *  Thus the viewing pipeline is:
 *    1)  world to NDC
 *    2)  current transformations
 *    3)  ndc to screen
 *  Note that, 1) and 3) remain constant while 2) varies
 *  according to the prevailing settings
 */


void draw3d(s3d)
    ThreeD *s3d;
{
    float temp[4][4];

  /* form the new ctm matrix */ 

    xform_3D(s3d->rotation[0],s3d->rotation[1],s3d->rotation[2],
      s3d->translation[0],s3d->translation[1],s3d->translation[2],
      s3d->scaling[0],s3d->scaling[1],s3d->scaling[2], s3d->ctm );

  /* now the whole pipeline - concatenated viewing transform */

    mat_mult(4, s3d->ctm, s3d->ndc_to_screen, temp );
    mat_mult(4, s3d->world_to_ndc, temp, s3d->cvt);

  /* use the concatenated viewing transform to draw object */


    switch(s3d->draw_mode){

    case SOLID:  
	draw_hidden(s3d);
	break;
  
    case SHADED:  
	draw_shaded(s3d);
	break;
    }

}


void term3d(s3d)
    ThreeD *s3d;
{
    int i;


  /*** free memory ***/

    for (i = 0; i < s3d->nfacets; i++)
      free(s3d->facets[i]);
    free(s3d->facets);
    free(s3d->ncorners);
    for (i = 0; i < s3d->nverts; i++)
      free(s3d->vertices[i]);
    free(s3d->vertices);
    for (i = 0; i < s3d->nverts; i++)
      free(s3d->out_verts[i]);
    free(s3d->out_verts);
    free(s3d->facet_depth);
    free(s3d->colours);
    free(s3d);

  /*** and free X resources ***/


    exit(0);
}






/*
 *  3D graphics transformations
 *
 */



/*
 *  Form the concatenated 3D transformation matrix
 *  Transformations occur in the order:scaling,
 *  rotation, translation. 
 */

xform_3D(rx, ry, rz, tx, ty, tz, sx, sy, sz, ctm)
    float rx, ry, rz, tx, ty, tz, sx, sy, sz, ctm[4][4];
{
    float smat[4][4],
      tmat[4][4],
      rmat[4][4],
      mat1[4][4],
      mat2[4][4];

    mat_id(4, ctm);
    xform_3D_scale(sx, sy, sz, smat);
    xform_3D_translate(tx, ty, tz, tmat);
    xform_3D_rotate(rx, ry, rz, rmat);

    mat_mult(4, ctm, smat, mat1);
    mat_mult(4, rmat, mat1, mat2);
    mat_mult(4, tmat, mat2, ctm);
}


/*
 *  Form the concatenated 3D rotation matrix 
 */

xform_3D_rotate(rx, ry, rz, rmat)
    float rx, ry, rz, rmat[4][4];
{
    float rxmat[4][4],
      rymat[4][4],
      rzmat[4][4],
      m1[4][4];

    xform_3D_rotx(rx, rxmat);
    xform_3D_roty(ry, rymat);
    xform_3D_rotz(rz, rzmat);

    mat_mult(4, rymat, rxmat, m1);
    mat_mult(4, rzmat, m1, rmat);
}


/*
 *  Form the X axis rotation matrix
 */

xform_3D_rotx(theta, mat) 
    float theta, mat[4][4];
{
    int i, j;
    float cos_theta, sin_theta;

    cos_theta = cos(theta);
    sin_theta = sin(theta);
    for(i=0;i<4;i++)
      for(j=0;j<4;j++)
	mat[i][j] = 0.0;

    mat[0][0] = mat[3][3] = 1.0;
    mat[1][1] = mat[2][2] = cos_theta;
    mat[2][1] = sin_theta;
    mat[1][2] = -sin_theta;;
}


/*
 *  Form the Y axis rotation matrix
 */

xform_3D_roty(theta,mat) 
    float theta,mat[4][4];
{
    int i,j;
    float cos_theta, sin_theta;

    cos_theta = cos(theta);
    sin_theta = sin(theta);
    for(i=0;i<4;i++)
      for(j=0;j<4;j++)
	mat[i][j] = 0.0;

    mat[1][1] = mat[3][3] = 1.0;
    mat[0][0] = mat[2][2] = cos_theta;
    mat[2][0] = -sin_theta;
    mat[0][2] = sin_theta;;
}


/*
 *  Form the Z axis rotation matrix
 */

xform_3D_rotz(theta, mat) 
    float theta, mat[4][4];
{
    int i, j;
    float cos_theta, sin_theta;

    for(i=0;i<4;i++)
      for(j=0;j<4;j++)
	mat[i][j] = 0.0;

    cos_theta = cos(theta);
    sin_theta = sin(theta);

    mat[2][2] = mat[3][3] = 1.0;
    mat[0][0] = mat[1][1] = cos_theta;
    mat[1][0] = sin_theta;
    mat[0][1] = -sin_theta;
}

/*
 *  Form the 3D scaling matrix
 */

xform_3D_scale(sx, sy, sz, mat)
    float sx, sy, sz, mat[4][4];
{
    int i, j;


    for(i=0;i<4;i++)
      for(j=0;j<4;j++)
	mat[i][j] = 0.0;

    mat[0][0] = sx;
    mat[1][1] = sy;
    mat[2][2] = sz;
    mat[3][3] = 1.0;
}

/*
 *  Form the 3D translation matrix
 */

xform_3D_translate(tx, ty, tz, mat)
    float tx, ty, tz, mat[4][4];
{
    int i, j;


    for(i=0;i<4;i++){
	for(j=0;j<4;j++){
	    mat[i][j] = (i == j ? 1.0 : 0.0);
	}
    }

    mat[3][0] = tx;
    mat[3][1] = ty;
    mat[3][2] = tz;
}



/*
 *  Matrix multiply for square matrices only
 *  Result matrix is returned in 'c'
 */

mat_mult(siz, mata, matb, matc)
    float *mata, *matb, *matc;
    int siz;
{
    int i, j, k;
    float sum;

    for(i=0;i<siz;i++){
	for(j=0;j<siz;j++){
	    sum = 0.0;
	    for(k=0;k<siz;k++){
		sum += *(mata + i * siz + k) *
		  *(matb + k * siz + j);
	    }
	    *(matc + i * siz + j) = sum;
	}
    }
}


/*
 *  Form the identity matrix
 *  mat[i][j] = 1.0  if i == j, 0.0 otherwise
 */

mat_id(siz,mat)
    int siz;
    float *mat;
{
    int i,j;
    float *ptr;

    ptr = mat;
    for(i=0;i<siz;i++)
      for(j=0;j<siz;j++)
	*ptr++ = (i == j ? 1.0 : 0.0);
}

/*
 *  Copy one matrix to another
 */

mat_copy(rows,cols,in,out)
    int rows, cols;
    float *in, *out;
{
    int i,j;
    float *ip, *op;

    ip = in; 
    op = out;
    for(i=0;i<rows;i++){
	for(j=0;j<cols;j++){
	    *op++ = *ip++;
	}
    }

}


/*
 *   Vector matrix multiplication for 4 component vectors
 */

vec_mat(veca, mata, vecb)
    float veca[4], mata[4][4], vecb[4];
{
    int j, k;

    for(j=0;j<4;j++){
	vecb[j] = 0.0;
	for(k=0;k<4;k++){
	    vecb[j] += veca[k] * mata[k][j];
	}
    }
}


/*
 *   Matrix vector multiplication for 4 component vectors
 */

mat_vec(mata, veca, vecb)
    float mata[4][4], veca[4], vecb[4];
{
    int j, k;

    for(j=0;j<4;j++){
	vecb[j] = 0.0;
	for(k=0;k<4;k++){
	    vecb[j] += mata[j][k] * veca[k];
	}
    }


}


/*
 *  Print out a matrix using a fixed format
 */

print_mat(rows,cols,mat)
    int rows,cols;
    float *mat;
{
    int i,j;
    float *ptr;

    ptr = mat;
    for(i=0;i<rows;i++){
	for(j=0;j<cols;j++){
	    fprintf(stderr,"%6.4f ",*ptr++);
	}
	fprintf(stderr,"\n");
    }
}


/*
 *  Normalise a vector by creating a unit vector
 *  Vectors for which || == 0 are unchanged.
 */

normalise(vec)
    float vec[3];
{
    float mag;

    mag = sqrt(vec[0] *  vec[0] + vec[1] * vec[1] +  
      vec[2] * vec[2]);
    if(mag != 0.0){
	vec[0] /= mag;
	vec[1] /= mag;
	vec[2] /= mag;
    }
}


/*
 *  Form the dot (scalar) product of two vectors
 */

float dot_product(v1,v2)
    float v1[3], v2[3];
{
    float dotp;

    dotp = v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2] ;

    return(dotp);

}


/*
 *  Form the cross (vector) product of two vectors
 */

cross_product(v1,v2,vout)
    float v1[3], v2[3], vout[3];
{
    vout[0] = v1[1] * v2[2] - v1[2] * v2[1];
    vout[1] = v1[2] * v2[0] - v1[0] * v2[2];
    vout[2] = v1[0] * v2[1] - v1[1] * v2[0];
}


/*
 *  Get the surface normal of a plane defined by three points.
 *  The order of the points (clockwise or counter-clockwise,
 *  looking at the plane) defines the direction of the normal.
 *  The rule is: if you are looking at the plane, the normal
 *  points towards you when the points are in clockwise order.
 *
 *  Method: obtain two vectors in the plane pointing away from 
 *  p2. Then take the cross product of these vectors to obtain
 *  a vector perpendicular to the plane.
 */
 
surf_normal(p1,p2,p3,norm)
    float p1[3], p2[3], p3[3], norm[3];
{
    int i;
    float d1[3],d2[3];

    for(i=0;i<3;i++){
	d1[i] = p1[i] - p2[i];
	d2[i] = p3[i] - p2[i];
    }

    cross_product(d2,d1,norm);

}
