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
 * Modification Log:
 * -----------------
 * .01  03-01-95        vong    2.0.0 release
 * .02  09-05-95        vong    2.1.0 release
 * .03  09-12-95        vong    2.1.1 release
 * .04  09-28-95        vong    2.1.2 release
 * .05  10-02-95        vong    2.1.4 release
 * .06  10-03-95        vong    2.1.5 release
 * .07  10-05-95        vong    2.1.6 release
 * .08  10-20-95        vong    2.1.7 release
 * .09  10-27-95        vong    2.1.8 release
 * .10  11-06-95        vong    2.1.9 release
 * .11  11-14-95        vong    2.1.10 release 
 * .12  12-05-95        vong    2.1.11 release
 * .13  02-23-96        vong    2.1.12 release
 * .14  02-29-96        vong    2.1.13 release
 * .15  03-06-96        vong    2.1.14 release
 * .16  03-19-96        vong    2.1.15 release
 * .17  03-22-96        vong    2.1.16 release
 * .18  04-11-96        vong    2.1.17 release
 * .19  05-13-96        vong    2.2.0 release
 * .20  06-04-96        vong    2.2.1 release
 * .21  06-10-96        vong    2.2.2 release
 * .22  06-12-96        vong    2.2.3 release
 * .23  06-28-96        vong    2.2.4 release
 * .24  10-10-96        vong    2.2.5 release
 * .25  11-06-96        vong    2.2.6 release
 * .26  11-13-96        vong    2.2.7 release
 * .27  11-20-96        vong    2.2.8 release
 * .28  12-03-96        vong    2.2.9 release
 * .29  12-11-96        evans   2.3.0 release
 * .30  12-12-96        evans   2.3.1 release
 *
 *****************************************************************************
*/

/*
 * MEDM Version information
 */
#define MEDM_VERSION		2
#define MEDM_REVISION		3
#define MEDM_UPDATE_LEVEL	2

#ifdef __COLOR_RULE_H__
#define MEDM_VERSION_STRING	"MEDM Version 2.3.2aBeta11"
#else
#define MEDM_VERSION_STRING	"MEDM Version 2.3.2Beta11"
#endif
#define MEDM_VERSION_DIGITS	"MEDM020302Beta11"
