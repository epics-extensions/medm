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

static void valuatorUpdateValueCb(Channel *pCh);
static void valuatorDestroyCb(Channel *pCh);


extern XtEventHandler handleValuatorRelease(Widget, XtPointer, XEvent *);

int valuatorFontListIndex(DlValuator *dlValuator)
{
  int i, index;
/* more complicated calculation based on orientation, etc */
  for (i = MAX_FONTS-1; i >=  0; i--) {
    switch (dlValuator->direction) {
	case UP:
	case DOWN:
	   switch(dlValuator->label) {
	      case LABEL_NONE:
		if ( (int)(.30*dlValuator->object.width) >= 
			(fontTable[i]->max_bounds.width) )
			return(i);
		break;
	      case OUTLINE:
	      case LIMITS:
		if ( (int)(.20*dlValuator->object.width) >= 
			(fontTable[i]->max_bounds.width) )
			return(i);
		break;
	      case CHANNEL:
		if ( (int)(.10*dlValuator->object.width) >= 
			(fontTable[i]->max_bounds.width) )
			return(i);
		break;
	   }
	   break;
	case LEFT:
	case RIGHT:
	   switch(dlValuator->label) {
	      case LABEL_NONE:
	      case OUTLINE:
	      case LIMITS:
		if ( (int)(.45*dlValuator->object.height) >= 
			(fontTable[i]->ascent + fontTable[i]->descent) )
			return(i);
		break;
	      case CHANNEL:
		if ( (int)(.32*dlValuator->object.height) >= 
			(fontTable[i]->ascent + fontTable[i]->descent) )
			return(i);
		break;
	   }
	   break;
    }
  }
  return (0);
}

createValuatorRunTimeInstance(DisplayInfo *displayInfo,
			      DlValuator *dlValuator) {
  Channel *pCh;
  Arg args[25];
  int i, n, heightDivisor, scalePopupBorder;
  WidgetList children;
  Cardinal numChildren;

  pCh = allocateChannel(displayInfo);

  pCh->monitorType = DL_Valuator;
  pCh->specifics=(XtPointer)dlValuator;
  pCh->clrmod = dlValuator->clrmod;
  pCh->backgroundColor = displayInfo->dlColormap[dlValuator->control.bclr];
  pCh->label = dlValuator->label;

  pCh->updateChannelCb = valuatorUpdateValueCb;
  pCh->updateGraphicalInfoCb = NULL;
  pCh->destroyChannel = valuatorDestroyCb;

  drawWhiteRectangle(pCh);

  SEVCHK(CA_BUILD_AND_CONNECT(dlValuator->control.ctrl,TYPENOTCONN,0,
	 &(pCh->chid),NULL,medmConnectEventCb, pCh),
	 "executeDlValuator: error in CA_BUILD_AND_CONNECT for Monitor");
/* from the valuator structure, we've got Valuator's specifics */
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlValuator->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlValuator->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlValuator->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlValuator->object.height); n++;
  XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
  XtSetArg(args[n],XmNforeground,(Pixel)
	displayInfo->dlColormap[dlValuator->control.clr]); n++;
  XtSetArg(args[n],XmNbackground,(Pixel)
	displayInfo->dlColormap[dlValuator->control.bclr]); n++;
  XtSetArg(args[n],XmNhighlightThickness,1); n++;
  XtSetArg(args[n],XmNhighlightOnEnter,TRUE); n++;
  switch(dlValuator->label) {
	case LABEL_NONE: 		/* add in border for keyboard popup */
	     scalePopupBorder = BORDER_WIDTH;
	     heightDivisor = 1;
	     break;
	case OUTLINE: case LIMITS: 
	     scalePopupBorder = 0;
	     heightDivisor = 2;
	     break;
	case CHANNEL:
	     scalePopupBorder = 0;
	     heightDivisor = 3;
	     break;
  }
/* need to handle Direction */
  switch (dlValuator->direction) {
    case UP: case DOWN:
	XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
	XtSetArg(args[n],XmNscaleWidth,dlValuator->object.width/heightDivisor 
				- scalePopupBorder); n++;
	break;
    case LEFT: case RIGHT:
	XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
	XtSetArg(args[n],XmNscaleHeight,dlValuator->object.height/heightDivisor
				- scalePopupBorder); n++;
	break;
  }
  /* add in CA controllerData as userData for valuator keyboard entry handling */
  XtSetArg(args[n],XmNuserData,(XtPointer)pCh); n++;
  XtSetArg(args[n],XmNfontList,fontListTable[
		valuatorFontListIndex(dlValuator)]); n++;
  pCh->self =  XtCreateWidget("valuator",
		xmScaleWidgetClass, displayInfo->drawingArea, args, n);

  displayInfo->child[displayInfo->childCount++] = pCh->self;

  /* get children of scale */
  XtVaGetValues(pCh->self,XmNnumChildren,&numChildren,
				XmNchildren,&children,NULL);
  /* set virtual range */
  n = 0;
  XtSetArg(args[n],XmNminimum,VALUATOR_MIN); n++;
  XtSetArg(args[n],XmNmaximum,VALUATOR_MAX); n++;
  XtSetArg(args[n],XmNscaleMultiple,VALUATOR_MULTIPLE_INCREMENT); n++;
  XtSetValues(pCh->self,args,n);
  /* add in drag/drop translations */
  XtOverrideTranslations(pCh->self,parsedTranslations);

  /* change translations for scrollbar child of valuator */
  for (i = 0; i < numChildren; i++) {
	 if (XtClass(children[i]) == xmScrollBarWidgetClass) {
		XtOverrideTranslations(children[i],parsedTranslations);
		/* add event handler for Key/ButtonRelease which enables updates */
		XtAddEventHandler(children[i],KeyReleaseMask|ButtonReleaseMask,
		False,(XtEventHandler)handleValuatorRelease,
		(XtPointer)pCh);
	 }
  }

  /* add the callbacks for update */
  XtAddCallback(pCh->self, XmNvalueChangedCallback,
		(XtCallbackProc)valuatorValueChanged,(XtPointer)pCh);
  XtAddCallback(pCh->self, XmNdragCallback,
		(XtCallbackProc)valuatorValueChanged,(XtPointer)pCh);

/* add event handler for expose - forcing display of min/max and value
 *	in own format
 */
  XtAddEventHandler(pCh->self,ExposureMask,
	False,(XtEventHandler)handleValuatorExpose,
	(XtPointer)pCh);

/* add event handler for Key/ButtonRelease which enables updates */
  XtAddEventHandler(pCh->self,KeyReleaseMask|ButtonReleaseMask,
	False,(XtEventHandler)handleValuatorRelease,
	(XtPointer)pCh);
}

void createValuatorEditInstance(DisplayInfo *displayInfo,
				DlValuator *dlValuator) {
  Arg args[25];
  int i, n, heightDivisor, scalePopupBorder;
  Widget widget;
  WidgetList children;
  Cardinal numChildren;

/* from the valuator structure, we've got Valuator's specifics */
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlValuator->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlValuator->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlValuator->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlValuator->object.height); n++;
  XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
  XtSetArg(args[n],XmNforeground,(Pixel)
	displayInfo->dlColormap[dlValuator->control.clr]); n++;
  XtSetArg(args[n],XmNbackground,(Pixel)
	displayInfo->dlColormap[dlValuator->control.bclr]); n++;
  XtSetArg(args[n],XmNhighlightThickness,1); n++;
  XtSetArg(args[n],XmNhighlightOnEnter,TRUE); n++;
  switch(dlValuator->label) {
	case LABEL_NONE: 		/* add in border for keyboard popup */
	     scalePopupBorder = BORDER_WIDTH;
	     heightDivisor = 1;
		  break;
	case OUTLINE: case LIMITS: 
	     scalePopupBorder = 0;
		  heightDivisor = 2;
	     break;
	case CHANNEL:
		  scalePopupBorder = 0;
	     heightDivisor = 3;
	     break;
  }
/* need to handle Direction */
  switch (dlValuator->direction) {
	 case UP: case DOWN:
	XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
	XtSetArg(args[n],XmNscaleWidth,dlValuator->object.width/heightDivisor 
				- scalePopupBorder); n++;
	break;
	 case LEFT: case RIGHT:
	XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
	XtSetArg(args[n],XmNscaleHeight,dlValuator->object.height/heightDivisor
				- scalePopupBorder); n++;
	break;
  }
/* add in CA controllerData as userData for valuator keyboard entry handling */
  XtSetArg(args[n],XmNfontList,fontListTable[
		valuatorFontListIndex(dlValuator)]); n++;
  widget =  XtCreateWidget("valuator",
		xmScaleWidgetClass, displayInfo->drawingArea, args, n);

  displayInfo->child[displayInfo->childCount++] = widget;

  /* get children of scale */
  XtVaGetValues(widget,XmNnumChildren,&numChildren,
				XmNchildren,&children,NULL);
  /* remove all translations if in edit mode */
  XtUninstallTranslations(widget);
  /* remove translations for children of valuator */
  for (i = 0; i < numChildren; i++) XtUninstallTranslations(children[i]);

  /*
	* add button press handlers too
	*/
  XtAddEventHandler(widget,
  ButtonPressMask, False, (XtEventHandler)handleButtonPress,
			(XtPointer)displayInfo);

  /* if in EDIT mode, add dlValuator as userData, and pass NULL in xepose */
  XtVaSetValues(widget,XmNuserData,(XtPointer)dlValuator, NULL);

  /* add event handler for expose - forcing display of min/max and value
	*	in own format
	*/
  XtAddEventHandler(widget,ExposureMask,False,
	 (XtEventHandler)handleValuatorExpose,(XtPointer)NULL);

  XtManageChild(widget);
}

void executeDlValuator(DisplayInfo *displayInfo, DlValuator *dlValuator,
			Boolean dummy)
{
  displayInfo->useDynamicAttribute = FALSE;
  
  if (displayInfo->traversalMode == DL_EXECUTE) {
	 createValuatorRunTimeInstance(displayInfo, dlValuator);
  } else
  if (displayInfo->traversalMode == DL_EDIT) {
    createValuatorEditInstance(displayInfo, dlValuator);
  }
}

static void valuatorUpdateValueCb(Channel *pCh) {
  DlValuator *dlValuator;
  Boolean dummy;

  if (ca_state(pCh->chid) == cs_conn) {
    if (ca_read_access(pCh->chid)) {
      if (pCh->self) 
        XtManageChild(pCh->self);
      else
	return;
      dlValuator = (DlValuator *)pCh->specifics;
      if (dlValuator == NULL) return;

      /* valuator is only controller/monitor which can have updates disabled */

      if (dlValuator->enableUpdates == True) {
        valuatorSetValue(pCh,pCh->value,True);
        {
           XExposeEvent event;
           event.count = 0;
           handleValuatorExpose(pCh->self,(XtPointer) pCh,(XEvent *) &event, &dummy);
	 }
        pCh->displayedValue = pCh->value;
      }
      if (ca_write_access(pCh->chid))
	XDefineCursor(XtDisplay(pCh->self),XtWindow(pCh->self),rubberbandCursor);
      else
	XDefineCursor(XtDisplay(pCh->self),XtWindow(pCh->self),noWriteAccessCursor);
    } else {
      draw3DPane(pCh);
      draw3DQuestionMark(pCh);
      if (pCh->self) XtUnmanageChild(pCh->self);
    }
  } else {
    if (pCh->self) XtUnmanageChild(pCh->self);
    drawWhiteRectangle(pCh);
  }
}

static void valuatorDestroyCb(Channel *pCh) {
  return;
}

static void valuatorStaticExposeCb(
  Widget w,
  XtPointer clientData,
  XEvent *pEvent,
  Boolean *continueToDispatch)
{ 
  XExposeEvent *event = (XExposeEvent *) pEvent;
  DlValuator *dlValuator;
  DisplayInfo *displayInfo;
  unsigned long foreground, background;
  Dimension scaleWidth, scaleHeight;
  int useableWidth, useableHeight, textHeight, textWidth, startX, startY;
  XFontStruct *font;
  unsigned long gcValueMask;
  XGCValues gcValues;
  XtPointer userData;

  if (event->count > 0) return;
  XtVaGetValues(w,XmNuserData,&dlValuator,NULL);
  if (dlValuator == NULL) return;
  displayInfo = dmGetDisplayInfoFromWidget(w);
  if (displayInfo == NULL) return;
  if (dlValuator->label != LABEL_NONE) {
    foreground = displayInfo->dlColormap[dlValuator->control.clr];
    background = displayInfo->dlColormap[dlValuator->control.bclr];
    font = fontTable[valuatorFontListIndex(dlValuator)];
    textHeight = font->ascent + font->descent;

    gcValueMask = GCForeground | GCFont | GCBackground | GCFunction;
    gcValues.function = GXcopy;
    gcValues.foreground = foreground;
    gcValues.background = background;
    gcValues.font = font->fid;
    XChangeGC(display, displayInfo->gc, gcValueMask, &gcValues);

    XtVaGetValues(w,XmNscaleWidth,&scaleWidth,
                    XmNscaleHeight,&scaleHeight,
                    NULL);
    switch (dlValuator->direction) {
      case UP:
      case DOWN:
        useableWidth = dlValuator->object.width - scaleWidth;
        switch(dlValuator->label) {
          case OUTLINE :
          case LIMITS :
          case CHANNEL :
            textWidth = XTextWidth(font,"0",1);
            startX = MAX(1,useableWidth - textWidth);
            startY = dlValuator->object.height - font->descent - 3;
            XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,"0",1);
            textWidth = XTextWidth(font,"0",1);
            startX = MAX(1,useableWidth - textWidth);
            if (dlValuator->label == CHANNEL) {
              /* need room for label above */
              startY = 1.3*(font->ascent + font->descent);
            } else {
              startY = font->ascent + 3;
            }
            XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,"0",1);
            if ((dlValuator->label == CHANNEL) && (dlValuator->control.ctrl)) {
              int nChars = strlen(dlValuator->control.ctrl);
              textWidth = XTextWidth(font,dlValuator->control.ctrl,nChars);
              startX = MAX(1,useableWidth - textWidth);
              startY = font->ascent + 2;
              XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,
                        dlValuator->control.ctrl,nChars);
            }
            break;
          default :
            break;
        }
        break;
      case LEFT:
      case RIGHT:
        useableHeight = dlValuator->object.height - scaleHeight;
	switch (dlValuator->label) {
          case OUTLINE :
          case LIMITS :
          case CHANNEL :
            startX = 2;
            startY = useableHeight - font->descent;/* NB: descent=0 for #s */
            XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,"0",1);
            textWidth = XTextWidth(font,"0",1);
            startX = dlValuator->object.width - textWidth - 2;
            startY = useableHeight - font->descent;
            XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,"0",1);
            if ((dlValuator->label == CHANNEL) && (dlValuator->control.ctrl)) {
              int nChars = strlen(dlValuator->control.ctrl);
              textWidth = XTextWidth(font,dlValuator->control.ctrl,nChars);
              startX = 2;
              startY = font->ascent + 2;
              XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,
                                dlValuator->control.ctrl,nChars);
            }
            break;
	  default :
	    break;
	}
      default :
        break;
    }
  }
}

void handleValuatorExpose(
  Widget w,
  XtPointer clientData,
  XEvent *pEvent,
  Boolean *continueToDispatch)
{
  XExposeEvent *event = (XExposeEvent *) pEvent;
  DlValuator *dlValuator;
  unsigned long foreground, background,alarmColor;
  Dimension scaleWidth, scaleHeight;
  int useableWidth, useableHeight, textHeight, textWidth, startX, startY;
  int n, nChars;
  Arg args[4];
  XFontStruct *font;
  char stringValue[40];
  unsigned long gcValueMask;
  XGCValues gcValues;
  Channel *pCh;
  DisplayInfo *displayInfo;
  XtPointer userData;
  double localLopr, localHopr;
  char *localTitle;
  int localPrecision;


  if (event->count > 0) return;

  if (clientData != NULL) {
/* then valid controllerData exists */
     pCh = (Channel *)clientData;
     if (pCh == NULL) return;
     displayInfo = pCh->displayInfo;
     if (displayInfo == NULL) return;
     dlValuator = (DlValuator *)pCh->specifics;
     if (dlValuator == NULL) return;
     localLopr = pCh->lopr;
     localHopr = pCh->hopr;
     localPrecision = pCh->precision;
     if (pCh->chid != NULL)
        localTitle = ca_name(pCh->chid);
     else localTitle = NULL;

  } else {
/* no controller data, therefore userData = dlValuator */
     XtSetArg(args[0],XmNuserData,&userData);
     XtGetValues(w,args,1);
     dlValuator = (DlValuator *)userData;
     if (dlValuator == NULL) return;
     localLopr = 0.0;
     localHopr = 0.0;
     localPrecision = 0;
     localTitle = dlValuator->control.ctrl;
     displayInfo = dmGetDisplayInfoFromWidget(w);
     if (displayInfo == NULL) return;

  }

/* since XmScale doesn't really do the right things, we'll do it by hand */

  if (dlValuator->label != LABEL_NONE) {

    foreground = displayInfo->dlColormap[dlValuator->control.clr];
    background = displayInfo->dlColormap[dlValuator->control.bclr];
    font = fontTable[valuatorFontListIndex(dlValuator)];
    textHeight = font->ascent + font->descent;

    gcValueMask = GCForeground | GCFont | GCBackground | GCFunction;
    gcValues.function = GXcopy;
    gcValues.foreground = foreground;
    gcValues.background = background;
    gcValues.font = font->fid;
    XChangeGC(display, displayInfo->pixmapGC, gcValueMask, &gcValues);
    XSetClipOrigin(display,displayInfo->pixmapGC,0,0);
    XSetClipMask(display,displayInfo->pixmapGC,None);

    switch (dlValuator->direction) {
      case UP: case DOWN:       /* but we know it's really only UP */
	XtVaGetValues(w,XmNscaleWidth,&scaleWidth,NULL);
        useableWidth = dlValuator->object.width - scaleWidth;
        if (dlValuator->label == OUTLINE || dlValuator->label == LIMITS
                                        || dlValuator->label == CHANNEL) {
/* LOPR */   localCvtDoubleToString(localLopr,stringValue,localPrecision);
             if (stringValue != NULL) {
               nChars = strlen(stringValue);
               textWidth = XTextWidth(font,stringValue,nChars);
               startX = MAX(1,useableWidth - textWidth);
               startY = dlValuator->object.height - font->descent - 3;
               XSetForeground(display,displayInfo->pixmapGC,background);
	       XFillRectangle(display,XtWindow(w),displayInfo->pixmapGC,
			      startX,MAX(1,startY-font->ascent),
			      textWidth,font->ascent+font->descent);
               XSetForeground(display,displayInfo->pixmapGC,foreground);
               XDrawString(display,XtWindow(w),displayInfo->pixmapGC,startX,startY,
                                                stringValue,nChars);
             }
/* HOPR */   localCvtDoubleToString(localHopr,stringValue,localPrecision);
             if (stringValue != NULL) {
               nChars = strlen(stringValue);
               textWidth = XTextWidth(font,stringValue,nChars);
               startX = MAX(1,useableWidth - textWidth);
               if (dlValuator->label == CHANNEL) {
                /* need room for label above */
                startY = 1.3*(font->ascent + font->descent)
                                + font->ascent;
               } else {
                startY = font->ascent + 3;
               }
               XSetForeground(display,displayInfo->pixmapGC,background);
	       XFillRectangle(display,XtWindow(w),displayInfo->pixmapGC,
			      startX,MAX(1,startY-font->ascent),
			      textWidth,font->ascent+font->descent);
               XSetForeground(display,displayInfo->pixmapGC,foreground);
               XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,
                                                stringValue,nChars);
             }
        }
        if (dlValuator->label == CHANNEL) {
/* TITLE */
             if (localTitle != NULL) {
               nChars = strlen(localTitle);
               textWidth = XTextWidth(font,localTitle,nChars);
               startX = MAX(1,useableWidth - textWidth);
               startY = font->ascent + 2;
               XDrawString(display,XtWindow(w),displayInfo->gc,startX,startY,
                                        localTitle,nChars);
             }
        }
        break;



      case LEFT: case RIGHT:    /* but we know it's really only RIGHT */
	XtVaGetValues(w,XmNscaleHeight,&scaleHeight,NULL);
        useableHeight = dlValuator->object.height - scaleHeight;

        if (dlValuator->label == OUTLINE || dlValuator->label == LIMITS
                                        || dlValuator->label == CHANNEL) {
/* LOPR */   localCvtDoubleToString(localLopr,stringValue,localPrecision);
             if (stringValue != NULL) {
               nChars = strlen(stringValue);
               textWidth = XTextWidth(font,stringValue,nChars);
               startX = 2;
               startY = useableHeight - font->descent;/* NB: descent=0 for #s */
               XSetForeground(display,displayInfo->pixmapGC,background);
	       XFillRectangle(display,XtWindow(w),displayInfo->pixmapGC,
			      startX,MAX(1,startY-font->ascent),
			      textWidth,font->ascent+font->descent);
               XSetForeground(display,displayInfo->pixmapGC,foreground);
               XDrawString(display,XtWindow(w),displayInfo->pixmapGC,startX,startY,
                                                stringValue,nChars);
             }
/* HOPR */   localCvtDoubleToString(localHopr,stringValue,localPrecision);
             if (stringValue != NULL) {
               nChars = strlen(stringValue);
               textWidth = XTextWidth(font,stringValue,nChars);
               startX = dlValuator->object.width - textWidth - 2;
               startY = useableHeight - font->descent;
               XSetForeground(display,displayInfo->pixmapGC,background);
	       XFillRectangle(display,XtWindow(w),displayInfo->pixmapGC,
			      startX,MAX(1,startY-font->ascent),
			      textWidth,font->ascent+font->descent);
               XSetForeground(display,displayInfo->pixmapGC,foreground);
               XDrawString(display,XtWindow(w),displayInfo->pixmapGC,startX,startY,
                                                stringValue,nChars);
             }
        }
        if (dlValuator->label == CHANNEL) {
/* TITLE */
             if (localTitle != NULL) {
               nChars = strlen(localTitle);
               textWidth = XTextWidth(font,localTitle,nChars);
               startX = 2;
               startY = font->ascent + 2;
               XDrawString(display,XtWindow(w),displayInfo->pixmapGC,startX,startY,
                                localTitle,nChars);
             }
        }
        break;
    }
    if (clientData != NULL) {
/* real data */
      valuatorRedrawValue(pCh,0.0,False,
            pCh->displayInfo,pCh->self,
                (DlValuator *)pCh->specifics);


    } else {
/* fake data */
      valuatorRedrawValue(NULL,0.0,False,currentDisplayInfo,w,dlValuator);
    }
  }  /* end --  if (dlValuator->label != LABEL_NONE) */


}

/*
 * set value (with implicit redraw of value) for valuator
 */
void valuatorSetValue(Channel *pCh, double forcedValue,
                        Boolean force)
{
  int iValue;
  double dValue;
  Arg args[1];

/* if we got here "too soon" simply return */
  if (pCh == NULL) return;
  if (pCh->self == NULL) return;

  if (pCh->hopr != pCh->lopr) {
    if (force)
        dValue = forcedValue;
    else
        dValue = pCh->value;

/* to make reworked event handling for Valuator work */
    pCh->value = dValue;

/* update scale widget */
    iValue = VALUATOR_MIN + ((dValue - pCh->lopr)
                /(pCh->hopr - pCh->lopr))
                *((double)(VALUATOR_MAX - VALUATOR_MIN));
    pCh->oldIntegerValue = iValue;
    XtSetArg(args[0],XmNvalue,iValue);
    XtSetValues(pCh->self,args,1);

/* update and render string value, use pCh (ignore other stuff) */
    valuatorRedrawValue(pCh,forcedValue,force,
        pCh->displayInfo,pCh->self,
        (DlValuator *)pCh->specifics);

  }
}

/*
 * redraw value for valuator
 */
void valuatorRedrawValue(Channel *pCh,
        double forcedValue, Boolean force, DisplayInfo *displayInfo, Widget w,
        DlValuator *dlValuator)
{
  unsigned long foreground, background;
  Dimension scaleWidth, scaleHeight;
  int useableWidth, useableHeight, textHeight, textWidth, startX, startY;
  int n, nChars;
  Arg args[4];
  XFontStruct *font;
  char stringValue[40];
  unsigned long gcValueMask;
  XGCValues gcValues;

  int localPrecision;
  double localValue;

/* return if no window for widget yet, or if displayInfo == NULL, or ... */
  if (XtWindow(w) == (Window)NULL || displayInfo == (DisplayInfo *)NULL ||
      dlValuator == (DlValuator *)NULL) return;

/* simply return if no value to render */
  if (!(dlValuator->label == LIMITS || dlValuator->label == CHANNEL)) return;

  if (pCh == NULL) {
/* faking values */
    localPrecision = 0;
    localValue = 0.0;
  } else {
/* use real values */
    localPrecision = pCh->precision;
    localValue = pCh->value;
  }

  foreground = displayInfo->dlColormap[dlValuator->control.clr];
  background = displayInfo->dlColormap[dlValuator->control.bclr];
  font = fontTable[valuatorFontListIndex(dlValuator)];
  textHeight = font->ascent + font->descent;
  if (force)
    localCvtDoubleToString(forcedValue,stringValue,localPrecision);
  else
    localCvtDoubleToString(localValue,stringValue,localPrecision);
   nChars = strlen(stringValue);

  gcValueMask = GCForeground | GCFont | GCBackground | GCFunction;
  gcValues.function = GXcopy;
  gcValues.foreground = background;
  gcValues.background = background;
  gcValues.font = font->fid;
  XChangeGC(display, displayInfo->pixmapGC, gcValueMask, &gcValues);
  switch (dlValuator->direction) {
      case UP: case DOWN:
	XtVaGetValues(w, XmNscaleWidth,&scaleWidth, NULL);
        useableWidth = dlValuator->object.width - scaleWidth;
        textWidth = XTextWidth(font,stringValue,nChars);
        startX = MAX(1,useableWidth - textWidth);
        startY = dlValuator->object.height/2 - font->ascent/2;

        if (pCh != NULL) {
/* reuse pCh->fontIndex (int) for storing Valuator's max text width */
           XFillRectangle(display,XtWindow(w),displayInfo->pixmapGC,
                MAX(1,useableWidth - pCh->fontIndex),
                startY - font->ascent,
                pCh->fontIndex,font->ascent+font->descent);
           pCh->fontIndex = textWidth;
        }


        if ((dlValuator->clrmod == ALARM) && (pCh != NULL))  {
           XSetForeground(display,displayInfo->pixmapGC,alarmColorPixel[pCh->severity]);
        } else {
           XSetForeground(display,displayInfo->pixmapGC,foreground);
        }
        break;
      case LEFT: case RIGHT:    /* but we know it's really only RIGHT */
	XtVaGetValues(w,XmNscaleHeight,&scaleHeight,NULL);
        useableHeight = dlValuator->object.height - scaleHeight;
        textWidth = XTextWidth(font,stringValue,nChars);
        startX = dlValuator->object.width/2 - textWidth/2;
        startY = useableHeight - font->descent;

        if (pCh != NULL) {
/* reuse pCh->fontIndex (int) for storing Valuator's max text width */
           XFillRectangle(display,XtWindow(w),displayInfo->pixmapGC,
                dlValuator->object.width/2 - pCh->fontIndex/2,
                startY - font->ascent,
                pCh->fontIndex,font->ascent+font->descent);
           pCh->fontIndex = textWidth;
        }

        if (dlValuator->clrmod == ALARM && pCh != NULL) {
           XSetForeground(display,displayInfo->pixmapGC,
                        alarmColorPixel[pCh->severity]);
        } else {
           XSetForeground(display,displayInfo->pixmapGC,foreground);
        }
        break;
  }  /* end switch() */

  XDrawString(display,XtWindow(w),displayInfo->pixmapGC,startX,startY,
              stringValue,nChars);
}

/*
 * thanks to complicated valuator interactions, need to rely on
 *  Key/ButtonRelease events to re-enable updates for dlValuator display
 */
XtEventHandler handleValuatorRelease(
  Widget w,
  XtPointer passedData,
  XEvent *event)
{
  Channel *pCh;
  DlValuator *dlValuator;

  if (passedData != NULL) {
/* then valid controllerData exists */
     pCh = (Channel *)passedData;
     dlValuator = (DlValuator *)pCh->specifics;
     switch(event->type) {
     case ButtonRelease:
     case KeyRelease:
        dlValuator->enableUpdates = True;
        /* don't reset ->dragging: let valuatorValueChanged() do that */
        break;
     }
  }
}

static XtCallbackProc destroyDialog(
  Widget  w,
  XtPointer client_data,
  XmSelectionBoxCallbackStruct *call_data)
{
  XtDestroyWidget(XtParent(w));
}


static XtCallbackProc precisionToggleChangedCallback(
  Widget w,
  XtPointer client_data,
  XmToggleButtonCallbackStruct *call_data)
{
  Widget widget;
  long longValue;
  short shortValue;
  Channel *pCh;
  DlValuator *valuator;

/* only respond to the button actually set */
  if (call_data->event != NULL && call_data->set == True) {

    longValue = (long)client_data;
    shortValue = (short)longValue;

    XtVaGetValues(w,XmNuserData,&pCh,NULL);
/*
 * now set the prec field in the valuator data structure, and update
 * the valuator (scale) resources
 */
    if (pCh != NULL) {
      valuator = (DlValuator *)pCh->specifics;
      valuator->dPrecision = pow(10.,(double)shortValue);
    }

/* hierarchy = TB<-RB<-Frame<-SelectionBox<-Dialog */
    widget = w;
    while (XtClass(widget) != xmDialogShellWidgetClass) {
      widget = XtParent(widget);
    }

    XtDestroyWidget(widget);
  }

}



/*
 * text field processing callback
 */
static XtCallbackProc precTextFieldActivateCallback(
  Widget w,
  DlValuator *dlValuator,
  XmTextVerifyCallbackStruct *cbs)
{
  char *stringValue;
  int i;
  Widget widget;

  stringValue = XmTextFieldGetString(w);
  dlValuator->dPrecision = atof(stringValue);
  XtFree(stringValue);

/* hierarchy = TB<-RB<-Frame<-SelectionBox<-Dialog */
    widget = w;
    while (XtClass(widget) != xmDialogShellWidgetClass) {
      widget = XtParent(widget);
    }
    XtDestroyWidget(widget);
}



/*
 * text field losing focus callback
 */
static XtCallbackProc precTextFieldLosingFocusCallback(
  Widget w,
  DlValuator *dlValuator,
  XmTextVerifyCallbackStruct *cbs)
{
  char string[MAX_TOKEN_LENGTH];
  int tail;

/*
 * losing focus - make sure that the text field remains accurate
 *      wrt dlValuator
 */
  sprintf(string,"%f",dlValuator->dPrecision);
  /* strip trailing zeroes */
  tail = strlen(string);
  while (string[--tail] == '0') string[tail] = '\0';
  XmTextFieldSetString(w,string);

}

static void sendKeyboardValue(
  Widget w,
  XtPointer clientData,
  XtPointer callbackStruct)
{
  Channel *pCh = (Channel *) clientData;
  XmSelectionBoxCallbackStruct *call_data = (XmSelectionBoxCallbackStruct *) callbackStruct;
  double value;
  char *stringValue;

  if (pCh == NULL) return;

  XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET, &stringValue);

  if (stringValue != NULL) {
    value = atof(stringValue);

    /* move/redraw valuator & value, but force use of user-selected value */
    if ((ca_state(pCh->chid) == cs_conn) && ca_write_access(pCh->chid)) {
      SEVCHK(ca_put(DBR_DOUBLE,pCh->chid,&value),"sendKeyboardValue: error in ca_put");
      ca_flush_io();
      valuatorSetValue(pCh,value,True);
    }
    XtFree(stringValue);
  }
  XtDestroyWidget(XtParent(w));

}

void popupValuatorKeyboardEntry(
  Widget w,
  DisplayInfo *displayInfo,
  XEvent *event)
{
#define MAX_TOGGLES 20
  Widget keyboardDialog;
  char valueLabel[MAX_TOKEN_LENGTH + 8];
  XmString xmTitle, xmValueLabel, valueXmString;
  char valueString[40];
  char *channel;
  Arg args[8];
  int n;
  Channel *pCh;

  Widget frame, frameLabel, radioBox, toggles[MAX_TOGGLES];
  Widget form, textField;
  XmString frameXmString, toggleXmString;
  double hoprLoprAbs;
  short numColumns, numPlusColumns, numMinusColumns, shortValue;
  char toggleString[4];
  int i, count, tail;
  long longValue;
  DlValuator *dlValuator;

  XButtonEvent *xEvent = (XButtonEvent *)event;

  if (globalDisplayListTraversalMode == DL_EDIT) {
  /* do nothing */
  } else {

    if (xEvent->button != Button3) return;

    XtVaGetValues(w,XmNuserData,&pCh,NULL);
    if (pCh) {
      if (pCh->chid) {
        channel = ca_name(pCh->chid);
        if ((ca_state(pCh->chid) == cs_conn) && ca_write_access(pCh->chid) &&
                                strlen(channel) > 0) {
          /* create selection box/prompt dialog */
          strcpy(valueLabel,"VALUE: ");
          strcat(valueLabel,channel);
          xmValueLabel = XmStringCreateSimple(valueLabel);
          xmTitle = XmStringCreateSimple(channel);
          dlValuator = (DlValuator *)pCh->specifics;
          localCvtDoubleToString(pCh->value,valueString,
                        pCh->precision);
          valueXmString = XmStringCreateSimple(valueString);
          n = 0;
          XtSetArg(args[n],XmNdialogStyle,
                                XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
          XtSetArg(args[n],XmNselectionLabelString,xmValueLabel); n++;
          XtSetArg(args[n],XmNdialogTitle,xmTitle); n++;
          XtSetArg(args[n],XmNtextString,valueXmString); n++;
          keyboardDialog = XmCreatePromptDialog(w,channel,args,n);

          /* remove resize handles from shell */

          XtVaSetValues(XtParent(keyboardDialog),
                      XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH,
                      NULL);

          XtAddCallback(keyboardDialog,XmNokCallback,
                (XtCallbackProc)sendKeyboardValue, pCh);
          XtAddCallback(keyboardDialog,XmNcancelCallback,
                (XtCallbackProc)destroyDialog,NULL);

          /* create frame/radiobox/toggles for precision selection */
          hoprLoprAbs = fabs(pCh->hopr);
          hoprLoprAbs = MAX(hoprLoprAbs,fabs(pCh->lopr));
        /* log10 + 1 */
          numPlusColumns =  (short)log10(hoprLoprAbs) + 1;
          numMinusColumns = (short)pCh->precision;
        /* leave room for decimal point */
          numColumns = numPlusColumns + 1 + numMinusColumns;
          if (numColumns > MAX_TOGGLES) {
            medmPrintf(
              "\npopupValuatorKeyboardEntry: maximum # of toggles exceeded");
              numColumns = MAX_TOGGLES;
          }
          n = 0;
          frame = XmCreateFrame(keyboardDialog,"frame",args,n);
          frameXmString = XmStringCreateSimple("VALUATOR PRECISION (10^X)");
          XtSetArg(args[n],XmNlabelString,frameXmString); n++;
          XtSetArg(args[n],XmNchildType,XmFRAME_TITLE_CHILD); n++;
          frameLabel = XmCreateLabel(frame,"frameLabel",args,n);
          XtManageChild(frameLabel);

          n = 0;
          XtSetArg(args[n],XmNchildType,XmFRAME_WORKAREA_CHILD); n++;
          XtSetArg(args[n],XmNshadowThickness,0); n++;
          form = XmCreateForm(frame,"form",args,n);

/* radio box */
          n = 0;
          XtSetArg(args[n],XmNnumColumns,numColumns); n++;
          XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
          XtSetArg(args[n],XmNadjustLast,False); n++;
          XtSetArg(args[n],XmNspacing,0); n++;
          radioBox = XmCreateRadioBox(form,"radioBox",args,n);

          toggleXmString = (XmString)NULL;
          XtSetArg(args[0],XmNindicatorOn,False);
/* digits to the left of the decimal point */
          count = 0;
          for (i = numPlusColumns - 1; i >= 0; i--) {
             if (toggleXmString != NULL) XmStringFree(toggleXmString);
             shortValue = (short)i;
             cvtShortToString(shortValue,toggleString);
             toggleXmString = XmStringCreateSimple(toggleString);
             XtSetArg(args[1],XmNlabelString,toggleXmString);
             XtSetArg(args[2],XmNuserData,(XtPointer)
                                        pCh);
             if (log10(dlValuator->dPrecision) == (double)i) {
                XtSetArg(args[3],XmNset,True);
             }
             toggles[count++] = XmCreateToggleButton(radioBox,"toggles",args,
                        (log10(dlValuator->dPrecision) == (double)i ? 4 : 3));
             longValue = (long)shortValue;
             XtAddCallback(toggles[count-1],XmNvalueChangedCallback,
                        (XtCallbackProc)precisionToggleChangedCallback,
                        (XtPointer)longValue);
          }
/* the decimal point */
          if (toggleXmString != NULL) XmStringFree(toggleXmString);
          toggleString[0] = '.'; toggleString[1] = '\0';
          toggleXmString = XmStringCreateSimple(toggleString);
          XtSetArg(args[1],XmNlabelString,toggleXmString);
          XtSetArg(args[2],XmNshadowThickness,0);
          toggles[count++] = XmCreateToggleButton(radioBox,"toggles",args,3);
          XtSetSensitive(toggles[count-1],False);

/* digits to the right of the decimal point */
          for (i = 1; i <= numMinusColumns; i++) {
             if (toggleXmString != NULL) XmStringFree(toggleXmString);
             shortValue = (short)-i;
             cvtShortToString(shortValue,toggleString);
             toggleXmString = XmStringCreateSimple(toggleString);
             XtSetArg(args[1],XmNlabelString,toggleXmString);
             XtSetArg(args[2],XmNuserData,(XtPointer) pCh);
             if (log10(dlValuator->dPrecision) == (double)-i) {
                XtSetArg(args[3],XmNset,True);
             }
             toggles[count++] = XmCreateToggleButton(radioBox,"toggles",args,
                        (log10(dlValuator->dPrecision) == (double)-i ? 4 : 3));
             longValue = (long)shortValue;
             XtAddCallback(toggles[count-1],XmNvalueChangedCallback,
                        (XtCallbackProc)precisionToggleChangedCallback,
                        (XtPointer)longValue);
          }

/* text field */
          n = 0;
          textField = XmCreateTextField(form,"textField",args,n);
          XtAddCallback(textField,XmNactivateCallback,
                        (XtCallbackProc)precTextFieldActivateCallback,
                        (XtPointer)dlValuator);
          XtAddCallback(textField,XmNlosingFocusCallback,
                        (XtCallbackProc)precTextFieldLosingFocusCallback,
                        (XtPointer)dlValuator);
          XtAddCallback(textField,XmNmodifyVerifyCallback,
                        (XtCallbackProc)textFieldFloatVerifyCallback,
                        (XtPointer)NULL);
          sprintf(valueString,"%f",dlValuator->dPrecision);
          /* strip trailing zeroes */
          tail = strlen(valueString);
          while (valueString[--tail] == '0') valueString[tail] = '\0';
          XmTextFieldSetString(textField,valueString);


/* now specify attatchments of radio box and text field in form */
          n = 0;
          XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
          XtSetValues(radioBox,args,n);
          n = 0;
          XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
          XtSetArg(args[n],XmNtopWidget,radioBox); n++;
          XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
          XtSetValues(textField,args,n);


          XtManageChildren(toggles,numColumns);
          XtManageChild(radioBox);
          XtManageChild(textField);
          XtManageChild(form);
          XtManageChild(frame);
          if (toggleXmString != NULL) XmStringFree(toggleXmString);
          XmStringFree(frameXmString);

          XtManageChild(keyboardDialog);
          XmStringFree(xmValueLabel);
          XmStringFree(xmTitle);
          XmStringFree(valueXmString);
        }
      }
    }
  }
}

/*
 * valuatorValueChanged - drag and value changed callback for valuator
 */

void valuatorValueChanged(
  Widget  w,
  XtPointer clientData,
  XtPointer callbackStruct)
{
  Channel *pCh = (Channel *) clientData;
  XmScaleCallbackStruct *call_data = (XmScaleCallbackStruct *) callbackStruct;
  DlValuator *dlValuator;
  Arg args[3];
  XButtonEvent *buttonEvent;
  XKeyEvent *keyEvent;
  double value;


  if (pCh == NULL) return;      /* do nothing if invalid data ptrs */
  if (pCh->chid == NULL) return; /* do nothing if invalid chid */
  if (ca_state(pCh->chid) == cs_conn) {

    /* set modified flag on monitor data so that next update traversal will
     * set controller visual state correctly (noting that for controllers
     * as monitors the ->modified flag alone is used to do updates
     */
    globalModifiedFlag = True;
    pCh->modified = PRIMARY_MODIFIED;

    dlValuator = (DlValuator *)pCh->specifics;
    if (dlValuator == NULL) return;

    value = pCh->value;
    if (call_data->reason == XmCR_DRAG) {
      dlValuator->dragging = True;            /* mark beginning of drag  */
      dlValuator->enableUpdates = False;      /* disable updates in drag */

      /* drag - set value based on relative position (easy) */
      pCh->oldIntegerValue = call_data->value;
      value = pCh->lopr + ((double)(call_data->value - VALUATOR_MIN))
                /((double)(VALUATOR_MAX - VALUATOR_MIN) )*(pCh->hopr - pCh->lopr);

    } else 
    if (call_data->reason = XmCR_VALUE_CHANGED) {
      if (dlValuator->dragging) {
        /* valueChanged can mark conclusion of drag, hence enable updates */
        dlValuator->enableUpdates = True;
        dlValuator->dragging = False;
      } else {
        /* rely on Button/KeyRelease event handler to re-enable updates */
        dlValuator->enableUpdates = False;
        dlValuator->dragging = False;
      }

      /* value changed - has to deal with precision, etc (hard) */
      if (call_data->event != NULL) {
        if (call_data->event->type == KeyPress) {
          keyEvent = (XKeyEvent *)call_data->event;
          if (keyEvent->state & ControlMask) {
            /* multiple increment (10*precision) */
            if (pCh->oldIntegerValue > call_data->value) {
              /* decrease value one 10*precision value */
              value = MAX(pCh->lopr, pCh->value - 10.*dlValuator->dPrecision);
            } else 
            if (pCh->oldIntegerValue < call_data->value) {
              /* increase value one 10*precision value */
              value = MIN(pCh->hopr, pCh->value + 10.*dlValuator->dPrecision);
            }
          } else {
            /* single increment (precision) */
            if (pCh->oldIntegerValue > call_data->value) {
              /* decrease value one precision value */
              value = MAX(pCh->lopr, pCh->value - dlValuator->dPrecision);
            } else
            if (pCh->oldIntegerValue < call_data->value) {
              /* increase value one precision value */
              value = MIN(pCh->hopr, pCh->value + dlValuator->dPrecision);
            }
          }
        } else
        if (call_data->event->type == ButtonPress) {
          buttonEvent = (XButtonEvent *)call_data->event;
          if (buttonEvent->state & ControlMask) {
            /* handle this as multiple increment/decrement */
            if (call_data->value - pCh->oldIntegerValue < 0) {
              /* decrease value one 10*precision value */
              value = MAX(pCh->lopr, pCh->value - 10.*dlValuator->dPrecision);
            } else
            if (call_data->value - pCh->oldIntegerValue > 0) {
              /* increase value one 10*precision value */
              value = MIN(pCh->hopr, pCh->value + 10.*dlValuator->dPrecision);
            }
          }
        }  /* end if/else (KeyPress/ButtonPress) */
      } else {
        /* handle null events (direct MB1, etc does this)
         * (MDA) modifying valuator to depart somewhat from XmScale, but more
         *   useful for real application (of valuator)
         * NB: modifying - MB1 either side of slider means move one increment only;
         *   even though std. is Multiple (let Ctrl-MB1 mean multiple (not go-to-end))
         */
        if (call_data->value - pCh->oldIntegerValue < 0) {
          /* decrease value one precision value */
          value = MAX(pCh->lopr, pCh->value - dlValuator->dPrecision);
        } else
        if (call_data->value - pCh->oldIntegerValue > 0) {
          /* increase value one precision value */
          value = MIN(pCh->hopr, pCh->value + dlValuator->dPrecision);
        }
      }  /* end if (call_data->event != NULL) */
    }

    if (ca_write_access(pCh->chid)) {
      SEVCHK(ca_put(DBR_DOUBLE,pCh->chid,&value),
          "valuatorValueChanged: error in ca_put");
      ca_flush_io();
      /* move/redraw valuator & value, but force use of user-selected value */
      valuatorSetValue(pCh,value,True);
    } else {
      fprintf(stderr,"\a");
      valuatorSetValue(pCh,pCh->value,True);
    }
  }
}


