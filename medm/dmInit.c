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
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (708-252-2000).
*/
/*****************************************************************************
 *
 *     Original Author : Mark Andersion
 *     Current Author  : Frederick Vong
 *
 * Modification Log:
 * -----------------
 * .01  03-01-95        vong    2.0.0 release
 * .02  09-05-95        vong    2.1.0 release
 *                              - strip chart list is removed
 *                                from the displayInfo structure.
 *                              - a updateTaskList is added to the 
 *                                displayInfo structure.
 * .03  09-08-95        vong    conform to c++ syntax
 * .04  10-03-95        vong    call dmResizeDisplayList() before
 *                              dmTraverseDisplayList() instead of
 *                              using XResizeWindows()
 *                              
 *
 *****************************************************************************
*/

#include "medm.h"

#include <X11/keysym.h>

extern Widget mainShell;

static Position x = 0, y = 0;
static Widget lastShell;


typedef DlElement *(*medmParseFunc)(DisplayInfo *, DlComposite *);
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
         {"falling line",         parseFallingLine},
         {"rising line",          parseRisingLine},
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

int parseFuncTableSize = sizeof(parseFuncTable)/sizeof(ParseFuncEntry);
DlElement *getNextElement(DisplayInfo *pDI, DlComposite *pc, char *token) {
  int i;
  for (i=0; i<parseFuncTableSize; i++) {
    if (!strcmp(token,parseFuncTable[i].name)) {
      return parseFuncTable[i].func(pDI, pc);
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
  displayInfo = (DisplayInfo *) calloc(1,sizeof(DisplayInfo));
  displayInfo->next = NULL;
  displayInfo->newDisplay = True;
  displayInfo->filePtr = NULL;
  displayInfo->displayFileName = NULL;
  displayInfo->hasBeenEditedButNotSaved = False;
  displayInfo->selectedElementsArray = NULL;
  displayInfo->numSelectedElements = 0;
  displayInfo->selectedElementsAreHighlighted = FALSE;
  displayInfo->fromRelatedDisplayExecution = FALSE;

  displayInfo->warningDialog = (Widget)NULL;
  displayInfo->questionDialog = (Widget)NULL;
  displayInfo->questionDialogAnswer = 0;
  displayInfo->shellCommandPromptD = (Widget)NULL;
  displayInfo->prev = displayInfoListTail;
  displayInfoListTail->next = displayInfo;
  displayInfoListTail = displayInfo;

/*
 * go get some data from globalResourceBundle
 */
  displayInfo->drawingAreaBackgroundColor = globalResourceBundle.bclr;
  displayInfo->drawingAreaForegroundColor = globalResourceBundle.clr;

/*
 * startup with traversal mode as specified in globalDisplayListTraversalMode
 */
  displayInfo->traversalMode = globalDisplayListTraversalMode;


/*
 * initialize the DlElement structure/display list pointers
 */
  displayInfo->dlElementListHead = (DlElement *) calloc(1,sizeof(DlElement));
  displayInfo->dlElementListHead->next = NULL;
  displayInfo->dlElementListTail = displayInfo->dlElementListHead;
  displayInfo->dlColormapElement = NULL;

/*
 * initialize strip chart list pointers
 */
  updateTaskInit(displayInfo);


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

  return(displayInfo);
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
  FILE *filePtr,
  char *argsString,
  char *filename,
  char *geometryString,
  Boolean fromRelatedDisplayExecution)
{
  DisplayInfo *displayInfo;
  DlColormap *dlCmap;
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  Arg args[7];
  DlBasicAttribute attr;
  DlDynamicAttribute dynAttr;
  DlElement *dlElement;
  int numPairs;
  DlComposite *dlComposite = NULL;

  initializeGlobalResourceBundle();
/* 
 * allocate a DisplayInfo structure and shell for this display file/list
 */
  displayInfo = allocateDisplayInfo();
  displayInfo->filePtr = filePtr;
  currentDisplayInfo = displayInfo;
  currentDisplayInfo->newDisplay = False;

  if (fromRelatedDisplayExecution)
	displayInfo->fromRelatedDisplayExecution = True;
  else
	displayInfo->fromRelatedDisplayExecution = False;

/*
 * generate the name-value table for macro substitutions (related display)
 */
  if (argsString != NULL) {
	displayInfo->nameValueTable = generateNameValueTable(argsString,
		&numPairs);
	displayInfo->numNameValues = numPairs;
  } else {
	displayInfo->nameValueTable = NULL;
	displayInfo->numNameValues = 0;
  }


/* if first token isn't "file" then bail out! */
  tokenType=getToken(displayInfo,token);
  if (tokenType == T_WORD && !strcmp(token,"file")) {
    DlElement *dlElement;
    dlElement = parseFile(displayInfo);
    if (dlElement) {
      displayInfo->versionNumber = dlElement->structure.file->versionNumber;
      if (displayInfo->versionNumber < 20200) {
        basicAttributeInit(&attr);
        dynamicAttributeInit(&dynAttr);
      }
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
/* set internal name of display to external file name */
  dmSetDisplayFileName(displayInfo,filename);


/*
 * proceed with parsing
 */

  while ( (tokenType=getToken(displayInfo,token)) != T_EOF ) {
    switch (tokenType) {
      case T_WORD : {
        DlElement *pe = 0;

        if (pe = getNextElement(displayInfo,dlComposite,token)) {
          if (displayInfo->versionNumber < 20200) {
            switch (pe->type) {
              case DL_Rectangle :
              case DL_Oval      :
              case DL_Arc       :
              case DL_Text      :
              case DL_Polyline  :
              case DL_Polygon   :
                pe->structure.rectangle->attr = attr;
                if (dynAttr.chan[0] != '\0') {
                  pe->structure.rectangle->dynAttr = dynAttr;
                  dynAttr.chan[0] = '\0';
                }
                break;
            }
          }
	} else
        if (!strcmp(token,"file")) {
	  parseFile(displayInfo);
	} else
        if (!strcmp(token,"display")) {
	  parseDisplay(displayInfo);
	} else
        if (!strcmp(token,"color map") ||
	    !strcmp(token,"<<color map>>")) {
	  dlCmap=parseColormap(displayInfo,displayInfo->filePtr);
	  if (!dlCmap) {
	    /* error - do total bail out */
	    fclose(displayInfo->filePtr);
	    dmRemoveDisplayInfo(displayInfo);
	    return;
	  }
	} else
        if (displayInfo->versionNumber < 20200) {
          if (!strcmp(token,"<<basic atribute>>") ||
	      !strcmp(token,"basic attribute") ||
	      !strcmp(token,"<<basic attribute>>")) {
	    parseOldBasicAttribute(displayInfo,&attr);
	  } else
          if (!strcmp(token,"dynamic attribute") ||
	      !strcmp(token,"<<dynamic attribute>>")) {
	    parseOldDynamicAttribute(displayInfo,&dynAttr);
          }
	}
      }
      default :
        break;
    }
  }

  displayInfo->filePtr = NULL;

/*
 * traverse (execute) this displayInfo and associated display list
 */
#if 0
  dmTraverseDisplayList(displayInfo);

  XtPopup(displayInfo->shell,XtGrabNone);

  {
    int x, y;
    unsigned int w, h;
    int mask;
    mask = XParseGeometry(geometryString,&x,&y,&w,&h);
    if ((mask & XValue) && (mask & YValue)
        && (mask & WidthValue) && (mask & HeightValue)) {
      XMoveResizeWindow(XtDisplay(displayInfo->shell),XtWindow(displayInfo->shell),x,y,w,h);
    } else
    if ((mask & XValue) && (mask & YValue)) {
      XMoveWindow(XtDisplay(displayInfo->shell),XtWindow(displayInfo->shell),x,y);
    } else
    if ((mask & WidthValue) && (mask & HeightValue)) {
      XResizeWindow(XtDisplay(displayInfo->shell),XtWindow(displayInfo->shell),w,h);
    }
  }
#else
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
#endif
}



/*
 * routine which actually parses the display list in the opened file for
 * a composite object
 *   N.B.  this function must be kept in sync with dmDisplayListParse()
 *   which is just prior to this code
 */
void parseCompositeChildren(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel;

  DlBasicAttribute attr;
  DlDynamicAttribute dynAttr;
  DlElement *dlElement;


  nestingLevel = 0;
  if (displayInfo->versionNumber < 20200) {
    basicAttributeInit(&attr);
    dynamicAttributeInit(&dynAttr);
  }

/*
 * proceed with parsing
 */

  do {
    switch( (tokenType=getToken(displayInfo,token)) ) {
      case T_WORD : {
        DlElement *pe = 0;
 
        if (pe = getNextElement(displayInfo,dlComposite,token)) {
          if (displayInfo->versionNumber < 20200) {
            switch (pe->type) {
              case DL_Rectangle :
              case DL_Oval      :
              case DL_Arc       :
              case DL_Text      :
              case DL_Polyline  :
              case DL_Polygon   :
                pe->structure.rectangle->attr = attr;
                if (dynAttr.chan[0] != '\0') {
                  pe->structure.rectangle->dynAttr = dynAttr;
                  dynAttr.chan[0] = '\0';
                }
                break;
            }
          }
        } else
        if (displayInfo->versionNumber < 20200) {
          if (!strcmp(token,"<<basic atribute>>") ||
              !strcmp(token,"basic attribute") ||
              !strcmp(token,"<<basic attribute>>")) {
            parseOldBasicAttribute(displayInfo,&attr);
          } else
          if (!strcmp(token,"dynamic attribute") ||
              !strcmp(token,"<<dynamic attribute>>")) {
            parseOldDynamicAttribute(displayInfo,&dynAttr);
          }
        }
      }
      case T_EQUAL:
	break;
      case T_LEFT_BRACE:
	nestingLevel++; break;
      case T_RIGHT_BRACE:
	nestingLevel--; break;
    }
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
                && (tokenType != T_EOF) );


}

#define DISPLAY_DEFAULT_X 10
#define DISPLAY_DEFAULT_Y 10

DlElement *createDlDisplay(
  DisplayInfo *displayInfo)
{
  DlDisplay *dlDisplay;
  DlElement *dlElement;
 
 
  dlDisplay = (DlDisplay *) malloc(sizeof(DlDisplay));
  if (!dlDisplay) return 0;
 
  objectAttributeInit(&(dlDisplay->object));
  dlDisplay->object.x = DISPLAY_DEFAULT_X;
  dlDisplay->object.y = DISPLAY_DEFAULT_Y;
  dlDisplay->clr = 0;
  dlDisplay->bclr = 1;
  dlDisplay->cmap[0] = '\0';
 
  dlElement = (DlElement *) malloc(sizeof(DlElement));
  if (!dlElement) {
    free(dlDisplay);
    return 0;
  }
  if (!(dlElement = createDlElement(DL_Display,
                    (XtPointer)      dlDisplay,
                    (medmExecProc)   executeDlDisplay,
                    (medmWriteProc)  writeDlDisplay,
										0,0,0))) {
    free(dlDisplay);
  }
 
  return(dlElement);
}

DlElement *parseDisplay(
  DisplayInfo *displayInfo)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlDisplay *dlDisplay;
  DlElement *dlElement = createDlDisplay(displayInfo);
 
  if (!dlElement) return 0;
  dlDisplay = dlElement->structure.display;
 
  do {
        switch( (tokenType=getToken(displayInfo,token)) ) {
            case T_WORD:
                if (!strcmp(token,"object")) {
                        parseObject(displayInfo,&(dlDisplay->object));
                } else if (!strcmp(token,"cmap")) {
/* parse separate display list to get and use that colormap */
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

  appendDlElement(&(displayInfo->dlElementListTail),dlElement); 
  /* fix up x,y so that 0,0 (old defaults) are replaced */
  if (dlDisplay->object.x <= 0) dlDisplay->object.x = DISPLAY_DEFAULT_X;
  if (dlDisplay->object.y <= 0) dlDisplay->object.y = DISPLAY_DEFAULT_Y;
 
  return dlElement;
}
