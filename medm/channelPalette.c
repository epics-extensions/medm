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

static Widget channelFilePDM, openFSD;


#define N_MAX_MENU_ELES 5
#define N_MAIN_MENU_ELES 2


/*
 * create the file pulldown menu pane
 */
#define N_FILE_MENU_ELES 5
#define FILE_BTN_POSN 0
#define FILE_OPEN_BTN	 0
#define FILE_SAVE_BTN	 1
#define FILE_SAVE_AS_BTN 2
#define FILE_CLOSE_BTN	 3

/*
 * create the help pulldown menu pane
 */
#define N_HELP_MENU_ELES 1
#define HELP_BTN_POSN 1



/********************************************
 **************** Callbacks *****************
 ********************************************/
static void fileOpenCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    UNREFERENCED(cd);

    switch(((XmAnyCallbackStruct *) cbs)->reason){
    case XmCR_CANCEL:
	XtUnmanageChild(w);
	break;
    case XmCR_OK:
	XtUnmanageChild(w);
	break;
    }
}

static void fileMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    int buttonNumber = (int) cd;
    XmString label;
    int n;
    Arg args[10];
    Widget textField;

    UNREFERENCED(cbs);

    switch(buttonNumber) {
    case FILE_OPEN_BTN:
	if (openFSD == NULL) {
	    n = 0;
	    label = XmStringCreateLocalized(CHANNEL_DIALOG_MASK);
	    XtSetArg(args[n],XmNdirMask,label); n++;
	    XtSetArg(args[n],XmNdialogStyle,
	      XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
	      openFSD = XmCreateFileSelectionDialog(channelFilePDM,
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
    case FILE_CLOSE_BTN:
	XtPopdown(channelS);
	break;
    }
}





/*
 * directly invoked routines...
 */


void createChannel()
{
    Widget paletteSW;

    XmString buttons[N_MAX_MENU_ELES];
    KeySym keySyms[N_MAX_MENU_ELES];
    XmButtonType buttonType[N_MAX_MENU_ELES];
    Widget channelMB;
    Widget channelHelpPDM;
    Widget menuHelpWidget;
    int i, n;
    Arg args[10];

    openFSD = NULL;

/*
 * create a main window in a shell
 */
    n = 0;
    XtSetArg(args[n],XmNiconName,"Channels"); n++;
    XtSetArg(args[n],XmNtitle,"Channel Palette"); n++;
    XtSetArg(args[n],XtNallowShellResize,TRUE); n++;
    XtSetArg(args[n],XmNkeyboardFocusPolicy,XmEXPLICIT); n++;
/* map window manager menu Close function to application close... */
    XtSetArg(args[n],XmNdeleteResponse,XmDO_NOTHING); n++;
    channelS = XtCreatePopupShell("channelS",topLevelShellWidgetClass,
      mainShell,args,n);
    XmAddWMProtocolCallback(channelS,WM_DELETE_WINDOW,
      (XtCallbackProc)wmCloseCallback, (XtPointer)OTHER_SHELL);

    channelMW = XmCreateMainWindow(channelS,"channelMW",NULL,0);


/*
 * create the menu bar
 */
    buttons[0] = XmStringCreateLocalized("File");
    buttons[1] = XmStringCreateLocalized("Help");
    keySyms[0] = 'F';
    keySyms[1] = 'H';
    n = 0;
    XtSetArg(args[n],XmNbuttonCount,N_MAIN_MENU_ELES); n++;
    XtSetArg(args[n],XmNbuttons,buttons); n++;
    XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
    channelMB = XmCreateSimpleMenuBar(channelMW, "channelMB",args,n);

  /* set the Help cascade button in the menu bar */
    menuHelpWidget = XtNameToWidget(channelMB,"*button_1");
    XtVaSetValues(channelMB,XmNmenuHelpWidget,menuHelpWidget,
      NULL);
    for (i = 0; i < N_MAIN_MENU_ELES; i++) XmStringFree(buttons[i]);


/*
 * create the file pulldown menu pane
 */
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
    n = 0;
    XtSetArg(args[n],XmNbuttonCount,N_FILE_MENU_ELES); n++;
    XtSetArg(args[n],XmNbuttons,buttons); n++;
    XtSetArg(args[n],XmNbuttonType,buttonType); n++;
    XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
    XtSetArg(args[n],XmNpostFromButton,FILE_BTN_POSN); n++;
    XtSetArg(args[n],XmNsimpleCallback,(XtCallbackProc)fileMenuSimpleCallback);
    n++;
    channelFilePDM = XmCreateSimplePulldownMenu(channelMB,"channelFilePDM",
      args,n);
    for (i = 0; i < N_FILE_MENU_ELES; i++) XmStringFree(buttons[i]);



/*
 * create the help pulldown menu pane
 */
    buttons[0] = XmStringCreateLocalized("On Channel Palette...");
    keySyms[0] = 'C';
    buttonType[0] = XmPUSHBUTTON;
    n = 0;
    XtSetArg(args[n],XmNbuttonCount,N_HELP_MENU_ELES); n++;
    XtSetArg(args[n],XmNbuttons,buttons); n++;
    XtSetArg(args[n],XmNbuttonType,buttonType); n++;
    XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
    XtSetArg(args[n],XmNpostFromButton,HELP_BTN_POSN); n++;
    channelHelpPDM = XmCreateSimplePulldownMenu(channelMB,
      "channelHelpPDM",args,n);
    XmStringFree(buttons[0]);



/*
 * Add the Palette Radio Box for the drawing channel toggle buttons
 *
 */
    paletteSW = XmCreateScrolledWindow(channelMW,"paletteSW",NULL,0);

    XmMainWindowSetAreas(channelMW,channelMB,NULL,NULL,NULL,paletteSW);


/*
 * manage the composites
 */
    XtManageChild(channelMB);
    XtManageChild(paletteSW);
    XtManageChild(channelMW);

}
