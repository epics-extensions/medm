/***
 ***   Utility routines & data structures
 ***
 ***	(MDA) 7 October 1990
 ***
 ***/

#include <X11/Xlib.h>
# include <string.h>
#include <stdio.h>


#define AVGSTRINGSIZE 30	/* aver. size of scale label string in pixels */

#define PATTERN_SIZE 120	/* max font pattern size */
#define MAX_FONT_NAMES 2	/* max number of returned fonts (only need 1) */
#define MAX_QUERIES 10		/* max number of server queries for font info */
#define DEFAULT_FONT "fixed"	/* string for default font */


/*
 * utility function to massage placement of axes scales
 */

int getOKStringPosition(stringPosition,usedSpots,usedCtr,stringHeight)
  int stringPosition, usedSpots[], *usedCtr, stringHeight;
{
int i, j, newPosition;

 newPosition = stringPosition;


 for (i=0; i<*usedCtr; i++) {
   if (newPosition > usedSpots[i] - stringHeight/2 &&
       newPosition < usedSpots[i] + stringHeight/2) {
	   newPosition = newPosition + 3*stringHeight/4;
	   newPosition = getOKStringPosition(newPosition,
				usedSpots,usedCtr,stringHeight);
	   return(newPosition);
   }
 }

 usedSpots[*usedCtr] = newPosition;
 (*usedCtr)++;

 return(newPosition);

}


/*
 * utility function to massage placement of axes scales (based on X and Y)
 */

void getOKStringPositionXY(stringPositionX, stringPositionY, 
	used, numUsed, nTicks, stringHeight)
  int *stringPositionX, *stringPositionY, used[], numUsed[], 
	nTicks, stringHeight;
{
int i, j, newPositionX, newPositionY;

 newPositionX = *stringPositionX;
 newPositionY = *stringPositionY;

/* change Y position if there are already entries there for that X position */

 for (i=0; i<nTicks; i++) {
   if (numUsed[i] != 0) {
      if (newPositionX > used[i] - AVGSTRINGSIZE &&
	newPositionX < used[i] + AVGSTRINGSIZE) {
	    newPositionY = *stringPositionY + numUsed[i]*(3*stringHeight/4);
	    numUsed[i]++;
	    *stringPositionX = newPositionX;
	    *stringPositionY = newPositionY;
	    return;
      }
   } else {
      used[i] = newPositionX;
      numUsed[i] = 1;
      return;
   }
 }
}



/*
 * utility function to get "best fit" of specified font family, face,
 *   type and size
 *
 *   this function allocates and returns a matching font string:
 *	first it tries the exactly specified pattern, if that fails,
 *	it tries the next smaller, then next larger, then next smaller...
 *
 *   NB: this function is server-intensive (potentially lots of queries to
 *   the server!)
 */

char *graphXGetBestFont(display,family,face,type,size)
  Display *display;
  char *family;			/* font family; e.g., "times" */
  char *face;			/* font typeface; e.g.,  "medium" or "bold" */
  char *type;			/* font type; e.g., "r" (roman), "i" (italic) */
  int size;			/* point size; e.g., 12 */
{
  char **fontList;
  char pattern[PATTERN_SIZE];
  char *fontString;
  int numFonts;
  int l, localSize;
  int loopCount;

/*
 * first see if the specified font exists
 */
  sprintf(pattern,"*-*%s*-*%s*-%s-*--%d-*",family,face,type,size);
printf("graphXGetBestFont: pattern = %s\n",pattern);
  fontList = XListFonts(display,pattern,MAX_FONT_NAMES,&numFonts);

  if (numFonts >= 1) {
/*
 * found the specified font, simply return that string
 */
      fontString = ((char *) malloc(strlen(fontList[0])+1));
      strcpy(fontString,fontList[0]);
      XFreeFontNames(fontList);
printf("graphXGetBestFont: found string: %s\n",fontString);
      return fontString;
  } else {
/*
 * didn't find that one, therefore try looking for ones around that size
 */
      l = 0;
      while ((numFonts <= 0) && (l <= MAX_QUERIES)) {
	l++;
	localSize = ((l%2 == 1) ? (size - (l -l/2)) : (size + (l - l/2)));
	sprintf(pattern,"*-*%s*-*%s*-%s-*--%d-*",family,face,type,localSize);
printf("graphXGetBestFont: pattern = %s\n",pattern);
	fontList = XListFonts(display,pattern,MAX_FONT_NAMES,&numFonts);
      }

      if (l >= MAX_QUERIES) {
printf("graphXGetBestFont: exceeded MAX_QUERIES; returning default string: %s\n"
		,DEFAULT_FONT);
         return (DEFAULT_FONT);
      } else {
         fontString = ((char *) malloc(strlen(fontList[0])+1));
         strcpy(fontString,fontList[0]);
         XFreeFontNames(fontList);
printf("graphXGetBestFont: found string: %s\n",fontString);
         return fontString;
      }

  }

}



/*
 * function to return a pixel value from a specified color string
 */
unsigned long getPixelFromString(display,screen,colorString)
  Display *display;
  int screen;
  char *colorString;
{
  XColor color, ignore;

  if(!XAllocNamedColor(display,DefaultColormap(display,screen),
         colorString,&color,&ignore)) {
        fprintf(stderr, "\ngetPixelFromString:  couldn't allocate color %s",
                colorString);
        return(WhitePixel(display, screen));
    } else {
        return(color.pixel);
    }
}

