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

static void fallingLineUpdateValueCb(Channel *pCh);
static void fallingLineDestroyCb(Channel *pCh);


static void drawFallingLine(Channel *pCh) {
  unsigned int lineWidth;
  DisplayInfo *displayInfo = pCh->displayInfo;
  Display *display = XtDisplay(pCh->displayInfo->drawingArea);
  DlFallingLine *dlFallingLine = (DlFallingLine *) pCh->specifics;

  lineWidth = (pCh->dlAttr->width+1)/2;
  XDrawLine(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
        dlFallingLine->object.x + lineWidth,
        dlFallingLine->object.y + lineWidth,
        dlFallingLine->object.x + dlFallingLine->object.width - lineWidth,
        dlFallingLine->object.y + dlFallingLine->object.height - lineWidth);
}

void executeDlFallingLine(DisplayInfo *displayInfo, DlFallingLine *dlFallingLine,
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

    pCh->monitorType = DL_FallingLine;
    pCh->specifics = (XtPointer) dlFallingLine;
    pCh->updateChannelCb = fallingLineUpdateValueCb;
    pCh->updateGraphicalInfoCb = NULL;
    pCh->destroyChannel = fallingLineDestroyCb;

    SEVCHK(CA_BUILD_AND_CONNECT(displayInfo->dynamicAttribute.attr.param.chan,TYPENOTCONN,0,
      &(pCh->chid),NULL,medmConnectEventCb, pCh),
      "executeDlFallingLine: error in CA_BUILD_AND_CONNECT");
  } else {
    unsigned int lineWidth = (displayInfo->attribute.width+1)/2;
    XDrawLine(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
        dlFallingLine->object.x + lineWidth,
        dlFallingLine->object.y + lineWidth,
        dlFallingLine->object.x + dlFallingLine->object.width - lineWidth,
        dlFallingLine->object.y + dlFallingLine->object.height - lineWidth);
    XDrawLine(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
        dlFallingLine->object.x + lineWidth,
        dlFallingLine->object.y + lineWidth,
        dlFallingLine->object.x + dlFallingLine->object.width - lineWidth,
        dlFallingLine->object.y + dlFallingLine->object.height - lineWidth);
  }
}

static void fallingLineUpdateValueCb(Channel *pCh) {
  XGCValues gcValues;
  unsigned long gcValueMask;
  Display *display = XtDisplay(pCh->displayInfo->drawingArea);

  if (ca_state(pCh->chid) == cs_conn) {
    gcValueMask = GCForeground|GCBackground|GCLineWidth|GCLineStyle;
    switch (pCh->clrmod) {
      case STATIC :
      case DISCRETE:
        gcValues.foreground = pCh->displayInfo->dlColormap[pCh->dlAttr->clr];
        break;
      case ALARM :
        gcValues.foreground = alarmColorPixel[pCh->severity];
        break;
      default :
        gcValues.foreground = pCh->displayInfo->dlColormap[pCh->dlAttr->clr];
	medmPrintf("internal error : fallingLineUpdatevalueCb : unknown attribute\n");
        break;
    }
    gcValues.background = pCh->displayInfo->dlColormap[pCh->displayInfo->drawingAreaBackgroundColor];
    gcValues.line_width = pCh->dlAttr->width;
    gcValues.line_style = ((pCh->dlAttr->style == SOLID) ? LineSolid : LineOnOffDash);
    XChangeGC(display,pCh->displayInfo->gc,gcValueMask,&gcValues);

    /* correct for lineWidths > 0 */
    switch (pCh->vismod) {
      case V_STATIC:
        drawFallingLine(pCh);
        break;
      case IF_NOT_ZERO:
        if (pCh->value != 0.0)
          drawFallingLine(pCh);
        break;
      case IF_ZERO:
        if (pCh->value == 0.0)
          drawFallingLine(pCh);
        break;
      default :
        medmPrintf("fallingLineUpdateValueCb : internal error\n");
        break;
    }
  } else {
    gcValueMask = GCForeground|GCBackground|GCLineWidth|GCLineStyle;
    gcValues.foreground = WhitePixel(display,DefaultScreen(display));
    gcValues.background = pCh->displayInfo->dlColormap[pCh->displayInfo->drawingAreaBackgroundColor];
    gcValues.line_width = pCh->dlAttr->width;
    gcValues.line_style = ((pCh->dlAttr->style == SOLID) ? LineSolid : LineOnOffDash);
    XChangeGC(display,pCh->displayInfo->gc,gcValueMask,&gcValues);
  }
}

static void fallingLineDestroyCb(Channel *pCh) {
  return;
}
