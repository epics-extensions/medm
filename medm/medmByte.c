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
 *     Original Author : David M. Wetherholt (CEBAF) & Mark Anderson
 *     Second Author   : Frederick Vong
 *     Third Author    : Kenneth Evans, Jr.
 *
 *****************************************************************************
*/

#include "medm.h"

/* Function Prototypes */

/* KE: Note that the following functions are really defined in xc/Byte.c as:
 * e.g. void XcBYUpdateByteForeground(ByteWidget w, unsigned long pixel);
 * but this is how they are being used and what avoids warnings.
 */
void XcBYUpdateByteForeground(Widget w, unsigned long pixel);
void XcBYUpdateValue(Widget w, XcVType *value);

static void byteUpdateValueCb(XtPointer cd);
static void byteDraw(XtPointer cd);
static void byteDestroyCb(XtPointer cd);
static void byteGetRecord(XtPointer, Record **, int *);
static void byteGetValues(ResourceBundle *pRCB, DlElement *p);
static void byteInheritValues(ResourceBundle *pRCB, DlElement *p);
static void byteSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void byteSetForegroundColor(ResourceBundle *pRCB, DlElement *p);

typedef struct _MedmByte {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           *record;
} MedmByte;

static DlDispatchTable byteDlDispatchTable = {
    createDlByte,
    NULL,
    executeDlByte,
    hideDlByte,
    writeDlByte,
    NULL,
    byteGetValues,
    byteInheritValues,
    byteSetBackgroundColor,
    byteSetForegroundColor,
    genericMove,
    genericScale,
    genericOrient,
    NULL,
    NULL};


void executeDlByte(DisplayInfo *displayInfo, DlElement *dlElement) {
/****************************************************************************
 * Execute DL Byte                                                          *
 ****************************************************************************/
    Arg args[30];
    int n;
    int usedHeight, usedCharWidth, bestSize, preferredHeight;
    Widget localWidget;
    MedmByte *pb;
    DlByte *dlByte = dlElement->structure.byte;

  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;
    
    if(!dlElement->widget) {
	if(displayInfo->traversalMode == DL_EXECUTE) {
	    if(dlElement->data) {
		pb = (MedmByte *)dlElement->data;
	    } else {
		pb = (MedmByte *)malloc(sizeof(MedmByte));
		dlElement->data = (void *)pb;
		pb->dlElement = dlElement;
		pb->updateTask = updateTaskAddTask(displayInfo,
		  &(dlByte->object),
		  byteDraw,
		  (XtPointer)pb);
		
		if(pb->updateTask == NULL) {
		    medmPrintf(1,"\nexecuteDlByte: Memory allocation error\n");
		} else {
		    updateTaskAddDestroyCb(pb->updateTask,byteDestroyCb);
		    updateTaskAddNameCb(pb->updateTask,byteGetRecord);
		}
		pb->record = medmAllocateRecord(dlByte->monitor.rdbk,
		  byteUpdateValueCb,
		  NULL,
		  (XtPointer) pb);
		drawWhiteRectangle(pb->updateTask);
	    }
	}

      /****** from the DlByte structure, we've got Byte's specifics */
	n = 0;
	XtSetArg(args[n],XtNx,(Position)dlByte->object.x); n++;
	XtSetArg(args[n],XtNy,(Position)dlByte->object.y); n++;
	XtSetArg(args[n],XtNwidth,(Dimension)dlByte->object.width); n++;
	XtSetArg(args[n],XtNheight,(Dimension)dlByte->object.height); n++;
	XtSetArg(args[n],XcNdataType,XcLval); n++;

      /****** note that this is orientation for the Byte */
	if(dlByte->direction == RIGHT) {
	    XtSetArg(args[n],XcNorient,XcHoriz); n++;
	} else if(dlByte->direction == UP) {
	    XtSetArg(args[n],XcNorient,XcVert); n++;
	} else if(dlByte->direction == LEFT) {
	  /* Override */
	    medmPrintf(1,"\nexecuteDlByte: "
	      "Direction=\"left\" is not supported for Byte\n");
	    XtSetArg(args[n],XcNorient,XcHoriz); n++;
	} else if(dlByte->direction == DOWN) {
	  /* Override */
	    medmPrintf(1,"\nexecuteDlByte: "
	      "Direction=\"down\" is not supported for Byte\n");
	    XtSetArg(args[n],XcNorient,XcVert); n++;
	}
	XtSetArg(args[n],XcNsBit,dlByte->sbit); n++;
	XtSetArg(args[n],XcNeBit,dlByte->ebit); n++;

      /****** Set arguments with other colors, etc */
	preferredHeight = dlByte->object.height/INDICATOR_FONT_DIVISOR;
	bestSize = dmGetBestFontWithInfo(fontTable,MAX_FONTS,NULL,
	  preferredHeight,0,&usedHeight,&usedCharWidth,FALSE);
	XtSetArg(args[n],XtNfont,fontTable[bestSize]); n++;
	XtSetArg(args[n],XcNbyteForeground,(Pixel)
	  displayInfo->colormap[dlByte->monitor.clr]); n++;
	  XtSetArg(args[n],XcNbyteBackground,(Pixel)
	    displayInfo->colormap[dlByte->monitor.bclr]); n++;
	    XtSetArg(args[n],XtNbackground,(Pixel)
	      displayInfo->colormap[dlByte->monitor.bclr]); n++;
	      XtSetArg(args[n],XcNcontrolBackground,(Pixel)
		displayInfo->colormap[dlByte->monitor.bclr]); n++;

	      /****** Add the pointer to the ChannelAccessMonitorData structure as
		userData to widget */
		XtSetArg(args[n],XcNuserData,(XtPointer)pb); n++;
		localWidget = XtCreateWidget("byte",
		  xcByteWidgetClass, displayInfo->drawingArea, args, n);
		dlElement->widget = localWidget;

	      /****** Record the widget that this structure belongs to */
		if(displayInfo->traversalMode == DL_EDIT) {
		    addCommonHandlers(localWidget, displayInfo);
		    XtManageChild(localWidget);
		}
    } else {
	DlObject *po = &(dlElement->structure.byte->object);
	XtVaSetValues(dlElement->widget,
	  XmNx, (Position) po->x,
	  XmNy, (Position) po->y,
	  XmNwidth, (Dimension) po->width,
	  XmNheight, (Dimension) po->height,
	  NULL);
    }
}

void hideDlByte(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element with a widget */
    hideWidgetElement(displayInfo, dlElement);
}

static void byteUpdateValueCb(XtPointer cd) {
    MedmByte *pb = (MedmByte *) ((Record *) cd)->clientData;
    updateTaskMarkUpdate(pb->updateTask);
}

static void byteDraw(XtPointer cd) {
    MedmByte *pb = (MedmByte *) cd;
    Record *pr = pb->record;
    DlElement *dlElement = pb->dlElement;
    Widget widget = dlElement->widget;
    DlByte *dlByte = dlElement->structure.byte;
    XcVType val;

  /* Check if hidden */
    if(dlElement->hidden) {
	if(widget && XtIsManaged(widget)) {
	    XtUnmanageChild(widget);
	}
	return;
    }
    
    
    if(pr && pr->connected) {
	if(pr->readAccess) {
	    if(widget) {
		addCommonHandlers(widget, pb->updateTask->displayInfo);
		XtManageChild(widget);
	    } else {
		return;
	    }
	    val.fval = (float) pr->value;
	    XcBYUpdateValue(widget,&val);
	    switch (dlByte->clrmod) {
	    case STATIC :
	    case DISCRETE :
		break;
	    case ALARM :
		XcBYUpdateByteForeground(widget,alarmColor(pr->severity));
		break;
	    }
	} else {
	    if(widget) XtUnmanageChild(widget);
	    draw3DPane(pb->updateTask,
	      pb->updateTask->displayInfo->colormap[dlByte->monitor.bclr]);
	    draw3DQuestionMark(pb->updateTask);
	}
    } else {
	if((widget) && XtIsManaged(widget))
	  XtUnmanageChild(widget);
	drawWhiteRectangle(pb->updateTask);
    }
}

static void byteDestroyCb(XtPointer cd) {
    MedmByte *pb = (MedmByte *) cd;
    if(pb) {
	medmDestroyRecord(pb->record);
	pb->dlElement->data = 0;
	free((char *)pb);
    }
}

static void byteGetRecord(XtPointer cd, Record **record, int *count) {
    MedmByte *pb = (MedmByte *) cd;
    *count = 1;
    record[0] = pb->record;
}

DlElement *createDlByte(DlElement *p) {
    DlByte *dlByte;
    DlElement *dlElement;

    dlByte = (DlByte *)malloc(sizeof(DlByte));
    if(!dlByte) return 0;
    if(p) {
	*dlByte = *p->structure.byte;
    } else {
	objectAttributeInit(&(dlByte->object));
	monitorAttributeInit(&(dlByte->monitor));
	dlByte->clrmod = STATIC;
	dlByte->direction = RIGHT;
	dlByte->sbit = 15;
	dlByte->ebit = 0;
    }

    if(!(dlElement = createDlElement(DL_Byte,
      (XtPointer)      dlByte,
      &byteDlDispatchTable))) {
	free(dlByte);
    }

    return(dlElement);
}

DlElement *parseByte( DisplayInfo *displayInfo) {
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlByte *dlByte;
    DlElement *dlElement = createDlByte(NULL);
 
    if(!dlElement) return 0;
    dlByte = dlElement->structure.byte;
 
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if(!strcmp(token,"object")) {
		parseObject(displayInfo,&(dlByte->object));
	    } else if(!strcmp(token,"monitor")) {
		parseMonitor(displayInfo,&(dlByte->monitor));
	    } else if(!strcmp(token,"clrmod")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"static"))       dlByte->clrmod = STATIC;
		else if(!strcmp(token,"alarm"))   dlByte->clrmod = ALARM;
		else if(!strcmp(token,"discrete"))dlByte->clrmod = DISCRETE;
	    } else if(!strcmp(token,"direction")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if(!strcmp(token,"up")) dlByte->direction = UP;
		else if(!strcmp(token,"right")) dlByte->direction = RIGHT;
		else if(!strcmp(token,"down")) dlByte->direction = DOWN;
		else if(!strcmp(token,"left")) dlByte->direction = LEFT;
	    } else if(!strcmp(token,"sbit")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlByte->sbit = atoi(token);
	    } else if(!strcmp(token,"ebit")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlByte->ebit = atoi(token);
	    }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
	}
    } while( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );
 
    return dlElement;
}

void writeDlByte(FILE *stream, DlElement *dlElement, int level) {
    int i;
    char indent[16];
    DlByte *dlByte = dlElement->structure.byte;

    for(i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%sbyte {",indent);
    writeDlObject(stream,&(dlByte->object),level+1);
    writeDlMonitor(stream,&(dlByte->monitor),level+1);
    if(dlByte->clrmod != STATIC)
      fprintf(stream,"\n%s\tclrmod=\"%s\"",indent,
        stringValueTable[dlByte->clrmod]);
    if(dlByte->direction != RIGHT)
      fprintf(stream,"\n%s\tdirection=\"%s\"",indent,
        stringValueTable[dlByte->direction]);
    if(dlByte->sbit != 15)
      fprintf(stream,"\n%s\tsbit=%d",indent,dlByte->sbit);
    if(dlByte->ebit != 0)
      fprintf(stream,"\n%s\tebit=%d",indent,dlByte->ebit);
    fprintf(stream,"\n%s}",indent);
}

static void byteInheritValues(ResourceBundle *pRCB, DlElement *p) {
    DlByte *dlByte = p->structure.byte;
    medmGetValues(pRCB,
      RDBK_RC,       &(dlByte->monitor.rdbk),
      CLR_RC,        &(dlByte->monitor.clr),
      BCLR_RC,       &(dlByte->monitor.bclr),
      DIRECTION_RC,  &(dlByte->direction),
      CLRMOD_RC,     &(dlByte->clrmod),
      SBIT_RC,       &(dlByte->sbit),
      EBIT_RC,       &(dlByte->ebit),
      -1);
}

static void byteGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlByte *dlByte = p->structure.byte;
    medmGetValues(pRCB,
      X_RC,          &(dlByte->object.x),
      Y_RC,          &(dlByte->object.y),
      WIDTH_RC,      &(dlByte->object.width),
      HEIGHT_RC,     &(dlByte->object.height),
      RDBK_RC,       &(dlByte->monitor.rdbk),
      CLR_RC,        &(dlByte->monitor.clr),
      BCLR_RC,       &(dlByte->monitor.bclr),
      DIRECTION_RC,  &(dlByte->direction),
      CLRMOD_RC,     &(dlByte->clrmod),
      SBIT_RC,       &(dlByte->sbit),
      EBIT_RC,       &(dlByte->ebit),
      -1);
}

static void byteSetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlByte *dlByte = p->structure.byte;
    medmGetValues(pRCB,
      BCLR_RC,       &(dlByte->monitor.bclr),
      -1);
}

static void byteSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
    DlByte *dlByte = p->structure.byte;
    medmGetValues(pRCB,
      CLR_RC,        &(dlByte->monitor.clr),
      -1);
}
