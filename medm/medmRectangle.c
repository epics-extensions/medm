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

#define DEBUG_COMPOSITE 0
#define DEBUG_CREATE 0

#include "medm.h"

typedef struct _MedmRectangle {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           **records;
} MedmRectangle;

static void rectangleDraw(XtPointer cd);
static void rectangleUpdateValueCb(XtPointer cd);
static void rectangleDestroyCb(XtPointer cd);
static void rectangleGetRecord(XtPointer, Record **, int *);
static void rectangleInheritValues(ResourceBundle *pRCB, DlElement *p);
static void rectangleSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void rectangleGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable rectangleDlDispatchTable = {
    createDlRectangle,
    NULL,
    executeDlRectangle,
    hideDlRectangle,
    writeDlRectangle,
    NULL,
    rectangleGetValues,
    rectangleInheritValues,
    NULL,
    rectangleSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

static void drawRectangle(MedmRectangle *pr)
{
    unsigned int lineWidth;
    DisplayInfo *displayInfo = pr->updateTask->displayInfo;
    Widget widget = pr->updateTask->displayInfo->drawingArea;
    Display *display = XtDisplay(widget);
    DlRectangle *dlRectangle = pr->dlElement->structure.rectangle;

#if DEBUG_COMPOSITE
    DlObject *po = &(pr->dlElement->structure.composite->object);

    print("drawRectangle: [%d,%d]\n",po->x,po->y);
#endif

    lineWidth = (dlRectangle->attr.width+1)/2;
    if(dlRectangle->attr.fill == F_SOLID) {
	XFillRectangle(display,displayInfo->updatePixmap,displayInfo->gc,
          dlRectangle->object.x,dlRectangle->object.y,
          dlRectangle->object.width,dlRectangle->object.height);
    } else if(dlRectangle->attr.fill == F_OUTLINE) {
	XDrawRectangle(display,displayInfo->updatePixmap,displayInfo->gc,
	  dlRectangle->object.x + lineWidth,
	  dlRectangle->object.y + lineWidth,
	  dlRectangle->object.width - 2*lineWidth,
	  dlRectangle->object.height - 2*lineWidth);
    }
}

void executeDlRectangle(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlRectangle *dlRectangle = dlElement->structure.rectangle;

#if DEBUG_COMPOSITE
    DlObject *po = &(dlElement->structure.composite->object);

    print("executeDlRectangle: {%3d,%3d}{%3d,%3d} data=%x "
      "STATIC_GRAPHIC=%s hidden=%s\n",
      po->x,po->y,po->x+po->width,po->y+po->height,dlElement->data,
      dlElement->updateType == STATIC_GRAPHIC?"True":"False",
      dlElement->hidden?"True":"False");
#endif
  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(displayInfo->traversalMode == DL_EXECUTE &&
      *dlRectangle->dynAttr.chan[0]) {
	MedmRectangle *pr;

	if(dlElement->data) {
	    pr = (MedmRectangle *)dlElement->data;
	} else {
	    pr = (MedmRectangle *)malloc(sizeof(MedmRectangle));
	    dlElement->updateType = DYNAMIC_GRAPHIC;
	    dlElement->data = (void *)pr;
	    if(pr == NULL) {
		medmPrintf(1,"\nexecuteDlRectangle: Memory allocation error\n");
		return;
	    }
	  /* Pre-initialize */
	    pr->updateTask = NULL;
	    pr->records = NULL;
	    pr->dlElement = dlElement;

	    pr->updateTask = updateTaskAddTask(displayInfo,
	      &(dlRectangle->object),rectangleDraw,(XtPointer)pr);

	    if(pr->updateTask == NULL) {
		medmPrintf(1,"\nexecuteDlRectangle: Memory allocation error\n");
	    } else {
		updateTaskAddDestroyCb(pr->updateTask,rectangleDestroyCb);
		updateTaskAddNameCb(pr->updateTask,rectangleGetRecord);
	    }
	    if(!isStaticDynamic(&dlRectangle->dynAttr, True)) {
		pr->records = medmAllocateDynamicRecords(&dlRectangle->dynAttr,
		  rectangleUpdateValueCb,NULL,(XtPointer)pr);
		calcPostfix(&dlRectangle->dynAttr);
		setDynamicAttrMonitorFlags(&dlRectangle->dynAttr, pr->records);
	    }
	}
    } else {
      /* Static */
	Drawable drawable=updateInProgress?
	  displayInfo->updatePixmap:displayInfo->drawingAreaPixmap;

	dlElement->updateType = STATIC_GRAPHIC;
	executeDlBasicAttribute(displayInfo,&(dlRectangle->attr));
#if DEBUG_CREATE
	{
	    XGCValues gcValues;

	    XGetGCValues(display, displayInfo->gc,
	      GCForeground|GCClipXOrigin|GCClipYOrigin, &gcValues);
	    print("executeDlRectangle: fg=%x x_origin=%d y_origin=%d\n",
	      gcValues.foreground,
	      gcValues.clip_x_origin,
	      gcValues.clip_y_origin);
	}
#endif
	if(dlRectangle->attr.fill == F_SOLID) {
	    XFillRectangle(display,drawable,
	      displayInfo->gc,
	      dlRectangle->object.x,dlRectangle->object.y,
	      dlRectangle->object.width,dlRectangle->object.height);
	} else if(dlRectangle->attr.fill == F_OUTLINE) {
	    unsigned int lineWidth = (dlRectangle->attr.width+1)/2;

	    XDrawRectangle(display,drawable,
	      displayInfo->gc,
	      dlRectangle->object.x + lineWidth,
	      dlRectangle->object.y + lineWidth,
	      dlRectangle->object.width - 2*lineWidth,
	      dlRectangle->object.height - 2*lineWidth);
	}
    }
}

void hideDlRectangle(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element drawn on the display drawingArea */
    hideDrawnElement(displayInfo, dlElement);
}

static void rectangleUpdateValueCb(XtPointer cd)
{
    MedmRectangle *pr = (MedmRectangle *)((Record *)cd)->clientData;

#if DEBUG_COMPOSITE
    DlObject *po = &(pr->dlElement->structure.composite->object);

    print("rectangleUpdateValueCb: [%d,%d]\n",po->x,po->y);
#endif
    updateTaskMarkUpdate(pr->updateTask);
}

static void rectangleDraw(XtPointer cd)
{
    MedmRectangle *pr = (MedmRectangle *)cd;
    Record *pRec = pr->records?pr->records[0]:NULL;
    DisplayInfo *displayInfo = pr->updateTask->displayInfo;
    XGCValues gcValues;
    unsigned long gcValueMask;
    Display *display = XtDisplay(pr->updateTask->displayInfo->drawingArea);
    DlRectangle *dlRectangle = pr->dlElement->structure.rectangle;

#if DEBUG_COMPOSITE
    DlObject *po = &(pr->dlElement->structure.composite->object);

    print("rectangleDraw: [%d,%d] value=%g\n",po->x,po->y,pRec->value);
#endif

    if(isConnected(pr->records)) {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	switch (dlRectangle->dynAttr.clr) {
	case STATIC:
	case DISCRETE:
	    gcValues.foreground = displayInfo->colormap[dlRectangle->attr.clr];
	    break;
	case ALARM:
	    gcValues.foreground = alarmColor(pRec->severity);
	    break;
	}
	gcValues.line_width = dlRectangle->attr.width;
	gcValues.line_style = ( (dlRectangle->attr.style == SOLID) ?
	  LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

      /* Draw depending on visibility */
	if(calcVisibility(&dlRectangle->dynAttr, pr->records))
	  drawRectangle(pr);
	if(!pRec->readAccess) {
	    drawBlackRectangle(pr->updateTask);
	}
    } else if(isStaticDynamic(&dlRectangle->dynAttr, True)) {
      /* clr and vis are both static */
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	gcValues.foreground = displayInfo->colormap[dlRectangle->attr.clr];
	gcValues.line_width = dlRectangle->attr.width;
	gcValues.line_style = ( (dlRectangle->attr.style == SOLID) ?
	  LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
	drawRectangle(pr);
    } else {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	gcValues.foreground = WhitePixel(display,DefaultScreen(display));
	gcValues.line_width = dlRectangle->attr.width;
	gcValues.line_style = ((dlRectangle->attr.style == SOLID) ?
	  LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
	drawRectangle(pr);
    }
}

static void rectangleDestroyCb(XtPointer cd)
{
    MedmRectangle *pr = (MedmRectangle *)cd;

    if(pr) {
	Record **records = pr->records;

	if(records) {
	    int i;
	    for(i=0; i < MAX_CALC_RECORDS; i++) {
		if(records[i]) medmDestroyRecord(records[i]);
	    }
	    free((char *)records);
	}
	if(pr->dlElement) pr->dlElement->data = NULL;
	free((char *)pr);
    }
    return;
}

static void rectangleGetRecord(XtPointer cd, Record **record, int *count)
{
    MedmRectangle *pr = (MedmRectangle *)cd;
    int i;

    *count = 0;
    if(pr && pr->records) {
	for(i=0; i < MAX_CALC_RECORDS; i++) {
	    if(pr->records[i]) {
		record[(*count)++] = pr->records[i];
	    }
	}
    }
}

DlElement *createDlRectangle(DlElement *p)
{
    DlRectangle *dlRectangle;
    DlElement *dlElement;

    dlRectangle = (DlRectangle *)malloc(sizeof(DlRectangle));
    if(!dlRectangle) return 0;
    if(p) {
	*dlRectangle = *p->structure.rectangle;
    } else {
	objectAttributeInit(&(dlRectangle->object));
	basicAttributeInit(&(dlRectangle->attr));
	dynamicAttributeInit(&(dlRectangle->dynAttr));
    }

    if(!(dlElement = createDlElement(DL_Rectangle,
      (XtPointer)dlRectangle,
      &rectangleDlDispatchTable))) {
	free(dlRectangle);
    }

    return(dlElement);
}

DlElement *parseRectangle(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlRectangle *dlRectangle;
    DlElement *dlElement = createDlRectangle(NULL);

    if(!dlElement) return 0;
    dlRectangle = dlElement->structure.rectangle;
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlRectangle->object));
	    else if(!strcmp(token,"basic attribute"))
	      parseBasicAttribute(displayInfo,&(dlRectangle->attr));
	    else if(!strcmp(token,"dynamic attribute"))
	      parseDynamicAttribute(displayInfo,&(dlRectangle->dynAttr));
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

void writeDlRectangle(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
    char indent[16];
    DlRectangle *dlRectangle = dlElement->structure.rectangle;

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
  	fprintf(stream,"\n%srectangle {",indent);
  	writeDlObject(stream,&(dlRectangle->object),level+1);
  	writeDlBasicAttribute(stream,&(dlRectangle->attr),level+1);
  	writeDlDynamicAttribute(stream,&(dlRectangle->dynAttr),level+1);
  	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
  	writeDlBasicAttribute(stream,&(dlRectangle->attr),level);
  	writeDlDynamicAttribute(stream,&(dlRectangle->dynAttr),level);
  	fprintf(stream,"\n%srectangle {",indent);
  	writeDlObject(stream,&(dlRectangle->object),level+1);
  	fprintf(stream,"\n%s}",indent);
    }
#endif
}

static void rectangleInheritValues(ResourceBundle *pRCB, DlElement *p)
{
    DlRectangle *dlRectangle = p->structure.rectangle;
    medmGetValues(pRCB,
      CLR_RC,        &(dlRectangle->attr.clr),
      STYLE_RC,      &(dlRectangle->attr.style),
      FILL_RC,       &(dlRectangle->attr.fill),
      LINEWIDTH_RC,  &(dlRectangle->attr.width),
      CLRMOD_RC,     &(dlRectangle->dynAttr.clr),
      VIS_RC,        &(dlRectangle->dynAttr.vis),
      VIS_CALC_RC,   &(dlRectangle->dynAttr.calc),
      CHAN_A_RC,     &(dlRectangle->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlRectangle->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlRectangle->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlRectangle->dynAttr.chan[3]),
      -1);
}

static void rectangleGetValues(ResourceBundle *pRCB, DlElement *p)
{
    DlRectangle *dlRectangle = p->structure.rectangle;
    medmGetValues(pRCB,
      X_RC,          &(dlRectangle->object.x),
      Y_RC,          &(dlRectangle->object.y),
      WIDTH_RC,      &(dlRectangle->object.width),
      HEIGHT_RC,     &(dlRectangle->object.height),
      CLR_RC,        &(dlRectangle->attr.clr),
      STYLE_RC,      &(dlRectangle->attr.style),
      FILL_RC,       &(dlRectangle->attr.fill),
      LINEWIDTH_RC,  &(dlRectangle->attr.width),
      CLRMOD_RC,     &(dlRectangle->dynAttr.clr),
      VIS_RC,        &(dlRectangle->dynAttr.vis),
      VIS_CALC_RC,   &(dlRectangle->dynAttr.calc),
      CHAN_A_RC,     &(dlRectangle->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlRectangle->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlRectangle->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlRectangle->dynAttr.chan[3]),
      -1);
}

static void rectangleSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlRectangle *dlRectangle = p->structure.rectangle;
    medmGetValues(pRCB,
      CLR_RC,        &(dlRectangle->attr.clr),
      -1);
}
