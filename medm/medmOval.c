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
 *                              - using new screen update dispatch mechanism
 *
 *****************************************************************************
*/

#include "medm.h"

typedef struct _Oval {
  Widget           widget;
  DlOval           *dlOval;
  Record           *record;
  UpdateTask       *updateTask;
  DlDynamicAttrMod dynAttr;
  DlAttribute      attr;
} Oval;

static void ovalDraw(XtPointer cd);
static void ovalUpdateValueCb(XtPointer cd);
static void ovalDestroyCb(XtPointer cd);
static void ovalName(XtPointer, char **, short *, int *);

static void drawOval(Oval *po) {
  unsigned int lineWidth;
  DisplayInfo *displayInfo = po->updateTask->displayInfo;
  Display *display = XtDisplay(po->widget);
  DlOval *dlOval = po->dlOval;

  lineWidth = (po->attr.width+1)/2;
  if (po->attr.fill == F_SOLID) {
    XFillArc(display,XtWindow(po->widget),displayInfo->gc,
        dlOval->object.x,dlOval->object.y,
        dlOval->object.width,dlOval->object.height,0,360*64);
  } else
  if (po->attr.fill == F_OUTLINE) {
    XDrawArc(display,XtWindow(po->widget),displayInfo->gc,
        dlOval->object.x + lineWidth,
        dlOval->object.y + lineWidth,
        dlOval->object.width - 2*lineWidth,
        dlOval->object.height - 2*lineWidth,0,360*64);
  }
}

void executeDlOval(DisplayInfo *displayInfo, DlOval *dlOval,
                                Boolean forcedDisplayToWindow)
{
  if ((displayInfo->traversalMode == DL_EXECUTE) 
      && (displayInfo->useDynamicAttribute != FALSE)){

    Oval *po;
    po = (Oval *) malloc(sizeof(Oval));
    po->dlOval = dlOval;
    po->updateTask = updateTaskAddTask(displayInfo,
				       &(dlOval->object),
				       ovalDraw,
				       (XtPointer)po);

    if (po->updateTask == NULL) {
      medmPrintf("ovalCreateRunTimeInstance : memory allocation error\n");
    } else {
      updateTaskAddDestroyCb(po->updateTask,ovalDestroyCb);
      updateTaskAddNameCb(po->updateTask,ovalName);
      po->updateTask->opaque = False;
    }
    po->record = medmAllocateRecord(
                  displayInfo->dynamicAttribute.attr.param.chan,
                  ovalUpdateValueCb,
                  NULL,
                  (XtPointer) po);
#if 0
    drawWhiteRectangle(po->updateTask);
#endif

    po->record->monitorValueChanged = False;
    if (displayInfo->dynamicAttribute.attr.mod.clr != ALARM ) {
      po->record->monitorSeverityChanged = False;
    }

    if (displayInfo->dynamicAttribute.attr.mod.vis == V_STATIC ) {
      po->record->monitorZeroAndNoneZeroTransition = False;
    }

    po->widget = displayInfo->drawingArea;
    po->attr = displayInfo->attribute;
    po->dynAttr = displayInfo->dynamicAttribute.attr.mod;
  } else
  if (displayInfo->attribute.fill == F_SOLID) {
    unsigned int lineWidth = (displayInfo->attribute.width+1)/2;
    XFillArc(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
        dlOval->object.x,dlOval->object.y,
        dlOval->object.width,dlOval->object.height,0,360*64);
    XFillArc(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
        dlOval->object.x,dlOval->object.y,
        dlOval->object.width,dlOval->object.height,0,360*64);

  } else
  if (displayInfo->attribute.fill == F_OUTLINE) {
    unsigned int lineWidth = (displayInfo->attribute.width+1)/2;
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


static void ovalUpdateValueCb(XtPointer cd) {
  Oval *po = (Oval *) ((Record *) cd)->clientData;
  updateTaskMarkUpdate(po->updateTask);
}

static void ovalDraw(XtPointer cd) {
  Oval *po = (Oval *) cd;
  Record *pd = po->record;
  DisplayInfo *displayInfo = po->updateTask->displayInfo;
  XGCValues gcValues;
  unsigned long gcValueMask;
  Display *display = XtDisplay(po->widget);

  if (pd->connected) {
    gcValueMask = GCForeground|GCBackground|GCLineWidth|GCLineStyle;
    switch (po->dynAttr.clr) {
      case STATIC :
      case DISCRETE:
        gcValues.foreground = displayInfo->dlColormap[po->attr.clr];
        break;
      case ALARM :
        gcValues.foreground = alarmColorPixel[pd->severity];
        break;
    }
    gcValues.background = displayInfo->dlColormap[displayInfo->drawingAreaBackgroundColor];
    gcValues.line_width = po->attr.width;
    gcValues.line_style = ((po->attr.style == SOLID) ? LineSolid : LineOnOffDash);
    XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

    switch (po->dynAttr.vis) {
      case V_STATIC:
        drawOval(po);
        break;
      case IF_NOT_ZERO:
        if (pd->value != 0.0)
          drawOval(po);
        break;
      case IF_ZERO:
        if (pd->value == 0.0)
          drawOval(po);
        break;
      default :
        medmPrintf("internal error : ovalUpdateValueCb\n");
        break;
    }
    if (pd->readAccess) {
      if (!po->updateTask->overlapped && po->dynAttr.vis == V_STATIC) {
        po->updateTask->opaque = True;
      }
    } else {
      po->updateTask->opaque = False;
      draw3DQuestionMark(po->updateTask);
    }
  } else {
    gcValueMask = GCForeground|GCBackground|GCLineWidth|GCLineStyle;
    gcValues.foreground = WhitePixel(display,DefaultScreen(display));
    gcValues.background = displayInfo->dlColormap[displayInfo->drawingAreaBackgroundColor];
    gcValues.line_width = po->attr.width;
    gcValues.line_style = ((po->attr.style == SOLID) ? LineSolid : LineOnOffDash);
    XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
    drawOval(po);
  }
}

static void ovalDestroyCb(XtPointer cd) {
  Oval *po = (Oval *) cd;
  if (po) {
    medmDestroyRecord(po->record);
    free(po);
  }
  return;
}

static void ovalName(XtPointer cd, char **name, short *severity, int *count) {
  Oval *po = (Oval *) cd;
  *count = 1;
  name[0] = po->record->name;
  severity[0] = po->record->severity;
}

