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

#define TIME_STRING_MAX 81
#define EARLY_MESSAGE_SIZE 2048

#define ERR_MSG_RAISE_BTN 0
#define ERR_MSG_CLOSE_BTN 1
#define ERR_MSG_CLEAR_BTN 2
#define ERR_MSG_PRINT_BTN 3
#define ERR_MSG_SEND_BTN 4
#define ERR_MSG_HELP_BTN 5


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
#include <unistd.h>
#endif

/* Function prototypes */

static void displayHelpCallback(Widget shell, XtPointer client_data,
  XtPointer call_data);
static int saveEarlyMessage(char *msg);
static void copyEarlyMessages(void);

/* Global variables */

static char *earlyMessages = NULL;
static int earlyMessagesDone = 0;

extern FILE *popen(const char *, const char *);     /* May not be defined for strict ANSI */
extern int	pclose(FILE *);     /* May not be defined for strict ANSI */

static Widget errMsgText = NULL;
static Widget errMsgSendSubjectText = NULL;
static Widget errMsgSendToText = NULL;
static Widget errMsgSendText = NULL;
static XtIntervalId errMsgDlgTimeOutId = 0;

void errMsgSendDlgCreateDlg();
void errMsgSendDlgSendButtonCb(Widget,XtPointer,XtPointer);
void errMsgSendDlgCloseButtonCb(Widget,XtPointer,XtPointer);
void errMsgDlgCb(Widget,XtPointer,XtPointer);
static void medmUpdateCAStudyDlg(XtPointer data, XtIntervalId *id);

/* Global variables */

static int raiseMessageWindow = 1;
static Widget raiseMessageWindowTB;

#ifdef __cplusplus
void globalHelpCallback(Widget, XtPointer cd, XtPointer)
#else
void globalHelpCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int helpIndex = (int) cd;

    switch (helpIndex) {
    case HELP_MAIN:
	callBrowser(MEDM_HELP_PATH"/MEDM.html#InitialLocations");
	break;
    }
}

#ifdef __cplusplus
void errMsgDlgCb(Widget, XtPointer clientData, XtPointer)
#else
void errMsgDlgCb(Widget w, XtPointer clientData, XtPointer callData)
#endif
{
    int button = (int)clientData;

    switch(button) {
    case ERR_MSG_RAISE_BTN:
	raiseMessageWindow = XmToggleButtonGetState(raiseMessageWindowTB);
	break;
    case ERR_MSG_CLOSE_BTN:
	if (errMsgS != NULL) {
	    XtUnmanageChild(errMsgS);
	}
	break;
    case ERR_MSG_CLEAR_BTN:
	{
	    long now; 
	    struct tm *tblock;
	    char timeStampStr[TIME_STRING_MAX];
	    XmTextPosition curpos = 0;
    
	    if (errMsgText == NULL) return;
	    
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
	    XmTextSetInsertionPosition(errMsgText, curpos);
	}
	break;
    case ERR_MSG_PRINT_BTN:
	{
	    long now; 
	    struct tm *tblock;
	    FILE *file;
	    char timeStampStr[TIME_STRING_MAX];
	    char *tmp, *psFileName, *commandBuffer;
	    
	    if (errMsgText == NULL) return;
	    if (getenv("PSPRINTER") == (char *)NULL) {
		medmPrintf(1,
		  "\nerrMsgDlgPrintButtonCb: PSPRINTER environment variable not set,"
		  " printing disallowed\n");
		return;
	    }
	    
	  /* Get selection and timestamp */
	    time(&now);
	    tblock = localtime(&now);
	    tmp = XmTextGetSelection(errMsgText);
	    strftime(timeStampStr,TIME_STRING_MAX,
	      "MEDM Message Window Selection at "STRFTIME_FORMAT":\n\n",tblock);
	    if (tmp == NULL) {
		tmp = XmTextGetString(errMsgText);
		strftime(timeStampStr,TIME_STRING_MAX,
		  "MEDM Message Window at "STRFTIME_FORMAT":\n\n",tblock);
	    }
	    timeStampStr[TIME_STRING_MAX-1]='0';
	    
	  /* Create filename */
	    psFileName = (char *)calloc(1,256);
	    sprintf(psFileName,"/tmp/medmMessageWindowPrintFile%d",getpid());
	    
	  /* Write file */
	    file = fopen(psFileName,"w+");
	    if (file == NULL) {
		medmPrintf(1,"\nerrMsgDlgPrintButtonCb:  Unable to open file: %s",
		  psFileName);
		XtFree(tmp);
		free(psFileName);
		return;
	    }
	    fprintf(file,timeStampStr);
	    fprintf(file,tmp);
	    XtFree(tmp);
	    fclose(file);
	    
	  /* Print file */
	    commandBuffer = (char *)calloc(1,256);
	    sprintf(commandBuffer,"lp -d$PSPRINTER %s",psFileName);
	    system(commandBuffer);
	    
	  /* Delete file */
	    sprintf(commandBuffer,"rm %s",psFileName);
	    system(commandBuffer);
	    
	  /* Clean up */
	    free(psFileName);
	    free(commandBuffer);
	    XBell(display,50);
	}
	break;
    case ERR_MSG_SEND_BTN:
	{
	    char *tmp;
	    
	    if (errMsgText == NULL) return;
	    if (errMsgSendS == NULL) {
		errMsgSendDlgCreateDlg();
	    }
	    XmTextSetString(errMsgSendToText,"");
	    XmTextSetString(errMsgSendSubjectText,"MEDM Message Window Contents");
	    tmp = XmTextGetSelection(errMsgText);
	    if (tmp == NULL) {
		tmp = XmTextGetString(errMsgText);
	    }
	    XmTextSetString(errMsgSendText,tmp);
	    XtFree(tmp);
	    XtManageChild(errMsgSendS);
	}
	break;
    case ERR_MSG_HELP_BTN:
	callBrowser(MEDM_HELP_PATH"/MEDM.html#MessageWindow");
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

    if (errMsgS != NULL) {
	if(raise) {
	    Window w=XtWindow(errMsgS);;
	
	    if(XtIsManaged(errMsgS)) XtUnmanageChild(errMsgS);
	    XtManageChild(errMsgS);
	  /* Raise the window if it exists (widget is realized) */
	    if(w) XRaiseWindow(display,w);
	}
	return;
    }

    if (mainShell == NULL) return;

    errMsgS = XtVaCreatePopupShell("errorMsgS",
#if 0
    /* KE: Gets iconized this way */
      xmDialogShellWidgetClass, mainShell,
#endif      
      topLevelShellWidgetClass, mainShell,
      XmNtitle, "MEDM Message Window",
      XmNdeleteResponse, XmDO_NOTHING,
      NULL);

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
      XmNset, (Boolean)raiseMessageWindow,
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
      XmNfractionBase, 11,
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
      XmNleftPosition,     3,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    4,
      NULL);
    XtAddCallback(clearButton,XmNactivateCallback,errMsgDlgCb,
      (XtPointer)ERR_MSG_CLEAR_BTN);

    printButton = XtVaCreateManagedWidget("Print",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     5,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    6,
      NULL);
#ifdef WIN32
    XtSetSensitive(printButton,False);
#else
    XtAddCallback(printButton,XmNactivateCallback,errMsgDlgCb,
      (XtPointer)ERR_MSG_PRINT_BTN);
#endif    

    sendButton = XtVaCreateManagedWidget("Mail",
      xmPushButtonWidgetClass, actionArea,
      XmNtopAttachment,    XmATTACH_FORM,
      XmNbottomAttachment, XmATTACH_FORM,
      XmNleftAttachment,   XmATTACH_POSITION,
      XmNleftPosition,     7,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    8,
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
      XmNleftPosition,     9,
      XmNrightAttachment,  XmATTACH_POSITION,
      XmNrightPosition,    10,
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
	long now; 
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
	XmTextSetInsertionPosition(errMsgText, curpos);

      /* Copy early messages */
	if(earlyMessages) copyEarlyMessages();
    }
}

#ifdef __cplusplus  
void errMsgSendDlgSendButtonCb(Widget, XtPointer, XtPointer)
#else
void errMsgSendDlgSendButtonCb(Widget w, XtPointer dummy1, XtPointer dummy2)
#endif
{
    char *text, *subject, *to, cmd[1024], *p;
    FILE *pp;
#ifndef WIN32    
    int status;
#endif    

    if (errMsgSendS == NULL) return;
    subject = XmTextFieldGetString(errMsgSendSubjectText);
    to = XmTextFieldGetString(errMsgSendToText);
    text = XmTextGetString(errMsgSendText);
    if (!(p = getenv("MEDM_MAIL_CMD")))
      p = "mail";
    p = strcpy(cmd,p);
    p += strlen(cmd);
    *p++ = ' ';
#if 0    
    if (subject && *subject) {
      /* KE: Doesn't work with Solaris */
	sprintf(p, "-s \"%s\" ", subject);
	p += strlen(p);
    }
#endif    
    if (to && *to) {
	sprintf(p, "%s", to);
    } else {
	medmPostMsg(1,"errMsgSendDlgSendButtonCb: No recipient specified for mail\n");
	if (to) XtFree(to);
	if (subject) XtFree(subject);
	if (text) XtFree(text);
	return;
    }

#ifdef WIN32
  /* Mail not implemented on WIN32 */
    pp = NULL;
#else
    pp = popen(cmd, "w");
#endif
    if (!pp) {
	medmPostMsg(1,"errMsgSendDlgSendButtonCb: Cannot execute mail command\n");
	if (to) XtFree(to);
	if (subject) XtFree(subject);
	if (text) XtFree(text);
	return;
    }
#if 1    
    if (subject && *subject) {
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
    if (to) XtFree(to);
    if (subject) XtFree(subject);
    if (text) XtFree(text);
    XBell(display,50);
    XtUnmanageChild(errMsgSendS);
    return;
}

#ifdef __cplusplus
void errMsgSendDlgCloseButtonCb(Widget, XtPointer, XtPointer)
#else
void errMsgSendDlgCloseButtonCb(Widget w, XtPointer dummy1, XtPointer dummy2)
#endif
{
    if (errMsgSendS != NULL)
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

    if (errMsgS == NULL) return;
    if (errMsgSendS == NULL) {
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
	XtAddCallback(closeButton,XmNactivateCallback,errMsgSendDlgCloseButtonCb, NULL);
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
	XtAddCallback(sendButton,XmNactivateCallback,errMsgSendDlgSendButtonCb, NULL);
#endif	
    }
    XtManageChild(actionArea);
    XtManageChild(rowCol);
    XtManageChild(pane);
}

static char medmPrintfStr[2048]; /* DANGER: Fixed buffer size */

void medmPostMsg(int priority, char *format, ...) {
    va_list args;
    long now; 
    struct tm *tblock;
    char timeStampStr[TIME_STRING_MAX];
    XmTextPosition curpos;

  /* Create (or manage) the error dialog */
    errMsgDlgCreateDlg(raiseMessageWindow && priority);

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

void medmPrintf(int priority, char *format, ...)
{
    va_list args;
    XmTextPosition curpos;

  /* Create (or manage) the error dialog */
    errMsgDlgCreateDlg(raiseMessageWindow && priority);

    va_start(args,format);
    vsprintf(medmPrintfStr, format, args);
    if(errMsgText) {
	curpos = XmTextGetLastPosition(errMsgText);
	XmTextInsert(errMsgText, curpos, medmPrintfStr);
	curpos+=strlen(medmPrintfStr);
	XmTextSetInsertionPosition(errMsgText, curpos);
	XmTextShowPosition(errMsgText, curpos);
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
    strcpy(string,"\n*** Messages received before Message Window was started:\n");
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
static Boolean caUpdateStudyDlg = False;
static char *caStatusDummyString =
"Time Interval (sec)       =         \n"
"CA connection(s)          =         \n"
"CA connected              =         \n"
"CA incoming event(s)      =         \n"
"Active Objects            =         \n"
"Object(s) Updated         =         \n"
"Update Requests           =         \n"
"Update Requests Discarded =         \n"
"Update Requests Queued    =         \n";


static double totalTimeElapsed = 0.0;
static double aveCAEventCount = 0.0;
static double aveUpdateExecuted = 0;
static double aveUpdateRequested = 0;
static double aveUpdateRequestDiscarded = 0;
static Boolean caStudyAverageMode = False;

#ifdef __cplusplus
void caStudyDlgCloseButtonCb(Widget, XtPointer, XtPointer)
#else
void caStudyDlgCloseButtonCb(Widget w, XtPointer dummy1, XtPointer dummy2)
#endif
{
    if (caStudyS != NULL) {
	XtUnmanageChild(caStudyS);
	caUpdateStudyDlg = False;
    }
    return;
}

#ifdef __cplusplus
void caStudyDlgResetButtonCb(Widget, XtPointer, XtPointer)
#else
void caStudyDlgResetButtonCb(Widget w, XtPointer dummy1, XtPointer dummy2)
#endif
{
    totalTimeElapsed = 0.0;
    aveCAEventCount = 0.0;
    aveUpdateExecuted = 0.0;
    aveUpdateRequested = 0.0;
    aveUpdateRequestDiscarded = 0.0;
    return;
}

#ifdef __cplusplus
void caStudyDlgModeButtonCb(Widget, XtPointer, XtPointer)
#else
void caStudyDlgModeButtonCb(Widget w, XtPointer dummy1, XtPointer dummy2)
#endif
{
    caStudyAverageMode = !(caStudyAverageMode);
    return;
}

void medmCreateCAStudyDlg() {
    Widget pane;
    Widget actionArea;
    Widget closeButton;
    Widget resetButton;
    Widget modeButton;
    XmString str;

    if (!caStudyS) {

	if (mainShell == NULL) return;

	caStudyS = XtVaCreatePopupShell("status",
	  xmDialogShellWidgetClass, mainShell,
	  XmNtitle, "MEDM Message Window",
	  XmNdeleteResponse, XmDO_NOTHING,
	  NULL);

	pane = XtVaCreateWidget("panel",
	  xmPanedWindowWidgetClass, caStudyS,
	  XmNsashWidth, 1,
	  XmNsashHeight, 1,
	  NULL);

	str = XmStringLtoRCreate(caStatusDummyString,XmSTRING_DEFAULT_CHARSET);

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
	XtAddCallback(closeButton,XmNactivateCallback,caStudyDlgCloseButtonCb, NULL);
	XtAddCallback(resetButton,XmNactivateCallback,caStudyDlgResetButtonCb, NULL);
	XtAddCallback(modeButton,XmNactivateCallback,caStudyDlgModeButtonCb, NULL);
	XtManageChild(actionArea);
	XtManageChild(pane);
    }

    XtManageChild(caStudyS);
    caUpdateStudyDlg = True;
    if (globalDisplayListTraversalMode == DL_EXECUTE) {
	if (errMsgDlgTimeOutId == 0)
	  errMsgDlgTimeOutId = XtAppAddTimeOut(appContext,1000,medmUpdateCAStudyDlg,NULL);
    } else {
	errMsgDlgTimeOutId = 0;
    }
}

#ifdef __cplusplus
static void medmUpdateCAStudyDlg(XtPointer, XtIntervalId *id)
#else
static void medmUpdateCAStudyDlg(XtPointer cd, XtIntervalId *id)
#endif
{
    if (caUpdateStudyDlg) {
	XmString str;
	int taskCount;
	int periodicTaskCount;
	int updateRequestCount;
	int updateDiscardCount;
	int periodicUpdateRequestCount;
	int periodicUpdateDiscardCount;
	int updateRequestQueued;
	int updateExecuted;
	int totalTaskCount;
	int totalUpdateRequested;
	int totalUpdateDiscarded;
	double timeInterval; 
	int channelCount;
	int channelConnected;
	int caEventCount;

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
	CATaskGetInfo(&channelCount,&channelConnected,&caEventCount);
#endif
	totalUpdateDiscarded = updateDiscardCount+periodicUpdateDiscardCount;
	totalUpdateRequested = updateRequestCount+periodicUpdateRequestCount + totalUpdateDiscarded;
	totalTaskCount = taskCount + periodicTaskCount;
	if (caStudyAverageMode) {
	    double elapseTime = totalTimeElapsed;
	    totalTimeElapsed += timeInterval;
	    aveCAEventCount = (aveCAEventCount * elapseTime + caEventCount) / totalTimeElapsed;
	    aveUpdateExecuted = (aveUpdateExecuted * elapseTime + updateExecuted) / totalTimeElapsed;
	    aveUpdateRequested = (aveUpdateRequested * elapseTime + totalUpdateRequested) / totalTimeElapsed;
	    aveUpdateRequestDiscarded =
	      (aveUpdateRequestDiscarded * elapseTime + totalUpdateDiscarded) / totalTimeElapsed;
	    sprintf(caStudyMsg,
	      "AVERAGE :\n"
	      "Total Time Elapsed        = %8.1f\n"
	      "CA Incoming Event(s)      = %8.1f\n"
	      "Object(s) Updated         = %8.1f\n"
	      "Update Requests           = %8.1f\n"
	      "Update Requests Discarded = %8.1f\n",
	      totalTimeElapsed,
	      aveCAEventCount,
	      aveUpdateExecuted,
	      aveUpdateRequested,
	      aveUpdateRequestDiscarded);
	} else { 
	    sprintf(caStudyMsg,  
	      "Time Interval (sec)       = %8.2f\n"
	      "CA connection(s)          = %8d\n"
	      "CA connected              = %8d\n"
	      "CA incoming event(s)      = %8d\n"
	      "Active Objects            = %8d\n"
	      "Object(s) Updated         = %8d\n"
	      "Update Requests           = %8d\n"
	      "Update Requests Discarded = %8d\n"
	      "Update Requests Queued    = %8d\n",
	      timeInterval,
	      channelCount,
	      channelConnected,
	      caEventCount,
	      totalTaskCount,
	      updateExecuted,
	      totalUpdateRequested,
	      totalUpdateDiscarded,
	      updateRequestQueued);
	}                   
	str = XmStringLtoRCreate(caStudyMsg,XmSTRING_DEFAULT_CHARSET);
	XtVaSetValues(caStudyLabel,XmNlabelString,str,NULL);
	XmStringFree(str);
	XFlush(XtDisplay(caStudyS));
	XmUpdateDisplay(caStudyS);
	if (globalDisplayListTraversalMode == DL_EXECUTE) {
	    if (errMsgDlgTimeOutId == *id)
	      errMsgDlgTimeOutId = XtAppAddTimeOut(appContext,1000,medmUpdateCAStudyDlg,NULL);
	} else {
	    errMsgDlgTimeOutId = 0;
	}
    } else {
	errMsgDlgTimeOutId = 0;
    }
}

void medmStartUpdateCAStudyDlg() {
    if (globalDisplayListTraversalMode == DL_EXECUTE) {
	if (errMsgDlgTimeOutId == 0) 
	  errMsgDlgTimeOutId = XtAppAddTimeOut(appContext,3000,medmUpdateCAStudyDlg,NULL);
    } else {
	errMsgDlgTimeOutId = 0;
    }
}

int xErrorHandler(Display *dpy, XErrorEvent *event)
{
    char buf[1024];     /* Warning: Fixed Size */
    XGetErrorText(dpy,event->error_code,buf,1024);
#ifdef WIN32
    lprintf("\n%s\n",buf);
#else
    fprintf(stderr,"\n%s\n",buf);
#endif    
    return 0;
}

void xtErrorHandler(char *message)
{
#ifdef WIN32
    lprintf("\n%s\n",message);
#else
    fprintf(stderr,"\n%s\n",message);
#endif    
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

    if (env != NULL) {
      /* Run the help command */
	char *command;
	
	command = (char*)malloc(strlen(env) + strlen(name) + 5);
	sprintf(command, "%s %s &", env, name);
	(void)system(command);
	free(command);
    } else {
      /* KE: Should no longer get here */
      /* Print error message */
	medmPostMsg(1,"displayHelpCallback: The environment variable MEDM_HELP is not set\n"
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

    message = XmInternAtom (XtDisplay(displayInfo->shell), "_MOTIF_WM_MESSAGES", FALSE);
    protocol = XmInternAtom (XtDisplay(displayInfo->shell), "_MEDM_DISPLAY_HELP", FALSE);

    XmAddProtocols(displayInfo->shell, message, &protocol, 1);
    XmAddProtocolCallback(displayInfo->shell, message, protocol, displayHelpCallback, (XtPointer)displayInfo);

    sprintf (buf, "Help _H Ctrl<Key>h f.send_msg %d", protocol);
    XtVaSetValues (displayInfo->shell, XmNmwmMenu, buf, NULL);
}
