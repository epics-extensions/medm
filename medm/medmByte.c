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

static void byteUpdateValueCb(Channel *pCh);
static void byteDestroyCb(Channel *pCh);

void executeDlByte(DisplayInfo *displayInfo, DlByte *dlByte, Boolean dummy) {
/****************************************************************************
 * Execute DL Byte                                                          *
 ****************************************************************************/
  Channel *pCh;
  Arg args[30];
  int n;
  int usedHeight, usedCharWidth, bestSize, preferredHeight;
  Widget localWidget;

  displayInfo->useDynamicAttribute = FALSE;

  if (displayInfo->traversalMode == DL_EXECUTE) {
    pCh = allocateChannel(displayInfo);
    pCh->monitorType = DL_Byte;
    pCh->specifics = (XtPointer) dlByte;
    pCh->clrmod = dlByte->clrmod;
    pCh->backgroundColor = displayInfo->dlColormap[dlByte->monitor.bclr];

    pCh->updateChannelCb = byteUpdateValueCb;
    pCh->updateGraphicalInfoCb = NULL;
    pCh->destroyChannel = byteDestroyCb;

    drawWhiteRectangle(pCh);


    SEVCHK(CA_BUILD_AND_CONNECT(dlByte->monitor.rdbk,TYPENOTCONN,0,
        &(pCh->chid),NULL,medmConnectEventCb,pCh),
        "executeDlByte: error in CA_BUILD_AND_CONNECT");
  }

/****** from the DlByte structure, we've got Byte's specifics */
    n = 0;
    XtSetArg(args[n],XtNx,(Position)dlByte->object.x); n++;
    XtSetArg(args[n],XtNy,(Position)dlByte->object.y); n++;
    XtSetArg(args[n],XtNwidth,(Dimension)dlByte->object.width); n++;
    XtSetArg(args[n],XtNheight,(Dimension)dlByte->object.height); n++;
    XtSetArg(args[n],XcNdataType,XcLval); n++;

/****** note that this is orientation for the Byte */
    if (dlByte->direction == RIGHT) {
      XtSetArg(args[n],XcNorient,XcHoriz); n++;
    }
    else {
      if (dlByte->direction == UP) XtSetArg(args[n],XcNorient,XcVert); n++;
    }
    XtSetArg(args[n],XcNsBit,dlByte->sbit); n++;
    XtSetArg(args[n],XcNeBit,dlByte->ebit); n++;

/****** Set arguments with other colors, etc */
    preferredHeight = dlByte->object.height/INDICATOR_FONT_DIVISOR;
    bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
      preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
    XtSetArg(args[n],XtNfont,fontTable[bestSize]); n++;
    XtSetArg(args[n],XcNbyteForeground,(Pixel)
      displayInfo->dlColormap[dlByte->monitor.clr]); n++;
    XtSetArg(args[n],XcNbyteBackground,(Pixel)
      displayInfo->dlColormap[dlByte->monitor.bclr]); n++;
    XtSetArg(args[n],XtNbackground,(Pixel)
      displayInfo->dlColormap[dlByte->monitor.bclr]); n++;
    XtSetArg(args[n],XcNcontrolBackground,(Pixel)
      displayInfo->dlColormap[dlByte->monitor.bclr]); n++;

/****** Add the pointer to the ChannelAccessMonitorData structure as
        userData to widget */
    XtSetArg(args[n],XcNuserData,(XtPointer)pCh); n++;
    localWidget = XtCreateWidget("byte",
      xcByteWidgetClass, displayInfo->drawingArea, args, n);
    displayInfo->child[displayInfo->childCount++] = localWidget;

/****** Record the widget that this structure belongs to */
    if (displayInfo->traversalMode == DL_EXECUTE) {
      pCh->self = localWidget;
/****** Add in drag/drop translations */
      XtOverrideTranslations(localWidget,parsedTranslations);
    } else if (displayInfo->traversalMode == DL_EDIT) {
/* add button press handlers */
      XtAddEventHandler(localWidget,ButtonPressMask,False,
        handleButtonPress,(XtPointer)displayInfo);
      XtManageChild(localWidget);
    }
}

static void byteUpdateValueCb(Channel *pCh) {
  XcVType val;
  if (ca_state(pCh->chid) == cs_conn) {
    if (ca_read_access(pCh->chid)) {
      if (pCh->self) {
        XtManageChild(pCh->self);
      } else {
	return;
      }
      val.fval = (float) pCh->value;
      XcBYUpdateValue(pCh->self,&val);
      switch (pCh->clrmod) {
	case STATIC :
	case DISCRETE :
	  break;
	case ALARM :
          XcBYUpdateByteForeground(pCh->self,alarmColorPixel[pCh->severity]);
	  break;
      }
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

static void byteDestroyCb(Channel *pCh) {
}

