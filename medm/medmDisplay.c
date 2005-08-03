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

#define DEBUG_EVENTS 0
#define DEBUG_RELATED_DISPLAY 0
#define DEBUG_CMAP 0
#define DEBUG_GRID 0
#define DEBUG_EXECUTE_MENU 0

#include "medm.h"
#include <Xm/MwmUtil.h>

/* Function prototypes */

static Widget createExecuteMenu(DisplayInfo *displayInfo, char *execPath);
static void createExecuteModeMenu(DisplayInfo *displayInfo);
static void displayGetValues(ResourceBundle *pRCB, DlElement *p);
static void displaySetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void displaySetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void refreshComposite(DlComposite *dlComposite);

static DlDispatchTable displayDlDispatchTable = {
    createDlDisplay,
    NULL,
    executeDlDisplay,
    NULL,
    writeDlDisplay,
    NULL,
    displayGetValues,
    NULL,
    displaySetBackgroundColor,
    displaySetForegroundColor,
    NULL,
    NULL,
    NULL,
    NULL};

/* Create a new, empty display.  Only called in EDIT mode. */
DisplayInfo *createDisplay()
{
    DisplayInfo *displayInfo;
    DlElement *pE;

  /* Clear currentDisplayInfo - not really one yet */
    currentDisplayInfo = NULL;
    initializeGlobalResourceBundle();

    if(!(displayInfo = allocateDisplayInfo())) return NULL;

  /* General scheme: update  globalResourceBundle, then do creates */
    globalResourceBundle.x = 0;
    globalResourceBundle.y = 0;
    globalResourceBundle.width = DEFAULT_DISPLAY_WIDTH;
    globalResourceBundle.height = DEFAULT_DISPLAY_HEIGHT;
    globalResourceBundle.gridSpacing = DEFAULT_GRID_SPACING;
    globalResourceBundle.gridOn = DEFAULT_GRID_ON;
    globalResourceBundle.snapToGrid = DEFAULT_GRID_SNAP;
    strcpy(globalResourceBundle.name, DEFAULT_FILE_NAME);
    displayInfo->dlFile = createDlFile(displayInfo);
    pE = createDlDisplay(NULL);
    if(pE) {
	DlDisplay *dlDisplay = pE->structure.display;
	dlDisplay->object.x = globalResourceBundle.x;
	dlDisplay->object.y = globalResourceBundle.y;
	dlDisplay->object.width = globalResourceBundle.width;
	dlDisplay->object.height = globalResourceBundle.height;
	dlDisplay->clr = globalResourceBundle.clr;
	dlDisplay->bclr = globalResourceBundle.bclr;
	dlDisplay->grid.gridSpacing = globalResourceBundle.gridSpacing;
	dlDisplay->grid.gridOn = globalResourceBundle.gridOn;
	dlDisplay->grid.snapToGrid = globalResourceBundle.snapToGrid;
	appendDlElement(displayInfo->dlElementList,pE);
    } else {
      /* Cleanup up displayInfo */
	return NULL;
    }
  /* Do not create the colormap now.  Do it in the execute method.
     The value of displayInfo->dlColormap is used as a flag to decide
     when and how to create it */
    displayInfo->dlColormap = NULL;
  /* Execute the display (the only element) */
    (pE->run->execute)(displayInfo, pE);
  /* Pop it up */
    XtPopup(displayInfo->shell, XtGrabNone);
  /* Make it be the current displayInfo */
    currentDisplayInfo = displayInfo;
  /* Refresh the display list dialog box */
    refreshDisplayListDlg();

    return(displayInfo);
}

void closeDisplay(Widget w) {
    DisplayInfo *newDisplayInfo;
    DlElement *pE;
    newDisplayInfo = dmGetDisplayInfoFromWidget(w);
    if(newDisplayInfo == currentDisplayInfo) {
      /* Unselect any selected elements */
	unselectElementsInDisplay();
	currentDisplayInfo = NULL;
    }
    if(newDisplayInfo->hasBeenEditedButNotSaved) {
	char warningString[2*MAX_FILE_CHARS];
	char *tmp, *tmp1;

	strcpy(warningString,"Save before closing display :\n");
	tmp = tmp1 = newDisplayInfo->dlFile->name;
	while(*tmp != '\0')
	  if(*tmp++ == MEDM_DIR_DELIMITER_CHAR) tmp1 = tmp;
	strcat(warningString,tmp1);
	dmSetAndPopupQuestionDialog(newDisplayInfo,warningString,"Yes","No","Cancel");
	switch (newDisplayInfo->questionDialogAnswer) {
	case 1 :
	  /* Yes, save display */
	    if(medmSaveDisplay(newDisplayInfo,
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
  /* Remove shells if their executeTime elements are in this display */
    if(executeTimeCartesianPlotWidget || executeTimePvLimitsElement ||
      executeTimeStripChartElement) {
	pE = FirstDlElement(newDisplayInfo->dlElementList);
	while(pE) {
	    if(executeTimeCartesianPlotWidget  &&
	      pE->widget == executeTimeCartesianPlotWidget &&
	      cartesianPlotAxisS) {
		executeTimeCartesianPlotWidget = NULL;
		XtPopdown(cartesianPlotAxisS);
	    }
	    if(executeTimePvLimitsElement  &&
	      pE == executeTimePvLimitsElement &&
	      pvLimitsS) {
		executeTimePvLimitsElement = NULL;
		XtPopdown(pvLimitsS);
	    }
	    if(executeTimeStripChartElement  &&
	      pE == executeTimeStripChartElement &&
	      stripChartS) {
		executeTimeStripChartElement = NULL;
		XtPopdown(stripChartS);
	    }
	    if(!executeTimeCartesianPlotWidget &&
	      !executeTimePvLimitsElement &&
	      !executeTimeStripChartElement) break;
	    pE = pE->next;
	}
    }
  /* Remove newDisplayInfo from displayInfoList and cleanup */
    dmRemoveDisplayInfo(newDisplayInfo);
    if(displayInfoListHead->next == NULL) {
	disableEditFunctions();
    }
}

#define DISPLAY_DEFAULT_X 10
#define DISPLAY_DEFAULT_Y 10

DlElement *createDlDisplay(DlElement *p)
{
    DlDisplay *dlDisplay;
    DlElement *dlElement;


    dlDisplay = (DlDisplay *)malloc(sizeof(DlDisplay));
    if(!dlDisplay) return 0;
    if(p) {
	*dlDisplay = *p->structure.display;
    } else {
	objectAttributeInit(&(dlDisplay->object));
	dlDisplay->object.x = DISPLAY_DEFAULT_X;
	dlDisplay->object.y = DISPLAY_DEFAULT_Y;
	dlDisplay->clr = 0;
	dlDisplay->bclr = 1;
	dlDisplay->cmap[0] = '\0';
	dlDisplay->grid.gridSpacing = DEFAULT_GRID_SPACING;
	dlDisplay->grid.gridOn = DEFAULT_GRID_ON;
	dlDisplay->grid.snapToGrid = DEFAULT_GRID_SNAP;
    }

    if(!(dlElement = createDlElement(DL_Display,(XtPointer)dlDisplay,
      &displayDlDispatchTable))) {
	free(dlDisplay);
    }

    return(dlElement);
}

/*
 * this execute... function is a bit unique in that it can properly handle
 *  being called on extant widgets (drawingArea's/shells) - all other
 *  execute... functions want to always create new widgets (since there
 *  is no direct record of the widget attached to the object/element)
 *  {this can be fixed to save on widget create/destroy cycles later}
 */
void executeDlDisplay(DisplayInfo *displayInfo, DlElement *dlElement)
{
    Arg args[12];
    int nargs;
    DlDisplay *dlDisplay = dlElement->structure.display;
    int new = 0;

#if DEBUG_CMAP
    print("executeDlDisplay: %s\n",displayInfo->dlFile->name);
#endif
#if DEBUG_GRID
    {
	DlElement *dlElement = FirstDlElement(displayInfo->dlElementList);
	DlDisplay *dlDisplay = dlElement->structure.display;

	if(displayInfo->colormap) {
	    print("executeDlDisplay:\n"
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
	    print("executeDlDisplay:\n"
	      "  displayInfo: fg=UNK[%d] bg=UNK[%d] dlDisplay: "
	      "fg=UNK[%d] bg=UNK[%d]\n",
	      displayInfo->drawingAreaForegroundColor,
	      displayInfo->drawingAreaBackgroundColor,
	      dlDisplay->clr,
	      dlDisplay->bclr);
	}
    }
#endif

  /* Set the display's foreground and background colors */
    displayInfo->drawingAreaBackgroundColor = dlDisplay->bclr;
    displayInfo->drawingAreaForegroundColor = dlDisplay->clr;

  /* Set the display's grid information */
    displayInfo->grid = &dlDisplay->grid;

  /* From the DlDisplay structure, we've got drawingArea's dimensions */
    nargs = 0;
    XtSetArg(args[nargs],XmNwidth,(Dimension)dlDisplay->object.width); nargs++;
    XtSetArg(args[nargs],XmNheight,(Dimension)dlDisplay->object.height); nargs++;
    XtSetArg(args[nargs],XmNborderWidth,(Dimension)0); nargs++;
    XtSetArg(args[nargs],XmNmarginWidth,(Dimension)0); nargs++;
    XtSetArg(args[nargs],XmNmarginHeight,(Dimension)0); nargs++;
    XtSetArg(args[nargs],XmNshadowThickness,(Dimension)0); nargs++;
    XtSetArg(args[nargs],XmNresizePolicy,XmRESIZE_NONE); nargs++;
  /* N.B.: don't use userData resource since it is used later on for aspect
   *   ratio-preserving resizes */

    if(displayInfo->drawingArea == NULL) {
	new = 1;
	displayInfo->drawingArea = XmCreateDrawingArea(displayInfo->shell,
	  "displayDA",args,nargs);
      /* Add expose & resize  & input callbacks for drawingArea */
	XtAddCallback(displayInfo->drawingArea,XmNexposeCallback,
	  drawingAreaCallback,(XtPointer)displayInfo);
	XtAddCallback(displayInfo->drawingArea,XmNresizeCallback,
	  drawingAreaCallback,(XtPointer)displayInfo);
	XtManageChild(displayInfo->drawingArea);

      /* Branch depending on the mode */
	if(displayInfo->traversalMode == DL_EDIT) {
	  /* Create the edit-mode popup menu */
	    createEditModeMenu(displayInfo);

	  /* Handle input */
	    XtAddCallback(displayInfo->drawingArea,XmNinputCallback,
	      drawingAreaCallback,(XtPointer)displayInfo);

	  /* Handle key presses */
	    XtAddEventHandler(displayInfo->drawingArea,KeyPressMask,False,
	      handleEditKeyPress,(XtPointer)displayInfo);

	  /* Handle button presses */
	    XtAddEventHandler(displayInfo->drawingArea,ButtonPressMask,False,
	      handleEditButtonPress,(XtPointer)displayInfo);

	  /* Handle enter windows */
	    XtAddEventHandler(displayInfo->drawingArea,EnterWindowMask,False,
	      handleEditEnterWindow,(XtPointer)displayInfo);

	} else if(displayInfo->traversalMode == DL_EXECUTE) {
	  /*
	   *  MDA --- HACK to fix DND visuals problem with SUN server
	   *    Note: This call is in here strictly to satisfy some defect in
	   *    the MIT and other X servers for SUNOS machines
	   *    This is completely unnecessary for HP, DEC, NCD, ...
	   */
#if 0
	  /* KE: This was useless in any event since there was no XtSetValues */
	    XtSetArg(args[0],XmNdropSiteType,XmDROP_SITE_COMPOSITE);
	    XmDropSiteRegister(displayInfo->drawingArea,args,1);
#endif

	  /* Create the execute-mode popup menu */
	    createExecuteModeMenu(displayInfo);

	  /* Handle button presses */
	    XtAddEventHandler(displayInfo->drawingArea,ButtonPressMask,False,
	      handleExecuteButtonPress,(XtPointer)displayInfo);

#if USE_DRAGDROP
	  /* Add in drag/drop translations */
	    XtOverrideTranslations(displayInfo->drawingArea,parsedTranslations);
#endif
	}
    } else  {     /* else for if(displayInfo->drawingArea == NULL) */
      /* Just set the values, drawing area is already created with these values
       * KE: Should be unnecessary except for object.x,y */
	nargs=2;
	XtSetValues(displayInfo->drawingArea,args,nargs);
    }

  /* Do the shell */
    nargs=0;
    XtSetArg(args[nargs],XmNx,(Position)dlDisplay->object.x); nargs++;
    XtSetArg(args[nargs],XmNy,(Position)dlDisplay->object.y); nargs++;
    XtSetArg(args[nargs],XmNwidth,(Dimension)dlDisplay->object.width); nargs++;
    XtSetArg(args[nargs],XmNheight,(Dimension)dlDisplay->object.height); nargs++;
    XtSetValues(displayInfo->shell,args,nargs);
    medmSetDisplayTitle(displayInfo);
    XtRealizeWidget(displayInfo->shell);

#if 0
  /* Mark it to be moved to x, y consistent with object.x,y.
   * XtSetValues, XtMoveWidget, or XMoveWindow do not work here.
   * Needs to be done in expose callback when final x,y are correct.
   * Is necessary in part because WM adds borders and title bar,
   * moving the shell down when first created */
    displayInfo->positionDisplay = True;
#endif

#if DEBUG_RELATED_DISPLAY
    print("executeDlDisplay: dlDisplay->object=%x\n"
      "  dlDisplay->object.x=%d dlDisplay->object.y=%d\n",
      dlDisplay->object,
      dlDisplay->object.x,dlDisplay->object.y);
    {
	Position xpos,ypos;

	nargs=0;
	XtSetArg(args[nargs],XmNx,&xpos); nargs++;
	XtSetArg(args[nargs],XmNy,&ypos); nargs++;
	XtGetValues(displayInfo->shell,args,nargs);
	print("executeDlDisplay: xpos=%d ypos=%d\n",xpos,ypos);
    }
#endif

#if DEBUG_EVENTS
    printEventMasks(display, XtWindow(displayInfo->shell),
      "\n[displayInfo->shell] ");
    printEventMasks(display, XtWindow(displayInfo->drawingArea),
      "\n[displayInfo->drawingArea] ");
#endif

#if DEBUG_CMAP
    print("  dlDisplay->cmap=%s\n",dlDisplay->cmap);
    print("  displayInfo->dlColormap(before)=%x\n",displayInfo->dlColormap);
#endif

  /* If there is no displayInfo->dlColormap yet, try to read it from a
     file */
    if(!displayInfo->dlColormap) {
	char cmap[MAX_TOKEN_LENGTH];

	if(*dlDisplay->cmap)  {
	  /* The cmap string is not blank. There is an external
	     colormap file specified.  Parse the file and get the
	     color map block, then create the colormap.  Note that
	     parseAndExtractExternalColormap will replace cmap with a
	     name that includes the path if the cmap is found in the
	     EPICS_DISPLAY_LIST (the same as it does for related
	     displays).  We want to keep the name the user entered, so
	     we pass the local copy and let it modify that one. */
	    strcpy(cmap, dlDisplay->cmap);
	    displayInfo->dlColormap = parseAndExtractExternalColormap(displayInfo,
	      cmap);
	    if(!displayInfo->dlColormap) {
	      /* Error */
		medmPostMsg(1,"executeDlDisplay: "
		  "Cannnot parse and execute external colormap\n"
		  "  File=%s\n"
		  "  Using the default colormap\n", dlDisplay->cmap);
	    }
	}
      /* If there is still no colormap defined, use the default */
	if(!displayInfo->dlColormap) {
	  /* Write a message unless it is the first time for a new
             display (so that this situation is normal and expected) */
	    if(!new || strcmp(displayInfo->dlFile->name,DEFAULT_FILE_NAME)) {
		medmPostMsg(1,"executeDlDisplay: No colormap defined\n"
		  "  Using the default.\n"
		  "  File=%s\n",displayInfo->dlFile->name);
	    }
	  /* Use the default */
	    displayInfo->dlColormap = createDlColormap(displayInfo);
	    if(!displayInfo->dlColormap) {
	      /* Cannot malloc it */
		medmPostMsg(1,"executeDlDisplay: Cannot allocate a colormap\n"
		  "  File=%s\n",displayInfo->dlFile->name);
	    }
	}
    }

#if DEBUG_CMAP
    print("  displayInfo->dlColormap(after)=%x\n",displayInfo->dlColormap);
#endif

  /* Execute the colormap (Note that executeDlColormap also defines
     the drawingAreaPixmap and statusPixmap) */
    executeDlColormap(displayInfo,displayInfo->dlColormap);
#if DEBUG_RELATED_DISPLAY
    print("executeDlDisplay: Done\n");
#endif
}

static void createExecuteModeMenu(DisplayInfo *displayInfo)
{
    Widget w;
    int nargs;
    Arg args[8];
    static int first = 1, doExec = 0;
    char *execPath = NULL;

  /* Get MEDM_EXECUTE_LIST only the first time to speed up creating
   * successive displays) */
    if(first) {
	first = 0;
	execPath = getenv("MEDM_EXEC_LIST");
	if(execPath && execPath[0]) doExec = 1;
    }

  /* Create the execute-mode popup menu */
    nargs = 0;
    if(doExec) {
      /* Include the Execute item */
	XtSetArg(args[nargs], XmNbuttonCount, NUM_EXECUTE_POPUP_ENTRIES); nargs++;
    } else {
      /* Don't include the Execute item */
	XtSetArg(args[nargs], XmNbuttonCount, NUM_EXECUTE_POPUP_ENTRIES - 1); nargs++;
    }
    XtSetArg(args[nargs], XmNbuttonType, executePopupMenuButtonType); nargs++;
    XtSetArg(args[nargs], XmNbuttons, executePopupMenuButtons); nargs++;
    XtSetArg(args[nargs], XmNsimpleCallback, executePopupMenuCallback); nargs++;
    XtSetArg(args[nargs], XmNuserData, displayInfo); nargs++;
    XtSetArg(args[nargs], XmNtearOffModel,XmTEAR_OFF_DISABLED); nargs++;
    displayInfo->executePopupMenu = XmCreateSimplePopupMenu(displayInfo->drawingArea,
      "executePopupMenu", args, nargs);

  /* Create the execute menu */
    if(doExec) {
	w = createExecuteMenu(displayInfo, execPath);
    }
}

static Widget createExecuteMenu(DisplayInfo *displayInfo, char *execPath)
{
    Widget w;
    static int first = 1;
    static XmButtonType *types = (XmButtonType *)0;
    static XmString *buttons = (XmString *)0;
    static int nbuttons = 0;
    int i, len;
    int nargs;
    Arg args[8];
    char *string, *pitem, *pcolon, *psemi;

  /* Return if we've already tried unsuccessfully */
    if(!first && !nbuttons) return (Widget)0;

  /* Parse MEDM_EXEC_LIST, allocate space, and fill arrays first time */
    if(first) {
	first = 0;

      /* Copy the environment string */
	len = strlen(execPath);
	string = (char *)calloc(len+1,sizeof(char));
	strcpy(string,execPath);

      /* Count the colons to get the number of buttons */
	pcolon = string;
	nbuttons = 1;
	while((pcolon = strchr(pcolon,':'))) {
#ifdef WIN32
	  /* Skip :\, assumed to be part of a path */
	    if(*(pcolon+1) != '\\') nbuttons++;
	    pcolon++;
#else
	    nbuttons++, pcolon++;
#endif
	}

      /* Allocate memory */
	types = (XmButtonType *)calloc(nbuttons,sizeof(XmButtonType));
	buttons = (XmString *)calloc(nbuttons,sizeof(XmString));
	execMenuCommandList = (char **)calloc(nbuttons,sizeof(char *));

      /* Parse the items */
	pitem = string;
	for(i=0; i < nbuttons; i++) {
	    pcolon = strchr(pitem,':');
#ifdef WIN32
	  /* Skip :\, assumed to be part of a path */
	    while(pcolon && *(pcolon+1) == '\\') {
		pcolon = strchr(pcolon+1,':');
	    }
#endif
	    if(pcolon) *pcolon='\0';
	  /* Text */
	    psemi = strchr(pitem,';');
	    if(!psemi) {
		medmPrintf(1,"\ncreateExecuteMenu: "
		  "Missing semi-colon in MEDM_EXEC_LIST item:\n"
		  "  %s\n",pitem);
		nbuttons = i;
		break;
	    } else {
		*psemi='\0';
		buttons[i] = XmStringCreateLocalized(pitem);
		types[i] = XmPUSHBUTTON;
		pitem = psemi+1;
		len = strlen(pitem);
		execMenuCommandList[i]=(char *)calloc(len + 1,sizeof(char));
		strcpy(execMenuCommandList[i],pitem);
	    }
	    pitem = pcolon+1;
	}
	if(string) free(string);
    }

  /* Create the menu */
    if(nbuttons) {
	nargs = 0;
	XtSetArg(args[nargs],
	  XmNpostFromButton, EXECUTE_POPUP_MENU_EXECUTE_ID); nargs++;
	XtSetArg(args[nargs], XmNbuttonCount, nbuttons); nargs++;
	XtSetArg(args[nargs], XmNbuttonType, types); nargs++;
	XtSetArg(args[nargs], XmNbuttons, buttons); nargs++;
	XtSetArg(args[nargs], XmNsimpleCallback, executeMenuCallback); nargs++;
	XtSetArg(args[nargs], XmNuserData, displayInfo); nargs++;
	XtSetArg(args[nargs], XmNtearOffModel, XmTEAR_OFF_DISABLED); nargs++;
	w = XmCreateSimplePulldownMenu(displayInfo->executePopupMenu,
	  "executeMenu", args, nargs);
    } else {
	w = NULL;
    }

#if DEBUG_EXECUTE_MENU
    print("createExecuteMenu: w=%x displayInfo=%x executePopupMenu=%x\n",
      w,displayInfo,displayInfo->executePopupMenu);
#endif

  /* Return */
    return(w);
}

void refreshDisplay(DisplayInfo *displayInfo)
{
    if(globalDisplayListTraversalMode == DL_EXECUTE) {
      /* EXECUTE mode */
	DlElement *pE;

      /* Map and unmap widgets so they will be sure to redraw */
	pE = SecondDlElement(displayInfo->dlElementList);
	while(pE) {
	    if(pE->type == DL_Composite) {
		refreshComposite(pE->structure.composite);
	    } else {
		if(pE->widget && XtIsRealized(pE->widget) &&
		  XtIsManaged(pE->widget) && !pE->hidden) {
		    XUnmapWindow(display, XtWindow(pE->widget));
		    XMapWindow(display, XtWindow(pE->widget));
		}
	    }
	    pE = pE->next;
	}

      /* Repaint the region without clipping */
	if(displayInfo->drawingAreaPixmap != (Pixmap)NULL &&
	  displayInfo->gc != (GC)NULL &&
	  displayInfo->drawingArea != (Widget)NULL) {
	    updateTaskRepaintRect(displayInfo, NULL, True);
	}
    } else {
      /* EDIT mode */
	DlElement *pE;

      /* Redraw to get all widgets in proper stacking order (Uses
       * dlElementList, not selectedDlElementList). */
	if(!displayInfo) return;
	if(IsEmpty(displayInfo->dlElementList)) return;

	unhighlightSelectedElements();

      /* Recreate widgets, not including the display */
	pE = SecondDlElement(displayInfo->dlElementList);
	while(pE) {
	    if(pE->widget) {
	      /* Destroy the widget */
		destroyElementWidgets(pE);
	      /* Recreate it */
		if(pE->run->execute) pE->run->execute(displayInfo,pE);
	    }
	    pE = pE->next;
	}
      /* Cleanup possible damage to non-widgets */
	dmTraverseNonWidgetsInDisplayList(displayInfo);

	highlightSelectedElements();
    }
}

/* Used to recurse into Composites in EXECUTE mode */
static void refreshComposite(DlComposite *dlComposite)
{
    DlElement *pE;

    pE = FirstDlElement(dlComposite->dlElementList);
    while(pE) {
	if(pE->type == DL_Composite) {
	    refreshComposite(pE->structure.composite);
	} else {
	    if(pE->widget && XtIsRealized(pE->widget) &&
	      XtIsManaged(pE->widget) && !pE->hidden) {
		XUnmapWindow(display, XtWindow(pE->widget));
		XMapWindow(display, XtWindow(pE->widget));
	    }
	}
	pE = pE->next;
    }
}

DlElement *parseDisplay(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlDisplay *dlDisplay;
    DlElement *dlElement = createDlDisplay(NULL);

    if(!dlElement) return 0;
    dlDisplay = dlElement->structure.display;

    do {
        switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlDisplay->object));
	    } else if(!strcmp(token,"grid")) {
		parseGrid(displayInfo,&(dlDisplay->grid));
	    } else if(!strcmp(token,"cmap")) {
	      /* Parse separate display list to get and use that colormap */
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(strlen(token) > (size_t) 0) {
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
	    nestingLevel++;
	    break;
	case T_RIGHT_BRACE:
	    nestingLevel--;
	    break;
	default:
	    break;
        }
    } while( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );

    appendDlElement(displayInfo->dlElementList,dlElement);
  /* fix up x,y so that 0,0 (old defaults) are replaced */
    if(dlDisplay->object.x <= 0) dlDisplay->object.x = DISPLAY_DEFAULT_X;
    if(dlDisplay->object.y <= 0) dlDisplay->object.y = DISPLAY_DEFAULT_Y;

    return dlElement;
}

void writeDlDisplay(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
    int i;
    char indent[16];
    DlDisplay *dlDisplay = dlElement->structure.display;

    for(i = 0; i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%sdisplay {",indent);
    writeDlObject(stream,&(dlDisplay->object),level+1);
    fprintf(stream,"\n%s\tclr=%d",indent,dlDisplay->clr);
    fprintf(stream,"\n%s\tbclr=%d",indent,dlDisplay->bclr);
    fprintf(stream,"\n%s\tcmap=\"%s\"",indent,dlDisplay->cmap);
    fprintf(stream,"\n%s\tgridSpacing=%d",indent,dlDisplay->grid.gridSpacing);
    fprintf(stream,"\n%s\tgridOn=%d",indent,dlDisplay->grid.gridOn);
    fprintf(stream,"\n%s\tsnapToGrid=%d",indent,dlDisplay->grid.snapToGrid);
    fprintf(stream,"\n%s}",indent);
}

static void displayGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlDisplay *dlDisplay = p->structure.display;
    medmGetValues(pRCB,
      X_RC,            &(dlDisplay->object.x),
      Y_RC,            &(dlDisplay->object.y),
      WIDTH_RC,        &(dlDisplay->object.width),
      HEIGHT_RC,       &(dlDisplay->object.height),
      CLR_RC,          &(dlDisplay->clr),
      BCLR_RC,         &(dlDisplay->bclr),
      CMAP_RC,         &(dlDisplay->cmap),
      GRID_SPACING_RC, &(dlDisplay->grid.gridSpacing),
      GRID_ON_RC,      &(dlDisplay->grid.gridOn),
      GRID_SNAP_RC,    &(dlDisplay->grid.snapToGrid),
      -1);
    currentDisplayInfo->drawingAreaBackgroundColor =
      globalResourceBundle.bclr;
    currentDisplayInfo->drawingAreaForegroundColor =
      globalResourceBundle.clr;
  /* Resize the shell */
    XtVaSetValues(currentDisplayInfo->shell,
      XmNx,dlDisplay->object.x,
      XmNy,dlDisplay->object.y,
      XmNwidth,dlDisplay->object.width,
      XmNheight,dlDisplay->object.height,NULL);
}

static void displaySetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlDisplay *dlDisplay = p->structure.display;

    medmGetValues(pRCB,
      BCLR_RC,       &(dlDisplay->bclr),
      -1);
    currentDisplayInfo->drawingAreaBackgroundColor =
      globalResourceBundle.bclr;
}

static void displaySetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlDisplay *dlDisplay = p->structure.display;

    medmGetValues(pRCB,
      CLR_RC,        &(dlDisplay->clr),
      -1);
    currentDisplayInfo->drawingAreaForegroundColor =
      globalResourceBundle.clr;
}
