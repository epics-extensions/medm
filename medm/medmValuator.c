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
 * .02  06-01-95        vong    2.1.0 release
 * .03  09-12-95        vong    conform to c++ syntax
 * .04  11-06-95        vong    fix the valuator increment problem
 *
 *****************************************************************************
*/

#include "medm.h"
#include <Xm/MwmUtil.h>

#ifdef __cplusplus
extern "C" {
#endif
#include <cvtFast.h>
#ifdef __cplusplus
}
#endif

typedef struct _Valuator {
  Widget      widget;
  DlValuator *dlValuator;
  Record      *record;
  UpdateTask  *updateTask;
  int         oldIntegerValue;
  int         fontIndex;
} Valuator;

static void valuatorDraw(XtPointer);
static void valuatorUpdateValueCb(XtPointer);
static void valuatorDestroyCb(XtPointer);
static void valuatorSetValue(Valuator *, double, Boolean force);
static void valuatorRedrawValue(Valuator *, DisplayInfo *, Widget, DlValuator *, double);
static void handleValuatorExpose(Widget, XtPointer, XEvent *, Boolean *);
static void valuatorName(XtPointer, char **, short *, int *);


static void handleValuatorRelease(Widget, XtPointer, XEvent *, Boolean *);

int valuatorFontListIndex(DlValuator *dlValuator)
{
  int i;
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

void createValuatorRunTimeInstance(DisplayInfo *displayInfo,
			      DlValuator *dlValuator) {
  Valuator *pv;
  Arg args[25];
  int i, n, heightDivisor, scalePopupBorder;
  WidgetList children;
  Cardinal numChildren;

  pv = (Valuator *) malloc(sizeof(Valuator));
  if (pv == NULL) {
    medmPrintf("valuatorCreateRunTimeInstance : memory allocation error\n");
    return;
  }
  pv->dlValuator = dlValuator;
  pv->updateTask = updateTaskAddTask(displayInfo,
				     &(dlValuator->object),
				     valuatorDraw,
				     (XtPointer)pv);

  if (pv->updateTask == NULL) {
    medmPrintf("valuatorCreateRunTimeInstance : memory allocation error\n");
    free((char *)pv);
    return;
  } else {
    updateTaskAddDestroyCb(pv->updateTask,valuatorDestroyCb);
    updateTaskAddNameCb(pv->updateTask,valuatorName);
  }
  pv->record = medmAllocateRecord(dlValuator->control.ctrl,
                  valuatorUpdateValueCb,
                  NULL,
                  (XtPointer) pv);
  pv->fontIndex = valuatorFontListIndex(dlValuator);
  drawWhiteRectangle(pv->updateTask);

/* from the valuator structure, we've got Valuator's specifics */
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlValuator->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlValuator->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlValuator->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlValuator->object.height); n++;
  XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
  XtSetArg(args[n],XmNforeground,displayInfo->dlColormap[dlValuator->control.clr]); n++;
  XtSetArg(args[n],XmNbackground,displayInfo->dlColormap[dlValuator->control.bclr]); n++;
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
  /* add in Valuator as userData for valuator keyboard entry handling */
  XtSetArg(args[n],XmNuserData,(XtPointer)pv); n++;
  XtSetArg(args[n],XmNfontList,fontListTable[pv->fontIndex]); n++;
  pv->widget =  XtCreateWidget("valuator",
		xmScaleWidgetClass, displayInfo->drawingArea, args, n);

  displayInfo->child[displayInfo->childCount++] = pv->widget;

  /* get children of scale */
  XtVaGetValues(pv->widget,XmNnumChildren,&numChildren,
				XmNchildren,&children,NULL);
  /* set virtual range */
  n = 0;
  XtSetArg(args[n],XmNminimum,VALUATOR_MIN); n++;
  XtSetArg(args[n],XmNmaximum,VALUATOR_MAX); n++;
  XtSetArg(args[n],XmNscaleMultiple,VALUATOR_MULTIPLE_INCREMENT); n++;
  XtSetValues(pv->widget,args,n);
  /* add in drag/drop translations */
  XtOverrideTranslations(pv->widget,parsedTranslations);

  /* change translations for scrollbar child of valuator */
  for (i = 0; i < numChildren; i++) {
	 if (XtClass(children[i]) == xmScrollBarWidgetClass) {
		XtOverrideTranslations(children[i],parsedTranslations);
		/* add event handler for Key/ButtonRelease which enables updates */
		XtAddEventHandler(children[i],KeyReleaseMask|ButtonReleaseMask,
		False,handleValuatorRelease,
		(XtPointer)pv);
	 }
  }

  /* add the callbacks for update */
  XtAddCallback(pv->widget, XmNvalueChangedCallback,
		valuatorValueChanged,(XtPointer)pv);
  XtAddCallback(pv->widget, XmNdragCallback,
		valuatorValueChanged,(XtPointer)pv);

/* add event handler for expose - forcing display of min/max and value
 *	in own format
 */
  XtAddEventHandler(pv->widget,ExposureMask,
	False,handleValuatorExpose,
	(XtPointer)pv);

/* add event handler for Key/ButtonRelease which enables updates */
  XtAddEventHandler(pv->widget,KeyReleaseMask|ButtonReleaseMask,
	False,handleValuatorRelease,
	(XtPointer)pv);
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

#ifdef __cplusplus
void executeDlValuator(DisplayInfo *displayInfo, DlValuator *dlValuator,
			Boolean)
#else
void executeDlValuator(DisplayInfo *displayInfo, DlValuator *dlValuator,
			Boolean dummy)
#endif
{
  displayInfo->useDynamicAttribute = FALSE;
  
  if (displayInfo->traversalMode == DL_EXECUTE) {
	 createValuatorRunTimeInstance(displayInfo, dlValuator);
  } else
  if (displayInfo->traversalMode == DL_EDIT) {
    createValuatorEditInstance(displayInfo, dlValuator);
  }
}

static void valuatorUpdateValueCb(XtPointer cd) {
  Valuator *pv = (Valuator *) ((Record *) cd)->clientData;
  updateTaskMarkUpdate(pv->updateTask);
}

static void valuatorDraw(XtPointer cd) {
  Valuator *pv = (Valuator *) cd;
  Record *pd = pv->record;
  DlValuator *dlValuator = pv->dlValuator;
  Boolean dummy;

  if (pd->connected) {
    if (pd->readAccess) {
      if (pv->widget) 
        XtManageChild(pv->widget);
      else
	return;

      /* valuator is only controller/monitor which can have updates disabled */

      if (dlValuator->enableUpdates) {
        valuatorSetValue(pv,pd->value,True);
        {
           XExposeEvent event;
           event.count = 0;
           handleValuatorExpose(pv->widget,(XtPointer) pv,(XEvent *) &event, &dummy);
	 }
      }
      if (pd->writeAccess)
	XDefineCursor(XtDisplay(pv->widget),XtWindow(pv->widget),rubberbandCursor);
      else
	XDefineCursor(XtDisplay(pv->widget),XtWindow(pv->widget),noWriteAccessCursor);
    } else {
      draw3DPane(pv->updateTask,
        pv->updateTask->displayInfo->dlColormap[pv->dlValuator->control.bclr]);
      draw3DQuestionMark(pv->updateTask);
      if (pv->widget) XtUnmanageChild(pv->widget);
    }
  } else {
    if (pv->widget) XtUnmanageChild(pv->widget);
    drawWhiteRectangle(pv->updateTask);
  }
}

static void valuatorDestroyCb(XtPointer cd) {
  Valuator *pv = (Valuator *) cd;
  if (pv) {
    medmDestroyRecord(pv->record);
    free((char *)pv);
  }
  return;
}

#ifdef __cplusplus
void handleValuatorExpose(
  Widget w,
  XtPointer clientData,
  XEvent *pEvent,
  Boolean *)
#else
void handleValuatorExpose(
  Widget w,
  XtPointer clientData,
  XEvent *pEvent,
  Boolean *continueToDispatch)
#endif
{
  XExposeEvent *event = (XExposeEvent *) pEvent;
  Valuator *pv;
  DlValuator *dlValuator;
  unsigned long foreground, background;
  Dimension scaleWidth, scaleHeight;
  int useableWidth, useableHeight, textHeight, textWidth, startX, startY;
  int nChars;
  Arg args[4];
  XFontStruct *font;
  char stringValue[40];
  unsigned long gcValueMask;
  XGCValues gcValues;
  DisplayInfo *displayInfo;
  double localLopr, localHopr;
  char *localTitle;
  int localPrecision;

  if (event->count > 0) return;

  if (clientData) {
/* then valid controllerData exists */
     Record *pd;
     pv = (Valuator *) clientData;
     pd = pv->record;
     dlValuator = pv->dlValuator;
     displayInfo = pv->updateTask->displayInfo;
     localLopr = pd->lopr;
     localHopr = pd->hopr;
     localPrecision = MAX(0,pd->precision);
     localTitle = pv->dlValuator->control.ctrl;

  } else {
/* no controller data, therefore userData = dlValuator */
     XtVaGetValues(w,XmNuserData,&dlValuator,NULL);
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
/* LOPR */   cvtDoubleToString(localLopr,stringValue,localPrecision);
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
/* HOPR */   cvtDoubleToString(localHopr,stringValue,localPrecision);
             if (stringValue != NULL) {
               nChars = strlen(stringValue);
               textWidth = XTextWidth(font,stringValue,nChars);
               startX = MAX(1,useableWidth - textWidth);
               if (dlValuator->label == CHANNEL) {
                /* need room for label above */
                startY = (int) (1.3*(font->ascent + font->descent)
                                + font->ascent);
               } else {
                startY = font->ascent + 3;
               }
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
               startX = MAX(1,useableWidth - textWidth);
               startY = font->ascent + 2;
               XDrawString(display,XtWindow(w),displayInfo->pixmapGC,startX,startY,
                                        localTitle,nChars);
             }
        }
        break;



      case LEFT: case RIGHT:    /* but we know it's really only RIGHT */
	XtVaGetValues(w,XmNscaleHeight,&scaleHeight,NULL);
        useableHeight = dlValuator->object.height - scaleHeight;

        if (dlValuator->label == OUTLINE || dlValuator->label == LIMITS
                                        || dlValuator->label == CHANNEL) {
/* LOPR */   cvtDoubleToString(localLopr,stringValue,localPrecision);
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
/* HOPR */   cvtDoubleToString(localHopr,stringValue,localPrecision);
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
      valuatorRedrawValue(pv,
            pv->updateTask->displayInfo,pv->widget,
            pv->dlValuator, pv->record->value);


    } else {
/* fake data */
      valuatorRedrawValue(NULL,displayInfo,w,dlValuator,0.0);
    }
  }  /* end --  if (dlValuator->label != LABEL_NONE) */


}

/*
 * set value (with implicit redraw of value) for valuator
 */
void valuatorSetValue(Valuator *pv, double forcedValue,
                        Boolean force)
{
  int iValue;
  double dValue;
  Arg args[1];
  Record *pd = pv->record;

  if (pd->hopr != pd->lopr) {
    if (force)
        dValue = forcedValue;
    else
        dValue = pd->value;
#if 0
/* to make reworked event handling for Valuator work */
    pd->value = dValue;
#endif

/* update scale widget */
    iValue = (int) (VALUATOR_MIN + ((dValue - pd->lopr)
                /(pd->hopr - pd->lopr))
                *((double)(VALUATOR_MAX - VALUATOR_MIN)));
    pv->oldIntegerValue = iValue;
    XtVaSetValues(pv->widget,XmNvalue,iValue,NULL);
    valuatorRedrawValue(pv,pv->updateTask->displayInfo,
                        pv->widget,pv->dlValuator,dValue);
  }
}

/*
 * redraw value for valuator
 */
void valuatorRedrawValue(Valuator *pv,
                         DisplayInfo *displayInfo,
                         Widget w,
                         DlValuator *dlValuator,
                         double value)
{
  unsigned long foreground, background;
  Dimension scaleWidth, scaleHeight;
  int useableWidth, useableHeight, textHeight, textWidth, startX, startY;
  int nChars;
  Arg args[4];
  XFontStruct *font;
  char stringValue[40];
  unsigned long gcValueMask;
  XGCValues gcValues;
  int precision;
  int x, y, height, width;

/* return if no window for widget yet, or if displayInfo == NULL, or ... */
  if (XtWindow(w) == (Window)NULL || displayInfo == (DisplayInfo *)NULL ||
      dlValuator == (DlValuator *)NULL) return;

/* simply return if no value to render */
  if (!(dlValuator->label == LIMITS || dlValuator->label == CHANNEL)) return;

  foreground = displayInfo->dlColormap[dlValuator->control.clr];
  background = displayInfo->dlColormap[dlValuator->control.bclr];
  if (pv && (pv->record->precision >= 0)) {
    Record *pd = pv->record;
    precision = pd->precision;
    font = fontTable[pv->fontIndex];
    if (dlValuator->clrmod == ALARM)
      foreground = alarmColorPixel[pv->record->severity];
  } else {
    precision = 0;
    font = fontTable[valuatorFontListIndex(dlValuator)];
  }

  textHeight = font->ascent + font->descent;
  cvtDoubleToString(value,stringValue,precision);
  nChars = strlen(stringValue);

  switch (dlValuator->direction) {
    case UP: case DOWN:
      XtVaGetValues(w, XmNscaleWidth,&scaleWidth, NULL);
      useableWidth = dlValuator->object.width - scaleWidth;
      textWidth = XTextWidth(font,stringValue,nChars);
      startX = MAX(1,useableWidth - textWidth);
      startY = (dlValuator->object.height + font->ascent)/2;

      x =  0;
      y =  MAX(1,startY - font->ascent);
      width = useableWidth;
      height = font->ascent+font->descent;
      break;
    case LEFT: case RIGHT:    /* but we know it's really only RIGHT */
      XtVaGetValues(w,XmNscaleHeight,&scaleHeight,NULL);
      useableHeight = dlValuator->object.height - scaleHeight;
      textWidth = XTextWidth(font,stringValue,nChars);
      startX = dlValuator->object.width/2 - textWidth/2;
      startY = useableHeight - font->descent;

      x = dlValuator->object.width/3,
      y = startY - font->ascent,
      width = dlValuator->object.width/3,
      height = font->ascent+font->descent;
      break;
  }  /* end switch() */

  /* set up the graphic context */
  gcValueMask = GCForeground | GCFont | GCFunction;
  gcValues.function = GXcopy;
  gcValues.foreground = background;
  gcValues.font = font->fid;
  XChangeGC(XtDisplay(w), displayInfo->pixmapGC, gcValueMask, &gcValues);
  /* fill the background */
  XFillRectangle(XtDisplay(w),
                 XtWindow(w),
                 displayInfo->pixmapGC,
                 x,y,
                 width,height);
  /* draw the string */
  XSetForeground(XtDisplay(w),
                 displayInfo->pixmapGC,
                 foreground);
  XDrawString(XtDisplay(w),
              XtWindow(w),
              displayInfo->pixmapGC,
              startX,startY,
              stringValue,nChars);
}

/*
 * thanks to complicated valuator interactions, need to rely on
 *  Key/ButtonRelease events to re-enable updates for dlValuator display
 */
#ifdef __cplusplus
void handleValuatorRelease(
  Widget,
  XtPointer passedData,
  XEvent *event,
  Boolean *)
#else
void handleValuatorRelease(
  Widget w,
  XtPointer passedData,
  XEvent *event,
  Boolean *continueToDispatch)
#endif
{
  Valuator *pv = (Valuator *) passedData;
  DlValuator *dlValuator = pv->dlValuator;

  switch(event->type) {
    case ButtonRelease:
    case KeyRelease:
      dlValuator->enableUpdates = True;
      updateTaskMarkUpdate(pv->updateTask);
      /* don't reset ->dragging: let valuatorValueChanged() do that */
      break;
  }
}

#ifdef __cplusplus
static void destroyDialog(Widget  w, XtPointer, XtPointer)
#else
static void destroyDialog(Widget  w, XtPointer cd, XtPointer cbs)
#endif
{
  XtDestroyWidget(XtParent(w));
}


static void precisionToggleChangedCallback(
  Widget w,
  XtPointer clientData,
  XtPointer cbs)
{
  Widget widget;
  long value;
  Valuator *pv;
  XmToggleButtonCallbackStruct *call_data = (XmToggleButtonCallbackStruct *) cbs;

/* only respond to the button actually set */
  if (call_data->event && call_data->set) {
    value = (long)clientData;
    XtVaGetValues(w,XmNuserData,&pv,NULL);
/*
 * now set the prec field in the valuator data structure, and update
 * the valuator (scale) resources
 */
    if (pv) {
      pv->dlValuator->dPrecision = pow(10.,(double)value);
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
#ifdef __cplusplus
static void precTextFieldActivateCallback(
  Widget w,
  XtPointer cd,
  XtPointer)
#else
static void precTextFieldActivateCallback(
  Widget w,
  XtPointer cd,
  XtPointer cbs)
#endif
{
  DlValuator *dlValuator = (DlValuator *) cd;
  char *stringValue;
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
#ifdef __cplusplus
static void precTextFieldLosingFocusCallback(
  Widget w,
  XtPointer cd,
  XtPointer)
#else
static void precTextFieldLosingFocusCallback(
  Widget w,
  XtPointer cd,
  XtPointer cbs)
#endif
{
  DlValuator *dlValuator = (DlValuator *) cd;
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
  Valuator *pv = (Valuator *) clientData;
  Record *pd = pv->record;
  XmSelectionBoxCallbackStruct *call_data = (XmSelectionBoxCallbackStruct *) callbackStruct;
  double value;
  char *stringValue;

  if (pv == NULL) return;

  XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET, &stringValue);

  if (stringValue != NULL) {
    value = atof(stringValue);

    /* move/redraw valuator & value, but force use of user-selected value */
    if ((pd->connected) && pd->writeAccess) {
      medmSendDouble(pv->record,value);
      valuatorSetValue(pv,value,True);
    }
    XtFree(stringValue);
  }
  XtDestroyWidget(XtParent(w));

}

#ifdef __cplusplus
void popupValuatorKeyboardEntry(
  Widget w,
  DisplayInfo *,
  XEvent *event)
#else
void popupValuatorKeyboardEntry(
  Widget w,
  DisplayInfo *displayInfo,
  XEvent *event)
#endif
{
#define MAX_TOGGLES 20
  Widget keyboardDialog;
  char valueLabel[MAX_TOKEN_LENGTH + 8];
  XmString xmTitle, xmValueLabel, valueXmString;
  char valueString[40];
  char *channel;
  Arg args[8];
  int n;
  Valuator *pv;
  Record   *pd;

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

    XtVaGetValues(w,XmNuserData,&pv,NULL);
    if (pv) {
      pd = pv->record;
      channel = pv->dlValuator->control.ctrl;
      if ((pd->connected) && pd->writeAccess && strlen(channel) > (size_t)0) {
        /* create selection box/prompt dialog */
        strcpy(valueLabel,"VALUE: ");
        strcat(valueLabel,channel);
        xmValueLabel = XmStringCreateSimple(valueLabel);
        xmTitle = XmStringCreateSimple(channel);
        dlValuator = pv->dlValuator;
        cvtDoubleToString(pd->value,valueString,
                        pd->precision);
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

        XtAddCallback(keyboardDialog,XmNokCallback,sendKeyboardValue, pv);
        XtAddCallback(keyboardDialog,XmNcancelCallback,destroyDialog,NULL);

        /* create frame/radiobox/toggles for precision selection */
        hoprLoprAbs = fabs(pd->hopr);
        hoprLoprAbs = MAX(hoprLoprAbs,fabs(pd->lopr));
        /* log10 + 1 */
        numPlusColumns =  (short)log10(hoprLoprAbs) + 1;
        numMinusColumns = (short)pd->precision;
        /* leave room for decimal point */
        numColumns = numPlusColumns + 1 + numMinusColumns;
        if (numColumns > MAX_TOGGLES) {
          numColumns = MAX_TOGGLES;
          if (numMinusColumns < MAX_TOGGLES) {
            numPlusColumns = numColumns - 1 - numMinusColumns;
          } else {
            numMinusColumns = numMinusColumns - numColumns - 1;
          }
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
          XtSetArg(args[2],XmNuserData,(XtPointer) pv);
          if (log10(dlValuator->dPrecision) == (double)i) {
             XtSetArg(args[3],XmNset,True);
          }
          toggles[count++] = XmCreateToggleButton(radioBox,"toggles",args,
                     (log10(dlValuator->dPrecision) == (double)i ? 4 : 3));
          longValue = (long)shortValue;
          XtAddCallback(toggles[count-1],XmNvalueChangedCallback,
                     precisionToggleChangedCallback,(XtPointer)longValue);
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
          XtSetArg(args[2],XmNuserData,(XtPointer) pv);
          if (log10(dlValuator->dPrecision) == (double)-i) {
             XtSetArg(args[3],XmNset,True);
          }
          toggles[count++] = XmCreateToggleButton(radioBox,"toggles",args,
                     (log10(dlValuator->dPrecision) == (double)-i ? 4 : 3));
          longValue = (long)shortValue;
          XtAddCallback(toggles[count-1],XmNvalueChangedCallback,
                     precisionToggleChangedCallback, (XtPointer)longValue);
        }

/* text field */
        n = 0;
        textField = XmCreateTextField(form,"textField",args,n);
        XtAddCallback(textField,XmNactivateCallback,
                      precTextFieldActivateCallback,(XtPointer)dlValuator);
        XtAddCallback(textField,XmNlosingFocusCallback,
                      precTextFieldLosingFocusCallback,(XtPointer)dlValuator);
        XtAddCallback(textField,XmNmodifyVerifyCallback,
                      textFieldFloatVerifyCallback, NULL);
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

/*
 * valuatorValueChanged - drag and value changed callback for valuator
 */

#ifdef __cplusplus
void valuatorValueChanged(
  Widget,
  XtPointer clientData,
  XtPointer callbackStruct)
#else
void valuatorValueChanged(
  Widget  w,
  XtPointer clientData,
  XtPointer callbackStruct)
#endif
{
  Valuator *pv = (Valuator *) clientData;
  Record *pd = pv->record;
  XmScaleCallbackStruct *call_data = (XmScaleCallbackStruct *) callbackStruct;
  DlValuator *dlValuator = (DlValuator *) pv->dlValuator;
  Arg args[3];
  XButtonEvent *buttonEvent;
  XKeyEvent *keyEvent;
  double value;

  if (pd->connected) {

    /* set modified flag on monitor data so that next update traversal will
     * set controller visual state correctly (noting that for controllers
     * as monitors the ->modified flag alone is used to do updates
     */

    value = pd->value;
    if (call_data->reason == XmCR_DRAG) {
      dlValuator->dragging = True;            /* mark beginning of drag  */
      dlValuator->enableUpdates = False;      /* disable updates in drag */

      /* drag - set value based on relative position (easy) */
      pv->oldIntegerValue = call_data->value;
      value = pd->lopr + ((double)(call_data->value - VALUATOR_MIN))
                /((double)(VALUATOR_MAX - VALUATOR_MIN) )*(pd->hopr - pd->lopr);

    } else 
    if (call_data->reason == XmCR_VALUE_CHANGED) { 
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
            if (pv->oldIntegerValue > call_data->value) {
              /* decrease value one 10*precision value */
              value = MAX(pd->lopr, pd->value - 10.*dlValuator->dPrecision);
            } else 
            if (pv->oldIntegerValue < call_data->value) {
              /* increase value one 10*precision value */
              value = MIN(pd->hopr, pd->value + 10.*dlValuator->dPrecision);
            }
          } else {
            /* single increment (precision) */
            if (pv->oldIntegerValue > call_data->value) {
              /* decrease value one precision value */
              value = MAX(pd->lopr, pd->value - dlValuator->dPrecision);
            } else
            if (pv->oldIntegerValue < call_data->value) {
              /* increase value one precision value */
              value = MIN(pd->hopr, pd->value + dlValuator->dPrecision);
            }
          }
        } else
        if (call_data->event->type == ButtonPress) {
          buttonEvent = (XButtonEvent *)call_data->event;
          if (buttonEvent->state & ControlMask) {
            /* handle this as multiple increment/decrement */
            if (call_data->value - pv->oldIntegerValue < 0) {
              /* decrease value one 10*precision value */
              value = MAX(pd->lopr, pd->value - 10.*dlValuator->dPrecision);
            } else
            if (call_data->value - pv->oldIntegerValue > 0) {
              /* increase value one 10*precision value */
              value = MIN(pd->hopr, pd->value + 10.*dlValuator->dPrecision);
            }
          } else {
            /* single increment (precision) */
            if (pv->oldIntegerValue > call_data->value) {
              /* decrease value one precision value */
              value = MAX(pd->lopr, pd->value - dlValuator->dPrecision);
            } else
            if (pv->oldIntegerValue < call_data->value) {
              /* increase value one precision value */
              value = MIN(pd->hopr, pd->value + dlValuator->dPrecision);
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
        if (call_data->value - pv->oldIntegerValue < 0) {
          /* decrease value one precision value */
          value = MAX(pd->lopr, pd->value - dlValuator->dPrecision);
        } else
        if (call_data->value - pv->oldIntegerValue > 0) {
          /* increase value one precision value */
          value = MIN(pd->hopr, pd->value + dlValuator->dPrecision);
        }
      }  /* end if (call_data->event != NULL) */
    }

    if (pd->writeAccess) {
      medmSendDouble(pv->record,value);
      /* move/redraw valuator & value, but force use of user-selected value */
      valuatorSetValue(pv,value,True);
    } else {
      fprintf(stderr,"\a");
      valuatorSetValue(pv,pd->value,True);
    }
  }
}

static void valuatorName(XtPointer cd, char **name, short *severity, int *count) {
  Valuator *pv = (Valuator *) cd;
  *count = 1;
  name[0] = pv->record->name;
  severity[0] = pv->record->severity;
}

