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

static void textUpdateUpdateValueCb(Channel *pCh);
static void textUpdateDestroyCb(Channel *pCh);

void executeDlTextUpdate(DisplayInfo *displayInfo, DlTextUpdate *dlTextUpdate,
				Boolean dummy)
{
  Channel *pCh;
  XRectangle clipRect[1];
  int usedHeight, usedWidth;
  int localFontIndex;
  size_t nChars;


  displayInfo->useDynamicAttribute = FALSE;

  if (displayInfo->traversalMode == DL_EXECUTE) {
    pCh = allocateChannel(displayInfo);
    pCh->monitorType = DL_TextUpdate;
    pCh->specifics = (XtPointer) dlTextUpdate;
    pCh->clrmod = dlTextUpdate->clrmod;
    pCh->backgroundColor = displayInfo->dlColormap[dlTextUpdate->monitor.bclr];
    pCh->label = LABEL_NONE;

    pCh->updateChannelCb = textUpdateUpdateValueCb;
    pCh->updateGraphicalInfoCb = NULL;
    pCh->destroyChannel = textUpdateDestroyCb;
    pCh->opaque = True;

    drawWhiteRectangle(pCh);

    pCh->fontIndex = dmGetBestFontWithInfo(fontTable,
	MAX_FONTS,DUMMY_TEXT_FIELD,
	dlTextUpdate->object.height, dlTextUpdate->object.width, 
	&usedHeight, &usedWidth, FALSE);	/* don't use width */

    SEVCHK(CA_BUILD_AND_CONNECT(dlTextUpdate->monitor.rdbk,TYPENOTCONN,0,
	&(pCh->chid),NULL,medmConnectEventCb, pCh),
	"executeDlTextUpdate: error in CA_BUILD_AND_CONNECT");

  } else {

  /* since no ca callbacks to put up text, put up dummy region */
    XSetForeground(display,displayInfo->gc,
	displayInfo->dlColormap[ dlTextUpdate->monitor.bclr]);
    XFillRectangle(display, XtWindow(displayInfo->drawingArea),
	displayInfo->gc,
	dlTextUpdate->object.x,dlTextUpdate->object.y,
	dlTextUpdate->object.width, dlTextUpdate->object.height);
    XFillRectangle(display,displayInfo->drawingAreaPixmap,
	displayInfo->gc,
	dlTextUpdate->object.x,dlTextUpdate->object.y,
	dlTextUpdate->object.width, dlTextUpdate->object.height);

    XSetForeground(display,displayInfo->gc,
	displayInfo->dlColormap[dlTextUpdate->monitor.clr]);
    XSetBackground(display,displayInfo->gc,
	displayInfo->dlColormap[dlTextUpdate->monitor.bclr]);
    nChars = strlen(dlTextUpdate->monitor.rdbk);
    localFontIndex = dmGetBestFontWithInfo(fontTable,
	MAX_FONTS,dlTextUpdate->monitor.rdbk,
	dlTextUpdate->object.height, dlTextUpdate->object.width, 
	&usedHeight, &usedWidth, FALSE);	/* don't use width */
    usedWidth = XTextWidth(fontTable[localFontIndex],dlTextUpdate->monitor.rdbk,
		nChars);

/* clip to bounding box (especially for text) */
    clipRect[0].x = dlTextUpdate->object.x;
    clipRect[0].y = dlTextUpdate->object.y;
    clipRect[0].width  = dlTextUpdate->object.width;
    clipRect[0].height =  dlTextUpdate->object.height;
    XSetClipRectangles(display,displayInfo->gc,0,0,clipRect,1,YXBanded);

    XSetFont(display,displayInfo->gc,fontTable[localFontIndex]->fid);
    switch(dlTextUpdate->align) {
      case HORIZ_LEFT:
      case VERT_TOP:
	XDrawString(display,displayInfo->drawingAreaPixmap,
	  displayInfo->gc,
	  dlTextUpdate->object.x,dlTextUpdate->object.y +
			fontTable[localFontIndex]->ascent,
	  dlTextUpdate->monitor.rdbk,strlen(dlTextUpdate->monitor.rdbk));
	XDrawString(display,XtWindow(displayInfo->drawingArea),
	  displayInfo->gc,
	  dlTextUpdate->object.x,dlTextUpdate->object.y +
			fontTable[localFontIndex]->ascent,
	  dlTextUpdate->monitor.rdbk,strlen(dlTextUpdate->monitor.rdbk));
	break;
      case HORIZ_CENTER:
      case VERT_CENTER:
	XDrawString(display,displayInfo->drawingAreaPixmap,
	  displayInfo->gc,
	  dlTextUpdate->object.x + (dlTextUpdate->object.width - usedWidth)/2,
	  dlTextUpdate->object.y + fontTable[localFontIndex]->ascent,
	  dlTextUpdate->monitor.rdbk,strlen(dlTextUpdate->monitor.rdbk));
	XDrawString(display,XtWindow(displayInfo->drawingArea),
	  displayInfo->gc,
	  dlTextUpdate->object.x + (dlTextUpdate->object.width - usedWidth)/2,
	  dlTextUpdate->object.y + fontTable[localFontIndex]->ascent,
	  dlTextUpdate->monitor.rdbk,strlen(dlTextUpdate->monitor.rdbk));
	break;
      case HORIZ_RIGHT:
      case VERT_BOTTOM:
	XDrawString(display,displayInfo->drawingAreaPixmap,
	  displayInfo->gc,
	  dlTextUpdate->object.x + dlTextUpdate->object.width - usedWidth,
	  dlTextUpdate->object.y + fontTable[localFontIndex]->ascent,
	  dlTextUpdate->monitor.rdbk,strlen(dlTextUpdate->monitor.rdbk));
	XDrawString(display,XtWindow(displayInfo->drawingArea),
	  displayInfo->gc,
	  dlTextUpdate->object.x + dlTextUpdate->object.width - usedWidth,
	  dlTextUpdate->object.y + fontTable[localFontIndex]->ascent,
	  dlTextUpdate->monitor.rdbk,strlen(dlTextUpdate->monitor.rdbk));
	break;
    }


/* and turn off clipping on exit */
    XSetClipMask(display,displayInfo->gc,None);
  }
}


static void textUpdateDestroyCb(Channel *pCh) {
  return;
}


static void textUpdateUpdateValueCb(Channel *pCh) {
  DlTextUpdate *pTU = (DlTextUpdate *) pCh->specifics;
  DisplayInfo *pDI = pCh->displayInfo;
  Display *display = XtDisplay(pDI->drawingArea);
  char textField[MAX_TEXT_UPDATE_WIDTH];
  int i;
  XRectangle clipRect[1];
  XGCValues gcValues;
  unsigned long gcValueMask;
  int usedWidth, usedHeight;
  Boolean isNumber;
  double value = 0.0;
  unsigned short precision = 0;

  if (ca_state(pCh->chid) == cs_conn) {
    if (ca_read_access(pCh->chid)) {
      switch (ca_field_type(pCh->chid)) {
        case DBF_STRING :
          strncpy(textField,pCh->stringValue, MAX_TEXT_UPDATE_WIDTH-1);
          textField[MAX_TEXT_UPDATE_WIDTH-1] = '\0';
          isNumber = False;
          break;
        case DBF_ENUM :
          if ((pCh->info) && (pCh->info->e.no_str > 0)) {
            i = (int) pCh->value;
            if (i >= 0 && i < pCh->info->e.no_str) {
              strcpy(textField,pCh->info->e.strs[i]);
            } else {
              textField[0] = ' '; textField[1] = '\0';
            }
            isNumber = False;
          } else {
            value = (double) pCh->value;
	    precision = 0;
            isNumber = True;
          }
          break;
        case DBF_CHAR :
        case DBF_INT :
        case DBF_LONG :
        case DBF_FLOAT :
        case DBF_DOUBLE :
          value = pCh->value;
          precision = pCh->precision;
          isNumber = True;
          break;
        default :
          medmPrintf("textUpdateUpdateValueCb: %s %s %s\n",
	    "unknown channel type for",ca_name(pCh->chid), ": cannot attach Meter");
          medmPostTime();
          break;
      }
      if (isNumber) {
        switch (pTU->format) {
          case DECIMAL:
            localCvtDoubleToString(value,textField,precision);
            break;
          case EXPONENTIAL:
            cvtDoubleToExpString(value,textField,precision);
            break;
          case ENGR_NOTATION:
            localCvtDoubleToExpNotationString(value,textField,precision);
            break;
          case COMPACT:
            cvtDoubleToCompactString(value,textField,precision);
            break;
          case TRUNCATED:
            cvtLongToString((long)value,textField);
            break;
          case HEXADECIMAL:
            cvtLongToHexString((long)value, textField);
            break;
          case OCTAL:
            cvtLongToOctalString((long)value, textField);
            break;
          default :
            medmPrintf("textUpdateUpdateValueCb: %s %s %s\n",
	          "unknown channel type for",ca_name(pCh->chid), ": cannot attach Meter");
            medmPostTime();
            break;
        }
      }
      i = pCh->fontIndex;

      XSetForeground(display,pDI->gc, pDI->dlColormap[pTU->monitor.bclr]);
      XFillRectangle(display, XtWindow(pDI->drawingArea),
			pDI->gc,
			pTU->object.x,pTU->object.y,
			pTU->object.width,
			pTU->object.height);


      /* calculate the color */
      gcValueMask = GCForeground | GCBackground;
      switch (pTU->clrmod) {
        case STATIC :
        case DISCRETE:
          gcValues.foreground = pDI->dlColormap[pTU->monitor.clr];
          break;
      case ALARM :
          gcValues.foreground =  alarmColorPixel[pCh->severity];
          break;
      }
      gcValues.background = pDI->dlColormap[pTU->monitor.bclr];
      XChangeGC(display,pDI->gc, gcValueMask,&gcValues);

      /* do the text alignment */
      switch (pTU->align) {
        case HORIZ_LEFT:
          XSetFont(display,pDI->gc,fontTable[i]->fid);
          XDrawString(display,XtWindow(pDI->drawingArea), pDI->gc,
	    pTU->object.x,
	    pTU->object.y + fontTable[i]->ascent,
	    textField,strlen(textField));
          break;
        case HORIZ_CENTER:
          i = dmGetBestFontWithInfo(fontTable,MAX_FONTS,textField,
		pTU->object.height,pTU->object.width,
		&usedHeight,&usedWidth,TRUE);
          XSetFont(display,pDI->gc,fontTable[i]->fid);
          XDrawString(display,XtWindow(pDI->drawingArea), pDI->gc,
		pTU->object.x + (pTU->object.width - usedWidth)/2,
		pTU->object.y + fontTable[i]->ascent,
		textField,strlen(textField));
          break;
        case HORIZ_RIGHT:
          i = dmGetBestFontWithInfo(fontTable,MAX_FONTS,textField,
		pTU->object.height,pTU->object.width,
		&usedHeight,&usedWidth,TRUE);
          XSetFont(display,pDI->gc,fontTable[i]->fid);
          XDrawString(display,XtWindow(pDI->drawingArea),pDI->gc,
		pTU->object.x + pTU->object.width - usedWidth,
		pTU->object.y + fontTable[i]->ascent,
		textField,strlen(textField));
          break;
        case VERT_TOP:
          XSetFont(display,pDI->gc,fontTable[i]->fid);
          XDrawString(display,XtWindow(pDI->drawingArea),pDI->gc,
		pTU->object.x,
		pTU->object.y + fontTable[i]->ascent,
		textField,strlen(textField));
          break;
        case VERT_BOTTOM:
          i = dmGetBestFontWithInfo(fontTable,MAX_FONTS,textField,
	        pTU->object.height,pTU->object.width,
		&usedHeight,&usedWidth,TRUE);
          XSetFont(display,pDI->gc,fontTable[i]->fid);
          XDrawString(display,XtWindow(pDI->drawingArea), pDI->gc,
		pTU->object.x + (pTU->object.width - usedWidth)/2,
		pTU->object.y + fontTable[i]->ascent,
		textField,strlen(textField));
		break;
        case VERT_CENTER:
          i = dmGetBestFontWithInfo(fontTable,MAX_FONTS,textField,
		pTU->object.height,pTU->object.width,
		&usedHeight,&usedWidth,TRUE);
          XSetFont(display,pDI->gc,fontTable[i]->fid);
          XDrawString(display,XtWindow(pDI->drawingArea), pDI->gc,
		pTU->object.x + (pTU->object.width - usedWidth)/2,
		pTU->object.y + fontTable[i]->ascent,
		textField,strlen(textField));
		break;
      }
    } else {
      /* no read access */
      draw3DPane(pCh);
      draw3DQuestionMark(pCh);
    }
  } else {
    /* no connection or disconnected */
    drawWhiteRectangle(pCh);
  }
}
