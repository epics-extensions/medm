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
 *
 *****************************************************************************
*/

#include "medm.h"
void menuCreateRunTimeInstance(DisplayInfo *displayInfo,DlMenu *dlChoiceButton);
void menuCreateEditInstance(DisplayInfo *displayInfo,DlMenu *dlChoiceButton);

static void menuUpdateValueCb(Channel *pCh);
static void menuUpdateGraphicalInfoCb(Channel *pCh);
static void menuDestroyCb(Channel *pCh);
static void menuValueChangedCb(Widget, XtPointer, XtPointer);

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
  Channel *pCh;
  Display *display = XtDisplay(displayInfo->drawingArea);

  pCh = allocateChannel(displayInfo);

  pCh->monitorType = DL_Menu;
  pCh->specifics = (XtPointer) dlMenu;
  pCh->clrmod = dlMenu->clrmod;
  pCh->backgroundColor = displayInfo->dlColormap[dlMenu->control.bclr];
  pCh->updateList = NULL;

  /* setup the callback rountine */
  pCh->updateChannelCb = menuUpdateValueCb;
  pCh->updateGraphicalInfoCb = menuUpdateGraphicalInfoCb;
  pCh->destroyChannel = menuDestroyCb;


  /* put up white rectangle so that unconnected channels are obvious */

  XSetForeground(display,displayInfo->pixmapGC,WhitePixel(display,DefaultScreen(display)));
  XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
	dlMenu->object.x,dlMenu->object.y,
	dlMenu->object.width,dlMenu->object.height);
  XSetForeground(display,displayInfo->gc,WhitePixel(display,DefaultScreen(display)));
  XFillRectangle(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	dlMenu->object.x,dlMenu->object.y,
	dlMenu->object.width,dlMenu->object.height);

  SEVCHK(CA_BUILD_AND_CONNECT(dlMenu->control.ctrl,TYPENOTCONN,0,
	&(pCh->chid),NULL,medmConnectEventCb,pCh),
	"executeDlMenu: error in CA_BUILD_AND_CONNECT for Monitor");
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

void menuUpdateGraphicalInfoCb(Channel *pCh) {
  DlMenu *dlMenu = (DlMenu *) pCh->specifics;
  XmFontList fontList = fontListTable[menuFontListIndex(dlMenu->object.height)];
  int i,n;
  Arg args[20];
  Widget widget, buttons[db_state_dim], menu;

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! This is a temperory work around !!!!! */
  /* !!!!! for the reconnection.           !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  pCh->updateGraphicalInfoCb = NULL;

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! End work around                 !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  /* buildMenu function needs an extra entry to work */

  if (ca_field_type(pCh->chid) != DBF_ENUM) {
    medmPrintf("menuUpdateGraphicalInfoCb :\n    %s\n    \"%s\" %s\n\n",
                "Cannot create Choice Button,",
                ca_name(pCh->chid),"is not an ENUM type!");
    medmPostTime();
    return;
  }

  if (dlMenu->object.width > OPTION_MENU_SUBTRACTIVE_WIDTH)
    XtSetArg(args[0],XmNwidth,dlMenu->object.width - OPTION_MENU_SUBTRACTIVE_WIDTH);
  else
    XtSetArg(args[0],XmNwidth,dlMenu->object.width);
  XtSetArg(args[1],XmNheight,dlMenu->object.height);
  XtSetArg(args[2],XmNforeground,
	    ((pCh->clrmod == ALARM)?
	      alarmColorPixel[pCh->info->e.severity] :
	      pCh->displayInfo->dlColormap[dlMenu->control.clr]));
  XtSetArg(args[3],XmNbackground, pCh->displayInfo->dlColormap[dlMenu->control.bclr]);
  XtSetArg(args[4],XmNrecomputeSize,False);
  XtSetArg(args[5],XmNfontList, fontList);
  XtSetArg(args[6],XmNuserData, pCh),
  XtSetArg(args[7],XmNtearOffModel, XmTEAR_OFF_DISABLED);
  XtSetArg(args[8],XmNentryAlignment, XmALIGNMENT_CENTER);
  XtSetArg(args[9],XmNisAligned,True);
  menu = XmCreatePulldownMenu(pCh->displayInfo->drawingArea,"menu",args,10);
  XtSetArg(args[7],XmNalignment,XmALIGNMENT_CENTER);
  for (i=0; i<pCh->info->e.no_str; i++) {
    XmString xmStr;
    xmStr = XmStringCreateSimple(pCh->info->e.strs[i]);
    XtSetArg(args[8], XmNlabelString, xmStr);
    buttons[i] = XmCreatePushButtonGadget(menu, "menuButtons", args, 9);
    XtAddCallback(buttons[i], XmNactivateCallback, menuValueChangedCb, (XtPointer) i);
    XmStringFree(xmStr);
  }
  XtManageChildren(buttons,pCh->info->e.no_str);
  n = 7;
  XtSetArg(args[n],XmNx, dlMenu->object.x); n++;
  XtSetArg(args[n],XmNy, dlMenu->object.y); n++;
  XtSetArg(args[n],XmNmarginWidth, 0); n++;
  XtSetArg(args[n],XmNmarginHeight, 0); n++;
  XtSetArg(args[n],XmNsubMenuId, menu); n++;
  XtSetArg(args[n],XmNtearOffModel, XmTEAR_OFF_DISABLED); n++;
  pCh->self = XmCreateOptionMenu(pCh->displayInfo->drawingArea,
		  "optionMenu",args,n);
  pCh->displayInfo->child[pCh->displayInfo->childCount++]
	      = pCh->self;
		
  /* unmanage the option label gadget, manage the option menu */
  XtUnmanageChild(XmOptionLabelGadget(pCh->self));
  XtManageChild(pCh->self);

  /* add in drag/drop translations */
  XtOverrideTranslations(pCh->self,parsedTranslations);
  menuUpdateValueCb(pCh);

}

static void menuUpdateValueCb(Channel *pCh) {
  if (ca_state(pCh->chid) == cs_conn) {
    if (ca_read_access(pCh->chid)) {
      if (pCh->self)
        XtManageChild(pCh->self);
      else
        return;

      if (ca_field_type(pCh->chid) == DBF_ENUM) {
        Widget menuWidget;
        WidgetList children;
        Cardinal numChildren;
        int i;

        XtVaGetValues(pCh->self,XmNsubMenuId,&menuWidget,NULL);
        XtVaGetValues(menuWidget,
		XmNchildren,&children,
		XmNnumChildren,&numChildren,
		NULL);
        i = (int) pCh->value;
        if ((i >=0) && (i < (int) numChildren)) {
          XtVaSetValues(pCh->self,XmNmenuHistory,children[i],NULL);
        } else {
          medmPrintf("menuUpdateValueCb: invalid menuHistory child\n");
          medmPostTime();
          return;
        }
        switch (pCh->clrmod) {
          case STATIC :
          case DISCRETE :
            break;
          case ALARM :
            XtVaSetValues(pCh->self,XmNforeground,alarmColorPixel[pCh->severity],NULL);
            XtVaSetValues(menuWidget,XmNforeground,alarmColorPixel[pCh->severity],NULL);
            break;
          default :
            medmPrintf("Message: Unknown color modifier!\n");
            medmPrintf("Channel Name : %s\n",ca_name(pCh->chid));
            medmPostMsg("Error: menuUpdateValueCb\n");
            return;
        }
      } else {
        medmPrintf("Message: Data type must be enum!\n");
        medmPrintf("Channel Name : %s\n",ca_name(pCh->chid));
        medmPostMsg("Error: menuUpdateValueCb\n");
        return;
      }
      if (ca_write_access(pCh->chid))
        XDefineCursor(XtDisplay(pCh->self),XtWindow(pCh->self),rubberbandCursor);
      else
        XDefineCursor(XtDisplay(pCh->self),XtWindow(pCh->self),noWriteAccessCursor);
    } else {
      if (pCh->self) XtUnmanageChild(pCh->self);
      draw3DPane(pCh);
      draw3DQuestionMark(pCh);
    }
  } else {
    if (pCh->self) XtUnmanageChild(pCh->self);
    drawWhiteRectangle(pCh);
  }
}

static void menuDestroyCb(Channel *pCh) {
  return;
}

static void menuValueChangedCb(
  Widget  w,
  XtPointer clientData,
  XtPointer callbackStruct)
{
  Arg args[3];
  Channel *pCh;
  short btnNumber = (short) clientData;
  XmPushButtonCallbackStruct *call_data = (XmPushButtonCallbackStruct *) callbackStruct;

/*
 * only do ca_put if this widget actually initiated the channel change
 */
  if (call_data->event != NULL && call_data->reason == XmCR_ACTIVATE) {

    /* button's parent (menuPane) has the displayInfo pointer */
    XtVaGetValues(w,XmNuserData,&pCh,NULL);

    if (pCh == NULL) return;

    if (ca_state(pCh->chid) == cs_conn) {
      if (ca_write_access(pCh->chid)) {
        SEVCHK(ca_put(DBR_SHORT,pCh->chid,&(btnNumber)),
          "menuValueChangedCb : error in ca_put");
        ca_flush_io();
      } else {
        fputc('\a',stderr);
        menuUpdateValueCb(pCh);
      } 
    } else {
      medmPrintf("menuValueChangedCb : %s not connected",
                ca_name(pCh->chid));
    }
  }
}

