/*
 *	Mark Anderson, Argonne National Laboratory:
 *		U.S. DOE, University of Chicago
 */


#include "medm.h"



XtCallbackProc globalHelpCallback(
  Widget w,
  int helpIndex,
  XmAnyCallbackStruct *call_data)
{
  XmString string;

  switch (helpIndex) {
	case HELP_MAIN:
		string = XmStringCreateSimple("In Main Help...");
		XtVaSetValues(helpMessageBox,XmNmessageString,string,
				NULL);
		XtPopup(helpS,XtGrabNone);
		XmStringFree(string);
		break;

  }
}



