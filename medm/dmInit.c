
#include "medm.h"

#include <X11/keysym.h>


#define N_MAX_MENU_ELES 20

#define N_EDIT_MENU_ELES 16

#define EDIT_BTN_POSN 1
#define EDIT_CUT_BTN 0
#define EDIT_COPY_BTN 1
#define EDIT_PASTE_BTN 2
#define EDIT_DELETE_BTN 3
#define EDIT_RAISE_BTN 4
#define EDIT_LOWER_BTN 5

#define EDIT_GROUP_BTN 6
#define EDIT_UNGROUP_BTN 7

#define EDIT_ALIGN_BTN 8
#define EDIT_UNSELECT_BTN 9
#define EDIT_SELECT_ALL_BTN 10

#define N_ALIGN_MENU_ELES 2
#define ALIGN_BTN_POSN 12

#define N_HORIZ_ALIGN_MENU_ELES 3
#define HORIZ_ALIGN_BTN_POSN 0

#define ALIGN_HORIZ_LEFT_BTN 0
#define ALIGN_HORIZ_CENTER_BTN 1
#define ALIGN_HORIZ_RIGHT_BTN 2

#define N_VERT_ALIGN_MENU_ELES 3
#define VERT_ALIGN_BTN_POSN 1

#define ALIGN_VERT_TOP_BTN 0
#define ALIGN_VERT_CENTER_BTN 1
#define ALIGN_VERT_BOTTOM_BTN 2




extern Widget mainShell;


static Position x = 0, y = 0;
static Widget lastShell;




/********************************************
 **************** Callbacks *****************
 ********************************************/

static XtCallbackProc editMenuSimpleCallback(
  Widget w,
  int buttonNumber,
  XmAnyCallbackStruct *call_data)
{

/* simply return if no current display */
  if (currentDisplayInfo == NULL) return;


/* (MDA) could be smarter about this too, and not do whole traversals...*/

    switch(buttonNumber) {

	case EDIT_CUT_BTN:
	case EDIT_DELETE_BTN:
	   copyElementsIntoClipboard();
	   deleteElementsInDisplay();
	   break;

	case EDIT_COPY_BTN:
	   copyElementsIntoClipboard();
	   break;

	case EDIT_PASTE_BTN:
	   copyElementsIntoDisplay();
	   break;

	case EDIT_RAISE_BTN:
	   raiseSelectedElements();
	   break;

	case EDIT_LOWER_BTN:
	   lowerSelectedElements();
	   break;

	case EDIT_GROUP_BTN:
	   createDlComposite(currentDisplayInfo);
	   break;

	case EDIT_UNGROUP_BTN:
	   ungroupSelectedElements();
	   break;

	case EDIT_UNSELECT_BTN:
	   unselectElementsInDisplay();
	   break;

	case EDIT_SELECT_ALL_BTN:
	   selectAllElementsInDisplay();
	   break;
    }
}



static XtCallbackProc alignHorizontalMenuSimpleCallback(
  Widget w,
  int buttonNumber,
  XmAnyCallbackStruct *call_data)
{
    switch(buttonNumber) {
/* reuse the TextAlign values here */
	case ALIGN_HORIZ_LEFT_BTN:
	   alignSelectedElements(HORIZ_LEFT);
	   break;
	case ALIGN_HORIZ_CENTER_BTN:
	   alignSelectedElements(HORIZ_CENTER);
	   break;
	case ALIGN_HORIZ_RIGHT_BTN:
	   alignSelectedElements(HORIZ_RIGHT);
	   break;
    }
}


static XtCallbackProc alignVerticalMenuSimpleCallback(
  Widget w,
  int buttonNumber,
  XmAnyCallbackStruct *call_data)
{
    switch(buttonNumber) {
/* reuse the TextAlign values here */
	case ALIGN_VERT_TOP_BTN:
	   alignSelectedElements(VERT_TOP);
	   break;
	case ALIGN_VERT_CENTER_BTN:
	   alignSelectedElements(VERT_CENTER);
	   break;
	case ALIGN_VERT_BOTTOM_BTN:
	   alignSelectedElements(VERT_BOTTOM);
	   break;
    }
}

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
 *** createEditMenu()
 ***
 ***/
Widget createEditMenu(
  unsigned int menuType,		/* XmMENU_POPUP, XmMENU_PULLDOWN */
  Widget parent,
  char *name,
  int postFromButton,		/* for XmMENU_PULLDOWN, specify */
  DisplayInfo *displayInfo)	/* for XmMENU_POPUP, specify    */
{
  Arg args[20];
  int i, j, n;
  Widget menu;
  XmString buttons[N_MAX_MENU_ELES];
  KeySym keySyms[N_MAX_MENU_ELES];
  String accelerators[N_MAX_MENU_ELES];
  XmString acceleratorText[N_MAX_MENU_ELES];
  XmButtonType buttonType[N_MAX_MENU_ELES];
  Widget mainArrangePDM, mainAlignPDM, horizAlignPDM, vertAlignPDM;
  int buttonNumber;

/*
 * create the edit menu pane
 */
  buttons[0] = XmStringCreateSimple("Cut");
  buttons[1] = XmStringCreateSimple("Copy");
  buttons[2] = XmStringCreateSimple("Paste");
  buttons[3] = XmStringCreateSimple("Separator");
  buttons[4] = XmStringCreateSimple("Delete");
  buttons[5] = XmStringCreateSimple("Separator");
  buttons[6] = XmStringCreateSimple("Raise");
  buttons[7] = XmStringCreateSimple("Lower");
  buttons[8] = XmStringCreateSimple("Separator");

  buttons[9] = XmStringCreateSimple("Group");
  buttons[10] = XmStringCreateSimple("Ungroup");
  buttons[11] = XmStringCreateSimple("Separator");

  buttons[12] = XmStringCreateSimple("Align");
  buttons[13] = XmStringCreateSimple("Separator");
  buttons[14] = XmStringCreateSimple("Unselect");
  buttons[15] = XmStringCreateSimple("Select All");
  keySyms[0] = 't';
  keySyms[1] = 'C';
  keySyms[2] = 'P';
  keySyms[3] = ' ';
  keySyms[4] = 'D';
  keySyms[5] = ' ';
  keySyms[6] = 'R';
  keySyms[7] = 'L';
  keySyms[8] = ' ';

  keySyms[9] = 'G';
  keySyms[10] = 'n';
  keySyms[11] = ' ';

  keySyms[12] = 'A';
  keySyms[13] = ' ';
  keySyms[14] = 'U';
  keySyms[15] = 'S';
  accelerators[0] = "Shift<Key>DeleteChar";
  accelerators[1] = "Ctrl<Key>InsertChar";
  accelerators[2] = "Shift<Key>InsertChar";
  accelerators[3] = "";
  accelerators[4] = "";
  accelerators[5] = "";
  accelerators[6] = "";
  accelerators[7] = "";
  accelerators[8] = "";
  accelerators[9] = "";
  accelerators[10] = "";
  accelerators[11] = "";
  accelerators[12] = "";
  accelerators[13] = "";
  accelerators[14] = "";
  accelerators[15] = "";
  acceleratorText[0] = XmStringCreateSimple("Shift+Del");
  acceleratorText[1] = XmStringCreateSimple("Ctrl+Ins"); 
  acceleratorText[2] = XmStringCreateSimple("Shift+Ins");
  acceleratorText[3] = XmStringCreateSimple("");
  acceleratorText[4] = XmStringCreateSimple("");
  acceleratorText[5] = XmStringCreateSimple("");
  acceleratorText[6] = XmStringCreateSimple("");
  acceleratorText[7] = XmStringCreateSimple("");
  acceleratorText[8] = XmStringCreateSimple("");
  acceleratorText[9] = XmStringCreateSimple("");
  acceleratorText[10] = XmStringCreateSimple("");
  acceleratorText[11] = XmStringCreateSimple("");
  acceleratorText[12] = XmStringCreateSimple("");
  acceleratorText[13] = XmStringCreateSimple("");
  acceleratorText[14] = XmStringCreateSimple("");
  acceleratorText[15] = XmStringCreateSimple("");

  buttonType[0] = XmPUSHBUTTON;
  buttonType[1] = XmPUSHBUTTON;
  buttonType[2] = XmPUSHBUTTON;
  buttonType[3] = XmSEPARATOR;
  buttonType[4] = XmPUSHBUTTON;
  buttonType[5] = XmSEPARATOR;
  buttonType[6] = XmPUSHBUTTON;
  buttonType[7] = XmPUSHBUTTON;
  buttonType[8] = XmSEPARATOR;
  buttonType[9] = XmPUSHBUTTON;
  buttonType[10] = XmPUSHBUTTON;
  buttonType[11] = XmSEPARATOR;
  buttonType[12] = XmCASCADEBUTTON;
  buttonType[13] = XmSEPARATOR;
  buttonType[14] = XmPUSHBUTTON;
  buttonType[15] = XmPUSHBUTTON;
  n = 0;
  XtSetArg(args[n],XmNbuttonCount,N_EDIT_MENU_ELES); n++;
  XtSetArg(args[n],XmNbuttons,buttons); n++;
  XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
  XtSetArg(args[n],XmNbuttonType,buttonType); n++;
  XtSetArg(args[n],XmNbuttonAccelerators,accelerators); n++;
  XtSetArg(args[n],XmNbuttonAcceleratorText,acceleratorText); n++;
  XtSetArg(args[n],XmNsimpleCallback,(XtCallbackProc)editMenuSimpleCallback);
	n++;

  if (menuType == XmMENU_PULLDOWN) {
    XtSetArg(args[n],XmNpostFromButton,postFromButton); n++;
    menu = XmCreateSimplePulldownMenu(parent,name,args,n);
  } else if (menuType == XmMENU_POPUP) {
    XtSetArg(args[n],XmNtearOffModel,XmTEAR_OFF_DISABLED); n++;
    XtSetArg(args[n],XmNuserData,displayInfo); n++;
    menu = XmCreateSimplePopupMenu(parent,name,args,n);
  } else {
    fprintf(stderr,"\ncreateEditMenu: invalid menuType.");
    menu = NULL;
  }

  for (i = 0; i < N_EDIT_MENU_ELES; i++) {
    XmStringFree(buttons[i]);
    XmStringFree(acceleratorText[i]);
  }


/*
 * create the align cascading menu child of edit menu
 */
  buttons[0] = XmStringCreateSimple("Horizontal");
  buttons[1] = XmStringCreateSimple("Vertical");
  keySyms[0] = 'H';
  keySyms[1] = 'V';
  buttonType[0] = XmCASCADEBUTTON;
  buttonType[1] = XmCASCADEBUTTON;
  n = 0;
  XtSetArg(args[n],XmNbuttonCount,N_ALIGN_MENU_ELES); n++;
  XtSetArg(args[n],XmNbuttons,buttons); n++;
  XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
  XtSetArg(args[n],XmNbuttonType,buttonType); n++;
  XtSetArg(args[n],XmNpostFromButton,ALIGN_BTN_POSN); n++;
  XtSetArg(args[n],XmNtearOffModel,XmTEAR_OFF_DISABLED); n++;
  mainAlignPDM = XmCreateSimplePulldownMenu(menu,
	"mainAlignPDM",args,n);
  for (i = 0; i < N_ALIGN_MENU_ELES; i++) XmStringFree(buttons[i]);

/*
 * create the horizontal cascading menu child of align menu
 */
  buttons[0] = XmStringCreateSimple("Left");
  buttons[1] = XmStringCreateSimple("Center");
  buttons[2] = XmStringCreateSimple("Right");
  keySyms[0] = 'L';
  keySyms[1] = 'C';
  keySyms[2] = 'R';
  buttonType[0] = XmPUSHBUTTON;
  buttonType[1] = XmPUSHBUTTON;
  buttonType[2] = XmPUSHBUTTON;
  n = 0;
  XtSetArg(args[n],XmNbuttonCount,N_HORIZ_ALIGN_MENU_ELES); n++;
  XtSetArg(args[n],XmNbuttons,buttons); n++;
  XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
  XtSetArg(args[n],XmNbuttonType,buttonType); n++;
  XtSetArg(args[n],XmNpostFromButton,HORIZ_ALIGN_BTN_POSN); n++;
  XtSetArg(args[n],XmNsimpleCallback,
		(XtCallbackProc)alignHorizontalMenuSimpleCallback); n++;
  XtSetArg(args[n],XmNtearOffModel,XmTEAR_OFF_DISABLED); n++;
  horizAlignPDM = XmCreateSimplePulldownMenu(mainAlignPDM,
	"horizAlignPDM",args,n);
  for (i = 0; i < N_HORIZ_ALIGN_MENU_ELES; i++) XmStringFree(buttons[i]);

/*
 * create the vertical cascading menu child of align menu
 */
  buttons[0] = XmStringCreateSimple("Top");
  buttons[1] = XmStringCreateSimple("Center");
  buttons[2] = XmStringCreateSimple("Bottom");
  keySyms[0] = 'L';
  keySyms[1] = 'C';
  keySyms[2] = 'R';
  buttonType[0] = XmPUSHBUTTON;
  buttonType[1] = XmPUSHBUTTON;
  buttonType[2] = XmPUSHBUTTON;
  n = 0;
  XtSetArg(args[n],XmNbuttonCount,N_VERT_ALIGN_MENU_ELES); n++;
  XtSetArg(args[n],XmNbuttons,buttons); n++;
  XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
  XtSetArg(args[n],XmNbuttonType,buttonType); n++;
  XtSetArg(args[n],XmNpostFromButton,VERT_ALIGN_BTN_POSN); n++;
  XtSetArg(args[n],XmNsimpleCallback,
		(XtCallbackProc)alignVerticalMenuSimpleCallback); n++;
  XtSetArg(args[n],XmNtearOffModel,XmTEAR_OFF_DISABLED); n++;
  vertAlignPDM = XmCreateSimplePulldownMenu(mainAlignPDM,
	"vertAlignPDM",args,n);
  for (i = 0; i < N_VERT_ALIGN_MENU_ELES; i++) XmStringFree(buttons[i]);


  return menu;
}





/***
 ***  displayInfo creation
 ***/


/*
 * create and return a DisplayInfo structure pointer on tail of displayInfoList
 *  this includes a shell (with it's dialogs and event handlers)
 */
DisplayInfo *allocateDisplayInfo(Widget parent)
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

/***
 *** if non-NULL parent was passed in, use that parent as "shell" and
 ***   run in "embedded" MEDM mode, otherwise create a shell and run
 ***   in normal MEDM mode.
 ***/
  if (parent != (Widget) NULL) {

    displayInfo->shell = parent;
  /*
   * attach event handlers to DA in executeStatics.c
   */

  } else {

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
 * create the shell's EDIT popup menu
 */
    displayInfo->editPopupMenu = createEditMenu(XmMENU_POPUP,displayInfo->shell,
	"editPopupMenu",0,displayInfo);

  }


/* 
 * create the shell's EXECUTE popup menu, and attach via event handlers
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

  XtAddEventHandler(displayInfo->shell,ButtonPressMask,False,
	(XtEventHandler)popupMenu,displayInfo);
  XtAddEventHandler(displayInfo->shell,ButtonReleaseMask,False,
	(XtEventHandler)popdownMenu,displayInfo);



  return(displayInfo);
}



/***
 *** entry point for "embedded" MEDM displays
 ***
 ***  macroBuffer is a character string of macro substitutions to be applied
 ***  parent is the parent widget of the display drawing area (recommended
 ***  widget type is drawing area which can resize...)
 ***/
Boolean libMedmImportDisplay(
	char *filename,
	char *macroBuffer,
	Widget parent)
{
  Boolean status;
  FILE *filePtr;

  if (!access(filename,F_OK|R_OK)) {
     filePtr = fopen(filename,"r");
     if (filePtr != NULL) {
	dmDisplayListParse(filePtr,macroBuffer,filename,parent,
		(Boolean)False);
	fclose(filePtr);
        status = True;
     }
  } else {
      fprintf(stderr,
	"\nlibMedmImportDisplay(): can't open display file: \"%s\"",filename);
      status = False;
  }

  return (status);
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
  Widget parent,		/* if non-NULL, then "embedded" MEDM */
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
  int numPairs;
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
  displayInfo = allocateDisplayInfo(parent);
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

  /* if non-NULL, then "embedded" MEDM */
  if (parent == (Widget)NULL) {
    XtPopup(displayInfo->shell,XtGrabNone);
  } else {
    XtManageChild(parent);
    if (displayInfo->drawingArea != (Widget)NULL)
	XtManageChild(displayInfo->drawingArea);
  }


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
