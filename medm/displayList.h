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

/****************************************************************************
 * Display list header definition                                           *
 * Mods: DMW - Added 'from center' option to stringValueTable, and          *
 *         FROM_CENTER to FillMode for DlBar                                *
 *       DMW - Added DlByte structure.                                      *
 ****************************************************************************/
#ifndef __DISPLAYLIST_H__
#define __DISPLAYLIST_H__

#include <stdio.h>
#include <sys/types.h>
#include <math.h>
#include <string.h>

#define MAX_TOKEN_LENGTH        256     /* max size of strings in adl    */
#define MAX_RELATED_DISPLAYS    8       /* max # of related displays     */
#define MAX_SHELL_COMMANDS      8       /* max # of shell commands       */
#define MAX_PENS                8       /* max # of pens on strip chart  */
#define MAX_TRACES              8       /* max # of traces on cart. plot */
#define MAX_FILE_CHARS          256     /* max # of chars in filename    */
#define MAX_CALC_RECORDS        4       /* max # of records for calc     */
#define MAX_CALC_INPUTS         12      /* max # of inputs for calc
					   (fixed by EPICS) */
#define DL_MAX_COLORS           65      /* max # of colors for display   */
#define DL_COLORS_COLUMN_SIZE   5       /* # of colors in each column    */

#if MAX_CALC_RECORDS != 4
#error Need to make changes (CALC_A_RC, etc.) if MAX_CALC_RECORDS != 4
#endif      

/*********************************************************************
 * Resource Types                                                    *
 *********************************************************************/
#define NUM_TRAVERSAL_MODES     2
typedef enum {
    DL_EXECUTE  = 0,
    DL_EDIT     = 1
} DlTraversalMode;
#ifdef ALLOCATE_STORAGE
const DlTraversalMode FIRST_TRAVERSAL_MODE = DL_EXECUTE;
#else
extern const DlTraversalMode FIRST_TRAVERSAL_MODE;
#endif

#define NUM_LABEL_TYPES         5
typedef enum {
    LABEL_NONE     = 2,
    NO_DECORATIONS = 3,
    OUTLINE        = 4,
    LIMITS         = 5,
    CHANNEL        = 6
} LabelType;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const LabelType FIRST_LABEL_TYPE = LABEL_NONE;
#else
extern const LabelType FIRST_LABEL_TYPE;
#endif

#define NUM_COLOR_MODES         3
typedef enum {
    STATIC   = 7,
    ALARM    = 8,
    DISCRETE = 9
} ColorMode;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const ColorMode FIRST_COLOR_MODE = STATIC;
#else
extern const ColorMode FIRST_COLOR_MODE;
#endif

#define NUM_VISIBILITY_MODES    4
typedef enum {
    V_STATIC    = 10,
    IF_NOT_ZERO = 11,
    IF_ZERO     = 12,
    V_CALC      = 13
} VisibilityMode;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const VisibilityMode FIRST_VISIBILITY_MODE = V_STATIC;
#else
extern const VisibilityMode FIRST_VISIBILITY_MODE;
#endif

#define NUM_DIRECTIONS          2
typedef enum {
    UP    = 14,
    RIGHT = 15
} Direction;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const Direction FIRST_DIRECTION = UP;
#else
extern const Direction FIRST_DIRECTION;
#endif
/* maybe DOWN, LEFT later */

#define NUM_EDGE_STYLES         2
typedef enum {
    SOLID = 16,
    DASH  = 17
} EdgeStyle;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const EdgeStyle FIRST_EDGE_STYLE = SOLID;
#else
extern const EdgeStyle FIRST_EDGE_STYLE;
#endif

#define NUM_FILL_STYLES         2
typedef enum {
    F_SOLID   = 18,
    F_OUTLINE = 19
} FillStyle;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const FillStyle FIRST_FILL_STYLE = F_SOLID;
#else
extern const FillStyle FIRST_FILL_STYLE;
#endif


#define NUM_TEXT_FORMATS        8
typedef enum {
    MEDM_DECIMAL  = 20,
    EXPONENTIAL   = 21,
    ENGR_NOTATION = 22,
    COMPACT       = 23,
    TRUNCATED     = 24,
    HEXADECIMAL   = 25,
    OCTAL         = 26,
    STRING        = 27
} TextFormat;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const TextFormat FIRST_TEXT_FORMAT = MEDM_DECIMAL;
#else
extern const TextFormat FIRST_TEXT_FORMAT;
#endif

#define NUM_TEXT_ALIGNS         3
typedef enum {
    HORIZ_LEFT   = 28,
    HORIZ_CENTER = 29,
    HORIZ_RIGHT  = 30
} TextAlign;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const TextAlign FIRST_TEXT_ALIGN = HORIZ_LEFT;
#else
extern const TextAlign FIRST_TEXT_ALIGN;
#endif

#define NUM_STACKINGS           3
typedef enum {
    COLUMN      = 31,
    ROW         = 32,
    ROW_COLUMN  = 33
} Stacking;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const Stacking FIRST_STACKING = COLUMN;
#else
extern const Stacking FIRST_STACKING;
#endif

#define NUM_FILL_MODES          2
typedef enum {
    FROM_EDGE   = 34,
    FROM_CENTER = 35 
} FillMode;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const FillMode FIRST_FILL_MODE = FROM_EDGE;
#else
extern const FillMode FIRST_FILL_MODE;
#endif

#define NUM_TIME_UNITS          3
typedef enum {
    MILLISECONDS = 36,
    SECONDS      = 37,
    MINUTES      = 38
} TimeUnits;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const TimeUnits FIRST_TIME_UNIT = MILLISECONDS;
#else
extern const TimeUnits FIRST_TIME_UNIT;
#endif

#define NUM_CARTESIAN_PLOT_STYLES       3
typedef enum {
    POINT_PLOT      = 39,
    LINE_PLOT       = 40,
    FILL_UNDER_PLOT = 41
} CartesianPlotStyle;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const CartesianPlotStyle FIRST_CARTESIAN_PLOT_STYLE = POINT_PLOT;
#else
extern const CartesianPlotStyle FIRST_CARTESIAN_PLOT_STYLE;
#endif

#define NUM_ERASE_OLDESTS       2
typedef enum {
    ERASE_OLDEST_OFF = 42,
    ERASE_OLDEST_ON  = 43
} EraseOldest;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const EraseOldest FIRST_ERASE_OLDEST = ERASE_OLDEST_OFF;
#else
extern const EraseOldest FIRST_ERASE_OLDEST;
#endif

#define NUM_IMAGE_TYPES 3
typedef enum {
    NO_IMAGE   = 44,
    GIF_IMAGE  = 45,
    TIFF_IMAGE = 46
} ImageType;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const ImageType FIRST_IMAGE_TYPE = NO_IMAGE;
#else
extern const ImageType FIRST_IMAGE_TYPE;
#endif

#define NUM_CARTESIAN_PLOT_AXIS_STYLES 3
typedef enum {
    LINEAR_AXIS = 47,
    LOG10_AXIS  = 48,
    TIME_AXIS   = 49
} CartesianPlotAxisStyle;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const CartesianPlotAxisStyle FIRST_CARTESIAN_PLOT_AXIS_STYLE = LINEAR_AXIS;
#else
extern const CartesianPlotAxisStyle FIRST_CARTESIAN_PLOT_AXIS_STYLE;
#endif

#define NUM_CARTESIAN_PLOT_RANGE_STYLES 3
typedef enum {
    CHANNEL_RANGE        = 50,
    USER_SPECIFIED_RANGE = 51,
    AUTO_SCALE_RANGE     = 52
} CartesianPlotRangeStyle;

#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const CartesianPlotRangeStyle FIRST_CARTESIAN_PLOT_RANGE_STYLE = CHANNEL_RANGE;
#else
extern const CartesianPlotRangeStyle FIRST_CARTESIAN_PLOT_RANGE_STYLE;
#endif

#define NUM_ERASE_MODES 2
typedef enum {
    ERASE_IF_NOT_ZERO = 53,
    ERASE_IF_ZERO     = 54
} eraseMode_t;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const eraseMode_t FIRST_ERASE_MODE = ERASE_IF_NOT_ZERO;
#else
extern const eraseMode_t FIRST_ERASE_MODE;
#endif

#define NUM_RD_MODES 2
typedef enum {
    ADD_NEW_DISPLAY = 55,
    REPLACE_DISPLAY = 56
} relatedDisplayMode_t;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const relatedDisplayMode_t FIRST_RD_MODE = ADD_NEW_DISPLAY;
#else
extern const relatedDisplayMode_t FIRST_RD_MODE;
#endif

#define NUM_RD_VISUAL 4
typedef enum {
    RD_MENU       = 57,
    RD_ROW_OF_BTN = 58,
    RD_COL_OF_BTN = 59,
    RD_HIDDEN_BTN = 60
} relatedDisplayVisual_t;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const relatedDisplayVisual_t FIRST_RD_VISUAL = RD_MENU;
#else
extern const relatedDisplayVisual_t FIRST_RD_VISUAL;
#endif

#define NUM_CP_TIME_FORMAT 7
typedef enum {
    HHMMSS    = 61,
    HHMM      = 62,
    HH00      = 63,
    MMMDDYYYY = 64,
    MMMDD     = 65,
    MMDDHH00  = 66,
    WDHH00    = 67
} CartesianPlotTimeFormat_t;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const CartesianPlotTimeFormat_t FIRST_CP_TIME_FORMAT = HHMMSS;
#else
extern const CartesianPlotTimeFormat_t FIRST_CP_TIME_FORMAT;
#endif

#define NUM_BOOLEAN 2
typedef enum {
    BOOLEAN_FALSE = 68,
    BOOLEAN_TRUE  = 69
} Boolean_t;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const Boolean_t FIRST_BOOLEAN = BOOLEAN_FALSE;
#else
extern const Boolean_t FIRST_BOOLEAN;
#endif

#define NUM_PV_LIMITS_SRC 4
typedef enum {
    PV_LIMITS_CHANNEL = 70,
    PV_LIMITS_DEFAULT = 71,
    PV_LIMITS_USER    = 72,
    PV_LIMITS_UNUSED  = 73
} PvLimitsSrc_t;
#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const PvLimitsSrc_t FIRST_PV_LIMITS_SRC = PV_LIMITS_CHANNEL;
#else
extern const PvLimitsSrc_t FIRST_PV_LIMITS_SRC;
#endif

#ifdef __COLOR_RULE_H__
#define NUM_COLOR_RULE 4
typedef enum {
    COLOR_RULE_1 = 74,
    COLOR_RULE_2 = 75,
    COLOR_RULE_3 = 76,
    COLOR_RULE_4 = 77
} colorRuleMode_t;

#if defined(ALLOCATE_STORAGE) || defined(__cplusplus)
const colorRuleMode_t FIRST_COLOR_RULE = COLOR_RULE_1;
#else
extern const colorRuleMode_t FIRST_COLOR_RULE;
#endif
#endif

#define MAX_OPTIONS             8     /* NUM_TEXT_FORMATS */
#ifndef __COLOR_RULE_H__
#define NUMBER_STRING_VALUES    (74)  /* COLOR_RULE_1 */
#else
#define NUMBER_STRING_VALUES    (78)  /* COLOR_RULE_1 + NUM_COLOR_RULE */
#endif

/*********************************************************************
 * stringValueTable for string-valued tokens - position sensitive!   *
 * any changes of types or ordering of above must have matching      *
 * changes in this table!                                            *
 *********************************************************************/
#ifndef ALLOCATE_STORAGE
extern char *stringValueTable[NUMBER_STRING_VALUES];
extern XmString xmStringValueTable[NUMBER_STRING_VALUES];
#else
char *stringValueTable[NUMBER_STRING_VALUES] = { 
    "execute", "edit",
    "none", "no decorations", "outline", "limits", "channel",
    "static", "alarm", "discrete",
    "static", "if not zero", "if zero", "calc",
    "up", "right",
    "solid", "dash",
    "solid", "outline",
    "decimal", "exponential", "engr. notation", "compact", "truncated",
    "hexadecimal", "octal", "string",
    "horiz. left", "horiz. centered", "horiz. right",
    "column", "row", "row column",
    "from edge", "from center",
    "milli-second", "second", "minute",
    "point", "line", "fill-under",
    "plot n pts & stop", "plot last n pts",
    "no image", "gif", "tiff",
    "linear", "log10", "time",
    "from channel", "user-specified", "auto-scale",
    "if not zero", "if zero",
    "create new display", "replace display",
    "menu", "a row of buttons", "a column of buttons", "invisible",
    "hh:mm:ss", "hh:mm", "hh:00", "MMM DD YYYY", "MMM DD", "MMM DD hh:00",
    "wd hh:00",
    "false", "true",
    "channel", "default", "user", "unused",
#ifdef __COLOR_RULE_H__
    "set #1", "set #2", "set #3", "set #4",
#endif
};
XmString xmStringValueTable[NUMBER_STRING_VALUES];

#endif

/*********************************************************************
 *  controllers are also monitors (controllers are a sub-class of    *
 *  monitors) -> order must be consistent with all of the following: *
 * controllers:                                                      *
 *    DL_Valuator DL_ChoiceButton    DL_MessageButton DL_TextEntry   *
 *    DL_Menu,    DL_RelatedDisplay, DL_ShellCommand                 *
 *  monitors:                                                        *
 *    DL_Meter      DL_TextUpdate    DL_Bar        DL_Indicator      *
 *    DL_StripChart DL_CartesianPlot DL_SurfacePlot                  *
 *  statics acting as monitors (dynamics):                           *
 *    DL_Rectangle    DL_Oval       DL_Arc   DL_Text                 *
 *    DL_Polyline     DL_Polygon                                     *
 *********************************************************************/

typedef enum {
  /* self */
    DL_Element        =100,
  /* basics */
    DL_Composite      =101,
    DL_Display        =102,
  /* controllers */
    DL_ChoiceButton   =103,
    DL_Menu           =104,
    DL_MessageButton  =105,
    DL_RelatedDisplay =106,
    DL_ShellCommand   =107,
    DL_TextEntry      =108,
    DL_Valuator       =109,
  /* monitors */
    DL_Bar            =110,
    DL_Byte           =111,
    DL_CartesianPlot  =112,
    DL_Indicator      =113,
    DL_Meter          =114,
    DL_StripChart     =115,
    DL_TextUpdate     =116,
  /* statics */
    DL_Arc            =117,
    DL_Image          =118,
    DL_Line           =119,
    DL_Oval           =120,
    DL_Polygon        =121,
    DL_Polyline       =122,
    DL_Rectangle      =123,
    DL_Text           =124
} DlElementType;

#define MIN_DL_ELEMENT_TYPE DL_Element
#define MAX_DL_ELEMENT_TYPE DL_Text
#define NUM_DL_ELEMENT_TYPES    ((MAX_DL_ELEMENT_TYPE-MIN_DL_ELEMENT_TYPE)+1)
#define FIRST_RENDERABLE        DL_Composite

#define ELEMENT_IS_STATIC(type) ((type >= DL_Arc && type <= DL_Text))

#define ELEMENT_HAS_WIDGET(type) ((type >= DL_Display && type <= DL_StripChart))

#define ELEMENT_IS_CONTROLLER(type) ((type >= DL_choiceButton && type <= DL_Valuator))

/* this macro defines those elements which occupy space/position and can
 *  be rendered.  Note: Composite is not strictly renderable because no
 *  pixels are affected as a result of it's creation...
 *  DL_Display appears to be a sort of exception, since it's creation gives
 *  the backcloth upon which all rendering actually occurs.
 */
#define ELEMENT_IS_RENDERABLE(type) ((type >= FIRST_RENDERABLE) ? True : False)

/* Masks used for determining selected elements */
#define SmallestTouched 1    
#define AllTouched      2    
#define AllEnclosed     4    

/*******************
 * Nested structures
 *******************/
typedef struct {
    int clr;
    EdgeStyle style;
    FillStyle fill;
    unsigned int width;
} DlBasicAttribute;

typedef struct {
    ColorMode clr;
    VisibilityMode vis;
#ifdef __COLOR_RULE_H__
    int colorRule;
#endif
    char chan[MAX_CALC_RECORDS][MAX_TOKEN_LENGTH];
    char calc[MAX_TOKEN_LENGTH];
    char post[MAX_TOKEN_LENGTH];
    Boolean validCalc;
} DlDynamicAttribute;
        
typedef struct {
    int x, y;
    unsigned int width, height;
} DlObject;

typedef struct {
    char rdbk[MAX_TOKEN_LENGTH];
    int clr, bclr;
} DlMonitor;

typedef struct {
    char ctrl[MAX_TOKEN_LENGTH];
    int clr, bclr;
} DlControl;

typedef struct {
    int loprSrc;
    int loprSrc0;
    double lopr;
    double loprChannel;
    double loprDefault;
    double loprUser;
    int hoprSrc;
    int hoprSrc0;
    double hopr;
    double hoprChannel;
    double hoprDefault;
    double hoprUser;
    int precSrc;
    int precSrc0;
    short prec;
    short precChannel;
    short precDefault;
    short precUser;
} DlLimits;

typedef struct {
    char title[MAX_TOKEN_LENGTH];
    char xlabel[MAX_TOKEN_LENGTH];
    char ylabel[MAX_TOKEN_LENGTH];
    int clr, bclr;
    char package[MAX_TOKEN_LENGTH];
} DlPlotcom;

typedef struct {
    CartesianPlotAxisStyle axisStyle;
    CartesianPlotRangeStyle rangeStyle;
    float minRange, maxRange;
    CartesianPlotTimeFormat_t timeFormat;
} DlPlotAxisDefinition;

typedef struct {
    char label[MAX_TOKEN_LENGTH];
    char name[MAX_TOKEN_LENGTH];
    char args[MAX_TOKEN_LENGTH];
    relatedDisplayMode_t mode;
} DlRelatedDisplayEntry;

typedef struct {
    char label[MAX_TOKEN_LENGTH];
    char command[MAX_TOKEN_LENGTH];
    char args[MAX_TOKEN_LENGTH];
} DlShellCommandEntry;

typedef struct {
    int r, g, b;
    int inten;
} DlColormapEntry;

typedef struct {
    char chan[MAX_TOKEN_LENGTH];
    int clr;
} DlPen;

typedef struct {
    char xdata[MAX_TOKEN_LENGTH];
    char ydata[MAX_TOKEN_LENGTH];
    int data_clr;
} DlTrace;

typedef struct {
    int gridSpacing;
    Boolean gridOn;
    Boolean snapToGrid;
} DlGrid;

/*********************************************************************
 * Top Level structures                                              *
 *********************************************************************/

typedef struct {
    char name[MAX_TOKEN_LENGTH];
    int versionNumber;
} DlFile;

typedef struct {
    DlObject object;
    int clr, bclr;
    char cmap[MAX_TOKEN_LENGTH];
    DlGrid grid;
} DlDisplay;

typedef struct {
    int ncolors;
    DlColormapEntry dl_color[DL_MAX_COLORS];
} DlColormap;

/****** Shapes */

typedef struct {
    DlObject object;
    DlBasicAttribute attr;
    DlDynamicAttribute dynAttr;
} DlRectangle;

typedef struct {
    DlObject object;
    DlBasicAttribute attr;
    DlDynamicAttribute dynAttr;
} DlOval;

typedef struct {
    DlObject object;
    DlBasicAttribute attr;
    DlDynamicAttribute dynAttr;
    int begin;
    int path;
} DlArc;

typedef struct {
    DlObject object;
    DlBasicAttribute attr;
    DlDynamicAttribute dynAttr;
    char textix[MAX_TOKEN_LENGTH];
    TextAlign align;
} DlText;

typedef struct {
    DlObject object;
    DlRelatedDisplayEntry display[MAX_RELATED_DISPLAYS];
    int clr, bclr;
    char label[MAX_TOKEN_LENGTH];
    relatedDisplayVisual_t visual;
} DlRelatedDisplay;

typedef struct {
    DlObject object;
    DlShellCommandEntry command[MAX_SHELL_COMMANDS];
    int clr, bclr;
} DlShellCommand;

/****** Monitors */

typedef struct {
    DlObject object;
    DlMonitor monitor;
    DlLimits limits;
    ColorMode clrmod;
    TextAlign align;
    TextFormat format;
} DlTextUpdate;

typedef struct {
    DlObject object;
    DlMonitor monitor;
    DlLimits limits;
    LabelType label;
    ColorMode clrmod;
    Direction direction;
} DlIndicator;

typedef struct {
    DlObject object;
    DlMonitor monitor;
    DlLimits limits;
    LabelType label;
    ColorMode clrmod;
} DlMeter;

typedef struct {
    DlObject object;
    DlMonitor monitor;
    DlLimits limits;
    LabelType label;
    ColorMode clrmod;
    Direction direction;
    FillMode fillmod;
} DlBar;

typedef struct {
    DlObject object;
    DlMonitor monitor;
    ColorMode clrmod;
    Direction direction;
    int sbit, ebit;
} DlByte;

typedef struct {
    DlObject object;
    DlPlotcom plotcom;
    double period;
    TimeUnits units;
#if 1
    double delay;           /* the delay and oldUnits are for compatible reason */
    TimeUnits oldUnits;     /* they will be removed for future release */
#endif
    DlPen pen[MAX_PENS];
} DlStripChart;

typedef struct {
    DlObject object;
    DlPlotcom plotcom;
    CartesianPlotStyle style;
    EraseOldest erase_oldest;
    int count;
    DlTrace trace[MAX_TRACES];
    DlPlotAxisDefinition axis[3];     /* x = 0, y = 1, y2 = 2 */
    char trigger[MAX_TOKEN_LENGTH];
    char erase[MAX_TOKEN_LENGTH];
    eraseMode_t eraseMode;
} DlCartesianPlot;

#define X_AXIS_ELEMENT  0
#define Y1_AXIS_ELEMENT 1
#define Y2_AXIS_ELEMENT 2

/****** Controllers */

typedef struct {
    DlObject object;
    DlControl control;
    DlLimits limits;
    LabelType label;
    ColorMode clrmod;
    Direction direction;
    double dPrecision;
  /* Private (run-time) data valuator needs for its operation */
    Boolean enableUpdates;
    Boolean dragging;
} DlValuator;     /* Slider */

typedef struct {
    DlObject object;
    DlControl control;
    ColorMode clrmod;
    Stacking stacking;
} DlChoiceButton;

typedef struct {
    DlObject object;
    DlControl control;
    char label[MAX_TOKEN_LENGTH];
    char press_msg[MAX_TOKEN_LENGTH];
    char release_msg[MAX_TOKEN_LENGTH];
    ColorMode clrmod;
} DlMessageButton;

typedef struct {
    DlObject object;
    DlControl control;
    ColorMode clrmod;
} DlMenu;

typedef struct {
    DlObject object;
    DlControl control;
    DlLimits limits;
    ColorMode clrmod;
    TextFormat format;
} DlTextEntry;

/****** Extensions */
typedef struct {
    DlObject object;
    DlDynamicAttribute dynAttr;
    ImageType imageType;
    char calc[MAX_TOKEN_LENGTH];
    char imageName[MAX_TOKEN_LENGTH];
    XtPointer privateData;
} DlImage;

struct  _DlElement;
struct  _DlList;
typedef struct _DlComposite {
    DlObject object;
    DlDynamicAttribute dynAttr;
    char compositeName[MAX_TOKEN_LENGTH];
    struct _DlList *dlElementList;
} DlComposite;

/* (if MEDM ever leaves the X environment, a DlPoint should be defined and
 * substituted here for XPoint...) */

typedef struct {
    DlObject object;
    DlBasicAttribute attr;
    DlDynamicAttribute dynAttr;
    XPoint *points;
    int nPoints;
    int isFallingOrRisingLine;
} DlPolyline;

typedef struct {
    DlObject object;
    DlBasicAttribute attr;
    DlDynamicAttribute dynAttr;
    XPoint *points;
    int nPoints;
} DlPolygon;

/*** NOTE:  DlObject must be first entry in each RENDERABLE structure!!!
   display list in memory (with notion of composite/hierarchical structures) */

typedef union {
  /* Note: DlStructurePtr depends on DlElement depends on DlStructurePtr
   *   so can't use DLElement here  */
    struct _DlElement *element;
    DlDisplay *display;
    DlRectangle *rectangle;
    DlOval *oval;
    DlArc *arc;
    DlText *text;
    DlRelatedDisplay *relatedDisplay;
    DlShellCommand *shellCommand;
    DlTextUpdate *textUpdate;
    DlIndicator *indicator;
    DlMeter *meter;
    DlBar *bar;
    DlByte *byte;
    DlStripChart *stripChart;
    DlCartesianPlot *cartesianPlot;
    DlValuator *valuator;
    DlChoiceButton *choiceButton;
    DlMessageButton *messageButton;
    DlMenu *menu;
    DlTextEntry *textEntry;
    DlImage *image;
    DlComposite *composite;
    DlPolyline *polyline;
    DlPolygon *polygon;
} DlStructurePtr;

struct _ResourceBundle;
struct _DisplayInfo;

typedef struct {
  /* Create (Allocate structures) method */
    struct _DlElement *(*create)();     /* note: 3 args in createDlElement, 1 in createDlxxx */
  /* Destroy (Free structures) method */
    void (*destroy)(struct _DisplayInfo *, struct _DlElement *);
  /* Execute (Make it appear on the display) method */
    void (*execute)(struct _DisplayInfo *, struct _DlElement *);
  /* Hide (Make it not appear on the display) method */
    void (*hide)(struct _DisplayInfo *, struct _DlElement *);
  /* Write (to file) method */
    void (*write)(FILE *, struct _DlElement *, int);
  /* Get limits (from DlLimits attribute) method */
    void (*getLimits)(struct _DlElement *, DlLimits **, char **);
  /* Get values (from the resource bundle) method */
    void (*getValues)(struct _ResourceBundle *, struct _DlElement *); 
  /* Inherit ((some of the) values from the resource bundle) method
   *   Used during rectangular creates */
    void (*inheritValues)(struct _ResourceBundle *, struct _DlElement *); 
    void (*setBackgroundColor)(struct _ResourceBundle *, struct _DlElement *);
    void (*setForegroundColor)(struct _ResourceBundle *, struct _DlElement *);
    void (*move)(struct _DlElement *, int, int);
    void (*scale)(struct _DlElement *, int, int);
    void (*orient)(struct _DlElement *, int, int, int);
    int  (*editVertex)(struct _DlElement *, int, int);
  /* Cleanup method (only exists for Composite and sets widgets to NULL) */
    void (*cleanup)(struct _DlElement *);
} DlDispatchTable; 

typedef struct _DlElement {
    DlElementType type;
    DlStructurePtr structure;
    DlDispatchTable *run;
    Widget widget;
    void * data;
    struct _DlElement *next;       /* next element in display list   */
    struct _DlElement *prev;       /* previous element ...           */
} DlElement;

typedef struct _DlList {
    DlElement *head;
    DlElement *tail;
    long      count;
    DlElement data;
} DlList;

#define FirstDlElement(x) (x->head->next)
#define SecondDlElement(x) (x->head->next?x->head->next->next:NULL)
#define LastDlElement(x) (x->tail)
#define IsEmpty(x) (x->count <= 0)
#define NumberOfDlElement(x) (x->count)
#endif
