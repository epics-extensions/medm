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
#include "Indicator.h"


XtWorkProc animate();
XtWorkProcId animateId;
Widget indicator;

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


  /*
   * Initialize the Intrinsics.
   */
    n = 0;
    topLevel = XtInitialize(argv[0], "Test", NULL, 0, &argc, argv);

    n = 0;
    XtSetArg(wargs[n],XcNdataType,XcFval); n++;
    XtSetArg(wargs[n],XcNorient,XcVert); n++;
    XtSetArg(wargs[n],XcNlabel," "); n++;
    XtSetArg(wargs[n],XcNvalueVisible,True); n++;
    XtSetArg(wargs[n],XtNheight,100); n++;
    XtSetArg(wargs[n],XtNwidth,100); n++;
    indicator = XtCreateManagedWidget("indicator", xcIndicatorWidgetClass,
      topLevel, wargs, n);

    white = getPixelFromStringW(indicator,"white");
    green = getPixelFromStringW(indicator,"green");
    yellow = getPixelFromStringW(indicator,"yellow");
    red = getPixelFromStringW(indicator,"red");

    max.fval = 1000.0;
    min.fval = 0.0;
    XtSetArg(wargs[0],XcNupperBound,max.lval);
    XtSetArg(wargs[1],XcNlowerBound,min.lval);
    XtSetValues(indicator,wargs,2);
    current.fval = 100.0;

    XcIndUpdateValue(indicator,&current);

    XtRealizeWidget(topLevel);

    n = 0;
    XtSetArg(wargs[n],XcNindicatorForeground,red); n++;
    XtSetValues(indicator,wargs,n);

  /* test resource changes for debugging */
    XtVaSetValues(indicator,XcNvalueVisible,TRUE,NULL);
    XtVaSetValues(indicator,XcNlabel," ",NULL);

    animateId = XtAddWorkProc((XtWorkProc)animate,NULL);

    XtMainLoop();
}



XtWorkProc animate()
{
    Arg wargs[4];
    int n;
    static int up = TRUE;


    if (up) {
	current.fval += 1.0;
	if (current.fval >= max.fval) up = FALSE;
    } else {
	current.fval -= 1.0;
	if (current.fval <= min.fval) up = TRUE;
    }


    XcIndUpdateValue(indicator,&current);

    return FALSE;


}
