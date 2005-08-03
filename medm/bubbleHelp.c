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
    UNREFERENCED(ctd);

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
    XtSetArg(args[nargs],XmNmwmDecorations,0); nargs++;
    XtSetArg(args[nargs],XmNmwmFunctions,0); nargs++;
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
