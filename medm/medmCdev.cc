/*-----------------------------------------------------------------------------
 * Copyright (c) 1994,1995 Southeastern Universities Research Association,
 *                         Continuous Electron Beam Accelerator Facility
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 *-----------------------------------------------------------------------------
 *
 * Description:
 *      Medm CDEV Interface Implementation
 *
 * Author:
 *      Jie Chen
 *      Jefferson Lab. Control Software Group
 *
 * Revision History:
 *   $Log$
 *   Revision 1.3  2005/08/03 14:56:55  evans
 *   Trimmed extra whitespace at end of lines in the source files.  There
 *   may be changes related to the WheelSwitch, as well.
 *
 *   Revision 1.2  1998/09/11 22:31:36  evans
 *   Merged changes from Jie Chen (CDEV) and Anton Mezger (VMS).
 *
 *   Revision 1.1.2.1  1998/09/11 20:59:39  evans
 *   Incorporated Jie Chen's CDEV modifications into MEDM 2.3.5Beta1.
 *
 *   Revision 1.1  1998/08/25 18:39:06  evans
 *   Incorporated changes from Jie Chen for CDEV.  Not tested with
 *   MEDM_CDEV defined.
 *
 *   Revision 1.4  1998/04/29 15:15:14  chen
 *   fix DBF_ENUM when services return strings
 *
 *   Revision 1.3  1998/04/24 14:07:50  chen
 *   fix problem related to string record
 *
 *   Revision 1.2  1998/04/22 15:19:20  chen
 *   fix buf for mis free (name) instead of free (attr)
 *
 *   Revision 1.1  1998/04/16 15:22:29  chen
 *   First working version of medm_cdev
 *
 *
 */
#ifdef MEDM_CDEV

#include "medm.h"
#include "xtParams.h"

#include "medmCdev.h"
#include "medmCdevP.h"
#include "medmCdevUtils.h"

#define BYTE medm_cdev_byte

#include <cdevSystem.h>
#include <cdevData.h>
#include <cdevDevice.h>
#include <cdevRequestObject.h>
#include <sys/resource.h>

#undef BYTE

#define _MEDM_MONITORON_STR   "monitorOn"
#define _MEDM_MONITOROFF_STR  "monitorOff"
#define _MEDM_GET_STR         "get"
#define _MEDM_SET_STR         "set"
#define _MEDM_DEFAULT_ATTR    "VAL"
#define _MEDM_DEVICE_NAME_LEN 128

static char* _medm_graphical_cxt[] = {
  "value", "status", "severity", "units", "precision", "displayHigh",
  "displayLow", "alarmHigh", "alarmLow", "warningHigh",
  "warningLow", "controlHigh", "controlLow"
};

static char* _medm_monitor_cxt[] = {
  "value", "status", "severity", "time"
};

static int _medm_graphical_cxt_num = 13;
static int _medm_monitor_cxt_num = 4;

medmInputFd::medmInputFd (int f, int i, medmInputFd* n)
:fd (f), id (i), next (n)
{
  //empty
}

medmInputFd::~medmInputFd (void)
{
  if (next)
    delete next;
}

/* global instance of medmXInput */
medmXInput* medmGXinput = 0;

medmXInput::medmXInput (XtAppContext context, cdevSystem* system)
:xfds_ (0), context_ (context), system_ (system)
{
#ifdef _TRACE_OBJECTS
  printf ("Create medmXInput Class Object\n");
#endif
}

medmXInput::~medmXInput (void)
{
#ifdef _TRACE_OBJECTS
  printf ("Delete medmXInput Class Object\n");
#endif

  medmInputFd* pfd = 0;
  if (xfds_) {
    for (pfd = xfds_; pfd; pfd = pfd->next)
      XtRemoveInput (pfd->id);

    delete xfds_;
  }
  xfds_ = 0;
}

void
medmXInput::addInput (int fd, XtPointer mask)
{
  medmInputFd* cfd = 0;

  /* find out whether this fd is already registered */
  if (xfds_) {
    for (cfd = xfds_; cfd; cfd = cfd->next)
      if (cfd->fd == fd) {
	fprintf (stderr, "medmCDEV: Fatal: fd %d is already registered\n", fd);
	return;
      }
  }

  int id = XtAppAddInput (context_, fd, mask,
			  &(medmXInput::inputCallback),
			  (XtPointer)this);
  if (!xfds_) {
    xfds_ = new medmInputFd (fd, id);
  }
  else {
    /* put new fd in the beginning of the list */
    cfd = new medmInputFd (fd, id, xfds_);
    xfds_ = cfd;
  }
}

void
medmXInput::removeInput (int fd)
{
  medmInputFd* cfd = 0;
  medmInputFd* pfd = 0;
  int          found = 0;

  for (cfd = xfds_; cfd; cfd = cfd->next) {
    if (cfd->fd == fd) {
      found = 1;
      if (!pfd)                /* deleting head of the list */
	xfds_ = cfd->next;
      else
	pfd->next = cfd->next;

                              /* remove XInput from X */
      XtRemoveInput (cfd->id);

                              /* free memory          */
      cfd->next = 0;          /* disable recursive delete  */
      delete cfd;
      break;
    }
    pfd = cfd;
  }

  if (!found)
    fprintf (stderr, "medmCDEV: Fatal: cannot remove fd %d from list\n", fd);
}

void
medmXInput::inputCallback (XtPointer data, int* fd, XtInputId* id)
{
  medmXInput* obj = (medmXInput *)data;

  obj->system_->poll ();
}

void
medmFdChangedCallback (int fd, int opened, void* arg)
{
  if (!medmGXinput)
    return;

  medmXInput* xinput = (medmXInput *)arg;

  if (opened)
    xinput->addInput    (fd, (XtPointer)XtInputReadMask);
  else
    xinput->removeInput (fd);
}

int
medmCDEVInitialize (void)
{
  /* first change file descriptor limit for medm */
  struct rlimit limit, nlimit;

  if (getrlimit (RLIMIT_NOFILE, &limit) != 0)
    printf ("medm_cdev: Cannot get resouce limit\n");
  else {
    if (limit.rlim_cur < 256) {
      nlimit.rlim_cur = 256;
      nlimit.rlim_max = limit.rlim_max;
      if (setrlimit (RLIMIT_NOFILE, &nlimit) != 0)
	printf ("medm_cdev:Cannot change the limit on number of opened files\n");
      else
	if (getrlimit (RLIMIT_NOFILE, &limit) != 0)
	  printf ("medm_cdev: Cannot change the limit on number of opened files\n");
    }
  }

  cdevSystem& system = cdevSystem::defaultSystem ();

  /* set error reporting threshold to severe */
  system.setThreshold (CDEV_SEVERITY_ERROR);

  medmGXinput = new medmXInput (appContext, &system);

  system.addFdChangedCallback (medmFdChangedCallback, (void *)medmGXinput);

  return 0;
}

void
medmCDEVTerminate (void)
{
  if (medmGXinput)
    delete medmGXinput;
  medmGXinput = 0;
}

static void
medmUpdateChannelCb (int status, void* arg,
		     cdevRequestObject& req, cdevData& data)
{
  Record* pr = (Record *)arg;
  double value = 0.0;
  Boolean severityChanged = False;
  Boolean zeroAndNoneZeroTransition = False;
  int     severity = 0;
  int     i;

  if (globalDisplayListTraversalMode != DL_EXECUTE) return;

  if (status == CDEV_DISCONNECTED) {
    pr->connected = False;
    if (pr->updateValueCb)
      pr->updateValueCb ((XtPointer)pr);
  }
  else if (status == CDEV_RECONNECTED) {
    pr->connected = True;
    if (pr->updateValueCb)
      pr->updateValueCb ((XtPointer)pr);
  }
  else if (status == CDEV_ACCESSCHANGED) {
    // really find access flags
    int acflag = req.getAccess ();

    if (acflag == CDEV_ACCESS_NONE) {
      pr->readAccess = False;
      pr->writeAccess = False;
    }
    else if (acflag == CDEV_ACCESS_READONLY) {
      pr->readAccess = True;
      pr->writeAccess = False;
    }
    else if (acflag == CDEV_ACCESS_WRITE) {
      pr->readAccess = True;
      pr->writeAccess = True;
    }
    else {
      pr->readAccess = True;
      pr->writeAccess = True;
    }
    if (pr->updateValueCb)
      pr->updateValueCb ((XtPointer)pr);
  }
  else if (status == CDEV_SUCCESS) {
    data.get ("time", &pr->time);
    void* dataptr = 0;

    if (data.find ("value", dataptr) != CDEV_SUCCESS)
      return;

    if (pr->elementCount > 1) {
      if (pr->array)
	free (pr->array);
      pr->array = 0;

      switch (pr->dataType) {
      case DBF_CHAR:
	{
	  unsigned char* tvals = (unsigned char *)malloc (pr->elementCount*sizeof(unsigned char));

	  if (data.get("value", tvals) != CDEV_SUCCESS)
	    return;

	  value = (double)(tvals[0]);
	  pr->array = tvals;
	}
	break;
      case DBF_INT:
	{
	  short *tvals = (short *)malloc (pr->elementCount*sizeof (short));
	  if (data.get("value", tvals) != CDEV_SUCCESS)
	    return;

	  value = (double)(tvals[0]);
	  pr->array = tvals;
	}
	break;
      case DBF_LONG:
	{
	  long *tvals = (long *)malloc (pr->elementCount*sizeof (long));
	  if (data.get("value", tvals) != CDEV_SUCCESS)
	    return;

	  value = (double)(tvals[0]);
	  pr->array = tvals;
	}
	break;
      case DBF_FLOAT:
	{
	  float *tvals = (float *)malloc (pr->elementCount*sizeof (float));
	  if (data.get("value", tvals) != CDEV_SUCCESS)
	    return;

	  value = (double)(tvals[0]);
	  pr->array = tvals;
	}
	break;
      case DBF_DOUBLE:
	{
	  double *tvals = (double *)malloc (pr->elementCount*sizeof (double));
	  if (data.get ("value", tvals) != CDEV_SUCCESS)
	    return;
	  value = tvals[0];
	  pr->array = tvals;
	}
	break;
      default:
	break;
      }
    }
    else {
      if (pr->array)
	free (pr->array);
      pr->array = 0;

      if (pr->dataType == DBF_STRING) {
	char tmpstr[1024];
	if (data.get ("value", tmpstr, sizeof (tmpstr)) != CDEV_SUCCESS)
	  return;
	char* tvals = new char[strlen (tmpstr) + 1];
	strcpy (tvals, tmpstr);
	pr->array = tvals;
	value = 0.0;
      }
      else if (pr->dataType == DBF_ENUM) {
	/* enum data type record may return either index or real string */
	/* or whole new list of strings for new choices                 */
	int datatype;
	if ((datatype = data.getType ("value")) == CDEV_INVALID)
	  return;

	size_t dim;
	if (data.getDim ("value", &dim) != CDEV_SUCCESS)
	  return;
	else if ( dim > 1) {
	  fprintf (stderr, "memd_cdev: Error: medm does not support multi-dimensional data\n");
	  return;
	}


	size_t elems;
	if (data.getElems ("value", &elems) != CDEV_SUCCESS)
	  return;
	else if (elems <= 0) {
	  fprintf (stderr, "medm_cdev: Error: graphical display info contains no data\n");
	  return;
	}
	if (datatype == CDEV_STRING) {
	  if (elems == 1) { /* service returns a single string */
	    char tmpstr[1024];
	    int hopr = 0;
	    int lopr = 0;

	    if (data.get ("displayHigh", &hopr) == CDEV_SUCCESS &&
		data.get ("displayLow", &lopr) == CDEV_SUCCESS) {
	      if (pr->stateStrings) {
		for (i = 0; i < pr->hopr + 1; i++)
		  free (pr->stateStrings[i]);
		free (pr->stateStrings);
		pr->stateStrings = 0;
	      }
	      /* new list of choices, only one */
	      pr->hopr = (double)hopr;
	      pr->lopr = (double)lopr;
	      value = 0.0;

	      if (data.get ("value", tmpstr, sizeof (tmpstr)) != CDEV_SUCCESS)
		return;

	      pr->stateStrings = (char **)malloc (elems*sizeof (char *));
	      pr->stateStrings[0] = (char *)malloc ((strlen (tmpstr) + 1)*sizeof(char));
	      strcpy (pr->stateStrings[0], tmpstr);
	      pr->value = value;

	      /* calling graphical info callback */
	      if (pr->updateGraphicalInfoCb)
		pr->updateGraphicalInfoCb ((XtPointer)pr);
	    }
	    else {  /* return a single choice */
	      if (data.get ("value", tmpstr, sizeof (tmpstr)) != CDEV_SUCCESS)
		return;

	      if (!pr->stateStrings)
		return;

	      for (i = 0; i < pr->hopr + 1; i++) {
		if (strcmp (tmpstr, pr->stateStrings[i]) == 0)
		  break;
	      }
	      if (i > pr->hopr)
		return;

	      value = (double) i;
	    }
	  }
	  else {  /* return list of strings: must be a new set of choices */
	    if (pr->stateStrings) {
	      for (i = 0; i < pr->hopr + 1; i++)
		free (pr->stateStrings[i]);
	      free (pr->stateStrings);
	      pr->stateStrings = 0;
	    }

	    pr->stateStrings = (char **)malloc(elems*sizeof (char *));
	    if (data.get ("value", pr->stateStrings) != CDEV_SUCCESS) {
	      free (pr->stateStrings);
	      pr->stateStrings = 0;
	      return;
	    }
	    /* new list of choices, only one */
	    pr->hopr = elems - 1;
	    pr->lopr = 0;
	    value = 0.0;
	    pr->value = value;

	    /* calling graphical info callback */
	    if (pr->updateGraphicalInfoCb)
	      pr->updateGraphicalInfoCb ((XtPointer)pr);
	  }
	}
	else
	  if (data.get ("value", &value) != CDEV_SUCCESS)
	    return;
      }
      else
	if (data.get ("value", &value) != CDEV_SUCCESS)
	  return;
    }

    pr->connected = True;

    if (pr->autoscale) {
      if (value > 0.0 && value*2 > pr->hopr) {
	pr->hopr = 2*value;
	if (pr->updateGraphicalInfoCb)
	  pr->updateGraphicalInfoCb ((XtPointer)pr);
      }
      else if (value < 0.0 && value*2 < pr->lopr) {
	pr->lopr = 2*value;
	if (pr->updateGraphicalInfoCb)
	  pr->updateGraphicalInfoCb ((XtPointer)pr);
      }
    }

    if (((value == 0.0) && (pr->value != 0.0))
	|| ((value != 0.0) && (pr->value == 0.0)))
      zeroAndNoneZeroTransition = True;

    pr->value = value;

    data.get ("status", &status);
    data.get ("severity", &severity);
    if (pr->severity != severity) {
      pr->severity = severity;
      severityChanged = True;
    }

    if (pr->monitorValueChanged && pr->updateValueCb) {
      pr->updateValueCb((XtPointer)pr);
    }
    else {
      if (pr->monitorSeverityChanged && severityChanged && pr->updateValueCb) {
	pr->updateValueCb((XtPointer)pr);
      }
      else
	if (pr->monitorZeroAndNoneZeroTransition
	    && zeroAndNoneZeroTransition && pr->updateValueCb) {
	  pr->updateValueCb((XtPointer)pr);
	}
    }
  }

}

static void
medmUpdateGraphicalInfoCb (int status, void* arg,
			   cdevRequestObject& req, cdevData& data)
{
  Record* pr = (Record *)arg;
  int i, acflag;

  if (globalDisplayListTraversalMode != DL_EXECUTE) return;

  if (status != CDEV_SUCCESS)
    return;

  pr->connected = True;
  // really find access flags
  acflag = req.getAccess ();

  if (acflag == CDEV_SUCCESS) {
    pr->readAccess = True;
    pr->writeAccess = True;
  }
  else if (acflag == CDEV_ACCESS_READONLY) {
    pr->readAccess = True;
    pr->writeAccess = False;
  }
  else if (acflag == CDEV_ACCESS_WRITE) {
    pr->readAccess = True;
    pr->writeAccess = True;
  }
  else {
    pr->readAccess = True;
    pr->writeAccess = True;
  }

  int datatype;
  if ((datatype = data.getType ("value")) == CDEV_INVALID)
    return;

  size_t dim;
  if (data.getDim ("value", &dim) != CDEV_SUCCESS)
    return;
  else if ( dim > 1) {
    fprintf (stderr, "memd_cdev: Error: medm does not support multi-dimensional data\n");
    return;
  }


  size_t elems;
  if (data.getElems ("value", &elems) != CDEV_SUCCESS)
    return;
  else if (elems <= 0) {
    fprintf (stderr, "medm_cdev: Error: graphical display info contains no data\n");
    return;
  }

  /* set data type etc */
  pr->elementCount = elems;
  pr->dataType = datatype;

  if (datatype == CDEV_STRING) {
    if (elems == 1) {
      pr->value = 0.0;
      pr->hopr = 0.0;
      pr->lopr = 0.0;
      pr->precision = 0;
      /* this data can be either ENUM or real strings */
      int hopr = 0.0;
      int lopr = 0.0;
      if (data.get ("displayHigh", &hopr) == CDEV_SUCCESS &&
	  data.get ("displayLow", &lopr)  == CDEV_SUCCESS) {
	pr->hopr = (double)hopr;
	pr->lopr = (double)lopr;
	pr->dataType = DBF_ENUM;
	pr->elementCount = 1;

	char tmpstr[1024];
	if (data.get ("value", tmpstr, sizeof (tmpstr)) != CDEV_SUCCESS)
	  return;

	pr->stateStrings = (char **)malloc (elems*sizeof (char *));
	pr->stateStrings[0] = (char *)malloc ((strlen (tmpstr) + 1)*sizeof(char));
	strcpy (pr->stateStrings[0], tmpstr);
      }
    }
    else {
      pr->value = (double)0.0;
      pr->precision = 0;
      pr->stateStrings = (char **)malloc (elems*sizeof (char *));
      if (data.get ("value", pr->stateStrings) != CDEV_SUCCESS) {
	free (pr->stateStrings);
	pr->stateStrings = 0;
	return;
      }
      /* this data can be either ENUM or real strings */
      int hopr = 0.0;
      int lopr = 0.0;
      data.get ("displayHigh", &hopr);
      data.get ("displayLow", &lopr);
      if (hopr > 0) { /* really a ENUM */
	pr->hopr = (double)hopr;
	pr->lopr = (double)lopr;
	pr->dataType = DBF_ENUM;
	pr->elementCount = 1;
      }
    }
  }
  else if (datatype == CDEV_TIMESTAMP) {
    pr->value = 0.0;
    pr->hopr = 0.0;
    pr->lopr = 0.0;
    pr->precision = 0;
  }
  else {
    double value = 0.0;
    double hopr = 0.0;
    double lopr = 0.0;
    short  precision = 5;

    if (pr->elementCount > 1) {
      double *tvals = new double[pr->elementCount];
      data.get ("value", tvals);
      value = tvals[0];
      delete []tvals;
    }
    else
      data.get ("value", &value);

    data.get ("displayHigh", &hopr);
    data.get ("displayLow", &lopr);
    data.get ("precision", &precision);

    pr->value = value;
    pr->hopr = hopr;
    pr->lopr = lopr;
    pr->precision = precision;

    if (pr->hopr == 0.0 && pr->lopr == 0.0) {
      if (pr->dataType != DBF_STRING && pr->dataType != DBF_ENUM)
	pr->autoscale = 1;
    }
  }

  if (pr->precision < 0) {
    medmPostMsg(1,"medm_cdev: medmUpdateGraphicalInfoCb: pv %s precision = %d\n",
		pr->name, pr->precision);
    pr->precision = 0;
  }
  else {
    if (pr->precision > 16) {
      medmPostMsg(1,"medm_cdev: medmUpdateGraphicalInfoCb: pv %s precision = %d\n",
		  pr->name, pr->precision);
      pr->precision = 16;
    }
  }

  if (pr->updateGraphicalInfoCb)
    pr->updateGraphicalInfoCb ((XtPointer)pr);
  else {
    if (pr->updateValueCb) {
      pr->updateValueCb ((XtPointer)pr);
    }
  }

  if (pr->dev) {
    cdevCallback valcbk (medmUpdateChannelCb, pr);
    cdevDevice* device = (cdevDevice *)pr->dev;

    /* this is the request object that monitors on the attribute */
    char msg[2*_MEDM_DEVICE_NAME_LEN];
    if (pr->verb) {
      if (pr->attr)
	sprintf (msg, "%s %s",pr->verb, pr->attr);
      else
	sprintf (msg, "%s", pr->verb);
    }
    else {
      if (pr->attr)
	sprintf (msg, "%s %s", _MEDM_MONITORON_STR, pr->attr);
      else
	sprintf (msg, "%s", _MEDM_MONITORON_STR);
    }

    cdevRequestObject* req = device->getRequestObject (msg);
    if (req) {
      if (req->sendCallback (0, valcbk) != CDEV_SUCCESS)
	fprintf (stderr, "medm_cdev: %s %s sendCallback failed\n",
		 pr->name, msg);
    }
  }
}


static cdevDevice *
cdevAdd (char* devattr, Record* pr)
{
  int status;
  int i;
  char* dot = 0;
  char* dotn = 0;

  /* copy device attribute into a local buffer                   */
  /* reason: gcc sometimes does not like sscanf in a temp buffer */
  char devattrcom[2*_MEDM_DEVICE_NAME_LEN];
  strncpy (devattrcom, devattr, sizeof (devattrcom) - 1);
  devattrcom[2*_MEDM_DEVICE_NAME_LEN - 1] = '\0';

  char device[_MEDM_DEVICE_NAME_LEN];
  char attr[_MEDM_DEVICE_NAME_LEN];
  char verb[_MEDM_DEVICE_NAME_LEN];
  char msg[2*_MEDM_DEVICE_NAME_LEN];
  char getmsg[2*_MEDM_DEVICE_NAME_LEN];

  /* initialize all char arrays */
  device[0] = 0;
  attr[0] = 0;
  verb[0] = 0;

  if ((status = sscanf (devattrcom, "%s %s %s", device, verb, attr)) == 1) {
    if ((dot = strrchr (device, '.')) != 0) {  /* device.attr */
      dotn = dot + 1;
      if (!dotn || !*dotn)
	strcpy (attr, _MEDM_DEFAULT_ATTR);
      else {
	strcpy (attr, dotn);
	*dot = '\0';
      }
    }
    else
      strcpy (attr, _MEDM_DEFAULT_ATTR);
  }
  else if (status < 1)
    return 0;

  cdevDevice* dev = cdevDevice::attachPtr (device);

  /* this is the request object that monitors on the attribute */
  if (!verb[0]) {
    if (!attr[0])
      sprintf (msg, "%s", _MEDM_MONITORON_STR);
    else
      sprintf (msg, "%s %s", _MEDM_MONITORON_STR, attr);
  }
  else {
    if (!attr[0])
      sprintf (msg, "%s", verb);
    else
      sprintf (msg, "%s %s", verb, attr);
  }
  cdevRequestObject* req = dev->getRequestObject (msg);

  /* this is the request object that gets all graphical information */
  if (!attr[0])
    sprintf (getmsg, "%s", _MEDM_GET_STR);
  else
    sprintf (getmsg, "%s %s", _MEDM_GET_STR, attr);
  cdevRequestObject* greq = dev->getRequestObject (getmsg);


  if (greq && req) {
    cdevData cxt;

    for (i = 0; i < _medm_graphical_cxt_num; i++)
      cxt.insert (_medm_graphical_cxt[i], 1);

    greq->setContext (cxt);

    cdevCallback getcallback (medmUpdateGraphicalInfoCb, pr);
    if (greq->sendCallback (0, getcallback) != CDEV_SUCCESS) {
      fprintf (stderr, "medm_cdev: %s %s sendCallback failed\n",
	       device, getmsg);
    }
  }

  if (req) {
    cdevData mcxt;
    for (i = 0; i < _medm_monitor_cxt_num; i++)
      mcxt.insert (_medm_monitor_cxt[i], 1);
    req->setContext (mcxt);
  }


  /* copy record name */
  pr->name = (char *)malloc ((strlen (device) + 1)*sizeof (char));
  strcpy (pr->name, device);

  /* copy verb on this record */
  if (!verb[0])
    pr->verb = 0;
  else {
    pr->verb = (char *)malloc((strlen (verb) + 1)*sizeof (char));
    strcpy (pr->verb, verb);
  }

  /* copy attribute of this record */
  if (!attr[0]) {
    pr->attr = 0;
    pr->fullname = (char *)malloc ((strlen (pr->name) + 1)*sizeof (char));
    strcpy (pr->fullname, pr->name);
  }
  else {
    pr->attr = (char *)malloc ((strlen (attr) + 1)*sizeof (char));
    strcpy (pr->attr, attr);

    int tlen = strlen (pr->name) + strlen (pr->attr) + 2;
    pr->fullname = (char *)malloc (tlen*sizeof (char));
    strcpy (pr->fullname, pr->name);
    strcat (pr->fullname, " ");
    strcat (pr->fullname, pr->attr);
  }
  return dev;
}

static void
cdevDelete (Record* pr)
{
  if (pr->dev) {
    cdevCallback valcbk (medmUpdateChannelCb, pr);
    cdevDevice* device = (cdevDevice *)pr->dev;

    /* this is the request object that monitors off the attribute */
    char msg[2*_MEDM_DEVICE_NAME_LEN];
    if (pr->attr)
      sprintf (msg, "%s %s",_MEDM_MONITOROFF_STR, pr->attr);
    else
      sprintf (msg, "%s %s",_MEDM_MONITOROFF_STR, _MEDM_DEFAULT_ATTR);

    cdevRequestObject* offreq = device->getRequestObject (msg);
    if (offreq) {
      if (offreq->sendCallback (0, valcbk) != CDEV_SUCCESS)
	fprintf (stderr, "medm_cdev: %s %s sendCallback failed\n",
		 pr->name, msg);
    }
    cdevDevice::detach (device);
  }
  pr->dev = 0;
}

static void medmInitRecord (Record *p)
{
  p->dev = 0;
  p->elementCount = -1;
  p->dataType = -1;
  p->value = 0.0;
  p->hopr = 0.0;
  p->lopr = 0.0;
  p->precision = -1;
  p->status = NO_ALARM;
  p->severity = NO_ALARM;
  p->connected = False;
  p->readAccess = True;
  p->writeAccess = True;
  p->useMsgWhenWrite[0] = False;
  p->useMsgWhenWrite[1] = False;
  p->stateStrings = 0;
  p->name = 0;
  p->attr = 0;
  p->verb = 0;
  p->fullname = 0;
  p->array = 0;
  p->time.secPastEpoch = 0;
  p->time.nsec = 0;
  p->autoscale = 0;
  p->clientData = 0;
  p->updateValueCb = 0;
  p->updateGraphicalInfoCb = 0;
  p->monitorSeverityChanged = True;
  p->monitorValueChanged = True;
  p->monitorZeroAndNoneZeroTransition = True;
}



Record*
medmAllocateRecord(char *name,
		   void (*updateValueCb)(XtPointer),
		   void (*updateGraphicalInfoCb)(XtPointer),
		   XtPointer clientData)
{
  Record *record;
  record = (Record *) malloc(sizeof(Record));
  if (record) {
    medmInitRecord (record);
    record->updateValueCb = updateValueCb;
    record->updateGraphicalInfoCb = updateGraphicalInfoCb;
    record->clientData = clientData;
    record->dev = (long)cdevAdd(name, record);
  }
  return record;
}

void
medmDestroyRecord(Record *pr)
{
  int i;
  cdevDelete(pr);

  if (pr->name)
    free (pr->name);
  if (pr->attr)
    free (pr->attr);
  if (pr->verb)
    free (pr->verb);
  if (pr->fullname)
    free (pr->fullname);
  if (pr->array)
    free (pr->array);

  if (pr->dataType == DBF_ENUM) {
    if (pr->stateStrings) {
      for (i = 0; i < pr->hopr + 1; i++)
	free (pr->stateStrings[i]);
      free (pr->stateStrings);
    }
  }

  medmInitRecord (pr);

  free((char *)pr);
}


cdevDevice *
getDeviceFromRecord (Record *pr)
{
  if (!pr) return 0;

  return (cdevDevice *)pr->dev;
}

void
medmSendDouble (Record *pr, double data)
{
  cdevDevice* device = (cdevDevice *)pr->dev;

  if (device) {
    char* attr = 0;
    cdevData out;
    out.insert ("value", data);

    char msg[128];


    if (!pr->verb) {
      if (!pr->attr)
	sprintf (msg, "%s", _MEDM_SET_STR);
      else
	sprintf (msg, "%s %s", _MEDM_SET_STR, pr->attr);
    }
    else {
      if (!pr->attr)
	sprintf (msg, "%s", pr->verb);
      else
	sprintf (msg, "%s %s", pr->verb, pr->attr);
    }

    device->send (msg, out, 0);
  }
}

void
medmSendCharacterArray (Record* pr, char* data, unsigned long size)
{
   cdevDevice* device = (cdevDevice *)pr->dev;

   if (device) {
    char* attr = 0;
    cdevData out;
    out.insert ("value", (unsigned char *)data, (size_t)size);

    char msg[128];


    if (!pr->verb) {
      if (!pr->attr)
	sprintf (msg, "%s", _MEDM_SET_STR);
      else
	sprintf (msg, "%s %s", _MEDM_SET_STR, pr->attr);
    }
    else {
      if (!pr->attr)
	sprintf (msg, "%s", pr->verb);
      else
	sprintf (msg, "%s %s", pr->verb, pr->attr);
    }

    device->send (msg, out, 0);
   }
}

void
medmSendString (Record* pr, char* data)
{
  cdevDevice* device = (cdevDevice *)pr->dev;

  if (device) {
    char* attr = 0;
    cdevData out;
    out.insert ("value", data);

    char msg[128];


    if (!pr->verb) {
      if (!pr->attr)
	sprintf (msg, "%s", _MEDM_SET_STR);
      else
	sprintf (msg, "%s %s", _MEDM_SET_STR, pr->attr);
    }
    else {
      if (!pr->attr)
	sprintf (msg, "%s", pr->verb);
      else
	sprintf (msg, "%s %s", pr->verb, pr->attr);
    }
    device->send (msg, out, 0);
  }
}

void
medmSendMsg (Record* pr, char* msg)
{
  cdevDevice* device = (cdevDevice *)pr->dev;

  if (device)
    device->send (msg, 0, 0);
}


void medmRecordAddUpdateValueCb(Record *pr,
				void (*updateValueCb)(XtPointer))
{
  pr->updateValueCb = updateValueCb;
}

void medmRecordAddGraphicalInfoCb(Record *pr,
				  void (*updateGraphicalInfoCb)(XtPointer))
{
  pr->updateGraphicalInfoCb = updateGraphicalInfoCb;
}

char *
medmPvName (Record* pr)
{
  static char pv[256];

  if (pr->attr)
    sprintf (pv, "%s %s", pr->name, pr->attr);
  else
    strcpy (pv, pr->name);

  return pv;
}

static medmPvTypeName medm_type_names[] =
{
  {"String", DBF_STRING},
  {"Short",  DBF_INT},
  {"Integer", DBF_LONG},
  {"Float", DBF_FLOAT},
  {"Double", DBF_DOUBLE},
  {"Byte", DBF_CHAR},
  {"ENUM", DBF_ENUM},
  {"Unknown", 1000}
};

char *
medmPvType (Record* pr)
{
  int i = 0;

  for (i = 0; i < 7; i++) {
    if (pr->dataType == medm_type_names[i].type)
      return medm_type_names[i].name;
  }
  return medm_type_names[i].name;
}

int
medmPvCount (Record* pr)
{
  return pr->elementCount;
}

int
medmPvDataType (Record* pr)
{
  return pr->dataType;
}

int
medmPvGetValue (Record* pr, struct dbr_time_string* value)
{
  cdevDevice* device = (cdevDevice *)pr->dev;
  value->value = 0;
  value->status = 0;
  value->severity = 0;
  value->stamp.secPastEpoch = 0;
  value->stamp.nsec = 0;


  if (!device) {
    return -1;
  }

  cdevData result, cxt;
  char msg[2*_MEDM_DEVICE_NAME_LEN];

  if (pr->attr)
    sprintf (msg, "%s %s", _MEDM_GET_STR, pr->attr);
  else
    sprintf (msg, "%s %s", _MEDM_GET_STR, _MEDM_DEFAULT_ATTR);
  cdevRequestObject* req = device->getRequestObject (msg);

  cxt.insert ("value", 1);
  cxt.insert ("status", 1);
  cxt.insert ("severity", 1);
  cxt.insert ("time", 1);

  if (!req) {
    return -1;
  }

  req->setContext (cxt);

  if (req->send (0, result) != CDEV_SUCCESS)
    return -1;

  value->value = (char *)malloc (MAX_STRING_SIZE);
  result.get ("value", value->value, MAX_STRING_SIZE);
  result.get ("status", &value->status);
  result.get ("severity", &value->severity);
  result.get ("time", &value->stamp);

  return 0;
}

char *
medm_tsStampToText (cdev_TS_STAMP* ts, int type, char* buffer)
{
  struct tm* ltime;
  char* retbuf = 0;

  ltime = localtime ((time_t *)&ts->secPastEpoch);
  char *atime = asctime (ltime);

  if (buffer) {
    strcpy (buffer, atime);
    retbuf = buffer;
  }
  else
    retbuf = atime;

  return retbuf;
}

#ifndef TIME_STRING_MAX
#define TIME_STRING_MAX 81
#endif

void
popupPvInfo(DisplayInfo *displayInfo)
{
  long now;
  struct tm *tblock;
  char timeStampStr[TIME_STRING_MAX];
  char tsTxt[32];     /* 28 for TS_TEXT_MMDDYY, 32 for TS_TEXT_MMDDYYYY */
  Arg args[1];
  int nargs;
  Record **records;
  struct dbr_time_string *timeVals, timeVal;
  int i, j, count, status;
  char string[1024];     /* Danger: Fixed length */
  Record *pR;
  XmTextPosition curpos = 0;

  /* Create the dialog box if it has not been created */
  if(!pvInfoS) createPvInfoDlg();

  /* Get the records */
  records = getPvInfoFromDisplay(displayInfo, &count);
  if(!records) return;   /* (Error messages have been sent) */

 /* Allocate space */
  timeVals = (struct dbr_time_string *)calloc(count,
					      sizeof(struct dbr_time_string));
  if(!timeVals) {
    medmPostMsg(1,"popupPvInfo: Memory allocation error\n");
    return;
  }

  /* Get timestamp */
  time(&now);
  tblock = localtime(&now);
  strftime(timeStampStr,TIME_STRING_MAX,"%a %h %e %k:%M:%S %Z %Y\n",tblock);
  timeStampStr[TIME_STRING_MAX-1]='0';

  /* Heading */
  sprintf(string,"           PV Information\n\n%s\n",timeStampStr);
  XmTextSetInsertionPosition(pvInfoMessageBox, 0);
  XmTextSetString(pvInfoMessageBox,string);
  curpos+=strlen(string);

  /* Loop over the records to get information */
  for(i=0; i < count; i++) {
    /* Check for a valid record */
    if(records[i]) {
      pR = records[i];
      if (pR->dev == 0) continue;
    } else continue;

    /* Get the time value as a string */
    if (medmPvGetValue (pR, &timeVals[i]) != 0)
      medmPostMsg(1,"popupPvInfo: cdev_get error on %s\n", medmPvName (pR));
  }

  /* Loop over the records to print information */
  for(i=0; i < count; i++) {
    timeVal = timeVals[i];
    pR = records[i];

    /* Items from chid */
    sprintf(string,"%s\n"
	    "======================================\n",
	    medmPvName (pR));
    sprintf(string,"%sTYPE: %s\n",string, medmPvType (pR));
    sprintf(string,"%sCOUNT: %d\n",string, medmPvCount (pR));
    if(timeVal.value) {
      if(medmPvCount (pR) == 1) {
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
    switch(medmPvDataType (pR)) {
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
      sprintf(string,"%sPRECISION: %d\n",
	      string, pR->precision);
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
  free(records);

  XtSetSensitive(pvInfoS,True);
  XtPopup(pvInfoS,XtGrabNone);
}

#endif     /* #ifdef MEDM_CDEV */
