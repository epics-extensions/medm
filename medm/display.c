/*
 *	Mark Anderson, Argonne National Laboratory:
 *		U.S. DOE, University of Chicago
 */


#include "medm.h"


/*
 * create and fill in widgets for display
 */

DisplayInfo *createDisplay()
{
  DisplayInfo *displayInfo;
  int i, n;
  Arg args[10];

/* clear currentDisplayInfo - not really one yet */
  currentDisplayInfo = NULL;
  initializeGlobalResourceBundle();

  displayInfo = allocateDisplayInfo((Widget)NULL);

/* general scheme: update  globalResourceBundle, then do creates */
  globalResourceBundle.x = 0;
  globalResourceBundle.y = 0;
  globalResourceBundle.width = DEFAULT_DISPLAY_WIDTH;
  globalResourceBundle.height = DEFAULT_DISPLAY_HEIGHT;
  strcpy(globalResourceBundle.name,DEFAULT_FILE_NAME);
  createDlFile(displayInfo),
  createDlDisplay(displayInfo);
  createDlColormap(displayInfo);

  dmTraverseDisplayList(displayInfo);
  XtPopup(displayInfo->shell,XtGrabNone);

  currentDisplayInfo = displayInfo;

  return(displayInfo);
}




/********************************************
 **************** Callbacks *****************
 ********************************************/

