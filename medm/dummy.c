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
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (708-252-2000).
*/
/*****************************************************************************
 *
 *     Original Author : Mark Andersion
 *     Current Author  : Frederick Vong
 *
 * Modification Log:
 * -----------------
 * .01  03-01-95        vong    2.0.0 release
 *
 *****************************************************************************
*/

/*
 *  dummy.c
 *
 *  dummy routines for architectures (not yet) supporting full EPICS services
 *	(e.g., channel access client libraries)
 */

#include <stdio.h>

#if defined(DUMMY_CA)

void ca_task_initialize() {fprintf(stderr,"using dummy CA library!!"); }
void ca_signal() { fprintf(stderr,"using dummy CA library!!"); }
void ca_add_fd_registration() { fprintf(stderr,"using dummy CA library!!"); }
void ca_task_exit() { fprintf(stderr,"using dummy CA library!!"); }
void ca_pend() { fprintf(stderr,"using dummy CA library!!"); }
int SEVCHK() { fprintf(stderr,"using dummy CA library!!"); }
int ca_get_callback() { fprintf(stderr,"using dummy CA library!!"); }
int ca_puser() { fprintf(stderr,"using dummy CA library!!"); }
int ca_add_event() { fprintf(stderr,"using dummy CA library!!"); }
int ca_build_and_connect() { fprintf(stderr,"using dummy CA library!!"); }
int ca_add_masked_array_event() { fprintf(stderr,"using dummy CA library!!"); }
int ca_array_get_callback() { fprintf(stderr,"using dummy CA library!!"); }
int ca_clear_event() { fprintf(stderr,"using dummy CA library!!"); }
int ca_put() {fprintf(stderr,"using dummy CA library!!"); }
int ca_array_put() {fprintf(stderr,"using dummy CA library!!"); }
int ca_flush_io() {fprintf(stderr,"using dummy CA library!!"); }

#endif
