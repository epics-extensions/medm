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
 * .02  09-13-95        vong    conform to c++ syntax
 *
 *****************************************************************************
*/

/***************************************************************************
 ****				writeExtensions.c			****
 ****	functions which writeDl a display list out to a file (ascii)	****
 ***************************************************************************/




#include "medm.h"

#include <X11/keysym.h>



/***
 *** Extensions
 ***/


void writeDlImage(
  FILE *stream,
  DlImage *dlImage,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%simage {",indent);
  writeDlObject(stream,&(dlImage->object),level+1);
  fprintf(stream,"\n%s\ttype=\"%s\"",indent,
	stringValueTable[dlImage->imageType]);
  fprintf(stream,"\n%s\t\"image name\"=\"%s\"",indent,dlImage->imageName);
  fprintf(stream,"\n%s}",indent);
}


/*
 * function to write all children of composite out (properly nested)
 */
void writeDlCompositeChildren(
  FILE *stream,
  DlComposite *dlComposite,
  int level)
{
  int i;
  char indent[16];
  DlElement *element;

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%schildren {",indent);

  element = ((DlElement *)dlComposite->dlElementListHead)->next;
  while (element != NULL) {		/* any union member is okay here */
     (*element->dmWrite)((XtPointer) stream,
                         (XtPointer) element->structure.rectangle,level+1);
     element = element->next;
  }

  fprintf(stream,"\n%s}",indent);
}


void writeDlComposite(
  FILE *stream,
  DlComposite *dlComposite,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%scomposite {",indent);
  writeDlObject(stream,&(dlComposite->object),level+1);
  fprintf(stream,"\n%s\t\"composite name\"=\"%s\"",indent,
		dlComposite->compositeName);
  fprintf(stream,"\n%s\tvis=\"%s\"",indent,stringValueTable[dlComposite->vis]);
  fprintf(stream,"\n%s\tchan=\"%s\"",indent,dlComposite->chan);
  writeDlCompositeChildren(stream,dlComposite,level+1);
  fprintf(stream,"\n%s}",indent);
}


/*
 * function to write all points of polyline out
 */
void writeDlPolylinePoints(
  FILE *stream,
  DlPolyline *dlPolyline,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%spoints {",indent);

  for (i = 0; i < dlPolyline->nPoints; i++) {
    fprintf(stream,"\n%s\t(%d,%d)",indent,
	dlPolyline->points[i].x,dlPolyline->points[i].y);
  }

  fprintf(stream,"\n%s}",indent);
}


void writeDlPolyline(
  FILE *stream,
  DlPolyline *dlPolyline,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%spolyline {",indent);
  writeDlObject(stream,&(dlPolyline->object),level+1);
  writeDlPolylinePoints(stream,dlPolyline,level+1);
  fprintf(stream,"\n%s}",indent);
}


/*
 * function to write all points of polygon out
 */
void writeDlPolygonPoints(
  FILE *stream,
  DlPolygon *dlPolygon,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%spoints {",indent);
  for (i = 0; i < dlPolygon->nPoints; i++) {
    fprintf(stream,"\n%s\t(%d,%d)",indent,
	dlPolygon->points[i].x,dlPolygon->points[i].y);
  }

  fprintf(stream,"\n%s}",indent);
}


void writeDlPolygon(
  FILE *stream,
  DlPolygon *dlPolygon,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%spolygon {",indent);
  writeDlObject(stream,&(dlPolygon->object),level+1);
  writeDlPolygonPoints(stream,dlPolygon,level+1);
  fprintf(stream,"\n%s}",indent);
}

