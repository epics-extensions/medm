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

#define DEBUG_FONTS 0
#define DEBUG_HIDE 0
#define DEBUG_BACKGROUND 0

#include <X11/keysym.h>
#include <ctype.h>
#include "medm.h"

typedef struct _MedmText {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           **records;
} MedmText;

static void textUpdateValueCb(XtPointer cd);
static void textDraw(XtPointer cd);
static void textDestroyCb(XtPointer cd);
static void textGetRecord(XtPointer, Record **, int *);
static void textGetValues(ResourceBundle *pRCB, DlElement *p);
static void textInheritValues(ResourceBundle *pRCB, DlElement *p);
static void textSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
#if 0
static void textSetValues(ResourceBundle *pRCB, DlElement *p);
#endif
static void textGetValues(ResourceBundle *pRCB, DlElement *p);

static void drawText(Drawable drawable,  GC gc, DlText *dlText);

static DlDispatchTable textDlDispatchTable = {
    createDlText,
    NULL,
    executeDlText,
    hideDlText,
    writeDlText,
    NULL,
    textGetValues,
    textInheritValues,
    NULL,
    textSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

static void drawText(Drawable drawable,  GC gc, DlText *dlText)
{
    int i = 0, usedWidth, usedHeight;
    int x, y;
    size_t nChars;

#if DEBUG_BACKGROUND
    printf("drawText:\n");
#endif

    nChars = strlen(dlText->textix);
    i = dmGetBestFontWithInfo(fontTable,MAX_FONTS,dlText->textix,
      dlText->object.height,dlText->object.width,
      &usedHeight,&usedWidth,FALSE);
    usedWidth = XTextWidth(fontTable[i],dlText->textix,nChars);

#if DEBUG_FONTS
     {
#if DEBUG_FONTS > 1
	static int ifirst=1;
	int j;

	if(ifirst) {
	    ifirst=0;
	    printf("\nFonts\n");
	    for(j=0; j < MAX_FONTS; j++) {
		printf("%2d widgetDM_%d ascent=%d descent=%d\n",
		  j,fontSizeTable[j],fontTable[j]->ascent,fontTable[j]->descent);
	    }
	    printf("\n");
	}
#endif
	printf("drawText: h=%d %s%d \"%s\"\n",dlText->object.height,
	  ALIAS_FONT_PREFIX,fontSizeTable[i],dlText->textix);
    }
#endif

    XSetFont(display,gc,fontTable[i]->fid);

    y = dlText->object.y + fontTable[i]->ascent;
    switch (dlText->align) {
    case HORIZ_LEFT:
	x = dlText->object.x;
	break;
    case HORIZ_CENTER:
	x = dlText->object.x + (dlText->object.width - usedWidth)/2;
	break;
    case HORIZ_RIGHT:
	x = dlText->object.x + dlText->object.width - usedWidth;
	break;
    }
    XDrawString(display,drawable,gc,x,y,dlText->textix,nChars);

#if DEBUG_HIDE
  /* Goes here because the values are not set until it needs them */
    {
	if(!strcmp(dlText->textix,"Test Display")) {
	    int status;
	    XGCValues values;
	    static Pixel pixel=0;
	    static int num=0;
	    char numString[11];

	    status=XGetGCValues(display,gc,
	      GCForeground|GCClipXOrigin|GCClipYOrigin,
	      &values);
	    print("  drawable=%x\n",drawable);
	    print("  %s GCForeground=%06x "
	      "GCClipXOrigin=%d GCClipYOrigin=%d\n",
	      status?"":"(Failed)",values.foreground,
	      values.clip_x_origin,values.clip_y_origin);

	    pixel+=0xff;
#if 0
	    print(  "Drawing a white rectangle\n");
	    XSetForeground(display,gc,WhitePixel(display,screenNum));
	    XFillRectangle(display,drawable,gc,
	      0,0,1000,1000);
	    XSetForeground(display,gc,values.foreground);
	    XDrawString(display,drawable,gc,x,y,dlText->textix,nChars);
	    XSetForeground(display,gc,0x0000ff);
	    XDrawString(display,drawable,gc,x,y,dlText->textix,nChars);

	    print(  "pixel=%d\n",pixel);
	    XSetForeground(display,gc,pixel);
	    XDrawString(display,drawable,gc,x+5,y,dlText->textix,nChars);
	    XSetForeground(display,gc,values.foreground);
#endif
	    sprintf(numString,"%10d",++num);
	    print(  "num=%s\n",numString);
	    XDrawString(display,drawable,gc,x+5,y,numString,10);
	    XSetForeground(display,gc,values.foreground);
	}
    }
#endif
}

void executeDlText(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlText *dlText = dlElement->structure.text;

#if DEBUG_FONTS  || DEBUG_BACKGROUND
	printf("executeDlText: displayInfo=%x dlElement=%x\n",
	  displayInfo,dlElement);
#endif
#if DEBUG_FONTS > 1
	dumpDlElementList(displayInfo->dlElementList);
#endif

  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(displayInfo->traversalMode == DL_EXECUTE &&
      *dlText->dynAttr.chan[0]) {
	MedmText *pt;

	if(dlElement->data) {
	    pt = (MedmText *)dlElement->data;
	} else {
	    pt = (MedmText *)malloc(sizeof(MedmText));
	    dlElement->updateType = DYNAMIC_GRAPHIC;
	    dlElement->data = (void *)pt;
	    if(pt == NULL) {
		medmPrintf(1,"\nexecuteDlText: Memory allocation error\n");
		return;
	    }
	  /* Pre-initialize */
	    pt->updateTask = NULL;
	    pt->records = NULL;
	    pt->dlElement = dlElement;

	    pt->updateTask = updateTaskAddTask(displayInfo,
	      &(dlText->object), textDraw, (XtPointer)pt);
	    if(pt->updateTask == NULL) {
		medmPrintf(1,"\nexecuteDlText: Memory allocation error\n");
	    } else {
		updateTaskAddDestroyCb(pt->updateTask,textDestroyCb);
		updateTaskAddNameCb(pt->updateTask,textGetRecord);
	    }
	    if(!isStaticDynamic(&dlText->dynAttr, True)) {
		pt->records = medmAllocateDynamicRecords(&dlText->dynAttr,
		  textUpdateValueCb, NULL,(XtPointer) pt);
		calcPostfix(&dlText->dynAttr);
		setDynamicAttrMonitorFlags(&dlText->dynAttr, pt->records);
	    }
	}
    } else {
      /* Static */
	Drawable drawable=updateInProgress?
	  displayInfo->updatePixmap:displayInfo->drawingAreaPixmap;

	dlElement->updateType = STATIC_GRAPHIC;
	executeDlBasicAttribute(displayInfo,&(dlText->attr));
#if DEBUG_FONTS > 1 || DEBUG_HIDE
	printf("executeDlText: Calling drawText PM\n");
#endif
	drawText(drawable, displayInfo->gc, dlText);
    }
}

void hideDlText(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element drawn on the display drawingArea */
    hideDrawnElement(displayInfo, dlElement);
}

static void textUpdateValueCb(XtPointer cd) {
    MedmText *pt = (MedmText *)((Record *) cd)->clientData;
    updateTaskMarkUpdate(pt->updateTask);
}

static void textDraw(XtPointer cd) {
    MedmText *pt = (MedmText *)cd;
    Record *pR = pt->records?pt->records[0]:NULL;
    DisplayInfo *displayInfo = pt->updateTask->displayInfo;
    XGCValues gcValues;
    unsigned long gcValueMask;
    Widget widget = pt->updateTask->displayInfo->drawingArea;
    Display *display = XtDisplay(widget);
    DlText *dlText = pt->dlElement->structure.text;

#if DEBUG_HIDE || DEBUG_BACKGROUND
    printf("textDraw: displayInfo=%x dlElement=%x\n",
      displayInfo,pt->dlElement);
#endif

    if(isConnected(pt->records)) {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	switch (dlText->dynAttr.clr) {
	case STATIC :
	case DISCRETE:
	    gcValues.foreground = displayInfo->colormap[dlText->attr.clr];
	    break;
	case ALARM :
	    gcValues.foreground = alarmColor(pR->severity);
	    break;
	default :
	    gcValues.foreground = displayInfo->colormap[dlText->attr.clr];
	    break;
	}
	gcValues.line_width = dlText->attr.width;
	gcValues.line_style = ((dlText->attr.style == SOLID) ?
	  LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

      /* Draw depending on visibility */
	if(calcVisibility(&dlText->dynAttr, pt->records))
	/* KE: Different drawXXX from other drawing objects */
	  drawText(displayInfo->updatePixmap, displayInfo->gc, dlText);
	if(!pR->readAccess) {
	    drawBlackRectangle(pt->updateTask);
	}
    } else if(isStaticDynamic(&dlText->dynAttr, True)) {
      /* clr and vis are both static */
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	gcValues.foreground = displayInfo->colormap[dlText->attr.clr];
	gcValues.line_width = dlText->attr.width;
	gcValues.line_style = ((dlText->attr.style == SOLID) ?
	  LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
	drawText(displayInfo->updatePixmap, displayInfo->gc, dlText);
    } else {
#if DEBUG_BACKGROUND
	print("  drawWhiteRectangle\n");
#endif
	drawWhiteRectangle(pt->updateTask);
    }
}

static void textDestroyCb(XtPointer cd) {
    MedmText *pt = (MedmText *)cd;

    if(pt) {
	Record **records = pt->records;

	if(records) {
	    int i;
	    for(i=0; i < MAX_CALC_RECORDS; i++) {
		if(records[i]) medmDestroyRecord(records[i]);
	    }
	    free((char *)records);
	}
	if(pt->dlElement) pt->dlElement->data = NULL;
	free((char *)pt);
    }
    return;
}

static void textGetRecord(XtPointer cd, Record **record, int *count) {
    MedmText *pt = (MedmText *)cd;
    int i;

    *count = 0;
    if(pt && pt->records) {
	for(i=0; i < MAX_CALC_RECORDS; i++) {
	    if(pt->records[i]) {
		record[(*count)++] = pt->records[i];
	    }
	}
    }
}

DlElement *createDlText(DlElement *p)
{
    DlText *dlText;
    DlElement *dlElement;

    dlText = (DlText *)malloc(sizeof(DlText));
    if(!dlText) return 0;
    if(p) {
	*dlText = *p->structure.text;
    } else {
	objectAttributeInit(&(dlText->object));
	basicAttributeInit(&(dlText->attr));
	dynamicAttributeInit(&(dlText->dynAttr));
	dlText->textix[0] = '\0';
	dlText->align = HORIZ_LEFT;
    }

    if(!(dlElement = createDlElement(DL_Text,
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

    if(!dlElement) return 0;
    dlText = dlElement->structure.text;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlText->object));
	    } else if(!strcmp(token,"basic attribute"))
	      parseBasicAttribute(displayInfo,&(dlText->attr));
	    else if(!strcmp(token,"dynamic attribute"))
	      parseDynamicAttribute(displayInfo,&(dlText->dynAttr));
	    else if(!strcmp(token,"textix")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlText->textix,token);
	    } else if(!strcmp(token,"align")) {
		int found=0;

		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for(i=FIRST_TEXT_ALIGN;i<FIRST_TEXT_ALIGN+NUM_TEXT_ALIGNS; i++) {
		    if(!strcmp(token,stringValueTable[i])) {
			dlText->align = i;
			found=1;
			break;
		    }
		}
	      /* Backward compatibility */
		if(!found) {
		    if(!strcmp(token,"vert. top")) {
			dlText->align = HORIZ_LEFT;
		    } else if(!strcmp(token,"vert. centered")) {
			dlText->align = HORIZ_CENTER;
		    } else if(!strcmp(token,"vert. bottom")) {
			dlText->align = HORIZ_RIGHT;
		    }
		}
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
    if(MedmUseNewFileFormat) {
#endif
  	fprintf(stream,"\n%stext {",indent);
  	writeDlObject(stream,&(dlText->object),level+1);
  	writeDlBasicAttribute(stream,&(dlText->attr),level+1);
  	writeDlDynamicAttribute(stream,&(dlText->dynAttr),level+1);
  	if(dlText->textix[0] != '\0')
	  fprintf(stream,"\n%s\ttextix=\"%s\"",indent,dlText->textix);
  	if(dlText->align != HORIZ_LEFT)
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

static void blinkCursor(XtPointer client_data, XtIntervalId *id)
{
    static Boolean state = FALSE;

    UNREFERENCED(cd);
    UNREFERENCED(id);

    if(state == TRUE) {
        XDrawLine(display,XtWindow(currentDisplayInfo->drawingArea),
	  currentDisplayInfo->gc,
	  cursorX, cursorY, cursorX + CURSOR_WIDTH, cursorY);
        state = FALSE;
    } else {
        XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
	  XtWindow(currentDisplayInfo->drawingArea),
	  currentDisplayInfo->gc,
	  (int)cursorX, (int)cursorY,
	  (unsigned int)CURSOR_WIDTH + 1u, 1u,
	  (int)cursorX, (int)cursorY);
        state = TRUE;
    }
    intervalId =
      XtAppAddTimeOut(appContext,BLINK_INTERVAL,blinkCursor,NULL);
}

DlElement *handleTextCreate(int x0, int y0)
{
    XEvent event;
    XKeyEvent *key;
    Window window;
    DlElement *dlElement = 0;
    DlText *dlText = 0;
    Modifiers modifiers;
    KeySym keysym;
  /* Buffer size for X-keycode lookups */
#define BUFFER_SIZE     16
    char buffer[BUFFER_SIZE];
    int stringIndex;
    int usedWidth, usedHeight;
    size_t length;
    int fontIndex;

    stringIndex = 0;
    length = 0;

    window = XtWindow(currentDisplayInfo->drawingArea);
    dlElement = createDlText(NULL);
    if(!dlElement) return 0;
    dlText = dlElement->structure.text;
    textGetValues(&globalResourceBundle,dlElement);
    dlText->object.x = x0;
    dlText->object.y = y0;
     /* This following are arbitrary  */
    dlText->object.width = 10;
    if(dlText->object.height > 40) dlText->object.height=40;

  /* Start with dummy string: really just based on character height */
    fontIndex = dmGetBestFontWithInfo(fontTable,MAX_FONTS,"Ag",
      dlText->object.height,dlText->object.width,
      &usedHeight,&usedWidth,FALSE); /* FALSE - don't use width */

    globalResourceBundle.x = x0;
    globalResourceBundle.y = y0;
    globalResourceBundle.width = dlText->object.width;
    globalResourceBundle.height = dlText->object.height;
    cursorX = x0;
    cursorY = y0 + usedHeight;

    intervalId =
      XtAppAddTimeOut(appContext,BLINK_INTERVAL,blinkCursor,NULL);

    XGrabPointer(display,window,FALSE,
      (unsigned int) (ButtonPressMask|LeaveWindowMask),
      GrabModeAsync,GrabModeAsync,None,xtermCursor,CurrentTime);
    XGrabKeyboard(display,window,FALSE,
      GrabModeAsync,GrabModeAsync,CurrentTime);

  /* Now loop until button is again pressed or CR is typed */
    while(TRUE) {
	XtAppNextEvent(appContext,&event);
	switch(event.type) {
        case ButtonPress:
        case LeaveNotify:
	    XUngrabPointer(display,CurrentTime);
	    XUngrabKeyboard(display,CurrentTime);
	    XFlush(display);
	    dlText->object.width = XTextWidth(fontTable[fontIndex],
	      dlText->textix,strlen(dlText->textix));
	    XtRemoveTimeOut(intervalId);
	    XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
	      XtWindow(currentDisplayInfo->drawingArea),
	      currentDisplayInfo->gc,
	      (int)cursorX, (int)cursorY,
	      (unsigned int) CURSOR_WIDTH + 1, (unsigned int)1,
	      (int)cursorX, (int)cursorY);
	    if(event.type == LeaveNotify) {
		XBell(display,50); XBell(display,50);
	    }
	    return (dlElement);

        case KeyPress:
	    key = (XKeyEvent *) &event;
	    XtTranslateKeycode(display,key->keycode,(Modifiers)NULL,
	      &modifiers,&keysym);
          /* if BS or DELETE */
	    if(keysym == osfXK_BackSpace || keysym == osfXK_Delete) {
		if(stringIndex > 0) {
		  /* remove last character */
		    stringIndex--;
		    dlText->textix[stringIndex] = '\0';
		    XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
		      XtWindow(currentDisplayInfo->drawingArea),
		      currentDisplayInfo->gc,
		      (int)dlText->object.x, (int)dlText->object.y,
		      (unsigned int)dlText->object.width,
		      (unsigned int)dlText->object.height,
		      (int)dlText->object.x, (int)dlText->object.y);
		    XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
		      XtWindow(currentDisplayInfo->drawingArea),
		      currentDisplayInfo->gc,
		      (int)cursorX, (int)cursorY,
		      (unsigned int) CURSOR_WIDTH + 1, (unsigned int) 1,
		      (int)cursorX, (int)cursorY);
		} else {
		  /* No more characters to remove therefore wait for next */
		    XBell(display,50);
		    break;
		}
	      /* If Enter, then all done */
	    } else if(keysym == XK_Return) {
		XUngrabPointer(display,CurrentTime);
		XUngrabKeyboard(display,CurrentTime);
		XFlush(display);
		dlText->object.width = XTextWidth(fontTable[fontIndex],
		  dlText->textix,strlen(dlText->textix));
		XtRemoveTimeOut(intervalId);
		XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
		  XtWindow(currentDisplayInfo->drawingArea),
		  currentDisplayInfo->gc,
		  (int)cursorX, (int)cursorY,
		  (unsigned int) CURSOR_WIDTH + 1, (unsigned int) 1,
		  (int)cursorX, (int)cursorY);
		return (dlElement);
	    } else {
		length = XLookupString(key,buffer,BUFFER_SIZE,NULL,NULL);
		if(!isprint(buffer[0]) || length == 0) break;
	      /* Ring bell and don't accept input if going to overflow string */
		if(stringIndex + length < MAX_TOKEN_LENGTH) {
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
                break;
            case HORIZ_CENTER:
                dlText->object.x = x0 - dlText->object.width/2;
                globalResourceBundle.x = dlText->object.x;
                globalResourceBundle.width = dlText->object.width;
                break;
            case HORIZ_RIGHT:
                dlText->object.x = x0 - dlText->object.width;
                globalResourceBundle.x = dlText->object.x;
                globalResourceBundle.width = dlText->object.width;
                break;
	    }
	    XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
	      XtWindow(currentDisplayInfo->drawingArea),
	      currentDisplayInfo->gc,
	      (int)dlText->object.x,
	      (int)dlText->object.y,
	      (unsigned int)dlText->object.width,
	      (unsigned int)dlText->object.height,
	      (int)dlText->object.x, (int)dlText->object.y);
	    XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
	      XtWindow(currentDisplayInfo->drawingArea),
	      currentDisplayInfo->gc,
	      (int)cursorX, (int)cursorY,
	      (unsigned int) CURSOR_WIDTH + 1, (unsigned int) 1,
	      (int)cursorX, (int)cursorY);
	    executeDlBasicAttribute(currentDisplayInfo,&(dlText->attr));
#if DEBUG_FONTS
	    printf("handleTextCreate: Calling drawText\n");
#endif
	    drawText(XtWindow(currentDisplayInfo->drawingArea),
	      currentDisplayInfo->gc,dlText);
	    drawText(currentDisplayInfo->drawingAreaPixmap,
	      currentDisplayInfo->gc,dlText);

	  /* Update these globals for blinking to work */
	    cursorX = dlText->object.x + dlText->object.width;

	    break;
        default:
	    XtDispatchEvent(&event);
	    break;
	}
    }
}

static void textInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlText *dlText = p->structure.text;

    medmGetValues(pRCB,
#if 0
    /* KE: Inheriting these dynamic attribute values for Text is more
     a nuisance than a help */
      CLRMOD_RC,     &(dlText->dynAttr.clr),
      VIS_RC,        &(dlText->dynAttr.vis),
      VIS_CALC_RC,   &(dlText->dynAttr.calc),
      CHAN_A_RC,     &(dlText->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlText->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlText->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlText->dynAttr.chan[3]),
#endif
    /* HEIGHT_RC is for creating text with a click rather than a drag */
      HEIGHT_RC,     &(dlText->object.height),
      CLR_RC,        &(dlText->attr.clr),
      ALIGN_RC,      &(dlText->align),
      -1);
}

#if 0
/* Unused */
static void textSetValues(ResourceBundle *pRCB, DlElement *p) {
    DlText *dlText = p->structure.text;

    medmGetValues(pRCB,
      X_RC,          &(dlText->object.x),
      Y_RC,          &(dlText->object.y),
      WIDTH_RC,      &(dlText->object.width),
      HEIGHT_RC,     &(dlText->object.height),
      CLR_RC,        &(dlText->attr.clr),
      CLRMOD_RC,     &(dlText->dynAttr.clr),
      VIS_RC,        &(dlText->dynAttr.vis),
      VIS_CALC_RC,   &(dlText->dynAttr.calc),
      CHAN_A_RC,     &(dlText->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlText->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlText->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlText->dynAttr.chan[3]),
      ALIGN_RC,      &(dlText->align),
      TEXTIX_RC,     &(dlText->textix),
      -1);
}
#endif

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
      VIS_CALC_RC,   &(dlText->dynAttr.calc),
      CHAN_A_RC,     &(dlText->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlText->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlText->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlText->dynAttr.chan[3]),
      ALIGN_RC,      &(dlText->align),
      TEXTIX_RC,       dlText->textix,
      -1);
}

static void textSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlText *dlText = p->structure.text;
    medmGetValues(pRCB,
      CLR_RC,        &(dlText->attr.clr),
      -1);
}
