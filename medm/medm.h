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

#ifndef __MEDM_H__
#define __MEDM_H__
#undef __MONITOR_CA_PEND_EVENT__
#define SUPPORT_0201XX_FILE_FORMAT


#include <limits.h>
#include <float.h>	/* XPG4 limits.h doesn't include float.h */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* PATH_MAX */
#ifdef WIN32
/* Is in stdlib.h for WIN32 */
# define PATH_MAX _MAX_PATH
#else
/* May be in limits.h.  Kludge it if not */
# ifndef PATH_MAX
# define PATH_MAX 1024
# endif
#endif

/* For dumb SUNOS and GNU... */
#ifndef FLT_MAX		/* FLT_MAX is supposed to be in limits.h/float.h */
#  define FLT_MAX ((float)1.+30)
#endif
#ifndef M_PI		/* Similarly M_PI should be in math.h */
#  define M_PI    3.14159265358979323846
#endif

#ifdef ALLOCATE_STORAGE
#define EXTERN
#else
#define EXTERN extern
#endif

#if defined(XRTGRAPH) || defined(SCIPLOT) || defined(JPT)
#define CARTESIAN_PLOT
#endif

#ifdef __cplusplus
#define UNREFERENCED(x) (x)
#else
#define UNREFERENCED(x)
#endif

#ifdef __cplusplus
extern "C" {
#endif

  /* X/Xt/Xm includes, globals */
#include "xtParams.h"

  /* WIN32 differences */
#ifdef WIN32
  /* Path delimiter is different */
# define MEDM_PATH_DELIMITER ';'
# define MEDM_DIR_DELIMITER_CHAR '\\'
# define MEDM_DIR_DELIMITER_STRING "\\"
  /* Hummingbird extra functions including lprintf
   *   Needs to be included after Intrinsic.h for Exceed 5
   *   (Intrinsic.h is included in xtParams.h) */
# include <X11/XlibXtra.h>
  /* The following is done in Exceed 6 but not in Exceed 5
   *   Need it to define printf as lprintf for Windows
   *   (as opposed to Console) apps */
# ifdef _WINDOWS
#  ifndef printf
#   define printf lprintf
#  endif
# endif
#else /* #ifdef WIN32 */
  /* Path delimiter is different */
# define MEDM_PATH_DELIMITER ':'
# define MEDM_DIR_DELIMITER_CHAR '/'
# define MEDM_DIR_DELIMITER_STRING "/"
  /* WIN32 does not have unistd.h */
# include <unistd.h>
#endif /* #ifdef WIN32 */

#ifdef MEDM_CDEV
#include "medmCdev.h"
#else
#include "epicsVersion.h"
#include "medmCA.h"
#endif

#ifdef VMS
#include "medmVMS.h"
#endif

#include "siteSpecific.h"
#include "medmWidget.h"
#include "parse.h"
#include "xgif.h"
#include "utilPrint.h"
#include "medmVersion.h"

#define MAIN_NAME "Medm"
#define OBJECT_PALETTE_NAME "Object "
  /* The following works on both WIN32 and Solaris */
#define STRFTIME_FORMAT "%a %b %d %H:%M:%S %Z %Y"

  /* Define the help layers  */
#define HELP_MAIN 0

    typedef struct menuEntry{
	char*           label;
	WidgetClass*    widgetClass;
	char            mnemonic;
	char*           accelerator;
	char*           accText;
	Widget          widget;
	XtCallbackProc  callback;
	XtPointer       callbackData;
	struct menuEntry *subItems;
    } menuEntry_t;

    typedef void(*medmExecProc)(DisplayInfo *,DlElement *);
    typedef void(*medmWriteProc)(FILE *,DlElement *,int);
    typedef void(*medmSetGetProc)(ResourceBundle *, DlElement *);

#include "proto.h"

  /* pixmap names : must be accessible by program according to Motif rules:

     rectangle25
     oval25
     arc25
     text25
     line25
     polyline25
     polygon25
     bezierCurve25

     meter25
     bar25
     indicator25
     textUpdate25
     stripChart25
     cartesianPlot25
     surfacePlot25

     choiceButton25
     messageButton25
     menu25
     textEntry25
     valuator25

     relatedDisplay25
     shellCommand25
     wheelSwitch25
  */

  /* Global variables */

  /* Help URL */
    EXTERN char medmHelpPath[PATH_MAX];

  /* XWD file */
    EXTERN char xwdFile[PATH_MAX];

  /* Window property atom */
    EXTERN Atom windowPropertyAtom;
    EXTERN char **execMenuCommandList;

  /* Global widgets (all permanent shells, most MWs, etc ) */
    EXTERN Widget mainShell, mainMW;
    EXTERN Widget objectS, objectMW;
    EXTERN Widget resourceS, resourceMW;
    EXTERN Widget colorS, colorMW;
    EXTERN Widget channelS, channelMW;

  /* Shells for related display, shell command,
   * Cartesian plot and strip chart data vectors */
    EXTERN Widget relatedDisplayS, shellCommandS, cartesianPlotS,
      cartesianPlotAxisS, stripChartS;
    EXTERN Widget cmdForm, cpAxisForm;
    EXTERN Widget executeTimeCartesianPlotWidget;
    EXTERN DlElement *executeTimePvLimitsElement;
    EXTERN DlElement *executeTimeStripChartElement;

    EXTERN Widget exitQD, saveAsPD;
    EXTERN Boolean saveReplacedDisplays;
    EXTERN Boolean popupExistingDisplay;

  /* Help information dialogs */
    EXTERN Widget helpS, helpMessageBox;
    EXTERN Widget editHelpS, editHelpMessageBox;
    EXTERN Widget pvInfoS, pvLimitsS, pvInfoMessageBox, printSetupS;
    EXTERN Widget displayListS;
    EXTERN Widget errMsgS, errMsgSendS, caStudyS;

  /* In main shell: labels on bulletin board for current display information */
    EXTERN Widget statusBB, displayL, nElementsL, nColorsL;

  /* Currently specified image type (from ObjectPalette's OpenFSD) */
    EXTERN ImageType imageType;

  /* Resource bundle stuff */
#define SELECTION_BUNDLE 0
    EXTERN int resourceBundleCounter;

    EXTERN XtWorkProcId medmWorkProcId;
    EXTERN long medmUpdateRequestCount;
    EXTERN long medmCAEventCount, medmScreenUpdateCount, medmUpdateMissedCount;
    EXTERN int medmRaiseMessageWindow;
    EXTERN Widget caStudyLabel;
    EXTERN XtIntervalId medmStatusIntervalId;
    EXTERN Boolean MedmUseNewFileFormat;
    EXTERN Dimension maxLabelWidth;
    EXTERN Dimension maxLabelHeight;
    EXTERN Widget cpMatrix, cpForm;
    EXTERN String dashes;

  /* Time data */
    EXTERN time_t time900101, time700101, timeOffset;

  /* Drag & Drop */
#if USE_DRAGDROP
#ifndef ALLOCATE_STORAGE
    extern char *dragTranslations;
    extern XtActionsRec *dragActions;
#else
  /* KE: This used to be None<Btn2Down> but NumLock became a modifier
     key on Solaris 8 and it didn't work (and in addition caused a
     slider to move the cursor) */
    static char dragTranslations[] = "#override <Btn2Down>:StartDrag()";
    static XtActionsRec dragActions[] = {{"StartDrag",(XtActionProc)StartDrag}};
#endif
#endif

  /* XR5 Resource ID patch */
#ifdef USE_XR5_RESOURCEID_PATCH
#  define XCreatePixmap XPatchCreatePixmap
#  define XFreePixmap XPatchFreePixmap
#  define XCreateGC XPatchCreateGC
#  define XFreeGC XPatchFreeGC
#endif

#ifdef __cplusplus
	   }  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif  /* __MEDM_H__ */
