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

#ifdef __cplusplus
extern "C" {
#endif
#include <cvtFast.h>
#ifdef __cplusplus
}
#endif

typedef struct _TextUpdate {
  Widget        widget;
  DlTextUpdate  *dlTextUpdate;
  Record        *record;
  UpdateTask    *updateTask;
  int           fontIndex;
} TextUpdate;

static void textUpdateUpdateValueCb(XtPointer cd);
static void textUpdateDraw(XtPointer cd);
static void textUpdateDestroyCb(XtPointer cd);
static void textUpdateName(XtPointer, char **, short *, int *);

#ifdef __cplusplus
void executeDlTextUpdate(DisplayInfo *displayInfo, DlTextUpdate *dlTextUpdate,
				Boolean)
#else
void executeDlTextUpdate(DisplayInfo *displayInfo, DlTextUpdate *dlTextUpdate,
                                Boolean dummy)
#endif
{
  XRectangle clipRect[1];
  int usedHeight, usedWidth;
  int localFontIndex;
  size_t nChars;

  if (displayInfo->traversalMode == DL_EXECUTE) {
    TextUpdate *ptu;
    ptu = (TextUpdate *) malloc(sizeof(TextUpdate));
    ptu->dlTextUpdate = dlTextUpdate;
    ptu->updateTask = updateTaskAddTask(displayInfo,
                                        &(dlTextUpdate->object),
                                        textUpdateDraw,
                                        (XtPointer)ptu);

    if (ptu->updateTask == NULL) {
      medmPrintf("textUpdateCreateRunTimeInstance : memory allocation error\n");
    } else {
      updateTaskAddDestroyCb(ptu->updateTask,textUpdateDestroyCb);
      updateTaskAddNameCb(ptu->updateTask,textUpdateName);
    }
    ptu->record = medmAllocateRecord(dlTextUpdate->monitor.rdbk,
                  textUpdateUpdateValueCb,
                  NULL,
                  (XtPointer) ptu);

    ptu->fontIndex = dmGetBestFontWithInfo(fontTable,
        MAX_FONTS,DUMMY_TEXT_FIELD,
        dlTextUpdate->object.height, dlTextUpdate->object.width,
        &usedHeight, &usedWidth, FALSE);        /* don't use width */

    drawWhiteRectangle(ptu->updateTask);

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


static void textUpdateDestroyCb(XtPointer cd) {
  TextUpdate *ptu = (TextUpdate *) cd;
  if (ptu) {
    medmDestroyRecord(ptu->record);
    free((char *)ptu);
  }
  return;
}

static void textUpdateUpdateValueCb(XtPointer cd) {
  Record *pd = (Record *) cd;
  TextUpdate *ptu = (TextUpdate *) pd->clientData;
  updateTaskMarkUpdate(ptu->updateTask);
}

static void textUpdateDraw(XtPointer cd) {
  TextUpdate *ptu = (TextUpdate *) cd;
  Record *pd = (Record *) ptu->record;
  DlTextUpdate *dlTextUpdate = ptu->dlTextUpdate;
  DisplayInfo *displayInfo = ptu->updateTask->displayInfo;
  Display *display = XtDisplay(displayInfo->drawingArea);
  char textField[MAX_TEXT_UPDATE_WIDTH];
  int i;
  XRectangle clipRect[1];
  XGCValues gcValues;
  unsigned long gcValueMask;
  Boolean isNumber;
  double value = 0.0;
  int precision = 0;
  int textWidth = 0;
  int strLen = 0;

  if (pd->connected) {
    if (pd->readAccess) {
      switch (pd->dataType) {
        case DBF_STRING :
          if (pd->array) {
            strncpy(textField,(char *)pd->array, MAX_TEXT_UPDATE_WIDTH-1);
            textField[MAX_TEXT_UPDATE_WIDTH-1] = '\0';
          } else {
            textField[0] = '\0';
          }
          isNumber = False;
          break;
        case DBF_ENUM :
          if (pd->hopr+1 > 0) {
            i = (int) pd->value;
            if ((i >= 0 && i < pd->hopr+1) && (pd->precision >= 0)){
              strcpy(textField,pd->stateStrings[i]);
            } else {
              textField[0] = ' '; textField[1] = '\0';
            }
            isNumber = False;
          } else {
            value = (double) pd->value;
	    precision = 0;
            isNumber = True;
          }
          break;
        case DBF_CHAR :
        case DBF_INT :
        case DBF_LONG :
        case DBF_FLOAT :
        case DBF_DOUBLE :
          value = pd->value;
          precision = pd->precision;
          if (precision < 0) {
            precision = 0;
          }
          isNumber = True;
          break;
        default :
          medmPrintf("textUpdateUpdateValueCb: %s %s %s\n",
	    "unknown channel type for",ptu->dlTextUpdate->monitor.rdbk, ": cannot attach TextUpdate");
          medmPostTime();
          break;
      }
      if (isNumber) {
        switch (dlTextUpdate->format) {
          case DECIMAL:
            cvtDoubleToString(value,textField,precision);
            break;
          case EXPONENTIAL:
#if 0
            cvtDoubleToExpString(value,textField,precision);
#endif
            sprintf(textField,"%.*e",precision,value);
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
	          "unknown channel type for",ptu->dlTextUpdate->monitor.rdbk, ": cannot attach TextUpdate");
            medmPostTime();
            break;
        }
      }

      XSetForeground(display,displayInfo->gc, displayInfo->dlColormap[dlTextUpdate->monitor.bclr]);
      XFillRectangle(display, XtWindow(displayInfo->drawingArea),
			displayInfo->gc,
			dlTextUpdate->object.x,dlTextUpdate->object.y,
			dlTextUpdate->object.width,
			dlTextUpdate->object.height);


      /* calculate the color */
      gcValueMask = GCForeground | GCBackground;
      switch (dlTextUpdate->clrmod) {
        case STATIC :
        case DISCRETE:
          gcValues.foreground = displayInfo->dlColormap[dlTextUpdate->monitor.clr];
          break;
      case ALARM :
          gcValues.foreground =  alarmColorPixel[pd->severity];
          break;
      }
      gcValues.background = displayInfo->dlColormap[dlTextUpdate->monitor.bclr];
      XChangeGC(display,displayInfo->gc, gcValueMask,&gcValues);

      i = ptu->fontIndex;
      strLen = strlen(textField);
      textWidth = XTextWidth(fontTable[i],textField,strLen);

      /* for compatibility reason, only the HORIZ_CENTER,
       * HORIZ_RIGHT, VERT_BOTTOM and VERT_CENTER
       * will recalculate the font size if the number does
       * not fit. */
      if (dlTextUpdate->object.width  < textWidth) {
        switch(dlTextUpdate->align) {
          case HORIZ_CENTER:
          case HORIZ_RIGHT:
          case VERT_BOTTOM:
          case VERT_CENTER:
            while (i > 0) {
              i--;
              textWidth = XTextWidth(fontTable[i],textField,strLen);
              if (dlTextUpdate->object.width > textWidth) break;
            }
            break;
          default :
            break;
        }
      }

      /* print text */
      {
        int x, y;
        XSetFont(display,displayInfo->gc,fontTable[i]->fid);
        switch (dlTextUpdate->align) {
          case HORIZ_LEFT:
	    x = dlTextUpdate->object.x;
	    y = dlTextUpdate->object.y + fontTable[i]->ascent;
            break;
          case HORIZ_CENTER:
	    x = dlTextUpdate->object.x + (dlTextUpdate->object.width - textWidth)/2;
	    y = dlTextUpdate->object.y + fontTable[i]->ascent;
            break;
          case HORIZ_RIGHT:
	    x = dlTextUpdate->object.x + dlTextUpdate->object.width - textWidth;
	    y =dlTextUpdate->object.y + fontTable[i]->ascent;
            break;
          case VERT_TOP:
	    x = dlTextUpdate->object.x;
	    y = dlTextUpdate->object.y + fontTable[i]->ascent;
            break;
          case VERT_BOTTOM:
	    x = dlTextUpdate->object.x + (dlTextUpdate->object.width - textWidth)/2;
	    y = dlTextUpdate->object.y + fontTable[i]->ascent;
	    break;
          case VERT_CENTER:
	    x = dlTextUpdate->object.x + (dlTextUpdate->object.width - textWidth)/2;
	    y = dlTextUpdate->object.y + fontTable[i]->ascent;
	    break;
        }
        XDrawString(display,XtWindow(displayInfo->drawingArea),
                  displayInfo->gc, x, y,
		  textField,strLen);
      }
    } else {
      /* no read access */
      draw3DPane(ptu->updateTask,
          ptu->updateTask->displayInfo->dlColormap[ptu->dlTextUpdate->monitor.bclr]);
      draw3DQuestionMark(ptu->updateTask);
    }
  } else {
    /* no connection or disconnected */
    drawWhiteRectangle(ptu->updateTask);
  }
}

static void textUpdateName(XtPointer cd, char **name, short *severity, int *count) {
  TextUpdate *pa = (TextUpdate *) cd;
  *count = 1;
  name[0] = pa->record->name;
  severity[0] = pa->record->severity;
}

void writeDlTextUpdate(FILE *stream, DlTextUpdate *dlTextUpdate, int level) {
  int i;
  char indent[16];

  for (i = 0;  i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%s\"text update\" {",indent);
  writeDlObject(stream,&(dlTextUpdate->object),level+1);
  writeDlMonitor(stream,&(dlTextUpdate->monitor),level+1);
  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
        stringValueTable[dlTextUpdate->clrmod]);
  fprintf(stream,"\n%s\talign=\"%s\"",indent,
        stringValueTable[dlTextUpdate->align]);
  fprintf(stream,"\n%s\tformat=\"%s\"",indent,
        stringValueTable[dlTextUpdate->format]);
  fprintf(stream,"\n%s}",indent);

}
