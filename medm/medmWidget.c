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

#include "medm.h"

/* "Green3", "Yellow", "Red", "White", "Gray80" */
/* The following do not work in Exceed 5 */
/* "#00C000","#FFFF00","#FF0000","#FFFFFF","#CCCCCC" */
static  char *alarmColorString[ALARM_MAX] = {
 "Green3", "Yellow", "Red", "White", "Gray80"
};

/* From the O'Reilly books - this scalable font handling code:
 *   (next two functions)
*/
/*
 * scalefonts.c
 *
 * Written by David Flanagan.  Copyright 1991, O'Reilly && Associates.
 * This program is freely distributable without licensing fees and
 * is provided without guarantee or warranty expressed or implied.
 * This program is -not- in the public domain.
 *
 *  SLIGHTLY modified to use pixel values rather than point sizes (MDA)
 *
 */

/*
 * This routine returns True only if the passed name is a well-formed
 * XLFD style font name with a pixel size, point size, and average
 * width (fields 7,8, and 12) of "0".
 */
Boolean isScalableFont(char *name)
{
    int i, field;

    if((name == NULL) || (name[0] != '-')) return False;

    for(i = field = 0; name[i] != '\0' && field <= 14; i++) {
	if(name[i] == '-') {
	    field++;
	    if((field == 7) || (field == 8) || (field == 12))
	      if((name[i+1] != '0') || (name[i+2] != '-'))
		return False;
	}
    }

    if(field != 14) return False;
    else return True;
}


/*
 * MDA - utilizes pixel, not point size
 *
 * This routine is passed a scalable font name and a PIXEL size.
 * It returns an XFontStruct for the given font scaled to the
 * specified size and the exact resolution of the screen.
 * The font name is assumed to be a well-formed XLFD name,
 * and to have pixel size, point size, and average width fields
 * of "0" and implementation dependent x-resolution and y-
 * resolution fields.  Size is specified in pixels.
 * Returns NULL if the name is malformed or no such font exists.
 */
XFontStruct *loadQueryScalableFont(
  Display *dpy,
  int screen,
  char *name,
  int size)
{
    int i,j, field;
    char newname[500];        /* big enough for a long font name */
    int res_x, res_y;         /* resolution values for this screen */

  /* catch obvious errors */
    if((name == NULL) || (name[0] != '-')) return NULL;

  /* calculate our screen resolution in dots per inch. 25.4mm = 1 inch */
    res_x = (int) (DisplayWidth(dpy, screen)/(DisplayWidthMM(dpy, screen)/25.4));
    res_y = (int) (DisplayHeight(dpy, screen)/
      (DisplayHeightMM(dpy, screen)/25.4));

  /* copy the font name, changing the scalable fields as we do so */
    for(i = j = field = 0; name[i] != '\0' && field <= 14; i++) {
	newname[j++] = name[i];
	if(name[i] == '-') {
	    field++;
	    switch(field) {
	    case 7:  /* pixel size */
	      /* change from "-0-" to "-<size>-" */
		(void)sprintf(&newname[j], "%d", size);
		while(newname[j] != '\0') j++;
		if(name[i+1] != '\0') i++;
		break;
	    case 8:  /* point size */
	    case 12: /* average width */
	      /* change from "-0-" to "-*-" */
		newname[j] = '*';
		j++;
		if(name[i+1] != '\0') i++;
		break;
	    case 9:  /* x resolution */
	    case 10: /* y resolution */
	      /* change from an unspecified resolution to res_x or res_y */
		(void)sprintf(&newname[j], "%d", (field == 9) ? res_x : res_y);
		while(newname[j] != '\0') j++;
		while((name[i+1] != '-') && (name[i+1] != '\0')) i++;
		break;
	    }
	}
    }
    newname[j] = '\0';

  /* if there aren't 14 hyphens, it isn't a well formed name */
    if(field != 14) return NULL;

    return XLoadQueryFont(dpy, newname);
}

/*
 * function to return a pixel value from a colormap by specified color string
 */
unsigned long getPixelFromColormapByString(
  Display *display,
  int screen,
  Colormap cmap,
  char *colorString)
{
    XColor color, ignore;

    if(!XAllocNamedColor(display,cmap,colorString,&color,&ignore)) {
	medmPrintf(1,"\ngetPixelFromColormapByString:"
	  "  Couldn't allocate color %s\n",
	  colorString);
	return(WhitePixel(display, screen));
    } else {
	return(color.pixel);
    }
}

/*****************************************************************************/

void medmInit(char *displayFont)
{
    XmFontListEntry entry;
    int i;
    char dashList[2];
    Boolean useDefaultFont;
    char *sizePosition;

#if 0
  /* KE: This doesn't appear in the Motif documentation.
   *   Assume it is not needed any more. */
    XmRegisterConverters();
#endif

#if 0
  /* Register action table */
    XtAppAddActions(appContext,actions,XtNumber(actions));
#endif

  /* Register a warning handler */
    XtSetWarningHandler((XtErrorHandler)trapExtraneousWarningsHandler);

  /* Initialize alarm color array */
    alarmColorPixel[NO_ALARM]=getPixelFromColormapByString(display,
      screenNum,cmap,alarmColorString[NO_ALARM]);
    alarmColorPixel[MINOR_ALARM]=getPixelFromColormapByString(display,
      screenNum,cmap,alarmColorString[MINOR_ALARM]);
    alarmColorPixel[MAJOR_ALARM]=getPixelFromColormapByString(display,
      screenNum,cmap,alarmColorString[MAJOR_ALARM]);
    alarmColorPixel[INVALID_ALARM]=getPixelFromColormapByString(display,
      screenNum,cmap,alarmColorString[INVALID_ALARM]);
    alarmColorPixel[ALARM_MAX-1]=getPixelFromColormapByString(display,
      screenNum,cmap,alarmColorString[ALARM_MAX-1]);

  /* Initialize Channel Access */
    medmCAInitialize();

  /* Initialize DisplayInfo structures list */
    displayInfoListHead = (DisplayInfo *)malloc(sizeof(DisplayInfo));
    displayInfoListHead->next = NULL;
    displayInfoListTail = displayInfoListHead;

  /* Initialize DisplayInfoSave structures list */
    displayInfoSaveListHead = (DisplayInfo *)malloc(sizeof(DisplayInfo));
    displayInfoSaveListHead->next = NULL;
    displayInfoSaveListTail = displayInfoSaveListHead;

  /* Initialize common XmStrings */
    dlXmStringMoreToComeSymbol = XmStringCreateLocalized(MORE_TO_COME_SYMBOL);

  /* Create the highlight GC */
    highlightGC = XCreateGC(display,rootWindow,0,NULL);
  /* Eliminate events that we do not handle anyway */
    XSetGraphicsExposures(display,highlightGC,False);
  /* Set the function to invert */
    XSetFunction(display,highlightGC,GXinvert);
  /* Pick a color which XOR-ing with makes reasonable sense for most
     colors */
  /* KE: Forground is irrelevant for GXinvert */
    XSetForeground(display,highlightGC,WhitePixel(display,screenNum));
#if 0
    XSetForeground(display,highlightGC,getPixelFromColormapByString(display,
      screenNum,cmap,"grey50"));
#endif
    XSetLineAttributes(display,highlightGC,HIGHLIGHT_LINE_THICKNESS,
      LineOnOffDash,CapButt,JoinMiter);
    dashList[0] = 3;
    dashList[1] = 3;
    XSetDashes(display,highlightGC,0,dashList,2);

/* Initialize the execute popup menu stuff for all shells.  Must be
   consistent with medmWidget.h definitions. */
    executePopupMenuButtonType[0] = XmPUSHBUTTON;
    executePopupMenuButtons[0] =
      XmStringCreateLocalized(EXECUTE_POPUP_MENU_PRINT);

    executePopupMenuButtonType[1] = XmPUSHBUTTON;
    executePopupMenuButtons[1] =
      XmStringCreateLocalized(EXECUTE_POPUP_MENU_CLOSE);

    executePopupMenuButtonType[2] = XmPUSHBUTTON;
    executePopupMenuButtons[2] =
      XmStringCreateLocalized(EXECUTE_POPUP_MENU_PVINFO);

    executePopupMenuButtonType[3] = XmPUSHBUTTON;
    executePopupMenuButtons[3] =
      XmStringCreateLocalized(EXECUTE_POPUP_MENU_PVLIMITS);

    executePopupMenuButtonType[4] = XmPUSHBUTTON;
    executePopupMenuButtons[4] =
      XmStringCreateLocalized(EXECUTE_POPUP_MENU_MAIN);

    executePopupMenuButtonType[5] = XmPUSHBUTTON;
    executePopupMenuButtons[5] =
      XmStringCreateLocalized(EXECUTE_POPUP_MENU_DISPLAY_LIST);

   executePopupMenuButtonType[6] = XmPUSHBUTTON;
    executePopupMenuButtons[6] =
      XmStringCreateLocalized(EXECUTE_POPUP_MENU_FLASH_HIDDEN);

    executePopupMenuButtonType[7] = XmPUSHBUTTON;
    executePopupMenuButtons[7] =
      XmStringCreateLocalized(EXECUTE_POPUP_MENU_REFRESH);

    executePopupMenuButtonType[8] = XmPUSHBUTTON;
    executePopupMenuButtons[8] =
      XmStringCreateLocalized(EXECUTE_POPUP_MENU_RETRY);

  /* Note that the Execute Menu is a cascade button */
    executePopupMenuButtonType[9] = XmCASCADEBUTTON;
    executePopupMenuButtons[9] =
      XmStringCreateLocalized(EXECUTE_POPUP_MENU_EXECUTE);

  /* Load font and fontList tables (but only once) */
    if(!strcmp(displayFont,FONT_ALIASES_STRING)) {

      /* Use the ALIAS fonts if possible */
	strcpy(displayFont,ALIAS_FONT_PREFIX);
	sizePosition = strstr(displayFont,"_");
	printf("\n%s: Loading aliased fonts.",MEDM_VERSION_STRING);
	for(i = 0; i < MAX_FONTS; i++) {
	    sprintf(sizePosition,"_%d",fontSizeTable[i]);
	    fontTable[i] = XLoadQueryFont(display,displayFont);
	    printf(".");
	    if(fontTable[i] == NULL) {
		medmPrintf(1,"\nmedmInit: Unable to load font %s\n"
		  "  Trying default (fixed) instead\n",
		  displayFont);
	      /* one last attempt: try a common default font */
		fontTable[i] = XLoadQueryFont(display,LAST_CHANCE_FONT);
		if(fontTable[i] == NULL) {
		    medmCATerminate();
		    dmTerminateX();
		    exit(-1);
		}
	    }
	  /* Load the XmFontList table for Motif font sizing */
	    entry = XmFontListEntryCreate(XmFONTLIST_DEFAULT_TAG, XmFONT_IS_FONT,
	      (XtPointer)fontTable[i]);
	    fontListTable[i] = XmFontListAppendEntry(NULL, entry);
	    XmFontListEntryFree(&entry);
	}

    } else {
      /* Try using scalable font - either default or passed in one */
      /* User requested default scalable, copy that name into string
         and proceed */
	if(!strcmp(displayFont,DEFAULT_SCALABLE_STRING))
	  strcpy(displayFont,DEFAULT_SCALABLE_DISPLAY_FONT);

	useDefaultFont = !isScalableFont(displayFont);
	if(useDefaultFont) {
	  /* This name wasn't in XLFD format */
	    medmPrintf(1,"\nmedmInit:"
	      "  Invalid scalable display font selected  (Not in XLFD format)\n"
	      "    font: %s\n"
	      "  Using fixed font\n",displayFont);
	} else {
	    printf("\n%s: Loading scalable fonts.",MEDM_VERSION_STRING);
	}
	for(i = 0; i < MAX_FONTS; i++) {
	    if(!useDefaultFont) {
		fontTable[i] = loadQueryScalableFont(display, screenNum,
		  displayFont, fontSizeTable[i]);
		printf(".");
	    } else {
		fontTable[i] = XLoadQueryFont(display,LAST_CHANCE_FONT);
	    }
	    if(fontTable[i] == NULL) {
		medmCATerminate();
		dmTerminateX();
		exit(-1);
	    }
	  /* Load the XmFontList table for Motif font sizing */
	    entry = XmFontListEntryCreate(XmFONTLIST_DEFAULT_TAG, XmFONT_IS_FONT,
	      (XtPointer)fontTable[i]);
	    fontListTable[i] = XmFontListAppendEntry(NULL, entry);
	    XmFontListEntryFree(&entry);
	}
    }
    printf("\n");
}

/*
 * termination of program wrt X; freeing of resources, etc
 */
void dmTerminateX()
{
/* Remove the properties on the root window */
    if(windowPropertyAtom != (Atom)NULL)
      XDeleteProperty(display,rootWindow,windowPropertyAtom);
    XFlush(display);

    XtDestroyApplicationContext(appContext);
    XtCloseDisplay(display);
}

int initMedmWidget() {
    if(clipboard) return 0;
    clipboard = createDlList();
    if(clipboard) {
	return 0;
    } else {
	return -1;
    }
}

/* KE: This is only called when MEDM is exiting
 *   It destroys the DisplayInfoListHead but not the DisplayInfo's
 *   It would need to do dmRemoveDisplayInfo if it did
 *   It probably is not necessary */
int destroyMedmWidget() {
    int i;

    if(displayInfoListHead) free((char *)displayInfoListHead);
    if(displayInfoSaveListHead) free((char *)displayInfoSaveListHead);
    if(dlXmStringMoreToComeSymbol) XmStringFree(dlXmStringMoreToComeSymbol);
    for(i=0; i < NUM_EXECUTE_POPUP_ENTRIES; i++) {
	if(executePopupMenuButtons[i]) XmStringFree(executePopupMenuButtons[i]);
    }
    if(highlightGC) XFreeGC(display,highlightGC);
    if(clipboard) {
	clearDlDisplayList(NULL, clipboard);
	free(clipboard);
    }
    return 0;
}

void addWidgetEvents(Widget w)
{
}

void moveDisplayInfoToDisplayInfoSave(DisplayInfo *displayInfo)
{
    DisplayInfo *di;
    char *filename = displayInfo->dlFile->name;

  /* Check if saving is necessary (because started in EDIT mode) */
    if(!saveReplacedDisplays) {
	dmRemoveDisplayInfo(displayInfo);
	return;
    }

  /* Check if it is already there */
    di = displayInfoSaveListHead->next;
    while(di) {
	if(!strcmp(filename, di->dlFile->name)) {
	  /* Already there, remove it */
	    dmRemoveDisplayInfo(displayInfo);
	    return;
	}
	di = di->next;
    }

  /* Remove it from the displayInfo list */
    displayInfo->prev->next = displayInfo->next;
    if(displayInfo->next != NULL)
      displayInfo->next->prev = displayInfo->prev;
    if(displayInfoListTail == displayInfo)
      displayInfoListTail = displayInfoListTail->prev;
    if(displayInfoListTail == displayInfoListHead)
      displayInfoListHead->next = NULL;

  /* Append it to the displayInfoSave List */
    displayInfoSaveListTail->next = displayInfo;
    displayInfo->prev = displayInfoSaveListTail;
    displayInfo->next = NULL;
    displayInfoSaveListTail = displayInfo;
}

void moveDisplayInfoSaveToDisplayInfo(DisplayInfo *displayInfo)
{
  /* Remove it from the displayInfoSave list */
    displayInfo->prev->next = displayInfo->next;
    if(displayInfo->next != NULL)
      displayInfo->next->prev = displayInfo->prev;
    if(displayInfoSaveListTail == displayInfo)
      displayInfoSaveListTail = displayInfoSaveListTail->prev;
    if(displayInfoSaveListTail == displayInfoSaveListHead)
      displayInfoSaveListHead->next = NULL;

  /* Append it to the displayInfo List */
    displayInfoListTail->next = displayInfo;
    displayInfo->prev = displayInfoListTail;
    displayInfo->next = NULL;
    displayInfoListTail = displayInfo;
}
