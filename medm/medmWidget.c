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

/* "green", "yellow", "red", "white", */
static  char *alarmColorString[] = {"#00C000",
				    "#FFFF00","#FF0000","#FFFFFF",};

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
    if ((name == NULL) || (name[0] != '-')) return NULL;
    
  /* calculate our screen resolution in dots per inch. 25.4mm = 1 inch */
    res_x = (int) (DisplayWidth(dpy, screen)/(DisplayWidthMM(dpy, screen)/25.4));
    res_y = (int) (DisplayHeight(dpy, screen)/(DisplayHeightMM(dpy, screen)/25.4));
    
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
	medmPrintf("\ngetPixelFromColormapByString:  Couldn't allocate color %s\n",
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
    int i;
    char dashList[2];
    Boolean useDefaultFont;
    char warningString[2*MAX_FILE_CHARS];
    char *sizePosition;


    XmRegisterConverters();

/*
 * register action table
 */
#if 0
    XtAppAddActions(appContext,actions,XtNumber(actions));
#endif

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
    medmCAInitialize();

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
    dlXmStringMoreToComeSymbol = XmStringCreateLocalized(MORE_TO_COME_SYMBOL);

/*
 * create the highlight GC
 */
    highlightGC = XCreateGC(display,rootWindow,0,NULL);
    XSetFunction(display,highlightGC,GXinvert);
  /* Pick a color which XOR-ing with makes reasonable sense for most colors */
  /* KE: Forgroung is irrelevant for GXinvert */
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

/*
 * initialize the execute popup menu stuff for all shells
 */
    executePopupMenuButtonType[0] = XmPUSHBUTTON;
    executePopupMenuButtonType[1] = XmPUSHBUTTON;
    executePopupMenuButtons[0] = XmStringCreateLocalized(EXECUTE_POPUP_MENU_PRINT);
    executePopupMenuButtons[1] = XmStringCreateLocalized(EXECUTE_POPUP_MENU_CLOSE);

/*
 * now load font and fontList tables (but only once)
 */
    if (!strcmp(displayFont,FONT_ALIASES_STRING)) {

/* use the ALIAS fonts if possible */
	strcpy(displayFont,ALIAS_FONT_PREFIX);
	sizePosition = strstr(displayFont,"_");
	printf("\n%s: Loading aliased fonts.",MEDM_VERSION_STRING);
	for (i = 0; i < MAX_FONTS; i++) {
	    sprintf(sizePosition,"_%d",fontSizeTable[i]);
	    fontTable[i] = XLoadQueryFont(display,displayFont);
	    printf(".");
	    if (fontTable[i] == NULL) {
		medmPrintf("\nmedmInit: Unable to load font %s\n  Trying default (fixed) instead\n",
		  displayFont);
	      /* one last attempt: try a common default font */
		fontTable[i] = XLoadQueryFont(display,LAST_CHANCE_FONT);
		if (fontTable[i] == NULL) {
		    medmCATerminate();
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
	    medmPrintf("\nmedmInit: Invalid scalable display font selected  (Not in XLFD format)\n"
	    "  font: %s\n"
	    "  Using fixed font\n",displayFont);
	} else {
	    printf("\n%s: Loading scalable fonts.",MEDM_VERSION_STRING);
	}
	for (i = 0; i < MAX_FONTS; i++) {
	    if (!useDefaultFont) {
		fontTable[i] = loadQueryScalableFont(display, screenNum, displayFont,
		  fontSizeTable[i]);
		printf(".");
	    } else {
		fontTable[i] = XLoadQueryFont(display,LAST_CHANCE_FONT);
	    }
	    if (fontTable[i] == NULL) {
		medmCATerminate();
		dmTerminateX();
		exit(-1);
	    }
	  /* now load the XmFontList table for Motif font sizing */
	    fontListTable[i] = XmFontListCreate(fontTable[i],
	      XmSTRING_DEFAULT_CHARSET);
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
    if (windowPropertyAtom != (Atom)NULL)
      XDeleteProperty(display,rootWindow,windowPropertyAtom);
    XFlush(display);

    XtDestroyApplicationContext(appContext);
    XtCloseDisplay(display);
}

int initMedmWidget() {
    if (clipboard) return 0;
    if (clipboard = createDlList()) {
	return 0;
    } else {
	return -1;
    }
}

int destroyMedmWidget() {
    if (displayInfoListHead) free((char *)displayInfoListHead);
    if (dlXmStringMoreToComeSymbol) XmStringFree(dlXmStringMoreToComeSymbol);
    if (executePopupMenuButtons[0]) XmStringFree(executePopupMenuButtons[0]);
    if (executePopupMenuButtons[1]) XmStringFree(executePopupMenuButtons[1]);
    if (highlightGC) XFreeGC(display,highlightGC);
    if (clipboard) {
	clearDlDisplayList(clipboard);
	free(clipboard);
    }
    return 0;
}
