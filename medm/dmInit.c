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

static int parseFuncTableSize = sizeof(parseFuncTable)/sizeof(ParseFuncEntry);

/* DEBUG */
#if 0
DisplayInfo *debugDisplayInfo=NULL;
#endif
/* End DEBUG */

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
    DisplayInfo *displayInfo = (DisplayInfo *)cd;
    Arg args[2];
    char *env = getenv("MEDM_HELP");

    if (shell == lastShell) {
	XtSetArg(args[0],XmNx,x);
	XtSetArg(args[1],XmNy,y);
	XtSetValues(shell,args,2);
    }
    if (env != NULL) addDisplayHelpProtocol(displayInfo);
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
    Widget w;
    int n;
    Arg args[8];

/* Allocate a DisplayInfo structure and shell for this display file/list */
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

    displayInfo->grid = NULL;
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

  /* Create the shell and add callbacks */
    n = 0;
    XtSetArg(args[n],XmNiconName,"display"); n++;
    XtSetArg(args[n],XmNtitle,"display"); n++;
    XtSetArg(args[n],XmNallowShellResize,TRUE); n++;
  /* For highlightOnEnter on pointer motion, this must be set for shells */
    XtSetArg(args[n],XmNkeyboardFocusPolicy,XmPOINTER); n++;
  /* Map window manager menu Close function to application close... */
    XtSetArg(args[n],XmNdeleteResponse,XmDO_NOTHING); n++;
    if (privateCmap) {
	XtSetArg(args[n],XmNcolormap,cmap); n++;
    }
    displayInfo->shell = XtCreatePopupShell("display",topLevelShellWidgetClass,
      mainShell,args,n);
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

  /* Register the callbacks for these protocols */
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
#if 0
		      /* KE: Don't want to do this.  The old format
		       *  relies on them not being reset */
		      /* Reset the attributes to defaults */
			basicAttributeInit(&attr);
			dynamicAttributeInit(&dynAttr);
#endif			
			break;
		    }
		}
	    } else if (displayInfo->versionNumber < 20200) {
	      /* Did not find an element and old file version
	       *   Must be an attribute, which appear before the object does */
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
      /* Did not come from a display that is to be replaced
       *   Allocate a DisplayInfo structure */
	currentDisplayInfo = allocateDisplayInfo();
	currentDisplayInfo->filePtr = filePtr;
	currentDisplayInfo->newDisplay = False;
    } else {
      /* Came from a display that is to be replaced
       *   See if it should be saved */
	if(displayInfo->fromRelatedDisplayExecution == True) {
	  /* Replace this one */
#if 0	    
	    printf("  True: displayInfo->fromRelatedDisplayExecution=%d\n",
	      displayInfo->fromRelatedDisplayExecution);
#endif	    
	    dmCleanupDisplayInfo(displayInfo,False);
	    clearDlDisplayList(displayInfo->dlElementList);
	    displayInfo->filePtr = filePtr;
	    currentDisplayInfo = displayInfo;
	    currentDisplayInfo->newDisplay = False;
	} else {
	  /* This is an original, allocate a new one and save this one */
#if 0	    
	    printf("  False: displayInfo->fromRelatedDisplayExecution=%d\n",
	      displayInfo->fromRelatedDisplayExecution);
#endif	    
	    XtPopdown(displayInfo->shell);
#if 0	    
	    dumpDisplayInfoList(displayInfoListHead,"dmDisplayListParse [1]: displayInfoList");
	    dumpDisplayInfoList(displayInfoSaveListHead,"dmDisplayListParse [1]: displayInfoSaveList");
#endif	    
	    moveDisplayInfoToDisplayInfoSave(displayInfo);
#if 0	    
	    dumpDisplayInfoList(displayInfoListHead,"dmDisplayListParse [2]: displayInfoList");
	    dumpDisplayInfoList(displayInfoSaveListHead,"dmDisplayListParse [2]: displayInfoSaveList");
#endif	    
	    currentDisplayInfo = allocateDisplayInfo();
	    currentDisplayInfo->filePtr = filePtr;
	    currentDisplayInfo->newDisplay = False;
	}
    }
  
  if (fromRelatedDisplayExecution)
	currentDisplayInfo->fromRelatedDisplayExecution = True;
  else
	currentDisplayInfo->fromRelatedDisplayExecution = False;

#if 0
/* KE: Doesn't make sense.  This just sets an argument which is not a pointer */
  fromRelatedDisplayExecution = currentDisplayInfo->fromRelatedDisplayExecution;
#endif  

  /*
   * generate the name-value table for macro substitutions (related display)
   */
    if (argsString) {
	currentDisplayInfo->nameValueTable = generateNameValueTable(argsString,&numPairs);
	currentDisplayInfo->numNameValues = numPairs;
    } else {
	currentDisplayInfo->nameValueTable = NULL;
	currentDisplayInfo->numNameValues = 0;
    }


  /* if first token isn't "file" then bail out! */
    tokenType=getToken(currentDisplayInfo,token);
    if (tokenType == T_WORD && !strcmp(token,"file")) {
	currentDisplayInfo->dlFile = parseFile(currentDisplayInfo);
	if (currentDisplayInfo->dlFile) {
	    currentDisplayInfo->versionNumber = currentDisplayInfo->dlFile->versionNumber;
	    strcpy(currentDisplayInfo->dlFile->name,filename);
	} else {
	    medmPostMsg("dmDisplayListParse: Out of memory\n"
	      "  file: %s\n",filename);
	    currentDisplayInfo->filePtr = NULL;
	    dmRemoveDisplayInfo(currentDisplayInfo);
	    currentDisplayInfo = NULL;
	    return;
	}
    } else {
	medmPostMsg("dmDisplayListParse: Invalid .adl file (Bad first token)\n"
	  "  file: %s\n",filename);
	currentDisplayInfo->filePtr = NULL;
	dmRemoveDisplayInfo(currentDisplayInfo);
	currentDisplayInfo = NULL;
	return;
    }

  /* DEBUG */
#if 0    
    printf("File: %s\n",currentDisplayInfo->dlFile->name);
    if(strstr(currentDisplayInfo->dlFile->name,"sMain.adl")) {
	debugDisplayInfo=displayInfo;
	printf("Set debugDisplayInfo for sMain.adl\n");
    }
#endif
  /* End DEBUG */

    tokenType=getToken(currentDisplayInfo,token);
    if (tokenType ==T_WORD && !strcmp(token,"display")) {
	parseDisplay(currentDisplayInfo);
    }

    tokenType=getToken(currentDisplayInfo,token);
    if (tokenType == T_WORD && 
      (!strcmp(token,"color map") ||
	!strcmp(token,"<<color map>>"))) {
	currentDisplayInfo->dlColormap=parseColormap(currentDisplayInfo,currentDisplayInfo->filePtr);
	if (!currentDisplayInfo->dlColormap) {
	  /* error - do total bail out */
	    fclose(currentDisplayInfo->filePtr);
	    dmRemoveDisplayInfo(currentDisplayInfo);
	    return;
	}
    }

  /*
   * proceed with parsing
   */
    while (parseAndAppendDisplayList(currentDisplayInfo,
      currentDisplayInfo->dlElementList) != T_EOF );

    currentDisplayInfo->filePtr = NULL;

/*
 * traverse (execute) this displayInfo and associated display list
 */
    {
	int x, y;
	unsigned int w, h;
	int mask;

	mask = XParseGeometry(geometryString,&x,&y,&w,&h);

	if ((mask & WidthValue) && (mask & HeightValue)) {
	    dmResizeDisplayList(currentDisplayInfo,w,h);
	}
	dmTraverseDisplayList(currentDisplayInfo);

	XtPopup(currentDisplayInfo->shell,XtGrabNone);

	if ((mask & XValue) && (mask & YValue)) {
	    XMoveWindow(XtDisplay(currentDisplayInfo->shell),
	      XtWindow(currentDisplayInfo->shell),x,y);
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
	    } else if(!strcmp(token,"grid")) {
		parseGrid(displayInfo,&(dlDisplay->grid));
	    } else if(!strcmp(token,"cmap")) {
	      /* Parse separate display list to get and use that colormap */
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (strlen(token) > (size_t) 0) {
		    strcpy(dlDisplay->cmap,token);
		}
	    } else if(!strcmp(token,"bclr")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlDisplay->bclr = atoi(token) % DL_MAX_COLORS;
		displayInfo->drawingAreaBackgroundColor =
		  dlDisplay->bclr;
	    } else if(!strcmp(token,"clr")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlDisplay->clr = atoi(token) % DL_MAX_COLORS;
		displayInfo->drawingAreaForegroundColor =
		  dlDisplay->clr;
	    } else if(!strcmp(token,"gridSpacing")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlDisplay->grid.gridSpacing = atoi(token);
	    } else if(!strcmp(token,"gridOn")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlDisplay->grid.gridOn = atoi(token);
	    } else if(!strcmp(token,"snapToGrid")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlDisplay->grid.snapToGrid = atoi(token);
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
