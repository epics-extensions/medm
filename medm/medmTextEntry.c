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

#define DEBUG_STRTOUL 0

#include "medm.h"
#include <stdlib.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <cvtFast.h>
#ifdef __cplusplus
	   }
#endif

typedef struct _MedmTextEntry {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           *record;
    Boolean          updateAllowed;
} MedmTextEntry;

void textEntryCreateRunTimeInstance(DisplayInfo *, DlElement *);
void textEntryCreateEditInstance(DisplayInfo *,DlElement *);

static void textEntryDraw(XtPointer cd);
static void textEntryUpdateGraphicalInfoCb(XtPointer cd);
static void textEntryUpdateValueCb(XtPointer cd);
static void textEntryDestroyCb(XtPointer cd);
static void textEntryValueChanged(Widget, XtPointer, XtPointer);
static void textEntryModifyVerifyCallback(Widget, XtPointer, XtPointer);
static char *valueToString(MedmTextEntry *, TextFormat format);
static void textEntryGetRecord(XtPointer, Record **, int *);
static void textEntryInheritValues(ResourceBundle *pRCB, DlElement *p);
static void textEntrySetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void textEntrySetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void textEntryGetLimits(DlElement *pE, DlLimits **ppL, char **pN);
static void textEntryGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable textEntryDlDispatchTable = {
    createDlTextEntry,
    NULL,
    executeDlTextEntry,
    hideDlTextEntry,
    writeDlTextEntry,
    textEntryGetLimits,
    textEntryGetValues,
    textEntryInheritValues,
    textEntrySetBackgroundColor,
    textEntrySetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};


int textFieldFontListIndex(int height)
{
    int i;
/* don't allow height of font to exceed 90% - 4 pixels of textField widget
 *	(includes nominal 2*shadowThickness=2 shadow)
 */
    for(i = MAX_FONTS-1; i >=  0; i--) {
	if( ((int)(.90*height) - 4) >=
	  (fontTable[i]->ascent + fontTable[i]->descent))
	  return(i);
    }
    return (0);
}

static char *valueToString(MedmTextEntry *pte, TextFormat format)
{
    Record *pr = pte->record;
    DlTextEntry *dlTextEntry = pte->dlElement->structure.textEntry;
    static char textField[MAX_TOKEN_LENGTH];
    double value;
    short precision = 0;
    int status;

    textField[0] = '\0';
    switch(pr->dataType) {
    case DBF_STRING:
	if(pr->array) {
	    strncpy(textField,(char *)pr->array, MAX_TEXT_UPDATE_WIDTH-1);
	    textField[MAX_TEXT_UPDATE_WIDTH-1] = '\0';
	}
	return textField;
    case DBF_ENUM:
	if((pr->precision >= 0) && (pr->hopr+1 > 0)) {
	    int i = (int) pr->value;
	  /* getting values of -1 for data->value for invalid connections */
	    if( i >= 0 && i < (int) pr->hopr+1) {
		strncpy(textField,pr->stateStrings[i], MAX_TEXT_UPDATE_WIDTH-1);
		textField[MAX_TEXT_UPDATE_WIDTH-1] = '\0';
		return textField;
	    } else {
		return " ";
	    }
	} else {
	    value = pr->value;
	}
	break;
    case DBF_CHAR:
	if(format == STRING) {
	    if(pr->array) {
		strncpy(textField,pr->array,
		  MIN(pr->elementCount,(MAX_TOKEN_LENGTH-1)));
		textField[MAX_TOKEN_LENGTH-1] = '\0';
	    }
	    return textField;
	}
    case DBF_INT:
    case DBF_LONG:
    case DBF_FLOAT:
    case DBF_DOUBLE:
	precision = dlTextEntry->limits.prec;
	value = pr->value;
	break;
    default:
	medmPostMsg(1,"valueToString:\n");
	medmPrintf(0,"  Name: %s\n",
	  pte->dlElement->structure.textEntry->control.ctrl);
	medmPrintf(0,"  Unknown Data Type\n");
	return "Error!";
    }

  /* KE: Value can be received before the graphical info
   *   Set precision to 0 if it is still -1 from initialization */
    if(precision < 0) precision = 0;
  /* Convert bad values of precision to high precision */
    if(precision > 17) precision = 17;
    switch (format) {
    case STRING:
	cvtDoubleToString(value,textField,precision);
	break;
    case MEDM_DECIMAL:
	cvtDoubleToString(value,textField,precision);
#if 0
      /* KE: Don't do this, it can overflow the stack for large numbers */
      /* Could be an exponential */
	if(strchr(textField,'e')) {
	    localCvtDoubleToString(value,textField,precision);
	}
#endif
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
#if DEBUG_STRTOUL
	print("valueToString: %s %.1f\n",
	  textField, value);
#endif
	break;
    case SEXAGESIMAL:
	medmLocalCvtDoubleToSexaStr(value,textField,precision,
	  0.0,0.0,&status);
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
  DlElement *dlElement)
{
    MedmTextEntry *pte;
    Arg args[20];
    int n;
    DlTextEntry *dlTextEntry = dlElement->structure.textEntry;

  /* If the widget is already created just return.  The update task
     will handle it. */
    if(dlElement->widget) {
	if(dlElement->data) {
	  /* This is necessary for PV Limits */
	    textEntryDraw((XtPointer)dlElement->data);
	}
	return;
    }

    if(dlElement->data) {
	pte = (MedmTextEntry *)dlElement->data;
    } else {
	pte = (MedmTextEntry *)malloc(sizeof(MedmTextEntry));
	dlElement->data = (void *)pte;
	if(pte == NULL) {
	    medmPrintf(1,"\ntextEntryCreateRunTimeInstance:"
	      " Memory allocation error\n");
	    return;
	}
      /* Pre-initialize */
	pte->updateTask = NULL;
	pte->record = NULL;
	pte->dlElement = dlElement;

	pte->updateTask = updateTaskAddTask(displayInfo,
	  &(dlTextEntry->object),
	  textEntryDraw,
	  (XtPointer)pte);

	if(pte->updateTask == NULL) {
	    medmPrintf(1,"\nmenuCreateRunTimeInstance: Memory allocation error\n");
	} else {
	    updateTaskAddDestroyCb(pte->updateTask,textEntryDestroyCb);
	    updateTaskAddNameCb(pte->updateTask,textEntryGetRecord);
	}
	pte->record = medmAllocateRecord(dlTextEntry->control.ctrl,
	  textEntryUpdateValueCb,
	  textEntryUpdateGraphicalInfoCb,
	  (XtPointer) pte);
	pte->updateAllowed = True;
	drawWhiteRectangle(pte->updateTask);
    }

  /* Create the widget */
    n = 0;
    XtSetArg(args[n],XmNx,(Position)dlTextEntry->object.x); n++;
    XtSetArg(args[n],XmNy,(Position)dlTextEntry->object.y); n++;
    XtSetArg(args[n],XmNwidth,(Dimension)dlTextEntry->object.width); n++;
    XtSetArg(args[n],XmNheight,(Dimension)dlTextEntry->object.height); n++;
    XtSetArg(args[n],XmNforeground,(Pixel)
      displayInfo->colormap[dlTextEntry->control.clr]); n++;
    XtSetArg(args[n],XmNbackground,(Pixel)
      displayInfo->colormap[dlTextEntry->control.bclr]); n++;
    XtSetArg(args[n],XmNhighlightThickness,0); n++;
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

  /* Add a callback called when the user starts entering new data into
   *   the text field.  The textEntryModifyVerifyCallback only adds the
   *   textEntryLosingFocus callback and only if it is not added already.
   *   The textEntryLosingFocus callback handles incomplete input by
   *   calling updateTaskMarkUpdate.  The textEntryLosingFocusCallback
   *   removes itself.
   * KE: But it isn't removed with textEntryValueChanged.  It stays
   *   in effect until the Text Entry loses focus
   */
    XtAddCallback(dlElement->widget,XmNmodifyVerifyCallback,
      (XtCallbackProc)textEntryModifyVerifyCallback,(XtPointer)pte);
}

void textEntryCreateEditInstance(DisplayInfo *displayInfo,
  DlElement *dlElement)
{
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
    XtSetArg(args[n],XmNhighlightThickness,0); n++;
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
    DlTextEntry *dlTextEntry = dlElement->structure.textEntry;

  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

  /* Update the limits to reflect current src's */
    updatePvLimits(&dlTextEntry->limits);

    if(displayInfo->traversalMode == DL_EXECUTE) {
	textEntryCreateRunTimeInstance(displayInfo,dlElement);
    } else if(displayInfo->traversalMode == DL_EDIT) {
	if(dlElement->widget) {
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

void hideDlTextEntry(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element with a widget */
    hideWidgetElement(displayInfo, dlElement);
}

static void textEntryUpdateValueCb(XtPointer cd)
{
    MedmTextEntry *pte = (MedmTextEntry *) ((Record *) cd)->clientData;
    updateTaskMarkUpdate(pte->updateTask);
}

static void textEntryDraw(XtPointer cd)
{
    MedmTextEntry *pte = (MedmTextEntry *) cd;
    Record *pr = pte->record;
    DlElement *dlElement = pte->dlElement;
    Widget widget = dlElement->widget;
    DlTextEntry *dlTextEntry = dlElement->structure.textEntry;

  /* Check if hidden */
    if(dlElement->hidden) {
	if(widget && XtIsManaged(widget)) {
	    XtUnmanageChild(widget);
	}
	return;
    }

    if(pr && pr->connected) {
	if(pr->readAccess) {
	    if(widget) {
		addCommonHandlers(widget, pte->updateTask->displayInfo);
		XtManageChild(widget);
	    }
	    else
	      return;
	    if(pr->writeAccess) {
		XtVaSetValues(widget,XmNeditable,True,NULL);
		XDefineCursor(XtDisplay(widget),XtWindow(widget),rubberbandCursor);
	    } else {
		XtVaSetValues(widget,XmNeditable,False,NULL);
		XDefineCursor(XtDisplay(widget),XtWindow(widget),noWriteAccessCursor);
		pte->updateAllowed = True;
	    }
	    if(pte->updateAllowed) {
		XmTextFieldSetString(widget,valueToString(pte,dlTextEntry->format));
		switch (dlTextEntry->clrmod) {
		case STATIC:
		case DISCRETE:
		    break;
		case ALARM:
		    pr->monitorSeverityChanged = True;
		    XtVaSetValues(widget,XmNforeground,alarmColor(pr->severity),NULL);
		    break;
		}
	    }
	} else {
	    if(widget && XtIsManaged(widget))
	      XtUnmanageChild(widget);
	    drawBlackRectangle(pte->updateTask);
	    if(widget) XtUnmanageChild(widget);
	}
    } else {
	if(widget && XtIsManaged(widget))
	  XtUnmanageChild(widget);
	drawWhiteRectangle(pte->updateTask);
    }
}

static void textEntryDestroyCb(XtPointer cd)
{
    MedmTextEntry *pte = (MedmTextEntry *) cd;
    if(pte) {
	medmDestroyRecord(pte->record);
	if(pte->dlElement) pte->dlElement->data = NULL;
	free((char *)pte);
    }
    return;
}

/*
 * This routine removes itself and calls textEntryUpdateValueCbTextEntry
 *   if the focus is lost.  This keeps the Text Field consistent with
 *   the underlying channel.
 * This callback is added by the textEntryModifyVerifyCallback when
 *   User input starts.
 */
static void textEntryLosingFocusCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    MedmTextEntry *pte = (MedmTextEntry *) cd;

    UNREFERENCED(cbs);

    XtRemoveCallback(w,XmNlosingFocusCallback,
      (XtCallbackProc)textEntryLosingFocusCallback,pte);
    pte->updateAllowed = True;
    textEntryUpdateValueCb((XtPointer)pte->record);
}


/* This routine adds the textEntryLosingFocusCallback when user input starts
 *   if it has not already been added.
 */
static void textEntryModifyVerifyCallback(Widget w, XtPointer clientData,
  XtPointer pCallbackData)
{
    MedmTextEntry *pte = (MedmTextEntry *) clientData;
    XmTextVerifyCallbackStruct *pcbs = (XmTextVerifyCallbackStruct *) pCallbackData;

  /* NULL event means value changed programmatically; hence don't process */
    if(pcbs->event != NULL) {
	switch (XtHasCallbacks(w,XmNlosingFocusCallback)) {
	case XtCallbackNoList:
	case XtCallbackHasNone:
	  /* No callback installed */
	    XtAddCallback(w,XmNlosingFocusCallback,
	      (XtCallbackProc)textEntryLosingFocusCallback,pte);
	    pte->updateAllowed = False;
	    break;
	case XtCallbackHasSome:
	  /* Callback already installed */
	    break;
	}
      /* Allow the modification that triggered this callback  */
	pcbs->doit = True;
    }
}

static void textEntryValueChanged(Widget  w, XtPointer clientData,
  XtPointer callData)
{
    char *textValue;
    double value;
    long longValue;
    char *end;
    MedmTextEntry *pte = (MedmTextEntry *)clientData;
    DlTextEntry *dlTextEntry = pte->dlElement->structure.textEntry;
    Record *pr = pte->record;
    Boolean match;
    int i;
    int status;

    UNREFERENCED(callData);

    if((pr->connected) && pr->writeAccess) {
	if(!(textValue = XmTextFieldGetString(w))) return;
	switch (pr->dataType) {
	case DBF_STRING:
	    if(strlen(textValue) >= (size_t) MAX_STRING_SIZE)
	      textValue[MAX_STRING_SIZE-1] = '\0';
	    medmSendString(pte->record,textValue);
	    break;
	case DBF_ENUM:
	    if(strlen(textValue) >= (size_t) MAX_STRING_SIZE)
	      textValue[MAX_STRING_SIZE-1] = '\0';
	  /* Check for a match */
	    match = False;
	    for(i = 0; i < pr->hopr+1; i++) {
		if(pr->stateStrings[i]) {
		    if(!strcmp(textValue,pr->stateStrings[i])) {
			medmSendString(pte->record,textValue);
			match = True;
			break;
		    }
		}
	    }
	    if(match == False) {
	      /* Assume it is a number */
		if(dlTextEntry->format == OCTAL) {
		  /* Want to use strtoul, not strtol, because we
                     assume hex and octal entries are unsigned long.
                     It doesn't matter whether the return value is
                     converted to long or unsigned long as the bits
                     are the same for both.  */
		    longValue = strtoul(textValue,&end,8);
		} else if(dlTextEntry->format == HEXADECIMAL) {
		  /* Same comment as above */
		    longValue = strtoul(textValue,&end,16);
		} else {
		    if((strlen(textValue) > (size_t) 2) && (textValue[0] == '0')
		      && (textValue[1] == 'x' || textValue[1] == 'X')) {
			longValue = strtoul(textValue,&end,16);
		    } else {
			longValue = strtol(textValue,&end,10);
		    }
		}
		if(*end == 0 && end != textValue &&
		  longValue >= 0 && longValue <= pr->hopr) {
		    medmSendLong(pte->record,longValue);
		} else {
		    char string[BUFSIZ];

		    sprintf(string,"textEntryValueChanged: Invalid value:\n"
		      "  Name: %s\n  Value: \"%s\"\n"
		      " [Use PV Info to determine valid values]\n",
		      pr->name?pr->name:"NULL",textValue);
		    medmPostMsg(1,string);
		    dmSetAndPopupWarningDialog(currentDisplayInfo,string,
		      "OK",NULL,NULL);
		}
	    }
	    break;
	case DBF_INT:
	case DBF_LONG:
	  /* We assume all hex input is meant to be unsigned
	     long even though there is no DBF_ULONG.
	     Distinguish between long and double because a
	     double converted to a long gives a different
	     result (always 0x7fffffff) than an unsigned long
	     converted to a long when the sign bit is set.  In
	     the unsigned long <-> long conversion, the bits
	     are not changed so either can be used.  What we
	     do insures that for a DBR_LONG record, the result
	     is the same whether the value is entered as hex
	     or not. */
	    if(dlTextEntry->format == OCTAL) {
		longValue = (long)strtoul(textValue,&end,8);
	    } else if(dlTextEntry->format == HEXADECIMAL) {
		longValue = (long)strtoul(textValue,&end,16);
#if DEBUG_STRTOUL
		print("textEntryValueChanged (DBF_LONG): %s %ld\n",
		  textValue, longValue);
#endif
	    } else {
		if((strlen(textValue) > (size_t)2) && (textValue[0] == '0')
		  && (textValue[1] == 'x' || textValue[1] == 'X')) {
		    longValue = (long)strtoul(textValue,&end,16);
		} else {
		  /* Do not use a base of zero.  We want to prevent
                     octal interpretation because of ambiguity
                     problems */
		    longValue = (long)strtoul(textValue,&end,10);
		}
	    }
	    if(*end == '\0' && end != textValue) {
		medmSendLong(pte->record,longValue);
	    } else {
		char string[BUFSIZ];
		sprintf(string,"textEntryValueChanged: Invalid value:\n"
		  "  Name: %s\n  Value: \"%s\"\n",
		  pr->name?pr->name:"NULL",textValue);
		medmPostMsg(1,string);
		dmSetAndPopupWarningDialog(currentDisplayInfo,string,
		  "OK",NULL,NULL);
	    }
	    break;
	case DBF_CHAR:
	    if(pte->dlElement->structure.textEntry->format == STRING) {
		unsigned long len =
		  MIN((unsigned long)pr->elementCount,
		    (unsigned long)(strlen(textValue)+1));
		textValue[len-1] = '\0';
		medmSendCharacterArray(pte->record,textValue,len);
		break;
	    }
	  /* Fall through */
	default:
	  /* Treat as a double */
	    if(dlTextEntry->format == OCTAL) {
		value = (double)strtoul(textValue,&end,8);
	    } else if(dlTextEntry->format == HEXADECIMAL) {
		value = (double)strtoul(textValue,&end,16);
#if DEBUG_STRTOUL
		print("textEntryValueChanged: %s %.1f\n",
		  textValue, value);
#endif
	    } else if(dlTextEntry->format == SEXAGESIMAL) {
		value = strtos(textValue,pr->hopr,pr->lopr,&end,&status);
	    } else if(dlTextEntry->format == SEXAGESIMAL_HMS) {
		value = strtos(textValue,pr->hopr,pr->lopr,&end,&status);
                value *= M_PI/12.0;
	    } else if(dlTextEntry->format == SEXAGESIMAL_DMS) {
		value = strtos(textValue,pr->hopr,pr->lopr,&end,&status);
                value *= M_PI/180.0;
	    } else {
		if((strlen(textValue) > (size_t) 2) && (textValue[0] == '0')
		  && (textValue[1] == 'x' || textValue[1] == 'X')) {
		    value = (double)strtoul(textValue,&end,16);
		} else {
		    value = (double)strtod(textValue,&end);
		}
	    }
	    if(*end == '\0' && end != textValue) {
		medmSendDouble(pte->record,value);
	    } else {
		char string[BUFSIZ];
		sprintf(string,"textEntryValueChanged: Invalid value:\n"
		  "  Name: %s\n  Value: \"%s\"\n",
		  pr->name?pr->name:"NULL",textValue);
		medmPostMsg(1,string);
		dmSetAndPopupWarningDialog(currentDisplayInfo,string,
		  "OK",NULL,NULL);
	    }
	    break;
	}
	XtFree(textValue);
    }
}

static void textEntryUpdateGraphicalInfoCb(XtPointer cd)
{
    Record *pr = (Record *) cd;
    MedmTextEntry *pte = (MedmTextEntry *) pr->clientData;
    DlTextEntry *dlTextEntry = pte->dlElement->structure.textEntry;
    XcVType hopr, lopr, val;
    short precision;


  /* Get values from the record  and adjust them */
    hopr.fval = (float) pr->hopr;
    lopr.fval = (float) pr->lopr;
    val.fval = (float) pr->value;
    if((hopr.fval == 0.0) && (lopr.fval == 0.0)) {
	hopr.fval += 1.0;
    }
    precision = pr->precision;
    if(precision < 0) precision = 0;
    if(precision > 17) precision = 17;

  /* Set lopr and hopr to channel - they aren't used by the TextEntry */
    dlTextEntry->limits.lopr = lopr.fval;
    dlTextEntry->limits.loprChannel = lopr.fval;
    dlTextEntry->limits.hopr = hopr.fval;
    dlTextEntry->limits.hoprChannel = hopr.fval;

  /* Set Channel and User limits for prec (if apparently not set yet) */
    dlTextEntry->limits.precChannel = precision;
    if(dlTextEntry->limits.precSrc != PV_LIMITS_USER &&
      dlTextEntry->limits.precUser == PREC_DEFAULT) {
	dlTextEntry->limits.precUser = precision;
    }
    if(dlTextEntry->limits.precSrc == PV_LIMITS_CHANNEL) {
	dlTextEntry->limits.prec = precision;
    }
}

static void textEntryGetRecord(XtPointer cd, Record **record, int *count)
{
    MedmTextEntry *pte = (MedmTextEntry *) cd;
    *count = 1;
    record[0] = pte->record;
}

DlElement *createDlTextEntry(DlElement *p)
{
    DlTextEntry *dlTextEntry;
    DlElement *dlElement;

    dlTextEntry = (DlTextEntry *)malloc(sizeof(DlTextEntry));
    if(!dlTextEntry) return 0;
    if(p) {
	*dlTextEntry = *(p->structure.textEntry);
    } else {
	objectAttributeInit(&(dlTextEntry->object));
	controlAttributeInit(&(dlTextEntry->control));
	limitsAttributeInit(&(dlTextEntry->limits));
	dlTextEntry->limits.loprSrc0 = PV_LIMITS_UNUSED;
	dlTextEntry->limits.loprSrc = PV_LIMITS_UNUSED;
	dlTextEntry->limits.hoprSrc0 = PV_LIMITS_UNUSED;
	dlTextEntry->limits.hoprSrc = PV_LIMITS_UNUSED;
	dlTextEntry->clrmod = STATIC;
	dlTextEntry->format = MEDM_DECIMAL;
    }

    if(!(dlElement = createDlElement(DL_TextEntry,
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

    if(!dlElement) return 0;
    dlTextEntry = dlElement->structure.textEntry;
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlTextEntry->object));
	    else if(!strcmp(token,"control"))
	      parseControl(displayInfo,&(dlTextEntry->control));
	    else if(!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for(i=FIRST_COLOR_MODE;i<FIRST_COLOR_MODE+NUM_COLOR_MODES;i++) {
		    if(!strcmp(token,stringValueTable[i])) {
			dlTextEntry->clrmod = i;
			break;
		    }
		}
	    } else if(!strcmp(token,"format")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for(i=FIRST_TEXT_FORMAT;i<FIRST_TEXT_FORMAT+NUM_TEXT_FORMATS; i++) {
		    if(!strcmp(token,stringValueTable[i])) {
			dlTextEntry->format = i;
			break;
		    }
		}
	    } else if(!strcmp(token,"limits")) {
		parseLimits(displayInfo,&(dlTextEntry->limits));
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

void writeDlTextEntry(FILE *stream, DlElement *dlElement, int level)
{
    char indent[16];
    DlTextEntry *dlTextEntry = dlElement->structure.textEntry;

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
	fprintf(stream,"\n%s\"text entry\" {",indent);
	writeDlObject(stream,&(dlTextEntry->object),level+1);
	writeDlControl(stream,&(dlTextEntry->control),level+1);
	if(dlTextEntry->clrmod != STATIC)
	  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	    stringValueTable[dlTextEntry->clrmod]);
	if(dlTextEntry->format != MEDM_DECIMAL)
	  fprintf(stream,"\n%s\tformat=\"%s\"",indent,
	    stringValueTable[dlTextEntry->format]);
	writeDlLimits(stream,&(dlTextEntry->limits),level+1);
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
	writeDlLimits(stream,&(dlTextEntry->limits),level+1);
	fprintf(stream,"\n%s}",indent);
    }
#endif
}

static void textEntryInheritValues(ResourceBundle *pRCB, DlElement *p)
{
    DlTextEntry *dlTextEntry = p->structure.textEntry;
    medmGetValues(pRCB,
      CTRL_RC,       &(dlTextEntry->control.ctrl),
      CLR_RC,        &(dlTextEntry->control.clr),
      BCLR_RC,       &(dlTextEntry->control.bclr),
      CLRMOD_RC,     &(dlTextEntry->clrmod),
      FORMAT_RC,     &(dlTextEntry->format),
      LIMITS_RC,     &(dlTextEntry->limits),
      -1);
}

static void textEntryGetLimits(DlElement *pE, DlLimits **ppL, char **pN)
{
    DlTextEntry *dlTextEntry = pE->structure.textEntry;

    *(ppL) = &(dlTextEntry->limits);
    *(pN) = dlTextEntry->control.ctrl;
}

static void textEntryGetValues(ResourceBundle *pRCB, DlElement *p)
{
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
      LIMITS_RC,     &(dlTextEntry->limits),
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
