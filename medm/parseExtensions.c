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
 *
 *****************************************************************************
*/

/***************************************************************************
 ****				parseExtensions.c			****
 ***************************************************************************/

#include "medm.h"

#include <X11/keysym.h>

static DlObject defaultObject = {0,0,5,5};



void parseImage(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel;
  DlImage *dlImage;
  DlElement *dlElement;


  dlImage = (DlImage *) calloc(1,sizeof(DlImage));

/* initialize some data in structure */
  dlImage->object = defaultObject;
  dlImage->imageType = NO_IMAGE;
  dlImage->imageName[0] = '\0';


  nestingLevel = 0;
  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object")) {
			parseObject(displayInfo,&(dlImage->object));
		} else if (!strcmp(token,"type")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"none"))
			    dlImage->imageType = NO_IMAGE;
			else if (!strcmp(token,"gif"))
			    dlImage->imageType = GIF_IMAGE;
			else if (!strcmp(token,"tiff"))
			    dlImage->imageType = TIFF_IMAGE;
		} else if (!strcmp(token,"image name")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(dlImage->imageName,token);
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

  /* just to be safe, initialize privateData member separately */
  dlImage->privateData = NULL;

  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Image;
  dlElement->structure.image = dlImage;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlImage;
  dlElement->dmWrite =  (void(*)())writeDlImage;

}



void parseComposite(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel;
  DlComposite *newDlComposite;
  DlElement *dlElement;


  newDlComposite = (DlComposite *) calloc(1,sizeof(DlComposite));

/* initialize some data in structure */
  newDlComposite->object = defaultObject;
  newDlComposite->compositeName[0] = '\0';
  newDlComposite->chan[0] = '\0';
  newDlComposite->vis = V_STATIC;
  newDlComposite->visible = True;	/* run-time visibility initially True */
  newDlComposite->monitorAlreadyAdded = False;

/* slight change here, since this object goes on main chain and subsequent
 *  children go on this composite's chain...
 */
  dlElement = (DlElement *) malloc(sizeof(DlElement));
  dlElement->type = DL_Composite;
  dlElement->structure.composite = newDlComposite;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlComposite;
  dlElement->dmWrite =  (void(*)())writeDlComposite;

  newDlComposite->dlElementListHead  = (XtPointer)calloc(1,sizeof(DlElement));
  ((DlElement *)(newDlComposite->dlElementListHead))->next = NULL;
  newDlComposite->dlElementListTail = newDlComposite->dlElementListHead;



  nestingLevel = 0;
  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object")) {
			parseObject(displayInfo,&(newDlComposite->object));
		} else if (!strcmp(token,"composite name")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(newDlComposite->compositeName,token);
		} else if (!strcmp(token,"vis")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			if (!strcmp(token,"static"))
				newDlComposite->vis = V_STATIC;
			else if (!strcmp(token,"if not zero"))
				newDlComposite->vis = IF_NOT_ZERO;
			else if (!strcmp(token,"if zero"))
				newDlComposite->vis = IF_ZERO;
		} else if (!strcmp(token,"chan")) {
			getToken(displayInfo,token);
			getToken(displayInfo,token);
			strcpy(newDlComposite->chan,token);
		} else if (!strcmp(token,"children")) {
			parseCompositeChildren(displayInfo,newDlComposite);
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

}



void parsePolylinePoints(
  DisplayInfo *displayInfo,
  DlPolyline *dlPolyline)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel;
#define INITIAL_NUM_POINTS 16
  int pointsArraySize = INITIAL_NUM_POINTS;

/* initialize some data in structure */
  dlPolyline->nPoints = 0;
  dlPolyline->points = (XPoint *)malloc(pointsArraySize*sizeof(XPoint));

  nestingLevel = 0;
  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"(")) {
			if (dlPolyline->nPoints >= pointsArraySize) {
			/* reallocate the points array: enlarge by 4X, etc */
			    pointsArraySize *= 4;
			    dlPolyline->points = (XPoint *)realloc(
					dlPolyline->points,
					(pointsArraySize+1)*sizeof(XPoint));
			}
			getToken(displayInfo,token);
			dlPolyline->points[dlPolyline->nPoints].x = atoi(token);
			getToken(displayInfo,token);	/* separator	*/
			getToken(displayInfo,token);
			dlPolyline->points[dlPolyline->nPoints].y = atoi(token);
			getToken(displayInfo,token);	/*   ")"	*/
			dlPolyline->nPoints++;
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

}


void parsePolyline(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel;
  DlPolyline *dlPolyline;
  DlElement *dlElement;

  dlPolyline = (DlPolyline *) calloc(1,sizeof(DlPolyline));
/* initialize some data in structure */
  dlPolyline->object = defaultObject;
  dlPolyline->nPoints = 0;
  dlPolyline->points = (XPoint *)NULL;

  nestingLevel = 0;
  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object")) {
			parseObject(displayInfo,&(dlPolyline->object));
		} else if (!strcmp(token,"points")) {
			parsePolylinePoints(displayInfo,dlPolyline);
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
  dlElement->type = DL_Polyline;
  dlElement->structure.polyline = dlPolyline;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlPolyline;
  dlElement->dmWrite =  (void(*)())writeDlPolyline;
}



void parsePolygonPoints(
  DisplayInfo *displayInfo,
  DlPolygon *dlPolygon)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel;
#define INITIAL_NUM_POINTS 16
  int pointsArraySize = INITIAL_NUM_POINTS;

/* initialize some data in structure */
  dlPolygon->nPoints = 0;
  dlPolygon->points = (XPoint *)malloc(pointsArraySize*sizeof(XPoint));

  nestingLevel = 0;
  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"(")) {
			if (dlPolygon->nPoints >= pointsArraySize) {
			/* reallocate the points array: enlarge by 4X, etc */
			    pointsArraySize *= 4;
			    dlPolygon->points = (XPoint *)realloc(
					dlPolygon->points,
					(pointsArraySize+1)*sizeof(XPoint));
			}
			getToken(displayInfo,token);
			dlPolygon->points[dlPolygon->nPoints].x = atoi(token);
			getToken(displayInfo,token);	/* separator	*/
			getToken(displayInfo,token);
			dlPolygon->points[dlPolygon->nPoints].y = atoi(token);
			getToken(displayInfo,token);	/*   ")"	*/
			dlPolygon->nPoints++;
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

  /* ensure closure of the polygon... */
  if (dlPolygon->points[0].x != dlPolygon->points[dlPolygon->nPoints-1].x &&
      dlPolygon->points[0].y != dlPolygon->points[dlPolygon->nPoints-1].y) {
    if (dlPolygon->nPoints >= pointsArraySize) {
	dlPolygon->points = (XPoint *)realloc(dlPolygon->points,
				(dlPolygon->nPoints+2)*sizeof(XPoint));
    }
    dlPolygon->points[dlPolygon->nPoints].x = dlPolygon->points[0].x;
    dlPolygon->points[dlPolygon->nPoints].y = dlPolygon->points[0].y;
    dlPolygon->nPoints++;
  }

}



void parsePolygon(
  DisplayInfo *displayInfo,
  DlComposite *dlComposite)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel;
  DlPolygon *dlPolygon;
  DlElement *dlElement;

  dlPolygon = (DlPolygon *) calloc(1,sizeof(DlPolygon));
/* initialize some data in structure */
  dlPolygon->object = defaultObject;
  dlPolygon->nPoints = 0;
  dlPolygon->points = (XPoint *)NULL;
  nestingLevel = 0;
  do {
	switch( (tokenType=getToken(displayInfo,token)) ) {
	    case T_WORD:
		if (!strcmp(token,"object")) {
			parseObject(displayInfo,&(dlPolygon->object));
		} else if (!strcmp(token,"points")) {
			parsePolygonPoints(displayInfo,dlPolygon);
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
  dlElement->type = DL_Polygon;
  dlElement->structure.polygon = dlPolygon;
  dlElement->next = NULL;

  POSITION_ELEMENT_ON_LIST();

  dlElement->dmExecute =  (void(*)())executeDlPolygon;
  dlElement->dmWrite =  (void(*)())writeDlPolygon;
}

