/*******************************************************************
 FILE:		ControlP.h
 CONTENTS:	Private definitions for the Control widget class.
 AUTHOR:	Paul D. Johnston
 HISTORY:
 Date		Action
 ---------	------------------------------------
 3/11/92	Created.

********************************************************************/

#ifndef __XC_CONTROLP_H
#define __XC_CONTROLP_H

/*
 * Include the superclass private header.
 */
#include <X11/CoreP.h>

/*
 * Include the widget set header. 
 */
#include "Xc.h"

/*
 * Include this widget class public header.
 */
#include "Control.h"

/* Private declarations and definitions. */
#define MAX_LABEL		20


/*
 * Class part.
 */
typedef struct 
{
   int dummy;	/* Minimum of one member required. */
} ControlClassPart;

/*
 * Class record.
 */
typedef struct _ControlClassRec
{
   CoreClassPart core_class;
   ControlClassPart control_class;
} ControlClassRec;

/*
 * Declare the widget class record as external for use in the widget source
 * file.
 */
extern ControlClassRec controlClassRec;



/*
 * Instance part.
 */
typedef struct
{
   /* Public instance variables. */
   Pixel background_pixel;		/* Widget's background color (used
					 * in producing 3D effect).
					 */
   Pixel label_pixel;			/* Widget's Label color. */
   char *label;				/* Widget's Label string. */
   XFontStruct *font;			/* Font for Label string. */
   int shade_depth;			/* Depth of the 3D effect in pixels. */

   /* Private instance variables. */
   Pixmap shade;			/* Stipple bitmap for generating 
					 * 3D shading effect.
					 */
   XColor shade1, shade2;		/* Colors derived from background
					 * used with the -DNICE_SHADES compiler
					 * option for 3D shading of widget.
					 */
   GC gc;				/* GC used for drawing in this 
					 * widget.
					 */

} ControlPart;

/*
 * Instance record.
 */
typedef struct _ControlRec
{
   CorePart core;
   ControlPart control;
} ControlRec;



#endif  /* __XC_CONTROLP_H */

