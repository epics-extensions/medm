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

/*
 * include file for widget-based display manager
 */
#ifndef __MEDMWIDGET_H__
#define __MEDMWIDGET_H__

#ifdef ALLOCATE_STORAGE
#define EXTERN
#else
#define EXTERN extern
#endif

#include <limits.h>
#include <sys/types.h>
#if 0
#include <sys/times.h>
#else
#include <time.h>
#endif
#include "xtParams.h"

#include "displayList.h"

/* this is ugly, but we need it for the action table */
#if 0
extern void popupValuatorKeyboardEntry(Widget, XEvent*, String *, Cardinal *);
#endif

#ifndef MAX
#define MAX(a,b)  ((a)>(b)?(a):(b))
#endif
#ifndef MIN
#define MIN(a,b)  ((a)<(b)?(a):(b))
#endif

#ifndef STRDUP
#define STRDUP(a) (strcpy( (char *)malloc(strlen(a)+1),a))
#endif

/* Limits definitions */
#define LOPR_DEFAULT 0.0
#define HOPR_DEFAULT 1.0
#define PREC_DEFAULT   0

/* File suffix definitions (as dir/file masks for file selection boxes) */
#define DISPLAY_DIALOG_MASK	"*.adl"	/* ascii display files */
#define RESOURCE_DIALOG_MASK	"*.rsc"	/* resource files */
#define COLOR_DIALOG_MASK	"*.clr"	/* colormap/color editor files */
#define CHANNEL_DIALOG_MASK	"*.chn"	/* channel list files */

#define DISPLAY_DIALOG_TITLE	"EPICS Display Manager"
#define DISPLAY_DIALOG_CANCEL_STRING	"Exit DM"
#define DISPLAY_FILE_BACKUP_SUFFIX	"_BAK"
#define DISPLAY_FILE_ASCII_SUFFIX	".adl"
#define DISPLAY_FILE_BINARY_SUFFIX	".dl"
#ifndef VMS
#define DISPLAY_XWD_FILE		"/tmp/medm.xwd"
#else
#define DISPLAY_XWD_FILE		"sys$scratch:medm.xwd"
#endif
#define MORE_TO_COME_SYMBOL	"..."	/* string for dialog popups        */
#define SHELL_CMD_PROMPT_CHAR	'?'	/* shell cmd. prompt indicator     */
#define DUMMY_TEXT_FIELD	"9.876543" /* dummy string for text calc.  */

#define NUM_EXECUTE_POPUP_ENTRIES       7         /* items in exec. popup menu  */
#define EXECUTE_POPUP_MENU_PRINT        "Print"   /* if this changes, change    */
#define EXECUTE_POPUP_MENU_CLOSE        "Close"   /* executePopupMenuCallback() */
#define EXECUTE_POPUP_MENU_PVINFO       "PV Info"
#define EXECUTE_POPUP_MENU_PVLIMITS     "PV Limits"
#define EXECUTE_POPUP_MENU_DISPLAY_LIST "Display List"
#define EXECUTE_POPUP_MENU_FLASH_HIDDEN "Flash Hidden Buttons"
#define EXECUTE_POPUP_MENU_EXECUTE      "Execute"
#define EXECUTE_POPUP_MENU_PRINT_ID        0
#define EXECUTE_POPUP_MENU_CLOSE_ID        1
#define EXECUTE_POPUP_MENU_PVINFO_ID       2
#define EXECUTE_POPUP_MENU_PVLIMITS_ID     3
#define EXECUTE_POPUP_MENU_DISPLAY_LIST_ID 4
#define EXECUTE_POPUP_MENU_FLASH_HIDDEN_ID 5
/* The following must be the last item */
#define EXECUTE_POPUP_MENU_EXECUTE_ID      6

#define COLOR_SCALE		(65535.0/255.0)
#define MAX_CHILDREN		1000	/* max # of child widgets...       */
#define MAX_TEXT_UPDATE_WIDTH	64	/* max length of text update       */
#define BORDER_WIDTH		5	/* border for valuator (for MB2)   */

/* Argument passing in related displays */
#define RELATED_DISPLAY_FILENAME_INDEX 0
#define RELATED_DISPLAY_ARGS_INDEX 1
#define RELATED_DISPLAY_FILENAME_AND_ARGS_SIZE	(RELATED_DISPLAY_ARGS_INDEX+1)

/* Display object or widget specific parameters */
#define METER_OKAY_SIZE		80	/* (meter>80) ? 11 : 5 markers     */
#define METER_MARKER_DIVISOR	9	/* height/9 = marker length        */
#define METER_FONT_DIVISOR	8	/* height/8 = height of font       */
#define METER_VALUE_FONT_DIVISOR 10	/* height/10 = height of value font*/
#define INDICATOR_MARKER_WIDTH	4	/* good "average" width            */
#define INDICATOR_MARKER_DIVISOR 20	/* height/20 = marker length (pair)*/
#define INDICATOR_OKAY_SIZE	80	/* (meter>80) ? 11 : 5 markers     */
#define INDICATOR_FONT_DIVISOR	8	/* height/8 = height of font       */
#define INDICATOR_VALUE_FONT_DIVISOR 10	/* height/10 = height of value font*/
#define GOOD_MARGIN_DIVISOR	10	/* height/10 = marginHeight/Width  */
#define BAR_TOO_SMALL_SIZE	30	/* bar is too small for extra data */

#define STRIP_NSAMPLES		60	/* number of samples to save       */

#define SECS_PER_MIN		60.0
#define SECS_PER_HOUR		(60.0 * SECS_PER_MIN)
#define SECS_PER_DAY		(24.0 * SECS_PER_HOUR)

/* Default display name, widths, heights for newly created displays */
#define DEFAULT_FILE_NAME	"newDisplay.adl"
#define DEFAULT_DISPLAY_WIDTH	400
#define DEFAULT_DISPLAY_HEIGHT	400

/* Default grid parameters */
#define DEFAULT_GRID_SPACING 5
#define DEFAULT_GRID_SNAP    False
#define DEFAULT_GRID_ON      False

/* Space parameters */
#define SPACE_HORIZ 0
#define SPACE_VERT  1

/* Align parameters */
#define ALIGN_HORIZ_LEFT   0
#define ALIGN_HORIZ_CENTER 1
#define ALIGN_HORIZ_RIGHT  2
#define ALIGN_VERT_TOP     3
#define ALIGN_VERT_CENTER  4
#define ALIGN_VERT_BOTTOM  5

/* Orient parameters */
#define ORIENT_HORIZ 0
#define ORIENT_VERT  1
#define ORIENT_CW    2
#define ORIENT_CCW   3

/* Highlight line thickness */
#define HIGHLIGHT_LINE_THICKNESS 2

/* Valuator-specific parameters */
#define VALUATOR_MIN		0
#define VALUATOR_MAX 		1000000
#define VALUATOR_INCREMENT 	1
#define VALUATOR_MULTIPLE_INCREMENT 2

/*
 * add in action table for complicated actions
 */
#if 0
static XtActionsRec actions[] = {
    {"popupValuatorKeyboardEntry",popupValuatorKeyboardEntry},
};
#endif

/************************************************************************
 * special types
 ************************************************************************/

/* Shell types that MEDM must worry about */
typedef enum {DISPLAY_SHELL, OTHER_SHELL} ShellType;

/* Action types for MB in display (based on object palette state */
typedef enum {SELECT_ACTION, CREATE_ACTION} ActionType;

/* Update tasks */
typedef struct _UpdateTask {
    void       (*executeTask)(XtPointer);    /* update routine */
    void       (*destroyTask)(XtPointer);
    void       (*getRecord)(XtPointer, Record **, int *);
    Widget     (*widget)(XtPointer);
    XtPointer  clientData;
    double     timeInterval;                 /* if not 0.0, periodic task */
    double     nextExecuteTime;               
    struct     _DisplayInfo *displayInfo;
    int        executeRequestsPendingCount;  /* How many update requests are pending */
    XRectangle rectangle;                    /* Geometry of the object */
    Boolean    overlapped;                   /* Indicates overlapped by others */
    Boolean    opaque;                       /* Indicates whether to redraw under */
    Boolean    disabled;                     /* Indicates not to update */
    struct     _UpdateTask *prev;
    struct     _UpdateTask *next;
} UpdateTask;

/* General private data structure */
typedef struct _MedmElement {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
} MedmElement;

typedef struct _InitTask {
    Boolean init;
    Boolean (*initTask)();
} InitTask;

/* Name-value table (for macro substitutions in display files) */
typedef struct {
    char *name;
    char *value;
} NameValueTable;

/* Undo information structure */
typedef struct {
    DlGrid  grid;
    int drawingAreaBackgroundColor;
    int drawingAreaForegroundColor;
    DlList  *dlElementList;
} UndoInfo;

/* DisplayInfo: EPICS Display specific information, one per display
   file */
typedef struct _DisplayInfo {
    FILE *filePtr;
    Boolean newDisplay;       /* Comes from File|New and not yet saved */
    Boolean elementsExecuted; /* All elements have been executed in EXECUTE */
    int versionNumber;
  /* Widgets and main pixmap */
    Widget shell;
    Widget drawingArea;
    Pixmap drawingAreaPixmap;
    Widget editPopupMenu;
    Widget executePopupMenu;
    Widget cartesianPlotPopupMenu;
    Widget selectedCartesianPlot;
    Widget warningDialog;
    int warningDialogAnswer;
    Widget questionDialog;
    int questionDialogAnswer;
    Widget shellCommandPromptD;
  /* Grid */
    DlGrid *grid;
  /* Undo */
    UndoInfo  *undoInfo;
  /* Widget instance data */
#if 0
    Widget child[MAX_CHILDREN];        /* children of drawing area */
    int childCount;
#endif
  /* Periodic tasks */
    UpdateTask updateTaskListHead;
    UpdateTask *updateTaskListTail;
    int  periodicTaskCount;
  /* Colormap and attribute data (one exists at a time for each display)        */
    Pixel *colormap;
    int dlColormapCounter;
    int dlColormapSize;
    int drawingAreaBackgroundColor;
    int drawingAreaForegroundColor;
    GC gc;
    GC pixmapGC;
  /* Execute or edit mode traversal  */
    DlTraversalMode traversalMode;
    Boolean hasBeenEditedButNotSaved;
    Boolean fromRelatedDisplayExecution;
  /* Display list pointers */
    DlList *dlElementList;
  /* For edit purposes */
    DlList *selectedDlElementList;
    Boolean selectedElementsAreHighlighted;
  /* For macro-substitution in display lists */
    NameValueTable *nameValueTable;
    int numNameValues;
    DlFile *dlFile;
    DlColormap  *dlColormap;
  /* Linked list of displayInfo's    */
    struct _DisplayInfo *next;
    struct _DisplayInfo *prev;
} DisplayInfo;

/* Miscellaneous support structures */
typedef struct {
    XtPointer	controllerData;	/* (ChannelAccessControllerData *) */
    int		nButtons;
    XmButtonType	*buttonType;
    XmStringTable	buttons;
} OptionMenuData;			/* used for MENU type */


/***
 *** ELEMENT_STRING_TABLE for definition of strings for display elements
 ***	any changes of types or ordering of {DL_Valuator,...,DL_Meter,...} etc
 ***	in displayList.h must have corresponding changes in this table!!
 ***/
#ifndef ALLOCATE_STORAGE
extern char *elementStringTable[NUM_DL_ELEMENT_TYPES];
extern XmString elementXmStringTable[NUM_DL_ELEMENT_TYPES];
#else
char *elementStringTable[NUM_DL_ELEMENT_TYPES] = {
    "Element", "Composite", "Display",
    "Choice Button", "Menu", "Message Button", "Related Display",
    "Shell Command", "Text Entry", "Slider",
    "Bar Monitor", "Byte Monitor", "Cartesian Plot", "Scale Monitor", "Meter",
    "Strip Chart", "Text Monitor",
    "Arc", "Image", "Line", "Oval", "Polygon", "Polyline", "Rectangle", "Text"
};
XmString elementXmStringTable[NUM_DL_ELEMENT_TYPES];
#endif

#define elementType(x) (elementStringTable[(x)-DL_Element])


/****************************************************************************
 ****				Resource Bundle type			 ****
 ***
 ***  This gets tricky - this is the aggregate set of all "attributes"
 ***	or resources for any object in MEDM.  There is some intersection
 ***	between objects (e.g., sharing DlObject) but any object ought to
 ***	be able to retrieve its necessary resources from this structure
 ***	somewhere.  Look at displayList.h for definition of these
 ***	structure components... 
 ***	NOTE:  exceptions: Display and Colormap not in here (these are
 ***	modified by other, more user-friendly, methods)
 ****************************************************************************/

/*
 * Resource bundle type
 */
typedef struct _ResourceBundle {

  /* The aggregate types have been decomposed into scalar/atomic types
   * to support the set intersection necessary */
    Position x;
    Position y;
    Dimension width;
    Dimension height;
    char title[MAX_TOKEN_LENGTH];
    char xlabel[MAX_TOKEN_LENGTH];
    char ylabel[MAX_TOKEN_LENGTH];
    int clr;
    int bclr;
    int begin;
    int path;
    TextAlign align;
    TextFormat format;
    LabelType label;
    Direction direction;
    ColorMode clrmod;
#ifdef __COLOR_RULE_H__
    int colorRule;
#endif
    FillMode fillmod;
    EdgeStyle style;
    FillStyle fill;
    VisibilityMode vis;
    char visCalc[MAX_TOKEN_LENGTH];
    char chan[MAX_CALC_RECORDS][MAX_TOKEN_LENGTH];
    int data_clr;
    int dis;
    int xyangle;
    int zangle;
    double period;
    TimeUnits units;
    CartesianPlotStyle cStyle;
    EraseOldest erase_oldest;
    int count;
    Stacking stacking;
    ImageType imageType;
    char textix[MAX_TOKEN_LENGTH];
    char messageLabel[MAX_TOKEN_LENGTH];
    char press_msg[MAX_TOKEN_LENGTH];
    char release_msg[MAX_TOKEN_LENGTH];
    char imageName[MAX_TOKEN_LENGTH];
    char imageCalc[MAX_TOKEN_LENGTH];
    char compositeName[MAX_TOKEN_LENGTH];
    char compositeFile[MAX_TOKEN_LENGTH];
    char data[MAX_TOKEN_LENGTH];
    char cmap[MAX_TOKEN_LENGTH];
    char name[MAX_TOKEN_LENGTH];
    int lineWidth;
    double dPrecision;
    int sbit, ebit;
    char rdLabel[MAX_TOKEN_LENGTH];
    DlTrace cpData[MAX_TRACES];
    DlPen scData[MAX_PENS];
    DlRelatedDisplayEntry rdData[MAX_RELATED_DISPLAYS];
    DlShellCommandEntry cmdData[MAX_SHELL_COMMANDS];

  /* X_AXIS_ELEMENT, Y1_AXIS_ELEMENT, Y2_AXIS_ELEMENT */
    DlPlotAxisDefinition axis[3];

    char trigger[MAX_TOKEN_LENGTH];
    char erase[MAX_TOKEN_LENGTH];
    eraseMode_t eraseMode;

  /* Related display specific */
    relatedDisplayVisual_t rdVisual;

  /* Grid */
    int gridSpacing;
    Boolean gridOn;
    Boolean snapToGrid;

  /* Limits */
    DlLimits limits;

    struct _ResourceBundle *next;	/* linked list of resourceBundle's   */
    struct _ResourceBundle *prev;
} ResourceBundle, *ResourceBundlePtr;


/* Define IDs for the resource entries for displacements into the resource RC
 *   widget array.  The order they appear in the resource palette is the order
 *   in which they appear below.  The labels are in the resourceEntryStringTable
 *   and need to be kept consistent with the index values below. */
#define X_RC              0
#define Y_RC              1
#define WIDTH_RC          2
#define HEIGHT_RC         3
#define RDBK_RC           4
#define CTRL_RC           5
#define LIMITS_RC         6
#define TITLE_RC          7
#define XLABEL_RC         8
#define YLABEL_RC         9

#define CLR_RC            10
#define BCLR_RC           11
#define BEGIN_RC          12
#define PATH_RC           13
#define ALIGN_RC          14
#define FORMAT_RC         15
#define LABEL_RC          16
#define DIRECTION_RC      17
#define FILLMOD_RC        18
#define STYLE_RC          19
#define FILL_RC           20
#define CLRMOD_RC         21
#define VIS_RC            22
#define VIS_CALC_RC       23
#if MAX_CALC_RECORDS != 4
#error Need to make changes if MAX_CALC_RECORDS != 4
#endif      
#define CHAN_A_RC         24
#define CHAN_B_RC         25
#define CHAN_C_RC         26
#define CHAN_D_RC         27
#define DATA_CLR_RC       28
#define DIS_RC            29
#define XYANGLE_RC        30
#define ZANGLE_RC         31
#define PERIOD_RC         32
#define UNITS_RC          33
#define CSTYLE_RC         34
#define ERASE_OLDEST_RC   35
#define COUNT_RC          36
#define STACKING_RC       37
#define IMAGE_TYPE_RC     38
#define TEXTIX_RC         39
#define MSG_LABEL_RC      40
#define PRESS_MSG_RC      41
#define RELEASE_MSG_RC    42
#define IMAGE_NAME_RC     43
#define IMAGE_CALC_RC     44
#define DATA_RC           45
#define CMAP_RC           46
#define NAME_RC           47
#define COMPOSITE_FILE_RC 48
#define LINEWIDTH_RC      49
#define PRECISION_RC      50
#define SBIT_RC           51
#define EBIT_RC           52
#define RD_LABEL_RC       53
#define RD_VISUAL_RC      54

/* Vectors/matrices of data */
#define RDDATA_RC         55  /* Related Display data           */
#define CPDATA_RC         56  /* Cartesian Plot channel data    */
#define SCDATA_RC         57  /* Strip Chart data               */
#define SHELLDATA_RC      58  /* Shell Command data             */
#define CPAXIS_RC         59  /* Cartesian Plot axis data       */
                            
/* Other new entry types */ 
#define TRIGGER_RC        60  /* Cartesian Plot trigger channel */
#define ERASE_RC          61  /* Cartesian Plot erase channel   */
#define ERASE_MODE_RC     62  /* Cartesian Plot erase mode      */

/* Grid */
#define GRID_SPACING_RC   63
#define GRID_ON_RC        64
#define GRID_SNAP_RC      65

#ifndef __COLOR_RULE_H__
# define MAX_RESOURCE_ENTRY (GRID_SNAP_RC + 1)
#else
# define COLOR_RULE_RC    66  /* Color Rule Entry Table         */
# define MAX_RESOURCE_ENTRY (COLOR_RULE_RC + 1)
#endif

#define MIN_RESOURCE_ENTRY 0

/***
 *** resourceEntryStringTable for definition of labels for resource entries
 ***	any changes of types or ordering of above must have corresponding
 ***	changes in this table!!
 ***/
#ifndef ALLOCATE_STORAGE
extern char *resourceEntryStringTable[MAX_RESOURCE_ENTRY];
#else
char *resourceEntryStringTable[MAX_RESOURCE_ENTRY] = {
    "X Position", "Y Position", "Width", "Height",
#ifndef MEDM_CDEV
    "Readback Channel", "Control Channel",
#else
    "Readback Dev/Attr", "Control Dev/Attr",
#endif
    "PV Limits",
    "Title", "X Label", "Y Label",
    "Foreground", "Background",
    "Begin Angle", "Path Angle",
    "Alignment", "Format",
    "Label",
    "Direction",
    "Fill Mode", "Style", "Fill",
    "Color Mode",
    "Visibility",
    "Visibility Calc",
    "Channel A",
    "Channel B",
    "Channel C",
    "Channel D",
    "Data Color", "Distance", "XY Angle", "Z Angle",
    "Period", "Units",
    "Plot Style", "Plot Mode", "Count",
    "Stacking",
    "Image Type",
    "Text",
    "Message Label",
    "Press Message", "Release Message",
    "Image Name",
    "Image Calc",
    "Data",
    "Colormap",
    "Name",
    "Composite File",
    "Line Width",
    "Increment",
    "Start Bit", "End Bit",
    "Label", "Visual",
    "Label/Name/Args",      /* Related Display data           */
    "X/Y/Trace Data",       /* Cartesian Plot data            */
    "Channel/Color",        /* Strip Chart data               */
    "Label/Cmd/Args",       /* Shell Command data             */
    "Axis Data",            /* Cartesian Plot axis data       */
#ifndef MEDM_CDEV
    "Trigger Channel",      /* Cartesian Plot trigger channel */
    "Erase Channel",        /* Cartesian Plot erase channel   */
#else
    "Trigger Dev/Attr",     /* Cartesian Plot trigger device  */
    "Erase Dev/Attr",       /* Cartesian Plot erase device    */
#endif
    "Erase Mode",           /* Cartesian Plot erase mode      */
    "Grid Spacing",
    "Grid On",
    "Snap To Grid",
#ifdef __COLOR_RULE_H__
    "Color Rule",
#endif
};
#endif


/***
 *** resourcePaletteElements for definition of widgets associated with
 ***	various display element types.
 ***	any changes of types or ordering of above definitions, or of
 ***	DL_Element definitions in displayList.h (e.g., DL_Valuator...)
 ***	must have corresponding changes in this table!!
 ***/
#define MAX_RESOURCES_FOR_DL_ELEMENT 18
typedef struct _ResourceMap{
    Cardinal childIndexRC[MAX_RESOURCES_FOR_DL_ELEMENT];
    Cardinal numChildren;
    Widget children[MAX_RESOURCES_FOR_DL_ELEMENT];
} ResourceMap;


/***
 *** see resourcePalette.c (initializeResourcePaletteElements())
 ***    for the enumerated set of dependencies
 ***/
#ifndef ALLOCATE_STORAGE
extern ResourceMap resourcePaletteElements[NUM_DL_ELEMENT_TYPES];
#else
ResourceMap resourcePaletteElements[NUM_DL_ELEMENT_TYPES];
#endif

#ifdef __COLOR_RULE_H__
/* Color Rule */
#define MAX_COLOR_RULES 16
#define MAX_SET_OF_COLOR_RULE 4
typedef struct {
    double lowerBoundary;
    double upperBoundary;
    int    colorIndex;
} colorRule_t;

typedef struct {
    colorRule_t rule[MAX_COLOR_RULES];
} setOfColorRule_t;

#ifndef ALLOCATE_STORAGE
extern setOfColorRule_t setOfColorRule[MAX_SET_OF_COLOR_RULE];
#endif
#endif

/* Global variables */

/* Display, connection, and miscellaneous */
EXTERN XmString dlXmStringMoreToComeSymbol;
EXTERN XmButtonType executePopupMenuButtonType[NUM_EXECUTE_POPUP_ENTRIES];
EXTERN XmString executePopupMenuButtons[NUM_EXECUTE_POPUP_ENTRIES];

EXTERN Pixel defaultForeground, defaultBackground;

/* Should be dimensioned to ALARM_NSEV+1 (alarm.h), with corresponding values */
#define ALARM_MAX 5
EXTERN Pixel alarmColorPixel[ALARM_MAX];

/* Initial (default - not related to any displays) colormap */
EXTERN Pixel defaultColormap[DL_MAX_COLORS];
EXTERN Pixel *currentColormap;
EXTERN int currentColormapSize;
EXTERN Pixel unphysicalPixel;

EXTERN XFontStruct *fontTable[MAX_FONTS];
EXTERN XmFontList fontListTable[MAX_FONTS];

EXTERN DisplayInfo *displayInfoListHead, *displayInfoListTail;
EXTERN DisplayInfo *displayInfoSaveListHead, *displayInfoSaveListTail;
EXTERN DisplayInfo *currentDisplayInfo;

/* (MDA) */
EXTERN DisplayInfo *pointerInDisplayInfo;

EXTERN ResourceBundle globalResourceBundle;
EXTERN Widget resourceEntryRC[MAX_RESOURCE_ENTRY];
EXTERN Widget resourceEntryLabel[MAX_RESOURCE_ENTRY];
EXTERN Widget resourceEntryElement[MAX_RESOURCE_ENTRY];
EXTERN Widget resourceElementTypeLabel;

EXTERN DlTraversalMode globalDisplayListTraversalMode;

/* Flag which says to traverse monitor list */
EXTERN Boolean globalModifiedFlag;

/* For object palette selections and single object selection */
EXTERN DlElementType currentElementType;

/* For clipboard/edit purposes  - note that this has objects and attributes */
#if 0
EXTERN DlElement *clipboardElementsArray;  /* array of DlElements, not ptrs */
EXTERN int numClipboardElements;
EXTERN int clipboardElementsArraySize;
EXTERN Boolean clipboardDelete;
#endif
EXTERN DlList *clipboard;

/* Define MB1 semantics in the display window - select, or create
 * (based on object palette selection) */
EXTERN ActionType currentActionType;

/* For private or shared/default colormap utilization */
EXTERN Boolean privateCmap;

#endif  /* __MEDMWIDGET_H__ */
