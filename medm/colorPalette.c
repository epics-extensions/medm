/*
 *	Mark Anderson, Argonne National Laboratory:
 *		U.S. DOE, University of Chicago
 */


#include "medm.h"
#include <Xm/MwmUtil.h>

/* menu-related values */
#define N_MAX_MENU_ELES 10
#define N_MAIN_MENU_ELES 2
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
#define N_HELP_MENU_ELES 1
#define HELP_BTN_POSN 1


static Widget globalColorPalettePB[DL_MAX_COLORS];
/*
 * this is a bit ugly, but we use this variable to communicate which
 *	rcType (element in resource palette) is being color-editted
 *	Since this is static and there should only be one of these in
 *	the world (just like one colorPalette) this should be okay
 */
static int elementTypeWhoseColorIsBeingEditted = -1;
static int elementTypeWhoseColorIsBeingEdittedIndex = 0;

static Widget openFSD;
static Widget colorFilePDM;



/********************************************
 **************** Callbacks *****************
 ********************************************/



static XtCallbackProc fileOpenCallback(
  Widget w,
  int btn,
  XmAnyCallbackStruct *call_data)
{
  switch(call_data->reason){
	case XmCR_CANCEL:
		XtUnmanageChild(w);
		break;
	case XmCR_OK:
		XtUnmanageChild(w);
		break;
  }
}


static XtCallbackProc fileMenuSimpleCallback(
  Widget w,
  int buttonNumber,
  XmAnyCallbackStruct *call_data)
{
  XmString label;
  int n;
  Arg args[10];
  Widget textField;

    switch(buttonNumber) {
#ifdef EXTENDED_INTERFACE
	    case FILE_OPEN_BTN:
		if (openFSD == NULL) {
		    n = 0;
		    label = XmStringCreateSimple(COLOR_DIALOG_MASK);
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
				(XtCallbackProc)fileOpenCallback,
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



static void colorPaletteActivateCallback(
  Widget w,
  int colorIndex,
  XmAnyCallbackStruct *call_data)
{
  DisplayInfo *cdi;
  Widget widget;
  int i;
  Arg args[4];
  Position x,y;
  Dimension width,height;

/* (MDA) requests to leave color palette up
  XtPopdown(colorS);
 */

  if (currentDisplayInfo == NULL) 
    currentColormap = defaultColormap;
  else
    currentColormap = currentDisplayInfo->dlColormap;

  cdi = currentDisplayInfo;

/* only proceed if a valid color element... */
  switch (elementTypeWhoseColorIsBeingEditted) {
    case CLR_RC: case BCLR_RC: case DATA_CLR_RC: case SCDATA_RC: case CPDATA_RC:
	break;
    default:
	return;
  }


 switch (elementTypeWhoseColorIsBeingEditted) {
    case CLR_RC:
	globalResourceBundle.clr = colorIndex;
	if (currentDisplayInfo->hasBeenEditedButNotSaved == False) {
	  medmMarkDisplayBeingEdited(currentDisplayInfo);
	}
	break;
    case BCLR_RC:
	globalResourceBundle.bclr = colorIndex;
	if (currentDisplayInfo->hasBeenEditedButNotSaved == False) {
	  medmMarkDisplayBeingEdited(currentDisplayInfo);
	}
	break;
    case DATA_CLR_RC:
	globalResourceBundle.data_clr = colorIndex;
	break;
    case SCDATA_RC:
	globalResourceBundle.scData[
		elementTypeWhoseColorIsBeingEdittedIndex].clr = colorIndex;
	scUpdateMatrixColors();
	break;
    case CPDATA_RC:
	globalResourceBundle.cpData[elementTypeWhoseColorIsBeingEdittedIndex
		].data_clr = colorIndex;
	cpUpdateMatrixColors();
	break;
  }
  /* update color bar in resource palette */
  /* 9-19-94  vong
   * makesure the resource palette is create before doing XtVaSetValues
   */
  if (resourceMW) {
    XtVaSetValues(resourceEntryElement[elementTypeWhoseColorIsBeingEditted],
		XmNbackground,currentColormap[colorIndex], NULL);
  }


/* return if no currentDisplayInfo */
  if (cdi == NULL) return;

/*
 * update all selected elements in display (this is overkill, but okay for now)
 *      -- not as efficient as it should be (don't update EVERYTHING if only
 *         one item changed!)
 */

  for (i = 0; i < cdi->numSelectedElements; i++) {

    updateElementFromGlobalResourceBundle( cdi->selectedElementsArray[i]);
    if (ELEMENT_HAS_WIDGET(cdi->selectedElementsArray[i]->type)){
	widget = lookupElementWidget(cdi,
	  &(cdi->selectedElementsArray[i]->structure.rectangle->object));
	if (widget != NULL) {
	  switch (elementTypeWhoseColorIsBeingEditted) {
	   case CLR_RC:
	    XtVaSetValues(widget,XmNforeground,
		currentColormap[globalResourceBundle.clr],NULL);
	/* if drawingArea: update drawingAreaForegroundColor */
	    if (widget == cdi->drawingArea) {
		cdi->drawingAreaForegroundColor = globalResourceBundle.clr;
	    }
	    break;
	   case BCLR_RC:
	    XtVaSetValues(widget,XmNbackground,
		currentColormap[globalResourceBundle.bclr],NULL);
	/* if drawingArea: update drawingAreaBackgroundColor & pixmap */
	    if (widget == cdi->drawingArea) {
		cdi->drawingAreaBackgroundColor = globalResourceBundle.bclr;
		XSetForeground(display,cdi->pixmapGC,
			cdi->dlColormap[cdi->drawingAreaBackgroundColor]);
		XtVaGetValues(cdi->drawingArea,XmNwidth,&width,
			XmNheight,&height,NULL);
		XFillRectangle(display,cdi->drawingAreaPixmap,
			cdi->pixmapGC, 0, 0,
			(unsigned int)width,(unsigned int)height);
	    }
	    break;
	   case DATA_CLR_RC:
	    XtVaSetValues(widget,XmNbackground,
		currentColormap[globalResourceBundle.data_clr],
		NULL);
	    break;
	  }
	}
    }
  }
  dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);

}








/***********************************************************
 ***********************************************************
 ***********************************************************/
void createColor()
{
 Widget paletteRC, paletteSW;

 XmString buttons[N_MAX_MENU_ELES];
 KeySym keySyms[N_MAX_MENU_ELES];
 XmString label, string;
 XmButtonType buttonType[N_MAX_MENU_ELES];
 Widget colorMB;
 Widget colorHelpPDM;
 Widget menuHelpWidget;

 Pixel fg, bg;
 Widget subjectWidget;
 int i, n, childCount;
 Arg args[10];



 openFSD = NULL;

/*
 * create a main window in a dialog, and then the palette radio box
 */
 n = 0;
 XtSetArg(args[n],XtNiconName,"Colors"); n++;
 XtSetArg(args[n],XtNtitle,"Color Palette"); n++;
 XtSetArg(args[n],XtNallowShellResize,TRUE); n++;
 XtSetArg(args[n],XmNkeyboardFocusPolicy,XmEXPLICIT); n++;
/* map window manager menu Close function to application close... */
 XtSetArg(args[n],XmNdeleteResponse,XmDO_NOTHING); n++;
 XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
 colorS = XtCreatePopupShell("colorS",topLevelShellWidgetClass,
				mainShell,args,n);
 XmAddWMProtocolCallback(colorS,WM_DELETE_WINDOW,
				(XtCallbackProc)wmCloseCallback,
				(XtPointer)OTHER_SHELL);

 colorMW = XmCreateMainWindow(colorS,"colorMW",NULL,0);


/*
 * create the menu bar
 */
  buttons[0] = XmStringCreateSimple("File");
  buttons[1] = XmStringCreateSimple("Help");
  keySyms[0] = 'F';
  keySyms[1] = 'H';
  n = 0;
  XtSetArg(args[n],XmNbuttonCount,N_MAIN_MENU_ELES); n++;
  XtSetArg(args[n],XmNbuttons,buttons); n++;
  XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
  XtSetArg(args[n],XmNforeground,defaultForeground); n++;
  XtSetArg(args[n],XmNbackground,defaultBackground); n++;
  colorMB = XmCreateSimpleMenuBar(colorMW, "colorMB",args,n);

/* color colorMB properly (force so VUE doesn't interfere) */
  colorMenuBar(colorMB,defaultForeground,defaultBackground);

  /* set the Help cascade button in the menu bar */
  menuHelpWidget = XtNameToWidget(colorMB,"*button_1");
  XtVaSetValues(colorMB,XmNmenuHelpWidget,menuHelpWidget,
		NULL);
  for (i = 0; i < N_MAIN_MENU_ELES; i++) XmStringFree(buttons[i]);


/*
 * create the file pulldown menu pane
 */
#ifdef EXTENDED_INTERFACE
  buttons[0] = XmStringCreateSimple("Open...");
  buttons[1] = XmStringCreateSimple("Save");
  buttons[2] = XmStringCreateSimple("Save As...");
  buttons[3] = XmStringCreateSimple("Separator");
  buttons[4] = XmStringCreateSimple("Close");
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
  buttons[0] = XmStringCreateSimple("Close");
  keySyms[0] = 'C';
  buttonType[0] = XmPUSHBUTTON;
#endif
  n = 0;
  XtSetArg(args[n],XmNbuttonCount,N_FILE_MENU_ELES); n++;
  XtSetArg(args[n],XmNbuttons,buttons); n++;
  XtSetArg(args[n],XmNbuttonType,buttonType); n++;
  XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
  XtSetArg(args[n],XmNpostFromButton,FILE_BTN_POSN); n++;
  XtSetArg(args[n],XmNsimpleCallback,(XtCallbackProc)fileMenuSimpleCallback);
	n++;
  colorFilePDM = XmCreateSimplePulldownMenu(colorMB,"colorFilePDM",
	args,n);
  for (i = 0; i < N_FILE_MENU_ELES; i++) XmStringFree(buttons[i]);



/*
 * create the help pulldown menu pane
 */
  buttons[0] = XmStringCreateSimple("On Color Palette...");
  keySyms[0] = 'C';
  buttonType[0] = XmPUSHBUTTON;
  n = 0;
  XtSetArg(args[n],XmNbuttonCount,N_HELP_MENU_ELES); n++;
  XtSetArg(args[n],XmNbuttons,buttons); n++;
  XtSetArg(args[n],XmNbuttonType,buttonType); n++;
  XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
  XtSetArg(args[n],XmNpostFromButton,HELP_BTN_POSN); n++;
  colorHelpPDM = XmCreateSimplePulldownMenu(colorMB,
		"colorHelpPDM",args,n);
  XmStringFree(buttons[0]);
  /* (MDA) for now, disable this menu */
  XtSetSensitive(colorHelpPDM,False);



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
		(XtCallbackProc)colorPaletteActivateCallback,
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
void setCurrentDisplayColorsInColorPalette(
  int rcType,
  int index)	/* for types with color vectors, also specify element */
{
  Arg args[2];
  int i, n, counter;

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
    currentColormap = currentDisplayInfo->dlColormap;
    for (i = 0; i < MIN(currentDisplayInfo->dlColormapCounter,
		DL_MAX_COLORS); i++) {
	XtSetArg(args[0],XmNbackground,currentColormap[i]); n++;
       XtSetValues(globalColorPalettePB[i],args,1);
    }
  }

  elementTypeWhoseColorIsBeingEditted = rcType;
  elementTypeWhoseColorIsBeingEdittedIndex = index;
}


