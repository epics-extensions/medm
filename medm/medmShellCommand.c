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

/* Local function to render the shell command display icon into a
 * pixmap */
static void renderShellCommandPixmap(Display *display, Pixmap pixmap,
  Pixel fg, Pixel bg, Dimension width, Dimension height,
  XFontStruct *font, int icon, char *label)
{
  /* Icon is based on the 25 pixel (w & h) bitmap shellCommand25 */
    static float rectangleX = (float)(12./25.), rectangleY = (float)(4./25.),
      rectangleWidth = (float)(3./25.), rectangleHeight = (float)(14./25.);
    static float dotX = (float)(12./25.), dotY = (float)(20./25.),
      dotWidth = (float)(3./25.), dotHeight = (float)(3./25.);
    GC gc = XCreateGC(display,pixmap,0,NULL);

  /* Eliminate events that we do not handle anyway */
    XSetGraphicsExposures(display,gc,False);

#if 0
    print("renderShellCommandPixmap: width=%d height=%d label=|%s|\n",
      width,height,label?label:"NULL");
#endif

  /* Draw the background */
    XSetForeground(display,gc,bg);
    XFillRectangle(display,pixmap,gc,0,0,width,height);
    XSetForeground(display,gc,fg);

  /* Draw the icon */
    if(icon) {
	XFillRectangle(display,pixmap,gc,
	  (int)(rectangleX*height),
	  (int)(rectangleY*height),
	  (Dimension)(MAX(1,(Dimension)(rectangleWidth*height))),
	  (Dimension)(MAX(1,(Dimension)(rectangleHeight*height))) );

	XFillRectangle(display,pixmap,gc,
	  (int)(dotX*height),
	  (int)(dotY*height),
	  (Dimension)(MAX(1,(Dimension)(dotWidth*height))),
	  (Dimension)(MAX(1,(Dimension)(dotHeight*height))) );
    }

  /* Draw the label */
    if(label && *label) {
	int base;

	XSetFont(display,gc,font->fid);
	base=(height+font->ascent-font->descent)/2;
	XDrawString(display,pixmap,gc,
	  icon?height:0,base,label,strlen(label));
    }

    XFreeGC(display,gc);
}

void executeDlShellCommand(DisplayInfo *displayInfo, DlElement *dlElement)
{
    Widget localMenuBar, tearOff;
    Arg args[16];
    int nargs;
    int i, index, icon;
    char *label;
    XmString xmString;
    Pixmap pixmap;
    Dimension pixmapH, pixmapW;
    int iNumberOfCommands = 0;
    DlShellCommand *dlShellCommand = dlElement->structure.shellCommand;

  /* These are widget ids, but they are recorded in the otherChild
   * widget list as well, for destruction when new shells are selected
   * at the top level */
    Widget shellCommandPulldownMenu, shellCommandMenuButton;
    Widget widget;

#if DEBUG_REDRAW
    widget=dlElement->widget;
    print("executeDlShellCommand: %x hidden=%s widget=%x managed=%s\n",
      dlElement,dlElement->hidden?"Yes":"No",
      widget,
      widget?(XtIsManaged(widget)?"Yes":"No"):"NR");
#endif

  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

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

  /* Count number of commands with non-NULL labels */
    for(i = 0; i < MAX_SHELL_COMMANDS; i++) {
	if(dlShellCommand->command[i].label[0] != '\0') {
	    iNumberOfCommands++;
	}
    }

  /* Get the size for the graphic part of pixmap */
    pixmapH = MIN(dlShellCommand->object.width,
      dlShellCommand->object.height);
  /* Allow for shadows, etc. */
    pixmapH = (unsigned int)MAX(1,(int)pixmapH - 8);

  /* Create the widget depending on the number of items */
    if(iNumberOfCommands <= 1) {
      /* Create the pixmap */
	if(dlShellCommand->label[0] == '\0') {
	    label=NULL;
	    index=0;
	    icon=1;
	    pixmapW=pixmapH;
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	} else if(dlShellCommand->label[0] == '-') {
	    int usedWidth;

	    label=dlShellCommand->label+1;
	    index=messageButtonFontListIndex(dlShellCommand->object.height);
	    icon=0;
	    usedWidth=XTextWidth(fontTable[index],label,strlen(label));
	    pixmapW=MAX(usedWidth,1);
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	} else {
	    int usedWidth;

	    label=dlShellCommand->label;
	    index=messageButtonFontListIndex(dlShellCommand->object.height);
	    icon=1;
	    usedWidth=XTextWidth(fontTable[index],label,strlen(label));
	    pixmapW=pixmapH+usedWidth;
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	}
      /* Draw the pixmap */
	renderShellCommandPixmap(display,pixmap,
	  displayInfo->colormap[dlShellCommand->clr],
	  displayInfo->colormap[dlShellCommand->bclr],
	  pixmapW, pixmapH, fontTable[index], icon, label);

      /* Create a push button */
	nargs = 0;
	dlElement->widget = createPushButton(
	  displayInfo->drawingArea,
	  &(dlShellCommand->object),
	  displayInfo->colormap[dlShellCommand->clr],
	  displayInfo->colormap[dlShellCommand->bclr],
	  pixmap,
	  NULL,     /* There a pixmap, not a label on the button */
	  (XtPointer)displayInfo);
      /* Add the callbacks for bringing up the menu */
	if(globalDisplayListTraversalMode == DL_EXECUTE) {
	    int i;

	  /* Check the display array to find the first non-empty one */
	    for(i=0; i < MAX_RELATED_DISPLAYS; i++) {
		if(*(dlShellCommand->command[i].label)) {
		    XtAddCallback(dlElement->widget,XmNactivateCallback,
		      (XtCallbackProc)dmExecuteShellCommand,
		      (XtPointer)&(dlShellCommand->command[i]));
		  /* Set the displayInfo as the button's userData */
		    XtVaSetValues(dlElement->widget,
		      XmNuserData,(XtPointer)displayInfo,
		      NULL);
		    break;
		}
	    }
	}
      /* Add handlers */
	addCommonHandlers(dlElement->widget, displayInfo);
	XtManageChild(dlElement->widget);
    } else {
      /* Make the widget */
	nargs = 0;
	XtSetArg(args[nargs],XmNforeground,(Pixel)
	  displayInfo->colormap[dlShellCommand->clr]); nargs++;
	XtSetArg(args[nargs],XmNbackground,(Pixel)
	  displayInfo->colormap[dlShellCommand->bclr]); nargs++;
	XtSetArg(args[nargs],XmNhighlightThickness,0); nargs++;
	XtSetArg(args[nargs],XmNwidth,dlShellCommand->object.width); nargs++;
	XtSetArg(args[nargs],XmNheight,dlShellCommand->object.height); nargs++;
	XtSetArg(args[nargs],XmNmarginHeight,0); nargs++;
	XtSetArg(args[nargs],XmNmarginWidth,0); nargs++;
	XtSetArg(args[nargs],XmNresizeHeight,(Boolean)FALSE); nargs++;
	XtSetArg(args[nargs],XmNresizeWidth,(Boolean)FALSE); nargs++;
	XtSetArg(args[nargs],XmNspacing,0); nargs++;
	XtSetArg(args[nargs],XmNx,(Position)dlShellCommand->object.x); nargs++;
	XtSetArg(args[nargs],XmNy,(Position)dlShellCommand->object.y); nargs++;
	localMenuBar =
	  XmCreateMenuBar(displayInfo->drawingArea,"shellCommandMenuBar",
	    args,nargs);
	dlElement->widget = localMenuBar;

      /* Add handlers */
	addCommonHandlers(dlElement->widget, displayInfo);
	XtManageChild(dlElement->widget);

#if EXPLICITLY_OVERWRITE_CDE_COLORS
      /* Color menu bar explicitly to avoid CDE interference */
	colorMenuBar(localMenuBar,
	  (Pixel)displayInfo->colormap[dlShellCommand->clr],
	  (Pixel)displayInfo->colormap[dlShellCommand->bclr]);
#endif

      /* Create the pulldown menu */
	nargs = 0;
	XtSetArg(args[nargs],XmNforeground,(Pixel)
	  displayInfo->colormap[dlShellCommand->clr]); nargs++;
	XtSetArg(args[nargs],XmNbackground,(Pixel)
	  displayInfo->colormap[dlShellCommand->bclr]); nargs++;
#if 0
	XtSetArg(args[nargs], XmNtearOffModel, XmTEAR_OFF_DISABLED); nargs++;
#endif
	shellCommandPulldownMenu = XmCreatePulldownMenu(
	  localMenuBar,"shellCommandPulldownMenu",args,nargs);
      /* Make the tear off colors right */
	tearOff = XmGetTearOffControl(shellCommandPulldownMenu);
	if(tearOff) {
	    XtVaSetValues(tearOff,
	      XmNforeground,(Pixel)displayInfo->colormap[dlShellCommand->clr],
	      XmNbackground,(Pixel)displayInfo->colormap[dlShellCommand->bclr],
	      NULL);
	}

      /* Create the pixmap */
	if(dlShellCommand->label[0] == '\0') {
	    label=NULL;
	    index=0;
	    icon=1;
	    pixmapW=pixmapH;
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	} else if(dlShellCommand->label[0] == '-') {
	    int usedWidth;

	    label=dlShellCommand->label+1;
	    index=messageButtonFontListIndex(dlShellCommand->object.height);
	    icon=0;
	    usedWidth=XTextWidth(fontTable[index],label,strlen(label));
	    pixmapW=MAX(usedWidth,1);
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	} else {
	    int usedWidth;

	    label=dlShellCommand->label;
	    index=messageButtonFontListIndex(dlShellCommand->object.height);
	    icon=1;
	    usedWidth=XTextWidth(fontTable[index],label,strlen(label));
	    pixmapW=pixmapH+usedWidth;
	    pixmap = XCreatePixmap(display,
	      RootWindow(display,screenNum), pixmapW, pixmapH,
	      XDefaultDepth(display,screenNum));
	}
      /* Draw the pixmap */
	renderShellCommandPixmap(display,pixmap,
	  displayInfo->colormap[dlShellCommand->clr],
	  displayInfo->colormap[dlShellCommand->bclr],
	  pixmapW, pixmapH, fontTable[index], icon, label);
      /* Create a cascade button */
	nargs = 0;
	XtSetArg(args[nargs],XmNforeground,(Pixel)
	  displayInfo->colormap[dlShellCommand->clr]); nargs++;
	XtSetArg(args[nargs],XmNbackground,(Pixel)
	  displayInfo->colormap[dlShellCommand->bclr]); nargs++;
	XtSetArg(args[nargs],XmNhighlightThickness,0); nargs++;
	XtSetArg(args[nargs],XmNwidth,dlShellCommand->object.width); nargs++;
	XtSetArg(args[nargs],XmNheight,dlShellCommand->object.height); nargs++;
	XtSetArg(args[nargs],XmNmarginHeight,0); nargs++;
	XtSetArg(args[nargs],XmNmarginWidth,0); nargs++;
	XtSetArg(args[nargs],XmNrecomputeSize,(Boolean)False); nargs++;
	XtSetArg(args[nargs],XmNlabelPixmap,pixmap); nargs++;
	XtSetArg(args[nargs],XmNlabelType,XmPIXMAP); nargs++;
	XtSetArg(args[nargs],XmNsubMenuId,shellCommandPulldownMenu); nargs++;
	widget = XtCreateManagedWidget("shellCommandMenuLabel",
	  xmCascadeButtonGadgetClass,
	  localMenuBar, args, nargs);

	for(i = 0; i < MAX_SHELL_COMMANDS; i++) {
	    if(strlen(dlShellCommand->command[i].command) > (size_t)0) {
		xmString =
		  XmStringCreateLocalized(dlShellCommand->command[i].label);
		nargs = 0;
		XtSetArg(args[nargs],XmNforeground,(Pixel)
		  displayInfo->colormap[dlShellCommand->clr]); nargs++;
		XtSetArg(args[nargs],XmNbackground,(Pixel)
		  displayInfo->colormap[dlShellCommand->bclr]); nargs++;
		XtSetArg(args[nargs], XmNlabelString,xmString); nargs++;
	      /* Set the displayInfo as the button's userData */
		XtSetArg(args[nargs], XmNuserData,(XtPointer)displayInfo); nargs++;
		shellCommandMenuButton =
		  XtCreateManagedWidget("shellCommandButton",
		  xmPushButtonWidgetClass, shellCommandPulldownMenu, args, nargs);
		XtAddCallback(shellCommandMenuButton,XmNactivateCallback,
		  (XtCallbackProc)dmExecuteShellCommand,
		  (XtPointer)&(dlShellCommand->command[i]));
		XmStringFree(xmString);
	    }
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
	for(cmdNumber = 0; cmdNumber < MAX_SHELL_COMMANDS; cmdNumber++)
	  createDlShellCommandEntry(
	    &(dlShellCommand->command[cmdNumber]),
	    cmdNumber );
	dlShellCommand->clr = globalResourceBundle.clr;
	dlShellCommand->bclr = globalResourceBundle.bclr;
	dlShellCommand->label[0] = '\0';
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
}

DlElement *parseShellCommand(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlShellCommand *dlShellCommand = 0;
    DlElement *dlElement = createDlShellCommand(NULL);
    int cmdNumber;
    int rc;

    if(!dlElement) return 0;
    dlShellCommand = dlElement->structure.shellCommand;
    do {
        switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlShellCommand->object));
	    } else if(!strncmp(token,"command",7)) {
	      /* Get the command number */
		cmdNumber=MAX_SHELL_COMMANDS-1;
		rc=sscanf(token,"command[%d]",&cmdNumber);
		if(rc == 0 || rc == EOF || cmdNumber < 0 ||
		  cmdNumber > MAX_SHELL_COMMANDS-1) {
		    cmdNumber=MAX_SHELL_COMMANDS-1;
		}
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
	    } else if(!strcmp(token,"label")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlShellCommand->label,token);
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

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
	fprintf(stream,"\n%s\"shell command\" {",indent);
	writeDlObject(stream,&(dlShellCommand->object),level+1);
	for(i = 0; i < MAX_SHELL_COMMANDS; i++) {
	    if((dlShellCommand->command[i].label[0] != '\0') ||
	      (dlShellCommand->command[i].command[0] != '\0') ||
	      (dlShellCommand->command[i].args[0] != '\0')) {
		writeDlShellCommandEntry(stream,
		  &(dlShellCommand->command[i]),i,level+1);
	    }
	}
	fprintf(stream,"\n%s\tclr=%d",indent,dlShellCommand->clr);
	fprintf(stream,"\n%s\tbclr=%d",indent,dlShellCommand->bclr);
	if(dlShellCommand->label[0] != '\0')
	  fprintf(stream,"\n%s\tlabel=\"%s\"",indent,dlShellCommand->label);
	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%s\"shell command\" {",indent);
	writeDlObject(stream,&(dlShellCommand->object),level+1);
	for(i = 0; i < MAX_SHELL_COMMANDS; i++) {
	    writeDlShellCommandEntry(stream,
	      &(dlShellCommand->command[i]),i,level+1);
	}
	fprintf(stream,"\n%s\tclr=%d",indent,dlShellCommand->clr);
	fprintf(stream,"\n%s\tbclr=%d",indent,dlShellCommand->bclr);
	fprintf(stream,"\n%s}",indent);
    }
#endif
}

static void shellCommandCb(Widget w, XtPointer client_data,
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
	    parseAndExecCommand(displayInfo,command);
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
    int nargs;
    XmString title;
    Widget prompt;

    title = XmStringCreateLocalized("Command");
    nargs = 0;
    XtSetArg(args[nargs],XmNdialogTitle,title); nargs++;
    XtSetArg(args[nargs],XmNdialogStyle,XmDIALOG_FULL_APPLICATION_MODAL); nargs++;
    XtSetArg(args[nargs],XmNselectionLabelString,title); nargs++;
    XtSetArg(args[nargs],XmNautoUnmanage,False); nargs++;
  /* update global for selection box widget access */
    prompt = XmCreatePromptDialog(parent,
      "shellCommandPromptD",args,nargs);
    XmStringFree(title);

    XtAddCallback(prompt, XmNcancelCallback,shellCommandCb,parent);
    XtAddCallback(prompt,XmNokCallback,shellCommandCb,parent);
    XtAddCallback(prompt,XmNhelpCallback,shellCommandCb,parent);
    return (prompt);
}

void dmExecuteShellCommand(Widget w, DlShellCommandEntry *commandEntry,
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
      RD_LABEL_RC,   &(dlShellCommand->label),
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
		  updateElementFromGlobalResourceBundle(
		    dlElement->structure.element);
		dlElement = dlElement->next;
	    }
	}
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
    int i, j, nargs;
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

    nargs = 0;
    XtSetArg(args[nargs],XmNautoUnmanage,False); nargs++;
    XtSetArg(args[nargs],XmNmarginHeight,8); nargs++;
    XtSetArg(args[nargs],XmNmarginWidth,8); nargs++;
    cmdForm = XmCreateFormDialog(parent,"shellCommandDataF",args,nargs);
    shell = XtParent(cmdForm);
    nargs = 0;
    XtSetArg(args[nargs],XmNtitle,"Shell Command Data"); nargs++;
#if OMIT_RESIZE_HANDLES
    XtSetArg(args[nargs],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); nargs++;
  /* KE: The following is necessary for Exceed, which turns off the
     resize function with the handles.  It should not be necessary */
    XtSetArg(args[nargs],XmNmwmFunctions, MWM_FUNC_ALL); nargs++;
#endif
    XtSetValues(shell,args,nargs);
    XmAddWMProtocolCallback(shell,WM_DELETE_WINDOW,
      shellCommandActivate,(XtPointer)CMD_CLOSE_BTN);
    nargs = 0;
    XtSetArg(args[nargs],XmNrows,MAX_SHELL_COMMANDS); nargs++;
    XtSetArg(args[nargs],XmNcolumns,3); nargs++;
    XtSetArg(args[nargs],XmNcolumnMaxLengths,cmdColumnMaxLengths); nargs++;
    XtSetArg(args[nargs],XmNcolumnWidths,cmdColumnWidths); nargs++;
    XtSetArg(args[nargs],XmNcolumnLabels,cmdColumnLabels); nargs++;
    XtSetArg(args[nargs],XmNcolumnMaxLengths,cmdColumnMaxLengths); nargs++;
    XtSetArg(args[nargs],XmNcolumnWidths,cmdColumnWidths); nargs++;
    XtSetArg(args[nargs],XmNcolumnLabelAlignments,cmdColumnLabelAlignments); nargs++;
    XtSetArg(args[nargs],XmNboldLabels,False); nargs++;
    cmdMatrix = XtCreateManagedWidget("cmdMatrix",
      xbaeMatrixWidgetClass,cmdForm,args,nargs);


    xmString = XmStringCreateLocalized("Cancel");
    nargs = 0;
    XtSetArg(args[nargs],XmNlabelString,xmString); nargs++;
    closeButton = XmCreatePushButton(cmdForm,"closeButton",args,nargs);
    XtAddCallback(closeButton,XmNactivateCallback,
      shellCommandActivate,(XtPointer)CMD_CLOSE_BTN);
    XtManageChild(closeButton);
    XmStringFree(xmString);

    xmString = XmStringCreateLocalized("Apply");
    nargs = 0;
    XtSetArg(args[nargs],XmNlabelString,xmString); nargs++;
    applyButton = XmCreatePushButton(cmdForm,"applyButton",args,nargs);
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
    nargs = 0;
    XtSetArg(args[nargs],XmNtopAttachment,XmATTACH_FORM); nargs++;
    XtSetArg(args[nargs],XmNleftAttachment,XmATTACH_FORM); nargs++;
    XtSetArg(args[nargs],XmNrightAttachment,XmATTACH_FORM); nargs++;
    XtSetValues(cmdMatrix,args,nargs);
  /* apply */
    nargs = 0;
    XtSetArg(args[nargs],XmNtopAttachment,XmATTACH_WIDGET); nargs++;
    XtSetArg(args[nargs],XmNtopWidget,cmdMatrix); nargs++;
    XtSetArg(args[nargs],XmNtopOffset,12); nargs++;
    XtSetArg(args[nargs],XmNleftAttachment,XmATTACH_POSITION); nargs++;
    XtSetArg(args[nargs],XmNleftPosition,30); nargs++;
    XtSetArg(args[nargs],XmNbottomAttachment,XmATTACH_FORM); nargs++;
    XtSetArg(args[nargs],XmNbottomOffset,12); nargs++;
    XtSetValues(applyButton,args,nargs);
  /* close */
    nargs = 0;
    XtSetArg(args[nargs],XmNtopAttachment,XmATTACH_WIDGET); nargs++;
    XtSetArg(args[nargs],XmNtopWidget,cmdMatrix); nargs++;
    XtSetArg(args[nargs],XmNtopOffset,12); nargs++;
    XtSetArg(args[nargs],XmNrightAttachment,XmATTACH_POSITION); nargs++;
    XtSetArg(args[nargs],XmNrightPosition,70); nargs++;
    XtSetArg(args[nargs],XmNbottomAttachment,XmATTACH_FORM); nargs++;
    XtSetArg(args[nargs],XmNbottomOffset,12); nargs++;
    XtSetValues(closeButton,args,nargs);

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
