/*
 * Interaction handling for Graph plot type
 *
 *
 */

#include <stdio.h>
#include <math.h>

#include <X11/Intrinsic.h>
#include <X11/Xlib.h>

#include "Graph.h"



/* forward declarations for a few functions */
#ifdef _NO_PROTO
void graphDeviceToWorld();
void graphStartRubberbanding();
void graphDrawRubberRectangle();
void graphStopRubberbanding();
#else
void graphDeviceToWorld(Graph *, int, int, int, double *, double *, double *);
void graphStartRubberbanding(Widget, Graph *, XEvent *);
void graphDrawRubberRectangle(Widget, Graph *, XEvent *);
void graphStopRubberbanding(Widget, Graph *, XEvent *);
#endif


#ifdef _NO_PROTO
void graphSetInteractive(graph,flag)
    Graph *graph;
    Boolean flag;
#else
    void graphSetInteractive(Graph *graph, Boolean flag)
#endif
{

  /*
   * add or remove interactive pan/zoom to plot
   */
    if (flag) {
	if (graph->interactive == NULL) {
	    graph->interactive = (GraphInteraction *) 
	      malloc(sizeof(GraphInteraction));
	    graph->interactive->first = TRUE;
	    graph->interactive->saveX0 = -1; graph->interactive->saveY0 = -1;
	    graph->interactive->saveX1 = -1; graph->interactive->saveY1 = -1;
	    graph->interactive->useX0 = -1;  graph->interactive->useY0 = -1;
	    graph->interactive->useX1 = -1;  graph->interactive->useY1 = -1;
	}

	XtAddEventHandler(XtWindowToWidget(graph->display,graph->window),
	  ButtonPressMask,False,
	  (XtEventHandler)graphStartRubberbanding,graph);
    } else {
      /*
       * remove interactive structure and event handler, free memory
       */
	if (graph->interactive != NULL) {
	    XtRemoveEventHandler(XtWindowToWidget(graph->display,graph->window),
	      ButtonPressMask,False,
	      (XtEventHandler)graphStartRubberbanding,graph);
	    free ((char *) graph->interactive);
	    graph->interactive = NULL;
	}
    }

}


/***
 *** event handlers for activity in the graph window
 ***/

extern void graphStartRubberbanding(w, graph, event)
    Widget w;
    Graph *graph;
    XEvent *event;
{
    int i;
    XButtonEvent *xEvent = (XButtonEvent *) event;
    double worldX, worldY, worldZ;


    if (xEvent->x > graph->dataX0 && xEvent->x < graph->dataX1  &&
      xEvent->y > graph->dataY0 && xEvent->y < graph->dataY1) {

	if (graph->interactive->first) {

	    graph->interactive->xorGC = XCreateGC(graph->display,
	      DefaultRootWindow(graph->display),
	      (unsigned long)NULL,NULL);
	    XSetFunction(graph->display,graph->interactive->xorGC,GXxor);
	    graph->interactive->crosshairPixel = getPixelFromString(
	      graph->display,graph->screen,CROSSHAIRCOLOR);
	    XSetForeground(graph->display,graph->interactive->xorGC,
	      graph->interactive->crosshairPixel);
	    XSetBackground(graph->display,graph->interactive->xorGC,
	      graph->backPixel);
	    XSetLineAttributes(graph->display,graph->interactive->xorGC,
	      0,LineSolid, CapButt,JoinBevel);
	    graph->interactive->first = FALSE;
	}


	if (xEvent->button == Button1) {

	    graph->interactive->saveX0 = xEvent->x;
	    graph->interactive->saveY0 = xEvent->y;

	  /*
	   * now add enable?) event handlers for pointer motion events and 
	   *	button up events
	   */
	    XtAddEventHandler(w, PointerMotionMask,False,
	      (XtEventHandler)graphDrawRubberRectangle,graph);
	    XtAddEventHandler(w, ButtonReleaseMask,False,
	      (XtEventHandler)graphStopRubberbanding,graph);


	}

    } else {
      /*
       * user clicked outside data window, therefore unzoom back to defaults
       */
	for (i = 0; i < graph->nBuffers; i++) {
	    graphSetRangeDefault(graph,i,'X');
	    graphSetRangeDefault(graph,i,'Y');
	    graphDraw(graph);
	}
    }
}



extern void graphDrawRubberRectangle(w, graph, event)
    Widget w;
    Graph *graph;
    XEvent *event;
{
    XMotionEvent *xEvent = (XMotionEvent *) event;


    if (xEvent->x > graph->dataX0 && xEvent->x < graph->dataX1  &&
      xEvent->y > graph->dataY0 && xEvent->y < graph->dataY1) {


      /* undraw previous rectangle */
	if (graph->interactive->saveX0 >= 0 && 
	  graph->interactive->saveY0 >= 0 && 
	  graph->interactive->saveX1 >= 0 && 
	  graph->interactive->saveY1 >= 0) {
	    graph->interactive->useX0 = 
	      min(graph->interactive->saveX0,
		graph->interactive->saveX1);
	    graph->interactive->useY0 = 
	      min(graph->interactive->saveY0,
		graph->interactive->saveY1);
	    graph->interactive->useX1 = 
	      max(graph->interactive->saveX0,
		graph->interactive->saveX1);
	    graph->interactive->useY1 = 
	      max(graph->interactive->saveY0,
		graph->interactive->saveY1);
	    XDrawRectangle(graph->display,graph->window,
	      graph->interactive->xorGC,
	      graph->interactive->useX0, 
	      graph->interactive->useY0, 
	      graph->interactive->useX1 - graph->interactive->useX0, 
	      graph->interactive->useY1 - graph->interactive->useY0);
	}


      /* draw new rubberbanded rectangle */
	if (graph->interactive->saveX0 >= 0 && 
	  graph->interactive->saveY0 >= 0) {

	  /* and save these positions */
	    graph->interactive->saveX1 = xEvent->x;
	    graph->interactive->saveY1 = xEvent->y;

	    graph->interactive->useX0 = 
	      min(graph->interactive->saveX0,graph->interactive->saveX1);
	    graph->interactive->useY0 = 
	      min(graph->interactive->saveY0,graph->interactive->saveY1);
	    graph->interactive->useX1 = 
	      max(graph->interactive->saveX0,graph->interactive->saveX1);
	    graph->interactive->useY1 = 
	      max(graph->interactive->saveY0,graph->interactive->saveY1);
	    XDrawRectangle(graph->display,graph->window,
	      graph->interactive->xorGC,
	      graph->interactive->useX0,
	      graph->interactive->useY0,
	      graph->interactive->useX1 - graph->interactive->useX0,
	      graph->interactive->useY1 - graph->interactive->useY0);
	}

    }

}




extern void graphStopRubberbanding(w, graph, event)
    Widget w;
    Graph *graph;
    XEvent *event;
{
    int i;
    XButtonEvent *xEvent = (XButtonEvent *) event;
    double worldX0, worldY0, worldZ0;
    double worldX1, worldY1, worldZ1;



    if (xEvent->x > graph->dataX0 && xEvent->x < graph->dataX1  &&
      xEvent->y > graph->dataY0 && xEvent->y < graph->dataY1) {

	if (xEvent->button == Button1) {

	  /*
	   * now remove event handlers for pointer motion events and button up events
	   */
	    XtRemoveEventHandler(w,PointerMotionMask,False,
	      (XtEventHandler)graphDrawRubberRectangle,graph);
	    XtRemoveEventHandler(w,ButtonReleaseMask,False,
	      (XtEventHandler)graphStopRubberbanding,graph);


	  /* undraw previous rectangle */
	    if (graph->interactive->saveX0 >= 0 && 
	      graph->interactive->saveY0 >= 0) {
		graph->interactive->useX0 = 
		  min(graph->interactive->saveX0,
		    graph->interactive->saveX1);
		graph->interactive->useY0 = 
		  min(graph->interactive->saveY0,
		    graph->interactive->saveY1);
		graph->interactive->useX1 = 
		  max(graph->interactive->saveX0,
		    graph->interactive->saveX1);
		graph->interactive->useY1 = 
		  max(graph->interactive->saveY0,
		    graph->interactive->saveY1);
		XDrawRectangle(graph->display,graph->window,
		  graph->interactive->xorGC,
		  graph->interactive->useX0,
		  graph->interactive->useY0,
		  graph->interactive->useX1 - graph->interactive->useX0,
		  graph->interactive->useY1 - graph->interactive->useY0);
	    }


	  /* now map from device to world coordinates */
	    for (i = 0; i < graph->nBuffers; i++) {
		graphDeviceToWorld(graph,i,
		  graph->interactive->saveX0,
		  graph->interactive->saveY0,&worldX0,&worldY0,&worldZ0);
		graphDeviceToWorld(graph,i,
		  xEvent->x,xEvent->y,
		  &worldX1,&worldY1,&worldZ1);

	      /* and update graph and display to reflect the zoomed region */
		graphSetRange(graph,i,'X', 
		  min(worldX0,worldX1), max(worldX0,worldX1) );
		graphSetRange(graph,i,'Y', 
		  min(worldY0,worldY1), max(worldY0,worldY1) );
		graphDraw(graph);
	    }

	    graph->interactive->saveX0 = graph->interactive->saveY0 = 
	      graph->interactive->saveX1 = graph->interactive->saveY1 = -1;

	}
    }

}




void graphDeviceToWorld(graph,bufNum,x,y,worldX,worldY,worldZ)
    Graph *graph;
    int bufNum;
    int x, y;
    double *worldX, *worldY, *worldZ;
{

  /***
  *** calculate the X world coordinates relative to device coordinates
  ***/

    *worldX  =   graph->xRange[bufNum].minVal + ((double) (x - graph->dataX0) /
      (double) (graph->dataX1 - graph->dataX0)) *
      (graph->xRange[bufNum].maxVal - 
	graph->xRange[bufNum].minVal);
  /***
  *** calculate Y world coordinates as above, but note the distinction:
  *** the difference in device coordinates is from Y1, not Y0
  ***/

    *worldY  =   graph->yRange[bufNum].minVal + ((double) (graph->dataY1 - y) /
      (double) (graph->dataY1 - graph->dataY0)) *
      (graph->yRange[bufNum].maxVal - 
	graph->yRange[bufNum].minVal);
    *worldZ = 0.0;

}
