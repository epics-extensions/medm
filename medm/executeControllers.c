
#include "medm.h"

extern XtEventHandler handleValuatorRelease(Widget, XtPointer, XEvent *);


int textFieldFontListIndex(int height)
{
  int i, index;
/* don't allow height of font to exceed 90% - 4 pixels of textField widget
 *	(includes nominal 2*shadowThickness=2 shadow)
 */
  for (i = MAX_FONTS-1; i >=  0; i--) {
    if ( ((int)(.90*height) - 4) >=
			(fontTable[i]->ascent + fontTable[i]->descent))
	return(i);
  }
  return (0);
}

int messageButtonFontListIndex(int height)
{
  int i, index;
/* don't allow height of font to exceed 90% - 4 pixels of messageButton widget
 *	(includes nominal 2*shadowThickness=2 shadow)
 */
  for (i = MAX_FONTS-1; i >=  0; i--) {
    if ( ((int)(.90*height) - 4) >= 
			(fontTable[i]->ascent + fontTable[i]->descent))
	return(i);
  }
  return (0);
}


int menuFontListIndex(int height)
{
  int i, index;
/* don't allow height of font to exceed 90% - 4 pixels of messageButton widget
 *	(includes nominal 2*shadowThickness=2 shadow)
 */
  for (i = MAX_FONTS-1; i >=  0; i--) {
    if ( ((int)(.90*height) - 4) >= 
			(fontTable[i]->ascent + fontTable[i]->descent))
	return(i);
  }
  return (0);
}


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

int choiceButtonFontListIndex(
  DlChoiceButton *dlChoiceButton,
  int numButtons,
  int maxChars)
{
  int i, index, useNumButtons;
  short sqrtNumButtons;

#define SHADOWS_SIZE 4		/* each Toggle Button has 2 shadows...*/

/* pay cost of sqrt() and ceil() once */
  sqrtNumButtons = (short) ceil(sqrt((double)numButtons));
  sqrtNumButtons = MAX(1,sqrtNumButtons);
  useNumButtons = MAX(1,numButtons);


/* more complicated calculation based on orientation, etc */
  for (i = MAX_FONTS-1; i >=  0; i--) {
    switch (dlChoiceButton->stacking) {
	case ROW:
	   if ( (int)(dlChoiceButton->object.height/useNumButtons 
					- SHADOWS_SIZE) >=
			(fontTable[i]->ascent + fontTable[i]->descent))
			return(i);
	   break;
	case ROW_COLUMN:
	   if ( (int)(dlChoiceButton->object.height/sqrtNumButtons
					- SHADOWS_SIZE) >=
			(fontTable[i]->ascent + fontTable[i]->descent))
			return(i);
	   break;
	case COLUMN:
	   if ( (int)(dlChoiceButton->object.height - SHADOWS_SIZE) >=
			(fontTable[i]->ascent + fontTable[i]->descent))
			return(i);
	   break;
    }
  }
  return (0);
}



/***
 *** Choice Button
 ***/

void executeDlChoiceButton(DisplayInfo *displayInfo,
		DlChoiceButton *dlChoiceButton, Boolean dummy)
{
  ChannelAccessControllerData *channelAccessControllerData;
  Arg args[24];
  int i, n;
  Widget localWidget;
  XmString buttons[2];
  XmButtonType buttonType[2];
  WidgetList children;
  Cardinal numChildren;
  int usedWidth, usedHeight;
  XmFontList fontList;
  short sqrtNumButtons;


 displayInfo->useDynamicAttribute = FALSE;

 if (displayInfo->traversalMode == DL_EXECUTE) {

/* add in as controller */
    channelAccessControllerData = (ChannelAccessControllerData *) 
	malloc(sizeof(ChannelAccessControllerData));
    channelAccessControllerData->connected = FALSE;
    channelAccessControllerData->self = NULL;
    channelAccessControllerData->chid = NULL;
    channelAccessControllerData->controllerType = DL_ChoiceButton;
    SEVCHK(ca_build_and_connect(dlChoiceButton->control.ctrl,TYPENOTCONN,0,
	&(channelAccessControllerData->chid),NULL,
	processControllerConnectionEvent,NULL),
       "executeDlChoiceButton: error in CA_BUILD_AND_CONNECT for Controller");
    if (channelAccessControllerData->chid != NULL)
        ca_puser(channelAccessControllerData->chid)=channelAccessControllerData;

/* also add in as monitor */
    channelAccessControllerData->monitorData = 
	allocateChannelAccessMonitorData(displayInfo);

    channelAccessControllerData->monitorData->monitorType = DL_ChoiceButton;
    channelAccessControllerData->monitorData->specifics =
	(XtPointer) dlChoiceButton;
    channelAccessControllerData->monitorData->clrmod = dlChoiceButton->clrmod;
    channelAccessControllerData->monitorData->controllerData = channelAccessControllerData;


/* put up white rectangle so that unconnected channels are obvious */
    XSetForeground(display,displayInfo->pixmapGC,WhitePixel(display,screenNum));
    XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
	dlChoiceButton->object.x,dlChoiceButton->object.y,
	dlChoiceButton->object.width,dlChoiceButton->object.height);
    XSetForeground(display,displayInfo->gc,WhitePixel(display,screenNum));
    XFillRectangle(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	dlChoiceButton->object.x,dlChoiceButton->object.y,
	dlChoiceButton->object.width,dlChoiceButton->object.height);


    SEVCHK(CA_BUILD_AND_CONNECT(dlChoiceButton->control.ctrl,TYPENOTCONN,0,
	&(channelAccessControllerData->monitorData->chid),
	NULL,processMonitorConnectionEvent, NULL),
	"executeDlChoiceButton: error in CA_BUILD_AND_CONNECT for Monitor");
    if (channelAccessControllerData->monitorData->chid != NULL)
        ca_puser(channelAccessControllerData->monitorData->chid)
				= channelAccessControllerData->monitorData;

/* choice button must be created after channel information has been returned
 *   see caDM.c - processControllerGrGetCallback() */

  } else if (displayInfo->traversalMode == DL_EDIT) {

/* from the choice button structure, we've got ChoiceButton's specifics */
    buttons[0] = XmStringCreateSimple("0...");
    buttons[1] = XmStringCreateSimple("1...");
    buttonType[0] = XmRADIOBUTTON;
    buttonType[1] = XmRADIOBUTTON;
    n = 0;
    XtSetArg(args[n],XmNx,(Position)dlChoiceButton->object.x); n++;
    XtSetArg(args[n],XmNy,(Position)dlChoiceButton->object.y); n++;
    XtSetArg(args[n],XmNwidth,(Dimension)dlChoiceButton->object.width); n++;
    XtSetArg(args[n],XmNheight,(Dimension)dlChoiceButton->object.height); n++;
    XtSetArg(args[n],XmNforeground,(Pixel)
	displayInfo->dlColormap[dlChoiceButton->control.clr]); n++;
    XtSetArg(args[n],XmNbackground,(Pixel)
	displayInfo->dlColormap[dlChoiceButton->control.bclr]); n++;
    XtSetArg(args[n],XmNindicatorOn,(Boolean)FALSE); n++;
    XtSetArg(args[n],XmNmarginWidth,0); n++;
    XtSetArg(args[n],XmNmarginHeight,0); n++;
    XtSetArg(args[n],XmNrecomputeSize,(Boolean)FALSE); n++;
    XtSetArg(args[n],XmNresizeWidth,FALSE); n++;
    XtSetArg(args[n],XmNresizeHeight,FALSE); n++;
    XtSetArg(args[n],XmNspacing,0); n++;
    XtSetArg(args[n],XmNbuttonCount,2); n++;
    XtSetArg(args[n],XmNbuttonType,buttonType); n++;
    XtSetArg(args[n],XmNbuttons,buttons); n++;
    XtSetArg(args[n],XmNbuttonSet,0); n++;
    switch (dlChoiceButton->stacking) {
/* modelling what caDM does for 2 buttons (stateStrings) */
	case COLUMN:
	   usedWidth = dlChoiceButton->object.width/2;
	   usedHeight = dlChoiceButton->object.height;
	   XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
	   break;
	case ROW:
	   usedWidth = dlChoiceButton->object.width;
	   usedHeight = dlChoiceButton->object.height/2;
	   XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
	   break;

	case ROW_COLUMN:
	   usedWidth = dlChoiceButton->object.width/2;
	   usedHeight = dlChoiceButton->object.height/2;
	   XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
	   XtSetArg(args[n],XmNnumColumns,2); n++;
	   break;
	default:
	   fprintf(stderr,
              "\nexecuteDlChoiceButton: unknown stacking = %d",
               dlChoiceButton->stacking);
	   break;
    }

/*** (MDA)  ChoiceButton is really a radio box and push button children ***/
    localWidget = XmCreateSimpleRadioBox(displayInfo->drawingArea,
		"radioBox", args, n);
/* remove all translations if in edit mode */
    XtUninstallTranslations(localWidget);

/* now make push-in type radio box...*/
    XtVaGetValues(localWidget,XmNchildren,&children,XmNnumChildren,&numChildren,
		NULL);

    fontList = fontListTable[choiceButtonFontListIndex(dlChoiceButton,2,4)];
    for (i = 0; i < numChildren; i++) {
	XtUninstallTranslations(children[i]);
	XtVaSetValues(children[i],XmNindicatorOn,False,XmNshadowThickness,2,
		XmNhighlightThickness,1,XmNfontList, fontList,
		XmNwidth, (Dimension)usedWidth,
		XmNheight, (Dimension)usedHeight,
		XmNrecomputeSize, (Boolean)FALSE,
		NULL);
    }

    if (displayInfo->traversalMode == DL_EDIT) XtManageChild(localWidget);
    displayInfo->child[displayInfo->childCount++] = localWidget;

    XmStringFree(buttons[0]);
    XmStringFree(buttons[1]);

/* add button press handlers for editing */
    XtAddEventHandler(localWidget, ButtonPressMask, False,
	(XtEventHandler)handleButtonPress,displayInfo);
  }
}


/***
 *** Message Button
 ***/

void executeDlMessageButton(DisplayInfo *displayInfo,
		DlMessageButton *dlMessageButton, Boolean dummy)
{
  ChannelAccessControllerData *channelAccessControllerData;
  XmString xmString;
  Arg args[20];
  int n;
  Widget localWidget;


  displayInfo->useDynamicAttribute = FALSE;
  
  if (displayInfo->traversalMode == DL_EXECUTE) {

/* add in as controller */
    channelAccessControllerData = (ChannelAccessControllerData *) 
	malloc(sizeof(ChannelAccessControllerData));
    channelAccessControllerData->connected = FALSE;
    channelAccessControllerData->self = NULL;
    channelAccessControllerData->chid = NULL;
    channelAccessControllerData->controllerType = DL_MessageButton;
    SEVCHK(CA_BUILD_AND_CONNECT(dlMessageButton->control.ctrl,TYPENOTCONN,0,
	&(channelAccessControllerData->chid),NULL,
	processControllerConnectionEvent,NULL),
      "executeDlMessageButton: error in CA_BUILD_AND_CONNECT for Controller");
    if (channelAccessControllerData->chid != NULL)
        ca_puser(channelAccessControllerData->chid)=channelAccessControllerData;

/* also add in as monitor */
    channelAccessControllerData->monitorData = 
	allocateChannelAccessMonitorData(displayInfo);

    channelAccessControllerData->monitorData->monitorType = DL_MessageButton;
    channelAccessControllerData->monitorData->specifics =
			(XtPointer)dlMessageButton;
    channelAccessControllerData->monitorData->clrmod = dlMessageButton->clrmod;

/* give the monitorData structure access to controllerData for cleanup */
    channelAccessControllerData->monitorData->controllerData = channelAccessControllerData;

/* put up white rectangle so that unconnected channels are obvious */
    XSetForeground(display,displayInfo->pixmapGC,WhitePixel(display,screenNum));
    XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
	dlMessageButton->object.x,dlMessageButton->object.y,
	dlMessageButton->object.width,dlMessageButton->object.height);
    XSetForeground(display,displayInfo->gc,WhitePixel(display,screenNum));
    XFillRectangle(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	dlMessageButton->object.x,dlMessageButton->object.y,
	dlMessageButton->object.width,dlMessageButton->object.height);

    SEVCHK(CA_BUILD_AND_CONNECT(dlMessageButton->control.ctrl,TYPENOTCONN,0,
	&(channelAccessControllerData->monitorData->chid),
	NULL,processMonitorConnectionEvent,NULL),
	"executeDlButton: error in CA_BUILD_AND_CONNECT for Monitor");
    if (channelAccessControllerData->monitorData->chid != NULL)
        ca_puser(channelAccessControllerData->monitorData->chid)
				= channelAccessControllerData->monitorData;
  }


/* from the message button structure, we've got MessageButton's specifics */
  xmString = XmStringCreateSimple(dlMessageButton->label);
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlMessageButton->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlMessageButton->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlMessageButton->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlMessageButton->object.height); n++;
  XtSetArg(args[n],XmNforeground,(Pixel)
	displayInfo->dlColormap[dlMessageButton->control.clr]); n++;
  XtSetArg(args[n],XmNbackground,(Pixel)
	displayInfo->dlColormap[dlMessageButton->control.bclr]); n++;
  XtSetArg(args[n],XmNhighlightThickness,1); n++;
  XtSetArg(args[n],XmNhighlightOnEnter,TRUE); n++;
  XtSetArg(args[n],XmNindicatorOn,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNrecomputeSize,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNlabelString, xmString); n++;
  XtSetArg(args[n],XmNlabelType, XmSTRING); n++;
  XtSetArg(args[n],XmNfontList,fontListTable[
	messageButtonFontListIndex(dlMessageButton->object.height)]); n++;
  localWidget = XtCreateWidget("messageButton", 
		xmPushButtonWidgetClass, displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] = localWidget;

  XmStringFree(xmString);

  if (displayInfo->traversalMode == DL_EXECUTE) {

    channelAccessControllerData->self = localWidget;
    channelAccessControllerData->monitorData->self = 
	channelAccessControllerData->self;

/* add in drag/drop translations */
    XtOverrideTranslations(localWidget,parsedTranslations);

/* add the callbacks for update */
    XtAddCallback(localWidget,
	XmNarmCallback,(XtCallbackProc)controllerValueChanged,
	(XtPointer)channelAccessControllerData);
    XtAddCallback(localWidget,
	XmNdisarmCallback, (XtCallbackProc)controllerValueChanged,
	(XtPointer)channelAccessControllerData);

  } else if (displayInfo->traversalMode == DL_EDIT) {

/* remove all translations if in edit mode */
    XtUninstallTranslations(localWidget);
/*
 * add button press handlers too
 */
    XtAddEventHandler(localWidget,
	ButtonPressMask, False, (XtEventHandler)handleButtonPress,
	(XtPointer)displayInfo);
    XtManageChild(localWidget);
  }
}


/***
 *** Valuator (Scale)
 ***/


void executeDlValuator(DisplayInfo *displayInfo, DlValuator *dlValuator,
			Boolean dummy)
{
  ChannelAccessControllerData *channelAccessControllerData = NULL;
  Arg args[25];
  int i, n, heightDivisor, scalePopupBorder;
  Widget localWidget;
  WidgetList children;
  Cardinal numChildren;


  displayInfo->useDynamicAttribute = FALSE;
  
  if (displayInfo->traversalMode == DL_EXECUTE) {

/* add in as controller */
    channelAccessControllerData = (ChannelAccessControllerData *) 
	malloc(sizeof(ChannelAccessControllerData));
    channelAccessControllerData->connected = FALSE;
    channelAccessControllerData->self = NULL;
    channelAccessControllerData->chid = NULL;
    channelAccessControllerData->controllerType = DL_Valuator;
    SEVCHK(CA_BUILD_AND_CONNECT(dlValuator->control.ctrl,TYPENOTCONN,0,
	&(channelAccessControllerData->chid),NULL,
	processControllerConnectionEvent,NULL),
	"executeDlValuator: error in CA_BUILD_AND_CONNECT for Controller");
    if (channelAccessControllerData->chid != NULL)
        ca_puser(channelAccessControllerData->chid)=channelAccessControllerData;

/* also add in as monitor */
    channelAccessControllerData->monitorData = 
	allocateChannelAccessMonitorData(displayInfo);

    channelAccessControllerData->monitorData->monitorType = DL_Valuator;
    channelAccessControllerData->monitorData->specifics=(XtPointer)dlValuator;
    channelAccessControllerData->monitorData->clrmod = dlValuator->clrmod;
    channelAccessControllerData->monitorData->label = dlValuator->label;

/* valuator monitor needs access to controller stuff to make reworked (mangled)
 *   event handling work/actions  */
    channelAccessControllerData->monitorData->controllerData = channelAccessControllerData;

/* put up white rectangle so that unconnected channels are obvious */
    XSetForeground(display,displayInfo->pixmapGC,WhitePixel(display,screenNum));
    XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
	dlValuator->object.x,dlValuator->object.y,
	dlValuator->object.width,dlValuator->object.height);
    XSetForeground(display,displayInfo->gc,WhitePixel(display,screenNum));
    XFillRectangle(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	dlValuator->object.x,dlValuator->object.y,
	dlValuator->object.width,dlValuator->object.height);

    SEVCHK(CA_BUILD_AND_CONNECT(dlValuator->control.ctrl,TYPENOTCONN,0,
	&(channelAccessControllerData->monitorData->chid),
	NULL,processMonitorConnectionEvent, NULL),
	"executeDlValuator: error in CA_BUILD_AND_CONNECT for Monitor");
    if (channelAccessControllerData->monitorData->chid != NULL)
        ca_puser(channelAccessControllerData->monitorData->chid)
				= channelAccessControllerData->monitorData;
  }

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
  XtSetArg(args[n],XmNuserData,(XtPointer)channelAccessControllerData); n++;
  XtSetArg(args[n],XmNfontList,fontListTable[
		valuatorFontListIndex(dlValuator)]); n++;
  localWidget =  XtCreateWidget("valuator", 
		xmScaleWidgetClass, displayInfo->drawingArea, args, n);

  displayInfo->child[displayInfo->childCount++] = localWidget;

/* get children of scale */
  XtVaGetValues(localWidget,XmNnumChildren,&numChildren,
				XmNchildren,&children,NULL);

  if (displayInfo->traversalMode == DL_EXECUTE) {

/* set virtual range */
    n = 0;
    XtSetArg(args[n],XmNminimum,VALUATOR_MIN); n++;
    XtSetArg(args[n],XmNmaximum,VALUATOR_MAX); n++;
    XtSetArg(args[n],XmNscaleMultiple,VALUATOR_MULTIPLE_INCREMENT); n++;
    XtSetValues(localWidget,args,n);
/* add in drag/drop translations */
    XtOverrideTranslations(localWidget,parsedTranslations);

  /* change translations for scrollbar child of valuator */
    for (i = 0; i < numChildren; i++) {
      if (XtClass(children[i]) == xmScrollBarWidgetClass) {

	  XtOverrideTranslations(children[i],parsedTranslations);

	/* add event handler for Key/ButtonRelease which enables updates */
	  XtAddEventHandler(children[i],KeyReleaseMask|ButtonReleaseMask,
		False,(XtEventHandler)handleValuatorRelease,
		(XtPointer)channelAccessControllerData);
      }
    }


/* record the widget that this structure belongs to */
    channelAccessControllerData->self = localWidget;
    channelAccessControllerData->monitorData->self = 
	channelAccessControllerData->self;

/* add the callbacks for update */
    XtAddCallback(localWidget, XmNvalueChangedCallback,
		(XtCallbackProc)valuatorValueChanged,
		(XtPointer)channelAccessControllerData);
    XtAddCallback(localWidget, XmNdragCallback,
		(XtCallbackProc)valuatorValueChanged,
		(XtPointer)channelAccessControllerData);

/* add event handler for expose - forcing display of min/max and value
 *	in own format
 */
  XtAddEventHandler(localWidget,ExposureMask,
	False,(XtEventHandler)handleValuatorExpose,
	(XtPointer)channelAccessControllerData);

/* add event handler for Key/ButtonRelease which enables updates */
  XtAddEventHandler(localWidget,KeyReleaseMask|ButtonReleaseMask,
	False,(XtEventHandler)handleValuatorRelease,
	(XtPointer)channelAccessControllerData);


  } else if (displayInfo->traversalMode == DL_EDIT) {

/* remove all translations if in edit mode */
    XtUninstallTranslations(localWidget);
/* remove translations for children of valuator */
    for (i = 0; i < numChildren; i++) XtUninstallTranslations(children[i]);

/*
 * add button press handlers too
 */
    XtAddEventHandler(localWidget,
	ButtonPressMask, False, (XtEventHandler)handleButtonPress,
	(XtPointer)displayInfo);

/* if in EDIT mode, add dlValuator as userData, and pass NULL in expose */
    n = 0;
    XtSetArg(args[n],XmNuserData,(XtPointer)dlValuator); n++;
    XtSetValues(localWidget,args,n);
/* add event handler for expose - forcing display of min/max and value
 *	in own format
 */
    XtAddEventHandler(localWidget,ExposureMask,False,
	(XtEventHandler)handleValuatorExpose,(XtPointer)NULL);

    XtManageChild(localWidget);

  }

}







/***
 *** Text Entry
 ***/

void executeDlTextEntry(DisplayInfo *displayInfo, DlTextEntry *dlTextEntry,
				Boolean dummy)
{
  ChannelAccessControllerData *channelAccessControllerData;
  Arg args[20];
  int n;
  Widget localWidget;


  displayInfo->useDynamicAttribute = FALSE;
  
  if (displayInfo->traversalMode == DL_EXECUTE) {

/* add in as controller */
    channelAccessControllerData = (ChannelAccessControllerData *) 
	malloc(sizeof(ChannelAccessControllerData));
    channelAccessControllerData->connected = FALSE;
    channelAccessControllerData->self = NULL;
    channelAccessControllerData->chid = NULL;
    channelAccessControllerData->controllerType = DL_TextEntry;
    SEVCHK(CA_BUILD_AND_CONNECT(dlTextEntry->control.ctrl,TYPENOTCONN,0,
	&(channelAccessControllerData->chid),NULL,
	processControllerConnectionEvent,NULL),
	"executeDlTextEntry: error in CA_BUILD_AND_CONNECT for Controller");
    if (channelAccessControllerData->chid != NULL)
        ca_puser(channelAccessControllerData->chid)=channelAccessControllerData;

/* also add in as monitor */
    channelAccessControllerData->monitorData = 
	allocateChannelAccessMonitorData(displayInfo);

    channelAccessControllerData->monitorData->monitorType = DL_TextEntry;
    channelAccessControllerData->monitorData->specifics=(XtPointer)dlTextEntry;
    channelAccessControllerData->monitorData->clrmod = dlTextEntry->clrmod;
    channelAccessControllerData->monitorData->controllerData = channelAccessControllerData;

/* put up white rectangle so that unconnected channels are obvious */
    XSetForeground(display,displayInfo->pixmapGC,WhitePixel(display,screenNum));
    XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
	dlTextEntry->object.x,dlTextEntry->object.y,
	dlTextEntry->object.width,dlTextEntry->object.height);
    XSetForeground(display,displayInfo->gc,WhitePixel(display,screenNum));
    XFillRectangle(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	dlTextEntry->object.x,dlTextEntry->object.y,
	dlTextEntry->object.width,dlTextEntry->object.height);


    SEVCHK(CA_BUILD_AND_CONNECT(dlTextEntry->control.ctrl,TYPENOTCONN,0,
	&(channelAccessControllerData->monitorData->chid),
	NULL,processMonitorConnectionEvent,NULL),
	"executeDlTextEntry: error in CA_BUILD_AND_CONNECT for Monitor");
    if (channelAccessControllerData->monitorData->chid != NULL)
        ca_puser(channelAccessControllerData->monitorData->chid)
				= channelAccessControllerData->monitorData;
  }


/* from the text entry structure, we've got TextEntry's specifics */
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlTextEntry->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlTextEntry->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlTextEntry->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlTextEntry->object.height); n++;
  XtSetArg(args[n],XmNforeground,(Pixel)
	displayInfo->dlColormap[dlTextEntry->control.clr]); n++;
  XtSetArg(args[n],XmNbackground,(Pixel)
	displayInfo->dlColormap[dlTextEntry->control.bclr]); n++;
  XtSetArg(args[n],XmNhighlightThickness,1); n++;
  XtSetArg(args[n],XmNhighlightOnEnter,TRUE); n++;
  XtSetArg(args[n],XmNindicatorOn,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNresizeWidth,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNmarginWidth,
	( (dlTextEntry->object.height <= 2*GOOD_MARGIN_DIVISOR)
	?  0 : (dlTextEntry->object.height/GOOD_MARGIN_DIVISOR)) ); n++;
  XtSetArg(args[n],XmNmarginHeight,
	( (dlTextEntry->object.height <= 2*GOOD_MARGIN_DIVISOR) 
	?  0 : (dlTextEntry->object.height/GOOD_MARGIN_DIVISOR)) ); n++;
  XtSetArg(args[n],XmNfontList,fontListTable[
	textFieldFontListIndex(dlTextEntry->object.height)]); n++;
  localWidget = XtCreateWidget("textField", 
		xmTextFieldWidgetClass, displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] =  localWidget;

  if (displayInfo->traversalMode == DL_EXECUTE) {

    channelAccessControllerData->self =  localWidget;
    channelAccessControllerData->monitorData->self = 
	channelAccessControllerData->self;

/* add in drag/drop translations */
    XtOverrideTranslations(localWidget,parsedTranslations);
	
/* add the callbacks for update */
    XtAddCallback(localWidget,XmNactivateCallback,
	(XtCallbackProc)controllerValueChanged,
	(XtPointer)channelAccessControllerData);

/* special stuff: if user started entering new data into text field, but
 *  doesn't do the actual Activate <CR>, then restore old value on
 *  losing focus...
 */
    XtAddCallback(localWidget,XmNmodifyVerifyCallback,
	(XtCallbackProc)textEntryModifyVerifyCallback,
	(XtPointer)channelAccessControllerData);

  } else if (displayInfo->traversalMode == DL_EDIT) {

/* remove all translations if in edit mode */
    XtUninstallTranslations(localWidget);
/*
 * add button press handlers too
 */
    XtAddEventHandler(localWidget,
	ButtonPressMask, False, (XtEventHandler)handleButtonPress,
	(XtPointer)displayInfo);

    XtManageChild(localWidget);
  }
}



/***
 *** Menu
 ***/

void executeDlMenu(DisplayInfo *displayInfo, DlMenu *dlMenu, Boolean dummy)
{
  ChannelAccessControllerData *channelAccessControllerData;
  Arg args[15];
  XmString buttons[1];
  XmButtonType buttonType[1];
  int i, n;
  Widget localWidget, optionButtonGadget, menu;
  WidgetList children;
  Cardinal numChildren;
  XmFontList fontList;
  Dimension useableWidth;


  displayInfo->useDynamicAttribute = FALSE;
  
  if (displayInfo->traversalMode == DL_EXECUTE) {

/* add in as controller */
    channelAccessControllerData = (ChannelAccessControllerData *) 
	malloc(sizeof(ChannelAccessControllerData));
    channelAccessControllerData->connected = FALSE;
    channelAccessControllerData->self = NULL;
    channelAccessControllerData->chid = NULL;
    channelAccessControllerData->controllerType = DL_Menu;
    SEVCHK(CA_BUILD_AND_CONNECT(dlMenu->control.ctrl,TYPENOTCONN,0,
	&(channelAccessControllerData->chid),NULL,
	processControllerConnectionEvent,NULL),
	"executeDlMenu: error in CA_BUILD_AND_CONNECT for Controller");
    if (channelAccessControllerData->chid != NULL)
        ca_puser(channelAccessControllerData->chid)=channelAccessControllerData;

/* also add in as monitor */
    channelAccessControllerData->monitorData = 
	allocateChannelAccessMonitorData(displayInfo);

    channelAccessControllerData->monitorData->monitorType = DL_Menu;
    channelAccessControllerData->monitorData->specifics=(XtPointer)dlMenu;
    channelAccessControllerData->monitorData->clrmod = dlMenu->clrmod;
    channelAccessControllerData->monitorData->controllerData = channelAccessControllerData;

/* put up white rectangle so that unconnected channels are obvious */
    XSetForeground(display,displayInfo->pixmapGC,WhitePixel(display,screenNum));
    XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
	dlMenu->object.x,dlMenu->object.y,
	dlMenu->object.width,dlMenu->object.height);
    XSetForeground(display,displayInfo->gc,WhitePixel(display,screenNum));
    XFillRectangle(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	dlMenu->object.x,dlMenu->object.y,
	dlMenu->object.width,dlMenu->object.height);


    SEVCHK(CA_BUILD_AND_CONNECT(dlMenu->control.ctrl,TYPENOTCONN,0,
	&(channelAccessControllerData->monitorData->chid),
	NULL,processMonitorConnectionEvent,NULL),
	"executeDlMenu: error in CA_BUILD_AND_CONNECT for Monitor");
    if (channelAccessControllerData->monitorData->chid != NULL)
        ca_puser(channelAccessControllerData->monitorData->chid)
				= channelAccessControllerData->monitorData;

/* option menu must be created after channel information has been returned
 *   see caDM.c - processControllerGrGetCallback() */

  } else if (displayInfo->traversalMode == DL_EDIT) {

/* (MDA) since ca callbacks never called in edit mode, put up a dummy option
 * menu */
    buttons[0] = XmStringCreateSimple("menu");
    buttonType[0] = XmPUSHBUTTON;
/* from the menu structure, we've got Menu's specifics */
/*
 * take a guess here  - keep this constant the same is in caDM.c
 *	this takes out the extra space needed for the cascade pixmap, etc
 */
#define OPTION_MENU_SUBTRACTIVE_WIDTH 23
    if (dlMenu->object.width > OPTION_MENU_SUBTRACTIVE_WIDTH)
	useableWidth = (Dimension) (dlMenu->object.width
		- OPTION_MENU_SUBTRACTIVE_WIDTH);
    else
	useableWidth = (Dimension) dlMenu->object.width;
    n = 0;
    XtSetArg(args[n],XmNx,(Position)dlMenu->object.x); n++;
    XtSetArg(args[n],XmNy,(Position)dlMenu->object.y); n++;
    XtSetArg(args[n],XmNwidth,(Dimension)useableWidth); n++;
    XtSetArg(args[n],XmNheight,(Dimension)dlMenu->object.height); n++;
    XtSetArg(args[n],XmNforeground,(Pixel)
                displayInfo->dlColormap[dlMenu->control.clr]); n++;
    XtSetArg(args[n],XmNbackground,(Pixel)
                displayInfo->dlColormap[dlMenu->control.bclr]); n++;
    XtSetArg(args[n],XmNbuttonCount,1); n++;
    XtSetArg(args[n],XmNbuttonType,buttonType); n++;
    XtSetArg(args[n],XmNbuttons,buttons); n++;
    XtSetArg(args[n],XmNresizeWidth,FALSE); n++;
    XtSetArg(args[n],XmNresizeHeight,FALSE); n++;
    XtSetArg(args[n],XmNrecomputeSize,FALSE); n++;
    XtSetArg(args[n],XmNmarginWidth,0); n++;
    XtSetArg(args[n],XmNmarginHeight,0); n++;
    fontList = fontListTable[menuFontListIndex(dlMenu->object.height)];
    XtSetArg(args[n],XmNfontList,fontList); n++;
    localWidget = XmCreateSimpleOptionMenu(displayInfo->drawingArea,
			"menu",args,n);

/* resize children */
    XtVaGetValues(localWidget,XmNsubMenuId,&menu,NULL);
    XtVaGetValues(menu,XmNnumChildren,&numChildren,XmNchildren,&children,NULL);
    for (i = 0; i < numChildren; i++) {
	XtUninstallTranslations(children[i]);
	XtVaSetValues(children[i], XmNfontList, fontList,
		XmNwidth, (Dimension) useableWidth,
		XmNheight, (Dimension)dlMenu->object.height,
		NULL);
	XtResizeWidget(children[i],useableWidth,dlMenu->object.height,0);

    }
    optionButtonGadget = XmOptionButtonGadget(localWidget);
    XtVaSetValues(optionButtonGadget, XmNfontList, fontList,
		XmNwidth, (Dimension)useableWidth,
		XmNheight, (Dimension)dlMenu->object.height,
		NULL);
	XtResizeWidget(optionButtonGadget,useableWidth,
		dlMenu->object.height,0);


    XmStringFree(buttons[0]);
/* unmanage the label in the option menu */
    XtUnmanageChild(XmOptionLabelGadget(localWidget));
    XtManageChild(localWidget);
    displayInfo->child[displayInfo->childCount++] =  localWidget;

/* remove all translations if in edit mode */
    XtUninstallTranslations(localWidget);

/* add button press handler */
    XtAddEventHandler(localWidget, ButtonPressMask, False,
			(XtEventHandler)handleButtonPress,
			(XtPointer)displayInfo);

  }
}

