/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*****************************************************************************
 *
 *     Original Author : Mark Anderson
 *     Second Author   : Frederick Vong
 *     Third Author    : Kenneth Evans, Jr.
 *
 *****************************************************************************
*/

#define DEBUG_SHORT 0
#define DEBUG_UPDATE 0

#include "medm.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <cvtFast.h>
#ifdef __cplusplus
	   }
#endif

typedef struct _MedmTextUpdate {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           *record;
    int              fontIndex;
} MedmTextUpdate;

static void textUpdateUpdateValueCb(XtPointer cd);
static void textUpdateDraw(XtPointer cd);
static void textUpdateUpdateGraphicalInfoCb(XtPointer cd);
static void textUpdateDestroyCb(XtPointer cd);
static void textUpdateGetRecord(XtPointer, Record **, int *);
static void textUpdateInheritValues(ResourceBundle *pRCB, DlElement *p);
static void textUpdateSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void textUpdateSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void textUpdateGetLimits(DlElement *pE, DlLimits **ppL, char **pN);
static void textUpdateGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable textUpdateDlDispatchTable = {
    createDlTextUpdate,
    NULL,
    executeDlTextUpdate,
    hideDlTextUpdate,
    writeDlTextUpdate,
    textUpdateGetLimits,
    textUpdateGetValues,
    textUpdateInheritValues,
    textUpdateSetBackgroundColor,
    textUpdateSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

void executeDlTextUpdate(DisplayInfo *displayInfo, DlElement *dlElement)
{
    int usedHeight, usedWidth;
    int localFontIndex;
    size_t nChars;
    DlTextUpdate *dlTextUpdate = dlElement->structure.textUpdate;

#if DEBUG_UPDATE
    print("executeDlTextUpdate:\n");
#endif
  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

  /* Update the limits to reflect current src's */
    updatePvLimits(&dlTextUpdate->limits);

    if(displayInfo->traversalMode == DL_EXECUTE) {
	MedmTextUpdate *ptu;

	if(dlElement->data) {
	  /* Necessary for PV Limits, Cannot use textUpdateDraw since
             it is drawn on the pixmap, unlike widgets */
	    ptu = (MedmTextUpdate *)dlElement->data;
	} else {
	    ptu = (MedmTextUpdate *)malloc(sizeof(MedmTextUpdate));
	    dlElement->updateType = DYNAMIC_GRAPHIC;
	    dlElement->data = (void *)ptu;
	    if(ptu == NULL) {
		medmPrintf(1,"\nexecuteDlTextUpdate: Memory allocation error\n");
		return;
	    }
	  /* Pre-initialize */
	    ptu->updateTask = NULL;
	    ptu->record = NULL;
	    ptu->dlElement = dlElement;

	    ptu->updateTask = updateTaskAddTask(displayInfo,
	      &(dlTextUpdate->object),
	      textUpdateDraw,
	      (XtPointer)ptu);

	    if(ptu->updateTask == NULL) {
		medmPrintf(1,"\nexecuteDlTextUpdate: Memory allocation error\n");
	    } else {
		updateTaskAddDestroyCb(ptu->updateTask,textUpdateDestroyCb);
		updateTaskAddNameCb(ptu->updateTask,textUpdateGetRecord);
	    }
	    ptu->record = medmAllocateRecord(dlTextUpdate->monitor.rdbk,
	      textUpdateUpdateValueCb,
	      textUpdateUpdateGraphicalInfoCb,
	      (XtPointer) ptu);

	    ptu->fontIndex = dmGetBestFontWithInfo(fontTable,
	      MAX_FONTS,DUMMY_TEXT_FIELD,
	      dlTextUpdate->object.height, dlTextUpdate->object.width,
	      &usedHeight, &usedWidth, FALSE);        /* don't use width */

	    drawWhiteRectangle(ptu->updateTask);
	}
    } else {
      /* Static */
	Drawable drawable=updateInProgress?
	  displayInfo->updatePixmap:displayInfo->drawingAreaPixmap;
	Pixmap pixmap;
	GC gc;

	dlElement->updateType = STATIC_GRAPHIC;

      /* Create a GC and pixmap to take care of clipping to the
	 size of the text update */
	gc = XCreateGC(display, rootWindow, 0, NULL);
      /* Eliminate events that we do not handle anyway */
	XSetGraphicsExposures(display, gc, False);
	pixmap = XCreatePixmap(display, RootWindow(display,screenNum),
	  dlTextUpdate->object.width, dlTextUpdate->object.height,
	  DefaultDepth(display,screenNum));

      /* Fill it with the background color */
	XSetForeground(display, gc,
	  displayInfo->colormap[dlTextUpdate->monitor.bclr]);
	XFillRectangle(display, pixmap, gc, 0, 0,
	  dlTextUpdate->object.width, dlTextUpdate->object.height);

	XSetForeground(display,gc,
	  displayInfo->colormap[dlTextUpdate->monitor.clr]);
	XSetBackground(display,gc,
	  displayInfo->colormap[dlTextUpdate->monitor.bclr]);
	nChars = strlen(dlTextUpdate->monitor.rdbk);
	localFontIndex = dmGetBestFontWithInfo(fontTable,
	  MAX_FONTS,dlTextUpdate->monitor.rdbk,
	  dlTextUpdate->object.height, dlTextUpdate->object.width,
	  &usedHeight, &usedWidth, FALSE);	/* don't use width */
	usedWidth = XTextWidth(fontTable[localFontIndex],dlTextUpdate->monitor.rdbk,
	  nChars);

	XSetFont(display,gc,fontTable[localFontIndex]->fid);
	switch(dlTextUpdate->align) {
	case HORIZ_LEFT:
	    XDrawString(display, pixmap, gc,
	      0,
	      fontTable[localFontIndex]->ascent, dlTextUpdate->monitor.rdbk,
	      strlen(dlTextUpdate->monitor.rdbk));
	    break;
	case HORIZ_CENTER:
	    XDrawString(display, pixmap, gc,
	      (dlTextUpdate->object.width - usedWidth)/2,
	      fontTable[localFontIndex]->ascent, dlTextUpdate->monitor.rdbk,
	      strlen(dlTextUpdate->monitor.rdbk));
	    break;
	case HORIZ_RIGHT:
	    XDrawString(display, pixmap, gc,
	      dlTextUpdate->object.width - usedWidth,
	      fontTable[localFontIndex]->ascent, dlTextUpdate->monitor.rdbk,
	      strlen(dlTextUpdate->monitor.rdbk));
	    break;
	}

      /* Copy the pixmap to the drawing area and the
         drawingAreaPixmap, utilizing any clipping. */
	XCopyArea(display, pixmap, drawable, displayInfo->gc,
	  0, 0, dlTextUpdate->object.width, dlTextUpdate->object.height,
	  dlTextUpdate->object.x, dlTextUpdate->object.y);
	XFreePixmap(display, pixmap);
	XFreeGC(display, gc);
    }
}

void hideDlTextUpdate(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element drawn on the display drawingArea */
    hideDrawnElement(displayInfo, dlElement);
}

static void textUpdateDestroyCb(XtPointer cd)
{
    MedmTextUpdate *ptu = (MedmTextUpdate *) cd;
    if(ptu) {
	medmDestroyRecord(ptu->record);
	if(ptu->dlElement) ptu->dlElement->data = NULL;
	free((char *)ptu);
    }
    return;
}

static void textUpdateUpdateValueCb(XtPointer cd)
{
    Record *pR = (Record *)cd;
    MedmTextUpdate *ptu = (MedmTextUpdate *)pR->clientData;

    updateTaskMarkUpdate(ptu->updateTask);
}

static void textUpdateDraw(XtPointer cd)
{
    MedmTextUpdate *ptu = (MedmTextUpdate *) cd;
    Record *pR = (Record *)ptu->record;
    DlTextUpdate *dlTextUpdate = ptu->dlElement->structure.textUpdate;
    DisplayInfo *displayInfo = ptu->updateTask->displayInfo;
    Display *display = XtDisplay(displayInfo->drawingArea);
    char textField[MAX_TOKEN_LENGTH];
    int i;
    Pixmap pixmap;
    GC gc;
    Boolean isNumber;
    double value = 0.0;
    short precision = 0;
    int textWidth = 0;
    int strLen = 0;
    int status;

#if DEBUG_UPDATE
    print("textUpdateDraw:\n");
#endif
    if(pR && pR->connected) {
      /* KE: Can be connected without graphical info or value yet */
	if(pR->readAccess) {
	    textField[0] = '\0';
	    switch (pR->dataType) {
	    case DBF_STRING:
		if(pR->array) {
		    strncpy(textField,(char *)pR->array, MAX_TEXT_UPDATE_WIDTH-1);
		    textField[MAX_TEXT_UPDATE_WIDTH-1] = '\0';
		}
		isNumber = False;
		break;
	    case DBF_ENUM:
		if(pR->precision >= 0 && pR->hopr+1 > 0) {
		    i = (int) pR->value;
		    if(i >= 0 && i < (int) pR->hopr+1){
			strncpy(textField,pR->stateStrings[i], MAX_TEXT_UPDATE_WIDTH-1);
			textField[MAX_TEXT_UPDATE_WIDTH-1] = '\0';
		    } else {
			textField[0] = ' '; textField[1] = '\0';
		    }
		    isNumber = False;
		} else {
		    value = pR->value;
		    isNumber = True;
		}
		break;
	    case DBF_CHAR:
		if(dlTextUpdate->format == STRING) {
		    if(pR->array) {
			strncpy(textField,pR->array,
			  MIN(pR->elementCount,(MAX_TOKEN_LENGTH-1)));
			textField[MAX_TOKEN_LENGTH-1] = '\0';
		    }
		    isNumber = False;
		    break;
		}
	    case DBF_INT:
	    case DBF_LONG:
	    case DBF_FLOAT:
	    case DBF_DOUBLE:
		value = pR->value;
		precision = dlTextUpdate->limits.prec;
		isNumber = True;
		break;
	    default:
		medmPostMsg(1,"textUpdateUpdateValueCb:\n"
		  "  Unknown channel type for %s\n"
		  "  Cannot attach TextUpdate\n",
		  dlTextUpdate->monitor.rdbk);
		break;
	    }
	  /* KE: Value can be received before the graphical info
	   *   Set precision to 0 if it is still -1 from initialization */
	    if(precision < 0) precision = 0;
	  /* Convert bad values of precision to high precision */
	    if(precision > 17) precision = 17;

	  /* Do the value */
	    if(isNumber) {
		switch (dlTextUpdate->format) {
		case STRING:
		    cvtDoubleToString(value,textField,precision);
		    break;
		case MEDM_DECIMAL:
		    cvtDoubleToString(value,textField,precision);
#if 0
		  /* KE: Don't do this, it can overflow the stack for large numbers */
		  /* Could be an exponential */
		    if(strchr(textField,'e')) {
			localCvtDoubleToString((double)value,textField,precision);
		    }
#endif
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
		case SEXAGESIMAL:
  		    medmLocalCvtDoubleToSexaStr(value,textField,precision,
		      0.0, 0.0, &status);
		    break;
                case SEXAGESIMAL_HMS:
                    medmLocalCvtDoubleToSexaStr(value*12.0/M_PI,textField,precision,
		      0.0,0.0,&status);
                    break;
                case SEXAGESIMAL_DMS:
                    medmLocalCvtDoubleToSexaStr(value*180.0/M_PI,textField,precision,
		      0.0,0.0,&status);
                    break;
		case OCTAL:
		    cvtLongToOctalString((long)value, textField);
		    break;
		default:
		    medmPostMsg(1,"textUpdateUpdateValueCb:\n"
		      "  Unknown channel type for %s\n"
		      "  Cannot attach TextUpdate\n",
		      dlTextUpdate->monitor.rdbk);
		    break;
		}
	    }

#if DEBUG_SHORT
	    print("textUpdateDraw: pR->name=%s pR->dataType=%d(%s)\n"
	      "  pR->value=%g  textField=|%s|\n",
	      pR->name,pR->dataType,dbf_type_to_text(pR->dataType),
	      pR->value,textField);
	    print("short=%d int=%d long=%d\n",sizeof(short),sizeof(int),
	      sizeof(long));
#endif

	  /* Create a GC and pixmap to take care of clipping to the
             size of the text update */
	    gc = XCreateGC(display, rootWindow, 0, NULL);
	  /* Eliminate events that we do not handle anyway */
	    XSetGraphicsExposures(display, gc, False);
	    pixmap = XCreatePixmap(display, RootWindow(display,screenNum),
	      dlTextUpdate->object.width, dlTextUpdate->object.height,
	      DefaultDepth(display,screenNum));
	  /* Fill it with the background color */
	    XSetForeground(display, gc,
	      displayInfo->colormap[dlTextUpdate->monitor.bclr]);
	    XFillRectangle(display, pixmap, gc, 0, 0,
	      dlTextUpdate->object.width, dlTextUpdate->object.height);

	  /* Calculate the foreground color */
	    switch (dlTextUpdate->clrmod) {
	    case STATIC:
	    case DISCRETE:
		XSetForeground(display, gc,
		  displayInfo->colormap[dlTextUpdate->monitor.clr]);
		break;
	    case ALARM:
		pR->monitorSeverityChanged = True;
		XSetForeground(display, gc, alarmColor(pR->severity));
		break;
	    }
	    XSetBackground(display, gc,
	      displayInfo->colormap[dlTextUpdate->monitor.bclr]);

	    i = ptu->fontIndex;
	    strLen = strlen(textField);
	    textWidth = XTextWidth(fontTable[i],textField,strLen);

	  /* for compatibility reason, only the HORIZ_CENTER and
	   * HORIZ_RIGHT will recalculate the font size if the number does
	   * not fit. */
	    if((int)dlTextUpdate->object.width  < textWidth) {
		switch(dlTextUpdate->align) {
		case HORIZ_CENTER:
		case HORIZ_RIGHT:
		    while(i > 0) {
			i--;
			textWidth = XTextWidth(fontTable[i],textField,strLen);
			if((int)dlTextUpdate->object.width > textWidth) break;
		    }
		    break;
		default:
		    break;
		}
	    }

	  /* Draw the text */
	    {
	      /* KE: y is the same for all and there are only three
                 distinct cases */
		int x, y;

		XSetFont(display, gc, fontTable[i]->fid);
		switch (dlTextUpdate->align) {
		case HORIZ_LEFT:
		    x = 0;
		    y = fontTable[i]->ascent;
		    break;
		case HORIZ_CENTER:
		    x = (dlTextUpdate->object.width - textWidth)/2;
		    y = fontTable[i]->ascent;
		    break;
		case HORIZ_RIGHT:
		    x = dlTextUpdate->object.width - textWidth;
		    y = fontTable[i]->ascent;
		    break;
		}

	      /* Draw the string */
		XDrawString(display, pixmap, gc, x, y,
		  textField, strLen);

	      /* Copy the pixmap to the drawing area and the
                 drawingAreaPixmap, utilizing any clipping. */
		XCopyArea(display, pixmap,
		  displayInfo->updatePixmap,
		  displayInfo->gc, 0, 0,
		  dlTextUpdate->object.width, dlTextUpdate->object.height,
		  dlTextUpdate->object.x, dlTextUpdate->object.y);
		XFreePixmap(display, pixmap);
		XFreeGC(display, gc);
	    }
	} else {
	  /* No read access */
	    drawBlackRectangle(ptu->updateTask);
	}
    } else {
      /* no connection or disconnected */
	drawWhiteRectangle(ptu->updateTask);
    }
}

static void textUpdateUpdateGraphicalInfoCb(XtPointer cd)
{
    Record *pR = (Record *) cd;
    MedmTextUpdate *ptu = (MedmTextUpdate *) pR->clientData;
    DlTextUpdate *dlTextUpdate = ptu->dlElement->structure.textUpdate;
    XcVType hopr, lopr, val;
    short precision;


  /* Get values from the record  and adjust them */
    hopr.fval = (float) pR->hopr;
    lopr.fval = (float) pR->lopr;
    val.fval = (float) pR->value;
    if((hopr.fval == 0.0) && (lopr.fval == 0.0)) {
	hopr.fval += 1.0;
    }
    precision = pR->precision;
    if(precision < 0) precision = 0;
    if(precision > 17) precision = 17;

  /* Set lopr and hopr to channel - they aren't used by the TextUpdate */
    dlTextUpdate->limits.lopr = lopr.fval;
    dlTextUpdate->limits.loprChannel = lopr.fval;
    dlTextUpdate->limits.hopr = hopr.fval;
    dlTextUpdate->limits.hoprChannel = hopr.fval;

  /* Set Channel and User limits for prec (if apparently not set yet) */
    dlTextUpdate->limits.precChannel = precision;
    if(dlTextUpdate->limits.precSrc != PV_LIMITS_USER &&
      dlTextUpdate->limits.precUser == PREC_DEFAULT) {
	dlTextUpdate->limits.precUser = precision;
    }
    if(dlTextUpdate->limits.precSrc == PV_LIMITS_CHANNEL) {
	dlTextUpdate->limits.prec = precision;
    }
}

static void textUpdateGetRecord(XtPointer cd, Record **record, int *count)
{
      MedmTextUpdate *pa = (MedmTextUpdate *) cd;
    *count = 1;
    record[0] = pa->record;
}

DlElement *createDlTextUpdate(DlElement *p)
{
    DlTextUpdate *dlTextUpdate;
    DlElement *dlElement;

    dlTextUpdate = (DlTextUpdate *)malloc(sizeof(DlTextUpdate));
    if(!dlTextUpdate) return 0;
    if(p) {
	*dlTextUpdate = *p->structure.textUpdate;
    } else {
	objectAttributeInit(&(dlTextUpdate->object));
	monitorAttributeInit(&(dlTextUpdate->monitor));
	limitsAttributeInit(&(dlTextUpdate->limits));
	dlTextUpdate->limits.loprSrc0 = PV_LIMITS_UNUSED;
	dlTextUpdate->limits.loprSrc = PV_LIMITS_UNUSED;
	dlTextUpdate->limits.hoprSrc0 = PV_LIMITS_UNUSED;
	dlTextUpdate->limits.hoprSrc = PV_LIMITS_UNUSED;
	dlTextUpdate->clrmod = STATIC;
	dlTextUpdate->align = HORIZ_LEFT;
	dlTextUpdate->format = MEDM_DECIMAL;
    }

    if(!(dlElement = createDlElement(DL_TextUpdate,
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

    if(!dlElement) return 0;
    dlTextUpdate = dlElement->structure.textUpdate;


    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlTextUpdate->object));
	    } else if(!strcmp(token,"monitor")) {
		parseMonitor(displayInfo,&(dlTextUpdate->monitor));
	    } else if(!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for(i=FIRST_COLOR_MODE;i<FIRST_COLOR_MODE+NUM_COLOR_MODES;i++) {
		    if(!strcmp(token,stringValueTable[i])) {
			dlTextUpdate->clrmod = i;
			break;
		    }
		}
	    } else if(!strcmp(token,"format")) {
		int found = 0;

		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for(i=FIRST_TEXT_FORMAT;i<FIRST_TEXT_FORMAT+NUM_TEXT_FORMATS; i++) {
		    if(!strcmp(token,stringValueTable[i])) {
			dlTextUpdate->format = i;
			found = 1;
			break;
		    }
		}
	      /* Backward compatibility */
		if(!found) {
		    if(!strcmp(token,"decimal")) {
			dlTextUpdate->format = MEDM_DECIMAL;
		    } else if(!strcmp(token,
		      "decimal- exponential notation")) {
			dlTextUpdate->format = EXPONENTIAL;
		    } else if(!strcmp(token,"engr. notation")) {
			dlTextUpdate->format = ENGR_NOTATION;
		    } else if(!strcmp(token,"decimal- compact")) {
			dlTextUpdate->format = COMPACT;
		    } else if(!strcmp(token,"decimal- truncated")) {
			dlTextUpdate->format = TRUNCATED;
		      /* (MDA) allow for LANL spelling errors {like
                         above, but with trailing space} */
		    } else if(!strcmp(token,"decimal- truncated ")) {
			dlTextUpdate->format = TRUNCATED;
		      /* (MDA) allow for LANL spelling errors
                         {hexidecimal vs. hexadecimal} */
		    } else if(!strcmp(token,"hexidecimal")) {
			dlTextUpdate->format = HEXADECIMAL;
		    }
		}
	    } else if(!strcmp(token,"align")) {
		int found=0;

		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for(i=FIRST_TEXT_ALIGN;i<FIRST_TEXT_ALIGN+NUM_TEXT_ALIGNS; i++) {
		    if(!strcmp(token,stringValueTable[i])) {
			dlTextUpdate->align = i;
			found=1;
			break;
		    }
		}
	      /* Backward compatibility */
		if(!found) {
		    if(!strcmp(token,"vert. top")) {
			dlTextUpdate->align = HORIZ_LEFT;
		    } else if(!strcmp(token,"vert. centered")) {
			dlTextUpdate->align = HORIZ_CENTER;
		    } else if(!strcmp(token,"vert. bottom")) {
			dlTextUpdate->align = HORIZ_RIGHT;
		    }
		}
	    } else if(!strcmp(token,"limits")) {
		parseLimits(displayInfo,&(dlTextUpdate->limits));
	    }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++;
	    break;
	case T_RIGHT_BRACE:
	    nestingLevel--;
	    break;
	default:
	    break;
	}
    } while( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );

    return dlElement;
}

void writeDlTextUpdate(FILE *stream, DlElement *dlElement, int level)
{
    char indent[16];
    DlTextUpdate *dlTextUpdate = dlElement->structure.textUpdate;

    memset(indent,'\t',level);
    indent[level] = '\0';


#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
	fprintf(stream,"\n%s\"text update\" {",indent);
	writeDlObject(stream,&(dlTextUpdate->object),level+1);
	writeDlMonitor(stream,&(dlTextUpdate->monitor),level+1);
	if(dlTextUpdate->clrmod != STATIC)
	  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	    stringValueTable[dlTextUpdate->clrmod]);
	if(dlTextUpdate->align != HORIZ_LEFT)
	  fprintf(stream,"\n%s\talign=\"%s\"",indent,
	    stringValueTable[dlTextUpdate->align]);
	if(dlTextUpdate->format != MEDM_DECIMAL)
	  fprintf(stream,"\n%s\tformat=\"%s\"",indent,
	    stringValueTable[dlTextUpdate->format]);
	writeDlLimits(stream,&(dlTextUpdate->limits),level+1);
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
	writeDlLimits(stream,&(dlTextUpdate->limits),level+1);
	fprintf(stream,"\n%s}",indent);
    }
#endif
}

static void textUpdateInheritValues(ResourceBundle *pRCB, DlElement *p)
{
    DlTextUpdate *dlTextUpdate = p->structure.textUpdate;
    medmGetValues(pRCB,
      CTRL_RC,       &(dlTextUpdate->monitor.rdbk),
      CLR_RC,        &(dlTextUpdate->monitor.clr),
      BCLR_RC,       &(dlTextUpdate->monitor.bclr),
      CLRMOD_RC,     &(dlTextUpdate->clrmod),
      ALIGN_RC,      &(dlTextUpdate->align),
      FORMAT_RC,     &(dlTextUpdate->format),
      LIMITS_RC,     &(dlTextUpdate->limits),
      -1);
}

static void textUpdateGetLimits(DlElement *pE, DlLimits **ppL, char **pN)
{
    DlTextUpdate *dlTextUpdate = pE->structure.textUpdate;

    *(ppL) = &(dlTextUpdate->limits);
    *(pN) = dlTextUpdate->monitor.rdbk;
}

static void textUpdateGetValues(ResourceBundle *pRCB, DlElement *p)
{
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
      LIMITS_RC,     &(dlTextUpdate->limits),
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
