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

typedef struct _Indicator {
  Widget      widget;
  DlIndicator *dlIndicator;
  Record      *record;
  UpdateTask  *updateTask;
} Indicator;

static void indicatorUpdateValueCb(XtPointer cd);
static void indicatorDraw(XtPointer cd);
static void indicatorUpdateGraphicalInfoCb(XtPointer cd);
static void indicatorDestroyCb(XtPointer cd);
static void indicatorName(XtPointer, char **, short *, int *);

void executeDlIndicator(DisplayInfo *displayInfo, DlIndicator *dlIndicator,
				Boolean dummy)
{
  
  Arg args[30];
  int n;
  int usedHeight, usedCharWidth, bestSize, preferredHeight;
  Widget localWidget;
  Indicator *pi;

  if (displayInfo->traversalMode == DL_EXECUTE) {
    pi = (Indicator *) malloc(sizeof(Indicator));
    pi->dlIndicator = dlIndicator;
    pi->updateTask = updateTaskAddTask(displayInfo,
                                       &(dlIndicator->object),
                                       indicatorDraw,
                                       (XtPointer)pi);

    if (pi->updateTask == NULL) {
      medmPrintf("indicatorCreateRunTimeInstance : memory allocation error\n");
    } else {
      updateTaskAddDestroyCb(pi->updateTask,indicatorDestroyCb);
      updateTaskAddNameCb(pi->updateTask,indicatorName);
    }
    pi->record = medmAllocateRecord(dlIndicator->monitor.rdbk,
                  indicatorUpdateValueCb,
                  indicatorUpdateGraphicalInfoCb,
                  (XtPointer) pi);
    drawWhiteRectangle(pi->updateTask);
  }

/* from the indicator structure, we've got Indicator's specifics */
  n = 0;
  XtSetArg(args[n],XtNx,(Position)dlIndicator->object.x); n++;
  XtSetArg(args[n],XtNy,(Position)dlIndicator->object.y); n++;
  XtSetArg(args[n],XtNwidth,(Dimension)dlIndicator->object.width); n++;
  XtSetArg(args[n],XtNheight,(Dimension)dlIndicator->object.height); n++;
  XtSetArg(args[n],XcNdataType,XcFval); n++;
  switch (dlIndicator->label) {
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
	XtSetArg(args[n],XcNlabel,dlIndicator->monitor.rdbk); n++;
	break;
  }

  switch (dlIndicator->direction) {
/*
 * note that this is  "direction of increase"
 */
     case DOWN:
	medmPrintf(
	    "\nexecuteDlIndicator: DOWN direction INDICATORS not supported");
     case UP:
	XtSetArg(args[n],XcNscaleSegments,
		(dlIndicator->object.width >INDICATOR_OKAY_SIZE ? 11 : 5)); n++;
	XtSetArg(args[n],XcNorient,XcVert); n++;
	if (dlIndicator->label == LABEL_NONE) {
		XtSetArg(args[n],XcNscaleSegments,0); n++;
	}
	break;

     case LEFT:
	medmPrintf(
	    "\nexecuteDlIndicator: LEFT direction INDICATORS not supported");
     case RIGHT:
	XtSetArg(args[n],XcNscaleSegments,
		(dlIndicator->object.height>INDICATOR_OKAY_SIZE ? 11 : 5)); n++;
	XtSetArg(args[n],XcNorient,XcHoriz); n++;
	if (dlIndicator->label == LABEL_NONE) {
		XtSetArg(args[n],XcNscaleSegments,0); n++;
	}
	break;
  }
  preferredHeight = dlIndicator->object.height/INDICATOR_FONT_DIVISOR;
  bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
	preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
  XtSetArg(args[n],XtNfont,fontTable[bestSize]); n++;

  XtSetArg(args[n],XcNindicatorForeground,(Pixel)
	displayInfo->dlColormap[dlIndicator->monitor.clr]); n++;
  XtSetArg(args[n],XcNindicatorBackground,(Pixel)
	displayInfo->dlColormap[dlIndicator->monitor.bclr]); n++;
  XtSetArg(args[n],XtNbackground,(Pixel)
	displayInfo->dlColormap[dlIndicator->monitor.bclr]); n++;
  XtSetArg(args[n],XcNcontrolBackground,(Pixel)
	displayInfo->dlColormap[dlIndicator->monitor.bclr]); n++;
/*
 * add the pointer to the Channel structure as userData 
 *  to widget
 */
  XtSetArg(args[n],XcNuserData,(XtPointer)pi); n++;
  localWidget = XtCreateWidget("indicator", 
		xcIndicatorWidgetClass, displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] = localWidget;

  if (displayInfo->traversalMode == DL_EXECUTE) {

/* record the widget that this structure belongs to */
    pi->widget = localWidget;

/* add in drag/drop translations */
    XtOverrideTranslations(localWidget,parsedTranslations);

  } else if (displayInfo->traversalMode == DL_EDIT) {

/* add button press handlers */
    XtAddEventHandler(localWidget,ButtonPressMask,False,
		handleButtonPress,(XtPointer)displayInfo);

    XtManageChild(localWidget);
  }

}

static void indicatorDraw(XtPointer cd) {
  Indicator *pi = (Indicator *) cd;
  Record *pd = pi->record;
  XcVType val;

  if (pd->connected) {
    if (pd->readAccess) {
      if (pi->widget)
	XtManageChild(pi->widget);
      else
	return;
      val.fval = (float) pd->value;
      XcIndUpdateValue(pi->widget,&val);
      switch (pi->dlIndicator->clrmod) {
        case STATIC :
        case DISCRETE :
	  break;
        case ALARM :
	  XcIndUpdateIndicatorForeground(pi->widget,alarmColorPixel[pd->severity]);
	  break;
      }
    } else {
      if (pi->widget) XtUnmanageChild(pi->widget);
      draw3DPane(pi->updateTask,
         pi->updateTask->displayInfo->dlColormap[pi->dlIndicator->monitor.bclr]);
      draw3DQuestionMark(pi->updateTask);
    }
  } else {
    if (pi->widget) XtUnmanageChild(pi->widget);
    drawWhiteRectangle(pi->updateTask);
  }
}

static void indicatorUpdateValueCb(XtPointer cd) {
  Indicator *pi = (Indicator *) ((Record *) cd)->clientData;
  updateTaskMarkUpdate(pi->updateTask);
}

static void indicatorUpdateGraphicalInfoCb(XtPointer cd) {
  Record *pd = (Record *) cd;
  Indicator *pi = (Indicator *) pd->clientData;
  XcVType hopr, lopr, val;
  int precision;

  switch (pd->dataType) {
  case DBF_STRING :
  case DBF_ENUM :
    medmPrintf("indicatorUpdateGraphicalInfoCb : %s %s %s\n",
	"illegal channel type for",pi->dlIndicator->monitor.rdbk, ": cannot attach Indicator");
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
    medmPrintf("indicatorUpdateGraphicalInfoCb: %s %s %s\n",
	"unknown channel type for",pi->dlIndicator->monitor.rdbk, ": cannot attach Indicator");
    medmPostTime();
    break;
  }
  if ((hopr.fval == 0.0) && (lopr.fval == 0.0)) {
    hopr.fval += 1.0;
  }

  if (pi->widget != NULL) {
    Pixel pixel;
    pixel = (pi->dlIndicator->clrmod == ALARM) ?
	    alarmColorPixel[pd->severity] :
	    pi->updateTask->displayInfo->dlColormap[pi->dlIndicator->monitor.clr];
    XtVaSetValues(pi->widget,
      XcNlowerBound,lopr.lval,
      XcNupperBound,hopr.lval,
      XcNindicatorForeground,pixel,
      XcNdecimals, precision,
      NULL);
    XcIndUpdateValue(pi->widget,&val);
  }
}

static void indicatorDestroyCb(XtPointer cd) {
  Indicator *pi = (Indicator *) cd;
  if (pi) {
    medmDestroyRecord(pi->record);
    free(pi);
  }
  return;
}

static void indicatorName(XtPointer cd, char **name, short *severity, int *count) {
  Indicator *pi = (Indicator *) cd;
  *count = 1;
  name[0] = pi->record->name;
  severity[0] = pi->record->severity;
}

