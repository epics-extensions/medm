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

#define DEBUG_LOSING_FOCUS 0
#define DEBUG_CARTESIAN_PLOT_BORDER 0
#define DEBUG_CARTESIAN_PLOT_UPDATE 0
#define DEBUG_TIME 0
#define DEBUG_XRT 0
#define DEBUG_HISTOGRAM 0
#ifndef VMS
#define CHECK_NAN
#endif

#ifdef CHECK_NAN
/* Note that for Solaris isnand and isnanf are defined in ieeefp.h.
 *  isnan is the same as isnand and is supposed to be defined in math.h
 *    but it isn't.
 * For Sun4 there is no isnand or isnanf and isnan is defined in math.h.
 * Consequently we use isnan and prototype it explicitly.
 * This should work on all systems and be safe.
 */
#ifdef WIN32
#include <float.h>
#define isnan(a) _isnan(a)  /* For some reason MS has leading _ */
#else
#include <math.h>
extern int isnan(double);     /* Because it is not in math.h as it should be */
#endif /* WIN32 */
#define NAN_SUBSTITUTE 0.0
#define SAFEFLOAT(x) (safeFloat(x))
#else
#define SAFEFLOAT(x) ((float)(x))
#endif     /* End #ifdef CHECK_NAN */

#include "medm.h"
#include <Xm/MwmUtil.h>

/* Include the appropriate header files for the plot package */
#if defined(XRTGRAPH)
#include "medmXrtGraph.h"
#elif defined(SCIPLOT)
#include "medmSciPlot.h"
#else
#error No plot package to implement the Cartesian Plot has been defined.
#endif

#include "medmCartesianPlot.h"

/* Function prototypes */

static void cartesianPlotCreateRunTimeInstance(DisplayInfo *, DlElement *);
static void cartesianPlotCreateEditInstance(DisplayInfo *, DlElement *);
static void cartesianPlotUpdateGraphicalInfoCb(XtPointer cd);
static void cartesianPlotUpdateScreenFirstTime(XtPointer cd);
static void cartesianPlotDraw(XtPointer cd);
static void cartesianPlotUpdateValueCb(XtPointer cd);
static void cartesianPlotDestroyCb(XtPointer cd);
static void cartesianPlotGetRecord(XtPointer, Record **, int *);
static void cartesianPlotInheritValues(ResourceBundle *pRCB, DlElement *p);
static void cartesianPlotSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void cartesianPlotSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void cartesianPlotGetValues(ResourceBundle *pRCB, DlElement *p);

static void cartesianPlotAxisActivate(Widget w, XtPointer cd, XtPointer cbs);

Widget cpMatrix = NULL, cpForm = NULL;

static DlDispatchTable cartesianPlotDlDispatchTable = {
    createDlCartesianPlot,
    NULL,
    executeDlCartesianPlot,
    hideDlCartesianPlot,
    writeDlCartesianPlot,
    NULL,
    cartesianPlotGetValues,
    cartesianPlotInheritValues,
    cartesianPlotSetBackgroundColor,
    cartesianPlotSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

static String cpColumnLabels[] = {"X Data","Y Data","Color",};
static int cpColumnMaxLengths[] = {MAX_TOKEN_LENGTH-1,MAX_TOKEN_LENGTH-1,6,};
static short cpColumnWidths[] = {36,36,6,};
static unsigned char cpColumnLabelAlignments[] = {
    XmALIGNMENT_CENTER, XmALIGNMENT_CENTER, XmALIGNMENT_CENTER,};

static String cpRows[MAX_TRACES][3];
static String *cpCells[MAX_TRACES];
 
static Pixel cpColorRows[MAX_TRACES][3];
static Pixel *cpColorCells[MAX_TRACES];

static Widget axisRangeMenu[3];                 /* X_AXIS_ELEMENT =0 */
static Widget axisStyleMenu[3];                 /* Y1_AXIS_ELEMENT=1 */
static Widget axisRangeMin[3], axisRangeMax[3]; /* Y2_AXIS_ELEMENT=2 */
static Widget axisRangeMinRC[3], axisRangeMaxRC[3];
static Widget axisTimeFormat;

char *cpTimeFormatString[NUM_CP_TIME_FORMAT] = {
    "%H:%M:%S",
    "%H:%M",
    "%H:00",
    "%b %d, %Y",
    "%b %d",
    "%b %d %H:00",
    "%a %H:00"};

#ifdef CHECK_NAN
float safeFloat(double x) {
    static int nerrs = 0;
    static int print = 1;
    
    if(isnan(x)) {
	if(print) {
	    if( nerrs < 25) {
		nerrs++;
		medmPostMsg(0,"CartesianPlot: "
		  "Value is NaN, using %g\n",NAN_SUBSTITUTE);
		if(nerrs >= 25) {
		    medmPrintf(0,"\nCartesianPlot: "
		      "Suppressing further NaN error messages\n");
		    print = 0;
		}
	    }
	}
	return NAN_SUBSTITUTE;
    } else {
	return (float)x;
    }
}
#endif

static void cartesianPlotCreateRunTimeInstance(DisplayInfo *displayInfo,
  DlElement *dlElement)
{
    MedmCartesianPlot *pcp;
    int i, validTraces;
    Widget localWidget;
    DlCartesianPlot *dlCartesianPlot = dlElement->structure.cartesianPlot;

  /* Allocate a MedmCartesianPlot and fill part of it in */
    if(dlElement->data) {
	pcp=(MedmCartesianPlot *)dlElement->data;
    } else {
	pcp = (MedmCartesianPlot *)malloc(sizeof(MedmCartesianPlot));
	dlElement->data = pcp;
	if(pcp == NULL) {
	    medmPrintf(1,"\ncartesianPlotCreateRunTimeInstance:"
	      " Memory allocation error\n");
	    return;
	}
      /* Pre-initialize */
	pcp->updateTask = NULL;
	pcp->hcp1 = pcp->hcp2 = NULL;
	pcp->dirty1 = pcp->dirty2 = False;
	pcp->timeScale = False;
	pcp->startTime.secPastEpoch = 0;
	pcp->startTime.nsec = 0;
	pcp->eraseMode = dlCartesianPlot->eraseMode;
	pcp->dlElement = dlElement;
	pcp->updateTask = updateTaskAddTask(displayInfo,
	  &(dlCartesianPlot->object),
	  cartesianPlotDraw,
	  (XtPointer)pcp);
	if(pcp->updateTask == NULL) {
	    medmPrintf(1,"\ncartesianPlotCreateRunTimeInstance: "
	      "Memory allocation error\n");
	} else {
	    updateTaskAddDestroyCb(pcp->updateTask,cartesianPlotDestroyCb);
	    updateTaskAddNameCb(pcp->updateTask,cartesianPlotGetRecord);
	}
	
      /* Allocate (or set to NULL) the Records in the xyTrace's XYTraces */
      /*   Note: Set the updateValueCb for the time being to
       *     cartesianPlotUpdateScreenFirstTime(), which will change it
       *     to cartesianPlotUpdateValueCb() when it is finished */
	validTraces = 0;
	for(i = 0; i < MAX_TRACES; i++) {
	    Boolean validTrace = False;
	  /* X data */
	    if(dlCartesianPlot->trace[i].xdata[0] != '\0') {
		pcp->xyTrace[validTraces].recordX = NULL;
		pcp->xyTrace[validTraces].recordX =
		  medmAllocateRecord(dlCartesianPlot->trace[i].xdata,
		    cartesianPlotUpdateScreenFirstTime,
		    cartesianPlotUpdateGraphicalInfoCb,
		    (XtPointer) &(pcp->xyTrace[validTraces]));
		validTrace = True;
	    } else {
		pcp->xyTrace[validTraces].recordX = NULL;
	    }
	  /* Y data */
	    if(dlCartesianPlot->trace[i].ydata[0] != '\0') {
		pcp->xyTrace[validTraces].recordY = NULL;
		pcp->xyTrace[validTraces].recordY = 
		  medmAllocateRecord(dlCartesianPlot->trace[i].ydata,
		    cartesianPlotUpdateScreenFirstTime,
		    cartesianPlotUpdateGraphicalInfoCb,
		    (XtPointer) &(pcp->xyTrace[validTraces]));
		validTrace = True;
	    } else {
		pcp->xyTrace[validTraces].recordY = NULL;
	    }
	    if(validTrace) {
		pcp->xyTrace[validTraces].cartesianPlot = pcp;
		validTraces++;
	    }
	}
	
      /* If no xyTraces, create one fake one */
	if(validTraces == 0) {
	    validTraces = 1;
	    pcp->xyTrace[0].recordX = NULL;
	    pcp->xyTrace[0].recordX = medmAllocateRecord(" ",
	      cartesianPlotUpdateScreenFirstTime,
	      cartesianPlotUpdateGraphicalInfoCb,
	      (XtPointer) &(pcp->xyTrace[0]));
	    pcp->xyTrace[0].recordY = NULL;
	}
	
      /* Record the number of traces in the cartesian plot */
	pcp->nTraces = validTraces;
	
      /* Allocate (or set to NULL) the X Record in the eraseCh XYTrace */
	if((dlCartesianPlot->erase[0] != '\0') && (validTraces > 0)) {
	    pcp->eraseCh.recordX = NULL;
	    pcp->eraseCh.recordX =
	      medmAllocateRecord(dlCartesianPlot->erase,
		cartesianPlotUpdateScreenFirstTime,
		cartesianPlotUpdateGraphicalInfoCb,
		(XtPointer) &(pcp->eraseCh));
	    pcp->eraseCh.cartesianPlot = pcp;
	} else {
	    pcp->eraseCh.recordX = NULL;
	}
	
      /* Allocate (or set to NULL) the X Record in the triggerCh XYTrace */
	if((dlCartesianPlot->trigger[0] != '\0') && (validTraces > 0)) {
	    pcp->triggerCh.recordX =  NULL;
	    pcp->triggerCh.recordX = 
	      medmAllocateRecord(dlCartesianPlot->trigger,
		cartesianPlotUpdateScreenFirstTime,
		cartesianPlotUpdateGraphicalInfoCb,
		(XtPointer) &(pcp->triggerCh));
	    pcp->triggerCh.cartesianPlot = pcp;
	} else {
	    pcp->triggerCh.recordX = NULL;
	}
	
      /* Note: Only the Record and MedmCartesianPlot parts of the
       *   XYTrace's are filled in now, the rest is filled in in
       *   cartesianPlotUpdateGraphicalInfoCb() */
	
	drawWhiteRectangle(pcp->updateTask);
    }

    if(!dlElement->widget) {
	localWidget = CpCreateCartesianPlot(displayInfo,
	  dlCartesianPlot, pcp);
	dlElement->widget = localWidget;
    }
}

static void cartesianPlotCreateEditInstance(DisplayInfo *displayInfo,
  DlElement *dlElement)
{
    MedmCartesianPlot *pcp;
    Widget localWidget;
    DlCartesianPlot *dlCartesianPlot = dlElement->structure.cartesianPlot;

    pcp = NULL;

    localWidget = CpCreateCartesianPlot(displayInfo,
      dlCartesianPlot, pcp);
    dlElement->widget = localWidget;

  /* Add handlers */
    addCommonHandlers(localWidget, displayInfo);

    XtManageChild(localWidget);
}

void executeDlCartesianPlot(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(dlElement->widget) {
	DlObject *po = &(dlElement->structure.cartesianPlot->object);
	XtVaSetValues(dlElement->widget,
	  XmNx, (Position) po->x,
	  XmNy, (Position) po->y,
	  XmNwidth, (Dimension) po->width,
	  XmNheight, (Dimension) po->height,
	  NULL);
    } else if(displayInfo->traversalMode == DL_EXECUTE) {
	cartesianPlotCreateRunTimeInstance(displayInfo, dlElement);
    } else if(displayInfo->traversalMode == DL_EDIT) {
	cartesianPlotCreateEditInstance(displayInfo, dlElement);
    }
}

void hideDlCartesianPlot(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element with a widget */
    hideWidgetElement(displayInfo, dlElement);
}

static void cartesianPlotUpdateGraphicalInfoCb(XtPointer cd) {
    Record *pr = (Record *)cd;
    XYTrace *pt = (XYTrace *)pr->clientData;
    MedmCartesianPlot *pcp = pt->cartesianPlot;
    DisplayInfo *displayInfo = pcp->updateTask->displayInfo;
    DlCartesianPlot *dlCartesianPlot = pcp->dlElement->structure.cartesianPlot;
    Widget widget = pcp->dlElement->widget;
    XColor xColors[MAX_TRACES];
    CpDataHandle hcp, hcp1, hcp2;
    float minX, maxX, minY, maxY, minY2, maxY2;
    XcVType minXF, maxXF, minYF, maxYF, minY2F, maxY2F, tickF;
    char string[24];
    int iPrec, kk, pointSize;
    int maxElements = 0;
    int i, j, n;

#if DEBUG_HISTOGRAM
    print("\ncartesianPlotUpdateGraphicalInfoCb: name=%s lopr=%g hopr=%g \n",
      pr->name,pr->lopr, pr->hopr);
#endif    

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! This is a temporary work around !!!!! */
  /* !!!!! for the reconnection.           !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  /* KE: This causes it to not do anything for the reconnection */
    medmRecordAddGraphicalInfoCb(pr,NULL);

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! End work around                 !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  /* Return until all channels get their graphical information */
    for(i = 0; i < pcp->nTraces; i++) {
	XYTrace *t = &(pcp->xyTrace[i]);
	if(t->recordX) {
	    if(t->recordX->precision < 0) return;
	}

	if(t->recordY) {
	    if(t->recordY->precision < 0) return;
	}
    }

  /* Get the colors for all the possible traces */
    for(i = 0; i < pcp->nTraces; i++)
      xColors[i].pixel =
	displayInfo->colormap[dlCartesianPlot->trace[i].data_clr];
    XQueryColors(XtDisplay(widget),cmap,xColors,pcp->nTraces);

  /* Set the maximum count (for allocating data structures) to be the
   *   largest of any of the Channel Access counts and the specified
       count (from the DlCartesianPlot) */
    maxElements = dlCartesianPlot->count;
    for(i = 0; i < pcp->nTraces; i++) {
	XYTrace *t = &(pcp->xyTrace[i]);
	if(t->recordX)
	  maxElements = MAX(maxElements,(int)t->recordX->elementCount);
	if(t->recordY)
	  maxElements = MAX(maxElements,(int)t->recordY->elementCount);
    }

  /* Allocate the CP data structures with size maxElements */
    hcp1 = hcp2 = NULL;
    hcp1 = CpDataCreate(widget,CP_GENERAL,1,maxElements);
    if(pcp->nTraces > 1) {
	hcp2 = CpDataCreate(widget,CP_GENERAL,pcp->nTraces-1,maxElements);
    }

  /* Loop over all the traces */
    minX = minY = minY2 = FLT_MAX;
    maxX = maxY = maxY2 = (float)(-0.99*FLT_MAX);
    for(i = 0; i < pcp->nTraces; i++) {
	XYTrace *t = &(pcp->xyTrace[i]);

      /* Set as uninitialized (Used for incrementing last point to
         distinguish between no points and one point) */
	 t->init=0;

      /* Determine data type (based on type (scalar or vector) of data) */
	if(t->recordX && t->recordY) {
	    if( t->recordX->elementCount > 1 && t->recordY->elementCount > 1 ) {
		t->type = CP_XYVector;
	    } else if( t->recordX->elementCount > 1 &&
	      t->recordY->elementCount <= 1 ) {
		t->type = CP_XVectorYScalar;
	    } else if( t->recordY->elementCount > 1 &&
	      t->recordX->elementCount <= 1 ) {
		t->type = CP_YVectorXScalar;
	    } else if( t->recordX->elementCount <= 1 &&
	      t->recordY->elementCount <= 1 ) {
		t->type = CP_XYScalar;
	    }
	  /* Put initial data in */
	    if(i <= 0) {
		CpDataSetLastPoint(hcp1,i,0);
		minY = (float)MIN(minY,t->recordY->lopr);
		maxY = (float)MAX(maxY,t->recordY->hopr);
	    } else {
		CpDataSetLastPoint(hcp2,i-1,0);
		minY2 = (float)MIN(minY2,t->recordY->lopr);
		maxY2 = (float)MAX(maxY2,t->recordY->hopr);
	    }
	    minX = (float)MIN(minX,t->recordX->lopr);
	    maxX = (float)MAX(maxX,t->recordX->hopr);
	} else if(t->recordX) {
	  /* X channel - supporting scalar or waveform */
	    if(t->recordX->elementCount > 1) {
	      /* Vector/waveform */
		t->type = CP_XVector;
		if(i <= 0) {
		    for(j = 0; j < (int)t->recordX->elementCount; j++)
		      CpDataSetYElement(hcp1,i,j,(float)j);
		    minY = (float)MIN(minY,0.);
		    maxY = (float)MAX(maxY,
		      (float)((int)t->recordX->elementCount-1));
		} else {
		    for(j = 0; j < (int)t->recordX->elementCount; j++)
		     CpDataSetYElement(hcp2,i-1,j,(float)j);
		    minY2 = (float)MIN(minY2,0.);
		    maxY2 = (float)MAX(maxY2,(float)t->recordX->elementCount-1);
		}
		minX = (float)MIN(minX,t->recordX->lopr);
		maxX = (float)MAX(maxX,t->recordX->hopr);
	    } else {
	      /* Scalar */
		t->type = CP_XScalar;
		if(i <= 0) {
		    for(j = 0; j < dlCartesianPlot->count; j++)
		      CpDataSetYElement(hcp1,i,j,(float)j);
		    CpDataSetXElement(hcp1,i,0,(float)t->recordX->value);
		    minY = (float)MIN(minY,0.);
		    maxY = (float)MAX(maxY,(float)dlCartesianPlot->count);
		} else {
		    for(j = 0; j < dlCartesianPlot->count; j++)
		      CpDataSetYElement(hcp2,i-1,j,(float)j);
		    CpDataSetXElement(hcp2,i-1,0,(float)t->recordX->value);
		    minY2 = (float)MIN(minY2,0.);
		    maxY2 = (float)MAX(maxY2,(float)dlCartesianPlot->count);
		}
		minX = (float)MIN(minX,t->recordX->lopr);
		maxX = (float)MAX(maxX,t->recordX->hopr);
	    }
	} else if(t->recordY) {
	  /* Y channel - supporting scalar or waveform */
	    if(t->recordY->elementCount > 1) {
	      /* Vector/waveform */
		t->type = CP_YVector;
		if(i <= 0) {
		    for(j = 0; j < t->recordY->elementCount; j++)
		      CpDataSetYElement(hcp1,i,j,(float)j);
		    minY = (float)MIN(minY,t->recordY->lopr);
		    maxY = (float)MAX(maxY,t->recordY->hopr);
		} else {
		    for(j = 0; j < t->recordY->elementCount; j++)
		      CpDataSetXElement(hcp2,i-1,j,(float)j);
		    minY2 = (float)MIN(minY2,t->recordY->lopr);
		    maxY2 = (float)MAX(maxY2,t->recordY->hopr);
		}
		minX = (float)MIN(minX,0.);
		maxX = (float)MAX(maxX,(float)(t->recordY->elementCount-1));
	    } else {
	      /* Scalar */
		t->type = CP_YScalar;
		if(i <= 0) {
		    for(j = 0; j < dlCartesianPlot->count; j++)
		      CpDataSetXElement(hcp1,0,j,(float)j);
		    CpDataSetYElement(hcp1,0,0,(float)t->recordY->value);
		    CpDataSetLastPoint(hcp1,0,0);
		    minY = (float)MIN(minY,t->recordY->lopr);
		    maxY = (float)MAX(maxY,t->recordY->hopr);
		} else {
		    for(j = 0; j < dlCartesianPlot->count; j++)
		      CpDataSetXElement(hcp2,0,j,(float)j);
		    CpDataSetYElement(hcp2,0,0,(float)t->recordY->value);
		    CpDataSetLastPoint(hcp2,0,0);
		    minY2 = (float)MIN(minY2,t->recordY->lopr);
		    maxY2 = (float)MAX(maxY2,t->recordY->hopr);
		}
		minX = (float)MIN(minX,0.);
		maxX = (float)MAX(maxX,(float)dlCartesianPlot->count);
	    }
	}
    }     /* End for loop over traces */
    
  /* Record the trace number and set the data pointers in the XYTrace */
    for(i = 0; i < pcp->nTraces; i++) {
	XYTrace *t = &(pcp->xyTrace[i]);

	if( i <= 0) {
	    t->hcp = hcp1;
	    t->trace = i;
	} else {
	    t->hcp = hcp2;
	    t->trace = i-1;
	}
    }

  /* Set the data pointers in the MedmCartesianPlot */
    pcp->hcp1 = hcp1;
    pcp->hcp2 = hcp2;

  /* Loop over traces and set CpDataStyle array */
    pointSize = MAX(2, dlCartesianPlot->object.height/70);
    for(i = 0; i < pcp->nTraces; i++) {
	hcp=i?hcp2:hcp1;
	switch(dlCartesianPlot->style) {
	case POINT_PLOT:
	    CpSetAxisStyle(widget, hcp, i, CP_LINE_NONE, CP_LINE_NONE,
	      xColors[i], pointSize);
	    break;
	case LINE_PLOT:
	    CpSetAxisStyle(widget, hcp, i, CP_LINE_SOLID, CP_LINE_NONE,
	      xColors[i], pointSize);
	    break;
	case FILL_UNDER_PLOT:
	    CpSetAxisStyle(widget, hcp, i, CP_LINE_SOLID, CP_LINE_SOLID,
	      xColors[i], pointSize);
	    break;
	}
    }

  /* Fill in connect-time channel-based range specifications
   *   This is different than the min/max stored in the display element */
    pcp->axisRange[X_AXIS_ELEMENT].axisMin = minX;
    pcp->axisRange[X_AXIS_ELEMENT].axisMax = maxX;
    pcp->axisRange[Y1_AXIS_ELEMENT].axisMin = minY;
    pcp->axisRange[Y1_AXIS_ELEMENT].axisMax = maxY;
    pcp->axisRange[Y2_AXIS_ELEMENT].axisMin = minY2;
    pcp->axisRange[Y2_AXIS_ELEMENT].axisMax = maxY2;
    for(kk = X_AXIS_ELEMENT; kk <= Y2_AXIS_ELEMENT; kk++) {
	if(dlCartesianPlot->axis[kk].rangeStyle == CHANNEL_RANGE) {
	    pcp->axisRange[kk].isCurrentlyFromChannel= True;
	} else {
	    pcp->axisRange[kk].isCurrentlyFromChannel = False;
	}
    }
    minXF.fval = minX;
    maxXF.fval = maxX;
    minYF.fval = minY;
    maxYF.fval = maxY;
    minY2F.fval = minY2;
    maxY2F.fval = maxY2;
    n = 0;
    if(dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle
      == CHANNEL_RANGE) {
	CpSetAxisMaxMin(widget, CP_X, maxXF, minXF);
    } else if(dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle
      == USER_SPECIFIED_RANGE) {
	int k;

	minXF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].minRange;
	maxXF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].maxRange;
	tickF.fval = (float)((maxXF.fval - minXF.fval)/4.0);
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while(string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while(string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	CpSetAxisAll(widget, CP_X, maxXF, minXF, tickF,
	  tickF, iPrec);
    }

  /* Set axis parameters for Y1 */
     if(dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle
       == CHANNEL_RANGE) {
	 CpSetAxisMaxMin(widget, CP_Y, maxYF, minYF);
     } else if(dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle
       == USER_SPECIFIED_RANGE) {
	 int k;

	 minYF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].minRange;
	 maxYF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange;
	 tickF.fval = (float)((maxYF.fval - minYF.fval)/4.0);
	 sprintf(string,"%f",tickF.fval);
	 k = strlen(string)-1;
	 while(string[k] == '0') k--; /* strip off trailing zeroes */
	 iPrec = k;
	 while(string[k] != '.' && k >= 0) k--;
	 iPrec = iPrec - k;
	 CpSetAxisAll(widget, CP_Y, maxYF, minYF, tickF,
	   tickF, iPrec);
     }

  /* Set axis parameters for Y2 */
    if(dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle
      == CHANNEL_RANGE) {
	CpSetAxisMaxMin(widget, CP_Y2, maxY2F, minY2F);

    } else if(dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle
      == USER_SPECIFIED_RANGE) {
	int k;

	minY2F.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].minRange;
	maxY2F.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].maxRange;
	tickF.fval = (float)((maxY2F.fval - minY2F.fval)/4.0);
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while(string[k] == '0') k--; /* strip off trailing zeroes */
	iPrec = k;
	while(string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
	CpSetAxisAll(widget, CP_Y2, maxY2F, minY2F, tickF,
	  tickF, iPrec);
    }
    
#if 0
  /* Add destroy callback */
    XtAddCallback(widget,XmNdestroyCallback,
      (XtCallbackProc)monitorDestroy, (XtPointer)pcp);
#endif

  /* Now call the plot update routine */
    cartesianPlotUpdateScreenFirstTime(cd);
}

void cartesianPlotUpdateTrace(XtPointer cd) {
    Record *pr = (Record *) cd;

    XYTrace *pt = (XYTrace *) pr->clientData;
    MedmCartesianPlot *pcp = pt->cartesianPlot;
    DlCartesianPlot *dlCartesianPlot = pcp->dlElement->structure.cartesianPlot;
    int nextPoint, j;

#if DEBUG_CARTESIAN_PLOT_UPDATE
    printf("cartesianPlotUpdateTrace:\n");
#endif    
    switch(pt->type) {
    case CP_XYScalar:
      /* x,y channels specified - scalars, up to dlCartesianPlot->count pairs */
	nextPoint = CpDataGetLastPoint(pt->hcp,pt->trace);
	if(pt->init) nextPoint++;
	else pt->init=1;
	if(nextPoint < dlCartesianPlot->count) {
	    CpDataSetXElement(pt->hcp,pt->trace,nextPoint,
	      SAFEFLOAT(pt->recordX->value));
	    CpDataSetYElement(pt->hcp,pt->trace,nextPoint,
	      SAFEFLOAT(pt->recordY->value));
	    CpDataSetLastPoint(pt->hcp,pt->trace,nextPoint);
	} else {
	    if(dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
	      /* All done, don't add any more points */
	    } else if(dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
	      /* Shift everybody down one, add at end */
		int j;
		for(j = 1; j < dlCartesianPlot->count; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j-1,
		      CpDataGetXElement(pt->hcp,pt->trace,j));
		    CpDataSetYElement(pt->hcp,pt->trace,j-1,
		      CpDataGetYElement(pt->hcp,pt->trace,j));
		}
		CpDataSetXElement(pt->hcp,pt->trace,j-1,
		  SAFEFLOAT(pt->recordX->value));
		CpDataSetYElement(pt->hcp,pt->trace,j-1,
		  SAFEFLOAT(pt->recordY->value));
	    }
	}
#if 0     /* Use with cpCP_XYScalar.adl or delete these lines */
	printf("nextPoint=%d dlCartesianPlot->count=%d x=%g y=%g\n",
	  nextPoint,dlCartesianPlot->count,
	  pt->recordX->value,pt->recordY->value);
	printf("x1=%g x2=%g x3=%g\n",
	  CpDataGetXElement(pt->hcp,pt->trace,0),
	  CpDataGetXElement(pt->hcp,pt->trace,1),
	  CpDataGetXElement(pt->hcp,pt->trace,2));
	printf("y1=%g y2=%g y3=%g\n\n",
	  CpDataGetYElement(pt->hcp,pt->trace,0),
	  CpDataGetYElement(pt->hcp,pt->trace,1),
	  CpDataGetYElement(pt->hcp,pt->trace,2));
#endif		
	break;

    case CP_XScalar:
      /* x channel scalar, up to dlCartesianPlot->count pairs */
	nextPoint = CpDataGetLastPoint(pt->hcp,pt->trace);
	if(pt->init) nextPoint++;
	else pt->init=1;
	if(nextPoint < dlCartesianPlot->count) {
	    CpDataSetXElement(pt->hcp,pt->trace,nextPoint,
	      SAFEFLOAT(pt->recordX->value));
	    CpDataSetYElement(pt->hcp,pt->trace,nextPoint,(float)nextPoint);
	    CpDataSetLastPoint(pt->hcp,pt->trace,nextPoint);
	} else {
	    if(dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
	      /* All done, don't add any more points */
	    } else if(dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
	      /* Shift everybody down one, add at end */
		int j;
		for(j = 1; j < dlCartesianPlot->count; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j-1,
		      CpDataGetXElement(pt->hcp,pt->trace,j));
		    CpDataSetYElement(pt->hcp,pt->trace,j-1,(float)(j-1));
		}
		CpDataSetXElement(pt->hcp,pt->trace,j-1,
		  SAFEFLOAT(pt->recordX->value));
		CpDataSetYElement(pt->hcp,pt->trace,j-1,(float)(j-1));
	    }
	}
	break;


    case CP_XVector:
      /* x channel vector, ca_element_count(chid) elements */
	CpDataSetLastPoint(pt->hcp,pt->trace,MAX(pt->recordX->elementCount-1,0));
	switch(pt->recordX->dataType) {
	case DBF_STRING:
	{
	    for(j = 0; j < pt->recordX->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,0.0);
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_INT:
	{
	    short *pShort = (short *) pt->recordX->array;
	    for(j = 0; j < pt->recordX->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)pShort[j]);
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_FLOAT:
	{
	    float *pFloat = (float *) pt->recordX->array;
	    for(j = 0; j < pt->recordX->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,SAFEFLOAT(pFloat[j]));
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_ENUM:
	{
	    unsigned short *pUShort = (unsigned short *) pt->recordX->array;
	    for(j = 0; j < pt->recordX->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)pUShort[j]);
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_CHAR:
	{
	    unsigned char *pUChar = (unsigned char *) pt->recordX->array;
	    for(j = 0; j < pt->recordX->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)pUChar[j]);
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_LONG:
	{
	    dbr_long_t *pLong = (dbr_long_t *) pt->recordX->array;
	    for(j = 0; j < pt->recordX->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)pLong[j]);
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_DOUBLE:
	{
	    double *pDouble = (double *) pt->recordX->array;
	    for(j = 0; j < pt->recordX->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,SAFEFLOAT(pDouble[j]));
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	    }
	}
	break;

    case CP_YScalar:
      /* y channel scalar, up to dlCartesianPlot->count pairs */
	nextPoint = CpDataGetLastPoint(pt->hcp,pt->trace);
	if(pt->init) nextPoint++;
	else pt->init=1;
#if DEBUG_CARTESIAN_PLOT_UPDATE  && defined(XRTGRAPH)
	printf("  nextPoint=%d dlCartesianPlot->count=%d\n  XRT_HUGE_VAL=%f\n",
	  nextPoint,
	  dlCartesianPlot->count,
	  XRT_HUGE_VAL);
#endif    
	if(nextPoint < dlCartesianPlot->count) {
	    if(!nextPoint && pcp->timeScale) {
		CpSetTimeBase(pcp->dlElement->widget,
		  timeOffset + (time_t)pt->recordY->time.secPastEpoch);
		pcp->startTime = pt->recordY->time;
	    }
	    CpDataSetXElement(pt->hcp,pt->trace,nextPoint,(pcp->timeScale) ?
	      (float)(pt->recordY->time.secPastEpoch -
		pcp->startTime.secPastEpoch) :
	      (float) nextPoint);
	    CpDataSetYElement(pt->hcp,pt->trace,nextPoint,
	      (float)pt->recordY->value);
	    CpDataSetLastPoint(pt->hcp,pt->trace,nextPoint);
	} else {
	    if(dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
	      /* All done, don't add any more points */
#if DEBUG_CARTESIAN_PLOT_UPDATE
		printf("  Array full\n");
#endif    
	    } else if(dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
	      /* Shift everybody down one, add at end */
		int j;

#if DEBUG_CARTESIAN_PLOT_UPDATE
		printf("  Shifting\n");
#endif    
		if(pcp->timeScale) {
		    for(j = 1; j < dlCartesianPlot->count; j++) {
			CpDataSetXElement(pt->hcp,pt->trace,j-1,
			  CpDataGetXElement(pt->hcp,pt->trace,j));
			CpDataSetYElement(pt->hcp,pt->trace,j-1,
			  CpDataGetYElement(pt->hcp,pt->trace,j));
		    }
		    CpDataSetXElement(pt->hcp,pt->trace,j-1,
		      (float)(pt->recordY->time.secPastEpoch -
		      pcp->startTime.secPastEpoch)); 
		    CpDataSetYElement(pt->hcp,pt->trace,j-1,
		      (float)pt->recordY->value);
		} else {
		    for(j = 1; j < dlCartesianPlot->count; j++) {
			CpDataSetYElement(pt->hcp,pt->trace,j-1,
			  CpDataGetYElement(pt->hcp,pt->trace,j));
		    }
		    CpDataSetYElement(pt->hcp,pt->trace,j-1,
		      (float)pt->recordY->value);
		} 
	    }
	}
#if DEBUG_CARTESIAN_PLOT_UPDATE && defined(XRTGRAPH)
	{
	    int nextPoint1=(nextPoint < dlCartesianPlot->count)?nextPoint:
	      dlCartesianPlot->count-1;
	    static char filename[]="xrtdata.data";

	    if(nextPoint == 10) {
		XrtDataSaveToFile(pt->hcp,filename,NULL);
		printf("Data saved to %s\n",filename);
	    }
	    
	    printf("  nextPoint=%d x=       %f y=       %f\n",
	      nextPoint,
	      (pcp->timeScale) ?
	      (float)(pt->recordY->time.secPastEpoch -
		pcp->startTime.secPastEpoch) :
	      (float)nextPoint1,
	      (float)pt->recordY->value);
	    printf("  nextPoint=%d XElement=%f YElement=%f\n",
	      CpDataGetLastPoint(pt->hcp,pt->trace),
	      CpDataGetXElement(pt->hcp,pt->trace,nextPoint1),
	      CpDataGetYElement(pt->hcp,pt->trace,nextPoint1));
	}
#endif    
	break;

    case CP_YVector:
      /* plot first "count" elements of vector per dlCartesianPlot */
	CpDataSetLastPoint(pt->hcp,pt->trace,pt->recordY->elementCount-1);
	switch(pt->recordY->dataType) {
	case DBF_STRING:
	{
	    for(j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetYElement(pt->hcp,pt->trace,j,0.0);
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_INT:
	{
	    short *pShort = (short *) pt->recordY->array;
	    for(j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)pShort[j]);
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_FLOAT:
	{
	    float *pFloat = (float *) pt->recordY->array;
	    for(j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetYElement(pt->hcp,pt->trace,j,SAFEFLOAT(pFloat[j]));
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_ENUM:
	{
	    unsigned short *pUShort = (unsigned short *) pt->recordY->array;
	    for(j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)pUShort[j]);
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_CHAR:
	{
	    unsigned char *pUChar = (unsigned char *) pt->recordY->array;
	    for(j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)pUChar[j]);
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_LONG:
	{
	    dbr_long_t *pLong = (dbr_long_t *) pt->recordY->array;
	    for(j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetYElement(pt->hcp,pt->trace,j,(float)pLong[j]);
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	case DBF_DOUBLE:
	{
	    double *pDouble = (double *) pt->recordY->array;
	    for(j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetYElement(pt->hcp,pt->trace,j,SAFEFLOAT(pDouble[j]));
		CpDataSetXElement(pt->hcp,pt->trace,j,(float)j);
	    }
	    break;
	}
	}
	break;


    case CP_XVectorYScalar:
	CpDataSetLastPoint(pt->hcp,pt->trace,MAX(pt->recordX->elementCount-1,0));
	if(pr == pt->recordX) {
	  /* plot first "count" elements of vector per dlCartesianPlot */
	    switch(pt->recordX->dataType) {
	    case DBF_STRING:
	    {
		for(j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j,0.0);
		}
		break;
	    }
	    case DBF_INT:
	    {
		short *pShort = (short *) pt->recordX->array;
		for(j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j,(float)pShort[j]);
		}
		break;
	    }
	    case DBF_FLOAT:
	    {
		float *pFloat = (float *) pt->recordX->array;
		for(j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j,SAFEFLOAT(pFloat[j]));
		}
		break;
	    }
	    case DBF_ENUM:
	    {
		unsigned short *pUShort = (unsigned short *) pt->recordX->array;
		for(j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j,(float)pUShort[j]);
		}
		break;
	    }
	    case DBF_CHAR:
	    {
		unsigned char *pUChar = (unsigned char *) pt->recordX->array;
		for(j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j,(float)pUChar[j]);
		}
		break;
	    }
	    case DBF_LONG:
	    {
		dbr_long_t *pLong = (dbr_long_t *) pt->recordX->array;
		for(j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j,(float)pLong[j]);
		}
		break;
	    }
	    case DBF_DOUBLE:
	    {
		double *pDouble = (double *) pt->recordX->array;
		for(j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetXElement(pt->hcp,pt->trace,j,
		      SAFEFLOAT(pDouble[j]));
		}
		break;
	    }
	    }
	} else {
	    if(pr == pt->recordY) {
		for(j = 0; j < pt->recordX->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,
		      SAFEFLOAT(pt->recordY->value));
		}
	    }
	}
	break;

    case CP_YVectorXScalar:
	CpDataSetLastPoint(pt->hcp,pt->trace,MAX(pt->recordY->elementCount-1,0));
	if(pr == pt->recordY) {
	  /* plot first "count" elements of vector per dlCartesianPlot */
	    switch(pt->recordY->dataType) {
	    case DBF_STRING:
	    {
		for(j = 0; j < pt->recordY->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,0.0);
		}
		break;
	    }
	    case DBF_INT:
	    {
		short *pShort = (short *) pt->recordY->array;
		for(j = 0; j < pt->recordY->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,(float)pShort[j]);
		}
		break;
	    }
	    case DBF_FLOAT:
	    {
		float *pFloat = (float *) pt->recordY->array;
		for(j = 0; j < pt->recordY->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,
		      SAFEFLOAT(pFloat[j]));
		}
		break;
	    }
	    case DBF_ENUM:
	    {
		unsigned short *pUShort = (unsigned short *) pt->recordY->array;
		for(j = 0; j < pt->recordY->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,(float)pUShort[j]);
		}
		break;
	    }
	    case DBF_CHAR:
	    {
		unsigned char *pUChar = (unsigned char *) pt->recordY->array;
		for(j = 0; j < pt->recordY->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,(float)pUChar[j]);
		}
		break;
	    }
	    case DBF_LONG:
	    {
		dbr_long_t *pLong = (dbr_long_t *) pt->recordY->array;
		for(j = 0; j < pt->recordY->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,(float)pLong[j]);
		}
		break;
	    }
	    case DBF_DOUBLE:
	    {
		double *pDouble = (double *) pt->recordY->array;
		for(j = 0; j < pt->recordY->elementCount; j++) {
		    CpDataSetYElement(pt->hcp,pt->trace,j,
		      SAFEFLOAT(pDouble[j]));
		}
		break;
	    }
	    }
	} else  if(pr == pt->recordX) {
	    for(j = 0; j < pt->recordY->elementCount; j++) {
		CpDataSetXElement(pt->hcp,pt->trace,j,
		  SAFEFLOAT(pt->recordX->value));
	    }
	}
	break;

    case CP_XYVector: {
	int dox;
	int count;

	count = MIN(pt->recordX->elementCount, pt->recordY->elementCount);
	CpDataSetLastPoint(pt->hcp,pt->trace,MAX(count-1,0));

	if(pr == pt->recordX) {
	    dox = 1;
	} else  if(pr == pt->recordY) {
	    dox = 0;
	} else {
	  /* don't do anything */
	    break;
	}
	if(pcp->timeScale && pr == pt->recordX) {
	    if(pcp->startTime.nsec) {
	    } else {
		pcp->startTime.secPastEpoch = (int)pr->value;
		pcp->startTime.nsec = 1;
		CpSetTimeBase(pcp->dlElement->widget,
		  timeOffset + pcp->startTime.secPastEpoch);
#if DEBUG_TIME		
		printf("pcp->startTime = %d\n",pcp->startTime.secPastEpoch);
#endif		
	    }
	    switch(pr->dataType) {
	    case DBF_LONG:
	    {
		dbr_long_t *pLong = (dbr_long_t *) pr->array;
		dbr_long_t offset = (dbr_long_t) pcp->startTime.secPastEpoch;
		for(j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,
			(float)(pLong[j] - offset));
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,
			(float)(pLong[j] - offset));
		}
		break;
	    }
	    case DBF_DOUBLE:
	    {
		double *pDouble = (double *) pr->array;
		double offset = (double) pcp->startTime.secPastEpoch;
		for(j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,
			SAFEFLOAT(pDouble[j] - offset));
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,
			SAFEFLOAT(pDouble[j] - offset));
		}
		break;
	    }
	    default :
		break;
	    }
	} else {
	    switch(pr->dataType) {
	    case DBF_STRING:
		for(j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,0.0);
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,0.0);
		}
		break;
	    case DBF_INT:
	    {
		short *pShort = (short *) pr->array;
		for(j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,(float)pShort[j]);
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,(float)pShort[j]);
		}
		break;
	    }
	    case DBF_FLOAT:
	    {
		float *pFloat = (float *) pr->array;
		for(j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,
			SAFEFLOAT(pFloat[j]));
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,
			SAFEFLOAT(pFloat[j]));
		}
		break;
	    }
	    case DBF_ENUM:
	    {
		unsigned short *pUShort = (unsigned short *) pr->array;
		for(j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,(float)pUShort[j]);
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,(float)pUShort[j]);
		}
		break;
	    }
	    case DBF_CHAR:
	    {
		unsigned char *pUChar = (unsigned char *) pr->array;
		for(j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,(float)pUChar[j]);
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,(float)pUChar[j]);
		}
		break;
	    }
	    case DBF_LONG:
	    {
		dbr_long_t *pLong = (dbr_long_t *) pr->array;
		for(j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,(float)pLong[j]);
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,(float)pLong[j]);
		}
		break;
	    }
	    case DBF_DOUBLE:
	    {
		double *pDouble = (double *) pr->array;
		for(j = 0; j < count; j++) {
		    if(dox)
		      CpDataSetXElement(pt->hcp,pt->trace,j,
			SAFEFLOAT(pDouble[j]));
		    else
		      CpDataSetYElement(pt->hcp,pt->trace,j,
			SAFEFLOAT(pDouble[j]));
		}
		break;
	    }
	    }
	}
	break;
    }
    default:
	medmPrintf(1,"\ncartesianPlotUpdateTrace: Unknown dataType\n");
    }
}

static void cartesianPlotUpdateScreenFirstTime(XtPointer cd) {
    Record *pr = (Record *) cd;
    XYTrace *pt = (XYTrace *) pr->clientData;
    MedmCartesianPlot *pcp = pt->cartesianPlot;
    Widget widget = pcp->dlElement->widget;
    int i;
    Boolean clearDataSet1 = True;
    Boolean clearDataSet2 = True;

#if DEBUG_CARTESIAN_PLOT_UPDATE
    printf("cartesianPlotUpdateScreenFirstTime: nTraces=%d\n",
      pcp->nTraces);
#endif    
  /* Return until all channels get their graphical information */
    for(i = 0; i < pcp->nTraces; i++) {
	XYTrace *t = &(pcp->xyTrace[i]);
#if DEBUG_CARTESIAN_PLOT_UPDATE
	printf("  recordX=%x recordY=%x hcp=%x\n",
	  t->recordX,t->recordY,t->hcp);
	if(t->recordX) {
	    printf("  x: array=%x precision=%d\n",
	      t->recordX->array,t->recordX->precision);
	}
	if(t->recordY) {
	    printf("  y: array=%x precision=%d\n",
	      t->recordY->array,t->recordY->precision);
	}
#endif    
	if((t->recordX) || (t->recordY)) {
	    if(t->hcp == NULL) return;
	    if((t->recordX) &&
	      ((!t->recordX->array) || (t->recordX->precision < 0))) return;
	    if((t->recordY) &&
	      ((!t->recordY->array) || (t->recordY->precision < 0))) return;
	}
    }
    if(pcp->triggerCh.recordX) {
	if(pcp->triggerCh.recordX->precision < 0) return;
    }
    if(pcp->eraseCh.recordX) {
	if(pcp->eraseCh.recordX->precision < 0) return;
    }

  /* Draw all traces once */
    for(i = 0; i < pcp->nTraces; i++) {
	XYTrace *t = &(pcp->xyTrace[i]);
	if(t->recordX && t->recordY) {
	    cartesianPlotUpdateTrace((XtPointer)t->recordX);
	    cartesianPlotUpdateTrace((XtPointer)t->recordY);
	} else if(t->recordX) {
	    cartesianPlotUpdateTrace((XtPointer)t->recordX);
	} else if(t->recordY) {
	    cartesianPlotUpdateTrace((XtPointer)t->recordY);
	} else {
	    continue;
	}
	if(t->hcp == pcp->hcp1) {
	    CpSetData(widget, CP_Y, t->hcp );
	} else if(t->hcp == pcp->hcp2) {
	    CpSetData(widget, CP_Y2, t->hcp );
	}
    }
  /* Erase the plot */
    if(pcp->eraseCh.recordX) {
	Record *pr = pcp->eraseCh.recordX;
	if(((pr->value == 0.0) && (pcp->eraseMode == ERASE_IF_ZERO))
	  || ((pr->value != 0.0) && (pcp->eraseMode == ERASE_IF_NOT_ZERO))) {
	    for(i = 0; i < pcp->nTraces; i++) {
		XYTrace *t = &(pcp->xyTrace[i]);
		if((t->recordX) || (t->recordY)) {
		    int n = CpDataGetLastPoint(t->hcp,t->trace);
		    if(n > 0) {
			if((t->hcp == pcp->hcp1) && (clearDataSet1)) {
			    CpEraseData(widget, CP_Y, t->hcp);
			    clearDataSet1 = False;
			} else if((t->hcp == pcp->hcp2) && (clearDataSet2)) {
			    CpEraseData(widget, CP_Y2, t->hcp);
			    clearDataSet2 = False;
			}
			t->init = 0;
			CpDataSetLastPoint(t->hcp,t->trace,0);
		    }
		} 
	    }
	}
    }

  /* Switch to the regular update routine from now on */
    for(i = 0; i < pcp->nTraces; i++) {
	XYTrace *t = &(pcp->xyTrace[i]);
	if(t->recordX) {
	    medmRecordAddUpdateValueCb(t->recordX,
	      cartesianPlotUpdateValueCb);
	}
	if(t->recordY) {
	    medmRecordAddUpdateValueCb(t->recordY,
	      cartesianPlotUpdateValueCb);
	}
    }
    if(pcp->eraseCh.recordX) {
	medmRecordAddUpdateValueCb(pcp->eraseCh.recordX,
	  cartesianPlotUpdateValueCb);
    }
    if(pcp->triggerCh.recordX) {
	medmRecordAddUpdateValueCb(pcp->triggerCh.recordX,
	  cartesianPlotUpdateValueCb);
    }
    CpUpdateWidget(widget, CP_FULL);
    updateTaskMarkUpdate(pcp->updateTask);
}


static void cartesianPlotUpdateValueCb(XtPointer cd) {
    Record *pr = (Record *) cd;
    XYTrace *pt = (XYTrace *) pr->clientData;
    MedmCartesianPlot *pcp = pt->cartesianPlot;
    Widget widget = pcp->dlElement->widget;
    int i;

#if DEBUG_CARTESIAN_PLOT_UPDATE
    printf("cartesianPlotUpdateValueCb:\n");
#endif    
  /* If this is an erase channel, erase screen */
    if(pr == pcp->eraseCh.recordX) {
	Boolean clearDataSet1 = True;
	Boolean clearDataSet2 = True;

      /* not the right value, return */
	if(((pr->value == 0) && (pcp->eraseMode == ERASE_IF_NOT_ZERO))
	  ||((pr->value != 0) && (pcp->eraseMode == ERASE_IF_ZERO))) {
	    return;
	}

      /* erase */
	for(i = 0; i < pcp->nTraces; i++) {
	    XYTrace *t = &(pcp->xyTrace[i]);
	    if((t->recordX) || (t->recordY)) {
		int n = CpDataGetLastPoint(t->hcp,t->trace);
		if(n > 0) {
		    if((t->hcp == pcp->hcp1) && (clearDataSet1)) {
			CpEraseData(widget, CP_Y, t->hcp);
			clearDataSet1 = False;
			pcp->dirty1 = False;
		    } else if((t->hcp == pcp->hcp2) && (clearDataSet2)) {
			CpEraseData(widget, CP_Y2, t->hcp);
			clearDataSet2 = False;
			pcp->dirty2 = False;
		    }
		    t->init = 0;
		    CpDataSetLastPoint(t->hcp,t->trace,0);
		}
	    }
	}
	CpUpdateWidget(widget, CP_FULL);
	updateTaskMarkUpdate(pcp->updateTask);
	return;
    }

  /* If there is a trigger channel, but this is not the one, return
       and do not update */
    if(pcp->triggerCh.recordX) {
	if(pr != pcp->triggerCh.recordX)
	  return;
    }

  /* Check if there is a trigger channel */
    if(pcp->triggerCh.recordX) {
  /* There is a trigger channel, update appropriate plots */
	for(i = 0; i < pcp->nTraces; i++) {
	    XYTrace *t = &(pcp->xyTrace[i]);
	    if((t->recordX == NULL) && (t->recordY == NULL)) continue;
	    if((t->recordX) && (t->hcp)) {
		cartesianPlotUpdateTrace((XtPointer)t->recordX);
		if(t->type == CP_XYVector)
		  cartesianPlotUpdateTrace((XtPointer)t->recordY);
		if(t->hcp == pcp->hcp1)
		  pcp->dirty1 = True;
		else if(t->hcp == pcp->hcp2)
		  pcp->dirty2 = True;
	    } else if((t->recordY) && (t->hcp)) {
		cartesianPlotUpdateTrace((XtPointer)t->recordY);
		if(t->hcp == pcp->hcp1)
		  pcp->dirty1 = True;
		else if(t->hcp == pcp->hcp2)
		  pcp->dirty2 = True;
	    }
	}
    } else {
      /* No trigger channel, proceed as normal */
	cartesianPlotUpdateTrace((XtPointer)pr);
	if(pt->hcp == pcp->hcp1) {
	    pcp->dirty1 = True;
	} else if(pt->hcp == pcp->hcp2) {
	    pcp->dirty2 = True;
	} else {
	    medmPrintf(1,"\ncartesianPlotUpdateValueCb: "
	      "Illegal cpDataSet specified\n");
	}
    }
    updateTaskMarkUpdate(pcp->updateTask);
}

static void cartesianPlotDestroyCb(XtPointer cd) {
    MedmCartesianPlot *pcp = (MedmCartesianPlot *) cd;
    
    if(executeTimeCartesianPlotWidget &&
      executeTimeCartesianPlotWidget == pcp->dlElement->widget) {
	executeTimeCartesianPlotWidget = NULL;
	XtSetSensitive(cartesianPlotAxisS,False);
    }
    if(pcp) {
	int i;
	for(i = 0; i < pcp->nTraces; i++) {
	    Record *pr;
	    if(pr = pcp->xyTrace[i].recordX) {
		medmDestroyRecord(pr);
	    }
	    if(pr = pcp->xyTrace[i].recordY) {
		medmDestroyRecord(pr);
	    }
	}
	if(pcp->triggerCh.recordX)
	  medmDestroyRecord(pcp->triggerCh.recordX);
	if(pcp->eraseCh.recordX)
	  medmDestroyRecord(pcp->eraseCh.recordX);
	if(pcp->hcp1) CpDataDestroy(pcp->hcp1);
	if(pcp->hcp2) CpDataDestroy(pcp->hcp2);
	if(pcp->dlElement) pcp->dlElement->data = NULL;
	free((char *)pcp);
    }
    return;
}

static void cartesianPlotDraw(XtPointer cd) {
    MedmCartesianPlot *pcp = (MedmCartesianPlot *) cd;
    DlElement *dlElement = pcp->dlElement;
    Widget widget = dlElement->widget;
    int i;
    Boolean connected = True;
    Boolean readAccess = True;

  /* Check if hidden */
    if(dlElement->hidden) {
	if(widget && XtIsManaged(widget)) {
	    XtUnmanageChild(widget);
	}
	return;
    }
    
  /* Check for connection */
    for(i = 0; i < pcp->nTraces; i++) {
	Record *pr;
	if(pr = pcp->xyTrace[i].recordX) {
	    if(!pr->connected) {
		connected = False;
		break;
	    } else if(!pr->readAccess)
	      readAccess = False;
	}
	if(pr = pcp->xyTrace[i].recordY) {
	    if(!pr->connected) {
		connected = False;
		break;
	    } else if(!pr->readAccess)
	      readAccess = False;
	}
    }
    if(pcp->triggerCh.recordX) {
	Record *pr = pcp->triggerCh.recordX;
	if(!pr->connected) {
	    connected = False;
	} else if(!pr->readAccess) {
	    readAccess = False;
	}
    }
    if(pcp->eraseCh.recordX) {
	Record *pr = pcp->eraseCh.recordX;
	if(!pr->connected) {
	    connected = False;
	} else if(!pr->readAccess) {
	    readAccess = False;
	}
    }
    if(connected) {
	if(readAccess) {
	    if(widget) {
		if(pcp->dirty1) {
		    pcp->dirty1 = False;
		    CpSetData(widget, CP_Y, pcp->hcp1);
		}
		if(pcp->dirty2) {
		    pcp->dirty2 = False;
		    CpSetData(widget, CP_Y2, pcp->hcp2);
		}
		addCommonHandlers(widget, pcp->updateTask->displayInfo);
		XtManageChild(widget);
	    }
	} else {
	    if(widget) {
		XtUnmanageChild(widget);
	    }
	    draw3DPane(pcp->updateTask,
	      pcp->updateTask->displayInfo->colormap[
		pcp->dlElement->structure.cartesianPlot->plotcom.bclr]);
	    draw3DQuestionMark(pcp->updateTask);
	}
    } else {
	if(widget) {
	    XtUnmanageChild(widget);
	}
	drawWhiteRectangle(pcp->updateTask);
    }
  /* KE: Used to use CP_FAST here.  SciPlot autoscales to larger, but
     not to smaller then. */
    CpUpdateWidget(widget, CP_FULL);
}

static void cartesianPlotGetRecord(XtPointer cd, Record **record, int *count)
{
    MedmCartesianPlot *pcp = (MedmCartesianPlot *)cd;
    int i, j;

    j = 0;
    for(i = 0; i < pcp->nTraces; i++) {
	XYTrace *pt = &(pcp->xyTrace[i]);
	record[j++] = pt->recordX;
	record[j++] = pt->recordY;
    }
    if(pcp->triggerCh.recordX) {
	record[j++] = pcp->triggerCh.recordX;
    } else {     /* Count these even if NULL */
	record[j++] = NULL;
    }
    if(pcp->eraseCh.recordX) {
	record[j++] = pcp->eraseCh.recordX;
    } else {     /* Count these even if NULL */
	record[j++] = NULL;
    }

  /* 200 means two columns */
    *count = j + 200;
}

DlElement *createDlCartesianPlot(DlElement *p)
{
    DlCartesianPlot *dlCartesianPlot;
    DlElement *dlElement;
    int traceNumber;

    dlCartesianPlot = (DlCartesianPlot *)malloc(sizeof(DlCartesianPlot));
    if(!dlCartesianPlot) return 0;
    if(p) {
	*dlCartesianPlot = *p->structure.cartesianPlot;
    } else {
	objectAttributeInit(&(dlCartesianPlot->object));
	plotcomAttributeInit(&(dlCartesianPlot->plotcom));
	dlCartesianPlot->count = 1;
	dlCartesianPlot->style = POINT_PLOT;
	dlCartesianPlot->erase_oldest = ERASE_OLDEST_OFF;
	for(traceNumber = 0; traceNumber < MAX_TRACES; traceNumber++)
	  traceAttributeInit(&(dlCartesianPlot->trace[traceNumber]));
	plotAxisDefinitionInit(&(dlCartesianPlot->axis[X_AXIS_ELEMENT]));
	plotAxisDefinitionInit(&(dlCartesianPlot->axis[Y1_AXIS_ELEMENT]));
	plotAxisDefinitionInit(&(dlCartesianPlot->axis[Y2_AXIS_ELEMENT]));
	dlCartesianPlot->trigger[0] = '\0';
	dlCartesianPlot->erase[0] = '\0';
	dlCartesianPlot->eraseMode = ERASE_IF_NOT_ZERO;
    }

    if(!(dlElement = createDlElement(DL_CartesianPlot,
      (XtPointer)      dlCartesianPlot,
      &cartesianPlotDlDispatchTable))) {
	free(dlCartesianPlot);
    }

    return(dlElement);
}

DlElement *parseCartesianPlot(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlCartesianPlot *dlCartesianPlot;
    DlElement *dlElement = createDlCartesianPlot(NULL);
    int traceNumber;

    if(!dlElement) return 0;
    dlCartesianPlot = dlElement->structure.cartesianPlot;
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlCartesianPlot->object));
	    else if(!strcmp(token,"plotcom"))
	      parsePlotcom(displayInfo,&(dlCartesianPlot->plotcom));
	    else if(!strcmp(token,"count")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlCartesianPlot->count= atoi(token);
	    } else if(!strcmp(token,"style")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"point plot")) 
		  dlCartesianPlot->style = POINT_PLOT;
		else if(!strcmp(token,"point")) 
		  dlCartesianPlot->style = POINT_PLOT;
		else if(!strcmp(token,"line plot")) 
		  dlCartesianPlot->style = LINE_PLOT;
		else if(!strcmp(token,"line")) 
		  dlCartesianPlot->style = LINE_PLOT;
		else if(!strcmp(token,"fill under")) 
		  dlCartesianPlot->style = FILL_UNDER_PLOT;
		else if(!strcmp(token,"fill-under")) 
		  dlCartesianPlot->style = FILL_UNDER_PLOT;
	    } else if(!strcmp(token,"erase_oldest")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"on")) 
		  dlCartesianPlot->erase_oldest = ERASE_OLDEST_ON;
		else if(!strcmp(token,"off")) 
		  dlCartesianPlot->erase_oldest = ERASE_OLDEST_OFF;
		else if(!strcmp(token,"plot last n pts")) 
		  dlCartesianPlot->erase_oldest = ERASE_OLDEST_ON;
		else if(!strcmp(token,"plot n pts & stop")) 
		  dlCartesianPlot->erase_oldest = ERASE_OLDEST_OFF;
	    } else if(!strncmp(token,"trace",5)) {
		traceNumber = MIN(token[6] - '0', MAX_TRACES - 1);
		parseTrace(displayInfo,
		  &(dlCartesianPlot->trace[traceNumber]));
	    } else if(!strcmp(token,"x_axis")) {
		parsePlotAxisDefinition(displayInfo,
		  &(dlCartesianPlot->axis[X_AXIS_ELEMENT]));
	    } else if(!strcmp(token,"y1_axis")) {
		parsePlotAxisDefinition(displayInfo,
		  &(dlCartesianPlot->axis[Y1_AXIS_ELEMENT]));
	    } else if(!strcmp(token,"y2_axis")) {
		parsePlotAxisDefinition(displayInfo,
		  &(dlCartesianPlot->axis[Y2_AXIS_ELEMENT]));
	    } else if(!strcmp(token,"trigger")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlCartesianPlot->trigger,token);
	    } else if(!strcmp(token,"erase")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlCartesianPlot->erase,token);
	    } else if(!strcmp(token,"eraseMode")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"if not zero"))
		  dlCartesianPlot->eraseMode = ERASE_IF_NOT_ZERO;
		else if(!strcmp(token,"if zero"))
		  dlCartesianPlot->eraseMode = ERASE_IF_ZERO;
	    }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
	}
    } while( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );

    return dlElement;

}

void writeDlCartesianPlot(FILE *stream, DlElement *dlElement, int level)
{
    int i;
    char indent[16];
    DlCartesianPlot *dlCartesianPlot = dlElement->structure.cartesianPlot;

    for(i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%s\"cartesian plot\" {",indent);
    writeDlObject(stream,&(dlCartesianPlot->object),level+1);
    writeDlPlotcom(stream,&(dlCartesianPlot->plotcom),level+1);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
	if(dlCartesianPlot->style != POINT_PLOT)
	  fprintf(stream,"\n%s\tstyle=\"%s\"",indent,
	    stringValueTable[dlCartesianPlot->style]);
	if(dlCartesianPlot->erase_oldest != ERASE_OLDEST_OFF)
	  fprintf(stream,"\n%s\terase_oldest=\"%s\"",indent,
	    stringValueTable[dlCartesianPlot->erase_oldest]);
	if(dlCartesianPlot->count != 1)
	  fprintf(stream,"\n%s\tcount=\"%d\"",indent,dlCartesianPlot->count);
	for(i = 0; i < MAX_TRACES; i++) {
	    writeDlTrace(stream,&(dlCartesianPlot->trace[i]),i,level+1);
	}
	writeDlPlotAxisDefinition(stream,
	  &(dlCartesianPlot->axis[X_AXIS_ELEMENT]),X_AXIS_ELEMENT,level+1);
	writeDlPlotAxisDefinition(stream,
	  &(dlCartesianPlot->axis[Y1_AXIS_ELEMENT]),Y1_AXIS_ELEMENT,level+1);
	writeDlPlotAxisDefinition(stream,
	  &(dlCartesianPlot->axis[Y2_AXIS_ELEMENT]),Y2_AXIS_ELEMENT,level+1);
	if(dlCartesianPlot->trigger[0] != '\0')
	  fprintf(stream,"\n%s\ttrigger=\"%s\"",indent,dlCartesianPlot->trigger);
	if(dlCartesianPlot->erase[0] != '\0')
	  fprintf(stream,"\n%s\terase=\"%s\"",indent,dlCartesianPlot->erase);
	if(dlCartesianPlot->eraseMode != ERASE_IF_NOT_ZERO)
	  fprintf(stream,"\n%s\teraseMode=\"%s\"",indent,
	    stringValueTable[dlCartesianPlot->eraseMode]);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%s\tstyle=\"%s\"",indent,
	  stringValueTable[dlCartesianPlot->style]);
	fprintf(stream,"\n%s\terase_oldest=\"%s\"",indent,
	  stringValueTable[dlCartesianPlot->erase_oldest]);
	fprintf(stream,"\n%s\tcount=\"%d\"",indent,dlCartesianPlot->count);
	for(i = 0; i < MAX_TRACES; i++) {
	    writeDlTrace(stream,&(dlCartesianPlot->trace[i]),i,level+1);
	}
	writeDlPlotAxisDefinition(stream,
	  &(dlCartesianPlot->axis[X_AXIS_ELEMENT]),X_AXIS_ELEMENT,level+1);
	writeDlPlotAxisDefinition(stream,
	  &(dlCartesianPlot->axis[Y1_AXIS_ELEMENT]),Y1_AXIS_ELEMENT,level+1);
	writeDlPlotAxisDefinition(stream,
	  &(dlCartesianPlot->axis[Y2_AXIS_ELEMENT]),Y2_AXIS_ELEMENT,level+1);
	fprintf(stream,"\n%s\ttrigger=\"%s\"",indent,dlCartesianPlot->trigger);
	fprintf(stream,"\n%s\terase=\"%s\"",indent,dlCartesianPlot->erase);
	fprintf(stream,"\n%s\teraseMode=\"%s\"",indent,
	  stringValueTable[dlCartesianPlot->eraseMode]);
    }
#endif
    fprintf(stream,"\n%s}",indent);
}

static void cartesianPlotInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlCartesianPlot *dlCartesianPlot = p->structure.cartesianPlot;
    medmGetValues(pRCB,
      CLR_RC,        &(dlCartesianPlot->plotcom.clr),
      BCLR_RC,       &(dlCartesianPlot->plotcom.bclr),
      -1);
}

static void cartesianPlotGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlCartesianPlot *dlCartesianPlot = p->structure.cartesianPlot;
    medmGetValues(pRCB,
      X_RC,          &(dlCartesianPlot->object.x),
      Y_RC,          &(dlCartesianPlot->object.y),
      WIDTH_RC,      &(dlCartesianPlot->object.width),
      HEIGHT_RC,     &(dlCartesianPlot->object.height),
      TITLE_RC,      &(dlCartesianPlot->plotcom.title),
      XLABEL_RC,     &(dlCartesianPlot->plotcom.xlabel),
      YLABEL_RC,     &(dlCartesianPlot->plotcom.ylabel),
      CLR_RC,        &(dlCartesianPlot->plotcom.clr),
      BCLR_RC,       &(dlCartesianPlot->plotcom.bclr),
      COUNT_RC,      &(dlCartesianPlot->count),
      CSTYLE_RC,     &(dlCartesianPlot->style),
      ERASE_OLDEST_RC,&(dlCartesianPlot->erase_oldest),
      CPDATA_RC,     &(dlCartesianPlot->trace),
      CPAXIS_RC,     &(dlCartesianPlot->axis),
      TRIGGER_RC,    &(dlCartesianPlot->trigger),
      ERASE_RC,      &(dlCartesianPlot->erase),
      ERASE_MODE_RC, &(dlCartesianPlot->eraseMode),
      -1);
}

static void cartesianPlotSetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlCartesianPlot *dlCartesianPlot = p->structure.cartesianPlot;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlCartesianPlot->plotcom.bclr),
      -1);
}

static void cartesianPlotSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlCartesianPlot *dlCartesianPlot = p->structure.cartesianPlot;
    medmGetValues(pRCB,
      CLR_RC,        &(dlCartesianPlot->plotcom.clr),
      -1);
}

/*
 * Set Cartesian Plot Axis attributes
 * (complex - has to handle both EDIT and EXECUTE time interactions)
 */
static void cpAxisOptionMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{     
    DisplayInfo *cdi=currentDisplayInfo;
    int buttonId = (int)cd;
    int k, n, rcType, iPrec;
    char string[24];
    char *stringMinValue, *stringMaxValue;
    XcVType minF, maxF, tickF;
    XtPointer userData;
    MedmCartesianPlot *pcp = NULL;
    DlCartesianPlot *dlCartesianPlot = NULL;

    UNREFERENCED(cbs);

  /* Get current cartesian plot */
    if(globalDisplayListTraversalMode == DL_EXECUTE) {
	if(executeTimeCartesianPlotWidget) {
	    XtVaGetValues(executeTimeCartesianPlotWidget,
	      XmNuserData, &userData, NULL);
	    if(pcp = (MedmCartesianPlot *) userData)
	      dlCartesianPlot = (DlCartesianPlot *) 
		pcp->dlElement->structure.cartesianPlot;
	} else {
	    medmPostMsg(1,"cpAxisOptionMenuSimpleCallback: "
	      "Element is no longer valid\n");
	    XtPopdown(cartesianPlotAxisS);
	    return;
	}
    }

  /* rcType (and therefore which option menu...) is stored in userData */
    XtVaGetValues(XtParent(w),XmNuserData,&rcType,NULL);
    n = 0;
    switch (rcType) {
    case CP_X_AXIS_STYLE: 
    case CP_Y_AXIS_STYLE: 
    case CP_Y2_AXIS_STYLE:
    {
	CartesianPlotAxisStyle style 
	  = (CartesianPlotAxisStyle)(FIRST_CARTESIAN_PLOT_AXIS_STYLE+buttonId);
	
	globalResourceBundle.axis[rcType - CP_X_AXIS_STYLE].axisStyle = style;
	switch (rcType) {
	case CP_X_AXIS_STYLE:
	    switch (style) {
	    case LINEAR_AXIS:
		if(globalDisplayListTraversalMode == DL_EXECUTE) {
		    CpSetAxisLinear(executeTimeCartesianPlotWidget, CP_X);
		}
		XtSetSensitive(axisTimeFormat,False);
		if(pcp) pcp->timeScale = False;
		break;
	    case LOG10_AXIS:
		if(globalDisplayListTraversalMode == DL_EXECUTE) {
		    CpSetAxisLog(executeTimeCartesianPlotWidget, CP_X);
		}
		XtSetSensitive(axisTimeFormat,False);
		if(pcp) pcp->timeScale = False;
		break;
	    case TIME_AXIS:
		if(globalDisplayListTraversalMode == DL_EXECUTE) {
		    CpSetAxisTime(executeTimeCartesianPlotWidget, CP_X,
		      time900101, cpTimeFormatString[
			(int)globalResourceBundle.axis[0].timeFormat -
			FIRST_CP_TIME_FORMAT]);
		}
		XtSetSensitive(axisTimeFormat,True);
		if(pcp) pcp->timeScale = True;
		break;
	    }
	    break;
	case CP_Y_AXIS_STYLE:
	    if(globalDisplayListTraversalMode == DL_EXECUTE) {
		CpSetAxisLinear(executeTimeCartesianPlotWidget, CP_Y);
		(style == LOG10_AXIS) ?
		  CpSetAxisLog(executeTimeCartesianPlotWidget, CP_Y) :
		  CpSetAxisLinear(executeTimeCartesianPlotWidget, CP_Y);
	    }
	    break;
	case CP_Y2_AXIS_STYLE:
	    if(globalDisplayListTraversalMode == DL_EXECUTE) {
		CpSetAxisLinear(executeTimeCartesianPlotWidget, CP_Y2);
		(style == LOG10_AXIS) ?
		  CpSetAxisLog(executeTimeCartesianPlotWidget, CP_Y2) :
		  CpSetAxisLinear(executeTimeCartesianPlotWidget, CP_Y2);
	    }
	    break;
	}
	break;
    }
    case CP_X_RANGE_STYLE:
    case CP_Y_RANGE_STYLE:
    case CP_Y2_RANGE_STYLE:
	globalResourceBundle.axis[rcType-CP_X_RANGE_STYLE].rangeStyle
	  = (CartesianPlotRangeStyle)(FIRST_CARTESIAN_PLOT_RANGE_STYLE
	    + buttonId);
	switch(globalResourceBundle.axis[rcType%3].rangeStyle) {
	case USER_SPECIFIED_RANGE:
	    XtSetSensitive(axisRangeMinRC[rcType%3],True);
	    XtSetSensitive(axisRangeMaxRC[rcType%3],True);
	    if(globalDisplayListTraversalMode == DL_EXECUTE) {
	      /* Determine max and min values */
		stringMinValue = XmTextFieldGetString(axisRangeMin[rcType%3]);
		stringMaxValue = XmTextFieldGetString(axisRangeMax[rcType%3]);
		if(*stringMinValue && *stringMaxValue) {
		  /* Values have been entered */
		    minF.fval = (float)atof(stringMinValue);
		    maxF.fval = (float)atof(stringMaxValue);
		} else if(executeTimeCartesianPlotWidget) {
		  /* Get values from the widget */
		    CpGetAxisMaxMin(executeTimeCartesianPlotWidget,
		      CP_X + rcType%3, &maxF, &minF);
		} else {
		  /* Get values from the resource palette */
		    minF.fval = globalResourceBundle.axis[rcType%3].minRange;
		    maxF.fval = globalResourceBundle.axis[rcType%3].maxRange;
		}
		XtFree(stringMinValue);
		XtFree(stringMaxValue);

	      /* Set values in the text fields */
		sprintf(string,"%f",minF.fval);
		XmTextFieldSetString(axisRangeMin[rcType%3],string);
		sprintf(string,"%f",maxF.fval);
		XmTextFieldSetString(axisRangeMax[rcType%3],string);

	      /* Set the values in the widget */
		tickF.fval = (float)((maxF.fval - minF.fval)/4.0);
		sprintf(string,"%f",tickF.fval);
		k = strlen(string)-1;
		while(string[k] == '0') k--;	/* strip off trailing zeroes */
		iPrec = k;
		while(string[k] != '.' && k >= 0) k--;
		iPrec = iPrec - k;
		switch(rcType%3) {
		case X_AXIS_ELEMENT:
		    CpSetAxisAll(executeTimeCartesianPlotWidget, CP_X,
		      maxF, minF, tickF,tickF, iPrec);  
		    break;
		case Y1_AXIS_ELEMENT:
		    CpSetAxisAll(executeTimeCartesianPlotWidget, CP_Y,
		      maxF, minF, tickF,tickF, iPrec);  
		    break;
		case Y2_AXIS_ELEMENT:
		    CpSetAxisAll(executeTimeCartesianPlotWidget, CP_Y2,
		      maxF, minF, tickF,tickF, iPrec);  
		    break;
		}
	    }
	    if(pcp) pcp->axisRange[rcType%3].isCurrentlyFromChannel = False;
	    break;
	case CHANNEL_RANGE: 
	    XtSetSensitive(axisRangeMinRC[rcType%3],False);
	    XtSetSensitive(axisRangeMaxRC[rcType%3],False);
	    if(globalDisplayListTraversalMode == DL_EXECUTE) {
		if(pcp) {
		  /* get channel-based range specifiers - NB: these are
		   *   different than the display element version of these
		   *   which are the user-specified values
		   */
		    minF.fval = pcp->axisRange[rcType%3].axisMin;
		    maxF.fval = pcp->axisRange[rcType%3].axisMax;
		}
		switch(rcType%3) {
		case X_AXIS_ELEMENT:
		    CpSetAxisChannel(executeTimeCartesianPlotWidget, CP_X,
		      maxF, minF);  
		    break;
		case Y1_AXIS_ELEMENT:
		    CpSetAxisChannel(executeTimeCartesianPlotWidget, CP_Y,
		      maxF, minF);  
		    break;
		case Y2_AXIS_ELEMENT:
		    CpSetAxisChannel(executeTimeCartesianPlotWidget, CP_Y2,
		      maxF, minF);  
		    break;
		}
	    }
	    if(pcp) pcp->axisRange[rcType%3].isCurrentlyFromChannel = True;
	    break;
	case AUTO_SCALE_RANGE:
	    XtSetSensitive(axisRangeMinRC[rcType%3],False);
	    XtSetSensitive(axisRangeMaxRC[rcType%3],False);
	    if(globalDisplayListTraversalMode == DL_EXECUTE) {
		switch(rcType%3) {
		case X_AXIS_ELEMENT:
		    CpSetAxisAuto(executeTimeCartesianPlotWidget, CP_X);
		    break;
		case Y1_AXIS_ELEMENT:
		    CpSetAxisAuto(executeTimeCartesianPlotWidget, CP_Y);
		    break;
		case Y2_AXIS_ELEMENT:
		    CpSetAxisAuto(executeTimeCartesianPlotWidget, CP_Y2);
		    break;
		}
	    }
	    if(pcp) pcp->axisRange[rcType%3].isCurrentlyFromChannel = False;
	    break;
	default:
	    break;
	}
	break;
    case CP_X_TIME_FORMAT:
	globalResourceBundle.axis[0].timeFormat =
	  (CartesianPlotTimeFormat_t)(FIRST_CP_TIME_FORMAT + buttonId);
	if(globalDisplayListTraversalMode == DL_EXECUTE) {
	    CpSetTimeFormat(executeTimeCartesianPlotWidget,
	      cpTimeFormatString[(int)globalResourceBundle.axis[0].timeFormat -
		FIRST_CP_TIME_FORMAT]);
	}
	break;
    default:
	medmPrintf(1,"\ncpAxisOptionMenuSimpleCallback: "
	  "Unknown rcType = %d\n",rcType/3);
	break;
    }
    
  /* Update from globalResourceBundle for EDIT mode
   *   EXECUTE mode already done */
    if(globalDisplayListTraversalMode == DL_EDIT) {
	if(cdi) {
	    DlElement *dlElement = FirstDlElement(
	      cdi->selectedDlElementList);
	    unhighlightSelectedElements();
	    while(dlElement) {
		updateElementFromGlobalResourceBundle(
		  dlElement->structure.element);
		dlElement = dlElement->next;
	    }
	    dmTraverseNonWidgetsInDisplayList(cdi);
	    highlightSelectedElements();
	}
    }
}

void cpAxisTextFieldActivateCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *cdi=currentDisplayInfo;
    int rcType = (int)cd;
    char *stringValue, string[24];
    int k, n, iPrec;
    XcVType valF, minF, maxF, tickF;
    int axis, isMax;

    UNREFERENCED(cbs);

#if DEBUG_XRT
    print("\ncpAxisTextFieldActivateCallback: Entered\n");
#endif    
    stringValue = XmTextFieldGetString(w);

  /* Determine axis max or min
   *   Note: For the strcpy() calls, note that the textField has a maxLength 
	  resource set such that the strcpy always succeeds) */
    n = 0;
    switch(rcType) {
    case CP_X_RANGE_MIN:
    case CP_Y_RANGE_MIN:
    case CP_Y2_RANGE_MIN:
	globalResourceBundle.axis[rcType%3].minRange= (float)atof(stringValue);
	if(globalDisplayListTraversalMode == DL_EXECUTE) {
	    valF.fval = globalResourceBundle.axis[rcType%3].minRange;
	    switch(rcType%3) {
	    case X_AXIS_ELEMENT: axis = CP_X; isMax = 0; break;
	    case Y1_AXIS_ELEMENT: axis = CP_Y; isMax = 0; break;
	    case Y2_AXIS_ELEMENT: axis = CP_Y2; isMax = 0; break;
	    default:
		medmPrintf(1,"\ncpAxisTextFieldActivateCallback (MIN): "
		  "Unknown rcType%%3 = %d\n",rcType%3);
		return;
	    }
#if DEBUG_XRT
	    print("\ncpAxisTextFieldActivateCallback [MIN]: "
	      "valF.fval =%g valF.lval=%ld Converted: %d\n",
	      valF.fval,valF.lval,XrtFloatToArgVal(valF.fval));
#endif    
	}
	break;
    case CP_X_RANGE_MAX:
    case CP_Y_RANGE_MAX:
    case CP_Y2_RANGE_MAX:
	globalResourceBundle.axis[rcType%3].maxRange= (float)atof(stringValue);
	if(globalDisplayListTraversalMode == DL_EXECUTE) {
	    valF.fval = globalResourceBundle.axis[rcType%3].maxRange;
	    switch(rcType%3) {
	    case X_AXIS_ELEMENT: axis = CP_X; isMax = 1; break;
	    case Y1_AXIS_ELEMENT: axis = CP_Y; isMax = 1; break;
	    case Y2_AXIS_ELEMENT: axis = CP_Y2; isMax = 1; break;
	    default:
		medmPrintf(1,"\ncpAxisTextFieldActivateCallback (MAX): "
		  "Unknown rcType%%3 = %d\n",rcType%3);
		return;
	    }
#if DEBUG_XRT
	    print("\ncpAxisTextFieldActivateCallback [MAX]: "
	      "valF.fval =%g valF.lval=%ld Converted: %d\n",
	      valF.fval,valF.lval,XrtFloatToArgVal(valF.fval));
#endif    
	}
	break;
    default:
	medmPrintf(1,"\ncpAxisTextFieldActivateCallback: "
	  "Unknown rcType = %d\n",rcType);
	return;
    }
    XtFree(stringValue);

  /* Recalculate ticks */
    if(globalDisplayListTraversalMode == DL_EXECUTE) {
	minF.fval = globalResourceBundle.axis[rcType%3].minRange;
	maxF.fval = globalResourceBundle.axis[rcType%3].maxRange;
	tickF.fval = (float)((maxF.fval - minF.fval)/4.0);
#if DEBUG_XRT
	print("cpAxisTextFieldActivateCallback: "
	  "minF.fval =%g minF.lval=%ld Converted: %d\n",
	  minF.fval,minF.lval,XrtFloatToArgVal(minF.fval));
	print("cpAxisTextFieldActivateCallback: "
	  "maxF.fval =%g maxF.lval=%ld Converted: %d\n",
	  maxF.fval,maxF.lval,XrtFloatToArgVal(maxF.fval));
	print("cpAxisTextFieldActivateCallback: "
	  "tickF.fval =%g tickF.lval=%ld Converted: %d\n",
	  tickF.fval,tickF.lval,XrtFloatToArgVal(tickF.fval));
#endif    
	sprintf(string,"%f",tickF.fval);
	k = strlen(string)-1;
	while(string[k] == '0') k--;	/* strip off trailing zeroes */
	iPrec = k;
	while(string[k] != '.' && k >= 0) k--;
	iPrec = iPrec - k;
    }
    
  /* Update for EDIT or EXECUTE mode  */
    switch(globalDisplayListTraversalMode) {
    case DL_EDIT:
      /*
       * update elements (this is overkill, but okay for now)
       *	-- not as efficient as it should be (don't update EVERYTHING if only
       *	   one item changed!)
       */
	if(cdi != NULL) {
	    DlElement *dlElement = FirstDlElement(
	      cdi->selectedDlElementList);
	    unhighlightSelectedElements();
	    while(dlElement) {
		updateElementFromGlobalResourceBundle(
		  dlElement->structure.element);
		dlElement = dlElement->next;
	    }
	    dmTraverseNonWidgetsInDisplayList(cdi);
	    highlightSelectedElements();
	}
	break;

    case DL_EXECUTE:
	if(executeTimeCartesianPlotWidget != NULL) {
	    if(isMax) {
		CpSetAxisMax(executeTimeCartesianPlotWidget, axis,
		  maxF, tickF, tickF, iPrec);
	    } else {
		CpSetAxisMin(executeTimeCartesianPlotWidget, axis,
		  minF, tickF, tickF, iPrec);
	    }
	}
	break;
    }
}

void cpAxisTextFieldLosingFocusCallback(Widget w, XtPointer cd, XtPointer cbs)
  /* Note: Losing focus happens when cursor leaves cartesianPlotAxisS,
     too */
{
    int rcType = (int)cd;
    char string[MAX_TOKEN_LENGTH], *currentString;
    int tail;
    XcVType minF[3], maxF[3];

    UNREFERENCED(cbs);

#if DEBUG_LOSING_FOCUS
    print("\ncpAxisTextFieldLosingFocusCallback: Entered\n");
#endif
#if DEBUG_XRT
    print("\ncpAxisTextFieldLosingFocusCallback: Entered\n");
    print("  executeTimeCartesianPlotWidget: %d\n",
      executeTimeCartesianPlotWidget);
    print("  rcType: %d  rcType%%3: %d\n",rcType,rcType%3);
    print("  axisRangeMin[rcType%%3]: %d  axisRangeMax[rcType%%3]: %d  "
      "w: %d\n",axisRangeMin[rcType%3],axisRangeMax[rcType%3],w);
#endif    
  /* Losing focus - make sure that the text field remains accurate wrt
   *   values stored in widget (not necessarily what is in the
   *   globalResourceBundle) */
    if(executeTimeCartesianPlotWidget != NULL) {
	CpGetAxisMaxMin(executeTimeCartesianPlotWidget, CP_X,
	  &maxF[X_AXIS_ELEMENT], &minF[X_AXIS_ELEMENT]);
	CpGetAxisMaxMin(executeTimeCartesianPlotWidget, CP_Y,
	  &maxF[Y1_AXIS_ELEMENT], &minF[Y1_AXIS_ELEMENT]);
	CpGetAxisMaxMin(executeTimeCartesianPlotWidget, CP_Y2,
	  &maxF[Y2_AXIS_ELEMENT], &minF[Y2_AXIS_ELEMENT]);
    }  else {
	return;
    }

#if DEBUG_LOSING_FOCUS
    print("  switch(rcType)\n");
#endif
    switch(rcType) {
    case CP_X_RANGE_MIN:
    case CP_Y_RANGE_MIN:
    case CP_Y2_RANGE_MIN:
	sprintf(string,"%f", minF[rcType%3].fval);
	break;
    case CP_X_RANGE_MAX:
    case CP_Y_RANGE_MAX:
    case CP_Y2_RANGE_MAX:
	sprintf(string,"%f", maxF[rcType%3].fval);
	break;
    default:
	medmPostMsg(1,"cpAxisTextFieldLosingFocusCallback: Unknown rcType = %d",
	  rcType/3);
	return;
    }
#if DEBUG_LOSING_FOCUS
    print("  Strip trailing zeros\n");
#endif
  /* Strip trailing zeros */
    tail = strlen(string);
    while(string[--tail] == '0') string[tail] = '\0';
  /* Set the new value if it is different */
    currentString = XmTextFieldGetString(w);
    if(strcmp(string,currentString)) {
	XmTextFieldSetString(w,string);
#if DEBUG_LOSING_FOCUS
	print("  XmTextFieldSetString\n");
#endif
    }
    XtFree(currentString);
}

/*
 * Menu entry support routine for the Cartesian Plot Axis Dialog...
 */
void createCartesianPlotAxisDialogMenuEntry(
  Widget parentRC,
  XmString axisLabelXmString,
  Widget *label,
  Widget *menu,
  XmString *menuLabelXmStrings,
  XmButtonType *buttonType,
  int numberOfLabels,
  XtPointer clientData)
{
    Arg args[10];
    int n = 0;
    Widget rowColumn;

  /* create rowColumn widget to hold the label and menu widgets */
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_NONE); n++;
    rowColumn = XmCreateRowColumn(parentRC,"entryRC",args,n);
 
  /* create the label widget */
    n = 0;
    XtSetArg(args[n],XmNalignment,XmALIGNMENT_END); n++;
    XtSetArg(args[n],XmNlabelString,axisLabelXmString); n++;
    XtSetArg(args[n],XmNrecomputeSize,False); n++;
    *label = XmCreateLabel(rowColumn,"localLabel",args,n);
 
  /* create the text widget */
    n = 0;
    XtSetArg(args[n],XmNbuttonType,buttonType); n++;
    XtSetArg(args[n],XmNbuttons,menuLabelXmStrings); n++;
    XtSetArg(args[n],XmNbuttonCount,numberOfLabels); n++;
    XtSetArg(args[n],XmNsimpleCallback,cpAxisOptionMenuSimpleCallback); n++;
    XtSetArg(args[n],XmNuserData,clientData); n++;
    *menu = XmCreateSimpleOptionMenu(rowColumn,"localElement",args,n);
    XtUnmanageChild(XmOptionLabelGadget(*menu));
    XtManageChild(rowColumn);
}

/* Text entry support routine for the Cartesian Plot Axis Dialog...  */
void createCartesianPlotAxisDialogTextEntry(
  Widget parentRC,
  XmString axisLabelXmString,
  Widget *rowColumn,
  Widget *label,
  Widget *text,
  XtPointer clientData)
{
    Arg args[10];
    int n = 0;
  /* Create a row column widget to hold the label and text field widget */
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_NONE); n++;
    *rowColumn = XmCreateRowColumn(parentRC,"entryRC",args,n);
 
  /* Create the label */
    n = 0;
    XtSetArg(args[n],XmNalignment,XmALIGNMENT_END); n++;
    XtSetArg(args[n],XmNlabelString,axisLabelXmString); n++;
    XtSetArg(args[n],XmNrecomputeSize,False); n++;
    *label = XmCreateLabel(*rowColumn,"localLabel",args,n);
 
  /* Create the text field */
    n = 0;
    XtSetArg(args[n],XmNmaxLength,MAX_TOKEN_LENGTH-1); n++;
    *text = XmCreateTextField(*rowColumn,"localElement",args,n);
    XtAddCallback(*text,XmNactivateCallback,
      cpAxisTextFieldActivateCallback,clientData);
    XtAddCallback(*text,XmNlosingFocusCallback,
      cpAxisTextFieldLosingFocusCallback,clientData);
    XtAddCallback(*text,XmNmodifyVerifyCallback,
      textFieldFloatVerifyCallback,NULL);
    XtManageChild(*rowColumn);
}
     

/*
 * Create Cartesian Plot axis dialog box
 */
Widget createCartesianPlotAxisDialog(Widget parent)
{
    Widget shell, closeButton;
    Arg args[12];
    int counter;
    XmString xmString, axisStyleXmString, axisRangeXmString, axisMinXmString,
      axisMaxXmString, axisTimeFmtXmString, frameLabelXmString;
    int i, n;
    static Boolean first = True;
    XmButtonType buttonType[MAX_CP_AXIS_BUTTONS];
    Widget entriesRC, frame, localLabel, parentRC;
  /* For keeping list of widgets around */
    Widget entryLabel[MAX_CP_AXIS_ELEMENTS], entryElement[MAX_CP_AXIS_ELEMENTS];
    Dimension width, height;
    static int maxWidth = 0, maxHeight = 0;
  /* Indexed like dlCartesianPlot->axis[]: X_ELEMENT_AXIS, Y1_ELEMENT_AXIS... */
    static char *frameLabelString[3] = {"X Axis", "Y1 Axis", "Y2 Axis",};

    UNREFERENCED(parent);

  /* Initialize XmString value tables (since this can be edit or execute time) */
    initializeXmStringValueTables();

  /* Set buttons to be push button */
    for(i = 0; i < MAX_CP_AXIS_BUTTONS; i++) buttonType[i] = XmPUSHBUTTON;

  /*
   * Create the interface
   *		     ...
   *		 OK     CANCEL
   */
    
  /* Shell */
    n = 0;
    XtSetArg(args[n],XmNdeleteResponse,XmDO_NOTHING); n++;
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
  /* KE: The following is necessary for Exceed, which turns off the
     resize function with the handles.  It should not be necessary */
    XtSetArg(args[n],XmNmwmFunctions, MWM_FUNC_ALL); n++;
    XtSetArg(args[n],XmNautoUnmanage,False); n++;
    XtSetArg(args[n],XmNtitle,"Cartesian Plot Axis Data"); n++;
    shell = XtCreatePopupShell("cartesianPlotAxisS",
      topLevelShellWidgetClass,mainShell,args,n);
    XmAddWMProtocolCallback(shell,WM_DELETE_WINDOW,
      cartesianPlotAxisActivate,
      (XtPointer)CP_CLOSE_BTN);

  /* Form */
    n = 0;
    XtSetArg(args[n],XmNautoUnmanage,False); n++;
    XtSetArg(args[n],XmNmarginHeight,8); n++;
    XtSetArg(args[n],XmNmarginWidth,8); n++;
    XtSetArg(args[n],XmNdialogStyle,XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
    cpAxisForm = XmCreateForm(shell,"cartesianPlotAxisF",args,n);

  /* RowColumn */
    n = 0;
    XtSetArg(args[n],XmNnumColumns,1); n++;
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_COLUMN); n++;
    entriesRC = XmCreateRowColumn(cpAxisForm,"entriesRC",args,n);

    axisStyleXmString = XmStringCreateLocalized("Axis Style");
    axisRangeXmString = XmStringCreateLocalized("Axis Range");
    axisMinXmString = XmStringCreateLocalized("Minimum Value");
    axisMaxXmString = XmStringCreateLocalized("Maximum Value");
    axisTimeFmtXmString = XmStringCreateLocalized("Time format");

  /* Loop over major elements */
    counter = 0;
    for(i = X_AXIS_ELEMENT /* 0 */; i <= Y2_AXIS_ELEMENT /* 2 */; i++) {
      /* Frame */
	n = 0;
	XtSetArg(args[n],XmNshadowType,XmSHADOW_ETCHED_IN); n++;
	frame = XmCreateFrame(entriesRC,"frame",args,n);
	XtManageChild(frame);

      /* Label */
	n = 0;
	frameLabelXmString = XmStringCreateLocalized(frameLabelString[i]);
	XtSetArg(args[n],XmNlabelString,frameLabelXmString); n++;
	XtSetArg(args[n],XmNmarginWidth,0); n++;
	XtSetArg(args[n],XmNmarginHeight,0); n++;
	XtSetArg(args[n],XmNchildType,XmFRAME_TITLE_CHILD); n++;
      /* (Use font calculation for textField (which uses ~90% of height)) */
	XtSetArg(args[n],XmNfontList,fontListTable[textFieldFontListIndex(24)]);
	n++;
	localLabel = XmCreateLabel(frame,"label",args,n);
	XtManageChild(localLabel);
	XmStringFree(frameLabelXmString);

      /* RC within frame */
	n = 0;
	XtSetArg(args[n],XmNnumColumns,1); n++;
	XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
	XtSetArg(args[n],XmNpacking,XmPACK_COLUMN); n++;
	parentRC = XmCreateRowColumn(frame,"parentRC",args,n);
	XtManageChild(parentRC);

      /* Create Axis Style Entry */
	createCartesianPlotAxisDialogMenuEntry(
	  parentRC,
	  axisStyleXmString,
	  &(entryLabel[counter]),
	  &(entryElement[counter]),
	  &(xmStringValueTable[FIRST_CARTESIAN_PLOT_AXIS_STYLE]),
	  buttonType,
	  (!i)?NUM_CARTESIAN_PLOT_AXIS_STYLES:NUM_CARTESIAN_PLOT_AXIS_STYLES-1,
	  (XtPointer)(CP_X_AXIS_STYLE+i));
	axisStyleMenu[i] =  entryElement[counter];
	counter++;

      /* Create Range Style Entry */
	createCartesianPlotAxisDialogMenuEntry(parentRC,
	  axisRangeXmString,
	  &(entryLabel[counter]),
	  &(entryElement[counter]),
	  &(xmStringValueTable[FIRST_CARTESIAN_PLOT_RANGE_STYLE]),
	  buttonType,
	  NUM_CARTESIAN_PLOT_RANGE_STYLES,
	  (XtPointer)(CP_X_RANGE_STYLE+i));
	axisRangeMenu[i] =  entryElement[counter];
	counter++;

      /* Create Min text field entry */
	createCartesianPlotAxisDialogTextEntry(
	  parentRC, axisMinXmString,
	  &(axisRangeMinRC[i]), &(entryLabel[counter]),
	  &(entryElement[counter]), (XtPointer)(CP_X_RANGE_MIN+i));
	axisRangeMin[i] = entryElement[counter];
	counter++;
 
      /* Create Max text field entry */
	createCartesianPlotAxisDialogTextEntry(
	  parentRC, axisMaxXmString,
	  &(axisRangeMaxRC[i]), &(entryLabel[counter]),
	  &(entryElement[counter]), (XtPointer)(CP_X_RANGE_MAX+i));
	axisRangeMax[i] = entryElement[counter];
	counter++;

      /* Create time format menu entry for X axis only */
	if(i == X_AXIS_ELEMENT) {
	    createCartesianPlotAxisDialogMenuEntry(
	      parentRC,
	      axisTimeFmtXmString,
	      &(entryLabel[counter]),
	      &(entryElement[counter]),
	      &(xmStringValueTable[FIRST_CP_TIME_FORMAT]),
	      buttonType,
	      NUM_CP_TIME_FORMAT,
	      (XtPointer)(CP_X_TIME_FORMAT));
	    axisTimeFormat = entryElement[counter];
	    counter++;
	}
    }

  /* Get max sizes and manage */
    for(i = 0; i < counter; i++) {
	XtVaGetValues(entryLabel[i],XmNwidth,&width,XmNheight,&height,NULL);
	maxLabelWidth = MAX(maxLabelWidth,width);
	maxLabelHeight = MAX(maxLabelHeight,height);
	XtVaGetValues(entryElement[i],XmNwidth,&width,XmNheight,&height,NULL);
	maxLabelWidth = MAX(maxLabelWidth,width);
	maxLabelHeight = MAX(maxLabelHeight,height);
#if 0
      /* Is done below */
	XtManageChild(entryLabel[i]);
	XtManageChild(entryElement[i]);
#endif	
    }

   /* Resize the labels and elements (to max width) for uniform appearance */
    for(i = 0; i < counter; i++) {
      /* Set label */
	XtVaSetValues(entryLabel[i],XmNwidth,maxLabelWidth,
	  XmNheight,maxLabelHeight,XmNrecomputeSize,False,
	  XmNalignment,XmALIGNMENT_END,NULL);

      /* Set element */
	if(XtClass(entryElement[i]) == xmRowColumnWidgetClass) {
	  /* must be option menu - unmanage label widget */
	    XtVaSetValues(XmOptionButtonGadget(entryElement[i]),
	      XmNx,(Position)maxLabelWidth, XmNwidth,maxLabelWidth,
	      XmNheight,maxLabelHeight,
	      XmNrecomputeSize,False, XmNresizeWidth,True,
	      XmNmarginWidth,0,
	      NULL);
	}
	XtVaSetValues(entryElement[i],
	  XmNx,(Position)maxLabelWidth, XmNwidth,maxLabelWidth,
	  XmNheight,maxLabelHeight,
	  XmNrecomputeSize,False, XmNresizeWidth,True,
	  XmNmarginWidth,0,
	  NULL);
	XtManageChild(entryLabel[i]);
	XtManageChild(entryElement[i]);
    }

  /* Free strings */
    XmStringFree(axisStyleXmString);
    XmStringFree(axisRangeXmString);
    XmStringFree(axisMinXmString);
    XmStringFree(axisMaxXmString);
    XmStringFree(axisTimeFmtXmString);
    
  /* Set values for entriesRC (After resizing) */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
    XtSetValues(entriesRC,args,n);

  /* Manage the RC */
    XtManageChild(entriesRC);
    
  /* Close button */
    n = 0;
    xmString = XmStringCreateLocalized("Close");
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,entriesRC); n++;
    XtSetArg(args[n],XmNtopOffset,12); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomOffset,12); n++;
  /* HACK - approximate centering button by putting at 43% of form width */
    XtSetArg(args[n],XmNleftPosition,(Position)43); n++;
    closeButton = XmCreatePushButton(cpAxisForm,"closeButton",args,n);
    XtAddCallback(closeButton,XmNactivateCallback,
      cartesianPlotAxisActivate, (XtPointer)CP_CLOSE_BTN);
    XtManageChild(closeButton);
    XmStringFree(xmString);

  /* Return the shell */
    return (shell);
}

/*
 * access function (for file-scoped globals, etc) to udpate the
 *	cartesian plot axis dialog with the values currently in
 *	globalResourceBundle
 */
void updateCartesianPlotAxisDialog()
{
    int i, tail;
    char string[MAX_TOKEN_LENGTH];

    for(i = X_AXIS_ELEMENT; i <= Y2_AXIS_ELEMENT; i++) {
	optionMenuSet(axisStyleMenu[i], globalResourceBundle.axis[i].axisStyle
	  - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
	optionMenuSet(axisRangeMenu[i], globalResourceBundle.axis[i].rangeStyle
	  - FIRST_CARTESIAN_PLOT_RANGE_STYLE);
	if(globalResourceBundle.axis[i].rangeStyle == USER_SPECIFIED_RANGE) {
	    sprintf(string,"%f",globalResourceBundle.axis[i].minRange);
	  /* strip trailing zeroes */
	    tail = strlen(string);
	    while(string[--tail] == '0') string[tail] = '\0';
	    XmTextFieldSetString(axisRangeMin[i],string);
	    sprintf(string,"%f",globalResourceBundle.axis[i].maxRange);
	  /* strip trailing zeroes */
	    tail = strlen(string);
	    while(string[--tail] == '0') string[tail] = '\0';
	    XmTextFieldSetString(axisRangeMax[i],string);
	    XtSetSensitive(axisRangeMinRC[i],True);
	    XtSetSensitive(axisRangeMaxRC[i],True);
	} else {
	    XtSetSensitive(axisRangeMinRC[i],False);
	    XtSetSensitive(axisRangeMaxRC[i],False);
	}
    }
    if(globalResourceBundle.axis[0].axisStyle == TIME_AXIS) {
	XtSetSensitive(axisTimeFormat,True);
	optionMenuSet(axisTimeFormat,globalResourceBundle.axis[0].timeFormat
	  - FIRST_CP_TIME_FORMAT);
    } else {
	XtSetSensitive(axisTimeFormat,False);
    }
}

/*
 * access function (for file-scoped globals, etc) to udpate the
 *      cartesian plot axis dialog with the values currently in
 *      the subject cartesian plot
 */
void updateCartesianPlotAxisDialogFromWidget(Widget cp)
{
    int i, tail, buttonId;
    char string[MAX_TOKEN_LENGTH];
    MedmCartesianPlot *pcp;
    XtPointer userData;
    Boolean xAxisIsLog, y1AxisIsLog, y2AxisIsLog, xAxisIsTime,
      xAxisIsAuto, y1AxisIsAuto, y2AxisIsAuto,
      xIsCurrentlyFromChannel, y1IsCurrentlyFromChannel,
      y2IsCurrentlyFromChannel;
    XcVType xMinF, xMaxF, y1MinF, y1MaxF, y2MinF, y2MaxF;
    char *timeFormat = NULL;
    if(globalDisplayListTraversalMode != DL_EXECUTE) return;

    CpGetAxisInfo(cp, &userData, &xAxisIsTime, &timeFormat,
      &xAxisIsLog, &y1AxisIsLog, &y2AxisIsLog,
      &xAxisIsAuto, &y1AxisIsAuto, &y2AxisIsAuto,
      &xMaxF, &y1MaxF, &y2MaxF,
      &xMinF, &y1MinF, &y2MinF);

    /* Determine range by first checking if from channel using the
     *   MedmCartesianPlot.  If not from channel, determine whether
     *   auto or user from AxisIsAuto. */
      if(pcp = (MedmCartesianPlot *)userData) {
	xIsCurrentlyFromChannel =
	  pcp->axisRange[X_AXIS_ELEMENT].isCurrentlyFromChannel;
	y1IsCurrentlyFromChannel =
	  pcp->axisRange[Y1_AXIS_ELEMENT].isCurrentlyFromChannel;
	y2IsCurrentlyFromChannel =
	  pcp->axisRange[Y2_AXIS_ELEMENT].isCurrentlyFromChannel;
    }

  /* X Axis Style */
    if(xAxisIsTime) {
      /* Style */
	optionMenuSet(axisStyleMenu[X_AXIS_ELEMENT],
	  TIME_AXIS - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
      /* Time format */
	buttonId = 0;     /* Use for  default */
	for(i = 0; i < NUM_CP_TIME_FORMAT; i++) {
	    if(timeFormat && !strcmp(cpTimeFormatString[i],timeFormat)) {
		buttonId = i;
		break;
	    }
	}
	optionMenuSet(axisTimeFormat, buttonId);
	XtSetSensitive(axisTimeFormat, True);
    } else {
      /* Style */
	optionMenuSet(axisStyleMenu[X_AXIS_ELEMENT],
	  (xAxisIsLog ? LOG10_AXIS : LINEAR_AXIS)
	  - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
	XtSetSensitive(axisTimeFormat, False);
    }
  /* X Axis Range */
    buttonId = (xIsCurrentlyFromChannel ? CHANNEL_RANGE :
      (xAxisIsAuto ? AUTO_SCALE_RANGE : USER_SPECIFIED_RANGE))
      - FIRST_CARTESIAN_PLOT_RANGE_STYLE;
    optionMenuSet(axisRangeMenu[X_AXIS_ELEMENT], buttonId);
    if(buttonId == USER_SPECIFIED_RANGE - FIRST_CARTESIAN_PLOT_RANGE_STYLE) {
	sprintf(string,"%f",xMinF.fval);
      /* strip trailing zeroes */
	tail = strlen(string);
	while(string[--tail] == '0') string[tail] = '\0';
	XmTextFieldSetString(axisRangeMin[X_AXIS_ELEMENT],string);
	sprintf(string,"%f",xMaxF.fval);
      /* strip trailing zeroes */
	tail = strlen(string);
	while(string[--tail] == '0') string[tail] = '\0';
	XmTextFieldSetString(axisRangeMax[X_AXIS_ELEMENT],string);
    }
    if(!xAxisIsAuto && !xIsCurrentlyFromChannel) { 
	XtSetSensitive(axisRangeMinRC[X_AXIS_ELEMENT],True);
	XtSetSensitive(axisRangeMaxRC[X_AXIS_ELEMENT],True);
    } else {
	XtSetSensitive(axisRangeMinRC[X_AXIS_ELEMENT],False);
	XtSetSensitive(axisRangeMaxRC[X_AXIS_ELEMENT],False);
    }

  /* Y1 Axis both */
    optionMenuSet(axisStyleMenu[Y1_AXIS_ELEMENT],
      (y1AxisIsLog ? LOG10_AXIS : LINEAR_AXIS)
      - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
    buttonId = (y1IsCurrentlyFromChannel ? CHANNEL_RANGE :
      (y1AxisIsAuto ? AUTO_SCALE_RANGE : USER_SPECIFIED_RANGE))
      - FIRST_CARTESIAN_PLOT_RANGE_STYLE;
    optionMenuSet(axisRangeMenu[Y1_AXIS_ELEMENT], buttonId);
    if(buttonId == USER_SPECIFIED_RANGE - FIRST_CARTESIAN_PLOT_RANGE_STYLE) {
	sprintf(string,"%f",y1MinF.fval);
      /* strip trailing zeroes */
	tail = strlen(string);
	while(string[--tail] == '0') string[tail] = '\0';
	XmTextFieldSetString(axisRangeMin[Y1_AXIS_ELEMENT],string);
	sprintf(string,"%f",y1MaxF.fval);
      /* strip trailing zeroes */
	tail = strlen(string);
	while(string[--tail] == '0') string[tail] = '\0';
	XmTextFieldSetString(axisRangeMax[Y1_AXIS_ELEMENT],string);
    }
    if(!y1AxisIsAuto && !y1IsCurrentlyFromChannel) {
	XtSetSensitive(axisRangeMinRC[Y1_AXIS_ELEMENT],True);
	XtSetSensitive(axisRangeMaxRC[Y1_AXIS_ELEMENT],True);
    } else {
	XtSetSensitive(axisRangeMinRC[Y1_AXIS_ELEMENT],False);
	XtSetSensitive(axisRangeMaxRC[Y1_AXIS_ELEMENT],False);
    }


  /* Y2 Axis both */
    optionMenuSet(axisStyleMenu[Y2_AXIS_ELEMENT],
      (y2AxisIsLog ? LOG10_AXIS : LINEAR_AXIS)
      - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
    buttonId = (y2IsCurrentlyFromChannel ? CHANNEL_RANGE :
      (y2AxisIsAuto ? AUTO_SCALE_RANGE : USER_SPECIFIED_RANGE))
      - FIRST_CARTESIAN_PLOT_RANGE_STYLE;
    optionMenuSet(axisRangeMenu[Y2_AXIS_ELEMENT], buttonId);
    if(buttonId == USER_SPECIFIED_RANGE - FIRST_CARTESIAN_PLOT_RANGE_STYLE) {
	sprintf(string,"%f",y2MinF.fval);
      /* strip trailing zeroes */
	tail = strlen(string);
	while(string[--tail] == '0') string[tail] = '\0';
	XmTextFieldSetString(axisRangeMin[Y2_AXIS_ELEMENT],string);
	sprintf(string,"%f",y2MaxF.fval);
      /* strip trailing zeroes */
	tail = strlen(string);
	while(string[--tail] == '0') string[tail] = '\0';
	XmTextFieldSetString(axisRangeMax[Y2_AXIS_ELEMENT],string);
    }
    if(!y2AxisIsAuto && !y2IsCurrentlyFromChannel) {
	XtSetSensitive(axisRangeMinRC[Y2_AXIS_ELEMENT],True);
	XtSetSensitive(axisRangeMaxRC[Y2_AXIS_ELEMENT],True);
    } else {
	XtSetSensitive(axisRangeMinRC[Y2_AXIS_ELEMENT],False);
	XtSetSensitive(axisRangeMaxRC[Y2_AXIS_ELEMENT],False);
    }
}

static void cartesianPlotActivate(Widget w, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *cdi=currentDisplayInfo;
    int buttonType = (int)cd;
    String **newCells;
    int i;

    UNREFERENCED(w);
    UNREFERENCED(cbs);

    switch (buttonType) {
    case CP_APPLY_BTN:
      /* commit changes in matrix to global matrix array data */
	XbaeMatrixCommitEdit(cpMatrix,False);
	XtVaGetValues(cpMatrix,XmNcells,&newCells,NULL);
      /* now update globalResourceBundle...*/
	for(i = 0; i < MAX_TRACES; i++) {
	    strcpy(globalResourceBundle.cpData[i].xdata,
	      newCells[i][CP_XDATA_COLUMN]);
	    strcpy(globalResourceBundle.cpData[i].ydata,
	      newCells[i][CP_YDATA_COLUMN]);
	    globalResourceBundle.cpData[i].data_clr = 
	      (int)cpColorRows[i][CP_COLOR_COLUMN];
	}
      /* and update the elements (since this level of "Apply" is analogous
       *	to changing text in a text field in the resource palette
       *	(don't need to traverse the display list since these changes
       *	 aren't visible at the first level)
       */
	if(cdi) {
	    DlElement *dlElement = FirstDlElement(
	      cdi->selectedDlElementList);
	    unhighlightSelectedElements();
	    while(dlElement) {
		if(dlElement->structure.element->type = DL_CartesianPlot)
		  updateElementFromGlobalResourceBundle(dlElement->structure.element);
		dlElement = dlElement->next;
	    }
	}
	medmMarkDisplayBeingEdited(cdi);
	XtPopdown(cartesianPlotS);
	break;
    case CP_CLOSE_BTN:
	XtPopdown(cartesianPlotS);
	break;
    }
}


static void cartesianPlotAxisActivate(Widget w, XtPointer cd, XtPointer cbs)
{
    int buttonType = (int)cd;

    UNREFERENCED(w);
    UNREFERENCED(cbs);

    switch (buttonType) {
    case CP_CLOSE_BTN:
	XtPopdown(cartesianPlotAxisS);
      /* Since done with CP Axis dialog, reset the selected widget */
	executeTimeCartesianPlotWidget  = NULL;
	break;
    }
}

/*
 * function to handle cell selection in the matrix
 *	mostly it passes through for the text field entry
 *	but pops up the color editor for the color field selection
 */
void cpEnterCellCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    XbaeMatrixEnterCellCallbackStruct *call_data = (XbaeMatrixEnterCellCallbackStruct *) cbs;
    int row;

    UNREFERENCED(w);
    UNREFERENCED(cbs);
    
    if(call_data->column == CP_COLOR_COLUMN) {
      /* set this cell non-editable */
	call_data->doit = False;
      /* update the color palette, set index of the color vector element to set */
	row = call_data->row;
	setCurrentDisplayColorsInColorPalette(CPDATA_RC,row);
	XtPopup(colorS,XtGrabNone);
    }
}


/*
 * function to actually update the colors in the COLOR_COLUMN of the matrix
 */
void cpUpdateMatrixColors()
{
    int i;

  /* XmNcolors needs pixel values */
    for(i = 0; i < MAX_TRACES; i++) {
	cpColorRows[i][CP_COLOR_COLUMN] = currentColormap[
	  globalResourceBundle.cpData[i].data_clr];
    }
    if(cpMatrix != NULL) XtVaSetValues(cpMatrix,XmNcolors,cpColorCells,NULL);

  /* but for resource editing cpData should contain indexes into colormap */
  /* this resource is copied, hence this is okay to do */
    for(i = 0; i < MAX_TRACES; i++) {
	cpColorRows[i][CP_COLOR_COLUMN] = globalResourceBundle.cpData[i].data_clr;
    }
}

/*
 * create data dialog
 */
Widget createCartesianPlotDataDialog(Widget parent)
{
    Widget shell, applyButton, closeButton;
    Dimension cWidth, cHeight, aWidth, aHeight;
    Arg args[12];
    XmString xmString;
    int i, j, n;
    static Boolean first = True;


  /* Initialize file-scoped globals */
    if(first) {
	first = False;
	for(i = 0; i < MAX_TRACES; i++) {
	    for(j = 0; j < 2; j++) cpRows[i][j] = NULL;
	    cpRows[i][2] = dashes;
	    cpCells[i] = &cpRows[i][0];
	    cpColorCells[i] = &cpColorRows[i][0];
	}
    }

  /*
   * Create the interface
   *
   *	       xdata | ydata | color
   *	       ---------------------
   *	    1 |  A      B      C
   *	    2 | 
   *	    3 | 
   *		     ...
   *		 OK     CANCEL
   */
    
    n = 0;
    XtSetArg(args[n],XmNautoUnmanage,False); n++;
    XtSetArg(args[n],XmNmarginHeight,8); n++;
    XtSetArg(args[n],XmNmarginWidth,8); n++;
    XtSetArg(args[n],XmNdialogStyle,XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
    cpForm = XmCreateFormDialog(parent,"cartesianPlotDataF",args,n);
    shell = XtParent(cpForm);
    n = 0;
    XtSetArg(args[n],XmNtitle,"Cartesian Plot Data"); n++;
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
  /* KE: The following is necessary for Exceed, which turns off the
     resize function with the handles.  It should not be necessary */
    XtSetArg(args[n],XmNmwmFunctions, MWM_FUNC_ALL); n++;
    XtSetValues(shell,args,n);
    XmAddWMProtocolCallback(shell,WM_DELETE_WINDOW,
      cartesianPlotActivate,(XtPointer)CP_CLOSE_BTN);
    n = 0;
    XtSetArg(args[n],XmNrows,MAX_TRACES); n++;
    XtSetArg(args[n],XmNcolumns,3); n++;
    XtSetArg(args[n],XmNcolumnMaxLengths,cpColumnMaxLengths); n++;
    XtSetArg(args[n],XmNcolumnWidths,cpColumnWidths); n++;
    XtSetArg(args[n],XmNcolumnLabels,cpColumnLabels); n++;
    XtSetArg(args[n],XmNcolumnMaxLengths,cpColumnMaxLengths); n++;
    XtSetArg(args[n],XmNcolumnWidths,cpColumnWidths); n++;
    XtSetArg(args[n],XmNcolumnLabelAlignments,cpColumnLabelAlignments); n++;
    XtSetArg(args[n],XmNboldLabels,False); n++;
    cpMatrix = XtCreateManagedWidget("cpMatrix",
      xbaeMatrixWidgetClass,cpForm,args,n);
    cpUpdateMatrixColors();
    XtAddCallback(cpMatrix,XmNenterCellCallback,
      cpEnterCellCallback,(XtPointer)NULL);


    xmString = XmStringCreateLocalized("Cancel");
    n = 0;
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    closeButton = XmCreatePushButton(cpForm,"closeButton",args,n);
    XtAddCallback(closeButton,XmNactivateCallback,
      cartesianPlotActivate,(XtPointer)CP_CLOSE_BTN);
    XtManageChild(closeButton);
    XmStringFree(xmString);

    xmString = XmStringCreateLocalized("Apply");
    n = 0;
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    applyButton = XmCreatePushButton(cpForm,"applyButton",args,n);
    XtAddCallback(applyButton,XmNactivateCallback,
      cartesianPlotActivate,(XtPointer)CP_APPLY_BTN);
    XtManageChild(applyButton);
    XmStringFree(xmString);

  /* Make APPLY and CLOSE buttons same size */
    XtVaGetValues(closeButton,XmNwidth,&cWidth,XmNheight,&cHeight,NULL);
    XtVaGetValues(applyButton,XmNwidth,&aWidth,XmNheight,&aHeight,NULL);
    XtVaSetValues(closeButton,XmNwidth,MAX(cWidth,aWidth),
      XmNheight,MAX(cHeight,aHeight),NULL);

  /* Make the APPLY button the default for the form */
    XtVaSetValues(cpForm,XmNdefaultButton,applyButton,NULL);

  /*
   * Do form layout 
   */

  /* cpMatrix */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
    XtSetValues(cpMatrix,args,n);
  /* apply */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,cpMatrix); n++;
    XtSetArg(args[n],XmNtopOffset,12); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNleftPosition,25); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomOffset,12); n++;
    XtSetValues(applyButton,args,n);
  /* close */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,cpMatrix); n++;
    XtSetArg(args[n],XmNtopOffset,12); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNrightPosition,75); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomOffset,12); n++;
    XtSetValues(closeButton,args,n);


    XtManageChild(cpForm);

    return shell;
}

/*
 * access function (for file-scoped globals, etc) to udpate the
 *	cartesian plot data dialog with the values currently in
 *	globalResourceBundle
 */
void updateCartesianPlotDataDialog()
{
    int i;

    for(i = 0; i < MAX_TRACES; i++) {
	cpRows[i][0] = globalResourceBundle.cpData[i].xdata;
	cpRows[i][1] = globalResourceBundle.cpData[i].ydata;
	cpRows[i][2] =  dashes;
    }
  /* handle data_clr in here */
    cpUpdateMatrixColors();
    if(cpMatrix)
      XtVaSetValues(cpMatrix,XmNcells,cpCells,NULL);
}
