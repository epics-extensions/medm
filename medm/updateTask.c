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

#define DEBUG_SCHEDULER 0
#define DEBUG_UPDATE 0

#include "medm.h"

/* Include this after medm.h to avoid problems with Exceed 6 */
#ifdef WIN32
/* In MSVC timeval is in winsock.h, winsock2.h, ws2spi.h, nowhere else */
#include <X11/Xos.h>
#else
#include <sys/time.h>
#endif

#if DEBUG_UPDATE
#define DEFAULT_TIME 1
#define ANIMATE_TIME(gif) \
  ((gif)->frames[CURFRAME(gif)]->DelayTime ? \
  ((gif)->frames[CURFRAME(gif)]->DelayTime)/100. : DEFAULT_TIME)

typedef struct _Image {
    DisplayInfo   *displayInfo;
    DlElement     *dlElement;
    Record        *record;
    UpdateTask    *updateTask;
} MedmImage;
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
Boolean medmInitSharedDotC();
static void medmScheduler(XtPointer, XtIntervalId *);
static Boolean updateTaskWorkProc(XtPointer);

/* Global variables */
static UpdateTaskStatus updateTaskStatus;
static PeriodicTask periodicTask;
static Boolean moduleInitialized = False;
static double medmStartTime=0;

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
#ifdef __cplusplus
static void medmScheduler(XtPointer cd, XtIntervalId *)
#else
static void medmScheduler(XtPointer cd, XtIntervalId *id)
#endif
{
  /* KE: the cd is set to the static global periodicTask.  Could just
     as well directly use the global here. */
    PeriodicTask *t = (PeriodicTask *)cd;
    double currentTime = medmTime();

#if DEBUG_UPDATE
    print("medmScheduler: time=%.3f\n",medmElapsedTime());
#endif    
#ifdef MEDM_SOFT_CLOCK
    t->tenthSecond += 0.1;
    if (currentTime != t->systemTime) {
	t->systemTime = currentTime;
	t->tenthSecond = 0.0;
    }
#endif

  /* Poll channel access  */
#ifdef __MONITOR_CA_PEND_EVENT__
    {
	double t;
	t = medmTime();
	ca_pend_event(0.00000001);
	t = medmTime() - t;
	if (t > 0.5) {
	    fprintf(stderr,"medmScheduler : time used by ca_pend_event ="
	      " %8.1f\n", t);
	}
    }
#else
    ca_pend_event(0.00000001);
#endif

  /* Look for displays that have a periodic task which is timed out */
    if (updateTaskStatus.periodicTaskCount > 0) { 
	DisplayInfo *di = displayInfoListHead->next;
	while (di) {
#if DEBUG_UPDATE
	    print("  di=%x head->nextExecuteTime=%.3f head->timeInterval=%.3f\n",
	      di,di->updateTaskListHead.nextExecuteTime-medmStartTime,
	      di->updateTaskListHead.timeInterval);
	    {
		UpdateTask *tmp = di->updateTaskListHead.next;
		int i=0;
		
		while (tmp) {
		    print("    %d nextExecuteTime=%.3f timeInterval=%.3f\n",
		      i++,tmp->nextExecuteTime-medmStartTime,
		      tmp->timeInterval);
		    tmp = tmp->next;
		}
		
	    }
#endif		    
	    if (di->periodicTaskCount > 0) {
#if 0
	      /* KE: Doesn't make sense */
		UpdateTask *pt = di->updateTaskListHead.next;
#else		
	      /* Check the head, which should have the soonest
                 nextExecuteTime of any of the update tasks for this
                 display */
		UpdateTask *pt = &di->updateTaskListHead;
#endif
		
#if DEBUG_SCHEDULER
		if(strstr(di->dlFile->name,"sMain.adl")) {
		    fprintf(stderr,"medmScheduler:\n");
		    fprintf(stderr,"  di->updateTaskListHead.next is %d\n",
		      di->updateTaskListHead.next);
		    fprintf(stderr,"  di->periodicTaskCount: %d\n",
		      di->periodicTaskCount);
		    fprintf(stderr,"  di->dlFile->name: |%s|\n",
		      di->dlFile->name);
		}
		if(!pt) {
		    fprintf(stderr,"medmScheduler:\n");
		    fprintf(stderr,"  di->updateTaskListHead.next is NULL\n");
		    fprintf(stderr,"  di->periodicTaskCount: %d\n",
		      di->periodicTaskCount);
		    fprintf(stderr,"  di->dlFile->name: |%s|\n",
		      di->dlFile->name);
		    fprintf(stderr,"Aborting\n");
		    abort();
		}
#endif
		if (pt->nextExecuteTime < currentTime) {
		    updateTaskMarkTimeout(pt,currentTime);
#if DEBUG_UPDATE
		    print("  nextExecuteTime=%.3f timeInterval=%.3f\n",
		      pt->nextExecuteTime-medmStartTime,
		     pt->timeInterval);
#endif		    
		}
	    }
	    di = di->next;
	}
    }

  /* If needed, install the work proc */
    if ((updateTaskStatus.updateRequestQueued > 0) && 
      (!updateTaskStatus.workProcId)) {
	updateTaskStatus.workProcId =
	  XtAppAddWorkProc(appContext,updateTaskWorkProc,&updateTaskStatus);
    }
    
  /* Reinstall the timer proc to be called again in TIMERINTERVAL ms */
    t->id = XtAppAddTimeOut(appContext,TIMERINTERVAL,medmScheduler,cd);
}

#ifdef MEDM_SOFT_CLOCK
double medmTime()
{
    return task.systemTime + task.tenthSecond;
}
#else
double medmTime()
{
    struct timeval tp;
    
#ifdef WIN32
  /* MSVC does not have gettimeofday.  It comes from Exceed and is
   *   defined differently */
    gettimeofday(&tp,0);
#else
    if (gettimeofday(&tp,NULL)) {
	medmPostMsg(1,"medmTime:  Failed to get time\n");
	return 0.;
    }
#endif
    return (double) tp.tv_sec + (double) tp.tv_usec*1e-6;
}
#endif

void startMedmScheduler(void)
{
    if (!moduleInitialized) medmInitSharedDotC();
    medmScheduler((XtPointer)&periodicTask, NULL);
}

void stopMedmScheduler(void)
{
    if (!moduleInitialized) medmInitSharedDotC();
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

void updateTaskInit(DisplayInfo *displayInfo)
{
    UpdateTask *pt = &(displayInfo->updateTaskListHead);
    
    pt->executeTask = NULL;
    pt->destroyTask = NULL;
    pt->clientData = NULL;
    pt->timeInterval = EXECINTERVAL;
    pt->nextExecuteTime = medmTime() + pt->timeInterval;
    pt->displayInfo = displayInfo;
    pt->next = NULL;
    pt->executeRequestsPendingCount = 0;
    displayInfo->updateTaskListTail = pt;
    displayInfo->periodicTaskCount = 0;

    if (!moduleInitialized) medmInitSharedDotC();
}

Boolean medmInitSharedDotC()
{
#if 0
    if (moduleInitialized) return True;
#endif    
    
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

    return True;
}

UpdateTask *updateTaskAddTask(DisplayInfo *displayInfo, DlObject *rectangle,
  void (*executeTask)(XtPointer), XtPointer clientData)
{
    UpdateTask *pt;
    
    if (displayInfo) {
	pt = (UpdateTask *)malloc(sizeof(UpdateTask));
	if (pt == NULL) return pt;
	pt->executeTask = executeTask;
	pt->destroyTask = NULL;
	pt->getRecord = NULL;
	pt->clientData = clientData;
	pt->timeInterval = 0.0;
	pt->nextExecuteTime = medmTime() + pt->timeInterval;
	pt->displayInfo = displayInfo;
	pt->next = NULL;
	pt->executeRequestsPendingCount = 0;
	if (rectangle) {
	    pt->rectangle.x      = rectangle->x;
	    pt->rectangle.y      = rectangle->y;
	    pt->rectangle.width  = rectangle->width;
	    pt->rectangle.height = rectangle->height;
	} else {
	    pt->rectangle.x      = 0;
	    pt->rectangle.y      = 0;
	    pt->rectangle.width  = 0;
	    pt->rectangle.height = 0;
	}
	pt->overlapped = True;  /* make the default be True */
	pt->opaque = True;      /* don't draw the background */
	displayInfo->updateTaskListTail->next = pt;
	displayInfo->updateTaskListTail = pt;

      /* KE: Should never execute this branch since pt->timeInterval=0.0 */
	if (pt->timeInterval > 0.0) {
	    displayInfo->periodicTaskCount++;
	    updateTaskStatus.periodicTaskCount++;
	    if (pt->nextExecuteTime <
	      displayInfo->updateTaskListHead.nextExecuteTime) {
		displayInfo->updateTaskListHead.nextExecuteTime =
		  pt->nextExecuteTime;
	    }
	}
	updateTaskStatus.taskCount++;
	return pt;
    } else {
	return NULL;
    }
}  

/* Delete all update tasks on the display associated with a given task */
void updateTaskDeleteAllTask(UpdateTask *pt)
{
    UpdateTask *tmp;
    DisplayInfo *displayInfo;
    
    if (pt == NULL) return;
    displayInfo = pt->displayInfo;
    displayInfo->periodicTaskCount = 0;
    tmp = displayInfo->updateTaskListHead.next;
    while (tmp) {
	UpdateTask *tmp1 = tmp;
	tmp = tmp1->next;
	if (tmp1->destroyTask) {
	    tmp1->destroyTask(tmp1->clientData);
	}
	if (tmp1->timeInterval > 0.0) {
	    updateTaskStatus.periodicTaskCount--;
	}
	updateTaskStatus.taskCount--;
	if (tmp1->executeRequestsPendingCount > 0) {
	    updateTaskStatus.updateRequestQueued--;
	}
	if (updateTaskStatus.nextToServe == tmp1) {
	    updateTaskStatus.nextToServe = NULL;
	}
      /* ??? Why is this necessary, the space is going to be freed */
	tmp1->executeTask = NULL;
	free((char *)tmp1);
    }
    if ((updateTaskStatus.taskCount <=0) && (updateTaskStatus.workProcId)) {
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
	if (t > 0.5) {   
	    fprintf(stderr, "updateTaskDeleteAllTask : time used by "
	      "ca_pend_event = %8.1f\n",t);
	} 
    }
#else
    ca_pend_event(CA_PEND_EVENT_TIME);
#endif
}

/* Delete all update tasks for a given element */
void updateTaskDeleteElementTasks(DisplayInfo *displayInfo, DlElement *pE)
{
    UpdateTask *pT;

    pT = displayInfo->updateTaskListHead.next; 
    while (pT) {
      /* The clientData is the first element in the MedmXxx structure
         pointed to by the clientData */
	if((DlElement *)pT->clientData == pE) {
	    updateTaskDeleteTask(displayInfo, pT);
	}
	pT = pT->next;
    }
}
  
/* Delete a single update task */
void updateTaskDeleteTask(DisplayInfo *displayInfo, UpdateTask *pt)
{
    if (pt == NULL) return;
  /* Adjust the next pointers */
    if
    displayInfo->updateTaskListTail->next = pt;
    displayInfo->updateTaskListTail = pt;
    
  /* Run the destroy callback */
    if (pt->destroyTask) {
	pt->destroyTask(pt->clientData);
    }
  /* Reset status counters */
    if (pt->timeInterval > 0.0) {
	updateTaskStatus.periodicTaskCount--;
    }
    updateTaskStatus.taskCount--;
    if (pt->executeRequestsPendingCount > 0) {
	updateTaskStatus.updateRequestQueued--;
    }
  /* Reset next to serve */
    if (updateTaskStatus.nextToServe == pt) {
	updateTaskStatus.nextToServe = NULL;
    }
  /* ??? Why is this necessary, the space is going to be freed */
    pt->executeTask = NULL;
    free((char *)pt);
}

int updateTaskMarkTimeout(UpdateTask *pt, double currentTime)
{
    UpdateTask *head = &(pt->displayInfo->updateTaskListHead);
    UpdateTask *tmp = head->next;
    int count = 0;
    
#if 0
  /* Reset the nextExecuteTime for the display */
    head->nextExecuteTime = currentTime + head->timeInterval;
#endif
  /* Loop over the update task list and check for timeouts */
    while(tmp) {
      /* Check if periodic task */
	if(tmp->timeInterval > 0.0) {
	  /* Mark if the task is timed out already */
	    if (currentTime > tmp->nextExecuteTime) {
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

void updateTaskMarkUpdate(UpdateTask *pt)
{
    if (pt->executeRequestsPendingCount > 0) {
	updateTaskStatus.updateDiscardCount++;
    } else {
	updateTaskStatus.updateRequestCount++;
	updateTaskStatus.updateRequestQueued++;
    }
    pt->executeRequestsPendingCount++;
}

void updateTaskSetScanRate(UpdateTask *pt, double timeInterval)
{
    UpdateTask *head = &(pt->displayInfo->updateTaskListHead);
    double currentTime = medmTime();

  /* timeInterval is in seconds */
    
  /* Whether to increase or decrease the periodic task count depends
   * on how timeInterval changes */
    if ((pt->timeInterval == 0.0) && (timeInterval != 0.0)) {
      /* was zero, now non-zero */
	pt->displayInfo->periodicTaskCount++;
	updateTaskStatus.periodicTaskCount++;
    } else if ((pt->timeInterval != 0.0) && (timeInterval == 0.0)) {
      /* was non-zero, now zero */
	pt->displayInfo->periodicTaskCount--;
	updateTaskStatus.periodicTaskCount--;
    }

  /* Set up the next scan time for this task, if this is the sooner
   * one, set it to the display scan time too */
    pt->timeInterval = timeInterval;
    pt->nextExecuteTime = currentTime + pt->timeInterval;
    if (pt->nextExecuteTime < head->nextExecuteTime) {
	head->nextExecuteTime = pt->nextExecuteTime;
    }
}

void updateTaskAddExecuteCb(UpdateTask *pt, void (*executeTaskCb)(XtPointer))
{
    pt->executeTask = executeTaskCb;
}

void updateTaskAddDestroyCb(UpdateTask *pt, void (*destroyTaskCb)(XtPointer))
{
    pt->destroyTask = destroyTaskCb;
}

/* Work proc for updateTask.  Is called when medm is not busy.  Checks
   for update tasks with executeRequestspendingCount > 0 and executes
   them. */
Boolean updateTaskWorkProc(XtPointer cd)
{
    UpdateTaskStatus *ts = (UpdateTaskStatus *)cd;
    UpdateTask *t = ts->nextToServe;
    double endTime;
   
#if DEBUG_UPDATE
    print("updateTaskWorkProc: time=%.3f\n",medmElapsedTime());
#endif    
    endTime = medmTime() + WORKINTERVAL; 
 
  /* Do for WORKINTERVAL sec */
    do {
      /* If no requests queued, remove work proc */
	if (ts->updateRequestQueued <= 0) {
	    ts->workProcId = 0; 
	    return True;
	} 
      /* If no valid update task, find one */
	if (t == NULL) {
	    DisplayInfo *di = displayInfoListHead->next;
	  /* If no display, remove work proc */
	    if (di == NULL) {
		ts->workProcId = 0;
		return True;
	    }
	  /* Loop over displays to find an update task */
	    while (di) {
		if (t = di->updateTaskListHead.next) break;
		di = di->next;
	    }
	  /* If no update task found, remove work proc */
	    if (t == NULL) {
		ts->workProcId = 0;
		return True;
	    }
	    ts->nextToServe = t;
	}

      /* At least one update task has been found.  Find one which has
	 executeRequestsPendingCount > 0 */
	while (t->executeRequestsPendingCount <= 0 ) {
	    DisplayInfo *di = t->displayInfo;
	    t = t->next;
	    while (t == NULL) {
	      /* End of the update tasks for this display, check the
		 next display */
		di = di->next;
	      /* If at the end of the displays, go to the beginning */
		if (di == NULL) {
		    di = displayInfoListHead->next;
		}
		t = di->updateTaskListHead.next;
	    }      
	  /* If found same t again, have now checked all displays and
	     there is nothing to do.  Remove work proc */
	    if (t == ts->nextToServe) {
		ts->workProcId = 0;
		return True;
	    }
	}
      /* An update task with executeRequestsPendingCount > 0 has been
         found.  Set it to be the next to serve.  */
	ts->nextToServe = t;

      /* Execute it */
	if (t->overlapped) {
	    DisplayInfo *pDI = t->displayInfo;
	    Display *display = XtDisplay(pDI->drawingArea);
	    GC gc = pDI->gc;
	    XPoint points[4];
	    Region region;

	    points[0].x = t->rectangle.x;
	    points[0].y = t->rectangle.y;
	    points[1].x = t->rectangle.x + t->rectangle.width;
	    points[1].y = t->rectangle.y;
	    points[2].x = t->rectangle.x + t->rectangle.width;
	    points[2].y = t->rectangle.y + t->rectangle.height;
	    points[3].x = t->rectangle.x;
	    points[3].y = t->rectangle.y + t->rectangle.height;
	    region = XPolygonRegion(points,4,EvenOddRule);

	    if (region == NULL) {
		medmPrintf(0,"\nupdateTaskWorkProc: XPolygonRegion is NULL\n");
		/* kill the work proc */
		ts->workProcId = 0;
		return True;
	    }

	    XSetClipRectangles(display,gc,0,0,&t->rectangle,1,YXBanded);
	    if (!t->opaque)
	      XCopyArea(display,pDI->drawingAreaPixmap,
		XtWindow(pDI->drawingArea),gc,
		t->rectangle.x, t->rectangle.y,
		t->rectangle.width, t->rectangle.height,
		t->rectangle.x, t->rectangle.y);

	    t->overlapped = False;     

	    t = t->displayInfo->updateTaskListHead.next;
	    while (t) {
		if (XRectInRegion(region,
		  t->rectangle.x, t->rectangle.y,
		  t->rectangle.width, t->rectangle.height) != RectangleOut) {
		    t->overlapped = True;
		    if (t->executeTask)
		      t->executeTask(t->clientData);
		}
		t = t->next;
	    }
	  /* Release the clipping region */
	    XSetClipOrigin(display,gc,0,0);
	    XSetClipMask(display,gc,None);
	    if (region) XDestroyRegion(region);
	} else {
	    if (!t->opaque) 
	      XCopyArea(XtDisplay(t->displayInfo->drawingArea),
		t->displayInfo->drawingAreaPixmap,
		XtWindow(t->displayInfo->drawingArea),
		t->displayInfo->gc,
		t->rectangle.x, t->rectangle.y,
		t->rectangle.width, t->rectangle.height,
		t->rectangle.x, t->rectangle.y);
	    if (t->executeTask) 
	      t->executeTask(t->clientData);
	}     
	ts->updateExecuted++;
	ts->updateRequestQueued--;
      /* Reset the executeRequestsPendingCount */
	t = ts->nextToServe;
	t->executeRequestsPendingCount = 0;
	
      /* Find the next one */
	while (t->executeRequestsPendingCount <= 0 ) {
	    DisplayInfo *di = t->displayInfo;
	    t = t->next;
	    while (t == NULL) {
	      /* End of the update tasks for this display, check the
		 next display */
		di = di->next;
		if (di == NULL) {
		    di = displayInfoListHead->next;
		}
		t = di->updateTaskListHead.next;
	    }
	  /* If found same t again, have now checked all displays and
	     there is nothing to do.  Remove work proc */
	    if (t == ts->nextToServe) {
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
    while (t) {
	if (XRectInRegion(*region, t->rectangle.x, t->rectangle.y,
	  t->rectangle.width, t->rectangle.height) != RectangleOut) {
	    if (t->executeTask)
	      t->executeTask(t->clientData);
	}
	t = t->next;
    }
}

void updateTaskAddNameCb(UpdateTask *pt, void (*nameCb)(XtPointer,
  Record **, int *))
{
    pt->getRecord = nameCb;
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
    while (pT) {
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

/*
 * return UpdateTask ptr given a DisplayInfo* and x,y positions
 */
UpdateTask *getUpdateTaskFromPosition(DisplayInfo *displayInfo, int x, int y)
{
    UpdateTask *ptu, *ptuSaved = NULL;
    int minWidth, minHeight;
  
    if(displayInfo == (DisplayInfo *)NULL) return NULL;

    minWidth = INT_MAX;	 	/* according to XPG2's values.h */
    minHeight = INT_MAX;

    ptu = displayInfo->updateTaskListHead.next;
    while (ptu) {
	if(x >= (int)ptu->rectangle.x &&
	  x <= (int)ptu->rectangle.x + (int)ptu->rectangle.width &&
	  y >= (int)ptu->rectangle.y &&
	  y <= (int)ptu->rectangle.y + (int)ptu->rectangle.height) {
	  /* eligible element, see if smallest so far */
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
