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

/* Include this after medm.h to avoid problems with Exceed 6 */
#ifdef WIN32
/* In MSVC timeval is in winsock.h, winsock2.h, ws2spi.h, nowhere else */
#include <X11/Xos.h>
#else
#include <sys/time.h>
#endif

#define TIMERINTERVAL 100 /* ms */
#define WORKINTERVAL .05
#define EXECINTERVAL 3600.

typedef struct _UpdateTaskStatus {
    XtWorkProcId workProcId;
    UpdateTask  *nextToServe;
    int          taskCount;
    int          periodicTaskCount;
    int          updateRequestCount;
    int          updateDiscardCount;
    int          periodicUpdateRequestCount;
    int          periodicUpdateDiscardCount;
    int          updateRequestQueued;          /* this one won't reset */
    int          updateExecuted;
    double       since;
} UpdateTaskStatus;

typedef struct {
    XtIntervalId id;
    double      systemTime;
    double      tenthSecond;
} PeriodicTask;

/* Function prototypes */
Boolean medmInitUpdateTask();
static void medmScheduler(XtPointer, XtIntervalId *);
static Boolean updateTaskWorkProc(XtPointer);
static DlElement *getElementFromUpdateTask(UpdateTask *t);

/* Global variables */
static UpdateTaskStatus updateTaskStatus;
static PeriodicTask periodicTask;
static Boolean moduleInitialized = False;
static double medmStartTime=0;
static UpdateTask nullTask = {
    NULL,
    NULL,
    NULL,
    NULL,
    (XtPointer)0,
    0.0,
    0.0,
    (struct _DisplayInfo *)0,
    0,
    {0,0,0,0},     /* Rectangle */
    False,
    False,
    False,
    (struct _UpdateTask *)0,
    (struct _UpdateTask *)0,
};

void updateTaskStatusGetInfo(int *taskCount,
  int *periodicTaskCount,
  int *updateRequestCount,
  int *updateDiscardCount,
  int *periodicUpdateRequestCount,
  int *periodicUpdateDiscardCount,
  int *updateRequestQueued,
  int *updateExecuted,
  double *timeInterval)
{
    double time = medmTime();
    
    *taskCount = updateTaskStatus.taskCount;
    *periodicTaskCount = updateTaskStatus.periodicTaskCount;
    *updateRequestCount = updateTaskStatus.updateRequestCount;
    *updateDiscardCount = updateTaskStatus.updateDiscardCount;
    *periodicUpdateRequestCount = updateTaskStatus.periodicUpdateRequestCount;
    *periodicUpdateDiscardCount = updateTaskStatus.periodicUpdateDiscardCount;
    *updateRequestQueued = updateTaskStatus.updateRequestQueued;
    *updateExecuted = updateTaskStatus.updateExecuted;
    *timeInterval = time - updateTaskStatus.since;

  /* reset the periodic data */
    updateTaskStatus.updateRequestCount = 0;
    updateTaskStatus.updateDiscardCount = 0;
    updateTaskStatus.periodicUpdateRequestCount = 0;
    updateTaskStatus.periodicUpdateDiscardCount = 0;
    updateTaskStatus.updateExecuted = 0;
    updateTaskStatus.since = time;
}

/* Timer proc for update tasks.  Is called every TIMERINTERVAL sec.
   Calls updateTaskMarkTimeout, which sets executeRequestsPendingCount
   > 0 for timed out tasks.  Also polls CA.  */
static void medmScheduler(XtPointer cd, XtIntervalId *id)
{
  /* KE: the cd is set to the static global periodicTask.  Could just
     as well directly use the global here. */
    PeriodicTask *t = (PeriodicTask *)cd;
    double currentTime = medmTime();

    UNREFERENCED(id);


  /* Poll channel access  */
#ifdef __MONITOR_CA_PEND_EVENT__
    {
	double t;
	t = medmTime();
	ca_pend_event(0.00000001);
	t = medmTime() - t;
	if(t > 0.5) {
	    fprintf(stderr,"medmScheduler : time used by ca_pend_event ="
	      " %8.1f\n", t);
	}
    }
#else
    ca_pend_event(0.00000001);
#endif

  /* Look for displays that have a periodic task which is timed out */
    if(updateTaskStatus.periodicTaskCount > 0) { 
	DisplayInfo *di = displayInfoListHead->next;
	while(di) {
	    if(di->periodicTaskCount > 0) {
	      /* Check the head, which should have the soonest
                 nextExecuteTime of any of the update tasks for this
                 display */
		UpdateTask *pt = &di->updateTaskListHead;
		
		if(pt->nextExecuteTime < currentTime) {
		    updateTaskMarkTimeout(pt,currentTime);
		}
	    }
	    di = di->next;
	}
    }

  /* If needed, install the work proc */
    if((updateTaskStatus.updateRequestQueued > 0) && 
      (!updateTaskStatus.workProcId)) {
	updateTaskStatus.workProcId =
	  XtAppAddWorkProc(appContext,updateTaskWorkProc,&updateTaskStatus);
    }
    
  /* Reinstall the timer proc to be called again in TIMERINTERVAL ms */
    t->id = XtAppAddTimeOut(appContext,TIMERINTERVAL,medmScheduler,cd);
}

double medmTime()
{
    struct timeval tp;
    
#ifdef WIN32
  /* MSVC does not have gettimeofday.  It comes from Exceed and is
   *   defined differently */
    gettimeofday(&tp,0);
#else
    if(gettimeofday(&tp,NULL)) {
	medmPostMsg(1,"medmTime:  Failed to get time\n");
	return 0.;
    }
#endif
    return (double) tp.tv_sec + (double) tp.tv_usec*1e-6;
}

void startMedmScheduler(void)
{
    if(!medmWorkProcId) medmScheduler((XtPointer)&periodicTask, NULL);
}

void stopMedmScheduler(void)
{
    if(periodicTask.id) {
	XtRemoveTimeOut(periodicTask.id);
	periodicTask.id = (XtIntervalId)0;
    }
}

double medmElapsedTime()
{
    return (medmTime()-medmStartTime);
}

double medmResetElapsedTime()
{
    return(medmStartTime=medmTime());
}

/* 
 *  ---------------------------
 *  routines for update tasks
 *  ---------------------------
 */

/* Called when initializing a DisplayInfo for
 *   DisplayInfo->updateTaskListHead only */
void updateTaskInitHead(DisplayInfo *displayInfo)
{
    UpdateTask *pt = &(displayInfo->updateTaskListHead);
    
  /* Initialize the task to the null task except as indicated */
    *pt = nullTask;
    pt->timeInterval = EXECINTERVAL;
    pt->nextExecuteTime = medmTime() + pt->timeInterval;
    pt->displayInfo = displayInfo;
    pt->executeRequestsPendingCount = 0;

  /* Set up the displayInfo */
    displayInfo->updateTaskListTail = pt;
    displayInfo->periodicTaskCount = 0;
}

void medmInitializeUpdateTasks(void)
{
/* Initialize static global variable updateTaskStatus */
    updateTaskStatus.workProcId          = 0;
    updateTaskStatus.nextToServe         = NULL;
    updateTaskStatus.taskCount           = 0;
    updateTaskStatus.periodicTaskCount   = 0;
    updateTaskStatus.updateRequestCount  = 0;
    updateTaskStatus.updateDiscardCount  = 0;
    updateTaskStatus.periodicUpdateRequestCount = 0;
    updateTaskStatus.periodicUpdateDiscardCount = 0;
    updateTaskStatus.updateRequestQueued = 0;
    updateTaskStatus.updateExecuted      = 0;
    updateTaskStatus.since = medmTime();

  /* Initialize the periodic task */
    periodicTask.systemTime = medmStartTime = medmTime();
    periodicTask.tenthSecond = 0.0; 
    periodicTask.id = (XtIntervalId)0; 
    moduleInitialized = True;
}

UpdateTask *updateTaskAddTask(DisplayInfo *displayInfo, DlObject *rectangle,
  void (*executeTask)(XtPointer), XtPointer clientData)
{
    UpdateTask *pT;
    
    if(displayInfo) {
	pT = (UpdateTask *)malloc(sizeof(UpdateTask));
	if(pT == NULL) return pT;
	pT->executeTask = executeTask;
	pT->destroyTask = NULL;
	pT->getRecord = NULL;
	pT->clientData = clientData;
	pT->timeInterval = 0.0;
	pT->nextExecuteTime = medmTime() + pT->timeInterval;
	pT->displayInfo = displayInfo;
	pT->prev = displayInfo->updateTaskListTail;
	pT->next = NULL;
	pT->executeRequestsPendingCount = 0;
	if(rectangle) {
	    pT->rectangle.x      = rectangle->x;
	    pT->rectangle.y      = rectangle->y;
	    pT->rectangle.width  = rectangle->width;
	    pT->rectangle.height = rectangle->height;
	} else {
	    pT->rectangle.x      = 0;
	    pT->rectangle.y      = 0;
	    pT->rectangle.width  = 0;
	    pT->rectangle.height = 0;
	}
	pT->overlapped = True;  /* Default is assumed to be overlapped */
	pT->opaque = True;      /* Default is don't draw the background */
	pT->disabled = False;   /* Default is not disabled */

	displayInfo->updateTaskListTail->next = pT;
	displayInfo->updateTaskListTail = pT;

      /* KE: Should never execute this branch since pT->timeInterval=0.0 */
	if(pT->timeInterval > 0.0) {
	    displayInfo->periodicTaskCount++;
	    updateTaskStatus.periodicTaskCount++;
	    if(pT->nextExecuteTime <
	      displayInfo->updateTaskListHead.nextExecuteTime) {
		displayInfo->updateTaskListHead.nextExecuteTime =
		  pT->nextExecuteTime;
	    }
	}
	updateTaskStatus.taskCount++;
	return pT;
    } else {
	return NULL;
    }
}  

/* Delete all update tasks on the display associated with a given
   task. More efficient than deleting them one by one using
   updateTaskDeleteTask. */
void updateTaskDeleteAllTask(UpdateTask *pT)
{
    UpdateTask *tmp;
    DisplayInfo *displayInfo;
    
    if(pT == NULL) return;
    displayInfo = pT->displayInfo;
    displayInfo->periodicTaskCount = 0;
    tmp = displayInfo->updateTaskListHead.next;
    while(tmp) {
	UpdateTask *tmp1 = tmp;
	tmp = tmp1->next;
	if(tmp1->destroyTask) {
	    tmp1->destroyTask(tmp1->clientData);
	}
	if(tmp1->timeInterval > 0.0) {
	    updateTaskStatus.periodicTaskCount--;
	}
	updateTaskStatus.taskCount--;
	if(tmp1->executeRequestsPendingCount > 0) {
	    updateTaskStatus.updateRequestQueued--;
	}
	if(updateTaskStatus.nextToServe == tmp1) {
	    updateTaskStatus.nextToServe = NULL;
	}
      /* Zero the task before freeing it to help with pointer errors */
	*tmp1 = nullTask;
	free((char *)tmp1);
	tmp1=NULL;
    }
    if((updateTaskStatus.taskCount <=0) && (updateTaskStatus.workProcId)) {
	XtRemoveWorkProc(updateTaskStatus.workProcId);
	updateTaskStatus.workProcId = 0;
    }
    displayInfo->updateTaskListHead.next = NULL;
    displayInfo->updateTaskListTail = &(displayInfo->updateTaskListHead);
#ifdef __MONITOR_CA_PEND_EVENT__
    {
	double t;
	t = medmTime();
	ca_pend_event(CA_PEND_EVENT_TIME);
	t = medmTime() - t;
	if(t > 0.5) {   
	    fprintf(stderr, "updateTaskDeleteAllTask : time used by "
	      "ca_pend_event = %8.1f\n",t);
	} 
    }
#else
    ca_pend_event(CA_PEND_EVENT_TIME);
#endif
}


/* Disable an update task */
void updateTaskDisableTask(DlElement *dlElement)
{
    if(dlElement && dlElement->data) {
	MedmElement *pe = (MedmElement *)dlElement->data;
	UpdateTask *pT = pe->updateTask;

	if(pT) pT->disabled = True;
    }
}

/* Enable an update task */
void updateTaskEnableTask(DlElement *dlElement)
{
    if(dlElement && dlElement->data) {
	MedmElement *pe = (MedmElement *)dlElement->data;
	UpdateTask *pT = pe->updateTask;

	if(pT) pT->disabled = False;
    }
}

int updateTaskMarkTimeout(UpdateTask *pT, double currentTime)
{
    UpdateTask *head = &(pT->displayInfo->updateTaskListHead);
    UpdateTask *tmp = head->next;
    int count = 0;
    
  /* Loop over the update task list and check for timeouts */
    while(tmp) {
      /* Check if periodic task */
	if(tmp->timeInterval > 0.0) {
	  /* Mark if the task is timed out already */
	    if(currentTime > tmp->nextExecuteTime) {
		count++;
		if(tmp->executeRequestsPendingCount > 0) {
		    updateTaskStatus.periodicUpdateDiscardCount++;
		} else {
		    updateTaskStatus.periodicUpdateRequestCount++;
		    updateTaskStatus.updateRequestQueued++;
		}
		tmp->executeRequestsPendingCount++;
		tmp->nextExecuteTime += tmp->timeInterval;
	    }
	}
      /* Set the next execute time for head */
	if(tmp->nextExecuteTime < head->nextExecuteTime) {
	    head->nextExecuteTime = tmp->nextExecuteTime;
	}
	tmp = tmp->next;
    }
    return count;
}   

void updateTaskMarkUpdate(UpdateTask *pT)
{
    if(pT->executeRequestsPendingCount > 0) {
	updateTaskStatus.updateDiscardCount++;
    } else {
	updateTaskStatus.updateRequestCount++;
	updateTaskStatus.updateRequestQueued++;
    }
    pT->executeRequestsPendingCount++;
}

void updateTaskSetScanRate(UpdateTask *pT, double timeInterval)
{
    UpdateTask *head = &(pT->displayInfo->updateTaskListHead);
    double currentTime = medmTime();

  /* timeInterval is in seconds */
    
  /* Whether to increase or decrease the periodic task count depends
   * on how timeInterval changes */
    if((pT->timeInterval == 0.0) && (timeInterval != 0.0)) {
      /* was zero, now non-zero */
	pT->displayInfo->periodicTaskCount++;
	updateTaskStatus.periodicTaskCount++;
    } else if((pT->timeInterval != 0.0) && (timeInterval == 0.0)) {
      /* was non-zero, now zero */
	pT->displayInfo->periodicTaskCount--;
	updateTaskStatus.periodicTaskCount--;
    }

  /* Set up the next scan time for this task, if this is the sooner
   * one, set it to the display scan time too */
    pT->timeInterval = timeInterval;
    pT->nextExecuteTime = currentTime + pT->timeInterval;
    if(pT->nextExecuteTime < head->nextExecuteTime) {
	head->nextExecuteTime = pT->nextExecuteTime;
    }
}

void updateTaskAddExecuteCb(UpdateTask *pT, void (*executeTaskCb)(XtPointer))
{
    pT->executeTask = executeTaskCb;
}

void updateTaskAddDestroyCb(UpdateTask *pT, void (*destroyTaskCb)(XtPointer))
{
    pT->destroyTask = destroyTaskCb;
}

/* Work proc for updateTask.  Is called when medm is not busy.  Checks
   for enabled update tasks with executeRequestspendingCount > 0 and
   executes them. */
static Boolean updateTaskWorkProc(XtPointer cd)
{
    DisplayInfo *displayInfo;
    UpdateTaskStatus *ts = (UpdateTaskStatus *)cd;
    UpdateTask *t = ts->nextToServe;
    UpdateTask *t1;
    UpdateTask *tStart;
    double endTime;
    XPoint points[4];
    Region region;
    DlElement *pE;
    int pass;
   
    endTime = medmTime() + WORKINTERVAL; 
 
  /* Do for WORKINTERVAL sec */
    do {
      /* If no requests queued, remove work proc */
	if(ts->updateRequestQueued <= 0) {
	    ts->workProcId = 0; 
	    return True;
	} 
      /* If no valid update task, find one */
	if(t == NULL) {
	    displayInfo = displayInfoListHead->next;
	    
	  /* If no display, remove work proc */
	    if(displayInfo == NULL) {
		ts->workProcId = 0;
		return True;
	    }
	  /* Loop over displays to find an update task */
	    while(displayInfo) {
		if(t = displayInfo->updateTaskListHead.next) break;
		displayInfo = displayInfo->next;
	    }
	  /* If no update task found, remove work proc */
	    if(t == NULL) {
		ts->workProcId = 0;
		return True;
	    }
	    ts->nextToServe = t;
	}
	
      /* At least one update task has been found.  Find one which has
	 is enabled and has executeRequestsPendingCount > 0 */
	tStart = t;
	pass = 0;
	while(t->executeRequestsPendingCount <= 0 || t->disabled) {
	    displayInfo = t->displayInfo;
	    t = t->next;
	    while(t == NULL) {
	      /* End of the update tasks for this display, check the
		 next display */
		displayInfo = displayInfo->next;
	      /* If at the end of the displays, go to the beginning */
		if(displayInfo == NULL) {
		    if(++pass > 1) {
		      /* We already tried this */
			ts->workProcId = 0;
			return True;
		    }
		    displayInfo = displayInfoListHead->next;
		}
		t = displayInfo->updateTaskListHead.next;
	    }      
	  /* If t is NULL or found same t again, have checked all
	     displays and there is nothing to do.  Remove work proc */
	    if(t == tStart) {
		ts->workProcId = 0;
		return True;
	    }
	}
      /* An enabled update task with executeRequestsPendingCount > 0
         has been found.  Set it to be the next to serve.  */
	ts->nextToServe = t;
      /* Get the displayInfo */
	displayInfo = t->displayInfo;

      /* Set the clip region */
	pE = getElementFromUpdateTask(t);
	
	points[0].x = t->rectangle.x;
	points[0].y = t->rectangle.y;
	points[1].x = t->rectangle.x + t->rectangle.width;
	points[1].y = t->rectangle.y;
	points[2].x = t->rectangle.x + t->rectangle.width;
	points[2].y = t->rectangle.y + t->rectangle.height;
	points[3].x = t->rectangle.x;
	points[3].y = t->rectangle.y + t->rectangle.height;
	region = XPolygonRegion(points,4,EvenOddRule);
	
	if(region == NULL) {
	    medmPrintf(0,"\nupdateTaskWorkProc: XPolygonRegion is NULL\n");
	  /* Kill the work proc */
	    ts->workProcId = 0;
	    return True;
	}
	
      /* Set the clip rectangle.  Since there is only one, whether
	 it is XYBanded or the alternatives isn't important. */
	XSetClipRectangles(display, displayInfo->gc,
	  0, 0, &t->rectangle, 1, YXBanded);
	XSetClipRectangles(display, displayInfo->pixmapGC,
	  0, 0, &t->rectangle, 1, YXBanded);
	
      /* Repaint the selected region */
	if(t->overlapped) {
	    int isComposite = 0;
	    
	  /* Check if this task is for a composite.  Composites need
             to be handled differently. */
	    if(pE && pE->type == DL_Composite) {
		isComposite = 1;
	    }
	    
	  /* Branch on if opaque or not */
	    if(!t->opaque)
	    /* For a composite we need to redo the pixmap in case
	       elements are hidden or unhidden when the composite is
	       executed. This will be inefficient if the Composite gets
	       a lot of updates that don't change its visibility */
	      if(isComposite) {
		/* Redraw all the static elements on the pixmap */
		  redrawStaticElements(displayInfo, pE);
	      }
	    
	  /* Copy the pixmap to the (clipped) drawingArea */
	    XCopyArea(display,displayInfo->drawingAreaPixmap,
	      XtWindow(displayInfo->drawingArea), displayInfo->gc,
	      t->rectangle.x, t->rectangle.y,
	      t->rectangle.width, t->rectangle.height,
	      t->rectangle.x, t->rectangle.y);
	    
	  /* Set overlapped to false.  This will override the
	     default of True.  It will be reset to True in the loop
	     below if it truly is overlapped. */
	    t->overlapped = False;
	    
	    t1 = t->displayInfo->updateTaskListHead.next;
	    while(t1) {
		if(!t1->disabled && XRectInRegion(region,
		  t1->rectangle.x, t1->rectangle.y,
		  t1->rectangle.width, t1->rectangle.height) != RectangleOut) {
		    t1->overlapped = True;
		    if(t1->executeTask) {
			t1->executeTask(t1->clientData);
		    }
		}
		t1 = t1->next;
	    }
	    
	} else {
	  /* Not overlapped */
	    if(!t->opaque) 
	      XCopyArea(display,t->displayInfo->drawingAreaPixmap,
		XtWindow(t->displayInfo->drawingArea),
		t->displayInfo->gc,
		t->rectangle.x, t->rectangle.y,
		t->rectangle.width, t->rectangle.height,
		t->rectangle.x, t->rectangle.y);
	    if(!t->disabled && t->executeTask) {
		t->executeTask(t->clientData);
	    }
	}
	
      /* Release the clipping region */
	XSetClipOrigin(display, displayInfo->gc, 0, 0);
	XSetClipMask(display, displayInfo->gc, None);
	XSetClipOrigin(display, displayInfo->pixmapGC, 0, 0);
	XSetClipMask(display, displayInfo->pixmapGC, None);
	XDestroyRegion(region);
	
      /* Update ts->updateExecuted since we executed it, but not
         executeRequestsPendingCount, since if the task was deleted,
         this will have been done in updateTaskDeleteTask. */
	ts->updateExecuted++;
	
      /* Reset the executeRequestsPendingCount only if t is still valid */
	t->executeRequestsPendingCount = 0;
	ts->updateRequestQueued--;
	
      /* Find the next one */
	tStart = t;
	pass = 0;
	while(t->executeRequestsPendingCount <= 0 ) {
	    DisplayInfo *displayInfo = t->displayInfo;
	    t = t->next;
	    while(t == NULL) {
	      /* End of the update tasks for this display, check the
		 next display */
		displayInfo = displayInfo->next;
		if(displayInfo == NULL) {
		    if(++pass > 1) {
		      /* We already tried this */
			ts->workProcId = 0;
			return True;
		    }
		    displayInfo = displayInfoListHead->next;
		}
		t = displayInfo->updateTaskListHead.next;
	    }
	  /* If found same t again, have now checked all displays and
	     there is nothing to do.  Remove work proc */
	    if(t == tStart) {
		ts->workProcId = 0;
		return True;
	    }
	}
	ts->nextToServe = t;
    } while(endTime > medmTime());
    
  /* Keep the work proc active */
    return False;
}

void updateTaskRepaintRegion(DisplayInfo *displayInfo, Region *region)
{
    UpdateTask *t = displayInfo->updateTaskListHead.next;

  /* Do executeTask for each updateTask in the region for this display */
    while(t) {
	if(!t->disabled && XRectInRegion(*region,
	  t->rectangle.x, t->rectangle.y,
	  t->rectangle.width, t->rectangle.height) != RectangleOut) {
	    if(t->executeTask) {
		t->executeTask(t->clientData);
	    }
	}
	t = t->next;
    }
}

void updateTaskAddNameCb(UpdateTask *pT, void (*nameCb)(XtPointer,
  Record **, int *))
{
    pT->getRecord = nameCb;
}

/*
 * return Channel ptr given a widget id
 */
UpdateTask *getUpdateTaskFromWidget(Widget widget)
{
    DisplayInfo *displayInfo;
    UpdateTask *pT;

    if(!(displayInfo = dmGetDisplayInfoFromWidget(widget)))
      return NULL; 

    pT = displayInfo->updateTaskListHead.next; 
    while(pT) {
      /* Note : vong
       * Below it is a very ugly way to dereference the widget pointer.
       * It assumes that the first element in the clientData is a pointer
       * to a DlElement structure.  However, if a SIGSEG or SIGBUS occurs,
       * please recheck the structure which pT->clientData points
       * at.
       */
	if((*(((DlElement **)pT->clientData)))->widget == widget) {
	    return pT;
	}
	pT = pT->next;
    }
    return NULL;
}

/* Return pointer to UpdateTask given a DisplayInfo* and x,y positions */
UpdateTask *getUpdateTaskFromPosition(DisplayInfo *displayInfo, int x, int y)
{
    UpdateTask *ptu, *ptuSaved = NULL;
    int minWidth, minHeight;
  
    if(displayInfo == (DisplayInfo *)NULL) return NULL;

    minWidth = INT_MAX;	 	/* according to XPG2's values.h */
    minHeight = INT_MAX;

    ptu = displayInfo->updateTaskListHead.next;
    while(ptu) {
	if(x >= (int)ptu->rectangle.x &&
	  x <= (int)ptu->rectangle.x + (int)ptu->rectangle.width &&
	  y >= (int)ptu->rectangle.y &&
	  y <= (int)ptu->rectangle.y + (int)ptu->rectangle.height) {
	  /* Eligible element, see if smallest so far */
	    if((int)ptu->rectangle.width < minWidth &&
	      (int)ptu->rectangle.height < minHeight) {
		minWidth = (int)ptu->rectangle.width;
		minHeight = (int)ptu->rectangle.height;
		ptuSaved = ptu;
	    }
	}
	ptu = ptu->next;
    }
    return ptuSaved;
}

UpdateTask *getUpdateTaskFromElement(DlElement *dlElement)
{
    MedmElement *element;

    if(!dlElement || !dlElement->data) return NULL;
    element = (MedmElement *)dlElement->data;
    return(element->updateTask);
}

static DlElement *getElementFromUpdateTask(UpdateTask *t)
{
    if(t && t->clientData) {
	MedmElement *element = (MedmElement *)t->clientData;
	
	return element->dlElement;
    }
    return NULL;
}

void dumpUpdatetaskList(DisplayInfo *displayInfo)
{
    UpdateTask *pT;
    int i=0;
    
    pT = displayInfo->updateTaskListHead.next; 
    while(pT) {
	MedmElement *pElement=(MedmElement *)pT->clientData;
	DlElement *pE = pElement->dlElement;
	DlComposite *pC = pE->structure.composite;
	DlObject *pO = &(pC->object);
	
	print("  %2d pT=%x pE=%x pC=%x x=%3d y=%3d %s%s\n",
	  ++i,pT,pE,pC,
	  pO->x, pO->y,
	  elementType(pE->type),
	  pT->disabled?" D":"");
	
	pT = pT->next;
    }
}
