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
 *
 *****************************************************************************
*/

#ifndef __PROTO_H__
#define __PROTO_H__


/* actions.c */
void StartDrag(Widget w, XEvent *event);

/* callbacks.c */
void dmDisplayListOk(Widget, XtPointer, XtPointer);
void executePopupMenuCallback(Widget, XtPointer, XtPointer);
void dmCreateRelatedDisplay(Widget, XtPointer, XtPointer);
void dmExecuteShellCommand(Widget w,
	DlShellCommandEntry *commandEntry,
	XmPushButtonCallbackStruct *call_data);
void drawingAreaCallback(Widget w, DisplayInfo *displayInfo,
	XmDrawingAreaCallbackStruct *call_data);
void relatedDisplayMenuButtonDestroy(Widget, XtPointer, XtPointer);
void warnCallback(Widget, XtPointer, XtPointer);
void exitCallback(Widget, XtPointer, XtPointer);
void simpleRadioBoxCallback(Widget w, int buttonNumber,
	XmToggleButtonCallbackStruct *call_data);
void valuatorValueChanged(Widget, XtPointer, XtPointer);

/* channelPalette.c */
void createChannel(void);

/* colorPalette.c */
void createColor(void);
void setCurrentDisplayColorsInColorPalette(int rcType, int index);

/* createControllers.c */
DlElement *createDlChoiceButton(DisplayInfo *displayInfo);
DlElement *createDlMessageButton(DisplayInfo *displayInfo);
DlElement *createDlValuator(DisplayInfo *displayInfo);

DlElement *createDlTextEntry(DisplayInfo *displayInfo);
DlElement *createDlMenu(DisplayInfo *displayInfo);

/* createExtensions.c */
DlElement *createDlImage(DisplayInfo *displayInfo);
DlElement *createDlComposite(DisplayInfo *displayInfo);
void handleImageCreate(void);
DlElement *createDlPolyline(DisplayInfo *displayInfo);
DlElement *createDlPolygon(DisplayInfo *displayInfo);
DlElement *handlePolylineCreate(int x0, int y0, Boolean simpleLine);
DlElement *handlePolygonCreate(int x0, int y0);
void handlePolylineVertexManipulation(DlPolyline *dlPolyline, int pointIndex);
void handlePolygonVertexManipulation(DlPolygon *dlPolygon, int pointIndex);

/* createMonitors.c */
DlElement *createDlMeter(DisplayInfo *displayInfo);
DlElement *createDlBar(DisplayInfo *displayInfo);
DlElement *createDlByte(DisplayInfo *displayInfo);
DlElement *createDlIndicator(DisplayInfo *displayInfo);
DlElement *createDlTextUpdate(DisplayInfo *displayInfo);
DlElement *createDlStripChart(DisplayInfo *displayInfo);
DlElement *createDlCartesianPlot(DisplayInfo *displayInfo);
/*
DlElement *createDlSurfacePlot(DisplayInfo *displayInfo);
*/

/* createStatics.c */
DlElement *createDlFile(DisplayInfo *displayInfo);
DlElement *createDlDisplay(DisplayInfo *displayInfo);
DlElement *createDlColormap(DisplayInfo *displayInfo);
DlElement *createDlBasicAttribute(DisplayInfo *displayInfo);
DlElement *createDlDynamicAttribute(DisplayInfo *displayInfo);
DlElement *createDlRectangle(DisplayInfo *displayInfo);
DlElement *createDlOval(DisplayInfo *displayInfo);
DlElement *createDlArc(DisplayInfo *displayInfo);
DlElement *createDlText(DisplayInfo *displayInfo);
DlElement *createDlRelatedDisplay(DisplayInfo *displayInfo);
DlElement *createDlShellCommand(DisplayInfo *displayInfo);
void createDlObject(DisplayInfo *displayInfo, DlObject *object);
DlElement *handleTextCreate(int x0, int y0);

/* display.c */
DisplayInfo *createDisplay(void);

/* dmInit.c */
DisplayInfo *allocateDisplayInfo(void);
void dmDisplayListParse(FILE *, char *, char *, char*, Boolean);
void parseCompositeChildren(DisplayInfo *displayInfo, DlComposite *dlComposite);

/* eventHandlers.c */
void popupMenu(Widget, XtPointer, XEvent *, Boolean *);
void popdownMenu(Widget, XtPointer, XEvent *, Boolean *);
void handleEnterWindow(Widget, XtPointer, XEvent *, Boolean *);
void handleButtonPress(Widget, XtPointer, XEvent *, Boolean *);
int highlightSelectedElements(void);
void unhighlightSelectedElements(void);
void unselectSelectedElements(void);
int highlightAndSetSelectedElements(DlElement **array, int arraySize,
	int numElements);
int highlightAndAugmentSelectedElements(DlElement **array, int arraySize,
	int numElements);
Boolean unhighlightAndUnselectElement(DlElement *element, int *numSelected);
void moveCompositeChildren(DisplayInfo *cdi, DlElement *element,
	int xOffset, int yOffset, Boolean moveWidgets);
void resizeCompositeChildren(DisplayInfo *cdi, DlElement *outerComposite,
	DlElement *composite, float scaleX, float scaleY);
void updateDraggedElements(Position x0, Position y0, Position x1, Position y1);
void updateResizedElements(Position x0, Position y0, Position x1, Position y1);
void handleRectangularCreates(DlElementType elementType);

/* executeControllers.c */
int textFieldFontListIndex(int height);
int messageButtonFontListIndex(int height);
int menuFontListIndex(int height);
int valuatorFontListIndex(DlValuator *dlValuator);
void executeDlChoiceButton(DisplayInfo *displayInfo,
	DlChoiceButton *dlChoiceButton, Boolean dummy);
void executeDlMessageButton(DisplayInfo *displayInfo,
	DlMessageButton *dlMessageButton, Boolean dummy);
void createChoiceButtonButtonsInstance(Channel *);
void executeDlValuator(DisplayInfo *displayInfo, DlValuator *dlValuator,
	Boolean dummy);
void executeDlTextEntry(DisplayInfo *displayInfo, DlTextEntry *dlTextEntry,
	Boolean dummy);
void executeDlMenu(DisplayInfo *displayInfo, DlMenu *dlMenu, Boolean);
void createMenuMenuInstance(Channel *);

/* executeExtensions.c */
void executeDlImage(DisplayInfo *displayInfo, DlImage *dlImage, Boolean dummy);
void executeDlPolyline(DisplayInfo *displayInfo, DlPolyline *dlPolyline,
	Boolean dummy);
void executeDlPolygon(DisplayInfo *displayInfo, DlPolygon *dlPolygon,
	Boolean dummy);

/* executeMonitors.c */
Channel *allocateChannel(
	DisplayInfo *displayInfo);
void executeDlMeter(DisplayInfo *displayInfo, DlMeter *dlMeter, Boolean dummy);
void executeDlBar(DisplayInfo *displayInfo, DlBar *dlBar, Boolean dummy);
void executeDlByte(DisplayInfo *displayInfo, DlByte *dlByte, Boolean dummy);
void executeDlIndicator(DisplayInfo *displayInfo, DlIndicator *dlIndicator,
	Boolean dummy);
void executeDlTextUpdate(DisplayInfo *displayInfo, DlTextUpdate *dlTextUpdate,
	Boolean dummy);
void executeDlStripChart(DisplayInfo *displayInfo, DlStripChart *dlStripChart,
	Boolean dummy);
void executeDlCartesianPlot(DisplayInfo *displayInfo,
	DlCartesianPlot *dlCartesianPlot, Boolean dummy);
void executeDlSurfacePlot(DisplayInfo *displayInfo,
	DlSurfacePlot *dlSurfacePlot, Boolean dummy);

/* executeStatics.c */
void executeDlComposite(DisplayInfo *displayInfo, DlComposite *dlComposite,
		Boolean dummy);
void executeDlFile(DisplayInfo *displayInfo, DlFile *dlFile, Boolean dummy);
void executeDlDisplay(DisplayInfo *displayInfo, DlDisplay *dlDisplay,
	Boolean dummy);
void executeDlColormap(DisplayInfo *displayInfo, DlColormap *dlColormap,
	Boolean dummy);
void executeDlBasicAttribute(DisplayInfo *displayInfo,
	DlBasicAttribute *dlBasicAttribute, Boolean dummy);
void executeDlDynamicAttribute(DisplayInfo *displayInfo,
	DlDynamicAttribute *dlDynamicAttribute, Boolean dummy);
void executeDlRectangle(DisplayInfo *displayInfo, DlRectangle *dlRectangle,
	Boolean forcedDisplayToWindow);
void executeDlOval(DisplayInfo *displayInfo, DlOval *dlOval,
	Boolean forcedDisplayToWindow);
void executeDlArc(DisplayInfo *displayInfo, DlArc *dlArc,
	Boolean forcedDisplayToWindow);
void executeDlText(DisplayInfo *displayInfo, DlText *dlText,
	Boolean forcedDisplayToWindow);
void executeDlRelatedDisplay(DisplayInfo *displayInfo,
	DlRelatedDisplay *dlRelatedDisplay, Boolean dummy);
void executeDlShellCommand(DisplayInfo *displayInfo,
	DlShellCommand *dlShellCommand, Boolean dummy);

/* help.c */
void errMsgDlgCreateDlg();
void globalHelpCallback(Widget, XtPointer, XtPointer);
void medmPostMsg(char *);
void medmPostTime();
void medmPrintf(char*,...);
void medmCreateCAStudyDlg();
void medmStartUpdateCAStudyDlg();

/* medm.c */
int main(int argc, char *argv[]);
Widget createDisplayMenu(Widget widget);
Widget buildMenu(Widget,int,char*,char,menuEntry_t*);
void medmExit();
Boolean medmSaveDisplay(DisplayInfo *, char *, Boolean);
void enableEditFunctions();
void disableEditFunctions();

/* medmCA.c */
int medmCAInitialize(void);
void medmCATerminate(void);
void updateListCreate(Channel *);
void updateListDestroy(Channel *);
void medmConnectEventCb(struct connection_handler_args);
void medmDisconnectChannel(Channel *pCh);
Record *medmAllocateRecord(char*,void(*)(XtPointer),void(*)(XtPointer),XtPointer);
void medmDestoryRecord(Record *);
void medmSendDouble(Record *, double);
void medmSendString(Record *, char *);
void CATaskGetInfo(int *, int *, int *);

/* medmPixmap.c */
void medmInitializeImageCache(void);
void medmClearImageCache(void);

/* medmValuator.c */
void popupValuatorKeyboardEntry(Widget, DisplayInfo *, XEvent *);

/* objectPalette.c */
void createObject(void);
void clearResourcePaletteEntries(void);
void objectMenuCallback(Widget,XtPointer,XtPointer);
void objectPaletteSetSensitivity(Boolean);
void setActionToSelect();
void setResourcePaletteEntries(void);
void updateGlobalResourceBundleFromElement(DlElement *element);
void updateGlobalResourceBundleAndResourcePalette(Boolean objectDataOnly);
void updateElementFromGlobalResourceBundle(DlElement *elementPtr);
void resetGlobalResourceBundleAndResourcePalette(void);

/* parseControllers.c */
void parseChoiceButton(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseMessageButton(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseValuator(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseTextEntry(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseMenu(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseControl(DisplayInfo *displayInfo, DlControl *control);

/* parseExtensions.c */
void parseImage(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseComposite(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parsePolyline(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parsePolygon(DisplayInfo *displayInfo, DlComposite *dlComposite);

/* parseMonitors.c */
void parseMeter(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseBar(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseByte(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseIndicator(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseTextUpdate(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseStripChart(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseCartesianPlot(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseSurfacePlot(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseMonitor(DisplayInfo *displayInfo, DlMonitor *monitor);
void parsePlotcom(DisplayInfo *displayInfo, DlPlotcom *plotcom);
void parsePen(DisplayInfo *displayInfo, DlPen *pen);
void parseTrace(DisplayInfo *displayInfo, DlTrace *trace);
void parsePlotAxisDefinition(DisplayInfo *displayInfo,
	DlPlotAxisDefinition *axisDefinition);

/* parseStatics.c */
void parseFile(DisplayInfo *displayInfo);
void parseDisplay(DisplayInfo *displayInfo);
DlColormap *parseColormap(DisplayInfo *displayInfo, FILE *filePtr);
void parseBasicAttribute(DisplayInfo *displayInfo, DlComposite *dlComposite);
DlDynamicAttribute *parseDynamicAttribute(DisplayInfo *displayInfo,
	DlComposite *dlComposite);
void parseRectangle(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseOval(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseArc(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseText(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseRelatedDisplay(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseShellCommand(DisplayInfo *displayInfo, DlComposite *dlComposite);
void parseDlColor(DisplayInfo *displayInfo, FILE *filePtr,
	DlColormapEntry *dlColor);
void parseObject(DisplayInfo *displayInfo, DlObject *object);
void parseAttr(DisplayInfo *displayInfo, DlAttribute *attr);
void parseDynamicAttr(DisplayInfo *displayInfo,
	DlDynamicAttributeData *dynAttr);
void parseDynAttrMod(DisplayInfo *displayInfo, DlDynamicAttrMod *dynAttr);
void parseDynAttrParam(DisplayInfo *displayInfo, DlDynamicAttrParam *dynAttr);
void parseRelatedDisplayEntry(DisplayInfo *displayInfo,
	DlRelatedDisplayEntry *relatedDisplay);
void parseShellCommandEntry(DisplayInfo *displayInfo,
	DlShellCommandEntry *shellCommand);
DlColormap *parseAndExtractExternalColormap(DisplayInfo *displayInfo,
	char *filename);
TOKEN getToken(DisplayInfo *displayInfo, char *word);

/* resourcePalette.c */
void initializeGlobalResourceBundle(void);
void createResource(void);
void textFieldNumericVerifyCallback(Widget, XtPointer, XtPointer);
void textFieldFloatVerifyCallback(Widget, XtPointer, XtPointer);
void textFieldActivateCallback(Widget w, XtPointer, XtPointer);
void textFieldLosingFocusCallback(Widget w, XtPointer, XtPointer);
Widget createRelatedDisplayDataDialog(Widget parent);
void updateRelatedDisplayDataDialog(void);
Widget createShellCommandDataDialog(Widget parent);
void updateShellCommandDataDialog(void);
void cpEnterCellCallback(Widget w, XtPointer, XtPointer);
void cpUpdateMatrixColors(void);
Widget createCartesianPlotDataDialog(Widget parent);
void updateCartesianPlotDataDialog(void);
Widget createCartesianPlotAxisDialog(Widget parent);
void updateCartesianPlotAxisDialog(void);
void updateCartesianPlotAxisDialogFromWidget(Widget cp);
void scEnterCellCallback(Widget w, XtPointer, XtPointer);
void scUpdateMatrixColors(void);
Widget createStripChartDataDialog(Widget parent);
void updateStripChartDataDialog(void);

/* shared.c */
void wmCloseCallback(Widget, XtPointer, XtPointer);
XtCallbackProc wmTakeFocusCallback(Widget w, ShellType shellType,
	XmAnyCallbackStruct *call_data);
void updateStatusFields(void);
void optionMenuSet(Widget menu, int buttonId);
double medmTime();
void updateTaskInit(DisplayInfo *displayInfo);
UpdateTask *updateTaskAddTask(DisplayInfo *, DlObject *, void (*)(XtPointer), XtPointer);
void updateTaskDeleteTask(UpdateTask *);
void updateTaskDeleteAllTask(UpdateTask *);
int updateTaskMarkTimeout(UpdateTask *, double);
void updateTaskSetScanRate(UpdateTask *, double);
void updateTaskAddExecuteCb(UpdateTask *, void (*)(XtPointer));
void updateTaskAddDestroyCb(UpdateTask *, void (*)(XtPointer));
void updateTaskMarkUpdate(UpdateTask *pt);
void updateTaskRepaintRegion(DisplayInfo *, Region *);
Boolean medmInitSharedDotC();
void updateTaskStatusGetInfo(int *taskCount,
                             int *periodicTaskCount,
                             int *updateRequestCount,
                             int *updateDiscardCount,
                             int *periodicUpdateRequestCount,
                             int *periodicUpdateDiscardCount,
                             int *updateRequestQueued,
                             int *updateExecuted,
                             double *timeInterval); 
void updateTaskAddNameCb(UpdateTask *, void (*)(XtPointer, char **, short *, int *));



/* updateMonitors.c */
void localCvtDoubleToString( double, char *, unsigned short);
void localCvtDoubleToExpNotationString(double, char *, unsigned short);

void traverseMonitorList(Boolean forcedTraversal, DisplayInfo *displayInfo,
	int regionX, int regionY, unsigned int regionWidth,
	unsigned int regionHeight);
void updateTextUpdate(UpdateTask *);
void draw3DQuestionMark(UpdateTask *);
void draw3DPane(UpdateTask *, Pixel);
void drawReadOnlySymbol(UpdateTask *);
void drawWhiteRectangle(UpdateTask *);

/* utils.c */
FILE *dmOpenUseableFile(char *filename);
Boolean extractStringBetweenColons(char *input, char *output, int startPos,
	int  *endPos);
void dmRemoveMonitorStructureFromMonitorList(
	Channel *monitorData);
void dmRemoveDisplayList(DisplayInfo *displayInfo);
void dmCleanupDisplayInfo(DisplayInfo *displayInfo, Boolean cleanupDisplayList);
void dmRemoveDisplayInfo(DisplayInfo *displayInfo);
void dmRemoveAllDisplayInfo(void);
void dmTraverseDisplayList(DisplayInfo *displayInfo);
void dmTraverseAllDisplayLists(void);
void dmTraverseNonWidgetsInDisplayList(DisplayInfo *displayInfo);
int dmGetBestFontWithInfo(XFontStruct **fontTable, int nFonts, char *text,
	int h, int w, int *usedH, int *usedW, Boolean textWidthFlag);
void dmSetAndPopupWarningDialog(DisplayInfo *displayInfo,
                                 char        *message,
                                 char        *okBtnLabel,
                                 char        *cancelBtnLabel,
                                 char        *helpBtnLabel);
void dmSetAndPopupQuestionDialog(DisplayInfo *displayInfo,
                                 char        *message,
                                 char        *okBtnLabel,
                                 char        *cancelBtnLabel,
                                 char        *helpBtnLabel);
XtErrorHandler trapExtraneousWarningsHandler(String message);
DisplayInfo *dmGetDisplayInfoFromWidget(Widget widget);
void dmWriteDisplayList(DisplayInfo *displayInfo, FILE *stream);
void dmSetDisplayFileName(DisplayInfo *displayInfo, char *filename);
char *dmGetDisplayFileName(DisplayInfo *displayInfo);
DlElement *lookupElement(DlElement *tail, Position x0, Position y0);
DlElement *lookupCompositeChild(DlElement *composite, Position x0, Position y0);
DlElement **selectedElementsLookup(DlElement *tail, Position x0, Position y0,
	Position x1, Position y1, int *arraySize, int *numSelected);
int numberOfElementsInComposite(DisplayInfo *displayInfo, DlElement *ele);
DlElement *lookupCompositeElement(DlElement *elementPtr);
DlElement *lookupDynamicAttributeElement(DlElement *elementPtr);
DlElement *lookupBasicAttributeElement(DlElement *elementPtr);
DlElement *lookupPrivateBasicAttributeElement(DlElement *elementPtr);
Boolean dmResizeDisplayList(DisplayInfo *displayInfo, Dimension newWidth,
	Dimension newHeight);
Boolean dmResizeSelectedElements(DisplayInfo *displayInfo, Dimension newWidth,
	Dimension newHeight);
void initializeRubberbanding(void);
void doRubberbanding(Window window, Position *initialX, Position *initialY,
	Position *finalX, Position *finalY);
Boolean doDragging(Window window, Dimension daWidth, Dimension daHeight,
	Position initialX,Position initialY,Position *finalX,Position *finalY);
DisplayInfo *doPasting(Position *displayX, Position *displayY, int *offsetX,
	int *offsetY);
Boolean alreadySelected(DlElement *element);
Boolean doResizing(Window window, Position initialX, Position initialY, 
	Position *finalX, Position *finalY);
Widget lookupElementWidget(DisplayInfo *displayInfo, DlObject *object);
void destroyElementWidget(DisplayInfo *displayInfo, Widget widget);
void clearClipboard(void);
void copyElementsIntoClipboard(void);
DlStructurePtr createCopyOfElementType(DlElementType type, DlStructurePtr ptr);
void copyElementsIntoDisplay(void);
void deleteElementsInDisplay(void);
void unselectElementsInDisplay(void);
void selectAllElementsInDisplay(void);
void lowerSelectedElements(void);
void ungroupSelectedElements(void);
void raiseSelectedElements(void);
void alignSelectedElements(int alignment);
void moveElementAfter(DisplayInfo *cdi, DlComposite *dlComposite,
	DlElement *src, DlElement *dst);
void moveSelectedElementsAfterElement(DisplayInfo *displayInfo,
	DlElement *afterThisElement);
void deleteAndFreeElementAndStructure(DisplayInfo *displayInfo, DlElement *ele);
UpdateTask *getUpdateTaskFromWidget(Widget sourceWidget);
UpdateTask *getUpdateTaskFromPosition(DisplayInfo *displayInfo, int x, int y);
NameValueTable *generateNameValueTable(char *argsString, int *numNameValues);
char *lookupNameValue(NameValueTable *nameValueTable, int numEntries,
	char *name);
void freeNameValueTable(NameValueTable *nameValueTable, int numEntries);
void performMacroSubstitutions(DisplayInfo *displayInfo,
        char *inputString, char *outputString, int sizeOfOutputString);
void colorMenuBar(Widget widget, Pixel fg, Pixel bg);
void medmSetDisplayTitle(DisplayInfo *displayInfo);
void medmMarkDisplayBeingEdited(DisplayInfo *displayInfo);
void closeDisplay(Widget);
#ifdef __COLOR_RULE_H__
Pixel extractColor(DisplayInfo *displayInfo, double value, int colorRule, int defaultColor);
#endif


/* medmWidget.c */
void medmInit(char *displayFontName);
void dmTerminateX(void);
unsigned long getPixelFromColormapByString(Display *display, int screen,
			Colormap cmap, char *colorString);


/* writeControllers.c */
void writeDlChoiceButton(FILE *stream, DlChoiceButton *dlChoiceButton,
	int level);
void writeDlMessageButton(FILE *stream, DlMessageButton *dlMessageButton,
	int level);
void writeDlValuator(FILE *stream, DlValuator *dlValuator, int level);
void writeDlTextEntry(FILE *stream, DlTextEntry *dlTextEntry, int level);
void writeDlMenu(FILE *stream, DlMenu *dlMenu, int level);
void writeDlControl(FILE *stream, DlControl *dlControl, int level);

/* writeExtensions.c */
void writeDlImage(FILE *stream, DlImage *dlImage, int level);
void writeDlCompositeChildren(FILE *stream, DlComposite *dlComposite,int level);
void writeDlComposite(FILE *stream, DlComposite *dlComposite, int level);
void writeDlPolyline(FILE *stream, DlPolyline *dlPolyline, int level);
void writeDlPolygon(FILE *stream, DlPolygon *dlPolygon, int level);

/* writeMonitors.c */
void writeDlMeter(FILE *stream, DlMeter *dlMeter, int level);
void writeDlBar(FILE *stream, DlBar *dlBar, int level);
void writeDlByte(FILE *stream, DlByte *dlByte, int level);
void writeDlIndicator(FILE *stream, DlIndicator *dlIndicator, int level);
void writeDlTextUpdate(FILE *stream, DlTextUpdate *dlTextUpdate, int level);
void writeDlStripChart(FILE *stream, DlStripChart *dlStripChart, int level);
void writeDlCartesianPlot(FILE *stream, DlCartesianPlot *dlCartesianPlot,
	int level);
void writeDlSurfacePlot(FILE *stream, DlSurfacePlot *dlSurfacePlot, int level);
void writeDlMonitor(FILE *stream, DlMonitor *dlMonitor, int level);
void writeDlPlotcom(FILE *stream, DlPlotcom *dlPlotcom, int level);
void writeDlPen(FILE *stream, DlPen *dlPen, int index, int level);
void writeDlTrace(FILE *stream, DlTrace *dlTrace, int index, int level);
void writeDlPlotAxisDefinition(FILE *stream,
	DlPlotAxisDefinition *dlAxisDefinition, int axisNumber, int level);

/* writeStatics.c */
void writeDlComposite(FILE *stream, DlComposite *dlComposite, int level);
void writeDlFile(FILE *stream, DlFile *dlFile, int level);
void writeDlDisplay(FILE *stream, DlDisplay *dlDisplay, int level);
void writeDlColormap(FILE *stream, DlColormap *dlColormap, int level);
void writeDlBasicAttribute(FILE *stream, DlBasicAttribute *dlBasicAttribute,
	int level);
void writeDlDynamicAttribute(FILE *stream,
	DlDynamicAttribute *dlDynamicAttribute, int level);
void writeDlRectangle(FILE *stream, DlRectangle *dlRectangle, int level);
void writeDlOval(FILE *stream, DlOval *dlOval, int level);
void writeDlArc(FILE *stream, DlArc *dlArc, int level);
void writeDlText(FILE *stream, DlText *dlText, int level);
void writeDlRelatedDisplay(FILE *stream, DlRelatedDisplay *dlRelatedDisplay,
	int level);
void writeDlShellCommand(FILE *stream, DlShellCommand *dlShellCommand,
	int level);
void writeDlColormapEntry(FILE *stream, DlColormapEntry *dlColormapEntry,
	int level);
void writeDlObject(FILE *stream, DlObject *dlObject, int level);
void writeDlAttr(FILE *stream, DlAttribute *dlAttr, int level);
void writeDlDynAttr(FILE *stream, DlDynamicAttributeData *dynAttr, int level);
void writeDlDynAttrMod(FILE *stream, DlDynamicAttrMod *dynAttr, int level);
void writeDlDynAttrParam(FILE *stream, DlDynamicAttrParam *dynAttr, int level);
void writeDlRelatedDisplayEntry(FILE *stream, DlRelatedDisplayEntry *entry,
	int index, int level);
void writeDlShellCommandEntry(FILE *stream, DlShellCommandEntry *entry,
	int index, int level);

/* xgif.c */
Boolean initializeGIF(DisplayInfo *displayInfo, DlImage *dlImage);
void drawGIF(DisplayInfo *displayInfo, DlImage *dlImage);
void resizeGIF(DisplayInfo *displayInfo, DlImage *dlImage);
Boolean loadGIF(DisplayInfo *displayInfo, DlImage *dlImage);
int ReadCode(void);
void freeGIF(DisplayInfo *displayInfo, DlImage *dlImage);

#endif  /* __PROTO_H__ */
