/*
 * X/Xt/Xm includes, NAME and CLASS for Xt contexts and shells....
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
#include <Xm/Label.h>
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
#include <Xm/CascadeB.h>
#include <Xm/CascadeBG.h>
#include <Xm/SelectioB.h>

/*
 * custom widgets
 */
#include "Xc.h"
#include "Meter.h"
#include "Indicator.h"
#include "BarGraph.h"
#include "Matrix.h"

/*
 * graphX graphics includes
 */
#include "GraphXMacros.h"
#include "Seql.h"
#include "Strip.h"
#include "Graph.h"

/*
 * commercial widgets
 */
#include "XrtGraph.h"






extern char *graphXGetBestFont();


#define NAME	"medm"
#define CLASS	"Medm"


/*
 * global X/Xt/Motif parameters
  */
EXTERN Display *display;
EXTERN XtAppContext appContext;
EXTERN Window rootWindow;
EXTERN int screenNum;
EXTERN Cursor helpCursor, closeCursor, printCursor,
	saveCursor, crosshairCursor, watchCursor,
	rubberbandCursor, dragCursor, resizeCursor, xtermCursor;
EXTERN Colormap cmap;
EXTERN Atom WM_DELETE_WINDOW;
EXTERN Atom WM_TAKE_FOCUS;
EXTERN Atom COMPOUND_TEXT;
EXTERN GC highlightGC;
EXTERN XtTranslations parsedTranslations;

extern unsigned long getPixelFromString(Display *display, int screen,
	char *colorString);
extern unsigned long getPixelFromStringW(Widget w, char *colorString);

extern void StartDrag(Widget w, XEvent *event);


#ifndef ALLOCATE_STORAGE
  extern char *dragTranslations;
  extern XtActionsRec *dragActions;
#else
  static char dragTranslations[] = "#override <Btn2Down>:StartDrag()";
  static XtActionsRec dragActions[] = {{"StartDrag",(XtActionProc)StartDrag}};
#endif


#endif  /* __XTPARAMS_H__ */
