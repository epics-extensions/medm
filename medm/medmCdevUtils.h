/*-----------------------------------------------------------------------------
 * Copyright (c) 1994,1995 Southeastern Universities Research Association,
 *                         Continuous Electron Beam Accelerator Facility
 *
 * This software was developed under a United States Government license
 * described in the NOTICE file included as part of this distribution.
 *
 *-----------------------------------------------------------------------------
 *
 * Description:
 *      Header file for all conversion routines
 *
 *      Simple copy of epics header to avoid to include the epics path
 *      when to build medm using cdev
 *
 * Author:
 *      Jie Chen
 *      Jefferson Lab. Control Software Group
 *
 * Revision History:
 *   $Log$
 *   Revision 1.2  2005/08/03 14:56:55  evans
 *   Trimmed extra whitespace at end of lines in the source files.  There
 *   may be changes related to the WheelSwitch, as well.
 *
 *   Revision 1.1  1998/08/25 18:39:06  evans
 *   Incorporated changes from Jie Chen for CDEV.  Not tested with
 *   MEDM_CDEV defined.
 *
 *   Revision 1.1  1998/04/16 15:22:28  chen
 *   First working version of medm_cdev
 *
 *
 */
#ifndef _MEDM_CDEV_UTILS_H
#define _MEDM_CDEV_UTILS_H

#ifdef MEDM_CDEV

#if defined (__cplusplus)
extern "C" {
#endif

/* all these convertion routines */
extern int medmCvtFloatToString  (float value, char *pstring,
				  unsigned short precision);
extern int medmCvtDoubleToString  (double value, char *pstring,
				   unsigned short precision);
extern int medmCvtFloatToExpString (float value, char *pstring,
				    unsigned short precision);
extern int medmCvtDoubleToExpString (double value, char *pstring,
				     unsigned short precision);
extern int medmCvtFloatToCompactString(float value, char *pstring,
				       unsigned short precision);
extern int medmCvtDoubleToCompactString(double value, char *pstring,
					unsigned short precision);
extern int medmCvtCharToString (char value, char *pstring);
extern int medmCvtUcharToString (unsigned char value, char *pstring);
extern int medmCvtShortToString (short value, char *pstring);
extern int medmCvtUshortToString (unsigned short value, char *pstring);
extern int medmCvtLongToString (long value, char *pstring);
extern int medmCvtUlongToString (unsigned long value, char *pstring);
extern int medmCvtLongToHexString (long value, char *pstring);
extern int medmCvtLongToOctalString (long value, char *pstring);
extern unsigned long medmCvtBitsToUlong (
					 unsigned long  src,
					 unsigned bitFieldOffset,
					 unsigned  bitFieldLength);
extern  unsigned long  medmCvtUlongToBits (
					   unsigned long src,
					   unsigned long dest,
					   unsigned      bitFieldOffset,
					   unsigned      bitFieldLength);

#define cvtFloatToString medmCvtFloatToString
#define cvtDoubleToString medmCvtDoubleToString
#define cvtFloatToExpString medmCvtFloatToExpString
#define cvtFloatToExpString medmCvtFloatToExpString
#define cvtDoubleToExpString medmCvtDoubleToExpString
#define cvtFloatToCompactString medmCvtFloatToCompactString
#define cvtDoubleToCompactString medmCvtDoubleToCompactString
#define cvtCharToString medmCvtCharToString
#define cvtUcharToString medmCvtUcharToString
#define cvtShortToString medmCvtShortToString
#define cvtUshortToString medmCvtUshortToString
#define cvtLongToString medmCvtLongToString
#define cvtUlongToString medmCvtUlongToString
#define cvtLongToHexString medmCvtLongToHexString
#define cvtLongToOctalString medmCvtLongToOctalString
#define cvtBitsToUlong medmCvtBitsToUlong
#define cvtUlongToBits medmCvtUlongToBits

#if defined (__cplusplus)
};
#endif

#endif

#endif
