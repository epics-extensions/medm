/* Print Utilities (currently supporting only PostScript) */

#define DEBUG_PRINT 0

#include <stdio.h>
#include <stdlib.h>
/* KE: Formerly #include <sys/time.h> */
#include <time.h>
#include <string.h>

#include <X11/Xlib.h>
/*
 * routine through which all graphic print requests are channeled
 */

/* KE: Added for function prototypes */
#include "xwd2ps.h"

void utilPrint(display,window,fileName)
    Display *display;
    Window window;
    char *fileName;
{
    char *commandBuffer=NULL, *newFileName=NULL, *psFileName=NULL, *printer=NULL;
    time_t seconds;
    FILE *fo;
    int status;
    if ((printer=getenv("PSPRINTER")) == (char *)NULL) {
	fprintf(stderr,
	  "\nutilPrint: PSPRINTER environment variable not set, printing disallowed\n");
	return;
    }

  /* return if not enough information around */
    if (display == (Display *)NULL || window == (Window)NULL
      || fileName == (char *)NULL) return;

    seconds = time(NULL);
    newFileName = (char *) calloc(1,256);
    sprintf(newFileName,"%s%d",fileName,seconds);

    commandBuffer = (char *) calloc(1,256);

    xwd(display, window, newFileName);

#if 0
  /* KE: Not necessary */
    newFileName = (char *) calloc(1,256);
    sprintf(newFileName,"%s%d",fileName,seconds);
#endif    
    psFileName = (char *) calloc(1,256);
#ifndef VMS
    sprintf(psFileName,"%s%s",newFileName,".ps");
#else
    sprintf(psFileName,"%s%s",newFileName,"_ps");
#endif

    {
	char *myArgv[5];
	int myArgc;

      /* KE:" Fixed to always be portrait since the result has always
         previously been portrait.  See the notes for getOrientation.  */
	myArgv[0] = "xwd2ps";
	myArgv[1] = "-d";     /* Date */
	myArgv[2] = "-t";     /* Time */
	myArgv[3] = "-P";     /* Portrait */
	myArgv[4] = newFileName;
	myArgc = 5;

	fo = fopen(psFileName,"w+");
	if (fo == NULL) {
	    fprintf(stderr,"\nutilPrint:  unable to open file: %s",psFileName);
	    return;
	}
	xwd2ps(myArgc,myArgv,fo);
	fclose(fo);

    }

#if DEBUG_PRINT == 0
#ifndef VMS     /* UNIX code */
    strcpy(commandBuffer,"lp -c -d$PSPRINTER ");
    strcat(commandBuffer, psFileName);
    status=system(commandBuffer);
    if(status) {
	fprintf(stderr,"\nutilPrint:  print command [%s] failed",
	  commandBuffer);
    } else {
      /* Delete files */
	strcpy(commandBuffer,"rm ");
	strcat(commandBuffer,newFileName);
	system(commandBuffer);
	strcpy(commandBuffer,"rm ");
	strcat(commandBuffer,psFileName);
	system(commandBuffer);
    }
#else     /* VMS code */
    sprintf(commandBuffer,"print /queue=%s/delete \0",printer);
    strcat(commandBuffer, psFileName);
    printf("system command %s\n",commandBuffer);
    system(commandBuffer);
    strcpy(commandBuffer,"delete/noconfirm/nolog ");
    strcat(commandBuffer,newFileName);
    strcat(commandBuffer,";*");
    printf("system command %s\n",commandBuffer);
    system(commandBuffer);
#endif     /* #ifndef VMS */
#endif     /* #if DEBUG_PRINT == 0 */

  /* KE: Need to free these.  Don't free print, which is an environment variable */
    if(commandBuffer) free(commandBuffer);
    if(newFileName) free(newFileName);
    if(psFileName) free(psFileName);
}
