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

/*****************************************************************************
 ******				 createStatics.c			******
 *****************************************************************************/

#include "medm.h"

#define DEFAULT_DISPLAY_X	10
#define DEFAULT_DISPLAY_Y	10

/* some timer (cursor blinking) related functions and globals */
#define BLINK_INTERVAL 700
#define CURSOR_WIDTH 10

XtIntervalId intervalId;
int cursorX, cursorY;


/**********************************************************************
 *********    nested objects (not to be put in display list)  *********
 **********************************************************************/




void createDlObject(
  DisplayInfo *displayInfo,
  DlObject *object)
{
  object->x = globalResourceBundle.x;
  object->y = globalResourceBundle.y;
  object->width = globalResourceBundle.width;
  object->height = globalResourceBundle.height;
}




static void createDlDynAttrMod(
  DisplayInfo *displayInfo,
  DlDynamicAttrMod *dynAttr)
{
  dynAttr->clr = globalResourceBundle.clrmod;
  dynAttr->vis = globalResourceBundle.vis;
}



static void createDlDynAttrParam(
  DisplayInfo *displayInfo,
  DlDynamicAttrParam *dynAttr)
{
  strcpy(dynAttr->chan,globalResourceBundle.chan);
}


static void createDlAttr(
  DisplayInfo *displayInfo,
  DlAttribute *attr)
{
  attr->clr = globalResourceBundle.clr;
  attr->style = globalResourceBundle.style;
  attr->fill = globalResourceBundle.fill;
  attr->width = globalResourceBundle.lineWidth;
}


static void createDlDynamicAttr(
  DisplayInfo *displayInfo,
  DlDynamicAttributeData *dynAttr)
{
  createDlDynAttrMod(displayInfo,&(dynAttr->mod));
  createDlDynAttrParam(displayInfo,&(dynAttr->param));
}


static void createDlRelatedDisplayEntry(
  DisplayInfo *displayInfo,
  DlRelatedDisplayEntry *relatedDisplay,
  int displayNumber)
{
/* structure copy */
  *relatedDisplay = globalResourceBundle.rdData[displayNumber];
}



static void createDlShellCommandEntry(
  DisplayInfo *displayInfo,
  DlShellCommandEntry *shellCommand,
  int cmdNumber)
{
/* structure copy */
  *shellCommand = globalResourceBundle.cmdData[cmdNumber];

}


static XtTimerCallbackProc blinkCursor(
  XtPointer client_data,
  XtIntervalId *id)
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
  intervalId = XtAppAddTimeOut(appContext,BLINK_INTERVAL,
		(XtTimerCallbackProc)blinkCursor,NULL);
}




DlElement *handleTextCreate(
  int x0, int y0)
{
  XEvent event;
  XKeyEvent *key;
  Window window;
  DlText dlText;
  DlElement *element;
  Modifiers modifiers;
  KeySym keysym;
/* buffer size for X-keycode lookups */
#define BUFFER_SIZE	16
  char buffer[BUFFER_SIZE];
  int stringIndex, newX, newY;
  int usedWidth, usedHeight;
  size_t length;
  DlElement **array;
  int fontIndex;

  stringIndex = 0;
  length = 0;

/* create a basic attribute */
  element = createDlBasicAttribute(currentDisplayInfo);
  (*element->dmExecute)(currentDisplayInfo,
                        element->structure.basicAttribute,FALSE);
/* create a dynamic attribute if appropriate */
  if (strlen(globalResourceBundle.chan) > 0 &&
      globalResourceBundle.vis != V_STATIC) {
    element = createDlDynamicAttribute(currentDisplayInfo);
    (*element->dmExecute)(currentDisplayInfo,
                        element->structure.dynamicAttribute,FALSE);
  }


  window = XtWindow(currentDisplayInfo->drawingArea);
  element = (DlElement *) NULL;
  dlText.object.x = x0;
  dlText.object.y = y0;
  dlText.object.width = 10;		/* this is arbitrary in this case */
  dlText.object.height = globalResourceBundle.height;
  dlText.align = globalResourceBundle.align;
  dlText.textix[0] = '\0';

  /* start with dummy string: really just based on character height */
  fontIndex = dmGetBestFontWithInfo(fontTable,MAX_FONTS,"Ag",
	dlText.object.height,dlText.object.width,
	&usedHeight,&usedWidth,FALSE); /* FALSE - don't use width */

  globalResourceBundle.x = x0;
  globalResourceBundle.y = y0;
  globalResourceBundle.width = dlText.object.width;
  cursorX = x0;
  cursorY = y0 + usedHeight;

  intervalId = XtAppAddTimeOut(appContext,BLINK_INTERVAL,
		(XtTimerCallbackProc)blinkCursor,NULL);

  XGrabPointer(display,window,FALSE,
	ButtonPressMask|LeaveWindowMask,
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
	  dlText.object.width = XTextWidth(fontTable[fontIndex],
		dlText.textix,strlen(dlText.textix));

	  /* update global resource bundle  - then do create out of it */
	  strcpy(globalResourceBundle.textix,dlText.textix);
	  globalResourceBundle.width = dlText.object.width;
	  element = createDlText(currentDisplayInfo);
	  (*element->dmExecute)(currentDisplayInfo,
			element->structure.text,FALSE);
	  XtRemoveTimeOut(intervalId);
	  XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
			XtWindow(currentDisplayInfo->drawingArea),
			currentDisplayInfo->pixmapGC,
			(int)cursorX, (int)cursorY,
			(unsigned int) CURSOR_WIDTH + 1, (unsigned int) 1,
			(int)cursorX, (int)cursorY);
	  XBell(display,50);
	  XBell(display,50);

	/* now select and highlight this element */
	  highlightAndSetSelectedElements(NULL,0,0);
	  clearResourcePaletteEntries();
	  array = (DlElement **) malloc(1*sizeof(DlElement *));
	  array[0] = element;
	  highlightAndSetSelectedElements(array,1,1);
	  setResourcePaletteEntries();

	  return (element);
	  break;

	case KeyPress:
	  key = (XKeyEvent *) &event;
	  XtTranslateKeycode(display,key->keycode,(Modifiers)NULL,
			&modifiers,&keysym);
	/* if BS or DELETE */
	  if (keysym == osfXK_BackSpace || keysym == osfXK_Delete) {
	    if (stringIndex > 0) {
	/* remove last character */
	      stringIndex--;
	      dlText.textix[stringIndex] = '\0';
	      XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
		   XtWindow(currentDisplayInfo->drawingArea),
		   currentDisplayInfo->pixmapGC,
		   (int)dlText.object.x,
		   (int)dlText.object.y - fontTable[fontIndex]->ascent,
		   (unsigned int)dlText.object.width,
		   (unsigned int)dlText.object.height,
		   (int)dlText.object.x, (int)dlText.object.y);
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
	      strncpy(&(dlText.textix[stringIndex]),buffer,length);
	      stringIndex += length;
	      dlText.textix[stringIndex] = '\0';
	    } else {
	      XBell(display,50);
	      break;
	    }
	  }

/* want to force display to window (keeping background intact) *
 * what we do here is a function of the alignment attribute */
/* (MDA) vertical text not supported! */

	  dlText.object.width = XTextWidth(fontTable[fontIndex],
			dlText.textix,strlen(dlText.textix));

	  switch (dlText.align) {
	    case HORIZ_LEFT:
	    case VERT_TOP:
		break;
	    case HORIZ_CENTER:
	    case VERT_CENTER:
		dlText.object.x = x0 - dlText.object.width/2;
		globalResourceBundle.x = dlText.object.x;
		globalResourceBundle.width = dlText.object.width;
		break;
	    case HORIZ_RIGHT:
	    case VERT_BOTTOM:
		dlText.object.x = x0 - dlText.object.width;
		globalResourceBundle.x = dlText.object.x;
		globalResourceBundle.width = dlText.object.width;
		break;
	  }
	  XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
		   XtWindow(currentDisplayInfo->drawingArea),
		   currentDisplayInfo->pixmapGC,
		   (int)dlText.object.x,
		   (int)dlText.object.y - fontTable[fontIndex]->ascent,
		   (unsigned int)dlText.object.width,
		   (unsigned int)dlText.object.height,
		   (int)dlText.object.x, (int)dlText.object.y);
	  XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
		   XtWindow(currentDisplayInfo->drawingArea),
		   currentDisplayInfo->pixmapGC,
		   (int)cursorX, (int)cursorY,
		   (unsigned int) CURSOR_WIDTH + 1, (unsigned int) 1,
		   (int)cursorX, (int)cursorY);
	  executeDlText(currentDisplayInfo,&dlText,TRUE);

       /* update these globals for blinking to work */
	  cursorX = dlText.object.x + dlText.object.width;

	  break;


	default:
	  XtDispatchEvent(&event);
	  break;
     }
  }

}







DlElement *createDlFile(
  DisplayInfo *displayInfo)
{
  DlFile *dlFile;
  DlElement *dlElement;

  dlFile = (DlFile *) malloc(sizeof(DlFile));
  strcpy(dlFile->name,globalResourceBundle.name);

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_File;
  dlElement->structure.file = dlFile;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlFile;
  dlElement->dmWrite = (void(*)())writeDlFile;

  return(dlElement);
}


DlElement *createDlDisplay(
  DisplayInfo *displayInfo)
{
  DlDisplay *dlDisplay;
  DlElement *dlElement;


  dlDisplay = (DlDisplay *) calloc(1,sizeof(DlDisplay));
  createDlObject(displayInfo,&(dlDisplay->object));
  if (dlDisplay->object.x <= 0) dlDisplay->object.x = DEFAULT_DISPLAY_X;
  if (dlDisplay->object.y <= 0) dlDisplay->object.y = DEFAULT_DISPLAY_Y;
  strcpy(dlDisplay->cmap,globalResourceBundle.cmap);
  dlDisplay->bclr = globalResourceBundle.bclr;
  dlDisplay->clr = globalResourceBundle.clr;

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Display;
  dlElement->structure.display = dlDisplay;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlDisplay;
  dlElement->dmWrite = (void(*)())writeDlDisplay;

  return(dlElement);
}



DlElement *createDlColormap(
  DisplayInfo *displayInfo)
{
  DlColormap *dlColormap;
  DlElement *dlElement;
  DlColormapEntry *dlColor;
  int counter;


  dlColormap = (DlColormap *) malloc(sizeof(DlColormap));
  /* structure copy */
  *dlColormap = defaultDlColormap;

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Colormap;
  dlElement->structure.colormap = dlColormap;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlColormap;
  dlElement->dmWrite = (void(*)())writeDlColormap;

  displayInfo->dlColormapElement = dlElement;

  return(dlElement);
}



DlElement *createDlBasicAttribute(
  DisplayInfo *displayInfo)
{
  DlBasicAttribute *dlBasicAttribute;
  DlElement *dlElement;

  dlBasicAttribute = (DlBasicAttribute *) malloc(sizeof(DlBasicAttribute));
  createDlAttr(displayInfo,&(dlBasicAttribute->attr));

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_BasicAttribute;
  dlElement->structure.basicAttribute = dlBasicAttribute;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlBasicAttribute;
  dlElement->dmWrite = (void(*)())writeDlBasicAttribute;

  return(dlElement);
}



DlElement *createDlDynamicAttribute(
  DisplayInfo *displayInfo)
{
  DlDynamicAttribute *dlDynamicAttribute;
  DlElement *dlElement;

  dlDynamicAttribute = (DlDynamicAttribute *)
		malloc(sizeof(DlDynamicAttribute));
  createDlDynamicAttr(displayInfo,&(dlDynamicAttribute->attr));

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_DynamicAttribute;
  dlElement->structure.dynamicAttribute = dlDynamicAttribute;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlDynamicAttribute;
  dlElement->dmWrite = (void(*)())writeDlDynamicAttribute;

  return(dlElement);
}





DlElement *createDlRectangle(
  DisplayInfo *displayInfo)
{
  DlRectangle *dlRectangle;
  DlElement *dlElement;

  dlRectangle = (DlRectangle *) malloc(sizeof(DlRectangle));
  createDlObject(displayInfo,&(dlRectangle->object));

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Rectangle;
  dlElement->structure.rectangle = dlRectangle;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlRectangle;
  dlElement->dmWrite = (void(*)())writeDlRectangle;

  return(dlElement);
}


DlElement *createDlOval(
  DisplayInfo *displayInfo)
{
  DlOval *dlOval;
  DlElement *dlElement;

  dlOval = (DlOval *) malloc(sizeof(DlOval));
  createDlObject(displayInfo,&(dlOval->object));

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Oval;
  dlElement->structure.oval = dlOval;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlOval;
  dlElement->dmWrite = (void(*)())writeDlOval;

  return(dlElement);
}



DlElement *createDlArc(
  DisplayInfo *displayInfo)
{
  DlArc *dlArc;
  DlElement *dlElement;

  dlArc = (DlArc*) malloc(sizeof(DlArc));
  createDlObject(displayInfo,&(dlArc->object));
  dlArc->begin = globalResourceBundle.begin;
  dlArc->path = globalResourceBundle.path;


  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Arc;
  dlElement->structure.arc = dlArc;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlArc;
  dlElement->dmWrite = (void(*)())writeDlArc;

  return(dlElement);
}


DlElement *createDlText(
  DisplayInfo *displayInfo)
{
  DlText *dlText;
  DlElement *dlElement;


  dlText = (DlText *) malloc(sizeof(DlText));
  createDlObject(displayInfo,&(dlText->object));
  strcpy(dlText->textix,globalResourceBundle.textix);
  dlText->align = globalResourceBundle.align;

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Text;
  dlElement->structure.text = dlText;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlText;
  dlElement->dmWrite = (void(*)())writeDlText;

  return(dlElement);
}



DlElement *createDlFallingLine(
  DisplayInfo *displayInfo)
{
  DlFallingLine *dlFallingLine;
  DlElement *dlElement;

  dlFallingLine = (DlFallingLine *) malloc(sizeof(DlFallingLine));
  createDlObject(displayInfo,&(dlFallingLine->object));

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_FallingLine;
  dlElement->structure.fallingLine = dlFallingLine;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlFallingLine;
  dlElement->dmWrite = (void(*)())writeDlFallingLine;

  return(dlElement);
}



DlElement *createDlRisingLine(
  DisplayInfo *displayInfo)
{
  DlRisingLine *dlRisingLine;
  DlElement *dlElement;

  dlRisingLine = (DlRisingLine*) malloc(sizeof(DlRisingLine));
  createDlObject(displayInfo,&(dlRisingLine->object));

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_RisingLine;
  dlElement->structure.risingLine = dlRisingLine;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlRisingLine;
  dlElement->dmWrite = (void(*)())writeDlRisingLine;

  return(dlElement);
}





DlElement *createDlRelatedDisplay(
  DisplayInfo *displayInfo)
{
  DlRelatedDisplay *dlRelatedDisplay;
  DlElement *dlElement;
  int displayNumber;

  dlRelatedDisplay = (DlRelatedDisplay *) malloc(sizeof(DlRelatedDisplay));
  createDlObject(displayInfo,&(dlRelatedDisplay->object));
  for (displayNumber = 0; displayNumber < MAX_RELATED_DISPLAYS;displayNumber++)
	createDlRelatedDisplayEntry(displayInfo,
			&(dlRelatedDisplay->display[displayNumber]),
			displayNumber );
  dlRelatedDisplay->clr = globalResourceBundle.clr;
  dlRelatedDisplay->bclr = globalResourceBundle.bclr;


  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_RelatedDisplay;
  dlElement->structure.relatedDisplay = dlRelatedDisplay;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlRelatedDisplay;
  dlElement->dmWrite = (void(*)())writeDlRelatedDisplay;

  return(dlElement);
}






DlElement *createDlShellCommand(
  DisplayInfo *displayInfo)
{
  DlShellCommand *dlShellCommand;
  DlElement *dlElement;
  int cmdNumber;

  dlShellCommand = (DlShellCommand *) malloc(sizeof(DlShellCommand));
  createDlObject(displayInfo,&(dlShellCommand->object));
  for (cmdNumber = 0; cmdNumber < MAX_SHELL_COMMANDS;cmdNumber++)
	createDlShellCommandEntry(displayInfo,
			&(dlShellCommand->command[cmdNumber]),
			cmdNumber );
  dlShellCommand->clr = globalResourceBundle.clr;
  dlShellCommand->bclr = globalResourceBundle.bclr;


  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_ShellCommand;
  dlElement->structure.shellCommand = dlShellCommand;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute = (void(*)())executeDlShellCommand;
  dlElement->dmWrite = (void(*)())writeDlShellCommand;

  return(dlElement);
}



