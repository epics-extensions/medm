/***
 *** Print Utilities (currently supporting only PostScript)
 ***
 *** MDA - 28 June 1990
 ***/

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
    char *commandBuffer, *newFileName, *psFileName;
    time_t seconds;
    FILE *fo;

    if (getenv("PSPRINTER") == (char *)NULL) {
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

    newFileName = (char *) calloc(1,256);
    sprintf(newFileName,"%s%d",fileName,seconds);
    psFileName = (char *) calloc(1,256);
    sprintf(psFileName,"%s%s",newFileName,".ps");

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

    strcpy(commandBuffer,"lp -d$PSPRINTER ");
    strcat(commandBuffer, psFileName);
    system(commandBuffer);
    strcpy(commandBuffer,"rm ");
    strcat(commandBuffer,newFileName);
    system(commandBuffer);
    strcpy(commandBuffer,"rm ");
    strcat(commandBuffer,psFileName);
    system(commandBuffer);

}
