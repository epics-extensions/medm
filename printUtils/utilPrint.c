/***
 *** Print Utilities (currently supporting only PostScript)
 ***
 *** MDA - 28 June 1990
 ***/

#define DEBUG_PRINT 0

#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <X11/Xlib.h>
/*
 * routine through which all graphic print requests are channeled
 */


void utilPrint(display,window,fileName)
    Display *display;
    Window window;
    char *fileName;
{
    char *commandBuffer, *newFileName, *psFileName, *printer;
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
	char *myArgv[4];
	int myArgc;

	myArgv[0] = "xwd2ps";
	myArgv[1] = "-d";
	myArgv[2] = "-t";
	myArgv[3] = newFileName;
	myArgc = 4;

	fo = fopen(psFileName,"w+");
	if (fo == NULL) {
	    fprintf(stderr,"\nutilPrint:  unable to open file: %s",psFileName);
	    return;
	}
	xwd2ps(myArgc,myArgv,fo);
	fclose(fo);

    }
#if 1
  /* KE: Need to free these */
    free(newFileName);
    free(psFileName);
#endif   

#if DEBUG_PRINT == 0
#ifndef VMS     /* UNIX code */
    strcpy(commandBuffer,"lp -c -d$PSPRINTER ");
    strcat(commandBuffer, psFileName);
    status=system(commandBuffer);
    if(status) {
	fprintf(stderr,"\nutilPrint:  print command [%s] failed",
	  commandBuffer);
	return;
    }
  /* Delete files */
    strcpy(commandBuffer,"rm ");
    strcat(commandBuffer,newFileName);
    system(commandBuffer);
    strcpy(commandBuffer,"rm ");
    strcat(commandBuffer,psFileName);
    system(commandBuffer);
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

}
