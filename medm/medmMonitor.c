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

#include "medm.h"
#define DEFAULT_CLR 14
#define DEFAULT_BCLR 3

void monitorAttributeInit(DlMonitor *monitor)
{
    monitor->rdbk[0] = '\0';
    monitor->clr = DEFAULT_CLR;
    monitor->bclr = DEFAULT_BCLR;
}

void plotcomAttributeInit(DlPlotcom *plotcom)
{
    plotcom->title[0] = '\0';
    plotcom->xlabel[0] = '\0';
    plotcom->ylabel[0] = '\0';
    plotcom->clr = DEFAULT_CLR;
    plotcom->bclr = DEFAULT_BCLR;
}

void plotAxisDefinitionInit(DlPlotAxisDefinition *axisDefinition)
{
    axisDefinition->axisStyle = LINEAR_AXIS;
    axisDefinition->rangeStyle = CHANNEL_RANGE;
    axisDefinition->minRange = 0.0;
    axisDefinition->maxRange = 1.0;
    axisDefinition->timeFormat = HHMMSS;
}

void penAttributeInit(DlPen *pen)
{
    pen->chan[0] = '\0';
    pen->clr = DEFAULT_CLR;
    limitsAttributeInit(&(pen->limits));
}

void traceAttributeInit(DlTrace *trace)
{
    trace->xdata[0] = '\0';
    trace->ydata[0] = '\0';
    trace->data_clr = DEFAULT_CLR;
}

void parseMonitor(DisplayInfo *displayInfo, DlMonitor *monitor)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"rdbk") ||
	      !strcmp(token,"chan")) {
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
	    nestingLevel++;
	    break;
	case T_RIGHT_BRACE:
	    nestingLevel--;
	    break;
	default:
	    break;
	}
    } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );
}

void parsePlotcom(DisplayInfo *displayInfo, DlPlotcom *plotcom)
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
	    nestingLevel++;
	    break;
	case T_RIGHT_BRACE:
	    nestingLevel--;
	    break;
	default:
	    break;
	}
    } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );
}


/***
 *** PlotAxisDefinition element in each plot type object
 ***/


void parsePlotAxisDefinition(DisplayInfo *displayInfo, DlPlotAxisDefinition *dlPlotAxisDefinition)
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
		else  if (!strcmp(token,"time"))
		  dlPlotAxisDefinition->axisStyle = TIME_AXIS;
	    } else if (!strcmp(token,"rangeStyle")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"from channel"))
		    dlPlotAxisDefinition->rangeStyle = CHANNEL_RANGE;
		else if (!strcmp(token,"user-specified"))
		  dlPlotAxisDefinition->rangeStyle = USER_SPECIFIED_RANGE;
		else if (!strcmp(token,"auto-scale"))
		  dlPlotAxisDefinition->rangeStyle = AUTO_SCALE_RANGE;
	    } else if (!strcmp(token,"minRange")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlPlotAxisDefinition->minRange = (float)atof(token);
	    } else if (!strcmp(token,"maxRange")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlPlotAxisDefinition->maxRange = (float)atof(token);
	    } else if (!strcmp(token,"timeFormat")) {
		int i;
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		for (i=FIRST_CP_TIME_FORMAT;
		     i<FIRST_CP_TIME_FORMAT+NUM_CP_TIME_FORMAT;i++) {
		    if (!strcmp(token,stringValueTable[i])) {
			dlPlotAxisDefinition->timeFormat = i;
			break;
		    }
		}
	    }
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++;
	    break;
	case T_RIGHT_BRACE:
	    nestingLevel--;
	    break;
	default:
	    break;
	}
    } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );
}



/***
 *** pen element in each strip chart
 ***/


void parsePen(DisplayInfo *displayInfo, DlPen *pen)
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
	    } else if (!strcmp(token,"limits")) {
		parseLimits(displayInfo,&pen->limits);
	    }
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++;
	    break;
	case T_RIGHT_BRACE:
	    nestingLevel--;
	    break;
	default:
	    break;
	}
    } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );
}

/***
 *** trace element in each cartesian plot
 ***/

void parseTrace(DisplayInfo *displayInfo, DlTrace *trace)
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
	    nestingLevel++;
	    break;
	case T_RIGHT_BRACE:
	    nestingLevel--;
	    break;
	default:
	    break;
	}
    } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );
}

void writeDlMonitor(FILE *stream, DlMonitor *dlMonitor, int level)
{
    int i;
    char indent[16];


    for (i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
	fprintf(stream,"\n%smonitor {",indent);
	if (dlMonitor->rdbk[0] != '\0')
	  fprintf(stream,"\n%s\tchan=\"%s\"",indent,dlMonitor->rdbk);
	fprintf(stream,"\n%s\tclr=%d",indent,dlMonitor->clr);
	fprintf(stream,"\n%s\tbclr=%d",indent,dlMonitor->bclr);
	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%smonitor {",indent);
	fprintf(stream,"\n%s\trdbk=\"%s\"",indent,dlMonitor->rdbk);
	fprintf(stream,"\n%s\tclr=%d",indent,dlMonitor->clr);
	fprintf(stream,"\n%s\tbclr=%d",indent,dlMonitor->bclr);
	fprintf(stream,"\n%s}",indent);
    }
#endif
}


void writeDlPlotcom(FILE *stream, DlPlotcom *dlPlotcom, int level)
{
    int i;
    char indent[16];

    for (i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
	fprintf(stream,"\n%splotcom {",indent);
	if (dlPlotcom->title[0] != '\0')
	  fprintf(stream,"\n%s\ttitle=\"%s\"",indent,dlPlotcom->title);
	if (dlPlotcom->xlabel[0] != '\0')
	  fprintf(stream,"\n%s\txlabel=\"%s\"",indent,dlPlotcom->xlabel);
	if (dlPlotcom->ylabel[0] != '\0')
	  fprintf(stream,"\n%s\tylabel=\"%s\"",indent,dlPlotcom->ylabel);
	fprintf(stream,"\n%s\tclr=%d",indent,dlPlotcom->clr);
	fprintf(stream,"\n%s\tbclr=%d",indent,dlPlotcom->bclr);
	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%splotcom {",indent);
	fprintf(stream,"\n%s\ttitle=\"%s\"",indent,dlPlotcom->title);
	fprintf(stream,"\n%s\txlabel=\"%s\"",indent,dlPlotcom->xlabel);
	fprintf(stream,"\n%s\tylabel=\"%s\"",indent,dlPlotcom->ylabel);
	fprintf(stream,"\n%s\tclr=%d",indent,dlPlotcom->clr);
	fprintf(stream,"\n%s\tbclr=%d",indent,dlPlotcom->bclr);
	fprintf(stream,"\n%s\tpackage=\"%s\"",indent,dlPlotcom->package);
	fprintf(stream,"\n%s}",indent);
    }
#endif
}


void writeDlPen(FILE *stream, DlPen *dlPen, int index, int level)
{
    int i;
    char indent[16];

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
  	if (dlPen->chan[0] == '\0') return;
#ifdef SUPPORT_0201XX_FILE_FORMAT
    }
#endif

    for (i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%spen[%d] {",indent,index);
    fprintf(stream,"\n%s\tchan=\"%s\"",indent,dlPen->chan);
    fprintf(stream,"\n%s\tclr=%d",indent,dlPen->clr);
    writeDlLimits(stream,&dlPen->limits,level+1);
    fprintf(stream,"\n%s}",indent);
}


void writeDlTrace(FILE *stream, DlTrace *dlTrace, int index, int level)
{
    int i;
    char indent[16];

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
	if ((dlTrace->xdata[0] == '\0') &&
	  (dlTrace->ydata[0] == '\0')) return;
#ifdef SUPPORT_0201XX_FILE_FORMAT
    }
#endif

    for (i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%strace[%d] {",indent,index);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
	if (dlTrace->xdata[0] != '\0')
	  fprintf(stream,"\n%s\txdata=\"%s\"",indent,dlTrace->xdata);
	if (dlTrace->ydata[0] != '\0')
	  fprintf(stream,"\n%s\tydata=\"%s\"",indent,dlTrace->ydata);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%s\txdata=\"%s\"",indent,dlTrace->xdata);
	fprintf(stream,"\n%s\tydata=\"%s\"",indent,dlTrace->ydata);
    }
#endif
    fprintf(stream,"\n%s\tdata_clr=%d",indent,dlTrace->data_clr);
    fprintf(stream,"\n%s}",indent);
}



void writeDlPlotAxisDefinition(FILE *stream, DlPlotAxisDefinition *dlPlotAxisDefinition,
  int axisNumber, int level)
{
    int i;
    char indent[16];

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
	if ((dlPlotAxisDefinition->axisStyle == LINEAR_AXIS) &&
	  (dlPlotAxisDefinition->rangeStyle == CHANNEL_RANGE) &&
	  (dlPlotAxisDefinition->minRange == 0.0) &&
	  (dlPlotAxisDefinition->maxRange == 1.0)) return;
#ifdef SUPPORT_0201XX_FILE_FORMAT
    }
#endif

    for (i = 0;  i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    switch (axisNumber) {
    case X_AXIS_ELEMENT: fprintf(stream,"\n%sx_axis {",indent); break;
    case Y1_AXIS_ELEMENT: fprintf(stream,"\n%sy1_axis {",indent); break;
    case Y2_AXIS_ELEMENT: fprintf(stream,"\n%sy2_axis {",indent); break;
    }
#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
	if (dlPlotAxisDefinition->axisStyle != LINEAR_AXIS)
	  fprintf(stream,"\n%s\taxisStyle=\"%s\"",indent,
	    stringValueTable[dlPlotAxisDefinition->axisStyle]);
	if (dlPlotAxisDefinition->rangeStyle != CHANNEL_RANGE)
	  fprintf(stream,"\n%s\trangeStyle=\"%s\"",indent,
	    stringValueTable[dlPlotAxisDefinition->rangeStyle]);
	if (dlPlotAxisDefinition->minRange != 0.0)
	  fprintf(stream,"\n%s\tminRange=%f",indent,dlPlotAxisDefinition->minRange);
	if (dlPlotAxisDefinition->maxRange != 1.0)
	  fprintf(stream,"\n%s\tmaxRange=%f",indent,dlPlotAxisDefinition->maxRange);
	if (dlPlotAxisDefinition->timeFormat != HHMMSS) {
	    fprintf(stream,"\n%s\ttimeFormat=\"%s\"",indent,
	      stringValueTable[dlPlotAxisDefinition->timeFormat]);

	}
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%s\taxisStyle=\"%s\"",indent,
	  stringValueTable[dlPlotAxisDefinition->axisStyle]);
	fprintf(stream,"\n%s\trangeStyle=\"%s\"",indent,
	  stringValueTable[dlPlotAxisDefinition->rangeStyle]);
	fprintf(stream,"\n%s\tminRange=%f",indent,dlPlotAxisDefinition->minRange);
	fprintf(stream,"\n%s\tmaxRange=%f",indent,dlPlotAxisDefinition->maxRange);
    }
#endif
    fprintf(stream,"\n%s}",indent);
}
