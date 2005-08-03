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

#define DEBUG_INFO 0
#define DEBUG_MARK_UPDATE 0
#define DEBUG_COMPOSITE 0
#define DEBUG_ENABLE 0
#define DEBUG_SCHEDULER 0
#define DEBUG_RETURN 0
#define DEBUG_LOOP 0
#define DEBUG_REPAINT 0
#define DEBUG_UPDATE_STATE 0
#define DEBUG_MEM 0
#define DEBUG_REDRAW 0

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
static void updateTaskRedrawPixmap(DisplayInfo *displayInfo,
  Region region);
static void updateTaskCompositeRedrawPixmap(DisplayInfo *displayInfo,
  DlComposite *dlComposite, Region region);

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
    UNDEFINED,
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

#if DEBUG_INFO
    print("updateTaskStatusGetInfo: RC=%d DC=%d URC=%d UDC=%d UE=%d TI=%.2f\n",
    updateTaskStatus.updateRequestCount,
    updateTaskStatus.updateDiscardCount,
    updateTaskStatus.periodicUpdateRequestCount,
    updateTaskStatus.periodicUpdateDiscardCount,
    updateTaskStatus.updateExecuted,
    time-updateTaskStatus.since);
#endif

  /* If some of these are not set, just do a reset */
    if(periodicTaskCount && updateRequestCount && updateDiscardCount &&
      periodicUpdateRequestCount &&  periodicUpdateDiscardCount &&
      updateRequestQueued && updateExecuted && timeInterval) {
	*taskCount = updateTaskStatus.taskCount;
	*periodicTaskCount = updateTaskStatus.periodicTaskCount;
	*updateRequestCount = updateTaskStatus.updateRequestCount;
	*updateDiscardCount = updateTaskStatus.updateDiscardCount;
	*periodicUpdateRequestCount = updateTaskStatus.periodicUpdateRequestCount;
	*periodicUpdateDiscardCount = updateTaskStatus.periodicUpdateDiscardCount;
	*updateRequestQueued = updateTaskStatus.updateRequestQueued;
	*updateExecuted = updateTaskStatus.updateExecuted;
	*timeInterval = time - updateTaskStatus.since;
    }

  /* Reset the periodic data */
    updateTaskStatus.updateRequestCount = 0;
    updateTaskStatus.updateDiscardCount = 0;
    updateTaskStatus.periodicUpdateRequestCount = 0;
    updateTaskStatus.periodicUpdateDiscardCount = 0;
    updateTaskStatus.updateExecuted = 0;
    updateTaskStatus.since = time;

#if DEBUG_INFO
    print("                    End: RC=%d DC=%d URC=%d UDC=%d UE=%d TI=%.2f\n",
    updateTaskStatus.updateRequestCount,
    updateTaskStatus.updateDiscardCount,
    updateTaskStatus.periodicUpdateRequestCount,
    updateTaskStatus.periodicUpdateDiscardCount,
    updateTaskStatus.updateExecuted,
    time-updateTaskStatus.since);
#endif
}

/* Timer proc for update tasks.  Is called every TIMERINTERVAL sec.
   Calls updateTaskMarkTimeout, which sets executeRequestsPendingCount
   > 0 for timed out tasks.  Also polls CA.  */
static void medmScheduler(XtPointer clientData, XtIntervalId *id)
{
  /* KE: the clientData is set to the static global periodicTask.
     Could just as well directly use the global here. */
    PeriodicTask *t = (PeriodicTask *)clientData;
    double currentTime = medmTime();

    UNREFERENCED(id);

#if DEBUG_SCHEDULER
    print("medmScheduler: workProcId=%x updateRequestQueued=%d\n",
      updateTaskStatus.workProcId,updateTaskStatus.updateRequestQueued);
#endif

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
    if(updateTaskStatus.updateRequestQueued > 0 &&
      !updateTaskStatus.workProcId) {
	updateTaskStatus.workProcId =
	  XtAppAddWorkProc(appContext,updateTaskWorkProc,&updateTaskStatus);
    }

  /* Reinstall the timer proc to be called again in TIMERINTERVAL ms */
    t->id = XtAppAddTimeOut(appContext,TIMERINTERVAL,medmScheduler,clientData);
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
  /* Remove any existing work procs */
    stopMedmScheduler();
    if(medmWorkProcId) {
	XtRemoveWorkProc(medmWorkProcId);
	medmWorkProcId = 0;
    }

  /* Reset values */
    medmInitializeUpdateTasks();

  /* Start the scheduler */
    medmScheduler((XtPointer)&periodicTask, NULL);
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

    updateInProgress = False;

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
	pT->overlapType = UNDEFINED;
	pT->disabled = False;        /* Default is not disabled */

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

/* Delete all update tasks on the display associated with a given task */
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
    if((updateTaskStatus.taskCount <= 0) && (updateTaskStatus.workProcId)) {
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
#if DEBUG_ENABLE
    print("updateTaskDisableTask:  dlElement=%x %s\n",
      dlElement,elementType(dlElement->type));
#endif
    if(dlElement && dlElement->data) {
	MedmElement *pe = (MedmElement *)dlElement->data;
	UpdateTask *pT = pe->updateTask;

	if(pT && !pT->disabled) {
#if DEBUG_ENABLE
	    print("  Before: executeRequestsPendingCount=%d\n",
	      pT->executeRequestsPendingCount);
#endif
	    pT->disabled = True;
	    if(pT->executeRequestsPendingCount) {
		pT->executeRequestsPendingCount = 0;
	      /* (do not decrement updateRequestCount)*/
		updateTaskStatus.updateRequestQueued--;
	    }
#if DEBUG_ENABLE
	    print("  After:  executeRequestsPendingCount=%d\n",
	      pT->executeRequestsPendingCount);
#endif
	}
    }
}

/* Enable an update task */
void updateTaskEnableTask(DlElement *dlElement)
{
#if DEBUG_ENABLE
	print("updateTaskEnableTask:  dlElement=%x %s\n",
	  dlElement,elementType(dlElement->type));
#endif
    if(dlElement && dlElement->data) {
	MedmElement *pe = (MedmElement *)dlElement->data;
	UpdateTask *pT = pe->updateTask;

#if DEBUG_MEM
	print("updateTaskEnableTask: dlElement=%x dlElement->data=%x"
	  " [%s]\n",
	  dlElement, dlElement->data, elementType(dlElement->type));
	print("  dlElement = %x updateTask=%x\n",
	  pe->dlElement, pe->updateTask);
#endif
      /* Don't do anything if it is already enabled */
	if(pT && pT->disabled) {
#if DEBUG_ENABLE
	    print("  Before: executeRequestsPendingCount=%d\n",
	      pT->executeRequestsPendingCount);
#endif
	    pT->disabled = False;
	  /* Set it to update if it is not already */
	    if(pT->executeRequestsPendingCount <= 0) {
		pT->executeRequestsPendingCount = 1;
		updateTaskStatus.updateRequestCount++;
		updateTaskStatus.updateRequestQueued++;
	    }
#if DEBUG_ENABLE
	    print("  After:  executeRequestsPendingCount=%d\n",
	      pT->executeRequestsPendingCount);
#endif
	}
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
    if(pT->disabled) return;

#if DEBUG_MARK_UPDATE
    {
	MedmElement *pElement=(MedmElement *)pT->clientData;
	DlElement *pET = pElement->dlElement;

	print("Marking pT=%x pE=%x"
	  " pE->type=%s x=%d y=%d pend=%d\n",
	  pT,pET,elementType(pET->type),
	  pT->rectangle.x,pT->rectangle.y,
	  pT->executeRequestsPendingCount);
    }
#endif
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
    UpdateTask *t1, *tStart;
    double endTime;
    Region region;
    DlElement *pE, *pE1;
    OverlapType newOverlapType;
    XRectangle clipBox;
    int pass;

  /* Define the ending time for this proc */
    endTime = medmTime() + WORKINTERVAL;

#if DEBUG_LOOP
    print("updateTaskWorkProc: nextToServe=%x\n",ts->nextToServe);
#endif
#if DEBUG_COMPOSITE
    print("\nupdateTaskWorkProc: time=%.3f requestsQueued=%d\n",
      medmElapsedTime(),ts->updateRequestQueued);
#if 0
    {
	UpdateTask *pT;
	int i=0;

	displayInfo = currentDisplayInfo;
#if 0
	dumpDlElementList(displayInfo->dlElementList);
#endif

	pT = displayInfo->updateTaskListHead.next;
	while(pT) {
	    MedmElement *pElement=(MedmElement *)pT->clientData;
	    DlElement *pET = pElement->dlElement;

	    print("  %2d pT=%x pE=%x pE->type=%s [%d pending]\n",
	      ++i,pT,pET,elementType(pET->type),
	      pT->executeRequestsPendingCount);

	    pT = pT->next;
	}
    }
#endif
#endif
#if DEBUG_INFO
    print("updateTaskWorkProc: RC=%d DC=%d URC=%d UDC=%d UE=%d\n",
    updateTaskStatus.updateRequestCount,
    updateTaskStatus.updateDiscardCount,
    updateTaskStatus.periodicUpdateRequestCount,
    updateTaskStatus.periodicUpdateDiscardCount,
    updateTaskStatus.updateExecuted);
#endif

  /* Do for WORKINTERVAL sec */
    do {
      /* If no requests queued, remove work proc */
	if(ts->updateRequestQueued <= 0) {
	    ts->workProcId = 0;
#if DEBUG_RETURN
	    print("Return 1\n");
#endif
	    return True;
	}
      /* If no valid update task, find one */
	if(t == NULL) {
	    displayInfo = displayInfoListHead->next;

	  /* If no display, remove work proc */
	    if(displayInfo == NULL) {
		ts->workProcId = 0;
#if DEBUG_RETURN
		print("Return 2\n");
#endif
		return True;
	    }
	  /* Loop over displays to find an update task */
	    while(displayInfo) {
		t = displayInfo->updateTaskListHead.next;
		if(t) break;
		displayInfo = displayInfo->next;
	    }
	  /* If no update task found, remove work proc */
	    if(t == NULL) {
		ts->workProcId = 0;
#if DEBUG_RETURN
		print("Return 3\n");
#endif
		return True;
	    }
	    ts->nextToServe = t;
	}

      /* At least one update task has been found.  Find one which is
	 enabled and has executeRequestsPendingCount > 0 */
	tStart = t;
	pass = 0;
	while(t->executeRequestsPendingCount <= 0 || t->disabled) {
	    displayInfo = t->displayInfo;
	    t = t->next;
	    while(t == NULL) {
	      /* End of the update tasks for this display, check the
		 next display */
		displayInfo = displayInfo->next;
		if(displayInfo == NULL) {
		    if(++pass > 1) {
		      /* We already tried this.  tStart must be invalid */
			ts->workProcId = 0;
#if DEBUG_RETURN
			print("Continue 4\n");
#endif
			ts->nextToServe = NULL;
			goto END;
		    }
		    displayInfo = displayInfoListHead->next;
		}
		t = displayInfo->updateTaskListHead.next;
	    }
	  /* If t is NULL or found same t again, have checked all
	     displays and there is nothing to do.  Remove work proc */
	    if(t == tStart) {
		ts->workProcId = 0;
#if DEBUG_RETURN
		print("Return 5\n");
#endif
		return True;
	    }
	}

#if DEBUG_COMPOSITE
	{
	    MedmElement *pElement=(MedmElement *)t->clientData;
	    DlElement *pET = pElement->dlElement;

	    print("Updating pT=%x pE=%x"
	      " pE->type=%s x=%d y=%d\n",
	      t,pET,elementType(pET->type),
	      t->rectangle.x,t->rectangle.y);
	}
#endif

      /* An enabled update task with executeRequestsPendingCount > 0
         has been found.  Set it to be the next to serve.  */
	ts->nextToServe = t;
      /* Get the displayInfo */
	displayInfo = t->displayInfo;
	pE = getElementFromUpdateTask(t);

      /* Set the initial clip region */
	region = XCreateRegion();
	if(region == NULL) {
	    medmPrintf(0,"\nupdateTaskWorkProc: Cannot create clip region\n");
	  /* Kill the work proc */
	    ts->workProcId = 0;
#if DEBUG_RETURN
	    print("Return 6\n");
#endif
	    return True;
	}
	XUnionRectWithRegion(&t->rectangle, region, region);

      /* Determine the overlapType if not done.  Assumes no new tasks
         will be created */
	if(t->overlapType == UNDEFINED) {
	    newOverlapType = NO_OVERLAP;
	    t1 = t->displayInfo->updateTaskListHead.next;
	    while(t1) {
		pE1 = getElementFromUpdateTask(t1);
		if(pE1->updateType != WIDGET && t1 != t) {
		    int status = XRectInRegion(region,
		      t1->rectangle.x, t1->rectangle.y,
		      t1->rectangle.width, t1->rectangle.height);
		    switch(status) {
		    case RectangleIn:
		    case RectanglePart:
			if(newOverlapType < OVERLAP) newOverlapType = OVERLAP;
			break;
		    }
		    if(newOverlapType == OVERLAP) break;
		}
		t1 = t1->next;
	    }
	    t->overlapType = newOverlapType;
	}

      /* Set the clip region in the GC */
	XSetRegion(display, displayInfo->gc, region);
	XClipBox(region, &clipBox);

      /* Copy the drawingAreaPixmap containing the static graphics to
	 the updatePixmap */
	XCopyArea(display,displayInfo->drawingAreaPixmap,
	  displayInfo->updatePixmap, displayInfo->gc,
	  clipBox.x, clipBox.y,
	  clipBox.width, clipBox.height,
	  clipBox.x, clipBox.y);

      /* Empty the updateTaskExposedRegion */
	if(!XEmptyRegion(updateTaskExposedRegion)) {
	    XDestroyRegion(updateTaskExposedRegion);
	    updateTaskExposedRegion = XCreateRegion();
	}

      /* Repaint the selected region */
	updateInProgress = True;
	switch(t->overlapType) {
	case NO_OVERLAP:
	    if(!t->disabled && t->executeTask) {
		t->executeTask(t->clientData);
	    }
	    break;
	case OVERLAP:
	  /* Loop over all tasks in region */
#if DEBUG_COMPOSITE
	    print("Before loop over all tasks in region\n");
#endif
	    t1 = t->displayInfo->updateTaskListHead.next;
	    while(t1) {
		if(!t1->disabled && XRectInRegion(region,
		  t1->rectangle.x, t1->rectangle.y,
		  t1->rectangle.width, t1->rectangle.height) != RectangleOut) {
		    if(t1->executeTask) {
#if DEBUG_COMPOSITE
			{
			    MedmElement *pElement=(MedmElement *)t1->clientData;
			    DlElement *pET = pElement->dlElement;

			    print("  Updating: pT=%x pE=%x"
			      " pE->type=%s x=%d y=%d pend=%d\n",
			      t1,pET,elementType(pET->type),
			      t1->rectangle.x,t1->rectangle.y,
			      t1->executeRequestsPendingCount);
#if 0
			    pE1 = getElementFromUpdateTask(t1);
			    print(" Executing task: %s\n",
			      elementType(pE1->type));
#endif
			}
#endif
			t1->executeTask(t1->clientData);
#if 0
		      /* Graphics will be done again as the primary
                         task to get their whole region, but widgets
                         don't have to be done again */
			if(t1 != t) {
			    pE1 = getElementFromUpdateTask(t1);
			    if(pE1->updateType == WIDGET) {
				ts->updateExecuted++;
				t1->executeRequestsPendingCount = 0;
				ts->updateRequestQueued--;
			    }
			}
#endif
		    }
		}
		t1 = t1->next;
	    }
	    break;
	case UNDEFINED:
	    break;
	}
	updateInProgress = False;

#if DEBUG_COMPOSITE
	print("After loop over all tasks in region\n");
#endif

      /* If the primary element is a composite, add the region to the
         updateTaskExposedRegion, since we do not know what might have
         changed in the drawingAreaPixmap */
	if(pE->type == DL_Composite) {
	    CompositeUpdateState updateState = getCompositeUpdateState(pE);
#if DEBUG_UPDATE_STATE
	    {
		MedmElement *pElement=(MedmElement *)t->clientData;
		DlElement *pET = pElement->dlElement;

		print("Updating pT=%x pE=%x"
		  " pE->type=%s x=%d y=%d\n"
		  "  updateState=%d\n",
		  t,pET,elementType(pET->type),
		  t->rectangle.x,t->rectangle.y,updateState);
	    }
#endif
	  /* Don't do anything if the element is already hidden and
	     updated */
	    if(updateState != COMPOSITE_HIDDEN_UPDATED) {
		XUnionRegion(region, updateTaskExposedRegion,
		  updateTaskExposedRegion);
	      /* If hidden, promote the updateState to hidden, updated */
		if(updateState == COMPOSITE_HIDDEN) {
		    setCompositeUpdateState(pE, COMPOSITE_HIDDEN_UPDATED);
		}
	    }
	}

      /* Check the updateTaskExposedRegion */
	if(!XEmptyRegion(updateTaskExposedRegion)) {
	  /* Set the updateTaskExposedRegion in the GC */
	    XSetRegion(display, displayInfo->gc, updateTaskExposedRegion);

	  /* Redraw the pixmap */
#if DEBUG_COMPOSITE
	    print("Before redraw pixmap (XEmptyRegion is False)\n");
#endif
	    updateTaskRedrawPixmap(displayInfo, updateTaskExposedRegion);

	  /* Repaint the region */
	    updateTaskRepaintRegion(displayInfo, updateTaskExposedRegion);

	  /* Restore the original region in the GC */
	    XSetRegion(display, displayInfo->gc, region);

	  /* Empty the updateTaskExposedRegion */
	    XDestroyRegion(updateTaskExposedRegion);
	    updateTaskExposedRegion = XCreateRegion();
#if DEBUG_COMPOSITE
	    print("After redraw pixmap\n");
#endif
	}

      /* Copy the updatePixmap to the window */
	XCopyArea(display,displayInfo->updatePixmap,
	  XtWindow(t->displayInfo->drawingArea), displayInfo->gc,
	  clipBox.x, clipBox.y,
	  clipBox.width, clipBox.height,
	  clipBox.x, clipBox.y);

      /* Release the clipping region */
	XSetClipOrigin(display, displayInfo->gc, 0, 0);
	XSetClipMask(display, displayInfo->gc, None);
	XDestroyRegion(region);

      /* Reset the executeRequestsPendingCount */
	if(!t->disabled) {
	    t->executeRequestsPendingCount = 0;
	    ts->updateExecuted++;
	    ts->updateRequestQueued--;
	}

#if DEBUG_COMPOSITE
	print("Update Done updateRequestQueued=%d updateExecuted=%d\n",
	  ts->updateRequestQueued,ts->updateExecuted);
#if 0
    {
	UpdateTask *pT;
	int i=0;

	displayInfo = currentDisplayInfo;
#if 0
	dumpDlElementList(displayInfo->dlElementList);
#endif

	pT = displayInfo->updateTaskListHead.next;
	while(pT) {
	    MedmElement *pElement=(MedmElement *)pT->clientData;
	    DlElement *pET = pElement->dlElement;

	    print("  %2d pT=%x pE=%x pE->type=%s [%d pending]\n",
	      ++i,pT,pET,elementType(pET->type),
	      pT->executeRequestsPendingCount);

	    pT = pT->next;
	}
    }
#endif
#endif

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
		      /* We already tried this.  tStart must be invalid */
			ts->workProcId = 0;
#if DEBUG_RETURN
			print("Continue 7\n");
#endif
			ts->nextToServe = NULL;
			goto END;
		    }
		    displayInfo = displayInfoListHead->next;
		}
		t = displayInfo->updateTaskListHead.next;
	    }
	  /* If found same t again, have now checked all displays and
	     there is nothing to do.  Remove work proc */
	    if(t == tStart) {
		ts->workProcId = 0;
#if DEBUG_RETURN
#if 0
		{
		    UpdateTask *pT;
		    int i=0;

		    displayInfo = currentDisplayInfo;
		    pT = displayInfo->updateTaskListHead.next;
		    while(pT) {
			MedmElement *pElement=(MedmElement *)pT->clientData;
			DlElement *pET = pElement->dlElement;

			print("  %2d pT=%x pE=%x pE->type=%s [%d pending]\n",
			  ++i,pT,pET,elementType(pET->type),
			  pT->executeRequestsPendingCount);

			pT = pT->next;
		    }
		}
#endif
		print("Return 8: time=%.3f requestsQueued=%d\n",
		  medmElapsedTime(),ts->updateRequestQueued);
#endif
		return True;
	    }
	}
	ts->nextToServe = t;
      END:;
    } while(endTime > medmTime());

  /* Keep the work proc active */
#if DEBUG_INFO
    print("updateTaskWorkProc: RC=%d DC=%d URC=%d UDC=%d UE=%d\n",
    updateTaskStatus.updateRequestCount,
    updateTaskStatus.updateDiscardCount,
    updateTaskStatus.periodicUpdateRequestCount,
    updateTaskStatus.periodicUpdateDiscardCount,
    updateTaskStatus.updateExecuted);
#endif
#if DEBUG_RETURN
    print("Return 9 False\n");
#endif
    return False;
}

/* Repaints a rectangle.  Takes care of setting the rectangle into the
   GC. If the rectangle is NULL, it repaints the whole display. Used
   for exposures and refresh.  */
void updateTaskRepaintRect(DisplayInfo *displayInfo, XRectangle *clipRect,
  Boolean redrawPixmap)
{
    UpdateTask *t = displayInfo->updateTaskListHead.next;
    Region region = (Region)0;
    XRectangle usedRect;

#if DEBUG_REPAINT
    print("updateTaskRepaintRect: clipRect=%x redrawPixmap=%s\n",
      clipRect,redrawPixmap?"Yes":"No");
#endif

  /* Create the region */
    region = XCreateRegion();

  /* Add a rectangle to the region */
    if(clipRect) {
	usedRect = *clipRect;
	if(region == NULL) {
	    medmPostMsg(0,"updateTaskRepaintRegion: Cannot create clip region\n");
	    return;
	}
	XUnionRectWithRegion(clipRect, region, region);
    } else {
	DlElement *pE = FirstDlElement(displayInfo->dlElementList);
	DlObject *po = &(pE->structure.display->object);

	usedRect.x = 0;
	usedRect.y = 0;
	usedRect.width = po->width;
	usedRect.height = po->height;
	XUnionRectWithRegion(&usedRect, region, region);
    }

  /* Clip the GC */
    XSetClipRectangles(display, displayInfo->gc,
      0, 0, &usedRect, 1, YXBanded);

#if DEBUG_REPAINT
    print(" {%3d,%3d}{%3d %3d}\n",
      usedRect.x,usedRect.x+usedRect.width,
      usedRect.y,usedRect.y+usedRect.height);
#endif

  /* Redraw the pixmap */
    if(redrawPixmap) {
	updateTaskRedrawPixmap(displayInfo, region);
    }

  /* Copy the drawingAreaPixmap to the updatePixmap */
    XCopyArea(display, displayInfo->drawingAreaPixmap,
      displayInfo->updatePixmap, displayInfo->gc,
      usedRect.x, usedRect.y, usedRect.width, usedRect.height,
      usedRect.x, usedRect.y);

  /* Do executeTask for each updateTask in the region for this display */
    updateInProgress = True;
    t = displayInfo->updateTaskListHead.next;
    if(clipRect) {
      /* Clipping */
	while(t) {
	    if(!t->disabled && XRectInRegion(region,
	      t->rectangle.x, t->rectangle.y,
	      t->rectangle.width, t->rectangle.height) != RectangleOut) {
		if(t->executeTask) {
		    t->executeTask(t->clientData);
		}
	    }
	    t = t->next;
	}
    } else {
      /* No clipping */
	while(t) {
	    if(!t->disabled) {
		if(t->executeTask) {
		    t->executeTask(t->clientData);
		}
	    }
	    t = t->next;
	}
    }
    updateInProgress = False;

  /* Copy the updatePixmap to the window */
    XCopyArea(display, displayInfo->updatePixmap,
      XtWindow(displayInfo->drawingArea), displayInfo->gc,
      usedRect.x, usedRect.y, usedRect.width, usedRect.height,
      usedRect.x, usedRect.y);

  /* Release the clipping region */
    if(clipRect) {
	XSetClipOrigin(display, displayInfo->gc, 0, 0);
	XSetClipMask(display, displayInfo->gc, None);
    }
    if(region) XDestroyRegion(region);
}

/* Repaints a region.  The region should be set in the GC before. */
void updateTaskRepaintRegion(DisplayInfo *displayInfo, Region region)
{
    UpdateTask *t = displayInfo->updateTaskListHead.next;
    XRectangle clipRect;

  /* Determine the bounding rectangle */
    XClipBox(region, &clipRect);

  /* Copy the drawingAreaPixmap to the updatePixmap */
    XCopyArea(display, displayInfo->drawingAreaPixmap,
      displayInfo->updatePixmap, displayInfo->gc,
      clipRect.x, clipRect.y, clipRect.width, clipRect.height,
      clipRect.x, clipRect.y);

  /* Do executeTask for each updateTask in the region for this display */
    updateInProgress = True;
    t = displayInfo->updateTaskListHead.next;
    while(t) {
	if(!t->disabled && XRectInRegion(region,
	  t->rectangle.x, t->rectangle.y,
	  t->rectangle.width, t->rectangle.height) != RectangleOut) {
	    if(t->executeTask) {
		t->executeTask(t->clientData);
	    }
	}
	t = t->next;
    }
    updateInProgress = False;

  /* Copy the updatePixmap to the window */
    XCopyArea(display, displayInfo->updatePixmap,
      XtWindow(displayInfo->drawingArea), displayInfo->gc,
      clipRect.x, clipRect.y, clipRect.width, clipRect.height,
      clipRect.x, clipRect.y);
}

/* Routine to redraw all the static drawing objects onto the
   displayInfo pixmap. The region should have already been set in the
   GC. */
static void updateTaskRedrawPixmap(DisplayInfo *displayInfo,
  Region region)
{
    DlElement *pE;
    DlObject *po;
    XRectangle clipBox;

#if DEBUG_REDRAW
    static int count=0;
    print("\nupdateTaskRedrawPixmap\n");
    {
	char string[80];
	DlElement *pE = FirstDlElement(displayInfo->dlElementList);
	DlObject *po = &(pE->structure.display->object);
	sprintf(string,"DrawingArea Pixmap Before %d",++count);
	dumpPixmap(displayInfo->drawingAreaPixmap,
	  po->width,po->height,string);
    }
#endif

    if(displayInfo == NULL || region == NULL) return;

  /* Determine the bounding rectangle */
    XClipBox(region, &clipBox);

#if DEBUG_REDRAW
    print("clipBox: x=%hd y=%hd width=%hu height=%hu\n",
      clipBox.x,clipBox.y,clipBox.width,clipBox.height);
#endif

  /* Fill the (clipped) background with the background color inside
     the bounding rectangle */
    XSetForeground(display, displayInfo->gc,
      displayInfo->colormap[displayInfo->drawingAreaBackgroundColor]);
    XFillRectangle(display, displayInfo->drawingAreaPixmap,
      displayInfo->gc, clipBox.x, clipBox.y, clipBox.width, clipBox.height);

  /* Loop over elements not including the display */
    pE = SecondDlElement(displayInfo->dlElementList);
    while(pE) {
#if 1
      /* Skip widgets */
	if(pE->updateType == WIDGET) {
	    pE = pE->next;
	    continue;
	}
#endif
	po = &(pE->structure.rectangle->object);
      /* Skip if not in region */
	if(XRectInRegion(region, po->x, po->y,
	  po->width, po->height) == RectangleOut) {
	    pE = pE->next;
	    continue;
	}
#if DEBUG_REDRAW
	print("  %s: x=%d y=%d width=%d height=%d %s\n",
	  elementType(pE->type),
	  po->x,po->y,po->width,po->height,
	  pE->hidden?"hidden":"");

#endif
	if(pE->type == DL_Composite) {
	  /* Element is composite */
	    updateTaskCompositeRedrawPixmap(displayInfo, pE->structure.composite,
	      region);
	} else if(pE->updateType == STATIC_GRAPHIC) {
	  /* Element is a non-composite, static drawing object */
#if DEBUG_REDRAW
	    print("    STATIC_GRAPHIC\n");
#endif
	    if(!pE->hidden && pE->run->execute) {
#if DEBUG_REDRAW
		print("    execute\n");
#endif
		pE->run->execute(displayInfo, pE);
#if DEBUG_REDRAW && 0
    print("Dumping pixmap\n");
    {
	char string[80];
	DlElement *pE = FirstDlElement(displayInfo->dlElementList);
	DlObject *po = &(pE->structure.display->object);
	sprintf(string,"DrawingArea Pixmap After Execute %d",count);
	dumpPixmap(displayInfo->drawingAreaPixmap,
	  po->width,po->height,string);
    }
#endif
	    }
	}
	pE = pE->next;
    }
#if DEBUG_REDRAW
    {
	char string[80];
	DlElement *pE = FirstDlElement(displayInfo->dlElementList);
	DlObject *po = &(pE->structure.display->object);
	sprintf(string,"DrawingArea Pixmap After %d",count);
	dumpPixmap(displayInfo->drawingAreaPixmap,
	  po->width,po->height,string);
    }
#endif
}

/* Called by updateTaskRedrawPixmap */
static void updateTaskCompositeRedrawPixmap(DisplayInfo *displayInfo,
  DlComposite *dlComposite, Region region)
{
    DlElement *pE;
    DlObject *po;

    pE = FirstDlElement(dlComposite->dlElementList);
    while(pE) {
#if 1
      /* Skip widgets */
	if(pE->updateType == WIDGET) {
	    pE = pE->next;
	    continue;
	}
#endif
	po = &(pE->structure.rectangle->object);
      /* Skip if not in region */
	if(XRectInRegion(region, po->x, po->y,
	  po->width, po->height) == RectangleOut) {
	    pE = pE->next;
	    continue;
	}
#if DEBUG_REDRAW
	print("    %s: x=%d y=%d width=%d height=%d %s\n",
	  elementType(pE->type),
	  po->x,po->y,po->width,po->height,
	  pE->hidden?"hidden":"");

#endif
	if(pE->type == DL_Composite) {
	  /* Element is composite */
	    updateTaskCompositeRedrawPixmap(displayInfo,
	      pE->structure.composite, region);
	} else if(pE->updateType == STATIC_GRAPHIC) {
	  /* Element is a non-composite, static drawing object */
#if DEBUG_REDRAW
	    print("      STATIC_GRAPHIC\n");
#endif
	    if(!pE->hidden && pE->run->execute) {
		pE->run->execute(displayInfo, pE);
#if DEBUG_REDRAW
		print("      execute\n");
#endif
	    }
	}
	pE = pE->next;
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
	  pT->disabled?" [Disabled]":"");

	pT = pT->next;
    }
}
