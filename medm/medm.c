/*
 *	Mark Anderson, Argonne National Laboratory:
 *		U.S. DOE, University of Chicago
 */


#define ALLOCATE_STORAGE
#include "medm.h"
#include <Xm/RepType.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <errno.h>

/* for X property cleanup */
#include <signal.h>
#include <Xm/MwmUtil.h>

#define HOT_SPOT_WIDTH 24

#define N_MAX_MENU_ELES 20
#define N_MAIN_MENU_ELES 5

#define N_FILE_MENU_ELES 8
#define FILE_BTN_POSN 0

#define FILE_NEW_BTN 0
#define FILE_OPEN_BTN 1
#define FILE_SAVE_BTN 2
#define FILE_SAVE_AS_BTN 3
#define FILE_CLOSE_BTN 4
#define FILE_PRINT_BTN 5
#define FILE_EXIT_BTN 6

#define EDIT_BTN_POSN 1
/* but note this menu is laid out in dmInit.c createEditMenu() */

#define N_VIEW_MENU_ELES 3
#define VIEW_BTN_POSN 2

#ifdef EXTENDED_INTERFACE
#define N_PALETTES_MENU_ELES 4
#else
#define N_PALETTES_MENU_ELES 3
#endif
#define PALETTES_BTN_POSN 3

#define PALETTES_OBJECT_BTN 0
#define PALETTES_RESOURCE_BTN 1
#define PALETTES_COLOR_BTN 2
#ifdef EXTENDED_INTERFACE
#define PALETTES_CHANNEL_BTN 3
#endif

#define N_HELP_MENU_ELES 7
#define HELP_BTN_POSN 4

#define HELP_ON_CONTEXT_BTN 0
#define HELP_ON_WINDOW_BTN 1
#define HELP_ON_KEYS_BTN 2
#define HELP_INDEX_BTN 3
#define HELP_ON_HELP_BTN 4
#define HELP_TUTORIAL_BTN 5
#define HELP_ON_VERSION_BTN 6


/* one lonesome function prototype without a home */
extern Widget createAndPopupProductDescriptionShell(
  XtAppContext appContext, Widget topLevelShell,
  char *name, XmFontList nameFontList,
  Pixmap namePixmap,
  char *description, XmFontList descriptionFontList,
  char *versionInfo, char *developedAt, XmFontList otherFontList,
  int background, int foreground, int seconds);

static void createMain();

/*
 * globals for use by all routines in this file
 */
static Widget openFSD;
static Widget mainEditPDM, mainViewPDM, mainPalettesPDM;
static Widget fileNewBtn, fileSaveBtn, fileSaveAsBtn;
static Widget productDescriptionShell;

static String fallbackResources[] = {
  "Medm*initialResourcesPersistent: False",
  "Medm*foreground: black",
  "Medm*background: #b0c3ca",
  "Medm*highlightThickness: 1",
  "Medm*shadowThickness: 2",
  "Medm*fontList: 6x10",
  "Medm*XmBulletinBoard.marginWidth: 2",
  "Medm*XmBulletinBoard.marginHeight: 2",
  "Medm*XmMainWindow.showSeparator: False",

  "Medm*XmTextField.translations: #override \
    None<Key>osfDelete:     delete-previous-character() \n\
    None<Key>osfBackSpace:  delete-next-character()",

  "Medm*XmTextField.verifyBell: False",
/***
 *** main window
 ***/
  "Medm.mainMW.mainMB*fontList: 8x13",
  "Medm.mainMW*frameLabel*fontList: 8x13",
  "Medm.mainMW*modeRB*XmToggleButton.indicatorOn: false",
  "Medm.mainMW*modeRB*XmToggleButton.shadowThickness: 2",
  "Medm.mainMW*modeRB*XmToggleButton.highlightThickness: 1",
  "Medm.mainMW*openFSD.dialogTitle: Open",
  "Medm.mainMW*helpMessageBox.dialogTitle: Help",
  "Medm.mainMW*closeQD.dialogTitle: Close",
  "Medm.mainMW*closeQD.messageString: Do you really want to Close \
    this display\nand (potentially) lose changes?",
  "Medm.mainMW*saveAsPD.dialogTitle: Save As...",
  "Medm.mainMW*saveAsPD.selectionLabelString: \
    Name of file to save display in:",
  "Medm.mainMW*exitQD.dialogTitle: Exit",
  "Medm.mainMW*exitQD.messageString: Do you really want to Exit?",
  "Medm.mainMW*XmRowColumn.tearOffModel: XmTEAR_OFF_ENABLED",
/***
 *** objectPalette
 ***/
  "Medm*objectMW.objectMB*fontList: 8x13",
  "Medm*objectMW*XmLabel.marginWidth: 0",
  "Medm*objectMW.objectBB.paletteRC.XmToggleButtonGadget.indicatorOn: False",
  "Medm*objectMW.objectBB.paletteRC.XmToggleButtonGadget.labelType: XmPIXMAP",
  "Medm*objectMW.objectBB.paletteRC.XmToggleButtonGadget.alignment: XmALIGNMENT_CENTER",
  "Medm*objectMW.objectBB.paletteRC.XmToggleButtonGadget.marginWidth: 0",
  "Medm*objectMW.objectBB.paletteRC.XmToggleButtonGadget.marginHeight: 0",
  "Medm*objectMW.objectBB.paletteRC.XmToggleButtonGadget.marginTop: 0",
  "Medm*objectMW.objectBB.paletteRC.XmToggleButtonGadget.marginBottom: 0",
  "Medm*objectMW.objectBB.paletteRC.XmToggleButtonGadget.marginLeft: 0",
  "Medm*objectMW.objectBB.paletteRC.XmToggleButtonGadget.marginRight: 0",
  "Medm*objectMW.objectBB.paletteRC.XmToggleButtonGadget.width: 29",
  "Medm*objectMW.objectBB.paletteRC.XmToggleButtonGadget.height: 29",
  "Medm*objectMW.objectBB.paletteRC.XmToggleButtonGadget.recomputeSize: False",
  "Medm*objectMW.objectBB.paletteRC.XmToggleButtonGadget.shadowType: XmSHADOW_OUT",
  "Medm*objectMW.objectBB.paletteRC.XmToggleButtonGadget.pushButtonEnabled: True",
  "Medm*objectMW.objectBB.paletteRC.XmToggleButtonGadget.highlightThickness: 0",
  "Medm*objectMW*paletteRC.marginHeight: 3",
  "Medm*objectMW*paletteRC.x: 0",
  "Medm*objectMW*paletteRC.y: 22",
  "Medm*objectMW*paletteRC.width: 485",
  "Medm*objectMW*paletteRC.height: 66",
  "Medm*objectMW*paletteRC.dummy.background: grey70",
  "Medm*objectMW*paletteRC.dummy.shadowThickness:0",
  "Medm*objectMW*paletteRC.dummy.sensitive: False",
  "Medm*objectMW*graphicsLabel.labelString: Graphics",
  "Medm*objectMW*graphicsLabel.x: 4",
  "Medm*objectMW*graphicsLabel.y: 6",
  "Medm*objectMW*monitorLabel.labelString: Monitors",
  "Medm*objectMW*monitorLabel.x: 196",
  "Medm*objectMW*monitorLabel.y: 6",
  "Medm*objectMW*controllerLabel.labelString: Controllers",
  "Medm*objectMW*controllerLabel.x: 355",
  "Medm*objectMW*controllerLabel.y: 6",
  "Medm*objectMW*miscLabel.labelString: Misc",
  "Medm*objectMW*miscLabel.x: 515",
  "Medm*objectMW*miscLabel.y: 6",
  "Medm*objectMW*importFSD.dialogTitle: Import...",
  "Medm*objectMW*importFSD.form.shadowThickness: 0",
  "Medm*objectMW*importFSD.form.typeLabel.labelString: Image Type:",
  "Medm*objectMW*importFSD.form.typeLabel.marginTop: 4",
  "Medm*objectMW*importFSD.form.frame.radioBox.orientation: XmHORIZONTAL",
  "Medm*objectMW*importFSD.form.frame.radioBox.numColumns: 1",
  "Medm*objectMW*importFSD.form.frame.radioBox*shadowThickness: 0",
  "Medm*objectMW*importFSD*XmToggleButton.indicatorOn: True",
  "Medm*objectMW*importFSD*XmToggleButton.labelType: XmString",
/***
 *** display area
 ***/
  "Medm*displayDA*XmRowColumn.tearOffModel: XmTEAR_OFF_ENABLED",
/* override some normal functions in displays for operational safety reasons */
  "Medm*displayDA*XmPushButton.translations: #override  <Key>space: ",
  "Medm*displayDA*XmPushButtonGadget.translations: #override <Key>space: ",
  "Medm*displayDA*XmToggleButton.translations: #override  <Key>space: ",
  "Medm*displayDA*XmToggleButtonGadget.translations: #override <Key>space: ",
  "Medm*displayDA*radioBox*translations: #override <Key>space: ",
/***
 *** colorPalette
 ***/
  "Medm*colorMW.colorMB*fontList: 8x13",
  "Medm*colorMW*XmLabel.marginWidth: 0",
  "Medm*colorMW*colorPB.width: 20",
  "Medm*colorMW*colorPB.height: 20",
/***
 *** resourcePalette
 ***/
  "Medm*resourceMW.width: 360",
  "Medm*resourceMW.height: 460",
  "Medm*resourceMW.resourceMB*fontList: 8x13",
  "Medm*resourceMW*localLabel.marginRight: 8",
  "Medm*resourceMW*bundlesSW.visualPolicy: XmCONSTANT",
  "Medm*resourceMW*bundlesSW.scrollBarDisplayPolicy: XmSTATIC",
  "Medm*resourceMW*bundlesSW.scrollingPolicy: XmAUTOMATIC",
  "Medm*resourceMW*bundlesRB.marginWidth: 10",
  "Medm*resourceMW*bundlesRB.marginHeight: 10",
  "Medm*resourceMW*bundlesRB.spacing: 10",
  "Medm*resourceMW*bundlesRB.packing: XmPACK_COLUMN",
  "Medm*resourceMW*bundlesRB.orientation: XmVERTICAL",
  "Medm*resourceMW*bundlesRB.numColumns: 3",
  "Medm*resourceMW*bundlesTB.indicatorOn: False",
  "Medm*resourceMW*bundlesTB.visibleWhenOff: False",
  "Medm*resourceMW*bundlesTB.borderWidth: 0",
  "Medm*resourceMW*bundlesTB.shadowThickness: 2",
  "Medm*resourceMW*messageF.rowColumn.spacing: 10",
  "Medm*resourceMW*messageF.resourceElementTypeLabel.fontList: 8x13",
  "Medm*resourceMW*messageF.verticalSpacing: 6",
  "Medm*resourceMW*messageF.horizontalSpacing: 3",
  "Medm*resourceMW*messageF.shadowType: XmSHADOW_IN",
#ifdef EXTENDED_INTERFACE
/***
 *** channelPalette
 ***/
  "Medm*channelMW.width: 140",
  "Medm*channelMW.channelMB*fontList: 8x13",
  "Medm*channelMW*XmLabel.marginWidth: 0",
#endif
/***
 *** widgetDM resource specifications
 ***/
  "Medm*warningDialog.background: salmon",
  "Medm*warningDialog.dialogTitle: Warning",
  "Medm*Indicator.AxisWidth: 3",
  "Medm*Bar.AxisWidth: 3",
  "Medm*Indicator.ShadowThickness: 2",
  "Medm*Bar.ShadowThickness: 2",
  "Medm*Meter.ShadowThickness: 2",
  NULL,
};


/********************************************
 **************** Callbacks *****************
 ********************************************/


static XtCallbackProc fileMenuSimpleCallback(Widget w, int buttonNumber,
		XmAnyCallbackStruct *call_data)
{
  Widget widget;
  DisplayInfo *newDisplayInfo;
  XmString dirMask;
  XEvent event;

    switch(buttonNumber) {

	case FILE_NEW_BTN:
		currentDisplayInfo = createDisplay();
		XtManageChild(currentDisplayInfo->drawingArea);
		break;

	case FILE_OPEN_BTN:
		XtVaGetValues(openFSD,XmNdirMask,&dirMask,NULL);
		XmListDeselectAllItems(XmFileSelectionBoxGetChild(openFSD,
			XmDIALOG_LIST));
		XmFileSelectionDoSearch(openFSD,dirMask);
		XtManageChild(openFSD);
		break;

	case FILE_SAVE_BTN:
	case FILE_SAVE_AS_BTN:
		if (displayInfoListHead->next == displayInfoListTail) {
/* do something about SAVING */
	/* only one display; no need to query user */
			currentDisplayInfo = displayInfoListHead->next;
			XtManageChild(saveAsPD);
		} else if (displayInfoListHead->next == NULL) {
	/* no displays, do nothing! */
		} else {
	/* more than one display; query user */
		    widget = XmTrackingEvent(mainShell,saveCursor,False,&event);
		    if (widget != (Widget)NULL) {
			currentDisplayInfo = dmGetDisplayInfoFromWidget(widget);
			if (currentDisplayInfo != NULL) XtManageChild(saveAsPD);
		    }
		}
		break;

	case FILE_CLOSE_BTN:
/* manage closeQD - based on interactive selection of display */
		if (displayInfoListHead->next == displayInfoListTail) {
	/* only one display; no need to query user */
			highlightAndSetSelectedElements(NULL,0,0);
			clearResourcePaletteEntries();
			currentDisplayInfo = displayInfoListHead->next;
			if (currentDisplayInfo->hasBeenEditedButNotSaved) {
			    XtManageChild(closeQD);
			} else {
		/* remove currentDisplayInfo from displayInfoList and cleanup */
			    dmRemoveDisplayInfo(currentDisplayInfo);
			    currentDisplayInfo = NULL;
			}
		} else if (displayInfoListHead->next == NULL) {
	/* no displays, do nothing! */
		} else {
	/* more than one display; query user */
		    widget = XmTrackingEvent(mainShell,closeCursor,False,
							&event);
		    if (widget != (Widget)NULL) {
			newDisplayInfo = dmGetDisplayInfoFromWidget(widget);
			if (newDisplayInfo == currentDisplayInfo) {
			  clearResourcePaletteEntries();
			  currentDisplayInfo = displayInfoListHead->next;
			}
			currentDisplayInfo = newDisplayInfo;
			if (currentDisplayInfo != NULL) {
			  if (currentDisplayInfo->hasBeenEditedButNotSaved) {
			    XtManageChild(closeQD);
			  } else {
		/* remove currentDisplayInfo from displayInfoList and cleanup */
			    dmRemoveDisplayInfo(currentDisplayInfo);
			    currentDisplayInfo = NULL;
			  }
			}
		    }
		}
		break;

	case FILE_PRINT_BTN:
		if (displayInfoListHead->next == displayInfoListTail) {
/* do something about PRINTING */
	/* only one display; no need to query user */
		    currentDisplayInfo = displayInfoListHead->next;
		    if (currentDisplayInfo != NULL)
			utilPrint(XtDisplay(currentDisplayInfo->drawingArea),
			   XtWindow(currentDisplayInfo->drawingArea),
			   DISPLAY_XWD_FILE);

		} else if (displayInfoListHead->next == NULL) {
	/* no displays, do nothing! */
		} else {
	/* more than one display; query user */
		    widget = XmTrackingEvent(mainShell,printCursor,False,
						&event);
		    if (widget != (Widget)NULL) {
			currentDisplayInfo = dmGetDisplayInfoFromWidget(widget);
			if (currentDisplayInfo != NULL) utilPrint(
			   XtDisplay(currentDisplayInfo->drawingArea),
			   XtWindow(currentDisplayInfo->drawingArea),
			   DISPLAY_XWD_FILE);
		    }
		}
		break;

	case FILE_EXIT_BTN:
		XtManageChild(exitQD);
		break;
    }
}




static XtCallbackProc fileMenuDialogCallback(Widget w, int btn,
				XmAnyCallbackStruct *call_data)
{
  XmSelectionBoxCallbackStruct *select;
  char *filename, warningString[2*MAX_FILE_CHARS];
  FILE *stream;
  XmString warningXmstring;
  int i, k, startPos, status;
  Boolean brandNewFile;

  char *suffixPosition;
  int appendPosition;
  char backupFilename[MAX_FILE_CHARS];
  struct stat statBuf;

  switch(call_data->reason){
	case XmCR_CANCEL:
		XtUnmanageChild(w);
		break;
	case XmCR_OK:
		switch(btn) {
		    case FILE_OPEN_BTN:
			currentDisplayInfo = createDisplay();
/* (MDA) need to create dlDisplay and dlColormaps for execution */
			XtManageChild(currentDisplayInfo->drawingArea);

			XtUnmanageChild(w);
			break;

		    case FILE_CLOSE_BTN:
/* (MDA) remove currentDisplayInfo from displayInfoList and cleanup */
			dmRemoveDisplayInfo(currentDisplayInfo);
			currentDisplayInfo = NULL;
			break;

/* FILE_SAVE_BTN: is implicitly handled here too */
		    case FILE_SAVE_AS_BTN:
			brandNewFile = False;
			select = (XmSelectionBoxCallbackStruct *)call_data;
			XmStringGetLtoR(select->value,
					XmSTRING_DEFAULT_CHARSET,&filename);
			if (!access(filename,W_OK)) {
			/* found writable file by that name */
			    suffixPosition = strstr(filename,
					DISPLAY_FILE_ASCII_SUFFIX);
			    if (suffixPosition == NULL) {
				sprintf(warningString,
	"Invalid filename:\n\n  %s\n\n(requires .adl suffix) aborting SAVE!",
					filename);
				dmSetAndPopupWarningDialog(currentDisplayInfo,
					warningString,
					(XtCallbackProc)warnCallback,
					(XtCallbackProc)warnCallback);
			    	XtFree(filename);
				return;
			    }
			/* get file status */
			    status = stat(filename,&statBuf);
			    if (status != 0) {
				sprintf(warningString,
	"Cannot get status information for:\n\n  %s\n\naborting SAVE!",
					filename);
				dmSetAndPopupWarningDialog(currentDisplayInfo,
					warningString,
					(XtCallbackProc)warnCallback,
					(XtCallbackProc)warnCallback);
			    	XtFree(filename);
				return;
			    }
			    appendPosition = suffixPosition - filename;
			    strncpy(backupFilename,filename,appendPosition);
			    backupFilename[appendPosition] = '\0';
			    strcat(backupFilename,DISPLAY_FILE_BACKUP_SUFFIX);
			    strcat(backupFilename,DISPLAY_FILE_ASCII_SUFFIX);
			    status = rename(filename,backupFilename);
			    if (status != 0) {
				sprintf(warningString,
		  "Can't move existing\n\n  %s\n\nto\n\n  %s\n\naborting SAVE!",
					filename,backupFilename);
				dmSetAndPopupWarningDialog(currentDisplayInfo,
					warningString,
					(XtCallbackProc)warnCallback,
					(XtCallbackProc)warnCallback);
			    	XtFree(filename);
				return;
			    }
			} else {
			  if (errno != ENOENT) {
/* if an error other than "no entry" occurred, then bail out */
			    sprintf(warningString,
	"Cannot write/create file:\n\n  %s\n\naborting SAVE!",filename);
			    dmSetAndPopupWarningDialog(currentDisplayInfo,
					warningString,
					(XtCallbackProc)warnCallback,
					(XtCallbackProc)warnCallback);
			    XtFree(filename);
			    return;
			  } else {
			    brandNewFile = True;
			  }
			}
			stream = fopen(filename,"w");
			if (stream == NULL) {
			    sprintf(warningString,"%s%s%s",
			       "Unable to write to file:\n\n    ",filename,
			       "\n\nPlease enter new filename");
			    warningXmstring = XmStringCreateLtoR(
					warningString,XmFONTLIST_DEFAULT_TAG);
			    XtVaSetValues(saveAsPD,XmNselectionLabelString,
					warningXmstring,NULL);
			    XtManageChild(saveAsPD);
			    XmStringFree(warningXmstring);
			    XtFree(filename);
			    return;
			}
			dmSetDisplayFileName(currentDisplayInfo,filename);
			dmWriteDisplayList(currentDisplayInfo,stream);
			fclose(stream);
			if (!brandNewFile) {
		/* now set the new file's permissions to be same as old one's */
			  chmod(filename,statBuf.st_mode);
			}

/* just give filename as title */
			startPos = 0;
			for (k = 0; k < strlen(filename); k++) {
			  if (filename[k] == '/') startPos = k;
			}
			XtVaSetValues(currentDisplayInfo->shell,XmNtitle,
			      &(filename[startPos > 0 ? startPos+1 : 0]),NULL);
			sprintf(warningString,"%s",
			      "Name of file to save display in:");
			warningXmstring = XmStringCreateSimple(
					warningString);
			XtVaSetValues(saveAsPD,XmNselectionLabelString,
					warningXmstring,NULL);
			XmStringFree(warningXmstring);
			XtFree(filename);
			break;

		    case FILE_EXIT_BTN:
			medmClearImageCache();
			dmTerminateX();
			dmTerminateCA();
			exit(0);
			break;
		}

  }
}





static XtCallbackProc palettesMenuSimpleCallback(Widget w, int buttonNumber,
			XmAnyCallbackStruct *call_data)
{
    switch(buttonNumber) {

	case PALETTES_OBJECT_BTN:
		/* fills in global objectMW */
		if (objectMW == NULL) createObject();
		XtPopup(objectS,XtGrabNone);
		break;

	case PALETTES_RESOURCE_BTN:
		/* fills in global resourceMW */
		if (resourceMW == NULL) createResource();
		/* (MDA) this is redundant - done at end of createResource() */
		XtPopup(resourceS,XtGrabNone);
		break;

	case PALETTES_COLOR_BTN:
		/* fills in global colorMW */
		if (colorMW == NULL)
		   setCurrentDisplayColorsInColorPalette(BCLR_RC,0);
		XtPopup(colorS,XtGrabNone);
		break;

#ifdef EXTENDED_INTERFACE
	case PALETTES_CHANNEL_BTN:
		/* fills in global channelMW */
		if (channelMW == NULL) createChannel();
		XtPopup(channelS,XtGrabNone);
		break;
#endif
    }

}




static XtCallbackProc helpMenuSimpleCallback(Widget w, int buttonNumber,
				XmAnyCallbackStruct *call_data)
{
  Widget widget;
  XEvent event;

    switch(buttonNumber) {
    /* implement context sensitive help */
	case HELP_ON_CONTEXT_BTN:
		widget = XmTrackingEvent(mainShell,helpCursor,False,&event);
		if (widget != (Widget)NULL) {
		  call_data->reason = XmCR_HELP;
		  XtCallCallbacks(widget,XmNhelpCallback,&call_data);
		}
		break;
	case HELP_ON_WINDOW_BTN:
	case HELP_ON_KEYS_BTN:
	case HELP_INDEX_BTN:
	case HELP_ON_HELP_BTN:
	case HELP_TUTORIAL_BTN:
		break;
	case HELP_ON_VERSION_BTN:
		XtPopup(productDescriptionShell,XtGrabNone);
		break;
    }

}



static XtCallbackProc helpDialogCallback(
  Widget w,
  XtPointer client_data,
  XmAnyCallbackStruct *call_data)
{
  switch(call_data->reason){
	case XmCR_OK:
	case XmCR_CANCEL:
		XtPopdown(helpS);
		break;
  }
}


/* 
 * Jeff claims you need this even if using some fd mgr. funtion (select(fd...))
 *   for refreshing tcp/ip connections, CA "flow control", etc
 */
static XtTimerCallbackProc caHeartBeat(XtPointer dummy)
{
  ca_pend_event(CA_PEND_EVENT_TIME);	/* need this rather than ca_pend_io */

/* reregister 2 second TimeOut function to handle lost connections, etc */
  XtAppAddTimeOut(appContext,2000,(XtTimerCallbackProc)caHeartBeat,NULL);
}


static XtCallbackProc modeCallback(
  Widget w,
  DlTraversalMode mode,
  XmToggleButtonCallbackStruct *call_data)
{
  DisplayInfo *displayInfo, *nextDisplayInfo;
  DlElement *element;

/*
 * since both on & off will invoke this callback, only care about transition
 * of one to ON (with real change of state)
 */
  if (call_data->set == False ||
	globalDisplayListTraversalMode == mode) return;

/*
 * set all the displayInfo->traversalMode(s) to the specified mode, and
 * then invoke the traversal
 */
  globalDisplayListTraversalMode = mode;

/* unselect anything that might be selected */
  highlightAndSetSelectedElements(NULL,0,0);
  clearResourcePaletteEntries();

  switch(mode) {
      case DL_EDIT:
	if (objectS != NULL) XtSetSensitive(objectS,True);
	if (resourceS != NULL) XtSetSensitive(resourceS,True);
	if (colorS != NULL) XtSetSensitive(colorS,True);
	if (channelS != NULL) XtSetSensitive(channelS,True);
	if (relatedDisplayS != NULL) XtSetSensitive(relatedDisplayS,True);
	if (cartesianPlotS != NULL) XtSetSensitive(cartesianPlotS,True);
	if (cartesianPlotAxisS != NULL) XtSetSensitive(cartesianPlotAxisS,True);
	if (stripChartS!= NULL) XtSetSensitive(stripChartS,True);
	XtSetSensitive(mainEditPDM,True);
	/* (MDA) when View done, can uncomment this 
	XtSetSensitive(mainViewPDM,True);
	 */
	XtSetSensitive(mainPalettesPDM,True);
	XtSetSensitive(fileNewBtn,True);
	XtSetSensitive(fileSaveBtn,True);
	XtSetSensitive(fileSaveAsBtn,True);
	break;

      case DL_EXECUTE:
	if (objectS != NULL) XtSetSensitive(objectS,False);
	if (resourceS != NULL) XtSetSensitive(resourceS,False);
	if (colorS != NULL) XtSetSensitive(colorS,False);
	if (channelS != NULL) XtSetSensitive(channelS,False);
	if (relatedDisplayS != NULL) {
	   XtSetSensitive(relatedDisplayS,False);
	   XtPopdown(relatedDisplayS);
	}
	if (cartesianPlotS != NULL) {
	   XtSetSensitive(cartesianPlotS,False);
	   XtPopdown(cartesianPlotS);
	}
	if (cartesianPlotAxisS != NULL) {
	   XtSetSensitive(cartesianPlotAxisS,False);
	   XtPopdown(cartesianPlotAxisS);
	}
	if (stripChartS!= NULL) {
	   XtSetSensitive(stripChartS,False);
	   XtPopdown(stripChartS);
	}
	XtSetSensitive(mainEditPDM, False);
	XtSetSensitive(mainViewPDM,False);
	XtSetSensitive(mainPalettesPDM,False);
	XtSetSensitive(fileNewBtn,False);
	XtSetSensitive(fileSaveBtn,False);
	XtSetSensitive(fileSaveAsBtn,False);
	break;

      default:
	break;
  }

  executeTimeCartesianPlotWidget = NULL;
/* no display is current */
  currentDisplayInfo = (DisplayInfo *)NULL;

  displayInfo = displayInfoListHead->next;
  if (displayInfo == NULL) return;	/* no displays yet */

/* simply return if there is no memory-resident display list */
  if (displayInfo->dlElementListHead == displayInfo->dlElementListTail)
	return;

/* cleanup the resources, CA stuff and timeouts for displays/display lists */
  while (displayInfo != NULL) {
	displayInfo->traversalMode = mode;

	if (displayInfo->fromRelatedDisplayExecution) {
	/* free all resources and remove display info completely */
	  nextDisplayInfo = displayInfo->next;
	  dmRemoveDisplayInfo(displayInfo);
	  displayInfo = nextDisplayInfo;
	} else {
	/* free resources, but don't free the display list in memory */
	  dmCleanupDisplayInfo(displayInfo,False);
	  displayInfo = displayInfo->next;
	}
  }


/* and traverse all display lists */
  dmTraverseAllDisplayLists();

  XFlush(display);
  ca_pend_event(CA_PEND_EVENT_TIME);
  if (mode == DL_EXECUTE) dmProcessCA();

}



static XtCallbackProc mapCallback(
  Widget w,
  XtPointer client_data,
  XmAnyCallbackStruct *call_data)
{
  Position X, Y;
  XmString xmString;

  if (w == closeQD || w == saveAsPD) {
    XtTranslateCoords(currentDisplayInfo->shell,0,0,&X,&Y);
    /* try to force correct popup the first time */
    XtMoveWidget(XtParent(w),X,Y);
  }

  if (w == saveAsPD) {
  /* be nice to the users - supply default text field as display name */
    xmString = XmStringCreateSimple(dmGetDisplayFileName(currentDisplayInfo));
    XtVaSetValues(w,XmNtextString,xmString,NULL);
    XmStringFree(xmString);
  }

}





static void createCursors()
{ 
  XCharStruct overall;
  int dir, asc, desc;
  Pixmap sourcePixmap, maskPixmap;
  XColor colors[2];
  GC gc;

  Dimension hotSpotWidth = HOT_SPOT_WIDTH;
  Dimension radius;

/*
 * create pixmap cursors
 */
  colors[0].pixel = BlackPixel(display,screenNum);
  colors[1].pixel = WhitePixel(display,screenNum);
  XQueryColors(display,cmap,colors,2);

/* CLOSE cursor */
  XTextExtents(fontTable[6],"Close",5,&dir,&asc,&desc,&overall);
  sourcePixmap = XCreatePixmap(display,RootWindow(display,screenNum),
			overall.width+hotSpotWidth,asc+desc,1);
  maskPixmap = XCreatePixmap(display,RootWindow(display,screenNum),
			overall.width+hotSpotWidth,asc+desc,1);
  gc =  XCreateGC(display,sourcePixmap,0,NULL);
  XSetBackground(display,gc,0);
  XSetFunction(display,gc,GXcopy);
  /* an arbitrary modest-sized font from the font table */
  XSetFont(display,gc,fontTable[6]->fid);
  XSetForeground(display,gc,0);
  XFillRectangle(display,sourcePixmap,gc,0,0,overall.width+hotSpotWidth,
			asc+desc);
  XFillRectangle(display,maskPixmap,gc,0,0,overall.width+hotSpotWidth,
			asc+desc);
  radius = MIN(hotSpotWidth,(Dimension)((asc+desc)/2));
  XSetForeground(display,gc,1);
  XFillArc(display,maskPixmap,gc,hotSpotWidth/2 - radius/2,
			(asc+desc)/2 - radius/2,radius,radius,0,360*64);
  XDrawString(display,sourcePixmap,gc,hotSpotWidth,asc,"Close",5);
  XDrawString(display,maskPixmap,gc,hotSpotWidth,asc,"Close",5);
  closeCursor = XCreatePixmapCursor(display,sourcePixmap,maskPixmap,
			&colors[0],&colors[1],0,(asc+desc)/2);
  XFreePixmap(display,sourcePixmap);
  XFreePixmap(display,maskPixmap);

/* SAVE cursor */
  XTextExtents(fontTable[6],"Save",4,&dir,&asc,&desc,&overall);
  sourcePixmap = XCreatePixmap(display,RootWindow(display,screenNum),
			overall.width+hotSpotWidth,asc+desc,1);
  maskPixmap = XCreatePixmap(display,RootWindow(display,screenNum),
			overall.width+hotSpotWidth,asc+desc,1);
  XSetForeground(display,gc,0);
  XFillRectangle(display,sourcePixmap,gc,0,0,overall.width+hotSpotWidth,
			asc+desc);
  XFillRectangle(display,maskPixmap,gc,0,0,overall.width+hotSpotWidth,
			asc+desc);
  radius = MIN(hotSpotWidth,(Dimension)((asc+desc)/2));
  XSetForeground(display,gc,1);
  XFillArc(display,maskPixmap,gc,hotSpotWidth/2 - radius/2,
			(asc+desc)/2 - radius/2,radius,radius,0,360*64);
  XDrawString(display,sourcePixmap,gc,hotSpotWidth,asc,"Save",4);
  XDrawString(display,maskPixmap,gc,hotSpotWidth,asc,"Save",4);
  saveCursor = XCreatePixmapCursor(display,sourcePixmap,maskPixmap,
			&colors[0],&colors[1],0,(asc+desc)/2);
  XFreePixmap(display,sourcePixmap);
  XFreePixmap(display,maskPixmap);

/* PRINT cursor */
  XTextExtents(fontTable[6],"Print",5,&dir,&asc,&desc,&overall);
  sourcePixmap = XCreatePixmap(display,RootWindow(display,screenNum),
			overall.width+hotSpotWidth,asc+desc,1);
  maskPixmap = XCreatePixmap(display,RootWindow(display,screenNum),
			overall.width+hotSpotWidth,asc+desc,1);
  XSetForeground(display,gc,0);
  XFillRectangle(display,sourcePixmap,gc,0,0,overall.width+hotSpotWidth,
			asc+desc);
  XFillRectangle(display,maskPixmap,gc,0,0,overall.width+hotSpotWidth,
			asc+desc);
  radius = MIN(hotSpotWidth,(Dimension)((asc+desc)/2));
  XSetForeground(display,gc,1);
  XFillArc(display,maskPixmap,gc,hotSpotWidth/2 - radius/2,
			(asc+desc)/2 - radius/2,radius,radius,0,360*64);
  XDrawString(display,sourcePixmap,gc,hotSpotWidth,asc,"Print",5);
  XDrawString(display,maskPixmap,gc,hotSpotWidth,asc,"Print",5);
  printCursor = XCreatePixmapCursor(display,sourcePixmap,maskPixmap,
			&colors[0],&colors[1],0,(asc+desc)/2);
  XFreePixmap(display,sourcePixmap);
  XFreePixmap(display,maskPixmap);
  XFreeGC(display,gc);

/*
 * now create standard font cursors
 */
  helpCursor = XCreateFontCursor(display,XC_question_arrow);
  crosshairCursor = XCreateFontCursor(display,XC_crosshair);
  watchCursor = XCreateFontCursor(display,XC_watch);
  xtermCursor = XCreateFontCursor(display,XC_xterm);

  rubberbandCursor = XCreateFontCursor(display,XC_hand2);
  dragCursor = XCreateFontCursor(display,XC_fleur);
  resizeCursor = XCreateFontCursor(display,XC_sizing);

}



/*
 * globals needed for remote display invocation...
 *   have limited scope (main() and dmTerminateX() are only routines which care
 */
Atom MEDM_EDIT_FIXED = (Atom)NULL, MEDM_EXEC_FIXED = (Atom)NULL,
	MEDM_EDIT_SCALABLE = (Atom)NULL, MEDM_EXEC_SCALABLE = (Atom)NULL;


/* 
 * SIGNAL HANDLER
 *   function to perform cleanup of X root window MEDM... properties
 *   which accomodate remote display requests
 */
static void handleSignals(int sig)
{
if (sig==SIGQUIT)      fprintf(stderr,"\nSIGQUIT\n");
else if (sig==SIGINT)  fprintf(stderr,"\nSIGINT\n");
else if (sig==SIGTERM) fprintf(stderr,"\nSIGTERM\n");
else if (sig==SIGSEGV) fprintf(stderr,"\nSIGSEGV\n");
else if (sig==SIGBUS) fprintf(stderr,"\nSIGBUS\n");

  /* remove the properties on the root window */
  if (MEDM_EDIT_FIXED != (Atom)NULL)
        XDeleteProperty(display,rootWindow,MEDM_EDIT_FIXED);
  if (MEDM_EXEC_FIXED != (Atom)NULL)
        XDeleteProperty(display,rootWindow,MEDM_EXEC_FIXED);
  if (MEDM_EDIT_SCALABLE != (Atom)NULL)
        XDeleteProperty(display,rootWindow,MEDM_EDIT_SCALABLE);
  if (MEDM_EXEC_SCALABLE != (Atom)NULL)
        XDeleteProperty(display,rootWindow,MEDM_EXEC_SCALABLE);
  XFlush(display);

  if (sig == SIGSEGV || sig == SIGBUS) {
  /* want core dump */
    abort();
  } else {
  /* and exit */
    exit(0);
  }
}


/*
 * the function to take a full path name and macro string,
 *  and send it as a series of clientMessage events to the root window
 *  with the specified Atom.
 *
 *  this allows the "smart-startup" feature to be independent of
 *  the actual property value (since properties can change faster
 *  than MEDM can handle them).
 *
 *  client message events are sent as full path names, followed by
 *  a semi-colon delimiter, followed by an optional macro string,
 *  delimited by parentheses {e.g., (/abc/def;a=b,c=d) or (/abc/def)}
 *  to allow the receiving client to figure out
 *  when it has received all the relevant information for that
 *  request.  (i.e., the string will usually be split up across
 *  events, therefore the parentheses mechanism allows the receiver to
 *  concatenate just enough client message events to properly handle the
 *  request).
 */

void sendFullPathNameAndMacroAsClientMessages(
  Window targetWindow,
  char *fullPathName,
  char *macroString,
  Atom atom)
{
  XClientMessageEvent clientMessageEvent;
  int index, i;
  char *ptr;

#define MAX_CHARS_IN_CLIENT_MESSAGE 20	/* as defined in XClientEventMessage */

  clientMessageEvent.type = ClientMessage;
  clientMessageEvent.serial = 0;
  clientMessageEvent.send_event = True;
  clientMessageEvent.display = display;
  clientMessageEvent.window = targetWindow;
  clientMessageEvent.message_type = atom;
  clientMessageEvent.format = 8;
  ptr = fullPathName;
/* leading "(" */
  clientMessageEvent.data.b[0] = '(';
  index = 1;

/* body of full path name string */
  while (ptr[0] != '\0') {
    if (index == MAX_CHARS_IN_CLIENT_MESSAGE) {
      XSendEvent(display,targetWindow,True,NoEventMask,
		(XEvent *)&clientMessageEvent);
      index = 0;
    }
    clientMessageEvent.data.b[index++] = ptr[0];
    ptr++;
  }

/* body of macro string if one was specified */
  if (macroString != NULL) {
  /* ; delimiter */
    if (index == MAX_CHARS_IN_CLIENT_MESSAGE) {
      XSendEvent(display,targetWindow,True,NoEventMask,
		(XEvent *)&clientMessageEvent);
      index = 0;
    }
    clientMessageEvent.data.b[index++] = ';';

    ptr = macroString;
    while (ptr[0] != '\0') {
      if (index == MAX_CHARS_IN_CLIENT_MESSAGE) {
        XSendEvent(display,targetWindow,True,NoEventMask,
		(XEvent *)&clientMessageEvent);
        index = 0;
      }
      clientMessageEvent.data.b[index++] = ptr[0];
      ptr++;
    }
  }


/* trailing ")" */
  if (index == MAX_CHARS_IN_CLIENT_MESSAGE) {
    XSendEvent(display,targetWindow,True,NoEventMask,
		(XEvent *)&clientMessageEvent);
    index = 0;
  }
  clientMessageEvent.data.b[index++] = ')';
/* fill out client event with spaces just for "cleanliness" */
  for (i = index; i < MAX_CHARS_IN_CLIENT_MESSAGE; i++)
	clientMessageEvent.data.b[i++] = ' ';
  XSendEvent(display,targetWindow,True,NoEventMask,
		(XEvent *)&clientMessageEvent);

}




/***********************************************************************
 ************ main()
 ***********************************************************************/

main(int argc, char *argv[])
{
  int c, i, n, index;
  Arg args[5];
  Pixel greyPixel;
  FILE *filePtr;
  XColor color;
  XEvent event;
  char *displayName = NULL;
  char versionString[60];
#define FONT_NAME_SIZE 80
  char displayFont[FONT_NAME_SIZE];
  Window targetWindow;

  int savedIndex, j;
  Boolean canAccess, medmAlreadyRunning, atLeastOneValidFile,
	completeClientMessage;
#define FULLPATHNAME_SIZE 256
  char fullPathName[FULLPATHNAME_SIZE+1],
	currentDirectoryName[FULLPATHNAME_SIZE+1], name[FULLPATHNAME_SIZE+1];
  char *dir;
  unsigned char *propertyData;
  int status, format;
  unsigned long nitems, left;
  Atom type;
  Boolean localMode = False, cleanupMode = False;
  int startPos, quoteIndex;
  char *macroString = NULL, *macroBuffer = NULL, *ptr;
  XColor colors[2];

typedef enum {EDIT,EXECUTE,ERROR} OpMode;
  OpMode opMode = EDIT;

typedef enum {FIXED,SCALABLE} FontStyle;
  FontStyle fontStyle = FIXED;

  Window execFixedTargetWindow = (Window)0, editFixedTargetWindow = (Window)0,
     execScalableTargetWindow = (Window)0, editScalableTargetWindow = (Window)0;


/*
 * initialize channel access here (to get around orphaned windows)
 */
  SEVCHK(ca_task_initialize(),"\nmain: error in ca_task_initialize");

/* initialize a few globals */
  privateCmap = False;

/*
 * allow for quick startup (using existing MEDM)
 *  open display, check for atoms, etc
 */
  savedIndex = 1;

/* make font aliases the default for a while yet */
  strcpy(displayFont,FONT_ALIASES_STRING);

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i],"-x")) {
	opMode = EXECUTE; savedIndex = i;
    } else if (!strcmp(argv[i],"-e")) {
	opMode = EDIT; savedIndex = i;
    } else if (!strcmp(argv[i],"-?")) {
	opMode = ERROR; savedIndex = i;
    } else if (!strcmp(argv[i],"-local")) {
	localMode = True; savedIndex = i;
    } else if (!strcmp(argv[i],"-cleanup")) {
	cleanupMode = True; savedIndex = i;
    } else if (!strcmp(argv[i],"-cmap")) {
	privateCmap = True; savedIndex = i;
    } else if (!strcmp(argv[i],"-macro")) {
	savedIndex = i;
	macroString = ( ((i+1) < argc) ? argv[i+1] : NULL); savedIndex = i+1;
	if (macroString != NULL) {
	  macroBuffer = STRDUP(macroString);
  /* since parameter of form   -macro "a=b,c=d,..."  replace '"' with ' ' */
	  if (macroBuffer != NULL) {
	    if (macroBuffer[0] == '"') macroBuffer[0] = ' ';
	    quoteIndex = strlen(macroBuffer)-1;
	    if (macroBuffer[0] == '"') macroBuffer[0] = ' ';
	    if (macroBuffer[quoteIndex] == '"') macroBuffer[quoteIndex] = ' ';
	    
	  }
	} else {
	  macroBuffer = NULL;
	}
    } else if (!strcmp(argv[i],"-displayFont")) {
	strncpy(displayFont,argv[++i],FONT_NAME_SIZE-1);
	displayFont[FONT_NAME_SIZE-1] = '\0';
	savedIndex = i;
	if (!strcmp(displayFont,FONT_ALIASES_STRING))
	    fontStyle = FIXED;
	else if (!strcmp(displayFont,DEFAULT_SCALABLE_STRING))
	    fontStyle = SCALABLE;
    } else if (!strcmp(argv[i],"-display")) {
	displayName = ( ((i+1) < argc) ? argv[i+1] : NULL); savedIndex = i+1;
    }
  }
  if (macroBuffer != NULL && opMode != EXECUTE) {
    fprintf(stderr,"\nmedm: %s %s","-macro command line option only valid",
	"for execute (-x) mode operation");
    free(macroBuffer);
    macroBuffer = NULL;
  }
  if (opMode == ERROR) {
    fprintf(stderr,
   "\nusage: medm -x  files... for execution  or  medm -e  files... for edit");
    fprintf(stderr,
   "\n       -local  for forced local display/execution...\n");
    exit(2);
  }

  if (!localMode) {

/* do remote protocol stuff */

    display = XOpenDisplay(displayName);
    if (display == NULL) {
      fprintf(stderr,"\nmedm: could not open Display!");
      exit(0);
    }
    screenNum = DefaultScreen(display);
    rootWindow = RootWindow(display,screenNum);

/* don't create the atom if it doesn't exist - this tells us if another
 *  instance of MEDM is already running in proper startup mode (-e or -x) */
    if (opMode == EXECUTE) {
      if (fontStyle == FIXED) {
        MEDM_EXEC_FIXED = XInternAtom(display,"MEDM_EXEC_FIXED",False);
        status = XGetWindowProperty(display,rootWindow,MEDM_EXEC_FIXED,
		0,FULLPATHNAME_SIZE,(Bool)False,AnyPropertyType,&type,
		&format,&nitems,&left,&propertyData);
        if (type != None) {
	  execFixedTargetWindow = *((Window *)propertyData);
	  if (cleanupMode) medmAlreadyRunning = False;
	  else medmAlreadyRunning = True;
	  XFree(propertyData);
        } else
	  medmAlreadyRunning = False;
      } else if (fontStyle == SCALABLE) {
        MEDM_EXEC_SCALABLE = XInternAtom(display,"MEDM_EXEC_SCALABLE",False);
        status = XGetWindowProperty(display,rootWindow,MEDM_EXEC_SCALABLE,
		0,FULLPATHNAME_SIZE,(Bool)False,AnyPropertyType,&type,
		&format,&nitems,&left,&propertyData);
        if (type != None) {
	  execScalableTargetWindow = *((Window *)propertyData);
	  if (cleanupMode) medmAlreadyRunning = False;
	  else medmAlreadyRunning = True;
	  XFree(propertyData);
        } else
	  medmAlreadyRunning = False;
      }
	
    } else if (opMode == EDIT) {
      if (fontStyle == FIXED) {
        MEDM_EDIT_FIXED = XInternAtom(display,"MEDM_EDIT_FIXED",False);
        status = XGetWindowProperty(display,rootWindow,MEDM_EDIT_FIXED,
		0,FULLPATHNAME_SIZE,(Bool)False,AnyPropertyType,&type,
		&format,&nitems,&left,&propertyData);
        if (type != None) {
	  editFixedTargetWindow = *((Window *)propertyData);
	  if (cleanupMode) medmAlreadyRunning = False;
	  else medmAlreadyRunning = True;
	  XFree(propertyData);
        } else
	  medmAlreadyRunning = False;
      } else if (fontStyle == SCALABLE) {
        MEDM_EDIT_SCALABLE = XInternAtom(display,"MEDM_EDIT_SCALABLE",False);
        status = XGetWindowProperty(display,rootWindow,MEDM_EDIT_SCALABLE,
		0,FULLPATHNAME_SIZE,(Bool)False,AnyPropertyType,&type,
		&format,&nitems,&left,&propertyData);
        if (type != None) {
	  editScalableTargetWindow = *((Window *)propertyData);
	  if (cleanupMode) medmAlreadyRunning = False;
	  else medmAlreadyRunning = True;
	  XFree(propertyData);
        } else
	  medmAlreadyRunning = False;
      }
    }


    if (medmAlreadyRunning) {
/*
 * go get the requested files (*.adl), convert to full path name if necessary,
 *   and change the property to initiate the display request to the
 *   remote MEDM
 */
      atLeastOneValidFile = False;
      fullPathName[0] = '\0';
      currentDirectoryName[0] = '\0';
      if (savedIndex < argc) { /* we potentially have displays to fire up */
        for (j = savedIndex; j < argc; j++) {
	  if (strstr(argv[j],DISPLAY_FILE_ASCII_SUFFIX)) {
	/* found string with right suffix - presume it's a valid display name */
	    canAccess = !access(argv[j],R_OK);
	    if (canAccess) {
/* found locally */
	      if (argv[j][0] == '/') {
		/* already a full pathname */
		strcpy(fullPathName,argv[j]);
	      } else {
		/* not a full pathanme, build a full pathname */
		if (currentDirectoryName[0] == '\0') {
		  getcwd(currentDirectoryName,FULLPATHNAME_SIZE);
		}
		strcpy(fullPathName,currentDirectoryName);
		strcat(fullPathName,"/");
		strcat(fullPathName,argv[j]);
	      }
	    } else {
/* try EPICS_DISPLAY_PATH directory */
	      dir = getenv(DISPLAY_LIST_ENV);
	      if (dir != NULL) {
		startPos = 0;
		while (!canAccess && extractStringBetweenColons(dir,name,
					startPos,&startPos)) {
		  strcpy(fullPathName,name);
		  strcat(fullPathName,"/");
		  strcat(fullPathName,argv[j]);
		  canAccess = !access(fullPathName,R_OK);
		}
	      }
	    }
	    if (canAccess) {
	      atLeastOneValidFile = True;
	      fprintf(stderr,
	      "\n  - dispatched display request to remote MEDM:\n    \"%s\"",
		fullPathName);
	      if (opMode == EXECUTE) {
		if (fontStyle == FIXED)
		  sendFullPathNameAndMacroAsClientMessages(
			execFixedTargetWindow,fullPathName,macroBuffer,
			MEDM_EXEC_FIXED);
		else if (fontStyle == SCALABLE)
		  sendFullPathNameAndMacroAsClientMessages(
			execScalableTargetWindow,fullPathName,macroBuffer,
			MEDM_EXEC_SCALABLE);
	      } else if (opMode == EDIT) {
		if (fontStyle == FIXED)
		  sendFullPathNameAndMacroAsClientMessages(
			editFixedTargetWindow,fullPathName,macroBuffer,
			MEDM_EDIT_FIXED);
		else if (fontStyle == SCALABLE)
		  sendFullPathNameAndMacroAsClientMessages(
			editScalableTargetWindow,fullPathName,macroBuffer,
			MEDM_EDIT_SCALABLE);
	      }
	      XFlush(display);
	    } else {
	      fprintf(stderr,"\n  - cannot access \"%s\"",argv[j]);
	    }
	  } /* end if (has right suffix) */
        } /* end for */
	if (atLeastOneValidFile) {
	  fprintf(stderr,
 "\n\n    (use -local for private copy or -cleanup to ignore existing MEDM)\n");
	}
      } /* end if (have displays to fire up) */
        if (!atLeastOneValidFile) fprintf(stderr,"%s%s",
   "\n  - no display to dispatch, and already a remote MEDM running:",
   "\n    (use -local for private copy or -cleanup to ignore existing MEDM)\n");
      exit(0);
    }

/************ we leave here if another MEDM can do our work  **************/

    XCloseDisplay(display);
  } /* end if (!localMode) */


/*
 * initialize the Intrinsics..., create main shell
 */   
  n = 0;
/* map window manager menu Close function to application close... */
  XtSetArg(args[n],XmNdeleteResponse,XmDO_NOTHING); n++;
  XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
  mainShell = XtAppInitialize(&appContext, CLASS, NULL, 0, &argc, argv,
		fallbackResources, args, n);

/* add necessary Motif resource converters */
  XmRegisterConverters();
  XmRepTypeInstallTearOffModelConverter();

  display = XtDisplay(mainShell);
  if (display == NULL) {
      XtWarning("cannot open display");
      exit(-1);
  }
#ifdef DEBUG
  XSynchronize(display,TRUE);
  fprintf(stderr,"\nRunning in SYNCHRONOUS mode!!");
#endif

  WM_DELETE_WINDOW = XmInternAtom(display,"WM_DELETE_WINDOW",False);
  WM_TAKE_FOCUS =  XmInternAtom(display,"WM_TAKE_FOCUS",False);
  COMPOUND_TEXT =  XmInternAtom(display,"COMPOUND_TEXT",False);
  screenNum = DefaultScreen(display);
  rootWindow = RootWindow(display,screenNum);
  cmap = DefaultColormap(display,screenNum);	/* X default colormap */

  if (privateCmap) {
 /* cheap/easy way to get colormap - do real PseudoColor cmap alloc later
  *  note this really creates a colormap for default visual with no
  *  entries
  */
    cmap = XCopyColormapAndFree(display,cmap);
    XtVaSetValues(mainShell,XmNcolormap,cmap,NULL);
  }


/*
 * for remote display start-up:
 *  register signal handlers to assure property states (unfortunately
 *    SIGKILL, SIGSTOP can't be caught...)
 */
  signal(SIGQUIT,handleSignals);
  signal(SIGINT, handleSignals);
  signal(SIGTERM,handleSignals);
  signal(SIGSEGV,handleSignals);
  signal(SIGBUS,handleSignals);


/* add translations/actions for drag-and-drop */
  parsedTranslations = XtParseTranslationTable(dragTranslations);
  XtAppAddActions(appContext,dragActions,XtNumber(dragActions));

  if (opMode == EDIT) {
    globalDisplayListTraversalMode = DL_EDIT;
  } else if (opMode == EXECUTE) {
    globalDisplayListTraversalMode = DL_EXECUTE;
    if (savedIndex < argc-1) {	/* assume .adl file names follow */
	XtSetArg(args[0],XmNinitialState,IconicState);
	XtSetValues(mainShell,args,1);
    }
  } else {
    globalDisplayListTraversalMode = DL_EDIT;
  }

/*
 * initialize some globals
 */
  globalModifiedFlag = False;
  mainMW = NULL;
  objectS = NULL; objectMW = NULL;
  colorS = NULL; colorMW = NULL;
  resourceS = NULL; resourceMW = NULL,
  channelS = NULL;channelMW = NULL;
  relatedDisplayS = NULL; shellCommandS = NULL;
  cartesianPlotS = NULL; cartesianPlotAxisS = NULL; stripChartS = NULL;
  cpAxisForm = NULL; executeTimeCartesianPlotWidget = NULL;
  currentDisplayInfo = NULL;
  pointerInDisplayInfo = NULL;
  resourceBundleCounter = 0;
  clipboardDelete = False;
  currentElementType = 0;

  /* not really unphysical, but being used for unallocable color cells */
  unphysicalPixel = BlackPixel(display,screenNum);

/* initialize the default colormap */

  if (privateCmap) {
  /* first add in black and white pixels to match [Black/White]Pixel(dpy,scr) */
    colors[0].pixel = BlackPixel(display,screenNum);
    colors[1].pixel = WhitePixel(display,screenNum);
    XQueryColors(display,DefaultColormap(display,screenNum),colors,2);
  /* need to allocate 0 pixel first, then 1 pixel, usually Black, White...
   *  note this is slightly risky in case of non Pseudo-Color visuals I think,
   *  but the preallocated colors of Black=0, White=1 for Psuedo-Color
   *  visuals is common, and only for Direct/TrueColor visuals will
   *  this be way off, but then we won't be using the private colormap
   *  since we won't run out of colors in that instance...
   */
    if (colors[0].pixel == 0) XAllocColor(display,cmap,&(colors[0]));
    else XAllocColor(display,cmap,&(colors[1]));
    if (colors[1].pixel == 1) XAllocColor(display,cmap,&(colors[1]));
    else XAllocColor(display,cmap,&(colors[0]));
  }

  for (i = 0; i < DL_MAX_COLORS; i++) {

  /* scale [0,255] to [0,65535] */
    color.red  = (unsigned short) COLOR_SCALE*(defaultDlColormap.dl_color[i].r);
    color.green= (unsigned short) COLOR_SCALE*(defaultDlColormap.dl_color[i].g);
    color.blue = (unsigned short) COLOR_SCALE*(defaultDlColormap.dl_color[i].b);
  /* allocate a shareable color cell with closest RGB value */
    if (XAllocColor(display,cmap,&color)) {
	defaultColormap[i] =  color.pixel;
    } else {
	fprintf(stderr,"\nmain: couldn't allocate requested color");
	/* put unphysical pixmap value in there as tag it was invalid */
	defaultColormap[i] =  unphysicalPixel;
    }

  }
  currentColormap = defaultColormap;
  currentColormapSize = DL_MAX_COLORS;

  /* and initialize the global resource bundle */
  initializeGlobalResourceBundle();
  globalResourceBundle.next = NULL;
  globalResourceBundle.prev = NULL;

  /* default action for MB in display is select (regulated by object palette) */
  currentActionType = SELECT_ACTION;

/*
 * intialize MEDM stuff
 */
  medmInit(displayFont);
  medmInitializeImageCache();

/*
 * create the cursors
 */
  createCursors();

/*
 * setup for rubber-banding in display editing
 */
 initializeRubberbanding();

/*
 * and create the main window area
 */
  createMain();

/*
 *  ...we're the first MEDM around in this mode - proceed with full execution
 *   but store dummy property first
 */
  targetWindow = XtWindow(mainShell);
  if (opMode == EXECUTE) {
    if (fontStyle == FIXED) {
      if (MEDM_EXEC_FIXED != (Atom)NULL)
	XChangeProperty(display,rootWindow,MEDM_EXEC_FIXED,
          XA_WINDOW,32,PropModeReplace,(unsigned char *)&targetWindow,1);
    } else if (fontStyle == SCALABLE) {
      if (MEDM_EXEC_SCALABLE != (Atom)NULL)
	XChangeProperty(display,rootWindow,MEDM_EXEC_SCALABLE,
          XA_WINDOW,32,PropModeReplace,(unsigned char *)&targetWindow,1);
    }
  } else if (opMode == EDIT) {
    if (fontStyle == FIXED) {
      if (MEDM_EDIT_FIXED != (Atom)NULL)
	XChangeProperty(display,rootWindow,MEDM_EDIT_FIXED,
          XA_WINDOW,32,PropModeReplace,(unsigned char *)&targetWindow,1);
    } else if (fontStyle == SCALABLE) {
      if (MEDM_EDIT_SCALABLE != (Atom)NULL)
	XChangeProperty(display,rootWindow,MEDM_EDIT_SCALABLE,
          XA_WINDOW,32,PropModeReplace,(unsigned char *)&targetWindow,1);
    }
  }


/*
 * start any command-line specified displays
 */
  currentDirectoryName[0] = '\0';
  for (i = 1; i < argc; i++) {
    if (strstr(argv[i],DISPLAY_FILE_ASCII_SUFFIX)) {
      if (!access(argv[i],F_OK|R_OK)) {
        filePtr = fopen(argv[i],"r");
        if (filePtr != NULL) {
	    fullPathName[0] = '\0';
	    if (argv[i][0] == '/') {
		/* already a full pathname */
		strcpy(fullPathName,argv[i]);
	    } else {
		/* not a full pathanme, build a full pathname */
		if (currentDirectoryName[0] == '\0') {
		  getcwd(currentDirectoryName,FULLPATHNAME_SIZE);
		}
		strcpy(fullPathName,currentDirectoryName);
		strcat(fullPathName,"/");
		strcat(fullPathName,argv[i]);
	    }
            dmDisplayListParse(filePtr,macroBuffer,fullPathName,(Widget)NULL,
					(Boolean)False);
	    fclose(filePtr);
        }
      } else {
        fprintf(stderr,"\nmedm: can't open display file: \"%s\"",argv[i]);
      }
    }
  }


/*
 * create and popup the product description shell
 *  (use defaults for fg/bg)
 */

  sprintf(versionString,"%s  (%s)",MEDM_VERSION_STRING,EPICS_VERSION_STRING);
  productDescriptionShell = createAndPopupProductDescriptionShell(appContext,
	mainShell,
        "MEDM", fontListTable[8],
	(Pixmap)NULL,
        "Motif-based Editor & Display Manager",
	fontListTable[6],
        versionString,
        "developed at Argonne National Laboratory, by Mark Anderson & Fred Vong",
	fontListTable[4],
        -1, -1, 3);

/* need this later than shell creation for some reason (?) */
  XmAddWMProtocolCallback(mainShell,WM_DELETE_WINDOW,
		(XtCallbackProc)wmCloseCallback, (XtPointer) OTHER_SHELL);


/* add 2 second TimeOut function to handle lost CA connections, etc */
  XtAppAddTimeOut(appContext,2000,(XtTimerCallbackProc)caHeartBeat,NULL);


/*
 * now go into event loop - formerly XtAppMainLoop(appContext);
 */

  while (True) {
    XtAppNextEvent(appContext,&event);
    switch (event.type) {
      case ClientMessage:
	if ( (event.xclient.message_type == MEDM_EDIT_FIXED &&
		opMode == EDIT && fontStyle == FIXED) ||
	     (event.xclient.message_type == MEDM_EXEC_FIXED &&
		opMode == EXECUTE && fontStyle == FIXED) ||
	     (event.xclient.message_type == MEDM_EDIT_SCALABLE &&
		opMode == EDIT && fontStyle == SCALABLE) ||
	     (event.xclient.message_type == MEDM_EXEC_SCALABLE &&
		opMode == EXECUTE && fontStyle == SCALABLE) ) {

/* concatenate clientMessage events to get full name from form: (xyz) */
	  completeClientMessage = False;
	  for (i = 0; i < MAX_CHARS_IN_CLIENT_MESSAGE; i++) {
	    switch (event.xclient.data.b[i]) {
		/* start with filename */
		case '(':  index = 0;
			   ptr = fullPathName;
			   break;
		/* keep filling in until ';', then start macro string if any */
		case ';':  ptr[index++] = '\0';
			   ptr = name;
			   index = 0;
			   break;
		/* terminate whatever string is being filled in */
		case ')':  completeClientMessage = True;
			   ptr[index++] = '\0';
			   break;
		default:   ptr[index++] = event.xclient.data.b[i];
			   break;
	    }
	  }

	  if (completeClientMessage) {
	    filePtr = fopen(fullPathName,"r");
	    if (filePtr != NULL) {
	      dmDisplayListParse(filePtr,name,fullPathName,(Widget)NULL,
				(Boolean)False);
	      fclose(filePtr);
	    } else {
fprintf(stderr,
"\nMEDM: could not open requested file\n\t\"%s\"\n  from remote MEDM request\n",
  fullPathName);
	    }
	  }

	} else
	  XtDispatchEvent(&event);
	break;

	default:
	  XtDispatchEvent(&event);
    }
  }


}






static void createMain()
{

  XmString buttons[N_MAX_MENU_ELES];
  KeySym keySyms[N_MAX_MENU_ELES];
  String accelerators[N_MAX_MENU_ELES];
  XmString acceleratorText[N_MAX_MENU_ELES];
  XmString label, string, cwdXmString;
  XmButtonType buttonType[N_MAX_MENU_ELES];
  Widget mainMB, mainBB, frame, frameLabel;
  Widget mainFilePDM, mainHelpPDM;
  Widget menuHelpWidget, modeRB, modeEditTB, modeExecTB;
  char *cwd;
  char name[12];
  int i, j, n;
  Arg args[20];

  n = 0;
/* create a main window child of the main shell */
  mainMW = XmCreateMainWindow(mainShell,"mainMW",args,n);

/* get default fg/bg colors from mainMW for later use */
  XtSetArg(args[0],XmNbackground,&defaultBackground);
  XtSetArg(args[1],XmNforeground,&defaultForeground);
  XtGetValues(mainMW,args,2);

/*
 * create the menu bar
 */
  buttons[0] = XmStringCreateSimple("File");
  buttons[1] = XmStringCreateSimple("Edit");
  buttons[2] = XmStringCreateSimple("View");
  buttons[3] = XmStringCreateSimple("Palettes");
  buttons[4] = XmStringCreateSimple("Help");
  keySyms[0] = 'F';
  keySyms[1] = 'E';
  keySyms[2] = 'V';
  keySyms[3] = 'P';
  keySyms[4] = 'H';
  n = 0;
  XtSetArg(args[n],XmNbuttonCount,N_MAIN_MENU_ELES); n++;
  XtSetArg(args[n],XmNbuttons,buttons); n++;
  XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
  XtSetArg(args[n],XmNforeground,defaultForeground); n++;
  XtSetArg(args[n],XmNbackground,defaultBackground); n++;
  mainMB = XmCreateSimpleMenuBar(mainMW,"mainMB",args,n);

/* color mainMB properly (force so VUE doesn't interfere) */
  colorMenuBar(mainMB,defaultForeground,defaultBackground);


/* dig out the Help cascade button  and set the resource in the menu bar */
  menuHelpWidget = XtNameToWidget(mainMB,"*button_5");
  XtVaSetValues(mainMB,XmNmenuHelpWidget,menuHelpWidget,
		NULL);
  for (i = 0; i < N_MAIN_MENU_ELES; i++) XmStringFree(buttons[i]);


/*
 * create the file pulldown menu pane
 */
  buttons[0] = XmStringCreateSimple("New");
  buttons[1] = XmStringCreateSimple("Open...");
  buttons[2] = XmStringCreateSimple("Save");
  buttons[3] = XmStringCreateSimple("Save As...");
  buttons[4] = XmStringCreateSimple("Close");
  buttons[5] = XmStringCreateSimple("Print...");
  buttons[6] = XmStringCreateSimple("Separator");
  buttons[7] = XmStringCreateSimple("Exit");
  keySyms[0] = 'N';
  keySyms[1] = 'O';
  keySyms[2] = 'S';
  keySyms[3] = 'A';
  keySyms[4] = 'C';
  keySyms[5] = 'P';
  keySyms[6] = ' ';
  keySyms[7] = 'x';
  accelerators[0] = "";
  accelerators[1] = "Ctrl<Key>O";
  accelerators[2] = "Ctrl<Key>S";
  accelerators[3] = "";
  accelerators[4] = "";
  accelerators[5] = "";
  accelerators[6] = "";
  accelerators[7] = "Ctrl<Key>X";
  acceleratorText[0] =  XmStringCreateSimple("");
  acceleratorText[1] = XmStringCreateSimple("Ctrl+O");
  acceleratorText[2] = XmStringCreateSimple("Ctrl+S");
  acceleratorText[3] = XmStringCreateSimple("");
  acceleratorText[4] = XmStringCreateSimple("");
  acceleratorText[5] = XmStringCreateSimple("");
  acceleratorText[6] = XmStringCreateSimple("");
  acceleratorText[7] = XmStringCreateSimple("Ctrl+X");
  for (i = 0; i <= 5; i++) buttonType[i] = XmPUSHBUTTON;
  buttonType[6] = XmSEPARATOR;
  buttonType[7] = XmPUSHBUTTON;
  n = 0;
  XtSetArg(args[n],XmNbuttonCount,N_FILE_MENU_ELES); n++;
  XtSetArg(args[n],XmNbuttons,buttons); n++;
  XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
  XtSetArg(args[n],XmNbuttonType,buttonType); n++;
  XtSetArg(args[n],XmNbuttonAccelerators,accelerators); n++;
  XtSetArg(args[n],XmNbuttonAcceleratorText,acceleratorText); n++;
  XtSetArg(args[n],XmNpostFromButton,FILE_BTN_POSN); n++;
  XtSetArg(args[n],XmNsimpleCallback,fileMenuSimpleCallback); n++;
  mainFilePDM = XmCreateSimplePulldownMenu(mainMB,"mainFilePDM",args,n);
  for (i = 0; i < N_FILE_MENU_ELES; i++) {
    XmStringFree(buttons[i]);
    XmStringFree(acceleratorText[i]);
  }

/* extract button Widget ids for (de)sensitizing as function of mode */
  sprintf(name,"*button_%d",FILE_NEW_BTN);
  fileNewBtn = XtNameToWidget(mainFilePDM,name);
  sprintf(name,"*button_%d",FILE_SAVE_BTN);
  fileSaveBtn = XtNameToWidget(mainFilePDM,name);
  sprintf(name,"*button_%d",FILE_SAVE_AS_BTN);
  fileSaveAsBtn = XtNameToWidget(mainFilePDM,name);

/*
 * create the edit pulldown menu pane
 */
  mainEditPDM = createEditMenu(XmMENU_PULLDOWN,mainMB,"mainEditPDM",
			EDIT_BTN_POSN,NULL);

/* set buttons and edit menu insensitive if in execute mode */
  if (globalDisplayListTraversalMode == DL_EXECUTE) {
    XtSetSensitive(fileNewBtn,False);
    XtSetSensitive(fileSaveBtn,False);
    XtSetSensitive(fileSaveAsBtn,False);
    XtSetSensitive(mainEditPDM,False);
  }


/*
 * create the view pulldown menu pane
 */
  buttons[0] = XmStringCreateSimple("Grid");
  buttons[1] = XmStringCreateSimple("Separator");
  buttons[2] = XmStringCreateSimple("Refresh Screen");
  keySyms[0] = 'G';
  keySyms[1] = 'S';
  keySyms[2] = 'R';
  buttonType[0] = XmCHECKBUTTON;
  buttonType[1] = XmSEPARATOR;
  buttonType[2] = XmPUSHBUTTON;
  n = 0;
  XtSetArg(args[n],XmNbuttonCount,N_VIEW_MENU_ELES); n++;
  XtSetArg(args[n],XmNbuttons,buttons); n++;
  XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
  XtSetArg(args[n],XmNbuttonType,buttonType); n++;
  XtSetArg(args[n],XmNpostFromButton,VIEW_BTN_POSN); n++;
  mainViewPDM = XmCreateSimplePulldownMenu(mainMB,"mainViewPDM",args,n);
  for (i = 0; i < N_VIEW_MENU_ELES; i++) XmStringFree(buttons[i]);
  /* (MDA) for now, disable this menu */
  XtSetSensitive(mainViewPDM,False);

/*
 * create the palettes pulldown menu pane
 */
  buttons[0] = XmStringCreateSimple("Object...");
  buttons[1] = XmStringCreateSimple("Resource...");
  buttons[2] = XmStringCreateSimple("Color...");
#ifdef EXTENDED_INTERFACE
  buttons[3] = XmStringCreateSimple("Channel...");
#endif
  keySyms[0] = 'O';
  keySyms[1] = 'R';
  keySyms[2] = 'C';
#ifdef EXTENDED_INTERFACE
  keySyms[3] = 'h';
#endif
  buttonType[0] = XmPUSHBUTTON;
  buttonType[1] = XmPUSHBUTTON;
  buttonType[2] = XmPUSHBUTTON;
#ifdef EXTENDED_INTERFACE
  buttonType[3] = XmPUSHBUTTON;
#endif
  n = 0;
  XtSetArg(args[n],XmNbuttonCount,N_PALETTES_MENU_ELES); n++;
  XtSetArg(args[n],XmNbuttons,buttons); n++;
  XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
  XtSetArg(args[n],XmNbuttonType,buttonType); n++;
  XtSetArg(args[n],XmNpostFromButton,PALETTES_BTN_POSN); n++;
  XtSetArg(args[n],XmNsimpleCallback,palettesMenuSimpleCallback); n++;
  mainPalettesPDM = XmCreateSimplePulldownMenu(mainMB,"mainPalettesPDM",args,n);
  for (i = 0; i < N_PALETTES_MENU_ELES; i++) XmStringFree(buttons[i]);

  if (globalDisplayListTraversalMode == DL_EXECUTE)
      XtSetSensitive(mainPalettesPDM,False);


/*
 * create the help pulldown menu pane
 */
  buttons[0] = XmStringCreateSimple("On Context");
  buttons[1] = XmStringCreateSimple("On Window...");
  buttons[2] = XmStringCreateSimple("On Keys...");
  buttons[3] = XmStringCreateSimple("Index...");
  buttons[4] = XmStringCreateSimple("On Help...");
  buttons[5] = XmStringCreateSimple("Tutorial...");
  buttons[6] = XmStringCreateSimple("On Version...");
  keySyms[0] = 'C';
  keySyms[1] = 'W';
  keySyms[2] = 'K';
  keySyms[3] = 'I';
  keySyms[4] = 'H';
  keySyms[5] = 'T';
  keySyms[6] = 'V';
  accelerators[0] = "Ctrl<Key>H"; 
  accelerators[1] = "";
  accelerators[2] = "";
  accelerators[3] = "";
  accelerators[4] = "";
  accelerators[5] = "";
  accelerators[6] = "";
  j = 0;
  acceleratorText[0] = XmStringCreateSimple("Ctrl+H");
  acceleratorText[1] = XmStringCreateSimple("");
  acceleratorText[2] = XmStringCreateSimple("");
  acceleratorText[3] = XmStringCreateSimple("");
  acceleratorText[4] = XmStringCreateSimple("");
  acceleratorText[5] = XmStringCreateSimple("");
  acceleratorText[6] = XmStringCreateSimple("");
  for (i = 0; i < 6; i++) buttonType[i] = XmPUSHBUTTON;
  buttonType[6] = XmSEPARATOR;
  buttonType[7] = XmPUSHBUTTON;

  for (i = 0; i < N_HELP_MENU_ELES; i++) buttonType[i] = XmPUSHBUTTON;
  n = 0;
  XtSetArg(args[n],XmNbuttonCount,N_HELP_MENU_ELES); n++;
  XtSetArg(args[n],XmNbuttons,buttons); n++;
  XtSetArg(args[n],XmNbuttonMnemonics,keySyms); n++;
  XtSetArg(args[n],XmNbuttonType,buttonType); n++;
  XtSetArg(args[n],XmNbuttonAccelerators,accelerators); n++;
  XtSetArg(args[n],XmNbuttonAcceleratorText,acceleratorText); n++;
  XtSetArg(args[n],XmNpostFromButton,HELP_BTN_POSN); n++;
  XtSetArg(args[n],XmNsimpleCallback,helpMenuSimpleCallback); n++;
  mainHelpPDM = XmCreateSimplePulldownMenu(mainMB,"mainHelpPDM",args,n);
  for (i = 0; i < N_HELP_MENU_ELES; i++) {
    XmStringFree(buttons[i]);
    XmStringFree(acceleratorText[i]);
  }

/* don't enable other help functions yet */
  XtSetSensitive(XtNameToWidget(mainHelpPDM,"*button_0"),False);
  XtSetSensitive(XtNameToWidget(mainHelpPDM,"*button_1"),False);
  XtSetSensitive(XtNameToWidget(mainHelpPDM,"*button_2"),False);
  XtSetSensitive(XtNameToWidget(mainHelpPDM,"*button_3"),False);
  XtSetSensitive(XtNameToWidget(mainHelpPDM,"*button_4"),False);
  XtSetSensitive(XtNameToWidget(mainHelpPDM,"*button_5"),False);


  n = 0;
  XtSetArg(args[n],XmNmarginHeight,9); n++;
  XtSetArg(args[n],XmNmarginWidth,18); n++;
  mainBB = XmCreateBulletinBoard(mainMW,"mainBB",args,n);
  XtAddCallback(mainBB,XmNhelpCallback,
		(XtCallbackProc)globalHelpCallback,(XtPointer)HELP_MAIN);


/*
 * create mode frame
 */
  n = 0;
  XtSetArg(args[n],XmNshadowType,XmSHADOW_ETCHED_IN); n++;
  frame = XmCreateFrame(mainBB,"frame",args,n);
  label = XmStringCreateSimple("Mode");
  n = 0;
  XtSetArg(args[n],XmNlabelString,label); n++;
  XtSetArg(args[n],XmNmarginWidth,0); n++;
  XtSetArg(args[n],XmNmarginHeight,0); n++;
  XtSetArg(args[n],XmNchildType,XmFRAME_TITLE_CHILD); n++;
  frameLabel = XmCreateLabel(frame,"frameLabel",args,n);
  XmStringFree(label);
  XtManageChild(frameLabel);
  XtManageChild(frame);

  if (globalDisplayListTraversalMode == DL_EDIT) {
/*
 * create the mode radio box and buttons
 */
    n = 0;
    XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
    XtSetArg(args[n],XmNpacking,XmPACK_COLUMN); n++;
    XtSetArg(args[n],XmNnumColumns,1); n++;
    XtSetArg(args[n],XmNchildType,XmFRAME_WORKAREA_CHILD); n++;
    modeRB = XmCreateRadioBox(frame,"modeRB",args,n);
    label = XmStringCreateSimple("Edit");

    n = 0;
    XtSetArg(args[n],XmNlabelString,label); n++;
    XtSetArg(args[n],XmNset,TRUE); n++;		/* start with EDIT as set */
    modeEditTB = XmCreateToggleButton(modeRB,"modeEditTB",args,n);
    XtAddCallback(modeEditTB,XmNvalueChangedCallback,
	(XtCallbackProc)modeCallback, (XtPointer)DL_EDIT);
    XmStringFree(label);
    label = XmStringCreateSimple("Execute");
    n = 0;
    XtSetArg(args[n],XmNlabelString,label); n++;
    modeExecTB = XmCreateToggleButton(modeRB,"modeExecTB",args,n);
    XtAddCallback(modeExecTB,XmNvalueChangedCallback,
		(XtCallbackProc)modeCallback,(XtPointer)DL_EXECUTE);
    XmStringFree(label);
    XtManageChild(modeRB);
    XtManageChild(modeEditTB);
    XtManageChild(modeExecTB);

  } else {

/* if started in execute mode, then no editing allowed, therefore
 * the modeRB widget is really a frame with a label indicating
 * execute-only mode
 */
    label = XmStringCreateSimple("Execute-Only");
    n = 0;
    XtSetArg(args[n],XmNlabelString,label); n++;
    XtSetArg(args[n],XmNmarginWidth,2); n++;
    XtSetArg(args[n],XmNmarginHeight,1); n++;
    XtSetArg(args[n],XmNchildType,XmFRAME_WORKAREA_CHILD); n++;
    modeRB = XmCreateLabel(frame,"modeRB",args,n);
    XmStringFree(label);
    XtManageChild(modeRB);

  }


/*
 * manage the composites
 */
  XtManageChild(mainBB);
  XtManageChild(mainMB);
  XtManageChild(mainMW);




/************************************************
 ****** create main-window related dialogs ******
 ************************************************/

 /*
  * note - all FILE pdm entry dialogs are sharing a single callback
  *	  fileMenuDialogCallback, with client data
  *	  of the BTN id in the simple menu...
  */


/*
 * create the Open... file selection dialog
 */
  n = 0;
  label = XmStringCreateSimple("*.adl");
  XtSetArg(args[n],XmNpattern,label); n++;
/* for some odd reason can't get PATH_MAX reliably defined between systems */
#define LOCAL_PATH_MAX  1023
  cwd = getcwd(NULL,LOCAL_PATH_MAX+1);
  cwdXmString = XmStringCreateSimple(cwd);
  XtSetArg(args[n],XmNdirectory,cwdXmString); n++;
  openFSD = XmCreateFileSelectionDialog(mainFilePDM,"openFSD",args,n);
  XtAddCallback(openFSD,XmNokCallback,(XtCallbackProc)dmDisplayListOk,
	(XtPointer)openFSD);
  XtAddCallback(openFSD,XmNcancelCallback,
	(XtCallbackProc)fileMenuDialogCallback,(XtPointer)FILE_OPEN_BTN);
  XmStringFree(label);
  XmStringFree(cwdXmString);
  free(cwd);

/*
 * create the Close... question dialog
 */

  n = 0;
  XtSetArg(args[n],XmNdefaultPosition,False); n++;
  closeQD = XmCreateQuestionDialog(mainFilePDM,"closeQD",args,n);
  XtSetArg(args[0],XmNmwmDecorations, MWM_DECOR_ALL|MWM_DECOR_RESIZEH);
  XtSetValues(XtParent(closeQD),args,1);
  XtAddCallback(closeQD,XmNcancelCallback,
	(XtCallbackProc)fileMenuDialogCallback,(XtPointer)FILE_CLOSE_BTN);
  XtAddCallback(closeQD,XmNokCallback,(XtCallbackProc)fileMenuDialogCallback,
	(XtPointer)FILE_CLOSE_BTN);
  XtAddCallback(closeQD,XmNmapCallback,
	(XtCallbackProc)mapCallback,(XtPointer)NULL);

/*
 * create the Save As... prompt dialog
 */
  n = 0;
  XtSetArg(args[n],XmNdefaultPosition,False); n++;
  saveAsPD = XmCreatePromptDialog(mainFilePDM,"saveAsPD",args,n);
  XtAddCallback(saveAsPD,XmNcancelCallback,
	(XtCallbackProc)fileMenuDialogCallback,(XtPointer)FILE_SAVE_AS_BTN);
  XtAddCallback(saveAsPD,XmNokCallback,(XtCallbackProc)fileMenuDialogCallback,
	(XtPointer)FILE_SAVE_AS_BTN);
  XtAddCallback(saveAsPD,XmNmapCallback,(XtCallbackProc)mapCallback,
	(XtPointer)NULL);

/*
 * create the Exit... warning dialog
 */
  exitQD = XmCreateQuestionDialog(mainFilePDM,"exitQD",NULL,0);
  XtSetArg(args[0],XmNmwmDecorations, MWM_DECOR_ALL|MWM_DECOR_RESIZEH);
  XtSetValues(XtParent(exitQD),args,1);
  XtAddCallback(exitQD,XmNcancelCallback,
	(XtCallbackProc)fileMenuDialogCallback,(XtPointer)FILE_EXIT_BTN);
  XtAddCallback(exitQD,XmNokCallback,(XtCallbackProc)fileMenuDialogCallback,
	(XtPointer)FILE_EXIT_BTN);

/*
 * create the Help information shell
 */
  n = 0;
  XtSetArg(args[n],XtNiconName,"Help"); n++;
  XtSetArg(args[n],XtNtitle,"Medm Help System"); n++;
  XtSetArg(args[n],XtNallowShellResize,TRUE); n++;
  XtSetArg(args[n],XmNkeyboardFocusPolicy,XmEXPLICIT); n++;
/* map window manager menu Close function to application close... */
  XtSetArg(args[n],XmNdeleteResponse,XmDO_NOTHING); n++;
  XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
  helpS = XtCreatePopupShell("helpS",topLevelShellWidgetClass,
		mainShell,args,n);
  XmAddWMProtocolCallback(helpS,WM_DELETE_WINDOW,
		(XtCallbackProc)wmCloseCallback,(XtPointer)OTHER_SHELL);

  n = 0;
  XtSetArg(args[n],XmNdialogType,XmDIALOG_INFORMATION); n++;
  helpMessageBox = XmCreateMessageBox(helpS,"helpMessageBox",
			args,n);
  XtAddCallback(helpMessageBox,XmNcancelCallback,
	(XtCallbackProc)helpDialogCallback, (XtPointer)NULL);
  XtAddCallback(helpMessageBox,XmNokCallback,
	(XtCallbackProc)helpDialogCallback,(XtPointer)NULL);

  XtManageChild(helpMessageBox);

/*
 * and realize the toplevel shell widget
 */
  XtRealizeWidget(mainShell);

}



