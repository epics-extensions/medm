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

#define DEBUG_PVINFO 0
#define DEBUG_FD_REGISTRATION 0
#define DEBUG_CHANNEL_CB 0
#define DEBUG_ADD 0
#define DEBUG_INPUT_ID 0
#define DEBUG_ERASE 0
#define DEBUG_CONNECTION 0
#define DEBUG_TIMESTAMP 0
#define DEBUG_RETRY 0
#define DEBUG_SLIDER 0
#define DEBUG_LARGE_ARRAY 0

/* Keep in mind the interface does not respond for this time and that
   most of the search requests are at the beginning of the sequence */
#define RETRY_TIMEOUT 1.0

#define NOT_AVAILABLE "Not available"
#define PVINFO_TIMEOUT 60000     /* ms */

#include "envDefs.h"     /* Must be before medm.h for WIN32 */
#include "medm.h"

typedef struct {
    Record *record;
    chid pvChid;
    Boolean pvOk;
    Boolean timeOk;
    struct dbr_time_string timeVal;
    chid descChid;
    Boolean descOk;
    char descVal[MAX_STRING_SIZE];
#if defined(DBR_CLASS_NAME) && DO_RTYP
    Boolean rtypOk;
    char rtypVal[MAX_STRING_SIZE];
#endif
} PvInfo;

static PvInfo *pvInfo = NULL;
static DlElement *pvInfoElement = NULL;
static int nPvInfoCbs = 0;
static int nPvInfoPvs = 0;
static XtIntervalId pvInfoTimeoutId;
static Boolean pvInfoTimerOn = False;
static unsigned long pvInfoTime;

static void pvInfoDescGetCb(struct event_handler_args args);
#if defined(DBR_CLASS_NAME) && DO_RTYP
static void pvInfoRtypGetCb(struct event_handler_args args);
#endif
static void pvInfoTimeGetCb(struct event_handler_args args);
static void pvInfoWriteInfo(void);
static void pvInfoTimeout(XtPointer cd, XtIntervalId *id);
static void medmConnectEventCb(struct connection_handler_args);
static void medmUpdateGraphicalInfoCb(struct event_handler_args args);
static void medmUpdateChannelCb(struct event_handler_args args);
static void medmReplaceAccessRightsEventCb(
  struct access_rights_handler_args args);
static void medmCAFdRegistrationCb(void *dummy, int fd, int condition);
static void medmProcessCA(XtPointer, int *, XtInputId *);
static int caTaskInit();
static void caTaskDelete();
static int caAdd(char *name, Record *pr);
static void caDelete(Record *pr);
static Channel *getChannelFromRecord(Record *pRecord);

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

static CATask caTask;

void caTaskGetInfo(int *channelCount, int *channelConnected, int *caEventCount)
{
    *channelCount = caTask.channelCount;
    *channelConnected = caTask.channelConnected;
    *caEventCount = caTask.caEventCount;
    caTask.caEventCount = 0;
    return;
}

static int caTaskInit()
{
    caTask.freeListSize = CA_PAGE_SIZE;
    caTask.freeListCount = 0;
    caTask.freeList = (int *)malloc(sizeof(int) * CA_PAGE_SIZE);
    if(caTask.freeList == NULL) {
	medmPostMsg(1,"caTaskInit: Memory allocation error\n");
	return -1;
    }
    caTask.pages = (Channel **)malloc(sizeof(Channel *) * CA_PAGE_COUNT);
    if(caTask.pages == NULL) {
	medmPostMsg(1,"caTaskInit: Memory allocation error\n");
	return -1;
    }
    caTask.pageCount = 1;
    caTask.pageSize = CA_PAGE_COUNT;
  /* allocate the page */
    caTask.pages[0] = (Channel *)malloc(sizeof(Channel) * CA_PAGE_SIZE);
    if(caTask.pages[0] == NULL) {
	medmPostMsg(1,"caTaskInit: Memory allocation error\n");
	return -1;
    }
    caTask.nextFree = 0;
    caTask.nextPage = 0;
    caTask.channelCount = 0;
    caTask.channelConnected = 0;
    caTask.caEventCount = 0;
    return ECA_NORMAL;
}

static void caTaskDelete()
{
    if(caTask.freeList) {
	free((char *)caTask.freeList);
	caTask.freeList = NULL;
	caTask.freeListCount = 0;
	caTask.freeListSize = 0;
    }
    if(caTask.pageCount) {
	int i;
	for (i=0; i < caTask.pageCount; i++) {
	    if(caTask.pages[i])
	      free((char *)caTask.pages[i]);
	}
	free((char *)caTask.pages);
	caTask.pages = NULL;
	caTask.pageCount = 0;
    }
}

static Channel *getChannelFromRecord(Record *pRecord)
{
    Channel *pCh;

    if(!pRecord) return (Channel *)0;
    pCh = &((caTask.pages[pRecord->caId/CA_PAGE_SIZE])
      [pRecord->caId % CA_PAGE_SIZE]);
    return pCh;
}

/* Find an unconnected PV and attempt to connect to it.  That should
 * restart the searches for all other unresolved PVs. */
void retryConnections(void)
{
    int i,j;
    const char *pvname=NULL;
    chid retryChid;
    int status;

#if DEBUG_RETRY
    print("retryConnections:\n");
    print(" freeListSize=%d freeListCount=%d\n",
      caTask.freeListSize,caTask.freeListCount);
    print(" pageSize=%d pageCount=%d\n",
      caTask.pageSize,caTask.pageCount);
    print(" nextpage=%d nextFree=%d\n",
      caTask.nextPage,caTask.nextFree);
    print(" channelCount=%d channelConnected=%d\n",
      caTask.channelCount,caTask.channelConnected);
    if(caTask.nextPage != caTask.pageCount-1) {
	print(" caTask.nextPage != caTask.pageCount-1\n");
    }
#else
  /* Check if all channels are connected */
    if(caTask.channelCount == caTask.channelConnected) {
	medmPostMsg(1,"retryConnections: All channels are connected\n");
	XBell(display, 50);
	return;
    }
#endif

  /* Find an unconnected PV */
    for(i=0; i < caTask.pageCount; i++) {
	int jmax=(i == caTask.nextPage)?caTask.nextFree:CA_PAGE_SIZE;
	for(j=0; j < jmax; j++) {
	    Channel *pCh=&caTask.pages[i][j];
	    if(pCh->chid && ca_state(pCh->chid) != cs_conn) {
		pvname=ca_name(pCh->chid);
		break;
	    }
	}
	if(pvname) break;
    }
#if DEBUG_RETRY
    print(" Found %s\n",pvname?pvname:"Not found");
    if(!pvname) return;
#else
    if(!pvname) {
	medmPostMsg(1,"retryConnections: Failed to find unconnected PV\n");
	return;
    }
#endif

  /* Search */
    status=ca_search_and_connect(pvname,&retryChid,NULL,NULL);
    if(status != ECA_NORMAL) {
	medmPostMsg(1,"retryConnections: ca_search failed for %s: %s\n",
	  pvname, ca_message(status));
    }

  /* Wait.  The searches will only continue for this time.  Keep the
   * time short as the interface is frozen, and most of the searches
   * occur at the start of the sequence.  Testing indicated:
   *
   * RETRY_TIMEOUT Searches
   *      30          15
   *       5          10
   *       3           9
   *       2           9
   *       1           8
   *
   * but this may vary owing to tuning and may change with new releases.
   */
    ca_pend_io(RETRY_TIMEOUT);

  /* Clear the channel */
    status = ca_clear_channel(retryChid);
    if(status != ECA_NORMAL) {
	medmPostMsg(1,"retryConnections: ca_clear_channel failed for %s: %s\n",
	  pvname, ca_message(status));
    }
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
      args.chid?dbf_type_to_text(ca_field_type(args.chid)):"Unavailable",
      args.chid?ca_element_count(args.chid):0,
      args.chid?(ca_read_access(args.chid)?"R":"None"):"Unavailable",
      args.chid?(ca_write_access(args.chid)?"W":""):"",
      args.chid?ca_host_name(args.chid):"Unavailable",
      ca_message(args.stat)?ca_message(args.stat):"Unavailable",
      args.ctx?args.ctx:"Unavailable",
      dbf_type_to_text(args.type),
      args.count,
      args.pFile?args.pFile:"Unavailable",
      args.pFile?args.lineNo:0);
}

int medmCAInitialize()
{
    int status;
  /*
   * add CA's fd to X
   */
    status = ca_add_fd_registration(medmCAFdRegistrationCb,NULL);
    if(status != ECA_NORMAL) return status;

    status = ca_add_exception_event(medmCAExceptionHandlerCb, NULL);
    if(status != ECA_NORMAL) return status;

    status = caTaskInit();
    return status;
}

void medmCATerminate()
{
    int status;

  /* Cancel registration of the CA file descriptors */
  /* KE: Doesn't cancel it.  The first argument should be NULL for cancel */
  /* And why do we want to cancel it ? */
    status = ca_task_initialize();
    if(status != ECA_NORMAL) {
	medmPostMsg(1,"medmCATerminate: ca_add_fd_registration failed: %s\n",
	  ca_message(status));
    }
  /* Do a pend_event */
  /* KE: Why? */
#ifdef __MONITOR_CA_PEND_EVENT__
    {
	double t;
	t = medmTime();
      /* Don't allow early returns */
	ca_pend_event(20.0*CA_PEND_EVENT_TIME);
	t = medmTime() - t;
	if(t > 0.5) {
	    print("medmCATerminate: time used by ca_pend_event = %8.1f\n",t);
	}
    }
#else
    ca_pend_event(20.0*CA_PEND_EVENT_TIME);   /* don't allow early returns */
#endif
  /* Close down channel access */
    status = ca_task_exit();
    if(status != ECA_NORMAL) {
	medmPostMsg(1,"medmCATerminate: ca_task_exit failed: %s\n",
	  ca_message(status));
    }
  /* Clean up the  memory allocated for Channel's */
  /* KE: Used to be done first */
    caTaskDelete();
}

static void medmCAFdRegistrationCb(void *user, int fd, int opened)
{
    int currentNumInps;

#define NUM_INITIAL_FDS 100
#ifdef WIN32
    int inputReadMask=XtInputReadWinsock;
#else
    int inputReadMask=XtInputReadMask;
#endif

    typedef struct {
        XtInputId inputId;
        int fd;
    } InputIdAndFd;
    static InputIdAndFd *inp = NULL;
    static int maxInps = 0, numInps = 0;
    int i, j, k;

    UNREFERENCED(user);

    if(inp == NULL && maxInps == 0) {
      /* First time through */
	inp = (InputIdAndFd *) calloc(1,NUM_INITIAL_FDS*sizeof(InputIdAndFd));
	maxInps = NUM_INITIAL_FDS;
	numInps = 0;
    }

    if(opened) {
      /* Add new fd */
	if(numInps < maxInps-1) {

	    inp[numInps].fd = fd;
	    inp[numInps].inputId  = XtAppAddInput(appContext,fd,
	      (XtPointer)inputReadMask,
	      medmProcessCA,(XtPointer)NULL);
#if DEBUG_INPUT_ID
	    print("medmCAFdRegistrationCb: Add fd=%d inpid=%lx\n",
	      inp[numInps].fd,inp[numInps].inputId);
#endif
	    numInps++;
	} else {
	    medmPostMsg(0,"dmRegisterCA: info: realloc-ing input fd's array");

	    maxInps = 2*maxInps;
#if defined(__cplusplus) && !defined(__GNUG__)
	    inp = (InputIdAndFd *) realloc((malloc_t)inp,
	      maxInps*sizeof(InputIdAndFd));
#else
	    inp = (InputIdAndFd *) realloc(inp,maxInps*sizeof(InputIdAndFd));
#endif
	    inp[numInps].fd = fd;
	    inp[numInps].inputId  = XtAppAddInput(appContext,fd,
	      (XtPointer)inputReadMask,
	      medmProcessCA,(XtPointer)NULL);
#if DEBUG_INPUT_ID
	    print("medmCAFdRegistrationCb (Realloc): Add fd=%d inpid=%lx\n",
	      inp[numInps].fd,inp[numInps].inputId);
#endif
	    numInps++;
	}
    } else {
	currentNumInps = numInps;

      /* Remove old fd */
	for (i = 0; i < numInps; i++) {
	    if(inp[i].fd == fd) {
#if DEBUG_INPUT_ID
		print("medmCAFdRegistrationCb: Remove fd=%d inpid=%lx\n",
		  inp[i].fd,inp[i].inputId);
#endif
		XtRemoveInput(inp[i].inputId);
#if DEBUG_INPUT_ID
		print("                        XtRemoveInput finished\n");
#endif
		inp[i].inputId = (XtInputId)NULL;
		inp[i].fd = (int)NULL;
		currentNumInps--;
	    }
	}

      /* Remove holes in the array */
	i = 0;
	while (i < numInps) {
	    if(inp[i].inputId == (XtInputId)NULL) {
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
    print("\ndmRegisterCA: fd=%d opened=%d ConnectionNumber=%d "
      "numInps = %d\n\t",
      fd,opened,ConnectionNumber(display),numInps);
    for (i = 0; i < maxInps; i++)
      print("%d ",inp[i].fd);
    print("\n");
#endif
}

static void medmProcessCA(XtPointer cd, int *source , XtInputId *id)
{
    UNREFERENCED(cd);
    UNREFERENCED(source);
    UNREFERENCED(id);

#ifdef __MONITOR_CA_PEND_EVENT__
    {
	double t;
	t = medmTime();
	ca_pend_event(CA_PEND_EVENT_TIME);
	t = medmTime() - t;
	if(t > 0.5) {
	    print("medmProcessCA: time used by ca_pend_event = %8.1f\n",t);
	}
    }
#else
    ca_pend_event(CA_PEND_EVENT_TIME);
#endif
}

static void medmConnectEventCb(struct connection_handler_args args) {
    int status;
    Channel *pCh = (Channel *)ca_puser(args.chid);

  /* Increment the event counter */
    caTask.caEventCount++;

  /* Check for valid values */
    if(globalDisplayListTraversalMode != DL_EXECUTE) return;
    if(!pCh || !pCh->chid || !pCh->pr) {
	medmPostMsg(0,"medmConnectEventCb: Invalid channel information\n");
	return;
    }

  /* Do a get every time a channel is connected or reconnected and has
   * read access.  The get will cause the graphical info callback to
   * be called */
    if(args.op == CA_OP_CONN_UP && ca_read_access(pCh->chid)) {
	status = ca_array_get_callback(
	  dbf_type_to_DBR_CTRL(ca_field_type(args.chid)),
	  1, args.chid, medmUpdateGraphicalInfoCb, NULL);
	if(status != ECA_NORMAL) {
	    medmPostMsg(0,"medmConnectEventCb: "
	      "ca_array_get_callback [%s]:\n %s\n",
	      ca_name(pCh->chid)?ca_name(pCh->chid):"Unknown",
	      ca_message(status));
#if DEBUG_CONNECTION
# if 0
	    system("netstat | grep iocacis");
# endif
	    print("  pCh->chid %s args.chid\n",
	      pCh->chid == args.chid?"==":"!=");
	    print(
	      "  Channel Name: %s\n"
	      "  State: %s\n"
	      "  Native Type: %s\n"
	      "  Native Count: %hu\n"
	      "  Access: %s%s\n"
	      "  IOC: %s\n",
	      args.chid?ca_name(args.chid):"Unavailable",
	      args.chid?ca_state(args.chid) == cs_never_conn?"Never":
	      ca_state(args.chid) == cs_prev_conn?"Prev":
	      ca_state(args.chid) == cs_conn?"Conn":
	      ca_state(args.chid) == cs_closed?"Closed":"Unknown":"Unavailable",
	      args.chid?dbf_type_to_text(ca_field_type(args.chid)):"Unavailable",
	      args.chid?ca_element_count(args.chid):0,
	      args.chid?(ca_read_access(args.chid)?"R":"None"):"Unavailable",
	      args.chid?(ca_write_access(args.chid)?"W":""):"",
	      args.chid?ca_host_name(args.chid):"Unavailable");
#endif
	}
    }

  /* Handle four cases: connected or not and previously connected or not */
    if(args.op == CA_OP_CONN_UP) {
      /* Connected */
	if(pCh->previouslyConnected == False) {
	  /* Connected and not previously connected */
	  /* Set these first so they will be right for the updateValueCb */
	    pCh->pr->elementCount = ca_element_count(pCh->chid);
	    pCh->pr->dataType = ca_field_type(args.chid);
	    pCh->pr->connected = True;
	    caTask.channelConnected++;
	  /* Add the access-rights-change callback and the
	     significant-change (value, alarm or severity)
	     callback. Don't call the updateValueCb because
	     ca_replace_access_rights_event will call it. */
	    status = ca_replace_access_rights_event(
	      pCh->chid,medmReplaceAccessRightsEventCb);
	    if(status != ECA_NORMAL) {
		medmPostMsg(0,"medmConnectEventCb: "
		  "ca_replace_access_rights_event [%s]: %s\n",
		  ca_name(pCh->chid)?ca_name(pCh->chid):"Unknown",
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
	    if(status != ECA_NORMAL) {
	      /* Set the pointer to NULL in case CA didn't.  We don't
                 want to use it or clear it later. */
#if DEBUG_CONNECTION
		if(!pCh->evid) {
		    print("medmConnectEventCb: ca_add_array_event: \n"
		      "  status[%d] != ECA_NORMAL and pCh->evid != NULL\n",
		      status);
		}
#endif
		pCh->evid = NULL;
		medmPostMsg(0,"medmConnectEventCb: "
		  "ca_add_array_event [%s]:\n %s\n",
		  ca_name(pCh->chid)?ca_name(pCh->chid):"Unknown",
		  ca_message(status));
#if DEBUG_CONNECTION
# if 0
		system("netstat | grep iocacis");
# endif
		print("  pCh->chid %s args.chid\n",
		  pCh->chid == args.chid?"==":"!=");
		print(
		  "  Channel Name: %s\n"
		  "  State: %s\n"
		  "  Native Type: %s\n"
		  "  Native Count: %hu\n"
		  "  Access: %s%s\n"
		  "  IOC: %s\n",
		  args.chid?ca_name(args.chid):"Unavailable",
		  args.chid?ca_state(args.chid) == cs_never_conn?"Never":
		  ca_state(args.chid) == cs_prev_conn?"Prev":
		  ca_state(args.chid) == cs_conn?"Conn":
		  ca_state(args.chid) == cs_closed?"Closed":"Unknown":"Unavailable",
		  args.chid?dbf_type_to_text(ca_field_type(args.chid)):"Unavailable",
		  args.chid?ca_element_count(args.chid):0,
		  args.chid?(ca_read_access(args.chid)?"R":"None"):"Unavailable",
		  args.chid?(ca_write_access(args.chid)?"W":""):"",
		  args.chid?ca_host_name(args.chid):"Unavailable");
#endif
	    }
	  /* Set this one last so ca_replace_access_rights_event can
             use the old value */
	    pCh->previouslyConnected = True;
	} else {
	  /* Connected and previously connected */
	    pCh->pr->connected = True;
	    caTask.channelConnected++;
	    if(pCh->pr->updateValueCb)
	      pCh->pr->updateValueCb((XtPointer)pCh->pr);
	}
    } else {
      /* Not connected */
	if(pCh->previouslyConnected == False) {
	  /* Not connected and not previously connected */
	  /* Probably doesn't happen -- if CA can't connect, this
	   *   routine never gets called */
	    pCh->pr->connected = False;
	} else {
	  /* Not connected but previously connected */
	    pCh->pr->connected = False;
	    caTask.channelConnected--;
	    if(pCh->pr->updateValueCb)
	      pCh->pr->updateValueCb((XtPointer)pCh->pr);
	}
    }
}

static void medmUpdateGraphicalInfoCb(struct event_handler_args args) {
    int nBytes;
    int i;
    Channel *pCh = (Channel *)ca_puser(args.chid);
    Record *pr;

  /* Increment the event counter */
    caTask.caEventCount++;

  /* Check for valid values */
  /* Same as for updateChannelCb */
  /* Don't need to check for read access, checking ECA_NORMAL is enough */
    if(globalDisplayListTraversalMode != DL_EXECUTE) return;
    if(args.status != ECA_NORMAL) return;
    if(!args.dbr) {
	medmPostMsg(0,"medmUpdateGraphicalInfoCb: Invalid data [%s]\n",
	  ca_name(args.chid)?ca_name(args.chid):"Name Unknown");
	return;
    }
    if(!pCh || !pCh->chid || !pCh->pr) {
	medmPostMsg(0,"medmUpdateGraphicalInfoCb: Invalid channel information [%s]\n",
	  ca_name(args.chid)?ca_name(args.chid):"Name Unknown");
	return;
    }
    if(pCh->chid != args.chid) {
	medmPostMsg(0,"medmUpdateGraphicalInfoCb: chid from args [%x] "
	  "does not match chid from channel [%x]\n"
	  "  [%s]\n",
	  args.chid,
	  pCh->chid,
	  ca_name(args.chid)?ca_name(args.chid):"Name Unknown");
	return;
    }
    pr = pCh->pr;

  /* Copy the information to the Channel struct */
    nBytes = dbr_size_n(args.type, args.count);
    memcpy((void *)&(pCh->info),args.dbr,nBytes);

  /* Handle hopr, lopr, and precision */
    switch (ca_field_type(args.chid)) {
    case DBF_STRING:
	pr->value = 0.0;
	pr->hopr = 0.0;
	pr->lopr = 0.0;
	pr->precision = 0;
	break;
    case DBF_ENUM:
	pr->value = (double) pCh->info.e.value;
	pr->hopr = (double) pCh->info.e.no_str - 1.0;
	pr->lopr = 0.0;
	pr->precision = 0;
	for (i = 0; i < pCh->info.e.no_str; i++) {
	    pr->stateStrings[i] = pCh->info.e.strs[i];
	}
	break;
    case DBF_CHAR:
	pr->value = (double) pCh->info.c.value;
	pr->hopr = (double) pCh->info.c.upper_disp_limit;
	pr->lopr = (double) pCh->info.c.lower_disp_limit;
	pr->precision = 0;
	break;
    case DBF_INT:
	pr->value = (double) pCh->info.i.value;
	pr->hopr = (double) pCh->info.i.upper_disp_limit;
	pr->lopr = (double) pCh->info.i.lower_disp_limit;
	pr->precision = 0;
	break;
    case DBF_LONG:
	pr->value = (double) pCh->info.l.value;
	pr->hopr = (double) pCh->info.l.upper_disp_limit;
	pr->lopr = (double) pCh->info.l.lower_disp_limit;
	pr->precision = 0;
	break;
    case DBF_FLOAT:
	pr->value = (double) pCh->info.f.value;
	pr->hopr = (double) pCh->info.f.upper_disp_limit;
	pr->lopr = (double) pCh->info.f.lower_disp_limit;
	pr->precision = pCh->info.f.precision;
	break;
    case DBF_DOUBLE:
	pr->value = (double) pCh->info.d.value;
	pr->hopr = (double) pCh->info.d.upper_disp_limit;
	pr->lopr = (double) pCh->info.d.lower_disp_limit;
	pr->precision = pCh->info.f.precision;
	break;
    default:
	medmPostMsg(0,"medmUpdateGraphicalInfoCb: Unknown data type\n");
	return;
    }

  /* Adjust the precision */
    if(pr->precision < 0) {
	medmPostMsg(0,"medmUpdateGraphicalInfoCb: "
	  "pv = \"%s\" precision = %d\n",
	  ca_name(pCh->chid), pr->precision);
	pr->precision = 0;
    } else if(pr->precision > 17) {
	medmPostMsg(1,"medmUpdateGraphicalInfoCb: "
	  "pv = \"%s\" precision = %d\n",
	  ca_name(pCh->chid), pr->precision);
	pr->precision = 17;
    }

  /* Call the record's graphical info callback or its update value callback */
    if(pr->updateGraphicalInfoCb) {
	pr->updateGraphicalInfoCb((XtPointer)pr);
    } else if(pr->updateValueCb) {
	pr->updateValueCb((XtPointer)pr);
    }
}

static void medmUpdateChannelCb(struct event_handler_args args) {
    Channel *pCh = (Channel *)ca_puser(args.chid);
    Boolean statusChanged = False;
    Boolean severityChanged = False;
    Boolean zeroAndNoneZeroTransition = False;
    Record *pr;
    double value;
    int nBytes;

#if DEBUG_CHANNEL_CB || DEBUG_LARGE_ARRAY
    const char *pvname=ca_name(args.chid);
    print("medmUpdateChannelCb: %s\n",pvname);
    if(args.status != ECA_NORMAL) {
	char buf[80];
	envGetConfigParam(&EPICS_CA_MAX_ARRAY_BYTES,80,buf);
	buf[79]='\0';
	print("EPICS_CA_MAX_ARRAY_BYTES: %s\n",buf);
    }
#endif

  /* Increment the event counter */
    caTask.caEventCount++;

  /* Check for valid values */
  /* Same as for updateGraphicalInfoCb */
  /* Don't need to check for read access, checking ECA_NORMAL is enough */
    if(globalDisplayListTraversalMode != DL_EXECUTE) return;
    if(args.status != ECA_NORMAL) {
	medmPostMsg(0,"medmUpdateChannelCb: Bad status [%d] for %s: %s\n",
	  args.status,
	  ca_name(args.chid)?ca_name(args.chid):"Name Unknown",
	  ca_message(args.status));
	return;
    }
    if(!args.dbr) {
	medmPostMsg(0,"medmUpdateChannelCb: Invalid data for [%s]\n",
	  ca_name(args.chid)?ca_name(args.chid):"Name Unknown");
	return;
    }
    if(!pCh || !pCh->chid || !pCh->pr) {
	medmPostMsg(0,"medmUpdateChannelCb: "
	  "Invalid channel information for [%s]\n",
	  ca_name(args.chid)?ca_name(args.chid):"Name Unknown");
	return;
    }
    if(pCh->chid != args.chid) {
	medmPostMsg(0,"medmUpdateChannelCb: chid from args [%x] "
	  "does not match chid from channel [%x]\n"
	  "  [%s]\n",
	  args.chid,
	  pCh->chid,
	  ca_name(args.chid)?ca_name(args.chid):"Name Unknown");
	return;
    }
    pr = pCh->pr;

  /* Allocate space for the data */
    nBytes = dbr_size_n(args.type, args.count);
    if(!pCh->data) {
      /* No previous data, allocate */
	pCh->data = (dataBuf *)malloc(nBytes);
	pCh->size = nBytes;
	if(!pCh->data) {
	    medmPostMsg(1,"medmUpdateChannelCb: Memory allocation error [%s]\n",
	      ca_name(args.chid)?ca_name(args.chid):"Name Unknown");
	    return;
	}
    } else if(pCh->size < nBytes) {
      /* Previous data exists but is too small
       * Free it and allocate it again */
	free((char *)pCh->data);
	pCh->data = (dataBuf *)malloc(nBytes);
	pCh->size = nBytes;
	if(pCh->data == NULL) {
	    medmPostMsg(1,"medmUpdateChannelCb: Memory reallocation error"
	      " [%s]\n",
	      ca_name(args.chid)?ca_name(args.chid):"Name Unknown");
	    return;
	}
    }

#ifdef __USING_TIME_STAMP__
  /* Copy the returned time stamp to the Record */
    pr->time = ((dataBuf *)args.dbr)->s.stamp;
#endif


  /* Set the array pointer in the Record to point to the value member of the
   *   appropriate struct stored in the Channel
   * (The returned data is not stored there yet) */
    switch (ca_field_type(args.chid)) {
    case DBF_STRING:
	pr->array = (XtPointer)pCh->data->s.value;
	break;
    case DBF_ENUM:
	pr->array = (XtPointer)&(pCh->data->e.value);
	break;
    case DBF_CHAR:
	pr->array = (XtPointer)&(pCh->data->c.value);
	break;
    case DBF_INT:
	pr->array = (XtPointer)&(pCh->data->i.value);
	break;
    case DBF_LONG:
	pr->array = (XtPointer)&(pCh->data->l.value);
	break;
    case DBF_FLOAT:
	pr->array = (XtPointer)&(pCh->data->f.value);
	break;
    case DBF_DOUBLE:
	pr->array = (XtPointer)&(pCh->data->d.value);
	break;
    default:
	break;
    }

  /* For strings and arrays copy the returned data to the Channel
   * (The space is allocated but the data is not stored otherwise) */
    if(ca_field_type(args.chid) == DBF_STRING ||
      ca_element_count(args.chid) > 1) {
	memcpy((void *)pCh->data, args.dbr, nBytes);
    }

  /* Copy the value from the returned data to the Record */
    switch (ca_field_type(args.chid)) {
    case DBF_STRING:
	value = 0.0;
	break;
    case DBF_ENUM:
	value = (double)((dataBuf *)(args.dbr))->e.value;
	break;
    case DBF_CHAR:
	value = (double)((dataBuf *)(args.dbr))->c.value;
	break;
    case DBF_INT:
	value = (double)((dataBuf *)(args.dbr))->i.value;
	break;
    case DBF_LONG:
	value = (double)((dataBuf *)(args.dbr))->l.value;
	break;
    case DBF_FLOAT:
	value = (double)((dataBuf *)(args.dbr))->f.value;
	break;
    case DBF_DOUBLE:
	value = ((dataBuf *)(args.dbr))->d.value;
#if DEBUG_SLIDER
	if(value > 150.0 || value < -150.0) {
	    print("medmUpdateChannelCb: %g\n",value);
	}
#endif
	break;
    default:
	value = 0.0;
	break;
    }

  /* Mark zero to nonzero transition */
    if(((value == 0.0) && (pr->value != 0.0)) ||
      ((value != 0.0) && (pr->value == 0.0)))
      zeroAndNoneZeroTransition = True;

  /* Mark status changed */
    if(pr->status != ((dataBuf *)(args.dbr))->d.status) {
	pr->status = ((dataBuf *)(args.dbr))->d.status;
	statusChanged = True;
    }

  /* Mark severity changed */
    if(pr->severity != ((dataBuf *)(args.dbr))->d.severity) {
	pr->severity = ((dataBuf *)(args.dbr))->d.severity;
	severityChanged = True;
    }

  /* Set the new value into the record */
    pr->value = value;

#if DEBUG_ERASE
    print("medmUpdateChannelCb: [%x]%s %g\n",
      pr, pr->name, pr->value);
#endif

  /* Call the update value callback if there is a monitored change */
    if(pCh->pr->updateValueCb) {
	if(pr->monitorValueChanged) {
	    pr->updateValueCb((XtPointer)pr);
	} else if(pr->monitorStatusChanged && statusChanged) {
	    pr->updateValueCb((XtPointer)pr);
	} else if(pr->monitorSeverityChanged && severityChanged) {
	    pr->updateValueCb((XtPointer)pr);
	} else if(pr->monitorZeroAndNoneZeroTransition &&
	  zeroAndNoneZeroTransition) {
	    pr->updateValueCb((XtPointer)pr);
	}
    }
}

static void medmReplaceAccessRightsEventCb(
  struct access_rights_handler_args args)
{
    Channel *pCh = (Channel *) ca_puser(args.chid);
    Boolean readAccessChangedToRead = False;

  /* Increment the event counter */
    caTask.caEventCount++;

  /* Check for valid values */
    if(globalDisplayListTraversalMode != DL_EXECUTE) return;
    if(!pCh || !pCh->pr) {
	medmPostMsg(0,"medmReplaceAccessRightsEvent: "
	  "Invalid channel information\n");
	return;
    }

  /* Determine if the rights are changing from no read to read */
    if(!pCh->pr->readAccess && ca_read_access(pCh->chid)) {
	readAccessChangedToRead = True;
    }
  /* Change the access rights in the Record */
    pCh->pr->readAccess = ca_read_access(pCh->chid);
    pCh->pr->writeAccess = ca_write_access(pCh->chid);
    if(pCh->pr->updateValueCb)
      pCh->pr->updateValueCb((XtPointer)pCh->pr);

  /* Do a get as in medmConnectEventCb.  The get will cause the
     graphical info callback to be called.  This is necessary for some
     objects if the access rights are changing from no read to
     read. */
    if(pCh->pr->connected && pCh->previouslyConnected
      && readAccessChangedToRead) {
	int status = ca_array_get_callback(
	  dbf_type_to_DBR_CTRL(ca_field_type(args.chid)),
	  1, args.chid, medmUpdateGraphicalInfoCb, NULL);
	if(status != ECA_NORMAL) {
	    medmPostMsg(0,"medmReplaceAccessRightsEventCb: %s\n",
	      ca_message(status));
	}
    }
}

static int caAdd(char *name, Record *pr)
{
    Channel *pCh;
    int status;

    if((caTask.freeListCount < 1) && (caTask.nextFree >= CA_PAGE_SIZE)) {
      /* if not enough pages, increase number of pages */
	if(caTask.pageCount >= caTask.pageSize) {
	    caTask.pageSize += CA_PAGE_COUNT;
#if defined(__cplusplus) && !defined(__GNUG__)
	    caTask.pages = (Channel **)realloc((malloc_t)caTask.pages,
	      sizeof(Channel *)*caTask.pageSize);
#else
	    caTask.pages = (Channel **)realloc(caTask.pages,
	      sizeof(Channel *)*caTask.pageSize);
#endif
	    if(caTask.pages == NULL) {
		medmPostMsg(1,"caAdd: Memory allocation error\n");
		return -1;
	    }
	}
      /* add one more page */
	caTask.pages[caTask.pageCount] = (Channel *)malloc(sizeof(Channel) *
	  CA_PAGE_SIZE);
	if(caTask.pages[caTask.pageCount] == NULL) {
	    medmPostMsg(1,"caAdd: Memory allocation error\n");
	    return -1;
	}
	caTask.pageCount++;
	caTask.nextPage++;
	caTask.nextFree=0;
    }
    if(caTask.nextFree < CA_PAGE_SIZE) {
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

  /* Do the search */
    status = ca_search_and_connect(name, &(pCh->chid), medmConnectEventCb,
      pCh);
    if(status != ECA_NORMAL) {
	medmPostMsg(1,"caAdd: ca_search_and_connect failed: %s\n",
	  ca_message(status));
    } else {
      /* Cast to avoid warning from READONLY */
	pCh->pr->name = (char *)ca_name(pCh->chid);
    }
    caTask.channelCount++;
#if DEBUG_ADD
    print("caAdd: %3d name=%s\n",
      caTask.channelCount, name);
#endif
    return pCh->caId;
}

static void caDelete(Record *pr)
{
    int status;
    Channel *pCh;

    if(!pr) return;
    pCh = &((caTask.pages[pr->caId/CA_PAGE_SIZE])
      [pr->caId % CA_PAGE_SIZE]);
    if(ca_state(pCh->chid) == cs_conn)
      caTask.channelConnected--;
#if 0
  /* KE: This is not necessary according to Jeff */
    ca_change_connection_event(pCh->chid,NULL);
    ca_replace_access_rights_event(pCh->chid,NULL);
#endif
    if(pCh->evid) {
	status = ca_clear_event(pCh->evid);
	if(status != ECA_NORMAL) {
	    medmPostMsg(1,"caDelete: ca_clear_event failed: %s\n",
	      ca_message(status));
	    return;
	}
    }
    pCh->evid = NULL;
    if(pCh->chid) {
	status = ca_clear_channel(pCh->chid);
	if(status != ECA_NORMAL) {
	    medmPostMsg(1,"caDelete: ca_clear_channel failed: %s\n",
	      ca_message(status));
	    return;
	}
    }
    pCh->chid = NULL;
    if(pCh->data) {
	free((char *)pCh->data);
	pCh->data = NULL;
    }
    if(caTask.freeListCount >= caTask.freeListSize) {
	caTask.freeListSize += CA_PAGE_SIZE;
#if defined(__cplusplus) && !defined(__GNUG__)
	caTask.freeList = (int *) realloc((malloc_t)caTask.freeList,
	  sizeof(int)*caTask.freeListSize);
#else
	caTask.freeList = (int *) realloc(caTask.freeList,
	  sizeof(int)*caTask.freeListSize);
#endif
	if(caTask.freeList == NULL) {
	    medmPostMsg(1,"caDelete: Memory allocation error\n");
	    return;
	}
    }
    caTask.freeList[caTask.freeListCount] = pCh->caId;
    caTask.freeListCount++;
    caTask.channelCount--;
#if DEBUG_ADD
    print("caAdd: channelCount=%3d\n",caTask.channelCount);
#endif
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
			  /* Set monitorValueChanged to True, others
                             to false */
                            True,False,False,False};

Record *medmAllocateRecord(char *name, void (*updateValueCb)(XtPointer),
  void (*updateGraphicalInfoCb)(XtPointer), XtPointer clientData)
{
    Record *pR;

  /* Don't allocate a record if the name is blank */
    if(strlen(name) <= (size_t)0) {
	return NULL;
    }

    pR = (Record *)malloc(sizeof(Record));
    if(pR) {
	*pR = nullRecord;
	pR->caId = caAdd(name,pR);
	pR->updateValueCb = updateValueCb;
	pR->updateGraphicalInfoCb = updateGraphicalInfoCb;
	pR->clientData = clientData;
    }
    return pR;
}

Record **medmAllocateDynamicRecords(DlDynamicAttribute *attr,
  void (*updateValueCb)(XtPointer), void (*updateGraphicalInfoCb)(XtPointer),
  XtPointer clientData)
{
    Record **records;
    int i;

    records = (Record **)malloc(MAX_CALC_RECORDS*sizeof(Record *));
    if(!records) return records;
  /* KE: Could put an error message here and also in medmAllocateRecord */

    for(i=0; i < MAX_CALC_RECORDS; i++) {
      /* Only add non-blank records */
	if(*attr->chan[i]) {
	  /* Use graphicalInfoCb only for record[0] */
	    records[i] = medmAllocateRecord(attr->chan[i], updateValueCb,
	      i?NULL:updateGraphicalInfoCb, clientData);
	} else {
	    records[i] = NULL;
	}
    }
    return records;
}

void medmDestroyRecord(Record *pr)
{
    if(pr) {
	caDelete(pr);
	*pr = nullRecord;
	free((char *)pr);
	pr = (Record *)0;
    }
}

void medmSendDouble(Record *pr, double data)
{
    int status;

    Channel *pCh = &((caTask.pages[pr->caId/CA_PAGE_SIZE])
      [pr->caId % CA_PAGE_SIZE]);
    status = ca_put(DBR_DOUBLE,pCh->chid,&data);
#if DEBUG_SLIDER
    if(data > 150.0 || data < -150.0) {
	print("medmSendDouble: %g\n",data);
    }
#endif
    if(status != ECA_NORMAL) {
	medmPostMsg(1,"medmSendDouble: ca_put failed: %s\n",
	  ca_message(status));
    }
    ca_flush_io();
}

void medmSendLong(Record *pr, long data)
{
    int status;

    Channel *pCh = &((caTask.pages[pr->caId/CA_PAGE_SIZE])
      [pr->caId % CA_PAGE_SIZE]);
    status = ca_put(DBR_LONG,pCh->chid,&data);
    if(status != ECA_NORMAL) {
	medmPostMsg(1,"medmSendLong: ca_put failed: %s\n",
	  ca_message(status));
    }
    ca_flush_io();
}

void medmSendCharacterArray(Record *pr, char *data, unsigned long size)
{
    int status;

    Channel *pCh = &((caTask.pages[pr->caId/CA_PAGE_SIZE])
      [pr->caId % CA_PAGE_SIZE]);
    status = ca_array_put(DBR_CHAR,size,pCh->chid,data);
    if(status != ECA_NORMAL) {
	medmPostMsg(1,"medmSendCharacterArray: ca_put failed: %s\n",
	  ca_message(status));
    }
    ca_flush_io();
}

void medmSendString(Record *pr, char *data)
{
    int status;

    Channel *pCh = &((caTask.pages[pr->caId/CA_PAGE_SIZE])
      [pr->caId % CA_PAGE_SIZE]);
    status = ca_put(DBR_STRING,pCh->chid,data);
    if(status != ECA_NORMAL) {
	medmPostMsg(1,"medmSendString: ca_put failed: %s\n",
	  ca_message(status));
    }
    ca_flush_io();
}

void medmRecordAddUpdateValueCb(Record *pr, void (*updateValueCb)(XtPointer))
{
    pr->updateValueCb = updateValueCb;
}

void medmRecordAddGraphicalInfoCb(Record *pr,
  void (*updateGraphicalInfoCb)(XtPointer))
{
    pr->updateGraphicalInfoCb = updateGraphicalInfoCb;
}

/* Control-system dependent PV Info routines */

void popupPvInfo(DisplayInfo *displayInfo)
{
    DlElement *pE;
    Record **records;
    chid chId;
    int i, status;
    Record *pR;
    Channel *pCh;
    char descName[MAX_TOKEN_LENGTH];
    char *pDot;
    double connTimeout;

#if DEBUG_PVINFO
    XUngrabPointer(display,CurrentTime);
#endif

  /* Check if another call is in progress */
    if(pvInfo) {
	medmPostMsg(1,"popupPvInfo: "
	  "Another PV Info request is already in progress\n"
	  "  It is probably having problems\n"
	  "  Wait for it to finish\n");
	return;
    }

  /* Create the dialog box if it has not been created */
    if(!pvInfoS) createPvInfoDlg();

  /* Get the records */
    records = getPvInfoFromDisplay(displayInfo, &nPvInfoPvs, &pE);
    if(!records) return;
    pvInfoElement = pE;

  /* Allocate space */
    pvInfo = (PvInfo *)calloc(nPvInfoPvs, sizeof(PvInfo));
    if(!pvInfo) {
	medmPostMsg(1,"popupPvInfo: Memory allocation error\n");
	if(records) free(records);
	if(pvInfoS && XtIsManaged(pvInfoS)) return;
    }

  /* Loop over the records, initialize, and initiate search for DESC */
    for(i=0; i < nPvInfoPvs; i++) {
      /* Initialize */
	pvInfo[i].pvChid = NULL;
	pvInfo[i].pvOk = False;
	pvInfo[i].timeOk = False;
	pvInfo[i].descChid = NULL;
	pvInfo[i].descOk = False;
	strcpy(pvInfo[i].descVal, NOT_AVAILABLE);
#if defined(DBR_CLASS_NAME) && DO_RTYP
	pvInfo[i].rtypOk = False;
	strcpy(pvInfo[i].rtypVal, NOT_AVAILABLE);
#endif

      /* Check for a valid record */
	if(records[i]) {
	    pR = pvInfo[i].record = records[i];
	    pCh = getChannelFromRecord(pR);
	    if(!pCh) continue;
	    if(!pCh->chid) continue;
	    chId = pvInfo[i].pvChid = pCh->chid;
	} else continue;
	pvInfo[i].pvOk = True;

      /* Don't try the others unless the PV is connected */
	if(ca_state(chId) != cs_conn || !ca_read_access(chId))
	  continue;

      /* Construct the DESC name */
	strcpy(descName,ca_name(chId));
	pDot = strchr(descName,'.');
	if(pDot) {
	  /* Assume it is a name with a field and replace the field
	   * with DESC */
	    strcpy(pDot,".DESC");
	} else {
	  /* Append .DESC */
	    strcat(descName,".DESC");
	}
      /* Search for the DESC */
	status = ca_search(descName, &pvInfo[i].descChid);
	if(status == ECA_NORMAL) {
	    pvInfo[i].descOk = True;
	} else {
	    medmPostMsg(1,"popupPvInfo: DESC: ca_search for %s: %s\n",
	      descName, ca_message(status));
	}
    }

  /* Free the records, they are now stored in pvInfo */
    if(records) free(records);

  /* Wait for the searches (Timeouts should be uncommon) */
    status=ca_pend_io(CA_PEND_IO_TIME);
    if(status != ECA_NORMAL) {
	medmPostMsg(1,"popupPvInfo: Waited %g seconds.  "
	  "Did not find the DESC information (%s).\n",
	  CA_PEND_IO_TIME, descName);
    }

  /* Loop over the records and do the gets */
    nPvInfoCbs = 0;
    for(i=0; i < nPvInfoPvs; i++) {
	if(!pvInfo[i].pvOk) continue;

      /* Don't try the others unless the PV is connected */
	chId = pvInfo[i].pvChid;
	if(ca_state(chId) != cs_conn || !ca_read_access(chId))
	  continue;

      /* Get the DESC */
	if(ca_state(pvInfo[i].descChid) == cs_conn &&
	  ca_read_access(pvInfo[i].descChid)) {
	  /* Do the get */
	    status = ca_get_callback(DBR_STRING, pvInfo[i].descChid,
	      pvInfoDescGetCb, &pvInfo[i]);
	    if(status == ECA_NORMAL) {
		nPvInfoCbs++;
	    } else {
		pvInfo[i].descOk = False;
		medmPostMsg(1,"pvInfoConnectCb: DESC: ca_array_get_callback"
		  " for %s: %s\n",
		  ca_name(pvInfo[i].descChid), ca_message(status));
	    }
	} else {
	    pvInfo[i].descOk = False;
	}

      /* Get the time value as a string */
	status = ca_get_callback(DBR_TIME_STRING, chId, pvInfoTimeGetCb,
	  &pvInfo[i]);
	if(status == ECA_NORMAL) {
	    nPvInfoCbs++;
	} else {
	    medmPostMsg(1,"popupPvInfo: STAMP: ca_get_callback for %s: %s\n",
	      ca_name(chId), ca_message(status));
	}

#if defined(DBR_CLASS_NAME) && DO_RTYP
      /* Get the RTYP */
	status = ca_get_callback(DBR_CLASS_NAME, chId, pvInfoRtypGetCb,
	  &pvInfo[i]);
	if(status == ECA_NORMAL) {
	    nPvInfoCbs++;
	} else {
	    medmPostMsg(1,"popupPvInfo: RTYP: ca_get_callback for %s: %s\n",
	      ca_name(chId), ca_message(status));
	}
#endif
    }

  /* Add a timeout and poll if there are callbacks
   *   The timeout is a safety net and should never be called
   *   All callbacks should come back inside the EPICS_CA_CONN_TMO
   *   Wait for 2 times this */
    if(nPvInfoCbs) {
	ca_poll();     /* May not be really necessary here */
	status = envGetDoubleConfigParam(&EPICS_CA_CONN_TMO, &connTimeout);
	if (status == 0) pvInfoTime = (unsigned long)(2000.*connTimeout+.5);
	else pvInfoTime = PVINFO_TIMEOUT;
	pvInfoTimeoutId = XtAppAddTimeOut(appContext, pvInfoTime,
	  pvInfoTimeout, NULL);
	pvInfoTimerOn = True;
    } else {
	pvInfoWriteInfo();
    }

#if DEBUG_PVINFO
    print("popupPvInfo: nPvInfoCbs=%d timeout=%ld\n",
      nPvInfoCbs, nPvInfoCbs?pvInfoTime:0L);
#endif
}

static void pvInfoDescGetCb(struct event_handler_args args)
{
    PvInfo *info = (PvInfo *)args.usr;

#if DEBUG_PVINFO
    print("pvInfoDescGetCb: nPvInfoCbs = %d\n", nPvInfoCbs);
    print("  pvInfo: %p info: %p info->descChid: %p args.dbr: %p\n",
      pvInfo,info,info->descChid,args.dbr);
#endif
    if(pvInfo && info && info->descChid && args.dbr) {
	if(args.status == ECA_NORMAL) {
	    strcpy(info->descVal, (char *)args.dbr);
	} else {
	    strcpy(info->descVal, NOT_AVAILABLE);
	}
	info->descOk = True;
    }

  /* Decrement the callback count */
    if(nPvInfoCbs > 0) nPvInfoCbs--;
    if(nPvInfoCbs == 0) pvInfoWriteInfo();
#if DEBUG_PVINFO
    print("                 nPvInfoCbs = %d\n", nPvInfoCbs);
#endif
}

#if defined(DBR_CLASS_NAME) && DO_RTYP
static void pvInfoRtypGetCb(struct event_handler_args args)
{
    PvInfo *info = (PvInfo *)args.usr;

#if DEBUG_PVINFO
    print("pvInfoRtypGetCb: nPvInfoCbs = %d\n", nPvInfoCbs);
    print("  pvInfo: %p info: %p info->descChid: %p args.dbr: %p\n",
      pvInfo,info,info->descChid,args.dbr);
#endif
  /* KE: Checking info->descChid not necessary */
    if(pvInfo && info && info->descChid && args.dbr) {
	if(args.status == ECA_NORMAL) {
	    strcpy(info->rtypVal, (char *)args.dbr);
	} else {
	    strcpy(info->rtypVal, NOT_AVAILABLE);
	}
	info->rtypOk = True;
    }

  /* Decrement the callback count */
    if(nPvInfoCbs > 0) nPvInfoCbs--;
    if(nPvInfoCbs == 0) pvInfoWriteInfo();
#if DEBUG_PVINFO
    print("                 nPvInfoCbs = %d\n", nPvInfoCbs);
#endif
}
#endif

static void pvInfoTimeGetCb(struct event_handler_args args)
{
    PvInfo *info = (PvInfo *)args.usr;

#if DEBUG_PVINFO
    print("pvInfoTimeGetCb: nPvInfoCbs = %d\n", nPvInfoCbs);
    print("  pvInfo: %p info: %p info->descChid: %p args.dbr: %p\n",
      pvInfo,info,info->descChid,args.dbr);
#endif
    if(pvInfo && info && info->descChid && args.dbr) {
	info->timeVal = *(struct dbr_time_string *)args.dbr;
	info->timeOk = True;
    }

  /* Decrement the callback count */
    if(nPvInfoCbs > 0) nPvInfoCbs--;
    if(nPvInfoCbs == 0) pvInfoWriteInfo();
#if DEBUG_PVINFO
    print("                 nPvInfoCbs = %d\n", nPvInfoCbs);
#endif
}

static void pvInfoWriteInfo(void)
{
    time_t now;
    struct tm *tblock;
    char timeStampStr[TIME_STRING_MAX];
    Record *pR;
    chid chId;
    char *descVal;
#if defined(DBR_CLASS_NAME) && DO_RTYP
    char *rtypVal;
#endif
    DlElement *pE;
    char *elementType;
    static char noType[]="Unknown";
    struct dbr_time_string timeVal;
    char string[1024];     /* Danger: Fixed length */
    XmTextPosition curpos = 0;
    int i, j;

  /* Free the timeout */
    if(pvInfoTimerOn) {
	XtRemoveTimeOut(pvInfoTimeoutId);
	pvInfoTimerOn = False;
    }

  /* Just clean up and abort if we get called with no pvInfo
   *   (Shouldn't happen) */
    if(!pvInfo) return;

  /* Get timestamp */
    time(&now);
    tblock = localtime(&now);
    strftime(timeStampStr, TIME_STRING_MAX, STRFTIME_FORMAT"\n", tblock);
    timeStampStr[TIME_STRING_MAX-1]='\0';

  /* Get element type */
    pE = pvInfoElement;
    if(pE &&
      pE->type >= MIN_DL_ELEMENT_TYPE && pE->type <= MAX_DL_ELEMENT_TYPE) {
	elementType=elementType(pE->type);
    } else {
	elementType=noType;
    }

  /* Heading */
    sprintf(string, "           PV Information\n\nObject: %s\n%s\n",
      elementType, timeStampStr);
    XmTextSetInsertionPosition(pvInfoMessageBox, 0);
    XmTextSetString(pvInfoMessageBox, string);
    curpos+=strlen(string);

  /* Loop over the records to print information */
    for(i=0; i < nPvInfoPvs; i++) {
      /* Check for a valid record */
	if(!pvInfo[i].pvOk) continue;

	chId = pvInfo[i].pvChid;
	timeVal = pvInfo[i].timeVal;
	pR = pvInfo[i].record;

      /* Name */
	sprintf(string, "%s\n"
	  "======================================\n",
	  ca_name(chId));
      /* DESC */
	descVal = pvInfo[i].descVal;
	if(pvInfo[i].descOk && descVal) {
	    sprintf(string, "%sDESC: %s\n", string, descVal);
	} else {
	    sprintf(string, "%sDESC: %s\n", string, NOT_AVAILABLE);
	}
	if(pvInfo[i].descChid) ca_clear_channel(pvInfo[i].descChid);
#if defined(DBR_CLASS_NAME) && DO_RTYP
      /* RTYP */
	rtypVal = pvInfo[i].rtypVal;
	if(pvInfo[i].rtypOk && rtypVal && *rtypVal) {
	    sprintf(string, "%sRTYP: %s\n", string, rtypVal);
	} else {
	    sprintf(string, "%sRTYP: %s\n", string, NOT_AVAILABLE);
	}
#endif
      /* Items from chid */
	sprintf(string, "%sTYPE: %s\n", string,
	  dbf_type_to_text(ca_field_type(chId)));
      /* ca_element_count is defined differently in 3.14 vs. 3.13 */
	sprintf(string, "%sCOUNT: %lu\n", string,
	  (unsigned long)ca_element_count(chId));
	sprintf(string, "%sACCESS: %s%s\n", string,
	  ca_read_access(chId)?"R":"", ca_write_access(chId)?"W":"");
	sprintf(string, "%sIOC: %s\n", string, ca_host_name(chId));
	if(timeVal.value) {
	    char fracPart[10];

	  /* Do the value */
	    if(ca_element_count(chId) == 1) {
		sprintf(string, "%sVALUE: %s\n", string,
		  timeVal.value);
	    } else {
		sprintf(string, "%sFIRST VALUE: %s\n", string,
		  timeVal.value);
	    }
	  /* Convert the seconds part of the timestamp to UNIX time */
	    now = timeVal.stamp.secPastEpoch + 631152000ul;
	    tblock = localtime(&now);
	    strftime(timeStampStr, TIME_STRING_MAX, "%a %b %d, %Y %H:%M:%S",
	      tblock);
	    timeStampStr[TIME_STRING_MAX-1]='\0';
	  /* Get the fractional part.  This assumes strftime truncates
             seconds rather than rounding them, which seems to be the
             case. */
	    sprintf(fracPart, "%09d",timeVal.stamp.nsec);
	  /* Truncate to 3 figures */
	    fracPart[3]='\0';
	    sprintf(timeStampStr,"%s.%s", timeStampStr, fracPart);
	    timeStampStr[TIME_STRING_MAX-1]='\0';
	    sprintf(string, "%sSTAMP: %s\n", string, timeStampStr);
#if DEBUG_TIMESTAMP
	  /* This prints 9 significant figures and requires 3.13 base */
	    {
		char tsTxt[32];
		sprintf(string,"%sSTAMP: %s\n",string,
		  tsStampToText(&timeVal.stamp,TS_TEXT_MONDDYYYY,tsTxt));
	    }
#endif
	} else {
	    sprintf(string, "%sVALUE: %s\n", string, NOT_AVAILABLE);
	    sprintf(string, "%sSTAMP: %s\n", string, NOT_AVAILABLE);
	}
	XmTextInsert(pvInfoMessageBox, curpos, string);
	curpos+=strlen(string);

      /* Alarms */
	switch(pR->severity) {
	case NO_ALARM:
	    sprintf(string, "ALARM: NO\n");
	    break;
	case MINOR_ALARM:
	    sprintf(string, "ALARM: MINOR\n");
	    break;
	case MAJOR_ALARM:
	    sprintf(string, "ALARM: MAJOR\n");
	    break;
	case INVALID_ALARM:
	    sprintf(string, "ALARM: INVALID\n");
	    break;
	default:
	    sprintf(string, "ALARM: Unknown\n");
	    break;
	}
	XmTextInsert(pvInfoMessageBox, curpos, string);
	curpos+=strlen(string);

      /* Items from record */
	sprintf(string, "\n");
	switch(ca_field_type(chId)) {
        case DBF_STRING:
	    break;
        case DBF_ENUM:
	    sprintf(string, "%sSTATES: %d\n",
	      string, (int)(pR->hopr+1.1));
	  /* KE: Bad way to use a double */
	    if(pR->hopr) {
		for (j=0; j <= pR->hopr; j++) {
		    sprintf(string, "%sSTATE %2d: %s\n",
		      string, j, pR->stateStrings[j]);
		}
	    }
	    break;
        case DBF_CHAR:
        case DBF_INT:
        case DBF_LONG:
	    sprintf(string, "%sHOPR: %g  LOPR: %g\n",
	      string, pR->hopr, pR->lopr);
	    break;
        case DBF_FLOAT:
        case DBF_DOUBLE:
	    if(pR->precision >= 0 && pR->precision <= 17) {
		sprintf(string, "%sPRECISION: %d\n",
		  string, pR->precision);
	    } else {
		sprintf(string, "%sPRECISION: %d [Bad Value]\n",
		  string, pR->precision);
	    }
	    sprintf(string, "%sHOPR: %g  LOPR: %g\n",
	      string, pR->hopr, pR->lopr);
	    break;
        default:
	    break;
	}
	sprintf(string, "%s\n", string);
	XmTextInsert(pvInfoMessageBox, curpos, string);
	curpos+=strlen(string);
    }

  /* Pop it up unless we have left EXECUTE mode */
    if(globalDisplayListTraversalMode == DL_EXECUTE) {
	XtSetSensitive(pvInfoS, True);
	XtPopup(pvInfoS, XtGrabNone);
    }

  /* Free space */
    if(pvInfo) free((char *)pvInfo);
    pvInfo = NULL;
    pvInfoElement = NULL;
}

static void pvInfoTimeout(XtPointer cd, XtIntervalId *id)
{
    pvInfoTimerOn = False;
    medmPostMsg(1,
      "pvInfoTimeout: PV Info request timed out after %g seconds\n"
      "  %d expected callbacks have not been received\n",
      (double)pvInfoTime/1000., nPvInfoCbs);
    pvInfoWriteInfo();
}
