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
 * .02  05-09-95        vong    2.1.0 release
 *                              - using new screen update dispatch mechanism
 *
 *****************************************************************************
*/

#include "medm.h"

typedef struct _Bar {
  Widget      widget;
  DlBar      *dlBar;
  Record      *record;
  UpdateTask  *updateTask;
} Bar;

static void barDraw(XtPointer cd);
static void barUpdateValueCb(XtPointer cd);
static void barUpdateGraphicalInfoCb(XtPointer cd);
static void barDestroyCb(XtPointer cd);
static void barName(XtPointer, char **, short *, int *);

void executeDlBar(DisplayInfo *displayInfo, DlBar *dlBar, Boolean dummy)
{
  Bar *pb;
  Arg args[30];
  int n;
  int usedHeight, usedCharWidth, bestSize, preferredHeight;
  Widget localWidget;


  displayInfo->useDynamicAttribute = FALSE;

  if (displayInfo->traversalMode == DL_EXECUTE) {
    pb = (Bar *) malloc(sizeof(Bar));
    pb->dlBar = dlBar;
    pb->updateTask = updateTaskAddTask(displayInfo,
				       &(dlBar->object),
				       barDraw,
				       (XtPointer)pb);

    if (pb->updateTask == NULL) {
      medmPrintf("barCreateRunTimeInstance : memory allocation error\n");
    } else {
      updateTaskAddDestroyCb(pb->updateTask,barDestroyCb);
      updateTaskAddNameCb(pb->updateTask,barName);
    }
    pb->record = medmAllocateRecord(dlBar->monitor.rdbk,
                  barUpdateValueCb,
                  barUpdateGraphicalInfoCb,
                  (XtPointer) pb);
    drawWhiteRectangle(pb->updateTask);
  }

/* from the bar structure, we've got Bar's specifics */
  n = 0;
  XtSetArg(args[n],XtNx,(Position)dlBar->object.x); n++;
  XtSetArg(args[n],XtNy,(Position)dlBar->object.y); n++;
  XtSetArg(args[n],XtNwidth,(Dimension)dlBar->object.width); n++;
  XtSetArg(args[n],XtNheight,(Dimension)dlBar->object.height); n++;
  XtSetArg(args[n],XcNdataType,XcFval); n++;
  switch (dlBar->label) {
     case LABEL_NONE:
	XtSetArg(args[n],XcNvalueVisible,FALSE); n++;
	XtSetArg(args[n],XcNlabel," "); n++;
	break;
     case OUTLINE:
	XtSetArg(args[n],XcNvalueVisible,FALSE); n++;
	XtSetArg(args[n],XcNlabel," "); n++;
	break;
     case LIMITS:
	XtSetArg(args[n],XcNvalueVisible,TRUE); n++;
	XtSetArg(args[n],XcNlabel," "); n++;
	break;
     case CHANNEL:
	XtSetArg(args[n],XcNvalueVisible,TRUE); n++;
	XtSetArg(args[n],XcNlabel,dlBar->monitor.rdbk); n++;
	break;
  }
  switch (dlBar->direction) {
/*
 * note that this is  "direction of increase" for Bar
 */
     case LEFT:
	medmPrintf("\nexecuteDlBar: LEFT direction BARS not supported");
     case RIGHT:
	XtSetArg(args[n],XcNorient,XcHoriz); n++;
	XtSetArg(args[n],XcNscaleSegments,
		(dlBar->object.width>INDICATOR_OKAY_SIZE ? 11 : 5)); n++;
	if (dlBar->label == LABEL_NONE) {
		XtSetArg(args[n],XcNscaleSegments, 0); n++;
	}
	break;

     case DOWN:
	medmPrintf("\nexecuteDlBar: DOWN direction BARS not supported");
     case UP:
	XtSetArg(args[n],XcNorient,XcVert); n++;
	XtSetArg(args[n],XcNscaleSegments,
		(dlBar->object.height>INDICATOR_OKAY_SIZE ? 11 : 5)); n++;
	if (dlBar->label == LABEL_NONE) {
		XtSetArg(args[n],XcNscaleSegments, 0); n++;
	}
	break;
  }

  if (dlBar->fillmod == FROM_CENTER) {
      XtSetArg(args[n], XcNfillmod, XcCenter); n++;
  } else {
    XtSetArg(args[n], XcNfillmod, XcEdge); n++;
  }


  preferredHeight = dlBar->object.height/INDICATOR_FONT_DIVISOR;
  bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
	preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
  XtSetArg(args[n],XtNfont,fontTable[bestSize]); n++;

  XtSetArg(args[n],XcNbarForeground,(Pixel)
	displayInfo->dlColormap[dlBar->monitor.clr]); n++;
  XtSetArg(args[n],XcNbarBackground,(Pixel)
	displayInfo->dlColormap[dlBar->monitor.bclr]); n++;
  XtSetArg(args[n],XtNbackground,(Pixel)
	displayInfo->dlColormap[dlBar->monitor.bclr]); n++;
  XtSetArg(args[n],XcNcontrolBackground,(Pixel)
	displayInfo->dlColormap[dlBar->monitor.bclr]); n++;
/*
 * add the pointer to the Channel structure as userData 
 *  to widget
 */
  XtSetArg(args[n],XcNuserData,(XtPointer)pb); n++;
  localWidget = XtCreateWidget("bar", 
		xcBarGraphWidgetClass, displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] = localWidget;

  if (displayInfo->traversalMode == DL_EXECUTE) {

/* record the widget that this structure belongs to */
    pb->widget = localWidget;

/* add in drag/drop translations */
    XtOverrideTranslations(localWidget,parsedTranslations);

  } else if (displayInfo->traversalMode == DL_EDIT) {

/* add button press handlers */
    XtAddEventHandler(localWidget,ButtonPressMask,False,
		handleButtonPress,(XtPointer)displayInfo);

    XtManageChild(localWidget);
  }
}

static void barUpdateValueCb(XtPointer cd) {
  Bar *pb = (Bar *) ((Record *) cd)->clientData;
  updateTaskMarkUpdate(pb->updateTask);
}

static void barDraw(XtPointer cd) {
  Bar *pb = (Bar *) cd;
  Record *pd = pb->record;
  XcVType val;

  if (pd->connected) {
    if (pd->readAccess) {
      if (pb->widget)
	XtManageChild(pb->widget);
      else
	return;
      val.fval = (float) pd->value;
      XcBGUpdateValue(pb->widget,&val);
      switch (pb->dlBar->clrmod) {
        case STATIC :
        case DISCRETE :
	  break;
        case ALARM :
	  XcBGUpdateBarForeground(pb->widget,alarmColorPixel[pd->severity]);
	  break;
      }
    } else {
      if (pb->widget) XtUnmanageChild(pb->widget);
      draw3DPane(pb->updateTask,
          pb->updateTask->displayInfo->dlColormap[pb->dlBar->monitor.bclr]);
      draw3DQuestionMark(pb->updateTask);
    }
  } else {
    if (pb->widget) XtUnmanageChild(pb->widget);
    drawWhiteRectangle(pb->updateTask);
  }
}

static void barUpdateGraphicalInfoCb(XtPointer cd) {
  Record *pd = (Record *) cd;
  Bar *pb = (Bar *) pd->clientData;
  XcVType hopr, lopr, val;
  Pixel pixel;
  int precision;

  if (pb->widget == NULL) return;
  switch (pd->dataType) {
  case DBF_STRING :
  case DBF_ENUM :
    medmPrintf("barUpdateGraphicalInfoCb : %s %s %s\n",
	"illegal channel type for",pb->dlBar->monitor.rdbk, ": cannot attach Bar");
    medmPostTime();
    return;
  case DBF_CHAR :
  case DBF_INT :
  case DBF_LONG :
  case DBF_FLOAT :
  case DBF_DOUBLE :
    hopr.fval = (float) pd->hopr;
    lopr.fval = (float) pd->lopr;
    val.fval = (float) pd->value;
    precision = pd->precision;
    break;
  default :
    medmPrintf("barUpdateGraphicalInfoCb: %s %s %s\n",
	"unknown channel type for",pb->dlBar->monitor.rdbk, ": cannot attach Bar");
    medmPostTime();
    break;
  }
  if ((hopr.fval == 0.0) && (lopr.fval == 0.0)) {
    hopr.fval += 1.0;
  }

  pixel = (pb->dlBar->clrmod == ALARM) ?
	    alarmColorPixel[pd->severity] :
	    pb->updateTask->displayInfo->dlColormap[pb->dlBar->monitor.clr];
  XtVaSetValues(pb->widget,
    XcNlowerBound,lopr.lval,
    XcNupperBound,hopr.lval,
    XcNbarForeground,pixel,
    XcNdecimals, precision,
    NULL);
  XcBGUpdateValue(pb->widget,&val);
}

static void barDestroyCb(XtPointer cd) {
  Bar *pb = (Bar *) cd;
  if (pb) {
    medmDestroyRecord(pb->record);
    free(pb);
  }
  return;
}

static void barName(XtPointer cd, char **name, short *severity, int *count) {
  Bar *pb = (Bar *) cd;
  *count = 1;
  name[0] = pb->record->name;
  severity[0] = pb->record->severity;
}
