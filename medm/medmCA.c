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

#define DEBUG_FD_REGISTRATION 0

#include "medm.h"

static void medmUpdateGraphicalInfoCb(struct event_handler_args args);
static void medmUpdateChannelCb(struct event_handler_args args);
static void medmCAFdRegistrationCb( void *dummy, int fd, int condition);
static void medmProcessCA(XtPointer, int *, XtInputId *);
static void medmAddUpdateRequest(Channel *);
Boolean medmWorkProc(XtPointer);
static void medmRepaintRegion(Channel *);

#define CA_PAGE_SIZE 100
#define CA_PAGE_COUNT 10
#define TIME_STRING_MAX 81     /* Maximum length of timestamp string */
#define CA_PEND_IO_TIME 30.0

typedef struct _CATask {
    int freeListSize;
    int freeListCount;
    int *freeList;
    Channel **pages;
    int pageCount;
    int pageSize;
    int nextPage;
    int nextFree;
    int channelCount;
    int channelConnected;
    int caEventCount;
} CATask;

CATask caTask;

void CATaskGetInfo(int *channelCount, int *channelConnected, int *caEventCount)
{
    *channelCount = caTask.channelCount;
    *channelConnected = caTask.channelConnected;
    *caEventCount = caTask.caEventCount;
    caTask.caEventCount = 0;
    return;
}

static void medmCAExceptionHandlerCb(struct exception_handler_args args)
{
#define MAX_EXCEPTIONS 25    
    static int nexceptions=0;
    static int ended=0;

    if(ended) return;
    if(nexceptions++ > MAX_EXCEPTIONS) {
	ended=1;
	medmPostMsg(1,"medmCAExceptionHandlerCb: Channel Access Exception:\n"
	  "Too many exceptions [%d]\n"
	  "No more will be handled\n"
	  "Please fix the problem and restart MEDM\n",
	  MAX_EXCEPTIONS);
	ca_add_exception_event(NULL, NULL);
	return;
    }
    
    medmPostMsg(1,"medmCAExceptionHandlerCb: Channel Access Exception:\n"
      "  Channel Name: %s\n"
      "  Native Type: %s\n"
      "  Native Count: %hu\n"
      "  Access: %s%s\n"
      "  IOC: %s\n"
      "  Message: %s\n"
      "  Context: %s\n"
      "  Requested Type: %s\n"
      "  Requested Count: %ld\n"
      "  Source File: %s\n"
      "  Line number: %u\n",
      args.chid?ca_name(args.chid):"Unavailable",
      args.chid?dbr_type_to_text(ca_field_type(args.chid)):"Unavailable",
      args.chid?ca_element_count(args.chid):0,
      args.chid?(ca_read_access(args.chid)?"R":""):"Unavailable",
      args.chid?(ca_write_access(args.chid)?"W":""):"",
      args.chid?ca_host_name(args.chid):"Unavailable",
      ca_message(args.stat)?ca_message(args.stat):"Unavailable",
      args.ctx?args.ctx:"Unavailable",
      dbr_type_to_text(args.type),
      args.count,
      args.pFile?args.pFile:"Unavailable",
      args.pFile?args.lineNo:0);
}

int CATaskInit()
{
    caTask.freeListSize = CA_PAGE_SIZE;
    caTask.freeListCount = 0;
    caTask.freeList = (int *) malloc(sizeof(int) * CA_PAGE_SIZE);
    if (caTask.freeList == NULL) {
	medmPostMsg(1,"CATaskInit: Memory allocation error\n");
	return -1;
    }
    caTask.pages = (Channel **) malloc(sizeof(Channel *) * CA_PAGE_COUNT);
    if (caTask.pages == NULL) {
	medmPostMsg(1,"CATaskInit: Memory allocation error\n");
	return -1;
    }
    caTask.pageCount = 1;
    caTask.pageSize = CA_PAGE_COUNT;
  /* allocate the page */
    caTask.pages[0] = (Channel *) malloc(sizeof(Channel) * CA_PAGE_SIZE);
    if (caTask.pages[0] == NULL) {
	medmPostMsg(1,"CATaskInit: Memory allocation error\n");
	return -1;
    }
    caTask.nextFree = 0;
    caTask.nextPage = 0;
    caTask.channelCount = 0;
    caTask.channelConnected = 0;
    caTask.caEventCount = 0;
    return ECA_NORMAL;
}

void caTaskDelete() {
    if (caTask.freeList) {
	free((char *)caTask.freeList);
	caTask.freeList = NULL;
	caTask.freeListCount = 0;
	caTask.freeListSize = 0;
    }  
    if (caTask.pageCount) {
	int i;
	for (i=0; i < caTask.pageCount; i++) {
	    if (caTask.pages[i])
	      free((char *)caTask.pages[i]);
	}
	free((char *)caTask.pages);
	caTask.pages = NULL;
	caTask.pageCount = 0;
    } 
}

Channel *getChannelFromRecord(Record *pRecord)
{
    Channel *pCh;

    if(!pRecord) return (Channel *)0;
    pCh = &((caTask.pages[pRecord->caId/CA_PAGE_SIZE])
      [pRecord->caId % CA_PAGE_SIZE]);
    return pCh;
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

    status = CATaskInit();
    return status;
}

void medmCATerminate()
{

    caTaskDelete();
  /* cancel registration of the CA file descriptors */
    SEVCHK(ca_add_fd_registration(medmCAFdRegistrationCb,NULL),
      "\ndmTerminateCA:  error removing CA's fd from X");
  /* and close channel access */
#ifdef __MONITOR_CA_PEND_EVENT__
    {
	double t;
	t = medmTime();
	ca_pend_event(20.0*CA_PEND_EVENT_TIME);   /* don't allow early returns */
	t = medmTime() - t;
	if (t > 0.5) {
	    printf("medmCATerminate : time used by ca_pend_event = %8.1f\n",t);
	}
    }
#else
    ca_pend_event(20.0*CA_PEND_EVENT_TIME);   /* don't allow early returns */
#endif



    SEVCHK(ca_task_exit(),"\ndmTerminateCA: error exiting CA");

}

#ifdef __cplusplus
static void medmCAFdRegistrationCb( void *, int fd, int condition)
#else
static void medmCAFdRegistrationCb( void *dummy, int fd, int condition)
#endif
{
    int currentNumInps;

#define NUM_INITIAL_FDS 100
#ifdef WIN32
    int medmInputMask=XtInputReadWinsock;
#else
    int medmInputMask=XtInputReadMask;
#endif
    
    typedef struct {
        XtInputId inputId;
        int fd;
    } InputIdAndFd;
    static InputIdAndFd *inp = NULL;
    static int maxInps = 0, numInps = 0;
    int i, j, k;
    
    if (inp == NULL && maxInps == 0) {
      /* First time through */
	inp = (InputIdAndFd *) calloc(1,NUM_INITIAL_FDS*sizeof(InputIdAndFd));
	maxInps = NUM_INITIAL_FDS;
	numInps = 0;
    }
    
    if (condition) {
      /* Add new fd */
	if (numInps < maxInps-1) {
	    
	    inp[numInps].fd = fd;
	    inp[numInps].inputId  = XtAppAddInput(appContext,fd,
	      (XtPointer)medmInputMask,
	      medmProcessCA,(XtPointer)NULL);
	    numInps++;
	} else {
	    medmPostMsg(0,"dmRegisterCA: info: realloc-ing input fd's array");
	    
	    maxInps = 2*maxInps;
#if defined(__cplusplus) && !defined(__GNUG__)
	    inp = (InputIdAndFd *) realloc((malloc_t)inp,maxInps*sizeof(InputIdAndFd));
#else
	    inp = (InputIdAndFd *) realloc(inp,maxInps*sizeof(InputIdAndFd));
#endif
	    inp[numInps].fd = fd;
	    inp[numInps].inputId  = XtAppAddInput(appContext,fd,
	      (XtPointer)medmInputMask,
	      medmProcessCA,(XtPointer)NULL);
	    numInps++;
	}
    } else {
	currentNumInps = numInps;
	
      /* Remove old fd */
	for (i = 0; i < numInps; i++) {
	    if (inp[i].fd == fd) {
		XtRemoveInput(inp[i].inputId);
		inp[i].inputId = (XtInputId)NULL;
		inp[i].fd = (int)NULL;
		currentNumInps--;
	    }
	}
	
      /* Remove holes in the array */
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

#if DEBUG_FD_REGISTRATION
    printf("\ndmRegisterCA: fd=%d condition=%d ConnectionNumber=%d "
      "numInps = %d\n\t",
      fd,condition,ConnectionNumber(display),numInps);
    for (i = 0; i < maxInps; i++)
      printf("%d ",inp[i].fd);
    printf("\n");
#endif
}

#ifdef __cplusplus
static void medmProcessCA(XtPointer, int *, XtInputId *)
#else
static void medmProcessCA(XtPointer dummy1, int *dummy2, XtInputId *dummy3)
#endif
{
#ifdef __MONITOR_CA_PEND_EVENT__
    {
	double t;
	t = medmTime();
	ca_pend_event(CA_PEND_EVENT_TIME);
	t = medmTime() - t;
	if (t > 0.5) {
	    printf("medmProcessCA : time used by ca_pend_event = %8.1f\n",t);
	}
    }
#else
    ca_pend_event(CA_PEND_EVENT_TIME);
#endif
}

static void medmReplaceAccessRightsEventCb(struct access_rights_handler_args args)
{
    Channel *pCh = (Channel *) ca_puser(args.chid);

    caTask.caEventCount++;
    if (globalDisplayListTraversalMode != DL_EXECUTE) return;
#if 1
    if ((!pCh) || (!pCh->chid)) return;
#endif

    pCh->pr->readAccess = ca_read_access(pCh->chid);
    pCh->pr->writeAccess = ca_write_access(pCh->chid);
    if (pCh->pr->updateValueCb) 
      pCh->pr->updateValueCb((XtPointer)pCh->pr); 
}

void medmConnectEventCb(struct connection_handler_args args) {
    int status;
    Channel *pCh = (Channel *) ca_puser(args.chid);

    caTask.caEventCount++;
    if (globalDisplayListTraversalMode != DL_EXECUTE) return;
#if 1
    if ((pCh == NULL) || (pCh->chid == NULL)) return;
#endif

    if ((args.op == CA_OP_CONN_UP) && (ca_read_access(pCh->chid))) {
      /* get the graphical information every time a channel is connected
	 or reconnected. */
	status = ca_array_get_callback(dbf_type_to_DBR_CTRL(ca_field_type(args.chid)),
	  1, args.chid, medmUpdateGraphicalInfoCb, NULL);
	if (status != ECA_NORMAL) {
	    medmPostMsg(0,"medmConnectEventCb: ca_array_get_callback: %s\n",
	      ca_message(status));
	}
    }
    if ((args.op == CA_OP_CONN_UP) && (pCh->previouslyConnected == False)) {
	status = ca_replace_access_rights_event(pCh->chid,medmReplaceAccessRightsEventCb);
	if (status != ECA_NORMAL) {
	    medmPostMsg(0,"medmConnectEventCb: ca_replace_access_rights_event: %s\n",
	      ca_message(status));
	}
#ifdef __USING_TIME_STAMP__
	status = ca_add_array_event(
	  dbf_type_to_DBR_TIME(ca_field_type(pCh->chid)),
	  ca_element_count(pCh->chid),pCh->chid,
	  medmUpdateChannelCb, pCh, 0.0,0.0,0.0, &(pCh->evid));
#else
	status = ca_add_array_event(
	  dbf_type_to_DBR_STS(ca_field_type(pCh->chid)),
	  ca_element_count(pCh->chid),pCh->chid,
	  medmUpdateChannelCb, pCh, 0.0,0.0,0.0, &(pCh->evid));
#endif
	if (status != ECA_NORMAL) {
	    medmPostMsg(0,"medmConnectEventCb: ca_add_array_event: %s\n",
	      ca_message(status));
	}
	pCh->previouslyConnected = True;
	pCh->pr->elementCount = ca_element_count(pCh->chid);
	pCh->pr->dataType = ca_field_type(args.chid);
	pCh->pr->connected = True;
	caTask.channelConnected++;
    } else {
	if (args.op == CA_OP_CONN_UP) {
	    pCh->pr->connected = True;
	    caTask.channelConnected++;
	} else {
	    pCh->pr->connected = False;
	    caTask.channelConnected--;
	}   
	if (pCh->pr->updateValueCb)
	  pCh->pr->updateValueCb((XtPointer)pCh->pr); 
    }
}


static void medmUpdateGraphicalInfoCb(struct event_handler_args args) {
    int nBytes;
    int i;
    Channel *pCh = (Channel *) ca_puser(args.chid);
    Record *pr = pCh->pr;

    if (globalDisplayListTraversalMode != DL_EXECUTE) return;
#if 1
    if ((!pCh) || (!pCh->chid)) return;
    if (!args.dbr) return;
#endif

    caTask.caEventCount++;
    nBytes = dbr_size_n(args.type, args.count);
    memcpy((void *)&(pCh->info),args.dbr,nBytes);
    switch (ca_field_type(args.chid)) {
    case DBF_STRING :
	pr->value = 0.0;
	pr->hopr = 0.0;
	pr->lopr = 0.0;
	pr->precision = 0;
	break;
    case DBF_ENUM :
	pr->value = (double) pCh->info.e.value;
	pr->hopr = (double) pCh->info.e.no_str - 1.0;
	pr->lopr = 0.0;
	pr->precision = 0;
	for (i = 0; i < pCh->info.e.no_str; i++) { 
	    pr->stateStrings[i] = pCh->info.e.strs[i];
	}
	break;
    case DBF_CHAR :
	pr->value = (double) pCh->info.c.value;
	pr->hopr = (double) pCh->info.c.upper_disp_limit;
	pr->lopr = (double) pCh->info.c.lower_disp_limit;
	pr->precision = 0;
	break;
    case DBF_INT :
	pr->value = (double) pCh->info.i.value;
	pr->hopr = (double) pCh->info.i.upper_disp_limit;
	pr->lopr = (double) pCh->info.i.lower_disp_limit;
	pr->precision = 0;
	break;
    case DBF_LONG :
	pr->value = (double) pCh->info.l.value;
	pr->hopr = (double) pCh->info.l.upper_disp_limit;
	pr->lopr = (double) pCh->info.l.lower_disp_limit;
	pr->precision = 0;
	break;
    case DBF_FLOAT :
	pr->value = (double) pCh->info.f.value;
	pr->hopr = (double) pCh->info.f.upper_disp_limit;
	pr->lopr = (double) pCh->info.f.lower_disp_limit;
	pr->precision = pCh->info.f.precision;
	break;
    case DBF_DOUBLE :
	pr->value = (double) pCh->info.d.value;
	pr->hopr = (double) pCh->info.d.upper_disp_limit;
	pr->lopr = (double) pCh->info.d.lower_disp_limit;
	pr->precision = pCh->info.f.precision;
	break;
    default :
	medmPostMsg(0,"medmUpdateGraphicalInfoCb: Unknown data type\n");
	return;
    }
    if (pr->precision < 0) {
	medmPostMsg(0,"medmUpdateGraphicalInfoCb: pv = \"%s\" precision = %d\n",
	  ca_name(pCh->chid), pr->precision);
	pr->precision = 0;
    } else
      if (pr->precision > 17) {
	  medmPostMsg(1,"medmUpdateGraphicalInfoCb: pv = \"%s\" precision = %d\n",
	    ca_name(pCh->chid), pr->precision);
	  pr->precision = 17;
      }
    if (pr->updateGraphicalInfoCb) {
	pr->updateGraphicalInfoCb((XtPointer)pr);
    } else
      if (pr->updateValueCb) {
	  pr->updateValueCb((XtPointer)pr);
      }
}

void medmUpdateChannelCb(struct event_handler_args args) {
    int nBytes;
    Channel *pCh = (Channel *) ca_puser(args.chid);
    Boolean severityChanged = False;
    Boolean zeroAndNoneZeroTransition = False;
    double value;
    Record *pr = pCh->pr;

    if (globalDisplayListTraversalMode != DL_EXECUTE) return;
    if ((!pCh) || (!pCh->chid)) return;
    if (!args.dbr) return;
    caTask.caEventCount++;
    if (ca_read_access(args.chid)) {
      /* if we have the read access */
	nBytes = dbr_size_n(args.type, args.count);
	if (!pCh->data) {
	    pCh->data = (dataBuf *) malloc(nBytes);
	    pCh->size = nBytes;
	    if (!pCh->data) {
		medmPostMsg(1,"medmUpdateChannelCb: Memory allocation error\n");
		return;
	    }
	} else 
	  if (pCh->size < nBytes) {
	      free((char *)pCh->data);
	      pCh->data = (dataBuf *) malloc(nBytes);
	      pCh->size = nBytes;
	      if (pCh->data == NULL) {
		  medmPostMsg(1,"medmUpdateChannelCb: Memory allocation error\n");
		  return;
	      }
	  }
	pr->time = ((dataBuf *) args.dbr)->s.stamp;
	switch (ca_field_type(args.chid)) {
	case DBF_STRING :
	    pr->array = (XtPointer) pCh->data->s.value;
	    break;
	case DBF_ENUM :
	    pr->array = (XtPointer) &(pCh->data->e.value);
	    break;
	case DBF_CHAR :
	    pr->array = (XtPointer) &(pCh->data->c.value);
	    break;
	case DBF_INT :
	    pr->array = (XtPointer) &(pCh->data->i.value);
	    break;
	case DBF_LONG :
	    pr->array = (XtPointer) &(pCh->data->l.value);
	    break;
	case DBF_FLOAT :
	    pr->array = (XtPointer) &(pCh->data->f.value);
	    break;
	case DBF_DOUBLE :
	    pr->array = (XtPointer) &(pCh->data->d.value);
	    break;
	default :
	    break;
	}

	if (ca_field_type(args.chid) == DBF_STRING || ca_element_count(args.chid) > 1) {
	    memcpy((void *)pCh->data,args.dbr,nBytes);
	}
	switch (ca_field_type(args.chid)) {
	case DBF_STRING :
	    value = 0.0;
	    break;
	case DBF_ENUM :
	    value = (double) ((dataBuf *) (args.dbr))->e.value;
	    break;
	case DBF_CHAR :
	    value = (double) ((dataBuf *) (args.dbr))->c.value;
	    break;
	case DBF_INT :
	    value = (double) ((dataBuf *) (args.dbr))->i.value;
	    break;
	case DBF_LONG :
	    value = (double) ((dataBuf *) (args.dbr))->l.value;
	    break;
	case DBF_FLOAT :
	    value = (double) ((dataBuf *) (args.dbr))->f.value;
	    break;
	case DBF_DOUBLE :
	    value = ((dataBuf *) (args.dbr))->d.value;
	    break;
	default :
	    value = 0.0;
	    break;
	}

	if (((value == 0.0) && (pr->value != 0.0)) || ((value != 0.0) && (pr->value == 0.0)))
	  zeroAndNoneZeroTransition = True;
	pr->value = value;

	if (pr->severity != ((dataBuf *) (args.dbr))->d.severity) {
	    pr->severity = ((dataBuf *) (args.dbr))->d.severity;
	    severityChanged = True;
	}
	if (pr->monitorValueChanged && pCh->pr->updateValueCb) {
	    pr->updateValueCb((XtPointer)pr);
	} else
	  if (pr->monitorSeverityChanged && severityChanged && pr->updateValueCb) {
	      pr->updateValueCb((XtPointer)pr);
	  } else
	    if (pr->monitorZeroAndNoneZeroTransition 
	      && zeroAndNoneZeroTransition && pr->updateValueCb) {
		pr->updateValueCb((XtPointer)pr);
	    }
    }
}

int caAdd(char *name, Record *pr) {
    Channel *pCh;
    int status;
    if ((caTask.freeListCount < 1) && (caTask.nextFree >= CA_PAGE_SIZE)) {
      /* if not enough pages, increase number of pages */
	if (caTask.pageCount >= caTask.pageSize) {
	    caTask.pageSize += CA_PAGE_COUNT;
#if defined(__cplusplus) && !defined(__GNUG__)
	    caTask.pages = (Channel **) realloc((malloc_t)caTask.pages,sizeof(Channel *)*caTask.pageSize);
#else
	    caTask.pages = (Channel **) realloc(caTask.pages,sizeof(Channel *)*caTask.pageSize);
#endif
	    if (caTask.pages == NULL) {
		medmPostMsg(1,"caAdd: Memory allocation error\n");
		return -1;
	    }
	}
      /* add one more page */
	caTask.pages[caTask.pageCount] = (Channel *) malloc(sizeof(Channel) * CA_PAGE_SIZE);
	if (caTask.pages[caTask.pageCount] == NULL) {
	    medmPostMsg(1,"caAdd: Memory allocation error\n");
	    return -1;
	}
	caTask.pageCount++;
	caTask.nextPage++;
	caTask.nextFree=0;
    }
    if (caTask.nextFree < CA_PAGE_SIZE) {
	pCh = &((caTask.pages[caTask.nextPage])[caTask.nextFree]);
	pCh->caId = caTask.nextPage * CA_PAGE_SIZE + caTask.nextFree;
	caTask.nextFree++;
    } else {
	int index;
	caTask.freeListCount--;
	index = caTask.freeList[caTask.freeListCount];
	pCh = &((caTask.pages[index/CA_PAGE_SIZE])[index % CA_PAGE_SIZE]);
	pCh->caId = index;
    }

    pCh->data = NULL;
    pCh->chid = NULL;
    pCh->evid = NULL;
    pCh->size = 0;
    pCh->pr = pr;
    pCh->previouslyConnected = False;
    if (strlen(name) > (size_t)0) {
	status = ca_build_and_connect(name,TYPENOTCONN,0,
	  &(pCh->chid),NULL,medmConnectEventCb,pCh);
    } else {
	status = ca_build_and_connect(" ",TYPENOTCONN,0,
	  &(pCh->chid),NULL,medmConnectEventCb,pCh);
    }
    if (status != ECA_NORMAL) {
	SEVCHK(status,"caAdd : ca_build_and_connect failed\n");
    } else {
      /* Cast to avoid warning from READONLY */
	pCh->pr->name = (char *)ca_name(pCh->chid);
    }
    caTask.channelCount++;
    return pCh->caId;
}

void caDelete(Record *pr) {
    int status;
    Channel *pCh = &((caTask.pages[pr->caId/CA_PAGE_SIZE])[pr->caId % CA_PAGE_SIZE]);
    if (ca_state(pCh->chid) == cs_conn)
      caTask.channelConnected--;
#if 0
    ca_change_connection_event(pCh->chid,NULL);
    ca_replace_access_rights_event(pCh->chid,NULL);
#endif
    if (pCh->evid) {
	status = ca_clear_event(pCh->evid);
	SEVCHK(status,"caDelete : ca_clear_event() failed!");
	if (status != ECA_NORMAL) return;
    }
    pCh->evid = NULL;
    if (pCh->chid) {
	status = ca_clear_channel(pCh->chid);
	SEVCHK(status,"vCA::vCA() : ca_add_exception_event failed!");
	if (status != ECA_NORMAL) return;
    }
    pCh->chid = NULL;
    if (pCh->data) {
	free((char *)pCh->data);
	pCh->data = NULL;
    }
    if (caTask.freeListCount >= caTask.freeListSize) {
	caTask.freeListSize += CA_PAGE_SIZE;
#if defined(__cplusplus) && !defined(__GNUG__)
	caTask.freeList = (int *) realloc((malloc_t)caTask.freeList,
	  sizeof(int)*caTask.freeListSize);
#else
	caTask.freeList = (int *) realloc(caTask.freeList,
	  sizeof(int)*caTask.freeListSize);
#endif
	if (caTask.freeList == NULL) {
	    medmPostMsg(1,"caDelete: Memory allocation error\n");
	    return;
	}
    }
    caTask.freeList[caTask.freeListCount] = pCh->caId;
    caTask.freeListCount++;
    caTask.channelCount--;
}

/* Note that precision is initialized to -1 and some routines depend on this */
static Record nullRecord = {-1,-1,-1,0.0,0.0,0.0,-1,
                            NO_ALARM,NO_ALARM,False,False,False,
                            {NULL,NULL,NULL,NULL,
                             NULL,NULL,NULL,NULL,
                             NULL,NULL,NULL,NULL,
                             NULL,NULL,NULL,NULL},
                            NULL,NULL,
                            {0,0},
                            NULL,NULL,NULL,
                            True,True,True};

Record *medmAllocateRecord(char *name,
  void (*updateValueCb)(XtPointer),
  void (*updateGraphicalInfoCb)(XtPointer),
  XtPointer clientData)
{
    Record *record;
    record = (Record *) malloc(sizeof(Record));
    if (record) {
	*record = nullRecord;
	record->caId = caAdd(name,record);
	record->updateValueCb = updateValueCb;
	record->updateGraphicalInfoCb = updateGraphicalInfoCb;
	record->clientData = clientData;
    }
    return record;
}

void medmDestroyRecord(Record *pr) {
    caDelete(pr);
    *pr = nullRecord;
    free((char *)pr);
}

void medmSendDouble(Record *pr, double data) {
    Channel *pCh = &((caTask.pages[pr->caId/CA_PAGE_SIZE])[pr->caId % CA_PAGE_SIZE]);
    SEVCHK(ca_put(DBR_DOUBLE,pCh->chid,&data),"medmSendDouble: error in ca_put");
    ca_flush_io();
}  

void medmSendCharacterArray(Record *pr, char *data, unsigned long size) {
    Channel *pCh = &((caTask.pages[pr->caId/CA_PAGE_SIZE])[pr->caId % CA_PAGE_SIZE]);
    SEVCHK(ca_array_put(DBR_CHAR,size,pCh->chid,data),
      "medmSendCharacterArray: error in ca_put");
    ca_flush_io();
}

void medmSendString(Record *pr, char *data) {
    Channel *pCh = &((caTask.pages[pr->caId/CA_PAGE_SIZE])[pr->caId % CA_PAGE_SIZE]);
    SEVCHK(ca_put(DBR_STRING,pCh->chid,data),"medmSendString: error in ca_put");
    ca_flush_io();
}

void medmRecordAddUpdateValueCb(Record *pr, void (*updateValueCb)(XtPointer)) {
    pr->updateValueCb = updateValueCb;
}

void medmRecordAddGraphicalInfoCb(Record *pr, void (*updateGraphicalInfoCb)(XtPointer)) {
    pr->updateGraphicalInfoCb = updateGraphicalInfoCb;
}

void popupPvInfo(DisplayInfo *displayInfo)
{
    long now; 
    struct tm *tblock;
    char timeStampStr[TIME_STRING_MAX];
    char tsTxt[32];     /* 28 for TS_TEXT_MMDDYY, 32 for TS_TEXT_MMDDYYYY */
    Record **records;
    chid *chids, chId;
    struct dbr_time_string *timeVals, timeVal;
    int i, j, count, status;
    char string[1024];     /* Danger: Fixed length */
    Record *pR;
    Channel *pCh;
    DlElement *pE;
    XmTextPosition curpos = 0;

  /* Create the dialog box if it has not been created */
    if(!pvInfoS) createPvInfoDlg();

  /* Get the records */
    records = getPvInfoFromDisplay(displayInfo, &count, &pE);
    if(!records) return;   /* (Error messages have been sent) */

  /* Get timestamp */
    time(&now);
    tblock = localtime(&now);
    strftime(timeStampStr,TIME_STRING_MAX,STRFTIME_FORMAT"\n",tblock);
    timeStampStr[TIME_STRING_MAX-1]='0';

  /* Heading */
    sprintf(string,"           PV Information\n\n%s\n",timeStampStr);
    XmTextSetInsertionPosition(pvInfoMessageBox, 0);
    XmTextSetString(pvInfoMessageBox,string);
    curpos+=strlen(string);
    
  /* Allocate space */
    timeVals = (struct dbr_time_string *)calloc(count,
      sizeof(struct dbr_time_string));
    if(!timeVals) {
	medmPostMsg(1,"popupPvInfo: Memory allocation error\n");
	return;
    }
    chids = (chid *)calloc(count,sizeof(chid));
    if(!chids) {
	medmPostMsg(1,"popupPvInfo: Memory allocation error\n");
	free(timeVals);
	return;
    }

  /* Loop over the records to get information */
    for(i=0; i < count; i++) {
      /* Check for a valid record */
	chids[i] = NULL;
	if(records[i]) {
	    pR = records[i];
	    pCh = getChannelFromRecord(pR);
	    if(!pCh) continue;
	    if(!pCh->chid) continue;
	    chId = chids[i] = pCh->chid;
	} else continue;
	
      /* Get the time value as a string */
	status = ca_array_get(DBR_TIME_STRING, 1, chId, &timeVals[i]);
	if(status != ECA_NORMAL) {
	    medmPostMsg(1,"popupPvInfo: ca_get: %s\n",
	      ca_message(status));
	}
    }

  /* Wait for results */
    status=ca_pend_io(CA_PEND_IO_TIME);
    if(status != ECA_NORMAL) {
	medmPostMsg(1,"popupPvInfo: Did not get all the PV information\n");
    }
    
  /* Loop over the records to print information */
    for(i=0; i < count; i++) {
	
      /* Check for a valid record */
	if(!chids[i]) continue;

	chId = chids[i];
	timeVal = timeVals[i];
	pR = records[i];

      /* Items from chid */
	sprintf(string,"%s\n"
	  "======================================\n",
	  ca_name(chId));
	sprintf(string,"%sTYPE: %s\n",string,
	  dbr_type_to_text(ca_field_type(chId)));
	sprintf(string,"%sCOUNT: %hu\n",string,ca_element_count(chId));
	sprintf(string,"%sACCESS: %s%s\n",string,
	  ca_read_access(chId)?"R":"",ca_write_access(chId)?"W":"");
	sprintf(string,"%sIOC: %s\n",string,ca_host_name(chId));
	if(timeVal.value) {
	    if(ca_element_count(chId) == 1) {
		sprintf(string,"%sVALUE: %s\n",string,
		  timeVal.value);
	    } else {
		sprintf(string,"%sFIRST VALUE: %s\n",string,
		  timeVal.value);
	    }
	    sprintf(string,"%sSTAMP: %s\n",string,
	      tsStampToText(&timeVal.stamp,TS_TEXT_MONDDYYYY,tsTxt));
	} else {
		sprintf(string,"%sVALUE: Not known\n");
		sprintf(string,"%sSTAMP: Not known\n");
	}
	XmTextInsert(pvInfoMessageBox, curpos, string);
	curpos+=strlen(string);

      /* Alarms */
	switch(pR->severity) {
	case NO_ALARM:
	    sprintf(string,"ALARM: NO\n");
	    break;
	case MINOR_ALARM:
	    sprintf(string,"ALARM: MINOR\n");
	    break;
	case MAJOR_ALARM:
	    sprintf(string,"ALARM: MAJOR\n");
	    break;
	case INVALID_ALARM:
	    sprintf(string,"ALARM: INVALID\n");
	    break;
	default:
	    sprintf(string,"ALARM: Unknown\n");
	    break;
	}
	XmTextInsert(pvInfoMessageBox, curpos, string);
	curpos+=strlen(string);

      /* Items from record */
	sprintf(string,"\n");
	switch(ca_field_type(chId)) {
        case DBF_STRING: 
	    break;
        case DBF_ENUM: 
	    sprintf(string,"%sSTATES: %d\n",
	      string, (int)(pR->hopr+1.1));
	  /* KE: Bad way to use a double */
	    for (j=0; j <= pR->hopr; j++) {
		sprintf(string,"%sSTATE %2d: %s\n",
		  string, j, pR->stateStrings[j]);
	    }
	    break;
        case DBF_CHAR:  
        case DBF_INT:           
        case DBF_LONG: 
	    sprintf(string,"%sHOPR: %g  LOPR: %g\n",
	      string, pR->hopr ,pR->lopr);
	    break;
        case DBF_FLOAT:  
        case DBF_DOUBLE:
	    if(pR->precision >= 0 && pR->precision <= 17) {
		sprintf(string,"%sPRECISION: %d\n",
		  string, pR->precision);
	    } else {
		sprintf(string,"%sPRECISION: %d [Bad Value]\n",
		  string, pR->precision);
	    }
	    sprintf(string,"%sHOPR: %g  LOPR: %g\n",
	      string, pR->hopr ,pR->lopr);
	    break;
        default         : 
	    break;
	}
	sprintf(string,"%s\n",string);
	XmTextInsert(pvInfoMessageBox, curpos, string);
	curpos+=strlen(string);
    }

  /* Free space */
    free(timeVals);
    free(chids);
    free(records);

    XtSetSensitive(pvInfoS,True);
    XtPopup(pvInfoS,XtGrabNone);
}
