/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/

/**************************************************************************
 **** UTILITY FUNCTIONS
 **************************************************************************/

#include <stdio.h>

#include <X11/Xlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>


#ifdef INCLUDE_GETPIXELFROMSTRING
/*
 * function to return a pixel value from a specified display and color string
 */

unsigned long getPixelFromString(display,colorString)
    Display *display;
    char *colorString;
{
    XColor color, ignore;

    if(!XAllocNamedColor(display,DefaultColormap(display,DefaultScreen(display)),
      colorString,&color,&ignore)) {
        fprintf(stderr,
	  "\ngetPixelFromString:  couldn't allocate color %s",
	  colorString);
        return(WhitePixel(display, DefaultScreen(display)));
    } else {
        return(color.pixel);
    }
}

#endif


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



/*
 * function to return an XFontStruct pointer from a specified widget
 *   and string
 */
XFontStruct *getFontStructFromStringW(w,string)
    Widget w;
    char *string;
{

    XFontStruct *returnFontStruct;

    if ((returnFontStruct=XLoadQueryFont(XtDisplay(w),string)) == NULL) {
	return (XLoadQueryFont(XtDisplay(w),XtDefaultFont));
    } else {
	return (returnFontStruct);
    }

}
