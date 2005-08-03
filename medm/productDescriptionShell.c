/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 * routine to create product description shell for all Motif-based EPICS tools
 *
 *  input parameters are the product name, and product description
 */

#define VERSION 1.1.0

#define DEBUG_POSITION 0

#include <stdio.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Xatom.h>
#include <Xm/Xm.h>
#include <Xm/Protocols.h>

#include <Xm/Label.h>
#include <Xm/Form.h>
#include <Xm/Separator.h>
#include <Xm/PushB.h>
#include <Xm/MwmUtil.h>

#ifdef WIN32
/* Hummingbird extra functions including lprintf
 *   Needs to be included after Intrinsic.h for Exceed 5 */
#include <X11/XlibXtra.h>
#define printf lprintf
#endif

static Widget okButton;

/* Function prototypes */

Widget createAndPopupProductDescriptionShell(
  XtAppContext appContext,	/* application context		*/
  Widget topLevelShell,		/* application's topLevel shell	*/
  char *name,			/* product/program name		*/
  XmFontList nameFontList,	/*   and font list (or NULL)	*/
  Pixmap namePixmap,		/* name Pixmap (or NULL)	*/
  char *description,		/* product description		*/
  XmFontList descriptionFontList,/*   and font list (or NULL)	*/
  char *versionInfo,		/* product version number	*/
  char *developedAt,		/* at and by...			*/
  XmFontList otherFontList,	/*   and font list (or NULL)	*/
  int background,		/* background color (or -1)	*/
  int foreground,		/* foreground color (or -1)	*/
  int seconds);			/* seconds to leave posted	*/
static void closeProductDescriptionCallback1(Widget w,
  XtPointer clientData, XtPointer callData);
static void closeProductDescriptionCallback2(Widget w,
  XtPointer clientData, XtPointer callData);
static void popdownProductDescriptionShell(XtPointer xtPointer);

/* Global variables */
static Atom WM_DELETE_WINDOW;

/*
 * Function to create, set and popup an EPICS product description shell
 *  widget hierarchy:
 *
 * productDescriptionShell
 *    form
 *	nameLabel
 *	separator
 *	descriptionLabel
 *	versionInfoLabel
 *	developedAtLabel
 *	okButton
 *
 */
Widget createAndPopupProductDescriptionShell(
  XtAppContext appContext,	/* application context		*/
  Widget topLevelShell,		/* application's topLevel shell	*/
  char *name,			/* product/program name		*/
  XmFontList nameFontList,	/*   and font list (or NULL)	*/
  Pixmap namePixmap,		/* name Pixmap (or NULL)	*/
  char *description,		/* product description		*/
  XmFontList descriptionFontList,/*   and font list (or NULL)	*/
  char *versionInfo,		/* product version number	*/
  char *developedAt,		/* at and by...			*/
  XmFontList otherFontList,	/*   and font list (or NULL)	*/
  int background,		/* background color (or -1)	*/
  int foreground,		/* foreground color (or -1)	*/
  int seconds)			/* seconds to leave posted	*/
{
    Display *display;
    Widget productDescriptionShell, form;
    Arg args[16];
    Widget children[6], nameLabel, descriptionLabel, versionInfoLabel,
      separator, developedAtLabel;
    XmString nameXmString = (XmString)NULL,
      descriptionXmString = (XmString)NULL,
      versionInfoXmString = (XmString)NULL,
      developedAtXmString = (XmString)NULL, okXmString = (XmString)NULL;
    Dimension formHeight, nameHeight;
    Dimension shellHeight, shellWidth;
    Dimension screenHeight, screenWidth;
    Position newY, newX;
    int nargs, offset, screen;


  /* Create the shell */
    nargs = 0;
    if(background >= 0) {
	XtSetArg(args[nargs],XmNbackground,(unsigned long)background);
	nargs++;
    }
    if(foreground >= 0) {
	XtSetArg(args[nargs],XmNforeground,(unsigned long)foreground);
	nargs++;
    }
    XtSetArg(args[nargs],XmNmwmDecorations, MWM_DECOR_ALL|
      MWM_DECOR_BORDER|MWM_DECOR_RESIZEH|MWM_DECOR_TITLE|MWM_DECOR_MENU|
      MWM_DECOR_MINIMIZE|MWM_DECOR_MAXIMIZE); nargs++;
    XtSetArg(args[nargs],XmNmwmFunctions, MWM_FUNC_ALL); nargs++;
    XtSetArg(args[nargs],XmNtitle,"Version"); nargs++;
    productDescriptionShell = XtCreatePopupShell("productDescriptionShell",
      topLevelShellWidgetClass,topLevelShell,args, nargs);
    display=XtDisplay(productDescriptionShell);
    screen=DefaultScreen(display);

  /* Set the window manager close callback to the first callback.
     This should not be necessary because the window manager close
     button should not appear, but it does anyway with some window
     managers */
    WM_DELETE_WINDOW = XmInternAtom(display,"WM_DELETE_WINDOW",False);
    XmAddWMProtocolCallback(productDescriptionShell,WM_DELETE_WINDOW,
      (XtCallbackProc)closeProductDescriptionCallback1,
      (XtPointer)productDescriptionShell);

    nargs = 0;
    if(background >= 0) {
	XtSetArg(args[nargs],XmNbackground,(unsigned long)background);
	nargs++; }
    if(foreground >= 0) {
	XtSetArg(args[nargs],XmNforeground,(unsigned long)foreground);
	nargs++; }
    XtSetArg(args[nargs],XmNnoResize,True); nargs++;
    XtSetArg(args[nargs],XmNshadowThickness,2); nargs++;
    XtSetArg(args[nargs],XmNshadowType,XmSHADOW_OUT); nargs++;
    XtSetArg(args[nargs],XmNautoUnmanage,False); nargs++;
    form = XmCreateForm(productDescriptionShell,"form",args,nargs);

  /* Generate XmStrings */
    if(name != NULL) nameXmString = XmStringCreateLtoR(name,
      XmFONTLIST_DEFAULT_TAG);
    if(description != NULL) descriptionXmString =
			       XmStringCreateLtoR(description,
				 XmFONTLIST_DEFAULT_TAG);
    if(versionInfo != NULL) versionInfoXmString =
			       XmStringCreateLtoR(versionInfo,
				 XmFONTLIST_DEFAULT_TAG);
    if(developedAt != NULL) developedAtXmString =
			       XmStringCreateLtoR(developedAt,
				 XmFONTLIST_DEFAULT_TAG);

  /* Create the label children  */
  /* Name */
    nargs = 0;
    if(namePixmap == (Pixmap) NULL) {
	XtSetArg(args[nargs],XmNlabelString,nameXmString); nargs++;
	if(nameFontList != NULL) {
	    XtSetArg(args[nargs],XmNfontList,nameFontList); nargs++;
	}
    } else {
	XtSetArg(args[nargs],XmNlabelType,XmPIXMAP); nargs++;
	XtSetArg(args[nargs],XmNlabelPixmap,namePixmap); nargs++;
    }
    XtSetArg(args[nargs],XmNalignment,XmALIGNMENT_BEGINNING); nargs++;
    XtSetArg(args[nargs],XmNleftAttachment,XmATTACH_POSITION); nargs++;
    XtSetArg(args[nargs],XmNleftPosition,1); nargs++;
    XtSetArg(args[nargs],XmNresizable,False); nargs++;
    if(background >= 0) {
	XtSetArg(args[nargs],XmNbackground,(unsigned long)background);
	nargs++; }
    if(foreground >= 0) {
	XtSetArg(args[nargs],XmNforeground,(unsigned long)foreground);
	nargs++; }
    nameLabel = XmCreateLabel(form,"nameLabel",args,nargs);


  /* Separator */
    nargs = 0;
    XtSetArg(args[nargs],XmNtopAttachment,XmATTACH_FORM); nargs++;
    XtSetArg(args[nargs],XmNbottomAttachment,XmATTACH_FORM); nargs++;
    XtSetArg(args[nargs],XmNleftAttachment,XmATTACH_WIDGET); nargs++;
    XtSetArg(args[nargs],XmNleftWidget,nameLabel); nargs++;
    XtSetArg(args[nargs],XmNorientation,XmVERTICAL); nargs++;
    XtSetArg(args[nargs],XmNshadowThickness,2); nargs++;
    XtSetArg(args[nargs],XmNseparatorType,XmSHADOW_ETCHED_IN); nargs++;
    if(background >= 0) {
	XtSetArg(args[nargs],XmNbackground,(unsigned long)background);
	nargs++; }
    if(foreground >= 0) {
	XtSetArg(args[nargs],XmNforeground,(unsigned long)foreground);
	nargs++; }
    separator = XmCreateSeparator(form,"separator",args,nargs);

  /* Description */
    nargs = 0;
    XtSetArg(args[nargs],XmNalignment,XmALIGNMENT_BEGINNING); nargs++;
    XtSetArg(args[nargs],XmNlabelString,descriptionXmString); nargs++;
    XtSetArg(args[nargs],XmNtopAttachment,XmATTACH_POSITION); nargs++;
    XtSetArg(args[nargs],XmNtopPosition,5); nargs++;
    XtSetArg(args[nargs],XmNleftAttachment,XmATTACH_WIDGET); nargs++;
    XtSetArg(args[nargs],XmNleftWidget,separator); nargs++;
	XtSetArg(args[nargs],XmNrightAttachment,XmATTACH_POSITION); nargs++;
	XtSetArg(args[nargs],XmNrightPosition,90); nargs++;
    if(descriptionFontList != NULL) {
	XtSetArg(args[nargs],XmNfontList,descriptionFontList); nargs++;
    }
    if(background >= 0) {
	XtSetArg(args[nargs],XmNbackground,(unsigned long)background); nargs++; }
    if(foreground >= 0) {
	XtSetArg(args[nargs],XmNforeground,(unsigned long)foreground); nargs++; }
    descriptionLabel = XmCreateLabel(form,"descriptionLabel",args,nargs);

  /* Version info */
    nargs = 0;
    XtSetArg(args[nargs],XmNalignment,XmALIGNMENT_BEGINNING); nargs++;
    XtSetArg(args[nargs],XmNlabelString,versionInfoXmString); nargs++;
    XtSetArg(args[nargs],XmNtopAttachment,XmATTACH_WIDGET); nargs++;
    XtSetArg(args[nargs],XmNtopWidget,descriptionLabel); nargs++;
    XtSetArg(args[nargs],XmNleftAttachment,XmATTACH_OPPOSITE_WIDGET); nargs++;
    XtSetArg(args[nargs],XmNleftWidget,descriptionLabel); nargs++;
	XtSetArg(args[nargs],XmNrightAttachment,XmATTACH_POSITION); nargs++;
	XtSetArg(args[nargs],XmNrightPosition,90); nargs++;
    if(otherFontList != NULL) {
	XtSetArg(args[nargs],XmNfontList,otherFontList); nargs++;
    }
    if(background >= 0) {
	XtSetArg(args[nargs],XmNbackground,(unsigned long)background); nargs++; }
    if(foreground >= 0) {
	XtSetArg(args[nargs],XmNforeground,(unsigned long)foreground); nargs++; }
    versionInfoLabel = XmCreateLabel(form,"versionInfoLabel",args,nargs);

  /* Developed at/by... */
    nargs = 0;
    XtSetArg(args[nargs],XmNalignment,XmALIGNMENT_BEGINNING); nargs++;
    XtSetArg(args[nargs],XmNlabelString,developedAtXmString); nargs++;
    XtSetArg(args[nargs],XmNtopAttachment,XmATTACH_WIDGET); nargs++;
    XtSetArg(args[nargs],XmNtopWidget,versionInfoLabel); nargs++;
    XtSetArg(args[nargs],XmNleftAttachment,XmATTACH_OPPOSITE_WIDGET); nargs++;
    XtSetArg(args[nargs],XmNleftWidget,versionInfoLabel); nargs++;
	XtSetArg(args[nargs],XmNrightAttachment,XmATTACH_POSITION); nargs++;
	XtSetArg(args[nargs],XmNrightPosition,90); nargs++;
    XtSetArg(args[nargs],XmNbottomAttachment,XmATTACH_POSITION); nargs++;
    XtSetArg(args[nargs],XmNbottomPosition,90); nargs++;

    if(otherFontList != NULL) {
	XtSetArg(args[nargs],XmNfontList,otherFontList); nargs++;
    }
    if(background >= 0) {
	XtSetArg(args[nargs],XmNbackground,(unsigned long)background); nargs++; }
    if(foreground >= 0) {
	XtSetArg(args[nargs],XmNforeground,(unsigned long)foreground); nargs++; }
    developedAtLabel = XmCreateLabel(form,"developedAtLabel",args,nargs);


  /* OK button */
    okXmString = XmStringCreateLocalized("OK");
    nargs = 0;
    XtSetArg(args[nargs],XmNlabelString,okXmString); nargs++;
    XtSetArg(args[nargs],XmNtopAttachment,XmATTACH_FORM); nargs++;
    XtSetArg(args[nargs],XmNtopOffset,8); nargs++;
    XtSetArg(args[nargs],XmNrightAttachment,XmATTACH_FORM); nargs++;
    XtSetArg(args[nargs],XmNrightOffset,8); nargs++;
    if(otherFontList != NULL) {
	XtSetArg(args[nargs],XmNfontList,otherFontList); nargs++;
    }
    if(background >= 0) {
	XtSetArg(args[nargs],XmNbackground,(unsigned long)background); nargs++; }
    if(foreground >= 0) {
	XtSetArg(args[nargs],XmNforeground,(unsigned long)foreground); nargs++; }
    okButton = XmCreatePushButton(form,"okButton",args,nargs);
    XtAddCallback(okButton,XmNactivateCallback,
      (XtCallbackProc)closeProductDescriptionCallback2,
      (XtPointer)productDescriptionShell); nargs++;
    XmStringFree(okXmString);

    children[0] = nameLabel; children[1] = descriptionLabel;
    children[2] = versionInfoLabel; children[3] = developedAtLabel;
    children[4] = separator;
    XtManageChildren(children,5);
    XtManageChild(form);

    XtPopup(productDescriptionShell,XtGrabNone);

  /* Center nameLabel vertically in form space */
    XtSetArg(args[0],XmNheight,&nameHeight);
    XtGetValues(nameLabel,args,1);
    XtSetArg(args[0],XmNheight,&formHeight);
    XtGetValues(form,args,1);
    offset = (formHeight - nameHeight);
    offset = offset/2;
    XtSetArg(args[0],XmNtopOffset,offset);
    XtSetValues(nameLabel,args,1);

  /* Center the whole thing on the screen */
    screenHeight=DisplayHeight(display,screen);
    screenWidth=DisplayWidth(display,screen);

    nargs=0;
    XtSetArg(args[nargs],XmNheight,&shellHeight); nargs++;
    XtSetArg(args[nargs],XmNwidth,&shellWidth); nargs++;
    XtGetValues(productDescriptionShell,args,nargs);

    newY=(screenHeight-shellHeight)/2;
    newX=(screenWidth-shellWidth)/2;

    nargs=0;
    XtSetArg(args[nargs],XmNy,newY); nargs++;
    XtSetArg(args[nargs],XmNx,newX); nargs++;
    XtSetValues(productDescriptionShell,args,nargs);

#if DEBUG_POSITION
    {
	Position newx, newy;

	printf("createAndPopupProductDescriptionShell:\n");
	printf("  sizeof(XtArgVal)=%d sizeof(Position)=%d "
	  "sizeof(Dimension)=%d sizeof(100)=%d\n",
	  sizeof(XtArgVal),sizeof(Position),sizeof(Dimension),sizeof(100));
	printf("  shellHeight=%d  shellWidth=%d\n",shellHeight,shellWidth);
	printf("  screenHeight=%d  screenWidth=%d\n",screenHeight,screenWidth);
	printf("  newY=%hd  newX=%hd\n",newY,newX);
	printf("  newY=%hx  newX=%hx\n",newY,newX);
	printf("  newY=%x  newX=%x\n",newY,newX);

	printf("(1) args[0].value=%4x  args[1].value=%4x\n",
	  args[0].value,args[1].value);

	nargs=0;
	XtSetArg(args[nargs],XmNy,&newy); nargs++;
	XtSetArg(args[nargs],XmNx,&newx); nargs++;
	XtGetValues(productDescriptionShell,args,nargs);

	printf("(1) newy=%d  newx=%d\n",newy,newx);

	nargs=0;
	XtSetArg(args[nargs],XmNy,474); nargs++;
	XtSetArg(args[nargs],XmNx,440); nargs++;
	XtSetValues(productDescriptionShell,args,nargs);

	printf("(2) args[0].value=%4x  args[1].value=%4x\n",
	  args[0].value,args[1].value);

	nargs=0;
	XtSetArg(args[nargs],XmNy,&newy); nargs++;
	XtSetArg(args[nargs],XmNx,&newx); nargs++;
	XtGetValues(productDescriptionShell,args,nargs);

	printf("(2) newy=%d  newx=%d\n",newy,newx);

	nargs=0;
	XtSetArg(args[nargs],XmNy,newY); nargs++;
	XtSetArg(args[nargs],XmNx,newX); nargs++;
	XtSetValues(productDescriptionShell,args,nargs);

	printf("(3) args[0].value=%4x  args[1].value=%4x\n",
	  args[0].value,args[1].value);

	nargs=0;
	XtSetArg(args[nargs],XmNy,&newy); nargs++;
	XtSetArg(args[nargs],XmNx,&newx); nargs++;
	XtGetValues(productDescriptionShell,args,nargs);

	printf("(3) newy=%d  newx=%d\n",newy,newx);

    }
#endif

  /* Free strings */
    if(nameXmString != (XmString)NULL) XmStringFree(nameXmString);
    if(descriptionXmString != (XmString)NULL)
      XmStringFree(descriptionXmString);
    if(versionInfoXmString != (XmString)NULL)
      XmStringFree(versionInfoXmString);
    if(developedAtXmString != (XmString)NULL)
      XmStringFree(developedAtXmString);

  /* Register timeout procedure to make the dialog go away after N
     seconds */
    XtAppAddTimeOut(appContext,(unsigned long)(1000*seconds),
      (XtTimerCallbackProc)popdownProductDescriptionShell,
      (XtPointer)productDescriptionShell);

    return(productDescriptionShell);
}

/* Window manager close callback to be used the first time */
static void closeProductDescriptionCallback1(Widget w,
  XtPointer clientData, XtPointer callData)
{
    popdownProductDescriptionShell(clientData);
}

/* Window manager close callback to be used thereafter */
static void closeProductDescriptionCallback2(Widget w,
  XtPointer clientData, XtPointer callData)
{
    Widget shell = (Widget)clientData;
    XtPopdown(shell);
}

static void popdownProductDescriptionShell(XtPointer xtPointer)
{
    Arg args[3];
    Widget widget;
    Cardinal nargs;

    widget = (Widget)xtPointer;
    XtPopdown(widget);

    nargs=0;
    XtSetArg(args[nargs],XmNdeleteResponse,XmDO_NOTHING); nargs++;
#if OMIT_RESIZE_HANDLES
    XtSetArg(args[nargs],XmNmwmDecorations,MWM_DECOR_ALL|MWM_DECOR_RESIZEH);
    nargs++;
  /* KE: The following is necessary for Exceed, which turns off the
     resize function with the handles.  It should not be necessary */
    XtSetArg(args[nargs],XmNmwmFunctions,MWM_FUNC_ALL); nargs++;
#endif
    XtSetValues(widget,args,nargs);

  /* Set the window manager close callback  to the first callback */
    XmRemoveWMProtocolCallback(widget,WM_DELETE_WINDOW,
      (XtCallbackProc)closeProductDescriptionCallback1,(XtPointer)widget);
    XmAddWMProtocolCallback(widget,WM_DELETE_WINDOW,
      (XtCallbackProc)closeProductDescriptionCallback2,(XtPointer)widget);

  /* Next time up the OK button will show */
    XtManageChild(okButton);
}

#ifdef TEST_PRODUCT_DESCRIPTION_SHELL
/*************************************************************************/

main(int argc, char **argv)
{
    Widget topLevel, shell;
    XtAppContext appContext;

    topLevel = XtAppInitialize(&appContext, "TEST", NULL, 0, &argc, argv,
      fallbackResources, NULL, 0);
    XmRegisterConverters();
    shell = createAndPopupProductDescriptionShell(appContext,topLevel,
      "MEDM", NULL,(Pixmap)NULL,
      "Motif-based Editor & Display Manager", NULL,
      "Version "VERSION,
      "developed at Argonne National Laboratory, by Mark Anderson", NULL,
      -1, -1, 3);


    XtRealizeWidget(topLevel);
    XtAppMainLoop(appContext);

}
#endif /* TEST */
