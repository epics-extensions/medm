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
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (630-252-2000).
*/
/*****************************************************************************
 *
 *     Original Author : Mark Anderson
 *     Second Author   : Frederick Vong
 *     Third Author    : Kenneth Evans, Jr.
 *
 *****************************************************************************
*/

#include "medm.h"

typedef struct _Meter {
    DlElement   *dlElement;
    Record      *record;
    UpdateTask  *updateTask;
} Meter;

static void meterUpdateValueCb(XtPointer cd);
static void meterDraw(XtPointer cd);
static void meterUpdateGraphicalInfoCb(XtPointer cd);
static void meterDestroyCb(XtPointer cd);
static void meterGetRecord(XtPointer, Record **, int *);
static void meterInheritValues(ResourceBundle *pRCB, DlElement *p);
static void meterSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void meterSetForegroundColor(ResourceBundle *pRCB, DlElement *p);

static void meterGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable meterDlDispatchTable = {
    createDlMeter,
    NULL,
    executeDlMeter,
    writeDlMeter,
    NULL,
    meterGetValues,
    meterInheritValues,
    meterSetBackgroundColor,
    meterSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

void executeDlMeter(DisplayInfo *displayInfo, DlElement *dlElement)
{
    Meter *pm;
    Arg args[24];
    int n;
    int usedHeight, usedCharWidth, bestSize, preferredHeight;
    Widget localWidget;
    DlMeter *dlMeter = dlElement->structure.meter;

    if (!dlElement->widget) {
	if (displayInfo->traversalMode == DL_EXECUTE) {
	    pm = (Meter *) malloc(sizeof(Meter));
	    pm->dlElement = dlElement;
	    pm->updateTask = updateTaskAddTask(displayInfo,
	      &(dlMeter->object),
	      meterDraw,
	      (XtPointer)pm);
	    
	    if (pm->updateTask == NULL) {
		medmPrintf(1,"\nmeterCreateRunTimeInstance: Memory allocation error\n");
	    } else {
		updateTaskAddDestroyCb(pm->updateTask,meterDestroyCb);
		updateTaskAddNameCb(pm->updateTask,meterGetRecord);
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
	XtSetArg(args[n],XcNmeterForeground,
	  (Pixel)displayInfo->colormap[dlMeter->monitor.clr]); n++;
	XtSetArg(args[n],XcNmeterBackground,(Pixel)
	  displayInfo->colormap[dlMeter->monitor.bclr]); n++;
	XtSetArg(args[n],XtNbackground,
	  (Pixel)displayInfo->colormap[dlMeter->monitor.bclr]); n++;
	XtSetArg(args[n],XcNcontrolBackground,(Pixel)
	  displayInfo->colormap[dlMeter->monitor.bclr]); n++;
	/*
	 * add the pointer to the Channel structure as userData 
	 *  to widget
	 */
	XtSetArg(args[n],XcNuserData,(XtPointer)pm); n++;
	localWidget = XtCreateWidget("meter", 
	  xcMeterWidgetClass, displayInfo->drawingArea, args, n);
	dlElement->widget = localWidget;
 	if (displayInfo->traversalMode == DL_EXECUTE) {
	    pm->dlElement->widget = localWidget;
	} else if (displayInfo->traversalMode == DL_EDIT) {
	    addCommonHandlers(localWidget, displayInfo);
	    XtManageChild(localWidget);
	}
    } else {
	DlObject *po = &(dlElement->structure.meter->object);
	XtVaSetValues(dlElement->widget,
	  XmNx, (Position)po->x,
	  XmNy, (Position)po->y,
	  XmNwidth, (Dimension)po->width,
	  XmNheight, (Dimension)po->height,
	  NULL);
    }
}

static void meterUpdateValueCb(XtPointer cd) {
    Meter *pm = (Meter *) ((Record *) cd)->clientData;
    updateTaskMarkUpdate(pm->updateTask);
}

static void meterDraw(XtPointer cd) {
    Meter *pm = (Meter *) cd;
    Record *pd = pm->record;
    Widget widget = pm->dlElement->widget;
    DlMeter *dlMeter = pm->dlElement->structure.meter;
    XcVType val;
    if (pd->connected) {
	if (pd->readAccess) {
	    if (widget) {
		addCommonHandlers(widget, pm->updateTask->displayInfo);
		XtManageChild(widget);
	    } else {
		return;
	    }
	    val.fval = (float) pd->value;
	    XcMeterUpdateValue(widget,&val);
	    switch (dlMeter->clrmod) {
	    case STATIC :
	    case DISCRETE :
		break;
	    case ALARM :
		XcMeterUpdateMeterForeground(widget,alarmColor(pd->severity));
		break;
	    }
	} else {
	    if (widget) XtUnmanageChild(widget);
	    draw3DPane(pm->updateTask,
	      pm->updateTask->displayInfo->colormap[dlMeter->monitor.bclr]);
	    draw3DQuestionMark(pm->updateTask);
	}
    } else {
	if (widget) XtUnmanageChild(widget);
	drawWhiteRectangle(pm->updateTask);
    }
}

static void meterUpdateGraphicalInfoCb(XtPointer cd) {
    Record *pd = (Record *) cd;
    Meter *pm = (Meter *) pd->clientData;
    DlMeter *dlMeter = pm->dlElement->structure.meter;
    Widget widget = pm->dlElement->widget;
    XcVType hopr, lopr, val;
    short precision;

    switch (pd->dataType) {
    case DBF_STRING :
    case DBF_ENUM :
	medmPostMsg(1,"meterUpdateGraphicalInfoCb:\n"
	  "  Illegal channel type for %s\n"
	  "  Cannot attach meter\n",
	  dlMeter->monitor.rdbk);
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
	medmPostMsg(1,"meterUpdateGraphicalInfoCb:\n"
	  "  Unknown channel type for %s\n"
	  "  Cannot attach meter\n",
	  dlMeter->monitor.rdbk);
	break;
    }
    if ((hopr.fval == 0.0) && (lopr.fval == 0.0)) {
	hopr.fval += 1.0;
    }
    if (widget != NULL) {
	Pixel pixel;
	pixel = (dlMeter->clrmod == ALARM) ?
	  alarmColor(pd->severity) :
	  pm->updateTask->displayInfo->colormap[dlMeter->monitor.clr];
	XtVaSetValues(widget,
	  XcNlowerBound,lopr.lval,
	  XcNupperBound,hopr.lval,
	  XcNmeterForeground,pixel,
	  XcNdecimals, (int)precision,
	  NULL);
	XcMeterUpdateValue(widget,&val);
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

static void meterGetRecord(XtPointer cd, Record **record, int *count) {
    Meter *pm = (Meter *) cd;
    *count = 1;
    record[0] = pm->record;
}

DlElement *createDlMeter(DlElement *p)
{
    DlMeter *dlMeter;
    DlElement *dlElement;

    dlMeter = (DlMeter *) malloc(sizeof(DlMeter));
    if (!dlMeter) return 0;
    if (p) {
	*dlMeter = *p->structure.meter;
    } else {
	objectAttributeInit(&(dlMeter->object));
	monitorAttributeInit(&(dlMeter->monitor));
	dlMeter->label = LABEL_NONE;
	dlMeter->clrmod = STATIC;
    }

    if (!(dlElement = createDlElement(DL_Meter, (XtPointer)dlMeter,
      &meterDlDispatchTable))) {
	free(dlMeter);
    }

    return(dlElement);
}

DlElement *parseMeter(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlMeter *dlMeter;
    DlElement *dlElement = createDlMeter(NULL);
    int i = 0;

    if (!dlElement) return 0;
    dlMeter = dlElement->structure.meter;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlMeter->object));
	    else if (!strcmp(token,"monitor"))
	      parseMonitor(displayInfo,&(dlMeter->monitor));
	    else if (!strcmp(token,"label")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for (i=FIRST_LABEL_TYPE;i<FIRST_LABEL_TYPE+NUM_LABEL_TYPES;i++) {
		    if (!strcmp(token,stringValueTable[i])) {
			dlMeter->label = i;
			break;
		    }
		}
	    } else if (!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for (i=FIRST_COLOR_MODE;i<FIRST_COLOR_MODE+NUM_COLOR_MODES;i++) {
		    if (!strcmp(token,stringValueTable[i])) {
			dlMeter->clrmod = i;
			break;
		    }
		}
	    }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
	}
    } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );

    return dlElement;
}

void writeDlMeter(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
    int i;
    char indent[16];
    DlMeter *dlMeter = dlElement->structure.meter;

    for (i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%smeter {",indent);
    writeDlObject(stream,&(dlMeter->object),level+1);
    writeDlMonitor(stream,&(dlMeter->monitor),level+1);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
	if (dlMeter->label != LABEL_NONE)
	  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,stringValueTable[dlMeter->label]);
	if (dlMeter->clrmod != STATIC) 
	  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,stringValueTable[dlMeter->clrmod]);
#ifdef SUPPORT_0201XX_FILE_FORMAT	
    } else {
	fprintf(stream,"\n%s\tlabel=\"%s\"",indent,stringValueTable[dlMeter->label]);
	fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,stringValueTable[dlMeter->clrmod]);
    }
#endif
    fprintf(stream,"\n%s}",indent);
}

static void meterInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlMeter *dlMeter = p->structure.meter;
    medmGetValues(pRCB,
      RDBK_RC,       &(dlMeter->monitor.rdbk),
      CLR_RC,        &(dlMeter->monitor.clr),
      BCLR_RC,       &(dlMeter->monitor.bclr),
      LABEL_RC,      &(dlMeter->label),
      CLRMOD_RC,     &(dlMeter->clrmod),
      -1);
}

static void meterGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlMeter *dlMeter = p->structure.meter;
    medmGetValues(pRCB,
      X_RC,          &(dlMeter->object.x),
      Y_RC,          &(dlMeter->object.y),
      WIDTH_RC,      &(dlMeter->object.width),
      HEIGHT_RC,     &(dlMeter->object.height),
      RDBK_RC,       &(dlMeter->monitor.rdbk),
      CLR_RC,        &(dlMeter->monitor.clr),
      BCLR_RC,       &(dlMeter->monitor.bclr),
      LABEL_RC,      &(dlMeter->label),
      CLRMOD_RC,     &(dlMeter->clrmod),
      -1);
}

static void meterSetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlMeter *dlMeter = p->structure.meter;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlMeter->monitor.bclr),
      -1);
}

static void meterSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlMeter *dlMeter = p->structure.meter;
    medmGetValues(pRCB,
      CLR_RC,        &(dlMeter->monitor.clr),
      -1);
}
