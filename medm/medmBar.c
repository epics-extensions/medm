/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*****************************************************************************
 *
 *     Original Author : Mark Anderson
 *     Second Author   : Frederick Vong
 *     Third Author    : Kenneth Evans, Jr.
 *
 *****************************************************************************
*/

#include "medm.h"

typedef struct _MedmBar {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           *record;
} MedmBar;

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
static void barGetLimits(DlElement *pE, DlLimits **ppL, char **pN);
static void barGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable barDlDispatchTable = {
    createDlBar,
    NULL,
    executeDlBar,
    hideDlBar,
    writeDlBar,
    barGetLimits,
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
    MedmBar *pb = NULL;
    Arg args[34];
    int n;
    int usedHeight, usedCharWidth, bestSize, preferredHeight;
    Widget localWidget;
    DlBar *dlBar = dlElement->structure.bar;

  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(!dlElement->widget) {
	if(displayInfo->traversalMode == DL_EXECUTE) {
	    if(dlElement->data) {
		pb = (MedmBar *)dlElement->data;
	    } else {
		pb = (MedmBar *)malloc(sizeof(MedmBar));
		dlElement->data = (void *)pb;
		if(pb == NULL) {
		    medmPrintf(1,"\nexecuteDlBar: Memory allocation error\n");
		    return;
		}
	      /* Pre-initialize */
		pb->updateTask = NULL;
		pb->record = NULL;
		pb->dlElement = dlElement;

		pb->updateTask = updateTaskAddTask(displayInfo,
		  &(dlBar->object),
		  barDraw,
		  (XtPointer)pb);

		if(pb->updateTask == NULL) {
		    medmPrintf(1,"\nexecuteDlBar: Memory allocation error\n");
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
	}

      /* Update the limits to reflect current src's */
	updatePvLimits(&dlBar->limits);

      /* From the bar structure, we've got Bar's specifics */
	n = 0;
	XtSetArg(args[n],XtNx,(Position)dlBar->object.x); n++;
	XtSetArg(args[n],XtNy,(Position)dlBar->object.y); n++;
	XtSetArg(args[n],XtNwidth,(Dimension)dlBar->object.width); n++;
	XtSetArg(args[n],XtNheight,(Dimension)dlBar->object.height); n++;
	XtSetArg(args[n],XcNdataType,XcFval); n++;
      /* KE: Need to set these 3 values explicitly and not use the defaults
       *  because the widget is an XcLval by default and the default
       *  initializations are into XcVType.lval, possibly giving meaningless
       *  numbers in XcVType.fval, which is what will be used for our XcFval
       *  widget.  They still need to be set from the lval, however, because
       *  they are XtArgVal's, which Xt typedef's as long (exc. Cray?)
       *  See Intrinsic.h */
	XtSetArg(args[n],XcNincrement,longFval(0.)); n++;     /* Not used */
	XtSetArg(args[n],XcNlowerBound,longFval(dlBar->limits.lopr)); n++;
	XtSetArg(args[n],XcNupperBound,longFval(dlBar->limits.hopr)); n++;
	XtSetArg(args[n],XcNdecimals,(int)dlBar->limits.prec); n++;
	switch (dlBar->label) {
	case LABEL_NONE:
	    XtSetArg(args[n],XcNvalueVisible,False); n++;
	    XtSetArg(args[n],XcNlabel," "); n++;
	    break;
	case NO_DECORATIONS:
	    XtSetArg(args[n],XcNvalueVisible,False); n++;
	    XtSetArg(args[n],XcNlabel," "); n++;
	    XtSetArg(args[n],XcNdecorations,False); n++;
	    break;
	case OUTLINE:
	    XtSetArg(args[n],XcNvalueVisible,False); n++;
	    XtSetArg(args[n],XcNlabel," "); n++;
	    break;
	case LIMITS:
	    XtSetArg(args[n],XcNvalueVisible,True); n++;
	    XtSetArg(args[n],XcNlabel," "); n++;
	    break;
	case CHANNEL:
	    XtSetArg(args[n],XcNvalueVisible,True); n++;
	    XtSetArg(args[n],XcNlabel,dlBar->monitor.rdbk); n++;
	    break;
	}
	switch(dlBar->direction) {
	  /* Note that this is the direction of increase */
	case RIGHT:
	    XtSetArg(args[n],XcNorient,XcHoriz); n++;
	    if(dlBar->label == LABEL_NONE || dlBar->label == NO_DECORATIONS) {
		XtSetArg(args[n],XcNscaleSegments, 0); n++;
	    } else {
		XtSetArg(args[n],XcNscaleSegments,
		  (dlBar->object.width>INDICATOR_OKAY_SIZE ? 11 : 5)); n++;
	    }
	    break;
	case UP:
	    XtSetArg(args[n],XcNorient,XcVert); n++;
	    if(dlBar->label == LABEL_NONE || dlBar->label == NO_DECORATIONS) {
		XtSetArg(args[n],XcNscaleSegments, 0); n++;
	    } else {
		XtSetArg(args[n],XcNscaleSegments,
		  (dlBar->object.height>INDICATOR_OKAY_SIZE ? 11 : 5)); n++;
	    }
	    break;
	case LEFT:
	    XtSetArg(args[n],XcNorient,XcHorizLeft); n++;
	    if(dlBar->label == LABEL_NONE || dlBar->label == NO_DECORATIONS) {
		XtSetArg(args[n],XcNscaleSegments, 0); n++;
	    } else {
		XtSetArg(args[n],XcNscaleSegments,
		  (dlBar->object.width>INDICATOR_OKAY_SIZE ? 11 : 5)); n++;
	    }
	    break;
	case DOWN:
	    XtSetArg(args[n],XcNorient,XcVertDown); n++;
	    if(dlBar->label == LABEL_NONE || dlBar->label == NO_DECORATIONS) {
		XtSetArg(args[n],XcNscaleSegments, 0); n++;
	    } else {
		XtSetArg(args[n],XcNscaleSegments,
		  (dlBar->object.height>INDICATOR_OKAY_SIZE ? 11 : 5)); n++;
	    }
	    break;
	}

	if(dlBar->fillmod == FROM_CENTER) {
	    XtSetArg(args[n], XcNfillmod, XcCenter); n++;
	} else {
	    XtSetArg(args[n], XcNfillmod, XcEdge); n++;
	}

#if 1
	preferredHeight = dlBar->object.height/INDICATOR_FONT_DIVISOR;
#else
      /* ACM: Suggested change */
      /* KE: Not a good idea */
        switch(dlBar->direction) {
	  case LEFT:
	  case RIGHT:
	    preferredHeight = dlBar->object.height/INDICATOR_FONT_DIVISOR;
	    break;
	  case DOWN:
	  case UP:
	    preferredHeight = dlBar->object.width/INDICATOR_FONT_DIVISOR;
	    break;
	}
#endif
	bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS, NULL,
	  preferredHeight, 0, &usedHeight, &usedCharWidth, False);
	XtSetArg(args[n],XtNfont,fontTable[bestSize]); n++;

	XtSetArg(args[n],XcNbarForeground,(Pixel)
	  displayInfo->colormap[dlBar->monitor.clr]); n++;
	XtSetArg(args[n],XcNbarBackground,(Pixel)
	  displayInfo->colormap[dlBar->monitor.bclr]); n++;
	XtSetArg(args[n],XtNbackground,(Pixel)
	  displayInfo->colormap[dlBar->monitor.bclr]); n++;
	XtSetArg(args[n],XcNcontrolBackground,(Pixel)
	  displayInfo->colormap[dlBar->monitor.bclr]); n++;

#ifdef BAR_DOUBLE_BUFFER
      /* Set double buffering */
	XtSetArg(args[n], XcNdoubleBuffer, True); n++;
#endif

      /* Add the pointer to the Channel structure as userData */
	XtSetArg(args[n],XcNuserData,(XtPointer)pb); n++;
	localWidget = XtCreateWidget("bar",
	  xcBarGraphWidgetClass, displayInfo->drawingArea, args, n);
	dlElement->widget = localWidget;

	if(displayInfo->traversalMode == DL_EDIT) {
	    addCommonHandlers(localWidget, displayInfo);
	    XtManageChild(localWidget);
	}
    } else {
	DlObject *po = &(dlElement->structure.bar->object);
	XtVaSetValues(dlElement->widget,
#if 1
	/* KE: This is probably not necessary, but not sure */
	  XmNx, (Position)po->x,
	  XmNy, (Position)po->y,
	  XmNwidth, (Dimension)po->width,
	  XmNheight, (Dimension)po->height,
#endif
	/* This is necessary for PV Limits */
	  XcNlowerBound, longFval(dlBar->limits.lopr),
	  XcNupperBound, longFval(dlBar->limits.hopr),
	  XcNdecimals, (int)dlBar->limits.prec,
	  NULL);
    }
}

void hideDlBar(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element with a widget */
    hideWidgetElement(displayInfo, dlElement);
}

static void barUpdateValueCb(XtPointer cd) {
    MedmBar *pb = (MedmBar *) ((Record *) cd)->clientData;
    updateTaskMarkUpdate(pb->updateTask);
}

static void barDraw(XtPointer cd) {
    MedmBar *pb = (MedmBar *) cd;
    Record *pr = pb->record;
    DlElement *dlElement = pb->dlElement;
    Widget widget = dlElement->widget;
    DlBar *dlBar = dlElement->structure.bar;
    XcVType val;

  /* Check if hidden */
    if(dlElement->hidden) {
	if(widget && XtIsManaged(widget)) {
	    XtUnmanageChild(widget);
	}
	return;
    }

    if(pr && pr->connected) {
	if(pr->readAccess) {
	    if(widget) {
		addCommonHandlers(widget, pb->updateTask->displayInfo);
		XtManageChild(widget);
	    } else {
		return;
	    }
	    val.fval = (float) pr->value;
	    XcBGUpdateValue(widget,&val);
	    switch (dlBar->clrmod) {
	    case STATIC :
	    case DISCRETE :
		break;
	    case ALARM :
		pr->monitorSeverityChanged = True;
		XcBGUpdateBarForeground(widget,alarmColor(pr->severity));
		break;
	    }
	} else {
	    if(widget && XtIsManaged(widget))
	      XtUnmanageChild(widget);
	    drawBlackRectangle(pb->updateTask);
	}
    } else {
	if(widget && XtIsManaged(widget))
	  XtUnmanageChild(widget);
	drawWhiteRectangle(pb->updateTask);
    }
}

static void barUpdateGraphicalInfoCb(XtPointer cd) {
    Record *pr = (Record *) cd;
    MedmBar *pb = (MedmBar *) pr->clientData;
    DlBar *dlBar = pb->dlElement->structure.bar;
    Pixel pixel;
    Widget widget = pb->dlElement->widget;
    XcVType hopr, lopr, val;
    short precision;
    Arg args[4];
    int nargs=0;

    switch (pr->dataType) {
    case DBF_STRING :
	medmPostMsg(1,"barUpdateGraphicalInfoCb:\n"
	  "  Illegal channel type for %s\n"
	  "  Cannot attach bar\n",
	  dlBar->monitor.rdbk);
	return;
    case DBF_ENUM :
    case DBF_CHAR :
    case DBF_INT :
    case DBF_LONG :
    case DBF_FLOAT :
    case DBF_DOUBLE :
	hopr.fval = (float) pr->hopr;
	lopr.fval = (float) pr->lopr;
	val.fval = (float) pr->value;
	precision = pr->precision;
	break;
    default :
	medmPostMsg(1,"barUpdateGraphicalInfoCb:\n"
	  "  Unknown channel type for %s\n"
	  "  Cannot attach bar\n",
	  dlBar->monitor.rdbk);
	return;
    }
    if((hopr.fval == 0.0) && (lopr.fval == 0.0)) {
	hopr.fval += 1.0;
    }
    if(widget != NULL) {
      /* Set foreground pixel according to alarm */
	pixel = (dlBar->clrmod == ALARM) ?
	  alarmColor(pr->severity) :
	  pb->updateTask->displayInfo->colormap[dlBar->monitor.clr];
	XtSetArg(args[nargs], XcNbarForeground, pixel); nargs++;

      /* Set Channel and User limits (if apparently not set yet) */
	dlBar->limits.loprChannel = lopr.fval;
	if(dlBar->limits.loprSrc != PV_LIMITS_USER &&
	  dlBar->limits.loprUser == LOPR_DEFAULT) {
	    dlBar->limits.loprUser = lopr.fval;
	}
	dlBar->limits.hoprChannel = hopr.fval;
	if(dlBar->limits.hoprSrc != PV_LIMITS_USER &&
	  dlBar->limits.hoprUser == HOPR_DEFAULT) {
	    dlBar->limits.hoprUser = hopr.fval;
	}
	dlBar->limits.precChannel = precision;
	if(dlBar->limits.precSrc != PV_LIMITS_USER &&
	  dlBar->limits.precUser == PREC_DEFAULT) {
	    dlBar->limits.precUser = precision;
	}

      /* Set values in the widget if src is Channel */
	if(dlBar->limits.loprSrc == PV_LIMITS_CHANNEL) {
	    dlBar->limits.lopr = lopr.fval;
	    XtSetArg(args[nargs], XcNlowerBound, lopr.lval); nargs++;
	}
	if(dlBar->limits.hoprSrc == PV_LIMITS_CHANNEL) {
	    dlBar->limits.hopr = hopr.fval;
	    XtSetArg(args[nargs], XcNupperBound, hopr.lval); nargs++;
	}
	if(dlBar->limits.precSrc == PV_LIMITS_CHANNEL) {
	    dlBar->limits.prec = precision;
	    XtSetArg(args[nargs], XcNdecimals, (int)precision); nargs++;
	}
	XtSetValues(widget, args, nargs);
	XcBGUpdateValue(widget, &val);
    }
}

static void barDestroyCb(XtPointer cd) {
    MedmBar *pb = (MedmBar *) cd;
    if(pb) {
	medmDestroyRecord(pb->record);
	if(pb->dlElement) pb->dlElement->data = NULL;
	free((char *)pb);
    }
    return;
}

static void barGetRecord(XtPointer cd, Record **record, int *count) {
    MedmBar *pb = (MedmBar *) cd;
    *count = 1;
    record[0] = pb->record;
}

DlElement *createDlBar(DlElement *p)
{
    DlBar *dlBar;
    DlElement *dlElement;

    dlBar = (DlBar *)malloc(sizeof(DlBar));
    if(!dlBar) return 0;
    if(p) {
	*dlBar = *p->structure.bar;
    } else {
	objectAttributeInit(&(dlBar->object));
	monitorAttributeInit(&(dlBar->monitor));
	limitsAttributeInit(&(dlBar->limits));
	dlBar->label = LABEL_NONE;
	dlBar->clrmod = STATIC;
	dlBar->direction = RIGHT;
	dlBar->fillmod = FROM_EDGE;
    }

    if(!(dlElement = createDlElement(DL_Bar,
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

    if(!dlElement) return 0;
    dlBar = dlElement->structure.bar;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlBar->object));
	    } else if(!strcmp(token,"monitor")) {
		parseMonitor(displayInfo,&(dlBar->monitor));
	    } else if(!strcmp(token,"label")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"none"))
		  dlBar->label = LABEL_NONE;
		else if(!strcmp(token,"no decorations"))
		  dlBar->label = NO_DECORATIONS;
		else if(!strcmp(token,"outline"))
		  dlBar->label = OUTLINE;
		else if(!strcmp(token,"limits"))
		  dlBar->label = LIMITS;
		else
		  if(!strcmp(token,"channel"))
		    dlBar->label = CHANNEL;
	    } else if(!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"static"))
		  dlBar->clrmod = STATIC;
		else if(!strcmp(token,"alarm"))
		  dlBar->clrmod = ALARM;
		else if(!strcmp(token,"discrete"))
		  dlBar->clrmod = DISCRETE;
	    } else if(!strcmp(token,"direction")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"up"))
		  dlBar->direction = UP;
		else if(!strcmp(token,"right"))
		  dlBar->direction = RIGHT;
		else if(!strcmp(token,"down"))
		  dlBar->direction = DOWN;
		else if(!strcmp(token,"left"))
		  dlBar->direction = LEFT;
	    } else if(!strcmp(token,"fillmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"from edge"))
		  dlBar->fillmod = FROM_EDGE;
		else if(!strcmp(token,"from center"))
		  dlBar->fillmod = FROM_CENTER;
	    } else if(!strcmp(token,"limits")) {
	      parseLimits(displayInfo,&(dlBar->limits));
	    }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++;
	    break;
	case T_RIGHT_BRACE:
	    nestingLevel--;
	    break;
	default:
	    break;
	}
    } while( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );

    return dlElement;
}

void writeDlBar( FILE *stream, DlElement *dlElement, int level) {
    int i;
    char indent[16];
    DlBar *dlBar = dlElement->structure.bar;

    for(i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%sbar {",indent);
    writeDlObject(stream,&(dlBar->object),level+1);
    writeDlMonitor(stream,&(dlBar->monitor),level+1);
    if(dlBar->label != LABEL_NONE)
      fprintf(stream,"\n%s\tlabel=\"%s\"",indent,
        stringValueTable[dlBar->label]);
    if(dlBar->clrmod != STATIC)
      fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
        stringValueTable[dlBar->clrmod]);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(dlBar->direction != RIGHT)
      fprintf(stream,"\n%s\tdirection=\"%s\"",indent,
        stringValueTable[dlBar->direction]);
#else
    if((dlBar->direction != RIGHT) || (!MedmUseNewFileFormat))
      fprintf(stream,"\n%s\tdirection=\"%s\"",indent,
        stringValueTable[dlBar->direction]);
#endif
    if(dlBar->fillmod != FROM_EDGE)
      fprintf(stream,"\n%s\tfillmod=\"%s\"",indent,
        stringValueTable[dlBar->fillmod]);
    writeDlLimits(stream,&(dlBar->limits),level+1);
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
      LIMITS_RC,     &(dlBar->limits),
      -1);
}

static void barGetLimits(DlElement *pE, DlLimits **ppL, char **pN)
{
    DlBar *dlBar = pE->structure.bar;

    *(ppL) = &(dlBar->limits);
    *(pN) = dlBar->monitor.rdbk;
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
      LIMITS_RC,     &(dlBar->limits),
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
