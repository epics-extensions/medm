/***************************************************************************
 ****				writeMonitors.c				****
 ****	functions which writeDl a display list out to a file (ascii)	****
 ***************************************************************************/



#include "medm.h"

#include <X11/keysym.h>



/***
 *** Monitors
 ***/


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


void writeDlBar(
  FILE *stream,
  DlBar *dlBar,
  int level)
{
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


void writeDlIndicator(
  FILE *stream,
  DlIndicator *dlIndicator,
  int level)
{
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



void writeDlTextUpdate(
  FILE *stream,
  DlTextUpdate *dlTextUpdate,
  int level)
{
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


void writeDlStripChart(
  FILE *stream,
  DlStripChart *dlStripChart,
  int level)
{
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

