/*
 *	Mark Anderson, Argonne National Laboratory:
 *		U.S. DOE, University of Chicago
 */

#ifndef __MEDM_H__
#define __MEDM_H__

/* STANDARDS CONFORMANCE: AES, XPG2, XPG3, XPG4, POSIX.1, POSIX.2 */
#include <unistd.h>
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


/*
 * X/Xt/Xm includes, globals
 */
#include "xtParams.h"


/*
 * MEDM includes
 */
#include "widgetDM.h"
#include "caDM.h"
#include "parse.h"
#include "xgif.h"

#include "proto.h"

#include "epicsVersion.h"
#include "medmVersion.h"

/***
 *** and on with the rest of Medm
 ***/

#define MAIN_NAME "Medm"
#define OBJECT_PALETTE_NAME "Object "


/*
 * define the help layers
 */
#define HELP_MAIN 0



/*
 * global widgets (all permanent shells, most MWs, etc )
 */
EXTERN Widget mainShell, mainMW;
EXTERN Widget objectS, objectMW;
EXTERN Widget resourceS, resourceMW;
EXTERN Widget colorS, colorMW;
EXTERN Widget channelS, channelMW;
/* shells for related display, shell command,
	cartesian plot and strip chart data vectors */
EXTERN Widget relatedDisplayS, shellCommandS, cartesianPlotS,
	cartesianPlotAxisS, stripChartS;
EXTERN Widget cpAxisForm, executeTimeCartesianPlotWidget;

EXTERN Widget exitQD, closeQD, saveAsPD;

/* the global Help Information Dialog */
EXTERN Widget helpS, helpMessageBox;


/* in main shell: labels on bulletin board for current display information */
EXTERN Widget statusBB, displayL, nElementsL, nColorsL;

/* currently specified image type (from ObjectPalette's OpenFSD) */
EXTERN ImageType imageType;

/* resource bundle stuff */
#define SELECTION_BUNDLE 0
EXTERN int resourceBundleCounter;



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

#endif  /* __MEDM_H__ */
