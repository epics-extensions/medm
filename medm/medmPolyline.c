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
a period of five years from Mpolylineh 30, 1993, the Government is
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

typedef struct _Polyline {
  Widget           widget;
  DlPolyline       *dlPolyline;
  Record           *record;
  UpdateTask       *updateTask;
  DlDynamicAttrMod dynAttr;
  DlAttribute      attr;
} Polyline;

static void polylineUpdateValueCb(XtPointer cd);
static void polylineDraw(XtPointer cd);
static void polylineDestroyCb(XtPointer cd);
static void polylineName(XtPointer, char **, short *, int *);


static void drawPolyline(Polyline *pp) {
  unsigned int lineWidth;
  DisplayInfo *displayInfo = pp->updateTask->displayInfo;
  Display *display = XtDisplay(pp->widget);
  DlPolyline *dlPolyline = pp->dlPolyline;

  XDrawLines(display,XtWindow(pp->widget),displayInfo->gc,
          dlPolyline->points,dlPolyline->nPoints,CoordModeOrigin);
}

void executeDlPolyline(DisplayInfo *displayInfo, DlPolyline *dlPolyline,
                                Boolean forcedDisplayToWindow)
{
  if (dlPolyline->isFallingOrRisingLine) {
    /* convert the falling line and rising line into polyline format */
    if (displayInfo->attribute.width > 0) {
      int width = displayInfo->attribute.width;
      int halfWidth = width/2;
      if (dlPolyline->points[1].y > dlPolyline->points[0].y) {
        /* falling line */
        dlPolyline->points[0].x += halfWidth;
        dlPolyline->points[0].y += halfWidth;
        dlPolyline->points[1].x -= halfWidth;
        dlPolyline->points[1].y -= halfWidth;
      } else
      if (dlPolyline->points[1].y < dlPolyline->points[0].y) {
        /* rising line */
        dlPolyline->points[0].x += halfWidth;
        dlPolyline->points[0].y -= width;
        dlPolyline->points[1].x -= width;
        dlPolyline->points[1].y -= halfWidth;
      }
    }
    dlPolyline->isFallingOrRisingLine = False;
  }
  if ((displayInfo->traversalMode == DL_EXECUTE) 
      && (displayInfo->useDynamicAttribute != FALSE)){

    Polyline *pp;
    DlObject object;
    pp = (Polyline *) malloc(sizeof(Polyline));
    pp->dlPolyline = dlPolyline;
#if 1
    object = dlPolyline->object;
    object.width++;
    object.height++;
#endif
    pp->updateTask = updateTaskAddTask(displayInfo,
                                       &object,
                                       polylineDraw,
                                       (XtPointer)pp);

    if (pp->updateTask == NULL) {
      medmPrintf("polylineCreateRunTimeInstance : memory allocation error\n");
    } else {
      updateTaskAddDestroyCb(pp->updateTask,polylineDestroyCb);
      updateTaskAddNameCb(pp->updateTask,polylineName);
      pp->updateTask->opaque = False;
    }
    pp->record = medmAllocateRecord(
                  displayInfo->dynamicAttribute.attr.param.chan,
                  polylineUpdateValueCb,
                  NULL,
                  (XtPointer) pp);
#if 0
    drawWhiteRectangle(pp->updateTask);
#endif

    pp->record->monitorValueChanged = False;
    if (displayInfo->dynamicAttribute.attr.mod.clr != ALARM ) {
      pp->record->monitorSeverityChanged = False;
    }

    if (displayInfo->dynamicAttribute.attr.mod.vis == V_STATIC ) {
      pp->record->monitorZeroAndNoneZeroTransition = False;
    }

    pp->widget = displayInfo->drawingArea;
    pp->attr = displayInfo->attribute;
    pp->dynAttr = displayInfo->dynamicAttribute.attr.mod;

  } else {
    XDrawLines(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
          dlPolyline->points,dlPolyline->nPoints,CoordModeOrigin);
    XDrawLines(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
          dlPolyline->points,dlPolyline->nPoints,CoordModeOrigin);
  }
}

static void polylineUpdateValueCb(XtPointer cd) {
  Polyline *pp = (Polyline *) ((Record *) cd)->clientData;
  updateTaskMarkUpdate(pp->updateTask);
}

static void polylineDraw(XtPointer cd) {
  Polyline *pp = (Polyline *) cd;
  Record *pd = pp->record;
  DisplayInfo *displayInfo = pp->updateTask->displayInfo;
  XGCValues gcValues;
  unsigned long gcValueMask;
  Display *display = XtDisplay(pp->widget);

  if (pd->connected) {
    gcValueMask = GCForeground|GCBackground|GCLineWidth|GCLineStyle;
    switch (pp->dynAttr.clr) {
      case STATIC :
      case DISCRETE:
        gcValues.foreground = displayInfo->dlColormap[pp->attr.clr];
        break;
      case ALARM :
        gcValues.foreground = alarmColorPixel[pd->severity];
        break;
      default :
	gcValues.foreground = displayInfo->dlColormap[pp->attr.clr];
	break;
    }
    gcValues.background = displayInfo->dlColormap[displayInfo->drawingAreaBackgroundColor];
    gcValues.line_width = pp->attr.width;
    gcValues.line_style = ((pp->attr.style == SOLID) ? LineSolid : LineOnOffDash);
    XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

    switch (pp->dynAttr.vis) {
      case V_STATIC:
        drawPolyline(pp);
        break;
      case IF_NOT_ZERO:
        if (pd->value != 0.0)
          drawPolyline(pp);
        break;
      case IF_ZERO:
        if (pd->value == 0.0)
          drawPolyline(pp);
        break;
      default :
        medmPrintf("internal error : polylineUpdateValueCb : unknown visibility modifier\n");
        break;
    }
    if (pd->readAccess) {
      if (!pp->updateTask->overlapped && pp->dynAttr.vis == V_STATIC) {
        pp->updateTask->opaque = True;
      }
    } else {
      pp->updateTask->opaque = False;
      draw3DQuestionMark(pp->updateTask);
    }
  } else {
    gcValueMask = GCForeground|GCBackground|GCLineWidth|GCLineStyle;
    gcValues.foreground = WhitePixel(display,DefaultScreen(display));
    gcValues.background = displayInfo->dlColormap[displayInfo->drawingAreaBackgroundColor];
    gcValues.line_width = pp->attr.width;
    gcValues.line_style = ((pp->attr.style == SOLID) ? LineSolid : LineOnOffDash);
    XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
    drawPolyline(pp);
  }
}

static void polylineDestroyCb(XtPointer cd) {
  Polyline *pp = (Polyline *) cd;
  if (pp) {
    medmDestroyRecord(pp->record);
    free(pp);
  }
  return;
}

static void polylineName(XtPointer cd, char **name, short *severity, int *count) {
  Polyline *pp = (Polyline *) cd;
  *count = 1;
  name[0] = pp->record->name;
  severity[0] = pp->record->severity;
}

