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
 * .02  09-12-95        vong    conform to c++ syntax
 *
 *****************************************************************************
*/

#include "medm.h"

static DlObject defaultObject = {0,0,5,5};
static DlControl defaultControl = {"",0,1};



/***
 *** Choice Button
 ***/

void parseChoiceButton(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlChoiceButton *dlChoiceButton;
  DlElement *dlElement;

  dlChoiceButton = (DlChoiceButton *) malloc(sizeof(DlChoiceButton));

/* initialize some data in structure */
  dlChoiceButton->object = defaultObject;
  dlChoiceButton->control = defaultControl;
  dlChoiceButton->clrmod = STATIC;
  dlChoiceButton->stacking = ROW;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object"))
			parseObject(displayInfo,&(dlChoiceButton->object));
		else if (!strcmp(token,"control"))
			parseControl(displayInfo,&(dlChoiceButton->control));
		else if (!strcmp(token,"clrmod")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"static")) 
			    dlChoiceButton->clrmod = STATIC;
			else if (!strcmp(token,"alarm"))
			    dlChoiceButton->clrmod = ALARM;
			else if (!strcmp(token,"discrete"))
			    dlChoiceButton->clrmod = DISCRETE;
		} else if (!strcmp(token,"stacking")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"row")) 
			    dlChoiceButton->stacking = ROW;
			else if (!strcmp(token,"column")) 
			    dlChoiceButton->stacking = COLUMN;
			else if (!strcmp(token,"row column")) 
			    dlChoiceButton->stacking = ROW_COLUMN;
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
  dlElement->type = DL_ChoiceButton;
  dlElement->structure.choiceButton = dlChoiceButton;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (medmExecProc)executeDlChoiceButton;
  dlElement->dmWrite =  (medmWriteProc)writeDlChoiceButton;

}


/***
 *** Message Button
 ***/

void parseMessageButton(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlMessageButton *dlMessageButton;
  DlElement *dlElement;

  dlMessageButton = (DlMessageButton *) malloc(sizeof(DlMessageButton));

/* initialize part of structure */
  dlMessageButton->object = defaultObject;
  dlMessageButton->control = defaultControl;
  dlMessageButton->label[0] = '\0';
  dlMessageButton->press_msg[0] = '\0';
  dlMessageButton->release_msg[0] = '\0';
  dlMessageButton->clrmod = STATIC;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object"))
			parseObject(displayInfo,
				&(dlMessageButton->object));
		else if (!strcmp(token,"control"))
			parseControl(displayInfo,
				&(dlMessageButton->control));
		else if (!strcmp(token,"press_msg")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(dlMessageButton->press_msg,token);
		} else if (!strcmp(token,"release_msg")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(dlMessageButton->release_msg,token);
		} else if (!strcmp(token,"label")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(dlMessageButton->label,token);
		} else if (!strcmp(token,"clrmod")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"static")) 
			    dlMessageButton->clrmod = STATIC;
			else if (!strcmp(token,"alarm"))
			    dlMessageButton->clrmod = ALARM;
			else if (!strcmp(token,"discrete"))
			    dlMessageButton->clrmod = DISCRETE;
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
  dlElement->type = DL_MessageButton;
  dlElement->structure.messageButton = dlMessageButton;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (medmExecProc)executeDlMessageButton;
  dlElement->dmWrite =  (medmWriteProc)writeDlMessageButton;

}


/***
 *** Valuator (Scale)
 ***/


void parseValuator(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlValuator *dlValuator;
  DlElement *dlElement;

  dlValuator = (DlValuator *) malloc(sizeof(DlValuator));

/* initialize OtherData structure */
  dlValuator->object = defaultObject;
  dlValuator->control = defaultControl;
  dlValuator->label = LABEL_NONE;
  dlValuator->clrmod = STATIC;
  dlValuator->direction = UP;
  dlValuator->dPrecision = 1.;

/* private run-time field */
  dlValuator->enableUpdates = True;
  dlValuator->dragging = False;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object"))
			parseObject(displayInfo,&(dlValuator->object));
		else if (!strcmp(token,"control"))
			parseControl(displayInfo,&(dlValuator->control));
		else if (!strcmp(token,"label")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
                        if (!strcmp(token,"none"))
                            dlValuator->label = LABEL_NONE;
                        else if (!strcmp(token,"outline"))
                            dlValuator->label = OUTLINE;
                        else if (!strcmp(token,"limits"))
                            dlValuator->label = LIMITS;
                        else if (!strcmp(token,"channel"))
                            dlValuator->label = CHANNEL;
		} else if (!strcmp(token,"clrmod")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"static")) 
			    dlValuator->clrmod = STATIC;
			else if (!strcmp(token,"alarm"))
			    dlValuator->clrmod = ALARM;
			else if (!strcmp(token,"discrete"))
			    dlValuator->clrmod = DISCRETE;
		} else if (!strcmp(token,"direction")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"up")) 
			    dlValuator->direction = UP;
			else if (!strcmp(token,"down"))
			    dlValuator->direction = DOWN;
			else if (!strcmp(token,"right"))
			    dlValuator->direction = RIGHT;
			else if (!strcmp(token,"left"))
			    dlValuator->direction = LEFT;
		} else if (!strcmp(token,"dPrecision")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlValuator->dPrecision = atof(token);
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
  dlElement->type = DL_Valuator;
  dlElement->structure.valuator = dlValuator;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (medmExecProc)executeDlValuator;
  dlElement->dmWrite =  (medmWriteProc)writeDlValuator;

}







/***
 *** Text Entry
 ***/

void parseTextEntry(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlTextEntry *dlTextEntry;
  DlElement *dlElement;

  dlTextEntry = (DlTextEntry *) malloc(sizeof(DlTextEntry));

/* initialize part of structure */
  dlTextEntry->object = defaultObject;
  dlTextEntry->control = defaultControl;
  dlTextEntry->clrmod = STATIC;
  dlTextEntry->format = DECIMAL;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object"))
			parseObject(displayInfo,&(dlTextEntry->object));
		else if (!strcmp(token,"control"))
			parseControl(displayInfo,&(dlTextEntry->control));
		else if (!strcmp(token,"clrmod")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"static")) 
			    dlTextEntry->clrmod = STATIC;
			else if (!strcmp(token,"alarm"))
			    dlTextEntry->clrmod = ALARM;
			else if (!strcmp(token,"discrete"))
			    dlTextEntry->clrmod = DISCRETE;
		} else if (!strcmp(token,"format")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"decimal")) {
				dlTextEntry->format = DECIMAL;
			} else if (!strcmp(token,
					"decimal- exponential notation")) {
				dlTextEntry->format = EXPONENTIAL;
			} else if (!strcmp(token,"exponential")) {
				dlTextEntry->format = EXPONENTIAL;
			} else if (!strcmp(token,"engr. notation")) {
				dlTextEntry->format = ENGR_NOTATION;
			} else if (!strcmp(token,"decimal- compact")) {
				dlTextEntry->format = COMPACT;
			} else if (!strcmp(token,"compact")) {
				dlTextEntry->format = COMPACT;
			} else if (!strcmp(token,"decimal- truncated")) {
				dlTextEntry->format = TRUNCATED;
			} else if (!strcmp(token,"truncated")) {
				dlTextEntry->format = TRUNCATED;
 			} else if (!strcmp(token,"hexadecimal")) {
				dlTextEntry->format = HEXADECIMAL;
			} else if (!strcmp(token,"octal")) {
				dlTextEntry->format = OCTAL;
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
  dlElement->type = DL_TextEntry;
  dlElement->structure.textEntry = dlTextEntry;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (medmExecProc)executeDlTextEntry;
  dlElement->dmWrite =  (medmWriteProc)writeDlTextEntry;

}



/***
 *** Menu
 ***/

void parseMenu(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlMenu *dlMenu;
  DlElement *dlElement;

  dlMenu = (DlMenu *) malloc(sizeof(DlMenu));

/* initialize part of structure */
  dlMenu->object = defaultObject;
  dlMenu->control = defaultControl;
  dlMenu->clrmod = STATIC;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object"))
			parseObject(displayInfo,&(dlMenu->object));
		else if (!strcmp(token,"control"))
			parseControl(displayInfo,&(dlMenu->control));
		else if (!strcmp(token,"clrmod")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"static")) 
			    dlMenu->clrmod = STATIC;
			else if (!strcmp(token,"alarm"))
			    dlMenu->clrmod = ALARM;
			else if (!strcmp(token,"discrete"))
			    dlMenu->clrmod = DISCRETE;
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
  dlElement->type = DL_Menu;
  dlElement->structure.menu = dlMenu;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (medmExecProc)executeDlMenu;
  dlElement->dmWrite =  (medmWriteProc)writeDlMenu;

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
		if (!strcmp(token,"ctrl")) {
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




