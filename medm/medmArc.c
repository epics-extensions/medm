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
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (630-252-2000).
*/
/*****************************************************************************
 *
 *     Original Author : Mark Anderson
 *     Second Author   : Frederick Vong
 *     Third Author    : Kenneth Evans, Jr.
 *
 *****************************************************************************
*/

#include "medm.h"

typedef struct _Arc {
    DlElement        *dlElement;     /* Must be first */
    Record           **records;
    UpdateTask       *updateTask;
} MedmArc;

static void arcDraw(XtPointer cd);
static void arcUpdateValueCb(XtPointer cd);
static void arcDestroyCb(XtPointer cd);
static void arcGetRecord(XtPointer, Record **, int *);
static void arcInheritValues(ResourceBundle *pRCB, DlElement *p);
static void arcSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void arcGetValues(ResourceBundle *pRCB, DlElement *p);
static void arcOrient(DlElement *dlElement, int type, int xCenter,
  int yCenter);

static DlDispatchTable arcDlDispatchTable = {
    createDlArc,
    NULL,
    executeDlArc,
    hideDlArc,
    writeDlArc,
    NULL,
    arcGetValues,
    arcInheritValues,
    NULL,
    arcSetForegroundColor,
    genericMove,
    genericScale,
    arcOrient,
    NULL,
    NULL};

static void drawArc(MedmArc *pa) {
    unsigned int lineWidth;
    DisplayInfo *displayInfo = pa->updateTask->displayInfo;
    Widget widget = pa->updateTask->displayInfo->drawingArea;
    Display *display = XtDisplay(widget);
    DlArc *dlArc = pa->dlElement->structure.arc;

    lineWidth = (dlArc->attr.width+1)/2;
    if (dlArc->attr.fill == F_SOLID) {
	XFillArc(display,XtWindow(widget),displayInfo->gc,
	  dlArc->object.x,dlArc->object.y,
	  dlArc->object.width,dlArc->object.height,dlArc->begin,dlArc->path);
    } else
      if (dlArc->attr.fill == F_OUTLINE) {
	  XDrawArc(display,XtWindow(widget),displayInfo->gc,
	    dlArc->object.x + lineWidth,
	    dlArc->object.y + lineWidth,
	    dlArc->object.width - 2*lineWidth,
	    dlArc->object.height - 2*lineWidth,dlArc->begin,dlArc->path);
      }
}

void executeDlArc(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlArc *dlArc = dlElement->structure.arc;
    if(displayInfo->traversalMode == DL_EXECUTE &&
      *dlArc->dynAttr.chan[0]) {
	MedmArc *pa;
	pa = (MedmArc *) malloc(sizeof(MedmArc));
	pa->dlElement = dlElement;
	pa->updateTask = updateTaskAddTask(displayInfo,
	  &(dlArc->object),
	  arcDraw,
	  (XtPointer)pa);

	if (pa->updateTask == NULL) {
	    medmPrintf(1,"\narcCreateRunTimeInstance: Memory allocation error\n");
	} else {
	    updateTaskAddDestroyCb(pa->updateTask,arcDestroyCb);
	    updateTaskAddNameCb(pa->updateTask,arcGetRecord);
	    pa->updateTask->opaque = False;
	}
	pa->records = medmAllocateDynamicRecords(&dlArc->dynAttr,arcUpdateValueCb,
	  NULL,(XtPointer) pa);
	calcPostfix(&dlArc->dynAttr);
	setMonitorChanged(&dlArc->dynAttr, pa->records);
    } else {
	executeDlBasicAttribute(displayInfo,&(dlArc->attr));
	if (dlArc->attr.fill == F_SOLID) {
	    unsigned int lineWidth = (dlArc->attr.width+1)/2;
	    XFillArc(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	      dlArc->object.x,dlArc->object.y,
	      dlArc->object.width,dlArc->object.height,dlArc->begin,dlArc->path);
	    XFillArc(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
	      dlArc->object.x,dlArc->object.y,
	      dlArc->object.width,dlArc->object.height,dlArc->begin,dlArc->path);

	} else
	  if (dlArc->attr.fill == F_OUTLINE) {
	      unsigned int lineWidth = (dlArc->attr.width+1)/2;
	      XDrawArc(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
		dlArc->object.x + lineWidth,
		dlArc->object.y + lineWidth,
		dlArc->object.width - 2*lineWidth,
		dlArc->object.height - 2*lineWidth,dlArc->begin,dlArc->path);
	      XDrawArc(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
		dlArc->object.x + lineWidth,
		dlArc->object.y + lineWidth,
		dlArc->object.width - 2*lineWidth,
		dlArc->object.height - 2*lineWidth,dlArc->begin,dlArc->path);
	  }
    }
}

void hideDlArc(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element drawn on the display drawingArea */
    hideDrawnElement(displayInfo, dlElement);
}

static void arcUpdateValueCb(XtPointer cd)
{
    MedmArc *pa = (MedmArc *)((Record *) cd)->clientData;
    updateTaskMarkUpdate(pa->updateTask);
}

static void arcDraw(XtPointer cd)
{
    MedmArc *pa = (MedmArc *)cd;
    Record *pd = pa->records[0];
    DisplayInfo *displayInfo = pa->updateTask->displayInfo;
    XGCValues gcValues;
    unsigned long gcValueMask;
    Display *display = XtDisplay(pa->updateTask->displayInfo->drawingArea);
    DlArc *dlArc = pa->dlElement->structure.arc;

    if (pd->connected) {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	switch (dlArc->dynAttr.clr) {
#ifdef __COLOR_RULE_H__
	case STATIC:
	    <<<<<<< medmArc.c
		      gcValues.foreground = displayInfo->colormap[pa->attr.clr];
	    =======
		      gcValues.foreground = displayInfo->dlColormap[dlArc->attr.clr];
		      >>>>>>> 1.7
				break;
	case DISCRETE:
	    gcValues.foreground = extractColor(displayInfo,
	      pd->value,
	      dlArc->dynAttr.colorRule,
	      dlArc->attr.clr);
	    break;
#else
	case STATIC :
	case DISCRETE:
	    gcValues.foreground = displayInfo->colormap[dlArc->attr.clr];
	    break;
#endif
	case ALARM :
	    gcValues.foreground = alarmColor(pd->severity);
	    break;
	default :
	    gcValues.foreground = displayInfo->colormap[dlArc->attr.clr];
	    medmPrintf(1,"\narcUpdatevalueCb: Unknown attribute\n");
	    break;
	}
	gcValues.line_width = dlArc->attr.width;
	gcValues.line_style = ((dlArc->attr.style == SOLID) ? LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

      /* Draw depending on visibility */
	if(calcVisibility(&dlArc->dynAttr, pa->records))
	  drawArc(pa);
	if (pd->readAccess) {
	    if (!pa->updateTask->overlapped && dlArc->dynAttr.vis == V_STATIC) {
		pa->updateTask->opaque = True;
	    }
	} else {
	    pa->updateTask->opaque = False;
	    draw3DQuestionMark(pa->updateTask);
	}
    } else {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	gcValues.foreground = WhitePixel(display,DefaultScreen(display));
	gcValues.line_width = dlArc->attr.width;
	gcValues.line_style = ((dlArc->attr.style == SOLID) ?
	  LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
	drawArc(pa);

    }

  /* Update the drawing objects above */
    redrawElementsAbove(displayInfo, (DlElement *)dlArc);
}

static void arcDestroyCb(XtPointer cd)
{
    MedmArc *pa = (MedmArc *)cd;

    if (pa) {
	Record **records = pa->records;
	
	if(records) {
	    int i;
	    for(i=0; i < MAX_CALC_RECORDS; i++) {
		if(records[i]) medmDestroyRecord(records[i]);
	    }
	    free((char *)records);
	}
	free((char *)pa);
    }
    return;
}

static void arcGetRecord(XtPointer cd, Record **record, int *count)
{
    MedmArc *pa = (MedmArc *)cd;
    int i;
    
    *count = MAX_CALC_RECORDS;
    for(i=0; i < MAX_CALC_RECORDS; i++) {
	record[i] = pa->records[i];
    }
}

DlElement *createDlArc(DlElement *p)
{
    DlArc *dlArc;
    DlElement *dlElement;
 
    dlArc = (DlArc*) malloc(sizeof(DlArc));
    if (!dlArc) return 0;
    if (p) {
	*dlArc = *p->structure.arc;
    } else { 
	objectAttributeInit(&(dlArc->object));
	basicAttributeInit(&(dlArc->attr));
	dynamicAttributeInit(&(dlArc->dynAttr));
	dlArc->begin = 0;
	dlArc->path = 90*64;
    }
 
    if (!(dlElement = createDlElement(DL_Arc,
      (XtPointer)      dlArc,
      &arcDlDispatchTable))) {
	free(dlArc);
    }

    return(dlElement);
}

DlElement *parseArc(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlArc *dlArc;
    DlElement *dlElement = createDlArc(NULL);
    if (!dlElement) return 0;
    dlArc = dlElement->structure.arc;
 
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlArc->object));
	    if (!strcmp(token,"basic attribute"))
	      parseBasicAttribute(displayInfo,&(dlArc->attr));
	    else
	      if (!strcmp(token,"dynamic attribute"))
		parseDynamicAttribute(displayInfo,&(dlArc->dynAttr));
	      else
		if (!strcmp(token,"begin")) {
		    getToken(displayInfo,token);
		    getToken(displayInfo,token);
		    dlArc->begin = atoi(token);
		} else
		  if (!strcmp(token,"path")) {
		      getToken(displayInfo,token);
		      getToken(displayInfo,token);
		      dlArc->path = atoi(token);
		  }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
	}
    } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );

    return dlElement; 
}

void writeDlArc(FILE *stream, DlElement *dlElement, int level)
{
    char indent[16];
    DlArc *dlArc = dlElement->structure.arc;

    memset(indent,'\t',level); 
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
  	fprintf(stream,"\n%sarc {",indent);
  	writeDlObject(stream,&(dlArc->object),level+1);
  	writeDlBasicAttribute(stream,&(dlArc->attr),level+1);
  	writeDlDynamicAttribute(stream,&(dlArc->dynAttr),level+1);
  	fprintf(stream,"\n%s\tbegin=%d",indent,dlArc->begin);
  	fprintf(stream,"\n%s\tpath=%d",indent,dlArc->path);
  	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
  	writeDlObject(stream,&(dlArc->object),level);
  	writeDlBasicAttribute(stream,&(dlArc->attr),level);
  	fprintf(stream,"\n%sarc {",indent);
  	writeDlDynamicAttribute(stream,&(dlArc->dynAttr),level+1);
  	fprintf(stream,"\n%s\tbegin=%d",indent,dlArc->begin);
  	fprintf(stream,"\n%s\tpath=%d",indent,dlArc->path);
  	fprintf(stream,"\n%s}",indent);
    }
#endif
}

void arcOrient(DlElement *dlElement, int type, int xCenter, int yCenter)
{
    DlArc *dlArc = dlElement->structure.arc;

   /* Units are 1/64 degrees */
    switch(type) {
    case ORIENT_HORIZ:
	dlArc->begin = 11520 - dlArc->begin;
	dlArc->begin -= dlArc->path;
	break;
    case ORIENT_VERT:
	dlArc->begin = -dlArc->begin;
	dlArc->begin -= dlArc->path;
	break;
    case ORIENT_CW:
	dlArc->begin -= 5760;
	break;
    case ORIENT_CCW:
	dlArc->begin += 5760;
	break;
    }
    while(dlArc->begin >= 23040) dlArc->begin -= 23040;
    while(dlArc->begin < 0) dlArc->begin += 23040;
    genericOrient(dlElement, type, xCenter, yCenter);
}

static void arcGetValues(ResourceBundle *pRCB, DlElement *p)
{
    DlArc *dlArc = p->structure.arc;
    medmGetValues(pRCB,
      X_RC,          &(dlArc->object.x),
      Y_RC,          &(dlArc->object.y),
      WIDTH_RC,      &(dlArc->object.width),
      HEIGHT_RC,     &(dlArc->object.height),
      CLR_RC,        &(dlArc->attr.clr),
      STYLE_RC,      &(dlArc->attr.style),
      FILL_RC,       &(dlArc->attr.fill),
      LINEWIDTH_RC,  &(dlArc->attr.width),
      CLRMOD_RC,     &(dlArc->dynAttr.clr),
      VIS_RC,        &(dlArc->dynAttr.vis),
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlArc->dynAttr.colorRule),
#endif
      VIS_CALC_RC,   &(dlArc->dynAttr.calc),
      CHAN_A_RC,     &(dlArc->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlArc->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlArc->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlArc->dynAttr.chan[3]),
      BEGIN_RC,      &(dlArc->begin),
      PATH_RC,       &(dlArc->path),
      -1);
}

static void arcInheritValues(ResourceBundle *pRCB, DlElement *p)
{
    DlArc *dlArc = p->structure.arc;
    medmGetValues(pRCB,
      CLR_RC,        &(dlArc->attr.clr),
      STYLE_RC,      &(dlArc->attr.style),
      FILL_RC,       &(dlArc->attr.fill),
      LINEWIDTH_RC,  &(dlArc->attr.width),
      CLRMOD_RC,     &(dlArc->dynAttr.clr),
      VIS_RC,        &(dlArc->dynAttr.vis),
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlArc->dynAttr.colorRule),
#endif
      VIS_CALC_RC,   &(dlArc->dynAttr.calc),
      CHAN_A_RC,     &(dlArc->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlArc->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlArc->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlArc->dynAttr.chan[3]),
      BEGIN_RC,      &(dlArc->begin),
      PATH_RC,       &(dlArc->path),
      -1);
}

static void arcSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlArc *dlArc = p->structure.arc;
    medmGetValues(pRCB,
      CLR_RC,        &(dlArc->attr.clr),
      -1);
}
