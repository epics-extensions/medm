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

/*
 *
 *  NOTE:  Xlib seems to do a pretty good job of only changing things and
 *		generating protocol when necessary.
 *	   Hence:  the XSetForeground()/XSetBackground()/XChangeGC()/...
 *		calls actually check for real change before generating
 *		protocol packets or more work.  Therefore, it's probably
 *		safe for me to rely on this mechanism, rather than
 *		doing my own checking.  Reason: somebody may change
 *		my GC behind my back and any internal buffering/checking
 *		in this reoutine may get out-of-sync wrt the current value.
 *		Performance penalty - cost of function calls instead of
 *		logical tests.  But this mechanism is guaranteed to be
 *		accurate.
 *
 *	   Alternative:  judicious use of XGetGCValues() with own checking
 *		in this routine, to avoid many function call penalties
 */

#include "medm.h"

/* optimized replacements for numeric sprintf()'s... */
#include "cvtFast.h"

void localCvtDoubleToExpNotationString(
  double value,
  char *textField,
  unsigned short precision)
{
    double absVal, newVal;
    Boolean minus;
    int exp, k, l;
    char TF[MAX_TEXT_UPDATE_WIDTH];
    
  /* The PREC field is a short, which can be negative, which will be a
   *   large number when converted to unsigned short.  This is handled
   *   in localCvtDoubleToString. */

    absVal = fabs(value);
    minus = (value < 0.0 ? True : False);
    newVal = absVal;
    
    if (absVal < 1.) {
      /* absVal < 1. */
	exp = 0;
	if (absVal != 0.) {     /* really ought to test against some epsilon */
	    do {
		newVal *= 1000.0;
		exp += 3;
	    } while (newVal < 1.);
	}
	localCvtDoubleToString(newVal,TF,precision);
	k = 0; l = 0;
	if (minus) textField[k++] = '-';
	while (TF[l] != '\0') textField[k++] = TF[l++];
	textField[k++] = 'e';
	if (exp == 0) {
	    textField[k++] = '+';     /* want e+00 for consistency with norms */
	} else {
	    textField[k++] = '-';
	}
	textField[k++] = '0' + exp/10;
	textField[k++] = '0' + exp%10;
	textField[k++] = '\0';
	
    } else {
	
      /* absVal >= 1. */
	exp = 0;
	while (newVal >= 1000.) {
	    newVal *= 0.001; /* since multiplying is usually faster than dividing */
	    exp += 3;
	}
	localCvtDoubleToString(newVal,TF,precision);
	k = 0; l = 0;
	if (minus) textField[k++] = '-';
	while (TF[l] != '\0') textField[k++] = TF[l++];
	textField[k++] = 'e';
	textField[k++] = '+';
	textField[k++] = '0' + exp/10;
	textField[k++] = '0' + exp%10;
	textField[k++] = '\0';
    }
}

void localCvtDoubleToString(
  double flt_value,
  char  *pstr_value,
  unsigned short precision)
{
  /* The PREC field is a short, which can be negative, which will be a
   *   large number when converted to unsigned short.  Handle this by
   *   not letting precision be larger than 17 (as for libCom:
   *   cvtDoubleToString()).  Letting it be zero here would be OK, but
   *   not in general, since precision is the number of significant
   *   digits for g and e formats. */
    if(precision > 17) precision=17;
    sprintf(pstr_value,"%.*f",(int)precision,flt_value);
}

void drawWhiteRectangle(UpdateTask *pt) {
    Display *display  = XtDisplay(pt->displayInfo->drawingArea);
    GC      gc        = pt->displayInfo->pixmapGC;
    Pixmap  pixmap    = pt->displayInfo->drawingAreaPixmap;
    Drawable drawable = XtWindow(pt->displayInfo->drawingArea);

    XSetForeground(display,gc,WhitePixel(display,DefaultScreen(display)));
    XFillRectangle(display,drawable,gc,pt->rectangle.x,pt->rectangle.y,
      pt->rectangle.width,pt->rectangle.height);
    return;
}

void draw3DPane(UpdateTask *pt, Pixel bgc) {

    Pixel   tsc, bsc, fgc, slc;
    DisplayInfo *displayInfo = pt->displayInfo;
    Display *display = XtDisplay(displayInfo->drawingArea);
    GC      gc        = displayInfo->gc;
    Pixmap  pixmap    = displayInfo->drawingAreaPixmap;
    Drawable drawable = XtWindow(displayInfo->drawingArea);
    int x = pt->rectangle.x;
    int y = pt->rectangle.y;
    int w = pt->rectangle.width;
    int h = pt->rectangle.height;

    XPoint  points[7];
    int n;
    int shadowThickness = 2;

#if 0
    XtVaGetValues(displayInfo->drawingArea,XmNshadowThickness,&shadowThickness,NULL);
#endif

    XmGetColors(XtScreen(displayInfo->drawingArea),cmap,bgc,&fgc,&tsc,&bsc,&slc);
  /* create the top shadow */
    n = 0;
    points[n].x = x;                       points[n].y = y;                       n++;
    points[n].x = x + w;                   points[n].y = y;                       n++;
    points[n].x = x + w - shadowThickness; points[n].y = y + shadowThickness;     n++;
    points[n].x = x + shadowThickness;     points[n].y = y + shadowThickness;     n++;
    points[n].x = x + shadowThickness;     points[n].y = y + h - shadowThickness; n++;
    points[n].x = x;                       points[n].y = y + h;                   n++;
    points[n].x = x;                       points[n].y = y;                       n++;
    XSetForeground(display,gc,tsc);
    XFillPolygon(display, drawable, gc, points, XtNumber(points),Nonconvex,CoordModeOrigin); 
  /* create the background pane */
    XSetForeground(display,gc,bgc);
    XFillRectangle(display, drawable, gc,
      x+shadowThickness,y+shadowThickness,
      w-2*shadowThickness,h-2*shadowThickness);
  /* create the bottom shadow */
    n = 0;
    points[n].x = x + shadowThickness;     points[n].y = y + h - shadowThickness; n++;
    points[n].x = x + w - shadowThickness; points[n].y = y + h - shadowThickness; n++;
    points[n].x = x + w - shadowThickness; points[n].y = y + shadowThickness;     n++;
    points[n].x = x + w;                   points[n].y = y;          n++;
    points[n].x = x + w;                   points[n].y = y + h;      n++;
    points[n].x = x;                       points[n].y = y + h;      n++;
    points[n].x = x + shadowThickness;     points[n].y = y + h - shadowThickness; n++;
    XSetForeground(display,gc,bsc);
    XFillPolygon(display, drawable, gc, points, XtNumber(points),Nonconvex,CoordModeOrigin); 
}

void draw3DQuestionMark(UpdateTask *pt) {
    Pixel   tsc, bsc, bgc, fgc, slc;
    Display *display  = XtDisplay(pt->displayInfo->drawingArea);
    GC      gc        = pt->displayInfo->pixmapGC;
    Pixmap  pixmap    = pt->displayInfo->drawingAreaPixmap;
    Drawable drawable = XtWindow(pt->displayInfo->drawingArea);
    int x = pt->rectangle.x;
    int y = pt->rectangle.y;
    int w = pt->rectangle.width;
    int h = pt->rectangle.height;
    Dimension qmh, qmw, qmlw;  /*  qmh = height of the drawing area for the question mark
				*  qmw = width of the drawing area for the question mark
				*  qmlw = line width of the question mark
				*/
    int ax, ay;                  
    unsigned int aw, ah;
    int lx1, lx2, ly1, ly2;
    int dx, dy;
    unsigned int dw, dh;
    char dotted[2] = {1,1};


    bgc = alarmColor(MAJOR_ALARM);
    XmGetColors(XtScreen(pt->displayInfo->drawingArea),cmap,bgc,&fgc,&tsc,&bsc,&slc);

    qmlw = (h > w) ? (w / 10) : (h / 10);  /* calculate the line width */
    if (qmlw != (qmlw/2*2)) qmlw = qmlw + 1;  /* if odd, make it even */
    qmh = h - qmlw * 4;
    if (qmh != (qmh/2*2)) qmh = qmh + 1;      /* if odd, make it even */
    qmw = w - qmlw * 4;
    if (qmw != (qmw/2*2)) qmw = qmw + 1;      /* if odd, make it even */
    if (qmh < qmw) {
	qmw = qmh;
    }
  /* calculate the size of the top arc of the question mark */
    ax = x + w / 2 - qmw / 2;
    ay = y + qmlw * 2;
    ah = qmh / 2;
    aw = qmw;
  /* calculate the size and position of the middle stroke */
    lx1 = x + w / 2;
    ly1 = y + h/ 2 - qmlw/2;
    lx2 = lx1;
    ly2 = ly1 + qmh / 4;
    if (lx1 > lx2) lx2 = lx1;
    if (ly1 > ly2) ly2 = ly1;
  /* calculate the size and position of the bottom circle */
    dx = lx1 - qmw / 8;
    dy = ly2 + qmlw; 
    dw = qmw / 4;
    dh = qmh / 4;
    if (dw <= 0) dw = 1;
    if (dh <= 0) dh = 1;
  /* draw the foreground of the question mark */
    XSetForeground(display, gc, bgc);
    XSetLineAttributes(display,gc, qmlw,LineSolid,CapButt,JoinMiter);
  /* draw mirror image */
    XDrawArc(display,drawable,gc, ax,ay,aw,ah,180*64,-270*64);
    XDrawLine(display,drawable, gc, lx1, ly1, lx2, ly2);
    XFillArc(display, drawable, gc, dx,dy,dw,dh,0,360*64);
  /* draw the top shadow */
    XSetDashes(display, gc, 0, dotted, 2);
    XSetForeground(display, gc, tsc);
    XSetLineAttributes(display,gc, qmlw/5,LineSolid,CapButt,JoinMiter);
/*  XSetLineAttributes(display,gc, qmlw/5,LineOnOffDash,CapButt,JoinMiter,FillSolid); */
  /* draw mirror image */
    XDrawArc(display,drawable,gc, ax-qmlw/2,ay-qmlw/2,aw+qmlw,ah+qmlw,180*64,-135*64);
    XDrawArc(display,drawable,gc, ax+qmlw/2,ay+qmlw/2,aw-qmlw,ah-qmlw,45*64,-128*64);
    XDrawLine(display,drawable, gc, lx1-qmlw/2, ly1, lx2-qmlw/2, ly2);
    XDrawLine(display,drawable, gc, lx1-qmlw/2, ly1, lx2+qmlw/2, ly1);
    XDrawArc(display, drawable, gc, dx,dy,dw,dh,45*64,180*64);
  /* draw the bottom shadow */
    XSetForeground(display, gc, bsc);
    XSetLineAttributes(display,gc, qmlw/5,LineSolid,CapButt,JoinMiter);
/*  XSetLineAttributes(display,gc, qmlw/5,LineOnOffDash,CapButt,JoinMiter,FillSolid);  */
  /* draw mirror image */
    XDrawArc(display,drawable,gc, ax-qmlw/2,ay-qmlw/2,aw+qmlw,ah+qmlw,45*64,-128*64);
    XDrawArc(display,drawable,gc, ax+qmlw/2,ay+qmlw/2,aw-qmlw,ah-qmlw,180*64,-135*64);
    XDrawLine(display,drawable, gc, ax-qmlw/2, ay+ah/2, ax+qmlw/2, ay+ah/2);
    XDrawLine(display,drawable, gc, lx1+qmlw/2, ly1+qmlw, lx2+qmlw/2, ly2);
    XDrawLine(display,drawable, gc, lx1-qmlw/2, ly2, lx2+qmlw/2, ly2);
    XDrawArc(display, drawable, gc, dx,dy,dw,dh,225*64,180*64);
}
