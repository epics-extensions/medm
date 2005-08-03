/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*****************************************************************************
 *
 *     Original Author : Mark Anderson
 *     Second Author   : Frederick Vong
 *     Third Author    : Kenneth Evans, Jr.
 *
 *****************************************************************************
*/

#ifndef __PROTO_H__
#define __PROTO_H__

/* This file uses timeval, but it will be defined before this file is
   included. */

/* Create methods  */
DisplayInfo *createDisplay(void);
DlColormap *createDlColormap(DisplayInfo *displayInfo);
DlElement *createDlArc(DlElement *);
DlElement *createDlBar(DlElement *);
DlElement *createDlByte(DlElement *);
DlElement *createDlCartesianPlot(DlElement *);
DlElement *createDlChoiceButton(DlElement *);
DlElement *createDlComposite(DlElement *);
DlElement *createDlDisplay(DlElement *);
DlElement *createDlImage(DlElement *);
DlElement *createDlIndicator(DlElement *);
DlElement *createDlMenu(DlElement *);
DlElement *createDlMessageButton(DlElement *);
DlElement *createDlMeter(DlElement *);
DlElement *createDlOval(DlElement *);
DlElement *createDlPolygon(DlElement *);
DlElement *createDlPolyline(DlElement *);
DlElement *createDlRectangle(DlElement *);
DlElement *createDlRelatedDisplay(DlElement *);
DlElement *createDlShellCommand(DlElement *);
DlElement *createDlStripChart(DlElement *);
DlElement *createDlText(DlElement *);
DlElement *createDlTextEntry(DlElement *);
DlElement *createDlTextUpdate(DlElement *);
DlElement *createDlValuator(DlElement *);
DlElement *createDlWheelSwitch(DlElement *);
DlElement *handleImageCreate();
DlElement *handlePolygonCreate(int x0, int y0);
DlElement *handlePolylineCreate(int x0, int y0, Boolean simpleLine);
DlElement *handleTextCreate(int x0, int y0);
DlElement* createDlElement(DlElementType, XtPointer, DlDispatchTable *);
DlFile *createDlFile(DisplayInfo *displayInfo);
void createDlObject(DisplayInfo *displayInfo, DlObject *object);

/* Execute methods */
void executeDlArc(DisplayInfo *, DlElement *);
void executeDlBar(DisplayInfo *, DlElement *);
void executeDlBasicAttribute(DisplayInfo *, DlBasicAttribute *);
void executeDlByte(DisplayInfo *, DlElement *);
void executeDlCartesianPlot(DisplayInfo *, DlElement *);
void executeDlChoiceButton(DisplayInfo *, DlElement *);
void executeDlColormap(DisplayInfo *, DlColormap *);
void executeDlComposite(DisplayInfo *, DlElement *);
void executeDlDisplay(DisplayInfo *, DlElement *);
void executeDlDynamicAttribute(DisplayInfo *, DlElement *);
void executeDlImage(DisplayInfo *, DlElement *);
void executeDlIndicator(DisplayInfo *, DlElement *);
void executeDlMenu(DisplayInfo *, DlElement *);
void executeDlMessageButton(DisplayInfo *, DlElement *);
void executeDlMeter(DisplayInfo *, DlElement *);
void executeDlOval(DisplayInfo *, DlElement *);
void executeDlPolygon(DisplayInfo *, DlElement *);
void executeDlPolyline(DisplayInfo *, DlElement *);
void executeDlRectangle(DisplayInfo *, DlElement *);
void executeDlRelatedDisplay(DisplayInfo *, DlElement *);
void executeDlShellCommand(DisplayInfo *, DlElement *);
void executeDlStripChart(DisplayInfo *, DlElement *);
void executeDlSurfacePlot(DisplayInfo *, DlElement *);
void executeDlText(DisplayInfo *, DlElement *);
void executeDlTextEntry(DisplayInfo *, DlElement *);
void executeDlTextUpdate(DisplayInfo *, DlElement *);
void executeDlValuator(DisplayInfo *, DlElement *);
void executeDlWheelSwitch(DisplayInfo *, DlElement *);

/* Hide methods */
void hideDlArc(DisplayInfo *, DlElement *);
void hideDlBar(DisplayInfo *, DlElement *);
void hideDlBasicAttribute(DisplayInfo *, DlBasicAttribute *);
void hideDlByte(DisplayInfo *, DlElement *);
void hideDlCartesianPlot(DisplayInfo *, DlElement *);
void hideDlChoiceButton(DisplayInfo *, DlElement *);
void hideDlColormap(DisplayInfo *, DlColormap *);
void hideDlComposite(DisplayInfo *, DlElement *);
void hideDlDisplay(DisplayInfo *, DlElement *);
void hideDlDynamicAttribute(DisplayInfo *, DlElement *);
void hideDlImage(DisplayInfo *, DlElement *);
void hideDlIndicator(DisplayInfo *, DlElement *);
void hideDlMenu(DisplayInfo *, DlElement *);
void hideDlMessageButton(DisplayInfo *, DlElement *);
void hideDlMeter(DisplayInfo *, DlElement *);
void hideDlOval(DisplayInfo *, DlElement *);
void hideDlPolygon(DisplayInfo *, DlElement *);
void hideDlPolyline(DisplayInfo *, DlElement *);
void hideDlRectangle(DisplayInfo *, DlElement *);
void hideDlRelatedDisplay(DisplayInfo *, DlElement *);
void hideDlShellCommand(DisplayInfo *, DlElement *);
void hideDlStripChart(DisplayInfo *, DlElement *);
void hideDlSurfacePlot(DisplayInfo *, DlElement *);
void hideDlText(DisplayInfo *, DlElement *);
void hideDlTextEntry(DisplayInfo *, DlElement *);
void hideDlTextUpdate(DisplayInfo *, DlElement *);
void hideDlValuator(DisplayInfo *, DlElement *);
void hideDlWheelSwitch(DisplayInfo *, DlElement *);

/* Write methods */
void writeDlArc(FILE *, DlElement *, int);
void writeDlBar(FILE *, DlElement *, int);
void writeDlBasicAttribute(FILE *, DlBasicAttribute *, int);
void writeDlByte(FILE *, DlElement *, int);
void writeDlCartesianPlot(FILE *, DlElement *, int);
void writeDlChoiceButton(FILE *, DlElement *, int);
void writeDlColormap(FILE *, DlColormap *, int);
void writeDlColormapEntry(FILE *, DlElement *, int);
void writeDlComposite(FILE *, DlElement *, int);
void writeDlComposite(FILE *, DlElement *, int);
void writeDlCompositeChildren(FILE *, DlElement *, int);
void writeDlControl(FILE *, DlControl *, int);
void writeDlDisplay(FILE *, DlElement *, int);
void writeDlDynamicAttribute(FILE *, DlDynamicAttribute *, int);
void writeDlFile(FILE *, DlFile *, int);
void writeDlImage(FILE *, DlElement *, int);
void writeDlIndicator(FILE *, DlElement *, int);
void writeDlMenu(FILE *, DlElement *, int);
void writeDlMessageButton(FILE *, DlElement *, int);
void writeDlMeter(FILE *, DlElement *, int);
void writeDlMonitor(FILE *, DlMonitor *, int);
void writeDlObject(FILE *, DlObject *, int);
void writeDlOval(FILE *, DlElement *, int);
void writeDlPen(FILE *, DlPen *, int, int);
void writeDlPlotAxisDefinition(FILE *, DlPlotAxisDefinition *, int, int);
void writeDlPlotcom(FILE *, DlPlotcom *, int);
void writeDlPolygon(FILE *, DlElement *, int);
void writeDlPolyline(FILE *, DlElement *, int);
void writeDlRectangle(FILE *, DlElement *, int);
void writeDlRelatedDisplay(FILE *, DlElement *, int);
void writeDlRelatedDisplayEntry(FILE *, DlRelatedDisplayEntry *, int, int);
void writeDlShellCommand(FILE *, DlElement *, int);
void writeDlShellCommandEntry(FILE *, DlShellCommandEntry *, int, int);
void writeDlStripChart(FILE *, DlElement *, int);
void writeDlSurfacePlot(FILE *, DlElement *, int);
void writeDlText(FILE *, DlElement *, int);
void writeDlTextEntry(FILE *, DlElement *, int);
void writeDlTextUpdate(FILE *, DlElement *, int);
void writeDlTrace(FILE *, DlTrace *, int, int);
void writeDlValuator(FILE *, DlElement *, int);
void writeDlWheelSwitch(FILE *, DlElement *, int);
void writeDlLimits(FILE *stream, DlLimits *dlLimits, int level);

/* actions.c */
void StartDrag(Widget w, XEvent *event);

/* bubbleHelp.c */
void handleBubbleHelp(Widget w, XtPointer clientData, XEvent *event,
  Boolean *ctd);

/* browserHelp.c */
int callBrowser(char *url, char *bookmark);

/* callbacks.c */
void wmCloseCallback(Widget, XtPointer, XtPointer);
void dmDisplayListOk(Widget, XtPointer, XtPointer);
void executePopupMenuCallback(Widget, XtPointer, XtPointer);
void executeMenuCallback(Widget  w, XtPointer cd, XtPointer cbs);
void dmCreateRelatedDisplay(Widget, XtPointer, XtPointer);
void dmExecuteShellCommand(Widget w,
  DlShellCommandEntry *commandEntry,
  XmPushButtonCallbackStruct *call_data);
void drawingAreaCallback(Widget w, XtPointer clientData, XtPointer callData);
void relatedDisplayMenuButtonDestroy(Widget, XtPointer, XtPointer);
void warnCallback(Widget, XtPointer, XtPointer);
void exitCallback(Widget, XtPointer, XtPointer);
void simpleRadioBoxCallback(Widget w, int buttonNumber,
  XmToggleButtonCallbackStruct *call_data);
void valuatorValueChanged(Widget, XtPointer, XtPointer);
void wheelSwitchValueChanged(Widget, XtPointer, XtPointer);

/* channelPalette.c */
void createChannel(void);

/* colorPalette.c */
void createColor(void);
void setCurrentDisplayColorsInColorPalette(int rcType, int index);

/* control.c */
void controlAttributeInit(DlControl *control);

/* dialogs.c */
void popupPrintSetup(void);
void popupPvInfo(DisplayInfo *displayInfo);
void popupPvLimits(DisplayInfo *displayInfo);
void updatePvLimits(DlLimits *limits);
void createPvInfoDlg(void);
Record **getPvInfoFromDisplay(DisplayInfo *displayInfo, int *count,
  DlElement **pE);
void popupDisplayListDlg(void);
void refreshDisplayListDlg(void);

/* eventHandlers.c */
int initEventHandlers(void);
void popdownMenu(Widget, XtPointer, XEvent *, Boolean *);
void handleExecuteButtonPress(Widget, XtPointer, XEvent *, Boolean *);
void handleEditButtonPress(Widget, XtPointer, XEvent *, Boolean *);
void handleEditEnterWindow(Widget, XtPointer, XEvent *, Boolean *);
void handleEditKeyPress(Widget, XtPointer, XEvent *, Boolean *);
void highlightSelectedElements(void);
void unhighlightSelectedElements(void);
void highlightAndAppendSelectedElements(DlList *);
Boolean unhighlightAndUnselectElement(DlElement *element, int *numSelected);
void moveCompositeChildren(DisplayInfo *cdi, DlElement *element,
  int xOffset, int yOffset, Boolean moveWidgets);
DlElement *handleRectangularCreates(DlElementType, int, int, unsigned int, unsigned
  int);
void addCommonHandlers(Widget w, DisplayInfo *displayInfo);

Widget createPushButton(Widget parent,
  DlObject *po,
  Pixel fg,
  Pixel bg,
  Pixmap pixmap,
  char *label,
  XtPointer userData);
int textFieldFontListIndex(int height);
int messageButtonFontListIndex(int height);
int menuFontListIndex(int height);
int valuatorFontListIndex(DlValuator *dlValuator);

/* help.c */
void errMsgDlgCreateDlg(int raise);
void globalHelpCallback(Widget, XtPointer, XtPointer);
void medmPostMsg(int priority, char *format, ...);
void medmPrintf(int priority, char *format, ...);
int checkEarlyMessages(void);
void medmCreateCAStudyDlg();
void medmStartUpdateCAStudyDlg();
void medmStopUpdateCAStudyDlg();
void medmResetUpdateCAStudyDlg(Widget w, XtPointer clientData,
  XtPointer callData);
int xDoNothingErrorHandler(Display *dpy, XErrorEvent *event);
int xErrorHandler(Display *dpy, XErrorEvent *event);
void xtErrorHandler(char *message);
int xInfoMsg(Widget parent, const char *fmt, ...);
void addDisplayHelpProtocol(DisplayInfo *displayInfo);

/* medm.c */
int main(int argc, char *argv[]);
Widget buildMenu(Widget,int,char*,char,menuEntry_t*);
void createEditModeMenu(DisplayInfo *displayInfo);
void disableEditFunctions();
void enableEditFunctions();
void mainFileMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs);
void medmExit();
Boolean medmSaveDisplay(DisplayInfo *, char *, Boolean);

/* medmCA.c and medmCdev.cc */
#ifdef MEDM_CDEV
# define medmCAInitialize medmCDEVInitialize
# define medmCATerminate medmCDEVTerminate
int  medmCDEVInitialize(void);
void medmCDEVTerminate(void);
#else
int medmCAInitialize(void);
void medmCATerminate(void);
void caTaskGetInfo(int *channelCount, int *channelConnected, int *caEventCount);
#endif
Record *medmAllocateRecord(char *name, void (*updateValueCb)(XtPointer),
  void (*updateGraphicalInfoCb)(XtPointer), XtPointer clientData);
Record **medmAllocateDynamicRecords(DlDynamicAttribute *attr,
  void (*updateValueCb)(XtPointer), void (*updateGraphicalInfoCb)(XtPointer),
  XtPointer clientData);
void medmDestoryRecord(Record *pr);
void medmSendDouble(Record *pr, double data);
void medmSendLong(Record *pr, long data);
void medmSendString(Record *pr, char *data);
void medmSendCharacterArray(Record *pr, char *data, unsigned long size);
void retryConnections(void);

/* medmCartesianPlot.c */
void cpEnterCellCallback(Widget w, XtPointer, XtPointer);
void cpUpdateMatrixColors(void);
Widget createCartesianPlotAxisDialog(Widget parent);
Widget createCartesianPlotDataDialog(Widget parent);
Widget createRelatedDisplayDataDialog(Widget parent);
void dumpCartesianPlot(Widget w);
void updateCartesianPlotAxisDialog(void);
void updateCartesianPlotAxisDialogFromWidget(Widget cp);
void updateCartesianPlotDataDialog(void);

/* medmComposite.c */
DlElement *groupObjects();
void ungroupSelectedElements(void);
CompositeUpdateState getCompositeUpdateState(DlElement *dlElement);
void setCompositeUpdateState(DlElement *dlElement, CompositeUpdateState state);

/* medmDisplay.c */
DlElement *parseDisplay(DisplayInfo *displayInfo);
void closeDisplay(Widget);
void refreshDisplay(DisplayInfo *displayInfo);
int repositionDisplay(DisplayInfo *displayInfo);

/* medmMonitor.c */
void monitorAttributeInit(DlMonitor *monitor);
void penAttributeInit(DlPen *pen);
void plotAxisDefinitionInit(DlPlotAxisDefinition *axisDefinition);
void plotcomAttributeInit(DlPlotcom *plotcom);
void traceAttributeInit(DlTrace *trace);

/* medmPixmap.c */
void medmInitializeImageCache(void);
void medmClearImageCache(void);

/* medmRelatedDisplay.c */
void relatedDisplayDataDialogPopup(Widget w);
void relatedDisplayCreateNewDisplay(DisplayInfo *displayInfo,
  DlRelatedDisplayEntry *pEntry, Boolean replaceDisplay);
void markHiddenButtons(DisplayInfo *displayInfo);

/* medmShellCommand.c */
Widget createShellCommandPromptD(Widget parent);
Widget createShellCommandDataDialog(Widget parent);
void updateShellCommandDataDialog(void);

/* medmValuator.c */
void popupValuatorKeyboardEntry(Widget, DisplayInfo *, XEvent *);

/* medmWheelSwitch.c */

/* medmWidget.c */
void medmInit(char *displayFontName);
void dmTerminateX(void);
unsigned long getPixelFromColormapByString(Display *display, int screen,
  Colormap cmap, char *colorString);
int initMedmWidget();
int destroyMedmWidget();
void hsort(double array[], int indx[], int n);
void moveDisplayInfoToDisplayInfoSave(DisplayInfo *displayInfo);
void moveDisplayInfoSaveToDisplayInfo(DisplayInfo *displayInfo);

/* objectPalette.c */
void createObject(void);
void objectMenuCallback(Widget,XtPointer,XtPointer);
void objectPaletteSetSensitivity(Boolean);
void setActionToSelect();

/* Parse methods */
DlElement *parseChoiceButton(DisplayInfo *);
DlElement *parseMessageButton(DisplayInfo *);
DlElement *parseValuator(DisplayInfo *);
DlElement *parseWheelSwitch(DisplayInfo *);
DlElement *parseTextEntry(DisplayInfo *);
DlElement *parseMenu(DisplayInfo *);
void parseControl(DisplayInfo *, DlControl *control);
DlElement *parseImage(DisplayInfo *);
DlElement *parseComposite(DisplayInfo *);
DlElement *parsePolyline(DisplayInfo *);
DlElement *parsePolygon(DisplayInfo *);
DlElement *parseMeter(DisplayInfo *);
DlElement *parseBar(DisplayInfo *);
DlElement *parseByte(DisplayInfo *);
DlElement *parseIndicator(DisplayInfo *);
DlElement *parseTextUpdate(DisplayInfo *);
DlElement *parseStripChart(DisplayInfo *);
DlElement *parseCartesianPlot(DisplayInfo *);
void parseMonitor(DisplayInfo *displayInfo, DlMonitor *monitor);
void parsePlotcom(DisplayInfo *displayInfo, DlPlotcom *plotcom);
void parsePen(DisplayInfo *displayInfo, DlPen *pen);
void parseTrace(DisplayInfo *displayInfo, DlTrace *trace);
void parsePlotAxisDefinition(DisplayInfo *displayInfo,
  DlPlotAxisDefinition *axisDefinition);

/* medmStripChart.c */
void popupStripChartDataDialog(void);
void resetStripChartDataDialog(void);
void stripChartUpdateMatrixColors(int clr, int row);

/* resourcePalette.c */
void clearResourcePaletteEntries(void);
void createResource(void);
void initializeGlobalResourceBundle(void);
void medmGetValues(ResourceBundle *pRB, ...);
void resetGlobalResourceBundleAndResourcePalette(void);
void setResourcePaletteEntries(void);
void textFieldActivateCallback(Widget w, XtPointer, XtPointer);
void textFieldFloatVerifyCallback(Widget, XtPointer, XtPointer);
void textFieldLosingFocusCallback(Widget w, XtPointer, XtPointer);
void textFieldNumericVerifyCallback(Widget, XtPointer, XtPointer);
void updateElementBackgroundColorFromGlobalResourceBundle(DlElement *element);
void updateElementForegroundColorFromGlobalResourceBundle(DlElement *element);
void updateElementFromGlobalResourceBundle(DlElement *elementPtr);
void updateGlobalResourceBundleAndResourcePalette(Boolean objectDataOnly);
void updateGlobalResourceBundleFromElement(DlElement *element);
void updateRelatedDisplayDataDialog(void);
void initializeXmStringValueTables();

/* updateTask.c */
UpdateTask *getUpdateTaskFromWidget(Widget sourceWidget);
UpdateTask *getUpdateTaskFromPosition(DisplayInfo *displayInfo, int x, int y);
void updateStatusFields(void);
double medmTime();
double medmElapsedTime();
double medmResetElapsedTime();
void updateTaskInitHead(DisplayInfo *displayInfo);
UpdateTask *updateTaskAddTask(DisplayInfo *, DlObject *, void (*)(XtPointer),
  XtPointer);
void updateTaskDeleteAllTask(UpdateTask *);
#if 0
void updateTaskDeleteTask(DisplayInfo *displayInfo, UpdateTask *pt);
void updateTaskDeleteElementTasks(DisplayInfo *displayInfo, DlElement *pE);
#endif
void updateTaskDisableTask(DlElement *dlElement);
void updateTaskEnableTask(DlElement *dlElement);
int updateTaskMarkTimeout(UpdateTask *, double);
void updateTaskSetScanRate(UpdateTask *, double);
void updateTaskAddExecuteCb(UpdateTask *, void (*)(XtPointer));
void updateTaskAddDestroyCb(UpdateTask *, void (*)(XtPointer));
void updateTaskMarkUpdate(UpdateTask *pt);
void updateTaskRepaintRect(DisplayInfo *displayInfo, XRectangle *clipRect,
  Boolean redrawPixmap);
void updateTaskRepaintRegion(DisplayInfo *displayInfo, Region region);
void medmInitializeUpdateTasks(void);
void updateTaskStatusGetInfo(int *taskCount,
  int *periodicTaskCount,
  int *updateRequestCount,
  int *updateDiscardCount,
  int *periodicUpdateRequestCount,
  int *periodicUpdateDiscardCount,
  int *updateRequestQueued,
  int *updateExecuted,
  double *timeInterval);
void updateTaskAddNameCb(UpdateTask *, void (*)(XtPointer, Record **, int *));
void startMedmScheduler(void);
void stopMedmScheduler(void);
void dumpUpdatetaskList(DisplayInfo *displayInfo);
UpdateTask *getUpdateTaskFromElement(DlElement *dlElement);

/* updateMonitors.c */
void localCvtDoubleToString( double, char *, unsigned short);
void localCvtDoubleToExpNotationString(double, char *, unsigned short);
int localCvtLongToHexString(long source, char *pdest);
void medmLocalCvtDoubleToSexaStr(double value,  char *string,
  unsigned short prec, double hopr, double lopr, int *status);
double strtos(char *string, double hopr, double lopr, char **rptr, int *status);

void traverseMonitorList(Boolean forcedTraversal, DisplayInfo *displayInfo,
  int regionX, int regionY, unsigned int regionWidth,
  unsigned int regionHeight);
void updateTextUpdate(UpdateTask *);
#if 0
void draw3DQuestionMark(UpdateTask *);
void draw3DPane(UpdateTask *, Pixel);
#endif
void drawWhiteRectangle(UpdateTask *);
void drawBlackRectangle(UpdateTask *);
void drawColoredRectangle(UpdateTask *pt, Pixel pixel);

/* display.c */
DisplayInfo *allocateDisplayInfo(void);
void dmDisplayListParse(DisplayInfo *, FILE *, char *, char *, char*, Boolean);
TOKEN parseAndAppendDisplayList(DisplayInfo *displayInfo, DlList *dlList,
  char *firstToken, TOKEN firstTokenType);
FILE *dmOpenUsableFile(char *filename, char *relatedDisplayFilename);
void clearDlDisplayList(DisplayInfo *displayInfo, DlList *list);
void removeDlDisplayListElementsExceptDisplay(DisplayInfo * displayInfo,
  DlList *list);
void dmCleanupDisplayInfo(DisplayInfo *displayInfo, Boolean cleanupDisplayList);
void dmRemoveDisplayInfo(DisplayInfo *displayInfo);
void dmRemoveAllDisplayInfo(void);
void dmTraverseDisplayList(DisplayInfo *displayInfo);
void dmTraverseAllDisplayLists(void);
void dmTraverseNonWidgetsInDisplayList(DisplayInfo *displayInfo);
DisplayInfo *dmGetDisplayInfoFromWidget(Widget widget);
void dmWriteDisplayList(DisplayInfo *displayInfo, FILE *stream);
void medmSetDisplayTitle(DisplayInfo *displayInfo);
void medmMarkDisplayBeingEdited(DisplayInfo *displayInfo);


/* utils.c */
long longFval(double f);
Pixel alarmColor(int type);
Boolean extractStringBetweenColons(char *input, char *output, int startPos,
  int  *endPos);
int isPath(const char *fileString);
int convertNameToFullPath(const char *name, char *pathName, int nChars);
void convertDirDelimiterToWIN32(char *pathName);
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
DlElement *findHiddenRelatedDisplay(DisplayInfo *displayInfo,
  Position x0, Position y0);
DlElement *findSmallestTouchedElement(DlList *pList, Position x0, Position y0,
  Boolean dynamicOnly);
DlElement *findSmallestTouchedExecuteElement(Widget w, DisplayInfo *displayInfo,
  Position *x, Position *y, Boolean dynamicOnly);
void findSelectedEditElements(DlList *pList1, Position x0, Position y0,
  Position x1, Position y1, DlList *pList2, unsigned int mode);
void findAllMatchingElements(DlList *pList1, Position x0, Position y0,
  Position x1, Position y1, DlList *pList2, unsigned int mode);
DlElement *lookupDynamicAttributeElement(DlElement *elementPtr);
DlElement *lookupBasicAttributeElement(DlElement *elementPtr);
Boolean dmResizeDisplayList(DisplayInfo *displayInfo, Dimension newWidth,
  Dimension newHeight);
Boolean dmResizeSelectedElements(DisplayInfo *displayInfo, Dimension newWidth,
  Dimension newHeight);
void initializeRubberbanding(void);
int doRubberbanding(Window window, Position *initialX, Position *initialY,
  Position *finalX,  Position *finalY, int doSnap);
Boolean doDragging(Window window, Dimension daWidth, Dimension daHeight,
  Position initialX,Position initialY,Position *finalX,Position *finalY);
Boolean alreadySelected(DlElement *element);
Boolean doResizing(Window window, Position initialX, Position initialY,
  Position *finalX, Position *finalY);
void destroyElementWidgets(DlElement *element);
void drawGrid(DisplayInfo *displayInfo);
void copySelectedElementsIntoClipboard(void);
DlStructurePtr createCopyOfElementType(DlElementType type, DlStructurePtr ptr);
int copyElementsIntoDisplay(void);
void deleteElementsInDisplay(DisplayInfo * displayInfo);
void unselectElementsInDisplay(void);
void selectAllElementsInDisplay(void);
void selectDisplay(void);
void lowerSelectedElements(void);
void raiseSelectedElements(void);
void alignSelectedElements(int alignment);
void findOutliers(void);
void centerSelectedElements(int alignment);
void orientSelectedElements(int alignment);
void sizeSelectedTextElements(void);
void equalSizeSelectedElements(void);
void alignSelectedElementsToGrid(Boolean edges);
void moveElementAfter(DlElement *dst, DlElement *src, DlElement **tail);
void moveSelectedElementsAfterElement(DisplayInfo *displayInfo,
  DlElement *afterThisElement);
void spaceSelectedElements(int plane);
void spaceSelectedElements2D(void);
void deleteAndFreeElementAndStructure(DisplayInfo *displayInfo, DlElement *ele);
NameValueTable *generateNameValueTable(char *argsString, int *numNameValues);
char *lookupNameValue(NameValueTable *nameValueTable, int numEntries,
  char *name);
void freeNameValueTable(NameValueTable *nameValueTable, int numEntries);
DisplayInfo *findDisplay(char *filename, char *argsString,
  char *relatedDisplayFilename);
void performMacroSubstitutions(DisplayInfo *displayInfo,
  char *inputString, char *outputString, int sizeOfOutputString);
void optionMenuSet(Widget menu, int buttonId);
void optionMenuRemoveLabel(Widget menu);
#if EXPLICITLY_OVERWRITE_CDE_COLORS
void colorMenuBar(Widget widget, Pixel fg, Pixel bg);
void colorPulldownMenu(Widget widget, Pixel fg, Pixel bg);
#endif
void appendDlElement(DlList *tail, DlElement *p);
DlList *createDlList();
void emptyDlList(DlList *);
void appendDlList(DlList *, DlList *);
void insertDlElement(DlList *,DlElement *);
void insertAfter(DlList *l, DlElement *p1, DlElement *p2);
void insertDlListAfter(DlList *l1, DlElement *p, DlList *l2);
void removeDlElement(DlList *,DlElement *);
void dumpDlElementList(DlList *l);
void genericMove(DlElement *, int, int);
void genericScale(DlElement *, int, int);
void genericOrient(DlElement *dlElement, int type, int xCenter, int yCenter);
void genericDestroy(DisplayInfo *displayInfo, DlElement *pE);
void resizeDlElementList(
  DlList *dlElementList,
  int x,
  int y,
  float scaleX,
  float scaleY);
void resizeDlElementReferenceList(
  DlList *dlElementList,
  int x,
  int y,
  float scaleX,
  float scaleY);
void createUndoInfo(DisplayInfo *displayInfo);
void destroyUndoInfo(DisplayInfo *displayInfo);
void clearUndoInfo(DisplayInfo *displayInfo);
void saveUndoInfo(DisplayInfo *displayInfo);
void restoreUndoInfo(DisplayInfo *displayInfo);
void updateAllDisplayPositions();
void setTimeValues(void);
void parseAndExecCommand(DisplayInfo *displayInfo, char * cmd);
void print(const char *fmt, ...);
Boolean isConnected(Record **records);
Boolean isStaticDynamic(DlDynamicAttribute *dynAttr, Boolean includeColor);
Boolean calcVisibility(DlDynamicAttribute *attr, Record **records);
void calcPostfix(DlDynamicAttribute *attr);
void setDynamicAttrMonitorFlags(DlDynamicAttribute *attr, Record **records);
int calcUsesStatus(char *calc);
int calcUsesSeverity(char *calc);
char *shortName(char *filename);
/* Debugging */
void resetTimer(void);
double getTimerDouble(void);
struct timeval getTimerTimeval(void);
void printEventMasks(Display *display, Window win, char *string);
void printWindowAttributes(Display *display, Window win, char *string);
char *getEventName(int type);
void dumpDisplayInfoList(DisplayInfo *head, char *string);
void dumpPixmap(Pixmap pixmap, Dimension width, Dimension height, char *title);
/* XR5 Resource ID patch */
#ifdef USE_XR5_RESOURCEID_PATCH
Pixmap XPatchCreatePixmap(Display *dpy, Drawable drawable, unsigned int width,
  unsigned int height, unsigned int depth);
GC XPatchCreateGC(Display *dpy, Drawable drawable, unsigned long valueMask,
  XGCValues *gcValues);
int XPatchFreeGC(Display *dpy,  GC gc);
int XPatchFreePixmap(Display *dpy, Pixmap pixmap);
#endif

/* xgif.c */
Boolean initializeGIF(DisplayInfo *displayInfo, DlImage *dlImage);
void drawGIF(DisplayInfo *displayInfo, DlImage *dlImage, Drawable drawable);
void resizeGIF(DlImage *dlImage);
void freeGIF(DlImage *dlImage);
void copyGIF(DlImage *dlImage1, DlImage *dlImage2);

/* medmCommon.c */
void objectAttributeInit(DlObject *object);
void basicAttributeInit(DlBasicAttribute *attr);
void dynamicAttributeInit(DlDynamicAttribute *dynAttr);
void limitsAttributeInit(DlLimits *limits);
int initMedmCommon();
void destroyDlElement(DisplayInfo *, DlElement *);
void hideDrawnElement(DisplayInfo *displayInfo, DlElement *dlElement);
void hideWidgetElement(DisplayInfo *displayInfo, DlElement *dlElement);
void objectAttributeSet(DlObject *object, int x, int y, unsigned int width,
  unsigned int height);
DlFile *parseFile(DisplayInfo *displayInfo);
DlColormap *parseColormap(DisplayInfo *displayInfo, FILE *filePtr);
void parseBasicAttribute(DisplayInfo *, DlBasicAttribute *);
void parseDynamicAttribute(DisplayInfo *, DlDynamicAttribute *);
void parseOldBasicAttribute(DisplayInfo *, DlBasicAttribute *);
void parseOldDynamicAttribute(DisplayInfo *, DlDynamicAttribute *);
void parseLimits(DisplayInfo *displayInfo, DlLimits *limits);
DlElement *parseRectangle(DisplayInfo *);
DlElement *parseOval(DisplayInfo *);
DlElement *parseArc(DisplayInfo *);
DlElement *parseText(DisplayInfo *);
DlElement *parseRisingLine(DisplayInfo *);
DlElement *parseFallingLine(DisplayInfo *);
DlElement *parseRelatedDisplay(DisplayInfo *);
DlElement * parseShellCommand(DisplayInfo *);
void parseDlColor(DisplayInfo *displayInfo, FILE *filePtr,
  DlColormapEntry *dlColor);
void parseObject(DisplayInfo *displayInfo, DlObject *object);
void parseGrid(DisplayInfo *displayInfo, DlGrid *grid);
void parseRelatedDisplayEntry(DisplayInfo *displayInfo,
  DlRelatedDisplayEntry *relatedDisplay);
void parseShellCommandEntry(DisplayInfo *displayInfo,
  DlShellCommandEntry *shellCommand);
DlColormap *parseAndExtractExternalColormap(DisplayInfo *displayInfo,
  char *filename);
void parseAndSkip(DisplayInfo *displayInfo);
TOKEN getToken(DisplayInfo *displayInfo, char *word);

#endif  /* __PROTO_H__ */
