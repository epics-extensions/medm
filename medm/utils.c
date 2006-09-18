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

#define DEBUG_DISPLAY_LIST 0
#define DEBUG_EVENTS 0
#define DEBUG_FILE 0
#define DEBUG_STRING_LIST 0
#define DEBUG_TRAVERSAL 0
#define DEBUG_UNDO 0
#define DEBUG_PVINFO 0
#define DEBUG_PVLIMITS 0
#define DEBUG_COMMAND 0
#define DEBUG_REDRAW 0
#define DEBUG_VISIBILITY 0
#define DEBUG_GRID 0
#define DEBUG_RESID 0
#define DEBUG_RELATED_DISPLAY 0

#define UNDO

#include <string.h>
#include <time.h>
#include <ctype.h>
#include <X11/IntrinsicP.h>
#include <X11/keysym.h>
#include <Xm/MwmUtil.h>
#include <Xm/List.h>
#include <cvtFast.h>
#include <postfix.h>
#include "medm.h"

/* Include this after medm.h to avoid problems with Exceed 6 */
#ifdef WIN32
/* In MSVC timeval is in winsock.h, winsock2.h, ws2spi.h, nowhere else */
#include <X11/Xos.h>
#else
#include <sys/time.h>
#endif

#ifdef WIN32
#include <direct.h>     /* for getcwd (usually in sys/parm.h or unistd.h) */
#endif

#ifdef  __TED__
#include <Dt/Wsm.h>
#endif

/* #define GRAB_WINDOW window */
#define GRAB_WINDOW None

/* Function prototypes */

static DlElement *findSmallestTouchedCompositeElement(DlElement *pEC,
  Position x0, Position y0, Boolean dynamicOnly);
static DlElement *findHiddenRelatedDisplayInComposite(DlElement *pEC,
  Position x0, Position y0);
static void medmPrintfDlElementList(DlList *l, char *text);
static int doPasting(int *offsetX, int *offsetY);
static void toggleHighlightRectangles(DisplayInfo *displayInfo,
  int xOffset, int yOffset);
static void dumpPixmapCb(Widget w, XtPointer clientData, XtPointer callData);

/* Global variables */

Boolean modalGrab = FALSE;     /* KE: Not used ?? */
static DlList *tmpDlElementList = NULL;

/* Function to convert float to long for use with X resources */
long longFval(double f)
{
    XcVType val;

    val.fval=(float)f;
    return val.lval;
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

/*
 *  extract strings between colons from input to output
 *    this function works as an iterator...
 */
Boolean extractStringBetweenColons(char *input, char *output,
  int  startPos, int  *endPos)
{
    int i, j;

    i = startPos; j = 0;
    while(input[i] != '\0') {
	if(input[i] != MEDM_PATH_DELIMITER) {
	    output[j++] = input[i];
	} else break;
	i++;
    }
    output[j] = '\0';
    if(input[i] != '\0') {
	i++;
    }
    *endPos = i;
    if(j == 0)
      return(False);
    else
      return(True);
}

/* Check if the name is a pathname */
int isPath(const char *fileString)
{
    int pathTest;

  /* Look for the appropriate leading character */
#if defined(VMS)
    pathTest = (strchr(fileString, '[') != NULL);
#elif defined(WIN32)
  /* A drive specification will also indicate a full path name */
    pathTest = (fileString[0] == MEDM_DIR_DELIMITER_CHAR ||
      fileString[1] == ':');
#else
    pathTest = (fileString[0] == MEDM_DIR_DELIMITER_CHAR);
#endif

  /* Note: Do not assume names starting with . or .. are full path
     names.  Paths can be prepended to these as for the related
     display, so we don't want to eliminate them. */

    return(pathTest);
}

/* Convert name to a full path name.  Returns 1 if converted, 0 if
   failed. */
int convertNameToFullPath(const char *name, char *pathName, int nChars)
{
    int retVal = 1;

    if(isPath(name)) {
      /* Is a full path name */
	strncpy(pathName, name, nChars);
	pathName[nChars-1] = '\0';
    } else {
	char currentDirectoryName[PATH_MAX];

      /* Insert the path before the file name */
	currentDirectoryName[0] = '\0';
	getcwd(currentDirectoryName, PATH_MAX);

	if(strlen(currentDirectoryName) + strlen(name) <
	  (size_t)(nChars - 1)) {
	    strcpy(pathName, currentDirectoryName);
#ifndef VMS
	    strcat(pathName, MEDM_DIR_DELIMITER_STRING);
#endif
	    strcat(pathName, name);
	} else {
	  /* Hopefully won't get here */
	    strncpy(pathName, name, nChars);
	    pathName[nChars-1] = '\0';
	    retVal = 0;
	}
    }

    return retVal;
}

/* Function to convert / to \ to avoid inconsistencies in WIN32 */
void convertDirDelimiterToWIN32(char *pathName)
{
    char *ptr;

    ptr=strchr(pathName,'/');
    while(ptr) {
	*ptr='\\';
	ptr=strchr(ptr,'/');
    }
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

    i = nFonts/2;
    upper = nFonts-1;
    lower = 0;
    count = 0;

/* first select based on height of bounding box */
    while( (i>0) && (i<nFonts) && ((upper-lower)>2) && (count<nFonts/2)) {
	count++;
	if(fontTable[i]->ascent + fontTable[i]->descent > h) {
	    upper = i;
	    i = upper - (upper-lower)/2;
	} else if(fontTable[i]->ascent + fontTable[i]->descent < h) {
	    lower = i;
	    i = lower + (upper-lower)/2;
	}
    }
    if(i < 0) i = 0;
    if(i >= nFonts) i = nFonts - 1;

    *usedH = fontTable[i]->ascent + fontTable[i]->descent;
    *usedW = fontTable[i]->max_bounds.width;

    if(textWidthFlag) {
/* now select on width of bounding box */
	while( ((temp = XTextWidth(fontTable[i],text,strlen(text))) > w)
	  && i > 0 ) i--;

	*usedW = temp;

#if 0
	if( *usedH > h || *usedW > w)
	  if(errorOnI != i) {
	      errorOnI = i;
	      print(
		"\ndmGetBestFontWithInfo: need another font near pixel height = %d",
		h);
	  }
#endif
    }

    return (i);
}


/*
 * optionMenuSet:  routine to set option menu to specified button index
 *		(0 - (# buttons - 1))
 *   Sets the XmNmenuHistory, which causes the button to be set
 */
void optionMenuSet(Widget menu, int buttonId)
{
    WidgetList buttons;
    Cardinal numButtons;
    Widget subMenu;

  /* (MDA) - if option menus are ever created using non pushbutton or
   *	pushbutton widgets in them (e.g., separators) then this routine must
   *	loop over all children and make sure to only reference the push
   *	button derived children
   *
   *	Note: for invalid buttons, don't do anything (this can occur
   *	for example, when setting dynamic attributes when they don't
   *	really apply (and this is usually okay because they are not
   *	managed in invalid cases anyway))
   */
    XtVaGetValues(menu,XmNsubMenuId,&subMenu,NULL);
    if(subMenu != NULL) {
	XtVaGetValues(subMenu, XmNchildren,&buttons, XmNnumChildren,
	  &numButtons, NULL);
	if(buttonId < (int)numButtons && buttonId >= 0) {
	    XtVaSetValues(menu,XmNmenuHistory,buttons[buttonId],NULL);
	}
    } else {
	medmPrintf(1,"\noptionMenuSet: No subMenu found for option menu\n");
    }
}

void optionMenuRemoveLabel(Widget menu)
{
    WidgetList children;
    Cardinal numChildren;
    Cardinal n;

  /* Get the children */
    XtVaGetValues(menu,
      XmNchildren,&children,
      XmNnumChildren,&numChildren,
      NULL);

  /* Look for the label, which has name OptionLabel */
    for(n=0; n < numChildren; n++) {
	if(!strcmp(XtName(children[n]),"OptionLabel")) {
	    XtUnmanageChild(children[n]);
	    break;
	}
    }
}

XtErrorHandler trapExtraneousWarningsHandler(String message)
{
    if(message && *message) {
      /* "Attempt to remove non-existant passive grab" */
	if(!strcmp(message,"Attempt to remove non-existent passive grab"))
	  return(0);
      /* "The specified scale value is less than the minimum scale value." */
      /* "The specified scale value is greater than the maximum scale value." */
	if(!strcmp(message,"The specified scale value is"))
	  return(0);
    } else {
	medmPostMsg(1,"trapExtraneousWarningsHandler:\n%s\n", message);
    }

    return(0);
}

/* Look for a hidden related display */
DlElement *findHiddenRelatedDisplay(DisplayInfo *displayInfo,
  Position x0, Position y0)
{
    DlElement *pE, *pE1;
    DlObject *po;

  /* Loop over elements in the composite */
    pE = SecondDlElement(displayInfo->dlElementList);
    while(pE) {
      /* See if it is a widget and hence not a hidden related display */
	if(pE->widget) {
	    pE = pE->next;
	    continue;
	}

      /* See if the point falls inside the element */
	po = &(pE->structure.rectangle->object);
	if(((x0 < po->x) || (x0 > po->x + (int)po->width)) ||
	  ((y0 < po->y) || (y0 > po->y + (int)po->height))) {
	    pE = pE->next;
	    continue;
	}

      /* Check for hidden related display */
	if(pE->type == DL_RelatedDisplay &&
	  pE->structure.relatedDisplay->visual == RD_HIDDEN_BTN) {
#if DEBUG_RELATED_DISPLAY
	    print("findHiddenRelatedDisplay: Related Display\n"
	      "  x0=%d y0=%d x=%d w=%d x1=%d y=%d h=%d y1=%d\n",
		  x0,y0,po->x,po->width,po->x + (int)po->width,
		  po->y,po->height,po->y + (int)po->height);
#endif
	    return(pE);
	} else if(pE->type == DL_Composite) {
	    pE1 = findHiddenRelatedDisplayInComposite(pE, x0, y0);
	    if(pE1) {
#if DEBUG_RELATED_DISPLAY
		print("findHiddenRelatedDisplay: Composite\n"
		  "  x0=%d y0=%d x=%d x1=%d y=%d y1=%d\n",
		  x0,y0,po->x,po->x + (int)po->width,
		  po->y,po->y + (int)po->height);
#endif
		return pE1;
	    }
	}
	pE = pE->next;
    }

  /* Return */
     return NULL;
}

/* Look for a hidden related display in a composite */
static DlElement *findHiddenRelatedDisplayInComposite(DlElement *pEC,
  Position x0, Position y0)
{
    DlElement *pE, *pE1;
    DlObject *po;

  /* Loop over elements not including the display */
    pE = FirstDlElement(pEC->structure.composite->dlElementList);
    while(pE) {
      /* See if it is a widget and hence not a hidden related display */
	if(pE->widget) {
	    pE = pE->next;
	    continue;
	}

      /* See if the point falls inside the element */
	po = &(pE->structure.rectangle->object);
	if(((x0 < po->x) || (x0 > po->x + (int)po->width)) ||
	  ((y0 < po->y) || (y0 > po->y + (int)po->height))) {
	    pE = pE->next;
	    continue;
	}

      /* Check for hidden related display */
	if(pE->type == DL_RelatedDisplay &&
	  pE->structure.relatedDisplay->visual == RD_HIDDEN_BTN) {
	    return(pE);
	} else if(pE->type == DL_Composite) {
	    pE1 = findHiddenRelatedDisplayInComposite(pE, x0, y0);
	    if(pE1) return pE1;
	}
	pE = pE->next;
    }

  /* Return */
     return NULL;
}

/* Look for smallest object which contains the specified position.
 * With dynamicOnly finds only elements with update tasks. */
DlElement *findSmallestTouchedElement(DlList *pList, Position x0, Position y0,
  Boolean dynamicOnly)
{
    DlElement *pE, *pE1, *pSmallest, *pDisplay;
    DlObject *po, *po1;
    double area, minArea;

#if DEBUG_PVINFO
    print("findSmallestTouchedElement: dynamicOnly=%s x0=%3d y0=%3d\n",
      dynamicOnly?"True ":"False",x0,y0);
#endif

  /* Traverse the display list */
    pSmallest = pDisplay = NULL;
    minArea = (double)(INT_MAX)*(double)(INT_MAX);
    pE = FirstDlElement(pList);
    while(pE) {
      /* Don't look at hidden elements */
	if(pE->hidden) {
	    pE = pE->next;
	    continue;
	}

      /* Don't use the display but save it as a fallback */
	if(pE->type == DL_Display) {
	    if(!dynamicOnly) pDisplay = pE;
	    pE = pE->next;
	    continue;
	}

      /* See if the point falls inside the element */
	po = &(pE->structure.rectangle->object);
	if(((x0 < po->x) || (x0 > po->x + (int)po->width)) ||
	  ((y0 < po->y) || (y0 > po->y + (int)po->height))) {
	    pE = pE->next;
	    continue;
	}

      /* If in EXECUTE mode recurse into a composite element */
	pE1 = pE;
	po1 = po;
	if(globalDisplayListTraversalMode == DL_EXECUTE &&
	  pE->type == DL_Composite) {
	    DlElement *pE2;

	    pE2 = findSmallestTouchedCompositeElement(pE, x0, y0, dynamicOnly);
	    if(pE2) {
		pE1 = pE2;
		po1 = &(pE2->structure.rectangle->object);
	    }
	}

      /* See if it is dynamic */
	if(dynamicOnly) {
	    UpdateTask *pT = getUpdateTaskFromElement(pE1);
	    if(!pT || !pT->getRecord) {
		pE = pE->next;
		continue;
	    }
	}

      /* See if it is smallest element */
	area=(double)(po1->width)*(double)(po1->height);

      /* Keep the last one (the one on top) */
	if(area <= minArea) {
	    pSmallest = pE1;
	    minArea = area;
	}
#if DEBUG_PVINFO
	print("  minArea (so far): %g"
	  " Smallest element: %s\n", minArea,
	  pSmallest?elementType(pSmallest->type):"");
#endif
	pE = pE->next;
    }

  /* Use the display as the fallback if it was found */
    if(pSmallest == NULL) pSmallest = pDisplay;

  /* Return the element */
    return (pSmallest);
}

/* Find the smallest element at the given coordinates when in EXECUTE
 * mode */
DlElement *findSmallestTouchedExecuteElement(Widget w, DisplayInfo *displayInfo,
  Position *x, Position *y, Boolean dynamicOnly)
{
    Widget child;
    Position x0, y0;
    int nargs;
    Arg args[2];
    DlElement *pE, *pE1;

  /* Get the position relative to the drawing area. Follow parents
     upward to the display */
    child = NULL;
    while(w != displayInfo->drawingArea) {
	nargs=0;
	XtSetArg(args[nargs],XmNx,&x0); nargs++;
	XtSetArg(args[nargs],XmNy,&y0); nargs++;
	XtGetValues(w,args,nargs);
	*x += x0;
	*y += y0;

	child = w;
	w = XtParent(w);
	if(w == mainShell) return NULL;
    }

  /* Try to find element from child since execute sizes are different
     from edit sizes */
    pE = NULL;
    if(child) {
	pE1 = FirstDlElement(displayInfo->dlElementList);
	while(pE1) {
	    if(child == pE1->widget) {
		pE = pE1;
		break;
	    }
	    pE1 = pE1->next;
	}
    }

  /* Fall back to findSmallestTouchedElement, which uses EDIT sizes */
    if(!pE) {
	pE = findSmallestTouchedElement(displayInfo->dlElementList,
	  *x, *y, dynamicOnly);
    }

  /* Return */
     return pE;
}


/* Used in findSmallestTouchedElement for recursing into composites */
static DlElement *findSmallestTouchedCompositeElement(DlElement *pEC,
  Position x0, Position y0, Boolean dynamicOnly)
{
    DlElement *pE, *pE1, *pSmallest, *pDisplay;
    DlObject *po, *po1;
    double area, minArea;

#if DEBUG_PVINFO
    print("findSmallestTouchedCompositeElement: dynamicOnly=%s "
      "x0=%3d y0=%3d\n",
      dynamicOnly?"True ":"False", x0,y0);
#endif

    if(!pEC || pEC->type != DL_Composite) return NULL;

  /* Traverse the display list */
    pSmallest = pDisplay = NULL;
    minArea = (double)(INT_MAX)*(double)(INT_MAX);
    pE = FirstDlElement(pEC->structure.composite->dlElementList);
    while(pE) {
      /* Don't look at hidden elements */
	if(pE->hidden) {
	    pE = pE->next;
	    continue;
	}

      /* See if the point falls inside the element */
	po = &(pE->structure.rectangle->object);
	if(((x0 < po->x) || (x0 > po->x + (int)po->width)) ||
	  ((y0 < po->y) || (y0 > po->y + (int)po->height))) {
	    pE = pE->next;
	    continue;
	}

      /* If in EXECUTE mode recurse into a composite element */
	pE1 = pE;
	po1 = po;
	if(globalDisplayListTraversalMode == DL_EXECUTE &&
	  pE->type == DL_Composite) {
	    DlElement *pE2;

	    pE2 = findSmallestTouchedCompositeElement(pE, x0, y0,
	      dynamicOnly);
	    if(pE2) {
		pE1 = pE2;
		po1 = &(pE2->structure.rectangle->object);
	    }
	}

      /* See if it is dynamic */
	if(dynamicOnly) {
	    UpdateTask *pT = getUpdateTaskFromElement(pE1);
	    if(!pT || !pT->getRecord) {
		pE = pE->next;
		continue;
	    }
	}

      /* See if it is smallest element */
	area=(double)(po1->width)*(double)(po1->height);

      /* Keep the last one (the one on top) */
	if(area <= minArea) {
	    pSmallest = pE1;
	    minArea = area;
	}
#if DEBUG_PVINFO
	print("  minArea (so far): %g"
	  " Smallest element: %s\n", minArea,
	  pSmallest?elementType(pSmallest->type):NULL);
#endif
	pE = pE->next;
    }

  /* Return the element */
    return (pSmallest);
}

/*
 * Finds elements in list 1 and puts them into list 2 according to
 *   the mode mask specification AND whether the points are close or not
 */
void findSelectedEditElements(DlList *pList1, Position x0, Position y0,
  Position x1, Position y1, DlList *pList2, unsigned int mode)
{
  /* Number of pixels to consider to be the same as no motion */
#define RUBBERBAND_EPSILON 4

    if((x1 - x0) <= RUBBERBAND_EPSILON && (y1 - y0) <= RUBBERBAND_EPSILON) {
      /* No motion, treat as a point */
	if(mode&SmallestTouched) {
	  /* Find the smallest element that is touched by the point */
	    Position x = (x0 + x1)/2, y = (y0 + y1)/2;
	    DlElement *pE;

	    pE = findSmallestTouchedElement(pList1, x, y, False);
#if DEBUG_PVINFO
	    print("findSelectedEditElements x=%3d y=%3d pE=%x [%s]\n",
	      x,y,pE,pE?elementType(pE->type):"");
#endif
	    if(pE) {
		DlElement *pENew = createDlElement(DL_Element,(XtPointer)pE,NULL);
		if(pENew) {
		    appendDlElement(pList2, pENew);
		}
	    }
	}
	if(mode&AllTouched) {
	  /* Find all the elements that are touched by the point */
	    findAllMatchingElements(pList1,x0,y0,x1,y1,pList2,AllTouched);
	}
    } else {
      /* Treat as a rectangle */
	if(mode&AllEnclosed) {
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

    while(pE->prev) {
	DlObject *po = &(pE->structure.rectangle->object);

	if(mode&AllTouched) {
	  /* Find all the elements that are touched by the midpoint */
	    Position x = (x0 + x1)/2, y = (y0 + y1)/2;

	    criterion = po->x <= x && (po->x + (int)po->width) >= x &&
	      po->y <= y && (po->y + (int)po->height) >= y;
	} else if(mode&AllEnclosed) {
	  /* Find all the elements that are enclosed by the rectangle */
	    criterion = x0 <= po->x && x1 >= (po->x + (int)po->width) &&
	      y0 <= po->y && y1 >= (po->y + (int)po->height);
	} else {
	    return;
	}

      /* Do not include the display */
	if(pE->type != DL_Display && criterion) {
	    DlElement *pENew = createDlElement(DL_Element,(XtPointer)pE,NULL);
	    if(pENew) {
		insertDlElement(pList2, pENew);
	    } else {
		medmPostMsg(1,"findAllMatchingElements: Could not create element\n");
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
    float sX = 1.0, sY = 1.0;
    Dimension oldWidth, oldHeight;

  /* The first element is the display */
    elementPtr = FirstDlElement(displayInfo->dlElementList);
    oldWidth = (int)elementPtr->structure.display->object.width;
    oldHeight = (int)elementPtr->structure.display->object.height;

  /* Simply return False if no real change */
    if(oldWidth == newWidth && oldHeight == newHeight) return(False);

  /* Resize the display, then do selected elements */
    elementPtr->structure.display->object.width = newWidth;
    elementPtr->structure.display->object.height = newHeight;

  /* Proceed with scaling */
    if(oldWidth)
      sX = (float)newWidth/(float)oldWidth;
    if(oldHeight)
      sY = (float)newHeight/(float)oldHeight;

    resizeDlElementList(displayInfo->dlElementList,0,0,sX,sY);
    return(True);
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
    Dimension oldWidth, oldHeight;

    elementPtr = FirstDlElement(displayInfo->dlElementList);
    oldWidth = (int)elementPtr->structure.display->object.width;
    oldHeight = (int)elementPtr->structure.display->object.height;

  /* Simply return (value FALSE) if no real change */
    if(oldWidth == newWidth && oldHeight == newHeight) return (FALSE);

  /* Resize the display, then do selected elements */
    elementPtr->structure.display->object.width = newWidth;
    elementPtr->structure.display->object.height = newHeight;

  /* Proceed with scaling...*/
    sX = (float) ((float)newWidth/(float)oldWidth);
    sY = (float) ((float)newHeight/(float)oldHeight);

    resizeDlElementReferenceList(displayInfo->selectedDlElementList,0,0,sX,sY);

    return (TRUE);
}

void resizeDlElementReferenceList(DlList *dlElementList, int x, int y,
  float scaleX, float scaleY)
{
    DlElement *dlElement;
    if(dlElementList->count < 1) return;
  /* Loop over selected elements not including the display */
    dlElement = FirstDlElement(dlElementList);
    while(dlElement) {
	DlElement *pE = dlElement->structure.element;
 	if(pE->type != DL_Display) {
	    int w = (int)pE->structure.rectangle->object.width;
	    int h = (int)pE->structure.rectangle->object.height;
	    int xOffset = (int) (scaleX * (float) w + 0.5) - w;
	    int yOffset = (int) (scaleY * (float) h + 0.5) - h;

	    if(pE->run->scale) {
		pE->run->scale(pE,xOffset,yOffset);
	    }

	    w = pE->structure.rectangle->object.x - x;
	    h = pE->structure.rectangle->object.y - y;
	    xOffset = (int) (scaleX * (float) w + 0.5) - w;
	    yOffset = (int) (scaleY * (float) h + 0.5) - h;
	    if(pE->run->move) {
		pE->run->move(pE,xOffset,yOffset);
	    }
	}
	dlElement = dlElement->next;
    }
}

void resizeDlElementList(DlList *dlElementList, int x, int y,
  float scaleX, float scaleY)
{
    DlElement *pE;
    if(dlElementList->count < 1) return;
  /* Loop over elements */
    pE = FirstDlElement(dlElementList);
    while(pE) {
	if(pE->type != DL_Display) {
	    int w = (int)pE->structure.rectangle->object.width;
	    int h = (int)pE->structure.rectangle->object.height;
	    int xOffset = (int) (scaleX * (float) w + 0.5) - w;
	    int yOffset = (int) (scaleY * (float) h + 0.5) - h;

	    if(pE->run->scale) {
		pE->run->scale(pE,xOffset,yOffset);
	    }
	    w = pE->structure.rectangle->object.x - x;
	    h = pE->structure.rectangle->object.y - y;
	    xOffset = (int) (scaleX * (float) w + 0.5) - w;
	    yOffset = (int) (scaleY * (float) h + 0.5) - h;
	    if(pE->run->move) {
		pE->run->move(pE,xOffset,yOffset);
	    }
	}
	pE = pE->next;
    }
}

/*** Server grabbing routines ****/

GC xorGC;

void initializeRubberbanding()
{
/* Create the xorGC and rubberbandCursor for drawing while dragging */
    xorGC = XCreateGC(display,rootWindow,0,NULL);
  /* Eliminate events that we do not handle anyway */
    XSetGraphicsExposures(display,xorGC,False);
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

int doRubberbanding(Window window, Position *initialX, Position *initialY,
  Position *finalX,  Position *finalY, int doSnap)
{
    XEvent event;
    Boolean returnVal = True;
    DisplayInfo *cdi =currentDisplayInfo;
    int x, y, x0, y0, x1, y1;
    int snap, gridSpacing;
    unsigned int w, h;

  /* Get current display info */
    cdi = currentDisplayInfo;
    if(!cdi) return 0;
    snap = cdi->grid->snapToGrid;
    gridSpacing = cdi->grid->gridSpacing;

  /* Setup */
    *finalX = *initialX;
    *finalY = *initialY;
    x0 = *initialX;
    y0 = *initialY;
    x1 = x0;
    y1 = y0;
    w = (Dimension)0;
    h = (Dimension)0;
    x = MIN(x0,x1);
    y = MIN(y0,y1);
    if(doSnap && snap) {
	x = ((x + gridSpacing/2)/gridSpacing)*gridSpacing;
	y = ((y + gridSpacing/2)/gridSpacing)*gridSpacing;
	w = ((w + gridSpacing/2)/gridSpacing)*gridSpacing;
	h = ((h + gridSpacing/2)/gridSpacing)*gridSpacing;
    }

  /* Have all interesting events go to window
   *  Note: ButtonPress and ButtonRelease do not need to be specified
   * KE: If include KeyPressMask get:
   *   BadValue (integer parameter out of range for operation)
   *   but it seems to work OK without it ??? */
#if DEBUG_EVENTS
    print("In doRubberbanding before XGrabPointer\n");
#endif
    XGrabPointer(display,window,FALSE,
      (unsigned int)(ButtonMotionMask|ButtonReleaseMask),
      GrabModeAsync,GrabModeAsync,GRAB_WINDOW,rubberbandCursor,CurrentTime);

/* Grab the server to ensure that XORing will be okay */
    XGrabServer(display);
    XDrawRectangle(display, window, xorGC, x, y, w, h);

  /* Loop until the button is released */
    while(TRUE) {
	XtAppNextEvent(appContext,&event);
	switch (event.type) {
	case KeyPress: {
	    XKeyEvent *kevent = (XKeyEvent *)&event;
	    Modifiers modifiers;
	    KeySym keysym;

	    XtTranslateKeycode(display, kevent->keycode, (Modifiers)NULL,
	      &modifiers, &keysym);
#if 0
	    print("doRubberbanding: Type: %d Keysym: %x (osfXK_Cancel=%x) "
	      "Shift: %d Ctrl: %d\n"
	      "  [KeyPress=%d, KeyRelease=%d ButtonPress=%d, ButtonRelease=%d]\n",
	      kevent->type,keysym,osfXK_Cancel,
	      kevent->state&ShiftMask,kevent->state&ControlMask,
	      KeyPress,KeyRelease,ButtonPress,ButtonRelease);     /* In X.h */
#endif
	    if(keysym != osfXK_Cancel) break;
	    returnVal = False;
	  /* Fall through */
	}

	case ButtonRelease:
#if DEBUG_EVENTS > 1
	    print("  ButtonRelease: x=%d y=%d\n",event.xbutton.x,event.xbutton.y);
	    fflush(stdout);
#endif
	  /* Undraw old one */
	    XDrawRectangle(display, window, xorGC, x, y, w, h);
	    XUngrabServer(display);
	    XUngrabPointer(display,CurrentTime);
	    XFlush(display);	  /* For debugger */
	  /* Update current coordinates */
	    x1 = event.xbutton.x;
	    y1 = event.xbutton.y;
	    w =  (MAX(x0,x1) - MIN(x0,x1));
	    h =  (MAX(y0,y1) - MIN(y0,y1));
	    x = MIN(x0,x1);
	    y = MIN(y0,y1);
	    if(doSnap && snap) {
		x = ((x + gridSpacing/2)/gridSpacing)*gridSpacing;
		y = ((y + gridSpacing/2)/gridSpacing)*gridSpacing;
		w = ((w + gridSpacing/2)/gridSpacing)*gridSpacing;
		h = ((h + gridSpacing/2)/gridSpacing)*gridSpacing;
		if((int)w < gridSpacing) w = gridSpacing;
		if((int)h < gridSpacing) h = gridSpacing;
	    }
	    *initialX =  x;
	    *initialY =  y;
	    *finalX   =  x + w;
	    *finalY   =  y + h;
	    return (returnVal);		/* return from while(TRUE) */

	case MotionNotify:
#if DEBUG_EVENTS > 1
	    print("  MotionNotify: x=%d y=%d\n",event.xbutton.x,event.xbutton.y);
	    fflush(stdout);
#endif
	  /* Undraw old one */
	    XDrawRectangle(display, window, xorGC, x, y, w, h);
	  /* Update current coordinates */
	    x1 = event.xbutton.x;
	    y1 = event.xbutton.y;
	    w =  (MAX(x0,x1) - MIN(x0,x1));
	    h =  (MAX(y0,y1) - MIN(y0,y1));
	    x = MIN(x0,x1);
	    y = MIN(y0,y1);
	    if(doSnap && snap) {
		x = ((x + gridSpacing/2)/gridSpacing)*gridSpacing;
		y = ((y + gridSpacing/2)/gridSpacing)*gridSpacing;
		w = ((w + gridSpacing/2)/gridSpacing)*gridSpacing;
		h = ((h + gridSpacing/2)/gridSpacing)*gridSpacing;
		if((int)w < gridSpacing) w = gridSpacing;
		if((int)h < gridSpacing) h = gridSpacing;
	    }
	  /* Draw new one */
	    XDrawRectangle(display, window, xorGC, x, y, w, h);
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
    XEvent event;
    DisplayInfo *cdi;
    DlElement *dlElement;
    Boolean returnVal = True;
    int x, y, xOffset, yOffset;
    int snap, gridSpacing;
    int minX, maxX, minY, maxY, groupWidth, groupHeight,
      groupDeltaX0, groupDeltaY0, groupDeltaX1, groupDeltaY1;
    int nelements;

  /* If no current display, simply return */
    cdi = currentDisplayInfo;
    if(!cdi) return(True);

  /* If no selected elements, return */
    if(IsEmpty(cdi->selectedDlElementList)) return False;
#if DEBUG_EVENTS > 1
    print("\n[doDragging] selectedDlElement list :\n");
    dumpDlElementList(cdi->selectedDlElementList);
#endif

    snap = cdi->grid->snapToGrid;
    gridSpacing = cdi->grid->gridSpacing;

    xOffset = 0;
    yOffset = 0;

#if DEBUG_EVENTS
    print("In doDragging before XGrabPointer\n");
#endif

  /* Have all interesting events go to window
   *  Note: ButtonPress and ButtonRelease do not need to be specified
   * KE: If include KeyPressMask get:
   *   BadValue (integer parameter out of range for operation)
   *   but it seems to work OK without it ??? */
    XGrabPointer(display, window, False,
      (unsigned int)(ButtonMotionMask|ButtonReleaseMask),
      GrabModeAsync,GrabModeAsync,GRAB_WINDOW,dragCursor,CurrentTime);

  /* Grab the server to ensure that XORing will be okay */
    XGrabServer(display);

  /* Draw first set of rectangles and get group extents */
  /* As usual, type in union unimportant as long as object is 1st */
    minX = INT_MAX; minY = INT_MAX;
    maxX = INT_MIN; maxY = INT_MIN;
    nelements=0;
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while(dlElement) {
	DlElement *pE = dlElement->structure.element;
	if(pE->type != DL_Display) {
	    DlObject *po = &pE->structure.rectangle->object;
	    nelements++;
	    XDrawRectangle(display,window, xorGC,
	      po->x + xOffset, po->y + yOffset, po->width , po->height);
	  /* Calculate extents of the group */
	    minX = MIN(minX, po->x);
	    maxX = MAX(maxX, po->x + (int)po->width);
	    minY = MIN(minY, po->y);
	    maxY = MAX(maxY, po->y + (int)po->height);
#if DEBUG_EVENTS > 1
	    print(" minX=%d maxX=%d minY=%d maxY=%d\n\n",
	      minX,maxX,minY,maxY);
	    print(" po->x=%d po->width=%d po->y=%d po->height =%d\n",
	      po->x,po->width,po->y,po->height);
#endif
	}
	dlElement = dlElement->next;
    }
  /* Check if there was anything selected besides the display */
    if(!nelements) {
	XUngrabServer(display);
	XUngrabPointer(display,CurrentTime);
	XBell(display,50);
	XFlush(display);
	return False;
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
#if DEBUG_EVENTS > 1
    print(" groupWidth=%d groupHeight=%d groupDeltaX0=%d\n"
      " groupDeltaY0=%d groupDeltaX1=%d groupDeltaY1=%d\n\n",
      groupWidth,groupHeight,groupDeltaX0,
      groupDeltaY0,groupDeltaX1,groupDeltaY1);
#endif
#if DEBUG_EVENTS > 2
    XUngrabServer(display);
    XUngrabPointer(display,CurrentTime);
    XFlush(display);
    return(returnVal);
#endif

/* Loop until the button is released */
    while(TRUE) {
	XtAppNextEvent(appContext, &event);
	switch (event.type) {
	case KeyPress: {
	    XKeyEvent *kevent = (XKeyEvent *)&event;
	    Modifiers modifiers;
	    KeySym keysym;

	    XtTranslateKeycode(display, kevent->keycode, (Modifiers)NULL,
	      &modifiers, &keysym);
#if 0
	    print("doDragging: Type: %d Keysym: %x (osfXK_Cancel=%x) "
	      "Shift: %d Ctrl: %d\n"
	      "  [KeyPress=%d, KeyRelease=%d ButtonPress=%d, ButtonRelease=%d]\n",
	      kevent->type,keysym,osfXK_Cancel,
	      kevent->state&ShiftMask,kevent->state&ControlMask,
	      KeyPress,KeyRelease,ButtonPress,ButtonRelease);     /* In X.h */
#endif
	    if(keysym != osfXK_Cancel) break;
	    returnVal = False;
	  /* Fall through */
	}
	case ButtonRelease:
#if DEBUG_EVENTS > 1
	    print("  ButtonRelease: x=%d y=%d\n",event.xbutton.x,event.xbutton.y);
	    fflush(stdout);
#endif
	  /* Undraw old ones */
	    dlElement = FirstDlElement(cdi->selectedDlElementList);
	    while(dlElement) {
		DlElement *pE = dlElement->structure.element;
		if(pE->type != DL_Display) {
		    DlObject *po = &pE->structure.rectangle->object;
		    XDrawRectangle(display, window, xorGC,
		      po->x + xOffset, po->y + yOffset, po->width , po->height);
		}
		dlElement = dlElement->next;
	    }
	    XUngrabServer(display);
	    XUngrabPointer(display,CurrentTime);
	    XFlush(display);
	    *finalX = initialX + xOffset;
	    *finalY = initialY + yOffset;
	    return (returnVal);	/* return from while(TRUE) */
	case MotionNotify:
#if DEBUG_EVENTS > 1
	    print("  MotionNotify: x=%d y=%d\n",event.xbutton.x,event.xbutton.y);
	    fflush(stdout);
#endif
	  /* Undraw old ones */
	    dlElement = FirstDlElement(cdi->selectedDlElementList);
	    while(dlElement) {
		DlElement *pE = dlElement->structure.element;
		if(pE->type != DL_Display) {
		    DlObject *po = &pE->structure.rectangle->object;
		    XDrawRectangle(display, window, xorGC,
		      po->x + xOffset, po->y + yOffset, po->width , po->height);
		}
		dlElement = dlElement->next;
	    }
	  /* Branch depening on snap to grid */
	    if(snap) {
	      /* Insure xMinNew and yMinNew are on the grid */
		int xMinNew, yMinNew;

		x = event.xmotion.x;
		y = event.xmotion.y;
		xMinNew = ((x - groupDeltaX0 + gridSpacing/2)/gridSpacing)*
		  gridSpacing;
		x = xMinNew + groupDeltaX0;
		yMinNew = ((y - groupDeltaY0 + gridSpacing/2)/gridSpacing)*
		  gridSpacing;
		y = yMinNew + groupDeltaY0;
	      /* Insure nothing goes outside the display */
		while(x < groupDeltaX0) x += gridSpacing;
		while(x > (int)(daWidth - groupDeltaX1)) x -= gridSpacing;
		while(y < groupDeltaY0) y += gridSpacing;
		while(y > (int)(daHeight - groupDeltaY1)) y -= gridSpacing;
	    } else {
	      /* Insure nothing goes outside the display */
		if(event.xmotion.x < groupDeltaX0)
		  x = groupDeltaX0;
		else if(event.xmotion.x > (int)(daWidth-groupDeltaX1))
		  x =  daWidth - groupDeltaX1;
		else
		  x =  event.xmotion.x;
		if(event.xmotion.y < groupDeltaY0)
		  y = groupDeltaY0;
		else if(event.xmotion.y > (int)(daHeight-groupDeltaY1))
		  y =  daHeight - groupDeltaY1;
		else
		  y =  event.xmotion.y;
	    }
	  /* Update current coordinates */
	    xOffset = x - initialX;
	    yOffset  = y - initialY;
	  /* Draw new ones */
	    dlElement = FirstDlElement(cdi->selectedDlElementList);
	    while(dlElement) {
		DlElement *pE = dlElement->structure.element;
		if(pE->type != DL_Display) {
		    DlObject *po = &pE->structure.rectangle->object;
		    XDrawRectangle(display,window, xorGC,
		      po->x + xOffset, po->y + yOffset, po->width , po->height);
		}
		dlElement = dlElement->next;
	    }
	    break;
	default:
#if DEBUG_EVENTS > 1
	    print("  Default: %s\n",getEventName(event.type));
	    fflush(stdout);
#endif
	    XtDispatchEvent(&event);
	}
    }
}

/*
 * do PASTING (with drag effect) of all elements in global
 *		 clipboardElementsArray
 */
static int doPasting(int *offsetX, int *offsetY)
{
    XEvent event;
    Window window, root, child;
    Dimension daWidth, daHeight;
    DisplayInfo *cdi;
    DlElement *dlElement = NULL;
    Boolean returnVal = True;
    int daLeft, daRight, daTop, daBottom;
    int rootX, rootY, winX, winY;
    int dx, dy, x0, y0, x1, y1;
    int snap, gridSpacing;
    unsigned int mask;
    int status;

  /* If no clipboard elements, simply return */
    if(IsEmpty(clipboard)) return 0;

  /* Get current display info */
    cdi = currentDisplayInfo;
    if(!cdi) return 0;
    snap = cdi->grid->snapToGrid;
    gridSpacing = cdi->grid->gridSpacing;

  /* Get position of upper left element in display */
    x0 = INT_MAX;
    y0 = INT_MAX;
    x1 = INT_MIN;
    y1 = INT_MIN;

  /* Normalize such that cursor is in middle of pasted objects */
    dlElement = FirstDlElement(clipboard);
    while(dlElement) {
	if(dlElement->type != DL_Display) {
	    DlObject *po = &(dlElement->structure.rectangle->object);
	    x0 = MIN(x0, po->x);
	    y0 = MIN(y0, po->y);
	    x1 = MAX(x1, po->x + (int)po->width);
	    y1 = MAX(y1, po->y + (int)po->height);
	}
	dlElement = dlElement->next;
    }
    dx = (x0 + x1)/2;
    dy = (y0 + y1)/2;

  /* Update offsets to be added when paste is done */
    *offsetX = -dx;
    *offsetY = -dy;

  /* Get drawingArea dimensions */
    XtVaGetValues(cdi->drawingArea,
      XmNwidth, &daWidth,
      XmNheight, &daHeight,
      NULL);
    daLeft = dx - x0;
    daRight = dx + (int)daWidth - x1;
    daTop = dy + - y0;
    daBottom = dy + (int)daHeight - y1;

  /* Warp the pointer to the center of the drawingArea */
    window = XtWindow(cdi->drawingArea);
    winX = daWidth/2;
    winY = daHeight/2;
    XWarpPointer(display, None, window, 0, 0, 0, 0, winX, winY);

  /* rootX, rootY is the pointer position relative to the root */
  /* winX, winY is the pointer position relative to the drawingArea */
    if(!XQueryPointer(display, window, &root, &child, &rootX, &rootY,
      &winX, &winY, &mask)) {
	XtAppWarning(appContext,"doPasting: Query pointer error");
    }

  /* Have all interesting events go to window
   *  Note: PointerMotionMask means all pointer motion (which we need)
   *        ButtonMotionMask means pointer motion with button pressed
   *  Note: ButtonPress and ButtonRelease do not need to be specified
   * KE: If include KeyPressMask get:
   *   BadValue (integer parameter out of range for operation)
   *   but it seems to work OK without it ??? */

#if 0
    print("\ndoPasting: Before XGrabPointer: window=%x "
      "XtWindow(cdi->drawingArea)=%x\n"
      "  PointerMotionMask=%d ButtonReleaseMask=%d KeyPressMask=%d\n",
      window, XtWindow(cdi->drawingArea),
      PointerMotionMask,ButtonReleaseMask,KeyPressMask);
#endif
    status = XGrabPointer(display, window, False,
      (unsigned int)(PointerMotionMask|ButtonReleaseMask),
      GrabModeAsync, GrabModeAsync, GRAB_WINDOW, dragCursor, CurrentTime);
#if 0
    print("\ndoPasting: Status=%d (GrabSuccess=%d GrabNotViewable=%d "
      "AlreadyGrabbed=%d GrabFrozen=%d GrabInvalidTime=%d\n",
      status,GrabSuccess,GrabNotViewable,AlreadyGrabbed,GrabFrozen,GrabInvalidTime);
#endif

  /* Grab the server to ensure that XORing will be okay */
  /* Take this out for debugging */
#if 1
    XGrabServer(display);
#endif

  /* Draw first rectangle */
  /* As usual, type in union unimportant as long as object is 1st thing */
    dlElement = FirstDlElement(clipboard);
    while(dlElement) {
	if(dlElement->type != DL_Display) {
	    DlObject *po = &(dlElement->structure.rectangle->object);
	    XDrawRectangle(display, window, xorGC,
	      winX + po->x - dx, winY + po->y - dy,
	      po->width, po->height);
	}
	dlElement = dlElement->next;
    }

  /* Now loop until the button is released */
    while(TRUE) {
	XtAppNextEvent(appContext, &event);
	switch (event.type) {
	case KeyPress: {
	    XKeyEvent *kevent = (XKeyEvent *)&event;
	    Modifiers modifiers;
	    KeySym keysym;
	    int deltaX = 0, deltaY = 0, mult = 1;


	    XtTranslateKeycode(display, kevent->keycode, (Modifiers)NULL,
	      &modifiers, &keysym);
#if 0
	    print("doPasting: Type: %d Keysym: %x (osfXK_Cancel=%x) "
	      "Shift: %d Ctrl: %d\n"
	      "  [KeyPress=%d, KeyRelease=%d ButtonPress=%d, ButtonRelease=%d]\n",
	      kevent->type,keysym,osfXK_Cancel,
	      kevent->state&ShiftMask,kevent->state&ControlMask,
	      KeyPress,KeyRelease,ButtonPress,ButtonRelease);     /* In X.h */
#endif
	    if(kevent->state&ControlMask ) {
		mult = 5;
	    } else if(kevent->state&ShiftMask) {
		mult = 25;
	    }
	    switch (keysym) {
	    case osfXK_Left:
		deltaX = -mult;
		break;
	    case osfXK_Right:
		deltaX = mult;
		break;
	    case osfXK_Up:
		deltaY = -mult;
		break;
	    case osfXK_Down:
		deltaY = mult;
		break;
	    default:
		break;
	    }
	  /* Move if there was any motion indicated */
	    if(deltaX != 0 || deltaY != 0) {
	      /* Undraw old ones */
		dlElement = FirstDlElement(clipboard);
		while(dlElement) {
		    if(dlElement->type != DL_Display) {
			DlObject *po = &(dlElement->structure.rectangle->object);
			XDrawRectangle(display, window, xorGC,
			  winX + po->x - dx, winY + po->y - dy,
			  po->width, po->height);
		    }
		    dlElement = dlElement->next;
		}

	      /* Set coordinates */
		if(snap) {
		    winX += gridSpacing * deltaX;
		    winY += gridSpacing * deltaY;
		} else {
		    winX += deltaX;
		    winY += deltaY;
		}

	      /* Branch depending on snap to grid */
		if(snap) {
		  /* Insure x0 and y0 are on the grid */
		    int x0New, y0New;

		    x0New = ((winX -dx + x0 + gridSpacing/2)/gridSpacing)*
		      gridSpacing;
		    winX = x0New - x0 + dx;
		    y0New = ((winY -dy + y0 + gridSpacing/2)/gridSpacing)*
		      gridSpacing;
		    winY = y0New - y0 + dy;
		  /* Insure nothing goes outside the display */
		    while(winX < daLeft) winX += gridSpacing;
		    while(winX > daRight) winX -= gridSpacing;
		    while(winY < daTop) winY += gridSpacing;
		    while(winY > daBottom) winY -= gridSpacing;
#if 0
		    print("x=%d winX=%d daLeft=%d daRight=%d x0=%d x1=%d x0New=%d y0New=%d dx=%d daWidth=%d\n",
		      event.xbutton.x,winX,daLeft,daRight,x0,x1,x0New,y0New,dx,(int)daWidth);
#endif
		} else {
		  /* Insure nothing goes outside the display */
		    if(winX < daLeft) winX = daLeft;
		    if(winX > daRight) winX = daRight;
		    if(winY < daTop) winY = daTop;
		    if(winY > daBottom) winY = daBottom;
		}

	      /* Draw new ones */
		dlElement = FirstDlElement(clipboard);
		while(dlElement) {
		    if(dlElement->type != DL_Display) {
			DlObject *po = &(dlElement->structure.rectangle->object);
#if 0
			print(" type=%s x=%d y=%d width=%d height=%d\n",
			  elementType(dlElement->type),
			  winX + po->x - dx,
			  winY + po->y - dy,
			  (int)po->width,
			  (int)po->height);
#endif
			XDrawRectangle(display, window, xorGC,
			  winX + po->x - dx, winY + po->y - dy,
			  po->width, po->height);
		    }
		    dlElement = dlElement->next;
		}
	      /* Move the pointer to the same place */
		XWarpPointer(display, None, window, 0, 0, 0, 0, winX, winY);
		break;
	    } else if(keysym == osfXK_Cancel) {
	      /* Cancel was pressed.  Set returnVal and fall through
                 to Button Release to quit */
		returnVal = False;
	    } else if(keysym == XK_Return || keysym == osfXK_Activate) {
	      /* Enter was pressed.  Fall through to Button Release to
                 quit */
	    } else {
	      /* Do nothing */
		break;
	    }
	}

	case ButtonRelease:
	  /* Undraw old ones */
	    dlElement = FirstDlElement(clipboard);
	    while(dlElement) {
		if(dlElement->type != DL_Display) {
		    DlObject *po = &(dlElement->structure.rectangle->object);
		    XDrawRectangle(display, window, xorGC,
		      winX + po->x - dx, winY + po->y - dy,
		      po->width, po->height);
		}
		dlElement = dlElement->next;
	    }
	    XUngrabServer(display);
	    XUngrabPointer(display,CurrentTime);
	    XFlush(display);
	    *offsetX += winX;
	    *offsetY += winY;
	    return (returnVal);

	case MotionNotify:
	  /* Undraw old ones */
	    dlElement = FirstDlElement(clipboard);
	    while(dlElement) {
		if(dlElement->type != DL_Display) {
		    DlObject *po = &(dlElement->structure.rectangle->object);
		    XDrawRectangle(display, window, xorGC,
		      winX + po->x - dx, winY + po->y - dy,
		      po->width, po->height);
		}
		dlElement = dlElement->next;
	    }

	  /* Set mouse coordinates */
	    winX = event.xmotion.x;
	    winY = event.xmotion.y;
#if 0
	    print("MotionNotify: winX=%d winY=%d\n",winX,winY);
#endif
	  /* Branch depending on snap to grid */
	    if(snap) {
	      /* Insure x0 and y0 are on the grid */
		int x0New, y0New;

		x0New = ((winX -dx + x0 + gridSpacing/2)/gridSpacing)*
		  gridSpacing;
		winX = x0New - x0 + dx;
		y0New = ((winY -dy + y0 + gridSpacing/2)/gridSpacing)*
		  gridSpacing;
		winY = y0New - y0 + dy;
	      /* Insure nothing goes outside the display */
		while(winX < daLeft) winX += gridSpacing;
		while(winX > daRight) winX -= gridSpacing;
		while(winY < daTop) winY += gridSpacing;
		while(winY > daBottom) winY -= gridSpacing;
#if 0
		print("x=%d winX=%d daLeft=%d daRight=%d x0=%d x1=%d x0New=%d y0New=%d dx=%d daWidth=%d\n",
		  event.xbutton.x,winX,daLeft,daRight,x0,x1,x0New,y0New,dx,(int)daWidth);
#endif
	    } else {
	      /* Insure nothing goes outside the display */
		if(winX < daLeft) winX = daLeft;
		if(winX > daRight) winX = daRight;
		if(winY < daTop) winY = daTop;
		if(winY > daBottom) winY = daBottom;
	    }

	  /* Draw new ones */
	    dlElement = FirstDlElement(clipboard);
	    while(dlElement) {
		if(dlElement->type != DL_Display) {
		    DlObject *po = &(dlElement->structure.rectangle->object);
#if 0
		    print(" type=%s x=%d y=%d width=%d height=%d\n",
		      elementType(dlElement->type),
		      winX + po->x - dx,
		      winY + po->y - dy,
		      (int)po->width,
		      (int)po->height);
#endif
		    XDrawRectangle(display, window, xorGC,
		      winX + po->x - dx, winY + po->y - dy,
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
 * do (multiple) resizing of all elements in global selectedElementsArray
 *	RETURNS: boolean indicating whether resize ended in the window
 *	(and hence was valid)
 * KE: This is not correct; It ALWAYS returns true
 */
Boolean doResizing(Window window, Position initialX, Position initialY,
  Position *finalX, Position *finalY)
{
    XEvent event;
    Boolean returnVal = True;
    DisplayInfo *cdi;
    int xOffset, yOffset;

    if(!currentDisplayInfo) return False;
    cdi = currentDisplayInfo;

    xOffset = 0;
    yOffset = 0;

#if DEBUG_EVENTS
    print("In doResizing before XGrabPointer\n");
#endif
   /* Have all interesting events go to window
   *  Note: ButtonPress and ButtonRelease do not need to be specified
   * KE: If include KeyPressMask get:
   *   BadValue (integer parameter out of range for operation)
   *   but it seems to work OK without it ??? */
   XGrabPointer(display,window,FALSE,
      (unsigned int)(ButtonMotionMask|ButtonReleaseMask),
      GrabModeAsync,GrabModeAsync,GRAB_WINDOW,resizeCursor,CurrentTime);

 /* Grab the server to ensure that XORing will be okay */
    XGrabServer(display);

  /* XOR the outline */
    toggleHighlightRectangles(currentDisplayInfo,xOffset,yOffset);

   /* Loop until the button is released */
    while(TRUE) {
	XtAppNextEvent(appContext,&event);
	switch (event.type) {
	case KeyPress: {
	    XKeyEvent *kevent = (XKeyEvent *)&event;
	    Modifiers modifiers;
	    KeySym keysym;

	    XtTranslateKeycode(display, kevent->keycode, (Modifiers)NULL,
	      &modifiers, &keysym);
#if 0
	    print("doResizing: Type: %d Keysym: %x (osfXK_Cancel=%x) "
	      "Shift: %d Ctrl: %d\n"
	      "  [KeyPress=%d, KeyRelease=%d ButtonPress=%d, ButtonRelease=%d]\n",
	      kevent->type,keysym,osfXK_Cancel,
	      kevent->state&ShiftMask,kevent->state&ControlMask,
	      KeyPress,KeyRelease,ButtonPress,ButtonRelease);     /* In X.h */
#endif
	    if(keysym != osfXK_Cancel) break;
	    returnVal = False;
	  /* Fall through */
	}
	case ButtonRelease:
	  /* Undraw old ones (XOR again) */
#if DEBUG_EVENTS > 1
	    print("ButtonRelease: x=%d y=%d\n",event.xbutton.x,event.xbutton.y);
	    fflush(stdout);
#endif
	    toggleHighlightRectangles(currentDisplayInfo,xOffset,yOffset);
	    XUngrabServer(display);
	    XUngrabPointer(display,CurrentTime);
	    XFlush(display);
	    *finalX =  initialX + xOffset;
	    *finalY =  initialY + yOffset;
	    return (returnVal);	/* Return from while(TRUE) */
	case MotionNotify:
	  /* Undraw old ones (XOR again to restore original) */
	    toggleHighlightRectangles(currentDisplayInfo,xOffset,yOffset);
	  /* Update current coordinates */
	    xOffset = event.xbutton.x - initialX;
	    yOffset = event.xbutton.y - initialY;
	  /* Draw new ones (XOR the outline) */
#if DEBUG_EVENTS > 1
	    print("MotionNotify: x=%d y=%d\n",event.xbutton.x,event.xbutton.y);
	    fflush(stdout);
#endif
	    toggleHighlightRectangles(currentDisplayInfo,xOffset,yOffset);
	    break;
	default:
	    XtDispatchEvent(&event);
	}
    }
}

/*** Editing routines ***/

/* Function to see if specified element is already in the global *
 * selectedElementsArray and return True or False based on that
 * evaluation */
Boolean alreadySelected(DlElement *element)
{
    DlElement *dlElement;

    if(!currentDisplayInfo) return (False);
    if(IsEmpty(currentDisplayInfo->selectedDlElementList)) return False;
    dlElement = FirstDlElement(currentDisplayInfo->selectedDlElementList);
    while(dlElement) {
	DlElement *pE = element->structure.element;
	if((pE->type != DL_Display) && (dlElement->structure.element == pE))
	  return True;
	dlElement = dlElement->next;
    }
    return (False);
}

static void toggleHighlightRectangles(DisplayInfo *displayInfo,
  int xOffset, int yOffset)
{
    DlElement *dlElement = FirstDlElement(displayInfo->selectedDlElementList);
    DlElementType type;
    DlObject *po;
    int width, height;
#if DEBUG_EVENTS > 1
    print("\n[toggleHighlightRectangles] selectedDlElement list :\n");
    dumpDlElementList(displayInfo->selectedDlElementList);
#endif
  /* Traverse the elements */
    while(dlElement) {
	if(dlElement->type == DL_Element) {
	    type = dlElement->structure.element->type;
	    po = &dlElement->structure.element->structure.composite->object;
	} else {     /* Should not be using this branch */
	    type = dlElement->type;
	    po = &dlElement->structure.composite->object;
	}
	width = ((int)po->width + xOffset);
	width = MAX(1,width);
	height = ((int)po->height + yOffset);
	height = MAX(1,height);
#if DEBUG_EVENTS > 1
	print("  %s (%s): x: %d y: %d width: %u height: %u\n"
	  "    xOffset: %d yOffset: %d Used-width: %d Used-height %d\n",
	  elementType(dlElement->type),
	  elementType(type),
	  po->x,po->y,po->width,po->height,xOffset,yOffset,width,height);
#endif
      /* If not the display, draw a rectangle */
	if(type != DL_Display) {
	    XDrawRectangle(XtDisplay(displayInfo->drawingArea),
	      XtWindow(displayInfo->drawingArea),xorGC,
	      po->x,po->y,(Dimension)width,(Dimension)height);
	}
      /* Set next element */
	dlElement = dlElement->next;
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
    if(element->type == DL_Display) return;
  /* Delete any children if it is composite */
    if(element->type == DL_Composite) {
	child = FirstDlElement(element->structure.composite->dlElementList);
	while(child) {
	    DlElement *pE = child;

	    if(pE->type == DL_Composite) {
		destroyElementWidgets(pE);
	    } else if(pE->widget) {
	      /* lookup widget of specified x,y,width,height and destroy */
		XtDestroyWidget(pE->widget);
		pE->widget = NULL;
	    }
	    child = child->next;
	}
    }
  /* Remove the widget */
    if(element->widget) {
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

    if(ele->type == DL_Composite) {

	child = FirstDlElement(ele->structure.composite->dlElementList);
	while(child) {
	    DlElement *pE = child;
	  /* if composite, delete any children */
	    if(pE->type == DL_Composite) {
		deleteWidgetsInComposite(displayInfo,pE);
	    } else
	      if(pE->widget) {
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
    int gridSpacing;
    Dimension width, height;
    int i, j, n;
    Arg args[2];

#if DEBUG_GRID
    {
	DlElement *dlElement = FirstDlElement(displayInfo->dlElementList);
	DlDisplay *dlDisplay = dlElement->structure.display;

	if(displayInfo->colormap) {
	    print("drawGrid:\n"
	      "  displayInfo: fg=%06x[%d] bg=%06x[%d] dlDisplay: "
	      "fg=%06x[%d] bg=%06x[%d]\n",
	      displayInfo->colormap[displayInfo->drawingAreaForegroundColor],
	      displayInfo->drawingAreaForegroundColor,
	      displayInfo->colormap[displayInfo->drawingAreaBackgroundColor],
	      displayInfo->drawingAreaBackgroundColor,
	      displayInfo->colormap[dlDisplay->clr],
	      dlDisplay->clr,
	      displayInfo->colormap[dlDisplay->bclr],
	      dlDisplay->bclr);
	} else {
	    print("drawGrid:\n"
	      "  displayInfo: fg=UNK[%d] bg=UNK[%d] dlDisplay: "
	      "fg=UNK[%d] bg=UNK[%d]\n",
	      displayInfo->drawingAreaForegroundColor,
	      displayInfo->drawingAreaBackgroundColor,
	      dlDisplay->clr,
	      dlDisplay->bclr);
	}
    }
#endif

  /* Return if displayInfo is invalid */
    if(!displayInfo || !displayInfo->drawingArea || !draw) return;
    gridSpacing = displayInfo->grid->gridSpacing;

  /* Get the size of the drawing area */
    n=0;
    XtSetArg(args[n],XmNwidth,&width); n++;
    XtSetArg(args[n],XmNheight,&height); n++;
    XtGetValues(displayInfo->drawingArea,args,n);

  /* Set the GC */
    XSetForeground(display,displayInfo->gc,
      displayInfo->colormap[displayInfo->drawingAreaForegroundColor]);
    XSetBackground(display,displayInfo->gc,
      displayInfo->colormap[displayInfo->drawingAreaBackgroundColor]);

  /* Draw grid */
    for(i=0; i < width; i+=gridSpacing) {
	for(j=0; j < height; j+=gridSpacing) {
	    XDrawPoint(display,draw,displayInfo->gc,i,j);
	}
    }
}

void copySelectedElementsIntoClipboard()
{
    DisplayInfo *cdi = currentDisplayInfo;
    DlElement *dlElement;

    if(!cdi) return;
    if(IsEmpty(cdi->selectedDlElementList)) return;

    if(!IsEmpty(clipboard)) {
	clearDlDisplayList(NULL, clipboard);
    }

    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while(dlElement) {
	DlElement *element = dlElement->structure.element;
	if(element->type != DL_Display) {
	    DlElement *pE = element->run->create(element);
	    appendDlElement(clipboard,pE);
	}
	dlElement = dlElement->next;
    }
}

int copyElementsIntoDisplay()
{
    DisplayInfo *cdi;
    int deltaX, deltaY;
    DlElement *dlElement;

  /* If no clipboard elements, simply return */
    if(IsEmpty(clipboard)) return 0;

  /* If no current display, simply return */
    cdi = currentDisplayInfo;
    if(!cdi) {
	medmPostMsg(1,"copyElementsIntoDisplay:  Can't determine current display\n");
	return 0;
    }
    XRaiseWindow(display,XtWindow(cdi->shell));
    XSetInputFocus(display,XtWindow(cdi->shell),RevertToParent,CurrentTime);
    unhighlightSelectedElements();

  /* Let user drag to paste */
    if(!doPasting(&deltaX, &deltaY)) return 0;

  /* Do actual element creation (with insertion into display list)
   *   Since elements are stored in clipboard in front-to-back order
   *   they can be pasted/copied into display in clipboard index order */
    saveUndoInfo(cdi);
    clearDlDisplayList(cdi, cdi->selectedDlElementList);
    dlElement = FirstDlElement(clipboard);
    while(dlElement) {
	if(dlElement->type != DL_Display) {
	    DlElement *pE, *pSE;

	    pE = dlElement->run->create(dlElement);
	    if(pE) {
		appendDlElement(cdi->dlElementList,pE);
	      /* execute the structure */
		if(pE->run->move) {
		    pE->run->move(pE, deltaX, deltaY);
		}
		if(pE->run->execute) (pE->run->execute)(cdi, pE);
		pSE = createDlElement(DL_Element,(XtPointer)pE,NULL);
		if(pSE) {
		    appendDlElement(cdi->selectedDlElementList,pSE);
		}
	    }
	}
	dlElement = dlElement->next;
    }
    highlightSelectedElements();
    if(cdi->selectedDlElementList->count == 1) {
	currentElementType =
	  FirstDlElement(cdi->selectedDlElementList)
	  ->structure.element->type;
	setResourcePaletteEntries();
    }

  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(cdi);

    return 1;
}

void deleteElementsInDisplay(DisplayInfo *displayInfo)
{
    DisplayInfo *cdi = currentDisplayInfo;
    DlElement *dlElement;

    if(!cdi) return;
    if(IsEmpty(cdi->selectedDlElementList)) return;
    saveUndoInfo(cdi);

  /* Unhighlight selected elements */
    unhighlightSelectedElements();
  /* Traverse the elements in the selected element list */
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while(dlElement) {
      /* Get the structure (element of union is as good as any) */
	DlElement *pE = dlElement->structure.element;
	if(pE->type != DL_Display) {
	  /* Destroy its widgets */
	    destroyElementWidgets(pE);
	  /* Remove it from the list */
	    removeDlElement(cdi->dlElementList,pE);
	  /* Destroy it with its destroy method if there is one */
	    if(pE->run->destroy) {
		pE->run->destroy(displayInfo, pE);
	    } else {
		genericDestroy(displayInfo, pE);
	    }
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

    if(!cdi) return;
  /* Clear resource palette */
    clearResourcePaletteEntries();
  /* Return if no selected elements */
    if(IsEmpty(cdi->selectedDlElementList)) return;
  /* Unhighlight and unselect */
    unhighlightSelectedElements();
    clearDlDisplayList(cdi, cdi->selectedDlElementList);
}

/*
 * Selects all renderable objects in display excluding the display itself
 */
void selectAllElementsInDisplay()
{
    DisplayInfo *cdi = currentDisplayInfo;
    DlElement *dlElement;

    if(!cdi) return;

  /* Unselect any selected elements */
    unselectElementsInDisplay();

  /* Loop over elements not including the display */
    dlElement = SecondDlElement(cdi->dlElementList);
    while(dlElement) {
	DlElement *pE;

	pE = createDlElement(DL_Element,(XtPointer)dlElement,NULL);
	if(pE) {
	    appendDlElement(cdi->selectedDlElementList,pE);
	}
	dlElement = dlElement->next;
    }
    highlightSelectedElements();
    if(cdi->selectedDlElementList->count == 1) {
	currentElementType =
	  FirstDlElement(cdi->selectedDlElementList)
	  ->structure.element->type;
	setResourcePaletteEntries();
    }
}

/*
 * Selects the display itself
 */
void selectDisplay()
{
    DisplayInfo *cdi = currentDisplayInfo;
    DlElement *dlElement;
    DlElement *pE;

    if(!cdi) return;

  /* Unselect any selected elements */
    unselectElementsInDisplay();

  /* Loop over elements not including the display */
    dlElement = FirstDlElement(cdi->dlElementList);
    pE = createDlElement(DL_Element,(XtPointer)dlElement,NULL);
    if(pE) {
	appendDlElement(cdi->selectedDlElementList,pE);
    }
    highlightSelectedElements();
    if(cdi->selectedDlElementList->count == 1) {
	currentElementType =
	  FirstDlElement(cdi->selectedDlElementList)
	  ->structure.element->type;
	setResourcePaletteEntries();
    }
}

/*
 * Move elements further up (traversed first) in display list
 *  so that these are "behind" other objects
 */
void lowerSelectedElements()
{
    DisplayInfo *cdi = currentDisplayInfo;
    DlElement *pE = 0; /* pointer to the element in the selected element list */
    DlElement *pFirst = 0; /* pointer to the first element in the display element list */
    DlElement *pTemp;

#if DEBUG_EVENTS
    print("\n[lowerSelectedElements:1]dlElementList :\n");
    dumpDlElementList(cdi->dlElementList);
    print("\n[lowerSelectedElements:1]selectedDlElementList :\n");
    dumpDlElementList(cdi->selectedDlElementList);
#endif
    if(IsEmpty(cdi->selectedDlElementList)) return;
  /* If the temporary list does not exist, create it */
    if(!tmpDlElementList) {
	tmpDlElementList=createDlList();
	if(!tmpDlElementList) {
	    medmPrintf(1,"\nlowerSelectedElements: Cannot create temporary element list\n");
	    return;
	}
    }
    clearDlDisplayList(NULL, tmpDlElementList);
    saveUndoInfo(cdi);

    unhighlightSelectedElements();

    pFirst = FirstDlElement(cdi->dlElementList);
    pE = LastDlElement(cdi->selectedDlElementList);
#if DEBUG_EVENTS > 1
    print("\n[lowerSelectedElements] selectedDlElement list :\n");
    dumpDlElementList(cdi->selectedDlElementList);
    print("\n[lowerSelectedElements] tmpDlElement list :\n");
    dumpDlElementList(tmpDlElementList);
#endif
    while(pE && (pE != cdi->selectedDlElementList->head)) {
	DlElement *pX = pE->structure.element;
#if DEBUG_EVENTS > 1
	print("   (%s) x=%d y=%d width=%u height=%u\n",
	  elementType(pE->structure.element->type),
	  pE->structure.element->structure.composite->object.x,
	  pE->structure.element->structure.composite->object.y,
	  pE->structure.element->structure.composite->object.width,
	  pE->structure.element->structure.composite->object.height);
#endif
	pTemp = pE->prev;
	if(pX->type != DL_Display) {
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
    if(cdi->selectedDlElementList->count == 1) {
	currentElementType =
	  FirstDlElement(cdi->selectedDlElementList)
	  ->structure.element->type;
	setResourcePaletteEntries();
    }
  /* Cleanup temporary list */
    clearDlDisplayList(NULL, tmpDlElementList);
#if DEBUG_EVENTS
    print("\n[lowerSelectedElements:2]dlElement list :\n");
    dumpDlElementList(cdi->dlElementList);
    print("\n[lowerSelectedElements:2]selectedDlElementList :\n");
    dumpDlElementList(cdi->selectedDlElementList);
#endif
}

/*
 * Move elements further down (traversed last) in display list
 *  so that these are "in front of" other objects
 */
void raiseSelectedElements()
{
    DisplayInfo *cdi = currentDisplayInfo;
    DlElement *pE = 0;
    DlElement *pTemp;

#if DEBUG_EVENTS
    print("\n[raiseSelectedElements:1]dlElementList :\n");
    dumpDlElementList(cdi->dlElementList);
    print("\n[raiseSelectedElements:1]selectedDlElementList :\n");
    dumpDlElementList(cdi->selectedDlElementList);
#endif
    if(IsEmpty(cdi->selectedDlElementList)) return;
  /* If the temporary list does not exist, create it */
    if(!tmpDlElementList) {
	tmpDlElementList=createDlList();
	if(!tmpDlElementList) {
	    medmPrintf(1,"\nraiseSelectedElements: Cannot create temporary element list\n");
	    return;
	}
    }
    clearDlDisplayList(NULL, tmpDlElementList);
    saveUndoInfo(cdi);

    unhighlightSelectedElements();

    pE = FirstDlElement(cdi->selectedDlElementList);
    while(pE) {
	DlElement *pX = pE->structure.element;

	pTemp = pE->next;
	if(pX->type != DL_Display) {
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
    if(cdi->selectedDlElementList->count == 1) {
	currentElementType =
	  FirstDlElement(cdi->selectedDlElementList)
	  ->structure.element->type;
	setResourcePaletteEntries();
    }
  /* Cleanup temporary list */
    clearDlDisplayList(NULL, tmpDlElementList);
#if DEBUG_EVENTS
    print("\n[raiseSelectedElements:2]dlElement list :\n");
    dumpDlElementList(cdi->dlElementList);
    print("\n[raiseSelectedElements:2]selectedDlElementList :\n");
    dumpDlElementList(cdi->selectedDlElementList);
#endif
}

/*
 * Align selected elements by top, bottom, left, or right edges
 */
void alignSelectedElements(int alignment)
{
    int minX, minY, maxX, maxY, deltaX, deltaY, x0, y0, xOffset, yOffset;
    DisplayInfo *cdi;
    DlElement *ele;
    DlElement *dlElement;

    if(!currentDisplayInfo) return;
    cdi = currentDisplayInfo;
    if(IsEmpty(cdi->selectedDlElementList)) return;
    if(NumberOfDlElement(cdi->selectedDlElementList) == 1) return;
    saveUndoInfo(cdi);

    minX = INT_MAX; minY = INT_MAX;
    maxX = INT_MIN; maxY = INT_MIN;

    unhighlightSelectedElements();

  /* Loop and get min/max (x,y) values */
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while(dlElement) {
	DlObject *po =
	  &(dlElement->structure.element->structure.rectangle->object);
	minX = MIN(minX, po->x);
	minY = MIN(minY, po->y);
	x0 = (po->x + (int)po->width);
	maxX = MAX(maxX,x0);
	y0 = (po->y + (int)po->height);
	maxY = MAX(maxY,y0);
	dlElement = dlElement->next;
    }
    deltaX = (minX + maxX)/2;
    deltaY = (minY + maxY)/2;

  /* Loop and set x,y values, and move if widgets */
    dlElement = LastDlElement(cdi->selectedDlElementList);
    while(dlElement != cdi->selectedDlElementList->head) {
	ele = dlElement->structure.element;

      /* Can't move the display */
	if(ele->type != DL_Display) {
	    switch(alignment) {
	    case ALIGN_HORIZ_LEFT:
		xOffset = minX - ele->structure.rectangle->object.x;
		yOffset = 0;
		break;
	    case ALIGN_HORIZ_CENTER:
	      /* Want   x + w/2 = dX  , therefore   x = dX - w/2   */
		xOffset = (deltaX - (int)ele->structure.rectangle->object.width/2)
		  - ele->structure.rectangle->object.x;
		yOffset = 0;
		break;
	    case ALIGN_HORIZ_RIGHT:
	      /* Want   x + w = maxX  , therefore   x = maxX - w  */
		xOffset = (maxX - (int)ele->structure.rectangle->object.width)
                  - ele->structure.rectangle->object.x;
		yOffset = 0;
		break;
	    case ALIGN_VERT_TOP:
		xOffset = 0;
		yOffset = minY - (int)ele->structure.rectangle->object.y;
		break;
	    case ALIGN_VERT_CENTER:
	      /* Want   y + h/2 = dY  , therefore   y = dY - h/2   */
		xOffset = 0;
		yOffset = (deltaY - (int)ele->structure.rectangle->object.height/2)
                  - ele->structure.rectangle->object.y;
		break;
	    case ALIGN_VERT_BOTTOM:
	      /* Want   y + h = maxY  , therefore   y = maxY - h  */
		xOffset = 0;
		yOffset = (maxY - (int)ele->structure.rectangle->object.height)
		  - ele->structure.rectangle->object.y;
		break;
	    }
	    if(ele->run->move) {
		ele->run->move(ele,xOffset,yOffset);
	    }
	    if(ele->widget) {
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
 * Orient selected elements (flip and rotate)
 */
void orientSelectedElements(int alignment)
{
    int minX, minY, maxX, maxY, centerX, centerY, x0, y0;
    DisplayInfo *cdi;
    DlElement *ele;
    DlElement *dlElement;

    if(!currentDisplayInfo) return;
    cdi = currentDisplayInfo;
    if(IsEmpty(cdi->selectedDlElementList)) return;
    saveUndoInfo(cdi);

    minX = INT_MAX; minY = INT_MAX;
    maxX = INT_MIN; maxY = INT_MIN;

    unhighlightSelectedElements();

  /* Loop and get min/max (x,y) values */
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while(dlElement) {
	DlObject *po =
	  &(dlElement->structure.element->structure.rectangle->object);
	minX = MIN(minX, po->x);
	minY = MIN(minY, po->y);
	x0 = (po->x + (int)po->width);
	maxX = MAX(maxX,x0);
	y0 = (po->y + (int)po->height);
	maxY = MAX(maxY,y0);
	dlElement = dlElement->next;
    }
    centerX = (minX + maxX)/2;
    centerY = (minY + maxY)/2;

  /* Loop and set x,y values, and move if widgets */
    dlElement = LastDlElement(cdi->selectedDlElementList);
    while(dlElement != cdi->selectedDlElementList->head) {
	ele = dlElement->structure.element;

      /* Can't move the display (but there isn't any orient method anyway) */
	if(ele->type != DL_Display) {
	    if(ele->run->orient) {
		ele->run->orient(ele,alignment,centerX,centerY);
		if(ele->widget) {
		  /* Destroy the widget */
		    destroyElementWidgets(ele);
		  /* Recreate it */
		    ele->run->execute(cdi,ele);
		}
	    }
	}
	dlElement = dlElement->prev;
    }
  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(cdi);

  /* Update resource palette if there is only one element */
    if(NumberOfDlElement(cdi->selectedDlElementList) == 1) {
	currentElementType =
	  FirstDlElement(cdi->selectedDlElementList)
	  ->structure.element->type;
	setResourcePaletteEntries();
    }

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

    if(!currentDisplayInfo) return;
    cdi = currentDisplayInfo;
    nele = NumberOfDlElement(cdi->selectedDlElementList);
    if(nele < 2) return;
    saveUndoInfo(cdi);
    gridSpacing=cdi->grid->gridSpacing;

  /* Allocate space */
    earray = (DlElement **)calloc(nele,sizeof(DlElement *));
    if(!earray) return;
    array = (double *)calloc(nele,sizeof(double));
    if(!array) {
	medmPrintf(1,"\nspaceSelectedElements: Memory allocation error\n");
	free((char *)earray);
	return;
    }
    indx = (int *)calloc(nele,sizeof(int));
    if(!indx) {
	medmPrintf(1,"\nspaceSelectedElements: Memory allocation error\n");
	free((char *)earray);
	free((char *)array);
	return;
    }

    unhighlightSelectedElements();

  /* Loop and put elements and z into the arrays */
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    i=0;
    while(dlElement) {
	DlObject *po =
	  &(dlElement->structure.element->structure.rectangle->object);
	earray[i]=dlElement;
	if(plane == SPACE_HORIZ) {
	    array[i]=po->x;
	} else if(plane == SPACE_VERT) {
	    array[i]=po->y;
	}
	dlElement = dlElement->next;
	i++;
    }

#if 0
    print("\nnele=%d\n",nele);
    for(i=0; i < nele; i++) {
	print("array[%d]=%f indx[%d]=%d\n",i,array[i],i,indx[i]);
    }
#endif
  /* Sort elements by position */
    hsort(array,indx,nele);
#if 0
    print("nele=%d\n",nele);
    for(i=0; i < nele; i++) {
	print("array[%d]=%f indx[%d]=%d\n",i,array[i],i,indx[i]);
    }
#endif

  /* Loop and and move */
    z = -1;
    for(i=0; i < nele; i++) {
	dlElement = earray[indx[i]];
	pE = dlElement->structure.element;
      /* Can't move the display */
	if(pE->type != DL_Display) {
	  /* Get position of first element to start */
	    if(z < 0) {
		if(plane == SPACE_HORIZ) {
		    z = pE->structure.rectangle->object.x;
		} else if(plane == SPACE_VERT) {
		    z = pE->structure.rectangle->object.y;
		}
	    } else {
		if(plane == SPACE_HORIZ) {
		    deltaz = z - pE->structure.rectangle->object.x;
		    if(pE->run->move) {
			pE->run->move(pE,deltaz,0);
		    }
		} else if(plane == SPACE_VERT) {
		    deltaz = z - pE->structure.rectangle->object.y;
		    if(pE->run->move) {
			pE->run->move(pE,0,deltaz);
		    }
		}
		if(pE->widget) {
		    XtMoveWidget(pE->widget,
		      (Position) pE->structure.rectangle->object.x,
		      (Position) pE->structure.rectangle->object.y);
		}
	    }
	  /* Get next position */
	    if(plane == SPACE_HORIZ) {
		z += ((int)pE->structure.rectangle->object.width + gridSpacing);
	    } else if(plane == SPACE_VERT) {
		z += ((int)pE->structure.rectangle->object.height + gridSpacing);
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
    int y, height, deltay, x, width, deltax, gridSpacing;
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

    if(!currentDisplayInfo) return;
    cdi = currentDisplayInfo;
    n = NumberOfDlElement(cdi->selectedDlElementList);
    if(n < 2) return;
    saveUndoInfo(cdi);
    gridSpacing=cdi->grid->gridSpacing;

  /* Determine the number of rows */
    minY = minX = INT_MAX;
    maxY = INT_MIN;
    nrows = 1;
    n = avgH = 0;
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while(dlElement) {
	pE = dlElement->structure.element;
      /* Don't include the display */
	if(pE->type != DL_Display) {
	    DlObject *po =  &(pE->structure.rectangle->object);
	    ytop = po->y;
	    height = (int)po->height;
	    ybottom = ytop + height;
	    avgH += height;
	    if(ytop < minY) minY = ytop;
	    if(ybottom > maxY) maxY = ybottom;
	    n++;
	  /* Loop and find elements whose centers are between xleft and xright */
	    xleft = po->x;
	    if(xleft < minX) minX = xleft;
	    width = (int)po->width;
	    xright = xleft + width;
	    nrows1 = 0;
	    dlElement1 = FirstDlElement(cdi->selectedDlElementList);
	    while(dlElement1) {
		pE = dlElement1->structure.element;
		if(pE->type != DL_Display) {
		    DlObject *po =  &(pE->structure.rectangle->object);
		    xcen1 = (int)(po->x + .5*(double)po->width +.5);
		    if(xcen1 >= xleft && xcen1 <= xright) nrows1++;
		}
		dlElement1 = dlElement1->next;
	    }
	    if(nrows1 > nrows) nrows = nrows1;
	}
	dlElement = dlElement->next;
    }
    if(n < 1) return;
    avgH = (int)((double)(avgH)/(double)(n)+.5);
    deltaY = (int)((double)(maxY - minY)/(double)nrows + .99999);
    maxY = minY + nrows * deltaY;     /* (Adjust maxY) */

  /* Allocate array to hold the number of elements for each row */
    nele=(int *)calloc(nrows,sizeof(int));
    if(!nele) {
	medmPrintf(1,"\nspaceSelectedElements2D: Memory allocation error\n");
	return;
    }

  /* Loop and count the number of elements in each row */
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while(dlElement) {
	pE = dlElement->structure.element;
      /* Don't include the display */
	if(pE->type != DL_Display) {
	    DlObject *po =  &(pE->structure.rectangle->object);
	    ytop = po->y;
	    ycen = (int)(po->y + .5*(double)po->height +.5);
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
    if(!earray || !array || !indx) {
	medmPrintf(1,"\nspaceSelectedElements2D: Memory allocation error\n");
	highlightSelectedElements();
	return;
    }
    for(i=0; i < nrows; i++) {
	if(nele[i] <= 0)  continue;
	earray[i] = (DlElement **)calloc(nele[i],sizeof(DlElement *));
	array[i] = (double *)calloc(nele[i],sizeof(double));
	indx[i] = (int *)calloc(nele[i],sizeof(int));
	if(!earray[i] || !array[i] || !indx[i]) {
	    medmPrintf(1,"\nspaceSelectedElements2D: Memory allocation error\n");
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
	while(dlElement) {
	    pE = dlElement->structure.element;
	  /* Don't include the display */
	    if(pE->type != DL_Display) {
		DlObject *po =  &(pE->structure.rectangle->object);
		ytop = po->y;
		ycen = (int)(po->y + .5*(double)po->height +.5);
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
	    if(pE->type != DL_Display) {
	      /* Set position of first element */
		if(x < 0) {
		    x = minX;
		}
		deltax = x - pE->structure.rectangle->object.x;
		deltay = y - pE->structure.rectangle->object.y;
		if(pE->run->move) {
		    pE->run->move(pE,deltax,deltay);
		    if(pE->widget) {
			    XtMoveWidget(pE->widget,
			      (Position) pE->structure.rectangle->object.x,
			      (Position) pE->structure.rectangle->object.y);
		    }
		}
	      /* Get next position */
		x += ((int)pE->structure.rectangle->object.width + gridSpacing);
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
void alignSelectedElementsToGrid(Boolean edges)
{
    DisplayInfo *cdi = currentDisplayInfo;
    int gridSpacing;
    int x, y, x0, y0, x00, y00, xoff, yoff, redraw;
    DlElement *pE;
    DlElement *dlElement;

    if(!cdi) return;
    if(IsEmpty(cdi->selectedDlElementList)) return;
    saveUndoInfo(cdi);
    gridSpacing = cdi->grid->gridSpacing;

    unhighlightSelectedElements();

  /* Loop and move the corners to grid */
    dlElement = LastDlElement(cdi->selectedDlElementList);
    while(dlElement != cdi->selectedDlElementList->head) {
	pE = dlElement->structure.element;
      /* Don't include the display */
	if(pE->type != DL_Display) {
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

	    redraw=0;

	  /* Move it only if the new values are different */
	    if(xoff != 0 || yoff != 0) {
		redraw=1;
		if(pE->run->move) {
		    pE->run->move(pE,-xoff,-yoff);
		}
	    }

	  /* Lower right */
	    if(edges) {
		x = x00 + (int)pE->structure.rectangle->object.width;
		x0 = (x/gridSpacing)*gridSpacing;
		xoff = x - x0;
		x0=x-xoff;
		if(xoff > gridSpacing/2) xoff -= gridSpacing;

		y = y00 + (int)pE->structure.rectangle->object.height;
		y0 = (y/gridSpacing)*gridSpacing;
		yoff = y - y0;
		y0=y-yoff;
		if(yoff > gridSpacing/2) yoff -= gridSpacing;

		if(xoff != 0 || yoff != 0) {
		    redraw=1;
		    if(pE->run->scale) {
			pE->run->scale(pE,-xoff,-yoff);
		    }
		}
	    }

	  /* Redraw widgets if necessary */
	    if(redraw) {
		if(pE->widget) {
		  /* Destroy the widget */
		    destroyElementWidgets(pE);
		  /* Recreate it */
		    if(pE->run->execute) pE->run->execute(cdi,pE);
		}
	    }
	}
	dlElement = dlElement->prev;
    }

  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(cdi);

  /* Update resource palette if there is only one element */
    if(NumberOfDlElement(cdi->selectedDlElementList) == 1) {
	currentElementType =
	  FirstDlElement(cdi->selectedDlElementList)
	  ->structure.element->type;
	setResourcePaletteEntries();
    }

    highlightSelectedElements();
}

/*
 * Find objects lying outside the visible region of the display
 */
void findOutliers(void)
{
    int displayH, displayW;
    DisplayInfo *cdi;
    DlElement *dlElement, *pE, *pD;
    DlObject *pO;
    int partial=0, total=0;
    char string[240];

    if(!currentDisplayInfo) return;
    cdi = currentDisplayInfo;

  /* Unselect any selected elements */
    unselectElementsInDisplay();

  /* Get the (current) height and width for display */
    pD = FirstDlElement(cdi->dlElementList);
    if(!pD) return;
    displayW = pD->structure.display->object.width;
    displayH = pD->structure.display->object.height;

  /* Loop over elements */
    dlElement = FirstDlElement(cdi->dlElementList);
    while(dlElement) {
	pO = &(dlElement->structure.rectangle->object);

      /* Omit the display */
	if(dlElement->type != DL_Display) {
	    if((pO->x) > displayW ||
	      (pO->x + (int)pO->width) < 0 ||
	      (pO->y) > displayH ||
	      (pO->y + (int)pO->height) < 0) {
		total++;
		pE = createDlElement(DL_Element, (XtPointer)dlElement, NULL);
		if(pE) {
		    appendDlElement(cdi->selectedDlElementList, pE);
		}
	    } else if((pO->x) < 0 ||
	      (pO->x + (int)pO->width) > displayW ||
	      (pO->y) < 0 ||
	      (pO->y + (int)pO->height) > displayH) {
		partial++;
		pE = createDlElement(DL_Element, (XtPointer)dlElement, NULL);
		if(pE) {
		    appendDlElement(cdi->selectedDlElementList, pE);
		}
	    }
	}
	dlElement = dlElement->next;
    }

  /* Display results */
    if(partial || total) {
	sprintf(string,"There are %d objects partially out of the visible display area.\n"
	  "There are %d objects totally out of the visible display area.\n\n"
	  "These %d objects are currently selected.",
	  partial, total, partial + total);
	highlightSelectedElements();
	dmSetAndPopupWarningDialog(cdi, string, "OK", "List Them", NULL);
	if(cdi->warningDialogAnswer == 2) {
	    sprintf(string,"Outliers:\n"
	      "Display width=%d  Display height=%d\n",
	      displayW, displayH);
	    medmPrintfDlElementList(cdi->selectedDlElementList, string);
	}
    } else {
	sprintf(string,"There are %d objects partially out of the visible display area.\n"
	  "There are %d objects totally out of the visible display area.",
	  partial, total);
	highlightSelectedElements();
	dmSetAndPopupWarningDialog(cdi, string, "OK", NULL, NULL);
    }
}

/*
 * Center selected elements in the display
 */
void centerSelectedElements(int alignment)
{
    int minX, minY, maxX, maxY, deltaX, deltaY, x0, y0, xOffset, yOffset;
    Position displayH, displayW;
    DisplayInfo *cdi;
    DlElement *pE;
    DlElement *dlElement;

    if(!currentDisplayInfo) return;
    cdi = currentDisplayInfo;
    if(IsEmpty(cdi->selectedDlElementList)) return;
    saveUndoInfo(cdi);

    minX = INT_MAX; minY = INT_MAX;
    maxX = INT_MIN; maxY = INT_MIN;

    unhighlightSelectedElements();

  /* Loop and get min/max (x,y) values */
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while(dlElement) {
	DlObject *po =
	  &(dlElement->structure.element->structure.rectangle->object);
	minX = MIN(minX, po->x);
	minY = MIN(minY, po->y);
	x0 = (po->x + (int)po->width);
	maxX = MAX(maxX,x0);
	y0 = (po->y + (int)po->height);
	maxY = MAX(maxY,y0);
	dlElement = dlElement->next;
    }
    deltaX = (minX + maxX)/2;
    deltaY = (minY + maxY)/2;

  /* Get the (current) height and width for display */
    XtVaGetValues(cdi->drawingArea,
      XmNwidth, &displayW,
      XmNheight, &displayH,
      NULL);

  /* Find the offsets */
    switch(alignment) {
    case ALIGN_HORIZ_CENTER:
	xOffset = displayW/2 - deltaX;
	yOffset = 0;
	break;
    case ALIGN_VERT_CENTER:
	xOffset = 0;
	yOffset = displayH/2 - deltaY;
	break;
    }

  /* Loop and set x,y values, and move if widgets */
    dlElement = LastDlElement(cdi->selectedDlElementList);
    while(dlElement != cdi->selectedDlElementList->head) {
	pE = dlElement->structure.element;

      /* Can't move the display */
	if(pE->type != DL_Display) {
	    if(pE->run->move) {
		pE->run->move(pE, xOffset, yOffset);
	    }
	    if(pE->widget) {
		XtMoveWidget(pE->widget,
		  (Position)pE->structure.rectangle->object.x,
		  (Position)pE->structure.rectangle->object.y);
	    }
	}
	dlElement = dlElement->prev;
    }

  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(cdi);

    highlightSelectedElements();
}

/*
 * Make text elements fit the enclosed text
 */
void sizeSelectedTextElements(void)
{
    int i = 0, usedWidth, usedHeight, xOffset, dWidth;
    DisplayInfo *cdi;
    DlElement *dlElement, *pE;
    DlText *dlText;
    size_t nChars;

    if(!currentDisplayInfo) return;
    cdi = currentDisplayInfo;
    if(IsEmpty(cdi->selectedDlElementList)) return;
    saveUndoInfo(cdi);

    unhighlightSelectedElements();

  /* Loop over selected elements */
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while(dlElement) {
	pE = dlElement->structure.element;
	if(pE->type == DL_Text) {
	    dlText = pE->structure.text;

	  /* Get used width */
	    nChars = strlen(dlText->textix);
	    i = dmGetBestFontWithInfo(fontTable,MAX_FONTS,dlText->textix,
	      dlText->object.height,dlText->object.width,
	      &usedHeight,&usedWidth,FALSE);
	    usedWidth = XTextWidth(fontTable[i],dlText->textix,nChars);
	    dWidth = usedWidth - (int)dlText->object.width;

	  /* Get new position (before changing width) */
	    switch (dlText->align) {
	    case HORIZ_LEFT:
		xOffset = 0;
		break;
	    case HORIZ_CENTER:
		xOffset = ((int)dlText->object.width - usedWidth)/2;
		break;
	    case HORIZ_RIGHT:
		xOffset = (int)dlText->object.width - usedWidth;
		break;
	    }

	  /* Resize */
	    if(pE->run->scale && dWidth) {
		pE->run->scale(pE, dWidth, 0);
	    }

	  /* Move */
	    if(pE->run->move && xOffset) {
		pE->run->move(pE, xOffset, 0);
	    }
	}
	dlElement = dlElement->next;
    }

  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(cdi);

    highlightSelectedElements();
}

/*
 * Make selected elements the same size
 */
void equalSizeSelectedElements(void)
{
    int n, avgW, avgH;
    DisplayInfo *cdi;
    DlElement *pE;
    DlElement *dlElement;

    if(!currentDisplayInfo) return;
    cdi = currentDisplayInfo;
    if(IsEmpty(cdi->selectedDlElementList)) return;
    saveUndoInfo(cdi);

    unhighlightSelectedElements();

/* Loop and get avgerage height and width values */
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    avgW = avgH = n = 0;
    while(dlElement) {
	pE = dlElement->structure.element;
      /* Don't include the display */
	if(pE->type != DL_Display) {
	    DlObject *po =  &(pE->structure.rectangle->object);
	    avgW += (int)po->width;
	    avgH += (int)po->height;
	    n++;
	}
	dlElement = dlElement->next;
    }
    if(n < 1) return;
    avgW = (int)((double)(avgW)/(double)(n)+.5);
    avgH = (int)((double)(avgH)/(double)(n)+.5);

/* Loop and set width and height values to average */
    dlElement = LastDlElement(cdi->selectedDlElementList);
    while(dlElement != cdi->selectedDlElementList->head) {
	pE = dlElement->structure.element;
      /* Don't include the display */
	if(pE->type != DL_Display) {
	    DlObject *po =  &(pE->structure.rectangle->object);
	    if(pE->run->scale) {
		pE->run->scale(pE,avgW - (int)po->width,avgH - (int)po->height);
	    }
	    if(pE->widget) {
	      /* Destroy the widget */
		destroyElementWidgets(pE);
	      /* Recreate it */
		if(pE->run->execute) pE->run->execute(cdi,pE);
	    }
	}
	dlElement = dlElement->prev;
    }
  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(cdi);

    highlightSelectedElements();
}

/* Moves specified <src> element to position just after specified
 <dst> element.  */
void moveElementAfter(DlElement *dst, DlElement *src, DlElement **tail)
{
    if(dst == src) return;
    if(src == *tail) {
	src->prev->next = NULL;
	*tail = src->prev;
    } else {
	src->prev->next = src->next;
	src->next->prev = src->prev;
    }
    if(dst == *tail) {
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
    DisplayInfo *cdi;
    DlElement *afterElement;
    DlElement *dlElement;

    if(!displayInfo) return;
    cdi = displayInfo;
    if(IsEmpty(cdi->selectedDlElementList)) return;
    afterElement = afterThisElement;

    dlElement = LastDlElement(cdi->selectedDlElementList);
    while(dlElement != cdi->selectedDlElementList->head) {
      /* if display was selected, skip over it (can't raise/lower it) */
	DlElement *pE = dlElement->structure.element;
	if(pE->type != DL_Display) {
#if 0
	    moveElementAfter(afterElement,pE,&(cdi->dlElementListTail));
#endif
	    afterElement = afterElement->next;
	}
	dlElement = dlElement->prev;
    }
}

/*** Name value table routines ***/

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

    if(argsString != NULL) {

	copyOfArgsString = STRDUP(argsString);
      /* see how many a=b name/value pairs are in the string */
	numPairs = 0;
	i = 0;
	while(copyOfArgsString[i++] != '\0')
	  if(copyOfArgsString[i] == '=') numPairs++;


	tableIndex = 0;
	first = True;
	for(numEntries = 0; numEntries < numPairs; numEntries++) {

	  /* at least one pair, proceed */
	    if(first) {
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
	    if(name != NULL && value != NULL) {
	      /* found legitimate name/value pair, put in table */
		j = 0;
		for(i = 0; i < (int) strlen(name); i++) {
		    if(!isspace(name[i]))
		      nameEntry[j++] =  name[i];
		}
		nameEntry[j] = '\0';
		j = 0;
		for(i = 0; i < (int) strlen(value); i++) {
		    if(!isspace(value[i]))
		      valueEntry[j++] =  value[i];
		}
		valueEntry[j] = '\0';
		nameTable[tableIndex].name = STRDUP(nameEntry);
		nameTable[tableIndex].value = STRDUP(valueEntry);
		tableIndex++;
	    }
	}
	if(copyOfArgsString) free(copyOfArgsString);

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
    if(nameValueTable != NULL && numEntries > 0) {
	for(i = 0; i < numEntries; i++)
	  if(!strcmp(nameValueTable[i].name,name))
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
    if(nameValueTable != NULL) {
	for(i = 0; i < numEntries; i++) {
	    if(nameValueTable[i].name != NULL) free ((char *)nameValueTable[i].name);
	    if(nameValueTable[i].value != NULL) free ((char *)
	      nameValueTable[i].value);
	}
	free ((char *)nameValueTable);
    }

}

DisplayInfo *findDisplay(char *filename, char *argsString,
  char *relatedDisplayFilename)
{
    DisplayInfo *retVal = NULL;
    DisplayInfo *di;
    NameValueTable *nameTable;
    int numNameValues;
    int i, matched;
    char pathName[MAX_TOKEN_LENGTH];
    FILE *filePtr;

  /* Generate the name-value table for these args */
    if(argsString) {
	nameTable = generateNameValueTable(argsString, &numNameValues);
    } else {
	nameTable = NULL;
	numNameValues = 0;
    }

  /* Open the file so we can get the name that would be used */
    strncpy(pathName, filename, MAX_TOKEN_LENGTH);
    filePtr = dmOpenUsableFile(pathName, relatedDisplayFilename);
    if(!filePtr) {
      /* It can't be opened anyway */
	return NULL;
    } else {
      /* We just wanted the pathName */
	fclose(filePtr);
    }

  /* Loop over displays */
    di = displayInfoListHead->next;
    while(di) {
      /* Continue if the pathName does not match */
	if(strcmp(di->dlFile->name, pathName)) {
	    di = di->next;
	    continue;
	}
      /* Continue if the number of name-values does not match */
	if(di->numNameValues != numNameValues) {
	    di = di->next;
	    continue;
	}
      /* Check each name and value for a match */
	matched = 1;
	for(i=0; i < di->numNameValues; i++) {
	    if(strcmp(di->nameValueTable[i].name, nameTable[i].name)) {
		matched = 0;
		break;
	    }
	    if(strcmp(di->nameValueTable[i].value, nameTable[i].value)) {
		matched = 0;
		break;
	    }
	}
      /* Return if everything matched */
	if(matched) {
	    retVal = di;
	    break;
	}
	di = di->next;
    }

  /* Free the nameTable */
    if(nameTable) freeNameValueTable(nameTable, numNameValues);

    return retVal;
}

/*
 * Utility function to perform macro substitutions on input string, putting
 *   substituted string in specified output string (up to sizeOfOutputString
 *   bytes)
 */
void performMacroSubstitutions(DisplayInfo *displayInfo,
  char *inputString, char *outputString, int sizeOfOutputString)
{
    int i, j, k, n;
    char *value, name[MAX_TOKEN_LENGTH];

    outputString[0] = '\0';
    if(!displayInfo) {
	strncpy(outputString,inputString,sizeOfOutputString-1);
	outputString[sizeOfOutputString-1] = '\0';
	return;
    }

    i = 0; j = 0; k = 0;
    if(inputString && strlen(inputString) > (size_t)0) {
	while(inputString[i] != '\0' && j < sizeOfOutputString-1) {
	    if( inputString[i] != '$') {
		outputString[j++] = inputString[i++];
	    } else {
	      /* found '$', see if followed by '(' */
		if(inputString[i+1] == '(' ) {
		    i = i+2;
		    while(inputString[i] != ')'  && inputString[i] != '\0' ) {
			name[k++] = inputString[i++];
		    }
		    name[k] = '\0';
		  /* now lookup macro */
		    value = lookupNameValue(displayInfo->nameValueTable,
		      displayInfo->numNameValues,name);
		    if(value) {
			n = 0;
			while(value[n] != '\0' && j < sizeOfOutputString-1)
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
    if(j >= sizeOfOutputString-1) {
	medmPostMsg(1,"performMacroSubstitutions: Substitutions failed\n"
	  "  Output buffer not large enough\n");
    }
}

#if EXPLICITLY_OVERWRITE_CDE_COLORS
/*
 * colorMenuBar - get CDE and its "ColorSetId" straightened out...
 *   color the passed in widget (usually menu bar) and its children
 *   to the specified foreground/background colors
 */
void colorMenuBar(Widget widget, Pixel fg, Pixel bg)
{
    Cardinal numChildren;
    WidgetList children;
    Arg args[4];
    int i;
    Pixel localFg,top,bottom,select;

    XtSetArg(args[0],XmNchildren,&children);
    XtSetArg(args[1],XmNnumChildren,&numChildren);
    XtGetValues(widget,args,2);

    XmGetColors(XtScreen(widget),cmap,bg,&localFg,&top,&bottom,&select);
    XtSetArg(args[0],XmNforeground,fg);
    XtSetArg(args[1],XmNbackground,bg);

    XtSetArg(args[2],XmNtopShadowColor,top);
    XtSetArg(args[3],XmNbottomShadowColor,bottom);
    XtSetValues(widget,args,4);

    for(i = 0; i < (int)numChildren; i++) {
	XtSetValues(children[i],args,2);
    }
}
#endif

#if EXPLICITLY_OVERWRITE_CDE_COLORS
/*
 * colorPulldownMenu - get CDE and its "ColorSetId" straightened out...
 *   color the passed in widget (which is the menu) and its children
 *   to the specified foreground/background colors
 */
void colorPulldownMenu(Widget widget, Pixel fg, Pixel bg)
{
    Widget menu,optionButton;
    Cardinal numChildren;
    WidgetList children;
    Arg args[4];
    int i;
    Pixel localFg,top,bottom,select;

  /* Get the menu widget */
    XtSetArg(args[0],XmNsubMenuId,&menu);
    XtGetValues(widget,args,1);

  /* Get the option button child of the widget */
    optionButton=XmOptionButtonGadget(widget);

  /* Get the children of the menu widget */
    XtSetArg(args[0],XmNchildren,&children);
    XtSetArg(args[1],XmNnumChildren,&numChildren);
    XtGetValues(menu,args,2);

  /* Determine the colors */
    XmGetColors(XtScreen(widget),cmap,bg,&localFg,&top,&bottom,&select);

  /* Set the colors */
    XtSetArg(args[0],XmNforeground,fg);
    XtSetArg(args[1],XmNbackground,bg);
    XtSetArg(args[2],XmNtopShadowColor,top);
    XtSetArg(args[3],XmNbottomShadowColor,bottom);
    XtSetValues(widget,args,4);
    XtSetValues(optionButton,args,4);
    XtSetValues(menu,args,4);
    for(i = 0; i < (int)numChildren; i++) {
	XtSetValues(children[i],args,2);
    }
}
#endif

void questionDialogCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
    DisplayInfo *displayInfo = (DisplayInfo *) clientData;
    XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *) callbackStruct;

    UNREFERENCED(w);


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

    if(displayInfo->questionDialog == NULL) {
      /* this doesn't seem to be working (and should check if MWM is running) */
	displayInfo->questionDialog = XmCreateQuestionDialog(displayInfo->shell,"questionDialog",NULL,0);
	XtVaSetValues(displayInfo->questionDialog, XmNdialogStyle,XmDIALOG_APPLICATION_MODAL, NULL);
	XtVaSetValues(XtParent(displayInfo->questionDialog),XmNtitle,"Question ?",NULL);
	XtAddCallback(displayInfo->questionDialog,XmNokCallback,questionDialogCb,displayInfo);
	XtAddCallback(displayInfo->questionDialog,XmNcancelCallback,questionDialogCb,displayInfo);
	XtAddCallback(displayInfo->questionDialog,XmNhelpCallback,questionDialogCb,displayInfo);
    }
    if(message == NULL) return;
    xmString = XmStringCreateLtoR(message,XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(displayInfo->questionDialog,XmNmessageString,xmString,NULL);
    XmStringFree(xmString);
    if(okBtnLabel) {
	xmString = XmStringCreateLocalized(okBtnLabel);
	XtVaSetValues(displayInfo->questionDialog,XmNokLabelString,xmString,NULL);
	XmStringFree(xmString);
	XtManageChild(XmMessageBoxGetChild(displayInfo->questionDialog,XmDIALOG_OK_BUTTON));
    } else {
	XtUnmanageChild(XmMessageBoxGetChild(displayInfo->questionDialog,XmDIALOG_OK_BUTTON));
    }
    if(cancelBtnLabel) {
	xmString = XmStringCreateLocalized(cancelBtnLabel);
	XtVaSetValues(displayInfo->questionDialog,XmNcancelLabelString,xmString,NULL);
	XmStringFree(xmString);
	XtManageChild(XmMessageBoxGetChild(displayInfo->questionDialog,XmDIALOG_CANCEL_BUTTON));
    } else {
	XtUnmanageChild(XmMessageBoxGetChild(displayInfo->questionDialog,XmDIALOG_CANCEL_BUTTON));
    }
    if(helpBtnLabel) {
	xmString = XmStringCreateLocalized(helpBtnLabel);
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
    while(!displayInfo->questionDialogAnswer || XtAppPending(appContext)) {
	XtAppNextEvent(appContext,&event);
	XtDispatchEvent(&event);
    }
    XtUnmanageChild(displayInfo->questionDialog);
    XtRemoveGrab(XtParent(displayInfo->questionDialog));
}

void warningDialogCb(Widget w, XtPointer clientData, XtPointer callbackStruct)
{
    DisplayInfo *displayInfo = (DisplayInfo *) clientData;
    XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *) callbackStruct;

    UNREFERENCED(w);

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

  /* Create the dialog if necessary */

    if(displayInfo->warningDialog == NULL) {
      /* this doesn't seem to be working (and should check if MWM is running) */
	displayInfo->warningDialog =
	  XmCreateWarningDialog(displayInfo->shell,"warningDialog",NULL,0);
	XtVaSetValues(displayInfo->warningDialog,XmNdialogStyle,XmDIALOG_APPLICATION_MODAL,NULL);
	XtVaSetValues(XtParent(displayInfo->warningDialog),XmNtitle,"Warning !",NULL);
	XtAddCallback(displayInfo->warningDialog,XmNokCallback,warningDialogCb,displayInfo);
	XtAddCallback(displayInfo->warningDialog,XmNcancelCallback,warningDialogCb,displayInfo);
	XtAddCallback(displayInfo->warningDialog,XmNhelpCallback,warningDialogCb,displayInfo);
    }
    if(message == NULL) return;
    xmString = XmStringCreateLtoR(message,XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(displayInfo->warningDialog,XmNmessageString,xmString,NULL);
    XmStringFree(xmString);
    if(okBtnLabel) {
	xmString = XmStringCreateLocalized(okBtnLabel);
	XtVaSetValues(displayInfo->warningDialog,XmNokLabelString,xmString,NULL);
	XmStringFree(xmString);
	XtManageChild(XmMessageBoxGetChild(displayInfo->warningDialog,XmDIALOG_OK_BUTTON));
    } else {
	XtUnmanageChild(XmMessageBoxGetChild(displayInfo->warningDialog,XmDIALOG_OK_BUTTON));
    }
    if(cancelBtnLabel) {
	xmString = XmStringCreateLocalized(cancelBtnLabel);
	XtVaSetValues(displayInfo->warningDialog,XmNcancelLabelString,xmString,NULL);
	XmStringFree(xmString);
	XtManageChild(XmMessageBoxGetChild(displayInfo->warningDialog,XmDIALOG_CANCEL_BUTTON));
    } else {
	XtUnmanageChild(XmMessageBoxGetChild(displayInfo->warningDialog,XmDIALOG_CANCEL_BUTTON));
    }
    if(helpBtnLabel) {
	xmString = XmStringCreateLocalized(helpBtnLabel);
	XtVaSetValues(displayInfo->warningDialog,XmNhelpLabelString,xmString,NULL);
	XmStringFree(xmString);
	XtManageChild(XmMessageBoxGetChild(displayInfo->warningDialog,XmDIALOG_HELP_BUTTON));
    } else {
	XtUnmanageChild(XmMessageBoxGetChild(displayInfo->warningDialog,XmDIALOG_HELP_BUTTON));
    }
    displayInfo->warningDialogAnswer = 0;
    XtManageChild(displayInfo->warningDialog);
    XSync(display,FALSE);
  /* Force Modal (blocking dialog) */
    XtAddGrab(XtParent(displayInfo->warningDialog),True,False);
    XmUpdateDisplay(XtParent(displayInfo->warningDialog));
    while(!displayInfo->warningDialogAnswer || XtAppPending(appContext)) {
	XtAppNextEvent(appContext,&event);
	XtDispatchEvent(&event);
    }
    XtRemoveGrab(XtParent(displayInfo->warningDialog));
    XtUnmanageChild(displayInfo->warningDialog);
}

#ifdef __TED__
void GetWorkSpaceList(Widget w) {
    Atom *paWs;
    char *pchWs;
    DtWsmWorkspaceInfo *pWsInfo;
    unsigned long numWorkspaces;

    if(DtWsmGetWorkspaceList(XtDisplay(w),
      XRootWindowOfScreen(XtScreen(w)),
      &paWs, (int *)&numWorkspaces) == Success)
	{
	    int i;
	    for(i=0; i<numWorkspaces; i++) {
		DtWsmGetWorkspaceInfo(XtDisplay(w),
		  XRootWindowOfScreen(XtScreen(w)),
		  paWs[i],
		  &pWsInfo);
		pchWs = (char *) XmGetAtomName (XtDisplay(w),
		  pWsInfo->workspace);
		print ("workspace %d : %s\n",pchWs);
	    }
	}
}
#endif

/*** DlList routines ***/

/* Makes a new, empty list */
DlList *createDlList() {
    DlList *dlList = malloc(sizeof(DlList));
    if(dlList) {
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
    if(l->tail == l->head) {
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
    if(l->tail == p1) {
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
    if(IsEmpty(pSrc)) return;
    FirstDlElement(pSrc)->prev = p;
    LastDlElement(pSrc)->next = p->next;
    if(pDest->tail == p) {
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
    if(pSrc->count <= 0) return;
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
    if(l->tail == p) {
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
    print("Number of Elements = %d\n",l->count);
    p = FirstDlElement(l);
    while(p) {
	if(p->type == DL_Element) {
	    print("%03d (%s) [%x] x=%d y=%d width=%u height=%u\n",i++,
	      elementType(p->structure.element->type),
	      p->structure.element->structure.composite,
	      p->structure.element->structure.composite->object.x,
	      p->structure.element->structure.composite->object.y,
	      (int)p->structure.element->structure.composite->object.width,
	      (int)p->structure.element->structure.composite->object.height);
	} else {
	    print("%03d %s [%x] x=%d y=%d width=%u height=%u\n",i++,
	      elementType(p->type),
	      p->structure.composite,
	      p->structure.composite->object.x,
	      p->structure.composite->object.y,
	      (int)p->structure.composite->object.width,
	      (int)p->structure.composite->object.height);
	}
	p = p->next;
    }
    return;
}

/* Prints the elements in a list */
static void medmPrintfDlElementList(DlList *l, char *text)
{
    DlElement *p = 0;
    DlObject *pO;
    int i = 0;
    medmPostMsg(1,"%sNumber of Elements=%d\n",text,l->count);
    p = FirstDlElement(l);
    while(p) {
	if(p->type == DL_Element) {
	    pO = &(p->structure.element->structure.composite->object);
	    medmPrintf(0,"%03d (%s) x1=%d x2=%d width=%d y1=%d y2=%d height=%d\n",++i,
	      elementType(p->structure.element->type),
	      pO->x, pO->x + (int)pO->width, (int)pO->width,
	      pO->y, pO->y + (int)pO->height, (int)pO->height);
	} else {
	    pO = &(p->structure.composite->object);
	    medmPrintf(0,"%03d %s x1=%d x2=%d y1=%d y2=%d\n",++i,
	      elementType(p->type),
	      pO->x, pO->x + (int)pO->width, (int)pO->width,
	      pO->y, pO->y + (int)pO->height, (int)pO->height);
	}
	p = p->next;
    }
    return;
}

/*** Undo Routines ***/

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
    if(!undoInfo->dlElementList) {
	medmPrintf(1,"\ncreateUndoInfo: Cannot create element list\n");
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
    if(!IsEmpty(undoInfo->dlElementList)) {
	clearDlDisplayList(displayInfo, undoInfo->dlElementList);
    }
    if(undoInfo->dlElementList) {
	free((char *)undoInfo->dlElementList);
	undoInfo->dlElementList = NULL;
    }
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
    print("\nSAVE\n");
    print("\n[saveUndoInfo: displayInfo->dlElementList(Before):\n");
    dumpDlElementList(displayInfo->dlElementList);
    print("\n[saveUndoInfo: undoInfo->dlElementList(Before):\n");
    dumpDlElementList(undoInfo->dlElementList);
#endif

    if(!IsEmpty(undoInfo->dlElementList)) {
	clearDlDisplayList(displayInfo, undoInfo->dlElementList);
    }
    if(IsEmpty(displayInfo->dlElementList)) return;

    dlElement = FirstDlElement(displayInfo->dlElementList);
    while(dlElement) {
	DlElement *element = dlElement;
	if(element->type != DL_Display) {
	    DlElement *pE = element->run->create(element);
	    appendDlElement(undoInfo->dlElementList,pE);
	}
	dlElement = dlElement->next;
    }

#if DEBUG_UNDO
    print("\n[saveUndoInfo: displayInfo->dlElementList(After):\n");
    dumpDlElementList(displayInfo->dlElementList);
    print("\n[saveUndoInfo: undoInfo->dlElementList(After):\n");
    dumpDlElementList(undoInfo->dlElementList);
    print("\n");
#endif

    undoInfo->grid.gridSpacing = displayInfo->grid->gridSpacing;
    undoInfo->grid.gridOn = displayInfo->grid->gridOn;
    undoInfo->grid.snapToGrid = displayInfo->grid->snapToGrid;
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
	    medmPostMsg(1,"restoreUndoInfo: Cannot create temporary element list\n");
	    return;
	}
    }

  /* Unselect any selected elements since we aren't keeping track of them */
    unselectElementsInDisplay();

#if DEBUG_UNDO
    print("\nRESTORE\n");
    print("\n[restoreUndoInfo: displayInfo->dlElementList(Before):\n");
    dumpDlElementList(displayInfo->dlElementList);
    print("\n[restoreUndoInfo: undoInfo->dlElementList(Before):\n");
    dumpDlElementList(undoInfo->dlElementList);
    print("\n[restoreUndoInfo: tmpDlElementList(Before):\n");
    dumpDlElementList(tmpDlElementList);
#endif

  /* Save current list in temporary list */
    if(!IsEmpty(tmpDlElementList)) {
	clearDlDisplayList(NULL, tmpDlElementList);
    }
    if(!IsEmpty(displayInfo->dlElementList)) {
	dlElement = FirstDlElement(displayInfo->dlElementList);
	while(dlElement) {
	    DlElement *element = dlElement;
	    if(element->type != DL_Display) {
		DlElement *pE = element->run->create(element);
		appendDlElement(tmpDlElementList,pE);
	    }
	    dlElement = dlElement->next;
	}
    }
#if DEBUG_UNDO
    print("\n[restoreUndoInfo: tmpDlElementList(1):\n");
    dumpDlElementList(tmpDlElementList);
#endif

  /* Copy undo list to current list */
    if(!IsEmpty(displayInfo->dlElementList)) {
	removeDlDisplayListElementsExceptDisplay(displayInfo,
	  displayInfo->dlElementList);
    }
    if(!IsEmpty(undoInfo->dlElementList)) {
	dlElement = FirstDlElement(undoInfo->dlElementList);
	while(dlElement) {
	    DlElement *element = dlElement;
	    if(element->type != DL_Display) {
		DlElement *pE = element->run->create(element);
		if(pE->run->execute) (pE->run->execute)(displayInfo, pE);
		appendDlElement(displayInfo->dlElementList,pE);
	    }
	    dlElement = dlElement->next;
	}
    }
#if DEBUG_UNDO
    print("\n[restoreUndoInfo: displayInfo->dlElementList(2):\n");
    dumpDlElementList(displayInfo->dlElementList);
#endif

  /* Copy temporary list to undo list */
    if(!IsEmpty(undoInfo->dlElementList)) {
	clearDlDisplayList(displayInfo, undoInfo->dlElementList);
    }
    if(!IsEmpty(tmpDlElementList)) {
	dlElement = FirstDlElement(tmpDlElementList);
	while(dlElement) {
	    DlElement *element = dlElement;
	    if(element->type != DL_Display) {
		DlElement *pE = element->run->create(element);
		appendDlElement(undoInfo->dlElementList,pE);
	    }
	    dlElement = dlElement->next;
	}
    }
#if DEBUG_UNDO
    print("\n[restoreUndoInfo: undoInfo->dlElementList(3):\n");
    dumpDlElementList(undoInfo->dlElementList);
#endif

#if DEBUG_UNDO
    print("\n[restoreUndoInfo: displayInfo->dlElementList(After):\n");
    dumpDlElementList(displayInfo->dlElementList);
    print("\n[restoreUndoInfo: undoInfo->dlElementList(After):\n");
    dumpDlElementList(undoInfo->dlElementList);
    print("\n[restoreUndoInfo: tmpDlElementList(After):\n");
    dumpDlElementList(tmpDlElementList);
#endif

  /* Clear the temporary element list to be neat */
    clearDlDisplayList(NULL, tmpDlElementList);

  /* Do direct copies */
    displayInfo->grid->gridSpacing = undoInfo->grid.gridSpacing;
    displayInfo->grid->gridOn = undoInfo->grid.gridOn;
    displayInfo->grid->snapToGrid = undoInfo->grid.snapToGrid;
    displayInfo->drawingAreaBackgroundColor = undoInfo->drawingAreaBackgroundColor;
    displayInfo->drawingAreaForegroundColor = undoInfo->drawingAreaForegroundColor;

  /* Insure that this is the currentDisplayInfo and refresh */
    currentDisplayInfo = displayInfo;     /* Shouldn't be necessary */
    refreshDisplay(displayInfo);
#endif
}

/*
 *  Updates the display object values from the current positions of the display
 *    shell windows since that isn't done when the user moves them */
void updateAllDisplayPositions()
{
    DisplayInfo *displayInfo;
    DlElement *pE;
    Position x, y;
    Arg args[2];
    int nargs;

    displayInfo = displayInfoListHead->next;

  /* Traverse the displayInfo list */
    while(displayInfo != NULL) {
      /* Get the current shell coordinates */

	nargs=0;
	XtSetArg(args[nargs],XmNx,&x); nargs++;
	XtSetArg(args[nargs],XmNy,&y); nargs++;
	XtGetValues(displayInfo->shell,args,nargs);

      /* The display is the first element */
	pE = FirstDlElement(displayInfo->dlElementList);
      /* Set the object values of the display from the shell */
	pE->structure.display->object.x = x;
	pE->structure.display->object.y = y;

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

void parseAndExecCommand(DisplayInfo *displayInfo, char * cmd)
{
    char *name, *title;
    char command[1024];     /* Danger: Fixed length */
    Record **records;
    int i, j, ic, len, clen, count;
    DlElement *pE;

#ifdef VMS
#include <descrip.h>
#include <clidef.h>
    int status,spawn_sts;
    int spawnFlags=CLI$M_NOWAIT;
    struct dsc$descriptor cmdDesc;
#endif

  /* Parse the command */
    clen = strlen(cmd);
    for(i=0, ic=0; i < clen; i++) {
	if(ic >= 1024) {
    return;
	}
	if(cmd[i] != '&') {
	    *(command+ic) = *(cmd+i); ic++;
	} else {
	    switch(cmd[i+1]) {
	    case 'P':
	      /* Get the names */
		records = getPvInfoFromDisplay(displayInfo, &count, &pE);
		if(!records) return;   /* (Error messages have been sent) */

	      /* Insert the names */
		for(j=0; j < count; j++) {
		    if(!records[j] || !records[j]->name) continue;
		    name = records[j]->name;
#if DEBUG_COMMAND
		    print("%2d |%s|\n",j,name);
#endif
		    len = strlen(name);
		    if(ic + len >= 1024) {
			medmPostMsg(1,"parseAndExecCommand: Command is too long\n");
			free(records);
			return;
		    }
		    strcpy(command+ic,name);
		    ic+=len;
		  /* Put in a space if required */
		    if(j < count-1) {
			if(ic + 1 >= 1024) {
			    medmPostMsg(1,"parseAndExecCommand: Command is too long\n");
			    free(records);
			    return;
			}
			strcpy(command+ic," ");
			ic++;
		    }
		}
		free(records);
		i++;
		break;
	    case 'A':
		name = displayInfo->dlFile->name;
		len = strlen(name);
		if(ic + len >= 1024) {
		    medmPostMsg(1,"parseAndExecCommand: Command is too long\n");
		    return;
		}
		strcpy(command+ic,name);
		i++; ic+=len;
		break;
	    case 'T':
		title = name = displayInfo->dlFile->name;
		while(*name != '\0')
		  if(*name++ == MEDM_DIR_DELIMITER_CHAR) title = name;
		len = strlen(title);
		if(ic + len >= 1024) {
		    medmPostMsg(1,"parseAndExecCommand: Command is too long\n");
		    return;
		}
		strcpy(command+ic,title);
		i++; ic+=len;
		break;
	    case '?':
	      /* Create shell command prompt dialog if necessary */
		if(displayInfo->shellCommandPromptD == (Widget)NULL) {
		    displayInfo->shellCommandPromptD = createShellCommandPromptD(
		      displayInfo->shell);
		}
		strcpy(command+ic,"&");
	      /* Set the command in the dialog */
		{
		    XmString xmString;

		    xmString = XmStringCreateLocalized(command);
		    XtVaSetValues(displayInfo->shellCommandPromptD,XmNtextString,
		      xmString,NULL);
		    XmStringFree(xmString);

		  /* Popup the prompt dialog, callback will do the rest */
		    XtManageChild(displayInfo->shellCommandPromptD);
		    return;
		}
	    default:
		*(command+ic) = *(cmd+i); ic++;
		break;
	    }
	}
    }
    command[ic]='\0';
#if DEBUG_COMMAND
    if(command && *command) print("\nparseAndExecCommand: %s\n",command);
#endif
    if(command && *command)
#ifndef VMS
    /* KE: This blocks unless the command includes & (on UNIX) */
    /* It should probably be fixed for WIN32 */
      system(command);
#else
  /* ACM: This does not block the whole application, but spawns a command */
    cmdDesc.dsc$w_length  = strlen(command);
    cmdDesc.dsc$b_dtype   = DSC$K_DTYPE_T;
    cmdDesc.dsc$b_class   = DSC$K_CLASS_S;
    cmdDesc.dsc$a_pointer = command;
    spawn_sts = lib$spawn(&cmdDesc,0,0,&spawnFlags,0,0, &status,0,0,0,0,0);
    if(spawn_sts != 1) printf("statuss %d %d\n",spawn_sts, status);
#endif
#if 0
    XBell(display,50);
#endif
}

Pixel alarmColor(int type)
{

    if(type >= 0 &&  type < ALARM_MAX) {
	return alarmColorPixel[type];
    } else {
      /* In case it is not defined properly */
	return alarmColorPixel[ALARM_MAX-1];
    }
}

/* General purpose output routine
 * Works with both UNIX and WIN32
 * Uses sprintf to avoid problem with lprintf not handling %f, etc.
 *   (Exceed 5 only)
 * Use for debugging */

void print(const char *fmt, ...)
{
    va_list vargs;
    static char lstring[1024];  /* DANGER: Fixed buffer size */

    va_start(vargs,fmt);
    vsprintf(lstring,fmt,vargs);
    va_end(vargs);

    if(lstring[0] != '\0') {
#ifdef WIN32
	lprintf("%s",lstring);
#else
	printf("%s",lstring);
#endif
    }
}

/* Returns False if any valid record is not connected.  Otherwise
   returns True.  Doesn't check for the case that all records are
   NULL, which should not be the case. */
Boolean isConnected(Record **records)
{
    int i;
    Boolean connected = True;

    if(!records) return False;

    for(i=0; i < MAX_CALC_RECORDS; i++) {
	if(records[i] && !records[i]->connected) {
	    connected = False;
	    break;
	}
    }

    return connected;
}

/* Returns True if vis = V_STATIC and, if includeColor is True, also
   clr = Static.  That is, the object has an updateTask but does not
   need a PV. */
Boolean isStaticDynamic(DlDynamicAttribute *dynAttr, Boolean includeColor)
{
    if(dynAttr) {
	if(dynAttr->vis == V_STATIC) {
	    if(includeColor) {
		if(dynAttr->clr == STATIC) {
		    return True;
		}
	    } else {
		return True;
	    }
	}
    }
    return False;
}

/*** CALC routines ***/

Boolean calcVisibility(DlDynamicAttribute *attr, Record **records)
{
    int i;

#if DEBUG_VISIBILITY
	printf("calcVisibility: attr->vis=%d validCalc: %s\n",
	  attr->vis,attr->validCalc?"True":"False");
#endif
  /* Determine whether to draw or not */
    switch(attr->vis) {
    case V_STATIC:
	return True;
    case IF_NOT_ZERO:
	return (records[0]->value != 0.0?True:False);
    case IF_ZERO:
	return (records[0]->value == 0.0?True:False);
    case V_CALC:
#if DEBUG_VISIBILITY
	printf("calcVisibility: validCalc: %s\n",
	  attr->validCalc?"True":"False");
#endif
      /* Determine the result of the calculation */
	if(attr->validCalc) {
	    Record *pr = records[0];
	    double valueArray[MAX_CALC_INPUTS];
	    double result;
	    long status;

	  /* Fill in the input array */
	    for(i=0; i < MAX_CALC_RECORDS; i++) {
		if(*attr->chan[i] && records[i]->connected) {
		    valueArray[i] = records[i]->value;
		} else {
		    valueArray[i] = 0.0;
		}
	    }
	    valueArray[4] = 0.0;              /* E: Reserved */
	    valueArray[5] = 0.0;              /* F: Reserved */
	    valueArray[6] = pr->elementCount; /* G: count */
	    valueArray[7] = pr->hopr;         /* H: hopr */
	    valueArray[8] = pr->status;       /* I: status */
	    valueArray[9] = pr->severity;     /* J: severity */
	    valueArray[10] = pr->precision;   /* K: precision */
	    valueArray[11] = pr->lopr;        /* L: lopr */

	  /* Perform the calculation */
	    status = calcPerform(valueArray, &result, attr->post);
#if DEBUG_VISIBILITY
	    printf(" valueA=%g valueB=%g valueC=%g valueC=%g"
	      " calc=%s result=%g\n",
	      valueArray[0],
	      valueArray[1],
	      valueArray[2],
	      valueArray[3],
	      attr->calc,
	      result);
#endif
	    if(!status) {
	      /* Result is valid */
		return (result?True:False);
	    } else {
	      /* Result is invalid */
		return False;
	    }
	} else {
	  /* Calc expression is invalid */
	    return False;
	}
    default :
	medmPrintf(1, "\ncalcVisibility: Unknown visibility: %d\n"
	  "  Associated channels:\n",
	  attr->vis);
	for(i=0; i < MAX_CALC_RECORDS; i++) {
	    medmPrintf(1, "    Channel %c: %s\n",
	      'A'+i, *attr->chan[i]?attr->chan[i]:"None");
	}
	return False;
    }
}

void calcPostfix(DlDynamicAttribute *attr)
{
    if(*attr->chan[0] && attr->vis == V_CALC && *attr->calc){
	long status;
	short errnum;

	status=postfix(attr->calc, attr->post, &errnum);
	if(status) {
	    medmPostMsg(1,"calcPostFix:\n"
	      "  Invalid calc expression [error %d]: %s\n",
	      errnum, attr->calc);
	    *attr->post = '\0';
	    attr->validCalc = False;
	} else {
	    attr->validCalc = True;
	}
    } else {
      /* If VisibilityMode = calc, print an error message */
	if(attr->vis == V_CALC){
	    int i;

	    medmPostMsg(1, "calcPostFix: "
	      "Cannot calculate postfix expression:\n");
	    medmPrintf(1, "    calc: %s\n",
	      *attr->calc?attr->calc:"None");
	    for(i=0; i < MAX_CALC_RECORDS; i++) {
		medmPrintf(1, "    Channel %c: %s\n",
		  'A'+i, *attr->chan[i]?attr->chan[i]:"None");
	    }
	}
	*attr->post = '\0';
	attr->validCalc = False;
    }
}

/* Set when we want an update value callback for objects with a
 * Dynamic Attribute */
void setDynamicAttrMonitorFlags(DlDynamicAttribute *attr, Record **records)
{
    int i;

    for(i=0; i < MAX_CALC_RECORDS; i++) {
	Record *pR = records[i];

      /* Skip over NULL records */
	if(!pR) continue;

	if(i == 0) {
	  /* The main record */
	  /* Set all requirements to zero */
	    pR->monitorValueChanged = False;
	    pR->monitorStatusChanged = False;
	    pR->monitorSeverityChanged = False;
	    pR->monitorZeroAndNoneZeroTransition = False;
	  /* Set the minimum requirement for ColorMode */
	    switch (attr->clr) {
	    case STATIC:
	      /* Even though it is static, we need to monitor the
                 value change to be able to redraw it when hiding and
                 unhiding */
		pR->monitorValueChanged = True;
		break;
	    case ALARM:
		pR->monitorSeverityChanged = True;
		break;
	    case DISCRETE:
		break;
	    }
	  /* Set the minimum requirement for each VisibilityMode */
	    switch(attr->vis) {
	    case V_STATIC:
	      /* Even though it is static, we need to monitor the
                 value change to be able to redraw it when hiding and
                 unhiding */
		pR->monitorValueChanged = True;
		break;
	    case IF_NOT_ZERO:
	    case IF_ZERO:
		pR->monitorZeroAndNoneZeroTransition = True;
		break;
	    case V_CALC:
		if(attr->validCalc) {
		    pR->monitorValueChanged = True;
		    if(calcUsesStatus(attr->calc)) {
			pR->monitorStatusChanged = True;
		    }
		    if(calcUsesSeverity(attr->calc)) {
			pR->monitorSeverityChanged = True;
		    }
		}
		break;
	    }
	} else {
	  /* Not the main record */
	  /* Set all requirements to zero */
	    pR->monitorValueChanged = False;
	    pR->monitorSeverityChanged = False;
	    pR->monitorZeroAndNoneZeroTransition = False;
	  /* Set the minimum requirement for each VisibilityMode */
	    if(attr->vis == V_CALC) {
		pR->monitorValueChanged = True;
	    }
	}
    }
}

/* Determines if a calc expression uses status (I) */
int calcUsesStatus(char *calc)
{
    char *cur;
    char *prev;

    if(!calc || !*calc) return 0;
    cur=calc;
    while(*cur) {
	if(*cur == 'i' || *cur == 'I') {
	  /* See if it is the first character */
	    if(cur == calc) return 1;
	  /* See if it is not preceeded by a letter as in sin, asin,
             sinh, min, ceil, nint, pi*/
	    prev=cur-1;
	    if(!((*prev >= 'A' && *prev <= 'Z') ||
	      (*prev >= 'a' && *prev <= 'z'))) {
		return 1;
	    }
	}
	cur++;
    }
    return 0;
}

/* Determines if a calc expression uses severity (J) */
int calcUsesSeverity(char *calc)
{
    char *cur;
    char *prev;

    if(!calc || !*calc) return 0;
    cur=calc;
    while(*cur) {
	if(*cur == 'j' || *cur == 'J') {
	  /* See if it is the first character */
	    if(cur == calc) return 1;
	  /* See if it is not preceeded by a letter even though there
             are no current operands that contain j */
	    prev=cur-1;
	    if(!((*prev >= 'A' && *prev <= 'Z') ||
	      (*prev >= 'a' && *prev <= 'z'))) {
		return 1;
	    }
	}
	cur++;
    }
    return 0;
}

/* Returns a pointer to the short part of the ADL file name */
char *shortName(char *filename)
{
    char *shortName = NULL, *ptr;

    if(filename) {
	ptr = shortName = filename;
	while(*ptr != '\0')
	  if(*ptr++ == MEDM_DIR_DELIMITER_CHAR) shortName = ptr;
    }
    return shortName;
}

/*** Debugging routines ***/

static struct timeval timerTime0={0,0};

void resetTimer(void)
{
    gettimeofday(&timerTime0,NULL);
}

double getTimerDouble(void)
{
    double t;
    struct timeval now;
    gettimeofday(&now,NULL);
    t=(double)now.tv_sec-(double)timerTime0.tv_sec+
      1.e-6*((double)now.tv_usec-(double)timerTime0.tv_usec);
    return t;
}

struct timeval getTimerTimeval(void)
{
    struct timeval now;
    gettimeofday(&now,NULL);
    now.tv_sec-=timerTime0.tv_sec;
    now.tv_usec-=timerTime0.tv_usec;
    return now;
}

void printEventMasks(Display *display, Window win, char *string)
{
    XWindowAttributes attr;
    int i;
    long mask;
  /* These values are from X11/X.h */
    static char *maskNames[]={
	"KeyPressMask",
	"KeyReleaseMask",
	"ButtonPressMask",
	"ButtonReleaseMask",
	"EnterWindowMask",
	"LeaveWindowMask",
	"PointerMotionMask",
	"PointerMotionHintMask",
	"Button1MotionMask",
	"Button2MotionMask",
	"Button3MotionMask",
	"Button4MotionMask",
	"Button5MotionMask",
	"ButtonMotionMask",
	"KeymapStateMask",
	"ExposureMask",
	"VisibilityChangeMask",
	"StructureNotifyMask",
	"ResizeRedirectMask",
	"SubstructureNotifyMask",
	"SubstructureRedirectMask",
	"FocusChangeMask",
	"PropertyChangeMask",
	"ColormapChangeMask",
	"OwnerGrabButtonMask",
    };

  /* Get the attributes */
    if(win) {
	print("%sMasks for window %x:\n", string, win);
	XGetWindowAttributes(display, win, &attr);
    } else {
	print("%sWindow is NULL (Masks are undefined)\n", string);
	return;
    }

    print("%-27s %-10s %-10s %-10s\n",
      "Mask","all_event","your_event","do_not_propagate");
/*     for(i=0; i < nmasks; i++) { */
    for(i=2; i < 4; i++) {

	mask=1<<i;
	print("%-27s %-10s %-10s %-10s\n",
	  maskNames[i],
	  (attr.all_event_masks&mask)?"X":" ",
	  (attr.your_event_mask&mask)?"X":" ",
	  (attr.do_not_propagate_mask&mask)?"X":" ");
    }
}

void printWindowAttributes(Display *display, Window win, char *string)
{
    XWindowAttributes attr;
    static char *mapNames[]={
	"IsUnmapped",
	"IsUnviewable",
	"IsViewable",
    };
    int screen=DefaultScreen(display);
    Window root,parent,next,*children=(Window *)0;
    Window rootWindow=RootWindow(display,screen);
    unsigned int nchildren;

  /* Get the attributes */
    if(win) {
	print("%sAttributes for window %x:\n", string, win);
	XGetWindowAttributes(display, win, &attr);
    } else {
	print("%sWindow is NULL (Attributes are undefined)\n", string);
	return;
    }
    print("  x=%d y=%d width=%d height=%d\n",attr.x,attr.y,attr.width,attr.height);
    if(attr.map_state >= 0 && attr.map_state < 3) {
	print("  map_state=%s\n",mapNames[attr.map_state]);
    } else {
	print("  map_state=Invalid Value\n");
    }

  /* Get the tree */
    print("  Window tree:\n");
    parent=(Window)0;
    next=win;
    while(parent != rootWindow) {
	if(!XQueryTree(display,next,&root,&parent,&children,&nchildren)) {
	    print("    [Could not get tree for %x]\n",next);
	} else {
	    print("    window=%8x parent=%8x root=%8x\n",next,parent,root);
	}
	if(children) XFree((void *)children);
	next=parent;
    }
}

char *getEventName(int type)
{
#if LASTEvent != 35
#error getEventType only works for LASTEvent=35 (See X.h)
#endif
  /* These types are from X11/X.h */
    static char *eventNames[LASTEvent+2]={
	"Reserved (0)",
	"Reserved (1)",
	"KeyPress",
	"KeyRelease",
	"ButtonPress",
	"ButtonRelease",
	"MotionNotify",
	"EnterNotify",
	"LeaveNotify",
	"FocusIn",
	"FocusOut",
	"KeymapNotify",
	"Expose",
	"GraphicsExpose",
	"NoExpose",
	"VisibilityNotify",
	"CreateNotify",
	"DestroyNotify",
	"UnmapNotify",
	"MapNotify",
	"MapRequest",
	"ReparentNotify",
	"ConfigureNotify",
	"ConfigureRequest",
	"GravityNotify",
	"ResizeRequest",
	"CirculateNotify",
	"CirculateRequest",
	"PropertyNotify",
	"SelectionClear",
	"SelectionRequest",
	"SelectionNotify",
	"ColormapNotify",
	"ClientMessage",
	"MappingNotify",
	"LASTEvent",
	"Unknown"
    };

    if(type > 2 && type < LASTEvent) {
	return eventNames[type];
    } else {
	return eventNames[LASTEvent+1];
    }
}

void dumpDisplayInfoList(DisplayInfo *head, char *string)
{
    DisplayInfo *displayInfo = head->next;
    int i=0;

    print("\nDisplayList: %s\n",string);
    while(displayInfo) {
	print("%2d %s\n",++i,displayInfo->dlFile->name);
	displayInfo = displayInfo->next;
    }
}

/* Use this routine to display a pixmap in a toplevel shell.  It makes
   a copy of the pixmap, so the original does not have to stay around */
void dumpPixmap(Pixmap pixmap, Dimension width, Dimension height, char *title)
{
    Widget shell, da;
    Pixmap savePixmap;
    GC gc;

  /* Save the pixmap for redraws */
    gc = XCreateGC(display, rootWindow, 0, NULL);
  /* Eliminate events that we do not handle anyway */
    XSetGraphicsExposures(display, gc, False);
    savePixmap = XCreatePixmap(display, RootWindow(display,screenNum),
      width, height, DefaultDepth(display,screenNum));
    XCopyArea(display, pixmap, savePixmap,
      gc, 0, 0, width, height, 0, 0);
    XFreeGC(display, gc);

  /* Make a toplevel shell */
    shell = XtVaCreatePopupShell("pixmapShell",
      topLevelShellWidgetClass, mainShell,
      XmNtitle, title,
      XmNdeleteResponse, XmDO_NOTHING,
      NULL);
    XmAddWMProtocolCallback(shell, WM_DELETE_WINDOW,
      dumpPixmapCb, (XtPointer)savePixmap);

    da = XtVaCreateManagedWidget("pixmapDrawingArea",
      xmDrawingAreaWidgetClass, shell,
      XmNwidth, width,
      XmNheight, height,
      NULL);
    XtAddCallback(da,XmNexposeCallback,dumpPixmapCb,(XtPointer)savePixmap);
    XtAddCallback(da,XmNresizeCallback,dumpPixmapCb,(XtPointer)savePixmap);

    XtPopup(shell, XtGrabNone);
}

static void dumpPixmapCb(Widget w, XtPointer clientData, XtPointer callData)
{
    XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *)callData;

    if (cbs->reason == XmCR_EXPOSE) {
      /* EXPOSE */
	GC gc = XCreateGC(display,rootWindow,0,NULL);
	Pixmap pixmap = (Pixmap)clientData;
	Dimension width, height;

      /* Eliminate events that we do not handle anyway */
	XSetGraphicsExposures(display, gc, False);

	XtVaGetValues(w,
	  XmNwidth, &width,
	  XmNheight, &height,
	  NULL);
#if 0
        XSetForeground(display,gc,WhitePixel(display,screenNum));
	XDrawRectangle(display, XtWindow(w), gc, 0, 0, 10, 10);
#endif
	if(width > 0 && height > 0) {
	    XCopyArea(display, pixmap, XtWindow(w), gc, 0, 0, width, height, 0, 0);
	}
	XFreeGC(display,gc);
    } else if (cbs->reason == XmCR_RESIZE) {
      /* RESIZE */
    } else if (cbs->reason == XmCR_PROTOCOLS) {
      /* WM_CLOSE */
	Pixmap pixmap = (Pixmap)clientData;

	if(pixmap) XFreePixmap(display, pixmap);
	XtPopdown(w);
    }
}

/* XR5 Resource ID patch */
#ifdef USE_XR5_RESOURCEID_PATCH
#undef XCreatePixmap
#undef XFreePixmap
#undef XCreateGC
#undef XFreeGC

/*
     This set of routines enable an X11 program to maintain its own
     list of reusable resource ids.  It uses an undocumented and unsupported
     feature of Xlib, the resource_alloc field of the display structure.  This
     field contains a pointer to the resource_id allocator function to be used.

     The reusable resource id list is implemented as a linked list.

     Ideally, for each resource id which the application frees (pixmaps, GCs,
     windows), it will put the resource id back on the reusable list.  In some
     cases this will be difficult, or impossible.  In the case of deleting a
     window which has subwindows, the application would have to know the window
     id of each subwindow to put them back on the reusable list.  Either that,
     or just accept that in this case there will be a slow leak of the number
     of resource ids available to the program.
 */

typedef struct _reusable_id_entry {
     struct _reusable_id_entry  *next;
     XID                id;
} reusable_id_entry;

static reusable_id_entry *reusable_id_list_head = NULL;
static reusable_id_entry *id_entry;

/*  Application defined resource ID allocator */

static XID xPatchResourceIDAllocator(register Display *dpy)
{
    XID return_id;
    reusable_id_entry *entry_ptr;

#define resource_base  (((_XPrivDisplay)dpy)->private3)
#define resource_mask (((_XPrivDisplay)dpy)->private4)
#define resource_id  (((_XPrivDisplay)dpy)->private5)
#define resource_shift  (((_XPrivDisplay)dpy)->private6)

    /* First, are there any IDs available for re-use? */
    if(reusable_id_list_head) {
       /* Yes, remove one from the list */
       return_id = reusable_id_list_head->id;
       entry_ptr = reusable_id_list_head->next;
       free(reusable_id_list_head);
       reusable_id_list_head = entry_ptr;
       return return_id;
    }
    /* Else use conventional resource id allocation (from _XAllocID) */
    if(resource_id <= resource_mask)
      return (resource_base + (resource_id++ << resource_shift));
    if(resource_id != 0x10000000) {
        medmPrintf(1,"\nxPatchResourceIDAllocator: "
	  "Resource ID allocation space exhausted!\n");
        resource_id = 0x10000000;
    }
#if DEBUG_RESID
    print("xPatchResourceIDAllocator: resID=%x\n",resource_id);
#endif
    return resource_id;
}

static void XPatchAddEntryForReuse(XID id)
{
       id_entry = malloc(sizeof(reusable_id_entry));
       id_entry->next = reusable_id_list_head;
       id_entry->id = id;
       reusable_id_list_head = id_entry;
}

Pixmap XPatchCreatePixmap(Display *dpy, Drawable drawable, unsigned int width,
  unsigned int height, unsigned int depth)
{
    XID (*oldAllocator)(Display *) =
      (((_XPrivDisplay)display)->resource_alloc);
    Pixmap pixmap;

  /* Replace the allocator */
    (((_XPrivDisplay)display)->resource_alloc) = xPatchResourceIDAllocator;

  /* Run the real routine */
    pixmap = XCreatePixmap(dpy, drawable, width, height, depth);

  /* Restore the allocator */
    (((_XPrivDisplay)display)->resource_alloc) = oldAllocator;

#if DEBUG_RESID
    print("XPatchCreatePixmap: pixmap=%x\n",pixmap);
#endif
    return pixmap;
}

GC XPatchCreateGC(Display *dpy, Drawable drawable, unsigned long valueMask,
    XGCValues *gcValues)
{
    XID (*oldAllocator)(Display *) =
      (((_XPrivDisplay)display)->resource_alloc);
    GC gc;

  /* Replace the allocator */
    (((_XPrivDisplay)display)->resource_alloc) = xPatchResourceIDAllocator;

  /* Run the real routine */
    gc = XCreateGC(dpy, drawable, valueMask, gcValues);

  /* Restore the allocator */
    (((_XPrivDisplay)display)->resource_alloc) = oldAllocator;

#if DEBUG_RESID
    print("XPatchCreateGC: gc=%x\n",gc);
#endif
    return gc;
}

int XPatchFreePixmap(Display *dpy, Pixmap pixmap)
{
    int retVal = BadPixmap;

    if(pixmap) {
	retVal = XFreePixmap(display, pixmap);
	XPatchAddEntryForReuse((XID)pixmap);
    }
#if DEBUG_RESID
    print("XPatchFreePixmap: pixmap=%x\n",pixmap);
#endif
    return retVal;
}
int XPatchFreeGC(Display *dpy,  GC gc)
{
    int retVal = BadPixmap;

    if(gc) {
	retVal = XFreeGC(display, gc);
	XPatchAddEntryForReuse((XID)gc);
    }
#if DEBUG_RESID
    print("XPatchFreeGC: gc=%x\n",gc);
#endif
    return retVal;
}

#endif     /* #ifdef USE_XR5_RESOURCEID_PATCH */
