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

#define DEBUG_COMPOSITE 0
#define DEBUG_DELETE 0

#include "medm.h"

typedef struct _Rectangle {
    DlElement  *dlElement;     /* Must be first */
    Record     **records;
    UpdateTask *updateTask;
    DisplayInfo *displayInfo;
    Boolean    childrenExecuted;
} MedmComposite;

/* Function prototypes */

static void destroyDlComposite(DisplayInfo *displayInfo, DlElement *pE);
static void compositeMove(DlElement *element, int xOffset, int yOffset);
static void compositeScale(DlElement *element, int xOffset, int yOffset);
static void compositeOrient(DlElement *dlElement, int type, int xCenter,
  int yCenter);
static void compositeGetValues(ResourceBundle *pRCB, DlElement *p);
static void compositeCleanup(DlElement *element);
static void compositeUpdateGraphicalInfoCb(XtPointer cd);
static void compositeUpdateValueCb(XtPointer cd);
static void compositeUpdateValueCb(XtPointer cd);
static void compositeDraw(XtPointer cd);
static void compositeDestroyCb(XtPointer cd);
static void compositeGetRecord(XtPointer cd, Record **record, int *count);
static void executeCompositeChildren(DisplayInfo *displayInfo,
  DlComposite *dlComposite);
static void hideCompositeChildren(DisplayInfo *displayInfo,
  DlComposite *dlComposite);
static void drawComposite(MedmComposite *pc);
static void hideComposite(MedmComposite *pc);

static DlDispatchTable compositeDlDispatchTable = {
    createDlComposite,
    destroyDlComposite,
    executeDlComposite,
    hideDlComposite,
    writeDlComposite,
    NULL,
    compositeGetValues,
    NULL,
    NULL,
    NULL,
    compositeMove,
    compositeScale,
    compositeOrient,
    NULL,
    compositeCleanup};

void executeDlComposite(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlComposite *dlComposite = dlElement->structure.composite;

#if DEBUG_COMPOSITE
    print("executeDlComposite:\n");
#endif    

    if(displayInfo->traversalMode == DL_EXECUTE) {
      /* EXECUTE mode */
	if(*dlComposite->dynAttr.chan[0]) {
	  /* A channel is defined */
	    MedmComposite *pc;
	    
	  /* Allocate and fill in MedmComposite struct */
	    pc = (MedmComposite *)malloc(sizeof(MedmComposite));
	    pc->displayInfo = displayInfo;
	    pc->dlElement = dlElement;
	    pc->records = NULL;
	    pc->updateTask = updateTaskAddTask(displayInfo,
	      &(dlComposite->object), compositeDraw, (XtPointer)pc);
	    pc->childrenExecuted = False;
	    if (pc->updateTask == NULL) {
		medmPrintf(1,"\nexecuteDlComposite: Memory allocation error\n");
	    } else {
		updateTaskAddDestroyCb(pc->updateTask,compositeDestroyCb);
		updateTaskAddNameCb(pc->updateTask,compositeGetRecord);
		pc->updateTask->opaque = False;
	    }
	    pc->records = medmAllocateDynamicRecords(&dlComposite->dynAttr,
	      compositeUpdateValueCb,
	      compositeUpdateGraphicalInfoCb,
	      (XtPointer)pc);
#if DEBUG_COMPOSITE
	    print("  pc=%x\n",pc);
#endif    

	  /* Calculate the postfix for visbilitiy calc */
	    calcPostfix(&dlComposite->dynAttr);
	    setMonitorChanged(&dlComposite->dynAttr, pc->records);
#if 0	    
	  /* Draw initial white rectangle */
	  /* KE: Check if this is necessary */
	    drawWhiteRectangle(pc->updateTask);
#endif	    
	} else {
	  /* No channel */
	    executeCompositeChildren(displayInfo, dlComposite);
	}
    } else {
      /* EDIT mode */
	executeCompositeChildren(displayInfo, dlComposite);
    }
}

void hideDlComposite(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlComposite *dlComposite;

    if(!displayInfo || !dlElement) return;
    
  /* Delete any update tasks.  The destroyCb will free the
     record(s) and private structure */
    updateTaskDeleteElementTasks(displayInfo,dlElement);
    
  /* The same code as in hideComposite */
    dlComposite = dlElement->structure.composite;
    hideCompositeChildren(displayInfo, dlComposite);
}

static void compositeUpdateGraphicalInfoCb(XtPointer cd)
{
    Record *pr = (Record *)cd;
    MedmComposite *pc = (MedmComposite *)pr->clientData;

#if DEBUG_COMPOSITE
    print("compositeUpdateGraphicalInfoCb: record=%x[%s] value=%g\n",
      pr,pr->name,pr->value);
#endif    
#if 0    
    updateTaskMarkUpdate(pc->updateTask);
#endif    
}

static void compositeUpdateValueCb(XtPointer cd)
{
    Record *pr = (Record *)cd;
    MedmComposite *pc = (MedmComposite *)pr->clientData;

#if DEBUG_COMPOSITE
    print("compositeUpdateValueCb: record=%x[%s] value=%g\n",
      pr,pr->name,pr->value);
#endif    
    updateTaskMarkUpdate(pc->updateTask);
}

static void compositeDraw(XtPointer cd)
{
    MedmComposite *pc = (MedmComposite *)cd;
    Record *pr = pc->records?pc->records[0]:NULL;
    DisplayInfo *displayInfo = pc->updateTask->displayInfo;
    Display *display = XtDisplay(pc->updateTask->displayInfo->drawingArea);
    DlComposite *dlComposite = pc->dlElement->structure.composite;

#if DEBUG_COMPOSITE
    print("\ncompositeDraw: pc=%x\n",pc);
#endif    
#if DEBUG_DELETE
    print("compositeDraw: connected=%s readAccess=%s value=%g\n",
      pr->connected?"Yes":"No",pr->readAccess?"Yes":"No",pr->value);
#endif    
  /* Branch on whether there is a channel or not */
    if(*dlComposite->dynAttr.chan[0]) {
      /* A channel is defined */
	if(!pr) return;
	if (pr->connected) {
	    if (pr->readAccess) {
#if DEBUG_COMPOSITE
		print("  vis=%s\n",stringValueTable[dlComposite->dynAttr.vis]);
		print("  calc=%s\n",dlComposite->dynAttr.calc);
		if(*dlComposite->dynAttr.chan[0])
		  print("  chan[0]=%s val=%g\n",dlComposite->dynAttr.chan[0],
		    pc->records[0]->value);
		if(*dlComposite->dynAttr.chan[1])
		  print("  chan[1]=%s val=%g\n",dlComposite->dynAttr.chan[1],
		    pc->records[1]->value);
		if(*dlComposite->dynAttr.chan[2])
		  print("  chan[2]=%s val=%g\n",dlComposite->dynAttr.chan[2],
		    pc->records[2]->value);
		if(*dlComposite->dynAttr.chan[3])
		  print("  chan[3]=%s val=%g\n",dlComposite->dynAttr.chan[3],
		    pc->records[3]->value);
		print("  calcVisibility=%s\n",
		  calcVisibility(&dlComposite->dynAttr,
		    pc->records)?"True":"False");
#endif    
	      /* Draw depending on visibility */
		if(calcVisibility(&dlComposite->dynAttr, pc->records)) {
		    drawComposite(pc);
		} else {
		    hideComposite(pc);
		}
		if(pr->readAccess) {
		    if (!pc->updateTask->overlapped &&
		      dlComposite->dynAttr.vis == V_STATIC) {
			pc->updateTask->opaque = True;
		    }
		} else {
		    pc->updateTask->opaque = False;
		    draw3DQuestionMark(pc->updateTask);
		}
	    } else {
		hideComposite(pc);
		pc->updateTask->opaque = False;
		drawWhiteRectangle(pc->updateTask);
	    }
	}
    } else {
      /* No channel */
	executeCompositeChildren(displayInfo, dlComposite);
    }
    
  /* Update the drawing objects above */
    redrawElementsAbove(displayInfo, (DlElement *)dlComposite);
}

static void drawComposite(MedmComposite *pc)
{
    DisplayInfo *displayInfo = pc->updateTask->displayInfo;
    DlComposite *dlComposite = pc->dlElement->structure.composite;
    
    if(!pc->childrenExecuted) {
	executeCompositeChildren(displayInfo, dlComposite);
	pc->childrenExecuted= True;
    }
}

static void hideComposite(MedmComposite *pc)
{
    DisplayInfo *displayInfo = pc->updateTask->displayInfo;
    DlComposite *dlComposite = pc->dlElement->structure.composite;

#if DEBUG_COMPOSITE
    print("hideComposite: childrenExecuted=%s\n",
      pc->childrenExecuted?"True !!!":"False");
    print(" dlComposite=%x x=%d y=%d\n",
      dlComposite,dlComposite->object.x,
      dlComposite,dlComposite->object.y);
#endif    
  /* The same code as in hideDlComposite */
    if(pc->childrenExecuted) {
	hideCompositeChildren(displayInfo, dlComposite);
	pc->childrenExecuted = False;
    }
}

static void executeCompositeChildren(DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
    DlElement *pE;
    
#if DEBUG_COMPOSITE
    print("executeCompositeChildren:\n");
    print("dlComposite->dlElementList:\n");
    dumpDlElementList(dlComposite->dlElementList);
/*      print("displayInfo->dlElementList:\n"); */
/*      dumpDlElementList(displayInfo->dlElementList); */
#endif    
    pE = FirstDlElement(dlComposite->dlElementList);
    while (pE) {
	if(pE->run->execute) pE->run->execute(displayInfo, pE);
	pE = pE->next;
    }
}

static void hideCompositeChildren(DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
    DlElement *pE;
    
#if DEBUG_COMPOSITE
    print("hideCompositeChildren:\n");
    print("dlComposite->dlElementList:\n");
    dumpDlElementList(dlComposite->dlElementList);
/*      print("displayInfo->dlElementList:\n"); */
/*      dumpDlElementList(displayInfo->dlElementList); */
#endif    
    pE = FirstDlElement(dlComposite->dlElementList);
    while (pE) {
	if(pE->run->hide) {
	    pE->run->hide(displayInfo, pE);
	}
	pE = pE->next;
    }
}

static void compositeDestroyCb(XtPointer cd)
{
    MedmComposite *pc = (MedmComposite *)cd;

#if DEBUG_COMPOSITE
    print("compositeDestroyCb:\n");
#endif    
    if (pc) {
	Record **records = pc->records;
	
	if(records) {
	    int i;
	    for(i=0; i < MAX_CALC_RECORDS; i++) {
		if(records[i]) medmDestroyRecord(records[i]);
	    }
	    free((char *)records);
	}
	free((char *)pc);
    }
    return;
}

static void compositeGetRecord(XtPointer cd, Record **record, int *count)
{
    MedmComposite *pc = (MedmComposite *)cd;
    int i;
    
    *count = MAX_CALC_RECORDS;
    for(i=0; i < MAX_CALC_RECORDS; i++) {
	record[i] = pc->records[i];
    }
}

DlElement *createDlComposite(DlElement *p) {
    DlComposite *dlComposite;
    DlElement *dlElement;

#if DEBUG_COMPOSITE
    print("createDlComposite:\n");
#endif    
    dlComposite = (DlComposite *) malloc(sizeof(DlComposite));
    if(!dlComposite) return 0;
    if(p) {
	DlElement *child;
	*dlComposite = *p->structure.composite;

      /* Create the first node */
	dlComposite->dlElementList = createDlList();
	if(!dlComposite->dlElementList) {
	    free(dlComposite);
	    return NULL;
	}
      /* Copy all children to the composite element list */
	child = FirstDlElement(p->structure.composite->dlElementList);
	while (child) {
	    DlElement *copy = child->run->create(child);
	    if(copy) 
	      appendDlElement(dlComposite->dlElementList,copy);
	    child = child->next;
	}
    } else {
	objectAttributeInit(&(dlComposite->object));
	dynamicAttributeInit(&(dlComposite->dynAttr));
	dlComposite->compositeName[0] = '\0';
	dlComposite->dlElementList = createDlList();
	if(!dlComposite->dlElementList) {
	    free(dlComposite);
	    return NULL;
	}
    }

    if(!(dlElement = createDlElement(DL_Composite,
      (XtPointer) dlComposite, &compositeDlDispatchTable))) {
	free(dlComposite->dlElementList);
	free(dlComposite);
	return NULL;
    }
    return dlElement;
}

DlElement *groupObjects()
{
    DisplayInfo *cdi = currentDisplayInfo;
    DlComposite *dlComposite;
    DlElement *dlElement, *pE;
    int minX, minY, maxX, maxY;

  /* if there is no element selected, return */
    if(IsEmpty(cdi->selectedDlElementList)) return (DlElement *)0;
    unhighlightSelectedElements();
    saveUndoInfo(cdi);

    if(!(dlElement = createDlComposite(NULL))) return (DlElement *)0;
    appendDlElement(cdi->dlElementList,dlElement);
    dlComposite = dlElement->structure.composite;

/*
 *  now loop over all selected elements and and determine x/y/width/height
 *    of the newly created composite and insert the element.
 */
    minX = INT_MAX; minY = INT_MAX;
    maxX = INT_MIN; maxY = INT_MIN;

    pE = FirstDlElement(cdi->selectedDlElementList);
    while (pE) { 
	DlElement *pE1 = pE->structure.element;
	if(pE1->type != DL_Display) {
	    DlObject *po = &(pE1->structure.composite->object);
	    minX = MIN(minX,po->x);
	    maxX = MAX(maxX,(int)(po->x+po->width));
	    minY = MIN(minY,po->y);
	    maxY = MAX(maxY,(int)(po->y+po->height));
	    removeDlElement(cdi->dlElementList,pE1);
	    appendDlElement(dlComposite->dlElementList,pE1);
	}
	pE = pE->next;
    }

    dlComposite->object.x = minX;
    dlComposite->object.y = minY;
    dlComposite->object.width = maxX - minX;
    dlComposite->object.height = maxY - minY;

    clearResourcePaletteEntries();
    clearDlDisplayList(cdi, cdi->selectedDlElementList);
    if(!(pE = createDlElement(DL_Element,(XtPointer)dlElement,NULL))) {
	return (DlElement *)0;
    }
    appendDlElement(cdi->selectedDlElementList,pE);
    highlightSelectedElements();
    currentActionType = SELECT_ACTION;
    currentElementType = DL_Composite;
    setResourcePaletteEntries();

    return(dlElement);
}

/* Ungroup any grouped (composite) elements which are currently
 * selected.  this removes the appropriate Composite element and moves
 * any children to reflect their new-found autonomy...  */
void ungroupSelectedElements()
{
    DisplayInfo *cdi = currentDisplayInfo;
    DlElement *ele, *dlElement;

    if(!cdi) return;
    saveUndoInfo(cdi);
    if(IsEmpty(cdi->selectedDlElementList)) return;
    unhighlightSelectedElements();

    dlElement = FirstDlElement(cdi->selectedDlElementList);
    while (dlElement) {
	ele = dlElement->structure.element;
	if(ele->type == DL_Composite) {
	    insertDlListAfter(cdi->dlElementList,ele->prev,
	      ele->structure.composite->dlElementList);
	    removeDlElement(cdi->dlElementList,ele);
	    free ((char *) ele->structure.composite->dlElementList);
	    free ((char *) ele->structure.composite);
	    free ((char *) ele);
	}
	dlElement = dlElement->next;
    }

  /* Unselect any selected elements */
    unselectElementsInDisplay();
  /* Cleanup possible damage to non-widgets */
    dmTraverseNonWidgetsInDisplayList(currentDisplayInfo);
}

DlElement *parseComposite(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlComposite *newDlComposite;
    DlElement *dlElement = createDlComposite(NULL);
 
    if(!dlElement) return 0;
    newDlComposite = dlElement->structure.composite;

    do {
        switch(tokenType=getToken(displayInfo,token)) {
	case T_WORD:
	    if(!strcmp(token,"object")) {
		parseObject(displayInfo,&(newDlComposite->object));
	    } else if (!strcmp(token,"dynamic attribute")) {
		parseDynamicAttribute(displayInfo,&(newDlComposite->dynAttr));
	    } else if(!strcmp(token,"composite name")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(newDlComposite->compositeName,token);
	    } else if(!strcmp(token,"children")) {
		tokenType=getToken(displayInfo,token);
		parseAndAppendDisplayList(displayInfo,
		  newDlComposite->dlElementList,token,tokenType);
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

    return dlElement;
}

void writeDlCompositeChildren(FILE *stream, DlElement *dlElement,
  int level)
{
    int i;
    char indent[16];
    DlElement *element;
    DlComposite *dlComposite = dlElement->structure.composite;

    for (i = 0; i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%schildren {",indent);

    element = FirstDlElement(dlComposite->dlElementList);
    while (element != NULL) {		/* any union member is okay here */
	(element->run->write)(stream, element, level+1);
	element = element->next;
    }

    fprintf(stream,"\n%s}",indent);
}


void writeDlComposite(FILE *stream, DlElement *dlElement,
  int level)
{
    int i;
    char indent[16];
    DlComposite *dlComposite = dlElement->structure.composite;

    for (i = 0; i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%scomposite {",indent);
    writeDlObject(stream,&(dlComposite->object),level+1);
    fprintf(stream,"\n%s\t\"composite name\"=\"%s\"",indent,
      dlComposite->compositeName);
    writeDlCompositeChildren(stream,dlElement,level+1);
    writeDlDynamicAttribute(stream,&(dlComposite->dynAttr),level+1);
    fprintf(stream,"\n%s}",indent);
}

/*
 * recursive function to resize Composite objects (and all children, which
 *  may be Composite objects)
 *  N.B.  this is relative to outermost composite, not parent composite
 */
void compositeScale(DlElement *dlElement, int xOffset, int yOffset)
{
    int width, height;
    float scaleX = 1.0, scaleY = 1.0;

    if(dlElement->type != DL_Composite) return;
    width = MAX(1,((int)dlElement->structure.composite->object.width
      + xOffset));
    height = MAX(1,((int)dlElement->structure.composite->object.height
      + yOffset));
    if(dlElement->structure.composite->object.width)
      scaleX =
	(float)width/(float)dlElement->structure.composite->object.width;
    if(dlElement->structure.composite->object.height)
      scaleY =
	(float)height/(float)dlElement->structure.composite->object.height;
    resizeDlElementList(dlElement->structure.composite->dlElementList,
      dlElement->structure.composite->object.x,
      dlElement->structure.composite->object.y,
      scaleX,
      scaleY);
    dlElement->structure.composite->object.width = width;
    dlElement->structure.composite->object.height = height;
}

static void destroyDlComposite(DisplayInfo *displayInfo, DlElement *pE)
{
#if DEBUG_COMPOSITE
    print("destroyDlComposite:\n");
#endif    
    clearDlDisplayList(displayInfo, pE->structure.composite->dlElementList);
    free((char *)pE->structure.composite->dlElementList);
    free((char *)pE->structure.composite);
    free((char *)pE);
}

/*
 * recursive function to move Composite objects (and all children, which
 *  may be Composite objects)
 */
void compositeMove(DlElement *dlElement, int xOffset, int yOffset)
{
    DlElement *ele;

    if(dlElement->type != DL_Composite) return; 
    ele = FirstDlElement(dlElement->structure.composite->dlElementList);
    while (ele != NULL) {
	if(ele->type != DL_Display) {
#if 0
	    if(ele->widget) {
		XtMoveWidget(widget,
		  (Position) (ele->structure.composite->object.x + xOffset),
		  (Position) (ele->structure.composite->object.y + yOffset));
	    }
#endif
	    if(ele->run->move)
	      ele->run->move(ele,xOffset,yOffset);
	}
	ele = ele->next;
    }
    dlElement->structure.composite->object.x += xOffset;
    dlElement->structure.composite->object.y += yOffset;
}

void compositeOrient(DlElement *dlElement, int type, int xCenter, int yCenter)
{
    DlElement *ele;

    if(dlElement->type != DL_Composite) return; 
    ele = FirstDlElement(dlElement->structure.composite->dlElementList);
    while (ele != NULL) {
	if(ele->type != DL_Display) {
	    if(ele->run->orient)
	      ele->run->orient(ele, type, xCenter, yCenter);
	}
	ele = ele->next;
    }
    genericOrient(dlElement, type, xCenter, yCenter);
}

#if 0
static void compositeInheritValues(ResourceBundle *pRCB, DlElement *p)
{
    DlComposite *dlComposite = p->structure.composite;
    medmGetValues(pRCB,
      VIS_RC,        &(dlComposite->dynAttr.vis),
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlComposite->dynAttr.colorRule),
#endif
      VIS_CALC_RC,   &(dlComposite->dynAttr.calc),
      CHAN_A_RC,     &(dlComposite->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlComposite->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlComposite->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlComposite->dynAttr.chan[3]),
      -1);
}
#endif

static void compositeGetValues(ResourceBundle *pRCB, DlElement *p)
{
    DlComposite *dlComposite = p->structure.composite;
    int x, y;
    unsigned int width, height;
    int xOffset, yOffset;
#if 0   
    int clr, bclr;
#endif    

    medmGetValues(pRCB,
      X_RC,          &x,
      Y_RC,          &y,
      WIDTH_RC,      &width,
      HEIGHT_RC,     &height,
#if 0      
      CLR_RC,        &(clr),
      BCLR_RC,       &(bclr),
#endif      
      VIS_RC,        &(dlComposite->dynAttr.vis),
#ifdef __COLOR_RULE_H__
      COLOR_RULE_RC, &(dlComposite->dynAttr.colorRule),
#endif
      VIS_CALC_RC,   &(dlComposite->dynAttr.calc),
      CHAN_A_RC,     &(dlComposite->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlComposite->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlComposite->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlComposite->dynAttr.chan[3]),
      -1);
    xOffset = (int) width - (int) dlComposite->object.width;
    yOffset = (int) height - (int) dlComposite->object.height;
    if(!xOffset || !yOffset) {
	compositeScale(p,xOffset,yOffset);
    }
    xOffset = x - dlComposite->object.x;
    yOffset = y - dlComposite->object.y;
    if(!xOffset || !yOffset) {
	compositeMove(p,xOffset,yOffset);
    }
#if 0    
  /* Colors */
    childE = FirstDlElement(dlComposite->dlElementList);
    while (childE) {
	if(childE->run->setForegroundColor) {
	    childE->run->setForegroundColor(pRCB, childE);
	}
	if(childE->run->setBackgroundColor) {
	    childE->run->setBackgroundColor(pRCB, childE);
	}
	childE = childE->next;
    }
#endif    
}

static void compositeCleanup(DlElement *dlElement)
{
    DlElement *pE =
      FirstDlElement(dlElement->structure.composite->dlElementList);

#if DEBUG_COMPOSITE
    print("compositeCleanup:\n");
#endif    
    while (pE) {
	if(pE->run->cleanup) {
	    pE->run->cleanup(pE);
	} else {
	    pE->widget = NULL;
	}
	pE = pE->next;
    }
}
