/* siteSpecific.h

   The parameters in this header file may be changed to customize them
   for your own purposes.  You do so at your own risk.  MEDM has been
   widely used and tested with the standard values.  Your changes are
   not guaranteed to work and should be tested.  They may also not
   work in the future because of changes to MEDM.

   On the other hand, the standard values, which were chosen a long
   time ago, may not be the best choices today, so feel free to
   experiment.
   
*/

/*** Font specifications ***/

#define MAX_FONTS		16		/* max # of fonts             */
#define LAST_CHANCE_FONT	"fixed"		/* a common font              */
#define ALIAS_FONT_PREFIX	"widgetDM_"	/* append pixel sz. for alias */
/* The font aliases are the concatenation of the ALIAS_FONT_PREFIX and
   the sizes in fontSizeTable, e.g. widgetDM_4, widgetDM_6, etc.  The
   actual fonts used depend on what is defined in the X font database
   for these aliases. */

#define FONT_ALIASES_STRING	"alias"
#define DEFAULT_SCALABLE_STRING	"scalable"

/* Choose the default scalable font here */
/* Speedo scalable, available from X11R5 (and higher) font server */
#define DEFAULT_SCALABLE_DISPLAY_FONT \
"-bitstream-charter-bold-r-normal--0-0-0-0-p-0-iso8859-1"

/* Pick one of the following depending on whether you want fixed or
   scalable (recommended) fonts as the default. */
#if 1
/* Fixed font default */
# define MEDM_DEFAULT_DISPLAY_FONT FONT_ALIASES_STRING
# define MEDM_DEFAULT_FONT_STYLE FIXED_FONT
#else
/* Scalable font default */
# define MEDM_DEFAULT_DISPLAY_FONT DEFAULT_SCALABLE_STRING
# define MEDM_DEFAULT_FONT_STYLE SCALABLE_FONT
#endif

#ifndef ALLOCATE_STORAGE
extern int fontSizeTable[MAX_FONTS];
#else
int fontSizeTable[MAX_FONTS] = {4,6,8,10,12,14,16,18,20,
				22,24,30,36,40,48,60,};
#endif

/*** Bar flashing  ***/
/* Pick double buffering only if you get flashing with the Bar */
#if 0
#define BAR_DOUBLE_BUFFER
#endif

/*** Help path ***/

/* MEDM help uses Netscape to display MEDM.html.  If you want to use a
   local URL for MEDM help, define it here.  The default points to the
   APS web site.  The version there is updated regularly to reflect
   the latest changes in MEDM.  Some of these changes may not be in
   this version.  The MEDM.html that goes with this version is in the
   help subdirectory to this directory. */

#define MEDM_HELP_PATH \
"http://www.aps.anl.gov/asd/controls/epics/EpicsDocumentation/ExtensionsManuals/MEDM/MEDM.html"

/*** Print setup defaults ***/

/* See medm/printUtils/utilPrint.h for definitions of macros */

/* The following are 1 for true and 0 for false */
#define DEFAULT_PRINT_TOFILE   0
#define DEFAULT_PRINT_TITLE    1
#define DEFAULT_PRINT_TIME     1
#define DEFAULT_PRINT_DATE     1
/* Alternatives are PRINT_PORTRAIT or PRINT_LANDSCAPE */
#define DEFAULT_PRINT_ORIENTATION PRINT_PORTRAIT
/* Alternatives are PRINT_A, PRINT_B, PRINT_A3, and PRINT_A4 */
#define DEFAULT_PRINT_SIZE     PRINT_A
#define DEFAULT_PRINT_FILENAME "medmScreen.ps"
#define DEFAULT_PRINT_CMD      "lpr -P$PSPRINTER"
#if 0
/* Command used internally before MEDM 2.3.6 */
#define DEFAULT_PRINT_CMD     "lp -c -d$PSPRINTER"
#endif

/*** Colormap specifications ***/

/* The RGB values in the default display colormap are changeable.  The
  inten field is for backward compatibility and is not used in the new
  ADL file format. Changing the DEFAULT_DL_COLORMAP_SIZE may cause
  problems. */

/* Default colormap for "New...". The default colormap size must be
  less than DL_MAX_COLORS. */
#define DEFAULT_DL_COLORMAP_SIZE	65
#ifndef ALLOCATE_STORAGE
extern DlColormap defaultDlColormap;
#else
DlColormap defaultDlColormap = {
  /* ncolors */
    65,
  /* r,  g,   b,   inten */
    {{ 255, 255, 255, 255, },
     { 236, 236, 236, 0, },
     { 218, 218, 218, 0, },
     { 200, 200, 200, 0, },
     { 187, 187, 187, 0, },
     { 174, 174, 174, 0, },
     { 158, 158, 158, 0, },
     { 145, 145, 145, 0, },
     { 133, 133, 133, 0, },
     { 120, 120, 120, 0, },
     { 105, 105, 105, 0, },
     { 90, 90, 90, 0, },
     { 70, 70, 70, 0, },
     { 45, 45, 45, 0, },
     { 0, 0, 0, 0, },
     { 0, 216, 0, 0, },
     { 30, 187, 0, 0, },
     { 51, 153, 0, 0, },
     { 45, 127, 0, 0, },
     { 33, 108, 0, 0, },
     { 253, 0, 0, 0, },
     { 222, 19, 9, 0, },
     { 190, 25, 11, 0, },
     { 160, 18, 7, 0, },
     { 130, 4, 0, 0, },
     { 88, 147, 255, 0, },
     { 89, 126, 225, 0, },
     { 75, 110, 199, 0, },
     { 58, 94, 171, 0, },
     { 39, 84, 141, 0, },
     { 251, 243, 74, 0, },
     { 249, 218, 60, 0, },
     { 238, 182, 43, 0, },
     { 225, 144, 21, 0, },
     { 205, 97, 0, 0, },
     { 255, 176, 255, 0, },
     { 214, 127, 226, 0, },
     { 174, 78, 188, 0, },
     { 139, 26, 150, 0, },
     { 97, 10, 117, 0, },
     { 164, 170, 255, 0, },
     { 135, 147, 226, 0, },
     { 106, 115, 193, 0, },
     { 77, 82, 164, 0, },
     { 52, 51, 134, 0, },
     { 199, 187, 109, 0, },
     { 183, 157, 92, 0, },
     { 164, 126, 60, 0, },
     { 125, 86, 39, 0, },
     { 88, 52, 15, 0, },
     { 153, 255, 255, 0, },
     { 115, 223, 255, 0, },
     { 78, 165, 249, 0, },
     { 42, 99, 228, 0, },
     { 10, 0, 184, 0, },
     { 235, 241, 181, 0, },
     { 212, 219, 157, 0, },
     { 187, 193, 135, 0, },
     { 166, 164, 98, 0, },
     { 139, 130, 57, 0, },
     { 115, 255, 107, 0, },
     { 82, 218, 59, 0, },
     { 60, 180, 32, 0, },
     { 40, 147, 21, 0, },
     { 26, 115, 9, 0, },
    }
};
#endif
