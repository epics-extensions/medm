/*
 * Interaction handling for Seql plot type
 *
 *
 */

#include <stdio.h>
#include <math.h>

#include <X11/Intrinsic.h>
#include <X11/Xlib.h>

#include "Seql.h"



/* forward declarations for a few functions */

#ifdef _NO_PROTO
void seqlDeviceToWorld();
void seqlStartRubberbanding();
void seqlDrawRubberRectangle();
void seqlStopRubberbanding();
#else
void seqlDeviceToWorld(Seql *, int, int, int, double *, double *, double *);
void seqlStartRubberbanding(Widget, Seql *, XEvent *);
void seqlDrawRubberRectangle(Widget, Seql *, XEvent *);
void seqlStopRubberbanding(Widget, Seql *, XEvent *);
#endif




#ifdef _NO_PROTO
void seqlSetInteractive(seql,flag)
  Seql *seql;
  Boolean flag;
#else
void seqlSetInteractive( Seql *seql, Boolean flag)
#endif
{

/*
 * add or remove interactive pan/zoom to plot
 */
  if (flag) {
     if (seql->interactive == NULL) {
	seql->interactive = (SeqlInteraction *) 
					malloc(sizeof(SeqlInteraction));
	seql->interactive->first = TRUE;
	seql->interactive->saveX0 = -1; seql->interactive->saveY0 = -1;
	seql->interactive->saveX1 = -1; seql->interactive->saveY1 = -1;
	seql->interactive->useX0 = -1;  seql->interactive->useY0 = -1;
	seql->interactive->useX1 = -1;  seql->interactive->useY1 = -1;
     }

     XtAddEventHandler(XtWindowToWidget(seql->display,seql->window),
		ButtonPressMask,False,
		(XtEventHandler)seqlStartRubberbanding,seql);
  } else {
/*
 * remove interactive structure and event handler, free memory
 */
     if (seql->interactive != NULL) {
	XtRemoveEventHandler(XtWindowToWidget(seql->display,seql->window),
		ButtonPressMask,False,
		(XtEventHandler)seqlStartRubberbanding,seql);
	free ((char *) seql->interactive);
	seql->interactive = NULL;
     }
  }
}


/***
 *** event handlers for activity in the seql window
 ***/

extern void seqlStartRubberbanding(w, seql, event)
  Widget w;
  Seql *seql;
  XEvent *event;
{
  int i;
  XButtonEvent *xEvent = (XButtonEvent *) event;
  double worldX, worldY, worldZ;


  if (xEvent->x > seql->dataX0 && xEvent->x < seql->dataX1  &&
      xEvent->y > seql->dataY0 && xEvent->y < seql->dataY1) {

    if (seql->interactive->first) {

      seql->interactive->xorGC = XCreateGC(seql->display,
			DefaultRootWindow(seql->display),
			(unsigned long)NULL,NULL);
      XSetFunction(seql->display,seql->interactive->xorGC,GXxor);
      seql->interactive->crosshairPixel = getPixelFromString(
			seql->display,seql->screen,CROSSHAIRCOLOR);
      XSetForeground(seql->display,seql->interactive->xorGC,
			seql->interactive->crosshairPixel);
      XSetBackground(seql->display,seql->interactive->xorGC,
			seql->backPixel);
      XSetLineAttributes(seql->display,seql->interactive->xorGC,
			0,LineSolid, CapButt,JoinBevel);
      seql->interactive->first = FALSE;
    }


    if (xEvent->button == Button1) {

      seql->interactive->saveX0 = xEvent->x;
      seql->interactive->saveY0 = xEvent->y;

/*
 * now add enable?) event handlers for pointer motion events and 
 *	button up events
 */
      XtAddEventHandler(w, PointerMotionMask,False,
			(XtEventHandler)seqlDrawRubberRectangle,seql);
      XtAddEventHandler(w, ButtonReleaseMask,False,
			(XtEventHandler)seqlStopRubberbanding,seql);


   }

  } else {
/*
 * user clicked outside data window, therefore unzoom back to defaults
 */
      for (i = 0; i < seql->nBuffers; i++) {
	seqlSetRangeDefault(seql,i,'X');
	seqlSetRangeDefault(seql,i,'Y');
	seqlDraw(seql);
      }
  }
}



extern void seqlDrawRubberRectangle(w, seql, event)
  Widget w;
  Seql *seql;
  XEvent *event;
{
  XMotionEvent *xEvent = (XMotionEvent *) event;


  if (xEvent->x > seql->dataX0 && xEvent->x < seql->dataX1  &&
      xEvent->y > seql->dataY0 && xEvent->y < seql->dataY1) {


/* undraw previous rectangle */
      if (seql->interactive->saveX0 >= 0 && 
		seql->interactive->saveY0 >= 0 && 
		seql->interactive->saveX1 >= 0 && 
		seql->interactive->saveY1 >= 0) {
	seql->interactive->useX0 = 
		min(seql->interactive->saveX0,
			seql->interactive->saveX1);
	seql->interactive->useY0 = 
		min(seql->interactive->saveY0,
			seql->interactive->saveY1);
	seql->interactive->useX1 = 
		max(seql->interactive->saveX0,
			seql->interactive->saveX1);
	seql->interactive->useY1 = 
		max(seql->interactive->saveY0,
			seql->interactive->saveY1);
	XDrawRectangle(seql->display,seql->window,
		seql->interactive->xorGC,
		seql->interactive->useX0, 
		seql->interactive->useY0, 
		seql->interactive->useX1 - seql->interactive->useX0, 
		seql->interactive->useY1 - seql->interactive->useY0);
      }


/* draw new rubberbanded rectangle */
      if (seql->interactive->saveX0 >= 0 && 
		seql->interactive->saveY0 >= 0) {

/* and save these positions */
	seql->interactive->saveX1 = xEvent->x;
	seql->interactive->saveY1 = xEvent->y;

	seql->interactive->useX0 = 
		min(seql->interactive->saveX0,seql->interactive->saveX1);
	seql->interactive->useY0 = 
		min(seql->interactive->saveY0,seql->interactive->saveY1);
	seql->interactive->useX1 = 
		max(seql->interactive->saveX0,seql->interactive->saveX1);
	seql->interactive->useY1 = 
		max(seql->interactive->saveY0,seql->interactive->saveY1);
	XDrawRectangle(seql->display,seql->window,
		seql->interactive->xorGC,
		seql->interactive->useX0,
		seql->interactive->useY0,
		seql->interactive->useX1 - seql->interactive->useX0,
		seql->interactive->useY1 - seql->interactive->useY0);
      }

  }

}




extern void seqlStopRubberbanding(w, seql, event)
  Widget w;
  Seql *seql;
  XEvent *event;
{
  int i;
  XButtonEvent *xEvent = (XButtonEvent *) event;
  double worldX0, worldY0, worldZ0;
  double worldX1, worldY1, worldZ1;



  if (xEvent->x > seql->dataX0 && xEvent->x < seql->dataX1  &&
      xEvent->y > seql->dataY0 && xEvent->y < seql->dataY1) {

    if (xEvent->button == Button1) {

/*
 * now remove event handlers for pointer motion events and button up events
 */
      XtRemoveEventHandler(w,PointerMotionMask,False,
			(XtEventHandler)seqlDrawRubberRectangle,seql);
      XtRemoveEventHandler(w,ButtonReleaseMask,False,
			(XtEventHandler)seqlStopRubberbanding,seql);


/* undraw previous rectangle */
      if (seql->interactive->saveX0 >= 0 && 
				seql->interactive->saveY0 >= 0) {
	seql->interactive->useX0 = 
		min(seql->interactive->saveX0,
			seql->interactive->saveX1);
	seql->interactive->useY0 = 
		min(seql->interactive->saveY0,
			seql->interactive->saveY1);
	seql->interactive->useX1 = 
		max(seql->interactive->saveX0,
			seql->interactive->saveX1);
	seql->interactive->useY1 = 
		max(seql->interactive->saveY0,
			seql->interactive->saveY1);
	XDrawRectangle(seql->display,seql->window,
		seql->interactive->xorGC,
		seql->interactive->useX0,
		seql->interactive->useY0,
		seql->interactive->useX1 - seql->interactive->useX0,
		seql->interactive->useY1 - seql->interactive->useY0);
      }


/* now map from device to world coordinates */
      for (i = 0; i < seql->nBuffers; i++) {
	seqlDeviceToWorld(seql,i,
		seql->interactive->saveX0,
		seql->interactive->saveY0,&worldX0,&worldY0,&worldZ0);
	seqlDeviceToWorld(seql,i,
		xEvent->x,xEvent->y,
		&worldX1,&worldY1,&worldZ1);

/* and update seql and display to reflect the zoomed region */
	seqlSetRange(seql,i,'X', 
			min(worldX0,worldX1), max(worldX0,worldX1) );
	seqlSetRange(seql,i,'Y', 
			min(worldY0,worldY1), max(worldY0,worldY1) );
	seqlDraw(seql);
      }

      seql->interactive->saveX0 = seql->interactive->saveY0 = 
	seql->interactive->saveX1 = seql->interactive->saveY1 = -1;

   }
  }

}




void seqlDeviceToWorld(seql,bufNum,x,y,worldX,worldY,worldZ)
  Seql *seql;
  int bufNum;
  int x, y;
  double *worldX, *worldY, *worldZ;
{

/***
 *** calculate the X world coordinates relative to device coordinates
 ***/

 *worldX  =   seql->xRange[bufNum].minVal + ((double) (x - seql->dataX0) /
                 (double) (seql->dataX1 - seql->dataX0)) *
                 (seql->xRange[bufNum].maxVal - 
			seql->xRange[bufNum].minVal);
/***
 *** calculate Y world coordinates as above, but note the distinction:
 *** the difference in device coordinates is from Y1, not Y0
 ***/

 *worldY  =   seql->yRange[bufNum].minVal + ((double) (seql->dataY1 - y) /
                 (double) (seql->dataY1 - seql->dataY0)) *
                 (seql->yRange[bufNum].maxVal - 
			seql->yRange[bufNum].minVal);
 *worldZ = 0.0;

}



