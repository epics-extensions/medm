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
#include <ctype.h>

#include <X11/keysym.h>
#include <Xm/MwmUtil.h>

void parseAttr(DisplayInfo *displayInfo, DlBasicAttribute *attr);
void parseDynamicAttr(DisplayInfo *displayInfo, DlDynamicAttribute *dynAttr);
void parseDynAttrMod(DisplayInfo *displayInfo, DlDynamicAttribute *dynAttr);
void parseDynAttrParam(DisplayInfo *displayInfo, DlDynamicAttribute *dynAttr);
void parseOldDlColor( DisplayInfo *, FILE *, DlColormapEntry *);

static DlList *dlElementFreeList = 0;

int initMedmCommon() {
    if (dlElementFreeList) return 0;
    if (dlElementFreeList = createDlList()) {
	return 0;
    } else {
	return -1;
    }
}

DlFile *createDlFile(
  DisplayInfo *displayInfo)
{
    DlFile *dlFile;

    dlFile = (DlFile *) malloc(sizeof(DlFile));
    if (!dlFile) return 0;
    strcpy(dlFile->name,"newDisplay.adl");
    dlFile->versionNumber =
      MEDM_VERSION * 10000 + MEDM_REVISION * 100 + MEDM_UPDATE_LEVEL;
    return(dlFile);
}

DlFile *parseFile(
  DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlFile *dlFile = createDlFile(displayInfo);;

    if (!dlFile) return 0;
    dlFile->versionNumber = 0;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"name")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		strcpy(dlFile->name,token);
	    }
	    if (!strcmp(token,"version")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		dlFile->versionNumber = atoi(token);
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

    return dlFile;
}

void writeDlFile(
  FILE *stream,
  DlFile *dlFile,
  int level)
{
    int i;
    char indent[16];
    int versionNumber = MEDM_VERSION * 10000 + MEDM_REVISION * 100 + MEDM_UPDATE_LEVEL;

    for (i = 0; i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%sfile {",indent);
    fprintf(stream,"\n%s\tname=\"%s\"",indent,dlFile->name);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
	fprintf(stream,"\n%s\tversion=%06d",indent,versionNumber);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	fprintf(stream,"\n%s\tversion=%06d",indent,20199);
    }
#endif
    fprintf(stream,"\n%s}",indent);
}

/*
 * this function gets called for executing the colormap section of
 *  each display list, for new display creation and for edit <-> execute
 *  transitions.  we could be more clever here for the edit/execute
 *  transitions and not re-execute the colormap info, but that would
 *  require changes to the cleanup code, etc...  hence let us leave
 *  this as is (since the colors are properly being freed (ref-count
 *  being decremented) and performance seems fine...
 *  -- for edit <-> execute type running, performance is not a big issue
 *  anyway and there is no additional cost incurred for the straight
 *  execute time running --
 */
void executeDlColormap(DisplayInfo *displayInfo, DlColormap *dlColormap)
{
    Arg args[10];
    int i, n;
    Dimension width, height;
    XtGCMask valueMask;
    XGCValues values;

    if (!displayInfo) return;
  /* already have a colormap - don't allow a second one! */
    if (displayInfo->colormap) return;

    displayInfo->colormap = (Pixel *) malloc(dlColormap->ncolors *
      sizeof(Pixel));
    displayInfo->dlColormapSize = dlColormap->ncolors;

  /**** allocate the X colormap from dlColormap data ****/
    for (i = 0; i < dlColormap->ncolors; i++) {
	XColor color;
      /* scale [0,255] to [0,65535] */
	color.red   = (unsigned short) COLOR_SCALE*(dlColormap->dl_color[i].r); 
	color.green = (unsigned short) COLOR_SCALE*(dlColormap->dl_color[i].g); 
	color.blue  = (unsigned short) COLOR_SCALE*(dlColormap->dl_color[i].b); 
      /* allocate a shareable color cell with closest RGB value */
	if (XAllocColor(display,cmap,&color)) {
	    displayInfo->colormap[displayInfo->dlColormapCounter] = color.pixel;
	} else {
	    fprintf(stderr,"\nexecuteDlColormap: couldn't allocate requested color");
	    displayInfo->colormap[displayInfo->dlColormapCounter] = unphysicalPixel;
	}

	if (displayInfo->dlColormapCounter < displayInfo->dlColormapSize) 
	  displayInfo->dlColormapCounter++;
	else
	  fprintf(stderr,"\nexecuteDlColormap:  too many colormap entries");
      /* just keep rewriting that last colormap entry */
    }

  /*
   * set the foreground and background of the display 
   */
    XtVaSetValues(displayInfo->drawingArea,
      XmNbackground,
      displayInfo->colormap[displayInfo->drawingAreaBackgroundColor],
      NULL);

  /* and create the drawing area pixmap */
    XtVaGetValues(displayInfo->drawingArea,
      XmNwidth,(Dimension *)&width,
      XmNheight,(Dimension *)&height,
      NULL);
    if (displayInfo->drawingAreaPixmap) {
	XFreePixmap(display,displayInfo->drawingAreaPixmap);
    }
    displayInfo->drawingAreaPixmap =
      XCreatePixmap(display, RootWindow(display,screenNum),
	MAX(1,width),MAX(1,height),
	DefaultDepth(display,screenNum));

  /* create the pixmap GC */
    valueMask = GCForeground | GCBackground ;
    values.foreground = 
      displayInfo->colormap[displayInfo->drawingAreaBackgroundColor];
    values.background =
      displayInfo->colormap[displayInfo->drawingAreaBackgroundColor];
    if (displayInfo->pixmapGC) {
	XFreeGC(display,displayInfo->pixmapGC);
    }
    displayInfo->pixmapGC = XCreateGC(display,
      XtWindow(displayInfo->drawingArea),valueMask,&values); 
  /* (MDA) don't generate GraphicsExpose events on XCopyArea() */
    XSetGraphicsExposures(display,displayInfo->pixmapGC,FALSE);

    XFillRectangle(display,displayInfo->drawingAreaPixmap,
      displayInfo->pixmapGC,0,0,width,height);
    XSetForeground(display,displayInfo->pixmapGC,
      displayInfo->colormap[displayInfo->drawingAreaForegroundColor]);

/* create the initial display GC */
    valueMask = GCForeground | GCBackground ;
    values.foreground = 
      displayInfo->colormap[displayInfo->drawingAreaForegroundColor];
    values.background = 
      displayInfo->colormap[displayInfo->drawingAreaBackgroundColor];
    if (displayInfo->gc) {
	XFreeGC(display,displayInfo->gc);
    }
    displayInfo->gc = XCreateGC(display,XtWindow(displayInfo->drawingArea),
      valueMask,&values);
}


DlColormap *createDlColormap(
  DisplayInfo *displayInfo)
{
    DlColormap *dlColormap;

    dlColormap = (DlColormap *) malloc(sizeof(DlColormap));
    if (!dlColormap) return 0;
  /* structure copy */
    *dlColormap = defaultDlColormap;

    return(dlColormap);
}


void parseDlColor(
  DisplayInfo *displayInfo,
  FILE *filePtr,
  DlColormapEntry *dlColor)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    int counter = 0;

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
	case T_WORD: {
	    char *tmp;
	    unsigned long color = strtoul(token,&tmp,16);
	    if (counter < DL_MAX_COLORS) {
		dlColor[counter].r = (color & 0x00ff0000) >> 16;
		dlColor[counter].g = (color & 0x0000ff00) >> 8;
		dlColor[counter].b = color & 0x000000ff;
		counter++;
	    }
	    getToken(displayInfo,token);
	    break;
	}
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

void parseOldDlColor(
  DisplayInfo *displayInfo,
  FILE *filePtr,
  DlColormapEntry *dlColor)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
 
    FILE *savedFilePtr;
 
/*
 * (MDA) have to be sneaky for these colormap parsing routines:
 *      since possibly external colormap, save and restore
 *      external file ptr in  displayInfo so that getToken()
 *      works with displayInfo and not the filePtr directly
 */
    savedFilePtr = displayInfo->filePtr;
    displayInfo->filePtr = filePtr;
 
    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"r")) {
		getToken(displayInfo,token);getToken(displayInfo,token);
		dlColor->r = atoi(token);
	    } else
	      if (!strcmp(token,"g")) {
		  getToken(displayInfo,token);getToken(displayInfo,token);
		  dlColor->g = atoi(token);
	      } else
		if (!strcmp(token,"b")) {
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

/* parseColormap and parseDlColor have two arguments, since could in fact
 *   be parsing and external colormap file, hence need to pass in the 
 *   explicit file ptr for the current colormap file
 */
DlColormap *parseColormap(
  DisplayInfo *displayInfo,
  FILE *filePtr)
{
    char token[MAX_TOKEN_LENGTH];
    char msg[2*MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlColormap *dlColormap = createDlColormap(displayInfo);
    DlColormapEntry dummyColormapEntry;
    DlElement *dlTarget;
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
		    dmSetAndPopupWarningDialog(displayInfo, msg,"Ok",NULL,NULL);
		}
	    } else if (!strcmp(token,"dl_color")) {
	      /* continue parsing but throw away "excess" colormap entries */
		if (counter < DL_MAX_COLORS) {
		    parseOldDlColor(displayInfo,filePtr,&(dlColormap->dl_color[counter]));
		    counter++;
		} else {
		    parseOldDlColor(displayInfo,filePtr,&dummyColormapEntry);
		    counter++;
		}
	    } else if (!strcmp(token,"colors")) {
		parseDlColor(displayInfo,filePtr,dlColormap->dl_color);
	    }
	    break;
	case T_EQUAL:
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
	}
    } while ((tokenType != T_RIGHT_BRACE) && (nestingLevel > 0) &&
      (tokenType != T_EOF));

  /*
   *  now since a valid colormap element has been brought into display list,
   *  remove the external cmap reference in the dlDisplay element
   */
    if (dlTarget = FirstDlElement(displayInfo->dlElementList)) {
	dlTarget->structure.display->cmap[0] = '\0';
    }
  /* restore the previous filePtr */
    displayInfo->filePtr = savedFilePtr;
    return (dlColormap);
}


void writeDlColormap(
  FILE *stream,
  DlColormap *dlColormap,
  int level)
{
    int i;
    char indent[16];

    for (i = 0; i < level; i++) indent[i] = '\t';
    indent[i] = '\0';

    fprintf(stream,"\n%s\"color map\" {",indent);
    fprintf(stream,"\n%s\tncolors=%d",indent,dlColormap->ncolors);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
  	fprintf(stream,"\n%s\tcolors {",indent,dlColormap->ncolors);

	for (i = 0; i < dlColormap->ncolors; i++) {
	    fprintf(stream,"\n\t\t%s%06x,",indent,
              dlColormap->dl_color[i].r*0x10000+
              dlColormap->dl_color[i].g*0x100 +
              dlColormap->dl_color[i].b);
  	}
	fprintf(stream,"\n\t%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
	for (i = 0; i < dlColormap->ncolors; i++) {
	    fprintf(stream,"\n%s\tdl_color {",indent);
	    fprintf(stream,"\n%s\t\tr=%d",indent,dlColormap->dl_color[i].r);
	    fprintf(stream,"\n%s\t\tg=%d",indent,dlColormap->dl_color[i].g);
	    fprintf(stream,"\n%s\t\tb=%d",indent,dlColormap->dl_color[i].b);
	    fprintf(stream,"\n%s\t\tinten=%d",indent,dlColormap->dl_color[i].inten);
	    fprintf(stream,"\n%s\t}",indent);
	}
    }
#endif
    fprintf(stream,"\n%s}",indent);
}

void executeDlBasicAttribute(DisplayInfo *displayInfo,
  DlBasicAttribute *attr)
{
    unsigned long gcValueMask;
    XGCValues gcValues;

    gcValueMask = GCForeground | GCBackground | GCLineStyle | GCLineWidth |
      GCCapStyle | GCJoinStyle | GCFillStyle;
    gcValues.foreground = displayInfo->colormap[attr->clr];
    gcValues.background = displayInfo->colormap[attr->clr];

    switch (attr->style) {
    case SOLID : gcValues.line_style = LineSolid; break;
    case DASH  : gcValues.line_style = LineOnOffDash; break;
    default    : gcValues.line_style = LineSolid; break;
    }

    gcValues.line_width = attr->width;
    gcValues.cap_style = CapButt;
    gcValues.join_style = JoinRound;
    gcValues.fill_style = FillSolid;

    XChangeGC(display,displayInfo->gc,gcValueMask,&gcValues);
}

void parseBasicAttribute(DisplayInfo *displayInfo,
  DlBasicAttribute *attr) {
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

void parseOldBasicAttribute(DisplayInfo *displayInfo,
  DlBasicAttribute *attr)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;

    attr->clr = 0;
    attr->style = SOLID;
    attr->fill = F_SOLID;
    attr->width = 0;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"attr"))
	      parseAttr(displayInfo,attr);
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
	}
    } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );

}

void writeDlBasicAttribute(FILE *stream, DlBasicAttribute *attr, int level)
{
    char indent[16];

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
  	fprintf(stream,"\n%s\"basic attribute\" {",indent);
  	fprintf(stream,"\n%s\tclr=%d",indent,attr->clr);
  	if (attr->style != SOLID)
	  fprintf(stream,"\n%s\tstyle=\"%s\"",indent,stringValueTable[attr->style]);
  	if (attr->fill != F_SOLID)
	  fprintf(stream,"\n%s\tfill=\"%s\"",indent,stringValueTable[attr->fill]);
  	if (attr->width != 0)
	  fprintf(stream,"\n%s\twidth=%d",indent,attr->width);
  	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
  	fprintf(stream,"\n%s\"basic attribute\" {",indent);
  	fprintf(stream,"\n%s\tattr {",indent);
  	fprintf(stream,"\n%s\t\tclr=%d",indent,attr->clr);
	fprintf(stream,"\n%s\t\tstyle=\"%s\"",indent,stringValueTable[attr->style]);
	fprintf(stream,"\n%s\t\tfill=\"%s\"",indent,stringValueTable[attr->fill]);
	fprintf(stream,"\n%s\t\twidth=%d",indent,attr->width);
  	fprintf(stream,"\n%s\t}",indent);
  	fprintf(stream,"\n%s}",indent);
    }
#endif
}

#ifdef __cplusplus
void createDlObject(
  DisplayInfo *,
  DlObject *object)
#else
void createDlObject(
  DisplayInfo *displayInfo,
  DlObject *object)
#endif
{
    object->x = globalResourceBundle.x;
    object->y = globalResourceBundle.y;
    object->width = globalResourceBundle.width;
    object->height = globalResourceBundle.height;
}

void objectAttributeInit(DlObject *object) {
    object->x = 0;
    object->y = 0;
    object->width = 10;
    object->height = 10;
}

void objectAttributeSet(DlObject *object, int x, int y, unsigned int width,
  unsigned int height) {
    object->x = x;
    object->y = y;
    object->width = width;
    object->height = height;
}

void basicAttributeInit(DlBasicAttribute *attr) {
    attr->clr = 0;
    attr->style = SOLID;
    attr->fill = F_SOLID;
    attr->width = 0;
}

void dynamicAttributeInit(DlDynamicAttribute *dynAttr) {
    dynAttr->clr = STATIC;
    dynAttr->vis = V_STATIC;
#ifdef __COLOR_RULE_H__
    dynAttr->colorRule = 0;
#endif
    dynAttr->name = 0;
}

void destroyDlElement(DlElement *);
DlElement* createDlElement(DlElementType, XtPointer, DlDispatchTable *);
void writeDlElement(FILE *stream, DlElement *DlElement, int level);
void executeDlElement(DisplayInfo *displayInfo, DlElement *dlElement);

void writeDlElement(FILE *stream, DlElement *DlElement, int level)
{
    return;
}

void executeDlElement(DisplayInfo *displayInfo, DlElement *dlElement) {
    return;
}

static DlDispatchTable elementDlDispatchTable = {
    createDlElement,
    destroyDlElement,
    executeDlElement,
    writeDlElement,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL};

DlElement* createDlElement(
  DlElementType type,
  XtPointer structure,
  DlDispatchTable *dlDispatchTable)
{
    DlElement *dlElement;
    if (dlElementFreeList->count > 0) {
	dlElement = dlElementFreeList->tail;
	removeDlElement(dlElementFreeList,dlElement);
#if 0
	printf("createDlElement, %d\n",dlElementFreeList->count);
#endif
    } else {
	dlElement = (DlElement *) malloc(sizeof(DlElement));
    }
    if (!dlElement) return 0;
    dlElement->type = type;
    dlElement->structure.composite = (DlComposite *) structure;
    if (dlDispatchTable) {
	dlElement->run = dlDispatchTable;
    } else {
#if 0
	printf("createDlElement\n");
#endif
	dlElement->run = &elementDlDispatchTable;
    }
    dlElement->widget = 0;
    dlElement->data = 0;
    dlElement->next = 0;
    dlElement->prev = 0;
    return dlElement;
}

void destroyDlElement(DlElement *dlElement) {
    appendDlElement(dlElementFreeList,dlElement);
#if 0
    printf("destroyDlElement, %d\n",dlElementFreeList->count);
#endif
}
  
static DlObject defaultObject = {0,0,5,5};


void parseDynamicAttribute(DisplayInfo *displayInfo,
  DlDynamicAttribute *dynAttr) {
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"clr")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
#ifdef __COLOR_RULE_H__
		if (!strcmp(token,"discrete") ||
		  !strcmp(token,"color rule"))
#else
		  if (!strcmp(token,"discrete"))
#endif
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
#ifdef __COLOR_RULE_H__
	    } else if (!strcmp(token,"colorRule")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"set#1"))
		  dynAttr->colorRule = 0;
		else if (!strcmp(token,"set#2"))
		  dynAttr->colorRule = 1;
		else if (!strcmp(token,"set#3"))
		  dynAttr->colorRule = 2;
		else if (!strcmp(token,"set#4"))
		  dynAttr->colorRule = 3;
#endif
	    } else if (!strcmp(token,"chan")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if ((strlen(token) > (size_t) 0)) {
		    if (!dynAttr->name) dynAttr->name = allocateString();
		    if (dynAttr->name)
		      strcpy(dynAttr->name,token);
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

void parseOldDynamicAttribute(DisplayInfo *displayInfo,
  DlDynamicAttribute *dynAttr)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;


    dynAttr->clr = STATIC;	/* ColorMode, actually */
#ifdef __COLOR_RULE_H__
    dynAttr->colorRule = 0;   /* Color Rule # */
#endif
    dynAttr->vis = V_STATIC;
    if (dynAttr->name) dynAttr->name[0] = '\0';

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"attr"))
	      parseDynamicAttr(displayInfo,dynAttr);
	    break;
	case T_LEFT_BRACE:
	    nestingLevel++; break;
	case T_RIGHT_BRACE:
	    nestingLevel--; break;
	}
    } while ( (tokenType != T_RIGHT_BRACE) && (nestingLevel > 0)
      && (tokenType != T_EOF) );
}


DlElement *parseFallingLine(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlPolyline *dlPolyline;
    DlElement *dlElement;

  /* Rising Line is replaced by PolyLine */
    dlPolyline = (DlPolyline *) calloc(1,sizeof(DlPolyline));
  /* initialize some data in structure */
    dlPolyline->object = defaultObject;
    dlPolyline->nPoints = 0;
    dlPolyline->points = (XPoint *)NULL;
    dlPolyline->isFallingOrRisingLine = True;

    do {
        switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlPolyline->object));
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

#define INITIAL_NUM_POINTS 16
    dlPolyline->points = (XPoint *)malloc(16*sizeof(XPoint));
    dlPolyline->nPoints = 2;
    dlPolyline->points[0].x = dlPolyline->object.x;
    dlPolyline->points[0].y = dlPolyline->object.y;
    dlPolyline->points[1].x = dlPolyline->object.x + dlPolyline->object.width; 
    dlPolyline->points[1].y = dlPolyline->object.y + dlPolyline->object.height;

    dlElement = (DlElement *) malloc(sizeof(DlElement));
    dlElement->type = DL_Polyline;
    dlElement->structure.polyline = dlPolyline;
    dlElement->next = NULL;

    return dlElement;
}

DlElement *parseRisingLine(DisplayInfo *displayInfo)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;
    DlPolyline *dlPolyline;
    DlElement *dlElement;

  /* Rising Line is replaced by PolyLine */
    dlPolyline = (DlPolyline *) calloc(1,sizeof(DlPolyline));
  /* initialize some data in structure */
    dlPolyline->object = defaultObject;
    dlPolyline->nPoints = 0;
    dlPolyline->points = (XPoint *)NULL;
    dlPolyline->isFallingOrRisingLine = True;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"object"))
	      parseObject(displayInfo,&(dlPolyline->object));
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

#define INITIAL_NUM_POINTS 16 
    dlPolyline->points = (XPoint *)malloc(16*sizeof(XPoint)); 
    dlPolyline->nPoints = 2;
    dlPolyline->points[0].x = dlPolyline->object.x;
    dlPolyline->points[0].y = dlPolyline->object.y + dlPolyline->object.height;
    dlPolyline->points[1].x = dlPolyline->object.x + dlPolyline->object.width;
    dlPolyline->points[1].y = dlPolyline->object.y;
  
    dlElement = (DlElement *) malloc(sizeof(DlElement));
    dlElement->type = DL_Polyline;
    dlElement->structure.polyline = dlPolyline;
    dlElement->next = NULL;

    return dlElement;
}

/**********************************************************************
 *********    nested objects (not to be put in display list   *********
 **********************************************************************/

void parseObject(DisplayInfo *displayInfo, DlObject *object)
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


void parseAttr(DisplayInfo *displayInfo, DlBasicAttribute *attr)
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
      && (tokenType != T_EOF) ) ;     /* Do nothing */
}

void parseDynamicAttr(DisplayInfo *displayInfo, DlDynamicAttribute *dynAttr)
{
    char token[MAX_TOKEN_LENGTH];
    TOKEN tokenType;
    int nestingLevel = 0;

    do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	case T_WORD:
	    if (!strcmp(token,"mod")) {
		parseDynAttrMod(displayInfo,dynAttr);
	    } else if (!strcmp(token,"param")) {
		parseDynAttrParam(displayInfo,dynAttr);
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



void parseDynAttrMod(DisplayInfo *displayInfo, DlDynamicAttribute *dynAttr)
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
#ifdef __COLOR_RULE_H__
		if (!strcmp(token,"discrete") || !strcmp(token,"color rule"))
#else
		if (!strcmp(token,"discrete"))
#endif
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
#ifdef __COLOR_RULE_H__
	    } else if (!strcmp(token,"colorRule")) {
		getToken(displayInfo,token);
		getToken(displayInfo,token);
		if (!strcmp(token,"set#1"))
		  dynAttr->colorRule = 0;
		else if (!strcmp(token,"set#2"))
		  dynAttr->colorRule = 1;
		else if (!strcmp(token,"set#3"))
		  dynAttr->colorRule = 2;
		else if (!strcmp(token,"set#4"))
		  dynAttr->colorRule = 3;
#endif
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



void parseDynAttrParam(DisplayInfo *displayInfo, DlDynamicAttribute *dynAttr)
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
		if (token[0]) {
		    if (!dynAttr->name)
		      dynAttr->name = allocateString();
		    if (dynAttr->name)
		      strcpy(dynAttr->name,token);
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


DlColormap *parseAndExtractExternalColormap(DisplayInfo *displayInfo, char *filename)
{
    DlColormap *dlColormap;
    FILE *externalFilePtr, *savedFilePtr;
    char token[MAX_TOKEN_LENGTH];
    char msg[512];		/* since common longest filename is 255... */
    TOKEN tokenType;
    int nestingLevel = 0;


    dlColormap = NULL;
    externalFilePtr = dmOpenUseableFile(filename);
    if (externalFilePtr == NULL) {
	sprintf(msg,
	  "Can't open \n\n        \"%s\" (.adl)\n\n%s",filename,
	  "to extract external colormap - check cmap specification");
	dmSetAndPopupWarningDialog(displayInfo,msg,"Ok",NULL,NULL);
	fprintf(stderr,
	  "\nparseAndExtractExternalColormap:can't open file %s (.adl)\n",
	  filename);
/*
 * awfully hard to get back to where we belong 
 * - maybe try setjmp/longjump later
 */
	medmCATerminate();
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
TOKEN getToken(	/* get and classify token */
  DisplayInfo *displayInfo,
  char *word)
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
		    for (j = 0; j < (int) strlen(value); j++) {
			*w++ = value[j];
		    }
		} else {
		    *w++ = '$';
		    *w++ = '(';
		    for (j = 0; j < (int) strlen(macro); j++) {
			*w++ = macro[j];
		    }
		    *w++ = ')';
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


void writeDlDynamicAttribute(FILE *stream, DlDynamicAttribute *dynAttr,
  int level)
{
    char indent[16];

    if (!dynAttr->name) return;

    memset(indent,'\t',level);
    indent[level] = '\0';

#ifdef SUPPORT_0201XX_FILE_FORMAT
    if (MedmUseNewFileFormat) {
#endif
  	fprintf(stream,"\n%s\"dynamic attribute\" {",indent);
  	if (dynAttr->clr != STATIC) 
	  fprintf(stream,"\n%s\tclr=\"%s\"",indent,stringValueTable[dynAttr->clr]);
  	if (dynAttr->vis != V_STATIC)
	  fprintf(stream,"\n%s\tvis=\"%s\"",indent,stringValueTable[dynAttr->vis]);
#ifdef __COLOR_RULE_H__
  	if (dynAttr->colorRule != 0)
	  fprintf(stream,"\n%s\tcolorRule=\"set#%d\"",indent,dynAttr->colorRule+1);
#endif
  	fprintf(stream,"\n%s\tchan=\"%s\"",indent,dynAttr->name);
  	fprintf(stream,"\n%s}",indent);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    } else {
  	fprintf(stream,"\n%s\"dynamic attribute\" {",indent);
  	fprintf(stream,"\n%s\tattr {",indent);
  	fprintf(stream,"\n%s\t\tmod {",indent);
	fprintf(stream,"\n%s\t\t\tclr=\"%s\"",indent,stringValueTable[dynAttr->clr]);
   	fprintf(stream,"\n%s\t\t\tvis=\"%s\"",indent,stringValueTable[dynAttr->vis]);
#ifdef __COLOR_RULE_H__
	fprintf(stream,"\n%s\t\t\tcolorRule=\"set#%d\"",indent,dynAttr->colorRule+1);
#endif
  	fprintf(stream,"\n%s\t\t}",indent);
  	fprintf(stream,"\n%s\t\tparam {",indent);
  	fprintf(stream,"\n%s\t\t\tchan=\"%s\"",indent,dynAttr->name);
  	fprintf(stream,"\n%s\t\t}",indent);
  	fprintf(stream,"\n%s\t}",indent);
  	fprintf(stream,"\n%s}",indent);
    }
#endif
}

void writeDlObject(
  FILE *stream,
  DlObject *dlObject,
  int level)
{
    char indent[16];

    memset(indent,'\t',level);
    indent[level] = '\0';

    fprintf(stream,"\n%sobject {",indent);
    fprintf(stream,"\n%s\tx=%d",indent,dlObject->x);
    fprintf(stream,"\n%s\ty=%d",indent,dlObject->y);
    fprintf(stream,"\n%s\twidth=%d",indent,dlObject->width);
    fprintf(stream,"\n%s\theight=%d",indent,dlObject->height);
    fprintf(stream,"\n%s}",indent);
}

void genericMove(DlElement *dlElement, int xOffset, int yOffset) {
    dlElement->structure.rectangle->object.x += xOffset;
    dlElement->structure.rectangle->object.y += yOffset;
}

void widgetMove(DlElement *dlElement, int xOffset, int yOffset) {
    dlElement->structure.rectangle->object.x += xOffset;
    dlElement->structure.rectangle->object.y += yOffset;
    if (dlElement->widget)
      XtMoveWidget(dlElement->widget,
	dlElement->structure.rectangle->object.x,
	dlElement->structure.rectangle->object.y);
}

void genericScale(DlElement *dlElement, int xOffset, int yOffset) {
    int width, height;
    width = (dlElement->structure.rectangle->object.width + xOffset);
    dlElement->structure.rectangle->object.width = MAX(1,width);
    height = (dlElement->structure.rectangle->object.height + yOffset);
    dlElement->structure.rectangle->object.height = MAX(1,height);
}

void destroyElementWithDynamicAttribute(DlElement *dlElement) {
    if (dlElement->structure.rectangle->dynAttr.name) {
	freeString(dlElement->structure.rectangle->dynAttr.name);
    }
    free( (char *) dlElement->structure.composite);
    destroyDlElement(dlElement);
}
