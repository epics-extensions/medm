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
 * .02  09-05-95        vong    2.1.0 release 
 * .03  09-07-95        vong    conform to c++ syntax
 * .04  02-23-96        vong    fixe the wrong aspect calculation.
 *
 *****************************************************************************
*/

#include "medm.h"

#include <limits.h>


/* for modal dialogs - defined in utils.c and referenced here in callbacks.c */
extern Boolean modalGrab;
extern Widget mainMW;

extern char *stripChartWidgetName;



/*
 * callbacks
 */

#ifdef __cplusplus
static void shellCommandCallback(
  Widget,
  XtPointer client_data,
  XtPointer cbs)
#else
static void shellCommandCallback(
  Widget w,
  XtPointer client_data,
  XtPointer cbs)
#endif

{
  char *command;
  DisplayInfo *displayInfo;
  char processedCommand[2*MAX_TOKEN_LENGTH];
  XmSelectionBoxCallbackStruct *call_data = (XmSelectionBoxCallbackStruct *) cbs;

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
    if (strlen(processedCommand) > (size_t) 0) system(processedCommand);

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
 
  XtAddCallback(prompt, XmNcancelCallback,shellCommandCallback,parent);
  XtAddCallback(prompt,XmNokCallback,shellCommandCallback,parent);
  return (prompt);
}




/***
 *** callbacks
 ***/

#ifdef __cplusplus
void dmDisplayListOk(Widget, XtPointer cd, XtPointer cbs)
#else
void dmDisplayListOk(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
  FILE *filePtr;
  char *filename;

  Widget dialog = (Widget)cd;
  XmSelectionBoxCallbackStruct *call_data = (XmSelectionBoxCallbackStruct *) cbs;


/* if no list element selected, simply return */
  if (call_data->value == NULL) return;

/* get the filename string from the selection box */
  XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET, &filename);

  if (filename != NULL) {
    filePtr = fopen(filename,"r");
    if (filePtr != NULL) {
	XtUnmanageChild(dialog);
	dmDisplayListParse(filePtr,NULL,filename,NULL,(Boolean)False);
	if (filePtr != NULL) fclose(filePtr);
    };
    XtFree(filename);
  }
}

void executePopupMenuCallback(Widget  w, XtPointer cd, XtPointer cbs)
{
  int buttonNumber = (int) cd;
  XmAnyCallbackStruct *call_data = (XmAnyCallbackStruct *) cbs;
  Arg args[3];
  XtPointer data;
  DisplayInfo *displayInfo;

/* button's parent (menuPane) has the displayInfo pointer */
  XtSetArg(args[0],XmNuserData,&data);
  XtGetValues(XtParent(w),args,1);
  displayInfo = (DisplayInfo *) data;

  if (buttonNumber == EXECUTE_POPUP_MENU_PRINT_ID) {

     utilPrint(XtDisplay(displayInfo->drawingArea),
		XtWindow(displayInfo->drawingArea),DISPLAY_XWD_FILE);

  } else if (buttonNumber == EXECUTE_POPUP_MENU_CLOSE_ID) {
    closeDisplay(w);
  }

}


#if 0  /* not used */

/* user doesn't want to continue... */
XtCallbackProc popdownDisplayFileDialog(
  Widget  w,
  XtPointer client_data,
  XmSelectionBoxCallbackStruct *call_data)
{
  medmCATerminate();
  dmTerminateX();
  exit(-1);
}

#endif



#ifdef __cplusplus
void dmCreateRelatedDisplay(Widget  w, XtPointer cd, XtPointer)
#else
void dmCreateRelatedDisplay(Widget  w, XtPointer cd, XtPointer cbs)
#endif
{
  DisplayInfo *displayInfo = (DisplayInfo *) cd;
  char *filename, *argsString, *newFilename, token[MAX_TOKEN_LENGTH];
  char **nameArgs;
  Arg args[5];
  FILE *filePtr;
  int suffixLength, prefixLength;
  char *adlPtr;
  char processedArgs[2*MAX_TOKEN_LENGTH], name[2*MAX_TOKEN_LENGTH];

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
    dmSetAndPopupWarningDialog(displayInfo,token,"Ok",NULL,NULL);
    fprintf(stderr,"\n%s",token);
    free(newFilename);
  } else {
    dmDisplayListParse(filePtr,processedArgs,filename,NULL,(Boolean)True);
    fclose(filePtr);
  }

 }
}



#ifdef __cplusplus
void dmExecuteShellCommand(
  Widget  w,
  DlShellCommandEntry *commandEntry,
  XmPushButtonCallbackStruct *)
#else
void dmExecuteShellCommand(
  Widget  w,
  DlShellCommandEntry *commandEntry,
  XmPushButtonCallbackStruct *call_data)
#endif
{
  char *promptPosition;
  int cmdLength, argsLength;
  int shellCommandStringPosition;
  char shellCommand[2*MAX_TOKEN_LENGTH],
		processedShellCommand[2*MAX_TOKEN_LENGTH];
  XmString xmString;
#if 0
  extern char *strchr();
#endif
  DisplayInfo *displayInfo;
  XtPointer userData;
  Arg args[4];


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
	if (strlen(processedShellCommand) > (size_t) 0) system(processedShellCommand);
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
    if (strlen(processedShellCommand) > (size_t) 0) system(processedShellCommand);
    shellCommand[0] = '\0';

  }

}

#if 0
#ifdef __cplusplus
void traverseDisplayLater(XtPointer cd, XtIntervalId *) {
#else
void traverseDisplayLater(XtPointer cd, XtIntervalId *id) {
#endif
  DisplayInfo *displayInfo = (DisplayInfo *) cd;
  dmTraverseDisplayList(displayInfo);
}
#endif

void drawingAreaCallback(
  Widget  w,
  DisplayInfo *displayInfo,
  XmDrawingAreaCallbackStruct *call_data)
{
  int x, y;
  unsigned int uiw, uih;
  Dimension width, height, goodWidth, goodHeight, oldWidth, oldHeight;
  Boolean resized;
  XKeyEvent *key;
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
      if (globalDisplayListTraversalMode == DL_EXECUTE) {
        Display *display = XtDisplay(displayInfo->drawingArea);
	GC gc = displayInfo->gc;

	XPoint points[4];
        Region region;
	XRectangle clipRect;

	points[0].x = x;
        points[0].y = y;
        points[1].x = x + uiw;
        points[1].y = y;
        points[2].x = x + uiw;
        points[2].y = y + uih;
        points[3].x = x;
        points[3].y = y + uih;
        region = XPolygonRegion(points,4,EvenOddRule);
        if (region == NULL) {
          medmPrintf("medmRepaintRegion : XPolygonRegion() return NULL\n");
          return;
        }

        /* clip the region */
        clipRect.x = x;
        clipRect.y = y;
        clipRect.width = uiw;
        clipRect.height = uih;


        XSetClipRectangles(display,gc,0,0,&clipRect,1,YXBanded);

        updateTaskRepaintRegion(displayInfo,&region);
        /* release the clipping region */
        XSetClipOrigin(display,gc,0,0);
        XSetClipMask(display,gc,None);
        if (region) XDestroyRegion(region);
      }
    }
    return;
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
      if (displayInfo->hasBeenEditedButNotSaved == False)
	medmMarkDisplayBeingEdited(displayInfo);

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
	  goodWidth = (unsigned short) (aspectRatio*(float)height);
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
#if 0
     XtAppAddTimeOut(appContext,1000,traverseDisplayLater,displayInfo); 
#else
     dmTraverseDisplayList(displayInfo);
#endif
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
	  if (currentDisplayInfo->hasBeenEditedButNotSaved == False) 
	    medmMarkDisplayBeingEdited(currentDisplayInfo);

	}


      }
    }
    
  }
}





#ifdef __cplusplus
void relatedDisplayMenuButtonDestroy(Widget, XtPointer cd, XtPointer) 
#else
void relatedDisplayMenuButtonDestroy(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
  char *data = (char *) cd;
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


#ifdef __cplusplus
void warnCallback(Widget, XtPointer cd, XtPointer)
#else
void warnCallback(Widget  w, XtPointer cd, XtPointer cbs)
#endif
{
  DisplayInfo *displayInfo = (DisplayInfo *) cd;

  if (displayInfo->warningDialog != NULL) {
/* remove any grab for this (modal) dialog */
    modalGrab = FALSE;
    XtRemoveGrab(XtParent(displayInfo->warningDialog));
    XtUnmanageChild(displayInfo->warningDialog);
  }
}



#ifdef __cplusplus
void exitCallback(Widget, XtPointer cd, XtPointer)
#else
void exitCallback(Widget  w, XtPointer cd, XtPointer cbs)
#endif
{
  DisplayInfo *displayInfo = (DisplayInfo *) cd;

  if (displayInfo->warningDialog != NULL) {
/* remove any grabs for (modal) dialog */
    modalGrab = FALSE;
    XtRemoveGrab(XtParent(displayInfo->warningDialog));
    XtUnmanageChild(displayInfo->warningDialog);
  }

/* clean up any existing displays in the displayInfo list */
  dmRemoveAllDisplayInfo();


}

/* just like simpleOptionMenu.. above, except for choice button/radio-box */
/* (don't want multiple ca_put()s when all toggles change */
void simpleRadioBoxCallback(
  Widget  w,
  int buttonNumber,
  XmToggleButtonCallbackStruct *call_data)
{
  Channel *pCh;
  short btnNumber = buttonNumber;

/*
 * only do ca_put if this widget actually initiated the channel change
 */
  if (call_data->event != NULL && call_data->set == True) {

     /* button's parent (menuPane) has the displayInfo pointer */
     XtVaGetValues(XtParent(w),XmNuserData,&pCh,NULL);

     if (pCh == NULL) return;

     if (ca_state(pCh->chid) == cs_conn) {
       SEVCHK(ca_put(DBR_SHORT,pCh->chid,&(btnNumber)),
	"simpleRadioBoxCallback: error in ca_put");
       ca_flush_io();
     } else {
       fprintf(stderr,"\nsimpleRadioBoxCallback: %s not connected",
		ca_name(pCh->chid));
     }
  }

}
