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

static void closeProductDescriptionCallback(Widget w,
  XtPointer client_data, XtPointer call_data);
static void popdownProductDescriptionShell(XtPointer xtPointer);

static void closeProductDescriptionCallback(Widget w,
  XtPointer client_data, XtPointer call_data)
{
    Widget shell = (Widget)client_data;
    XtPopdown(shell);
}

static void popdownProductDescriptionShell(XtPointer xtPointer)
{
    Arg args[3];
    Widget widget;
    Atom WM_DELETE_WINDOW;

    widget = (Widget) xtPointer;
    XtPopdown(widget);

    XtSetArg(args[0],XmNmwmDecorations,MWM_DECOR_ALL);
    XtSetArg(args[1],XmNdeleteResponse,XmDO_NOTHING);
    XtSetValues(widget,args,2);

    WM_DELETE_WINDOW = XmInternAtom(XtDisplay(widget),
      "WM_DELETE_WINDOW",False);
    XmAddWMProtocolCallback(widget,WM_DELETE_WINDOW,
      (XtCallbackProc)closeProductDescriptionCallback,(XtPointer)widget);

  /* Next time up the OK button will show */
    XtManageChild(okButton);
}


/*
 * function to create, set and popup an EPICS product description shell
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
    Arg args[15];
    Widget children[6], nameLabel, descriptionLabel, versionInfoLabel,
      separator, developedAtLabel;
    XmString nameXmString = (XmString)NULL, descriptionXmString = (XmString)NULL,
      versionInfoXmString = (XmString)NULL,
      developedAtXmString = (XmString)NULL, okXmString = (XmString)NULL;
    Dimension formHeight, nameHeight;
    Dimension shellHeight, shellWidth;
    Dimension screenHeight, screenWidth;
    Position newY, newX;
    int n, offset, screen;


  /* Create the shell */
    n = 0;
    if (background >= 0) {
	XtSetArg(args[n],XmNbackground,(unsigned long)background); n++;
    }
    if (foreground >= 0) {
	XtSetArg(args[n],XmNforeground,(unsigned long)foreground); n++;
    }
    XtSetArg(args[n],XmNmwmDecorations, MWM_DECOR_ALL|
      MWM_DECOR_BORDER|MWM_DECOR_RESIZEH|MWM_DECOR_TITLE|MWM_DECOR_MENU|
      MWM_DECOR_MINIMIZE|MWM_DECOR_MAXIMIZE); n++;
    XtSetArg(args[n],XmNtitle,"Version"); n++;
    productDescriptionShell = XtCreatePopupShell("productDescriptionShell",
      topLevelShellWidgetClass,topLevelShell,args, n);
    display=XtDisplay(productDescriptionShell);
    screen=DefaultScreen(display);
      
    n = 0;
    if (background >= 0) {
	XtSetArg(args[n],XmNbackground,(unsigned long)background); n++; }
    if (foreground >= 0) {
	XtSetArg(args[n],XmNforeground,(unsigned long)foreground); n++; }
    XtSetArg(args[n],XmNnoResize,True); n++;
    XtSetArg(args[n],XmNshadowThickness,2); n++;
    XtSetArg(args[n],XmNshadowType,XmSHADOW_OUT); n++;
    XtSetArg(args[n],XmNautoUnmanage,False); n++;
    form = XmCreateForm(productDescriptionShell,"form",args,n);
    
  /* Generate XmStrings */
    if (name != NULL) nameXmString = XmStringCreateLtoR(name,
      XmFONTLIST_DEFAULT_TAG);
    if (description != NULL) descriptionXmString =
			       XmStringCreateLtoR(description,XmFONTLIST_DEFAULT_TAG);
    if (versionInfo != NULL) versionInfoXmString =
			       XmStringCreateLtoR(versionInfo,XmFONTLIST_DEFAULT_TAG);
    if (developedAt != NULL) developedAtXmString =
			       XmStringCreateLtoR(developedAt,XmFONTLIST_DEFAULT_TAG);

  /* Create the label children  */
  /* Name */
    n = 0;
    if (namePixmap == (Pixmap) NULL) {
	XtSetArg(args[n],XmNlabelString,nameXmString); n++;
	if (nameFontList != NULL) {
	    XtSetArg(args[n],XmNfontList,nameFontList); n++;
	}
    } else {
	XtSetArg(args[n],XmNlabelType,XmPIXMAP); n++;
	XtSetArg(args[n],XmNlabelPixmap,namePixmap); n++;
    }
    XtSetArg(args[n],XmNalignment,XmALIGNMENT_BEGINNING); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNleftPosition,1); n++;
    XtSetArg(args[n],XmNresizable,False); n++;
    if (background >= 0) {
	XtSetArg(args[n],XmNbackground,(unsigned long)background); n++; }
    if (foreground >= 0) {
	XtSetArg(args[n],XmNforeground,(unsigned long)foreground); n++; }
    nameLabel = XmCreateLabel(form,"nameLabel",args,n);
    
    
  /* Separator */
    n = 0;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNleftWidget,nameLabel); n++;
    XtSetArg(args[n],XmNorientation,XmVERTICAL); n++;
    XtSetArg(args[n],XmNshadowThickness,2); n++;
    XtSetArg(args[n],XmNseparatorType,XmSHADOW_ETCHED_IN); n++;
    if (background >= 0) {
	XtSetArg(args[n],XmNbackground,(unsigned long)background); n++; }
    if (foreground >= 0) {
	XtSetArg(args[n],XmNforeground,(unsigned long)foreground); n++; }
    separator = XmCreateSeparator(form,"separator",args,n);
    
  /* Description */
    n = 0;
    XtSetArg(args[n],XmNalignment,XmALIGNMENT_BEGINNING); n++;
    XtSetArg(args[n],XmNlabelString,descriptionXmString); n++;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNtopPosition,5); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNleftWidget,separator); n++;
	XtSetArg(args[n],XmNrightAttachment,XmATTACH_POSITION); n++;
	XtSetArg(args[n],XmNrightPosition,90); n++;
    if (descriptionFontList != NULL) {
	XtSetArg(args[n],XmNfontList,descriptionFontList); n++;
    }
    if (background >= 0) {
	XtSetArg(args[n],XmNbackground,(unsigned long)background); n++; }
    if (foreground >= 0) {
	XtSetArg(args[n],XmNforeground,(unsigned long)foreground); n++; }
    descriptionLabel = XmCreateLabel(form,"descriptionLabel",args,n);
    
  /* Version info */
    n = 0;
    XtSetArg(args[n],XmNalignment,XmALIGNMENT_BEGINNING); n++;
    XtSetArg(args[n],XmNlabelString,versionInfoXmString); n++;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,descriptionLabel); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg(args[n],XmNleftWidget,descriptionLabel); n++;
	XtSetArg(args[n],XmNrightAttachment,XmATTACH_POSITION); n++;
	XtSetArg(args[n],XmNrightPosition,90); n++;
    if (otherFontList != NULL) {
	XtSetArg(args[n],XmNfontList,otherFontList); n++;
    }
    if (background >= 0) {
	XtSetArg(args[n],XmNbackground,(unsigned long)background); n++; }
    if (foreground >= 0) {
	XtSetArg(args[n],XmNforeground,(unsigned long)foreground); n++; }
    versionInfoLabel = XmCreateLabel(form,"versionInfoLabel",args,n);
    
  /* Developed at/by... */
    n = 0;
    XtSetArg(args[n],XmNalignment,XmALIGNMENT_BEGINNING); n++;
    XtSetArg(args[n],XmNlabelString,developedAtXmString); n++;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_WIDGET); n++;
    XtSetArg(args[n],XmNtopWidget,versionInfoLabel); n++;
    XtSetArg(args[n],XmNleftAttachment,XmATTACH_OPPOSITE_WIDGET); n++;
    XtSetArg(args[n],XmNleftWidget,versionInfoLabel); n++;
	XtSetArg(args[n],XmNrightAttachment,XmATTACH_POSITION); n++;
	XtSetArg(args[n],XmNrightPosition,90); n++;
    XtSetArg(args[n],XmNbottomAttachment,XmATTACH_POSITION); n++;
    XtSetArg(args[n],XmNbottomPosition,90); n++;
    
    if (otherFontList != NULL) {
	XtSetArg(args[n],XmNfontList,otherFontList); n++;
    }
    if (background >= 0) {
	XtSetArg(args[n],XmNbackground,(unsigned long)background); n++; }
    if (foreground >= 0) {
	XtSetArg(args[n],XmNforeground,(unsigned long)foreground); n++; }
    developedAtLabel = XmCreateLabel(form,"developedAtLabel",args,n);
    
    
  /* OK button */
    okXmString = XmStringCreateLocalized("OK");
    n = 0;
    XtSetArg(args[n],XmNlabelString,okXmString); n++;
    XtSetArg(args[n],XmNtopAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNtopOffset,8); n++;
    XtSetArg(args[n],XmNrightAttachment,XmATTACH_FORM); n++;
    XtSetArg(args[n],XmNrightOffset,8); n++;
    if (otherFontList != NULL) {
	XtSetArg(args[n],XmNfontList,otherFontList); n++;
    }
    if (background >= 0) {
	XtSetArg(args[n],XmNbackground,(unsigned long)background); n++; }
    if (foreground >= 0) {
	XtSetArg(args[n],XmNforeground,(unsigned long)foreground); n++; }
    okButton = XmCreatePushButton(form,"okButton",args,n);
    XtAddCallback(okButton,XmNactivateCallback,
      (XtCallbackProc)closeProductDescriptionCallback,
      (XtPointer)productDescriptionShell); n++;
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

    n=0;
    XtSetArg(args[n],XmNheight,&shellHeight); n++;
    XtSetArg(args[n],XmNwidth,&shellWidth); n++;
    XtGetValues(productDescriptionShell,args,n);
    
    newY=(screenHeight-shellHeight)/2;
    newX=(screenWidth-shellWidth)/2;
    
    n=0;
    XtSetArg(args[n],XmNy,newY); n++;
    XtSetArg(args[n],XmNx,newX); n++;
    XtSetValues(productDescriptionShell,args,n);
    
#ifdef WIN32
  /* Seems to be an Exceed bug that it doesn't get set the first time */
    n=0;
    XtSetArg(args[n],XmNy,newY); n++;
    XtSetArg(args[n],XmNx,newX); n++;
    XtSetValues(productDescriptionShell,args,n);
#endif    
    
#if DEBUG_POSITION
    {
	Position newx, newy;

	printf("createAndPopupProductDescriptionShell:\n");
	printf("  sizeof(XtArgVal)=%d sizeof(Position)=%d sizeof(Dimension)=%d sizeof(100)=%d\n",
	  sizeof(XtArgVal),sizeof(Position),sizeof(Dimension),sizeof(100));
	printf("  shellHeight=%d  shellWidth=%d\n",shellHeight,shellWidth);
	printf("  screenHeight=%d  screenWidth=%d\n",screenHeight,screenWidth);
	printf("  newY=%hd  newX=%hd\n",newY,newX);
	printf("  newY=%hx  newX=%hx\n",newY,newX);
	printf("  newY=%x  newX=%x\n",newY,newX);

	printf("(1) args[0].value=%4x  args[1].value=%4x\n",args[0].value,args[1].value);

	n=0;
	XtSetArg(args[n],XmNy,&newy); n++;
	XtSetArg(args[n],XmNx,&newx); n++;
	XtGetValues(productDescriptionShell,args,n);
	
	printf("(1) newy=%d  newx=%d\n",newy,newx);

	n=0;
	XtSetArg(args[n],XmNy,474); n++;
	XtSetArg(args[n],XmNx,440); n++;
	XtSetValues(productDescriptionShell,args,n);
	
	printf("(2) args[0].value=%4x  args[1].value=%4x\n",args[0].value,args[1].value);

	n=0;
	XtSetArg(args[n],XmNy,&newy); n++;
	XtSetArg(args[n],XmNx,&newx); n++;
	XtGetValues(productDescriptionShell,args,n);
	
	printf("(2) newy=%d  newx=%d\n",newy,newx);

	n=0;
	XtSetArg(args[n],XmNy,newY); n++;
	XtSetArg(args[n],XmNx,newX); n++;
	XtSetValues(productDescriptionShell,args,n);
	
	printf("(3) args[0].value=%4x  args[1].value=%4x\n",args[0].value,args[1].value);

	n=0;
	XtSetArg(args[n],XmNy,&newy); n++;
	XtSetArg(args[n],XmNx,&newx); n++;
	XtGetValues(productDescriptionShell,args,n);
	
	printf("(3) newy=%d  newx=%d\n",newy,newx);
	
    }
#endif    
    
  /* Free strings */
    if (nameXmString != (XmString)NULL) XmStringFree(nameXmString);
    if (descriptionXmString != (XmString)NULL) XmStringFree(descriptionXmString);
    if (versionInfoXmString != (XmString)NULL) XmStringFree(versionInfoXmString);
    if (developedAtXmString != (XmString)NULL) XmStringFree(developedAtXmString);
    
  /* Register timeout procedure to make the dialog go away after N seconds */
    XtAppAddTimeOut(appContext,(unsigned long)(1000*seconds),
      (XtTimerCallbackProc)popdownProductDescriptionShell,
      (XtPointer)productDescriptionShell);
    
    return(productDescriptionShell);
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
