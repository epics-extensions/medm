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

#define DEBUG_MENU 0
#define DEBUG_FONT 0

#define OPTION_MENU_ADJUST_WIDTH 22
#define OPTION_MENU_ADJUST_HEIGHT 4
#define SETSIZE 1

#include "medm.h"
#include <X11/IntrinsicP.h>

typedef struct _MedmMenu {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           *record;
    Pixel            color;
} MedmMenu;

static void menuCreateRunTimeInstance(DisplayInfo *, DlElement *);
static void menuCreateEditInstance(DisplayInfo *, DlElement *);
static Widget createMenu(DisplayInfo *displayInfo, Record *pr, DlMenu *dlMenu,
  XmStringTable labels, int nbuttons);

static void menuDraw(XtPointer);
static void menuUpdateValueCb(XtPointer);
static void menuUpdateGraphicalInfoCb(XtPointer);
static void menuDestroyCb(XtPointer cd);
static void menuValueChangedCb(Widget, XtPointer, XtPointer);
static void menuGetRecord(XtPointer, Record **, int *);
static void menuInheritValues(ResourceBundle *pRCB, DlElement *p);
static void menuSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void menuSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void menuGetValues(ResourceBundle *pRCB, DlElement *p);

#if DEBUG_MENU
static void menuDump(Widget w);
static void handleMenuButtonPress(Widget w, XtPointer cd, XEvent *event,
  Boolean *ctd);
#endif

static DlDispatchTable menuDlDispatchTable = {
    createDlMenu,
    NULL,
    executeDlMenu,
    hideDlMenu,
    writeDlMenu,
    NULL,
    menuGetValues,
    menuInheritValues,
    menuSetBackgroundColor,
    menuSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};


int menuFontListIndex(int height)
{
    int i;

#if DEBUG_FONT
    {
	int j, h, h1, h2, indx1, indx2;

	print("\n h   h1  i1    h2  i2\n");
	for(j=0; j < 80; j++) {
	    h = j + 1;
	    h1 = (int)(.90*h) - 4;
	    indx1 = 0;
	    for(i = MAX_FONTS-1; i >= 0; i--) {
		if( ((int)(.90*h) - 4) >=
		  (fontTable[i]->ascent + fontTable[i]->descent)) {
		    indx1 = i;
		    break;
		}
	    }
	    h2 = (int)(h) - 8;
	    indx2 = 0;
	    for(i = MAX_FONTS-1; i >= 0; i--) {
		if( ((int)(h) - 8) >=
		  (fontTable[i]->ascent + fontTable[i]->descent)) {
		    indx2 = i;
		    break;
		}
	    }
	    print("%2d   %2d  %2d    %2d  %2d\n",
	      h, h1, indx1, h2, indx2);
	}
    }
#endif

#if 0
  /* Don't allow height of font to exceed 90% - 4 pixels of menu
   *   widget.  Includes nominal 2*shadowThickness = 4 */
    for(i = MAX_FONTS-1; i >= 0; i--) {
	if( ((int)(.90*height) - 4) >=
	  (fontTable[i]->ascent + fontTable[i]->descent))
	  return(i);
    }
#else
  /* Allow for shadowThickness + marginHeight 2*(2+2)=8.  Allow full
   *   ascent + descent.  Gives better spacing of the menu items for
   *   small Menu's */
    for(i = MAX_FONTS-1; i >= 0; i--) {
	if( ((int)(height) - 8) >=
	  (fontTable[i]->ascent + fontTable[i]->descent))
	  return(i);
    }
#endif
    return (0);
}

void executeDlMenu(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    switch (displayInfo->traversalMode) {
    case DL_EXECUTE:
	menuCreateRunTimeInstance(displayInfo,dlElement);
	break;
    case DL_EDIT:
	if(dlElement->widget) {
	    XtDestroyWidget(dlElement->widget);
	    dlElement->widget = NULL;
	}
	menuCreateEditInstance(displayInfo,dlElement);
	break;
    default:
	break;
    }
}

void hideDlMenu(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element with a widget */
    hideWidgetElement(displayInfo, dlElement);
}

static void menuCreateRunTimeInstance(DisplayInfo *displayInfo,
  DlElement *dlElement)
{
    MedmMenu *pm;
    DlMenu *dlMenu = dlElement->structure.menu;

    if(dlElement->data) {
	pm = (MedmMenu *)dlElement->data;
    } else {
	pm = (MedmMenu *)malloc(sizeof(MedmMenu));
	dlElement->data = (void *)pm;
	if(pm == NULL) {
	    medmPrintf(1,"\nmenuCreateRunTimeInstance:"
	      " Memory allocation error\n");
	    return;
	}
      /* Pre-initialize */
	pm->updateTask = NULL;
	pm->record = NULL;
	pm->dlElement = dlElement;

	pm->updateTask = updateTaskAddTask(displayInfo,
	  &(dlMenu->object),
	  menuDraw,
	  (XtPointer)pm);

	if(pm->updateTask == NULL) {
	    medmPrintf(1,"\nmenuCreateRunTimeInstance: Memory allocation error\n");
	} else {
	    updateTaskAddDestroyCb(pm->updateTask,menuDestroyCb);
	    updateTaskAddNameCb(pm->updateTask,menuGetRecord);
	}
	pm->record = medmAllocateRecord(dlMenu->control.ctrl,
	  menuUpdateValueCb,
	  menuUpdateGraphicalInfoCb,
	  (XtPointer) pm);
	drawWhiteRectangle(pm->updateTask);
	pm->color = displayInfo->colormap[dlMenu->control.bclr];
    }
}

static void menuCreateEditInstance(DisplayInfo *displayInfo,
  DlElement *dlElement)
{
    Widget localWidget;
    XmString labels[1];
    DlMenu *dlMenu = dlElement->structure.menu;

    labels[0] = XmStringCreateLocalized("Menu");
    localWidget = createMenu(displayInfo, NULL, dlMenu, labels, 1);
    dlElement->widget = localWidget;
    XmStringFree(labels[0]);

  /* Add handlers */
    addCommonHandlers(localWidget, displayInfo);

    XtManageChild(localWidget);
}

static void menuUpdateGraphicalInfoCb(XtPointer cd)
{
    Record *pr = (Record *)cd;
    MedmMenu *pm = (MedmMenu *)pr->clientData;
    DlMenu *dlMenu = pm->dlElement->structure.menu;
    XmStringTable labels;
    int i, nbuttons;

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! This is a temporary work around !!!!! */
  /* !!!!! for the reconnection.           !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  /* KE: This causes it to not do anything for the reconnection */
    medmRecordAddGraphicalInfoCb(pm->record, NULL);

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! End work around                 !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

    if(pr->dataType != DBF_ENUM) {
	medmPostMsg(1,"menuUpdateGraphicalInfoCb:\n"
	  "  Cannot create Menu for %s\n"
	  "  Is not an ENUM type\n",
	  dlMenu->control.ctrl);
	return;
    }
    if(pr->hopr <= 0.0) {
	medmPostMsg(1,"menuUpdateGraphicalInfoCb:\n"
	  "  Cannot create Menu for %s\n"
	  "  There are no states to assign to menu items\n",
	  dlMenu->control.ctrl);
	return;
    }

  /* Allocate the menu string table and fill it
   *   XmStringTable is a typedef for(XmString *) */
    labels = NULL;
    nbuttons = 0;
    nbuttons = (int)(pr->hopr + 1.5);
    labels = (XmStringTable)calloc(nbuttons, sizeof(XmString));
    if(!labels) {
	nbuttons = 0;
	medmPrintf(1,"\nmenuUpdateGraphicalInfoCb: Memory allocation error\n");
    }
    for(i=0; i < nbuttons; i++) {
	labels[i] = XmStringCreateLocalized(pr->stateStrings[i]);
    }

  /* Create the option menu */
    pm->dlElement->widget = createMenu(pm->updateTask->displayInfo, pr, dlMenu,
      labels, nbuttons);

  /* Free the XmStrings */
    for(i=0; i < nbuttons; i++) XmStringFree(labels[i]);
    if(labels) free((char *)labels);

  /* Add handlers and manage will be done in menuDraw */
}

/* This routine creates the Option Menu for either EDIT or EXEC
 *   Only the menu items, colors, and callbacks are different
 * This is hard because the option menu is designed to set its own size based
 *   on the items in the menu.  We have to circumvent this behavior.
 * If SETSIZE is not True, the menu will size itself as it wishes and
 *   always look attractive, but not have exactly the same sizes as
 *   specified in the object.   SETSIZE is left in to show what needs to be
 *   changed */
static Widget createMenu(DisplayInfo *displayInfo, Record *pr, DlMenu *dlMenu,
  XmStringTable labels, int nbuttons)
{
    Widget w, menu, pushbutton, tearOff;
    MedmMenu *pm;
    Arg args[25];
    int i, nargs, nargs0;
    Widget optionButtonGadget;
    XmFontList fontList;
    Dimension useableWidth, useableHeight;
    Pixel foreground, background;

  /* Determine the font based on the height */
    fontList = fontListTable[menuFontListIndex(dlMenu->object.height)];

  /* Set the foreground and background colors depending on mode */
    if(globalDisplayListTraversalMode == DL_EXECUTE) {
	pm = (MedmMenu *)pr->clientData;
	foreground = (dlMenu->clrmod == ALARM)?
	  alarmColor(pr->severity) :
	  pm->updateTask->displayInfo->colormap[dlMenu->control.clr];
	background = pm->updateTask->displayInfo->colormap[dlMenu->control.bclr];
    } else {
	pm = NULL;
	foreground = displayInfo->colormap[dlMenu->control.clr];
	background = displayInfo->colormap[dlMenu->control.bclr];
    }

#if SETSIZE
    {
	int intnum;

      /* Adjust for the width of the cascade button glyph, which Motif
       *   adds to our width */
	intnum = (int)dlMenu->object.width - OPTION_MENU_ADJUST_WIDTH;
	if(intnum > 0) useableWidth = intnum;
	else useableWidth = 1;
      /* Adjust for the shadow height that Motif adds to the height of the
       *   items (push button gadgets) in the menu */
	intnum = (int)dlMenu->object.height - OPTION_MENU_ADJUST_HEIGHT;
	if(intnum > 0) useableHeight = intnum;
	else useableHeight = 1;
    }
#else
    useableWidth = (int)dlMenu->object.width;
    useableHeight = (int)dlMenu->object.height;
#endif

  /* Create the pulldown menu */
    nargs = 0;
    XtSetArg(args[nargs], XmNforeground, foreground); nargs++;
    XtSetArg(args[nargs], XmNbackground, background); nargs++;
#if 0
    XtSetArg(args[nargs], XmNtearOffModel, XmTEAR_OFF_DISABLED); nargs++;
#endif
    menu = XmCreatePulldownMenu(displayInfo->drawingArea, "menuPulldownMenu", args, nargs);
  /* Make the tear off colors right */
    tearOff = XmGetTearOffControl(menu);
    if(tearOff) {
	XtVaSetValues(tearOff,
	  XmNforeground,(Pixel)displayInfo->colormap[dlMenu->control.clr],
	  XmNbackground,(Pixel)displayInfo->colormap[dlMenu->control.bclr],
	  NULL);
    }

  /* Add the push button gadget children */
    nargs = 0;
    XtSetArg(args[nargs], XmNmarginWidth, 0); nargs++;
    XtSetArg(args[nargs], XmNmarginHeight ,0); nargs++;
    XtSetArg(args[nargs], XmNfontList,fontList); nargs++;
    XtSetArg(args[nargs], XmNuserData, (XtPointer)pm); nargs++;
#if SETSIZE
  /* The option menu will not allow resizing that doesn't allow space on
   *   the cascade button gadget for the largest menu item, so make them
   *   all be the useable size */
    XtSetArg(args[nargs], XmNrecomputeSize, FALSE); nargs++;
    XtSetArg(args[nargs], XmNwidth, useableWidth); nargs++;
    XtSetArg(args[nargs], XmNheight, useableHeight); nargs++;
#endif
    nargs0 = nargs;
    for(i = 0; i < nbuttons; i++) {
	nargs = nargs0;
	XtSetArg(args[nargs],XmNlabelString, labels[i]); nargs++;
	pushbutton = XmCreatePushButtonGadget(menu, "menuButton", args, nargs);
	XtManageChild(pushbutton);

      /* Add callback and userData in execute mode */
	if(globalDisplayListTraversalMode == DL_EXECUTE) {
	    XtAddCallback(pushbutton, XmNactivateCallback,
	      menuValueChangedCb, (XtPointer)i);
	}
    }

  /* Create the option menu */
    nargs = 0;
    XtSetArg(args[nargs], XmNx, (Position)dlMenu->object.x); nargs++;
    XtSetArg(args[nargs], XmNy, (Position)dlMenu->object.y); nargs++;
    XtSetArg(args[nargs], XmNforeground, foreground); nargs++;
    XtSetArg(args[nargs], XmNbackground, background); nargs++;
    XtSetArg(args[nargs], XmNmarginWidth, 0); nargs++;
    XtSetArg(args[nargs], XmNmarginHeight ,0); nargs++;
    XtSetArg(args[nargs], XmNtearOffModel, XmTEAR_OFF_DISABLED); nargs++;
    XtSetArg(args[nargs], XmNsubMenuId, menu); nargs++;
    w = XmCreateOptionMenu(displayInfo->drawingArea, "menuOptionMenu", args, nargs);

  /* Unmanage the label gadget child */
    XtUnmanageChild(XmOptionLabelGadget(w));

  /* Adjust the cascade button child
   * Add the things that don't get set when you set them above
   *   and resize the button */
    optionButtonGadget = XmOptionButtonGadget(w);
    nargs = 0;
    XtSetArg(args[nargs], XmNmarginWidth, 0); nargs++;
    XtSetArg(args[nargs], XmNmarginHeight ,0); nargs++;
    XtSetArg(args[nargs], XmNhighlightThickness, 0); nargs++;
    XtSetValues(optionButtonGadget, args, nargs);
#if SETSIZE
    XtResizeWidget(optionButtonGadget,
      dlMenu->object.width, dlMenu->object.height, 0);
#endif

#if EXPLICITLY_OVERWRITE_CDE_COLORS
  /* Color the menu explicitly to avoid CDE interference */
    colorPulldownMenu(w,foreground,background);
#endif

#if DEBUG_MENU
    {
	Dimension height,width,borderWidth;
	Dimension oheight,owidth;
	Widget wbutton;

      /* Add special handler */
	XtAddEventHandler(w, ButtonPressMask, False, handleMenuButtonPress,
	  (XtPointer)w);

	print("\n\014createMenu: mode=%s optionMenu=%x OptionButton=%x\n",
	  globalDisplayListTraversalMode == DL_EXECUTE?"EXECUTE":"EDIT",
	  w,optionButtonGadget);
	if(w) {
	    XtVaGetValues(w,XmNheight,&height,XmNwidth,&width,NULL);
	    wbutton = XmOptionButtonGadget(w);
	    XtVaGetValues(wbutton,XmNheight,&oheight,XmNwidth,&owidth,NULL);
	    i=menuFontListIndex(dlMenu->object.height);
	    print("  w=%d wuse=%d wact=%d,%d h=%d hact=%d,%d indx=%d font=%d %d %d\n",
	      dlMenu->object.width, useableWidth, width, owidth,
	      dlMenu->object.height, height, oheight, i,
	      fontTable[i]->ascent,
	      fontTable[i]->descent,
	      fontTable[i]->ascent + fontTable[i]->descent);
#if 0
	    print("  numChildren=%d\n",numChildren);
	    for(i=0; i < numChildren; i++) {
		XmStringContext context;
		char string[1024], *pstring, *text;
		XmStringCharSet tag;
		XmStringDirection direction;
		Boolean separator;

		if(XmStringInitContext(&context, labels[i])) {
		    pstring=string;
		    while(XmStringGetNextSegment(context,&text,
		      &tag,&direction,&separator)) {
			pstring+=(strlen(strcpy(pstring,text)));
			if(separator) {
			    *pstring++='\n';
			    *pstring='\0';}
			XtFree(text);
		    }
		    XmStringFreeContext(context);
		    print("  Button %2d: %x %s\n",i,children[i],string);
		}
	    }
#endif
#if 0
	    menuDump(w);
#endif
	}
    }

#endif

    return w;
}

static void menuUpdateValueCb(XtPointer cd)
{
    MedmMenu *pm = (MedmMenu *) ((Record *) cd)->clientData;
    updateTaskMarkUpdate(pm->updateTask);
}

static void menuDraw(XtPointer cd)
{
    MedmMenu *pm = (MedmMenu *)cd;
    Record *pr = pm->record;
    DlElement *dlElement = pm->dlElement;
    Widget widget = dlElement->widget;
    DlMenu *dlMenu = dlElement->structure.menu;

#if DEBUG_MENU
    printf("\nmenuDraw: pr->connected=%s widget=%x\n",
      pr->connected?"Yes":"No",widget);
#endif

  /* Check if hidden */
    if(dlElement->hidden) {
	if(widget && XtIsManaged(widget)) {
	    XtUnmanageChild(widget);
	}
	return;
    }

    if(pr && pr->connected) {
	if(pr->readAccess) {
	    if(!widget) return;
	    if(!XtIsManaged(widget)) {
		addCommonHandlers(widget, pm->updateTask->displayInfo);
		XtManageChild(widget);
	    }
	    if(pr->writeAccess)
	      XDefineCursor(XtDisplay(widget),XtWindow(widget),rubberbandCursor);
	    else
	      XDefineCursor(XtDisplay(widget),XtWindow(widget),noWriteAccessCursor);
	    if(pr->precision < 0) return;    /* Wait for pr->value */
	    if(pr->dataType == DBF_ENUM) {
		Widget menuWidget;
		WidgetList children;
		Cardinal numChildren;
		int i;

		XtVaGetValues(widget,XmNsubMenuId,&menuWidget,NULL);
		if(menuWidget) {
		    XtVaGetValues(menuWidget,
		      XmNchildren,&children,
		      XmNnumChildren,&numChildren,
		      NULL);
		    i = (int) pr->value;
		    if((i >=0) && (i < (int) numChildren)) {
			XtVaSetValues(widget,XmNmenuHistory,children[i],NULL);
		    } else {
			medmPostMsg(1,"menuUpdateValueCb: Got state %d.\n"
			  "  Only have strings for %d states (starting at 0).\n"
			  "%s  %s\n",
			  i,(int)numChildren,
			  ((int)numChildren == 16)?
			  "  [Channel Access is limited to 16 strings, "
			  "but there may be more states]\n":"",
			  dlMenu->control.ctrl);
			return;
		    }
		} else {
		    medmPostMsg(1,"menuUpdateValueCb: No subMenuId\n");
		    return;
		}
		switch (dlMenu->clrmod) {
		case STATIC:
		case DISCRETE:
		    break;
		case ALARM:
		    pr->monitorSeverityChanged = True;
		    XtVaSetValues(widget,XmNforeground,alarmColor(pr->severity),NULL);
		    XtVaSetValues(menuWidget,XmNforeground,alarmColor(pr->severity),NULL);
		    break;
		default:
		    medmPostMsg(1,"menuUpdateValueCb:\n");
		    medmPrintf(0,"  Channel Name: %s\n",dlMenu->control.ctrl);
		    medmPrintf(0,"  Message: Unknown color modifier\n");
		    return;
		}
	    } else {
		medmPostMsg(1,"menuUpdateValueCb:\n");
		medmPrintf(0,"  Channel Name: %s\n",dlMenu->control.ctrl);
		medmPrintf(0,"  Message: Data type must be enum\n");
		return;
	    }
	} else {
	    if(widget && XtIsManaged(widget))
	      XtUnmanageChild(widget);
	    drawBlackRectangle(pm->updateTask);
	}
    } else {
	if(widget && XtIsManaged(widget))
	  XtUnmanageChild(widget);
	drawWhiteRectangle(pm->updateTask);
    }
}

static void menuDestroyCb(XtPointer cd)
{
    MedmMenu *pm = (MedmMenu *) cd;
    if(pm) {
	medmDestroyRecord(pm->record);
	if(pm->dlElement) pm->dlElement->data = NULL;
	free((char *)pm);
    }
}

static void menuValueChangedCb(Widget  w, XtPointer clientData,
  XtPointer callbackStruct)
{
    MedmMenu *pm;
    Record *pr;
    int btnNumber = (int) clientData;
    XmPushButtonCallbackStruct *call_data =
      (XmPushButtonCallbackStruct *) callbackStruct;

  /* Only do ca_put if this widget actually initiated the channel change */
    if(call_data->event != NULL && call_data->reason == XmCR_ACTIVATE) {

      /* button's parent (menuPane) has the displayInfo pointer */
	XtVaGetValues(w,XmNuserData,&pm,NULL);
	pr = pm->record;

	if(pr->connected) {
	    if(pr->writeAccess) {
#ifdef MEDM_CDEV
		if(pr->stateStrings)
		  medmSendString(pr, pr->stateStrings[btnNumber]);
#else
		medmSendDouble(pm->record,(double)btnNumber);
#endif
	    } else {
		fputc('\a',stderr);
		menuUpdateValueCb((XtPointer)pm->record);
	    }
	} else {
	    medmPrintf(1,"\nmenuValueChangedCb: %s not connected\n",
	      pm->dlElement->structure.menu->control.ctrl);
	}
    }
}

static void menuGetRecord(XtPointer cd, Record **record, int *count)
{
    MedmMenu *pm = (MedmMenu *) cd;
    *count = 1;
    record[0] = pm->record;
}

DlElement *createDlMenu(DlElement *p)
{
    DlMenu *dlMenu;
    DlElement *dlElement;

    dlMenu = (DlMenu *)malloc(sizeof(DlMenu));
    if(!dlMenu) return 0;
    if(p) {
	*dlMenu = *(p->structure.menu);
    } else {
	objectAttributeInit(&(dlMenu->object));
	controlAttributeInit(&(dlMenu->control));
	dlMenu->clrmod = STATIC;
    }
    if(!(dlElement = createDlElement(DL_Menu,
      (XtPointer)      dlMenu,
      &menuDlDispatchTable))) {
	free(dlMenu);
    }

    return(dlElement);
}

DlElement *parseMenu(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlMenu *dlMenu;
    DlElement *dlElement = createDlMenu(NULL);

    if(!dlElement) return 0;
    dlMenu = dlElement->structure.menu;
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlMenu->object));
	    else if(!strcmp(token,"control"))
	      parseControl(displayInfo,&(dlMenu->control));
	    else if(!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"static"))
		  dlMenu->clrmod = STATIC;
		else if(!strcmp(token,"alarm"))
		  dlMenu->clrmod = ALARM;
		else if(!strcmp(token,"discrete"))
		  dlMenu->clrmod = DISCRETE;
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

void writeDlMenu(FILE *stream, DlElement *dlElement, int level)
{
    int i;
    char indent[16];
    DlMenu *dlMenu = dlElement->structure.menu;

    for(i = 0; i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%smenu {",indent);
    writeDlObject(stream,&(dlMenu->object),level+1);
    writeDlControl(stream,&(dlMenu->control),level+1);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
	if(dlMenu->clrmod != STATIC)
	  fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	    stringValueTable[dlMenu->clrmod]);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
	  stringValueTable[dlMenu->clrmod]);
    }
#endif
    fprintf(stream,"\n%s}",indent);
}

static void menuInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlMenu *dlMenu = p->structure.menu;
    medmGetValues(pRCB,
      CTRL_RC,       &(dlMenu->control.ctrl),
      CLR_RC,        &(dlMenu->control.clr),
      BCLR_RC,       &(dlMenu->control.bclr),
      CLRMOD_RC,     &(dlMenu->clrmod),
      -1);
}

static void menuGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlMenu *dlMenu = p->structure.menu;
    medmGetValues(pRCB,
      X_RC,          &(dlMenu->object.x),
      Y_RC,          &(dlMenu->object.y),
      WIDTH_RC,      &(dlMenu->object.width),
      HEIGHT_RC,     &(dlMenu->object.height),
      CTRL_RC,       &(dlMenu->control.ctrl),
      CLR_RC,        &(dlMenu->control.clr),
      BCLR_RC,       &(dlMenu->control.bclr),
      CLRMOD_RC,     &(dlMenu->clrmod),
      -1);
}

static void menuSetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlMenu *dlMenu = p->structure.menu;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlMenu->control.bclr),
      -1);
}

static void menuSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlMenu *dlMenu = p->structure.menu;
    medmGetValues(pRCB,
      CLR_RC,        &(dlMenu->control.clr),
      -1);
}

#if DEBUG_MENU
/* EXEC: Ctrl-Btn2 will invoke this without invoking other things */
/* Use Shift-Btn to get a menuDump, too */
static void handleMenuButtonPress(Widget w, XtPointer cd, XEvent *event,
  Boolean *ctd)
{
    XButtonEvent *xEvent = (XButtonEvent *)event;
    Widget wrc = (Widget)cd;
    Widget wbutton;
    Dimension height,width;
    Dimension oheight,owidth;

    if(!w) return;

    wbutton = XmOptionButtonGadget(wrc);
    print("\nhandleMenuButtonPress: mode=%s optionMenu=%x OptionButton=%x\n",
      globalDisplayListTraversalMode == DL_EXECUTE?"EXECUTE":"EDIT",
      wrc, wbutton);
    XtVaGetValues(w,XmNheight,&height,XmNwidth,&width,NULL);
    XtVaGetValues(wbutton,XmNheight,&oheight,XmNwidth,&owidth,NULL);
    print("  w%d,%d h=%d,%d\n",
      width, owidth, height, oheight);

    if(xEvent->state & ShiftMask) menuDump(wrc);
}

static void menuDump(Widget w0)
{
    Widget w, menu;
    Dimension borderWidth;
    Dimension height;
    Dimension width;
    Dimension entryBorder;
    Dimension marginHeight;
    Dimension marginWidth;
    Dimension shadowThickness;
    Dimension spacing;
    Dimension highlightThickness;
    Dimension marginTop;
    Dimension marginBottom;
    Dimension marginLeft;
    Dimension marginRight;
    Boolean resizeHeight;
    Boolean resizeWidth;
    Boolean traversalOn;
    Boolean recomputeSize;
    Widget subMenuId;
    XtPointer userData;
    WidgetList children;
    Cardinal numChildren;

    print("\nmenuDump:\n");

  /* OptionMenu */
    w=w0;
    print("OptionMenu (XmRowColumn) %x\n",w);
    if(w) {
	XtVaGetValues(w,
	  XmNborderWidth,&borderWidth,
	  XmNheight,&height,
	  XmNwidth,&width,
	  XmNentryBorder,&entryBorder,
	  XmNmarginHeight,&marginHeight,
	  XmNmarginWidth,&marginWidth,
	  XmNshadowThickness,&shadowThickness,
	  XmNspacing,&spacing,
	  XmNresizeHeight,&resizeHeight,
	  XmNresizeWidth,&resizeWidth,
	  XmNtraversalOn,&traversalOn,
	  XmNsubMenuId,&subMenuId,
	  XmNuserData,&userData,
	  NULL);
	print("  borderWidth=%d\n",borderWidth);
	print("  height=%d\n",height);
	print("  width=%d\n",width);
	print("  entryBorder=%d\n",entryBorder);
	print("  marginHeight=%d\n",marginHeight);
	print("  marginWidth=%d\n",marginWidth);
	print("  shadowThickness=%d\n",shadowThickness);
	print("  spacing=%d\n",spacing);
	print("  resizeHeight=%s\n",resizeHeight?"True":"False");
	print("  resizeWidth=%s\n",resizeWidth?"True":"False");
	print("  traversalOn=%s\n",traversalOn?"True":"False");
	print("  subMenuId=%x\n",subMenuId);
	print("  userData=%x\n",userData);
	menu=subMenuId;
    }

  /* OptionButton */
    w=XmOptionButtonGadget(w0);
    print("OptionButton (XmCascadeButtonGadget) %x\n",w);
    if(w) {
	XtVaGetValues(w,
	  XmNborderWidth,&borderWidth,
	  XmNheight,&height,
	  XmNwidth,&width,
	  XmNmarginTop,&marginTop,
	  XmNmarginBottom,&marginBottom,
	  XmNmarginLeft,&marginLeft,
	  XmNmarginRight,&marginRight,
	  XmNmarginHeight,&marginHeight,
	  XmNmarginWidth,&marginWidth,
	  XmNshadowThickness,&shadowThickness,
	  XmNhighlightThickness,&highlightThickness,
	  XmNtraversalOn,&traversalOn,
	  XmNrecomputeSize,&recomputeSize,
	  XmNuserData,&userData,
	  NULL);
	print("  borderWidth=%d\n",borderWidth);
	print("  height=%d\n",height);
	print("  width=%d\n",width);
	print("  marginTop=%d\n",marginTop);
	print("  marginBottom=%d\n",marginBottom);
	print("  marginLeft=%d\n",marginLeft);
	print("  marginRight=%d\n",marginRight);
	print("  marginHeight=%d\n",marginHeight);
	print("  marginWidth=%d\n",marginWidth);
	print("  shadowThickness=%d\n",shadowThickness);
	print("  highlightThickness=%d\n",highlightThickness);
	print("  traversalOn=%s\n",traversalOn?"True":"False");
	print("  recomputeSize=%s\n",recomputeSize?"True":"False");
	print("  userData=%x\n",userData);
    }

#if 0
  /* popup_menu */
    w=XtParent(menu);
    print("popup_menu (XmMenuShell) %x\n",w);
    if(w) {
	XtVaGetValues(w,
	  XmNborderWidth,&borderWidth,
	  XmNheight,&height,
	  XmNwidth,&width,
	  NULL);
	print("  borderWidth=%d\n",borderWidth);
	print("  height=%d\n",height);
	print("  width=%d\n",width);
    }
#endif

  /* menu */
    w=menu;
    print("menu (XmRowColumn) %x\n",w);
    if(w) {
	XtVaGetValues(w,
	  XmNborderWidth,&borderWidth,
	  XmNheight,&height,
	  XmNwidth,&width,
	  XmNentryBorder,&entryBorder,
	  XmNmarginHeight,&marginHeight,
	  XmNmarginWidth,&marginWidth,
	  XmNshadowThickness,&shadowThickness,
	  XmNspacing,&spacing,
	  XmNresizeHeight,&resizeHeight,
	  XmNresizeWidth,&resizeWidth,
	  XmNtraversalOn,&traversalOn,
	  XmNsubMenuId,&subMenuId,
	  XmNuserData,&userData,
	  NULL);
	print("  borderWidth=%d\n",borderWidth);
	print("  height=%d\n",height);
	print("  width=%d\n",width);
	print("  entryBorder=%d\n",entryBorder);
	print("  marginHeight=%d\n",marginHeight);
	print("  marginWidth=%d\n",marginWidth);
	print("  shadowThickness=%d\n",shadowThickness);
	print("  spacing=%d\n",spacing);
	print("  resizeHeight=%s\n",resizeHeight?"True":"False");
	print("  resizeWidth=%s\n",resizeWidth?"True":"False");
	print("  traversalOn=%s\n",traversalOn?"True":"False");
	print("  subMenuId=%x\n",subMenuId);
	print("  userData=%x\n",userData);
    }

  /* menuButtons */
    XtVaGetValues(menu,
      XmNnumChildren,&numChildren,
      XmNchildren,&children,
      NULL);
    if(numChildren) w=children[0];
    else w=0;
    print("menuButtons (XmPushButtonGadget) %x\n",w);
    if(w) {
	XtVaGetValues(w,
	  XmNborderWidth,&borderWidth,
	  XmNheight,&height,
	  XmNwidth,&width,
	  XmNmarginTop,&marginTop,
	  XmNmarginBottom,&marginBottom,
	  XmNmarginLeft,&marginLeft,
	  XmNmarginRight,&marginRight,
	  XmNmarginHeight,&marginHeight,
	  XmNmarginWidth,&marginWidth,
	  XmNshadowThickness,&shadowThickness,
	  XmNhighlightThickness,&highlightThickness,
	  XmNtraversalOn,&traversalOn,
	  XmNrecomputeSize,&recomputeSize,
	  XmNuserData,&userData,
	  NULL);
	print("  numChildren=%d\n",numChildren);
	print("  borderWidth=%d\n",borderWidth);
	print("  height=%d\n",height);
	print("  width=%d\n",width);
	print("  marginTop=%d\n",marginTop);
	print("  marginBottom=%d\n",marginBottom);
	print("  marginLeft=%d\n",marginLeft);
	print("  marginRight=%d\n",marginRight);
	print("  marginHeight=%d\n",marginHeight);
	print("  marginWidth=%d\n",marginWidth);
	print("  shadowThickness=%d\n",shadowThickness);
	print("  highlightThickness=%d\n",highlightThickness);
	print("  traversalOn=%s\n",traversalOn?"True":"False");
	print("  recomputeSize=%s\n",recomputeSize?"True":"False");
	print("  userData=%x\n",userData);
    }
}

#endif
