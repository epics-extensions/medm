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
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (708-252-2000).
*/
/*****************************************************************************
 *
 *     Original Author : Mark Andersion
 *     Current Author  : Frederick Vong
 *
 * Modification Log:
 * -----------------
 * .01  03-01-95        vong    2.0.0 release
 * .02  09-05-95        vong    2.1.0 release
 *                              - using new screen update dispatch mechanism
 * .03  09-12-95        vong    conform to c++ syntax
 *
 *****************************************************************************
*/

#include "medm.h"

typedef struct _MessageButton {
  Widget             widget;
  DlMessageButton    *dlMessageButton;
  Record             *record;
  UpdateTask         *updateTask;
  double             pressValue;
  double             releaseValue;
} MessageButton;

void messageButtonCreateRunTimeInstance(DisplayInfo *displayInfo,DlMessageButton *dlMessageButton);
void messageButtonCreateEditInstance(DisplayInfo *displayInfo,DlMessageButton *dlMessageButton);

static void messageButtonDraw(XtPointer cd);
static void messageButtonUpdateValueCb(XtPointer cd);
static void messageButtonUpdateGraphicalInfoCb(XtPointer cd);
static void messageButtonDestroyCb(XtPointer);
static void messageButtonValueChangedCb(Widget, XtPointer, XtPointer);
static void messageButtonName(XtPointer, char **, short *, int *);
static void messageButtonInheritValues(ResourceBundle *pRCB, DlElement *p);


int messageButtonFontListIndex(int height)
{
  int i;
/* don't allow height of font to exceed 90% - 4 pixels of messageButton widget
 *	(includes nominal 2*shadowThickness=2 shadow)
 */
  for (i = MAX_FONTS-1; i >=  0; i--) {
    if ( ((int)(.90*height) - 4) >= 
			(fontTable[i]->ascent + fontTable[i]->descent))
	return(i);
  }
  return (0);
}

void messageButtonCreateEditInstance(DisplayInfo *displayInfo,
		DlMessageButton *dlMessageButton) {
  XmString xmString;
  Widget widget;
  int n;
  Arg args[20];

  xmString = XmStringCreateSimple(dlMessageButton->label);
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlMessageButton->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlMessageButton->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlMessageButton->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlMessageButton->object.height); n++;
  XtSetArg(args[n],XmNforeground,(Pixel)
	displayInfo->dlColormap[dlMessageButton->control.clr]); n++;
  XtSetArg(args[n],XmNbackground,(Pixel)
	displayInfo->dlColormap[dlMessageButton->control.bclr]); n++;
  XtSetArg(args[n],XmNhighlightThickness,1); n++;
  XtSetArg(args[n],XmNhighlightOnEnter,TRUE); n++;
  XtSetArg(args[n],XmNindicatorOn,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNrecomputeSize,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNlabelString, xmString); n++;
  XtSetArg(args[n],XmNlabelType, XmSTRING); n++;
  XtSetArg(args[n],XmNfontList,fontListTable[
	messageButtonFontListIndex(dlMessageButton->object.height)]); n++;
  widget = XtCreateWidget("messageButton",
		xmPushButtonWidgetClass, displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] = widget;

  XmStringFree(xmString);

  /* remove all translations if in edit mode */
  XtUninstallTranslations(widget);
  /*
	* add button press handlers too
	*/
  XtAddEventHandler(widget,ButtonPressMask, False,
	 handleButtonPress,(XtPointer)displayInfo);
  XtManageChild(widget);
}

void messageButtonCreateRunTimeInstance(DisplayInfo *displayInfo,
		DlMessageButton *dlMessageButton) {
  MessageButton *pmb;
  XmString xmString;
  int n;
  Arg args[20];

  pmb = (MessageButton *) malloc(sizeof(MessageButton));
  pmb->dlMessageButton = dlMessageButton;

  pmb->updateTask = updateTaskAddTask(displayInfo,
                                     &(dlMessageButton->object),
                                     messageButtonDraw,
                                     (XtPointer)pmb);

  if (pmb->updateTask == NULL) {
    medmPrintf("messageButtonCreateRunTimeInstance : memory allocation error\n");
  } else {
    updateTaskAddDestroyCb(pmb->updateTask,messageButtonDestroyCb);
    updateTaskAddNameCb(pmb->updateTask,messageButtonName);
  }
  pmb->record = medmAllocateRecord(dlMessageButton->control.ctrl,
                  messageButtonUpdateValueCb,
                  messageButtonUpdateGraphicalInfoCb,
                  (XtPointer) pmb);
  drawWhiteRectangle(pmb->updateTask);
 
  xmString = XmStringCreateSimple(dlMessageButton->label);
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlMessageButton->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlMessageButton->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlMessageButton->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlMessageButton->object.height); n++;
  XtSetArg(args[n],XmNforeground,(Pixel)
	displayInfo->dlColormap[dlMessageButton->control.clr]); n++;
  XtSetArg(args[n],XmNbackground,(Pixel)
	displayInfo->dlColormap[dlMessageButton->control.bclr]); n++;
  XtSetArg(args[n],XmNhighlightThickness,1); n++;
  XtSetArg(args[n],XmNhighlightOnEnter,TRUE); n++;
  XtSetArg(args[n],XmNindicatorOn,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNrecomputeSize,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNlabelString, xmString); n++;
  XtSetArg(args[n],XmNlabelType, XmSTRING); n++;
  XtSetArg(args[n],XmNfontList,fontListTable[
	 messageButtonFontListIndex(dlMessageButton->object.height)]); n++;
  pmb->widget = XtCreateWidget("messageButton",
		xmPushButtonWidgetClass, displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] = pmb->widget;
  XmStringFree(xmString);

  /* add in drag/drop translations */
  XtOverrideTranslations(pmb->widget,parsedTranslations);

  /* add the callbacks for update */
  XtAddCallback(pmb->widget,XmNarmCallback,messageButtonValueChangedCb,
	(XtPointer)pmb);
  XtAddCallback(pmb->widget,XmNdisarmCallback,messageButtonValueChangedCb,
	(XtPointer)pmb);
}

#ifdef __cplusplus
void executeDlMessageButton(DisplayInfo *displayInfo,
		DlMessageButton *dlMessageButton, Boolean)
#else
void executeDlMessageButton(DisplayInfo *displayInfo,
                DlMessageButton *dlMessageButton, Boolean dummy)
#endif
{

  if (displayInfo->traversalMode == DL_EXECUTE) {
	 messageButtonCreateRunTimeInstance(displayInfo,dlMessageButton);
  } else
  if (displayInfo->traversalMode == DL_EDIT) {
	 messageButtonCreateEditInstance(displayInfo,dlMessageButton);
  }
}

static void messageButtonUpdateGraphicalInfoCb(XtPointer cd) {
  Record *pd = (Record *) cd;
  MessageButton *pmb = (MessageButton *) pd->clientData;
  DlMessageButton *dlMessageButton = pmb->dlMessageButton;
  int i;
  Boolean match;

  switch (pd->dataType) {
    case DBF_STRING:
      break;
    case DBF_ENUM :
      if (dlMessageButton->press_msg[0] != '\0') {
        match = False;
        for (i = 0; i < pd->hopr+1; i++) {
          if (pd->stateStrings[i]) {
            if (!strcmp(dlMessageButton->press_msg,pd->stateStrings[i])) {
              pmb->pressValue = (double) i;
              match = True;
              break;
            }
	  }
        }
        if (match == False) {
          pmb->pressValue = (double) atof(dlMessageButton->press_msg);
        }
      }
      if (dlMessageButton->release_msg[0] != '\0') {
        match = False;
        for (i = 0; i < pd->hopr+1; i++) {
          if (pd->stateStrings[i]) {
            if (!strcmp(dlMessageButton->release_msg,pd->stateStrings[i])) {
              pmb->releaseValue = (double) i;
              match = True;
              break;
            }
          }
	}
        if (match == False) {
           pmb->releaseValue = (double) atof(dlMessageButton->release_msg);
        }
      }
      break;
    default:
      if (dlMessageButton->press_msg[0] != '\0')
        pmb->pressValue = (double) atof(dlMessageButton->press_msg);
      if (dlMessageButton->release_msg[0] != '\0')
        pmb->releaseValue = (double) atof(dlMessageButton->release_msg);
      break;
  }
}


static void messageButtonUpdateValueCb(XtPointer cd) {
  MessageButton *pmb = (MessageButton *) ((Record *) cd)->clientData;
  updateTaskMarkUpdate(pmb->updateTask);
}

static void messageButtonDraw(XtPointer cd) {
  MessageButton *pmb = (MessageButton *) cd;
  Record *pd = pmb->record;
  if (pd->connected) {
    if (pd->readAccess) {
      if (pmb->widget)
	XtManageChild(pmb->widget);
      else 
        return;
      switch (pmb->dlMessageButton->clrmod) {
        case STATIC :
        case DISCRETE :
          break;
        case ALARM :
          XtVaSetValues(pmb->widget,XmNforeground,alarmColorPixel[pd->severity],NULL);
          break;
        default :
          break;
      }
      if (pd->writeAccess)
	XDefineCursor(XtDisplay(pmb->widget),XtWindow(pmb->widget),rubberbandCursor);
      else
	XDefineCursor(XtDisplay(pmb->widget),XtWindow(pmb->widget),noWriteAccessCursor);
    } else {
      draw3DPane(pmb->updateTask,
         pmb->updateTask->displayInfo->dlColormap[pmb->dlMessageButton->control.bclr]);
      draw3DQuestionMark(pmb->updateTask);
      if (pmb->widget) XtUnmanageChild(pmb->widget);
    }
  } else {
    if (pmb->widget) XtUnmanageChild(pmb->widget);
    drawWhiteRectangle(pmb->updateTask);
  }
}

static void messageButtonDestroyCb(XtPointer cd) {
  MessageButton *pmb = (MessageButton *) cd;
  if (pmb) {
    medmDestroyRecord(pmb->record);
    free((char *)pmb);
  }
}

#ifdef __cplusplus
static void messageButtonValueChangedCb(Widget,
                XtPointer clientData,
                XtPointer callbackData) {
#else
static void messageButtonValueChangedCb(Widget w,
                XtPointer clientData,
                XtPointer callbackData) {
#endif
  MessageButton *pmb = (MessageButton *) clientData;
  Record *pd = pmb->record;
  XmPushButtonCallbackStruct *pushCallData = (XmPushButtonCallbackStruct *) callbackData;
  DlMessageButton *dlMessageButton = pmb->dlMessageButton;

  if (pd->connected) {
    if (pd->writeAccess) {
      if (pushCallData->reason == XmCR_ARM) {
        /* message button can only put strings */
        if (dlMessageButton->press_msg[0] != '\0') {
          switch (pd->dataType) {
            case DBF_STRING:
              medmSendString(pmb->record,dlMessageButton->press_msg);
              break;
            default:
              medmSendDouble(pmb->record,pmb->pressValue);
              break;
          }
        }
      } else
      if (pushCallData->reason == XmCR_DISARM) {
        if (dlMessageButton->release_msg[0] != '\0') {
          switch (pd->dataType) {
            case DBF_STRING:
              medmSendString(pmb->record,dlMessageButton->release_msg);
              break;
            default:
              medmSendDouble(pmb->record,pmb->releaseValue);
              break;
          }
        }
      }
    } else {
      fputc((int)'\a',stderr);
    }
  }
}

static void messageButtonName(XtPointer cd, char **name, short *severity, int *count) {
  MessageButton *pmb = (MessageButton *) cd;
  *count = 1;
  name[0] = pmb->record->name;
  severity[0] = pmb->record->severity;
}

 
 
/***
 *** Message Button
 ***/
 
DlElement *createDlMessageButton(
  DisplayInfo *displayInfo)
{
  DlMessageButton *dlMessageButton;
  DlElement *dlElement;
 
  dlMessageButton = (DlMessageButton *) malloc(sizeof(DlMessageButton));
  objectAttributeInit(&(dlMessageButton->object));
  controlAttributeInit(&(dlMessageButton->control));
  dlMessageButton->label[0] = '\0';
  dlMessageButton->press_msg[0] = '\0';
  dlMessageButton->release_msg[0] = '\0';
  dlMessageButton->clrmod = STATIC;

  if (!(dlElement = createDlElement(DL_MessageButton,
                    (XtPointer)      dlMessageButton,
                    (medmExecProc)   executeDlMessageButton,
                    (medmWriteProc)  writeDlMessageButton,
										0,0,
                    messageButtonInheritValues))) {
    free(dlMessageButton);
  }

  return(dlElement);
}

DlElement *parseMessageButton(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlMessageButton *dlMessageButton;
  DlElement *dlElement = createDlMessageButton(displayInfo);

  if (!dlElement) return 0;
  dlMessageButton = dlElement->structure.messageButton;

  do {
    switch( (tokenType=getToken(displayInfo,token)) ) {
      case T_WORD:
        if (!strcmp(token,"object"))
          parseObject(displayInfo,&(dlMessageButton->object));
        else
        if (!strcmp(token,"control"))
          parseControl(displayInfo,&(dlMessageButton->control));
        else
        if (!strcmp(token,"press_msg")) {
          getToken(displayInfo,token);
          getToken(displayInfo,token);
          strcpy(dlMessageButton->press_msg,token);
        } else
        if (!strcmp(token,"release_msg")) {
          getToken(displayInfo,token);
          getToken(displayInfo,token);
          strcpy(dlMessageButton->release_msg,token);
        } else
        if (!strcmp(token,"label")) {
          getToken(displayInfo,token);
          getToken(displayInfo,token);
          strcpy(dlMessageButton->label,token);
        } else
        if (!strcmp(token,"clrmod")) {
          getToken(displayInfo,token);
          getToken(displayInfo,token);
          if (!strcmp(token,"static"))
            dlMessageButton->clrmod = STATIC;
          else
          if (!strcmp(token,"alarm"))
            dlMessageButton->clrmod = ALARM;
          else
          if (!strcmp(token,"discrete"))
            dlMessageButton->clrmod = DISCRETE;
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

  POSITION_ELEMENT_ON_LIST();

  return dlElement;
}

void writeDlMessageButton(
  FILE *stream,
  DlMessageButton *dlMessageButton,
  int level)
{
  char indent[16];

  memset(indent,'\t',level);
  indent[level] = '\0';
 
#ifdef SUPPORT_0201XX_FILE_FORMAT
  if (MedmUseNewFileFormat) {
#endif
		fprintf(stream,"\n%s\"message button\" {",indent);
		writeDlObject(stream,&(dlMessageButton->object),level+1);
		writeDlControl(stream,&(dlMessageButton->control),level+1);
		if (dlMessageButton->label[0] != '\0')
			fprintf(stream,"\n%s\tlabel=\"%s\"",indent,dlMessageButton->label);
		if (dlMessageButton->press_msg[0] != '\0')
			fprintf(stream,"\n%s\tpress_msg=\"%s\"",indent,dlMessageButton->press_msg);
		if (indent,dlMessageButton->release_msg[0] != '\0')
			fprintf(stream,"\n%s\trelease_msg=\"%s\"",
				indent,dlMessageButton->release_msg);
		if (dlMessageButton->clrmod != STATIC) 
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
