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
 * .03  09-11-95        vong    conform to c++ syntax
 *
 *****************************************************************************
*/

#include "medm.h"

typedef struct _Meter {
  Widget      widget;
  DlMeter     *dlMeter;
  Record      *record;
  UpdateTask  *updateTask;
} Meter;

static void meterUpdateValueCb(XtPointer cd);
static void meterDraw(XtPointer cd);
static void meterUpdateGraphicalInfoCb(XtPointer cd);
static void meterDestroyCb(XtPointer cd);
static void meterName(XtPointer, char **, short *, int *);

#ifdef __cplusplus
void executeDlMeter(DisplayInfo *displayInfo, DlMeter *dlMeter, Boolean)
#else
void executeDlMeter(DisplayInfo *displayInfo, DlMeter *dlMeter, Boolean dummy)
#endif
{
  Meter *pm;
  Arg args[24];
  int n;
  int usedHeight, usedCharWidth, bestSize, preferredHeight;
  Widget localWidget;

  if (displayInfo->traversalMode == DL_EXECUTE) {
    pm = (Meter *) malloc(sizeof(Meter));
    pm->dlMeter = dlMeter;
    pm->updateTask = updateTaskAddTask(displayInfo,
                                       &(dlMeter->object),
                                       meterDraw,
                                       (XtPointer)pm);

    if (pm->updateTask == NULL) {
      medmPrintf("meterCreateRunTimeInstance : memory allocation error\n");
    } else {
      updateTaskAddDestroyCb(pm->updateTask,meterDestroyCb);
      updateTaskAddNameCb(pm->updateTask,meterName);
    }
    pm->record = medmAllocateRecord(dlMeter->monitor.rdbk,
                  meterUpdateValueCb,
                  meterUpdateGraphicalInfoCb,
                  (XtPointer) pm);
    drawWhiteRectangle(pm->updateTask);
  }

/* from the meter structure, we've got Meter's specifics */
  n = 0;
  XtSetArg(args[n],XtNx,(Position)dlMeter->object.x); n++;
  XtSetArg(args[n],XtNy,(Position)dlMeter->object.y); n++;
  XtSetArg(args[n],XtNwidth,(Dimension)dlMeter->object.width); n++;
  XtSetArg(args[n],XtNheight,(Dimension)dlMeter->object.height); n++;
  XtSetArg(args[n],XcNdataType,XcFval); n++;
  XtSetArg(args[n],XcNscaleSegments,
		(dlMeter->object.width > METER_OKAY_SIZE ? 11 : 5) ); n++;
  switch (dlMeter->label) {
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
	XtSetArg(args[n],XcNlabel,dlMeter->monitor.rdbk); n++;
	break;
  }
  preferredHeight = dlMeter->object.height/METER_FONT_DIVISOR;
  bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
	preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
  XtSetArg(args[n],XtNfont,fontTable[bestSize]); n++;
  XtSetArg(args[n],XcNmeterForeground,(Pixel)
	displayInfo->dlColormap[dlMeter->monitor.clr]); n++;
  XtSetArg(args[n],XcNmeterBackground,(Pixel)
	displayInfo->dlColormap[dlMeter->monitor.bclr]); n++;
  XtSetArg(args[n],XtNbackground,(Pixel)
	displayInfo->dlColormap[dlMeter->monitor.bclr]); n++;
  XtSetArg(args[n],XcNcontrolBackground,(Pixel)
	displayInfo->dlColormap[dlMeter->monitor.bclr]); n++;
/*
 * add the pointer to the Channel structure as userData 
 *  to widget
 */
  XtSetArg(args[n],XcNuserData,(XtPointer)pm); n++;
  localWidget = XtCreateWidget("meter", 
		xcMeterWidgetClass, displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] = localWidget;

  if (displayInfo->traversalMode == DL_EXECUTE) {

/* record the widget that this structure belongs to */
    pm->widget = localWidget;

/* add in drag/drop translations */
    XtOverrideTranslations(localWidget,parsedTranslations);


  } else if (displayInfo->traversalMode == DL_EDIT) {

/* add button press handlers */
    XtAddEventHandler(localWidget,ButtonPressMask,False,
		handleButtonPress,(XtPointer)displayInfo);

    XtManageChild(localWidget);
  }

}

static void meterUpdateValueCb(XtPointer cd) {
  Meter *pm = (Meter *) ((Record *) cd)->clientData;
  updateTaskMarkUpdate(pm->updateTask);
}

static void meterDraw(XtPointer cd) {
  Meter *pm = (Meter *) cd;
  Record *pd = pm->record;
  XcVType val;
  if (pd->connected) {
    if (pd->readAccess) {
      if (pm->widget)
        XtManageChild(pm->widget);
      else
        return;
      val.fval = (float) pd->value;
      XcMeterUpdateValue(pm->widget,&val);
      switch (pm->dlMeter->clrmod) {
        case STATIC :
        case DISCRETE :
          break;
        case ALARM :
	  XcMeterUpdateMeterForeground(pm->widget,alarmColorPixel[pd->severity]);
          break;
      }
    } else {
      if (pm->widget) XtUnmanageChild(pm->widget);
      draw3DPane(pm->updateTask,
          pm->updateTask->displayInfo->dlColormap[pm->dlMeter->monitor.bclr]);
      draw3DQuestionMark(pm->updateTask);
    }
  } else {
    if (pm->widget) XtUnmanageChild(pm->widget);
    drawWhiteRectangle(pm->updateTask);
  }
}

static void meterUpdateGraphicalInfoCb(XtPointer cd) {
  Record *pd = (Record *) cd;
  Meter *pm = (Meter *) pd->clientData;
  XcVType hopr, lopr, val;
  int precision;

  switch (pd->dataType) {
  case DBF_STRING :
  case DBF_ENUM :
    medmPrintf("meterUpdateGraphicalInfoCb : %s %s %s\n",
	"illegal channel type for",pm->dlMeter->monitor.rdbk, ": cannot attach Meter");
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
    medmPrintf("meterUpdateGraphicalInfoCb: %s %s %s\n",
	"unknown channel type for",pm->dlMeter->monitor.rdbk, ": cannot attach Meter");
    medmPostTime();
    break;
  }
  if ((hopr.fval == 0.0) && (lopr.fval == 0.0)) {
    hopr.fval += 1.0;
  }
  if (pm->widget != NULL) {
    Pixel pixel;
    pixel = (pm->dlMeter->clrmod == ALARM) ?
	    alarmColorPixel[pd->severity] :
	    pm->updateTask->displayInfo->dlColormap[pm->dlMeter->monitor.clr];
    XtVaSetValues(pm->widget,
      XcNlowerBound,lopr.lval,
      XcNupperBound,hopr.lval,
      XcNmeterForeground,pixel,
      XcNdecimals, precision,
      NULL);
    XcMeterUpdateValue(pm->widget,&val);
  }
}

static void meterDestroyCb(XtPointer cd) {
  Meter *pm = (Meter *) cd;
  if (pm) {
    medmDestroyRecord(pm->record);
    free((char *)pm);
  }
  return;
}

static void meterName(XtPointer cd, char **name, short *severity, int *count) {
  Meter *pm = (Meter *) cd;
  *count = 1;
  name[0] = pm->record->name;
  severity[0] = pm->record->severity;
}

void writeDlMeter(
  FILE *stream,
  DlMeter *dlMeter,
  int level)
{
  int i;
  char indent[16];

  for (i = 0;  i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%smeter {",indent);
  writeDlObject(stream,&(dlMeter->object),level+1);
  writeDlMonitor(stream,&(dlMeter->monitor),level+1);
  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,
        stringValueTable[dlMeter->label]);
  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
        stringValueTable[dlMeter->clrmod]);
  fprintf(stream,"\n%s}",indent);
}

