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

#include "medm.h"
#include <Xm/MwmUtil.h>


#define BUBBLE_DELTAX 0
#define BUBBLE_DELTAY 3

/* Function prototypes */

static void makeBubbleHelpWidget(Widget parent, char *label);

/* Global variables */

static Widget bubbleHelpWidget = NULL;
static Widget bubbleHelpWidgetLabel = NULL;

/* Callback for bubble help events */
void handleBubbleHelp(Widget w, XtPointer clientData, XEvent *event,
  Boolean *ctd)
{
  /* Avoid compiler warning messages */
    ctd;
    
  /* Switch depending on type */
    switch (event->type) {
    case EnterNotify:
      /* Manage the bubble help widget*/
	makeBubbleHelpWidget(w,(char *)clientData);
	break;
    case LeaveNotify:
      /* Destroy the bubble help widget*/
	if(bubbleHelpWidget) {
	    XtDestroyWidget(bubbleHelpWidget);
	    bubbleHelpWidget=NULL;
	}
	break;
    }
}

/*
 * Procedure to make a bubble help widget
 *   Note that background, borderColor, and borderWidth are specified elsewhere
 */
static void makeBubbleHelpWidget(Widget w, char *label)
{
    Window child;
    Widget wparent=XtParent(w);
    XmString xmString;
    Position x,y;
    Dimension width,height;
    Arg args[20];
    int nargs;
    int xroot,yroot;

  /* Destroy any old one */
    if(bubbleHelpWidget) XtDestroyWidget(bubbleHelpWidget);
  /* Get location of widget relative to its parent */
    nargs=0;
    XtSetArg(args[nargs],XmNx,&x); nargs++;
    XtSetArg(args[nargs],XmNy,&y); nargs++;
    XtSetArg(args[nargs],XmNwidth,&width); nargs++;
    XtSetArg(args[nargs],XmNheight,&height); nargs++;
    XtGetValues(w,args,nargs);
  /* Translate the coordinates to the root window
   *  XmNx and XmNy are supposed to be relative to the parent, but they are not.
   *  They are relative to the root window */
    if(XTranslateCoordinates(XtDisplay(w),XtWindow(XtParent(w)),
      RootWindowOfScreen(XtScreen(w)),(int)x,(int)y,&xroot,&yroot,&child)) {
	x=xroot;
	y=yroot;
    }
  /* Create dialog shell */
    nargs=0;
#if OMIT_RESIZE_HANDLES
    XtSetArg(args[nargs],XmNmwmDecorations,0); nargs++;
    XtSetArg(args[nargs],XmNmwmFunctions,0); nargs++;
#endif
    XtSetArg(args[nargs],XmNx,x+width/2+BUBBLE_DELTAX); nargs++;
    XtSetArg(args[nargs],XmNy,y+height+BUBBLE_DELTAY); nargs++;
    bubbleHelpWidget=XmCreateDialogShell(wparent,"bubbleHelpD",args,nargs);
  /* Create label */
    xmString=XmStringCreateLocalized(label);
    nargs=0;
    XtSetArg(args[nargs],XmNlabelString,xmString); nargs++;
    bubbleHelpWidgetLabel=XmCreateLabel(bubbleHelpWidget,"bubbleHelpDL",
      args,nargs);
    XtManageChild(bubbleHelpWidgetLabel);
    XtManageChild(bubbleHelpWidget);
    XmStringFree(xmString);
  /* Get location of widget */
    nargs=0;
    XtSetArg(args[nargs],XmNx,&x); nargs++;
    XtSetArg(args[nargs],XmNy,&y); nargs++;
    XtSetArg(args[nargs],XmNwidth,&width); nargs++;
    XtSetArg(args[nargs],XmNheight,&height); nargs++;
    XtGetValues(bubbleHelpWidget,args,nargs);
}
