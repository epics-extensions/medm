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
#define DEBUG_COMPOSITE 0
#define DEBUG_DELETE 0
#define DEBUG_HIDE 0

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

typedef struct _MedmImage {
    DlElement *dlElement;     /* Must be first */
    Record **records;
    UpdateTask *updateTask;
    DisplayInfo *displayInfo;
    Boolean validCalc;
    Boolean animate;
    char post[MAX_TOKEN_LENGTH];
} MedmImage;
#endif		    

#if DEBUG_COMPOSITE
typedef struct _MedmMeter {
    DlElement   *dlElement;     /* Must be first */
    UpdateTask  *updateTask;
    Record      *record;
} MedmMeter;
typedef struct _MedmRectangle {
    DlElement        *dlElement;     /* Must be first */
    UpdateTask       *updateTask;
    Record           **records;
} MedmRectangle;
typedef struct _MedmTextUpdate {
    DlElement     *dlElement;     /* Must be first */
    UpdateTask    *updateTask;
    Record        *record;
    int           fontIndex;
} MedmTextUpdate;
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
    if(currentTime != t->systemTime) {
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
#if DEBUG_UPDATE
	    print("  di=%x head->nextExecuteTime=%.3f head->timeInterval=%.3f\n",
	      di,di->updateTaskListHead.nextExecuteTime-medmStartTime,
	      di->updateTaskListHead.timeInterval);
	    {
		UpdateTask *tmp = di->updateTaskListHead.next;
		int i=0;
		
		while(tmp) {
		    print("    %d nextExecuteTime=%.3f timeInterval=%.3f\n",
		      i++,tmp->nextExecuteTime-medmStartTime,
		      tmp->timeInterval);
		    tmp = tmp->next;
		}
		
	    }
#endif		    
	    if(di->periodicTaskCount > 0) {
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
		if(pt->nextExecuteTime < currentTime) {
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
    if((updateTaskStatus.updateRequestQueued > 0) && 
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
    if(gettimeofday(&tp,NULL)) {
	medmPostMsg(1,"medmTime:  Failed to get time\n");
	return 0.;
    }
#endif
    return (double) tp.tv_sec + (double) tp.tv_usec*1e-6;
}
#endif

void startMedmScheduler(void)
{
    if(!moduleInitialized) medmInitSharedDotC();
    medmScheduler((XtPointer)&periodicTask, NULL);
}

void stopMedmScheduler(void)
{
    if(!moduleInitialized) medmInitSharedDotC();
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

    if(!moduleInitialized) medmInitSharedDotC();
}

Boolean medmInitSharedDotC()
{
#if 0
    if(moduleInitialized) return True;
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

#if DEBUG_COMPOSITE || DEBUG_DELETE
	{
	    MedmElement *pElement=(MedmElement *)pT->clientData;
	    DlElement *pET = pElement->dlElement;
	    
	    print("updateTaskAddTask: Added pT=%x pE=%x[%x] pE->type=%s\n",
	      pT,pET,pET->structure.element,elementType(pET->type));
	}
#endif			
	
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

#if 0
/* KE: If we delete tasks under the current (original) scheme and then
 * add them back later, the stacking order will not be correct.  Keep
 * these debugged routines around in case we change our mind */

/* Delete all update tasks for a given element  */
void updateTaskDeleteElementTasks(DisplayInfo *displayInfo, DlElement *pE)
{
    UpdateTask *pT,*pTNext;
    int found=1;

#if DEBUG_COMPOSITE
    int i=0;
    print("updateTaskDeleteElementTasks: dlElement=%x[%s] dlComposite=%x\n",
      pE,elementType(pE->type),pE->structure.composite);
#if 0    
    pT = displayInfo->updateTaskListHead.next; 
    while(pT) {
	MedmElement *pElement=(MedmElement *)pT->clientData;
	DlElement *pET = pElement->dlElement;
	
	print("  %2d pT=%x pTNext=%x pET=%x",
	  ++i,pT,pTNext,pET);
	fflush(stdout);
	print("[%s] pE=%x[%s] pE->structure.composite=%x\n",
	  elementType(pET->type),
	  pE,elementType(pE->type),
	  pE->structure.composite);
	pT = pT->next;
    }
#endif
#endif

    pT = displayInfo->updateTaskListHead.next; 
    while(pT) {
	MedmElement *pElement=(MedmElement *)pT->clientData;
	DlElement *pET = pElement->dlElement;
	
	pTNext=pT->next;
	if(pET == pE) {
#if DEBUG_COMPOSITE
	    print("  Deleted pT=%x pE=%x"
	      " pE->type=%s\n",pT,pE,elementType(pE->type));
#endif			
	    updateTaskDeleteTask(displayInfo, pT);
	}
	pT = pTNext;
    }
}
  
/* Delete a single update task */
void updateTaskDeleteTask(DisplayInfo *displayInfo, UpdateTask *pT)
{
    if(pT == NULL) return;
  /* Adjust the next pointers */
    if(pT == displayInfo->updateTaskListTail) {
	displayInfo->updateTaskListTail = pT->prev;
	displayInfo->updateTaskListTail->next = NULL;
    } else {
	pT->prev->next = pT->next;
	pT->next->prev = pT->prev;
    }
  /* Run the destroy callback */
    if(pT->destroyTask) {
	pT->destroyTask(pT->clientData);
    }
  /* Reset status counters */
    if(pT->timeInterval > 0.0) {
	updateTaskStatus.periodicTaskCount--;
    }
    updateTaskStatus.taskCount--;
    if(pT->executeRequestsPendingCount > 0) {
	updateTaskStatus.updateRequestQueued--;
    }
  /* Reset next to serve */
    if(updateTaskStatus.nextToServe == pT) {
	updateTaskStatus.nextToServe = NULL;
    } 
#if DEBUG_COMPOSITE || DEBUG_DELETE
	{
	    MedmElement *pElement=(MedmElement *)pT->clientData;
	    DlElement *pET = pElement->dlElement;
	    
	    print("updateTaskDeleteTask: Deleted pT=%x pE=%x"
	      " pE->type=%s\n",
	      pT,pET,elementType(pET->type));
	}
#endif			
  /* Zero the task before freeing it to help with pointer errors */
   *pT = nullTask;
    free((char *)pT);
}
#endif

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
    
#if 0
  /* Reset the nextExecuteTime for the display */
    head->nextExecuteTime = currentTime + head->timeInterval;
#endif
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
#if DEBUG_COMPOSITE || DEBUG_DELETE
	{
	    MedmElement *pElement=(MedmElement *)pT->clientData;
	    DlElement *pET = pElement->dlElement;
	    
	    print("updateTaskMarkUpdate: [%d queued] Marked pT=%x pE=%x"
	      " pE->type=%s\n",
	      updateTaskStatus.updateRequestQueued,pT,pET,elementType(pET->type));
	}
#endif			
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
Boolean updateTaskWorkProc(XtPointer cd)
{
    DisplayInfo *displayInfo;
    UpdateTaskStatus *ts = (UpdateTaskStatus *)cd;
    UpdateTask *t = ts->nextToServe;
    UpdateTask *t1;
    double endTime;
    int found;
   
#if DEBUG_UPDATE || DEBUG_DELETE
    print("\nupdateTaskWorkProc: time=%.3f\n",medmElapsedTime());
#if 1
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
    endTime = medmTime() + WORKINTERVAL; 
 
  /* Do for WORKINTERVAL sec */
    do {
#if DEBUG_DELETE
	print("  Top of loop: t=%x ts->nextToServe=%x [pending %d]\n",
	      t,ts->nextToServe,ts->updateRequestQueued);
#endif    
      /* If no requests queued, remove work proc */
	if(ts->updateRequestQueued <= 0) {
	    ts->workProcId = 0; 
#if DEBUG_UPDATE || DEBUG_DELETE
	    print("updateTaskWorkProc Return 1 (True) [pending %d]\n",
	      ts->updateRequestQueued);
#endif    
	    return True;
	} 
      /* If no valid update task, find one */
	if(t == NULL) {
	    displayInfo = displayInfoListHead->next;

	  /* If no display, remove work proc */
	    if(displayInfo == NULL) {
		ts->workProcId = 0;
#if DEBUG_UPDATE || DEBUG_DELETE
		print("updateTaskWorkProc Return 2 (True) [pending %d]\n",
		  ts->updateRequestQueued);
#endif    
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
#if DEBUG_UPDATE || DEBUG_DELETE
	    print("updateTaskWorkProc Return 3 (True) [pending %d]\n",
	      ts->updateRequestQueued);
#endif    
		return True;
	    }
	    ts->nextToServe = t;
	}

#if DEBUG_DELETE > 1
	print("  Position 1: t=%x ts->nextToServe=%x\n",
	      t,ts->nextToServe);
#endif    
      /* At least one update task has been found.  Find one which has
	 is enabled and has executeRequestsPendingCount > 0 */
	while(t->executeRequestsPendingCount <= 0 || t->disabled) {
	    displayInfo = t->displayInfo;
	    t = t->next;
	    while(t == NULL) {
	      /* End of the update tasks for this display, check the
		 next display */
		displayInfo = displayInfo->next;
	      /* If at the end of the displays, go to the beginning */
		if(displayInfo == NULL) {
		    displayInfo = displayInfoListHead->next;
		}
		t = displayInfo->updateTaskListHead.next;
	    }      
	  /* If t is NULL or found same t again, have checked all
	     displays and there is nothing to do.  Remove work proc */
	    if(t == ts->nextToServe) {
		ts->workProcId = 0;
#if DEBUG_UPDATE || DEBUG_DELETE
		print("updateTaskWorkProc Return 4 (True) [pending %d]\n",
		  ts->updateRequestQueued);
#endif    
		return True;
	    }
	}
      /* An enabled update task with executeRequestsPendingCount > 0
         has been found.  Set it to be the next to serve.  */
	ts->nextToServe = t;
      /* Get the displayInfo */
	displayInfo = t->displayInfo;

      /* Repaint the selected region */
	if(t->overlapped) {
	    Display *display = XtDisplay(displayInfo->drawingArea);
	    GC gc = displayInfo->gc;
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

	    if(region == NULL) {
		medmPrintf(0,"\nupdateTaskWorkProc: XPolygonRegion is NULL\n");
	      /* Kill the work proc */
		ts->workProcId = 0;
#if DEBUG_UPDATE || DEBUG_DELETE
		print("updateTaskWorkProc Return 5 (True) [pending %d]\n",
		  ts->updateRequestQueued);
#endif    
		return True;
	    }

	    XSetClipRectangles(display,gc,0,0,&t->rectangle,1,YXBanded);
	    if(!t->opaque)
	      XCopyArea(display,displayInfo->drawingAreaPixmap,
		XtWindow(displayInfo->drawingArea),gc,
		t->rectangle.x, t->rectangle.y,
		t->rectangle.width, t->rectangle.height,
		t->rectangle.x, t->rectangle.y);

	  /* KE: This does no good.  Each task is set to overlapped
             when it is created.  This particular task will be one of
             the ones executed in the loop below, where overlapped
             will be set back. The non-overlapped branch will never be
             executed. */
	    t->overlapped = False;     

	  /* Do the update tasks for overlapped objects.  Equivalent
             to calling updateTaskRepaintRegion except that it sets
             overlapped. */
#if 0
	    updateTaskRepaintRegion(t->displayInfo, &region);
#else
#if DEBUG_DELETE  || DEBUG_HIDE
	    {
		MedmElement *pElement=(MedmElement *)t->clientData;
		DlElement *pET = pElement->dlElement;
		
		print("> Doing overlapped for pT=%x pE=%x"
		  " pE->type=%s x=%d y=%d\n",
		  t,pET,elementType(pET->type),
		  t->rectangle.x,t->rectangle.y);
	    }
#endif
	    t1 = t->displayInfo->updateTaskListHead.next;
	    while(t1) {
		if(!t1->disabled && XRectInRegion(region,
		  t1->rectangle.x, t1->rectangle.y,
		  t1->rectangle.width, t1->rectangle.height) != RectangleOut) {
		    t1->overlapped = True;
		    if(t1->executeTask) {
#if DEBUG_DELETE  || DEBUG_HIDE
			{
			    MedmElement *pElement=(MedmElement *)t1->clientData;
			    DlElement *pET = pElement->dlElement;
			    
			    print("  Executed (overlapped) pT=%x pE=%x"
			      " pE->type=%s [%d,%d]\n",
			      t1,pET,elementType(pET->type),
			      t1->rectangle.x,t1->rectangle.y);
			}
#endif			
			t1->executeTask(t1->clientData);
		    }
		}
		t1 = t1->next;
	    }
#endif
	  /* Release the clipping region */
	    XSetClipOrigin(display,gc,0,0);
	    XSetClipMask(display,gc,None);
	    if(region) XDestroyRegion(region);
#if DEBUG_DELETE  || DEBUG_HIDE
	    print("< Done with overlapped for pT=%x\n",t);
#endif
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
#if DEBUG_COMPOSITE
		print("updateTaskWorkProc (not overlapped): "
		  "clientData=%x x=%d y=%d\n",
		  t->clientData,t->rectangle.x,t->rectangle.y);
#endif			
#if DEBUG_DELETE  || DEBUG_HIDE
		{
		    MedmElement *pElement=(MedmElement *)t->clientData;
		    DlElement *pET = pElement->dlElement;
		    
		    print("  Executed (not overlapped) pT=%x pE=%x"
		      " pE->type=%s [%d,%d]\n",
		      t,pET,elementType(pET->type),
		      t->rectangle.x,t->rectangle.y);
		}
#endif			
		t->executeTask(t->clientData);
	    }
	}

#if DEBUG_DELETE
	print(" Pending %d\n",ts->updateRequestQueued);
	if(t != ts->nextToServe) {
	    print("  t != ts->nextToServe:  t=%x ts->nextToServe=%x !!!!!\n",
	      t,ts->nextToServe);
	}
#endif

      /* Update ts->updateExecuted since we executed it, but not
         executeRequestsPendingCount, since if the task was deleted,
         this will have been done in updateTaskDeleteTask. */
	ts->updateExecuted++;

#if 0
      /* Check if *t got deleted by the executeTask's.  If so, t is an
         invalid pointer and cannot be used anymore, so start over at
         the top.  Also, we should not decrement the
         executeRequestsPendingCount */
	t1 = displayInfo->updateTaskListHead.next;
	found = 0;
	while(t1) {
	    if(t1 == t) {
		found = 1;
		break;
	    }
	    t1 = t1->next;
	}
	if(!found) {
	  /* The next to serve is also invalid */
#if DEBUG_DELETE
	    print("  Invalid t: t=%x ts->nextToServe=%x\n",
	      t,ts->nextToServe);
#endif    
	    ts->nextToServe = t = NULL;	    
	    continue;
	}
#endif
	
      /* Reset the executeRequestsPendingCount only if t is still valid */
	t->executeRequestsPendingCount = 0;
	ts->updateRequestQueued--;
	
      /* Find the next one */
	while(t->executeRequestsPendingCount <= 0 ) {
	    DisplayInfo *displayInfo = t->displayInfo;
	    t = t->next;
	    while(t == NULL) {
	      /* End of the update tasks for this display, check the
		 next display */
		displayInfo = displayInfo->next;
		if(displayInfo == NULL) {
		    displayInfo = displayInfoListHead->next;
		}
		t = displayInfo->updateTaskListHead.next;
	    }
	  /* If found same t again, have now checked all displays and
	     there is nothing to do.  Remove work proc */
	    if(t == ts->nextToServe) {
		ts->workProcId = 0;
#if DEBUG_UPDATE || DEBUG_DELETE
		print("updateTaskWorkProc Return 6 (True) [pending %d]\n",
		  ts->updateRequestQueued);
#endif    
		return True;
	    }
	}
	ts->nextToServe = t;
    } while(endTime > medmTime());
    
  /* Keep the work proc active */
#if DEBUG_UPDATE || DEBUG_DELETE
    print("updateTaskWorkProc Return Bottom (False) [pending %d]\n",
      ts->updateRequestQueued);
#endif    
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
#if DEBUG_COMPOSITE
		print("updateTaskRepaintRegion: "
		  "clientData=%x x=%d y=%d\n",
		  t->clientData,t->rectangle.x,t->rectangle.y);
#endif			
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
    while(ptu) {
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
	
	print("  %2d pT=%x pE=%x pC=%x x=%3d y=%3d %s\n",
	  ++i,pT,pE,pC,
	  pO->x, pO->y,
	  elementType(pE->type));
	
	pT = pT->next;
    }
}
