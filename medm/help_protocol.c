/******************************************************************************
 *  
 * Author: Vladimir T. Romanovski  (romsky@x4u2.desy.de)
 *
 * Organization: KRYK/@DESY 1996
 *

*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>         /* system() */
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Shell.h>
#include <Xm/Xm.h>
#include <Xm/XmP.h>
#include <Xm/Protocols.h>
#include <Xm/AtomMgr.h>
  

/******************************************************************************
 *
 * callback: medm_help_callback
 *         
 * It is intended for reading MEDM_HELP system environment
 * and make system call mentioned in it via system(). Parameter for that is
 * a title of shell widget.
 *

*****************************************************************************/
static XtCallbackProc medm_help_callback (Widget shell, 
					  caddr_t closure,
					  caddr_t call_data)
{
  char * title = NULL;
  char * env = getenv("MEDM_HELP");

  if (env != NULL) 
  {
    char * command;
    
    XtVaGetValues (shell, XmNtitle, &title, NULL);

    command = (char*) malloc (strlen(env) + strlen(title) + 5);
    sprintf (command, "%s %s &", env, title);

    (void) system (command);
    free (command);
  } 
  else 
  {
    fprintf (stderr, "Sorry, environment MEDM_HELP is not set..\n");
  }
}


/*****************************************************************************
*
 * function: help_protocol(Widget shell)
 *
 * It is intented for installing customized Motif window manager protocol for
 * shell widget.
 *

*****************************************************************************/
void help_protocol (Widget shell)
{
  Atom message, protocol;
  char buf[80];

  message = XmInternAtom (XtDisplay(shell), "_MOTIF_WM_MESSAGES", FALSE);
  protocol = XmInternAtom (XtDisplay(shell), "_MEDM_HELP", FALSE);

  XmAddProtocols (shell, message, &protocol, 1);
  XmAddProtocolCallback (shell, message, protocol, medm_help_callback, NULL);

  sprintf (buf, "Medm_Help _l Ctrl<Key>l f.send_msg %d", protocol);
  XtVaSetValues (shell, XmNmwmMenu, buf, NULL);
}
