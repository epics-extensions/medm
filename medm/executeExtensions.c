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


/***************************************************************************
 ****                        executeExtensions.c                        ****
 ***************************************************************************/


#include "medm.h"

#include "xgif.h"

#include <X11/keysym.h>



void executeDlImage(DisplayInfo *displayInfo, DlImage *dlImage, Boolean dummy)
{
  Arg args[10];
  int i, n;
  GIFData *gif;


  displayInfo->useDynamicAttribute = FALSE;

/* from the DlImage structure, we've got the image's dimensions */
  switch (dlImage->imageType) {
    case GIF_IMAGE:
	if (dlImage->privateData == NULL) {
	   if (!initializeGIF(displayInfo,dlImage)) {
	   /* something failed in there - bail out! */
	      if (dlImage->privateData != NULL) {
		free((char *)dlImage->privateData);
		dlImage->privateData = NULL;
	      }
	   }
	} else {
	   gif = (GIFData *) dlImage->privateData;
	   if (gif != NULL) {
	     if (dlImage->object.width == gif->currentWidth &&
	       dlImage->object.height == gif->currentHeight) {
	       drawGIF(displayInfo,dlImage);
	     } else {
	       resizeGIF(displayInfo,dlImage);
	       drawGIF(displayInfo,dlImage);
	     }
	   }
	}
	break;
  }


}


void executeDlComposite(DisplayInfo *displayInfo, DlComposite *dlComposite,
	Boolean forcedDisplayToWindow)
{
  DlElement *element;

  if (displayInfo->traversalMode == DL_EDIT) {

/* like dmTraverseDisplayList: traverse composite's display list */
    element = ((DlElement *)dlComposite->dlElementListHead)->next;
    while (element != NULL) {
/* type in union is unimportant: just trying to get to element structure */
/* third argument is for statics acting as dynamics (for forced display) */
        (*element->dmExecute)(displayInfo,element->structure.file,FALSE);
        element = element->next;
    }



  } else if (displayInfo->traversalMode == DL_EXECUTE) {

    if (dlComposite->visible) {

/* like dmTraverseDisplayList: traverse composite's display list */
      element = ((DlElement *)dlComposite->dlElementListHead)->next;
      while (element != NULL) {
/* type in union is unimportant: just trying to get to element structure */
/* third argument is for statics acting as dynamics (for forced display) */
        (*element->dmExecute)(displayInfo,element->structure.file,
			forcedDisplayToWindow);
        element = element->next;
      }


    }

    if (!dlComposite->monitorAlreadyAdded) {

      dlComposite->visible = True;	/* run-time visibility initally True */

   /* setup monitor if channel is specified */

      if (dlComposite->chan[0] != '\0' && dlComposite->vis != V_STATIC) {


/*
	Channel *channelAccessMonitorData =
                        allocateChannel(displayInfo);
	channelAccessMonitorData->monitorType = DL_Composite;
	channelAccessMonitorData->specifics = (XtPointer) dlComposite;
	channelAccessMonitorData->dlAttr = NULL;
	SEVCHK(CA_BUILD_AND_CONNECT(dlComposite->chan,TYPENOTCONN,0,
          &(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
          NULL),
          "executeDlComposite: error in CA_BUILD_AND_CONNECT");
	if (channelAccessMonitorData->chid != NULL)
	    ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;
*/
	dlComposite->monitorAlreadyAdded = True;

      }
    }
  }

}
