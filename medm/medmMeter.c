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

#define DEBUG_COMPOSITE 0
#define DEBUG_DELETE 0

#include "medm.h"

typedef struct _MedmMeter {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           *record;
} MedmMeter;

static void meterUpdateValueCb(XtPointer cd);
static void meterDraw(XtPointer cd);
static void meterUpdateGraphicalInfoCb(XtPointer cd);
static void meterDestroyCb(XtPointer cd);
static void meterGetRecord(XtPointer, Record **, int *);
static void meterInheritValues(ResourceBundle *pRCB, DlElement *p);
static void meterSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void meterSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void meterGetLimits(DlElement *pE, DlLimits **ppL, char **pN);
static void meterGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable meterDlDispatchTable = {
    createDlMeter,
    NULL,
    executeDlMeter,
    hideDlMeter,
    writeDlMeter,
    meterGetLimits,
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
    MedmMeter *pm = NULL;
    Arg args[27];
    int nargs;
    int usedHeight, usedCharWidth, bestSize, preferredHeight;
    Widget localWidget;
    DlMeter *dlMeter = dlElement->structure.meter;

#if DEBUG_COMPOSITE
    print("executeDlMeter: dlMeter=%x\n",dlMeter);
#endif
  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(!dlElement->widget) {
	if(displayInfo->traversalMode == DL_EXECUTE) {
	    if(dlElement->data) {
		pm = (MedmMeter *)dlElement->data;
	    } else {
		pm = (MedmMeter *)malloc(sizeof(MedmMeter));
		dlElement->data = (void *)pm;
		if(pm == NULL) {
		    medmPrintf(1,"\nexecuteDlMeter: Memory allocation error\n");
		    return;
		}
	      /* Pre-initialize */
		pm->updateTask = NULL;
		pm->record = NULL;
		pm->dlElement = dlElement;

		pm->updateTask = updateTaskAddTask(displayInfo,
		  &(dlMeter->object),
		  meterDraw,
		  (XtPointer)pm);

		if(pm->updateTask == NULL) {
		    medmPrintf(1,"\nexecuteDlMeter: Memory allocation error\n");
		} else {
		    updateTaskAddDestroyCb(pm->updateTask,meterDestroyCb);
		    updateTaskAddNameCb(pm->updateTask,meterGetRecord);
		}
		pm->record = medmAllocateRecord(dlMeter->monitor.rdbk,
		  meterUpdateValueCb,
		  meterUpdateGraphicalInfoCb,
		  (XtPointer)pm);
		drawWhiteRectangle(pm->updateTask);
	    }
#if DEBUG_COMPOSITE
	    print("  pm=%x\n",pm);
#endif
	}

      /* Update the limits to reflect current src's */
	updatePvLimits(&dlMeter->limits);

      /* Create the widget */
	nargs = 0;
	XtSetArg(args[nargs],XtNx,(Position)dlMeter->object.x); nargs++;
	XtSetArg(args[nargs],XtNy,(Position)dlMeter->object.y); nargs++;
	XtSetArg(args[nargs],XtNwidth,(Dimension)dlMeter->object.width); nargs++;
	XtSetArg(args[nargs],XtNheight,(Dimension)dlMeter->object.height); nargs++;
	XtSetArg(args[nargs],XcNdataType,XcFval); nargs++;
      /* KE: Need to set these 3 values explicitly and not use the defaults
       *  because the widget is an XcLval by default and the default
       *  initializations are into XcVType.lval, possibly giving meaningless
       *  numbers in XcVType.fval, which is what will be used for our XcFval
       *  widget.  They still need to be set from the lval, however, because
       *  they are XtArgVal's, which Xt typedef's as long (exc. Cray?)
       *  See Intrinsic.h */
	XtSetArg(args[nargs],XcNincrement,longFval(0.)); nargs++;     /* Not used */
	XtSetArg(args[nargs],XcNlowerBound,longFval(dlMeter->limits.lopr)); nargs++;
	XtSetArg(args[nargs],XcNupperBound,longFval(dlMeter->limits.hopr)); nargs++;
	XtSetArg(args[nargs],XcNdecimals,(int)dlMeter->limits.prec); nargs++;
	XtSetArg(args[nargs],XcNscaleSegments,
	  (dlMeter->object.width > METER_OKAY_SIZE ? 11 : 5) ); nargs++;
	switch (dlMeter->label) {
	case LABEL_NONE:
	case NO_DECORATIONS:
	    XtSetArg(args[nargs],XcNvalueVisible,FALSE); nargs++;
	    XtSetArg(args[nargs],XcNlabel," "); nargs++;
	    break;
	case OUTLINE:
	    XtSetArg(args[nargs],XcNvalueVisible,FALSE); nargs++;
	    XtSetArg(args[nargs],XcNlabel," "); nargs++;
	    break;
	case LIMITS:
	    XtSetArg(args[nargs],XcNvalueVisible,TRUE); nargs++;
	    XtSetArg(args[nargs],XcNlabel," "); nargs++;
	    break;
	case CHANNEL:
	    XtSetArg(args[nargs],XcNvalueVisible,TRUE); nargs++;
	    XtSetArg(args[nargs],XcNlabel,dlMeter->monitor.rdbk); nargs++;
	    break;
	}
	preferredHeight = dlMeter->object.height/METER_FONT_DIVISOR;
	bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
	  preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
	XtSetArg(args[nargs],XtNfont,fontTable[bestSize]); nargs++;
	XtSetArg(args[nargs],XcNmeterForeground,
	  (Pixel)displayInfo->colormap[dlMeter->monitor.clr]); nargs++;
	XtSetArg(args[nargs],XcNmeterBackground,(Pixel)
	  displayInfo->colormap[dlMeter->monitor.bclr]); nargs++;
	XtSetArg(args[nargs],XtNbackground,
	  (Pixel)displayInfo->colormap[dlMeter->monitor.bclr]); nargs++;
	XtSetArg(args[nargs],XcNcontrolBackground,(Pixel)
	  displayInfo->colormap[dlMeter->monitor.bclr]); nargs++;
	/*
	 * add the pointer to the Channel structure as userData
	 *  to widget
	 */
	XtSetArg(args[nargs],XcNuserData,(XtPointer)pm); nargs++;
	localWidget = XtCreateWidget("meter",
	  xcMeterWidgetClass, displayInfo->drawingArea, args, nargs);
	dlElement->widget = localWidget;
 	if(displayInfo->traversalMode == DL_EXECUTE) {
	    pm->dlElement->widget = localWidget;
	} else if(displayInfo->traversalMode == DL_EDIT) {
	    addCommonHandlers(localWidget, displayInfo);
	    XtManageChild(localWidget);
	}
    } else {
      /* There is a widget */
	DlObject *po = &(dlElement->structure.meter->object);
	XtVaSetValues(dlElement->widget,
#if 1
	/* KE: This is probably not necessary, but not sure */
	  XmNx, (Position)po->x,
	  XmNy, (Position)po->y,
	  XmNwidth, (Dimension)po->width,
	  XmNheight, (Dimension)po->height,
#endif
	/* This is necessary for PV Limits */
	  XcNlowerBound, longFval(dlMeter->limits.lopr),
	  XcNupperBound, longFval(dlMeter->limits.hopr),
	  XcNdecimals, (int)dlMeter->limits.prec,
	  NULL);
    }
}

void hideDlMeter(DisplayInfo *displayInfo, DlElement *dlElement)
{
#if DEBUG_COMPOSITE
    print("hideDlMeter: dlElement=%x dlMeter=%x \n",
      dlElement,dlElement->structure.composite);
#endif

  /* Use generic hide for an element with a widget */
    hideWidgetElement(displayInfo, dlElement);
}

static void meterUpdateValueCb(XtPointer cd) {
    MedmMeter *pm = (MedmMeter *) ((Record *)cd)->clientData;
    updateTaskMarkUpdate(pm->updateTask);
}

static void meterDraw(XtPointer cd) {
    MedmMeter *pm = (MedmMeter *) cd;
    Record *pr = pm->record;
    DlElement *dlElement = pm->dlElement;
    Widget widget = dlElement->widget;
    DlMeter *dlMeter = dlElement->structure.meter;
    XcVType val;

#if DEBUG_DELETE
    print("meterDraw: connected=%s readAccess=%s value=%g\n",
      pr->connected?"Yes":"No",pr->readAccess?"Yes":"No",pr->value);
#endif

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
		addCommonHandlers(widget, pm->updateTask->displayInfo);
		XtManageChild(widget);
	    } else {
		return;
	    }
	    val.fval = (float)pr->value;
	    XcMeterUpdateValue(widget,&val);
	    switch (dlMeter->clrmod) {
	    case STATIC :
	    case DISCRETE :
		break;
	    case ALARM :
		pr->monitorSeverityChanged = True;
		XcMeterUpdateMeterForeground(widget,alarmColor(pr->severity));
		break;
	    }
	} else {
	    if(widget && XtIsManaged(widget))
	      XtUnmanageChild(widget);
	    drawBlackRectangle(pm->updateTask);
	}
    } else {
	if(widget && XtIsManaged(widget))
	  XtUnmanageChild(widget);
	drawWhiteRectangle(pm->updateTask);
    }
}

static void meterUpdateGraphicalInfoCb(XtPointer cd) {
    Record *pr = (Record *) cd;
    MedmMeter *pm = (MedmMeter *) pr->clientData;
    DlMeter *dlMeter = pm->dlElement->structure.meter;
    Pixel pixel;
    Widget widget = pm->dlElement->widget;
    XcVType hopr, lopr, val;
    short precision;
    Arg args[4];
    int nargs=0;

    switch (pr->dataType) {
    case DBF_STRING :
	medmPostMsg(1,"meterUpdateGraphicalInfoCb:\n"
	  "  Illegal channel type for %s\n"
	  "  Cannot attach meter\n",
	  dlMeter->monitor.rdbk);
	return;
    case DBF_ENUM :
    case DBF_CHAR :
    case DBF_INT :
    case DBF_LONG :
    case DBF_FLOAT :
    case DBF_DOUBLE :
	hopr.fval = (float)pr->hopr;
	lopr.fval = (float)pr->lopr;
	val.fval = (float)pr->value;
	precision = pr->precision;
	break;
    default :
	medmPostMsg(1,"meterUpdateGraphicalInfoCb:\n"
	  "  Unknown channel type for %s\n"
	  "  Cannot attach meter\n",
	  dlMeter->monitor.rdbk);
	break;
    }
    if((hopr.fval == 0.0) && (lopr.fval == 0.0)) {
	hopr.fval += 1.0;
    }
    if(widget != NULL) {
      /* Set foreground pixel according to alarm */
	pixel = (dlMeter->clrmod == ALARM) ?
	  alarmColor(pr->severity) :
	  pm->updateTask->displayInfo->colormap[dlMeter->monitor.clr];
	XtSetArg(args[nargs], XcNmeterForeground, pixel); nargs++;

      /* Set Channel and User limits (if apparently not set yet) */
	dlMeter->limits.loprChannel = lopr.fval;
	if(dlMeter->limits.loprSrc != PV_LIMITS_USER &&
	  dlMeter->limits.loprUser == LOPR_DEFAULT) {
	    dlMeter->limits.loprUser = lopr.fval;
	}
	dlMeter->limits.hoprChannel = hopr.fval;
	if(dlMeter->limits.hoprSrc != PV_LIMITS_USER &&
	  dlMeter->limits.hoprUser == HOPR_DEFAULT) {
	    dlMeter->limits.hoprUser = hopr.fval;
	}
	dlMeter->limits.precChannel = precision;
	if(dlMeter->limits.precSrc != PV_LIMITS_USER &&
	  dlMeter->limits.precUser == PREC_DEFAULT) {
	    dlMeter->limits.precUser = precision;
	}

      /* Set values in the widget if src is Channel */
	if(dlMeter->limits.loprSrc == PV_LIMITS_CHANNEL) {
	    dlMeter->limits.lopr = lopr.fval;
	    XtSetArg(args[nargs], XcNlowerBound, lopr.lval); nargs++;
	}
	if(dlMeter->limits.hoprSrc == PV_LIMITS_CHANNEL) {
	    dlMeter->limits.hopr = hopr.fval;
	    XtSetArg(args[nargs], XcNupperBound, hopr.lval); nargs++;
	}
	if(dlMeter->limits.precSrc == PV_LIMITS_CHANNEL) {
	    dlMeter->limits.prec = precision;
	    XtSetArg(args[nargs], XcNdecimals, (int)precision); nargs++;
	}
	XtSetValues(widget, args, nargs);
	XcMeterUpdateValue(widget, &val);
    }
}

static void meterDestroyCb(XtPointer cd) {
    MedmMeter *pm = (MedmMeter *)cd;
    if(pm) {
	medmDestroyRecord(pm->record);
	if(pm->dlElement) pm->dlElement->data = NULL;
	free((char *)pm);
    }
    return;
}

static void meterGetRecord(XtPointer cd, Record **record, int *count) {
    MedmMeter *pm = (MedmMeter *)cd;
    *count = 1;
    record[0] = pm->record;
}

DlElement *createDlMeter(DlElement *p)
{
    DlMeter *dlMeter;
    DlElement *dlElement;

    dlMeter = (DlMeter *)malloc(sizeof(DlMeter));
    if(!dlMeter) return 0;
    if(p) {
	*dlMeter = *p->structure.meter;
    } else {
	objectAttributeInit(&(dlMeter->object));
	monitorAttributeInit(&(dlMeter->monitor));
	limitsAttributeInit(&(dlMeter->limits));
	dlMeter->label = LABEL_NONE;
	dlMeter->clrmod = STATIC;
    }

    if(!(dlElement = createDlElement(DL_Meter, (XtPointer)dlMeter,
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

    if(!dlElement) return 0;
    dlMeter = dlElement->structure.meter;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlMeter->object));
	    else if(!strcmp(token,"monitor"))
	      parseMonitor(displayInfo,&(dlMeter->monitor));
	    else if(!strcmp(token,"label")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for(i=FIRST_LABEL_TYPE;i<FIRST_LABEL_TYPE+NUM_LABEL_TYPES;i++) {
		    if(!strcmp(token,stringValueTable[i])) {
			dlMeter->label = i;
			break;
		    }
		}
	    } else if(!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for(i=FIRST_COLOR_MODE;i<FIRST_COLOR_MODE+NUM_COLOR_MODES;i++) {
		    if(!strcmp(token,stringValueTable[i])) {
			dlMeter->clrmod = i;
			break;
		    }
		}
	    } else if(!strcmp(token,"limits")) {
	      parseLimits(displayInfo,&(dlMeter->limits));
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

void writeDlMeter(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
    int i;
    char indent[16];
    DlMeter *dlMeter = dlElement->structure.meter;

    for(i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%smeter {",indent);
    writeDlObject(stream,&(dlMeter->object),level+1);
    writeDlMonitor(stream,&(dlMeter->monitor),level+1);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
	if(dlMeter->label != LABEL_NONE)
	  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,stringValueTable[dlMeter->label]);
	if(dlMeter->clrmod != STATIC)
	  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,stringValueTable[dlMeter->clrmod]);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%s\tlabel=\"%s\"",indent,stringValueTable[dlMeter->label]);
	fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,stringValueTable[dlMeter->clrmod]);
    }
#endif
    writeDlLimits(stream,&(dlMeter->limits),level+1);
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
      LIMITS_RC,     &(dlMeter->limits),
      -1);
}

static void meterGetLimits(DlElement *pE, DlLimits **ppL, char **pN)
{
    DlMeter *dlMeter = pE->structure.meter;

    *(ppL) = &(dlMeter->limits);
    *(pN) = dlMeter->monitor.rdbk;
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
      LIMITS_RC,     &(dlMeter->limits),
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
