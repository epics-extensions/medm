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

void messageButtonCreateRunTimeInstance(DisplayInfo *displayInfo,DlMessageButton *dlMessageButton);
void messageButtonCreateEditInstance(DisplayInfo *displayInfo,DlMessageButton *dlMessageButton);

static void messageButtonUpdateValueCb(Channel *pCh);
static void messageButtonUpdateGraphicalInfoCb(Channel *pCh);
static void messageButtonDestroyCb(Channel *pCh);
static void messageButtonValueChangedCb(Widget, XtPointer, XtPointer);


int messageButtonFontListIndex(int height)
{
  int i, index;
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
  Channel *pCh;
  XmString xmString;
  int n;
  Arg args[20];

  pCh = allocateChannel(displayInfo);
  pCh->monitorType = DL_MessageButton;
  pCh->specifics = (XtPointer) dlMessageButton;
  pCh->clrmod = dlMessageButton->clrmod;
  pCh->backgroundColor = displayInfo->dlColormap[dlMessageButton->control.bclr];
  pCh->updateList = NULL;

  /* setup the callback rountine */
  pCh->updateChannelCb = messageButtonUpdateValueCb;     
  pCh->updateGraphicalInfoCb =  messageButtonUpdateGraphicalInfoCb;
  pCh->destroyChannel = messageButtonDestroyCb;

  /* put up white rectangle so that unconnected channels are obvious */
  drawWhiteRectangle(pCh);

  SEVCHK(CA_BUILD_AND_CONNECT(dlMessageButton->control.ctrl,TYPENOTCONN,0,
	&(pCh->chid),NULL,medmConnectEventCb,pCh),
	"executeDlButton: error in CA_BUILD_AND_CONNECT for Monitor");

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
  pCh->self = XtCreateWidget("messageButton",
		xmPushButtonWidgetClass, displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] = pCh->self;
  XmStringFree(xmString);

  /* add in drag/drop translations */
  XtOverrideTranslations(pCh->self,parsedTranslations);

  /* add the callbacks for update */
  XtAddCallback(pCh->self,XmNarmCallback,messageButtonValueChangedCb,
	(XtPointer)pCh);
  XtAddCallback(pCh->self,XmNdisarmCallback,messageButtonValueChangedCb,
	(XtPointer)pCh);
}

void executeDlMessageButton(DisplayInfo *displayInfo,
		DlMessageButton *dlMessageButton, Boolean dummy)
{

  displayInfo->useDynamicAttribute = FALSE;
  
  if (displayInfo->traversalMode == DL_EXECUTE) {
	 messageButtonCreateRunTimeInstance(displayInfo,dlMessageButton);
  } else
  if (displayInfo->traversalMode == DL_EDIT) {
	 messageButtonCreateEditInstance(displayInfo,dlMessageButton);
  }
}

static void messageButtonUpdateGraphicalInfoCb(Channel *pCh) {
  DlMessageButton *dlMessageButton = (DlMessageButton *) pCh->specifics;
  int i,j;
  Boolean match;

  switch (ca_field_type(pCh->chid)) {
    case DBF_STRING:
      break;
    case DBF_ENUM :
      if (dlMessageButton->press_msg[0] != '\0') {
        match = False;
        for (i = 0; i < pCh->info->e.no_str; i++) {
          if (pCh->info->e.strs[i]) {
            if (!strcmp(dlMessageButton->press_msg,pCh->info->e.strs[i])) {
              pCh->pressValue = (double) i;
              match = True;
              break;
            }
	  }
        }
        if (match == False) {
          pCh->pressValue = (double) atof(dlMessageButton->press_msg);
        }
      }
      if (dlMessageButton->release_msg[0] != '\0') {
        match = False;
        for (i = 0; i < pCh->info->e.no_str; i++) {
          if (pCh->info->e.strs[i]) {
            if (!strcmp(dlMessageButton->release_msg,pCh->info->e.strs[i])) {
              pCh->releaseValue = (double) i;
              match = True;
              break;
            }
          }
	}
        if (match == False) {
           pCh->releaseValue = (double) atof(dlMessageButton->release_msg);
        }
      }
      break;
    default:
      if (dlMessageButton->press_msg[0] != '\0')
        pCh->pressValue = (double) atof(dlMessageButton->press_msg);
      if (dlMessageButton->release_msg[0] != '\0')
        pCh->releaseValue = (double) atof(dlMessageButton->release_msg);
      break;
  }
}


static void messageButtonUpdateValueCb(Channel *pCh) {
  if (ca_state(pCh->chid) == cs_conn) {
    if (ca_read_access(pCh->chid)) {
      if (pCh->self)
	XtManageChild(pCh->self);
      else 
        return;
      switch (pCh->clrmod) {
        case STATIC :
        case DISCRETE :
          break;
        case ALARM :
          XtVaSetValues(pCh->self,XmNforeground,alarmColorPixel[pCh->severity],NULL);
          break;
        default :
          break;
      }
      if (ca_write_access(pCh->chid))
	XDefineCursor(XtDisplay(pCh->self),XtWindow(pCh->self),rubberbandCursor);
      else
	XDefineCursor(XtDisplay(pCh->self),XtWindow(pCh->self),noWriteAccessCursor);
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

static void messageButtonDestroyCb(Channel *pCh) {
  return;
}

static void messageButtonValueChangedCb(Widget w,
                XtPointer clientData,
                XtPointer callbackData) {

  Channel *pCh = (Channel *) clientData;
  XmPushButtonCallbackStruct *pushCallData = (XmPushButtonCallbackStruct *) callbackData;
  DlMessageButton *dlMessageButton;

  if (pCh == NULL) return;
  if (pCh->chid == NULL) return;
  if (pushCallData == NULL) return;

  if (ca_state(pCh->chid) == cs_conn) {
    dlMessageButton = (DlMessageButton *) pCh->specifics;
    if (ca_write_access(pCh->chid)) {
      if (pushCallData->reason == XmCR_ARM) {
        /* message button can only put strings */
        if (dlMessageButton->press_msg[0] != '\0') {
          switch (ca_field_type(pCh->chid)) {
            case DBF_STRING:
              SEVCHK(ca_put(DBR_STRING,pCh->chid,dlMessageButton->press_msg),
                  "messageButtonValueChangedCb : error in ca_put");
              break;
            default:
              SEVCHK(ca_put(DBR_DOUBLE,pCh->chid,&pCh->pressValue),
                  "messageButtonValueChangedCb : error in ca_put");
              break;
          }
        }
      } else
      if (pushCallData->reason == XmCR_DISARM) {
        if (dlMessageButton->release_msg[0] != '\0') {
          switch (ca_field_type(pCh->chid)) {
            case DBF_STRING:
              SEVCHK(ca_put(DBR_STRING,pCh->chid,dlMessageButton->release_msg),
                  "messageButtonValueChangedCb : error in ca_put");
              break;
            default:
              SEVCHK(ca_put(DBR_DOUBLE,pCh->chid,&pCh->releaseValue),
                  "messageButtonValueChangedCb : error in ca_put");
              break;
          }
        }
      }
    } else {
      fputc((int)'\a',stderr);
    }
  }
}
