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

typedef struct _Bar {
    DlElement   *dlElement;
    Record      *record;
    UpdateTask  *updateTask;
} Bar;

/* Function Prototypes */

/* KE: Note that the following functions are really defined in xc/BarGraph.c as:
 * e.g. void XcBGUpdateBarForeground(ByteWidget w, unsigned long pixel);
 * but this is how they are being used and what avoids warnings.
 */
void XcBGUpdateBarForeground(Widget w, unsigned long pixel);
void XcBGUpdateValue(Widget w, XcVType *value);

static void barDraw(XtPointer cd);
static void barUpdateValueCb(XtPointer cd);
static void barUpdateGraphicalInfoCb(XtPointer cd);
static void barDestroyCb(XtPointer cd);
static void barGetRecord(XtPointer, Record **, int *);
static void barInheritValues(ResourceBundle *pRCB, DlElement *p);
static void barSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void barSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void barGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable barDlDispatchTable = {
    createDlBar,
    NULL,
    executeDlBar,
    writeDlBar,
    NULL,
    barGetValues,
    barInheritValues,
    barSetBackgroundColor,
    barSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

void executeDlBar(DisplayInfo *displayInfo, DlElement *dlElement)
{
    Bar *pb;
    Arg args[30];
    int n;
    int usedHeight, usedCharWidth, bestSize, preferredHeight;
    Widget localWidget;
    DlBar *dlBar = dlElement->structure.bar;

    if (!dlElement->widget) {
	if (displayInfo->traversalMode == DL_EXECUTE) {
	    pb = (Bar *) malloc(sizeof(Bar));
	    pb->dlElement = dlElement;
	    pb->updateTask = updateTaskAddTask(displayInfo,
	      &(dlBar->object),
	      barDraw,
	      (XtPointer)pb);

	    if (pb->updateTask == NULL) {
		medmPrintf(1,"\nbarCreateRunTimeInstance: Memory allocation error\n");
	    } else {
		updateTaskAddDestroyCb(pb->updateTask,barDestroyCb);
		updateTaskAddNameCb(pb->updateTask,barGetRecord);
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
	case RIGHT:
	    XtSetArg(args[n],XcNorient,XcHoriz); n++;
	    XtSetArg(args[n],XcNscaleSegments,
	      (dlBar->object.width>INDICATOR_OKAY_SIZE ? 11 : 5)); n++;
	      if (dlBar->label == LABEL_NONE) {
		  XtSetArg(args[n],XcNscaleSegments, 0); n++;
	      }
	      break;
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
	  displayInfo->colormap[dlBar->monitor.clr]); n++;
	XtSetArg(args[n],XcNbarBackground,(Pixel)
	  displayInfo->colormap[dlBar->monitor.bclr]); n++;
	XtSetArg(args[n],XtNbackground,(Pixel)
	  displayInfo->colormap[dlBar->monitor.bclr]); n++;
	XtSetArg(args[n],XcNcontrolBackground,(Pixel)
	  displayInfo->colormap[dlBar->monitor.bclr]); n++;
      /*
       * add the pointer to the Channel structure as userData 
       *  to widget
       */
	XtSetArg(args[n],XcNuserData,(XtPointer)pb); n++;
	localWidget = XtCreateWidget("bar", 
	  xcBarGraphWidgetClass, displayInfo->drawingArea, args, n);
	dlElement->widget = localWidget;
	
	if (displayInfo->traversalMode == DL_EDIT) {
	    addCommonHandlers(localWidget, displayInfo);
	    XtManageChild(localWidget);
	}
    } else {
	DlObject *po = &(dlElement->structure.bar->object);
	XtVaSetValues(dlElement->widget,
	  XmNx, (Position) po->x,
	  XmNy, (Position) po->y,
	  XmNwidth, (Dimension) po->width,
	  XmNheight, (Dimension) po->height,
	  NULL);
	printf("x=%d, y=%d, w=%d, h=%d\n",po->x,po->y,po->width,po->height);
    }
}

static void barUpdateValueCb(XtPointer cd) {
    Bar *pb = (Bar *) ((Record *) cd)->clientData;
    updateTaskMarkUpdate(pb->updateTask);
}

static void barDraw(XtPointer cd) {
    Bar *pb = (Bar *) cd;
    Record *pd = pb->record;
    Widget widget = pb->dlElement->widget;
    DlBar *dlBar = pb->dlElement->structure.bar;
    XcVType val;

    if (pd->connected) {
	if (pd->readAccess) {
	    if (widget) {
		addCommonHandlers(widget, pb->updateTask->displayInfo);
		XtManageChild(widget);
	    } else {
		return;
	    }
	    val.fval = (float) pd->value;
	    XcBGUpdateValue(widget,&val);
	    switch (dlBar->clrmod) {
	    case STATIC :
	    case DISCRETE :
		break;
	    case ALARM :
		XcBGUpdateBarForeground(widget,alarmColorPixel[pd->severity]);
		break;
	    }
	} else {
	    if (widget) XtUnmanageChild(widget);
	    draw3DPane(pb->updateTask,
	      pb->updateTask->displayInfo->colormap[dlBar->monitor.bclr]);
	    draw3DQuestionMark(pb->updateTask);
	}
    } else {
	if (widget) XtUnmanageChild(widget);
	drawWhiteRectangle(pb->updateTask);
    }
}

static void barUpdateGraphicalInfoCb(XtPointer cd) {
    Record *pd = (Record *) cd;
    Bar *pb = (Bar *) pd->clientData;
    XcVType hopr, lopr, val;
    Pixel pixel;
    short precision;
    Widget widget = pb->dlElement->widget;
    DlBar *dlBar = pb->dlElement->structure.bar;

    if (widget == NULL) return;
    switch (pd->dataType) {
    case DBF_STRING :
    case DBF_ENUM :
	medmPostMsg(1,"barUpdateGraphicalInfoCb:\n"
	  "  Illegal channel type for %s\n"
	  "  Cannot attach bar\n",
	  dlBar->monitor.rdbk);
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
	medmPostMsg(1,"barUpdateGraphicalInfoCb:\n"
	  "  Unknown channel type for %s\n"
	  "  Cannot attach bar\n",
	  dlBar->monitor.rdbk);
	return;
    }
    if ((hopr.fval == 0.0) && (lopr.fval == 0.0)) {
	hopr.fval += 1.0;
    }

    pixel = (dlBar->clrmod == ALARM) ?
      alarmColorPixel[pd->severity] :
	pb->updateTask->displayInfo->colormap[dlBar->monitor.clr];
    XtVaSetValues(widget,
      XcNlowerBound,lopr.lval,
      XcNupperBound,hopr.lval,
      XcNbarForeground,pixel,
      XcNdecimals, (int)precision,
      NULL);
    XcBGUpdateValue(widget,&val);
}

static void barDestroyCb(XtPointer cd) {
    Bar *pb = (Bar *) cd;
    if (pb) {
	medmDestroyRecord(pb->record);
	free((char *)pb);
    }
    return;
}

static void barGetRecord(XtPointer cd, Record **record, int *count) {
    Bar *pb = (Bar *) cd;
    *count = 1;
    record[0] = pb->record;
}

DlElement *createDlBar(DlElement *p)
{
    DlBar *dlBar;
    DlElement *dlElement;

    dlBar = (DlBar *) malloc(sizeof(DlBar));
    if (!dlBar) return 0;
    if (p) {
	*dlBar = *p->structure.bar;
    } else {
	objectAttributeInit(&(dlBar->object));
	monitorAttributeInit(&(dlBar->monitor));
	dlBar->label = LABEL_NONE;
	dlBar->clrmod = STATIC;
	dlBar->direction = RIGHT;
	dlBar->fillmod = FROM_EDGE;
    }

    if (!(dlElement = createDlElement(DL_Bar,
      (XtPointer)      dlBar,
      &barDlDispatchTable))) {
	free(dlBar);
    }
    return(dlElement);
}

DlElement *parseBar(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlBar *dlBar;
    DlElement *dlElement = createDlBar(NULL);
 
    if (!dlElement) return 0;
    dlBar = dlElement->structure.bar;
 
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlBar->object));
	    } else if (!strcmp(token,"monitor")) {
		parseMonitor(displayInfo,&(dlBar->monitor));
	    } else if (!strcmp(token,"label")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"none"))
		  dlBar->label = LABEL_NONE;
		else if (!strcmp(token,"outline"))
		  dlBar->label = OUTLINE;
		else if (!strcmp(token,"limits"))
		  dlBar->label = LIMITS;
		else
		  if (!strcmp(token,"channel"))
		    dlBar->label = CHANNEL;
	    } else if (!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"static"))
		  dlBar->clrmod = STATIC;
		else if (!strcmp(token,"alarm"))
		  dlBar->clrmod = ALARM;
		else if (!strcmp(token,"discrete"))
		  dlBar->clrmod = DISCRETE;
	    } else if (!strcmp(token,"direction")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"up"))
		  dlBar->direction = UP;
		else if (!strcmp(token,"right"))
		  dlBar->direction = RIGHT;
	      /* Backward compatibility */
		else if (!strcmp(token,"down"))
		  dlBar->direction = UP;
		else if (!strcmp(token,"left"))
		  dlBar->direction = RIGHT;
	    } else if (!strcmp(token,"fillmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"from edge"))
		  dlBar->fillmod = FROM_EDGE;
		else if(!strcmp(token,"from center"))
		  dlBar->fillmod = FROM_CENTER;
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

void writeDlBar( FILE *stream, DlElement *dlElement, int level) {
    int i;
    char indent[16];
    DlBar *dlBar = dlElement->structure.bar;

    for (i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%sbar {",indent);
    writeDlObject(stream,&(dlBar->object),level+1);
    writeDlMonitor(stream,&(dlBar->monitor),level+1);
    if (dlBar->label != LABEL_NONE)
      fprintf(stream,"\n%s\tlabel=\"%s\"",indent,
        stringValueTable[dlBar->label]);
    if (dlBar->clrmod != STATIC)
      fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
        stringValueTable[dlBar->clrmod]);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (dlBar->direction != RIGHT)
      fprintf(stream,"\n%s\tdirection=\"%s\"",indent,
        stringValueTable[dlBar->direction]);
#else
    if ((dlBar->direction != RIGHT) || (!MedmUseNewFileFormat))
      fprintf(stream,"\n%s\tdirection=\"%s\"",indent,
        stringValueTable[dlBar->direction]);
#endif
    if (dlBar->fillmod != FROM_EDGE) 
      fprintf(stream,"\n%s\tfillmod=\"%s\"",indent,
        stringValueTable[dlBar->fillmod]);
    fprintf(stream,"\n%s}",indent);
}

static void barInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlBar *dlBar = p->structure.bar;
    medmGetValues(pRCB,
      RDBK_RC,       &(dlBar->monitor.rdbk),
      CLR_RC,        &(dlBar->monitor.clr),
      BCLR_RC,       &(dlBar->monitor.bclr),
      LABEL_RC,      &(dlBar->label),
      DIRECTION_RC,  &(dlBar->direction),
      CLRMOD_RC,     &(dlBar->clrmod),
      FILLMOD_RC,    &(dlBar->fillmod),
      -1);
}

static void barGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlBar *dlBar = p->structure.bar;
    medmGetValues(pRCB,
      X_RC,          &(dlBar->object.x),
      Y_RC,          &(dlBar->object.y),
      WIDTH_RC,      &(dlBar->object.width),
      HEIGHT_RC,     &(dlBar->object.height),
      RDBK_RC,       &(dlBar->monitor.rdbk),
      CLR_RC,        &(dlBar->monitor.clr),
      BCLR_RC,       &(dlBar->monitor.bclr),
      LABEL_RC,      &(dlBar->label),
      DIRECTION_RC,  &(dlBar->direction),
      CLRMOD_RC,     &(dlBar->clrmod),
      FILLMOD_RC,    &(dlBar->fillmod),
      -1);
}

static void barSetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlBar *dlBar = p->structure.bar;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlBar->monitor.bclr),
      -1);
}

static void barSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlBar *dlBar = p->structure.bar;
    medmGetValues(pRCB,
      CLR_RC,        &(dlBar->monitor.clr),
      -1);
}
