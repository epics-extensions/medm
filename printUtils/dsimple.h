/* $XConsortium: dsimple.h,v 1.2 88/09/06 17:12:31 jim Exp $ */
/*
 * Just_display.h: This file contains the definitions needed to use the
 *                 functions in just_display.c.  It also declares the global
 *                 variables dpy, screen, and program_name which are needed to
 *                 use just_display.c.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 *
 * Send bugs, etc. to chariot@athena.mit.edu.
 */

/* This stuff is defined in the calling program by just_display.h */
extern char *program_name;
extern Display *dpy;
extern int screen;

/* Declarations for functions in just_display.c */

#ifdef _NO_PROTO

void Fatal_Error();
#ifndef VMS
char *Malloc();
char *Realloc();
#endif
char *Get_Display_Name();
Display *Open_Display();
void Setup_Display_And_Screen();
XFontStruct *Open_Font();
void xwdBeep();
Pixmap ReadBitmapFile();
void WriteBitmapFile();
Window Select_Window_Args();

#else

void Fatal_Error(char *, char *);
char *Malloc(unsigned);
char *Realloc(char *, int);
char *Get_Display_Name(int *, char **);
Display *Open_Display(char *);
void Setup_Display_And_Screen(int *, char **);
XFontStruct *Open_Font(char *);
void xwdBeep();
Pixmap ReadBitmapFile(Drawable, char *, int *, int*, int*, int*);
void WriteBitmapFile(char *, Pixmap, int, int, int, int);
Window Select_Window_Args(int *, char **);

#endif

#define X_USAGE "[host:display]"              /* X arguments handled by
						 Get_Display_Name */
#define SELECT_USAGE "[{-root|-id <id>|-font <font>|-name <name>}]"

/*
 * Other_stuff.h: Definitions of routines in other_stuff.
 *
 * Written by Mark Lillibridge.   Last updated 7/1/87
 *
 * Send bugs, etc. to chariot@athena.mit.edu.
 */

#ifdef _NO_PROTO
unsigned long Resolve_Color();
Pixmap Bitmap_To_Pixmap();
Window Select_Window();
void out();
void blip();
Window Window_With_Name();
void outl();
#else
unsigned long Resolve_Color(Window, char*);
Pixmap Bitmap_To_Pixmap(Display *, Drawable, GC, Pixmap, int, int);
Window Select_Window(Display *);
void out();
void blip();
Window Window_With_Name(Display *, Window, char *);
int outl(const char *fmt, ...);
#endif

