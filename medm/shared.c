/*
 *	Mark Anderson, Argonne National Laboratory:
 *		U.S. DOE, University of Chicago
 */


#include "medm.h"



XtCallbackProc wmCloseCallback(
  Widget w,
  ShellType shellType,
  XmAnyCallbackStruct *call_data)
{
  DisplayInfo *newDisplayInfo;
  Position X,Y;
/*
 * handle WM Close functions like all the separate dialog close functions,
 *   dispatch based upon widget value that the callback is called with
 */
   switch (shellType) {
	case DISPLAY_SHELL:
	/* it's a display shell */
	    newDisplayInfo = dmGetDisplayInfoFromWidget(w);
	    if (newDisplayInfo == currentDisplayInfo) {
	      highlightAndSetSelectedElements(NULL,0,0);
	      clearResourcePaletteEntries();
	    }
	    currentDisplayInfo = newDisplayInfo;
	    if (currentDisplayInfo->hasBeenEditedButNotSaved) {
		XtManageChild(closeQD);
	    } else {
	    /* remove currentDisplayInfo from displayInfoList and cleanup */
		dmRemoveDisplayInfo(currentDisplayInfo);
		currentDisplayInfo = NULL;
	    }
	    break;

	case OTHER_SHELL:
	/* it's one of the permanent shells */
	    if (w == mainShell) {
		XtManageChild(exitQD);
	    } else if (w == objectS) {
		XtPopdown(objectS);
	    } else if (w == resourceS) {
		XtPopdown(resourceS);
	    } else if (w == colorS) {
		XtPopdown(colorS);
	    } else if (w == channelS) {
		XtPopdown(channelS);
	    } else if (w == helpS) {
		XtPopdown(helpS);
	    }
	    break;
   }
}




/*
 * optionMenuSet:  routine to set option menu to specified button index
 *		(0 - (# buttons - 1))
 */
void optionMenuSet(Widget menu, int buttonId)
{
  WidgetList buttons;
  Cardinal numButtons;
  Widget subMenu;

/* (MDA) - if option menus are ever created using non pushbutton or
 *	pushbutton widgets in them (e.g., separators) then this routine must
 *	loop over all children and make sure to only reference the push
 *	button derived children
 *
 *	Note: for invalid buttons, don't do anything (this can occur
 *	for example, when setting dynamic attributes when they don't
 *	really apply (and this is usually okay because they are not
 *	managed in invalid cases anyway))
 */
  XtVaGetValues(menu,XmNsubMenuId,&subMenu,NULL);
  if (subMenu != NULL) {
    XtVaGetValues(subMenu,XmNchildren,&buttons,XmNnumChildren,&numButtons,NULL);
    if (buttonId < numButtons && buttonId >= 0) {
      XtVaSetValues(menu,XmNmenuHistory,buttons[buttonId],NULL);
    }
  } else {
    fprintf(stderr,"\noptionMenuSet: no subMenu found for option menu");
  }
}
