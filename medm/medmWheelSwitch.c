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
#define DEBUG_VALUE 0
#define DEBUG_CALCULATE_FORMAT 0

#define CALCULATE_FORMAT 0

#include "medm.h"

typedef struct _MedmWheelSwitch {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           *record;
} MedmWheelSwitch;

static void wheelSwitchUpdateValueCb(XtPointer cd);
static void wheelSwitchDraw(XtPointer cd);
static void wheelSwitchUpdateGraphicalInfoCb(XtPointer cd);
static void wheelSwitchDestroyCb(XtPointer cd);
static void wheelSwitchGetRecord(XtPointer, Record **, int *);
static void wheelSwitchInheritValues(ResourceBundle *pRCB, DlElement *p);
static void wheelSwitchSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void wheelSwitchSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void wheelSwitchGetLimits(DlElement *pE, DlLimits **ppL, char **pN);
static void wheelSwitchGetValues(ResourceBundle *pRCB, DlElement *p);
#if CALCULATE_FORMAT
static char *wheelSwitchCalculateFormat(DlWheelSwitch *dlWheelSwitch);
#endif
#if 0
static int wheelSwitchFontListIndex(int height);
#endif

static DlDispatchTable wheelSwitchDlDispatchTable = {
    createDlWheelSwitch,
    NULL,
    executeDlWheelSwitch,
    hideDlWheelSwitch,
    writeDlWheelSwitch,
    wheelSwitchGetLimits,
    wheelSwitchGetValues,
    wheelSwitchInheritValues,
    wheelSwitchSetBackgroundColor,
    wheelSwitchSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

#if 0
/* KE: Unused for now*/
static int wheelSwitchFontListIndex(int height)
{
    int i;
/* Don't allow height of font to exceed 90% - 4 pixels of wheelSwitch
 * widget (includes nominal 2*shadowThickness=2 shadow)
 */
    for(i = MAX_FONTS-1; i >=  0; i--) {
	if( ((int)(.90*height) - 4) >=
	  (fontTable[i]->ascent + fontTable[i]->descent))
	  return(i);
    }
    return (0);
}
#endif

void executeDlWheelSwitch(DisplayInfo *displayInfo, DlElement *dlElement)
{
    MedmWheelSwitch *pw;
    Arg args[27];
    int nargs;
#if 0
    int usedHeight, usedCharWidth, bestSize, preferredHeight;
    int fontIndex;
#endif
    Widget localWidget;
    DlWheelSwitch *dlWheelSwitch = dlElement->structure.wheelSwitch;
    char *format;

#if DEBUG_COMPOSITE
    print("executeDlWheelSwitch: dlWheelSwitch=%x\n",dlWheelSwitch);
#endif
  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(!dlElement->widget) {
	if(displayInfo->traversalMode == DL_EXECUTE) {
	    if(dlElement->data) {
		pw = (MedmWheelSwitch *)dlElement->data;
	    } else {
		pw = (MedmWheelSwitch *)malloc(sizeof(MedmWheelSwitch));
		dlElement->data = (void *)pw;
		if(pw == NULL) {
		    medmPrintf(1,"\nexecuteDlWheelSwitch: Memory allocation error\n");
		    return;
		}
	      /* Pre-initialize */
		pw->updateTask = NULL;
		pw->record = NULL;
		pw->dlElement = dlElement;

		pw->updateTask = updateTaskAddTask(displayInfo,
		  &(dlWheelSwitch->object),
		  wheelSwitchDraw,
		  (XtPointer)pw);
		
		if(pw->updateTask == NULL) {
		    medmPrintf(1,"\nexecuteDlWheelSwitch: Memory allocation error\n");
		} else {
		    updateTaskAddDestroyCb(pw->updateTask,wheelSwitchDestroyCb);
		    updateTaskAddNameCb(pw->updateTask,wheelSwitchGetRecord);
		}
		pw->record = medmAllocateRecord(dlWheelSwitch->control.ctrl,
		  wheelSwitchUpdateValueCb,
		  wheelSwitchUpdateGraphicalInfoCb,
		  (XtPointer)pw);
		drawWhiteRectangle(pw->updateTask);
	    }
#if DEBUG_COMPOSITE
	    print("  pw=%x\n",pw);
#endif
	}
	
#if 0
      /* Update the limits to reflect current src's */
	updatePvLimits(&dlWheelSwitch->limits);
#endif
	
      /* Create the widget */
	nargs = 0;
	
#if 0
	XtSetArg(args[nargs],XmNformat,"xxx% 19.2fyyy"); nargs++;
#else
#if CALCULATE_FORMAT	
	if(!*dlWheelSwitch->format) {
	  /* Calculate it from hopr, lopr, prec */
	    format = wheelSwitchCalculateFormat(dlWheelSwitch);
	} else {
	  /* Use the specified one */
	    format = dlWheelSwitch->format;
	}
#else
	format = dlWheelSwitch->format;
#endif
	XtSetArg(args[nargs],XmNformat,format); nargs++;
#endif	
	XtSetArg(args[nargs],XmNmarginWidth,0); nargs++;
	XtSetArg(args[nargs],XmNmarginHeight,0); nargs++;
	XtSetArg(args[nargs],XmNconformToContent,False); nargs++;
#if 0
      /* Determine the font */
	fontIndex=wheelSwitchFontListIndex(dlWheelSwitch->object.height);
	XtSetArg(args[nargs],XmNfontList,fontListTable[fontIndex]); nargs++;
#else	
	XtSetArg(args[nargs],XmNfontList,NULL); nargs++;
#endif	
	XtSetArg(args[nargs],XtNx,(Position)dlWheelSwitch->object.x); nargs++;
	XtSetArg(args[nargs],XtNy,(Position)dlWheelSwitch->object.y); nargs++;
	XtSetArg(args[nargs],XtNwidth,(Dimension)dlWheelSwitch->object.width);
	nargs++;
	XtSetArg(args[nargs],XtNheight,(Dimension)dlWheelSwitch->object.height);
	nargs++;
	XtSetArg(args[nargs],XmNforeground,
	  (Pixel)displayInfo->colormap[dlWheelSwitch->control.clr]); nargs++;
	XtSetArg(args[nargs],XmNbackground,(Pixel)
	  displayInfo->colormap[dlWheelSwitch->control.bclr]); nargs++;

	XtSetArg(args[nargs],XmNminValue,&dlWheelSwitch->limits.lopr); nargs++;
	XtSetArg(args[nargs],XmNmaxValue,&dlWheelSwitch->limits.hopr); nargs++;

      /* Add the pointer to the Channel structure as userData */
	XtSetArg(args[nargs],XmNuserData,(XtPointer)pw); nargs++;
	localWidget = XtCreateWidget("wheelSwitch", 
	  wheelSwitchWidgetClass, displayInfo->drawingArea, args, nargs);
	dlElement->widget = localWidget;
 	if(displayInfo->traversalMode == DL_EXECUTE) {
	    pw->dlElement->widget = localWidget;
	    XtAddCallback(localWidget, XmNvalueChangedCallback,
	      wheelSwitchValueChanged,(XtPointer)pw);
	} else if(displayInfo->traversalMode == DL_EDIT) {
	    addCommonHandlers(localWidget, displayInfo);
	    XtManageChild(localWidget);
	}
    } else {
      /* There is a widget */
	DlObject *po = &(dlElement->structure.wheelSwitch->object);

	nargs = 0;
#if 1
	/* KE: This is probably not necessary, but not sure */
	XtSetArg(args[nargs],XmNx,(Position)po->x); nargs++;
	XtSetArg(args[nargs],XmNy,(Position)po->y); nargs++;
	XtSetArg(args[nargs],XmNwidth,(Dimension)po->width); nargs++;
	XtSetArg(args[nargs],XmNheight,(Dimension)po->height); nargs++;
#endif
	/* This is necessary for PV Limits */
	XtSetArg(args[nargs],XmNminValue,&dlWheelSwitch->limits.lopr); nargs++;
	XtSetArg(args[nargs],XmNmaxValue,&dlWheelSwitch->limits.hopr); nargs++;

#if CALCULATE_FORMAT
      /* Recalculate the format if none is specified */
	if(!*dlWheelSwitch->format) {
	    char *format;
	  /* Calculate it from hopr, lopr, prec */
	    format = wheelSwitchCalculateFormat(dlWheelSwitch);
	    XtSetArg(args[nargs],XmNformat,format); nargs++;
	}
#endif	
    	XtSetValues(dlElement->widget,args,nargs);
    }
}

void hideDlWheelSwitch(DisplayInfo *displayInfo, DlElement *dlElement)
{
#if DEBUG_COMPOSITE
    print("hideDlWheelSwitch: dlElement=%x dlWheelSwitch=%x \n",
      dlElement,dlElement->structure.composite);
#endif

  /* Use generic hide for an element with a widget */
    hideWidgetElement(displayInfo, dlElement);
}

static void wheelSwitchUpdateValueCb(XtPointer cd) {
    MedmWheelSwitch *pw = (MedmWheelSwitch *)((Record *)cd)->clientData;
    updateTaskMarkUpdate(pw->updateTask);
}

static void wheelSwitchDraw(XtPointer cd) {
    MedmWheelSwitch *pw = (MedmWheelSwitch *) cd;
    Record *pr = pw->record;
    DlElement *dlElement = pw->dlElement;
    Widget widget = dlElement->widget;
    DlWheelSwitch *dlWheelSwitch = dlElement->structure.wheelSwitch;

#if DEBUG_DELETE || DEBUG_VALUE
    print("wheelSwitchDraw: connected=%s readAccess=%s value=%g\n",
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
		addCommonHandlers(widget, pw->updateTask->displayInfo);
		XtManageChild(widget);
	    } else {
		return;
	    }
#if DEBUG_VALUE
	    printf("  Calling XmWheelSwitchSetValue for value=%g\n",pr->value);
#endif
	    XmWheelSwitchSetValue(widget,&pr->value);
	    switch (dlWheelSwitch->clrmod) {
	    case STATIC :
	    case DISCRETE :
		break;
	    case ALARM :
		pr->monitorSeverityChanged = True;
#if 0
		XcWheelSwitchUpdateWheelSwitchForeground(widget,
		  alarmColor(pr->severity));
#endif
		break;
	    }
	} else {
	    if(widget && XtIsManaged(widget))
	      XtUnmanageChild(widget);
	    drawBlackRectangle(pw->updateTask);
	}
    } else {
	if(widget && XtIsManaged(widget))
	  XtUnmanageChild(widget);
	drawWhiteRectangle(pw->updateTask);
    }
}

static void wheelSwitchUpdateGraphicalInfoCb(XtPointer cd) {
    Record *pr = (Record *) cd;
    MedmWheelSwitch *pw = (MedmWheelSwitch *)pr->clientData;
    DlWheelSwitch *dlWheelSwitch = pw->dlElement->structure.wheelSwitch;
    Pixel pixel;
    Widget widget = pw->dlElement->widget;
    double hopr, lopr;
    short precision;
    Arg args[4];
    int nargs=0;

    switch (pr->dataType) {
    case DBF_STRING :
	medmPostMsg(1,"wheelSwitchUpdateGraphicalInfoCb:\n"
	  "  Illegal channel type for %s\n"
	  "  Cannot attach wheelSwitch\n",
	  dlWheelSwitch->control.ctrl);
	return;
    case DBF_ENUM :
    case DBF_CHAR :
    case DBF_INT :
    case DBF_LONG :
    case DBF_FLOAT :
    case DBF_DOUBLE :
	hopr = pr->hopr;
	lopr = pr->lopr;
	precision = pr->precision;
	break;
    default :
	medmPostMsg(1,"wheelSwitchUpdateGraphicalInfoCb:\n"
	  "  Unknown channel type for %s\n"
	  "  Cannot attach wheelSwitch\n",
	  dlWheelSwitch->control.ctrl);
	break;
    }
    if((hopr == 0.0) && (lopr == 0.0)) {
	hopr += 1.0;
    }
    if(widget != NULL) {
      /* Set foreground pixel according to alarm */
	pixel = (dlWheelSwitch->clrmod == ALARM) ?
	  alarmColor(pr->severity) :
	  pw->updateTask->displayInfo->colormap[dlWheelSwitch->control.clr];
	XtSetArg(args[nargs], XmNforeground, pixel); nargs++;

      /* Set Channel and User limits (if apparently not set yet) */
	dlWheelSwitch->limits.loprChannel = lopr;
	if(dlWheelSwitch->limits.loprSrc != PV_LIMITS_USER &&
	  dlWheelSwitch->limits.loprUser == LOPR_DEFAULT) {
	    dlWheelSwitch->limits.loprUser = lopr;
	}
	dlWheelSwitch->limits.hoprChannel = hopr;
	if(dlWheelSwitch->limits.hoprSrc != PV_LIMITS_USER &&
	  dlWheelSwitch->limits.hoprUser == HOPR_DEFAULT) {
	    dlWheelSwitch->limits.hoprUser = hopr;
	}
	dlWheelSwitch->limits.precChannel = precision;
	if(dlWheelSwitch->limits.precSrc != PV_LIMITS_USER &&
	  dlWheelSwitch->limits.precUser == PREC_DEFAULT) {
	    dlWheelSwitch->limits.precUser = precision;
	}

      /* Set values in the widget if src is Channel */
	if(dlWheelSwitch->limits.loprSrc == PV_LIMITS_CHANNEL) {
	    dlWheelSwitch->limits.lopr = lopr;
	    XtSetArg(args[nargs], XmNminValue, &lopr); nargs++;
	}
	if(dlWheelSwitch->limits.hoprSrc == PV_LIMITS_CHANNEL) {
	    dlWheelSwitch->limits.hopr = hopr;
	    XtSetArg(args[nargs], XmNmaxValue, &hopr); nargs++;
	}
	if(dlWheelSwitch->limits.precSrc == PV_LIMITS_CHANNEL) {
	    dlWheelSwitch->limits.prec = precision;
	    XtSetArg(args[nargs], XcNdecimals, (int)precision); nargs++;
	}

#if RECALCULATE_FORMAT
      /* Recalculate the format if none is specified */
	if(!*dlWheelSwitch->format) {
	    char *format;
	  /* Calculate it from hopr, lopr, prec */
	    format = wheelSwitchCalculateFormat(dlWheelSwitch);
	    XtSetArg(args[nargs],XmNformat,format); nargs++;
	}
#endif
	
	XtSetValues(widget, args, nargs);
	XmWheelSwitchSetValue(widget, &pr->value);
    }
}

static void wheelSwitchDestroyCb(XtPointer cd) {
    MedmWheelSwitch *pw = (MedmWheelSwitch *)cd;
    if(pw) {
	medmDestroyRecord(pw->record);
	if(pw->dlElement) pw->dlElement->data = NULL;
	free((char *)pw);
    }
    return;
}

void wheelSwitchValueChanged(Widget w, XtPointer clientData,
  XtPointer callbackStruct)
{
    MedmWheelSwitch *pw = (MedmWheelSwitch *)clientData;
    Record *pr = pw->record;
    XmWheelSwitchCallbackStruct *call_data =
      (XmWheelSwitchCallbackStruct *)callbackStruct;
    double value = *call_data->value;

    UNREFERENCED(w);

#if DEBUG_VALUE
    printf("wheelSwitchValueChanged: value=%g\n",value);
#endif

    if(call_data->reason == XmCR_VALUE_CHANGED) { 
	if(pr->connected) {
	    if(pr->writeAccess) {
		medmSendDouble(pw->record,value);
	      /* Call this to check if the value was allowed */
	    } else {
	      /* Reset it */
		XBell(display,50);
	    }
	  /* Necessary in case the value is reset by the channel */
	    wheelSwitchUpdateValueCb((XtPointer)pw->record);
	}
    }
}

#if CALCULATE_FORMAT
static char *wheelSwitchCalculateFormat(DlWheelSwitch *dlWheelSwitch)
{
    static char format[MAX_TOKEN_LENGTH];
    double maxAbsHoprLopr;
    int width;
    int precision = dlWheelSwitch->limits.prec;

  /* Find the largest absolute value of hopr and l;opr */
    maxAbsHoprLopr = MAX(abs(dlWheelSwitch->limits.hopr),
      abs(dlWheelSwitch->limits.lopr));

  /* Determine how many places in front of the decimal point this
   * requires and add room for the sign, decimal point and precision
   * points after the decimal point */
    if(maxAbsHoprLopr > 1.0) {
	width = (int)log10(maxAbsHoprLopr) + 5 + precision;
    } else {
	width = 5 + precision;
    }
    sprintf(format,"%% %d.%df", width, precision);
    
#if DEBUG_CALCULATE_FORMAT
    printf("wheelSwitchCalculateFormat: hopr=%g lopr=%g prec=%hd\n"
      " maxAbsHoprLopr=%g width=%d precision=%d format=%s\n",
      dlWheelSwitch->limits.hopr,dlWheelSwitch->limits.lopr,
      dlWheelSwitch->limits.prec,maxAbsHoprLopr,width,precision,format);
#endif

    return format;
}
#endif

static void wheelSwitchGetRecord(XtPointer cd, Record **record, int *count) {
    MedmWheelSwitch *pw = (MedmWheelSwitch *)cd;
    *count = 1;
    record[0] = pw->record;
}

DlElement *createDlWheelSwitch(DlElement *p)
{
    DlWheelSwitch *dlWheelSwitch;
    DlElement *dlElement;

    dlWheelSwitch = (DlWheelSwitch *)malloc(sizeof(DlWheelSwitch));
    if(!dlWheelSwitch) return 0;
    if(p) {
	*dlWheelSwitch = *p->structure.wheelSwitch;
    } else {
	objectAttributeInit(&(dlWheelSwitch->object));
	controlAttributeInit(&(dlWheelSwitch->control));
	limitsAttributeInit(&(dlWheelSwitch->limits));
	dlWheelSwitch->label = LABEL_NONE;
	dlWheelSwitch->clrmod = STATIC;
#if !CALCULATE_FORMAT
	strncpy(dlWheelSwitch->format,WHEEL_SWITCH_DEFAULT_FORMAT,
	  MAX_TOKEN_LENGTH);
	dlWheelSwitch->format[MAX_TOKEN_LENGTH-1] = '\0';
#else
	*dlWheelSwitch->format = '\0';
#endif
    }

    if(!(dlElement = createDlElement(DL_WheelSwitch, (XtPointer)dlWheelSwitch,
      &wheelSwitchDlDispatchTable))) {
	free(dlWheelSwitch);
    }

    return(dlElement);
}

DlElement *parseWheelSwitch(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlWheelSwitch *dlWheelSwitch;
    DlElement *dlElement = createDlWheelSwitch(NULL);
    int i = 0;

    if(!dlElement) return 0;
    dlWheelSwitch = dlElement->structure.wheelSwitch;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlWheelSwitch->object));
	    else if(!strcmp(token,"control"))
	      parseControl(displayInfo,&(dlWheelSwitch->control));
	    else if(!strcmp(token,"label")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for(i=FIRST_LABEL_TYPE;i<FIRST_LABEL_TYPE+NUM_LABEL_TYPES;i++) {
		    if(!strcmp(token,stringValueTable[i])) {
			dlWheelSwitch->label = i;
			break;
		    }
		}
	    } else if(!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for(i=FIRST_COLOR_MODE;i<FIRST_COLOR_MODE+NUM_COLOR_MODES;i++) {
		    if(!strcmp(token,stringValueTable[i])) {
			dlWheelSwitch->clrmod = i;
			break;
		    }
		}
	    } else if(!strcmp(token,"limits")) {
		parseLimits(displayInfo,&(dlWheelSwitch->limits));
	    } else if(!strcmp(token,"format")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlWheelSwitch->format,token);
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

void writeDlWheelSwitch(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
    int i;
    char indent[16];
    DlWheelSwitch *dlWheelSwitch = dlElement->structure.wheelSwitch;

    for(i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%s\"wheel switch\" {",indent);
    writeDlObject(stream,&(dlWheelSwitch->object),level+1);
    writeDlControl(stream,&(dlWheelSwitch->control),level+1);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
	if(dlWheelSwitch->label != LABEL_NONE)
	  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,stringValueTable[dlWheelSwitch->label]);
	if(dlWheelSwitch->clrmod != STATIC) 
	  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,stringValueTable[dlWheelSwitch->clrmod]);
	if(dlWheelSwitch->format[0] != '\0')
	  fprintf(stream,"\n%s\tformat=\"%s\"",
	    indent,dlWheelSwitch->format);
#ifdef SUPPORT_0201XX_FILE_FORMAT	
    } else {
	fprintf(stream,"\n%s\tlabel=\"%s\"",indent,stringValueTable[dlWheelSwitch->label]);
	fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,stringValueTable[dlWheelSwitch->clrmod]);
	fprintf(stream,"\n%s\tformat=\"%s\"",indent,dlWheelSwitch->format);
    }
#endif
    writeDlLimits(stream,&(dlWheelSwitch->limits),level+1);
    fprintf(stream,"\n%s}",indent);
}

static void wheelSwitchInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlWheelSwitch *dlWheelSwitch = p->structure.wheelSwitch;
    medmGetValues(pRCB,
      CTRL_RC,       &(dlWheelSwitch->control.ctrl),
      CLR_RC,        &(dlWheelSwitch->control.clr),
      BCLR_RC,       &(dlWheelSwitch->control.bclr),
      LABEL_RC,      &(dlWheelSwitch->label),
      CLRMOD_RC,     &(dlWheelSwitch->clrmod),
      LIMITS_RC,     &(dlWheelSwitch->limits),
      WS_FORMAT_RC,  &(dlWheelSwitch->format),
      -1);
}

static void wheelSwitchGetLimits(DlElement *pE, DlLimits **ppL, char **pN)
{
    DlWheelSwitch *dlWheelSwitch = pE->structure.wheelSwitch;

    *(ppL) = &(dlWheelSwitch->limits);
    *(pN) = dlWheelSwitch->control.ctrl;
}

static void wheelSwitchGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlWheelSwitch *dlWheelSwitch = p->structure.wheelSwitch;
    medmGetValues(pRCB,
      X_RC,          &(dlWheelSwitch->object.x),
      Y_RC,          &(dlWheelSwitch->object.y),
      WIDTH_RC,      &(dlWheelSwitch->object.width),
      HEIGHT_RC,     &(dlWheelSwitch->object.height),
      CTRL_RC,       &(dlWheelSwitch->control.ctrl),
      CLR_RC,        &(dlWheelSwitch->control.clr),
      BCLR_RC,       &(dlWheelSwitch->control.bclr),
      LABEL_RC,      &(dlWheelSwitch->label),
      CLRMOD_RC,     &(dlWheelSwitch->clrmod),
      LIMITS_RC,     &(dlWheelSwitch->limits),
      WS_FORMAT_RC,  &(dlWheelSwitch->format),
      -1);
}

static void wheelSwitchSetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlWheelSwitch *dlWheelSwitch = p->structure.wheelSwitch;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlWheelSwitch->control.bclr),
      -1);
}

static void wheelSwitchSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlWheelSwitch *dlWheelSwitch = p->structure.wheelSwitch;
    medmGetValues(pRCB,
      CLR_RC,        &(dlWheelSwitch->control.clr),
      -1);
}
