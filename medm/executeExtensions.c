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


	ChannelAccessMonitorData *channelAccessMonitorData =
                        allocateChannelAccessMonitorData(displayInfo);
	channelAccessMonitorData->monitorType = DL_Composite;
	channelAccessMonitorData->specifics = (XtPointer) dlComposite;
	channelAccessMonitorData->dlAttr = NULL;
	channelAccessMonitorData->dlDyn  = NULL;

	SEVCHK(CA_BUILD_AND_CONNECT(dlComposite->chan,TYPENOTCONN,0,
          &(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
          NULL),
          "executeDlComposite: error in CA_BUILD_AND_CONNECT");
	if (channelAccessMonitorData->chid != NULL)
	    ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;

	dlComposite->monitorAlreadyAdded = True;

      }
    }
  }

}





void executeDlPolyline(DisplayInfo *displayInfo, DlPolyline *dlPolyline,
				Boolean forcedDisplayToWindow)
{
  Drawable drawable;


  if (displayInfo->useDynamicAttribute == FALSE ||
			displayInfo->traversalMode == DL_EDIT) {

/* from the dlPolyline structure, we've got polyline's dimensions */
  /* always render to window */
    drawable = XtWindow(displayInfo->drawingArea);
    XDrawLines(display,drawable,displayInfo->gc,
	  dlPolyline->points,dlPolyline->nPoints,CoordModeOrigin);

    if (!forcedDisplayToWindow) {
  /* also render to pixmap if not part of the dynamics stuff and not
   * member of composite */
      drawable = displayInfo->drawingAreaPixmap;
      XDrawLines(display,drawable,displayInfo->gc,
	  dlPolyline->points,dlPolyline->nPoints,CoordModeOrigin);
    }

  } else {

/* Dynamic Attribute to be used: setup monitor */

    if (displayInfo->traversalMode == DL_EXECUTE) {

      DlAttribute *dlAttr = (DlAttribute *) malloc(sizeof(DlAttribute));
      DlDynamicAttribute *dlDyn = (DlDynamicAttribute *)
	malloc(sizeof(DlDynamicAttribute));
      ChannelAccessMonitorData *channelAccessMonitorData = 
			allocateChannelAccessMonitorData(displayInfo);
      *dlAttr = displayInfo->attribute;
      *dlDyn = displayInfo->dynamicAttribute;
      channelAccessMonitorData->monitorType = DL_Polyline;
      channelAccessMonitorData->specifics = (XtPointer) dlPolyline;
      channelAccessMonitorData->dlAttr = dlAttr;
      channelAccessMonitorData->dlDyn  = dlDyn;

      SEVCHK(CA_BUILD_AND_CONNECT(dlDyn->attr.param.chan,TYPENOTCONN,0,
        &(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
        NULL),
        "executeDlPolyline: error in CA_BUILD_AND_CONNECT");
      if (channelAccessMonitorData->chid != NULL)
	    ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;
    }
  }

  displayInfo->useDynamicAttribute = FALSE;
}




void executeDlPolygon(DisplayInfo *displayInfo, DlPolygon *dlPolygon,
				Boolean forcedDisplayToWindow)
{
  Drawable drawable;
  FillStyle fillStyle;
  unsigned int lineWidth;

  fillStyle = displayInfo->attribute.fill;
  lineWidth = displayInfo->attribute.width;


  if (displayInfo->useDynamicAttribute == FALSE ||
			displayInfo->traversalMode == DL_EDIT) {

/* from the dlPolygon structure, we've got polygon's dimensions */
  /* always render to window */
    drawable = XtWindow(displayInfo->drawingArea);
    if (fillStyle == F_SOLID)
      XFillPolygon(display,drawable,displayInfo->gc,
	  dlPolygon->points,dlPolygon->nPoints,Complex,CoordModeOrigin);
    else if (fillStyle == F_OUTLINE)
      XDrawLines(display,drawable,displayInfo->gc,
	  dlPolygon->points,dlPolygon->nPoints,CoordModeOrigin);

    if (!forcedDisplayToWindow) {
  /* also render to pixmap if not part of the dynamics stuff and not
   * member of composite */
      drawable = displayInfo->drawingAreaPixmap;
      if (fillStyle == F_SOLID)
        XFillPolygon(display,drawable,displayInfo->gc,
	  dlPolygon->points,dlPolygon->nPoints,Complex,CoordModeOrigin);
      else if (fillStyle == F_OUTLINE)
        XDrawLines(display,drawable,displayInfo->gc,
	  dlPolygon->points,dlPolygon->nPoints,CoordModeOrigin);
    }

  } else {

/* Dynamic Attribute to be used: setup monitor */

    if (displayInfo->traversalMode == DL_EXECUTE) {

      DlAttribute *dlAttr = (DlAttribute *) malloc(sizeof(DlAttribute));
      DlDynamicAttribute *dlDyn = (DlDynamicAttribute *)
	malloc(sizeof(DlDynamicAttribute));
      ChannelAccessMonitorData *channelAccessMonitorData = 
			allocateChannelAccessMonitorData(displayInfo);
      *dlAttr = displayInfo->attribute;
      *dlDyn = displayInfo->dynamicAttribute;
      channelAccessMonitorData->monitorType = DL_Polygon;
      channelAccessMonitorData->specifics = (XtPointer) dlPolygon;
      channelAccessMonitorData->dlAttr = dlAttr;
      channelAccessMonitorData->dlDyn  = dlDyn;

      SEVCHK(CA_BUILD_AND_CONNECT(dlDyn->attr.param.chan,TYPENOTCONN,0,
        &(channelAccessMonitorData->chid),NULL,processMonitorConnectionEvent,
        NULL),
        "executeDlPolygon: error in CA_BUILD_AND_CONNECT");
      if (channelAccessMonitorData->chid != NULL)
	    ca_puser(channelAccessMonitorData->chid) = channelAccessMonitorData;
    }
  }

  displayInfo->useDynamicAttribute = FALSE;
}
