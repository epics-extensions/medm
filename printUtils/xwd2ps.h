/* xwd2ps.h - defines for xwd2ps */

/* KE: The function prototypes have been added to this file for all
   the programs in the directory. */

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

/* KE: Function prototypes */

/* xwd2ps.h */

int xwd2ps(int argc, char **argv, FILE *fo);

/* xwd.c */
void usage();
void xwd(Display *display, Window window, char *file);

/* pUtils */
void get_time_and_date(char mytime[], char mydate[]);
void fullread(int file, char *data, int nbytes);
void xwd2ps_swapshort(register char *bp, register long n);
void xwd2ps_swaplong(register char *bp, register long n);
void xwd2ps_usage(void);
float fmax(float a, float b);

/* ps_utils.c */
void outputBorder(FILE *fo, Image the_image);
void outputDate(FILE *fo, Image the_image);
void outputTitle(FILE *fo, Image the_image, Options the_options);
void outputTime(FILE *fo, Image the_image);
void outputColorImage(FILE *fo);
void outputLogo(FILE *fo, Image the_image);
void printPS(FILE *fo, char **p);
void printEPSF(FILE *fo, Image image, Page  page, char  *file_name);





