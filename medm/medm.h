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

#ifndef __MEDM_H__
#define __MEDM_H__
#undef __MONITOR_CA_PEND_EVENT__
#define SUPPORT_0201XX_FILE_FORMAT


#include <limits.h>
#include <float.h>	/* XPG4 limits.h doesn't include float.h */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* for dumb SUNOS and GNU... */
#ifndef FLT_MAX		/* FLT_MAX is supposed to be in limits.h/float.h */
#  define FLT_MAX ((float)1.+30)
#endif
#ifndef M_PI		/* similarly M_PI should be in math.h */
#  define M_PI    3.14159265358979323846
#endif

#ifdef ALLOCATE_STORAGE
#define EXTERN
#else
#define EXTERN extern
#endif

#if defined(XRTGRAPH) || defined(SCIPLOT)
#define CARTESIAN_PLOT
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

#include "medmWidget.h"
#include "parse.h"
#include "xgif.h"
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
#include "medmInitTask.h"
    
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
  */
    
  /* Global variables */
    
  /* Window property atom */
    Atom windowPropertyAtom;
    char **execMenuCommandList;

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
    EXTERN Widget cpAxisForm, executeTimeCartesianPlotWidget;
    EXTERN DlElement *executeTimePvLimitsElement;

    EXTERN Widget exitQD, saveAsPD;
    EXTERN Boolean saveReplacedDisplays;

  /* Help information dialogs */
    EXTERN Widget helpS, helpMessageBox;
    EXTERN Widget editHelpS, editHelpMessageBox;
    EXTERN Widget pvInfoS, pvLimitsS, pvInfoMessageBox;
    EXTERN Widget displayListS;
    EXTERN Widget errMsgS, errMsgSendS, caStudyS;

  /* In main shell: labels on bulletin board for current display information */
    EXTERN Widget statusBB, displayL, nElementsL, nColorsL;

  /* Currently specified image type (from ObjectPalette's OpenFSD) */
    EXTERN ImageType imageType;

  /* Resource bundle stuff */
#define SELECTION_BUNDLE 0
    EXTERN int resourceBundleCounter;
    extern utilPrint(Display *, Window, char *);

    EXTERN XtWorkProcId medmWorkProcId;
#ifndef MEDM_CDEV
    EXTERN Channel *nextToServe;
#endif
    EXTERN long medmUpdateRequestCount;
    EXTERN long medmCAEventCount, medmScreenUpdateCount, medmUpdateMissedCount;
    EXTERN Widget caStudyLabel;
    EXTERN XtIntervalId medmStatusIntervalId;
    EXTERN Boolean MedmUseNewFileFormat;
    EXTERN Dimension maxLabelWidth;
    EXTERN Dimension maxLabelHeight;
    EXTERN Widget cpMatrix, cpForm;
    EXTERN String dashes;
    
  /* Time data */
    EXTERN time_t time900101, time700101, timeOffset;

#ifdef __cplusplus
	   }  /* Close scope of 'extern "C"' declaration which encloses file. */
#endif

#endif  /* __MEDM_H__ */
