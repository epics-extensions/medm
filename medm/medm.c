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

#define DEBUG_RADIO_BUTTONS 0
#define DEBUG_DEFINITIONS 0

#define ALLOCATE_STORAGE
#include "medm.h"
#include <Xm/RepType.h>

#ifdef EDITRES
#include <X11/Xmu/Editres.h>
#endif

#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

#include <errno.h>
#include "icon25"

/* For X property cleanup */
#include <signal.h>
#include <Xm/MwmUtil.h>
#include <X11/IntrinsicP.h>

#include <XrtGraph.h>

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
#define FILE_PRINT_SETUP_BTN 5
#define FILE_PRINT_BTN 6
#define FILE_EXIT_BTN 7

#define PRINTER_SETUP_OK     0
#define PRINTER_SETUP_CANCEL 1
#define PRINTER_SETUP_MAP    2

#define GRID_OK     0
#define GRID_CANCEL 1
#define GRID_HELP   2

#define EDIT_BTN_POSN 1
#define EDIT_OBJECT_BTN           0
#define EDIT_CUT_BTN              1
#define EDIT_COPY_BTN             2
#define EDIT_PASTE_BTN            3
#define EDIT_RAISE_BTN            4
#define EDIT_LOWER_BTN            5

#define EDIT_GROUP_BTN            6
#define EDIT_UNGROUP_BTN          7

#define EDIT_ALIGN_BTN            8
#define EDIT_UNSELECT_BTN         9
#define EDIT_SELECT_ALL_BTN      10
#define EDIT_REFRESH_BTN         11
#define EDIT_SAME_SIZE_BTN       12
#define EDIT_GRID_SPACING_BTN    13
#define EDIT_GRID_TOGGLE_BTN     14
#define EDIT_SPACE_BTN           15
#define EDIT_HELP_BTN            16
#define EDIT_UNDO_BTN            17

#define N_VIEW_MENU_ELES         3
#define VIEW_BTN_POSN            2
#define VIEW_MESSAGE_WINDOW_BTN  1
#define VIEW_STATUS_WINDOW_BTN   2

#define N_ALIGN_MENU_ELES  2
#define ALIGN_BTN_POSN    13

#define N_HORIZ_ALIGN_MENU_ELES 3
#define HORIZ_ALIGN_BTN_POSN    0

#define ALIGN_HORIZ_LEFT_BTN   0
#define ALIGN_HORIZ_CENTER_BTN 1
#define ALIGN_HORIZ_RIGHT_BTN  2
#define ALIGN_TO_GRID_BTN      3

#define N_VERT_ALIGN_MENU_ELES 3
#define VERT_ALIGN_BTN_POSN    1

#define ALIGN_VERT_TOP_BTN    0
#define ALIGN_VERT_CENTER_BTN 1
#define ALIGN_VERT_BOTTOM_BTN 2

#define SPACE_HORIZ_BTN 0
#define SPACE_VERT_BTN  1
#define SPACE_2D_BTN  2

#ifdef EXTENDED_INTERFACE
#define N_PALETTES_MENU_ELES 4
#else
#define N_PALETTES_MENU_ELES 3
#endif
#define PALETTES_BTN_POSN    3

#define PALETTES_OBJECT_BTN   0
#define PALETTES_RESOURCE_BTN 1
#define PALETTES_COLOR_BTN    2
#ifdef EXTENDED_INTERFACE
#define PALETTES_CHANNEL_BTN  3
#endif

#define N_HELP_MENU_ELES 7
#define HELP_BTN_POSN    4

#define HELP_OVERVIEW_BTN   0
#define HELP_CONTENTS_BTN   1
#define HELP_OBJECTS_BTN    2
#define HELP_EDIT_BTN       3
#define HELP_NEW_BTN        4
#define HELP_ON_HELP_BTN    5
#define HELP_ON_VERSION_BTN 6

/* Function prototypes */

extern int putenv(const char *);     /* May not be defined for strict ANSI */
static void createCursors(void);
static void createMain(void);
static Boolean medmInitWorkProc(XtPointer cd);
static void fileMenuSimpleCallback(Widget,XtPointer,XtPointer);
static void fileMenuDialogCallback(Widget,XtPointer,XtPointer);
static void editMenuSimpleCallback(Widget,XtPointer,XtPointer);
static void palettesMenuSimpleCallback(Widget,XtPointer,XtPointer);
static void helpMenuSimpleCallback(Widget,XtPointer,XtPointer);
static void alignHorizontalMenuSimpleCallback(Widget,XtPointer,XtPointer);
static void alignVerticalMenuSimpleCallback(Widget,XtPointer,XtPointer);
static void spaceMenuSimpleCallback(Widget, XtPointer, XtPointer);
static void viewMenuSimpleCallback(Widget,XtPointer,XtPointer);

Widget mainFilePDM, mainHelpPDM;
static Widget printerSetupDlg = 0;
static Widget gridDlg = 0;
static int medmUseBigCursor = 0;


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
    NULL,
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
    NULL,
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
    NULL,
};

static menuEntry_t editAlignHorzMenu[] = {
    { "Left",   &xmPushButtonGadgetClass, 'L', NULL, NULL, NULL,
      alignHorizontalMenuSimpleCallback, (XtPointer) ALIGN_HORIZ_LEFT_BTN,  NULL},
    { "Center", &xmPushButtonGadgetClass, 'C', NULL, NULL, NULL,
      alignHorizontalMenuSimpleCallback, (XtPointer) ALIGN_HORIZ_CENTER_BTN,  NULL},
    { "Right",  &xmPushButtonGadgetClass, 'R', NULL, NULL, NULL,
      alignHorizontalMenuSimpleCallback, (XtPointer) ALIGN_HORIZ_RIGHT_BTN,  NULL},
    NULL,
};
  
static menuEntry_t editAlignVertMenu[] = {
    { "Top",    &xmPushButtonGadgetClass, 'T', NULL, NULL, NULL, 
      alignVerticalMenuSimpleCallback, (XtPointer) ALIGN_VERT_TOP_BTN,  NULL},
    { "Center", &xmPushButtonGadgetClass, 'C', NULL, NULL, NULL,
      alignVerticalMenuSimpleCallback, (XtPointer) ALIGN_VERT_CENTER_BTN,  NULL},
    { "Bottom", &xmPushButtonGadgetClass, 'B', NULL, NULL, NULL,
      alignVerticalMenuSimpleCallback, (XtPointer) ALIGN_VERT_BOTTOM_BTN,  NULL},
    NULL,
};
  

static menuEntry_t editAlignMenu[] = {
    { "Horizontal", &xmCascadeButtonGadgetClass, 'H', NULL, NULL, NULL,
      NULL, NULL, editAlignHorzMenu},
    { "Vertical",   &xmCascadeButtonGadgetClass, 'V', NULL, NULL, NULL,
      NULL, NULL, editAlignVertMenu},
    { "To Grid",    &xmPushButtonGadgetClass, 'G', NULL, NULL, NULL,
      alignHorizontalMenuSimpleCallback, (XtPointer) ALIGN_TO_GRID_BTN,  NULL},
    NULL,
};
  
static menuEntry_t editSpaceMenu[] = {
    { "Horizontal", &xmPushButtonGadgetClass, 'H', NULL, NULL, NULL,
      spaceMenuSimpleCallback, (XtPointer) SPACE_HORIZ_BTN,  NULL},
    { "Vertical",   &xmPushButtonGadgetClass, 'V', NULL, NULL, NULL,
      spaceMenuSimpleCallback, (XtPointer) SPACE_VERT_BTN,  NULL},
    { "2-D",       &xmPushButtonGadgetClass, 'B', NULL, NULL, NULL,
      spaceMenuSimpleCallback, (XtPointer) SPACE_2D_BTN,  NULL},
    NULL,
};
  
static menuEntry_t editObjectMenu[] = {
    { "Graphics",   &xmCascadeButtonGadgetClass,'G', NULL, NULL, NULL,
      NULL,        NULL,                     graphicsObjectMenu},
    { "Monitors",   &xmCascadeButtonGadgetClass,'M', NULL, NULL, NULL,
      NULL,        NULL,                     monitorsObjectMenu},
    { "Controllers",&xmCascadeButtonGadgetClass,'C', NULL, NULL, NULL,
      NULL,        NULL,                     controllersObjectMenu},
    NULL,
};

static menuEntry_t editMenu[] = {
    { "Undo",      &xmPushButtonGadgetClass, 'U', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_UNDO_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0',
      NULL,        NULL,                     NULL},
    { "Cut",       &xmPushButtonGadgetClass, 't', "Shift<Key>DeleteChar", "Shift+Del", NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_CUT_BTN,  NULL},
    { "Copy",      &xmPushButtonGadgetClass, 'C', "Ctrl<Key>InsertChar",  "Ctrl+Ins",   NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_COPY_BTN,  NULL},
    { "Paste" ,    &xmPushButtonGadgetClass, 'P', "Shift<Key>InsertChar", "Shift+Ins",  NULL,
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
    { "Ungroup",   &xmPushButtonGadgetClass, 'n', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_UNGROUP_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0',NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Align",     &xmCascadeButtonGadgetClass, 'A', NULL, NULL, NULL,
      NULL,        NULL,                     editAlignMenu},
    { "Set Spacing", &xmCascadeButtonGadgetClass, 'i', NULL, NULL, NULL,
      NULL,        NULL,                     editSpaceMenu},
    { "Toggle Grid",       &xmPushButtonGadgetClass, 'o', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_GRID_TOGGLE_BTN,  NULL},
    { "Grid...",   &xmPushButtonGadgetClass, 'd', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_GRID_SPACING_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Same Size", &xmPushButtonGadgetClass, 'm', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_SAME_SIZE_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Unselect",  &xmPushButtonGadgetClass, 'e', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_UNSELECT_BTN,  NULL},
    { "Select All",&xmPushButtonGadgetClass, 'S', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_SELECT_ALL_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Refresh",   &xmPushButtonGadgetClass, 'f', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_REFRESH_BTN,  NULL},
    { "Edit Summary...", &xmPushButtonGadgetClass, 'y', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_HELP_BTN,  NULL},
    NULL,
};

static menuEntry_t displayMenu[] = {
    { "Object",    &xmCascadeButtonGadgetClass, 'j', NULL, NULL, NULL,
      NULL,        NULL,                     editObjectMenu},
    { "Undo",      &xmPushButtonGadgetClass, 'U', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_UNDO_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0',
      NULL,        NULL,                     NULL},
    { "Cut",       &xmPushButtonGadgetClass, 't', "Shift<Key>DeleteChar", "Shift+Del", NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_CUT_BTN,  NULL},
    { "Copy",      &xmPushButtonGadgetClass, 'C', "Ctrl<Key>InsertChar",  "Ctrl+Ins",   NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_COPY_BTN,  NULL},
    { "Paste" ,    &xmPushButtonGadgetClass, 'P', "Shift<Key>InsertChar", "Shift+Ins",  NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_PASTE_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Raise",     &xmPushButtonGadgetClass, 'R', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_RAISE_BTN,  NULL},
    { "Lower",     &xmPushButtonGadgetClass, 'L', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_LOWER_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Group",     &xmPushButtonGadgetClass, 'G', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_GROUP_BTN,  NULL},
    { "Ungroup",   &xmPushButtonGadgetClass, 'n', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_UNGROUP_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Align",     &xmCascadeButtonGadgetClass, 'A', NULL, NULL, NULL,
      NULL,        NULL,                     editAlignMenu},
    { "Set Spacing", &xmCascadeButtonGadgetClass, 'i', NULL, NULL, NULL,
      NULL,        NULL,                     editSpaceMenu},
    { "Toggle Grid",       &xmPushButtonGadgetClass, 'o', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_GRID_TOGGLE_BTN,  NULL},
    { "Grid...",   &xmPushButtonGadgetClass, 'd', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_GRID_SPACING_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Same Size", &xmPushButtonGadgetClass, 'm', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_SAME_SIZE_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Unselect",  &xmPushButtonGadgetClass, 'l', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_UNSELECT_BTN,  NULL},
    { "Select All",&xmPushButtonGadgetClass, 'S', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_SELECT_ALL_BTN,  NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL, NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Refresh",   &xmPushButtonGadgetClass, 'f', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_REFRESH_BTN,  NULL},
    { "Edit Summary...", &xmPushButtonGadgetClass, 'y', NULL, NULL, NULL,
      editMenuSimpleCallback, (XtPointer) EDIT_HELP_BTN,  NULL},
    NULL,
};

static menuEntry_t fileMenu[] = {
    { "New",       &xmPushButtonGadgetClass, 'N', NULL,         NULL, NULL,
      fileMenuSimpleCallback, (XtPointer) FILE_NEW_BTN, NULL},
    { "Open...",   &xmPushButtonGadgetClass, 'O', "Ctrl<Key>O", "Ctrl+O", NULL,
      fileMenuSimpleCallback, (XtPointer) FILE_OPEN_BTN, NULL},
    { "Save",      &xmPushButtonGadgetClass, 'S', "Ctrl<Key>S", "Ctrl+S", NULL,
      fileMenuSimpleCallback, (XtPointer) FILE_SAVE_BTN, NULL},
    { "Save As...",&xmPushButtonGadgetClass, 'A', NULL,         NULL, NULL,
      fileMenuSimpleCallback, (XtPointer) FILE_SAVE_AS_BTN, NULL},
    { "Close",     &xmPushButtonGadgetClass, 'C', NULL,         NULL, NULL,
      fileMenuSimpleCallback, (XtPointer) FILE_CLOSE_BTN, NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL,         NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Printer Setup...",  &xmPushButtonGadgetClass, 'u', NULL,      NULL, NULL,
      fileMenuSimpleCallback, (XtPointer) FILE_PRINT_SETUP_BTN, NULL},
    { "Print",  &xmPushButtonGadgetClass, 'P', NULL,         NULL, NULL,
      fileMenuSimpleCallback, (XtPointer) FILE_PRINT_BTN, NULL},
    { "Separator", &xmSeparatorGadgetClass,  '\0', NULL,         NULL, NULL,
      NULL,        NULL,                     NULL},
    { "Exit",      &xmPushButtonGadgetClass, 'x', "Ctrl<Key>x", "Ctrl+x", NULL,
      fileMenuSimpleCallback, (XtPointer) FILE_EXIT_BTN, NULL},
    NULL,
};

static menuEntry_t viewMenu[] = {
    { "Message Window", &xmPushButtonGadgetClass, 'M', NULL, NULL, NULL,
      viewMenuSimpleCallback, (XtPointer) VIEW_MESSAGE_WINDOW_BTN, NULL},
    { "Status Window", &xmPushButtonGadgetClass, 'S', NULL, NULL, NULL,
      viewMenuSimpleCallback, (XtPointer) VIEW_STATUS_WINDOW_BTN, NULL},
    NULL,
};
/*
  { "Grid",           &xmPushButtonGadgetClass, 'G', NULL, NULL, NULL, NULL, NULL, NULL},
  { "Separator",      &xmSeparatorGadgetClass,  NULL, NULL, NULL, NULL, NULL, NULL, NULL},
  { "Refresh Screen", &xmPushButtonGadgetClass, 'R', NULL, NULL, NULL, NULL, NULL, NULL},
*/

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
      NULL,
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
{ "On Help",  &xmPushButtonGadgetClass, 'H', NULL, NULL, NULL,
    helpMenuSimpleCallback, (XtPointer) HELP_ON_HELP_BTN, NULL},
{ "On Version",  &xmPushButtonGadgetClass, 'V', NULL, NULL, NULL,
    helpMenuSimpleCallback, (XtPointer) HELP_ON_VERSION_BTN, NULL},
  NULL,
  };

#ifdef __COLOR_RULE_H__
/* color rule set */
setOfColorRule_t setOfColorRule[MAX_SET_OF_COLOR_RULE] = {
    {
	{
	    { 0.00, 0.00, 20},
	    { 1.00, 1.00, 15},
	    { 2.00, 2.00, 22},
	    { 3.00, 3.00, 17},
	    { 4.00, 4.00, 35},
	    { 5.00, 5.00, 55},
	    { 6.00, 6.00, 40},
	    { 7.00, 7.00, 40},
	    { 8.00, 8.00, 30},
	    { 9.00, 9.00, 32},
	    {10.00,10.00, 30},
	    {11.00,11.00, 32},
	    {12.00,12.00, 30},
	    {13.00,13.00, 32},
	    {14.00,14.00, 30},
	    {15.00,15.00, 32}
	}
    },
    {
	{
	    { 0.00, 0.00, 20},
	    { 1.00, 1.00, 24},
	    { 2.00, 2.00, 15},
	    { 3.00, 3.00, 19},
	    { 4.00, 4.00, 55},
	    { 5.00, 5.00, 30},
	    { 6.00, 6.00, 50},
	    { 7.00, 7.00, 54},
	    { 8.00, 8.00, 45},
	    { 9.00, 9.00, 47},
	    {10.00,10.00, 40},
	    {11.00,11.00, 42},
	    {12.00,12.00, 32},
	    {13.00,13.00, 33},
	    {14.00,14.00,  4},
	    {15.00,15.00, 14}
	}
    },
    {
	{
	    {-0.01, 0.01,32},
	    { 0.99, 1.01,33},
	    { 1.99, 2.01,34},
	    { 2.99, 3.01,35},
	    { 3.99, 4.01,36},
	    { 4.99, 5.01,37},
	    { 5.99, 6.01,38},
	    { 6.99, 7.01,39},
	    { 7.99, 8.01,40},
	    { 8.99, 9.01,41},
	    { 9.99,10.01,42},
	    {10.99,11.01,43},
	    {11.99,12.01,44},
	    {12.99,13.01,45},
	    {13.99,14.01,46},
	    {14.99,15.01,47}
	}
    },
    {
	{
	    {-0.01, 0.01,48},
	    { 0.99, 1.01,49},
	    { 1.99, 2.01,50},
	    { 2.99, 3.01,51},
	    { 3.99, 4.01,52},
	    { 4.99, 5.01,53},
	    { 5.99, 6.01,54},
	    { 6.99, 7.01,55},
	    { 7.99, 8.01,56},
	    { 8.99, 9.01,57},
	    { 9.99,10.01,58},
	    {10.99,11.01,59},
	    {11.99,12.01,60},
	    {12.99,13.01,61},
	    {13.99,14.01,62},
	    {14.99,15.01,63}
	}
    }
};
#endif
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
  /***
  *** main window
  ***/
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
  /***
  *** objectPalette
  ***/
    "Medm.objectS.geometry: -5+137",
    "Medm*objectMW.objectMB*fontList: 8x13",
    "Medm*objectMW*XmLabel.marginWidth: 0",
  /***
  *** bubbleHelp
  ***/
    "Medm*bubbleHelpD*background: #f9da3c",
    "Medm*bubbleHelpD*borderColor: black",
    "Medm*bubbleHelpD*borderWidth: 1",
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
    "Medm.colorS.geometry: -5-5",
    "Medm*colorMW.colorMB*fontList: 8x13",
    "Medm*colorMW*XmLabel.marginWidth: 0",
    "Medm*colorMW*colorPB.width: 20",
    "Medm*colorMW*colorPB.height: 20",
  /***
  *** resourcePalette
  ***/
    "Medm.resourceS.geometry: -5+304",
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
    "Medm*resourceMW*importFSD.dialogTitle: Import...",
    "Medm*resourceMW*importFSD.form.shadowThickness: 0",
    "Medm*resourceMW*importFSD.form.typeLabel.labelString: Image Type:",
    "Medm*resourceMW*importFSD.form.typeLabel.marginTop: 4",
    "Medm*resourceMW*importFSD.form.frame.radioBox.orientation: XmHORIZONTAL",
    "Medm*resourceMW*importFSD.form.frame.radioBox.numColumns: 1",
    "Medm*resourceMW*importFSD.form.frame.radioBox*shadowThickness: 0",
    "Medm*resourceMW*importFSD*XmToggleButton.indicatorOn: True",
    "Medm*resourceMW*importFSD*XmToggleButton.labelType: XmString",
#ifdef EXTENDED_INTERFACE
  /***
  *** channelPalette
  ***/
    "Medm*channelMW.width: 140",
    "Medm*channelMW.channelMB*fontList: 8x13",
    "Medm*channelMW*XmLabel.marginWidth: 0",
#endif
  /***
  *** medmWidget resource specifications
  ***/
    "Medm*warningDialog.foreground: white",
    "Medm*warningDialog.background: red",
    "Medm*warningDialog.dialogTitle: Warning",
    "Medm*Indicator.AxisWidth: 3",
    "Medm*Bar.AxisWidth: 3",
    "Medm*Indicator.ShadowThickness: 2",
    "Medm*Bar.ShadowThickness: 2",
    "Medm*Meter.ShadowThickness: 2",

#if XRT_VERSION > 2
  /* XRTGraph Property Editor */
    "Medm*.PropEdit_shell*.background:                  White",
    "Medm*.PropEdit_shell*.foreground:                  Black",
    "Medm*.PropEdit_shell.width:                        630",
    "Medm*.PropEdit_shell.height:                       390",
    "Medm*.PropEdit_shell*.XmXrtOutliner*.background:   White",
    "Medm*.PropEdit_shell*.XmXrtOutliner*.foreground:   Black",
    "Medm*.PropEdit_shell*.XmXrtIntField.background:    White",
    "Medm*.PropEdit_shell*.XmXrtIntField.foreground:    Black",
    "Medm*.PropEdit_shell*.XmXrtStringField.background: White",
    "Medm*.PropEdit_shell*.XmXrtStringField.foreground: Black",
    "Medm*.PropEdit_shell*.XmXrtFloatField.background:  White",
    "Medm*.PropEdit_shell*.XmXrtFloatField.foreground:  Black",
    "Medm*.PropEdit_shell*.XmXrtDateField.background:   White",
    "Medm*.PropEdit_shell*.XmXrtDateField.foreground:   Black",
#endif
    
    NULL,
};

typedef enum {EDIT,EXECUTE,HELP,VERSION} opMode_t;
typedef enum {NORMAL,CLEANUP,LOCAL} medmMode_t;
typedef enum {FIXED,SCALABLE} fontStyle_t;

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

void requestDestroy(request_t *request) {
    if (request) {
	if (request->macroString) free(request->macroString);
/*      if (request->displayFont) free(request->displayFont);     */
	if (request->displayName) free(request->displayName);
	if (request->displayGeometry) free(request->displayGeometry);
	if (request->fileList) {
	    int i;
	    for (i=0; i < request->fileCnt; i++) {
		if (request->fileList[i]) free(request->fileList[i]);
	    }
	    free((char *)request->fileList);
	    request->fileList = NULL;
	}
	free((char *)request);
	request = NULL;
    }
}

request_t * parseCommandLine(int argc, char *argv[]) {
    int i;
    int argsUsed = 0;
    int fileEntryTableSize = 0;
    request_t *request = NULL;
#define FULLPATHNAME_SIZE 1024
    char currentDirectoryName[FULLPATHNAME_SIZE+1];
    char fullPathName[FULLPATHNAME_SIZE+1];

    request = (request_t *) malloc(sizeof(request_t));
    if (request == NULL) return request;
    request->opMode = EDIT;
    request->medmMode = NORMAL;
    request->fontStyle = FIXED;
    request->privateCmap = False;
    request->macroString = NULL;
    strcpy(request->displayFont,FONT_ALIASES_STRING);
    request->displayName = NULL;
    request->displayGeometry = NULL;
    request->fileCnt = 0;
    request->fileList = NULL;

  /* Parse the switches */
    for (i = 1; i < argc; i++) {
	if (!strcmp(argv[i],"-x")) {
	    request->opMode = EXECUTE;
	    argsUsed = i;
	} else if (!strcmp(argv[i],"-help") || !strcmp(argv[i],"-?")) {
	    request->opMode = HELP;
	    argsUsed = i;
	} else if (!strcmp(argv[i],"-version")) {
	    request->opMode = VERSION;
	    argsUsed = i;
	} else if (!strcmp(argv[i],"-local")) {
	    request->medmMode = LOCAL;
	    argsUsed = i;
	} else if (!strcmp(argv[i],"-cleanup")) {
	    request->medmMode = CLEANUP;
	    argsUsed = i;
	} else if (!strcmp(argv[i],"-cmap")) {
	    request->privateCmap = True;
	    argsUsed = i;
	} else if (!strcmp(argv[i],"-macro")) {
	    char *tmp;
	    argsUsed = i;
	    tmp = (((i+1) < argc) ? argv[i+1] : NULL);
	    if (tmp) {
		argsUsed = i + 1;
		request->macroString = STRDUP(tmp);
	      /* since parameter of form   -macro "a=b,c=d,..."  replace '"' with ' ' */
		if (request->macroString != NULL) {
		    int len;
		    if (request->macroString[0] == '"') request->macroString[0] = ' ';
		    len = strlen(request->macroString) - 1;
		    if (request->macroString[len] == '"') request->macroString[len] = ' ';
		}
	    }
	} else if (!strcmp(argv[i],"-displayFont")) {
	    char *tmp;
	    argsUsed = i;
	    tmp = (((i+1) < argc) ? argv[i+1] : NULL);
	    if (tmp) {
		argsUsed = i + 1;
		strcpy(request->displayFont,tmp);
		if (request->displayFont[0] == '\0') {
		    if (!strcmp(request->displayFont,FONT_ALIASES_STRING))
		      request->fontStyle = FIXED;
		    else if (!strcmp(request->displayFont,DEFAULT_SCALABLE_STRING))
		      request->fontStyle = SCALABLE;
		}
	    }
	} else if (!strcmp(argv[i],"-display")) {
	  /* (Not trapped by X because this routine is called first) */
	    char *tmp;
	    argsUsed = i;
	    tmp = (((i+1) < argc) ? argv[i+1] : NULL);
	    if (tmp) {
		argsUsed = i + 1;
		request->displayName = STRDUP(tmp);
	    }
	} else if ((!strcmp(argv[i],"-displayGeometry")) || (!strcmp(argv[i],"-dg"))) {
	    char *tmp;
	    argsUsed = i;
	    tmp = (((i+1) < argc) ? argv[i+1] : NULL);
	    if (tmp) {
		argsUsed = i + 1;
		request->displayGeometry = STRDUP(tmp);
	    }
	} else if (!strcmp(argv[i],"-bigMousePointer")) {
	    medmUseBigCursor = 1;

	    argsUsed = i;
	}
    }
    
  /* Get the current directory */
    currentDirectoryName[0] = '\0';
    getcwd(currentDirectoryName,FULLPATHNAME_SIZE);
    
  /* Make sure fullPathName is a terminated with '\0' string */
  /* KE: Not necessary, is set to a null string below */
    fullPathName[FULLPATHNAME_SIZE] = '\0';

  /* Parse the display name */
    for (i = argsUsed+1; i < argc; i++) {
	Boolean canAccess;
	char    *fileStr;

	canAccess = False;

      /* Check the next argument, if doesn't match the suffix, continue */
      /* KE: May not be a suffix (junk.adlebrained.txt fits) */
	fileStr = argv[i];
	if (strstr(fileStr,DISPLAY_FILE_ASCII_SUFFIX) == NULL) {
	    medmPrintf("\nFile has wrong suffix: %s\n",fileStr);
	    continue;
	}
	if (strlen(fileStr) > (size_t) FULLPATHNAME_SIZE) {
	    medmPrintf("\nFile name too long: %s\n",fileStr);
	    continue;
	}

      /* Mark the fullPathName as an empty string */
	fullPathName[0] = '\0';

      /* Found string with right suffix - presume it's a valid display name */
	if (canAccess = !access(fileStr,R_OK|F_OK)) { /* Found the file */
	    if (fileStr[0] == '/') {
	      /* Is a full path name */
		strncpy(fullPathName,fileStr,FULLPATHNAME_SIZE);
	    } else {
	      /* Insert the path before the file name */
		if (strlen(currentDirectoryName)+strlen(fileStr)+1 < (size_t) FULLPATHNAME_SIZE) {
		    strcpy(fullPathName,currentDirectoryName);
		    strcat(fullPathName,"/");
		    strcat(fullPathName,fileStr);
		} else {
		    canAccess = False;
		}
	    }
	} else {
	  /* Not found, try with directory specified in the environment */
	    char *dir = NULL;
	    char name[FULLPATHNAME_SIZE];
	    int startPos;
	    dir = getenv("EPICS_DISPLAY_PATH");
	    if (dir != NULL) {
		startPos = 0;
		while (extractStringBetweenColons(dir,name,startPos,&startPos)) {
		    if (strlen(name)+strlen(fileStr)+1 < (size_t) FULLPATHNAME_SIZE) {
			strcpy(fullPathName,name);
			strcat(fullPathName,"/");
			strcat(fullPathName,fileStr);
			if (canAccess = !access(fullPathName,R_OK|F_OK)) break;
		    }
		}
	    }
	}
	if (canAccess) {
	  /* build the request */
	    if (fileEntryTableSize == 0) {
		fileEntryTableSize =  10;
		request->fileList = (char **) malloc(fileEntryTableSize*sizeof(char *));
	    }
	    if (fileEntryTableSize > request->fileCnt) {
		fileEntryTableSize *= 2;
#if defined(__cplusplus) && !defined(__GNUG__)
		request->fileList = (char **) realloc((malloc_t)request->fileList,fileEntryTableSize);
#else
		request->fileList = (char **) realloc(request->fileList,fileEntryTableSize);
#endif
	    }
	    if (request->fileList) {
		request->fileList[request->fileCnt] = STRDUP(fullPathName);
		request->fileCnt++;
	    }
	} else {
	    medmPrintf("\nCannot access file: %s\n",fileStr);
	}
    }
    return request;
}

/********************************************
 **************** Callbacks *****************
 ********************************************/
#ifdef __cplusplus
static void printerSetupDlgCb(Widget w, XtPointer cd, XtPointer)
#else
static void printerSetupDlgCb(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    Position X, Y;
    XmString xmString;
    char *printerName;
    char *variable;
    char *prefix = "PSPRINTER=";

    switch ((int)cd) {
    case PRINTER_SETUP_OK :
	XtVaGetValues(w,XmNtextString,&xmString,NULL);
	XmStringGetLtoR(xmString,XmFONTLIST_DEFAULT_TAG,&printerName);
	variable = (char*) malloc(
	  sizeof(char)*(strlen(printerName) + strlen(prefix) + 1));
	if (variable) {
	    strcpy(variable,prefix);
	    strcat(variable,printerName);
	    putenv(variable);
	  /* Warning!!!! : Do not free the variable */
	}
	free(printerName);
	XmStringFree(xmString);
	XtUnmanageChild(w);
	break;
    case PRINTER_SETUP_CANCEL :
	XtUnmanageChild(w);
	break;
    case PRINTER_SETUP_MAP :
	if (getenv("PSPRINTER")) {
	    xmString = XmStringCreateLocalized(getenv("PSPRINTER"));
	} else {
	    xmString = XmStringCreateLocalized("");
	}
	XtVaSetValues(w,XmNtextString,xmString,NULL);
	XmStringFree(xmString);
	XtTranslateCoords(mainShell,0,0,&X,&Y);
      /* try to force correct popup the first time */
	XtMoveWidget(XtParent(w),X,Y);

	break;
    }
}

#ifdef __cplusplus
static void gridDlgCb(Widget w, XtPointer cd, XtPointer)
#else
static void gridDlgCb(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    DisplayInfo *cdi=currentDisplayInfo;
    Position X, Y;
    XmString xmString;

    switch ((int)cd) {
    case GRID_OK:
	if(cdi) {
	    char *gridVal;
	    XmString xmString;
	    
	    XtVaGetValues(w,XmNtextString,&xmString,NULL);
	    XmStringGetLtoR(xmString,XmFONTLIST_DEFAULT_TAG,&gridVal);
	    cdi->gridSpacing = atoi(gridVal);
	    if(cdi->gridSpacing < 2) cdi->gridSpacing = 2;
	    free(gridVal);
	    XmStringFree(xmString);
	    XtUnmanageChild(w);
	    dmTraverseNonWidgetsInDisplayList(cdi);
	}
	break;
    case GRID_CANCEL:
	XtUnmanageChild(w);
	break;
    case GRID_HELP:
	callBrowser(MEDM_HELP_PATH"/MEDM.html#Grid");
	break;
    }
}

#ifdef __cplusplus
static void viewMenuSimpleCallback(Widget, XtPointer cd, XtPointer)
#else
static void viewMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int buttonNumber = (int) cd;

    switch(buttonNumber) {
    case VIEW_MESSAGE_WINDOW_BTN:
	errMsgDlgCreateDlg();
	break;
    case VIEW_STATUS_WINDOW_BTN:
	medmCreateCAStudyDlg();
	break;
    default :
	break;
    }
}

#ifdef __cplusplus
static void editMenuSimpleCallback(Widget, XtPointer cd, XtPointer)
#else
static void editMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    DisplayInfo *cdi=currentDisplayInfo;
    int buttonNumber = (int) cd;

  /* simply return if no current display */
    if (cdi == NULL) return;

  /* (MDA) could be smarter about this too, and not do whole traversals...*/

    switch(buttonNumber) {
    case EDIT_OBJECT_BTN:
	break;

    case EDIT_UNDO_BTN:
	restoreUndoInfo(cdi);
	break;

    case EDIT_CUT_BTN:
	copySelectedElementsIntoClipboard();
	deleteElementsInDisplay();
	if (cdi->hasBeenEditedButNotSaved == False)
	  medmMarkDisplayBeingEdited(cdi);
	break;

    case EDIT_COPY_BTN:
	copySelectedElementsIntoClipboard();
	break;

    case EDIT_PASTE_BTN:
	copyElementsIntoDisplay();
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;

    case EDIT_RAISE_BTN:
	raiseSelectedElements();
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;

    case EDIT_LOWER_BTN:
	lowerSelectedElements();
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;

    case EDIT_GROUP_BTN:
	groupObjects();
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;

    case EDIT_UNGROUP_BTN:
	ungroupSelectedElements();
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;

    case EDIT_SAME_SIZE_BTN:
	equalSizeSelectedElements();
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;

    case EDIT_GRID_SPACING_BTN:
	XDefineCursor(display,XtWindow(mainShell),watchCursor);
	if (!gridDlg) {
	    int n;
	    Arg args[4];
	    XmString xmString;
	    char label[1024];
	    
	    sprintf(label,"%d",cdi->gridSpacing);
	    xmString = XmStringCreateLocalized(label);
	    n = 0;
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
	    
	    sprintf(label,"%d",cdi->gridSpacing);
	    xmString = XmStringCreateLocalized(label);
	    n = 0;
	    XtSetArg(args[n],XmNtextString,xmString); n++;
	    XtSetValues(gridDlg,args,n);
	    XmStringFree(xmString);
	}
	XtManageChild(gridDlg);
	XUndefineCursor(display,XtWindow(mainShell));
	break;

    case EDIT_GRID_TOGGLE_BTN:
	cdi->gridOn=!cdi->gridOn;
	dmTraverseNonWidgetsInDisplayList(cdi);
	break;
	
    case EDIT_UNSELECT_BTN:
	unselectElementsInDisplay();
	break;

    case EDIT_SELECT_ALL_BTN:
	selectAllElementsInDisplay();
	break;

    case EDIT_REFRESH_BTN:
	refreshDisplay();
	break;	

    case EDIT_HELP_BTN:
    {
	XmString xmString=XmStringCreateLocalized(
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
	  "               (Shift afterward constrains direction.)\n"
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
	  "Ctrl-Arrow   Resize selected objects.\n"
	    );
	Arg args[20];
	int nargs;
	
	nargs=0;
	XtSetArg(args[nargs],XmNmessageString,xmString); nargs++;
	XtSetValues(editHelpMessageBox,args,nargs);
	XmStringFree(xmString);
	XtPopup(editHelpS,XtGrabNone);
	break;
    }
    }
}
    
#ifdef __cplusplus
static void alignHorizontalMenuSimpleCallback(Widget, XtPointer cd, XtPointer)
#else
static void alignHorizontalMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    DisplayInfo *cdi=currentDisplayInfo;
    int buttonNumber = (int) cd;
  
    switch(buttonNumber) {
      /* reuse the TextAlign values here */
    case ALIGN_HORIZ_LEFT_BTN:
	alignSelectedElements(HORIZ_LEFT);
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;
    case ALIGN_HORIZ_CENTER_BTN:
	alignSelectedElements(HORIZ_CENTER);
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;
    case ALIGN_HORIZ_RIGHT_BTN:
	alignSelectedElements(HORIZ_RIGHT);
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;
    case ALIGN_TO_GRID_BTN:
	alignSelectedElementsToGrid();
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;
    }
}

#ifdef __cplusplus
static void alignVerticalMenuSimpleCallback(Widget, XtPointer cd, XtPointer)
#else
static void alignVerticalMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    DisplayInfo *cdi=currentDisplayInfo;
    int buttonNumber = (int) cd;

    switch(buttonNumber) {
      /* reuse the TextAlign values here */
    case ALIGN_VERT_TOP_BTN:
	alignSelectedElements(VERT_TOP);
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;
    case ALIGN_VERT_CENTER_BTN:
	alignSelectedElements(VERT_CENTER);
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;
    case ALIGN_VERT_BOTTOM_BTN:
	alignSelectedElements(VERT_BOTTOM);
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;
    }
}

#ifdef __cplusplus
static void spaceMenuSimpleCallback(Widget, XtPointer cd, XtPointer)
#else
static void spaceMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    DisplayInfo *cdi=currentDisplayInfo;
    int buttonNumber = (int) cd;

    switch(buttonNumber) {
      /* reuse the TextAlign values here */
    case SPACE_HORIZ_BTN:
	spaceSelectedElements(HORIZONTAL);
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;
    case SPACE_VERT_BTN:
	spaceSelectedElements(VERTICAL);
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;
    case SPACE_2D_BTN:
	spaceSelectedElements2D();
	if (cdi->hasBeenEditedButNotSaved == False) 
	  medmMarkDisplayBeingEdited(cdi);
	break;
    }
}

#ifdef __cplusplus
static void mapCallback(Widget w, XtPointer , XtPointer)
#else
static void mapCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    Position X, Y;
    XmString xmString;

    XtTranslateCoords(currentDisplayInfo->shell,0,0,&X,&Y);
  /* try to force correct popup the first time */
    XtMoveWidget(XtParent(w),X,Y);

  /* be nice to the users - supply default text field as display name */
    xmString = XmStringCreateSimple(currentDisplayInfo->dlFile->name);
    XtVaSetValues(w,XmNtextString,xmString,NULL);
    XmStringFree(xmString);
}

static void fileTypeCallback(
  Widget w,
  int buttonNumber,
  XmToggleButtonCallbackStruct *call_data)
{
    Widget fsb;
    Arg args[4];
 
    if (call_data->set == False) return;
    switch(buttonNumber) {
    case 0:
	MedmUseNewFileFormat = True;
	break;
    case 1:
	MedmUseNewFileFormat = False;
	break;
    }
}

#ifdef __cplusplus
static void fileMenuSimpleCallback(Widget, XtPointer cd, XtPointer)
#else
static void fileMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int buttonNumber = (int) cd;
    Widget widget;
    static Widget radioBox = 0;
    XmString dirMask;
    XEvent event;

    switch(buttonNumber) {
    case FILE_NEW_BTN:
	currentDisplayInfo = createDisplay();
	XtManageChild(currentDisplayInfo->drawingArea);
	enableEditFunctions();
	break;

    case FILE_OPEN_BTN:
      /*
       * note - all FILE pdm entry dialogs are sharing a single callback
       *	  fileMenuDialogCallback, with client data
       *	  of the BTN id in the simple menu...
       */

      /*
       * create the Open... file selection dialog
       */
	XDefineCursor(display,XtWindow(mainShell),watchCursor);
	if (openFSD == NULL) {
	    Arg args[4];
	    int n = 0;
	    XmString label = XmStringCreateSimple("*.adl");
	  /* for some odd reason can't get PATH_MAX reliably defined between systems */
#define LOCAL_PATH_MAX  1023
	    char *cwd = getcwd(NULL,LOCAL_PATH_MAX+1);
	    XmString cwdXmString = XmStringCreateSimple(cwd);

	    XtSetArg(args[n],XmNpattern,label); n++;
	    XtSetArg(args[n],XmNdirectory,cwdXmString); n++;
	    openFSD = XmCreateFileSelectionDialog(XtParent(mainFilePDM),"openFSD",args,n);
	    XtUnmanageChild(XmFileSelectionBoxGetChild(openFSD,
	      XmDIALOG_HELP_BUTTON));
	    XtAddCallback(openFSD,XmNokCallback,
	      fileMenuDialogCallback,(XtPointer)FILE_OPEN_BTN);
	    XtAddCallback(openFSD,XmNcancelCallback,
	      fileMenuDialogCallback,(XtPointer)FILE_OPEN_BTN);
	    XmStringFree(label);
	    XmStringFree(cwdXmString);
	    free(cwd);
	}

	XmListDeselectAllItems(XmFileSelectionBoxGetChild(openFSD,XmDIALOG_LIST));
	XmFileSelectionDoSearch(openFSD,NULL);
	XtManageChild(openFSD);
	XUndefineCursor(display,XtWindow(mainShell));
	break;

    case FILE_SAVE_BTN:
    case FILE_SAVE_AS_BTN:

      /*
       * create the Open... file selection dialog
       */
	if (!saveAsPD) {
	    Arg args[10];
	    XmString buttons[NUM_IMAGE_TYPES-1];
	    XmButtonType buttonType[NUM_IMAGE_TYPES-1];
	    Widget rowColumn, frame, typeLabel;
	    int i, n;

	    XmString label = XmStringCreateSimple("*.adl");
	  /* for some odd reason can't get PATH_MAX reliably defined between systems */
#define LOCAL_PATH_MAX  1023
	    char *cwd = getcwd(NULL,LOCAL_PATH_MAX+1);
	    XmString cwdXmString = XmStringCreateSimple(cwd);

	    n = 0;
	    XtSetArg(args[n],XmNdefaultPosition,False); n++;
	    XtSetArg(args[n],XmNpattern,label); n++;
	    XtSetArg(args[n],XmNdirectory,cwdXmString); n++;
	    saveAsPD = XmCreateFileSelectionDialog(XtParent(mainFilePDM),
	      "saveAsFSD",args,n);
	    XtUnmanageChild(XmFileSelectionBoxGetChild(saveAsPD,
	      XmDIALOG_HELP_BUTTON));
	    XtAddCallback(saveAsPD,XmNokCallback,
	      fileMenuDialogCallback,(XtPointer)FILE_SAVE_AS_BTN);
	    XtAddCallback(saveAsPD,XmNcancelCallback,
	      fileMenuDialogCallback,(XtPointer)FILE_SAVE_AS_BTN);
	    XtAddCallback(saveAsPD,XmNmapCallback,mapCallback,(XtPointer)NULL);
	    XmStringFree(label);
	    XmStringFree(cwdXmString);
	    free(cwd);
	    n = 0;
	    XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
	    rowColumn = XmCreateRowColumn(saveAsPD,"rowColumn",args,n);
	    n = 0;
	    typeLabel = XmCreateLabel(rowColumn,"File Format",args,n);
 
	    buttons[0] = XmStringCreateSimple("2.2.x");
	    buttons[1] = XmStringCreateSimple("2.1.x");
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
	    for (i = 0; i < 2; i++) XmStringFree(buttons[i]);
	}

	if (!displayInfoListHead->next) break;
      /* no display, do nothing */
	if (displayInfoListHead->next != displayInfoListTail) {
	  /* more than one display, query user */
	    widget = XmTrackingEvent(mainShell,saveCursor,False,&event);
	    if (widget) {
		currentDisplayInfo = dmGetDisplayInfoFromWidget(widget);
	    }  
	} else {
	  /* only one display */
	    currentDisplayInfo = displayInfoListHead->next;
	}
	if (!currentDisplayInfo) break;
      /* for some reason, currentDisplay is not valid, break */
	if ((currentDisplayInfo->newDisplay)
	  || (buttonNumber == FILE_SAVE_AS_BTN)) {
	  /* new display or user wants to save as a different name */
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
	  /* save the file */
	    medmSaveDisplay(currentDisplayInfo,
	      currentDisplayInfo->dlFile->name,True);
	}
	break;

    case FILE_CLOSE_BTN:
	if (displayInfoListHead->next == displayInfoListTail) {
	  /* only one display; no need to query user */
	    widget = displayInfoListTail->drawingArea;
	} else
	  if (displayInfoListHead->next) {
	    /* more than one display; query user */
	      widget = XmTrackingEvent(mainShell,closeCursor,False, &event);
	      if (widget == (Widget) NULL) return;
	  } else {
	    /* no display */
	      return;
	  }
	closeDisplay(widget);
	break;

    case FILE_PRINT_SETUP_BTN:
	XDefineCursor(display,XtWindow(mainShell),watchCursor);
	if (!printerSetupDlg) {
	    int n = 0;
	    Arg args[4];
	    
	    XtSetArg(args[n],XmNdefaultPosition,False); n++;
	    printerSetupDlg = XmCreatePromptDialog(
	      XtParent(mainFilePDM),
	      "printerSetupPD",args,n);
	    XtUnmanageChild(XmSelectionBoxGetChild(
	      printerSetupDlg,XmDIALOG_HELP_BUTTON));
	    XtAddCallback(printerSetupDlg,XmNokCallback,printerSetupDlgCb,
	      PRINTER_SETUP_OK);
	    XtAddCallback(printerSetupDlg,XmNcancelCallback,
	      printerSetupDlgCb,(XtPointer)PRINTER_SETUP_CANCEL);
	    XtAddCallback(printerSetupDlg,XmNmapCallback,
	      printerSetupDlgCb,(XtPointer)PRINTER_SETUP_MAP);
	}
	XtManageChild(printerSetupDlg);
	XUndefineCursor(display,XtWindow(mainShell));
	break;

    case FILE_PRINT_BTN:
	if (displayInfoListHead->next == displayInfoListTail) {
	  /* only one display; no need to query user */
	    currentDisplayInfo = displayInfoListHead->next;
	    if (currentDisplayInfo != NULL)
	      utilPrint(XtDisplay(currentDisplayInfo->drawingArea),
		XtWindow(currentDisplayInfo->drawingArea),DISPLAY_XWD_FILE);

	} else
	  if (displayInfoListHead->next) {
	    /* more than one display; query user */
	      widget = XmTrackingEvent(mainShell,printCursor,False,&event);
	      if (widget != (Widget)NULL) {
		  currentDisplayInfo = dmGetDisplayInfoFromWidget(widget);
		  if (currentDisplayInfo != NULL) 
		    utilPrint(XtDisplay(currentDisplayInfo->drawingArea),
		      XtWindow(currentDisplayInfo->drawingArea),DISPLAY_XWD_FILE);
	      }
	  }
	break;

    case FILE_EXIT_BTN:
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
    xmString = XmStringCreateSimple(displayInfo->dlFile->name);
    XtVaSetValues(w,XmNtextString,xmString,NULL);
    XmStringFree(xmString);
    */
}
#endif

/* medm allowed a .template used as a suffix for compatibility. This exception is
   caused by a bug in the save routine at checking the ".adl" suffix.
*/

const char *templateSuffix = ".template";
Boolean medmSaveDisplay(DisplayInfo *displayInfo, char *filename, Boolean overwrite) {
    char *suffix;
    char f1[MAX_FILE_CHARS], f2[MAX_FILE_CHARS+4];
    char warningString[2*MAX_FILE_CHARS];
    int  strLen1, strLen2, strLen3, strLen4;
    int  status;
    FILE *stream;
    Boolean brandNewFile = False;
    Boolean templateException = False;
    struct stat statBuf;

    if (displayInfo == NULL) return False;
    if (filename == NULL) return False;

    strLen1 = strlen(filename);
    strLen2 = strlen(DISPLAY_FILE_BACKUP_SUFFIX);
    strLen3 = strlen(DISPLAY_FILE_ASCII_SUFFIX);
    strLen4 = strlen(templateSuffix);

 
    if (strLen1 >= MAX_FILE_CHARS) {
	medmPrintf("\nPath too Long: %s\n",filename);
	return False;
    }

  /* search for the position of the .adl suffix */
    strcpy(f1,filename);
    suffix = strstr(f1,DISPLAY_FILE_ASCII_SUFFIX);
    if ((suffix) && (suffix == f1 + strLen1 - strLen3)) {
      /* chop off the .adl suffix */
	*suffix = '\0';
	strLen1 = strLen1 - strLen3;
    } else {
      /* search for the position of the .template suffix */
	suffix = strstr(f1,templateSuffix);
	if ((suffix) && (suffix == f1 + strLen1 - strLen4)) { 
	  /* this is a .template special case */
	    templateException = True;
	}
    }
  

  /* create the backup file name with suffux _BAK.adl*/
    strcpy(f2,f1);
    strcat(f2,DISPLAY_FILE_BACKUP_SUFFIX);
    strcat(f2,DISPLAY_FILE_ASCII_SUFFIX);
  /* append the .adl suffix */
  /* check for the special case .template */
    if (!templateException) 
      strcat(f1,DISPLAY_FILE_ASCII_SUFFIX);

  /* See whether the file already exists. */
    if (access(f1,W_OK) == -1) {
	if (errno == ENOENT) {
	    brandNewFile = True;
	} else {
	    sprintf(warningString,"Fail to create/write file :\n%s",filename);
	    dmSetAndPopupWarningDialog(displayInfo,warningString,"OK",NULL,NULL);
	    return False;
	}
    } else {
      /* file exists, see whether the user wants to overwrite the file. */
	if (!overwrite) {
	    sprintf(warningString,"Do you want to overwrite file :\n%s",f1);
	    dmSetAndPopupQuestionDialog(displayInfo,warningString,"Yes","No",NULL);
	    switch (displayInfo->questionDialogAnswer) {
	    case 1 :
	      /* Yes, Save the file */
		break;
	    default :
	      /* No, return */
		return False;
	    }
	}
      /* see whether the backup file can be overwritten */
	if (access(f2,W_OK) == -1) {
	    if (errno != ENOENT) {
		sprintf(warningString,"Cannot write backup file :\n%s",filename);
		dmSetAndPopupWarningDialog(displayInfo,warningString,"OK",NULL,NULL);
		return False;
	    }
	}
	status = stat(f1,&statBuf);
	if (status) {
	    medmPrintf("\nFailed to read status of file %s\n",filename);
	    return False;
	}
	status = rename(f1,f2);
	if (status) {
	    medmPrintf("\nCannot rename file %s\n",filename);
	    return False;
	}
    }

    stream = fopen(f1,"w");
    if (stream == NULL) {
	sprintf(warningString,"Fail to create/write file :\n%s",filename);
	dmSetAndPopupWarningDialog(displayInfo,warningString,"OK",NULL,NULL);
	return False;
    }
    strcpy(displayInfo->dlFile->name,f1);
    dmWriteDisplayList(displayInfo,stream);
    fclose(stream);
    displayInfo->hasBeenEditedButNotSaved = False;
    displayInfo->newDisplay = False;
    medmSetDisplayTitle(displayInfo);
    if (!brandNewFile) {
	chmod(f1,statBuf.st_mode);
    }
    return True;
}

void medmExit() {
    char *filename, *tmp;
    char str[2*MAX_FILE_CHARS];
    Arg args[2];
    Boolean saveAll = False;
    Boolean saveThis = False;

    DisplayInfo *displayInfo = displayInfoListHead->next;

    while (displayInfo) {
	if (displayInfo->hasBeenEditedButNotSaved) {
	    if (saveAll == False) {
		filename = tmp = displayInfo->dlFile->name;
	      /* strip off the path */
		while (*tmp != '\0') {
		    if (*tmp == '/') 
		      filename = tmp+1;
		    tmp++;
		}
		sprintf(str,"Save display \"%s\" before exit?",filename);
#ifdef PROMPT_TO_EXIT
	      /* Don't use Cancel, use All (Only 3 buttons) */
		if (displayInfo->next)
		  dmSetAndPopupQuestionDialog(displayInfo,str,"Yes","No","All");
		else
		  dmSetAndPopupQuestionDialog(displayInfo,str,"Yes","No",NULL);
#else
	      /* Use Cancel, don't use All (Only 3 buttons) */
		if (displayInfo->next)
		  dmSetAndPopupQuestionDialog(displayInfo,str,"Yes","No","Cancel");
		else
		  dmSetAndPopupQuestionDialog(displayInfo,str,"Yes","No","Cancel");
#endif
		switch (displayInfo->questionDialogAnswer) {
		case 1 :
		  /* Yes, save this file */
		    saveThis = True;
		    break;
		case 2 :
		  /* No, check next file */
		    saveThis = False;
		    break;
		case 3 :
#ifdef PROMPT_TO_EXIT
		  /* Save all files */
		    saveAll = True;
		    saveThis = True;
		    break;
#else
		  /* Cancel */
		    return;
#endif
		default :
		    saveThis = False;
		    break;
		}
	    }
	    if (saveThis == True) 
	      if (medmSaveDisplay(displayInfo,
		displayInfo->dlFile->name,True) == False) return;
	}
	displayInfo = displayInfo->next;
    }
#ifdef PROMPT_TO_EXIT
  /* Prompt to exit */
    XtVaSetValues(mainShell,
      XmNiconic, False,
      NULL);
    XtPopup(mainShell,XtGrabNone);
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

    char backupFilename[MAX_FILE_CHARS];

    switch(call_data->reason){
    case XmCR_CANCEL:
	XtUnmanageChild(w);
	break;
    case XmCR_OK:
	switch(btn) {
        case FILE_OPEN_BTN: {
	    FILE *filePtr;
	    char *filename;

	    XmSelectionBoxCallbackStruct *call_data =
	      (XmSelectionBoxCallbackStruct *) callbackStruct;

          /* if no list element selected, simply return */
	    if (call_data->value == NULL) return;

          /* get the filename string from the selection box */
	    XmStringGetLtoR(call_data->value, XmSTRING_DEFAULT_CHARSET, &filename);

	    if (filename) {
		filePtr = fopen(filename,"r");
		if (filePtr) {
		    XtUnmanageChild(w);
		    dmDisplayListParse(NULL,filePtr,NULL,filename,NULL,(Boolean)False);
		    enableEditFunctions();
		    if (filePtr) fclose(filePtr);
		}
		XtFree(filename);
	    }
	    break;
        }
        case FILE_CLOSE_BTN:
	    dmRemoveDisplayInfo(currentDisplayInfo);
	    currentDisplayInfo = NULL;
	    break;
        case FILE_SAVE_AS_BTN:
	    select = (XmSelectionBoxCallbackStruct *)call_data;
	    XmStringGetLtoR(select->value,XmSTRING_DEFAULT_CHARSET,&filename);
	    medmSaveDisplay(currentDisplayInfo,filename,False);
	    sprintf(warningString,"%s","Name of file to save display in:");
	    warningXmstring = XmStringCreateSimple(warningString);
	    XtVaSetValues(saveAsPD,XmNselectionLabelString,warningXmstring,NULL);
	    XmStringFree(warningXmstring);
	    XtFree(filename);
	    XtUnmanageChild(w);
	    break;
        case FILE_EXIT_BTN:
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

#ifdef __cplusplus
static void palettesMenuSimpleCallback(Widget, XtPointer cd, XtPointer)
#else
static void palettesMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int buttonNumber = (int) cd;
  
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

#ifdef __cplusplus
static void helpMenuSimpleCallback(Widget, XtPointer cd, XtPointer cbs)
#else
static void helpMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    int buttonNumber = (int)cd;
    XmAnyCallbackStruct *call_data = (XmAnyCallbackStruct *)cbs;
    Widget widget;
    XEvent event;
    
    switch(buttonNumber) {
      /* implement context sensitive help */
/*     case HELP_OVERVIEW_BTN: */
/* 	widget = XmTrackingEvent(mainShell,helpCursor,False,&event); */
/* 	if (widget != (Widget)NULL) { */
/* 	    call_data->reason = XmCR_HELP; */
/* 	    XtCallCallbacks(widget,XmNhelpCallback,&call_data); */
/* 	} */
/* 	break; */
    case HELP_OVERVIEW_BTN:
	callBrowser(MEDM_HELP_PATH"/MEDM.html#Overview");
	break;
    case HELP_CONTENTS_BTN:
	callBrowser(MEDM_HELP_PATH"/MEDM.html#Contents");
	break;
    case HELP_OBJECTS_BTN:
	callBrowser(MEDM_HELP_PATH"/MEDM.html#ObjectIndex");
	break;
    case HELP_EDIT_BTN:
	callBrowser(MEDM_HELP_PATH"/MEDM.html#Editing");
	break;
    case HELP_NEW_BTN:
	callBrowser(MEDM_HELP_PATH"/MEDM.html#NewFeatures");
	break;
    case HELP_ON_HELP_BTN:
    {
	XmString xmString=XmStringCreateLocalized(
	  "     Help in this version of MEDM is implemented using Netscape.  If\n"
	  "the environmental variable NETSCAPEPATH containing the full pathname\n"
	  "of the Netscape executable exists, then that path is used to call\n"
	  "Netscape.  Otherwise, it is called using just the command, netscape.\n"
	  "If Netscape is not available, then most of the MEDM help is not\n"
	  "available.\n"
	  "\n"
	  "     If Netscape is running when MEDM first calls it, then the\n"
	  "response should be fairly quick.  Otherwise, the first call to help\n"
	  "must wait until Netscape comes up, which will take somewhat longer.\n"
	    );
	Arg args[20];
	int nargs;
	
	nargs=0;
	XtSetArg(args[nargs],XmNmessageString,xmString); nargs++;
	XtSetValues(helpMessageBox,args,nargs);
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
	    printf("\nEdit/Execute: globalDisplayListTraversalMode=%d [DL_EXECUTE=%d DL_EDIT=%d]\n",
	      globalDisplayListTraversalMode,DL_EXECUTE,DL_EDIT);
	    printf("modeRB(%x): XmNradioBehavior=%d \n",modeRB,(int)radioBehavior);
	    
	    nargs=0;
	    XtSetArg(args[nargs],XmNindicatorType,&indicatorType); nargs++;
	    XtSetArg(args[nargs],XmNset,&set); nargs++;
	    XtGetValues(modeEditTB,args,nargs);
	    printf("modeEditTB(%x): XmNset=%d  XmNindicatorType=%d "
	      "[XmN_OF_MANY=%d XmONE_OF_MANY=%d]\n",
	      modeEditTB,(int)set,(int)indicatorType,(int)XmN_OF_MANY,(int)XmONE_OF_MANY);

	    nargs=0;
	    XtSetArg(args[nargs],XmNindicatorType,&indicatorType); nargs++;
	    XtSetArg(args[nargs],XmNset,&set); nargs++;
	    XtGetValues(modeExecTB,args,nargs);
	    printf("modeExecTB(%x): XmNset=%d  XmNindicatorType=%d "
	      "[XmN_OF_MANY=%d XmONE_OF_MANY=%d]\n",
	      modeExecTB,(int)set,(int)indicatorType,(int)XmN_OF_MANY,(int)XmONE_OF_MANY);
	}
#endif
	break;
    }
}

#ifdef __cplusplus
static void helpDialogCallback(Widget, XtPointer, XtPointer cbs)
#else
static void helpDialogCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    switch(((XmAnyCallbackStruct *) cbs)->reason){
    case XmCR_OK:
    case XmCR_CANCEL:
	XtPopdown(helpS);
	break;
    }
}

#ifdef __cplusplus
static void editHelpDialogCallback(Widget, XtPointer, XtPointer cbs)
#else
static void editHelpDialogCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    switch(((XmAnyCallbackStruct *) cbs)->reason){
    case XmCR_OK:
    case XmCR_CANCEL:
	XtPopdown(editHelpS);
	break;
    }
}

#ifdef __cplusplus
static void modeCallback(Widget, XtPointer cd, XtPointer cbs)
#else
static void modeCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    DlTraversalMode mode = (DlTraversalMode)cd;
    XmToggleButtonCallbackStruct *call_data = (XmToggleButtonCallbackStruct *)cbs;
    DisplayInfo *displayInfo;

#if DEBUG_RADIO_BUTTONS
    {
	Boolean radioBehavior,set;
	unsigned char indicatorType;
	Arg args[20];
	int nargs;
	
	nargs=0;
	XtSetArg(args[nargs],XmNradioBehavior,&radioBehavior); nargs++;
	XtGetValues(XtParent(w),args,nargs);
	printf("\nmodeCallback: mode=%d [DL_EXECUTE=%d DL_EDIT=%d]\n",
	  mode,DL_EXECUTE,DL_EDIT);
	printf("\nParent(%x): XmNradioBehavior=%d \n",XtParent(w),(int)radioBehavior);

	nargs=0;
	XtSetArg(args[nargs],XmNindicatorType,&indicatorType); nargs++;
	XtSetArg(args[nargs],XmNset,&set); nargs++;
	XtGetValues(w,args,nargs);
	printf("Widget(%x): XmNindicatorType=%d "
	  "[XmN_OF_MANY=%d XmONE_OF_MANY=%d]\n",
	  w,(int)indicatorType,(int)XmN_OF_MANY,(int)XmONE_OF_MANY);
	printf("Widget(%x): XmNset=%d\n",w,(int)set);
	printf("If both the Edit and the Execute buttons are down,"
	  " select \"On Version\"\n  on the Help menu to get more information."
	  "  Send to evans@aps.anl.gov.\n");
    }
#endif

  /* Since both on & off will invoke this callback, only care about transition
   *   of one to ON (with real change of state) */
    if (call_data->set == False ||
      globalDisplayListTraversalMode == mode) return;

  /* Set all the displayInfo->traversalMode(s) to the specified mode, and
   *   then invoke the traversal */
    globalDisplayListTraversalMode = mode;

  /* Unselect any selected elements */
    unselectElementsInDisplay();

  /* Disable EDIT functions */
    disableEditFunctions();

  /* Mode is the mode to which we are going */
    switch(mode) {
    case DL_EDIT:
	updateAllDisplayPositions();
	if (relatedDisplayS) XtSetSensitive(relatedDisplayS,True);
	if (cartesianPlotS) XtSetSensitive(cartesianPlotS,True);
	if (cartesianPlotAxisS) {
	    XtSetSensitive(cartesianPlotAxisS,True);
	    XtPopdown(cartesianPlotAxisS);
	}
	if (stripChartS) XtSetSensitive(stripChartS,True);
	XtSetSensitive(fileMenu[FILE_NEW_BTN].widget,True);
	XtSetSensitive(fileMenu[FILE_SAVE_BTN].widget,True);
	XtSetSensitive(fileMenu[FILE_SAVE_AS_BTN].widget,True);
        if (medmWorkProcId) {
	    XtRemoveWorkProc(medmWorkProcId);
	    medmWorkProcId = 0;
        }
	break;

    case DL_EXECUTE:
	updateAllDisplayPositions();
	if (relatedDisplayS) {
	    XtSetSensitive(relatedDisplayS,False);
	    XtPopdown(relatedDisplayS);
	}
	if (cartesianPlotS) {
	    XtSetSensitive(cartesianPlotS,False);
	    XtPopdown(cartesianPlotS);
	}
	if (cartesianPlotAxisS) {
	    XtSetSensitive(cartesianPlotAxisS,False);
	    XtPopdown(cartesianPlotAxisS);
	}
	if (stripChartS) {
	    XtSetSensitive(stripChartS,False);
	    XtPopdown(stripChartS);
	}
	if (pvInfoS) {
	    XtSetSensitive(pvInfoS,False);
	    XtPopdown(pvInfoS);
	}
	XtSetSensitive(fileMenu[FILE_NEW_BTN].widget,False);
	XtSetSensitive(fileMenu[FILE_SAVE_BTN].widget,False);
	XtSetSensitive(fileMenu[FILE_SAVE_AS_BTN].widget,False);
        medmStartUpdateCAStudyDlg();
	break;

    default:
	break;
    }

    executeTimeCartesianPlotWidget = NULL;
  /* No display is current */
    currentDisplayInfo = (DisplayInfo *)NULL;

  /* Go through the whole display list, if any display is
   *   brought up as a related display, shut down that display
   *   and remove that display from the display list, otherwise,
   *   just shutdown that display. */
    displayInfo = displayInfoListHead->next;
    while (displayInfo) {
	DisplayInfo *pDI = displayInfo;

	displayInfo->traversalMode = mode;
	displayInfo = displayInfo->next;
	if (pDI->fromRelatedDisplayExecution) {
	    dmRemoveDisplayInfo(pDI);
	} else {
	    dmCleanupDisplayInfo(pDI,False);
	}
    }

  /* See whether there is any display in the display list.
   *   If any, enable resource palette, object palette and
   *   color palette, traverse the whole display list. */
    if (displayInfoListHead->next) {
	if (globalDisplayListTraversalMode == DL_EDIT) {
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
	if (t > 0.5) {
	    printf("modecallback : time used by ca_pend_event = %8.1f\n",t);
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

  /* no write access cursor */
    colors[0].pixel = alarmColorPixel[MAJOR_ALARM];
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
    if (medmUseBigCursor) {
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
    if (medmUseBigCursor) {
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
    if (medmUseBigCursor) {
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
    if (medmUseBigCursor) {
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
    if (medmUseBigCursor) {
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
    if (menuType == XmMENU_PULLDOWN) {
	menu = XmCreatePulldownMenu(parent, "_pulldown",NULL,0);
	str = XmStringCreateSimple(menuTitle);
	cascade = XtVaCreateManagedWidget(menuTitle,
	  xmCascadeButtonGadgetClass, parent,
	  XmNsubMenuId, menu,
	  XmNlabelString, str,
	  XmNmnemonic, menuMnemonic,
	  NULL);
	XmStringFree(str);
    } else {
	menu = XmCreatePopupMenu(parent, "_popup", NULL, 0);
    }

  /* now add the menu items */
    for (i=0;items[i].label != NULL; i++) {
      /* if subitems exist, create the pull-right menu by calling this
       * function recursively. Since the function returns a cascade
       * button, the widget returned is used..
       */

	if (items[i].subItems) {
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
	if (items[i].mnemonic) {
	    XtVaSetValues(items[i].widget, XmNmnemonic, items[i].mnemonic, NULL);
	}

      /* any item can have an accelerator, execpt cascade menus. But,
       * we don't worry about that; we know better in our declarations.
       */
	if (items[i].accelerator) {
	    str = XmStringCreateSimple(items[i].accText);
	    XtVaSetValues(items[i].widget,
	      XmNaccelerator, items[i].accelerator,
	      XmNacceleratorText, str,
	      NULL);
	    XmStringFree(str);
	}
      /* again, anyone can have a callback -- however, this is an
       * activate-callback.  This may not be appropriate for all items.
       */
	if (items[i].callback) {
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
    if (sig==SIGQUIT)      fprintf(stderr,"\nSIGQUIT\n");
    else if (sig==SIGINT)  fprintf(stderr,"\nSIGINT\n");
    else if (sig==SIGTERM) fprintf(stderr,"\nSIGTERM\n");
    else if (sig==SIGSEGV) fprintf(stderr,"\nSIGSEGV\n");
    else if (sig==SIGBUS) fprintf(stderr,"\nSIGBUS\n");

  /* Remove the property on the root window (if not LOCAL) */
    if (windowPropertyAtom != (Atom)NULL)
      XDeleteProperty(display,rootWindow,windowPropertyAtom);
    XFlush(display);
    
  /* Exit */
    if (sig == SIGSEGV || sig == SIGBUS) {
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
    while (ptr[0] != '\0') {
	if (index == MAX_CHARS_IN_CLIENT_MESSAGE) {
	    XSendEvent(display,targetWindow,True,NoEventMask,
	      (XEvent *)&clientMessageEvent);
	    index = 0;
	}
	clientMessageEvent.data.b[index++] = ptr[0];
	ptr++;
    }

  /* ; delimiter */
    if (index == MAX_CHARS_IN_CLIENT_MESSAGE) {
	XSendEvent(display,targetWindow,True,NoEventMask,
	  (XEvent *)&clientMessageEvent);
	index = 0;
    }
    clientMessageEvent.data.b[index++] = ';';

  /* body of macro string if one was specified */
    if ((ptr = macroString) != NULL) {
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

  /* ; delimiter */
    if (index == MAX_CHARS_IN_CLIENT_MESSAGE) {
	XSendEvent(display,targetWindow,True,NoEventMask,
	  (XEvent *)&clientMessageEvent);
	index = 0;
    }
    clientMessageEvent.data.b[index++] = ';';

  /* body of geometry string if one was specified */
    if ((ptr = geometryString) != NULL) {
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
      clientMessageEvent.data.b[i] = ' ';
    XSendEvent(display,targetWindow,True,NoEventMask,
      (XEvent *)&clientMessageEvent);

}

char token[MAX_TOKEN_LENGTH];
DisplayInfo* parseDisplayFile(char *filename) {
    DisplayInfo *displayInfo = 0;
    FILE *filePtr;
    TOKEN tokenType;
    if (filePtr = fopen(filename,"r")) {
	displayInfo = (DisplayInfo *)malloc(sizeof(DisplayInfo));
	displayInfo->dlElementList = createDlList();
	currentDisplayInfo = displayInfo;
	displayInfo->filePtr = filePtr;
      /* if first token isn't "file" then bail out! */
	tokenType=getToken(displayInfo,token);
	if (tokenType == T_WORD && !strcmp(token,"file")) {
	    displayInfo->dlFile = parseFile(displayInfo);
	    if (displayInfo->dlFile) {
		displayInfo->versionNumber = displayInfo->dlFile->versionNumber;
		strcpy(displayInfo->dlFile->name,filename);
	    }
	} else {
	    return 0;
	}
	tokenType=getToken(displayInfo,token);
	if (tokenType ==T_WORD && !strcmp(token,"display")) {
	    parseDisplay(displayInfo);
	}
	tokenType=getToken(displayInfo,token);
	if (tokenType == T_WORD && (!strcmp(token,"color map") ||
	  !strcmp(token,"<<color map>>"))) {
	    displayInfo->dlColormap=parseColormap(displayInfo,displayInfo->filePtr);
	} else {
	    return 0;
	}
 
      /*
       * proceed with parsing
       */
	while (parseAndAppendDisplayList(displayInfo,displayInfo->dlElementList)
	  != T_EOF );
	fclose(filePtr);
    }
    return displayInfo;
}

/**************************************************************************/
/**************************** main ****************************************/
/**************************************************************************/
main(int argc, char *argv[])
{
    int i, n, index;
    Arg args[5];
    FILE *filePtr;
    XColor color;
    XEvent event;
    char versionString[60];
    Window targetWindow;

    Boolean attachToExistingMedm, completeClientMessage;
    char fullPathName[FULLPATHNAME_SIZE+1],
      currentDirectoryName[FULLPATHNAME_SIZE+1], name[FULLPATHNAME_SIZE+1];
    unsigned char *propertyData;
    int status, format;
    unsigned long nitems, left;
    Atom type;
    char *macroString = NULL, *ptr;
    XColor colors[2];
    request_t *request;
    typedef enum {FILENAME_MSG,MACROSTR_MSG,GEOMETRYSTR_MSG} msgClass_t;
    msgClass_t msgClass;
    Window medmHostWindow = (Window)0;

#if DEBUG_DEFINITIONS
    printf("\n");
#ifdef __EXTENSIONS__
    printf("__EXTENSIONS__= % d\n",__EXTENSIONS__);
#else      
    printf("__EXTENSIONS__ is undefined\n");
#endif
#ifdef __STDC__
    printf("__STDC__ = %d\n",__STDC__);
    printf("__STDC__ - 0 = %d\n",__STDC__ - 0);
#else      
    printf("__STDC__ is undefined\n");
    printf("__STDC__ - 0 = %d\n",__STDC__ - 0);
#endif
#ifdef _POSIX_C_SOURCE
    printf("_POSIX_C_SOURCE = %d\n",_POSIX_C_SOURCE);
#else      
    printf("_POSIX_C_SOURCE is undefined\n");
#endif
#ifdef _XOPEN_SOURCE
    printf("_XOPEN_SOURCE = %d\n",_XOPEN_SOURCE);
#else      
    printf("_XOPEN_SOURCE is undefined\n");
#endif
#ifdef _NO_LONGLONG
    printf("_NO_LONGLONG = %d\n",_NO_LONGLONG);
#else      
    printf("_NO_LONGLONG is undefined\n");
#endif
#endif

  /* Initialize global variables */
    windowPropertyAtom = (Atom)NULL;
    medmWorkProcId = 0;
    medmUpdateRequestCount = 0;
    nextToServe = NULL;
    medmCAEventCount = 0;
    medmScreenUpdateCount = 0;
    medmUpdateMissedCount = 0;
    MedmUseNewFileFormat = True;
    setTimeValues();

  /* Handle file conversions */
    if (argc == 4 && (!strcmp(argv[1],"-c21x") ||
      !strcmp(argv[1],"-c22x"))) {
	DisplayInfo *displayInfo = NULL;
	FILE *filePtr;
	initMedmCommon();
	if (displayInfo = parseDisplayFile(argv[2])) {
	    filePtr = fopen(argv[3],"w");
	    if (filePtr) {
		strcpy(displayInfo->dlFile->name,argv[3]);
		if (!strcmp(argv[1],"-c21x")) {
		    MedmUseNewFileFormat = False;
		} else {
		    MedmUseNewFileFormat = True;
		}
		dmWriteDisplayList(displayInfo,filePtr);
		fclose(filePtr);
	    } else {
		medmPrintf("\nCannot create display file: \"%s\"\n",argv[3]);
	    }
	} else {
	    medmPrintf("\nCannot open display file: \"%s\"\n",argv[2]);
	}
	return 0;
    }

   /* Initialize channel access here (to get around orphaned windows) */
    SEVCHK(ca_task_initialize(),"\nmain: error in ca_task_initialize");

  /* Parse command line */
    request = parseCommandLine(argc,argv);

    if (request->macroString != NULL && request->opMode != EXECUTE) {
	medmPrintf("\nIgnored -macro command line option\n"
	  "  (Only valid for Execute (-x) mode operation)\n");
	free(request->macroString);
	request->macroString = NULL;
    }

  /* Usage and error exit */
    if (request->opMode == HELP) {
	printf("\n%s\n",MEDM_VERSION_STRING);
	printf("Usage:\n"
	  "  medm [X options]\n"
	  "  [-help | -?]\n"
	  "  [-version]\n"
	  "  [-x | -e]\n"
	  "  [-local | -cleanup]\n"
	  "  [-cmap]\n"
	  "  [-bigMousePointer]\n"
	  "  [-displayFont font-spec]\n"
	  "  [-macro \"xxx=aaa,yyy=bbb, ...\"]\n"
	  "  [-dg [xpos[xypos]][+xoffset[+yoffset]]\n"
	  "  [display-files]\n"
	  "  [&]\n"
	  "\n");
	exit(0);
    } else if (request->opMode == VERSION) {
	printf("\n%s\n",MEDM_VERSION_STRING);
	exit(0);
    }

  /* Do remote protocol stuff if not LOCAL */
    if (request->medmMode != LOCAL) {
      /* Open display */
	display = XOpenDisplay(request->displayName);
	if (display == NULL) {
	    medmPrintf("\nCould not open Display\n");
	    exit(1);
	}
	screenNum = DefaultScreen(display);
	rootWindow = RootWindow(display,screenNum);

      /*   Intern the appropriate atom if it doesn't exist (False implies this) */
	if (request->fontStyle == FIXED) {
	    if (request->opMode == EXECUTE) {
		windowPropertyAtom = XInternAtom(display,MEDM_VERSION_DIGITS"_EXEC_FIXED",False);
	    } else {
		windowPropertyAtom = XInternAtom(display,MEDM_VERSION_DIGITS"_EDIT_FIXED",False);
	    }
	} else if (request->fontStyle == SCALABLE) {
	    if (request->opMode == EXECUTE) {
		windowPropertyAtom = XInternAtom(display,MEDM_VERSION_DIGITS"_EXEC_SCALABLE",False);
	    } else {
		windowPropertyAtom = XInternAtom(display,MEDM_VERSION_DIGITS"_EDIT_SCALABLE",False);
	    }
	} 

      /*   Get the property  (Should a the mainShell window number)
       *     type:          Actual type of the property (None if it doesn't exist)
       *     propertyData:  The value of the property */
	status = XGetWindowProperty(display,rootWindow,windowPropertyAtom,
	  0,FULLPATHNAME_SIZE,(Bool)False,AnyPropertyType,&type,
	  &format,&nitems,&left,&propertyData);

      /* Decide whether to attach to existing MEDM */
	if (type != None) {
	    medmHostWindow = *((Window *)propertyData);
	    attachToExistingMedm = (request->medmMode == CLEANUP) ? False : True;
	    XFree(propertyData);
	} else {
	    attachToExistingMedm = False;
	}

      /* Attach to existing MEDM if appropriate
       *   Note that we only know there is a property
       *   We do not know if there actually is an MEDM running */	
	if (attachToExistingMedm) {
	    XWindowAttributes attr;
	    char *fileStr;
	    int i, status;

	  /* Check if the medmHostWindow is valid */
	    XSetErrorHandler(xErrorHandler);     /* Otherwise exits */
	    status=XGetWindowAttributes(display,medmHostWindow,&attr);
	    if(!status) {
	      /* Window doesn't exist */
		printf("\nCannot connect to existing MEDM because it is invalid\n"
		  "  Continuing with this one as if -cleanup were specified\n");
		printf("(Use -local to not use existing MEDM or be available as an existing MEDM\n"
		  "  or -cleanup to set this MEDM as the existing one)\n");
	    } else {
	      /* Window does exist */
	      /* Check if there were valid display files specified */
		if (request->fileCnt > 0) {
		    printf("\nAttaching to existing MEDM\n");
		    for (i=0; i<request->fileCnt; i++) {
			if (fileStr = request->fileList[i]) {
			    sendFullPathNameAndMacroAsClientMessages(medmHostWindow,fileStr,
			      request->macroString,request->displayGeometry,windowPropertyAtom);
			    XFlush(display);
			    printf("  Dispatched: %s\n",fileStr);
			}
		    }
		} else {
		    printf("\nAborting: No valid display specified and already a remote MEDM running.\n");
		}
		printf("(Use -local to not use existing MEDM or be available as an existing MEDM\n"
		  "  or -cleanup to set this MEDM as the existing one)\n");
		
	      /* Leave this MEDM */
		XCloseDisplay(display);
		exit(0);
	    }
	}  

      /* Close the display that was opened (Will start over later) */
	XCloseDisplay(display);
    } /* End if(request->medmMode != LOCAL) */

  /* Initialize the Intrinsics
   *   Create mainShell
   *   Map window manager menu Close function to do nothing
   *     (Will handle this ourselves) */
    n = 0;
    XtSetArg(args[n],XmNdeleteResponse,XmDO_NOTHING); n++;
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;
    mainShell = XtAppInitialize(&appContext, CLASS, NULL, 0, &argc, argv,
      fallbackResources, args, n);

  /* Set error handlers */
    XtAppSetErrorHandler(appContext,xtErrorHandler);
    XtAppSetWarningHandler(appContext,xtErrorHandler);
    XSetErrorHandler(xErrorHandler);
    

  /* Enable Editres */
#ifdef EDITRES    
    XtAddEventHandler(mainShell,(EventMask)NULL,TRUE,_XEditResCheckMessages,NULL);
#endif
    
  /* Add necessary Motif resource converters */
    XmRegisterConverters();
    XmRepTypeInstallTearOffModelConverter();

  /* Set display and related quantities */
    display = XtDisplay(mainShell);
    if (display == NULL) {
	XtWarning("Cannot open display");
	exit(-1);
    }
    screenNum = DefaultScreen(display);
    rootWindow = RootWindow(display,screenNum);
    cmap = DefaultColormap(display,screenNum);	/* X default colormap */

  /* Set XSynchronize for debugging */
#ifdef XSYNC
    XSynchronize(display,TRUE);
    medmPrintf("\nRunning in SYNCHRONOUS mode\n");
#endif

  /* Intern some atoms if they aren't there already */
    WM_DELETE_WINDOW = XmInternAtom(display,"WM_DELETE_WINDOW",False);
    WM_TAKE_FOCUS =  XmInternAtom(display,"WM_TAKE_FOCUS",False);
    COMPOUND_TEXT =  XmInternAtom(display,"COMPOUND_TEXT",False);

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

  /* Add translations/actions for drag-and-drop */
    parsedTranslations = XtParseTranslationTable(dragTranslations);
    XtAppAddActions(appContext,dragActions,XtNumber(dragActions));

    if (request->opMode == EDIT) {
	globalDisplayListTraversalMode = DL_EDIT;
    } else
      if (request->opMode == EXECUTE) {
	  globalDisplayListTraversalMode = DL_EXECUTE;
	  if (request->fileCnt > 0) {	/* assume .adl file names follow */
	      XtVaSetValues(mainShell,
		XmNinitialState,IconicState,
		NULL);
	  }
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
    cartesianPlotS = NULL;
    cartesianPlotAxisS = NULL;
    stripChartS = NULL;
    cpAxisForm = NULL;
    executeTimeCartesianPlotWidget = NULL;
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
    if (request->privateCmap) {
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
	if (colors[0].pixel == 0) XAllocColor(display,cmap,&(colors[0]));
	else XAllocColor(display,cmap,&(colors[1]));
	if (colors[1].pixel == 1) XAllocColor(display,cmap,&(colors[1]));
	else XAllocColor(display,cmap,&(colors[0]));
    }
    
  /* Allocate colors */
    for (i = 0; i < DL_MAX_COLORS; i++) {
      /* Scale [0,255] to [0,65535] */
	color.red  = (unsigned short) COLOR_SCALE*(defaultDlColormap.dl_color[i].r);
	color.green= (unsigned short) COLOR_SCALE*(defaultDlColormap.dl_color[i].g);
	color.blue = (unsigned short) COLOR_SCALE*(defaultDlColormap.dl_color[i].b);
      /* Allocate a shareable color cell with closest RGB value */
	if (XAllocColor(display,cmap,&color)) {
	    defaultColormap[i] =  color.pixel;
	} else {
	    medmPrintf("\nmain: Cannot not allocate color (%d: "
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

  /* We're the first MEDM around in this mode - proceed with full execution
   *   Store mainShell window as the property if the atom is defined
   *     (Will be stored if CLEANUP or first MEDM, won't be stored if LOCAL)  */
    targetWindow = XtWindow(mainShell);
    if(windowPropertyAtom)
      XChangeProperty(display,rootWindow,windowPropertyAtom,
	XA_WINDOW,32,PropModeReplace,(unsigned char *)&targetWindow,1);

  /* Start any command-line specified displays */
    for (i=0; i < request->fileCnt; i++) {
	char *fileStr;
	if (fileStr = request->fileList[i]) {
	    if (filePtr = fopen(fileStr,"r")) {
		dmDisplayListParse(NULL,filePtr,request->macroString,fileStr,
		  request->displayGeometry,(Boolean)False);
		fclose(filePtr);
	    } else {
		medmPrintf("\nCannot open display file: \"%s\"\n",fileStr);
	    }
	}
    }
    if ((displayInfoListHead->next) &&
      (globalDisplayListTraversalMode == DL_EDIT)) {
	enableEditFunctions();
    }

  /* Create and popup the product description shell
   *  (use defaults for fg/bg) */
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

  /* Add callback for disabled window manager Close function
   *   Need this later than shell creation for some reason (?) */
    XmAddWMProtocolCallback(mainShell,WM_DELETE_WINDOW,
      wmCloseCallback, (XtPointer) OTHER_SHELL);

  /* Add the initialization work proc */
    XtAppAddWorkProc(appContext,medmInitWorkProc,NULL);

#ifdef __TED__
  /* Get CDE workspace list */
    GetWorkSpaceList(mainMW);
#endif

  /* Go into event loop
   *   Normally just XtAppMainLoop(appContext)
   *     but we want to handle remote requests from other MEDM's */
    while (True) {
	XtAppNextEvent(appContext,&event);
	switch (event.type) {
	case ClientMessage:
	    if(windowPropertyAtom && event.xclient.message_type == windowPropertyAtom) {
	      /* Request from remote MEDM */
		char geometryString[256];

	      /* Concatenate ClientMessage events to get full name from form: (xyz) */
		completeClientMessage = False;
		for (i = 0; i < MAX_CHARS_IN_CLIENT_MESSAGE; i++) {
		    switch (event.xclient.data.b[i]) {
		      /* Start with filename */
		    case '(':  index = 0;
			ptr = fullPathName;
			msgClass = FILENAME_MSG;
			break;
		      /* Keep filling in until ';', then start macro string if any */
		    case ';':  ptr[index++] = '\0';
			if (msgClass == FILENAME_MSG) {
			    msgClass = MACROSTR_MSG;
			    ptr = name;
			} else {
			    msgClass = GEOMETRYSTR_MSG;
			    ptr = geometryString;
			}
			index = 0;
			break;
		      /* Terminate whatever string is being filled in */
		    case ')':  completeClientMessage = True;
			ptr[index++] = '\0';
			break;
		    default:   ptr[index++] = event.xclient.data.b[i];
			break;
		    }
		}

		if (completeClientMessage) {
		    filePtr = fopen(fullPathName,"r");
		    if (filePtr) {
			dmDisplayListParse(NULL,filePtr,name,fullPathName,geometryString,
			  (Boolean)False);
			if (globalDisplayListTraversalMode == DL_EDIT) {
			    enableEditFunctions();
			}
			medmPostMsg("File Dispatch Request:\n");
			if (fullPathName[0] != '\0')
			  medmPrintf("  filename = %s\n",fullPathName);
			if (name[0] != '\0')
			  medmPrintf("  macro = %s\n",name);
			if (geometryString[0] != '\0')
			  medmPrintf("  geometry = %s\n",geometryString);
			fclose(filePtr);
		    } else {
			medmPrintf(
			  "\nCould not open requested file\n\t\"%s\"\n  from remote MEDM request\n",
			  fullPathName);
		    }
		}
	    } else {
	      /* Handle these ClientMessage's the normal way */
		XtDispatchEvent(&event);
	    }
	    break;
	default:
	  /* Handle all other event types the normal way */
	    XtDispatchEvent(&event);
	}
    }
}

Widget createDisplayMenu(Widget parent) {
    return buildMenu(parent,XmMENU_POPUP,
      "displayMenu",'\0',displayMenu);
}

static void createMain()
{

    XmString buttons[N_MAX_MENU_ELES];
    KeySym keySyms[N_MAX_MENU_ELES];
    String accelerators[N_MAX_MENU_ELES];
    XmString acceleratorText[N_MAX_MENU_ELES];
    XmString label;
    XmButtonType buttonType[N_MAX_MENU_ELES];
    Widget mainMB, mainBB, frame, frameLabel;
    char name[12];
    int n;
    Arg args[20];

  /* create a main window child of the main shell */
    n = 0;
    mainMW = XmCreateMainWindow(mainShell,"mainMW",args,n);

  /* get default fg/bg colors from mainMW for later use */
    n = 0;
    XtSetArg(args[n],XmNbackground,&defaultBackground); n++;
    XtSetArg(args[n],XmNforeground,&defaultForeground); n++;
    XtGetValues(mainMW,args,n);

  /*
   * create the menu bar
   */
    mainMB = XmCreateMenuBar(mainMW,"mainMB",NULL,0);

  /* color mainMB properly (force so VUE doesn't interfere) */
    colorMenuBar(mainMB,defaultForeground,defaultBackground);

  /*
   * create the file pulldown menu pane
   */

    mainFilePDM = buildMenu(mainMB,XmMENU_PULLDOWN,
      "File", 'F', fileMenu);

    if (globalDisplayListTraversalMode == DL_EXECUTE) {
	XtSetSensitive(fileMenu[FILE_NEW_BTN].widget,False);
	XtSetSensitive(fileMenu[FILE_SAVE_BTN].widget,False);
	XtSetSensitive(fileMenu[FILE_SAVE_AS_BTN].widget,False);
    }

  /*
   * create the edit pulldown menu pane
   */
    mainEditPDM = buildMenu(mainMB,XmMENU_PULLDOWN,
      "Edit", 'E', editMenu);

    if (globalDisplayListTraversalMode == DL_EXECUTE)
      XtSetSensitive(mainEditPDM,False);

  /*
   * create the view pulldown menu pane
   */
    mainViewPDM = buildMenu(mainMB,XmMENU_PULLDOWN,
      "View", 'V', viewMenu);

  /*
   * create the palettes pulldown menu pane
   */
    mainPalettesPDM = buildMenu(mainMB,XmMENU_PULLDOWN,
      "Palettes", 'P', palettesMenu);

    if (globalDisplayListTraversalMode == DL_EXECUTE)
      XtSetSensitive(mainPalettesPDM,False);

  /*
   * create the help pulldown menu pane
   */
    mainHelpPDM = buildMenu(mainMB,XmMENU_PULLDOWN,
      "Help", 'H', helpMenu);

    XtVaSetValues(mainMB, XmNmenuHelpWidget, mainHelpPDM, NULL);

  /* don't enable other help functions yet */
/*     XtSetSensitive(helpMenu[HELP_OVERVIEW_BTN].widget,False); */
/*     XtSetSensitive(helpMenu[HELP_CONTENTS_BTN].widget,False); */
/*     XtSetSensitive(helpMenu[HELP_OBJECTS_BTN].widget,False); */
/*     XtSetSensitive(helpMenu[HELP_EDIT_BTN].widget,False); */
/*     XtSetSensitive(helpMenu[HELP_NEW_BTN].widget,False); */
/*     XtSetSensitive(helpMenu[HELP_ON_HELP_BTN].widget,False); */

    n = 0;
    XtSetArg(args[n],XmNmarginHeight,9); n++;
    XtSetArg(args[n],XmNmarginWidth,18); n++;
    mainBB = XmCreateBulletinBoard(mainMW,"mainBB",args,n);
    XtAddCallback(mainBB,XmNhelpCallback,
      globalHelpCallback,(XtPointer)HELP_MAIN);

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
	  modeCallback, (XtPointer)DL_EDIT);
	XmStringFree(label);
	label = XmStringCreateSimple("Execute");
	n = 0;
	XtSetArg(args[n],XmNlabelString,label); n++;
	modeExecTB = XmCreateToggleButton(modeRB,"modeExecTB",args,n);
	XtAddCallback(modeExecTB,XmNvalueChangedCallback,
	  modeCallback,(XtPointer)DL_EXECUTE);
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
#if 0
  /*
   * create the Save As... prompt dialog
   */
    {
	n = 0;
	XtSetArg(args[n],XmNdefaultPosition,False); n++;
#if 0
	XtSetArg(args[n],XmNdirMask,gifDirMask); n++;
#endif
	XtSetArg(args[n],XmNdialogStyle,XmDIALOG_PRIMARY_APPLICATION_MODAL); n++;
	saveAsPD = XmCreateFileSelectionDialog(XtParent(mainFilePDM),
	  "saveAsPD",args,n);
	XtUnmanageChild(XmFileSelectionBoxGetChild(saveAsPD,XmDIALOG_HELP_BUTTON));
	XtAddCallback(saveAsPD,XmNcancelCallback,
	  fileMenuDialogCallback,(XtPointer)FILE_SAVE_AS_BTN);
	XtAddCallback(saveAsPD,XmNokCallback,fileMenuDialogCallback,
	  (XtPointer)FILE_SAVE_AS_BTN);
	XtAddCallback(saveAsPD,XmNmapCallback,mapCallback,(XtPointer)NULL);
	{
	    XmString buttons[NUM_IMAGE_TYPES-1];
	    XmButtonType buttonType[NUM_IMAGE_TYPES-1];
	    Widget radioBox, rowColumn, frame, typeLabel;
	    int i, n;
	    Arg args[10];

	    n = 0;
	    XtSetArg(args[n],XmNorientation,XmHORIZONTAL); n++;
	    rowColumn = XmCreateRowColumn(saveAsPD,"rowColumn",args,n);
	    n = 0;
	    typeLabel = XmCreateLabel(rowColumn,"File Format",args,n);
 
	    buttons[0] = XmStringCreateSimple("2.2.x");
	    buttons[1] = XmStringCreateSimple("2.1.x");
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
	    for (i = 0; i < 2; i++) XmStringFree(buttons[i]);
	}
    }
#endif
    
#ifdef PROMPT_TO_EXIT
  /*
   * create the Exit... warning dialog
   */

    exitQD = XmCreateQuestionDialog(XtParent(mainFilePDM),"exitQD",NULL,0);
    XtVaSetValues(XtParent(exitQD),XmNmwmDecorations, MWM_DECOR_ALL|MWM_DECOR_RESIZEH, NULL);
    XtUnmanageChild(XmMessageBoxGetChild(exitQD,XmDIALOG_HELP_BUTTON));
    XtAddCallback(exitQD,XmNcancelCallback,
      fileMenuDialogCallback,(XtPointer)FILE_EXIT_BTN);
    XtAddCallback(exitQD,XmNokCallback,fileMenuDialogCallback,
      (XtPointer)FILE_EXIT_BTN);
#endif
    
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

  /*
   * create the EditHelp information shell
   */
    n = 0;
    XtSetArg(args[n],XtNiconName,"EditHelp"); n++;
    XtSetArg(args[n],XtNtitle,"Edit Help"); n++;
    XtSetArg(args[n],XtNallowShellResize,TRUE); n++;
    XtSetArg(args[n],XmNkeyboardFocusPolicy,XmEXPLICIT); n++;
  /* map window manager menu Close function to application close... */
    XtSetArg(args[n],XmNdeleteResponse,XmDO_NOTHING); n++;
    XtSetArg(args[n],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH); n++;

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

  /*
   * Initialize the PV Info shell
   */
    pvInfoS = (Widget)0;

  /*
   * and realize the toplevel shell widget
   */
    XtRealizeWidget(mainShell);

}

#ifdef __cplusplus
Boolean medmInitWorkProc(XtPointer)
#else
Boolean medmInitWorkProc(XtPointer cd)
#endif
{
    int i;
    for (i=0; i<LAST_INIT_C; i++) {
	if (medmInitTask[i].init == False) {
	    medmInitTask[i].init = medmInitTask[i].initTask();
	    return False;
	}
    }
    return True;
}

void enableEditFunctions() {
    if (objectS)   XtSetSensitive(objectS,True);
    if (resourceS) XtSetSensitive(resourceS,True);
    if (colorS)    XtSetSensitive(colorS,True);
    if (channelS)  XtSetSensitive(channelS,True);
    XtSetSensitive(mainEditPDM,True);
    XtSetSensitive(mainPalettesPDM,True);
}

void disableEditFunctions() {
    if (objectS)   XtSetSensitive(objectS,False);
    if (resourceS) XtSetSensitive(resourceS,False);
    if (colorS)    XtSetSensitive(colorS,False);
    if (channelS)  XtSetSensitive(channelS,False);
    XtSetSensitive(mainEditPDM,False);
    XtSetSensitive(mainPalettesPDM,False);
}
