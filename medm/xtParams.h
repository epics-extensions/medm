/*
*****************************************************************
                          COPYRIGHT NOTIFICATION
*****************************************************************

THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
AND IN ALL SOURCE LISTINGS OF THE CODE.

(C)  COPYRIGHT 1993 UNIVERSITY OF CHICAGO

Argonne National Laboratory (ANL), with facilities in the States of
Illinois and Idaho, is owned by the United States Government, and
operated by the University of Chicago under provision of a contract
with the Department of Energy.

Portions of this material resulted from work developed under a U.S.
Government contract and are subject to the following license:  For
a period of five years from March 30, 1993, the Government is
granted for itself and others acting on its behalf a paid-up,
nonexclusive, irrevocable worldwide license in this computer
software to reproduce, prepare derivative works, and perform
publicly and display publicly.  With the approval of DOE, this
period may be renewed for two additional five year periods.
Following the expiration of this period or periods, the Government
is granted for itself and others acting on its behalf, a paid-up,
nonexclusive, irrevocable worldwide license in this computer
software to reproduce, prepare derivative works, distribute copies
to the public, perform publicly and display publicly, and to permit
others to do so.

*****************************************************************
                                DISCLAIMER
*****************************************************************

NEITHER THE UNITED STATES GOVERNMENT NOR ANY AGENCY THEREOF, NOR
THE UNIVERSITY OF CHICAGO, NOR ANY OF THEIR EMPLOYEES OR OFFICERS,
MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL
LIABILITY OR RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR
USEFULNESS OF ANY INFORMATION, APPARATUS, PRODUCT, OR PROCESS
DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT INFRINGE PRIVATELY
OWNED RIGHTS.

*****************************************************************
LICENSING INQUIRIES MAY BE DIRECTED TO THE INDUSTRIAL TECHNOLOGY
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (630-252-2000).
*/
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

