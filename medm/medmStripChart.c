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

#include <Xm/DrawingAP.h>

#include <X11/keysym.h>

char *stripChartWidgetName = "stripChart";
static void stripChartUpdateValueCb(Channel *pCh);
static void stripChartUpdateGraphicalInfoCb(Channel *pCh);
static void stripChartDestroyCb(Channel *pCh);

static double stripCalculateInterval(int delay, TimeUnits units)
{
  double returnValue;

  switch (units) {
    case MILLISECONDS:
        returnValue = SECS_PER_MILLISEC * delay;
        break;
    case SECONDS:
        returnValue = delay;
        break;
    case MINUTES:
        returnValue = SECS_PER_MIN * delay;
        break;

    default:
        medmPrintf("\nstripCalculateInterval: unknown UNITS specified");
        returnValue = delay;
        break;
  }
  return (returnValue);

}

static double stripGetValue(int channelNumber, StripChartData *stripChartData)
{
  Channel *pCh;

  pCh = (Channel *) stripChartData->monitors[channelNumber];
  return pCh->value;
}

static XtCallbackProc drawShadows(
  XmDrawingAreaWidget w,
  XtPointer client_data,
  XtPointer call_data)
{
  Arg args[8];
  int n;
  Dimension width, height, shadowThickness;
  GC topGC;
  GC bottomGC;

/* trying to force DrawingArea to draw a shadow */
  topGC = ((struct _XmDrawingAreaRec *)w)->manager.top_shadow_GC;
  bottomGC = ((struct _XmDrawingAreaRec *)w)->manager.bottom_shadow_GC;
 
  n = 0;
  XtSetArg(args[n],XmNwidth,&width); n++;
  XtSetArg(args[n],XmNheight,&height); n++;
  XtSetArg(args[n],XmNshadowThickness,&shadowThickness); n++;
  XtGetValues((Widget)w,args,n);
  _XmDrawShadows(XtDisplay(w),XtWindow(w),topGC,bottomGC,0,0,
        width,height,shadowThickness,XmSHADOW_OUT);
}


static XtCallbackProc redisplayFakeStrip(
  Widget w,
  DlStripChart *dlStripChart,
  XtPointer call_data)
{
  if (currentDisplayInfo != NULL && dlStripChart != NULL) {
    XSetLineAttributes(display,currentDisplayInfo->gc,0,
                                LineSolid,CapButt,JoinMiter);
    XSetForeground(display,currentDisplayInfo->gc,
        currentDisplayInfo->dlColormap[dlStripChart->plotcom.bclr]);
    XFillRectangle(display,XtWindow(w),currentDisplayInfo->gc,
        0,0,dlStripChart->object.width,dlStripChart->object.height);
    XSetForeground(display,currentDisplayInfo->gc,
        currentDisplayInfo->dlColormap[dlStripChart->plotcom.clr]);
    XDrawRectangle(display,XtWindow(w),currentDisplayInfo->gc,
        (int)(0.2*dlStripChart->object.width),
        (int)(0.2*dlStripChart->object.height),
        (unsigned int) (0.6*dlStripChart->object.width),
        (unsigned int) (0.6*dlStripChart->object.height));
    XDrawString(display,XtWindow(w),currentDisplayInfo->gc,
        (int)(0.2*dlStripChart->object.width),
        (int)(0.15*dlStripChart->object.height),"Strip Chart", 11);
  }
}



void executeDlStripChart(DisplayInfo *displayInfo, DlStripChart *dlStripChart,
                                Boolean dummy)
{
  Channel *pCh, *monitorData;
  StripChartData *stripChartData;
  Strip *strip;
  StripRange stripRange[1];
  int i, j, n;
  Arg args[15];
  Widget localWidget;


  displayInfo->useDynamicAttribute = FALSE;

  stripChartData = NULL;

  if (displayInfo->traversalMode == DL_EXECUTE) {
    stripChartData = (StripChartData *) calloc(1,sizeof(StripChartData));
    j = 0;
    for (i = 0; i < MAX_PENS; i++) {
      if (dlStripChart->pen[i].chan[0] != '\0') {

       pCh = allocateChannel(displayInfo);
       pCh->monitorType = DL_StripChart;
       pCh->specifics = (XtPointer) dlStripChart;
       pCh->backgroundColor = displayInfo->dlColormap[dlStripChart->plotcom.bclr];

       pCh->updateChannelCb = stripChartUpdateValueCb;
       pCh->updateGraphicalInfoCb = stripChartUpdateGraphicalInfoCb;
       pCh->destroyChannel = stripChartDestroyCb;


       SEVCHK(CA_BUILD_AND_CONNECT(dlStripChart->pen[i].chan,TYPENOTCONN,0,
        &(pCh->chid),NULL,medmConnectEventCb, pCh),
        "executeDlStripChart: error in CA_BUILD_AND_CONNECT");

       /* add pointer to this monitor data structure to the strip chart structure */
       stripChartData->monitors[j] = (XtPointer) pCh;
       j++;

      }
    }
    drawWhiteRectangle(pCh);
  }

/* from the stripChart structure, we've got some of StripChart's specifics */
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlStripChart->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlStripChart->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlStripChart->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlStripChart->object.height); n++;
  XtSetArg(args[n],XmNmarginWidth,0); n++;
  XtSetArg(args[n],XmNmarginHeight,0); n++;
  XtSetArg(args[n],XmNshadowThickness,2); n++;
  XtSetArg(args[n],XmNforeground,(Pixel)
        displayInfo->dlColormap[dlStripChart->plotcom.clr]); n++;
  XtSetArg(args[n],XmNbackground,(Pixel)
        displayInfo->dlColormap[dlStripChart->plotcom.bclr]); n++;
  XtSetArg(args[n],XmNtraversalOn,False); n++;
/*
 * add the pointer to the StripChartData structure as userData to widget
 */
  XtSetArg(args[n], XmNuserData, (XtPointer) stripChartData); n++;

  localWidget = XmCreateDrawingArea(displayInfo->drawingArea,
        stripChartWidgetName,args,n);
  displayInfo->child[displayInfo->childCount++] = localWidget;

/* add expose callback to force shadows drawn (DrawingArea should do this!) */
  XtAddCallback(localWidget,XmNexposeCallback,(XtCallbackProc)drawShadows,NULL);

  if (displayInfo->traversalMode == DL_EXECUTE) {

/* record the number of channels in the strip chart */
    stripChartData->nChannels = j;

/* record the widget that the monitor structures belongs to */
    for (i = 0; i < stripChartData->nChannels; i++) {
      monitorData = (Channel *) stripChartData->monitors[i];
      monitorData->self = localWidget;
    }
/* add in drag/drop translations */
    XtOverrideTranslations(localWidget,parsedTranslations);


  } else if (displayInfo->traversalMode == DL_EDIT) {

    XtManageChild(localWidget);

    XSetForeground(display,displayInfo->gc,
        displayInfo->dlColormap[dlStripChart->plotcom.clr]);
    XDrawRectangle(display,XtWindow(localWidget),displayInfo->gc,
                        (int)(0.2*dlStripChart->object.width),
                        (int)(0.2*dlStripChart->object.height),
                        (unsigned int) (0.6*dlStripChart->object.width),
                        (unsigned int) (0.6*dlStripChart->object.height));
    XDrawString(display,XtWindow(localWidget),displayInfo->gc,
                        (int)(0.1*dlStripChart->object.width),
                        (int)(0.1*dlStripChart->object.height),
                         "Strip Chart...", 14);

    XtAddCallback(localWidget,XmNexposeCallback,
                        (XtCallbackProc)redisplayFakeStrip,dlStripChart);

/* add button press handlers */
    XtAddEventHandler(localWidget,
        ButtonPressMask,False,(XtEventHandler)handleButtonPress,
        (XtPointer)displayInfo);


  }

/* add expose callback to force shadows drawn (DrawingArea should do this!) */
/*   important to add this last therefore shadows always drawn! */
  XtAddCallback(localWidget,XmNexposeCallback,(XtCallbackProc)drawShadows,NULL);

}

static void stripChartUpdateGraphicalInfoCb(Channel *pCh) {
  StripChartData *stripChartData;
  DlStripChart *dlStripChart;
  StripChartList *stripElement;
  StripRange stripRange[MAX_PENS];
  Dimension shadowThickness;
  XRectangle clipRect[1];
  int ii, usedHeight, usedCharWidth, bestSize, preferredHeight;
  double (*stripGetValueArray[MAX_PENS])();
  double stripGetValue();
  int maxElements;
  Channel *monitorData;
  int i;
  Arg widgetArgs[12];

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! This is a temperory work around !!!!! */
  /* !!!!! for the reconnection.           !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  pCh->updateGraphicalInfoCb = NULL;

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! End work around                 !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  XtVaGetValues(pCh->self,XmNuserData,(XtPointer)&stripChartData,NULL);
  /*
   *  check to see if all component channels of strip chart have connected.
   *  if so, proceed with strip chart initialization
   */
  for (i = 0; i < stripChartData->nChannels; i++) {
    if (((Channel *)stripChartData->monitors[i])->info == NULL) {
      return;  /* wait for other channels */
    }
  }
  monitorData = pCh;

  /* initialize strip chart */
  dlStripChart = (DlStripChart *)monitorData->specifics;
  stripChartData->strip = stripInit(display,screenNum,
                XtWindow(monitorData->self));
  /* set a clip rectangle in strip's GC to prevent drawing over shadows */
  XtVaGetValues(pCh->self,XmNshadowThickness,&shadowThickness,NULL);
  clipRect[0].x = shadowThickness;
  clipRect[0].y = shadowThickness;
  clipRect[0].width  = dlStripChart->object.width - 2*shadowThickness;
  clipRect[0].height = dlStripChart->object.height - 2*shadowThickness;
  XSetClipRectangles(display,stripChartData->strip->gc,
                0,0,clipRect,1,YXBanded);

  /* allocate a node on displayInfo's strip list */
  stripElement = (StripChartList *) malloc(sizeof(StripChartList));
  if (monitorData->displayInfo->stripChartListHead == NULL) {
    /* first on list */
    monitorData->displayInfo->stripChartListHead = stripElement;
  } else {
    monitorData->displayInfo->stripChartListTail->next=stripElement;
  }
  stripElement->strip = stripChartData->strip;
  stripElement->next = NULL;
  monitorData->displayInfo->stripChartListTail = stripElement;
  /* get ranges for each channel; get proper lopr/hopr one way or another */
  for (i = 0; i < stripChartData->nChannels; i++) {
    stripRange[i].minVal = MIN(pCh->lopr,pCh->hopr);
    stripRange[i].maxVal = MAX(pCh->hopr,pCh->lopr);

    if (stripRange[i].minVal == stripRange[i].maxVal) {
      stripRange[i].maxVal = stripRange[i].minVal + 1.0;
    }
    stripGetValueArray[i] = stripGetValue;
  }
  /* do own font and color handling */
  stripChartData->strip->setOwnFonts  = TRUE;
  stripChartData->strip->shadowThickness = shadowThickness;

  preferredHeight = GraphX_TitleFontSize(stripChartData->strip);
  bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
                preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
  stripChartData->strip->titleFontStruct = fontTable[bestSize];
  stripChartData->strip->titleFontHeight = usedHeight;
  stripChartData->strip->titleFontWidth = usedCharWidth;

  preferredHeight = GraphX_AxesFontSize(stripChartData->strip);
  bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
      preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
  stripChartData->strip->axesFontStruct = fontTable[bestSize];
  stripChartData->strip->axesFontHeight = usedHeight;
  stripChartData->strip->axesFontWidth = usedCharWidth;

  stripChartData->strip->setOwnColors = TRUE;
  /* all the monitors point to same DlStripChart structure, so pick arbitrarily */
  monitorData = pCh;
  dlStripChart = (DlStripChart *)monitorData->specifics;
  stripChartData->strip->forePixel =
      monitorData->displayInfo->dlColormap[dlStripChart->plotcom.clr];
  stripChartData->strip->backPixel =
      monitorData->displayInfo->dlColormap[dlStripChart->plotcom.bclr];
/* let user data be the stripChartData aux. structure */
  stripChartData->strip->userData = (XtPointer) stripChartData;

  stripSet(stripChartData->strip,stripChartData->nChannels,
      STRIP_NSAMPLES,NULL,stripRange,
      stripCalculateInterval(dlStripChart->delay,dlStripChart->units),
      stripGetValueArray,
      (dlStripChart->plotcom.title == NULL
              ? " " : dlStripChart->plotcom.title),NULL,
      (dlStripChart->plotcom.xlabel == NULL
              ? " " : dlStripChart->plotcom.xlabel),
      (dlStripChart->plotcom.ylabel == NULL
              ? " " : dlStripChart->plotcom.ylabel),
      NULL,NULL,NULL,NULL,StripInternal);
  stripSetAppContext(stripChartData->strip,appContext);
  for (i = 0; i < stripChartData->nChannels; i++)
    stripChartData->strip->dataPixel[i] =
         monitorData->displayInfo->dlColormap[dlStripChart->pen[i].clr];
  stripDraw(stripChartData->strip);
  /* add expose and destroy callbacks */
  XtAddCallback(monitorData->self,XmNexposeCallback,(XtCallbackProc)redisplayStrip,
                (XtPointer)&stripChartData->strip);

  /* add destroy callback */
  /* not freeing monitorData here, rather using monitorData for
   it's information */
  XtAddCallback(pCh->self,XmNdestroyCallback,
                (XtCallbackProc)monitorDestroy, (XtPointer)stripChartData);
}

static void stripChartUpdateValueCb(Channel *pCh) {
  StripChartData *stripChartData;
  Boolean connected = True;
  Boolean readAccess = True;
  int i;

  XtVaGetValues(pCh->self,XmNuserData,(XtPointer)&stripChartData,NULL);

  for (i = 0; i< stripChartData->nChannels; i++) {
    Channel *tmpCh = (Channel *) stripChartData->monitors[i];
    if (tmpCh) {
      if (ca_state(tmpCh->chid) != cs_conn) {
	connected = False;
	break;
      }
      if (!ca_read_access(tmpCh->chid)) {
	readAccess = False;
      }
    }
  }

  if (connected) {
    if (readAccess) {
      if (pCh->self) {
        XtManageChild(pCh->self);
      }
    } else {
      if (pCh->self) {
        XtUnmanageChild(pCh->self);
      draw3DPane(pCh);
      draw3DQuestionMark(pCh);
      }
    }
  } else {
    if (pCh->self)
      XtUnmanageChild(pCh->self);
    drawWhiteRectangle(pCh);
  }
}

static void stripChartDestroyCb(Channel *pCh) {
  return;
}
