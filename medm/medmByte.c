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
 *     Original Author : David M. Wetherholt (CEBAF) & Mark Anderson
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

typedef struct _Byte {
  Widget      widget;
  DlByte      *dlByte;
  Record      *record;
  UpdateTask  *updateTask;
} Bits;

static void byteUpdateValueCb(XtPointer cd);
static void byteDraw(XtPointer cd);
static void byteDestroyCb(XtPointer cd);
static void byteName(XtPointer, char **, short *, int *);

void executeDlByte(DisplayInfo *displayInfo, DlByte *dlByte, Boolean dummy) {
/****************************************************************************
 * Execute DL Byte                                                          *
 ****************************************************************************/
  Arg args[30];
  int n;
  int usedHeight, usedCharWidth, bestSize, preferredHeight;
  Widget localWidget;
  Bits *pb;

  if (displayInfo->traversalMode == DL_EXECUTE) {
    pb = (Bits *) malloc(sizeof(Bits));
    pb->dlByte = dlByte;
    pb->updateTask = updateTaskAddTask(displayInfo,
                                       &(dlByte->object),
                                       byteDraw,
                                       (XtPointer)pb);

    if (pb->updateTask == NULL) {
      medmPrintf("byteCreateRunTimeInstance : memory allocation error\n");
    } else {
      updateTaskAddDestroyCb(pb->updateTask,byteDestroyCb);
      updateTaskAddNameCb(pb->updateTask,byteName);
    }
    pb->record = medmAllocateRecord(dlByte->monitor.rdbk,
                  byteUpdateValueCb,
                  NULL,
                  (XtPointer) pb);
    drawWhiteRectangle(pb->updateTask);
  }

/****** from the DlByte structure, we've got Byte's specifics */
    n = 0;
    XtSetArg(args[n],XtNx,(Position)dlByte->object.x); n++;
    XtSetArg(args[n],XtNy,(Position)dlByte->object.y); n++;
    XtSetArg(args[n],XtNwidth,(Dimension)dlByte->object.width); n++;
    XtSetArg(args[n],XtNheight,(Dimension)dlByte->object.height); n++;
    XtSetArg(args[n],XcNdataType,XcLval); n++;

/****** note that this is orientation for the Byte */
    if (dlByte->direction == RIGHT) {
      XtSetArg(args[n],XcNorient,XcHoriz); n++;
    }
    else {
      if (dlByte->direction == UP) XtSetArg(args[n],XcNorient,XcVert); n++;
    }
    XtSetArg(args[n],XcNsBit,dlByte->sbit); n++;
    XtSetArg(args[n],XcNeBit,dlByte->ebit); n++;

/****** Set arguments with other colors, etc */
    preferredHeight = dlByte->object.height/INDICATOR_FONT_DIVISOR;
    bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
      preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
    XtSetArg(args[n],XtNfont,fontTable[bestSize]); n++;
    XtSetArg(args[n],XcNbyteForeground,(Pixel)
      displayInfo->dlColormap[dlByte->monitor.clr]); n++;
    XtSetArg(args[n],XcNbyteBackground,(Pixel)
      displayInfo->dlColormap[dlByte->monitor.bclr]); n++;
    XtSetArg(args[n],XtNbackground,(Pixel)
      displayInfo->dlColormap[dlByte->monitor.bclr]); n++;
    XtSetArg(args[n],XcNcontrolBackground,(Pixel)
      displayInfo->dlColormap[dlByte->monitor.bclr]); n++;

/****** Add the pointer to the ChannelAccessMonitorData structure as
        userData to widget */
    XtSetArg(args[n],XcNuserData,(XtPointer)pb); n++;
    localWidget = XtCreateWidget("byte",
      xcByteWidgetClass, displayInfo->drawingArea, args, n);
    displayInfo->child[displayInfo->childCount++] = localWidget;

/****** Record the widget that this structure belongs to */
    if (displayInfo->traversalMode == DL_EXECUTE) {
      pb->widget = localWidget;
/****** Add in drag/drop translations */
      XtOverrideTranslations(localWidget,parsedTranslations);
    } else if (displayInfo->traversalMode == DL_EDIT) {
/* add button press handlers */
      XtAddEventHandler(localWidget,ButtonPressMask,False,
        handleButtonPress,(XtPointer)displayInfo);
      XtManageChild(localWidget);
    }
}

static void byteUpdateValueCb(XtPointer cd) {
  Bits *pb = (Bits *) ((Record *) cd)->clientData;
  updateTaskMarkUpdate(pb->updateTask);
}

static void byteDraw(XtPointer cd) {
  Bits *pb = (Bits *) cd;
  Record *pd = pb->record;
  XcVType val;
  if (pd->connected) {
    if (pd->readAccess) {
      if (pb->widget) {
        XtManageChild(pb->widget);
      } else {
	return;
      }
      val.fval = (float) pd->value;
      XcBYUpdateValue(pb->widget,&val);
      switch (pb->dlByte->clrmod) {
	case STATIC :
	case DISCRETE :
	  break;
	case ALARM :
          XcBYUpdateByteForeground(pb->widget,alarmColorPixel[pd->severity]);
	  break;
      }
    } else {
      if (pb->widget) XtUnmanageChild(pb->widget);
      draw3DPane(pb->updateTask,
          pb->updateTask->displayInfo->dlColormap[pb->dlByte->monitor.bclr]);
      draw3DQuestionMark(pb->updateTask);
    }
  } else {
    if (pb->widget) XtUnmanageChild(pb->widget);
    drawWhiteRectangle(pb->updateTask);
  }
}

static void byteDestroyCb(XtPointer cd) {
  Bits *pb = (Bits *) cd;
  if (pb) {
    medmDestroyRecord(pb->record);
    free(pb);
  }
}

static void byteName(XtPointer cd, char **name, short *severity, int *count) {
  Bits *pb = (Bits *) cd;
  *count = 1;
  name[0] = pb->record->name;
  severity[0] = pb->record->severity;
}
