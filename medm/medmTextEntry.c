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

void textEntryCreateRunTimeInstance(DisplayInfo *displayInfo,DlTextEntry *dlTextEntry);
void textEntryCreateEditInstance(DisplayInfo *displayInfo,DlTextEntry *dlTextEntry);

static void textEntryUpdateValueCb(Channel *pCh);
static void textEntryDestroyCb(Channel *pCh);
static void textEntryValueChanged(Widget, XtPointer, XtPointer);
static void textEntryModifyVerifyCallback(Widget, XtPointer, XtPointer);


int textFieldFontListIndex(int height)
{
  int i, index;
/* don't allow height of font to exceed 90% - 4 pixels of textField widget
 *	(includes nominal 2*shadowThickness=2 shadow)
 */
  for (i = MAX_FONTS-1; i >=  0; i--) {
    if ( ((int)(.90*height) - 4) >=
			(fontTable[i]->ascent + fontTable[i]->descent))
	return(i);
  }
  return (0);
}

/***
 *** Text Entry
 ***/
void textEntryCreateRunTimeInstance(DisplayInfo *displayInfo,
				    DlTextEntry *dlTextEntry) {
  Channel *pCh;
  Arg args[20];
  int n;

  pCh = allocateChannel(displayInfo);

  pCh->monitorType = DL_TextEntry;
  pCh->specifics=(XtPointer)dlTextEntry;
  pCh->clrmod = dlTextEntry->clrmod;
  pCh->backgroundColor = displayInfo->dlColormap[dlTextEntry->control.bclr];
  pCh->updateList = NULL;

  /* setup the callback rountine */
  pCh->updateChannelCb = textEntryUpdateValueCb;
  pCh->updateGraphicalInfoCb = NULL;
  pCh->destroyChannel = textEntryDestroyCb;

  /* put up white rectangle so that unconnected channels are obvious */
  drawWhiteRectangle(pCh);


  SEVCHK(CA_BUILD_AND_CONNECT(dlTextEntry->control.ctrl,TYPENOTCONN,0,
			  &(pCh->chid),NULL,medmConnectEventCb,pCh),
			  "executeDlTextEntry: error in CA_BUILD_AND_CONNECT for Monitor");

/* from the text entry structure, we've got TextEntry's specifics */
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlTextEntry->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlTextEntry->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlTextEntry->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlTextEntry->object.height); n++;
  XtSetArg(args[n],XmNforeground,(Pixel)
	displayInfo->dlColormap[dlTextEntry->control.clr]); n++;
  XtSetArg(args[n],XmNbackground,(Pixel)
	displayInfo->dlColormap[dlTextEntry->control.bclr]); n++;
  XtSetArg(args[n],XmNhighlightThickness,1); n++;
  XtSetArg(args[n],XmNhighlightOnEnter,TRUE); n++;
  XtSetArg(args[n],XmNindicatorOn,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNresizeWidth,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNmarginWidth,
	( (dlTextEntry->object.height <= 2*GOOD_MARGIN_DIVISOR)
	?  0 : (dlTextEntry->object.height/GOOD_MARGIN_DIVISOR)) ); n++;
  XtSetArg(args[n],XmNmarginHeight,
	( (dlTextEntry->object.height <= 2*GOOD_MARGIN_DIVISOR) 
	?  0 : (dlTextEntry->object.height/GOOD_MARGIN_DIVISOR)) ); n++;
  XtSetArg(args[n],XmNfontList,fontListTable[
	textFieldFontListIndex(dlTextEntry->object.height)]); n++;
  pCh->self = XtCreateWidget("textField",
		xmTextFieldWidgetClass, displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] = pCh->self;

  /* add in drag/drop translations */
  XtOverrideTranslations(pCh->self,parsedTranslations);

  /* add the callbacks for update */
  XtAddCallback(pCh->self,XmNactivateCallback,
	  (XtCallbackProc)textEntryValueChanged, (XtPointer)pCh);

  /* special stuff: if user started entering new data into text field, but
	*  doesn't do the actual Activate <CR>, then restore old value on
	*  losing focus...
	*/
  XtAddCallback(pCh->self,XmNmodifyVerifyCallback,
	 (XtCallbackProc)textEntryModifyVerifyCallback,(XtPointer)pCh);
}

void textEntryCreateEditInstance(DisplayInfo *displayInfo,
				DlTextEntry *dlTextEntry) {
  Arg args[20];
  int n;
  Widget localWidget;

  /* from the text entry structure, we've got TextEntry's specifics */
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlTextEntry->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlTextEntry->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlTextEntry->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlTextEntry->object.height); n++;
  XtSetArg(args[n],XmNforeground,(Pixel)
	displayInfo->dlColormap[dlTextEntry->control.clr]); n++;
  XtSetArg(args[n],XmNbackground,(Pixel)
	displayInfo->dlColormap[dlTextEntry->control.bclr]); n++;
  XtSetArg(args[n],XmNhighlightThickness,1); n++;
  XtSetArg(args[n],XmNhighlightOnEnter,TRUE); n++;
  XtSetArg(args[n],XmNindicatorOn,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNresizeWidth,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNmarginWidth,
	( (dlTextEntry->object.height <= 2*GOOD_MARGIN_DIVISOR)
	?  0 : (dlTextEntry->object.height/GOOD_MARGIN_DIVISOR)) ); n++;
  XtSetArg(args[n],XmNmarginHeight,
	( (dlTextEntry->object.height <= 2*GOOD_MARGIN_DIVISOR)
	?  0 : (dlTextEntry->object.height/GOOD_MARGIN_DIVISOR)) ); n++;
  XtSetArg(args[n],XmNfontList,fontListTable[
	textFieldFontListIndex(dlTextEntry->object.height)]); n++;
  localWidget = XtCreateWidget("textField",
		xmTextFieldWidgetClass, displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] =  localWidget;

  /* remove all translations if in edit mode */
  XtUninstallTranslations(localWidget);
  /*
	* add button press handlers too
	*/
  XtAddEventHandler(localWidget, ButtonPressMask, False,
	  (XtEventHandler)handleButtonPress,(XtPointer)displayInfo);

  XtManageChild(localWidget);
}

void executeDlTextEntry(DisplayInfo *displayInfo, DlTextEntry *dlTextEntry,
				Boolean dummy)
{
  displayInfo->useDynamicAttribute = FALSE;
  
  if (displayInfo->traversalMode == DL_EXECUTE) {
	 textEntryCreateRunTimeInstance(displayInfo,dlTextEntry);
  } else
  if (displayInfo->traversalMode == DL_EDIT) {
	 textEntryCreateEditInstance(displayInfo,dlTextEntry);
  }
}

void textEntryUpdateValueCb(Channel *pCh) {
  if (ca_state(pCh->chid) == cs_conn) {
    if (ca_read_access(pCh->chid)) {
      if (pCh->self) 
	XtManageChild(pCh->self);
      else
        return;
      if (ca_write_access(pCh->chid)) {
	XtVaSetValues(pCh->self,XmNeditable,True,NULL);
	XDefineCursor(XtDisplay(pCh->self),XtWindow(pCh->self),rubberbandCursor);
      } else {
	XtVaSetValues(pCh->self,XmNeditable,False,NULL);
	XDefineCursor(XtDisplay(pCh->self),XtWindow(pCh->self),noWriteAccessCursor);
        pCh->updateAllowed = True;
      }
      if (pCh->updateAllowed) {
        XmTextFieldSetString(pCh->self,valueToString(pCh,
		       ((DlTextEntry *) pCh->specifics)->format));
        switch (pCh->clrmod) {
          case STATIC :
          case DISCRETE:
            break;
          case ALARM:
            XtVaSetValues(pCh->self,XmNforeground,alarmColorPixel[pCh->severity],NULL);
            break;
        }
      }
    } else {
      draw3DPane(pCh);
      draw3DQuestionMark(pCh);
      if (pCh->self) XtUnmanageChild(pCh->self);
    }
  } else {
    if (pCh->self) XtUnmanageChild(pCh->self);
    drawWhiteRectangle(pCh);
  }
}

void textEntryDestroyCb(Channel *pCh) {
  return;
}

/*
 * TextEntry special handling:  if user starts editing text field,
 *  then be sure to update value on losingFocus (since until activate,
 *  the value isn't ca_put()-ed, and the text field can be inconsistent
 *  with the underlying channel
 */
static XtCallbackProc textEntryLosingFocusCallback(
  Widget w,
  Channel *pCh,
  XmTextVerifyCallbackStruct *call_data)
{
  XtRemoveCallback(w,XmNlosingFocusCallback,
        (XtCallbackProc)textEntryLosingFocusCallback,pCh);
  pCh->updateAllowed = True;
  textEntryUpdateValueCb(pCh);
}


void textEntryModifyVerifyCallback(
  Widget w,
  XtPointer clientData,
  XtPointer pCallbackData)
{
  Channel *pCh = (Channel *) clientData;
  XmTextVerifyCallbackStruct *pcbs = (XmTextVerifyCallbackStruct *) pCallbackData;

  /* NULL event means value changed programmatically; hence don't process */
  if (pcbs->event != NULL) {
    switch (XtHasCallbacks(w,XmNlosingFocusCallback)) {
      case XtCallbackNoList:
      case XtCallbackHasNone:
        XtAddCallback(w,XmNlosingFocusCallback,
                (XtCallbackProc)textEntryLosingFocusCallback,pCh);
        pCh->updateAllowed = False; 
        break;
      case XtCallbackHasSome:
        break;
    }
    pcbs->doit = True;
  }

}

void textEntryValueChanged(Widget  w, XtPointer clientData, XtPointer dummy)
{
  char *textValue;
  double value;
  Channel *pCh = (Channel *) clientData;

  if (pCh == NULL) return;
  if (pCh->chid == NULL) return;

  if ((ca_state(pCh->chid) == cs_conn) && ca_write_access(pCh->chid)) {

    globalModifiedFlag = True;

    textValue = XmTextFieldGetString(w);
    switch (ca_field_type(pCh->chid)) {
      case DBF_STRING:
        if (strlen(textValue) >= MAX_STRING_SIZE) 
          textValue[MAX_STRING_SIZE-1] = '\0';
        SEVCHK(ca_put(DBR_STRING,pCh->chid,textValue),"textEntryValueChanged: error in ca_put");
        break;
      default:
        value = (double) atof(textValue);
        SEVCHK(ca_put(DBR_DOUBLE,pCh->chid,&value),
          "textEntryValueChanged: error in ca_put");
        break;
    }
    XtFree(textValue);
    ca_flush_io();
  }
}
