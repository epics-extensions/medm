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

/****************************************************************************
 * writeMonitors.c - Functions which save an ascii display file             *
 * Mods: DMW - writeDlByte - Write Byte structure to display file.          *
 ****************************************************************************/
#include "medm.h"
#include <X11/keysym.h>

/****** Monitors ***/

void writeDlMeter(
  FILE *stream,
  DlMeter *dlMeter,
  int level)
{
  int i;
  char indent[16];

  for (i = 0;  i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%smeter {",indent);
  writeDlObject(stream,&(dlMeter->object),level+1);
  writeDlMonitor(stream,&(dlMeter->monitor),level+1);
  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,
	stringValueTable[dlMeter->label]);
  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	stringValueTable[dlMeter->clrmod]);
  fprintf(stream,"\n%s}",indent);
}

void writeDlBar( FILE *stream, DlBar *dlBar, int level) {
/****************************************************************************
 * Write DL Bar                                                             *
 ****************************************************************************/
  int i;
  char indent[16];

    for (i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%sbar {",indent);
    writeDlObject(stream,&(dlBar->object),level+1);
    writeDlMonitor(stream,&(dlBar->monitor),level+1);
    fprintf(stream,"\n%s\tlabel=\"%s\"",indent,
      stringValueTable[dlBar->label]);
    fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
      stringValueTable[dlBar->clrmod]);
    fprintf(stream,"\n%s\tdirection=\"%s\"",indent,
      stringValueTable[dlBar->direction]);
    fprintf(stream,"\n%s\tfillmod=\"%s\"",indent,
      stringValueTable[dlBar->fillmod]);
    fprintf(stream,"\n%s}",indent);
}

void writeDlByte(FILE *stream, DlByte *dlByte, int level) {
/****************************************************************************
 * Write DL Byte                                                            *
 ****************************************************************************/
  int i;
  char indent[16];

    for (i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%sbyte {",indent);
    writeDlObject(stream,&(dlByte->object),level+1);
    writeDlMonitor(stream,&(dlByte->monitor),level+1);
    fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
      stringValueTable[dlByte->clrmod]);
    fprintf(stream,"\n%s\tdirection=\"%s\"",indent,
      stringValueTable[dlByte->direction]);
    fprintf(stream,"\n%s\tsbit=%d",indent,dlByte->sbit);
    fprintf(stream,"\n%s\tebit=%d",indent,dlByte->ebit);
    fprintf(stream,"\n%s}",indent);
}

void writeDlIndicator( FILE *stream, DlIndicator *dlIndicator, int level) {
/****************************************************************************
 * Write DL Indicator                                                       *
 ****************************************************************************/
  int i;
  char indent[16];

    for (i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%sindicator {",indent);
    writeDlObject(stream,&(dlIndicator->object),level+1);
    writeDlMonitor(stream,&(dlIndicator->monitor),level+1);
    fprintf(stream,"\n%s\tlabel=\"%s\"",indent,
      stringValueTable[dlIndicator->label]);
    fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
      stringValueTable[dlIndicator->clrmod]);
    fprintf(stream,"\n%s\tdirection=\"%s\"",indent,
      stringValueTable[dlIndicator->direction]);
    fprintf(stream,"\n%s}",indent);
}

void writeDlTextUpdate(FILE *stream, DlTextUpdate *dlTextUpdate, int level) {
  int i;
  char indent[16];

  for (i = 0;  i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%s\"text update\" {",indent);
  writeDlObject(stream,&(dlTextUpdate->object),level+1);
  writeDlMonitor(stream,&(dlTextUpdate->monitor),level+1);
  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	stringValueTable[dlTextUpdate->clrmod]);
  fprintf(stream,"\n%s\talign=\"%s\"",indent,
	stringValueTable[dlTextUpdate->align]);
  fprintf(stream,"\n%s\tformat=\"%s\"",indent,
	stringValueTable[dlTextUpdate->format]);
  fprintf(stream,"\n%s}",indent);

}


void writeDlStripChart( FILE *stream, DlStripChart *dlStripChart, int level) {
  int i;
  char indent[16];

    for (i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%s\"strip chart\" {",indent);
    writeDlObject(stream,&(dlStripChart->object),level+1);
    writeDlPlotcom(stream,&(dlStripChart->plotcom),level+1);
    fprintf(stream,"\n%s\tdelay=%d",indent,dlStripChart->delay);
    fprintf(stream,"\n%s\tunits=\"%s\"",indent,
      stringValueTable[dlStripChart->units]);
    for (i = 0; i < MAX_PENS; i++) {
      writeDlPen(stream,&(dlStripChart->pen[i]),i,level+1);
    }
    fprintf(stream,"\n%s}",indent);

}


void writeDlCartesianPlot(
  FILE *stream,
  DlCartesianPlot *dlCartesianPlot,
  int level)
{
  int i;
  char indent[16];

  for (i = 0;  i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%s\"cartesian plot\" {",indent);
  writeDlObject(stream,&(dlCartesianPlot->object),level+1);
  writeDlPlotcom(stream,&(dlCartesianPlot->plotcom),level+1);
  fprintf(stream,"\n%s\tstyle=\"%s\"",indent,
	stringValueTable[dlCartesianPlot->style]);
  fprintf(stream,"\n%s\terase_oldest=\"%s\"",indent,
	stringValueTable[dlCartesianPlot->erase_oldest]);
  fprintf(stream,"\n%s\tcount=\"%d\"",indent,dlCartesianPlot->count);
  for (i = 0; i < MAX_TRACES; i++) {
    writeDlTrace(stream,&(dlCartesianPlot->trace[i]),i,level+1);
  }
  writeDlPlotAxisDefinition(stream,&(dlCartesianPlot->axis[X_AXIS_ELEMENT]),
	X_AXIS_ELEMENT,level+1);
  writeDlPlotAxisDefinition(stream,&(dlCartesianPlot->axis[Y1_AXIS_ELEMENT]),
	Y1_AXIS_ELEMENT,level+1);
  writeDlPlotAxisDefinition(stream,&(dlCartesianPlot->axis[Y2_AXIS_ELEMENT]),
	Y2_AXIS_ELEMENT,level+1);
  fprintf(stream,"\n%s\ttrigger=\"%s\"",indent,dlCartesianPlot->trigger);
  fprintf(stream,"\n%s\terase=\"%s\"",indent,dlCartesianPlot->erase);
  fprintf(stream,"\n%s\teraseMode=\"%s\"",indent,
	stringValueTable[dlCartesianPlot->eraseMode]);
  fprintf(stream,"\n%s}",indent);

}


void writeDlSurfacePlot(
  FILE *stream,
  DlSurfacePlot *dlSurfacePlot,
  int level)
{
  int i;
  char indent[16];

  for (i = 0;  i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%s\"surface plot\" {",indent);
  writeDlObject(stream,&(dlSurfacePlot->object),level+1);
  writeDlPlotcom(stream,&(dlSurfacePlot->plotcom),level+1);
  fprintf(stream,"\n%s\tdata=\"%s\"",indent,dlSurfacePlot->data);
  fprintf(stream,"\n%s\tdata_clr=\"%d\"",indent,dlSurfacePlot->data_clr);
  fprintf(stream,"\n%s\tdis=\"%d\"",indent,dlSurfacePlot->dis);
  fprintf(stream,"\n%s\txyangle=\"%d\"",indent,dlSurfacePlot->xyangle);
  fprintf(stream,"\n%s\tzangle=\"%d\"",indent,dlSurfacePlot->zangle);
  fprintf(stream,"\n%s}",indent);

}


/****************************************************************
 *****                       nested objects                 *****
 ****************************************************************/


void writeDlMonitor(
  FILE *stream,
  DlMonitor *dlMonitor,
  int level)
{
  int i;
  char indent[16];

  for (i = 0;  i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%smonitor {",indent);
  fprintf(stream,"\n%s\trdbk=\"%s\"",indent,dlMonitor->rdbk);
  fprintf(stream,"\n%s\tclr=%d",indent,dlMonitor->clr);
  fprintf(stream,"\n%s\tbclr=%d",indent,dlMonitor->bclr);
  fprintf(stream,"\n%s}",indent);
}


void writeDlPlotcom(
  FILE *stream,
  DlPlotcom *dlPlotcom,
  int level)
{
  int i;
  char indent[16];

  for (i = 0;  i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%splotcom {",indent);
  fprintf(stream,"\n%s\ttitle=\"%s\"",indent,dlPlotcom->title);
  fprintf(stream,"\n%s\txlabel=\"%s\"",indent,dlPlotcom->xlabel);
  fprintf(stream,"\n%s\tylabel=\"%s\"",indent,dlPlotcom->ylabel);
  fprintf(stream,"\n%s\tclr=%d",indent,dlPlotcom->clr);
  fprintf(stream,"\n%s\tbclr=%d",indent,dlPlotcom->bclr);
  fprintf(stream,"\n%s\tpackage=\"%s\"",indent,dlPlotcom->package);
  fprintf(stream,"\n%s}",indent);

}


void writeDlPen(
  FILE *stream,
  DlPen *dlPen,
  int index,
  int level)
{
  int i;
  char indent[16];

  for (i = 0;  i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%spen[%d] {",indent,index);
  fprintf(stream,"\n%s\tchan=\"%s\"",indent,dlPen->chan);
  fprintf(stream,"\n%s\tclr=%d",indent,dlPen->clr);
  fprintf(stream,"\n%s}",indent);
}


void writeDlTrace(
  FILE *stream,
  DlTrace *dlTrace,
  int index,
  int level)
{
  int i;
  char indent[16];

  for (i = 0;  i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%strace[%d] {",indent,index);
  fprintf(stream,"\n%s\txdata=\"%s\"",indent,dlTrace->xdata);
  fprintf(stream,"\n%s\tydata=\"%s\"",indent,dlTrace->ydata);
  fprintf(stream,"\n%s\tdata_clr=%d",indent,dlTrace->data_clr);
  fprintf(stream,"\n%s}",indent);
}



void writeDlPlotAxisDefinition(
  FILE *stream,
  DlPlotAxisDefinition *dlPlotAxisDefinition,
  int axisNumber,
  int level)
{
  int i;
  char indent[16];

  for (i = 0;  i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  switch (axisNumber) {
    case X_AXIS_ELEMENT: fprintf(stream,"\n%sx_axis {",indent); break;
    case Y1_AXIS_ELEMENT: fprintf(stream,"\n%sy1_axis {",indent); break;
    case Y2_AXIS_ELEMENT: fprintf(stream,"\n%sy2_axis {",indent); break;
  }
  fprintf(stream,"\n%s\taxisStyle=\"%s\"",indent,
	stringValueTable[dlPlotAxisDefinition->axisStyle]);
  fprintf(stream,"\n%s\trangeStyle=\"%s\"",indent,
	stringValueTable[dlPlotAxisDefinition->rangeStyle]);
  fprintf(stream,"\n%s\tminRange=%f",indent,dlPlotAxisDefinition->minRange);
  fprintf(stream,"\n%s\tmaxRange=%f",indent,dlPlotAxisDefinition->maxRange);
  fprintf(stream,"\n%s}",indent);
}

