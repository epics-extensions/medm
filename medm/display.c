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

#define DEBUG_RELATED_DISPLAY 0

#include "medm.h"

#include <X11/keysym.h>
#include <Xm/MwmUtil.h>

extern Widget mainShell;

static Position x = 0, y = 0;
static Widget lastShell;

/* KE: Used to move the display if its x and y are zero
 * The following account for the borders and title bar with the TED WM
 *   Were formerly both 10 (not very aesthetic) */
#define DISPLAY_DEFAULT_X 5
#define DISPLAY_DEFAULT_Y 24

#define MEDM_EXEC_LIST_MAX 1024

typedef DlElement *(*medmParseFunc)(DisplayInfo *);
typedef struct {
    char *name;
    medmParseFunc func;
} ParseFuncEntry;

typedef struct _parseFuncEntryNode {
    ParseFuncEntry *entry;
    struct _parseFuncEntryNode *next;
    struct _parseFuncEntryNode *prev;
} ParseFuncEntryNode;

/* Function prototypes */
static void displayShellPopdownCallback(Widget shell, XtPointer, XtPointer);
static void displayShellPopupCallback(Widget shell, XtPointer, XtPointer);

/* Global variables */

ParseFuncEntry parseFuncTable[] = {
    {"rectangle",            parseRectangle},
    {"oval",                 parseOval},
    {"arc",                  parseArc},
    {"text",                 parseText},
/*     {"falling line",         parseFallingLine}, */
/*     {"rising line",          parseRisingLine}, */
    {"falling line",         parsePolyline},
    {"rising line",          parsePolyline},
    {"related display",      parseRelatedDisplay},
    {"shell command",        parseShellCommand},
    {"bar",                  parseBar},
    {"indicator",            parseIndicator},
    {"meter",                parseMeter},
    {"byte",                 parseByte},
    {"strip chart",          parseStripChart},
#ifdef CARTESIAN_PLOT
    {"cartesian plot",       parseCartesianPlot},
#endif     /* #ifdef CARTESIAN_PLOT */
    {"text update",          parseTextUpdate},
    {"choice button",        parseChoiceButton},
    {"button",               parseChoiceButton},
    {"message button",       parseMessageButton},
    {"menu",                 parseMenu},
    {"text entry",           parseTextEntry},
    {"valuator",             parseValuator},
    {"image",                parseImage},
    {"composite",            parseComposite},
    {"polyline",             parsePolyline},
    {"polygon",              parsePolygon},
};

static int parseFuncTableSize = sizeof(parseFuncTable)/sizeof(ParseFuncEntry);

/* DEBUG */
#if 0
DisplayInfo *debugDisplayInfo=NULL;
#endif
/* End DEBUG */

/***  displayInfo routines ***/

/*
 * create and return a DisplayInfo structure pointer on tail of displayInfoList
 *  this includes a shell (with it's dialogs and event handlers)
 */
DisplayInfo *allocateDisplayInfo()
{
    DisplayInfo *displayInfo;
    int nargs;
    Arg args[8];

/* Allocate a DisplayInfo structure and shell for this display file/list */
    displayInfo = (DisplayInfo *)malloc(sizeof(DisplayInfo));
    if(!displayInfo) return NULL;

    displayInfo->dlElementList = createDlList();
    if(!displayInfo->dlElementList) {
	free(displayInfo);
	return NULL;
    }

    displayInfo->selectedDlElementList = createDlList();
    if(!displayInfo->selectedDlElementList) {
	free(displayInfo->dlElementList);
	free(displayInfo);
	return NULL;
    }
    displayInfo->selectedElementsAreHighlighted = False;

    displayInfo->filePtr = NULL;
    displayInfo->newDisplay = True;
    displayInfo->elementsExecuted = False;
    displayInfo->versionNumber = 0;

    displayInfo->drawingArea = 0;
    displayInfo->drawingAreaPixmap = 0;
    displayInfo->updatePixmap = 0;
    displayInfo->cartesianPlotPopupMenu = 0;
    displayInfo->selectedCartesianPlot = 0;
    displayInfo->warningDialog = NULL;
    displayInfo->warningDialogAnswer = 0;
    displayInfo->questionDialog = NULL;
    displayInfo->questionDialogAnswer = 0;
    displayInfo->shellCommandPromptD = NULL;

    displayInfo->grid = NULL;
    displayInfo->undoInfo = NULL;

    displayInfo->markerWidgetList = NULL;
    displayInfo->nMarkerWidgets = 0;    

    updateTaskInitHead(displayInfo);

#if 0
    displayInfo->childCount = 0;
#endif

    displayInfo->colormap = 0;
    displayInfo->dlColormapCounter = 0;
    displayInfo->dlColormapSize = 0;
    displayInfo->drawingAreaBackgroundColor = globalResourceBundle.bclr;
    displayInfo->drawingAreaForegroundColor = globalResourceBundle.clr;
    displayInfo->gc = 0;

    displayInfo->traversalMode = globalDisplayListTraversalMode;
    displayInfo->hasBeenEditedButNotSaved = False;
    displayInfo->fromRelatedDisplayExecution = FALSE;

    displayInfo->nameValueTable = NULL;
    displayInfo->numNameValues = 0;

    displayInfo->dlFile = NULL;
    displayInfo->dlColormap = NULL;

  /* Create the shell and add callbacks */
    nargs = 0;
    XtSetArg(args[nargs],XmNiconName,"display"); nargs++;
    XtSetArg(args[nargs],XmNtitle,"display"); nargs++;
    XtSetArg(args[nargs],XmNallowShellResize,True); nargs++;
  /* Turn resize handles off
   * KE: Is is really good to do this? */
    XtSetArg(args[nargs],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH);
    nargs++;
  /* KE: The following is necessary for Exceed, which turns off the
     resize function with the handles.  It should not be necessary */
    XtSetArg(args[nargs],XmNmwmFunctions,MWM_FUNC_ALL); nargs++;
#if 1
  /* For highlightOnEnter on pointer motion, this must be set for shells */
  /* KE: It seems like the user should set this.   highlightOnEnter is
   * set for MessageButton, RelatedDisplay, Shell Command, TextEntry,
   * Valuator. Seems like it only does something for the Slider and
   * Text Entry */
    XtSetArg(args[nargs],XmNkeyboardFocusPolicy,XmPOINTER); nargs++;
#endif    
  /* Map window manager menu Close function to nothing for now */
    XtSetArg(args[nargs],XmNdeleteResponse,XmDO_NOTHING); nargs++;
    if(privateCmap) {
	XtSetArg(args[nargs],XmNcolormap,cmap); nargs++;
    }
    displayInfo->shell = XtCreatePopupShell("display",topLevelShellWidgetClass,
      mainShell,args,nargs);
    XtAddCallback(displayInfo->shell, XmNpopupCallback,
      displayShellPopupCallback, (XtPointer)displayInfo);
    XtAddCallback(displayInfo->shell,XmNpopdownCallback,
      displayShellPopdownCallback, NULL);

  /* Register interest in these protocols */
    { Atom atoms[2];
    atoms[0] = WM_DELETE_WINDOW;
    atoms[1] = WM_TAKE_FOCUS;
    XmAddWMProtocols(displayInfo->shell,atoms,2);
    }

  /* Register the window manager close callbacks */
    XmAddWMProtocolCallback(displayInfo->shell,WM_DELETE_WINDOW,
      (XtCallbackProc)wmCloseCallback,
      (XtPointer)DISPLAY_SHELL);
    
  /* Append to end of the list */
    displayInfo->next = NULL;
    displayInfo->prev = displayInfoListTail;
    displayInfoListTail->next = displayInfo;
    displayInfoListTail = displayInfo;
    
    return(displayInfo);
}

/*
 * function which cleans up a given displayInfo in the displayInfoList
 * (including the displayInfo's display list if specified)
 */
void dmCleanupDisplayInfo(DisplayInfo *displayInfo, Boolean cleanupDisplayList)
{
    int i;
    Boolean alreadyFreedUnphysical;
    Widget drawingArea;
    UpdateTask *pt = &(displayInfo->updateTaskListHead);

  /* Turn off any hidden button markers */
    if(displayInfo->nMarkerWidgets) {
      /* Toggle them off */
	markHiddenButtons(displayInfo);
    }

  /* Save a pointer to the drawingArea */
    drawingArea = displayInfo->drawingArea;
  /* Now set drawingArea to NULL in displayInfo to signify "in cleanup" */
    displayInfo->drawingArea = NULL;
    displayInfo->editPopupMenu = (Widget)0;
    displayInfo->executePopupMenu = (Widget)0;

  /* Remove all update tasks in this display */
    updateTaskDeleteAllTask(pt);
 
  /* As a composite widget, drawingArea is responsible for destroying
   it's children */
    if(drawingArea != NULL) {
	XtDestroyWidget(drawingArea);
	drawingArea = NULL;
    }

  /* force a wait for all outstanding CA event completion */
  /* (wanted to do   while(ca_pend_event() != ECA_NORMAL);  but that sits there     forever)
   */
#ifdef __MONITOR_CA_PEND_EVENT__
    {
	double t;
	t = medmTime();
	ca_pend_event(CA_PEND_EVENT_TIME);
	t = medmTime() - t;
	if(t > 0.5) {
	    print("dmCleanupDisplayInfo : time used by ca_pend_event = %8.1f\n",t);
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
    if(cleanupDisplayList) {
	XtDestroyWidget(displayInfo->shell);
	displayInfo->shell = NULL;
      /* remove display list here */
	clearDlDisplayList(displayInfo, displayInfo->dlElementList);
    } else {
	DlElement *dlElement = FirstDlElement(displayInfo->dlElementList);
	while(dlElement) {
	    if(dlElement->run->cleanup) {
		dlElement->run->cleanup(dlElement);
	    } else {
		dlElement->widget = NULL;
	    }
	    dlElement->data = NULL;
	    dlElement = dlElement->next;
	}
    }

  /*
   * free other X resources
   */
    if(displayInfo->drawingAreaPixmap != (Pixmap)NULL) {
	XFreePixmap(display,displayInfo->drawingAreaPixmap);
	displayInfo->drawingAreaPixmap = (Pixmap)NULL;
    }
    if(displayInfo->updatePixmap != (Pixmap)NULL) {
	XFreePixmap(display,displayInfo->updatePixmap);
	displayInfo->updatePixmap = (Pixmap)NULL;
    }
    if(displayInfo->colormap != NULL && displayInfo->dlColormapCounter > 0) {
	alreadyFreedUnphysical = False;
	for(i = 0; i < displayInfo->dlColormapCounter; i++) {
	    if(displayInfo->colormap[i] != unphysicalPixel) {
		XFreeColors(display,cmap,&(displayInfo->colormap[i]),1,0);
	    } else if(!alreadyFreedUnphysical) {
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
    if(displayInfo->gc) {
	XFreeGC(display,displayInfo->gc);
	displayInfo->gc = NULL;
    }
    displayInfo->drawingAreaBackgroundColor = 0;
    displayInfo->drawingAreaForegroundColor = 0;
}


void dmRemoveDisplayInfo(DisplayInfo *displayInfo)
{
    displayInfo->prev->next = displayInfo->next;
    if(displayInfo->next != NULL)
      displayInfo->next->prev = displayInfo->prev;
    if(displayInfoListTail == displayInfo)
      displayInfoListTail = displayInfoListTail->prev;
    if(displayInfoListTail == displayInfoListHead )
      displayInfoListHead->next = NULL;

  /* Cleanup resources and free display list */
    dmCleanupDisplayInfo(displayInfo,True);
    freeNameValueTable(displayInfo->nameValueTable,displayInfo->numNameValues);
    if(displayInfo->dlElementList) {
	clearDlDisplayList(displayInfo, displayInfo->dlElementList);
	free ( (char *) displayInfo->dlElementList);
    }
    if(displayInfo->selectedDlElementList) {
	clearDlDisplayList(displayInfo, displayInfo->selectedDlElementList);
	free ( (char *) displayInfo->selectedDlElementList);
    }
    if(displayInfo->dlFile) {
	free((char *)displayInfo->dlFile);
	displayInfo->dlFile = NULL;
    }
    if(displayInfo->dlColormap) {
	free((char *)displayInfo->dlColormap);
	displayInfo->dlColormap = NULL;
    }
    if(displayInfo) {
	free((char *)displayInfo);
	displayInfo = NULL;
    }

    if(displayInfoListHead == displayInfoListTail) {
	currentColormap = defaultColormap;
	currentColormapSize = DL_MAX_COLORS;
	currentDisplayInfo = NULL;
    }

  /* Refresh the display list dialog box */
    refreshDisplayListDlg();
}

/*
 * function to remove ALL displayInfo's
 *   this includes a full cleanup of associated resources and displayList
 */
void dmRemoveAllDisplayInfo()
{
    DisplayInfo *nextDisplay, *displayInfo;

    displayInfo = displayInfoListHead->next;
    while(displayInfo != NULL) {
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

/*** Callbck routines ***/

static void displayShellPopdownCallback(Widget shell, XtPointer cd, XtPointer cbs)
{
    Arg args[2];

    UNREFERENCED(cd);
    UNREFERENCED(cbs);
    
    XtSetArg(args[0],XmNx,&x);
    XtSetArg(args[1],XmNy,&y);
    XtGetValues(shell,args,2);
    lastShell = shell;
}

static void displayShellPopupCallback(Widget shell, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *displayInfo = (DisplayInfo *)cd;
    Arg args[2];
    char *env = getenv("MEDM_HELP");

    UNREFERENCED(cd);
    UNREFERENCED(cbs);
    
    if(shell == lastShell) {
	XtSetArg(args[0],XmNx,x);
	XtSetArg(args[1],XmNy,y);
	XtSetValues(shell,args,2);
    }
    if(env != NULL) addDisplayHelpProtocol(displayInfo);
}

/***  Parsing routines ***/

DlElement *getNextElement(DisplayInfo *pDI, char *token) {
    int i;
    for(i=0; i<parseFuncTableSize; i++) {
	if(!strcmp(token,parseFuncTable[i].name)) {
	    return parseFuncTable[i].func(pDI);
	}
    }
    return 0;
}

TOKEN parseAndAppendDisplayList(DisplayInfo *displayInfo, DlList *dlList,
  char *firstToken, TOKEN firstTokenType)
{
    TOKEN tokenType;
    char token[MAX_TOKEN_LENGTH];
    int nestingLevel = 0;
    static DlBasicAttribute attr;
    static DlDynamicAttribute dynAttr;
    static Boolean init = True;
    int first = 1;
    
  /* Initialize attributes to defaults for old format */
    if(init && displayInfo->versionNumber < 20200) {
	basicAttributeInit(&attr);
	dynamicAttributeInit(&dynAttr);
	init = False;
    }
 
  /* Loop over tokens until T_EOF */
    do {
	if(first) {
	    tokenType=firstTokenType;
	    strcpy(token,firstToken);
	    first = 0;
	} else {
	    tokenType=getToken(displayInfo,token);
	}
	switch(tokenType) {
	case T_WORD : {
	    DlElement *pe = 0;
	    if(pe = getNextElement(displayInfo,token)) {
	      /* Found an element via the parseFuncTable */
		if(displayInfo->versionNumber < 20200) {
		    switch (pe->type) {
		    case DL_Rectangle :
		    case DL_Oval      :
		    case DL_Arc       :
		    case DL_Text      :
		    case DL_Polyline  :
		    case DL_Polygon   :
		      /* Use the last found attributes */
			pe->structure.rectangle->attr = attr;
#if 0
			pe->structure.rectangle->dynAttr = dynAttr;
		      /* KE: Don't want to do this.  The old format
		       *  relies on them not being reset */
		      /* Reset the attributes to defaults */
			basicAttributeInit(&attr);
			dynamicAttributeInit(&dynAttr);
#else
		      /* KE: This was what was done in MEDM 2.2.9 */
			if(dynAttr.chan[0][0] != '\0') {
			    pe->structure.rectangle->dynAttr = dynAttr;
			    dynAttr.chan[0][0] = '\0';
			}
#endif			
			break;
		    }
		}
	    } else if(displayInfo->versionNumber < 20200) {
	      /* Did not find an element and old file version
	       *   Must be an attribute, which appear before the object does */
		if(!strcmp(token,"<<basic atribute>>") ||
		  !strcmp(token,"basic attribute") ||
		  !strcmp(token,"<<basic attribute>>")) {
		    parseOldBasicAttribute(displayInfo,&attr);
		} else if(!strcmp(token,"dynamic attribute") ||
		  !strcmp(token,"<<dynamic attribute>>")) {
		    parseOldDynamicAttribute(displayInfo,&dynAttr);
		}
	    }
	    if(pe) {
		appendDlElement(dlList,pe);
	    }
	}
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
	default :
	    break;
	}
    } while((tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF));

  /* Reset the init flag */
    if(tokenType == T_EOF) init = True;
    return tokenType;
}

/*
 * routine which actually parses the display list in the opened file
 *  N.B.  this function must be kept in sync with parseCompositeChildren
 *  which follows...
 */
void dmDisplayListParse(DisplayInfo *displayInfo, FILE *filePtr,
  char *argsString, char *filename,  char *geometryString,
  Boolean fromRelatedDisplayExecution)
{
    DisplayInfo *cdi;
    DlDisplay *dlDisplay;
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int numPairs;
    Position x, y;
    int xg, yg;
    unsigned int width, height;
    int mask;
    int reuse=0;
    DlElement *pE;
    Arg args[2];
    int nargs;
	
    
#if DEBUG_RELATED_DISPLAY
    print("\ndmDisplayListParse: displayInfo=%x\n"
      "  argsString=%s\n"
      "  filename=%s\n"
      "  geometryString=%s\n"
      "  fromRelatedDisplayExecution=%s\n",
      displayInfo,
      argsString?argsString:"NULL",
      filename?filename:"NULL",
      geometryString?geometryString:"NULL",
      fromRelatedDisplayExecution?"True":"False");
#endif    

    initializeGlobalResourceBundle();

    if(!displayInfo) {
      /* Did not come from a display that is to be replaced
       *   Allocate a DisplayInfo structure */
	cdi = currentDisplayInfo = allocateDisplayInfo();
	cdi->filePtr = filePtr;
	cdi->newDisplay = False;
    } else {
      /* Came from a display that is to be replaced */
      /* Get the current values of x and y */
	nargs=0;
	XtSetArg(args[nargs],XmNx,&x); nargs++;
	XtSetArg(args[nargs],XmNy,&y); nargs++;
	XtGetValues(displayInfo->shell,args,nargs);
#if DEBUG_RELATED_DISPLAY
	print("dmDisplayListParse: Old values: x=%d y=%d\n",x,y);
#endif      
      /* See if we want to reuse it or save it */
	if(displayInfo->fromRelatedDisplayExecution == True) {
	  /* Not an original, don't save it, reuse it */
	    reuse=1;
	  /* Clear out old display */
	    dmCleanupDisplayInfo(displayInfo,False);
	    clearDlDisplayList(displayInfo, displayInfo->dlElementList);
	  /* dlFile will be created during parsing so free it */
	    if(displayInfo->dlFile) {
		free((char *)displayInfo->dlFile);
		displayInfo->dlFile = NULL;
	    }	    
	    displayInfo->filePtr = filePtr;
	    cdi = currentDisplayInfo = displayInfo;
	    cdi->newDisplay = False;
	} else {
	  /* This is an original, pop it down */
	    XtPopdown(displayInfo->shell);
#if DEBUG_RELATED_DISPLAY > 1
	    dumpDisplayInfoList(displayInfoListHead,
	      "dmDisplayListParse [1]: displayInfoList");
	    dumpDisplayInfoList(displayInfoSaveListHead,
	      "dmDisplayListParse [1]: displayInfoSaveList");
#endif
	  /* Save it if not already saved */
	    moveDisplayInfoToDisplayInfoSave(displayInfo);
#if DEBUG_RELATED_DISPLAY > 1
	    dumpDisplayInfoList(displayInfoListHead,
	      "dmDisplayListParse [2]: displayInfoList");
	    dumpDisplayInfoList(displayInfoSaveListHead,
	      "dmDisplayListParse [2]: displayInfoSaveList");
#endif	    
	    cdi = currentDisplayInfo = allocateDisplayInfo();
	    cdi->filePtr = filePtr;
	    cdi->newDisplay = False;
	}
    }
  
    cdi->fromRelatedDisplayExecution = fromRelatedDisplayExecution;
    
  /* Generate the name-value table for macro substitutions (related display) */
    if(argsString) {
	cdi->nameValueTable = generateNameValueTable(argsString,&numPairs);
	cdi->numNameValues = numPairs;
    } else {
	cdi->nameValueTable = NULL;
	cdi->numNameValues = 0;
    }

  /* Read the file block (Must be there) */
  /* If first token isn't "file" then bail out */
    tokenType=getToken(cdi,token);
    if(tokenType == T_WORD && !strcmp(token,"file")) {
	cdi->dlFile = parseFile(cdi);
	if(cdi->dlFile) {
	    cdi->versionNumber = cdi->dlFile->versionNumber;
	    strcpy(cdi->dlFile->name,filename);
	} else {
	    medmPostMsg(1,"dmDisplayListParse: Out of memory\n"
	      "  file: %s\n",filename);
	    cdi->filePtr = NULL;
	    dmRemoveDisplayInfo(cdi);
	    cdi = NULL;
	    return;
	}
    } else {
	medmPostMsg(1,"dmDisplayListParse: Invalid .adl file "
	  "(First block is not file block)\n"
	  "  file: %s\n",filename);
	cdi->filePtr = NULL;
	dmRemoveDisplayInfo(cdi);
	cdi = NULL;
	return;
    }

#if DEBUG_RELATED_DISPLAY
    print("  File: %s\n",cdi->dlFile->name);
#if 0    
    if(strstr(cdi->dlFile->name,"sMain.adl")) {
	debugDisplayInfo=displayInfo;
	print("Set debugDisplayInfo for sMain.adl\n");
    }
#endif
#endif

  /* Read the display block (Must be there) */
    tokenType=getToken(cdi,token);
    if(tokenType == T_WORD && !strcmp(token,"display")) {
	parseDisplay(cdi);
    } else {
	medmPostMsg(1,"dmDisplayListParse: Invalid .adl file "
	  "(Second block is not display block)\n"
	  "  file: %s\n",filename);
	cdi->filePtr = NULL;
	dmRemoveDisplayInfo(cdi);
	cdi = NULL;
	return;
    }

  /* Read the colormap if there.  Will also create cdi->dlColormap. */
    tokenType=getToken(cdi,token);
    if(tokenType == T_WORD && 
      (!strcmp(token,"color map") ||
	!strcmp(token,"<<color map>>"))) {
      /* This is a colormap block, parse it */
	cdi->dlColormap=parseColormap(cdi,cdi->filePtr);
	if(cdi->dlColormap) {
	  /* Success.  Get the next token */
	    tokenType=getToken(cdi,token);
	} else {
	  /* Error */
	    medmPostMsg(1,"dmDisplayListParse: Invalid .adl file "
	      "(Cannot parse colormap)\n"
	      "  file: %s\n",filename);
	    cdi->filePtr = NULL;
	    dmRemoveDisplayInfo(cdi);
	    cdi = NULL;
	    return;
	}
    }

  /* Proceed with parsing */
    while(parseAndAppendDisplayList(cdi, cdi->dlElementList, token, tokenType)
      != T_EOF) {
	tokenType=getToken(cdi,token);
    }

  /* NULL the file pointer.  We are done with it */
    cdi->filePtr = NULL;

  /* The display is the first element */
    pE = FirstDlElement(cdi->dlElementList);
    if(!pE || !pE->structure.display) {
      /* Error */
	medmPostMsg(1,"dmDisplayListParse: Invalid .adl file "
	  "(Display is not the first element)\n"
	  "  file: %s\n",filename);
	cdi->filePtr = NULL;
	dmRemoveDisplayInfo(cdi);
	cdi = NULL;
	return;
    }
    dlDisplay = pE->structure.display;

  /* The presense of a cmap overrides the presense of a color map
    block.  If there is a colormap in the displayInfo from parsing a
    color map block, free it. */
    if(*dlDisplay->cmap && cdi->dlColormap) {
	free((char *)cdi->dlColormap);
	cdi->dlColormap = NULL;
    }
    
  /* Change DlObject values for x and y to be the same as the original
     if it is a Related Display */
    if(displayInfo) {
      /* Note that cdi is not necessarily the same as displayInfo */
#if DEBUG_RELATED_DISPLAY
	print("  Set replaced values: XtIsRealized=%s XtIsManaged=%s\n",
	  XtIsRealized(cdi->shell)?"True":"False",
	  XtIsManaged(cdi->shell)?"True":"False");
	print("  x=%d dlDisplay->object.x=%d\n"
	  "  y=%d dlDisplay->object.y=%d\n",
	  x,dlDisplay->object.x,
	  y,dlDisplay->object.y);
#endif	
	dlDisplay->object.x = x;
	dlDisplay->object.y = y;
    }

  /* Do resizing */
    if(reuse) {
      /* Resize the display to the new size now
       *   (There should be no resize callback yet so it will not try
       *     to resize all the elements) */
	nargs=0;
	XtSetArg(args[nargs],XmNwidth,dlDisplay->object.width);
	nargs++;
	XtSetArg(args[nargs],XmNheight,dlDisplay->object.height);
	nargs++;
	XtSetValues(displayInfo->shell,args,nargs);
    } else if(geometryString && *geometryString) {
      /* Handle geometry string */
      /* Parse the geometry string (mask indicates what was found) */
	mask = XParseGeometry(geometryString,&xg,&yg,&width,&height);
#if DEBUG_RELATED_DISPLAY
	print("dmDisplayListParse: Geometry values: xg=%d yg=%d\n",xg,yg);
#endif      
	
      /* Change width and height object values now, x and y later */
	if((mask & WidthValue) && (mask & HeightValue)) {
	    dmResizeDisplayList(cdi,(Dimension)width,(Dimension)height);
	}
    }

  /* Execute all the elements including the display.  Set the
     elementsExecuted flag since this is the first time for this
     display. */
    cdi->elementsExecuted=False;
    dmTraverseDisplayList(cdi);
    cdi->elementsExecuted=True;
    
  /* Pop it up */
    XtPopup(cdi->shell,XtGrabNone);
#if DEBUG_RELATED_DISPLAY
    print("  After XtPopup: XtIsRealized=%s XtIsManaged=%s\n",
      XtIsRealized(cdi->shell)?"True":"False",
      XtIsManaged(cdi->shell)?"True":"False");
    print("  Current values:\n"
      "    dlDisplay->object.x=%d\n"
      "    dlDisplay->object.y=%d\n",
      dlDisplay->object.x,
      dlDisplay->object.y);
    {
	Position xpos,ypos;
	
	nargs=0;
	XtSetArg(args[nargs],XmNx,&xpos); nargs++;
	XtSetArg(args[nargs],XmNy,&ypos); nargs++;
	XtGetValues(cdi->shell,args,nargs);
	print("dmDisplayListParse: After XtPopup: xpos=%d ypos=%d\n",xpos,ypos);
    }
#endif      
    
  /* Do moving after it is realized */
    if(displayInfo && !reuse) {
#if DEBUG_RELATED_DISPLAY
	print("  Move(1)\n");
	dlDisplay->object.x=x;
	dlDisplay->object.y=y;
#endif	
    } else if(geometryString && *geometryString) {
#if DEBUG_RELATED_DISPLAY
	print("  Move(2)\n");
#endif	
	if((mask & XValue) && (mask & YValue)) {
	    dlDisplay->object.x=xg;
	    dlDisplay->object.y=yg;
	}
#if DEBUG_RELATED_DISPLAY
    } else {
	print("  Move(0)\n");
#endif
    }

  /* KE: Move it to be consistent with its object values
   * XtSetValues or XtMoveWidget do not work here
   * Is necessary in part because WM adds borders and title bar,
       moving the shell down when first created */
    XMoveWindow(display,XtWindow(cdi->shell),
      dlDisplay->object.x,
      dlDisplay->object.y);

#if DEBUG_RELATED_DISPLAY
    print("  Final values:\n"
      "    dlDisplay->object.x=%d\n"
      "    dlDisplay->object.y=%d\n",
      dlDisplay->object.x,
      dlDisplay->object.y);
    {
	Position xpos,ypos;
	
	nargs=0;
	XtSetArg(args[nargs],XmNx,&xpos); nargs++;
	XtSetArg(args[nargs],XmNy,&ypos); nargs++;
	XtGetValues(cdi->shell,args,nargs);
	print("dmDisplayListParse: At end: xpos=%d ypos=%d\n",xpos,ypos);
    }
#endif
    
  /* Refresh the display list dialog box */
    refreshDisplayListDlg();
}

/*
 * Function to open a specified file (as .adl if specified as .dl),
 *   looking in EPICS_DISPLAY_PATH directory if unavailable in the
 *   working directory
 */

FILE *dmOpenUsableFile(char *filename, char *relatedDisplayFilename)
{
    FILE *filePtr;
    int startPos;
    char name[MAX_TOKEN_LENGTH], fullPathName[MAX_DIR_LENGTH],
      dirName[MAX_DIR_LENGTH];
    char *dir, *ptr;
#if DEBUG_FILE
    static FILE *file=NULL;
    static pid_t pid=0;
#endif

  /* Try to open with the given name first
   *   (Will be in cwd if not an absolute pathname) */
    strncpy(name, filename, MAX_TOKEN_LENGTH);
    name[MAX_TOKEN_LENGTH-1] = '\0';
    filePtr = fopen(name,"r");
#if DEBUG_FILE
    if(!file) {
	file=fopen("/tmp/medmLog","a");
	pid=getpid();
	if(file) {
	    fprintf(file,"Initializing PID: %d\n",pid);
	} else {
	    print("Cannot open /tmp/medmLog\n");
	}
    }
#if DEBUG_FILE > 1
    print("[%d] dmOpenUsableFile: %s\n",pid,filename);
    print("  [1] Converted to: %s\n",name);
    if(filePtr) print("    Found as: %s\n",name);
#endif	
    if(file) {
	fprintf(file,"[%d] dmOpenUsableFile: %s\n",pid,filename);
	fprintf(file,"  [1] Converted to: %s\n",name);
	if(filePtr) fprintf(file,"    Found as: %s\n",name);
	fflush(file);
    }
#endif
    if(filePtr) {
	strcpy(filename, name);
	return (filePtr);
    }

  /* If the name starts with / or . then we can do no more */
    if(name[0] == MEDM_DIR_DELIMITER_CHAR || name[0] == '.') return (NULL);

  /* If the name comes from a related display,
   *   then try the directory of the related display */
    if(relatedDisplayFilename && *relatedDisplayFilename) {
	strncpy(fullPathName, relatedDisplayFilename, MAX_DIR_LENGTH);
	fullPathName[MAX_DIR_LENGTH-1] = '\0';
	if(fullPathName && fullPathName[0]) {
	    ptr = strrchr(fullPathName, MEDM_DIR_DELIMITER_CHAR);
	    if(ptr) {
		*(++ptr) = '\0';
		strcat(fullPathName, name);
		filePtr = fopen(fullPathName, "r");
#if DEBUG_FILE
#if DEBUG_FILE > 1
		print("  [2] Converted to: %s\n",fullPathName);
		if(filePtr) print("    Found as: %s\n",fullPathName);
#endif		
		if(file) {
		    fprintf(file,"  [2] Converted to: %s\n",fullPathName);
		    if(filePtr) fprintf(file,"    Found as: %s\n",fullPathName);
		    fflush(file);
		}
#endif
		if(filePtr) {
		    strcpy(filename, fullPathName);
		    return (filePtr);
		}
	    }
	}
    }

  /* Look in EPICS_DISPLAY_PATH directories */
    dir = getenv("EPICS_DISPLAY_PATH");
    if(dir != NULL) {
	startPos = 0;
	while(filePtr == NULL &&
	  extractStringBetweenColons(dir,dirName,startPos,&startPos)) {
	    strncpy(fullPathName, dirName, MAX_DIR_LENGTH);
	    fullPathName[MAX_DIR_LENGTH-1] = '\0';
	    strcat(fullPathName, MEDM_DIR_DELIMITER_STRING);
	    strcat(fullPathName, name);
	    filePtr = fopen(fullPathName, "r");
#if DEBUG_FILE
#if DEBUG_FILE > 1
	    print("  [3] Converted to: %s\n",fullPathName);
	    if(filePtr) print("    Found as: %s\n",fullPathName);
#endif		
	    if(file) {
		fprintf(file,"  [3] Converted to: %s\n",fullPathName);
		if(filePtr) fprintf(file,"    Found as: %s\n",fullPathName);
		fflush(file);
	    }
#endif
	    if(filePtr) {
		strcpy(filename, fullPathName);
		return (filePtr);
	    }
	}
    }

  /* Not found */
    return (NULL);
}

/*** Other routines ***/

/*
 * clean up the memory-resident display list (if there is one)
 */
void clearDlDisplayList(DisplayInfo *displayInfo, DlList *list)
{
    DlElement *dlElement, *pE;
    
    if(list->count == 0) return;
    dlElement = FirstDlElement(list);
    while(dlElement) {
	pE = dlElement;
	dlElement = dlElement->next;
	if(pE->run->destroy) {
	    pE->run->destroy(displayInfo, pE);
	} else {
	    genericDestroy(displayInfo, pE);
	}
    }
    emptyDlList(list);
}

/*
 * Same as clearDlDisplayList except that it does not clear the display
 * and it destroys any widgets
 */
void removeDlDisplayListElementsExceptDisplay(DisplayInfo * displayInfo,
  DlList *list)
{
    DlElement *dlElement, *pE;
    DlElement *psave = NULL;
    
    if(list->count == 0) return;
    dlElement = FirstDlElement(list);
    while(dlElement) {
	pE = dlElement;
	if(dlElement->type != DL_Display) {
	    dlElement = dlElement->next;
	    destroyElementWidgets(pE);
	    if(pE->run->destroy) {
		pE->run->destroy(displayInfo, pE);
	    } else {
		genericDestroy(displayInfo, pE);
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
 * Traverse (execute) specified displayInfo's display list
 */
void dmTraverseDisplayList(DisplayInfo *displayInfo)
{
    DlElement *pE;
    Dimension width, height;

  /* Traverse the display list */
#if DEBUG_TRAVERSAL
    print("\n[dmTraverseDisplayList: displayInfo->dlElementList:\n");
    dumpDlElementList(displayInfo->dlElementList);
#endif
    pE = FirstDlElement(displayInfo->dlElementList);
    width = pE->structure.display->object.width;
    height = pE->structure.display->object.height;
    while(pE) {
	if(pE->run->execute) (pE->run->execute)(displayInfo,pE);
	pE = pE->next;
    }

  /* Update the window */
    XCopyArea(display,displayInfo->drawingAreaPixmap,
      XtWindow(displayInfo->drawingArea), displayInfo->gc,
      0, 0, width, height, 0, 0);
    
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
	if(t > 0.5) {
	    print("dmTraverseDisplayList : time used by ca_pend_event = %8.1f\n",t);
	}
    }
#else
    ca_pend_event(CA_PEND_EVENT_TIME);
#endif
}



/* Traverse (execute) all displayInfos and display lists.  Could call
 * dmTraverseDisplayList inside the displayInfo traversal, but only
 * need one XFlush and one ca_pend_event this way.  This is only
 * called from modeCallback and is used for initialization. */
void dmTraverseAllDisplayLists()
{
    DisplayInfo *displayInfo;
    DlElement *pE;
    Dimension width, height;

    displayInfo = displayInfoListHead->next;

  /* Traverse the displayInfo list */
    while(displayInfo != NULL) {

      /* Traverse the display list for this displayInfo */
#if DEBUG_TRAVERSAL
	print("\n[dmTraverseAllDisplayLists: displayInfo->dlElementList:\n");
	dumpDlElementList(displayInfo->dlElementList);
#endif
      /* Set the elementsExecuted flag since this is the first time
       *  for this display. */
	displayInfo->elementsExecuted = False;
	pE = FirstDlElement(displayInfo->dlElementList);
	width = pE->structure.display->object.width;
	height = pE->structure.display->object.height;
	while(pE) {
	    
	    pE->updateType = WIDGET;
	    pE->hidden = False;
	    pE->data = NULL;
	    if(pE->run->execute) (pE->run->execute)(displayInfo,pE);
	    pE = pE->next;
	}
      /* Set elementsExecuted to True since first time is done */
	displayInfo->elementsExecuted = True;

      /* Update the window */
	XCopyArea(display,displayInfo->drawingAreaPixmap,
	  XtWindow(displayInfo->drawingArea), displayInfo->gc,
	  0, 0, width, height, 0, 0);

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
	if(t > 0.5) {
	    print("dmTraverseAllDisplayLists : time used by ca_pend_event = %8.1f\n",t);
	}
    }
#else
    ca_pend_event(CA_PEND_EVENT_TIME);
#endif

}

/* Traverse (execute) specified displayInfo's display list non-widget
 * elements.  Should only be called in EDIT mode. */
void dmTraverseNonWidgetsInDisplayList(DisplayInfo *displayInfo)
{
    DlElement *pE;
    Dimension width,height;

    if(displayInfo == NULL) return;

  /* Unhighlight any selected elements */
    unhighlightSelectedElements();

  /* Get width and height */
    pE = FirstDlElement(displayInfo->dlElementList);
    width = pE->structure.display->object.width;
    height = pE->structure.display->object.height;
    
  /* Fill the background with the background color */
    XSetForeground(display, displayInfo->gc,
      displayInfo->colormap[displayInfo->drawingAreaBackgroundColor]);
    XFillRectangle(display, displayInfo->drawingAreaPixmap,
      displayInfo->gc, 0, 0, (unsigned int)width, (unsigned int)height);

  /* Draw grid */
    if(displayInfo->grid->gridOn && globalDisplayListTraversalMode == DL_EDIT)
     drawGrid(displayInfo);

  /* Traverse the display list */
  /* Loop over elements not including the display */
    pE = SecondDlElement(displayInfo->dlElementList);
    while(pE) {
	if(!pE->widget) {
	    (pE->run->execute)(displayInfo, pE);
	}
	pE = pE->next;
    }

  /* Update the window */
    XCopyArea(display,displayInfo->drawingAreaPixmap,
      XtWindow(displayInfo->drawingArea), displayInfo->gc,
      0, 0, width, height, 0, 0);

  /* Highlight any selected elements */
    highlightSelectedElements();
    
  /* Change drawingArea's cursor to the appropriate cursor */
    XDefineCursor(display,XtWindow(displayInfo->drawingArea),
      (currentActionType == SELECT_ACTION ?
	rubberbandCursor : crosshairCursor));
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
    while(w && (XtClass(w) != topLevelShellWidgetClass)) {
	w = XtParent(w);
    }

    if(w) {
	displayInfo = displayInfoListHead->next;
	while(displayInfo && (displayInfo->shell != w)) {
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
    DlElement *pE;
    DlDisplay *dlDisplay = NULL;

    writeDlFile(stream,displayInfo->dlFile,0);
    if(pE = FirstDlElement(displayInfo->dlElementList)) {
      /* This must be DL_DISPLAY */
	(pE->run->write)(stream,pE,0);
	dlDisplay=pE->structure.display;
    }

  /* Write the colormap unless there is a cmap defined for the display */
    if(displayInfo->dlColormap && dlDisplay && !*dlDisplay->cmap)
      writeDlColormap(stream,displayInfo->dlColormap,0);

  /* Traverse the display list */
    pE = pE->next;
    while(pE) {
	(pE->run->write)(stream,pE,0);
	pE = pE->next;
    }
    fprintf(stream,"\n");
}

void medmSetDisplayTitle(DisplayInfo *displayInfo)
{
    char str[MAX_FILE_CHARS+10];

    if(displayInfo->dlFile) {
	char *tmp, *tmp1;
	tmp = tmp1 = displayInfo->dlFile->name;
	while(*tmp != '\0')
	  if(*tmp++ == MEDM_DIR_DELIMITER_CHAR) tmp1 = tmp;
	if(displayInfo->hasBeenEditedButNotSaved) {
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
    if(globalDisplayListTraversalMode == DL_EDIT) {
	displayInfo->hasBeenEditedButNotSaved = True;
	medmSetDisplayTitle(displayInfo);
    }
}

