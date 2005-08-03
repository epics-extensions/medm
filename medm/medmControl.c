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

void controlAttributeInit(DlControl *control) {
    control->ctrl[0] = '\0';
    control->clr = 14;
    control->bclr = 0;
}

void parseControl(
  DisplayInfo *displayInfo,
  DlControl *control)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;

    do {
        switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"ctrl") ||
	      !strcmp(token,"chan")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(control->ctrl,token);
	    } else if (!strcmp(token,"clr")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		control->clr = atoi(token) % DL_MAX_COLORS;
	    } else if (!strcmp(token,"bclr")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		control->bclr = atoi(token) % DL_MAX_COLORS;
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

void writeDlControl(
  FILE *stream,
  DlControl *dlControl,
  int level)
{
    char indent[16];

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
	fprintf(stream,"\n%scontrol {",indent);
	if (dlControl->ctrl[0] != '\0')
	  fprintf(stream,"\n%s\tchan=\"%s\"",indent,dlControl->ctrl);
	fprintf(stream,"\n%s\tclr=%d",indent,dlControl->clr);
	fprintf(stream,"\n%s\tbclr=%d",indent,dlControl->bclr);
	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%scontrol {",indent);
	fprintf(stream,"\n%s\tctrl=\"%s\"",indent,dlControl->ctrl);
	fprintf(stream,"\n%s\tclr=%d",indent,dlControl->clr);
	fprintf(stream,"\n%s\tbclr=%d",indent,dlControl->bclr);
	fprintf(stream,"\n%s}",indent);
    }
#endif
}
