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
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <cvtFast.h>
#ifdef __cplusplus
	   }
#endif

typedef struct _TextEntry {
    DlElement   *dlElement;
    Record      *record;
    UpdateTask  *updateTask;
    Boolean     updateAllowed;
} TextEntry;

void textEntryCreateRunTimeInstance(DisplayInfo *, DlElement *);
void textEntryCreateEditInstance(DisplayInfo *,DlElement *);

static void textEntryDraw(XtPointer cd);
static void textEntryUpdateValueCb(XtPointer cd);
static void textEntryDestroyCb(XtPointer cd);
static void textEntryValueChanged(Widget, XtPointer, XtPointer);
static void textEntryModifyVerifyCallback(Widget, XtPointer, XtPointer);
static char *valueToString(TextEntry *, TextFormat format);
static void textEntryGetRecord(XtPointer, Record **, int *);
static void textEntryInheritValues(ResourceBundle *pRCB, DlElement *p);
static void textEntrySetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void textEntrySetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void textEntryGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable textEntryDlDispatchTable = {
    createDlTextEntry,
    NULL,
    executeDlTextEntry,
    writeDlTextEntry,
    NULL,
    textEntryGetValues,
    textEntryInheritValues,
    textEntrySetBackgroundColor,
    textEntrySetForegroundColor,
    genericMove,
    genericScale,
    NULL,
    NULL};


int textFieldFontListIndex(int height)
{
    int i;
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

char *valueToString(TextEntry *pte, TextFormat format) {
    Record *pd = pte->record;
    static char textField[MAX_TOKEN_LENGTH];
    double value;
    short precision = 0;

    textField[0] = '\0';
    switch(pd->dataType) {
    case DBF_STRING :
	if (pd->array) {
	    strncpy(textField,(char *)pd->array, MAX_TEXT_UPDATE_WIDTH-1);
	    textField[MAX_TEXT_UPDATE_WIDTH-1] = '\0';
	}
	return textField;
    case DBF_ENUM :
	if ((pd->precision >= 0) && (pd->hopr+1 > 0)) {
	    int i = (int) pd->value;
	  /* getting values of -1 for data->value for invalid connections */
	    if ( i >= 0 && i < (int) pd->hopr+1) {
		strncpy(textField,pd->stateStrings[i], MAX_TEXT_UPDATE_WIDTH-1);
		textField[MAX_TEXT_UPDATE_WIDTH-1] = '\0';
		return textField;
	    } else {
		return " ";
	    }
	} else {
	    value = pd->value;
	}
	break;
    case DBF_CHAR :
	if (format == STRING) {
	    if (pd->array) {
		strncpy(textField,pd->array,
		  MIN(pd->elementCount,(MAX_TOKEN_LENGTH-1)));
		textField[MAX_TOKEN_LENGTH-1] = '\0';
	    }
	    return textField;
	}
    case DBF_INT :
    case DBF_LONG :
    case DBF_FLOAT :
    case DBF_DOUBLE :
	precision = pd->precision;
	value = pd->value;
	break;
    default :
	medmPostMsg(1,"valueToString:\n");
	medmPrintf(0,"  Name: %s\n",pte->dlElement->structure.textEntry->control.ctrl);
	medmPrintf(0,"  Unknown Data Type\n");
	return "Error!";
    }

    if (precision < 0) {
	precision = 0;
    }

    switch (format) {
    case STRING:
	cvtDoubleToString(value,textField,precision);
	break;
    case DECIMAL:
	cvtDoubleToString(value,textField,precision);
      /* Could be an exponential */
	if(strchr(textField,'e')) {
	    localCvtDoubleToString(value,textField,precision);
	}
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
	localCvtLongToHexString((long)value, textField);
	break;
    case OCTAL:
	cvtLongToOctalString((long)value, textField);
	break;
    default :
	medmPostMsg(1,"valueToString:\n");
	medmPrintf(0,"  Name: %s\n",pte->dlElement->structure.textEntry->control.ctrl);
	medmPrintf(0,"  Unknown Format Type\n");
	return "Error!";
    }
    return textField;
}
 
/***
 *** Text Entry
 ***/
void textEntryCreateRunTimeInstance(DisplayInfo *displayInfo,
  DlElement *dlElement) {
    TextEntry *pte;
    Arg args[20];
    int n;
    DlTextEntry *dlTextEntry = dlElement->structure.textEntry;

    pte = (TextEntry *) malloc(sizeof(TextEntry));
    pte->dlElement = dlElement;
    pte->updateTask = updateTaskAddTask(displayInfo,
      &(dlTextEntry->object),
      textEntryDraw,
      (XtPointer)pte);

    if (pte->updateTask == NULL) {
	medmPrintf(1,"\nmenuCreateRunTimeInstance: Memory allocation error\n");
    } else {
	updateTaskAddDestroyCb(pte->updateTask,textEntryDestroyCb);
	updateTaskAddNameCb(pte->updateTask,textEntryGetRecord);
    }
    pte->record = medmAllocateRecord(dlTextEntry->control.ctrl,
      textEntryUpdateValueCb,
      NULL,
      (XtPointer) pte);
    pte->updateAllowed = True;
    drawWhiteRectangle(pte->updateTask);

  /* from the text entry structure, we've got TextEntry's specifics */
    n = 0;
    XtSetArg(args[n],XmNx,(Position)dlTextEntry->object.x); n++;
    XtSetArg(args[n],XmNy,(Position)dlTextEntry->object.y); n++;
    XtSetArg(args[n],XmNwidth,(Dimension)dlTextEntry->object.width); n++;
    XtSetArg(args[n],XmNheight,(Dimension)dlTextEntry->object.height); n++;
    XtSetArg(args[n],XmNforeground,(Pixel)
      displayInfo->colormap[dlTextEntry->control.clr]); n++;
    XtSetArg(args[n],XmNbackground,(Pixel)
      displayInfo->colormap[dlTextEntry->control.bclr]); n++;
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
    dlElement->widget = XtCreateWidget("textField",
      xmTextFieldWidgetClass, displayInfo->drawingArea, args, n);
    
  /* Add the callbacks for update */
    XtAddCallback(dlElement->widget,XmNactivateCallback,
      (XtCallbackProc)textEntryValueChanged, (XtPointer)pte);

  /* Unregister it as a drop site unless it is explicitly a string
   *   (Btn2 drag and drop tends to trash it) */
    if(dlTextEntry->format != STRING) {
	XmDropSiteUnregister(dlElement->widget);
    }
    
  /* special stuff: if user started entering new data into text field, but
   *  doesn't do the actual Activate <CR>, then restore old value on
   *  losing focus...
   */
    XtAddCallback(dlElement->widget,XmNmodifyVerifyCallback,
      (XtCallbackProc)textEntryModifyVerifyCallback,(XtPointer)pte);
}

void textEntryCreateEditInstance(DisplayInfo *displayInfo,
  DlElement *dlElement) {
    Arg args[20];
    int n;
    Widget localWidget;
    DlTextEntry *dlTextEntry = dlElement->structure.textEntry;

  /* from the text entry structure, we've got TextEntry's specifics */
    n = 0;
    XtSetArg(args[n],XmNx,(Position)dlTextEntry->object.x); n++;
    XtSetArg(args[n],XmNy,(Position)dlTextEntry->object.y); n++;
    XtSetArg(args[n],XmNwidth,(Dimension)dlTextEntry->object.width); n++;
    XtSetArg(args[n],XmNheight,(Dimension)dlTextEntry->object.height); n++;
    XtSetArg(args[n],XmNforeground,(Pixel)
      displayInfo->colormap[dlTextEntry->control.clr]); n++;
    XtSetArg(args[n],XmNbackground,(Pixel)
      displayInfo->colormap[dlTextEntry->control.bclr]); n++;
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
    dlElement->widget = localWidget;
    
  /* Add handlers */
    addCommonHandlers(localWidget, displayInfo);
    
    XtManageChild(localWidget);
}

void executeDlTextEntry(DisplayInfo *displayInfo, DlElement *dlElement)
{
    if (displayInfo->traversalMode == DL_EXECUTE) {
	textEntryCreateRunTimeInstance(displayInfo,dlElement);
    } else
      if (displayInfo->traversalMode == DL_EDIT) {
	  if (dlElement->widget) {
	      DlObject *po = &(dlElement->structure.textEntry->object);
	      XtVaSetValues(dlElement->widget,
		XmNx, (Position) po->x,
		XmNy, (Position) po->y,
		XmNwidth, (Dimension) po->width,
		XmNheight, (Dimension) po->height,
		NULL);
	  } else {
	      textEntryCreateEditInstance(displayInfo,dlElement);
	  }
      }
}

void textEntryUpdateValueCb(XtPointer cd) {
    TextEntry *pte = (TextEntry *) ((Record *) cd)->clientData;
    updateTaskMarkUpdate(pte->updateTask);
}

void textEntryDraw(XtPointer cd) {
    TextEntry *pte = (TextEntry *) cd;
    Record *pd = pte->record;
    Widget widget = pte->dlElement->widget;
    DlTextEntry *dlTextEntry = pte->dlElement->structure.textEntry;

    if (pd->connected) {
	if (pd->readAccess) {
	    if (widget) {
		addCommonHandlers(widget, pte->updateTask->displayInfo);
		XtManageChild(widget);
	    }
	    else
	      return;
	    if (pd->writeAccess) {
		XtVaSetValues(widget,XmNeditable,True,NULL);
		XDefineCursor(XtDisplay(widget),XtWindow(widget),rubberbandCursor);
	    } else {
		XtVaSetValues(widget,XmNeditable,False,NULL);
		XDefineCursor(XtDisplay(widget),XtWindow(widget),noWriteAccessCursor);
		pte->updateAllowed = True;
	    }
	    if (pte->updateAllowed) {
		XmTextFieldSetString(widget,valueToString(pte,dlTextEntry->format));
		switch (dlTextEntry->clrmod) {
		case STATIC :
		case DISCRETE:
		    break;
		case ALARM:
		    XtVaSetValues(widget,XmNforeground,alarmColorPixel[pd->severity],NULL);
		    break;
		}
	    }
	} else {
	    draw3DPane(pte->updateTask,
	      pte->updateTask->displayInfo->colormap[dlTextEntry->control.bclr]);
	    draw3DQuestionMark(pte->updateTask);
	    if (widget) XtUnmanageChild(widget);
	}
    } else {
	if (widget) XtUnmanageChild(widget);
	drawWhiteRectangle(pte->updateTask);
    }
}

void textEntryDestroyCb(XtPointer cd) {
    TextEntry *pte = (TextEntry *) cd;
    if (pte) {
	medmDestroyRecord(pte->record);
	free((char *)pte);
    }
    return;
}

/*
 * TextEntry special handling:  if user starts editing text field,
 *  then be sure to update value on losingFocus (since until activate,
 *  the value isn't ca_put()-ed, and the text field can be inconsistent
 *  with the underlying channel
 */
#ifdef __cplusplus
static void textEntryLosingFocusCallback(
  Widget w,
  XtPointer cd,
  XtPointer)
#else
static void textEntryLosingFocusCallback(
  Widget w,
  XtPointer cd,
  XtPointer cbs)
#endif
{
    TextEntry *pte = (TextEntry *) cd;
    XtRemoveCallback(w,XmNlosingFocusCallback,
      (XtCallbackProc)textEntryLosingFocusCallback,pte);
    pte->updateAllowed = True;
    textEntryUpdateValueCb((XtPointer)pte->record);
}


void textEntryModifyVerifyCallback(
  Widget w,
  XtPointer clientData,
  XtPointer pCallbackData)
{
    TextEntry *pte = (TextEntry *) clientData;
    XmTextVerifyCallbackStruct *pcbs = (XmTextVerifyCallbackStruct *) pCallbackData;

  /* NULL event means value changed programmatically; hence don't process */
    if (pcbs->event != NULL) {
	switch (XtHasCallbacks(w,XmNlosingFocusCallback)) {
	case XtCallbackNoList:
	case XtCallbackHasNone:
	    XtAddCallback(w,XmNlosingFocusCallback,
	      (XtCallbackProc)textEntryLosingFocusCallback,pte);
	    pte->updateAllowed = False; 
	    break;
	case XtCallbackHasSome:
	    break;
	}
	pcbs->doit = True;
    }

}

#ifdef __cplusplus
void textEntryValueChanged(Widget  w, XtPointer clientData, XtPointer)
#else
void textEntryValueChanged(Widget  w, XtPointer clientData, XtPointer dummy)
#endif
{
    char *textValue;
    double value;
    TextEntry *pte = (TextEntry *) clientData;
    Record *pd = pte->record;


    if ((pd->connected) && pd->writeAccess) {
	if (!(textValue = XmTextFieldGetString(w))) return;
	switch (pd->dataType) {
	case DBF_STRING:
	    if (strlen(textValue) >= (size_t) MAX_STRING_SIZE) 
	      textValue[MAX_STRING_SIZE-1] = '\0';
	    medmSendString(pte->record,textValue);
	    break;
	case DBF_CHAR:
	    if (pte->dlElement->structure.textEntry->format == STRING) {
		unsigned long len =
		  MIN((unsigned long)pd->elementCount,
		    (unsigned long)(strlen(textValue)+1));
		textValue[len-1] = '\0';
		medmSendCharacterArray(pte->record,textValue,len);
		break;
	    }
	default:
	    if ((strlen(textValue) > (size_t) 2) && (textValue[0] == '0')
	      && (textValue[1] == 'x' || textValue[1] == 'X')) {
		unsigned long longValue;
		longValue = strtoul(textValue,NULL,16);
		value = (double) longValue;
	    } else {
		value = (double) atof(textValue);
	    }
	    medmSendDouble(pte->record,value);
	    break;
	}
	XtFree(textValue);
    }
}

static void textEntryGetRecord(XtPointer cd, Record **record, int *count) {
    TextEntry *pte = (TextEntry *) cd;
    *count = 1;
    record[0] = pte->record;
}

DlElement *createDlTextEntry(DlElement *p)
{
    DlTextEntry *dlTextEntry;
    DlElement *dlElement;

    dlTextEntry = (DlTextEntry *) malloc(sizeof(DlTextEntry));
    if (!dlTextEntry) return 0;
    if (p) {
	*dlTextEntry = *(p->structure.textEntry);
    } else {
	objectAttributeInit(&(dlTextEntry->object));
	controlAttributeInit(&(dlTextEntry->control));
	dlTextEntry->clrmod = STATIC;
	dlTextEntry->format = DECIMAL;
    }

    if (!(dlElement = createDlElement(DL_TextEntry,
      (XtPointer)      dlTextEntry,
      &textEntryDlDispatchTable))) {
	free(dlTextEntry);
    }
    return dlElement;
}

DlElement *parseTextEntry(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlTextEntry *dlTextEntry;
    DlElement *dlElement = createDlTextEntry(NULL);
    int i = 0;

    if (!dlElement) return 0;
    dlTextEntry = dlElement->structure.textEntry; 
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlTextEntry->object));
	    else
	      if (!strcmp(token,"control"))
		parseControl(displayInfo,&(dlTextEntry->control));
	      else
		if (!strcmp(token,"clrmod")) {
		    getToken(displayInfo,token);
		    getToken(displayInfo,token);
		    for (i=FIRST_COLOR_MODE;i<FIRST_COLOR_MODE+NUM_COLOR_MODES;i++) { 
			if (!strcmp(token,stringValueTable[i])) {
			    dlTextEntry->clrmod = i;
			    break;
			}
		    }
		} else
		  if (!strcmp(token,"format")) {
		      getToken(displayInfo,token);
		      getToken(displayInfo,token);
		      for (i=FIRST_TEXT_FORMAT;i<FIRST_TEXT_FORMAT+NUM_TEXT_FORMATS; i++) { 
			  if (!strcmp(token,stringValueTable[i])) {
			      dlTextEntry->format = i;
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

void writeDlTextEntry(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
    char indent[16];
    DlTextEntry *dlTextEntry = dlElement->structure.textEntry;

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
	fprintf(stream,"\n%s\"text entry\" {",indent);
	writeDlObject(stream,&(dlTextEntry->object),level+1);
	writeDlControl(stream,&(dlTextEntry->control),level+1);
	if (dlTextEntry->clrmod != STATIC)
	  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	    stringValueTable[dlTextEntry->clrmod]);
	if (dlTextEntry->format != DECIMAL)
	  fprintf(stream,"\n%s\tformat=\"%s\"",indent,
	    stringValueTable[dlTextEntry->format]);
	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%s\"text entry\" {",indent);
	writeDlObject(stream,&(dlTextEntry->object),level+1);
	writeDlControl(stream,&(dlTextEntry->control),level+1);
	fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	  stringValueTable[dlTextEntry->clrmod]);
	fprintf(stream,"\n%s\tformat=\"%s\"",indent,
	  stringValueTable[dlTextEntry->format]);
	fprintf(stream,"\n%s}",indent);
    }
#endif
}

static void textEntryInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlTextEntry *dlTextEntry = p->structure.textEntry;
    medmGetValues(pRCB,
      CTRL_RC,       &(dlTextEntry->control.ctrl),
      CLR_RC,        &(dlTextEntry->control.clr),
      BCLR_RC,       &(dlTextEntry->control.bclr),
      CLRMOD_RC,     &(dlTextEntry->clrmod),
      FORMAT_RC,     &(dlTextEntry->format),
      -1);
}

static void textEntryGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlTextEntry *dlTextEntry = p->structure.textEntry;
    medmGetValues(pRCB,
      X_RC,          &(dlTextEntry->object.x),
      Y_RC,          &(dlTextEntry->object.y),
      WIDTH_RC,      &(dlTextEntry->object.width),
      HEIGHT_RC,     &(dlTextEntry->object.height),
      CTRL_RC,       &(dlTextEntry->control.ctrl),
      CLR_RC,        &(dlTextEntry->control.clr),
      BCLR_RC,       &(dlTextEntry->control.bclr),
      CLRMOD_RC,     &(dlTextEntry->clrmod),
      FORMAT_RC,     &(dlTextEntry->format),
      -1);
}

static void textEntrySetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlTextEntry *dlTextEntry = p->structure.textEntry;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlTextEntry->control.bclr),
      -1);
}

static void textEntrySetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlTextEntry *dlTextEntry = p->structure.textEntry;
    medmGetValues(pRCB,
      CLR_RC,        &(dlTextEntry->control.clr),
      -1);
}
