/* Print Utilities (currently supporting only PostScript) */

#define DEBUG_PRINT 0
#define DEBUG_PARAMS 1

#include <stdio.h>
#include <stdlib.h>
/* KE: Formerly #include <sys/time.h> */
#include <time.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#include "xwd2ps.h"
#include "utilPrint.h"

int utilPrint(Display *display, Widget w, char *xwdFileName, char *title)
{
    Window window = XtWindow(w);
    char *commandBuffer=NULL, *newFileName=NULL, *psFileName=NULL, *printer=NULL;
    time_t seconds;
    FILE *fo;
    char *myArgv[10];
    char *ptr;
    int myArgc;
    int status;
    int retCode = 1;
    char widthString[80];
    char heightString[80];
    char sizeString[80];
    char titleString[PRINT_BUF_SIZE];

  /* Return if not enough information around */
    if(display == (Display *)NULL || window == (Window)NULL
      || xwdFileName == (char *)NULL) {
	retCode = 0;
	goto CLEAN;
    }

  /* Make a file name for Xwd */
    seconds = time(NULL);
    newFileName = (char *)calloc(1,PRINT_BUF_SIZE);
    if(!newFileName) {
	fprintf(stderr,"\nutilPrint:  "
	  "Unable to allocate space for the window dump file name/n");
	retCode = 0;
	goto CLEAN;
    }
    sprintf(newFileName,"%s%d",xwdFileName,seconds);

  /* Dump the window */
    xwd(display, window, newFileName);

  /* Allocate a PS file name */
    psFileName = (char *)calloc(1,PRINT_BUF_SIZE);
    if(!psFileName) {
	fprintf(stderr,"\nutilPrint:  "
	  "Unable to allocate space for the postscript file name/n");
	retCode = 0;
	goto CLEAN;
    }

  /* Set up the psFileName */
    if(printToFile) {
      /* Use the specified file name */
	if(strlen(printFile) >= PRINT_BUF_SIZE) {
	    fprintf(stderr,"\nutilPrint:  File name is too long: \n%s\n",
	      printFile);
	    retCode = 0;
	    goto CLEAN;
	} else {
	    strncpy(psFileName, printFile, PRINT_BUF_SIZE);
	    psFileName[PRINT_BUF_SIZE-1] = '\0';
	}
    } else {
      /* Make a file name */
#ifndef VMS
	sprintf(psFileName,"%s%s",newFileName,".ps");
#else
	sprintf(psFileName,"%s%s",newFileName,"_ps");
#endif
    }

  /* Allocate a buffer for system commands */
    commandBuffer = (char *)calloc(1,PRINT_BUF_SIZE);
    if(!commandBuffer) {
	fprintf(stderr,"\nutilPrint:  "
	  "Unable to allocate space for a command buffer/n");
	retCode = 0;
	goto CLEAN;
    }

  /* Create the program arguments for xwd2ps */
    myArgc = 0;
    myArgv[myArgc++] = "xwd2ps";
  /* KE: Specify something explicit for orientation, don't rely on
     getOrientation unless it is fixed */
    myArgv[myArgc++] = printOrientation == PRINT_PORTRAIT?"-P":"-L";

    sprintf(sizeString,"-p%s", printerSizeTable[printSize]);
  /* Only use up to the first blank */
    if(ptr = strchr(sizeString, ' ')) *ptr='\0';
    myArgv[myArgc++] = sizeString;

    if(printDate) myArgv[myArgc++] = "-d";

    if(printTime) myArgv[myArgc++] = "-t";

    if(printWidth > 0.00) {
	sprintf(widthString,"-w%f", printWidth);
	myArgv[myArgc++] = widthString;
    }

    if(printHeight > 0.00) {
	sprintf(heightString,"-h%f", printHeight);
	myArgv[myArgc++] = heightString;
    }

    if(printTitle && *title) {
	sprintf(titleString,"-s%s", title);
	myArgv[myArgc++] = titleString;
    }
    
    myArgv[myArgc++] = newFileName;
    
  /* Open the output file */
    fo = fopen(psFileName,"w+");
    if(fo == NULL) {
	fprintf(stderr,"\nutilPrint:  Unable to open file: %s\n",psFileName);
	retCode = 0;
	goto CLEAN;
    }

  /* Call xwd2ps */
    retCode = xwd2ps(myArgc,myArgv,fo);
    fclose(fo);
    if(!retCode) goto CLEAN;

  /* All done if print to file, otherwise print */
    if(printToFile) {
	goto CLEAN;
    } else {
      /* Print the file */
#if DEBUG_PRINT == 0
      /* Don't do this when debugging so you can look at the files */
#ifndef VMS     /* UNIX code */
	sprintf(commandBuffer, "%s %s", printCommand, psFileName);
	status=system(commandBuffer);
	if(status) {
	    fprintf(stderr,"\nutilPrint:  Print command [%s] failed\n",
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
	status=system(commandBuffer);
	if(status) {
	    fprintf(stderr,"\nutilPrint:  Print command [%s] failed\n",
	      commandBuffer);
	} else {
	    strcpy(commandBuffer,"delete/noconfirm/nolog ");
	    strcat(commandBuffer,newFileName);
	    strcat(commandBuffer,";*");
	    printf("system command %s\n",commandBuffer);
	    system(commandBuffer);
	}
#endif     /* #ifndef VMS */
#endif     /* #if DEBUG_PRINT == 0 */
    }

  CLEAN:
#if DEBUG_PARAMS
    {
	int i;
	
	printf("utilPrint:\n");
	printf("  printCommand: %s\n",printCommand?printCommand:"NULL");
	printf("  printFile:    %s\n",printFile?printFile:"NULL");
	printf("  xwdFileName:  %s\n",xwdFileName?xwdFileName:"NULL");
	printf("  newFileName:  %s\n",newFileName?newFileName:NULL);
	printf("  psFileName:   %s\n",psFileName?psFileName:"NULL");
	for(i=0; i < myArgc; i++) {
	    printf("  myArgv[%d]:    %s\n",i,myArgv[i]);
	}
    }
#endif

  /* KE: Need to free these.  Don't free print, which is an
     environment variable */
    if(commandBuffer) free(commandBuffer);
    if(newFileName) free(newFileName);
    if(psFileName) free(psFileName);

    return retCode;
}
