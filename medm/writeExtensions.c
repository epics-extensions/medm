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
     (*element->dmWrite)(stream,element->structure.rectangle,level+1);
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
  DlElement *element;

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
  DlElement *element;

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

