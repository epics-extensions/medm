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
a period of five years from Mrectangleh 30, 1993, the Government is
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

typedef struct _Rectangle {
  Widget           widget;
  DlRectangle      *dlRectangle;
  Record           *record;
  UpdateTask       *updateTask;
  DlDynamicAttrMod dynAttr;
  DlAttribute      attr;
} Rectangle;

static void rectangleDraw(XtPointer cd);
static void rectangleUpdateValueCb(XtPointer cd);
static void rectangleDestroyCb(XtPointer cd);
static void rectangleName(XtPointer, char **, short *, int *);


static void drawRectangle(Rectangle *pr) {
  unsigned int lineWidth;
  DisplayInfo *displayInfo = pr->updateTask->displayInfo;
  Display *display = XtDisplay(pr->widget);
  DlRectangle *dlRectangle = pr->dlRectangle;

  lineWidth = (pr->attr.width+1)/2;
  if (pr->attr.fill == F_SOLID) {
    XFillRectangle(display,XtWindow(pr->widget),displayInfo->gc,
          dlRectangle->object.x,dlRectangle->object.y,
          dlRectangle->object.width,dlRectangle->object.height);
  } else
  if (pr->attr.fill == F_OUTLINE) {
    XDrawRectangle(display,XtWindow(pr->widget),displayInfo->gc,
          dlRectangle->object.x + lineWidth,
          dlRectangle->object.y + lineWidth,
          dlRectangle->object.width - 2*lineWidth,
          dlRectangle->object.height - 2*lineWidth);
  }
}

void executeDlRectangle(DisplayInfo *displayInfo, DlRectangle *dlRectangle,
                                Boolean forcedDisplayToWindow)
{
  if ((displayInfo->traversalMode == DL_EXECUTE) 
      && (displayInfo->useDynamicAttribute != FALSE)){
    Rectangle *pr;
    pr = (Rectangle *) malloc(sizeof(Rectangle));
    pr->dlRectangle = dlRectangle;
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
    pr->record = medmAllocateRecord(
                  displayInfo->dynamicAttribute.attr.param.chan,
                  rectangleUpdateValueCb,
                  NULL,
                  (XtPointer) pr);

    pr->record->monitorValueChanged = False;
    if (displayInfo->dynamicAttribute.attr.mod.clr != ALARM ) {
      pr->record->monitorSeverityChanged = False;
    }

    if (displayInfo->dynamicAttribute.attr.mod.vis == V_STATIC ) {
      pr->record->monitorZeroAndNoneZeroTransition = False;
    }

    pr->widget = displayInfo->drawingArea;
    pr->attr = displayInfo->attribute;
    pr->dynAttr = displayInfo->dynamicAttribute.attr.mod;
  } else
  if (displayInfo->attribute.fill == F_SOLID) {
    unsigned int lineWidth = (displayInfo->attribute.width+1)/2;
    XFillRectangle(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
          dlRectangle->object.x,dlRectangle->object.y,
          dlRectangle->object.width,dlRectangle->object.height);
    XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
          dlRectangle->object.x,dlRectangle->object.y,
          dlRectangle->object.width,dlRectangle->object.height);
  } else
  if (displayInfo->attribute.fill == F_OUTLINE) {
    unsigned int lineWidth = (displayInfo->attribute.width+1)/2;
    XDrawRectangle(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
          dlRectangle->object.x + lineWidth,
          dlRectangle->object.y + lineWidth,
          dlRectangle->object.width - 2*lineWidth,
          dlRectangle->object.height - 2*lineWidth);
    XDrawRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
          dlRectangle->object.x + lineWidth,
          dlRectangle->object.y + lineWidth,
          dlRectangle->object.width - 2*lineWidth,
          dlRectangle->object.height - 2*lineWidth);
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
  Display *display = XtDisplay(pr->widget);

  if (pd->connected) {
    gcValueMask = GCForeground|GCBackground|GCLineWidth|GCLineStyle;
    switch (pr->dynAttr.clr) {
      case STATIC :
      case DISCRETE:
        gcValues.foreground = displayInfo->dlColormap[pr->attr.clr];
        break;
      case ALARM :
        gcValues.foreground = alarmColorPixel[pd->severity];
        break;
    }
    gcValues.background = displayInfo->dlColormap[displayInfo->drawingAreaBackgroundColor];
    gcValues.line_width = pr->attr.width;
    gcValues.line_style = ( (pr->attr.style == SOLID) ? LineSolid : LineOnOffDash);
    XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

    switch (pr->dynAttr.vis) {
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
      if (!pr->updateTask->overlapped && pr->dynAttr.vis == V_STATIC) {
        pr->updateTask->opaque = True;
      }
    } else {
      pr->updateTask->opaque = False;
      draw3DQuestionMark(pr->updateTask);
    }
  } else {
    gcValueMask = GCForeground|GCBackground|GCLineWidth|GCLineStyle;
    gcValues.foreground = WhitePixel(display,DefaultScreen(display));
    gcValues.background = displayInfo->dlColormap[displayInfo->drawingAreaBackgroundColor];
    gcValues.line_width = pr->attr.width;
    gcValues.line_style = ((pr->attr.style == SOLID) ? LineSolid : LineOnOffDash);
    XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
    drawRectangle(pr);
  }
}

static void rectangleDestroyCb(XtPointer cd) {
  Rectangle *pr = (Rectangle *) cd;
  if (pr) {
    medmDestroyRecord(pr->record);
    free(pr);
  }
  return;
}

static void rectangleName(XtPointer cd, char **name, short *severity, int *count) {
  Rectangle *pr = (Rectangle *) cd;
  *count = 1;
  name[0] = pr->record->name;
  severity[0] = pr->record->severity;
}
