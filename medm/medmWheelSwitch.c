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
#define DEBUG_FONTLIST 0
#define DEBUG_TRAVERSAL 0
#define DEBUG_REPEAT 0
#define DEBUG_NEWSTRING 0

#define USE_FONTLISTINDEX 1

#define FONT_ALGORITHM 2

#include "medm.h"

#if DEBUG_TRAVERSAL
#ifndef WIN32
/* Not found on WIN32 */
/* From TMprint.c */
String _XtPrintXlations(Widget w, XtTranslations xlations,
  Widget accelWidget, _XtBoolean includeRHS);
#endif
#endif

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
static char *wheelSwitchCalculateFormat(DlWheelSwitch *dlWheelSwitch);
#if USE_FONTLISTINDEX
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

#if USE_FONTLISTINDEX
static int wheelSwitchFontListIndex(int height)
{
    int i;
    unsigned effHeight = MAX(height-4,0);  /* Subtract shadows */

#if DEBUG_FONTLIST
    {
	static int first = 1;
	if(first) {
	    first = 0;
	    printf("\nMEDM Fonts\n");
	    for(i = MAX_FONTS-1; i >=  0; i--) {
		int totalFontHeight =
		  fontTable[i]->ascent + 2*fontTable[i]->descent;
		printf("%2d widgetDM_%-2d ascent=%-2d descent=%-2d "
		  "total=%-2d wstotal=%-2d\n",
		  i,fontSizeTable[i],
		  fontTable[i]->ascent,fontTable[i]->descent,
		  fontTable[i]->ascent+fontTable[i]->descent,
		  totalFontHeight);
	    }
	    printf("\n");
	}
    }
#endif

  /* Loop over the MEDM fonts */
    for(i = MAX_FONTS-1; i >=  0; i--) {
	int totalFontHeight = fontTable[i]->ascent+2*fontTable[i]->descent;

#if FONT_ALGORITHM == 1
      /* Don't allow totalFontHeight to exceed .5 of effHeight
       * (Similar to algorithm in WheelSwitch for patterns) */
	int testHeight = effheight/2;
	if(totalFontHeight <= testHeight) return(i);
#elif FONT_ALGORITHM == 2
      /* Use actual height of buttons */
	int buttonHeight = XTextWidth(fontTable[i], "0", 1);
	int testHeight = MAX(effHeight-2*buttonHeight,0);
	if(totalFontHeight <= testHeight) return(i);
#else
# error	Undefined FONT_ALGORITHM
#endif
    }

  /* Fallback is to return the smallest font */
    return (0);
}
#endif

void executeDlWheelSwitch(DisplayInfo *displayInfo, DlElement *dlElement)
{
    MedmWheelSwitch *pw = NULL;
    Arg args[28];
    int nargs;
#if 0
    int usedHeight, usedCharWidth, bestSize, preferredHeight;
#endif
#if USE_FONTLISTINDEX
    int fontIndex;
#endif
    Widget localWidget;
    DlWheelSwitch *dlWheelSwitch = dlElement->structure.wheelSwitch;
    char *format;

#if DEBUG_COMPOSITE
    print("executeDlWheelSwitch: dlWheelSwitch=%x\n",dlWheelSwitch);
#endif
#if DEBUG_NEWSTRING
    printf("executeDlWheelSwitch: start\n");
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

      /* Update the limits to reflect current src's */
	updatePvLimits(&dlWheelSwitch->limits);

      /* Create the widget */
	nargs = 0;
	if(!*dlWheelSwitch->format) {
	  /* Calculate it from hopr, lopr, prec */
	    format = wheelSwitchCalculateFormat(dlWheelSwitch);
	} else {
	  /* Use the specified one */
	    format = dlWheelSwitch->format;
	}
#if DEBUG_REPEAT
	XtSetArg(args[nargs],XmNrepeatInterval,1000); nargs++;
#endif
	XtSetArg(args[nargs],XmNformat,format); nargs++;
	XtSetArg(args[nargs],XmNmarginWidth,0); nargs++;
	XtSetArg(args[nargs],XmNmarginHeight,0); nargs++;
	XtSetArg(args[nargs],XmNconformToContent,False); nargs++;
	if(globalDisplayListTraversalMode == DL_EDIT) {
	    static double zero=0.5,min=-DBL_MAX,max=DBL_MAX;

	  /* Specify fixed values for the EDIT display.  Use large min
	   * and max to allow the maximum number of buttons to be
	   * drawn */
	    XtSetArg(args[nargs],XmNvalue,&zero); nargs++;
	    XtSetArg(args[nargs],XmNminValue,&min); nargs++;
	    XtSetArg(args[nargs],XmNmaxValue,&max); nargs++;

	  /* Disable the buttons and keyboard */
	    XtSetArg(args[nargs],XmNdisableInput,True); nargs++;
	} else {
	  /* These will probably be 0 and 1 the first time until the
	   * graphical info comes */
	    XtSetArg(args[nargs],XmNminValue,&dlWheelSwitch->limits.lopr); nargs++;
	    XtSetArg(args[nargs],XmNmaxValue,&dlWheelSwitch->limits.hopr); nargs++;

	  /* Enable the buttons and keyboard (is the default)*/
	    XtSetArg(args[nargs],XmNdisableInput,False); nargs++;
	    XtSetArg(args[nargs],XmNhighlightOnEnter,True); nargs++;
	}
#if USE_FONTLISTINDEX
      /* Determine the font */
	fontIndex=wheelSwitchFontListIndex(dlWheelSwitch->object.height);
	XtSetArg(args[nargs],XmNfontList,fontListTable[fontIndex]); nargs++;
#if DEBUG_FONTLIST
	printf("executeDlWheelSwitch: fontIndex=%d width=%u height=%u\n",
	  fontIndex,dlWheelSwitch->object.width,dlWheelSwitch->object.height);
#endif

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

      /* Add the pointer to the Channel structure as userData */
	XtSetArg(args[nargs],XmNuserData,(XtPointer)pw); nargs++;
#if DEBUG_NEWSTRING
	printf("executeDlWheelSwitch: format=%p |%s|\n",
	  (void *)format,format);
#endif
	localWidget = XtCreateWidget("wheelSwitch",
	  wheelSwitchWidgetClass, displayInfo->drawingArea, args, nargs);
#if DEBUG_NEWSTRING
	printf("executeDlWheelSwitch: localWidget=%p\n",
	  (void *)localWidget);
#endif
	dlElement->widget = localWidget;
 	if(displayInfo->traversalMode == DL_EXECUTE) {
	    pw->dlElement->widget = localWidget;
	    XtAddCallback(localWidget, XmNvalueChangedCallback,
	      wheelSwitchValueChanged,(XtPointer)pw);
	} else if(displayInfo->traversalMode == DL_EDIT) {
	    addCommonHandlers(localWidget, displayInfo);
	    XtManageChild(localWidget);
	}
#if DEBUG_TRAVERSAL
	{
	    XtTranslations xlations=NULL;
	    String xString=NULL;

	    XtVaGetValues(localWidget,XtNtranslations,&xlations,NULL);
	    print("WheelSwitch translations: [%s]\n",
	      displayInfo->traversalMode == DL_EDIT?"EDIT":"EXECUTE");
	    printf("  %-9s %-7s TabGroup=%p widget=%p\n",
	      XtIsManaged(localWidget)?"Managed":"Unmanaged",
	      XmIsTraversable(localWidget)?"Trav":"NotTrav",
	      (void *)XmGetTabGroup(localWidget),
	      (void *)localWidget);
	    if(xlations) {
#ifndef WIN32
		xString= _XtPrintXlations(localWidget,xlations,NULL,True);
#endif
		print("%s\n",xString?xString:"Null");
		if(xString) XtFree(xString);
	    } else {
		printf(" XtNtranslations not found\n");
	    }
	}
#endif
#if DEBUG_NEWSTRING
	printf("executeDlWheelSwitch: End of create\n");
#endif
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

      /* Recalculate the format if none is specified */
	if(!*dlWheelSwitch->format) {
	    char *format;
	  /* Calculate it from hopr, lopr, prec */
	    format = wheelSwitchCalculateFormat(dlWheelSwitch);
#if DEBUG_NEWSTRING
	    printf("executeDlWheelSwitch: format=%p |%s|\n",
	      (void *)format,format);
#endif
	    XtSetArg(args[nargs],XmNformat,format); nargs++;
	}
#if DEBUG_NEWSTRING
	printf("executeDlWheelSwitch: XtSetValues for existing widget=%p\n",
	  (void *)dlElement->widget);
#endif
    	XtSetValues(dlElement->widget,args,nargs);
    }
#if DEBUG_NEWSTRING
    printf("executeDlWheelSwitch: end\n");
#endif
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
	    case STATIC:
	    case DISCRETE:
		break;
	    case ALARM:
		pr->monitorSeverityChanged = True;
		XmWheelSwitchSetForeground(widget,
		  alarmColor(pr->severity));
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
    Arg args[5];
    int nargs=0;

#if DEBUG_NEWSTRING
    printf("wheelSwitchUpdateGraphicalInfoCb: start\n");
#endif
    switch (pr->dataType) {
    case DBF_STRING:
	medmPostMsg(1,"wheelSwitchUpdateGraphicalInfoCb:\n"
	  "  Illegal channel type for %s\n"
	  "  Cannot attach wheelSwitch\n",
	  dlWheelSwitch->control.ctrl);
	return;
    case DBF_ENUM:
    case DBF_CHAR:
    case DBF_INT:
    case DBF_LONG:
    case DBF_FLOAT:
    case DBF_DOUBLE:
	hopr = pr->hopr;
	lopr = pr->lopr;
	precision = pr->precision;
	break;
    default:
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
	}

      /* Recalculate the format if none is specified */
	if(!*dlWheelSwitch->format) {
	    char *format;
	  /* Calculate it from hopr, lopr, prec */
	    format = wheelSwitchCalculateFormat(dlWheelSwitch);
#if DEBUG_NEWSTRING
	    printf("wheelSwitchUpdateGraphicalInfoCb: format=%p |%s|\n",
	      (void *)format,format);
#endif
	    XtSetArg(args[nargs],XmNformat,format); nargs++;
	}

      /* Set the value (Could have used XmWheelSwitchSetValue(widget,
       * &pr->value) after XtSetValues, but this results in two calls
       * to SetValues with no added benefit */
	XtSetArg(args[nargs],XmNcurrentValue,&pr->value); nargs++;
#if DEBUG_NEWSTRING
	printf("wheelSwitchUpdateGraphicalInfoCb: XtSetValues for widget=%p\n",
	  (void *)widget);
#endif
	XtSetValues(widget, args, nargs);
    }
#if DEBUG_NEWSTRING
    printf("wheelSwitchUpdateGraphicalInfoCb: end\n");
#endif
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
#if DEBUG_VALUE || DEBUG_NEWSTRING
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

static char *wheelSwitchCalculateFormat(DlWheelSwitch *dlWheelSwitch)
{
    static char format[MAX_TOKEN_LENGTH];
    double maxAbsHoprLopr;
    int width;
    int precision = dlWheelSwitch->limits.prec;

    if(globalDisplayListTraversalMode == DL_EDIT) {
      /* Return the default format in EDIT mode */
	strcpy(format, WHEEL_SWITCH_DEFAULT_FORMAT);
    } else {
      /* Limit the values */
	if(precision < 0) precision=0;

      /* Find the largest absolute value of hopr and lopr */
	maxAbsHoprLopr = MAX(fabs(dlWheelSwitch->limits.hopr),
	  fabs(dlWheelSwitch->limits.lopr));

      /* Determine how many places in front of the decimal point this
       * requires and add room for the sign, decimal point and precision
       * characters after the decimal point */
	if(maxAbsHoprLopr > 1.0) {
	    width = (int)log10(maxAbsHoprLopr) + 3 + precision;
	} else {
	    width = 2 + precision;
	}
	sprintf(format,"%% %d.%df", width, precision);
    }

#if DEBUG_CALCULATE_FORMAT
    if(globalDisplayListTraversalMode == DL_EDIT) {
	printf("wheelSwitchCalculateFormat[EDIT]: hopr=%g lopr=%g prec=%hd"
	  " format=%s\n",
	  dlWheelSwitch->limits.hopr,dlWheelSwitch->limits.lopr,
	  dlWheelSwitch->limits.prec,format);
    } else {
	printf("wheelSwitchCalculateFormat: hopr=%g lopr=%g prec=%hd"
	  " format=%s\n"
	  " maxAbsHoprLopr=%g width=%d precision=%d\n",
	  dlWheelSwitch->limits.hopr,dlWheelSwitch->limits.lopr,
	  dlWheelSwitch->limits.prec,format,maxAbsHoprLopr,width,precision);
    }
#endif

    return format;
}

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
	dlWheelSwitch->clrmod = STATIC;
	*dlWheelSwitch->format = '\0';
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
	    if(!strcmp(token,"object")) {
	      parseObject(displayInfo,&(dlWheelSwitch->object));
	    } else if(!strcmp(token,"control")) {
	      parseControl(displayInfo,&(dlWheelSwitch->control));
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
	if(dlWheelSwitch->clrmod != STATIC)
	  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,stringValueTable[dlWheelSwitch->clrmod]);
	if(dlWheelSwitch->format[0] != '\0')
	  fprintf(stream,"\n%s\tformat=\"%s\"",
	    indent,dlWheelSwitch->format);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
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
