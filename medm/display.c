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
 * .02  09-08-95        vong    conform to c++ syntax
 *
 *****************************************************************************
*/

#include "medm.h"

/*
 * create and fill in widgets for display
 */

DisplayInfo *createDisplay()
{
  DisplayInfo *displayInfo;
  DlElement *dlElement;
  Arg args[10];

/* clear currentDisplayInfo - not really one yet */
  currentDisplayInfo = NULL;
  initializeGlobalResourceBundle();

  displayInfo = allocateDisplayInfo();

/* general scheme: update  globalResourceBundle, then do creates */
  globalResourceBundle.x = 0;
  globalResourceBundle.y = 0;
  globalResourceBundle.width = DEFAULT_DISPLAY_WIDTH;
  globalResourceBundle.height = DEFAULT_DISPLAY_HEIGHT;
  strcpy(globalResourceBundle.name,DEFAULT_FILE_NAME);
  dlElement = createDlFile(displayInfo);
  if (dlElement) {
    appendDlElement(&(displayInfo->dlElementListTail),dlElement);
  }
  dlElement = createDlDisplay(displayInfo);
  if (dlElement) {
    DlDisplay *dlDisplay = dlElement->structure.display;
    dlDisplay->object.x = globalResourceBundle.x;
    dlDisplay->object.y = globalResourceBundle.y;
    dlDisplay->object.width = globalResourceBundle.width;
    dlDisplay->object.height = globalResourceBundle.height;
    dlDisplay->clr = globalResourceBundle.clr;
    dlDisplay->bclr = globalResourceBundle.bclr;
    appendDlElement(&(displayInfo->dlElementListTail),dlElement);
  }
  dlElement = createDlColormap(displayInfo);
  if (dlElement) {
    appendDlElement(&(displayInfo->dlElementListTail),dlElement);
    displayInfo->dlColormapElement = dlElement;
  }
  dmTraverseDisplayList(displayInfo);
  XtPopup(displayInfo->shell,XtGrabNone);

  currentDisplayInfo = displayInfo;

  return(displayInfo);
}
