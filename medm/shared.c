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
#include <time.h>

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

static UpdateTaskStatus updateTaskStatus;
static PeriodicTask periodicTask;
static Boolean moduleInitialized = False;

static void medmScheduler(XtPointer, XtIntervalId *);
static Boolean updateTaskWorkProc(XtPointer);

Boolean medmInitSharedDotC() {
    if (moduleInitialized) return True;
  /* initialize the update task */
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

  /* initialize the periodic task */
    periodicTask.systemTime = medmTime();
    periodicTask.tenthSecond = 0.0; 
    medmScheduler((XtPointer) &periodicTask, NULL);
    moduleInitialized = True;
    return True;
}

void updateTaskStatusGetInfo(int *taskCount,
  int *periodicTaskCount,
  int *updateRequestCount,
  int *updateDiscardCount,
  int *periodicUpdateRequestCount,
  int *periodicUpdateDiscardCount,
  int *updateRequestQueued,
  int *updateExecuted,
  double *timeInterval) {
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

#ifdef __cplusplus 
void wmCloseCallback(Widget w, XtPointer cd, XtPointer)
#else
void wmCloseCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
    ShellType shellType = (ShellType) cd;
  /*
   * handle WM Close functions like all the separate dialog close functions,
   *   dispatch based upon widget value that the callback is called with
   */
    switch (shellType) {
    case DISPLAY_SHELL:
	closeDisplay(w);
	break;

    case OTHER_SHELL:
      /* it's one of the permanent shells */
	if (w == mainShell) {
	    medmExit();
	} else if (w == objectS) {
	    XtPopdown(objectS);
	} else if (w == resourceS) {
	    XtPopdown(resourceS);
	} else if (w == colorS) {
	    XtPopdown(colorS);
	} else if (w == channelS) {
	    XtPopdown(channelS);
	} else if (w == helpS) {
	    XtPopdown(helpS);
	}
	break;
    }
}




/*
 * optionMenuSet:  routine to set option menu to specified button index
 *		(0 - (# buttons - 1))
 */
void optionMenuSet(Widget menu, int buttonId)
{
    WidgetList buttons;
    Cardinal numButtons;
    Widget subMenu;

  /* (MDA) - if option menus are ever created using non pushbutton or
   *	pushbutton widgets in them (e.g., separators) then this routine must
   *	loop over all children and make sure to only reference the push
   *	button derived children
   *
   *	Note: for invalid buttons, don't do anything (this can occur
   *	for example, when setting dynamic attributes when they don't
   *	really apply (and this is usually okay because they are not
   *	managed in invalid cases anyway))
   */
    XtVaGetValues(menu,XmNsubMenuId,&subMenu,NULL);
    if (subMenu != NULL) {
	XtVaGetValues(subMenu,XmNchildren,&buttons,XmNnumChildren,&numButtons,NULL);
	if (buttonId < numButtons && buttonId >= 0) {
	    XtVaSetValues(menu,XmNmenuHistory,buttons[buttonId],NULL);
	}
    } else {
	fprintf(stderr,"\noptionMenuSet: no subMenu found for option menu");
    }
}

#ifdef __cplusplus
static void medmScheduler(XtPointer cd, XtIntervalId *)
#else
static void medmScheduler(XtPointer cd, XtIntervalId *id)
#endif
{
    PeriodicTask *t = (PeriodicTask *) cd;
    double currentTime = medmTime();

#ifdef MEDM_SOFT_CLOCK
    t->tenthSecond += 0.1;
    if (currentTime != t->systemTime) {
	t->systemTime = currentTime;
	t->tenthSecond = 0.0;
    }
#endif

  /* poll channel access connection event every one tenth of a second */
#ifdef __MONITOR_CA_PEND_EVENT__
    {
	double t;
	t = medmTime();
	ca_pend_event(0.00000001);
	t = medmTime() - t;
	if (t > 0.5) {
	    printf("medmScheduler : time used by ca_pend_event = %8.1f\n",t);
	}
    }
#else
    ca_pend_event(0.00000001);
#endif

  /* wake up any periodic task which is time out */
    if (updateTaskStatus.periodicTaskCount > 0) { 
	DisplayInfo *d = displayInfoListHead->next;
	while (d) {
	    if (d->periodicTaskCount > 0) {
		UpdateTask *pt = d->updateTaskListHead.next;
		if (pt->nextExecuteTime < currentTime) {
		    updateTaskMarkTimeout(pt,currentTime);
		}
	    }
	    d = d->next;
	}
    }  
    if ((updateTaskStatus.updateRequestQueued > 0) && 
      (!updateTaskStatus.workProcId)) {
	updateTaskStatus.workProcId =
	  XtAppAddWorkProc(appContext,updateTaskWorkProc,&updateTaskStatus);
    }
    t->id = XtAppAddTimeOut(appContext,100,medmScheduler,cd);
}

#ifdef MEDM_SOFT_CLOCK
double medmTime() {
    return task.systemTime + task.tenthSecond;
}
#else
double medmTime() {
    struct timeval tp;
    if (gettimeofday(&tp,NULL)) fprintf(stderr,"Failed!\n");
    return (double) tp.tv_sec + (double) tp.tv_usec*1e-6;
}
#endif

/* 
 *  ---------------------------
 *  routines for update tasks
 *  ---------------------------
 */

void updateTaskInit(DisplayInfo *displayInfo) {
    UpdateTask *pt = &(displayInfo->updateTaskListHead);
    pt->executeTask = NULL;
    pt->destroyTask = NULL;
    pt->clientData = NULL;
    pt->timeInterval = 3600.0;            /* pull every hours */
    pt->nextExecuteTime = medmTime() + pt->timeInterval;
    pt->displayInfo = displayInfo;
    pt->next = NULL;
    pt->executeRequestsPendingCount = 0;
    displayInfo->updateTaskListTail = pt;

    if (!moduleInitialized) medmInitSharedDotC();
}

UpdateTask *updateTaskAddTask(
  DisplayInfo *displayInfo, 
  DlObject *rectangle,
  void (*executeTask)(XtPointer),
  XtPointer clientData) {

    UpdateTask *pt;
    if (displayInfo) {
	pt = (UpdateTask *) malloc(sizeof(UpdateTask));
	if (pt == NULL) return pt;
	pt->executeTask = executeTask;
	pt->destroyTask = NULL;
	pt->name = NULL;
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
	pt->overlapped = True;  /* make the default is True */
	pt->opaque = True;      /* don't draw the background */
	displayInfo->updateTaskListTail->next = pt;
	displayInfo->updateTaskListTail = pt;

	if (pt->timeInterval > 0.0) {
	    displayInfo->periodicTaskCount++;
	    updateTaskStatus.periodicTaskCount++;
	    if (pt->nextExecuteTime < displayInfo->updateTaskListHead.nextExecuteTime) {
		displayInfo->updateTaskListHead.nextExecuteTime = pt->nextExecuteTime;
	    }
	}
	updateTaskStatus.taskCount++;
	return pt;
    } else {
	return NULL;
    }
}  

void updateTaskDeleteTask(UpdateTask *pt) {
    UpdateTask *tmp;
    if (pt == NULL) return;
    tmp = &(pt->displayInfo->updateTaskListHead);
    while (tmp->next) {
	if (tmp->next == pt) {
	    tmp->next = pt->next;
	    if (pt == pt->displayInfo->updateTaskListTail) {
		pt->displayInfo->updateTaskListTail = tmp;
	    }
	    if (pt->timeInterval > 0.0) {
		pt->displayInfo->periodicTaskCount--;
		updateTaskStatus.periodicTaskCount--;
	    }
	    updateTaskStatus.taskCount--;
	    free((char *)pt);
	    break;
	}
    }
}

void updateTaskDeleteAllTask(UpdateTask *pt) {
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
	    printf("updateTaskDeleteAllTask : time used by ca_pend_event = %8.1f\n",t);
	} 
    }
#else
    ca_pend_event(CA_PEND_EVENT_TIME);
#endif
}
  
int updateTaskMarkTimeout(UpdateTask *pt, double currentTime) {
    UpdateTask *head = &(pt->displayInfo->updateTaskListHead);
    UpdateTask *tmp = head->next;
    int count = 0;
  /* reset the nextExecuteTime for the display an hour later */
    head->nextExecuteTime = currentTime + head->timeInterval;
    while (tmp) {
      /* if periodic task */
	if (tmp->timeInterval > 0.0) {
	  /* mark if the task is time out already */
	    if (currentTime > tmp->nextExecuteTime) {
		count++;
		if (tmp->executeRequestsPendingCount > 0) {
		    updateTaskStatus.periodicUpdateDiscardCount++;
		} else {
		    updateTaskStatus.periodicUpdateRequestCount++;
		    updateTaskStatus.updateRequestQueued++;
		}
		tmp->executeRequestsPendingCount++;
		tmp->nextExecuteTime += tmp->timeInterval;
	      /* retrieve the closest next execute time */
		if (tmp->nextExecuteTime < head->nextExecuteTime) {
		    head->nextExecuteTime = tmp->nextExecuteTime;
		}
	    }
	}
	tmp = tmp->next;
    }
    return count;
}   

void updateTaskMarkUpdate(UpdateTask *pt) {
    if (pt->executeRequestsPendingCount > 0) {
	updateTaskStatus.updateDiscardCount++;
    } else {
	updateTaskStatus.updateRequestCount++;
	updateTaskStatus.updateRequestQueued++;
    }
    pt->executeRequestsPendingCount++;
}

void updateTaskSetScanRate(UpdateTask *pt, double timeInterval) {
    UpdateTask *head = &(pt->displayInfo->updateTaskListHead);
    double currentTime = medmTime();

  /*
   * increase or decrease the periodic task count depends
   * on the condition
   */
    if ((pt->timeInterval == 0.0) && (timeInterval != 0.0)) {
	pt->displayInfo->periodicTaskCount++;
	updateTaskStatus.periodicTaskCount++;
    } else
      if ((pt->timeInterval != 0.0) && (timeInterval == 0.0)) {
	  pt->displayInfo->periodicTaskCount--;
	  updateTaskStatus.periodicTaskCount--;
      }

  /*
   * set up the next scan time for this task, if it
   * this is the sooner one, set it to the display
   * scan time too.
   */
    pt->timeInterval = timeInterval;
    pt->nextExecuteTime = currentTime + pt->timeInterval;
    if (pt->nextExecuteTime < head->nextExecuteTime) {
	head->nextExecuteTime = pt->nextExecuteTime;
    }
}

void updateTaskAddExecuteCb(UpdateTask *pt, void (*executeTaskCb)(XtPointer)) {
    pt->executeTask = executeTaskCb;
}

void updateTaskAddDestroyCb(UpdateTask *pt, void (*destroyTaskCb)(XtPointer)) {
    pt->destroyTask = destroyTaskCb;
}

Boolean updateTaskWorkProc(XtPointer cd) {
    UpdateTaskStatus *ts = (UpdateTaskStatus *) cd;
    UpdateTask *t = ts->nextToServe;
    double endTime;
   
    endTime = medmTime() + 0.05; 
 
    do { 
	if (ts->updateRequestQueued <=0) {
	    ts->workProcId = 0; 
	    return True;
	} 
      /* if no valid update task, find one */
	if (t == NULL) {
	    DisplayInfo *d = displayInfoListHead->next;
	    if (d == NULL) {
	      /* no display, tell Xt LIB that this routine is finished */
		ts->workProcId = 0;
		return True;
	    }
	  /* find the next update task */
	    while (d) {
		if (t = d->updateTaskListHead.next)
		  break;
		d = d->next;
	    }
	    if (t == NULL) {
	      /* no update task, tell Xt LIB that this routine is finished */
		ts->workProcId = 0;
		return True;
	    }
	    ts->nextToServe = t;
	}

      /* Now, at least one update task found */
      /* find one of which executeRequestsPendingCount > 0 */ 
	while (t->executeRequestsPendingCount <=0 ) {
	    DisplayInfo *d = t->displayInfo;
	    t = t->next;
	    while (t == NULL) {
	      /* end of the update task for this display */
	      /* check the next display.                 */
		d = d->next;
		if (d == NULL) {
		    d = displayInfoListHead->next;
		}
		t = d->updateTaskListHead.next;
	    }      
	    if (t == ts->nextToServe) {
	      /* check all display, no update is needed */
		ts->workProcId = 0;
		return True;
	    }
	}
	ts->nextToServe = t;

      /* repaint the selected region */
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
		medmPrintf("medmRepaintRegion : XPolygonRegion() return NULL\n");
		ts->workProcId = 0;
		return True;
	    }

	    XSetClipRectangles(display,gc,0,0,&t->rectangle,1,YXBanded);
	    if (!t->opaque)
	      XCopyArea(display,pDI->drawingAreaPixmap, XtWindow(pDI->drawingArea),gc,
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
	  /* release the clipping region */
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
  
      /* find next to serve */
	t = ts->nextToServe;
	t->executeRequestsPendingCount = 0;
	while (t->executeRequestsPendingCount <=0 ) {
	    DisplayInfo *d = t->displayInfo;
	    t = t->next;
	    while (t == NULL) {
	      /* end of the update task for this display */
	      /* check the next display.                 */
		d = d->next;
		if (d == NULL) {
		    d = displayInfoListHead->next;
		}
		t = d->updateTaskListHead.next;
	    }
	    if (t == ts->nextToServe) {
	      /* check all display, no update is needed */
		ts->workProcId = 0;
		return True;
	    }
	}
	ts->nextToServe = t;
    } while (endTime > medmTime());
    return False;
}

void updateTaskRepaintRegion(DisplayInfo *displayInfo, Region *region) {
    UpdateTask *t = displayInfo->updateTaskListHead.next;
    while (t) {
	if (XRectInRegion(*region, t->rectangle.x, t->rectangle.y,
	  t->rectangle.width, t->rectangle.height) != RectangleOut) {
	    if (t->executeTask)
	      t->executeTask(t->clientData);
	}
	t = t->next;
    }
}

void updateTaskAddNameCb(UpdateTask *pt, void (*nameCb)(XtPointer, char **, short *, int *)) {
    pt->name = nameCb;
}
