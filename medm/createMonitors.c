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


/****************************************************************
 *****    nested objects (not to be put in display list )   *****
/****************************************************************


/***
 *** monitor element in each monitor object
 ***/


static void createDlMonitor(
  DisplayInfo *displayInfo,
  DlMonitor *monitor)
{
  strcpy(monitor->rdbk,globalResourceBundle.rdbk);
  monitor->clr = globalResourceBundle.clr;
  monitor->bclr = globalResourceBundle.bclr;
}



/***
 *** plotcom element in each plot type object
 ***/


static void createDlPlotcom(
  DisplayInfo *displayInfo,
  DlPlotcom *plotcom)
{
  strcpy(plotcom->title,globalResourceBundle.title);
  strcpy(plotcom->xlabel,globalResourceBundle.xlabel);
  strcpy(plotcom->ylabel,globalResourceBundle.ylabel);
  plotcom->clr = globalResourceBundle.clr;
  plotcom->bclr = globalResourceBundle.bclr;
}


/***
 *** axis definition element in each plot type object
 ***/


static void createDlPlotAxisDefinition(
  DisplayInfo *displayInfo,
  DlPlotAxisDefinition *axisDefinition,
  int axis)
{
  axisDefinition->axisStyle = globalResourceBundle.axis[axis].axisStyle;
  axisDefinition->rangeStyle = globalResourceBundle.axis[axis].rangeStyle;
  axisDefinition->minRange = globalResourceBundle.axis[axis].minRange;
  axisDefinition->maxRange = globalResourceBundle.axis[axis].maxRange;
}


/***
 *** pen element in each strip chart
 ***/


static void createDlPen(
  DisplayInfo *displayInfo,
  DlPen *pen,
  int penNumber)
{
/* structure copy */
  *pen = globalResourceBundle.scData[penNumber];
}


/***
 *** trace element in each cartesian plot
 ***/


static void createDlTrace(
  DisplayInfo *displayInfo,
  DlTrace *trace,
  int traceNum)
{
/* structure copy */
  *trace  = globalResourceBundle.cpData[traceNum];
}





/***
 *** Meter
 ***/


DlElement *createDlMeter(
  DisplayInfo *displayInfo)
{
  DlMeter *dlMeter;
  DlElement *dlElement;

  dlMeter = (DlMeter *) malloc(sizeof(DlMeter));
  createDlObject(displayInfo,&(dlMeter->object));
  createDlMonitor(displayInfo,&(dlMeter->monitor));
  dlMeter->label = globalResourceBundle.label;
  dlMeter->clrmod = globalResourceBundle.clrmod;

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Meter;
  dlElement->structure.meter = dlMeter;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlMeter;
  dlElement->dmWrite = (void(*)())writeDlMeter;

  return(dlElement);
}



/***
 *** Bar
 ***/


DlElement *createDlBar(
  DisplayInfo *displayInfo)
{
  DlBar *dlBar;
  DlElement *dlElement;

  dlBar = (DlBar *) malloc(sizeof(DlBar));
  createDlObject(displayInfo,&(dlBar->object));
  createDlMonitor(displayInfo,&(dlBar->monitor));
  dlBar->label = globalResourceBundle.label;
  dlBar->clrmod = globalResourceBundle.clrmod;
  dlBar->direction = globalResourceBundle.direction;
  dlBar->fillmod = globalResourceBundle.fillmod;

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Bar;
  dlElement->structure.bar = dlBar;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlBar;
  dlElement->dmWrite = (void(*)())writeDlBar;

  return(dlElement);
}

DlElement *createDlByte( DisplayInfo *displayInfo) {
/****************************************************************************
 * Create DL Byte                                                           *
 ****************************************************************************/
  DlByte *dlByte;
  DlElement *dlElement;

    dlByte = (DlByte *) malloc(sizeof(DlByte));
    createDlObject(displayInfo,&(dlByte->object));
    createDlMonitor(displayInfo,&(dlByte->monitor));
    dlByte->clrmod = globalResourceBundle.clrmod;
    dlByte->direction = globalResourceBundle.direction;
    dlByte->sbit = globalResourceBundle.sbit;
    dlByte->ebit = globalResourceBundle.ebit;

    dlElement = (DlElement *) malloc(sizeof(DlElement));
    dlElement->type = DL_Byte;
    dlElement->structure.byte = dlByte;
    dlElement->next = NULL;
    dlElement->prev = displayInfo->dlElementListTail;
    displayInfo->dlElementListTail->next = dlElement;
    displayInfo->dlElementListTail = dlElement;
    dlElement->dmExecute = (void(*)())executeDlByte;
    dlElement->dmWrite = (void(*)())writeDlByte;

    return(dlElement);
}

/***
 *** Indicator
 ***/


DlElement *createDlIndicator(
  DisplayInfo *displayInfo)
{
  DlIndicator *dlIndicator;
  DlElement *dlElement;

  dlIndicator = (DlIndicator *) malloc(sizeof(DlIndicator));
  createDlObject(displayInfo,&(dlIndicator->object));
  createDlMonitor(displayInfo,&(dlIndicator->monitor));
  dlIndicator->label = globalResourceBundle.label;
  dlIndicator->clrmod = globalResourceBundle.clrmod;
  dlIndicator->direction = globalResourceBundle.direction;

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Indicator;
  dlElement->structure.indicator = dlIndicator;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlIndicator;
  dlElement->dmWrite = (void(*)())writeDlIndicator;

  return(dlElement);
}





/***
 *** Text Update
 ***/


DlElement *createDlTextUpdate(
  DisplayInfo *displayInfo)
{
  DlTextUpdate *dlTextUpdate;
  DlElement *dlElement;

  dlTextUpdate = (DlTextUpdate *) malloc(sizeof(DlTextUpdate));
  createDlObject(displayInfo,&(dlTextUpdate->object));
  createDlMonitor(displayInfo,&(dlTextUpdate->monitor));
  dlTextUpdate->clrmod = globalResourceBundle.clrmod;
  dlTextUpdate->format = globalResourceBundle.format;
  dlTextUpdate->align = globalResourceBundle.align;

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_TextUpdate;
  dlElement->structure.textUpdate = dlTextUpdate;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlTextUpdate;
  dlElement->dmWrite = (void(*)())writeDlTextUpdate;

  return(dlElement);
}





/***
 ***  Strip Chart
 ***/


DlElement *createDlStripChart(
  DisplayInfo *displayInfo)
{
  DlStripChart *dlStripChart;
  DlElement *dlElement;
  int penNumber;


  dlStripChart = (DlStripChart *) malloc(sizeof(DlStripChart));
  createDlObject(displayInfo,&(dlStripChart->object));
  createDlPlotcom(displayInfo,&(dlStripChart->plotcom));
  dlStripChart->period = globalResourceBundle.period;
  dlStripChart->units = globalResourceBundle.units;
#if 1
  /* for backward compatible */
  dlStripChart->delay = -1.0;
  dlStripChart->oldUnits = globalResourceBundle.units;
#endif
  for (penNumber = 0; penNumber < MAX_PENS; penNumber++)
	createDlPen(displayInfo,&(dlStripChart->pen[penNumber]),penNumber);


  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_StripChart;
  dlElement->structure.stripChart = dlStripChart;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlStripChart;
  dlElement->dmWrite = (void(*)())writeDlStripChart;

  return(dlElement);
}



/***
 ***  Cartesian Plot
 ***/


DlElement *createDlCartesianPlot(
  DisplayInfo *displayInfo)
{
  DlCartesianPlot *dlCartesianPlot;
  DlElement *dlElement;
  int traceNumber;

  dlCartesianPlot = (DlCartesianPlot *) malloc(sizeof(DlCartesianPlot));
  createDlObject(displayInfo,&(dlCartesianPlot->object));
  createDlPlotcom(displayInfo,&(dlCartesianPlot->plotcom));
  dlCartesianPlot->count = globalResourceBundle.count;
  dlCartesianPlot->style = globalResourceBundle.cStyle;
  dlCartesianPlot->erase_oldest = globalResourceBundle.erase_oldest;
  for (traceNumber = 0; traceNumber < MAX_TRACES; traceNumber++)
	createDlTrace(displayInfo,&(dlCartesianPlot->trace[traceNumber]),
	traceNumber);
  createDlPlotAxisDefinition(displayInfo,
	&(dlCartesianPlot->axis[X_AXIS_ELEMENT]),X_AXIS_ELEMENT);
  createDlPlotAxisDefinition(displayInfo,
	&(dlCartesianPlot->axis[Y1_AXIS_ELEMENT]),Y1_AXIS_ELEMENT);
  createDlPlotAxisDefinition(displayInfo,
	&(dlCartesianPlot->axis[Y2_AXIS_ELEMENT]),Y2_AXIS_ELEMENT);
  strcpy(dlCartesianPlot->trigger,globalResourceBundle.trigger);
  strcpy(dlCartesianPlot->erase,globalResourceBundle.erase);
  dlCartesianPlot->eraseMode = globalResourceBundle.eraseMode;

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_CartesianPlot;
  dlElement->structure.cartesianPlot = dlCartesianPlot;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlCartesianPlot;
  dlElement->dmWrite = (void(*)())writeDlCartesianPlot;

  return(dlElement);
}



/***
 ***  Surface Plot
 ***/


DlElement *createDlSurfacePlot(
  DisplayInfo *displayInfo)
{
  DlSurfacePlot *dlSurfacePlot;
  DlElement *dlElement;

#if 0
  dlSurfacePlot = (DlSurfacePlot *) malloc(sizeof(DlSurfacePlot));
  createDlObject(displayInfo,&(dlSurfacePlot->object));
  createDlPlotcom(displayInfo,&(dlSurfacePlot->plotcom));
  strcpy(dlSurfacePlot->data,globalResourceBundle.data);
  dlSurfacePlot->data_clr = globalResourceBundle.data_clr;
  dlSurfacePlot->dis = globalResourceBundle.dis;
  dlSurfacePlot->xyangle = globalResourceBundle.xyangle;
  dlSurfacePlot->zangle = globalResourceBundle.zangle;

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_SurfacePlot;
  dlElement->structure.surfacePlot = dlSurfacePlot;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlSurfacePlot;
  dlElement->dmWrite = (void(*)())writeDlSurfacePlot;
#endif
  return(dlElement);
}

