
/************************************************************************
 ****                  Utility Functions                             ****
 ************************************************************************/


#include <string.h>

#include "medm.h"

#define MAX_DIR_LENGTH 512		/* max. length of directory name */


Boolean modalGrab = FALSE;


static void attributeSet(
  DisplayInfo *displayInfo,
  DlElement *elementPtr)
{
  DlElement *dyn, *basic;

  if (elementPtr->type != DL_Composite) {
/*
 * update currentDisplayInfo's DlDynamicAttribute data,
 *  see if one precedes this entry
 */
   dyn = lookupDynamicAttributeElement(elementPtr);

   if (dyn != NULL) {
  /* found a dynamic attribute!  - structure copy */
    currentDisplayInfo->dynamicAttribute = *(dyn->structure.dynamicAttribute);
   } else {
  /* clear some bytes */
    memset((void *)&(currentDisplayInfo->dynamicAttribute),(int)NULL,
	sizeof(DlDynamicAttribute));
   }

/*
 * update currentDisplayInfo's DlAttribute data (loop back arbitrarily far)
 */
   basic = lookupBasicAttributeElement(elementPtr);
   if (basic != NULL) {
  /* found a basic attribute!  - structure copy */
      currentDisplayInfo->attribute = basic->structure.basicAttribute->attr;
   }
  }

}


static void attributeClear(
  DisplayInfo *displayInfo)
{

/*
 * update currentDisplayInfo's DlDynamicAttribute data,
 * (clear some bytes)
 */
    memset((void *)&(currentDisplayInfo->dynamicAttribute),(int)NULL,
	sizeof(DlDynamicAttribute));
}




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
  int n, suffixLength, usedLength, startPos;

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

/* if not in current directory, look in EPICS_DISPLAY_PATH directory */
  if (filePtr == NULL) {
     dir = getenv(DISPLAY_LIST_ENV);
     if (dir != NULL) {
        startPos = 0;
        while (filePtr == NULL &&
		extractStringBetweenColons(dir,dirName,startPos,&startPos)) {
	  strcpy(fullPathName,dirName);
	  strcat(fullPathName,"/");
	  strcat(fullPathName,name);
	  filePtr = fopen(fullPathName,"r");
        }
     }
  }
  return (filePtr);
}


/*
 *  extract strings between colons from input to output
 *    this function works as an iterator...
 */
Boolean extractStringBetweenColons(
  char *input,
  char *output,
  int  startPos,
  int  *endPos)
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
  i++;

  *endPos = i;
  if (j == 0)
     return(False);
  else
     return(True);
}


/*
 * function to determine if the specified monitorData is in the current
 *	monitor list
 */
Boolean isCurrentMonitor(Channel *monitorData)
{
  Channel *data;

  data = channelAccessMonitorListHead->next;
  while (data != NULL) {
     if (data == monitorData) return(True);
     data = data->next;
  }
  return(False);
}



/*
 * function which removes a monitor structure from monitor list and does
 *   appropriate CA cleanup...
 *   but does not free specifics field, since this is usually the
 *   dlBar, dlMeter... pointer which we want to keep around
 */
void dmRemoveMonitorStructureFromMonitorList(
  Channel *pCh)
{
  int i;
  DisplayInfo *displayInfo = pCh->displayInfo;

  if (pCh->destroyChannel)
    pCh->destroyChannel(pCh);

  medmDisconnectChannel(pCh);

  if (pCh->dlAttr != NULL) {
    free ((char *) pCh->dlAttr);
    pCh->dlAttr = NULL;
  }

  /* disable all callbacks */
  pCh->updateChannelCb = NULL;
  pCh->updateGraphicalInfoCb = NULL;
  pCh->destroyChannel = NULL;

  /* remove the this node from the double link list */
  /* and free the memory */
  pCh->prev->next = pCh->next;
  if (pCh->next != NULL) pCh->next->prev = pCh->prev;
  if (channelAccessMonitorListTail == pCh) 
    channelAccessMonitorListTail = channelAccessMonitorListTail->prev;
  if (channelAccessMonitorListTail == channelAccessMonitorListHead)
    channelAccessMonitorListHead->next = NULL;
  free( (char *) pCh);

  return;
}


void dmFreeCompositeAndChildrenFromDisplayList(DlElement *ele)
{
  DlElement *child, *freeElement, *element;

  if (ele == NULL) return;
  if (ele->structure.composite == NULL) return;
  if (ele->structure.composite->dlElementListHead == NULL) return;

  child = ((DlElement *)ele->structure.composite->dlElementListHead)->next;
  while (child != NULL) {

    if (child->type == DL_Composite) {
      dmFreeCompositeAndChildrenFromDisplayList(child);
    }
    free( (char *) child->structure.file);	/* arbitrary wrt type */
    freeElement = child;
    child = child->next;
    free( (char *) freeElement);
  }
  free((char *)ele->structure.file);		/* arbitrary wrt type */
  free((char *)ele);
}


/*
 * clean up the memory-resident display list (if there is one)
 */

void dmRemoveDisplayList(
  DisplayInfo *displayInfo)
{
  DlElement *element, *freeElement;

  if (displayInfo->dlElementListHead != displayInfo->dlElementListTail ) {

/* start at first element past placeholder (list head) */
      element = ((DlElement *)displayInfo->dlElementListHead)->next;
      do {
	if (element->type == DL_Composite) {
	    dmFreeCompositeAndChildrenFromDisplayList(element);
	    element = element->next;
	} else {
	  if (element->type == DL_Polyline) {
	    free ((char *)element->structure.polyline->points);
	  } else if (element->type == DL_Polygon) {
	    free ((char *)element->structure.polygon->points);
	  }
          free( (char *) element->structure.file);  /* arbitrary wrt type */
	  freeElement = element;
	  element = element->next;
	  free( (char *) freeElement);
	}
      } while (element != NULL);
  }
  displayInfo->dlElementListTail = displayInfo->dlElementListHead;

}     



/*
 * function which cleans up a given displayInfo in the displayInfoList
 * (including the displayInfo's display list if specified)
 */
void dmCleanupDisplayInfo(
  DisplayInfo *displayInfo,
  Boolean cleanupDisplayList)
{
  int i;
  Channel *data, *saveData;
  StripChartList *stripElement, *oldStrip;
  Boolean alreadyFreedUnphysical;
  Widget DA;


/* save off current DA */
  DA = displayInfo->drawingArea;
/* now set to NULL in displayInfo to signify "in cleanup" */
  displayInfo->drawingArea = NULL;

/*
 * remove any strip charts in this display 
 */
  stripElement = displayInfo->stripChartListHead;
  while (stripElement != NULL) {
    stripPause(stripElement->strip);
    stripTerm(stripElement->strip);
    oldStrip = stripElement;
    stripElement = stripElement->next;
    free( (char *) oldStrip);
  }
  displayInfo->stripChartListHead = NULL;
  displayInfo->stripChartListTail = NULL;

/* 
 * as a composite widget, drawingArea is responsible for destroying
 *  it's children
 */
  if (DA != NULL) {
	XtDestroyWidget(DA);
	DA = NULL;
  }

/*
 * close the monitored channels, and free elements from the monitor
 *	list if this is the displayInfo being cleared
 */
  data = channelAccessMonitorListHead->next;
  while (data != NULL) {
    if (data->displayInfo == displayInfo) {
      /* this monitorData belongs to this display */
      saveData = data->next;
      dmRemoveMonitorStructureFromMonitorList(data);
      data = saveData;
    } else {
      data = data->next;
    }
  }



/* force a wait for all outstanding CA event completion */
/* (wanted to do   while (ca_pend_event() != ECA_NORMAL);  but that sits there     forever)
 */
  ca_pend_event(CA_PEND_EVENT_TIME);

/*
 * if cleanupDisplayList == TRUE
 *   then global cleanup ==> delete shell, free memory/structures, etc
 */

  if (cleanupDisplayList) {
    XtDestroyWidget(displayInfo->shell);
    displayInfo->shell = NULL;
    /* remove display list here */
    dmRemoveDisplayList(displayInfo);
  }


/*
 * free other X resources
 */
  if (displayInfo->drawingAreaPixmap != (Pixmap)NULL) {
        XFreePixmap(display,displayInfo->drawingAreaPixmap);
	displayInfo->drawingAreaPixmap = (Pixmap)NULL;
  }
  if (displayInfo->dlColormap != NULL && displayInfo->dlColormapCounter > 0) {
	alreadyFreedUnphysical = False;
	for (i = 0; i < displayInfo->dlColormapCounter; i++) {
	    if (displayInfo->dlColormap[i] != unphysicalPixel) {
		XFreeColors(display,cmap,&(displayInfo->dlColormap[i]),1,0);
	    } else if (!alreadyFreedUnphysical) {
		/* only free "unphysical" pixel once */
		XFreeColors(display,cmap,&(displayInfo->dlColormap[i]),1,0);
		alreadyFreedUnphysical = True;
	    }
	}
        free( (char *) displayInfo->dlColormap);
	displayInfo->dlColormap = NULL;
	displayInfo->dlColormapCounter = 0;
	displayInfo->dlColormapSize = 0;
  }
  if (displayInfo->gc != NULL) {
	XFreeGC(display,displayInfo->gc);
	displayInfo->gc = NULL;
  }
  if (displayInfo->pixmapGC != NULL) {
	XFreeGC(display,displayInfo->pixmapGC);
	displayInfo->pixmapGC = NULL;
  }
  displayInfo->drawingAreaBackgroundColor = 0;
  displayInfo->drawingAreaForegroundColor = 0;
  displayInfo->childCount = 0;
  displayInfo->otherChildCount = 0;


}




/*
 * function to remove a specified DisplayInfo  from displayInfoList
 *   this includes a full cleanup of associated resources and displayList
 */
void dmRemoveDisplayInfo(
  DisplayInfo *displayInfo)
{
  displayInfo->prev->next = displayInfo->next;
   if (displayInfo->next != NULL)
	displayInfo->next->prev = displayInfo->prev;
  if (displayInfoListTail == displayInfo)
	displayInfoListTail = displayInfoListTail->prev;
  if (displayInfoListTail == displayInfoListHead )
	displayInfoListHead->next = NULL;
/* cleaup resources and free display list */
  dmCleanupDisplayInfo(displayInfo,True);
  freeNameValueTable(displayInfo->nameValueTable,displayInfo->numNameValues);
  free ( (char *) displayInfo->dlElementListHead);
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
 * traverse (execute) specified displayInfo's display list
 */
void dmTraverseDisplayList(
  DisplayInfo *displayInfo)
{
  DlElement *element;

/* traverse the display list */
  element = ((DlElement *)displayInfo->dlElementListHead)->next;
  while (element != NULL) {
/* type in union is unimportant: just trying to get to element structure */
/* third argument is for statics acting as dynamics (for forced display) */
	(*element->dmExecute)(displayInfo,element->structure.file,FALSE);
	element = element->next;
  }

/* change drawingArea's cursor to the appropriate cursor */
    XDefineCursor(display,XtWindow(displayInfo->drawingArea),
      (currentActionType == SELECT_ACTION ? rubberbandCursor : crosshairCursor));

/*
 * flush CA send buffer and continue, since connection event handler used
 *	also flush X buffer
 */
  XFlush(display);
  ca_pend_event(CA_PEND_EVENT_TIME);

}


/*
 * traverse (execute) all displayInfos and display lists
 */
void dmTraverseAllDisplayLists()
{
  DisplayInfo *displayInfo;
  DlElement *element;


  displayInfo = displayInfoListHead->next;

  while (displayInfo != NULL) {

/* traverse the display list */
    element = ((DlElement *)displayInfo->dlElementListHead)->next;
    while (element != NULL) {
/* type in union is unimportant: just trying to get to element structure */
/* third argument is for statics acting as dynamics (for forced display) */
	(*element->dmExecute)(displayInfo,element->structure.file,FALSE);
	element = element->next;
    }

/* change drawingArea's cursor to the appropriate cursor */
    XDefineCursor(display,XtWindow(displayInfo->drawingArea),
      (currentActionType == SELECT_ACTION ? rubberbandCursor : crosshairCursor));

    displayInfo = displayInfo->next;
  }



/*
 * flush CA send buffer and continue, since connection event handler used
 *	also flush X buffer
 */
  XFlush(display);
  ca_pend_event(CA_PEND_EVENT_TIME);	/* don't allow early returns */

}



/*
 * traverse (execute) composite - don't execute children wi/widgets
 */
void traverseCompositeNonWidgets(DisplayInfo *displayInfo,
	DlElement* composite)
{
  DlElement *child;

  child = ((DlElement*)composite->structure.composite->dlElementListHead)->next;
  while (child != NULL) {
     if (!ELEMENT_HAS_WIDGET(child->type))  {
	   if (child->type == DL_Composite) {
	/* special handling for composite - don't execute children wi/widgets */
	     traverseCompositeNonWidgets(displayInfo,child);
	   } else {
	     (*child->dmExecute)(displayInfo,child->structure.file,FALSE);
	   }
     }
     child = child->next;
  }

}


/*
 * traverse (execute) specified displayInfo's display list non-widget elements
 */
void dmTraverseNonWidgetsInDisplayList(
  DisplayInfo *displayInfo)
{
  DlElement *element;
  Dimension width,height;

  if (displayInfo == NULL) return;

  XSetForeground(display,displayInfo->pixmapGC,
	displayInfo->dlColormap[displayInfo->drawingAreaBackgroundColor]);
  XtVaGetValues(displayInfo->drawingArea,
		XmNwidth,&width,XmNheight,&height,NULL);
  XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
		0, 0, (unsigned int)width,(unsigned int)height);

/* traverse the display list */
  element = ((DlElement *)displayInfo->dlElementListHead)->next;
  while (element != NULL) {
/* type in union is unimportant: just trying to get to element structure */
/* third argument is for statics acting as dynamics (for forced display) */

/* don't want or need to execute DL_File or DL_Colormap (especially) */
	if (!ELEMENT_HAS_WIDGET(element->type) &&
		element->type != DL_File && element->type != DL_Colormap) {
	   if (element->type == DL_Composite) {
	/* special handling for composite - don't execute children wi/widgets */
	     traverseCompositeNonWidgets(displayInfo,element);
	   } else {
	     (*element->dmExecute)(displayInfo,element->structure.file,FALSE);
	   }
	}
	element = element->next;
  }

/* since the execute traversal copies to the pixmap, now udpate the window */
   XCopyArea(display,displayInfo->drawingAreaPixmap,
		XtWindow(displayInfo->drawingArea),
		displayInfo->pixmapGC, 0, 0, (unsigned int)width,
		(unsigned int)height, 0, 0);

/* change drawingArea's cursor to the appropriate cursor */
    XDefineCursor(display,XtWindow(displayInfo->drawingArea),
      (currentActionType == SELECT_ACTION ? rubberbandCursor : crosshairCursor));

}



/*
 * function to return the best fitting font for the field and string
 *   if textWidthFlag = TRUE:  use the text string and find width also
 *   if textWidthFlag = FALSE: ignore text,w fields and
 *	make judgment based on height info only & return
 *	width of largest character as *usedW
 */
int dmGetBestFontWithInfo(
  XFontStruct **fontTable,
  int nFonts,
  char *text,
  int h,
  int w, 
  int *usedH, 
  int *usedW,
  Boolean textWidthFlag)
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

#ifdef DEBUG
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
	fprintf(stderr,"Warning: %s\n", message);
    }
  
  return(0);
}


/*
 * function to march up widget hierarchy to retrieve top shell, and
 *  then run over displayInfoList and return the corresponding DisplayInfo *
 */
DisplayInfo *dmGetDisplayInfoFromWidget(
  Widget widget)
{
  Widget w;
  DisplayInfo *displayInfo;

  w = widget;
  while (w != (Widget)NULL) {
    if (XtClass(w) == topLevelShellWidgetClass) break;
    w = XtParent(w);
  }

  if (w != (Widget)NULL) {
    displayInfo = displayInfoListHead->next;
    while (displayInfo != NULL) {
      if (displayInfo->shell == w)
	  return (displayInfo);
      displayInfo = displayInfo->next;
    }
  }
  return ((DisplayInfo *)NULL);
}





/*
 * write specified displayInfo's display list
 */
void dmWriteDisplayList(
  DisplayInfo *displayInfo,
  FILE *stream)
{
  DlElement *element, *cmapElement;

/* traverse the display list */
  element = ((DlElement *)displayInfo->dlElementListHead)->next;

  while (element != NULL) {
/* type in union is unimportant: just trying to get to element structure */
/* third argument is for indention level */

/* note: don't put out multiple consecutive basic attributes, only last one */
     while (element->type == DL_BasicAttribute && element->next != NULL) {
	if (element->next->type == DL_BasicAttribute)
	  element = element->next;
	else
	  break;		/* break out of while */
     }
/* note: don't put out dynamic if it is followed by non-"static"            */
/*  this prevents dyn-dyn-..., or dyn-basic-..., or dyn-controller-,    etc */
     while (element->type == DL_DynamicAttribute && element->next != NULL) {
	if (!ELEMENT_IS_STATIC(element->next->type))
	  element = element->next;
	else
	  break;		/* break out of while */
     }

/*
 * a little inefficient to have this test which is always false except once...
 *  but need to guard against display with external colormap which is now
 *  internal
 */
     if (element->type == DL_Display) {
        (*element->dmWrite)(stream,element->structure.display,0);
/* go find colormap element, in case it's not immediately following */
	 cmapElement = element->next;
	 while (cmapElement != NULL && cmapElement->type != DL_Colormap) {
	    cmapElement = cmapElement->next;
	 }
	 if (cmapElement != NULL)
	    writeDlColormap(stream,&defaultDlColormap,0);
     } else {
       if (element->type != DL_Colormap)
		(*element->dmWrite)(stream,element->structure.file,0);
     }

     element = element->next;
  }
  fprintf(stream,"\n");

}




/*
 * set the filename for the display in specified displayInfo's display list
 */
void dmSetDisplayFileName(
  DisplayInfo *displayInfo,
  char *filename)
{
  DlElement *element;
  DlFile *dlFile;

/* look for the DlFile element in the display list */
  element = ((DlElement *)displayInfo->dlElementListHead)->next;
  while (element != NULL) {
      if (element->type == DL_File) {
	strcpy(element->structure.file->name,filename);
	return;
      } else {
	element = element->next;
      }
  }
}


/*
 * get the filename for the display in specified displayInfo's display list
 */
char *dmGetDisplayFileName(
  DisplayInfo *displayInfo)
{
  DlElement *element;
  DlFile *dlFile;

/* look for the DlFile element in the display list */
  element = ((DlElement *)displayInfo->dlElementListHead)->next;
  while (element != NULL) {
      if (element->type == DL_File) {
	return(element->structure.file->name);
      } else {
	element = element->next;
      }
  }
}


void medmSetDisplayTitle(DisplayInfo *displayInfo)
{
  DlElement *element;
  DlFile *dlFile;
  char str[MAX_FILE_CHARS+10];

  /* look for the DlFile element in the display list */
  element = ((DlElement *)displayInfo->dlElementListHead)->next;
  while (element != NULL) {
    if (element->type == DL_File) {
      char *tmp, *tmp1;
      tmp = tmp1 = element->structure.file->name;
      while (*tmp != '\0')
        if (*tmp++ == '/') tmp1 = tmp;
      if (displayInfo->hasBeenEditedButNotSaved) {
        strcpy(str,tmp1);
        strcat(str," (edited)");
        XtVaSetValues(displayInfo->shell,XmNtitle,str,NULL);
      } else {
        XtVaSetValues(displayInfo->shell,XmNtitle,tmp1,NULL);
      }
      return;
    } else {
      element = element->next;
    }
  }
}

void medmMarkDisplayBeingEdited(DisplayInfo *displayInfo)
{
  DlElement *element;
  DlFile *dlFile;
  char str[MAX_FILE_CHARS+10];

  if (globalDisplayListTraversalMode == DL_EXECUTE) return;
  if (displayInfo->hasBeenEditedButNotSaved) return;
  displayInfo->hasBeenEditedButNotSaved = True;
  medmSetDisplayTitle(displayInfo);
}

/*
 * starting at tail of display list, look for smallest object which bounds
 *   the specified position 
 */
DlElement *lookupElement(
  DlElement *tail,
  Position x0,
  Position y0)
{
  DlElement *element, *saveElement, *displayElement;
  int minWidth, minHeight;

  if (currentDisplayInfo == NULL)
    return (NULL);

/* traverse the display list */
  minWidth = INT_MAX;		/* according to XPG2's values.h */
  minHeight = INT_MAX;
  saveElement = NULL;
  displayElement = NULL;

/*
 * single element lookup
 */
     element = tail;

     while (element != currentDisplayInfo->dlElementListHead) {
	if (ELEMENT_IS_RENDERABLE(element->type) 			&&
	    x0 >= (element->structure.rectangle)->object.x		&&
	    x0 <= (element->structure.rectangle)->object.x
			+ (element->structure.rectangle)->object.width	&&
	    y0 >= (element->structure.rectangle)->object.y 		&&
	    y0 <= (element->structure.rectangle)->object.y
			+ (element->structure.rectangle)->object.height) {

	/* eligible element, now see if smallest element so far */

	    if ((element->structure.rectangle)->object.width < minWidth &&
		(element->structure.rectangle)->object.height < minHeight) {

	     /* if only DL_Display is smaller than object, return object */
	      if (element->type != DL_Display || saveElement == NULL) {
	        minWidth = (element->structure.rectangle)->object.width;
	        minHeight = (element->structure.rectangle)->object.height;
	        saveElement = element;
	      }
	    }
	}
	if (element->type == DL_Display) {
	    minWidth = (element->structure.rectangle)->object.width;
	    minHeight = (element->structure.rectangle)->object.height;
	    displayElement = element;
	}
	element = element->prev;
     }

/* assume we'll always find a DL_Display, use that as fallback */
     if (saveElement == NULL) saveElement = displayElement;

/*
 * if we are in EXECUTE mode, then we need to decompose a group hit into
 *  its component, if in EDIT mode, then the group is what we want
 */
     if (globalDisplayListTraversalMode == DL_EXECUTE) {
       if (saveElement->type == DL_Composite) {
  /* find child of composite which was picked */
	 return(lookupCompositeChild(saveElement,x0,y0));
       }
     }

     return (saveElement);
}



/*
 * starting at head of composite (specified element), lookup picked object
 */
DlElement *lookupCompositeChild(
  DlElement *composite,
  Position x0,
  Position y0)
{
  DlElement *element, *saveElement;
  int minWidth, minHeight;

  if (composite == NULL) return (NULL);
  if (composite->type != DL_Composite) return (NULL);

  minWidth = INT_MAX;		/* according to XPG2's values.h */
  minHeight = INT_MAX;
  saveElement = NULL;

/*
 * single element lookup
 */
  element = ((DlElement *)
	composite->structure.composite->dlElementListHead)->next;

  while (element != NULL) {
	if (ELEMENT_IS_RENDERABLE(element->type) 			&&
	    x0 >= (element->structure.rectangle)->object.x		&&
	    x0 <= (element->structure.rectangle)->object.x
			+ (element->structure.rectangle)->object.width	&&
	    y0 >= (element->structure.rectangle)->object.y 		&&
	    y0 <= (element->structure.rectangle)->object.y
			+ (element->structure.rectangle)->object.height) {

	/* eligible element, now see if smallest element so far */

	    if ((element->structure.rectangle)->object.width < minWidth &&
		(element->structure.rectangle)->object.height < minHeight) {

	        minWidth = (element->structure.rectangle)->object.width;
	        minHeight = (element->structure.rectangle)->object.height;
	        saveElement = element;
	    }
	}
	element = element->prev;
  }
  if (saveElement != NULL) {
/* found a new element - if it is composite, recurse, otherwise return it */
    if (saveElement->type == DL_Composite) {
	return(lookupCompositeChild(saveElement,x0,y0));
    } else {
	return(saveElement);
    }
  } else {
/* didn't find anything, return old composite */
    return(composite);
  }

}



/*
 * starting at tail of display list, look for smallest object which bounds
 *   the specified position if single point select, else look for list
 *   of objects bounded by the region defined by (x0,y0) and (x1,y1)
 *
 * if the smallest bounding object has a composite "parent" (is a member of
 *   a group) then actually select the composite/group
 *
 *   - also update currentDisplayInfo's attribute / dynamicAttribute structures
 *     from previous data in display list for single element select
 */
DlElement **selectedElementsLookup(
  DlElement *tail,
  Position x0, 
  Position y0,
  Position x1,
  Position y1,
  int *arraySize,
  int *numSelected)
{
  DlElement **array;
  DlElement *element, *saveElement, *displayElement;
  Position x, y;
  int minWidth, minHeight;
  char string[48];


  if (currentDisplayInfo == NULL) {
    *arraySize = 0;
    *numSelected = 0;
    return (NULL);
  }


/* traverse the display list */
  minWidth = INT_MAX;		/* according to XPG2's values.h */
  minHeight = INT_MAX;
  saveElement = NULL;
  displayElement = NULL;

/* number of pixels to consider  same as no motion... */
#define RUBBERBAND_EPSILON 4

#define INITIAL_ARRAY_SIZE 40

  if ((x1 - x0) <= RUBBERBAND_EPSILON && (y1 - y0) <= RUBBERBAND_EPSILON) {

/*
 * single element lookup
 *   N.B. - this is really just lookupElement()!
 */
     array = (DlElement **) malloc(1*sizeof(DlElement *));
     *arraySize = 1;
     x = (x0 + x1)/2;
     y = (y0 + y1)/2;
     element = tail;

/* MDA - the test against NULL shouldn't be necessary really */
     while (element != currentDisplayInfo->dlElementListHead
							&& element != NULL) {
	if ( ELEMENT_IS_RENDERABLE(element->type)			&&
	    x >= (element->structure.rectangle)->object.x		&&
	    x <= (element->structure.rectangle)->object.x
			+ (element->structure.rectangle)->object.width	&&
	    y >= (element->structure.rectangle)->object.y 		&&
	    y <= (element->structure.rectangle)->object.y
			+ (element->structure.rectangle)->object.height) {

	/* eligible element, now see if smallest element so far */

	    if ((element->structure.rectangle)->object.width < minWidth &&
		(element->structure.rectangle)->object.height < minHeight) {

	     /* if only DL_Display is smaller than object, return object */
	      if (element->type != DL_Display || saveElement == NULL) {
	        minWidth = (element->structure.rectangle)->object.width;
	        minHeight = (element->structure.rectangle)->object.height;
	        saveElement = element;
	      }
	    }
	}
	if (element->type == DL_Display) {
	    minWidth = (element->structure.rectangle)->object.width;
	    minHeight = (element->structure.rectangle)->object.height;
	    displayElement = element;
	}

	element = element->prev;
     }

/* assume we'll always find a DL_Display, use that as fallback */
     if (saveElement == NULL) saveElement = displayElement;


     attributeSet(currentDisplayInfo,saveElement);
     array[0] = saveElement;
     *numSelected = 1;
     return (array);


  } else {

/*
 * multi-element lookup
 *	don't allow DL_Display to be part of multi-element lookup
 *	(only single element lookups for display - this avoids problems
 *	further down the pike and makes sense since logically the display
 *	can in fact extend beyond visible borders {since objects can
 *	extend beyond visible borders})
 */
     element = tail;
     array = (DlElement **) malloc(INITIAL_ARRAY_SIZE*sizeof(DlElement *));
     *arraySize = INITIAL_ARRAY_SIZE;
     *numSelected = 0;

     while (element != currentDisplayInfo->dlElementListHead) {
	if ( ELEMENT_IS_RENDERABLE(element->type)			&&
	    element->type != DL_Display					&&
	    x0 <= (element->structure.rectangle)->object.x		&&
	    x1 >= (element->structure.rectangle)->object.x
			+ (element->structure.rectangle)->object.width	&&
	    y0 <= (element->structure.rectangle)->object.y 		&&
	    y1 >= (element->structure.rectangle)->object.y
			+ (element->structure.rectangle)->object.height) {


	      if (*numSelected < *arraySize) {
	        array[*numSelected] = element;
	        (*numSelected)++;
	      } else {
	      /* overflow allocated array - reallocate */
		array = (DlElement **) realloc((DlElement **) array,
			((*arraySize)*10)*sizeof(DlElement *));
		if (array != NULL) {
		   *arraySize = (*arraySize)*10;
		} else {
		   sprintf(string,"\nselectedElementsLookup: realloc failed!");
		   dmSetAndPopupWarningDialog(currentDisplayInfo,string,"Ok",NULL,NULL);
		   fprintf(stderr,"%s",string);

		   return (array);
		}

	        array[*numSelected] = element;
	        (*numSelected)++;
	      }
	}

	element = element->prev;
     }

     return (array);
  }

}



/*
 * function to return element's dynamic attribute element if one exists
 */
DlElement *lookupDynamicAttributeElement(
  DlElement *elementPtr)
{
  DlElement *returnPtr;

  returnPtr = (DlElement *)NULL;

  if (elementPtr == NULL)	 return(returnPtr);
  if (elementPtr->prev == NULL)  return(returnPtr);

/* see if one precedes this entry */
  if (elementPtr->prev->type == DL_DynamicAttribute)
    returnPtr = elementPtr->prev;

  return(returnPtr);

}




/*
 * function to return element's basic attribute element
 */
DlElement *lookupBasicAttributeElement(
  DlElement *elementPtr)
{
  DlElement *ptr, *returnPtr;

  returnPtr = (DlElement *) NULL;
  if (elementPtr == NULL)	 return(returnPtr);
  if (elementPtr->prev == NULL)  return(returnPtr);

/*
 * find element's basic attribute (loop back arbitrarily far)
 */
  ptr = elementPtr->prev;
  while (ptr != NULL && ptr->type != DL_BasicAttribute && 
	 ptr->type != DL_Display &&
	 ptr != currentDisplayInfo->dlElementListHead) {
       ptr = ptr->prev;
  }

  if (ptr->type == DL_BasicAttribute)
       returnPtr = ptr;

  return(returnPtr);

}


/*
 * function to return element's PRIVATE basic attribute element
 */
DlElement *lookupPrivateBasicAttributeElement(
  DlElement *elementPtr)
{
  DlElement *returnPtr;

  returnPtr = (DlElement *) NULL;
  if (elementPtr == NULL)	 return(returnPtr);
  if (elementPtr->prev == NULL)  return(returnPtr);

/*
 * find element's private basic attribute 
 *	this means that the element is immediately preceded by
 *		a basic attribute, or
 *		a dynamic attribute which is preceded by a basic attribute
 *	and immediately followed by 
 *		a NULL ptr (no other element), or
 *		a basic attribute
 */
  if (elementPtr->prev->type == DL_BasicAttribute) {
    if (elementPtr->next == NULL)
	returnPtr = elementPtr->prev;
    else if (elementPtr->next->type == DL_BasicAttribute)
	returnPtr = elementPtr->prev;
  }  else if (elementPtr->prev->type == DL_DynamicAttribute) {
    if (elementPtr->prev->prev != NULL) {
      if (elementPtr->prev->prev->type == DL_BasicAttribute) {
        if (elementPtr->next == NULL)
	  returnPtr = elementPtr->prev->prev;
        else if (elementPtr->next->type == DL_BasicAttribute)
	  returnPtr = elementPtr->prev->prev;
      }
    }
  }

  return(returnPtr);

}






/*
 * function to resize the display elements based on new drawingArea size
 *	return value is a boolean saying whether resized actually occurred
 */

Boolean dmResizeDisplayList(
  DisplayInfo *displayInfo,
  Dimension newWidth, 
  Dimension newHeight)
{
  DlElement *elementPtr;
  float sX, sY;
  Dimension oldWidth, oldHeight;
  Boolean moveWidgets;
  int j, newX, newY;

  elementPtr = ((DlElement *)displayInfo->dlElementListHead)->next;

/* get to DL_Display type which has old x,y,width,height */
  while (elementPtr->type != DL_Display) { elementPtr = elementPtr->next; }
  oldWidth = elementPtr->structure.display->object.width;
  oldHeight = elementPtr->structure.display->object.height;

/* simply return (value FALSE) if no real change */
  if (oldWidth == newWidth && oldHeight == newHeight) return (FALSE);

/* resize the display, then do selected elements */
  elementPtr->structure.display->object.width = newWidth;
  elementPtr->structure.display->object.height = newHeight;

/* else proceed with scaling...*/
  sX = (float) ((float)newWidth/(float)oldWidth);
  sY = (float) ((float)newHeight/(float)oldHeight);


  while (elementPtr != NULL) {

    if (ELEMENT_IS_RENDERABLE(elementPtr->type)
			&& elementPtr->type != DL_Display ) {

       if (elementPtr->type == DL_Composite) {

	resizeCompositeChildren(displayInfo,elementPtr,elementPtr,sX,sY);

/* special handling for composite - since this resize is not "anchored" and the 
 *  resizeCompositeChildren() function does "anchored" resizes of parent
 *  composite, we do the resize of the children, and then additionally move
 *  the composite and its children
 */
	newX = (Position) (sX*elementPtr->structure.rectangle->object.x+0.5);
	newY = (Position) (sY*elementPtr->structure.rectangle->object.y+0.5);
	moveWidgets = False;
	moveCompositeChildren(displayInfo,elementPtr,
		(int)(newX - elementPtr->structure.rectangle->object.x),
		(int)(newY - elementPtr->structure.rectangle->object.y),
		moveWidgets);
	elementPtr->structure.rectangle->object.x = newX;
	elementPtr->structure.rectangle->object.y = newY;

       } else {

/* extra work if polyline/polygon: move constituent points too */
	if (elementPtr->type == DL_Polyline) {
	  for (j = 0; j < elementPtr->structure.polyline->nPoints; j++) {
	    elementPtr->structure.polyline->points[j].x = 
	        (Position)(sX*elementPtr->structure.polyline->points[j].x+0.5);
	    elementPtr->structure.polyline->points[j].y = 
	        (Position)(sY*elementPtr->structure.polyline->points[j].y+0.5);
	  }
	} else if (elementPtr->type == DL_Polygon) {
	  for (j = 0; j < elementPtr->structure.polygon->nPoints; j++) {
	    elementPtr->structure.polygon->points[j].x = 
	        (Position)(sX*elementPtr->structure.polygon->points[j].x+0.5);
	    elementPtr->structure.polygon->points[j].y = 
	        (Position)(sY*elementPtr->structure.polygon->points[j].y+0.5);
	  }
	}

/* get object data: must have object entry  - use rectangle type (arbitrary) */
	elementPtr->structure.rectangle->object.x = 
	   (Position) (sX*elementPtr->structure.rectangle->object.x+0.5);
	elementPtr->structure.rectangle->object.y = 
	   (Position) (sY*elementPtr->structure.rectangle->object.y+0.5);
	elementPtr->structure.rectangle->object.width = 
	   (Dimension) (sX*elementPtr->structure.rectangle->object.width+0.5);
	elementPtr->structure.rectangle->object.height = 
	   (Dimension) (sY*elementPtr->structure.rectangle->object.height+0.5);
       }

    }
    elementPtr = elementPtr->next;
  }
  return (TRUE);

}



/*
 * function to resize the selected display elements based on new drawingArea.
 *
 *   return value is a boolean saying whether resized actually occurred.
 *   this function resizes the selected elements when the whole display
 *   is resized.
 */

Boolean dmResizeSelectedElements(
  DisplayInfo *displayInfo,
  Dimension newWidth, 
  Dimension newHeight)
{
  DlElement *elementPtr;
  float sX, sY;
  Position newX, newY;
  Dimension oldWidth, oldHeight;
  int i, j;
  Boolean moveWidgets;

  elementPtr = ((DlElement *)displayInfo->dlElementListHead)->next;

/* get to DL_Display type which has old x,y,width,height */
  while (elementPtr->type != DL_Display) { elementPtr = elementPtr->next; }
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


  for (i = 0; i < displayInfo->numSelectedElements; i++) {

    elementPtr = displayInfo->selectedElementsArray[i];

    if ( ELEMENT_IS_RENDERABLE(elementPtr->type)
				&& elementPtr->type != DL_Display) {

       if (elementPtr->type == DL_Composite) {

	resizeCompositeChildren(displayInfo,elementPtr,elementPtr,sX,sY);

/* special handling for composite - since this resize is not "anchored" and the 
 *  resizeCompositeChildren() function does "anchored" resizes of parent
 *  composite, we do the resize of the children, and then additionally move
 *  the composite and its children
 */
	newX = (Position) (sX*elementPtr->structure.rectangle->object.x+0.5);
	newY = (Position) (sY*elementPtr->structure.rectangle->object.y+0.5);
	moveWidgets = True;
	moveCompositeChildren(displayInfo,elementPtr,
		(int)(newX - elementPtr->structure.rectangle->object.x),
		(int)(newY - elementPtr->structure.rectangle->object.y),
		moveWidgets );
	elementPtr->structure.rectangle->object.x = newX;
	elementPtr->structure.rectangle->object.y = newY;

/* Polyline */
       } else if (elementPtr->type == DL_Polyline) {
	  for (j = 0; j < elementPtr->structure.polyline->nPoints; j++) {
		elementPtr->structure.polyline->points[j].x *= sX;
		elementPtr->structure.polyline->points[j].y *= sY;
	  }
	  elementPtr->structure.polyline->object.x = 
	   (Position) (sX*elementPtr->structure.polyline->object.x+0.5);
	  elementPtr->structure.polyline->object.y = 
	   (Position) (sY*elementPtr->structure.polyline->object.y+0.5);
	  elementPtr->structure.polyline->object.width = 
	   (Dimension) (sX*elementPtr->structure.polyline->object.width+0.5);
	  elementPtr->structure.polyline->object.height = 
	   (Dimension) (sY*elementPtr->structure.polyline->object.height+0.5);

/* Polygon */
       } else if (elementPtr->type == DL_Polygon) {
	  for (j = 0; j < elementPtr->structure.polygon->nPoints; j++) {
		elementPtr->structure.polygon->points[j].x *= sX;
		elementPtr->structure.polygon->points[j].y *= sY;
	  }
	  elementPtr->structure.polygon->object.x = 
	   (Position) (sX*elementPtr->structure.polygon->object.x+0.5);
	  elementPtr->structure.polygon->object.y = 
	   (Position) (sY*elementPtr->structure.polygon->object.y+0.5);
	  elementPtr->structure.polygon->object.width = 
	   (Dimension) (sX*elementPtr->structure.polygon->object.width+0.5);
	  elementPtr->structure.polygon->object.height = 
	   (Dimension) (sY*elementPtr->structure.polygon->object.height+0.5);


/* all the others */
       } else {

/* get object data: must have object entry  - use rectangle type (arbitrary) */
	elementPtr->structure.rectangle->object.x = 
	   (Position) (sX*elementPtr->structure.rectangle->object.x+0.5);
	elementPtr->structure.rectangle->object.y = 
	   (Position) (sY*elementPtr->structure.rectangle->object.y+0.5);
	elementPtr->structure.rectangle->object.width = 
	   (Dimension) (sX*elementPtr->structure.rectangle->object.width+0.5);
	elementPtr->structure.rectangle->object.height = 
	   (Dimension) (sY*elementPtr->structure.rectangle->object.height+0.5);
       }

    }

  }
  return (TRUE);

}








/******************************************
 ************ rubberbanding, etc.
 ******************************************/


GC xorGC;

/*
 * initialize rubberbanding
 */

void initializeRubberbanding()
{
/*
 * create the xorGC and rubberbandCursor for drawing while dragging
 */
  xorGC = XCreateGC(display,rootWindow,0,NULL);
  XSetSubwindowMode(display,xorGC,IncludeInferiors);
  XSetFunction(display,xorGC,GXxor);

  XSetForeground(display,xorGC,getPixelFromColormapByString(display,screenNum,
	cmap,"grey50"));
}




/*
 * do rubberbanding
 */
void doRubberbanding(
  Window window,
  Position *initialX,
  Position *initialY,
  Position *finalX,
  Position *finalY)
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


/* have all interesting events go to window */
  XGrabPointer(display,window,FALSE,
	ButtonMotionMask|ButtonReleaseMask,
        GrabModeAsync,GrabModeAsync,None,rubberbandCursor,CurrentTime);

/* grab the server to ensure that XORing will be okay */
  XGrabServer(display);
  XDrawRectangle(display,window, xorGC,
	MIN(x0,x1), MIN(y0,y1), w, h);

/*
 * now loop until the button is released
 */
  while (TRUE) {
	XtAppNextEvent(appContext,&event);
	switch (event.type) {
		case ButtonRelease:
		/* undraw old one */
			XDrawRectangle(display,window, xorGC,
			    MIN(x0,x1), MIN(y0,y1), w, h);
			XUngrabServer(display);
			XUngrabPointer(display,CurrentTime);
			*initialX =  MIN(x0,event.xbutton.x);
			*initialY =  MIN(y0,event.xbutton.y);
			*finalX   =  MAX(x0,event.xbutton.x);
			*finalY   =  MAX(y0,event.xbutton.y);
			return;		/* return from while(TRUE) */
			break;
		case MotionNotify:
		/* undraw old one */
			XDrawRectangle(display,window, xorGC,
			    MIN(x0,x1), MIN(y0,y1), w, h);
		/* update current coordinates */
			x1 = event.xbutton.x;
			y1 = event.xbutton.y;
			w =  (MAX(x0,x1) - MIN(x0,x1));
			h =  (MAX(y0,y1) - MIN(y0,y1));
		/* draw new one */
			XDrawRectangle(display,window, xorGC,
			    MIN(x0,x1), MIN(y0,y1), w, h); 
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
Boolean doDragging(
  Window window,
  Dimension daWidth,
  Dimension daHeight,
  Position initialX, 
  Position initialY, 
  Position *finalX,
  Position *finalY)
{
  DlElement **array;
  int i, minX, maxX, minY, maxY, groupWidth, groupHeight,
	groupDeltaX0, groupDeltaY0, groupDeltaX1, groupDeltaY1;
  XEvent event;
  int xOffset, yOffset;
  DisplayInfo *cdi;
  int xdel, ydel;

/* if on current display, simply return */
  if (currentDisplayInfo == NULL) return (False);

  cdi = currentDisplayInfo;

/* just have different names for globals (less typing, more clarity) */
  array = cdi->selectedElementsArray;

  xOffset = 0;
  yOffset = 0;

  minX = INT_MAX; minY = INT_MAX;
  maxX = INT_MIN; maxY = INT_MIN;

/* have all interesting events go to window */
  XGrabPointer(display,window,FALSE,
	ButtonMotionMask|ButtonReleaseMask,
        GrabModeAsync,GrabModeAsync,None,dragCursor,CurrentTime);
/* grab the server to ensure that XORing will be okay */
  XGrabServer(display);

  /* as usual, type in union unimportant as long as object is 1st thing...*/
  for (i = 0; i < cdi->numSelectedElements; i++) {
    if (array[i]->type != DL_Display)
       XDrawRectangle(display,window, xorGC, 
	array[i]->structure.rectangle->object.x + xOffset,
	array[i]->structure.rectangle->object.y + yOffset,
	array[i]->structure.rectangle->object.width ,
	array[i]->structure.rectangle->object.height);

	minX = MIN(minX, (int)array[i]->structure.rectangle->object.x);
	maxX = MAX(maxX, (int)array[i]->structure.rectangle->object.x +
			(int)array[i]->structure.rectangle->object.width);
	minY = MIN(minY, (int)array[i]->structure.rectangle->object.y);
	maxY = MAX(maxY, (int)array[i]->structure.rectangle->object.y +
			(int)array[i]->structure.rectangle->object.height);
  }

  groupWidth = maxX - minX;
  groupHeight = maxY - minY;
/* how many pixels is the cursor position from the left edge of all objects */
  groupDeltaX0 = initialX - minX;
/* how many pixels is the cursor position from the top edge of all objects */
  groupDeltaY0 = initialY - minY;
/* how many pixels is the cursor position from the right edge of all objects */
  groupDeltaX1 = groupWidth - groupDeltaX0;
/* how many pixels is the cursor position from the bottom edge of all objects */
  groupDeltaY1 = groupHeight - groupDeltaY0;

/*
 * now loop until the button is released
 */
  while (TRUE) {
	XtAppNextEvent(appContext,&event);
	switch (event.type) {
		case ButtonRelease:
		/* undraw old ones */
			for (i = 0; i < cdi->numSelectedElements; i++) {
			  if (array[i]->type != DL_Display)
			    XDrawRectangle(display,window, xorGC, 
				array[i]->structure.rectangle->object.x+xOffset,
				array[i]->structure.rectangle->object.y+yOffset,
				array[i]->structure.rectangle->object.width,
				array[i]->structure.rectangle->object.height);
			}
			XUngrabServer(display);
			XUngrabPointer(display,CurrentTime);
			 *finalX = initialX + xOffset;
			 *finalY = initialY + yOffset;
/* (always return true - for clipped dragging...) */
			return (True);	/* return from while(TRUE) */
			break;
		case MotionNotify:
		/* undraw old ones */
			for (i = 0; i < cdi->numSelectedElements; i++) {
			  if (array[i]->type != DL_Display)
			     XDrawRectangle(display,window, xorGC, 
				array[i]->structure.rectangle->object.x+xOffset,
				array[i]->structure.rectangle->object.y+yOffset,
				array[i]->structure.rectangle->object.width,
				array[i]->structure.rectangle->object.height);
			}
		/* update current coordinates */
			if (event.xmotion.x < groupDeltaX0)
				xdel = groupDeltaX0;
			else if (event.xmotion.x > (int)(daWidth-groupDeltaX1))
				xdel =  daWidth - groupDeltaX1;
			else xdel =  event.xmotion.x;
			if (event.xmotion.y < groupDeltaY0)
				ydel = groupDeltaY0;
			else if (event.xmotion.y > (int)(daHeight-groupDeltaY1))
				ydel =  daHeight - groupDeltaY1;
			else ydel =  event.xmotion.y;

			xOffset = xdel - initialX;
			yOffset  = ydel - initialY;
			for (i = 0; i < cdi->numSelectedElements; i++) {
			  if (array[i]->type != DL_Display)
			    XDrawRectangle(display,window, xorGC, 
				array[i]->structure.rectangle->object.x+xOffset,
				array[i]->structure.rectangle->object.y+yOffset,
				array[i]->structure.rectangle->object.width,
				array[i]->structure.rectangle->object.height);
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
DisplayInfo *doPasting(
  Position *displayX, 
  Position *displayY,
  int *offsetX,
  int *offsetY)
{
  XEvent event;
  DisplayInfo *displayInfo;
  int dx, dy, xul, yul, xlr, ylr;
  Window window, childWindow, root, child;
  int rootX, rootY, winX, winY;
  int x, y;
  unsigned int mask;
  Boolean status;
  int i;

/* if no clipboard elements, simply return */
  if (numClipboardElements <= 0 ||
	clipboardElementsArray == NULL) return (NULL);

  window = RootWindow(display,screenNum);

/* get position of upper left element in display */
  xul = INT_MAX;
  yul = INT_MAX;
  xlr = 0;
  ylr = 0;
/* try to normalize for paste such that cursor is in middle of pasted objects */
  for (i = 0; i < numClipboardElements; i++) {
    if ( ELEMENT_IS_RENDERABLE(clipboardElementsArray[i].type) &&
			clipboardElementsArray[i].type != DL_Display) {
      xul = MIN(xul, clipboardElementsArray[i].structure.rectangle->object.x);
      yul = MIN(yul, clipboardElementsArray[i].structure.rectangle->object.y);
      xlr = MAX(xlr, clipboardElementsArray[i].structure.rectangle->object.x +
	clipboardElementsArray[i].structure.rectangle->object.width);
      ylr = MAX(ylr, clipboardElementsArray[i].structure.rectangle->object.y +
	clipboardElementsArray[i].structure.rectangle->object.height);
    }
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
  XGrabPointer(display,window,False,
	PointerMotionMask|ButtonReleaseMask|ButtonPressMask|EnterWindowMask,
        GrabModeAsync,GrabModeAsync,None,dragCursor,CurrentTime);
/* grab the server to ensure that XORing will be okay */
  XGrabServer(display);

  /* as usual, type in union unimportant as long as object is 1st thing...*/
  for (i = 0; i < numClipboardElements; i++) {
    if ( ELEMENT_IS_RENDERABLE(clipboardElementsArray[i].type) &&
			clipboardElementsArray[i].type != DL_Display) {
       XDrawRectangle(display,window, xorGC, 
	rootX + clipboardElementsArray[i].structure.rectangle->object.x - dx,
	rootY + clipboardElementsArray[i].structure.rectangle->object.y - dy,
	clipboardElementsArray[i].structure.rectangle->object.width ,
	clipboardElementsArray[i].structure.rectangle->object.height);
    }
  }

/*
 * now loop until the button is released
 */
  while (TRUE) {
	XtAppNextEvent(appContext,&event);
	switch (event.type) {
		case ButtonRelease:
		/* undraw old ones */
			for (i = 0; i < numClipboardElements; i++) {
			  if ( ELEMENT_IS_RENDERABLE(
				clipboardElementsArray[i].type) &&
				clipboardElementsArray[i].type != DL_Display) {
			    XDrawRectangle(display,window, xorGC, 
	rootX + clipboardElementsArray[i].structure.rectangle->object.x - dx,
	rootY + clipboardElementsArray[i].structure.rectangle->object.y - dy,
		clipboardElementsArray[i].structure.rectangle->object.width,
		clipboardElementsArray[i].structure.rectangle->object.height);
			  }
			}
			XUngrabServer(display);
			XUngrabPointer(display,CurrentTime);
			XSync(display,False);
			while (XtAppPending(appContext)) {
			    XtAppNextEvent(appContext,&event);
			    XtDispatchEvent(&event);
			}

			displayInfo = pointerInDisplayInfo;
			if (displayInfo != NULL) {
			   XTranslateCoordinates(display,window,
				XtWindow(displayInfo->drawingArea),
				rootX,rootY,
				&winX,&winY,&childWindow);
			}
			*displayX =  winX;
			*displayY =  winY;
			return (displayInfo);
			break;

		case MotionNotify:
		/* undraw old ones */
			for (i = 0; i < numClipboardElements; i++) {
			  if ( ELEMENT_IS_RENDERABLE(
				clipboardElementsArray[i].type) &&
				clipboardElementsArray[i].type != DL_Display) {
			     XDrawRectangle(display,window, xorGC, 
	rootX + clipboardElementsArray[i].structure.rectangle->object.x - dx,
	rootY + clipboardElementsArray[i].structure.rectangle->object.y - dy,
		clipboardElementsArray[i].structure.rectangle->object.width,
		clipboardElementsArray[i].structure.rectangle->object.height);
			  }
			}
		/* update current coordinates */
			rootX = event.xbutton.x_root;
			rootY = event.xbutton.y_root;

		/* draw new ones */
			for (i = 0; i < numClipboardElements; i++) {
			  if ( ELEMENT_IS_RENDERABLE(
				clipboardElementsArray[i].type) &&
				clipboardElementsArray[i].type != DL_Display) {
			    XDrawRectangle(display,window, xorGC, 
	rootX + clipboardElementsArray[i].structure.rectangle->object.x - dx,
	rootY + clipboardElementsArray[i].structure.rectangle->object.y - dy,
		clipboardElementsArray[i].structure.rectangle->object.width,
		clipboardElementsArray[i].structure.rectangle->object.height);
			  }
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
  int i;

  if (currentDisplayInfo == NULL) return (False);
  if (currentDisplayInfo->selectedElementsArray == NULL) return (False);

  for (i = 0; i < currentDisplayInfo->numSelectedElements; i++) {
     if (currentDisplayInfo->selectedElementsArray[i] == element) return (True);
  }
  return (False);

}





/*
 * do (multiple) resizing of all elements in global selectedElementsArray
 *	RETURNS: boolean indicating whether resize ended in the window
 *	(and hence was valid)
 */
Boolean doResizing(
  Window window,
  Position initialX, 
  Position initialY, 
  Position *finalX, 
  Position *finalY)
{
  DlElement **array;
  int i, xOffset, yOffset;
  XEvent event;
  Boolean inWindow;
  DisplayInfo *cdi;
  int width, height;


  if (currentDisplayInfo == NULL) return False;
  cdi = currentDisplayInfo;

/* just have different names for globals (less typing, more clarity) */
  array = cdi->selectedElementsArray;
  xOffset = 0;
  yOffset = 0;

  inWindow = True;

/* have all interesting events go to window */
  XGrabPointer(display,window,FALSE,
	ButtonMotionMask|ButtonReleaseMask,
        GrabModeAsync,GrabModeAsync,None,resizeCursor,CurrentTime);
/* grab the server to ensure that XORing will be okay */
  XGrabServer(display);

  /* as usual, type in union unimportant as long as object is 1st thing...*/
  for (i = 0; i < cdi->numSelectedElements; i++) {
    if (array[i]->type != DL_Display)
       width = (array[i]->structure.rectangle->object.width + xOffset);
       width = MAX(1,width);
       height = (array[i]->structure.rectangle->object.height + yOffset);
       height = MAX(1,height);
       XDrawRectangle(display,window, xorGC, 
	array[i]->structure.rectangle->object.x,
	array[i]->structure.rectangle->object.y,
	(Dimension)width, (Dimension)height);
  }

/*
 * now loop until the button is released
 */
  while (TRUE) {
	XtAppNextEvent(appContext,&event);
	switch (event.type) {
		case ButtonRelease:
		/* undraw old ones */
			for (i = 0; i < cdi->numSelectedElements; i++) {
			 if (array[i]->type != DL_Display)
			   width = (
				array[i]->structure.rectangle->object.width
				+ xOffset);
			   width = MAX(1,width);
			   height = (
				array[i]->structure.rectangle->object.height
				+ yOffset);
			   height = MAX(1,height);
			   XDrawRectangle(display,window, xorGC, 
			    array[i]->structure.rectangle->object.x,
			    array[i]->structure.rectangle->object.y,
			    (Dimension)width, (Dimension)height);
			}
			XUngrabServer(display);
			XUngrabPointer(display,CurrentTime);
			*finalX =  initialX + xOffset;
			*finalY =  initialY + yOffset;
			return (inWindow);	/* return from while(TRUE) */
			break;
		case MotionNotify:
		/* undraw old ones */
			for (i = 0; i < cdi->numSelectedElements; i++) {
			 if (array[i]->type != DL_Display)
			   width = (
				array[i]->structure.rectangle->object.width
				+ xOffset);
			   width = MAX(1,width);
			   height = (
				array[i]->structure.rectangle->object.height
				+ yOffset);
			   height = MAX(1,height);
			   XDrawRectangle(display,window, xorGC, 
			    array[i]->structure.rectangle->object.x,
			    array[i]->structure.rectangle->object.y,
			    (Dimension)width, (Dimension)height);
			}
		/* update current coordinates */
			xOffset = event.xbutton.x - initialX;
			yOffset = event.xbutton.y - initialY;
		/* draw new ones */
			for (i = 0; i < cdi->numSelectedElements; i++) {
			  if (array[i]->type != DL_Display)
			   width = (
				array[i]->structure.rectangle->object.width
				+ xOffset);
			   width = MAX(1,width);
			   height = (
				array[i]->structure.rectangle->object.height
				+ yOffset);
			   height = MAX(1,height);
			   XDrawRectangle(display,window, xorGC, 
			    array[i]->structure.rectangle->object.x,
			    array[i]->structure.rectangle->object.y,
			    (Dimension)width, (Dimension)height);
			}
			break;
		default:
			XtDispatchEvent(&event);
	}
  }
}



/*
 * function to lookup widgets in displayInfo's child[] array and compare
 *	to the object's specified x,y,w,h.  if a match is found,
 *	then that widget is returned
 *	since two widgets can't really (sensibly) occupy the same exact space,
 *	it is safe to assume that returning 0 or 1 widgets is adequate
 */
Widget lookupElementWidget(
  DisplayInfo *displayInfo,
  DlObject *object)
{
  Position x, y;
  Dimension width, height;
  int i;

  for (i = 0; i < displayInfo->childCount; i++) {
    /* not so bad since these are cached client-side - no server trips */
    XtVaGetValues(displayInfo->child[i],
	XmNx,&x,XmNy,&y,NULL);
/* supposing that overlap of widgets is not an issue, then x,y is adequate */
    if (x == object->x && y == object->y) {
	/* found a widget! */
	return (displayInfo->child[i]);
    }
  }
  /* didn't find one yet, see if drawing area (parent is the one) */
  XtVaGetValues(displayInfo->drawingArea,
	XmNwidth,&width,XmNheight,&height,NULL);
  if (width == object->width && height == object->height) {
      return(displayInfo->drawingArea);
  }

  return ((Widget)NULL);

}


/*
 * function to destroy a widget in displayInfo's child[] array.
 *	if a match is found, then that widget is destroyed and the
 *	child array is updated.  note that this implicitly can't destroy
 *	the parent (DL_Display) widget (DrawingArea) since it is the parent,
 *	not a child...
 */
void destroyElementWidget(
  DisplayInfo *displayInfo,
  Widget widget)
{
 int i,j;

 if (displayInfo != NULL && widget != NULL) {
  for (i = 0; i < displayInfo->childCount; i++) {
     if ( widget == displayInfo->child[i]) {
	XtDestroyWidget(displayInfo->child[i]);
	/* now shift remaining widgets up in array and update child count */
	for (j = i; j < displayInfo->childCount - 1; j++) {
	    displayInfo->child[j] = displayInfo->child[j+1];
	}
	displayInfo->child[displayInfo->childCount - 1] = NULL;
	displayInfo->childCount -= 1;
	return;
    }
  }
 }

}

/*
 * function to return the number of children/grandchildren... in a composite
 *  this does a depth-first search for any descendent composites and adds
 *  in their children, etc.
 */
int numberOfElementsInComposite(DisplayInfo *displayInfo, DlElement *ele)
{
  DlElement *child;
  int returnCount;

  returnCount = 0;
  if (ele->type == DL_Composite) {
    child = ((DlElement *)ele->structure.composite->dlElementListHead)->next;
    while (child != NULL) {
      if (child->type == DL_Composite) {
	returnCount += numberOfElementsInComposite(displayInfo,child) + 1;
      } else {
	returnCount++;
      }
      child = child->next;
    }
  }
  return(returnCount);
}



/*
 * function to copy and return the number of children/grandchildren...
 *  in a composite
 *  this does a depth-first search for any descendent composites and adds
 *  in their children, etc.
 */
int copyCompositeChildrenIntoClipboard(DisplayInfo *displayInfo,
		DlElement *ele,
		DlElement clipboardArray[])
{

  DlElement *child, *basic, *dyn;
  int localCount, numCompositeChildren, currentIndex;

  localCount = 0;
  numCompositeChildren = 0;
  currentIndex = 0;
  if (ele->type == DL_Composite) {

    child = ((DlElement *)ele->structure.composite->dlElementListHead)->next;
    while (child != NULL) {

      if (child->type != DL_Display) {
/* don't allow copy of DL_Display into clipboard! */

       if (child->type != DL_Composite) {
/* -- careful about order: dynamic attribute must immediately precede object */
 /* basic attribute data */
	basic = lookupBasicAttributeElement(child);
	if (basic != NULL) {
	  clipboardArray[localCount] = *(basic);
	  localCount++;
	}
 /* dynamic attribute data (if any) */
	dyn = lookupDynamicAttributeElement(child);
	if (dyn != NULL) {
	  clipboardArray[localCount] = *(dyn);
	  localCount++;
	}
       }

 /* copy actual (renderable) element */
       clipboardArray[localCount] = *(child);
       currentIndex = localCount;
       localCount++;

 /* special case for Composite & Image: */

       switch (child->type) {

	  case DL_Image:
 /* since hauling that privateData pointer around
  *	need to zero it, because traversal for an image is not quite as
  *	easy as the other types
  */
            clipboardArray[currentIndex].structure.image->privateData = NULL;
	    break;

 /* since composites have children - haul them (recursively) into array too */
	  case DL_Composite:
	    numCompositeChildren = copyCompositeChildrenIntoClipboard(
		displayInfo,
		&(clipboardArray[currentIndex]),
		&clipboardArray[localCount]);
	    localCount += numCompositeChildren;
	    break;

       }

      } /* end if */

      child = child->next;
    }
  }
  return(localCount);
}



/*
 * function to delete composite's children/grandchildren...
 *  this does a depth-first search for any descendent composites...
 */
void deleteElementsInComposite(DisplayInfo *displayInfo, DlElement *ele)
{
  DlElement *child, *dyn;

  if (ele->type == DL_Composite) {

    child = ((DlElement *)ele->structure.composite->dlElementListHead)->next;
    while (child != NULL) {
      if ( ELEMENT_IS_RENDERABLE(child->type) && child->type != DL_Display) {
    /* don't allow user to delete the display! */

	if (child->type != DL_Composite) {
      /* first look to see if a dynamic attribute exists for this element */
         dyn = lookupDynamicAttributeElement(child);
         if (dyn != NULL) {
      /* found a dynamicAttribute - delete it, and then its operand element */
          if (dyn->prev != NULL) (dyn->prev)->next = dyn->next;
          if (dyn->next != NULL)
	    (dyn->next)->prev = dyn->prev;
	  else /* must be at tail - update tail */
	    displayInfo->dlElementListTail = dyn->prev;
         }
	}
      /* now delete the selected element */
        if (child->prev != NULL) (child->prev)->next = child->next;
        if (child->next != NULL)
	   (child->next)->prev = child->prev;
	 else /* must be at tail - update tail */
	    displayInfo->dlElementListTail = child->prev;

      /* if composite, delete any children */
        if (child->type == DL_Composite) {
	  deleteElementsInComposite(displayInfo,child);
        }

/*
 * set this flag to true - next clearClipboard call can actually 
 *	free the display list elements in memory (elements have actually
 *	been deleted)
 */
        clipboardDelete = True;

        if (ELEMENT_HAS_WIDGET(child->type)) {
       /* lookup widget of specified x,y,width,height and destroy */
	 destroyElementWidget(displayInfo,
	     lookupElementWidget(displayInfo,
			&(child->structure.rectangle->object)));
        }
      }
      child = child->next;
    }
  }
}



/*
 * function to delete composite's children/grandchildren... WIDGETS ONLY
 *  this does a depth-first search for any descendent composites...
 */
void deleteWidgetsInComposite(DisplayInfo *displayInfo, DlElement *ele)
{
  DlElement *child, *dyn;

  if (ele->type == DL_Composite) {

    child = ((DlElement *)ele->structure.composite->dlElementListHead)->next;
    while (child != NULL) {
      /* if composite, delete any children */
        if (child->type == DL_Composite) {
	  deleteWidgetsInComposite(displayInfo,child);
        }
        if (ELEMENT_HAS_WIDGET(child->type)) {
       /* lookup widget of specified x,y,width,height and destroy */
	 destroyElementWidget(displayInfo,
	     lookupElementWidget(displayInfo,
			&(child->structure.rectangle->object)));
        }
        child = child->next;
    }
  }
}




/****************************************************************************
 ***
 ***		   display element clipboard functions
 ***
 ****************************************************************************/

void clearClipboard()
{
  int i;

/*
 * since we need undo and paste functions, we must keep dynamically 
 * allocated display list elements around until clipboard is cleared.
 *
 * free the display list elements out there in memory only when clipboard
 * is cleared, not when elements are removed from the display list.
 * N.B. that the elements are logically removed from display list 
 * on cut/delete operations.
 */

  if (numClipboardElements == 0 || clipboardElementsArray == NULL) return;

/*
 *  now free the actual display list elements (structures)
 */
  if (clipboardDelete) {
    for (i = 0; i < numClipboardElements; i++) {
	if (clipboardElementsArray[i].type != DL_BasicAttribute) {
	  if (clipboardElementsArray[i].type != DL_Composite) {
	    free ((char *) (clipboardElementsArray[i].structure.rectangle));
	    if (clipboardElementsArray[i].type == DL_Polyline) {
		free ((char *)(clipboardElementsArray[i].structure.polyline
				->points));
	    } else if (clipboardElementsArray[i].type == DL_Polygon) {
		free ((char *)(clipboardElementsArray[i].structure.polygon
				->points));
	    }
	  } else {

/*** need special handling for composite - this as is doesn't totally work
fprintf(stderr,"\nclearClipboard: special handling for composite");
	    deleteElementsInComposite(currentDisplayInfo,
			&(clipboardElementsArray[i]));
***/
	  }
	}
    }
    clipboardDelete = False;
  }

  if (clipboardElementsArray != NULL) free((char *)clipboardElementsArray);
  numClipboardElements = 0;
  clipboardElementsArraySize = 0;
}



void copyElementsIntoClipboard()
{
  int i;
  DisplayInfo *cdi;
  DlElement *dyn, *basic;
  int currentIndex;

  if (currentDisplayInfo == NULL) return;
  if (currentDisplayInfo->numSelectedElements == 0) return;
  cdi = currentDisplayInfo;
  clearClipboard();


/* worst case scenario - assume all elements have both Basic and Dynamic
 *  attributes - and allocate accordingly
 */
  clipboardElementsArraySize = 3*cdi->numSelectedElements;
  clipboardElementsArray = (DlElement *) calloc(clipboardElementsArraySize,
					sizeof(DlElement));

/*
 * since the lookup algorithm processes back to front, the elements
 *	in selectedElementsArray are in reverse order wrt visibility
 *	hence - store elements into clipboard in original front to
 *	back order which preserves visibility
 */
  numClipboardElements = 0;

/* NB: structure copies all over the place */

  for (i = cdi->numSelectedElements - 1; i >= 0; i--) {

   if (cdi->selectedElementsArray[i]->type != DL_Display) {
/* don't allow copy of DL_Display into clipboard! */

    if (cdi->selectedElementsArray[i]->type != DL_Composite) {

/* -- careful about order: dynamic attribute must immediately precede object */
 /* basic attribute data */
      basic = lookupBasicAttributeElement(cdi->selectedElementsArray[i]);
      if (basic != NULL) {
        clipboardElementsArray[numClipboardElements] = *(basic);
        numClipboardElements++;
      }

 /* dynamic attribute data (if any) */
      dyn = lookupDynamicAttributeElement(cdi->selectedElementsArray[i]);
      if (dyn != NULL) {
        clipboardElementsArray[numClipboardElements] = *(dyn);
        numClipboardElements++;
      }
    }

 /* copy actual (renderable) element */
    clipboardElementsArray[numClipboardElements] = 
			*(cdi->selectedElementsArray[i]);
    currentIndex = numClipboardElements;
    numClipboardElements++;

 /* special case for Image: */

    switch (clipboardElementsArray[currentIndex].type) {

	case DL_Image:
 /* since hauling that privateData pointer around
  *	need to zero it, because traversal for an image is not quite as
  *	easy as the other types
  */
          clipboardElementsArray[
		currentIndex].structure.image->privateData = NULL;
	  break;
    }

   } /* end if */

  } /* end for */

}




DlStructurePtr createCopyOfElementType(
  DlElementType type,
  DlStructurePtr ptr)
{
  DlStructurePtr new, structurePtr;
  DlElement *child, *elementPtr;
  int i;

/* NOTE:  using structure copies instead of memcpy's */

  switch(type) {

      case DL_BasicAttribute:
	new.basicAttribute = (DlBasicAttribute *)
			malloc(sizeof(DlBasicAttribute));
	*new.basicAttribute = *ptr.basicAttribute;
	break;

      case DL_DynamicAttribute:
	new.dynamicAttribute = (DlDynamicAttribute*)
			malloc(sizeof(DlDynamicAttribute));
	*new.dynamicAttribute = *ptr.dynamicAttribute;
	break;

      case DL_Rectangle:
	new.rectangle = (DlRectangle *)malloc(sizeof(DlRectangle));
	*new.rectangle = *ptr.rectangle;
	break;

      case DL_Oval:
	new.oval = (DlOval *)malloc(sizeof(DlOval));
	*new.oval = *ptr.oval;
	break;

      case DL_Arc:
	new.arc = (DlArc *)malloc(sizeof(DlArc));
	*new.arc = *ptr.arc;
	break;

      case DL_FallingLine:
	new.fallingLine = (DlFallingLine *)malloc(sizeof(DlFallingLine));
	*new.fallingLine = *ptr.fallingLine;
	break;

      case DL_RisingLine:
	new.risingLine = (DlRisingLine *)malloc(sizeof(DlRisingLine));
	*new.risingLine = *ptr.risingLine;
	break;

      case DL_Text:
	new.text = (DlText *)malloc(sizeof(DlText));
	*new.text = *ptr.text;
	break;

      case DL_RelatedDisplay:
	new.relatedDisplay = (DlRelatedDisplay *)malloc(
				sizeof(DlRelatedDisplay));
	*new.relatedDisplay = *ptr.relatedDisplay;
	break;

      case DL_ShellCommand:
	new.shellCommand = (DlShellCommand *)malloc(
				sizeof(DlShellCommand));
	*new.shellCommand = *ptr.shellCommand;
	break;

      case DL_TextUpdate:
	new.textUpdate = (DlTextUpdate *)malloc(sizeof(DlTextUpdate));
	*new.textUpdate = *ptr.textUpdate;
	break;

      case DL_Indicator:
	new.indicator = (DlIndicator *)malloc(sizeof(DlIndicator));
	*new.indicator = *ptr.indicator;
	break;
	
      case DL_Meter:
	new.meter = (DlMeter *)malloc(sizeof(DlMeter));
	*new.meter = *ptr.meter;
	break;

      case DL_Byte:
        new.byte = (DlByte *)malloc(sizeof(DlByte));
        *new.byte = *ptr.byte;
        break;
      
      case DL_Bar:
	new.bar = (DlBar *)malloc(sizeof(DlBar));
	*new.bar = *ptr.bar;
	break;
      
	
      case DL_SurfacePlot:
	new.surfacePlot = (DlSurfacePlot *)malloc(sizeof(DlSurfacePlot));
	*new.surfacePlot = *ptr.surfacePlot;
	break;
	
      case DL_StripChart:
	new.stripChart = (DlStripChart *)malloc(sizeof(DlStripChart));
	*new.stripChart = *ptr.stripChart;
	break;
	
      case DL_CartesianPlot:
	new.cartesianPlot = (DlCartesianPlot *)malloc(sizeof(DlCartesianPlot));
	*new.cartesianPlot = *ptr.cartesianPlot;
	break;
	
      case DL_Valuator:
	new.valuator = (DlValuator *)malloc(sizeof(DlValuator));
	*new.valuator = *ptr.valuator;
	break;
	
      case DL_ChoiceButton:
	new.choiceButton = (DlChoiceButton *)malloc(sizeof(DlChoiceButton));
	*new.choiceButton = *ptr.choiceButton;
	break;
	
      case DL_MessageButton:
	new.messageButton = (DlMessageButton *)malloc(sizeof(DlMessageButton));
	*new.messageButton = *ptr.messageButton;
	break;
	
      case DL_Menu:
	new.menu = (DlMenu *)malloc(sizeof(DlMenu));
	*new.menu = *ptr.menu;
	break;
	
      case DL_TextEntry:
	new.textEntry = (DlTextEntry *)malloc(sizeof(DlTextEntry));
	*new.textEntry = *ptr.textEntry;
	break;
	
      case DL_Image:
	new.image = (DlImage *)malloc(sizeof(DlImage));
	*new.image = *ptr.image;
	break;
	
      case DL_Composite:
/*
 * this one does a lot of work - including (recursive) copying/creation of
 *  composite children display list
 */
	new.composite= (DlComposite *)malloc(sizeof(DlComposite));
	*new.composite= *ptr.composite;
	 new.composite->dlElementListHead  = (XtPointer)calloc((size_t)1,
		sizeof(DlElement));
	((DlElement *)(new.composite->dlElementListHead))->next = NULL;
	new.composite->dlElementListTail = new.composite->dlElementListHead;
	child = ((DlElement *)ptr.composite->dlElementListHead)->next;
	while (child != NULL) {
	  structurePtr = createCopyOfElementType(child->type,child->structure);
/* any old element of the union */
	  if (structurePtr.rectangle != NULL) {
/* add the element to composite's display list */
	    elementPtr = (DlElement *) malloc(sizeof(DlElement));
	    elementPtr->type = child->type;
	    elementPtr->structure = structurePtr;
	    elementPtr->dmExecute = child->dmExecute;
	    elementPtr->dmWrite = child->dmWrite;
	    if (((DlElement *)(new.composite->dlElementListHead))->next
			== NULL) {
		((DlElement *)(new.composite->dlElementListHead))->next =
			elementPtr;
	    }
	    elementPtr->prev = (DlElement *)new.composite->dlElementListTail;
	    ((DlElement *)new.composite->dlElementListTail)->next = elementPtr;
	    elementPtr->next = NULL;
	    new.composite->dlElementListTail = (XtPointer)elementPtr;
	  }
	  child = child->next;
	}
	break;


      case DL_Polyline:
	new.polyline = (DlPolyline *)malloc(sizeof(DlPolyline));
	*new.polyline= *ptr.polyline;
	new.polyline->points = (XPoint *)malloc(ptr.polyline->nPoints
							*sizeof(XPoint));
	for (i = 0; i < ptr.polyline->nPoints; i++) {
	    new.polyline->points[i] = ptr.polyline->points[i];
	}
	break;

      case DL_Polygon:
	new.polygon = (DlPolygon *)malloc(sizeof(DlPolygon));
	*new.polygon= *ptr.polygon;
	new.polygon->points = (XPoint *)malloc(ptr.polygon->nPoints
							*sizeof(XPoint));
	for (i = 0; i < ptr.polygon->nPoints; i++) {
	    new.polygon->points[i] = ptr.polygon->points[i];
	}
	break;

	
      default:
	fprintf(stderr,"\ncreateCopyOfElementType(type=%d) missed type!",type);
	new.file = (DlFile *)NULL;
  }
  return(new);
}



void copyElementsIntoDisplay()
{
  int i, j;
  DisplayInfo *cdi;
  Position displayX, displayY;
  int offsetX, offsetY;
  DlElement *elementPtr, *moveTo, *newElementsListHead;
  DlElement *ele, *child;
  DlStructurePtr structurePtr;
  Dimension w, h;
  int deltaX, deltaY;
  Boolean moveWidgets;

  int counter, arraySize, numElements, numSelected;
  DlElement *element, **array;
  DisplayInfo *displayInfo;

/*
 * since elements are stored in clipboard in front-to-back order
 *	they can be pasted/copied into display in clipboard index order
 */
/* MDA -  since doPasting() can change currentDisplayInfo,
   clear old highlights now */
  displayInfo = displayInfoListHead->next;
  while (displayInfo != NULL) {
    currentDisplayInfo = displayInfo;
    unselectElementsInDisplay();
    displayInfo = displayInfo->next;
  }

  cdi = doPasting(&displayX,&displayY,&offsetX,&offsetY);
  deltaX = displayX + offsetX;
  deltaY = displayY + offsetY;

  if (cdi != NULL) {
  /* make sure pasted window is on top and has focus (and updated status) */
    XRaiseWindow(display,XtWindow(cdi->shell));
    XSetInputFocus(display,XtWindow(cdi->shell),RevertToParent,CurrentTime);
    currentDisplayInfo = cdi;
  } else {
    fprintf(stderr,"\ncopyElementsIntoDisplay:  can't discern current display");
    return;
  }


/***
 *** now do actual element creation (with insertion into display list)
 ***/


  newElementsListHead = cdi->dlElementListTail;

  for (i = 0; i < numClipboardElements; i++) {

    if (!clipboardDelete) {

/* elements are not in clipboard due to cut/delete, therefore create copy */
      structurePtr = createCopyOfElementType(
	  clipboardElementsArray[i].type,clipboardElementsArray[i].structure);

    } else {

/* elements ARE in clipboard due to cut/delete, therefore reuse structure */

      structurePtr = clipboardElementsArray[i].structure;

    }

/* any old element of the union */
    if (structurePtr.rectangle != NULL) {

/* add the element to the display list */
      elementPtr = (DlElement *) malloc(sizeof(DlElement));
      elementPtr->type = clipboardElementsArray[i].type;
      elementPtr->structure = structurePtr;
      elementPtr->dmExecute = clipboardElementsArray[i].dmExecute;
      elementPtr->dmWrite = clipboardElementsArray[i].dmWrite;

      elementPtr->prev = cdi->dlElementListTail;
      cdi->dlElementListTail->next = elementPtr;
      elementPtr->next = NULL;
      cdi->dlElementListTail = elementPtr;

/* update positions of all renderable objects */
      if (ELEMENT_IS_RENDERABLE(elementPtr->type)) {
	if (elementPtr->type == DL_Composite) {
	  moveWidgets = False;	/* only update positions */
	  moveCompositeChildren(cdi, elementPtr, deltaX, deltaY, moveWidgets);
	} else if (elementPtr->type == DL_Polyline) {
	  for (j = 0; j < structurePtr.polyline->nPoints; j++) {
	     structurePtr.polyline->points[j].x += deltaX;
	     structurePtr.polyline->points[j].y += deltaY;
	  }
	} else if (elementPtr->type == DL_Polygon) {
	  for (j = 0; j < structurePtr.polygon->nPoints; j++) {
	     structurePtr.polygon->points[j].x += deltaX;
	     structurePtr.polygon->points[j].y += deltaY;
	  }
	}
        structurePtr.rectangle->object.x += deltaX;
        structurePtr.rectangle->object.y += deltaY;
      }

/* execute the structure */
      (elementPtr->dmExecute)(cdi,structurePtr.rectangle,FALSE);


    }
  } /* end for */


  clipboardDelete = False;


/* now make pasted elements the currently selected elements */

/* one pass to see how many */
  counter = 0;
  element = newElementsListHead->next;
  while (element != NULL) {
    if (ELEMENT_IS_RENDERABLE(element->type) && element->type != DL_Display) {
	counter++;
    }
    element = element->next;
  }
  array = (DlElement **) malloc(counter*sizeof(DlElement *));
  arraySize = counter;
  numElements = counter;
  element = newElementsListHead->next;

/* second pass to copy pointers into array */
  counter = 0;
  while (element != NULL) {
    if (ELEMENT_IS_RENDERABLE(element->type) && element->type != DL_Display ) {
	array[counter] = element;
	counter++;
    }
    element = element->next;
  }
  numSelected = highlightAndSetSelectedElements(array,arraySize,
			   numElements);
  if (numElements == 1) setResourcePaletteEntries();

}



void deleteElementsInDisplay()
{
  int i;
  DisplayInfo *cdi;
  DlElement *elementPtr, *dyn;

  if (currentDisplayInfo == NULL) return;
  if (currentDisplayInfo->numSelectedElements == 0) return;
  cdi = currentDisplayInfo;


  for (i = 0; i < cdi->numSelectedElements; i++) {

    elementPtr = cdi->selectedElementsArray[i];

    if ( ELEMENT_IS_RENDERABLE(elementPtr->type) &&
			elementPtr->type != DL_Display) {
    /* don't allow user to delete the display! */

      if (elementPtr->type != DL_Composite) {
      /* first look to see if a dynamic attribute exists for this element */
       dyn = lookupDynamicAttributeElement(elementPtr);
       if (dyn != NULL) {
      /* found a dynamicAttribute - delete it, and then its operand element */
         if (dyn->prev != NULL) (dyn->prev)->next = dyn->next;
         if (dyn->next != NULL)
	    (dyn->next)->prev = dyn->prev;
	 else /* must be at tail - update tail */
	    cdi->dlElementListTail = dyn->prev;
       }
      }
      /* now delete the selected element */
      if (elementPtr->prev !=NULL) (elementPtr->prev)->next = elementPtr->next;
      if (elementPtr->next != NULL)
	   (elementPtr->next)->prev = elementPtr->prev;
	 else /* must be at tail - update tail */
	    cdi->dlElementListTail = elementPtr->prev;

      /* if composite, delete any widget children */
      if (elementPtr->type == DL_Composite) {
	  deleteWidgetsInComposite(cdi,elementPtr);
      }

/*
 * set this flag to true - next clearClipboard call can actually 
 *	free the display list elements in memory (elements have actually
 *	been deleted)
 */
      clipboardDelete = True;

      if (ELEMENT_HAS_WIDGET(elementPtr->type)) {
       /* lookup widget of specified x,y,width,height and destroy */
	 destroyElementWidget(cdi,
	     lookupElementWidget(cdi,
			&(elementPtr->structure.rectangle->object)));
      }
    }
  }

  /* unhighlight, unselect,  and clear resource palette */
  unhighlightSelectedElements();
  unselectSelectedElements();
  clearResourcePaletteEntries();



  /* (MDA) could use a new element-lookup based on region (write routine
   *      which returns all elements which intersect rather than are
   *      bounded by a given region) and do partial traversal based on
   *      those elements in start and end regions.  this could be much
   *      more efficient and not suffer from the "flash" updates
   */
   dmTraverseNonWidgetsInDisplayList(cdi);

}


void unselectElementsInDisplay()
{
  int i;
  DisplayInfo *cdi;

  if (currentDisplayInfo == NULL) return;
  if (currentDisplayInfo->numSelectedElements == 0) return;
  cdi = currentDisplayInfo;
  /* unhighlight and clear resource palette */
  unhighlightSelectedElements();
  unselectSelectedElements();
  clearResourcePaletteEntries();

}


/*
 * select all renderable objects in display - note that this excludes the
 *	display itself
 */

void selectAllElementsInDisplay()
{
  int i;
  DisplayInfo *cdi;
  Position x, y;
  Dimension width, height;
  DlElement **array, *element;
  int numSelected, numElements, arraySize;
  int counter;

  if (currentDisplayInfo == NULL) return;
  cdi = currentDisplayInfo;
  clearClipboard();

  /* unhighlight and clear resource palette */
  unhighlightSelectedElements();
  unselectSelectedElements();
  clearResourcePaletteEntries();

  XtVaGetValues(currentDisplayInfo->drawingArea,XmNx,&x,XmNy,&y,
	XmNwidth,&width,XmNheight,&height,NULL);


/* one pass to see how many */
  counter = 0;
  element = ((DlElement *)currentDisplayInfo->dlElementListHead)->next;
  while (element != NULL) {
    if (ELEMENT_IS_RENDERABLE(element->type) && element->type != DL_Display) {
	counter++;
    }
    element = element->next;
  }
  array = (DlElement **) malloc(counter*sizeof(DlElement *));
  arraySize = counter;
  numElements = counter;
  element = ((DlElement *)currentDisplayInfo->dlElementListHead)->next;

/* second pass to copy pointers into array */
  counter = 0;
  while (element != NULL) {
    if (ELEMENT_IS_RENDERABLE(element->type) && element->type != DL_Display ) {
	array[counter] = element;
	counter++;
    }
    element = element->next;
  }

  numSelected = highlightAndSetSelectedElements(array,arraySize,
			   numElements);
}




/*
 * move elements further up (traversed first) in display list
 *  so that these are "behind" other objects
 */
void lowerSelectedElements()
{

  moveSelectedElementsAfterElement(currentDisplayInfo,
				currentDisplayInfo->dlColormapElement);

  /* unhighlight and clear resource palette */
  unhighlightSelectedElements();
  unselectSelectedElements();
  clearResourcePaletteEntries();

  dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
}



/*
 * move elements further down (traversed last) in display list
 *  so that these are "in front of" other objects
 */
void raiseSelectedElements()
{

  moveSelectedElementsAfterElement(currentDisplayInfo,
			currentDisplayInfo->dlElementListTail);

  /* unhighlight and clear resource palette */
  unhighlightSelectedElements();
  unselectSelectedElements();
  clearResourcePaletteEntries();

  dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
}



/*
 * ungroup any grouped (composite) elements which are currently selected.
 *  this removes the appropriate Composite element and moves any children
 *  to reflect their new-found autonomy...
 */
void ungroupSelectedElements()
{
  DisplayInfo *cdi;
  DlElement *ele, *child, *nextChild, *dyn, *composite;
  int i, id;


  /* unhighlight */
  unhighlightSelectedElements();

  cdi = currentDisplayInfo;
  for (i = 0; i < cdi->numSelectedElements; i++) {
    ele = cdi->selectedElementsArray[i];
    if (ele->type == DL_Composite) {
      child = ((DlElement *)ele->structure.composite->dlElementListHead)->next;

/* should be simple - tie first child in and last child in */

      ele->prev->next = child;
      child->prev = ele->prev;
      ((DlElement *)ele->structure.composite->dlElementListTail)->next =
	  ele->next;
      if (ele->next != NULL)
	ele->next->prev = (DlElement *)
			ele->structure.composite->dlElementListTail;
      else
	cdi->dlElementListTail = (DlElement *)
			ele->structure.composite->dlElementListTail;

      free ((char *) ele->structure.composite->dlElementListHead);
      free ((char *) ele->structure.composite);
      free ((char *) ele);
    }
  }


  /* unselect and clear resource palette */
  unselectSelectedElements();
  clearResourcePaletteEntries();

  dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
}




/*
 * align selected elements by top, bottom, left, or right edges
 */
void alignSelectedElements(
  int alignment)
{
  int i, j, minX, minY, maxX, maxY, deltaX, deltaY, x0, y0, xOffset, yOffset;
  DisplayInfo *cdi;
  Widget widget;
  DlElement *ele;

  if (currentDisplayInfo == NULL) return;
  if (currentDisplayInfo->numSelectedElements == 0) return;
  cdi = currentDisplayInfo;

  minX = INT_MAX; minY = INT_MAX;
  maxX = INT_MIN; maxY = INT_MIN;

  unhighlightSelectedElements();

/* loop and get min/max (x,y) values */
  for (i = cdi->numSelectedElements - 1; i >= 0; i--) {
      ele = cdi->selectedElementsArray[i];
      minX = MIN(minX, ele->structure.rectangle->object.x);
      minY = MIN(minY, ele->structure.rectangle->object.y);
      x0 = (ele->structure.rectangle->object.x
		+ ele->structure.rectangle->object.width);
      maxX = MAX(maxX,x0);
      y0 = (ele->structure.rectangle->object.y
		+ ele->structure.rectangle->object.height);
      maxY = MAX(maxY,y0);
  }
  deltaX = (minX + maxX)/2;
  deltaY = (minY + maxY)/2;

/* loop and set x,y values, and move if widgets */
  for (i = cdi->numSelectedElements - 1; i >= 0; i--) {
 
    ele = cdi->selectedElementsArray[i];

  /* can't move the display */
    if (ele->type != DL_Display) {

     switch(alignment) {
      case HORIZ_LEFT:
	widget = NULL;
	if (ELEMENT_HAS_WIDGET(ele->type)) widget = lookupElementWidget(cdi,
                &(ele->structure.rectangle->object));
	xOffset = minX - ele->structure.rectangle->object.x;
	if (ele->type == DL_Composite) {
	    moveCompositeChildren(cdi,ele,xOffset,0,True);
	} else if (ele->type == DL_Polygon) {
	    for (j = 0; j < ele->structure.polygon->nPoints; j++) {
		ele->structure.polygon->points[j].x += xOffset;
	    }
	} else if (ele->type == DL_Polyline) {
	    for (j = 0; j < ele->structure.polyline->nPoints; j++) {
		ele->structure.polyline->points[j].x += xOffset;
	    }
	}
	ele->structure.rectangle->object.x = minX; 
	if (widget != NULL) XtMoveWidget(widget,
		(Position) ele->structure.rectangle->object.x,
		(Position) ele->structure.rectangle->object.y);
	break;

      case HORIZ_CENTER:
/* want   x + w/2 = dX  , therefore   x = dX - w/2   */
	widget = NULL;
	if (ELEMENT_HAS_WIDGET(ele->type))
	    widget = lookupElementWidget(cdi,
		&(ele->structure.rectangle->object));
	xOffset = (deltaX - ele->structure.rectangle->object.width/2)
			- ele->structure.rectangle->object.x;
	if (ele->type == DL_Composite) {
	    moveCompositeChildren(cdi,ele,xOffset,0,True);
	} else if (ele->type == DL_Polygon) {
	    for (j = 0; j < ele->structure.polygon->nPoints; j++) {
		ele->structure.polygon->points[j].x += xOffset;
	    }
	} else if (ele->type == DL_Polyline) {
	    for (j = 0; j < ele->structure.polyline->nPoints; j++) {
		ele->structure.polyline->points[j].x += xOffset;
	    }
	}
	ele->structure.rectangle->object.x = (deltaX -
		ele->structure.rectangle->object.width/2);
	if (widget != NULL) XtMoveWidget(widget,
		(Position) ele->structure.rectangle->object.x,
		(Position) ele->structure.rectangle->object.y);
	break;

      case HORIZ_RIGHT:
/* want   x + w = maxX  , therefore   x = maxX - w  */
	widget = NULL;
	if (ELEMENT_HAS_WIDGET(ele->type)) widget = lookupElementWidget(cdi,
			&(ele->structure.rectangle->object));
	xOffset = (maxX - ele->structure.rectangle->object.width)
			- ele->structure.rectangle->object.x;
	if (ele->type == DL_Composite) {
	    moveCompositeChildren(cdi,ele,xOffset,0,True);
	} else if (ele->type == DL_Polygon) {
	    for (j = 0; j < ele->structure.polygon->nPoints; j++) {
		ele->structure.polygon->points[j].x += xOffset;
	    }
	} else if (ele->type == DL_Polyline) {
	    for (j = 0; j < ele->structure.polyline->nPoints; j++) {
		ele->structure.polyline->points[j].x += xOffset;
	    }
	}
	ele->structure.rectangle->object.x = (maxX -
		ele->structure.rectangle->object.width);
	if (widget != NULL) XtMoveWidget(widget,
		(Position) ele->structure.rectangle->object.x,
		(Position) ele->structure.rectangle->object.y);
	break;

      case VERT_TOP:
	widget = NULL;
	if (ELEMENT_HAS_WIDGET(ele->type))
	    widget = lookupElementWidget(cdi,
                &(ele->structure.rectangle->object));
	yOffset = minY - ele->structure.rectangle->object.y;
	if (ele->type == DL_Composite) {
	    moveCompositeChildren(cdi,ele,0,yOffset,True);
	} else if (ele->type == DL_Polygon) {
	    for (j = 0; j < ele->structure.polygon->nPoints; j++) {
		ele->structure.polygon->points[j].y += yOffset;
	    }
	} else if (ele->type == DL_Polyline) {
	    for (j = 0; j < ele->structure.polyline->nPoints; j++) {
		ele->structure.polyline->points[j].y += yOffset;
	    }
	}
	ele->structure.rectangle->object.y += 
		(minY - ele->structure.rectangle->object.y);
	if (widget != NULL) XtMoveWidget(widget,
		(Position) ele->structure.rectangle->object.x,
		(Position) ele->structure.rectangle->object.y);
	break;

      case VERT_CENTER:
/* want   y + h/2 = dY  , therefore   y = dY - h/2   */
	widget = NULL;
	if (ELEMENT_HAS_WIDGET(ele->type))
	    widget = lookupElementWidget(cdi,
                &(ele->structure.rectangle->object));
	yOffset = (deltaY - ele->structure.rectangle->object.height/2)
			- ele->structure.rectangle->object.y;
	if (ele->type == DL_Composite) {
	    moveCompositeChildren(cdi,ele,0,yOffset,True);
	} else if (ele->type == DL_Polygon) {
	    for (j = 0; j < ele->structure.polygon->nPoints; j++) {
		ele->structure.polygon->points[j].y += yOffset;
	    }
	} else if (ele->type == DL_Polyline) {
	    for (j = 0; j < ele->structure.polyline->nPoints; j++) {
		ele->structure.polyline->points[j].y += yOffset;
	    }
	}
	ele->structure.rectangle->object.y = (deltaY -
		ele->structure.rectangle->object.height/2);
	if (widget != NULL) XtMoveWidget(widget,
		(Position) ele->structure.rectangle->object.x,
		(Position) ele->structure.rectangle->object.y);
	break;

      case VERT_BOTTOM:
/* want   y + h = maxY  , therefore   y = maxY - h  */
	widget = NULL;
	if (ELEMENT_HAS_WIDGET(ele->type))
	    widget = lookupElementWidget(cdi,
                &(ele->structure.rectangle->object));
	yOffset = (maxY - ele->structure.rectangle->object.height)
			- ele->structure.rectangle->object.y;
	if (ele->type == DL_Composite) {
	    moveCompositeChildren(cdi,ele,0,yOffset,True);
	} else if (ele->type == DL_Polygon) {
	    for (j = 0; j < ele->structure.polygon->nPoints; j++) {
		ele->structure.polygon->points[j].y += yOffset;
	    }
	} else if (ele->type == DL_Polyline) {
	    for (j = 0; j < ele->structure.polyline->nPoints; j++) {
		ele->structure.polyline->points[j].y += yOffset;
	    }
	}
	ele->structure.rectangle->object.y = (maxY -
		ele->structure.rectangle->object.height);
	if (widget != NULL) XtMoveWidget(widget,
		(Position) ele->structure.rectangle->object.x,
		(Position) ele->structure.rectangle->object.y);
	break;
    }
   }
  }


/* retraverse all non-widgets (since potential window damage can result from
 * the movement of objects) */
  dmTraverseNonWidgetsInDisplayList(cdi);

  highlightSelectedElements();

}





/*
 * moves specified <src> element to position just after specified <dst> element
 */
void moveElementAfter(
  DisplayInfo *cdi,
  DlComposite *dlComposite, /* for updating tail ptr. if adding to composite */
  DlElement *src,
  DlElement *dst)
{
  DlStructurePtr structure;
  DlElement *element;

  if (cdi == NULL || src == NULL || dst == NULL) return;


  if (src->type == DL_BasicAttribute) {
/*
 * copy Basic Attributes (since following elements may depend on it)
 *	N.B.: this is like createDlBasicAttribute, except this routine
 *	should not create based on the globalResourceBundle...
 */

 /* allocate a display element in memory and copy the pointed-to data into it */
     element = (DlElement *) malloc(sizeof(DlElement));
     structure = createCopyOfElementType(src->type,src->structure);
     element->structure = structure;
     element->type = DL_BasicAttribute;
     element->dmExecute = (void(*)())executeDlBasicAttribute;
     element->dmWrite = (void(*)())writeDlBasicAttribute;

 /* now add to the display list (insert in list, update tail if necessary */
     element->prev = dst;
     if (dst->next != NULL) {
       dst->next->prev = element;
     } else {
       if (dlComposite == NULL) {
     /* must be adding to end or tail of display list */
	cdi->dlElementListTail = element;
       } else {
     /* must be adding to end or tail of composite */
	dlComposite->dlElementListTail = (XtPointer)element;
       }
     }
     element->next = dst->next;
     dst->next = element;


  } else {

/*
 * simple move of element
 */

     /* tie up hole where src is being moved from */
     src->prev->next = src->next;
     if (src->next != NULL) {
       src->next->prev = src->prev;
     } else {
     /* must be at tail, modify tail ptr. (effectively removed one from end) */
       cdi->dlElementListTail =  src->prev;
     }
     src->prev = dst;
     src->next = dst->next;

     /* now tidy up dst */
     if (dst->next != NULL) {
       dst->next->prev = src;
     } else {
       if (dlComposite == NULL) {
     /* must be adding to end or tail of display list */
	cdi->dlElementListTail = src;
       } else {
     /* must be adding to end or tail of composite */
	dlComposite->dlElementListTail = (XtPointer)src;
       }
     }
     dst->next = src;
  }

}



/*
 * move all selected elements to position after specified element
 *	N.B.: this can move them up or down in relative visibility
 *	("stacking order" a la painter's algorithm)
 */
void  moveSelectedElementsAfterElement(
  DisplayInfo *displayInfo,
  DlElement *afterThisElement)
{
  int i;
  DisplayInfo *cdi;
  DlElement *dyn, *basic, *afterElement;

  if (displayInfo == NULL) return;
  if (displayInfo->numSelectedElements == 0) return;
  cdi = displayInfo;
  afterElement = afterThisElement;


  for (i = cdi->numSelectedElements - 1; i >= 0; i--) {

/* if display was selected, skip over it (can't raise/lower it) */
    if (cdi->selectedElementsArray[i]->type != DL_Display) {

     if (cdi->selectedElementsArray[i]->type != DL_Composite) {

/* -- careful about order: dynamic attribute must immediately precede object */
 /* basic attribute data */
       basic = lookupBasicAttributeElement(cdi->selectedElementsArray[i]);
       if (basic != NULL) {
        moveElementAfter(cdi,NULL,basic,afterElement);
        afterElement = afterElement->next;
       }

 /* dynamic attribute data (if any) */
       dyn = lookupDynamicAttributeElement(cdi->selectedElementsArray[i]);
       if (dyn != NULL) {
        moveElementAfter(cdi,NULL,dyn,afterElement);
        afterElement = afterElement->next;
       }
     }

 /* actual elements */
     moveElementAfter(cdi,NULL,cdi->selectedElementsArray[i],afterElement);
     afterElement = afterElement->next;

    }
  }

}


/*
 * function to remove an element from a display list, free associated
 *	memory (where applicable) and update the displayInfo structure
 */
void deleteAndFreeElementAndStructure(
  DisplayInfo *displayInfo,
  DlElement *ele)
{
  if (displayInfo != NULL && ele != NULL) {

    if (ele->prev != NULL) ele->prev->next = ele->next;
    if (ele->next != NULL) ele->next->prev = ele->prev;
/* assuming we won't use this for deleting the head of the list (DL_Display) */
    if (currentDisplayInfo->dlElementListTail == ele)
			currentDisplayInfo->dlElementListTail = ele->prev;
    if (ELEMENT_HAS_WIDGET(ele->type)) {
	destroyElementWidget(currentDisplayInfo,
		lookupElementWidget(currentDisplayInfo,
			&(ele->structure.rectangle->object)));
    }

  /* free structure memory */
    if (ele->structure.file != NULL) free ((char *) ele->structure.file);

    free ((char *) ele);
  }

}


/*
 * return Channel ptr given a widget id
 */
Channel *dmGetChannelFromWidget(
  Widget sourceWidget)
{
  DisplayInfo *displayInfo;
  DlElement *ele, *targetEle;
  Channel *mData;
 
  mData = channelAccessMonitorListHead->next;
  while (mData != NULL) {
     if (mData->self == sourceWidget) {
	return mData;
     }
     mData = mData->next;
  }
  return ((Channel *)NULL);

}


/*
 * return Channel ptr given a DisplayInfo* and x,y positions
 */
Channel *dmGetChannelFromPosition(
  DisplayInfo *displayInfo,
  int x,
  int y)
{
  Channel *mData, *saveMonitorData;
  DlRectangle *dlRectangle;
  int minWidth, minHeight;
  
  saveMonitorData = (Channel *)NULL;
  if (displayInfo == (DisplayInfo *)NULL) return(saveMonitorData);

  minWidth = INT_MAX;	 	/* according to XPG2's values.h */
  minHeight = INT_MAX;

  mData = channelAccessMonitorListHead->next;
  while (mData != NULL) {
    if (mData->displayInfo == displayInfo) {
/* as long as OBJECT is the first part of any structure, this works... */
      dlRectangle = (DlRectangle *)mData->specifics;
      if (x >= dlRectangle->object.x &&
	 x <= dlRectangle->object.x + dlRectangle->object.width &&
	 y >= dlRectangle->object.y &&
	 y <= dlRectangle->object.y + dlRectangle->object.height) {
  /* eligible element, see if smallest so far */
        if (dlRectangle->object.width < minWidth &&
	   dlRectangle->object.height < minHeight) {
	   minWidth = dlRectangle->object.width;
	   minHeight = dlRectangle->object.height;
	   saveMonitorData = mData;
        }
      }
    }
    mData = mData->next;
  }
  return (saveMonitorData);

}



/*
 * generate a name-value table from the passed-in argument string
 *	returns a pointer to a NameValueTable as the function value,
 *	and the number of values in the second parameter
 *	Syntax: argsString: "a=b,c=d,..."
 */

NameValueTable *generateNameValueTable(
  char *argsString,
  int *numNameValues)
{
 char *copyOfArgsString,  *name, *value;
 char *s1;
 char nameEntry[80], valueEntry[80];
 int i, j, k, tableIndex, numPairs, numEntries;
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
      for (i = 0; i < strlen(name); i++) {
	if (!isspace(name[i]))
	   nameEntry[j++] =  name[i];
      }
      nameEntry[j] = '\0';
      j = 0;
      for (i = 0; i < strlen(value); i++) {
	if (!isspace(value[i]))
	   valueEntry[j++] =  value[i];
      }
      valueEntry[j] = '\0';
      nameTable[tableIndex].name = STRDUP(nameEntry);
      nameTable[tableIndex].value = STRDUP(valueEntry);
      tableIndex++;
     }
   }

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

char *lookupNameValue(
  NameValueTable *nameValueTable,
  int numEntries,
  char *name)
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

void freeNameValueTable(
  NameValueTable *nameValueTable,
  int numEntries)
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
  if (displayInfo == NULL) {
    strncpy(outputString,inputString,sizeOfOutputString-1);
    outputString[sizeOfOutputString-1] = '\0';
    return;
  }


  i = 0; j = 0; k = 0;
  if (inputString != NULL && strlen(inputString) > 0) {
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
	  /* now lookup macro */
	   value = lookupNameValue(displayInfo->nameValueTable,
		displayInfo->numNameValues,name);
	   n = 0;
	   while (value[n] != '\0' && j < sizeOfOutputString-1)
		outputString[j++] = value[n++];
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
  if (j >= sizeOfOutputString-1) fprintf(stderr,"\n%s%s",
	" performMacroSubstitutions: substitutions failed",
	" - output buffer not large enough");
}



/*
 * colorMenuBar - get VUE and its "ColorSetId" straightened out...
 *   color the passed in widget (usually menu bar) and its children
 *   to the specified foreground/background colors
 */
void colorMenuBar(
  Widget widget,
  Pixel fg,
  Pixel bg)
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


void questionDialogCb(Widget widget, 
  XtPointer clientData,
  XtPointer callbackStruct)
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
  XtCallbackProc cancel, ok, help;
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
}

void warningDialogCb(Widget widget,
                     XtPointer clientData,
                     XtPointer callbackStruct)
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
  XtCallbackProc cancel, ok, help;
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
  XtUnmanageChild(displayInfo->warningDialog);
}

