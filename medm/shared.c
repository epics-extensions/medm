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

Boolean medmSaveDisplay(DisplayInfo *displayInfo, char *filename, Boolean overwrite);

void wmCloseCallback(
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
        XmString warningXmstring;
        char warningString[2*MAX_FILE_CHARS];
        char *tmp, *tmp1;

        strcpy(warningString,"Save before closing display :\n");
        tmp = tmp1 = dmGetDisplayFileName(currentDisplayInfo);
        while (*tmp != '\0')
          if (*tmp++ == '/') tmp1 = tmp;
        strcat(warningString,tmp1);
        dmSetAndPopupQuestionDialog(currentDisplayInfo,warningString,"Yes","No","Cancel");
        switch (currentDisplayInfo->questionDialogAnswer) {
          case 1 :
            /* Yes, save display */
	    if (medmSaveDisplay(currentDisplayInfo,
		    dmGetDisplayFileName(currentDisplayInfo),True) == False) return;
            break;
	  case 2 :
            /* No, return */
	    break;
	  case 3 :
	    /* Don't close display */
	    return;
          default :
	    return;
        }
      }
      /* remove currentDisplayInfo from displayInfoList and cleanup */
      dmRemoveDisplayInfo(currentDisplayInfo);
      currentDisplayInfo = NULL;

      break;

    case OTHER_SHELL:
      /* it's one of the permanent shells */
      if (w == mainShell) {
	medmExit();
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
