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

#include "medm.h"
#include <Xm/MwmUtil.h>

/* menu-related values */
#define N_MAX_MENU_ELES 10
#define N_MAIN_MENU_ELES 1
/* create the file pulldown menu pane */
#ifdef EXTENDED_INTERFACE
# define N_FILE_MENU_ELES 5
# define FILE_OPEN_BTN	 0
# define FILE_SAVE_BTN	 1
# define FILE_SAVE_AS_BTN 2
# define FILE_CLOSE_BTN	 3
#else
# define N_FILE_MENU_ELES 1
# define FILE_CLOSE_BTN	 0
#endif
#define FILE_BTN_POSN 0

/* create the help pulldown menu pane */
#define N_HELP_MENU_ELES 2
#define HELP_BTN_POSN 1

#define HELP_COLOR_PALETTE_BTN 0
#define HELP_COLOR_CONVENTIONS_BTN 1

static void helpColorCallback(Widget,XtPointer,XtPointer);
static Widget globalColorPalettePB[DL_MAX_COLORS];

static menuEntry_t helpMenu[] = {
    { "On Color Palette",  &xmPushButtonGadgetClass, 'P', NULL, NULL, NULL,
      helpColorCallback, (XtPointer)HELP_COLOR_PALETTE_BTN, NULL},
    { "Color Conventions",  &xmPushButtonGadgetClass, 'C', NULL, NULL, NULL,
      helpColorCallback, (XtPointer)HELP_COLOR_CONVENTIONS_BTN, NULL},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL},
};

/*
 * this is a bit ugly, but we use this variable to communicate which
 *	rcType (element in resource palette) is being color-edited
 *	Since this is static and there should only be one of these in
 *	the world (just like one colorPalette) this should be okay
 */
static int elementTypeWhoseColorIsBeingEdited = -1;
static int elementTypeWhoseColorIsBeingEditedIndex = 0;

#ifdef EXTENDED_INTERFACE
static Widget openFSD = NULL;
static Widget colorFilePDM = NULL;
#endif


#if 0
/* Unused */
static void fileOpenCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    XmAnyCallbackStruct *call_data = (XmAnyCallbackStruct *)cbs;

    UNREFERENCED(cd);

    switch(call_data->reason){
    case XmCR_CANCEL:
	XtUnmanageChild(w);
	break;
    case XmCR_OK:
	XtUnmanageChild(w);
	break;
    }
}
#endif

static void fileMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    int buttonNumber = (int) cd;
#ifdef EXTENDED_INTERFACE
    int n;
    Arg args[10];
    Widget textField;
#endif

    UNREFERENCED(cbs);

    switch(buttonNumber) {
#ifdef EXTENDED_INTERFACE
    case FILE_OPEN_BTN:
	if (openFSD == NULL) {
	    n = 0;
	    label = XmStringCreateLocalized(COLOR_DIALOG_MASK);
	    XtSetArg(args[n],XmNdirMask,label); n++;
	    XtSetArg(args[n],XmNdialogStyle,
	      XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
	      openFSD = XmCreateFileSelectionDialog(colorFilePDM,
		"openFSD",args,n);
/* make Filter text field insensitive to prevent user hand-editing dirMask */
	      textField = XmFileSelectionBoxGetChild(openFSD,
		XmDIALOG_FILTER_TEXT);
	      XtSetSensitive(textField,FALSE);
	      XtAddCallback(openFSD,XmNokCallback,
		fileOpenCallback,
		FILE_OPEN_BTN);
	      XtAddCallback(openFSD,XmNcancelCallback,
		(XtCallbackProc)fileOpenCallback,FILE_OPEN_BTN);
	      XmStringFree(label);
	      XtManageChild(openFSD);
	} else {
	    XtManageChild(openFSD);
	}
	break;
    case FILE_SAVE_BTN:
	break;
    case FILE_SAVE_AS_BTN:
	break;
#endif
    case FILE_CLOSE_BTN:
	XtPopdown(colorS);
	break;
    }
}


static void colorPaletteActivateCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    int colorIndex = (int)cd;
    DisplayInfo *cdi;
    Widget widget;
    DlElement *dlElement;
    int recolor = TRUE;

    UNREFERENCED(cbs);


/* (MDA) requests to leave color palette up
   XtPopdown(colorS);
   */

    if (currentDisplayInfo)
      currentColormap = defaultColormap;
    else
      currentColormap = currentDisplayInfo->colormap;

    if (!(cdi = currentDisplayInfo)) return;

    switch (elementTypeWhoseColorIsBeingEdited) {
    case CLR_RC:
	globalResourceBundle.clr = colorIndex;
	medmMarkDisplayBeingEdited(currentDisplayInfo);
	break;
    case BCLR_RC:
	globalResourceBundle.bclr = colorIndex;
	medmMarkDisplayBeingEdited(currentDisplayInfo);
	break;
    case DATA_CLR_RC:
	globalResourceBundle.data_clr = colorIndex;
	break;
    case SCDATA_RC:
#if 0
	globalResourceBundle.scData[
	  elementTypeWhoseColorIsBeingEditedIndex].clr = colorIndex;
#endif
	stripChartUpdateMatrixColors(colorIndex,
	  elementTypeWhoseColorIsBeingEditedIndex);
	recolor = FALSE;
	break;
    case CPDATA_RC:
	globalResourceBundle.cpData[
	  elementTypeWhoseColorIsBeingEditedIndex].data_clr = colorIndex;
#ifdef CARTESIAN_PLOT
	cpUpdateMatrixColors();
#endif
	recolor = FALSE;
	break;
    default :
	return;
    }

  /* Update color bar in resource palette */
  /* 9-19-94  vong
   * Make sure the resource palette is created before doing XtVaSetValues
   */
    if (resourceMW && recolor) {
	XtVaSetValues(resourceEntryElement[elementTypeWhoseColorIsBeingEdited],
	  XmNbackground,currentColormap[colorIndex], NULL);
    }

  /* Update appropriate color property */
    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while (dlElement) {
	DlElement *pE = dlElement->structure.element;
	switch (elementTypeWhoseColorIsBeingEdited) {
	case CLR_RC:
	    updateElementForegroundColorFromGlobalResourceBundle(pE);
	    break;
	case BCLR_RC:
	    updateElementBackgroundColorFromGlobalResourceBundle(pE);
	    break;
	case DATA_CLR_RC:
	  /* KE: This is overkill.  Will update all properties */
	  /* This will not loop over composite children */
	    updateElementFromGlobalResourceBundle(pE);
	    widget = pE->widget;
	    if(widget) {
		XtVaSetValues(widget,XmNbackground,
		  currentColormap[globalResourceBundle.data_clr],NULL);
	    }
	    break;
	}
	dlElement = dlElement->next;
    }
    dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
}

/***********************************************************
 ***********************************************************
 ***********************************************************/
void createColor()
{
    Widget paletteRC;

    XmString buttons[N_MAX_MENU_ELES];
    KeySym keySyms[N_MAX_MENU_ELES];
    XmButtonType buttonType[N_MAX_MENU_ELES];
    Widget colorMB;
    Widget colorHelpPDM;
    Pixel fg, bg;
    int i, n, childCount;
    Arg args[10];
#ifdef EXTENDED_INTERFACE
    openFSD = NULL;
#endif

/*
 * create a main window in a dialog, and then the palette radio box
 */
    n = 0;
    XtSetArg(args[n],XtNiconName,"Colors"); n++;
    XtSetArg(args[n],XtNtitle,"Color Palette"); n++;
    XtSetArg(args[n],XtNallowShellResize,TRUE); n++;
    XtSetArg(args[n],XmNkeyboardFocusPolicy,XmEXPLICIT); n++;
  /* Map window manager menu Close function to application close... */
    XtSetArg(args[n],XmNdeleteResponse,XmDO_NOTHING); n++;
#if OMIT_RESIZE_HANDLES
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
  /* KE: The following is necessary for Exceed, which turns off the
     resize function with the handles.  It should not be necessary */
    XtSetArg(args[n],XmNmwmFunctions, MWM_FUNC_ALL); n++;
#endif
    colorS = XtCreatePopupShell("colorS",topLevelShellWidgetClass,
      mainShell,args,n);
    XmAddWMProtocolCallback(colorS,WM_DELETE_WINDOW,
      (XtCallbackProc)wmCloseCallback,
      (XtPointer)OTHER_SHELL);

    colorMW = XmCreateMainWindow(colorS,"colorMW",NULL,0);

/*
 * create the menu bar
 */
    buttons[0] = XmStringCreateLocalized("File");

    colorMB = XmVaCreateSimpleMenuBar(colorMW, "colorMB",
      XmVaCASCADEBUTTON, buttons[0], 'F',
      NULL);


#if EXPLICITLY_OVERWRITE_CDE_COLORS
  /* Color menu bar explicitly to avoid CDE interference */
    colorMenuBar(colorMB,defaultForeground,defaultBackground);
#endif

  /* Free strings */
    for (i = 0; i < N_MAIN_MENU_ELES; i++) XmStringFree(buttons[i]);

/*
 * create the file pulldown menu pane
 */
#ifdef EXTENDED_INTERFACE
    buttons[0] = XmStringCreateLocalized("Open...");
    buttons[1] = XmStringCreateLocalized("Save");
    buttons[2] = XmStringCreateLocalized("Save As...");
    buttons[3] = XmStringCreateLocalized("Separator");
    buttons[4] = XmStringCreateLocalized("Close");
    keySyms[0] = 'O';
    keySyms[1] = 'S';
    keySyms[2] = 'A';
    keySyms[3] = ' ';
    keySyms[4] = 'C';
    buttonType[0] = XmPUSHBUTTON;
    buttonType[1] = XmPUSHBUTTON;
    buttonType[2] = XmPUSHBUTTON;
    buttonType[3] = XmSEPARATOR;
    buttonType[4] = XmPUSHBUTTON;
#else
    buttons[0] = XmStringCreateLocalized("Close");
    keySyms[0] = 'C';
    buttonType[0] = XmPUSHBUTTON;
#endif
    n = 0;
    XtSetArg(args[n],XmNbuttonCount,N_FILE_MENU_ELES); n++;
    XtSetArg(args[n],XmNbuttons,buttons); n++;
    XtSetArg(args[n],XmNbuttonType,buttonType); n++;
    XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
    XtSetArg(args[n],XmNpostFromButton,FILE_BTN_POSN); n++;
    XtSetArg(args[n],XmNsimpleCallback,fileMenuSimpleCallback);
    n++;
#if 0
    colorFilePDM = XmCreateSimplePulldownMenu(colorMB,"colorFilePDM",
      args,n);
#endif
    XmCreateSimplePulldownMenu(colorMB,"colorFilePDM",
      args,n);
    for (i = 0; i < N_FILE_MENU_ELES; i++) XmStringFree(buttons[i]);



/*
 * create the help pulldown menu pane
 */
    colorHelpPDM = buildMenu(colorMB,XmMENU_PULLDOWN,
      "Help", 'H', helpMenu);
    XtVaSetValues(colorMB, XmNmenuHelpWidget, colorHelpPDM, NULL);
  /* (MDA) for now, disable this menu */
/*     XtSetSensitive(colorHelpPDM,False); */

/*
 * Add the Palette Radio Box for the drawing color toggle buttons
 *
 */
    n = 0;
    XtSetArg(args[n],XmNadjustLast,False); n++;
    XtSetArg(args[n],XmNadjustLast,False); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_COLUMN); n++;
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNnumColumns,(short)(DL_MAX_COLORS/DL_COLORS_COLUMN_SIZE)); n++;
    XtSetArg(args[n],XmNspacing,0); n++;
    paletteRC = XmCreateRowColumn(colorMW,"paletteRC",args,n);
    XtVaGetValues(paletteRC,XmNforeground,&fg,XmNbackground,&bg,NULL);


/*
 * create the (maximum number of) color buttons
 */

    n = 0;
    XtSetArg(args[n],XmNlabelType,XmPIXMAP); n++;
    XtSetArg(args[n],XmNrecomputeSize,False); n++;
    for (childCount = 0; childCount < DL_MAX_COLORS; childCount++) {
      /* use values in default colormap if in valid area of colormap */
	globalColorPalettePB[childCount] = XmCreatePushButton(paletteRC,"colorPB",
	  args,n);
	XtAddCallback(globalColorPalettePB[childCount],XmNactivateCallback,
	  colorPaletteActivateCallback,
	  (XtPointer)childCount);
    }

/* want to keep same shadow colors... but different background */
    for (childCount = 0; childCount < DL_MAX_COLORS; childCount++) {
	XtSetArg(args[0],XmNbackground,(Pixel)defaultColormap[childCount]);
	XtSetValues(globalColorPalettePB[childCount],args,1);
    }

    XmMainWindowSetAreas(colorMW,colorMB,NULL,NULL,NULL,paletteRC);


/*
 * manage the composites
 */
    XtManageChild(colorMB);
    XtManageChildren(globalColorPalettePB,childCount);
    XtManageChild(paletteRC);
    XtManageChild(colorMW);
}


/*
 * function to set the current displayInfo's colors in the color palette
 *	and define the rcType (element type) being modified by any
 *	color palette selections
 */
void setCurrentDisplayColorsInColorPalette(int rcType, int index)
  /* for types with color vectors, also specify element */
{
    Arg args[2];
    int i, n = 0;

/* create the color palette if it doesn't yet exist */
    if (colorMW == NULL) createColor();

    if (currentDisplayInfo == NULL)  {
      /* reset to default colormap */
	currentColormap = defaultColormap;
	for (i = 0; i < DL_MAX_COLORS; i++) {
	    XtSetArg(args[0],XmNbackground,(Pixel)currentColormap[i]);
	    XtSetValues(globalColorPalettePB[i],args,1);
	}
    } else {
	currentColormap = currentDisplayInfo->colormap;
	for (i = 0; i < MIN(currentDisplayInfo->dlColormapCounter,
	  DL_MAX_COLORS); i++) {
	    XtSetArg(args[0],XmNbackground,currentColormap[i]); n++;
	    XtSetValues(globalColorPalettePB[i],args,1);
	}
    }

    elementTypeWhoseColorIsBeingEdited = rcType;
    elementTypeWhoseColorIsBeingEditedIndex = index;
}

static void helpColorCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    int buttonNumber = (int)cd;
    UNREFERENCED(w);
    UNREFERENCED(cbs);

    switch(buttonNumber) {
    case HELP_COLOR_PALETTE_BTN:
	callBrowser(medmHelpPath,"#ColorPalette");
	break;
    case HELP_COLOR_CONVENTIONS_BTN:
	callBrowser(medmHelpPath,"#ColorConventions");
	break;
    }
}
