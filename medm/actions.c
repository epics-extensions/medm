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
#include <Xm/MwmUtil.h>

#include "cvtFast.h"

extern char *stripChartWidgetName;

/*
 * action routines (XtActionProc) and some associated callbacks
 */

/* since passing client_data didn't seem to work... */
static char *channelName;


static Boolean DragConvertProc(w,selection,target,typeRtn,valueRtn,
				lengthRtn,formatRtn,max_lengthRtn,
				client_data,
				request_id)
  Widget w;
  Atom *selection, *target, *typeRtn;
  XtPointer *valueRtn;
  unsigned long *lengthRtn;
  int *formatRtn;
  unsigned long *max_lengthRtn;
  XtPointer client_data;
  XtRequestId *request_id;
{
  XmString cString;
  char *cText, *passText;

  if (channelName != NULL) {
    if (*target != COMPOUND_TEXT) return(False);
    cString = XmStringCreateSimple(channelName);
    cText = XmCvtXmStringToCT(cString);
    passText = XtMalloc(strlen(cText)+1);
    memcpy(passText,cText,strlen(cText)+1);
   /* probably need this too */
    XmStringFree(cString);

   /* format the value for return */
    *typeRtn = COMPOUND_TEXT;
    *valueRtn = (XtPointer)passText;
    *lengthRtn = strlen(passText);
    *formatRtn = 8;	/* from example - related to #bits for data elements */
    return(True);
  } else {
/* monitorData not found */
    return(False);
  }

}


/*
 * cleanup after drag/drop
 */
static void dragDropFinish(
  Widget w,
  XtPointer client,
  XtPointer call)
{
  Widget sourceIcon;
  Pixmap pixmap;
  Arg args[2];

/* perform cleanup at conclusion of DND */
  XtSetArg(args[0],XmNsourcePixmapIcon,&sourceIcon);
  XtGetValues(w,args,1);

  XtSetArg(args[0],XmNpixmap,&pixmap);
  XtGetValues(sourceIcon,args,1);

  XFreePixmap(display,pixmap);
  XtDestroyWidget(sourceIcon);
}

static XtCallbackRec dragDropFinishCB[] = {
        {dragDropFinish,NULL},
        {NULL,NULL}
};




void StartDrag(
  Widget w,
  XEvent *event)
{
  Arg args[8];
  Cardinal n;
  Atom exportList[1];
  Widget sourceIcon;
  Channel *mData, *md, *mdArray[MAX(MAX_PENS,MAX_TRACES)][2];
  int dir, asc, desc, maxWidth, maxHeight, maxAsc, maxDesc, maxTextWidth;
  int nominalY, i, j, liveChannels, liveTraces;
  XCharStruct overall;
  unsigned long fg, bg;
  Widget searchWidget;
  XButtonEvent *xbutton;
  XtPointer userData;
  StripChartData *stripChartData;
  CartesianPlotData *cartesianPlotData;
  XGCValues gcValues;
  unsigned long gcValueMask;
  Boolean newTrace, haveXY;
  DisplayInfo *displayInfo;

  static char *channelNames[MAX(MAX_PENS,MAX_TRACES)][2];
  Pixmap sourcePixmap = (Pixmap)NULL;
  static GC gc = NULL;

/* a nice sized font */
#define FONT_TABLE_INDEX 6
/* move the text over... */
#define X_SHIFT 8
#define MARGIN  2

  
/* (MDA) since widget doing drag could be toggleButton or optionMenu button
 *   (which has more than just flat, single parent),
 *   find the widget that has a parent that is the drawing area and
 *   search based on that *   (since that is what is rooted in the display)
 * - NB if drawing areas as children of the main drawing area are allowed
 *   as parents of controllers/monitors, this logic must change...
 */
  searchWidget = w;
  if (XtClass(searchWidget) == xmDrawingAreaWidgetClass
		&& strcmp(XtName(searchWidget),stripChartWidgetName)) {
    /* starting search from a DrawingArea which is not a StripChart 
     *  (i.e., DL_Display) therefore lookup "graphic" (non-widget) elements 
     *  ---get data from position
     */
    displayInfo = dmGetDisplayInfoFromWidget(searchWidget);
    xbutton = (XButtonEvent *)event;
    mData = dmGetChannelFromPosition(displayInfo,
		xbutton->x,xbutton->y);
  } else {
   /* ---get data from widget */
    while (XtClass(XtParent(searchWidget)) != xmDrawingAreaWidgetClass)
	searchWidget = XtParent(searchWidget);
    mData = dmGetChannelFromWidget(searchWidget);
  }


  if (mData != NULL) {

/* always color according to severity (to convey channel name and severity) */
    bg = BlackPixel(display,screenNum);

    switch (mData->monitorType) {

	case DL_StripChart:
		channelName = NULL;
		maxAsc = 0; maxDesc = 0; maxWidth = 0;
		liveChannels = 0;
		for (i = 0; i < MAX_PENS; i++) {
		   channelNames[i][0]= NULL;
		   mdArray[i][0] = NULL;
		}
		if (XtClass(searchWidget) == xmDrawingAreaWidgetClass
		    && strcmp(XtName(searchWidget),stripChartWidgetName)) {
		/* if we got here via parent DA (not StripChart), get widget */
		   searchWidget = mData->self;
		}
		XtVaGetValues(searchWidget,XmNuserData,&userData,NULL);
		stripChartData = (StripChartData *)userData;
		for (i = 0; i < stripChartData->nChannels; i++) {
		  if (stripChartData->monitors[i] != NULL){
		    md = (Channel *)
				stripChartData->monitors[i];
		    mdArray[liveChannels][0] = md;
		    if ( md->chid != NULL) {
		      channelNames[liveChannels][0] = ca_name(md->chid);
 		      XTextExtents(fontTable[FONT_TABLE_INDEX],
			channelNames[liveChannels][0],
			strlen(channelNames[liveChannels][0]),
			&dir,&asc,&desc,&overall);
		      maxWidth = MAX(maxWidth,overall.width);
		      maxAsc = MAX(maxAsc,asc);
		      maxDesc = MAX(maxDesc,desc);
		      liveChannels++;
		    }
		  }
		}
		/* since most information for StripCharts is from connect...*/
		if (liveChannels > 0) {
		  maxWidth += X_SHIFT + MARGIN;
		  maxHeight = liveChannels*(maxAsc+maxDesc+MARGIN);
		  nominalY = maxAsc + MARGIN;
		  sourcePixmap = XCreatePixmap(display,RootWindow(display,
			screenNum),maxWidth,maxHeight,
			DefaultDepth(display,screenNum));
		  if (gc == NULL) gc = XCreateGC(display,sourcePixmap,0,NULL);
		  gcValueMask = GCForeground|GCBackground|GCFunction|GCFont;
		  gcValues.foreground = bg;
		  gcValues.background = bg;
		  gcValues.function = GXcopy;
		  gcValues.font = fontTable[FONT_TABLE_INDEX]->fid;
		  XChangeGC(display,gc,gcValueMask,&gcValues);
		  XFillRectangle(display,sourcePixmap,gc,0,0,maxWidth,
			maxHeight);
		  for (i = 0; i < liveChannels; i++) {
		    if (mdArray[i][0] != NULL && channelNames[i][0] != NULL) {
		      fg = alarmColorPixel[mdArray[i][0]->severity];
		      XSetForeground(display,gc,fg);
		      XDrawString(display,sourcePixmap,gc,X_SHIFT,
			(i+1)*nominalY,channelNames[i][0],
			strlen(channelNames[i][0]));
		    }
		  }
		}
		break;

	case DL_CartesianPlot:
		channelName = NULL;
		maxAsc = 0; maxDesc = 0; maxWidth = 0;
		liveTraces = 0;
		for (i = 0; i < MAX_PENS; i++)
		  for (j = 0; j <= 1; j++) {
		     channelNames[i][j] = NULL;
		     mdArray[i][j] = NULL;
		  }
		if (XtClass(searchWidget) == xmDrawingAreaWidgetClass) {
		/* if we got here via parent DA (not CP), get widget */
		   searchWidget = mData->self;
		}
		XtVaGetValues(searchWidget,XmNuserData,&userData,NULL);
		cartesianPlotData = (CartesianPlotData *)userData;
		for (i = 0; i < cartesianPlotData->nTraces; i++) {
		  newTrace = False;
		  for (j = 0; j <= 1; j++) {
		    if (cartesianPlotData->monitors[i][j] != NULL){
		      md = (Channel *)
				cartesianPlotData->monitors[i][j];
		      newTrace = True;
		      mdArray[liveTraces][j] = md;
		      if ( md->chid != NULL) {
		        channelNames[liveTraces][j] = ca_name(md->chid);
 		        XTextExtents(fontTable[FONT_TABLE_INDEX],
			  channelNames[liveTraces][j],
			  strlen(channelNames[liveTraces][j]),
			  &dir,&asc,&desc,&overall);
		        maxWidth = MAX(maxWidth,overall.width);
		        maxAsc = MAX(maxAsc,asc);
		        maxDesc = MAX(maxDesc,desc);
		      }
		    }
		  }
		  if (newTrace) liveTraces++;
		}
	     /* since most information for CartesianPlots is from connect..*/
		if (liveTraces > 0) {
		/* see if we have XY plot; if so make room for x,y in pixmap */
		  haveXY = False;
		  for (i = 0; i < liveTraces; i++) {
		    if (mdArray[i][0] != NULL && mdArray[i][1] != NULL)
			haveXY = True;
		  }
		  maxTextWidth = maxWidth;
		  if (haveXY) {
		    maxWidth += maxWidth + X_SHIFT + 4*MARGIN;
		  } else {
		    maxWidth += X_SHIFT + MARGIN;
		  }
		  maxHeight = liveTraces*(maxAsc+maxDesc+MARGIN);
		  nominalY = maxAsc + MARGIN;
		  sourcePixmap = XCreatePixmap(display,RootWindow(display,
			screenNum),maxWidth,maxHeight,
			DefaultDepth(display,screenNum));
		  if (gc == NULL) gc = XCreateGC(display,sourcePixmap,0,NULL);
		  gcValueMask = GCForeground|GCBackground|GCFunction|GCFont;
		  gcValues.foreground = bg;
		  gcValues.background = bg;
		  gcValues.function = GXcopy;
		  gcValues.font = fontTable[FONT_TABLE_INDEX]->fid;
		  XChangeGC(display,gc,gcValueMask,&gcValues);
		  XFillRectangle(display,sourcePixmap,gc,0,0,maxWidth,
			maxHeight);
		  for (i = 0; i < liveTraces; i++) {
		    for (j = 0; j <= 1; j++) {
		      if (mdArray[i][j] != NULL && channelNames[i][j] != NULL) {
		        fg = alarmColorPixel[mdArray[i][j]->severity];
		        XSetForeground(display,gc,fg);
		        XDrawString(display,sourcePixmap,gc,
			  (j == 0 ? X_SHIFT :
				maxTextWidth + 3*MARGIN + X_SHIFT),
			  (i+1)*nominalY,
			  channelNames[i][j],strlen(channelNames[i][j]));
		      }
		    }
		  }
		}
		break;

	default:
		fg = alarmColorPixel[mData->severity];
		overall.width = 0;
		asc = 0; desc = 0;
		channelName = NULL;
		if (mData->chid != NULL) channelName = ca_name(mData->chid);
		if (channelName != NULL) {
 		  XTextExtents(fontTable[FONT_TABLE_INDEX],channelName,
			strlen(channelName),&dir,&asc,&desc,&overall);
		  overall.width += X_SHIFT + MARGIN;
		  maxWidth = overall.width;
		  maxHeight = asc+desc+2*MARGIN;
		  sourcePixmap = XCreatePixmap(display,RootWindow(display,
			screenNum),maxWidth,maxHeight,
			DefaultDepth(display,screenNum));
		  if (gc == NULL) gc = XCreateGC(display,sourcePixmap,0,NULL);
		  gcValueMask = GCForeground|GCBackground|GCFunction|GCFont;
		  gcValues.foreground = bg;
		  gcValues.background = bg;
		  gcValues.function = GXcopy;
		  gcValues.font = fontTable[FONT_TABLE_INDEX]->fid;
		  XChangeGC(display,gc,gcValueMask,&gcValues);
		  XFillRectangle(display,sourcePixmap,gc,0,0,maxWidth,
			maxHeight);
		  XSetForeground(display,gc,fg);
		  XDrawString(display,sourcePixmap,gc,X_SHIFT,asc+MARGIN,
			channelName,strlen(channelName));
		}
		break;
    }

  } else {

    channelName = NULL;
    return;

  }


  if (sourcePixmap != (Pixmap)NULL) {

/* use source widget as parent - can inherit visual attributes that way */
    n = 0;
    XtSetArg(args[n],XmNpixmap,sourcePixmap); n++;
    XtSetArg(args[n],XmNwidth,maxWidth); n++;
    XtSetArg(args[n],XmNheight,maxHeight); n++;
    XtSetArg(args[n],XmNdepth,DefaultDepth(display,screenNum)); n++;
    sourceIcon = XmCreateDragIcon(XtParent(searchWidget),"sourceIcon",args,n);

/* establish list of valid target types */
    exportList[0] = COMPOUND_TEXT;

    n = 0;
    XtSetArg(args[n],XmNexportTargets,exportList); n++;
    XtSetArg(args[n],XmNnumExportTargets,1); n++;
    XtSetArg(args[n],XmNdragOperations,XmDROP_COPY); n++;
    XtSetArg(args[n],XmNconvertProc,DragConvertProc); n++;
    XtSetArg(args[n],XmNsourcePixmapIcon,sourceIcon); n++;
    XtSetArg(args[n],XmNcursorForeground,fg); n++;
    XtSetArg(args[n],XmNcursorBackground,bg); n++;
    XtSetArg(args[n],XmNdragDropFinishCallback,dragDropFinishCB); n++;
    XmDragStart(searchWidget,event,args,n);

  }
}

