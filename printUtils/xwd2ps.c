/*
 *** xwd2ps.c - convert X11 window dumps to PostScript
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

#include <X11/Xos.h>
#include <X11/Xlib.h>
#include <X11/XWDFile.h>
#ifndef SCO
#include <sys/uio.h>
#endif 
#include <stdio.h>
#include <pwd.h>

#include "xwd2ps.h"

int intensity_map[4096];    /* max size of color map is 4096 (I hope) */
unsigned char line[1280*3]; /* raster line buffer, max size */
unsigned char line4bits[1280];   /* raster line buffer for unpacking 4 bit images */
XColor *colors;             /* the color map, allocated with "malloc" */
char   progname[128];

/* this is a structure containg the values of all command line options
   initially, we assume no options are set */
Flag flag = { FALSE, FALSE, FALSE, FALSE, FALSE, 
	      FALSE, FALSE, FALSE, FALSE, FALSE, 
	      FALSE, FALSE, FALSE, FALSE, FALSE };

xwd2ps(argc, argv,fo)
    int argc;
    char **argv;
    FILE *fo;
{
    struct passwd *pswd;     /* variables for userid, date and time */

    extern int optind;
    FILE *incfile, *fp;
  
    XWDFileHeader win;

  /* program options */
    Options options;

  /* variables for image size */
    Image my_image;

  /* variables for image data */
    unsigned char intens;
    int color, runlen, gray_index, intensity;
    int c, i, j, im, intwidth, ig;
    int outputcount;
  /*, image.ps_width, image.ps_height, image.pixels_width; */
    int line_skip, line_end, col_skip, col_start, col_end;
    long swaptest = 1;

    unsigned long intensity0, intensity1;

  /*  page size and image positioning */
    Page page;

    float maxhcheck, maxwcheck;

    float ha, wa;

    int dump_type = UNKNOWN;

    unsigned char *buffer;     /* temporary work buffer */
    unsigned char rr, gg, bb;
    char *w_name;                 /* window name from X11-Window xwd header */
    char s_translate[80];         /* PostScript code to position image */
    char s_scale[80];             /* PostScript code to scale image */
    char s_matrix[80];            /* PostScript code for image matrix */
    char hostname[256];           /* name of host machine */

  /* initializations */
    strcpy(progname, argv[0]);     /* copy the program name to a global */
    strcpy(page.type, "letter");   /* default page type */
    options.title.height    = 0.0; /* no title */
    options.title.string    = (char *)malloc(256); /* no title */
    options.title.font_size = 16;  /* 16 point font */
    options.ncopies         = 1;   /* 1 copy of output */
    options.input_file.name = "standard input";
    page.yoffset            = 0.0; /* position adjustment  (for logo, time & title)*/
    page.height_adjust      = 0.0; /* image height adjustment (for logo, time & title) */
    page.time_date_height   = 0.0; /* height (inches) of time & date characters   */
    my_image.height_frac    = 1.0;
    my_image.width_frac     = 1.0;
    my_image.orientation    = PORTRAIT;

  /* get the command-line flags and options */
    parseArgs(argc, argv, &options, &my_image, &page);   

  /* determine the orientation of the image if it specified, otherwise
     orientation is determined by the image shape later on */

    if (flag.portrait == TRUE && flag.landscape == TRUE) {
	fprintf (stderr, "%s: cannot use L and P options together.\n",progname);
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

    if (get_page_info(&page)) {
	fprintf (stderr, "%s: p option error -- page type: %s undefined\n", 
	  progname, page.type);
	flag.error++;
    }

    if (flag.error) {
	usage();
	exit(2);
    }

  /*
   * make correction to positioning for annotation
   *
   *                     default fontsize = 10; .12 added for vertical spacing
   */
    if ((flag.date == TRUE) || (flag.time == TRUE))
      page.time_date_height = 10/72. + .12;

  /*
   *                    .1 added for vertical spacing
   */
    if (flag.title == TRUE)
      options.title.height = options.title.font_size/72. + .1;

    page.height_adjust += fmax(page.time_date_height, options.title.height); 
    page.yoffset       -= fmax(page.time_date_height, options.title.height); 


  /* 
   * Open the image file
   */
    if(argc > optind) {
	options.input_file.name = argv[optind];
	options.input_file.pointer = freopen(options.input_file.name, "r",stdin);
	if (options.input_file.pointer == NULL) {
	    fprintf (stderr,"%s: could not open input file %s\n", 
	      progname, options.input_file.name);
	    exit(1);
	}
    }

  /* 
   * Get rasterfile header info
   */
    get_raster_header(&win, w_name);

    dump_type = getDumpType(&win); /* TEMP result not used yet */

  /*
   * check raster depth
   */
    if (win.pixmap_depth == 1)
      flag.mono = TRUE;
  
  /*
   * invert values if intensity is screwed up
   */
    intensity0 = INTENSITY(colors[0]);
    intensity1 = INTENSITY(colors[1]);
    if (win.ncolors == 2 && (intensity0 > intensity1)) flag.invert = TRUE;

  /*
   * process width factor
   */
    if (my_image.width_frac == 0 || my_image.width_frac > 1.0) {
	fprintf (stderr,
	  "%s: W option argument error -- %f is not between 0 and 1.\n",
	  progname, my_image.width_frac);
	exit(1);
    }

    intwidth = win.pixmap_width;
  
  /* TEMP fix strap output for binary image data; strap adds a blank column */
    if(win.pixmap_depth == 1)
      intwidth--;

    my_image.ps_width = abs( (int) (intwidth * my_image.width_frac));
    if (my_image.width_frac < 0.0 )
      col_skip = intwidth -my_image.ps_width;
    else 
      col_skip = 0;
  
    col_start = col_skip + 1;
    col_end   = col_skip +my_image.ps_width;
  
  /* process height factor */
    if (my_image.height_frac == 0 || my_image.height_frac > 1.0) {
	fprintf (stderr,
	  "%s: H option argument error -- %f is not between 0 and 1.\n",
	  progname, my_image.height_frac);
	exit(1);
    }
    my_image.ps_height = abs ( (int) (win.pixmap_height * my_image.height_frac) );
    if (my_image.height_frac < 0.0 ) 
      line_skip =  win.pixmap_height -my_image.ps_height;
    else 
      line_skip = 0;
  
    line_end = line_skip +my_image.ps_height;
  
    my_image.pixels_width =my_image.ps_width;   /* TEMP - this might have to be fixed later */

    if (flag.w == FALSE && flag.h == TRUE)
      my_image.width = (my_image.height*my_image.pixels_width)/my_image.ps_height;

    if (flag.w == TRUE && flag.h == FALSE)
      my_image.height = (my_image.width*my_image.ps_height)/my_image.pixels_width;


  /* determine which orientation to use if not specified */
    if(flag.portrait == FALSE && flag.landscape == FALSE) 
      my_image.orientation = getOrientation(page, my_image);

  
  /*
   * choose defaults to make image as large as possible on page
   */
    if (flag.w == FALSE && flag.h == FALSE) {
      /* setup page sizes for orientation requested */
	if (my_image.orientation == PORTRAIT) {
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
	sprintf (s_matrix,"  [%d 0 0 -%d 0 %d]",
	  my_image.pixels_width,my_image.ps_height,my_image.ps_height);
    }
    else {
	maxwcheck = page.maxhsize;
	maxhcheck = page.maxwsize;
	page.ximagepos = page.xcenter - (my_image.height/2);
	page.yimagepos = page.ycenter - (my_image.width/2) + page.yoffset;
	sprintf (s_matrix,"  [0 %d %d 0 0 0]", my_image.pixels_width,my_image.ps_height);
    }
    sprintf (s_translate, "%f inch %f inch translate", page.ximagepos, page.yimagepos);
    sprintf (s_scale, "%f inch %f inch scale", my_image.width, my_image.height);

    if (my_image.width <= 0.0 || my_image.width > maxwcheck) {
	fprintf (stderr,
	  "%s: w option error -- width (%f) is outside range for standard 8.5 x 11 in. page.\n", 
	  progname, my_image.width);
	exit(1);
    }
  
    if (my_image.height <= 0.0 || my_image.height > maxhcheck) {
	fprintf (stderr,
	  "%s: h option error -- height (%f) is outside range for standard 8.5 x 11 in. page.\n", 
	  progname, my_image.height);
	exit(1);
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
    pswd = getpwuid (getuid ());
    gethostname (hostname, sizeof hostname);
    fprintf(fo,"%% by %s:%s (%s)\n",hostname, pswd->pw_name, pswd->pw_gecos);
    fprintf(fo,"%% Information from XWD rasterfile header:\n");
    fprintf(fo,"%%   width =  %d, height = %d, depth = %d\n",win.pixmap_width, 
      win.pixmap_height, win.pixmap_depth);
    fprintf(fo,"%%   file_version = %d, pixmap_format = %d, byte_order = %d\n",
      win.file_version, win.pixmap_format, win.byte_order);
    fprintf(fo,"%%   bitmap_unit = %d, bitmap_bit_order = %d, bitmap_pad = %d\n", 
      win.bitmap_unit, win.bitmap_bit_order, win.bitmap_pad);
    fprintf(fo,"%%   bits_per_pixel = %d, bytes_per_line = %d, visual_class = %d\n",
      win.bits_per_pixel, win.bytes_per_line, win.visual_class);
    fprintf(fo,"%%   bits/rgb = %d, colormap entries = %d, ncolors = %d\n",
      win.bits_per_rgb, win.colormap_entries, win.ncolors);
    fprintf(fo,"%% Portion of raster image in this file:\n");
    fprintf(fo,"%%   starting line = %d\n",line_skip+1);
    fprintf(fo,"%%   ending line = %d\n",line_end);
    fprintf(fo,"%%   starting column = %d\n",col_start);
    fprintf(fo,"%%   ending column = %d\n",col_end);
    fprintf(fo,"gsave\n");

    fprintf(fo,"/inch {72 mul} def\n");

    if(flag.gamma == TRUE)
      fprintf(fo,"{ %f exp } settransfer\n", my_image.gamma);

  /* get a buffer to hold each line of the image */
    buffer = (unsigned char *) malloc( win.bytes_per_line );

    switch (win.pixmap_depth) {   /* what type of image do we have ? */

    case 24:   /* 24 bit image */
	if (flag.mono == FALSE) {   /* print as color image */
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
	skipLines(&win, &line[0], buffer, line_skip);

      /* start outputting the image */
	outputcount = 0;
	for (i = 0; i <my_image.ps_height; i++)  {
	    get_next_raster_line(&win, line, buffer);
	    rr = line[3*col_skip];
	    gg = line[3*col_skip];
	    bb = line[3*col_skip];
	    runlen = 0;
	    for(j = 3*col_start; j < 3*col_end; j += 3) {
		if (flag.black2white == TRUE &&
		  line[j] == 0 && line[j+1] == 0 && line[j+2] == 0 ) {
		    line[j]   = 255;
		    line[j+1] = 255;
		    line[j+2] = 255;
		}
	      /*
	       * If the colors are the same, just keep counting, but make sure that
	       * the run length will fit in a byte (256 max, i.e. 0 means length 1,
	       * 1 means length 2, etc.)
	       */
		if (line[j] == rr && line[j+1] == gg && line[j+2] == bb) {
		    if (runlen == 0xff) {
			if (flag.mono == FALSE) {
			    fprintf(fo,"%02x%02x%02x%02x", runlen, rr, gg, bb);
			    outputcount += 4;
			}
			else {
			    intens = 0.299*rr + 0.587*gg + 0.114*bb;
			    fprintf(fo,"%02x%02x", runlen, intens);
			    outputcount += 2;
			}
			runlen = -1;
		    }
		    runlen++;
		}
	      /* if the color changed, then output the last run and reset counter */
		else {
		    if (flag.mono == FALSE) {
			fprintf(fo,"%02x%02x%02x%02x", runlen, rr, gg, bb);
			outputcount += 4;
		    }
		    else {
			intens = 0.299*rr + 0.587*gg + 0.114*bb;
			fprintf(fo,"%02x%02x", runlen, intens);
			outputcount += 2;
		    }
		    runlen = 0;
		    rr = line[j];
		    gg = line[j+1];
		    bb = line[j+2];
		}
	      /* check to make sure the output lines are not too long */
		if (outputcount >= MAXPERLINE) {
		    outputcount = 0;
		    fprintf(fo,"\n");
		}
	    }
	    if (flag.mono == FALSE) {
		fprintf(fo,"%02x%02x%02x%02x", runlen, rr, gg, bb);
		outputcount += 4;
	    }
	    else {
		intens = 0.299*rr + 0.587*gg + 0.114*bb;
		fprintf(fo,"%02x%02x", runlen, intens);
		outputcount += 2;
	    }
	}
	fprintf(fo,"\n");
	break;

    case 8:  /* eight bit image */
	fprintf(fo,"/buffer 2 string def\n");
	fprintf(fo,"/rgbmap %d string def\n", 3*win.ncolors);
	fprintf(fo,"/rgb (000) def\n");
	fprintf(fo,"/pixels %d string def\n", 3*win.ncolors);
	outputColorImage(fo);
	fprintf(fo,"/drawcolorimage {\n");
	fprintf(fo,"  %d %d %d\n",my_image.pixels_width,my_image.ps_height, win.pixmap_depth);
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
	for(im = 0; im < win.ncolors; im++) {
	    if (flag.black2white == TRUE) {
		if (colors[im].red == 255 && colors[im].green == 255 && colors[im].blue == 255) {
		    colors[im].red = 0;
		    colors[im].green = 0;
		    colors[im].blue = 0;
		}
		else if (colors[im].red == 0 && colors[im].green == 0 && colors[im].blue == 0) {
		    colors[im].red = 255;
		    colors[im].green = 255;
		    colors[im].blue = 255;
		}
	    } 
	    fprintf(fo,"%02x",   colors[im].red);
	    fprintf(fo,"%02x",   colors[im].green);
	    fprintf(fo,"%02x\n", colors[im].blue);
	}
	fprintf(fo,"\n");
	fprintf(fo,"\ndrawcolorimage\n");
  
	skipLines(&win, &line[0], buffer, line_skip);    
      /* run thru each row of pixels and run-len length encode it */
	outputcount = 0;
	for (i = 0; i <my_image.ps_height; i++)  {
	    get_next_raster_line(&win, line, buffer);
	    runlen = 0;
	    color = line[col_skip];
	    for(j = col_start; j < col_end; j++) {
	      /*
		If the colors are the same, just keep counting, but make sure that
		the run length will fit in a byte (256 max, i.e. 0 means length 1,
		1 means length 2, etc.)
		*/
		if (line[j] == color) {
		    if (runlen == 0xff) {
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
		if (outputcount > MAXPERLINE) {
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
	    fprintf(fo,"/rgbmap %d string def\n", 3*win.ncolors);
	    fprintf(fo,"/rgb (000) def\n");
	    outputColorImage(fo);
	    fprintf(fo,"/drawcolorimage {\n");
	    fprintf(fo,"  %d %d %d\n",my_image.pixels_width,my_image.ps_height, win.pixmap_depth*2);
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
	    for(im = 0; im < win.ncolors; im++) {
		fprintf(fo,"%02x",   colors[im].red);
		fprintf(fo,"%02x",   colors[im].green);
		fprintf(fo,"%02x\n", colors[im].blue);
	    }
	    fprintf(fo,"\n");
	    fprintf(fo,"\ndrawcolorimage\n");
	}

	skipLines(&win, &line[0], buffer, line_skip);    
      /* run thru each row of pixels and run-len length encode it */
	outputcount = 0;

	for (i = 0; i <my_image.ps_height; i++)  {
	    get_next_raster_line(&win, line, buffer);
	  /* unpack the 4 bit images into a byte */
	    if (*(char *) &swaptest)
	      for (j = 0; j < win.bytes_per_line; j++) {
		  line4bits[2*j]   = line[j] & 0x0f;
		  line4bits[2*j+1] = line[j] >> 4;
	      }
	    else
	      for (j = 0; j < win.bytes_per_line; j++) {
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
		if (line4bits[j] == color) {
		    if (runlen == 0xff) {
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
		if (outputcount > MAXPERLINE) {
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
	fprintf(fo,"  %d %d %2d\n", my_image.pixels_width, my_image.ps_height, win.pixmap_depth);
	fprintf(fo,"%s\n",s_matrix);
	fprintf(fo,"  { currentfile buffer readhexstring pop pop buffer }\n");
	fprintf(fo,"  image\n");
	fprintf(fo,"} bind def\n");
	fprintf(fo,"\n");
	fprintf(fo,"%s\nmatrix currentmatrix\n%s\n", s_translate, s_scale);    
	fprintf(fo,"\ndrawbinaryimage\n");

	skipLines(&win, &line[0], buffer, line_skip);    
      /* run through each row of pixels (8 per byte) and copy it  */
	outputcount = 0;
	for (i = 0; i < my_image.ps_height; i++)  {
	    get_next_raster_line(&win, line, buffer);
	    for(j=col_skip; j < col_end; j++) {
		fprintf(fo,"%02x", 255 - line[j]);       /* invert data for PostScript */
		outputcount++;
	      /* check to make sure the output lines are not too long	 */
		if (outputcount > MAXPERLINE) {
		    outputcount = 0;
		    fprintf(fo,"\n");
		}
	    }
	    outputcount = 0;
	    fprintf(fo,"\n");
	}
	break;

    default:
	fprintf (stderr,"%s: sorry! cannot handle input file of this format.\n", progname);
	fprintf (stderr,"    (See program author.) \n");
	exit(3);
    }
  
    switch (win.bits_per_pixel) {  /* TEMP I don't know why this needs to be done - CAM */
    case 8:
	fprintf(fo,"\npop pop setmatrix\n");
	break;
    case 24:
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
    if(flag.inc_file == TRUE) {
      /* Silicon Graphics did not recognize EOF so cludge this way */
#ifdef mips 
	while ((c = fgetc(incfile)) != ((char) -1))
#else
	  while ((c = fgetc(incfile)) != EOF)
#endif
	    putchar(c);
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
} 
/* End of Main Program */


/*
 ** getDumpType() - returns an int describing the type of dump we have
 */

int
getDumpType(header)
    XWDFileHeader *header;
{
    switch(header->bits_per_pixel) {
    case 32:
    case 24:
    case 16:
    case 12:
    case 8:
    case 4:
	return(1); /* TEMP */
	break;

    case 2:
    case 1:
    default:
	fprintf (stderr, "can't handle %u bits_per_pixel\n", header->bits_per_pixel);
	exit(1);
	break;
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

get_page_info(the_page)
    Page *the_page;
{
  /* letter paper size 8.5 x 11 */
    if ( (strcmp(the_page->type, "A") == 0) || (strcmp(the_page->type, "letter") == 0) ) {
	the_page->xcenter = 8.5/2;
	the_page->ycenter = 11.0/2 + .21;
	the_page->maxwsize = 8.11;
	the_page->maxhsize = 8.91;
	the_page->default_width = 7.0;
	the_page->default_height = 8.5;
	return(0);
    }
    else if (strcmp(the_page->type, "B") == 0) {         /* 11 x 17 inch paper size */
	the_page->xcenter = 11.0/2;
	the_page->ycenter = 17.0/2 + .21;
	the_page->maxwsize = 10.60;
	the_page->maxhsize = 14.91;
	the_page->default_width = 9.0;
	the_page->default_height = 14.5;
	return(0);
    }
    else if (strcmp(the_page->type, "A4") == 0 ) {       /* A4 paper size 8.3 x 11.7 */
	the_page->xcenter = 8.3/2;
	the_page->ycenter = 11.7/2 + .21;
	the_page->maxwsize = 8.26;
	the_page->maxhsize = 9.60;
	the_page->default_width = 7.25;
	the_page->default_height = 9.0;
	return(0);
    }
    else if (strcmp(the_page->type, "A3") == 0 ) {       /* A3 paper size 11.7 x 16.6 */
	the_page->xcenter = 11.7/2;
	the_page->ycenter = 16.6/2 + .21;
	the_page->maxwsize = 11.69;
	the_page->maxhsize = 16.53;
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
get_raster_header(win, w_name)
    XWDFileHeader *win;
    char *w_name;
{
    unsigned long swaptest = 1;
    int i, zflg, idifsize;

  /* read in window header */
    fullread(0, (char *)win, sizeof( *win ));

    if (*(char *) &swaptest)  /* am I running on a byte swapped machine? */
      xwd2ps_swaplong((char *)win, (long)sizeof(*win)); /* swap all the bytes
							   in the header */
    if (win->file_version != XWD_FILE_VERSION) {
	fprintf (stderr,"%s: file format version missmatch.\n", progname);
	exit(1);
    }

    if (win->header_size < sizeof(*win)) {
	fprintf (stderr,"%s: header size is too small.\n", progname);
	exit(1);
    }

    if (win->pixmap_depth != 1 && win->pixmap_format != ZPixmap) {
	fprintf (stderr,"%s: image is not in Z format\n", progname);
	exit(1);
    }

  /* set z pixmap flag if required for operations */

    if( win->pixmap_depth != 1 && win->pixmap_format == ZPixmap)
      zflg= win->bits_per_pixel / 8;
    else
      zflg=0; /* TEMP - as far as I can tell, zflg is never used ! */
  
    if (win->byte_order != win->bitmap_bit_order) {
	fprintf (stderr, "%s: image will be incorrect,", progname);
	fprintf (stderr, " byte swapping required but not performed.\n");
    }

  /* TEMP - put in large image warning here! */
    if(idifsize = (unsigned)(win->header_size - sizeof *win)) {
	w_name = (char *)malloc(idifsize);
	fullread(0, w_name, idifsize);
    }
    if(win->ncolors) {
	if( (colors = (XColor *)malloc((unsigned) (win->ncolors * sizeof(XColor)))) == NULL) {
	    fprintf (stderr, "%s: can't get memory for color map\n", progname);
	    exit(1);
	}
	fullread(0, colors, win->ncolors * sizeof(XColor));
      /*
       * Scale the values received from the colormap to 8 bits
       */

	if ((!*(char *) &swaptest) || (win->byte_order))
	  for(i = 0; i < win->ncolors; i++) {
	      colors[i].red   = colors[i].red   >> 8;
	      colors[i].green = colors[i].green >> 8;
	      colors[i].blue  = colors[i].blue  >> 8;
	  }
    }
}

/* 
 ** get_next_raster_line() -  returns the next raster line in the image.
 * written 2-12-89 by R.C.Tatar
 */
get_next_raster_line(win, linec, buffer)
    XWDFileHeader *win;
    unsigned char *linec, *buffer;
{
    unsigned char *bufptr, *bufptr1;
    int iwbytes =  win->bytes_per_line;
    int iwbits  =  win->bits_per_pixel;
    int j;

    switch(iwbits) {
    case 32:
	fullread(0, buffer, iwbytes);
      /* For this case, copy byte triplets into line */
	bufptr1 = linec;
	bufptr = buffer;
	for( j = 0; j < iwbytes/4; j++) {
	    bufptr++;                      /* skip unused byte */
	    *bufptr1++ = *bufptr++;
	    *bufptr1++ = *bufptr++;
	    *bufptr1++ = *bufptr++;
	}
	return;
	break;

    case 8:
	fullread(0, linec, iwbytes);
	return;
	break;

    case 4:
	fullread(0, linec, iwbytes);
	return;
	break;

    default:
      /* error message if no case selected  */
	fprintf (stderr, 
	  "%s: Do not know how to handle %u bits per pixel.\n", 
	  progname, iwbits);
	exit(1);
    }
}

/* 
 ** skipLines() - skip over unprinted parts of the image 
 */
skipLines(winP, line, buffer, line_skip)
    XWDFileHeader *winP;
    char line[], buffer[];
    int line_skip;
{
    int i;

    for (i = 0; i < line_skip; i++)
      get_next_raster_line(winP, line, buffer);
}

/*
 *** parseArgs() - figure out which command line options were set
 */
parseArgs(argc, argv, option, image, page)
    int argc;
    char **argv;
    Options *option;
    Image *image;
    Page *page;
{
    char c;
    extern char *optarg;

    while ((c = getopt(argc, argv, "tdlLPc:s:f:h:w:H:W:mS:p:bg:I")) != EOF)
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
	  sprintf (optarg, "%d", option->ncopies);
	  break;
      case 's':
	  flag.title = TRUE;
	  option->title.string = optarg;
	  break;
      case 'p':
	  strcpy(page->type, optarg);
	  break;
      case 'f':
	  option->inc_file.pointer = fopen (optarg, "r");
	  if (option->inc_file.pointer == NULL) {
	      fprintf (stderr, 
		"%s: f option error -- cannot open %s for include file.\n",
		progname, optarg);
	      flag.error++;
	  }
	  flag.inc_file = TRUE;
	  break;
      case 'g':
	  flag.gamma = TRUE;
	  if( sscanf(optarg, "%f", &(image->gamma)) == 0 ) {
	      fprintf (stderr, "%s: g option argument error\n", progname);
	      flag.error++;
	  }
	  if ( image->gamma < 0.0 || image->gamma > 1.0 ) {
	      fprintf (stderr, "%s: gamma value out of range: %d\n", progname, image->gamma);
	      flag.error++;
	  }
	  break;
      case 'h':
	  flag.h = TRUE;
	  if( sscanf(optarg, "%f", &(image->height)) == 0 ) {
	      fprintf (stderr, "%s: h option argument error\n", progname);
	      flag.error++;
	  }
	  break;
      case 'w':
	  flag.w = TRUE;
	  if( sscanf(optarg, "%f", &(image->width)) == 0 ) {
	      fprintf (stderr, "%s: w option argument error\n", progname);
	      flag.error++;
	  }
	  break;
      case 'H':
	  if( sscanf(optarg, "%f", &(image->height_frac)) == 0) {
	      fprintf (stderr, "%s: H option argument error\n", progname);
	      flag.error++;
	  }
	  break;
      case 'W':
	  if( sscanf(optarg, "%f", &(image->width_frac)) == 0 ) {
	      fprintf (stderr, "%s: W option argument error\n", progname);
	      flag.error++;
	  }
	  break;
      case 'S':
	  if( sscanf(optarg, "%d", &(option->title.font_size)) == 0 ) {
	      fprintf (stderr, "%s: S option argument error\n", progname);
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
getOrientation(pg, im)
    Page pg;
    Image im;
{
    float k1, k2, k;

    k1 = (pg.default_height - pg.height_adjust)/im.ps_height;
    k  = pg.default_width/im.pixels_width;
    k1 = fmax(k1, k);
  
    k2 = (pg.default_width - pg.height_adjust)/im.ps_height;
    k  = pg.default_height/im.pixels_width;
    k2 = fmax(k2, k);

    if (k1 >= k2)
      return(PORTRAIT);
    else
      return(LANDSCAPE);
}
