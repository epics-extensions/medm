/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*****************************************************************************
 *
 *     Original Author : Mark Anderson
 *     Second Author   : Frederick Vong
 *     Third Author    : Kenneth Evans, Jr.
 *
 *****************************************************************************
*/

#include "medm.h"

typedef struct _MedmArc {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           **records;
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
    if(dlArc->attr.fill == F_SOLID) {
	XFillArc(display,displayInfo->updatePixmap,displayInfo->gc,
	  dlArc->object.x,dlArc->object.y,
	  dlArc->object.width,dlArc->object.height,dlArc->begin,dlArc->path);
    } else if(dlArc->attr.fill == F_OUTLINE) {
	XDrawArc(display, displayInfo->updatePixmap, displayInfo->gc,
	  dlArc->object.x + lineWidth,
	  dlArc->object.y + lineWidth,
	  dlArc->object.width - 2*lineWidth,
	  dlArc->object.height - 2*lineWidth,dlArc->begin,dlArc->path);
    }
}

void executeDlArc(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlArc *dlArc = dlElement->structure.arc;

  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(displayInfo->traversalMode == DL_EXECUTE &&
      *dlArc->dynAttr.chan[0]) {
	MedmArc *pa;

	if(dlElement->data) {
	    pa = (MedmArc *)dlElement->data;
	} else {
	    pa = (MedmArc *)malloc(sizeof(MedmArc));
	    dlElement->data = (void *)pa;
	    dlElement->updateType = DYNAMIC_GRAPHIC;
	    if(pa == NULL) {
		medmPrintf(1,"\nexecuteDlArc: Memory allocation error\n");
		return;
	    }
	  /* Pre-initialize */
	    pa->updateTask = NULL;
	    pa->records = NULL;
	    pa->dlElement = dlElement;

	    pa->updateTask = updateTaskAddTask(displayInfo,
	      &(dlArc->object),
	      arcDraw,
	      (XtPointer)pa);

	    if(pa->updateTask == NULL) {
		medmPrintf(1,"\nexecuteDlArc: Memory allocation error\n");
	    } else {
		updateTaskAddDestroyCb(pa->updateTask,arcDestroyCb);
		updateTaskAddNameCb(pa->updateTask,arcGetRecord);
	    }
	    if(!isStaticDynamic(&dlArc->dynAttr, True)) {
		pa->records = medmAllocateDynamicRecords(&dlArc->dynAttr,
		  arcUpdateValueCb, NULL, (XtPointer) pa);
		calcPostfix(&dlArc->dynAttr);
		setDynamicAttrMonitorFlags(&dlArc->dynAttr, pa->records);
	    }
	}
    } else {
      /* Static */
	Drawable drawable=updateInProgress?
	  displayInfo->updatePixmap:displayInfo->drawingAreaPixmap;

	dlElement->updateType = STATIC_GRAPHIC;
	executeDlBasicAttribute(displayInfo,&(dlArc->attr));
	if(dlArc->attr.fill == F_SOLID) {
	    XFillArc(display,drawable,displayInfo->gc,
	      dlArc->object.x,dlArc->object.y,
	      dlArc->object.width,dlArc->object.height,dlArc->begin,dlArc->path);
	} else if(dlArc->attr.fill == F_OUTLINE) {
	    unsigned int lineWidth = (dlArc->attr.width+1)/2;

	    XDrawArc(display,drawable,displayInfo->gc,
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
    Record *pR = pa->records?pa->records[0]:NULL;
    DisplayInfo *displayInfo = pa->updateTask->displayInfo;
    XGCValues gcValues;
    unsigned long gcValueMask;
    Display *display = XtDisplay(pa->updateTask->displayInfo->drawingArea);
    DlArc *dlArc = pa->dlElement->structure.arc;

    if(isConnected(pa->records)) {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	switch (dlArc->dynAttr.clr) {
	case STATIC :
	case DISCRETE:
	    gcValues.foreground = displayInfo->colormap[dlArc->attr.clr];
	    break;
	case ALARM :
	    gcValues.foreground = alarmColor(pR->severity);
	    break;
	default :
	    gcValues.foreground = displayInfo->colormap[dlArc->attr.clr];
	    medmPrintf(1,"\narcDraw: Unknown attribute\n");
	    break;
	}
	gcValues.line_width = dlArc->attr.width;
	gcValues.line_style = ((dlArc->attr.style == SOLID) ?
	  LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

      /* Draw depending on visibility */
	if(calcVisibility(&dlArc->dynAttr, pa->records))
	  drawArc(pa);
	if(!pR->readAccess) {
	    drawBlackRectangle(pa->updateTask);
	}
    } else if(isStaticDynamic(&dlArc->dynAttr, True)) {
      /* clr and vis are both static */
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	gcValues.foreground = displayInfo->colormap[dlArc->attr.clr];
	gcValues.line_width = dlArc->attr.width;
	gcValues.line_style = ((dlArc->attr.style == SOLID) ?
	  LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
	drawArc(pa);
    } else {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	gcValues.foreground = WhitePixel(display,DefaultScreen(display));
	gcValues.line_width = dlArc->attr.width;
	gcValues.line_style = ((dlArc->attr.style == SOLID) ?
	  LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
	drawArc(pa);

    }
}

static void arcDestroyCb(XtPointer cd)
{
    MedmArc *pa = (MedmArc *)cd;

    if(pa) {
	Record **records = pa->records;

	if(records) {
	    int i;
	    for(i=0; i < MAX_CALC_RECORDS; i++) {
		if(records[i]) medmDestroyRecord(records[i]);
	    }
	    free((char *)records);
	}
	if(pa->dlElement) pa->dlElement->data = NULL;
	free((char *)pa);
    }
    return;
}

static void arcGetRecord(XtPointer cd, Record **record, int *count)
{
    MedmArc *pa = (MedmArc *)cd;
    int i;

    *count = 0;
    if(pa && pa->records) {
	for(i=0; i < MAX_CALC_RECORDS; i++) {
	    if(pa->records[i]) {
		record[(*count)++] = pa->records[i];
	    }
	}
    }
}

DlElement *createDlArc(DlElement *p)
{
    DlArc *dlArc;
    DlElement *dlElement;

    dlArc = (DlArc*)malloc(sizeof(DlArc));
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
	    nestingLevel++;
	    break;
	case T_RIGHT_BRACE:
	    nestingLevel--;
	    break;
	  default:
	    break;
	}
    } while( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
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

static void arcOrient(DlElement *dlElement, int type, int xCenter, int yCenter)
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
