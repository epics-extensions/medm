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
 *
 *****************************************************************************
*/

#include "medm.h"
void cartesianPlotCreateRunTimeInstance(DisplayInfo *displayInfo,DlCartesianPlot *dlCartesianPlot);
void cartesianPlotCreateEditInstance(DisplayInfo *displayInfo,DlCartesianPlot *dlCartesianPlot);
static void cartesianPlotUpdateGraphicalInfoCb(Channel *pCh);
static void cartesianPlotUpdateScreen(Channel *pCh);
static void cartesianPlotUpdateValueCb(Channel *pCh);
static void cartesianPlotDestroyCb(Channel *pCh);

static XrtData *nullData = NULL;
void cartesianPlotCreateRunTimeInstance(DisplayInfo *displayInfo,
			DlCartesianPlot *dlCartesianPlot)
{
  Channel *pCh;
  CartesianPlotData *cartesianPlotData;
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

  if (!nullData) {
    nullData = XrtMakeData(XRT_GENERAL,1,1,TRUE);
    nullData->g.hole = 0.0;
    nullData->g.data[0].npoints = 1;
    nullData->g.data[0].xp[0] = 0.0;
    nullData->g.data[0].yp[0] = 0.0;
  }
  validTraces = 0;
  cartesianPlotData = (CartesianPlotData *)
				calloc(1,sizeof(CartesianPlotData));
  cartesianPlotData->xrtData1 = cartesianPlotData->xrtData2 = NULL;
  cartesianPlotData->triggerCh = (XtPointer) NULL;
  cartesianPlotData->eraseCh = (XtPointer) NULL;
  cartesianPlotData->eraseMode = dlCartesianPlot->eraseMode;

  for (i = 0; i < MAX_TRACES; i++) {
    validTrace = False;
    /* X data */
    if (dlCartesianPlot->trace[validTraces].xdata[0] != '\0') {
      pCh = allocateChannel(displayInfo);
      pCh->monitorType = DL_CartesianPlot;
      pCh->specifics = (XtPointer) dlCartesianPlot;
      pCh->backgroundColor = displayInfo->dlColormap[dlCartesianPlot->plotcom.bclr];

      pCh->updateChannelCb =  cartesianPlotUpdateValueCb;
      pCh->updateDataCb = cartesianPlotUpdateScreen;
      pCh->updateGraphicalInfoCb = cartesianPlotUpdateGraphicalInfoCb;
      pCh->destroyChannel = cartesianPlotDestroyCb;
      pCh->handleArray = True;

      SEVCHK(CA_BUILD_AND_CONNECT(dlCartesianPlot->trace[validTraces].xdata,
	       TYPENOTCONN,0,&(pCh->chid),NULL,medmConnectEventCb,pCh),
	       "executeDlCartesianPlot: error in CA_BUILD_AND_CONNECT");
      /* add pointer to this monitor data structure to the cartesian plot structure */
      cartesianPlotData->monitors[validTraces][0] = (XtPointer) pCh;
      validTrace = True;
    }
    /* Y data */
    if (dlCartesianPlot->trace[validTraces].ydata[0] != '\0') {
      pCh = allocateChannel(displayInfo);
      pCh->monitorType = DL_CartesianPlot;
      pCh->specifics = (XtPointer) dlCartesianPlot;
      pCh->backgroundColor = displayInfo->dlColormap[dlCartesianPlot->plotcom.bclr];

      pCh->updateChannelCb =  cartesianPlotUpdateValueCb;
      pCh->updateDataCb = cartesianPlotUpdateScreen;
      pCh->updateGraphicalInfoCb = cartesianPlotUpdateGraphicalInfoCb;
      pCh->destroyChannel = cartesianPlotDestroyCb;
      pCh->handleArray = True;

      SEVCHK(CA_BUILD_AND_CONNECT(dlCartesianPlot->trace[validTraces].ydata,
	       TYPENOTCONN, 0,&(pCh->chid),NULL,medmConnectEventCb,pCh),
	       "executeDlCartesianPlot: error in CA_BUILD_AND_CONNECT");
      /* add pointer to this monitor data structure to the cartesian plot structure */
      cartesianPlotData->monitors[validTraces][1] = (XtPointer) pCh;
      validTrace = True;
    }
    if (validTrace) validTraces++;
  }

  /* now add monitor for trigger channel if appropriate */
  if ((dlCartesianPlot->trigger[0] != '\0') && (validTraces > 0)) {
    pCh = allocateChannel(displayInfo);
    pCh->monitorType = DL_CartesianPlot;
    pCh->specifics = (XtPointer) dlCartesianPlot;
    pCh->backgroundColor = displayInfo->dlColormap[dlCartesianPlot->plotcom.bclr];

    pCh->updateChannelCb =  cartesianPlotUpdateValueCb;
    pCh->updateDataCb = cartesianPlotUpdateScreen;
    pCh->updateGraphicalInfoCb = NULL;
    pCh->destroyChannel = cartesianPlotDestroyCb;

    SEVCHK(CA_BUILD_AND_CONNECT(dlCartesianPlot->trigger, TYPENOTCONN,
	0,&(pCh->chid),NULL,medmConnectEventCb,pCh),
	"executeDlCartesianPlot: error in CA_BUILD_AND_CONNECT");
    /* add ptr. to trigger monitor data structure to the cartesian plot structure */
    cartesianPlotData->triggerCh = (XtPointer) pCh;
  }

  /* now add monitor for erase channel if appropriate */
  if ((dlCartesianPlot->erase[0] != '\0') && (validTraces > 0)) {
    pCh = allocateChannel(displayInfo);
    pCh->monitorType = DL_CartesianPlot;
    pCh->specifics = (XtPointer) dlCartesianPlot;
    pCh->backgroundColor = displayInfo->dlColormap[dlCartesianPlot->plotcom.bclr];

    pCh->updateChannelCb =  cartesianPlotUpdateValueCb;
    pCh->updateDataCb = cartesianPlotUpdateScreen;
    pCh->updateGraphicalInfoCb = NULL;
    pCh->destroyChannel = cartesianPlotDestroyCb;

    SEVCHK(CA_BUILD_AND_CONNECT(dlCartesianPlot->erase, TYPENOTCONN,
        0,&(pCh->chid),NULL,medmConnectEventCb,pCh),
        "executeDlCartesianPlot: error in CA_BUILD_AND_CONNECT");
    /* add ptr. to trigger monitor data structure to the cartesian plot structure */
    cartesianPlotData->eraseCh = (XtPointer) pCh;
  }

  drawWhiteRectangle(pCh);

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
 * add the pointer to the CartesianPlotData structure as userData to widget
 */
  XtSetArg(args[n], XmNuserData, (XtPointer) cartesianPlotData); n++;
  XtSetArg(args[n], XtNxrtDoubleBuffer, True); n++;
  localWidget = XtCreateWidget("cartesianPlot",xtXrtGraphWidgetClass,
		displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] = localWidget;

/* record the number of traces in the cartesian plot */
  cartesianPlotData->nTraces = validTraces;

/* record the widget that the monitor structures belong to */
  if (cartesianPlotData->triggerCh)
    ((Channel *)cartesianPlotData->triggerCh)->self = localWidget;
  if (cartesianPlotData->eraseCh)
    ((Channel *)cartesianPlotData->eraseCh)->self = localWidget;
  for (i = 0; i < cartesianPlotData->nTraces; i++) {
    for (j = 0; j <= 1; j++) {
      if (pCh = (Channel *) cartesianPlotData->monitors[i][j]) 
        pCh->self = localWidget;
    }
  }
/* add in drag/drop translations */
  XtOverrideTranslations(localWidget,parsedTranslations);

}

void cartesianPlotCreateEditInstance(DisplayInfo *displayInfo,
			DlCartesianPlot *dlCartesianPlot)
{
  Channel *pCh, *monitorData,
	*triggerCh;
  CartesianPlotData *cartesianPlotData;
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

  cartesianPlotData = NULL;
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
 * add the pointer to the CartesianPlotData structure as userData to widget
 */
  XtSetArg(args[n], XmNuserData, (XtPointer) cartesianPlotData); n++;
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

static void cartesianPlotUpdateGraphicalInfoCb(Channel *pCh) {
  /* Cartesian Plot variables */
  DisplayInfo *displayInfo;
  CartesianPlotData *cartesianPlotData;
  DlCartesianPlot *dlCartesianPlot;
  chid xChid, yChid;
  XColor xColors[MAX_TRACES];
  char rgb[MAX_TRACES][16];
  /* xrtData1 for 1st trace, xrtData2 for 2nd -> nTraces */
  XrtData *xrtData1, *xrtData2;
  XrtDataStyle myds1[MAX_TRACES], myds2[MAX_TRACES];
  float minX, maxX, minY, maxY, minY2, maxY2;
  XcVType minXF, maxXF, minYF, maxYF, minY2F, maxY2F, tickF;
  char string[24];
  Channel *monitorData, *monitorDataX, *monitorDataY;
  int iPrec, kk;

  int maxElements;
  int i,j;
  int n;
  Arg widgetArgs[20];

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! This is a temperory work around !!!!! */
  /* !!!!! for the reconnection.           !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  pCh->updateGraphicalInfoCb = NULL;

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! End work around                 !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  xrtData1 = xrtData2 = NULL;

  XtVaGetValues(pCh->self,XmNuserData,(XtPointer)&cartesianPlotData,NULL);

  /* MDA - okay I hate to do this but the nesting of ifs is getting out of hand */
  if (cartesianPlotData == (CartesianPlotData *)NULL) {
    medmPostMsg("processMonitorGrGetCallback: cartesianPlotData == NULL, returning\n");
    return;
  }

  /* return until all channels get their graphical information */
  for (i = 0; i < cartesianPlotData->nTraces; i++) {
    Channel *tmpCh;
    tmpCh = (Channel *) cartesianPlotData->monitors[i][0];
    if (tmpCh != NULL) {
      if (tmpCh->info == NULL) return;
    }

    tmpCh = (Channel*) cartesianPlotData->monitors[i][1];
    if (tmpCh != NULL) {
      if (tmpCh->info == NULL) return;
    }
  }

  /* initialize min/max to some other-end-of-the-range values */
  minX = minY = minY2 = FLT_MAX;
  maxX = maxY = maxY2 = -0.99*FLT_MAX;

  dlCartesianPlot = (DlCartesianPlot *) pCh->specifics;
  for (i = 0; i < cartesianPlotData->nTraces; i++)
    xColors[i].pixel = pCh->displayInfo->dlColormap[dlCartesianPlot->trace[i].data_clr];
  XQueryColors(display,cmap,xColors,cartesianPlotData->nTraces);
  for (i = 0; i < cartesianPlotData->nTraces; i++) {
    sprintf(rgb[i],"#%2.2x%2.2x%2.2x", xColors[i].red>>8,
                 xColors[i].green>>8, xColors[i].blue>>8);
  }
  /*
   * now create XrtData
   */
  /* allocate XrtData structures */

  /* find out the maximum number of element */
  maxElements = dlCartesianPlot->count;
  for (i = 0; i < cartesianPlotData->nTraces; i++) {
    Channel *pChX = (Channel *) cartesianPlotData->monitors[i][0];
    Channel *pChY = (Channel *) cartesianPlotData->monitors[i][1];
    if (pChX)
      if (pChX->chid)
        maxElements = MAX(maxElements,(int)ca_element_count(pChX->chid));
    if (pChY)
      if (pChY->chid)
        maxElements = MAX(maxElements,(int)ca_element_count(pChY->chid));
  }

  xrtData1 = XrtMakeData(XRT_GENERAL,1,maxElements,TRUE);
  /* now 2nd xrt data set if appropriate */
  if (cartesianPlotData->nTraces > 1) {
    xrtData2 = XrtMakeData(XRT_GENERAL,cartesianPlotData->nTraces-1,maxElements,TRUE);
  }

  /* loop over monitors, extracting information */
  for (i = 0; i < cartesianPlotData->nTraces; i++) {
    /* trace i, X data */
    Channel *pChX = (Channel *) cartesianPlotData->monitors[i][0];
    /* trace i, Y data */
    Channel *pChY = (Channel *) cartesianPlotData->monitors[i][1];
    /*
     * now process data type (based on type (scalar or vector) of data)
     */
    if (pChX != NULL && pChY != NULL) {
      if ( ca_element_count(pChX->chid) > 1 && ca_element_count(pChY->chid) > 1 ) {
        pChX->xyChannelType = CP_XYVectorX;
        pChY->xyChannelType = CP_XYVectorY;
      } else
      if ( ca_element_count(pChX->chid) > 1 && ca_element_count(pChY->chid) <= 1 ) {
        pChX->xyChannelType = CP_XVectorYScalarX;
        pChY->xyChannelType = CP_XVectorYScalarY;
      } else
      if ( ca_element_count(pChY->chid) > 1 && ca_element_count(pChX->chid) <= 1 ) {
        pChX->xyChannelType = CP_YVectorXScalarX;
        pChY->xyChannelType = CP_YVectorXScalarY;
      } else
      if ( ca_element_count(pChX->chid) <= 1 && ca_element_count(pChY->chid) <= 1 ) {
        pChX->xyChannelType = CP_XYScalarX;
        pChY->xyChannelType = CP_XYScalarY;
      }

      pChX->other = pChY;
      pChY->other = pChX;
      /* put initial data in */
      if (i <= 0) {
        xrtData1->g.data[i].npoints = 0;
        minY = MIN(minY,pChY->lopr);
        maxY = MAX(maxY,pChY->hopr);
      } else {
        xrtData2->g.data[i-1].npoints = 0;
        minY2 = MIN(minY2,pChY->lopr);
        maxY2 = MAX(maxY2,pChY->hopr);
      }
      minX = MIN(minX,pChX->lopr);
      maxX = MAX(maxX,pChX->hopr);

    } else
    if (pChX != NULL && pChY == NULL) {
      /* X channel - supporting scalar or waveform */
      if (ca_element_count(pChX->chid) > 1) {
        /* vector/waveform */
        pChX->xyChannelType = CP_XVector;
        if (i <= 0) {
          for (j = 0; j < (int)ca_element_count(pChX->chid); j++)
            xrtData1->g.data[i].yp[j]= (float)j;
          minY = MIN(minY,0.);
          maxY = MAX(maxY,(float)((int)ca_element_count(pChX->chid)-1));
        } else {
          for(j = 0; j < (int)ca_element_count(pChX->chid); j++)
            xrtData2->g.data[i-1].yp[j]= (float)j;
          minY2 = MIN(minY2,0.);
          maxY2 = MAX(maxY2,(float)ca_element_count(pChX->chid));
        }
        minX = MIN(minX,pChX->lopr);
        maxX = MAX(maxX,pChX->hopr);
      } else {
        /* scalar */
        pChX->xyChannelType = CP_XScalar;
        if (i <= 0) {
          for(j = 0; j < dlCartesianPlot->count; j++)
            xrtData1->g.data[i].yp[j]= (float)j;
          xrtData1->g.data[i].xp[0]= (float)pChX->value;
          minY = MIN(minY,0.);
          maxY = MAX(maxY,(float)dlCartesianPlot->count);
        } else {
          for(j = 0; j < dlCartesianPlot->count; j++)
            xrtData2->g.data[i-1].yp[j]= (float)j;
          xrtData2->g.data[i-1].xp[0]= (float)pChX->value;
          minY2 = MIN(minY2,0.);
          maxY2 = MAX(maxY2,(float)dlCartesianPlot->count);
        }
        minX = MIN(minX,pChX->lopr);
        maxX = MAX(maxX,pChX->hopr);
      }
    } else
    if (pChX == NULL && pChY != NULL) {
      /* Y channel - supporting scalar or waveform */
      if (ca_element_count(pChY->chid) > 1) {
        /* vector/waveform */
        pChY->xyChannelType = CP_YVector;
        if (i <= 0) {
          for(j = 0; j < (int)ca_element_count(pChY->chid); j++)
            xrtData1->g.data[i].xp[j]= (float)j;
          minY = MIN(minY,pChY->lopr);
          maxY = MAX(maxY,pChY->hopr);
        } else {
          for(j = 0; j < (int)ca_element_count(pChY->chid); j++)
            xrtData2->g.data[i-1].xp[j]= (float)j;
          minY2 = MIN(minY2,pChY->lopr);
          maxY2 = MAX(maxY2,pChY->hopr);
        }
        minX = MIN(minX,0.);
        maxX = MAX(maxX,(float)((int)ca_element_count(pChY->chid)-1));
      } else {
        /* scalar */
        pChY->xyChannelType = CP_YScalar;
        if (i <= 0) {
          for(j = 0; j < dlCartesianPlot->count; j++)
            xrtData1->g.data[i].xp[j]= (float)j;
          xrtData1->g.data[i].yp[0]= (float)pChY->value;
          minY = MIN(minY,pChY->lopr);
          maxY = MAX(maxY,pChY->hopr);
        } else {
          for(j = 0; j < dlCartesianPlot->count; j++)
            xrtData2->g.data[i-1].xp[j]= (float)j;
          xrtData2->g.data[i-1].yp[0]= (float)pChY->value;
          minY2 = MIN(minY2,pChY->lopr);
          maxY2 = MAX(maxY2,pChY->hopr);
        }
        minX = MIN(minX,0.);
        maxX = MAX(maxX,(float)dlCartesianPlot->count);
      }
    } /* end for */
  }
  monitorData = pCh;

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
  for (i = 0; i < cartesianPlotData->nTraces; i++) {
    monitorData = (Channel *) cartesianPlotData->monitors[i][0];
    if (monitorData == NULL)
      monitorData = (Channel*)cartesianPlotData->monitors[i][1];
    if (monitorData != NULL) {
      switch(dlCartesianPlot->style) {
      case POINT_PLOT:
        if (i <= 0) {
          myds1[i].lpat = XRT_LPAT_NONE;
          myds1[i].fpat = XRT_FPAT_NONE;
          myds1[i].color = rgb[i];
          myds1[i].pcolor = rgb[i];
          myds1[i].psize = MAX(2,dlCartesianPlot->object.height/70);
          XrtSetNthDataStyle(monitorData->self,i,&myds1[i]);
        } else {
          myds2[i-1].lpat = XRT_LPAT_NONE;
          myds2[i-1].fpat = XRT_FPAT_NONE;
          myds2[i-1].color = rgb[i];
          myds2[i-1].pcolor = rgb[i];
          myds2[i-1].psize = MAX(2, dlCartesianPlot->object.height/70);
          XrtSetNthDataStyle2(monitorData->self, i-1,&myds2[i-1]);
        }
        break;
      case LINE_PLOT:
        if (i <= 0) {
          myds1[i].fpat = XRT_FPAT_NONE;
          myds1[i].color = rgb[i];
          myds1[i].pcolor = rgb[i];
          myds1[i].psize = MAX(2, dlCartesianPlot->object.height/70);
          XrtSetNthDataStyle(monitorData->self,i,&myds1[i]);
        } else {
          myds2[i-1].fpat = XRT_FPAT_NONE;
          myds2[i-1].color = rgb[i];
          myds2[i-1].pcolor = rgb[i];
          myds2[i-1].psize = MAX(2, dlCartesianPlot->object.height/70);
          XrtSetNthDataStyle2(monitorData->self, i-1,&myds2[i-1]);
        }
        break;
      case FILL_UNDER_PLOT:
        if (i <= 0) {
          myds1[i].color = rgb[i];
          myds1[i].pcolor = rgb[i];
          myds1[i].psize = MAX(2, dlCartesianPlot->object.height/70);
          XrtSetNthDataStyle(monitorData->self,i,&myds1[i]);
        } else {
          myds2[i-1].color = rgb[i];
          myds2[i-1].pcolor = rgb[i];
          myds2[i-1].psize = MAX(2, dlCartesianPlot->object.height/70);
          XrtSetNthDataStyle2(monitorData->self, i-1,&myds2[i-1]);
        }
        break;
      }
    }
  }

  /*
   * now register event handlers for vectors of channels/monitors
   */
  for (i = 0; i < cartesianPlotData->nTraces; i++) {
    Channel *pChTmp = (Channel *) cartesianPlotData->monitors[i][0];
    if (pChTmp != NULL) {
      if ( i <= 0) {
        pChTmp->xrtData = xrtData1;
        pChTmp->xrtDataSet = 1;
        pChTmp->trace = i;
      } else {
        pChTmp->xrtData = xrtData2;
        pChTmp->xrtDataSet = 2;
        pChTmp->trace = i-1;
      }
    }
    pChTmp = (Channel *) cartesianPlotData->monitors[i][1];
    if (pChTmp != NULL) {
      if ( i <= 0) {
        pChTmp->xrtData = xrtData1;
        pChTmp->xrtDataSet = 1;
        pChTmp->trace = i;
      } else {
        pChTmp->xrtData = xrtData2;
        pChTmp->xrtDataSet = 2;
        pChTmp->trace = i-1;
      }
    }
  }


  /* fill in connect-time channel-based range specifications - NB:
   *  this is different than the min/max stored in the display element
   */
  cartesianPlotData->axisRange[X_AXIS_ELEMENT].axisMin = minX;
  cartesianPlotData->axisRange[X_AXIS_ELEMENT].axisMax = maxX;
  cartesianPlotData->axisRange[Y1_AXIS_ELEMENT].axisMin = minY;
  cartesianPlotData->axisRange[Y1_AXIS_ELEMENT].axisMax = maxY;
  cartesianPlotData->axisRange[Y2_AXIS_ELEMENT].axisMin = minY2;
  cartesianPlotData->axisRange[Y2_AXIS_ELEMENT].axisMax = maxY2;
  for (kk = X_AXIS_ELEMENT; kk <= Y2_AXIS_ELEMENT; kk++) {
    if (dlCartesianPlot->axis[kk].rangeStyle==CHANNEL_RANGE) {
      cartesianPlotData->axisRange[kk].isCurrentlyFromChannel= True;
    } else {
      cartesianPlotData->axisRange[kk].isCurrentlyFromChannel = False;
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


  XtSetValues(pCh->self,widgetArgs,n);

  /* add destroy callback */
  /* not freeing monitorData here, rather using monitorData for
     it's information */
  cartesianPlotData->xrtData1 = xrtData1;
  cartesianPlotData->xrtData2 = xrtData2;
  XtAddCallback(pCh->self,XmNdestroyCallback,
          (XtCallbackProc)monitorDestroy, (XtPointer)cartesianPlotData);
}

void cartesianPlotUpdateTrace(Channel *pCh) {
  CartesianPlotData *cartesianPlotData;
  DlCartesianPlot *dlCartesianPlot;
  int count, i;
  unsigned short j;
  Arg args[20];

  XtVaGetValues((Widget)pCh->self,
                 XmNuserData,(XtPointer)&cartesianPlotData,
                 NULL);
  dlCartesianPlot = (DlCartesianPlot *)pCh->specifics;

  switch(pCh->xyChannelType) {
  case CP_XYScalarX:
    /* X: x & y channels specified - scalars, up to dlCartesianPlot->count pairs */
    count = pCh->xrtData->g.data[pCh->trace].npoints;
    if (count <= dlCartesianPlot->count-1) {
      pCh->xrtData->g.data[pCh->trace].xp[count] = (float) pCh->value;
      pCh->xrtData->g.data[pCh->trace].yp[count] = (float) pCh->other->value;
      pCh->xrtData->g.data[pCh->trace].npoints++;
    } else
    if (count > dlCartesianPlot->count-1) {
      if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
	/* don't update */
      } else
      if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
        /* shift everybody down one, add at end */
        int j;
        for (j = 1; j < dlCartesianPlot->count; j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j-1] =
                   pCh->xrtData->g.data[pCh->trace].xp[j];
          pCh->xrtData->g.data[pCh->trace].yp[j-1] =
                   pCh->xrtData->g.data[pCh->trace].yp[j];
        }
        pCh->xrtData->g.data[pCh->trace].xp[j-1] = (float) pCh->value;
        pCh->xrtData->g.data[pCh->trace].yp[j-1] = (float) pCh->other->value;
        pCh->xrtData->g.data[pCh->trace].npoints = dlCartesianPlot->count;
      }
    }
    break;

  case CP_XYScalarY:
    /* Y: x & y channels specified - scalars, up to dlCartesianPlot->count pairs */
    count = pCh->xrtData->g.data[pCh->trace].npoints;
    if (count <= dlCartesianPlot->count-1) {
      pCh->xrtData->g.data[pCh->trace].yp[count] = (float) pCh->value;
      pCh->xrtData->g.data[pCh->trace].xp[count] = (float) pCh->other->value;
      pCh->xrtData->g.data[pCh->trace].npoints++;
    } else
    if (count > dlCartesianPlot->count-1) {
      if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
        /* don't update */
      } else
      if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
        /* shift everybody down one, add at end */
	int j;
        for (j = 1; j < dlCartesianPlot->count; j++) {
          pCh->xrtData->g.data[pCh->trace].yp[j-1] =
                   pCh->xrtData->g.data[pCh->trace].yp[j];
          pCh->xrtData->g.data[pCh->trace].xp[j-1] =
                   pCh->xrtData->g.data[pCh->trace].xp[j];
        }
        pCh->xrtData->g.data[pCh->trace].yp[j-1] = (float) pCh->value;
        pCh->xrtData->g.data[pCh->trace].xp[j-1] = (float) pCh->other->value;
      }
    }
    break;

  case CP_XScalar:
    /* x channel scalar, up to dlCartesianPlot->count pairs */
    count = pCh->xrtData->g.data[pCh->trace].npoints;
    if (count <= dlCartesianPlot->count-1) {
      pCh->xrtData->g.data[pCh->trace].xp[count] = (float) pCh->value;
      pCh->xrtData->g.data[pCh->trace].yp[count] = (float) count;
      pCh->xrtData->g.data[pCh->trace].npoints++;
    } else
    if (count > dlCartesianPlot->count-1) {
      if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
        /* don't update */
      } else
      if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
        /* shift everybody down one, add at end */
	int j;
        for (j = 1; j < dlCartesianPlot->count; j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j-1] =
                   pCh->xrtData->g.data[pCh->trace].xp[j];
          pCh->xrtData->g.data[pCh->trace].yp[j-1] =
                   (float) (j-1);
        }
        pCh->xrtData->g.data[pCh->trace].xp[j-1] = (float) pCh->value;
        pCh->xrtData->g.data[pCh->trace].yp[j-1] = (float) (j-1);
      }
    }
    break;


  case CP_XVector:
    /* x channel vector, ca_element_count(chid) elements */

    /* plot first "count" elements of vector per dlCartesianPlot*/
    switch(ca_field_type(pCh->chid)) {
      case DBF_STRING:
      {
        medmPrintf("\ncartesianPlotUpdateTrace: %s: %s",
                "illegal field type for Cartesian Plot channel",
                ca_name(pCh->chid));
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = 0.0;
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) j;
        }
        break;
      }
      case DBF_INT:
      {
        short *pShort = &pCh->data->i.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pShort[j];
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) j;
        }
        break;
      }
      case DBF_FLOAT:
      {
        float *pFloat =  &pCh->data->f.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pFloat[j];
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) j;
        }
        break;
      }
      case DBF_ENUM:
      {
        unsigned short *pUShort = &pCh->data->e.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pUShort[j];
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) j;
        }
        break;
      }
      case DBF_CHAR:
      {
        unsigned char *pChar = &pCh->data->c.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pChar[j];
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) j;
        }
        break;
      }
      case DBF_LONG:
      {
        dbr_long_t *pLong = &pCh->data->l.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pLong[j];
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) j;
        }
        break;
      }
      case DBF_DOUBLE:
      {
        double *pDouble = &pCh->data->d.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pDouble[j];
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) j;
        }
        break;
      }
    }
    pCh->xrtData->g.data[pCh->trace].npoints = ca_element_count(pCh->chid);
    break;

  case CP_YScalar:
    /* y channel scalar, up to dlCartesianPlot->count pairs */
    count = pCh->xrtData->g.data[pCh->trace].npoints;
    if (count <= dlCartesianPlot->count-1) {
      pCh->xrtData->g.data[pCh->trace].yp[count] = (float) pCh->value;
      pCh->xrtData->g.data[pCh->trace].xp[count] = (float) count;
      pCh->xrtData->g.data[pCh->trace].npoints++;
    } else
    if (count > dlCartesianPlot->count-1) {
      if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF) {
        /* Don't update */
      } else
      if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON) {
        /* shift everybody down one, add at end */
	int j;
        for (j = 1; j < dlCartesianPlot->count; j++) {
          pCh->xrtData->g.data[pCh->trace].yp[j-1] =
              pCh->xrtData->g.data[pCh->trace].yp[j];
          pCh->xrtData->g.data[pCh->trace].xp[j-1] = (float) (j-1);
        }
        pCh->xrtData->g.data[pCh->trace].yp[j-1] = (float) pCh->value;
        pCh->xrtData->g.data[pCh->trace].xp[j-1] = (float) (j-1);
      }
    }
    break;

  case CP_YVector:
    /* plot first "count" elements of vector per dlCartesianPlot*/
    switch(ca_field_type(pCh->chid)) {
    case DBF_STRING:
    {
      medmPrintf("\ncartesianPlotUpdateTrace: %s: %s",
                "illegal field type for Cartesian Plot channel",
                ca_name(pCh->chid));
      for (j = 0; j < ca_element_count(pCh->chid); j++) {
        pCh->xrtData->g.data[pCh->trace].yp[j] = 0.0;
        pCh->xrtData->g.data[pCh->trace].xp[j] = (float) j;
      }
      break;
    }
    case DBF_INT:
    {
      short *pShort = &pCh->data->i.value;
      for (j = 0; j < ca_element_count(pCh->chid); j++) {
        pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pShort[j];
        pCh->xrtData->g.data[pCh->trace].xp[j] = (float) j;
      }
      break;
    }
    case DBF_FLOAT:
    {
      float *pFloat = &pCh->data->f.value;
      for (j = 0; j < ca_element_count(pCh->chid); j++) {
        pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pFloat[j];
        pCh->xrtData->g.data[pCh->trace].xp[j] = (float) j;
      }
      break;
    }
    case DBF_ENUM:
    {
      unsigned short *pUShort = &pCh->data->e.value;;
      for (j = 0; j < ca_element_count(pCh->chid); j++) {
        pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pUShort[j];
        pCh->xrtData->g.data[pCh->trace].xp[j] = (float) j;
      }
      break;
    }
    case DBF_CHAR:
    {
      unsigned char *pChar = &pCh->data->c.value;;
      for (j = 0; j < ca_element_count(pCh->chid); j++) {
        pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pChar[j];
        pCh->xrtData->g.data[pCh->trace].xp[j] = (float) j;
      }
      break;
    }
    case DBF_LONG:
    {
      dbr_long_t *pLong = &pCh->data->l.value;
      for (j = 0; j < ca_element_count(pCh->chid); j++) {
        pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pLong[j];
        pCh->xrtData->g.data[pCh->trace].xp[j] = (float) j;
      }
      break;
    }
    case DBF_DOUBLE:
    {
      double *pDouble = &pCh->data->d.value;
      for (j = 0; j < ca_element_count(pCh->chid); j++) {
        pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pDouble[j];
        pCh->xrtData->g.data[pCh->trace].xp[j] = (float) j;
      }
      break;
    }
  }
  pCh->xrtData->g.data[pCh->trace].npoints = ca_element_count(pCh->chid);
  break;


  case CP_XVectorYScalarX:
 /* x channel vector, ca_element_count(chid) elements */
    pCh->xrtData->g.data[pCh->trace].npoints = ca_element_count(pCh->chid);

    /* plot first "count" elements of vector per dlCartesianPlot*/
    switch(ca_field_type(pCh->chid)) {
    case DBF_STRING:
    {
      medmPrintf("\ncartesianPlotUpdateTrace: %s: %s",
              "illegal field type for Cartesian Plot channel",
              ca_name(pCh->chid));
      for (j = 0; j < ca_element_count(pCh->chid); j++) {
        pCh->xrtData->g.data[pCh->trace].xp[j] = 0.0;
        pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pCh->other->value;
      }
      break;
    }
    case DBF_INT:
    {
      short *pShort = &pCh->data->i.value;
      for (j = 0; j < ca_element_count(pCh->chid); j++) {
        pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pShort[j];
        pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pCh->other->value;
      }
      break;
    }
    case DBF_FLOAT:
    {
      float *pFloat = &pCh->data->f.value;
      for (j = 0; j < ca_element_count(pCh->chid); j++) {
        pCh->xrtData->g.data[pCh->trace].xp[j] = pFloat[j];
        pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pCh->other->value;
      }
      break;
    }
    case DBF_ENUM:
    {
      unsigned short *pUShort = &pCh->data->e.value;
      for (j = 0; j < ca_element_count(pCh->chid); j++) {
        pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pUShort[j];
        pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pCh->other->value;
      }
      break;
    }
    case DBF_CHAR:
    {
      unsigned char *pChar = &pCh->data->c.value;
      for (j = 0; j < ca_element_count(pCh->chid); j++) {
        pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pChar[j];
        pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pCh->other->value;
      }
      break;
    }
    case DBF_LONG:
    {
      dbr_long_t *pLong = &pCh->data->l.value;
      for (j = 0; j < ca_element_count(pCh->chid); j++) {
        pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pLong[j];
        pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pCh->other->value;
      }
      break;
    }
    case DBF_DOUBLE:
    {
      double *pDouble = &pCh->data->d.value;
      for (j = 0; j < ca_element_count(pCh->chid); j++) {
        pCh->xrtData->g.data[pCh->trace].xp[j] = (double) pDouble[j];
        pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pCh->other->value;
      }
      break;
    }
  }
  pCh->xrtData->g.data[pCh->trace].npoints = ca_element_count(pCh->chid);
  break;


  case CP_YVectorXScalarY:
    /* y channel vector, ca_element_count(chid) elements */
    pCh->xrtData->g.data[pCh->trace].npoints = ca_element_count(pCh->chid);

    /* plot first "count" elements of vector per dlCartesianPlot*/
    switch(ca_field_type(pCh->chid)) {
      case DBF_STRING:
      {
        medmPrintf("\ncartesianPlotUpdateTrace: %s: %s",
                     "illegal field type for Cartesian Plot channel",
                     ca_name(pCh->chid));
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pCh->other->value;
          pCh->xrtData->g.data[pCh->trace].yp[j] = 0.0;
        }
        break;
      }
      case DBF_INT:
      {
        short *pShort = &pCh->data->i.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pCh->other->value;
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pShort[j];
        }
        break;
      }
      case DBF_FLOAT:
      {
        float *pFloat = &pCh->data->f.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pCh->other->value;
          pCh->xrtData->g.data[pCh->trace].yp[j] = pFloat[j];
        }
        break;
      }
      case DBF_ENUM:
      {
        unsigned short *pUShort = &pCh->data->e.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pCh->other->value;
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pUShort[j];
        }
        break;
      }
      case DBF_CHAR:
      {
        unsigned char *pChar = &pCh->data->c.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pCh->other->value;
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pChar[j];
        }
        break;
      }
      case DBF_LONG:
      {
        dbr_long_t *pLong = &pCh->data->l.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pCh->other->value;
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pLong[j];
        }
        break;
      }
      case DBF_DOUBLE:
      {
        double *pDouble = &pCh->data->d.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pCh->other->value;
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pDouble[j];
        }
        break;
      }
    }
    pCh->xrtData->g.data[pCh->trace].npoints = ca_element_count(pCh->chid);
    break;

  case CP_XVectorYScalarY:
    pCh->xrtData->g.data[pCh->trace].npoints = ca_element_count(pCh->chid);
    for (j = 0; j < ca_element_count(pCh->chid); j++) {
      pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pCh->value;
    }
    break;

  case CP_YVectorXScalarX:
    pCh->xrtData->g.data[pCh->trace].npoints = ca_element_count(pCh->chid);
    for (j = 0; j < ca_element_count(pCh->chid); j++) {
      pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pCh->value;
    }
    break;

/**** N.B. - not handling update of other vector in this call, relying
 ****           upon old value information until it is also updated
 ****           by monitor events.  Note that this is probabilistically
 ****           equivalent to the other more complicated monitor-occurred-
 ****           on-this-channel-but-fill-in-other-channel-from-its-current-
 ****           value
 ****/
  case CP_XYVectorX:
    /* x channel vector, ca_element_count(chid) elements */
    pCh->xrtData->g.data[pCh->trace].npoints =
      MIN(ca_element_count(pCh->chid), ca_element_count(pCh->other->chid));

    /* plot first "count" elements of vector per dlCartesianPlot*/
    switch(ca_field_type(pCh->chid)) {
      case DBF_STRING:
        medmPrintf("\ncartesianPlotUpdateTrace: %s: %s",
                        "illegal field type for Cartesian Plot channel",
                        ca_name(pCh->chid));
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = 0.0;
        }
        break;
      case DBF_INT:
      {
        short *pShort = &pCh->data->i.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pShort[j];
        }
        break;
      }
      case DBF_FLOAT:
      {
        float *pFloat = &pCh->data->f.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = pFloat[j];
        }
        break;
      }
      case DBF_ENUM:
      {
        unsigned short *pUShort = &pCh->data->e.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pUShort[j];
        }
        break;
      }
      case DBF_CHAR:
      {
        unsigned char *pChar = &pCh->data->c.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pChar[j];
        }
        break;
      }
      case DBF_LONG:
      {
        dbr_long_t *pLong = &pCh->data->l.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pLong[j];
        }
        break;
      }
      case DBF_DOUBLE:
      {
        double *pDouble = &pCh->data->d.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].xp[j] = (float) pDouble[j];
        }
        break;
      }
    }
    pCh->xrtData->g.data[pCh->trace].npoints = ca_element_count(pCh->chid);
    break;


  case CP_XYVectorY:
    /* y channel vector, ca_element_count(chid) elements */
    pCh->xrtData->g.data[pCh->trace].npoints =
      MIN(ca_element_count(pCh->chid), ca_element_count(pCh->other->chid));

    /* plot first "count" elements of vector per dlCartesianPlot*/
    switch(ca_field_type(pCh->chid)) {
      case DBF_STRING:
        medmPrintf("\ncartesianPlotUpdateTrace: %s: %s",
                        "illegal field type for Cartesian Plot channel",
                        ca_name(pCh->chid));
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].yp[j] = 0.0;
        }
        break;
      case DBF_INT:
      {
        short *pShort = &pCh->data->i.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pShort[j];
        }
        break;
      }
      case DBF_FLOAT:
      {
        float *pFloat = &pCh->data->f.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].yp[j] = pFloat[j];
        }
        break;
      }
      case DBF_ENUM:
      {
        unsigned short *pUShort = &pCh->data->e.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pUShort[j];
        }
        break;
      }
      case DBF_CHAR:
      {
        unsigned char *pChar = &pCh->data->c.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pChar[j];
        }
        break;
      }
      case DBF_LONG:
      {
        dbr_long_t *pLong = &pCh->data->l.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pLong[j];
        }
        break;
      }
      case DBF_DOUBLE:
      {
        double *pDouble = &pCh->data->d.value;
        for (j = 0; j < ca_element_count(pCh->chid); j++) {
          pCh->xrtData->g.data[pCh->trace].yp[j] = (float) pDouble[j];
        }
        break;
      }
    }
    pCh->xrtData->g.data[pCh->trace].npoints = ca_element_count(pCh->chid);
    break;
  default:
    medmPrintf("\ncartesianPlotUpdateTrace: unhandled CP case: %d",
                        pCh->xyChannelType);
  }
}

void cartesianPlotUpdateScreen(Channel *pCh) {
  CartesianPlotData *cartesianPlotData;
  DlCartesianPlot *dlCartesianPlot;
  int count, i;
  unsigned short j;
  Arg args[20];

  /*
   * MDA - handle trigger channel here - only set modified on trigger if trigger
   *    is used, but always update the data for the other channels
   */
  XtVaGetValues((Widget)pCh->self,
                 XmNuserData,(XtPointer)&cartesianPlotData,
                 NULL);
  if (cartesianPlotData == (CartesianPlotData *)NULL) {
    medmPrintf("\ncartesianPlotUpdateScreen: cartesianPlotData == NULL");
    return;
  }

  /* return until all channels get their graphical information */
  for (i = 0; i < cartesianPlotData->nTraces; i++) {
    Channel *tmpCh;
    tmpCh = (Channel *) cartesianPlotData->monitors[i][0];
    if (tmpCh != NULL) {
      if (tmpCh->xrtData == NULL) return;
    }
    tmpCh = (Channel *) cartesianPlotData->monitors[i][1];
    if (tmpCh != NULL) {
      if (tmpCh->xrtData == NULL) return;
    }
  }

  /* if this is in trigger mode, and this update request is not from
   * the trigger, do not update.
   */

  /* if this is an erase channel, erase screen */
  if (pCh == (Channel *) cartesianPlotData->eraseCh) {
    Boolean clearDataSet1 = True;
    Boolean clearDataSet2 = True;
    XrtDataStyle style;

    if (((pCh->value == 0) && (cartesianPlotData->eraseMode == ERASE_IF_NOT_ZERO))
	||((pCh->value != 0) && (cartesianPlotData->eraseMode == ERASE_IF_ZERO))) {
      return;
    }
    for (i = 0; i < cartesianPlotData->nTraces; i++) {
      Channel *pChX = (Channel *) cartesianPlotData->monitors[i][0];
      Channel *pChY = (Channel *) cartesianPlotData->monitors[i][1];
      if (pChX) {
	int n = pChX->xrtData->g.data[pChX->trace].npoints;
        if (n > 0) {
	  if ((pChX->xrtDataSet == 1) && (clearDataSet1)) {
	    XtVaSetValues(pChX->self,XtNxrtData,nullData,NULL);
	    clearDataSet1 = False;
	  } else
	  if ((pChX->xrtDataSet == 2) && (clearDataSet2)) {
	    XtVaSetValues(pChX->self,XtNxrtData2,nullData,NULL);
	    clearDataSet2 = False;
	  }
	  pChX->xrtData->g.data[pChX->trace].npoints = 0;
	}
      } else
      if (pChY) {
	int n = pChY->xrtData->g.data[pChY->trace].npoints;
        if (n > 0) {
	  if ((pChY->xrtDataSet == 1) && (clearDataSet1)) {
	    XtVaSetValues(pChY->self,XtNxrtData,nullData,NULL);
	    clearDataSet1 = False;
	  } else
	  if ((pChY->xrtDataSet == 2) && (clearDataSet2)) {
	    XtVaSetValues(pChY->self,XtNxrtData2,nullData,NULL);
	    clearDataSet2 = False;
	  }
	  pChY->xrtData->g.data[pChY->trace].npoints = 0;
	}
      }
    }
    return;
  }

  if (cartesianPlotData->triggerCh) {
    if (pCh != (Channel *)cartesianPlotData->triggerCh)
      return;
  }


  dlCartesianPlot = (DlCartesianPlot *)pCh->specifics;

  if (cartesianPlotData->triggerCh) {
    /* trigger channel monitor, update appropriate plots */
    for (i = 0; i < cartesianPlotData->nTraces; i++) {
      Channel *pChX = (Channel *) cartesianPlotData->monitors[i][0];
      Channel *pChY = (Channel *) cartesianPlotData->monitors[i][1];
      if ((pChX == NULL) && (pChY == NULL)) continue;
      if ((pChX) && (pChX->xrtData))
        cartesianPlotUpdateTrace(pChX);
      else
      if ((pChY) && (pChY->xrtData))
        cartesianPlotUpdateTrace(pChY);
      if (pChX) {
        if (pChX->xrtDataSet == 1)
	  XtVaSetValues(pChX->self,XtNxrtData,pChX->xrtData,NULL);
	else
	if (pChX->xrtDataSet == 2)
	  XtVaSetValues(pChX->self,XtNxrtData2,pChX->xrtData,NULL);
      } else {
	if (pChY->xrtDataSet == 1)
	  XtVaSetValues(pChY->self,XtNxrtData,pChY->xrtData,NULL);
	else
	if (pChY->xrtDataSet == 2)
	  XtVaSetValues(pChY->self,XtNxrtData2,pChY->xrtData,NULL);
      }
    }
  } else {
    cartesianPlotUpdateTrace(pCh);
    /* not in CP related to trigger channel, proceed as normal */
    if (pCh->xrtDataSet == 1) {
      XtVaSetValues(pCh->self,XtNxrtData,pCh->xrtData,NULL);
    } else
    if (pCh->xrtDataSet == 2) {
      XtVaSetValues(pCh->self,XtNxrtData2,pCh->xrtData,NULL);
    } else
      medmPrintf("\ncartesianPlotUpdateScreen : illegal xrtDataSet specified (%d)",
                      pCh->xrtDataSet);
  }
}

void cartesianPlotDestroyCb(Channel *pCh) {
  if (executeTimeCartesianPlotWidget == pCh->self) {
    executeTimeCartesianPlotWidget = NULL;
    XtSetSensitive(cartesianPlotAxisS,False);
  }
  return;
}

void cartesianPlotUpdateValueCb(Channel *pCh) {
  CartesianPlotData *cartesianPlotData;
  DlCartesianPlot *dlCartesianPlot;
  int count, i;
  Boolean connected = True;
  Boolean readAccess = True;

  XtVaGetValues((Widget)pCh->self,XmNuserData,(XtPointer)&cartesianPlotData,NULL);
  if (cartesianPlotData == (CartesianPlotData *)NULL) {
    medmPrintf("\ncartesianPlotUpdateValueCb: cartesianPlotData == NULL");
    return;
  }

  /* check for connection */
  for (i = 0; i < cartesianPlotData->nTraces; i++) {
    Channel *tmpCh;
    tmpCh = (Channel *) cartesianPlotData->monitors[i][0];
    if (tmpCh != NULL) {
      if (ca_state(tmpCh->chid) != cs_conn) {
	connected = False;
	break;
      }
      if (!ca_read_access(tmpCh->chid)) readAccess = False;
    }
    tmpCh = (Channel *) cartesianPlotData->monitors[i][1];
    if (tmpCh != NULL) {
      if (ca_state(tmpCh->chid) != cs_conn) {
	connected = False;
	break;
      }
      if (!ca_read_access(tmpCh->chid)) readAccess = False;
    }
  }

  if (cartesianPlotData->triggerCh) {
    Channel *tmpCh = (Channel *) cartesianPlotData->triggerCh;
    if (ca_state(tmpCh->chid) != cs_conn) {
      connected = False;
    } else
    if (!ca_read_access(tmpCh->chid)) readAccess = False;
  }

  if (cartesianPlotData->eraseCh) {
    Channel *tmpCh = (Channel *) cartesianPlotData->eraseCh;
    if (ca_state(tmpCh->chid) != cs_conn) {
      connected = False;
    } else
    if (!ca_read_access(tmpCh->chid)) readAccess = False;
  }

  if (connected) {
    if (readAccess) {
      if (pCh->self) {
        XtManageChild(pCh->self);
      }
    } else {
      if (pCh->self) {
        XtUnmanageChild(pCh->self);
      }
      draw3DPane(pCh);
      draw3DQuestionMark(pCh);
    }
  } else {
    if (pCh->self)
      XtUnmanageChild(pCh->self);
    drawWhiteRectangle(pCh);
  }
}
