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
 * .03  09-12-95        vong    conform to c++ syntax
 *
 *****************************************************************************
*/

#include "medm.h"

#include <X11/keysym.h>



static DlObject defaultObject = {0,0,5,5};
static DlMonitor defaultMonitor = {"",0,1};
static DlPlotcom defaultPlotcom = {"","","",0,1,""};
static DlPlotAxisDefinition defaultPlotAxisDefinition = {LINEAR_AXIS,
	CHANNEL_RANGE,0.0,0.0};


/***
 *** Meter
 ***/



void parseMeter(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlMeter *dlMeter;
  DlElement *dlElement;

  dlMeter = (DlMeter *) malloc(sizeof(DlMeter));

/* initialize some data in structure */
  dlMeter->object = defaultObject;
  dlMeter->monitor = defaultMonitor;
  dlMeter->label = LABEL_NONE;
  dlMeter->clrmod = STATIC;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object"))
			parseObject(displayInfo,&(dlMeter->object));
		else if (!strcmp(token,"monitor"))
			parseMonitor(displayInfo,&(dlMeter->monitor));
		else if (!strcmp(token,"label")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"none")) 
			    dlMeter->label = LABEL_NONE;
			else if (!strcmp(token,"outline"))
			    dlMeter->label = OUTLINE;
			else if (!strcmp(token,"limits"))
			    dlMeter->label = LIMITS;
			else if (!strcmp(token,"channel"))
			    dlMeter->label = CHANNEL;
		} else if (!strcmp(token,"clrmod")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"static")) 
			    dlMeter->clrmod = STATIC;
			else if (!strcmp(token,"alarm"))
			    dlMeter->clrmod = ALARM;
			else if (!strcmp(token,"discrete"))
			    dlMeter->clrmod = DISCRETE;
		}
		break;
	    case T_EQUAL:
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
		&& (tokenType != T_EOF) );

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Meter;
  dlElement->structure.meter = dlMeter;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (medmExecProc)executeDlMeter;
  dlElement->dmWrite =  (medmWriteProc)writeDlMeter;

}



/***
 *** Bar
 ***/


void parseBar(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlBar *dlBar;
  DlElement *dlElement;


  dlBar = (DlBar *) malloc(sizeof(DlBar));

/* initialize some data in structure */
  dlBar->object = defaultObject;
  dlBar->monitor= defaultMonitor;
  dlBar->label = LABEL_NONE;
  dlBar->clrmod = STATIC;
  dlBar->direction = UP;
  dlBar->fillmod = FROM_EDGE;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object")) {
			parseObject(displayInfo,&(dlBar->object));
		} else if (!strcmp(token,"monitor")) {
			parseMonitor(displayInfo,&(dlBar->monitor));
		} else if (!strcmp(token,"label")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"none")) 
			    dlBar->label = LABEL_NONE;
			else if (!strcmp(token,"outline"))
			    dlBar->label = OUTLINE;
			else if (!strcmp(token,"limits"))
			    dlBar->label = LIMITS;
			else if (!strcmp(token,"channel"))
			    dlBar->label = CHANNEL;
		} else if (!strcmp(token,"clrmod")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"static")) 
			    dlBar->clrmod = STATIC;
			else if (!strcmp(token,"alarm"))
			    dlBar->clrmod = ALARM;
			else if (!strcmp(token,"discrete"))
			    dlBar->clrmod = DISCRETE;
		} else if (!strcmp(token,"direction")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"up")) 
			    dlBar->direction = UP;
			else if (!strcmp(token,"down"))
			    dlBar->direction = DOWN;
			else if (!strcmp(token,"right"))
			    dlBar->direction = RIGHT;
			else if (!strcmp(token,"left"))
			    dlBar->direction = LEFT;
		} else if (!strcmp(token,"fillmod")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"from edge")) 
			    dlBar->fillmod = FROM_EDGE;
                        else if(!strcmp(token,"from center")) dlBar->fillmod = FROM_CENTER;
		}
		break;
	    case T_EQUAL:
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
		&& (tokenType != T_EOF) );

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Bar;
  dlElement->structure.bar = dlBar;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (medmExecProc)executeDlBar;
  dlElement->dmWrite =  (medmWriteProc)writeDlBar;

}

void parseByte( DisplayInfo *displayInfo, DlComposite *dlComposite) {
/****************************************************************************
 * Parse Byte                                                               *
 ****************************************************************************/
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlByte *dlByte;
  DlElement *dlElement;

    dlByte = (DlByte *) malloc(sizeof(DlByte));

/****** Initialize some data in structure */
    dlByte->object = defaultObject;
    dlByte->monitor= defaultMonitor;
    dlByte->clrmod = STATIC;
    dlByte->direction = UP;
    dlByte->sbit = 0;
    dlByte->ebit = 15;

    do {
      switch( (tokenType=getToken(displayInfo,token)) ) {
        case T_WORD:
          if (!strcmp(token,"object")) {
            parseObject(displayInfo,&(dlByte->object));
          } else if (!strcmp(token,"monitor")) {
            parseMonitor(displayInfo,&(dlByte->monitor));
          } else if (!strcmp(token,"clrmod")) {
            getToken(displayInfo,token);
            getToken(displayInfo,token);
            if (!strcmp(token,"static"))       dlByte->clrmod = STATIC;
            else if (!strcmp(token,"alarm"))   dlByte->clrmod = ALARM;
            else if (!strcmp(token,"discrete"))dlByte->clrmod = DISCRETE;
          } else if (!strcmp(token,"direction")) {
            getToken(displayInfo,token);
            getToken(displayInfo,token);
            if (!strcmp(token,"up"))        dlByte->direction = UP;
            else if (!strcmp(token,"right"))dlByte->direction = RIGHT;
          } else if (!strcmp(token,"sbit")) {
            getToken(displayInfo,token);
            getToken(displayInfo,token);
            dlByte->sbit = atoi(token);
          } else if (!strcmp(token,"ebit")) {
            getToken(displayInfo,token);
            getToken(displayInfo,token);
            dlByte->ebit = atoi(token);
          }
          break;
        case T_EQUAL:
          break;
        case T_LEFT_BRACE:
          nestingLevel++; break;
        case T_RIGHT_BRACE:
          nestingLevel--; break;
      }
    } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
                && (tokenType != T_EOF) );

    dlElement = (DlElement *) malloc(sizeof(DlElement));
    dlElement->type = DL_Byte;
    dlElement->structure.byte = dlByte;
    dlElement->next = NULL;

    POSITION_ELEMENT_ON_LIST();

    dlElement->dmExecute = (medmExecProc)executeDlByte;
    dlElement->dmWrite = (medmWriteProc)writeDlByte;
}

/***
 *** Indicator
 ***/


void parseIndicator(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlIndicator *dlIndicator;
  DlElement *dlElement;

  dlIndicator = (DlIndicator *) malloc(sizeof(DlIndicator));

/* initialize some data in structure */
  dlIndicator->object = defaultObject;
  dlIndicator->monitor= defaultMonitor;
  dlIndicator->label = LABEL_NONE;
  dlIndicator->clrmod = STATIC;
  dlIndicator->direction = UP;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object")) {
			parseObject(displayInfo,&(dlIndicator->object));
		} else if (!strcmp(token,"monitor")) {
			parseMonitor(displayInfo,&(dlIndicator->monitor));
		} else if (!strcmp(token,"label")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"none")) 
			    dlIndicator->label = LABEL_NONE;
			else if (!strcmp(token,"outline"))
			    dlIndicator->label = OUTLINE;
			else if (!strcmp(token,"limits"))
			    dlIndicator->label = LIMITS;
			else if (!strcmp(token,"channel"))
			    dlIndicator->label = CHANNEL;
		} else if (!strcmp(token,"clrmod")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"static")) 
			    dlIndicator->clrmod = STATIC;
			else if (!strcmp(token,"alarm"))
			    dlIndicator->clrmod = ALARM;
			else if (!strcmp(token,"discrete"))
			    dlIndicator->clrmod = DISCRETE;
		} else if (!strcmp(token,"direction")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"up")) 
			    dlIndicator->direction = UP;
			else if (!strcmp(token,"down"))
			    dlIndicator->direction = DOWN;
			else if (!strcmp(token,"right"))
			    dlIndicator->direction = RIGHT;
			else if (!strcmp(token,"left"))
			    dlIndicator->direction = LEFT;
		}
		break;
	    case T_EQUAL:
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
		&& (tokenType != T_EOF) );

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Indicator;
  dlElement->structure.indicator = dlIndicator;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (medmExecProc)executeDlIndicator;
  dlElement->dmWrite =  (medmWriteProc)writeDlIndicator;

}





/***
 *** Text Update
 ***/


void parseTextUpdate(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlTextUpdate *dlTextUpdate;
  DlElement *dlElement;

  dlTextUpdate = (DlTextUpdate *) malloc(sizeof(DlTextUpdate));

/* initialize some data in structure */
  dlTextUpdate->object = defaultObject;
  dlTextUpdate->monitor= defaultMonitor;
  dlTextUpdate->clrmod = STATIC;
  dlTextUpdate->align = HORIZ_LEFT;
  dlTextUpdate->format = DECIMAL;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object")) {
			parseObject(displayInfo,&(dlTextUpdate->object));
		} else if (!strcmp(token,"monitor")) {
			parseMonitor(displayInfo,&(dlTextUpdate->monitor));
		} else if (!strcmp(token,"clrmod")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"static")) 
			    dlTextUpdate->clrmod = STATIC;
			else if (!strcmp(token,"alarm"))
			    dlTextUpdate->clrmod = ALARM;
			else if (!strcmp(token,"discrete"))
			    dlTextUpdate->clrmod = DISCRETE;
		} else if (!strcmp(token,"format")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"decimal")) {
				dlTextUpdate->format = DECIMAL;
			} else if (!strcmp(token,
					"decimal- exponential notation")) {
				dlTextUpdate->format = EXPONENTIAL;
			} else if (!strcmp(token,"exponential")) {
				dlTextUpdate->format = EXPONENTIAL;
			} else if (!strcmp(token,"engr. notation")) {
				dlTextUpdate->format = ENGR_NOTATION;
			} else if (!strcmp(token,"decimal- compact")) {
				dlTextUpdate->format = COMPACT;
			} else if (!strcmp(token,"compact")) {
				dlTextUpdate->format = COMPACT;
			} else if (!strcmp(token,"decimal- truncated")) {
				dlTextUpdate->format = TRUNCATED;
/* (MDA) allow for LANL spelling errors {like above, but with trailing space} */
			} else if (!strcmp(token,"decimal- truncated ")) {
				dlTextUpdate->format = TRUNCATED;
			} else if (!strcmp(token,"truncated")) {
				dlTextUpdate->format = TRUNCATED;
			} else if (!strcmp(token,"hexadecimal")) {
				dlTextUpdate->format = HEXADECIMAL;
/* (MDA) allow for LANL spelling errors {hexidecimal vs. hexadecimal} */
			} else if (!strcmp(token,"hexidecimal")) {
				dlTextUpdate->format = HEXADECIMAL;
			} else if (!strcmp(token,"octal")) {
				dlTextUpdate->format = OCTAL;
			}
		} else if (!strcmp(token,"align")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"horiz. left")) {
				dlTextUpdate->align = HORIZ_LEFT;
			} else if (!strcmp(token,"horiz. centered")) {
				dlTextUpdate->align = HORIZ_CENTER;
			} else if (!strcmp(token,"horiz. right")) {
				dlTextUpdate->align = HORIZ_RIGHT;
			} else if (!strcmp(token,"vert. top")) {
				dlTextUpdate->align = VERT_TOP;
			} else if (!strcmp(token,"vert. bottom")) {
				dlTextUpdate->align = VERT_BOTTOM;
			} else if (!strcmp(token,"vert. centered")) {
				dlTextUpdate->align = VERT_CENTER;
			}
		}
		break;
	    case T_EQUAL:
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
		&& (tokenType != T_EOF) );


  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_TextUpdate;
  dlElement->structure.textUpdate = dlTextUpdate;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (medmExecProc)executeDlTextUpdate;
  dlElement->dmWrite =  (medmWriteProc)writeDlTextUpdate;

}





/***
 ***  Strip Chart
 ***/
#ifdef __cplusplus
extern "C" {
  void linear_scale(double xmin, double xmax, int n,
       double *xminp, double *xmaxp, double *dist);
}
#endif

void parseStripChart(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlStripChart *dlStripChart;
  DlElement *dlElement;
  int penNumber;
  int isVersion2_1_x = False;


  dlStripChart = (DlStripChart *) calloc(1,sizeof(DlStripChart));

/* initialize some data in structure */
  dlStripChart->object = defaultObject;
  dlStripChart->plotcom = defaultPlotcom;
  dlStripChart->period = 60.0;
  dlStripChart->delay = -1.0;
  dlStripChart->units = SECONDS;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object"))
			parseObject(displayInfo,&(dlStripChart->object));
		else if (!strcmp(token,"plotcom"))
			parsePlotcom(displayInfo,&(dlStripChart->plotcom));
		else if (!strcmp(token,"period")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlStripChart->period = atof(token);
                        isVersion2_1_x = True;
		} else if (!strcmp(token,"delay")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlStripChart->delay = atoi(token);
		} else if (!strcmp(token,"units")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"minute")) 
			    dlStripChart->units = MINUTES;
			else if (!strcmp(token,"second")) 
			    dlStripChart->units = SECONDS;
			else if (!strcmp(token,"milli second")) 
			    dlStripChart->units = MILLISECONDS;
			else if (!strcmp(token,"milli-second")) 
			    dlStripChart->units = MILLISECONDS;
			else
			    fprintf(stderr,
			    "\nparseStripChart: illegal units %s,%s",token,
			    "default of SECONDS taken");
		} else if (!strncmp(token,"pen",3)) {
			penNumber = MIN(token[4] - '0', MAX_PENS-1);
			parsePen(displayInfo,&(dlStripChart->pen[penNumber]));
		}
		break;
	    case T_EQUAL:
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
		&& (tokenType != T_EOF) );

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_StripChart;
  dlElement->structure.stripChart = dlStripChart;
  dlElement->next = NULL;

  if (isVersion2_1_x) {
    dlStripChart->delay = -1.0;  /* -1.0 is used as a indicator to save
                                    as new format */
  } else
  if (dlStripChart->delay > 0) {
    double val, dummy1, dummy2;
    switch (dlStripChart->units) {
      case MILLISECONDS:
        dummy1 = -0.060 * (double) dlStripChart->delay;
        break;
      case SECONDS:
        dummy1 = -60 * (double) dlStripChart->delay;
        break;
      case MINUTES:
        dummy1 = -3600.0 * (double) dlStripChart->delay;
        break;
      default:
        dummy1 = -60 * (double) dlStripChart->delay;
        break;
    }

    linear_scale(dummy1, 0.0, 2, &val, &dummy1, &dummy2);
    dlStripChart->period = -val; 
    dlStripChart->oldUnits = dlStripChart->units;
    dlStripChart->units = SECONDS;
  }

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (medmExecProc)executeDlStripChart;
  dlElement->dmWrite =  (medmWriteProc)writeDlStripChart;

}



/***
 ***  Cartesian Plot
 ***/


void parseCartesianPlot(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlCartesianPlot *dlCartesianPlot;
  DlElement *dlElement;
  int traceNumber;

  dlCartesianPlot = (DlCartesianPlot *) calloc(1,sizeof(DlCartesianPlot));

/* initialize some data in structure */
  dlCartesianPlot->object = defaultObject;
  dlCartesianPlot->plotcom = defaultPlotcom;
  dlCartesianPlot->style = POINT_PLOT;
  dlCartesianPlot->erase_oldest = ERASE_OLDEST_OFF;
  dlCartesianPlot->axis[X_AXIS_ELEMENT] = defaultPlotAxisDefinition;
  dlCartesianPlot->axis[Y1_AXIS_ELEMENT] = defaultPlotAxisDefinition;
  dlCartesianPlot->axis[Y2_AXIS_ELEMENT] = defaultPlotAxisDefinition;
  dlCartesianPlot->trigger[0] = '\0';
  dlCartesianPlot->erase[0] = '\0';
  dlCartesianPlot->eraseMode = ERASE_IF_NOT_ZERO;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object"))
			parseObject(displayInfo,&(dlCartesianPlot->object));
		else if (!strcmp(token,"plotcom"))
			parsePlotcom(displayInfo,&(dlCartesianPlot->plotcom));
		else if (!strcmp(token,"count")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlCartesianPlot->count= atoi(token);
		} else if (!strcmp(token,"style")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"point plot")) 
			    dlCartesianPlot->style = POINT_PLOT;
			else if (!strcmp(token,"point")) 
			    dlCartesianPlot->style = POINT_PLOT;
			else if (!strcmp(token,"line plot")) 
			    dlCartesianPlot->style = LINE_PLOT;
			else if (!strcmp(token,"line")) 
			    dlCartesianPlot->style = LINE_PLOT;
			else if (!strcmp(token,"fill under")) 
			    dlCartesianPlot->style = FILL_UNDER_PLOT;
			else if (!strcmp(token,"fill-under")) 
			    dlCartesianPlot->style = FILL_UNDER_PLOT;
		} else if (!strcmp(token,"erase_oldest")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"on")) 
			    dlCartesianPlot->erase_oldest = ERASE_OLDEST_ON;
			else if (!strcmp(token,"off")) 
			    dlCartesianPlot->erase_oldest = ERASE_OLDEST_OFF;
			else if (!strcmp(token,"plot last n pts")) 
			    dlCartesianPlot->erase_oldest = ERASE_OLDEST_ON;
			else if (!strcmp(token,"plot n pts & stop")) 
			    dlCartesianPlot->erase_oldest = ERASE_OLDEST_OFF;
		} else if (!strncmp(token,"trace",5)) {
			traceNumber = MIN(token[6] - '0', MAX_TRACES - 1);
			parseTrace(displayInfo,
				&(dlCartesianPlot->trace[traceNumber]));
		} else if (!strcmp(token,"x_axis")) {
			parsePlotAxisDefinition(displayInfo,
				&(dlCartesianPlot->axis[X_AXIS_ELEMENT]));
		} else if (!strcmp(token,"y1_axis")) {
			parsePlotAxisDefinition(displayInfo,
				&(dlCartesianPlot->axis[Y1_AXIS_ELEMENT]));
		} else if (!strcmp(token,"y2_axis")) {
			parsePlotAxisDefinition(displayInfo,
				&(dlCartesianPlot->axis[Y2_AXIS_ELEMENT]));
		} else if (!strcmp(token,"trigger")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(dlCartesianPlot->trigger,token);
		} else if (!strcmp(token,"erase")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(dlCartesianPlot->erase,token);
		} else if (!strcmp(token,"eraseMode")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"if not zero"))
			   dlCartesianPlot->eraseMode = ERASE_IF_NOT_ZERO;
			else if (!strcmp(token,"if zero"))
			   dlCartesianPlot->eraseMode = ERASE_IF_ZERO;
                }
		break;
	    case T_EQUAL:
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
		&& (tokenType != T_EOF) );

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_CartesianPlot;
  dlElement->structure.cartesianPlot = dlCartesianPlot;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (medmExecProc)executeDlCartesianPlot;
  dlElement->dmWrite =  (medmWriteProc)writeDlCartesianPlot;

}



#if 0
/***
 ***  Surface Plot
 ***/


void parseSurfacePlot(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlSurfacePlot *dlSurfacePlot;
  DlElement *dlElement;

  dlSurfacePlot = (DlSurfacePlot *) malloc(sizeof(DlSurfacePlot));

/* initialize some data in structure */
  dlSurfacePlot->object = defaultObject;
  dlSurfacePlot->plotcom = defaultPlotcom;
  dlSurfacePlot->data[0] = '\0';;
  dlSurfacePlot->data_clr = 0;
  dlSurfacePlot->dis = 10;
  dlSurfacePlot->xyangle = 45;
  dlSurfacePlot->zangle = 45;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object"))
			parseObject(displayInfo,&(dlSurfacePlot->object));
		else if (!strcmp(token,"plotcom"))
			parsePlotcom(displayInfo,&(dlSurfacePlot->plotcom));
		else if (!strcmp(token,"data")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(dlSurfacePlot->data,token);
		} else if (!strcmp(token,"data_clr")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlSurfacePlot->data_clr = atoi(token) % DL_MAX_COLORS;
		} else if (!strcmp(token,"dis")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlSurfacePlot->dis = atoi(token);
		} else if (!strcmp(token,"xyangle")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlSurfacePlot->xyangle = atoi(token);
		} else if (!strcmp(token,"zangle")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlSurfacePlot->zangle = atoi(token);
		}
		break;
	    case T_EQUAL:
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
		&& (tokenType != T_EOF) );

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_SurfacePlot;
  dlElement->structure.surfacePlot = dlSurfacePlot;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlSurfacePlot;
  dlElement->dmWrite =  (void(*)())writeDlSurfacePlot;
}
#endif


void parseMonitor(
  DisplayInfo *displayInfo,
  DlMonitor *monitor)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"rdbk")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(monitor->rdbk,token);
		} else if (!strcmp(token,"clr")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			monitor->clr = atoi(token) % DL_MAX_COLORS;
		} else if (!strcmp(token,"bclr")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			monitor->bclr = atoi(token) % DL_MAX_COLORS;
		}
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
		&& (tokenType != T_EOF) );
}



/***
 *** plotcom element in each plot type object
 ***/


void parsePlotcom(
  DisplayInfo *displayInfo,
  DlPlotcom *plotcom)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"title")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(plotcom->title,token);
		} else if (!strcmp(token,"xlabel")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(plotcom->xlabel,token);
		} else if (!strcmp(token,"ylabel")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(plotcom->ylabel,token);
		} else if (!strcmp(token,"package")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(plotcom->package,token);
		} else if (!strcmp(token,"clr")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			plotcom->clr = atoi(token) % DL_MAX_COLORS;
		} else if (!strcmp(token,"bclr")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			plotcom->bclr = atoi(token) % DL_MAX_COLORS;
		}
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
		&& (tokenType != T_EOF) );
}


/***
 *** PlotAxisDefinition element in each plot type object
 ***/


void parsePlotAxisDefinition(
  DisplayInfo *displayInfo,
  DlPlotAxisDefinition *dlPlotAxisDefinition)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"axisStyle")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"linear")) 
			    dlPlotAxisDefinition->axisStyle = LINEAR_AXIS;
			else if (!strcmp(token,"log10")) 
			    dlPlotAxisDefinition->axisStyle = LOG10_AXIS;
		} else if (!strcmp(token,"rangeStyle")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"from channel")) 
			    dlPlotAxisDefinition->rangeStyle = CHANNEL_RANGE;
			else if (!strcmp(token,"user-specified")) 
			    dlPlotAxisDefinition->rangeStyle =
							USER_SPECIFIED_RANGE;
			else if (!strcmp(token,"auto-scale")) 
			    dlPlotAxisDefinition->rangeStyle = AUTO_SCALE_RANGE;
		} else if (!strcmp(token,"minRange")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlPlotAxisDefinition->minRange = atof(token);
		} else if (!strcmp(token,"maxRange")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlPlotAxisDefinition->maxRange = atof(token);
		}
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
		&& (tokenType != T_EOF) );
}



/***
 *** pen element in each strip chart
 ***/


void parsePen(
  DisplayInfo *displayInfo,
  DlPen *pen)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"chan")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(pen->chan,token);
		} else if (!strcmp(token,"clr")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			pen->clr = atoi(token) % DL_MAX_COLORS;
		}
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
		&& (tokenType != T_EOF) );
}



/***
 *** trace element in each cartesian plot
 ***/


void parseTrace(
  DisplayInfo *displayInfo,
  DlTrace *trace)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"xdata")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(trace->xdata,token);
		} else if (!strcmp(token,"ydata")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(trace->ydata,token);
		} else if (!strcmp(token,"data_clr")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			trace->data_clr = atoi(token) % DL_MAX_COLORS;
		}
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
		&& (tokenType != T_EOF) );
}
