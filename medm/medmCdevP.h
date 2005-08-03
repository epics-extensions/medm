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
 *      Medm CDEV Interface Implementation Header file
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
 *   Revision 1.1  1998/09/14 18:40:07  evans
 *   Renamed icon25 and medmPix25 to icon25.xpm and medmPix25.xpm.
 *
 *
 *
 */
#ifndef _MEDM_CDEVP_H
#define _MEDM_CDEVP_H

#ifdef MEDM_CDEV

#include <stdio.h>
#include <string.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <cdevTypes.h>

class cdevSystem;
class cdevRequestObject;

class medmInputFd
{
public:
  // constructor
  medmInputFd        (int f, int i, medmInputFd* next = 0);
  // destructor
  ~medmInputFd       (void);

  int                fd;
  XtInputId          id;
  medmInputFd*       next;
};

class medmXInput
{
public:
  // constrcutor
  medmXInput  (XtAppContext context, cdevSystem* system);
  // destructor
  ~medmXInput (void);

  // operations
  // add a single file descriptor
  void addInput    (int fd, XtPointer mask);
  void removeInput (int fd);

protected:
  // internal file descriptors
  medmInputFd* xfds_;

private:
  static void inputCallback (XtPointer, int*, XtInputId*);
  XtAppContext context_;

  cdevSystem* system_;

  // deny copy and assignment operations
  medmXInput (const medmXInput& input);
  medmXInput& operator = (const medmXInput& input);
};

extern medmXInput* medmGXinput;

#endif

#endif

