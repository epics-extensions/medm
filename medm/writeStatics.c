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
 * .03  09-07-95        vong    remove all fallingLine and risingLine stuff
 * .04  09-13-95        vong    conform to c++ syntax
 *
 *****************************************************************************
*/

/***************************************************************************
 ****				writeStatics.c				****
 ****	functions which writeDl a display list out to a file (ascii)	****
 ***************************************************************************/



#include "medm.h"

#include <X11/keysym.h>


/***
 *** Statics
 ***/


void writeDlFile(
  FILE *stream,
  DlFile *dlFile,
  int level)
{
  int i;
  char indent[16];
  int versionNumber = MEDM_VERSION * 10000 + MEDM_REVISION * 100 + MEDM_UPDATE_LEVEL;

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%sfile {",indent);
  fprintf(stream,"\n%s\tname=\"%s\"",indent,dlFile->name);
  fprintf(stream,"\n%s\tversion=%06d",indent,versionNumber);
  fprintf(stream,"\n%s}",indent);
}


void writeDlDisplay(
  FILE *stream,
  DlDisplay *dlDisplay,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%sdisplay {",indent);
  writeDlObject(stream,&(dlDisplay->object),level+1);
  fprintf(stream,"\n%s\tclr=%d",indent,dlDisplay->clr);
  fprintf(stream,"\n%s\tbclr=%d",indent,dlDisplay->bclr);
  fprintf(stream,"\n%s\tcmap=\"%s\"",indent,dlDisplay->cmap);
  fprintf(stream,"\n%s}",indent);

}



void writeDlColormap(
  FILE *stream,
  DlColormap *dlColormap,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%s\"color map\" {",indent);
  fprintf(stream,"\n%s\tncolors=%d",indent,dlColormap->ncolors);
  for (i = 0; i < dlColormap->ncolors; i++) {
    writeDlColormapEntry(stream,&(dlColormap->dl_color[i]),level+1);
  }
  fprintf(stream,"\n%s}",indent);

}



void writeDlBasicAttribute(
  FILE *stream,
  DlBasicAttribute *dlBasicAttribute,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%s\"basic attribute\" {",indent);
  writeDlAttr(stream,&(dlBasicAttribute->attr),level+1);
  fprintf(stream,"\n%s}",indent);
}



void writeDlDynamicAttribute(
  FILE *stream,
  DlDynamicAttribute *dlDynamicAttribute,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%s\"dynamic attribute\" {",indent);
  writeDlDynAttr(stream,&(dlDynamicAttribute->attr),level+1);
  fprintf(stream,"\n%s}",indent);
}





void writeDlRectangle(
  FILE *stream,
  DlRectangle *dlRectangle,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%srectangle {",indent);
  writeDlObject(stream,&(dlRectangle->object),level+1);
  fprintf(stream,"\n%s}",indent);
}


void writeDlOval(
  FILE *stream,
  DlOval *dlOval,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%soval {",indent);
  writeDlObject(stream,&(dlOval->object),level+1);
  fprintf(stream,"\n%s}",indent);
}



void writeDlArc(
  FILE *stream,
  DlArc *dlArc,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%sarc {",indent);
  writeDlObject(stream,&(dlArc->object),level+1);
  fprintf(stream,"\n%s\tbegin=%d",indent,dlArc->begin);
  fprintf(stream,"\n%s\tpath=%d",indent,dlArc->path);
  fprintf(stream,"\n%s}",indent);
}


void writeDlText(
  FILE *stream,
  DlText *dlText,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%stext {",indent);
  writeDlObject(stream,&(dlText->object),level+1);
  fprintf(stream,"\n%s\ttextix=\"%s\"",indent,dlText->textix);
  fprintf(stream,"\n%s\talign=\"%s\"",indent,stringValueTable[dlText->align]);
  fprintf(stream,"\n%s}",indent);
}


void writeDlRelatedDisplay(
  FILE *stream,
  DlRelatedDisplay *dlRelatedDisplay,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%s\"related display\" {",indent);
  writeDlObject(stream,&(dlRelatedDisplay->object),level+1);
  for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
    writeDlRelatedDisplayEntry(stream,&(dlRelatedDisplay->display[i]),
			i,level+1);
  }
  fprintf(stream,"\n%s\tclr=%d",indent,dlRelatedDisplay->clr);
  fprintf(stream,"\n%s\tbclr=%d",indent,dlRelatedDisplay->bclr);
  fprintf(stream,"\n%s}",indent);
}



void writeDlShellCommand(
  FILE *stream,
  DlShellCommand *dlShellCommand,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%s\"shell command\" {",indent);
  writeDlObject(stream,&(dlShellCommand->object),level+1);
  for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
    writeDlShellCommandEntry(stream,&(dlShellCommand->command[i]),
			i,level+1);
  }
  fprintf(stream,"\n%s\tclr=%d",indent,dlShellCommand->clr);
  fprintf(stream,"\n%s\tbclr=%d",indent,dlShellCommand->bclr);
  fprintf(stream,"\n%s}",indent);
}






/**********************************************************************
 *********                  nested objects                    *********
 **********************************************************************/



void writeDlColormapEntry(
  FILE *stream,
  DlColormapEntry *dlColormapEntry,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%sdl_color {",indent);
  fprintf(stream,"\n%s\tr=%d",indent,dlColormapEntry->r);
  fprintf(stream,"\n%s\tg=%d",indent,dlColormapEntry->g);
  fprintf(stream,"\n%s\tb=%d",indent,dlColormapEntry->b);
  fprintf(stream,"\n%s\tinten=%d",indent,dlColormapEntry->inten);
  fprintf(stream,"\n%s}",indent);
}




void writeDlObject(
  FILE *stream,
  DlObject *dlObject,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%sobject {",indent);
  fprintf(stream,"\n%s\tx=%d",indent,dlObject->x);
  fprintf(stream,"\n%s\ty=%d",indent,dlObject->y);
  fprintf(stream,"\n%s\twidth=%d",indent,dlObject->width);
  fprintf(stream,"\n%s\theight=%d",indent,dlObject->height);
  fprintf(stream,"\n%s}",indent);
}



void writeDlAttr(
  FILE *stream,
  DlAttribute *dlAttr,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%sattr {",indent);
  fprintf(stream,"\n%s\tclr=%d",indent,dlAttr->clr);

  fprintf(stream,"\n%s\tstyle=\"%s\"",indent,stringValueTable[dlAttr->style]);
  fprintf(stream,"\n%s\tfill=\"%s\"",indent,stringValueTable[dlAttr->fill]);
  fprintf(stream,"\n%s\twidth=%d",indent,dlAttr->width);
  fprintf(stream,"\n%s}",indent);
}




void writeDlDynAttr(
  FILE *stream,
  DlDynamicAttributeData *dynAttr,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%sattr {",indent);
  writeDlDynAttrMod(stream,&(dynAttr->mod),level+1);
  writeDlDynAttrParam(stream,&(dynAttr->param),level+1);
  fprintf(stream,"\n%s}",indent);
}


void writeDlDynAttrMod(
  FILE *stream,
  DlDynamicAttrMod *dynAttr,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%smod {",indent);
  fprintf(stream,"\n%s\tclr=\"%s\"",indent,stringValueTable[dynAttr->clr]);
  fprintf(stream,"\n%s\tvis=\"%s\"",indent,stringValueTable[dynAttr->vis]);
#ifdef __COLOR_RULE_H__
  fprintf(stream,"\n%s\tcolorRule=\"set#%d\"",indent,dynAttr->colorRule + 1);
#endif
  fprintf(stream,"\n%s}",indent);
}



void writeDlDynAttrParam(
  FILE *stream,
  DlDynamicAttrParam *dynAttr,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%sparam {",indent);
  fprintf(stream,"\n%s\tchan=\"%s\"",indent,dynAttr->chan);
  fprintf(stream,"\n%s}",indent);
}




void writeDlRelatedDisplayEntry(
  FILE *stream,
  DlRelatedDisplayEntry *entry,
  int index,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%sdisplay[%d] {",indent,index);
  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,entry->label);
  fprintf(stream,"\n%s\tname=\"%s\"",indent,entry->name);
  fprintf(stream,"\n%s\targs=\"%s\"",indent,entry->args);
  fprintf(stream,"\n%s}",indent);
}




void writeDlShellCommandEntry(
  FILE *stream,
  DlShellCommandEntry *entry,
  int index,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%scommand[%d] {",indent,index);
  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,entry->label);
  fprintf(stream,"\n%s\tname=\"%s\"",indent,entry->command);
  fprintf(stream,"\n%s\targs=\"%s\"",indent,entry->args);
  fprintf(stream,"\n%s}",indent);
}

