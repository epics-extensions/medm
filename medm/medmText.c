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
a period of five years from Mtexth 30, 1993, the Government is
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
 * .03  09-12-95        vong    conform to c++ syntax
 * .04  09-25-95        vong    add the primitive color rule set
 *
 *****************************************************************************
*/

#include "medm.h"

typedef struct _Text {
  Widget           widget;
  DlText           *dlText;
  Record           *record;
  UpdateTask       *updateTask;
  DlDynamicAttrMod dynAttr;
  DlAttribute      attr;
} Text;

static void textUpdateValueCb(XtPointer cd);
static void textDraw(XtPointer cd);
static void textDestroyCb(XtPointer cd);
static void textName(XtPointer, char **, short *, int *);


static void drawText(Text *pt) {
  int i = 0, usedWidth, usedHeight;
  size_t nChars;
  DisplayInfo *displayInfo = pt->updateTask->displayInfo;
  Display *display = XtDisplay(pt->widget);
  DlText *dlText = pt->dlText;

  nChars = strlen(dlText->textix);
  i = dmGetBestFontWithInfo(fontTable,MAX_FONTS,dlText->textix,
      dlText->object.height,dlText->object.width,
      &usedHeight,&usedWidth,FALSE);
  usedWidth = XTextWidth(fontTable[i],dlText->textix,nChars);


  XSetFont(display,displayInfo->gc,fontTable[i]->fid);

  switch (dlText->align) {
  case HORIZ_LEFT:
  case VERT_TOP:
    XDrawString(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
                    dlText->object.x,dlText->object.y + fontTable[i]->ascent,
                    dlText->textix,nChars);
    break;
  case HORIZ_CENTER:
  case VERT_CENTER:
    XDrawString(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
                    dlText->object.x + (dlText->object.width - usedWidth)/2,
                    dlText->object.y + fontTable[i]->ascent,
                    dlText->textix,nChars);
    break;
  case HORIZ_RIGHT:
  case VERT_BOTTOM:
    XDrawString(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
                    dlText->object.x + dlText->object.width - usedWidth,
                    dlText->object.y + fontTable[i]->ascent,
                    dlText->textix,nChars);
    break;
  }
}

#ifdef __cplusplus
void executeDlText(DisplayInfo *displayInfo, DlText *dlText,
                                Boolean)
#else
void executeDlText(DisplayInfo *displayInfo, DlText *dlText,
                                Boolean forcedDisplayToWindow)
#endif
{
  if ((displayInfo->traversalMode == DL_EXECUTE) 
      && (displayInfo->useDynamicAttribute != FALSE)){

    Text *pt;
    pt = (Text *) malloc(sizeof(Text));
    pt->dlText = dlText;
    pt->updateTask = updateTaskAddTask(displayInfo,
                                       &(dlText->object),
                                       textDraw,
                                       (XtPointer)pt);

    if (pt->updateTask == NULL) {
      medmPrintf("textCreateRunTimeInstance : memory allocation error\n");
    } else {
      updateTaskAddDestroyCb(pt->updateTask,textDestroyCb);
      updateTaskAddNameCb(pt->updateTask,textName);
      pt->updateTask->opaque = False;
    }
    pt->record = medmAllocateRecord(
                  displayInfo->dynamicAttribute.attr.param.chan,
                  textUpdateValueCb,
                  NULL,
                  (XtPointer) pt);
    drawWhiteRectangle(pt->updateTask);

#ifdef __COLOR_RULE_H__
    switch (displayInfo->dynamicAttribute.attr.mod.clr) {
      STATIC :
        pt->record->monitorValueChanged = False;
        pt->record->monitorSeverityChanged = False;
        break;
      ALARM :
        pt->record->monitorValueChanged = False;
        break;
      DISCRETE :
        pt->record->monitorSeverityChanged = False;
        break;
    }
#else
    pt->record->monitorValueChanged = False;
    if (displayInfo->dynamicAttribute.attr.mod.clr != ALARM ) {
      pt->record->monitorSeverityChanged = False;
    }
#endif

    if (displayInfo->dynamicAttribute.attr.mod.vis == V_STATIC ) {
      pt->record->monitorZeroAndNoneZeroTransition = False;
    }

    pt->widget = displayInfo->drawingArea;
    pt->attr = displayInfo->attribute;
    pt->dynAttr = displayInfo->dynamicAttribute.attr.mod;

  } else {
    int i = 0, usedWidth, usedHeight;
    size_t nChars;
    nChars = strlen(dlText->textix);
    i = dmGetBestFontWithInfo(fontTable,MAX_FONTS,dlText->textix,
        dlText->object.height,dlText->object.width,
        &usedHeight,&usedWidth,FALSE);
    usedWidth = XTextWidth(fontTable[i],dlText->textix,nChars);


    XSetFont(display,displayInfo->gc,fontTable[i]->fid);

    switch (dlText->align) {
    case HORIZ_LEFT:
    case VERT_TOP:
      XDrawString(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
                    dlText->object.x,dlText->object.y + fontTable[i]->ascent,
                    dlText->textix,nChars);
      XDrawString(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
                    dlText->object.x,dlText->object.y + fontTable[i]->ascent,
                    dlText->textix,nChars);
      break;
    case HORIZ_CENTER:
    case VERT_CENTER:
      XDrawString(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
                    dlText->object.x + (dlText->object.width - usedWidth)/2,
                    dlText->object.y + fontTable[i]->ascent,
                    dlText->textix,nChars);
      XDrawString(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
                    dlText->object.x + (dlText->object.width - usedWidth)/2,
                    dlText->object.y + fontTable[i]->ascent,
                    dlText->textix,nChars);
      break;
    case HORIZ_RIGHT:
    case VERT_BOTTOM:
      XDrawString(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
                    dlText->object.x + dlText->object.width - usedWidth,
                    dlText->object.y + fontTable[i]->ascent,
                    dlText->textix,nChars);
      XDrawString(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
                    dlText->object.x + dlText->object.width - usedWidth,
                    dlText->object.y + fontTable[i]->ascent,
                    dlText->textix,nChars);
      break;
    }
  }
}

static void textUpdateValueCb(XtPointer cd) {
  Text *pt = (Text *) ((Record *) cd)->clientData;
  updateTaskMarkUpdate(pt->updateTask);
}

static void textDraw(XtPointer cd) {
  Text *pt = (Text *) cd;
  Record *pd = pt->record;
  DisplayInfo *displayInfo = pt->updateTask->displayInfo;
  XGCValues gcValues;
  unsigned long gcValueMask;
  Display *display = XtDisplay(pt->widget);

  if (pd->connected) {
    gcValueMask = GCForeground|GCBackground|GCLineWidth|GCLineStyle;
    switch (pt->dynAttr.clr) {
#ifdef __COLOR_RULE_H__
      case STATIC :
        gcValues.foreground = displayInfo->dlColormap[pt->attr.clr];
        break;
      case DISCRETE:
        gcValues.foreground = extractColor(displayInfo,
                                  pd->value,
                                  pt->dynAttr.colorRule,
                                  pt->attr.clr);
        break;
#else
      case STATIC :
      case DISCRETE:
        gcValues.foreground = displayInfo->dlColormap[pt->attr.clr];
        break;
#endif
      case ALARM :
        gcValues.foreground = alarmColorPixel[pd->severity];
        break;
      default :
	gcValues.foreground = displayInfo->dlColormap[pt->attr.clr];
	break;
    }
    gcValues.background = displayInfo->dlColormap[displayInfo->drawingAreaBackgroundColor];
    gcValues.line_width = pt->attr.width;
    gcValues.line_style = ((pt->attr.style == SOLID) ? LineSolid : LineOnOffDash);
    XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

    switch (pt->dynAttr.vis) {
      case V_STATIC:
        drawText(pt);
        break;
      case IF_NOT_ZERO:
        if (pd->value != 0.0)
          drawText(pt);
        break;
      case IF_ZERO:
        if (pd->value == 0.0)
          drawText(pt);
        break;
      default :
        medmPrintf("internal error : textUpdateValueCb\n");
        break;
    }
    if (pd->readAccess) {
      if (!pt->updateTask->overlapped && pt->dynAttr.vis == V_STATIC) {
        pt->updateTask->opaque = True;
      }
    } else {
      pt->updateTask->opaque = False;
      draw3DQuestionMark(pt->updateTask);
    }
  } else {
    drawWhiteRectangle(pt->updateTask);
  }
}

static void textDestroyCb(XtPointer cd) {
  Text *pt = (Text *) cd;
  if (pt) {
    medmDestroyRecord(pt->record);
    free((char *)pt);
  }
  return;
}

static void textName(XtPointer cd, char **name, short *severity, int *count) {
  Text *pt = (Text *) cd;
  *count = 1;
  name[0] = pt->record->name;
  severity[0] = pt->record->severity;
}

