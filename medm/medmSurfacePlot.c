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
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (708-252-2000).
*/
/*****************************************************************************
 *
 *     Original Author : Mark Andersion
 *     Current Author  : Frederick Vong
 *
 * Modification Log:
 * -----------------
 * .01  03-01-95        vong    2.0.0 release
 *
 *****************************************************************************
*/

#include "medm.h"

void executeDlSurfacePlot(DisplayInfo *displayInfo,
			DlSurfacePlot *dlSurfacePlot, Boolean dummy)
{
  Channel *pCh;
  int n;
  Arg args[12];
  Widget localWidget;


  displayInfo->useDynamicAttribute = FALSE;

  if (displayInfo->traversalMode == DL_EXECUTE) {
    pCh = allocateChannel(displayInfo);
    pCh->monitorType = DL_SurfacePlot;
    pCh->specifics = (XtPointer) dlSurfacePlot;
  }

/* put up white rectangle so that unconnected channels are obvious */
  XSetForeground(display,displayInfo->pixmapGC,WhitePixel(display,screenNum));
  XFillRectangle(display,displayInfo->drawingAreaPixmap,displayInfo->pixmapGC,
	dlSurfacePlot->object.x,dlSurfacePlot->object.y,
	dlSurfacePlot->object.width,dlSurfacePlot->object.height);
  XSetForeground(display,displayInfo->gc,WhitePixel(display,screenNum));
  XFillRectangle(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	dlSurfacePlot->object.x,dlSurfacePlot->object.y,
	dlSurfacePlot->object.width,dlSurfacePlot->object.height);


/* from the surfacePlot structure, we've got SurfacePlot's specifics */
  n = 0;
  XtSetArg(args[n],XmNx,(Position)dlSurfacePlot->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlSurfacePlot->object.y); n++;
  XtSetArg(args[n],XmNwidth,(Dimension)dlSurfacePlot->object.width); n++;
  XtSetArg(args[n],XmNheight,(Dimension)dlSurfacePlot->object.height); n++;
  XtSetArg(args[n],XmNhighlightThickness,0); n++;
  XtSetArg(args[n],XmNforeground,(Pixel)
	displayInfo->dlColormap[dlSurfacePlot->plotcom.clr]); n++;
  XtSetArg(args[n],XmNbackground,(Pixel)
	displayInfo->dlColormap[dlSurfacePlot->plotcom.bclr]); n++;
  XtSetArg(args[n],XmNtraversalOn,False); n++;
/*
 * add the pointer to the Channel structure as userData 
 *  to widget
 */
  XtSetArg(args[n],XmNuserData,(XtPointer)pCh); n++;
  localWidget = XtCreateWidget("surfacePlot", xmDrawingAreaWidgetClass, 
		displayInfo->drawingArea, args, n);
  displayInfo->child[displayInfo->childCount++] =  localWidget;

  if (displayInfo->traversalMode == DL_EXECUTE) {

/* record the widget that this structure belongs to */
    pCh->self = localWidget;

  } else if (displayInfo->traversalMode == DL_EDIT) {

/* add button press handlers */
  XtAddEventHandler(localWidget,
	ButtonPressMask,False,(XtEventHandler)handleButtonPress,
	(XtPointer)displayInfo);
    XtManageChild(localWidget);
  }
}



