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
 *     Original Author : Kenneth Evans, Jr.
 *
 *****************************************************************************
*/

/* Note that there is a Mosaic version at the end of the file */

#define DEBUG 0

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>

#include <X11/Xlib.h>

#ifndef NETSCAPEPATH
#define NETSCAPEPATH "netscape"
#endif

/* Function prototypes */
int callBrowser(char *url);
static Window checkNetscapeWindow(Window w);
static int eexecute(char *s);
static Window findNetscapeWindow(void);
static int ignoreXError(Display *display, XErrorEvent *xev);

/* Global variables */
extern Display *display;

/**************************** callBrowser ********************************/
int callBrowser(char *url)
/* Returns non-zero on success, 0 on failure */
/* url is the URL that Mosaic is to display  */
/*   or "quit" to terminate Mosaic           */
{
    int (*oldhandler)(Display *, XErrorEvent *);
    static Window netscapew=(Window)0;
    static pid_t pid=0;
    int ntries=0,found,status;
    char command[BUFSIZ];
    char *envstring;
    
  /* Handle quit */
    if(!strcmp(url,"quit")) {
	if (pid) {
	    kill(pid,SIGTERM);
	    pid=0;
	}
	return 3;
    }
  /* Set handler to ignore possible BadWindow error */
  /*   (Would prefer a routine that tells if the window is defined) */
    oldhandler=XSetErrorHandler(ignoreXError);
  /* Check if the stored window value is valid */
    netscapew=checkNetscapeWindow(netscapew);
  /* Reset error handler */
    XSetErrorHandler(oldhandler);
  /* If stored window is not valid, look for a valid one */
    if(!netscapew) {
	netscapew=findNetscapeWindow();
      /* If no window found, exec Netscape */
	if(!netscapew) {
	    envstring=getenv("NETSCAPEPATH");
	    if(!envstring) {
		sprintf(command,"%s -install '%s' &",NETSCAPEPATH,url);
	    }
	    else {
		sprintf(command,"%s -install '%s' &",envstring);
	    }
#if DEBUG
	    printf("eexecute(before): cmd=%s\n",command);
#endif	    
	    status=eexecute(command);
#if DEBUG
	    printf("eexecute(after): cmd=%s status=%d\n",command,status);
#endif	    
	    return 1;
	}
    }
  /* Netscape window is valid, send url via -remote */
  /*   (Use -id for speed) */
    envstring=getenv("NETSCAPEPATH");
    if(!envstring) {
	sprintf(command,"%s -id 0x%x -remote 'openURL(%s)' &",
	  NETSCAPEPATH,netscapew,url);
    }
    else {
	sprintf(command,"%s -id 0x%x -remote 'openURL(%s)' &",
	  envstring,netscapew,url);
    }
#if DEBUG
    printf("eexecute(before): cmd=%s\n",command);
#endif    
    status=eexecute(command);
#if DEBUG
    printf("eexecute(after): cmd=%s status=%d\n",command,status);
#endif    
    return 2;
}
/**************************** checkNetscapeWindow ************************/
static Window checkNetscapeWindow(Window w)
  /* Checks if this window is the Netscape window and returns the window
   * if it is or 0 otherwise */    
{
    Window wfound=(Window)0;
    static Atom typeatom,versionatom=(Atom)0;
    unsigned long nitems,bytesafter;
    int format,status;
    unsigned char *version=NULL;
    
  /* If window is NULL, return it */
    if(!w) return w;
  /* Get the atom for the version property (once) */
    if(!versionatom) versionatom=XInternAtom(display,"_MOZILLA_VERSION",False);
  /* Get the version property for this window if it exists */
    status=XGetWindowProperty(display,w,versionatom,0,
      (65536/sizeof(long)),False,AnyPropertyType,
      &typeatom,&format,&nitems,&bytesafter,&version);
  /* If the version property exists, it is the Netscape window */
    if(version && status == Success) wfound=w;
#if DEBUG
    printf("XGetWindowProperty: status=%d version=%d w=%x wfound=%x\n",
      status,version,w,wfound);
#endif      
  /* Free space and return */
    if(version) XFree((void *)version);
    return wfound;
}
/**************************** eexecute ************************************/
static int eexecute(char *s)
/* From O'Reilly, Vol. 1, p. 438 */
{
    int status,pid,w;
    register void (*istat)(),(*qstat)();
    
    if((pid=fork()) == 0) {
	signal(SIGINT,SIG_DFL);
	signal(SIGQUIT,SIG_DFL);
	signal(SIGHUP,SIG_DFL);
	execl("/bin/sh","sh","-c",s,(char *)0);
	_exit(127);
    }
    istat=signal(SIGINT,SIG_IGN);
    qstat=signal(SIGQUIT,SIG_IGN);
    while((w=wait(&status)) != pid && w != -1) ;
    if(w == -1) status=-1;
    signal(SIGINT,istat);
    signal(SIGQUIT,qstat);
    return(status);
}
/**************************** findNetscapeWindow *************************/
static Window findNetscapeWindow(void)
{
    int screen=DefaultScreen(display);
    Window rootwindow=RootWindow(display,screen);
    Window *children,dummy,w,wfound=(Window)0;
    unsigned int nchildren;
    int i;

  /* Get the root window tree */
    if(!XQueryTree(display,rootwindow,&dummy,&dummy,&children,&nchildren))
      return (Window)0;
  /* Look at the children from the top of the stacking order */
    for(i=nchildren-1; i >= 0; i--) {
	w=XmuClientWindow(display,children[i]);
      /* Check if this is the Netsacpe window */
#if DEBUG
	printf("Child %d ",i);
#endif	
	wfound=checkNetscapeWindow(w);
	if(wfound) break;
    }
    if(children) XFree((void *)children);
    return wfound;
}
/**************************** ignoreXError *******************************/
static int ignoreXError(Display *display, XErrorEvent *xev)
{
#if DEBUG
    printf("In ignoreXError\n");
#endif    
    return 0;
}

/*************************************************************************/
/*************************************************************************/
/* Mosaic Version                                                        */ 
/*************************************************************************/
/*************************************************************************/
#if 0

#ifndef MOSAICPATH
/* #define MOSAICPATH "/usr/bin/X11/mosaic" */
/* #define MOSAICPATH "/opt/local/bin/mosaic" */
#define MOSAICPATH "mosaic"
#endif

/**************************** callBrowser ********************************/
int callBrowser(char *url)
/* Returns non-zero on success, 0 on failure */
/* url is the URL that Mosaic is to display  */
/*   or "quit" to terminate Mosaic           */
{
    static pid_t pid=0;
    char filename[32];
    FILE *file;
    char path[BUFSIZ];
    char *envstring;
    
    signal(SIGCHLD,SIG_IGN);
    
/* Handle quit */
    if(!strcmp(url,"quit")) {
	if (pid) {
	    sprintf(filename,"/tmp/Mosaic.%d",pid);
	    unlink(filename);
	    kill(pid,SIGTERM);
	    pid=0;
	}
	return 3;
    }
/* If Mosaic is not up, exec it */
    if ((!pid) || kill(pid,0)) {
	if (!(pid=fork())) {
	    envstring=getenv("MOSAICPATH");
	    if(!envstring) {
		sprintf(path,"%s",MOSAICPATH);
	    }
	    else {
		sprintf(path,"%s",envstring);
	    }
	    execlp(path,path,url,(char *)0);
	    perror(path);
	    _exit(127);
	}
	return 1;
    }
/* Mosaic is up, send message through file */
    sprintf(filename,"/tmp/Mosaic.%d",pid);
    if (!(file=fopen(filename,"w"))) return 0;
    fprintf(file,"goto\n%s\n",url);
    fclose(file);
    kill(pid,SIGUSR1);
    return 2;
}
#endif
