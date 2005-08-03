/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/* Print Utilities (currently supporting only PostScript) */

#define DEBUG_PRINT 0
#define DEBUG_PARAMS 0

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
/* KE: Formerly #include <sys/time.h> */
#include <time.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Intrinsic.h>

#ifdef WIN32
#include <X11/XlibXtra.h>
#endif

#include "printUtils.h"
#include "utilPrint.h"

int utilPrint(Display *display, Widget w, char *xwdFileName, char *title)
{
    Window window = XtWindow(w);
    char *commandBuffer=NULL, *newFileName=NULL, *psFileName=NULL;
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
	errMsg("utilPrint: Invalid display or window\n");
	retCode = 0;
	goto CLEAN;
    }

  /* Make a file name for Xwd */
    seconds = time(NULL);
    newFileName = (char *)calloc(1,PRINT_BUF_SIZE);
    if(!newFileName) {
	errMsg("utilPrint: "
	  "Unable to allocate space for the window dump file name\n");
	retCode = 0;
	goto CLEAN;
    }
    sprintf(newFileName,"%s%ld",xwdFileName,(long)seconds);

#if DEBUG_PARAMS
    print("utilPrint: newFilename=%s\n",newFileName);
#endif

  /* Dump the window */
    status = xwd(display, window, newFileName);
    if(!status) {
	errMsg("utilPrint: Cannot dump window\n");
	retCode=0;
	goto CLEAN;
    }

  /* Allocate a PS file name */
    psFileName = (char *)calloc(1,PRINT_BUF_SIZE);
    if(!psFileName) {
	errMsg("utilPrint: "
	  "Unable to allocate space for the Postscript file name\n");
	retCode = 0;
	goto CLEAN;
    }

  /* Set up the psFileName */
    if(printToFile) {
      /* Use the specified file name */
	if(strlen(printFile) >= PRINT_BUF_SIZE) {
	    errMsg("utilPrint: File name is too long: \n%s\n",
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

#if DEBUG_PARAMS
    print("utilPrint: psFilename=%s\n",psFileName);
#endif

  /* Allocate a buffer for system commands */
    commandBuffer = (char *)calloc(1,PRINT_BUF_SIZE);
    if(!commandBuffer) {
	errMsg("utilPrint: "
	  "Unable to allocate space for a command buffer\n");
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
    ptr = strchr(sizeString, ' ');
    if(ptr) *ptr='\0';
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

  /* Choose whether to print the argument or the global title */
    switch(printTitle) {
    case PRINT_TITLE_SHORT_NAME:
    case PRINT_TITLE_LONG_NAME:
	if(title && *title) {
	    sprintf(titleString,"-s%s", title);
	    myArgv[myArgc++] = titleString;
	}
	break;
    case PRINT_TITLE_SPECIFIED:
	if(printTitleString && *printTitleString) {
	    sprintf(titleString,"-s%s", printTitleString);
	    myArgv[myArgc++] = titleString;
	}
	break;
    }

    myArgv[myArgc++] = newFileName;

  /* Open the output file */
#ifdef WIN32
  /* WIN32 opens files in text mode by default and then mangles CRLF */
    fo = fopen(psFileName,"wb+");
#else
    fo = fopen(psFileName,"w+");
#endif
    if(fo == NULL) {
	errMsg("utilPrint: Unable to open file: %s\n",psFileName);
	retCode = 0;
	goto CLEAN;
    }

  /* Call xwd2ps */
    retCode = xwd2ps(myArgc,myArgv,fo);
    fclose(fo);
    if(!retCode) {
	errMsg("utilPrint: Could not convert window dump to Postscript\n");
	goto CLEAN;
    }
#if DEBUG_PRINT == 0    /* (When not debugging) */
    if(printRemoveTempFiles > 0) remove(newFileName);
#endif

  /* All done if print to file, otherwise print */
    if(printToFile) {
	goto CLEAN;
    } else {
      /* Print the file to the printer, unless you are debugging, in
         which case you can look at the files instead */
	sprintf(commandBuffer, "%s %s", printCommand, psFileName);
	status=system(commandBuffer);
#ifndef VMS
	if(status) {
	    errMsg("utilPrint: Print command [%s] failed\n",
	      commandBuffer);
	} else {
#if DEBUG_PRINT == 0    /* (When not debugging) */
	    if(printRemoveTempFiles > 1) remove(psFileName);
#endif
	}
#endif
    }

  CLEAN:
#if DEBUG_PARAMS
    {
	int i;

	print("utilPrint:\n");
	print("  printCommand: %s\n",printCommand?printCommand:"NULL");
	print("  printFile:    %s\n",printFile?printFile:"NULL");
	print("  xwdFileName:  %s\n",xwdFileName?xwdFileName:"NULL");
	print("  newFileName:  %s\n",newFileName?newFileName:NULL);
	print("  psFileName:   %s\n",psFileName?psFileName:"NULL");
	for(i=0; i < myArgc; i++) {
	    print("  myArgv[%d]:    %s\n",i,myArgv[i]);
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

int errMsg(const char *fmt, ...)
{
    va_list vargs;
    static char lstring[LPRINTF_SIZE];

    va_start(vargs,fmt);
    (void)vsprintf(lstring,fmt,vargs);
    va_end(vargs);

    if(lstring[0] != '\0') {
#ifdef WIN32
	lprintf("%s\n",lstring);
	fflush(stdout);
#else
	fprintf(stderr,"%s\n",lstring);
	fflush(stderr);
#endif
    }

    return 0;
}
