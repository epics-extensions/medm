
/**
 ** toplevel Surface Plotter test program
 **
 ** MDA - 1 June 1990
 **/


#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <Xm/Xm.h>
#include <Xm/DrawingA.h>
#include <Xm/PushB.h>
#include <Xm/BulletinB.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/Scale.h>
#include <Xm/RowColumn.h>
#include <Xm/List.h>

#include <math.h>                /* very Very VERY IMPORTANT ! */

#include "GraphX.h"             /* overall GraphX include file
                                   defines data types, plot types,
                                   functions and macros            */
#include "Surface.h"


#define X_SIZE 60
#define Y_SIZE 60

#define ROTATION_SIZE 5.0

static XtWorkProc animateSurface();

Widget canvas;
Surface *surface;
Boolean animate = FALSE;
Boolean shaded = FALSE;
XtWorkProcId animateId = NULL;
int surfaceWidth = 384;
int surfaceHeight = 384;
float xAngle = 45.0, yAngle = 0.0, zAngle = 45.0;



/* 
 * TRANSLATIONS
 */

static void printPlot(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;
int    num_params;
{

  surfacePrint(surface);

}


static void rotateSurface(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;
int    num_params;
{

switch(atoi(params[0])) {

  case 1 :	xAngle -= ROTATION_SIZE;
		if (xAngle < 0.0) xAngle += 360.0;
		break;

  case 2 :	xAngle += ROTATION_SIZE;
		if (xAngle > 360.0) xAngle -= 360.0;
		break;

  case 3 :	yAngle -= ROTATION_SIZE;
		if (yAngle < 0.0) yAngle += 360.0;
		break;

  case 4 :	yAngle += ROTATION_SIZE;
		if (yAngle > 360.0) yAngle -= 360.0;
		break;
  }

surfaceSetView(surface, xAngle, yAngle, zAngle);
surfaceDraw(surface);

}



static void toggleRenderMode(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;
int    num_params;
{
   if (shaded) {
      shaded = FALSE;
      surfaceSetRenderMode(surface,SurfaceSolid);
   } else {
      shaded = TRUE;
      surfaceSetRenderMode(surface,SurfaceShaded);
   }
   surfaceDraw(surface);

}



static void toggleSurfaceAnimation(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;
int    num_params;
{
   if (animate) {
      animate = FALSE;
      XtRemoveWorkProc(animateId);
   } else {
      animate = TRUE;
      /* must call this after XtInitialize() */
      animateId = XtAddWorkProc((XtWorkProc)animateSurface,NULL);
   }
}



static void quit(w,event,params,num_params)
Widget w;
XEvent *event;
String *params;
int    num_params;
{
  surfaceTerm(surface);
  exit(0);
}



/*
 * CALLBACKS 
 */


static void redisplaySurface(w,surface,call_data)
Widget w;
Surface **surface;     /*  double indirection since filling in actual */
XtPointer call_data;     /* address of Surface struct after XtAddCallback */
{

  surfaceRefresh(*surface);

}


static void resizeSurface(w,mySurface,call_data)
Widget w;
Surface **mySurface;   /*  double indirection since filling in actual */
XtPointer call_data;     /* address of Surface struct after XtAddCallback */
{

   surfaceResize(surface);
   surfaceDraw(surface);

}




static XtWorkProc animateSurface()
{

yAngle -= ROTATION_SIZE;
if (yAngle < 0.0) yAngle += 360.0;
xAngle -= ROTATION_SIZE;
if (xAngle < 0.0) xAngle += 360.0;

surfaceSetView(surface, xAngle, yAngle, zAngle);
surfaceDraw(surface);

/* return FALSE so this WorkProc keeps getting called (just in case...) */
return FALSE;

}


static char defaultTranslations[] =
        "Ctrl<Key>p: printPlot()  \n\
         <Key>l: rotateSurface(1)  \n\
         <Key>h: rotateSurface(2)  \n\
         <Key>j: rotateSurface(3)  \n\
         <Key>k: rotateSurface(4)  \n\
         <Key>a: toggleSurfaceAnimation()  \n\
         <Key>s: toggleRenderMode()  \n\
         <Key>q: quit() ";

static XtActionsRec actionsTable[] = {
        {"rotateSurface", (XtActionProc)rotateSurface},
        {"toggleSurfaceAnimation", (XtActionProc)toggleSurfaceAnimation},
        {"toggleRenderMode", (XtActionProc)toggleRenderMode},
        {"printPlot", (XtActionProc)printPlot},
        {"quit", (XtActionProc)quit},
};





main(argc,argv)
  int   argc;
  char *argv[];
{
  Widget topLevel;
  Arg args[5];
  int n;

  XtTranslations transTable;

  Display *display;
  Window window;
  int screen;

  int i, j;
  double mesh[X_SIZE*Y_SIZE], xArray[X_SIZE], yArray[Y_SIZE];
  double r;
  char *foreColor = "white";
  char *backColor = "black";
  char *dataColor = "light blue";


/* these are filled in by graphXGetBestFont() calls */
  char *titleFont, *axesFont;
  int   titleFontSize, axesFontSize;


/*
 * Initialize the Intrinsics right away
 */
   topLevel = XtInitialize(argv[0], "Surface Plotter", NULL, 0, &argc, argv);




/*
 * Register new actions and compile translations table
 */
   XtAddActions(actionsTable,XtNumber(actionsTable));
   transTable = XtParseTranslationTable(defaultTranslations);


/*
 * setup a canvas widget to draw in
 */
   n = 0;
   XtSetArg(args[n], XmNwidth, surfaceWidth); n++;
   XtSetArg(args[n], XmNheight,surfaceHeight); n++;

   canvas = XtCreateManagedWidget("canvas", xmDrawingAreaWidgetClass,
                                        topLevel, args, n);

/*
 * merge program-defined translations with existing translations
 */
   XtOverrideTranslations(canvas,transTable);
   XtOverrideTranslations(topLevel,transTable);

/*
 * add the expose and resize callbacks, passing surface as the data of 
 *   the callback
 */
   XtAddCallback(canvas, XmNexposeCallback,
	(XtCallbackProc)redisplaySurface, &surface);
   XtAddCallback(canvas, XmNresizeCallback,
	(XtCallbackProc)resizeSurface, &surface);

/*
 *  need to realize the widgets for windows to be created...
 */

   XtRealizeWidget(topLevel);


/*
 * now proceed with surface generation
 */

   display = XtDisplay(topLevel);
   screen  = DefaultScreen(display);
   window  = XtWindow(canvas);

   surface = surfaceInit(display,screen,window);


{

static double a[12] =    /* was [3][4] */
        { 0.,5.,6., 2.,1.,.2,5.,3.,0., 5.,3.,.5 };
int nx = X_SIZE, ny = Y_SIZE, i_1, i_2;
double xmin, xmax, ymin, ymax, dx, dy, x, y, zf, r_1, r_2;
int l;


   xmin = -1.5;
   xmax = 7.5;
   ymin = -3.5;
   ymax = 10.5;
   dx = (xmax - xmin)/(double)(X_SIZE-1);
   dy = (ymax - ymin)/(double)(Y_SIZE-1);

   for (i=0; i<X_SIZE; i++) xArray[i] = xmin + (double)i * dx;
   for (j=0; j<Y_SIZE; j++) yArray[j] = ymin + (double)j * dy;

/* compute function values at mesh points */


    y = ymin - dy;
    i_1 = ny;
    for (j = 1; j <= i_1; ++j) {
        x = xmin - dx;
        y += dy;
        i_2 = nx;
        for (i = 1; i <= i_2; ++i) {
            x += dx;

            mesh[i + j * X_SIZE - (X_SIZE+1)] = (double)0.;
            for (l = 1; l <= 3; ++l) {
/* Computing 2nd power */
                r_1 = (x - a[l - 1]) / a[l + 2];
/* Computing 2nd power */
                r_2 = (y - a[l + 5]) / a[l + 8];
                zf = r_1 * r_1 + r_2 * r_2 - (double)1.;
                if (fabs(zf) > (double)20.) {
                    goto L50;
                }
                mesh[i + j * X_SIZE - (X_SIZE+1)] += zf * zf * exp(-(double)zf);
L50:
                ;
            }
            mesh[i + j * X_SIZE - (X_SIZE+1)] = 
			(mesh[i + j*X_SIZE - (X_SIZE+1)] - (double)2.) 
				* (double).3;

        }
    }



}



   titleFontSize = GraphX_TitleFontSize(surface);
   titleFont = graphXGetBestFont(display,"times","bold","r", titleFontSize);


   /* setup the Surface */

   surfaceSet(surface, mesh, xArray, yArray, X_SIZE, Y_SIZE, "Surface Plot", 
       titleFont, foreColor, backColor, dataColor, SurfaceExternal);

   surfaceSetView(surface, xAngle, yAngle, zAngle);

   surfaceDraw(surface);


   XtMainLoop();
}


