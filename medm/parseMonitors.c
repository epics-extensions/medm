
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

  dlElement->dmExecute =  (void(*)())executeDlMeter;
  dlElement->dmWrite =  (void(*)())writeDlMeter;

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

  dlElement->dmExecute =  (void(*)())executeDlBar;
  dlElement->dmWrite =  (void(*)())writeDlBar;

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

  dlElement->dmExecute =  (void(*)())executeDlIndicator;
  dlElement->dmWrite =  (void(*)())writeDlIndicator;

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

  dlElement->dmExecute =  (void(*)())executeDlTextUpdate;
  dlElement->dmWrite =  (void(*)())writeDlTextUpdate;

}





/***
 ***  Strip Chart
 ***/


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


  dlStripChart = (DlStripChart *) calloc(1,sizeof(DlStripChart));

/* initialize some data in structure */
  dlStripChart->object = defaultObject;
  dlStripChart->plotcom = defaultPlotcom;
  dlStripChart->delay = 500;
  dlStripChart->units = MILLISECONDS;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object"))
			parseObject(displayInfo,&(dlStripChart->object));
		else if (!strcmp(token,"plotcom"))
			parsePlotcom(displayInfo,&(dlStripChart->plotcom));
		else if (!strcmp(token,"delay")) {
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

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlStripChart;
  dlElement->dmWrite =  (void(*)())writeDlStripChart;

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

  dlElement->dmExecute =  (void(*)())executeDlCartesianPlot;
  dlElement->dmWrite =  (void(*)())writeDlCartesianPlot;

}



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
  int penNumber;

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



/****************************************************************
 *****    nested objects (not to be put in display list )   *****
/****************************************************************


/***
 *** monitor element in each monitor object
 ***/


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
