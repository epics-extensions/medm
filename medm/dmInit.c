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

#include <X11/keysym.h>

extern Widget mainShell;

static Position x = 0, y = 0;
static Widget lastShell;

#define DISPLAY_DEFAULT_X 10
#define DISPLAY_DEFAULT_Y 10

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
    {"cartesian plot",       parseCartesianPlot},
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

/* DEBUG */
#if 0
DisplayInfo *debugDisplayInfo=NULL;
#endif
/* End DEBUG */

int parseFuncTableSize = sizeof(parseFuncTable)/sizeof(ParseFuncEntry);

DlElement *getNextElement(DisplayInfo *pDI, char *token) {
    int i;
    for (i=0; i<parseFuncTableSize; i++) {
	if (!strcmp(token,parseFuncTable[i].name)) {
	    return parseFuncTable[i].func(pDI);
	}
    }
    return 0;
}

#ifdef __cplusplus
static void displayShellPopdownCallback(Widget shell, XtPointer, XtPointer)
#else
static void displayShellPopdownCallback(Widget shell, XtPointer cd, XtPointer cbs)
#endif
{
    Arg args[2];
    XtSetArg(args[0],XmNx,&x);
    XtSetArg(args[1],XmNy,&y);
    XtGetValues(shell,args,2);
    lastShell = shell;
}

#ifdef __cplusplus
static void displayShellPopupCallback(Widget shell, XtPointer, XtPointer)
#else
    static void displayShellPopupCallback(Widget shell, XtPointer cd, XtPointer cbs)
#endif
{
    Arg args[2];

    if (shell == lastShell) {
	XtSetArg(args[0],XmNx,x);
	XtSetArg(args[1],XmNy,y);
	XtSetValues(shell,args,2);
    }
    help_protocol(shell);
}

/***
 ***  displayInfo creation
 ***/

/*
 * create and return a DisplayInfo structure pointer on tail of displayInfoList
 *  this includes a shell (with it's dialogs and event handlers)
 */
DisplayInfo *allocateDisplayInfo()
{
    DisplayInfo *displayInfo;
    int n;
    Arg args[8];

/* 
 * allocate a DisplayInfo structure and shell for this display file/list
 */
    displayInfo = (DisplayInfo *) malloc(sizeof(DisplayInfo));
    if (!displayInfo) return NULL;

    displayInfo->dlElementList = createDlList();
    if (!displayInfo->dlElementList) {
	free(displayInfo);
	return NULL;
    }

    displayInfo->selectedDlElementList = createDlList();
    if (!displayInfo->selectedDlElementList) {
	free(displayInfo->dlElementList);
	free(displayInfo);
	return NULL;
    }
    displayInfo->selectedElementsAreHighlighted = False;

    displayInfo->filePtr = NULL;
    displayInfo->newDisplay = True;
    displayInfo->versionNumber = 0;

    displayInfo->drawingArea = 0;
    displayInfo->drawingAreaPixmap = 0;
    displayInfo->cartesianPlotPopupMenu = 0;
    displayInfo->selectedCartesianPlot = 0;
    displayInfo->warningDialog = NULL;
    displayInfo->warningDialogAnswer = 0;
    displayInfo->questionDialog = NULL;
    displayInfo->questionDialogAnswer = 0;
    displayInfo->shellCommandPromptD = NULL;

    displayInfo->gridOn = DEFAULT_GRID_ON;
    displayInfo->gridSpacing = DEFAULT_GRID_SPACING;
    displayInfo->undoInfo = NULL;

    updateTaskInit(displayInfo);

#if 0
    displayInfo->childCount = 0;
#endif

    displayInfo->colormap = 0;
    displayInfo->dlColormapCounter = 0;
    displayInfo->dlColormapSize = 0;
    displayInfo->drawingAreaBackgroundColor = globalResourceBundle.bclr;
    displayInfo->drawingAreaForegroundColor = globalResourceBundle.clr;
    displayInfo->gc = 0;
    displayInfo->pixmapGC = 0;

    displayInfo->traversalMode = globalDisplayListTraversalMode;
    displayInfo->hasBeenEditedButNotSaved = False;
    displayInfo->fromRelatedDisplayExecution = FALSE;

    displayInfo->nameValueTable = NULL;
    displayInfo->numNameValues = 0;

    displayInfo->dlFile = NULL;
    displayInfo->dlColormap = NULL;

  /*
   * create the shell and add callbacks
   */
    n = 0;
    XtSetArg(args[n],XmNiconName,"display"); n++;
    XtSetArg(args[n],XmNtitle,"display"); n++;
    XtSetArg(args[n],XmNallowShellResize,TRUE); n++;
  /* for highlightOnEnter on pointer motion, this must be set for shells */
    XtSetArg(args[n],XmNkeyboardFocusPolicy,XmPOINTER); n++;
  /* map window manager menu Close function to application close... */
    XtSetArg(args[n],XmNdeleteResponse,XmDO_NOTHING); n++;
    if (privateCmap) {
	XtSetArg(args[n],XmNcolormap,cmap); n++;
    }
    displayInfo->shell = XtCreatePopupShell("display",topLevelShellWidgetClass,
      mainShell,args,n);
    XtAddCallback(displayInfo->shell,XmNpopupCallback,
      displayShellPopupCallback,NULL);
    XtAddCallback(displayInfo->shell,XmNpopdownCallback,
      displayShellPopdownCallback,NULL);

  /* register interest in these protocols */
    { Atom atoms[2];
    atoms[0] = WM_DELETE_WINDOW;
    atoms[1] = WM_TAKE_FOCUS;
    XmAddWMProtocols(displayInfo->shell,atoms,2);
    }

  /* and register the callbacks for these protocols */
    XmAddWMProtocolCallback(displayInfo->shell,WM_DELETE_WINDOW,
      (XtCallbackProc)wmCloseCallback,
      (XtPointer)DISPLAY_SHELL);

  /* 
   * create the shell's EXECUTE popup menu
   */
    n = 0;
    XtSetArg(args[n], XmNbuttonCount, NUM_EXECUTE_POPUP_ENTRIES); n++;
    XtSetArg(args[n], XmNbuttonType, executePopupMenuButtonType); n++;
    XtSetArg(args[n], XmNbuttons, executePopupMenuButtons); n++;
    XtSetArg(args[n], XmNsimpleCallback, executePopupMenuCallback); n++;
    XtSetArg(args[n], XmNuserData, displayInfo); n++;
    XtSetArg(args[n],XmNtearOffModel,XmTEAR_OFF_DISABLED); n++;
    displayInfo->executePopupMenu = XmCreateSimplePopupMenu(displayInfo->shell,
      "executePopupMenu", args, n);
  /* 
   * create the shell's EDIT popup menu
   */
    displayInfo->editPopupMenu = createDisplayMenu(displayInfo->shell);
    XtVaSetValues(displayInfo->editPopupMenu,
      XmNtearOffModel, XmTEAR_OFF_DISABLED,
      XmNuserData, displayInfo,
      NULL);

/*
 * attach event handlers for menu activation
 */
    XtAddEventHandler(displayInfo->shell,ButtonPressMask,False,
      popupMenu,displayInfo);
    XtAddEventHandler(displayInfo->shell,ButtonReleaseMask,False,
      popdownMenu,displayInfo);

  /* append to end of the list */
    displayInfo->next = NULL;
    displayInfo->prev = displayInfoListTail;
    displayInfoListTail->next = displayInfo;
    displayInfoListTail = displayInfo;

    return(displayInfo);
}


TOKEN parseAndAppendDisplayList(DisplayInfo *displayInfo, DlList *dlList) {
    TOKEN tokenType;
    char token[MAX_TOKEN_LENGTH];
    int nestingLevel = 0;
    static DlBasicAttribute attr;
    static DlDynamicAttribute dynAttr;
    static Boolean init = True;
 
  /* Initialize attributes to defaults for old format */
    if (init && displayInfo->versionNumber < 20200) {
	basicAttributeInit(&attr);
	dynamicAttributeInit(&dynAttr);
	init = False;
    }
 
  /* Loop over tokens until T_EOF */
    do {
	switch (tokenType=getToken(displayInfo,token)) {
	case T_WORD : {
	    DlElement *pe = 0;
	    if (pe = getNextElement(displayInfo,token)) {
	      /* Found an element via the parseFuncTable */
		if (displayInfo->versionNumber < 20200) {
		    switch (pe->type) {
		    case DL_Rectangle :
		    case DL_Oval      :
		    case DL_Arc       :
		    case DL_Text      :
		    case DL_Polyline  :
		    case DL_Polygon   :
		      /* Use the last found attributes */
			pe->structure.rectangle->attr = attr;
			pe->structure.rectangle->dynAttr = dynAttr;
		      /* Reset the attributes to defaults */
			basicAttributeInit(&attr);
			dynamicAttributeInit(&dynAttr);
			break;
		    }
		}
	    } else if (displayInfo->versionNumber < 20200) {
	      /* Did not find an element and old file version */
	      /* Parse attributes, which appear before the object does */
		if (!strcmp(token,"<<basic atribute>>") ||
		  !strcmp(token,"basic attribute") ||
		  !strcmp(token,"<<basic attribute>>")) {
		    parseOldBasicAttribute(displayInfo,&attr);
		} else if (!strcmp(token,"dynamic attribute") ||
		  !strcmp(token,"<<dynamic attribute>>")) {
		    parseOldDynamicAttribute(displayInfo,&dynAttr);
		}
	    }
	    if (pe) {
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
    } while ((tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF));

  /* Reset the init flag */
    if (tokenType == T_EOF) init = True;
    return tokenType;
}

/***
 ***  parsing and drawing/widget creation routines
 ***/

/*
 * routine which actually parses the display list in the opened file
 *  N.B.  this function must be kept in sync with parseCompositeChildren
 *  which follows...
 */
void dmDisplayListParse(
  DisplayInfo *displayInfo,
  FILE *filePtr,
  char *argsString,
  char *filename,
  char *geometryString,
  Boolean fromRelatedDisplayExecution)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    Arg args[7];
    DlElement *dlElement;
    int numPairs;

    initializeGlobalResourceBundle();

    if (!displayInfo) {
      /* 
       * allocate a DisplayInfo structure and shell for this display file/list
       */
	displayInfo = allocateDisplayInfo();
	displayInfo->filePtr = filePtr;
	currentDisplayInfo = displayInfo;
	currentDisplayInfo->newDisplay = False;
    } else {
	dmCleanupDisplayInfo(displayInfo,False);
	clearDlDisplayList(displayInfo->dlElementList);
	displayInfo->filePtr = filePtr;
	currentDisplayInfo = displayInfo;
	currentDisplayInfo->newDisplay = False;
    }
  
    fromRelatedDisplayExecution = displayInfo->fromRelatedDisplayExecution;

  /*
   * generate the name-value table for macro substitutions (related display)
   */
    if (argsString) {
	displayInfo->nameValueTable = generateNameValueTable(argsString,&numPairs);
	displayInfo->numNameValues = numPairs;
    } else {
	displayInfo->nameValueTable = NULL;
	displayInfo->numNameValues = 0;
    }


  /* if first token isn't "file" then bail out! */
    tokenType=getToken(displayInfo,token);
    if (tokenType == T_WORD && !strcmp(token,"file")) {
	displayInfo->dlFile = parseFile(displayInfo);
	if (displayInfo->dlFile) {
	    displayInfo->versionNumber = displayInfo->dlFile->versionNumber;
	    strcpy(displayInfo->dlFile->name,filename);
	} else {
	    fprintf(stderr,"\ndmDisplayListParse: out of memory!");
	    displayInfo->filePtr = NULL;
	    dmRemoveDisplayInfo(displayInfo);
	    currentDisplayInfo = NULL;
	    return;
	}
    } else {
	fprintf(stderr,"\ndmDisplayListParse: invalid .adl file (bad first token)");
	displayInfo->filePtr = NULL;
	dmRemoveDisplayInfo(displayInfo);
	currentDisplayInfo = NULL;
	return;
    }

  /* DEBUG */
#if 0    
    printf("File: %s\n",displayInfo->dlFile->name);
    if(strstr(displayInfo->dlFile->name,"sMain.adl")) {
	debugDisplayInfo=displayInfo;
	printf("Set debugDisplayInfo for sMain.adl\n");
    }
#endif
  /* End DEBUG */

    tokenType=getToken(displayInfo,token);
    if (tokenType ==T_WORD && !strcmp(token,"display")) {
	parseDisplay(displayInfo);
    }

    tokenType=getToken(displayInfo,token);
    if (tokenType == T_WORD && 
      (!strcmp(token,"color map") ||
	!strcmp(token,"<<color map>>"))) {
	displayInfo->dlColormap=parseColormap(displayInfo,displayInfo->filePtr);
	if (!displayInfo->dlColormap) {
	  /* error - do total bail out */
	    fclose(displayInfo->filePtr);
	    dmRemoveDisplayInfo(displayInfo);
	    return;
	}
    }

  /*
   * proceed with parsing
   */
    while (parseAndAppendDisplayList(displayInfo,displayInfo->dlElementList)
      != T_EOF );

    displayInfo->filePtr = NULL;

/*
 * traverse (execute) this displayInfo and associated display list
 */
    {
	int x, y;
	unsigned int w, h;
	int mask;

	mask = XParseGeometry(geometryString,&x,&y,&w,&h);

	if ((mask & WidthValue) && (mask & HeightValue)) {
	    dmResizeDisplayList(displayInfo,w,h);
	}
	dmTraverseDisplayList(displayInfo);

	XtPopup(displayInfo->shell,XtGrabNone);

	if ((mask & XValue) && (mask & YValue)) {
	    XMoveWindow(XtDisplay(displayInfo->shell),XtWindow(displayInfo->shell),x,y);
	}
    }
}

DlElement *parseDisplay(
  DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlDisplay *dlDisplay;
    DlElement *dlElement = createDlDisplay(NULL);
 
    if (!dlElement) return 0;
    dlDisplay = dlElement->structure.display;
 
    do {
        switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlDisplay->object));
	    } else if (!strcmp(token,"cmap")) {
	      /* Parse separate display list to get and use that colormap */
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (strlen(token) > (size_t) 0) {
		    strcpy(dlDisplay->cmap,token);
		}
	    } else if (!strcmp(token,"bclr")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlDisplay->bclr = atoi(token) % DL_MAX_COLORS;
		displayInfo->drawingAreaBackgroundColor =
		  dlDisplay->bclr;
	    } else if (!strcmp(token,"clr")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlDisplay->clr = atoi(token) % DL_MAX_COLORS;
		displayInfo->drawingAreaForegroundColor =
		  dlDisplay->clr;
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

    appendDlElement(displayInfo->dlElementList,dlElement); 
  /* fix up x,y so that 0,0 (old defaults) are replaced */
    if (dlDisplay->object.x <= 0) dlDisplay->object.x = DISPLAY_DEFAULT_X;
    if (dlDisplay->object.y <= 0) dlDisplay->object.y = DISPLAY_DEFAULT_Y;
 
    return dlElement;
}
