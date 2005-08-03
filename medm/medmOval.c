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

#define DEBUG_VISIBILITY 0

#include "medm.h"

typedef struct _MedmOval {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           **records;
} MedmOval;

static void ovalDraw(XtPointer cd);
static void ovalUpdateValueCb(XtPointer cd);
static void ovalDestroyCb(XtPointer cd);
static void ovalGetRecord(XtPointer, Record **, int *);
static void ovalGetValues(ResourceBundle *pRCB, DlElement *p);
static void ovalInheritValues(ResourceBundle *pRCB, DlElement *p);
static void ovalSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void ovalGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable ovalDlDispatchTable = {
    createDlOval,
    NULL,
    executeDlOval,
    hideDlOval,
    writeDlOval,
    NULL,
    ovalGetValues,
    ovalInheritValues,
    NULL,
    ovalSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

static void drawOval(MedmOval *po) {
    unsigned int lineWidth;
    DisplayInfo *displayInfo = po->updateTask->displayInfo;
    Widget widget = po->updateTask->displayInfo->drawingArea;
    Display *display = XtDisplay(widget);
    DlOval *dlOval = po->dlElement->structure.oval;

    lineWidth = (dlOval->attr.width+1)/2;
    if(dlOval->attr.fill == F_SOLID) {
	XFillArc(display,displayInfo->updatePixmap,displayInfo->gc,
	  dlOval->object.x,dlOval->object.y,
	  dlOval->object.width,dlOval->object.height,0,360*64);
    } else if(dlOval->attr.fill == F_OUTLINE) {
	XDrawArc(display,displayInfo->updatePixmap,displayInfo->gc,
	  dlOval->object.x + lineWidth,
	  dlOval->object.y + lineWidth,
	  dlOval->object.width - 2*lineWidth,
	  dlOval->object.height - 2*lineWidth,0,360*64);
    }
}

void executeDlOval(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlOval *dlOval = dlElement->structure.oval;

  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(displayInfo->traversalMode == DL_EXECUTE &&
      *dlOval->dynAttr.chan[0]) {
	MedmOval *po;

	if(dlElement->data) {
	    po = (MedmOval *)dlElement->data;
	} else {
	    po = (MedmOval *)malloc(sizeof(MedmOval));
	    dlElement->updateType = DYNAMIC_GRAPHIC;
	    dlElement->data = (void *)po;
	    if(po == NULL) {
		medmPrintf(1,"\nexecuteDlOval: Memory allocation error\n");
		return;
	    }
	  /* Pre-initialize */
	    po->updateTask = NULL;
	    po->records = NULL;
	    po->dlElement = dlElement;

	    po->updateTask = updateTaskAddTask(displayInfo,
	      &(dlOval->object),
	      ovalDraw,
	      (XtPointer)po);

	    if(po->updateTask == NULL) {
		medmPrintf(1,"\nexecuteDlOval: Memory allocation error\n");
	    } else {
		updateTaskAddDestroyCb(po->updateTask,ovalDestroyCb);
		updateTaskAddNameCb(po->updateTask,ovalGetRecord);
	    }
	    if(!isStaticDynamic(&dlOval->dynAttr, True)) {
		po->records = medmAllocateDynamicRecords(&dlOval->dynAttr,
		  ovalUpdateValueCb, NULL, (XtPointer)po);
		calcPostfix(&dlOval->dynAttr);
		setDynamicAttrMonitorFlags(&dlOval->dynAttr, po->records);
	    }
	}
    } else {
      /* Static */
	Drawable drawable=updateInProgress?
	  displayInfo->updatePixmap:displayInfo->drawingAreaPixmap;

	dlElement->updateType = STATIC_GRAPHIC;
	executeDlBasicAttribute(displayInfo,&(dlOval->attr));
	if(dlOval->attr.fill == F_SOLID) {
	    XFillArc(display,drawable,displayInfo->gc,
	      dlOval->object.x,dlOval->object.y,
	      dlOval->object.width,dlOval->object.height,0,360*64);
	} else if(dlOval->attr.fill == F_OUTLINE) {
	    unsigned int lineWidth = (dlOval->attr.width+1)/2;

	    XDrawArc(display,drawable,displayInfo->gc,
	      dlOval->object.x + lineWidth,
	      dlOval->object.y + lineWidth,
	      dlOval->object.width - 2*lineWidth,
	      dlOval->object.height - 2*lineWidth,0,360*64);
	}
    }
}

void hideDlOval(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element drawn on the display drawingArea */
    hideDrawnElement(displayInfo, dlElement);
}

static void ovalUpdateValueCb(XtPointer cd) {
    MedmOval *po = (MedmOval *) ((Record *) cd)->clientData;
#if DEBUG_VISIBILITY
    print("ovalUpdateValueCb: \n");
#endif
    updateTaskMarkUpdate(po->updateTask);
}

static void ovalDraw(XtPointer cd) {
    MedmOval *po = (MedmOval *)cd;
    Record *pR = po->records?po->records[0]:NULL;
    DisplayInfo *displayInfo = po->updateTask->displayInfo;
    XGCValues gcValues;
    unsigned long gcValueMask;
    Display *display = XtDisplay(po->updateTask->displayInfo->drawingArea);
    DlOval *dlOval = po->dlElement->structure.oval;

#if DEBUG_VISIBILITY
    print("ovalDraw: \n");
#endif
    if(isConnected(po->records)) {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	switch (dlOval->dynAttr.clr) {
	case STATIC :
	case DISCRETE:
	    gcValues.foreground = displayInfo->colormap[dlOval->attr.clr];
	    break;
	case ALARM :
	    gcValues.foreground = alarmColor(pR->severity);
	    break;
	default :
	    gcValues.foreground = displayInfo->colormap[dlOval->attr.clr];
	    medmPrintf(1,"\novalDraw: Unknown attribute\n");
	    break;
	}
	gcValues.line_width = dlOval->attr.width;
	gcValues.line_style = ((dlOval->attr.style == SOLID) ?
	  LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

      /* Draw depending on visibility */
	if(calcVisibility(&dlOval->dynAttr, po->records))
	  drawOval(po);
	if(!pR->readAccess) {
	    drawBlackRectangle(po->updateTask);
	}
    } else if(isStaticDynamic(&dlOval->dynAttr, True)) {
      /* clr and vis are both static */
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	gcValues.foreground = displayInfo->colormap[dlOval->attr.clr];
	gcValues.line_width = dlOval->attr.width;
	gcValues.line_style = ((dlOval->attr.style == SOLID) ?
	  LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
	drawOval(po);
    } else {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	gcValues.foreground = WhitePixel(display,DefaultScreen(display));
	gcValues.line_width = dlOval->attr.width;
	gcValues.line_style = ((dlOval->attr.style == SOLID) ?
	  LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
	drawOval(po);
    }
}

static void ovalDestroyCb(XtPointer cd) {
    MedmOval *po = (MedmOval *)cd;

    if(po) {
	Record **records = po->records;

	if(records) {
	    int i;
	    for(i=0; i < MAX_CALC_RECORDS; i++) {
		if(records[i]) medmDestroyRecord(records[i]);
	    }
	    free((char *)records);
	}
	if(po->dlElement) po->dlElement->data = NULL;
	free((char *)po);
    }
    return;
}

static void ovalGetRecord(XtPointer cd, Record **record, int *count) {
    MedmOval *po = (MedmOval *)cd;
    int i;

    *count = 0;
    if(po && po->records) {
	for(i=0; i < MAX_CALC_RECORDS; i++) {
	    if(po->records[i]) {
		record[(*count)++] = po->records[i];
	    }
	}
    }
}

DlElement *createDlOval(DlElement *p)
{
    DlOval *dlOval;
    DlElement *dlElement;

    dlOval = (DlOval *)malloc(sizeof(DlOval));
    if(!dlOval) return 0;
    if(p) {
	*dlOval = *p->structure.oval;
    } else {
	objectAttributeInit(&(dlOval->object));
	basicAttributeInit(&(dlOval->attr));
	dynamicAttributeInit(&(dlOval->dynAttr));
    }

    if(!(dlElement = createDlElement(DL_Oval, (XtPointer)dlOval,
      &ovalDlDispatchTable))) {
	free(dlOval);
    }

    return(dlElement);
}

DlElement *parseOval(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlOval *dlOval;
    DlElement *dlElement = createDlOval(NULL);

    if(!dlElement) return 0;
    dlOval = dlElement->structure.oval;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlOval->object));
	    else if(!strcmp(token,"basic attribute"))
	      parseBasicAttribute(displayInfo,&(dlOval->attr));
	    else if(!strcmp(token,"dynamic attribute"))
	      parseDynamicAttribute(displayInfo,&(dlOval->dynAttr));
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

void writeDlOval(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
    char indent[16];
    DlOval *dlOval = dlElement->structure.oval;

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
  	fprintf(stream,"\n%soval {",indent);
  	writeDlObject(stream,&(dlOval->object),level+1);
  	writeDlBasicAttribute(stream,&(dlOval->attr),level+1);
  	writeDlDynamicAttribute(stream,&(dlOval->dynAttr),level+1);
  	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
  	writeDlBasicAttribute(stream,&(dlOval->attr),level);
  	writeDlDynamicAttribute(stream,&(dlOval->dynAttr),level);
  	fprintf(stream,"\n%soval {",indent);
  	writeDlObject(stream,&(dlOval->object),level+1);
  	fprintf(stream,"\n%s}",indent);
    }
#endif
}

static void ovalInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlOval *dlOval = p->structure.oval;
    medmGetValues(pRCB,
      CLR_RC,        &(dlOval->attr.clr),
      STYLE_RC,      &(dlOval->attr.style),
      FILL_RC,       &(dlOval->attr.fill),
      LINEWIDTH_RC,  &(dlOval->attr.width),
      CLRMOD_RC,     &(dlOval->dynAttr.clr),
      VIS_RC,        &(dlOval->dynAttr.vis),
      VIS_CALC_RC,   &(dlOval->dynAttr.calc),
      CHAN_A_RC,     &(dlOval->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlOval->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlOval->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlOval->dynAttr.chan[3]),
      -1);
}

static void ovalGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlOval *dlOval = p->structure.oval;
    medmGetValues(pRCB,
      X_RC,          &(dlOval->object.x),
      Y_RC,          &(dlOval->object.y),
      WIDTH_RC,      &(dlOval->object.width),
      HEIGHT_RC,     &(dlOval->object.height),
      CLR_RC,        &(dlOval->attr.clr),
      STYLE_RC,      &(dlOval->attr.style),
      FILL_RC,       &(dlOval->attr.fill),
      LINEWIDTH_RC,  &(dlOval->attr.width),
      CLRMOD_RC,     &(dlOval->dynAttr.clr),
      VIS_RC,        &(dlOval->dynAttr.vis),
      VIS_CALC_RC,   &(dlOval->dynAttr.calc),
      CHAN_A_RC,     &(dlOval->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlOval->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlOval->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlOval->dynAttr.chan[3]),
      -1);
}

static void ovalSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlOval *dlOval = p->structure.oval;
    medmGetValues(pRCB,
      CLR_RC,        &(dlOval->attr.clr),
      -1);
}
