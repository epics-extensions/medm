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

#define DEBUG_EVENTS 0
#define DEBUG_FILE 1
#define DEBUG_STRING_LIST 0
#define DEBUG_TRAVERSAL 0
#define DEBUG_UNDO 0
#define UNDO

#include <string.h>
#include <ctype.h>
#include <X11/IntrinsicP.h>
#include "medm.h"

#ifdef  __TED__
#include <Dt/Wsm.h>
#endif

#define MAX_DIR_LENGTH 512		/* max. length of directory name */

/* #define GRAB_WINDOW window */
#define GRAB_WINDOW None

Boolean modalGrab = FALSE;     /* KE: Not used ?? */
static DlList *tmpDlElementList = NULL;

/*
 * function to open a specified file (as .adl if specified as .dl),
 *	looking in EPICS_DISPLAY_PATH directory if unavailable in the
 *	working directory
 */

FILE *dmOpenUseableFile(char *filename)
{
    FILE *filePtr;
    char name[MAX_TOKEN_LENGTH], fullPathName[MAX_DIR_LENGTH],
      dirName[MAX_DIR_LENGTH];
    char *dir, *adlPtr;
    int suffixLength, usedLength, startPos;
#if DEBUG_FILE
    static FILE *file=NULL;
    static pid_t pid=0;
#endif

  /*
   * try to open the file as a ".adl"  rather than ".dl" which the
   *    editor generates
   */
    
  /* look in current directory first */
    adlPtr = strstr(filename,DISPLAY_FILE_ASCII_SUFFIX);
    if (adlPtr != NULL) {
      /* ascii name */
	suffixLength = strlen(DISPLAY_FILE_ASCII_SUFFIX);
    } else {
      /* binary name */
	suffixLength = strlen(DISPLAY_FILE_BINARY_SUFFIX);
    }

    usedLength = strlen(filename);
    strcpy(name,filename);
    name[usedLength-suffixLength] = '\0';
    strcat(name,DISPLAY_FILE_ASCII_SUFFIX);
    filePtr = fopen(name,"r");
#if DEBUG_FILE
    if(!file) {
	file=fopen("/tmp/medmLog","a");
	pid=getpid();
	if(file) {
	    fprintf(file,"Initializing PID: %d\n",pid);
	} else {
	    printf("Cannot open /tmp/medmLog\n");
	}
    }
    if(file) {
	fprintf(file,"[%d] dmOpenUseableFile: %s\n",pid,filename);
	fprintf(file,"  Converted to: %s\n",name);
	if(filePtr) fprintf(file,"    Found as: %s\n",name);
	fflush(file);
    }
#endif

  /* If not in current directory, look in EPICS_DISPLAY_PATH directory */
    if (filePtr == NULL) {
	dir = getenv("EPICS_DISPLAY_PATH");
	if (dir != NULL) {
	    startPos = 0;
	    while (filePtr == NULL &&
	      extractStringBetweenColons(dir,dirName,startPos,&startPos)) {
		strcpy(fullPathName,dirName);
		strcat(fullPathName,"/");
		strcat(fullPathName,name);
		filePtr = fopen(fullPathName,"r");
#if DEBUG_FILE
		if(file) {
		    fprintf(file,"  Converted to: %s\n",fullPathName);
		    if(filePtr) fprintf(file,"    Found as: %s\n",fullPathName);
		    fflush(file);
		}
#endif
	    }
	}
    }

    return (filePtr);
}

/*
 *  extract strings between colons from input to output
 *    this function works as an iterator...
 */
Boolean extractStringBetweenColons(char *input, char *output,
  int  startPos, int  *endPos)
{
    int i, j;

    i = startPos; j = 0;
    while (input[i] != '\0') {
	if (input[i] != ':') {
	    output[j++] = input[i];
	} else break;
	i++;
    }
    output[j] = '\0';
    if (input[i] != '\0') {
	i++;
    }
    *endPos = i;
    if (j == 0)
      return(False);
    else
      return(True);
}

/*
 * clean up the memory-resident display list (if there is one)
 */
void clearDlDisplayList(DlList *list)
{
    DlElement *dlElement, *pE;
    
    if (list->count == 0) return;
    dlElement = FirstDlElement(list);
    while (dlElement) {
	pE = dlElement;
	dlElement = dlElement->next;
	if (pE->run->destroy) {
	    pE->run->destroy(pE);
	} else {
	    free((char *)pE->structure.composite);
	    destroyDlElement(pE);
	}
    }
    emptyDlList(list);
}

/*
 * Same as clearDlDisplayList except that it does not clear the display
 * and it destroys any widgets
 */
void removeDlDisplayListElementsExceptDisplay(DlList *list)
{
    DlElement *dlElement, *pE;
    DlElement *psave = NULL;
    
    if (list->count == 0) return;
    dlElement = FirstDlElement(list);
    while (dlElement) {
	pE = dlElement;
	if (dlElement->type != DL_Display) {
	    dlElement = dlElement->next;
	    destroyElementWidgets(pE);
	    if (pE->run->destroy) {
		pE->run->destroy(pE);
	    } else {
		free((char *)pE->structure.composite);
		destroyDlElement(pE);
	    }
	} else {
	    psave = pE;
	    dlElement = dlElement->next;
	}
    }
    emptyDlList(list);

  /* Put the display back if there was one */
    if(psave) {
	appendDlElement(list, psave);
    }
}

/*
 * function which cleans up a given displayInfo in the displayInfoList
 * (including the displayInfo's display list if specified)
 */
void dmCleanupDisplayInfo(DisplayInfo *displayInfo, Boolean cleanupDisplayList)
{
    int i;
    Boolean alreadyFreedUnphysical;
    Widget DA;
    UpdateTask *ut = &(displayInfo->updateTaskListHead);

  /* save off current DA */
    DA = displayInfo->drawingArea;
  /* now set to NULL in displayInfo to signify "in cleanup" */
    displayInfo->drawingArea = NULL;

  /*
   * remove all update tasks in this display 
   */
    updateTaskDeleteAllTask(ut);
 
  /* 
   * as a composite widget, drawingArea is responsible for destroying
   *  it's children
   */
    if (DA != NULL) {
	XtDestroyWidget(DA);
	DA = NULL;
    }

  /* force a wait for all outstanding CA event completion */
  /* (wanted to do   while (ca_pend_event() != ECA_NORMAL);  but that sits there     forever)
   */
#ifdef __MONITOR_CA_PEND_EVENT__
    {
	double t;
	t = medmTime();
	ca_pend_event(CA_PEND_EVENT_TIME);
	t = medmTime() - t;
	if (t > 0.5) {
	    printf("dmCleanupDisplayInfo : time used by ca_pend_event = %8.1f\n",t);
	}
    }
#else
    ca_pend_event(CA_PEND_EVENT_TIME);
#endif

  /*
   * if cleanupDisplayList == TRUE
   *   then global cleanup ==> delete shell, free memory/structures, etc
   */

  /* Destroy undo information */
    if(displayInfo->undoInfo) destroyUndoInfo(displayInfo);
    
  /* Branch depending on cleanup mode */
    if (cleanupDisplayList) {
	XtDestroyWidget(displayInfo->shell);
	displayInfo->shell = NULL;
      /* remove display list here */
	clearDlDisplayList(displayInfo->dlElementList);
    } else {
	DlElement *dlElement = FirstDlElement(displayInfo->dlElementList);
	while (dlElement) {
	    if (dlElement->run->cleanup) {
		dlElement->run->cleanup(dlElement);
	    } else {
		dlElement->widget = NULL;
	    }
	    dlElement = dlElement->next;
	}
    }

  /*
   * free other X resources
   */
    if (displayInfo->drawingAreaPixmap != (Pixmap)NULL) {
	XFreePixmap(display,displayInfo->drawingAreaPixmap);
	displayInfo->drawingAreaPixmap = (Pixmap)NULL;
    }
    if (displayInfo->colormap != NULL && displayInfo->dlColormapCounter > 0) {
	alreadyFreedUnphysical = False;
	for (i = 0; i < displayInfo->dlColormapCounter; i++) {
	    if (displayInfo->colormap[i] != unphysicalPixel) {
		XFreeColors(display,cmap,&(displayInfo->colormap[i]),1,0);
	    } else if (!alreadyFreedUnphysical) {
	      /* only free "unphysical" pixel once */
		XFreeColors(display,cmap,&(displayInfo->colormap[i]),1,0);
		alreadyFreedUnphysical = True;
	    }
	}
	free( (char *) displayInfo->colormap);
	displayInfo->colormap = NULL;
	displayInfo->dlColormapCounter = 0;
	displayInfo->dlColormapSize = 0;
    }
    if (displayInfo->gc) {
	XFreeGC(display,displayInfo->gc);
	displayInfo->gc = NULL;
    }
    if (displayInfo->pixmapGC != NULL) {
	XFreeGC(display,displayInfo->pixmapGC);
	displayInfo->pixmapGC = NULL;
    }
    displayInfo->drawingAreaBackgroundColor = 0;
    displayInfo->drawingAreaForegroundColor = 0;
}


void dmRemoveDisplayInfo(DisplayInfo *displayInfo)
{
    displayInfo->prev->next = displayInfo->next;
    if (displayInfo->next != NULL)
      displayInfo->next->prev = displayInfo->prev;
    if (displayInfoListTail == displayInfo)
      displayInfoListTail = displayInfoListTail->prev;
    if (displayInfoListTail == displayInfoListHead )
      displayInfoListHead->next = NULL;
/* Cleanup resources and free display list */
    dmCleanupDisplayInfo(displayInfo,True);
    freeNameValueTable(displayInfo->nameValueTable,displayInfo->numNameValues);
    if (displayInfo->dlElementList) {
	clearDlDisplayList(displayInfo->dlElementList);
	free ( (char *) displayInfo->dlElementList);
    }
    if (displayInfo->selectedDlElementList) {
	clearDlDisplayList(displayInfo->selectedDlElementList);
	free ( (char *) displayInfo->selectedDlElementList);
    }
    free ( (char *) displayInfo->dlFile);
    free ( (char *) displayInfo->dlColormap);
    free ( (char *) displayInfo);

    if (displayInfoListHead == displayInfoListTail) {
	currentColormap = defaultColormap;
	currentColormapSize = DL_MAX_COLORS;
	currentDisplayInfo = NULL;
    }

}

/*
 * function to remove ALL displayInfo's
 *   this includes a full cleanup of associated resources and displayList
 */
void dmRemoveAllDisplayInfo()
{
    DisplayInfo *nextDisplay, *displayInfo;

    displayInfo = displayInfoListHead->next;
    while (displayInfo != NULL) {
	nextDisplay = displayInfo->next;
	dmRemoveDisplayInfo(displayInfo);
	displayInfo = nextDisplay;
    }
    displayInfoListHead->next = NULL;
    displayInfoListTail = displayInfoListHead;

    currentColormap = defaultColormap;
    currentColormapSize = DL_MAX_COLORS;
    currentDisplayInfo = NULL;
}

/*
 * Traverse (execute) specified displayInfo's display list
 */
void dmTraverseDisplayList(DisplayInfo *displayInfo)
{
    DlElement *element;

  /* Traverse the display list */
#if DEBUG_TRAVERSAL
    fprintf(stderr,"\n[dmTraverseDisplayList: displayInfo->dlElementList:\n");
    dumpDlElementList(displayInfo->dlElementList);
#endif
    element = FirstDlElement(displayInfo->dlElementList);
    while (element) {
	(element->run->execute)(displayInfo,element);
	element = element->next;
    }

  /* Change the cursor for the drawing area */
    XDefineCursor(display,XtWindow(displayInfo->drawingArea),
      (currentActionType == SELECT_ACTION ? rubberbandCursor : crosshairCursor));
  /* Flush the display to implement the cursor change */
    XFlush(display);

  /* Poll CA */
#ifdef __MONITOR_CA_PEND_EVENT__
    {
	double t;
	t = medmTime();
	ca_pend_event(CA_PEND_EVENT_TIME);
	t = medmTime() - t;
	if (t > 0.5) {
	    printf("dmTraverseDisplayList : time used by ca_pend_event = %8.1f\n",t);
	}
    }
#else
    ca_pend_event(CA_PEND_EVENT_TIME);
#endif
}


/*
 * Traverse (execute) all displayInfos and display lists
 * (Could call dmTraverseDisplayList inside the displayInfo traversal,
 *    but only need one XFlush and one ca_pend_event)
 */
void dmTraverseAllDisplayLists()
{
    DisplayInfo *displayInfo;
    DlElement *element;

    displayInfo = displayInfoListHead->next;

  /* Traverse the displayInfo list */
    while (displayInfo != NULL) {

      /* Traverse the display list for this displayInfo */
#if DEBUG_TRAVERSAL
	fprintf(stderr,"\n[dmTraverseAllDisplayLists: displayInfo->dlElementList:\n");
	dumpDlElementList(displayInfo->dlElementList);
#endif
	element = FirstDlElement(displayInfo->dlElementList);
	while (element) {
	    (element->run->execute)(displayInfo,element);
	    element = element->next;
	}

      /* Change the cursor for the drawing area for this displayInfo */
	XDefineCursor(display,XtWindow(displayInfo->drawingArea),
	  (currentActionType == SELECT_ACTION ? rubberbandCursor : crosshairCursor));
      /* Flush the display to implement cursor changes */
      /* Also necessary to keep the stacking order rendered correctly */
	XFlush(display);

	displayInfo = displayInfo->next;
    }

  /* Poll CA */
#ifdef __MONITOR_CA_PEND_EVENT__
    {
	double t;
	t = medmTime();
	ca_pend_event(CA_PEND_EVENT_TIME);
	t = medmTime() - t;
	if (t > 0.5) {
	    printf("dmTraverseAllDisplayLists : time used by ca_pend_event = %8.1f\n",t);
	}
    }
#else
    ca_pend_event(CA_PEND_EVENT_TIME);
#endif

}

/*
 * traverse (execute) specified displayInfo's display list non-widget elements
 */
void dmTraverseNonWidgetsInDisplayList(DisplayInfo *displayInfo)
{
    DlElement *element;
    Dimension width,height;

    if (displayInfo == NULL) return;

  /* Unhighlight any selected elements */
    unhighlightSelectedElements();
    
  /* Fill the background with the background color */
    XSetForeground(display,displayInfo->pixmapGC,
      displayInfo->colormap[displayInfo->drawingAreaBackgroundColor]);
    XtVaGetValues(displayInfo->drawingArea,
      XmNwidth,&width,XmNheight,&height,NULL);
    XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
      0, 0, (unsigned int)width,(unsigned int)height);

  /* Draw grid */
    if(displayInfo->gridOn && globalDisplayListTraversalMode == DL_EDIT)
     drawGrid(displayInfo);

  /* Traverse the display list */
  /* Point to element after the display */
    element = FirstDlElement(displayInfo->dlElementList)->next;
    while (element) {
	if (!element->widget) {
	    (element->run->execute)(displayInfo, element);
	}
	element = element->next;
    }

  /* Since the execute traversal copies to the pixmap, now udpate the window */
    XCopyArea(display,displayInfo->drawingAreaPixmap,
      XtWindow(displayInfo->drawingArea),
      displayInfo->pixmapGC, 0, 0, (unsigned int)width,
      (unsigned int)height, 0, 0);

  /* Highlight any selected elements */
    highlightSelectedElements();
    
  /* Change drawingArea's cursor to the appropriate cursor */
    XDefineCursor(display,XtWindow(displayInfo->drawingArea),
      (currentActionType == SELECT_ACTION ?
	rubberbandCursor : crosshairCursor));

}

/*
 * function to return the best fitting font for the field and string
 *   if textWidthFlag = TRUE:  use the text string and find width also
 *   if textWidthFlag = FALSE: ignore text,w fields and
 *	make judgment based on height info only & return
 *	width of largest character as *usedW
 * KE: This algorithm is flakey
 *   Can never return 0 or MAX_FONTS-1
 *   Doesn't pick scaled fonts uniformly
 */
int dmGetBestFontWithInfo(XFontStruct **fontTable, int nFonts, char *text,
  int h, int w, int *usedH, int *usedW, Boolean textWidthFlag)
{
    int i, temp, count, upper, lower;
    static int errorOnI = -1;

    i = nFonts/2;
    upper = nFonts-1;
    lower = 0;
    count = 0;

/* first select based on height of bounding box */
    while ( (i>0) && (i<nFonts) && ((upper-lower)>2) && (count<nFonts/2)) {
	count++;
	if (fontTable[i]->ascent + fontTable[i]->descent > h) {
	    upper = i;
	    i = upper - (upper-lower)/2;
	} else if (fontTable[i]->ascent + fontTable[i]->descent < h) {
	    lower = i;
	    i = lower + (upper-lower)/2;
	}
    }
    if (i < 0) i = 0;
    if (i >= nFonts) i = nFonts - 1;

    *usedH = fontTable[i]->ascent + fontTable[i]->descent;
    *usedW = fontTable[i]->max_bounds.width;

    if (textWidthFlag) {
/* now select on width of bounding box */
	while ( ((temp = XTextWidth(fontTable[i],text,strlen(text))) > w)
	  && i > 0 ) i--;

	*usedW = temp;

#if 0
	if ( *usedH > h || *usedW > w)
	  if (errorOnI != i) {
	      errorOnI = i;
	      fprintf(stderr,
		"\ndmGetBestFontWithInfo: need another font near pixel height = %d",
		h);
	  }
#endif
    }

    return (i);
}


XtErrorHandler trapExtraneousWarningsHandler(String message)
{
    if (message && *message) {
/* "Attempt to remove non-existant passive grab" */
	if (!strcmp(message,"Attempt to remove non-existant passive grab"))
	  return(0);
/* "The specified scale value is less than the minimum scale value." */
/* "The specified scale value is greater than the maximum scale value." */
	if (!strcmp(message,"The specified scale value is"))
	  return(0);
    } else {
	medmPostMsg("trapExtraneousWarningsHandler:\n%s\n", message);
    }
  
    return(0);
}


/*
 * function to march up widget hierarchy to retrieve top shell, and
 *  then run over displayInfoList and return the corresponding DisplayInfo *
 */
DisplayInfo *dmGetDisplayInfoFromWidget(Widget widget)
{
    Widget w;
    DisplayInfo *displayInfo = NULL;

    w = widget;
    while (w && (XtClass(w) != topLevelShellWidgetClass)) {
	w = XtParent(w);
    }

    if (w) {
	displayInfo = displayInfoListHead->next;
	while (displayInfo && (displayInfo->shell != w)) {
	    displayInfo = displayInfo->next;
	}
    }
    return displayInfo;
}

/*
 * write specified displayInfo's display list
 */
void dmWriteDisplayList(DisplayInfo *displayInfo, FILE *stream)
{
    DlElement *element, *cmapElement;

    writeDlFile(stream,displayInfo->dlFile,0);
    if (element = FirstDlElement(displayInfo->dlElementList)) {
      /* This must be DL_DISPLAY */
	(element->run->write)(stream,element,0);
    }
    writeDlColormap(stream,displayInfo->dlColormap,0);
    element = element->next;
  /* traverse the display list */
    while (element) {
	(element->run->write)(stream,element,0);
	element = element->next;
    }
    fprintf(stream,"\n");
}

void medmSetDisplayTitle(DisplayInfo *displayInfo)
{
    char str[MAX_FILE_CHARS+10];

    if (displayInfo->dlFile) {
	char *tmp, *tmp1;
	tmp = tmp1 = displayInfo->dlFile->name;
	while (*tmp != '\0')
	  if (*tmp++ == '/') tmp1 = tmp;
	if (displayInfo->hasBeenEditedButNotSaved) {
	    strcpy(str,tmp1);
	    strcat(str," (edited)");
	    XtVaSetValues(displayInfo->shell,XmNtitle,str,NULL);
	} else {
	    XtVaSetValues(displayInfo->shell,XmNtitle,tmp1,NULL);
	}
    }
}

void medmMarkDisplayBeingEdited(DisplayInfo *displayInfo)
{
    char str[MAX_FILE_CHARS+10];

    if (globalDisplayListTraversalMode == DL_EXECUTE) return;
    if (displayInfo->hasBeenEditedButNotSaved) return;
    displayInfo->hasBeenEditedButNotSaved = True;
    medmSetDisplayTitle(displayInfo);
}

/*
 * Starting at tail of display list, look for smallest object which contains
 *   the specified position 
 */
DlElement *findSmallestTouchedElement(DlList *pList, Position x0, Position y0)
{
    DlElement *pE, *pSmallest, *pDisplay;
    double area, minArea;

  /* Traverse the display list */
    pSmallest = pDisplay = NULL;
    minArea = (double)(INT_MAX)*(double)(INT_MAX);
    pE = pList->tail;
    while (pE->prev) {
	DlObject *po = &(pE->structure.rectangle->object);

      /* Don't use the display but save it as a fallback */
	if (pE->type == DL_Display) {
	    pDisplay = pE;
	} else {
	  /* See if the point falls inside the element */
	    if (((x0 >= po->x) && (x0 <= po->x + po->width))	&&
	      ((y0 >= po->y) && (y0 <= po->y + po->height))) {
	      /* See if smallest element so far */
		area=(double)(po->width)*(double)(po->height);
		if (area < minArea) {
		    pSmallest = pE;
		    minArea = area;
		}
	    }
	}
	pE = pE->prev;
    }

  /* Use the display as the fallback (Assume we'll always find one) */
    if (pSmallest == NULL) pSmallest = pDisplay;

  /* If in EXECUTE mode decompose a composite element */
    if (globalDisplayListTraversalMode == DL_EXECUTE &&
      pSmallest->type == DL_Composite) {
      /* Find the particular component that was picked */
	pSmallest = lookupCompositeChild(pSmallest,x0,y0);
    }

  /* Return the element */
    return (pSmallest);
}

/*
 * Starting at head of composite (specified element), lookup picked object
 */
DlElement *lookupCompositeChild(DlElement *composite, Position x0, Position y0)
{
    DlElement *element, *saveElement;
    int minWidth, minHeight;

    if (!composite || (composite->type != DL_Composite))
      return composite;

    minWidth = INT_MAX;		/* according to XPG2's values.h */
    minHeight = INT_MAX;
    saveElement = NULL;

  /* Single element lookup  */
    element = FirstDlElement(composite->structure.composite->dlElementList);

    while (element) {
	DlObject *po = &(element->structure.rectangle->object);
	if (x0 >= po->x	&& x0 <= po->x + po->width &&
	  y0 >= po->y && y0 <= po->y + po->height) {
	  /* eligible element, now see if smallest element so far */
	    if (po->width < minWidth && po->height < minHeight) {
		minWidth = (element->structure.rectangle)->object.width;
		minHeight = (element->structure.rectangle)->object.height;
		saveElement = element;
	    }
	}
	element = element->prev;
    }
    if (saveElement) {
      /* Found a new element
       *   If it is composite, recurse, otherwise return it */
	if (saveElement->type == DL_Composite) {
	    return(lookupCompositeChild(saveElement,x0,y0));
	} else {
	    return(saveElement);
	}
    } else {
      /* Didn't find anything, return old composite */
	return(composite);
    }
}

/*
 * Finds elements in list 1 and puts them into list 2 according to
 *   the mode mask specification AND whether the points are close or not
 */
void findSelectedElements(DlList *pList1, Position x0, Position y0,
  Position x1, Position y1, DlList *pList2, unsigned int mode)
{
  /* Number of pixels to consider to be the same as no motion */
#define RUBBERBAND_EPSILON 4

    if ((x1 - x0) <= RUBBERBAND_EPSILON && (y1 - y0) <= RUBBERBAND_EPSILON) {
      /* No motion, treat as a point */
	if (mode&SmallestTouched) {
	  /* Find the smallest element that is touched by the point */
	    Position x = (x0 + x1)/2, y = (y0 + y1)/2;
	    DlElement *pE;

	    pE = findSmallestTouchedElement(pList1,x,y);
	    if (pE) {
		DlElement *pENew = createDlElement(DL_Element,(XtPointer)pE,NULL);
		if (pENew) {
		    appendDlElement(pList2, pENew);
		}
	    }
	}
	if (mode&AllTouched) {
	  /* Find all the elements that are touched by the point */
	    findAllMatchingElements(pList1,x0,y0,x1,y1,pList2,AllTouched);
	}
    } else {
      /* Treat as a rectangle */
	if (mode&AllEnclosed) {
	  /* Find all the elements that are enclosed by the rectangle */
	    findAllMatchingElements(pList1,x0,y0,x1,y1,pList2,AllEnclosed);
	}
    }
}

/*
 * Finds all elements in list 1 that match according to the mode mask
 *   and inserts them into the front of list 2
 *   Does not include the display
 */
void findAllMatchingElements(DlList *pList1, Position x0, Position y0,
  Position x1, Position y1, DlList *pList2, unsigned int mode)
{
    DlElement *pE = LastDlElement(pList1);
    int criterion;
    
    while (pE->prev) {
	DlObject *po = &(pE->structure.rectangle->object);

	if(mode&AllTouched) {
	  /* Find all the elements that are touched by the midpoint */
	    Position x = (x0 + x1)/2, y = (y0 + y1)/2;
	    
	    criterion = po->x <= x && (po->x + po->width) >= x &&
	      po->y <= y && (po->y + po->height) >= y;
	} else if(mode&AllEnclosed) {
	  /* Find all the elements that are enclosed by the rectangle */
	    criterion = x0 <= po->x && x1 >= (po->x + po->width) &&
	      y0 <= po->y && y1 >= (po->y + po->height);
	} else {
	    return;
	}
	
      /* Do not include the display */
	if (pE->type != DL_Display && criterion) {
	    DlElement *pENew = createDlElement(DL_Element,(XtPointer)pE,NULL);
	    if (pENew) {
		insertDlElement(pList2, pENew);
	    } else {
		char string[48];
		
		medmPostMsg("findAllMatchingElements: Could not create element\n");
		return;
	    }
	}
	pE = pE->prev;
    }
    return;
}

Boolean dmResizeDisplayList(DisplayInfo *displayInfo,
  Dimension newWidth, Dimension newHeight)
{
    DlElement *elementPtr;
    float sX, sY;
    Dimension oldWidth, oldHeight;
    Boolean moveWidgets;
    int j, newX, newY;

    elementPtr = FirstDlElement(displayInfo->dlElementList);
    oldWidth = elementPtr->structure.display->object.width;
    oldHeight = elementPtr->structure.display->object.height;

  /* simply return (value FALSE) if no real change */
    if (oldWidth == newWidth && oldHeight == newHeight) return (FALSE);

  /* resize the display, then do selected elements */
    elementPtr->structure.display->object.width = newWidth;
    elementPtr->structure.display->object.height = newHeight;

  /* proceed with scaling...*/
    sX = (float) ((float)newWidth/(float)oldWidth);
    sY = (float) ((float)newHeight/(float)oldHeight);

    resizeDlElementList(displayInfo->dlElementList,0,0,sX,sY);
    return (TRUE);
}

/*
 * function to resize the selected display elements based on new drawingArea.
 *
 *   return value is a boolean saying whether resized actually occurred.
 *   this function resizes the selected elements when the whole display
 *   is resized.
 */

Boolean dmResizeSelectedElements(DisplayInfo *displayInfo, Dimension newWidth, 
  Dimension newHeight)
{
    DlElement *elementPtr;
    float sX, sY;
    Position newX, newY;
    Dimension oldWidth, oldHeight;
    int i, j;
    Boolean moveWidgets;

    elementPtr = FirstDlElement(displayInfo->dlElementList);
    oldWidth = elementPtr->structure.display->object.width;
    oldHeight = elementPtr->structure.display->object.height;

  /* simply return (value FALSE) if no real change */
    if (oldWidth == newWidth && oldHeight == newHeight) return (FALSE);

  /* resize the display, then do selected elements */
    elementPtr->structure.display->object.width = newWidth;
    elementPtr->structure.display->object.height = newHeight;

  /* proceed with scaling...*/
    sX = (float) ((float)newWidth/(float)oldWidth);
    sY = (float) ((float)newHeight/(float)oldHeight);

    resizeDlElementReferenceList(displayInfo->selectedDlElementList,0,0,sX,sY);
    return (TRUE);
}

void resizeDlElementReferenceList(DlList *dlElementList, int x, int y,
  float scaleX, float scaleY)
{
    DlElement *dlElement;
    if (dlElementList->count < 1) return;
    dlElement = FirstDlElement(dlElementList);
    while (dlElement) {
	DlElement *ele = dlElement->structure.element;
	if (ele->type != DL_Display) {
	    int w = ele->structure.rectangle->object.width;
	    int h = ele->structure.rectangle->object.height;
	    int xOffset = (int) (scaleX * (float) w + 0.5) - w;
	    int yOffset = (int) (scaleY * (float) h + 0.5) - h;
	    if (ele->run->scale) {
		ele->run->scale(ele,xOffset,yOffset);
	    }
	    w = ele->structure.rectangle->object.x - x;
	    h = ele->structure.rectangle->object.y - y;
	    xOffset = (int) (scaleX * (float) w + 0.5) - w;
	    yOffset = (int) (scaleY * (float) h + 0.5) - h;
	    if (ele->run->move) {
		ele->run->move(ele,xOffset,yOffset);
	    }
	}
	dlElement = dlElement->next;
    }
}

void resizeDlElementList(DlList *dlElementList, int x, int y,
  float scaleX, float scaleY)
{
    DlElement *ele;
    if (dlElementList->count < 1) return;
    ele = FirstDlElement(dlElementList);
    while (ele) {
	if (ele->type != DL_Display) {
	    int w = ele->structure.rectangle->object.width;
	    int h = ele->structure.rectangle->object.height;
	    int xOffset = (int) (scaleX * (float) w + 0.5) - w;
	    int yOffset = (int) (scaleY * (float) h + 0.5) - h;
	    if (ele->run->scale) {
		ele->run->scale(ele,xOffset,yOffset);
	    }
	    w = ele->structure.rectangle->object.x - x;
	    h = ele->structure.rectangle->object.y - y;
	    xOffset = (int) (scaleX * (float) w + 0.5) - w;
	    yOffset = (int) (scaleY * (float) h + 0.5) - h;
	    if (ele->run->move) {
		ele->run->move(ele,xOffset,yOffset);
	    }
	}
	ele = ele->next;
    }
}

/******************************************
 ************ rubberbanding, etc.
 ******************************************/

GC xorGC;

void initializeRubberbanding()
{
/* Create the xorGC and rubberbandCursor for drawing while dragging */
    xorGC = XCreateGC(display,rootWindow,0,NULL);
    XSetSubwindowMode(display,xorGC,IncludeInferiors);
    XSetFunction(display,xorGC,GXxor);
    XSetForeground(display,xorGC,~0);
#if 0
    XSetBackground(display,xorGC,WhitePixel(display,screenNum));
    XSetForeground(display,xorGC,WhitePixel(display,screenNum));
    XSetForeground(display,xorGC,getPixelFromColormapByString(display,screenNum,
      cmap,"grey50"));
    XSetFunction(display,xorGC,GXxor);
    XSetFunction(display,xorGC,GXinvert);
#endif
}

void doRubberbanding(Window window, Position *initialX, Position *initialY,
  Position *finalX,  Position *finalY)
{
    XEvent event;

    int x0, y0, x1, y1;
    unsigned int w, h;

    *finalX = *initialX;
    *finalY = *initialY;
    x0 = *initialX;
    y0 = *initialY;
    x1 = x0;
    y1 = y0;
    w = (Dimension) 0;
    h = (Dimension) 0;

/* Have all interesting events go to window */
#if DEBUG_EVENTS > 1
    printf("In doRubberbanding before XGrabPointer\n");
#endif
    XGrabPointer(display,window,FALSE,
      (unsigned int)(ButtonMotionMask|ButtonReleaseMask),
      GrabModeAsync,GrabModeAsync,GRAB_WINDOW,rubberbandCursor,CurrentTime);

/* Grab the server to ensure that XORing will be okay */
    XGrabServer(display);
    XDrawRectangle(display,window,xorGC,MIN(x0,x1),MIN(y0,y1),w,h);

  /* Loop until the button is released */
    while (TRUE) {
	XtAppNextEvent(appContext,&event);
	switch (event.type) {
	case ButtonRelease:
#if DEBUG_EVENTS > 1
	    printf("ButtonRelease: x=%d y=%d\n",event.xbutton.x,event.xbutton.y);
	    fflush(stdout);
#endif	    
	  /* Undraw old one */
	    XDrawRectangle(display,window,xorGC,MIN(x0,x1),MIN(y0,y1),w,h);
	    XUngrabServer(display);
	    XUngrabPointer(display,CurrentTime);
	    *initialX =  MIN(x0,event.xbutton.x);
	    *initialY =  MIN(y0,event.xbutton.y);
	    *finalX   =  MAX(x0,event.xbutton.x);
	    *finalY   =  MAX(y0,event.xbutton.y);
	    return;		/* return from while(TRUE) */
	case MotionNotify:
	  /* Undraw old one */
	    XDrawRectangle(display,window,xorGC,MIN(x0,x1),MIN(y0,y1),w,h);
	  /* Update current coordinates */
	    x1 = event.xbutton.x;
	    y1 = event.xbutton.y;
	    w =  (MAX(x0,x1) - MIN(x0,x1));
	    h =  (MAX(y0,y1) - MIN(y0,y1));
	  /* Draw new one */
#if DEBUG_EVENTS > 1
	    printf("MotionNotify: x=%d y=%d\n",event.xbutton.x,event.xbutton.y);
	    fflush(stdout);
#endif	    
	    XDrawRectangle(display,window,xorGC,MIN(x0,x1),MIN(y0,y1),w,h); 
	    break;
	default:
	    XtDispatchEvent(&event);
	}
    }
}

/*
 * do (multiple) dragging  of all elements in global selectedElementsArray
 *	RETURNS: boolean indicating whether drag ended in the window
 *	(and hence was valid)
 */
Boolean doDragging(Window window, Dimension daWidth, Dimension daHeight,
  Position initialX, Position initialY, Position *finalX, Position *finalY)
{
    int i, minX, maxX, minY, maxY, groupWidth, groupHeight,
      groupDeltaX0, groupDeltaY0, groupDeltaX1, groupDeltaY1;
    XEvent event;
    int xOffset, yOffset;
    DisplayInfo *cdi;
    int xdel, ydel;
    DlElement *dlElement;

  /* If on current display, simply return */
    if (currentDisplayInfo == NULL) return (False);

    cdi = currentDisplayInfo;

    xOffset = 0;
    yOffset = 0;

    minX = INT_MAX; minY = INT_MAX;
    maxX = INT_MIN; maxY = INT_MIN;

  /* Have all interesting events go to window */
    XGrabPointer(display,window,FALSE,
      (unsigned int)(ButtonMotionMask|ButtonReleaseMask),
      GrabModeAsync,GrabModeAsync,GRAB_WINDOW,dragCursor,CurrentTime);
  /* Grab the server to ensure that XORing will be okay */
    XGrabServer(display);

  /* As usual, type in union unimportant as long as object is 1st thing...*/
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while (dlElement) {
	DlElement *pE = dlElement->structure.element;
	if (pE->type != DL_Display) {
	    DlObject *po = &pE->structure.rectangle->object;
	    XDrawRectangle(display,window, xorGC, 
	      po->x + xOffset, po->y + yOffset, po->width , po->height);
	    minX = MIN(minX, (int)po->x);
	    maxX = MAX(maxX, (int)po->x + (int)po->width);
	    minY = MIN(minY, (int)po->y);
	    maxY = MAX(maxY, (int)po->y + (int)po->height);
	}
	dlElement = dlElement->next;
    }
    groupWidth = maxX - minX;
    groupHeight = maxY - minY;
  /* How many pixels is the cursor position from the left edge of all objects */
    groupDeltaX0 = initialX - minX;
  /* How many pixels is the cursor position from the top edge of all objects */
    groupDeltaY0 = initialY - minY;
  /* How many pixels is the cursor position from the right edge of all objects */
    groupDeltaX1 = groupWidth - groupDeltaX0;
  /* How many pixels is the cursor position from the bottom edge of all objects */
    groupDeltaY1 = groupHeight - groupDeltaY0;

/* Loop until the button is released */
    while (TRUE) {
	XtAppNextEvent(appContext,&event);
	switch (event.type) {
	case ButtonRelease:
	  /* Undraw old ones */
	    dlElement = FirstDlElement(cdi->selectedDlElementList);
	    while (dlElement) {
		DlElement *pE = dlElement->structure.element;
		if (pE->type != DL_Display) {
		    DlObject *po = &pE->structure.rectangle->object;
		    XDrawRectangle(display,window, xorGC,
		      po->x + xOffset, po->y + yOffset, po->width , po->height);
		}
		dlElement = dlElement->next;
	    }
	    XUngrabServer(display);
	    XUngrabPointer(display,CurrentTime);
	    *finalX = initialX + xOffset;
	    *finalY = initialY + yOffset;
	  /* (Always return true - for clipped dragging...) */
	    return (True);	/* return from while(TRUE) */
	case MotionNotify:
	  /* Undraw old ones */
	    dlElement = FirstDlElement(cdi->selectedDlElementList);
	    while (dlElement) {
		DlElement *pE = dlElement->structure.element;
		if (pE->type != DL_Display) {
		    DlObject *po = &pE->structure.rectangle->object;
		    XDrawRectangle(display,window, xorGC,
		      po->x + xOffset, po->y + yOffset, po->width , po->height);
		}
		dlElement = dlElement->next;
	    }
	  /* Update current coordinates */
	    if (event.xmotion.x < groupDeltaX0)
	      xdel = groupDeltaX0;
	    else
	      if (event.xmotion.x > (int)(daWidth-groupDeltaX1))
		xdel =  daWidth - groupDeltaX1;
	      else
		xdel =  event.xmotion.x;
	    if (event.xmotion.y < groupDeltaY0)
	      ydel = groupDeltaY0;
	    else
	      if (event.xmotion.y > (int)(daHeight-groupDeltaY1))
		ydel =  daHeight - groupDeltaY1;
	      else
		ydel =  event.xmotion.y;

	    xOffset = xdel - initialX;
	    yOffset  = ydel - initialY;
	    dlElement = FirstDlElement(cdi->selectedDlElementList);
	    while (dlElement) {
		DlElement *pE = dlElement->structure.element;
		if (pE->type != DL_Display) {
		    DlObject *po = &pE->structure.rectangle->object;
		    XDrawRectangle(display,window, xorGC,
		      po->x + xOffset, po->y + yOffset, po->width , po->height);
		}
		dlElement = dlElement->next;
	    }
	    break;
	default:
	    XtDispatchEvent(&event);
	}
    }
}

/*
 * do PASTING (with drag effect) of all elements in global
 *		 clipboardElementsArray
 *	RETURNS: DisplayInfo ptr indicating whether drag ended in a display
 *		 and the positions in that display
 */
DisplayInfo *doPasting(Position *displayX, Position *displayY,
  int *offsetX, int *offsetY)
{
    XEvent event;
    DisplayInfo *displayInfo;
    int dx, dy, xul, yul, xlr, ylr;
    Window window, childWindow, root, child;
    int rootX, rootY, winX, winY;
    unsigned int mask;
    int i;
    DlElement *dlElement = NULL;

  /* if no clipboard elements, simply return */
    if (IsEmpty(clipboard)) return NULL;

    window = RootWindow(display,screenNum);

  /* get position of upper left element in display */
    xul = INT_MAX;
    yul = INT_MAX;
    xlr = 0;
    ylr = 0;
  /* try to normalize for paste such that cursor is in middle of pasted objects */
    dlElement = FirstDlElement(clipboard);
    while (dlElement) {
	if (dlElement->type != DL_Display) {
	    DlObject *po = &(dlElement->structure.rectangle->object);
	    xul = MIN(xul, po->x);
	    yul = MIN(yul, po->y);
	    xlr = MAX(xlr, po->x + po->width);
	    ylr = MAX(ylr, po->y + po->height);
	}
	dlElement = dlElement->next;
    }
    dx = (xul + xlr)/2;
    dy = (yul + ylr)/2;

  /* update offsets to be added when paste is done */
    *offsetX = -dx;
    *offsetY = -dy;

    if (!XQueryPointer(display,window,&root,&child,&rootX,&rootY,
      &winX,&winY,&mask)) {
	XtAppWarning(appContext,"doPasting: query pointer error");
    }

  /* have all interesting events go to window  - including some for WM's sake */
    XGrabPointer(display,window,False, (unsigned int)(PointerMotionMask|
      ButtonReleaseMask|ButtonPressMask|EnterWindowMask),
      GrabModeAsync,GrabModeAsync,GRAB_WINDOW,dragCursor,CurrentTime);
  /* grab the server to ensure that XORing will be okay */
    XGrabServer(display);

  /* as usual, type in union unimportant as long as object is 1st thing...*/
    dlElement = FirstDlElement(clipboard);
    while (dlElement) {
	if (dlElement->type != DL_Display) {
	    DlObject *po = &(dlElement->structure.rectangle->object);
	    XDrawRectangle(display,window, xorGC, 
	      rootX + po->x - dx, rootY + po->y - dy,
	      po->width, po->height);
	}
	dlElement = dlElement->next;
    }

  /*
   * now loop until the button is released
   */
    while (TRUE) {
	XtAppNextEvent(appContext,&event);
	switch (event.type) {
	case ButtonRelease:
	  /* undraw old ones */
	    dlElement = FirstDlElement(clipboard);
	    while (dlElement) {
		if (dlElement->type != DL_Display) {
		    DlObject *po = &(dlElement->structure.rectangle->object);
		    XDrawRectangle(display,window, xorGC, 
		      rootX + po->x - dx, rootY + po->y - dy,
		      po->width, po->height);
		}
		dlElement = dlElement->next;
	    }
	    XUngrabServer(display);
	    XUngrabPointer(display,CurrentTime);
	    XSync(display,False);
	    while (XtAppPending(appContext)) {
		XtAppNextEvent(appContext,&event);
		XtDispatchEvent(&event);
	    }
	    displayInfo = pointerInDisplayInfo;
	    if (displayInfo) {
		XTranslateCoordinates(display,window,
		  XtWindow(displayInfo->drawingArea),
		  rootX,rootY,
		  &winX,&winY,&childWindow);
	    }
	    *displayX =  winX;
	    *displayY =  winY;
	    return (displayInfo);

	case MotionNotify:
	  /* undraw old ones */
	    dlElement = FirstDlElement(clipboard);
	    while (dlElement) {
		if (dlElement->type != DL_Display) {
		    DlObject *po = &(dlElement->structure.rectangle->object);
		    XDrawRectangle(display,window, xorGC,
		      rootX + po->x - dx, rootY + po->y - dy,
		      po->width, po->height);
		}
		dlElement = dlElement->next;
	    }
	  /* update current coordinates */
	    rootX = event.xbutton.x_root;
	    rootY = event.xbutton.y_root;

	  /* draw new ones */
	    dlElement = FirstDlElement(clipboard);
	    while (dlElement) {
		if (dlElement->type != DL_Display) {
		    DlObject *po = &(dlElement->structure.rectangle->object);
		    XDrawRectangle(display,window, xorGC,
		      rootX + po->x - dx, rootY + po->y - dy,
		      po->width, po->height);
		}
		dlElement = dlElement->next;
	    }
	    break;

	default:
	    XtDispatchEvent(&event);
	    break;
	}
    }
}

/*
 * function to see if specified element is already in the global
 *	selectedElementsArray and return True or False based on that evaluation
 */
Boolean alreadySelected(DlElement *element)
{
    DlElement *dlElement;

    if (!currentDisplayInfo) return (False);
    if (IsEmpty(currentDisplayInfo->selectedDlElementList)) return False;
    dlElement = FirstDlElement(currentDisplayInfo->selectedDlElementList);
    while (dlElement) {
	DlElement *pE = element->structure.element;
	if ((pE->type != DL_Display) && (dlElement->structure.element == pE))
	  return True;
	dlElement = dlElement->next;
    }
    return (False);
}

void toggleHighlightRectangles(DisplayInfo *displayInfo, int xOffset, int yOffset)
{
    DlElement *dlElement = FirstDlElement(displayInfo->selectedDlElementList);
    DlElementType type;
    DlObject *po;
    int width, height;
#if DEBUG_EVENTS > 1
    printf("\n[toggleHighlightRectangles] selectedDlElement list :\n");
    dumpDlElementList(displayInfo->selectedDlElementList);
#endif
  /* Traverse the elements */
    while (dlElement) {
	if (dlElement->type == DL_Element) {
	    type = dlElement->structure.element->type;
	    po = &dlElement->structure.element->structure.composite->object;
	} else {     /* Should not be using this branch */
	    type = dlElement->type;
	    po = &dlElement->structure.composite->object;
	}
	width = (po->width + xOffset);
	width = MAX(1,width);
	height = (po->height + yOffset);
	height = MAX(1,height);
#if DEBUG_EVENTS > 1
	printf("  %s (%s): x: %d y: %d width: %u height: %u\n"
	  "    xOffset: %d yOffset: %d Used-width: %d Used-height %d\n",
	  elementType(dlElement->type),
	  elementType(type),
	  po->x,po->y,po->width,po->height,xOffset,yOffset,width,height);
#endif
      /* If not the display, draw a rectangle */
	if (type != DL_Display) {
	    XDrawRectangle(XtDisplay(displayInfo->drawingArea),
	      XtWindow(displayInfo->drawingArea),xorGC,
	      po->x,po->y,(Dimension)width,(Dimension)height);
	}
      /* Set next element */
	dlElement = dlElement->next;
    }
}

/*
 * do (multiple) resizing of all elements in global selectedElementsArray
 *	RETURNS: boolean indicating whether resize ended in the window
 *	(and hence was valid)
 * KE: This is not correct; It ALWAYS returns true
 */
Boolean doResizing(Window window, Position initialX, Position initialY, 
  Position *finalX, Position *finalY)
{
    int i, xOffset, yOffset;
    XEvent event;
    Boolean inWindow;
    DisplayInfo *cdi;
    int width, height;
    DlElement *dlElement;

    if (!currentDisplayInfo) return False;
    cdi = currentDisplayInfo;

    xOffset = 0;
    yOffset = 0;

    inWindow = True;

#if DEBUG_EVENTS
    printf("In doResizing before XGrabPointer\n");
#endif
  /* Grab all interesting events */
    XGrabPointer(display,window,FALSE,
      (unsigned int)(ButtonMotionMask|ButtonReleaseMask),
      GrabModeAsync,GrabModeAsync,GRAB_WINDOW,resizeCursor,CurrentTime);
  /* Grab the server to ensure that XORing will be okay */
    XGrabServer(display);

  /* XOR the outline */
    toggleHighlightRectangles(currentDisplayInfo,xOffset,yOffset);

   /* Loop until the button is released */
    while (TRUE) {
	XtAppNextEvent(appContext,&event);
	switch (event.type) {
	case ButtonRelease:
	  /* Undraw old ones (XOR again) */
#if DEBUG_EVENTS > 1
	    printf("ButtonRelease: x=%d y=%d\n",event.xbutton.x,event.xbutton.y);
	    fflush(stdout);
#endif	    
	    toggleHighlightRectangles(currentDisplayInfo,xOffset,yOffset);
	    XUngrabServer(display);
	    XUngrabPointer(display,CurrentTime);
	    *finalX =  initialX + xOffset;
	    *finalY =  initialY + yOffset;
	    return (inWindow);	/* Return from while(TRUE) */
	case MotionNotify:
	  /* Undraw old ones (XOR again to restore original) */
	    toggleHighlightRectangles(currentDisplayInfo,xOffset,yOffset);
	  /* Update current coordinates */
	    xOffset = event.xbutton.x - initialX;
	    yOffset = event.xbutton.y - initialY;
	  /* Draw new ones (XOR the outline) */
#if DEBUG_EVENTS > 1
	    printf("MotionNotify: x=%d y=%d\n",event.xbutton.x,event.xbutton.y);
	    fflush(stdout);
#endif	    
	    toggleHighlightRectangles(currentDisplayInfo,xOffset,yOffset);
	    break;
	default:
	    XtDispatchEvent(&event);
	}
    }
}

/*
 * Function to delete all widgets in an element and its children
 * Does a depth-first search for any children of composites
 * KE: Doesn't seem to use displayInfo
 */
void destroyElementWidgets(DlElement *element)
{
    DlElement *child;

  /* Do not delete the display */
    if (element->type == DL_Display) return;
  /* Delete any children if it is composite */
    if (element->type == DL_Composite) {
	child = FirstDlElement(element->structure.composite->dlElementList);
	while (child) {
	    DlElement *pE = child;
	    
	    if (pE->type == DL_Composite) {
		destroyElementWidgets(pE);
	    } else if (pE->widget) {
	      /* lookup widget of specified x,y,width,height and destroy */
		XtDestroyWidget(pE->widget);
		pE->widget = NULL;
	    }
	    child = child->next;
	}
    }
  /* Remove the widget */
    if (element->widget) {
	XtDestroyWidget(element->widget);
	element->widget = NULL;
    }
}

#if 0
/*
 * function to delete composite's children/grandchildren... WIDGETS ONLY
 *  this does a depth-first search for any descendent composites...
 *  KE: No longer used.  Replaced by destroyElementWidgets
 *  KE: Doesn't seem to use displayInfo
 */
void deleteWidgetsInComposite(DisplayInfo *displayInfo, DlElement *ele)
{
    DlElement *child;

    if (ele->type == DL_Composite) {

	child = FirstDlElement(ele->structure.composite->dlElementList);
	while (child) {
	    DlElement *pE = child;
	  /* if composite, delete any children */
	    if (pE->type == DL_Composite) {
		deleteWidgetsInComposite(displayInfo,pE);
	    } else
	      if (pE->widget) {
		/* lookup widget of specified x,y,width,height and destroy */
		  XtDestroyWidget(pE->widget);
		  pE->widget = NULL;
	      }
	    child = child->next;
	}
    }
}
#endif

void drawGrid(DisplayInfo *displayInfo)
{
    Drawable draw = displayInfo->drawingAreaPixmap;
    int x, y, xmax, ymax;
    int gridSpacing;
    Dimension width, height;
    int i, j, n;
    Arg args[2];

  /* Return if displayInfo is invalid */
    if(!displayInfo || !displayInfo->drawingArea || !draw) return;
    gridSpacing = displayInfo->gridSpacing;

  /* Get the size of the drawing area */
    n=0;
    XtSetArg(args[n],XmNwidth,&width); n++;
    XtSetArg(args[n],XmNheight,&height); n++;
    XtGetValues(displayInfo->drawingArea,args,n);
    xmax = width-1;
    ymax = height-1;

  /* Set the GC */
    XSetForeground(display,displayInfo->pixmapGC,
      displayInfo->drawingAreaForegroundColor);
    XSetBackground(display,displayInfo->pixmapGC,
      displayInfo->drawingAreaBackgroundColor);

  /* Draw grid */
    for(i=0; i < width; i+=gridSpacing) {
	for(j=0; j < height; j+=gridSpacing) {
	    XDrawPoint(display,draw,displayInfo->pixmapGC,i,j);
	}
    }
}

void copySelectedElementsIntoClipboard()
{
    DisplayInfo *cdi = currentDisplayInfo;
    DlElement *dlElement;

    if (!cdi) return;
    if (IsEmpty(cdi->selectedDlElementList)) return;

    if (!IsEmpty(clipboard)) {
	clearDlDisplayList(clipboard);
    }
  
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while (dlElement) {
	DlElement *element = dlElement->structure.element;
	if (element->type != DL_Display) {
	    DlElement *pE = element->run->create(element);
	    appendDlElement(clipboard,pE);
	}
	dlElement = dlElement->next;
    }
}

void copyElementsIntoDisplay()
{
    int i, j;
    DisplayInfo *cdi;
    Position displayX, displayY;
    int offsetX, offsetY;
    DlElement *elementPtr, *newElementsListHead;
    DlStructurePtr structurePtr;
    int deltaX, deltaY;
    Boolean moveWidgets;

    DlElement *element;
    DisplayInfo *displayInfo;
    DlElement *dlElement;

  /*
   * since elements are stored in clipboard in front-to-back order
   * they can be pasted/copied into display in clipboard index order
   */

  /* MDA -  since doPasting() can change currentDisplayInfo,
     clear old highlights now */
    displayInfo = displayInfoListHead->next;
    while (displayInfo) {
	currentDisplayInfo = displayInfo;
	unselectElementsInDisplay();
	displayInfo = displayInfo->next;
    }

    cdi = doPasting(&displayX,&displayY,&offsetX,&offsetY);
    deltaX = displayX + offsetX;
    deltaY = displayY + offsetY;

    if (cdi) {
      /* make sure pasted window is on top and has focus (and updated status) */
	XRaiseWindow(display,XtWindow(cdi->shell));
	XSetInputFocus(display,XtWindow(cdi->shell),RevertToParent,CurrentTime);
	currentDisplayInfo = cdi;
    } else {
	medmPrintf("\ncopyElementsIntoDisplay:  Can't determine current display\n");
	return;
    }

  /* Do actual element creation (with insertion into display list) */
    saveUndoInfo(cdi);
    clearDlDisplayList(cdi->selectedDlElementList);
    dlElement = FirstDlElement(clipboard);
    while (dlElement) {
	if (dlElement->type != DL_Display) {
	    DlElement *pE, *pSE;
	    pE = dlElement->run->create(dlElement);
	    if (pE) {
		appendDlElement(cdi->dlElementList,pE);
	      /* execute the structure */
		if (pE->run->move) {
		    pE->run->move(pE, deltaX, deltaY);
		}
		if (pE->run->execute) {
		    (pE->run->execute)(cdi, pE);
		}
		pSE = createDlElement(DL_Element,(XtPointer)pE,NULL);
		if (pSE) {
		    appendDlElement(cdi->selectedDlElementList,pSE);
		}
	    }
	}
	dlElement = dlElement->next;
    }
    highlightSelectedElements();
    if (cdi->selectedDlElementList->count == 1) {
	setResourcePaletteEntries();
    }
}

void deleteElementsInDisplay()
{
    int i;
    DisplayInfo *cdi = currentDisplayInfo;
    DlElement *dlElement;

    if (!cdi) return;
    if (IsEmpty(cdi->selectedDlElementList)) return;
    saveUndoInfo(cdi);

  /* Unhighlight selected elements */
    unhighlightSelectedElements();
  /* Traverse the elements in the selected element list */
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while (dlElement) {
      /* Get the structure (element of union is as good as any) */
	DlElement *pE = dlElement->structure.element;
	if (pE->type != DL_Display) {
	  /* Destroy its widgets */
	    destroyElementWidgets(pE);
	  /* Remove it from the list */
	    removeDlElement(cdi->dlElementList,pE);
	  /* Destroy it with its destroy method if there is one */
	    if(pE->run->destroy) pE->run->destroy(pE);
	}
	dlElement = dlElement->next;
    }
  /* Unselect any selected elements */
    unselectElementsInDisplay();

  /* Cleanup possible damage to non-widgets */
  /* (MDA) could use a new element-lookup based on region (write routine
   *      which returns all elements which intersect rather than are
   *      bounded by a given region) and do partial traversal based on
   *      those elements in start and end regions.  this could be much
   *      more efficient and not suffer from the "flash" updates
   */
    dmTraverseNonWidgetsInDisplayList(cdi);
}

/*
 * Unhighlights and unselects any selected elements, then clears the resource
 *   palette
 */
void unselectElementsInDisplay()
{
    DisplayInfo *cdi = currentDisplayInfo;

    if (!cdi) return;
  /* Clear resource palette */
    clearResourcePaletteEntries();
  /* Return if no selected elements */
    if (IsEmpty(cdi->selectedDlElementList)) return;
  /* Unhighlight and unselect */
    unhighlightSelectedElements();
    clearDlDisplayList(cdi->selectedDlElementList);
}

/*
 * Selects all renderable objects in display excluding the display itself
 */
void selectAllElementsInDisplay()
{
    DisplayInfo *cdi = currentDisplayInfo;
    Position x, y;
    Dimension width, height;
    DlElement *dlElement;

    if (!cdi) return;

  /* Unselect any selected elements */
    unselectElementsInDisplay();

    dlElement = FirstDlElement(cdi->dlElementList);
    while (dlElement) {
	if (dlElement->type != DL_Display) {
	    DlElement *pE;
	    pE = createDlElement(DL_Element,(XtPointer)dlElement,NULL);
	    if (pE) {
		appendDlElement(cdi->selectedDlElementList,pE);
	    }
	}
	dlElement = dlElement->next;
    }
    highlightSelectedElements();
    if (cdi->selectedDlElementList->count == 1) {
	setResourcePaletteEntries();
    }
}

/*
 * move elements further up (traversed first) in display list
 *  so that these are "behind" other objects
 */
void lowerSelectedElements()
{
    DisplayInfo *cdi = currentDisplayInfo;
    DlElement *pE = 0; /* pointer to the element in the selected element list */
    DlElement *pFirst = 0; /* pointer to the first element in the display element list */
    DlElement *pTemp;
    
#if DEBUG_EVENTS
    printf("\n[lowerSelectedElements:1]dlElementList :\n");
    dumpDlElementList(cdi->dlElementList);
    printf("\n[lowerSelectedElements:1]selectedDlElementList :\n");
    dumpDlElementList(cdi->selectedDlElementList);
#endif
    if (IsEmpty(cdi->selectedDlElementList)) return;
  /* If the temporary list does not exist, create it */
    if(!tmpDlElementList) {
	tmpDlElementList=createDlList();
	if(!tmpDlElementList) {
	    medmPrintf("\nlowerSelectedElements: Cannot create temporary element list\n");
	    return;
	}
    }
    clearDlDisplayList(tmpDlElementList);
    saveUndoInfo(cdi);

    unhighlightSelectedElements();
    
    pFirst = FirstDlElement(cdi->dlElementList);
    pE = LastDlElement(cdi->selectedDlElementList);
#if DEBUG_EVENTS > 1
    printf("\n[lowerSelectedElements] selectedDlElement list :\n");
    dumpDlElementList(cdi->selectedDlElementList);
    printf("\n[lowerSelectedElements] tmpDlElement list :\n");
    dumpDlElementList(tmpDlElementList);
#endif
    while (pE && (pE != cdi->selectedDlElementList->head)) {
	DlElement *pX = pE->structure.element;
#if DEBUG_EVENTS > 1
	printf("   (%s) x=%d y=%d width=%u height=%u\n",
	  elementType(pE->structure.element->type),
	  pE->structure.element->structure.composite->object.x,
	  pE->structure.element->structure.composite->object.y,
	  pE->structure.element->structure.composite->object.width,
	  pE->structure.element->structure.composite->object.height);
#endif
	pTemp = pE->prev;
	if (pX->type != DL_Display) {
	    removeDlElement(cdi->dlElementList,pX);
	    insertAfter(cdi->dlElementList,pFirst,pX);
	    removeDlElement(cdi->selectedDlElementList,pE);
	    insertDlElement(tmpDlElementList,pE);
	}
	pE = pTemp;
    }

  /* Unselect formerly selected elements */
    unselectElementsInDisplay();
  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(cdi);
  /* Select new ones */
    appendDlList(cdi->selectedDlElementList,tmpDlElementList);
    highlightSelectedElements();
    if (cdi->selectedDlElementList->count == 1) {
	setResourcePaletteEntries();
    }
  /* Cleanup temporary list */
    clearDlDisplayList(tmpDlElementList);
#if DEBUG_EVENTS
    printf("\n[lowerSelectedElements:2]dlElement list :\n");
    dumpDlElementList(cdi->dlElementList);
    printf("\n[lowerSelectedElements:2]selectedDlElementList :\n");
    dumpDlElementList(cdi->selectedDlElementList);
#endif
}

/*
 * move elements further down (traversed last) in display list
 *  so that these are "in front of" other objects
 */
void raiseSelectedElements()
{
    DisplayInfo *cdi = currentDisplayInfo;
    DlElement *pE = 0;
    DlElement *pTemp;
    
#if DEBUG_EVENTS
    printf("\n[raiseSelectedElements:1]dlElementList :\n");
    dumpDlElementList(cdi->dlElementList);
    printf("\n[raiseSelectedElements:1]selectedDlElementList :\n");
    dumpDlElementList(cdi->selectedDlElementList);
#endif
    if (IsEmpty(cdi->selectedDlElementList)) return;
  /* If the temporary list does not exist, create it */
    if(!tmpDlElementList) {
	tmpDlElementList=createDlList();
	if(!tmpDlElementList) {
	    medmPrintf("\nraiseSelectedElements: Cannot create temporary element list\n");
	    return;
	}
    }
    clearDlDisplayList(tmpDlElementList);
    saveUndoInfo(cdi);

    unhighlightSelectedElements();

    pE = FirstDlElement(cdi->selectedDlElementList);
    while (pE) {
	DlElement *pX = pE->structure.element;

	pTemp = pE->next;
	if (pX->type != DL_Display) {
	    removeDlElement(cdi->dlElementList,pX);
	    appendDlElement(cdi->dlElementList,pX);
	    removeDlElement(cdi->selectedDlElementList,pE);
	    appendDlElement(tmpDlElementList,pE);
	}
	pE = pTemp;
    }
  /* Unselect formerly selected elements */
    unselectElementsInDisplay();
  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(cdi);
  /* Select new ones */
    appendDlList(cdi->selectedDlElementList,tmpDlElementList);
    highlightSelectedElements();
    if (cdi->selectedDlElementList->count == 1) {
	setResourcePaletteEntries();
    }
  /* Cleanup temporary list */
    clearDlDisplayList(tmpDlElementList);
#if DEBUG_EVENTS
    printf("\n[raiseSelectedElements:2]dlElement list :\n");
    dumpDlElementList(cdi->dlElementList);
    printf("\n[raiseSelectedElements:2]selectedDlElementList :\n");
    dumpDlElementList(cdi->selectedDlElementList);
#endif
}

/*
 * ungroup any grouped (composite) elements which are currently selected.
 *  this removes the appropriate Composite element and moves any children
 *  to reflect their new-found autonomy...
 */
void ungroupSelectedElements()
{
    DisplayInfo *cdi = currentDisplayInfo;
    DlElement *ele, *child, *dlElement;
    int i;

    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while (dlElement) {
	ele = dlElement->structure.element;
	if (ele->type == DL_Composite) {
	    insertDlListAfter(cdi->dlElementList,ele->prev,
	      ele->structure.composite->dlElementList);
	    removeDlElement(cdi->dlElementList,ele);
	    free ((char *) ele->structure.composite->dlElementList);
	    free ((char *) ele->structure.composite);
	    free ((char *) ele);
	}
	dlElement = dlElement->next;
    }

  /* Unselect any selected elements */
    unselectElementsInDisplay();
  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
}

/*
 * align selected elements by top, bottom, left, or right edges
 */
void alignSelectedElements(int alignment)
{
    int i, j, minX, minY, maxX, maxY, deltaX, deltaY, x0, y0, xOffset, yOffset;
    DisplayInfo *cdi;
    DlElement *ele;
    DlElement *dlElement;

    if (!currentDisplayInfo) return;
    cdi = currentDisplayInfo;
    if (IsEmpty(cdi->selectedDlElementList)) return;
    if (NumberOfDlElement(cdi->selectedDlElementList) == 1) return;
    saveUndoInfo(cdi);

    minX = INT_MAX; minY = INT_MAX;
    maxX = INT_MIN; maxY = INT_MIN;

    unhighlightSelectedElements();

    dlElement = FirstDlElement(cdi->selectedDlElementList);

/* loop and get min/max (x,y) values */
    while (dlElement) {
	DlObject *po =
	  &(dlElement->structure.element->structure.rectangle->object);
	minX = MIN(minX, po->x);
	minY = MIN(minY, po->y);
	x0 = (po->x + po->width);
	maxX = MAX(maxX,x0);
	y0 = (po->y + po->height);
	maxY = MAX(maxY,y0);
	dlElement = dlElement->next;
    }
    deltaX = (minX + maxX)/2;
    deltaY = (minY + maxY)/2;

/* loop and set x,y values, and move if widgets */
    dlElement = LastDlElement(cdi->selectedDlElementList);
    while (dlElement != cdi->selectedDlElementList->head) {
	ele = dlElement->structure.element;

      /* can't move the display */
	if (ele->type != DL_Display) {
	    switch(alignment) {
	    case HORIZ_LEFT:
		xOffset = minX - ele->structure.rectangle->object.x;
		yOffset = 0;
		break;
	    case HORIZ_CENTER:
	      /* want   x + w/2 = dX  , therefore   x = dX - w/2   */
		xOffset = (deltaX - ele->structure.rectangle->object.width/2)
		  - ele->structure.rectangle->object.x;
		yOffset = 0;
		break;
	    case HORIZ_RIGHT:
	      /* want   x + w = maxX  , therefore   x = maxX - w  */
		xOffset = (maxX - ele->structure.rectangle->object.width)
                  - ele->structure.rectangle->object.x;
		yOffset = 0;
		break;
	    case VERT_TOP:
		xOffset = 0;
		yOffset = minY - ele->structure.rectangle->object.y;
		break;
	    case VERT_CENTER:
	      /* want   y + h/2 = dY  , therefore   y = dY - h/2   */
		xOffset = 0;
		yOffset = (deltaY - ele->structure.rectangle->object.height/2)
                  - ele->structure.rectangle->object.y;
		break;
	    case VERT_BOTTOM:
	      /* want   y + h = maxY  , therefore   y = maxY - h  */
		xOffset = 0;
		yOffset = (maxY - ele->structure.rectangle->object.height)
		  - ele->structure.rectangle->object.y;
		break;
	    }
	    if (ele->run->move) {
		ele->run->move(ele,xOffset,yOffset);
	    }
	    if (ele->widget) {
		XtMoveWidget(ele->widget,
		  (Position) ele->structure.rectangle->object.x,
		  (Position) ele->structure.rectangle->object.y);
	    }
	}
	dlElement = dlElement->prev;
    }
  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(cdi);

    highlightSelectedElements();
}

/*
 * Space selected elements horizontally
 */
void spaceSelectedElements(int plane)
{
    int i, deltaz, z, nele, gridSpacing;
    DisplayInfo *cdi;
    DlElement *pE;
    DlElement *dlElement;
    DlElement **earray;
    double *array;
    int *indx;

    if (!currentDisplayInfo) return;
    cdi = currentDisplayInfo;
    nele = NumberOfDlElement(cdi->selectedDlElementList);
    if(nele < 2) return;
    saveUndoInfo(cdi);
    gridSpacing=cdi->gridSpacing;

  /* Allocate space */
    earray = (DlElement **)calloc(nele,sizeof(DlElement *));
    if (!earray) return;
    array = (double *)calloc(nele,sizeof(double));
    if (!array) {
	medmPrintf("\nspaceSelectedElements: Memory allocation error\n");
	free((char *)earray);
	return;
    }
    indx = (int *)calloc(nele,sizeof(int));
    if (!indx) {
	medmPrintf("\nspaceSelectedElements: Memory allocation error\n");
	free((char *)earray);
	free((char *)array);
	return;
    }

    unhighlightSelectedElements();

  /* Loop and put elements and z into the arrays */
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    i=0;
    while (dlElement) {
	DlObject *po =
	  &(dlElement->structure.element->structure.rectangle->object);
	earray[i]=dlElement;
	if(plane == HORIZONTAL) {
	    array[i]=po->x;
	} else {
	    array[i]=po->y;
	}
	dlElement = dlElement->next;
	i++;
    }

#if 0
    printf("\nnele=%d\n",nele);
    for(i=0; i < nele; i++) {
	printf("array[%d]=%f indx[%d]=%d\n",i,array[i],i,indx[i]);
    }
#endif    
  /* Sort elements by position */
    hsort(array,indx,nele);
#if 0
    printf("nele=%d\n",nele);
    for(i=0; i < nele; i++) {
	printf("array[%d]=%f indx[%d]=%d\n",i,array[i],i,indx[i]);
    }
#endif    

  /* Loop and and move */
    z = -1;
    for(i=0; i < nele; i++) {
	dlElement = earray[indx[i]];
	pE = dlElement->structure.element;
      /* Can't move the display */
	if (pE->type != DL_Display) {
	  /* Get position of first element to start */
	    if(z < 0) {
		if(plane == HORIZONTAL) {
		    z = pE->structure.rectangle->object.x;
		} else {
		    z = pE->structure.rectangle->object.y;
		}
	    } else {
		if(plane == HORIZONTAL) {
		    deltaz = z - pE->structure.rectangle->object.x;
		    if (pE->run->move) {
			pE->run->move(pE,deltaz,0);
		    }
		} else {
		    deltaz = z - pE->structure.rectangle->object.y;
		    if (pE->run->move) {
			pE->run->move(pE,0,deltaz);
		    }
		}
		if (pE->widget) {
		    XtMoveWidget(pE->widget,
		      (Position) pE->structure.rectangle->object.x,
		      (Position) pE->structure.rectangle->object.y);
		}
	    }
	  /* Get next position */
	    if(plane == HORIZONTAL) {
		z += (pE->structure.rectangle->object.width + gridSpacing);
	    } else {
		z += (pE->structure.rectangle->object.height + gridSpacing);
	    }
	}
    }

  /* Free space */
    free((char *)earray);
    free((char *)array);
    free((char *)indx);

  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(cdi);

    highlightSelectedElements();
}

/*
 * Space selected elements in both directions
 */
void spaceSelectedElements2D(void)
{
    int y, y1, height, deltay, x, width, deltax, gridSpacing;
    int n, maxY, minY, deltaY, avgH, minX;
    int nrows, nrows1, xcen1, xleft, xright, ytop, ybottom, ycen;
    int i, irow, iarr;
    DisplayInfo *cdi;
    DlElement *pE;
    DlElement *dlElement, *dlElement1;
    DlElement ***earray = NULL;
    double **array = NULL;
    int **indx =NULL;
    int *nele = NULL;

    if (!currentDisplayInfo) return;
    cdi = currentDisplayInfo;
    n = NumberOfDlElement(cdi->selectedDlElementList);
    if(n < 2) return;
    saveUndoInfo(cdi);
    gridSpacing=cdi->gridSpacing;

  /* Determine the number of rows */
    minY = minX = INT_MAX;
    maxY = INT_MIN;
    nrows = 1;
    n = avgH = 0;
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while (dlElement) {
	pE = dlElement->structure.element;
      /* Don't include the display */
	if (pE->type != DL_Display) {
	    DlObject *po =  &(pE->structure.rectangle->object);
	    ytop = po->y;
	    height = po->height;
	    ybottom = ytop + height;
	    avgH += height;
	    if(ytop < minY) minY = ytop;
	    if(ybottom > maxY) maxY = ybottom;
	    n++;
	  /* Loop and find elements whose centers are between xleft and xright */
	    xleft = po->x;
	    if(xleft < minX) minX = xleft;
	    width = po->width;
	    xright = xleft + width;
	    nrows1 = 0;
	    dlElement1 = FirstDlElement(cdi->selectedDlElementList);
	    while (dlElement1) {
		pE = dlElement1->structure.element;
		if (pE->type != DL_Display) {
		    DlObject *po =  &(pE->structure.rectangle->object);
		    xcen1 = po->x + .5*po->width +.5;
		    if(xcen1 >= xleft && xcen1 <= xright) nrows1++;
		}
		dlElement1 = dlElement1->next;
	    }
	    if(nrows1 > nrows) nrows = nrows1;
	}
	dlElement = dlElement->next;
    }
    if(n < 1) return;
    avgH = (double)(avgH)/(double)(n)+.5;
    deltaY = (double)(maxY - minY)/(double)nrows + .99999;
    maxY = minY + nrows * deltaY;     /* (Adjust maxY) */

  /* Allocate array to hold the number of elements for each row */
    nele=(int *)calloc(nrows,sizeof(int));
    if(!nele) {
	medmPrintf("\nspaceSelectedElements2D: Memory allocation error\n");
	return;
    }

  /* Loop and count the number of elements in each row */
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while (dlElement) {
	pE = dlElement->structure.element;
      /* Don't include the display */
	if (pE->type != DL_Display) {
	    DlObject *po =  &(pE->structure.rectangle->object);
	    ytop = po->y;
	    ycen = po->y + .5*po->height +.5;
	    irow = (ycen - minY) / deltaY;
	    if(irow < 0) irow = 0;
	    else if(irow >= nrows) irow = nrows-1;
	    nele[irow]++;
	}
	dlElement = dlElement->next;
    }

  /* Allocate array storage */
    earray = (DlElement ***)calloc(nrows,sizeof(DlElement **));
    array = (double **)calloc(nrows,sizeof(double *));
    indx = (int **)calloc(nrows,sizeof(int *));
    if (!earray || !array || !indx) {
	medmPrintf("\nspaceSelectedElements2D: Memory allocation error\n");
	highlightSelectedElements();
	return;
    }
    for(i=0; i < nrows; i++) {
	if(nele[i] <= 0)  continue;
	earray[i] = (DlElement **)calloc(nele[i],sizeof(DlElement *));
	array[i] = (double *)calloc(nele[i],sizeof(double));
	indx[i] = (int *)calloc(nele[i],sizeof(int));
	if (!earray[i] || !array[i] || !indx[i]) {
	    medmPrintf("\nspaceSelectedElements2D: Memory allocation error\n");
	    highlightSelectedElements();
	    return;
	}
    }

  /* Fill arrays with current values before they are moved */
    for(i=0; i < nrows; i++) {
	if(nele[i] <= 0) continue;

      /* Loop and put elements and x into the arrays */
	dlElement = FirstDlElement(cdi->selectedDlElementList);
	iarr=0;
	while (dlElement) {
	    pE = dlElement->structure.element;
	  /* Don't include the display */
	    if (pE->type != DL_Display) {
		DlObject *po =  &(pE->structure.rectangle->object);
		ytop = po->y;
		ycen = po->y + .5*po->height +.5;
		irow = (ycen - minY) / deltaY;
		if(irow < 0) irow = 0;
		else if(irow >= nrows) irow = nrows-1;
		if(irow == i) {
		  /* This element is in this group */
		    earray[i][iarr]=dlElement;
		    array[i][iarr]=po->x;
		    iarr++;
		}
	    }
	    dlElement = dlElement->next;
	}
    }

    unhighlightSelectedElements();

  /* Move the elements */
    for(i=0; i < nrows; i++) {
	if(nele[i] <= 0) continue;
      /* Sort elements by position */
	hsort(array[i],indx[i],nele[i]);
      /* Loop over elements and and move */
	x = -1;
	y = minY + i*(avgH + gridSpacing);
	for(iarr=0; iarr < nele[i]; iarr++) {
	    dlElement = earray[i][indx[i][iarr]];
	    pE = dlElement->structure.element;
	  /* Can't move the display */
	    if (pE->type != DL_Display) {
	      /* Set position of first element */
		if(x < 0) {
		    x = minX;
		}
		deltax = x - pE->structure.rectangle->object.x;
		deltay = y - pE->structure.rectangle->object.y;
		if (pE->run->move) {
		    pE->run->move(pE,deltax,deltay);
		    if (pE->widget) {
			    XtMoveWidget(pE->widget,
			      (Position) pE->structure.rectangle->object.x,
			      (Position) pE->structure.rectangle->object.y);
		    }
		}
	      /* Get next position */
		x += (pE->structure.rectangle->object.width + gridSpacing);
	    }
	}
    }

  /* Free array storage */
    for(i=0; i < nrows; i++) {
	if(nele[i] <= 0)  continue;
	free((char *)earray[i]);
	free((char *)array[i]);
	free((char *)indx[i]);
    }
    free((char *)earray);
    free((char *)array);
    free((char *)indx);
    free((char *)nele);

  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(cdi);
    
    highlightSelectedElements();
}

/*
 * Align selected elements to grid
 */
void alignSelectedElementsToGrid(void)
{
    DisplayInfo *cdi = currentDisplayInfo;
    int gridSpacing;
    int x, y, x0, y0, x00, y00, xoff, yoff, redraw;
    DlElement *pE;
    DlElement *dlElement;

    if (!cdi) return;
    if (IsEmpty(cdi->selectedDlElementList)) return;
    saveUndoInfo(cdi);
    gridSpacing = cdi->gridSpacing;

    unhighlightSelectedElements();

  /* Loop and move the corners to grid */
    dlElement = LastDlElement(cdi->selectedDlElementList);
    while (dlElement != cdi->selectedDlElementList->head) {
	pE = dlElement->structure.element;
      /* Don't include the display */
	if (pE->type != DL_Display) {
	  /* Upper left */
	    x = pE->structure.rectangle->object.x;
	    x0 = (x/gridSpacing)*gridSpacing;
	    xoff = x - x0;
	    if(xoff > gridSpacing/2) xoff = xoff - gridSpacing;
	    x00 = x - xoff;

	    y = pE->structure.rectangle->object.y;
	    y0 = (y/gridSpacing)*gridSpacing;
	    yoff = y - y0;
	    if(yoff > gridSpacing/2) yoff = yoff - gridSpacing;
	    y00 = y - yoff;

	  /* Move it only if the new values are different */
	    redraw=0;
	    if(xoff != 0 || yoff != 0) {
		redraw=1;
		if (pE->run->move) {
		    pE->run->move(pE,-xoff,-yoff);
		}
	    }

	  /* Lower right */
	    x = x00 + pE->structure.rectangle->object.width;
	    x0 = (x/gridSpacing)*gridSpacing;
	    xoff = x - x0;
	    x0=x-xoff;
	    if(xoff > gridSpacing/2) xoff = xoff - gridSpacing;

	    y = y00 + pE->structure.rectangle->object.height;
	    y0 = (y/gridSpacing)*gridSpacing;
	    yoff = y - y0;
	    y0=y-yoff;
	    if(yoff > gridSpacing/2) yoff = yoff - gridSpacing;

	    if(xoff != 0 || yoff != 0) {
		redraw=1;
		if (pE->run->scale) {
		    pE->run->scale(pE,-xoff,-yoff);
		}
	    }
	    if(redraw) {
		if (pE->widget) {
		  /* Destroy the widget */
		    destroyElementWidgets(pE);
		  /* Recreate it */
		    pE->run->execute(cdi,pE);
		}
	    }
	}
	dlElement = dlElement->prev;
    }
  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(cdi);

  /* Update resource palette if there is only one element */
    if (NumberOfDlElement(cdi->selectedDlElementList) == 1) {
	currentElementType =
	  FirstDlElement(cdi->selectedDlElementList)
	  ->structure.element->type;
	setResourcePaletteEntries();
    }
    
    highlightSelectedElements();
}

/*
 * Make selected elements the same size
 */
void equalSizeSelectedElements(void)
{
    int i, j, n, avgW, avgH, xOffset, yOffset;
    DisplayInfo *cdi;
    DlElement *pE;
    DlElement *dlElement;

    if (!currentDisplayInfo) return;
    cdi = currentDisplayInfo;
    if (IsEmpty(cdi->selectedDlElementList)) return;
    saveUndoInfo(cdi);

    unhighlightSelectedElements();

/* Loop and get avgerage height and width values */
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    avgW = avgH = n = 0;
    while (dlElement) {
	pE = dlElement->structure.element;
      /* Don't include the display */
	if (pE->type != DL_Display) {
	    DlObject *po =  &(pE->structure.rectangle->object);
	    avgW += po->width;
	    avgH += po->height;
	    n++;
	}
	dlElement = dlElement->next;
    }
    if(n < 1) return;
    avgW = (double)(avgW)/(double)(n)+.5;
    avgH = (double)(avgH)/(double)(n)+.5;

/* Loop and set width and height values to average */
    dlElement = LastDlElement(cdi->selectedDlElementList);
    while (dlElement != cdi->selectedDlElementList->head) {
	pE = dlElement->structure.element;
      /* Don't include the display */
	if (pE->type != DL_Display) {
	    DlObject *po =  &(pE->structure.rectangle->object);
	    if (pE->run->scale) {
		pE->run->scale(pE,avgW - po->width,avgH - po->height);
	    }
	    if (pE->widget) {
	      /* Destroy the widget */
		destroyElementWidgets(pE);
	      /* Recreate it */
		pE->run->execute(cdi,pE);
	    }
	}
	dlElement = dlElement->prev;
    }
  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(cdi);

    highlightSelectedElements();
}

/*
 * Refresh display
 *  Redraw to get all widgets in proper stacking order
 *  (Uses dlElementList, not selectedDlElementList)
 */
void refreshDisplay(void)
{
    DisplayInfo *cdi;
    DlElement *pE;

    if (!currentDisplayInfo) return;
    cdi = currentDisplayInfo;
    if (IsEmpty(cdi->dlElementList)) return;

    unhighlightSelectedElements();

/* Loop and recreate widgets */
#if DEBUG_TRAVERSAL
	fprintf(stderr,"\n[refreshDisplay: cdi->dlElementList:\n");
	dumpDlElementList(cdi->dlElementList);
#endif
    pE = FirstDlElement(cdi->dlElementList);
    while (pE) {
      /* Don't include the display */
	if (pE->type != DL_Display) {
	    if (pE->widget) {
	      /* Destroy the widget */
		destroyElementWidgets(pE);
	      /* Recreate it */
		pE->run->execute(cdi,pE);
	    }
	}
	pE = pE->next;
    }
  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(cdi);
    
    highlightSelectedElements();
}

/*
 * moves specified <src> element to position just after specified <dst> element
 */
void moveElementAfter(DlElement *dst, DlElement *src, DlElement **tail)
{
    if (dst == src) return;
    if (src == *tail) {
	src->prev->next = NULL;
	*tail = src->prev;
    } else {
	src->prev->next = src->next;
	src->next->prev = src->prev;
    }
    if (dst == *tail) {
	dst->next = src;
	src->next = NULL;
	src->prev = dst;
	*tail = src;
    } else {
	dst->next->prev = src;
	src->next = dst->next;
	dst->next = src;
	src->prev = dst;
    }
}

/*
 * move all selected elements to position after specified element
 *	N.B.: this can move them up or down in relative visibility
 *	("stacking order" a la painter's algorithm)
 */
void  moveSelectedElementsAfterElement(DisplayInfo *displayInfo,
  DlElement *afterThisElement)
{
    int i;
    DisplayInfo *cdi;
    DlElement *afterElement;
    DlElement *dlElement;

    if (!displayInfo) return;
    cdi = displayInfo;
    if (IsEmpty(cdi->selectedDlElementList)) return;
    afterElement = afterThisElement;

    dlElement = LastDlElement(cdi->selectedDlElementList);
    while (dlElement != cdi->selectedDlElementList->head) {
      /* if display was selected, skip over it (can't raise/lower it) */
	DlElement *pE = dlElement->structure.element;
	if (pE->type != DL_Display) {
#if 0
	    moveElementAfter(afterElement,pE,&(cdi->dlElementListTail));
#endif
	    afterElement = afterElement->next;
	}
	dlElement = dlElement->prev;
    }
}

/*
 * return Channel ptr given a widget id
 */
UpdateTask *getUpdateTaskFromWidget(Widget widget)
{
    DisplayInfo *displayInfo;
    UpdateTask *pt;

    if (!(displayInfo = dmGetDisplayInfoFromWidget(widget)))
      return NULL; 

    pt = displayInfo->updateTaskListHead.next; 
    while (pt) {
      /* Note : vong
       * Below it is a very ugly way to dereference the widget pointer.
       * It assumes that the first element in the clientData is a pointer
       * to a DlElement structure.  However, if a SIGSEG or SIGBUS occurs,
       * please recheck the structure which pt->clientData points
       * at.
       */
	if ((*(((DlElement **) pt->clientData)))->widget == widget) {
	    return pt;
	}
	pt = pt->next;
    }
    return NULL;
}

/*
 * return UpdateTask ptr given a DisplayInfo* and x,y positions
 */
UpdateTask *getUpdateTaskFromPosition(DisplayInfo *displayInfo, int x, int y)
{
    UpdateTask *ptu, *ptuSaved = NULL;
    int minWidth, minHeight;
  
    if (displayInfo == (DisplayInfo *)NULL) return NULL;

    minWidth = INT_MAX;	 	/* according to XPG2's values.h */
    minHeight = INT_MAX;

    ptu = displayInfo->updateTaskListHead.next;
    while (ptu) {
	if (x >= (int)ptu->rectangle.x &&
	  x <= (int)ptu->rectangle.x + (int)ptu->rectangle.width &&
	  y >= (int)ptu->rectangle.y &&
	  y <= (int)ptu->rectangle.y + (int)ptu->rectangle.height) {
	  /* eligible element, see if smallest so far */
	    if ((int)ptu->rectangle.width < minWidth &&
	      (int)ptu->rectangle.height < minHeight) {
		minWidth = ptu->rectangle.width;
		minHeight = ptu->rectangle.height;
		ptuSaved = ptu;
	    }
	}
	ptu = ptu->next;
    }
    return ptuSaved;
}

/*
 * generate a name-value table from the passed-in argument string
 *	returns a pointer to a NameValueTable as the function value,
 *	and the number of values in the second parameter
 *	Syntax: argsString: "a=b,c=d,..."
 */
NameValueTable *generateNameValueTable(char *argsString, int *numNameValues)
{
    char *copyOfArgsString,  *name, *value;
    char *s1;
    char nameEntry[80], valueEntry[80];
    int i, j, tableIndex, numPairs, numEntries;
    NameValueTable *nameTable;
    Boolean first;


    nameTable = NULL;
    copyOfArgsString = NULL;

    if (argsString != NULL) {

	copyOfArgsString = STRDUP(argsString);
      /* see how many a=b name/value pairs are in the string */
	numPairs = 0;
	i = 0;
	while (copyOfArgsString[i++] != '\0')
	  if (copyOfArgsString[i] == '=') numPairs++;


	tableIndex = 0;
	first = True;
	for (numEntries = 0; numEntries < numPairs; numEntries++) {

	  /* at least one pair, proceed */
	    if (first) {
		first = False;
		nameTable = (NameValueTable *) calloc(1,
		  numPairs*sizeof(NameValueTable));
	      /* name = value, name = value, ...  therefore */
	      /* name delimited by "=" and value delimited by ","  */
		s1 = copyOfArgsString;
	    } else {
		s1 = NULL;
	    }
	    name = strtok(s1,"=");
	    value = strtok(NULL,",");
	    if (name != NULL && value != NULL) {
	      /* found legitimate name/value pair, put in table */
		j = 0;
		for (i = 0; i < (int) strlen(name); i++) {
		    if (!isspace(name[i]))
		      nameEntry[j++] =  name[i];
		}
		nameEntry[j] = '\0';
		j = 0;
		for (i = 0; i < (int) strlen(value); i++) {
		    if (!isspace(value[i]))
		      valueEntry[j++] =  value[i];
		}
		valueEntry[j] = '\0';
		nameTable[tableIndex].name = STRDUP(nameEntry);
		nameTable[tableIndex].value = STRDUP(valueEntry);
		tableIndex++;
	    }
	}
	if (copyOfArgsString) free(copyOfArgsString);

    } else {

      /* no pairs */

    }
    *numNameValues = tableIndex;
    return (nameTable);
}

/*
 * lookup name in name-value table, return associated value (or NULL if no
 *	match)
 */
char *lookupNameValue(NameValueTable *nameValueTable, int numEntries, char *name)
{
    int i;
    if (nameValueTable != NULL && numEntries > 0) {
	for (i = 0; i < numEntries; i++)
	  if (!strcmp(nameValueTable[i].name,name))
	    return (nameValueTable[i].value);
    }

    return (NULL);

}

/*
 * free the name value table
 *	first, all the strings pointed to by its entries
 *	then the table itself
 */
void freeNameValueTable(NameValueTable *nameValueTable, int numEntries)
{
    int i;
    if (nameValueTable != NULL) {
	for (i = 0; i < numEntries; i++) {
	    if (nameValueTable[i].name != NULL) free ((char *)nameValueTable[i].name);
	    if (nameValueTable[i].value != NULL) free ((char *)
	      nameValueTable[i].value);
	}
	free ((char *)nameValueTable);
    }

}


/*
 * utility function to perform macro substitutions on input string, putting
 *	substituted string in specified output string (up to sizeOfOutputString
 *	bytes)
 */
void performMacroSubstitutions(DisplayInfo *displayInfo,
  char *inputString, char *outputString, int sizeOfOutputString)
{
    int i, j, k, n;
    char *value, name[MAX_TOKEN_LENGTH];

    outputString[0] = '\0';
    if (!displayInfo) {
	strncpy(outputString,inputString,sizeOfOutputString-1);
	outputString[sizeOfOutputString-1] = '\0';
	return;
    }

    i = 0; j = 0; k = 0;
    if (inputString && strlen(inputString) > (size_t)0) {
	while (inputString[i] != '\0' && j < sizeOfOutputString-1) {
	    if ( inputString[i] != '$') {
		outputString[j++] = inputString[i++];
	    } else {
	      /* found '$', see if followed by '(' */
		if (inputString[i+1] == '(' ) {
		    i = i+2;
		    while (inputString[i] != ')'  && inputString[i] != '\0' ) {
			name[k++] = inputString[i++];
		    }
		    name[k] = '\0';
		  /* now lookup macro */
		    value = lookupNameValue(displayInfo->nameValueTable,
		      displayInfo->numNameValues,name);
		    if (value) {
			n = 0;
			while (value[n] != '\0' && j < sizeOfOutputString-1)
			  outputString[j++] = value[n++];
		    }
		  /* to skip over ')' */
		    i++;
		} else {
		    outputString[j++] = inputString[i++];
		}
	    }
	    outputString[j] = '\0';
	}

    } else {
	outputString[0] = '\0';
    }
    if (j >= sizeOfOutputString-1) {
	medmPostMsg("performMacroSubstitutions: Substitutions failed\n"
	  "  Output buffer not large enough\n");
    }
}

/*
 * colorMenuBar - get VUE and its "ColorSetId" straightened out...
 *   color the passed in widget (usually menu bar) and its children
 *   to the specified foreground/background colors
 */
void colorMenuBar(Widget widget, Pixel fg, Pixel bg)
{
    Cardinal numChildren;
    WidgetList children;
    Arg args[4];
    int i;
    Pixel localFg, top, bottom,select;

    XtSetArg(args[0],XmNchildren,&children);
    XtSetArg(args[1],XmNnumChildren,&numChildren);
    XtGetValues(widget,args,2);

    XmGetColors(XtScreen(widget),cmap,bg,&localFg,&top,&bottom,&select);
    XtSetArg(args[0],XmNforeground,fg);
    XtSetArg(args[1],XmNbackground,bg);

    XtSetArg(args[2],XmNtopShadowColor,top);
    XtSetArg(args[3],XmNbottomShadowColor,bottom);
    XtSetValues(widget,args,4);

    for (i = 0; i < numChildren; i++) {
	XtSetValues(children[i],args,2);
    }
}

#ifdef __cplusplus
void questionDialogCb(Widget, XtPointer clientData, XtPointer callbackStruct)
#else
void questionDialogCb(Widget widget, XtPointer clientData, XtPointer callbackStruct)
#endif
{
    DisplayInfo *displayInfo = (DisplayInfo *) clientData;
    XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *) callbackStruct;
    switch (cbs->reason) {
    case XmCR_OK:
	displayInfo->questionDialogAnswer = 1;
	break;
    case XmCR_CANCEL:
	displayInfo->questionDialogAnswer = 2;
	break;
    case XmCR_HELP:
	displayInfo->questionDialogAnswer = 3;
	break;
    default :
	displayInfo->questionDialogAnswer = -1;
	break;
    }
}

/*
 * function to create (if necessary), set and popup a display's question dialog
 */
void dmSetAndPopupQuestionDialog(DisplayInfo    *displayInfo,
  char           *message,
  char           *okBtnLabel,
  char           *cancelBtnLabel,
  char           *helpBtnLabel)
{
    XmString xmString;
    XEvent event;

  /* create the dialog if necessary */

    if (displayInfo->questionDialog == NULL) {
      /* this doesn't seem to be working (and should check if MWM is running) */
	displayInfo->questionDialog = XmCreateQuestionDialog(displayInfo->shell,"questionDialog",NULL,0);
	XtVaSetValues(displayInfo->questionDialog, XmNdialogStyle,XmDIALOG_APPLICATION_MODAL, NULL);
	XtVaSetValues(XtParent(displayInfo->questionDialog),XmNtitle,"Question ?",NULL);
	XtAddCallback(displayInfo->questionDialog,XmNokCallback,questionDialogCb,displayInfo);
	XtAddCallback(displayInfo->questionDialog,XmNcancelCallback,questionDialogCb,displayInfo);
	XtAddCallback(displayInfo->questionDialog,XmNhelpCallback,questionDialogCb,displayInfo);
    }
    if (message == NULL) return;
    xmString = XmStringCreateLtoR(message,XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(displayInfo->questionDialog,XmNmessageString,xmString,NULL);
    XmStringFree(xmString);
    if (okBtnLabel) {
	xmString = XmStringCreateLtoR(okBtnLabel,XmFONTLIST_DEFAULT_TAG);
	XtVaSetValues(displayInfo->questionDialog,XmNokLabelString,xmString,NULL);
	XmStringFree(xmString);
	XtManageChild(XmMessageBoxGetChild(displayInfo->questionDialog,XmDIALOG_OK_BUTTON));
    } else {
	XtUnmanageChild(XmMessageBoxGetChild(displayInfo->questionDialog,XmDIALOG_OK_BUTTON));
    }
    if (cancelBtnLabel) {
	xmString = XmStringCreateLtoR(cancelBtnLabel,XmFONTLIST_DEFAULT_TAG);
	XtVaSetValues(displayInfo->questionDialog,XmNcancelLabelString,xmString,NULL);
	XmStringFree(xmString);
	XtManageChild(XmMessageBoxGetChild(displayInfo->questionDialog,XmDIALOG_CANCEL_BUTTON));
    } else {
	XtUnmanageChild(XmMessageBoxGetChild(displayInfo->questionDialog,XmDIALOG_CANCEL_BUTTON));
    }
    if (helpBtnLabel) {
	xmString = XmStringCreateLtoR(helpBtnLabel,XmFONTLIST_DEFAULT_TAG);
	XtVaSetValues(displayInfo->questionDialog,XmNhelpLabelString,xmString,NULL);
	XmStringFree(xmString);
	XtManageChild(XmMessageBoxGetChild(displayInfo->questionDialog,XmDIALOG_HELP_BUTTON));
    } else {
	XtUnmanageChild(XmMessageBoxGetChild(displayInfo->questionDialog,XmDIALOG_HELP_BUTTON));
    }
    displayInfo->questionDialogAnswer = 0;
    XtManageChild(displayInfo->questionDialog);
    XSync(display,FALSE);
  /* force Modal (blocking dialog) */
    XtAddGrab(XtParent(displayInfo->questionDialog),True,False);
    XmUpdateDisplay(XtParent(displayInfo->questionDialog));
    while (!displayInfo->questionDialogAnswer || XtAppPending(appContext)) {
	XtAppNextEvent(appContext,&event);
	XtDispatchEvent(&event);
    }
    XtUnmanageChild(displayInfo->questionDialog);
    XtRemoveGrab(XtParent(displayInfo->questionDialog));
}

#ifdef __cplusplus
void warningDialogCb(Widget,
  XtPointer clientData,
  XtPointer callbackStruct)
#else
void warningDialogCb(Widget widget,
  XtPointer clientData,
  XtPointer callbackStruct)
#endif
{
    DisplayInfo *displayInfo = (DisplayInfo *) clientData;
    XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *) callbackStruct;
    switch (cbs->reason) {
    case XmCR_OK:
	displayInfo->warningDialogAnswer = 1;
	break;
    case XmCR_CANCEL:
	displayInfo->warningDialogAnswer = 2;
	break;
    case XmCR_HELP:
	displayInfo->warningDialogAnswer = 3;
	break;
    default :
	displayInfo->warningDialogAnswer = -1;
	break;
    }
}

/*
 * function to create (if necessary), set and popup a display's warning dialog
 */
void dmSetAndPopupWarningDialog(DisplayInfo    *displayInfo,
  char           *message,
  char           *okBtnLabel,
  char           *cancelBtnLabel,
  char           *helpBtnLabel)
{
    XmString xmString;
    XEvent event;

/* create the dialog if necessary */

    if (displayInfo->warningDialog == NULL) {
      /* this doesn't seem to be working (and should check if MWM is running) */
	displayInfo->warningDialog =
	  XmCreateWarningDialog(displayInfo->shell,"warningDialog",NULL,0);
	XtVaSetValues(displayInfo->warningDialog,XmNdialogStyle,XmDIALOG_APPLICATION_MODAL,NULL);
	XtVaSetValues(XtParent(displayInfo->warningDialog),XmNtitle,"Warning !",NULL);
	XtAddCallback(displayInfo->warningDialog,XmNokCallback,warningDialogCb,displayInfo);
	XtAddCallback(displayInfo->warningDialog,XmNcancelCallback,warningDialogCb,displayInfo);
	XtAddCallback(displayInfo->warningDialog,XmNhelpCallback,warningDialogCb,displayInfo);
    }
    if (message == NULL) return;
    xmString = XmStringCreateLtoR(message,XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(displayInfo->warningDialog,XmNmessageString,xmString,NULL);
    XmStringFree(xmString);
    if (okBtnLabel) {
	xmString = XmStringCreateLtoR(okBtnLabel,XmFONTLIST_DEFAULT_TAG);
	XtVaSetValues(displayInfo->warningDialog,XmNokLabelString,xmString,NULL);
	XmStringFree(xmString);
	XtManageChild(XmMessageBoxGetChild(displayInfo->warningDialog,XmDIALOG_OK_BUTTON));
    } else {
	XtUnmanageChild(XmMessageBoxGetChild(displayInfo->warningDialog,XmDIALOG_OK_BUTTON));
    }
    if (cancelBtnLabel) {
	xmString = XmStringCreateLtoR(cancelBtnLabel,XmFONTLIST_DEFAULT_TAG);
	XtVaSetValues(displayInfo->warningDialog,XmNcancelLabelString,xmString,NULL);
	XmStringFree(xmString);
	XtManageChild(XmMessageBoxGetChild(displayInfo->warningDialog,XmDIALOG_CANCEL_BUTTON));
    } else {
	XtUnmanageChild(XmMessageBoxGetChild(displayInfo->warningDialog,XmDIALOG_CANCEL_BUTTON));
    }
    if (helpBtnLabel) {
	xmString = XmStringCreateLtoR(helpBtnLabel,XmFONTLIST_DEFAULT_TAG);
	XtVaSetValues(displayInfo->warningDialog,XmNhelpLabelString,xmString,NULL);
	XmStringFree(xmString);
	XtManageChild(XmMessageBoxGetChild(displayInfo->warningDialog,XmDIALOG_HELP_BUTTON));
    } else {
	XtUnmanageChild(XmMessageBoxGetChild(displayInfo->warningDialog,XmDIALOG_HELP_BUTTON));
    }
    displayInfo->warningDialogAnswer = 0;
    XtManageChild(displayInfo->warningDialog);
    XSync(display,FALSE);
  /* force Modal (blocking dialog) */
    XtAddGrab(XtParent(displayInfo->warningDialog),True,False);
    XmUpdateDisplay(XtParent(displayInfo->warningDialog));
    while (!displayInfo->warningDialogAnswer || XtAppPending(appContext)) {
	XtAppNextEvent(appContext,&event);
	XtDispatchEvent(&event);
    }
    XtRemoveGrab(XtParent(displayInfo->warningDialog));
    XtUnmanageChild(displayInfo->warningDialog);
}

void closeDisplay(Widget w) {
    DisplayInfo *newDisplayInfo;
    newDisplayInfo = dmGetDisplayInfoFromWidget(w);
    if (newDisplayInfo == currentDisplayInfo) {
      /* Unselect any selected elements */
	unselectElementsInDisplay();
	currentDisplayInfo = NULL;
    }
    if (newDisplayInfo->hasBeenEditedButNotSaved) {
	char warningString[2*MAX_FILE_CHARS];
	char *tmp, *tmp1;

	strcpy(warningString,"Save before closing display :\n");
	tmp = tmp1 = newDisplayInfo->dlFile->name;
	while (*tmp != '\0')
	  if (*tmp++ == '/') tmp1 = tmp;
	strcat(warningString,tmp1);
	dmSetAndPopupQuestionDialog(newDisplayInfo,warningString,"Yes","No","Cancel");
	switch (newDisplayInfo->questionDialogAnswer) {
	case 1 :
	  /* Yes, save display */
	    if (medmSaveDisplay(newDisplayInfo,
	      newDisplayInfo->dlFile->name,True) == False) return;
	    break;
	case 2 :
	  /* No, return */
	    break;
	case 3 :
	  /* Don't close display */
	    return;
	default :
	    return;
	}
    }
  /* remove newDisplayInfo from displayInfoList and cleanup */
    dmRemoveDisplayInfo(newDisplayInfo);
    if (displayInfoListHead->next == NULL) {
	disableEditFunctions();
    }
}

#ifdef __COLOR_RULE_H__
Pixel extractColor(DisplayInfo *displayInfo, double value, int colorRule, int defaultColor) {
    setOfColorRule_t *color = &(setOfColorRule[colorRule]);
    int i;
    for (i = 0; i<MAX_COLOR_RULES; i++) {
	if (value <=color->rule[i].upperBoundary
	  && value >= color->rule[i].lowerBoundary) {
	    return displayInfo->dlColormap[color->rule[i].colorIndex];
	}
    }
    return displayInfo->dlColormap[defaultColor];
}
#endif

#ifdef __TED__
void GetWorkSpaceList(Widget w) {
    Atom *paWs;
    char *pchWs;
    DtWsmWorkspaceInfo *pWsInfo;
    unsigned long numWorkspaces;

    if (DtWsmGetWorkspaceList(XtDisplay(w),
      XRootWindowOfScreen(XtScreen(w)),
      &paWs, (int *)&numWorkspaces) == Success)
	{
	    int i;
	    for (i=0; i<numWorkspaces; i++) {
		DtWsmGetWorkspaceInfo(XtDisplay(w),
		  XRootWindowOfScreen(XtScreen(w)),
		  paWs[i],
		  &pWsInfo);
		pchWs = (char *) XmGetAtomName (XtDisplay(w),
		  pWsInfo->workspace);
		printf ("workspace %d : %s\n",pchWs);
	    }
	}
}
#endif

/* Convert hex digits to ascii */
static char hex_digit_to_ascii[16]={'0','1','2','3','4','5','6','7','8','9',
				    'a','b','c','d','e','f'};
 
int localCvtLongToHexString(
  long source,
  char  *pdest)
{
    long  val,temp;
    char  digit[10];
    int   i,j;
    char  *startAddr = pdest;
 
    *pdest++ = '0';
    *pdest++ = 'x';
    if(source==0) {
	*pdest++ = '0';
    } else {
	val = source;
	temp = 0xf & val;
	val = val >> 4;
	digit[0] = hex_digit_to_ascii[temp];
	if (val < 0) val = val & 0xfffffff;
	for (i=1; val!=0; i++) {
	    temp = 0xf & val;
	    val = val >> 4;
	    digit[i] = hex_digit_to_ascii[temp];
	}
	for(j=i-1; j>=0; j--) {
	    *pdest++ = digit[j];
	}

    }
    *pdest = 0;
    return((int)(pdest-startAddr));
}

/* Makes a new, empty list */
DlList *createDlList() {
    DlList *dlList = malloc(sizeof(DlList));
    if (dlList) {
	dlList->head = &(dlList->data);
	dlList->tail = dlList->head;
	dlList->head->next = NULL;
	dlList->head->prev = NULL;
	dlList->count = 0;
    }
    return dlList;
}

/* Put element at the end of the list (Doesn't create element) */
void appendDlElement(DlList *l, DlElement *p)
{
    p->prev = l->tail;
    p->next = NULL;
    l->tail->next = p;
    l->tail = p;
    l->count++;
}

/* Put element at the head of the list */
void insertDlElement(DlList *l, DlElement *p)
{
    p->prev = l->head;
    p->next = l->head->next;
    if (l->tail == l->head) {
	l->tail = p;
    } else {
	l->head->next->prev = p;
    }
    l->head->next = p;
    l->count++;
}

/* Adjusts pointers to include (Doesn't create element) */
void insertAfter(DlList *l, DlElement *p1, DlElement *p2)
{
    p2->prev = p1;
    p2->next = p1->next;
    if (l->tail == p1) {
	l->tail = p2;
    } else {
	p1->next->prev = p2;
    }
    p1->next = p2;
    l->count++;
}

/* Moves elements from src to after an element in dest */
void insertDlListAfter(DlList *pDest, DlElement *p, DlList *pSrc)
{
    if (IsEmpty(pSrc)) return;
    FirstDlElement(pSrc)->prev = p;
    LastDlElement(pSrc)->next = p->next;
    if (pDest->tail == p) {
	pDest->tail = LastDlElement(pSrc);
    } else {
	p->next->prev = LastDlElement(pSrc);
    }
    p->next = FirstDlElement(pSrc);
    pDest->count += pSrc->count;
    emptyDlList(pSrc);
}

/* Moves elements from src to the end of dest */
void appendDlList(DlList *pDest, DlList *pSrc)
{
    DlElement *p;
    if (pSrc->count <= 0) return;
    pDest->count += pSrc->count;
    pSrc->count = 0;
    p = pSrc->head->next;
    pSrc->head->next = NULL;
    p->prev = pDest->tail;
    pDest->tail->next = p;
    pDest->tail = pSrc->tail;
    pSrc->tail = pSrc->head;
}

/* Adjusts pointers to be empty list (Doesn't free elements) */
void emptyDlList(DlList *l)
{
    l->head->next = 0;
    l->tail = l->head;
    l->count = 0;
}

/* Adjusts pointers to eliminate element (Doesn't free element) */
void removeDlElement(DlList *l, DlElement *p)
{
    l->count--;
    p->prev->next = p->next;
    if (l->tail == p) {
	l->tail = p->prev;
    } else {
	p->next->prev = p->prev;
    }
    p->next = p->prev = 0;
}

/* Prints the elements in a list */
void dumpDlElementList(DlList *l)
{
    DlElement *p = 0;
    int i = 0;
    printf("Number of Elements = %d\n",l->count);
    p = FirstDlElement(l);
    while (p) {
	if (p->type == DL_Element) {
	    printf("%03d (%s) x=%d y=%d width=%u height=%u\n",i++,
	      elementType(p->structure.element->type),
	      p->structure.element->structure.composite->object.x,
	      p->structure.element->structure.composite->object.y,
	      p->structure.element->structure.composite->object.width,
	      p->structure.element->structure.composite->object.height);
	} else {
	    printf("%03d %s x=%d y=%d width=%u height=%u\n",i++,
	      elementType(p->type),
	      p->structure.composite->object.x,
	      p->structure.composite->object.y,
	      p->structure.composite->object.width,
	      p->structure.composite->object.height);
	}
	p = p->next;
    }
    return;
}

/*
 * Heap sort routine
 *  Sorts an array of length n into ascending order and puts the sorted
 *    indices in indx
 */
void hsort(double array[], int indx[], int n)
{
    int l,j,ir,indxt,i;
    double q;

  /* All done if none or one element */
    if(n == 0) return;
    if(n == 1) {
	indx[0]=0;
	return;
    }
    
  /* Initialize indx array */
    for(j=0; j < n; j++) indx[j]=j;
  /* Loop over elements */
    l=(n>>1);
    ir=n-1;
    for(;;) {
	if(l > 0) q=array[(indxt=indx[--l])];
	else {
	    q=array[(indxt=indx[ir])];
	    indx[ir]=indx[0];
	    if(--ir == 0) {
		indx[0]=indxt;
		return;
	    }
	}
	i=l;
	j=(l<<1)+1;
	while(j <= ir) {
	    if(j < ir && array[indx[j]] < array[indx[j+1]]) j++;
	    if(q < array[indx[j]]) {
		indx[i]=indx[j];
		j+=((i=j)+1);
		
	    }
	    else break;
	}
	indx[i]=indxt;
    }
}

/* Undo Routines */

/* Allocate UndoInfo struct (if necessary) and element list inside it */
void createUndoInfo(DisplayInfo *displayInfo)
{
    UndoInfo *undoInfo;

    if(!displayInfo) return;
    if(displayInfo->undoInfo) {
	clearUndoInfo(displayInfo);
    } else {
	displayInfo->undoInfo = (UndoInfo *)calloc(1,sizeof(UndoInfo));
    }
    undoInfo = displayInfo->undoInfo;

    undoInfo->dlElementList = createDlList();
    if (!undoInfo->dlElementList) {
	medmPrintf("\ncreateUndoInfo: Cannot create element list\n");
	free(undoInfo);
	displayInfo->undoInfo = NULL;
    }
}

/* Free UndoInfo struct and element list inside it */
void destroyUndoInfo(DisplayInfo *displayInfo)
{
    if(!displayInfo) return;
    if(!displayInfo->undoInfo) return;
    clearUndoInfo(displayInfo);
    free((char *)displayInfo->undoInfo);
    displayInfo->undoInfo = NULL;
}

/* Free element list inside UndoInfo struct (Indicates undo is not available) */
void clearUndoInfo(DisplayInfo *displayInfo)
{
    UndoInfo *undoInfo;

    if(!displayInfo) return;
    undoInfo = displayInfo->undoInfo;
    if(!undoInfo) return;
  /*    clearDlDisplayList(undoInfo->dlElementList); */
    free ((char *) undoInfo->dlElementList);
}

/* Save Undo information */
void saveUndoInfo(DisplayInfo *displayInfo)
{
#ifdef UNDO    
    UndoInfo *undoInfo;
    DlElement *dlElement;

    if(!displayInfo) return;
    undoInfo = displayInfo->undoInfo;
    if(!undoInfo) {
	createUndoInfo(displayInfo);
	undoInfo = displayInfo->undoInfo;
    }
    if(!undoInfo) return;

#if DEBUG_UNDO
    fprintf(stderr,"\nSAVE\n");
    fprintf(stderr,"\n[saveUndoInfo: displayInfo->dlElementList(Before):\n");
    dumpDlElementList(displayInfo->dlElementList);
    fprintf(stderr,"\n[saveUndoInfo: undoInfo->dlElementList(Before):\n");
    dumpDlElementList(undoInfo->dlElementList);
#endif

    if (!IsEmpty(undoInfo->dlElementList)) {
	clearDlDisplayList(undoInfo->dlElementList);
    }
    if (IsEmpty(displayInfo->dlElementList)) return;
  
    dlElement = FirstDlElement(displayInfo->dlElementList);
    while (dlElement) {
	DlElement *element = dlElement;
	if (element->type != DL_Display) {
	    DlElement *pE = element->run->create(element);
	    appendDlElement(undoInfo->dlElementList,pE);
	}
	dlElement = dlElement->next;
    }

#if DEBUG_UNDO
    fprintf(stderr,"\n[saveUndoInfo: displayInfo->dlElementList(After):\n");
    dumpDlElementList(displayInfo->dlElementList);
    fprintf(stderr,"\n[saveUndoInfo: undoInfo->dlElementList(After):\n");
    dumpDlElementList(undoInfo->dlElementList);
    fprintf(stderr,"\n");
#endif

    undoInfo->gridSpacing = displayInfo->gridSpacing;
    undoInfo->gridOn = displayInfo->gridOn;
    undoInfo->drawingAreaBackgroundColor = displayInfo->drawingAreaBackgroundColor;
    undoInfo->drawingAreaForegroundColor = displayInfo->drawingAreaForegroundColor;
#endif
}

/* Restore from saved undo information and save current information for redo */
void restoreUndoInfo(DisplayInfo *displayInfo)
{
#ifdef UNDO    
    UndoInfo *undoInfo;
    DlElement *dlElement;

  /* Check if undo is available */
    if(!displayInfo) {
	XBell(display,50);
	return;
    }
    undoInfo = displayInfo->undoInfo;
    if(!undoInfo) {
	XBell(display,50);
	return;
    }

  /* If the temporary list does not exist, create it */
    if(!tmpDlElementList) {
	tmpDlElementList=createDlList();
	if(!tmpDlElementList) {
	    medmPrintf("\nrestoreUndoInfo: Cannot create temporary element list\n");
	    XBell(display,50); XBell(display,50); XBell(display,50);
	    return;
	}
    }

  /* Unselect any selected elements since we aren't keeping track of them */
    unselectElementsInDisplay();
    
#if DEBUG_UNDO
    fprintf(stderr,"\nRESTORE\n");
    fprintf(stderr,"\n[restoreUndoInfo: displayInfo->dlElementList(Before):\n");
    dumpDlElementList(displayInfo->dlElementList);
    fprintf(stderr,"\n[restoreUndoInfo: undoInfo->dlElementList(Before):\n");
    dumpDlElementList(undoInfo->dlElementList);
    fprintf(stderr,"\n[restoreUndoInfo: tmpDlElementList(Before):\n");
    dumpDlElementList(tmpDlElementList);
#endif

  /* Save current list in temporary list */
    if (!IsEmpty(tmpDlElementList)) {
	clearDlDisplayList(tmpDlElementList);
    }
    if (!IsEmpty(displayInfo->dlElementList)) {
	dlElement = FirstDlElement(displayInfo->dlElementList);
	while (dlElement) {
	    DlElement *element = dlElement;
	    if (element->type != DL_Display) {
		DlElement *pE = element->run->create(element);
		appendDlElement(tmpDlElementList,pE);
	    }
	    dlElement = dlElement->next;
	}
    }
#if DEBUG_UNDO
    fprintf(stderr,"\n[restoreUndoInfo: tmpDlElementList(1):\n");
    dumpDlElementList(tmpDlElementList);
#endif

  /* Copy undo list to current list */
    if (!IsEmpty(displayInfo->dlElementList)) {
	removeDlDisplayListElementsExceptDisplay(displayInfo->dlElementList);
    }
    if (!IsEmpty(undoInfo->dlElementList)) {  
	dlElement = FirstDlElement(undoInfo->dlElementList);
	while (dlElement) {
	    DlElement *element = dlElement;
	    if (element->type != DL_Display) {
		DlElement *pE = element->run->create(element);
		if (pE->run->execute) {
		    (pE->run->execute)(displayInfo, pE);
		}
		appendDlElement(displayInfo->dlElementList,pE);
	    }
	    dlElement = dlElement->next;
	}
    }
#if DEBUG_UNDO
    fprintf(stderr,"\n[restoreUndoInfo: displayInfo->dlElementList(2):\n");
    dumpDlElementList(displayInfo->dlElementList);
#endif

  /* Copy temporary list to undo list */
    if (!IsEmpty(undoInfo->dlElementList)) {
	clearDlDisplayList(undoInfo->dlElementList);
    }
    if (!IsEmpty(tmpDlElementList)) {
	dlElement = FirstDlElement(tmpDlElementList);
	while (dlElement) {
	    DlElement *element = dlElement;
	    if (element->type != DL_Display) {
		DlElement *pE = element->run->create(element);
		appendDlElement(undoInfo->dlElementList,pE);
	    }
	    dlElement = dlElement->next;
	}
    }
#if DEBUG_UNDO
    fprintf(stderr,"\n[restoreUndoInfo: undoInfo->dlElementList(3):\n");
    dumpDlElementList(undoInfo->dlElementList);
#endif
	
#if DEBUG_UNDO
    fprintf(stderr,"\n[restoreUndoInfo: displayInfo->dlElementList(After):\n");
    dumpDlElementList(displayInfo->dlElementList);
    fprintf(stderr,"\n[restoreUndoInfo: undoInfo->dlElementList(After):\n");
    dumpDlElementList(undoInfo->dlElementList);
    fprintf(stderr,"\n[restoreUndoInfo: tmpDlElementList(After):\n");
    dumpDlElementList(tmpDlElementList);
#endif

  /* Clear the temporary element list to be neat */
    clearDlDisplayList(tmpDlElementList);

  /* Do direct copies */
    displayInfo->gridSpacing = undoInfo->gridSpacing;
    displayInfo->gridOn = undoInfo->gridOn;
    displayInfo->drawingAreaBackgroundColor = undoInfo->drawingAreaBackgroundColor;
    displayInfo->drawingAreaForegroundColor = undoInfo->drawingAreaForegroundColor;

  /* Insure that this is the currentDisplayInfo and refresh */
    currentDisplayInfo = displayInfo;     /* Shouldn't be necessary */
    refreshDisplay();
#endif    
}

/* Updates the display object values from the current positions of the display
 *   shell windows since that isn't done when the user moves them */
void updateAllDisplayPositions()
{
    DisplayInfo *displayInfo;
    DlElement *element;
    Position x, y;
    Arg args[2];
    int nargs;

    displayInfo = displayInfoListHead->next;

  /* Traverse the displayInfo list */
    while (displayInfo != NULL) {
      /* Get the current shell coordinates */
	nargs=0;
	XtSetArg(args[nargs],XmNx,&x); nargs++;
	XtSetArg(args[nargs],XmNy,&y); nargs++;
	XtGetValues(displayInfo->shell,args,nargs);
	
      /* Traverse the element list list to find the Dl_Display */
	element = FirstDlElement(displayInfo->dlElementList);
	while (element) {
	    if (element->type == DL_Display) {
		DlDisplay *p = element->structure.display;
	      /* Set the object values from the shell */
		p->object.x = x;
		p->object.y = y;
		break;     /* Don't need any more elements */
	    }
	}
	displayInfo = displayInfo->next;
    }
}

/* Set time base values
 * Note that the time base in MEDM is set as Jan. 1, 1990, 00:00:00, local time
 *   whereas the EPICS time base is Jan. 1, 1990, 00:00:00 GMT.  This has the
 *   effect of making EPICS values appear as local time rather than the non-
 *   intuitive GMT.  Note that UNIX time is relative to Jan. 1, 1970,
 *   00:00:00 GMT.
 */
void setTimeValues(void)
{
	struct tm ltime;

	ltime.tm_year = 70;  /* year */
	ltime.tm_mon = 0;    /* January */
	ltime.tm_mday = 1;   /* day */
	ltime.tm_hour = 0;   /* day */
	ltime.tm_min  = 0;   /* day */
	ltime.tm_sec  = 0;   /* day */
	ltime.tm_isdst = -1; /* mktime figures out TZ effect */
	time700101 = mktime(&ltime);

	ltime.tm_year = 90;  /* year */
	ltime.tm_mon = 0;    /* January */
	ltime.tm_mday = 1;   /* day */
	ltime.tm_hour = 0;   /* day */
	ltime.tm_min  = 0;   /* day */
	ltime.tm_sec  = 0;   /* day */
	ltime.tm_isdst = -1; /* mktime figures out TZ effect */
	time900101 = mktime(&ltime);
	timeOffset = time900101 - time700101;
}
