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
 * .01  03-01-95        vong    2.0.0 release
 * .02  09-05-95        vong    2.1.0 release
 *                              - using new screen update dispatch mechanism
 *
 *****************************************************************************
*/

#include "medm.h"

typedef struct _Menu {
  Widget      widget;
  DlMenu      *dlMenu; 
  Record      *record; 
  UpdateTask  *updateTask;
  Pixel       color;
} Menu;

void menuCreateRunTimeInstance(DisplayInfo *displayInfo,DlMenu *dlChoiceButton);
void menuCreateEditInstance(DisplayInfo *displayInfo,DlMenu *dlChoiceButton);

static void menuDraw(XtPointer);
static void menuUpdateValueCb(XtPointer);
static void menuUpdateGraphicalInfoCb(XtPointer);
static void menuDestroyCb(XtPointer cd);
static void menuValueChangedCb(Widget, XtPointer, XtPointer);
static void menuName(XtPointer, char **, short *, int *);

int menuFontListIndex(int height)
{
  int i, index;
/* don't allow height of font to exceed 90% - 4 pixels of messageButton widget
 *	(includes nominal 2*shadowThickness=2 shadow)
 */
  for (i = MAX_FONTS-1; i >=  0; i--) {
    if ( ((int)(.90*height) - 4) >= 
			(fontTable[i]->ascent + fontTable[i]->descent))
	return(i);
  }
  return (0);
}

void menuCreateRunTimeInstance(DisplayInfo *displayInfo,DlMenu *dlMenu) {
  Menu *pm;
  pm = (Menu *) malloc(sizeof(Menu));
  pm->dlMenu = dlMenu;
  pm->updateTask = updateTaskAddTask(displayInfo,
                                     &(dlMenu->object),
                                     menuDraw,
                                     (XtPointer)pm);

  if (pm->updateTask == NULL) {
    medmPrintf("menuCreateRunTimeInstance : memory allocation error\n");
  } else {
    updateTaskAddDestroyCb(pm->updateTask,menuDestroyCb);
    updateTaskAddNameCb(pm->updateTask,menuName);
  }
  pm->record = medmAllocateRecord(dlMenu->control.ctrl,
                  menuUpdateValueCb,
                  menuUpdateGraphicalInfoCb,
                  (XtPointer) pm);
  drawWhiteRectangle(pm->updateTask);
  pm->color = displayInfo->dlColormap[dlMenu->control.bclr];
  pm->widget = NULL;
  return;
}

void menuCreateEditInstance(DisplayInfo *displayInfo, DlMenu *dlMenu) {
  Arg args[15];
  XmString buttons[1];
  XmButtonType buttonType[1];
  int i, n;
  Widget localWidget, optionButtonGadget, menu;
  WidgetList children;
  Cardinal numChildren;
  XmFontList fontList;
  Dimension useableWidth;

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
                displayInfo->dlColormap[dlMenu->control.clr]); n++;
  XtSetArg(args[n],XmNbackground,(Pixel)
                displayInfo->dlColormap[dlMenu->control.bclr]); n++;
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
  /* unmanage the label in the option menu */
  XtUnmanageChild(XmOptionLabelGadget(localWidget));
  XtManageChild(localWidget);
  displayInfo->child[displayInfo->childCount++] =  localWidget;

  /* remove all translations if in edit mode */
  XtUninstallTranslations(localWidget);

  /* add button press handler */
  XtAddEventHandler(localWidget, ButtonPressMask, False,
      handleButtonPress,(XtPointer)displayInfo);
}

void executeDlMenu(DisplayInfo *displayInfo, DlMenu *dlMenu, Boolean dummy)
{

  displayInfo->useDynamicAttribute = FALSE;
 
  switch (displayInfo->traversalMode) {
  case DL_EXECUTE :
    menuCreateRunTimeInstance(displayInfo,dlMenu);
    break;
  case DL_EDIT :
    menuCreateEditInstance(displayInfo,dlMenu);
    break;
  default :
    break;
  }
}

void menuUpdateGraphicalInfoCb(XtPointer cd) {
  Record *pd = (Record *) cd;
  Menu *pm = (Menu *) pd->clientData;
  DlMenu *dlMenu = pm->dlMenu;
  XmFontList fontList = fontListTable[menuFontListIndex(dlMenu->object.height)];
  int i,n;
  Arg args[20];
  Widget widget, buttons[db_state_dim], menu;

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
    medmPrintf("menuUpdateGraphicalInfoCb :\n    %s\n    \"%s\" %s\n\n",
                "Cannot create Choice Button,",
                dlMenu->control.ctrl,"is not an ENUM type!");
    medmPostTime();
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
	      pm->updateTask->displayInfo->dlColormap[dlMenu->control.clr])); n++;
  XtSetArg(args[3],XmNbackground,
              pm->updateTask->displayInfo->dlColormap[dlMenu->control.bclr]); n++;
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
  pm->widget = XmCreateOptionMenu(pm->updateTask->displayInfo->drawingArea,
		  "optionMenu",args,n);
  pm->updateTask->displayInfo->child[pm->updateTask->displayInfo->childCount++]
	      = pm->widget;
		
  /* unmanage the option label gadget, manage the option menu */
  XtUnmanageChild(XmOptionLabelGadget(pm->widget));
  XtManageChild(pm->widget);

  /* add in drag/drop translations */
  XtOverrideTranslations(pm->widget,parsedTranslations);
  updateTaskMarkUpdate(pm->updateTask);

}

static void menuUpdateValueCb(XtPointer cd) {
  Menu *pm = (Menu *) ((Record *) cd)->clientData;
  updateTaskMarkUpdate(pm->updateTask);
}

static void menuDraw(XtPointer cd) {
  Menu *pm = (Menu *) cd;
  Record *pd = pm->record;
  if (pd->connected) {
    if (pd->readAccess) {
      if ((pm->widget) && !XtIsManaged(pm->widget))
        XtManageChild(pm->widget);
 
      if (pd->precision < 0) return;

      if (pd->dataType == DBF_ENUM) {
        Widget menuWidget;
        WidgetList children;
        Cardinal numChildren;
        int i;

        XtVaGetValues(pm->widget,XmNsubMenuId,&menuWidget,NULL);
        XtVaGetValues(menuWidget,
		XmNchildren,&children,
		XmNnumChildren,&numChildren,
		NULL);
        i = (int) pd->value;
        if ((i >=0) && (i < (int) numChildren)) {
          XtVaSetValues(pm->widget,XmNmenuHistory,children[i],NULL);
        } else {
          medmPrintf("menuUpdateValueCb: invalid menuHistory child\n");
          medmPostTime();
          return;
        }
        switch (pm->dlMenu->clrmod) {
          case STATIC :
          case DISCRETE :
            break;
          case ALARM :
            XtVaSetValues(pm->widget,XmNforeground,alarmColorPixel[pd->severity],NULL);
            XtVaSetValues(menuWidget,XmNforeground,alarmColorPixel[pd->severity],NULL);
            break;
          default :
            medmPrintf("Message: Unknown color modifier!\n");
            medmPrintf("Channel Name : %s\n",pm->dlMenu->control.ctrl);
            medmPostMsg("Error: menuUpdateValueCb\n");
            return;
        }
      } else {
        medmPrintf("Message: Data type must be enum!\n");
        medmPrintf("Channel Name : %s\n",pm->dlMenu->control.ctrl);
        medmPostMsg("Error: menuUpdateValueCb\n");
        return;
      }
      if (pd->writeAccess)
        XDefineCursor(XtDisplay(pm->widget),XtWindow(pm->widget),rubberbandCursor);
      else
        XDefineCursor(XtDisplay(pm->widget),XtWindow(pm->widget),noWriteAccessCursor);
    } else {
      if (pm->widget) XtUnmanageChild(pm->widget);
      draw3DPane(pm->updateTask,pm->color);
      draw3DQuestionMark(pm->updateTask);
    }
  } else {
    if ((pm->widget) && XtIsManaged(pm->widget))
      XtUnmanageChild(pm->widget);
    drawWhiteRectangle(pm->updateTask);
  }
}

static void menuDestroyCb(XtPointer cd) {
  Menu *pm = (Menu *) cd;
  if (pm) {
    medmDestroyRecord(pm->record); 
    free(pm);
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
  short btnNumber = (short) clientData;
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
      medmPrintf("menuValueChangedCb : %s not connected",
                pm->dlMenu->control.ctrl);
    }
  }
}

static void menuName(XtPointer cd, char **name, short *severity, int *count) {
  Menu *pm = (Menu *) cd;
  *count = 1;
  name[0] = pm->record->name;
  severity[0] = pm->record->severity;
}

