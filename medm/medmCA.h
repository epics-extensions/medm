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
 * .02  09-05-95        vong    2.1.0 release
 *
 *****************************************************************************
*/

/*
 * include file for widget-based display manager
 */

#ifndef __MEDMCA_H__
#define __MEDMCA_H__


#ifdef ALLOCATE_STORAGE
#define EXTERN
#else
#define EXTERN extern
#endif


#include "cadef.h"
#include "db_access.h"
#include "alarm.h"			/* alarm status, severity */

/* allow ca_build_and_connect to take NULL strings if user specifies them,
   and treat them as " " a whitespace string
 */
#define CA_BUILD_AND_CONNECT(a,b,c,d,e,f,g) \
	( a == NULL ? ca_build_and_connect(" ",b,c,d,e,f,g) : \
	 ( (a)[0] == '\0' ? ca_build_and_connect(" ",b,c,d,e,f,g) : \
	   ca_build_and_connect(a,b,c,d,e,f,g) ) )
	


#define CA_PEND_EVENT_TIME	1e-6			/* formerly 0.0001    */
#define MAX_STATE_STRING_SIZE	(db_state_text_dim+1)	/* from db_access.h   */
				/* also have MAX_STRING_SIZE from db_access.h */

/***
 *** new data types
 ***/

typedef union {
  struct dbr_time_string s;
  struct dbr_time_enum   e;
  struct dbr_time_char   c;
  struct dbr_time_short  i;
  struct dbr_time_long   l;
  struct dbr_time_float  f;
  struct dbr_time_double d;
} dataBuf;

typedef union {
  struct dbr_sts_string  s;
  struct dbr_ctrl_enum   e;
  struct dbr_ctrl_char   c;
  struct dbr_ctrl_short  i;
  struct dbr_ctrl_long   l;
  struct dbr_ctrl_float  f;
  struct dbr_ctrl_double d;
} infoBuf;

#if 0

typedef enum {
  NOT_MODIFIED,		/* monitor not modified                              */
  PRIMARY_MODIFIED,	/* modified and traversable (requires visual update) */
  SECONDARY_MODIFIED 	/* modified but visual update is pending other data  */
} ModifiedTypeEnum;

struct _ChannelList;

typedef enum {MEDMNoOp, MEDMNoConnection, MEDMNoReadAccess, MEDMNoWriteAccess,
	      MEDMUpdateValue} updateRequest_t;

typedef struct _Channel {
	DlMonitorType monitorType;		/* type of monitor obj       */
	XtPointer specifics;			/* display element pointer   */
	Boolean previouslyConnected;		/* chid previously connected */
        chid   chid;
        evid   evid;
        Widget self;
        short  status;				/* status information        */
        short  severity;
        short  oldSeverity;
	struct _Channel *next;	                /* ptr to next in list       */
	struct _Channel *prev;	                /* ptr to previous in list   */
	DisplayInfo *displayInfo;		/* display info struct       */

/* Database field and request type specific stuff is here  -  
   could union these together
*/
	/* numerics, enum and char types - store all values as doubles */
        double value;				/* current value             */
        double displayedValue;			/* currently displayed value */
	Boolean updateAllowed;	                /* allow screens update.     */
	/* string type - store as string */
	char stringValue[MAX_STRING_SIZE];	/* store string value        */

/* for enumerated types, store state strings too */
	short numberStateStrings;		/* number of state strings   */
	char **stateStrings;			/* the state strings         */
	XmString *xmStateStrings;		/* XmString state strings    */
	double pressValue;
	double releaseValue;

        double hopr;				/* control information       */
        double lopr;
	short precision;			/* number of decimal places  */

/* monitor object specific stuff is here */
	int oldIntegerValue;			/* for valuators only        */
	ColorMode clrmod;			/* meter, bar, indicator,    */
	VisibilityMode vismod;         
	LabelType label;			/*   and valuator need these */
	int fontIndex;				/* text update needs this    */
	DlAttribute *dlAttr;			/* general attributes        */

	XrtData *xrtData;			/* xrtData for XRT/Graph     */
	int xrtDataSet;				/* xrtData set (1 or 2)      */
	int trace;				/* trace number in plot      */
	XYChannelTypeEnum xyChannelType;	/* XYScalar,[]Scalar[]Vector */
	struct _Channel *other;                 /* ptr to other channel data */

/* graphical information */
	Pixel backgroundColor;                  /* back ground color */
	int   shadowBorderWidth;                /* shadow border width */

        /* callback routunes */
	void (*updateChannelCb)(struct _Channel *);
	void (*updateDataCb)(struct _Channel *);
	void (*updateGraphicalInfoCb)(struct _Channel*);
	void (*destroyChannel)(struct _Channel*);

	updateRequest_t lastUpdateRequest;

	/* update list */
	struct _ChannelList *updateList;
	dataBuf *data;
	infoBuf *info;
	int size;                               /* size of data buffer (number of char) */
	int caStatus;                           /* channel access status */
	Boolean handleArray;                    /* channel handle array */
	Boolean ignoreValueChanged;             /* Don't update if only value changed */
	Boolean opaque;                         /* Don't redraw background */
        Boolean dirty;		                /* data modified flag        */
} Channel;

/*
 * global variables
 */

#endif

#define MAX_EVENT_DATA 16
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

  XtPointer clientData;
  void (*updateValueCb)(XtPointer); 
  void (*updateGraphicalInfoCb)(XtPointer); 
  Boolean  monitorSeverityChanged;
  Boolean  monitorValueChanged;
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
