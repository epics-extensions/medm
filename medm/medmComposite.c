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
 * .02  09-05-95        vong    2.1.0 release
 *                              correct the falling line and rising line to
 *                              polyline geometry calculation
 * .03  09-08-95        vong    conform to c++ syntax
 *
 *****************************************************************************
*/

#include "../medm/medm.h"

void executeDlComposite(DisplayInfo *displayInfo, DlComposite *dlComposite,
        Boolean forcedDisplayToWindow)
{
  DlElement *element;

  if (displayInfo->traversalMode == DL_EDIT) {

/* like dmTraverseDisplayList: traverse composite's display list */
    element = ((DlElement *)dlComposite->dlElementListHead)->next;
    while (element != NULL) {
/* type in union is unimportant: just trying to get to element structure */
/* third argument is for statics acting as dynamics (for forced display) */
        (*element->dmExecute)(displayInfo,
                              (XtPointer) element->structure.file,
                                          FALSE);
        element = element->next;
    }



  } else if (displayInfo->traversalMode == DL_EXECUTE) {

    if (dlComposite->visible) {

/* like dmTraverseDisplayList: traverse composite's display list */
      element = ((DlElement *)dlComposite->dlElementListHead)->next;
      while (element != NULL) {
/* type in union is unimportant: just trying to get to element structure */
/* third argument is for statics acting as dynamics (for forced display) */
        (*element->dmExecute)(displayInfo,
                              (XtPointer) element->structure.file,
                                          forcedDisplayToWindow);
        element = element->next;
      }


    }

  }

}

DlElement *createDlComposite(DisplayInfo *displayInfo) {
  DlComposite *dlComposite;
  DlElement *dlElement;

  dlComposite = (DlComposite *) malloc(sizeof(DlComposite));
  if (!dlComposite) return 0;
  objectAttributeInit(&(dlComposite->object));
  dlComposite->compositeName[0] = '\0';
  dlComposite->vis = V_STATIC;
  dlComposite->chan[0] = '\0';
  dlComposite->dlElementListHead = (DlElement *)malloc(sizeof(DlElement));
  if (!dlComposite->dlElementListHead) {
    free(dlComposite);
    return NULL;
  }
  dlComposite->dlElementListHead->next = 0;
  dlComposite->dlElementListHead->prev = 0;
  dlComposite->dlElementListTail = dlComposite->dlElementListHead;
  dlComposite->visible = True;

  if (!(dlElement = createDlElement(DL_Composite,
                    (XtPointer)      dlComposite,
                    (medmExecProc)   executeDlComposite,
                    (medmWriteProc)  writeDlComposite,
										0,0,0))) {
    free(dlComposite->dlElementListHead);
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
  DlElement **array;
  DlElement *newCompositePosition;
  Boolean foundFirstSelected;

/* composites only exist if members exist:  numSelectedElements must be > 0 */
  if (displayInfo->numSelectedElements <= 0) return 0;

  if (!(dlElement = createDlComposite(displayInfo))) return 0;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;

  dlComposite = dlElement->structure.composite;
/*
 * Save the position of first element in "visibility space" (but not 
 *  necessarily the last element in the selected elements array);
 *  since selects can be multiple selects appended to by Shift-MB1, the
 *  relative order in the selectedElementsArray[] can be not "proper"
 *  -- start at dlColormap element and move forward 'til first selected
 *      element is found
 */
  newCompositePosition = displayInfo->dlColormapElement->next;
  foundFirstSelected = False;
  while (!foundFirstSelected) {
    for (i = 0; i < displayInfo->numSelectedElements; i++) {
      if (newCompositePosition == displayInfo->selectedElementsArray[i]) {
	foundFirstSelected = True;
	break;	/* out of for loop */
      }
    }
    if (!foundFirstSelected)
      newCompositePosition = newCompositePosition->next;
  }
  newCompositePosition = newCompositePosition->prev;

/*
 *  now loop over all selected elements and and determine x/y/width/height
 *    of the newly created composite and insert the element.
 */
  minX = INT_MAX; minY = INT_MAX;
  maxX = INT_MIN; maxY = INT_MIN;

  for (i = displayInfo->numSelectedElements - 1; i >= 0; i--) {
    elementPtr = displayInfo->selectedElementsArray[i];
    if (elementPtr->type != DL_Display) {
      minX = MIN(minX,elementPtr->structure.rectangle->object.x);
      maxX = MAX(maxX,elementPtr->structure.rectangle->object.x +
		(int)elementPtr->structure.rectangle->object.width);
      minY = MIN(minY,elementPtr->structure.rectangle->object.y);
      maxY = MAX(maxY,elementPtr->structure.rectangle->object.y +
		(int)elementPtr->structure.rectangle->object.height);
      moveElementAfter(
         dlElement->structure.composite->dlElementListTail,
         elementPtr,
         &(dlElement->structure.composite->dlElementListTail));
    }
  }

  dlComposite->object.x = minX;
  dlComposite->object.y = minY;
  dlComposite->object.width = maxX - minX;
  dlComposite->object.height = maxY - minY;

/* move the newly created element to just before the first child element */
  moveElementAfter(newCompositePosition,dlElement,
                   &(displayInfo->dlElementListTail));

/*
 * now select this newly created Composite/group (this unselects the previously
 *	selected children, etc)
 */
  highlightAndSetSelectedElements(NULL,0,0);
  clearResourcePaletteEntries();
  array = (DlElement **)malloc(1*sizeof(DlElement *));
  array[0] = dlElement;
  highlightAndSetSelectedElements(array,1,1);
  currentActionType = SELECT_ACTION;
  currentElementType = DL_Composite;
  setResourcePaletteEntries();

  return(dlElement);
}


DlElement *parseComposite(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlComposite *newDlComposite;
  DlElement *dlElement = createDlComposite(displayInfo);
 
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
                        parseCompositeChildren(displayInfo,newDlComposite);
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

  POSITION_ELEMENT_ON_LIST();

  return dlElement;

}

void writeDlCompositeChildren(
  FILE *stream,
  DlComposite *dlComposite,
  int level)
{
  int i;
  char indent[16];
  DlElement *element;

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%schildren {",indent);

  element = ((DlElement *)dlComposite->dlElementListHead)->next;
  while (element != NULL) {		/* any union member is okay here */
     (*element->dmWrite)(stream,
                         (XtPointer) element->structure.rectangle,level+1);
     element = element->next;
  }

  fprintf(stream,"\n%s}",indent);
}


void writeDlComposite(
  FILE *stream,
  DlComposite *dlComposite,
  int level)
{
  int i;
  char indent[16];

  for (i = 0; i < level; i++) indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream,"\n%scomposite {",indent);
  writeDlObject(stream,&(dlComposite->object),level+1);
  fprintf(stream,"\n%s\t\"composite name\"=\"%s\"",indent,
		dlComposite->compositeName);
  fprintf(stream,"\n%s\tvis=\"%s\"",indent,stringValueTable[dlComposite->vis]);
  fprintf(stream,"\n%s\tchan=\"%s\"",indent,dlComposite->chan);
  writeDlCompositeChildren(stream,dlComposite,level+1);
  fprintf(stream,"\n%s}",indent);
}
