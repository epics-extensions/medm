/*
 *** xwd2ps.h - defines for xwd2ps
 * $Header$
 */

/*
 * define the various types of dumps we might print
 */

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

typedef struct Image {
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

typedef struct Page {
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

typedef struct Flag {
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

typedef struct Title {
    char  *string;
    int   font_size;
    float height;
} Title;

typedef struct File {
    FILE *pointer;
    char *name;
} File;

typedef struct Options {
    Title title;
    int   ncopies;
    File  input_file;
    File  inc_file;
} Options;
