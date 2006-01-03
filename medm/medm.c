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

#define DEBUG_SYNC 0
#define DEBUG_RADIO_BUTTONS 0
#define DEBUG_DEFINITIONS 0
#define DEBUG_ALLEVENTS 0
#define DEBUG_EVENTS 0
#define DEBUG_STDC 0
#define DEBUG_WIN32_LEAKS 0
#define DEBUG_FILE_RENAME 0
#define DEBUG_SAVE_ALL 0
#define DEBUG_VERSION 0
#define DEBUG_PROP 0
#define DEBUG_ERRORHANDLER 0

#define ALLOCATE_STORAGE
#include "medm.h"
#include <Xm/RepType.h>

#ifdef EDITRES
#include <X11/Xmu/Editres.h>
#endif

#if DEBUG_WIN32_LEAKS
# ifdef WIN32
#  ifdef _DEBUG
#   include <crtdbg.h>
#  endif
# endif
#endif

#ifdef WIN32
/* WIN32 does not have unistd.h and does not define the following constants */
#define F_OK 00
#define W_OK 02
#define R_OK 04
#include <direct.h>     /* for getcwd (usually in sys/parm.h or unistd.h) */
#include <io.h>         /* for access, chmod  (usually in unistd.h) */
#endif

#include <signal.h>
#ifdef WIN32
/* Define signals that Windows does not define */
#define SIGQUIT SIGTERM
#define SIGBUS SIGILL
#endif

#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <Xm/MwmUtil.h>
#include <X11/IntrinsicP.h>

/* For Xrt/Graph property editor */
#ifdef XRTGRAPH
#if XRT_VERSION > 2
#ifdef XRT_EXTENSIONS
#include <XrtGraph.h>
#include <XrtGraphProp.h>
#endif
#endif
#endif

#include "icon25.xpm"

#define HOT_SPOT_WIDTH 24

#define N_MAX_MENU_ELES 20
#define N_MAIN_MENU_ELES 5

#define N_FILE_MENU_ELES 8
#define FILE_BTN_POSN 0

#define GRID_OK     0
#define GRID_CANCEL 1
#define GRID_HELP   2

#define EDIT_OBJECT_BTN           0
#define EDIT_CUT_BTN              1
#define EDIT_COPY_BTN             2
#define EDIT_PASTE_BTN            3
#define EDIT_RAISE_BTN            4
#define EDIT_LOWER_BTN            5

#define EDIT_GROUP_BTN            6
#define EDIT_UNGROUP_BTN          7

#define EDIT_UNSELECT_BTN         8
#define EDIT_SELECT_ALL_BTN       9
#define EDIT_SELECT_DISPLAY_BTN  10
#define EDIT_REFRESH_BTN         11
#define EDIT_HELP_BTN            12
#define EDIT_UNDO_BTN            13
#define EDIT_FIND_BTN            14

#define VIEW_MESSAGE_WINDOW_BTN  1
#define VIEW_STATUS_WINDOW_BTN   2
#define VIEW_DISPLAY_LIST_BTN    3

#define GRID_SPACING_BTN 0
#define GRID_ON_BTN      1
#define GRID_SNAP_BTN    2

#define CENTER_HORIZ_BTN 0
#define CENTER_VERT_BTN  1
#define CENTER_BOTH_BTN  2

#define ORIENT_HORIZ_BTN 0
#define ORIENT_VERT_BTN  1
#define ORIENT_CW_BTN    2
#define ORIENT_CCW_BTN   3

#define SIZE_SAME_BTN 0
#define SIZE_TEXT_BTN 1

#define ALIGN_HORIZ_LEFT_BTN    0
#define ALIGN_HORIZ_CENTER_BTN  1
#define ALIGN_HORIZ_RIGHT_BTN   2
#define ALIGN_VERT_TOP_BTN      3
#define ALIGN_VERT_CENTER_BTN   4
#define ALIGN_VERT_BOTTOM_BTN   5
#define ALIGN_POS_TO_GRID_BTN   6
#define ALIGN_EDGE_TO_GRID_BTN  7

#define SPACE_HORIZ_BTN 0
#define SPACE_VERT_BTN  1
#define SPACE_2D_BTN    2

#ifdef EXTENDED_INTERFACE
#define N_PALETTES_MENU_ELES 4
#else
#define N_PALETTES_MENU_ELES 3
#endif

#define PALETTES_OBJECT_BTN   0
#define PALETTES_RESOURCE_BTN 1
#define PALETTES_COLOR_BTN    2
#ifdef EXTENDED_INTERFACE
#define PALETTES_CHANNEL_BTN  3
#endif

#define N_HELP_MENU_ELES 7

#define HELP_OVERVIEW_BTN     0
#define HELP_CONTENTS_BTN     1
#define HELP_OBJECTS_BTN      2
#define HELP_EDIT_BTN         3
#define HELP_NEW_BTN          4
#define HELP_TECH_SUPPORT_BTN 5
#define HELP_ON_HELP_BTN      6
#define HELP_ON_VERSION_BTN   7

/* Function prototypes */

static void createCursors(void);
static void createMain(void);
static void fileMenuDialogCallback(Widget,XtPointer,XtPointer);
static void editMenuSimpleCallback(Widget,XtPointer,XtPointer);
static void palettesMenuSimpleCallback(Widget,XtPointer,XtPointer);
static void helpMenuSimpleCallback(Widget,XtPointer,XtPointer);
static void alignMenuSimpleCallback(Widget,XtPointer,XtPointer);
static void centerMenuSimpleCallback(Widget,XtPointer,XtPointer);
static void orientMenuSimpleCallback(Widget,XtPointer,XtPointer);
static void sizeMenuSimpleCallback(Widget,XtPointer,XtPointer);
static void spaceMenuSimpleCallback(Widget, XtPointer, XtPointer);
static void gridMenuSimpleCallback(Widget, XtPointer, XtPointer);
static void viewMenuSimpleCallback(Widget,XtPointer,XtPointer);

Widget mainFilePDM, mainHelpPDM, mainMB;
static Widget gridDlg = 0;
static int medmUseBigCursor = 0;

#ifdef VMS
void vmsTrimVersionNumber (char *fileName);
#endif


#ifdef __TED__
void GetWorkSpaceList(Widget w);
#endif
static menuEntry_t graphicsObjectMenu[] = {
    { "Text",        &xmPushButtonGadgetClass, 'T', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_Text,  NULL},
    { "Rectangle",   &xmPushButtonGadgetClass, 'G', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_Rectangle,  NULL},
    { "Line",        &xmPushButtonGadgetClass, 'i', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_Line,  NULL},
    { "Polygon",     &xmPushButtonGadgetClass, 'P', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_Polygon,  NULL},
    { "Polyline",    &xmPushButtonGadgetClass, 'L', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_Polyline,  NULL},
    { "Oval",        &xmPushButtonGadgetClass, 'O', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_Oval,  NULL},
    { "Arc",         &xmPushButtonGadgetClass, 'A', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_Arc,  NULL},
    { "Image",       &xmPushButtonGadgetClass, 'I', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_Image,  NULL},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
};

static menuEntry_t monitorsObjectMenu[] = {
    { "Text Monitor",  &xmPushButtonGadgetClass, 'T', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_TextUpdate,  NULL},
    { "Meter",         &xmPushButtonGadgetClass, 'M', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_Meter,  NULL},
    { "Bar Monitor",   &xmPushButtonGadgetClass, 'B', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_Bar,  NULL},
    { "Byte Monitor",  &xmPushButtonGadgetClass, 'y', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_Byte,  NULL},
    { "Scale Monitor", &xmPushButtonGadgetClass, 'I', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_Indicator,  NULL},
    { "Strip Chart",   &xmPushButtonGadgetClass, 'S', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_StripChart,  NULL},
    { "Cartesian Plot",&xmPushButtonGadgetClass, 'C', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_CartesianPlot,  NULL},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
};

static menuEntry_t controllersObjectMenu[] = {
    { "Text Entry",  &xmPushButtonGadgetClass, 'T', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_TextEntry,  NULL},
    { "Choice Button",  &xmPushButtonGadgetClass, 'C', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_ChoiceButton,  NULL},
    { "Menu",           &xmPushButtonGadgetClass, 'M', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_Menu,  NULL},
    { "Slider",       &xmPushButtonGadgetClass, 'V', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_Valuator,  NULL},
    { "Message Button", &xmPushButtonGadgetClass, 'B', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_MessageButton,  NULL},
    { "Related Display",&xmPushButtonGadgetClass, 'R', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_RelatedDisplay,  NULL},
    { "Shell Command",  &xmPushButtonGadgetClass, 'S', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_ShellCommand,  NULL},
    { "Wheel Switch",  &xmPushButtonGadgetClass, 'W', NULL, NULL, NULL,
      objectMenuCallback, (XtPointer) DL_WheelSwitch,  NULL},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
};

static menuEntry_t editAlignMenu[] = {
    { "Left",              &xmPushButtonGadgetClass, 'L', NULL, NULL, NULL,
      alignMenuSimpleCallback, (XtPointer) ALIGN_HORIZ_LEFT_BTN,  NULL},
    { "Horizontal Center", &xmPushButtonGadgetClass, 'H', NULL, NULL, NULL,
      alignMenuSimpleCallback, (XtPointer) ALIGN_HORIZ_CENTER_BTN,  NULL},
    { "Right",             &xmPushButtonGadgetClass, 'R', NULL, NULL, NULL,
      alignMenuSimpleCallback, (XtPointer) ALIGN_HORIZ_RIGHT_BTN,  NULL},
    { "Top",               &xmPushButtonGadgetClass, 'T', NULL, NULL, NULL,
      alignMenuSimpleCallback, (XtPointer) ALIGN_VERT_TOP_BTN,  NULL},
    { "Vertical Center",   &xmPushButtonGadgetClass, 'V', NULL, NULL, NULL,
      alignMenuSimpleCallback, (XtPointer) ALIGN_VERT_CENTER_BTN,  NULL},
    { "Bottom",            &xmPushButtonGadgetClass, 'B', NULL, NULL, NULL,
      alignMenuSimpleCallback, (XtPointer) ALIGN_VERT_BOTTOM_BTN,  NULL},
    { "Position to Grid",  &xmPushButtonGadgetClass, 'P', NULL, NULL, NULL,
      alignMenuSimpleCallback, (XtPointer) ALIGN_POS_TO_GRID_BTN,  NULL},
    { "Edges to Grid",     &xmPushButtonGadgetClass, 'E', NULL, NULL, NULL,
      alignMenuSimpleCallback, (XtPointer) ALIGN_EDGE_TO_GRID_BTN,  NULL},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
};

static menuEntry_t editSizeMenu[] = {
    { "Same Size",        &xmPushButtonGadgetClass, 'S', NULL, NULL, NULL,
      sizeMenuSimpleCallback, (XtPointer) SIZE_SAME_BTN,  NULL},
    { "Text to Contents", &xmPushButtonGadgetClass, 'T', NULL, NULL, NULL,
      sizeMenuSimpleCallback, (XtPointer) SIZE_TEXT_BTN,  NULL},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
};

static menuEntry_t editCenterMenu[] = {
    { "Horizontally in Display", &xmPushButtonGadgetClass, 'H', NULL, NULL, NULL,
      centerMenuSimpleCallback, (XtPointer) CENTER_HORIZ_BTN,  NULL},
    { "Vertically in Display",   &xmPushButtonGadgetClass, 'V', NULL, NULL, NULL,
      centerMenuSimpleCallback, (XtPointer) CENTER_VERT_BTN,  NULL},
    { "Both",                    &xmPushButtonGadgetClass, 'B', NULL, NULL, NULL,
      centerMenuSimpleCallback, (XtPointer) CENTER_BOTH_BTN,  NULL},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
};

static menuEntry_t editOrientMenu[] = {
    { "Flip Horizontally", &xmPushButtonGadgetClass, 'H', NULL, NULL, NULL,
      orientMenuSimpleCallback, (XtPointer) ORIENT_HORIZ_BTN,  NULL},
    { "Flip Vertically",   &xmPushButtonGadgetClass, 'V', NULL, NULL, NULL,
      orientMenuSimpleCallback, (XtPointer) ORIENT_VERT_BTN,  NULL},
    { "Rotate Clockwise", &xmPushButtonGadgetClass, 'R', NULL, NULL, NULL,
      orientMenuSimpleCallback, (XtPointer) ORIENT_CW_BTN,  NULL},
    { "Rotate Counterclockwise",   &xmPushButtonGadgetClass, 'C', NULL, NULL, NULL,
      orientMenuSimpleCallback, (XtPointer) ORIENT_CCW_BTN,  NULL},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
};

static menuEntry_t editSpaceMenu[] = {
    { "Horizontal", &xmPushButtonGadgetClass, 'H', NULL, NULL, NULL,
      spaceMenuSimpleCallback, (XtPointer) SPACE_HORIZ_BTN,  NULL},
    { "Vertical",   &xmPushButtonGadgetClass, 'V', NULL, NULL, NULL,
      spaceMenuSimpleCallback, (XtPointer) SPACE_VERT_BTN,  NULL},
    { "2-D",        &xmPushButtonGadgetClass, 'D', NULL, NULL, NULL,
      spaceMenuSimpleCallback, (XtPointer) SPACE_2D_BTN,  NULL},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
};

static menuEntry_t editGridMenu[] = {
    { "Toggle Show Grid",    &xmPushButtonGadgetClass, 'G', NULL, NULL, NULL,
      gridMenuSimpleCallback, (XtPointer) GRID_ON_BTN,  NULL},
    { "Toggle Snap To Grid", &xmPushButtonGadgetClass, 'S', NULL, NULL, NULL,
      gridMenuSimpleCallback, (XtPointer) GRID_SNAP_BTN,  NULL},
    { "Grid Spacing...", &xmPushButtonGadgetClass, 'c', NULL, NULL, NULL,
      gridMenuSimpleCallback, (XtPointer) GRID_SPACING_BTN,  NULL},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
};

static menuEntry_t editObjectMenu[] = {
    { "Graphics",   &xmCascadeButtonGadgetClass,'G', NULL, NULL, NULL,
      NULL,        NULL,                     graphicsObjectMenu},
    { "Monitors",   &xmCascadeButtonGadgetClass,'M', NULL, NULL, NULL,
      NULL,        NULL,                     monitorsObjectMenu},
    { "Controllers",&xmCascadeButtonGadgetClass,'C', NULL, NULL, NULL,
      NULL,        NULL,                     controllersObjectMenu},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
};

static menuEntry_t editMenu[] = {
    { "Undo",      &xmPushButtonGadgetClass, 'U', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_UNDO_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0',
      NULL,        NULL,                     NULL},
    { "Cut",       &xmPushButtonGadgetClass, 't',  "Shift<Key>osfDelete", "Shift+Del", NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_CUT_BTN,  NULL},
    { "Copy",      &xmPushButtonGadgetClass, 'C', "Ctrl<Key>osfInsert",  "Ctrl+Ins",   NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_COPY_BTN,  NULL},
    { "Paste" ,    &xmPushButtonGadgetClass, 'P', "Shift<Key>osfInsert", "Shift+Ins",  NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_PASTE_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0',  NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Raise",     &xmPushButtonGadgetClass, 'R', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_RAISE_BTN,  NULL},
    { "Lower",     &xmPushButtonGadgetClass, 'L', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_LOWER_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0',NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Group",     &xmPushButtonGadgetClass, 'G', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_GROUP_BTN,  NULL},
    { "Ungroup",   &xmPushButtonGadgetClass, 'o', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_UNGROUP_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0',NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Align",     &xmCascadeButtonGadgetClass, 'A', NULL, NULL, NULL,
      NULL,        NULL,                     editAlignMenu},
    { "Space Evenly", &xmCascadeButtonGadgetClass, 'v', NULL, NULL, NULL,
      NULL,        NULL,                     editSpaceMenu},
    { "Center",    &xmCascadeButtonGadgetClass, 'e', NULL, NULL, NULL,
      NULL,        NULL,                     editCenterMenu},
    { "Orient",    &xmCascadeButtonGadgetClass, 'i', NULL, NULL, NULL,
      NULL,        NULL,                     editOrientMenu},
    { "Size", &xmPushButtonGadgetClass, 'z', NULL, NULL, NULL,
      NULL,        NULL,                     editSizeMenu},
    { "Grid",      &xmPushButtonGadgetClass, 'd', NULL, NULL, NULL,
      NULL,        NULL,                     editGridMenu},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Unselect",  &xmPushButtonGadgetClass, 'n', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_UNSELECT_BTN,  NULL},
    { "Select All",&xmPushButtonGadgetClass, 'S', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_SELECT_ALL_BTN,  NULL},
    { "Select Display",&xmPushButtonGadgetClass, '\0', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_SELECT_DISPLAY_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Find Outliers", &xmPushButtonGadgetClass, 'F', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_FIND_BTN,  NULL},
    { "Refresh",   &xmPushButtonGadgetClass, 'h', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_REFRESH_BTN,  NULL},
    { "Edit Summary...", &xmPushButtonGadgetClass, 'y', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_HELP_BTN,  NULL},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
};

static menuEntry_t editModeMenu[] = {
    { "Object",    &xmCascadeButtonGadgetClass, 'j', NULL, NULL, NULL,
      NULL,        NULL,                     editObjectMenu},
    { "Undo",      &xmPushButtonGadgetClass, 'U', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_UNDO_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0',
      NULL,        NULL,                     NULL},
    { "Cut",       &xmPushButtonGadgetClass, 't', "Shift<Key>osfDelete", "Shift+Del", NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_CUT_BTN,  NULL},
    { "Copy",      &xmPushButtonGadgetClass, 'C', "Ctrl<Key>osfInsert",  "Ctrl+Ins",   NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_COPY_BTN,  NULL},
    { "Paste" ,    &xmPushButtonGadgetClass, 'P', "Shift<Key>osfInsert", "Shift+Ins",  NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_PASTE_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0',  NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Raise",     &xmPushButtonGadgetClass, 'R', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_RAISE_BTN,  NULL},
    { "Lower",     &xmPushButtonGadgetClass, 'L', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_LOWER_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0',NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Group",     &xmPushButtonGadgetClass, 'G', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_GROUP_BTN,  NULL},
    { "Ungroup",   &xmPushButtonGadgetClass, 'o', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_UNGROUP_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0',NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Align",     &xmCascadeButtonGadgetClass, 'A', NULL, NULL, NULL,
      NULL,        NULL,                     editAlignMenu},
    { "Space Evenly", &xmCascadeButtonGadgetClass, 'v', NULL, NULL, NULL,
      NULL,        NULL,                     editSpaceMenu},
    { "Center",    &xmCascadeButtonGadgetClass, 'e', NULL, NULL, NULL,
      NULL,        NULL,                     editCenterMenu},
    { "Orient",    &xmCascadeButtonGadgetClass, 'i', NULL, NULL, NULL,
      NULL,        NULL,                     editOrientMenu},
    { "Size", &xmPushButtonGadgetClass, 'z', NULL, NULL, NULL,
      NULL,        NULL,                     editSizeMenu},
    { "Grid",      &xmPushButtonGadgetClass, 'd', NULL, NULL, NULL,
      NULL,        NULL,                     editGridMenu},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Unselect",  &xmPushButtonGadgetClass, 'n', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_UNSELECT_BTN,  NULL},
    { "Select All",&xmPushButtonGadgetClass, 'S', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_SELECT_ALL_BTN,  NULL},
    { "Select Display",&xmPushButtonGadgetClass, '\0', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_SELECT_DISPLAY_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Find Outliers", &xmPushButtonGadgetClass, 'F', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_FIND_BTN,  NULL},
    { "Refresh",   &xmPushButtonGadgetClass, 'h', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_REFRESH_BTN,  NULL},
    { "Edit Summary...", &xmPushButtonGadgetClass, 'y', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_HELP_BTN,  NULL},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
};

static menuEntry_t fileMenu[] = {
    { "New",       &xmPushButtonGadgetClass, 'N', NULL,         NULL, NULL,
      mainFileMenuSimpleCallback, (XtPointer) MAIN_FILE_NEW_BTN, NULL},
    { "Open...",   &xmPushButtonGadgetClass, 'O', "Ctrl<Key>O", "Ctrl+O", NULL,
      mainFileMenuSimpleCallback, (XtPointer) MAIN_FILE_OPEN_BTN, NULL},
    { "Save",      &xmPushButtonGadgetClass, 'S', "Ctrl<Key>S", "Ctrl+S", NULL,
      mainFileMenuSimpleCallback, (XtPointer) MAIN_FILE_SAVE_BTN, NULL},
    { "Save All",  &xmPushButtonGadgetClass, 'l', "Ctrl<Key>L", NULL, NULL,
      mainFileMenuSimpleCallback, (XtPointer) MAIN_FILE_SAVE_ALL_BTN, NULL},
    { "Save As...",&xmPushButtonGadgetClass, 'A', NULL,         NULL, NULL,
      mainFileMenuSimpleCallback, (XtPointer) MAIN_FILE_SAVE_AS_BTN, NULL},
    { "Close",     &xmPushButtonGadgetClass, 'C', NULL,         NULL, NULL,
      mainFileMenuSimpleCallback, (XtPointer) MAIN_FILE_CLOSE_BTN, NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL,        NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Print Setup...",  &xmPushButtonGadgetClass, 'u', NULL, NULL, NULL,
      mainFileMenuSimpleCallback, (XtPointer) MAIN_FILE_PRINT_SETUP_BTN, NULL},
    { "Print",  &xmPushButtonGadgetClass, 'P', NULL,            NULL, NULL,
      mainFileMenuSimpleCallback, (XtPointer) MAIN_FILE_PRINT_BTN, NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL,        NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Exit",      &xmPushButtonGadgetClass, 'x', "Ctrl<Key>X", "Ctrl+X", NULL,
      mainFileMenuSimpleCallback, (XtPointer) MAIN_FILE_EXIT_BTN, NULL},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
};

static menuEntry_t viewMenu[] = {
    { "Message Window", &xmPushButtonGadgetClass, 'M', NULL, NULL, NULL,
      viewMenuSimpleCallback, (XtPointer) VIEW_MESSAGE_WINDOW_BTN, NULL},
    { "Statistics Window", &xmPushButtonGadgetClass, 'S', NULL, NULL, NULL,
      viewMenuSimpleCallback, (XtPointer) VIEW_STATUS_WINDOW_BTN, NULL},
    { "Display List", &xmPushButtonGadgetClass, 'D', NULL, NULL, NULL,
      viewMenuSimpleCallback, (XtPointer) VIEW_DISPLAY_LIST_BTN, NULL},
    { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
};

static menuEntry_t palettesMenu[] = {
    { "Object",   &xmPushButtonGadgetClass, 'O', NULL, NULL, NULL,
      palettesMenuSimpleCallback, (XtPointer) PALETTES_OBJECT_BTN, NULL},
    { "Resource", &xmPushButtonGadgetClass, 'R', NULL, NULL, NULL,
      palettesMenuSimpleCallback, (XtPointer) PALETTES_RESOURCE_BTN, NULL},
    { "Color",    &xmPushButtonGadgetClass, 'C', NULL, NULL, NULL,
      palettesMenuSimpleCallback, (XtPointer) PALETTES_COLOR_BTN, NULL},
#ifdef EXTENDED_INTERFACE
    "Channel",  &xmPushButtonGadgetClass, 'h', NULL, NULL, NULL,
    palettesMenuSimpleCallback, (XtPointer) PALETTES_CHANNEL_BTN, NULL},
#endif
      { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
      };

static menuEntry_t helpMenu[] = {
{ "Overview",  &xmPushButtonGadgetClass, 'O', "Ctrl<key>H", NULL, NULL,
  helpMenuSimpleCallback, (XtPointer) HELP_OVERVIEW_BTN, NULL},
{ "Contents",&xmPushButtonGadgetClass, 'C', NULL, NULL, NULL,
    helpMenuSimpleCallback, (XtPointer) HELP_CONTENTS_BTN, NULL},
{ "Object Index", &xmPushButtonGadgetClass, 'I', NULL, NULL, NULL,
    helpMenuSimpleCallback, (XtPointer) HELP_OBJECTS_BTN, NULL},
{ "Editing",  &xmPushButtonGadgetClass, 'E', NULL, NULL, NULL,
    helpMenuSimpleCallback, (XtPointer) HELP_EDIT_BTN, NULL},
{ "New Features",    &xmPushButtonGadgetClass, 'N', NULL, NULL, NULL,
    helpMenuSimpleCallback, (XtPointer) HELP_NEW_BTN, NULL},
{ "Technical Support",  &xmPushButtonGadgetClass, 'T', NULL, NULL, NULL,
    helpMenuSimpleCallback, (XtPointer) HELP_TECH_SUPPORT_BTN, NULL},
{ "On Help",  &xmPushButtonGadgetClass, 'H', NULL, NULL, NULL,
    helpMenuSimpleCallback, (XtPointer) HELP_ON_HELP_BTN, NULL},
{ "On Version",  &xmPushButtonGadgetClass, 'V', NULL, NULL, NULL,
    helpMenuSimpleCallback, (XtPointer) HELP_ON_VERSION_BTN, NULL},
  { NULL, NULL, 0, NULL, NULL, NULL, NULL, NULL, NULL },
  };

/* last mouse position of the display before popup the menu */
XButtonPressedEvent lastEvent;

/* one lonesome function prototype without a home */
#ifdef __cplusplus
extern "C" {
#endif
    extern Widget createAndPopupProductDescriptionShell(
      XtAppContext appContext, Widget topLevelShell,
      char *name, XmFontList nameFontList,
      Pixmap namePixmap,
      char *description, XmFontList descriptionFontList,
      char *versionInfo, char *developedAt, XmFontList otherFontList,
      int background, int foreground, int seconds);
#ifdef __cplusplus
	   }
#endif

/* Global variables */

static Widget modeRB, modeEditTB, modeExecTB;
static Widget openFSD;
static Widget mainEditPDM, mainViewPDM, mainPalettesPDM;
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
  /* Main window */
    "Medm.geometry: -5+5",
    "Medm.mainMW.mainMB*fontList: 8x13",
    "Medm.mainMW*frameLabel*fontList: 8x13",
    "Medm.mainMW*modeRB*XmToggleButton.indicatorOn: false",
    "Medm.mainMW*modeRB*XmToggleButton.shadowThickness: 2",
    "Medm.mainMW*modeRB*XmToggleButton.highlightThickness: 1",
    "Medm.mainMW*openFSD.dialogTitle: Open",
    "Medm.mainMW*helpMessageBox.dialogTitle: Help",
    "Medm.mainMW*editHelpMessageBox.dialogTitle: Edit Summary",
    "Medm.mainMW*saveAsPD.dialogTitle: Save As...",
    "Medm.mainMW*saveAsPD.selectionLabelString: \
Name of file in which to save display:",
    "Medm.mainMW*saveAsPD.okLabelString: Save",
    "Medm.mainMW*saveAsPD.cancelLabelString: Cancel",
    "Medm.mainMW*printerSetupPD.selectionLabelString: Printer Name:",
    "Medm.mainMW*gridPD.selectionLabelString: Grid Spacing:",
#ifdef PROMPT_TO_EXIT
    "Medm.mainMW*exitQD.dialogTitle: Exit",
    "Medm.mainMW*exitQD.messageString: Do you really want to Exit?",
    "Medm.mainMW*exitQD.okLabelString: Yes",
    "Medm.mainMW*exitQD.cancelLabelString: No",
#endif
    "Medm.mainMW*XmRowColumn.tearOffModel: XmTEAR_OFF_ENABLED",
  /* Object palette */
    "Medm.objectS.geometry: -5+137",
    "Medm*objectMW.objectMB*fontList: 8x13",
    "Medm*objectMW*XmLabel.marginWidth: 0",
  /* Bubble help */
    "Medm*bubbleHelpD*background: #f9da3c",
    "Medm*bubbleHelpD*borderColor: black",
    "Medm*bubbleHelpD*borderWidth: 1",
  /* Display area */
    "Medm*displayDA*XmRowColumn.tearOffModel: XmTEAR_OFF_ENABLED",
  /* override some normal functions in displays for operational safety reasons */
    "Medm*displayDA*XmPushButton.translations: #override  <Key>space: ",
    "Medm*displayDA*XmPushButtonGadget.translations: #override <Key>space: ",
    "Medm*displayDA*XmToggleButton.translations: #override  <Key>space: ",
    "Medm*displayDA*XmToggleButtonGadget.translations: #override <Key>space: ",
    "Medm*displayDA*radioBox*translations: #override <Key>space: ",
  /* Color palette */
    "Medm.colorS.geometry: -5-5",
    "Medm*colorMW.colorMB*fontList: 8x13",
    "Medm*colorMW*XmLabel.marginWidth: 0",
    "Medm*colorMW*colorPB.width: 20",
    "Medm*colorMW*colorPB.height: 20",
  /* Resource palette */
    "Medm.resourceS.geometry: -5+304",
    "Medm*resourceMW.width: 375",
    "Medm*resourceMW.height: 515",
    "Medm*resourceMW.shadowThickness: 0",
    "Medm*resourceMW.resourceMB*fontList: 8x13",
    "Medm*resourceMW*localLabel.marginRight: 8",
    "Medm*resourceMW*messageF.rowColumn.spacing: 10",
    "Medm*resourceMW*messageF.resourceElementTypeLabel.fontList: 8x13",
    "Medm*resourceMW*messageF.verticalSpacing: 6",
    "Medm*resourceMW*messageF.horizontalSpacing: 3",
    "Medm*resourceMW*messageF.shadowType: XmSHADOW_IN",
#ifdef UNNECESSARY
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
    "Medm*resourceMW*imageNameFSD.dialogTitle: Image File Name",
    "Medm*resourceMW*imageNameFSD.form.shadowThickness: 0",
    "Medm*resourceMW*imageNameFSD.form.typeLabel.labelString: Image Type:",
    "Medm*resourceMW*imageNameFSD.form.typeLabel.marginTop: 4",
    "Medm*resourceMW*imageNameFSD.form.frame.radioBox.orientation: XmHORIZONTAL",
    "Medm*resourceMW*imageNameFSD.form.frame.radioBox.numColumns: 1",
    "Medm*resourceMW*imageNameFSD.form.frame.radioBox*shadowThickness: 0",
    "Medm*resourceMW*imageNameFSD*XmToggleButton.indicatorOn: True",
    "Medm*resourceMW*imageNameFSD*XmToggleButton.labelType: XmString",
#endif
#ifdef EXTENDED_INTERFACE
  /* Channel palette */
    "Medm*channelMW.width: 140",
    "Medm*channelMW.channelMB*fontList: 8x13",
    "Medm*channelMW*XmLabel.marginWidth: 0",
#endif
  /* Message window */
    "Medm.errorMsgS.x: 10",
    "Medm.errorMsgS.y: 10",
  /* Strip chart dialog */
    "Medm*scForm.marginWidth: 10",
    "Medm*scForm.marginHeight: 10",
    "Medm*scForm.scActionArea.marginHeight: 5",
#if 0
    "Medm*scForm.scMatrix.marginHeight: 0",
    "Medm*scForm.scMatrix.columnRC.marginHeight: 5",
#endif
  /* Medm widget resource specifications */
    "Medm*warningDialog*foreground: white",
    "Medm*warningDialog*background: red",
    "Medm*warningDialog.dialogTitle: Warning",
    "Medm*Indicator.AxisWidth: 3",
    "Medm*Bar.AxisWidth: 3",
    "Medm*BarGraph.BorderWidth: 0",
    "Medm*Meter.BorderWidth: 0",
    "Medm*Indicator.BorderWidth: 0",

#ifdef XRTGRAPH
#if XRT_VERSION > 2
#ifdef XRT_EXTENSIONS
  /* XRTGraph Property Editor */
    "Medm*.PropEdit_shell*.background:                  White",
    "Medm*.PropEdit_shell*.foreground:                  Black",
    "Medm*.PropEdit_shell.width:                        630",
    "Medm*.PropEdit_shell.height:                       390",
    "Medm*.PropEdit_shell*.XmXrtOutliner*background:    White",
    "Medm*.PropEdit_shell*.XmXrtOutliner*foreground:    Black",
    "Medm*.PropEdit_shell*.XmXrtIntField.background:    White",
    "Medm*.PropEdit_shell*.XmXrtIntField.foreground:    Black",
    "Medm*.PropEdit_shell*.XmXrtStringField.background: White",
    "Medm*.PropEdit_shell*.XmXrtStringField.foreground: Black",
    "Medm*.PropEdit_shell*.XmXrtFloatField.background:  White",
    "Medm*.PropEdit_shell*.XmXrtFloatField.foreground:  Black",
    "Medm*.PropEdit_shell*.XmXrtDateField.background:   White",
    "Medm*.PropEdit_shell*.XmXrtDateField.foreground:   Black",
#endif
#endif
#endif

#ifdef SCIPLOT
  /* Sciplot */
    "Medm*cartesianPlot.traversalOn:        False",
    "Medm*cartesianPlot.borderWidth:        0",
    "Medm*cartesianPlot.highlightThickness: 0",
    "Medm*cartesianPlot.titleMargin:        5",
    "Medm*cartesianPlot.showLegend:         False",
    "Medm*cartesianPlot.drawMajor:          False",
    "Medm*cartesianPlot.drawMinor:          False",
#endif

#if 0
  /* Keep CDE from recoloring. (Medm* doesn't work here) This fix is
     for Solaris with CDE and may no longer be needed, but shouldn't
     hurt.  */
#ifndef WIN32
  /* This makes Drag and Drop not work for Exceed */
    "*useColorObj: False",
#endif
#endif

    NULL,
};

typedef enum {EDIT,EXECUTE,HELP,VERSION} opMode_t;
typedef enum {ATTACH,CLEANUP,LOCAL} medmMode_t;
typedef enum {FIXED_FONT,SCALABLE_FONT} fontStyle_t;

typedef struct {
    opMode_t opMode;
    medmMode_t medmMode;
    fontStyle_t fontStyle;
    Boolean privateCmap;
    char *macroString;
    char displayFont[256];          /* !!!! warning : fixed array size */
    char *displayName;
    char *displayGeometry;
    int  fileCnt;
    char **fileList;
} request_t;

/* KE: Not used */
void requestDestroy(request_t *request) {
    if(request) {
	if(request->macroString) free(request->macroString);
/*      if(request->displayFont) free(request->displayFont);     */
	if(request->displayName) free(request->displayName);
	if(request->displayGeometry) free(request->displayGeometry);
	if(request->fileList) {
	    int i;
	    for(i=0; i < request->fileCnt; i++) {
		if(request->fileList[i]) free(request->fileList[i]);
	    }
	    free((char *)request->fileList);
	    request->fileList = NULL;
	}
	free((char *)request);
	request = NULL;
    }
}

#if DEBUG_STDC
#ifndef __STDC__
#error __STDC__ is undefined
#else
#if __STDC__ == 0
#error __STDC__=0
#elif __STDC__ == 1
#error __STDC__=1
#else
#error __STDC__ is defined and not 0 or 1
#endif
#endif
#endif

request_t * parseCommandLine(int argc, char *argv[]) {
    int i;
    int argsUsed = 0;
    int fileEntryTableSize = 0;
    request_t *request = NULL;
    char fullPathName[PATH_MAX];

    request = (request_t *)malloc(sizeof(request_t));
    if(request == NULL) return request;
    request->opMode = EDIT;
    request->medmMode = LOCAL;
    request->fontStyle = MEDM_DEFAULT_FONT_STYLE;
    request->privateCmap = False;
    request->macroString = NULL;
    strcpy(request->displayFont,MEDM_DEFAULT_DISPLAY_FONT);
    request->displayName = NULL;
    request->displayGeometry = NULL;
    request->fileCnt = 0;
    request->fileList = NULL;

  /* Parse the switches */
    for(i = 1; i < argc; i++) {
	if(!strcmp(argv[i],"-x")) {
	    request->opMode = EXECUTE;
	    argsUsed = i;
	} else if(!strcmp(argv[i],"-help") || !strcmp(argv[i],"-h") ||
	  !strcmp(argv[i],"-?")) {
	    request->opMode = HELP;
	    argsUsed = i;
	} else if(!strcmp(argv[i],"-version")) {
	    request->opMode = VERSION;
	    argsUsed = i;
	} else if(!strcmp(argv[i],"-local")) {
	    request->medmMode = LOCAL;
	    argsUsed = i;
	} else if(!strcmp(argv[i],"-attach")) {
	    request->medmMode = ATTACH;
	    argsUsed = i;
	} else if(!strcmp(argv[i],"-cleanup")) {
	    request->medmMode = CLEANUP;
	    argsUsed = i;
	} else if(!strcmp(argv[i],"-cmap")) {
	    request->privateCmap = True;
	    argsUsed = i;
	} else if(!strcmp(argv[i],"-macro")) {
	    char *tmp;
	    argsUsed = i;
	    tmp = (((i+1) < argc) ? argv[++i] : NULL);
	    if(tmp) {
		argsUsed = i;
		request->macroString = STRDUP(tmp);
	      /* since parameter of form   -macro "a=b,c=d,..."  replace '"' with ' ' */
		if(request->macroString != NULL) {
		    int len;
		    if(request->macroString[0] == '"') request->macroString[0] = ' ';
		    len = strlen(request->macroString) - 1;
		    if(request->macroString[len] == '"') request->macroString[len] = ' ';
		}
	    }
	} else if(!strcmp(argv[i],"-displayFont")) {
	    char *tmp;
	    argsUsed = i;
	    tmp = (((i+1) < argc) ? argv[++i] : NULL);
	    if(tmp) {
		argsUsed = i;
		strcpy(request->displayFont,tmp);
#if 0
	      /* KE: The following code is useless.  We could change
                 == to !=, however, request->fontStyle eventually is
                 only used to set the windowPropertyAtom.  Currently,
                 it will be set to whatever the default is for
                 request->fontStyle.  Moreover, if the displayFont is
                 an X font specification, this method will not be able
                 to determine if it is scalable or not. Further, it
                 probably doesn't make a difference.  */
		if(request->displayFont[0] == '\0') {
		    if(!strcmp(request->displayFont,FONT_ALIASES_STRING))
		      request->fontStyle = FIXED_FONT;
		    else if(!strcmp(request->displayFont,DEFAULT_SCALABLE_STRING))
		      request->fontStyle = SCALABLE_FONT;
		}
#endif
	    }
	} else if(!strcmp(argv[i],"-display")) {
	  /* (Not trapped by X because this routine is called first) */
	    char *tmp;
	    argsUsed = i;
	    tmp = (((i+1) < argc) ? argv[++i] : NULL);
	    if(tmp) {
		argsUsed = i;
		request->displayName = STRDUP(tmp);
	    }
	} else if((!strcmp(argv[i],"-displayGeometry")) || (!strcmp(argv[i],"-dg"))) {
	    char *tmp;
	    argsUsed = i;
	    tmp = (((i+1) < argc) ? argv[++i] : NULL);
	    if(tmp) {
		argsUsed = i;
		request->displayGeometry = STRDUP(tmp);
	    }
	} else if(!strcmp(argv[i],"-bigMousePointer")) {
	    medmUseBigCursor = 1;

	    argsUsed = i;
	} else if(!strcmp(argv[i],"-noMsg")) {
	    medmRaiseMessageWindow = 0;

	    argsUsed = i;
	} else if(argv[i][0] == '-') {
	    medmPrintf(1,"\nInvalid option: %s\n",argv[i]);
	    request->opMode = HELP;
	    argsUsed = i;
	}

    }

  /* Parse the display name */
    for(i = argsUsed+1; i < argc; i++) {
	Boolean canAccess;
	char    *fileStr;

	canAccess = False;

      /* Check the next argument, if doesn't match the suffix, continue */
      /* KE: May not be a suffix (junk.adlebrained.txt fits) */
	fileStr = argv[i];
	if(strstr(fileStr,DISPLAY_FILE_ASCII_SUFFIX) == NULL) {
	    medmPrintf(1,"\nFile has wrong suffix: %s\n",fileStr);
	    continue;
	}
	if(strlen(fileStr) > (size_t)(PATH_MAX-1)) {
	    medmPrintf(1,"\nFile name too long: %s\n",fileStr);
	    continue;
	}

      /* Mark the fullPathName as an empty string */
	fullPathName[0] = '\0';

      /* Found string with right suffix - presume it's a valid display name */
	canAccess = !access(fileStr,R_OK|F_OK);
	if(canAccess) {
	    int status;

	  /* Found the file.  Convert to a full path. */
	    status = convertNameToFullPath(fileStr, fullPathName, PATH_MAX);
	    if(!status) canAccess = False;
	} else {
	  /* Not found, try with directory specified in the environment */
	    char *dir = NULL;
	    char name[PATH_MAX];
	    int startPos;

	    dir = getenv("EPICS_DISPLAY_PATH");
	    if(dir != NULL) {
		startPos = 0;
		while(extractStringBetweenColons(dir,name,startPos,&startPos)) {
		    if(strlen(name)+strlen(fileStr) <
		      (size_t)(PATH_MAX - 1)) {
			strcpy(fullPathName,name);
#ifndef VMS
			strcat(fullPathName,MEDM_DIR_DELIMITER_STRING);
#endif
			strcat(fullPathName,fileStr);
			canAccess = !access(fullPathName,R_OK|F_OK);
			if(canAccess) break;
		    }
		}
	    }
	}
	if(canAccess) {
	  /* build the request */
	    if(fileEntryTableSize == 0) {
		fileEntryTableSize =  10;
		request->fileList =
		  (char **)malloc(fileEntryTableSize*sizeof(char *));
	    }
	    if(fileEntryTableSize > request->fileCnt) {
		fileEntryTableSize *= 2;
#if defined(__cplusplus) && !defined(__GNUG__)
		request->fileList =
		  (char **)realloc((malloc_t)request->fileList,
		    fileEntryTableSize);
#else
		request->fileList =
		  (char **)realloc(request->fileList,fileEntryTableSize);
#endif
	    }
	    if(request->fileList) {
		request->fileList[request->fileCnt] = STRDUP(fullPathName);
		request->fileCnt++;
	    }
	} else {
	    medmPrintf(1,"\nCannot access file: %s\n",fileStr);
	}
    }
    return request;
}

/********************************************
 **************** Callbacks *****************
 ********************************************/
static void gridDlgCb(Widget w, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *cdi=currentDisplayInfo;

    UNREFERENCED(cbs);

    switch ((int)cd) {
    case GRID_OK:
	if(cdi) {
	    char *gridVal;
	    XmString xmString;

	    XtVaGetValues(w,XmNtextString,&xmString,NULL);
	  /* Use XmStringGetLtoR because it handles multiple lines */
	    XmStringGetLtoR(xmString,XmFONTLIST_DEFAULT_TAG,&gridVal);
	    cdi->grid->gridSpacing = atoi(gridVal);
	    if(cdi->grid->gridSpacing < 2) cdi->grid->gridSpacing = 2;
	    XtFree(gridVal);
	    XmStringFree(xmString);
	    XtUnmanageChild(w);
	    updateGlobalResourceBundleAndResourcePalette(False);
	    dmTraverseNonWidgetsInDisplayList(cdi);
	    medmMarkDisplayBeingEdited(cdi);
	}
	break;
    case GRID_CANCEL:
	XtUnmanageChild(w);
	break;
    case GRID_HELP:
	callBrowser(medmHelpPath,"#Grid");
	break;
    }
}

static void viewMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    int buttonNumber = (int) cd;

    UNREFERENCED(cbs);

    switch(buttonNumber) {
    case VIEW_MESSAGE_WINDOW_BTN:
	  errMsgDlgCreateDlg(True);
	break;
    case VIEW_STATUS_WINDOW_BTN:
	medmCreateCAStudyDlg();
	break;
    case VIEW_DISPLAY_LIST_BTN:
        popupDisplayListDlg();
	break;
    default :
	break;
    }
}

static void editMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *cdi=currentDisplayInfo;
    int buttonNumber = (int)cd;
    int fromMain;
    Widget parent;

    UNREFERENCED(cbs);

  /* simply return if no current display */
    if(cdi == NULL) return;

  /* (MDA) could be smarter about this too, and not do whole traversals...*/

    switch(buttonNumber) {
    case EDIT_OBJECT_BTN:
	break;

    case EDIT_UNDO_BTN:
	restoreUndoInfo(cdi);
	break;

    case EDIT_CUT_BTN:
	copySelectedElementsIntoClipboard();
	deleteElementsInDisplay(cdi);
	medmMarkDisplayBeingEdited(cdi);
	break;

    case EDIT_COPY_BTN:
	copySelectedElementsIntoClipboard();
	break;

    case EDIT_PASTE_BTN:
      /* See if this came from the main window edit menu
       *   (either attached or torn off) */
	parent = w;
	fromMain = 1;
	while(parent != mainShell) {
	    parent = XtParent(parent);
	    if(parent == mainMB) {
	      /* Definitely from main window */
		break;
	    } else if(parent == cdi->drawingArea) {
	      /* Definitely not from main window */
		fromMain = 0;
		break;
	    }
	}
	if(fromMain) {
	  /* Pushed on main edit menu, need to determine which display */
	    Widget widget;
	    XEvent event;

	    if(displayInfoListHead->next != displayInfoListTail) {
	      /* More than one display, query user */
		widget = XmTrackingEvent(mainShell,pasteCursor,False,&event);
		if(widget) {
		    cdi = currentDisplayInfo =
		      dmGetDisplayInfoFromWidget(widget);
		}
	    }
	}
	copyElementsIntoDisplay();
	medmMarkDisplayBeingEdited(cdi);
	break;

    case EDIT_RAISE_BTN:
	raiseSelectedElements();
	medmMarkDisplayBeingEdited(cdi);
	break;

    case EDIT_LOWER_BTN:
	lowerSelectedElements();
	medmMarkDisplayBeingEdited(cdi);
	break;

    case EDIT_GROUP_BTN:
	groupObjects();
	medmMarkDisplayBeingEdited(cdi);
	break;

    case EDIT_UNGROUP_BTN:
	ungroupSelectedElements();
	medmMarkDisplayBeingEdited(cdi);
	break;

    case EDIT_UNSELECT_BTN:
	unselectElementsInDisplay();
	break;

    case EDIT_SELECT_ALL_BTN:
	selectAllElementsInDisplay();
	break;

    case EDIT_SELECT_DISPLAY_BTN:
	selectDisplay();
	break;

    case EDIT_REFRESH_BTN:
	refreshDisplay(cdi);
	break;

    case EDIT_FIND_BTN:
	findOutliers();
	break;

    case EDIT_HELP_BTN:
    {
      /* Use XmStringGetLtoR because it handles multiple lines.  Use
         two strings to avoid the minimum length (509) ISO C89 is
         required to support */
	XmString xmString1=XmStringCreateLtoR(
	  "             EDIT Operations Summary\n"
	  "\n"
	  "Pointer in Create Mode (Crosshair Cursor)\n"
	  "=========================================\n"
	  "Btn1         Drag to create object.\n"
	  "Btn3         Popup edit menu.\n"
	  "\n"
	  "Pointer in Select Mode (Pointing Hand Cursor)\n"
	  "=============================================\n"
	  "Btn1         Select objects.\n"
	  "             Vertex edit for Polygon and Polyline.\n"
	  "               (Shift afterward constrains direction.)\n",
	  XmFONTLIST_DEFAULT_TAG);
	XmString xmString2=XmStringCreateLtoR(
	  "Shift-Btn1   Add or remove from selected objects.\n"
	  "Ctrl-Btn1    Cycle selection through overlapping objects.\n"
	  "Btn2         Move objects.\n"
	  "Ctrl-Btn2    Resize objects.\n"
	  "Btn3         Popup edit menu.\n"
	  "\n"
	  "Keyboard\n"
	  "========\n"
	  "Arrow Key    Move selected objects.\n"
	  "Shift-Arrow  Move selected objects.\n"
	  "Ctrl-Arrow   Resize selected objects.\n",
	  XmFONTLIST_DEFAULT_TAG);
	XmString xmString=XmStringConcat(xmString1,xmString2);
	Arg args[20];
	int nargs;

	nargs=0;
	XtSetArg(args[nargs],XmNmessageString,xmString); nargs++;
	XtSetValues(editHelpMessageBox,args,nargs);
	XmStringFree(xmString1);
	XmStringFree(xmString2);
	XmStringFree(xmString);
	XtPopup(editHelpS,XtGrabNone);
	break;
    }
    }
}

static void alignMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *cdi=currentDisplayInfo;
    int buttonNumber = (int) cd;

    UNREFERENCED(cbs);

    switch(buttonNumber) {
    case ALIGN_HORIZ_LEFT_BTN:
	alignSelectedElements(ALIGN_HORIZ_LEFT);
	medmMarkDisplayBeingEdited(cdi);
	break;
    case ALIGN_HORIZ_CENTER_BTN:
	alignSelectedElements(ALIGN_HORIZ_CENTER);
	medmMarkDisplayBeingEdited(cdi);
	break;
    case ALIGN_HORIZ_RIGHT_BTN:
	alignSelectedElements(ALIGN_HORIZ_RIGHT);
	medmMarkDisplayBeingEdited(cdi);
	break;
    case ALIGN_VERT_TOP_BTN:
	alignSelectedElements(ALIGN_VERT_TOP);
	medmMarkDisplayBeingEdited(cdi);
	break;
    case ALIGN_VERT_CENTER_BTN:
	alignSelectedElements(ALIGN_VERT_CENTER);
	medmMarkDisplayBeingEdited(cdi);
	break;
    case ALIGN_VERT_BOTTOM_BTN:
	alignSelectedElements(ALIGN_VERT_BOTTOM);
	medmMarkDisplayBeingEdited(cdi);
	break;
    case ALIGN_POS_TO_GRID_BTN:
	alignSelectedElementsToGrid(False);
	medmMarkDisplayBeingEdited(cdi);
	break;
    case ALIGN_EDGE_TO_GRID_BTN:
	alignSelectedElementsToGrid(True);
	medmMarkDisplayBeingEdited(cdi);
	break;
    }
}

static void sizeMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *cdi=currentDisplayInfo;
    int buttonNumber = (int) cd;

    UNREFERENCED(cbs);

    switch(buttonNumber) {
    case SIZE_SAME_BTN:
	equalSizeSelectedElements();
	medmMarkDisplayBeingEdited(cdi);
	break;

    case SIZE_TEXT_BTN:
	sizeSelectedTextElements();
	medmMarkDisplayBeingEdited(cdi);
	break;
    }
}

static void centerMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *cdi=currentDisplayInfo;
    int buttonNumber = (int) cd;

    UNREFERENCED(cbs);

    switch(buttonNumber) {
    case CENTER_HORIZ_BTN:
	centerSelectedElements(ALIGN_HORIZ_CENTER);
	medmMarkDisplayBeingEdited(cdi);
	break;
    case CENTER_VERT_BTN:
	centerSelectedElements(ALIGN_VERT_CENTER);
	medmMarkDisplayBeingEdited(cdi);
	break;
    case CENTER_BOTH_BTN:
	centerSelectedElements(ALIGN_HORIZ_CENTER);
	centerSelectedElements(ALIGN_VERT_CENTER);
	medmMarkDisplayBeingEdited(cdi);
	break;
    }
}

static void orientMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *cdi=currentDisplayInfo;
    int buttonNumber = (int) cd;

    UNREFERENCED(cbs);

    switch(buttonNumber) {
    case ORIENT_HORIZ_BTN:
	orientSelectedElements(ORIENT_HORIZ);
	medmMarkDisplayBeingEdited(cdi);
	break;
    case ORIENT_VERT_BTN:
	orientSelectedElements(ORIENT_VERT);
	medmMarkDisplayBeingEdited(cdi);
	break;
    case ORIENT_CW_BTN:
	orientSelectedElements(ORIENT_CW);
	medmMarkDisplayBeingEdited(cdi);
	break;
    case ORIENT_CCW_BTN:
	orientSelectedElements(ORIENT_CCW);
	medmMarkDisplayBeingEdited(cdi);
	break;
    }
}

static void spaceMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *cdi=currentDisplayInfo;
    int buttonNumber = (int) cd;

    UNREFERENCED(cbs);

    switch(buttonNumber) {
      /* reuse the TextAlign values here */
    case SPACE_HORIZ_BTN:
	spaceSelectedElements(SPACE_HORIZ);
	medmMarkDisplayBeingEdited(cdi);
	break;
    case SPACE_VERT_BTN:
	spaceSelectedElements(SPACE_VERT);
	medmMarkDisplayBeingEdited(cdi);
	break;
    case SPACE_2D_BTN:
	spaceSelectedElements2D();
	medmMarkDisplayBeingEdited(cdi);
	break;
    }
}

static void gridMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *cdi=currentDisplayInfo;
    int buttonNumber = (int) cd;

    UNREFERENCED(cbs);

    switch(buttonNumber) {
      /* reuse the TextAlign values here */
    case GRID_ON_BTN:
	cdi->grid->gridOn=!cdi->grid->gridOn;
	updateGlobalResourceBundleAndResourcePalette(False);
	dmTraverseNonWidgetsInDisplayList(cdi);
	medmMarkDisplayBeingEdited(cdi);
	break;

    case GRID_SNAP_BTN:
	cdi->grid->snapToGrid=!cdi->grid->snapToGrid;
	updateGlobalResourceBundleAndResourcePalette(False);
	dmTraverseNonWidgetsInDisplayList(cdi);
	medmMarkDisplayBeingEdited(cdi);
	break;

    case GRID_SPACING_BTN:
	XDefineCursor(display,XtWindow(mainShell),watchCursor);
	XFlush(display);
	if(!gridDlg) {
	    int n;
	    Arg args[4];
	    XmString xmString;
	    char label[1024];

	    sprintf(label,"%d",cdi->grid->gridSpacing);
	    xmString = XmStringCreateLocalized(label);
	    n = 0;
	    XtSetArg(args[n],XmNtitle,"Grid Spacing"); n++;
	    XtSetArg(args[n],XmNtextString,xmString); n++;
	    XtSetArg(args[n],XmNdefaultPosition,False); n++;
	    gridDlg = XmCreatePromptDialog(XtParent(mainEditPDM),"gridPD",args,n);
	    XtAddCallback(gridDlg,XmNokCallback,gridDlgCb,
	      (XtPointer)GRID_OK);
	    XtAddCallback(gridDlg,XmNcancelCallback,
	      gridDlgCb,(XtPointer)GRID_CANCEL);
	    XtAddCallback(gridDlg,XmNhelpCallback,
	      gridDlgCb,(XtPointer)GRID_HELP);
	} else {
	    int n;
	    Arg args[4];
	    XmString xmString;
	    char label[1024];

	    sprintf(label,"%d",cdi->grid->gridSpacing);
	    xmString = XmStringCreateLocalized(label);
	    n = 0;
	    XtSetArg(args[n],XmNtextString,xmString); n++;
	    XtSetValues(gridDlg,args,n);
	    XmStringFree(xmString);
	}
	XtManageChild(gridDlg);
	XUndefineCursor(display,XtWindow(mainShell));
	break;
    }
}

static void mapCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    Position X, Y;
    XmString xmString;

    UNREFERENCED(cbs);

    XtTranslateCoords(currentDisplayInfo->shell,0,0,&X,&Y);
  /* Try to force correct popup the first time */
    XtMoveWidget(XtParent(w),X,Y);

  /* Be nice to the users - supply default text field as display name */
    xmString = XmStringCreateLocalized(currentDisplayInfo->dlFile->name);
    XtVaSetValues(w,XmNtextString,xmString,NULL);
    XmStringFree(xmString);
}

static void fileTypeCallback(
  Widget w,
  int buttonNumber,
  XmToggleButtonCallbackStruct *call_data)
{
    if(call_data->set == False) return;
    switch(buttonNumber) {
    case 0:
	MedmUseNewFileFormat = True;
	break;
    case 1:
	MedmUseNewFileFormat = False;
	break;
    }
}

void mainFileMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    DisplayInfo *displayInfo;
    int buttonNumber = (int)cd;
    Widget widget;
    static Widget radioBox = 0;
    XEvent event;
    Boolean saveAll = False;
    int status;
    char *adlName;

    UNREFERENCED(cbs);

    switch(buttonNumber) {
    case MAIN_FILE_NEW_BTN:
	currentDisplayInfo = createDisplay();
	XtManageChild(currentDisplayInfo->drawingArea);
	enableEditFunctions();
	break;

    case MAIN_FILE_OPEN_BTN:
      /*
       * note - all FILE pdm entry dialogs are sharing a single callback
       *	  fileMenuDialogCallback, with client data
       *	  of the BTN id in the simple menu...
       */

      /*
       * create the Open... file selection dialog
       */
	XDefineCursor(display,XtWindow(mainShell),watchCursor);
	XFlush(display);
	if(openFSD == NULL) {
	    Arg args[4];
	    int n = 0;
	    XmString label = XmStringCreateLocalized("*.adl");

#if 0
	  /* KE: Note that not specifying the XmNdirectory is probably
             the same as getting the CWD and setting it to that.  Also
             using "." for the XmNdirectory is probably the same. */
	    char *cwd = getcwd(NULL,PATH_MAX);
	    XmString cwdXmString = XmStringCreateLocalized(cwd);
	    free(cwd);
	    XtSetArg(args[n],XmNdirectory,cwdXmString); n++;
	    XmStringFree(cwdXmString);
#endif

	    XtSetArg(args[n],XmNpattern,label); n++;
	    openFSD = XmCreateFileSelectionDialog(XtParent(mainFilePDM),"openFSD",args,n);
	    XtUnmanageChild(XmFileSelectionBoxGetChild(openFSD,
	      XmDIALOG_HELP_BUTTON));
	    XtAddCallback(openFSD,XmNokCallback,
	      fileMenuDialogCallback,(XtPointer)MAIN_FILE_OPEN_BTN);
	    XtAddCallback(openFSD,XmNcancelCallback,
	      fileMenuDialogCallback,(XtPointer)MAIN_FILE_OPEN_BTN);
	    XmStringFree(label);
	}

	XmListDeselectAllItems(XmFileSelectionBoxGetChild(openFSD,XmDIALOG_LIST));
	XmFileSelectionDoSearch(openFSD,NULL);
	XtManageChild(openFSD);
	XUndefineCursor(display,XtWindow(mainShell));
	break;

    case MAIN_FILE_SAVE_BTN:
    case MAIN_FILE_SAVE_AS_BTN:
      /* No display, do nothing */
	if(!displayInfoListHead->next) break;
      /* Create the Open... file selection dialog */
	if(!saveAsPD) {
	    Arg args[10];
	    XmString buttons[NUM_IMAGE_TYPES-1];
	    Widget rowColumn, typeLabel;
	    int i, n;

	    XmString label = XmStringCreateLocalized("*.adl");
	    char *cwd = getcwd(NULL,PATH_MAX);
	    XmString cwdXmString = XmStringCreateLocalized(cwd);

	    n = 0;
	    XtSetArg(args[n],XmNdefaultPosition,False); n++;
	    XtSetArg(args[n],XmNpattern,label); n++;
	    XtSetArg(args[n],XmNdirectory,cwdXmString); n++;
	    saveAsPD = XmCreateFileSelectionDialog(XtParent(mainFilePDM),
	      "saveAsFSD",args,n);
	    XtUnmanageChild(XmFileSelectionBoxGetChild(saveAsPD,
	      XmDIALOG_HELP_BUTTON));
	    XtAddCallback(saveAsPD,XmNokCallback,
	      fileMenuDialogCallback,(XtPointer)MAIN_FILE_SAVE_AS_BTN);
	    XtAddCallback(saveAsPD,XmNcancelCallback,
	      fileMenuDialogCallback,(XtPointer)MAIN_FILE_SAVE_AS_BTN);
	    XtAddCallback(saveAsPD,XmNmapCallback,mapCallback,(XtPointer)NULL);
	    XmStringFree(label);
	    XmStringFree(cwdXmString);
	    free(cwd);
	    n = 0;
	    XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
	    rowColumn = XmCreateRowColumn(saveAsPD,"rowColumn",args,n);
	    n = 0;
	    typeLabel = XmCreateLabel(rowColumn,"File Format",args,n);

	    buttons[0] = XmStringCreateLocalized("Default");
	    buttons[1] = XmStringCreateLocalized("2.1.x");
	    n = 0;
	    XtSetArg(args[n],XmNbuttonCount,2); n++;
	    XtSetArg(args[n],XmNbuttons,buttons); n++;
	    XtSetArg(args[n],XmNbuttonSet,0); n++;
	    XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
	    XtSetArg(args[n],XmNsimpleCallback,fileTypeCallback); n++;
	    radioBox = XmCreateSimpleRadioBox(rowColumn,"radioBox",args,n);
	    XtManageChild(typeLabel);
	    XtManageChild(radioBox);
	    XtManageChild(rowColumn);
	    for(i = 0; i < 2; i++) XmStringFree(buttons[i]);
	}
      /* Check if more than one display */
	if(displayInfoListHead->next != displayInfoListTail) {
	  /* more than one display, query user */
	    widget = XmTrackingEvent(mainShell,saveCursor,False,&event);
	    if(widget) {
		currentDisplayInfo = dmGetDisplayInfoFromWidget(widget);
	    }
	} else {
	  /* only one display */
	    currentDisplayInfo = displayInfoListHead->next;
	}
	if(!currentDisplayInfo) break;
      /* For some reason, currentDisplay is not valid, break */
	if((currentDisplayInfo->newDisplay)
	  || (buttonNumber == MAIN_FILE_SAVE_AS_BTN)) {
	  /* New display or user wants to save as a different name */
	    WidgetList children;
	    XmListDeselectAllItems(
	      XmFileSelectionBoxGetChild(saveAsPD,XmDIALOG_LIST));
	    XmFileSelectionDoSearch(saveAsPD,NULL);
	    XtVaGetValues(radioBox,
	      XmNchildren, &children, NULL);
	    XmToggleButtonGadgetSetState(children[0],True,True);
	    MedmUseNewFileFormat = True;
	    XtManageChild(saveAsPD);
	} else {
	  /* Save the file */
	    medmSaveDisplay(currentDisplayInfo,
	      currentDisplayInfo->dlFile->name,True);
	}
	break;

    case MAIN_FILE_SAVE_ALL_BTN:
      /* Loop over displays */
	displayInfo = displayInfoListHead->next;
	while(displayInfo) {
#if DEBUG_SAVE_ALL
	    printf("mainFileMenuSimpleCallback: %s %s %s\n",
	      displayInfo->newDisplay?"New":"Old",
	      displayInfo->hasBeenEditedButNotSaved?"Edited    ":"Not Edited",
	      displayInfo->dlFile->name);
#endif
	  /* Only do ones that have been edited */
	    if(displayInfo->hasBeenEditedButNotSaved) {
		char str[2*MAX_FILE_CHARS];
		Boolean saveThis = True;

	      /* Prompt if All has not been choosen */
		if(!saveAll) {
		    sprintf(str,"Save \"%s\" ?",displayInfo->dlFile->name);
		    dmSetAndPopupQuestionDialog(displayInfo,str,"Yes","No",
		      "All Remaining");
		    switch (displayInfo->questionDialogAnswer) {
		    case 1:
		      /* Yes, save this file */
			saveThis = True;
			break;
		    case 2:     /* No */
		    default:
		      /* No, check next file */
			saveThis = False;
			break;
		    case 3:
			saveThis = True;
			saveAll = True;
			break;
		    }
		}
#if DEBUG_SAVE_ALL
		printf("  saveThis=%s\n",
		  saveThis?"True":"False");
#endif
		if(saveThis)
		/* Overwrite unless it is a new display */
		  medmSaveDisplay(displayInfo, displayInfo->dlFile->name,
		    (Boolean)(displayInfo->newDisplay?False:True));
	    }
	    displayInfo = displayInfo->next;
	}
	break;

    case MAIN_FILE_CLOSE_BTN:
	if(displayInfoListHead->next == displayInfoListTail) {
	  /* only one display; no need to query user */
	    widget = displayInfoListTail->drawingArea;
	} else
	  if(displayInfoListHead->next) {
	    /* more than one display; query user */
	      widget = XmTrackingEvent(mainShell,closeCursor,False, &event);
	      if(widget == (Widget) NULL) return;
	  } else {
	    /* no display */
	      return;
	  }
	closeDisplay(widget);
	break;

    case MAIN_FILE_PRINT_SETUP_BTN:
	popupPrintSetup();
	break;

    case MAIN_FILE_PRINT_BTN:
	if(displayInfoListHead->next == displayInfoListTail) {
	  /* only one display; no need to query user */
	    currentDisplayInfo = displayInfoListHead->next;
	    if(currentDisplayInfo != NULL) {
#if 0
#ifdef WIN32
		if(!printToFile) {
		    dmSetAndPopupWarningDialog(currentDisplayInfo,
		      "Printing from MEDM is not available for WIN32\n"
		      "You can use Alt+PrintScreen to copy the window "
		      "to the clipboard",
		      "OK", NULL, NULL);
		}
		break;
#endif
#endif
	      /* Pop it up so it won't be covered by something else */
		XtPopup(currentDisplayInfo->shell,XtGrabNone);
		refreshDisplay(currentDisplayInfo);
		XmUpdateDisplay(currentDisplayInfo->shell);
	      /* Print it */
		if(printTitle == PRINT_TITLE_SHORT_NAME) {
		    adlName = shortName(currentDisplayInfo->dlFile->name);
		} else {
		    adlName = currentDisplayInfo->dlFile->name;
		}
		status = utilPrint(display, currentDisplayInfo->drawingArea,
		  xwdFile, adlName);
		if(!status) {
		    medmPrintf(1,"\nmainFileMenuSimpleCallback: "
		      "Print was not successful\n");
		}
	    }
	} else if(displayInfoListHead->next) {
	  /* more than one display; query user */
	    widget = XmTrackingEvent(mainShell,printCursor,False,&event);
	    if(widget != (Widget)NULL) {
		currentDisplayInfo = dmGetDisplayInfoFromWidget(widget);
		if(currentDisplayInfo != NULL) {
#if 0
#ifdef WIN32
		    if(!printToFile) {
			dmSetAndPopupWarningDialog(currentDisplayInfo,
			  "Printing from MEDM is not available for WIN32\n"
			  "You can use Alt+PrintScreen to copy the window "
			  "to the clipboard",
			  "OK", NULL, NULL);
		    }
		    break;
#endif
#endif
		  /* Pop it up so it won't be covered by something else */
		    XtPopup(currentDisplayInfo->shell,XtGrabNone);
		    refreshDisplay(currentDisplayInfo);
		    XmUpdateDisplay(currentDisplayInfo->shell);
		  /* Print it */
		    if(printTitle == PRINT_TITLE_SHORT_NAME) {
			adlName = shortName(currentDisplayInfo->dlFile->name);
		    } else {
			adlName = currentDisplayInfo->dlFile->name;
		    }
		    status = utilPrint(display, currentDisplayInfo->drawingArea,
		      xwdFile, adlName);
		    if(!status) {
			medmPrintf(1,"\nmainFileMenuSimpleCallback: "
			  "Print was not successful\n");
		    }
		}
	    }
	}
	break;
    case MAIN_FILE_EXIT_BTN:
	medmExit();
	break;
    }
}

#if 0
static void medmExitMapCallback(
  Widget w,
  DisplayInfo *displayInfo,
  XmAnyCallbackStruct *call_data)
{
    Position X, Y;

    XtTranslateCoords(displayInfo->shell,0,0,&X,&Y);
  /* try to force correct popup the first time */
    XtMoveWidget(XtParent(w),X,Y);

  /*
    xmString = XmStringCreateLocalized(displayInfo->dlFile->name);
    XtVaSetValues(w,XmNtextString,xmString,NULL);
    XmStringFree(xmString);
    */
}
#endif

#if DEBUG_FILE_RENAME
/* Debug routine for file permissions */
void printStat(char *filename, char *comment)
{
#ifdef WIN32
    int  status;
    struct stat statBuf;

    status = stat(filename,&statBuf);
    if(status) {
	print("stat failed [%s]: %s\n",comment,filename);
	return;
    }
    print("%s: %c%c%c\n",
      comment,
      statBuf.st_mode&S_IREAD?'r':'-',
      statBuf.st_mode&S_IWRITE?'w':'-',
      statBuf.st_mode&S_IEXEC?'x':'-');
#else
    int  status;
    struct stat statBuf;

    status = stat(filename,&statBuf);
    if(status) {
	print("stat failed [%s]: %s\n",comment,filename);
	return;
    }
    print("%s: %c%c%c%c%c%c%c%c%c\n",
      comment,
      statBuf.st_mode&S_IRUSR?'r':'-',
      statBuf.st_mode&S_IWUSR?'w':'-',
      statBuf.st_mode&S_IXUSR?'x':'-',
      statBuf.st_mode&S_IRGRP?'r':'-',
      statBuf.st_mode&S_IWGRP?'w':'-',
      statBuf.st_mode&S_IXGRP?'x':'-',
      statBuf.st_mode&S_IROTH?'r':'-',
      statBuf.st_mode&S_IWOTH?'w':'-',
      statBuf.st_mode&S_IXOTH?'x':'-');
#endif
}
#endif

/* medm allowed a .template used as a suffix for compatibility. This exception is
   caused by a bug in the save routine at checking the ".adl" suffix.
*/

const char *templateSuffix = ".template";

Boolean medmSaveDisplay(DisplayInfo *displayInfo, char *filename, Boolean overwrite)
{
    char *suffix;
    char f1[MAX_FILE_CHARS], f2[MAX_FILE_CHARS+4];
    char warningString[2*MAX_FILE_CHARS];
    int  strLen1, strLen2, strLen3, strLen4;
    int  status;
    FILE *stream;
    Boolean brandNewFile = False;
    Boolean templateException = False;
    struct stat statBuf;

    if(displayInfo == NULL) return False;
    if(filename == NULL) return False;

    strLen1 = strlen(filename);

    if(strLen1 >= MAX_FILE_CHARS) {
	medmPrintf(1,"\nPath too Long: %s\n",filename);
	return False;
    }

#ifdef VMS
    vmsTrimVersionNumber(filename);
    strcpy(f1,filename);
    tolower(f1,filename);
#endif

    strLen1 = strlen(filename);
    strLen2 = strlen(DISPLAY_FILE_BACKUP_SUFFIX);
    strLen3 = strlen(DISPLAY_FILE_ASCII_SUFFIX);
    strLen4 = strlen(templateSuffix);

  /* Search for the position of the .adl suffix */
    strcpy(f1,filename);
    suffix = strstr(f1,DISPLAY_FILE_ASCII_SUFFIX);
    if((suffix) && (suffix == f1 + strLen1 - strLen3)) {
      /* Chop off the .adl suffix */
	*suffix = '\0';
	strLen1 = strLen1 - strLen3;
    } else {
      /* Search for the position of the .template suffix */
	suffix = strstr(f1,templateSuffix);
	if((suffix) && (suffix == f1 + strLen1 - strLen4)) {
	  /* this is a .template special case */
	    templateException = True;
	}
    }


  /* Create the backup file name with suffix _BAK.adl*/
    strcpy(f2,f1);
    strcat(f2,DISPLAY_FILE_BACKUP_SUFFIX);
    strcat(f2,DISPLAY_FILE_ASCII_SUFFIX);
#if DEBUG_FILE_RENAME
    print("medmSaveDisplay: \n");
    print("  filename=|%s|\n",filename);
    print("  f1=|%s|\n",f1);
    print("  f2=|%s|\n",f2);
#endif

  /* Check for the special case .template */
    if(!templateException)
      strcat(f1,DISPLAY_FILE_ASCII_SUFFIX);

  /* See whether the file already exists. */
    errno=0;
    if(access(f1,W_OK) == -1) {
	if(errno == ENOENT) {
	  /* File not found */
	    brandNewFile = True;
	} else {
	    char *errstring=strerror(errno);

	    medmPostMsg(1,"Error accessing file:\n%s\n%s\n",
	      f1,errstring);
	    return False;
	}
    } else {
      /* File exists, see whether the user wants to overwrite the file. */
	if(!overwrite) {
	    sprintf(warningString,"Do you want to overwrite file:\n%s",f1);
	    dmSetAndPopupQuestionDialog(displayInfo,warningString,"Yes","No",NULL);
	    switch (displayInfo->questionDialogAnswer) {
	    case 1:
	      /* Yes, Save the file */
		break;
	    default:
	      /* No, return */
		return False;
	    }
	}
      /* See whether the backup file can be overwritten */
	errno=0;
	if(access(f2,W_OK) == -1) {
	    if(errno != ENOENT) {
		char *errstring=strerror(errno);

		medmPostMsg(1,"Cannot write backup file:\n%s\n%s\n",
		  filename,errstring);
		return False;
	    }
	} else {
	  /* File exists and has write permission */
#ifdef WIN32
	  /* WIN32 cannot rename the file if the name is in use so delete it */
	    status = remove(f2);
	    if(status) {
	        medmPrintf(1,"\nCannot remove old file:\n%s",f2);
	        return False;
            }
#endif
        }
      /* Get the status of the file to be renamed */
	status = stat(f1,&statBuf);
	if(status) {
	    medmPrintf(1,"\nFailed to get status of file %s\n",f1);
	    return False;
	}
      /* Rename it */
	errno=0;
	status = rename(f1,f2);
	if(status) {
	    char *errstring=strerror(errno);

	    medmPrintf(1,"\nCannot rename file: %s\n"
	      "  To: %s\n"
	      "  %s\n",
	      f1,f2,errstring);
	    return False;
	}
    }

  /* Open for writing (Use w+ or WIN32 makes it readonly) */
    stream = fopen(f1,"w+");
    if(stream == NULL) {
	char *errstring=strerror(errno);

	medmPostMsg(1,"Failed to create/write file:\n%s\n%s\n",
	  filename,errstring);
	return False;
    }
    strcpy(displayInfo->dlFile->name,f1);
    dmWriteDisplayList(displayInfo,stream);
    fclose(stream);
    displayInfo->hasBeenEditedButNotSaved = False;
    displayInfo->newDisplay = False;
    medmSetDisplayTitle(displayInfo);
  /* If it was an existing file set its mode equal to the old mode */
    if(!brandNewFile) {
#if DEBUG_FILE_RENAME
	printStat(f1,"  f1 Current permissions");
#endif
	chmod(f1,statBuf.st_mode);
#if DEBUG_FILE_RENAME
	printStat(f1,"  f1 Changed permissions");
#endif
    }
    return True;
}

#ifdef VMS
void vmsTrimVersionNumber (char *fileName)
{
    char *tmpPtr;


    tmpPtr = fileName + strlen (fileName) - 1;

    while((tmpPtr > fileName) && (*tmpPtr != ';'))
      tmpPtr--;
    if(*tmpPtr == ';')
      *tmpPtr = '\0';
}
#endif

void medmExit()
{
    char *filename, *tmp;
    char str[2*MAX_FILE_CHARS];
    Boolean saveAll = False;
    Boolean saveThis = False;

    DisplayInfo *displayInfo = displayInfoListHead->next;
    while(displayInfo) {
	if(displayInfo->hasBeenEditedButNotSaved) {
	    if(saveAll == False) {
		filename = tmp = displayInfo->dlFile->name;
	      /* strip off the path */
		while(*tmp != '\0') {
		    if(*tmp == MEDM_DIR_DELIMITER_CHAR)
		      filename = tmp+1;
		    tmp++;
		}
		sprintf(str,"Save display \"%s\" before exit?",filename);
#ifdef PROMPT_TO_EXIT
	      /* Don't use Cancel, use All (Only 3 buttons) */
		if(displayInfo->next)
		  dmSetAndPopupQuestionDialog(displayInfo,str,"Yes","No","All");
		else
		  dmSetAndPopupQuestionDialog(displayInfo,str,"Yes","No",NULL);
#else
	      /* Use Cancel, don't use All (Only 3 buttons) */
		if(displayInfo->next)
		  dmSetAndPopupQuestionDialog(displayInfo,str,"Yes","No","Cancel");
		else
		  dmSetAndPopupQuestionDialog(displayInfo,str,"Yes","No","Cancel");
#endif
		switch (displayInfo->questionDialogAnswer) {
		case 1:
		  /* Yes, save this file */
		    saveThis = True;
		    break;
		case 2:
		  /* No, check next file */
		    saveThis = False;
		    break;
		case 3:
#ifdef PROMPT_TO_EXIT
		  /* Save all files */
		    saveAll = True;
		    saveThis = True;
		    break;
#else
		  /* Cancel */
		    return;
#endif
		default:
		    saveThis = False;
		    break;
		}
	    }
	    if(saveThis == True)
	      if(medmSaveDisplay(displayInfo,
		displayInfo->dlFile->name,True) == False) return;
	}
	displayInfo = displayInfo->next;
    }
#ifdef PROMPT_TO_EXIT
  /* Prompt to exit */
#if 0
  /* KE: This appears to work and deiconify if iconic */
    XMapRaised(display, XtWindow(mainShell));
#else
  /* KE: This can only be set before realization */
    XtVaSetValues(mainShell, XmNiconic, False, NULL);
    XtPopup(mainShell,XtGrabNone);
#endif
    XtManageChild(exitQD);
    XtPopup(XtParent(exitQD),XtGrabNone);
#else
  /* Exit without prompt */
    medmClearImageCache();
    medmCATerminate();
    destroyMedmWidget();
    dmTerminateX();
    exit(0);
#endif
}

static void fileMenuDialogCallback(
  Widget w,
  XtPointer clientData,
  XtPointer callbackStruct)
{
    int btn = (int) clientData;
    XmAnyCallbackStruct *call_data = (XmAnyCallbackStruct *) callbackStruct;
    XmSelectionBoxCallbackStruct *select;
    char *filename, warningString[2*MAX_FILE_CHARS];
    XmString warningXmstring;

    switch(call_data->reason){
    case XmCR_CANCEL:
	XtUnmanageChild(w);
	break;
    case XmCR_OK:
	switch(btn) {
        case MAIN_FILE_OPEN_BTN: {
	    FILE *filePtr;
	    char *filename;

	    XmSelectionBoxCallbackStruct *call_data =
	      (XmSelectionBoxCallbackStruct *) callbackStruct;

          /* if no list element selected, simply return */
	    if(call_data->value == NULL) return;

          /* Get the filename string from the selection box */
	    XmStringGetLtoR(call_data->value, XmFONTLIST_DEFAULT_TAG, &filename);

	    if(filename) {
		filePtr = fopen(filename,"r");
		if(filePtr) {
		    XtUnmanageChild(w);
		    dmDisplayListParse(NULL, filePtr, NULL, filename, NULL,
		      (Boolean)False);
		    fclose(filePtr);
		    enableEditFunctions();
		}
		XtFree(filename);
	    }
	    break;
        }
        case MAIN_FILE_CLOSE_BTN:
	    dmRemoveDisplayInfo(currentDisplayInfo);
	    currentDisplayInfo = NULL;
	    break;
        case MAIN_FILE_SAVE_AS_BTN:
	    select = (XmSelectionBoxCallbackStruct *)call_data;
	    XmStringGetLtoR(select->value,XmFONTLIST_DEFAULT_TAG,&filename);
	    medmSaveDisplay(currentDisplayInfo,filename,False);
	    sprintf(warningString,"%s","Name of file to save display in:");
	    warningXmstring = XmStringCreateLocalized(warningString);
	    XtVaSetValues(saveAsPD,XmNselectionLabelString,warningXmstring,NULL);
	    XmStringFree(warningXmstring);
	    XtFree(filename);
	    XtUnmanageChild(w);
	    break;
        case MAIN_FILE_EXIT_BTN:
	    medmClearImageCache();
	    medmCATerminate();
	    destroyMedmWidget();
	    dmTerminateX();
	    exit(0);
	    break;
	}
	break;
    }
}

static void palettesMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    int buttonNumber = (int) cd;

    UNREFERENCED(cbs);

    switch(buttonNumber) {

    case PALETTES_OBJECT_BTN:
      /* fills in global objectMW */
	if(objectMW == NULL) createObject();
	XtPopup(objectS,XtGrabNone);
	break;

    case PALETTES_RESOURCE_BTN:
      /* fills in global resourceMW */
	if(resourceMW == NULL) createResource();
      /* (MDA) this is redundant - done at end of createResource() */
	XtPopup(resourceS,XtGrabNone);
	break;

    case PALETTES_COLOR_BTN:
      /* fills in global colorMW */
	if(colorMW == NULL)
	  setCurrentDisplayColorsInColorPalette(BCLR_RC,0);
	XtPopup(colorS,XtGrabNone);
	break;

#ifdef EXTENDED_INTERFACE
    case PALETTES_CHANNEL_BTN:
      /* fills in global channelMW */
	if(channelMW == NULL) createChannel();
	XtPopup(channelS,XtGrabNone);
	break;
#endif
    }

}

static void helpMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    int buttonNumber = (int)cd;

    UNREFERENCED(w);
    UNREFERENCED(cbs);

    switch(buttonNumber) {
      /* implement context sensitive help */
/*     case HELP_OVERVIEW_BTN: */
/* 	widget = XmTrackingEvent(mainShell,helpCursor,False,&event); */
/* 	if(widget != (Widget)NULL) { */
/* 	    call_data->reason = XmCR_HELP; */
/* 	    XtCallCallbacks(widget,XmNhelpCallback,&call_data); */
/* 	} */
/* 	break; */
    case HELP_OVERVIEW_BTN:
	callBrowser(medmHelpPath,"#Overview");
	break;
    case HELP_CONTENTS_BTN:
	callBrowser(medmHelpPath,"#Contents");
	break;
    case HELP_OBJECTS_BTN:
	callBrowser(medmHelpPath,"#ObjectIndex");
	break;
    case HELP_EDIT_BTN:
	callBrowser(medmHelpPath,"#Editing");
	break;
    case HELP_NEW_BTN:
	callBrowser(medmHelpPath,"#NewFeatures");
	break;
    case HELP_TECH_SUPPORT_BTN:
	callBrowser(medmHelpPath,"#TechSupport");
	break;
    case HELP_ON_HELP_BTN:
    {
#ifdef WIN32
	XmString xmString1=XmStringCreateLtoR(
	  "     Help is implemented in the WIN32 version of MEDM by using your\n"
	  "default browser.  The browser is called using the start command with\n"
	  "the name of the relevant help URL.  If the browser is not running,\n"
	  "this should cause it to come up with the URL.  If it is already up,\n",
	  XmFONTLIST_DEFAULT_TAG);
	XmString xmString2=XmStringCreateLtoR(
	  "this should cause it to change to the requested URL.  It is necessary\n"
	  "for the environment variable ComSpec to be defined, but it should be\n"
	  "defined by default.\n"
	  "\n"
	  "     You should be able to change the displayed URL via the MEDM Help\n"
	  "menu or the context-sensitive Help buttons.\n",
	  XmFONTLIST_DEFAULT_TAG);
#else
	XmString xmString1=XmStringCreateLtoR(
	  "     Help in this version of MEDM is implemented using Netscape.  If\n"
	  "the environmental variable NETSCAPEPATH containing the full pathname\n"
	  "of the Netscape executable exists, then that path is used to call\n"
	  "Netscape.  Otherwise, it is called using just the command, netscape.\n",
	  XmFONTLIST_DEFAULT_TAG);
	XmString xmString2=XmStringCreateLtoR(
	  "If Netscape is not available, then most of the MEDM help is not\n"
	  "available.\n"
	  "\n"
	  "     If Netscape is running when MEDM first calls it, then the\n"
	  "response should be fairly quick.  Otherwise, the first call to help\n"
	  "must wait until Netscape comes up, which will take somewhat longer.\n",
	  XmFONTLIST_DEFAULT_TAG);
#endif
	XmString xmString=XmStringConcat(xmString1,xmString2);
	Arg args[20];
	int nargs;

	nargs=0;
	XtSetArg(args[nargs],XmNmessageString,xmString); nargs++;
	XtSetValues(helpMessageBox,args,nargs);
	XmStringFree(xmString1);
	XmStringFree(xmString2);
	XmStringFree(xmString);
	XtPopup(helpS,XtGrabNone);
	break;
    }
    case HELP_ON_VERSION_BTN:
	XtPopup(productDescriptionShell,XtGrabNone);
#if DEBUG_RADIO_BUTTONS
	{
	    Boolean radioBehavior,set;
	    unsigned char indicatorType;
	    Arg args[20];
	    int nargs;

	    nargs=0;
	    XtSetArg(args[nargs],XmNradioBehavior,&radioBehavior); nargs++;
	    XtGetValues(modeRB,args,nargs);
	    print("\nEdit/Execute: globalDisplayListTraversalMode=%d [DL_EXECUTE=%d DL_EDIT=%d]\n",
	      globalDisplayListTraversalMode,DL_EXECUTE,DL_EDIT);
	    print("modeRB(%x): XmNradioBehavior=%d \n",modeRB,(int)radioBehavior);

	    nargs=0;
	    XtSetArg(args[nargs],XmNindicatorType,&indicatorType); nargs++;
	    XtSetArg(args[nargs],XmNset,&set); nargs++;
	    XtGetValues(modeEditTB,args,nargs);
	    print("modeEditTB(%x): XmNset=%d  XmNindicatorType=%d "
	      "[XmN_OF_MANY=%d XmONE_OF_MANY=%d]\n",
	      modeEditTB,(int)set,(int)indicatorType,(int)XmN_OF_MANY,(int)XmONE_OF_MANY);

	    nargs=0;
	    XtSetArg(args[nargs],XmNindicatorType,&indicatorType); nargs++;
	    XtSetArg(args[nargs],XmNset,&set); nargs++;
	    XtGetValues(modeExecTB,args,nargs);
	    print("modeExecTB(%x): XmNset=%d  XmNindicatorType=%d "
	      "[XmN_OF_MANY=%d XmONE_OF_MANY=%d]\n",
	      modeExecTB,(int)set,(int)indicatorType,(int)XmN_OF_MANY,(int)XmONE_OF_MANY);
	}
#endif
	break;
    }
}

static void helpDialogCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    UNREFERENCED(w);
    UNREFERENCED(cd);

    switch(((XmAnyCallbackStruct *) cbs)->reason){
    case XmCR_OK:
    case XmCR_CANCEL:
	XtPopdown(helpS);
	break;
    }
}

static void editHelpDialogCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    UNREFERENCED(w);

    switch(((XmAnyCallbackStruct *) cbs)->reason){
    case XmCR_OK:
    case XmCR_CANCEL:
	XtPopdown(editHelpS);
	break;
    }
}

static void modeCallback(Widget w, XtPointer cd, XtPointer cbs)
{
    DlTraversalMode mode = (DlTraversalMode)cd;
    XmToggleButtonCallbackStruct *call_data = (XmToggleButtonCallbackStruct *)cbs;
    DisplayInfo *displayInfo;

    UNREFERENCED(w);

#if DEBUG_RADIO_BUTTONS
    {
	Boolean radioBehavior,set;
	unsigned char indicatorType;
	Arg args[20];
	int nargs;

	nargs=0;
	XtSetArg(args[nargs],XmNradioBehavior,&radioBehavior); nargs++;
	XtGetValues(XtParent(w),args,nargs);
	print("\nmodeCallback: mode=%d [DL_EXECUTE=%d DL_EDIT=%d]\n",
	  mode,DL_EXECUTE,DL_EDIT);
	print("\nParent(%x): XmNradioBehavior=%d \n",XtParent(w),(int)radioBehavior);

	nargs=0;
	XtSetArg(args[nargs],XmNindicatorType,&indicatorType); nargs++;
	XtSetArg(args[nargs],XmNset,&set); nargs++;
	XtGetValues(w,args,nargs);
	print("Widget(%x): XmNindicatorType=%d "
	  "[XmN_OF_MANY=%d XmONE_OF_MANY=%d]\n",
	  w,(int)indicatorType,(int)XmN_OF_MANY,(int)XmONE_OF_MANY);
	print("Widget(%x): XmNset=%d\n",w,(int)set);
	print("If both the Edit and the Execute buttons are down,"
	  " select \"On Version\"\n  on the Help menu to get more information."
	  "  Send to evans@aps.anl.gov.\n");
    }
#endif

  /* Since both on & off will invoke this callback, only care about transition
   *   of one to ON (with real change of state) */
    if(call_data->set == False ||
      globalDisplayListTraversalMode == mode) return;

  /* Set the globalDisplayListTraversalMode to the specified mode */
    globalDisplayListTraversalMode = mode;

  /* Unselect any selected elements */
    unselectElementsInDisplay();

  /* Disable EDIT functions */
    disableEditFunctions();

  /* Mode is the mode to which we are going */
    switch(mode) {
    case DL_EDIT:
      /* Turn off any hidden button markers */
	displayInfo = displayInfoSaveListHead->next;
	while(displayInfo) {
	    if(displayInfo->nMarkerWidgets) {
	      /* Toggle them off */
		markHiddenButtons(displayInfo);
	    }
	    displayInfo =displayInfo->next;
	}

      /* Restore any related displays that were replaced */
#if 0
	dumpDisplayInfoList(displayInfoListHead,"medm.c [1]: displayInfoList");
	dumpDisplayInfoList(displayInfoSaveListHead,"medm.c [1]: displayInfoSaveList");
#endif
	displayInfo = displayInfoSaveListHead->next;
	while(displayInfo) {
	    DisplayInfo *pDI = displayInfo->next;

	    moveDisplayInfoSaveToDisplayInfo(displayInfo);
	    XtPopup(displayInfo->shell,XtGrabNone);
	    displayInfo = pDI;
	}
#if 0
	dumpDisplayInfoList(displayInfoListHead,"medm.c [2]: displayInfoList");
	dumpDisplayInfoList(displayInfoSaveListHead,"medm.c [2]: displayInfoSaveList");
#endif
      /* Update the x and y values for each display */
	updateAllDisplayPositions();

      /* Set appropriate sensitivity */
	if(relatedDisplayS) XtSetSensitive(relatedDisplayS,True);
	if(cartesianPlotS) XtSetSensitive(cartesianPlotS,True);
	if(cartesianPlotAxisS) {
	    XtSetSensitive(cartesianPlotAxisS,True);
	    XtPopdown(cartesianPlotAxisS);
	}
	if(stripChartS) {
	    XtSetSensitive(stripChartS,True);
	    executeTimeStripChartElement = NULL;
	    XtPopdown(stripChartS);
	}
	if(pvInfoS) {
	    XtSetSensitive(pvInfoS,False);
	    XtPopdown(pvInfoS);
	}
	if(pvLimitsS) {
	    XtSetSensitive(pvLimitsS,True);
	    XtPopdown(pvLimitsS);
	}
	XtSetSensitive(fileMenu[MAIN_FILE_NEW_BTN].widget,True);
	XtSetSensitive(fileMenu[MAIN_FILE_SAVE_BTN].widget,True);
	XtSetSensitive(fileMenu[MAIN_FILE_SAVE_ALL_BTN].widget,True);
	XtSetSensitive(fileMenu[MAIN_FILE_SAVE_AS_BTN].widget,True);

      /* Stop the scheduler */
	stopMedmScheduler();
        if(medmWorkProcId) {
	    XtRemoveWorkProc(medmWorkProcId);
	    medmWorkProcId = 0;
        }
      /* Stop the PV statistics */
	medmStopUpdateCAStudyDlg();

	break;
    case DL_EXECUTE:
      /* Start the scheduler */
	startMedmScheduler();

      /* Update the x and y values for each display */
	updateAllDisplayPositions();

      /* Set appropriate sensitivity */
	if(relatedDisplayS) {
	    XtSetSensitive(relatedDisplayS,False);
	    XtPopdown(relatedDisplayS);
	}
	if(cartesianPlotS) {
	    XtSetSensitive(cartesianPlotS,False);
	    XtPopdown(cartesianPlotS);
	}
	if(cartesianPlotAxisS) {
	    XtSetSensitive(cartesianPlotAxisS,False);
	    XtPopdown(cartesianPlotAxisS);
	}
	if(stripChartS) {
	    XtSetSensitive(stripChartS,False);
	    executeTimeStripChartElement = NULL;
	    XtPopdown(stripChartS);
	}
	if(pvLimitsS) {
	    XtSetSensitive(pvLimitsS,False);
	    XtPopdown(pvLimitsS);
	}
	XtSetSensitive(fileMenu[MAIN_FILE_NEW_BTN].widget,False);
	XtSetSensitive(fileMenu[MAIN_FILE_SAVE_BTN].widget,False);
	XtSetSensitive(fileMenu[MAIN_FILE_SAVE_ALL_BTN].widget,False);
	XtSetSensitive(fileMenu[MAIN_FILE_SAVE_AS_BTN].widget,False);

      /* Start the PV statistics */
	medmResetUpdateCAStudyDlg(NULL,NULL,NULL);
	medmStartUpdateCAStudyDlg();

	break;
    default:
	break;
    }

    executeTimeCartesianPlotWidget = NULL;
    executeTimePvLimitsElement = NULL;
    executeTimeStripChartElement = NULL;
    currentDisplayInfo = (DisplayInfo *)NULL;

  /* Go through the whole display list, if any display is brought up
     as a related display, shut down that display and remove that
     display from the display list, otherwise, just shutdown that
     display. */
    displayInfo = displayInfoListHead->next;
    while(displayInfo) {
	DisplayInfo *pDI = displayInfo;

      /* Set the mode */
	displayInfo->traversalMode = mode;

      /* Remove any extraneous clipping (Shouldn't be any)  */
	XSetClipOrigin(display, displayInfo->gc, 0, 0);
	XSetClipMask(display, displayInfo->gc, None);

	displayInfo = displayInfo->next;

      /* Remove any displays that came from related displays and were
         not open before */
	if(pDI->fromRelatedDisplayExecution) {
	    dmRemoveDisplayInfo(pDI);
	} else {
	    dmCleanupDisplayInfo(pDI,False);
	}
    }

  /* See whether there is any display in the display list.  If any,
     enable resource palette, object palette and color palette,
     traverse the whole display list. */
    if(displayInfoListHead->next) {
	if(globalDisplayListTraversalMode == DL_EDIT) {
	    enableEditFunctions();
	}
	currentDisplayInfo = displayInfoListHead->next;
	dmTraverseAllDisplayLists();
	XFlush(display);
    }

#ifdef __MONITOR_CA_PEND_EVENT__
    {
	double t;
	t = medmTime();
	ca_pend_event(CA_PEND_EVENT_TIME);
	t = medmTime() - t;
	if(t > 0.5) {
	    print("modecallback : time used by ca_pend_event = %8.1f\n",t);
	}
    }
#else
    ca_pend_event(CA_PEND_EVENT_TIME);
#endif
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
    gc = XCreateGC(display,sourcePixmap,0,NULL);
  /* Eliminate events that we do not handle anyway */
    XSetGraphicsExposures(display,gc,False);
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
      &colors[0],&colors[1],hotSpotWidth/2,(asc+desc)/2);
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
      &colors[0],&colors[1],hotSpotWidth/2,(asc+desc)/2);
    XFreePixmap(display,sourcePixmap);
    XFreePixmap(display,maskPixmap);

  /* PASTE cursor */
    XTextExtents(fontTable[6],"Paste",5,&dir,&asc,&desc,&overall);
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
    XDrawString(display,sourcePixmap,gc,hotSpotWidth,asc,"Paste",5);
    XDrawString(display,maskPixmap,gc,hotSpotWidth,asc,"Paste",5);
    pasteCursor = XCreatePixmapCursor(display,sourcePixmap,maskPixmap,
      &colors[0],&colors[1],hotSpotWidth/2,(asc+desc)/2);
    XFreePixmap(display,sourcePixmap);
    XFreePixmap(display,maskPixmap);

  /* PV cursor */
    XTextExtents(fontTable[6],"PV",2,&dir,&asc,&desc,&overall);
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
    XDrawString(display,sourcePixmap,gc,hotSpotWidth,asc,"PV",2);
    XDrawString(display,maskPixmap,gc,hotSpotWidth,asc,"PV",2);
    pvCursor = XCreatePixmapCursor(display,sourcePixmap,maskPixmap,
      &colors[0],&colors[1],hotSpotWidth/2,(asc+desc)/2);
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
      &colors[0],&colors[1],hotSpotWidth/2,(asc+desc)/2);
    XFreePixmap(display,sourcePixmap);
    XFreePixmap(display,maskPixmap);

  /* no write access cursor */
    colors[0].pixel = alarmColor(MAJOR_ALARM);
    colors[1].pixel = WhitePixel(display,screenNum);
    XQueryColors(display,cmap,colors,2);

    sourcePixmap = XCreateBitmapFromData(display,RootWindow(display,screenNum),
      (char *)noAccess25_bits, noAccess25_width, noAccess25_height);
    maskPixmap = XCreateBitmapFromData(display,RootWindow(display,screenNum),
      (char *)noAccessMask25_bits, noAccessMask25_width, noAccessMask25_height);
    noWriteAccessCursor = XCreatePixmapCursor(display,sourcePixmap,maskPixmap,
      &colors[0],&colors[1],13,13);
    XFreePixmap(display,sourcePixmap);
    XFreePixmap(display,maskPixmap);

  /* big hand cursor */
    if(medmUseBigCursor) {
	colors[0].pixel = BlackPixel(display,screenNum);
	colors[1].pixel = WhitePixel(display,screenNum);
	XQueryColors(display,cmap,colors,2);

	sourcePixmap = XCreateBitmapFromData(display,RootWindow(display,screenNum),
	  (char *)bigHand25_bits, bigHand25_width, bigHand25_height);
	maskPixmap = XCreateBitmapFromData(display,RootWindow(display,screenNum),
	  (char *)bigHandMask25_bits, bigHandMask25_width, bigHandMask25_height);
	rubberbandCursor = XCreatePixmapCursor(display,sourcePixmap,maskPixmap,
	  &colors[0],&colors[1],1,2);
	XFreePixmap(display,sourcePixmap);
	XFreePixmap(display,maskPixmap);
    } else {
	rubberbandCursor = XCreateFontCursor(display,XC_hand2);
    }

  /* big cross cursor */
    if(medmUseBigCursor) {
	colors[0].pixel = BlackPixel(display,screenNum);
	colors[1].pixel = WhitePixel(display,screenNum);
	XQueryColors(display,cmap,colors,2);

	sourcePixmap = XCreateBitmapFromData(display,RootWindow(display,screenNum),
	  (char *)bigCross25_bits, bigCross25_width, bigCross25_height);
	maskPixmap = XCreateBitmapFromData(display,RootWindow(display,screenNum),
	  (char *)bigCrossMask25_bits, bigCrossMask25_width, bigCrossMask25_height);
	crosshairCursor = XCreatePixmapCursor(display,sourcePixmap,maskPixmap,
	  &colors[0],&colors[1],13,13);
	XFreePixmap(display,sourcePixmap);
	XFreePixmap(display,maskPixmap);
    } else {
	crosshairCursor = XCreateFontCursor(display,XC_crosshair);
    }

  /* big 4 way pointers */
    if(medmUseBigCursor) {
	colors[0].pixel = BlackPixel(display,screenNum);
	colors[1].pixel = WhitePixel(display,screenNum);
	XQueryColors(display,cmap,colors,2);

	sourcePixmap = XCreateBitmapFromData(display,RootWindow(display,screenNum),
	  (char *)big4WayPtr25_bits, big4WayPtr25_width, big4WayPtr25_height);
	maskPixmap = XCreateBitmapFromData(display,RootWindow(display,screenNum),
	  (char *)big4WayPtrMask25_bits, big4WayPtrMask25_width,
	  big4WayPtrMask25_height);
	dragCursor = XCreatePixmapCursor(display,sourcePixmap,maskPixmap,
	  &colors[0],&colors[1],13,13);
	XFreePixmap(display,sourcePixmap);
	XFreePixmap(display,maskPixmap);
    } else {
	dragCursor = XCreateFontCursor(display,XC_fleur);
    }

  /* big size cursor pointers */
    if(medmUseBigCursor) {
	colors[0].pixel = BlackPixel(display,screenNum);
	colors[1].pixel = WhitePixel(display,screenNum);
	XQueryColors(display,cmap,colors,2);

	sourcePixmap = XCreateBitmapFromData(display,RootWindow(display,screenNum),
	  (char *)bigSizeCursor25_bits, bigSizeCursor25_width,
	  bigSizeCursor25_height);
	maskPixmap = XCreateBitmapFromData(display,RootWindow(display,screenNum),
	  (char *)bigSizeCursorMask25_bits, bigSizeCursorMask25_width,
	  bigSizeCursorMask25_height);
	resizeCursor = XCreatePixmapCursor(display,sourcePixmap,maskPixmap,
	  &colors[0],&colors[1],25,25);
	XFreePixmap(display,sourcePixmap);
	XFreePixmap(display,maskPixmap);
    } else {
	resizeCursor = XCreateFontCursor(display,XC_bottom_right_corner);
    }

  /* big watch cursor pointers */
    if(medmUseBigCursor) {
	colors[0].pixel = BlackPixel(display,screenNum);
	colors[1].pixel = WhitePixel(display,screenNum);
	XQueryColors(display,cmap,colors,2);

	sourcePixmap = XCreateBitmapFromData(display,RootWindow(display,screenNum),
	  (char *)bigWatchCursor25_bits, bigWatchCursor25_width,
	  bigWatchCursor25_height);
	maskPixmap = XCreateBitmapFromData(display,RootWindow(display,screenNum),
	  (char *)bigWatchCursorMask25_bits, bigWatchCursorMask25_width,
	  bigWatchCursorMask25_height);
	watchCursor = XCreatePixmapCursor(display,sourcePixmap,maskPixmap,
	  &colors[0],&colors[1],25,25);
	XFreePixmap(display,sourcePixmap);
	XFreePixmap(display,maskPixmap);
    } else {
	watchCursor = XCreateFontCursor(display,XC_watch);
    }

    XFreeGC(display,gc);

  /*
   * now create standard font cursors
   */
    helpCursor = XCreateFontCursor(display,XC_question_arrow);
    xtermCursor = XCreateFontCursor(display,XC_xterm);

}

Widget buildMenu(Widget parent,
  int menuType,
  char *menuTitle,
  char menuMnemonic,
  menuEntry_t *items) {
    Widget menu, cascade;
    int i;
    XmString str;
    if(menuType == XmMENU_PULLDOWN) {
	menu = XmCreatePulldownMenu(parent, "pulldownMenu",NULL,0);
	str = XmStringCreateLocalized(menuTitle);
	cascade = XtVaCreateManagedWidget(menuTitle,
	  xmCascadeButtonGadgetClass, parent,
	  XmNsubMenuId, menu,
	  XmNlabelString, str,
	  XmNmnemonic, menuMnemonic,
	  NULL);
	XmStringFree(str);
    } else {
	menu = XmCreatePopupMenu(parent, "popupMenu", NULL, 0);
    }

  /* now add the menu items */
    for(i=0; items[i].label != NULL; i++) {
      /* if subitems exist, create the pull-right menu by calling this
       * function recursively. Since the function returns a cascade
       * button, the widget returned is used..
       */

	if(items[i].subItems) {
	    items[i].widget = buildMenu(menu, XmMENU_PULLDOWN,
	      items[i].label,
	      items[i].mnemonic,
	      items[i].subItems);
	} else {
	    items[i].widget = XtVaCreateManagedWidget(items[i].label,
	      *items[i].widgetClass, menu,
	      NULL);
	}

      /* Whether the item is a real item or a cascade button with a
       * menu, it can still have a mnemonic.
       */
	if(items[i].mnemonic) {
	    XtVaSetValues(items[i].widget, XmNmnemonic, items[i].mnemonic, NULL);
	}

      /* any item can have an accelerator, execpt cascade menus. But,
       * we don't worry about that; we know better in our declarations.
       */
	if(items[i].accelerator) {
	    str = XmStringCreateLocalized(items[i].accText);
	    XtVaSetValues(items[i].widget,
	      XmNaccelerator, items[i].accelerator,
	      XmNacceleratorText, str,
	      NULL);
	    XmStringFree(str);
	}
      /* again, anyone can have a callback -- however, this is an
       * activate-callback.  This may not be appropriate for all items.
       */
	if(items[i].callback) {
	    XtAddCallback(items[i].widget, XmNactivateCallback,
	      items[i].callback, items[i].callbackData);
	}
    }
    return (menuType == XmMENU_POPUP) ? menu : cascade;
}

/*
 * SIGNAL HANDLER
 *   function to perform cleanup of X root window MEDM... properties
 *   which accomodate remote display requests
 */
static void handleSignals(int sig)
{
    if(sig==SIGQUIT)      print("\nSIGQUIT\n");
    else if(sig==SIGINT)  print("\nSIGINT\n");
    else if(sig==SIGTERM) print("\nSIGTERM\n");
    else if(sig==SIGSEGV) print("\nSIGSEGV\n");
    else if(sig==SIGBUS)  print("\nSIGBUS\n");

  /* Remove the property on the root window (if not LOCAL) */
    if(windowPropertyAtom != (Atom)NULL)
      XDeleteProperty(display,rootWindow,windowPropertyAtom);
    XFlush(display);

  /* Exit */
    if(sig == SIGSEGV || sig == SIGBUS) {
      /* Exit with core dump */
	abort();
    } else {
      /* Just exit */
	exit(0);
    }
}

/*
 *  the function to take a full path name and macro string,
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
  char *geometryString,
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
    while(ptr[0] != '\0') {
	if(index == MAX_CHARS_IN_CLIENT_MESSAGE) {
	    XSendEvent(display,targetWindow,True,NoEventMask,
	      (XEvent *)&clientMessageEvent);
	    index = 0;
	}
	clientMessageEvent.data.b[index++] = ptr[0];
	ptr++;
    }

  /* ; delimiter */
    if(index == MAX_CHARS_IN_CLIENT_MESSAGE) {
	XSendEvent(display,targetWindow,True,NoEventMask,
	  (XEvent *)&clientMessageEvent);
	index = 0;
    }
    clientMessageEvent.data.b[index++] = ';';

  /* body of macro string if one was specified */
    if((ptr = macroString) != NULL) {
	while(ptr[0] != '\0') {
	    if(index == MAX_CHARS_IN_CLIENT_MESSAGE) {
		XSendEvent(display,targetWindow,True,NoEventMask,
		  (XEvent *)&clientMessageEvent);
		index = 0;
	    }
	    clientMessageEvent.data.b[index++] = ptr[0];
	    ptr++;
	}
    }

  /* ; delimiter */
    if(index == MAX_CHARS_IN_CLIENT_MESSAGE) {
	XSendEvent(display,targetWindow,True,NoEventMask,
	  (XEvent *)&clientMessageEvent);
	index = 0;
    }
    clientMessageEvent.data.b[index++] = ';';

  /* body of geometry string if one was specified */
    if((ptr = geometryString) != NULL) {
	while(ptr[0] != '\0') {
	    if(index == MAX_CHARS_IN_CLIENT_MESSAGE) {
		XSendEvent(display,targetWindow,True,NoEventMask,
		  (XEvent *)&clientMessageEvent);
		index = 0;
	    }
	    clientMessageEvent.data.b[index++] = ptr[0];
	    ptr++;
	}
    }

  /* trailing ")" */
    if(index == MAX_CHARS_IN_CLIENT_MESSAGE) {
	XSendEvent(display,targetWindow,True,NoEventMask,
	  (XEvent *)&clientMessageEvent);
	index = 0;
    }
    clientMessageEvent.data.b[index++] = ')';
  /* fill out client event with spaces just for "cleanliness" */
    for(i = index; i < MAX_CHARS_IN_CLIENT_MESSAGE; i++)
      clientMessageEvent.data.b[i] = ' ';
    XSendEvent(display,targetWindow,True,NoEventMask,
      (XEvent *)&clientMessageEvent);

}

/* This routines is used for file conversions only. */
/* KE: The error handling could be improved.  See dmDisplayListParse.  */
char token[MAX_TOKEN_LENGTH];
DisplayInfo* parseDisplayFile(char *filename) {
    DisplayInfo *displayInfo = NULL;
    FILE *filePtr;
    TOKEN tokenType;
    filePtr = fopen(filename,"r");
    if(filePtr) {
	displayInfo = (DisplayInfo *)malloc(sizeof(DisplayInfo));
	displayInfo->dlElementList = createDlList();
	currentDisplayInfo = displayInfo;
	displayInfo->filePtr = filePtr;
      /* if first token isn't "file" then bail out! */
	tokenType=getToken(displayInfo,token);
	if(tokenType == T_WORD && !strcmp(token,"file")) {
	    displayInfo->dlFile = parseFile(displayInfo);
	    if(displayInfo->dlFile) {
		displayInfo->versionNumber = displayInfo->dlFile->versionNumber;
		strcpy(displayInfo->dlFile->name,filename);
	    }
	} else {
	    fclose(filePtr);
	    return NULL;
	}
	tokenType=getToken(displayInfo,token);
	if(tokenType == T_WORD && !strcmp(token,"display")) {
	    parseDisplay(displayInfo);
	}
	tokenType=getToken(displayInfo,token);
	if(tokenType == T_WORD && (!strcmp(token,"color map") ||
	  !strcmp(token,"<<color map>>"))) {
	    displayInfo->dlColormap=parseColormap(displayInfo,displayInfo->filePtr);
	    tokenType=getToken(displayInfo,token);
	} else {
	    fclose(filePtr);
	    return NULL;
	}

      /* Proceed with parsing */
	while(parseAndAppendDisplayList(displayInfo,displayInfo->dlElementList,
	  token,tokenType) != T_EOF) {
	    tokenType=getToken(displayInfo,token);
	}
	displayInfo->filePtr=NULL;
	fclose(filePtr);
    }

    return displayInfo;
}

/**************************************************************************/
/**************************** main ****************************************/
/**************************************************************************/
int main(int argc, char *argv[])
{
    int i, n, index;
    Arg args[5];
    FILE *filePtr;
    XColor color;
    XEvent event;
    char versionString[60];
    Window targetWindow;
    Boolean attachToExistingMedm, completeClientMessage;
    char fullPathName[PATH_MAX], name[PATH_MAX];
    unsigned char *propertyData;
    int status, format;
    unsigned long nitems, left;
    Atom type;
    char *ptr = NULL;
    XColor colors[2];
    request_t *request;
    typedef enum {FILENAME_MSG,MACROSTR_MSG,GEOMETRYSTR_MSG} msgClass_t;
    msgClass_t msgClass;
    Window medmHostWindow = (Window)0;
    char *envPrintCommand = NULL;
    char *envHelpPath = NULL;

#ifdef WIN32
  /* Hummingbird Exceed XDK initialization for WIN32 */
    HCLXmInit();
#endif

#if DEBUG_PROP
    print("Starting MEDM\n");
#endif

#if DEBUG_WIN32_LEAKS
#ifdef WIN32
#ifdef _DEBUG
    {
	int tmpDbgFlag;

	/* Get the flag */
	tmpDbgFlag = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	/* Enable memory block checking (Should be default) */
	tmpDbgFlag |= _CRTDBG_ALLOC_MEM_DF;
	/* Enable checking at every allocation (slower but pinpoints error) */
	tmpDbgFlag |= _CRTDBG_CHECK_ALWAYS_DF;
	/* Enable keeping freed memory to check overwriting it */
	tmpDbgFlag |= _CRTDBG_DELAY_FREE_MEM_DF;
	/* Enable memory leak report at exit */
	tmpDbgFlag |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(tmpDbgFlag);
/*	_CrtSetBreakAlloc(84); */
    }
#endif
#endif
#endif


#if DEBUG_DEFINITIONS

    print("\n");
#ifdef __EXTENSIONS__
    print("__EXTENSIONS__= % d\n",__EXTENSIONS__);
#else
    print("__EXTENSIONS__ is undefined\n");
#endif
#ifdef __STDC__
    print("__STDC__ = %d\n",__STDC__);
    print("__STDC__ - 0 = %d\n",__STDC__ - 0);
#else
    print("__STDC__ is undefined\n");
    print("__STDC__ - 0 = %d\n",__STDC__ - 0);
#endif
#ifdef _POSIX_C_SOURCE
    print("_POSIX_C_SOURCE = %d\n",_POSIX_C_SOURCE);
#else
    print("_POSIX_C_SOURCE is undefined\n");
#endif
#ifdef _XOPEN_SOURCE
    print("_XOPEN_SOURCE = %d\n",_XOPEN_SOURCE);
#else
    print("_XOPEN_SOURCE is undefined\n");
#endif
#ifdef _NO_LONGLONG
    print("_NO_LONGLONG = %d\n",_NO_LONGLONG);
#else
    print("_NO_LONGLONG is undefined\n");
#endif
#ifdef NeedFunctionPrototypes
    print("NeedFunctionPrototypes = %d\n",NeedFunctionPrototypes);
#else
    print("NeedFunctionPrototypes is undefined\n");
#endif
    print("sizeof(1 && 2)=%d\n",sizeof(1 && 2));
    print("sizeof(Boolean)=%d\n",sizeof(Boolean));
    printf("This came from printf\n");
    fprintf(stdout,"This came from fprintf to stdout\n");

#endif     /*  DEBUG_DEFINITIONS */

  /* Initialize global variables */
    maxLabelWidth = 0;
    maxLabelHeight = 0;
    dashes = "******";
    cpMatrix = cpForm = NULL;
    medmInitializeUpdateTasks();
    updateTaskExposedRegion =  XCreateRegion();
    windowPropertyAtom = (Atom)NULL;
    medmWorkProcId = 0;
    medmUpdateRequestCount = 0;
    medmCAEventCount = 0;
    medmScreenUpdateCount = 0;
    medmUpdateMissedCount = 0;
    MedmUseNewFileFormat = True;
    medmRaiseMessageWindow = 1;
    popupExistingDisplay = POPUP_EXISTING_DISPLAY;
    setTimeValues();
    printOrientation = DEFAULT_PRINT_ORIENTATION;
    printSize =  DEFAULT_PRINT_SIZE;
    printToFile = DEFAULT_PRINT_TOFILE;
    printTitle = DEFAULT_PRINT_TITLE;
    printTime = DEFAULT_PRINT_TIME;
    printDate = DEFAULT_PRINT_DATE;
    printWidth = printHeight = 0.0;
    printRemoveTempFiles = PRINT_REMOVE_TEMP_FILES;
    strcpy(printFile, DEFAULT_PRINT_FILENAME);
    strcpy(printTitleString, DEFAULT_PRINT_TITLE_STRING);
    envPrintCommand = getenv("MEDM_PRINT_CMD");
    if(envPrintCommand != NULL) {
	strcpy(printCommand, envPrintCommand);
    } else {
	strcpy(printCommand, DEFAULT_PRINT_CMD);
    }

  /* Help URL */
    envHelpPath = getenv("MEDM_HELP_PATH");
    if(envHelpPath != NULL) {
	strncpy(medmHelpPath, envHelpPath, PATH_MAX);
    } else {
	strncpy(medmHelpPath, MEDM_HELP_PATH, PATH_MAX);
    }
    medmHelpPath[PATH_MAX-1]='\0';

  /* XWD file name */
#ifdef WIN32
    {
      /* We don't know the location of the temp directory beforehand.  Get
	 it from the environment variable. */
	char *tempDir = getenv("TEMP");
	if(!tempDir) tempDir = getenv("TMP");
	if(tempDir) {
	    sprintf(xwdFile,"%s\\%s",tempDir,PRINT_XWD_FILE);
	} else {
	    strcpy(xwdFile,PRINT_XWD_FILE);
	}
    }
#else
    strcpy(xwdFile,PRINT_XWD_FILE);
#endif

  /* Handle file conversions */
    if(argc == 4 && (!strcmp(argv[1],"-c21x") ||
      !strcmp(argv[1],"-c22x"))) {
	DisplayInfo *displayInfo = NULL;
	FILE *filePtr;
	initMedmCommon();
	displayInfo = parseDisplayFile(argv[2]);
	if(displayInfo) {
	  /* Open for writing (Use w+ or WIN32 makes it readonly) */
	    filePtr = fopen(argv[3],"w+");
	    if(filePtr) {
		strcpy(displayInfo->dlFile->name,argv[3]);
		if(!strcmp(argv[1],"-c21x")) {
		    MedmUseNewFileFormat = False;
		} else {
		    MedmUseNewFileFormat = True;
		}
		dmWriteDisplayList(displayInfo,filePtr);
		fclose(filePtr);
	    } else {
		medmPrintf(1,"\nCannot create display file: \"%s\"\n",argv[3]);
	    }
	} else {
	    medmPrintf(1,"\nCannot open display file: \"%s\"\n",argv[2]);
	}
	return 0;
    }

#ifndef MEDM_CDEV
   /* Initialize channel access here (to get around orphaned windows) */
    status = ca_task_initialize();
    if(status != ECA_NORMAL) {
	medmPostMsg(1,"main: ca_task_initialize failed: %s\n",
	  ca_message(status));
    }
#endif

  /* Parse command line */
    request = parseCommandLine(argc,argv);

    if(request->macroString != NULL && request->opMode != EXECUTE) {
	medmPrintf(0,"\nIgnored -macro command line option\n"
	  "  (Only valid for Execute (-x) mode operation)\n");
	free(request->macroString);
	request->macroString = NULL;
    }

  /* Usage and error exit */
    if(request->opMode == HELP) {
	print("\n%s\n",MEDM_VERSION_STRING);
	print("Usage:\n"
	  "  medm [X options]\n"
	  "  [-help | -h | -?]\n"
	  "  [-version]\n"
	  "  [-x | -e]\n"
	  "  [-local | -attach | -cleanup]\n"
	  "  [-cmap]\n"
	  "  [-bigMousePointer]\n"
	  "  [-noMsg]\n"
	  "  [-displayFont font-spec]\n"
	  "  [-macro \"xxx=aaa,yyy=bbb, ...\"]\n"
	  "  [-dg [xpos[xypos]][+xoffset[+yoffset]]\n"
	  "  [display-files]\n"
	  "  [&]\n"
	  "\n");
	exit(0);
    } else if(request->opMode == VERSION) {
	print("\n%s\n",MEDM_VERSION_STRING);
	exit(0);
    }

  /* Do remote protocol stuff if not LOCAL */
    if(request->medmMode != LOCAL) {
      /* Open display */
	display = XOpenDisplay(request->displayName);
	if(display == NULL) {
	    medmPrintf(1,"\nCould not open Display\n");
	    exit(1);
	}
	screenNum = DefaultScreen(display);
	rootWindow = RootWindow(display,screenNum);

      /* Intern the appropriate atom if it doesn't exist (i.e. use
           False) */
	if(request->fontStyle == FIXED_FONT) {
	    if(request->opMode == EXECUTE) {
		windowPropertyAtom = XInternAtom(display,
		  MEDM_VERSION_DIGITS"_EXEC_FIXED",False);
	    } else {
		windowPropertyAtom = XInternAtom(display,
		  MEDM_VERSION_DIGITS"_EDIT_FIXED",False);
	    }
	} else if(request->fontStyle == SCALABLE_FONT) {
	    if(request->opMode == EXECUTE) {
		windowPropertyAtom = XInternAtom(display,
		  MEDM_VERSION_DIGITS"_EXEC_SCALABLE",False);
	    } else {
		windowPropertyAtom = XInternAtom(display,
		  MEDM_VERSION_DIGITS"_EDIT_SCALABLE",False);
	    }
	}

      /* Get the property  (Should a the mainShell window number)
       *  type:          Actual type of the property
       *                 None if it doesn't exist
       *  propertyData:  The value of the property */
	status = XGetWindowProperty(display, rootWindow, windowPropertyAtom,
	  0, PATH_MAX, (Bool)False, AnyPropertyType, &type,
	  &format, &nitems, &left, &propertyData);

#if DEBUG_PROP
	{
	    char *atomName;

	    atomName = XGetAtomName(display, windowPropertyAtom);
	    print("\nAfter XInternAtom and XGetWindowProperty\n");
	    print("atomName(1)=|%s| atom=%d\n",atomName?atomName:"NULL",
	      windowPropertyAtom);
	}
	print("\nXGetWindowProperty(1): "
	  "status=%d type=%ld format=%d nitems=%ld left=%ld"
	  "  windowPropertyAtom=%d propertyData=%x\n",
	  status,type,format,nitems,left,windowPropertyAtom,
	  propertyData?*(long *)propertyData:0);
#endif

      /* Decide whether to attach to existing MEDM */
	if(type != None) {
	    medmHostWindow = *(Window *)propertyData;
	    attachToExistingMedm = (request->medmMode == CLEANUP) ? False : True;
	    XFree(propertyData);
	} else {
	    attachToExistingMedm = False;
	}

      /* Attach to existing MEDM if appropriate
       *   Note that we only know there is a property
       *   We do not know if there actually is an MEDM running */
	if(attachToExistingMedm) {
	    XWindowAttributes attr;
	    char *fileStr;
	    int i, status;

	  /* Check if the medmHostWindow is valid */
	    XSetErrorHandler(xErrorHandler);     /* Otherwise exits */
	    status=XGetWindowAttributes(display,medmHostWindow,&attr);
	    if(!status) {
	      /* Window doesn't exist */
		print("\nCannot connect to existing MEDM because it is invalid\n"
 		  "  (An accompanying Bad Window error can be ignored)\n"
		  "  Continuing with this one as if -cleanup were specified\n");
		print("(Use -local to not use existing MEDM "
		  "or be available as an existing MEDM\n"
		  "  or -cleanup to set this MEDM as the existing one)\n");
	    } else {
	      /* Window does exist */
	      /* Check if there were valid display files specified */
		if(request->fileCnt > 0) {
		    print("\nAttaching to existing MEDM\n");
		    for(i=0; i<request->fileCnt; i++) {
			fileStr = request->fileList[i];
			if(fileStr) {
			    sendFullPathNameAndMacroAsClientMessages(
			      medmHostWindow,fileStr,
			      request->macroString,request->displayGeometry,
			      windowPropertyAtom);
			    XFlush(display);
			    print("  Dispatched: %s\n",fileStr);
			}
		    }
		} else {
		    print("\nAborting: No valid display specified and already "
		      "a remote MEDM running.\n");
		}
		print("(Use -local to not use existing MEDM or be available "
		  "as an existing MEDM\n"
		  "  or -cleanup to set this MEDM as the existing one)\n");

	      /* Leave this MEDM */
		XCloseDisplay(display);
		exit(0);
	    }
	}

#if DEBUG_PROP
	{
	    char *atomName;

	    atomName = XGetAtomName(display, windowPropertyAtom);
	    print("\nBefore XCloseDisplay\n");
	    print("atomName(2)=|%s| atom=%d\n",atomName?atomName:"NULL",
	      windowPropertyAtom);
	}
#endif

      /* Close the display that was opened (Will start over later) */
	XCloseDisplay(display);
    } /* End if(request->medmMode != LOCAL) */

  /* Initialize the Intrinsics
   *   Create mainShell
   *   Map window manager menu Close function to do nothing
   *     (Will handle this ourselves) */
#if DEBUG_PROP
	{
	    char *atomName;

	    atomName = XGetAtomName(display, windowPropertyAtom);
	    print("\nBefore XtAppInitailize\n");
	    print("atomName(3)=|%s| atom=%d\n",atomName?atomName:"NULL",
	      windowPropertyAtom);
	}
#endif
    n = 0;
    XtSetArg(args[n],XmNdeleteResponse,XmDO_NOTHING); n++;
#if OMIT_RESIZE_HANDLES
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
  /* KE: The following is necessary for Exceed, which turns off the
     resize function with the handles.  It should not be necessary */
    XtSetArg(args[n],XmNmwmFunctions, MWM_FUNC_ALL); n++;
#endif
    mainShell = XtAppInitialize(&appContext, CLASS, NULL, 0, &argc, argv,
      fallbackResources, args, n);

  /* Set error handlers */
#if DEBUG_ERRORHANDLER
    {
	XtErrorHandler errHandler,warnHandler;
	printf("MEDM: Set error handlers: appContext=%p\n",(void *)appContext);
	errHandler=XtAppSetErrorHandler(appContext,xtErrorHandler);
	warnHandler=XtAppSetWarningHandler(appContext,xtErrorHandler);
	printf(" errHandler=%p xtErrorHandler=%p\n",
	  (void *)errHandler,(void *)xtErrorHandler);
	printf(" warnHandler=%p xtErrorHandler=%p\n",
	  (void *)warnHandler,(void *)xtErrorHandler);
	errHandler=XtAppSetErrorHandler(appContext,xtErrorHandler);
	warnHandler=XtAppSetWarningHandler(appContext,xtErrorHandler);
	printf(" errHandler=%p xtErrorHandler=%p\n",
	  (void *)errHandler,(void *)xtErrorHandler);
	printf(" warnHandler=%p xtErrorHandler=%p\n",
	  (void *)warnHandler,(void *)xtErrorHandler);
    }
#else
    XtAppSetErrorHandler(appContext,xtErrorHandler);
    XtAppSetWarningHandler(appContext,xtErrorHandler);
#endif
    XSetErrorHandler(xErrorHandler);

  /* Enable Editres */
#ifdef EDITRES
    XtAddEventHandler(mainShell,(EventMask)NULL,TRUE,
      (XtEventHandler)_XEditResCheckMessages,NULL);
#endif

#if 0
  /* KE: This doesn't appear in the documentation.
   *   Assume it is not needed any more. */
  /* Add necessary Motif resource converters */
    XmRegisterConverters();
#endif

  /* Needed to specify XmNtearOffModel in resources */
    XmRepTypeInstallTearOffModelConverter();

  /* Set display and related quantities */
    display = XtDisplay(mainShell);
    if(display == NULL) {
	XtWarning("MEDM initialization: Cannot open display");
	exit(-1);
    }
#if DEBUG_PROP
	{
	    char *atomName;

	    atomName = XGetAtomName(display, windowPropertyAtom);
	    print("\nAfter XtDisplay\n");
	    print("atomName(4)=|%s| atom=%d\n",atomName?atomName:"NULL",
	      windowPropertyAtom);
	}
#endif
    screenNum = DefaultScreen(display);
    rootWindow = RootWindow(display,screenNum);
    cmap = DefaultColormap(display,screenNum);	/* X default colormap */

      /* Set XSynchronize for debugging */
#if DEBUG_SYNC
    XSynchronize(display,TRUE);
    medmPrintf(0,"\nRunning in SYNCHRONOUS mode\n");
#endif

  /* Intern some atoms if they aren't there already */
    WM_DELETE_WINDOW = XmInternAtom(display,"WM_DELETE_WINDOW",False);
    WM_TAKE_FOCUS = XmInternAtom(display,"WM_TAKE_FOCUS",False);
#if USE_DRAGDROP
  /* These are the drag & drop exports we support */
  /* Note that XA_STRING is predefined (Xatom.h) and doesn't have to
     be interned */
    compoundTextAtom = XmInternAtom(display,"COMPOUND_TEXT",False);
    textAtom = XmInternAtom(display,"TEXT",False);
#endif

  /* Register signal handlers so we can shut down ourselves
   *   (Unfortunately SIGKILL, SIGSTOP can't be caught...) */
#if defined(__cplusplus) && !defined(__GNUG__)
    signal(SIGQUIT,(SIG_PF)handleSignals);
    signal(SIGINT, (SIG_PF)handleSignals);
    signal(SIGTERM,(SIG_PF)handleSignals);
    signal(SIGSEGV,(SIG_PF)handleSignals);
    signal(SIGBUS,(SIG_PF)handleSignals);
#else
    signal(SIGQUIT,handleSignals);
    signal(SIGINT, handleSignals);
    signal(SIGTERM,handleSignals);
    signal(SIGSEGV,handleSignals);
    signal(SIGBUS, handleSignals);
#endif

#if USE_DRAGDROP
  /* Add translations/actions for drag-and-drop */
    parsedTranslations = XtParseTranslationTable(dragTranslations);
    XtAppAddActions(appContext,dragActions,XtNumber(dragActions));
#endif

    if(request->opMode == EDIT) {
	globalDisplayListTraversalMode = DL_EDIT;
    } else if(request->opMode == EXECUTE) {
	globalDisplayListTraversalMode = DL_EXECUTE;
	if(request->fileCnt > 0) {	/* assume .adl file names follow */
	    XtVaSetValues(mainShell, XmNinitialState, IconicState, NULL);
	}
      /* Start the scheduler */
	startMedmScheduler();
    } else {
	globalDisplayListTraversalMode = DL_EDIT;
    }

  /* Initialize some globals */
    globalModifiedFlag = False;
    mainMW = NULL;
    objectS = NULL; objectMW = NULL;
    colorS = NULL; colorMW = NULL;
    resourceS = NULL;
    resourceMW = NULL;
    channelS = NULL;
    channelMW = NULL;
    relatedDisplayS = NULL;
    shellCommandS = NULL;
    cmdForm = NULL;
    cartesianPlotS = NULL;
    cartesianPlotAxisS = NULL;
    stripChartS = NULL;
    cpAxisForm = NULL;
    executeTimeCartesianPlotWidget = NULL;
    executeTimePvLimitsElement = NULL;
    executeTimeStripChartElement = NULL;
    currentDisplayInfo = NULL;
    pointerInDisplayInfo = NULL;
    resourceBundleCounter = 0;
    currentElementType = 0;
  /* Not really unphysical, but being used for unallocable color cells */
    unphysicalPixel = BlackPixel(display,screenNum);
  /* Set default action for MB in display to select
   *   (Afterward regulated by object palette) */
    currentActionType = SELECT_ACTION;

  /* Initialize the private colormap if there is one */
    if(request->privateCmap) {
      /* Cheap/easy way to get colormap - do real PseudoColor cmap alloc later
       *   Note this really creates a colormap for default visual with no
       *     entries */
	cmap = XCopyColormapAndFree(display,cmap);
	XtVaSetValues(mainShell,XmNcolormap,cmap,NULL);

      /* Add in black and white pixels to match [Black/White]Pixel(dpy,scr) */
	colors[0].pixel = BlackPixel(display,screenNum);
	colors[1].pixel = WhitePixel(display,screenNum);
	XQueryColors(display,DefaultColormap(display,screenNum),colors,2);
      /* Need to allocate 0 pixel first, then 1 pixel, usually Black, White...
       *   note this is slightly risky in case of non Pseudo-Color visuals I think,
       *   but the preallocated colors of Black=0, White=1 for Psuedo-Color
       *   visuals is common, and only for Direct/TrueColor visuals will
       *   this be way off, but then we won't be using the private colormap
       *   since we won't run out of colors in that instance...  */
	if(colors[0].pixel == 0) XAllocColor(display,cmap,&(colors[0]));
	else XAllocColor(display,cmap,&(colors[1]));
	if(colors[1].pixel == 1) XAllocColor(display,cmap,&(colors[1]));
	else XAllocColor(display,cmap,&(colors[0]));
    }

  /* Allocate colors */
    for(i = 0; i < DL_MAX_COLORS; i++) {
      /* Scale [0,255] to [0,65535] */
	color.red  = (unsigned short) COLOR_SCALE*(defaultDlColormap.dl_color[i].r);
	color.green= (unsigned short) COLOR_SCALE*(defaultDlColormap.dl_color[i].g);
	color.blue = (unsigned short) COLOR_SCALE*(defaultDlColormap.dl_color[i].b);
      /* Allocate a shareable color cell with closest RGB value */
	if(XAllocColor(display,cmap,&color)) {
	    defaultColormap[i] =  color.pixel;
	} else {
	    medmPrintf(1,"\nmain: Cannot not allocate color (%d: "
	      "r=%d  g=%d  b=%d)\n",i,defaultDlColormap.dl_color[i].r,
	      defaultDlColormap.dl_color[i].g,defaultDlColormap.dl_color[i].b);
	  /* Put unphysical pixmap value in there as tag it was invalid */
	    defaultColormap[i] =  unphysicalPixel;
	}
    }
    currentColormap = defaultColormap;
    currentColormapSize = DL_MAX_COLORS;

  /* Initialize the global resource bundle */
    initializeGlobalResourceBundle();
    globalResourceBundle.next = NULL;
    globalResourceBundle.prev = NULL;


  /* Intialize MEDM stuff */
    medmInit(request->displayFont);
    medmInitializeImageCache();
    createCursors();
    initializeRubberbanding();
    createMain();
    disableEditFunctions();
    initMedmCommon();
    initEventHandlers();
    initMedmWidget();

#if DEBUG_VERSION
    print("ServerVendor=%s\n", ServerVendor(display));
    print("VendorRelease=%d\n", VendorRelease(display));
    print("ProtocolVersion=%d\n", ProtocolVersion(display));
    print("ProtocolRevision=%d\n", ProtocolRevision(display));
#endif

  /* We're the first MEDM around in this mode - proceed with full
    execution.  Store mainShell window as the property associated with
    the windowPropertyAtom.  (Will be stored if CLEANUP or first MEDM.
    Won't be stored if LOCAL.)  We need to reintern the atom as it may
    have been lost when we closed the display above if we are the only
    X connection (likely on WIN32).  (Nothing changes unless it was
    lost, since we use False.) */
    if(request->medmMode != LOCAL) {
	if(request->fontStyle == FIXED_FONT) {
	    if(request->opMode == EXECUTE) {
		windowPropertyAtom = XInternAtom(display,
		  MEDM_VERSION_DIGITS"_EXEC_FIXED",False);
	    } else {
		windowPropertyAtom = XInternAtom(display,
		  MEDM_VERSION_DIGITS"_EDIT_FIXED",False);
	    }
	} else if(request->fontStyle == SCALABLE_FONT) {
	    if(request->opMode == EXECUTE) {
		windowPropertyAtom = XInternAtom(display,
		  MEDM_VERSION_DIGITS"_EXEC_SCALABLE",False);
	    } else {
		windowPropertyAtom = XInternAtom(display,
		  MEDM_VERSION_DIGITS"_EDIT_SCALABLE",False);
	    }
	}
    }
    targetWindow = XtWindow(mainShell);
    if(windowPropertyAtom) {
#if DEBUG_PROP
	{
	    char *atomName;

	    atomName = XGetAtomName(display, windowPropertyAtom);
	    print("\nBefore XChangeProperty\n");
	    print("atomName(5)=|%s| atom=%d\n",atomName?atomName:"NULL",
	      windowPropertyAtom);
	}
	status = XGetWindowProperty(display,rootWindow,windowPropertyAtom,
	  0,FULLPATHNAME_SIZE,(Bool)False,AnyPropertyType,&type,
	  &format,&nitems,&left,&propertyData);
	print("\nXGetWindowProperty(2): status=%d type=%ld format=%d nitems=%ld left=%ld"
	  "  windowPropertyAtom=%d propertyData=%x\n",
	  status,type,format,nitems,left,windowPropertyAtom,
	  	  propertyData?*(long *)propertyData:0);
	print("\nChanged window property: windowPropertyAtom=%d targetWindow=%x\n",
	  windowPropertyAtom,targetWindow);
#endif
	XChangeProperty(display,rootWindow,windowPropertyAtom,
	  XA_WINDOW,32,PropModeReplace,(unsigned char *)&targetWindow,1);
#if DEBUG_PROP
	status = XGetWindowProperty(display,rootWindow,windowPropertyAtom,
	  0,FULLPATHNAME_SIZE,(Bool)False,AnyPropertyType,&type,
	  &format,&nitems,&left,&propertyData);
	print("\nXGetWindowProperty(3): status=%d type=%ld format=%d nitems=%ld left=%ld"
	  "  windowPropertyAtom=%d propertyData=%x\n",
	  status,type,format,nitems,left,windowPropertyAtom,
	  propertyData?*(long *)propertyData:0);
#endif
    }

  /* Start any command-line specified displays */
    for(i=0; i < request->fileCnt; i++) {
	char *fileStr;
	fileStr = request->fileList[i];
	if(fileStr) {
	    filePtr = fopen(fileStr,"r");
	    if(filePtr) {
		dmDisplayListParse(NULL,filePtr,request->macroString,fileStr,
		  request->displayGeometry,(Boolean)False);
		fclose(filePtr);
	    } else {
		medmPrintf(1,"\nCannot open display file: \"%s\"\n",fileStr);
	    }
	}
    }
    if((displayInfoListHead->next) &&
      (globalDisplayListTraversalMode == DL_EDIT)) {
	enableEditFunctions();
    }

  /* Create and popup the product description shell
   *  (use defaults for fg/bg) */
#ifdef MEDM_CDEV
    sprintf(versionString,"%s  (%s)",MEDM_VERSION_STRING,CDEV_VERSION_STRING);
    productDescriptionShell = createAndPopupProductDescriptionShell(appContext,
      mainShell,
      "MEDM", fontListTable[8],
      (Pixmap)NULL,
      "Motif-Based Editor & Display Manager",
      fontListTable[6],
      versionString,
      "Developed at Argonne National Laboratory\n"
      "     by Mark Anderson, Fred Vong, & Ken Evans\n"
      "Extended for CDEV at Jefferson Laboratory\n"
      "     by Jie Chen",
     fontListTable[4],
      -1, -1, 5);
#else
    sprintf(versionString,"%s  (%s)",MEDM_VERSION_STRING,EPICS_VERSION_STRING);
    productDescriptionShell = createAndPopupProductDescriptionShell(appContext,
      mainShell,
      "MEDM", fontListTable[8],
      (Pixmap)NULL,
      "Motif-Based Editor & Display Manager",
      fontListTable[6],
      versionString,
      "Developed at Argonne National Laboratory\n"
      "     by Mark Anderson, Fred Vong, & Ken Evans",
      fontListTable[4],
      -1, -1, 5);
#endif

  /* Add callback for disabled window manager Close function
   *   Need this later than shell creation for some reason (?) */
    XmAddWMProtocolCallback(mainShell,WM_DELETE_WINDOW,
      wmCloseCallback, (XtPointer) OTHER_SHELL);

  /* Check if there were early messages and bring up Message Window */
    checkEarlyMessages();

  /* Print that we are started in the message window which will cause
   * it to get created */
    medmPrintf(0,"\nStarting main loop\n");

#ifdef __TED__
  /* Get CDE workspace list */
    GetWorkSpaceList(mainMW);
#endif

  /* Go into event loop
   *   Normally just XtAppMainLoop(appContext)
   *     but we want to handle remote requests from other MEDM's */
  /* KE: Could have done this with XtAppMainLoop(appContext) and event
   *   handler for ClientMessage events ? */
    while(True) {
#if 0
      /* KE: This causes the program to hang for Btn3 Press */
	XPeekEvent(display, &event);
	switch (event.type) {
	case ButtonPress:
	case ButtonRelease: {
	    XButtonEvent bEvent = event.xbutton;

	    print("\nXLIB EVENT: Type: %-7s  Button: %d  Window %x  SubWindow: %x\n"
	      "  Shift: %s  Ctrl: %s\n",
	      (bEvent.type == ButtonPress)?"ButtonPress":"ButtonRelease",
	      bEvent.button, bEvent.window, bEvent.subwindow,
	      bEvent.state&ShiftMask?"Yes":"No",
	      bEvent.state&ControlMask?"Yes":"No");
	    print("  Send_event: %s  State: %x\n",
	      bEvent.send_event?"True":"False",bEvent.state);

	    break;
	}
	}
#endif
	XtAppNextEvent(appContext,&event);
#if DEBUG_ALLEVENTS
	{
	    static int afterButtonPress=0;
	    XAnyEvent aEvent=event.xany;
	    time_t now;
	    struct tm *tblock;
	    char timeStampStr[80];
	    XWindowAttributes attr;
	    Status status;

	    time(&now);
	    tblock = localtime(&now);
	    strftime(timeStampStr,80,"%H:%M:%S",tblock);

	  /* Reset the timer if it is a ButtonPress event */
	    if(event.type == ButtonPress) {
		resetTimer();
		afterButtonPress=1;
	    }

	    if(afterButtonPress) {
	      /* Reset the error handler so it won't bomb on BadWindow */
		XSetErrorHandler(xDoNothingErrorHandler);

	      /* Get the window attributes */
		status=XGetWindowAttributes(display,aEvent.window,&attr);
		if(status == 0) {
		    print("%8.3f %s %6d %s %08x Error               %-18s\n",
		      getTimerDouble(),timeStampStr,aEvent.serial,
		      aEvent.send_event?"Yes":"No ",aEvent.window,
		      getEventName(aEvent.type));
		} else {
		    print("%8.3f %s %6d %s %08x %4d %4d %4d %4d %-18s\n",
		      getTimerDouble(),timeStampStr,
		      aEvent.serial,
		      aEvent.send_event?"Yes":"No ",aEvent.window,
		      attr.x, attr.y,attr.width,attr.height,
		      getEventName(aEvent.type));
		}

	      /* Reset afterButtonPress if it is a DestroyNotify */
		if(event.type == DestroyNotify) afterButtonPress=0;
	    }
	}
#endif
	switch (event.type) {
	case ClientMessage:
	    if(windowPropertyAtom && event.xclient.message_type == windowPropertyAtom) {
	      /* Request from remote MEDM */
		char geometryString[256];

	      /* Concatenate ClientMessage events to get full name from form: (xyz) */
		completeClientMessage = False;
		for(i = 0; i < MAX_CHARS_IN_CLIENT_MESSAGE; i++) {
		    switch (event.xclient.data.b[i]) {
		      /* Start with filename */
		    case '(':
			index = 0;
			ptr = fullPathName;
			msgClass = FILENAME_MSG;
			break;
		      /* Keep filling in until ';', then start macro string if any */
		    case ';':
			ptr[index++] = '\0';
			if(msgClass == FILENAME_MSG) {
			    msgClass = MACROSTR_MSG;
			    ptr = name;
			} else {
			    msgClass = GEOMETRYSTR_MSG;
			    ptr = geometryString;
			}
			index = 0;
			break;
		      /* Terminate whatever string is being filled in */
		    case ')':
			completeClientMessage = True;
			ptr[index++] = '\0';
			break;
		    default:
			ptr[index++] = event.xclient.data.b[i];
			break;
		    }
		}

	      /* If the message is complete, then process the request */
		if(completeClientMessage) {
		    DisplayInfo *existingDisplayInfo = NULL;

		  /* Post a message about the request */
		    medmPostMsg(0,"File Dispatch Request:\n");
		    if(fullPathName[0] != '\0')
		      medmPrintf(0,"  filename = %s\n",fullPathName);
		    if(name[0] != '\0')
		      medmPrintf(0,"  macro = %s\n",name);
		    if(geometryString[0] != '\0')
		      medmPrintf(0,"  geometry = %s\n",geometryString);

		  /* Check if a display with these parameters exists */
		    if(popupExistingDisplay) {
			existingDisplayInfo = findDisplay(fullPathName,
			  name, NULL);
		    }
		    if(existingDisplayInfo) {
			DisplayInfo *cdi;

			cdi = currentDisplayInfo = existingDisplayInfo;
#if 0
		      /* KE: Doesn't work on WIN32 */
			XtPopdown(currentDisplayInfo->shell);
			XtPopup(currentDisplayInfo->shell, XtGrabNone);
#else
			if(cdi && cdi->shell && XtIsRealized(cdi->shell)) {
			    XMapRaised(display, XtWindow(cdi->shell));
			}
#endif
			medmPrintf(0,
			  "  Found existing display with same parameters\n");
		    } else {
		      /* Open the file */
			filePtr = fopen(fullPathName,"r");
			if(filePtr) {
			    dmDisplayListParse(NULL, filePtr, name, fullPathName,
			      geometryString, (Boolean)False);
			    fclose(filePtr);
			    if(globalDisplayListTraversalMode == DL_EDIT) {
				enableEditFunctions();
			    }
			} else {
			    medmPrintf(1,
			      "  Could not open requested file\n");
			}
		    }
		}     /* if(completeClientMessage) */
	    } else {
	      /* Handle these ClientMessage's the normal way */
		XtDispatchEvent(&event);
	    }
	    break;
#if DEBUG_EVENTS
	case KeyPress:
	case KeyRelease: {
	    XKeyEvent xEvent = event.xkey;
	    Modifiers modifiers;
	    KeySym keysym;
	    char buffer[10];
	    int nbytes;
	    Widget w;
	    Window win;
	    int i = 0;

	    for(i=0; i < 10; i++) buffer[i] = '\0';
	    nbytes = XLookupString(&xEvent, buffer, 10,
	      &keysym, NULL);

	    print("\nEVENT: Type: %-7s  Keycode: %d  Window %x  SubWindow: %x\n"
	      "  Key: %s Shift: %s  Ctrl: %s\n",
	      (xEvent.type == KeyPress)?"KeyPress":"KeyRelease",
	      xEvent.keycode, xEvent.window, xEvent.subwindow,
	      buffer,
	      xEvent.state&ShiftMask?"Yes":"No",
	      xEvent.state&ControlMask?"Yes":"No");
	    print("  Send_event: %s  State: %x\n",
	      xEvent.send_event?"True":"False",xEvent.state);

	    XtTranslateKeycode(display, xEvent.keycode, (Modifiers)NULL,
	      &modifiers, &keysym);

	    switch (keysym) {
	    case osfXK_Left:
		print("  Keysym: %s\n","osfXK_Left");
		break;
	    case osfXK_Right:
		print("  Keysym: %s\n","osfXK_Right");
		break;
	    case osfXK_Up:
		print("  Keysym: %s\n","osfXK_Up");
		break;
	    case osfXK_Down:
		print("  Keysym: %s\n","osfXK_Down");
		break;
	    default:
		print("  Keysym: %s\n","Undetermined");
		break;
	    }

	    XtDispatchEvent(&event);
	    break;
	}

	case ButtonPress:
	case ButtonRelease: {
	    XButtonEvent xEvent = event.xbutton;
	    Widget w;
	    Window win;
	    int i = 0;

	    print("\nEVENT: Type: %-7s  Button: %d  Window %x  SubWindow: %x\n"
	      "  Shift: %s  Ctrl: %s\n",
	      (xEvent.type == ButtonPress)?"ButtonPress":"ButtonRelease",
	      xEvent.button, xEvent.window, xEvent.subwindow,
	      xEvent.state&ShiftMask?"Yes":"No",
	      xEvent.state&ControlMask?"Yes":"No");
	    print("  Send_event: %s  State: %x\n",
	      xEvent.send_event?"True":"False",xEvent.state);

#if 0
	    if(xEvent.subwindow) win=xEvent.subwindow;
	    else win=xEvent.window;
	    w=XtWindowToWidget(display,win);
	    print("\nHierarchy:\n");
	    while(1) {
		print("%4d %x",i++,win);
		if(w == mainShell) {
		    print(" (mainShell)\n");
		    break;
		} else if(win == xEvent.window) {
		    print(" (window)\n");
		} else if(win == xEvent.subwindow) {
		    print(" (subwindow)\n");
		} else {
		    print("\n");
		}
		w=XtParent(w);
		win=XtWindow(w);
	    }

	    printEventMasks(display, xEvent.window, "\n[window] ");
	    printEventMasks(display, xEvent.subwindow, "\n[subwindow] ");
#endif

	    XtDispatchEvent(&event);
	    break;
	}

#endif
	default:
	  /* Handle all other event types the normal way */
	    XtDispatchEvent(&event);
	}
    }
}

void createEditModeMenu(DisplayInfo *displayInfo)
{
    Widget w;

    displayInfo->editPopupMenu = buildMenu(displayInfo->drawingArea,
      XmMENU_POPUP, "editModeMenu",'\0',editModeMenu);

  /* Disable the tear off
   *    KE: Unmanaging the tear-off is a kluge.  Without it, Motif leaves
   *      the space where the dotted line was, not the first time, but after
   *      returning from EDIT mode */
    w = XmGetTearOffControl(displayInfo->editPopupMenu);
    if(w) XtUnmanageChild(w);
    XtVaSetValues(displayInfo->editPopupMenu,
      XmNtearOffModel, XmTEAR_OFF_DISABLED,
      XmNuserData, displayInfo,
      NULL);
}

static void createMain()
{
    XmString label;
    Widget mainBB, frame, frameLabel;
    int n;
    Arg args[20];

  /* Create a main window child of the main shell */
    n = 0;
    mainMW = XmCreateMainWindow(mainShell,"mainMW",args,n);

  /* Get default fg/bg colors from mainMW for later use */
    n = 0;
    XtSetArg(args[n],XmNbackground,&defaultBackground); n++;
    XtSetArg(args[n],XmNforeground,&defaultForeground); n++;
    XtGetValues(mainMW,args,n);

  /* Create the menu bar */
    mainMB = XmCreateMenuBar(mainMW,"mainMB",NULL,0);

#if EXPLICITLY_OVERWRITE_CDE_COLORS
  /* Color menu bar explicitly to avoid CDE interference */
    colorMenuBar(mainMB,defaultForeground,defaultBackground);
#endif

  /* Create the file pulldown menu pane */
    mainFilePDM = buildMenu(mainMB,XmMENU_PULLDOWN,
      "File", 'F', fileMenu);

    if(globalDisplayListTraversalMode == DL_EXECUTE) {
	XtSetSensitive(fileMenu[MAIN_FILE_NEW_BTN].widget,False);
	XtSetSensitive(fileMenu[MAIN_FILE_SAVE_BTN].widget,False);
	XtSetSensitive(fileMenu[MAIN_FILE_SAVE_ALL_BTN].widget,False);
	XtSetSensitive(fileMenu[MAIN_FILE_SAVE_AS_BTN].widget,False);
    }

  /* Create the edit pulldown menu pane */
    mainEditPDM = buildMenu(mainMB,XmMENU_PULLDOWN,
      "Edit", 'E', editMenu);

    if(globalDisplayListTraversalMode == DL_EXECUTE)
      XtSetSensitive(mainEditPDM,False);

  /* Create the view pulldown menu pane */
    mainViewPDM = buildMenu(mainMB,XmMENU_PULLDOWN,
      "View", 'V', viewMenu);

  /* Create the palettes pulldown menu pane */
    mainPalettesPDM = buildMenu(mainMB,XmMENU_PULLDOWN,
      "Palettes", 'P', palettesMenu);

    if(globalDisplayListTraversalMode == DL_EXECUTE)
      XtSetSensitive(mainPalettesPDM,False);

  /* Create the help pulldown menu pane */
    mainHelpPDM = buildMenu(mainMB,XmMENU_PULLDOWN,
      "Help", 'H', helpMenu);

    XtVaSetValues(mainMB, XmNmenuHelpWidget, mainHelpPDM, NULL);

#if 0
  /* Don't enable other help functions yet */
    XtSetSensitive(helpMenu[HELP_OVERVIEW_BTN].widget,False);
    XtSetSensitive(helpMenu[HELP_CONTENTS_BTN].widget,False);
    XtSetSensitive(helpMenu[HELP_OBJECTS_BTN].widget,False);
    XtSetSensitive(helpMenu[HELP_EDIT_BTN].widget,False);
    XtSetSensitive(helpMenu[HELP_NEW_BTN].widget,False);
    XtSetSensitive(helpMenu[HELP_TECH_SUPPORT_BTN].widget,False);
    XtSetSensitive(helpMenu[HELP_ON_HELP_BTN].widget,False);
#endif

    n = 0;
    XtSetArg(args[n],XmNmarginHeight,9); n++;
    XtSetArg(args[n],XmNmarginWidth,18); n++;
    mainBB = XmCreateBulletinBoard(mainMW,"mainBB",args,n);
    XtAddCallback(mainBB,XmNhelpCallback,
      globalHelpCallback,(XtPointer)HELP_MAIN);

  /* Create mode frame  */
    n = 0;
    XtSetArg(args[n],XmNshadowType,XmSHADOW_ETCHED_IN); n++;
    frame = XmCreateFrame(mainBB,"frame",args,n);
    label = XmStringCreateLocalized("Mode");
    n = 0;
    XtSetArg(args[n],XmNlabelString,label); n++;
    XtSetArg(args[n],XmNmarginWidth,0); n++;
    XtSetArg(args[n],XmNmarginHeight,0); n++;
    XtSetArg(args[n],XmNchildType,XmFRAME_TITLE_CHILD); n++;
    frameLabel = XmCreateLabel(frame,"frameLabel",args,n);
    XmStringFree(label);
    XtManageChild(frameLabel);
    XtManageChild(frame);

    if(globalDisplayListTraversalMode == DL_EDIT) {
      /* Create the mode radio box and buttons */
	n = 0;
	XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
	XtSetArg(args[n],XmNpacking,XmPACK_COLUMN); n++;
	XtSetArg(args[n],XmNnumColumns,1); n++;
	XtSetArg(args[n],XmNchildType,XmFRAME_WORKAREA_CHILD); n++;
	modeRB = XmCreateRadioBox(frame,"modeRB",args,n);
	label = XmStringCreateLocalized("Edit");

	n = 0;
	XtSetArg(args[n],XmNlabelString,label); n++;
	XtSetArg(args[n],XmNset,TRUE); n++;		/* start with EDIT as set */
	modeEditTB = XmCreateToggleButton(modeRB,"modeEditTB",args,n);
	XtAddCallback(modeEditTB,XmNvalueChangedCallback,
	  modeCallback, (XtPointer)DL_EDIT);
	XmStringFree(label);
	label = XmStringCreateLocalized("Execute");
	n = 0;
	XtSetArg(args[n],XmNlabelString,label); n++;
	modeExecTB = XmCreateToggleButton(modeRB,"modeExecTB",args,n);
	XtAddCallback(modeExecTB,XmNvalueChangedCallback,
	  modeCallback,(XtPointer)DL_EXECUTE);
	XmStringFree(label);
	XtManageChild(modeRB);
	XtManageChild(modeEditTB);
	XtManageChild(modeExecTB);

      /* We want to to save replaced displays if we go back to EDIT mode */
	saveReplacedDisplays = True;
    } else {
      /* If started in execute mode, then no editing allowed, therefore
       *   the modeRB widget is really a frame with a label indicating
       *   execute-only mode */
	label = XmStringCreateLocalized("Execute-Only");
	n = 0;
	XtSetArg(args[n],XmNlabelString,label); n++;
	XtSetArg(args[n],XmNmarginWidth,2); n++;
	XtSetArg(args[n],XmNmarginHeight,1); n++;
	XtSetArg(args[n],XmNchildType,XmFRAME_WORKAREA_CHILD); n++;
	modeRB = XmCreateLabel(frame,"modeRB",args,n);
	XmStringFree(label);
	XtManageChild(modeRB);

      /* There is no reason to save replaced displays */
	saveReplacedDisplays = False;
    }

  /* Manage the composites */
    XtManageChild(mainBB);
    XtManageChild(mainMB);
    XtManageChild(mainMW);

  /************************************************
  ****** Create main-window related dialogs ******
  ************************************************/
#ifdef PROMPT_TO_EXIT
  /* Create the Exit... warning dialog  */

    exitQD = XmCreateQuestionDialog(XtParent(mainFilePDM),"exitQD",NULL,0);
#if OMIT_RESIZE_HANDLES
    XtVaSetValues(XtParent(exitQD),
      XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH,
      NULL);
  /* KE: The following is necessary for Exceed, which turns off the
     resize function with the handles.  It should not be necessary */
    XtVaSetValues(XtParent(exitQD),
      XmNmwmFunctions,MWM_FUNC_ALL,
      NULL);
#endif
    XtUnmanageChild(XmMessageBoxGetChild(exitQD,XmDIALOG_HELP_BUTTON));
    XtAddCallback(exitQD,XmNcancelCallback,
      fileMenuDialogCallback,(XtPointer)MAIN_FILE_EXIT_BTN);
    XtAddCallback(exitQD,XmNokCallback,fileMenuDialogCallback,
      (XtPointer)MAIN_FILE_EXIT_BTN);
#endif

  /* Create the Help information shell (Help on Help) */
    n = 0;
    XtSetArg(args[n],XtNiconName,"Help"); n++;
    XtSetArg(args[n],XtNtitle,"Medm Help System"); n++;
    XtSetArg(args[n],XtNallowShellResize,TRUE); n++;
    XtSetArg(args[n],XmNkeyboardFocusPolicy,XmEXPLICIT); n++;
  /* Map window manager menu Close function to application close... */
    XtSetArg(args[n],XmNdeleteResponse,XmDO_NOTHING); n++;
#if OMIT_RESIZE_HANDLES
  /* Remove resize handles */
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
  /* KE: The following is necessary for Exceed, which turns off the
     resize function with the handles.  It should not be necessary */
    XtSetArg(args[n],XmNmwmFunctions, MWM_FUNC_ALL); n++;
#endif
    helpS = XtCreatePopupShell("helpS",topLevelShellWidgetClass,
      mainShell,args,n);
    XmAddWMProtocolCallback(helpS,WM_DELETE_WINDOW,
      wmCloseCallback,(XtPointer)OTHER_SHELL);
    n = 0;
    XtSetArg(args[n],XmNdialogType,XmDIALOG_INFORMATION); n++;
    helpMessageBox = XmCreateMessageBox(helpS,"helpMessageBox",
      args,n);
    XtUnmanageChild(XmMessageBoxGetChild(helpMessageBox,XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(helpMessageBox,XmDIALOG_HELP_BUTTON));
    XtAddCallback(helpMessageBox,XmNokCallback,
      helpDialogCallback,(XtPointer)NULL);

    XtManageChild(helpMessageBox);

  /* Create the EditHelp information shell (Edit Summary in Edit-Mode menu) */
    n = 0;
    XtSetArg(args[n],XtNiconName,"EditHelp"); n++;
    XtSetArg(args[n],XtNtitle,"Edit Help"); n++;
    XtSetArg(args[n],XtNallowShellResize,TRUE); n++;
    XtSetArg(args[n],XmNkeyboardFocusPolicy,XmEXPLICIT); n++;
  /* Map window manager menu Close function to application close... */
    XtSetArg(args[n],XmNdeleteResponse,XmDO_NOTHING); n++;
#if OMIT_RESIZE_HANDLES
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
  /* KE: The following is necessary for Exceed, which turns off the
     resize function with the handles.  It should not be necessary */
    XtSetArg(args[n],XmNmwmFunctions, MWM_FUNC_ALL); n++;
#endif
    editHelpS = XtCreatePopupShell("editHelpS",topLevelShellWidgetClass,
      mainShell,args,n);
    XmAddWMProtocolCallback(editHelpS,WM_DELETE_WINDOW,
      wmCloseCallback,(XtPointer)OTHER_SHELL);
    n = 0;
    XtSetArg(args[n],XmNdialogType,XmDIALOG_INFORMATION); n++;
    editHelpMessageBox = XmCreateMessageBox(editHelpS,"editHelpMessageBox",
      args,n);
    XtUnmanageChild(XmMessageBoxGetChild(editHelpMessageBox,XmDIALOG_CANCEL_BUTTON));
    XtUnmanageChild(XmMessageBoxGetChild(editHelpMessageBox,XmDIALOG_HELP_BUTTON));
    XtAddCallback(editHelpMessageBox,XmNokCallback,
      editHelpDialogCallback,(XtPointer)NULL);

    XtManageChild(editHelpMessageBox);

  /* Initialize other shells */
    caStudyS = (Widget)0;
    displayListS = (Widget)0;
    errMsgS = (Widget)0;
    errMsgSendS = (Widget)0;
    printSetupS = (Widget)0;
    pvInfoS = (Widget)0;
    pvLimitsS = (Widget)0;

  /* Realize the toplevel shell widget */
    XtRealizeWidget(mainShell);
}

void enableEditFunctions() {
    if(objectS)   XtSetSensitive(objectS,True);
    if(resourceS) XtSetSensitive(resourceS,True);
    if(colorS)    XtSetSensitive(colorS,True);
    if(channelS)  XtSetSensitive(channelS,True);
    XtSetSensitive(mainEditPDM,True);
    XtSetSensitive(mainPalettesPDM,True);
}

void disableEditFunctions() {
    if(objectS)   XtSetSensitive(objectS,False);
    if(resourceS) XtSetSensitive(resourceS,False);
    if(colorS)    XtSetSensitive(colorS,False);
    if(channelS)  XtSetSensitive(channelS,False);
    XtSetSensitive(mainEditPDM,False);
    XtSetSensitive(mainPalettesPDM,False);
}
