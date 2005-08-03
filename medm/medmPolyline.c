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

#define INITIAL_NUM_POINTS 16

typedef struct _MedmPolyline {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           **records;
} MedmPolyline;

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

static void polylineUpdateValueCb(XtPointer cd);
static void polylineDraw(XtPointer cd);
static void polylineDestroyCb(XtPointer cd);
static void polylineGetRecord(XtPointer, Record **, int *);
static void polylineGetValues(ResourceBundle *pRCB, DlElement *p);
static void polylineInheritValues(ResourceBundle *pRCB, DlElement *p);
static void polylineSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void destroyDlPolyline(DisplayInfo *displayInfo, DlElement *pE);
static void polylineMove(DlElement *dlElement, int xOffset, int yOffset);
static void polylineScale(DlElement *dlElement, int xOffset, int yOffset);
static void polylineOrient(DlElement *dlElement, int type, int xCenter,
  int yCenter);
static int handlePolylineVertexManipulation(DlElement *, int, int);

static DlDispatchTable polylineDlDispatchTable = {
    createDlPolyline,
    destroyDlPolyline,
    executeDlPolyline,
    hideDlPolyline,
    writeDlPolyline,
    NULL,
    polylineGetValues,
    polylineInheritValues,
    NULL,
    polylineSetForegroundColor,
    polylineMove,
    polylineScale,
    polylineOrient,
    handlePolylineVertexManipulation,
    NULL};

static void calculateTheBoundingBox(DlPolyline* dlPolyline)
{
    int minX = INT_MAX, maxX = INT_MIN, minY = INT_MAX, maxY = INT_MIN;
    int i;
    for(i = 0; i < dlPolyline->nPoints; i++) {
	minX = MIN(minX,dlPolyline->points[i].x);
	maxX = MAX(maxX,dlPolyline->points[i].x);
	minY = MIN(minY,dlPolyline->points[i].y);
	maxY = MAX(maxY,dlPolyline->points[i].y);
    }
    dlPolyline->object.x = minX - dlPolyline->attr.width/2;
    dlPolyline->object.y = minY - dlPolyline->attr.width/2;
    dlPolyline->object.width = maxX - minX + dlPolyline->attr.width;
    dlPolyline->object.height = maxY - minY + dlPolyline->attr.width;
}

static void drawPolyline(MedmPolyline *pp)
{
    DisplayInfo *displayInfo = pp->updateTask->displayInfo;
    Widget widget = pp->updateTask->displayInfo->drawingArea;
    Display *display = XtDisplay(widget);
    DlPolyline *dlPolyline = pp->dlElement->structure.polyline;

    XDrawLines(display,displayInfo->updatePixmap,displayInfo->gc,
      dlPolyline->points,dlPolyline->nPoints,CoordModeOrigin);
}

void executeDlPolyline(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlPolyline *dlPolyline = dlElement->structure.polyline;

  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(dlPolyline->isFallingOrRisingLine) {
      /* convert the falling line and rising line into polyline format */
	if(dlPolyline->attr.width > 0) {
	    int width = dlPolyline->attr.width;
	    int halfWidth = width/2;
	    if(dlPolyline->points[1].y > dlPolyline->points[0].y) {
	      /* falling line */
		dlPolyline->points[0].x += halfWidth;
		dlPolyline->points[0].y += halfWidth;
		dlPolyline->points[1].x -= halfWidth;
		dlPolyline->points[1].y -= halfWidth;
	    } else
	      if(dlPolyline->points[1].y < dlPolyline->points[0].y) {
		/* rising line */
		  dlPolyline->points[0].x += halfWidth;
		  dlPolyline->points[0].y -= width;
		  dlPolyline->points[1].x -= width;
		  dlPolyline->points[1].y -= halfWidth;
	      }
	}
	dlPolyline->isFallingOrRisingLine = False;
    }
    if(displayInfo->traversalMode == DL_EXECUTE &&
      *dlPolyline->dynAttr.chan[0]) {
	MedmPolyline *pp;
	DlObject object;

	if(dlElement->data) {
	    pp = (MedmPolyline *)dlElement->data;
	} else {
	    pp = (MedmPolyline *)malloc(sizeof(MedmPolyline));
	    dlElement->updateType = DYNAMIC_GRAPHIC;
	    dlElement->data = (void *)pp;
	    if(pp == NULL) {
		medmPrintf(1,"\nexecuteDlPolyline: Memory allocation error\n");
		return;
	    }
	  /* Pre-initialize */
	    pp->updateTask = NULL;
	    pp->records = NULL;
	    pp->dlElement = dlElement;

#if 1
	    object = dlPolyline->object;
	    object.width++;
	    object.height++;
#endif
	    pp->updateTask = updateTaskAddTask(displayInfo,
	      &object,
	      polylineDraw,
	      (XtPointer)pp);

	    if(pp->updateTask == NULL) {
		medmPrintf(1,"\nexecuteDlPolyline: Memory allocation error\n");
	    } else {
		updateTaskAddDestroyCb(pp->updateTask,polylineDestroyCb);
		updateTaskAddNameCb(pp->updateTask,polylineGetRecord);
	    }
	    if(!isStaticDynamic(&dlPolyline->dynAttr, True)) {
		pp->records = medmAllocateDynamicRecords(&dlPolyline->dynAttr,
		  polylineUpdateValueCb, NULL, (XtPointer) pp);
		calcPostfix(&dlPolyline->dynAttr);
		setDynamicAttrMonitorFlags(&dlPolyline->dynAttr, pp->records);
	    }
	}
    } else {
      /* Static */
	Drawable drawable=updateInProgress?
	  displayInfo->updatePixmap:displayInfo->drawingAreaPixmap;

	dlElement->updateType = STATIC_GRAPHIC;
	executeDlBasicAttribute(displayInfo,&(dlPolyline->attr));
	XDrawLines(display,drawable,displayInfo->gc,
          dlPolyline->points,dlPolyline->nPoints,CoordModeOrigin);
    }
}

void hideDlPolyline(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element drawn on the display drawingArea */
    hideDrawnElement(displayInfo, dlElement);
}

static void polylineUpdateValueCb(XtPointer cd)
{
    MedmPolyline *pp = (MedmPolyline *)((Record *) cd)->clientData;
    updateTaskMarkUpdate(pp->updateTask);
}

static void polylineDraw(XtPointer cd)
{
    MedmPolyline *pp = (MedmPolyline *)cd;
    Record *pR = pp->records?pp->records[0]:NULL;
    DisplayInfo *displayInfo = pp->updateTask->displayInfo;
    XGCValues gcValues;
    unsigned long gcValueMask;
    Widget widget = pp->updateTask->displayInfo->drawingArea;
    Display *display = XtDisplay(widget);
    DlPolyline *dlPolyline = pp->dlElement->structure.polyline;

    if(isConnected(pp->records)) {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	switch (dlPolyline->dynAttr.clr) {
	case STATIC :
	case DISCRETE:
	    gcValues.foreground = displayInfo->colormap[dlPolyline->attr.clr];
	    break;
	case ALARM :
	    gcValues.foreground = alarmColor(pR->severity);
	    break;
	default :
	    gcValues.foreground = displayInfo->colormap[dlPolyline->attr.clr];
	    break;
	}
	gcValues.line_width = dlPolyline->attr.width;
	gcValues.line_style = ((dlPolyline->attr.style == SOLID) ?
	  LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);

      /* Draw depending on visibility */
	if(calcVisibility(&dlPolyline->dynAttr, pp->records))
	  drawPolyline(pp);
	if(!pR->readAccess) {
	    drawBlackRectangle(pp->updateTask);
	}
    } else if(isStaticDynamic(&dlPolyline->dynAttr, True)) {
      /* clr and vis are both static */
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	gcValues.foreground = displayInfo->colormap[dlPolyline->attr.clr];
	gcValues.line_width = dlPolyline->attr.width;
	gcValues.line_style = ((dlPolyline->attr.style == SOLID) ?
	  LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
	drawPolyline(pp);
    } else {
	gcValueMask = GCForeground|GCLineWidth|GCLineStyle;
	gcValues.foreground = WhitePixel(display,DefaultScreen(display));
	gcValues.line_width = dlPolyline->attr.width;
	gcValues.line_style = ((dlPolyline->attr.style == SOLID) ?
	  LineSolid : LineOnOffDash);
	XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
	drawPolyline(pp);
    }
}

static void polylineDestroyCb(XtPointer cd)
{
    MedmPolyline *pp = (MedmPolyline *)cd;

    if(pp) {
	Record **records = pp->records;

	if(records) {
	    int i;
	    for(i=0; i < MAX_CALC_RECORDS; i++) {
		if(records[i]) medmDestroyRecord(records[i]);
	    }
	    free((char *)records);
	}
	if(pp->dlElement) pp->dlElement->data = NULL;
	free((char *)pp);
    }
    return;
}

static void polylineGetRecord(XtPointer cd, Record **record, int *count)
{
    MedmPolyline *pp = (MedmPolyline *)cd;
    int i;

    *count = 0;
    if(pp && pp->records) {
	for(i=0; i < MAX_CALC_RECORDS; i++) {
	    if(pp->records[i]) {
		record[(*count)++] = pp->records[i];
	    }
	}
    }
}

DlElement *createDlPolyline(DlElement *p)
{
    DlPolyline *dlPolyline;
    DlElement *dlElement;


    dlPolyline = (DlPolyline *)malloc(sizeof(DlPolyline));
    if(!dlPolyline) return 0;
    if(p) {
	int i;
	*dlPolyline = *p->structure.polyline;
	dlPolyline->points = (XPoint *)malloc(dlPolyline->nPoints*sizeof(XPoint));
	for(i = 0; i < dlPolyline->nPoints; i++) {
	    dlPolyline->points[i] = p->structure.polyline->points[i];
	}
    } else {
	objectAttributeInit(&(dlPolyline->object));
	basicAttributeInit(&(dlPolyline->attr));
	dynamicAttributeInit(&(dlPolyline->dynAttr));
	dlPolyline->points = NULL;
	dlPolyline->nPoints = 0;
	dlPolyline->isFallingOrRisingLine = False;
    }

    if(!(dlElement = createDlElement(DL_Polyline,
      (XtPointer)      dlPolyline,
      &polylineDlDispatchTable))) {
	free(dlPolyline);
    }

    return(dlElement);
}

void parsePolylinePoints(DisplayInfo *displayInfo, DlPolyline *dlPolyline)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel;
    int pointsArraySize = INITIAL_NUM_POINTS;

/* initialize some data in structure */
    dlPolyline->nPoints = 0;
    dlPolyline->points = (XPoint *)malloc(pointsArraySize*sizeof(XPoint));

    nestingLevel = 0;
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"(")) {
		if(dlPolyline->nPoints >= pointsArraySize) {
		  /* reallocate the points array: enlarge by 4X, etc */
		    pointsArraySize *= 4;
		    dlPolyline->points = (XPoint *)realloc(
		      dlPolyline->points,
		      (pointsArraySize+1)*sizeof(XPoint));
		}
		getToken(displayInfo,token);
		dlPolyline->points[dlPolyline->nPoints].x = atoi(token);
		getToken(displayInfo,token);	/* separator	*/
		getToken(displayInfo,token);
		dlPolyline->points[dlPolyline->nPoints].y = atoi(token);
		getToken(displayInfo,token);	/*   ")"	*/
		dlPolyline->nPoints++;
	    }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++;
	    break;
	case T_RIGHT_BRACE:
	    nestingLevel--;
	    break;
	default:
	    break;
	}
    } while( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );
}


DlElement *parsePolyline(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlPolyline *dlPolyline;
    DlElement *dlElement = createDlPolyline(NULL);
    if(!dlElement) return 0;
    dlPolyline = dlElement->structure.polyline;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlPolyline->object));
	    else if(!strcmp(token,"basic attribute"))
	      parseBasicAttribute(displayInfo,&(dlPolyline->attr));
	    else if(!strcmp(token,"dynamic attribute"))
	      parseDynamicAttribute(displayInfo,&(dlPolyline->dynAttr));
	    else if(!strcmp(token,"points"))
	      parsePolylinePoints(displayInfo,dlPolyline);
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++;
	    break;
	case T_RIGHT_BRACE:
	    nestingLevel--;
	    break;
	default:
	    break;
	}
    } while( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );

    return dlElement;
}


/*
 * function to write all points of polyline out
 */
void writeDlPolylinePoints(FILE *stream, DlPolyline *dlPolyline, int level)
{
    int i;
    char indent[16];

    memset(indent,'\t',level);
    indent[level] = '\0';

    fprintf(stream,"\n%spoints {",indent);

    for(i = 0; i < dlPolyline->nPoints; i++) {
	fprintf(stream,"\n%s\t(%d,%d)",indent,
	  dlPolyline->points[i].x,dlPolyline->points[i].y);
    }

    fprintf(stream,"\n%s}",indent);
}


void writeDlPolyline(FILE *stream, DlElement *dlElement, int level)
{
    char indent[16];
    DlPolyline *dlPolyline = dlElement->structure.polyline;

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if(MedmUseNewFileFormat) {
#endif
  	fprintf(stream,"\n%spolyline {",indent);
  	writeDlObject(stream,&(dlPolyline->object),level+1);
  	writeDlBasicAttribute(stream,&(dlPolyline->attr),level+1);
  	writeDlDynamicAttribute(stream,&(dlPolyline->dynAttr),level+1);
  	writeDlPolylinePoints(stream,dlPolyline,level+1);
  	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
  	writeDlBasicAttribute(stream,&(dlPolyline->attr),level);
  	writeDlDynamicAttribute(stream,&(dlPolyline->dynAttr),level);
  	fprintf(stream,"\n%spolyline {",indent);
  	writeDlObject(stream,&(dlPolyline->object),level+1);
  	writeDlPolylinePoints(stream,dlPolyline,level+1);
  	fprintf(stream,"\n%s}",indent);
    }
#endif
}

/*
 * Manipulate a polyline vertex
 */
static int handlePolylineVertexManipulation(DlElement *dlElement, int x0, int y0)
{
    XEvent event;
    Window window;
    int i;
    int x01, y01;
    DlPolyline *dlPolyline = dlElement->structure.polyline;
    int pointIndex = 0;

    int deltaX, deltaY, okIndex;
    double radians, okRadians, length;
    int foundVertex = False;

    window = XtWindow(currentDisplayInfo->drawingArea);

    for(i = 0; i < dlPolyline->nPoints; i++) {
	x01 = dlPolyline->points[i].x;
	y01 = dlPolyline->points[i].y;
#define TOR 6
	if((x01 + TOR > x0) && (x01 - TOR < x0) &&
	  (y01 + TOR > y0) && (y01 - TOR < y0)) {
	    pointIndex = i;
	    foundVertex = True;
	    break;
	}
#undef TOR
    }

    if(!foundVertex) return 0;

    XGrabPointer(display,window,FALSE,
      (unsigned int)(PointerMotionMask|ButtonMotionMask|ButtonReleaseMask),
      GrabModeAsync,GrabModeAsync,None,None,CurrentTime);
    XGrabServer(display);

/* Loop until button is released */
    while(TRUE) {
	XtAppNextEvent(appContext,&event);
	switch(event.type) {
	case ButtonRelease:
	  /* Modify point and leave here */
	    if(event.xbutton.state & ShiftMask) {
	      /* Constrain to 45 degree increments */
		deltaX = event.xmotion.x - dlPolyline->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].x;
		deltaY = event.xmotion.y - dlPolyline->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].y;
		length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
		radians = atan2((double)(deltaY),(double)(deltaX));
	      /* Use positive radians */
		if(radians < 0.) radians = 2*M_PI + radians;
		okIndex = (int)((radians*8.0)/M_PI);
		okRadians = okRadiansTable[okIndex];
		x01 = (int)(cos(okRadians)*length) + dlPolyline->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].x;
		y01 = (int)(sin(okRadians)*length) + dlPolyline->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].y;
		dlPolyline->points[pointIndex].x = x01;
		dlPolyline->points[pointIndex].y = y01;
	    } else {
	      /* Unconstrained */
		dlPolyline->points[pointIndex].x = event.xbutton.x;
		dlPolyline->points[pointIndex].y = event.xbutton.y;
	    }
	    XUngrabPointer(display,CurrentTime);
	    XUngrabServer(display);
	    XFlush(display);
            calculateTheBoundingBox(dlPolyline);
	  /* Update global resource bundle  - then do create out of it */
	    globalResourceBundle.x = dlPolyline->object.x;
	    globalResourceBundle.y = dlPolyline->object.y;
	    globalResourceBundle.width = dlPolyline->object.width;
	    globalResourceBundle.height = dlPolyline->object.height;
	    dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
	  /* Since dmTraverseNonWidgets... clears the window, redraw highlights */
	    highlightSelectedElements();
	    return 1;

	case MotionNotify:
	  /* Undraw old line segments */
	    if(pointIndex > 0)
	      XDrawLine(display,window,xorGC,
		dlPolyline->points[pointIndex-1].x,
		dlPolyline->points[pointIndex-1].y, x01,y01);
	    if(pointIndex < dlPolyline->nPoints-1)
	      XDrawLine(display,window,xorGC,
		dlPolyline->points[pointIndex+1].x,
		dlPolyline->points[pointIndex+1].y, x01,y01);
	    if(event.xmotion.state & ShiftMask) {
	      /* Constrain redraw to 45 degree increments */
		deltaX = event.xmotion.x - dlPolyline->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].x;
		deltaY = event.xmotion.y - dlPolyline->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].y;
		length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
		radians = atan2((double)(deltaY),(double)(deltaX));
	      /* Use positive radians */
		if(radians < 0.) radians = 2*M_PI + radians;
		okIndex = (int)((radians*8.0)/M_PI);
		okRadians = okRadiansTable[okIndex];
		x01 = (int)(cos(okRadians)*length) + dlPolyline->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].x;
		y01 = (int)(sin(okRadians)*length) + dlPolyline->points[
		  (pointIndex > 0 ? pointIndex-1 : dlPolyline->nPoints-1)].y;
	    } else {
	      /* Unconstrained */
		x01 = event.xmotion.x;
		y01 = event.xmotion.y;
	    }
	  /* Draw new line segments */
	    if(pointIndex > 0)
	      XDrawLine(display,window,xorGC,
		dlPolyline->points[pointIndex-1].x,
		dlPolyline->points[pointIndex-1].y, x01,y01);
	    if(pointIndex < dlPolyline->nPoints-1)
	      XDrawLine(display,window,xorGC,
		dlPolyline->points[pointIndex+1].x,
		dlPolyline->points[pointIndex+1].y, x01,y01);
	    break;

	default:
	    XtDispatchEvent(&event);
	    break;
	}
    }
}

/*
 * Create a polyline - if Boolean simpleLine is True then want a simple
 *  (2 point) line, else create and add points to the polyline until
 *  the user enters a double click
 */

DlElement *handlePolylineCreate(int x0, int y0, Boolean simpleLine)
{
    XEvent event, newEvent;
    int newEventType;
    Window window;
    DlPolyline *dlPolyline;
    DlElement *element;
    int pointsArraySize = INITIAL_NUM_POINTS;
    int x01, y01;

    int deltaX, deltaY, okIndex;
    double radians, okRadians, length;

    window = XtWindow(currentDisplayInfo->drawingArea);
    element = createDlPolyline(NULL);
    if(!element) return 0;
    dlPolyline = element->structure.polyline;
    polylineInheritValues(&globalResourceBundle,element);
    objectAttributeSet(&(dlPolyline->object),x0,y0,0,0);

  /* First click is first point... */
    dlPolyline->nPoints = 1;
    if(simpleLine) {
	dlPolyline->points = (XPoint *)malloc(2*sizeof(XPoint));
    } else {
	dlPolyline->points = (XPoint *)malloc((pointsArraySize+1)*sizeof(XPoint));
    }
    dlPolyline->points[0].x = (short)x0;
    dlPolyline->points[0].y = (short)y0;
    x01 = x0; y01 = y0;

    XGrabPointer(display,window,FALSE,
      (unsigned int) (PointerMotionMask|ButtonMotionMask|ButtonPressMask|ButtonReleaseMask),
      GrabModeAsync,GrabModeAsync,None,None,CurrentTime);
    XGrabServer(display);


  /* Loop until button is double-clicked (or until 2 points if simpleLine) */
    while(TRUE) {
	XtAppNextEvent(appContext,&event);
	switch(event.type) {
	case ButtonPress:
	    if(!simpleLine) {
	      /* need double click to end */
		XWindowEvent(display,XtWindow(currentDisplayInfo->drawingArea),
		  ButtonPressMask|Button1MotionMask|PointerMotionMask,&newEvent);
		newEventType = newEvent.type;
	    } else {
	      /* Just need second point, set type so we can terminate wi/ 2 points */
		newEventType = ButtonPress;
	    }
	    if(newEventType == ButtonPress) {
	      /* -> Double click... add last point and leave here */
		if(event.xbutton.state & ShiftMask) {
		  /* Constrain to 45 degree increments */
		    deltaX = event.xbutton.x -
		      dlPolyline->points[dlPolyline->nPoints-1].x;
		    deltaY = event.xbutton.y -
		      dlPolyline->points[dlPolyline->nPoints-1].y;
		    length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
		    radians = atan2((double)(deltaY),(double)(deltaX));
		  /* Use positive radians */
		    if(radians < 0.) radians = 2*M_PI + radians;
		    okIndex = (int)((radians*8.0)/M_PI);
		    okRadians = okRadiansTable[okIndex];
		    x01 = (int)(cos(okRadians)*length) +
		      dlPolyline->points[dlPolyline->nPoints-1].x;
		    y01 = (int)(sin(okRadians)*length) +
		      dlPolyline->points[dlPolyline->nPoints-1].y;
		    dlPolyline->nPoints++;
		    dlPolyline->points[dlPolyline->nPoints-1].x = x01;
		    dlPolyline->points[dlPolyline->nPoints-1].y = y01;
		} else {
		  /* Unconstrained */
		    dlPolyline->nPoints++;
		    dlPolyline->points[dlPolyline->nPoints-1].x = event.xbutton.x;
		    dlPolyline->points[dlPolyline->nPoints-1].y = event.xbutton.y;
		}
		XUngrabPointer(display,CurrentTime);
		XUngrabServer(display);
		XFlush(display);
		calculateTheBoundingBox(dlPolyline);
		XBell(display,50); XBell(display,50);
		return (element);
	    } else {
	      /* Not last point: more points to add.. */
	      /* Undraw old last line segment */
		XDrawLine(display,window,xorGC,
		  dlPolyline->points[dlPolyline->nPoints-1].x,
		  dlPolyline->points[dlPolyline->nPoints-1].y,
		  x01, y01);
	      /* New line segment added: update coordinates */
		if(dlPolyline->nPoints >= pointsArraySize) {
		  /* reallocate the points array: enlarge by 4X, etc */
		    pointsArraySize *= 4;
		    dlPolyline->points = (XPoint *)realloc(dlPolyline->points,
		      (pointsArraySize+1)*sizeof(XPoint));
		}
		if(event.xbutton.state & ShiftMask) {
		  /* Constrain to 45 degree increments */
		    deltaX = event.xbutton.x -
		      dlPolyline->points[dlPolyline->nPoints-1].x;
		    deltaY = event.xbutton.y -
		      dlPolyline->points[dlPolyline->nPoints-1].y;
		    length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
		    radians = atan2((double)(deltaY),(double)(deltaX));
		  /* Use positive radians */
		    if(radians < 0.) radians = 2*M_PI + radians;
		    okIndex = (int)((radians*8.0)/M_PI);
		    okRadians = okRadiansTable[okIndex];
		    x01 = (int)(cos(okRadians)*length) +
		      dlPolyline->points[dlPolyline->nPoints-1].x;
		    y01 = (int)(sin(okRadians)*length) +
		      dlPolyline->points[dlPolyline->nPoints-1].y;
		    dlPolyline->nPoints++;
		    dlPolyline->points[dlPolyline->nPoints-1].x = x01;
		    dlPolyline->points[dlPolyline->nPoints-1].y = y01;
		} else {
		  /* Unconstrained */
		    dlPolyline->nPoints++;
		    dlPolyline->points[dlPolyline->nPoints-1].x = event.xbutton.x;
		    dlPolyline->points[dlPolyline->nPoints-1].y = event.xbutton.y;
		    x01 = event.xbutton.x; y01 = event.xbutton.y;
		}
	      /* Draw new line segment */
		XDrawLine(display,window,xorGC,
		  dlPolyline->points[dlPolyline->nPoints-2].x,
		  dlPolyline->points[dlPolyline->nPoints-2].y,
		  dlPolyline->points[dlPolyline->nPoints-1].x,
		  dlPolyline->points[dlPolyline->nPoints-1].y);
	    }
	    break;

	case MotionNotify:
	  /* Undraw old last line segment */
	    XDrawLine(display,window,xorGC,
	      dlPolyline->points[dlPolyline->nPoints-1].x,
	      dlPolyline->points[dlPolyline->nPoints-1].y,
	      x01, y01);
	    if(event.xmotion.state & ShiftMask) {
	      /* Constrain redraw to 45 degree increments */
		deltaX = event.xmotion.x -
		  dlPolyline->points[dlPolyline->nPoints-1].x;
		deltaY = event.xmotion.y -
		  dlPolyline->points[dlPolyline->nPoints-1].y;
		length = sqrt(pow((double)deltaX,2.) + pow((double)deltaY,2.0));
		radians = atan2((double)(deltaY),(double)(deltaX));
	      /* Use positive radians */
		if(radians < 0.) radians = 2*M_PI + radians;
		okIndex = (int)((radians*8.0)/M_PI);
		okRadians = okRadiansTable[okIndex];
		x01 = (int)(cos(okRadians)*length)
		  + dlPolyline->points[dlPolyline->nPoints-1].x;
		y01 = (int)(sin(okRadians)*length)
		  + dlPolyline->points[dlPolyline->nPoints-1].y;
	    } else {
	      /* Unconstrained */
		x01 = event.xmotion.x;
		y01 = event.xmotion.y;
	    }
	  /* Draw new last line segment */
	    XDrawLine(display,window,xorGC,
	      dlPolyline->points[dlPolyline->nPoints-1].x,
	      dlPolyline->points[dlPolyline->nPoints-1].y,
	      x01,y01);
	    break;

	default:
	    XtDispatchEvent(&event);
	    break;
	}
    }
}

static void polylineMove(DlElement *dlElement, int xOffset, int yOffset)
{
    int i;
    XPoint *pts;
    if(dlElement->type != DL_Polyline) return;
    dlElement->structure.polyline->object.x += xOffset;
    dlElement->structure.polyline->object.y += yOffset;
    pts = dlElement->structure.polyline->points;
    for(i = 0; i < dlElement->structure.polyline->nPoints; i++) {
	pts[i].x += xOffset;
	pts[i].y += yOffset;
    }
}

static void polylineScale(DlElement *dlElement, int xOffset, int yOffset)
{
    float sX, sY;
    int i;
    DlPolyline *dlPolyline;
    int width, height;

    if(dlElement->type != DL_Polyline) return;
    dlPolyline = dlElement->structure.polyline;

    width = (int)dlPolyline->object.width + xOffset;
    height = (int)dlPolyline->object.height + yOffset;
    if(dlPolyline->object.width) {
	sX = (float)((float)width/(float)dlPolyline->object.width);
    } else {
	sX = (float)width;
    }
    if(dlPolyline->object.height) {
	sY = (float)((float)height/(float)dlPolyline->object.height);
    } else {
	sY = (float)height;
    }
    for(i = 0; i < dlPolyline->nPoints; i++) {
	dlPolyline->points[i].x = (short) (dlPolyline->object.x +
	  sX*(dlPolyline->points[i].x - dlPolyline->object.x));
	dlPolyline->points[i].y = (short) (dlPolyline->object.y +
	  sY*(dlPolyline->points[i].y - dlPolyline->object.y));
    }
    calculateTheBoundingBox(dlPolyline);
}

static void polylineOrient(DlElement *dlElement, int type, int xCenter,
  int yCenter)
{
    int i;
    DlPolyline *dlPolyline =  dlElement->structure.polyline;
    XPoint *pts = dlPolyline->points;
    int nPoints = dlPolyline->nPoints;
    int x, y;

    for(i = 0; i < nPoints; i++) {
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
    calculateTheBoundingBox(dlPolyline);
}

static void polylineInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlPolyline *dlPolyline = p->structure.polyline;
    medmGetValues(pRCB,
      CLR_RC,        &(dlPolyline->attr.clr),
      STYLE_RC,      &(dlPolyline->attr.style),
      FILL_RC,       &(dlPolyline->attr.fill),
      LINEWIDTH_RC,  &(dlPolyline->attr.width),
      CLRMOD_RC,     &(dlPolyline->dynAttr.clr),
      VIS_RC,        &(dlPolyline->dynAttr.vis),
      VIS_CALC_RC,   &(dlPolyline->dynAttr.calc),
      CHAN_A_RC,     &(dlPolyline->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlPolyline->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlPolyline->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlPolyline->dynAttr.chan[3]),
      -1);
}

static void destroyDlPolyline(DisplayInfo *displayInfo, DlElement *pE)
{
    free((char *)pE->structure.polygon->points);
    genericDestroy(displayInfo, pE);
}


static void polylineGetValues(ResourceBundle *pRCB, DlElement *p)
{
    DlPolyline *dlPolyline = p->structure.polyline;
    int x, y;
    unsigned int width, height;
    int xOffset, yOffset;

    medmGetValues(pRCB,
      X_RC,          &x,
      Y_RC,          &y,
      WIDTH_RC,      &width,
      HEIGHT_RC,     &height,
      CLR_RC,        &(dlPolyline->attr.clr),
      STYLE_RC,      &(dlPolyline->attr.style),
      FILL_RC,       &(dlPolyline->attr.fill),
      LINEWIDTH_RC,  &(dlPolyline->attr.width),
      CLRMOD_RC,     &(dlPolyline->dynAttr.clr),
      VIS_RC,        &(dlPolyline->dynAttr.vis),
      VIS_CALC_RC,   &(dlPolyline->dynAttr.calc),
      CHAN_A_RC,     &(dlPolyline->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlPolyline->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlPolyline->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlPolyline->dynAttr.chan[3]),
      -1);
    xOffset = (int)width - (int)dlPolyline->object.width;
    yOffset = (int)height - (int)dlPolyline->object.height;
    if(xOffset || yOffset) {
	polylineScale(p, xOffset, yOffset);
    }
    xOffset = x - dlPolyline->object.x;
    yOffset = y - dlPolyline->object.y;
    if(xOffset || yOffset) {
	polylineMove(p, xOffset, yOffset);
    }
    calculateTheBoundingBox(dlPolyline);
}

static void polylineSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlPolyline *dlPolyline = p->structure.polyline;
    medmGetValues(pRCB,
      CLR_RC,        &(dlPolyline->attr.clr),
      -1);
}
