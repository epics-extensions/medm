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

/* from utils.c - get XOR GC */
extern GC xorGC;

/* for use in handlePoly*Create() functions */
static double okRadiansTable[24] = { 0.,
				     1.*M_PI/4., 1.*M_PI/4.,
				     2.*M_PI/4., 2.*M_PI/4.,
				     3.*M_PI/4., 3.*M_PI/4.,
				     4.*M_PI/4., 4.*M_PI/4.,
				     5.*M_PI/4., 5.*M_PI/4.,
				     6.*M_PI/4., 6.*M_PI/4.,
				     7.*M_PI/4., 7.*M_PI/4.,
				     0.};

typedef struct _Polygon {
    DlElement       *dlElement;
    Record           *record;
    UpdateTask       *updateTask;
} MedmPolygon;

static void polygonDraw(XtPointer cd);
static void polygonUpdateValueCb(XtPointer cd);
static void polygonDestroyCb(XtPointer cd);
static void polygonGetRecord(XtPointer, Record **, int *);
static void polygonGetValues(ResourceBundle *pRCB, DlElement *p);
static void polygonInheritValues(ResourceBundle *pRCB, DlElement *p);
static void polygonSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void polygonMove(DlElement *, int, int);
static void polygonScale(DlElement *, int, int);
static void polygonOrient(DlElement *dlElement, int type, int xCenter,
  int yCenter);
static void destroyDlPolygon(DlElement *);
static int handlePolygonVertexManipulation(DlElement *, int, int);

static DlDispatchTable polygonDlDispatchTable = {
    createDlPolygon,
    destroyDlPolygon,
    executeDlPolygon,
    writeDlPolygon,
    NULL,
    polygonGetValues,
    polygonInheritValues,
    NULL,
    polygonSetForegroundColor,
    polygonMove,
    polygonScale,
    polygonOrient,
    handlePolygonVertexManipulation,
    NULL};

static void drawPolygon(MedmPolygon *pp) {
    DisplayInfo *displayInfo = pp->updateTask->displayInfo;
    Widget widget = pp->updateTask->displayInfo->drawingArea;
    Display *display = XtDisplay(widget);
    DlPolygon *dlPolygon = pp->dlElement->structure.polygon;

    if (dlPolygon->attr.fill == F_SOLID) {
	XFillPolygon(display,XtWindow(widget),displayInfo->gc,
	  dlPolygon->points,dlPolygon->nPoints,Complex,CoordModeOrigin);
    } else
      if (dlPolygon->attr.fill == F_OUTLINE) {
	  XDrawLines(display,XtWindow(widget),displayInfo->gc,
	    dlPolygon->points,dlPolygon->nPoints,CoordModeOrigin);
      }
}

static void calculateTheBoundingBox(DlPolygon *dlPolygon) {
    int minX = INT_MAX, maxX = INT_MIN, minY = INT_MAX, maxY = INT_MIN;
    int i;
    for (i = 0; i < dlPolygon->nPoints; i++) {
	minX = MIN(minX,dlPolygon->points[i].x);
	maxX = MAX(maxX,dlPolygon->points[i].x);
	minY = MIN(minY,dlPolygon->points[i].y);
	maxY = MAX(maxY,dlPolygon->points[i].y);
    }
    if (dlPolygon->attr.fill == F_SOLID) {
	dlPolygon->object.x = minX;
	dlPolygon->object.y = minY;
	dlPolygon->object.width = maxX - minX;
	dlPolygon->object.height = maxY - minY;
    } else {            /* F_OUTLINE, therfore lineWidth is a factor */
	dlPolygon->object.x = minX - dlPolygon->attr.width/2;
	dlPolygon->object.y = minY - dlPolygon->attr.width/2;
	dlPolygon->object.width = maxX - minX + dlPolygon->attr.width;
	dlPolygon->object.height = maxY - minY + dlPolygon->attr.width;
    }
}

void executeDlPolygon(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlPolygon *dlPolygon = dlElement->structure.polygon;
    if (displayInfo->traversalMode == DL_EXECUTE  && *dlPolygon->dynAttr.chan) {

	MedmPolygon *pp;
	DlObject object;

	pp = (MedmPolygon *) malloc(sizeof(MedmPolygon));
	pp->dlElement = dlElement;
#if 1
	object = dlPolygon->object;
	object.width++;
	object.height++;
#endif
	pp->updateTask = updateTaskAddTask(displayInfo,
	  &object,
	  polygonDraw,
	  (XtPointer)pp);

	if (pp->updateTask == NULL) {
	    medmPrintf(1,"\npolygonCreateRunTimeInstance: Memory allocation error\n");
	} else {
	    updateTaskAddDestroyCb(pp->updateTask,polygonDestroyCb);
	    updateTaskAddNameCb(pp->updateTask,polygonGetRecord);
	    pp->updateTask->opaque = False;
	}
	pp->record = medmAllocateRecord(
	  dlPolygon->dynAttr.chan,
	  polygonUpdateValueCb,
	  NULL,
	  (XtPointer) pp);
#if 0
	drawWhiteRectangle(pp->updateTask);
#endif
#ifdef __COLOR_RULE_H__
	switch (dlPolygon->dynAttr.clr) {
	case STATIC:
	    pp->record->monitorValueChanged = False;
	    pp->record->monitorSeverityChanged = False;
	    break;
	case ALARM:
	    pp->record->monitorValueChanged = False;
	    break;
	case DISCRETE:
	    pp->record->monitorSeverityChanged = False;
	    break;
	}
#else
	pp->record->monitorValueChanged = False;
	if (dlPolygon->dynAttr.clr != ALARM ) {
	    pp->record->monitorSeverityChanged = False;
	}
#endif

	if (dlPolygon->dynAttr.vis == V_STATIC ) {
	    pp->record->monitorZeroAndNoneZeroTransition = False;
	}

    } else {
	executeDlBasicAttribute(displayInfo,&(dlPolygon->attr));
	if (dlPolygon->attr.fill == F_SOLID) {
	    XFillPolygon(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
	      dlPolygon->points,dlPolygon->nPoints,Complex,CoordModeOrigin);
	    XFillPolygon(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
	      dlPolygon->points,dlPolygon->nPoints,Complex,CoordModeOrigin);
	} else
	  if (dlPolygon->attr.fill == F_OUTLINE) {
	      XDrawLines(display,XtWindow(displayInfo->drawingArea),displayInfo->gc,
		dlPolygon->points,dlPolygon->nPoints,CoordModeOrigin);
	      XDrawLines(display,displayInfo->drawingAreaPixmap,displayInfo->gc,
		dlPolygon->points,dlPolygon->nPoints,CoordModeOrigin);
	  }
    }
}

static void polygonUpdateValueCb(XtPointer cd) {
    MedmPolygon *pp = (MedmPolygon *)((Record *) cd)->clientData;
    updateTaskMarkUpdate(pp->updateTask);
}

static void polygonDraw(XtPointer cd) {
    MedmPolygon *pp = (MedmPolygon *)cd;
    Record *pd = pp->record;
    DisplayInfo *displayInfo = pp->updateTask->displayInfo;
    XGCValues gcValues;
    unsigned long gcValueMask;
    Display *display = XtDisplay(pp->updateTask->displayInfo->drawingArea);
    DlPolygon *dlPolygon = pp->dlElement->structure.polygon;

    if (pd->connected) {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	switch (dlPolygon->dynAttr.clr) {
#ifdef __COLOR_RULE_H__
	case STATIC :
	    gcValues.foreground = displayInfo->colormap[dlPolygon->attr.clr];
	    break;
	case DISCRETE:
	    gcValues.foreground = extractColor(displayInfo,
	      pd->value,
	      dlPolygon->dynAttr.colorRule,
	      dlPolygon->attr.clr);
	    break;
#else
	case STATIC :
	case DISCRETE:
	    gcValues.foreground = displayInfo->colormap[dlPolygon->attr.clr];
	    break;
#endif
	case ALARM :
	    gcValues.foreground = alarmColorPixel[pd->severity];
	    break;
	default :
	    gcValues.foreground = displayInfo->colormap[dlPolygon->attr.clr];
	    break;
	}
	gcValues.line_width = dlPolygon->attr.width;
	gcValues.line_style = ((dlPolygon->attr.style == SOLID) ? LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

	switch (dlPolygon->dynAttr.vis) {
	case V_STATIC:
	    drawPolygon(pp);
	    break;
	case IF_NOT_ZERO:
	    if (pd->value != 0.0)
	      drawPolygon(pp);
	    break;
	case IF_ZERO:
	    if (pd->value == 0.0)
	      drawPolygon(pp);
	    break;
	default :
	    medmPrintf(1,"\npolygonUpdateValueCb: Unknown visibility\n");
	    break;
	}
	if (pd->readAccess) {
	    if (!pp->updateTask->overlapped && dlPolygon->dynAttr.vis == V_STATIC) {
		pp->updateTask->opaque = True;
	    }
	} else {
	    pp->updateTask->opaque = False;
	    draw3DQuestionMark(pp->updateTask);
	}
    } else {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	gcValues.foreground = WhitePixel(display,DefaultScreen(display));
	gcValues.line_width = dlPolygon->attr.width;
	gcValues.line_style = ((dlPolygon->attr.style == SOLID) ? LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
	drawPolygon(pp);
    }
}

static void polygonDestroyCb(XtPointer cd) {
    MedmPolygon *pp = (MedmPolygon *)cd;
    if (pp) {
	medmDestroyRecord(pp->record);
	free((char *)pp);
    }
    return;
}

static void polygonGetRecord(XtPointer cd, Record **record, int *count) {
    MedmPolygon *pp = (MedmPolygon *)cd;
    *count = 1;
    record[0] = pp->record;
}


DlElement *createDlPolygon(DlElement *p)
{
    DlPolygon *dlPolygon;
    DlElement *dlElement;
 
    dlPolygon = (DlPolygon *) malloc(sizeof(DlPolygon));
    if (!dlPolygon) return 0;
    if (p) {
	int i;
	*dlPolygon = *p->structure.polygon;
	dlPolygon->points = (XPoint *)malloc(dlPolygon->nPoints*sizeof(XPoint));
	for (i = 0; i < dlPolygon->nPoints; i++) {
	    dlPolygon->points[i] = p->structure.polygon->points[i];
	}
    } else {
	objectAttributeInit(&(dlPolygon->object));
	basicAttributeInit(&(dlPolygon->attr));
	dynamicAttributeInit(&(dlPolygon->dynAttr));
	dlPolygon->points = NULL;
	dlPolygon->nPoints = 0;
    }
 
    if (!(dlElement = createDlElement(DL_Polygon,
      (XtPointer)      dlPolygon,
      &polygonDlDispatchTable))) {
	free(dlPolygon);
    }

    return(dlElement);
}

void parsePolygonPoints(
  DisplayInfo *displayInfo,
  DlPolygon *dlPolygon)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel;
#define INITIAL_NUM_POINTS 16
    int pointsArraySize = INITIAL_NUM_POINTS;

/* initialize some data in structure */
    dlPolygon->nPoints = 0;
    dlPolygon->points = (XPoint *)malloc(pointsArraySize*sizeof(XPoint));

    nestingLevel = 0;
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"(")) {
		if (dlPolygon->nPoints >= pointsArraySize) {
		  /* reallocate the points array: enlarge by 4X, etc */
		    pointsArraySize *= 4;
		    dlPolygon->points = (XPoint *)realloc(
		      dlPolygon->points,
		      (pointsArraySize+1)*sizeof(XPoint));
		}
		getToken(displayInfo,token);
		dlPolygon->points[dlPolygon->nPoints].x = atoi(token);
		getToken(displayInfo,token);	/* separator	*/
		getToken(displayInfo,token);
		dlPolygon->points[dlPolygon->nPoints].y = atoi(token);
		getToken(displayInfo,token);	/*   ")"	*/
		dlPolygon->nPoints++;
	    }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
	}
    } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );

  /* ensure closure of the polygon... */
    if (dlPolygon->points[0].x != dlPolygon->points[dlPolygon->nPoints-1].x &&
      dlPolygon->points[0].y != dlPolygon->points[dlPolygon->nPoints-1].y) {
	if (dlPolygon->nPoints >= pointsArraySize) {
	    dlPolygon->points = (XPoint *)realloc(dlPolygon->points,
	      (dlPolygon->nPoints+2)*sizeof(XPoint));
	}
	dlPolygon->points[dlPolygon->nPoints].x = dlPolygon->points[0].x;
	dlPolygon->points[dlPolygon->nPoints].y = dlPolygon->points[0].y;
	dlPolygon->nPoints++;
    }

}

DlElement *parsePolygon(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlPolygon *dlPolygon;
    DlElement *dlElement = createDlPolygon(NULL);
    if (!dlElement) return 0;
    dlPolygon = dlElement->structure.polygon;
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlPolygon->object));
	    else
	      if (!strcmp(token,"basic attribute"))
		parseBasicAttribute(displayInfo,&(dlPolygon->attr));
	      else
		if (!strcmp(token,"dynamic attribute"))
		  parseDynamicAttribute(displayInfo,&(dlPolygon->dynAttr));
		else
		  if (!strcmp(token,"points"))
		    parsePolygonPoints(displayInfo,dlPolygon);
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
	}
    } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );

    return dlElement;
}

/*
 * function to write all points of polygon out
 */
void writeDlPolygonPoints(
  FILE *stream,
  DlPolygon *dlPolygon,
  int level)
{
    int i;
    char indent[16];

    memset(indent,'\t',level);
    indent[level] = '\0';

    fprintf(stream,"\n%spoints {",indent);
    for (i = 0; i < dlPolygon->nPoints; i++) {
	fprintf(stream,"\n%s\t(%d,%d)",indent,
	  dlPolygon->points[i].x,dlPolygon->points[i].y);
    }

    fprintf(stream,"\n%s}",indent);
}


void writeDlPolygon(
  FILE *stream,
  DlElement *dlElement,
  int level)
{
    char indent[16];
    DlPolygon *dlPolygon = dlElement->structure.polygon;

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
  	fprintf(stream,"\n%spolygon {",indent);
  	writeDlObject(stream,&(dlPolygon->object),level+1);
  	writeDlBasicAttribute(stream,&(dlPolygon->attr),level+1);
  	writeDlDynamicAttribute(stream,&(dlPolygon->dynAttr),level+1);
  	writeDlPolygonPoints(stream,dlPolygon,level+1);
  	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
  	writeDlBasicAttribute(stream,&(dlPolygon->attr),level);
  	writeDlDynamicAttribute(stream,&(dlPolygon->dynAttr),level);
  	fprintf(stream,"\n%spolygon {",indent);
  	writeDlObject(stream,&(dlPolygon->object),level+1);
  	writeDlPolygonPoints(stream,dlPolygon,level+1);
  	fprintf(stream,"\n%s}",indent);
    }
#endif
}

/*
 * Manipulate a polygon vertex
 */
static int handlePolygonVertexManipulation(DlElement *dlElement,int x0, int y0)
{
    XEvent event;
    Window window;
    int i;
    int x01, y01;
    DlPolygon *dlPolygon = dlElement->structure.polygon;
    int pointIndex = 0;

    int deltaX, deltaY, okIndex;
    double radians, okRadians, length;
    int foundVertex = False;

    window = XtWindow(currentDisplayInfo->drawingArea);
 
    for (i = 0; i < dlPolygon->nPoints; i++) {
	x01 = dlPolygon->points[i].x;
	y01 = dlPolygon->points[i].y;
#define TOR 6
	if ((x01 + TOR > x0) && (x01 - TOR < x0) &&
	  (y01 + TOR > y0) && (y01 - TOR < y0)) {
	    pointIndex = i;
	    foundVertex = True;
	    break;
	}
#undef TOR 
    }

    if (!foundVertex) return 0;

    XGrabPointer(display,window,FALSE,
      (unsigned int) (PointerMotionMask|ButtonMotionMask|ButtonReleaseMask),
      GrabModeAsync,GrabModeAsync,None,None,CurrentTime);
    XGrabServer(display);


  /* Loop until button is released */
    while (TRUE) {
	XtAppNextEvent(appContext,&event);
	switch(event.type) {
	case ButtonRelease:
	  /* Modify point and leave here */
	    if (event.xbutton.state & ShiftMask) {
	      /* Constrain to 45 degree increments */
		deltaX = event.xbutton.x - dlPolygon->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].x;
		deltaY = event.xbutton.y - dlPolygon->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].y;
		length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
		radians = atan2((double)(deltaY),(double)(deltaX));
	      /* Use positive radians */
		if (radians < 0.) radians = 2*M_PI + radians;
		okIndex = (int)((radians*8.0)/M_PI);
		okRadians = okRadiansTable[okIndex];
		x01 = (int) (cos(okRadians)*length) + dlPolygon->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].x;
		y01 = (int) (sin(okRadians)*length) + dlPolygon->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].y;
		dlPolygon->points[pointIndex].x = x01;
		dlPolygon->points[pointIndex].y = y01;
	    } else {
	      /* Unconstrained */
		dlPolygon->points[pointIndex].x = event.xbutton.x;
		dlPolygon->points[pointIndex].y = event.xbutton.y;
	    }

	  /* Also update 0 or nPoints-1 point to keep order of polygon same */
	    if (pointIndex == 0) {
		dlPolygon->points[dlPolygon->nPoints-1] = dlPolygon->points[0];
	    } else if (pointIndex == dlPolygon->nPoints-1) {
		dlPolygon->points[0] = dlPolygon->points[dlPolygon->nPoints-1];
	    }

	    XUngrabPointer(display,CurrentTime);
	    XUngrabServer(display);
	    XFlush(display);
            calculateTheBoundingBox(dlPolygon);
	  /* Update global resource bundle  - then do create out of it */
	    globalResourceBundle.x = dlPolygon->object.x;
	    globalResourceBundle.y = dlPolygon->object.y;
	    globalResourceBundle.width = dlPolygon->object.width;
	    globalResourceBundle.height = dlPolygon->object.height;

	    dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
	  /* Since dmTraverseNonWidgets... clears the window, redraw highlights */
	    highlightSelectedElements();
	    return 1;

	case MotionNotify:
	  /* Undraw old line segments */
	    if (pointIndex > 0)
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[pointIndex-1].x,
		dlPolygon->points[pointIndex-1].y, x01,y01);
	    else 
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[MAX(dlPolygon->nPoints-2,0)].x,
		dlPolygon->points[MAX(dlPolygon->nPoints-2,0)].y, x01,y01);

	    if (pointIndex < dlPolygon->nPoints-1)
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[pointIndex+1].x,
		dlPolygon->points[pointIndex+1].y, x01,y01);
	    else
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[0].x,dlPolygon->points[0].y, x01,y01);

	    if (event.xmotion.state & ShiftMask) {
	      /* Constrain redraw to 45 degree increments */
		deltaX =  event.xmotion.x - dlPolygon->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].x;
		deltaY = event.xmotion.y - dlPolygon->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].y;
		length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
		radians = atan2((double)(deltaY),(double)(deltaX));
	      /* Use positive radians */
		if (radians < 0.) radians = 2*M_PI + radians;
		okIndex = (int)((radians*8.0)/M_PI);
		okRadians = okRadiansTable[okIndex];
		x01 = (int) (cos(okRadians)*length) + dlPolygon->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].x;
		y01 = (int) (sin(okRadians)*length) + dlPolygon->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolygon->nPoints-1)].y;
	    } else {
	      /* Unconstrained */
		x01 = event.xmotion.x;
		y01 = event.xmotion.y;
	    }
	  /* Draw new line segments */
	    if (pointIndex > 0)
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[pointIndex-1].x,
		dlPolygon->points[pointIndex-1].y, x01,y01);
	    else 
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[MAX(dlPolygon->nPoints-2,0)].x,
		dlPolygon->points[MAX(dlPolygon->nPoints-2,0)].y, x01,y01);
	    if (pointIndex < dlPolygon->nPoints-1)
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[pointIndex+1].x,
		dlPolygon->points[pointIndex+1].y, x01,y01);
	    else
	      XDrawLine(display,window,xorGC,
		dlPolygon->points[0].x,dlPolygon->points[0].y, x01,y01);
	    break;

	default:
	    XtDispatchEvent(&event);
	    break;
	}
    }
}

DlElement *handlePolygonCreate(int x0, int y0)
{
    XEvent event, newEvent;
    int newEventType;
    Window window;
    DlPolygon *dlPolygon;
    DlElement *dlElement;
#define INITIAL_NUM_POINTS 16
    int pointsArraySize = INITIAL_NUM_POINTS;
    int i;
    int x01, y01;

    int deltaX, deltaY, okIndex;
    double radians, okRadians, length;

    window = XtWindow(currentDisplayInfo->drawingArea);
    if (!(dlElement = createDlPolygon(NULL))) return 0;
    dlPolygon = dlElement->structure.polygon;
    polygonInheritValues(&globalResourceBundle,dlElement);
    objectAttributeSet(&(dlPolygon->object),x0,y0,0,0);

  /* First click is first point... */
    dlPolygon->nPoints = 1;
    dlPolygon->points = (XPoint *)malloc((pointsArraySize+3)*sizeof(XPoint));
    dlPolygon->points[0].x = (short)x0;
    dlPolygon->points[0].y = (short)y0;
    x01 = x0; y01 = y0;

    XGrabPointer(display,window,FALSE,
      (unsigned int) (PointerMotionMask|ButtonMotionMask|ButtonPressMask|ButtonReleaseMask),
      GrabModeAsync,GrabModeAsync,None,None,CurrentTime);
    XGrabServer(display);

  /* Now loop until button is double-clicked */
    while (TRUE) {
	XtAppNextEvent(appContext,&event);
	switch(event.type) {
	case ButtonPress:
	    XWindowEvent(display,XtWindow(currentDisplayInfo->drawingArea),
	      ButtonPressMask|Button1MotionMask|PointerMotionMask,&newEvent);
	    newEventType = newEvent.type;
	    if (newEventType == ButtonPress) {
	      /* -> Double click... add last point and leave here */
		if (event.xbutton.state & ShiftMask) {
		  /* Constrain to 45 degree increments */
		    deltaX = event.xbutton.x -
		      dlPolygon->points[dlPolygon->nPoints-1].x;
		    deltaY = event.xbutton.y -
		      dlPolygon->points[dlPolygon->nPoints-1].y;
		    length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
		    radians = atan2((double)(deltaY),(double)(deltaX));
				/* use positive radians */
		    if (radians < 0.) radians = 2*M_PI + radians;
		    okIndex = (int)((radians*8.0)/M_PI);
		    okRadians = okRadiansTable[okIndex];
		    x01 = (int) (cos(okRadians)*length)
		      + dlPolygon->points[dlPolygon->nPoints-1].x;
		    y01 = (int) (sin(okRadians)*length)
		      + dlPolygon->points[dlPolygon->nPoints-1].y;
		    dlPolygon->nPoints++;
		    dlPolygon->points[dlPolygon->nPoints-1].x = x01;
		    dlPolygon->points[dlPolygon->nPoints-1].y = y01;
		} else {
		  /* Unconstrained */
		    dlPolygon->nPoints++;
		    dlPolygon->points[dlPolygon->nPoints-1].x = event.xbutton.x;
		    dlPolygon->points[dlPolygon->nPoints-1].y = event.xbutton.y;
		}
		XUngrabPointer(display,CurrentTime);
		XUngrabServer(display);
		XFlush(display);
	      /* To ensure closure, make sure last point = first point */
		if(!(dlPolygon->points[0].x ==
		  dlPolygon->points[dlPolygon->nPoints-1].x &&
		  dlPolygon->points[0].y ==
		  dlPolygon->points[dlPolygon->nPoints-1].y)) {
		    dlPolygon->points[dlPolygon->nPoints] = dlPolygon->points[0];
		    dlPolygon->nPoints++;
		}
		calculateTheBoundingBox(dlPolygon);
		XCopyArea(display,currentDisplayInfo->drawingAreaPixmap,
		  XtWindow(currentDisplayInfo->drawingArea),
		  currentDisplayInfo->pixmapGC,
		  dlPolygon->object.x, dlPolygon->object.y,
		  dlPolygon->object.width, dlPolygon->object.height,
		  dlPolygon->object.x, dlPolygon->object.y);
		XBell(display,50); XBell(display,50);
		return (dlElement);
	    } else {
	      /* Not last point: more points to add.. */
	      /* Undraw old line segments */
		XDrawLine(display,window,xorGC,
		  dlPolygon->points[dlPolygon->nPoints-1].x,
		  dlPolygon->points[dlPolygon->nPoints-1].y, x01, y01);
		if (dlPolygon->nPoints > 1)
		  XDrawLine(display,window,xorGC,dlPolygon->points[0].x,
		    dlPolygon->points[0].y, x01, y01);

	      /* New line segment added: update coordinates */
		if (dlPolygon->nPoints >= pointsArraySize) {
		  /* Reallocate the points array: enlarge by 4X, etc */
		    pointsArraySize *= 4;
		    dlPolygon->points = (XPoint *)realloc(dlPolygon->points,
		      (pointsArraySize+3)*sizeof(XPoint));
		}

		if (event.xbutton.state & ShiftMask) {
		  /* Constrain to 45 degree increments */
		    deltaX = event.xbutton.x - dlPolygon->points[dlPolygon->nPoints-1].x;
		    deltaY = event.xbutton.y - dlPolygon->points[dlPolygon->nPoints-1].y;
		    length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
		    radians = atan2((double)(deltaY),(double)(deltaX));
		  /* Use positive radians */
		    if (radians < 0.) radians = 2*M_PI + radians;
		    okIndex = (int)((radians*8.0)/M_PI);
		    okRadians = okRadiansTable[okIndex];
		    x01 = (int) (cos(okRadians)*length)
		      + dlPolygon->points[dlPolygon->nPoints-1].x;
		    y01 = (int) (sin(okRadians)*length)
		      + dlPolygon->points[dlPolygon->nPoints-1].y;
		    dlPolygon->nPoints++;
		    dlPolygon->points[dlPolygon->nPoints-1].x = x01;
		    dlPolygon->points[dlPolygon->nPoints-1].y = y01;
		} else {
		  /* Unconstrained */
		    dlPolygon->nPoints++;
		    dlPolygon->points[dlPolygon->nPoints-1].x = event.xbutton.x;
		    dlPolygon->points[dlPolygon->nPoints-1].y = event.xbutton.y;
		    x01 = event.xbutton.x; y01 = event.xbutton.y;
		}
	      /* Draw new line segments */
		XDrawLine(display,window,xorGC,
		  dlPolygon->points[dlPolygon->nPoints-2].x,
		  dlPolygon->points[dlPolygon->nPoints-2].y,
		  dlPolygon->points[dlPolygon->nPoints-1].x,
		  dlPolygon->points[dlPolygon->nPoints-1].y);
		if (dlPolygon->nPoints > 1) XDrawLine(display,window,xorGC,
		  dlPolygon->points[0].x,dlPolygon->points[0].y,x01,y01);
	    }
	    break;

	case MotionNotify:
	  /* Undraw old line segments */
	    XDrawLine(display,window,xorGC,
	      dlPolygon->points[dlPolygon->nPoints-1].x,
	      dlPolygon->points[dlPolygon->nPoints-1].y,
	      x01, y01);
	    if (dlPolygon->nPoints > 1) XDrawLine(display,window,xorGC,
	      dlPolygon->points[0].x,dlPolygon->points[0].y, x01, y01);

	    if (event.xmotion.state & ShiftMask) {
	      /* Constrain redraw to 45 degree increments */
		deltaX = event.xmotion.x -
		  dlPolygon->points[dlPolygon->nPoints-1].x;
		deltaY = event.xmotion.y -
		  dlPolygon->points[dlPolygon->nPoints-1].y;
		length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
		radians = atan2((double)(deltaY),(double)(deltaX));
	      /* Use positive radians */
		if (radians < 0.) radians = 2*M_PI + radians;
		okIndex = (int)((radians*8.0)/M_PI);
		okRadians = okRadiansTable[okIndex];
		x01 = (int) (cos(okRadians)*length)
		  + dlPolygon->points[dlPolygon->nPoints-1].x;
		y01 = (int) (sin(okRadians)*length)
		  + dlPolygon->points[dlPolygon->nPoints-1].y;
	    } else {
	      /* Unconstrained */
		x01 = event.xmotion.x;
		y01 = event.xmotion.y;
	    }
	  /* Draw new last line segment */
	    XDrawLine(display,window,xorGC,
	      dlPolygon->points[dlPolygon->nPoints-1].x,
	      dlPolygon->points[dlPolygon->nPoints-1].y,
	      x01,y01);
	    if (dlPolygon->nPoints > 1) XDrawLine(display,window,xorGC,
	      dlPolygon->points[0].x,dlPolygon->points[0].y, x01,y01);

	    break;

	default:
	    XtDispatchEvent(&event);
	    break;
	}
    }
}

void polygonMove(DlElement *dlElement, int xOffset, int yOffset) {
    int i;
    XPoint *pts;
    if (dlElement->type != DL_Polygon) return;
    dlElement->structure.polygon->object.x += xOffset;
    dlElement->structure.polygon->object.y += yOffset;
    pts = dlElement->structure.polygon->points;
    for (i = 0; i < dlElement->structure.polygon->nPoints; i++) {
	pts[i].x += xOffset;
	pts[i].y += yOffset;
    }
}

void polygonScale(DlElement *dlElement, int xOffset, int yOffset) {
    float sX, sY;
    int i;
    DlPolygon *dlPolygon;
    int width, height;

    if (dlElement->type != DL_Polygon) return;
    dlPolygon = dlElement->structure.polygon;

    width = MAX(1,((int)dlPolygon->object.width + xOffset));
    height = MAX(1,((int)dlPolygon->object.height + yOffset));
    sX = (float)((float)width/(float)(dlPolygon->object.width));
    sY = (float)((float)(height)/(float)(dlPolygon->object.height));
    for (i = 0; i < dlPolygon->nPoints; i++) {
	dlPolygon->points[i].x = (short) (dlPolygon->object.x +
	  sX*(dlPolygon->points[i].x - dlPolygon->object.x));
	dlPolygon->points[i].y = (short) (dlPolygon->object.y +
	  sY*(dlPolygon->points[i].y - dlPolygon->object.y));
    }
    dlPolygon->object.width = width;
    dlPolygon->object.height = height;
}

void polygonOrient(DlElement *dlElement, int type, int xCenter, int yCenter)
{
    int i;
    DlPolygon *dlPolygon =  dlElement->structure.polygon;
    XPoint *pts = dlPolygon->points;
    int nPoints = dlPolygon->nPoints;
    int x, y;

    for (i = 0; i < nPoints; i++) {
	switch(type) {
	case ORIENT_HORIZ:
	    x = 2*xCenter - pts[i].x;
	    pts[i].x = MAX(0,x);
	    break;
	case ORIENT_VERT:
	    y = 2*yCenter - pts[i].y;
	    pts[i].y = MAX(0,y);
	    break;
	case ORIENT_CW:
	    x = xCenter - pts[i].y + yCenter;
	    y = yCenter + pts[i].x - xCenter;
	    pts[i].x = MAX(0,x);
	    pts[i].y = MAX(0,y);
	    break;
	case ORIENT_CCW:
	    x = xCenter +pts[i].y - yCenter;
	    y = yCenter -pts[i].x + xCenter;
	    pts[i].x = MAX(0,x);
	    pts[i].y = MAX(0,y);
	    break;
	}
    }
    calculateTheBoundingBox(dlPolygon);
}

static void polygonInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlPolygon *dlPolygon = p->structure.polygon;
    medmGetValues(pRCB,
      CLR_RC,        &(dlPolygon->attr.clr),
      STYLE_RC,      &(dlPolygon->attr.style),
      FILL_RC,       &(dlPolygon->attr.fill),
      LINEWIDTH_RC,  &(dlPolygon->attr.width),
      CLRMOD_RC,     &(dlPolygon->dynAttr.clr),
      VIS_RC,        &(dlPolygon->dynAttr.vis),
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlPolygon->dynAttr.colorRule),
#endif
      CHAN_RC,       &(dlPolygon->dynAttr.chan),
      -1);
}

static void destroyDlPolygon(DlElement *dlElement) {
    free ((char *)dlElement->structure.polygon->points);
    free ((char *)dlElement->structure.polygon);
    free ((char *)dlElement);
}

static void polygonGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlPolygon *dlPolygon = p->structure.polygon;
    int x, y;
    unsigned int width, height;
    int xOffset, yOffset;
    medmGetValues(pRCB,
      X_RC,          &x,
      Y_RC,          &y,
      WIDTH_RC,      &width,
      HEIGHT_RC,     &height,
      CLR_RC,        &(dlPolygon->attr.clr),
      STYLE_RC,      &(dlPolygon->attr.style),
      FILL_RC,       &(dlPolygon->attr.fill),
      LINEWIDTH_RC,  &(dlPolygon->attr.width),
      CLRMOD_RC,     &(dlPolygon->dynAttr.clr),
      VIS_RC,        &(dlPolygon->dynAttr.vis),
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlPolygon->dynAttr.colorRule),
#endif
      CHAN_RC,       &(dlPolygon->dynAttr.chan),
      -1);
    xOffset = (int) width - (int) dlPolygon->object.width;
    yOffset = (int) height - (int) dlPolygon->object.height;
    if (xOffset || yOffset) {
	polygonScale(p,xOffset,yOffset);
    }
    xOffset = x - dlPolygon->object.x;
    yOffset = y - dlPolygon->object.y;
    if (xOffset || yOffset) {
	polygonMove(p,xOffset,yOffset);
    }
}

static void polygonSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlPolygon *dlPolygon = p->structure.polygon;
    medmGetValues(pRCB,
      CLR_RC,        &(dlPolygon->attr.clr),
      -1);
}
