

#include "medm.h"

extern Atom MEDM_EDIT_FIXED, MEDM_EXEC_FIXED, MEDM_EDIT_SCALABLE,
		MEDM_EXEC_SCALABLE;


/* from the O'Reilly books - this scalable font handling code:
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
Boolean isScalableFont(name) 
char *name; 
{
    int i, field;
    
    if ((name == NULL) || (name[0] != '-')) return False;
    
    for(i = field = 0; name[i] != '\0' && field <= 14; i++) {
	if (name[i] == '-') {
	    field++;
	    if ((field == 7) || (field == 8) || (field == 12))
		if ((name[i+1] != '0') || (name[i+2] != '-'))
		    return False;
	}
    }
    
    if (field != 14) return False;
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
XFontStruct *loadQueryScalableFont(dpy, screen, name, size)
Display *dpy;
int screen;
char *name;
int size;
{
    int i,j, field;
    char newname[500];        /* big enough for a long font name */
    int res_x, res_y;         /* resolution values for this screen */
    
    /* catch obvious errors */
    if ((name == NULL) || (name[0] != '-')) return NULL;
    
    /* calculate our screen resolution in dots per inch. 25.4mm = 1 inch */
    res_x = DisplayWidth(dpy, screen)/(DisplayWidthMM(dpy, screen)/25.4);
    res_y = DisplayHeight(dpy, screen)/(DisplayHeightMM(dpy, screen)/25.4);
    
    /* copy the font name, changing the scalable fields as we do so */
    for(i = j = field = 0; name[i] != '\0' && field <= 14; i++) {
	newname[j++] = name[i];
	if (name[i] == '-') {
	    field++;
	    switch(field) {
	    case 7:  /* pixel size */
		/* change from "-0-" to "-<size>-" */
		(void)sprintf(&newname[j], "%d", size);
		while (newname[j] != '\0') j++;
		if (name[i+1] != '\0') i++;
		break;
	    case 8:  /* point size */
	    case 12: /* average width */
		/* change from "-0-" to "-*-" */
		newname[j] = '*'; 
		j++;
		if (name[i+1] != '\0') i++;
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
    if (field != 14) return NULL;
    
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
        fprintf(stderr,
		"\ngetPixelFromColormapByString:  couldn't allocate color %s",
                colorString);
        return(WhitePixel(display, screen));
    } else {
        return(color.pixel);
    }
}

/*****************************************************************************/







void medmInit(char *displayFont)
{
  Arg args[10];
  int i, n, status;
  char dashList[2];
  Boolean useDefaultFont;
  char warningString[2*MAX_FILE_CHARS];
  char *sizePosition;


  XmRegisterConverters();

/*
 * register action table
 */
  XtAppAddActions(appContext,actions,XtNumber(actions));

/*
 * register a warning handler (catch extraneous warning msgs.)
 */
  XtSetWarningHandler((XtErrorHandler)trapExtraneousWarningsHandler);

/*
 * initialize alarm color array
 */
  alarmColorPixel[NO_ALARM]=getPixelFromColormapByString(display,screenNum,
	cmap,alarmColorString[NO_ALARM]);
  alarmColorPixel[MINOR_ALARM]=getPixelFromColormapByString(display,screenNum,
	cmap,alarmColorString[MINOR_ALARM]);
  alarmColorPixel[MAJOR_ALARM]=getPixelFromColormapByString(display,screenNum,
	cmap,alarmColorString[MAJOR_ALARM]);
  alarmColorPixel[INVALID_ALARM]=getPixelFromColormapByString(display,screenNum,
	cmap,alarmColorString[INVALID_ALARM]);

  

/*
 * initialize Channel Access 
 */
  dmInitializeCA();


/*
 * create an empty (placeholder) ChannelAccessMonitorData for the
 *    channelAccessMonitorListHead and Tail 
 */
  channelAccessMonitorListHead = (ChannelAccessMonitorData *)
	malloc(sizeof(ChannelAccessMonitorData));
  channelAccessMonitorListHead->modified = NOT_MODIFIED;
  channelAccessMonitorListHead->next = NULL;
  channelAccessMonitorListHead->prev = NULL;
  channelAccessMonitorListTail = channelAccessMonitorListHead;

/*
 * likewise for DisplayInfo structures list
 */
  displayInfoListHead = (DisplayInfo *)malloc(sizeof(DisplayInfo));
  displayInfoListHead->next = NULL;
  displayInfoListTail = displayInfoListHead;

/* this routine should be called just once, so do some initialization here */

/*
 * initialize common XmStrings
  */
  dlXmStringOn  = XmStringCreateSimple(ON_STRING);
  dlXmStringOff = XmStringCreateSimple(OFF_STRING);
  dlXmStringNull= XmStringCreateSimple("");
  dlXmStringMoreToComeSymbol = XmStringCreateSimple(MORE_TO_COME_SYMBOL);


/*
 * create the highlight GC
 */
  highlightGC = XCreateGC(display,rootWindow,0,NULL);
  XSetFunction(display,highlightGC,GXxor);
  /* pick a color which XOR-ing with makes reasonable sense for most colors */
  XSetForeground(display,highlightGC,getPixelFromColormapByString(display,
		screenNum,cmap,"grey50"));
  XSetLineAttributes(display,highlightGC,HIGHLIGHT_LINE_THICKNESS,
		LineOnOffDash,CapButt,JoinMiter);
  dashList[0] = 3;
  dashList[1] = 3;
  XSetDashes(display,highlightGC,0,dashList,2);

/*
 * initialize the execute popup menu stuff for all shells
 */
  executePopupMenuButtonType[0] = XmPUSHBUTTON;
  executePopupMenuButtonType[1] = XmPUSHBUTTON;
  executePopupMenuButtons[0] = XmStringCreateSimple(EXECUTE_POPUP_MENU_PRINT);
  executePopupMenuButtons[1] = XmStringCreateSimple(EXECUTE_POPUP_MENU_CLOSE);


/*
 * now load font and fontList tables (but only once)
 */
  if (!strcmp(displayFont,FONT_ALIASES_STRING)) {

/* use the ALIAS fonts if possible */
    strcpy(displayFont,ALIAS_FONT_PREFIX);
    sizePosition = strstr(displayFont,"_");
    fprintf(stderr,"\nMEDM: Loading aliased fonts.");
    for (i = 0; i < MAX_FONTS; i++) {
      sprintf(sizePosition,"_%d",fontSizeTable[i]);
      fontTable[i] = XLoadQueryFont(display,displayFont);
      fprintf(stderr,".");
      if (fontTable[i] == NULL) {
	fprintf(stderr,
	"\nmedmInit: unable to load font %s, trying default (fixed) instead",
	displayFont);
  /* one last attempt: try a common default font */
        fontTable[i] = XLoadQueryFont(display,LAST_CHANCE_FONT);
	if (fontTable[i] == NULL) {
		dmTerminateCA();
		dmTerminateX();
		exit(-1);
	}
       }
      /* now load the XmFontList table for Motif font sizing */
       fontListTable[i] = XmFontListCreate(fontTable[i],
		XmSTRING_DEFAULT_CHARSET);
    }

  } else {

/* try using scalable font - either default or passed in one */

  /* user requested default scalable, copy that name into string and proceed */
    if(!strcmp(displayFont,DEFAULT_SCALABLE_STRING))
	strcpy(displayFont,DEFAULT_SCALABLE_DISPLAY_FONT);

    useDefaultFont = !isScalableFont(displayFont);
    if (useDefaultFont) {
  /* this name wasn't in XLFD format */
        fprintf(stderr,"\nmedmInit: %s%s%s",
		"Invalid scalable display font selected:\n\n  ",
		displayFont,"\n\n(requires XLFD format) using fixed!");
    } else {
	fprintf(stderr,"\nMEDM: Loading scalable fonts.");
    }
    for (i = 0; i < MAX_FONTS; i++) {
      if (!useDefaultFont) {
        fontTable[i] = loadQueryScalableFont(display, screenNum, displayFont,
		fontSizeTable[i]);
	fprintf(stderr,".");
      } else {
        fontTable[i] = XLoadQueryFont(display,LAST_CHANCE_FONT);
      }
      if (fontTable[i] == NULL) {
		dmTerminateCA();
		dmTerminateX();
		exit(-1);
      }
  /* now load the XmFontList table for Motif font sizing */
      fontListTable[i] = XmFontListCreate(fontTable[i],
		XmSTRING_DEFAULT_CHARSET);
    }
  }

  fprintf(stderr,"\n");


}




/*
 * termination of program wrt X; freeing of resources, etc
 */
void dmTerminateX()
{
/* remove the properties on the root window */
  if (MEDM_EDIT_FIXED != (Atom)NULL)
	XDeleteProperty(display,rootWindow,MEDM_EDIT_FIXED);
  if (MEDM_EXEC_FIXED != (Atom)NULL)
	XDeleteProperty(display,rootWindow,MEDM_EXEC_FIXED);
  if (MEDM_EDIT_SCALABLE != (Atom)NULL)
	XDeleteProperty(display,rootWindow,MEDM_EDIT_SCALABLE);
  if (MEDM_EXEC_SCALABLE != (Atom)NULL)
	XDeleteProperty(display,rootWindow,MEDM_EXEC_SCALABLE);

  XFlush(display);

  XtDestroyApplicationContext(appContext);
  XtCloseDisplay(display);
}

