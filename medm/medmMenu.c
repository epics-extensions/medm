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
#include <X11/IntrinsicP.h>

typedef struct _Menu {
    DlElement   *dlElement;
    Record      *record; 
    UpdateTask  *updateTask;
    Pixel       color;
} Menu;

void menuCreateRunTimeInstance(DisplayInfo *, DlElement *);
void menuCreateEditInstance(DisplayInfo *, DlElement *);

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

static DlDispatchTable menuDlDispatchTable = {
    createDlMenu,
    NULL,
    executeDlMenu,
    writeDlMenu,
    NULL,
    menuGetValues,
    menuInheritValues,
    menuSetBackgroundColor,
    menuSetForegroundColor,
    genericMove,
    genericScale,
    NULL,
    NULL};


int menuFontListIndex(int height)
{
    int i;
/* don't allow height of font to exceed 90% - 4 pixels of menu widget
 *	(includes nominal 2*shadowThickness=2 shadow)
 */
    for (i = MAX_FONTS-1; i >=  0; i--) {
	if ( ((int)(.90*height) - 4) >= 
	  (fontTable[i]->ascent + fontTable[i]->descent))
	  return(i);
    }
    return (0);
}

void menuCreateRunTimeInstance(DisplayInfo *displayInfo,DlElement *dlElement) {
    Menu *pm;
    DlMenu *dlMenu = dlElement->structure.menu;
    pm = (Menu *) malloc(sizeof(Menu));
    pm->dlElement = dlElement;
    pm->updateTask = updateTaskAddTask(displayInfo,
      &(dlMenu->object),
      menuDraw,
      (XtPointer)pm);

    if (pm->updateTask == NULL) {
	medmPrintf("\nmenuCreateRunTimeInstance: Memory allocation error\n");
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
    return;
}

void menuCreateEditInstance(DisplayInfo *displayInfo, DlElement *dlElement) {
    Arg args[15];
    XmString buttons[1];
    XmButtonType buttonType[1];
    int i, n;
    Widget localWidget, optionButtonGadget, menu;
    WidgetList children;
    Cardinal numChildren;
    XmFontList fontList;
    Dimension useableWidth;
    DlMenu *dlMenu = dlElement->structure.menu;

    buttons[0] = XmStringCreateSimple("menu");
    buttonType[0] = XmPUSHBUTTON;
  /* from the menu structure, we've got Menu's specifics */
  /*
   * take a guess here  - keep this constant the same is in medmCA.c
   *	this takes out the extra space needed for the cascade pixmap, etc
   */
#define OPTION_MENU_SUBTRACTIVE_WIDTH 23
    if (dlMenu->object.width > OPTION_MENU_SUBTRACTIVE_WIDTH)
      useableWidth = (Dimension) (dlMenu->object.width
	- OPTION_MENU_SUBTRACTIVE_WIDTH);
    else
      useableWidth = (Dimension) dlMenu->object.width;
    n = 0;
    XtSetArg(args[n],XmNx,(Position)dlMenu->object.x); n++;
    XtSetArg(args[n],XmNy,(Position)dlMenu->object.y); n++;
    XtSetArg(args[n],XmNwidth,(Dimension)useableWidth); n++;
    XtSetArg(args[n],XmNheight,(Dimension)dlMenu->object.height); n++;
    XtSetArg(args[n],XmNforeground,(Pixel)
      displayInfo->colormap[dlMenu->control.clr]); n++;
      XtSetArg(args[n],XmNbackground,(Pixel)
	displayInfo->colormap[dlMenu->control.bclr]); n++;
	XtSetArg(args[n],XmNbuttonCount,1); n++;
	XtSetArg(args[n],XmNbuttonType,buttonType); n++;
	XtSetArg(args[n],XmNbuttons,buttons); n++;
	XtSetArg(args[n],XmNresizeWidth,FALSE); n++;
	XtSetArg(args[n],XmNresizeHeight,FALSE); n++;
	XtSetArg(args[n],XmNrecomputeSize,FALSE); n++;
	XtSetArg(args[n],XmNmarginWidth,0); n++;
	XtSetArg(args[n],XmNmarginHeight,0); n++;
	fontList = fontListTable[menuFontListIndex(dlMenu->object.height)];
	XtSetArg(args[n],XmNfontList,fontList); n++;
	localWidget = XmCreateSimpleOptionMenu(displayInfo->drawingArea,
	  "menu",args,n);

      /* resize children */
	XtVaGetValues(localWidget,XmNsubMenuId,&menu,NULL);
	XtVaGetValues(menu,XmNnumChildren,&numChildren,XmNchildren,&children,NULL);
	for (i = 0; i < numChildren; i++) {
	    XtUninstallTranslations(children[i]);
	    XtVaSetValues(children[i], XmNfontList, fontList,
	      XmNwidth, (Dimension) useableWidth,
	      XmNheight, (Dimension)dlMenu->object.height,
	      NULL);
	    XtResizeWidget(children[i],useableWidth,dlMenu->object.height,0);
	}
	optionButtonGadget = XmOptionButtonGadget(localWidget);
	XtVaSetValues(optionButtonGadget, XmNfontList, fontList,
	  XmNwidth, (Dimension)useableWidth,
	  XmNheight, (Dimension)dlMenu->object.height,
	  NULL);
	XtResizeWidget(optionButtonGadget,useableWidth,
	  dlMenu->object.height,0);

	XmStringFree(buttons[0]);

      /* Unmanage the label in the option menu */
	XtUnmanageChild(XmOptionLabelGadget(localWidget));
	dlElement->widget = localWidget;

      /* Remove all translations if in edit mode */
	XtUninstallTranslations(localWidget);
      /* Add back the KeyPress handler invoked in executeDlDisplay */
	XtAddEventHandler(localWidget,KeyPressMask,False,
	  handleKeyPress,(XtPointer)displayInfo);
      /* Add the ButtonPress handler */
	XtAddEventHandler(localWidget,ButtonPressMask,False,
	  handleButtonPress,(XtPointer)displayInfo);

	XtManageChild(localWidget);
}

void executeDlMenu(DisplayInfo *displayInfo, DlElement *dlElement)
{
    switch (displayInfo->traversalMode) {
    case DL_EXECUTE :
	menuCreateRunTimeInstance(displayInfo,dlElement);
	break;
    case DL_EDIT :
	if (dlElement->widget) {
	    XtDestroyWidget(dlElement->widget);
	    dlElement->widget = NULL;
	}
	menuCreateEditInstance(displayInfo,dlElement);
	break;
    default :
	break;
    }
}

void menuUpdateGraphicalInfoCb(XtPointer cd) {
    Record *pd = (Record *) cd;
    Menu *pm = (Menu *) pd->clientData;
    DlElement *dlElement = pm->dlElement;
    DlMenu *dlMenu = pm->dlElement->structure.menu;
    XmFontList fontList = fontListTable[menuFontListIndex(dlMenu->object.height)];
    int i,n;
    Arg args[20];
    Widget buttons[db_state_dim], menu;

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! This is a temperory work around !!!!! */
  /* !!!!! for the reconnection.           !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
    medmRecordAddGraphicalInfoCb(pm->record,NULL);

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! End work around                 !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  /* buildMenu function needs an extra entry to work */

    if (pd->dataType != DBF_ENUM) {
	medmPostMsg("menuUpdateGraphicalInfoCb:\n"
	  "  Cannot create Choice Button for %s\n",
	  "  Is not an ENUM type\n",
	  dlMenu->control.ctrl);
	return;
    }

    n = 0;
    if (dlMenu->object.width > OPTION_MENU_SUBTRACTIVE_WIDTH) {
	XtSetArg(args[0],XmNwidth,dlMenu->object.width - OPTION_MENU_SUBTRACTIVE_WIDTH); n++;
    } else {
	XtSetArg(args[0],XmNwidth,dlMenu->object.width); n++;
    }
    XtSetArg(args[1],XmNheight,dlMenu->object.height); n++;
    XtSetArg(args[2],XmNforeground,
      ((dlMenu->clrmod == ALARM)?
	alarmColorPixel[pd->severity] :
	pm->updateTask->displayInfo->colormap[dlMenu->control.clr])); n++;
	XtSetArg(args[3],XmNbackground,
	  pm->updateTask->displayInfo->colormap[dlMenu->control.bclr]); n++;
	  XtSetArg(args[4],XmNrecomputeSize,False); n++;
	  XtSetArg(args[5],XmNfontList, fontList); n++;
	  XtSetArg(args[6],XmNuserData, pm), n++;
	  XtSetArg(args[7],XmNtearOffModel, XmTEAR_OFF_DISABLED); n++;
	  XtSetArg(args[8],XmNentryAlignment, XmALIGNMENT_CENTER); n++;
	  XtSetArg(args[9],XmNisAligned,True); n++;
	  menu = XmCreatePulldownMenu(pm->updateTask->displayInfo->drawingArea,"menu",args,n);
	  XtSetArg(args[7],XmNalignment,XmALIGNMENT_CENTER);
	  for (i=0; i<=pd->hopr; i++) {
	      XmString xmStr;
	      xmStr = XmStringCreateSimple(pd->stateStrings[i]);
	      XtSetArg(args[8], XmNlabelString, xmStr);
	      buttons[i] = XmCreatePushButtonGadget(menu, "menuButtons", args, 9);
	      XtAddCallback(buttons[i], XmNactivateCallback, menuValueChangedCb, (XtPointer) i);
	      XmStringFree(xmStr);
	  }
	  XtManageChildren(buttons,i);
	  n = 7;
	  XtSetArg(args[n],XmNx, dlMenu->object.x); n++;
	  XtSetArg(args[n],XmNy, dlMenu->object.y); n++;
	  XtSetArg(args[n],XmNmarginWidth, 0); n++;
	  XtSetArg(args[n],XmNmarginHeight, 0); n++;
	  XtSetArg(args[n],XmNsubMenuId, menu); n++;
	  XtSetArg(args[n],XmNtearOffModel, XmTEAR_OFF_DISABLED); n++;
	  pm->dlElement->widget =
	    XmCreateOptionMenu(pm->updateTask->displayInfo->drawingArea,
	      "optionMenu",args,n);
		
	/* unmanage the option label gadget, manage the option menu */
	  XtUnmanageChild(XmOptionLabelGadget(pm->dlElement->widget));
	  XtManageChild(pm->dlElement->widget);

	/* add in drag/drop translations */
	  XtOverrideTranslations(pm->dlElement->widget,parsedTranslations);
	  updateTaskMarkUpdate(pm->updateTask);

}

static void menuUpdateValueCb(XtPointer cd) {
    Menu *pm = (Menu *) ((Record *) cd)->clientData;
    updateTaskMarkUpdate(pm->updateTask);
}

static void menuDraw(XtPointer cd) {
    Menu *pm = (Menu *) cd;
    Record *pd = pm->record;
    Widget widget = pm->dlElement->widget;
    DlMenu *dlMenu = pm->dlElement->structure.menu;
    if (pd->connected) {
	if (pd->readAccess) {
	    if ((widget) && !XtIsManaged(widget))
	      XtManageChild(widget);
 
	    if (pd->precision < 0) return;

	    if (pd->dataType == DBF_ENUM) {
		Widget menuWidget;
		WidgetList children;
		Cardinal numChildren;
		int i;

		XtVaGetValues(widget,XmNsubMenuId,&menuWidget,NULL);
		XtVaGetValues(menuWidget,
		  XmNchildren,&children,
		  XmNnumChildren,&numChildren,
		  NULL);
		i = (int) pd->value;
		if ((i >=0) && (i < (int) numChildren)) {
		    XtVaSetValues(widget,XmNmenuHistory,children[i],NULL);
		} else {
		    medmPostMsg("menuUpdateValueCb: Invalid menuHistory child\n");
		    return;
		}
		switch (dlMenu->clrmod) {
		case STATIC :
		case DISCRETE :
		    break;
		case ALARM :
		    XtVaSetValues(widget,XmNforeground,alarmColorPixel[pd->severity],NULL);
		    XtVaSetValues(menuWidget,XmNforeground,alarmColorPixel[pd->severity],NULL);
		    break;
		default :
		    medmPostMsg("menuUpdateValueCb:\n");
		    medmPrintf("  Channel Name: %s\n",dlMenu->control.ctrl);
		    medmPrintf("  Message: Unknown color modifier\n");
		    return;
		}
	    } else {
		medmPostMsg("menuUpdateValueCb:\n");
		medmPrintf("  Channel Name: %s\n",dlMenu->control.ctrl);
		medmPrintf("  Message: Data type must be enum\n");
		return;
	    }
	    if (pd->writeAccess)
	      XDefineCursor(XtDisplay(widget),XtWindow(widget),rubberbandCursor);
	    else
	      XDefineCursor(XtDisplay(widget),XtWindow(widget),noWriteAccessCursor);
	} else {
	    if (widget) XtUnmanageChild(widget);
	    draw3DPane(pm->updateTask,pm->color);
	    draw3DQuestionMark(pm->updateTask);
	}
    } else {
	if ((widget) && XtIsManaged(widget))
	  XtUnmanageChild(widget);
	drawWhiteRectangle(pm->updateTask);
    }
}

static void menuDestroyCb(XtPointer cd) {
    Menu *pm = (Menu *) cd;
    if (pm) {
	medmDestroyRecord(pm->record); 
	free((char *)pm);
    }
}

static void menuValueChangedCb(
  Widget  w,
  XtPointer clientData,
  XtPointer callbackStruct)
{
    Arg args[3];
    Menu *pm;
    Record *pd;
    int btnNumber = (int) clientData;
    XmPushButtonCallbackStruct *call_data = (XmPushButtonCallbackStruct *) callbackStruct;

/*
 * only do ca_put if this widget actually initiated the channel change
 */
    if (call_data->event != NULL && call_data->reason == XmCR_ACTIVATE) {

      /* button's parent (menuPane) has the displayInfo pointer */
	XtVaGetValues(w,XmNuserData,&pm,NULL);
	pd = pm->record;

	if (pd->connected) {
	    if (pd->writeAccess) {
		medmSendDouble(pm->record,(double)btnNumber);
	    } else {
		fputc('\a',stderr);
		menuUpdateValueCb((XtPointer)pm->record);
	    } 
	} else {
	    medmPrintf("\nmenuValueChangedCb: %s not connected\n",
	      pm->dlElement->structure.menu->control.ctrl);
	}
    }
}

static void menuGetRecord(XtPointer cd, Record **record, int *count) {
    Menu *pm = (Menu *) cd;
    *count = 1;
    record[0] = pm->record;
}

DlElement *createDlMenu(DlElement *p)
{
    DlMenu *dlMenu;
    DlElement *dlElement;
 
    dlMenu = (DlMenu *) malloc(sizeof(DlMenu));
    if (!dlMenu) return 0;
    if (p) {
	*dlMenu = *(p->structure.menu);
    } else {
	objectAttributeInit(&(dlMenu->object));
	controlAttributeInit(&(dlMenu->control));
	dlMenu->clrmod = STATIC;
    }
    if (!(dlElement = createDlElement(DL_Menu,
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
 
    if (!dlElement) return 0;
    dlMenu = dlElement->structure.menu;
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlMenu->object));
	    else if (!strcmp(token,"control"))
	      parseControl(displayInfo,&(dlMenu->control));
	    else if (!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"static"))
		  dlMenu->clrmod = STATIC;
		else if (!strcmp(token,"alarm"))
		  dlMenu->clrmod = ALARM;
		else if (!strcmp(token,"discrete"))
		  dlMenu->clrmod = DISCRETE;
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

void writeDlMenu(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
    int i;
    char indent[16];
    DlMenu *dlMenu = dlElement->structure.menu;
 
    for (i = 0; i < level; i++) indent[i] = '\t';
    indent[i] = '\0';
 
    fprintf(stream,"\n%smenu {",indent);
    writeDlObject(stream,&(dlMenu->object),level+1);
    writeDlControl(stream,&(dlMenu->control),level+1);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
	if (dlMenu->clrmod != STATIC) 
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
