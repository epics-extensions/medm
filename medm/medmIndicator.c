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

typedef struct _Indicator {
    DlElement   *dlElement;
    Record      *record;
    UpdateTask  *updateTask;
} Indicator;

static void indicatorUpdateValueCb(XtPointer cd);
static void indicatorDraw(XtPointer cd);
static void indicatorUpdateGraphicalInfoCb(XtPointer cd);
static void indicatorDestroyCb(XtPointer cd);
static void indicatorGetRecord(XtPointer, Record **, int *);
static void indicatorInheritValues(ResourceBundle *pRCB, DlElement *p);
static void indicatorSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void indicatorSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void indicatorGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable indicatorDlDispatchTable = {
    createDlIndicator,
    NULL,
    executeDlIndicator,
    writeDlIndicator,
    NULL,
    indicatorGetValues,
    indicatorInheritValues,
    indicatorSetBackgroundColor,
    indicatorSetForegroundColor,
    genericMove,
    genericScale,
    NULL,
    NULL};

void executeDlIndicator(DisplayInfo *displayInfo, DlElement *dlElement)
{
    Arg args[30];
    int n;
    int usedHeight, usedCharWidth, bestSize, preferredHeight;
    Widget localWidget;
    Indicator *pi;
    DlIndicator *dlIndicator = dlElement->structure.indicator;

    if (!dlElement->widget) {
	if (displayInfo->traversalMode == DL_EXECUTE) {
	    pi = (Indicator *) malloc(sizeof(Indicator));
	    pi->dlElement = dlElement;
	    pi->updateTask = updateTaskAddTask(displayInfo,
	      &(dlIndicator->object),
	      indicatorDraw,
	      (XtPointer)pi);

	    if (pi->updateTask == NULL) {
		medmPrintf("\nindicatorCreateRunTimeInstance: Memory allocation error\n");
	    } else {
		updateTaskAddDestroyCb(pi->updateTask,indicatorDestroyCb);
		updateTaskAddNameCb(pi->updateTask,indicatorGetRecord);
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
	    medmPrintf("\nexecuteDlIndicator: DOWN direction not supported\n");
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
	      "\nexecuteDlIndicator: LEFT direction not supported\n");
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
	  displayInfo->colormap[dlIndicator->monitor.clr]); n++;
	XtSetArg(args[n],XcNindicatorBackground,(Pixel)
	  displayInfo->colormap[dlIndicator->monitor.bclr]); n++;
	XtSetArg(args[n],XtNbackground,(Pixel)
	  displayInfo->colormap[dlIndicator->monitor.bclr]); n++;
	XtSetArg(args[n],XcNcontrolBackground,(Pixel)
	  displayInfo->colormap[dlIndicator->monitor.bclr]); n++;
      /*
       * add the pointer to the Channel structure as userData 
       *  to widget
       */
	XtSetArg(args[n],XcNuserData,(XtPointer)pi); n++;
	localWidget = XtCreateWidget("indicator", 
	  xcIndicatorWidgetClass, displayInfo->drawingArea, args, n);
	dlElement->widget = localWidget;
	
	if (displayInfo->traversalMode == DL_EDIT) {
	    addCommonHandlers(localWidget, displayInfo);
	    XtManageChild(localWidget);
	}
    } else {
	DlObject *po = &(dlElement->structure.indicator->object);
	XtVaSetValues(dlElement->widget,
	  XmNx, (Position) po->x,
	  XmNy, (Position) po->y,
	  XmNwidth, (Dimension) po->width,
	  XmNheight, (Dimension) po->height,
	  NULL);
    }
}

static void indicatorDraw(XtPointer cd) {
    Indicator *pi = (Indicator *) cd;
    Record *pd = pi->record;
    Widget widget= pi->dlElement->widget;
    DlIndicator *dlIndicator = pi->dlElement->structure.indicator;
    XcVType val;

    if (pd->connected) {
	if (pd->readAccess) {
	    if (widget) {
		addCommonHandlers(widget, pi->updateTask->displayInfo);
		XtManageChild(widget);
	    } else {
		return;
	    }
	    val.fval = (float) pd->value;
	    XcIndUpdateValue(widget,&val);
	    switch (dlIndicator->clrmod) {
	    case STATIC :
	    case DISCRETE :
		break;
	    case ALARM :
		XcIndUpdateIndicatorForeground(widget,alarmColorPixel[pd->severity]);
		break;
	    }
	} else {
	    if (widget) XtUnmanageChild(widget);
	    draw3DPane(pi->updateTask,
	      pi->updateTask->displayInfo->colormap[dlIndicator->monitor.bclr]);
	    draw3DQuestionMark(pi->updateTask);
	}
    } else {
	if (widget) XtUnmanageChild(widget);
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
    Widget widget = pi->dlElement->widget;
    DlIndicator *dlIndicator = pi->dlElement->structure.indicator;

    switch (pd->dataType) {
    case DBF_STRING :
    case DBF_ENUM :
	medmPostMsg("indicatorUpdateGraphicalInfoCb:\n"
	  "  Illegal channel type for %s\n"
	  "  Cannot attach Indicator\n",
	  dlIndicator->monitor.rdbk);
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
	medmPostMsg("indicatorUpdateGraphicalInfoCb:\n"
	  "  Unknown channel type for %s\n"
	  "  Cannot attach Indicator\n",
	  dlIndicator->monitor.rdbk);
	break;
    }
    if ((hopr.fval == 0.0) && (lopr.fval == 0.0)) {
	hopr.fval += 1.0;
    }

    if (widget != NULL) {
	Pixel pixel;
	pixel = (dlIndicator->clrmod == ALARM) ?
	  alarmColorPixel[pd->severity] :
	  pi->updateTask->displayInfo->colormap[dlIndicator->monitor.clr];
	XtVaSetValues(widget,
	  XcNlowerBound,lopr.lval,
	  XcNupperBound,hopr.lval,
	  XcNindicatorForeground,pixel,
	  XcNdecimals, precision,
	  NULL);
	XcIndUpdateValue(widget,&val);
    }
}

static void indicatorDestroyCb(XtPointer cd) {
    Indicator *pi = (Indicator *) cd;
    if (pi) {
	medmDestroyRecord(pi->record);
	free((char *)pi);
    }
    return;
}

static void indicatorGetRecord(XtPointer cd, Record **record, int *count) {
    Indicator *pi = (Indicator *) cd;
    *count = 1;
    record[0] = pi->record;
}

DlElement *createDlIndicator(DlElement *p) 
{
    DlIndicator *dlIndicator;
    DlElement *dlElement;

    dlIndicator = (DlIndicator *) malloc(sizeof(DlIndicator));
    if (!dlIndicator) return 0;
    if (p) {
	*dlIndicator = *p->structure.indicator;
    } else {
	objectAttributeInit(&(dlIndicator->object));
	monitorAttributeInit(&(dlIndicator->monitor));
	dlIndicator->label = LABEL_NONE;
	dlIndicator->clrmod = STATIC;
	dlIndicator->direction = RIGHT;
    }

    if (!(dlElement = createDlElement(DL_Indicator,
      (XtPointer)      dlIndicator,
      &indicatorDlDispatchTable))) {
	free(dlIndicator);
    }

    return(dlElement);
}

DlElement *parseIndicator(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlIndicator *dlIndicator;
    DlElement *dlElement = createDlIndicator(NULL);

    if (!dlElement) return 0;
    dlIndicator = dlElement->structure.indicator;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlIndicator->object));
	    } else if (!strcmp(token,"monitor")) {
		parseMonitor(displayInfo,&(dlIndicator->monitor));
	    } else if (!strcmp(token,"label")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"none")) 
		  dlIndicator->label = LABEL_NONE;
		else if (!strcmp(token,"outline"))
		  dlIndicator->label = OUTLINE;
		else if (!strcmp(token,"limits"))
		  dlIndicator->label = LIMITS;
		else if (!strcmp(token,"channel"))
		  dlIndicator->label = CHANNEL;
	    } else if (!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"static")) 
		  dlIndicator->clrmod = STATIC;
		else if (!strcmp(token,"alarm"))
		  dlIndicator->clrmod = ALARM;
		else if (!strcmp(token,"discrete"))
		  dlIndicator->clrmod = DISCRETE;
	    } else if (!strcmp(token,"direction")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"up")) 
		  dlIndicator->direction = UP;
		else if (!strcmp(token,"down"))
		  dlIndicator->direction = DOWN;
		else if (!strcmp(token,"right"))
		  dlIndicator->direction = RIGHT;
		else if (!strcmp(token,"left"))
		  dlIndicator->direction = LEFT;
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
void writeDlIndicator( FILE *stream, DlElement *dlElement, int level) {
/****************************************************************************
 * Write DL Indicator                                                       *
 ****************************************************************************/
    int i;
    char indent[16];
    DlIndicator *dlIndicator = dlElement->structure.indicator;

    for (i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%sindicator {",indent);
    writeDlObject(stream,&(dlIndicator->object),level+1);
    writeDlMonitor(stream,&(dlIndicator->monitor),level+1);
    if (dlIndicator->label != LABEL_NONE)
      fprintf(stream,"\n%s\tlabel=\"%s\"",indent,
        stringValueTable[dlIndicator->label]);
    if (dlIndicator->clrmod != STATIC)
      fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
        stringValueTable[dlIndicator->clrmod]);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (dlIndicator->direction != RIGHT)
      fprintf(stream,"\n%s\tdirection=\"%s\"",indent,
        stringValueTable[dlIndicator->direction]);
#else
    if ((dlIndicator->direction != RIGHT) || (!MedmUseNewFileFormat))
      fprintf(stream,"\n%s\tdirection=\"%s\"",indent,
        stringValueTable[dlIndicator->direction]);
#endif
    fprintf(stream,"\n%s}",indent);
}

static void indicatorInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlIndicator *dlIndicator = p->structure.indicator;
    medmGetValues(pRCB,
      RDBK_RC,       &(dlIndicator->monitor.rdbk),
      CLR_RC,        &(dlIndicator->monitor.clr),
      BCLR_RC,       &(dlIndicator->monitor.bclr),
      LABEL_RC,      &(dlIndicator->label),
      DIRECTION_RC,  &(dlIndicator->direction),
      CLRMOD_RC,     &(dlIndicator->clrmod),
      -1);
}

static void indicatorGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlIndicator *dlIndicator = p->structure.indicator;
    medmGetValues(pRCB,
      X_RC,          &(dlIndicator->object.x),
      Y_RC,          &(dlIndicator->object.y),
      WIDTH_RC,      &(dlIndicator->object.width),
      HEIGHT_RC,     &(dlIndicator->object.height),
      RDBK_RC,       &(dlIndicator->monitor.rdbk),
      CLR_RC,        &(dlIndicator->monitor.clr),
      BCLR_RC,       &(dlIndicator->monitor.bclr),
      LABEL_RC,      &(dlIndicator->label),
      DIRECTION_RC,  &(dlIndicator->direction),
      CLRMOD_RC,     &(dlIndicator->clrmod),
      -1);
}

static void indicatorSetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlIndicator *dlIndicator = p->structure.indicator;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlIndicator->monitor.bclr),
      -1);
}

static void indicatorSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlIndicator *dlIndicator = p->structure.indicator;
    medmGetValues(pRCB,
      CLR_RC,        &(dlIndicator->monitor.clr),
      -1);
}
