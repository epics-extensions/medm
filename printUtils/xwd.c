/* $XConsortium: copyright.h,v 1.5 89/12/22 16:11:28 rws Exp $ */
/*

  Copyright 1985, 1986, 1987, 1988, 1989 by the
  Massachusetts Institute of Technology

  Permission to use, copy, modify, distribute, and sell this software and its
  documentation for any purpose is hereby granted without fee, provided that
  the above copyright notice appear in all copies and that both that
  copyright notice and this permission notice appear in supporting
  documentation, and that the name of M.I.T. not be used in advertising or
  publicity pertaining to distribution of the software without specific,
  written prior permission.  M.I.T. makes no representations about the
  suitability of this software for any purpose.  It is provided "as is"
  without express or implied warranty.

*/


/* Copyright 1987 Massachusetts Institute of Technology */

/*
 * xwd.c MIT Project Athena, X Window system window raster image dumper.
 *
 * This program will dump a raster image of the contents of a window into a 
 * file for output on graphics printers or for other uses.
 *
 *  Author:	Tony Della Fera, DEC
 *		17-Jun-85
 * 
 *  Modification history:
 *
 *  11/14/86 Bill Wyatt, Smithsonian Astrophysical Observatory
 *    - Removed Z format option, changing it to an XY option. Monochrome 
 *      windows will always dump in XY format. Color windows will dump
 *      in Z format by default, but can be dumped in XY format with the
 *      -xy option.
 *
 *  11/18/86 Bill Wyatt
 *    - VERSION 6 is same as version 5 for monchrome. For colors, the 
 *      appropriate number of Color structs are dumped after the header,
 *      which has the number of colors (=0 for monochrome) in place of the
 *      V5 padding at the end. Up to 16-bit displays are supported. I
 *      don't yet know how 24- to 32-bit displays will be handled under
 *      the Version 11 protocol.
 *
 *  6/15/87 David Krikorian, MIT Project Athena
 *    - VERSION 7 runs under the X Version 11 servers, while the previous
 *      versions of xwd were are for X Version 10.  This version is based
 *      on xwd version 6, and should eventually have the same color
 *      abilities. (Xwd V7 has yet to be tested on a color machine, so
 *      all color-related code is commented out until color support
 *      becomes practical.)
 *
 * 27 June 1990 - Mark Anderson, Argonne National Laboratory
 *   -	a procedural version of XWD for internal print routines (basically
 *	strip out the select window stuff, just pass in the window of
 *	interest...
 */


/*%
 *%    This is the format for commenting out color-related code until
 *%  color can be supported.
%*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <X11/Xmd.h>
/*  (MDA) use Xmd.h instead of Xmu...
#include <X11/Xmu/WinUtil.h>
*/

typedef unsigned long Pixel;

#define FEEP_VOLUME 0

/* KE: Added for function prototypes */
#include "printUtils.h"

/* Function prototypes */
static long parse_long (char *s);
static int Window_Dump(Display *display, Window window, FILE *out);
static int Image_Size(XImage *image);
static int Get_XColors(Display *display, XWindowAttributes *win_info,
  XColor **colors);
static void _swaplong (register char *bp, register unsigned n);
static void _swapshort (register char *bp, register unsigned n);


/* Setable Options */

int format = ZPixmap;
Bool nobdrs = False;
Bool standard_out = True;
#if DEBUG
Bool debug = True;
#else
Bool debug = False;
#endif
long add_pixel_value = 0;

extern int (*_XErrorFunction)();
extern int _XDefaultError();

static long parse_long (char *s)
{
    char *fmt = "%lu";
    long retval = 0L;
    int thesign = 1;

    if (s && s[0]) {
	if (s[0] == '-') s++, thesign = -1;
	if (s[0] == '0') s++, fmt = "%lo";
	if (s[0] == 'x' || s[0] == 'X') s++, fmt = "%lx";
	(void) sscanf (s, fmt, &retval);
    }
    return (thesign * retval);
}



/*
 * the xwd() routine (used to be a main()...with extra stuff)
 */

int xwd(Display *display, Window window, char *file)
{
    Window target_win;
    FILE *out_file = stdout;
    Bool frame_only = False;
    int status;

    target_win = window;

    if (!(out_file = fopen(file, "w"))) {
	errMsg("xwd: Can't open output file: %s\n",file);
	return 0;
    }
    standard_out = False;

  /* for color displays only */
  /****
    if (!strcmp(argv[i], "-xy")) {
    format = XYPixmap;
    continue;
    }
    ****/
  /* to include the window manager frame */
  /****
    if (!strcmp(argv[i], "-frame")) {
    frame_only = True;
    continue;
    }
    ****/
    
  /**** don't believe you need this since you're not asking the window mgr.
    for any help....

    if (window != None && !frame_only) {
    Window root;
    int dummyi;
    unsigned int dummy;

    target_win = window;

    if (XGetGeometry (dpy, target_win, &root, &dummyi, &dummyi,
    &dummy, &dummy, &dummy, &dummy) &&
    target_win != root)
    target_win = XmuClientWindow (dpy, target_win);
    }
    ****/


  /*
   * Dump it!
   */
    status = Window_Dump(display, target_win, out_file);
    fclose(out_file);
    return status;
}


/*
 * Window_Dump: dump a window to a file which must already be open for
 *              writting.
 */

#include "X11/XWDFile.h"

static int Window_Dump(Display *display, Window window, FILE *out)
{
    unsigned long swaptest = 1;
    XColor *colors;
    unsigned buffer_size;
    int win_name_size;
    int header_size;
    int ncolors, i;
    char *win_name;
    Bool got_win_name;
    XWindowAttributes win_info;
    XImage *image;
    int absx, absy, x, y;
    unsigned width, height;
    int dwidth, dheight;
    int bw;
    Window dummywin;
    XWDFileHeader header;
    int screen;
    
  /*
   * Inform the user not to alter the screen.
   */
    XBell(display, 50);

  /*
   * Get the parameters of the window being dumped.
   */
    if (debug) print("xwd: Error getting target window information\n");
    if(!XGetWindowAttributes(display, window, &win_info)) {
	errMsg("xwd: Can't get target window attributes\n");
	return 0;
    }
    screen = DefaultScreen(display);

  /* handle any frame window */
    if (!XTranslateCoordinates (display, window, DefaultRootWindow(display), 0, 0,
      &absx, &absy, &dummywin)) {
	errMsg("xwd: Unable to translate window coordinates (%d,%d)\n",
	  absx, absy);
	return 0;
    }
    win_info.x = absx;
    win_info.y = absy;
    width = win_info.width;
    height = win_info.height;
    bw = 0;

    if (!nobdrs) {
	absx -= win_info.border_width;
	absy -= win_info.border_width;
	bw = win_info.border_width;
	width += (2 * bw);
	height += (2 * bw);
    }
    dwidth = DisplayWidth (display, screen);
    dheight = DisplayHeight (display, screen);
    
    
  /* clip to window */
    if (absx < 0) width += absx, absx = 0;
    if (absy < 0) height += absy, absy = 0;
    if (absx + (int)width > dwidth) width = dwidth - absx;
    if (absy + (int)height > dheight) height = dheight - absy;
    
    XFetchName(display, window, &win_name);
    if (!win_name || !win_name[0]) {
	win_name = "xwdump";
	got_win_name = False;
    } else {
	got_win_name = True;
    }
    
  /* sizeof(char) is included for the null string terminator. */
    win_name_size = strlen(win_name) + sizeof(char);
    
  /*
   * Snarf the pixmap with XGetImage.
   */
    
    x = absx - win_info.x;
    y = absy - win_info.y;
    image = XGetImage (display, window, x, y, width, height, AllPlanes, format);
    if (!image) {
	errMsg("xwd: Unable to get image at %dx%d+%d+%d\n",
	  width, height, x, y);
	return 0;
    }
    
    if (add_pixel_value != 0) XAddPixel((XImage *)image, add_pixel_value);
    
  /*
   * Determine the pixmap size.
   */
    buffer_size = Image_Size(image);
    
    if (debug) print("xwd: Getting Colors.\n");
    
    ncolors = Get_XColors(display, &win_info, &colors);
    
  /*
   * Inform the user that the image has been retrieved.
   */
    XBell(display, FEEP_VOLUME);
    XBell(display, FEEP_VOLUME);
    XFlush(display);
    
  /*
   * Calculate header size.
   */
    if (debug) print("xwd: Calculating header size.\n");
    header_size = sizeof(header) + win_name_size;
    
  /*
   * Write out header information.
   */
    if (debug) print("xwd: Constructing and dumping file header.\n");
    header.header_size = (CARD32) header_size;
    header.file_version = (CARD32) XWD_FILE_VERSION;
    header.pixmap_format = (CARD32) format;
    header.pixmap_depth = (CARD32) image->depth;
    header.pixmap_width = (CARD32) image->width;
    header.pixmap_height = (CARD32) image->height;
    header.xoffset = (CARD32) image->xoffset;
    header.byte_order = (CARD32) image->byte_order;
    header.bitmap_unit = (CARD32) image->bitmap_unit;
    header.bitmap_bit_order = (CARD32) image->bitmap_bit_order;
    header.bitmap_pad = (CARD32) image->bitmap_pad;
    header.bits_per_pixel = (CARD32) image->bits_per_pixel;
    header.bytes_per_line = (CARD32) image->bytes_per_line;
    header.visual_class = (CARD32) win_info.visual->class;
    header.red_mask = (CARD32) win_info.visual->red_mask;
    header.green_mask = (CARD32) win_info.visual->green_mask;
    header.blue_mask = (CARD32) win_info.visual->blue_mask;
    header.bits_per_rgb = (CARD32) win_info.visual->bits_per_rgb;
    header.colormap_entries = (CARD32) win_info.visual->map_entries;
    header.ncolors = ncolors;
    header.window_width = (CARD32) win_info.width;
    header.window_height = (CARD32) win_info.height;
    header.window_x = absx;
    header.window_y = absy;
    header.window_bdrwidth = (CARD32) win_info.border_width;
    
    if (*(char *) &swaptest) {
	_swaplong((char *) &header, sizeof(header));
	for (i = 0; i < ncolors; i++) {
	    _swaplong((char *) &colors[i].pixel, sizeof(long));
	    _swapshort((char *) &colors[i].red, 3 * sizeof(short));
	}
    }
    
    (void) fwrite((char *)&header, sizeof(header), 1, out);
    (void) fwrite(win_name, win_name_size, 1, out);
    
  /*
   * Write out the color maps, if any
   */
    
    if (debug) print("xwd: Dumping %d colors.\n", ncolors);
    (void) fwrite((char *) colors, sizeof(XColor), ncolors, out);
    
  /*
   * Write out the buffer.
   */
    if (debug) print("xwd: Dumping pixmap.  bufsize=%d\n",buffer_size);
    
  /*
   *    This copying of the bit stream (data) to a file is to be replaced
   *  by an Xlib call which hasn't been written yet.  It is not clear
   *  what other functions of xwd will be taken over by this (as yet)
   *  non-existant X function.
   */
    (void) fwrite(image->data, (int) buffer_size, 1, out);
    
  /*
   * free the color buffer.
   */
    
    if(debug && ncolors > 0) print("xwd: Freeing colors.\n");
    if(ncolors > 0) free(colors);
    
  /*
   * Free window name string.
   */
    if (debug) print("xwd: Freeing window name string.\n");
    if (got_win_name) XFree(win_name);
    
  /*
   * Free image
   */
    XDestroyImage((XImage *)image);

    return 1;
}

/*
 * Determine the pixmap size.
 */
static int Image_Size(XImage *image)
{
    if (format != ZPixmap)
      return(image->bytes_per_line * image->height * image->depth);

    return(image->bytes_per_line * image->height);
}

#define lowbit(x) ((x) & (~(x) + 1))

/*
 * Get the XColors of all pixels in image - returns # of colors
 */
static int Get_XColors(Display *display, XWindowAttributes *win_info,
  XColor **colors)
{
    int i, ncolors;

    if (!win_info->colormap)
      return(0);

    ncolors = win_info->visual->map_entries;
    if (!(*colors = (XColor *) malloc (sizeof(XColor) * ncolors))) {
      errMsg("xwd: Error allocating memory for colors/n");
      return(0);
    }

    if (win_info->visual->class == DirectColor ||
      win_info->visual->class == TrueColor) {
	Pixel red, green, blue, red1, green1, blue1;

	red = green = blue = 0;
	red1 = lowbit(win_info->visual->red_mask);
	green1 = lowbit(win_info->visual->green_mask);
	blue1 = lowbit(win_info->visual->blue_mask);
	for (i=0; i<ncolors; i++) {
	    (*colors)[i].pixel = red|green|blue;
	    (*colors)[i].pad = 0;
	    red += red1;
	    if (red > win_info->visual->red_mask)
	      red = 0;
	    green += green1;
	    if (green > win_info->visual->green_mask)
	      green = 0;
	    blue += blue1;
	    if (blue > win_info->visual->blue_mask)
	      blue = 0;
	}
    } else {
	for (i=0; i<ncolors; i++) {
	    (*colors)[i].pixel = i;
	    (*colors)[i].pad = 0;
	}
    }

    XQueryColors(display, win_info->colormap, *colors, ncolors);
    
    return(ncolors);
}

static void _swapshort (register char *bp, register unsigned n)
{
    register char c;
    register char *ep = bp + n;

    while (bp < ep) {
	c = *bp;
	*bp = *(bp + 1);
	bp++;
	*bp++ = c;
    }
}

static void _swaplong (register char *bp, register unsigned n)
{
    register char c;
    register char *ep = bp + n;
    register char *sp;

    while (bp < ep) {
	sp = bp + 3;
	c = *sp;
	*sp = *bp;
	*bp++ = c;
	sp = bp + 1;
	c = *sp;
	*sp = *bp;
	*bp++ = c;
	bp += 2;
    }
}
