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

/*
 * include file for widget-based display manager
 */

#ifndef __MEDMCA_H__
#define __MEDMCA_H__

#define __USING_TIME_STAMP__

#ifdef ALLOCATE_STORAGE
#define EXTERN
#else
#define EXTERN extern
#endif


#include "cadef.h"
#include "db_access.h"
#include "alarm.h"			/* alarm status, severity */

#define CA_PEND_EVENT_TIME	1e-6			/* formerly 0.0001    */
#define MAX_STATE_STRING_SIZE	(db_state_text_dim+1)	/* from db_access.h   */
				/* also have MAX_STRING_SIZE from db_access.h */

/***
 *** new data types
 ***/

#ifdef __USING_TIME_STAMP__
    typedef union {
	struct dbr_time_string s;
	struct dbr_time_enum   e;
	struct dbr_time_char   c;
	struct dbr_time_short  i;
	struct dbr_time_long   l;
	struct dbr_time_float  f;
	struct dbr_time_double d;
    } dataBuf;
#else
typedef union {
    struct dbr_sts_string s;
    struct dbr_sts_enum   e;
    struct dbr_sts_char   c;
    struct dbr_sts_short  i;
    struct dbr_sts_long   l;
    struct dbr_sts_float  f;
    struct dbr_sts_double d;
} dataBuf;
#endif

typedef union {
    struct dbr_sts_string  s;
    struct dbr_ctrl_enum   e;
    struct dbr_ctrl_char   c;
    struct dbr_ctrl_short  i;
    struct dbr_ctrl_long   l;
    struct dbr_ctrl_float  f;
    struct dbr_ctrl_double d;
} infoBuf;

typedef struct _Record {
    int       caId;
    int       elementCount;
    short     dataType;
    double    value;
    double    hopr;
    double    lopr;
    short     precision;
    short     status;
    short     severity;
    Boolean   connected;
    Boolean   readAccess;
    Boolean   writeAccess;
    char      *stateStrings[16];
    char      *name;
    XtPointer array;
    TS_STAMP  time;

    XtPointer clientData;
    void (*updateValueCb)(XtPointer);
    void (*updateGraphicalInfoCb)(XtPointer);
    Boolean  monitorValueChanged;
    Boolean  monitorStatusChanged;
    Boolean  monitorSeverityChanged;
    Boolean  monitorZeroAndNoneZeroTransition;
} Record;

typedef struct _Channel {
    int       caId;
    dataBuf   *data;
    infoBuf   info;
    chid      chid;
    evid      evid;
    int       size;             /* size of data buffer (number of char) */
    Record    *pr;
    Boolean   previouslyConnected;
} Channel;

void medmDestroyRecord(Record *pr);
void medmRecordAddUpdateValueCb(Record *pr, void (*updateValueCb)(XtPointer));
void medmRecordAddGraphicalInfoCb(Record *pr, void (*updateGraphicalInfoCb)(XtPointer));
#endif  /* __MEDMCA_H__ */
