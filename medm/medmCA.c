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
 *
 *****************************************************************************
*/

#include "medm.h"

#define intersect(TP,TL,BR) (((TL.x <= TP.x) && (TP.x <= BR.x)) && ((TL.y <= TP.y) && (TP.y <= BR.y)))
#define INIT_SIZE 256

static void medmUpdateGraphicalInfoCb(struct event_handler_args args);
static void medmUpdateChannelCb(struct event_handler_args args);
static void medmCAFdRegistrationCb( void *dummy, int fd, int condition);
static XtInputCallbackProc medmProcessCA();
static void medmRepaintRegion(Channel *pCh);


static void medmCAExceptionHandlerCb(struct exception_handler_args args) {
  if (args.chid == NULL) {
    medmPrintf("message : %s\n",ca_message(args.stat));
  } else {
    medmPrintf("channel name : %s\nmessage : %s\n\n",
          ca_name(args.chid),ca_message(args.stat));
  }
  medmPostTime();
  return;
}

int medmCAInitialize()
{
  int status;
  /*
   * add CA's fd to X
   */
  status = ca_add_fd_registration(medmCAFdRegistrationCb,NULL);
  if (status != ECA_NORMAL) return status;

  status = ca_add_exception_event(medmCAExceptionHandlerCb, NULL);
  if (status != ECA_NORMAL) return status;

}

void medmCATerminate()
{

   /* cancel registration of the CA file descriptors */
      SEVCHK(ca_add_fd_registration(medmCAFdRegistrationCb,NULL),
                "\ndmTerminateCA:  error removing CA's fd from X");
   /* and close channel access */
      ca_pend_event(20.0*CA_PEND_EVENT_TIME);   /* don't allow early returns */


      SEVCHK(ca_task_exit(),"\ndmTerminateCA: error exiting CA");

}

static void medmCAFdRegistrationCb( void *dummy, int fd, int condition)
{
  int currentNumInps;

#define NUM_INITIAL_FDS 100
typedef struct {
        XtInputId inputId;
        int fd;
} InputIdAndFd;
 static InputIdAndFd *inp = NULL;
 static int maxInps = 0, numInps = 0;
 int i, j, k;



 if (inp == NULL && maxInps == 0) {
/* first time through */
   inp = (InputIdAndFd *) calloc(1,NUM_INITIAL_FDS*sizeof(InputIdAndFd));
   maxInps = NUM_INITIAL_FDS;
   numInps = 0;
 }

 if (condition) {
/*
 * add new fd
 */
   if (numInps < maxInps-1) {

        inp[numInps].fd = fd;
        inp[numInps].inputId  = XtAppAddInput(appContext,fd,
                        (XtPointer)XtInputReadMask,
                        (XtInputCallbackProc)medmProcessCA,(XtPointer)NULL);
        numInps++;

   } else {

fprintf(stderr,"\ndmRegisterCA: info: realloc-ing input fd's array");

        maxInps = 2*maxInps;
        inp = (InputIdAndFd *) realloc(inp,maxInps*sizeof(InputIdAndFd));
        inp[numInps].fd = fd;
        inp[numInps].inputId  = XtAppAddInput(appContext,fd,
                        (XtPointer)XtInputReadMask,
                        (XtInputCallbackProc)medmProcessCA,(XtPointer)NULL);
        numInps++;
   }

 } else {

  currentNumInps = numInps;

/*
 * remove old fd/inputId
 */
   for (i = 0; i < numInps; i++) {
        if (inp[i].fd == fd) {
           XtRemoveInput(inp[i].inputId);
           inp[i].inputId = (XtInputId)NULL;
           inp[i].fd = (int)NULL;
           currentNumInps--;
        }
   }

/* now remove holes in the array */
   i = 0;
   while (i < numInps) {
        if (inp[i].inputId == (XtInputId)NULL) {
           j = i+1;
           k = 0;
           while(inp[j].inputId != (XtInputId)NULL) {
              inp[i+k].inputId = inp[j].inputId;
              inp[i+k].fd = inp[j].fd;
              j++;
              k++;
           }
           i = j-1;
        }
        i++;
   }
   numInps = currentNumInps;

 }

#ifdef DEBUG
fprintf(stderr,"\ndmRegisterCA: numInps = %d\n\t",numInps);
for (i = 0; i < maxInps; i++)
    fprintf(stderr,"%d ",inp[i].fd);
fprintf(stderr,"\n");
#endif

}

static XtInputCallbackProc medmProcessCA()
{
  ca_pend_event(CA_PEND_EVENT_TIME);    /* don't allow early return */
}

static void medmReplaceAccessRightsEventCb(struct access_rights_handler_args args)
{
  Channel *pCh = (Channel *) ca_puser(args.chid);

  if (globalDisplayListTraversalMode != DL_EXECUTE) return;
  if (pCh == NULL) return;
  if (pCh->displayInfo == NULL) return;
  if (pCh->displayInfo->drawingArea == NULL) return;
  medmRepaintRegion(pCh);
}

void medmConnectEventCb(struct connection_handler_args args) {
  Channel *pCh = (Channel *) ca_puser(args.chid);
  if (globalDisplayListTraversalMode != DL_EXECUTE) return;
  if (pCh == NULL) return;
  if (pCh->displayInfo == NULL) return;
  if (pCh->displayInfo->drawingArea == NULL) return;

  if ((args.op == CA_OP_CONN_UP) && (pCh->previouslyConnected == False)) {
    pCh->caStatus = ca_replace_access_rights_event(pCh->chid,medmReplaceAccessRightsEventCb);
    if (pCh->caStatus != ECA_NORMAL) {
      medmPrintf("Error : connectionEventCb : ca_replace_access_rights_event : %s\n",
		   ca_message(pCh->caStatus));
      medmPostTime();
    }
    if (pCh->handleArray == True) {
      pCh->caStatus = ca_add_array_event(
			  dbf_type_to_DBR_TIME(ca_field_type(pCh->chid)),
			  ca_element_count(pCh->chid),pCh->chid,
			  medmUpdateChannelCb, pCh,
			  0.0,0.0,0.0, &(pCh->evid));
    } else {
      /* just ask for one count */
      pCh->caStatus = ca_add_array_event(
			  dbf_type_to_DBR_TIME(ca_field_type(pCh->chid)),
			  1,pCh->chid,
			  medmUpdateChannelCb, pCh,
			  0.0,0.0,0.0, &(pCh->evid));
    }
    if (pCh->caStatus != ECA_NORMAL) {
      medmPrintf("Error : connectionEventCb : ca_add_event : %s\n",
		   ca_message(pCh->caStatus));
      medmPostTime();
    }
    pCh->previouslyConnected = True;
  } else {
  }
  if (ca_read_access(pCh->chid)) {
    /* get the graphical information every time a channel is connected
       or reconnected. */
    pCh->caStatus = ca_array_get_callback(dbf_type_to_DBR_CTRL(ca_field_type(args.chid)),
		      1, args.chid, medmUpdateGraphicalInfoCb, NULL);
    if (pCh->caStatus != ECA_NORMAL) {
      medmPrintf("Error : connectionEventCb : ca_get_callback : %s\n",
		ca_message(pCh->caStatus));
      medmPostTime();
    }
  }
  medmRepaintRegion(pCh);
}

static void medmUpdateGraphicalInfoCb(struct event_handler_args args) {
  int nBytes;
  Channel *pCh = (Channel *) ca_puser(args.chid);
  char *tmp;
  if (pCh->displayInfo->drawingArea == NULL) return;

  if (pCh->info == NULL) {
    pCh->info = (infoBuf *) malloc(sizeof(infoBuf));
    if (pCh->info == NULL) {
      medmPrintf("medmUpdateGraphicalInfoCb : memory allocation error\n");
      return;
    }
  } 
  nBytes = dbr_size_n(args.type, args.count);
  memcpy((void *)pCh->info,args.dbr,nBytes);
  switch (ca_field_type(args.chid)) {
  case DBF_STRING :
    pCh->value = 0.0;
    pCh->hopr = 0.0;
    pCh->lopr = 0.0;
    pCh->precision = 0;
    break;
  case DBF_ENUM :
    pCh->value = (double) pCh->info->e.value;
    pCh->hopr = (double) pCh->info->e.no_str - 1.0;
    pCh->lopr = 0.0;
    pCh->precision = 0;
    break;
  case DBF_CHAR :
    pCh->value = (double) pCh->info->c.value;
    pCh->hopr = (double) pCh->info->c.upper_disp_limit;
    pCh->lopr = (double) pCh->info->c.lower_disp_limit;
    pCh->precision = 0;
    break;
  case DBF_INT :
    pCh->value = (double) pCh->info->i.value;
    pCh->hopr = (double) pCh->info->i.upper_disp_limit;
    pCh->lopr = (double) pCh->info->i.lower_disp_limit;
    pCh->precision = 0;
    break;
  case DBF_LONG :
    pCh->value = (double) pCh->info->l.value;
    pCh->hopr = (double) pCh->info->l.upper_disp_limit;
    pCh->lopr = (double) pCh->info->l.lower_disp_limit;
    pCh->precision = 0;
    break;
  case DBF_FLOAT :
    pCh->value = (double) pCh->info->f.value;
    pCh->hopr = (double) pCh->info->f.upper_disp_limit;
    pCh->lopr = (double) pCh->info->f.lower_disp_limit;
    pCh->precision = pCh->info->f.precision;
    break;
  case DBF_DOUBLE :
    pCh->value = (double) pCh->info->d.value;
    pCh->hopr = (double) pCh->info->d.upper_disp_limit;
    pCh->lopr = (double) pCh->info->d.lower_disp_limit;
    pCh->precision = pCh->info->f.precision;
    break;
  default :
    medmPostMsg("medmUpdateGraphicalInfoCb : unknown data type\n");
    return;
  }
  if (pCh->updateGraphicalInfoCb) {
    pCh->updateGraphicalInfoCb(pCh);
  } else {
    medmRepaintRegion(pCh);
  }
}

void medmUpdateChannelCb(struct event_handler_args args) {
  int nBytes;
  Channel *pCh = (Channel *) ca_puser(args.chid);
  unsigned short valueChanged = False;
  unsigned short severityChanged = False;
  unsigned short visibilityChanged = False;
  double tmp;
  short severity;

  if (pCh->displayInfo->drawingArea == NULL) return;
  if (ca_read_access(args.chid)) {
    /* if we have the read access */
    nBytes = dbr_size_n(args.type, args.count);
    if (pCh->data == NULL) {
      pCh->data = (dataBuf *) malloc(nBytes);
      pCh->size = nBytes;
    } else 
    if (pCh->size < nBytes) {
      free(pCh->data);
      pCh->data = (dataBuf *) malloc(nBytes);
      pCh->size = nBytes;
    }
    if (pCh->data == NULL) {
      medmPrintf("medmUpdateChannelCb : memory allocation error\n");
      return;
    }
    memcpy((void *)pCh->data,args.dbr,nBytes);
    switch (ca_field_type(args.chid)) {
    case DBF_STRING :
      tmp = 0.0;
      if (strcmp(pCh->stringValue,pCh->data->s.value)) {
	strcpy(pCh->stringValue,pCh->data->s.value);
        valueChanged = True;
      }
      severity = pCh->data->s.severity;
      break;
    case DBF_ENUM :
      tmp = (double) pCh->data->e.value;
      severity = pCh->data->e.severity;
      break;
    case DBF_CHAR :
      tmp = (double) pCh->data->c.value;
      severity = pCh->data->c.severity;
      break;
    case DBF_INT :
      tmp = (double) pCh->data->i.value;
      severity = pCh->data->i.severity;
      break;
    case DBF_LONG :
      tmp = (double) pCh->data->l.value;
      severity = pCh->data->l.severity;
      break;
    case DBF_FLOAT :
      tmp = (double) pCh->data->f.value;
      severity = pCh->data->f.severity;
      break;
    case DBF_DOUBLE :
      tmp = (double) pCh->data->d.value;
      severity = pCh->data->d.severity;
      break;
    default :
      break;
    }
    if (((tmp == 0.0) && (pCh->value != 0.0)) || ((tmp != 0.0) && (pCh->value == 0.0)))
      visibilityChanged = True;
    if (pCh->value != tmp) {
      pCh->value = tmp;
      valueChanged = True;
    }
    if (pCh->severity != severity) {
      pCh->severity = severity;
      severityChanged = True;
    }
    if (pCh->updateDataCb) pCh->updateDataCb(pCh);
    if ((!pCh->ignoreValueChanged) && (valueChanged))
      medmRepaintRegion(pCh);
    else
    if ((pCh->clrmod == ALARM) && (severityChanged)) 
      medmRepaintRegion(pCh);
    else
    if ((pCh->vismod != V_STATIC) && (visibilityChanged))
      medmRepaintRegion(pCh);
  }
}

void medmDisconnectChannel(Channel *pCh) {
  if (pCh->chid) {
    ca_puser(pCh->chid) = NULL;
    ca_clear_channel(pCh->chid);
    ca_pend_event(CA_PEND_EVENT_TIME);
    pCh->chid = NULL;
    if (pCh->data) free(pCh->data);
    if (pCh->info) free(pCh->info);
  }
}

/*
#include "medm.h"

#include <Xm/DrawingAP.h>

#include <X11/keysym.h>

char *stripChartWidgetName = "stripChart";

*/
Channel *allocateChannel(
  DisplayInfo *displayInfo)
{
  Channel *pCh = (Channel *) malloc(sizeof(Channel));
  pCh->modified = NOT_MODIFIED;
  pCh->previouslyConnected = FALSE;
  pCh->chid = NULL;
  pCh->evid = NULL;
  pCh->self = NULL;
  pCh->value = 0.0;
  pCh->displayedValue = 0.0;
  strcpy(pCh->stringValue," ");
  pCh->updateAllowed = True;
  pCh->numberStateStrings = 0;
  pCh->stateStrings = NULL;
  pCh->hopr = 0.0;
  pCh->lopr = 0.0;
  pCh->precision = 0;
  pCh->oldIntegerValue = 0;
  pCh->status = 0;
  pCh->severity = NO_ALARM;
  pCh->oldSeverity = NO_ALARM;
  pCh->next = NULL;
  pCh->prev = channelAccessMonitorListTail;
  pCh->displayInfo = displayInfo;
  pCh->clrmod = STATIC;
  pCh->vismod = V_STATIC;
  pCh->label = LABEL_NONE;
  pCh->fontIndex = 0;
  pCh->dlAttr = NULL;
  pCh->xrtData = NULL;
  pCh->xrtDataSet = 1;
  pCh->trace = 0;
  pCh->xyChannelType = CP_XYScalarX;
  pCh->other = NULL;
  pCh->shadowBorderWidth = 2;

  /* initialize all the callback routines to NULL */
  pCh->updateChannelCb = NULL;
  pCh->updateDataCb = NULL;
  pCh->updateGraphicalInfoCb = NULL;
  pCh->destroyChannel = NULL;

  pCh->lastUpdateRequest = MEDMNoOp;
  /* initialize the data pointer to NULL */
  pCh->updateList = NULL;
  pCh->data = NULL;
  pCh->info = NULL;
  pCh->size = 1;

  pCh->handleArray = False;
  pCh->ignoreValueChanged = False;
  pCh->opaque = False;

  /* add this ca monitor data node into monitor list */
  channelAccessMonitorListTail->next = pCh;
  channelAccessMonitorListTail= pCh;

  return (pCh);
}

static void medmRepaintRegion(Channel *pCh) {
  DlRectangle *pR = (DlRectangle *) pCh->specifics;
  DisplayInfo *pDI = pCh->displayInfo;
  Display *display = XtDisplay(pDI->drawingArea);
  GC gc = pDI->gc;
  XPoint points[4];
  Region region;
  Channel *pTmp;
  XRectangle clipRect;

  points[0].x = pR->object.x;
  points[0].y = pR->object.y;
  points[1].x = pR->object.x + pR->object.width;
  points[1].y = pR->object.y;
  points[2].x = pR->object.x + pR->object.width;
  points[2].y = pR->object.y + pR->object.height;
  points[3].x = pR->object.x;
  points[3].y = pR->object.y + pR->object.height;
  region = XPolygonRegion(points,4,EvenOddRule);
  if (region == NULL) {
    medmPrintf("medmRepaintRegion : XPolygonRegion() return NULL\n");
    return;
  }

  /* clip the region */
  clipRect.x = pR->object.x;
  clipRect.y = pR->object.y;
  clipRect.width = pR->object.width;
  clipRect.height = pR->object.height;

  XSetClipRectangles(display,gc,0,0,&clipRect,1,YXBanded);

  if (!pCh->opaque)
    XCopyArea(display,pCh->displayInfo->drawingAreaPixmap,
        XtWindow(pCh->displayInfo->drawingArea),
        pCh->displayInfo->pixmapGC,
        pR->object.x, pR->object.y,
        pR->object.width, pR->object.height,
        pR->object.x, pR->object.y);

  pTmp = channelAccessMonitorListHead->next;
  while (pTmp != NULL) {
    if (pTmp->displayInfo == pCh->displayInfo) {
      if (XRectInRegion(region,
          ((DlRectangle *)pTmp->specifics)->object.x,
          ((DlRectangle *)pTmp->specifics)->object.y,
          ((DlRectangle *)pTmp->specifics)->object.width,
          ((DlRectangle *)pTmp->specifics)->object.height)!=RectangleOut) {
        if (pTmp->updateChannelCb) {
            pTmp->updateChannelCb(pTmp);
        }
      }
    }
    pTmp = pTmp->next;
  }
  /* release the clipping region */
  XSetClipOrigin(display,gc,0,0);
  XSetClipMask(display,gc,None);
  if (region) XDestroyRegion(region);
}
