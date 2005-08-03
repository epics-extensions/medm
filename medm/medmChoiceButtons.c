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

#define DEBUG_TOGGLE_BUTTONS 0
#define DEBUG_ACCESS 0

#include "medm.h"

typedef struct _MedmChoiceButtons {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           *record;
} MedmChoiceButtons;

void choiceButtonCreateRunTimeInstance(DisplayInfo *,DlElement *);
void choiceButtonCreateEditInstance(DisplayInfo *, DlElement *);

static void choiceButtonDraw(XtPointer);
static void choiceButtonUpdateValueCb(XtPointer);
static void choiceButtonUpdateGraphicalInfoCb(XtPointer);
static void choiceButtonDestroyCb(XtPointer cd);
static void choiceButtonGetRecord(XtPointer, Record **, int *);
static void choiceButtonInheritValues(ResourceBundle *pRCB, DlElement *p);
static void choiceButtonSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void choiceButtonSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void choiceButtonGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable choiceButtonDlDispatchTable = {
    createDlChoiceButton,
    NULL,
    executeDlChoiceButton,
    hideDlChoiceButton,
    writeDlChoiceButton,
    NULL,
    choiceButtonGetValues,
    choiceButtonInheritValues,
    choiceButtonSetBackgroundColor,
    choiceButtonSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

int choiceButtonFontListIndex(DlObject *po,
  int numButtons,
  Stacking stacking)
{
    int i, useNumButtons;
    short sqrtNumButtons;

#define SHADOWS_SIZE 4             /* each Toggle Button has 2 shadows...*/

  /* pay cost of sqrt() and ceil() once */
    sqrtNumButtons = (short) ceil(sqrt((double)numButtons));
    sqrtNumButtons = MAX(1,sqrtNumButtons);
    useNumButtons = MAX(1,numButtons);

  /* more complicated calculation based on orientation, etc */
    for(i = MAX_FONTS-1; i >=  0; i--) {
	switch (stacking) {
	case ROW:
	    if((int)(po->height/useNumButtons - SHADOWS_SIZE) >=
	      (fontTable[i]->ascent + fontTable[i]->descent))
	      return(i);
	    break;
	case ROW_COLUMN:
	    if((int)(po->height/sqrtNumButtons - SHADOWS_SIZE) >=
	      (fontTable[i]->ascent + fontTable[i]->descent))
	      return(i);
	    break;
	case COLUMN:
	    if((int)(po->height - SHADOWS_SIZE) >=
	      (fontTable[i]->ascent + fontTable[i]->descent))
	      return(i);
	    break;
	}
    }
    return 0;
}

Widget createToggleButtons(Widget parent,
  DlObject *po,
  Pixel fg,
  Pixel bg,
  int numberOfButtons,
  char **labels,
  XtPointer userData,
  Widget *buttons,
  Stacking stacking) {
    Widget widget;
    Arg wargs[20];
    int i, n, maxChars, usedWidth, usedHeight;
    short sqrtEntries;
    double dSqrt;
    XmFontList fontList;

    maxChars = 0;
    for(i = 0; i < numberOfButtons; i++) {
	maxChars = MAX((size_t)maxChars, strlen(labels[i]));
    }
    n = 0;
    XtSetArg(wargs[n],XmNx,(Position)po->x); n++;
    XtSetArg(wargs[n],XmNy,(Position)po->y); n++;
    XtSetArg(wargs[n],XmNwidth,(Dimension)po->width); n++;
    XtSetArg(wargs[n],XmNheight,(Dimension)po->height); n++;
    XtSetArg(wargs[n],XmNforeground,fg); n++;
    XtSetArg(wargs[n],XmNbackground,bg); n++;
    XtSetArg(wargs[n],XmNindicatorOn,False); n++;
    XtSetArg(wargs[n],XmNmarginWidth,0); n++;
    XtSetArg(wargs[n],XmNmarginHeight,0); n++;
    XtSetArg(wargs[n],XmNresizeWidth,False); n++;
    XtSetArg(wargs[n],XmNresizeHeight,False); n++;
    XtSetArg(wargs[n],XmNspacing,0); n++;
    XtSetArg(wargs[n],XmNrecomputeSize,False); n++;
    XtSetArg(wargs[n],XmNuserData,userData); n++;
    switch (stacking) {
    case ROW:
	XtSetArg(wargs[n],XmNorientation,XmVERTICAL); n++;
	usedWidth = po->width;
	usedHeight = (int) (po->height/MAX(1,numberOfButtons));
	break;
    case COLUMN:
	XtSetArg(wargs[n],XmNorientation,XmHORIZONTAL); n++;
	usedWidth = (int) (po->width/MAX(1,numberOfButtons));
	usedHeight = po->height;
	break;
    case ROW_COLUMN:
	XtSetArg(wargs[n],XmNorientation,XmVERTICAL); n++;
	dSqrt = ceil(sqrt((double)numberOfButtons));
	sqrtEntries = MAX(2,(short)dSqrt);
	XtSetArg(wargs[n],XmNnumColumns,sqrtEntries); n++;
	usedWidth = po->width/sqrtEntries;
	usedHeight = po->height/sqrtEntries;
	break;
    default :
	usedWidth = 10;
	usedHeight = 10;
    }
    widget = XmCreateRadioBox(parent,"choice",wargs,n);
  /* now make push-in type radio buttons of the correct size */
    fontList = fontListTable[choiceButtonFontListIndex(
      po,numberOfButtons,stacking)];
    n = 0;
    XtSetArg(wargs[n],XmNshadowThickness,2); n++;
    XtSetArg(wargs[n],XmNhighlightThickness,0); n++;
    XtSetArg(wargs[n],XmNrecomputeSize,False); n++;
    XtSetArg(wargs[n],XmNwidth,(Dimension)usedWidth); n++;
    XtSetArg(wargs[n],XmNheight,(Dimension)usedHeight); n++;
    XtSetArg(wargs[n],XmNfontList,fontList); n++;
    XtSetArg(wargs[n],XmNalignment,XmALIGNMENT_CENTER); n++;
    XtSetArg(wargs[n],XmNindicatorOn,False); n++;
    XtSetArg(wargs[n],XmNindicatorSize,0); n++;
    XtSetArg(wargs[n],XmNspacing,0); n++;
    XtSetArg(wargs[n],XmNvisibleWhenOff,False); n++;
    XtSetArg(wargs[n],XmNforeground,fg); n++;
    XtSetArg(wargs[n],XmNbackground,bg); n++;
    XtSetArg(wargs[n],XmNuserData,userData); n++;
    for(i = 0; i < numberOfButtons; i++) {
	XmString xmStr;
	xmStr = XmStringCreateLocalized(labels[i]);
	XtSetArg(wargs[n],XmNlabelString,xmStr);
      /* Use gadgets here so that changing foreground
	 of radioBox changes buttons */
	buttons[i] = XmCreateToggleButtonGadget(widget,"choiceButton",
	  wargs,n+1);
      /* MDA - for some reason, need to do this
	 after the fact for gadgets...  */
	XtVaSetValues(buttons[i],XmNalignment,XmALIGNMENT_CENTER,NULL);
	XmStringFree(xmStr);
    }
    return widget;
}

void choiceButtonValueChangedCb(Widget  w, XtPointer clientData,
  XtPointer callbackStruct)
{
    MedmChoiceButtons *pcb;
    int btnNumber = (int)clientData;
    XmToggleButtonCallbackStruct *call_data =
      (XmToggleButtonCallbackStruct *)callbackStruct;
    Record *pd;

#if DEBUG_TOGGLE_BUTTONS
#if 0
    printf("\nchoiceButtonValueChangedCb:  btnNumber=%d call_data->set=%d\n",
      btnNumber,call_data->set);
#endif
    {
	int ic;
	Boolean set,vis,radioBehavior;
	unsigned char indicatorType;
	WidgetList children;
	Cardinal numChildren;

	XtVaGetValues(XtParent(w),
	  XmNchildren,&children, XmNnumChildren,&numChildren,
	  NULL);
	XtVaGetValues(XtParent(children[0]),
	  XmNradioBehavior,&radioBehavior,NULL);
	printf("\nchoiceButtonValueChangedCb: btnNumber=%d  XmNradioBehavior=%d  XmNindicatorType: [XmONE_OF_MANY=%d]\n",
	  btnNumber,radioBehavior,XmONE_OF_MANY);
	for(ic=0; ic < (int)numChildren; ic++) {
	    XtVaGetValues(children[ic],
	      XmNset,&set,
	      XmNindicatorType,&indicatorType,
	      XmNvisibleWhenOff,&vis,
	      NULL);
	    printf("Button %2d:  XmNset=%d  XmNindicatorType=%d  "
	      "XmNvisibleWhenOff=%d\n",ic,set,indicatorType,vis);
	}
    }
#endif
  /* Only do ca_put if this widget actually initiated the channel change */
    if(call_data->event != NULL && call_data->set == True) {

      /* Button's parent (menuPane) has the displayInfo pointer */
	XtVaGetValues(XtParent(w),XmNuserData,&pcb,NULL);
	pd = pcb->record;

	if(pd->connected) {
	    if(pd->writeAccess) {
#ifdef MEDM_CDEV
		if(pd->stateStrings)
		  medmSendString(pd, pd->stateStrings[btnNumber]);
#else
		medmSendDouble(pcb->record,(double)btnNumber);
#endif
	    } else {
		XBell(display,50);
		choiceButtonUpdateValueCb((XtPointer)pcb->record);
	    }
	}
    }
}

static void choiceButtonUpdateGraphicalInfoCb(XtPointer cd)
{
    Record *pr = (Record *) cd;
    MedmChoiceButtons *cb = (MedmChoiceButtons *) pr->clientData;
    DlElement *dlElement = cb->dlElement;
    DlChoiceButton *pCB = dlElement->structure.choiceButton;
    int i;
    Pixel fg, bg;
    char *labels[16];
    Widget buttons[16];

#if DEBUG_TOGGLE_BUTTONS || DEBUG_ACCESS
    printf("\nchoiceButtonUpdateGraphicalInfoCb:\n");
#endif

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! This is a temporary work around !!!!! */
  /* !!!!! for the reconnection.           !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  /* KE: This causes it to not do anything for the reconnection */
    medmRecordAddGraphicalInfoCb(cb->record,NULL);

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! End work around                 !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

    if(pr->dataType != DBF_ENUM) {
	medmPostMsg(1,"choiceButtonUpdateGraphicalInfoCb:\n"
	  "  %s is not an ENUM type\n"
	  "  Cannot create Choice Button\n",
	  pCB->control.ctrl);
	return;
    }
    if(pr->hopr <= 0.0) {
	medmPostMsg(1,"choiceButtonUpdateGraphicalInfoCb:\n"
	  "  Cannot create Choice Button for %s\n",
	  "  There are no states to assign to buttons\n",
	  pCB->control.ctrl);
	return;
    }

    for(i = 0; i <= pr->hopr; i++) {
	labels[i] = pr->stateStrings[i];
    }
    fg = (pCB->clrmod == ALARM ? alarmColor(pr->severity) :
      cb->updateTask->displayInfo->colormap[pCB->control.clr]);
    bg = cb->updateTask->displayInfo->colormap[pCB->control.bclr];
    dlElement->widget = createToggleButtons(
      cb->updateTask->displayInfo->drawingArea,
      &(pCB->object),
      fg,
      bg,
      (int)(pr->hopr+1.5),     /* Record->hopr is a double */
      labels,
      (XtPointer) cb,
      buttons,
      pCB->stacking);

    for(i = 0; i <= pr->hopr; i++) {
	if(i==(int)pr->value)
	  XmToggleButtonGadgetSetState(buttons[i],True,True);
	XtAddCallback(buttons[i],XmNvalueChangedCallback,
	  (XtCallbackProc)choiceButtonValueChangedCb,(XtPointer)i);
	XtManageChild(buttons[i]);
    }
    choiceButtonUpdateValueCb(cd);
}

static void choiceButtonUpdateValueCb(XtPointer cd) {
    MedmChoiceButtons *pcb = (MedmChoiceButtons *)((Record *) cd)->clientData;

#if DEBUG_TOGGLE_BUTTONS || DEBUG_ACCESS
    printf("\nchoiceButtonUpdateValueCb:\n");
#endif
    updateTaskMarkUpdate(pcb->updateTask);
}

static void choiceButtonDraw(XtPointer cd) {
    MedmChoiceButtons *pcb = (MedmChoiceButtons *) cd;
    Record *pr = pcb->record;
    DlElement *dlElement = pcb->dlElement;
    Widget widget = dlElement->widget;
    DlChoiceButton *dlChoiceButton = dlElement->structure.choiceButton;

#if DEBUG_ACCESS
    printf("\nchoiceButtonDraw: widget=%x managed=%s\n",
      widget,widget?(XtIsManaged(widget)?"Yes":"No"):"NA");
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
#if DEBUG_ACCESS
	    printf("  [connected/access] widget=%x managed=%s precision=%d\n",
	      widget,widget?(XtIsManaged(widget)?"Yes":"No"):"NA",
	      pr->precision);
#endif

	    if(!widget) return;
	    if(widget && !XtIsManaged(widget)) {
		addCommonHandlers(widget, pcb->updateTask->displayInfo);
		XtManageChild(widget);
	    }
	    if(pr->writeAccess)
	      XDefineCursor(XtDisplay(widget),XtWindow(widget),rubberbandCursor);
	    else
	      XDefineCursor(XtDisplay(widget),XtWindow(widget),noWriteAccessCursor);
	    if(pr->precision < 0) return;    /* Wait for pr->value */
	    if(pr->dataType == DBF_ENUM) {
		WidgetList children;
		Cardinal numChildren;
		int i;
		XtVaGetValues(widget,
		  XmNchildren,&children, XmNnumChildren,&numChildren,
		  NULL);
	      /* Change the color */
		switch (dlChoiceButton->clrmod) {
		case STATIC :
		case DISCRETE :
		    break;
		case ALARM :
		    pr->monitorSeverityChanged = True;
		    XtVaSetValues(widget,XmNforeground,alarmColor(pr->severity),NULL);
		    break;
		default :
		    medmPostMsg(1,"choiceButtonUpdateValueCb:\n");
		    medmPrintf(0,"  Channel Name : %s\n",dlChoiceButton->control.ctrl);
		    medmPrintf(0,"  Message: Unknown color modifier\n");
		    return;
		}
		i = (int) pr->value;
		if((i >= 0) && (i < (int)numChildren)) {
#if DEBUG_TOGGLE_BUTTONS
		    {
			int ic;
			Boolean set,vis,radioBehavior;
			unsigned char indicatorType;

			XtVaGetValues(XtParent(children[0]),
			  XmNradioBehavior,&radioBehavior,NULL);
			printf("\nchoiceButtonDraw: i=%d  XmNradioBehavior=%d  XmNindicatorType: [XmONE_OF_MANY=%d]\n",
			  i,radioBehavior,XmONE_OF_MANY);
			for(ic=0; ic < (int)numChildren; ic++) {
			    XtVaGetValues(children[ic],
			      XmNset,&set,
			      XmNindicatorType,&indicatorType,
			      XmNvisibleWhenOff,&vis,
			      NULL);
			    printf("Button %2d:  XmNset=%d  XmNindicatorType=%d  "
			      "XmNvisibleWhenOff=%d\n",ic,set,indicatorType,vis);
			}
		    }
#endif
		    XmToggleButtonGadgetSetState(children[i],True,True);
#if DEBUG_TOGGLE_BUTTONS > 1
		    {
			int ic;
			Boolean set,vis;
			unsigned char indicatorType;

			printf("after:\n");
			for(ic=0; ic < (int)numChildren; ic++) {
			    XtVaGetValues(children[ic],
			      XmNset,&set,
			      XmNindicatorType,&indicatorType,
			      XmNvisibleWhenOff,&vis,
			      NULL);
			    printf("Button %2d:  XmNset=%d  XmNindicatorType=%d  "
			      "XmNvisibleWhenOff=%d\n",ic,set,indicatorType,vis);
			}
		    }
#endif
		} else {
		    medmPostMsg(1,"choiceButtonUpdateValueCb: Got state %d.\n"
		      "  Only have strings for %d states (starting at 0).\n"
		      "%s  %s\n",
		      i,(int)numChildren,
		      ((int)numChildren == 16)?
		      "  [Channel Access is limited to 16 strings, "
		      "but there may be more states]\n":"",
		      dlChoiceButton->control.ctrl);
		    return;
		}
	    } else {
		medmPostMsg(1,"choiceButtonUpdateValueCb:\n");
		medmPrintf(1,"  Channel Name: %s\n",dlChoiceButton->control.ctrl);
		medmPrintf(1,"  Message: Data type must be enum\n");
		return;
	    }
	} else {
#if DEBUG_ACCESS
	    printf("  [connected/no access] widget=%x managed=%s precision=%d\n",
	      widget,widget?(XtIsManaged(widget)?"Yes":"No"):"NA",
	      pr->precision);
#endif
	    if(widget && XtIsManaged(widget))
	      XtUnmanageChild(widget);
	    drawBlackRectangle(pcb->updateTask);
	}
    } else {
#if DEBUG_ACCESS
	printf("  [not connected] widget=%x managed=%s precision=%d\n",
	  widget,widget?(XtIsManaged(widget)?"Yes":"No"):"NA",
	  pr->precision);
#endif
	if(widget && XtIsManaged(widget))
	  XtUnmanageChild(widget);
	drawWhiteRectangle(pcb->updateTask);
    }
}

void choiceButtonCreateRunTimeInstance(DisplayInfo *displayInfo,
  DlElement *dlElement) {

    MedmChoiceButtons *pcb;
    DlChoiceButton *dlChoiceButton = dlElement->structure.choiceButton;

    if(dlElement->data) {
	pcb = (MedmChoiceButtons *)dlElement->data;
    } else {
	pcb = (MedmChoiceButtons *)malloc(sizeof(MedmChoiceButtons));
	dlElement->data = (void *)pcb;
	if(pcb == NULL) {
	    medmPrintf(1,"\nchoiceButtonCreateRunTimeInstance:"
	      " Memory allocation error\n");
	    return;
	}
      /* Pre-initialize */
	pcb->updateTask = NULL;
	pcb->record = NULL;
	pcb->dlElement = dlElement;

	pcb->updateTask = updateTaskAddTask(displayInfo,
	  &(dlChoiceButton->object),
	  choiceButtonDraw,
	  (XtPointer) pcb);
	if(pcb->updateTask == NULL) {
	    medmPrintf(1,"\nchoiceButtonCreateRunTimeInstance: Memory allocation error\n");
	} else {
	    updateTaskAddDestroyCb(pcb->updateTask,choiceButtonDestroyCb);
	    updateTaskAddNameCb(pcb->updateTask,choiceButtonGetRecord);
	}

	pcb->record = medmAllocateRecord(dlChoiceButton->control.ctrl,
	  choiceButtonUpdateValueCb,
	  choiceButtonUpdateGraphicalInfoCb,
	  (XtPointer) pcb);
      /* Put up white rectangle so that unconnected channels are obvious */
	drawWhiteRectangle(pcb->updateTask);
    }
}

void choiceButtonCreateEditInstance(DisplayInfo *displayInfo, DlElement *dlElement)
{
    Widget buttons[2];
    DlChoiceButton *dlChoiceButton = dlElement->structure.choiceButton;
    char *labels[] = {"0...","1..."};

    dlElement->widget = createToggleButtons(
      displayInfo->drawingArea,
      &(dlChoiceButton->object),
      displayInfo->colormap[dlChoiceButton->control.clr],
      displayInfo->colormap[dlChoiceButton->control.bclr],
      2,
      labels,
      (XtPointer)displayInfo,
      buttons,
      dlChoiceButton->stacking);
    XmToggleButtonGadgetSetState(buttons[0],True,True);
    XtManageChildren(buttons,2);

  /* Add handlers */
    addCommonHandlers(dlElement->widget, displayInfo);

    XtManageChild(dlElement->widget);

    return;
}

void executeDlChoiceButton(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(displayInfo->traversalMode == DL_EXECUTE) {
	choiceButtonCreateRunTimeInstance(displayInfo, dlElement);
    } else if(displayInfo->traversalMode == DL_EDIT) {
	if(dlElement->widget) {
#if 1
	    XtDestroyWidget(dlElement->widget);
#endif
	    dlElement->widget = 0;
	}
	choiceButtonCreateEditInstance(displayInfo, dlElement);
    }
}

void hideDlChoiceButton(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element with a widget */
    hideWidgetElement(displayInfo, dlElement);
}

static void choiceButtonDestroyCb(XtPointer cd) {
    MedmChoiceButtons *pcb = (MedmChoiceButtons *) cd;
    if(pcb) {
	medmDestroyRecord(pcb->record);
	if(pcb->dlElement->data) pcb->dlElement->data = NULL;
	free((char *)pcb);
    }
}

static void choiceButtonGetRecord(XtPointer cd, Record **record, int *count) {
    MedmChoiceButtons *pcb = (MedmChoiceButtons *) cd;
    *count = 1;
    record[0] = pcb->record;
}

DlElement *createDlChoiceButton(DlElement *p)
{
    DlChoiceButton *dlChoiceButton;
    DlElement *dlElement;

    dlChoiceButton = (DlChoiceButton *)malloc(sizeof(DlChoiceButton));
    if(!dlChoiceButton) return 0;
    if(p) {
	*dlChoiceButton = *(p->structure.choiceButton);
    } else {
	objectAttributeInit(&(dlChoiceButton->object));
	controlAttributeInit(&(dlChoiceButton->control));
	dlChoiceButton->clrmod = STATIC;
	dlChoiceButton->stacking = ROW;
    }

    if(!(dlElement = createDlElement(DL_ChoiceButton,
      (XtPointer)      dlChoiceButton,
      &choiceButtonDlDispatchTable))) {
	free(dlChoiceButton);
    }

    return(dlElement);
}

DlElement *parseChoiceButton(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlChoiceButton *dlChoiceButton;
    DlElement *dlElement = createDlChoiceButton(NULL);

    if(!dlElement) return 0;
    dlChoiceButton = dlElement->structure.choiceButton;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlChoiceButton->object));
	    else if(!strcmp(token,"control"))
	      parseControl(displayInfo,&(dlChoiceButton->control));
	    else if(!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"static"))
		  dlChoiceButton->clrmod = STATIC;
		else if(!strcmp(token,"alarm"))
		  dlChoiceButton->clrmod = ALARM;
		else if(!strcmp(token,"discrete"))
		  dlChoiceButton->clrmod = DISCRETE;
	    } else if(!strcmp(token,"stacking")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"row"))
		  dlChoiceButton->stacking = ROW;
		else if(!strcmp(token,"column"))
		  dlChoiceButton->stacking = COLUMN;
		else if(!strcmp(token,"row column"))
		  dlChoiceButton->stacking = ROW_COLUMN;
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

void writeDlChoiceButton(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
    char indent[16];
    DlChoiceButton *dlChoiceButton = dlElement->structure.choiceButton;

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
	fprintf(stream,"\n%s\"choice button\" {",indent);
	writeDlObject(stream,&(dlChoiceButton->object),level+1);
	writeDlControl(stream,&(dlChoiceButton->control),level+1);
	if(dlChoiceButton->clrmod != STATIC)
	  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	    stringValueTable[dlChoiceButton->clrmod]);
	if(dlChoiceButton->stacking != ROW)
	  fprintf(stream,"\n%s\tstacking=\"%s\"",indent,
	    stringValueTable[dlChoiceButton->stacking]);
	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%s\"choice button\" {",indent);
	writeDlObject(stream,&(dlChoiceButton->object),level+1);
	writeDlControl(stream,&(dlChoiceButton->control),level+1);
	fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	  stringValueTable[dlChoiceButton->clrmod]);
	fprintf(stream,"\n%s\tstacking=\"%s\"",indent,
	  stringValueTable[dlChoiceButton->stacking]);
	fprintf(stream,"\n%s}",indent);
    }
#endif
}

static void choiceButtonInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlChoiceButton *dlChoiceButton = p->structure.choiceButton;
    medmGetValues(pRCB,
      CTRL_RC,       &(dlChoiceButton->control.ctrl),
      CLR_RC,        &(dlChoiceButton->control.clr),
      BCLR_RC,       &(dlChoiceButton->control.bclr),
      CLRMOD_RC,     &(dlChoiceButton->clrmod),
      STACKING_RC,   &(dlChoiceButton->stacking),
      -1);
}

static void choiceButtonGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlChoiceButton *dlChoiceButton = p->structure.choiceButton;
    medmGetValues(pRCB,
      X_RC,          &(dlChoiceButton->object.x),
      Y_RC,          &(dlChoiceButton->object.y),
      WIDTH_RC,      &(dlChoiceButton->object.width),
      HEIGHT_RC,     &(dlChoiceButton->object.height),
      CTRL_RC,       &(dlChoiceButton->control.ctrl),
      CLR_RC,        &(dlChoiceButton->control.clr),
      BCLR_RC,       &(dlChoiceButton->control.bclr),
      CLRMOD_RC,     &(dlChoiceButton->clrmod),
      STACKING_RC,   &(dlChoiceButton->stacking),
      -1);
}

static void choiceButtonSetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlChoiceButton *dlChoiceButton = p->structure.choiceButton;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlChoiceButton->control.bclr),
      -1);
}

static void choiceButtonSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlChoiceButton *dlChoiceButton = p->structure.choiceButton;
    medmGetValues(pRCB,
      CLR_RC,        &(dlChoiceButton->control.clr),
      -1);
}
