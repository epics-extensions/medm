/*
 * include file for widget-based display manager
 */

#ifndef __CADM_H__
#define __CADM_H__


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

typedef enum {
    CP_XYScalarX,       CP_XYScalarY,
    CP_XScalar,         CP_YScalar,
    CP_XVector,         CP_YVector,
    CP_XVectorYScalarX, CP_XVectorYScalarY,
    CP_YVectorXScalarX, CP_YVectorXScalarY,
    CP_XYVectorX,       CP_XYVectorY
} XYChannelTypeEnum;

typedef enum {
  NOT_MODIFIED,		/* monitor not modified                              */
  PRIMARY_MODIFIED,	/* modified and traversable (requires visual update) */
  SECONDARY_MODIFIED,	/* modified but visual update is pending other data  */
} ModifiedTypeEnum;

typedef struct _ChannelAccessMonitorData {
	DlMonitorType monitorType;		/* type of monitor obj       */
	XtPointer specifics;			/* display element pointer   */
        ModifiedTypeEnum modified;		/* data modified flag        */
	Boolean previouslyConnected;		/* chid previously connected */
        chid   chid;
        evid   evid;
        Widget self;
        short  status;				/* status information        */
        short  severity;
        short  oldSeverity;
	struct _ChannelAccessMonitorData *next;	/* ptr to next in list       */
	struct _ChannelAccessMonitorData *prev;	/* ptr to previous in list   */
	DisplayInfo *displayInfo;		/* display info struct       */

/* Database field and request type specific stuff is here  -  
   could union these together
*/
	/* numerics, enum and char types - store all values as doubles */
        double value;				/* current value             */
        double oldValue;			/* previous value            */
        double displayedValue;			/* currently displayed value */
	Boolean displayed;			/* has value been displayed? */
	/* string type - store as string */
	char stringValue[MAX_STRING_SIZE];	/* store string value        */
	char oldStringValue[MAX_STRING_SIZE];	/* previous string value     */

/* for enumerated types, store state strings too */
	short numberStateStrings;		/* number of state strings   */
	char **stateStrings;			/* the state strings         */
	XmString *xmStateStrings;		/* XmString state strings    */

        double hopr;				/* control information       */
        double lopr;
	short precision;			/* number of decimal places  */

/* monitor object specific stuff is here */
	int oldIntegerValue;			/* for valuators only        */
	ColorMode clrmod;			/* meter, bar, indicator,    */
	LabelType label;			/*   and valuator need these */
	int fontIndex;				/* text update needs this    */
	DlAttribute *dlAttr;			/* general attributes        */
	DlDynamicAttribute *dlDyn;		/* dynamic attrs./statics    */

	XrtData *xrtData;			/* xrtData for XRT/Graph     */
	int xrtDataSet;				/* xrtData set (1 or 2)      */
	int trace;				/* trace number in plot      */
	XYChannelTypeEnum xyChannelType;	/* XYScalar,[]Scalar[]Vector */
	struct _ChannelAccessMonitorData *other;/* ptr to other channel data */
	struct _ChannelAccessControllerData *controllerData;
						/* ptr to controllerData     */

} ChannelAccessMonitorData;



typedef struct _ChannelAccessControllerData {
	ChannelAccessMonitorData *monitorData;	/* monitor information       */
	DlControllerType controllerType;	/* type of controller obj    */
	Boolean connected;			/* channel connection state  */
        chid chid;
        Widget self;

/* Database field and request type specific stuff is here -- this is kept
   separate even though the monitor data structure has some of the same
   fields:  want the monitor structure to actually contain the most recent
   values from the IOC.  These fields are the "set" fields, and contain
   the values sent to the IOC...
*/
	/* numerics, enum and char types - store all values as doubles */
        double value;				/* current value             */
	/* string types - store as string */
	char   stringValue[MAX_STRING_SIZE];	/* store string values       */

        double hopr;				/* control information       */
        double lopr;

} ChannelAccessControllerData;





/*
 * global variables
 */

/* monitor data list head and tail (a single monitor list for all displays) */
EXTERN ChannelAccessMonitorData *channelAccessMonitorListHead;
EXTERN ChannelAccessMonitorData *channelAccessMonitorListTail;


#endif  /* __CADM_H__ */
