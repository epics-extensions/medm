
#include "medm.h"

#include <limits.h>


/* for modal dialogs - defined in utils.c and referenced here in callbacks.c */
extern Boolean modalGrab;
extern Widget closeQD;
extern Widget mainMW;

extern char *stripChartWidgetName;



/*
 * callbacks
 */

static XtCallbackProc shellCommandCallback(
  Widget w,
  XtPointer client_data,
  XmSelectionBoxCallbackStruct *call_data)
{
  char *command;
  DisplayInfo *displayInfo;
  char processedCommand[2*MAX_TOKEN_LENGTH];

  Widget realParent = (Widget)client_data;

  displayInfo = dmGetDisplayInfoFromWidget(realParent);

/* CANCEL */
  if (call_data->reason == XmCR_CANCEL) {
    XtUnmanageChild(displayInfo->shellCommandPromptD);
    return;
  }

/* OK */
  XmStringGetLtoR(call_data->value,XmSTRING_DEFAULT_CHARSET,&command);

  /* (MDA) NB: system() blocks! need to background (&) to not block */

  if (command != NULL) {
    performMacroSubstitutions(displayInfo,command,processedCommand,
					2*MAX_TOKEN_LENGTH);
    if (strlen(processedCommand) > 0) system(processedCommand);

    XtFree(command);
  }
  XtUnmanageChild(displayInfo->shellCommandPromptD);

}






static Widget createShellCommandPromptD(
  Widget parent)
{
  Arg args[6];
  int n;
  XmString title;
  Widget prompt;

  title = XmStringCreateSimple("Command");
  n = 0;
  XtSetArg(args[n],XmNdialogTitle,title); n++;
  XtSetArg(args[n],XmNdialogStyle,XmDIALOG_FULL_APPLICATION_MODAL); n++;
  XtSetArg(args[n],XmNselectionLabelString,title); n++;
  XtSetArg(args[n],XmNautoUnmanage,False); n++;
 /* update global for selection box widget access */
  prompt = XmCreatePromptDialog(parent,
		"shellCommandPromptD",args,n);
  XmStringFree(title);
 
  XtAddCallback(prompt,
	XmNcancelCallback,(XtCallbackProc)shellCommandCallback,parent);
  XtAddCallback(prompt,XmNokCallback,
	(XtCallbackProc)shellCommandCallback,parent);
  return (prompt);
}




/***
 *** callbacks
 ***/


XtCallbackProc dmDisplayListOk(
  Widget  w,
  XtPointer client_data,
  XmSelectionBoxCallbackStruct *call_data)
{
  FILE *filePtr;
  char *filename;
  DisplayInfo *displayInfo, *nextDisplay;

  Widget dialog = (Widget)client_data;


/* if no list element selected, simply return */
  if (call_data->value == NULL) return;

/* get the filename string from the selection box */
  XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET, &filename);

  if (filename != NULL) {
    filePtr = fopen(filename,"r");
    if (filePtr != NULL) {
	XtUnmanageChild(dialog);
	dmDisplayListParse(filePtr,NULL,filename,(Widget)NULL,(Boolean)False);
	if (filePtr != NULL) fclose(filePtr);
    };
    XtFree(filename);
  }
}
	


XtCallbackProc popdownDialog(
  Widget  w,
  XtPointer client_data,
  XmSelectionBoxCallbackStruct *call_data)
{
  XtUnmanageChild(w);
}



XtCallbackProc executePopupMenuCallback(
  Widget  w,
  int buttonNumber,
  XmAnyCallbackStruct *call_data)
{
  Arg args[3];
  XtPointer data;
  DisplayInfo *displayInfo, *displayPtr, *nextDisplay;

/* button's parent (menuPane) has the displayInfo pointer */
  XtSetArg(args[0],XmNuserData,&data);
  XtGetValues(XtParent(w),args,1);
  displayInfo = (DisplayInfo *) data;

  if (buttonNumber == EXECUTE_POPUP_MENU_PRINT_ID) {

     utilPrint(XtDisplay(displayInfo->drawingArea),
		XtWindow(displayInfo->drawingArea),DISPLAY_XWD_FILE);

  } else if (buttonNumber == EXECUTE_POPUP_MENU_CLOSE_ID) {

  /* clear resource palette if this is the current display... */
     if (displayInfo == currentDisplayInfo) {
	highlightAndSetSelectedElements(NULL,0,0);
	clearResourcePaletteEntries();
     }

     currentDisplayInfo = displayInfo;
     if (currentDisplayInfo->hasBeenEditedButNotSaved) {
	XtManageChild(closeQD);
     } else {
     /* remove currentDisplayInfo from displayInfoList and cleanup */
	dmRemoveDisplayInfo(currentDisplayInfo);
	currentDisplayInfo = NULL;
     }

  }

}




/* user doesn't want to continue... */
XtCallbackProc popdownDisplayFileDialog(
  Widget  w,
  XtPointer client_data,
  XmSelectionBoxCallbackStruct *call_data)
{
  dmTerminateCA();
  dmTerminateX();
  exit(-1);
}




XtCallbackProc dmCreateRelatedDisplay(
  Widget  w,
  DisplayInfo *displayInfo,
  XmPushButtonCallbackStruct *call_data)
{
  char *filename, *argsString, *newFilename, token[MAX_TOKEN_LENGTH];
  char **nameArgs;
  Arg args[5];
  FILE *filePtr;
  int suffixLength, prefixLength;
  char *adlPtr;
  char processedArgs[2*MAX_TOKEN_LENGTH], name[2*MAX_TOKEN_LENGTH], *value;
  int i, j, k, n;

  XtSetArg(args[0],XmNuserData,&nameArgs);
  XtGetValues(w,args,1);

  filename = nameArgs[RELATED_DISPLAY_FILENAME_INDEX];
  argsString = nameArgs[RELATED_DISPLAY_ARGS_INDEX];
/*
 * if we want to be able to have RD's inherit their parent's
 *   macro-substitutions, then we must perform any macro substitution on 
 *   this argument string in this displayInfo's context before passing
 *   it to the created child display
 */
 if (globalDisplayListTraversalMode == DL_EXECUTE) {

  performMacroSubstitutions(displayInfo,argsString,processedArgs,
					2*MAX_TOKEN_LENGTH);
  filePtr = dmOpenUseableFile(filename);
  if (filePtr == NULL) {
    newFilename = STRDUP(filename);
    adlPtr = strstr(filename,DISPLAY_FILE_ASCII_SUFFIX);
    if (adlPtr != NULL) {
      /* ascii name */
       suffixLength = strlen(DISPLAY_FILE_ASCII_SUFFIX);
    } else {
       /* binary name */
       suffixLength = strlen(DISPLAY_FILE_BINARY_SUFFIX);
    }
    prefixLength = strlen(newFilename) - suffixLength;
    newFilename[prefixLength] = '\0';
    sprintf(token,
         "Can't open related display:\n\n        %s%s\n\n%s",
          newFilename, DISPLAY_FILE_ASCII_SUFFIX,
          "--> check EPICS_DISPLAY_PATH ");
    dmSetAndPopupWarningDialog(displayInfo,token,
          (XtCallbackProc)warnCallback, (XtCallbackProc)warnCallback);
    fprintf(stderr,"\n%s",token);
    free(newFilename);
  } else {
    dmDisplayListParse(filePtr,processedArgs,filename,(Widget)NULL,
	  (Boolean)True);
    fclose(filePtr);
  }

 }
}




XtCallbackProc dmExecuteShellCommand(
  Widget  w,
  DlShellCommandEntry *commandEntry,
  XmPushButtonCallbackStruct *call_data)
{
  char *promptPosition;
  int cmdLength, argsLength;
  int shellCommandStringPosition;
  char shellCommand[2*MAX_TOKEN_LENGTH],
		processedShellCommand[2*MAX_TOKEN_LENGTH];
  XmString xmString;
  extern char *strchr();
  DisplayInfo *displayInfo;
  XtPointer userData;
  Arg args[4];
  int n;


/* the displayInfo has been registered as userData on each shell command
 *    push button */
  XtVaGetValues(w,XmNuserData,&userData,NULL);
  displayInfo = (DisplayInfo *)userData;

  currentDisplayInfo = displayInfo;

/* 
 * this is really an ugly bit of code which will have to be cleaned
 *	up when the requirements are better defined!!
 */
  cmdLength = strlen(commandEntry->command);
  argsLength = strlen(commandEntry->args);
  promptPosition = NULL;
  shellCommandStringPosition = 0;
  shellCommand[0] = '\0';


/* create shell command prompt dialog */
  if (displayInfo->shellCommandPromptD == (Widget)NULL) {
    displayInfo->shellCommandPromptD = createShellCommandPromptD(
		displayInfo->shell);
  }


  /* command */
  if (cmdLength > 0) {
      strcpy(shellCommand,commandEntry->command);
      shellCommandStringPosition = cmdLength;
      shellCommand[shellCommandStringPosition++] = ' ';
      shellCommand[shellCommandStringPosition] = '\0';
      /* also support ? as first char. in command field for arbitrary cmd. */
      if (commandEntry->command[0] == '?') {
	xmString = XmStringCreateSimple("");
	XtVaSetValues(displayInfo->shellCommandPromptD,XmNtextString,xmString,
		NULL);
	XmStringFree(xmString);
	XtManageChild(displayInfo->shellCommandPromptD);
	return;
      }
  }


  /* also have some command arguments, see if ? for prompted input */
  if (cmdLength > 0 && argsLength > 0) {

    promptPosition = strchr(commandEntry->args,SHELL_CMD_PROMPT_CHAR);

    if (promptPosition == NULL) {

      /* no  prompt character found */
	strcpy(&(shellCommand[shellCommandStringPosition]),commandEntry->args);
	/* (MDA) NB: system() blocks! need to background (&) to not block */
	performMacroSubstitutions(displayInfo,
		shellCommand,processedShellCommand,
		2*MAX_TOKEN_LENGTH);
	if (strlen(processedShellCommand) > 0) system(processedShellCommand);
	shellCommand[0] = '\0';

    } else {

   /* a prompt character found - handle it by replacing with NULL and copying */
	strcpy(&(shellCommand[shellCommandStringPosition]),commandEntry->args);
	promptPosition = strchr(shellCommand,SHELL_CMD_PROMPT_CHAR);
	if (promptPosition != NULL) *promptPosition = '\0';

	/* now popup the prompt dialog and wait for input */
	xmString = XmStringCreateSimple(shellCommand);
	XtVaSetValues(displayInfo->shellCommandPromptD,XmNtextString,xmString,
		NULL);
	XmStringFree(xmString);
	XtManageChild(displayInfo->shellCommandPromptD);
    }

  } else if (cmdLength > 0 && argsLength == 0) {
  /* (MDA) NB: system() blocks! need to background (&) to not block */
    performMacroSubstitutions(displayInfo,
		shellCommand,processedShellCommand,
		2*MAX_TOKEN_LENGTH);
    if (strlen(processedShellCommand) > 0) system(processedShellCommand);
    shellCommand[0] = '\0';

  }

}




XtCallbackProc drawingAreaCallback(
  Widget  w,
  DisplayInfo *displayInfo,
  XmDrawingAreaCallbackStruct *call_data)
{
  int x, y;
  unsigned int uiw, uih;
  Dimension width, height, goodWidth, goodHeight, oldWidth, oldHeight;
  Boolean resized;
  XKeyEvent *key;
  XEvent event;
  Modifiers modifiers;
  KeySym keysym;
  int numSelected;
  Boolean objectDataOnly;
  Arg args[4];
  XtPointer userData;
  DlElement *elementPtr;
  float aspectRatio, newAspectRatio;

  Window root, child;
  int rootX,rootY,winX,winY;
  unsigned int mask;


  if (call_data->reason == XmCR_EXPOSE) {

    x = call_data->event->xexpose.x;
    y = call_data->event->xexpose.y;
    uiw = call_data->event->xexpose.width;
    uih = call_data->event->xexpose.height;

    if (displayInfo->drawingAreaPixmap != (Pixmap)NULL &&
	displayInfo->pixmapGC != (GC)NULL && 
	displayInfo->drawingArea != (Widget)NULL) {

	XCopyArea(display,displayInfo->drawingAreaPixmap,XtWindow(w),
	   displayInfo->pixmapGC,x,y,uiw,uih,x,y);

/*
 * do a forced traversal of the monitor list to guarantee proper display
 *  this is especially necessary for dynamic elements which may overlay
 *  static elements
 */
	if (globalDisplayListTraversalMode == DL_EXECUTE)
		traverseMonitorList(TRUE,displayInfo,x,y,uiw,uih);

    }


  } else if (call_data->reason == XmCR_RESIZE) {

/* RESIZE */
    XtSetArg(args[0],XmNwidth,&width);
    XtSetArg(args[1],XmNheight,&height);
    XtSetArg(args[2],XmNuserData,&userData);
    XtGetValues(w,args,3);


    if (globalDisplayListTraversalMode == DL_EDIT) {

/* in EDIT mode - only resize selected elements */
      clearClipboard();
      resized = dmResizeSelectedElements(displayInfo,width,height);
      unhighlightSelectedElements();
      unselectSelectedElements();

    } else {

/* in EXECUTE mode - resize all elements */


/* since calling for resize in resize handler - use this flag to ignore
 *   derived resize */
      XQueryPointer(display,RootWindow(display,screenNum),&root,&child,
		&rootX,&rootY,&winX,&winY,&mask);

      if (userData != NULL || !(mask & ShiftMask) ) {

        XtSetArg(args[0],XmNuserData,(XtPointer)NULL);
        XtSetValues(w,args,1);
	goodWidth = width;
	goodHeight = height;

      } else {

  /* constrain resizes to original aspect ratio, call for resize, then return */

	elementPtr = ((DlElement *)displayInfo->dlElementListHead)->next;
  /* get to DL_Display type which has old x,y,width,height */
	while (elementPtr->type != DL_Display) {elementPtr = elementPtr->next;}
	oldWidth = elementPtr->structure.display->object.width;
	oldHeight = elementPtr->structure.display->object.height;
	aspectRatio = (float)oldWidth/(float)oldHeight;
	newAspectRatio = (float)width/(float)height;
	if (newAspectRatio > aspectRatio) {
  /* w too big; derive w=f(h) */
	  goodWidth = aspectRatio*height;
	  goodHeight = height;
	} else {
  /* h too big; derive h=f(w) */
	  goodWidth = width;
	  goodHeight = (Dimension)((float)width/aspectRatio);
	}
  /* change width/height of DA */
  /* use DA's userData to signify a "forced" resize which can be ignored */
	XtSetArg(args[0],XmNwidth,goodWidth);
	XtSetArg(args[1],XmNheight,goodHeight);
	XtSetArg(args[2],XmNuserData,(XtPointer)1);
	XtSetValues(w,args,3);

	return;
      }


      resized = dmResizeDisplayList(displayInfo,goodWidth,goodHeight);
    }

/* (MDA) should always cleanup before traversing!! */
   if (resized) {
     clearResourcePaletteEntries();	/* clear any selected entries */
     dmCleanupDisplayInfo(displayInfo,FALSE);
     dmTraverseDisplayList(displayInfo);
   }


  } else if (call_data->reason == XmCR_INPUT) {

/* INPUT */
/* left/right/up/down for movement of selected elements */

    if (currentActionType == SELECT_ACTION &&
		currentDisplayInfo->numSelectedElements > 0) {

      key = &(call_data->event->xkey);

      if (key->type == KeyPress ) {

	XtTranslateKeycode(display,key->keycode,(Modifiers)NULL,
		&modifiers,&keysym);

	if (keysym == osfXK_Left || keysym == osfXK_Right  ||
	    keysym == osfXK_Up   || keysym == osfXK_Down) {

	/* unhighlight */
	  unhighlightSelectedElements();

          switch (keysym) {
	    case osfXK_Left:
		updateDraggedElements(1,0,0,0);
		break;
	    case osfXK_Right:
		updateDraggedElements(0,0,1,0);
		break;
	    case osfXK_Up:
		updateDraggedElements(0,1,0,0);
		break;
	    case osfXK_Down:
		updateDraggedElements(0,0,0,1);
		break;
	    default:
		break;
          }
/*
 * (MDA) could be smarter about this update 
 *	- just update small part of display, (restricted updateDraggedElements)
 */
	/* highlight */
	  numSelected = highlightSelectedElements();
	  if (numSelected == 1) {
	    objectDataOnly = True;
	    updateGlobalResourceBundleAndResourcePalette(objectDataOnly);
	  }

	}


      }
    }
    
  }
}





XtCallbackProc relatedDisplayMenuButtonDestroy(
  Widget w,
  char *data,
  XmPushButtonCallbackStruct *call_data)
{
  char **freeData;
  /* free up the memory associated with the menu button  */
     if (data != NULL) {
  /* since this is a  "char * nameArgs[]" thing... */
	freeData = (char **)data;
	free( (char *)freeData[RELATED_DISPLAY_FILENAME_INDEX]);
	free( (char *)freeData[RELATED_DISPLAY_ARGS_INDEX]);
	free( (char *) data );
	data = NULL;
    }
}



XtCallbackProc controllerDestroy(
  Widget  w,
  ChannelAccessControllerData *data,
  XmAnyCallbackStruct *call_data)
{
  Arg args[2];
  XtPointer userData;
  XmString xmString;
  OptionMenuData *optionMenuData;
  int i;

  XtSetArg(args[0],XmNuserData,&userData);
  XtGetValues(w,args,1);
  
/*
 * free any userData attached to the widget (okay as long as default=NULL)
 */
     if (userData != NULL) {
	switch (data->controllerType) {
	    case DL_Menu:
	    case DL_ChoiceButton:
		optionMenuData = (OptionMenuData *) userData;
		for (i = 0; i < optionMenuData->nButtons; i++)
		    XmStringFree(optionMenuData->buttons[i]);
		free ( (char *) optionMenuData->buttonType);
		free ( (char *) optionMenuData->buttons);
		free ( (char *) optionMenuData);
		break;
	}
     }

  /* free up the memory associated with the controller object */
     free( (char *) data );
}



XtCallbackProc monitorDestroy(
  Widget  w,
  XtPointer data,
  XmAnyCallbackStruct *call_data)
{
  Arg args[2];
  XtPointer userData;
  CartesianPlotData *cpData;
  StripChartData *scData;
  ChannelAccessMonitorData *mData;
  int i;


  XtSetArg(args[0],XmNuserData,&userData);
  XtGetValues(w,args,1);

  if (XtClass(w) == xtXrtGraphWidgetClass && userData != NULL) {
    if (data != NULL) {
	cpData = (CartesianPlotData *) data;
	if (cpData->xrtData1 != NULL) XrtDestroyData(cpData->xrtData1,TRUE);
	if (cpData->xrtData2 != NULL) XrtDestroyData(cpData->xrtData2,TRUE);
	free( (char *)data);
    }
    return;
  }


  if (!strcmp(XtName(w),stripChartWidgetName)) {
    if (data != NULL) {
	free( (char *)data);
    }
    return;
  }
  

  /* free any userData attached to the widget (okay as long as default=NULL) */
  if (userData != NULL) {
    if (data != NULL) {
     mData = (ChannelAccessMonitorData *) data;
     if (mData->numberStateStrings > 0) {
	   for (i = 0 ; i < mData->numberStateStrings; i++) {
		free( (char *) mData->stateStrings[i]);
		XmStringFree(mData->xmStateStrings[i]);
	   }
	   free ( (char *) mData->stateStrings);
	   free ( (char *) mData->xmStateStrings);
     }
    }
    free( (char *) userData);
  }

}





XtCallbackProc warnCallback(
  Widget  w,
  DisplayInfo *displayInfo,
  XmAnyCallbackStruct *call_data)
{
  if (displayInfo->warningDialog != NULL) {
/* remove any grab for this (modal) dialog */
    modalGrab = FALSE;
    XtRemoveGrab(XtParent(displayInfo->warningDialog));
    XtUnmanageChild(displayInfo->warningDialog);
  }
}




XtCallbackProc exitCallback(
  Widget  w,
  DisplayInfo *displayInfo,
  XmAnyCallbackStruct *call_data)
{

  if (displayInfo->warningDialog != NULL) {
/* remove any grabs for (modal) dialog */
    modalGrab = FALSE;
    XtRemoveGrab(XtParent(displayInfo->warningDialog));
    XtUnmanageChild(displayInfo->warningDialog);
  }

/* clean up any existing displays in the displayInfo list */
  dmRemoveAllDisplayInfo();


}





/*
 * option menu callback for Menu type display list object
 */

XtCallbackProc simpleOptionMenuCallback(
  Widget  w,
  int buttonNumber,
  XmPushButtonCallbackStruct *call_data)
{
  Arg args[3];
  OptionMenuData *data;
  ChannelAccessControllerData *caData;
  short btnNumber = buttonNumber;

/*
 * only do ca_put if this widget actually initiated the channel change
 */
  if (call_data->event != NULL && call_data->reason == XmCR_ACTIVATE) {

/* button's parent (menuPane) has the displayInfo pointer */
     XtSetArg(args[0],XmNuserData,&data);
     XtGetValues(XtParent(w),args,1);
     if (data == NULL) return;		/* return if invalid OptionMenuData */

     caData = (ChannelAccessControllerData *) data->controllerData;
     if (caData == NULL) return;	/* return if invalid caData */

     globalModifiedFlag = True;
     if (caData->monitorData != NULL)
	 caData->monitorData->modified = PRIMARY_MODIFIED;

     if (ca_state(caData->chid) == cs_conn) {
       SEVCHK(ca_put(DBR_SHORT,caData->chid,&(btnNumber)),
	"simpleOptionMenuCallback: error in ca_put");
       ca_flush_io();
     } else {
       fprintf(stderr,"\nsimpleOptionMenuCallback: %s not connected",
		ca_name(caData->chid));
     }
  }

}


/* just like simpleOptionMenu.. above, except for choice button/radio-box */
/* (don't want multiple ca_put()s when all toggles change */
XtCallbackProc simpleRadioBoxCallback(
  Widget  w,
  int buttonNumber,
  XmToggleButtonCallbackStruct *call_data)
{
  Arg args[3];
  OptionMenuData *data;
  ChannelAccessControllerData *caData;
  short btnNumber = buttonNumber;

/*
 * only do ca_put if this widget actually initiated the channel change
 */
  if (call_data->event != NULL && call_data->set == True) {

/* button's parent (menuPane) has the displayInfo pointer */
     XtSetArg(args[0],XmNuserData,&data);
     XtGetValues(XtParent(w),args,1);
     if (data == NULL) return;		/* return if invalid OptionMenuData */

     caData = (ChannelAccessControllerData *) data->controllerData;
     if (caData == NULL) return;	/* return if invalid caData */

     globalModifiedFlag = True;
     if (caData->monitorData != NULL)
	caData->monitorData->modified = PRIMARY_MODIFIED;

     if (ca_state(caData->chid) == cs_conn) {
       SEVCHK(ca_put(DBR_SHORT,caData->chid,&(btnNumber)),
	"simpleRadioBoxCallback: error in ca_put");
       ca_flush_io();
     } else {
       fprintf(stderr,"\nsimpleRadioBoxCallback: %s not connected",
		ca_name(caData->chid));
     }
  }

}


/*
 * TextEntry special handling:  if user starts editing text field,
 *  then be sure to update value on losingFocus (since until activate,
 *  the value isn't ca_put()-ed, and the text field can be inconsistent
 *  with the underlying channel
 */
static XtCallbackProc textEntryLosingFocusCallback(
  Widget w,
  ChannelAccessControllerData *data,
  XmTextVerifyCallbackStruct *call_data)
{
  globalModifiedFlag = True;
  data->monitorData->modified = PRIMARY_MODIFIED;
  data->monitorData->displayed = False;
  XtRemoveCallback(w,XmNlosingFocusCallback,
	(XtCallbackProc)textEntryLosingFocusCallback,data);
}


XtCallbackProc textEntryModifyVerifyCallback(
  Widget w,
  ChannelAccessControllerData *data,
  XmTextVerifyCallbackStruct *call_data)
{

/* NULL event means value changed programmatically; hence don't process */
  if (call_data->event != NULL) {
    switch (XtHasCallbacks(w,XmNlosingFocusCallback)) {
      case XtCallbackNoList:
      case XtCallbackHasNone:
	XtAddCallback(w,XmNlosingFocusCallback,
		(XtCallbackProc)textEntryLosingFocusCallback,data);
	break;

      case XtCallbackHasSome:
	break;
    }
    call_data->doit = True;
  }

}



/*
 * valueChanged type callbacks for Controller Objects/Widgets
 *	(everybody except valuator which need special handling)
 */

XtCallbackProc controllerValueChanged(
  Widget  w,
  ChannelAccessControllerData *data,
  XmAnyCallbackStruct *call_data)
{
  XmToggleButtonCallbackStruct *toggleCallData;
  XmPushButtonCallbackStruct *pushCallData;
  DlMessageButton *dlMessageButton;
  char *textValue;

  Arg args[3];


  if (data == NULL) return;

/*
 * only on operator input should this be processed (not monitors doing a set)
 */
  if (data->chid!=NULL && data->connected==TRUE && call_data->event != NULL) {

     /* set modified flag on monitor data so that next update traversal will
      * set controller visual state correctly (noting that for controllers
      * as monitors the ->modified flag alone is used to do updates
      */
      globalModifiedFlag = True;
      if (data->monitorData != NULL) data->monitorData->modified =
							PRIMARY_MODIFIED;

      switch (data->controllerType) {

	case DL_Valuator:
	    fprintf(stderr,
	    "\ncontrollerValueChanged: Valuator shouldn't be processed here!");
		break;


	case DL_MessageButton:
		pushCallData = (XmPushButtonCallbackStruct *) call_data;
		dlMessageButton = (DlMessageButton *)
			data->monitorData->specifics;

		if (pushCallData->reason == XmCR_ARM) {
/* message button can only put strings */
		    if (dlMessageButton->press_msg[0] != '\0') {
			switch (ca_field_type(data->chid)) {
			  case DBF_STRING:
			    strncpy(data->stringValue,
				dlMessageButton->press_msg,
				MAX_STRING_SIZE-1);
			    data->stringValue[MAX_STRING_SIZE-1] = '\0';
			    if (ca_state(data->chid) == cs_conn) {
			      SEVCHK(ca_put(DBR_STRING,data->chid,
			   	dlMessageButton->press_msg),
				"controllerValueChanged: error in ca_put");
			    } else {
				fprintf(stderr,
				"\ncontrollerValueChanged: %s not connected",
					ca_name(data->chid));
			     }
			     break;
			  default:
			    data->value = (double)
				atof(dlMessageButton->press_msg);
			    if (ca_state(data->chid) == cs_conn) {
			      SEVCHK(ca_put(DBR_DOUBLE,data->chid,&data->value),
				"controllerValueChanged: error in ca_put");
			    } else {
				fprintf(stderr,
				"\ncontrollerValueChanged: %s not connected",
					ca_name(data->chid));
			     }
			   break;
			}
		    }
		} else if (pushCallData->reason == XmCR_DISARM) {
		    if (dlMessageButton->release_msg[0] != '\0') {
			switch (ca_field_type(data->chid)) {
			  case DBF_STRING:
			    strncpy(data->stringValue,
				dlMessageButton->release_msg,
				MAX_STRING_SIZE-1);
			    data->stringValue[MAX_STRING_SIZE-1] = '\0';
			    if (ca_state(data->chid) == cs_conn) { 
			      SEVCHK(ca_put(DBR_STRING,data->chid,
				dlMessageButton->release_msg),
				"controllerValueChanged: error in ca_put");
			    } else {
				fprintf(stderr,
				"\ncontrollerValueChanged: %s not connected",
					ca_name(data->chid));
			     }
			     break;
			  default:
			    data->value = (double)
				atof(dlMessageButton->release_msg);
			    if (ca_state(data->chid) == cs_conn) { 
			      SEVCHK(ca_put(DBR_DOUBLE,data->chid,&data->value),
				"controllerValueChanged: error in ca_put");
			    } else {
				fprintf(stderr,
				"\ncontrollerValueChanged: %s not connected",
					ca_name(data->chid));
			     }
			   break;
			}
		    }
		}
		break;


	case DL_TextEntry:
		textValue = XmTextFieldGetString(w);
		switch (ca_field_type(data->chid)) {
			case DBF_STRING:
			   strncpy(data->stringValue,textValue,
				MAX_STRING_SIZE-1);
			   data->stringValue[MAX_STRING_SIZE-1] = '\0';
			    if (ca_state(data->chid) == cs_conn) {
			     SEVCHK(ca_put(DBR_STRING,data->chid,
				data->stringValue),
				"controllerValueChanged: error in ca_put");
			    } else {
				fprintf(stderr,
				"\ncontrollerValueChanged: %s not connected",
					ca_name(data->chid));
			    }
			   break;
			default:
			   data->value = (double) atof(textValue);
			    if (ca_state(data->chid) == cs_conn) {
			      SEVCHK(ca_put(DBR_DOUBLE,data->chid,
				&(data->value)),
				"controllerValueChanged: error in ca_put");
			    } else {
				fprintf(stderr,
				"\ncontrollerValueChanged: %s not connected",
					ca_name(data->chid));
			    }
			   break;
		}
		XtFree(textValue);
		break;
      }
      ca_flush_io();
  }
}




/*
 * valuatorValueChanged - drag and value changed callback for valutor
 */

XtCallbackProc valuatorValueChanged(
  Widget  w,
  ChannelAccessControllerData *data,
  XmScaleCallbackStruct *call_data)
{
  ChannelAccessMonitorData *mData;
  DlValuator *dlValuator;
  Arg args[3];
  XButtonEvent *buttonEvent;
  XKeyEvent *keyEvent;


  if (data == NULL) return;	/* do nothing if invalid data ptrs */

/*
 * only on operator input should this be processed (not monitors doing a set)
 */
  if (data->chid != NULL && data->connected == TRUE) {

     mData = data->monitorData;
     if (mData == NULL) return;

    /* set modified flag on monitor data so that next update traversal will
     * set controller visual state correctly (noting that for controllers
     * as monitors the ->modified flag alone is used to do updates
     */
     globalModifiedFlag = True;
     mData->modified = PRIMARY_MODIFIED;

     dlValuator = (DlValuator *)mData->specifics;
     if (dlValuator == NULL) return;


     if (call_data->reason == XmCR_DRAG) {

	dlValuator->dragging = True;		/* mark beginning of drag  */
	dlValuator->enableUpdates = False;	/* disable updates in drag */

/* drag - set value based on relative position (easy) */
	mData->oldIntegerValue = call_data->value;
	data->value = data->lopr
		+ ((double)(call_data->value - VALUATOR_MIN))
		/((double)(VALUATOR_MAX - VALUATOR_MIN) )
		*(data->hopr - data->lopr);

     } else if (call_data->reason = XmCR_VALUE_CHANGED) {

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
		    if (mData->oldIntegerValue > call_data->value) {
			/* decrease value one 10*precision value */
			data->value = MAX(data->lopr, data->value -
			    10.*dlValuator->dPrecision);
		    } else if (mData->oldIntegerValue < call_data->value) {
			/* increase value one 10*precision value */
			data->value = MIN(data->hopr, data->value +
			    10.*dlValuator->dPrecision);
		    }
		} else {
		/* single increment (precision) */
		    if (mData->oldIntegerValue > call_data->value) {
			/* decrease value one precision value */
			data->value = MAX(data->lopr, data->value -
			    dlValuator->dPrecision);
		    } else if (mData->oldIntegerValue < call_data->value) {
			/* increase value one precision value */
			data->value = MIN(data->hopr, data->value +
			    dlValuator->dPrecision);
		    }
		}

	   } else if (call_data->event->type == ButtonPress) {

		buttonEvent = (XButtonEvent *)call_data->event;
		if (buttonEvent->state & ControlMask) {
/* handle this as multiple increment/decrement */
		  if (call_data->value - mData->oldIntegerValue < 0) {
		  /* decrease value one 10*precision value */
			data->value = MAX(data->lopr, data->value -
			    10.*dlValuator->dPrecision);
		  } else if (call_data->value - mData->oldIntegerValue > 0) {
		  /* increase value one 10*precision value */
			data->value = MIN(data->hopr, data->value +
			    10.*dlValuator->dPrecision);
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
	   if (call_data->value - mData->oldIntegerValue < 0) {
		/* decrease value one precision value */
			data->value = MAX(data->lopr, data->value -
			    dlValuator->dPrecision);
	   } else if (call_data->value - mData->oldIntegerValue > 0) {
		/* increase value one precision value */
			data->value = MIN(data->hopr, data->value +
			    dlValuator->dPrecision);
	   }

	}  /* end if (call_data->event != NULL) */
     }
/* move/redraw valuator & value, but force use of user-selected value */
     valuatorSetValue(mData,data->value,True);

     if (ca_state(data->chid) == cs_conn) {
       SEVCHK(ca_put(DBR_DOUBLE,data->chid,&(data->value)),
			"valuatorValueChanged: error in ca_put");
       ca_flush_io();
     } else {
#ifdef DEBUG
	fprintf(stderr,
	"\nvaluatorValueChanged: %s not connected", ca_name(data->chid));
#endif
     }
  }
}



/*
 * refresh/redisplay callback for strip charts
 */

XtCallbackProc redisplayStrip(
  Widget  w,
  Strip **strip,
  XmAnyCallbackStruct *call_data)
{
  if (*strip != NULL) stripRefresh(*strip);
}


