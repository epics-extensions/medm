/*
*****************************************************************
                          COPYRIGHT NOTIFICATION
*****************************************************************

THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
AND IN ALL SOURCE LISTINGS OF THE CODE

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

#define DEBUG_EVENTS 0
#define DEBUG_RELATED_DISPLAY 0

#include "medm.h"
#include <Xm/MwmUtil.h>

/* Function prototypes */

static Widget createExecuteMenu(DisplayInfo *displayInfo, char *execPath);
static void createExecuteModeMenu(DisplayInfo *displayInfo);
static void displayGetValues(ResourceBundle *pRCB, DlElement *p);
static void displaySetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void displaySetForegroundColor(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable displayDlDispatchTable = {
    createDlDisplay,
    NULL,
    executeDlDisplay,
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

/*
 * Create and fill in widgets for display
 */

DisplayInfo *createDisplay()
{
    DisplayInfo *displayInfo;
    DlElement *dlElement;

  /* Clear currentDisplayInfo - not really one yet */
    currentDisplayInfo = NULL;
    initializeGlobalResourceBundle();

    if (!(displayInfo = allocateDisplayInfo())) return NULL;

  /* General scheme: update  globalResourceBundle, then do creates */
    globalResourceBundle.x = 0;
    globalResourceBundle.y = 0;
    globalResourceBundle.width = DEFAULT_DISPLAY_WIDTH;
    globalResourceBundle.height = DEFAULT_DISPLAY_HEIGHT;
    globalResourceBundle.gridSpacing = DEFAULT_GRID_SPACING;
    globalResourceBundle.gridOn = DEFAULT_GRID_ON;
    globalResourceBundle.snapToGrid = DEFAULT_GRID_SNAP;
    strcpy(globalResourceBundle.name,DEFAULT_FILE_NAME);
    displayInfo->dlFile = createDlFile(displayInfo);
    dlElement = createDlDisplay(NULL);
    if (dlElement) {
	DlDisplay *dlDisplay = dlElement->structure.display;
	dlDisplay->object.x = globalResourceBundle.x;
	dlDisplay->object.y = globalResourceBundle.y;
	dlDisplay->object.width = globalResourceBundle.width;
	dlDisplay->object.height = globalResourceBundle.height;
	dlDisplay->clr = globalResourceBundle.clr;
	dlDisplay->bclr = globalResourceBundle.bclr;
	dlDisplay->grid.gridSpacing = globalResourceBundle.gridSpacing;
	dlDisplay->grid.gridOn = globalResourceBundle.gridOn;
	dlDisplay->grid.snapToGrid = globalResourceBundle.snapToGrid;
	appendDlElement(displayInfo->dlElementList,dlElement);
    } else {
      /* Cleanup up displayInfo */
	return NULL;
    }
  /* Create the colormap, also the pixmap and gc */
    displayInfo->dlColormap = createDlColormap(displayInfo);
  /* Execute all the elements including the display */
    dmTraverseDisplayList(displayInfo);
  /* Pop it up */
    XtPopup(displayInfo->shell,XtGrabNone);
  /* Make it be the current displayInfo */
    currentDisplayInfo = displayInfo;
  /* Refresh the display list dialog box */
    refreshDisplayListDlg();

    return(displayInfo);
}

#define DISPLAY_DEFAULT_X 10
#define DISPLAY_DEFAULT_Y 10
 
DlElement *createDlDisplay(DlElement *p)
{
    DlDisplay *dlDisplay;
    DlElement *dlElement;
 
 
    dlDisplay = (DlDisplay *) malloc(sizeof(DlDisplay));
    if (!dlDisplay) return 0;
    if (p) {
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
 
    if (!(dlElement = createDlElement(DL_Display,(XtPointer)dlDisplay,
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
    DlColormap *dlColormap;
    Arg args[12];
    int nargs;
    DlDisplay *dlDisplay = dlElement->structure.display;
    
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
 
    if (displayInfo->drawingArea == NULL) {
 
	displayInfo->drawingArea = XmCreateDrawingArea(displayInfo->shell,
	  "displayDA",args,nargs);
      /* Add expose & resize  & input callbacks for drawingArea */
	XtAddCallback(displayInfo->drawingArea,XmNexposeCallback,
	  drawingAreaCallback,(XtPointer)displayInfo);
	XtAddCallback(displayInfo->drawingArea,XmNresizeCallback,
	  drawingAreaCallback,(XtPointer)displayInfo);
	XtManageChild(displayInfo->drawingArea);
	
      /* Branch depending on the mode */
	if (displayInfo->traversalMode == DL_EDIT) {
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
 
	} else if (displayInfo->traversalMode == DL_EXECUTE) {
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
	    
	  /* Add in drag/drop translations */
	    XtOverrideTranslations(displayInfo->drawingArea,parsedTranslations);
	}
    } else  {     /* else for if (displayInfo->drawingArea == NULL) */
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
    XtSetArg(args[nargs],XmNiconName,displayInfo->dlFile->name); nargs++;
    XtSetValues(displayInfo->shell,args,nargs);
    medmSetDisplayTitle(displayInfo);
    XtRealizeWidget(displayInfo->shell);

  /* KE: Move it to be consistent with its object values
   * XtSetValues (here or above) or XtMoveWidget (here) do not work
   * Is necessary in part because WM adds borders and title bar,
       moving the shell down when first created */
    XMoveWindow(display,XtWindow(displayInfo->shell),
      dlDisplay->object.x,dlDisplay->object.y);

#if DEBUG_RELATED_DISPLAY
    print("executeDlDisplay: dlDisplay->object=%x\n"
      "  dlDisplay->object.x=%d dlDisplay->object.y=%d\n",
      dlDisplay->object,
      dlDisplay->object.x,dlDisplay->object.y);
#if 0
    print("mainShell: XtIsRealized=%s XtIsManaged=%s\n",
      XtIsRealized(mainShell)?"True":"False",
      XtIsManaged(mainShell)?"True":"False");
    printWindowAttributes(display,XtWindow(mainShell),"MainShell: ");

    print("shell: XtIsRealized=%s XtIsManaged=%s\n",
      XtIsRealized(displayInfo->shell)?"True":"False",
      XtIsManaged(displayInfo->shell)?"True":"False");
    printWindowAttributes(display,XtWindow(displayInfo->shell),"Shell: ");
    
    print("DA: XtIsRealized=%s XtIsManaged=%s\n",
      XtIsRealized(displayInfo->drawingArea)?"True":"False",
      XtIsManaged(displayInfo->drawingArea)?"True":"False");
    printWindowAttributes(display,XtWindow(displayInfo->drawingArea),"DA: ");
#endif    
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

  /* If there is an external colormap file specification, parse/execute it now */
  /*   (Note that executeDlColormap also defines the drawingAreaPixmap) */
     if (strlen(dlDisplay->cmap) > (size_t)1)  {
 	if (dlColormap = parseAndExtractExternalColormap(displayInfo,
	  dlDisplay->cmap)) {
	    executeDlColormap(displayInfo,dlColormap);
	} else {
	    medmPostMsg(1,"executeDlDisplay: Cannnot parse and execute external"
	      " colormap %s\n",dlDisplay->cmap);
	    medmCATerminate();
	    dmTerminateX();
	    exit(-1);
	}
    } else {
	executeDlColormap(displayInfo,displayInfo->dlColormap);
    }
}

static void createExecuteModeMenu(DisplayInfo *displayInfo)
{
    Widget w;
    int nargs;
    Arg args[8];
    static int first = 1, doExec = 0;
    char *execPath = NULL;
    
  /* Get MEDM_EXECUTE_LIST only the first time to speed up creating
   *   successive displays) */
    if(first) {
	first = 0;
	execPath = getenv("MEDM_EXEC_LIST");
	if(execPath && execPath[0]) doExec = 1;
    }

  /* Create the execute-mode popup menu */
    nargs = 0;
    if (doExec) {
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
    XtSetArg(args[nargs],XmNtearOffModel,XmTEAR_OFF_DISABLED); nargs++;
    displayInfo->executePopupMenu = XmCreateSimplePopupMenu(displayInfo->drawingArea,
      "executePopupMenu", args, nargs);
    
  /* Create the execute menu */
    if (doExec) {
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
	while(pcolon = strchr(pcolon,MEDM_PATH_DELIMITER))
	  nbuttons++, pcolon++;
	
      /* Allocate memory */
	types = (XmButtonType *)calloc(nbuttons,sizeof(XmButtonType));
	buttons = (XmString *)calloc(nbuttons,sizeof(XmString));
	execMenuCommandList = (char **)calloc(nbuttons,sizeof(char *));
	  
      /* Parse the items */
	pitem = string;
	for(i=0; i < nbuttons; i++) {
	    pcolon = strchr(pitem,MEDM_PATH_DELIMITER);
	    if(pcolon) *pcolon='\0';
	  /* Text */
	    psemi = strchr(pitem,';');
	    if(!psemi) {
		medmPrintf(1,"\ncreateExecuteMenu: "
		  "Missing semi-colon in MEDM_EXEC_LIST item:\n"
		  "  %s\n",pitem);
		free(types);
		free(buttons);
		free(string);
		return (Widget)0;
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
	free(string);
    }

  /* Create the menu */
    nargs = 0;
    XtSetArg(args[nargs], XmNpostFromButton, EXECUTE_POPUP_MENU_EXECUTE_ID); nargs++;
    XtSetArg(args[nargs], XmNbuttonCount, nbuttons); nargs++;
    XtSetArg(args[nargs], XmNbuttonType, types); nargs++;
    XtSetArg(args[nargs], XmNbuttons, buttons); nargs++;
    XtSetArg(args[nargs], XmNsimpleCallback, executeMenuCallback); nargs++;
    XtSetArg(args[nargs], XmNuserData, displayInfo); nargs++;
    XtSetArg(args[nargs],XmNtearOffModel,XmTEAR_OFF_DISABLED); nargs++;
    w = XmCreateSimplePulldownMenu(displayInfo->executePopupMenu,
      "executePopupMenu", args, nargs);

  /* Return */
    return(w);
}

void writeDlDisplay(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
    int i;
    char indent[16];
    DlDisplay *dlDisplay = dlElement->structure.display;
 
    for (i = 0; i < level; i++) indent[i] = '\t';
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
      currentColormap[globalResourceBundle.bclr];
    currentDisplayInfo->drawingAreaForegroundColor =
      currentColormap[globalResourceBundle.clr];
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
      currentColormap[globalResourceBundle.bclr];
}

static void displaySetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlDisplay *dlDisplay = p->structure.display;

    medmGetValues(pRCB,
      CLR_RC,        &(dlDisplay->clr),
      -1);
    currentDisplayInfo->drawingAreaForegroundColor =
      currentColormap[globalResourceBundle.clr];
}
