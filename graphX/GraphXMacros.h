/**
 ** Macro Include for graphX library
 **
 **
 **   MDA - 13 December 1990
 **/

#ifndef __GRAPHXMACROS_H___
#define __GRAPHXMACROS_H___



/*
 * some utility macros
 */
#ifndef min
#  define min(a,b) ((a)<(b)?(a):(b))
#endif
#ifndef max
#  define max(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef BOOLEAN
#  define BOOLEAN int
#endif
#ifndef TRUE
#  define TRUE 1
#endif
#ifndef FALSE
#  define FALSE 0
#endif


/* 
 * some "heuristic" calculations of good font sizes, for "average"
 *   text fields in plots
 */

#define TITLE_SCALE_FACTOR	20
#define AXES_SCALE_FACTOR	27

#define GraphX_TitleFontSize(a)  min(a->w,a->h)/TITLE_SCALE_FACTOR
#define GraphX_AxesFontSize(a)  min(a->w,a->h)/AXES_SCALE_FACTOR


#define CROSSHAIRCOLOR "red"


#endif  /* __GRAPHXMACROS_H___ */
