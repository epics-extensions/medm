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

#define DEBUG_REDRAW 0

#include "medm.h"
#include <Xm/MwmUtil.h>

#define CMD_APPLY_BTN	0
#define CMD_CLOSE_BTN	1

static void shellCommandInheritValues(ResourceBundle *pRCB, DlElement *p);
static void shellCommandSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void shellCommandSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void shellCommandGetValues(ResourceBundle *pRCB, DlElement *p);

static DlDispatchTable shellCommandDlDispatchTable = {
    createDlShellCommand,
    NULL,
    executeDlShellCommand,
    hideDlShellCommand,
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

/* Global variables */

static Widget cmdMatrix = NULL;
static String cmdColumnLabels[] = {"Command Label","Command","Arguments",};
static int cmdColumnMaxLengths[] = {MAX_TOKEN_LENGTH-1,MAX_TOKEN_LENGTH-1,
				    MAX_TOKEN_LENGTH-1,};
static short cmdColumnWidths[] = {36,36,36,};
static unsigned char cmdColumnLabelAlignments[] = {
    XmALIGNMENT_CENTER, XmALIGNMENT_CENTER, XmALIGNMENT_CENTER,};
/* and the cmdCells array of strings (filled in from globalResourceBundle...) */
static String cmdRows[MAX_SHELL_COMMANDS][3];
static String *cmdCells[MAX_SHELL_COMMANDS];

static void freePixmapCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    Pixmap pixmap = (Pixmap) cd;
    
    UNREFERENCED(cbs);

/*     if(pixmap != (Pixmap)0) XmDestroyPixmap(XtScreen(w),pixmap); */
    if(pixmap != (Pixmap)0) XFreePixmap(display,pixmap);
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
  /* These are widget ids, but they are recorded in the otherChild
   *   widget list as well, for destruction when new shells are
   *   selected at the top level */
    Widget shellCommandPulldownMenu, shellCommandMenuButton;
#if 1
    Widget widget;
#endif

#if DEBUG_REDRAW
    widget=dlElement->widget;
    print("executeDlShellCommand: %x hidden=%s widget=%x managed=%s\n",
      dlElement,dlElement->hidden?"Yes":"No",
      widget,
      widget?(XtIsManaged(widget)?"Yes":"No"):"NR");
#endif    
  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

  /***
   *** from the DlShellCommand structure, we've got specifics
   *** (MDA)  create a pulldown menu with the following related shell menu
   ***   entries in it...  --  careful with the XtSetArgs here (special)
   ***/
    if(dlElement->widget) {
      /* The widget already exists */
	if(displayInfo->traversalMode == DL_EDIT) {
	  /* In EDIT mode update its dimensions */
	    DlObject *po = &(dlElement->structure.shellCommand->object);
	    XtVaSetValues(dlElement->widget,
	      XmNx, (Position) po->x,
	      XmNy, (Position) po->y,
	      XmNwidth, (Dimension) po->width,
	      XmNheight, (Dimension) po->height,
	      NULL);
	} else {
	  /* In EXECUTE manage it if necessary */
	    if(!XtIsManaged(dlElement->widget)) {
		XtManageChild(dlElement->widget);
	    }
	}
	return;
    }

  /* Make the widget */
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
    
    for(i = 0; i < MAX_SHELL_COMMANDS; i++) {
	if(strlen(dlShellCommand->command[i].command) > (size_t)0) {
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

void hideDlShellCommand(DisplayInfo *displayInfo, DlElement *dlElement)
{
#if DEBUG_REDRAW
    Widget widget = dlElement->widget;
    
    print("hideDlShellCommand: %x hidden=%s widget=%x managed=%s\n",
      dlElement,dlElement->hidden?"Yes":"No",
      widget,
      widget?(XtIsManaged(widget)?"Yes":"No"):"NR");
#endif    

  /* Use generic hide for an element with a widget */
    hideWidgetElement(displayInfo, dlElement);
#if DEBUG_REDRAW
    print("                    %x hidden=%s widget=%x managed=%s\n",
      dlElement,dlElement->hidden?"Yes":"No",
      widget,
      widget?(XtIsManaged(widget)?"Yes":"No"):"NR");
#endif    
}

static void createDlShellCommandEntry(DlShellCommandEntry *shellCommand,
  int cmdNumber)
{
  /* Structure copy */
    *shellCommand = globalResourceBundle.cmdData[cmdNumber];

}

DlElement *createDlShellCommand(DlElement *p)
{
    DlShellCommand *dlShellCommand;
    DlElement *dlElement;
    int cmdNumber;

    dlShellCommand = (DlShellCommand *)malloc(sizeof(DlShellCommand));
    if(!dlShellCommand) return 0;
    if(p) {
	*dlShellCommand = *p->structure.shellCommand;
    } else {
	objectAttributeInit(&(dlShellCommand->object));
	for(cmdNumber = 0; cmdNumber < MAX_SHELL_COMMANDS;cmdNumber++)
	  createDlShellCommandEntry(
	    &(dlShellCommand->command[cmdNumber]),
	    cmdNumber );
	dlShellCommand->clr = globalResourceBundle.clr;
	dlShellCommand->bclr = globalResourceBundle.bclr;
    }

    if(!(dlElement = createDlElement(DL_ShellCommand,
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
	    if(!strcmp(token,"label")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(shellCommand->label,token);
	    } else if(!strcmp(token,"name")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(shellCommand->command,token);
	    } else if(!strcmp(token,"args")) {
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
    } while( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
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

    if(!dlElement) return 0;
    dlShellCommand = dlElement->structure.shellCommand;
    do {
        switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlShellCommand->object));
	    } else if(!strncmp(token,"command",7)) {
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
	    } else if(!strcmp(token,"clr")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlShellCommand->clr = atoi(token) % DL_MAX_COLORS;
	    } else if(!strcmp(token,"bclr")) {
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
    } while( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
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
    if(MedmUseNewFileFormat) {
#endif
	if(entry->label[0] != '\0')
	  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,entry->label);
	if(entry->command[0] != '\0')
	  fprintf(stream,"\n%s\tname=\"%s\"",indent,entry->command);
	if(entry->args[0] != '\0')
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
    for(i = 0; i < MAX_RELATED_DISPLAYS; i++) {
#ifdef SUPPORT_0201XX_FILE_FORMAT
  	if(MedmUseNewFileFormat) {
#endif
	    if((dlShellCommand->command[i].label[0] != '\0') ||
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

static void shellCommandCallback(Widget w, XtPointer client_data,
  XtPointer cbs)
{
    char *command;
    DisplayInfo *displayInfo;
    XmSelectionBoxCallbackStruct *call_data =
      (XmSelectionBoxCallbackStruct *) cbs;
    Widget realParent = (Widget)client_data;
    displayInfo = dmGetDisplayInfoFromWidget(realParent);

    UNREFERENCED(w);

    switch(call_data->reason) {
    case XmCR_CANCEL:
	XtUnmanageChild(displayInfo->shellCommandPromptD);
	break;
    case XmCR_OK:
	XmStringGetLtoR(call_data->value,XmFONTLIST_DEFAULT_TAG,&command);
      /* (MDA) NB: system() blocks! need to background (&) to not block */
      /* KE: User has to do this as it is coded */
	if(command && *command) {
#if 0
	  /* KE: Isn't necessary */
	    performMacroSubstitutions(displayInfo,command,processedCommand,
	      2*MAX_TOKEN_LENGTH);
	    if(strlen(processedCommand) > (size_t) 0)
	      parseAndExecCommand(displayInfo,processedCommand);
#else
	    parseAndExecCommand(displayInfo,command);
#endif	    
	    XtFree(command);
	}
	XtUnmanageChild(displayInfo->shellCommandPromptD);
	break;
    case XmCR_HELP:
	callBrowser(medmHelpPath,"#ShellCommand");
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

void dmExecuteShellCommand(Widget  w, DlShellCommandEntry *commandEntry,
  XmPushButtonCallbackStruct *cbs)
{
    XmString xmString;
    DisplayInfo *displayInfo;
    XtPointer userData;
    int cmdLength, argsLength;
    char shellCommand[2*MAX_TOKEN_LENGTH];
    char *promptPosition;

    UNREFERENCED(cbs);

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
    if(promptPosition == NULL) {
      /* No  prompt character found */
      /* (MDA) NB: system() blocks! need to background (&) to not block */
      /* KE: User has to do this as it is coded */
#if 0	
      /* KE: Isn't necessary */
	performMacroSubstitutions(displayInfo,
	  shellCommand,processedShellCommand,2*MAX_TOKEN_LENGTH);
	if(strlen(processedShellCommand) > (size_t) 0)
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
	if(displayInfo->shellCommandPromptD == (Widget)NULL) {
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

static void shellCommandActivate(Widget w, XtPointer cd, XtPointer cb)
{
    DisplayInfo *cdi=currentDisplayInfo;
    int buttonType = (int) cd;
    String **newCells;
    int i;

    UNREFERENCED(w);
    UNREFERENCED(cb);

    switch(buttonType) {
    case CMD_APPLY_BTN:
      /* Commit changes in matrix to global matrix array data */
	XbaeMatrixCommitEdit(cmdMatrix,False);
	XtVaGetValues(cmdMatrix,XmNcells,&newCells,NULL);
      /* Now update globalResourceBundle...*/
	for (i = 0; i < MAX_SHELL_COMMANDS; i++) {
	    strcpy(globalResourceBundle.cmdData[i].label, newCells[i][0]);
	    strcpy(globalResourceBundle.cmdData[i].command, newCells[i][1]);
	    strcpy(globalResourceBundle.cmdData[i].args, newCells[i][2]);
	}
      /* and update the elements (since this level of "OK" is analogous
       *	to changing text in a text field in the resource palette
       *	(don't need to traverse the display list since these changes
       *	 aren't visible at the first level)
       */
	if(cdi) {
	    DlElement *dlElement = FirstDlElement(
	      cdi->selectedDlElementList);
	    unhighlightSelectedElements();
	    while(dlElement) {
	      /* KE: Changed = to == here */
		if(dlElement->structure.element->type == DL_ShellCommand)
		  updateElementFromGlobalResourceBundle(dlElement->structure.element);
		dlElement = dlElement->next;
	    }
	}
	if(cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	XtPopdown(shellCommandS);
	break;
    case CMD_CLOSE_BTN:
	XtPopdown(shellCommandS);
	break;
    }
}

/*
 * create shell command data dialog
 */
Widget createShellCommandDataDialog(
  Widget parent)
{
    Widget shell, applyButton, closeButton;
    Dimension cWidth, cHeight, aWidth, aHeight;
    Arg args[12];
    XmString xmString;
    int i, j, n;
    static Boolean first = True;


  /* initialize those file-scoped globals */
    if(first) {
	first = False;
	for (i = 0; i < MAX_SHELL_COMMANDS; i++) {
	    for (j = 0; j < 3; j++) cmdRows[i][j] = NULL;
	    cmdCells[i] = &cmdRows[i][0];
	}
    }

  /*
   * now create the interface
   *
   *	       label | cmd | args
   *	       -------------------
   *	    1 |  A      B      C
   *	    2 | 
   *	    3 | 
   *		     ...
   *		 OK     CANCEL
   */

    n = 0;
    XtSetArg(args[n],XmNautoUnmanage,False); n++;
    XtSetArg(args[n],XmNmarginHeight,8); n++;
    XtSetArg(args[n],XmNmarginWidth,8); n++;
/*     cmdForm = XmCreateFormDialog(mainShell,"shellCommandDataF",args,n); */
    cmdForm = XmCreateFormDialog(parent,"shellCommandDataF",args,n);
    shell = XtParent(cmdForm);
    n = 0;
    XtSetArg(args[n],XmNtitle,"Shell Command Data"); n++;
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
  /* KE: The following is necessary for Exceed, which turns off the
     resize function with the handles.  It should not be necessary */
    XtSetArg(args[n],XmNmwmFunctions, MWM_FUNC_ALL); n++;
    XtSetValues(shell,args,n);
    XmAddWMProtocolCallback(shell,WM_DELETE_WINDOW,
      shellCommandActivate,(XtPointer)CMD_CLOSE_BTN);
    n = 0;
    XtSetArg(args[n],XmNrows,MAX_RELATED_DISPLAYS); n++;
    XtSetArg(args[n],XmNcolumns,3); n++;
    XtSetArg(args[n],XmNcolumnMaxLengths,cmdColumnMaxLengths); n++;
    XtSetArg(args[n],XmNcolumnWidths,cmdColumnWidths); n++;
    XtSetArg(args[n],XmNcolumnLabels,cmdColumnLabels); n++;
    XtSetArg(args[n],XmNcolumnMaxLengths,cmdColumnMaxLengths); n++;
    XtSetArg(args[n],XmNcolumnWidths,cmdColumnWidths); n++;
    XtSetArg(args[n],XmNcolumnLabelAlignments,cmdColumnLabelAlignments); n++;
    XtSetArg(args[n],XmNboldLabels,False); n++;
    cmdMatrix = XtCreateManagedWidget("cmdMatrix",
      xbaeMatrixWidgetClass,cmdForm,args,n);


    xmString = XmStringCreateLocalized("Cancel");
    n = 0;
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    closeButton = XmCreatePushButton(cmdForm,"closeButton",args,n);
    XtAddCallback(closeButton,XmNactivateCallback,
      shellCommandActivate,(XtPointer)CMD_CLOSE_BTN);
    XtManageChild(closeButton);
    XmStringFree(xmString);

    xmString = XmStringCreateLocalized("Apply");
    n = 0;
    XtSetArg(args[n],XmNlabelString,xmString); n++;
    applyButton = XmCreatePushButton(cmdForm,"applyButton",args,n);
    XtAddCallback(applyButton,XmNactivateCallback,
      shellCommandActivate,(XtPointer)CMD_APPLY_BTN);
    XtManageChild(applyButton);
    XmStringFree(xmString);

  /* make APPLY and CLOSE buttons same size */
    XtVaGetValues(closeButton,XmNwidth,&cWidth,XmNheight,&cHeight,NULL);
    XtVaGetValues(applyButton,XmNwidth,&aWidth,XmNheight,&aHeight,NULL);
    XtVaSetValues(closeButton,XmNwidth,MAX(cWidth,aWidth),
      XmNheight,MAX(cHeight,aHeight),NULL);

  /* and make the APPLY button the default for the form */
    XtVaSetValues(cmdForm,XmNdefaultButton,applyButton,NULL);

  /*
   * now do form layout 
   */

  /* cmdMatrix */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
    XtSetValues(cmdMatrix,args,n);
  /* apply */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,cmdMatrix); n++;
    XtSetArg(args[n],XmNtopOffset,12); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNleftPosition,30); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomOffset,12); n++;
    XtSetValues(applyButton,args,n);
  /* close */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,cmdMatrix); n++;
    XtSetArg(args[n],XmNtopOffset,12); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNrightPosition,70); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomOffset,12); n++;
    XtSetValues(closeButton,args,n);

    XtManageChild(cmdForm);

    return shell;
}

/*
 * access function (for file-scoped globals, etc) to udpate the
 *	shell command data dialog with the values currently in
 *	globalResourceBundle
 */
void updateShellCommandDataDialog()
{
    int i;

    for (i = 0; i < MAX_SHELL_COMMANDS; i++) {
	cmdRows[i][0] = globalResourceBundle.cmdData[i].label;
	cmdRows[i][1] = globalResourceBundle.cmdData[i].command;
	cmdRows[i][2] = globalResourceBundle.cmdData[i].args;
    }
    if(cmdMatrix != NULL) XtVaSetValues(cmdMatrix,XmNcells,cmdCells,NULL);
  
}
