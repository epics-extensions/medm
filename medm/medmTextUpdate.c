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
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (630-252-2000).
*/
/*****************************************************************************
 *
 *     Original Author : Mark Anderson
 *     Second Author   : Frederick Vong
 *     Third Author    : Kenneth Evans, Jr.
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
    DlElement     *dlElement;
    Record        *record;
    UpdateTask    *updateTask;
    int           fontIndex;
} TextUpdate;

static void textUpdateUpdateValueCb(XtPointer cd);
static void textUpdateDraw(XtPointer cd);
static void textUpdateDestroyCb(XtPointer cd);
static void textUpdateName(XtPointer, char **, short *, int *);
static void textUpdateInheritValues(ResourceBundle *pRCB, DlElement *p);
static void textUpdateSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void textUpdateSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void textUpdateGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable textUpdateDlDispatchTable = {
    createDlTextUpdate,
    NULL,
    executeDlTextUpdate,
    writeDlTextUpdate,
    NULL,
    textUpdateGetValues,
    textUpdateInheritValues,
    textUpdateSetBackgroundColor,
    textUpdateSetForegroundColor,
    genericMove,
    genericScale,
    NULL,
    NULL};

void executeDlTextUpdate(DisplayInfo *displayInfo, DlElement *dlElement)
{
    XRectangle clipRect[1];
    int usedHeight, usedWidth;
    int localFontIndex;
    size_t nChars;
    DlTextUpdate *dlTextUpdate = dlElement->structure.textUpdate;

    if (displayInfo->traversalMode == DL_EXECUTE) {
	TextUpdate *ptu;
	ptu = (TextUpdate *) malloc(sizeof(TextUpdate));
	ptu->dlElement = dlElement;
	ptu->updateTask = updateTaskAddTask(displayInfo,
	  &(dlTextUpdate->object),
	  textUpdateDraw,
	  (XtPointer)ptu);

	if (ptu->updateTask == NULL) {
	    medmPrintf("\ntextUpdateCreateRunTimeInstance: Memory allocation error\n");
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
	  displayInfo->colormap[ dlTextUpdate->monitor.bclr]);
	XFillRectangle(display, XtWindow(displayInfo->drawingArea),
	  displayInfo->gc,
	  dlTextUpdate->object.x,dlTextUpdate->object.y,
	  dlTextUpdate->object.width, dlTextUpdate->object.height);
	XFillRectangle(display,displayInfo->drawingAreaPixmap,
	  displayInfo->gc,
	  dlTextUpdate->object.x,dlTextUpdate->object.y,
	  dlTextUpdate->object.width, dlTextUpdate->object.height);

	XSetForeground(display,displayInfo->gc,
	  displayInfo->colormap[dlTextUpdate->monitor.clr]);
	XSetBackground(display,displayInfo->gc,
	  displayInfo->colormap[dlTextUpdate->monitor.bclr]);
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
    DlTextUpdate *dlTextUpdate = ptu->dlElement->structure.textUpdate;
    DisplayInfo *displayInfo = ptu->updateTask->displayInfo;
    Display *display = XtDisplay(displayInfo->drawingArea);
    char textField[MAX_TOKEN_LENGTH];
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
	    textField[0] = '\0';
	    switch (pd->dataType) {
	    case DBF_STRING :
		if (pd->array) {
		    strncpy(textField,(char *)pd->array, MAX_TEXT_UPDATE_WIDTH-1);
		    textField[MAX_TEXT_UPDATE_WIDTH-1] = '\0';
		}
		isNumber = False;
		break;
	    case DBF_ENUM :
		if (pd->precision >= 0 && pd->hopr+1 > 0) {
		    i = (int) pd->value;
		    if (i >= 0 && i < (int) pd->hopr+1){
			strncpy(textField,pd->stateStrings[i], MAX_TEXT_UPDATE_WIDTH-1);
			textField[MAX_TEXT_UPDATE_WIDTH-1] = '\0';
		    } else {
			textField[0] = ' '; textField[1] = '\0';
		    }
		    isNumber = False;
		} else {
		    value = pd->value;
		    isNumber = True;
		}
		break;
	    case DBF_CHAR :
		if (dlTextUpdate->format == STRING) {
		    if (pd->array) {
			strncpy(textField,pd->array,
			  MIN(pd->elementCount,(MAX_TOKEN_LENGTH-1)));
			textField[MAX_TOKEN_LENGTH-1] = '\0';
		    }
		    isNumber = False;
		    break;
		}
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
		medmPostMsg("textUpdateUpdateValueCb:\n"
		  "  Unknown channel type for %s\n"
		  "  Cannot attach TextUpdate\n",
		  dlTextUpdate->monitor.rdbk);
		break;
	    }
	    if (isNumber) {
		switch (dlTextUpdate->format) {
		case STRING:
		    cvtDoubleToString(value,textField,precision);
		    break;
		case DECIMAL:
		    cvtDoubleToString(value,textField,precision);
		  /* Could be an exponential */
		    if(strchr(textField,'e')) {
			localCvtDoubleToString((double)value,textField,precision);
		    }
		    break;
		case EXPONENTIAL:
#if 0
		    cvtDoubleToExpString(value,textField,precision);
#endif
		  /* KE: This is different from textEntryDRAW/valueToString */
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
		    localCvtLongToHexString((long)value, textField);
		    break;
		case OCTAL:
		    cvtLongToOctalString((long)value, textField);
		    break;
		default :
		    medmPostMsg("textUpdateUpdateValueCb:\n"
		      "  Unknown channel type for %s\n"
		      "  Cannot attach TextUpdate\n",
		      dlTextUpdate->monitor.rdbk);
		    break;
		}
	    }

	    XSetForeground(display,displayInfo->gc, displayInfo->colormap[dlTextUpdate->monitor.bclr]);
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
		gcValues.foreground = displayInfo->colormap[dlTextUpdate->monitor.clr];
		break;
	    case ALARM :
		gcValues.foreground =  alarmColorPixel[pd->severity];
		break;
	    }
	    gcValues.background = displayInfo->colormap[dlTextUpdate->monitor.bclr];
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

	  /* Draw text */
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
	      ptu->updateTask->displayInfo->colormap[dlTextUpdate->monitor.bclr]);
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

DlElement *createDlTextUpdate(DlElement *p)
{
    DlTextUpdate *dlTextUpdate;
    DlElement *dlElement;

    dlTextUpdate = (DlTextUpdate *) malloc(sizeof(DlTextUpdate));
    if (!dlTextUpdate) return 0;
    if (p) {
	*dlTextUpdate = *p->structure.textUpdate;
    } else {
	objectAttributeInit(&(dlTextUpdate->object));
	monitorAttributeInit(&(dlTextUpdate->monitor));
	dlTextUpdate->clrmod = STATIC;
	dlTextUpdate->align = HORIZ_LEFT;
	dlTextUpdate->format = DECIMAL;
    }

    if (!(dlElement = createDlElement(DL_TextUpdate,
      (XtPointer)      dlTextUpdate,
      &textUpdateDlDispatchTable))) {
	free(dlTextUpdate);
    }

    return(dlElement);
}

DlElement *parseTextUpdate(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlTextUpdate *dlTextUpdate;
    DlElement *dlElement = createDlTextUpdate(NULL);
    int i= 0;

    if (!dlElement) return 0;
    dlTextUpdate = dlElement->structure.textUpdate;


    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlTextUpdate->object));
	    } else if (!strcmp(token,"monitor")) {
		parseMonitor(displayInfo,&(dlTextUpdate->monitor));
	    } else if (!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for (i=FIRST_COLOR_MODE;i<FIRST_COLOR_MODE+NUM_COLOR_MODES;i++) {
		    if (!strcmp(token,stringValueTable[i])) {
			dlTextUpdate->clrmod = i;
			break;
		    }
		}
	    } else if (!strcmp(token,"format")) {
		int found = 0;
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for (i=FIRST_TEXT_FORMAT;i<FIRST_TEXT_FORMAT+NUM_TEXT_FORMATS; i++) {
		    if (!strcmp(token,stringValueTable[i])) {
			dlTextUpdate->format = i;
			found = 1;
			break;
		    }
		}
		if (found) {
		    break;
		} else
		/* if not found, do the backward compatibility test */
		  if (!strcmp(token,"decimal")) {
		      dlTextUpdate->format = DECIMAL;
		  } else if (!strcmp(token,
		    "decimal- exponential notation")) {
		      dlTextUpdate->format = EXPONENTIAL;
		  } else if (!strcmp(token,"engr. notation")) {
		      dlTextUpdate->format = ENGR_NOTATION;
		  } else if (!strcmp(token,"decimal- compact")) {
		      dlTextUpdate->format = COMPACT;
		  } else if (!strcmp(token,"decimal- truncated")) {
		      dlTextUpdate->format = TRUNCATED;
/* (MDA) allow for LANL spelling errors {like above, but with trailing space} */
		  } else if (!strcmp(token,"decimal- truncated ")) {
		      dlTextUpdate->format = TRUNCATED;
/* (MDA) allow for LANL spelling errors {hexidecimal vs. hexadecimal} */
		  } else if (!strcmp(token,"hexidecimal")) {
		      dlTextUpdate->format = HEXADECIMAL;
		  }
	    } else if (!strcmp(token,"align")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for (i=FIRST_TEXT_ALIGN;i<FIRST_TEXT_ALIGN+NUM_TEXT_ALIGNS; i++) {
		    if (!strcmp(token,stringValueTable[i])) {
			dlTextUpdate->align = i;
			break;
		    }
		}
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

    return dlElement;

}

void writeDlTextUpdate(FILE *stream, DlElement *dlElement, int level) {
    int i;
    char indent[16];
    DlTextUpdate *dlTextUpdate = dlElement->structure.textUpdate;

    memset(indent,'\t',level);
    indent[level] = '\0';


#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
	fprintf(stream,"\n%s\"text update\" {",indent);
	writeDlObject(stream,&(dlTextUpdate->object),level+1);
	writeDlMonitor(stream,&(dlTextUpdate->monitor),level+1);
	if (dlTextUpdate->clrmod != STATIC) 
	  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	    stringValueTable[dlTextUpdate->clrmod]);
	if (dlTextUpdate->align != HORIZ_LEFT)
	  fprintf(stream,"\n%s\talign=\"%s\"",indent,
	    stringValueTable[dlTextUpdate->align]);
	if (dlTextUpdate->format != DECIMAL)
	  fprintf(stream,"\n%s\tformat=\"%s\"",indent,
	    stringValueTable[dlTextUpdate->format]);
	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
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
#endif
}

static void textUpdateInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlTextUpdate *dlTextUpdate = p->structure.textUpdate;
    medmGetValues(pRCB,
      CTRL_RC,       &(dlTextUpdate->monitor.rdbk),
      CLR_RC,        &(dlTextUpdate->monitor.clr),
      BCLR_RC,       &(dlTextUpdate->monitor.bclr),
      CLRMOD_RC,     &(dlTextUpdate->clrmod),
      ALIGN_RC,      &(dlTextUpdate->align),
      FORMAT_RC,     &(dlTextUpdate->format),
      -1);
}


static void textUpdateGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlTextUpdate *dlTextUpdate = p->structure.textUpdate;
    medmGetValues(pRCB,
      X_RC,          &(dlTextUpdate->object.x),
      Y_RC,          &(dlTextUpdate->object.y),
      WIDTH_RC,      &(dlTextUpdate->object.width),
      HEIGHT_RC,     &(dlTextUpdate->object.height),
      CTRL_RC,       &(dlTextUpdate->monitor.rdbk),
      CLR_RC,        &(dlTextUpdate->monitor.clr),
      BCLR_RC,       &(dlTextUpdate->monitor.bclr),
      CLRMOD_RC,     &(dlTextUpdate->clrmod),
      ALIGN_RC,      &(dlTextUpdate->align),
      FORMAT_RC,     &(dlTextUpdate->format),
      -1);
}

static void textUpdateSetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlTextUpdate *dlTextUpdate = p->structure.textUpdate;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlTextUpdate->monitor.bclr),
      -1);
}

static void textUpdateSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlTextUpdate *dlTextUpdate = p->structure.textUpdate;
    medmGetValues(pRCB,
      CLR_RC,        &(dlTextUpdate->monitor.clr),
      -1);
}
