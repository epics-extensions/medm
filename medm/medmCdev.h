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
 *      MEDM CDEV Interface Header file
 *      
 *      Device Name And Attribute Name Cannot Exceed Length 127
 *
 * Author:  
 *      Jie Chen
 *      Jefferson Lab. Control Software Group
 *
 * Revision History:
 *   $Log$
 *   Revision 1.1  1998/08/25 18:39:06  evans
 *   Incorporated changes from Jie Chen for CDEV.  Not tested with
 *   MEDM_CDEV defined.
 *
 *   Revision 1.1  1998/04/16 15:22:30  chen
 *   First working version of medm_cdev
 *
 *
 */

#ifndef _MEDM_CDEV_H
#define _MEDM_CDEV_H

#ifdef MEDM_CDEV

#define BYTE medm_cdev_byte

#include <stdarg.h>
#include <cdev.h>
#include <cdevTypes.h>

#define DBF_STRING CDEV_STR
#define DBF_INT    CDEV_INT_16
#define DBF_SHORT  CDEV_INT_16
#define DBF_LONG   CDEV_INT_32
#define DBF_FLOAT  CDEV_FLT
#define DBF_CHAR   CDEV_BYTE_
#define DBF_DOUBLE CDEV_DBL
/* Special data type */
#define DBF_ENUM   100

#undef BYTE

/* KE: The following lines were formerly in medm_cdev_impl.h */

#include <stdio.h>
#include <string.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <cdevTypes.h>

class cdevSystem;
class cdevRequestObject;

class medmInputFd
{
public:
  // constructor
  medmInputFd        (int f, int i, medmInputFd* next = 0);
  // destructor
  ~medmInputFd       (void);

  int                fd;
  XtInputId          id;
  medmInputFd*       next;
};

class medmXInput
{
public:
  // constrcutor
  medmXInput  (XtAppContext context, cdevSystem* system);
  // destructor
  ~medmXInput (void);

  // operations
  // add a single file descriptor 
  void addInput    (int fd, XtPointer mask);
  void removeInput (int fd);

protected:
  // internal file descriptors
  medmInputFd* xfds_;
  
private:
  static void inputCallback (XtPointer, int*, XtInputId*);
  XtAppContext context_;

  cdevSystem* system_;

  // deny copy and assignment operations
  medmXInput (const medmXInput& input);
  medmXInput& operator = (const medmXInput& input);
};

extern medmXInput* medmGXinput;

/* KE: End of lines formerly in medm_cdev_impl.h */

#if defined (__cplusplus)
extern "C" {
#endif

extern int  medmCDEVInitialize (void);
extern void medmCDEVTerminate  (void);

typedef struct _Record {
  long               dev;
  int                elementCount;
  short              dataType;
  double             value;
  double             hopr;
  double             lopr;
  short              precision;
  short              status;
  short              severity;
  Boolean            connected;
  Boolean            readAccess;
  Boolean            writeAccess;
  char               **stateStrings;
  char               *name;
  char               *attr; 
  char               *verb;
  char               *fullname;  /* device attr */
  XtPointer          array;
  cdev_TS_STAMP      time;
  int                autoscale;

  XtPointer          clientData;
  void (*updateValueCb)(XtPointer); 
  void (*updateGraphicalInfoCb)(XtPointer); 
  Boolean  monitorSeverityChanged;
  Boolean  monitorValueChanged;
  Boolean  monitorZeroAndNoneZeroTransition;
} Record;

extern void medmDestroyRecord            (Record *pr);
extern void medmRecordAddUpdateValueCb   (Record *pr, 
					  void (*updateValueCb)(XtPointer));
extern void medmRecordAddGraphicalInfoCb (Record *pr, 
					  void (*updateGraphicalInfoCb)(XtPointer));

#define CA_PEND_EVENT_TIME 1e-6			/* formerly 0.0001 */
#define ca_pend_event cdevPend

#define NO_ALARM		0x0
#define	MINOR_ALARM		0x1
#define	MAJOR_ALARM		0x2
#define INVALID_ALARM		0x3
#define ALARM_NSEV		INVALID_ALARM+1

typedef long                    dbr_long_t;
typedef cdev_TS_STAMP           TS_STAMP;

#define MAX_ENUM_STATES		16
#define db_state_dim		MAX_ENUM_STATES	
#define MAX_STRING_SIZE         40

typedef struct _pv_type_name_
{
  char *name;
  int   type;
}medmPvTypeName;

struct dbr_time_string{
  short  	status;	 		/* status of value */
  short	        severity;		/* severity of alarm */
  cdev_TS_STAMP	stamp;			/* time stamp */
  char* 	value;			/* current value */
};


/* Misc utility routines */
extern char* medmPvName                 (Record* pr);

extern char* medmPvType                 (Record* pr);

extern int   medmPvDataType             (Record* pr);

extern int   medmPvCount                (Record* pr);

extern int   medmPvGetValue             (Record* pr, struct dbr_time_string *value);

enum tsTextType{
    TS_TEXT_MONDDYYYY,
    TS_TEXT_MMDDYY
};

char * medm_tsStampToText (cdev_TS_STAMP* ts, int type, char* buffer);

#define tsStampToText medm_tsStampToText


#define  PVNAME_STRINGSZ 29	/* includes NULL terminator for PVNAME_SZ */
#define	 PVNAME_SZ  (PVNAME_STRINGSZ - 1) /*Process Variable Name Size	*/
#define	 FLDNAME_SZ   4	                  /*Field Name Size		*/

#if defined (__cplusplus)
};
#endif

#endif     /* #ifdef MEDM_CDEV */

#endif     /* #ifndef _MEDM_CDEV_H */
