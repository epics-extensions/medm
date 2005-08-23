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

typedef struct _MedmMessageButton {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           *record;
    double           pressValue;
    double           releaseValue;
} MedmMessageButton;

void messageButtonCreateRunTimeInstance(DisplayInfo *, DlElement *);
void messageButtonCreateEditInstance(DisplayInfo *, DlElement *);

static void messageButtonDraw(XtPointer cd);
static void messageButtonUpdateValueCb(XtPointer cd);
static void messageButtonUpdateGraphicalInfoCb(XtPointer cd);
static void messageButtonDestroyCb(XtPointer);
static void messageButtonValueChangedCb(Widget w, XtPointer clientData,
  XtPointer callbackData);
static void messageButtonGetRecord(XtPointer, Record **, int *);
static void messageButtonInheritValues(ResourceBundle *pRCB, DlElement *p);
static void messageButtonSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void messageButtonSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void messageButtonGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable messageButtonDlDispatchTable = {
    createDlMessageButton,
    NULL,
    executeDlMessageButton,
    hideDlMessageButton,
    writeDlMessageButton,
    NULL,
    messageButtonGetValues,
    messageButtonInheritValues,
    messageButtonSetBackgroundColor,
    messageButtonSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};


int messageButtonFontListIndex(int height)
{
    int i;
/* don't allow height of font to exceed 90% - 4 pixels of messageButton widget
 *	(includes nominal 2*shadowThickness=2 shadow)
 */
    for(i = MAX_FONTS-1; i >=  0; i--) {
	if( ((int)(.90*height) - 4) >=
	  (fontTable[i]->ascent + fontTable[i]->descent))
	  return(i);
    }
    return (0);
}

Widget createPushButton(Widget parent,
  DlObject *po,
  Pixel fg,
  Pixel bg,
  Pixmap pixmap,
  char *label,
  XtPointer userData) {
    Arg args[15];
    Widget widget;
    XmString xmString = 0;
    int n = 0;

    XtSetArg(args[n],XmNx,(Position) po->x); n++;
    XtSetArg(args[n],XmNy,(Position) po->y); n++;
    XtSetArg(args[n],XmNwidth,(Dimension) po->width); n++;
    XtSetArg(args[n],XmNheight,(Dimension) po->height); n++;
    XtSetArg(args[n],XmNforeground,fg); n++;
    XtSetArg(args[n],XmNbackground,bg); n++;
    XtSetArg(args[n],XmNhighlightThickness,0); n++;
    XtSetArg(args[n],XmNindicatorOn,False); n++;
    XtSetArg(args[n],XmNrecomputeSize,False); n++;
    XtSetArg(args[n], XmNuserData, userData); n++;
    if(label) {
	xmString = XmStringCreateLocalized(label);
	XtSetArg(args[n],XmNlabelString, xmString); n++;
	XtSetArg(args[n],XmNlabelType, XmSTRING); n++;
	XtSetArg(args[n],XmNfontList,
	  fontListTable[messageButtonFontListIndex(po->height)]); n++;
    } else
      if(pixmap) {
	  XtSetArg(args[n],XmNlabelPixmap,pixmap); n++;
	  XtSetArg(args[n],XmNlabelType,XmPIXMAP); n++;
      }
    widget = XtCreateWidget("messageButton",
      xmPushButtonWidgetClass, parent, args, n);
    if(xmString) XmStringFree(xmString);
    return widget;
}

void messageButtonCreateEditInstance(DisplayInfo *displayInfo,
  DlElement *dlElement) {
    DlMessageButton *dlMessageButton = dlElement->structure.messageButton;

    dlElement->widget = createPushButton(displayInfo->drawingArea,
      &(dlMessageButton->object),
      displayInfo->colormap[dlMessageButton->control.clr],
      displayInfo->colormap[dlMessageButton->control.bclr],
      (Pixmap)0,
      dlMessageButton->label,
      (XtPointer) displayInfo);

  /* Add handlers */
    addCommonHandlers(dlElement->widget, displayInfo);

    XtManageChild(dlElement->widget);
}

void messageButtonCreateRunTimeInstance(DisplayInfo *displayInfo,
  DlElement *dlElement) {
    MedmMessageButton *pmb;
    DlMessageButton *dlMessageButton = dlElement->structure.messageButton;

    if(dlElement->data) {
	pmb = (MedmMessageButton *)dlElement->data;
    } else {
	pmb = (MedmMessageButton *)malloc(sizeof(MedmMessageButton));
	dlElement->data = (void *)pmb;
	if(pmb == NULL) {
	    medmPrintf(1,"\nmessageButtonCreateRunTimeInstance:"
	      " Memory allocation error\n");
	    return;
	}
      /* Pre-initialize */
	pmb->updateTask = NULL;
	pmb->record = NULL;
	pmb->dlElement = dlElement;

	pmb->updateTask = updateTaskAddTask(displayInfo,
	  &(dlMessageButton->object),
	  messageButtonDraw,
	  (XtPointer)pmb);

	if(!pmb->updateTask) {
	    medmPrintf(1,"\nmessageButtonCreateRunTimeInstance: Memory allocation error\n");
	} else {
	    updateTaskAddDestroyCb(pmb->updateTask,messageButtonDestroyCb);
	    updateTaskAddNameCb(pmb->updateTask,messageButtonGetRecord);
	}
	pmb->record = medmAllocateRecord(dlMessageButton->control.ctrl,
	  messageButtonUpdateValueCb,
	  messageButtonUpdateGraphicalInfoCb,
	  (XtPointer) pmb);
	drawWhiteRectangle(pmb->updateTask);
    }

    if(!dlElement->widget) {
	dlElement->widget = createPushButton(displayInfo->drawingArea,
	  &(dlMessageButton->object),
	  displayInfo->colormap[dlMessageButton->control.clr],
	  displayInfo->colormap[dlMessageButton->control.bclr],
	  (Pixmap)0,
	  dlMessageButton->label,
	  (XtPointer) displayInfo);

      /* Add the callbacks for update */
	XtAddCallback(dlElement->widget,XmNarmCallback,messageButtonValueChangedCb,
	  (XtPointer)pmb);
	XtAddCallback(dlElement->widget,XmNdisarmCallback,messageButtonValueChangedCb,
	  (XtPointer)pmb);
    }
}

void executeDlMessageButton(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(displayInfo->traversalMode == DL_EXECUTE) {
	messageButtonCreateRunTimeInstance(displayInfo,dlElement);
    } else if(displayInfo->traversalMode == DL_EDIT) {
	if(dlElement->widget) {
	    DlMessageButton *dlMessageButton = dlElement->structure.messageButton;
	    DlObject *po = &(dlMessageButton->object);
	    XmString xmString;
	    xmString = XmStringCreateLocalized(dlMessageButton->label);
	    XtVaSetValues(dlElement->widget,
	      XmNx, (Position) po->x,
	      XmNy, (Position) po->y,
	      XmNwidth, (Dimension) po->width,
	      XmNheight, (Dimension) po->height,
	      XmNlabelString, xmString,
	      NULL);
	    XmStringFree(xmString);
	} else {
	    messageButtonCreateEditInstance(displayInfo,dlElement);
	}
    }
}

void hideDlMessageButton(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element with a widget */
    hideWidgetElement(displayInfo, dlElement);
}

static void messageButtonUpdateGraphicalInfoCb(XtPointer cd)
{
    Record *pr = (Record *) cd;
    MedmMessageButton *pmb = (MedmMessageButton *) pr->clientData;
    DlMessageButton *dlMessageButton = pmb->dlElement->structure.messageButton;
    int i;
    Boolean match;
    char *end;

#ifdef MEDM_CDEV
    pr->useMsgWhenWrite[0] = False;
    pr->useMsgWhenWrite[1] = False;
#endif

    switch (pr->dataType) {
    case DBF_STRING:
	break;
    case DBF_ENUM :
	if(dlMessageButton->press_msg[0] != '\0') {
	    match = False;
	    for(i = 0; i < pr->hopr+1; i++) {
		if(pr->stateStrings[i]) {
		    if(!strcmp(dlMessageButton->press_msg,pr->stateStrings[i])) {
			pmb->pressValue = (double)i;
			match = True;
			break;
		    }
		}
	    }
	    if(match == False) {
		pmb->pressValue = strtod(dlMessageButton->press_msg,&end);
		if(*end != '\0' || end == dlMessageButton->press_msg) {
		    medmPostMsg(1,"messageButtonUpdateGraphicalInfoCb: "
		      "Invalid press value:\n"
		      "  Name: %s\n"
		      "  Value: \"%s\"\n",
		      pr->name?pr->name:"NULL",
		      dlMessageButton->press_msg);
		}
	    }
	}
	if(dlMessageButton->release_msg[0] != '\0') {
	    match = False;
	    for(i = 0; i < pr->hopr+1; i++) {
		if(pr->stateStrings[i]) {
		    if(!strcmp(dlMessageButton->release_msg,pr->stateStrings[i])) {
			pmb->releaseValue = (double) i;
			match = True;
			break;
		    }
		}
	    }
	    if(match == False) {
		pmb->releaseValue = strtod(dlMessageButton->release_msg,&end);
		if(*end != '\0' || end == dlMessageButton->release_msg) {
		    medmPostMsg(1,"messageButtonUpdateGraphicalInfoCb: "
		      "Invalid release value:\n"
		      "  Name: %s\n"
		      "  Value: \"%s\"\n",
		      pr->name?pr->name:"NULL",
		      dlMessageButton->release_msg);
		}
	    }
	}
	break;
    default:
	if(dlMessageButton->press_msg[0] != '\0') {
	    pmb->pressValue = strtod(dlMessageButton->press_msg,&end);
	    if(*end != '\0' || end == dlMessageButton->press_msg) {
		medmPostMsg(1,"messageButtonUpdateGraphicalInfoCb: "
		  "Invalid press value:\n"
		  "  Name: %s\n"
		  "  Value: \"%s\"\n",
		  pr->name?pr->name:"NULL",
		  dlMessageButton->press_msg);
#ifdef MEDM_CDEV
		pr->useMsgWhenWrite[0] = True;
#endif
	    }
	}
	if(dlMessageButton->release_msg[0] != '\0') {
	    pmb->releaseValue = strtod(dlMessageButton->release_msg,&end);
	    if(*end != '\0' || end == dlMessageButton->release_msg) {
		medmPostMsg(1,"messageButtonUpdateGraphicalInfoCb: "
		  "Invalid release value:\n"
		  "  Name: %s\n"
		  "  Value: \"%s\"\n",
		  pr->name?pr->name:"NULL",
		  dlMessageButton->release_msg);
#ifdef MEDM_CDEV
		pr->useMsgWhenWrite[1] = True;
#endif
	    }
	}
	break;
    }
}

static void messageButtonUpdateValueCb(XtPointer cd)
{
    MedmMessageButton *pmb = (MedmMessageButton *) ((Record *) cd)->clientData;
    updateTaskMarkUpdate(pmb->updateTask);
}

static void messageButtonDraw(XtPointer cd)
{
    MedmMessageButton *pmb = (MedmMessageButton *) cd;
    Record *pr = pmb->record;
    DlElement *dlElement = pmb->dlElement;
    Widget widget = dlElement->widget;
    DlMessageButton *dlMessageButton = dlElement->structure.messageButton;

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
		addCommonHandlers(widget, pmb->updateTask->displayInfo);
		XtManageChild(widget);
	    } else {
		return;
	    }
	    switch (dlMessageButton->clrmod) {
	    case STATIC :
	    case DISCRETE :
		break;
	    case ALARM :
		pr->monitorSeverityChanged = True;
		XtVaSetValues(widget,XmNforeground,alarmColor(pr->severity),NULL);
		break;
	    default :
		break;
	    }
	    if(pr->writeAccess)
	      XDefineCursor(XtDisplay(widget),XtWindow(widget),rubberbandCursor);
	    else
	      XDefineCursor(XtDisplay(widget),XtWindow(widget),noWriteAccessCursor);
	} else {
	    if(widget && XtIsManaged(widget))
	      XtUnmanageChild(widget);
	    drawBlackRectangle(pmb->updateTask);
	}
    } else {
	if(widget && XtIsManaged(widget))
	  XtUnmanageChild(widget);
	drawWhiteRectangle(pmb->updateTask);
    }
}

static void messageButtonDestroyCb(XtPointer cd)
{
    MedmMessageButton *pmb = (MedmMessageButton *) cd;
    if(pmb) {
	medmDestroyRecord(pmb->record);
	if(pmb->dlElement) pmb->dlElement->data = NULL;
	free((char *)pmb);
    }
}

static void messageButtonValueChangedCb(Widget w, XtPointer clientData,
  XtPointer callbackData)
{
    MedmMessageButton *pmb = (MedmMessageButton *) clientData;
    Record *pr = pmb->record;
    XmPushButtonCallbackStruct *pushCallData = (XmPushButtonCallbackStruct *) callbackData;
    DlMessageButton *dlMessageButton = pmb->dlElement->structure.messageButton;

    UNREFERENCED(w);

    if(pr->connected) {
	if(pr->writeAccess) {
	    if(pushCallData->reason == XmCR_ARM) {
	      /* message button can only put strings */
		if(dlMessageButton->press_msg[0] != '\0') {
		    switch (pr->dataType) {
		    case DBF_STRING:
		        medmSendString(pmb->record,dlMessageButton->press_msg);
			break;
#ifdef MEDM_CDEV
		    case DBF_ENUM:
		        medmSendString(pmb->record,
				       pr->stateStrings[(int)pmb->pressValue]);
			break;
#endif
		    default:
#ifdef MEDM_CDEV
		        if(pr->useMsgWhenWrite[0])
			  medmSendMsg (pmb->record, dlMessageButton->press_msg);
			else
			  medmSendDouble(pmb->record,pmb->pressValue);
#else
			medmSendDouble(pmb->record,pmb->pressValue);
#endif
			break;
		    }
		}
	    } else
	      if(pushCallData->reason == XmCR_DISARM) {
		  if(dlMessageButton->release_msg[0] != '\0') {
		      switch (pr->dataType) {
		      case DBF_STRING:
			  medmSendString(pmb->record,dlMessageButton->release_msg);
			  break;
#ifdef MEDM_CDEV
		      case DBF_ENUM:
			  medmSendString(pmb->record,
					 pr->stateStrings[(int)pmb->releaseValue]);
			  break;
#endif
		      default:
#ifdef MEDM_CDEV
		        if(pr->useMsgWhenWrite[1])
			  medmSendMsg (pmb->record, dlMessageButton->release_msg);
			else
			  medmSendDouble(pmb->record,pmb->releaseValue);
#else
			  medmSendDouble(pmb->record,pmb->releaseValue);
#endif
			  break;
		      }
		  }
	      }
	} else {
	    fputc((int)'\a',stderr);
	}
    }
}

static void messageButtonGetRecord(XtPointer cd, Record **record, int *count) {
    MedmMessageButton *pmb = (MedmMessageButton *) cd;
    *count = 1;
    record[0] = pmb->record;
}

/***
 *** Message Button
 ***/

DlElement *createDlMessageButton(DlElement *p)
{
    DlMessageButton *dlMessageButton;
    DlElement *dlElement;

    dlMessageButton = (DlMessageButton *)malloc(sizeof(DlMessageButton));
    if(p) {
	*dlMessageButton = *(p->structure.messageButton);
    } else {
	objectAttributeInit(&(dlMessageButton->object));
	controlAttributeInit(&(dlMessageButton->control));
	dlMessageButton->label[0] = '\0';
	dlMessageButton->press_msg[0] = '\0';
	dlMessageButton->release_msg[0] = '\0';
	dlMessageButton->clrmod = STATIC;
    }

    if(!(dlElement = createDlElement(DL_MessageButton,
      (XtPointer) dlMessageButton,
      &messageButtonDlDispatchTable))) {
	free(dlMessageButton);
    }

    return(dlElement);
}

DlElement *parseMessageButton(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlMessageButton *dlMessageButton;
    DlElement *dlElement = createDlMessageButton(NULL);

    if(!dlElement) return 0;
    dlMessageButton = dlElement->structure.messageButton;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlMessageButton->object));
	    else if(!strcmp(token,"control"))
	      parseControl(displayInfo,&(dlMessageButton->control));
	    else if(!strcmp(token,"press_msg")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlMessageButton->press_msg,token);
	    } else if(!strcmp(token,"release_msg")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlMessageButton->release_msg,token);
	    } else if(!strcmp(token,"label")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlMessageButton->label,token);
	    } else if(!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"static"))
		  dlMessageButton->clrmod = STATIC;
		else if(!strcmp(token,"alarm"))
		  dlMessageButton->clrmod = ALARM;
		else if(!strcmp(token,"discrete"))
		  dlMessageButton->clrmod = DISCRETE;
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

void writeDlMessageButton(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
    char indent[16];
    DlMessageButton *dlMessageButton = dlElement->structure.messageButton;

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
	fprintf(stream,"\n%s\"message button\" {",indent);
	writeDlObject(stream,&(dlMessageButton->object),level+1);
	writeDlControl(stream,&(dlMessageButton->control),level+1);
	if(dlMessageButton->label[0] != '\0')
	  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,dlMessageButton->label);
	if(dlMessageButton->press_msg[0] != '\0')
	  fprintf(stream,"\n%s\tpress_msg=\"%s\"",indent,dlMessageButton->press_msg);
      /* KE: Removed intent, from next statement just after ( */
	if(dlMessageButton->release_msg[0] != '\0')
	  fprintf(stream,"\n%s\trelease_msg=\"%s\"",
	    indent,dlMessageButton->release_msg);
	if(dlMessageButton->clrmod != STATIC)
	  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	    stringValueTable[dlMessageButton->clrmod]);
	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%s\"message button\" {",indent);
	writeDlObject(stream,&(dlMessageButton->object),level+1);
	writeDlControl(stream,&(dlMessageButton->control),level+1);
	fprintf(stream,"\n%s\tlabel=\"%s\"",indent,dlMessageButton->label);
	fprintf(stream,"\n%s\tpress_msg=\"%s\"",indent,dlMessageButton->press_msg);
	fprintf(stream,"\n%s\trelease_msg=\"%s\"",
	  indent,dlMessageButton->release_msg);
	fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	  stringValueTable[dlMessageButton->clrmod]);
	fprintf(stream,"\n%s}",indent);
    }
#endif
}

static void messageButtonInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlMessageButton *dlMessageButton = p->structure.messageButton;
    medmGetValues(pRCB,
      CTRL_RC,       &(dlMessageButton->control.ctrl),
      CLR_RC,        &(dlMessageButton->control.clr),
      BCLR_RC,       &(dlMessageButton->control.bclr),
      CLRMOD_RC,     &(dlMessageButton->clrmod),
      -1);
}

static void messageButtonGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlMessageButton *dlMessageButton = p->structure.messageButton;
    medmGetValues(pRCB,
      X_RC,          &(dlMessageButton->object.x),
      Y_RC,          &(dlMessageButton->object.y),
      WIDTH_RC,      &(dlMessageButton->object.width),
      HEIGHT_RC,     &(dlMessageButton->object.height),
      CTRL_RC,       &(dlMessageButton->control.ctrl),
      CLR_RC,        &(dlMessageButton->control.clr),
      BCLR_RC,       &(dlMessageButton->control.bclr),
      MSG_LABEL_RC,  &(dlMessageButton->label),
      PRESS_MSG_RC,  &(dlMessageButton->press_msg),
      RELEASE_MSG_RC,&(dlMessageButton->release_msg),
      CLRMOD_RC,     &(dlMessageButton->clrmod),
      -1);
}

static void messageButtonSetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlMessageButton *dlMessageButton = p->structure.messageButton;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlMessageButton->control.bclr),
      -1);
}

static void messageButtonSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlMessageButton *dlMessageButton = p->structure.messageButton;
    medmGetValues(pRCB,
      CLR_RC,        &(dlMessageButton->control.clr),
      -1);
}
