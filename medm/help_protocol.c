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
 *     Original Author : Mark Anderson
 *     Second Author   : Frederick Vong
 *     Third Author    : Kenneth Evans, Jr.
 *
 *****************************************************************************
*/
/*****************************************************************************
 *  
 * Original Author, This file: Vladimir T. Romanovski  (romsky@x4u2.desy.de)
 * Organization: KRYK/@DESY 1996
 *
 *****************************************************************************
*/

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
