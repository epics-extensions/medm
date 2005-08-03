/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/* xwd2ps.h - defines for xwd2ps */

/*
 * define the various types of dumps we might print
 */

#define DEBUG_STDC 0

#if DEBUG_STDC
#ifndef __STDC__
#error __STDC__ is undefined
#else
#if __STDC__ == 0
#error __STDC__=0
#elif __STDC__ == 1
#error __STDC__=1
#else
#error __STDC__ is defined and not 0 or 1
#endif
#endif
#endif

#define UNKNOWN   0
#define COLOR24   1
#define PSEUDO12  2
#define PSEUDO8   3
#define PSEUD04   4
#define GRAY24    5
#define GRAY12    6
#define GRAY8     7
#define GRAY4     8
#define GRAY2     9
#define GRAY1    10

#define INTENSITY(color) (39L*color.red + 50L*color.green + 11L*color.blue)
#define FALSE     0
#define TRUE      1
#define PORTRAIT  0
#define LANDSCAPE 1
#define LOGOHEIGHT 0.5

#define MAPMAXPERLINE 35    /* PostScript colormap data line length-1 */
#define MAXPERLINE    35    /* PostScript image data line length-1 */

#include <X11/Xlib.h>

typedef struct _Image {
    float height;
    float width;
    float gamma;             /* gamma correction factor for brightening images */
    float width_frac;        /* fraction of horizontal raster to use for image */
    float height_frac;       /* fraction of vertical raster to use for image */
    int   orientation;
    int   pixels_width;
    int   ps_width;
    int   ps_height;
} Image;

typedef struct _Page {
    char  type[20];        /* page size, e.g. A or letter, B, A4, A3 */
    float ximagepos;
    float yimagepos;
    float maxhsize;
    float maxwsize;
    float xcenter;
    float ycenter;
    float default_height;
    float default_width;
    float yoffset;
    float height_adjust;
    float time_date_height;
} Page;

typedef struct _Flag {
    int logo;
    int black2white; /* map black values to white */
    int date;
    int title;
    int time;
    int w;
    int h;
    int gamma;      /* gamma correction flag */
    int mono;       /* monochrome flag: TRUE = create monochrome PS */
    int inc_file;    /* include file flag: TRUE = append a file */
    int landscape;
    int portrait;
    int border;
    int invert;
    int error;            /* option error flag */
} Flag;

typedef struct _Title {
    char  *string;
    int   font_size;
    float height;
} Title;

typedef struct _File {
    FILE *pointer;
    char *name;
} File;

typedef struct _Options {
    Title title;
    int   ncopies;
    File  input_file;
    File  inc_file;
} Options;
