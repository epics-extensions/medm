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
 *                              - using the new screen update mechanism
 * .03  09-11-95        vong    conform to c++ syntax
 * .04  09-25-95        vong    add the primitive color rule set
 *
 *****************************************************************************
*/

#include "medm.h"

typedef struct _Arc {
  Widget           widget;
  DlArc            *dlArc;
  Record           *record;
  UpdateTask       *updateTask;
  DlDynamicAttrMod dynAttr;
  DlAttribute      attr; 
} Arc;

static void arcDraw(XtPointer cd);
static void arcUpdateValueCb(XtPointer cd);
static void arcDestroyCb(XtPointer cd);
static void arcName(XtPointer, char **, short *, int *);


static void drawArc(Arc *pa) {
  unsigned int lineWidth;
  DisplayInfo *displayInfo = pa->updateTask->displayInfo;
  Display *display = XtDisplay(pa->widget);
  DlArc *dlArc = pa->dlArc;

  lineWidth = (pa->attr.width+1)/2;
  if (pa->attr.fill == F_SOLID) {
    XFillArc(display,XtWindow(pa->widget),displayInfo->gc,
        dlArc->object.x,dlArc->object.y,
        dlArc->object.width,dlArc->object.height,dlArc->begin,dlArc->path);
  } else
  if (pa->attr.fill == F_OUTLINE) {
    XDrawArc(display,XtWindow(pa->widget),displayInfo->gc,
        dlArc->object.x + lineWidth,
        dlArc->object.y + lineWidth,
        dlArc->object.width - 2*lineWidth,
        dlArc->object.height - 2*lineWidth,dlArc->begin,dlArc->path);
  }
}

#ifdef __cplusplus
void executeDlArc(DisplayInfo *displayInfo, DlArc *dlArc, Boolean)
#else
void executeDlArc(DisplayInfo *displayInfo, DlArc *dlArc,
                                Boolean forcedDisplayToWindow)
#endif
{
  if ((displayInfo->traversalMode == DL_EXECUTE) 
      && (displayInfo->useDynamicAttribute != FALSE)){
    Arc *pa;
    pa = (Arc *) malloc(sizeof(Arc));
    pa->dlArc = dlArc;
    pa->updateTask = updateTaskAddTask(displayInfo,
				       &(dlArc->object),
				       arcDraw,
				       (XtPointer)pa);

    if (pa->updateTask == NULL) {
      medmPrintf("arcCreateRunTimeInstance : memory allocation error\n");
    } else {
      updateTaskAddDestroyCb(pa->updateTask,arcDestroyCb);
      updateTaskAddNameCb(pa->updateTask,arcName);
      pa->updateTask->opaque = False;
    }
    pa->record = medmAllocateRecord(
                  displayInfo->dynamicAttribute.attr.param.chan,
                  arcUpdateValueCb,
                  NULL,
                  (XtPointer) pa);
#if 0
    drawWhiteRectangle(pa->updateTask);
#endif

#ifdef __COLOR_RULE_H__
    switch (displayInfo->dynamicAttribute.attr.mod.clr) {
      STATIC :
        pa->record->monitorValueChanged = False;
        pa->record->monitorSeverityChanged = False;
        break;
      ALARM :
        pa->record->monitorValueChanged = False;
        break;
      DISCRETE :
        pa->record->monitorSeverityChanged = False;
        break;
    }
#else
    pa->record->monitorValueChanged = False;
    if (displayInfo->dynamicAttribute.attr.mod.clr != ALARM ) {
      pa->record->monitorSeverityChanged = False;
    }
#endif

    if (displayInfo->dynamicAttribute.attr.mod.vis == V_STATIC ) {
      pa->record->monitorZeroAndNoneZeroTransition = False;
    }

    pa->widget = displayInfo->drawingArea;
    pa->attr = displayInfo->attribute;
    pa->dynAttr = displayInfo->dynamicAttribute.attr.mod;
  } else
  if (displayInfo->attribute.fill == F_SOLID) {
    unsigned int lineWidth = (displayInfo->attribute.width+1)/2;
    XFillArc(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
        dlArc->object.x,dlArc->object.y,
        dlArc->object.width,dlArc->object.height,dlArc->begin,dlArc->path);
    XFillArc(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
        dlArc->object.x,dlArc->object.y,
        dlArc->object.width,dlArc->object.height,dlArc->begin,dlArc->path);

  } else
  if (displayInfo->attribute.fill == F_OUTLINE) {
    unsigned int lineWidth = (displayInfo->attribute.width+1)/2;
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

static void arcUpdateValueCb(XtPointer cd) {
  Arc *pa = (Arc *) ((Record *) cd)->clientData;
  updateTaskMarkUpdate(pa->updateTask);
}

static void arcDraw(XtPointer cd) {
  Arc *pa = (Arc *) cd;
  Record *pd = pa->record;
  DisplayInfo *displayInfo = pa->updateTask->displayInfo;
  XGCValues gcValues;
  unsigned long gcValueMask;
  Display *display = XtDisplay(pa->widget);

  if (pd->connected) {
    gcValueMask = GCForeground|GCBackground|GCLineWidth|GCLineStyle;
    switch (pa->dynAttr.clr) {
#ifdef __COLOR_RULE_H__
      case STATIC :
        gcValues.foreground = displayInfo->dlColormap[pa->attr.clr];
        break;
      case DISCRETE:
        gcValues.foreground = extractColor(displayInfo,
                                  pd->value,
                                  pa->dynAttr.colorRule,
                                  pa->attr.clr);
        break;
#else
      case STATIC :
      case DISCRETE:
        gcValues.foreground = displayInfo->dlColormap[pa->attr.clr];
        break;
#endif
      case ALARM :
        gcValues.foreground = alarmColorPixel[pd->severity];
        break;
      default :
        gcValues.foreground = displayInfo->dlColormap[pa->attr.clr];
	medmPrintf("internal error : arcUpdatevalueCb : unknown attribute\n");
	break;
    }
    gcValues.background = displayInfo->dlColormap[displayInfo->drawingAreaBackgroundColor];
    gcValues.line_width = pa->attr.width;
    gcValues.line_style = ((pa->attr.style == SOLID) ? LineSolid : LineOnOffDash);
    XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

    switch (pa->dynAttr.vis) {
      case V_STATIC:
        drawArc(pa);
        break;
      case IF_NOT_ZERO:
        if (pd->value != 0.0)
          drawArc(pa);
        break;
      case IF_ZERO:
        if (pd->value == 0.0)
          drawArc(pa);
        break;
      default :
        medmPrintf("arcUpdateValueCb : internal error\n");
        break;
    }
    if (pd->readAccess) {
      if (!pa->updateTask->overlapped && pa->dynAttr.vis == V_STATIC) {
        pa->updateTask->opaque = True;
      }
    } else {
      pa->updateTask->opaque = False;
      draw3DQuestionMark(pa->updateTask);
    }
  } else {
    gcValueMask = GCForeground|GCBackground|GCLineWidth|GCLineStyle;
    gcValues.foreground = WhitePixel(display,DefaultScreen(display));
    gcValues.background = displayInfo->dlColormap[displayInfo->drawingAreaBackgroundColor];
    gcValues.line_width = pa->attr.width;
    gcValues.line_style = ( (pa->attr.style == SOLID) ? LineSolid : LineOnOffDash);
    XChangeGC(display, displayInfo->gc, gcValueMask,&gcValues);
    drawArc(pa);
  }
}

static void arcDestroyCb(XtPointer cd) {
  Arc *pa = (Arc *) cd;
  if (pa) {
    medmDestroyRecord(pa->record);
    free((char *)pa);
  }
  return;
}

static void arcName(XtPointer cd, char **name, short *severity, int *count) {
  Arc *pa = (Arc *) cd;
  *count = 1;
  name[0] = pa->record->name;
  severity[0] = pa->record->severity;
}

