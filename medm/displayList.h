
/*
 * display list structure definition 
 */
#ifndef __DISPLAYLIST_H__
#define __DISPLAYLIST_H__


#include <stdio.h>
#include <sys/types.h>
#include <math.h>
#include <strings.h>

#define MAX_TOKEN_LENGTH	256	/* max size of strings in adl  */
#define MAX_RELATED_DISPLAYS	8	/* max # of related displays   */
#define MAX_SHELL_COMMANDS 	8	/* max # of shell commands     */
#define MAX_PENS		8	/* max # of pens on strip chart*/
#define MAX_TRACES		8	/* max # of traces on cart.plot*/
#define MAX_FILE_CHARS		256	/* max # of chars in filename  */
#define DL_MAX_COLORS		65	/* max # of colors for display */
#define DL_COLORS_COLUMN_SIZE	5	/* # of colors in each column  */


/*
 * new types
 */

typedef enum {
  DL_EXECUTE	= 0,
  DL_EDIT	= 1
} DlTraversalMode;
#ifdef ALLOCATE_STORAGE
const DlTraversalMode FIRST_TRAVERSAL_MODE = DL_EXECUTE;
#else
extern const DlTraversalMode FIRST_TRAVERSAL_MODE;
#endif
/* DL_EXECUTE, DL_EDIT */
#define NUM_TRAVERSAL_MODES	2

typedef enum {
  LABEL_NONE 	= 2,
  OUTLINE	= 3,
  LIMITS	= 4,
  CHANNEL	= 5
} LabelType;
#ifdef ALLOCATE_STORAGE
const LabelType FIRST_LABEL_TYPE = LABEL_NONE;
#else
extern const LabelType FIRST_LABEL_TYPE;
#endif
/* LABEL_NONE, OUTLINE, LIMITS, CHANNEL */
#define NUM_LABEL_TYPES		4

typedef enum {
  STATIC	= 6,
  ALARM		= 7,
  DISCRETE	= 8
} ColorMode;
#ifdef ALLOCATE_STORAGE
const ColorMode FIRST_COLOR_MODE = STATIC;
#else
extern const ColorMode FIRST_COLOR_MODE;
#endif
/* STATIC, ALARM, DISCRETE */
#define NUM_COLOR_MODES		3

typedef enum {
  V_STATIC	= 9,
  IF_NOT_ZERO	= 10,
  IF_ZERO	= 11
} VisibilityMode;
#ifdef ALLOCATE_STORAGE
const VisibilityMode FIRST_VISIBILITY_MODE = V_STATIC;
#else
extern const VisibilityMode FIRST_VISIBILITY_MODE;
#endif
/* STATIC, IF_NOT_ZERO, IF_ZERO	*/
#define NUM_VISIBILITY_MODES	3

typedef enum {
  UP		= 12,
  RIGHT		= 13,
  DOWN		= 14,
  LEFT		= 15
} Direction;
#ifdef ALLOCATE_STORAGE
const Direction FIRST_DIRECTION = UP;
#else
extern const Direction FIRST_DIRECTION;
#endif
/* UP, RIGHT, (maybe DOWN, LEFT later) */
#define NUM_DIRECTIONS		2

typedef enum {
  SOLID		= 16,
  DASH		= 17
} EdgeStyle;
#ifdef ALLOCATE_STORAGE
const EdgeStyle FIRST_EDGE_STYLE = SOLID;
#else
extern const EdgeStyle FIRST_EDGE_STYLE;
#endif
/* SOLID, DASH */
#define NUM_EDGE_STYLES		2

typedef enum {
  F_SOLID	= 18,
  F_OUTLINE	= 19
} FillStyle;
#ifdef ALLOCATE_STORAGE
const FillStyle FIRST_FILL_STYLE = F_SOLID;
#else
extern const FillStyle FIRST_FILL_STYLE;
#endif
/* SOLID, OUTLINE */
#define NUM_FILL_STYLES		2


typedef enum {
  DECIMAL	= 20,
  EXPONENTIAL	= 21,
  ENGR_NOTATION = 22,
  COMPACT	= 23,
  TRUNCATED	= 24,
  HEXADECIMAL	= 25,
  OCTAL 	= 26
} TextFormat;
#ifdef ALLOCATE_STORAGE
const TextFormat FIRST_TEXT_FORMAT = DECIMAL;
#else
extern const TextFormat FIRST_TEXT_FORMAT;
#endif
/* DECIMAL, EXPONENTIAL, ENGR_NOTATION, COMPACT, TRUNCATED, HEXADECIMAL, OCTAL*/
#define NUM_TEXT_FORMATS	7

typedef enum {
  HORIZ_LEFT	= 27,
  HORIZ_CENTER	= 28,
  HORIZ_RIGHT	= 29,
  VERT_TOP	= 30,
  VERT_BOTTOM	= 31,
  VERT_CENTER	= 32
} TextAlign;
#ifdef ALLOCATE_STORAGE
const TextAlign FIRST_TEXT_ALIGN = HORIZ_LEFT;
#else
extern const TextAlign FIRST_TEXT_ALIGN;
#endif
/* HORIZ_LEFT, HORIZ_CENTER, HORIZ_RIGHT, VERT_TOP, VERT_BOTTOM, VERT_CENTER */
#define NUM_TEXT_ALIGNS		6

typedef enum {
  COLUMN	= 33,
  ROW		= 34,
  ROW_COLUMN 	= 35
} Stacking;
#ifdef ALLOCATE_STORAGE
const Stacking FIRST_STACKING = COLUMN;
#else
extern const Stacking FIRST_STACKING;
#endif
/* COLUMN, ROW, ROW_COLUMN */
#define NUM_STACKINGS		3

typedef enum {
  FROM_EDGE	= 36,
} FillMode;
#ifdef ALLOCATE_STORAGE
const FillMode FIRST_FILL_MODE = FROM_EDGE;
#else
extern const FillMode FIRST_FILL_MODE;
#endif
/* FROM_EDGE */
#define NUM_FILL_MODES		1

typedef enum {
  MILLISECONDS	= 37,
  SECONDS	= 38,
  MINUTES	= 39
} TimeUnits;
#ifdef ALLOCATE_STORAGE
const TimeUnits FIRST_TIME_UNIT = MILLISECONDS;
#else
extern const TimeUnits FIRST_TIME_UNIT;
#endif
/* MILLISECONDS,SECONDS,MINUTES */
#define NUM_TIME_UNITS		3

typedef enum {
  POINT_PLOT		= 40,
  LINE_PLOT		= 41,
  FILL_UNDER_PLOT	= 42
} CartesianPlotStyle;
#ifdef ALLOCATE_STORAGE
const CartesianPlotStyle FIRST_CARTESIAN_PLOT_STYLE = POINT_PLOT;
#else
extern const CartesianPlotStyle FIRST_CARTESIAN_PLOT_STYLE;
#endif
/* POINT_PLOT,LINE_PLOT,FILL_UNDER_PLOT */
#define NUM_CARTESIAN_PLOT_STYLES	3

typedef enum {
  ERASE_OLDEST_OFF	= 43,
  ERASE_OLDEST_ON	= 44
} EraseOldest;
#ifdef ALLOCATE_STORAGE
const EraseOldest FIRST_ERASE_OLDEST = ERASE_OLDEST_OFF;
#else
extern const EraseOldest FIRST_ERASE_OLDEST;
#endif
/* ERASE_OLDEST_OFF, ERASE_OLDEST_ON */
#define NUM_ERASE_OLDESTS	2

typedef enum {
  NO_IMAGE		= 45,
  GIF_IMAGE		= 46,
  TIFF_IMAGE		= 47
} ImageType;
#ifdef ALLOCATE_STORAGE
const ImageType FIRST_IMAGE_TYPE = NO_IMAGE;
#else
extern const ImageType FIRST_IMAGE_TYPE;
#endif
/* NO_IMAGE, GIF_IMAGE, TIFF_IMAGE */
#define	NUM_IMAGE_TYPES		3

typedef enum {
  LINEAR_AXIS		= 48,
  LOG10_AXIS		= 49
} CartesianPlotAxisStyle;
#ifdef ALLOCATE_STORAGE
const CartesianPlotAxisStyle FIRST_CARTESIAN_PLOT_AXIS_STYLE = LINEAR_AXIS;
#else
extern const CartesianPlotAxisStyle FIRST_CARTESIAN_PLOT_AXIS_STYLE;
#endif
/* LINEAR_AXIS, LOG10_AXIS */
#define NUM_CARTESIAN_PLOT_AXIS_STYLES	2

typedef enum {
  CHANNEL_RANGE		= 50,
  USER_SPECIFIED_RANGE	= 51,
  AUTO_SCALE_RANGE	= 52
} CartesianPlotRangeStyle;
#ifdef ALLOCATE_STORAGE
const CartesianPlotRangeStyle FIRST_CARTESIAN_PLOT_RANGE_STYLE
		= CHANNEL_RANGE;
#else
extern const CartesianPlotRangeStyle FIRST_CARTESIAN_PLOT_RANGE_STYLE;
#endif
/* CHANNEL_RANGE, USER_SPECIFIED_RANGE, AUTO_SCALE_RANGE*/
#define NUM_CARTESIAN_PLOT_RANGE_STYLES	3

#define MAX_OPTIONS		7	/* NUM_TEXT_FORMATS	*/
#define NUMBER_STRING_VALUES	(52+1)	/* AUTO_SCALE_RANGE + 1	*/

/***
 *** stringValueTable for string-valued tokens - position sensitive!!
 ***	any changes of types or ordering of above must have corresponding
 ***	changes in this table!!
 ***/
#ifndef ALLOCATE_STORAGE
 extern char *stringValueTable[NUMBER_STRING_VALUES];
 extern XmString xmStringValueTable[NUMBER_STRING_VALUES];
#else
 char *stringValueTable[NUMBER_STRING_VALUES] = { 
    "execute", "edit",
    "none","outline","limits","channel",
    "static","alarm","discrete",
    "static", "if not zero", "if zero",
    "up","right", "down","left",
    "solid","dash",
    "solid","outline",
    "decimal","exponential","engr. notation","compact","truncated",
				"hexadecimal","octal",
    "horiz. left","horiz. centered","horiz. right",
				"vert. top","vert. bottom","vert. centered",
    "column","row","row column",
    "from edge",
    "milli-second","second","minute",
    "point","line","fill-under",
    "off","on",
    "no image","gif","tiff",
    "linear","log10",
    "from channel", "user-specified", "auto-scale",
  };
 XmString xmStringValueTable[NUMBER_STRING_VALUES];

#endif




/*
 *  controllers are also monitors (controllers are a sub-class of monitors)
 *    -> order must be consistent between all of the following:
 */
typedef int DlControllerType;
/* DL_Valuator, DL_ChoiceButton, DL_MessageButton, DL_TextEntry,
	DL_Menu, DL_ShellCommand */

typedef int DlMonitorType;
/* controllers:
	DL_Valuator, DL_ChoiceButton, DL_MessageButton, DL_TextEntry,
	DL_Menu, DL_RelatedDisplay, DL_ShellCommand,
   monitors:
	DL_Meter, DL_TextUpdate, DL_Bar, DL_Indicator, 
	DL_StripChart, DL_CartesianPlot, DL_SurfacePlot,
   statics acting as monitors (dynamics):
	DL_Rectangle, DL_Oval, DL_Arc, DL_Text,
	DL_FallingLine, DL_RisingLine
*/

typedef int DlElementType;
/* controllers:
	DL_Valuator, DL_ChoiceButton, DL_MessageButton, DL_TextEntry,
	DL_Menu, DL_RelatedDisplay, DL_ShellCommand,
   monitors:
	DL_Meter, DL_TextUpdate, DL_Bar, DL_Indicator,
	DL_StripChart, DL_CartesianPlot, DL_SurfacePlot,
   statics acting as monitors (dynamics):
	DL_Rectangle, DL_Oval, DL_Arc, DL_Text,
	DL_FallingLine, DL_RisingLine,
   and all the other element types:
	DL_File, DL_Display, DL_Colormap, DL_BasicAttribute,DL_DynamicAttribute,
   and the new (extensions) object types:
	DL_Image, DL_Composite, DL_Line, DL_Polyline,DL_Polygon, DL_BezierCurve
*/

/* NON-RENDERABLE */
#define DL_File			94
#define DL_Colormap		95
#define DL_BasicAttribute	96
#define DL_DynamicAttribute	97

/* QUASI-RENDERABLE (has object attributes) */
#define DL_Composite		98
#define DL_Display		99

/* CONTROLLERS */
#define DL_Valuator		100
#define DL_ChoiceButton		101
#define DL_MessageButton	102
#define DL_TextEntry		103
#define DL_Menu			104
#define DL_RelatedDisplay	105
#define DL_ShellCommand		106
/* MONITORS */
#define DL_Meter		107
#define DL_TextUpdate		108
#define DL_Bar			109
#define DL_Indicator		110
#define DL_StripChart		111
#define DL_CartesianPlot	112
#define DL_SurfacePlot		113
/* STATICS */
#define DL_Rectangle		114
#define DL_Oval			115
#define DL_Arc			116
#define DL_Text			117
#define DL_FallingLine		118
#define DL_RisingLine		119

/* NEW ONES */
#define DL_Image		120
#define DL_Line			121

#define DL_Polyline		122
#define DL_Polygon		123
#define DL_BezierCurve		124

#define MIN_DL_ELEMENT_TYPE	DL_File
#define MAX_DL_ELEMENT_TYPE	DL_BezierCurve
#define NUM_DL_ELEMENT_TYPES	((MAX_DL_ELEMENT_TYPE-MIN_DL_ELEMENT_TYPE)+1)
#define FIRST_RENDERABLE	DL_Composite

#define ELEMENT_IS_STATIC(type) \
	( (type >= DL_Rectangle) )

#define ELEMENT_HAS_WIDGET(type) \
	(((type >= DL_Display && type <= DL_SurfacePlot \
		&& type != DL_TextUpdate)) ? True : False)

#define ELEMENT_IS_CONTROLLER(type) \
	((type >= DL_Valuator && type <= DL_ShellCommand) ? True : False)

/* this macro defines those elements which occupy space/position and can
 *  be rendered.  Note: Composite is not strictly renderable because no
 *  pixels are affected as a result of it's creation...
 *  DL_Display appears to be a sort of exception, since it's creation gives
 *  the backcloth upon which all rendering actually occurs.
 */
#define ELEMENT_IS_RENDERABLE(type) \
	((type >= FIRST_RENDERABLE) ? True : False)



/***************************************************************************
 *****************               nested structures                   *******
 ***************************************************************************/

/* 
 * attr
 */
typedef struct {
	int clr;
	FillStyle style, fill;
	unsigned int width;
} DlAttribute;


/* 
 * dynamic attr
 */
typedef struct {
	ColorMode clr;
	VisibilityMode vis;
} DlDynamicAttrMod;

typedef struct {
	char chan[MAX_TOKEN_LENGTH];
} DlDynamicAttrParam;

typedef struct {
	DlDynamicAttrMod mod;		/*   in their proper scope (probably  */
	DlDynamicAttrParam param;	/*   only apply to next object...)    */
} DlDynamicAttributeData;
	


/*
 * object
 */
typedef struct {
	int x, y;
	unsigned int width, height;
} DlObject;


/*
 * monitor
 */
typedef struct {
	char rdbk[MAX_TOKEN_LENGTH];
	int clr, bclr;
} DlMonitor;


/*
 * control
 */
typedef struct {
	char ctrl[MAX_TOKEN_LENGTH];
	int clr, bclr;
} DlControl;


/*
 * plotcom
 */
typedef struct {
	char title[MAX_TOKEN_LENGTH];
	char xlabel[MAX_TOKEN_LENGTH];
	char ylabel[MAX_TOKEN_LENGTH];
	int clr, bclr;
	char package[MAX_TOKEN_LENGTH];
} DlPlotcom;

/*
 * strip/cartesian plot axes definitions
 */
typedef struct {
	CartesianPlotAxisStyle axisStyle;
	CartesianPlotRangeStyle rangeStyle;
	float minRange, maxRange;
} DlPlotAxisDefinition;

/*
 * related display data (display[i])
 */
typedef struct {
	char label[MAX_TOKEN_LENGTH];
	char name[MAX_TOKEN_LENGTH];
	char args[MAX_TOKEN_LENGTH];
} DlRelatedDisplayEntry;

/*
 * shell command data (command[i])
 */
typedef struct {
	char label[MAX_TOKEN_LENGTH];
	char command[MAX_TOKEN_LENGTH];
	char args[MAX_TOKEN_LENGTH];
} DlShellCommandEntry;


/*
 * dl_color
 */
typedef struct {
	int r, g, b;
	int inten;
} DlColormapEntry;


/*
 * pen
 */
typedef struct {
	char chan[MAX_TOKEN_LENGTH];
	int clr;
} DlPen;


/*
 * trace
 */
typedef struct {
	char xdata[MAX_TOKEN_LENGTH];
	char ydata[MAX_TOKEN_LENGTH];
	int data_clr;
} DlTrace;


/***************************************************************************
 *****************            top level structures                   *******
 ***************************************************************************/




/* * * * *
 * * * * *   Static Objects
 * * * * */


/*
 * file
 */
typedef struct {
	char name[MAX_TOKEN_LENGTH];
} DlFile;


/*
 * display
 */
typedef struct {
	DlObject object;
	int clr, bclr;
	char cmap[MAX_TOKEN_LENGTH];
} DlDisplay;


/*
 * <<color map>>
 */
typedef struct {
	int ncolors;
	DlColormapEntry dl_color[DL_MAX_COLORS];
} DlColormap;


/*
 * <<basic attribute>>
 */
typedef struct {
	DlAttribute attr;
} DlBasicAttribute;


/*
 * <<dynamic attribute>>
 */
typedef struct {
	DlDynamicAttributeData attr;
} DlDynamicAttribute;



/*
 * rectangle
 */
typedef struct {
	DlObject object;
} DlRectangle;


/*
 * oval
 */
typedef struct {
	DlObject object;
} DlOval;



/*
 * arc
 */
typedef struct {
	DlObject object;
	int begin;
	int path;
} DlArc;



/*
 * falling line
 */
typedef struct {
	DlObject object;
} DlFallingLine;



/*
 * rising line
 */
typedef struct {
	DlObject object;
} DlRisingLine;



/*
 * text
 */
typedef struct {
	DlObject object;
	char textix[MAX_TOKEN_LENGTH];
	TextAlign align;
} DlText;


/*
 * related display
 */
typedef struct {
	DlObject object;
	DlRelatedDisplayEntry display[MAX_RELATED_DISPLAYS];
	int clr, bclr;
} DlRelatedDisplay;


/*
 * shell command
 */
typedef struct {
	DlObject object;
	DlShellCommandEntry command[MAX_SHELL_COMMANDS];
	int clr, bclr;
} DlShellCommand;



/* * * * *
 * * * * *   Monitor Objects
 * * * * */

/*
 * text update
 */
typedef struct {
	DlObject object;
	DlMonitor monitor;
	ColorMode clrmod;
	TextAlign align;
	TextFormat format;
} DlTextUpdate;



/*
 * indicator
 */
typedef struct {
	DlObject object;
	DlMonitor monitor;
	LabelType label;
	ColorMode clrmod;
	Direction direction;
} DlIndicator;


/*
 * meter
 */
typedef struct {
	DlObject object;
	DlMonitor monitor;
	LabelType label;
	ColorMode clrmod;
} DlMeter;


/*
 * bar
 */
typedef struct {
	DlObject object;
	DlMonitor monitor;
	LabelType label;
	ColorMode clrmod;
	Direction direction;
	FillMode fillmod;
} DlBar;



/*
 * surface plot
 */
typedef struct {
	DlObject object;
	DlPlotcom plotcom;
	char data[MAX_TOKEN_LENGTH];
	int data_clr;
	int dis;
	int xyangle;
	int zangle;
} DlSurfacePlot;


/*
 * strip chart
 */
typedef struct {
	DlObject object;
	DlPlotcom plotcom;
	int delay;
	TimeUnits units;
	DlPen pen[MAX_PENS];
} DlStripChart;


/*
 * cartesian plot
 */
typedef struct {
	DlObject object;
	DlPlotcom plotcom;
	CartesianPlotStyle style;
	EraseOldest erase_oldest;
	int count;
	DlTrace trace[MAX_TRACES];
	DlPlotAxisDefinition axis[3];	/* x = [0], y1 = [1], y2 = [2] */
	char trigger[MAX_TOKEN_LENGTH];
} DlCartesianPlot;

#define X_AXIS_ELEMENT	0
#define Y1_AXIS_ELEMENT	1
#define Y2_AXIS_ELEMENT	2

/* * * * *
 * * * * *   Controller Objects
 * * * * */


/*
 * valuator
 */
typedef struct {
	DlObject object;
	DlControl control;
	LabelType label;
	ColorMode clrmod;
	Direction direction;
	double dPrecision;
     /* private (run-time) data valuator needs for its operation */
	Boolean enableUpdates;
	Boolean dragging;
} DlValuator;



/*
 * choice button
 */
typedef struct {
	DlObject object;
	DlControl control;
	ColorMode clrmod;
	Stacking stacking;
} DlChoiceButton;



/*
 * message button
 */
typedef struct {
	DlObject object;
	DlControl control;
	char label[MAX_TOKEN_LENGTH];
	char press_msg[MAX_TOKEN_LENGTH];
	char release_msg[MAX_TOKEN_LENGTH];
	ColorMode clrmod;
} DlMessageButton;



/*
 * menu
 */
typedef struct {
	DlObject object;
	DlControl control;
	ColorMode clrmod;
} DlMenu;



/*
 * text entry
 */
typedef struct {
	DlObject object;
	DlControl control;
	ColorMode clrmod;
	TextFormat format;
} DlTextEntry;



/* * * * *
 * * * * *   Extension Objects
 * * * * */


/*
 * image
 */
typedef struct {
	DlObject object;
	ImageType imageType;
	char imageName[MAX_TOKEN_LENGTH];
     /* private (run-time) data image needs for its operation */
	XtPointer privateData;
} DlImage;


/*
 * composite
 */
typedef struct _DlComposite {
	DlObject object;
	char compositeName[MAX_TOKEN_LENGTH];
	VisibilityMode vis;
	char chan[MAX_TOKEN_LENGTH];
     /* private (run-time) data composite needs for its operation      */
	XtPointer dlElementListHead;		/*  (DlElement *)      */
	XtPointer dlElementListTail;		/*  (DlElement *)      */
	Boolean visible;			/* run-time visibility */
	Boolean monitorAlreadyAdded;		/* for monitor status  */
} DlComposite;


/* (if MEDM ever leaves the X environment, a DlPoint should be defined and
 * substituted here for XPoint...) */

/*
 * polyline
 */
typedef struct {
	DlObject object;
	XPoint *points;		/* array of XPoint-s for efficient rendering */
      /* private (run-time) data */
	int nPoints;
} DlPolyline;


/*
 * polygon
 */
typedef struct {
	DlObject object;
	XPoint *points;		/* array of XPoint-s for efficient rendering */
      /* private (run-time) data */
	int nPoints;
} DlPolygon;



/***
 *** NOTE:  DlObject must be first entry in each RENDERABLE structure!!!
 ***/




/* display list in memory (with notion of composite/hierarchical structures) */


typedef union {
	DlFile *file;
	DlDisplay *display;
	DlColormap *colormap;
	DlBasicAttribute *basicAttribute;
	DlDynamicAttribute *dynamicAttribute;
	DlRectangle *rectangle;
	DlOval *oval;
	DlArc *arc;
	DlFallingLine *fallingLine;
	DlRisingLine *risingLine;
	DlText *text;
	DlRelatedDisplay *relatedDisplay;
	DlShellCommand *shellCommand;
	DlTextUpdate *textUpdate;
	DlIndicator *indicator;
	DlMeter *meter;
	DlBar *bar;
	DlSurfacePlot *surfacePlot;
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


typedef struct _DlElement {
	DlElementType type;
	DlStructurePtr structure;
	void (*dmExecute)();		   /* execute thyself method          */
	void (*dmWrite)();		   /* write thyself (to file) method  */
	struct _DlElement *next;	   /* next element in display list    */
	struct _DlElement *prev;	   /* previous element ...            */
} DlElement;



#endif  /* __DISPLAYLIST_H__ */
