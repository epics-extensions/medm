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

/***************************************************************************
 **** 			writeControllers.c				****
 ****   functions which writeDl a display list out to a file (ascii)	****
 ***************************************************************************/


#include "medm.h"

#include <X11/keysym.h>



/***
 *** Controllers
 ***/

void writeDlChoiceButton(
  FILE *stream,
  DlChoiceButton *dlChoiceButton,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%s\"choice button\" {",indent);
  writeDlObject(stream,&(dlChoiceButton->object),level+1);
  writeDlControl(stream,&(dlChoiceButton->control),level+1);
  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
		stringValueTable[dlChoiceButton->clrmod]);
  fprintf(stream,"\n%s\tstacking=\"%s\"",indent,
		stringValueTable[dlChoiceButton->stacking]);
  fprintf(stream,"\n%s}",indent);

}


void writeDlMessageButton(
  FILE *stream,
  DlMessageButton *dlMessageButton,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%s\"message button\" {",indent);
  writeDlObject(stream,&(dlMessageButton->object),level+1);
  writeDlControl(stream,&(dlMessageButton->control),level+1);
  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,dlMessageButton->label);
  fprintf(stream,"\n%s\tpress_msg=\"%s\"",indent,dlMessageButton->press_msg);
  fprintf(stream,"\n%s\trelease_msg=\"%s\"",
		indent,dlMessageButton->release_msg);
  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
		stringValueTable[dlMessageButton->clrmod]);
  fprintf(stream,"\n%s}",indent);

}


void writeDlValuator(
  FILE *stream,
  DlValuator *dlValuator,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%svaluator {",indent);
  writeDlObject(stream,&(dlValuator->object),level+1);
  writeDlControl(stream,&(dlValuator->control),level+1);
  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,
		stringValueTable[dlValuator->label]);
  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
		stringValueTable[dlValuator->clrmod]);
  fprintf(stream,"\n%s\tdirection=\"%s\"",indent,
		stringValueTable[dlValuator->direction]);
  fprintf(stream,"\n%s\tdPrecision=%f",indent,dlValuator->dPrecision);
  fprintf(stream,"\n%s}",indent);

}


void writeDlTextEntry(
  FILE *stream,
  DlTextEntry *dlTextEntry,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%s\"text entry\" {",indent);
  writeDlObject(stream,&(dlTextEntry->object),level+1);
  writeDlControl(stream,&(dlTextEntry->control),level+1);
  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
		stringValueTable[dlTextEntry->clrmod]);
  fprintf(stream,"\n%s\tformat=\"%s\"",indent,
                stringValueTable[dlTextEntry->format]);
  fprintf(stream,"\n%s}",indent);

}


void writeDlMenu(
  FILE *stream,
  DlMenu *dlMenu,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%smenu {",indent);
  writeDlObject(stream,&(dlMenu->object),level+1);
  writeDlControl(stream,&(dlMenu->control),level+1);
  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
		stringValueTable[dlMenu->clrmod]);
  fprintf(stream,"\n%s}",indent);

}



/****************************************************************
 *****                      nested objects                  *****
 ****************************************************************/


void writeDlControl(
  FILE *stream,
  DlControl *dlControl,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%scontrol {",indent);
  fprintf(stream,"\n%s\tctrl=\"%s\"",indent,dlControl->ctrl);
  fprintf(stream,"\n%s\tclr=%d",indent,dlControl->clr);
  fprintf(stream,"\n%s\tbclr=%d",indent,dlControl->bclr);
  fprintf(stream,"\n%s}",indent);

}




