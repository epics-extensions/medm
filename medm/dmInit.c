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
    int nargs;
    Arg args[8];

/* Allocate a DisplayInfo structure and shell for this display file/list */
    displayInfo = (DisplayInfo *)malloc(sizeof(DisplayInfo));
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
    displayInfo->pixmapGC = 0;

    displayInfo->traversalMode = globalDisplayListTraversalMode;
    displayInfo->hasBeenEditedButNotSaved = False;
    displayInfo->fromRelatedDisplayExecution = FALSE;

    displayInfo->nameValueTable = NULL;
    displayInfo->numNameValues = 0;

    displayInfo->dlFile = NULL;
    displayInfo->dlColormap = NULL;

  /* Create the shell and add callbacks */
    nargs = 0;
    XtSetArg(args[nargs],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); nargs++;
    XtSetArg(args[nargs],XmNiconName,"display"); nargs++;
    XtSetArg(args[nargs],XmNtitle,"display"); nargs++;
    XtSetArg(args[nargs],XmNallowShellResize,True); nargs++;
  /* Turn resize handles off
   * KE: Is is really good to do this? */
    XtSetArg(args[nargs],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH);
    nargs++;
  /* KE: The following is necessary for Exceed, which turns off the resize
   *   function with the handles.  It should not be necessary */
    XtSetArg(args[nargs],XmNmwmFunctions,MWM_FUNC_ALL); nargs++;
#if 1
  /* For highlightOnEnter on pointer motion, this must be set for shells
   * KE: It seems like the user should set this
   *   highlightOnEnter is set for MessageButton, RelatedDisplay, Shell Command,
   *     TextEntry, Valuator
   *   Seems like it only does something for the Slider and Text Entry */
    XtSetArg(args[nargs],XmNkeyboardFocusPolicy,XmPOINTER); nargs++;
#endif    
  /* Map window manager menu Close function to nothing for now */
    XtSetArg(args[nargs],XmNdeleteResponse,XmDO_NOTHING); nargs++;
    if (privateCmap) {
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
#if 0
			pe->structure.rectangle->dynAttr = dynAttr;
		      /* KE: Don't want to do this.  The old format
		       *  relies on them not being reset */
		      /* Reset the attributes to defaults */
			basicAttributeInit(&attr);
			dynamicAttributeInit(&dynAttr);
#else
		      /* KE: This was what was done in MEDM 2.2.9 */
			if (dynAttr.chan[0][0] != '\0') {
			    pe->structure.rectangle->dynAttr = dynAttr;
			    dynAttr.chan[0][0] = '\0';
			}
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
void dmDisplayListParse(DisplayInfo *displayInfo, FILE *filePtr,
  char *argsString, char *filename,  char *geometryString,
  Boolean fromRelatedDisplayExecution)
{
    DisplayInfo *cdi;
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

    if (!displayInfo) {
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
	    displayInfo->filePtr = filePtr;
	    cdi = currentDisplayInfo = displayInfo;
	    cdi->newDisplay = False;
	} else {
	  /* This is an original, pop it down */
	    XtPopdown(displayInfo->shell);
#if DEBUG_RELATED_DISPLAY > 1
	    dumpDisplayInfoList(displayInfoListHead,"dmDisplayListParse [1]: displayInfoList");
	    dumpDisplayInfoList(displayInfoSaveListHead,"dmDisplayListParse [1]: displayInfoSaveList");
#endif
	  /* Save it if not already saved */
	    moveDisplayInfoToDisplayInfoSave(displayInfo);
#if DEBUG_RELATED_DISPLAY > 1
	    dumpDisplayInfoList(displayInfoListHead,"dmDisplayListParse [2]: displayInfoList");
	    dumpDisplayInfoList(displayInfoSaveListHead,"dmDisplayListParse [2]: displayInfoSaveList");
#endif	    
	    cdi = currentDisplayInfo = allocateDisplayInfo();
	    cdi->filePtr = filePtr;
	    cdi->newDisplay = False;
	}
    }
  
    cdi->fromRelatedDisplayExecution = fromRelatedDisplayExecution;
    
  /* Generate the name-value table for macro substitutions (related display) */
    if (argsString) {
	cdi->nameValueTable = generateNameValueTable(argsString,&numPairs);
	cdi->numNameValues = numPairs;
    } else {
	cdi->nameValueTable = NULL;
	cdi->numNameValues = 0;
    }


  /* If first token isn't "file" then bail out */
    tokenType=getToken(cdi,token);
    if (tokenType == T_WORD && !strcmp(token,"file")) {
	cdi->dlFile = parseFile(cdi);
	if (cdi->dlFile) {
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
	medmPostMsg(1,"dmDisplayListParse: Invalid .adl file (Bad first token)\n"
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

    tokenType=getToken(cdi,token);
    if (tokenType ==T_WORD && !strcmp(token,"display")) {
	parseDisplay(cdi);
    }

    tokenType=getToken(cdi,token);
    if (tokenType == T_WORD && 
      (!strcmp(token,"color map") ||
	!strcmp(token,"<<color map>>"))) {

	DlElement *pE;
	
	cdi->dlColormap=parseColormap(cdi,cdi->filePtr);
	if (!cdi->dlColormap) {
	  /* error - do total bail out */
	    fclose(cdi->filePtr);
	    dmRemoveDisplayInfo(cdi);
	    return;
	}
#if 0	
      /* Since a valid colormap element has been brought into the
         display list, remove the external cmap reference in the
         dlDisplay element */
	if(pE = FirstDlElement(displayInfo->dlElementList)) {
	    pE->structure.display->cmap[0] = '\0';
	}
#endif	
    }

  /* Proceed with parsing */
    while (parseAndAppendDisplayList(cdi,
      cdi->dlElementList) != T_EOF );
    cdi->filePtr = NULL;

  /* The display is the first element */
    pE = FirstDlElement(cdi->dlElementList);

  /* Change DlObject values for x and y to be the same as the original */
    if(displayInfo) {
      /* Note that cdi is not necessarily the same as displayInfo */
#if DEBUG_RELATED_DISPLAY
	print("  Set replaced values: XtIsRealized=%s XtIsManaged=%s\n",
	  XtIsRealized(cdi->shell)?"True":"False",
	  XtIsManaged(cdi->shell)?"True":"False");
	print("  x=%d pE->structure.display->object.x=%d\n"
	  "  y=%d pE->structure.display->object.y=%d\n",
	  x,pE->structure.display->object.x,
	  y,pE->structure.display->object.y);
#endif	
	pE->structure.display->object.x = x;
	pE->structure.display->object.y = y;
    }

  /* Do resizing */
    if(reuse) {
      /* Resize the display to the new size now
       *   (There should be no resize callback yet so it will not try
       *     to resize all the elements) */
	nargs=0;
	XtSetArg(args[nargs],XmNwidth,pE->structure.display->object.width);
	nargs++;
	XtSetArg(args[nargs],XmNheight,pE->structure.display->object.height);
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
	if ((mask & WidthValue) && (mask & HeightValue)) {
	    dmResizeDisplayList(cdi,(Dimension)width,(Dimension)height);
	}
    }

  /* Execute all the elements including the display */
    dmTraverseDisplayList(cdi);
    
  /* Pop it up */
    XtPopup(cdi->shell,XtGrabNone);
#if DEBUG_RELATED_DISPLAY
    print("  After XtPopup: XtIsRealized=%s XtIsManaged=%s\n",
      XtIsRealized(cdi->shell)?"True":"False",
      XtIsManaged(cdi->shell)?"True":"False");
    print("  Current values:\n"
      "    pE->structure.display->object.x=%d\n"
      "    pE->structure.display->object.y=%d\n",
      pE->structure.display->object.x,
      pE->structure.display->object.y);
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
	pE->structure.display->object.x=x;
	pE->structure.display->object.y=y;
#endif	
    } else if(geometryString && *geometryString) {
#if DEBUG_RELATED_DISPLAY
	print("  Move(2)\n");
#endif	
	if ((mask & XValue) && (mask & YValue)) {
	    pE->structure.display->object.x=xg;
	    pE->structure.display->object.y=yg;
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
      pE->structure.display->object.x,
      pE->structure.display->object.y);

#if DEBUG_RELATED_DISPLAY
    print("  Final values:\n"
      "    pE->structure.display->object.x=%d\n"
      "    pE->structure.display->object.y=%d\n",
      pE->structure.display->object.x,
      pE->structure.display->object.y);
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
