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

typedef struct _ChoiceButtons {
    DlElement      *dlElement;
    Record         *record;
    UpdateTask     *updateTask;
} ChoiceButtons;

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
    writeDlChoiceButton,
    NULL,
    choiceButtonGetValues,
    choiceButtonInheritValues,
    choiceButtonSetBackgroundColor,
    choiceButtonSetForegroundColor,
    genericMove,
    genericScale,
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
    for (i = MAX_FONTS-1; i >=  0; i--) {
	switch (stacking) {
	case ROW:
	    if ((int)(po->height/useNumButtons - SHADOWS_SIZE) >=
	      (fontTable[i]->ascent + fontTable[i]->descent))
	      return(i);
	    break;
	case ROW_COLUMN:
	    if ((int)(po->height/sqrtNumButtons - SHADOWS_SIZE) >=
	      (fontTable[i]->ascent + fontTable[i]->descent))
	      return(i);
	    break;
	case COLUMN:
	    if ((int)(po->height - SHADOWS_SIZE) >=
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
    for (i = 0; i < numberOfButtons; i++) {
	maxChars = MAX((size_t) maxChars, strlen(labels[i]));
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
    widget = XmCreateRadioBox(parent,"radioBox",wargs,n);
  /* now make push-in type radio buttons of the correct size */
    fontList = fontListTable[choiceButtonFontListIndex(
      po,numberOfButtons,stacking)];
    n = 0;
    XtSetArg(wargs[n],XmNshadowThickness,2); n++;
    XtSetArg(wargs[n],XmNhighlightThickness,1); n++;
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
    for (i = 0; i < numberOfButtons; i++) {
	XmString xmStr;
	xmStr = XmStringCreateLocalized(labels[i]);
	XtSetArg(wargs[n],XmNlabelString,xmStr);
      /* use gadgets here so that changing foreground
	 of radioBox changes buttons */
	buttons[i] = XmCreateToggleButtonGadget(widget,"toggleButton",
	  wargs,n+1);
      /* MDA - for some reason, need to do this
	 after the fact for gadgets...  */
	XtVaSetValues(buttons[i],XmNalignment,XmALIGNMENT_CENTER,NULL);
	XmStringFree(xmStr);
    }
    return widget;
}
void choiceButtonValueChangedCb(
  Widget  w,
  XtPointer clientData,
  XtPointer callbackStruct)
{
    ChoiceButtons *pcb;
    int btnNumber = (int) clientData;
    XmToggleButtonCallbackStruct *call_data = (XmToggleButtonCallbackStruct *) callbackStruct;
    Record *pd;

/*
 * only do ca_put if this widget actually initiated the channel change
 */
    if (call_data->event != NULL && call_data->set == True) {

      /* button's parent (menuPane) has the displayInfo pointer */
	XtVaGetValues(XtParent(w),XmNuserData,&pcb,NULL);
	pd = pcb->record;

	if (pd->connected) {
	    if (pd->writeAccess) {
		medmSendDouble(pcb->record,(double)btnNumber);
	    } else {
		fputc('\a',stderr);
		choiceButtonUpdateValueCb((XtPointer)pcb->record);
	    }
	}
    }
}

static void choiceButtonUpdateGraphicalInfoCb(XtPointer cd) {
    Record *pd = (Record *) cd;
    ChoiceButtons *cb = (ChoiceButtons *) pd->clientData;
    DlElement *dlElement = cb->dlElement;
    DlChoiceButton *pCB = dlElement->structure.choiceButton;
    int i;
    short sqrtEntries;
    double dSqrt;
    XmFontList fontList;
    Pixel fg, bg;
    char *labels[16];
    Widget buttons[16];
    
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! This is a temporary work around !!!!! */
  /* !!!!! for the reconnection.           !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
    medmRecordAddGraphicalInfoCb(cb->record,NULL);

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! End work around                 !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

    if (pd->dataType != DBF_ENUM) {
	medmPostMsg("choiceButtonUpdateGraphicalInfoCb:\n"
	  "  %s is not an ENUM type\n"
	  "  Cannot create Choice Buttom\n",
	  pCB->control.ctrl);
	return;
    }
    for (i = 0; i <= pd->hopr; i++) {
	labels[i] = pd->stateStrings[i];
    }
    fg = (pCB->clrmod == ALARM ? alarmColorPixel[pd->severity] :
      cb->updateTask->displayInfo->colormap[pCB->control.clr]);
    bg = cb->updateTask->displayInfo->colormap[pCB->control.bclr];
    dlElement->widget = createToggleButtons(
      cb->updateTask->displayInfo->drawingArea,
      &(pCB->object),
      fg,
      bg,
      pd->hopr+1,
      labels,
      (XtPointer) cb,
      buttons,
      pCB->stacking);

    for (i = 0; i <= pd->hopr; i++) {
	if (i==(int)pd->value)
	  XmToggleButtonGadgetSetState(buttons[i],True,True);
	XtAddCallback(buttons[i],XmNvalueChangedCallback,
	  (XtCallbackProc)choiceButtonValueChangedCb,(XtPointer)i);
	XtManageChild(buttons[i]);
    }
  /* add in drag/drop translations */
    XtOverrideTranslations(dlElement->widget,parsedTranslations);
    choiceButtonUpdateValueCb(cd);
}

static void choiceButtonUpdateValueCb(XtPointer cd) {
    ChoiceButtons *pcb = (ChoiceButtons *) ((Record *) cd)->clientData;
    updateTaskMarkUpdate(pcb->updateTask);
}

static void choiceButtonDraw(XtPointer cd) {
    ChoiceButtons *pcb = (ChoiceButtons *) cd;
    Record *pd = pcb->record;
    Widget widget = pcb->dlElement->widget;
    DlChoiceButton *dlChoiceButton = pcb->dlElement->structure.choiceButton;
    if (pd->connected) {
	if (pd->readAccess) {
	    if (widget && !XtIsManaged(widget))
	      XtManageChild(widget);
	    if (pd->precision < 0) return;
	    if (pd->dataType == DBF_ENUM) {
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
		  /* set alarm color */
		    XtVaSetValues(widget,XmNforeground,alarmColorPixel[pd->severity],NULL);
		    break;
		default :
		    medmPostMsg("choiceButtonUpdateValueCb:\n");
		    medmPrintf("  Channel Name : %s\n",dlChoiceButton->control.ctrl);
		    medmPrintf("  Message: Unknown color modifier\n");
		    return;
		}
		i = (int) pd->value;
		if ((i >= 0) && (i < (int) numChildren)) {
		    XmToggleButtonGadgetSetState(children[i],True,True);
		} else {
		    medmPostMsg("choiceButtonUpdateValueCb:\n");
		    medmPrintf("  Channel Name: %s\n",dlChoiceButton->control.ctrl);
		    medmPrintf("  Message: Value out of range\n");
		    return;
		}
	    } else {
		medmPostMsg("choiceButtonUpdateValueCb:\n");
		medmPrintf("  Channel Name: %s\n",dlChoiceButton->control.ctrl);
		medmPrintf("  Message: Data type must be enum\n");
		return;
	    }
	    if (pd->writeAccess) 
	      XDefineCursor(XtDisplay(widget),XtWindow(widget),rubberbandCursor);
	    else
	      XDefineCursor(XtDisplay(widget),XtWindow(widget),noWriteAccessCursor);
	} else {
	    if (widget && XtIsManaged(widget)) XtUnmanageChild(widget);
	    draw3DPane(pcb->updateTask,
	      pcb->updateTask->displayInfo->colormap[dlChoiceButton->control.bclr]);
	    draw3DQuestionMark(pcb->updateTask);
	}
    } else {
	if (widget) XtUnmanageChild(widget);
	drawWhiteRectangle(pcb->updateTask);
    }
}

void choiceButtonCreateRunTimeInstance(DisplayInfo *displayInfo,
  DlElement *dlElement) {

    ChoiceButtons *pcb;
    DlChoiceButton *dlChoiceButton = dlElement->structure.choiceButton;
    pcb = (ChoiceButtons *) malloc(sizeof(ChoiceButtons));
    pcb->dlElement = dlElement;
    pcb->updateTask = updateTaskAddTask(displayInfo,
      &(dlChoiceButton->object),
      choiceButtonDraw,
      (XtPointer) pcb);
    if (pcb->updateTask == NULL) {
	medmPrintf("\nchoiceButtonCreateRunTimeInstance: Memory allocation error\n");
    } else {
	updateTaskAddDestroyCb(pcb->updateTask,choiceButtonDestroyCb);
	updateTaskAddNameCb(pcb->updateTask,choiceButtonGetRecord);
    }

    pcb->record = medmAllocateRecord(dlChoiceButton->control.ctrl,
      choiceButtonUpdateValueCb,
      choiceButtonUpdateGraphicalInfoCb,
      (XtPointer) pcb);
  /* put up white rectangle so that unconnected channels are obvious */
    drawWhiteRectangle(pcb->updateTask);

    return;
}

void choiceButtonCreateEditInstance(DisplayInfo *displayInfo, DlElement *dlElement)
{
    Arg args[24];
    int n;
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
      (XtPointer) displayInfo,
      buttons,
      dlChoiceButton->stacking);
    XmToggleButtonGadgetSetState(buttons[0],True,True);
    XtManageChildren(buttons,2);

  /* Remove all translations if in edit mode */
    XtUninstallTranslations(dlElement->widget);
  /* Add back the KeyPress handler invoked in executeDlDisplay */
    XtAddEventHandler(dlElement->widget,KeyPressMask,False,
      handleKeyPress,(XtPointer)displayInfo);
  /* Add the ButtonPress handler */
    XtAddEventHandler(dlElement->widget,ButtonPressMask,False,
      handleButtonPress,(XtPointer)displayInfo);
    
    XtManageChild(dlElement->widget);

    return;
}

void executeDlChoiceButton(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlChoiceButton *dlChoiceButton = dlElement->structure.choiceButton;

    if (displayInfo->traversalMode == DL_EXECUTE) {
	choiceButtonCreateRunTimeInstance(displayInfo, dlElement);
    } else if (displayInfo->traversalMode == DL_EDIT) {
	if (dlElement->widget) {
#if 1
	    XtDestroyWidget(dlElement->widget);
#endif
	    dlElement->widget = 0;
	}
	choiceButtonCreateEditInstance(displayInfo, dlElement);
    }
}

static void choiceButtonDestroyCb(XtPointer cd) {
    ChoiceButtons *pcb = (ChoiceButtons *) cd;
    if (pcb) {
	medmDestroyRecord(pcb->record);
	free((char *)pcb);
    }
}

static void choiceButtonGetRecord(XtPointer cd, Record **record, int *count) {
    ChoiceButtons *pcb = (ChoiceButtons *) cd;
    *count = 1;
    record[0] = pcb->record;
}

DlElement *createDlChoiceButton(DlElement *p)
{
    DlChoiceButton *dlChoiceButton;
    DlElement *dlElement;
 
    dlChoiceButton = (DlChoiceButton *) malloc(sizeof(DlChoiceButton));
    if (!dlChoiceButton) return 0;
    if (p) {
	*dlChoiceButton = *(p->structure.choiceButton);
    } else {
	objectAttributeInit(&(dlChoiceButton->object));
	controlAttributeInit(&(dlChoiceButton->control));
	dlChoiceButton->clrmod = STATIC;
	dlChoiceButton->stacking = ROW;
    }
 
    if (!(dlElement = createDlElement(DL_ChoiceButton,
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
 
    if (!dlElement) return 0;
    dlChoiceButton = dlElement->structure.choiceButton;
 
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlChoiceButton->object));
	    else if (!strcmp(token,"control"))
	      parseControl(displayInfo,&(dlChoiceButton->control));
	    else if (!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"static"))
		  dlChoiceButton->clrmod = STATIC;
		else if (!strcmp(token,"alarm"))
		  dlChoiceButton->clrmod = ALARM;
		else if (!strcmp(token,"discrete"))
		  dlChoiceButton->clrmod = DISCRETE;
	    } else if (!strcmp(token,"stacking")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"row"))
		  dlChoiceButton->stacking = ROW;
		else if (!strcmp(token,"column"))
		  dlChoiceButton->stacking = COLUMN;
		else if (!strcmp(token,"row column"))
		  dlChoiceButton->stacking = ROW_COLUMN;
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
    if (MedmUseNewFileFormat) {
#endif 
	fprintf(stream,"\n%s\"choice button\" {",indent);
	writeDlObject(stream,&(dlChoiceButton->object),level+1);
	writeDlControl(stream,&(dlChoiceButton->control),level+1);
	if (dlChoiceButton->clrmod != STATIC) 
	  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	    stringValueTable[dlChoiceButton->clrmod]);
	if (dlChoiceButton->stacking != ROW)
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
