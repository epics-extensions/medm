
#include "medm.h"

#include <X11/keysym.h>




extern Widget mainShell;


static Position x = 0, y = 0;
static Widget lastShell;

static XtCallbackProc displayShellPopdownCallback(
  Widget shell,
  XtPointer client_data,
  XtPointer call_data)
{
  Arg args[2];
  XtSetArg(args[0],XmNx,&x);
  XtSetArg(args[1],XmNy,&y);
  XtGetValues(shell,args,2);
  lastShell = shell;
}

static XtCallbackProc displayShellPopupCallback(
  Widget shell,
  XtPointer client_data,
  XtPointer call_data)
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
  int n, width, height;
  Arg args[8];


/* 
 * allocate a DisplayInfo structure and shell for this display file/list
 */
  displayInfo = (DisplayInfo *) calloc(1,sizeof(DisplayInfo));
  displayInfo->next = NULL;
  displayInfo->filePtr = NULL;
  displayInfo->displayFileName = NULL;
  displayInfo->useDynamicAttribute = FALSE;
  displayInfo->hasBeenEditedButNotSaved = FALSE;
  displayInfo->selectedElementsArray = NULL;
  displayInfo->numSelectedElements = 0;
  displayInfo->selectedElementsAreHighlighted = FALSE;
  displayInfo->fromRelatedDisplayExecution = FALSE;

  displayInfo->warningDialog = (Widget)NULL;
  displayInfo->shellCommandPromptD = (Widget)NULL;
  displayInfo->prev = displayInfoListTail;
  displayInfoListTail->next = displayInfo;
  displayInfoListTail = displayInfo;

/*
 * go get some data from globalResourceBundle
 */
  displayInfo->drawingAreaBackgroundColor = globalResourceBundle.bclr;
  displayInfo->drawingAreaForegroundColor = globalResourceBundle.clr;

  displayInfo->attribute.clr = globalResourceBundle.clr;
  displayInfo->attribute.style = globalResourceBundle.style;
  displayInfo->attribute.fill = globalResourceBundle.fill;
  displayInfo->attribute.width = globalResourceBundle.lineWidth;

  displayInfo->dynamicAttribute.attr.mod.clr = globalResourceBundle.clrmod;
  displayInfo->dynamicAttribute.attr.mod.vis = globalResourceBundle.vis;
  displayInfo->dynamicAttribute.attr.param.chan[0] = '\0';


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
  displayInfo->stripChartListHead = NULL;
  displayInfo->stripChartListTail = NULL;


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
			(XtCallbackProc)displayShellPopupCallback,NULL);
  XtAddCallback(displayInfo->shell,XmNpopdownCallback,
			(XtCallbackProc)displayShellPopdownCallback,NULL);

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
  displayInfo->editPopupMenu = createEditMenu(XmMENU_POPUP,displayInfo->shell,
	"editPopupMenu",0,displayInfo);

/*
 * attach event handlers for menu activation
 */
  XtAddEventHandler(displayInfo->shell,ButtonPressMask,False,
	(XtEventHandler)popupMenu,displayInfo);
  XtAddEventHandler(displayInfo->shell,ButtonReleaseMask,False,
	(XtEventHandler)popdownMenu,displayInfo);

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
  Boolean fromRelatedDisplayExecution)
{
  DisplayInfo *displayInfo;
  DlColormap *dlCmap;
  long fileSize, i;
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int n, width, height;
  Arg args[7];
  DlDynamicAttribute *dlDynamicAttribute, *localDynamicAttribute;
  DlElement *dlElement, *ele;
  int numPairs, parent;
  DlComposite *dlComposite = NULL;

/* append dynamic attribute if appropriate */
#define APPEND_DYNAMIC_ATTRIBUTE()				\
if (dlDynamicAttribute != (DlDynamicAttribute *)NULL) {		\
  localDynamicAttribute = (DlDynamicAttribute *)malloc(		\
			sizeof(DlDynamicAttribute));		\
  *localDynamicAttribute = *dlDynamicAttribute;			\
								\
  dlElement = (DlElement *) malloc(sizeof(DlElement));		\
  dlElement->type = DL_DynamicAttribute;			\
  dlElement->structure.dynamicAttribute = localDynamicAttribute;\
  dlElement->next = (DlElement *)NULL;				\
  POSITION_ELEMENT_ON_LIST();					\
  dlElement->dmExecute =  (void(*)())executeDlDynamicAttribute;	\
  dlElement->dmWrite =  (void(*)())writeDlDynamicAttribute;	\
}								\


/* 
 * allocate a DisplayInfo structure and shell for this display file/list
 */
  displayInfo = allocateDisplayInfo();
  displayInfo->filePtr = filePtr;
  currentDisplayInfo = displayInfo;

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
    dlDynamicAttribute = NULL;
    parseFile(displayInfo);
  } else {
    displayInfo->filePtr = NULL;
    fprintf(stderr,"\ndmDisplayListParse: invalid .adl file (bad first token)");
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
  
	    case T_WORD     : 
/*
 * statics
 */
		if (!strcmp(token,"basic attribute") ||
		    !strcmp(token,"<<basic attribute>>")) {
			dlDynamicAttribute = NULL;
			parseBasicAttribute(displayInfo,dlComposite);
		} else if (!strcmp(token,"dynamic attribute") ||
			   !strcmp(token,"<<dynamic attribute>>")) {
/* need to propogate dynamicAttribute until:
	another dynamicAttribute
	or a basicAttribute
	or a non-static (non-graphic) object is found */
			dlDynamicAttribute =
				parseDynamicAttribute(displayInfo,dlComposite);
		} else if (!strcmp(token,"rectangle")) {
			APPEND_DYNAMIC_ATTRIBUTE();
			parseRectangle(displayInfo,dlComposite);
		} else if (!strcmp(token,"oval")) {
			APPEND_DYNAMIC_ATTRIBUTE();
			parseOval(displayInfo,dlComposite);
		} else if (!strcmp(token,"arc")) {
			APPEND_DYNAMIC_ATTRIBUTE();
			parseArc(displayInfo,dlComposite);
		} else if (!strcmp(token,"text")) {
			APPEND_DYNAMIC_ATTRIBUTE();
			parseText(displayInfo,dlComposite);
		} else if (!strcmp(token,"falling line")) {
			APPEND_DYNAMIC_ATTRIBUTE();
			parseFallingLine(displayInfo,dlComposite);
		} else if (!strcmp(token,"rising line")) {
			APPEND_DYNAMIC_ATTRIBUTE();
			parseRisingLine(displayInfo,dlComposite);

/* not really static/graphic objects, but not really monitors or controllers? */
		} else if (!strcmp(token,"related display")) {
			dlDynamicAttribute = NULL;
			parseRelatedDisplay(displayInfo,dlComposite);
		} else if (!strcmp(token,"shell command")) {
			dlDynamicAttribute = NULL;
			parseShellCommand(displayInfo,dlComposite);
/* 
 * monitors
 */
		} else if (!strcmp(token,"bar")) {
			dlDynamicAttribute = NULL;
			parseBar(displayInfo,dlComposite);
		} else if (!strcmp(token,"indicator")) {
			dlDynamicAttribute = NULL;
			parseIndicator(displayInfo,dlComposite);
		} else if (!strcmp(token,"meter")) {
			dlDynamicAttribute = NULL;
			parseMeter(displayInfo,dlComposite);
		} else if (!strcmp(token,"strip chart")) {
			dlDynamicAttribute = NULL;
			parseStripChart(displayInfo,dlComposite);
		} else if (!strcmp(token,"cartesian plot")) {
			dlDynamicAttribute = NULL;
			parseCartesianPlot(displayInfo,dlComposite);
		} else if (!strcmp(token,"surface plot")) {
			dlDynamicAttribute = NULL;
			parseSurfacePlot(displayInfo,dlComposite);
		} else if (!strcmp(token,"text update")) {
			dlDynamicAttribute = NULL;
			parseTextUpdate(displayInfo,dlComposite);
/* 
 * controllers
 */
		} else if (!strcmp(token,"choice button")) {
			dlDynamicAttribute = NULL;
			parseChoiceButton(displayInfo,dlComposite);
/* button and choice button are the same thing */
		} else if (!strcmp(token,"button")) {
			dlDynamicAttribute = NULL;
			parseChoiceButton(displayInfo,dlComposite);
		} else if (!strcmp(token,"message button")) {
			dlDynamicAttribute = NULL;
			parseMessageButton(displayInfo,dlComposite);
		} else if (!strcmp(token,"menu")) {
			dlDynamicAttribute = NULL;
			parseMenu(displayInfo,dlComposite);
		} else if (!strcmp(token,"text entry")) {
			dlDynamicAttribute = NULL;
			parseTextEntry(displayInfo,dlComposite);
		} else if (!strcmp(token,"valuator")) {
			dlDynamicAttribute = NULL;
			parseValuator(displayInfo,dlComposite);
/* 
 * extensions
 */
		} else if (!strcmp(token,"image")) {
			dlDynamicAttribute = NULL;
			parseImage(displayInfo,dlComposite);
		} else if (!strcmp(token,"composite")) {
			dlDynamicAttribute = NULL;
			parseComposite(displayInfo,dlComposite);
		} else if (!strcmp(token,"polyline")) {
			APPEND_DYNAMIC_ATTRIBUTE();
			parsePolyline(displayInfo,dlComposite);
		} else if (!strcmp(token,"polygon")) {
			APPEND_DYNAMIC_ATTRIBUTE();
			parsePolygon(displayInfo,dlComposite);
/*
 * less commonly used statics  (since walks through entire if/else-if put last)
 */
/************************* done as first step to see if valid file 
		} else if (!strcmp(token,"file")) {
			dlDynamicAttribute = NULL;
			parseFile(displayInfo);
**************************/
		} else if (!strcmp(token,"display")) {
			dlDynamicAttribute = NULL;
			parseDisplay(displayInfo);
		} else if (!strcmp(token,"color map") ||
			   !strcmp(token,"<<color map>>")) {
			dlDynamicAttribute = NULL;
			dlCmap=parseColormap(displayInfo,displayInfo->filePtr);
			if (dlCmap == NULL) {
			/* error - do total bail out */
			    fclose(displayInfo->filePtr);
			    dmRemoveDisplayInfo(displayInfo);
			    return;
			}
/* attribute is spelled wrong here on purpose  - to agree with LANL */
		} else if (!strcmp(token,"<<basic atribute>>")) {
			dlDynamicAttribute = NULL;
			parseBasicAttribute(displayInfo,dlComposite);
		}
	}
  }

  displayInfo->filePtr = NULL;

/*
 * traverse (execute) this displayInfo and associated display list
 */
  dmTraverseDisplayList(displayInfo);

  XtPopup(displayInfo->shell,XtGrabNone);


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

  DlDynamicAttribute *dlDynamicAttribute, *localDynamicAttribute;
  DlElement *dlElement, *ele;


  nestingLevel = 0;

/*
 * proceed with parsing
 */

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
  
	    case T_WORD     : 
/*
 * statics
 */

		if (!strcmp(token,"basic attribute") ||
		    !strcmp(token,"<<basic attribute>>")) {
			dlDynamicAttribute = NULL;
			parseBasicAttribute(displayInfo,dlComposite);
		} else if (!strcmp(token,"dynamic attribute") ||
			   !strcmp(token,"<<dynamic attribute>>")) {
/* need to propogate this dynamicAttribute until:
	another dynamicAttribute
	or a basicAttribute
	or a non-static (non-graphic) object is found */
			dlDynamicAttribute =
				parseDynamicAttribute(displayInfo,dlComposite);
		} else if (!strcmp(token,"rectangle")) {
			APPEND_DYNAMIC_ATTRIBUTE();
			parseRectangle(displayInfo,dlComposite);
		} else if (!strcmp(token,"oval")) {
			APPEND_DYNAMIC_ATTRIBUTE();
			parseOval(displayInfo,dlComposite);
		} else if (!strcmp(token,"arc")) {
			APPEND_DYNAMIC_ATTRIBUTE();
			parseArc(displayInfo,dlComposite);
		} else if (!strcmp(token,"text")) {
			APPEND_DYNAMIC_ATTRIBUTE();
			parseText(displayInfo,dlComposite);
		} else if (!strcmp(token,"falling line")) {
			APPEND_DYNAMIC_ATTRIBUTE();
			parseFallingLine(displayInfo,dlComposite);
		} else if (!strcmp(token,"rising line")) {
			APPEND_DYNAMIC_ATTRIBUTE();
			parseRisingLine(displayInfo,dlComposite);

/* not really static/graphic objects, but not really monitors or controllers? */
		} else if (!strcmp(token,"related display")) {
			dlDynamicAttribute = NULL;
			parseRelatedDisplay(displayInfo,dlComposite);
		} else if (!strcmp(token,"shell command")) {
			dlDynamicAttribute = NULL;
			parseShellCommand(displayInfo,dlComposite);
/* 
 * monitors
 */
		} else if (!strcmp(token,"bar")) {
			dlDynamicAttribute = NULL;
			parseBar(displayInfo,dlComposite);
		} else if (!strcmp(token,"indicator")) {
			dlDynamicAttribute = NULL;
			parseIndicator(displayInfo,dlComposite);
		} else if (!strcmp(token,"meter")) {
			dlDynamicAttribute = NULL;
			parseMeter(displayInfo,dlComposite);
		} else if (!strcmp(token,"strip chart")) {
			dlDynamicAttribute = NULL;
			parseStripChart(displayInfo,dlComposite);
		} else if (!strcmp(token,"cartesian plot")) {
			dlDynamicAttribute = NULL;
			parseCartesianPlot(displayInfo,dlComposite);
		} else if (!strcmp(token,"surface plot")) {
			dlDynamicAttribute = NULL;
			parseSurfacePlot(displayInfo,dlComposite);
		} else if (!strcmp(token,"text update")) {
			dlDynamicAttribute = NULL;
			parseTextUpdate(displayInfo,dlComposite);
/* 
 * controllers
 */
		} else if (!strcmp(token,"choice button")) {
			dlDynamicAttribute = NULL;
			parseChoiceButton(displayInfo,dlComposite);
/* button and choice button are the same thing */
		} else if (!strcmp(token,"button")) {
			dlDynamicAttribute = NULL;
			parseChoiceButton(displayInfo,dlComposite);
		} else if (!strcmp(token,"message button")) {
			dlDynamicAttribute = NULL;
			parseMessageButton(displayInfo,dlComposite);
		} else if (!strcmp(token,"menu")) {
			dlDynamicAttribute = NULL;
			parseMenu(displayInfo,dlComposite);
		} else if (!strcmp(token,"text entry")) {
			dlDynamicAttribute = NULL;
			parseTextEntry(displayInfo,dlComposite);
		} else if (!strcmp(token,"valuator")) {
			dlDynamicAttribute = NULL;
			parseValuator(displayInfo,dlComposite);
/* 
 * extensions
 */
		} else if (!strcmp(token,"image")) {
			dlDynamicAttribute = NULL;
			parseImage(displayInfo,dlComposite);
		} else if (!strcmp(token,"composite")) {
			dlDynamicAttribute = NULL;
			parseComposite(displayInfo,dlComposite);
		} else if (!strcmp(token,"polyline")) {
			dlDynamicAttribute = NULL;
			parsePolyline(displayInfo,dlComposite);
		} else if (!strcmp(token,"polygon")) {
			dlDynamicAttribute = NULL;
			parsePolygon(displayInfo,dlComposite);

		} else if (!strcmp(token,"<<basic atribute>>")) {
/* put less commonly used static down here (since walks through if/else-if) */

/* attribute is spelled wrong here on purpose  - to agree with LANL */
			dlDynamicAttribute = NULL;
			parseBasicAttribute(displayInfo,dlComposite);
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


}


#undef APPEND_DYNAMIC_ATTRIBUTE
