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

#define DEBUG_VISIBILITY 0

#include "medm.h"

typedef struct _Oval {
    DlElement        *dlElement;     /* Must be first */
    Record           **records;
    UpdateTask       *updateTask;
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
    if (dlOval->attr.fill == F_SOLID) {
	XFillArc(display,XtWindow(widget),displayInfo->gc,
	  dlOval->object.x,dlOval->object.y,
	  dlOval->object.width,dlOval->object.height,0,360*64);
    } else
      if (dlOval->attr.fill == F_OUTLINE) {
	  XDrawArc(display,XtWindow(widget),displayInfo->gc,
	    dlOval->object.x + lineWidth,
	    dlOval->object.y + lineWidth,
	    dlOval->object.width - 2*lineWidth,
	    dlOval->object.height - 2*lineWidth,0,360*64);
      }
}

void executeDlOval(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlOval *dlOval = dlElement->structure.oval;
    if(displayInfo->traversalMode == DL_EXECUTE &&
      *dlOval->dynAttr.chan[0]) {
	MedmOval *po;
	
	po = (MedmOval *) malloc(sizeof(MedmOval));
	po->dlElement = dlElement;
	po->updateTask = updateTaskAddTask(displayInfo,
	  &(dlOval->object),
	  ovalDraw,
	  (XtPointer)po);

	if (po->updateTask == NULL) {
	    medmPrintf(1,"\novalCreateRunTimeInstance: Memory allocation error\n");
	} else {
	    updateTaskAddDestroyCb(po->updateTask,ovalDestroyCb);
	    updateTaskAddNameCb(po->updateTask,ovalGetRecord);
	    po->updateTask->opaque = False;
	}
	po->records = medmAllocateDynamicRecords(&dlOval->dynAttr,
	  ovalUpdateValueCb, NULL, (XtPointer)po);
	calcPostfix(&dlOval->dynAttr);
	setMonitorChanged(&dlOval->dynAttr, po->records);
    } else {
	executeDlBasicAttribute(displayInfo,&(dlOval->attr));
	if (dlOval->attr.fill == F_SOLID) {
	    unsigned int lineWidth = (dlOval->attr.width+1)/2;
	    XFillArc(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	      dlOval->object.x,dlOval->object.y,
	      dlOval->object.width,dlOval->object.height,0,360*64);
	    XFillArc(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
	      dlOval->object.x,dlOval->object.y,
	      dlOval->object.width,dlOval->object.height,0,360*64);

	} else
	  if (dlOval->attr.fill == F_OUTLINE) {
	      unsigned int lineWidth = (dlOval->attr.width+1)/2;
	      XDrawArc(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
		dlOval->object.x + lineWidth,
		dlOval->object.y + lineWidth,
		dlOval->object.width - 2*lineWidth,
		dlOval->object.height - 2*lineWidth,0,360*64);
	      XDrawArc(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
		dlOval->object.x + lineWidth,
		dlOval->object.y + lineWidth,
		dlOval->object.width - 2*lineWidth,
		dlOval->object.height - 2*lineWidth,0,360*64);
	  }
    }
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
    Record *pd = po->records[0];
    DisplayInfo *displayInfo = po->updateTask->displayInfo;
    XGCValues gcValues;
    unsigned long gcValueMask;
    Display *display = XtDisplay(po->updateTask->displayInfo->drawingArea);
    DlOval *dlOval = po->dlElement->structure.oval;

#if DEBUG_VISIBILITY
    print("ovalDraw: \n");
#endif    
    if(pd->connected) {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	switch (dlOval->dynAttr.clr) {
#ifdef __COLOR_RULE_H__
	case STATIC :
	    gcValues.foreground = displayInfo->dlColormap[dlOval->attr.clr];
	    break;
	case DISCRETE:
	    gcValues.foreground = extractColor(displayInfo,
	      pd->value,
	      dlOval->dynAttr.colorRule,
	      dlOval->attr.clr);
	    break;
#else
	case STATIC :
	case DISCRETE:
	    gcValues.foreground = displayInfo->colormap[dlOval->attr.clr];
	    break;
#endif
	case ALARM :
	    gcValues.foreground = alarmColor(pd->severity);
	    break;
	}
	gcValues.line_width = dlOval->attr.width;
	gcValues.line_style = ((dlOval->attr.style == SOLID) ? LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

      /* Draw depending on visibility */
	if(calcVisibility(&dlOval->dynAttr, po->records))
	  drawOval(po);
	if (pd->readAccess) {
	    if (!po->updateTask->overlapped && dlOval->dynAttr.vis == V_STATIC) {
		po->updateTask->opaque = True;
	    }
	} else {
	    po->updateTask->opaque = False;
	    draw3DQuestionMark(po->updateTask);
	}
    } else {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	gcValues.foreground = WhitePixel(display,DefaultScreen(display));
	gcValues.line_width = dlOval->attr.width;
	gcValues.line_style = ((dlOval->attr.style == SOLID) ? LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
	drawOval(po);
    }

  /* Update the drawing objects above */
    redrawElementsAbove(displayInfo, (DlElement *)dlOval);
}

static void ovalDestroyCb(XtPointer cd) {
    MedmOval *po = (MedmOval *)cd;

    if (po) {
	Record **records = po->records;
	
	if(records) {
	    int i;
	    for(i=0; i < MAX_CALC_RECORDS; i++) {
		if(records[i]) medmDestroyRecord(records[i]);
	    }
	    free((char *)records);
	}
	free((char *)po);
    }
    return;
}

static void ovalGetRecord(XtPointer cd, Record **record, int *count) {
    MedmOval *po = (MedmOval *)cd;
    int i;
    
    *count = MAX_CALC_RECORDS;
    for(i=0; i < MAX_CALC_RECORDS; i++) {
	record[i] = po->records[i];
    }
}

DlElement *createDlOval(DlElement *p)
{
    DlOval *dlOval;
    DlElement *dlElement;
 
    dlOval = (DlOval *)malloc(sizeof(DlOval));
    if (!dlOval) return 0;
    if (p) {
	*dlOval = *p->structure.oval;
    } else {
	objectAttributeInit(&(dlOval->object));
	basicAttributeInit(&(dlOval->attr));
	dynamicAttributeInit(&(dlOval->dynAttr));
    }
 
    if (!(dlElement = createDlElement(DL_Oval, (XtPointer)dlOval,
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

    if (!dlElement) return 0;
    dlOval = dlElement->structure.oval;
 
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlOval->object));
	    else
	      if (!strcmp(token,"basic attribute"))
		parseBasicAttribute(displayInfo,&(dlOval->attr));
	      else
		if (!strcmp(token,"dynamic attribute"))
		  parseDynamicAttribute(displayInfo,&(dlOval->dynAttr));
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
    if (MedmUseNewFileFormat) {
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
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlOval->dynAttr.colorRule),
#endif
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
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlOval->dynAttr.colorRule),
#endif
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
