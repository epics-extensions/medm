/**
 ** toplevel Strip Chart Plotter test program
 **/


#include <stdio.h>
#include <math.h>


#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>
#include <X11/Shell.h>

#include <Xm/Xm.h>
#include <Xm/DrawingA.h>

#include <Xm/PushB.h>
#include <Xm/BulletinB.h>
#include <Xm/MessageB.h>
#include <Xm/SelectioB.h>
#include <Xm/Scale.h>
#include <Xm/RowColumn.h>
#include <Xm/List.h>

#ifndef max
#define max(a,b) ((a)>(b)?(a):(b))
#endif


#include "GraphX.h"             /* overall GraphX include file
                                   defines data types, plot types,
                                   functions and macros            */



#define WIDTH   384
#define HEIGHT  384

#define NCHANNELS 3
#define NPTS 100 

#define SCALE_WIDTH 350
#define SCALE_HEIGHT 100
#define COLOR_WIDTH 200
#define COLOR_RC_HEIGHT 250
#define COLOR_LIST_HEIGHT 100
#define COLOR_HEIGHT 	COLOR_RC_HEIGHT+COLOR_LIST_HEIGHT
#define MIN_FREQUENCY 1
#define MAX_FREQUENCY 10

#define BAD_CHANNEL -1




static double updateFrequency = 1.0;
static int globalChannelNumber = BAD_CHANNEL;
static Strip *globalStrip;
static Boolean globalPause = FALSE;
#define NCOLORS 9
static char *color[] = {"gray", "sandybrown", "green yellow", "cadet blue",
                        "light steel blue", "pink",
                        "goldenrod", "orange", "violet red",};
static StripRange stripRange[NCHANNELS];


/* 
 * TRANSLATIONS
 */
static void printPlot(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{

  stripPrint(globalStrip);

}



static void quit(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
/*
 * clean up our space like a good program
 */
  stripTerm(globalStrip);

  exit(0);
}


static void togglePause(w,event,params,num_params)
  Widget w;
  XEvent *event;
  String *params;
  int    num_params;
{
  globalPause = ~globalPause;

  if (globalPause)
	stripPause(globalStrip);
   else
	stripResume(globalStrip);
}



/*
 * CALLBACKS 
 */

static void redisplayStrip(w,strip,call_data)
  Widget w;
  Strip **strip;	/* double indirection since filling in actual */
  XtPointer call_data;	/* address of Strip structure after XtAddCallback */
{

  stripRefresh(*strip);	/* copy pixmap onto window (this is quick) */	

}


static void resizeStrip(w,strip,call_data)
  Widget w;
  Strip **strip;	/* double indirection since filling in actual */
  XtPointer call_data;	/* address of Strip structure after XtAddCallback */
{

/* 
 * widget/window changed: reset sizes, free & create new pixmap
 * update this new pixmap and map to window
 */
  stripResize(*strip);
  stripDraw(*strip);

}


static void colorSelect(w,colorString,call_data)
  Widget w;
  char *colorString;
  XtPointer *call_data;
{


if (globalChannelNumber > BAD_CHANNEL)
    stripSetDataColor(globalStrip,globalChannelNumber,colorString);

}



static void frequencyChanged(w,frequencyPtr,call_data)
  Widget w;
  double *frequencyPtr;
  XmScaleCallbackStruct *call_data;
{

*frequencyPtr = (double) call_data->value;  /* set passed in address */

/* since frequency is inverse of time interval... */
stripSetInterval(globalStrip, 1.0/((double) call_data->value) );

}


static void channelSelect(w,client_data,call_data)
  Widget w;
  XtPointer *client_data;
  XmListCallbackStruct *call_data;
{

char *text;
int channelNumber;

XmStringGetLtoR(call_data->item,XmSTRING_DEFAULT_CHARSET,&text);

sscanf(text,"%d",&channelNumber);
globalChannelNumber = channelNumber;


}




/*
 * support routine to get normal string from XmString
 *  borrowed from motifgif code
 */
char *extractNormalString(cs)
  XmString cs;
{

  XmStringContext context;
  XmStringCharSet charset;
  XmStringDirection direction;
  Boolean separator;
  static char *primitive_string;

  XmStringInitContext (&context,cs);
  XmStringGetNextSegment (context,&primitive_string,
                          &charset,&direction,&separator);
  XmStringFreeContext (context);
  return ((char *) primitive_string);
}



/*
 * even though we have registered different callback functions for
 *   each channel in the strip chart, we could use the 
 *   channelNumber and userData arguments for something...
 */


static double getChannel1ForStripChart(channelNumber, userData)
  int channelNumber;
  XtPointer userData;
{
#define DELTA 0.2
static double staticData = -10.0;
static int direction = 1;
/***
return ((double) 5.0 + 2.0*(random()/pow(2.0,31.0)) );
 ***/
if (staticData >= 10.0) direction = -1;		/* now go downhill */
if (staticData <= -10.0) direction = 1;		/* now go uphill */

if (direction == 1) {
  staticData += DELTA;
} else {
  staticData -= DELTA; }
return (staticData);

}

static double getChannel2ForStripChart(channelNumber, userData)
  int channelNumber;
  XtPointer userData;
{

return ((double)  3.0*(random()/pow(2.0,31.0)) - 3.0*(random()/pow(2.0,31.0)) );

}


static double getChannel3ForStripChart(channelNumber, userData)
  int channelNumber;
  XtPointer userData;
{

return ((double) -5.0 - 2.0*(random()/pow(2.0,31.0)) );

}


/* function to call to get next value for strip chart */
static double (*getChannelValues[])() = { getChannel1ForStripChart,
                                        getChannel2ForStripChart,
                                        getChannel3ForStripChart,};


static char defaultTranslations[] = 
		"<Key>q: quit() \n\
		 Ctrl<Key>p: printPlot() \n\
		 <Key>p: togglePause()";

static XtActionsRec actionsTable[] = {
	{"quit", (XtActionProc)quit},
	{"printPlot", (XtActionProc)printPlot},
	{"togglePause", (XtActionProc)togglePause},
};



main(argc,argv)
  int   argc;
  char *argv[];
{
  Widget topLevel, drawingArea, colorDialog, scaleDialog, frequencyScale;
  Widget colorButton[NCOLORS], colorRC, channelList;
  Arg args[15];
  int n;

  XtTranslations transTable;
  XmString colorItem[NCOLORS];
  XmString channelStrings[NCHANNELS];
  XmString xmStringData;

  int i, j;
  double *data[NCHANNELS];
  unsigned long forePixel, backPixel;
  char *foregroundColor = "white";
  char *backgroundColor= "black";

  char channelName[20];


  XColor xColor, ignore;

  unsigned long colorPixel[NCOLORS];

  char titleString[40];
  char string[40];

  Display *display = NULL;
  int screen;
  Window window1;

/* these are filled in by graphXGetBestFont() calls */
  char *titleFont, *axesFont;
  int   titleFontSize, axesFontSize;


/* 
 * first let's initialize our data buffers (just in case we really care
 *  about looking at the data the strip chart is getting/using...
 *  (probably skip this in most instances)
 */
  for (i = 0; i < NCHANNELS; i++)
     data[i] = (double *) malloc((unsigned) NPTS * sizeof(double));



   topLevel = XtInitialize(argv[0], "Strip Plotter", NULL, 0, &argc, argv);

/* could set the icon pixmap after Xt and  is initialized
#include "StripChart.bit"
   XtSetArg(args[0],XtNiconPixmap,
		XCreateBitmapFromData(XtDisplay(),
			XtScreen()->root,
			StripChart_bits,
			StripChart_width, StripChart_height));
   XtSetValues(,args,1);
 */




/*
 * Register new actions and compile translations table
 */
   XtAddActions(actionsTable,XtNumber(actionsTable));
   transTable = XtParseTranslationTable(defaultTranslations);


/*
 * setup a drawingArea widget to draw in
 */
   n = 0;
   XtSetArg(args[n], XmNwidth, WIDTH); n++;
   XtSetArg(args[n], XmNheight, HEIGHT); n++;

   drawingArea = XmCreateDrawingArea(topLevel,"drawingArea",args,n);
   XtManageChild(drawingArea);


/*
 * merge program-defined translations with existing translations
 */
   XtOverrideTranslations(drawingArea,transTable);
   XtOverrideTranslations(topLevel,transTable);

/*
 * add the expose and resize callbacks, passing pointer to strip as the data of 
 *   the callback
 */
   XtAddCallback(drawingArea, XmNexposeCallback,
	(XtCallbackProc)redisplayStrip, &globalStrip);
   XtAddCallback(drawingArea, XmNresizeCallback,
	(XtCallbackProc)resizeStrip, &globalStrip);



/*
 *  need to realize the widgets for windows to be created...
 */

   XtRealizeWidget(topLevel);

   display = XtDisplay(topLevel);
   screen  = DefaultScreen(display);
   XtCreateWindow(drawingArea,InputOutput,CopyFromParent,0L,0);
   window1  = XtWindow(drawingArea);


/*
 * if a color monitor, see what color to do the data in
 */

/******
   if (DisplayCells(display,screen) > 2) {
 ******/
   if (FALSE) {

     n = 0;
     XtSetArg(args[n], XtNwidth, COLOR_WIDTH); n++;
     XtSetArg(args[n], XtNheight, COLOR_HEIGHT); n++;
     xmStringData = XmStringCreateLtoR("data color", XmSTRING_DEFAULT_CHARSET);
     XtSetArg(args[n], XmNdialogTitle, xmStringData); n++;
     colorDialog = XmCreateBulletinBoardDialog(topLevel,"colorDialog",args,n);

     n = 0;
     XtSetArg(args[n], XtNwidth, COLOR_WIDTH); n++;
     XtSetArg(args[n], XtNheight, COLOR_RC_HEIGHT); n++;
     colorRC = XmCreateRowColumn(colorDialog,"colorRC",args,n);
     XtManageChild(colorRC);

     for (i=0; i<NCOLORS; i++) {

       if(!XAllocNamedColor(display,
          DefaultColormap(display,screen),color[i],&xColor,&ignore) ) {
          printf("\n>>  couldn't allocate color %s",color[i]);
          colorPixel[i] = WhitePixel(display,screen);
       } else {
          colorPixel[i] = xColor.pixel;
       }

       colorItem[i] = XmStringCreateLtoR(color[i],XmSTRING_DEFAULT_CHARSET);
       n = 0;
       XtSetArg(args[n], XmNforeground, BlackPixel(display,screen)); n++;
       XtSetArg(args[n], XmNbackground, colorPixel[i]); n++;
       XtSetArg(args[n], XmNlabelString, colorItem[i]); n++;
       XtSetArg(args[n], XmNshadowThickness, 2); n++;
       colorButton[i] = XmCreatePushButton(colorRC,color[i],args,n);
       XtAddCallback(colorButton[i],XmNactivateCallback,
		(XtCallbackProc)colorSelect,color[i]);
     }
     XtManageChildren(colorButton,NCOLORS);
     XtManageChild(colorDialog);


     for (n = 0; n < NCHANNELS; n++) {
	sprintf(channelName,"%d -- Channel # %d",n,n);
	channelStrings[n] = XmStringCreate(channelName,
		XmSTRING_DEFAULT_CHARSET);
     }
     n = 0;
     XtSetArg(args[n], XmNx, 0); n++;
     XtSetArg(args[n], XmNy, COLOR_RC_HEIGHT); n++;
     XtSetArg(args[n], XmNwidth, COLOR_WIDTH); n++;
     XtSetArg(args[n], XmNheight, COLOR_LIST_HEIGHT); n++;
     XtSetArg(args[n], XmNitems, channelStrings); n++;
     XtSetArg(args[n], XmNitemCount, NCHANNELS); n++;
     XtSetArg(args[n], XmNvisibleItemCount, NCHANNELS); n++; 
     channelList = XtCreateManagedWidget("channelList",xmListWidgetClass,
		colorDialog, args, n);
     XtAddCallback(channelList,XmNbrowseSelectionCallback,
		(XtCallbackProc)channelSelect,NULL);


     for (i=0; i<NCOLORS; i++) {
       XmStringFree(colorItem[i]);
     }
   }


   n = 0;
   XtSetArg(args[n], XtNwidth, SCALE_WIDTH); n++;
   XtSetArg(args[n], XtNheight, SCALE_HEIGHT); n++;
   xmStringData = XmStringCreateLtoR("frequency", XmSTRING_DEFAULT_CHARSET);
   XtSetArg(args[n], XmNdialogTitle, xmStringData); n++;
   scaleDialog = XmCreateBulletinBoardDialog(topLevel,"scaleDialog",args,n);

   n = 0;
   XtSetArg(args[n], XmNx, 0); n++;
   XtSetArg(args[n], XmNy, 0); n++;
   XtSetArg(args[n], XmNwidth, SCALE_WIDTH); n++;
   XtSetArg(args[n], XmNheight, SCALE_HEIGHT); n++;
   XtSetArg(args[n],XmNmaximum, MAX_FREQUENCY); n++;
   XtSetArg(args[n],XmNminimum, MIN_FREQUENCY); n++;
   XtSetArg(args[n],XmNorientation, XmHORIZONTAL); n++;
   XtSetArg(args[n],XmNprocessingDirection, XmMAX_ON_RIGHT); n++;
   xmStringData = XmStringCreateLtoR("Sampling Frequency (Hz)",
                XmSTRING_DEFAULT_CHARSET);
   XtSetArg(args[n],XmNtitleString,xmStringData); n++;
   XtSetArg(args[n],XmNshowValue, TRUE); n++;
   XtSetArg(args[n],XmNvalue,( (int) updateFrequency) ); n++;
   frequencyScale = XtCreateManagedWidget("frequencyScale",
                xmScaleWidgetClass, scaleDialog, args, n);
   XtAddCallback(frequencyScale,XmNvalueChangedCallback,
                (XtCallbackProc)frequencyChanged,&updateFrequency);
   XmStringFree(xmStringData);

   XtManageChild(scaleDialog);




/*
 * now proceed with strip generation
 */

  globalStrip = stripInit(display,screen,window1);

   titleFontSize = GraphX_TitleFontSize(globalStrip);
   titleFont = graphXGetBestFont(display,"times","bold","r", titleFontSize);
   axesFontSize = GraphX_AxesFontSize(globalStrip);
   axesFont = graphXGetBestFont(display,"times","medium","r",axesFontSize);


  /* setup the Strip, use the external-to-stripchart version of variables */

   stripRange[0].minVal = -10.0; stripRange[0].maxVal = 10.0;
   stripRange[1].minVal = -5.0; stripRange[1].maxVal = 5.0;
   stripRange[2].minVal = -8.0; stripRange[2].maxVal = 8.0;

   sprintf(titleString,"Time Axis (%d samples)",NPTS);
   stripSet(globalStrip, NCHANNELS, NPTS, data,
     stripRange, 1.0/updateFrequency, getChannelValues,
     "A StripChart", 
     titleFont,
     titleString, "Value Axis", 
     axesFont,
     foregroundColor, backgroundColor, &(color[6]), StripExternal);



   /* do the drawing so that expose event can map pixmap onto window */
   stripDraw(globalStrip);


   XtMainLoop();

}


