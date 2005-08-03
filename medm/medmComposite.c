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

#define DEBUG_COMPOSITE 0
#define DEBUG_DELETE 0
#define DEBUG_REDRAW 0
#define DEBUG_FILE 0

#include "medm.h"

typedef struct _MedmComposite {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;    /* Must be second */
    Record           **records;
    DisplayInfo      *displayInfo;
    CompositeUpdateState updateState;
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
  DlElement *dlElement);
static void hideCompositeChildren(DisplayInfo *displayInfo,
  DlElement *dlElement);
static void drawComposite(MedmComposite *pc);
static void hideComposite(MedmComposite *pc);
static void compositeFileParse(DisplayInfo *displayInfo,
  DlElement *dlElement);

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
    print("executeDlComposite: dlComposite=%x x=%d y=%d\n",
      dlComposite,dlComposite->object.x,dlComposite->object.y);
#endif

  /* Don't do anyting if the element is hidden */
    if(dlElement->hidden) return;

    if(displayInfo->traversalMode == DL_EXECUTE) {
      /* EXECUTE mode */
	if(*dlComposite->dynAttr.chan[0]) {
	  /* A channel is defined */
	    MedmComposite *pc;

	  /* Allocate and fill in MedmComposite struct */
	    if(dlElement->data) {
		pc = (MedmComposite *)dlElement->data;
	    } else {
		pc = (MedmComposite *)malloc(sizeof(MedmComposite));
		dlElement->updateType = DYNAMIC_GRAPHIC;
		dlElement->data = (void *)pc;
		if(pc == NULL) {
		    medmPrintf(1,"\nexecuteDlArcComposite:"
		      " Memory allocation error\n");
		    return;
		}
	      /* Pre-initialize */
		pc->updateTask = NULL;
		pc->records = NULL;
		pc->dlElement = dlElement;
		pc->updateState = COMPOSITE_NEW;

		pc->displayInfo = displayInfo;
		pc->records = NULL;
		pc->updateTask = updateTaskAddTask(displayInfo,
		  &(dlComposite->object), compositeDraw, (XtPointer)pc);
		if(pc->updateTask == NULL) {
		    medmPrintf(1,"\nexecuteDlComposite: Memory allocation error\n");
		} else {
		    updateTaskAddDestroyCb(pc->updateTask,compositeDestroyCb);
		    updateTaskAddNameCb(pc->updateTask,compositeGetRecord);
		}
		if(!isStaticDynamic(&dlComposite->dynAttr, False)) {
		    pc->records = medmAllocateDynamicRecords(&dlComposite->dynAttr,
		      compositeUpdateValueCb,
		      compositeUpdateGraphicalInfoCb,
		      (XtPointer)pc);
		    calcPostfix(&dlComposite->dynAttr);
		    setDynamicAttrMonitorFlags(&dlComposite->dynAttr,
		      pc->records);
		}
	    }
	} else {
	  /* Static */
	    dlElement->updateType = STATIC_GRAPHIC;
	    executeCompositeChildren(displayInfo, dlElement);
	}
    } else {
      /* EDIT mode */
      /* Check if the element list has been cleared because there is a
         new file */
	if(dlComposite->dlElementList->count == 0 &&
	  *dlComposite->compositeFile) {
	    compositeFileParse(displayInfo, dlElement);
	}
	executeCompositeChildren(displayInfo, dlElement);
    }

#if DEBUG_COMPOSITE
    print("END executeDlComposite: dlComposite=%x\n",dlComposite);
#endif
}

static void compositeUpdateGraphicalInfoCb(XtPointer cd)
{
#if DEBUG_COMPOSITE
    Record *pR = (Record *)cd;
    print("compositeUpdateGraphicalInfoCb: record=%x[%s] value=%g\n",
      pR,pR->name,pR->value);
#endif
#if 0
    MedmComposite *pc = (MedmComposite *)pR->clientData;
    updateTaskMarkUpdate(pc->updateTask);
#endif
}

static void compositeUpdateValueCb(XtPointer cd)
{
    Record *pR = (Record *)cd;
    MedmComposite *pc = (MedmComposite *)pR->clientData;

#if DEBUG_COMPOSITE
    print("compositeUpdateValueCb: record=%x[%s] value=%g\n",
      pR,pR->name,pR->value);
#endif
    updateTaskMarkUpdate(pc->updateTask);
#if DEBUG_COMPOSITE
    print("  executePendingRequestsCount=%d\n",
      pc->updateTask->executeRequestsPendingCount);
#endif
}

static void compositeDraw(XtPointer cd)
{
    MedmComposite *pc = (MedmComposite *)cd;
    Record *pR = pc->records?pc->records[0]:NULL;
    DisplayInfo *displayInfo = pc->updateTask->displayInfo;
    DlComposite *dlComposite = pc->dlElement->structure.composite;

#if DEBUG_COMPOSITE
    print("compositeDraw: pc=%x dlComposite=%x\n",
      pc,pc->dlElement->structure.composite);
#endif
#if DEBUG_DELETE
    print("compositeDraw: connected=%s readAccess=%s value=%g\n",
      pR->connected?"Yes":"No",pR->readAccess?"Yes":"No",pR->value);
#endif
  /* Branch on whether there is a channel or not */
    if(*dlComposite->dynAttr.chan[0]) {
	if(isConnected(pc->records)) {
      /* A channel is defined */
#if DEBUG_COMPOSITE > 1
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
	    if(!pR->readAccess) {
		hideComposite(pc);
		drawBlackRectangle(pc->updateTask);
	    }
	} else if(isStaticDynamic(&dlComposite->dynAttr, False)) {
	    executeCompositeChildren(displayInfo, pc->dlElement);
	} else {
	    hideComposite(pc);
	    drawWhiteRectangle(pc->updateTask);
	}
    } else {
      /* No channel */
	executeCompositeChildren(displayInfo, pc->dlElement);
    }
}

/* The routine that draws as opposed to hide.  Currently just calls
   executeCompositeChildren.  Called from compositeDraw */
static void drawComposite(MedmComposite *pc)
{
    DisplayInfo *displayInfo = pc->updateTask->displayInfo;

    executeCompositeChildren(displayInfo, pc->dlElement);
}

static void executeCompositeChildren(DisplayInfo *displayInfo,
  DlElement *dlElement)
{
    DlElement *pE;
    DlComposite *dlComposite = dlElement->structure.composite;

#if DEBUG_COMPOSITE
    print("executeCompositeChildren: dlElement=%x\n",dlElement);
#endif
  /* Loop over children */
    pE = FirstDlElement(dlComposite->dlElementList);
    while(pE) {
#if 0
	UpdateTask *t = getUpdateTaskFromElement(pE);
#endif
	pE->hidden = False;
	if(!displayInfo->elementsExecuted) {
	    pE->data = NULL;
	}
	if(pE->data) updateTaskEnableTask(pE);
	if(pE->run->execute) {
#if DEBUG_COMPOSITE || DEBUG_REDRAW
	    print("  executed: pE=%x[%s] x=%d y=%d hidden=%s\n",
	      pE,elementType(pE->type),
	      pE->structure.composite->object.x,
	      pE->structure.composite->object.y,
	      pE->hidden?"True":"False");
#endif
	    pE->run->execute(displayInfo, pE);
	}
	pE = pE->next;
    }

  /* Change updateState */
    if(dlElement->data) {
	MedmComposite *pc = (MedmComposite *)dlElement->data;
	if(pc->updateState != COMPOSITE_VISIBLE_UPDATED) {
	    pc->updateState = COMPOSITE_VISIBLE;
	}
    }
}

/* The routine that hides as opposed to draw.  Currently just calls
   hideCompositeChildren.  Called from compositeDraw */
static void hideComposite(MedmComposite *pc)
{
    DisplayInfo *displayInfo = pc->updateTask->displayInfo;

  /* Don't do anything if already hidden and updated */
    if(pc && pc->updateState == COMPOSITE_HIDDEN_UPDATED) return;

    hideCompositeChildren(displayInfo, pc->dlElement);
}

/* The routine that does the actual hiding.  Called from
   hideDlComposite and hideComposite (called from compositeDraw). */
static void hideCompositeChildren(DisplayInfo *displayInfo,
  DlElement *dlElement)
{
    DlElement *pE;
    DlComposite *dlComposite = dlElement->structure.composite;
    MedmComposite *pc = (MedmComposite *)dlElement->data;

#if DEBUG_COMPOSITE
    print("hideCompositeChildren: dlElement=%x\n",dlElement);
/*      print("dlComposite->dlElementList:\n"); */
/*      dumpDlElementList(dlComposite->dlElementList); */
/*      print("displayInfo->dlElementList:\n"); */
/*      dumpDlElementList(displayInfo->dlElementList); */
#endif
  /* If we are doing this the first time, we need to execute all the
     elements to insure their update tasks are in the right stacking
     order */
    if(!displayInfo->elementsExecuted) {
	executeCompositeChildren(displayInfo, dlElement);
    }

  /* Don't do anything if already hidden and updated */
    if(pc && pc->updateState == COMPOSITE_HIDDEN_UPDATED) return;

  /* Hide them */
    pE = FirstDlElement(dlComposite->dlElementList);
    while(pE) {
	if(pE->run->hide) {
#if DEBUG_COMPOSITE
	    print("  hid: pE=%x[%s] x=%d y=%d\n",
	      pE,elementType(pE->type),
	      pE->structure.composite->object.x,
	      pE->structure.composite->object.y);
#endif
	    pE->run->hide(displayInfo, pE);
	}
	pE = pE->next;
    }

  /* Change updateState */
    if(dlElement->data) {
	MedmComposite *pc = (MedmComposite *)dlElement->data;

	if(pc->updateState != COMPOSITE_HIDDEN_UPDATED) {
	    pc->updateState = COMPOSITE_HIDDEN;
	}
    }
#if DEBUG_COMPOSITE
    print("END hideCompositeChildren: dlElement=%x\n",dlElement);
#endif
  }

/* The hide routine in the dispatch table */
void hideDlComposite(DisplayInfo *displayInfo, DlElement *dlElement)
{

    MedmComposite *pc;
#if DEBUG_COMPOSITE
    print("hideDlComposite:\n");
#endif
    if(!displayInfo || !dlElement) return;

  /* Don't do anything if already hidden and updated */
    pc = (MedmComposite *)dlElement->data;
    if(pc && pc->updateState == COMPOSITE_HIDDEN_UPDATED) return;

  /* Disable the update task */
    if(dlElement->data) updateTaskDisableTask(dlElement);

  /* The same code as in hideComposite */
    hideCompositeChildren(displayInfo, dlElement);
}

static void compositeDestroyCb(XtPointer cd)
{
    MedmComposite *pc = (MedmComposite *)cd;

#if DEBUG_COMPOSITE
    print("compositeDestroyCb:\n");
#endif
    if(pc) {
	Record **records = pc->records;

	if(records) {
	    int i;
	    for(i=0; i < MAX_CALC_RECORDS; i++) {
		if(records[i]) medmDestroyRecord(records[i]);
	    }
	    free((char *)records);
	}
	if(pc->dlElement) pc->dlElement->data = NULL;
	free((char *)pc);
    }
    return;
}

static void compositeGetRecord(XtPointer cd, Record **record, int *count)
{
    MedmComposite *pc = (MedmComposite *)cd;
    int i;

    *count = 0;
    if(pc && pc->records) {
	for(i=0; i < MAX_CALC_RECORDS; i++) {
	    if(pc->records[i]) {
		record[(*count)++] = pc->records[i];
	    }
	}
    }
}

DlElement *createDlComposite(DlElement *p) {
    DlComposite *dlComposite;
    DlElement *dlElement;

#if DEBUG_COMPOSITE
    print("createDlComposite:\n");
#endif
    dlComposite = (DlComposite *)malloc(sizeof(DlComposite));
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
	while(child) {
	    DlElement *copy = child->run->create(child);
	    if(copy)
	      appendDlElement(dlComposite->dlElementList,copy);
	    child = child->next;
	}
    } else {
	objectAttributeInit(&(dlComposite->object));
	dynamicAttributeInit(&(dlComposite->dynAttr));
	dlComposite->compositeName[0] = '\0';
	dlComposite->compositeFile[0] = '\0';
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
    while(pE) {
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
    while(dlElement) {
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
	    } else if(!strcmp(token,"dynamic attribute")) {
		parseDynamicAttribute(displayInfo,&(newDlComposite->dynAttr));
	    } else if(!strcmp(token,"composite name")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(newDlComposite->compositeName,token);
	    } else if(!strcmp(token,"composite file")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(newDlComposite->compositeFile,token);
		compositeFileParse(displayInfo,dlElement);
	      /* Handle composite file here */
	    } else if(!strcmp(token,"children")) {
		tokenType=getToken(displayInfo,token);
		parseAndAppendDisplayList(displayInfo,
		  newDlComposite->dlElementList,token,tokenType);
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

    return dlElement;
}

static void compositeFileParse(DisplayInfo *displayInfo,
  DlElement *dlElement)
{
    FILE *filePtr, *savedFilePtr;
    int savedVersionNumber;
    char filename[MAX_TOKEN_LENGTH];
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    DlFile *dlFile;
    DlElement *pE, *pD;
    DlObject *pO;
    DlComposite *dlComposite;
    int minX, minY, maxX, maxY, oldX, oldY;
    int displayH, displayW;

    if(!displayInfo || !dlElement) return;
    dlComposite = dlElement->structure.composite;

  /* Work with a copy of the name so the original doesn't get paths
     attached, delimiter characters changed, etc. */
    strncpy(filename,dlComposite->compositeFile,MAX_TOKEN_LENGTH);
    filename[MAX_TOKEN_LENGTH-1]='\0';

  /* Open the file */
    filePtr = dmOpenUsableFile(filename, displayInfo->dlFile->name);
    if(!filePtr) {
	medmPrintf(1,"\ncompositeFileParse: Cannot open file\n"
	  "  filename: %s\n",dlComposite->compositeFile);
	return;
    }

  /* Since getToken() uses the displayInfo, we have to save the file
     pointer in the displayInfo and plug in the current one.  We also
     have to save the version number. (It is zero for a new display.)  */
    savedFilePtr = displayInfo->filePtr;
    savedVersionNumber = displayInfo->versionNumber;
    displayInfo->filePtr = filePtr;

  /* Read the file block (Must be there) */
    dlFile = createDlFile(displayInfo);;
  /* If first token isn't "file" then bail out */
    tokenType=getToken(displayInfo,token);
    if(tokenType == T_WORD && !strcmp(token,"file")) {
	parseAndSkip(displayInfo);
    } else {
	medmPostMsg(1,"compositeFileParse: Invalid .adl file "
	  "(First block is not file block)\n"
	  "  file: %s\n",filename);
	fclose(filePtr);
	goto RETURN;
    }
  /* Plug the current version number into the displayInfo */
    displayInfo->versionNumber = dlFile->versionNumber;
    free((char *)dlFile);

  /* Read the display block */
    tokenType=getToken(displayInfo,token);
    if(tokenType == T_WORD && !strcmp(token,"display")) {
	parseAndSkip(displayInfo);
	tokenType=getToken(displayInfo,token);
    }

  /* Read the colormap */
    if(tokenType == T_WORD &&
      (!strcmp(token,"color map") ||
	!strcmp(token,"<<color map>>"))) {
	parseAndSkip(displayInfo);
	tokenType=getToken(displayInfo,token);
    }

  /* Proceed with parsing */
    while(parseAndAppendDisplayList(displayInfo, dlComposite->dlElementList,
      token, tokenType) != T_EOF) {
	tokenType=getToken(displayInfo,token);
    }
    fclose(filePtr);

  /* Rearrange the composite to fit its contents */
    minX = INT_MAX; minY = INT_MAX;
    maxX = INT_MIN; maxY = INT_MIN;
    pE = FirstDlElement(dlComposite->dlElementList);
    while(pE) {
	pO = &(pE->structure.composite->object);

	minX = MIN(minX,pO->x);
	maxX = MAX(maxX,(int)(pO->x+pO->width));
	minY = MIN(minY,pO->y);
	maxY = MAX(maxY,(int)(pO->y+pO->height));
#if DEBUG_FILE
	print("  %-20s %3d %3d %3d %3d %3d %3d\n",
	  elementType(pE->type),
	  pO->x,pO->y,pO->width,pO->height,
	  (int)(pO->x+pO->width),(int)(pO->x+pO->height));
#endif
	pE = pE->next;
    }
    oldX = dlComposite->object.x;
    oldY = dlComposite->object.y;
    dlComposite->object.x = minX;
    dlComposite->object.y = minY;
    dlComposite->object.width = maxX - minX;
    dlComposite->object.height = maxY - minY;

  /* Move the rearranged composite to its original x and y coordinates */
    compositeMove(dlElement, oldX - minX, oldY - minY);

  /* Check composite is in bounds */
    displayW = displayH = 0;
    pD = FirstDlElement(displayInfo->dlElementList);
    pO = &(dlComposite->object);
    if(pD && pO) {
	displayW = pD->structure.display->object.width;
	displayH = pD->structure.display->object.height;
	if((pO->x) > displayW ||
	  (pO->x + (int)pO->width) < 0 ||
	  (pO->y) > displayH ||
	  (pO->y + (int)pO->height) < 0) {
	    medmPrintf(1,"\ncompositeFileParse:"
	      " Composite from file extends beyond display:\n"
	      "  File: %s\n", filename);
	} else if((pO->x) < 0 ||
	  (pO->x + (int)pO->width) > displayW ||
	  (pO->y) < 0 ||
	  (pO->y + (int)pO->height) > displayH) {
	    medmPrintf(1,"\ncompositeFileParse:"
	      " Composite from file extends beyond display:\n"
	      "  File: %s\n", filename);
	}
    }
#if DEBUG_FILE
    print("  displayW=%d displayH=%d width=%d height=%d\n",
      displayW,displayH,(int)dlComposite->object.width,
      (int)dlComposite->object.height);
#endif

  RETURN:

  /* Restore displayInfo file parameters */
    displayInfo->filePtr = savedFilePtr;
    displayInfo->versionNumber = savedVersionNumber;
}

void writeDlCompositeChildren(FILE *stream, DlElement *dlElement,
  int level)
{
    int i;
    char indent[16];
    DlElement *element;
    DlComposite *dlComposite = dlElement->structure.composite;

    for(i = 0; i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%schildren {",indent);

    element = FirstDlElement(dlComposite->dlElementList);
    while(element != NULL) {		/* any union member is okay here */
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

    for(i = 0; i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%scomposite {",indent);
    writeDlObject(stream,&(dlComposite->object),level+1);
    fprintf(stream,"\n%s\t\"composite name\"=\"%s\"",indent,
      dlComposite->compositeName);
    if(*dlComposite->compositeFile) {
	fprintf(stream,"\n%s\t\"composite file\"=\"%s\"",indent,
	  dlComposite->compositeFile);
    } else {
	writeDlCompositeChildren(stream,dlElement,level+1);
    }
    writeDlDynamicAttribute(stream,&(dlComposite->dynAttr),level+1);
    fprintf(stream,"\n%s}",indent);
}

/*
 * recursive function to resize Composite objects (and all children, which
 *  may be Composite objects)
 *  N.B.  this is relative to outermost composite, not parent composite
 */
static void compositeScale(DlElement *dlElement, int xOffset, int yOffset)
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
static void compositeMove(DlElement *dlElement, int xOffset, int yOffset)
{
    DlElement *ele;

    if(dlElement->type != DL_Composite) return;
    ele = FirstDlElement(dlElement->structure.composite->dlElementList);
    while(ele != NULL) {
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

static void compositeOrient(DlElement *dlElement, int type, int xCenter,
  int yCenter)
{
    DlElement *ele;

    if(dlElement->type != DL_Composite) return;
    ele = FirstDlElement(dlElement->structure.composite->dlElementList);
    while(ele != NULL) {
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
      VIS_CALC_RC,   &(dlComposite->dynAttr.calc),
      CHAN_A_RC,     &(dlComposite->dynAttr.chan[0]),
      CHAN_B_RC,     &(dlComposite->dynAttr.chan[1]),
      CHAN_C_RC,     &(dlComposite->dynAttr.chan[2]),
      CHAN_D_RC,     &(dlComposite->dynAttr.chan[3]),
      COMPOSITE_FILE_RC, &(dlComposite->compositeFile),
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
    while(childE) {
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
    while(pE) {
	if(pE->run->cleanup) {
	    pE->run->cleanup(pE);
	} else {
	    pE->widget = NULL;
	}
	pE = pE->next;
    }
}

CompositeUpdateState getCompositeUpdateState(DlElement *dlElement)
{
    if(dlElement && dlElement->data) {
	MedmComposite *pc = (MedmComposite *)dlElement->data;

	return pc->updateState;
    } else {
	return COMPOSITE_NEW;
    }
}

void setCompositeUpdateState(DlElement *dlElement, CompositeUpdateState state)
{
    if(dlElement && dlElement->data) {
	MedmComposite *pc = (MedmComposite *)dlElement->data;

	pc->updateState = state;
    }
}
