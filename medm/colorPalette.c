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
 * .01  09-05-95        vong    2.1.0 release
 * .02  09-08-95        vong    conform to c++ syntax
 *
 *****************************************************************************
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

#ifdef EXTENDED_INTERFACE
static Widget openFSD = NULL;
static Widget colorFilePDM = NULL;
#endif


#ifdef __cplusplus
static void fileOpenCallback(Widget w, XtPointer, XtPointer cbs)
#else
static void fileOpenCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
  XmAnyCallbackStruct *call_data = (XmAnyCallbackStruct *) cbs;
  switch(call_data->reason){
	case XmCR_CANCEL:
		XtUnmanageChild(w);
		break;
	case XmCR_OK:
		XtUnmanageChild(w);
		break;
  }
}

#ifdef __cplusplus
static void fileMenuSimpleCallback(Widget, XtPointer cd, XtPointer)
#else
static void fileMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
  int buttonNumber = (int) cd;
#ifdef EXTENDED_INTERFACE
  int n;
  Arg args[10];
  Widget textField;
#endif


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


#ifdef __cplusplus
static void colorPaletteActivateCallback(Widget, XtPointer cd, XtPointer)
#else
static void colorPaletteActivateCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
  int colorIndex = (int) cd;
  DisplayInfo *cdi;
  Widget widget;
  int i;
  Arg args[4];
  Dimension width,height;
  DlElement *dlElement;

/* (MDA) requests to leave color palette up
  XtPopdown(colorS);
 */

  if (currentDisplayInfo) 
    currentColormap = defaultColormap;
  else
    currentColormap = currentDisplayInfo->colormap;

  if (!(cdi = currentDisplayInfo)) return;

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
    globalResourceBundle.cpData[
        elementTypeWhoseColorIsBeingEdittedIndex].data_clr = colorIndex;
    cpUpdateMatrixColors();
    break;
  default :
    return;
  }

  /* update color bar in resource palette */
  /* 9-19-94  vong
   * makesure the resource palette is create before doing XtVaSetValues
   */
  if (resourceMW) {
    XtVaSetValues(resourceEntryElement[elementTypeWhoseColorIsBeingEditted],
		XmNbackground,currentColormap[colorIndex], NULL);
  }

/*
 * update all selected elements in display (this is overkill, but okay for now)
 *      -- not as efficient as it should be (don't update EVERYTHING if only
 *         one item changed!)
 */
  dlElement = FirstDlElement(cdi->selectedDlElementList);
  while (dlElement) {
    DlElement *pE = dlElement->structure.element;
    updateElementFromGlobalResourceBundle(pE);
    if (widget = pE->widget){
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
        break;
      case DATA_CLR_RC:
        XtVaSetValues(widget,XmNbackground,
            currentColormap[globalResourceBundle.data_clr],NULL);
        break;
      }
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
 Widget menuHelpWidget;

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
#if 0
  keySyms[0] = 'F';
  keySyms[1] = 'H';
  n = 0;
  XtSetArg(args[n],XmNbuttonCount,N_MAIN_MENU_ELES); n++;
  XtSetArg(args[n],XmNbuttons,buttons); n++;
  XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
  XtSetArg(args[n],XmNforeground,defaultForeground); n++;
  XtSetArg(args[n],XmNbackground,defaultBackground); n++;
  colorMB = XmCreateSimpleMenuBar(colorMW, "colorMB",args,n);
#endif

  colorMB = XmVaCreateSimpleMenuBar(colorMW, "colorMB",
    XmVaCASCADEBUTTON, buttons[0], 'F',
    XmVaCASCADEBUTTON, buttons[1], 'H',
    NULL);


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
void setCurrentDisplayColorsInColorPalette(
  int rcType,
  int index)	/* for types with color vectors, also specify element */
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

  elementTypeWhoseColorIsBeingEditted = rcType;
  elementTypeWhoseColorIsBeingEdittedIndex = index;
}


