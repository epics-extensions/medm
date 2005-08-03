/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/

#include <stdio.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <Xm/Xm.h>

#include "Xc.h"
#include "BarGraph.h"


XtWorkProc animate();
XtWorkProcId animateId;
Widget graph;

XcVType min, max, current;
unsigned long white, green, yellow, red;

/*
 * function to return a pixel value from a specified widget and color string
 */

unsigned long getPixelFromStringW(w,colorString)
    Widget w;
    char *colorString;
{
    XColor color, ignore;

    if(!XAllocNamedColor(XtDisplay(w),DefaultColormap(XtDisplay(w),
      DefaultScreen(XtDisplay(w))), colorString,&color,&ignore)) {
	fprintf(stderr,
	  "\ngetPixelFromStringW:  couldn't allocate color %s",
	  colorString);
	return(WhitePixel(XtDisplay(w), DefaultScreen(XtDisplay(w))));
    } else {
	return(color.pixel);
    }
}



main(argc, argv)
    int   argc;
    char *argv[];
{
    Widget topLevel;
    Arg wargs[20];
    int i, n;

    XtPointer ptr;

  /*
   * Initialize the Intrinsics.
   */
    n = 0;
    topLevel = XtInitialize(argv[0], "Test", NULL, 0, &argc, argv);


    n = 0;
    XtSetArg(wargs[n],XcNdataType,XcFval); n++;
    XtSetArg(wargs[n],XcNorient,XcHoriz); n++;
    XtSetArg(wargs[n],XcNscaleSegments,11); n++;
    XtSetArg(wargs[n],XcNlabel," "); n++;
    XtSetArg(wargs[n],XcNvalueVisible,False); n++;
    XtSetArg(wargs[n],XtNheight,100); n++;
    XtSetArg(wargs[n],XtNwidth,100); n++;
    graph = XtCreateManagedWidget("graph", xcBarGraphWidgetClass,
      topLevel, wargs, n);

    white = getPixelFromStringW(graph,"white");
    green = getPixelFromStringW(graph,"green");
    yellow = getPixelFromStringW(graph,"yellow");
    red = getPixelFromStringW(graph,"red");

    max.fval = 1000.0;
    min.fval = 0.0;
    XtSetArg(wargs[0],XcNupperBound,max.lval);
    XtSetArg(wargs[1],XcNlowerBound,min.lval);
    XtSetValues(graph,wargs,2);
    current.fval = 100.0;

    XcBGUpdateValue(graph,&current);

    n = 0;
    XtSetArg(wargs[n],XcNbarForeground,red); n++;
    XtSetValues(graph,wargs,n);


    animateId = XtAddWorkProc((XtWorkProc)animate,NULL);

    XtRealizeWidget(topLevel);

    XtMainLoop();
}



XtWorkProc animate()
{
    Arg wargs[4];
    int i, n;
    static int up = TRUE;
    static Pixel oldPixel, currentPixel;

    if (up) {
	current.fval += 1.0;
	if (current.fval >= max.fval) up = FALSE;
    } else {
	current.fval -= 1.0;
	if (current.fval <= min.fval) up = TRUE;
    }


  /* trigger as often as needed (test gc swapping) */

    n = 0;
    if (up == TRUE) {
	if ( current.fval <= (min.fval + (max.fval - min.fval)/3)) {
	    currentPixel = red;
	} else if ( current.fval >= (min.fval + (max.fval - min.fval)/3) &&
	  current.fval <= (min.fval + 2*(max.fval - min.fval)/3)) {
	    currentPixel = yellow;
	} else if ( current.fval >= (min.fval + 2*(max.fval - min.fval)/3)) {
	    currentPixel = green;
	}
    } else {
	if ( current.fval <= (min.fval + (max.fval - min.fval)/3)) {
	    currentPixel = yellow;
	} else if ( current.fval >= (min.fval + 2*(max.fval - min.fval)/3)) {
	    currentPixel = green;
	}
    }
    n = 0;
    if (currentPixel != oldPixel) {
      /* use faster XcBGUpdateBarForeground()
	 XtSetArg(wargs[n],XcNbarForeground,currentPixel); n++;
	 */
	oldPixel = currentPixel;
	XcBGUpdateBarForeground(graph,currentPixel);
    }
    if (n > 0) XtSetValues(graph,wargs,n);
  /* MDA:  USE XcBGUpdateValue() fucnction:  MUCH MUCH MUCH faster than
     XtSetValues of XcNvalue!!
     */
    XcBGUpdateValue(graph,&current);

    return FALSE;


}
