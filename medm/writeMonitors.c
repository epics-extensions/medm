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
 * .02  09-05-95        vong    2.1.0 release
 * .03  09-13-95        vong    conform to c++ syntax
 *
 *****************************************************************************
*/

/****************************************************************************
 * writeMonitors.c - Functions which save an ascii display file             *
 * Mods: DMW - writeDlByte - Write Byte structure to display file.          *
 ****************************************************************************/
#include "medm.h"
#include <X11/keysym.h>

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

