/*************************************************************************\
* Copyright (c) 2004 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2004 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution. 
\*************************************************************************/
/* Based on                          */
/* WheelSwitch widget class          */
/* F. Di Maio - CERN 1990            */
/* Modified by Jean-Michel Nonglaton */
/* Mofified extensively for MEDM     */

#define DEBUG_ASSERT 0
#define DEBUG_EXPOSE 0
#define DEBUG_SETVALUE 0
#define DEBUG_DESTRUCTOR 0
#define DEBUG_CONVERTER 0
#define DEBUG_FONTS 0
#define DEBUG_INITIALIZE 0
#define DEBUG_FORMAT 0
#define DEBUG_ERRORHANDLER 0
#define DEBUG_FONTLEAK 0
#define DEBUG_KEYSYM 0
#define DEBUG_POINTER 0
#define DEBUG_FLOW 0

#define MAX_FONTS 100
#define FORMAT_SIZE 255

/* Format defaults, must be consistent */
#define DEFAULT_FORMAT "% 6.2f"
#define DEFAULT_FORMAT_WIDTH 6
#define DEFAULT_FORMAT_PRECISION 2
#define DEFAULT_CHAR_WIDTH 8;
#define DEFAULT_CHAR_HEIGHT 16;

/* Use a complete name so another routine is unlikely to add a
 * converter  for the same things */
#define XmRDoublePointer "WheelSwitchDoublePointer"

/* Set this if you want to have memory allocated by
 * CvtStringToDoublePointer be freed when the widget is freed.  In
 * addition, each widget needs to set XmNInitialResourcesPersistent to
 * False for this to happen.  Note that the converter is only used
 * when string defaults for XmNcurrentValue, XmNminValue, or
 * XmNmaxValue are used, as in a resource specification or the
 * from the defaults in this widget. */
#define USE_CACHE_REF_COUNT 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <Xm/Xm.h>
#include <Xm/XmStrDefs.h>
#include <Xm/GadgetP.h>
#include <Xm/DrawP.h>  /* For XmeDrawShadows */
#include <Xm/ArrowBG.h>
#include "WheelSwitch.h"
#include "WheelSwitchP.h"

#include <math.h>
#include <ctype.h>

#define sgn(x) ((x >= 0)?1:-1)
#define abs(x) (((x) > 0)?(x):(-x))
#ifndef min
# define min(x, y) (((x) > (y))?(y):(x))
#endif
#ifndef max
# define max(x, y) (((x) > (y))?(x):(y))
#endif

/* Macros for convert routines.  One in O'Reilly Vol. 4, Motif is
 * wrong.  This one is from Xt source code. What you do depends on
 * toVal->addr coming in.  */
#define	done(value, type) \
    {                                                       \
        if (toVal->addr != NULL) {                          \
            if(toVal->size < sizeof(type)) {                \
                toVal->size = sizeof(type);                 \
                return False;                               \
            }                                               \
            *(type*)(toVal->addr) = (value);                \
        } else {                                            \
            static type static_val;                         \
            static_val = (value);                           \
            toVal->addr = (XPointer)&static_val;            \
        }                                                   \
        toVal->size = sizeof(type);                         \
    }

/* Function prototypes */

/* Intrinsics routines */
static Boolean CvtStringToDoublePointer(Display *display, XrmValue *args,
  Cardinal *num_args,  XrmValue *fromVal,  XrmValue *toVal, XtPointer *data);
static void FreeDoublePointer(XtAppContext app, XrmValuePtr toVal,
  XtPointer closure, XrmValuePtr rargs, Cardinal *num_args);
static void ClassInitialize(void);
static void Initialize(Widget wreq, Widget wnew,
  ArgList args, Cardinal *nargs);
static Boolean SetValues(Widget wcur, Widget wreq, Widget wnew,
  ArgList args, Cardinal *nargs);
static void Resize(Widget widget);
static void Destroy(Widget widget);
static void Realize(Widget widget, Mask *valueMask,
  XSetWindowAttributes *attributes);
static void Redisplay(Widget widget, XEvent *event, Region region);
#if 0
/* KE: Not used */
static Boolean AcceptFocus(Widget w, Time *time);
#endif

/* Utlilties */
static void post_format_warning(WswWidget wsw, char *format);
static void free_buttons(WswWidget wsw, Boolean destroy);
static void compute_format_size(WswWidget wsw);
static void compute_format(WswWidget wsw);
static Widget create_button(WswWidget wsw, Boolean up_flag, int digit);
static void create_buttons(WswWidget wsw);
static void compute_inner_geo( WswWidget wsw);
static void compute_geometry(WswWidget wsw);
static void set_increments(WswWidget wsw);
static void truncate_value(WswWidget wsw);
static void compute_format_min_max(WswWidget wsw);
static void draw(XtPointer clientdata, XtIntervalId *id);
static void set_button_visibility(WswWidget wsw);
static void select_digit(WswWidget wsw, int digit);
static void highlight_digit(WswWidget wsw, int digit);
static void unhighlight_digit(WswWidget wsw, int digit);
static void clear_all_buttons(WswWidget wsw);
static void arm_execute_timer(WswWidget wsw,  XEvent *event);
static void allocstr(char **str);
static void wsFree(XtPointer *ptr);
static XFontStruct *getFontFromPattern(Display *display,
  char *font_pattern, Dimension width, Dimension height,
  Dimension *width_return, Dimension *height_return);
static XFontStruct *getFontFromList(Display *display,
  XmFontList font_list, Dimension width, Dimension height,
  Dimension *width_return, Dimension *height_return);
static Boolean kbdIsEntering(WswWidget wsw);
static void kbdReset(WswWidget wsw);
static void freeFont(WswWidget wsw);

/* Callbacks, timer procs, and event handlers */
static void upButtonArmCallback(Widget button, XtPointer userdata,
  XtPointer calldata);
static void downButtonArmCallback(Widget button, XtPointer userdata,
  XtPointer calldata);
static void upButtonTimerProc(XtPointer userdata, XtIntervalId *id);
static void downButtonTimerProc(XtPointer userdata,  XtIntervalId *id);
static void btn1UpProc(Widget widget, XEvent *event, String *params,
  Cardinal *num_params);
static void btn1DownProc(Widget widget,  XEvent *event, String *params,
  Cardinal *num_params);
static void upArrowkeyProc(Widget widget, XEvent *event, String *params,
  Cardinal *num_params);
static void downArrowkeyProc(Widget widget, XEvent *event, String *params,
  Cardinal *num_params);
static void releaseArrowkeyProc(Widget widget,  XEvent *event, String *params,
  Cardinal *num_params);
static void rightArrowkeyProc(Widget widget, XEvent *event, String *params,
  Cardinal *num_params);
static void leftArrowkeyProc(Widget widget, XEvent *event, String *params,
  Cardinal *num_params);
static void lostFocusProc(Widget widget,  XEvent *event, String *params,
  Cardinal *num_params);
static void getFocusProc(Widget widget, XEvent *event, String *params,
  Cardinal *num_params);
static void executeTimerProc(XtPointer clientdata, XtIntervalId *id);
static void kbdHandler(Widget widget, XtPointer dummy, XEvent *event,
  Boolean *ctd);

/* Internal Motif functions not defined in header files */
void _XmGadgetTraverseNextTabGroup();
void _XmGadgetTraversePrevTabGroup();
unsigned char _XmGetFocusPolicy();
Widget _XmGetTabGroup();
void _XmDispatchGadgetInput();
void _XmUnhighlightBorder();
void _XmHighlightBorder();

/* Global variables */

static char defaultFormat[] = DEFAULT_FORMAT;
static char defaultFontPattern[] =
"-*-courier-medium-r-normal--*-*-75-75-*-*-iso8859-1";
/* Buffer for the event which initiated timer callbacks */
static XEvent timer_original_event;
WidgetClass wheelSwitchWidgetClass = (WidgetClass)&wswClassRec;

/* Resources  */

static XtResource resources[] = {
  /* Geometry related */
    {XmNconformToContent, XmCRecomputeSize, XmRBoolean, sizeof(Boolean),
     XtOffset(WswWidget,wsw.conform_to_content), XmRImmediate, (XtPointer)True},
    {XmNmarginWidth, XmCMarginWidth, XmRShort, sizeof(short),
     XtOffset(WswWidget, wsw.margin_width), XmRImmediate, (XtPointer)0},
    {XmNmarginHeight, XmCMarginHeight, XmRShort, sizeof(short),
     XtOffset(WswWidget, wsw.margin_height), XmRImmediate, (XtPointer)0},

  /* Shadows */
    {XmNshadowThickness, XmCShadowThickness, XmRDimension, sizeof(Dimension),
     XtOffset(WswWidget, manager.shadow_thickness), XmRImmediate, (XtPointer)2},
    {XmNshadowType, XmCShadowType, XmRShadowType, sizeof(unsigned int),
     XtOffset(WswWidget, wsw.shadow_type), XmRImmediate,
     (XtPointer)XmSHADOW_OUT},

  /* Format and font related */
    {XmNformat, XmCformat, XmRPointer, sizeof(char *),
     XtOffset(WswWidget,wsw.format), XmRImmediate, (XtPointer)defaultFormat},
    {XmNfontPattern, XmCfontPattern, XmRPointer, sizeof(char *),
     XtOffset(WswWidget,wsw.font_pattern), XmRImmediate,
     (XtPointer)defaultFontPattern},
    {XmNfontList, XmCFontList, XmRFontList, sizeof(XmFontList),
     XtOffset(WswWidget,wsw.font_list), XmRString, (XtPointer)NULL},

  /* These are double * and non-standard */
    {XmNcurrentValue, XmCCurrentValue, XmRDoublePointer, sizeof(double *),
     XtOffset(WswWidget, wsw.value), XmRString, "0.0"},
    {XmNminValue, XmCMinValue, XmRDoublePointer, sizeof(double *),
     XtOffset(WswWidget, wsw.min_value), XmRString, "-1.e+10"},
    {XmNmaxValue, XmCMaxValue, XmRDoublePointer, sizeof(double *),
     XtOffset(WswWidget, wsw.max_value), XmRString, "1.e+10"},

  /* Settings */
    {XmNrepeatInterval, XmCRepeatInterval, XmRInt, sizeof(int),
     XtOffset(WswWidget, wsw.repeat_interval), XmRImmediate, (XtPointer)30},
    {XmNcallbackDelay, XmCCallbackDelay, XmRInt, sizeof(int),
     XtOffset(WswWidget, wsw.callback_delay), XmRImmediate, (XtPointer)0},

  /* Callback */
    {XmNvalueChangedCallback, XmCCallback, XmRCallback,
     sizeof(XtCallbackList),
     XtOffset(WswWidget, wsw.value_changed_callback),
     XmRCallback,(XtPointer)NULL},
};

/* Action table */

static XtActionsRec actions[] = {
    { "Btn1Up", btn1UpProc },
    { "Btn1Down", btn1DownProc },
    { "WswPrevTabGroup",(XtActionProc)_XmGadgetTraversePrevTabGroup },
    { "WswNextTabGroup",(XtActionProc)_XmGadgetTraverseNextTabGroup },
    { "WswGetFocus", getFocusProc },
    { "WswLostFocus", lostFocusProc },
    { "WswUpArrowkey", upArrowkeyProc },
    { "WswDownArrowkey", downArrowkeyProc },
    { "WswRightArrowkey", rightArrowkeyProc },
    { "WswLeftArrowkey", leftArrowkeyProc },
    { "WswReleaseArrowkey", releaseArrowkeyProc },
};

/* Translations table */

static char translations[] =
"<Btn1Up>:  Btn1Up() \n\
         <KeyUp>osfUp: WswReleaseArrowkey()\n\
         <KeyUp>osfDown: WswReleaseArrowkey()\n\
         <Key>osfUp: WswUpArrowkey()\n\
         <Key>osfDown: WswDownArrowkey()\n\
         <Key>osfRight: WswRightArrowkey()\n\
         <Key>osfLeft: WswLeftArrowkey() \n\
         <Btn1Down>:  Btn1Down() \n\
         Shift<Key>Tab:  WswPrevTabGroup() \n\
         <Key>Tab:       WswNextTabGroup() \n\
         <FocusOut>: WswLostFocus()\n\
         <FocusIn>: WswGetFocus()";

/* Class record  */

WswClassRec wswClassRec = {
    {
      /* core_class fields */
        /* superclass             */ (WidgetClass)&xmManagerClassRec,
        /* class_name             */ "XmWheelSwitch",
        /* widget_size            */ sizeof(WswRec),
        /* class_initialise       */ ClassInitialize,
        /* class_part_initialize  */ NULL,
        /* class_inited           */ False,
        /* initialize             */ Initialize,
        /* initialize_hook        */ NULL,
        /* realize                */ Realize,
        /* actions                */ actions,
        /* num_actions            */ XtNumber(actions),
        /* resources              */ resources,
        /* num_resources          */ XtNumber(resources),
        /* xrm_class              */ NULLQUARK,
        /* compress_motion        */ True,
        /* compress_exposure      */ True,
        /* compress_enterleave    */ True,
        /* visible_interest       */ False,
        /* destroy                */ Destroy,
        /* resize                 */ Resize,
        /* expose                 */ Redisplay,
        /* set_values             */ SetValues,
        /* set_values_hook        */ NULL,
        /* set_values_almost      */ XtInheritSetValuesAlmost,
        /* get_values_hook        */ NULL,
        /* accept_focus           */ NULL,
        /* version                */ XtVersion,
        /* callback_offsets       */ NULL,
        /* tm_table               */ translations,
        /* query_geometry         */ XtInheritQueryGeometry,
        /* display_accelerator    */ NULL,
        /* extension              */ NULL,
    },
  /* Composite class fields */
    {
	/* geometry handler       */ XtInheritGeometryManager,
        /* change_managed         */ XtInheritChangeManaged,
        /* insert_child           */ XtInheritInsertChild,
        /* delete_child           */ XtInheritDeleteChild,
        /* extension              */ NULL,
    },
  /* Constraint_class fields */
    {
        /* resource list          */ NULL,         
        /* num resources          */ 0,            
        /* constraint size        */ 0,            
        /* init proc              */ NULL,         
        /* destroy proc           */ NULL,         
	/* set values proc        */ NULL,         
        /* extension              */ NULL,         
    },
  /* Manager_class fields */
    {
        /* default translations   */ NULL,        
        /* get resources          */ NULL,        
        /* num get_resources      */ 0,           
        /* get_cont_resources     */ NULL,        
        /* num_get_cont_resources */ 0,           
        /* parent process         */ NULL,        
        /* extension              */ NULL,        
    },
  /* Wsw Class Field */
    {
        /* extension              */ NULL
    }
};

/* Convenience routines */

Widget XmWheelSwitch(Widget parent_widget, char *name, Position x, Position y,
  double *value, char *format, double *min_value, double *max_value,
  XtCallbackList callback, XtCallbackList help_callback)
{
    Arg arglist[8];

    XtSetArg(arglist[0], XmNx, x);
    XtSetArg(arglist[1], XmNy, y);
    XtSetArg(arglist[2], XmNcurrentValue, value);
    XtSetArg(arglist[3], XmNformat, format);
    XtSetArg(arglist[4], XmNminValue, min_value);
    XtSetArg(arglist[5], XmNmaxValue, max_value);
    XtSetArg(arglist[6], XmNvalueChangedCallback, callback);
    XtSetArg(arglist[7], XmNhelpCallback, help_callback);
    return(XtCreateWidget(name, (WidgetClass)&wswClassRec, parent_widget,
	     arglist, 8));
}

void XmWheelSwitchSetValue(Widget wsw, double *value)
{
    Arg arglist[1];

    XtSetArg(arglist[0], XmNcurrentValue, value);
    XtSetValues(wsw, arglist, 1);
}

Widget XmCreateWheelSwitch(Widget parent_widget, char *name, ArgList arglist,
  int num_args)
{
    return(XtCreateWidget(name, (WidgetClass)&wswClassRec, parent_widget,
	     arglist, num_args));
}

/* Converter */

/* It is not possible to use doubles with XtRval in Arg or ArgList.
 * XtRval is not long enough for a double.  (It is not even possible
 * to use floats, because XtRval is usually typdef'ed to long, and the
 * float gets converted to long.  Programs that use floats in
 * resources have a mechanism to convert the floats to/from the longs
 * in XtRval.)  WheelSwitch uses doubles and gets around this by
 * passing double * pointers. This is not a standard thing to do.
 * This routine converts resource strings to pointers, allocating the
 * space. Note that if another converter from XtRString to
 * XtRDoublePointer is added, then the last one added will be used. */
static Boolean CvtStringToDoublePointer(Display *display, XrmValue *args,
  Cardinal *num_args,  XrmValue *fromVal,  XrmValue *toVal, XtPointer *data)
{
    static double *double_pointer;
    int nread = 0;
    
#if DEBUG_CONVERTER
    printf("CvtStringToDoublePointer [before]: from: %p[%s] to: %p size=%u\n",
      fromVal->addr,(char *)fromVal->addr,toVal->addr,toVal->size);
#endif    

  /* Check arguments */
    if(*num_args != 0) {
      /* No application context, use XtErrorMsg */
	XtErrorMsg("wrongParameters","CvtStringToDoublePointer",
	  "XtToolkitError",
	  "WheelSwitch: String to double conversion takes no arguments", 
	  (String *)NULL, (Cardinal *)NULL);
#if DEBUG_CONVERTER
	printf("  Should be no arguments! Returning False\n");
#endif    
	return False;
    }

  /* Allocate the space for the value and scan it in */
    double_pointer = (double *)XtMalloc(sizeof(double));
    nread=sscanf((char *)fromVal->addr, "%lf", double_pointer);
    if(nread != 1) {
	XtDisplayStringConversionWarning(display, (char *)fromVal->addr,
	  XmRDoublePointer);
#if DEBUG_CONVERTER
	printf("  Bad value! Returning False\n");
#endif    
	return False;
    }
    else {
	done(double_pointer, double *);
    }

#if DEBUG_CONVERTER
    printf("CvtStringToDoublePointer [after]: from: %p[%s] to: %p[%g]\n",
      fromVal->addr,(char *)fromVal->addr,toVal->addr,
      toVal->addr?*(*(double **)toVal->addr):0.0);
#endif    

    return True;
}

/* This destructor releases the memory allocated in the convertor,
 * which will not be freed otherwise.  The documentation on such
 * destructors is sketchy.  It probably isn't needed.  It will be used
 * if the converter in ClassInitialize is set with XtCacheRefCount AND
 * the widget sets XmNInitialResourcesPersistent to False. */
static void FreeDoublePointer(XtAppContext app, XrmValuePtr toVal,
  XtPointer closure, XrmValuePtr rargs, Cardinal *num_args)
{
#if DEBUG_CONVERTER || DEBUG_DESTRUCTOR
    printf("FreeDoublePointer: %p[%g]\n",
      (void *)*(double **)toVal->addr,
      toVal->addr?*(*(double **)toVal->addr):0.0);
#endif    

  /* Check arguments */
    if(*num_args != 0) {
	XtAppErrorMsg(app,
	  "wrongParameters", "FreeDoublePointer", "WheelSwitchError",
	  "WheelSwitch: FreeDoublePointer takes no arguments", 
	  (String *)NULL, (Cardinal *)NULL);
	return;
    }

    if(toVal->addr) XtFree((char *)*(double **)toVal->addr);
}

/* Intrinsics routines */

static void ClassInitialize(void)
{
#if USE_CACHE_REF_COUNT
    XtCacheType cacheType = XtCacheAll|XtCacheRefCount;
#else
    XtCacheType cacheType = XtCacheAll;
#endif    

#if DEBUG_CONVERTER || DEBUG_FLOW
    printf("ClassInitialize: start\n");
#if USE_CACHE_REF_COUNT
    printf("  Using XtCacheAll|XtCacheRefCount\n");
#else
    printf("  Using XtCacheAll\n");
#endif
#endif    

  /* Add a converter for pointers to doubles */
    XtSetTypeConverter(XmRString, XmRDoublePointer, CvtStringToDoublePointer,
      NULL, 0, cacheType, FreeDoublePointer);
}

static void Initialize(Widget wreq, Widget wnew,
  ArgList args, Cardinal *nargs)
{
    WswWidget req = (WswWidget)wreq;
    WswWidget new = (WswWidget)wnew;

#if DEBUG_INITIALIZE || DEBUG_FLOW
    printf("Initialize: format=|%s| font_pattern=|%s|\n",
      new->wsw.format,new->wsw.font_pattern);
    printf("  font_list=%p value_changed_callback=%p\n",
      (void *)new->wsw.font_list,(void *)new->wsw.value_changed_callback);
    printf("  value=%p[%g] min_value=%p[%g] max_value=%p[%g]\n",
      (void *)new->wsw.value,
      (void *)new->wsw.value?*new->wsw.value:0.0,
      (void *)new->wsw.min_value,
      (void *)new->wsw.min_value?*new->wsw.min_value:0.0,
      (void *)new->wsw.max_value,
      (void *)new->wsw.max_value?*new->wsw.max_value:0.0);
    printf("  repeat_interval=%d callback_delay=%d\n",
      new->wsw.repeat_interval,new->wsw.callback_delay);
    printf("  margin_height=%d margin_width=%d\n",
      new->wsw.margin_height,new->wsw.margin_width);
    printf("  shadow_thickness=%hu shadowType=%u\n",
      new->manager.shadow_thickness,new->wsw.shadow_type);
    printf("  top_shadow_color=%08lx top_shadow_color=%08lx\n",
      new->manager.top_shadow_color,new->manager.bottom_shadow_color);
#endif    

  /* Initialize internal fields */
    new->wsw.format_min_value = 0.0;
    new->wsw.format_max_value = 0.0;
    new->wsw.foreground_GC = NULL;
    new->wsw.background_GC = NULL;
    new->wsw.GC_inited = False;
    new->wsw.nb_digit = 0;
    new->wsw.user_font = False;
    new->wsw.font = NULL;

    new->wsw.increments = NULL;
    new->wsw.digit_number = 0;
    new->wsw.prefix_size = 0;
    new->wsw.digit_size = 0;
    new->wsw.postfix_size = 0;
    new->wsw.point_position = 0;
    new->wsw.up_buttons = NULL;
    new->wsw.down_buttons = NULL;
    new->wsw.current_flag = False;  /* wsw is selected */
    new->wsw.selected_digit = 0;
    new->wsw.pending_timeout_id = (XtIntervalId)0;
    new->wsw.callback_timeout_id = (XtIntervalId)0;
    new->wsw.prefix_x = 0;
    new->wsw.digit_x = 0;
    new->wsw.postfix_x = 0;
    new->wsw.up_button_y = 0;
    new->wsw.digit_y = 0;
    new->wsw.string_base_y = 0;
    new->wsw.down_button_y = 0;
    new->wsw.digit_width = 0;
    
    new-> wsw.kbd_format= NULL;
    new-> wsw.kbd_value = NULL;
    new-> wsw.blink_timeout_id = (XtIntervalId)0;
    new-> wsw.to_clear= True;

  /* Compute format-related quantities */
    compute_format(new);

  /* Allocate space for the font pattern */
    if(new->wsw.font_pattern == NULL) {
      /* Insure there is always a font pattern */
	new->wsw.font_pattern = defaultFontPattern;
    }
    allocstr(&new->wsw.font_pattern);
    
  /* Flag if there is a font list specified */
    if(new->wsw.font_list != NULL) {
        new->wsw.user_font = True;
    }

  /* Get the font and compute the geometry */
    compute_geometry(new);

  /* Allocate space for pointers to values, min, and max and set
   * them */
    new->wsw.value = (double *)XtMalloc(sizeof(double));
    *new->wsw.value = *req->wsw.value;
    new->wsw.min_value = (double *)XtMalloc(sizeof(double));
    *new->wsw.min_value = *req->wsw.min_value;
    new->wsw.max_value = (double *)XtMalloc(sizeof(double));
    *new->wsw.max_value = *req->wsw.max_value;
    compute_format_min_max(new);

  /* Add translations */
    XtOverrideTranslations(wnew,
      XtParseTranslationTable(translations));
  /* KE: This seems to call SetValues */
    XmAddTabGroup(wnew);

  /* Add keyboard event handler */
    XtAddEventHandler(wnew,KeyPressMask,False,kbdHandler,NULL);
#if DEBUG_FLOW
    printf("Initialize: end\n");
#endif
}

/* Note update does different things at different points in this
 * procedure, but stays True once it is true.  The return value
 * determines if an expose (Redisplay) is sent */
static Boolean SetValues(Widget wcur, Widget wreq, Widget wnew,
  ArgList args, Cardinal *nargs)
{
    WswWidget cur = (WswWidget)wcur;
    WswWidget req = (WswWidget)wreq;
    WswWidget new = (WswWidget)wnew;
    Boolean update = False;

#if DEBUG_POINTER
    static int call=0;
    
    call++;
    printf("%d SetValues [Before]:\n"
      " cur->value=%p *cur->value=%g\n"
      " req->value=%p *req->value=%g\n"
      " new->value=%p *new->value=%g\n",
      call,
      (void *)cur->wsw.value,*cur->wsw.value,
      (void *)req->wsw.value,*req->wsw.value,
      (void *)new->wsw.value,*new->wsw.value);
    printf(
      " cur->max_value=%p *cur->max_value=%g\n"
      " req->max_value=%p *req->max_value=%g\n"
      " new->max_value=%p *new->max_value=%g\n",
      (void *)cur->wsw.max_value,*cur->wsw.max_value,
      (void *)req->wsw.max_value,*req->wsw.max_value,
      (void *)new->wsw.max_value,*new->wsw.max_value);
#endif    
#if DEBUG_FLOW
    printf("SetValues: start\n");
#endif
  /* Format change */
    if(cur->wsw.format != new->wsw.format) {
        wsFree((XtPointer)&cur->wsw.format);
	free_buttons(new, False);
        compute_format(new);
        if(new->wsw.conform_to_content == True &&
          cur->core.width == new->core.width &&
          cur->core.height == new->core.height) {
            new->core.width = 0;
            new->core.height = 0;
        }
        update = True;
    }

  /* Font pattern change */
    if(new->wsw.font_pattern == NULL) {
      /* Insure there is always a font pattern */
	new->wsw.font_pattern = defaultFontPattern;
    }
    if(cur->wsw.font_pattern != new->wsw.font_pattern) {
	wsFree((XtPointer)&cur->wsw.font_pattern);
	allocstr(&new->wsw.font_pattern);
	update = True;
    }

  /* Font list change */
    if(cur->wsw.font_list != new->wsw.font_list) {
	if(new->wsw.font_list == NULL) {
	    new->wsw.user_font = False;
	} else {
	    new->wsw.user_font = True;
	}
        update = True;
    }

  /* Geometry changes */
    if(cur->core.width != new->core.width ||
      cur->core.height != new->core.height ||
      cur->wsw.margin_width != new->wsw.margin_width ||
      cur->wsw.margin_height != new->wsw.margin_height ||
      cur->manager.shadow_thickness != new->manager.shadow_thickness) {
        update = True;
    }

  /* Update the geometry */
    if(update) {
      /* Free the font (if necessary) so it will be recalculated */
	freeFont(new);
	compute_geometry(new);
    }

  /* Update value, min, and max.  Copy the value, use the old pointer
   * (allocated in Initialize and freed in Destroy). The user's
   * pointer is not kept, and he does not have a pointer to the value
   * in the widget. Pointers are used because doubles cannot be passed
   * as resources in XtSetValues, etc. */
    if(req->wsw.min_value != cur->wsw.min_value) {
        new->wsw.min_value = cur->wsw.min_value;
        *new->wsw.min_value = *req->wsw.min_value;
        update = True;
    }
    if(req->wsw.max_value != cur->wsw.max_value) {
        new->wsw.max_value =  cur->wsw.max_value;
        *new->wsw.max_value = *req->wsw.max_value;
        update = True;
    }
    if(update) {
        compute_format_min_max(new);
    }

    if(req->wsw.value != cur->wsw.value) {
        new->wsw.value = cur->wsw.value;
      /* Don't change if there is a pending_timeout_id (pending
       * up/downButtonTimerProc) or callback_timeout_id (pending
       * executeTimerProc) */
      /* KE: Need to fix this or an only update may be lost */
        if(cur->wsw.pending_timeout_id == 0 &&
          cur->wsw.callback_timeout_id == 0 &&
          *cur->wsw.value != *req->wsw.value) {
            *new->wsw.value = *req->wsw.value;
            if(!update && XtIsRealized(wnew)) {
	      /* KE: Check this */
                set_button_visibility(new);
                if(new->wsw.GC_inited) {
		    draw((XtPointer)new,NULL);
		}
            } else {
                update = True;
            }
        } else {
	  /* Keep the old value */
#if DEBUG_SETVALUE
	    printf("!!!SetValue: old value=%g not changed to new value=%g\n",
	      *cur->wsw.value,*new->wsw.value);
	    XBell(XtDisplay(cur),50);
#endif
	}
    }
    
  /* Foreground or background change */
    if(new->wsw.GC_inited) {
        if(cur->manager.foreground != new->manager.foreground) {
            XSetForeground(XtDisplay(new), new->wsw.foreground_GC,
              new->manager.foreground);
            XSetForeground(XtDisplay(new), new->wsw.background_GC,
              new->manager.foreground);
	    update = True;
        }
        if(cur->core.background_pixel != new->core.background_pixel) {
            XSetBackground(XtDisplay(new), new->wsw.foreground_GC,
              new->core.background_pixel);
            XSetBackground(XtDisplay(new), new->wsw.background_GC,
              new->core.background_pixel);
	    update = True;
        }
    }

  /* Shadow type change */
    if(cur->wsw.shadow_type != new->wsw.shadow_type) {
        update = True;
    }
    
#if DEBUG_POINTER
    printf("%d SetValues [After]:\n"
      " cur->value=%p *cur->value=%g\n"
      " req->value=%p *req->value=%g\n"
      " new->value=%p *new->value=%g\n",
      call,
      (void *)cur->wsw.value,*cur->wsw.value,
      (void *)req->wsw.value,*req->wsw.value,
      (void *)new->wsw.value,*new->wsw.value);
    printf(
      " cur->max_value=%p *cur->max_value=%g\n"
      " req->max_value=%p *req->max_value=%g\n"
      " new->max_value=%p *new->max_value=%g\n",
      (void *)cur->wsw.max_value,*cur->wsw.max_value,
      (void *)req->wsw.max_value,*req->wsw.max_value,
      (void *)new->wsw.max_value,*new->wsw.max_value);
#endif    
#if DEBUG_FLOW
    printf("SetValues: end\n");
#endif

    return(update && XtIsRealized(wnew));
}

static void Resize(Widget widget)
{
    WswWidget wsw = (WswWidget)widget;
    Dimension height, width;

#if DEBUG_FLOW
    printf("Resize: start\n");
#endif

    if(!wsw->wsw.user_font) {
	Dimension reqWidth = 
	  (wsw->core.width - 2*wsw->wsw.margin_width -
	    2*wsw->manager.shadow_thickness) / wsw->wsw.nb_digit;
	Dimension reqHeight =
	  (wsw->core.height - 2*wsw->wsw.margin_height -
	    2*wsw->manager.shadow_thickness) / 2;
        wsw->wsw.font = getFontFromPattern(XtDisplay(wsw),
	  wsw->wsw.font_pattern, reqWidth, reqHeight, &width, &height);
        compute_inner_geo(wsw);
        if(wsw->wsw.GC_inited) {
            XSetFont(XtDisplay(wsw), wsw->wsw.foreground_GC,
              wsw->wsw.font->fid);
            XSetFont(XtDisplay(wsw), wsw->wsw.background_GC,
              wsw->wsw.font->fid);
        }
    }
#if DEBUG_FLOW
    printf("Resize: end\n");
#endif
}

static void Destroy(Widget widget)
{
    WswWidget wsw = (WswWidget)widget;

    wsFree((XtPointer)&wsw->wsw.format);
    XmRemoveTabGroup(widget);
    if(wsw->wsw.background_GC != NULL) {
	XFreeGC(XtDisplay(wsw), wsw->wsw.background_GC);
	wsw->wsw.background_GC = NULL;
    }
    if(wsw->wsw.foreground_GC != NULL) {
	XFreeGC(XtDisplay(wsw), wsw->wsw.foreground_GC);
	wsw->wsw.foreground_GC = NULL;
    }
    wsFree((XtPointer)&wsw->wsw.font_pattern);
    wsw->wsw.font_pattern = NULL;
    if(wsw->wsw.font_list != NULL) {
	XmFontListFree(wsw->wsw.font_list);
	wsw->wsw.font_list = NULL;
    }
    if(wsw->wsw.font != NULL) {
	freeFont(wsw);
    }
    if(XtIsRealized(widget)) {
      /* Do not destroy them.  Xt does this and it will cause
       * problems here */
	free_buttons(wsw, False);
    }
    if(wsw->wsw.increments != NULL) {
	XtFree((char *)wsw->wsw.increments);
	wsw->wsw.increments = NULL;
    }
    if(wsw->wsw.value != NULL) {
	XtFree((char *)wsw->wsw.value);
	wsw->wsw.value = NULL;
    }
    if(wsw->wsw.min_value != NULL) {
	XtFree((char *)wsw->wsw.min_value);
	wsw->wsw.min_value = NULL;
    }
    if(wsw->wsw.max_value != NULL) {
	XtFree((char *)wsw->wsw.max_value);
	wsw->wsw.max_value = NULL;
    }
    wsFree((XtPointer)&wsw->wsw.kbd_format);
    wsw->wsw.kbd_format = NULL;
    if(wsw->wsw.kbd_value != NULL) {
	XtFree(wsw->wsw.kbd_value);
	wsw->wsw.kbd_value = NULL;
    }
  /* TBC ?? */
}

static void Realize(Widget widget, Mask *valueMask,
  XSetWindowAttributes *attributes)
{
#if DEBUG_FLOW
    printf("Realize:\n");
#endif

    (*widgetClassRec.core_class.realize)(widget, valueMask, attributes);

  /* change cursor */
  /* XDefineCursor(XtDisplay(wsw), XtWindow(wsw),
     XCreateFontCursor(XtDisplay(wsw), XC_double_arrow)); */

}

/* Expose routine.  GCs are created here on the first call, and the
 * font is set on all calls.  Manager takes care of shadow GCs and
 * colors. The event structure should be XExposeEvent (Expose
 * events).  */
static void Redisplay(Widget widget, XEvent *event, Region region)
{
    WswWidget wsw = (WswWidget)widget;

#if DEBUG_FLOW || DEBUG_EXPOSE
    printf("Redisplay: start\n");
#endif
#if DEBUG_EXPOSE    
    printf("  type=%d send_event=%s serial=%lu window=%p\n",
      ((XAnyEvent *)event)->type,((XAnyEvent *)event)->send_event?"True":"False",
      ((XAnyEvent *)event)->serial,(void *)((XAnyEvent *)event)->window);
    if(((XAnyEvent *)event)->type == Expose) {
	XExposeEvent *eev = (XExposeEvent *)event;
	printf("  count=%d x=%d y=%d width=%d height=%d\n",
	  eev->count,eev->x,eev->y,eev->width,eev->height);
    }
#endif

    if(wsw->wsw.GC_inited == False) {
      /* Create GCs */
        unsigned long valuemask = 0;
        XGCValues val;

      /* Background */
	wsw->wsw.background_GC = XCreateGC(XtDisplay(wsw), XtWindow(wsw),
          valuemask, &val);
        XSetFont(XtDisplay(wsw), wsw->wsw.background_GC, wsw->wsw.font->fid);
        XSetBackground(XtDisplay(wsw), wsw->wsw.background_GC,
	  wsw->core.background_pixel);
        XSetForeground(XtDisplay(wsw), wsw->wsw.background_GC,
	  wsw->manager.foreground);
      /* Foreground */
        wsw->wsw.foreground_GC = XCreateGC(XtDisplay(wsw), XtWindow(wsw),
          valuemask, &val);
        XSetFont(XtDisplay(wsw), wsw->wsw.foreground_GC, wsw->wsw.font->fid);
        XSetBackground(XtDisplay(wsw), wsw->wsw.foreground_GC,
	  wsw->core.background_pixel);
        XSetForeground(XtDisplay(wsw), wsw->wsw.foreground_GC,
	  wsw->manager.foreground);
        wsw->wsw.GC_inited = True;
    }
    XSetRegion(XtDisplay(wsw), wsw->wsw.background_GC, region);
    XSetRegion(XtDisplay(wsw), wsw->wsw.foreground_GC, region);
    draw((XtPointer)wsw,NULL);
    _XmRedisplayGadgets(widget, event, region);
    XSetClipMask(XtDisplay(wsw), wsw->wsw.background_GC, None);
    XSetClipMask(XtDisplay(wsw), wsw->wsw.foreground_GC, None);

#if DEBUG_FLOW
    printf("Redisplay: end\n");
#endif
}

#if 0
/* KE: Not used */
static Boolean AcceptFocus(Widget w, Time *time)
{
    WswWidget wsw = (WswWidget)w;
    XSetInputFocus(XtDisplay(wsw), XtWindow(wsw), RevertToPointerRoot,
      CurrentTime);
    return(True);
}
#endif

/* Utilities */

static void post_format_warning(WswWidget wsw, char *format)
{
    String params[2];
    unsigned int num_params = 2;
    params[0] = wsw->wsw.format;
    params[1] = format;
    
#if DEBUG_ERRORHANDLER
   printf("post_format_warning:\n"
     "Invalid format: \"%s\", use \"%s\" instead\n",
     wsw->wsw.format,format);
   printf("  appContext=%p\n",
     (void *)XtWidgetToApplicationContext((Widget)wsw));
#endif
#if 0
 /* KE: Using XtAppWarningMsg seems to result in nothing happening.
  * If this can be resolved, use XtAppWarningMsg.  Note that
  * XtWarningMsg would be easier and equally effective. See
  * CvtStringToDoublePointer. */
   XtAppWarningMsg(XtWidgetToApplicationContext((Widget)wsw),
     "badFormat", "XtWswInitialize", "WheelSwitchWarning",
     "WheelSwitch: Invalid format: \"%s\", use \"%s\" instead",
     params, &num_params);
#else
 /* KE: MEDM replaces both the XtErrorHandler and the XtWarningHandler
  * with the same function, xtErrorHandler.  Using XtAppErrorMsg
  * message results in this handler getting called whereas
  * XtAppWarningMsg does not. The proper usage would be to use the
  * XtAppWarningMsg, as the default handler does not exit, whereas the
  * default for XtAppErrorMsg does. We do it this way so something
  * happens. The MEDM xtErrorhandler does not exit, so this is not a
  * problem for MEDM. */
   XtAppErrorMsg(XtWidgetToApplicationContext((Widget)wsw),
     "badFormat", "XtWswInitialize", "WheelSwitchError",
     "WheelSwitch: Invalid format: \"%s\", use \"%s\" instead",
     params, &num_params);
#endif   
}

/* Computes quantities associated with the format */
static void compute_format_size(WswWidget wsw)
{
    char zero_string[FORMAT_SIZE];
    int format_size;
    int i;
    
#if DEBUG_FLOW
    printf("compute_format_size: start\n");
#endif

  /* Write zero using the format */
    sprintf(zero_string, wsw->wsw.format, 0.);
    wsw->wsw.nb_digit = strlen(zero_string);
    wsw->wsw.prefix_size = strchr(wsw->wsw.format, '%') - wsw->wsw.format;
    format_size = strcspn(wsw->wsw.format + wsw->wsw.prefix_size, "f") + 1;
    wsw->wsw.postfix_size = strlen(wsw->wsw.format) - format_size -
      wsw->wsw.prefix_size;
    wsw->wsw.digit_size = wsw->wsw.nb_digit - wsw->wsw.prefix_size -
      wsw->wsw.postfix_size;
    wsw->wsw.point_position = 0;
    for(i = 0; i < wsw->wsw.digit_size; i++)
      if(zero_string[wsw->wsw.prefix_size + wsw->wsw.digit_size - 1 - i] == '.')
        wsw->wsw.point_position = i;
  /* digit_number is the number of arrows, one for each digit, none
   * for sign and point */
    wsw->wsw.digit_number = wsw->wsw.digit_size - 1;  /* Do not count the sign */
    if(wsw->wsw.point_position != 0)
      wsw->wsw.digit_number--;                        /* Do not count the point */
#if DEBUG_FORMAT
    printf("compute_format_size: format=|%s| zero_string=|%s|\n"
      "  nb_digit=%hd prefix_size=%d format_size=%d postfix_size=%d\n"
      "  digit_size=%d digit_number=%d point_position=%d\n"
      "  value=%g min_value=%g max_value=%g\n",
      wsw->wsw.format,zero_string,
      wsw->wsw.nb_digit,wsw->wsw.prefix_size,format_size,
      wsw->wsw.postfix_size,wsw->wsw.digit_size,
      wsw->wsw.digit_number,wsw->wsw.point_position,
      *wsw->wsw.value,*wsw->wsw.min_value,*wsw->wsw.max_value);
#endif    
#if DEBUG_FLOW
    printf("compute_format_size: end\n");
#endif
}

/* Checks format validity and calls compute_format_size */
static void compute_format(WswWidget wsw)
{
    char *percent = NULL,*fpart = NULL,*ad;
    char format_used[FORMAT_SIZE];
    char kbd_format_buffer[FORMAT_SIZE];
    char flag_char[2] = " ";
    int nb_digit0 = wsw->wsw.nb_digit;
    int width = DEFAULT_FORMAT_WIDTH;
    int precision = DEFAULT_FORMAT_PRECISION;
    int nparsed = 0;
    int i,j;
    
#if DEBUG_FLOW
    printf("compute_format: start\n");
#endif

  /* Check for a % with an f following somewhere after */
    percent = strchr(wsw->wsw.format, '%');
    if(percent != NULL) {
	fpart = strchr(percent, 'f');
    }
    if(percent == NULL || fpart == NULL) {
      /* Doesn't have % with an f following, use the default */
        strcpy(format_used, DEFAULT_FORMAT);
    } else {
      /* Copy prefix and % (i.e. everything through %) */
        format_used[0] = '\0';
        strncat(format_used, wsw->wsw.format, percent - wsw->wsw.format + 1);

      /* Check flag(s).  We want '+' or ' ', with '+' superceding ' ' */
	ad = percent + 1;
	while(1) {
	    if(*ad == '+') {
	      /* Use + as the flag */
		flag_char[0]='+';
		ad++;
	    } else if(*ad == ' ' || *ad == '#' || *ad == '0' || *ad == '-') {
	      /* A flag, but use what we have now (+ or space) */
		ad++;
	    } else {
	      /* Not a flag */
		break;
	    }
	}
      /* Add the single flag we want to use */
	strcat(format_used, flag_char);

      /* Width, point, and precision: Must be of form n.m.  Ignore
       * anything between this and f. Copy f to end. */
	nparsed=sscanf(ad,"%d.%d",&width,&precision);
	if(nparsed == 2) {
	    if(width < 0) width=DEFAULT_FORMAT_WIDTH;
	    if(precision < 0) precision=DEFAULT_FORMAT_PRECISION;
	    if(precision > width-1) precision=width-1;
	    if(precision < 0) precision=0;
	} else if(nparsed == 1) {
	    if(width < 0) {
		width=DEFAULT_FORMAT_WIDTH;
		precision=DEFAULT_FORMAT_PRECISION;
	    } else {
		precision=0;
	    }
	} else {
	    width=DEFAULT_FORMAT_WIDTH;
	    precision=DEFAULT_FORMAT_PRECISION;
	}
	sprintf(format_used+strlen(format_used),"%d.%d%s",
	  width,precision,fpart);
    }

  /* Check and see if what we constructed matches what was supplied,
   * but use what was constructed */
    if(strcmp(wsw->wsw.format, format_used) != 0) {
	post_format_warning(wsw, format_used);
    }
    wsw->wsw.format = format_used;
    allocstr(&wsw->wsw.format);

    compute_format_size(wsw);
    i=j=0;
    while((kbd_format_buffer[j++]=format_used[i++])!='%') ;
    while(!isdigit(kbd_format_buffer[j++]=format_used[i++])) ;
    while(format_used[i++]!='f') ;
    kbd_format_buffer[j-1]='\0';
    sprintf(kbd_format_buffer,"%s%ds%s",
      kbd_format_buffer,
      wsw->wsw.digit_size,
      format_used+i);
    wsw->wsw.kbd_format = kbd_format_buffer;
    allocstr(&wsw->wsw.kbd_format);

  /* Allocate and reset the kbd_value */
    if(wsw->wsw.kbd_value != NULL) {
	XtFree(wsw->wsw.kbd_value);
    }
    wsw->wsw.kbd_value = (char *)XtCalloc(wsw->wsw.digit_size+1, sizeof(char));
    kbdReset(wsw);

  /* Allocate the increments and set them */
    if(wsw->wsw.nb_digit != nb_digit0 || wsw->wsw.increments == NULL) {
	if(wsw->wsw.increments != NULL) {
	    XtFree((char *)wsw->wsw.increments);
	}
        wsw->wsw.increments =
          (double *)XtCalloc(wsw->wsw.digit_number, sizeof(double));
    }
    set_increments(wsw);
    
#if DEBUG_FORMAT
    printf("compute_format: format_used=|%s| kbd_format_buffer=|%s|\n"
      "  kbd_format=|%s| kbd_value=|%s|\n",
      format_used,kbd_format_buffer,
      wsw->wsw.kbd_format,wsw->wsw.kbd_value);
#endif    
#if DEBUG_FLOW
    printf("compute_format: end\n");
#endif
}

/* Create the buttons arrays */
static void create_buttons(WswWidget wsw)
{
    int     digit;

#if DEBUG_FLOW
    printf("create_buttons: digit_number=%d up_buttons=%p down_buttons=%p\n",
      wsw->wsw.digit_number,
      (void *)wsw->wsw.up_buttons,
      (void *)wsw->wsw.down_buttons);
#endif

    /* Free them first if they exist */
    if(wsw->wsw.up_buttons != NULL) {
      /* Set True to also destroy them */
        free_buttons(wsw, True);
    }

  /* Make new ones */
    wsw->wsw.up_buttons = (Widget *)XtCalloc(wsw->wsw.digit_number,
      sizeof(Widget));
    wsw->wsw.down_buttons = (Widget *)XtCalloc(wsw->wsw.digit_number,
      sizeof(Widget));
    for(digit = 0; digit < wsw->wsw.digit_number; digit++) {
        wsw->wsw.up_buttons[digit] = create_button(wsw, True, digit);
        XtAddCallback(wsw->wsw.up_buttons[digit], XmNarmCallback,
          upButtonArmCallback, (XtPointer)digit);
        wsw->wsw.down_buttons[digit] = create_button(wsw, False, digit);
        XtAddCallback(wsw->wsw.down_buttons[digit], XmNarmCallback,
          downButtonArmCallback,(XtPointer)digit);
    }

  /* Set the visibility */
    set_button_visibility(wsw);
}

/* Destroy the button if destroy is specified and free the buttons
 * arrays.  Don't specify destroy when the widget is being
 * destroyed. */
static void free_buttons(WswWidget wsw, Boolean destroy)
{    
#if DEBUG_FLOW
    printf("free_buttons: digit_number=%d up_buttons=%p down_buttons=%p\n",
      wsw->wsw.digit_number,
      (void *)wsw->wsw.up_buttons,
      (void *)wsw->wsw.down_buttons);
#endif    
    if(wsw->wsw.up_buttons != NULL) {
	if(destroy) {
	    int digit;
	    for(digit = 0; digit < wsw->wsw.digit_number; digit++) {
		XtDestroyWidget(wsw->wsw.up_buttons[digit]);
		XtDestroyWidget(wsw->wsw.down_buttons[digit]);
	    }
	}
        XtFree((char *)wsw->wsw.up_buttons);
        XtFree((char *)wsw->wsw.down_buttons);
        wsw->wsw.up_buttons = NULL;
        wsw->wsw.down_buttons = NULL;
    }
}

/* Create a single button */
static Widget create_button(WswWidget wsw, Boolean up_flag, int digit)
{
    Position x, y;
    char name[100];
    Arg args[10];
    Cardinal num_args = 0;
    Widget button;
    int direction;

    x = wsw->wsw.postfix_x - (digit + 1) * wsw->wsw.digit_width;
    if((wsw->wsw.point_position != 0) && (digit >= wsw->wsw.point_position))
      x -= XTextWidth(wsw->wsw.font, ".", 1);
    if(up_flag) {
        y = wsw->wsw.up_button_y;
        sprintf(name, "%s_%1d", "up_button", digit);
        direction = XmARROW_UP;
    } else {
        y = wsw->wsw.down_button_y;
        sprintf(name, "%s_%1d", "down_button", digit);
        direction = XmARROW_DOWN;
    }
    XtSetArg(args[num_args], XmNx, x); num_args++;
    XtSetArg(args[num_args], XmNy, y); num_args++;
    XtSetArg(args[num_args], XmNwidth, wsw->wsw.digit_width - 1); num_args++;
    XtSetArg(args[num_args], XmNheight, wsw->wsw.digit_width + 1); num_args++;
    XtSetArg(args[num_args], XmNarrowDirection, direction); num_args++;
    XtSetArg(args[num_args], XmNmarginHeight, 0); num_args++;
    XtSetArg(args[num_args], XmNmarginWidth, 0); num_args++;
    XtSetArg(args[num_args], XmNshadow, False); num_args++;
    XtSetArg(args[num_args], XmNborderWidth, 0); num_args++;
    XtSetArg(args[num_args], XmNshadowThickness, 0); num_args++;
    XtSetArg(args[num_args], XmNhighlightThickness, 1); num_args++;
    XtSetArg(args[num_args], XmNtraversalOn, True); num_args++;

    button = (Widget)XmCreateArrowButtonGadget((Widget)wsw, name, args,
      num_args);
#if 0
    /* KE: Manage them in set_button_visibility */
    XtManageChild(button);
#endif    
#if DEBUG_EXPOSE
    printf("  %2d %s x=%d y=%d\n",
      digit,up_flag?"Up  ":"Down",x,y);
#endif    
    return(button);
}

/* Compute the geometry, first getting a font if there is none. The
 * font must be set to zero beforehand if a new font is necessary */
static void compute_geometry(WswWidget wsw)
{
    Dimension width, height;
    char zero_string[FORMAT_SIZE];

#if DEBUG_FLOW
    printf("compute_geometry: start\n");
#endif

  /* Get a font if necessary */
    if(wsw->wsw.font == NULL) {
	Dimension reqWidth, reqHeight;

      /* Determine requested character width and height */
	if(wsw->wsw.conform_to_content || wsw->core.width == 0 ||
	  wsw->core.height == 0) {
	    reqWidth = DEFAULT_CHAR_WIDTH;
	    reqHeight = DEFAULT_CHAR_HEIGHT;;
	} else {
	    reqWidth = 
	    (wsw->core.width - 2*wsw->wsw.margin_width -
	      2*wsw->manager.shadow_thickness) / wsw->wsw.nb_digit;
	    reqHeight =
	    (wsw->core.height - 2*wsw->wsw.margin_height -
	      2*wsw->manager.shadow_thickness) / 2;
	}
	
      /* Get a font */
	if(wsw->wsw.user_font) {
	    wsw->wsw.font = getFontFromList(XtDisplay(wsw),
	      wsw->wsw.font_list, reqWidth, reqHeight, &width, &height);
	} else {
	    wsw->wsw.font = getFontFromPattern(XtDisplay(wsw),
	      wsw->wsw.font_pattern, reqWidth, reqHeight, &width, &height);
	}
	
      /* If there is still no font */
	if(wsw->wsw.font == NULL) {
	  /* KE: Check this and implement it */
	}
    }

    /* Make a string using the font with value zero */
      sprintf(zero_string, wsw->wsw.format, 0.);
      
    /* If conformToContent is set, set the core width and height */
      if(wsw->wsw.conform_to_content) {
	  wsw->core.width = 2 * wsw->wsw.margin_width +
	    2 * wsw->manager.shadow_thickness +
	    + XTextWidth(wsw->wsw.font, zero_string, strlen(zero_string));
	  wsw->core.height = 2 * wsw->wsw.margin_height +
	    2 * wsw->manager.shadow_thickness +
	    2 * wsw->wsw.font-> descent +
	    2 * XTextWidth(wsw->wsw.font, "0", 1)
	    + wsw->wsw.font->ascent;
      }
      
    /* Compute the rest of the geometry */
      compute_inner_geo(wsw);
      if(wsw->wsw.GC_inited) {
	  XSetFont(XtDisplay(wsw), wsw->wsw.foreground_GC,
	    wsw->wsw.font->fid);
	  XSetFont(XtDisplay(wsw), wsw->wsw.background_GC,
	    wsw->wsw.font->fid);
      }
#if DEBUG_FLOW
    printf("compute_geometry: end\n");
#endif
}

/* Calculates geometry quantities related to placement in the
 * widget and places the buttons accordingly */
static void compute_inner_geo(WswWidget wsw)
{
    char zero_string[FORMAT_SIZE];
    int xOff, yOff, borderWidth, borderHeight, textWidth, textHeight;
    
#if DEBUG_FLOW
    printf("compute_inner_geo: start\n");
#endif

    sprintf(zero_string, wsw->wsw.format, 0.);
    wsw->wsw.digit_width = XTextWidth(wsw->wsw.font, "0", 1);
    textWidth = XTextWidth(wsw->wsw.font, zero_string, strlen(zero_string));
    textHeight = 2*(wsw->wsw.digit_width + wsw->wsw.font->descent) +
      wsw->wsw.font->ascent;
    borderWidth = wsw->wsw.margin_width + wsw->manager.shadow_thickness;
    borderHeight = wsw->wsw.margin_height + wsw->manager.shadow_thickness;

  /* Calculate offsets to center the text in the widget */
    xOff = (wsw->core.width - textWidth)/2 - borderWidth;
    yOff = (wsw->core.height - textHeight)/2 - borderHeight;

  /* X positions */
    wsw->wsw.prefix_x = xOff + borderWidth;
    wsw->wsw.digit_x = wsw->wsw.prefix_x
      + XTextWidth(wsw->wsw.font, zero_string, wsw->wsw.prefix_size);
    wsw->wsw.postfix_x = wsw->wsw.digit_x
      + XTextWidth(wsw->wsw.font, zero_string + wsw->wsw.prefix_size,
	wsw->wsw.digit_size);

  /* Y positions */
    wsw->wsw.up_button_y = yOff + borderHeight;
    wsw->wsw.digit_y = wsw->wsw.up_button_y + wsw->wsw.digit_width;
    wsw->wsw.string_base_y = wsw->wsw.digit_y
      + wsw->wsw.font->ascent + wsw->wsw.font->descent;
    wsw->wsw.down_button_y = wsw->wsw.string_base_y + wsw->wsw.font->descent;

  /* Keep the arrows in bounds */
    if(wsw->wsw.up_button_y < borderHeight) {
	wsw->wsw.up_button_y = borderHeight;
    }
    if(wsw->wsw.down_button_y + wsw->wsw.digit_width +borderHeight >
      wsw->core.height) {
	wsw->wsw.down_button_y = wsw->core.height - borderHeight -
	  wsw->wsw.digit_width;
    }
    
#if DEBUG_FLOW
    printf("  width=%hu borderWidth=%d textWidth=%d xOff=%d\n"
      "  prefix_x=%hd digit_x=%hd postfix_x=%hd\n",
      wsw->core.width,borderWidth, textWidth,xOff,
      wsw->wsw.prefix_x,wsw->wsw.digit_x,wsw->wsw.postfix_x);
    printf("  height=%hu borderHeight=%d textHeight=%d yOff=%d\n"
      "  up_button_y=%hd digit_y=%hd down_button_y=%hd\n",
      wsw->core.height,borderHeight,textHeight,yOff,
      wsw->wsw.up_button_y,wsw->wsw.digit_y,wsw->wsw.down_button_y);
#endif 

  /* Recreate buttons at the new positions */
    create_buttons(wsw);
#if DEBUG_FLOW
    printf("compute_inner_geo: end\n");
#endif
}

/* Calculates the increments array: 10^(i-point_position) */
static void set_increments(WswWidget wsw)
{
    int i;
    double inc = 1;

#if DEBUG_FLOW
    printf("set_increments:\n");
#endif
  /* Set the starting increment, depending on the digits after the
   * decimal point */
    for(i = 0; i < wsw->wsw.point_position; i++) {
	inc /= 10;
    }
  /* Set the increments, multiplying by 10 each time */
    for(i = 0; i < wsw->wsw.digit_number; i++) {
	wsw->wsw.increments[i] = inc;
	inc *= 10;
    }
}

/* Truncates tha value to what is displayed by the format */
static void truncate_value(WswWidget wsw)
{
    char value_string[FORMAT_SIZE];     /* Danger: Fixed size */
    char number_format[FORMAT_SIZE];

  /* Use the number part of the format */
    strcpy(number_format,wsw->wsw.format + wsw->wsw.prefix_size);
    number_format[wsw->wsw.prefix_size + wsw->wsw.digit_size] = '\0';

    sprintf(value_string, wsw->wsw.format, *(wsw->wsw.value));
    *(wsw->wsw.value) = atof(value_string);

#if 0    
    float value = 0.0;
    int nread = 0;
    nread=sscanf(value_string, wsw->wsw.format, &value);
    if(nread != 1) {
	String params[2];
	unsigned int num_params = 2;
	params[0] = value_string;
	params[1] = wsw->wsw.format;
	
	XtAppErrorMsg(XtWidgetToApplicationContext((Widget)wsw),
	  "wrongParameters", "truncate_value", "WheelSwitchError",
	  "WheelSwitch: Error truncating value \"%s\" using \"%s\"", 
	  params, &num_params);
    } else {
	*(wsw->wsw.value) = value;
    }
#endif    
}

/* Calculates the min and max allowable by the format and settable by
 * buttons.  This may be more limiting than min_value and max_value */
static void compute_format_min_max(WswWidget wsw)
{
    double minmin = 0.0;
    double maxmax = 0.0;
    int i;

#if DEBUG_FLOW
    printf("compute_format_min_max: start:\n");
#endif
    for(i = 0; i < wsw->wsw.digit_number; i++) {
        minmin -= wsw->wsw.increments[i] * 9;
        maxmax += wsw->wsw.increments[i] * 9;
    }
    if(wsw->wsw.format_min_value > wsw->wsw.format_max_value) {
        wsw->wsw.format_min_value = minmin;
        wsw->wsw.format_max_value = maxmax;
    }
    wsw->wsw.format_min_value = max(*wsw->wsw.min_value, minmin);
    wsw->wsw.format_max_value = min(*wsw->wsw.max_value, maxmax);

  /* Set the button visibility  */
    set_button_visibility(wsw);

#if DEBUG_FLOW
    printf("compute_format_min_max: end:\n");
    printf("  min=%g max=%g\n",
      *wsw->wsw.min_value,*wsw->wsw.max_value);
    printf("  format_min=%g format_max=%g\n",
      wsw->wsw.format_min_value,wsw->wsw.format_max_value);
#endif
}

/* Main drawing routine -- draws the widget */
static void draw(XtPointer clientdata, XtIntervalId *id)
{
    Widget widget = (Widget)clientdata;
    WswWidget wsw = (WswWidget)widget;
    char value_string[FORMAT_SIZE];     /* Danger: Fixed size */
    double roff = 0.0;

#if DEBUG_FLOW
    printf("draw: start\n");
#endif
  /* Create buttons if necessary */
  /* KE: Should not be necessary */
    if(wsw->wsw.up_buttons == NULL) {
#if DEBUG_ASSERT
	printf("\n!!! draw: Call create_buttons for wsw=%p\n"
	  "!!!   Not expected\n\n",
	  (void *)wsw);
#endif
        create_buttons(wsw);
    }

  /* Check if keyboard entry is happening */
    if(kbdIsEntering(wsw)) {
      /* Cause entered text to blink */
        clear_all_buttons(wsw);
        if(wsw->wsw.blink_timeout_id != 0)
          XtRemoveTimeOut(wsw->wsw.blink_timeout_id);
        if(wsw->wsw.to_clear == False) {
            sprintf(value_string,wsw->wsw.kbd_format,wsw->wsw.kbd_value);
            wsw->wsw.blink_timeout_id =
	      XtAppAddTimeOut(XtWidgetToApplicationContext(widget),
		500,draw,(XtPointer)wsw);
        } else {
            sprintf(value_string,wsw->wsw.kbd_format," ");
            wsw->wsw.blink_timeout_id =
	      XtAppAddTimeOut(XtWidgetToApplicationContext(widget),
		200,draw,(XtPointer)wsw);
        }
        wsw->wsw.to_clear = ! wsw->wsw.to_clear;
    } else {
      /* Not entering, remove any blinking */
        if(wsw->wsw.blink_timeout_id != 0) {
            XtRemoveTimeOut(wsw->wsw.blink_timeout_id);
        }
        wsw->wsw.blink_timeout_id = 0;

      /* Determine the roundoff to be .1 the smallest increment */
	if(wsw->wsw.digit_number > 0) roff=.1*wsw->wsw.increments[0];
	
      /* Check if in format range */
	if(*(wsw->wsw.value) < wsw->wsw.format_max_value + roff &&
	  *(wsw->wsw.value) > wsw->wsw.format_min_value - roff) {
	  /* Value is in range */
	    sprintf(value_string, wsw->wsw.format, *(wsw->wsw.value));
	} else {
	  /* Value is out of format range, use stars */
	    int i;
	    int imin = wsw->wsw.prefix_size;
	    int imax = wsw->wsw.prefix_size + wsw->wsw.digit_size;
	    int ipoint = imax - wsw->wsw.point_position -1;
	    
	  /* Print zero to establish the string */
	    sprintf(value_string, wsw->wsw.format, 0.0);
	  /* Place a minus if the value is negative */
	    if(*(wsw->wsw.value) < 0) value_string[imin]='-';
	  /* Place stars in the digit positions */
	    for(i=imin+1; i < imax; i++) {
		value_string[i] = '*';
	    }
	  /* Place the decimal point */
	    if(wsw->wsw.point_position) {
		value_string[ipoint] = '.';
	    }
	  /* Be sure the null is there */
	    value_string[wsw->wsw.nb_digit]='\0';
	}
    }
    XDrawImageString(XtDisplay(wsw), XtWindow(wsw),
      wsw->wsw.foreground_GC, wsw->wsw.prefix_x, wsw->wsw.string_base_y,
      value_string, strlen(value_string));

  /* Draw shadows */
    XmeDrawShadows(XtDisplay(wsw),  XtWindow(wsw),
      wsw->manager.top_shadow_GC, wsw->manager.bottom_shadow_GC,
      0, 0, wsw->core.width, wsw->core.height, wsw->manager.shadow_thickness,
      wsw->wsw.shadow_type);
#if DEBUG_FLOW
    printf("draw: end\n");
#endif
}

/* Determines what buttons will show (be managed) */
static void set_button_visibility(WswWidget wsw)
{
    int digit;
    double roff=0.0;
    double value = *wsw->wsw.value;

#if DEBUG_FLOW
    printf("set_button_visibility: start\n");
#endif
#if 0
  /* Don't let the value exceed the max or min allowed */
  /* KE: We don't want to lose information as this does */
    *wsw->wsw.value = min(*wsw->wsw.value, wsw->wsw.format_max_value);
    *wsw->wsw.value = max(*wsw->wsw.value, wsw->wsw.format_min_value);
#endif
  /* Don't let the value set by the buttons exceed the max or min
   * allowed */
    value = min(value, wsw->wsw.format_max_value);
    value = max(value, wsw->wsw.format_min_value);

  /* Manage and unmanage the buttons */
    if(wsw->wsw.up_buttons != NULL) {
      /* Determine the roundoff to be .1 the smallest increment */
	if(wsw->wsw.digit_number > 0) roff=.1*wsw->wsw.increments[0];
      /* Manage depending on whether an increase in that digit will
       * exceed the max_value or not */
        for(digit = 0; digit < wsw->wsw.digit_number; digit++) {
            if(value + wsw->wsw.increments[digit] >
	      wsw->wsw.format_max_value + roff) {
                int i;
                for(i = digit; i < wsw->wsw.digit_number; i++) {
                    XtUnmanageChild(wsw->wsw.up_buttons[i]);
                    _XmUnhighlightBorder(wsw->wsw.up_buttons[i]);
                }
                break;
            } else if(!XtIsManaged(wsw->wsw.up_buttons[digit])) {
                XtManageChild(wsw->wsw.up_buttons[digit]);
                if(wsw->wsw.current_flag) {
                    _XmHighlightBorder(
		      wsw->wsw.up_buttons[wsw->wsw.selected_digit]);
                }
            }
        }
      /* Manage depending on whether a decrease in that digit will
       * exceed the format_min_value or not */
        for(digit = 0; digit < wsw->wsw.digit_number; digit++) {
            if(value - wsw->wsw.increments[digit] <
	      wsw->wsw.format_min_value - roff) {
                int i;
                for(i = digit; i < wsw->wsw.digit_number; i++) {
                    _XmUnhighlightBorder(wsw->wsw.down_buttons[i]);
                    XtUnmanageChild(wsw->wsw.down_buttons[i]);
                }
                break;
            } else if(!XtIsManaged(wsw->wsw.down_buttons[digit])) {
                XtManageChild(wsw->wsw.down_buttons[digit]);
                if(wsw->wsw.current_flag)
                  _XmHighlightBorder(
		    wsw->wsw.down_buttons[wsw->wsw.selected_digit]);
            }
        }
    }
#if DEBUG_FLOW
    printf("set_button_visibility: end\n");
#endif
}

static void select_digit(WswWidget wsw, int digit)
{
    if(wsw->wsw.selected_digit == digit)
      return;
    if(wsw->wsw.current_flag) {
        unhighlight_digit(wsw, wsw->wsw.selected_digit);
        highlight_digit(wsw, digit);
    }
    wsw->wsw.selected_digit = digit;

}

static void highlight_digit(WswWidget wsw, int digit)
{
    if(wsw->wsw.up_buttons != NULL) {
        if(XtIsManaged(wsw->wsw.up_buttons[digit]))
          _XmHighlightBorder(wsw->wsw.up_buttons[digit]);
        if(XtIsManaged(wsw->wsw.down_buttons[digit]))
          _XmHighlightBorder(wsw->wsw.down_buttons[digit]);
    }
}

static void unhighlight_digit(WswWidget wsw, int digit)
{
    if(wsw->wsw.up_buttons != NULL) {
        _XmUnhighlightBorder(wsw->wsw.up_buttons[digit]);
        _XmUnhighlightBorder(wsw->wsw.down_buttons[digit]);
    }
}

static void clear_all_buttons(WswWidget wsw)
{
    int i;
    for(i = 0; i < wsw->wsw.digit_number; i++) {
        XtUnmanageChild(wsw->wsw.down_buttons[i]);
        _XmUnhighlightBorder(wsw->wsw.down_buttons[i]);
        XtUnmanageChild(wsw->wsw.up_buttons[i]);
        _XmUnhighlightBorder(wsw->wsw.up_buttons[i]);
    }
}

static void arm_execute_timer(WswWidget wsw,  XEvent *event)
{
    static WswCallData call_data;

    call_data.widget = wsw;
    call_data.event = event;
    if(wsw->wsw.callback_delay != 0) {
        if(wsw->wsw.callback_timeout_id != 0)
          XtRemoveTimeOut(wsw->wsw.callback_timeout_id);
        wsw->wsw.callback_timeout_id =
	  XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)wsw),
	    (unsigned long)wsw->wsw.callback_delay, executeTimerProc,
	    (XtPointer)&call_data);
    } else {
        executeTimerProc(&call_data, NULL);
    }
}

static void executeTimerProc(XtPointer clientdata, XtIntervalId *id)
{
    WswCallData *call_ev=(WswCallData *)clientdata;
    XmWheelSwitchCallbackStruct call_data;
    if(call_ev->widget->wsw.callback_timeout_id != 0) {
      /* KE: Is this necessary */
        XtRemoveTimeOut(call_ev->widget->wsw.callback_timeout_id);
	call_ev->widget->wsw.callback_timeout_id = 0;
    }
    if(XtHasCallbacks((Widget)call_ev->widget, XmNvalueChangedCallback) ==
      XtCallbackHasSome) {
	call_data.reason = XmCR_VALUE_CHANGED;
	call_data.event = call_ev->event;
	call_data.value = call_ev->widget->wsw.value;
	XtCallCallbacks((Widget)call_ev->widget, XmNvalueChangedCallback,
	  &call_data);
    }
}

static void kbdHandler(Widget widget, XtPointer dummy, XEvent *event,
  Boolean *ctd)
{
    WswWidget wsw = (WswWidget)widget;
    char c,*ptr;
    KeySym keysym;
    
  /* Return if it was not a key press */
    if(event->type != KeyPress) return;
  /* Get the values */
    XLookupString((XKeyPressedEvent*)event,&c,1,&keysym,0);
#if DEBUG_KEYSYM
    printf("Keycode=%d ; KeySym=%x ; c=%d\n",
      ((XKeyPressedEvent*)event)->keycode,keysym,(int)c);
#endif
  /* Switch depending on the keysym.  Note that break causes draw()
   * and return does not */
    switch(keysym) {
    case XK_Return: case XK_KP_Enter:
      /* Not supposed to be used except for keyboard input */
	if(!kbdIsEntering(wsw)) {
	    XBell(XtDisplay(wsw),50);
	    return;
	}
      /* Check if something other than - is entered */
        if(wsw->wsw.kbd_value[1] != '\0') {
	  /* Set the value from the kbd_value */
            *(wsw->wsw.value) = atof(wsw->wsw.kbd_value);
	    /* Reset the kbd_value */
	    kbdReset(wsw);
            set_button_visibility(wsw);
        } else {
	    XBell(XtDisplay(wsw),50);
	    return;
	}
        arm_execute_timer(wsw, &timer_original_event);
        break;

    case XK_Escape:
    case XK_KP_F1: case XK_KP_F2:
    case XK_KP_F3: case XK_KP_F4:
    case XK_KP_Separator:
      /* Not supposed to be used except for keyboard input */
	if(!kbdIsEntering(wsw)) {
	    XBell(XtDisplay(wsw),50);
	    return;
	}
	/* These are a reset */
	kbdReset(wsw);
	set_button_visibility(wsw);
	break;

    case XK_plus:  case XK_KP_Add:
    case XK_minus: case XK_KP_Subtract:
      /* Can be used when there is no keyboard input to initiate it */
      /* Make - toggle -, and + result in space */
        wsw->wsw.kbd_value[0] =
          ((wsw->wsw.kbd_value[0] == '-') || (c == '+'))
          ? ' ': '-' ;
        break;

    case XK_BackSpace: case XK_Delete:
#ifdef ultrix
    case DXK_Remove:
#endif
      /* Not supposed to be used except for keyboard input */
	if(!kbdIsEntering(wsw)) {
	    XBell(XtDisplay(wsw),50);
	    return;
	}
	/* Reset if only - entered */
        if(wsw->wsw.kbd_value[1] == '\0') {
	    kbdReset(wsw);
	} else {
	  /* Move the '\0' back one position */
	    wsw->wsw.kbd_value[strlen(wsw->wsw.kbd_value)-1]='\0';
	}
        break;

    case XK_KP_Decimal: case XK_period:
      /* Can be used when there is no keyboard input to initiate it */
      /* Return if there is no decimal point allowed */
        if(wsw->wsw.point_position == '\0') {
	    XBell(XtDisplay(wsw),50);
	    return;
	}
      /* Find the '\0' */
        ptr = strchr(wsw->wsw.kbd_value,0);
	/* Check if there is a decimal point already or if the buffer
	 * is full */
        if(!strchr(wsw->wsw.kbd_value,'.') &&
	  (int)strlen(wsw->wsw.kbd_value) < wsw->wsw.digit_size) {
	  /* If nothing entered, enter 0 and advance the pointer */
            if(wsw->wsw.kbd_value[1] == '\0') {
		*ptr++='0';
		if((int)strlen(wsw->wsw.kbd_value) >= wsw->wsw.digit_size) {
		    break;
		}
	    }
	  /* Enter the decimal point and set the null */
	    *ptr++='.';
	    *ptr='\0';
	} else {
	    XBell(XtDisplay(wsw),50);
	    return;
	}
        break;

    case XK_KP_0: case XK_0:
    case XK_KP_1: case XK_1:
    case XK_KP_2: case XK_2:
    case XK_KP_3: case XK_3:
    case XK_KP_4: case XK_4:
    case XK_KP_5: case XK_5:
    case XK_KP_6: case XK_6:
    case XK_KP_7: case XK_7:
    case XK_KP_8: case XK_8:
    case XK_KP_9: case XK_9:
      /* Can be used when there is no keyboard input to initiate it */
      /* Check if there is a decimal point entered */
        if((ptr=strchr(wsw->wsw.kbd_value,'.')) != NULL) {
	  /* There is a decimal point.  Check if the digits are
	   * already entered */
            if((int)(strlen(ptr)-1) >= wsw->wsw.point_position) {
		XBell(XtDisplay(wsw),50);
		return;
	    }
        } else {
	  /* There is no decimal point. Check if there is room for
	   * more digits before the decimal point. */
            if((int)(strlen(wsw->wsw.kbd_value)-1) >=
              wsw->wsw.digit_number - wsw->wsw.point_position) {
		XBell(XtDisplay(wsw),50);
		return;
	    }
        }
      /* Find the '\0' */
        ptr = strchr(wsw->wsw.kbd_value,0);
      /* Replace a leading 0 by the number */
        if((wsw->wsw.kbd_value[1] == '0') && (wsw->wsw.kbd_value[2] == '\0')) {
            wsw->wsw.kbd_value[1]= c;
        } else {
	  /* Check if there is enough space */
	    if((int)strlen(wsw->wsw.kbd_value) < wsw->wsw.digit_size) {
		*ptr++=c;
		*ptr='\0';
	    } else {
		XBell(XtDisplay(wsw),50);
		return;
	    }
        }
        break;
        
    case XK_KP_Right: case XK_Right:
    case XK_KP_Left:  case XK_Left:
    case XK_KP_Up:    case XK_Up:
    case XK_KP_Down:  case XK_Down:
      /* Can be used when there is no keyboard input. Don't beep.  */
	return;

    default:
      /* Don't do anything, but beep */
	XBell(XtDisplay(wsw),50);
	return;
    }
    draw((XtPointer)wsw,NULL);
}

static void allocstr(char **str)
{
    char form[100];     /* Danger: Fixed size */
    
    if(*str != NULL){
        strcpy(form,*str);
        *str = (char *)XtCalloc(strlen(form)+1,sizeof(char));
        strcpy(*str,form);
    }
}

static void wsFree(XtPointer *ptr)
{
  if(ptr == NULL || *ptr == NULL) return;
   XtFree(*ptr);
   *ptr = NULL;
}

/* Returns font determined from font_pattern.  Finds the fonts
 * matching font_pattern, sorts them by ascending height, then finds
 * the one that is just less than height, then finds the first one
 * that is less than width if there is one */
static XFontStruct *getFontFromPattern(Display *display,
  char *font_pattern, Dimension width, Dimension height,
  Dimension *width_return, Dimension *height_return)
{
    static char *cur_font_pattern = NULL;
    static char **names = NULL;
    static XFontStruct  *fonts = NULL;
    static int font_count;
    int i,j;
    XFontStruct *font;
    XFontStruct *font_loaded;
    int font_ind=0;
    
#if DEBUG_FONTLEAK || DEBUG_FLOW
    printf("getFontFromPattern:\n"
      "  width=%hd height=%hd\n"
      "  cur_font_pattern=%p names=%p fonts=%p font_count=%d\n"
      "  |%s|\n",
      width,height,
      (void *)cur_font_pattern,(void *)names,
      (void *)fonts,font_count,
      cur_font_pattern?cur_font_pattern:"NULL");
#endif    
    /* Free existing info if the name has changed */
    if(cur_font_pattern != NULL && strcmp(cur_font_pattern, font_pattern)) {
#if DEBUG_FONTLEAK
	printf(" Freeing for font_pattern=%s\n",
	  font_pattern?font_pattern:"NULL");
#endif	
	XtFree(cur_font_pattern);
	if(names && fonts) {
#if DEBUG_FONTLEAK
	    printf(" XFreeFontInfo\n");
#endif
	    XFreeFontInfo(names, fonts, font_count);
	}
	cur_font_pattern = NULL;
	names = NULL;
	fonts = NULL;
	font_count = 0;
    }
  /* Get new info if necessary */
    if(cur_font_pattern == NULL) {
#if DEBUG_FONTLEAK
	printf(" XListFontsWithInfo\n");
#endif
	font_count = 0;
	names = XListFontsWithInfo(display, font_pattern, MAX_FONTS,
	  &font_count, &fonts);
	if(names == NULL || font_count == 0) {
	  /* No application context, use XtErrorMsg */
	    XtErrorMsg("badPattern", "getFontFromPattern", "WheelSwitchError",
	      "WheelSwitch: String to double conversion takes no arguments", 
	      (String *)NULL, (Cardinal *)NULL);
	    return(NULL);
	}
	cur_font_pattern = XtNewString(font_pattern);
	if(cur_font_pattern == NULL) {
	    String params[1];
	    unsigned int num_params = 1;
	    params[0] = font_pattern;
	    
	  /* No application context, use XtErrorMsg */
	    XtErrorMsg("badPattern", "getFontFromPattern", "WheelSwitchError",
	      "WheelSwitch:  error allocating font pattern \"%s\"",
	      params, &num_params);
	    return(NULL);
	}
      /* Sort by increasing ascent */
	for(i=0; i< font_count; i++) {
	    for(j=i+1; j < font_count; j++) {
		if(fonts[i].ascent > fonts[j].ascent) {
		    XFontStruct font_buffer;
		    char *name_buffer;
		    font_buffer = fonts[i];
		    fonts[i] = fonts[j];
		    fonts[j] = font_buffer;
		    name_buffer = names[i];
		    names[i] = names[j];
		    names[j] = name_buffer;
		}
	    }
	}
#if DEBUG_FONTLEAK || DEBUG_FLOW
    printf("  cur_font_pattern=%p names=%p fonts=%p font_count=%d\n"
      "  |%s|\n",
      (void *)cur_font_pattern,(void *)names,
      (void *)fonts,font_count,
      cur_font_pattern?cur_font_pattern:"NULL");
#endif    
    }
    /* Search by ascending height until height fits ascent+2*descent */
    font_ind = 0;
    for(i=0; i < font_count; i++) {
#if DEBUG_FONTS
	printf("%2d ascent=%u descent=%u max_bounds.width=%u crit=%u height=%hu\n",
	  i,fonts[i].ascent,fonts[i].descent,
	  fonts[i].max_bounds.width,
	  fonts[i].ascent + 2*fonts[i].descent,height);
#endif
	if((fonts[i].ascent + 2*fonts[i].descent) > height) {
	    break;
	} else {
	    font_ind = i;
	}
    }
#if DEBUG_FONTS
    printf("font_ind=%d\n",font_ind);
#endif
    /* Then by descending width until max_bounds.width fits width */
    for(j = font_ind; j > 0; j--) {
#if DEBUG_FONTS
	printf("%2d ascent=%u descent=%u max_bounds.width=%u crit=%u width=%hu\n",
	  j,fonts[j].ascent,fonts[j].descent,
	  fonts[j].max_bounds.width,
	  fonts[j].max_bounds.width,width);
#endif
	if(fonts[j].max_bounds.width <= width) {
	    break;
	}
    }
#if DEBUG_FONTS
    printf("j=%d\n",j);
#endif
    
    font_ind = j;
    font = fonts + font_ind;
    *height_return = (Dimension)(font->ascent + 2*font->descent);
    *width_return = font->max_bounds.width;
    font_loaded = XLoadQueryFont(display, names[font_ind]);

#if DEBUG_FONTS
    printf("getFontFromPattern: font_ind=%d font_count=%d\n  %s\n"
      "  width=%hu->%hu height=%hu->%hu\n",
      font_ind,font_count,cur_font_pattern,
      width,*width_return,height,*height_return);
#endif    
#if DEBUG_FONTLEAK
    printf("Ending getFontFromPattern: font_loaded=%p\n",
      (void *)font_loaded);
#endif
      return(font_loaded);
}

/* Returns font determined from font_list.  !!!Finds the fonts
 * matching font_pattern, sorts them by ascending height, then finds
 * the one that is just less than height, then finds the first one
 * that is less than width if there is one */
static XFontStruct *getFontFromList(Display *display,
  XmFontList font_list, Dimension width, Dimension height,
  Dimension *width_return, Dimension *height_return)
{
    XFontStruct *font_return = NULL;
    XFontStruct *font = NULL;
    XmFontContext context;
    XmFontListEntry entry;
#if 0    
    XmStringCharSet charset;
#endif    
    Boolean status;
    int font_count=0;
    
#if DEBUG_FONTLEAK || DEBUG_FLOW
    printf("getFontFromList:\n"
      "  width=%hd height=%hd\n",
      width,height);
#endif    

    status=XmFontListInitFontContext(&context, font_list);
#if 0
    XmFontListGetNextFont(context, &charset, &font);
#else
    entry = XmFontListNextEntry(context);
    while(entry != NULL) {
	XmFontType type_return;

	font = XmFontListEntryGetFont(entry, &type_return);
	if(type_return == XmFONT_IS_FONT) {
#if DEBUG_FONTS
	    printf("  %2d ascent=%u descent=%u max_bounds.width=%u "
	      "crit=%u height=%hu\n",
	      font_count,font->ascent,font->descent,
	      font->max_bounds.width,
	      font->ascent + 2*font->descent,height);
#endif


#if DEBUG_FONTS
	    printf("  font=%p fid=%p\n",
	      (void *)font,(void *)font->fid);
#endif    
	    
	} else {
	    XtErrorMsg("wrongParameters","getFontFromList",
	      "WheelSwitchError",
	      "WheelSwitch: Cannot handle font lists inside font lists", 
	      (String *)NULL, (Cardinal *)NULL);
	}

      /* Get the next entry */
	font_count++;
	entry = XmFontListNextEntry(context);
    }
    
#endif
    XmFontListFreeFontContext(context);
    
  /* Use the last one for now */
    *height_return = (Dimension)(font->ascent + 2*font->descent);
    *width_return = font->max_bounds.width;
    font_return = font;

#if DEBUG_FONTS
    printf("  Returned: font=%p fid=%p\n"
      "  width=%hu->%hu height=%hu->%hu\n",
      (void *)font_return,(void *)font_return->fid,
      width,*width_return,height,*height_return);
#endif

    return font_return;
}

/* Callbacks, timer procs, and event handlers */

static void upButtonArmCallback(Widget button, XtPointer userdata,
  XtPointer calldata)
{
    int digit = (int)userdata;
    XmAnyCallbackStruct *callback_data = (XmAnyCallbackStruct *)calldata;
    WswWidget wsw;
    Widget widget;

    if(callback_data->event->xany.type == ButtonPress) {
	widget = XtParent(button); 
        wsw = (WswWidget)widget;
        if(_XmGetFocusPolicy(widget) == XmEXPLICIT 
          && widget != _XmGetTabGroup(widget)) {
	    (void)XmProcessTraversal(widget, XmTRAVERSE_CURRENT);
	}
        select_digit(wsw, digit);
        *wsw->wsw.value += wsw->wsw.increments[digit];
	truncate_value(wsw);
        if(*wsw->wsw.value >= wsw->wsw.format_max_value) {
	    arm_execute_timer(wsw, &timer_original_event);
	}
        set_button_visibility(wsw);
        draw((XtPointer)wsw,NULL);
        wsw->wsw.pending_timeout_id =
	  XtAppAddTimeOut(XtWidgetToApplicationContext(widget),
	    (unsigned long)max(470, wsw->wsw.repeat_interval),
	    upButtonTimerProc, (XtPointer)wsw);
        timer_original_event = *(callback_data->event);
    }
}

static void downButtonArmCallback(Widget button, XtPointer userdata,
  XtPointer calldata)
{
    int digit = (int)userdata;
    XmAnyCallbackStruct *callback_data = (XmAnyCallbackStruct *)calldata;
    WswWidget wsw;
    Widget widget;

    if(callback_data->event->xany.type == ButtonPress) {
	widget = XtParent(button); 
        wsw = (WswWidget)widget;
        if(_XmGetFocusPolicy(widget) == XmEXPLICIT 
          && widget != _XmGetTabGroup(widget)) {
	    (void)XmProcessTraversal(widget, XmTRAVERSE_CURRENT);
	}
        select_digit(wsw, digit);
        *wsw->wsw.value -= wsw->wsw.increments[digit];
	truncate_value(wsw);
        if(*wsw->wsw.value <= wsw->wsw.format_min_value) {
	    arm_execute_timer(wsw, &timer_original_event);
	}
        set_button_visibility(wsw);
        draw((XtPointer)widget,NULL);
        wsw->wsw.pending_timeout_id =
	  XtAppAddTimeOut(XtWidgetToApplicationContext(widget),
	    (unsigned long)max(470, wsw->wsw.repeat_interval),
	    downButtonTimerProc, (XtPointer)wsw);
        timer_original_event = *(callback_data->event);
    }
}

/* Timer callbacks */

static void upButtonTimerProc(XtPointer userdata, XtIntervalId *id)
{
    WswWidget wsw = (WswWidget)userdata;
    if(wsw->wsw.pending_timeout_id == *id) {
        *wsw->wsw.value += wsw->wsw.increments[wsw->wsw.selected_digit];
	truncate_value(wsw);
        if(*wsw->wsw.value >= wsw->wsw.format_max_value) {
	    arm_execute_timer(wsw, &timer_original_event);
	}
        set_button_visibility(wsw);
        draw((XtPointer)wsw,NULL);
        if(XtIsManaged(wsw->wsw.up_buttons[wsw->wsw.selected_digit])) {
            wsw->wsw.pending_timeout_id =
	      XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)wsw),
		(unsigned long)max(30, wsw->wsw.repeat_interval),
		upButtonTimerProc, (XtPointer)wsw);
        } else {
            wsw->wsw.pending_timeout_id = 0;
        }
    } else {
        wsw->wsw.pending_timeout_id = 0;
    }
}

static void downButtonTimerProc(XtPointer userdata,  XtIntervalId *id)
{
    WswWidget wsw = (WswWidget)userdata;
    if(wsw->wsw.pending_timeout_id == *id) {
        *wsw->wsw.value -= wsw->wsw.increments[wsw->wsw.selected_digit];
	truncate_value(wsw);
        if(*wsw->wsw.value <= wsw->wsw.format_min_value) {
	    arm_execute_timer(wsw, &timer_original_event);
	}
        set_button_visibility(wsw);
        draw((XtPointer)wsw,NULL);
        if(XtIsManaged(wsw->wsw.down_buttons[wsw->wsw.selected_digit])) {
            wsw->wsw.pending_timeout_id =
	      XtAppAddTimeOut(XtWidgetToApplicationContext((Widget)wsw),
		(unsigned long)max(30, wsw->wsw.repeat_interval),
		downButtonTimerProc, (XtPointer)wsw);
        } else {
            wsw->wsw.pending_timeout_id = 0;
        }
    } else {
        wsw->wsw.pending_timeout_id = 0;
    }
}

static void btn1UpProc(Widget widget, XEvent *event, String *params,
  Cardinal *num_params)
{
    WswWidget wsw = (WswWidget)widget;
    if(wsw->wsw.pending_timeout_id != 0) {
        XtRemoveTimeOut(wsw->wsw.pending_timeout_id);
        wsw->wsw.pending_timeout_id = 0;
    }
    arm_execute_timer(wsw, &timer_original_event);
}

static void btn1DownProc(Widget widget,  XEvent *event, String *params,
  Cardinal *num_params)
{
    WswWidget wsw = (WswWidget)widget;
    XmGadget g;
    wsw->wsw.current_flag = True;
    g = (XmGadget)_XmInputInGadget(widget, event->xbutton.x, event->xbutton.y);
    if(g != NULL) {
        _XmDispatchGadgetInput(g, event, XmARM_EVENT);
        wsw->manager.active_child = (Widget)g;
    }
    if(_XmGetFocusPolicy(widget) == XmEXPLICIT 
      && widget != _XmGetTabGroup(widget))
      (void)XmProcessTraversal(widget, XmTRAVERSE_CURRENT);
    if(wsw->wsw.up_buttons != NULL)
      wsw->manager.active_child = (Widget)wsw->wsw.up_buttons[0];
}

static void upArrowkeyProc(Widget widget, XEvent *event, String *params,
  Cardinal *num_params)
{
    WswWidget wsw =(WswWidget)widget;
    if(kbdIsEntering(wsw)) return;
  /* Don't do anything if the arrow is not managed */
    if(!XtIsManaged(wsw->wsw.up_buttons[wsw->wsw.selected_digit])) return;
    *wsw->wsw.value += wsw->wsw.increments[wsw->wsw.selected_digit];
    truncate_value(wsw);
    set_button_visibility(wsw);
    draw((XtPointer)wsw,NULL);
    wsw->wsw.pending_timeout_id =
      XtAppAddTimeOut(XtWidgetToApplicationContext(widget),
	(unsigned long)max(470, wsw->wsw.repeat_interval),
	upButtonTimerProc, (XtPointer)wsw);
    timer_original_event = *event;
}

static void downArrowkeyProc(Widget widget, XEvent *event, String *params,
  Cardinal *num_params)
{
    WswWidget wsw = (WswWidget)widget;
    if(kbdIsEntering(wsw)) return;
  /* Don't do anything if the arrow is not managed */
    if(!XtIsManaged(wsw->wsw.down_buttons[wsw->wsw.selected_digit])) return;
    *wsw->wsw.value -= wsw->wsw.increments[wsw->wsw.selected_digit];
    truncate_value(wsw);
    set_button_visibility(wsw);
    draw((XtPointer)wsw,NULL);
    wsw->wsw.pending_timeout_id =
      XtAppAddTimeOut(XtWidgetToApplicationContext(widget),
	(unsigned long)max(470, wsw->wsw.repeat_interval),
	downButtonTimerProc, (XtPointer)wsw);
    timer_original_event = *event;
}

static void releaseArrowkeyProc(Widget widget,  XEvent *event, String *params,
  Cardinal *num_params)
{
    WswWidget wsw = (WswWidget)widget;
    if(kbdIsEntering(wsw)) return;
    if(wsw->wsw.pending_timeout_id != 0) {
        XtRemoveTimeOut(wsw->wsw.pending_timeout_id);
        wsw->wsw.pending_timeout_id = 0;
    }
    arm_execute_timer(wsw, &timer_original_event);
}

static void rightArrowkeyProc(Widget widget, XEvent *event, String *params,
  Cardinal *num_params)
{
    WswWidget wsw = (WswWidget)widget;
    if(kbdIsEntering(wsw)) return;
    if(wsw->wsw.selected_digit > 0)
      select_digit(wsw, wsw->wsw.selected_digit - 1);
}

static void leftArrowkeyProc(Widget widget, XEvent *event, String *params,
  Cardinal *num_params)
{
    WswWidget wsw = (WswWidget)widget;
    if(kbdIsEntering(wsw)) return;
    if(wsw->wsw.selected_digit < wsw->wsw.digit_number - 1)
      select_digit(wsw, wsw->wsw.selected_digit + 1);
}

/* Focus callback and procs */

static void lostFocusProc(Widget widget,  XEvent *event, String *params,
  Cardinal *num_params)
{
    WswWidget wsw = (WswWidget)widget;
    Boolean newhasfocus = wsw->wsw.current_flag;
  /* Check if it came from SendEvent */
  /* KE: Not sure why SendEvent is important */
    if(event->xfocus.send_event) {
      /* Focus is lost */
	newhasfocus = False;
    }
  /* Check if the focus has changed */
    if(newhasfocus != wsw->wsw.current_flag) {
        wsw->wsw.current_flag = newhasfocus;
        if(wsw->core.sensitive) {
            unhighlight_digit(wsw, wsw->wsw.selected_digit);
#if 0	    
	  /* KE: Bad: Doing this tends to leave the workstation with
	   * repeat off if the user has repeat on and would override
	   * the users choice to have it off */
           XAutoRepeatOn(XtDisplay(wsw));
#endif	    
        }
      /* Reset any keyboard input */
        if(kbdIsEntering(wsw)) {
	    kbdReset(wsw);
            set_button_visibility(wsw);
            draw((XtPointer)wsw,NULL);
        }
    }
}

static void getFocusProc(Widget widget, XEvent *event, String *params,
  Cardinal *num_params)
{
    WswWidget wsw = (WswWidget)widget;
    Boolean newhasfocus = wsw->wsw.current_flag;
  /* Check if it came from SendEvent */
  /* KE: Not sure why SendEvent is important */
    if(event->xfocus.send_event) {
      /* Focus is gained */
	newhasfocus = True;
    }
  /* Check if the focus has changed */
    if(newhasfocus != wsw->wsw.current_flag) {
        wsw->wsw.current_flag = newhasfocus;
        if(wsw->core.sensitive) {
            highlight_digit(wsw, wsw->wsw.selected_digit);
#if 0	    
	  /* KE: Bad: Doing this tends to leave the workstation with
	   * repeat off if the user has repeat on and would override
	   * the users choice to have it off */
            XAutoRepeatOff(XtDisplay(wsw));
#endif	    
        }
    }
}

static Boolean kbdIsEntering(WswWidget wsw)
{
    if(wsw && wsw->wsw.kbd_value &&
      wsw->wsw.kbd_value[0] == ' ' &&
      wsw->wsw.kbd_value[1] == '\0') {
	return False;
    } else {
	return True;
    }
}

static void kbdReset(WswWidget wsw)
{
    if(wsw && wsw->wsw.kbd_value) {
      wsw->wsw.kbd_value[0] = ' ';
      wsw->wsw.kbd_value[1] = '\0';
    }
}

/* Frees the font if necessary and set the XFontStruct pointer to
 * NULL.  XFontStruct's obtained from XLoadQueryFont need to be freed.
 * XFontStruct's obtained from an XmFontList should not be freed. */
static void freeFont(WswWidget wsw)
{
    if(wsw->wsw.font == NULL) return;

  /* See if it came from XLoadQueryFont */
    if(!wsw->wsw.user_font) {
	XFreeFont(XtDisplay(wsw),wsw->wsw.font);
    }

  /* Set the pointer to zero in both cases */
    wsw->wsw.font = NULL;
}
