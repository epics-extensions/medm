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

#ifndef CARTESIAN_PLOT_H
#define CARTESIAN_PLOT_H

#include "XrtGraph.h"

#if XRT_VERSION > 2
#include <XrtGraphProp.h>
#else
typedef XrtData * XrtDataHandle;
#endif

#define CP_XDATA_COLUMN         0
#define CP_YDATA_COLUMN         1
#define CP_COLOR_COLUMN         2
 
#define CP_APPLY_BTN    0
#define CP_CLOSE_BTN    1
 
typedef struct {
    float axisMin;
    float axisMax;
    Boolean isCurrentlyFromChannel;
} CartesianPlotAxisRange;

typedef enum {
    CP_XYScalar,
    CP_XScalar,         CP_YScalar,
    CP_XVector,         CP_YVector,
    CP_XVectorYScalar,
    CP_YVectorXScalar,
    CP_XYVector
} XYChannelTypeEnum;

typedef struct {
    struct _CartesianPlot *cartesianPlot;
    XrtDataHandle         hxrt;
    int                   trace;
    Record                *recordX;
    Record                *recordY;
    XYChannelTypeEnum     type;
} XYTrace;

typedef struct _CartesianPlot {
    DlElement      *dlElement;
    XYTrace         xyTrace[MAX_TRACES];
    XYTrace         eraseCh;
    XYTrace         triggerCh;
    UpdateTask      *updateTask;
    int             nTraces;              /* number of traces (<=MAX_TRACES) */
    XrtDataHandle   hxrt1, hxrt2;         /* XrtData handles */
  /* Used for channel-based range determination (filled in at connect) */
    CartesianPlotAxisRange  axisRange[3]; /* X, Y, Y2 _AXIS_ELEMENT          */
    eraseMode_t     eraseMode;            /* erase mode */
    Boolean         dirty1;               /* xrtData1 needs screen update */
    Boolean         dirty2;               /* xrtData2 needs screen update */
    TS_STAMP        startTime;
    Boolean         timeScale;
} CartesianPlot;

#endif

/****************************************************************************
 * CARTESIAN PLOT DATA
 *********************************************************************/
static Widget cpMatrix = NULL, cpForm = NULL;
static String cpColumnLabels[] = {"X Data","Y Data","Color",};
static int cpColumnMaxLengths[] = {MAX_TOKEN_LENGTH-1,MAX_TOKEN_LENGTH-1,6,};
static short cpColumnWidths[] = {36,36,6,};
static unsigned char cpColumnLabelAlignments[] = {XmALIGNMENT_CENTER,

						  XmALIGNMENT_CENTER,XmALIGNMENT_CENTER,};
/* and the cpCells array of strings (filled in from globalResourceBundle...)
*/
static String cpRows[MAX_TRACES][3];
static String *cpCells[MAX_TRACES];
static String dashes = "******";
 
static Pixel cpColorRows[MAX_TRACES][3];
static Pixel *cpColorCells[MAX_TRACES];

/*********************************************************************
 * CARTESIAN PLOT AXIS DATA
 *********************************************************************/
 
/*
 * for the Cartesian Plot Axis Dialog, use the following static globals
 *   (N.B. This dialog has semantics for both EDIT and EXECUTE time
 *    operation)
 */
 
/* Widget cpAxisForm defined in medm.h since execute-time needs it too */
 
/* define array of widgets (for X, Y1, Y2) */
static Widget axisRangeMenu[3];                 /* X_AXIS_ELEMENT =0 */
static Widget axisStyleMenu[3];                 /* Y1_AXIS_ELEMENT=1 */
static Widget axisRangeMin[3], axisRangeMax[3]; /* Y2_AXIS_ELEMENT=2 */
static Widget axisRangeMinRC[3], axisRangeMaxRC[3];
static Widget axisTimeFormat;
 
#define CP_X_AXIS_STYLE   0
#define CP_Y_AXIS_STYLE   1
#define CP_Y2_AXIS_STYLE  2
#define CP_X_RANGE_STYLE  3
#define CP_Y_RANGE_STYLE  4
#define CP_Y2_RANGE_STYLE 5
#define CP_X_RANGE_MIN    6
#define CP_Y_RANGE_MIN    7
#define CP_Y2_RANGE_MIN   8
#define CP_X_RANGE_MAX    9
#define CP_Y_RANGE_MAX    10
#define CP_Y2_RANGE_MAX   11
#define CP_X_TIME_FORMAT  12
 
#define MAX_CP_AXIS_ELEMENTS    20
/* The following should be the largest of NUM_CP_TIME_FORMAT,
 *   NUM_CARTESIAN_PLOT_RANGE_STYLES, NUM_CARTESIAN_PLOT_AXIS_STYLES */
#define MAX_CP_AXIS_BUTTONS  NUM_CP_TIME_FORMAT
  
