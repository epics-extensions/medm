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
a period of five years from Mtexth 30, 1993, the Government is
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

typedef struct _Text {
    DlElement        *dlElement;
    Record           *record;
    UpdateTask       *updateTask;
} Text;

static void textUpdateValueCb(XtPointer cd);
static void textDraw(XtPointer cd);
static void textDestroyCb(XtPointer cd);
static void textName(XtPointer, char **, short *, int *);
static void textGetValues(ResourceBundle *pRCB, DlElement *p);
static void textInheritValues(ResourceBundle *pRCB, DlElement *p);
static void textSetValues(ResourceBundle *pRCB, DlElement *p);
static void textGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable textDlDispatchTable = {
    createDlText,
    destroyElementWithDynamicAttribute,
    executeDlText,
    writeDlText,
    NULL,
    textGetValues,
    textInheritValues,
    NULL,
    NULL,
    genericMove,
    genericScale,
    NULL,
    NULL};

static void drawText(Display *display,
  Drawable drawable,
  GC gc,
  DlText *dlText) {
    int i = 0, usedWidth, usedHeight;
    int x, y;
    size_t nChars;

    nChars = strlen(dlText->textix);
    i = dmGetBestFontWithInfo(fontTable,MAX_FONTS,dlText->textix,
      dlText->object.height,dlText->object.width,
      &usedHeight,&usedWidth,FALSE);
    usedWidth = XTextWidth(fontTable[i],dlText->textix,nChars);

    XSetFont(display,gc,fontTable[i]->fid);

    y = dlText->object.y + fontTable[i]->ascent;
    switch (dlText->align) {
    case HORIZ_LEFT:
    case VERT_TOP:
	x = dlText->object.x;
	break;
    case HORIZ_CENTER:
    case VERT_CENTER:
	x = dlText->object.x + (dlText->object.width - usedWidth)/2;
	break;
    case HORIZ_RIGHT:
    case VERT_BOTTOM:
	x = dlText->object.x + dlText->object.width - usedWidth;
	break;
    }
    XDrawString(display,drawable,gc,x,y,dlText->textix,nChars);
}

void executeDlText(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlText *dlText = dlElement->structure.text;
    if ((displayInfo->traversalMode == DL_EXECUTE) 
      && (dlText->dynAttr.name)){

	Text *pt;
	pt = (Text *) malloc(sizeof(Text));
	pt->dlElement = dlElement;
	pt->updateTask = updateTaskAddTask(displayInfo,
	  &(dlText->object),
	  textDraw,
	  (XtPointer)pt);

	if (pt->updateTask == NULL) {
	    medmPrintf("textCreateRunTimeInstance : memory allocation error\n");
	} else {
	    updateTaskAddDestroyCb(pt->updateTask,textDestroyCb);
	    updateTaskAddNameCb(pt->updateTask,textName);
	    pt->updateTask->opaque = False;
	}
	pt->record = medmAllocateRecord(
	  dlText->dynAttr.name,
	  textUpdateValueCb,
	  NULL,
	  (XtPointer) pt);
	drawWhiteRectangle(pt->updateTask);

#ifdef __COLOR_RULE_H__
	switch (dlText->dynAttr.clr) {
	    STATIC :
	      pt->record->monitorValueChanged = False;
	    pt->record->monitorSeverityChanged = False;
	    break;
	    ALARM :
	      pt->record->monitorValueChanged = False;
	    break;
	    DISCRETE :
	      pt->record->monitorSeverityChanged = False;
	    break;
	}
#else
	pt->record->monitorValueChanged = False;
	if (dlText->dynAttr.clr != ALARM ) {
	    pt->record->monitorSeverityChanged = False;
	}
#endif

	if (dlText->dynAttr.vis == V_STATIC ) {
	    pt->record->monitorZeroAndNoneZeroTransition = False;
	}

    } else {
	executeDlBasicAttribute(displayInfo,&(dlText->attr));
	drawText(display,XtWindow(displayInfo->drawingArea),
	  displayInfo->gc,dlText);
	drawText(display,displayInfo->drawingAreaPixmap,
	  displayInfo->gc,dlText);
    }
}

static void textUpdateValueCb(XtPointer cd) {
    Text *pt = (Text *) ((Record *) cd)->clientData;
    updateTaskMarkUpdate(pt->updateTask);
}

static void textDraw(XtPointer cd) {
    Text *pt = (Text *) cd;
    Record *pd = pt->record;
    DisplayInfo *displayInfo = pt->updateTask->displayInfo;
    XGCValues gcValues;
    unsigned long gcValueMask;
    Widget widget = pt->updateTask->displayInfo->drawingArea;
    Display *display = XtDisplay(widget);
    DlText *dlText = pt->dlElement->structure.text;

    if (pd->connected) {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	switch (dlText->dynAttr.clr) {
#ifdef __COLOR_RULE_H__
	case STATIC :
	    gcValues.foreground = displayInfo->colormap[dlText->attr.clr];
	    break;
	case DISCRETE:
	    gcValues.foreground = extractColor(displayInfo,
	      pd->value,
	      dlText->dynAttr.colorRule,
	      dlText->attr.clr);
	    break;
#else
	case STATIC :
	case DISCRETE:
	    gcValues.foreground = displayInfo->colormap[dlText->attr.clr];
	    break;
#endif
	case ALARM :
	    gcValues.foreground = alarmColorPixel[pd->severity];
	    break;
	default :
	    gcValues.foreground = displayInfo->colormap[dlText->attr.clr];
	    break;
	}
	gcValues.line_width = dlText->attr.width;
	gcValues.line_style = ((dlText->attr.style == SOLID) ? LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

	switch (dlText->dynAttr.vis) {
	case V_STATIC:
	    drawText(display,XtWindow(displayInfo->drawingArea),
	      displayInfo->gc,dlText);
	    break;
	case IF_NOT_ZERO:
	    if (pd->value != 0.0)
	      drawText(display,XtWindow(displayInfo->drawingArea),
		displayInfo->gc,dlText);
	    break;
	case IF_ZERO:
	    if (pd->value == 0.0)
	      drawText(display,XtWindow(displayInfo->drawingArea),
		displayInfo->gc,dlText);
	    break;
	default :
	    medmPrintf("internal error : textUpdateValueCb\n");
	    break;
	}
	if (pd->readAccess) {
	    if (!pt->updateTask->overlapped && dlText->dynAttr.vis == V_STATIC) {
		pt->updateTask->opaque = True;
	    }
	} else {
	    pt->updateTask->opaque = False;
	    draw3DQuestionMark(pt->updateTask);
	}
    } else {
	drawWhiteRectangle(pt->updateTask);
    }
}

static void textDestroyCb(XtPointer cd) {
    Text *pt = (Text *) cd;
    if (pt) {
	medmDestroyRecord(pt->record);
	free((char *)pt);
    }
    return;
}

static void textName(XtPointer cd, char **name, short *severity, int *count) {
    Text *pt = (Text *) cd;
    *count = 1;
    name[0] = pt->record->name;
    severity[0] = pt->record->severity;
}

DlElement *createDlText(DlElement *p)
{
    DlText *dlText;
    DlElement *dlElement;
 
    dlText = (DlText *) malloc(sizeof(DlText));
    if (!dlText) return 0;
    if (p) { 
	*dlText = *p->structure.text;
    } else {
	objectAttributeInit(&(dlText->object));
	basicAttributeInit(&(dlText->attr));
	dynamicAttributeInit(&(dlText->dynAttr));
	dlText->textix[0] = '\0';
	dlText->align = HORIZ_LEFT;
    }
 
    if (!(dlElement = createDlElement(DL_Text,
      (XtPointer)      dlText,
      &textDlDispatchTable))) {
	free(dlText);
    }
 
    return(dlElement);
}

DlElement *parseText(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlText *dlText;
    DlElement *dlElement = createDlText(NULL);
    int i = 0;

    if (!dlElement) return 0;
    dlText = dlElement->structure.text;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlText->object));
	    } else
	      if (!strcmp(token,"basic attribute"))
		parseBasicAttribute(displayInfo,&(dlText->attr));
	      else
		if (!strcmp(token,"dynamic attribute"))
		  parseDynamicAttribute(displayInfo,&(dlText->dynAttr));
		else
		  if (!strcmp(token,"textix")) {
		      getToken(displayInfo,token);
		      getToken(displayInfo,token);
		      strcpy(dlText->textix,token);
		  } else
		    if (!strcmp(token,"align")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			for (i=FIRST_TEXT_ALIGN;i<FIRST_TEXT_ALIGN+NUM_TEXT_ALIGNS; i++) {
			    if (!strcmp(token,stringValueTable[i])) {
				dlText->align = i;
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

void writeDlText(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
    char indent[16];
    DlText *dlText = dlElement->structure.text;

    memset(indent,'\t',level);
    indent[level] = '\0'; 

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
  	fprintf(stream,"\n%stext {",indent);
  	writeDlObject(stream,&(dlText->object),level+1);
  	writeDlBasicAttribute(stream,&(dlText->attr),level+1);
  	writeDlDynamicAttribute(stream,&(dlText->dynAttr),level+1);
  	if (dlText->textix[0] != '\0') 
	  fprintf(stream,"\n%s\ttextix=\"%s\"",indent,dlText->textix);
  	if (dlText->align != HORIZ_LEFT) 
	  fprintf(stream,"\n%s\talign=\"%s\"",indent,stringValueTable[dlText->align]);
  	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
  	writeDlBasicAttribute(stream,&(dlText->attr),level);
  	writeDlDynamicAttribute(stream,&(dlText->dynAttr),level);
  	fprintf(stream,"\n%stext {",indent);
  	writeDlObject(stream,&(dlText->object),level+1);
   	fprintf(stream,"\n%s\ttextix=\"%s\"",indent,dlText->textix);
   	fprintf(stream,"\n%s\talign=\"%s\"",indent,stringValueTable[dlText->align]);
  	fprintf(stream,"\n%s}",indent);
    }
#endif
}

/* some timer (cursor blinking) related functions and globals */
#define BLINK_INTERVAL 700
#define CURSOR_WIDTH 10
 
XtIntervalId intervalId;
int cursorX, cursorY;

#ifdef __cplusplus
static void blinkCursor(
  XtPointer,
  XtIntervalId *)
#else
static void blinkCursor(
  XtPointer client_data,
  XtIntervalId *id)
#endif
{
    static Boolean state = FALSE;
 
    if (state == TRUE) {
        XDrawLine(display,XtWindow(currentDisplayInfo->drawingArea),
	  currentDisplayInfo->gc,
	  cursorX, cursorY, cursorX + CURSOR_WIDTH, cursorY);
        state = FALSE;
    } else {
        XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
	  XtWindow(currentDisplayInfo->drawingArea),
	  currentDisplayInfo->pixmapGC,
	  (int)cursorX, (int)cursorY,
	  (unsigned int) CURSOR_WIDTH + 1,
	  (unsigned int) 1,
	  (int)cursorX, (int)cursorY);
        state = TRUE;
    }
    intervalId =
      XtAppAddTimeOut(appContext,BLINK_INTERVAL,blinkCursor,NULL);
}
 
DlElement *handleTextCreate(
  int x0, int y0)
{
    XEvent event;
    XKeyEvent *key;
    Window window;
    DlElement *dlElement = 0;
    DlText *dlText = 0;
    Modifiers modifiers;
    KeySym keysym;
/* buffer size for X-keycode lookups */
#define BUFFER_SIZE     16
    char buffer[BUFFER_SIZE];
    int stringIndex;
    int usedWidth, usedHeight;
    size_t length;
    DlElement **array;
    int fontIndex;
 
    stringIndex = 0;
    length = 0;
 
    window = XtWindow(currentDisplayInfo->drawingArea);
    dlElement = createDlText(NULL);
    if (!dlElement) return 0;
    dlText = dlElement->structure.text;
    textGetValues(&globalResourceBundle,dlElement);
    dlText->object.x = x0; 
    dlText->object.y = y0;
    dlText->object.width = 10;     /* this is arbitrary in this case */

  /* start with dummy string: really just based on character height */
    fontIndex = dmGetBestFontWithInfo(fontTable,MAX_FONTS,"Ag",
      dlText->object.height,dlText->object.width,
      &usedHeight,&usedWidth,FALSE); /* FALSE - don't use width */
 
    globalResourceBundle.x = x0;
    globalResourceBundle.y = y0;
    globalResourceBundle.width = dlText->object.width;
    cursorX = x0;
    cursorY = y0 + usedHeight;
 
    intervalId =
      XtAppAddTimeOut(appContext,BLINK_INTERVAL,blinkCursor,NULL);
 
    XGrabPointer(display,window,FALSE,
      (unsigned int) (ButtonPressMask|LeaveWindowMask),
      GrabModeAsync,GrabModeAsync,None,xtermCursor,CurrentTime);
    XGrabKeyboard(display,window,FALSE,
      GrabModeAsync,GrabModeAsync,CurrentTime);
 
  /* now loop until button is again pressed and released */
    while (TRUE) {
 
	XtAppNextEvent(appContext,&event);
 
	switch(event.type) {
 
        case ButtonPress:
        case LeaveNotify:
	    XUngrabPointer(display,CurrentTime);
	    XUngrabKeyboard(display,CurrentTime);
	    dlText->object.width = XTextWidth(fontTable[fontIndex],
	      dlText->textix,strlen(dlText->textix));
	    XtRemoveTimeOut(intervalId);
	    XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
	      XtWindow(currentDisplayInfo->drawingArea),
	      currentDisplayInfo->pixmapGC,
	      (int)cursorX, (int)cursorY,
	      (unsigned int) CURSOR_WIDTH + 1, (unsigned int) 1,
	      (int)cursorX, (int)cursorY);
	    XBell(display,50);
	    XBell(display,50);

	    return (dlElement);
 
        case KeyPress:
	    key = (XKeyEvent *) &event;
	    XtTranslateKeycode(display,key->keycode,(Modifiers)NULL,
	      &modifiers,&keysym);
          /* if BS or DELETE */
	    if (keysym == osfXK_BackSpace || keysym == osfXK_Delete) {
		if (stringIndex > 0) {
		  /* remove last character */
		    stringIndex--;
		    dlText->textix[stringIndex] = '\0';
		    XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
		      XtWindow(currentDisplayInfo->drawingArea),
		      currentDisplayInfo->pixmapGC,
		      (int)dlText->object.x, (int)dlText->object.y,
		      (unsigned int)dlText->object.width,
		      (unsigned int)dlText->object.height,
		      (int)dlText->object.x, (int)dlText->object.y);
		    XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
		      XtWindow(currentDisplayInfo->drawingArea),
		      currentDisplayInfo->pixmapGC,
		      (int)cursorX, (int)cursorY,
		      (unsigned int) CURSOR_WIDTH + 1, (unsigned int) 1,
		      (int)cursorX, (int)cursorY);
		} else {
		  /* no more characters  to remove therefore wait for next */
		    XBell(display,50);
		    break;
		}
	    } else {
		length = XLookupString(key,buffer,BUFFER_SIZE,NULL,NULL);
		if (!isprint(buffer[0]) || length == 0) break;
	      /* ring bell and don't accept input if going to overflow string */
		if (stringIndex + length < MAX_TOKEN_LENGTH) {
		    strncpy(&(dlText->textix[stringIndex]),buffer,length);
		    stringIndex += length;
		    dlText->textix[stringIndex] = '\0';
		} else {
		    XBell(display,50);
		    break;
		}
	    }
 
	    dlText->object.width = XTextWidth(fontTable[fontIndex],
	      dlText->textix,strlen(dlText->textix));
 
	    switch (dlText->align) {
            case HORIZ_LEFT:
            case VERT_TOP:
                break;
            case HORIZ_CENTER:
            case VERT_CENTER:
                dlText->object.x = x0 - dlText->object.width/2;
                globalResourceBundle.x = dlText->object.x;
                globalResourceBundle.width = dlText->object.width;
                break;
            case HORIZ_RIGHT:
            case VERT_BOTTOM:
                dlText->object.x = x0 - dlText->object.width;
                globalResourceBundle.x = dlText->object.x;
                globalResourceBundle.width = dlText->object.width;
                break;
	    }
	    XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
	      XtWindow(currentDisplayInfo->drawingArea),
	      currentDisplayInfo->pixmapGC,
	      (int)dlText->object.x,
	      (int)dlText->object.y,
	      (unsigned int)dlText->object.width,
	      (unsigned int)dlText->object.height,
	      (int)dlText->object.x, (int)dlText->object.y);
	    XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
	      XtWindow(currentDisplayInfo->drawingArea),
	      currentDisplayInfo->pixmapGC,
	      (int)cursorX, (int)cursorY,
	      (unsigned int) CURSOR_WIDTH + 1, (unsigned int) 1,
	      (int)cursorX, (int)cursorY);
	    executeDlBasicAttribute(currentDisplayInfo,&(dlText->attr));
	    drawText(display,XtWindow(currentDisplayInfo->drawingArea),
	      currentDisplayInfo->gc,dlText);
 
	  /* update these globals for blinking to work */
	    cursorX = dlText->object.x + dlText->object.width;
 
	    break;
        default:
	    XtDispatchEvent(&event);
	    break;
	}
    }
}

static void textGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlText *dlText = p->structure.text;
    medmGetValues(pRCB,
      X_RC,          &(dlText->object.x),
      Y_RC,          &(dlText->object.y),
      WIDTH_RC,      &(dlText->object.width),
      HEIGHT_RC,     &(dlText->object.height),
      CLR_RC,        &(dlText->attr.clr),
      CLRMOD_RC,     &(dlText->dynAttr.clr),
      VIS_RC,        &(dlText->dynAttr.vis),
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlText->dynAttr.colorRule),
#endif
      CHAN_RC,       &(dlText->dynAttr.name),
      ALIGN_RC,      &(dlText->align),
      TEXTIX_RC,       dlText->textix,
      -1);
}

static void textInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlText *dlText = p->structure.text;
    medmGetValues(pRCB,
      HEIGHT_RC,     &(dlText->object.height),
      CLR_RC,        &(dlText->attr.clr),
      CLRMOD_RC,     &(dlText->dynAttr.clr),
      VIS_RC,        &(dlText->dynAttr.vis),
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlText->dynAttr.colorRule),
#endif
      CHAN_RC,       &(dlText->dynAttr.name),
      ALIGN_RC,      &(dlText->align),
      -1);
}

static void textSetValues(ResourceBundle *pRCB, DlElement *p) {
    DlText *dlText = p->structure.text;
#if 0
    medmSetValues(pRCB,
      X_RC,          p->text->object.x,
      Y_RC,          p->text->object.y,
      WIDTH_RC,      p->text->object.width,
      HEIGHT_RC,     p->text->object.height,
      CLR_RC,        p->text->attr.clr,
      CLRMOD_RC,     p->text->dynAttr.clr,
      VIS_RC,        p->text->dynAttr.vis,
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, p->text->dynAttr.colorRule,
#endif
      CHAN_RC,       &(p->text->dynAttr.name),
      ALIGN_RC,      p->text->align,
      TEXTIX_RC,     p->text->textix,
      -1);
#endif
}
