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

typedef struct _Rectangle {
    DlElement        *dlElement;
    Record           *record;
    UpdateTask       *updateTask;
} Rectangle;

static void rectangleDraw(XtPointer cd);
static void rectangleUpdateValueCb(XtPointer cd);
static void rectangleDestroyCb(XtPointer cd);
static void rectangleName(XtPointer, char **, short *, int *);
static void rectangleInheritValues(ResourceBundle *pRCB, DlElement *p);
static void rectangleSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void rectangleGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable rectangleDlDispatchTable = {
    createDlRectangle,
    destroyElementWithDynamicAttribute,
    executeDlRectangle,
    writeDlRectangle,
    NULL,
    rectangleGetValues,
    rectangleInheritValues,
    NULL,
    rectangleSetForegroundColor,
    genericMove,
    genericScale,
    NULL,
    NULL};

static void drawRectangle(Rectangle *pr) {
    unsigned int lineWidth;
    DisplayInfo *displayInfo = pr->updateTask->displayInfo;
    Widget widget = pr->updateTask->displayInfo->drawingArea;
    Display *display = XtDisplay(widget);
    DlRectangle *dlRectangle = pr->dlElement->structure.rectangle;

    lineWidth = (dlRectangle->attr.width+1)/2;
    if (dlRectangle->attr.fill == F_SOLID) {
	XFillRectangle(display,XtWindow(widget),displayInfo->gc,
          dlRectangle->object.x,dlRectangle->object.y,
          dlRectangle->object.width,dlRectangle->object.height);
    } else
      if (dlRectangle->attr.fill == F_OUTLINE) {
	  XDrawRectangle(display,XtWindow(widget),displayInfo->gc,
	    dlRectangle->object.x + lineWidth,
	    dlRectangle->object.y + lineWidth,
	    dlRectangle->object.width - 2*lineWidth,
	    dlRectangle->object.height - 2*lineWidth);
      }
}

void executeDlRectangle(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlRectangle *dlRectangle = dlElement->structure.rectangle;
    if (displayInfo->traversalMode == DL_EXECUTE) {
	Rectangle *pr;
	pr = (Rectangle *) malloc(sizeof(Rectangle));
	pr->dlElement = dlElement;
	pr->updateTask = updateTaskAddTask(displayInfo,
	  &(dlRectangle->object),
	  rectangleDraw,
	  (XtPointer)pr);

	if (pr->updateTask == NULL) {
	    medmPrintf("rectangleCreateRunTimeInstance : memory allocation error\n");
	} else {
	    updateTaskAddDestroyCb(pr->updateTask,rectangleDestroyCb);
	    updateTaskAddNameCb(pr->updateTask,rectangleName);
	    pr->updateTask->opaque = False;
	}
	pr->record = medmAllocateRecord(dlRectangle->dynAttr.chan,
	  rectangleUpdateValueCb,NULL,(XtPointer) pr);

#ifdef __COLOR_RULE_H__
	switch (dlRectangle->dynAttr.clr) {
	    STATIC :
	      pr->record->monitorValueChanged = False;
	    pr->record->monitorSeverityChanged = False;
	    break;
	    ALARM :
	      pr->record->monitorValueChanged = False;
	    break;
	    DISCRETE :
	      pr->record->monitorSeverityChanged = False;
	    break;
	}
#else
	pr->record->monitorValueChanged = False;
	if (dlRectangle->dynAttr.clr != ALARM ) {
	    pr->record->monitorSeverityChanged = False;
	}
#endif

	if (dlRectangle->dynAttr.vis == V_STATIC ) {
	    pr->record->monitorZeroAndNoneZeroTransition = False;
	}
    } else {
	executeDlBasicAttribute(displayInfo,&(dlRectangle->attr));

	if (dlRectangle->attr.fill == F_SOLID) {
	    unsigned int lineWidth = (dlRectangle->attr.width+1)/2;
	    XFillRectangle(display,XtWindow(displayInfo->drawingArea),
	      displayInfo->gc,
	      dlRectangle->object.x,dlRectangle->object.y,
	      dlRectangle->object.width,dlRectangle->object.height);
	    XFillRectangle(display,displayInfo->drawingAreaPixmap,
	      displayInfo->gc,
	      dlRectangle->object.x,dlRectangle->object.y,
	      dlRectangle->object.width,dlRectangle->object.height);
	} else
	  if (dlRectangle->attr.fill == F_OUTLINE) {
	      unsigned int lineWidth = (dlRectangle->attr.width+1)/2;
	      XDrawRectangle(display,XtWindow(displayInfo->drawingArea),
		displayInfo->gc,
		dlRectangle->object.x + lineWidth,
		dlRectangle->object.y + lineWidth,
		dlRectangle->object.width - 2*lineWidth,
		dlRectangle->object.height - 2*lineWidth);
	      XDrawRectangle(display,displayInfo->drawingAreaPixmap,
		displayInfo->gc,
		dlRectangle->object.x + lineWidth,
		dlRectangle->object.y + lineWidth,
		dlRectangle->object.width - 2*lineWidth,
		dlRectangle->object.height - 2*lineWidth);
	  }
    }
}

static void rectangleUpdateValueCb(XtPointer cd) {
    Rectangle *pr = (Rectangle *) ((Record *) cd)->clientData;
    updateTaskMarkUpdate(pr->updateTask);
}

static void rectangleDraw(XtPointer cd) {
    Rectangle *pr = (Rectangle *) cd;
    Record *pd = pr->record;
    DisplayInfo *displayInfo = pr->updateTask->displayInfo;
    XGCValues gcValues;
    unsigned long gcValueMask;
    Display *display = XtDisplay(pr->updateTask->displayInfo->drawingArea);
    DlRectangle *dlRectangle = pr->dlElement->structure.rectangle;

    if (pd->connected) {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	switch (dlRectangle->dynAttr.clr) {
#ifdef __COLOR_RULE_H__
	case STATIC :
	    gcValues.foreground = displayInfo->colormap[dlRectangle->attr.clr];
	    break;
	case DISCRETE:
	    gcValues.foreground = extractColor(displayInfo,
	      pd->value,
	      dlRectangle->dynAttr.colorRule,
	      dlRectangle->attr.clr);
	    break;
#else
	case STATIC :
	case DISCRETE:
	    gcValues.foreground = displayInfo->colormap[dlRectangle->attr.clr];
	    break;
	case ALARM :
	    gcValues.foreground = alarmColorPixel[pd->severity];
	    break;
#endif
	}
	gcValues.line_width = dlRectangle->attr.width;
	gcValues.line_style = ( (dlRectangle->attr.style == SOLID) ? LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

	switch (dlRectangle->dynAttr.vis) {
	case V_STATIC:
	    drawRectangle(pr);
	    break;
	case IF_NOT_ZERO:
	    if (pd->value != 0.0)
	      drawRectangle(pr);
	    break;
	case IF_ZERO:
	    if (pd->value == 0.0)
	      drawRectangle(pr);
	    break;
	default :
	    medmPrintf("internal error : rectangleUpdateValueCb\n");
	    break;
	}
	if (pd->readAccess) {
	    if (!pr->updateTask->overlapped && dlRectangle->dynAttr.vis == V_STATIC) {
		pr->updateTask->opaque = True;
	    }
	} else {
	    pr->updateTask->opaque = False;
	    draw3DQuestionMark(pr->updateTask);
	}
    } else {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	gcValues.foreground = WhitePixel(display,DefaultScreen(display));
	gcValues.line_width = dlRectangle->attr.width;
	gcValues.line_style = ((dlRectangle->attr.style == SOLID) ? LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
	drawRectangle(pr);
    }
}

static void rectangleDestroyCb(XtPointer cd) {
    Rectangle *pr = (Rectangle *) cd;
    if (pr) {
	medmDestroyRecord(pr->record);
	free((char *)pr);
    }
    return;
}

static void rectangleName(XtPointer cd, char **name, short *severity, int *count) {
    Rectangle *pr = (Rectangle *) cd;
    *count = 1;
    name[0] = pr->record->name;
    severity[0] = pr->record->severity;
}


DlElement *createDlRectangle(DlElement *p)
{
    DlRectangle *dlRectangle;
    DlElement *dlElement;

    dlRectangle = (DlRectangle *) malloc(sizeof(DlRectangle));
    if (!dlRectangle) return 0;
    if (p) {
	*dlRectangle = *p->structure.rectangle;
    } else {
	objectAttributeInit(&(dlRectangle->object)); 
	basicAttributeInit(&(dlRectangle->attr));
	dynamicAttributeInit(&(dlRectangle->dynAttr));
    }
 
    if (!(dlElement = createDlElement(DL_Rectangle,
      (XtPointer)      dlRectangle,
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

    if (!dlElement) return 0;
    dlRectangle = dlElement->structure.rectangle;
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlRectangle->object));
	    else
	      if (!strcmp(token,"basic attribute"))
		parseBasicAttribute(displayInfo,&(dlRectangle->attr));
	      else
		if (!strcmp(token,"dynamic attribute"))
		  parseDynamicAttribute(displayInfo,&(dlRectangle->dynAttr));
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
    if (MedmUseNewFileFormat) {
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


static void rectangleInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlRectangle *dlRectangle = p->structure.rectangle;
    medmGetValues(pRCB,
      CLR_RC,        &(dlRectangle->attr.clr),
      STYLE_RC,      &(dlRectangle->attr.style),
      FILL_RC,       &(dlRectangle->attr.fill),
      LINEWIDTH_RC,  &(dlRectangle->attr.width),
      CLRMOD_RC,     &(dlRectangle->dynAttr.clr),
      VIS_RC,        &(dlRectangle->dynAttr.vis),
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlRectangle->dynAttr.colorRule),
#endif
      CHAN_RC,       &(dlRectangle->dynAttr.chan),
      -1);
}


static void rectangleGetValues(ResourceBundle *pRCB, DlElement *p) {
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
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlRectangle->dynAttr.colorRule),
#endif
      CHAN_RC,       &(dlRectangle->dynAttr.chan),
      -1);
}
 
static void rectangleSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlRectangle *dlRectangle = p->structure.rectangle;
    medmGetValues(pRCB,
      CLR_RC,        &(dlRectangle->attr.clr),
      -1);
}
