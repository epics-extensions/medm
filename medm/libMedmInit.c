/*
 *	Mark Anderson, Argonne National Laboratory:
 *		U.S. DOE, University of Chicago
 */

/***
 *** these routines and file roughly correspond to the medm.c file and
 ***  the normal (non-embedded) MEDM main() entry point
 ***/

#define ALLOCATE_STORAGE
#include "medm.h"
#include <Xm/RepType.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>

/*
 * globals needed for remote display invocation...
 *   have limited scope (main(), libMedmInit() and dmTerminateX() are only
 *   routines which care
 */
Atom MEDM_EDIT_FIXED = (Atom)NULL, MEDM_EXEC_FIXED = (Atom)NULL,
	   MEDM_EDIT_SCALABLE = (Atom)NULL, MEDM_EXEC_SCALABLE = (Atom)NULL;

/*
 * Jeff claims you need this even if using some fd mgr. funtion (select(fd...))
 *   for refreshing tcp/ip connections, CA "flow control", etc
 */
static XtTimerCallbackProc caHeartBeat(XtPointer dummy)
{
   ca_pend_event(CA_PEND_EVENT_TIME);    /* need this rather than ca_pend_io */

   /* reregister 2 second TimeOut function to handle lost connections, etc */
   XtAppAddTimeOut(appContext,2000,(XtTimerCallbackProc)caHeartBeat,NULL);
}



/***********************************************************************
 ************ libMedmInit()
 ***********************************************************************/

void libMedmInit(
  XtAppContext passedInAppContext,
  Widget passedInMainShell,
  char *fontString)
{
  int c, i, n, index;
  Arg args[5];
  XColor color;

#define FONT_NAME_SIZE 80
  char displayFont[FONT_NAME_SIZE];


/* need private local copy of this string to be passed to medmInit() */
  strncpy(displayFont,fontString,FONT_NAME_SIZE-1);
  displayFont[FONT_NAME_SIZE-1] = '\0';



/*
 * initialize channel access here (to get around orphaned windows)
 */
  SEVCHK(ca_task_initialize(),"\nmain: error in ca_task_initialize");

/* initialize a few globals */
  privateCmap = False;


/* add necessary Motif resource converters */
  XmRegisterConverters();
  XmRepTypeInstallTearOffModelConverter();

  appContext = passedInAppContext;
  display = XtDisplay(passedInMainShell);
  mainShell = passedInMainShell;

  screenNum = DefaultScreen(display);
  rootWindow = RootWindow(display,screenNum);
  cmap = DefaultColormap(display,screenNum);	/* X default colormap */


/* add translations/actions for drag-and-drop */
  parsedTranslations = XtParseTranslationTable(dragTranslations);
  XtAppAddActions(appContext,dragActions,XtNumber(dragActions));

  globalDisplayListTraversalMode = DL_EXECUTE;

/*
 * initialize some globals
 */
  globalModifiedFlag = False;
  mainMW = NULL;
  objectS = NULL; objectMW = NULL;
  colorS = NULL; colorMW = NULL;
  resourceS = NULL; resourceMW = NULL,
  channelS = NULL;channelMW = NULL;
  relatedDisplayS = NULL; shellCommandS = NULL;
  cartesianPlotS = NULL; cartesianPlotAxisS = NULL; stripChartS = NULL;
  cpAxisForm = NULL; executeTimeCartesianPlotWidget = NULL;
  currentDisplayInfo = NULL;
  pointerInDisplayInfo = NULL;
  resourceBundleCounter = 0;
  clipboardDelete = False;
  currentElementType = 0;


  /* not really unphysical, but being used for unallocable color cells */
  unphysicalPixel = BlackPixel(display,screenNum);

/* initialize the default colormap */

  for (i = 0; i < DL_MAX_COLORS; i++) {

  /* scale [0,255] to [0,65535] */
    color.red  = (unsigned short) COLOR_SCALE*(defaultDlColormap.dl_color[i].r);
    color.green= (unsigned short) COLOR_SCALE*(defaultDlColormap.dl_color[i].g);
    color.blue = (unsigned short) COLOR_SCALE*(defaultDlColormap.dl_color[i].b);
  /* allocate a shareable color cell with closest RGB value */
    if (XAllocColor(display,cmap,&color)) {
	defaultColormap[i] =  color.pixel;
    } else {
	fprintf(stderr,"\nmain: couldn't allocate requested color");
	/* put unphysical pixmap value in there as tag it was invalid */
	defaultColormap[i] =  unphysicalPixel;
    }

  }
  currentColormap = defaultColormap;
  currentColormapSize = DL_MAX_COLORS;

  /* and initialize the global resource bundle */
  initializeGlobalResourceBundle();
  globalResourceBundle.next = NULL;
  globalResourceBundle.prev = NULL;

  /* default action for MB in display is select (regulated by object palette) */
  currentActionType = SELECT_ACTION;

/*
 * intialize MEDM stuff
 */
  medmInit(displayFont);

/* add 2 second TimeOut function to handle lost CA connections, etc */
  XtAppAddTimeOut(appContext,2000,(XtTimerCallbackProc)caHeartBeat,NULL);

}


