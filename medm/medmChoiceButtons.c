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
 *
 *****************************************************************************
*/

#include "medm.h"

void choiceButtonCreateRunTimeInstance(DisplayInfo *displayInfo,DlChoiceButton *dlChoiceButton);
void choiceButtonCreateEditInstance(DisplayInfo *displayInfo,DlChoiceButton *dlChoiceButton);

static void choiceButtonUpdateValueCb(Channel *pCh);
static void choiceButtonUpdateGraphicalInfoCb(Channel *pCh);
static void choiceButtonDestroyCb(Channel *pCh);


int choiceButtonFontListIndex(
  DlChoiceButton *dlChoiceButton,
  int numButtons,
  int maxChars)
{
  int i, index, useNumButtons;
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
  Channel *pCh;
  short btnNumber = (short) clientData;
  XmToggleButtonCallbackStruct *call_data = (XmToggleButtonCallbackStruct *) callbackStruct;

/*
 * only do ca_put if this widget actually initiated the channel change
 */
  if (call_data->event != NULL && call_data->set == True) {

    /* button's parent (menuPane) has the displayInfo pointer */
    XtVaGetValues(XtParent(w),XmNuserData,&pCh,NULL);

    if (pCh == NULL) return;

    if (ca_state(pCh->chid) == cs_conn) {
      if (ca_write_access(pCh->chid)) {
        SEVCHK(ca_put(DBR_SHORT,pCh->chid,&(btnNumber)),
	  "choiceButtonValueChangedCb: error in ca_put");
        ca_flush_io();
      } else {
        fputc('\a',stderr);
	choiceButtonUpdateValueCb(pCh);
      }
    }
  }
}

static void choiceButtonUpdateGraphicalInfoCb(Channel *pCh) {
  
  DlChoiceButton *pCB = (DlChoiceButton *) pCh->specifics;
  Arg wargs[20];
  int i, n, maxChars, usedWidth, usedHeight;
  short sqrtEntries;
  double dSqrt;
  XmFontList fontList;

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! This is a temperory work around !!!!! */
  /* !!!!! for the reconnection.           !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  pCh->updateGraphicalInfoCb = NULL;

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! End work around                 !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  if (ca_field_type(pCh->chid) != DBF_ENUM) {
    medmPrintf("choiceButtonUpdateGraphicalInfoCb :\n    %s\n    \"%s\" %s\n\n",
                "Cannot create Choice Button,",
		ca_name(pCh->chid),"is not an ENUM type!");
    medmPostTime();
    return;
  }
  maxChars = 0;
  for (i = 0; i < pCh->info->e.no_str; i++) {
    maxChars = MAX(maxChars,strlen(pCh->info->e.strs[i]));
  }
	
  n = 0;
  XtSetArg(wargs[n],XmNx,(Position)pCB->object.x); n++;
  XtSetArg(wargs[n],XmNy,(Position)pCB->object.y); n++;
  XtSetArg(wargs[n],XmNwidth,(Dimension)pCB->object.width); n++;
  XtSetArg(wargs[n],XmNheight,(Dimension)pCB->object.height); n++;
  XtSetArg(wargs[n],XmNforeground,(Pixel)
	(pCh->clrmod == ALARM ? alarmColorPixel[pCh->info->e.severity] :
	 pCh->displayInfo->dlColormap[pCB->control.clr])); n++;
  XtSetArg(wargs[n],XmNbackground,(Pixel)
	pCh->displayInfo->dlColormap[pCB->control.bclr]); n++;
  XtSetArg(wargs[n],XmNindicatorOn,(Boolean)FALSE); n++;
  XtSetArg(wargs[n],XmNmarginWidth,0); n++;
  XtSetArg(wargs[n],XmNmarginHeight,0); n++;
  XtSetArg(wargs[n],XmNresizeWidth,(Boolean)FALSE); n++;
  XtSetArg(wargs[n],XmNresizeHeight,(Boolean)FALSE); n++;
  XtSetArg(wargs[n],XmNspacing,0); n++;
  XtSetArg(wargs[n],XmNrecomputeSize,(Boolean)FALSE); n++;
  XtSetArg(wargs[n],XmNuserData,pCh); n++;
  switch (pCB->stacking) {
  case ROW:
    XtSetArg(wargs[n],XmNorientation,XmVERTICAL); n++;
    usedWidth = pCB->object.width;
    usedHeight = pCB->object.height/MAX(1,pCh->info->e.no_str);
    break;
  case COLUMN:
    XtSetArg(wargs[n],XmNorientation,XmHORIZONTAL); n++;
    usedWidth = pCB->object.width/MAX(1,pCh->info->e.no_str);
    usedHeight = pCB->object.height;
    break;
  case ROW_COLUMN:
    XtSetArg(wargs[n],XmNorientation,XmVERTICAL); n++;
    dSqrt = ceil(sqrt((double)pCh->info->e.no_str));
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
  pCh->self = XmCreateRadioBox(pCh->displayInfo->drawingArea,
			"radioBox",wargs,n);
  pCh->displayInfo->child[pCh->displayInfo->childCount++] = pCh->self;

  /* now make push-in type radio buttons of the correct size */
  fontList = fontListTable[choiceButtonFontListIndex(
			pCB,pCh->info->e.no_str,maxChars)];
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
  XtSetArg(wargs[n],XmNforeground,(Pixel)
	(pCh->clrmod == ALARM ? alarmColorPixel[pCh->info->e.severity] :
	 pCh->displayInfo->dlColormap[pCB->control.clr])); n++;
  XtSetArg(wargs[n],XmNbackground,(Pixel)
	    pCh->displayInfo->dlColormap[pCB->control.bclr]); n++;
  XtSetArg(wargs[n],XmNalignment,XmALIGNMENT_CENTER); n++;
  for (i = 0; i < pCh->info->e.no_str; i++) {
    XmString xmStr;
    Widget   toggleButton;
    xmStr = XmStringCreateSimple(pCh->info->e.strs[i]);
    XtSetArg(wargs[n],XmNlabelString,xmStr);
    /* use gadgets here so that changing foreground of radioBox changes buttons */
    toggleButton = XmCreateToggleButtonGadget(pCh->self,"toggleButton",
					wargs,n+1);
    if (i==(int)pCh->value)
      XmToggleButtonGadgetSetState(toggleButton,True,True);
    XtAddCallback(toggleButton,XmNvalueChangedCallback,
	(XtCallbackProc)choiceButtonValueChangedCb,(XtPointer)i);

    /* MDA - for some reason, need to do this after the fact for gadgets... */
    XtVaSetValues(toggleButton,XmNalignment,XmALIGNMENT_CENTER,NULL);

    XtManageChild(toggleButton);
  }
  /* add in drag/drop translations */
  XtOverrideTranslations(pCh->self,parsedTranslations);
  choiceButtonUpdateValueCb(pCh);
}

static void choiceButtonUpdateValueCb(Channel *pCh) {
  if (ca_state(pCh->chid) == cs_conn) {
    if (ca_read_access(pCh->chid)) {
      if (pCh->self)
        XtManageChild(pCh->self);
      else
        return;
      if (ca_field_type(pCh->chid) == DBF_ENUM) {
        WidgetList children;
        Cardinal numChildren;
        int i;
        XtVaGetValues(pCh->self,
           XmNchildren,&children,
  	   XmNnumChildren,&numChildren,
	   NULL);
        /* Change the color */
        switch (pCh->clrmod) {
          case STATIC :
          case DISCRETE :
  	    break;
          case ALARM :
	    /* set alarm color */
	    XtVaSetValues(pCh->self,XmNforeground,alarmColorPixel[pCh->severity], NULL);
	    break;
          default :
	    medmPrintf("Message: Unknown color modifier!\n");
	    medmPrintf("Channel Name : %s\n",ca_name(pCh->chid));
	    medmPostMsg("Error: choiceButtonUpdateValueCb\n");
	    return;
        }
        i = (int) pCh->value;
        if ((i >= 0) && (i < (int) numChildren)) {
          XmToggleButtonGadgetSetState(children[i],True,True);
        } else {
          medmPrintf("Message: Value out of range!\n");
          medmPrintf("Channel Name : %s\n",ca_name(pCh->chid));
          medmPostMsg("Error: choiceButtonUpdateValueCb\n");
	  return;
        }
      } else {
        medmPrintf("Message: Data type must be enum!\n");
        medmPrintf("Channel Name : %s\n",ca_name(pCh->chid));
        medmPostMsg("Error: choiceButtonUpdateValueCb\n");
	return;
      }
      if (ca_write_access(pCh->chid)) 
	XDefineCursor(XtDisplay(pCh->self),XtWindow(pCh->self),rubberbandCursor);
      else
	XDefineCursor(XtDisplay(pCh->self),XtWindow(pCh->self),noWriteAccessCursor);
    } else {
      if (pCh->self) XtUnmanageChild(pCh->self);
      draw3DPane(pCh);
      draw3DQuestionMark(pCh);
    }
  } else {
    if (pCh->self) XtUnmanageChild(pCh->self);
    drawWhiteRectangle(pCh);
  }
}

void choiceButtonCreateRunTimeInstance(DisplayInfo *displayInfo,
		DlChoiceButton *dlChoiceButton) {

  Channel *pCh;

  /* also add in as monitor */
  pCh = allocateChannel(displayInfo);

  pCh->monitorType = DL_ChoiceButton;
  pCh->specifics = (XtPointer) dlChoiceButton;
  pCh->clrmod = dlChoiceButton->clrmod;
  pCh->backgroundColor = displayInfo->dlColormap[dlChoiceButton->control.bclr];
  pCh->updateList = NULL;

  /* setup the callback rountine */
  pCh->updateChannelCb = choiceButtonUpdateValueCb;
  pCh->updateGraphicalInfoCb = choiceButtonUpdateGraphicalInfoCb;
  pCh->destroyChannel = choiceButtonDestroyCb;

  /* put up white rectangle so that unconnected channels are obvious */
  drawWhiteRectangle(pCh);

  SEVCHK(CA_BUILD_AND_CONNECT(dlChoiceButton->control.ctrl,TYPENOTCONN,0,
	&(pCh->chid), NULL,medmConnectEventCb, pCh),
	"executeDlChoiceButton: error in CA_BUILD_AND_CONNECT for Monitor");
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
  short sqrtNumButtons;

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

void executeDlChoiceButton(DisplayInfo *displayInfo,
		DlChoiceButton *dlChoiceButton, Boolean dummy)
{

 displayInfo->useDynamicAttribute = FALSE;

 if (displayInfo->traversalMode == DL_EXECUTE) {
    choiceButtonCreateRunTimeInstance(displayInfo, dlChoiceButton);
  } else if (displayInfo->traversalMode == DL_EDIT) {
    choiceButtonCreateEditInstance(displayInfo, dlChoiceButton);
  }
}

static void choiceButtonDestroyCb(Channel *pCh) {
}

