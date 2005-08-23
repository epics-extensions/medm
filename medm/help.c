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

#define DEBUG_STATISTICS 0
#define DEBUG_ERRORHANDLER 0

#define MAX_ERRORS 25

#define TIME_STRING_MAX 81
#define EARLY_MESSAGE_SIZE 2048

#define ERR_MSG_RAISE_BTN      0
#define ERR_MSG_CLOSE_BTN      1
#define ERR_MSG_CLEAR_BTN      2
#define ERR_MSG_PRINT_BTN      3
#define ERR_MSG_MAINWINDOW_BTN 4
#define ERR_MSG_SEND_BTN       5
#define ERR_MSG_HELP_BTN       6

#define CASTUDY_INITIAL_UPDATE (XtPointer)1
#define CASTUDY_REDRAW_ONLY    (XtPointer)2

#define CASTUDYINTERVAL 5000
#define MIN_SIGNIFICANT_TIME_INTERVAL .01

#include "medm.h"
#include <time.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>         /* system() */

#include <Xm/Protocols.h>

#ifdef WIN32
#include <process.h>
#else
#include <sys/types.h>
#endif

/* Function prototypes */

static void displayHelpCallback(Widget shell, XtPointer client_data,
  XtPointer call_data);
static int saveEarlyMessage(char *msg);
static void copyEarlyMessages(void);

/* Global variables */

static char *earlyMessages = NULL;
static int earlyMessagesDone = 0;

/* popen, pclose  may not be defined for strict ANSI */
extern FILE *popen(const char *, const char *);
extern int pclose(FILE *);

static Widget errMsgText = NULL;
static Widget errMsgSendSubjectText = NULL;
static Widget errMsgSendToText = NULL;
static Widget errMsgSendText = NULL;
static XtIntervalId caStudyDlgTimeOutId = 0;

void errMsgSendDlgCreateDlg();
void errMsgSendDlgSendButtonCb(Widget w, XtPointer clientData, XtPointer callData);
void errMsgSendDlgCloseButtonCb(Widget w, XtPointer clientData, XtPointer callData);
void errMsgDlgCb(Widget w, XtPointer clientData, XtPointer callData);
static void medmUpdateCAStudyDlg(XtPointer data, XtIntervalId *id);

/* Global variables */

static Widget raiseMessageWindowTB;

void globalHelpCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    int helpIndex = (int) cd;

    UNREFERENCED(cbs);

    switch (helpIndex) {
    case HELP_MAIN:
	callBrowser(medmHelpPath,"#InitialLocations");
	break;
    }
}

void errMsgDlgCb(Widget w, XtPointer clientData, XtPointer callData)
{
    int button = (int)clientData;

    UNREFERENCED(callData);

    switch(button) {
    case ERR_MSG_RAISE_BTN:
	medmRaiseMessageWindow = XmToggleButtonGetState(raiseMessageWindowTB);
	break;
    case ERR_MSG_CLOSE_BTN:
	if(errMsgS != NULL) {
	    XtUnmanageChild(errMsgS);
	}
	break;
    case ERR_MSG_CLEAR_BTN:
	{
	    time_t now;
	    struct tm *tblock;
	    char timeStampStr[TIME_STRING_MAX];
	    XmTextPosition curpos = 0;

	    if(errMsgText == NULL) return;

	  /* Clear the buffer */
	    XmTextSetString(errMsgText,"");
	    XmTextSetInsertionPosition(errMsgText, 0);

	  /* Reinitialize */
	    time(&now);
	    tblock = localtime(&now);
	    strftime(timeStampStr,TIME_STRING_MAX,
	      "Message Log cleared at "STRFTIME_FORMAT"\n",tblock);
	    timeStampStr[TIME_STRING_MAX-1]='0';
	    XmTextInsert(errMsgText, curpos, timeStampStr);
	    curpos+=strlen(timeStampStr);
	    sprintf(timeStampStr,"MEDM PID: %d Window ID: 0x%x",
	      getpid(),(unsigned int)XtWindow(mainShell));
	    timeStampStr[TIME_STRING_MAX-1]='0';
	    XmTextInsert(errMsgText, curpos, timeStampStr);
	    curpos+=strlen(timeStampStr);
	    XmTextSetInsertionPosition(errMsgText, curpos);
	}
	break;
    case ERR_MSG_MAINWINDOW_BTN:
	{
	  /* KE: This appears to work and deiconify if iconic */
	    XMapRaised(display, XtWindow(mainShell));
	}
	break;
    case ERR_MSG_PRINT_BTN:
	{
	    time_t now;
	    struct tm *tblock;
	    FILE *file;
	    char timeStampStr[TIME_STRING_MAX];
	    char *tmp, *psFileName, *commandBuffer;

	    if(errMsgText == NULL) {
		XBell(display,50); XBell(display,50); XBell(display,50);
		return;
	    }
	    if(printToFile && !*printFile) {
		medmPrintf(1,"\nerrMsgDlgCb: "
		  "Printer Setup specifies Print To File, but there is no filename\n");
		XBell(display,50); XBell(display,50); XBell(display,50);
		return;
	    }
	    if(!printToFile && !*printCommand) {
		medmPrintf(1,"\nerrMsgDlgCb: "
		  "Printer Setup specifies Print To File, but there is no filename\n");
		XBell(display,50); XBell(display,50); XBell(display,50);
		return;
	    }
#ifdef WIN32
	    if(!printToFile) {
		medmPrintf(1,"\nerrMsgDlgCb: "
		  "Printing to a printer is not available for WIN32\n"
		  "  You can print to a file by using Print Setup\n");
		XBell(display,50); XBell(display,50); XBell(display,50);
		return;
	    }
#endif

	  /* Get selection and timestamp */
	    time(&now);
	    tblock = localtime(&now);
	    tmp = XmTextGetSelection(errMsgText);
	    strftime(timeStampStr,TIME_STRING_MAX,
	      "MEDM Message Window Selection at "STRFTIME_FORMAT":\n\n",
	      tblock);
	    if(tmp == NULL) {
		tmp = XmTextGetString(errMsgText);
		strftime(timeStampStr,TIME_STRING_MAX,
		  "MEDM Message Window at "STRFTIME_FORMAT":\n\n",tblock);
	    }
	    timeStampStr[TIME_STRING_MAX-1]='0';

	  /* Create filename */
	    psFileName = (char *)calloc(1,MAX_TOKEN_LENGTH);
	    if(printToFile) {
		strcpy(psFileName,printFile);
	    } else {
		sprintf(psFileName,"/tmp/medmMessageWindowPrintFile%d",
		  getpid());
	    }

	  /* Write file */
	    file = fopen(psFileName,"w+");
	    if(file == NULL) {
		medmPrintf(1,"\nerrMsgDlgCb: Unable to open file: %s",
		  psFileName);
		XtFree(tmp);
		free(psFileName);
		XBell(display,50); XBell(display,50); XBell(display,50);
		return;
	    }
	    fprintf(file,timeStampStr);
	    fprintf(file,tmp);
	    XtFree(tmp);
	    fclose(file);

	  /* Print file */
	    if(printToFile) {
		xInfoMsg(w,"Output sent to:\n  %s\n",psFileName);
	    } else {
		commandBuffer = (char *)calloc(1,MAX_TOKEN_LENGTH+256);
		sprintf(commandBuffer,"%s %s",printCommand,psFileName);
		system(commandBuffer);

	      /* Delete file */
		sprintf(commandBuffer,"rm %s",psFileName);
		system(commandBuffer);
		free(commandBuffer);
	    }

	  /* Clean up */
	    free(psFileName);
	    XBell(display,50);
	}
	break;
    case ERR_MSG_SEND_BTN:
	{
	    char *tmp;

	    if(errMsgText == NULL) return;
	    if(errMsgSendS == NULL) {
		errMsgSendDlgCreateDlg();
	    }
	    XmTextSetString(errMsgSendToText,"");
	    XmTextSetString(errMsgSendSubjectText,
	      "MEDM Message Window Contents");
	    tmp = XmTextGetSelection(errMsgText);
	    if(tmp == NULL) {
		tmp = XmTextGetString(errMsgText);
	    }
	    XmTextSetString(errMsgSendText,tmp);
	    XtFree(tmp);
	    XtManageChild(errMsgSendS);
	}
	break;
    case ERR_MSG_HELP_BTN:
	callBrowser(medmHelpPath,"#MessageWindow");
	break;
    }
}

void errMsgDlgCreateDlg(int raise)
{
    Widget pane;
    Widget actionArea;
    Widget optionArea;
    Widget closeButton, sendButton, clearButton, printButton, helpButton;
    XmString label;
    Arg args[10];
    int n;

    if(errMsgS != NULL) {
	if(raise) {
	    Window w=XtWindow(errMsgS);;

	    if(XtIsManaged(errMsgS)) XtUnmanageChild(errMsgS);
	    XtManageChild(errMsgS);
	  /* Raise the window if it exists (widget is realized) */
	    if(w) XRaiseWindow(display,w);
	}
	return;
    }

    if(mainShell == NULL) return;

    errMsgS = XtVaCreatePopupShell("errorMsgS",
#if 0
    /* KE: Gets iconized this way */
      xmDialogShellWidgetClass, mainShell,
#endif
      topLevelShellWidgetClass, mainShell,
      XmNtitle, "MEDM Message Window",
      XmNdeleteResponse, XmDO_NOTHING,
      NULL);

  /* Make the window manager close button do same as Close button */
    XmAddWMProtocolCallback(errMsgS,WM_DELETE_WINDOW,
      errMsgDlgCb,(XtPointer)ERR_MSG_CLOSE_BTN);

  /* Create paned window */
    pane = XtVaCreateWidget("panel",
      xmPanedWindowWidgetClass, errMsgS,
      XmNsashWidth, 1,
      XmNsashHeight, 1,
      NULL);

  /* Create scrolled text */
    n = 0;
    XtSetArg(args[n], XmNrows,  10); n++;
    XtSetArg(args[n], XmNcolumns, 80); n++;
    XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
    XtSetArg(args[n], XmNeditable, False); n++;
    errMsgText = XmCreateScrolledText(pane,"text",args,n);
    XtManageChild(errMsgText);

  /* Create option area */
    optionArea = XtVaCreateWidget("optionArea",
      xmFormWidgetClass, pane,
      XmNshadowThickness, 0,
      XmNfractionBase, 11,
      XmNskipAdjust, True,
      NULL);

  /* Create Toggle button */
    label = XmStringCreateLocalized(
      "Raise Message Window When Important Message is Posted");
    raiseMessageWindowTB =  XtVaCreateManagedWidget("toggleButton",
      xmToggleButtonWidgetClass, optionArea,
      XmNlabelString, label,
      XmNset, (Boolean)medmRaiseMessageWindow,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     0,
      NULL);
    XmStringFree(label);
    XtAddCallback(raiseMessageWindowTB,XmNvalueChangedCallback,errMsgDlgCb,
      (XtPointer)ERR_MSG_RAISE_BTN);

  /* Create action area */
    actionArea = XtVaCreateWidget("actionArea",
      xmFormWidgetClass, pane,
      XmNshadowThickness, 0,
      XmNfractionBase, 67,
      XmNskipAdjust, True,
      NULL);

    closeButton = XtVaCreateManagedWidget("Close",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     1,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    11,
/*
  XmNshowAsDefault,    True,
  XmNdefaultButtonShadowThickness, 1,
  */
      NULL);
    XtAddCallback(closeButton,XmNactivateCallback,errMsgDlgCb,
      (XtPointer)ERR_MSG_CLOSE_BTN);

    clearButton = XtVaCreateManagedWidget("Clear",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_OPPOSITE_WIDGET,
      XmNtopWidget,        closeButton,
      XmNbottomAttachment, XmATTACH_OPPOSITE_WIDGET,
      XmNbottomWidget,     closeButton,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     12,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    22,
      NULL);
    XtAddCallback(clearButton,XmNactivateCallback,errMsgDlgCb,
      (XtPointer)ERR_MSG_CLEAR_BTN);

    printButton = XtVaCreateManagedWidget("Main Window",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     23,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    33,
      NULL);
    XtAddCallback(printButton,XmNactivateCallback,errMsgDlgCb,
      (XtPointer)ERR_MSG_MAINWINDOW_BTN);

    printButton = XtVaCreateManagedWidget("Print",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     34,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    44,
      NULL);
    XtAddCallback(printButton,XmNactivateCallback,errMsgDlgCb,
      (XtPointer)ERR_MSG_PRINT_BTN);

    sendButton = XtVaCreateManagedWidget("Mail",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     45,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    55,
      NULL);
#ifdef WIN32
    XtSetSensitive(sendButton,False);
#else
    XtAddCallback(sendButton,XmNactivateCallback,errMsgDlgCb,
      (XtPointer)ERR_MSG_SEND_BTN);
#endif

    helpButton = XtVaCreateManagedWidget("Help",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     56,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    66,
      NULL);
    XtAddCallback(helpButton,XmNactivateCallback,errMsgDlgCb,
      (XtPointer)ERR_MSG_HELP_BTN);

  /* Manage */
    XtManageChild(optionArea);
    XtManageChild(actionArea);
    XtManageChild(pane);
    if(raise) XtManageChild(errMsgS);

  /* Initialize */
    if(errMsgS) {
	time_t now;
	struct tm *tblock;
	char timeStampStr[TIME_STRING_MAX];
	XmTextPosition curpos = 0;

	time(&now);
	tblock = localtime(&now);
	strftime(timeStampStr,TIME_STRING_MAX,
	  "Message Log started at "STRFTIME_FORMAT"\n",tblock);
	timeStampStr[TIME_STRING_MAX-1]='0';
	XmTextInsert(errMsgText, curpos, timeStampStr);
	curpos+=strlen(timeStampStr);
	sprintf(timeStampStr,"MEDM PID: %d Window ID: 0x%x",
	  getpid(),(unsigned int)XtWindow(mainShell));
	timeStampStr[TIME_STRING_MAX-1]='0';
	XmTextInsert(errMsgText, curpos, timeStampStr);
	curpos+=strlen(timeStampStr);
	XmTextSetInsertionPosition(errMsgText, curpos);

      /* Copy early messages */
	if(earlyMessages) copyEarlyMessages();
    }
}

void errMsgSendDlgSendButtonCb(Widget w, XtPointer clientData, XtPointer callData)
{
    char *text, *subject, *to, cmd[1024], *p;
    FILE *pp;
#ifndef WIN32
    int status;
#endif

    UNREFERENCED(clientData);
    UNREFERENCED(callData);

    if(errMsgSendS == NULL) return;
    subject = XmTextFieldGetString(errMsgSendSubjectText);
    to = XmTextFieldGetString(errMsgSendToText);
    text = XmTextGetString(errMsgSendText);
    if(!(p = getenv("MEDM_MAIL_CMD")))
      p = "mail";
    p = strcpy(cmd,p);
    p += strlen(cmd);
    *p++ = ' ';
#if 0
    if(subject && *subject) {
      /* KE: Doesn't work with Solaris */
	sprintf(p, "-s \"%s\" ", subject);
	p += strlen(p);
    }
#endif
    if(to && *to) {
	sprintf(p, "%s", to);
    } else {
	medmPostMsg(1,"errMsgSendDlgSendButtonCb: "
	  "No recipient specified for mail\n");
	if(to) XtFree(to);
	if(subject) XtFree(subject);
	if(text) XtFree(text);
	return;
    }

#ifdef WIN32
  /* Mail not implemented on WIN32 */
    pp = NULL;
#else
    pp = popen(cmd, "w");
#endif
    if(!pp) {
	medmPostMsg(1,"errMsgSendDlgSendButtonCb: "
	  "Cannot execute mail command\n");
	if(to) XtFree(to);
	if(subject) XtFree(subject);
	if(text) XtFree(text);
	return;
    }
#if 1
    if(subject && *subject) {
	fputs("Subject: ", pp);
	fputs(subject, pp);
	fputs("\n\n", pp);
    }
#endif
    fputs(text, pp);
    fputc('\n', pp);      /* make sure there's a terminating newline */
#ifndef WIN32
  /* KE: Shouldn't be necessary to comment this out for WIN32 */
    status = pclose(pp);  /* close mail program */
#endif
    if(to) XtFree(to);
    if(subject) XtFree(subject);
    if(text) XtFree(text);
    XBell(display,50);
    XtUnmanageChild(errMsgSendS);
    return;
}

void errMsgSendDlgCloseButtonCb(Widget w, XtPointer clientData, XtPointer callData)
{
    UNREFERENCED(clientData);
    UNREFERENCED(callData);

    if(errMsgSendS != NULL)
      XtUnmanageChild(errMsgSendS);
}

void errMsgSendDlgCreateDlg()
{
    Widget pane;
    Widget rowCol;
    Widget toForm;
    Widget toLabel;
    Widget subjectForm;
    Widget subjectLabel;
    Widget actionArea;
    Widget closeButton;
    Widget sendButton;
    Arg    args[10];
    int n;

    if(errMsgS == NULL) return;
    if(errMsgSendS == NULL) {
	errMsgSendS = XtVaCreatePopupShell("errorMsgSendS",
#if 0
	/* KE: Gets iconized this way */
	  xmDialogShellWidgetClass, mainShell,
#endif
	  topLevelShellWidgetClass, mainShell,
	  XmNtitle, "MEDM Mail Message Window",
	  XmNdeleteResponse, XmDO_NOTHING,
	  NULL);
	pane = XtVaCreateWidget("panel",
	  xmPanedWindowWidgetClass, errMsgSendS,
	  XmNsashWidth, 1,
	  XmNsashHeight, 1,
	  NULL);
	rowCol = XtVaCreateWidget("rowCol",
	  xmRowColumnWidgetClass, pane,
	  NULL);
	toForm = XtVaCreateWidget("form",
	  xmFormWidgetClass, rowCol,
	  XmNshadowThickness, 0,
	  NULL);
	toLabel = XtVaCreateManagedWidget("To:",
	  xmLabelGadgetClass, toForm,
	  XmNleftAttachment,  XmATTACH_FORM,
	  XmNtopAttachment,   XmATTACH_FORM,
	  XmNbottomAttachment,XmATTACH_FORM,
	  NULL);
	errMsgSendToText = XtVaCreateManagedWidget("text",
	  xmTextFieldWidgetClass, toForm,
	  XmNleftAttachment,  XmATTACH_WIDGET,
	  XmNleftWidget,      toLabel,
	  XmNrightAttachment, XmATTACH_FORM,
	  XmNtopAttachment,   XmATTACH_FORM,
	  XmNbottomAttachment,XmATTACH_FORM,
	  NULL);
	XtManageChild(toForm);
	subjectForm = XtVaCreateManagedWidget("form",
	  xmFormWidgetClass, rowCol,
	  XmNshadowThickness, 0,
	  NULL);
	subjectLabel = XtVaCreateManagedWidget("Subject:",
	  xmLabelGadgetClass, subjectForm,
	  XmNleftAttachment,  XmATTACH_FORM,
	  XmNtopAttachment,   XmATTACH_FORM,
	  XmNbottomAttachment,XmATTACH_FORM,
	  NULL);
	errMsgSendSubjectText = XtVaCreateManagedWidget("text",
	  xmTextFieldWidgetClass, subjectForm,
	  XmNleftAttachment,  XmATTACH_WIDGET,
	  XmNleftWidget,      subjectLabel,
	  XmNrightAttachment, XmATTACH_FORM,
	  XmNtopAttachment,   XmATTACH_FORM,
	  XmNbottomAttachment,XmATTACH_FORM,
	  NULL);
	XtManageChild(subjectForm);
	n = 0;
	XtSetArg(args[n], XmNrows,  10); n++;
	XtSetArg(args[n], XmNcolumns, 80); n++;
	XtSetArg(args[n], XmNeditMode, XmMULTI_LINE_EDIT); n++;
	XtSetArg(args[n], XmNeditable, True); n++;
	errMsgSendText = XmCreateScrolledText(rowCol,"text",args,n);
	XtManageChild(errMsgSendText);
	actionArea = XtVaCreateWidget("actionArea",
	  xmFormWidgetClass, pane,
	  XmNfractionBase, 5,
	  XmNskipAdjust, True,
	  XmNshadowThickness, 0,
	  NULL);
	closeButton = XtVaCreateManagedWidget("Close",
	  xmPushButtonWidgetClass, actionArea,
	  XmNtopAttachment,    XmATTACH_FORM,
	  XmNbottomAttachment, XmATTACH_FORM,
	  XmNleftAttachment,   XmATTACH_POSITION,
	  XmNleftPosition,     1,
	  XmNrightAttachment,  XmATTACH_POSITION,
	  XmNrightPosition,    2,
/*
  XmNshowAsDefault,    True,
  XmNdefaultButtonShadowThickness, 1,
  */
	  NULL);
	XtAddCallback(closeButton, XmNactivateCallback,
	  errMsgSendDlgCloseButtonCb, NULL);
	sendButton = XtVaCreateManagedWidget("Send",
	  xmPushButtonWidgetClass, actionArea,
	  XmNtopAttachment,    XmATTACH_FORM,
	  XmNbottomAttachment, XmATTACH_FORM,
	  XmNleftAttachment,   XmATTACH_POSITION,
	  XmNleftPosition,     3,
	  XmNrightAttachment,  XmATTACH_POSITION,
	  XmNrightPosition,    4,
	  NULL);
#ifdef WIN32
	XtSetSensitive(sendButton,False);
#else
	XtAddCallback(sendButton, XmNactivateCallback,
	  errMsgSendDlgSendButtonCb, NULL);
#endif
    }
    XtManageChild(actionArea);
    XtManageChild(rowCol);
    XtManageChild(pane);
}

static char medmPrintfStr[2048]; /* DANGER: Fixed buffer size */

/* Priority = 1 means raise the message window */
void medmPostMsg(int priority, char *format, ...) {
    va_list args;
    time_t now;
    struct tm *tblock;
    char timeStampStr[TIME_STRING_MAX];
    XmTextPosition curpos;

  /* Create (or manage) the error dialog */
    errMsgDlgCreateDlg(medmRaiseMessageWindow && priority);

  /* Do timestamp */
    time(&now);
    tblock = localtime(&now);
    strftime(timeStampStr,TIME_STRING_MAX,"\n"STRFTIME_FORMAT"\n",tblock);
    timeStampStr[TIME_STRING_MAX-1]='0';
    if(errMsgText) {
	curpos = XmTextGetLastPosition(errMsgText);
	XmTextInsert(errMsgText, curpos, timeStampStr);
	curpos+=strlen(timeStampStr);
    } else {
	saveEarlyMessage(timeStampStr);
    }

  /* Also print to stderr */
#ifdef WIN32
    lprintf(timeStampStr);
#else
    fprintf(stderr, timeStampStr);
#endif

  /* Start variable arguments */
    va_start(args,format);
    vsprintf(medmPrintfStr, format, args);
    if(errMsgText) {
	XmTextInsert(errMsgText, curpos, medmPrintfStr);
	curpos+=strlen(medmPrintfStr);
	XmTextSetInsertionPosition(errMsgText, curpos);
	XmTextShowPosition(errMsgText, curpos);
	XmUpdateDisplay(mainShell);
	XFlush(display);
    } else {
	saveEarlyMessage(medmPrintfStr);
    }

  /* Also print to stderr. Use %s to preserve ", %, etc. */
#ifdef WIN32
    lprintf("%s",medmPrintfStr);
#else
    fprintf(stderr,"%s",medmPrintfStr);
#endif
    va_end(args);
}

void medmPrintf(int priority, char *format, ...)
{
    va_list args;
    XmTextPosition curpos;

  /* Create (or manage) the error dialog */
    errMsgDlgCreateDlg(medmRaiseMessageWindow && priority);

    va_start(args,format);
    vsprintf(medmPrintfStr, format, args);
    if(errMsgText) {
	curpos = XmTextGetLastPosition(errMsgText);
	XmTextInsert(errMsgText, curpos, medmPrintfStr);
	curpos+=strlen(medmPrintfStr);
	XmTextSetInsertionPosition(errMsgText, curpos);
	XmTextShowPosition(errMsgText, curpos);
	XmUpdateDisplay(mainShell);
	XFlush(display);
    } else {
	saveEarlyMessage(medmPrintfStr);
    }

/* Also print to stderr */
#ifdef WIN32
    lprintf(medmPrintfStr);
#else
    fprintf(stderr, medmPrintfStr);
#endif

    va_end(args);
}

static int saveEarlyMessage(char *msg)
{
    int len, newLen;
    int full = 0;

  /* Don't do it again */
    if(earlyMessagesDone) return 0;

  /* Allocate space */
    if(!earlyMessages) {
	earlyMessages = (char *)calloc(EARLY_MESSAGE_SIZE, sizeof(char));
	if(!earlyMessages) return 0;
    }

  /* Concatenate the message */
    if(!full) {
	len = strlen(earlyMessages);
	newLen = strlen(msg);
	if(len + newLen < EARLY_MESSAGE_SIZE - 14) {
	    strcat(earlyMessages, msg);
	    return 1;
	} else {
	    strcat(earlyMessages, "\n[BufferFull]");
	    full = 1;
	    return 0;
	}
    } else {
	return 0;
    }
}

static void copyEarlyMessages(void)
{
    XmTextPosition curpos;
    char string[80];

    earlyMessagesDone = 1;
    if(!earlyMessages || !*earlyMessages) return;

  /* Insert the messages */
    curpos = XmTextGetLastPosition(errMsgText);
    strcpy(string,"\n*** Messages received before Message Window was "
      "started:\n");
    XmTextInsert(errMsgText, curpos, string);
    curpos+=strlen(string);
    XmTextInsert(errMsgText, curpos, earlyMessages);
    curpos+=strlen(earlyMessages);
    strcpy(string,"\n*** End of early messages\n");
    XmTextInsert(errMsgText, curpos, string);
    curpos+=strlen(string);

  /* Free the space */
    free(earlyMessages);
}

int checkEarlyMessages(void)
{
    if(earlyMessages && !earlyMessagesDone) {
	medmPrintf(1, "");
	return 1;
    } else {
	return 0;
    }
}

static char caStudyMsg[512];
static char *caStatusDummyString =
"Time Interval (sec)       =         \n"
"CA Channels               =         \n"
"CA Channels Connected     =         \n"
"CA Incoming Events        =         \n"
"MEDM Objects Updating     =         \n"
"MEDM Objects Updated      =         \n"
"Update Requests           =         \n"
"Update Requests Discarded =         \n"
"Update Requests Queued    =         \n";


static double totalTimeElapsed = 0.0;
static double aveCAEventCount = 0.0;
static double aveUpdateExecuted = 0;
static double aveUpdateRequested = 0;
static double aveUpdateRequestDiscarded = 0;
static Boolean caStudyAverageMode = False;

void caStudyDlgCloseButtonCb(Widget w, XtPointer clientData, XtPointer callData)
{
    UNREFERENCED(clientData);
    UNREFERENCED(callData);

    if(caStudyS != NULL) {
	medmStopUpdateCAStudyDlg();
	XtUnmanageChild(caStudyS);
    }
    return;
}

void medmResetUpdateCAStudyDlg(Widget w, XtPointer clientData,
  XtPointer callData)
{
    UNREFERENCED(w);
    UNREFERENCED(clientData);
    UNREFERENCED(callData);

    totalTimeElapsed = 0.0;
    aveCAEventCount = 0.0;
    aveUpdateExecuted = 0.0;
    aveUpdateRequested = 0.0;
    aveUpdateRequestDiscarded = 0.0;

  /* Reset the statistics */
    updateTaskStatusGetInfo(NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL);

  /* Restart the timeout */
    medmStartUpdateCAStudyDlg();
}

void caStudyDlgModeButtonCb(Widget w, XtPointer clientData, XtPointer callData)
{
    UNREFERENCED(clientData);
    UNREFERENCED(callData);

  /* Toggle the mode */
    caStudyAverageMode = !(caStudyAverageMode);

  /* Redraw the dialog */
    medmUpdateCAStudyDlg(CASTUDY_REDRAW_ONLY,&caStudyDlgTimeOutId);
}

void medmCreateCAStudyDlg() {
    Widget pane;
    Widget actionArea;
    Widget closeButton;
    Widget resetButton;
    Widget modeButton;
    XmString str;

    if(!caStudyS) {
	if(mainShell == NULL) return;

	caStudyS = XtVaCreatePopupShell("status",
	  xmDialogShellWidgetClass, mainShell,
	  XmNtitle, "MEDM Statistics Window",
	  XmNdeleteResponse, XmDO_NOTHING,
	  NULL);

      /* Make the window manager close button do same as Close button */
	XmAddWMProtocolCallback(caStudyS,WM_DELETE_WINDOW,
	  caStudyDlgCloseButtonCb, NULL);

	pane = XtVaCreateWidget("panel",
	  xmPanedWindowWidgetClass, caStudyS,
	  XmNsashWidth, 1,
	  XmNsashHeight, 1,
	  NULL);

	str = XmStringLtoRCreate(caStatusDummyString,XmFONTLIST_DEFAULT_TAG);

	caStudyLabel = XtVaCreateManagedWidget("status",
	  xmLabelWidgetClass, pane,
	  XmNalignment, XmALIGNMENT_BEGINNING,
	  XmNlabelString,str,
	  NULL);
	XmStringFree(str);


	actionArea = XtVaCreateWidget("actionArea",
	  xmFormWidgetClass, pane,
	  XmNshadowThickness, 0,
	  XmNfractionBase, 7,
	  XmNskipAdjust, True,
	  NULL);
	closeButton = XtVaCreateManagedWidget("Close",
	  xmPushButtonWidgetClass, actionArea,
	  XmNtopAttachment,    XmATTACH_FORM,
	  XmNbottomAttachment, XmATTACH_FORM,
	  XmNleftAttachment,   XmATTACH_POSITION,
	  XmNleftPosition,     1,
	  XmNrightAttachment,  XmATTACH_POSITION,
	  XmNrightPosition,    2,
	  NULL);
	resetButton = XtVaCreateManagedWidget("Reset",
	  xmPushButtonWidgetClass, actionArea,
	  XmNtopAttachment,    XmATTACH_FORM,
	  XmNbottomAttachment, XmATTACH_FORM,
	  XmNleftAttachment,   XmATTACH_POSITION,
	  XmNleftPosition,     3,
	  XmNrightAttachment,  XmATTACH_POSITION,
	  XmNrightPosition,    4,
	  NULL);
	modeButton = XtVaCreateManagedWidget("Mode",
	  xmPushButtonWidgetClass, actionArea,
	  XmNtopAttachment,    XmATTACH_FORM,
	  XmNbottomAttachment, XmATTACH_FORM,
	  XmNleftAttachment,   XmATTACH_POSITION,
	  XmNleftPosition,     5,
	  XmNrightAttachment,  XmATTACH_POSITION,
	  XmNrightPosition,    6,
	  NULL);
	XtAddCallback(closeButton, XmNactivateCallback,
	  caStudyDlgCloseButtonCb, NULL);
	XtAddCallback(resetButton,XmNactivateCallback,
	  medmResetUpdateCAStudyDlg, NULL);
	XtAddCallback(modeButton,XmNactivateCallback,
	  caStudyDlgModeButtonCb, NULL);
	XtManageChild(actionArea);
	XtManageChild(pane);
    }
    XtManageChild(caStudyS);

  /* Start it if in EXECUTE mode */
    if(globalDisplayListTraversalMode == DL_EXECUTE) {
	medmStartUpdateCAStudyDlg();
    }
}

static void medmUpdateCAStudyDlg(XtPointer clientData, XtIntervalId *id)
{
    XmString str;
    int taskCount;
    int periodicTaskCount;
    int updateRequestCount;
    int updateDiscardCount;
    int periodicUpdateRequestCount;
    int periodicUpdateDiscardCount;
    int updateRequestQueued;
    int updateExecuted;
    int totalUpdateRequested;
    int totalUpdateDiscarded;
    double timeInterval;
    int channelCount;
    int channelConnected;
    int caEventCount;

    UNREFERENCED(id);

#if DEBUG_STATISTICS
    print("medmUpdateCAStudyDlg[1]: id=%x elapsed=%f\n",caStudyDlgTimeOutId,
      totalTimeElapsed);
#endif

  /* Check if we should do something */
  /* clientData is NULL except when starting up */
    if(!caStudyS ||
      (clientData != CASTUDY_INITIAL_UPDATE && !caStudyDlgTimeOutId) ||
      globalDisplayListTraversalMode == DL_EDIT) {
	caStudyDlgTimeOutId=0;
	return;
    }

  /* Update the dialog */
    updateTaskStatusGetInfo(&taskCount,
      &periodicTaskCount,
      &updateRequestCount,
      &updateDiscardCount,
      &periodicUpdateRequestCount,
      &periodicUpdateDiscardCount,
      &updateRequestQueued,
      &updateExecuted,
      &timeInterval);
#ifdef MEDM_CDEV
  /* No channels connected or CA events for CDEV */
    channelConnected = caEventCount = 0;
#else
    caTaskGetInfo(&channelCount, &channelConnected, &caEventCount);
#endif
    totalUpdateDiscarded = updateDiscardCount + periodicUpdateDiscardCount;
    totalUpdateRequested = updateRequestCount + periodicUpdateRequestCount +
      totalUpdateDiscarded;
    if(caStudyAverageMode) {
	double elapsedTime = totalTimeElapsed;
	totalTimeElapsed += timeInterval;
	if(totalTimeElapsed > MIN_SIGNIFICANT_TIME_INTERVAL) {
	    aveCAEventCount = (aveCAEventCount * elapsedTime + caEventCount) /
	      totalTimeElapsed;
	    aveUpdateExecuted =
	      (aveUpdateExecuted * elapsedTime + updateExecuted) /
	      totalTimeElapsed;
	    aveUpdateRequested =
	      (aveUpdateRequested * elapsedTime + totalUpdateRequested) /
	      totalTimeElapsed;
	    aveUpdateRequestDiscarded =
	      (aveUpdateRequestDiscarded * elapsedTime + totalUpdateDiscarded) /
	      totalTimeElapsed;
	} else {
	    aveCAEventCount =  aveUpdateExecuted = aveUpdateRequested =
	      aveUpdateRequestDiscarded = 0.0;
	}
	sprintf(caStudyMsg,
	  "AVERAGES\n\n"
	  "CA Incoming Events        = %8.1f\n"
	  "MEDM Objects Updated      = %8.1f\n"
	  "Update Requests           = %8.1f\n"
	  "Update Requests Discarded = %8.1f\n\n"
	  "Total Time Elapsed        = %8.1f\n",
	  aveCAEventCount,
	  aveUpdateExecuted,
	  aveUpdateRequested,
	  aveUpdateRequestDiscarded,
	  totalTimeElapsed);
    } else {
	totalTimeElapsed += timeInterval;
	sprintf(caStudyMsg,
	  "Time Interval (sec)       = %8.2f\n"
	  "CA Channels               = %8d\n"
	  "CA Channels Connected     = %8d\n"
	  "CA Incoming Events        = %8d\n"
	  "MEDM Objects Updating     = %8d\n"
	  "MEDM Objects Updated      = %8d\n"
	  "Update Requests           = %8d\n"
	  "Update Requests Discarded = %8d\n"
	  "Update Requests Queued    = %8d\n",
	  timeInterval,
	  channelCount,
	  channelConnected,
	  caEventCount,
	  taskCount,
	  updateExecuted,
	  totalUpdateRequested,
	  totalUpdateDiscarded,
	  updateRequestQueued);
    }
    str = XmStringLtoRCreate(caStudyMsg,XmFONTLIST_DEFAULT_TAG);
    XtVaSetValues(caStudyLabel,XmNlabelString,str,NULL);
    XmStringFree(str);
    XFlush(XtDisplay(caStudyS));
    XmUpdateDisplay(caStudyS);

  /* Call it again */
    if(clientData != CASTUDY_REDRAW_ONLY) {
	caStudyDlgTimeOutId = XtAppAddTimeOut(appContext, CASTUDYINTERVAL,
	  medmUpdateCAStudyDlg, NULL);
    }
#if DEBUG_STATISTICS
    print("medmUpdateCAStudyDlg[2]: id=%x elapsed=%f\n",caStudyDlgTimeOutId,
      totalTimeElapsed);
#endif
}

void medmStartUpdateCAStudyDlg()
{
#if DEBUG_STATISTICS
    print("medmStartUpdateCAStudyDlg[1]: id=%x\n",caStudyDlgTimeOutId);
#endif
    if(caStudyDlgTimeOutId) medmStopUpdateCAStudyDlg();
#if 0
  /* Start the timer.  First update will be at the end of the
     interval. */
    caStudyDlgTimeOutId = XtAppAddTimeOut(appContext, CASTUDYINTERVAL,
      medmUpdateCAStudyDlg, NULL);
#else
  /* Call the update procedure with a client data of
     CASTUDY_INITIAL_UPDATE to indicate we are starting up.  It will
     update and then set caStudyDlgTimeOutId. */
#if DEBUG_STATISTICS
    print("medmStartUpdateCAStudyDlg[2]: id=%x\n",caStudyDlgTimeOutId);
#endif
    medmUpdateCAStudyDlg(CASTUDY_INITIAL_UPDATE,&caStudyDlgTimeOutId);
#if DEBUG_STATISTICS
    print("medmStartUpdateCAStudyDlg[3]: id=%x\n",caStudyDlgTimeOutId);
#endif
#endif
}

void medmStopUpdateCAStudyDlg()
{
#if DEBUG_STATISTICS
    print("medmStopUpdateCAStudyDlg[1]: id=%x\n",caStudyDlgTimeOutId);
#endif
    if(caStudyDlgTimeOutId) {
	XtRemoveTimeOut(caStudyDlgTimeOutId);
	caStudyDlgTimeOutId = 0;
    }
#if DEBUG_STATISTICS
    print("medmStopUpdateCAStudyDlg[2]: id=%x\n",caStudyDlgTimeOutId);
#endif
}

int xDoNothingErrorHandler(Display *dpy, XErrorEvent *event)
{
  /* Return value is ignored */
    return 0;
}

int xErrorHandler(Display *dpy, XErrorEvent *event)
{
    char buf[1024];     /* Warning: Fixed Size */
    static int nerrors=0;
    static int ended=0;

  /* Prevent error storms and recursive errors */
    if(ended) return 0;
    if(nerrors++ > MAX_ERRORS) {
	ended=1;
	medmPostMsg(1,"xErrorHandler:"
	  "Too many X errors [%d]\n"
	  "No more will be handled\n"
	  "Please fix the problem and restart MEDM\n",
	  MAX_ERRORS);
	return 0;
    }

    if(event) {
	XGetErrorText(dpy,event->error_code,buf,1024);
    } else {
	sprintf(buf,"[Event associated with error is NULL]");
    }
#if POST_X_ERRORS_TO_MESSAGE_WINDOW
  /* Post to the message window (and also print to stderr there) */
    medmPostMsg(1,"xErrorHandler:\n%s\n", buf);
#else
  /* Just print to stderr */
# ifdef WIN32
    lprintf("\n%s\n", buf);
# else
    fprintf(stderr,"\n%s\n", buf);
# endif
#endif

  /* Return value is ignored */
    return 0;
}

void xtErrorHandler(char *message)
{
    static int nerrors=0;
    static int ended=0;

#if DEBUG_ERRORHANDLER
    printf("xtErrorHandler: |%s|\n",message);
#endif

  /* Prevent error storms and recursive errors */
    if(ended) return;
    if(nerrors++ > MAX_ERRORS) {
	ended=1;
	medmPostMsg(1,"xtErrorHandler:"
	  "Too many Xt errors [%d]\n"
	  "No more will be handled\n"
	  "Please fix the problem and restart MEDM\n",
	  MAX_ERRORS);
	return;
    }
  /* Post the message */
    medmPostMsg(1,"xtErrorHandler:\n%s\n", message);
}

int xInfoMsg(Widget parent, const char *fmt, ...)
{
    Widget warningBox,child;
    va_list vargs;
    static char lstring[1024];     /* Warning: Fixed Size */
    XmString xmString;
    Arg args[2];
    int nargs;

    va_start(vargs,fmt);
    (void)vsprintf(lstring,fmt,vargs);
    va_end(vargs);

    if(lstring[0] != '\0') {
	xmString=XmStringCreateLtoR(lstring,XmSTRING_DEFAULT_CHARSET);
	nargs=0;
	XtSetArg(args[nargs],XmNtitle,"Information"); nargs++;
	XtSetArg(args[nargs],XmNmessageString,xmString); nargs++;
	warningBox=XmCreateWarningDialog(parent,"infoMessage",
	  args,nargs);
	XmStringFree(xmString);
	child=XmMessageBoxGetChild(warningBox,XmDIALOG_CANCEL_BUTTON);
	XtDestroyWidget(child);
	child=XmMessageBoxGetChild(warningBox,XmDIALOG_HELP_BUTTON);
	XtDestroyWidget(child);
	XtManageChild(warningBox);
	XmUpdateDisplay(warningBox);
    }
#ifdef WIN32
    lprintf("%s\n",lstring);
#else
    fprintf(stderr,"%s\n",lstring);
#endif

    return 0;
}

/*****************************************************************************
 *
 * Display Help
 *
 * Original Author: Vladimir T. Romanovski  (romsky@x4u2.desy.de)
 * Organization: KRYK/@DESY 1996
 *
 *****************************************************************************
*/

/*
 * This routine reads the MEDM_HELP environment variable and makes a system
 *   call to provide help for the display
 */
static void displayHelpCallback(Widget shell, XtPointer client_data,
  XtPointer call_data)
{
    DisplayInfo *displayInfo = (DisplayInfo *)client_data;
    char *env = getenv("MEDM_HELP");
    char *name = displayInfo->dlFile->name;

    if(env != NULL) {
      /* Run the help command */
	char *command;

	command = (char*)malloc(strlen(env) + strlen(name) + 5);
	sprintf(command, "%s %s &", env, name);
	(void)system(command);
	free(command);
    } else {
      /* KE: Should no longer get here */
      /* Print error message */
	medmPostMsg(1,"displayHelpCallback: "
	  "The environment variable MEDM_HELP is not set\n"
	  "  Cannot implement help for %s\n", name);
    }
}

/*
 * This routine installs a customized Motif window manager protocol for
 *   the shell widget of the display
 */
void addDisplayHelpProtocol(DisplayInfo *displayInfo)
{
    Atom message, protocol;
    char buf[80];

    message = XmInternAtom (XtDisplay(displayInfo->shell),
      "_MOTIF_WM_MESSAGES", FALSE);
    protocol = XmInternAtom (XtDisplay(displayInfo->shell),
      "_MEDM_DISPLAY_HELP", FALSE);

    XmAddProtocols(displayInfo->shell, message, &protocol, 1);
    XmAddProtocolCallback(displayInfo->shell, message, protocol,
      displayHelpCallback, (XtPointer)displayInfo);

    sprintf (buf, "Help _H Ctrl<Key>h f.send_msg %ld", protocol);
    XtVaSetValues (displayInfo->shell, XmNmwmMenu, buf, NULL);
}
