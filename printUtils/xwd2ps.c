/*
 * xwd2ps.c - convert X11 window dumps to PostScript
 *
 *  This program converts color Xwd files to color PostScript for
 *  printing on a color device, such as the QMS ColorScript 100.
 *
 *  Run length encoding is used.  See "usage" subroutine for more
 *  information.  This program is adapted from a similar program for SUN
 *  color raster files, cras2ps, also by R. Tatar.
 *
 *  Acknowledgments:
 *  Thanks to Skip Montanaro, Bill Lorensen, Chris Nafis for
 *  help with C, Unix and PostScript.
 *  Special thanks to Gerry White for all his help with X11 and for
 *  providing example code and initial data files to test the program.
 *
 *  Written by Robert Tatar and Craig A. McGowan
 *
 *  Modification 3-29-90 by CAM
 *     - support for byte-swapped machines
 *  Modification 3-12-90 by CAM
 *     - support for 8 bit, colormapped displays
 *  Modification 2-16-90 by RCT.
 *     - creates properly encapsulated PostScript
 *  Modification 2-20-89 by RCT
 *     - gamma factor for "brightening"
 *  Created      2-xx-89 by RCT
 *     - ported from cras2ps for Sun Raster images
 *
 *  Robert C. Tatar (tatar@crd.ge.com)
 *  Office: (518) 387-5946 or 8*833-5946
 *
 *  Craig A. McGowan (mcgowan@crd.ge.com)
 *  Office: (518) 387-7079 or 8*883-7079
 *
 *  US Mail: GE Corporate Research and Development,
 *           P.O. Box 8,
 *           Bldg KW, Room C214,
 *           Schenectady, NY 12301
 *  Fax:    (518) 387-6560 or 8*833-6560
 *
 * Suggested improvements:
 *  - rotate bitmap instead of using PostScript rotate (up to 4 times faster)
 *  - add option to disable compression


Copyright (c) 1990 General Electric Company

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of General Electric
Company not be used in advertising or publicity pertaining to
distribution of the software without specific, written prior
permission.  General Electric Company makes no representations about
the suitability of this software for any purpose.  It is provided "as
is" without express or implied warranty.

General Electric Company DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS, IN NO EVENT SHALL General Electric Company BE LIABLE FOR ANY
SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

Portions of this program are also Copyright (c) 1989 Massachusetts
Institute of Technology

Permission to use, copy, modify, distribute, and sell this software
and its documentation for any purpose is hereby granted without fee,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of M.I.T. not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.  M.I.T. make no
representations about the suitability of this software for any
purpose.  It is provided "as is" without express or implied warranty.

M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT
SHALL M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.

 */


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

#define DEBUG_FORMAT 0
#define DEBUG_SWAP 0
#define DEBUG_DATA 0
#define DEBUG_MEMORY 0

#define PROG_NAME_SIZE 128

/* KE: Commented this out.  Not found on WIN32 and apparently not needed */
#if 0
#ifndef SCO
#include <sys/uio.h>
#endif
#endif

#include <stdio.h>
#include <stdlib.h>
#ifndef WIN32
#include <pwd.h>
#endif

/* Moved these here to avoid Exceed 6 problems */
#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/XWDFile.h>

#include "printUtils.h"
#include "getopt.h"     /* Use local version */

/* Function prototypes */
static int get_next_raster_line(FILE *file, XWDFileHeader *win,
  unsigned char *linec,  unsigned char *buffer);
static void skipLines(FILE *file, XWDFileHeader *winP, unsigned char *line,
  unsigned char *buffer, int line_skip);
static void parseArgs(int argc, char **argv, Options *option, Image *image,
    Page *page);
static int get_page_info(Page *the_page);
static int get_raster_header(FILE *file, XWDFileHeader *win, char *w_name);
static int getDumpType(XWDFileHeader *header);
static int getOrientation(Page pg, Image im);

static unsigned char *line = NULL;      /* raster line buffer */
static unsigned char *line4bits = NULL; /* raster line buffer for unpacking 4 bit images */
static XColor *colors = NULL;           /* the color map */
char progname[PROG_NAME_SIZE];

int redshift,greenshift,blueshift;
int redadjust,greenadjust,blueadjust;
int swapData;

/* this is a structure containg the values of all command line options
   initially, we assume no options are set */
Flag nullFlag = { FALSE, FALSE, FALSE, FALSE, FALSE,
	      FALSE, FALSE, FALSE, FALSE, FALSE,
	      FALSE, FALSE, FALSE, FALSE, FALSE };
Flag flag;

int xwd2ps(int argc, char **argv, FILE *fo)
{
#ifndef WIN32
    struct passwd *pswd;     /* variables for userid, date and time */
    char hostname[256];           /* name of host machine */
#endif

    extern int optind;
    FILE *incfile = NULL;
    FILE *file = NULL;

    XWDFileHeader win;

  /* program options */
    Options options;

  /* variables for image size */
    Image my_image;

  /* variables for image data */
    unsigned char intens;
    int color, runlen;
    int c, i, j, im, intwidth;
    int outputcount;
  /*, image.ps_width, image.ps_height, image.pixels_width; */
    int line_skip, line_end, col_skip, col_start, col_end;
#ifndef VMS
    long swaptest = 1;
#else
  /* ACM:  This empirically works on VMS */
    long swaptest = 0;
#endif

    unsigned long intensity0, intensity1;

  /*  page size and image positioning */
    Page page;

    float maxhcheck, maxwcheck;

    float ha, wa;

    int dump_type = UNKNOWN;

    unsigned char *buffer=NULL;   /* temporary work buffer */
    unsigned char rr, gg, bb;
    char *w_name = NULL;          /* window name from X11-Window xwd header */
    char s_translate[80];         /* PostScript code to position image */
    char s_scale[80];             /* PostScript code to scale image */
    char s_matrix[80];            /* PostScript code for image matrix */

    int redbits,greenbits,bluebits;

    int retCode = 1;

  /* initializations */
    colors = NULL;
    strncpy(progname, argv[0], PROG_NAME_SIZE); /* copy the program name to a global */
    progname[PROG_NAME_SIZE-1]='\0';
    strcpy(page.type, "letter");   /* default page type */
    options.title.height    = 0.0; /* no title */
    options.title.string    = (char *)malloc(256);
    options.title.font_size = 10;  /* 16 point font */
    options.ncopies         = 1;   /* 1 copy of output */
    options.input_file.name = "standard input";
    page.yoffset            = 0.0; /* position adjustment  (for logo, time & title)*/
    page.height_adjust      = 0.0; /* image height adjustment (for logo, time & title) */
    page.time_date_height   = 0.0; /* height (inches) of time & date characters   */
    my_image.height_frac    = 1.0;
    my_image.width_frac     = 1.0;
    my_image.orientation    = PORTRAIT;

#if DEBUG_SWAP
    if (*(char *) &swaptest) {
	print("LSBFirst\n");
    } else {
	print("MSBFirst\n");
    }
#endif

  /* get the command-line flags and options */
    parseArgs(argc, argv, &options, &my_image, &page);

  /* determine the orientation of the image if it is specified,
     otherwise orientation is determined by the image shape later on */

    if(flag.portrait == TRUE && flag.landscape == TRUE) {
	errMsg( "%s: cannot use L and P options together.\n",progname);
	flag.error++;
    }
    else if(flag.portrait == TRUE)
      my_image.orientation = PORTRAIT;
    else if(flag.landscape == TRUE)
      my_image.orientation = LANDSCAPE;

    if(flag.logo) {
	page.height_adjust += LOGOHEIGHT;
	page.yoffset       += LOGOHEIGHT;
    }

  /* Get page size information */

    if(get_page_info(&page)) {
	errMsg( "%s: p option error -- page type: %s undefined\n",
	  progname, page.type);
	flag.error++;
    }

    if(flag.error) {
	retCode = 0;
	goto CLEAN;
    }

  /*
   * make correction to positioning for annotation
   *
   *                     default fontsize = 10; .12 added for vertical spacing
   */
    if((flag.date == TRUE) || (flag.time == TRUE))
      page.time_date_height = (float)(10./72. + .12);

  /*
   *                    .1 added for vertical spacing
   */
    if(flag.title == TRUE)
      options.title.height = (float)(options.title.font_size/72. + .1);

    page.height_adjust += fMax(page.time_date_height, options.title.height);
    page.yoffset       -= fMax(page.time_date_height, options.title.height);


  /*
   * Open the image file
   */
    if(argc > optind) {
	options.input_file.name = argv[optind];
#ifdef WIN32
      /* WIN32 opens files in text mode by default and then mangles CRLF */
	file = fopen(options.input_file.name, "rb");
#else
	file = fopen(options.input_file.name, "r");
#endif
	if(file == NULL) {
	    errMsg("%s: could not open input file %s\n",
	      progname, options.input_file.name);
	    retCode = 0;
	    goto CLEAN;
	}
	options.input_file.pointer = file;
    }

  /*
   * Get rasterfile header info
   */
  /* KE: w_name was not defined.  It does not appear to be used here.
    This is not a valid way to return the name to the program in any
    event.  Set it to NULL.  It will be changed locally in
    get_raster_header, which mallocs some storage and now frees it.
    (It was not freed and caused a MLK previously.  If this routine
    really needs w_name, then the code will have to be written
    properly. */

    w_name=NULL;
    retCode = get_raster_header(file, &win, w_name);
    if(!retCode) goto CLEAN;

    dump_type = getDumpType(&win); /* TEMP result not used yet */

  /*
   * check raster depth
   */
    if(win.pixmap_depth == 1)
      flag.mono = TRUE;

  /*
   * invert values if intensity is screwed up
   */
    intensity0 = INTENSITY(colors[0]);
    intensity1 = INTENSITY(colors[1]);
    if(win.ncolors == 2 && (intensity0 > intensity1)) flag.invert = TRUE;

  /*
   * process width factor
   */
    if(my_image.width_frac == 0 || my_image.width_frac > 1.0) {
	errMsg(
	  "%s: W option argument error -- %f is not between 0 and 1.\n",
	  progname, my_image.width_frac);
	retCode = 0;
	goto CLEAN;
    }

    intwidth = win.pixmap_width;

  /* TEMP fix strap output for binary image data; strap adds a blank column */
    if(win.pixmap_depth == 1)
      intwidth--;

    my_image.ps_width = abs( (int) (intwidth * my_image.width_frac));
    if(my_image.width_frac < 0.0 )
      col_skip = intwidth - my_image.ps_width;
    else
      col_skip = 0;

    col_start = col_skip + 1;
    col_end   = col_skip + my_image.ps_width;

  /* process height factor */
    if(my_image.height_frac == 0 || my_image.height_frac > 1.0) {
	errMsg(
	  "%s: H option argument error -- %f is not between 0 and 1.\n",
	  progname, my_image.height_frac);
	retCode = 0;
	goto CLEAN;
    }
    my_image.ps_height = abs ( (int) (win.pixmap_height * my_image.height_frac) );
    if(my_image.height_frac < 0.0 )
      line_skip =  win.pixmap_height - my_image.ps_height;
    else
      line_skip = 0;

    line_end = line_skip + my_image.ps_height;

    my_image.pixels_width = my_image.ps_width;   /* TEMP - this might have to be fixed later */

    if(flag.w == FALSE && flag.h == TRUE)
      my_image.width = (my_image.height*my_image.pixels_width)/my_image.ps_height;

    if(flag.w == TRUE && flag.h == FALSE)
      my_image.height = (my_image.width*my_image.ps_height)/my_image.pixels_width;


  /* determine which orientation to use if not specified */
    if(flag.portrait == FALSE && flag.landscape == FALSE)
      my_image.orientation = getOrientation(page, my_image);


  /*
   * choose defaults to make image as large as possible on page
   */
    if(flag.w == FALSE && flag.h == FALSE) {
      /* setup page sizes for orientation requested */
	if(my_image.orientation == PORTRAIT) {
	    wa = page.default_width;
	    ha = page.default_height - page.height_adjust;
	}
	else {
	    wa = page.default_height;
	    ha = page.default_width - page.height_adjust;
	}
	if( wa*my_image.ps_height >= ha*my_image.pixels_width) {
	    my_image.height = ha;
	    my_image.width = (my_image.height*my_image.pixels_width)/my_image.ps_height;
	}
	else {
	    my_image.width = wa;
	    my_image.height = (my_image.width*my_image.ps_height)/my_image.pixels_width;
	}
    }

  /*
   * Center & scale image and setup image matrix
   */
    if(my_image.orientation == PORTRAIT) {
	maxwcheck = page.maxwsize;
	maxhcheck = page.maxhsize;
	page.ximagepos = page.xcenter - (my_image.width/2);
	page.yimagepos = page.ycenter - (my_image.height/2) + page.yoffset;
	sprintf(s_matrix,"  [%d 0 0 -%d 0 %d]",
	  my_image.pixels_width,my_image.ps_height,my_image.ps_height);
    }
    else {
	maxwcheck = page.maxhsize;
	maxhcheck = page.maxwsize;
	page.ximagepos = page.xcenter - (my_image.height/2);
	page.yimagepos = page.ycenter - (my_image.width/2) + page.yoffset;
	sprintf(s_matrix,"  [0 %d %d 0 0 0]", my_image.pixels_width,my_image.ps_height);
    }
    sprintf(s_translate, "%f inch %f inch translate", page.ximagepos, page.yimagepos);
    sprintf(s_scale, "%f inch %f inch scale", my_image.width, my_image.height);

    if(my_image.width <= 0.0 || my_image.width > maxwcheck) {
	errMsg(
	  "%s: w option error -- width (%f) is outside range for standard 8.5 x 11 in. page.\n",
	  progname, my_image.width);
	retCode = 0;
	goto CLEAN;
    }

    if(my_image.height <= 0.0 || my_image.height > maxhcheck) {
	errMsg(
	  "%s: h option error -- height (%f) is outside range for standard 8.5 x 11 in. page.\n",
	  progname, my_image.height);
	retCode = 0;
	goto CLEAN;
    }

  /*
   * Output the PostScript program to display the data
   */

    printEPSF(fo, my_image, page, options.input_file.name);

    fprintf(fo,"%% %s -- program written by Robert C. Tatar and Craig A. McGowan.\n",
      progname);
    fprintf(fo,"%% The command used to create this file (missing quotes on strings):\n%%  ");
    for (i=0; i< argc; i++)
      fprintf(fo," %s", argv[i]);
    fprintf(fo,"\n");
#if !defined WIN32 && !defined VMS
    pswd = getpwuid (getuid ());
    gethostname (hostname, sizeof hostname);
    fprintf(fo,"%% by %s:%s (%s)\n",hostname, pswd->pw_name, pswd->pw_gecos);
#endif
    fprintf(fo,"%% Information from XWD rasterfile header:\n");
    fprintf(fo,"%%   width =  %d, height = %d, depth = %d\n",
      (int)win.pixmap_width, (int)win.pixmap_height, (int)win.pixmap_depth);
    fprintf(fo,"%%   file_version = %d, pixmap_format = %d, byte_order = %d\n",
      (int)win.file_version, (int)win.pixmap_format, (int)win.byte_order);
    fprintf(fo,"%%   bitmap_unit = %d, bitmap_bit_order = %d, bitmap_pad = %d\n",
      (int)win.bitmap_unit, (int)win.bitmap_bit_order, (int)win.bitmap_pad);
    fprintf(fo,"%%   bits_per_pixel = %d, bytes_per_line = %d, visual_class = %d\n",
      (int)win.bits_per_pixel, (int)win.bytes_per_line, (int)win.visual_class);
    fprintf(fo,"%%   bits/rgb = %d, colormap entries = %d, ncolors = %d\n",
      (int)win.bits_per_rgb, (int)win.colormap_entries, (int)win.ncolors);
    fprintf(fo,"%% Portion of raster image in this file:\n");
    fprintf(fo,"%%   starting line = %d\n",line_skip+1);
    fprintf(fo,"%%   ending line = %d\n",line_end);
    fprintf(fo,"%%   starting column = %d\n",col_start);
    fprintf(fo,"%%   ending column = %d\n",col_end);
    fprintf(fo,"gsave\n");

    fprintf(fo,"/inch {72 mul} def\n");

    if(flag.gamma == TRUE)
      fprintf(fo,"{ %f exp } settransfer\n", my_image.gamma);

  /* allocate global static memory */
  /* KE: Used to be hard-coded for max screen size of 1280.  It should
   * use the width rather than the screen size and should be dynamic
   * to handle any width. */
    if(!line) {
      /* raster line buffer, max size */
#if 0	
	Screen *scr_ptr = DefaultScreenOfDisplay(display);
	size_t bytes = 3*WidthOfScreen(scr_ptr);
#else	
	size_t bytes = 3*win.pixmap_width;
#endif	
#if DEBUG_MEMORY
	printf("xwd2ps: line=%d bytes\n",(int)bytes);
#endif
	line=(unsigned char *)malloc(bytes);
	if(!line) {
	    errMsg("xwd2ps: Error allocating intensity map.\n");
	    retCode = 0;
	    goto CLEAN;
	}
    }
    if(!line4bits) {
      /* raster line buffer for unpacking 4 bit images */
#if 0	
	Screen *scr_ptr = DefaultScreenOfDisplay(display);
	size_t bytes = WidthOfScreen(scr_ptr);
#else	
	size_t bytes = win.pixmap_width;
#endif	
#if DEBUG_MEMORY
	printf("xwd2ps: line4bits=%d bytes\n",(int)bytes);
#endif
	line4bits=(unsigned char *)malloc(bytes);
	if(!line4bits) {
	    errMsg("xwd2ps: Error allocating intensity map.\n");
	    retCode = 0;
	    goto CLEAN;
	}
    }
  /* get a buffer to hold each line of the image */
#if DEBUG_MEMORY
    printf("xwd2ps: buffer=%d bytes width=%d (x3=%d)\n"
      " depth=%d bits_per_pixel=%d bytes_per_pixel=%d\n",
      (int)win.bytes_per_line,
      (int)win.pixmap_width,3*(int)win.pixmap_width,
      (int)win.pixmap_depth,
      (int)win.bits_per_pixel,(int)win.bits_per_pixel/8);
#endif
    buffer = (unsigned char *) malloc( win.bytes_per_line );
    if(!buffer) {
	errMsg("xwd2ps: Error allocating line buffer.\n");
	retCode = 0;
	goto CLEAN;
    }

  /* switch based on image depth */
    switch (win.pixmap_depth) {
    case 32:   /* 32 bit image */
    case 24:   /* 24 bit image */
    case 16:   /* 16 bit image */
	if(flag.mono == FALSE) {   /* print as color image */
	    fprintf(fo,"/buffer 4 string def\n");
	    fprintf(fo,"/rgb 3 string def\n");
	    outputColorImage(fo);
	    fprintf(fo,"/drawfullcolorimage {\n");
	    fprintf(fo,"  %4d %4d 8\n",my_image.pixels_width,my_image.ps_height);
	    fprintf(fo,"%s\n", s_matrix);
	    fprintf(fo,"  { currentfile buffer readhexstring pop pop %% get runlength and rgb values\n");
	    fprintf(fo,"    /npixels buffer 0 get 1 add 3 mul store  %% number of pixels\n");
	    fprintf(fo,"    /pixels npixels string store             %% create string to hold rgb values\n");
	    fprintf(fo,"    /rgb buffer 1 3 getinterval store        %% red green blue values\n");
	    fprintf(fo,"    0 3 npixels -1 add {\n");
	    fprintf(fo,"      pixels exch rgb putinterval\n");
	    fprintf(fo,"    } for\n");
	    fprintf(fo,"  pixels                                     %% return string of colors\n");
	    fprintf(fo,"  }\n");
	    fprintf(fo,"  false 3\n");
	    fprintf(fo,"  colorimage\n");
	    fprintf(fo,"} bind def\n");
	    fprintf(fo,"\n");
	    fprintf(fo,"%s\nmatrix currentmatrix\n%s\n", s_translate, s_scale);
	    fprintf(fo,"\ndrawfullcolorimage\n");
	}
	else {      /*  print as monochrome image */
	    fprintf(fo,"/buffer 2 string def\n");
	    fprintf(fo,"/drawimage {\n");
	    fprintf(fo,"  %4d %4d 8\n",my_image.pixels_width,my_image.ps_height);
	    fprintf(fo,"%s\n", s_matrix);
	    fprintf(fo,"  { currentfile buffer readhexstring pop pop\n");
	    fprintf(fo,"    /npixels buffer 0 get store\n");
	    fprintf(fo,"    /grayscale buffer 1 get store\n");
	    fprintf(fo,"    /pixels npixels 1 add string store\n");
	    fprintf(fo,"       grayscale 0 ne {\n");
	    fprintf(fo,"         0 1 npixels {pixels exch grayscale put } for\n");
	    fprintf(fo,"       } if\n");
	    fprintf(fo,"    pixels\n");
	    fprintf(fo,"  }\n");
	    fprintf(fo,"  image\n");
	    fprintf(fo,"} bind def\n");
	    fprintf(fo,"\n");
	    fprintf(fo,"%s\nmatrix currentmatrix\n%s\n", s_translate, s_scale);
	    fprintf(fo,"\ndrawimage\n");
	}
	skipLines(file, &win, &line[0], buffer, line_skip);

      /* determine if the image data order is the same as the hardware
         order, and set up to swap the data if not */
	if((!(*(char *)&swaptest)) ^ win.byte_order) swapData=1;
	else swapData=0;

      /* determine shifts and number of bits used in the masks */
	redshift=greenshift=blueshift=-1;
	redbits=greenbits=bluebits=0;
	for(i=0; i < 32; i++) {
	    if((win.red_mask>>i)&1) {
		if(redshift < 0) redshift=i;
		redbits++;
	    }
	    if((win.green_mask>>i)&1) {
		if(greenshift < 0) greenshift=i;
		greenbits++;
	    }
	    if((win.blue_mask>>i)&1) {
		if(blueshift < 0) blueshift=i;
		bluebits++;
	    }
	}

      /* determine shift to put most significant bit in position 7 */
	redadjust=8-redbits;
	greenadjust=8-greenbits;
	blueadjust=8-bluebits;

#if DEBUG_DATA
	print("\nred_mask=%08x green_mask=%08x blue_mask=%08x\n",
	  win.red_mask,win.green_mask,win.blue_mask);
	print("redshift=%d greenshift=%d blueshift=%d\n",
	  redshift,greenshift,blueshift);
	print("redadjust=%d greenadjust=%d blueadjust=%d\n",
	  redadjust,greenadjust,blueadjust);
	print("line_skip=%d line_end=%d col_skip=%d col_start=%d col_end=%d\n",
	  line_skip,line_end,col_skip,col_start,col_end);
	printf("width=%d height=%d depth=%d\n",
	  (int)win.pixmap_width,(int)win.pixmap_height,(int)win.pixmap_depth);
#endif

      /* check validity*/
	if(redshift < 0 || greenshift < 0 || blueshift < 0 ||
	    redadjust < 0 || greenadjust < 0 || blueadjust < 0) {
	    errMsg(
	      "%s: red, green, or blue mask not specified or invalid.\n",
	      progname);
	    retCode = 0;
	    goto CLEAN;
	}
	if(win.bits_per_pixel > 32 || win.bits_per_pixel/8 <= 0) {
	    errMsg(
	      "%s: Invalid bits_per_pixel.\n", progname);
	    retCode = 0;
	    goto CLEAN;
	}

      /* start outputting the image */
	outputcount = 0;
	for (i = 0; i < my_image.ps_height; i++)  {
	    retCode = get_next_raster_line(file, &win, line, buffer);
	    if(!retCode) goto CLEAN;
	  /* Fix next 3 lines per Brian McAllister */
	    rr = line[3*col_skip];
	    gg = line[3*col_skip+1];
	    bb = line[3*col_skip+2];
#if DEBUG_DATA
	    if(i == 0) {
		print("\nBuffer start: %02x %02x %02x %02x 0x%08lx\n",
		  (int)buffer[0],(int)buffer[1],(int)buffer[2],
		  (int)buffer[3],*(unsigned long *)buffer);
		print("              rr=%02x gg=%02x bb=%02x\n",
		  (int)line[0],(int)line[1],(int)line[2]);
	    }
#endif
	    runlen = 0;
	    for(j = 3*col_start; j < 3*col_end; j += 3) {
		if(flag.black2white == TRUE &&
		  line[j] == 0 && line[j+1] == 0 && line[j+2] == 0 ) {
		    line[j]   = 255;
		    line[j+1] = 255;
		    line[j+2] = 255;
		}
	      /* If the colors are the same, just keep counting, but
	       * make sure that the run length will fit in a byte (256
	       * max, i.e. 0 means length 1, 1 means length 2, etc.)
	       * */
		if(line[j] == rr && line[j+1] == gg && line[j+2] == bb) {
		    if(runlen == 0xff) {
			if(flag.mono == FALSE) {
			    fprintf(fo,"%02x%02x%02x%02x", runlen, rr, gg, bb);
			    outputcount += 4;
			}
			else {
			    intens = (unsigned char)(0.299*rr + 0.587*gg +
			      0.114*bb);
			    fprintf(fo,"%02x%02x", runlen, intens);
			    outputcount += 2;
			}
			runlen = -1;
		    }
		    runlen++;
		} else {
		  /* if the color changed, then output the last run
                     and reset counter */
		    if(flag.mono == FALSE) {
			fprintf(fo,"%02x%02x%02x%02x", runlen, rr, gg, bb);
			outputcount += 4;
		    }  else {
			intens = (unsigned char)(0.299*rr + 0.587*gg + 0.114*bb);
			fprintf(fo,"%02x%02x", runlen, intens);
			outputcount += 2;
		    }
		    runlen = 0;
		  /* colors are stored as r,g,b */
		    rr = line[j];
		    gg = line[j+1];
		    bb = line[j+2];
		}
	      /* check to make sure the output lines are not too long */
		if(outputcount >= MAXPERLINE) {
		    outputcount = 0;
		    fprintf(fo,"\n");
		}
	    }
	    if(flag.mono == FALSE) {
		fprintf(fo,"%02x%02x%02x%02x", runlen, rr, gg, bb);
		outputcount += 4;
	    } else {
		intens = (unsigned char)(0.299*rr + 0.587*gg + 0.114*bb);
		fprintf(fo,"%02x%02x", runlen, intens);
		outputcount += 2;
	    }
	}
	fprintf(fo,"\n");
	break;

    case 8:  /* eight bit image */
	fprintf(fo,"/buffer 2 string def\n");
	fprintf(fo,"/rgbmap %d string def\n", (int)(3*win.ncolors));
	fprintf(fo,"/rgb (000) def\n");
	fprintf(fo,"/pixels %d string def\n", (int)(3*win.ncolors));
	outputColorImage(fo);
	fprintf(fo,"/drawcolorimage {\n");
	fprintf(fo,"  %d %d %d\n",my_image.pixels_width,my_image.ps_height,
	  (int)win.pixmap_depth);
	fprintf(fo,"%s\n", s_matrix);
	fprintf(fo,"  {currentfile buffer readhexstring pop pop  %% get run length & color info\n");
	fprintf(fo,"    /npixels buffer 0 get 1 add 3 mul store  %% number of pixels (run length)\n");
	fprintf(fo,"    /color buffer 1 get 3 mul store          %% color of pixels\n");
	fprintf(fo,"    %% /pixels npixels string store          %% create string to hold colors\n");
	fprintf(fo,"    /rgb rgbmap color 3 getinterval store    %% get rgb value\n");
	fprintf(fo,"    0 3 npixels -1 add {\n");
	fprintf(fo,"	  pixels exch rgb putinterval\n");
	fprintf(fo,"    } for\n");
	fprintf(fo,"    pixels 0 npixels getinterval             %% Return color values\n");
	fprintf(fo,"  }\n");
	fprintf(fo,"  false 3\n");
	fprintf(fo,"  colorimage\n");
	fprintf(fo,"} bind def\n");
	fprintf(fo,"%s\nmatrix currentmatrix\n%s\n", s_translate, s_scale);
      /* Write out color table for color image */
	fprintf(fo,"\n%% get rgb color table\n");
	fprintf(fo,"currentfile rgbmap readhexstring pop pop\n");
	for(im = 0; im < (int)win.ncolors; im++) {
	    if(flag.black2white == TRUE) {
		if(colors[im].red == 255 && colors[im].green == 255 && colors[im].blue == 255) {
		    colors[im].red = 0;
		    colors[im].green = 0;
		    colors[im].blue = 0;
		}
		else if(colors[im].red == 0 && colors[im].green == 0 && colors[im].blue == 0) {
		    colors[im].red = 255;
		    colors[im].green = 255;
		    colors[im].blue = 255;
		}
	    }

#ifdef GREY_PRINTS_WHITE
	  /* ACM: Use if you use a standard grey scale as background,
             and want white on the printer */
            if(colors[im].red == 0xc8 &&
	      colors[im].green == 0xc8 &&
	      colors[im].blue == 0xc8) {
		colors[im].red = 255;
		colors[im].green = 255;
		colors[im].blue = 255;
            }
#endif

	  /* KE: The following mask is reported to solve problems on
             DecUnix.  The cause, however, may be that the swaptest
             and shift logic in get_raster_header is not right. It
             shouldn't hurt in any event.  */
	    fprintf(fo,"%02x",   colors[im].red&0xff);
	    fprintf(fo,"%02x",   colors[im].green&0xff);
	    fprintf(fo,"%02x\n", colors[im].blue&0xff);
	}
	fprintf(fo,"\n");
	fprintf(fo,"\ndrawcolorimage\n");

	skipLines(file, &win, &line[0], buffer, line_skip);
      /* run thru each row of pixels and run-len length encode it */
	outputcount = 0;
	for (i = 0; i <my_image.ps_height; i++)  {
	    get_next_raster_line(file, &win, line, buffer);
	    runlen = 0;
	    color = line[col_skip];
	    for(j = col_start; j < col_end; j++) {
	      /*
		If the colors are the same, just keep counting, but make sure that
		the run length will fit in a byte (256 max, i.e. 0 means length 1,
		1 means length 2, etc.)
		*/
		if(line[j] == color) {
		    if(runlen == 0xff) {
			fprintf(fo,"%02x%02x", runlen, color);
			outputcount++;
			runlen = -1;
		    }
		    runlen++;
		}
	      /*
	       * if the color changed, then output the last run
	       */
		else {
		    fprintf(fo,"%02x%02x", runlen, color);
		    outputcount++;
		    runlen = 0;
		    color = line[j];
		}

	      /*
	       * check to make sure the output lines are not too long
	       */
		if(outputcount > MAXPERLINE) {
		    outputcount = 0;
		    fprintf(fo,"\n");
		}
	    }
	    fprintf(fo,"%02x%02x", runlen, color);
	    outputcount++;
	}

	break;

    case 4:  /* four-bit image */
	if(flag.mono == FALSE) { /* print as color image */
	    fprintf(fo,"/buffer 2 string def\n");
	    fprintf(fo,"/rgbmap %d string def\n", (int)(3*win.ncolors));
	    fprintf(fo,"/rgb (000) def\n");
	    outputColorImage(fo);
	    fprintf(fo,"/drawcolorimage {\n");
	    fprintf(fo,"  %d %d %d\n",my_image.pixels_width,my_image.ps_height,
	      (int)win.pixmap_depth*2);
	    fprintf(fo,"%s\n", s_matrix);
	    fprintf(fo,"  {currentfile buffer readhexstring pop pop  %% get run length & color info\n");
	    fprintf(fo,"    /npixels buffer 0 get 1 add 3 mul store  %% number of pixels (run length)\n");
	    fprintf(fo,"    /color buffer 1 get 3 mul store          %% color of pixels\n");
	    fprintf(fo,"    /pixels npixels string store             %% create string to hold colors\n");
	    fprintf(fo,"    /rgb rgbmap color 3 getinterval store    %% get rgb value\n");
	    fprintf(fo,"    0 3 npixels -1 add {\n");
	    fprintf(fo,"	  pixels exch rgb putinterval\n");
	    fprintf(fo,"    } for\n");
	    fprintf(fo,"    pixels                                   %% Return color values\n");
	    fprintf(fo,"  }\n");
	    fprintf(fo,"  false 3\n");
	    fprintf(fo,"  colorimage\n");
	    fprintf(fo,"} bind def\n");
	    fprintf(fo,"%s\nmatrix currentmatrix\n%s\n", s_translate, s_scale);
	  /* Write out color table for color image */
	    fprintf(fo,"\n%% get rgb color table\n");
	    fprintf(fo,"currentfile rgbmap readhexstring pop pop\n");
	    for(im = 0; im < (int)win.ncolors; im++) {
	      /* KE: The following mask is reported to solve problems
		 on DecUnix.  The cause, however, may be that the
		 swaptest and shift logic in get_raster_header is not
		 right. It shouldn't hurt in any event.  */
		fprintf(fo,"%02x",   colors[im].red&0xff);
		fprintf(fo,"%02x",   colors[im].green&0xff);
		fprintf(fo,"%02x\n", colors[im].blue&0xff);
	    }
	    fprintf(fo,"\n");
	    fprintf(fo,"\ndrawcolorimage\n");
	}

	skipLines(file, &win, &line[0], buffer, line_skip);
      /* run thru each row of pixels and run-len length encode it */
	outputcount = 0;

	for (i = 0; i <my_image.ps_height; i++)  {
	    get_next_raster_line(file, &win, line, buffer);
	  /* unpack the 4 bit images into a byte */
	    if(*(char *) & swaptest)
	      for (j = 0; j < (int)win.bytes_per_line; j++) {
		  line4bits[2*j]   = line[j] & 0x0f;
		  line4bits[2*j+1] = line[j] >> 4;
	      }
	    else
	      for (j = 0; j < (int)win.bytes_per_line; j++) {
		  line4bits[2*j+1]   = line[j] & 0x0f;
		  line4bits[2*j] = line[j] >> 4;
	      }

	  /* treat it like a Color, 8-bit, colormapped image
	   * Start looking for runs of the same color.
	   */
	    runlen = 0;
	    color = line4bits[col_skip];
	    for(j = col_start; j < col_end; j++) {
	      /*
		If the colors are the same, just keep counting, but make sure that
		the run length will fit in a byte (256 max, i.e. 0 means length 1,
		1 means length 2, etc.)
		*/
		if(line4bits[j] == color) {
		    if(runlen == 0xff) {
			fprintf(fo,"%02x%02x", runlen, color);
			outputcount++;
			runlen = -1;
		    }
		    runlen++;
		}
	      /*
	       * if the color changed, then output the last run
	       */
		else {
		    fprintf(fo,"%02x%02x", runlen, color);
		    outputcount++;
		    runlen = 0;
		    color = line4bits [j];
		}

	      /*
	       * check to make sure the output lines are not too long
	       */
#ifdef NOTDEF
		if(outputcount > MAXPERLINE) {
		    outputcount = 0;
		    fprintf(fo,"\n");
		}
#endif
	    }
	    fprintf(fo,"%02x%02x", runlen, color);
#ifndef NOTDEF
	    fprintf(fo,"\n");
#endif
	    outputcount++;
	}

	break;

    case 1:  /* Monochrome 1-bit, binary image */
      /* setup for binary image */
	fprintf(fo,"/buffer %d string def\n",my_image.ps_width);
	fprintf(fo,"/drawbinaryimage {\n");
	fprintf(fo,"  %d %d %2d\n", my_image.pixels_width, my_image.ps_height,
	  (int)win.pixmap_depth);
	fprintf(fo,"%s\n",s_matrix);
	fprintf(fo,"  { currentfile buffer readhexstring pop pop buffer }\n");
	fprintf(fo,"  image\n");
	fprintf(fo,"} bind def\n");
	fprintf(fo,"\n");
	fprintf(fo,"%s\nmatrix currentmatrix\n%s\n", s_translate, s_scale);
	fprintf(fo,"\ndrawbinaryimage\n");

	skipLines(file, &win, &line[0], buffer, line_skip);
      /* run through each row of pixels (8 per byte) and copy it  */
	outputcount = 0;
	for (i = 0; i < my_image.ps_height; i++)  {
	    get_next_raster_line(file, &win, line, buffer);
	    for(j=col_skip; j < col_end; j++) {
		fprintf(fo,"%02x", 255 - line[j]);       /* invert data for PostScript */
		outputcount++;
	      /* check to make sure the output lines are not too long	 */
		if(outputcount > MAXPERLINE) {
		    outputcount = 0;
		    fprintf(fo,"\n");
		}
	    }
	    outputcount = 0;
	    fprintf(fo,"\n");
	}
	break;

    default:
	errMsg("%s: Sorry! cannot handle input file of this depth [%d]",
	  progname,win.pixmap_depth);
	retCode = 0;
	goto CLEAN;
    }

  /* Necessary to get back to original CTM scaling */
    switch (win.pixmap_depth) {
    case 8:
	fprintf(fo,"\npop pop setmatrix\n");
	break;
    case 32:
    case 24:
    case 16:
	fprintf(fo,"\nsetmatrix\n");
	break;
    }

  /*
   * Add border if required
   */
    if(flag.border == TRUE)
      outputBorder(fo,my_image);

  /*
   * Copy include file if required
   */
  /* KE: incfile was not defined.  Define it. (This change should be
     tested if it is really desired to use this feature.) */
    incfile=options.inc_file.pointer;
    if(flag.inc_file == TRUE) {
      /* Silicon Graphics did not recognize EOF so cludge this way */
#ifdef mips
	while((c = fgetc(incfile)) != ((char) -1))
	  putchar(c);
#else
	while((c = fgetc(incfile)) != EOF)
	  putchar(c);
#endif
	fclose(incfile);
    }

    if(flag.date == TRUE)
      outputDate(fo, my_image);

    if(flag.title == TRUE)
      outputTitle(fo, my_image, options);

    if(flag.time == TRUE)
      outputTime(fo, my_image);

    if(flag.logo == TRUE)
      outputLogo(fo, my_image);

  /* finish up PostScript code */
    fprintf(fo,"\n/#copies %d def\n",options.ncopies);
    fprintf(fo,"showpage grestore\n%%%%Trailer\n");

  CLEAN:
  /* Close the input file */
    if(file) fclose(file);

  /* Clean up everything that may have been allocated */
    if(buffer) {
	free(buffer);
	buffer = NULL;
    }
    if(options.title.string) {
	free(options.title.string);
	options.title.string = NULL;
    }
    if(colors) {
	free(colors);
	colors = NULL;
    }
    if(w_name) {
	free(w_name);
	w_name = NULL;
    }
    if(line) {
	free(line);
	line = NULL;
    }
    if(line4bits) {
	free(line4bits);
	line4bits = NULL;
    }

    return retCode;
}
/* End of Main Program */


/*
 ** getDumpType() - returns an int describing the type of dump we have
 */

static int getDumpType(XWDFileHeader *header)
{
    switch(header->bits_per_pixel) {
    case 32:
    case 24:
    case 16:
    case 12:
    case 8:
    case 4:
	return(1); /* TEMP */
    case 2:
    case 1:
    default:
	errMsg( "can't handle %u bits_per_pixel\n", header->bits_per_pixel);
	return UNKNOWN; /* TEMP */
    }
}


/*
 **  get_page_info() - return size information about the output page
 *
 * returns information about the size (in inches) of the paper intended
 * for the PostScript output.  The numbers are optimized for the
 * QMS ColorScript 100 printer (manual page 10-19).
 *
 *  Written January 1989 by Robert C. Tatar
 *  Last Modification 2-1-89 by RCT.
 */

static int get_page_info(Page *the_page)
{
  /* letter paper size 8.5 x 11 */
    if( (strcmp(the_page->type, "A") == 0) || (strcmp(the_page->type, "letter") == 0) ) {
	the_page->xcenter = (float)(8.5/2);
	the_page->ycenter = (float)(11.0/2. + .21);
	the_page->maxwsize = (float)8.11;
	the_page->maxhsize = (float)8.91;
	the_page->default_width = (float)7.0;
	the_page->default_height = (float)8.5;
	return(0);
    }
    else if(strcmp(the_page->type, "B") == 0) {         /* 11 x 17 inch paper size */
	the_page->xcenter = (float)(11.0/2.);
	the_page->ycenter = (float)(17.0/2. + .21);
	the_page->maxwsize = (float)10.60;
	the_page->maxhsize = (float)14.91;
	the_page->default_width = (float)9.0;
	the_page->default_height = (float)14.5;
	return(0);
    }
    else if(strcmp(the_page->type, "A4") == 0 ) {       /* A4 paper size 8.3 x 11.7 */
	the_page->xcenter = (float)(8.3/2.);
	the_page->ycenter = (float)(11.7/2. + .21);
	the_page->maxwsize = (float)8.26;
	the_page->maxhsize = (float)9.60;
	the_page->default_width = (float)7.25;
	the_page->default_height = (float)9.0;
	return(0);
    }
    else if(strcmp(the_page->type, "A3") == 0 ) {       /* A3 paper size 11.7 x 16.6 */
	the_page->xcenter = (float)(11.7/2.);
	the_page->ycenter = (float)(16.6/2. + .21);
	the_page->maxwsize = (float)11.69;
	the_page->maxhsize = (float)16.53;
	the_page->default_width = 9.0;
	the_page->default_height = 14.0;
	return(0);
    }
    return(1);
}

/*
 ** get_raster_header() - read in the xwd header, including the colormap
 **                       and the window title
 */
static int get_raster_header(FILE *file, XWDFileHeader *win, char *w_name)
{
#ifndef VMS
    unsigned long swaptest = 1;
#else
  /* ACM:  This empirically works on VMS */
    unsigned long swaptest = 0;
#endif
    int i, zflg, idifsize;

#if DEBUG_FORMAT
    print("get_raster_header: stdin->_file=%d\n",stdin->_file);
#endif

  /* read in window header */
    fread((char *)win, sizeof( *win ), 1, file);

  /* KE: This just undoes what xwd did for the header */
    if(*(char *) & swaptest)  /* am I running on a byte swapped machine? */
      xwd2ps_swaplong((char *)win, (long)sizeof(*win));

    if(win->file_version != XWD_FILE_VERSION) {
	errMsg("%s: File format version missmatch\n"
	  "  Need %d     Got %d",
	  progname, XWD_FILE_VERSION, win->file_version);
	return 0;
    }

    if(win->header_size < sizeof(*win)) {
	errMsg("%s: Header size is too small.\n", progname);
	return 0;
    }

    if(win->pixmap_depth != 1 && win->pixmap_format != ZPixmap) {
	errMsg("%s: Image is not in Z format\n", progname);
	return 0;
    }

  /* set z pixmap flag if required for operations */

    if( win->pixmap_depth != 1 && win->pixmap_format == ZPixmap)
      zflg= win->bits_per_pixel / 8;
    else
      zflg=0; /* TEMP - as far as I can tell, zflg is never used ! */

    if(win->byte_order != win->bitmap_bit_order) {
	errMsg( "%s: Image will be incorrect\n"
	  "  Byte swapping required but not performed.\n", progname);
    }

  /* TEMP - put in large image warning here! */
    idifsize = (unsigned)(win->header_size - sizeof *win);
    if(idifsize) {
	w_name = (char *)malloc(idifsize);
	fread(w_name, idifsize, 1, file);
      /* KE: Freed w_name to avoid MLK (Doesn't appear to be used anyway) */
	if(w_name) {
	    free(w_name);
	    w_name = NULL;
	}
    }

    if(win->ncolors) {
	colors = (XColor *)malloc((unsigned) (win->ncolors * sizeof(XColor)));
	if(colors ==  NULL) {
	    errMsg( "%s: Cannot allocate memory for color map\n", progname);
	    return 0;
	}
	fread((char *)colors, win->ncolors, sizeof(XColor), file);
      /*
       * Scale the values received from the colormap to 8 bits
       */
#if DEBUG_SWAP
	print("get_raster_header: swap=%d win->byte_order=%d\n",
	  *(char *)&swaptest,win->byte_order);
#endif

	if((!*(char *)&swaptest) || win->byte_order) {
	    for(i = 0; i < (int)win->ncolors; i++) {
#if DEBUG_SWAP
		print("Before: %3d %4hx %4hx %4hx %6x\n",
		  i,colors[i].red,colors[i].green,colors[i].blue,colors[i].pixel);
#endif
		colors[i].red   = colors[i].red   >> 8;
		colors[i].green = colors[i].green >> 8;
		colors[i].blue  = colors[i].blue  >> 8;
#if DEBUG_SWAP
		print("After:  %3d %4hx %4hx %4hx %6x\n",
		  i,colors[i].red,colors[i].green,colors[i].blue,colors[i].pixel);
#endif
	    }
	}
    }     /* if(win->ncolors) */

    return 1;
}

/*
 ** get_next_raster_line() -  returns the next raster line in the image.
 * written 2-12-89 by R.C.Tatar
 */
static int get_next_raster_line(FILE *file, XWDFileHeader *win,
  unsigned char *linec,  unsigned char *buffer)
{
    unsigned char *bufptr, *bufptr1;
    int iwbytes =  win->bytes_per_line;
    int iwbits  =  win->bits_per_pixel;
    int bytes_per_pixel=iwbits/8;
    int j,jmax;

#if DEBUG_DATA
    {
	static int first=1;

	if(first) {
	    first=0;
	    jmax=iwbytes/bytes_per_pixel;
	    print("\nget_next_raster_line");
	    print("iwbytes=%d iwbits=%d bytes_per_pixel=%d jmax=%d\n",
	      iwbytes,iwbits,bytes_per_pixel,jmax);
	}
    }
#endif

    switch(iwbits) {
    case 32:
	fread((char *)buffer, iwbytes, 1, file);
	bufptr1 = linec;
	bufptr = buffer;
      /* KE: buffer size is bytes_per_line=width*bytes_per_pixel,
	     linec size is width*3 */
	jmax=iwbytes/bytes_per_pixel;
	for(j = 0; j < jmax; j++) {
	    unsigned long pixel,red,green,blue;

	    pixel=*(unsigned long *)bufptr;
	  /* swap the data if it is the wrong order for the hardware */
	    if(swapData) {
		xwd2ps_swaplong((char *)&pixel,(long)sizeof(unsigned long));
	    }
	    red = ((pixel&win->red_mask)>>redshift)<<redadjust;
	    green = ((pixel&win->green_mask)>>greenshift)<<greenadjust;
	    blue = ((pixel&win->blue_mask)>>blueshift)<<blueadjust;
	    *bufptr1++ = (unsigned char)red;
	    *bufptr1++ = (unsigned char)green;
	    *bufptr1++ = (unsigned char)blue;
	    bufptr += bytes_per_pixel;
	}
	return 1;

    case 16:
	fread((char *)buffer, iwbytes, 1, file);
	bufptr1 = linec;
	bufptr = buffer;
      /* KE: buffer size is bytes_per_line=width*bytes_per_pixel,
	     linec size is width*3 */
	jmax=iwbytes/bytes_per_pixel;
	for(j = 0; j < jmax; j++) {
	    unsigned long pixel,red,green,blue;
	    unsigned short data;

	    data=*(unsigned short *)bufptr;
	  /* swap the data if it is the wrong order for the hardware */
	    if(swapData) {
		xwd2ps_swapshort((char *)&data,(long)sizeof(unsigned short));
	    }
	    pixel=data;
#if DEBUG_DATA
	    {
		static int first=1;

		if(first) {
		    first=0;
		    print("\ndata=%04hx\n",data);
		    print("pixel=%08lx\n",pixel);
		    print("win->red_mask=%08lx\n",win->red_mask);
		    print("pixel&win->red_mask=%08lx\n",pixel&win->red_mask);
		    print("(pixel&win->red_mask)>>redshift=%08lx\n",
		      (pixel&win->red_mask)>>redshift);
		    print("((pixel&win->red_mask)>>redshift)<<redadjust=%08lx\n",
		      ((pixel&win->red_mask)>>redshift)<<redadjust);
		}
	    }
#endif
	    red = ((pixel&win->red_mask)>>redshift)<<redadjust;
	    green = ((pixel&win->green_mask)>>greenshift)<<greenadjust;
	    blue = ((pixel&win->blue_mask)>>blueshift)<<blueadjust;
	    *bufptr1++ = (unsigned char)red;
	    *bufptr1++ = (unsigned char)green;
	    *bufptr1++ = (unsigned char)blue;
	    bufptr += bytes_per_pixel;
	}
	return 1;

    case 8:
    case 4:
	fread((char *)linec, iwbytes, 1, file);
	return 1;

    default:
      /* error message if no case selected  */
	errMsg("%s: Do not know how to handle %u bits per pixel.\n",
	  progname, iwbits);
	return 0;
    }
}

/*
 ** skipLines() - skip over unprinted parts of the image
 */
static void skipLines(FILE *file, XWDFileHeader *winP, unsigned char *line,
  unsigned char *buffer, int line_skip)
{
    int i;

    for (i = 0; i < line_skip; i++)
      get_next_raster_line(file, winP, line, buffer);
}

/*
 *** parseArgs() - figure out which command line options were set
 */
#include <stdlib.h>
static void parseArgs(int argc, char **argv, Options *option, Image *image,
    Page *page)
{
    char c;
    extern char *optarg;
    extern int optind;

  /* KE: getopt wasn't intended for subroutine usage.  It internally
     sets optind to 1 before the first call.  If we want to use it
     again on the next call, we must set optind ourselves.  This
     wasn't being done so default arguments were being used after the
     first time. */
    flag = nullFlag;
    optind = 1;
    while((c = getOpt(argc, argv, "tdlLPc:s:f:h:w:H:W:mS:p:bg:I")) != EOF)
      switch (c) {
      case 't':
	  flag.time = TRUE;
	  break;
      case 'I':
	  flag.black2white = TRUE;
	  break;
      case 'd':
	  flag.date = TRUE;
	  break;
      case 'm':
	  flag.mono = TRUE;
	  break;
      case 'b':
	  flag.border = TRUE;
	  break;
      case 'l':
	  flag.logo = TRUE;
	  break;
      case 'L':
	  flag.landscape = TRUE;
	  break;
      case 'P':
	  flag.portrait = TRUE;
	  break;
      case 'c':
	  sprintf(optarg, "%d", option->ncopies);
	  break;
      case 's':
	  flag.title = TRUE;
	  strcpy(option->title.string, optarg);
	  break;
      case 'p':
	  strcpy(page->type, optarg);
	  break;
      case 'f':
#ifdef WIN32
	/* KE: This isn't used and opening with "rb" hasn't been tested */
	/* WIN32 opens files in text mode by default and then mangles CRLF */
	  option->inc_file.pointer = fopen(optarg, "rb");
#else
	  option->inc_file.pointer = fopen(optarg, "r");
#endif
	  if(option->inc_file.pointer == NULL) {
	      errMsg("%s: f option error -- cannot open %s for include file.\n",
		progname, optarg);
	      flag.error++;
	  }
	  flag.inc_file = TRUE;
	  break;
      case 'g':
	  flag.gamma = TRUE;
	  if( sscanf(optarg, "%f", &(image->gamma)) == 0 ) {
	      errMsg("%s: g option argument error\n", progname);
	      flag.error++;
	  }
	  if( image->gamma < 0.0 || image->gamma > 1.0 ) {
	      errMsg("%s: Gamma value out of range: %d\n", progname, image->gamma);
	      flag.error++;
	  }
	  break;
      case 'h':
	  flag.h = TRUE;
	  if( sscanf(optarg, "%f", &(image->height)) == 0 ) {
	      errMsg("%s: h option argument error\n", progname);
	      flag.error++;
	  }
	  break;
      case 'w':
	  flag.w = TRUE;
	  if( sscanf(optarg, "%f", &(image->width)) == 0 ) {
	      errMsg("%s: w option argument error\n", progname);
	      flag.error++;
	  }
	  break;
      case 'H':
	  if( sscanf(optarg, "%f", &(image->height_frac)) == 0) {
	      errMsg("%s: H option argument error\n", progname);
	      flag.error++;
	  }
	  break;
      case 'W':
	  if( sscanf(optarg, "%f", &(image->width_frac)) == 0 ) {
	      errMsg("%s: W option argument error\n", progname);
	      flag.error++;
	  }
	  break;
      case 'S':
	  if( sscanf(optarg, "%d", &(option->title.font_size)) == 0 ) {
	      errMsg("%s: S option argument error\n", progname);
	      flag.error++;
	  }
	  break;
      case '?':
	  flag.error++;
      }
}


/*
 *** getOrientation() - get the orientation of the image
 */
/* KE: Not sure what this is doing but it isn't right.  k1 and k2 are
almost always small positive numbers less than 1.  Previously fMax
returned an int, so both were 0 and PORTRAIT was always chosen.  With
fMax defined as an float, the result is the reverse of what you want.
The problem has been fixed by passing the -P (portrait) option.  This
routine will have to be fixed if something else is wanted.  */
static int getOrientation(Page pg, Image im)
{
    float k1, k2, k;

    k1 = (pg.default_height - pg.height_adjust)/im.ps_height;
    k  = pg.default_width/im.pixels_width;
    k1 = fMax(k1, k);

    k2 = (pg.default_width - pg.height_adjust)/im.ps_height;
    k  = pg.default_height/im.pixels_width;
    k2 = fMax(k2, k);

    if(k1 >= k2)
      return(PORTRAIT);
    else
      return(LANDSCAPE);
}
