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
 *                              - add version number into the FILE object
 * .03  09-08-95        vong    conform to c++ syntax
 *
 *****************************************************************************
*/

#include "medm.h"

static void relatedDisplayInheritValues(ResourceBundle *pRCB, DlElement *p);
#ifdef __cplusplus
static void freePixmapCallback(Widget w, XtPointer cd, XtPointer)
#else
static void freePixmapCallback(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
  Pixmap pixmap = (Pixmap) cd;
  if (pixmap != (Pixmap)NULL) XmDestroyPixmap(XtScreen(w),pixmap);
}

/*
 * local function to render the related display icon into a pixmap
 */
static void renderRelatedDisplayPixmap(Display *display, Pixmap pixmap,
        Pixel fg, Pixel bg, Dimension width, Dimension height)
{
  typedef struct { float x; float y;} XY;
/* icon is based on the 25 pixel (w & h) bitmap relatedDisplay25 */
  static float rectangleX = 4./25., rectangleY = 4./25.,
        rectangleWidth = 13./25., rectangleHeight = 14./25.;
  static XY segmentData[] = {
        {16./25.,9./25.},
        {22./25.,9./25.},
        {22./25.,22./25.},
        {10./25.,22./25.},
        {10./25.,18./25.},
  };
  GC gc;
  XSegment segments[4];

  gc = XCreateGC(display,pixmap,0,NULL);
  XSetForeground(display,gc,bg);
  XFillRectangle(display,pixmap,gc,0,0,width,height);
  XSetForeground(display,gc,fg);

  segments[0].x1 = (short)(segmentData[0].x*width);
  segments[0].y1 = (short)(segmentData[0].y*height);
  segments[0].x2 = (short)(segmentData[1].x*width);
  segments[0].y2 = (short)(segmentData[1].y*height);

  segments[1].x1 = (short)(segmentData[1].x*width);
  segments[1].y1 = (short)(segmentData[1].y*height);
  segments[1].x2 = (short)(segmentData[2].x*width);
  segments[1].y2 = (short)(segmentData[2].y*height);

  segments[2].x1 = (short)(segmentData[2].x*width);
  segments[2].y1 = (short)(segmentData[2].y*height);
  segments[2].x2 = (short)(segmentData[3].x*width);
  segments[2].y2 = (short)(segmentData[3].y*height);

  segments[3].x1 = (short)(segmentData[3].x*width);
  segments[3].y1 = (short)(segmentData[3].y*height);
  segments[3].x2 = (short)(segmentData[4].x*width);
  segments[3].y2 = (short)(segmentData[4].y*height);

  XDrawSegments(display,pixmap,gc,segments,4);

/* erase any out-of-bounds edges due to roundoff error by blanking out
 *  area of top rectangle */
  XSetForeground(display,gc,bg);
  XFillRectangle(display,pixmap,gc,
        (int)(rectangleX*width),
        (int)(rectangleY*height),
        (unsigned int)(rectangleWidth*width),
        (unsigned int)(rectangleHeight*height));

/* and draw the top rectangle */
  XSetForeground(display,gc,fg);
  XDrawRectangle(display,pixmap,gc,
        (int)(rectangleX*width),
        (int)(rectangleY*height),
        (unsigned int)(rectangleWidth*width),
        (unsigned int)(rectangleHeight*height));

  XFreeGC(display,gc);
}



#ifdef __cplusplus
void executeDlRelatedDisplay(DisplayInfo *displayInfo,
                        DlRelatedDisplay *dlRelatedDisplay, Boolean)
#else
void executeDlRelatedDisplay(DisplayInfo *displayInfo,
                        DlRelatedDisplay *dlRelatedDisplay, Boolean dummy)
#endif
{
  Widget localMenuBar, tearOff;
  Arg args[20];
  int n;
  int i, displayNumber=0;
  char *name, *argsString;
  char **nameArgs;
  XmString xmString;
  Pixmap relatedDisplayPixmap;
  unsigned int pixmapSize;

/*
 * these are widget ids, but they are recorded in the otherChild widget list
 *   as well, for destruction when new displays are selected at the top level
 */
  Widget relatedDisplayPulldownMenu, relatedDisplayMenuButton;
#if 1
  Widget widget;
#endif

/***
 *** from the DlRelatedDisplay structure, we've got specifics
 *** (MDA)  create a pulldown menu with the following related display menu
 ***   entries in it...  --  careful with the XtSetArgs here (special)
 ***/
  n = 0;
  XtSetArg(args[n],XmNbackground,(Pixel)
        displayInfo->dlColormap[dlRelatedDisplay->bclr]); n++;
  XtSetArg(args[n],XmNforeground,(Pixel)
        displayInfo->dlColormap[dlRelatedDisplay->clr]); n++;
  XtSetArg(args[n],XmNhighlightThickness,1); n++;
  XtSetArg(args[n],XmNwidth,dlRelatedDisplay->object.width); n++;
  XtSetArg(args[n],XmNheight,dlRelatedDisplay->object.height); n++;
  XtSetArg(args[n],XmNmarginHeight,0); n++;
  XtSetArg(args[n],XmNmarginWidth,0); n++;
  XtSetArg(args[n],XmNresizeHeight,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNresizeWidth,(Boolean)FALSE); n++;
  XtSetArg(args[n],XmNspacing,0); n++;
  XtSetArg(args[n],XmNx,(Position)dlRelatedDisplay->object.x); n++;
  XtSetArg(args[n],XmNy,(Position)dlRelatedDisplay->object.y); n++;
  XtSetArg(args[n],XmNhighlightOnEnter,TRUE); n++;
  XtSetArg(args[n],XmNtearOffModel,XmTEAR_OFF_DISABLED); n++;
  localMenuBar =
     XmCreateMenuBar(displayInfo->drawingArea,"relatedDisplayMenuBar",args,n);
  XtManageChild(localMenuBar);
  displayInfo->child[displayInfo->childCount++] = localMenuBar;

  colorMenuBar(localMenuBar,
        (Pixel)displayInfo->dlColormap[dlRelatedDisplay->clr],
        (Pixel)displayInfo->dlColormap[dlRelatedDisplay->bclr]);

  relatedDisplayPulldownMenu = XmCreatePulldownMenu(
        displayInfo->child[displayInfo->childCount-1],
                "relatedDisplayPulldownMenu",args,2);
#if 0
  displayInfo->otherChild[displayInfo->otherChildCount++] =
        relatedDisplayPulldownMenu;
#endif

  tearOff = XmGetTearOffControl(relatedDisplayPulldownMenu);
  if (tearOff) {
    XtVaSetValues(tearOff,
        XmNforeground,(Pixel) displayInfo->dlColormap[dlRelatedDisplay->clr],
        XmNbackground,(Pixel) displayInfo->dlColormap[dlRelatedDisplay->bclr],
        XmNtearOffModel,XmTEAR_OFF_DISABLED,
        NULL);
#if 0
    XtSetSensitive(tearOff,False);
#endif
  }

  pixmapSize = MIN(dlRelatedDisplay->object.width,
                        dlRelatedDisplay->object.height);
/* allowing for shadows etc */
  pixmapSize = (unsigned int) MAX(1,(int)pixmapSize - 8);

/* create relatedDisplay icon (render to appropriate size) */
  relatedDisplayPixmap = XCreatePixmap(display,RootWindow(display,screenNum),
        pixmapSize, pixmapSize, XDefaultDepth(display,screenNum));
  renderRelatedDisplayPixmap(display,relatedDisplayPixmap,
        displayInfo->dlColormap[dlRelatedDisplay->clr],
        displayInfo->dlColormap[dlRelatedDisplay->bclr],
        pixmapSize, pixmapSize);

  XtSetArg(args[7],XmNrecomputeSize,(Boolean)False);
  XtSetArg(args[8],XmNlabelPixmap,relatedDisplayPixmap);
  XtSetArg(args[9],XmNlabelType,XmPIXMAP);
  XtSetArg(args[10],XmNsubMenuId,relatedDisplayPulldownMenu);
  XtSetArg(args[11],XmNhighlightOnEnter,TRUE);
  XtSetArg(args[12],XmNalignment,XmALIGNMENT_BEGINNING);

  XtSetArg(args[13],XmNmarginLeft,0);
  XtSetArg(args[14],XmNmarginRight,0);
  XtSetArg(args[15],XmNmarginTop,0);
  XtSetArg(args[16],XmNmarginBottom,0);
  XtSetArg(args[17],XmNmarginWidth,0);
  XtSetArg(args[18],XmNmarginHeight,0);

#if 1
  widget =
#else
  displayInfo->otherChild[displayInfo->otherChildCount++] =
#endif
        XtCreateManagedWidget("relatedDisplayMenuLabel",
                xmCascadeButtonGadgetClass,
                displayInfo->child[displayInfo->childCount-1], args, 19);

/* add destroy callback to free pixmap from pixmap cache */
#if 1
  XtAddCallback(widget,
#else
  XtAddCallback(displayInfo->otherChild[displayInfo->otherChildCount-1],
#endif
        XmNdestroyCallback,freePixmapCallback,
        (XtPointer)relatedDisplayPixmap);

  for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
     if (strlen(dlRelatedDisplay->display[i].name) > (size_t)1) {
        xmString = XmStringCreateSimple(dlRelatedDisplay->display[i].label);
        XtSetArg(args[3], XmNlabelString,xmString);
        name = STRDUP(dlRelatedDisplay->display[i].name);
        argsString = STRDUP(dlRelatedDisplay->display[i].args);
        nameArgs = (char **)malloc(
                RELATED_DISPLAY_FILENAME_AND_ARGS_SIZE*sizeof(char *));
        nameArgs[RELATED_DISPLAY_FILENAME_INDEX] = name;
        nameArgs[RELATED_DISPLAY_ARGS_INDEX] = argsString;
        XtSetArg(args[4], XmNuserData, nameArgs);
        relatedDisplayMenuButton = XtCreateManagedWidget("relatedButton",
                xmPushButtonWidgetClass, relatedDisplayPulldownMenu, args, 5);
#if 0
        displayInfo->otherChild[displayInfo->otherChildCount++] =
                relatedDisplayMenuButton;
#endif
        XtAddCallback(relatedDisplayMenuButton,XmNactivateCallback,
                (XtCallbackProc)dmCreateRelatedDisplay,(XtPointer)displayInfo);
        XtAddCallback(relatedDisplayMenuButton,XmNdestroyCallback,
                (XtCallbackProc)relatedDisplayMenuButtonDestroy,
                (XtPointer)nameArgs);
        XmStringFree(xmString);
     }
  }


/* add event handlers to relatedDisplay... */
  if (displayInfo->traversalMode == DL_EDIT) {

/* remove all translations if in edit mode */
    XtUninstallTranslations(localMenuBar);

    XtAddEventHandler(localMenuBar,ButtonPressMask,False,
                handleButtonPress, (XtPointer)displayInfo);
  }

}

#ifdef __cplusplus
static void createDlRelatedDisplayEntry(
  DisplayInfo *,
  DlRelatedDisplayEntry *relatedDisplay,
  int displayNumber)
#else
static void createDlRelatedDisplayEntry(
  DisplayInfo *displayInfo,
  DlRelatedDisplayEntry *relatedDisplay,
  int displayNumber)
#endif
{
/* structure copy */
  *relatedDisplay = globalResourceBundle.rdData[displayNumber];
}

DlElement *createDlRelatedDisplay(
  DisplayInfo *displayInfo)
{
  DlRelatedDisplay *dlRelatedDisplay;
  DlElement *dlElement;
  int displayNumber;

  dlRelatedDisplay = (DlRelatedDisplay *) malloc(sizeof(DlRelatedDisplay));
  if (!dlRelatedDisplay) return 0;
  objectAttributeInit(&(dlRelatedDisplay->object));
  for (displayNumber = 0; displayNumber < MAX_RELATED_DISPLAYS;displayNumber++)
	createDlRelatedDisplayEntry(displayInfo,
			&(dlRelatedDisplay->display[displayNumber]),
			displayNumber );
  dlRelatedDisplay->clr = globalResourceBundle.clr;
  dlRelatedDisplay->bclr = globalResourceBundle.bclr;

  if (!(dlElement = createDlElement(DL_RelatedDisplay,
                    (XtPointer)      dlRelatedDisplay,
                    (medmExecProc)   executeDlRelatedDisplay,
                    (medmWriteProc)  writeDlRelatedDisplay,
										0,0,
                    relatedDisplayInheritValues))) {
    free(dlRelatedDisplay);
  }

  return(dlElement);
}

void parseRelatedDisplayEntry(DisplayInfo *displayInfo, DlRelatedDisplayEntry *relatedDisplay)
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

DlElement *parseRelatedDisplay(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlRelatedDisplay *dlRelatedDisplay = 0;
  DlElement *dlElement = createDlRelatedDisplay(displayInfo);
  int displayNumber;

  if (!dlElement) return 0;
  dlRelatedDisplay = dlElement->structure.relatedDisplay;

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

  POSITION_ELEMENT_ON_LIST();

  return dlElement;

}

void writeDlRelatedDisplayEntry(
  FILE *stream,
  DlRelatedDisplayEntry *entry,
  int index,
  int level)
{
  char indent[16];

  memset(indent,'\t',level);
  indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
  if (MedmUseNewFileFormat) {
#endif
		fprintf(stream,"\n%sdisplay[%d] {",indent,index);
		if (entry->label[0] != '\0')
			fprintf(stream,"\n%s\tlabel=\"%s\"",indent,entry->label);
		if (entry->name[0] != '\0')
			fprintf(stream,"\n%s\tname=\"%s\"",indent,entry->name);
		if (entry->args[0] != '\0')
			fprintf(stream,"\n%s\targs=\"%s\"",indent,entry->args);
		fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
  } else {
		fprintf(stream,"\n%sdisplay[%d] {",indent,index);
		fprintf(stream,"\n%s\tlabel=\"%s\"",indent,entry->label);
		fprintf(stream,"\n%s\tname=\"%s\"",indent,entry->name);
		fprintf(stream,"\n%s\targs=\"%s\"",indent,entry->args);
		fprintf(stream,"\n%s}",indent);
	}
#endif
}

void writeDlRelatedDisplay(
  FILE *stream,
  DlRelatedDisplay *dlRelatedDisplay,
  int level)
{
  int i;
  char indent[16];

  memset(indent,'\t',level);
  indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
  if (MedmUseNewFileFormat) {
#endif
		fprintf(stream,"\n%s\"related display\" {",indent);
		writeDlObject(stream,&(dlRelatedDisplay->object),level+1);
		for (i = 0; i < MAX_RELATED_DISPLAYS; i++) {
		if ((dlRelatedDisplay->display[i].label[0] != '\0') ||
			(dlRelatedDisplay->display[i].name[0] != '\0') ||
			(dlRelatedDisplay->display[i].args[0] != '\0'))
			writeDlRelatedDisplayEntry(stream,&(dlRelatedDisplay->display[i]),i,level+1);
		}
		fprintf(stream,"\n%s\tclr=%d",indent,dlRelatedDisplay->clr);
		fprintf(stream,"\n%s\tbclr=%d",indent,dlRelatedDisplay->bclr);
		fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
	} else {
		fprintf(stream,"\n%s\"related display\" {",indent);
		writeDlObject(stream,&(dlRelatedDisplay->object),level+1);
		for (i = 0; i < MAX_RELATED_DISPLAYS; i++)
			writeDlRelatedDisplayEntry(stream,&(dlRelatedDisplay->display[i]),i,level+1);
		fprintf(stream,"\n%s\tclr=%d",indent,dlRelatedDisplay->clr);
		fprintf(stream,"\n%s\tbclr=%d",indent,dlRelatedDisplay->bclr);
		fprintf(stream,"\n%s}",indent);
	}
#endif
}

#ifdef __cplusplus
void relatedDisplayMenuButtonDestroy(Widget, XtPointer cd, XtPointer)
#else
void relatedDisplayMenuButtonDestroy(Widget w, XtPointer cd, XtPointer cbs)
#endif
{
  char *data = (char *) cd;
  char **freeData;
  /* free up the memory associated with the menu button  */
     if (data != NULL) {
  /* since this is a  "char * nameArgs[]" thing... */
  freeData = (char **)data;
  free( (char *)freeData[RELATED_DISPLAY_FILENAME_INDEX]);
  free( (char *)freeData[RELATED_DISPLAY_ARGS_INDEX]);
  free( (char *) data );
  data = NULL;
    }
}

#ifdef __cplusplus
void dmCreateRelatedDisplay(Widget  w, XtPointer cd, XtPointer)
#else
void dmCreateRelatedDisplay(Widget  w, XtPointer cd, XtPointer cbs)
#endif
{
  DisplayInfo *displayInfo = (DisplayInfo *) cd;
  char *filename, *argsString, *newFilename, token[MAX_TOKEN_LENGTH];
  char **nameArgs;
  Arg args[5];
  FILE *filePtr;
  int suffixLength, prefixLength;
  char *adlPtr;
  char processedArgs[2*MAX_TOKEN_LENGTH], name[2*MAX_TOKEN_LENGTH];

  XtSetArg(args[0],XmNuserData,&nameArgs);
  XtGetValues(w,args,1);

  filename = nameArgs[RELATED_DISPLAY_FILENAME_INDEX];
  argsString = nameArgs[RELATED_DISPLAY_ARGS_INDEX];
/*
 * if we want to be able to have RD's inherit their parent's
 *   macro-substitutions, then we must perform any macro substitution on
 *   this argument string in this displayInfo's context before passing
 *   it to the created child display
 */
 if (globalDisplayListTraversalMode == DL_EXECUTE) {

  performMacroSubstitutions(displayInfo,argsString,processedArgs,
          2*MAX_TOKEN_LENGTH);
  filePtr = dmOpenUseableFile(filename);
  if (filePtr == NULL) {
    newFilename = STRDUP(filename);
    adlPtr = strstr(filename,DISPLAY_FILE_ASCII_SUFFIX);
    if (adlPtr != NULL) {
      /* ascii name */
       suffixLength = strlen(DISPLAY_FILE_ASCII_SUFFIX);
    } else {
       /* binary name */
       suffixLength = strlen(DISPLAY_FILE_BINARY_SUFFIX);
    }
    prefixLength = strlen(newFilename) - suffixLength;
    newFilename[prefixLength] = '\0';
    sprintf(token,
         "Can't open related display:\n\n        %s%s\n\n%s",
          newFilename, DISPLAY_FILE_ASCII_SUFFIX,
          "--> check EPICS_DISPLAY_PATH ");
    dmSetAndPopupWarningDialog(displayInfo,token,"Ok",NULL,NULL);
    fprintf(stderr,"\n%s",token);
    free(newFilename);
  } else {
    dmDisplayListParse(filePtr,processedArgs,filename,NULL,(Boolean)True);
    fclose(filePtr);
  }

 }
}

static void relatedDisplayInheritValues(ResourceBundle *pRCB, DlElement *p) {
  DlRelatedDisplay *dlRelatedDisplay = p->structure.relatedDisplay;
  medmGetValues(pRCB,
    CLR_RC,        &(dlRelatedDisplay->clr),
    BCLR_RC,       &(dlRelatedDisplay->bclr),
    -1);
}
