/******************************************************************
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
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (708-252-2000).
*/
/*****************************************************************************
 *
 *     Original Author : Mark Andersion
 *     Current Author  : Frederick Vong
 *
 * Modification Log:
 * -----------------
 * .01  03-01-95        vong    2.0.0 release
 * .02  09-05-95        vong    2.1.0 release
 *                              - using new screen update dispatch mechanism
 *
 *****************************************************************************
*/

#include "medm.h"
#include "medmCartesianPlot.h"

void cartesianPlotCreateRunTimeInstance(DisplayInfo *displayInfo,DlCartesianPlot *dlCartesianPlot);
void cartesianPlotCreateEditInstance(DisplayInfo *displayInfo,DlCartesianPlot *dlCartesianPlot);
static void cartesianPlotUpdateGraphicalInfoCb(XtPointer cd);
static void cartesianPlotUpdateScreenFirstTime(XtPointer cd);
static void cartesianPlotDraw(XtPointer cd);
static void cartesianPlotUpdateValueCb(XtPointer cd);
static void cartesianPlotDestroyCb(XtPointer cd);
static void cartesianPlotName(XtPointer, char **, short *, int *);

static XrtData *nullData = NULL;
void cartesianPlotCreateRunTimeInstance(DisplayInfo *displayInfo,
			DlCartesianPlot *dlCartesianPlot)
{
  CartesianPlot *pcp;
  int i, j, k, n, validTraces, iPrec;
  Arg args[42];
  Widget localWidget;
  XColor xColors[2];
  char *headerStrings[2];
  char rgb[2][16], string[24];
  int usedHeight, usedCharWidth, bestSize, preferredHeight;
  XcVType minF, maxF, tickF;
  int precision, log10Precision;

  if (!nullData) {
    nullData = XrtMakeData(XRT_GENERAL,1,1,TRUE);
    nullData->g.hole = 0.0;
    nullData->g.data[0].npoints = 1;
    nullData->g.data[0].xp[0] = 0.0;
    nullData->g.data[0].yp[0] = 0.0;
  }
  validTraces = 0;
  pcp = (CartesianPlot *) malloc(sizeof(CartesianPlot));
  pcp->xrtData1 = pcp->xrtData2 = NULL;
  pcp->dirty1 = pcp->dirty2 = False;
  pcp->eraseMode = dlCartesianPlot->eraseMode;
  pcp->dlCartesianPlot = dlCartesianPlot;
  pcp->updateTask = updateTaskAddTask(displayInfo,
           &(dlCartesianPlot->object),
	   cartesianPlotDraw,
	   (XtPointer)pcp);
  if (pcp->updateTask == NULL) {
    medmPrintf("cartesianPlotCreateRunTimeInstance : memory allocation error\n");
  } else {
    updateTaskAddDestroyCb(pcp->updateTask,cartesianPlotDestroyCb);
    updateTaskAddNameCb(pcp->updateTask,cartesianPlotName);
  }

  for (i = 0; i < MAX_TRACES; i++) {
    Boolean validTrace = False;
    /* X data */
    if (dlCartesianPlot->trace[i].xdata[0] != '\0') {
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
    if (dlCartesianPlot->trace[i].ydata[0] != '\0') {
      pcp->xyTrace[validTraces].recordY =
              medmAllocateRecord(dlCartesianPlot->trace[i].ydata,
                cartesianPlotUpdateScreenFirstTime,
                cartesianPlotUpdateGraphicalInfoCb,
                (XtPointer) &(pcp->xyTrace[validTraces]));
      validTrace = True;
    } else {
      pcp->xyTrace[validTraces].recordY = NULL;
    }
    if (validTrace) {
      pcp->xyTrace[validTraces].cartesianPlot = pcp;
      validTraces++;
    }
  }

  if (validTraces == 0) {
    /* if no channel, create a fake channel */
    validTraces = 1;
    pcp->xyTrace[0].recordX = medmAllocateRecord(" ",
                cartesianPlotUpdateScreenFirstTime,
                cartesianPlotUpdateGraphicalInfoCb,
                (XtPointer) &(pcp->xyTrace[0]));
    pcp->xyTrace[0].recordY = NULL;
  }

  /* now add monitor for trigger channel if appropriate */
  if ((dlCartesianPlot->trigger[0] != '\0') && (validTraces > 0)) {
    pcp->triggerCh.recordX = 
              medmAllocateRecord(dlCartesianPlot->trigger,
                cartesianPlotUpdateScreenFirstTime,
                cartesianPlotUpdateGraphicalInfoCb,
                (XtPointer) &(pcp->triggerCh));
    pcp->triggerCh.cartesianPlot = pcp;
  } else {
    pcp->triggerCh.recordX = NULL;
  }

  /* now add monitor for erase channel if appropriate */
  if ((dlCartesianPlot->erase[0] != '\0') && (validTraces > 0)) {
    pcp->eraseCh.recordX =
              medmAllocateRecord(dlCartesianPlot->erase,
                cartesianPlotUpdateScreenFirstTime,
                cartesianPlotUpdateGraphicalInfoCb,
                (XtPointer) &(pcp->eraseCh));
    pcp->eraseCh.cartesianPlot = pcp;
  } else {
    pcp->eraseCh.recordX = NULL;
  }

  drawWhiteRectangle(pcp->updateTask);

  /* from the cartesianPlot structure, we've got CartesianPlot's specifics */
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlCartesianPlot->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlCartesianPlot->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlCartesianPlot->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlCartesianPlot->object.height); n++;
  XtSetArg(args[n],XmNhighlightThickness,0); n++;

  /* long way around for color handling... but XRT/Graph insists on strings! */
  xColors[0].pixel = displayInfo->dlColormap[dlCartesianPlot->plotcom.clr];
  xColors[1].pixel = displayInfo->dlColormap[dlCartesianPlot->plotcom.bclr];
  XQueryColors(display,cmap,xColors,2);
  sprintf(rgb[0],"#%2.2x%2.2x%2.2x",xColors[0].red>>8, xColors[0].green>>8,
		xColors[0].blue>>8);
  sprintf(rgb[1],"#%2.2x%2.2x%2.2x",xColors[1].red>>8, xColors[1].green>>8,
		xColors[1].blue>>8);
  XtSetArg(args[n],XtNxrtGraphForegroundColor,rgb[0]); n++;
  XtSetArg(args[n],XtNxrtGraphBackgroundColor,rgb[1]); n++;
  XtSetArg(args[n],XtNxrtForegroundColor,rgb[0]); n++;
  XtSetArg(args[n],XtNxrtBackgroundColor,rgb[1]); n++;
  preferredHeight = MIN(dlCartesianPlot->object.width,
		dlCartesianPlot->object.height)/TITLE_SCALE_FACTOR;
  bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
		preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
  XtSetArg(args[n],XtNxrtHeaderFont,fontTable[bestSize]->fid); n++;
  if (strlen(dlCartesianPlot->plotcom.title) > 0) {
    headerStrings[0] = dlCartesianPlot->plotcom.title;
  } else {
    headerStrings[0] = NULL;
  }
  headerStrings[1] = NULL;
  XtSetArg(args[n],XtNxrtHeaderStrings,headerStrings); n++;
  if (strlen(dlCartesianPlot->plotcom.xlabel) > 0) {
    XtSetArg(args[n],XtNxrtXTitle,dlCartesianPlot->plotcom.xlabel); n++;
  } else {
    XtSetArg(args[n],XtNxrtXTitle,NULL); n++;
  }
  if (strlen(dlCartesianPlot->plotcom.ylabel) > 0) {
    XtSetArg(args[n],XtNxrtYTitle,dlCartesianPlot->plotcom.ylabel); n++;
  } else {
    XtSetArg(args[n],XtNxrtYTitle,NULL); n++;
  }
  preferredHeight = MIN(dlCartesianPlot->object.width,
		dlCartesianPlot->object.height)/AXES_SCALE_FACTOR;
  bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
		preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
  XtSetArg(args[n],XtNxrtAxisFont,fontTable[bestSize]->fid); n++;
  switch (dlCartesianPlot->style) {
    case POINT_PLOT:
    case LINE_PLOT:
	XtSetArg(args[n],XtNxrtType,XRT_PLOT); n++;
	if (validTraces > 1) {
	    XtSetArg(args[n],XtNxrtType2,XRT_PLOT); n++;
	}
	break;
    case FILL_UNDER_PLOT:
	XtSetArg(args[n],XtNxrtType,XRT_AREA); n++;
	if (validTraces > 1) {
	    XtSetArg(args[n],XtNxrtType2,XRT_AREA); n++;
	}
	break;
  }
  if (validTraces > 1) {
    XtSetArg(args[n],XtNxrtY2AxisShow,TRUE); n++;
  }

/******* Axis Definitions *******/
/* X Axis definition */
  switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].axisStyle) {
	case LINEAR_AXIS:
		break;
	case LOG10_AXIS:
		XtSetArg(args[n],XtNxrtYAxisLogarithmic,True); n++;
		break;
	default:
		medmPrintf(
		"\nexecuteDlCartesianPlot: unknown X axis style"); break;
		break;
  }
  switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle) {
	case CHANNEL_RANGE:		/* handle as default until connected */
	case AUTO_SCALE_RANGE:
		XtSetArg(args[n],XtNxrtXNumUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtXTickUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtXPrecisionUseDefault,True); n++;
		break;
	case USER_SPECIFIED_RANGE:
		minF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].minRange;
		maxF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].maxRange;
		tickF.fval = (maxF.fval - minF.fval)/4.0;
		XtSetArg(args[n],XtNxrtXMin,minF.lval); n++;
		XtSetArg(args[n],XtNxrtXMax,maxF.lval); n++;
		XtSetArg(args[n],XtNxrtXTick,tickF.lval); n++;
		XtSetArg(args[n],XtNxrtXNum,tickF.lval); n++;
		sprintf(string,"%f",tickF.fval);
		k = strlen(string)-1;
		while (string[k] == '0') k--; /* strip off trailing zeroes */
		iPrec = k;
		while (string[k] != '.' && k >= 0) k--;
		iPrec = iPrec - k;
		XtSetArg(args[n],XtNxrtXPrecision,iPrec); n++;
		break;
	default:
		medmPrintf(
		"\nexecuteDlCartesianPlot: unknown X range style"); break;
		break;
  }
/* Y1 Axis definition */
  switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].axisStyle) {
	case LINEAR_AXIS:
		break;
	case LOG10_AXIS:
		XtSetArg(args[n],XtNxrtYAxisLogarithmic,True); n++;
		break;
	default:
		medmPrintf(
		"\nexecuteDlCartesianPlot: unknown Y1 axis style"); break;
		break;
  }
  switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle) {
	case CHANNEL_RANGE:		/* handle as default until connected */
	case AUTO_SCALE_RANGE:
		XtSetArg(args[n],XtNxrtYNumUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtYTickUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtYPrecisionUseDefault,True); n++;
		break;
	case USER_SPECIFIED_RANGE:
		minF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].minRange;
		maxF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange;
		tickF.fval = (maxF.fval - minF.fval)/4.0;
		XtSetArg(args[n],XtNxrtYMin,minF.lval); n++;
		XtSetArg(args[n],XtNxrtYMax,maxF.lval); n++;
		XtSetArg(args[n],XtNxrtYTick,tickF.lval); n++;
		XtSetArg(args[n],XtNxrtYNum,tickF.lval); n++;
		sprintf(string,"%f",tickF.fval);
		k = strlen(string)-1;
		while (string[k] == '0') k--; /* strip off trailing zeroes */
		iPrec = k;
		while (string[k] != '.' && k >= 0) k--;
		iPrec = iPrec - k;
		XtSetArg(args[n],XtNxrtYPrecision,iPrec); n++;
		break;
	default:
		medmPrintf(
		"\nexecuteDlCartesianPlot: unknown Y1 range style"); break;
		break;
  }
/* Y2 Axis definition */
  switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].axisStyle) {
	case LINEAR_AXIS:
		break;
	case LOG10_AXIS:
		XtSetArg(args[n],XtNxrtY2AxisLogarithmic,True); n++;
		break;
	default:
		medmPrintf(
		"\nexecuteDlCartesianPlot: unknown Y2 axis style"); break;
		break;
  }
  switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle) {
	case CHANNEL_RANGE:		/* handle as default until connected */
	case AUTO_SCALE_RANGE:
		XtSetArg(args[n],XtNxrtY2NumUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtY2TickUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtY2PrecisionUseDefault,True); n++;
		break;
	case USER_SPECIFIED_RANGE:
		minF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].minRange;
		maxF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].maxRange;
		tickF.fval = (maxF.fval - minF.fval)/4.0;
		XtSetArg(args[n],XtNxrtY2Min,minF.lval); n++;
		XtSetArg(args[n],XtNxrtY2Max,maxF.lval); n++;
		XtSetArg(args[n],XtNxrtY2Tick,tickF.lval); n++;
		XtSetArg(args[n],XtNxrtY2Num,tickF.lval); n++;
		sprintf(string,"%f",tickF.fval);
		k = strlen(string)-1;
		while (string[k] == '0') k--; /* strip off trailing zeroes */
		iPrec = k;
		while (string[k] != '.' && k >= 0) k--;
		iPrec = iPrec - k;
		XtSetArg(args[n],XtNxrtY2Precision,iPrec); n++;
		break;
	default:
		medmPrintf(
		"\nexecuteDlCartesianPlot: unknown Y2 range style"); break;
		break;
  }
  


  XtSetArg(args[n],XmNtraversalOn,False); n++;
/*
 * add the pointer to the CartesianPlot structure as userData to widget
 */
  XtSetArg(args[n], XmNuserData, (XtPointer) pcp); n++;
  XtSetArg(args[n], XtNxrtDoubleBuffer, True); n++;
  localWidget = XtCreateWidget("cartesianPlot",xtXrtGraphWidgetClass,
		displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] = localWidget;

/* record the number of traces in the cartesian plot */
  pcp->nTraces = validTraces;

/* record the widget that the monitor structures belong to */
  pcp->widget = localWidget;

/* add in drag/drop translations */
  XtOverrideTranslations(localWidget,parsedTranslations);

}

void cartesianPlotCreateEditInstance(DisplayInfo *displayInfo,
			DlCartesianPlot *dlCartesianPlot)
{
  Channel *pCh, *monitorData,
	*triggerCh;
  CartesianPlot *pcp;
  int i, j, k, n, validTraces, iPrec;
  Arg args[42];
  Widget localWidget;
  XColor xColors[2];
  char *headerStrings[2];
  char rgb[2][16], string[24];
  Boolean validTrace;
  int usedHeight, usedCharWidth, bestSize, preferredHeight;
  XcVType minF, maxF, tickF;
  int precision, log10Precision;


  displayInfo->useDynamicAttribute = FALSE;

  pcp = NULL;
  triggerCh = (Channel *)NULL;

  validTraces = 0;

  /* from the cartesianPlot structure, we've got CartesianPlot's specifics */
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlCartesianPlot->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlCartesianPlot->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlCartesianPlot->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlCartesianPlot->object.height); n++;
  XtSetArg(args[n],XmNhighlightThickness,0); n++;

  /* long way around for color handling... but XRT/Graph insists on strings! */
  xColors[0].pixel = displayInfo->dlColormap[dlCartesianPlot->plotcom.clr];
  xColors[1].pixel = displayInfo->dlColormap[dlCartesianPlot->plotcom.bclr];
  XQueryColors(display,cmap,xColors,2);
  sprintf(rgb[0],"#%2.2x%2.2x%2.2x",xColors[0].red>>8, xColors[0].green>>8,
		xColors[0].blue>>8);
  sprintf(rgb[1],"#%2.2x%2.2x%2.2x",xColors[1].red>>8, xColors[1].green>>8,
		xColors[1].blue>>8);
  XtSetArg(args[n],XtNxrtGraphForegroundColor,rgb[0]); n++;
  XtSetArg(args[n],XtNxrtGraphBackgroundColor,rgb[1]); n++;
  XtSetArg(args[n],XtNxrtForegroundColor,rgb[0]); n++;
  XtSetArg(args[n],XtNxrtBackgroundColor,rgb[1]); n++;
  preferredHeight = MIN(dlCartesianPlot->object.width,
		dlCartesianPlot->object.height)/TITLE_SCALE_FACTOR;
  bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
		preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
  XtSetArg(args[n],XtNxrtHeaderFont,fontTable[bestSize]->fid); n++;
  if (strlen(dlCartesianPlot->plotcom.title) > 0) {
    headerStrings[0] = dlCartesianPlot->plotcom.title;
  } else {
    headerStrings[0] = NULL;
  }
  headerStrings[1] = NULL;
  XtSetArg(args[n],XtNxrtHeaderStrings,headerStrings); n++;
  if (strlen(dlCartesianPlot->plotcom.xlabel) > 0) {
    XtSetArg(args[n],XtNxrtXTitle,dlCartesianPlot->plotcom.xlabel); n++;
  } else {
    XtSetArg(args[n],XtNxrtXTitle,NULL); n++;
  }
  if (strlen(dlCartesianPlot->plotcom.ylabel) > 0) {
    XtSetArg(args[n],XtNxrtYTitle,dlCartesianPlot->plotcom.ylabel); n++;
  } else {
    XtSetArg(args[n],XtNxrtYTitle,NULL); n++;
  }
  preferredHeight = MIN(dlCartesianPlot->object.width,
		dlCartesianPlot->object.height)/AXES_SCALE_FACTOR;
  bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
		preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
  XtSetArg(args[n],XtNxrtAxisFont,fontTable[bestSize]->fid); n++;
  switch (dlCartesianPlot->style) {
    case POINT_PLOT:
    case LINE_PLOT:
	XtSetArg(args[n],XtNxrtType,XRT_PLOT); n++;
	if (validTraces > 1) {
	    XtSetArg(args[n],XtNxrtType2,XRT_PLOT); n++;
	}
	break;
    case FILL_UNDER_PLOT:
	XtSetArg(args[n],XtNxrtType,XRT_AREA); n++;
	if (validTraces > 1) {
	    XtSetArg(args[n],XtNxrtType2,XRT_AREA); n++;
	}
	break;
  }
  if (validTraces > 1) {
    XtSetArg(args[n],XtNxrtY2AxisShow,TRUE); n++;
  }

/******* Axis Definitions *******/
/* X Axis definition */
  switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].axisStyle) {
	case LINEAR_AXIS:
		break;
	case LOG10_AXIS:
		XtSetArg(args[n],XtNxrtYAxisLogarithmic,True); n++;
		break;
	default:
		medmPrintf(
		"\nexecuteDlCartesianPlot: unknown X axis style"); break;
		break;
  }
  switch (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle) {
	case CHANNEL_RANGE:		/* handle as default until connected */
	case AUTO_SCALE_RANGE:
		XtSetArg(args[n],XtNxrtXNumUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtXTickUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtXPrecisionUseDefault,True); n++;
		break;
	case USER_SPECIFIED_RANGE:
		minF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].minRange;
		maxF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].maxRange;
		tickF.fval = (maxF.fval - minF.fval)/4.0;
		XtSetArg(args[n],XtNxrtXMin,minF.lval); n++;
		XtSetArg(args[n],XtNxrtXMax,maxF.lval); n++;
		XtSetArg(args[n],XtNxrtXTick,tickF.lval); n++;
		XtSetArg(args[n],XtNxrtXNum,tickF.lval); n++;
		sprintf(string,"%f",tickF.fval);
		k = strlen(string)-1;
		while (string[k] == '0') k--; /* strip off trailing zeroes */
		iPrec = k;
		while (string[k] != '.' && k >= 0) k--;
		iPrec = iPrec - k;
		XtSetArg(args[n],XtNxrtXPrecision,iPrec); n++;
		break;
	default:
		medmPrintf(
		"\nexecuteDlCartesianPlot: unknown X range style"); break;
		break;
  }
/* Y1 Axis definition */
  switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].axisStyle) {
	case LINEAR_AXIS:
		break;
	case LOG10_AXIS:
		XtSetArg(args[n],XtNxrtYAxisLogarithmic,True); n++;
		break;
	default:
		medmPrintf(
		"\nexecuteDlCartesianPlot: unknown Y1 axis style"); break;
		break;
  }
  switch (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle) {
	case CHANNEL_RANGE:		/* handle as default until connected */
	case AUTO_SCALE_RANGE:
		XtSetArg(args[n],XtNxrtYNumUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtYTickUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtYPrecisionUseDefault,True); n++;
		break;
	case USER_SPECIFIED_RANGE:
		minF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].minRange;
		maxF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange;
		tickF.fval = (maxF.fval - minF.fval)/4.0;
		XtSetArg(args[n],XtNxrtYMin,minF.lval); n++;
		XtSetArg(args[n],XtNxrtYMax,maxF.lval); n++;
		XtSetArg(args[n],XtNxrtYTick,tickF.lval); n++;
		XtSetArg(args[n],XtNxrtYNum,tickF.lval); n++;
		sprintf(string,"%f",tickF.fval);
		k = strlen(string)-1;
		while (string[k] == '0') k--; /* strip off trailing zeroes */
		iPrec = k;
		while (string[k] != '.' && k >= 0) k--;
		iPrec = iPrec - k;
		XtSetArg(args[n],XtNxrtYPrecision,iPrec); n++;
		break;
	default:
		medmPrintf(
		"\nexecuteDlCartesianPlot: unknown Y1 range style"); break;
		break;
  }
/* Y2 Axis definition */
  switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].axisStyle) {
	case LINEAR_AXIS:
		break;
	case LOG10_AXIS:
		XtSetArg(args[n],XtNxrtY2AxisLogarithmic,True); n++;
		break;
	default:
		medmPrintf(
		"\nexecuteDlCartesianPlot: unknown Y2 axis style"); break;
		break;
  }
  switch (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle) {
	case CHANNEL_RANGE:		/* handle as default until connected */
	case AUTO_SCALE_RANGE:
		XtSetArg(args[n],XtNxrtY2NumUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtY2TickUseDefault,True); n++;
		XtSetArg(args[n],XtNxrtY2PrecisionUseDefault,True); n++;
		break;
	case USER_SPECIFIED_RANGE:
		minF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].minRange;
		maxF.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].maxRange;
		tickF.fval = (maxF.fval - minF.fval)/4.0;
		XtSetArg(args[n],XtNxrtY2Min,minF.lval); n++;
		XtSetArg(args[n],XtNxrtY2Max,maxF.lval); n++;
		XtSetArg(args[n],XtNxrtY2Tick,tickF.lval); n++;
		XtSetArg(args[n],XtNxrtY2Num,tickF.lval); n++;
		sprintf(string,"%f",tickF.fval);
		k = strlen(string)-1;
		while (string[k] == '0') k--; /* strip off trailing zeroes */
		iPrec = k;
		while (string[k] != '.' && k >= 0) k--;
		iPrec = iPrec - k;
		XtSetArg(args[n],XtNxrtY2Precision,iPrec); n++;
		break;
	default:
		medmPrintf(
		"\nexecuteDlCartesianPlot: unknown Y2 range style"); break;
		break;
  }
  


  XtSetArg(args[n],XmNtraversalOn,False); n++;
/*
 * add the pointer to the CartesianPlot structure as userData to widget
 */
  XtSetArg(args[n], XmNuserData, (XtPointer) pcp); n++;
  XtSetArg(args[n], XtNxrtDoubleBuffer, True); n++;
  localWidget = XtCreateWidget("cartesianPlot",xtXrtGraphWidgetClass,
		displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] = localWidget;

/* add button press handlers */
  XtAddEventHandler(localWidget,ButtonPressMask,False,handleButtonPress,
	(XtPointer)displayInfo);

  XtManageChild(localWidget);

}

void executeDlCartesianPlot(DisplayInfo *displayInfo,
			DlCartesianPlot *dlCartesianPlot, Boolean dummy) {
  displayInfo->useDynamicAttribute = FALSE;

  if (displayInfo->traversalMode == DL_EXECUTE) {
    cartesianPlotCreateRunTimeInstance(displayInfo, dlCartesianPlot);
  } else if (displayInfo->traversalMode == DL_EDIT) {
    cartesianPlotCreateEditInstance(displayInfo, dlCartesianPlot);
  }

}

static void cartesianPlotUpdateGraphicalInfoCb(XtPointer cd) {
  Record *pd = (Record *) cd;
  XYTrace *pt = (XYTrace *) pd->clientData;
  CartesianPlot *pcp = pt->cartesianPlot;
  DisplayInfo *displayInfo = pcp->updateTask->displayInfo;
  DlCartesianPlot *dlCartesianPlot = pcp->dlCartesianPlot;
  chid xChid, yChid;
  XColor xColors[MAX_TRACES];
  char rgb[MAX_TRACES][16];
  /* xrtData1 for 1st trace, xrtData2 for 2nd -> nTraces */
  XrtData *xrtData1, *xrtData2;
  XrtDataStyle myds1[MAX_TRACES], myds2[MAX_TRACES];
  float minX, maxX, minY, maxY, minY2, maxY2;
  XcVType minXF, maxXF, minYF, maxYF, minY2F, maxY2F, tickF;
  char string[24];
  int iPrec, kk;

  int maxElements = 0;
  int i,j;
  int n;
  Arg widgetArgs[20];

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! This is a temperory work around !!!!! */
  /* !!!!! for the reconnection.           !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  medmRecordAddGraphicalInfoCb(pd,NULL);

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! End work around                 !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  xrtData1 = xrtData2 = NULL;

  /* return until all channels get their graphical information */
  for (i = 0; i < pcp->nTraces; i++) {
    XYTrace *t = &(pcp->xyTrace[i]);
    if (t->recordX != NULL) {
      if (t->recordX->precision < 0) return;
    }

    if (t->recordY != NULL) {
      if (t->recordY->precision < 0) return;
    }
  }

  /* initialize min/max to some other-end-of-the-range values */
  minX = minY = minY2 = FLT_MAX;
  maxX = maxY = maxY2 = -0.99*FLT_MAX;

  for (i = 0; i < pcp->nTraces; i++)
    xColors[i].pixel = displayInfo->dlColormap[dlCartesianPlot->trace[i].data_clr];
  XQueryColors(XtDisplay(pcp->widget),cmap,xColors,pcp->nTraces);
  for (i = 0; i < pcp->nTraces; i++) {
    sprintf(rgb[i],"#%2.2x%2.2x%2.2x", xColors[i].red>>8,
                 xColors[i].green>>8, xColors[i].blue>>8);
  }
  /*
   * now create XrtData
   */
  /* allocate XrtData structures */

  /* find out the maximum number of element */
  maxElements = dlCartesianPlot->count;
  for (i = 0; i < pcp->nTraces; i++) {
    XYTrace *t = &(pcp->xyTrace[i]);
    if (t->recordX)
      maxElements = MAX(maxElements,(int)t->recordX->elementCount);
    if (t->recordY)
      maxElements = MAX(maxElements,(int)t->recordY->elementCount);
  }

  xrtData1 = XrtMakeData(XRT_GENERAL,1,maxElements,TRUE);
  /* now 2nd xrt data set if appropriate */
  if (pcp->nTraces > 1) {
    xrtData2 = XrtMakeData(XRT_GENERAL,pcp->nTraces-1,maxElements,TRUE);
  }

  /* loop over monitors, extracting information */
  for (i = 0; i < pcp->nTraces; i++) {
    XYTrace *t = &(pcp->xyTrace[i]);
    /*
     * now process data type (based on type (scalar or vector) of data)
     */
    if (t->recordX != NULL && t->recordY != NULL) {
      if ( t->recordX->elementCount > 1 && t->recordY->elementCount > 1 ) {
        t->type = CP_XYVector;
      } else
      if ( t->recordX->elementCount > 1 && t->recordY->elementCount <= 1 ) {
        t->type = CP_XVectorYScalar;
      } else
      if ( t->recordY->elementCount > 1 && t->recordX->elementCount <= 1 ) {
        t->type = CP_YVectorXScalar;
      } else
      if ( t->recordX->elementCount <= 1 && t->recordY->elementCount <= 1 ) {
        t->type = CP_XYScalar;
      }
      /* put initial data in */
      if (i <= 0) {
        xrtData1->g.data[i].npoints = 0;
        minY = MIN(minY,t->recordY->lopr);
        maxY = MAX(maxY,t->recordY->hopr);
      } else {
        xrtData2->g.data[i-1].npoints = 0;
        minY2 = MIN(minY2,t->recordY->lopr);
        maxY2 = MAX(maxY2,t->recordY->hopr);
      }
      minX = MIN(minX,t->recordX->lopr);
      maxX = MAX(maxX,t->recordX->hopr);

    } else
    if (t->recordX != NULL && t->recordY == NULL) {
      /* X channel - supporting scalar or waveform */
      if (t->recordX->elementCount > 1) {
        /* vector/waveform */
        t->type = CP_XVector;
        if (i <= 0) {
          for (j = 0; j < (int)t->recordX->elementCount; j++)
            xrtData1->g.data[i].yp[j]= (float)j;
          minY = MIN(minY,0.);
          maxY = MAX(maxY,(float)((int)t->recordX->elementCount-1));
        } else {
          for(j = 0; j < (int)t->recordX->elementCount; j++)
            xrtData2->g.data[i-1].yp[j]= (float)j;
          minY2 = MIN(minY2,0.);
          maxY2 = MAX(maxY2,(float)t->recordX->elementCount-1);
        }
        minX = MIN(minX,t->recordX->lopr);
        maxX = MAX(maxX,t->recordX->hopr);
      } else {
        /* scalar */
        t->type = CP_XScalar;
        if (i <= 0) {
          for(j = 0; j < dlCartesianPlot->count; j++)
            xrtData1->g.data[i].yp[j]= (float)j;
          xrtData1->g.data[i].xp[0]= (float)t->recordX->value;
          minY = MIN(minY,0.);
          maxY = MAX(maxY,(float)dlCartesianPlot->count);
        } else {
          for(j = 0; j < dlCartesianPlot->count; j++)
            xrtData2->g.data[i-1].yp[j]= (float)j;
          xrtData2->g.data[i-1].xp[0]= (float)t->recordX->value;
          minY2 = MIN(minY2,0.);
          maxY2 = MAX(maxY2,(float)dlCartesianPlot->count);
        }
        minX = MIN(minX,t->recordX->lopr);
        maxX = MAX(maxX,t->recordX->hopr);
      }
    } else
    if (t->recordX == NULL && t->recordY != NULL) {
      /* Y channel - supporting scalar or waveform */
      if (t->recordY->elementCount > 1) {
        /* vector/waveform */
        t->type = CP_YVector;
        if (i <= 0) {
          for(j = 0; j < t->recordY->elementCount; j++)
            xrtData1->g.data[i].xp[j]= (float)j;
          minY = MIN(minY,t->recordY->lopr);
          maxY = MAX(maxY,t->recordY->hopr);
        } else {
          for(j = 0; j < t->recordY->elementCount; j++)
            xrtData2->g.data[i-1].xp[j]= (float)j;
          minY2 = MIN(minY2,t->recordY->lopr);
          maxY2 = MAX(maxY2,t->recordY->hopr);
        }
        minX = MIN(minX,0.);
        maxX = MAX(maxX,(float)(t->recordY->elementCount-1));
      } else {
        /* scalar */
        t->type = CP_YScalar;
        if (i <= 0) {
          for(j = 0; j < dlCartesianPlot->count; j++)
            xrtData1->g.data[i].xp[j]= (float)j;
          xrtData1->g.data[i].yp[0]= (float)t->recordY->value;
          minY = MIN(minY,t->recordY->lopr);
          maxY = MAX(maxY,t->recordY->hopr);
        } else {
          for(j = 0; j < dlCartesianPlot->count; j++)
            xrtData2->g.data[i-1].xp[j]= (float)j;
          xrtData2->g.data[i-1].yp[0]= (float)t->recordY->value;
          minY2 = MIN(minY2,t->recordY->lopr);
          maxY2 = MAX(maxY2,t->recordY->hopr);
        }
        minX = MIN(minX,0.);
        maxX = MAX(maxX,(float)dlCartesianPlot->count);
      }
    } /* end for */
  }

  /*
   * now set XrtDataStyle
   */

  /* initialize XrtDataStyle array */
  for (i = 0; i < MAX_TRACES; i++) {
    /* there is really only going to be one myds1 in a graph */
    myds1[i].lpat = XRT_LPAT_SOLID;
    myds1[i].fpat = XRT_FPAT_SOLID;
    myds1[i].color = rgb[i];
    myds1[i].width = 1;
    myds1[i].point = XRT_POINT_DOT;
    myds1[i].pcolor = rgb[i];
    myds1[i].psize = MAX(2,dlCartesianPlot->object.height/70);
    myds1[i].res1 = 0;
    myds1[i].res1 = 0;

    myds2[i].lpat = XRT_LPAT_SOLID;
    myds2[i].fpat = XRT_FPAT_SOLID + i;
    myds2[i].color = rgb[i];
    myds2[i].width = 1;
    myds2[i].point = XRT_POINT_BOX+i;
    myds2[i].pcolor = rgb[i];
    myds2[i].psize = MAX(2,dlCartesianPlot->object.height/70);
    myds2[i].res1 = 0;
    myds2[i].res1 = 0;
  }


  /* loop over each trace and set xrtData attributes */
  for (i = 0; i < pcp->nTraces; i++) {
    switch(dlCartesianPlot->style) {
      case POINT_PLOT:
        if (i <= 0) {
          myds1[i].lpat = XRT_LPAT_NONE;
          myds1[i].fpat = XRT_FPAT_NONE;
          myds1[i].color = rgb[i];
          myds1[i].pcolor = rgb[i];
          myds1[i].psize = MAX(2,dlCartesianPlot->object.height/70);
          XrtSetNthDataStyle(pcp->widget,i,&myds1[i]);
        } else {
          myds2[i-1].lpat = XRT_LPAT_NONE;
          myds2[i-1].fpat = XRT_FPAT_NONE;
          myds2[i-1].color = rgb[i];
          myds2[i-1].pcolor = rgb[i];
          myds2[i-1].psize = MAX(2, dlCartesianPlot->object.height/70);
          XrtSetNthDataStyle2(pcp->widget, i-1,&myds2[i-1]);
        }
        break;
      case LINE_PLOT:
        if (i <= 0) {
          myds1[i].fpat = XRT_FPAT_NONE;
          myds1[i].color = rgb[i];
          myds1[i].pcolor = rgb[i];
          myds1[i].psize = MAX(2, dlCartesianPlot->object.height/70);
          XrtSetNthDataStyle(pcp->widget,i,&myds1[i]);
        } else {
          myds2[i-1].fpat = XRT_FPAT_NONE;
          myds2[i-1].color = rgb[i];
          myds2[i-1].pcolor = rgb[i];
          myds2[i-1].psize = MAX(2, dlCartesianPlot->object.height/70);
          XrtSetNthDataStyle2(pcp->widget, i-1,&myds2[i-1]);
        }
        break;
      case FILL_UNDER_PLOT:
        if (i <= 0) {
          myds1[i].color = rgb[i];
          myds1[i].pcolor = rgb[i];
          myds1[i].psize = MAX(2, dlCartesianPlot->object.height/70);
          XrtSetNthDataStyle(pcp->widget,i,&myds1[i]);
        } else {
          myds2[i-1].color = rgb[i];
          myds2[i-1].pcolor = rgb[i];
          myds2[i-1].psize = MAX(2, dlCartesianPlot->object.height/70);
          XrtSetNthDataStyle2(pcp->widget, i-1,&myds2[i-1]);
        }
        break;
    }
  }

  /*
   * now register event handlers for vectors of channels/monitors
   */
  for (i = 0; i < pcp->nTraces; i++) {
    XYTrace *t = &(pcp->xyTrace[i]);
    if ( i <= 0) {
      t->xrtData = xrtData1;
      t->trace = i;
    } else {
      t->xrtData = xrtData2;
      t->trace = i-1;
    }
  }


  /* fill in connect-time channel-based range specifications - NB:
   *  this is different than the min/max stored in the display element
   */
  pcp->axisRange[X_AXIS_ELEMENT].axisMin = minX;
  pcp->axisRange[X_AXIS_ELEMENT].axisMax = maxX;
  pcp->axisRange[Y1_AXIS_ELEMENT].axisMin = minY;
  pcp->axisRange[Y1_AXIS_ELEMENT].axisMax = maxY;
  pcp->axisRange[Y2_AXIS_ELEMENT].axisMin = minY2;
  pcp->axisRange[Y2_AXIS_ELEMENT].axisMax = maxY2;
  for (kk = X_AXIS_ELEMENT; kk <= Y2_AXIS_ELEMENT; kk++) {
    if (dlCartesianPlot->axis[kk].rangeStyle==CHANNEL_RANGE) {
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
  if (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle == CHANNEL_RANGE) {

    XtSetArg(widgetArgs[n],XtNxrtXMin,minXF.lval); n++;
    XtSetArg(widgetArgs[n],XtNxrtXMax,maxXF.lval); n++;

  } else
  if (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle
                                        == USER_SPECIFIED_RANGE) {
    int k;
    minXF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].minRange;
    maxXF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].maxRange;

    tickF.fval = (maxXF.fval - minXF.fval)/4.0;
    XtSetArg(widgetArgs[n],XtNxrtXMin,minXF.lval); n++;
    XtSetArg(widgetArgs[n],XtNxrtXMax,maxXF.lval); n++;
    XtSetArg(widgetArgs[n],XtNxrtXTick,tickF.lval); n++;
    XtSetArg(widgetArgs[n],XtNxrtXNum,tickF.lval); n++;
    sprintf(string,"%f",tickF.fval);
    k = strlen(string)-1;
    while (string[k] == '0') k--; /* strip off trailing zeroes */
    iPrec = k;
    while (string[k] != '.' && k >= 0) k--;
    iPrec = iPrec - k;
    XtSetArg(widgetArgs[n],XtNxrtXPrecision,iPrec); n++;

  }

  if (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle
                                        == CHANNEL_RANGE) {

    XtSetArg(widgetArgs[n],XtNxrtYMin,minYF.lval); n++;
    XtSetArg(widgetArgs[n],XtNxrtYMax,maxYF.lval); n++;

  } else
  if (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle
                                        == USER_SPECIFIED_RANGE) {

    int k;
    minYF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].minRange;
    maxYF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange;

    tickF.fval = (maxYF.fval - minYF.fval)/4.0;
    XtSetArg(widgetArgs[n],XtNxrtYMin,minYF.lval); n++;
    XtSetArg(widgetArgs[n],XtNxrtYMax,maxYF.lval); n++;
    XtSetArg(widgetArgs[n],XtNxrtYTick,tickF.lval); n++;
    XtSetArg(widgetArgs[n],XtNxrtYNum,tickF.lval); n++;
    sprintf(string,"%f",tickF.fval);
    k = strlen(string)-1;
    while (string[k] == '0') k--; /* strip off trailing zeroes */
    iPrec = k;
    while (string[k] != '.' && k >= 0) k--;
      iPrec = iPrec - k;
      XtSetArg(widgetArgs[n],XtNxrtYPrecision,iPrec); n++;

  }

  if (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle
                                        == CHANNEL_RANGE) {
    XtSetArg(widgetArgs[n],XtNxrtY2Min,minY2F.lval); n++;
    XtSetArg(widgetArgs[n],XtNxrtY2Max,maxY2F.lval); n++;

  } else
  if (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle
                                        == USER_SPECIFIED_RANGE) {

    int k;
    minY2F.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].minRange;
    maxY2F.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].maxRange;

    tickF.fval = (maxY2F.fval - minY2F.fval)/4.0;
    XtSetArg(widgetArgs[n],XtNxrtY2Min,minY2F.lval); n++;
    XtSetArg(widgetArgs[n],XtNxrtY2Max,maxY2F.lval); n++;
    XtSetArg(widgetArgs[n],XtNxrtY2Tick,tickF.lval); n++;
    XtSetArg(widgetArgs[n],XtNxrtY2Num,tickF.lval); n++;
    sprintf(string,"%f",tickF.fval);
    k = strlen(string)-1;
    while (string[k] == '0') k--; /* strip off trailing zeroes */
    iPrec = k;
    while (string[k] != '.' && k >= 0) k--;
    iPrec = iPrec - k;
    XtSetArg(widgetArgs[n],XtNxrtY2Precision,iPrec); n++;

  }


  XtSetValues(pcp->widget,widgetArgs,n);

  /* add destroy callback */
  /* not freeing monitorData here, rather using monitorData for
     it's information */
  pcp->xrtData1 = xrtData1;
  pcp->xrtData2 = xrtData2;
#if 0
  XtAddCallback(pcp->widget,XmNdestroyCallback,
          (XtCallbackProc)monitorDestroy, (XtPointer)pcp);
#endif
  cartesianPlotUpdateScreenFirstTime(cd);
}

void cartesianPlotUpdateTrace(XtPointer cd) {
  Record *pd = (Record *) cd;
  XYTrace *pt = (XYTrace *) pd->clientData;
  CartesianPlot *pcp = pt->cartesianPlot;
  DlCartesianPlot *dlCartesianPlot = pcp->dlCartesianPlot;
  int count, i, j;
  Arg args[20];

  switch(pt->type) {
  case CP_XYScalar:
    /* X: x & y channels specified - scalars, up to dlCartesianPlot->count pairs */
    count = pt->xrtData->g.data[pt->trace].npoints;
    if (count <= dlCartesianPlot->count-1) {
      pt->xrtData->g.data[pt->trace].xp[count] = (float) pt->recordX->value;
      pt->xrtData->g.data[pt->trace].yp[count] = (float) pt->recordY->value;
      pt->xrtData->g.data[pt->trace].npoints++;
    } else
    if (count > dlCartesianPlot->count-1) {
      if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
	/* don't update */
      } else
      if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
        /* shift everybody down one, add at end */
        int j;
        for (j = 1; j < dlCartesianPlot->count; j++) {
          pt->xrtData->g.data[pt->trace].xp[j-1] =
                   pt->xrtData->g.data[pt->trace].xp[j];
          pt->xrtData->g.data[pt->trace].yp[j-1] =
                   pt->xrtData->g.data[pt->trace].yp[j];
        }
        pt->xrtData->g.data[pt->trace].xp[j-1] = (float) pt->recordX->value;
        pt->xrtData->g.data[pt->trace].yp[j-1] = (float) pt->recordY->value;
        pt->xrtData->g.data[pt->trace].npoints = dlCartesianPlot->count;
      }
    }
    break;

  case CP_XScalar:
    /* x channel scalar, up to dlCartesianPlot->count pairs */
    count = pt->xrtData->g.data[pt->trace].npoints;
    if (count <= dlCartesianPlot->count-1) {
      pt->xrtData->g.data[pt->trace].xp[count] = (float) pt->recordX->value;
      pt->xrtData->g.data[pt->trace].yp[count] = (float) count;
      pt->xrtData->g.data[pt->trace].npoints++;
    } else
    if (count > dlCartesianPlot->count-1) {
      if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
        /* don't update */
      } else
      if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
        /* shift everybody down one, add at end */
	int j;
        for (j = 1; j < dlCartesianPlot->count; j++) {
          pt->xrtData->g.data[pt->trace].xp[j-1] =
                   pt->xrtData->g.data[pt->trace].xp[j];
          pt->xrtData->g.data[pt->trace].yp[j-1] =
                   (float) (j-1);
        }
        pt->xrtData->g.data[pt->trace].xp[j-1] = (float) pt->recordX->value;
        pt->xrtData->g.data[pt->trace].yp[j-1] = (float) (j-1);
      }
    }
    break;


  case CP_XVector:
    /* x channel vector, ca_element_count(chid) elements */
    switch(pt->recordX->dataType) {
      case DBF_STRING:
      {
        for (j = 0; j < pt->recordX->elementCount; j++) {
          pt->xrtData->g.data[pt->trace].xp[j] = 0.0;
          pt->xrtData->g.data[pt->trace].yp[j] = (float) j;
        }
        break;
      }
      case DBF_INT:
      {
        short *pShort = (short *) pt->recordX->array;
        for (j = 0; j < pt->recordX->elementCount; j++) {
          pt->xrtData->g.data[pt->trace].xp[j] = (float) pShort[j];
          pt->xrtData->g.data[pt->trace].yp[j] = (float) j;
        }
        break;
      }
      case DBF_FLOAT:
      {
        float *pFloat = (float *) pt->recordX->array;
        for (j = 0; j < pt->recordX->elementCount; j++) {
          pt->xrtData->g.data[pt->trace].xp[j] = (float) pFloat[j];
          pt->xrtData->g.data[pt->trace].yp[j] = (float) j;
        }
        break;
      }
      case DBF_ENUM:
      {
        unsigned short *pUShort = (unsigned short *) pt->recordX->array;
        for (j = 0; j < pt->recordX->elementCount; j++) {
          pt->xrtData->g.data[pt->trace].xp[j] = (float) pUShort[j];
          pt->xrtData->g.data[pt->trace].yp[j] = (float) j;
        }
        break;
      }
      case DBF_CHAR:
      {
        unsigned char *ptar = (unsigned char *) pt->recordX->array;
        for (j = 0; j < pt->recordX->elementCount; j++) {
          pt->xrtData->g.data[pt->trace].xp[j] = (float) ptar[j];
          pt->xrtData->g.data[pt->trace].yp[j] = (float) j;
        }
        break;
      }
      case DBF_LONG:
      {
        dbr_long_t *pLong = (dbr_long_t *) pt->recordX->array;
        for (j = 0; j < pt->recordX->elementCount; j++) {
          pt->xrtData->g.data[pt->trace].xp[j] = (float) pLong[j];
          pt->xrtData->g.data[pt->trace].yp[j] = (float) j;
        }
        break;
      }
      case DBF_DOUBLE:
      {
        double *pDouble = (double *) pt->recordX->array;
        for (j = 0; j < pt->recordX->elementCount; j++) {
          pt->xrtData->g.data[pt->trace].xp[j] = (float) pDouble[j];
          pt->xrtData->g.data[pt->trace].yp[j] = (float) j;
        }
        break;
      }
    }
    pt->xrtData->g.data[pt->trace].npoints = pt->recordX->elementCount;
    break;

  case CP_YScalar:
    /* y channel scalar, up to dlCartesianPlot->count pairs */
    count = pt->xrtData->g.data[pt->trace].npoints;
    if (count <= dlCartesianPlot->count-1) {
      pt->xrtData->g.data[pt->trace].yp[count] = (float) pt->recordY->value;
      pt->xrtData->g.data[pt->trace].xp[count] = (float) count;
      pt->xrtData->g.data[pt->trace].npoints++;
    } else
    if (count > dlCartesianPlot->count-1) {
      if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
        /* Don't update */
      } else
      if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
        /* shift everybody down one, add at end */
	int j;
        for (j = 1; j < dlCartesianPlot->count; j++) {
          pt->xrtData->g.data[pt->trace].yp[j-1] =
              pt->xrtData->g.data[pt->trace].yp[j];
          pt->xrtData->g.data[pt->trace].xp[j-1] = (float) (j-1);
        }
        pt->xrtData->g.data[pt->trace].yp[j-1] = (float) pt->recordY->value;
        pt->xrtData->g.data[pt->trace].xp[j-1] = (float) (j-1);
      }
    }
    break;

  case CP_YVector:
    /* plot first "count" elements of vector per dlCartesianPlot*/
    switch(pt->recordY->dataType) {
    case DBF_STRING:
    {
      for (j = 0; j < pt->recordY->elementCount; j++) {
        pt->xrtData->g.data[pt->trace].yp[j] = 0.0;
        pt->xrtData->g.data[pt->trace].xp[j] = (float) j;
      }
      break;
    }
    case DBF_INT:
    {
      short *pShort = (short *) pt->recordY->array;
      for (j = 0; j < pt->recordY->elementCount; j++) {
        pt->xrtData->g.data[pt->trace].yp[j] = (float) pShort[j];
        pt->xrtData->g.data[pt->trace].xp[j] = (float) j;
      }
      break;
    }
    case DBF_FLOAT:
    {
      float *pFloat = (float *) pt->recordY->array;
      for (j = 0; j < pt->recordY->elementCount; j++) {
        pt->xrtData->g.data[pt->trace].yp[j] = (float) pFloat[j];
        pt->xrtData->g.data[pt->trace].xp[j] = (float) j;
      }
      break;
    }
    case DBF_ENUM:
    {
      unsigned short *pUShort = (unsigned short *) pt->recordY->array;
      for (j = 0; j < pt->recordY->elementCount; j++) {
        pt->xrtData->g.data[pt->trace].yp[j] = (float) pUShort[j];
        pt->xrtData->g.data[pt->trace].xp[j] = (float) j;
      }
      break;
    }
    case DBF_CHAR:
    {
      unsigned char *ptar = (unsigned char *) pt->recordY->array;
      for (j = 0; j < pt->recordY->elementCount; j++) {
        pt->xrtData->g.data[pt->trace].yp[j] = (float) ptar[j];
        pt->xrtData->g.data[pt->trace].xp[j] = (float) j;
      }
      break;
    }
    case DBF_LONG:
    {
      dbr_long_t *pLong = (dbr_long_t *) pt->recordY->array;
      for (j = 0; j < pt->recordY->elementCount; j++) {
        pt->xrtData->g.data[pt->trace].yp[j] = (float) pLong[j];
        pt->xrtData->g.data[pt->trace].xp[j] = (float) j;
      }
      break;
    }
    case DBF_DOUBLE:
    {
      double *pDouble = (double *) pt->recordY->array;
      for (j = 0; j < pt->recordY->elementCount; j++) {
        pt->xrtData->g.data[pt->trace].yp[j] = (float) pDouble[j];
        pt->xrtData->g.data[pt->trace].xp[j] = (float) j;
      }
      break;
    }
  }
  pt->xrtData->g.data[pt->trace].npoints = pt->recordY->elementCount;
  break;


  case CP_XVectorYScalar:
    pt->xrtData->g.data[pt->trace].npoints = pt->recordX->elementCount;
    if (pd == pt->recordX) {
      /* plot first "count" elements of vector per dlCartesianPlot*/
      switch(pt->recordX->dataType) {
        case DBF_STRING:
        {
          for (j = 0; j < pt->recordX->elementCount; j++) {
            pt->xrtData->g.data[pt->trace].xp[j] = 0.0;
          }
          break;
        }
        case DBF_INT:
        {
          short *pShort = (short *) pt->recordX->array;
          for (j = 0; j < pt->recordX->elementCount; j++) {
            pt->xrtData->g.data[pt->trace].xp[j] = (float) pShort[j];
          }
          break;
        }
        case DBF_FLOAT:
        {
          float *pFloat = (float *) pt->recordX->array;
          for (j = 0; j < pt->recordX->elementCount; j++) {
            pt->xrtData->g.data[pt->trace].xp[j] = pFloat[j];
          }
          break;
        }
        case DBF_ENUM:
        {
          unsigned short *pUShort = (unsigned short *) pt->recordX->array;
          for (j = 0; j < pt->recordX->elementCount; j++) {
            pt->xrtData->g.data[pt->trace].xp[j] = (float) pUShort[j];
          }
          break;
        }
        case DBF_CHAR:
        {
          unsigned char *ptar = (unsigned char *) pt->recordX->array;
          for (j = 0; j < pt->recordX->elementCount; j++) {
            pt->xrtData->g.data[pt->trace].xp[j] = (float) ptar[j];
          }
          break;
        }
        case DBF_LONG:
        {
          dbr_long_t *pLong = (dbr_long_t *) pt->recordX->array;
          for (j = 0; j < pt->recordX->elementCount; j++) {
            pt->xrtData->g.data[pt->trace].xp[j] = (float) pLong[j];
          }
          break;
        }
        case DBF_DOUBLE:
        {
          double *pDouble = (double *) pt->recordX->array;
          for (j = 0; j < pt->recordX->elementCount; j++) {
            pt->xrtData->g.data[pt->trace].xp[j] = (double) pDouble[j];
          }
          break;
        }
      }
    } else 
    if (pd == pt->recordY) {
      for (j = 0; j < pt->recordX->elementCount; j++) {
        pt->xrtData->g.data[pt->trace].yp[j] = (float) pt->recordY->value;
      }
    }
    break;

  case CP_YVectorXScalar:
    pt->xrtData->g.data[pt->trace].npoints = pt->recordY->elementCount;
    if (pd == pt->recordY) {
      /* plot first "count" elements of vector per dlCartesianPlot*/
      switch(pt->recordY->dataType) {
        case DBF_STRING:
        {
          for (j = 0; j < pt->recordY->elementCount; j++) {
            pt->xrtData->g.data[pt->trace].yp[j] = 0.0;
          }
          break;
        }
        case DBF_INT:
        {
          short *pShort = (short *) pt->recordY->array;
          for (j = 0; j < pt->recordY->elementCount; j++) {
            pt->xrtData->g.data[pt->trace].yp[j] = (float) pShort[j];
          }
          break;
        }
        case DBF_FLOAT:
        {
          float *pFloat = (float *) pt->recordY->array;
          for (j = 0; j < pt->recordY->elementCount; j++) {
            pt->xrtData->g.data[pt->trace].yp[j] = pFloat[j];
          }
          break;
        }
        case DBF_ENUM:
        {
          unsigned short *pUShort = (unsigned short *) pt->recordY->array;
          for (j = 0; j < pt->recordY->elementCount; j++) {
            pt->xrtData->g.data[pt->trace].yp[j] = (float) pUShort[j];
          }
          break;
        }
        case DBF_CHAR:
        {
          unsigned char *ptar = (unsigned char *) pt->recordY->array;
          for (j = 0; j < pt->recordY->elementCount; j++) {
            pt->xrtData->g.data[pt->trace].yp[j] = (float) ptar[j];
          }
          break;
        }
        case DBF_LONG:
        {
          dbr_long_t *pLong = (dbr_long_t *) pt->recordY->array;
          for (j = 0; j < pt->recordY->elementCount; j++) {
            pt->xrtData->g.data[pt->trace].yp[j] = (float) pLong[j];
          }
          break;
        }
        case DBF_DOUBLE:
          {
          double *pDouble = (double *) pt->recordY->array;
          for (j = 0; j < pt->recordY->elementCount; j++) {
            pt->xrtData->g.data[pt->trace].yp[j] = (double) pDouble[j];
          }
          break;
        }
      }
    } else 
    if (pd == pt->recordX) {
      for (j = 0; j < pt->recordY->elementCount; j++) {
        pt->xrtData->g.data[pt->trace].xp[j] = (float) pt->recordX->value;
      }
    }
    break;

  case CP_XYVector:
    pt->xrtData->g.data[pt->trace].npoints =
      MIN(pt->recordX->elementCount, pt->recordY->elementCount);

    if (pd == pt->recordX) {
      int count = pt->xrtData->g.data[pt->trace].npoints;
      switch(pt->recordX->dataType) {
        case DBF_STRING:
          for (j = 0; j < count; j++) {
            pt->xrtData->g.data[pt->trace].xp[j] = 0.0;
          }
          break;
        case DBF_INT:
        {
          short *pShort = (short *) pt->recordX->array;
          for (j = 0; j < count; j++) {
            pt->xrtData->g.data[pt->trace].xp[j] = (float) pShort[j];
          }
          break;
        }
        case DBF_FLOAT:
        {
          float *pFloat = (float *) pt->recordX->array;
          for (j = 0; j < count; j++) {
            pt->xrtData->g.data[pt->trace].xp[j] = pFloat[j];
          }
          break;
        }
        case DBF_ENUM:
        {
          unsigned short *pUShort = (unsigned short *) pt->recordX->array;
          for (j = 0; j < count; j++) {
            pt->xrtData->g.data[pt->trace].xp[j] = (float) pUShort[j];
          }
          break;
        }
        case DBF_CHAR:
        {
          unsigned char *ptar = (unsigned char *) pt->recordX->array;
          for (j = 0; j < count; j++) {
            pt->xrtData->g.data[pt->trace].xp[j] = (float) ptar[j];
          }
          break;
        }
        case DBF_LONG:
        {
          dbr_long_t *pLong = (dbr_long_t *) pt->recordX->array;
          for (j = 0; j < count; j++) {
            pt->xrtData->g.data[pt->trace].xp[j] = (float) pLong[j];
          }
          break;
        }
        case DBF_DOUBLE:
        {
          double *pDouble = (double *) pt->recordX->array;
          for (j = 0; j < count; j++) {
            pt->xrtData->g.data[pt->trace].xp[j] = (float) pDouble[j];
          }
          break;
        }
      }
    } else
    if (pd == pt->recordY) {
      int count = pt->xrtData->g.data[pt->trace].npoints;
      switch(pt->recordY->dataType) {
        case DBF_STRING:
          for (j = 0; j < count; j++) {
            pt->xrtData->g.data[pt->trace].yp[j] = 0.0;
          }
          break;
        case DBF_INT:
        {
          short *pShort = (short *) pt->recordY->array;
          for (j = 0; j < count; j++) {
            pt->xrtData->g.data[pt->trace].yp[j] = (float) pShort[j];
          }
          break;
        }
        case DBF_FLOAT:
        {
          float *pFloat = (float *) pt->recordY->array;
          for (j = 0; j < count; j++) {
            pt->xrtData->g.data[pt->trace].yp[j] = pFloat[j];
          }
          break;
        }
        case DBF_ENUM:
        {
          unsigned short *pUShort = (unsigned short *) pt->recordY->array;
          for (j = 0; j < count; j++) {
            pt->xrtData->g.data[pt->trace].yp[j] = (float) pUShort[j];
          }
          break;
        }
        case DBF_CHAR:
        {
          unsigned char *ptar = (unsigned char *) pt->recordY->array;
          for (j = 0; j < count; j++) {
            pt->xrtData->g.data[pt->trace].yp[j] = (float) ptar[j];
          }
          break;
        }
        case DBF_LONG:
        {
          dbr_long_t *pLong = (dbr_long_t *) pt->recordY->array;
          for (j = 0; j < count; j++) {
            pt->xrtData->g.data[pt->trace].yp[j] = (float) pLong[j];
          }
          break;
        }
        case DBF_DOUBLE:
        {
          double *pDouble = (double *) pt->recordY->array;
          for (j = 0; j < count; j++) {
            pt->xrtData->g.data[pt->trace].yp[j] = (float) pDouble[j];
          }
          break;
        }
      }
    }
    break;
  default:
      medmPrintf("\ncartesianPlotUpdateTrace: unhandled CP case\n");
  }
}

void cartesianPlotUpdateScreenFirstTime(XtPointer cd) {
  Record *pd = (Record *) cd;
  XYTrace *pt = (XYTrace *) pd->clientData;
  CartesianPlot *pcp = pt->cartesianPlot;
  int count, i;
  unsigned short j;
  Arg args[20];
  Boolean clearDataSet1 = True;
  Boolean clearDataSet2 = True;

  /* return until all channels get their graphical information */
  for (i = 0; i < pcp->nTraces; i++) {
    XYTrace *t = &(pcp->xyTrace[i]);
    if ((t->recordX) || (t->recordY)) {
      if (t->xrtData == NULL) return;
      if ((t->recordX) &&
         ((!t->recordX->array) || (t->recordX->precision < 0))) return;
      if ((t->recordY) &&
         ((!t->recordY->array) || (t->recordY->precision < 0))) return;
    }
  }
  if (pcp->triggerCh.recordX) {
    if (pcp->triggerCh.recordX->precision < 0) return;
  }
  if (pcp->eraseCh.recordX) {
    if (pcp->eraseCh.recordX->precision < 0) return;
  }
  /* draw all plots once */
  for (i = 0; i < pcp->nTraces; i++) {
    XYTrace *t = &(pcp->xyTrace[i]);
    if (t->recordX) {
      cartesianPlotUpdateTrace((XtPointer)t->recordX);
      if (t->type == CP_XYVector)
        cartesianPlotUpdateTrace((XtPointer)t->recordY);
    } else
    if (t->recordY) {
      cartesianPlotUpdateTrace((XtPointer)t->recordY);
    } else {
      continue;
    }
    if (t->xrtData == pcp->xrtData1)
      XtVaSetValues(pcp->widget,XtNxrtData,t->xrtData,NULL);
    else
    if (t->xrtData == pcp->xrtData2)
      XtVaSetValues(pcp->widget,XtNxrtData2,t->xrtData,NULL);
  }
  /* erase the plot */
  if (pcp->eraseCh.recordX) {
    Record *pr = pcp->eraseCh.recordX;
    if (((pr->value == 0.0) && (pcp->eraseMode == ERASE_IF_ZERO))
      || ((pr->value != 0.0) && (pcp->eraseMode == ERASE_IF_NOT_ZERO))) {
      for (i = 0; i < pcp->nTraces; i++) {
        XYTrace *t = &(pcp->xyTrace[i]);
        if ((t->recordX) || (t->recordY)) {
          int n = t->xrtData->g.data[t->trace].npoints;
          if (n > 0) {
            if ((t->xrtData == pcp->xrtData1) && (clearDataSet1)) {
              XtVaSetValues(pcp->widget,XtNxrtData,nullData,NULL);
              clearDataSet1 = False;
            } else
            if ((t->xrtData == pcp->xrtData2) && (clearDataSet2)) {
              XtVaSetValues(pcp->widget,XtNxrtData2,nullData,NULL);
              clearDataSet2 = False;
            }
            t->xrtData->g.data[t->trace].npoints = 0;
          }
        } 
      }
    }
  }
  /* switch back to regular update routine */
  for (i = 0; i < pcp->nTraces; i++) {
    XYTrace *t = &(pcp->xyTrace[i]);
    if (t->recordX) {
      medmRecordAddUpdateValueCb(t->recordX,cartesianPlotUpdateValueCb);
    }
    if (t->recordY) {
      medmRecordAddUpdateValueCb(t->recordY,cartesianPlotUpdateValueCb);
    }
  }
  if (pcp->eraseCh.recordX) {
    medmRecordAddUpdateValueCb(pcp->eraseCh.recordX,cartesianPlotUpdateValueCb);
  }
  if (pcp->triggerCh.recordX) {
    medmRecordAddUpdateValueCb(pcp->triggerCh.recordX,cartesianPlotUpdateValueCb);
  }
  updateTaskMarkUpdate(pcp->updateTask);
}


void cartesianPlotUpdateValueCb(XtPointer cd) {
  Record *pd = (Record *) cd;
  XYTrace *pt = (XYTrace *) pd->clientData;
  CartesianPlot *pcp = pt->cartesianPlot;
  int count, i;
  unsigned short j;
  Arg args[20];

  /* if this is in trigger mode, and this update request is not from
   * the trigger, do not update.
   */

  /* if this is an erase channel, erase screen */
  if (pd == pcp->eraseCh.recordX) {
    Boolean clearDataSet1 = True;
    Boolean clearDataSet2 = True;
    XrtDataStyle style;

    /* not the right value, return */
    if (((pd->value == 0) && (pcp->eraseMode == ERASE_IF_NOT_ZERO))
	||((pd->value != 0) && (pcp->eraseMode == ERASE_IF_ZERO))) {
      return;
    }

    /* erase */
    for (i = 0; i < pcp->nTraces; i++) {
      XYTrace *t = &(pcp->xyTrace[i]);
      if ((t->recordX) || (t->recordY)) {
	int n = t->xrtData->g.data[t->trace].npoints;
        if (n > 0) {
	  if ((t->xrtData == pcp->xrtData1) && (clearDataSet1)) {
	    XtVaSetValues(pcp->widget,XtNxrtData,nullData,NULL);
	    clearDataSet1 = False;
            pcp->dirty1 = False;
	  } else
	  if ((t->xrtData == pcp->xrtData2) && (clearDataSet2)) {
	    XtVaSetValues(pcp->widget,XtNxrtData2,nullData,NULL);
	    clearDataSet2 = False;
            pcp->dirty2 = False;
	  }
	  t->xrtData->g.data[t->trace].npoints = 0;
	}
      }
    }
    updateTaskMarkUpdate(pcp->updateTask);
    return;
  }

  /* if there is a trigger channel, but this is not the one, return */
  if (pcp->triggerCh.recordX) {
    if (pd != pcp->triggerCh.recordX)
      return;
  }

  if (pcp->triggerCh.recordX) {
    /* trigger channel monitor, update appropriate plots */
    for (i = 0; i < pcp->nTraces; i++) {
      XYTrace *t = &(pcp->xyTrace[i]);
      if ((t->recordX == NULL) && (t->recordY == NULL)) continue;
      if ((t->recordX) && (t->xrtData)) {
        cartesianPlotUpdateTrace((XtPointer)t->recordX);
        if (t->type == CP_XYVector)
          cartesianPlotUpdateTrace((XtPointer)t->recordY);
        if (t->xrtData == pcp->xrtData1)
          pcp->dirty1 = True;
	else
	if (t->xrtData == pcp->xrtData2)
          pcp->dirty2 = True;
      } else
      if ((t->recordY) && (t->xrtData)) {
        cartesianPlotUpdateTrace((XtPointer)t->recordY);
	if (t->xrtData == pcp->xrtData1)
          pcp->dirty1 = True;
	else
	if (t->xrtData == pcp->xrtData2)
          pcp->dirty2 = True;
      }
    }
  } else {
    cartesianPlotUpdateTrace((XtPointer)pd);
    /* not in CP related to trigger channel, proceed as normal */
    if (pt->xrtData == pcp->xrtData1) {
      pcp->dirty1 = True;
    } else
    if (pt->xrtData == pcp->xrtData2) {
      pcp->dirty2 = True;
    } else
      medmPrintf("\ncartesianPlotUpdateScreen : illegal xrtDataSet specified\n");
  }
  updateTaskMarkUpdate(pcp->updateTask);
}

void cartesianPlotDestroyCb(XtPointer cd) {
  CartesianPlot *pcp = (CartesianPlot *) cd;
  if (executeTimeCartesianPlotWidget == pcp->widget) {
    executeTimeCartesianPlotWidget = NULL;
    XtSetSensitive(cartesianPlotAxisS,False);
  }
  if (pcp) {
    int i;
    for (i = 0; i < pcp->nTraces; i++) {
      Record *pr;
      if (pr = pcp->xyTrace[i].recordX) {
        medmDestroyRecord(pr);
      }
      if (pr = pcp->xyTrace[i].recordY) {
        medmDestroyRecord(pr);
      }
    }
    if (pcp->triggerCh.recordX)
      medmDestroyRecord(pcp->triggerCh.recordX);
    if (pcp->eraseCh.recordX)
      medmDestroyRecord(pcp->eraseCh.recordX);
    free(pcp);
  }
  return;
}

void cartesianPlotDraw(XtPointer cd) {
  CartesianPlot *pcp = (CartesianPlot *) cd;
  int count, i;
  Boolean connected = True;
  Boolean readAccess = True;

  /* check for connection */
  for (i = 0; i < pcp->nTraces; i++) {
    Record *pr;
    if (pr = pcp->xyTrace[i].recordX) {
      if (!pr->connected) {
	connected = False;
	break;
      } else
      if (!pr->readAccess)
        readAccess = False;
    }
    if (pr = pcp->xyTrace[i].recordY) {
      if (!pr->connected) {
	connected = False;
	break;
      } else
      if (!pr->readAccess)
        readAccess = False;
    }
  }

  if (pcp->triggerCh.recordX) {
    Record *pr = pcp->triggerCh.recordX;
    if (!pr->connected) {
      connected = False;
    } else
    if (!pr->readAccess)
      readAccess = False;
  }

  if (pcp->eraseCh.recordX) {
    Record *pr = pcp->eraseCh.recordX;
    if (!pr->connected) {
      connected = False;
    } else
    if (!pr->readAccess)
      readAccess = False;
  }

  if (connected) {
    if (readAccess) {
      if (pcp->widget) {
        if (pcp->dirty1) {
          pcp->dirty1 = False;
          XtVaSetValues(pcp->widget,XtNxrtData,pcp->xrtData1,NULL);
        }
        if (pcp->dirty2) {
          pcp->dirty2 = False;
          XtVaSetValues(pcp->widget,XtNxrtData2,pcp->xrtData2,NULL);
        }
        XtManageChild(pcp->widget);
      }
    } else {
      if (pcp->widget) {
        XtUnmanageChild(pcp->widget);
      }
      draw3DPane(pcp->updateTask,
          pcp->updateTask->displayInfo->dlColormap[pcp->dlCartesianPlot->plotcom.bclr]);
      draw3DQuestionMark(pcp->updateTask);
    }
  } else {
    if (pcp->widget)
      XtUnmanageChild(pcp->widget);
    drawWhiteRectangle(pcp->updateTask);
  }
}

static void cartesianPlotName(XtPointer cd, char **name, short *severity, int *count) {
  CartesianPlot *pcp = (CartesianPlot *) cd;
  int i, j;

  j = 0;
  for (i = 0; i < pcp->nTraces; i++) {
    XYTrace *pt = &(pcp->xyTrace[i]);
    if (pt->recordX) {
      name[j] = pt->recordX->name;
      severity[j] = pt->recordX->severity;
    } else {
      name[j] = NULL;
      severity[j] = NO_ALARM;
    }
    j++;
    if (pt->recordY) {
      name[j] = pt->recordY->name;
      severity[j] = pt->recordY->severity;
    } else {
      name[j] = NULL;
      severity[j] = NO_ALARM;
    }
    j++;
  }
  if (pcp->triggerCh.recordX) {
    name[j] = pcp->triggerCh.recordX->name;
    severity[j] = pcp->triggerCh.recordX->severity;
  } else {
    name[j] = NULL;
    severity[j] = NO_ALARM;
  }
  j++;
  if (pcp->eraseCh.recordX) {
    name[j] = pcp->eraseCh.recordX->name;
    severity[j] = pcp->eraseCh.recordX->severity;
  } else {
    name[j] = NULL;
    severity[j] = NO_ALARM;
  }
  j++;
  /* 200 means two columns */
  *count = j + 200;
}
