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

static void risingLineUpdateStateCb(Channel *pCh);
static void risingLineUpdateValueCb(Channel *pCh);
static void risingLineDestroyCb(Channel *pCh);


static void drawRisingLine(Channel *pCh) {
  unsigned int lineWidth;
  DisplayInfo *displayInfo = pCh->displayInfo;
  Display *display = XtDisplay(pCh->displayInfo->drawingArea);
  DlRisingLine *dlRisingLine = (DlRisingLine *) pCh->specifics;

  lineWidth = (displayInfo->attribute.width+1)/2;
  XDrawLine(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
        dlRisingLine->object.x + lineWidth,
        dlRisingLine->object.y + dlRisingLine->object.height
                - displayInfo->attribute.width,
        dlRisingLine->object.x + dlRisingLine->object.width
                - displayInfo->attribute.width,
        dlRisingLine->object.y - lineWidth);
  XDrawLine(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
        dlRisingLine->object.x + lineWidth,
        dlRisingLine->object.y + dlRisingLine->object.height
                - displayInfo->attribute.width,
        dlRisingLine->object.x + dlRisingLine->object.width
                - displayInfo->attribute.width,
        dlRisingLine->object.y - lineWidth);

}

void executeDlRisingLine(DisplayInfo *displayInfo, DlRisingLine *dlRisingLine,
                                Boolean forcedDisplayToWindow)
{
  if ((displayInfo->traversalMode == DL_EXECUTE) 
      && (displayInfo->useDynamicAttribute != FALSE)){

    Channel *pCh = allocateChannel(displayInfo);
    if (pCh == NULL) return;

    pCh->dlAttr = (DlAttribute *) malloc(sizeof(DlAttribute));
    if (pCh->dlAttr == NULL) return;
    *pCh->dlAttr =displayInfo->attribute;

    pCh->clrmod = displayInfo->dynamicAttribute.attr.mod.clr;
    pCh->vismod = displayInfo->dynamicAttribute.attr.mod.vis;

    pCh->monitorType = DL_RisingLine;
    pCh->specifics = (XtPointer) dlRisingLine;
    pCh->updateChannelCb = risingLineUpdateValueCb;
    pCh->updateGraphicalInfoCb = NULL;
    pCh->destroyChannel = risingLineDestroyCb;

    SEVCHK(CA_BUILD_AND_CONNECT(displayInfo->dynamicAttribute.attr.param.chan,TYPENOTCONN,0,
      &(pCh->chid),NULL,medmConnectEventCb, pCh),
      "executeDlRisingLine: error in CA_BUILD_AND_CONNECT");
  } else {
    unsigned int lineWidth = (displayInfo->attribute.width+1)/2;
    XDrawLine(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
        dlRisingLine->object.x + lineWidth,
        dlRisingLine->object.y + dlRisingLine->object.height
                - displayInfo->attribute.width,
        dlRisingLine->object.x + dlRisingLine->object.width
                - displayInfo->attribute.width,
        dlRisingLine->object.y - lineWidth);
    XDrawLine(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
        dlRisingLine->object.x + lineWidth,
        dlRisingLine->object.y + dlRisingLine->object.height
                - displayInfo->attribute.width,
        dlRisingLine->object.x + dlRisingLine->object.width
                - displayInfo->attribute.width,
        dlRisingLine->object.y - lineWidth);
  }
}

static void risingLineUpdateStateCb(Channel *pCh) {
  if (ca_state(pCh->chid) == cs_conn) {
    if (ca_read_access(pCh->chid)) {
      risingLineUpdateValueCb(pCh);
    } else {
      risingLineUpdateValueCb(pCh);
      draw3DQuestionMark(pCh);
    }
  } else {
    risingLineUpdateValueCb(pCh);
  }
}

static void risingLineUpdateValueCb(Channel *pCh) {
  XGCValues gcValues;
  unsigned long gcValueMask;
  Display *display = XtDisplay(pCh->displayInfo->drawingArea);

  switch (pCh->clrmod) {
  case STATIC :
  case DISCRETE:
    gcValueMask = GCForeground|GCBackground|GCLineWidth|GCLineStyle;
    if (ca_state(pCh->chid) == cs_conn) {
      gcValues.foreground = pCh->displayInfo->dlColormap[pCh->dlAttr->clr];
    } else {
      gcValues.foreground = WhitePixel(display,DefaultScreen(display));
    }
    gcValues.background = 
      pCh->displayInfo->dlColormap[pCh->displayInfo->drawingAreaBackgroundColor];
    gcValues.line_width = pCh->dlAttr->width;
    gcValues.line_style = ((pCh->dlAttr->style == SOLID) ? LineSolid : LineOnOffDash);
    XChangeGC(display,pCh->displayInfo->gc,gcValueMask,&gcValues);
    break;
  case ALARM :
    gcValueMask = GCForeground|GCBackground|GCLineWidth;
    if (ca_state(pCh->chid) == cs_conn) {
      gcValues.foreground = alarmColorPixel[pCh->severity];
    } else {
      gcValues.foreground = WhitePixel(display,DefaultScreen(display));
    }
    gcValues.background = pCh->displayInfo->dlColormap[
            pCh->displayInfo->drawingAreaBackgroundColor];
    gcValues.line_width = pCh->dlAttr->width;
    gcValues.line_style = ( (pCh->dlAttr->style == SOLID)
                            ? LineSolid : LineOnOffDash);
    XChangeGC(display,pCh->displayInfo->gc,
            gcValueMask,&gcValues);
    break;
  }

  /* correct for lineWidths > 0 */
  switch (pCh->vismod) {
  case V_STATIC:
    drawRisingLine(pCh);
    break;
  case IF_NOT_ZERO:
    if (pCh->value != 0.0)
      drawRisingLine(pCh);
    break;
  case IF_ZERO:
    if (pCh->value == 0.0)
      drawRisingLine(pCh);
    break;
  default :
    medmPrintf("internal error : risingLineUpdateValueCb\n");
    break;
  }

}

static void risingLineDestroyCb(Channel *pCh) {
  return;
}
