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

#include "medm.h"

static void shellCommandInheritValues(ResourceBundle *pRCB, DlElement *p);
static void shellCommandSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void shellCommandSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void shellCommandGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable shellCommandDlDispatchTable = {
    createDlShellCommand,
    NULL,
    executeDlShellCommand,
    writeDlShellCommand,
    NULL,
    shellCommandGetValues,
    shellCommandInheritValues,
    shellCommandSetBackgroundColor,
    shellCommandSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};

#ifdef __cplusplus
static void freePixmapCallback(Widget w, XtPointer cd, XtPointer)
#else
static void freePixmapCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    Pixmap pixmap = (Pixmap) cd;
    
/*     if (pixmap != (Pixmap)0) XmDestroyPixmap(XtScreen(w),pixmap); */
    if (pixmap != (Pixmap)0) XFreePixmap(display,pixmap);
    pixmap=(Pixmap)0;
}

/*
 * local function to render the related display icon into a pixmap
 */
static void renderShellCommandPixmap(Display *display, Pixmap pixmap,
  Pixel fg, Pixel bg, unsigned int width, unsigned int height)
{
    typedef struct { float x; float y;} XY;
/* icon is based on the 25 pixel (w & h) bitmap shellCommand25 */
    static float rectangleX = (float)(12./25.), rectangleY = (float)(4./25.),
      rectangleWidth = (float)(3./25.), rectangleHeight = (float)(14./25.);
    static float dotX = (float)(12./25.), dotY = (float)(20./25.),
      dotWidth = (float)(3./25.), dotHeight = (float)(3./25.);
    GC gc;

    gc = XCreateGC(display,pixmap,0,NULL);
    XSetForeground(display,gc,bg);
    XFillRectangle(display,pixmap,gc,0,0,width,height);
    XSetForeground(display,gc,fg);

    XFillRectangle(display,pixmap,gc,
      (int)(rectangleX*width),
      (int)(rectangleY*height),
      (unsigned int)(MAX(1,(unsigned int)(rectangleWidth*width))),
      (unsigned int)(MAX(1,(unsigned int)(rectangleHeight*height))) );

    XFillRectangle(display,pixmap,gc,
      (int)(dotX*width),
      (int)(dotY*height),
      (unsigned int)(MAX(1,(unsigned int)(dotWidth*width))),
      (unsigned int)(MAX(1,(unsigned int)(dotHeight*height))) );

    XFreeGC(display,gc);
}

void executeDlShellCommand(DisplayInfo *displayInfo, DlElement *dlElement)
{
    Widget localMenuBar;
    Arg args[16];
    int i, shellNumber=0;
    XmString xmString;
    Pixmap shellCommandPixmap;
    unsigned int pixmapSize;
    DlShellCommand *dlShellCommand = dlElement->structure.shellCommand;
/*
 * these are widget ids, but they are recorded in the otherChild widget list
 *   as well, for destruction when new shells are selected at the top level
 */
    Widget shellCommandPulldownMenu, shellCommandMenuButton;
#if 1
    Widget widget;
#endif

/***
*** from the DlShellCommand structure, we've got specifics
*** (MDA)  create a pulldown menu with the following related shell menu
***   entries in it...  --  careful with the XtSetArgs here (special)
***/
    if (dlElement->widget && displayInfo->traversalMode == DL_EDIT) {
	DlObject *po = &(dlElement->structure.shellCommand->object);
	XtVaSetValues(dlElement->widget,
          XmNx, (Position) po->x,
          XmNy, (Position) po->y,
          XmNwidth, (Dimension) po->width,
          XmNheight, (Dimension) po->height,
          NULL);
	return;
    }
    XtSetArg(args[0],XmNforeground,(Pixel)
      displayInfo->colormap[dlShellCommand->clr]);
    XtSetArg(args[1],XmNbackground,(Pixel)
      displayInfo->colormap[dlShellCommand->bclr]);
    XtSetArg(args[2],XmNhighlightThickness,1);
    XtSetArg(args[3],XmNwidth,dlShellCommand->object.width);
    XtSetArg(args[4],XmNheight,dlShellCommand->object.height);
    XtSetArg(args[5],XmNmarginHeight,0);
    XtSetArg(args[6],XmNmarginWidth,0);
    XtSetArg(args[7],XmNresizeHeight,(Boolean)FALSE);
    XtSetArg(args[8],XmNresizeWidth,(Boolean)FALSE);
    XtSetArg(args[9],XmNspacing,0);
    XtSetArg(args[10],XmNx,(Position)dlShellCommand->object.x);
    XtSetArg(args[11],XmNy,(Position)dlShellCommand->object.y);
    XtSetArg(args[12],XmNhighlightOnEnter,TRUE);
    localMenuBar =
      XmCreateMenuBar(displayInfo->drawingArea,"shellCommandMenuBar",args,13);
    dlElement->widget = localMenuBar;
  /* Add handlers */
    addCommonHandlers(dlElement->widget, displayInfo);
    XtManageChild(dlElement->widget);

    colorMenuBar(localMenuBar,
      (Pixel)displayInfo->colormap[dlShellCommand->clr],
      (Pixel)displayInfo->colormap[dlShellCommand->bclr]);

    shellCommandPulldownMenu = XmCreatePulldownMenu(
      localMenuBar,"shellCommandPulldownMenu",args,2);

    pixmapSize = MIN(dlShellCommand->object.width,dlShellCommand->object.height);
/* allowing for shadows etc */
    pixmapSize = (unsigned int) MAX(1,(int)pixmapSize - 8);

/* create shellCommand icon (render to appropriate size) */
    shellCommandPixmap = XCreatePixmap(display,RootWindow(display,screenNum),
      pixmapSize,pixmapSize,XDefaultDepth(display,screenNum));
    renderShellCommandPixmap(display,shellCommandPixmap,
      displayInfo->colormap[dlShellCommand->clr],
      displayInfo->colormap[dlShellCommand->bclr],
      pixmapSize,pixmapSize);

    XtSetArg(args[7],XmNrecomputeSize,(Boolean)False);
    XtSetArg(args[8],XmNlabelPixmap,shellCommandPixmap);
    XtSetArg(args[9],XmNlabelType,XmPIXMAP);
    XtSetArg(args[10],XmNsubMenuId,shellCommandPulldownMenu);
    XtSetArg(args[11],XmNhighlightOnEnter,TRUE);
    widget = XtCreateManagedWidget("shellCommandMenuLabel",
      xmCascadeButtonGadgetClass,
      localMenuBar, args, 12);

/* Add destroy callback to free pixmap from pixmap cache */
    XtAddCallback(widget,
      XmNdestroyCallback,freePixmapCallback,
      (XtPointer)shellCommandPixmap);
    
    for (i = 0; i < MAX_SHELL_COMMANDS; i++) {
	if (strlen(dlShellCommand->command[i].command) > (size_t)0) {
	    xmString = XmStringCreateLocalized(dlShellCommand->command[i].label);
	    XtSetArg(args[3], XmNlabelString,xmString);
	  /* set the displayInfo as the button's userData */
	    XtSetArg(args[4], XmNuserData,(XtPointer)displayInfo);
	    shellCommandMenuButton = XtCreateManagedWidget("relatedButton",
	      xmPushButtonWidgetClass, shellCommandPulldownMenu, args, 5);
	    XtAddCallback(shellCommandMenuButton,XmNactivateCallback,
	      (XtCallbackProc)dmExecuteShellCommand,
	      (XtPointer)&(dlShellCommand->command[i]));
	    XmStringFree(xmString);
	}
    }
}

#ifdef __cplusplus
static void createDlShellCommandEntry(
  DlShellCommandEntry *shellCommand,
  int cmdNumber)
#else
static void createDlShellCommandEntry(
  DlShellCommandEntry *shellCommand,
  int cmdNumber)
#endif
{
/* structure copy */
    *shellCommand = globalResourceBundle.cmdData[cmdNumber];

}


DlElement *createDlShellCommand(DlElement *p)
{
    DlShellCommand *dlShellCommand;
    DlElement *dlElement;
    int cmdNumber;

    dlShellCommand = (DlShellCommand *) malloc(sizeof(DlShellCommand));
    if (!dlShellCommand) return 0;
    if (p) {
	*dlShellCommand = *p->structure.shellCommand;
    } else {
	objectAttributeInit(&(dlShellCommand->object));
	for (cmdNumber = 0; cmdNumber < MAX_SHELL_COMMANDS;cmdNumber++)
	  createDlShellCommandEntry(
	    &(dlShellCommand->command[cmdNumber]),
	    cmdNumber );
	dlShellCommand->clr = globalResourceBundle.clr;
	dlShellCommand->bclr = globalResourceBundle.bclr;
    }

    if (!(dlElement = createDlElement(DL_ShellCommand,
      (XtPointer)      dlShellCommand,
      &shellCommandDlDispatchTable))) {
	free(dlShellCommand);
    }

    return(dlElement);
}

void parseShellCommandEntry(DisplayInfo *displayInfo,
  DlShellCommandEntry *shellCommand)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;

    do {
        switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"label")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(shellCommand->label,token);
	    } else if (!strcmp(token,"name")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(shellCommand->command,token);
	    } else if (!strcmp(token,"args")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(shellCommand->args,token);
	    }
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
        }
    } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );
}

DlElement *parseShellCommand(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlShellCommand *dlShellCommand = 0;
    DlElement *dlElement = createDlShellCommand(NULL);
    int cmdNumber;

    if (!dlElement) return 0;
    dlShellCommand = dlElement->structure.shellCommand;
    do {
        switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlShellCommand->object));
	    } else if (!strncmp(token,"command",7)) {
/*
 * compare the first 7 characters to see if a command entry.
 *   if more than one digit is allowed for the command index, then change
 *   the following code to pick up all the digits (can't use atoi() unless
 *   we get a null-terminated string
 */
		cmdNumber = MIN(
		  token[8] - '0', MAX_SHELL_COMMANDS - 1);
		parseShellCommandEntry(displayInfo,
		  &(dlShellCommand->command[cmdNumber]) );
	    } else if (!strcmp(token,"clr")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlShellCommand->clr = atoi(token) % DL_MAX_COLORS;
	    } else if (!strcmp(token,"bclr")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlShellCommand->bclr = atoi(token) % DL_MAX_COLORS;
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

    return dlElement;
}

void writeDlShellCommandEntry(
  FILE *stream,
  DlShellCommandEntry *entry,
  int index,
  int level)
{
    char indent[16];
 
    memset(indent,'\t',level);
    indent[level] = '\0';
 
    fprintf(stream,"\n%scommand[%d] {",indent,index);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
	if (entry->label[0] != '\0')
	  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,entry->label);
	if (entry->command[0] != '\0')
	  fprintf(stream,"\n%s\tname=\"%s\"",indent,entry->command);
	if (entry->args[0] != '\0')
	  fprintf(stream,"\n%s\targs=\"%s\"",indent,entry->args);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%s\tlabel=\"%s\"",indent,entry->label);
	fprintf(stream,"\n%s\tname=\"%s\"",indent,entry->command);
	fprintf(stream,"\n%s\targs=\"%s\"",indent,entry->args);
    }
#endif
    fprintf(stream,"\n%s}",indent);
}

void writeDlShellCommand(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
    int i;
    char indent[16];
    DlShellCommand *dlShellCommand = dlElement->structure.shellCommand;

    memset(indent,'\t',level);
    indent[level] = '\0';
 
    fprintf(stream,"\n%s\"shell command\" {",indent);
    writeDlObject(stream,&(dlShellCommand->object),level+1);
    for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
#ifdef SUPPORT_0201XX_FILE_FORMAT
  	if (MedmUseNewFileFormat) {
#endif
	    if ((dlShellCommand->command[i].label[0] != '\0') ||
	      (dlShellCommand->command[i].command[0] != '\0') ||
	      (dlShellCommand->command[i].args[0] != '\0'))
	      writeDlShellCommandEntry(stream,&(dlShellCommand->command[i]),i,level+1);
#ifdef SUPPORT_0201XX_FILE_FORMAT
	} else {
	    writeDlShellCommandEntry(stream,&(dlShellCommand->command[i]),i,level+1);
	}
#endif
    }
    fprintf(stream,"\n%s\tclr=%d",indent,dlShellCommand->clr);
    fprintf(stream,"\n%s\tbclr=%d",indent,dlShellCommand->bclr);
    fprintf(stream,"\n%s}",indent);
}

#ifdef __cplusplus
static void shellCommandCallback(Widget, XtPointer client_data,
  XtPointer cbs)
#else
static void shellCommandCallback(Widget w, XtPointer client_data,
  XtPointer cbs)
#endif

{
    char *command;
    DisplayInfo *displayInfo;
    XmSelectionBoxCallbackStruct *call_data =
      (XmSelectionBoxCallbackStruct *) cbs;

    Widget realParent = (Widget)client_data;
    displayInfo = dmGetDisplayInfoFromWidget(realParent);

    switch(call_data->reason) {
    case XmCR_CANCEL:
	XtUnmanageChild(displayInfo->shellCommandPromptD);
	break;
    case XmCR_OK:
	XmStringGetLtoR(call_data->value,XmSTRING_DEFAULT_CHARSET,&command);
      /* (MDA) NB: system() blocks! need to background (&) to not block */
      /* KE: User has to do this as it is coded */
	if (command && *command) {
#if 0
	  /* KE: Isn't necessary */
	    performMacroSubstitutions(displayInfo,command,processedCommand,
	      2*MAX_TOKEN_LENGTH);
	    if (strlen(processedCommand) > (size_t) 0)
	      parseAndExecCommand(displayInfo,processedCommand);
#else
	    parseAndExecCommand(displayInfo,command);
#endif	    
	    XtFree(command);
	}
	XtUnmanageChild(displayInfo->shellCommandPromptD);
	break;
    case XmCR_HELP:
	callBrowser(MEDM_HELP_PATH"/MEDM.html#ShellCommand");
	break;
    }
}

Widget createShellCommandPromptD(Widget parent)
{
    Arg args[6];
    int n;
    XmString title;
    Widget prompt;

    title = XmStringCreateLocalized("Command");
    n = 0;
    XtSetArg(args[n],XmNdialogTitle,title); n++;
    XtSetArg(args[n],XmNdialogStyle,XmDIALOG_FULL_APPLICATION_MODAL); n++;
    XtSetArg(args[n],XmNselectionLabelString,title); n++;
    XtSetArg(args[n],XmNautoUnmanage,False); n++;
  /* update global for selection box widget access */
    prompt = XmCreatePromptDialog(parent,
      "shellCommandPromptD",args,n);
    XmStringFree(title);

    XtAddCallback(prompt, XmNcancelCallback,shellCommandCallback,parent);
    XtAddCallback(prompt,XmNokCallback,shellCommandCallback,parent);
    XtAddCallback(prompt,XmNhelpCallback,shellCommandCallback,parent);
    return (prompt);
}

#ifdef __cplusplus
void dmExecuteShellCommand(
  Widget  w,
  DlShellCommandEntry *commandEntry,
  XmPushButtonCallbackStruct *)
#else
void dmExecuteShellCommand(
  Widget  w,
  DlShellCommandEntry *commandEntry,
  XmPushButtonCallbackStruct *call_data)
#endif
{
    XmString xmString;
    DisplayInfo *displayInfo;
    XtPointer userData;
    int cmdLength, argsLength;
    char shellCommand[2*MAX_TOKEN_LENGTH];
    char *promptPosition;

  /* Return if command is empty */
    cmdLength = strlen(commandEntry->command);
    if(cmdLength < 0) return;
    argsLength = strlen(commandEntry->args);
    
  /* Get the displayInfo from the userData */
    XtVaGetValues(w,XmNuserData,&userData,NULL);
    displayInfo = (DisplayInfo *)userData;
    currentDisplayInfo = displayInfo;
    
  /* Copy the command to shellCommand */
    strcpy(shellCommand,commandEntry->command);

  /* Concatenate the arguments */
    if(argsLength > 0) {
	strcat(shellCommand," ");
	strcat(shellCommand,commandEntry->args);
    }
    
  /* Check for a prompt char */
    promptPosition = strchr(shellCommand,SHELL_CMD_PROMPT_CHAR);
    if (promptPosition == NULL) {
      /* No  prompt character found */
      /* (MDA) NB: system() blocks! need to background (&) to not block */
      /* KE: User has to do this as it is coded */
#if 0	
      /* KE: Isn't necessary */
	performMacroSubstitutions(displayInfo,
	  shellCommand,processedShellCommand,2*MAX_TOKEN_LENGTH);
	if (strlen(processedShellCommand) > (size_t) 0)
	  parseAndExecCommand(displayInfo,processedShellCommand);
#else	
	  parseAndExecCommand(displayInfo,shellCommand);
#endif	
    } else {
      /* A prompt character found, replace it with NULL */
	promptPosition = strchr(shellCommand,SHELL_CMD_PROMPT_CHAR);
	*promptPosition = '\0';

      /* Add & to remind to run in background */
	strcat(shellCommand,"&");

      /* Create shell command prompt dialog if necessary */
	if (displayInfo->shellCommandPromptD == (Widget)NULL) {
	    displayInfo->shellCommandPromptD = createShellCommandPromptD(
	      displayInfo->shell);
	}
	
      /* Set the command in the dialog */
	xmString = XmStringCreateLocalized(shellCommand);
	XtVaSetValues(displayInfo->shellCommandPromptD,XmNtextString,
	  xmString,NULL);
	XmStringFree(xmString);

      /* Popup the prompt dialog, callback will do the rest */
	XtManageChild(displayInfo->shellCommandPromptD);
    }
}

static void shellCommandInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlShellCommand *dlShellCommand = p->structure.shellCommand;
    medmGetValues(pRCB,
      CLR_RC,        &(dlShellCommand->clr),
      BCLR_RC,       &(dlShellCommand->bclr),
      -1);
}

static void shellCommandGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlShellCommand *dlShellCommand = p->structure.shellCommand;
    medmGetValues(pRCB,
      X_RC,          &(dlShellCommand->object.x),
      Y_RC,          &(dlShellCommand->object.y),
      WIDTH_RC,      &(dlShellCommand->object.width),
      HEIGHT_RC,     &(dlShellCommand->object.height),
      CLR_RC,        &(dlShellCommand->clr),
      BCLR_RC,       &(dlShellCommand->bclr),
      SHELLDATA_RC,  &(dlShellCommand->command),
      -1);
}

static void shellCommandSetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlShellCommand *dlShellCommand = p->structure.shellCommand;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlShellCommand->bclr),
      -1);
}

static void shellCommandSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlShellCommand *dlShellCommand = p->structure.shellCommand;
    medmGetValues(pRCB,
      CLR_RC,        &(dlShellCommand->clr),
      -1);
}
