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

#include "../medm/medm.h"
static void destroyDlComposite(DlElement *dlElement);
static void compositeMove(DlElement *element, int xOffset, int yOffset);
static void compositeScale(DlElement *element, int xOffset, int yOffset);
static void compositeGetValues(ResourceBundle *pRCB, DlElement *p);
static void compositeCleanup(DlElement *element);

static DlDispatchTable compositeDlDispatchTable = {
    createDlComposite,
    destroyDlComposite,
    executeDlComposite,
    writeDlComposite,
    NULL,
    compositeGetValues,
    NULL,
    NULL,
    NULL,
    compositeMove,
    compositeScale,
    NULL,
    compositeCleanup};

void executeDlComposite(DisplayInfo *displayInfo, DlElement *dlElement)
{
    DlComposite *dlComposite = dlElement->structure.composite;
    DlElement *element;

    if (displayInfo->traversalMode == DL_EDIT) {
	element = FirstDlElement(dlComposite->dlElementList);
	while (element) {
	    (element->run->execute)(displayInfo, element);
	    element = element->next;
	}
    } else
      if (displayInfo->traversalMode == DL_EXECUTE) {
	  if (dlComposite->visible) {
	      element = FirstDlElement(dlComposite->dlElementList);
	      while (element) {
		  (element->run->execute)(displayInfo, element);
		  element = element->next;
	      }
	  }
      }
}

DlElement *createDlComposite(DlElement *p) {
    DlComposite *dlComposite;
    DlElement *dlElement;

    dlComposite = (DlComposite *) malloc(sizeof(DlComposite));
    if (!dlComposite) return 0;
    if (p) {
	DlElement *child;
	*dlComposite = *p->structure.composite;

      /* create the first node */
	dlComposite->dlElementList = createDlList();
	if (!dlComposite->dlElementList) {
	    free(dlComposite);
	    return NULL;
	}
      /* copy all childrern */
	child = FirstDlElement(p->structure.composite->dlElementList);
	while (child) {
	    DlElement *copy = child->run->create(child);
	    if (copy) 
	      appendDlElement(dlComposite->dlElementList,copy);
	    child = child->next;
	}
    } else {
	objectAttributeInit(&(dlComposite->object));
	dlComposite->compositeName[0] = '\0';
	dlComposite->vis = V_STATIC;
	dlComposite->chan[0] = '\0';
	dlComposite->dlElementList = createDlList();
	if (!dlComposite->dlElementList) {
	    free(dlComposite);
	    return NULL;
	}
	dlComposite->visible = True;
    }

    if (!(dlElement = createDlElement(DL_Composite,
      (XtPointer) dlComposite, &compositeDlDispatchTable))) {
	free(dlComposite->dlElementList);
	free(dlComposite);
	return NULL;
    }
    return dlElement;
}

DlElement *groupObjects(DisplayInfo *displayInfo)
{
    DlComposite *dlComposite;
    DlElement *dlElement, *elementPtr;
    int i, minX, minY, maxX, maxY;

  /* if there is no element selected, return */
    if (IsEmpty(displayInfo->selectedDlElementList)) return 0;

    if (!(dlElement = createDlComposite(NULL))) return 0;
    appendDlElement(displayInfo->dlElementList,dlElement);
    dlComposite = dlElement->structure.composite;

/*
 *  now loop over all selected elements and and determine x/y/width/height
 *    of the newly created composite and insert the element.
 */
    minX = INT_MAX; minY = INT_MAX;
    maxX = INT_MIN; maxY = INT_MIN;

    elementPtr = FirstDlElement(displayInfo->selectedDlElementList);
    while (elementPtr) { 
	DlElement *pE = elementPtr->structure.element;
	if (pE->type != DL_Display) {
	    DlObject *po = &(pE->structure.rectangle->object);
	    minX = MIN(minX,po->x);
	    maxX = MAX(maxX,(int)(po->x+po->width));
	    minY = MIN(minY,po->y);
	    maxY = MAX(maxY,(int)(po->y+po->height));
	    removeDlElement(displayInfo->dlElementList,pE);
	    appendDlElement(dlComposite->dlElementList,pE);
	}
	elementPtr = elementPtr->next;
    }

    dlComposite->object.x = minX;
    dlComposite->object.y = minY;
    dlComposite->object.width = maxX - minX;
    dlComposite->object.height = maxY - minY;

    clearResourcePaletteEntries();
    unhighlightSelectedElements();
    destroyDlDisplayList(displayInfo->selectedDlElementList);
    if (!(elementPtr = createDlElement(NULL,NULL,NULL))) {
	return 0;
    }
    elementPtr->structure.element = dlElement;
    appendDlElement(displayInfo->selectedDlElementList,elementPtr);
    highlightSelectedElements();
    currentActionType = SELECT_ACTION;
    currentElementType = DL_Composite;
    setResourcePaletteEntries();

    return(dlElement);
}


DlElement *parseComposite(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlComposite *newDlComposite;
    DlElement *dlElement = createDlComposite(NULL);
 
    if (!dlElement) return 0;
    newDlComposite = dlElement->structure.composite;

    do {
        switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object")) {
		parseObject(displayInfo,&(newDlComposite->object));
	    } else if (!strcmp(token,"composite name")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(newDlComposite->compositeName,token);
	    } else if (!strcmp(token,"vis")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"static"))
		  newDlComposite->vis = V_STATIC;
		else if (!strcmp(token,"if not zero"))
		  newDlComposite->vis = IF_NOT_ZERO;
		else if (!strcmp(token,"if zero"))
		  newDlComposite->vis = IF_ZERO;
	    } else if (!strcmp(token,"chan")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(newDlComposite->chan,token);
	    } else if (!strcmp(token,"children")) {
		parseAndAppendDisplayList(displayInfo,
		  newDlComposite->dlElementList);
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

void writeDlCompositeChildren(
  FILE *stream,
  DlElement *dlElement,
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


void writeDlComposite(
  FILE *stream,
  DlElement *dlElement,
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
    fprintf(stream,"\n%s\tvis=\"%s\"",indent,stringValueTable[dlComposite->vis]);
    fprintf(stream,"\n%s\tchan=\"%s\"",indent,dlComposite->chan);
    writeDlCompositeChildren(stream,dlElement,level+1);
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

    if (dlElement->type != DL_Composite) return;
    width = MAX(1,((int)dlElement->structure.composite->object.width
      + xOffset));
    height = MAX(1,((int)dlElement->structure.composite->object.height
      + yOffset));
    scaleX = (float)width/(float)dlElement->structure.composite->object.width;
    scaleY = (float)height/(float)dlElement->structure.composite->object.height;
    resizeDlElementList(dlElement->structure.composite->dlElementList,
      dlElement->structure.composite->object.x,
      dlElement->structure.composite->object.y,
      scaleX,
      scaleY);
    dlElement->structure.composite->object.width = width;
    dlElement->structure.composite->object.height = height;
}

static void destroyDlComposite(DlElement *dlElement) {
    destroyDlDisplayList(dlElement->structure.composite->dlElementList);
    free((char *) dlElement->structure.composite->dlElementList);
    free((char *) dlElement->structure.composite);
    free((char *) dlElement);
}

/*
 * recursive function to move Composite objects (and all children, which
 *  may be Composite objects)
 */
void compositeMove(DlElement *dlElement, int xOffset, int yOffset)
{
    DlElement *ele;

    if (dlElement->type != DL_Composite) return; 
    ele = FirstDlElement(dlElement->structure.composite->dlElementList);
    while (ele != NULL) {
	if (ele->type != DL_Display) {
#if 0
	    if (ele->widget) {
		XtMoveWidget(widget,
		  (Position) (ele->structure.rectangle->object.x + xOffset),
		  (Position) (ele->structure.rectangle->object.y + yOffset));
	    }
#endif
	    if (ele->run->move)
	      ele->run->move(ele,xOffset,yOffset);
	}
	ele = ele->next;
    }
    dlElement->structure.composite->object.x += xOffset;
    dlElement->structure.composite->object.y += yOffset;
}

static void compositeGetValues(ResourceBundle *pRCB, DlElement *p) {
    DlComposite *dlComposite = p->structure.composite;
    int x, y;
    unsigned int width, height;
    int xOffset, yOffset;

    medmGetValues(pRCB,
      X_RC,          &x,
      Y_RC,          &y,
      WIDTH_RC,      &width,
      HEIGHT_RC,     &height,
      -1);
    xOffset = (int) width - (int) dlComposite->object.width;
    yOffset = (int) height - (int) dlComposite->object.height;
    if (!xOffset || !yOffset) {
	compositeScale(p,xOffset,yOffset);
    }
    xOffset = x - dlComposite->object.x;
    yOffset = y - dlComposite->object.y;
    if (!xOffset || !yOffset) {
	compositeMove(p,xOffset,yOffset);
    }
}

static void compositeCleanup(DlElement *dlElement) {
    DlElement *pE = FirstDlElement(dlElement->structure.composite->dlElementList);
    while (pE) {
	if (pE->run->cleanup) {
	    pE->run->cleanup(pE);
	} else {
	    pE->widget = NULL;
	}
	pE = pE->next;
    }
}
