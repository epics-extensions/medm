/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*****************************************************************************
 *
 *     Original Author : Mark Anderson
 *     Second Author   : Frederick Vong
 *     Third Author    : Kenneth Evans, Jr.
 *
 *****************************************************************************
*/

#ifndef __XTPARAMS_H__
#define __XTPARAMS_H__

#include <sys/types.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/cursorfont.h>
#include <X11/Xatom.h>

#include <Xm/Xm.h>
#include <Xm/Protocols.h>
#include <Xm/AtomMgr.h>
#include <Xm/DragDrop.h>

#include <Xm/DrawingA.h>
#include <Xm/BulletinB.h>
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>
#include <Xm/PushBG.h>
#include <Xm/ToggleB.h>
#include <Xm/ToggleBG.h>
#include <Xm/Scale.h>
#include <Xm/ScrollBar.h>
#include <Xm/DrawnB.h>
#include <Xm/TextF.h>
#include <Xm/Text.h>
#include <Xm/Label.h>
#include <Xm/LabelG.h>
#include <Xm/FileSB.h>
#include <Xm/DialogS.h>
#include <Xm/Form.h>
#include <Xm/Frame.h>
#include <Xm/MessageB.h>
#include <Xm/MainW.h>
#include <Xm/ScrolledW.h>
#include <Xm/List.h>
#include <Xm/ArrowB.h>
#include <Xm/PanedW.h>
#include <Xm/Separator.h>
#include <Xm/SeparatoG.h>
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/SelectioB.h>

#ifdef linux
#include <Xm/VirtKeys.h>
#endif

/* Custom widgets */
#include "Xc.h"
#include "Meter.h"
#include "Indicator.h"
#include "BarGraph.h"
#include "Matrix.h"
#include "Byte.h"
#include "WheelSwitch.h"

#define NAME	"medm"
#define CLASS	"Medm"

/* Global X/Xt/Motif parameters */
EXTERN Display *display;
EXTERN XtAppContext appContext;
EXTERN Window rootWindow;
EXTERN int screenNum;
EXTERN Cursor helpCursor, closeCursor, printCursor,
  saveCursor, pasteCursor, pvCursor, crosshairCursor, watchCursor,
  rubberbandCursor, dragCursor, resizeCursor, xtermCursor,
  noWriteAccessCursor;
EXTERN Colormap cmap;
EXTERN Atom WM_DELETE_WINDOW;
EXTERN Atom WM_TAKE_FOCUS;
  /* These are the drag & drop exports we support */
EXTERN Atom compoundTextAtom;
EXTERN Atom textAtom;
EXTERN GC highlightGC;
EXTERN XtTranslations parsedTranslations;

extern unsigned long getPixelFromString(Display *display, int screen,
  char *colorString);
extern unsigned long getPixelFromStringW(Widget w, char *colorString);

extern void StartDrag(Widget w, XEvent *event);

#endif  /* __XTPARAMS_H__ */

