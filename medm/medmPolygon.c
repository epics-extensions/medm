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
a period of five years from Mpolygonh 30, 1993, the Government is
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

typedef struct _Polygon {
  Widget           widget;
  DlPolygon       *dlPolygon;
  Record           *record;
  UpdateTask       *updateTask;
  DlDynamicAttrMod dynAttr;
  DlAttribute      attr;
} Polygon;

static void polygonDraw(XtPointer cd);
static void polygonUpdateValueCb(XtPointer cd);
static void polygonDestroyCb(XtPointer cd);
static void polygonName(XtPointer, char **, short *, int *);


static void drawPolygon(Polygon *pp) {
  unsigned int lineWidth;
  DisplayInfo *displayInfo = pp->updateTask->displayInfo;
  Display *display = XtDisplay(pp->widget);
  DlPolygon *dlPolygon = pp->dlPolygon;

  if (pp->attr.fill == F_SOLID) {
    XFillPolygon(display,XtWindow(pp->widget),displayInfo->gc,
        dlPolygon->points,dlPolygon->nPoints,Complex,CoordModeOrigin);
  } else
  if (pp->attr.fill == F_OUTLINE) {
    XDrawLines(display,XtWindow(pp->widget),displayInfo->gc,
        dlPolygon->points,dlPolygon->nPoints,CoordModeOrigin);
  }
}

void executeDlPolygon(DisplayInfo *displayInfo, DlPolygon *dlPolygon,
                                Boolean forcedDisplayToWindow)
{
  if ((displayInfo->traversalMode == DL_EXECUTE) 
      && (displayInfo->useDynamicAttribute != FALSE)){

    Polygon *pp;
    DlObject object;

    pp = (Polygon *) malloc(sizeof(Polygon));
    pp->dlPolygon = dlPolygon;
#if 1
    object = dlPolygon->object;
    object.width++;
    object.height++;
#endif
    pp->updateTask = updateTaskAddTask(displayInfo,
				       &object,
				       polygonDraw,
				       (XtPointer)pp);

    if (pp->updateTask == NULL) {
      medmPrintf("polygonCreateRunTimeInstance : memory allocation error\n");
    } else {
      updateTaskAddDestroyCb(pp->updateTask,polygonDestroyCb);
      updateTaskAddNameCb(pp->updateTask,polygonName);
      pp->updateTask->opaque = False;
    }
    pp->record = medmAllocateRecord(
                  displayInfo->dynamicAttribute.attr.param.chan,
                  polygonUpdateValueCb,
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

  } else
  if (displayInfo->attribute.fill == F_SOLID) {
    XFillPolygon(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
        dlPolygon->points,dlPolygon->nPoints,Complex,CoordModeOrigin);
    XFillPolygon(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
        dlPolygon->points,dlPolygon->nPoints,Complex,CoordModeOrigin);
  } else
  if (displayInfo->attribute.fill == F_OUTLINE) {
    XDrawLines(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
        dlPolygon->points,dlPolygon->nPoints,CoordModeOrigin);
    XDrawLines(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
        dlPolygon->points,dlPolygon->nPoints,CoordModeOrigin);
  }
}

static void polygonUpdateValueCb(XtPointer cd) {
  Polygon *pp = (Polygon *) ((Record *) cd)->clientData;
  updateTaskMarkUpdate(pp->updateTask);
}

static void polygonDraw(XtPointer cd) {
  Polygon *pp = (Polygon *) cd;
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
        drawPolygon(pp);
        break;
      case IF_NOT_ZERO:
        if (pd->value != 0.0)
          drawPolygon(pp);
        break;
      case IF_ZERO:
        if (pd->value == 0.0)
          drawPolygon(pp);
        break;
      default :
        medmPrintf("internal error : polygonUpdateValueCb : unknown visibility modifier\n");
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
    drawPolygon(pp);
  }
}

static void polygonDestroyCb(XtPointer cd) {
  Polygon *pp = (Polygon *) cd;
  if (pp) {
    medmDestroyRecord(pp->record);
    free(pp);
  }
  return;
}

static void polygonName(XtPointer cd, char **name, short *severity, int *count) {
  Polygon *pp = (Polygon *) cd;
  *count = 1;
  name[0] = pp->record->name;
  severity[0] = pp->record->severity;
}
