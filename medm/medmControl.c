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
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (630-252-2000).
*/
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
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
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
