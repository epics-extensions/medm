
#include "medm.h"

#include <X11/keysym.h>


static DlObject defaultObject = {0,0,5,5};
#define DISPLAY_DEFAULT_X	10
#define DISPLAY_DEFAULT_Y	10



void parseFile(
  DisplayInfo *displayInfo)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlFile *dlFile;
  DlElement *dlElement;

  dlFile = (DlFile *) malloc(sizeof(DlFile));

/* initialize some data in structure */
  strcpy(dlFile->name,"display.adl");


  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"name")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(dlFile->name,token);
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
  dlElement->type = DL_File;
  dlElement->structure.file = dlFile;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute =  (void(*)())executeDlFile;
  dlElement->dmWrite =  (void(*)())writeDlFile;

}


void parseDisplay(
  DisplayInfo *displayInfo)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel;
  DlDisplay *dlDisplay;
  DlElement *dlElement;


  dlDisplay = (DlDisplay *) calloc(1,sizeof(DlDisplay));

/* initialize some data in structure */
  dlDisplay->object = defaultObject;
  dlDisplay->object.x = DISPLAY_DEFAULT_X;
  dlDisplay->object.y = DISPLAY_DEFAULT_Y;
  dlDisplay->clr = 0;
  dlDisplay->bclr = 1;
  dlDisplay->cmap[0] = '\0';


  nestingLevel = 0;
  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object")) {
			parseObject(displayInfo,&(dlDisplay->object));
		} else if (!strcmp(token,"cmap")) {
/* parse separate display list to get and use that colormap */
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (strlen(token) > 0) {
			    strcpy(dlDisplay->cmap,token);
			}
		} else if (!strcmp(token,"bclr")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlDisplay->bclr = atoi(token) % DL_MAX_COLORS;
			displayInfo->drawingAreaBackgroundColor =
				dlDisplay->bclr;
		} else if (!strcmp(token,"clr")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlDisplay->clr = atoi(token) % DL_MAX_COLORS;
			displayInfo->drawingAreaForegroundColor =
				dlDisplay->clr;
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

  /* fix up x,y so that 0,0 (old defaults) are replaced */
  if (dlDisplay->object.x <= 0) dlDisplay->object.x = DISPLAY_DEFAULT_X;
  if (dlDisplay->object.y <= 0) dlDisplay->object.y = DISPLAY_DEFAULT_Y;

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Display;
  dlElement->structure.display = dlDisplay;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute =  (void(*)())executeDlDisplay;
  dlElement->dmWrite =  (void(*)())writeDlDisplay;

}



/* parseColormap and parseDlColor have two arguments, since could in fact
 *   be parsing and external colormap file, hence need to pass in the 
 *   explicit file ptr for the current colormap file
 */
DlColormap *parseColormap(displayInfo,filePtr)
  DisplayInfo *displayInfo;
  FILE *filePtr;
{
  char token[MAX_TOKEN_LENGTH];
  char msg[2*MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlColormap *dlColormap;
  DlColormapEntry dummyColormapEntry;
  DlElement *dlElement, *dlTarget;
  int counter;

  FILE *savedFilePtr;


/*
 * (MDA) have to be sneaky for these colormap parsing routines:
 *	since possibly external colormap, save and restore 
 *	external file ptr in  displayInfo so that getToken()
 *	works with displayInfo and not the filePtr directly
 */
  savedFilePtr = displayInfo->filePtr;
  displayInfo->filePtr = filePtr;

  dlColormap = (DlColormap *) calloc(1,sizeof(DlColormap));

/* initialize some data in structure */
  dlColormap->ncolors = 0;


/* new colormap, get values (pixel values are being stored) */
  counter = 0;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"ncolors")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlColormap->ncolors = atoi(token);
			if (dlColormap->ncolors > DL_MAX_COLORS) {
			   sprintf(msg,"%s%s%s",
			"Maximum # of colors in colormap exceeded;\n\n",
			"truncating color space, but will continue...\n\n",
			"(you may want to change the colors of some objects)");
			   fprintf(stderr,"\n%s\n",msg);
			   dmSetAndPopupWarningDialog(displayInfo, msg,
					(XtCallbackProc)warnCallback,
					(XtCallbackProc)warnCallback);
			}
		} else if (!strcmp(token,"dl_color")) {
		/* continue parsing but throw away "excess" colormap entries */
			if (counter < DL_MAX_COLORS) {
			  parseDlColor(displayInfo,filePtr,
				&(dlColormap->dl_color[counter]));
			  counter++;
			} else {
			  parseDlColor(displayInfo,filePtr,
				&dummyColormapEntry);
			  counter++;
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
  dlElement->type = DL_Colormap;
  dlElement->structure.colormap = dlColormap;
  dlElement->next = NULL;
  dlElement->prev = displayInfo->dlElementListTail;
  displayInfo->dlElementListTail->next = dlElement;
  displayInfo->dlElementListTail = dlElement;
  dlElement->dmExecute =  (void(*)())executeDlColormap;
  dlElement->dmWrite =  (void(*)())writeDlColormap;

  displayInfo->dlColormapElement = dlElement;
/*
 *  now since a valid colormap element has been brought into display list,
 *  remove the external cmap reference in the dlDisplay element
 */
  dlTarget = displayInfo->dlElementListHead->next;
  while (dlTarget != NULL &&
	dlTarget->type != DL_Display) dlTarget = dlTarget->next;
  if (dlTarget != NULL) dlTarget->structure.display->cmap[0] = '\0';

/* make sure colormap element is right after display */
  if (dlElement->prev->type != DL_Display)
	moveElementAfter(displayInfo,NULL,dlElement,dlTarget);


/* restore the previous filePtr */
  displayInfo->filePtr = savedFilePtr;

  return (dlColormap);
}



void parseBasicAttribute(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlBasicAttribute *dlBasicAttribute;
  DlElement *dlElement;

  dlBasicAttribute = (DlBasicAttribute *) malloc(sizeof(DlBasicAttribute));

/* initialize some data in structure */
  dlBasicAttribute->attr.clr = 0;
  dlBasicAttribute->attr.style = SOLID;
  dlBasicAttribute->attr.fill = F_SOLID;
  dlBasicAttribute->attr.width = 0;


  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"attr"))parseAttr(displayInfo,
			&(dlBasicAttribute->attr));
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
		&& (tokenType != T_EOF) );


  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_BasicAttribute;
  dlElement->structure.basicAttribute = dlBasicAttribute;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlBasicAttribute;
  dlElement->dmWrite =  (void(*)())writeDlBasicAttribute;

}



DlDynamicAttribute *parseDynamicAttribute(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlDynamicAttribute *dlDynamicAttribute;
  DlElement *dlElement;

  dlDynamicAttribute = (DlDynamicAttribute *)malloc(sizeof(DlDynamicAttribute));

/* initialize some data in structure */
  dlDynamicAttribute->attr.mod.clr = STATIC;	/* ColorMode, actually */
  dlDynamicAttribute->attr.mod.vis = V_STATIC;
  dlDynamicAttribute->attr.param.chan[0] = '\0';


  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"attr"))parseDynamicAttr(displayInfo,
			&(dlDynamicAttribute->attr));
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
		&& (tokenType != T_EOF) );

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_DynamicAttribute;
  dlElement->structure.dynamicAttribute = dlDynamicAttribute;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlDynamicAttribute;
  dlElement->dmWrite =  (void(*)())writeDlDynamicAttribute;

  return (dlDynamicAttribute);
}





void parseRectangle(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlRectangle *dlRectangle;
  DlElement *dlElement;

  dlRectangle = (DlRectangle *) malloc(sizeof(DlRectangle));

/* initialize some data in structure */
  dlRectangle->object = defaultObject;


  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object"))
			parseObject(displayInfo,&(dlRectangle->object));
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
  dlElement->type = DL_Rectangle;
  dlElement->structure.rectangle = dlRectangle;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlRectangle;
  dlElement->dmWrite =  (void(*)())writeDlRectangle;

}


void parseOval(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlOval *dlOval;
  DlElement *dlElement;

  dlOval = (DlOval *) malloc(sizeof(DlOval));

/* initialize some data in structure */
  dlOval->object = defaultObject;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object"))
			parseObject(displayInfo,&(dlOval->object));
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
  dlElement->type = DL_Oval;
  dlElement->structure.oval = dlOval;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlOval;
  dlElement->dmWrite =  (void(*)())writeDlOval;

}



void parseArc(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlArc *dlArc;
  DlElement *dlElement;

  dlArc = (DlArc*) malloc(sizeof(DlArc));

/* initialize some data in structure */
  dlArc->object = defaultObject;
  dlArc->begin = 0;
  dlArc->path = 64*90;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object")) {
			parseObject(displayInfo,&(dlArc->object));
		} else if (!strcmp(token,"begin")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlArc->begin = atoi(token);
		} else if (!strcmp(token,"path")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlArc->path = atoi(token);
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
  dlElement->type = DL_Arc;
  dlElement->structure.arc = dlArc;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlArc;
  dlElement->dmWrite =  (void(*)())writeDlArc;

}


void parseText(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlText *dlText;
  DlElement *dlElement;


  dlText = (DlText *) malloc(sizeof(DlText));

/* initialize some data in structure */
  dlText->object = defaultObject;
  dlText->textix[0] = '\0';
  dlText->align = HORIZ_LEFT;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object")) {
			parseObject(displayInfo,&(dlText->object));
		} else if (!strcmp(token,"textix")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(dlText->textix,token);
		} else if (!strcmp(token,"align")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"horiz. left")) {
				dlText->align = HORIZ_LEFT;
			} else if (!strcmp(token,"horiz. centered")) {
				dlText->align = HORIZ_CENTER;
			} else if (!strcmp(token,"horiz. right")) {
				dlText->align = HORIZ_RIGHT;
			} else if (!strcmp(token,"vert. top")) {
				dlText->align = VERT_TOP;
			} else if (!strcmp(token,"vert. bottom")) {
				dlText->align = VERT_BOTTOM;
			} else if (!strcmp(token,"vert. centered")) {
				dlText->align = VERT_CENTER;
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
  dlElement->type = DL_Text;
  dlElement->structure.text = dlText;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlText;
  dlElement->dmWrite =  (void(*)())writeDlText;

}



void parseFallingLine(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlFallingLine *dlFallingLine;
  DlElement *dlElement;

  dlFallingLine = (DlFallingLine *) malloc(sizeof(DlFallingLine));
/* initialize some data in structure */
  dlFallingLine->object = defaultObject;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object"))
			parseObject(displayInfo,&(dlFallingLine->object));
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
  dlElement->type = DL_FallingLine;
  dlElement->structure.fallingLine = dlFallingLine;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlFallingLine;
  dlElement->dmWrite =  (void(*)())writeDlFallingLine;

}



void parseRisingLine(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlRisingLine *dlRisingLine;
  DlElement *dlElement;

  dlRisingLine = (DlRisingLine*) malloc(sizeof(DlRisingLine));
/* initialize some data in structure */
  dlRisingLine->object = defaultObject;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object"))
			parseObject(displayInfo,&(dlRisingLine->object));
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
  dlElement->type = DL_RisingLine;
  dlElement->structure.risingLine = dlRisingLine;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlRisingLine;
  dlElement->dmWrite =  (void(*)())writeDlRisingLine;

}





void parseRelatedDisplay(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlRelatedDisplay *dlRelatedDisplay;
  DlElement *dlElement;
  int displayNumber;


  dlRelatedDisplay = (DlRelatedDisplay *) calloc(1,sizeof(DlRelatedDisplay));
/* initialize some data in structure */
  dlRelatedDisplay->object = defaultObject;
  dlRelatedDisplay->clr = 0;
  dlRelatedDisplay->bclr = 1;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object")) {
			parseObject(displayInfo,&(dlRelatedDisplay->object));
		} else if (!strncmp(token,"display",7)) {
/* 
 * compare the first 7 characters to see if a display entry.
 *   if more than one digit is allowed for the display index, then change
 *   the following code to pick up all the digits (can't use atoi() unless
 *   we get a null-terminated string
 */
			displayNumber = MIN(
				token[8] - '0', MAX_RELATED_DISPLAYS-1);
			parseRelatedDisplayEntry(displayInfo,
				&(dlRelatedDisplay->display[displayNumber]) );
		 } else if (!strcmp(token,"clr")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlRelatedDisplay->clr = atoi(token) % DL_MAX_COLORS;
		} else if (!strcmp(token,"bclr")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlRelatedDisplay->bclr = atoi(token) % DL_MAX_COLORS;
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
  dlElement->type = DL_RelatedDisplay;
  dlElement->structure.relatedDisplay = dlRelatedDisplay;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlRelatedDisplay;
  dlElement->dmWrite =  (void(*)())writeDlRelatedDisplay;

}



void parseShellCommand(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlShellCommand *dlShellCommand;
  DlElement *dlElement;
  int cmdNumber;

  dlShellCommand = (DlShellCommand *) calloc(1,sizeof(DlShellCommand));
/* initialize some data in structure */
  dlShellCommand->object = defaultObject;
  dlShellCommand->clr = 0;
  dlShellCommand->bclr = 1;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object")) {
			parseObject(displayInfo,&(dlShellCommand->object));
		} else if (!strncmp(token,"command",7)) {
/* 
 * compare the first 7 characters to see if a command entry.
 *   if more than one digit is allowed for the command index, then change
 *   the following code to pick up all the digits (can't use atoi() unless
 *   we get a null-terminated string
 */
			cmdNumber = MIN(
				token[8] - '0', MAX_SHELL_COMMANDS - 1);
			parseShellCommandEntry(displayInfo,
				&(dlShellCommand->command[cmdNumber]) );
		 } else if (!strcmp(token,"clr")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlShellCommand->clr = atoi(token) % DL_MAX_COLORS;
		} else if (!strcmp(token,"bclr")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			dlShellCommand->bclr = atoi(token) % DL_MAX_COLORS;
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
  dlElement->type = DL_ShellCommand;
  dlElement->structure.shellCommand = dlShellCommand;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlShellCommand;
  dlElement->dmWrite =  (void(*)())writeDlShellCommand;

}










/**********************************************************************
 *********    nested objects (not to be put in display list   *********
 **********************************************************************/



void parseDlColor(displayInfo,filePtr,dlColor)
  DisplayInfo *displayInfo;
  FILE *filePtr;
  DlColormapEntry *dlColor;
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;

  FILE *savedFilePtr;

/*
 * (MDA) have to be sneaky for these colormap parsing routines:
 *	since possibly external colormap, save and restore 
 *	external file ptr in  displayInfo so that getToken()
 *	works with displayInfo and not the filePtr directly
 */
  savedFilePtr = displayInfo->filePtr;
  displayInfo->filePtr = filePtr;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"r")) {
			getToken(displayInfo,token);getToken(displayInfo,token);
			dlColor->r = atoi(token);
		} else if (!strcmp(token,"g")) {
			getToken(displayInfo,token);getToken(displayInfo,token);
			dlColor->g = atoi(token);
		} else if (!strcmp(token,"b")) {
			getToken(displayInfo,token);getToken(displayInfo,token);
			dlColor->b = atoi(token);
		} else if (!strcmp(token,"inten")) {
			getToken(displayInfo,token);getToken(displayInfo,token);
			dlColor->inten = atoi(token);
		}
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
  } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
		&& (tokenType != T_EOF) );

/* and restore displayInfo->filePtr to previous value */
  displayInfo->filePtr = savedFilePtr;
}




void parseObject(displayInfo,object)
  DisplayInfo *displayInfo;
  DlObject *object;
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"x")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			object->x = atoi(token);
		} else if (!strcmp(token,"y")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			object->y = atoi(token);
		} else if (!strcmp(token,"width")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			object->width = atoi(token);
		} else if (!strcmp(token,"height")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			object->height = atoi(token);
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




void parseAttr(displayInfo,attr)
  DisplayInfo *displayInfo;
  DlAttribute *attr;
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"clr")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			attr->clr = atoi(token) % DL_MAX_COLORS;
		} else if (!strcmp(token,"style")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"solid")) {
			    attr->style = SOLID;
			} else if (!strcmp(token,"dash")) {
			    attr->style = DASH;
			}
		} else if (!strcmp(token,"fill")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"solid")) {
			    attr->fill = F_SOLID;
			} else if (!strcmp(token,"outline")) {
			    attr->fill = F_OUTLINE;
			}
		} else if (!strcmp(token,"width")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			attr->width = atoi(token);
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




void parseDynamicAttr(displayInfo,dynAttr)
  DisplayInfo *displayInfo;
  DlDynamicAttributeData *dynAttr;
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"mod")) {
			parseDynAttrMod(displayInfo,&(dynAttr->mod));
		} else if (!strcmp(token,"param")) {
			parseDynAttrParam(displayInfo,&(dynAttr->param));
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



void parseDynAttrMod(displayInfo,dynAttr)
  DisplayInfo *displayInfo;
  DlDynamicAttrMod *dynAttr;
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"clr")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"discrete"))
				dynAttr->clr = DISCRETE;
			else if (!strcmp(token,"static"))
				dynAttr->clr = STATIC;
			else if (!strcmp(token,"alarm"))
				dynAttr->clr = ALARM;
		} else if (!strcmp(token,"vis")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"static"))
				dynAttr->vis = V_STATIC;
			else if (!strcmp(token,"if not zero"))
				dynAttr->vis = IF_NOT_ZERO;
			else if (!strcmp(token,"if zero"))
				dynAttr->vis = IF_ZERO;
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



void parseDynAttrParam(displayInfo,dynAttr)
  DisplayInfo *displayInfo;
  DlDynamicAttrParam *dynAttr;
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
			if (strlen(token) > 0) {
			    strcpy(dynAttr->chan,token);
			}
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




void parseRelatedDisplayEntry(displayInfo,relatedDisplay)
  DisplayInfo *displayInfo;
  DlRelatedDisplayEntry *relatedDisplay;
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"label")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(relatedDisplay->label,token);
		} else if (!strcmp(token,"name")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(relatedDisplay->name,token);
		} else if (!strcmp(token,"args")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(relatedDisplay->args,token);
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




void parseShellCommandEntry(displayInfo,shellCommand)
  DisplayInfo *displayInfo;
  DlShellCommandEntry *shellCommand;
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;

  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"label")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(shellCommand->label,token);
		} else if (!strcmp(token,"name")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(shellCommand->command,token);
		} else if (!strcmp(token,"args")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(shellCommand->args,token);
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




/*********************************************************************
 *********     miscellaneous functions in support of parsing    ******
/*********************************************************************



/*
 * function to open and scan the specified display list file and
 *    extract the colormap information
 */
DlColormap *parseAndExtractExternalColormap(displayInfo,filename)
  DisplayInfo *displayInfo;
  char *filename;
{
  DlColormap *dlColormap;
  FILE *externalFilePtr, *savedFilePtr;
  char token[MAX_TOKEN_LENGTH];
  char msg[512];		/* since common longest filename is 255... */
  TOKEN tokenType;
  int nestingLevel = 0;
  DlObject objectData;
  int n;


  dlColormap = NULL;
  externalFilePtr = dmOpenUseableFile(filename);
  if (externalFilePtr == NULL) {
	sprintf(msg,
	  "Can't open \n\n        \"%s\" (.adl)\n\n%s",filename,
	  "to extract external colormap - check cmap specification");
	dmSetAndPopupWarningDialog(displayInfo,msg,
		(XtCallbackProc)exitCallback,(XtCallbackProc)exitCallback);
	fprintf(stderr,
	"\nparseAndExtractExternalColormap:can't open file %s (.adl)\n",
		filename);
/*
 * awfully hard to get back to where we belong 
 * - maybe try setjmp/longjump later
 */
	dmTerminateCA();
	dmTerminateX();
	exit(-1);

  } else {

    savedFilePtr = displayInfo->filePtr;
    displayInfo->filePtr = externalFilePtr;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"<<color map>>")) {
			dlColormap = 
				parseColormap(displayInfo,externalFilePtr);
		/* don't want to needlessly parse, so we'll return here */
			return (dlColormap);
		}
		break;
	    case T_EQUAL:
		break;
	    case T_LEFT_BRACE:
		nestingLevel++; break;
	    case T_RIGHT_BRACE:
		nestingLevel--; break;
	}
    } while (tokenType != T_EOF);

    fclose(externalFilePtr);

   /* restore old filePtr */
    displayInfo->filePtr = savedFilePtr;
  }


}







/***
 *** the lexical analyzer
 ***/

/*
 * a lexical analyzer (as a state machine), based upon ideas from
 *	"Advanced Unix Programming"  by Marc J. Rochkind, with extensions:
 * understands macros of the form $(xyz), and substitutes the value in
 *	displayInfo's nameValueTable..name with nameValueTable..value
 */
TOKEN getToken(displayInfo,word)	/* get and classify token */
  DisplayInfo *displayInfo;
  char *word;
{
  FILE *filePtr;
  enum {NEUTRAL,INQUOTE,INWORD,INMACRO} state = NEUTRAL, savedState = NEUTRAL;
  int c;
  char *w, *value;
  char *m, macro[MAX_TOKEN_LENGTH];
  int j;

  filePtr = displayInfo->filePtr;

  w = word;
  m = macro;

  while ( (c=getc(filePtr)) != EOF) {
    switch (state) {
	case NEUTRAL:
	   switch(c) {
		case '=' : return (T_EQUAL);
		case '{' : return (T_LEFT_BRACE);
		case '}' : return (T_RIGHT_BRACE);
		case '"' : state = INQUOTE; 
			   break;
		case '$' : c=getc(filePtr);
/* only do macro substitution if in execute mode */
			   if (globalDisplayListTraversalMode == DL_EXECUTE
				&& c == '(' ) {
				state = INMACRO;
			   } else {
				*w++ = '$';
				*w++ = c;
			   }
			   break;
		case ' ' :
		case '\t':
		case '\n': break;

/* for constructs of the form (a,b) */
		case '(' :
		case ',' :
		case ')' : *w++ = c; *w = '\0'; return (T_WORD);

		default  : state = INWORD;
			   *w++ = c;
			   break;
	   }
	   break;
	case INQUOTE:
	   switch(c) {
		case '"' : *w = '\0'; return (T_WORD);
		case '$' : c=getc(filePtr);
/* only do macro substitution if in execute mode */
			   if (globalDisplayListTraversalMode == DL_EXECUTE
				&& c == '(' ) {
				savedState = INQUOTE;
				state = INMACRO;
			   } else {
				*w++ = '$';
				*w++ = c;
			   }
			   break;
		default  : *w++ = c;
			   break;
	   }
	   break;
	case INMACRO:
	   switch(c) {
		case ')' : *m = '\0';
			   value = lookupNameValue(displayInfo->nameValueTable,
				displayInfo->numNameValues,macro);
			   if (value != NULL) {
			      for (j = 0; j < strlen(value); j++) {
				*w++ = value[j];
			      }
			   }
			   state = savedState;
			   m = macro;
			   break;

		default  : *m++ = c;
			   break;
	   }
	   break;
	case INWORD:
	   switch(c) {
		case ' ' :
		case '\n':
		case '\t':
		case '=' :
		case '(' :
		case ',' :
		case ')' :
		case '"' : ungetc(c,filePtr); *w = '\0'; return (T_WORD);
		default  : *w++ = c;
			   break;
	   }
	   break;
    }
  }
  return (T_EOF);
}




