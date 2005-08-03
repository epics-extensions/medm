/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
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
   scaleable (recommended) fonts as the default. */
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

/*** WheelSwitch format  ***/
/* Set the WheelSwitch default format here.  It is used in EDIT mode
 * only. */
#define WHEEL_SWITCH_DEFAULT_FORMAT "% 6.2f"

/*** Help path ***/

/* MEDM help uses Netscape to display MEDM.html.  If you want to use a
   local URL for MEDM help, define it here.  The default points to the
   APS web site.  The version there is updated regularly to reflect
   the latest changes in MEDM.  Some of these changes may not be in
   this version.  The MEDM.html that goes with this version is in the
   help subdirectory to this directory. This value will be overwritten
   if MEDM_HELP_PATH is defined in the environment. This is another
   way to customize the location.  */

#define MEDM_HELP_PATH \
"http://www.aps.anl.gov/asd/controls/epics/EpicsDocumentation/ExtensionsManuals/MEDM/MEDM.html"

/*** Print setup defaults ***/

/* See medm/printUtils/utilPrint.h for definitions of macros */

/* The following are 1 for true and 0 for false */
#define DEFAULT_PRINT_TOFILE 0
#define DEFAULT_PRINT_TIME 1
#define DEFAULT_PRINT_DATE 1
#define DEFAULT_PRINT_REMOVE_TEMP_FILES 1
/* Alternatives are PRINT_PORTRAIT or PRINT_LANDSCAPE */
#define DEFAULT_PRINT_ORIENTATION PRINT_PORTRAIT
/* Alternatives are PRINT_A, PRINT_B, PRINT_A3, and PRINT_A4 */
#define DEFAULT_PRINT_SIZE PRINT_A
/* Alternatives are PRINT_TITLE_NONE, PRINT_TITLE_SHORT_NAME,
 * PRINT_TITLE_LONG_NAME, PRINT_TITLE_SPECIFIED */
#define DEFAULT_PRINT_TITLE PRINT_TITLE_SHORT_NAME
#define DEFAULT_PRINT_TITLE_STRING  ""
#define DEFAULT_PRINT_FILENAME "medmScreen.ps"

/* MEDM screen dumps first create an XWD file with the name
 * PRINT_XWD_FILE and then convert it to a Postscript file with the
 * same name with a ps suffix.  DEFAULT_PRINT_REMOVE_TEMP_FILES
 * determines how these files are disposed of:
 *   0 Don't remove either temporary file
 *   1 Remove PRINT_XWD_FILE file from XWD
 *   2 Remove PRINT_XWD_FILE and the Postscript file
 * Keep in mind the user can change the print command. */
#if defined(WIN32)
# define DEFAULT_PRINT_CMD "gsview32.exe"
/* We don't know where the temp directory is on WIN32.  For WIN32
   %TEMP%/ will be prepended later if it exists. For the other
   platforms, the name is used as entered here. */
# define PRINT_XWD_FILE "medm.xwd"
/* Even with PRINT_REMOVE_TEMP_FILES=2, WIN32 usually prevents the
   Postscript file from being deleted provided Ghostview starts fast
   enough.  You can set it to 1 to be sure. */
# define PRINT_REMOVE_TEMP_FILES 1
#elif defined(VMS)
# define DEFAULT_PRINT_CMD "print /queue=%s/delete"
# define PRINT_XWD_FILE "sys$scratch:medm.xwd"
# define PRINT_REMOVE_TEMP_FILES 2
#else
# define DEFAULT_PRINT_CMD "lpr -P$PSPRINTER"
# define PRINT_XWD_FILE "/tmp/medm.xwd"
# define PRINT_REMOVE_TEMP_FILES 2
# if 0
/* Use for testing to save paper and time. Doesn't require
   PRINT_REMOVE_TEMP_FILES=2 */
#  define DEFAULT_PRINT_CMD "ghostview"
/* Command used internally before MEDM 2.3.6 */
#  define DEFAULT_PRINT_CMD "lp -c -d$PSPRINTER"
# endif
#endif

/*** Related display options ***/

/* If POPUP_EXISTING_DISPLAY is True and a display is invoked through
   the Related Display, then MEDM will pop up an existing display if
   it has the same name and macro arguments.  Otherwise, MEDM will
   open a new copy. */
#define POPUP_EXISTING_DISPLAY True

/* Define this if you want the displays to raise on a button event.
   This was the old behavior, but is not suggested, since X controls
   this via user preference. */
#if 0
#define MEDM_AUTO_RAISE 1
#endif

/* Define these to set the number of menu items in the Related Display
 * and Shell Command. */
#define MAX_RELATED_DISPLAYS    16
#define MAX_SHELL_COMMANDS      16

#if 0
/* XR5 has a bug in that resource ID's are not reused, even if the
   resource is freed.  Eventually it runs out of ID's and MEDM doesn't
   work right.  Define the following to locally handle the resource
   allocation for pixmaps and graphics contexts.  These are the most
   created/freed resources */
/* !!! Do not use this yet.  It doesn't work. */
#define USE_XR5_RESOURCEID_PATCH
#endif

/* Define this if you want to use the Btn2 Drag & Drop feature.
Normally, you do want to use this feature.  This switch was put here
to turn it off temporarily when an intermittent bug in Xsun for
Solaris 8 caused it to not work and in addition when Btn2 was clicked
on a slider, to move the slider instead. */

#define USE_DRAGDROP 1

/* Define this to be 1 if you do not want resize handles on many of
the toplevel shells.  */

#define OMIT_RESIZE_HANDLES 0

  /* Define this to be 1 if you want to keep track of Image colors and
be sure they are allocated and freed as needed or not needed.  This
will allow other programs to use the colors when MEDM does not need
them.  It will only be useful if Images are both created and destroyed
in an MEDM session.  It will cause editing to be slow when there are
many Images, since the color allocation routines (XAllocColor and
XFreeColors) are slow and are called repeatedly for Undo.  It should
be unnecessary for static visuals (StaticGray, StaticColor, and
TrueColor) since the cells are Read-only and can be always used by
other programs anyway.  It is not recommended.  The colors will be
freed when MEDM exits in any event.  */

#define MANAGE_IMAGE_COLOR_ALLOCATION 0

/* Define this to be 1 if you want X errors to go to the MEDM Message
Window.  It is recommended.  X errors should be uncommon.  If they do
occur, however, MEDM may generate further errors trying to print to
the Message Window.  The errors should be always be printed to the
console.  If the console is always readily available, you may want to
set this to 0 to avoid these problems. If you want a visual
notification that seems to work in most cases, you can set it to 1.
In the event you are getting X errors, you can alternatively modify
the xErrorHandler routine in help.c to help debug the situation.  */

#define POST_X_ERRORS_TO_MESSAGE_WINDOW 1

/* Define this to be 1 to explicitly set the colors and shadows for
menus and all of their children in order to override any CDE changes.
It should not be necessary.  The problem has been fixed by specifying
*useColorObj in the fallback resources, but this switch is being kept
in case that fix doesn't work. */

#define EXPLICITLY_OVERWRITE_CDE_COLORS 0

/* Define this to include RTYP (DBR_CLASSNAME) in PvInfo.  For IOCs
with later versions of EPICS base, this is a good idea.  However, if
RTYP is not defined for the PV, it will take PvInfo a long time to
determine that and be sure it is not defined.  In that case, defining
DO_RTYPE to be zero will eliminate the delay before the PvInfo popup
appears. */
#define DO_RTYP 1

/*** Colormap specifications ***/
#include "displayList.h"

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
