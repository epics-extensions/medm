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
 * .03  09-11-95        vong    conform to c++ syntax
 *
 *****************************************************************************
*/

#include "medm.h"

typedef struct _ChoiceButtons {
  Widget         widget;
  DlChoiceButton *dlChoiceButton;
  Record         *record;
  UpdateTask     *updateTask;
} ChoiceButtons;

void choiceButtonCreateRunTimeInstance(DisplayInfo *displayInfo,DlChoiceButton *dlChoiceButton);
void choiceButtonCreateEditInstance(DisplayInfo *displayInfo,DlChoiceButton *dlChoiceButton);

static void choiceButtonDraw(XtPointer);
static void choiceButtonUpdateValueCb(XtPointer);
static void choiceButtonUpdateGraphicalInfoCb(XtPointer);
static void choiceButtonDestroyCb(XtPointer cd);
static void choiceButtonName(XtPointer, char **, short *, int *);


#ifdef __cplusplus
int choiceButtonFontListIndex(
  DlChoiceButton *dlChoiceButton,
  int numButtons,
  int)
#else
int choiceButtonFontListIndex(
  DlChoiceButton *dlChoiceButton,
  int numButtons,
  int maxChars)
#endif
{
  int i, useNumButtons;
  short sqrtNumButtons;

#define SHADOWS_SIZE 4		/* each Toggle Button has 2 shadows...*/

/* pay cost of sqrt() and ceil() once */
  sqrtNumButtons = (short) ceil(sqrt((double)numButtons));
  sqrtNumButtons = MAX(1,sqrtNumButtons);
  useNumButtons = MAX(1,numButtons);


/* more complicated calculation based on orientation, etc */
  for (i = MAX_FONTS-1; i >=  0; i--) {
    switch (dlChoiceButton->stacking) {
	case ROW:
	   if ( (int)(dlChoiceButton->object.height/useNumButtons 
					- SHADOWS_SIZE) >=
			(fontTable[i]->ascent + fontTable[i]->descent))
			return(i);
	   break;
	case ROW_COLUMN:
	   if ( (int)(dlChoiceButton->object.height/sqrtNumButtons
					- SHADOWS_SIZE) >=
			(fontTable[i]->ascent + fontTable[i]->descent))
			return(i);
	   break;
	case COLUMN:
	   if ( (int)(dlChoiceButton->object.height - SHADOWS_SIZE) >=
			(fontTable[i]->ascent + fontTable[i]->descent))
			return(i);
	   break;
    }
  }
  return (0);
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
  DlChoiceButton *pCB = cb->dlChoiceButton;
  Arg wargs[20];
  int i, n, maxChars, usedWidth, usedHeight;
  short sqrtEntries;
  double dSqrt;
  XmFontList fontList;
  Pixel fg, bg;

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! This is a temperory work around !!!!! */
  /* !!!!! for the reconnection.           !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  medmRecordAddGraphicalInfoCb(cb->record,NULL);

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! End work around                 !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  if (pd->dataType != DBF_ENUM) {
    medmPrintf("choiceButtonUpdateGraphicalInfoCb :\n    %s\n    \"%s\" %s\n\n",
                "Cannot create Choice Button,",
		pCB->control.ctrl,"is not an ENUM type!");
    medmPostTime();
    return;
  }
  maxChars = 0;
  for (i = 0; i <= pd->hopr; i++) {
    maxChars = MAX((size_t) maxChars,strlen(pd->stateStrings[i]));
  }

  fg = (pCB->clrmod == ALARM ? alarmColorPixel[pd->severity] :
         cb->updateTask->displayInfo->dlColormap[pCB->control.clr]);
  bg = cb->updateTask->displayInfo->dlColormap[pCB->control.bclr];
  n = 0;
  XtSetArg(wargs[n],XmNx,(Position)pCB->object.x); n++;
  XtSetArg(wargs[n],XmNy,(Position)pCB->object.y); n++;
  XtSetArg(wargs[n],XmNwidth,(Dimension)pCB->object.width); n++;
  XtSetArg(wargs[n],XmNheight,(Dimension)pCB->object.height); n++;
  XtSetArg(wargs[n],XmNforeground,fg); n++;
  XtSetArg(wargs[n],XmNbackground,bg); n++;
  XtSetArg(wargs[n],XmNindicatorOn,(Boolean)FALSE); n++;
  XtSetArg(wargs[n],XmNmarginWidth,0); n++;
  XtSetArg(wargs[n],XmNmarginHeight,0); n++;
  XtSetArg(wargs[n],XmNresizeWidth,(Boolean)FALSE); n++;
  XtSetArg(wargs[n],XmNresizeHeight,(Boolean)FALSE); n++;
  XtSetArg(wargs[n],XmNspacing,0); n++;
  XtSetArg(wargs[n],XmNrecomputeSize,(Boolean)FALSE); n++;
  XtSetArg(wargs[n],XmNuserData,cb); n++;
  switch (pCB->stacking) {
  case ROW:
    XtSetArg(wargs[n],XmNorientation,XmVERTICAL); n++;
    usedWidth = pCB->object.width;
    usedHeight = (int) (pCB->object.height/MAX(1,pd->hopr+1));
    break;
  case COLUMN:
    XtSetArg(wargs[n],XmNorientation,XmHORIZONTAL); n++;
    usedWidth = (int) (pCB->object.width/MAX(1,pd->hopr+1));
    usedHeight = pCB->object.height;
    break;
  case ROW_COLUMN:
    XtSetArg(wargs[n],XmNorientation,XmVERTICAL); n++;
    dSqrt = ceil(sqrt((double)pd->hopr+1));
    sqrtEntries = MAX(2,(short)dSqrt);
    XtSetArg(wargs[n],XmNnumColumns,sqrtEntries); n++;
    usedWidth = pCB->object.width/sqrtEntries;
    usedHeight = pCB->object.height/sqrtEntries;
    break;
  default:
    medmPrintf(
      "choiceButtonUpdateGraphicalInfoCb:\n    Unknown stacking mode  = %d",pCB->stacking);
    medmPostTime();
    break;
  }
  cb->widget = XmCreateRadioBox(cb->updateTask->displayInfo->drawingArea,
			"radioBox",wargs,n);
  cb->updateTask->displayInfo->child[cb->updateTask->displayInfo->childCount++] = cb->widget;

  /* now make push-in type radio buttons of the correct size */
  fontList = fontListTable[choiceButtonFontListIndex(
			pCB,(int)pd->hopr+1,maxChars)];
  n = 0;
  XtSetArg(wargs[n],XmNindicatorOn,False); n++;
  XtSetArg(wargs[n],XmNshadowThickness,2); n++;
  XtSetArg(wargs[n],XmNhighlightThickness,1); n++;
  XtSetArg(wargs[n],XmNrecomputeSize,(Boolean)FALSE); n++;
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
  XtSetArg(wargs[n],XmNalignment,XmALIGNMENT_CENTER); n++;
  for (i = 0; i <= pd->hopr; i++) {
    XmString xmStr;
    Widget   toggleButton;
    xmStr = XmStringCreateSimple(pd->stateStrings[i]);
    XtSetArg(wargs[n],XmNlabelString,xmStr);
    /* use gadgets here so that changing foreground of radioBox changes buttons */
    toggleButton = XmCreateToggleButtonGadget(cb->widget,"toggleButton",
					wargs,n+1);
    if (i==(int)pd->value)
      XmToggleButtonGadgetSetState(toggleButton,True,True);
    XtAddCallback(toggleButton,XmNvalueChangedCallback,
	(XtCallbackProc)choiceButtonValueChangedCb,(XtPointer)i);

    /* MDA - for some reason, need to do this after the fact for gadgets... */
    XtVaSetValues(toggleButton,XmNalignment,XmALIGNMENT_CENTER,NULL);

    XtManageChild(toggleButton);
  }
  /* add in drag/drop translations */
  XtOverrideTranslations(cb->widget,parsedTranslations);
  choiceButtonUpdateValueCb(cd);
}

static void choiceButtonUpdateValueCb(XtPointer cd) {
  ChoiceButtons *pcb = (ChoiceButtons *) ((Record *) cd)->clientData;
  updateTaskMarkUpdate(pcb->updateTask);
}

static void choiceButtonDraw(XtPointer cd) {
  ChoiceButtons *pcb = (ChoiceButtons *) cd;
  Record *pd = pcb->record;
  if (pd->connected) {
    if (pd->readAccess) {
      if (pcb->widget && !XtIsManaged(pcb->widget))
        XtManageChild(pcb->widget);
      if (pd->precision < 0) return;
      if (pd->dataType == DBF_ENUM) {
        WidgetList children;
        Cardinal numChildren;
        int i;
        XtVaGetValues(pcb->widget,
           XmNchildren,&children,
  	   XmNnumChildren,&numChildren,
	   NULL);
        /* Change the color */
        switch (pcb->dlChoiceButton->clrmod) {
          case STATIC :
          case DISCRETE :
  	    break;
          case ALARM :
	    /* set alarm color */
	    XtVaSetValues(pcb->widget,XmNforeground,alarmColorPixel[pd->severity], NULL);
	    break;
          default :
	    medmPrintf("Message: Unknown color modifier!\n");
	    medmPrintf("Channel Name : %s\n",pcb->dlChoiceButton->control.ctrl);
	    medmPostMsg("Error: choiceButtonUpdateValueCb\n");
	    return;
        }
        i = (int) pd->value;
        if ((i >= 0) && (i < (int) numChildren)) {
          XmToggleButtonGadgetSetState(children[i],True,True);
        } else {
          medmPrintf("Message: Value out of range!\n");
          medmPrintf("Channel Name : %s\n",pcb->dlChoiceButton->control.ctrl);
          medmPostMsg("Error: choiceButtonUpdateValueCb\n");
	  return;
        }
      } else {
        medmPrintf("Message: Data type must be enum!\n");
        medmPrintf("Channel Name : %s\n",pcb->dlChoiceButton->control.ctrl);
        medmPostMsg("Error: choiceButtonUpdateValueCb\n");
	return;
      }
      if (pd->writeAccess) 
	XDefineCursor(XtDisplay(pcb->widget),XtWindow(pcb->widget),rubberbandCursor);
      else
	XDefineCursor(XtDisplay(pcb->widget),XtWindow(pcb->widget),noWriteAccessCursor);
    } else {
      if (pcb->widget && XtIsManaged(pcb->widget)) XtUnmanageChild(pcb->widget);
      draw3DPane(pcb->updateTask,
         pcb->updateTask->displayInfo->dlColormap[pcb->dlChoiceButton->control.bclr]);
      draw3DQuestionMark(pcb->updateTask);
    }
  } else {
    if (pcb->widget) XtUnmanageChild(pcb->widget);
    drawWhiteRectangle(pcb->updateTask);
  }
}

void choiceButtonCreateRunTimeInstance(DisplayInfo *displayInfo,
		DlChoiceButton *dlChoiceButton) {

  ChoiceButtons *pcb;
  pcb = (ChoiceButtons *) malloc(sizeof(ChoiceButtons));
  pcb->dlChoiceButton = dlChoiceButton;
  pcb->updateTask = updateTaskAddTask(displayInfo,
                              &(dlChoiceButton->object),
                              choiceButtonDraw,
                              (XtPointer) pcb);
  if (pcb->updateTask == NULL) {
    medmPrintf("choiceButtonCreateRunTimeInstance : memory allocation error\n");
  } else {
    updateTaskAddDestroyCb(pcb->updateTask,choiceButtonDestroyCb);
    updateTaskAddNameCb(pcb->updateTask,choiceButtonName);
  }

  pcb->record = medmAllocateRecord(dlChoiceButton->control.ctrl,
              choiceButtonUpdateValueCb,
              choiceButtonUpdateGraphicalInfoCb,
              (XtPointer) pcb);
  pcb->widget = NULL;
  /* put up white rectangle so that unconnected channels are obvious */
  drawWhiteRectangle(pcb->updateTask);

  return;
}

void choiceButtonCreateEditInstance(DisplayInfo *displayInfo,
		DlChoiceButton *dlChoiceButton) {

  Arg args[24];
  int i, n;
  Widget localWidget;
  XmString buttons[2];
  XmButtonType buttonType[2];
  WidgetList children;
  Cardinal numChildren;
  int usedWidth, usedHeight;
  XmFontList fontList;

  buttons[0] = XmStringCreateSimple("0...");
  buttons[1] = XmStringCreateSimple("1...");
  buttonType[0] = XmRADIOBUTTON;
  buttonType[1] = XmRADIOBUTTON;
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlChoiceButton->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlChoiceButton->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlChoiceButton->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlChoiceButton->object.height); n++;
  XtSetArg(args[n],XmNforeground,(Pixel)
	displayInfo->dlColormap[dlChoiceButton->control.clr]); n++;
  XtSetArg(args[n],XmNbackground,(Pixel)
	displayInfo->dlColormap[dlChoiceButton->control.bclr]); n++;
  XtSetArg(args[n],XmNindicatorOn,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNmarginWidth,0); n++;
  XtSetArg(args[n],XmNmarginHeight,0); n++;
  XtSetArg(args[n],XmNrecomputeSize,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNresizeWidth,FALSE); n++;
  XtSetArg(args[n],XmNresizeHeight,FALSE); n++;
  XtSetArg(args[n],XmNspacing,0); n++;
  XtSetArg(args[n],XmNbuttonCount,2); n++;
  XtSetArg(args[n],XmNbuttonType,buttonType); n++;
  XtSetArg(args[n],XmNbuttons,buttons); n++;
  XtSetArg(args[n],XmNbuttonSet,0); n++;
  switch (dlChoiceButton->stacking) {
  case COLUMN:
    usedWidth = dlChoiceButton->object.width/2;
    usedHeight = dlChoiceButton->object.height;
    XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
    break;
  case ROW:
    usedWidth = dlChoiceButton->object.width;
    usedHeight = dlChoiceButton->object.height/2;
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    break;

  case ROW_COLUMN:
    usedWidth = dlChoiceButton->object.width/2;
    usedHeight = dlChoiceButton->object.height/2;
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNnumColumns,2); n++;
    break;
  default:
    medmPrintf(
      "\nexecuteDlChoiceButton: unknown stacking = %d",dlChoiceButton->stacking);
    break;
  }

/*** (MDA)  ChoiceButton is really a radio box and push button children ***/
  localWidget = XmCreateSimpleRadioBox(displayInfo->drawingArea,
		"radioBox", args, n);
/* remove all translations if in edit mode */
  XtUninstallTranslations(localWidget);

/* now make push-in type radio box...*/
  XtVaGetValues(localWidget,XmNchildren,&children,XmNnumChildren,&numChildren,
		NULL);

  fontList = fontListTable[choiceButtonFontListIndex(dlChoiceButton,2,4)];
  for (i = 0; i < numChildren; i++) {
    XtUninstallTranslations(children[i]);
    XtVaSetValues(children[i],XmNindicatorOn,False,XmNshadowThickness,2,
		XmNhighlightThickness,1,XmNfontList, fontList,
		XmNwidth, (Dimension)usedWidth,
		XmNheight, (Dimension)usedHeight,
		XmNrecomputeSize, (Boolean)FALSE,
		NULL);
  }

  XtManageChild(localWidget);
  displayInfo->child[displayInfo->childCount++] = localWidget;

  XmStringFree(buttons[0]);
  XmStringFree(buttons[1]);

  /* add button press handlers for editing */
  XtAddEventHandler(localWidget, ButtonPressMask, False,
	handleButtonPress,displayInfo);
  return;
}

#ifdef __cplusplus
void executeDlChoiceButton(DisplayInfo *displayInfo,
		DlChoiceButton *dlChoiceButton, Boolean)
#else
void executeDlChoiceButton(DisplayInfo *displayInfo,
                DlChoiceButton *dlChoiceButton, Boolean dummy)
#endif
{

 displayInfo->useDynamicAttribute = FALSE;

 if (displayInfo->traversalMode == DL_EXECUTE) {
    choiceButtonCreateRunTimeInstance(displayInfo, dlChoiceButton);
  } else if (displayInfo->traversalMode == DL_EDIT) {
    choiceButtonCreateEditInstance(displayInfo, dlChoiceButton);
  }
}

static void choiceButtonDestroyCb(XtPointer cd) {
  ChoiceButtons *pcb = (ChoiceButtons *) cd;
  if (pcb) {
    medmDestroyRecord(pcb->record);
    free((char *)pcb);
  }
}

static void choiceButtonName(XtPointer cd, char **name, short *severity, int *count) {
  ChoiceButtons *pcb = (ChoiceButtons *) cd;
  *count = 1;
  name[0] = pcb->record->name;
  severity[0] = pcb->record->severity;
}

