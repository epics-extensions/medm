/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*****************************************************************************
 *
 *     Original Author : Kenneth Evans, Jr.
 *
 *****************************************************************************
*/

/* Note that there are separate WIN32 and UNIX versions */

#define DEBUG_EXEC 0
#define DEBUG_FIND 0

#ifndef WIN32
/*************************************************************************/
/*************************************************************************/
/* Firefox UNIX Version                                                        */
/*************************************************************************/
/*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xmu/WinUtil.h>


#ifndef FIREFOXPATH
#define FIREFOXPATH "firefox"
#endif

#ifdef VMS
#include <ssdef.h>
#include <lib$routines.h>
#include <ctype.h>
#include <descrip.h>
#include <clidef.h>
#endif

/* Function prototypes */

extern int kill(pid_t, int);     /* May not be defined for strict ANSI */

int callBrowser(char *url, char *bookmark);
static Window checkFirefoxWindow(Window w);
static int execute(char *s);
static Window findFirefoxWindow(void);
static int ignoreXError(Display *display, XErrorEvent *xev);

/* Global variables */
extern Display *display;

/**************************** callBrowser ********************************/
int callBrowser(char *url, char *bookmark)
  /* Returns non-zero on success, 0 on failure      */
  /* url is the URL that the browser is to display  */
  /*   or "quit" to terminate the browser           */
{
    int (*oldhandler)(Display *, XErrorEvent *);
    static Window firefoxw=(Window)0;
    static pid_t pid=0;
#if DEBUG_EXEC
    int status;
#endif
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
  /* Get BROWSER if it exists */
    envstring=getenv("BROWSER");
#if DEBUG_EXEC
    printf("BROWSER=%s (%p)\n",
      envstring?envstring:"Not Found",envstring);
#endif
    if (!envstring) { 
  /* Get FIREFOXPATH if it exists */
        envstring=getenv("FIREFOXPATH");
#if DEBUG_EXEC
        printf("FIREFOXPATH=%s (%p)\n",
          envstring?envstring:"Not Found",envstring);
#endif
    }
  /* Set handler to ignore possible BadWindow error */
  /*   (Would prefer a routine that tells if the window is defined) */
    oldhandler=XSetErrorHandler(ignoreXError);
  /* Check if the stored window value is valid */
    firefoxw=checkFirefoxWindow(firefoxw);
  /* Reset error handler */
    XSetErrorHandler(oldhandler);
  /* If stored window is not valid, look for a valid one */
    if(!firefoxw) {
	firefoxw=findFirefoxWindow();
      /* If no window found, exec Firefox */
	if(!firefoxw) {
#ifndef VMS
	    sprintf(command,"%s '%s%s' &",
	      envstring?envstring:FIREFOXPATH,url,bookmark);
#else
	    sprintf(command,"%s \"%s%s\"",
	      envstring?envstring:FIREFOXPATH,url,bookmark);
#endif
#if DEBUG_EXEC
	    printf("execute(no window before): cmd=%s\n",command);
	    status=execute(command);
	    printf("execute(no window after): cmd=%s status=%d\n",command,status);
#else
            execute(command);
#endif
	    return 1;
	}
    }
    
  /* Firefox window is valid, send url via -url */
#ifndef VMS
    sprintf(command,"%s -url '%s%s' &",
      envstring?envstring:FIREFOXPATH,url,bookmark);
#else
    sprintf(command,"%s -url \"%s%s\"",
      envstring?envstring:FIREFOXPATH,url,bookmark);
#endif

#if DEBUG_EXEC
    printf("execute(found window before): cmd=%s\n",command);
    status=execute(command);
    printf("execute(found window after): cmd=%s status=%d\n",command,status);
#else
    execute(command);
#endif

  /* Raise the window */
    XMapRaised(display,firefoxw);

    return 2;
}
/**************************** checkFirefoxWindow ************************/
static Window checkFirefoxWindow(Window w)
  /* Checks if this window is the Firefox window and returns the window
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
  /* If the version property exists, it is the Firefox window */
    if(version && status == Success) wfound=w;
#if DEBUG_FIND
    printf("XGetWindowProperty: status=%d version=%d w=%x wfound=%x\n",
      status,version,w,wfound);
#endif
  /* Free space and return */
    if(version) XFree((void *)version);
    return wfound;
}
/**************************** execute ************************************/
static int execute(char *s)
/* From O'Reilly, Vol. 1, p. 438 */
{
#ifndef VMS
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
#else
    int status,spawn_sts;
    int spawnFlags=CLI$M_NOWAIT;
    struct dsc$descriptor cmdDesc;
    cmdDesc.dsc$w_length  = strlen(s);
    cmdDesc.dsc$b_dtype   = DSC$K_DTYPE_T;
    cmdDesc.dsc$b_class   = DSC$K_CLASS_S;
    cmdDesc.dsc$a_pointer = s;
    spawn_sts = lib$spawn(&cmdDesc,0,0,&spawnFlags,0,0, &status,0,0,0,0,0);
    if(spawn_sts != 1)
      printf("statuss %d %d\n",spawn_sts, status);
#endif
}
/**************************** findFirefoxWindow *************************/
static Window findFirefoxWindow(void)
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
      /* Check if this is the Firefox window */
#if DEBUG_FIND
	printf("Child %d ",i);
#endif
	wfound=checkFirefoxWindow(w);
	if(wfound) break;
    }
    if(children) XFree((void *)children);
    return wfound;
}
/**************************** ignoreXError *******************************/
static int ignoreXError(Display *display, XErrorEvent *xev)
{
#if DEBUG_FIND
    printf("In ignoreXError\n");
#endif
    return 0;
}

#else     /*ifndef WIN32 */
/*************************************************************************/
/*************************************************************************/
/* WIN32 Version                                                        */
/*************************************************************************/
/*************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <string.h>
#include <errno.h>

void medmPrintf(int priority, char *format, ...);
int callBrowser(char *url, char *bookmark);

/**************************** callBrowser (WIN32) ************************/
int callBrowser(char *url, char *bookmark)
  /* Returns non-zero on success, 0 on failure */
  /* Should use the default browser            */
  /* Does nothing with "quit"                  */
{
    static int first=1;
    static char *ComSpec;
    char command[BUFSIZ];
    int status;

  /* Handle quit */
    if(!strcmp(url,"quit")) {
      /* For compatibility, but do nothing */
	return(3);
    }

  /* Get ComSpec for the command shell (should be defined) */
    if (first) {
	first=0;
	ComSpec = getenv("ComSpec");
    }
    if (!ComSpec) return(0);     /* Abort with no message like the UNIX version*/
  /* Spawn the process that handles a url */
#if 0
  /* Works, command window that goes away */
    sprintf(command,"start \"%s%s\"",url,bookmark);
    status = _spawnl(_P_WAIT, ComSpec, ComSpec, "/C", command, NULL);

  /* Works, command window that goes away */
    sprintf(command,"start \"%s%s\"",url,bookmark);
    status = _spawnl(_P_DETACH, ComSpec, ComSpec, "/C", command, NULL);

  /* Works, command window that goes away */
    sprintf(command,"\"%s%s\"",url,bookmark);
    status = _spawnl(_P_NOWAIT, "c:\\windows\\command\\start.exe",
      "c:\\windows\\command\\start.exe", command, NULL);

  /* Works, command window that goes away */
    sprintf(command,"\"%s%s\"",url,bookmark);
    status = _spawnl(_P_WAIT, ComSpec, "start", command, NULL);

  /* Works, command window that goes away */
    sprintf(command,"start \"%s%s\"",url,bookmark);
    status = _spawnl(_P_NOWAIT, ComSpec, ComSpec, "/C", command, NULL);

  /* Doesn't work on 95 (No such file or directory), works on NT */
    sprintf(command,"start \"%s%s\"",url,bookmark);
    status = _spawnl(_P_NOWAIT, ComSpec, "/C", command, NULL);

  /* Works on 95, not NT, no command window
   *   No start.exe for NT */
    sprintf(command,"\"%s%s\"",url,bookmark);
    status = _spawnlp(_P_DETACH, "start", "start", command, NULL);

  /* Doesn't work on 95 */
    sprintf(command,"\"start %s%s\"",url,bookmark);
    status = _spawnl(_P_DETACH, ComSpec, ComSpec, "/C", command, NULL);
#else
  /* This seems to work on 95 and NT, with a command box on 95
   *   It may have trouble if the URL has spaces */
    sprintf(command,"start %s%s",url,bookmark);
    status = _spawnl(_P_DETACH, ComSpec, ComSpec, "/C", command, NULL);
#endif
    if(status == -1) {
	char *errstring=strerror(errno);

	medmPrintf(1,"\ncallBrowser: Cannot start browser:\n"
	  "%s %s\n"
	  "  %s\n",ComSpec,command,errstring);
/* 	perror("callBrowser:"); */
	return(0);
    }
    return(1);
}
#endif     /* #ifndef WIN32 */
