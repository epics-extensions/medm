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
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (630-252-2000).
*/
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


/* actions.c */
void StartDrag(Widget w, XEvent *event);

/* bubbleHelp.c */
void handleBubbleHelp(Widget w, XtPointer clientData, XEvent *event,
  Boolean *ctd);

/* browserHelp.c */
int callBrowser(char *url);

/* callbacks.c */
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

/* channelPalette.c */
void createChannel(void);

/* colorPalette.c */
void createColor(void);
void setCurrentDisplayColorsInColorPalette(int rcType, int index);

/* createControllers.c */
DlElement *createDlChoiceButton(DlElement *);
DlElement *createDlMenu(DlElement *);
DlElement *createDlMessageButton(DlElement *);
DlElement *createDlTextEntry(DlElement *);
DlElement *createDlValuator(DlElement *);
void controlAttributeInit(DlControl *control);

/* createExtensions.c */
DlElement *createDlImage(DlElement *);
DlElement *createDlComposite(DlElement *);
DlElement *handleImageCreate();
DlElement *createDlPolyline(DlElement *);
DlElement *createDlPolygon(DlElement *);
DlElement *handlePolylineCreate(int x0, int y0, Boolean simpleLine);
DlElement *handlePolygonCreate(int x0, int y0);

/* createMonitors.c */
DlElement *createDlMeter(DlElement *);
DlElement *createDlBar(DlElement *);
DlElement *createDlByte(DlElement *);
DlElement *createDlIndicator(DlElement *);
DlElement *createDlTextUpdate(DlElement *);
DlElement *createDlStripChart(DlElement *);
DlElement *createDlCartesianPlot(DlElement *);
void monitorAttributeInit(DlMonitor *monitor);

/* createStatics.c */
DlElement* createDlElement(DlElementType, XtPointer, DlDispatchTable *);
DlFile *createDlFile(DisplayInfo *displayInfo);
DlElement *createDlDisplay(DlElement *);
DlColormap *createDlColormap(DisplayInfo *displayInfo);
DlElement *createDlRectangle(DlElement *);
DlElement *createDlOval(DlElement *);
DlElement *createDlArc(DlElement *);
DlElement *createDlText(DlElement *);
DlElement *createDlRelatedDisplay(DlElement *);
DlElement *createDlShellCommand(DlElement *);
void createDlObject(DisplayInfo *displayInfo, DlObject *object);
DlElement *handleTextCreate(int x0, int y0);
void objectAttributeInit(DlObject *object);
void basicAttributeInit(DlBasicAttribute *attr);
void dynamicAttributeInit(DlDynamicAttribute *dynAttr);

/* display.c */
DisplayInfo *createDisplay(void);

/* dmInit.c */
DisplayInfo *allocateDisplayInfo(void);
void dmDisplayListParse(DisplayInfo *, FILE *, char *, char *, char*, Boolean);
TOKEN parseAndAppendDisplayList(DisplayInfo *, DlList *);

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
void updateDraggedElements(Position x0, Position y0, Position x1, Position y1);
void updateResizedElements(Position x0, Position y0, Position x1, Position y1);
DlElement *handleRectangularCreates(DlElementType, int, int, unsigned int, unsigned
  int);
void addCommonHandlers(Widget w, DisplayInfo *displayInfo);

/* executeControllers.c */
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
void executeDlChoiceButton(DisplayInfo *, DlElement *);
void executeDlMessageButton(DisplayInfo *, DlElement *);
void executeDlValuator(DisplayInfo *, DlElement *);
void executeDlTextEntry(DisplayInfo *, DlElement *);
void executeDlMenu(DisplayInfo *, DlElement *);

/* executeExtensions.c */
void executeDlImage(DisplayInfo *, DlElement *);
void executeDlPolyline(DisplayInfo *, DlElement *);
void executeDlPolygon(DisplayInfo *, DlElement *);

/* executeMonitors.c */
Channel *allocateChannel(
  DisplayInfo *displayInfo);
void executeDlMeter(DisplayInfo *, DlElement *);
void executeDlBar(DisplayInfo *, DlElement *);
void executeDlByte(DisplayInfo *, DlElement *);
void executeDlIndicator(DisplayInfo *, DlElement *);
void executeDlTextUpdate(DisplayInfo *, DlElement *);
void executeDlStripChart(DisplayInfo *, DlElement *);
void executeDlCartesianPlot(DisplayInfo *, DlElement *);
void executeDlSurfacePlot(DisplayInfo *, DlElement *);

/* executeStatics.c */
void executeDlComposite(DisplayInfo *, DlElement *);
void executeDlDisplay(DisplayInfo *, DlElement *);
void executeDlColormap(DisplayInfo *, DlColormap *);
void executeDlBasicAttribute(DisplayInfo *, DlBasicAttribute *);
void executeDlDynamicAttribute(DisplayInfo *, DlElement *);
void executeDlRectangle(DisplayInfo *, DlElement *);
void executeDlOval(DisplayInfo *, DlElement *);
void executeDlArc(DisplayInfo *, DlElement *);
void executeDlText(DisplayInfo *, DlElement *);
void executeDlRelatedDisplay(DisplayInfo *, DlElement *);
void executeDlShellCommand(DisplayInfo *, DlElement *);

/* help.c */
void errMsgDlgCreateDlg();
void globalHelpCallback(Widget, XtPointer, XtPointer);
void medmPostMsg(char *format, ...);
void medmPostTime();
void medmPrintf(char *format, ...);
void medmCreateCAStudyDlg();
void medmStartUpdateCAStudyDlg();
int xErrorHandler(Display *dpy, XErrorEvent *event);
void xtErrorHandler(char *message);

/* medm.c */
int main(int argc, char *argv[]);
void createEditModeMenu(DisplayInfo *displayInfo);
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
void medmSendCharacterArray(Record *, char *, unsigned long);
void CATaskGetInfo(int *, int *, int *);
Channel *getChannelFromRecord(Record *pRecord);

/* medmPixmap.c */
void medmInitializeImageCache(void);
void medmClearImageCache(void);

/* medmValuator.c */
void popupValuatorKeyboardEntry(Widget, DisplayInfo *, XEvent *);

/* objectPalette.c */
void createObject(void);
void objectMenuCallback(Widget,XtPointer,XtPointer);
void objectPaletteSetSensitivity(Boolean);
void setActionToSelect();

/* parseControllers.c */
DlElement *parseChoiceButton(DisplayInfo *); 
DlElement *parseMessageButton(DisplayInfo *);
DlElement *parseValuator(DisplayInfo *);
DlElement *parseTextEntry(DisplayInfo *);
DlElement *parseMenu(DisplayInfo *);
void parseControl(DisplayInfo *, DlControl *control);

/* parseExtensions.c */
DlElement *parseImage(DisplayInfo *);
DlElement *parseComposite(DisplayInfo *);
DlElement *parsePolyline(DisplayInfo *);
DlElement *parsePolygon(DisplayInfo *);

/* parseMonitors.c */
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

/* parseStatics.c */
DlFile *parseFile(DisplayInfo *displayInfo);
DlElement *parseDisplay(DisplayInfo *displayInfo);
DlColormap *parseColormap(DisplayInfo *displayInfo, FILE *filePtr);
void parseBasicAttribute(DisplayInfo *, DlBasicAttribute *);
void parseDynamicAttribute(DisplayInfo *, DlDynamicAttribute *);
void parseOldBasicAttribute(DisplayInfo *, DlBasicAttribute *);
void parseOldDynamicAttribute(DisplayInfo *, DlDynamicAttribute *);
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
void clearResourcePaletteEntries(void);
void setResourcePaletteEntries(void);
void updateGlobalResourceBundleFromElement(DlElement *element);
void updateElementForegroundColorFromGlobalResourceBundle(DlElement *element);
void updateElementBackgroundColorFromGlobalResourceBundle(DlElement *element);
void updateElementFromGlobalResourceBundle(DlElement *elementPtr);
void updateGlobalResourceBundleAndResourcePalette(Boolean objectDataOnly);
void resetGlobalResourceBundleAndResourcePalette(void);
void medmGetValues(ResourceBundle *pRB, ...);

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
void updateTaskAddNameCb(UpdateTask *, void (*)(XtPointer, Record **, int *));



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
int localCvtLongToHexString(long source, char *pdest);
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
DlElement *findSmallestTouchedElement(DlList *pList, Position x0, Position y0);
DlElement *findSmallestTouchedExecuteElementFromWidget(Widget w,
  DisplayInfo *displayInfo, Position *x, Position *y);
void findSelectedElements(DlList *pList1, Position x0, Position y0,
  Position x1, Position y1, DlList *pList2, unsigned int mode);
void findAllMatchingElements(DlList *pList1, Position x0, Position y0,
  Position x1, Position y1, DlList *pList2, unsigned int mode);
DlElement *lookupCompositeChild(DlElement *composite, Position x0, Position y0);
DlElement *lookupCompositeElement(DlElement *elementPtr);
DlElement *lookupDynamicAttributeElement(DlElement *elementPtr);
DlElement *lookupBasicAttributeElement(DlElement *elementPtr);
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
void destroyElementWidgets(DlElement *element);
void drawGrid(DisplayInfo *displayInfo);
void copySelectedElementsIntoClipboard(void);
DlStructurePtr createCopyOfElementType(DlElementType type, DlStructurePtr ptr);
void copyElementsIntoDisplay(void);
void deleteElementsInDisplay(void);
void unselectElementsInDisplay(void);
void selectAllElementsInDisplay(void);
void lowerSelectedElements(void);
void ungroupSelectedElements(void);
void raiseSelectedElements(void);
void alignSelectedElements(int alignment);
void equalSizeSelectedElements(void);
void refreshDisplay(void);
void alignSelectedElementsToGrid(void);
void moveElementAfter(DlElement *dst, DlElement *src, DlElement **tail);
void moveSelectedElementsAfterElement(DisplayInfo *displayInfo,
  DlElement *afterThisElement);
void spaceSelectedElements(int plane);
void spaceSelectedElements2D(void);
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
void clearDlDisplayList(DlList *);
void removeDlDisplayListElementsExceptDisplay(DlList *list);
#ifdef __COLOR_RULE_H__
Pixel extractColor(DisplayInfo *displayInfo, double value, int colorRule, int defaultColor);
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
void destroyElementWithDynamicAttribute(DlElement *dlElement);
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
#if 0
char *allocateString();
void freeString(char *string);
void destroyFreeStringList();
#endif
void createUndoInfo(DisplayInfo *displayInfo);
void destroyUndoInfo(DisplayInfo *displayInfo);
void clearUndoInfo(DisplayInfo *displayInfo);
void saveUndoInfo(DisplayInfo *displayInfo);
void restoreUndoInfo(DisplayInfo *displayInfo);
void updateAllDisplayPositions();
void setTimeValues(void);
void popupPvInfo(DisplayInfo *displayInfo);
void createPvInfoDlg(void);
Record **getPvInfoFromDisplay(DisplayInfo *displayInfo, int *count);
/* Debugging */
void dumpCartesianPlot(void);
void printEventMasks(Display *display, Window win, char *string);

/* medmWidget.c */
void medmInit(char *displayFontName);
void dmTerminateX(void);
unsigned long getPixelFromColormapByString(Display *display, int screen,
  Colormap cmap, char *colorString);
int initMedmWidget();
int destroyMedmWidget();
void hsort(double array[], int indx[], int n);


/* writeControllers.c */
void writeDlChoiceButton(FILE *, DlElement *, int);
void writeDlMessageButton(FILE *, DlElement *, int);
void writeDlValuator(FILE *, DlElement *, int);
void writeDlTextEntry(FILE *, DlElement *, int);
void writeDlMenu(FILE *, DlElement *, int);
void writeDlControl(FILE *, DlControl *, int);

/* writeExtensions.c */
void writeDlImage(FILE *, DlElement *, int);
void writeDlCompositeChildren(FILE *, DlElement *, int);
void writeDlComposite(FILE *, DlElement *, int);
void writeDlPolyline(FILE *, DlElement *, int);
void writeDlPolygon(FILE *, DlElement *, int);

/* writeMonitors.c */
void writeDlMeter(FILE *, DlElement *, int);
void writeDlBar(FILE *, DlElement *, int);
void writeDlByte(FILE *, DlElement *, int);
void writeDlIndicator(FILE *, DlElement *, int);
void writeDlTextUpdate(FILE *, DlElement *, int);
void writeDlStripChart(FILE *, DlElement *, int);
void writeDlCartesianPlot(FILE *, DlElement *, int);
void writeDlSurfacePlot(FILE *, DlElement *, int);
void writeDlMonitor(FILE *, DlMonitor *, int);
void writeDlPlotcom(FILE *, DlPlotcom *, int);
void writeDlPen(FILE *, DlPen *, int, int);
void writeDlTrace(FILE *, DlTrace *, int, int);
void writeDlPlotAxisDefinition(FILE *, DlPlotAxisDefinition *, int, int);

/* writeStatics.c */
void writeDlComposite(FILE *, DlElement *, int);
void writeDlFile(FILE *, DlFile *, int);
void writeDlDisplay(FILE *, DlElement *, int);
void writeDlColormap(FILE *, DlColormap *, int);
void writeDlBasicAttribute(FILE *, DlBasicAttribute *, int);
void writeDlDynamicAttribute(FILE *, DlDynamicAttribute *, int);
void writeDlRectangle(FILE *, DlElement *, int);
void writeDlOval(FILE *, DlElement *, int);
void writeDlArc(FILE *, DlElement *, int);
void writeDlText(FILE *, DlElement *, int);
void writeDlRelatedDisplay(FILE *, DlElement *, int);
void writeDlShellCommand(FILE *, DlElement *, int);
void writeDlColormapEntry(FILE *, DlElement *, int);
void writeDlObject(FILE *, DlObject *, int);
void writeDlRelatedDisplayEntry(FILE *, DlRelatedDisplayEntry *, int, int);
void writeDlShellCommandEntry(FILE *, DlShellCommandEntry *, int, int);

/* xgif.c */
Boolean initializeGIF(DisplayInfo *displayInfo, DlImage *dlImage);
void drawGIF(DisplayInfo *displayInfo, DlImage *dlImage);
void resizeGIF(DisplayInfo *displayInfo, DlImage *dlImage);
Boolean loadGIF(DisplayInfo *displayInfo, DlImage *dlImage);
int ReadCode(void);
void freeGIF(DlImage *dlImage);

/* medmComposite.c */
DlElement *groupObjects();

/* medmCommon.c */
int initMedmCommon();
void destroyDlElement(DlElement *);
void objectAttributeSet(DlObject *object, int x, int y, unsigned int width,
  unsigned int height);

/* medmRelatedDisplay.c */
void relatedDisplayDataDialogPopup(Widget w);
void relatedDisplayCreateNewDisplay(DisplayInfo *displayInfo,
  DlRelatedDisplayEntry *pEntry);

/* medmMonitor.c */
void plotAxisDefinitionInit(DlPlotAxisDefinition *axisDefinition);
void plotcomAttributeInit(DlPlotcom *plotcom);
void penAttributeInit(DlPen *pen);
void traceAttributeInit(DlTrace *trace);
/* help_protocol.c */
void help_protocol (Widget shell);

#endif  /* __PROTO_H__ */
