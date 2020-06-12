/************************************************************************\
 * Copyright (c) 2002 The University of Chicago, as Operator of Argonne
 * National Laboratory.
 * Copyright (c) 2002 The Regents of the University of California, as
 * Operator of Los Alamos National Laboratory.
 * This file is distributed subject to a Software License Agreement found
 * in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*****************************************************************************
 *
 *     Original Author : Mark Anderson
 *     Second Author   : Frederick Vong
 *     Third Author    : Kenneth Evans, Jr.
 *
 *****************************************************************************
 */

#define DEBUG_COUNT 0
#define DEBUG_RESET 0
#define DEBUG_LOSING_FOCUS 0
#define DEBUG_CARTESIAN_PLOT_BORDER 0
#define DEBUG_CARTESIAN_PLOT_UPDATE 0
#define DEBUG_TIME 07
#define DEBUG_XRT 0
#define DEBUG_HISTOGRAM 0
#define DEBUG_ERASE 0
#define DEBUG_ACCESS 0

#if defined(SCIPLOT)
#  define CP_COLS 5
#else
#  if defined(XRTGRAPH)
#    define CP_COLS 4
#  else
#    define CP_COLS 3
#  endif
#endif

/* These must start with 2, 0-1 are for option menu buttons.  If any
   of option menus have more values, 2 must change to accomodate the
   largest. */
#define CP_XDATA_BTN 2
#define CP_YDATA_BTN 3
#define CP_COLOR_BTN 4
#define CP_YAXIS_BTN 5
#define CP_YSIDE_BTN 6

#define CP_APPLY_BTN 7
#define CP_CLOSE_BTN 8
#define CP_X_BTN 9

#define CP_DEFAULT_CLR 4 /* grey */

#define SETHIGH(x) ((x) << 16)
#define GETHIGH(x) ((x) >> 16)
#define GETLOW(x) ((x)&0xFF)

/* Determines if the count from the PV is used. Does not determine if
 * the PV is implemented. It is implemented in any event. */
#define USECOUNTPV 1

#ifndef VMS
#  define CHECK_NAN
#endif

#ifdef CHECK_NAN
/* Note that for Solaris isnand and isnanf are defined in ieeefp.h.
 *  isnan is the same as isnand and is supposed to be defined in math.h
 *    but it isn't.
 * For Sun4 there is no isnand or isnanf and isnan is defined in math.h.
 * Consequently we use isnan and prototype it explicitly.
 * This should work on all systems and be safe.
 */
#  ifdef WIN32
#    include <float.h>
#    define isnan(a) _isnan(a) /* For some reason MS has leading _ */
#  else
extern int isnan(double); /* In case it's not in math.h as it should be */
#    include <math.h> /* Must follow extern statement in case isnan is a macro */
#  endif              /* WIN32 */
#  define NAN_SUBSTITUTE 0.0
#  define SAFEFLOAT(x) (safeFloat(x))
#else
#  define SAFEFLOAT(x) ((float)(x))
#endif /* End #ifdef CHECK_NAN */
/* If not MS Visual C++ or MS Visual C++ is 2010 or later */
#if !defined(_MSC_VER) || _MSC_VER >= 1600
#  include <stdint.h>
#endif

#include "medm.h"
#include <Xm/MwmUtil.h>

/* Include the appropriate header files for the plot package */
#if defined(XRTGRAPH)
#  include "medmXrtGraph.h"
#elif defined(SCIPLOT)
#  include "medmSciPlot.h"
#elif defined(JPT)
#  include "medmJpt.h"
#else
#  error No plot package to implement the Cartesian Plot has been defined.
#endif

#include <ctype.h>
#include "medmCartesianPlot.h"

/* Function prototypes */

static void cartesianPlotCreateRunTimeInstance(DisplayInfo *, DlElement *);
static void cartesianPlotCreateEditInstance(DisplayInfo *, DlElement *);
static void cartesianPlotUpdateGraphicalInfoCb(XtPointer cd);
static void cartesianPlotUpdateScreenFirstTime(XtPointer cd);
static void cartesianPlotDraw(XtPointer cd);
static void cartesianPlotUpdateValueCb(XtPointer cd);
static void cartesianPlotDestroyCb(XtPointer cd);
static void cartesianPlotGetRecord(XtPointer, Record **, int *);
static void cartesianPlotInheritValues(ResourceBundle *pRCB, DlElement *p);
static void cartesianPlotSetBackgroundColor(ResourceBundle *pRCB, DlElement *p);
static void cartesianPlotSetForegroundColor(ResourceBundle *pRCB, DlElement *p);
static void cartesianPlotGetValues(ResourceBundle *pRCB, DlElement *p);
static Boolean cartesianPlotResetPlot(MedmCartesianPlot *pcp);
static void cartesianPlotUpdateTrace(XtPointer cd, Boolean updateLastPoint);

static void cartesianPlotAxisActivate(Widget w, XtPointer cd, XtPointer cbs);
static void cartesianPlotDialogStoreTextEntries(void);
static void cartesianPlotDialogReset();

#if DEBUG_ERASE
static void dumpCartesianPlotData(const char *title,
                                  MedmCartesianPlot *pcp);
#endif

static DlDispatchTable cartesianPlotDlDispatchTable = {
  createDlCartesianPlot,
  NULL,
  executeDlCartesianPlot,
  hideDlCartesianPlot,
  writeDlCartesianPlot,
  NULL,
  cartesianPlotGetValues,
  cartesianPlotInheritValues,
  cartesianPlotSetBackgroundColor,
  cartesianPlotSetForegroundColor,
  genericMove,
  genericScale,
  genericOrient,
  NULL,
  NULL};

static String cpColumnLabels[] = {
  "X Data",
  "Y Data",
  "Color",
  "Y Axis",
  "Y Side",
};
static short cpColumnWidths[] = {
  36,
  36,
  6,
  6,
  6,
};
#if 0
static int cpColumnMaxLengths[] = {MAX_TOKEN_LENGTH-1,MAX_TOKEN_LENGTH-1,6,6,};
static unsigned char cpColumnLabelAlignments[] = {
  XmALIGNMENT_CENTER, XmALIGNMENT_CENTER, XmALIGNMENT_CENTER,  XmALIGNMENT_CENTER,};
#endif

static Widget table[MAX_TRACES][CP_COLS];
static DlTrace tempTrace[MAX_TRACES];

static Widget axisRangeMenu[5];                 /* X_AXIS_ELEMENT =0 */
static Widget axisStyleMenu[5];                 /* Y1_AXIS_ELEMENT=1 */
static Widget axisRangeMin[5], axisRangeMax[5]; /* Y2_AXIS_ELEMENT=2  Y3_AXIS_ELEMENT=3 Y4_AXIS_ELEMENT=4 */
static Widget axisRangeMinRC[5], axisRangeMaxRC[5];
static Widget axisTimeFormat;

char *cpTimeFormatString[NUM_CP_TIME_FORMAT] = {
  "%H:%M:%S",
  "%H:%M",
  "%H:00",
  "%b %d, %Y",
  "%b %d",
  "%b %d %H:00",
  "%a %H:00"};

#ifdef CHECK_NAN
float safeFloat(double x)
{
  static int nerrs = 0;
  static int ierrs = 0;
  static int print = 1;

  if (isnan(x))
    {
      if (print)
        {
          if (nerrs < 25)
            {
              nerrs++;
              medmPostMsg(0, "CartesianPlot: "
                          "Value is NaN, using %g\n",
                          NAN_SUBSTITUTE);
              if (nerrs >= 25)
                {
                  medmPrintf(0, "\nCartesianPlot: "
                             "Suppressing further NaN error messages\n");
                  print = 0;
                }
            }
        }
      return NAN_SUBSTITUTE;
    }
  else
    {
#  if defined(XRTGRAPH)
      /* Check for Inf and -Inf */
      /* XRTgraph hangs if plot point with value > FLT_MAX */
      /* XRTgraph does not plot point with value = FLT_MAX */
      /* XRTgraph hangs if plot point with value <= -FLT_MAX */
      if (x > FLT_MAX || x < -FLT_MAX)
        {
          if (print)
            {
              if (ierrs < 25)
                {
                  ierrs++;
                  medmPostMsg(0, "CartesianPlot: "
                              "Value is Inf, using %g\n",
                              FLT_MAX);
                  if (ierrs >= 25)
                    {
                      medmPrintf(0, "\nCartesianPlot: "
                                 "Suppressing further Inf error messages\n");
                      print = 0;
                    }
                }
            }
          return FLT_MAX;
        }
#  else
      /* SciPlot hangs if plot point with value > FLT_MAX */
      /* SciPlot has bad axis if plot point with value >= FLT_MAX */
      /* SciPlot does not plot point with value <= -FLT_MAX */
      if (x >= FLT_MAX || x < -FLT_MAX)
        {
          if (print)
            {
              if (ierrs < 25)
                {
                  ierrs++;
                  medmPostMsg(0, "CartesianPlot: "
                              "Value is Inf, skipping point.\n");
                  if (ierrs >= 25)
                    {
                      medmPrintf(0, "\nCartesianPlot: "
                                 "Suppressing further Inf error messages\n");
                      print = 0;
                    }
                }
            }
          return -FLT_MAX;
        }
#  endif
      return (float)x;
    }
}
#endif

static void cartesianPlotCreateRunTimeInstance(DisplayInfo *displayInfo,
                                               DlElement *dlElement)
{
  MedmCartesianPlot *pcp;
  int i, validTraces;
  int nTracesY1, nTracesY2, nTracesY3, nTracesY4;
  Widget localWidget;
  DlCartesianPlot *dlCartesianPlot = dlElement->structure.cartesianPlot;

  /* Allocate a MedmCartesianPlot and fill part of it in */
  if (dlElement->data)
    {
      pcp = (MedmCartesianPlot *)dlElement->data;
    }
  else
    {
      pcp = (MedmCartesianPlot *)malloc(sizeof(MedmCartesianPlot));
      dlElement->data = pcp;
      if (pcp == NULL)
        {
          medmPrintf(1, "\ncartesianPlotCreateRunTimeInstance:"
                     " Memory allocation error\n");
          return;
        }
      /* Pre-initialize */
      pcp->updateTask = NULL;
      pcp->nTraces = 0;
      pcp->nTracesY1 = 0;
      pcp->nTracesY2 = 0;
      pcp->nTracesY3 = 0;
      pcp->nTracesY4 = 0;
      pcp->nPoints = 0;
      pcp->hcp1 = pcp->hcp2 = pcp->hcp3 = pcp->hcp4 = NULL;
      pcp->dirty1 = pcp->dirty2 = pcp->dirty3 = pcp->dirty4 = False;
      pcp->timeScale = False;
      pcp->startTime.secPastEpoch = 0;
      pcp->startTime.nsec = 0;
      pcp->eraseMode = dlCartesianPlot->eraseMode;
      pcp->dlElement = dlElement;
      pcp->updateTask = updateTaskAddTask(displayInfo,
                                          &(dlCartesianPlot->object),
                                          cartesianPlotDraw,
                                          (XtPointer)pcp);
      if (pcp->updateTask == NULL)
        {
          medmPrintf(1, "\ncartesianPlotCreateRunTimeInstance: "
                     "Memory allocation error\n");
        }
      else
        {
          updateTaskAddDestroyCb(pcp->updateTask, cartesianPlotDestroyCb);
          updateTaskAddNameCb(pcp->updateTask, cartesianPlotGetRecord);
        }

      /* Allocate (or set to NULL) the Records in the xyTrace's XYTraces */
      /*   Note: Set the updateValueCb for the time being to
       *     cartesianPlotUpdateScreenFirstTime(), which will change it
       *     to cartesianPlotUpdateValueCb() when it is finished */
      validTraces = 0;
      nTracesY1 = 0;
      nTracesY2 = 0;
      nTracesY3 = 0;
      nTracesY4 = 0;
      for (i = 0; i < MAX_TRACES; i++)
        {
          Boolean validTrace = False;
          /* X data */
          if (dlCartesianPlot->trace[i].xdata[0] != '\0')
            {
              if (dlCartesianPlot->count == 0)
                {
                  pcp->xyTrace[validTraces].recordX =
                    medmAllocateRecordDynamicArrays(dlCartesianPlot->trace[i].xdata,
                                                    cartesianPlotUpdateScreenFirstTime,
                                                    cartesianPlotUpdateGraphicalInfoCb,
                                                    (XtPointer) & (pcp->xyTrace[validTraces]));
                }
              else
                {
                  pcp->xyTrace[validTraces].recordX =
                    medmAllocateRecord(dlCartesianPlot->trace[i].xdata,
                                       cartesianPlotUpdateScreenFirstTime,
                                       cartesianPlotUpdateGraphicalInfoCb,
                                       (XtPointer) & (pcp->xyTrace[validTraces]));
                }

              validTrace = True;
            }
          else
            {
              pcp->xyTrace[validTraces].recordX = NULL;
            }
          /* Y data */
          if (dlCartesianPlot->trace[i].ydata[0] != '\0')
            {
              if (dlCartesianPlot->count == 0)
                {
                  pcp->xyTrace[validTraces].recordY =
                    medmAllocateRecordDynamicArrays(dlCartesianPlot->trace[i].ydata,
                                                    cartesianPlotUpdateScreenFirstTime,
                                                    cartesianPlotUpdateGraphicalInfoCb,
                                                    (XtPointer) & (pcp->xyTrace[validTraces]));
                }
              else
                {
                  pcp->xyTrace[validTraces].recordY =
                    medmAllocateRecord(dlCartesianPlot->trace[i].ydata,
                                       cartesianPlotUpdateScreenFirstTime,
                                       cartesianPlotUpdateGraphicalInfoCb,
                                       (XtPointer) & (pcp->xyTrace[validTraces]));
                }
              validTrace = True;
            }
          else
            {
              pcp->xyTrace[validTraces].recordY = NULL;
            }
          if (validTrace)
            {
              pcp->xyTrace[validTraces].cartesianPlot = pcp;
              if (dlCartesianPlot->trace[i].yaxis == 0)
                {
                  nTracesY1++;
                }
              else if (dlCartesianPlot->trace[i].yaxis == 1)
                {
                  nTracesY2++;
                }
              else if (dlCartesianPlot->trace[i].yaxis == 2)
                {
                  nTracesY3++;
                }
              else if (dlCartesianPlot->trace[i].yaxis == 3)
                {
                  nTracesY4++;
                }
              else
                {
                  fprintf(stderr, "Invalid yaxis\n");
                  exit(1);
                }
              validTraces++;
            }
        }

      /* If no xyTraces, create one fake one */
      if (validTraces == 0)
        {
          validTraces = 1;
          pcp->xyTrace[0].recordX = medmAllocateRecord(" ",
                                                       cartesianPlotUpdateScreenFirstTime,
                                                       cartesianPlotUpdateGraphicalInfoCb,
                                                       (XtPointer) & (pcp->xyTrace[0]));
          pcp->xyTrace[0].recordY = NULL;
        }

      /* Record the number of traces in the cartesian plot */
      pcp->nTraces = validTraces;
      pcp->nTracesY1 = nTracesY1;
      pcp->nTracesY2 = nTracesY2;
      pcp->nTracesY3 = nTracesY3;
      pcp->nTracesY4 = nTracesY4;

      /* Allocate (or set to NULL) the X Record in the eraseCh XYTrace */
      if ((dlCartesianPlot->erase[0] != '\0') && (validTraces > 0))
        {
          pcp->eraseCh.recordX =
            medmAllocateRecord(dlCartesianPlot->erase,
                               cartesianPlotUpdateScreenFirstTime,
                               cartesianPlotUpdateGraphicalInfoCb,
                               (XtPointer) & (pcp->eraseCh));
          pcp->eraseCh.cartesianPlot = pcp;
        }
      else
        {
          pcp->eraseCh.recordX = NULL;
        }

      /* Allocate (or set to NULL) the X Record in the triggerCh XYTrace */
      if ((dlCartesianPlot->trigger[0] != '\0') && (validTraces > 0))
        {
          pcp->triggerCh.recordX =
            medmAllocateRecord(dlCartesianPlot->trigger,
                               cartesianPlotUpdateScreenFirstTime,
                               cartesianPlotUpdateGraphicalInfoCb,
                               (XtPointer) & (pcp->triggerCh));
          pcp->triggerCh.cartesianPlot = pcp;
        }
      else
        {
          pcp->triggerCh.recordX = NULL;
        }

      /* Allocate (or set to NULL) the X Record in the countCh XYTrace */
      if ((dlCartesianPlot->countPvName[0] != '\0') && (validTraces > 0))
        {
          if (isdigit(dlCartesianPlot->countPvName[0]))
            {
              dlCartesianPlot->count = atoi(dlCartesianPlot->countPvName);
              pcp->countCh.recordX = NULL;
            }
          else
            {
              pcp->countCh.recordX =
                medmAllocateRecord(dlCartesianPlot->countPvName,
                                   cartesianPlotUpdateScreenFirstTime,
                                   cartesianPlotUpdateGraphicalInfoCb,
                                   (XtPointer) & (pcp->countCh));
              pcp->countCh.cartesianPlot = pcp;
            }
        }
      else
        {
          pcp->countCh.recordX = NULL;
        }

      /* Note: Only the Record and MedmCartesianPlot parts of the
       *   XYTrace's are filled in now, the rest is filled in in
       *   cartesianPlotUpdateGraphicalInfoCb() */

      drawWhiteRectangle(pcp->updateTask);
    }

  if (!dlElement->widget)
    {
      localWidget = CpCreateCartesianPlot(displayInfo,
                                          dlCartesianPlot, pcp);
      dlElement->widget = localWidget;
    }
}

static void cartesianPlotCreateEditInstance(DisplayInfo *displayInfo,
                                            DlElement *dlElement)
{
  MedmCartesianPlot *pcp;
  Widget localWidget;
  DlCartesianPlot *dlCartesianPlot = dlElement->structure.cartesianPlot;

  pcp = NULL;

  localWidget = CpCreateCartesianPlot(displayInfo,
                                      dlCartesianPlot, pcp);
  dlElement->widget = localWidget;

  /* Add handlers */
  addCommonHandlers(localWidget, displayInfo);

  XtManageChild(localWidget);
}

void executeDlCartesianPlot(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Don't do anyting if the element is hidden */
  if (dlElement->hidden)
    return;

  if (dlElement->widget)
    {
      DlObject *po = &(dlElement->structure.cartesianPlot->object);
      XtVaSetValues(dlElement->widget,
                    XmNx, (Position)po->x,
                    XmNy, (Position)po->y,
                    XmNwidth, (Dimension)po->width,
                    XmNheight, (Dimension)po->height,
                    NULL);
    }
  else if (displayInfo->traversalMode == DL_EXECUTE)
    {
      cartesianPlotCreateRunTimeInstance(displayInfo, dlElement);
    }
  else if (displayInfo->traversalMode == DL_EDIT)
    {
      cartesianPlotCreateEditInstance(displayInfo, dlElement);
    }
}

void hideDlCartesianPlot(DisplayInfo *displayInfo, DlElement *dlElement)
{
  /* Use generic hide for an element with a widget */
  hideWidgetElement(displayInfo, dlElement);
}

static void cartesianPlotUpdateGraphicalInfoCb(XtPointer cd)
{
  Record *pr = (Record *)cd;
  XYTrace *pt = (XYTrace *)pr->clientData;
  MedmCartesianPlot *pcp = pt->cartesianPlot;
  int i;

#if DEBUG_HISTOGRAM
  print("\ncartesianPlotUpdateGraphicalInfoCb: name=%s lopr=%g hopr=%g \n",
        pr->name, pr->lopr, pr->hopr);
#endif

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! This is a temporary work around !!!!! */
  /* !!!!! for the reconnection.           !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  /* KE: This causes it to not do anything for the reconnection */
  medmRecordAddGraphicalInfoCb(pr, NULL);

  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */
  /* !!!!! End work around                 !!!!! */
  /* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

  /* Return until all channels get their graphical information */
  if (pcp->eraseCh.recordX && (pcp->eraseCh.recordX->precision < 0))
    return;
  if (pcp->triggerCh.recordX && (pcp->triggerCh.recordX->precision < 0))
    return;
  if (pcp->countCh.recordX && (pcp->countCh.recordX->precision < 0))
    return;
  for (i = 0; i < pcp->nTraces; i++)
    {
      XYTrace *t = &(pcp->xyTrace[i]);
      if (t->recordX)
        {
          if (t->recordX->precision < 0)
            return;
        }

      if (t->recordY)
        {
          if (t->recordY->precision < 0)
            return;
        }
    }

  /* Set up the plot */
  cartesianPlotResetPlot(pcp);

#if 0
  /* Add destroy callback */
  XtAddCallback(widget,XmNdestroyCallback,
                (XtCallbackProc)monitorDestroy, (XtPointer)pcp);
#endif

  /* Now call the plot update routine */
  cartesianPlotUpdateScreenFirstTime(cd);
}

static void cartesianPlotUpdateTrace(XtPointer cd, Boolean updateLastPoint)
{
  Record *pr = (Record *)cd;
  XYTrace *pt = (XYTrace *)pr->clientData;
  MedmCartesianPlot *pcp = pt->cartesianPlot;
  Widget w = pcp->dlElement->widget;
  DlCartesianPlot *dlCartesianPlot = pcp->dlElement->structure.cartesianPlot;
  int pointsUsed, j;
  int count;

#if DEBUG_CARTESIAN_PLOT_UPDATE || DEBUG_COUNT
  printf("cartesianPlotUpdateTrace: updateLastPoint=%s pt->hcp=%p\n",
         updateLastPoint ? "True" : "False", (void *)pt->hcp);
#endif
  switch (pt->type)
    {
    case CP_XYScalar:
      /* x,y channels specified - scalars, up to dlCartesianPlot->count pairs */
      count = dlCartesianPlot->count;
#if USECOUNTPV
      /* Use count from the PV */
      if (pcp->countCh.recordX != NULL)
        {
          int chCount = (int)(pcp->countCh.recordX->value + .5);
          if (chCount > 0)
            count = chCount;
          else
            count = 0;
        }
#endif
      if (count <= 0 || !updateLastPoint)
        {
          break;
        }

      pointsUsed = CpDataGetPointsUsed(pt->hcp, pt->trace);
#if DEBUG_COUNT
      print("CP_XYScalar counts: CP: %d PV: %d Count: %d pointsUsed: %d\n",
            dlCartesianPlot->count,
            pcp->countCh.recordX != 0 ? (int)(pcp->countCh.recordX->value + .5) : -1,
            count, pointsUsed);
#endif
      if (pointsUsed < count)
        {
          CpDataSetXElement(pt->hcp, pt->trace, pointsUsed,
                            SAFEFLOAT(pt->recordX->value));
          CpDataSetYElement(pt->hcp, pt->trace, pointsUsed,
                            SAFEFLOAT(pt->recordY->value));
          CpDataSetPointsUsed(w, pt->hcp, pt->trace, ++pointsUsed);
#if DEBUG_COUNT
          print("  New pointsUsed: %d\n", pointsUsed);
#endif
        }
      else
        {
          if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF)
            {
              /* All done, don't add any more points */
            }
          else if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON)
            {
              /* Shift everybody down one, add at end */
              int j;
              for (j = 1; j < count; j++)
                {
                  CpDataSetXElement(pt->hcp, pt->trace, j - 1,
                                    CpDataGetXElement(pt->hcp, pt->trace, j));
                  CpDataSetYElement(pt->hcp, pt->trace, j - 1,
                                    CpDataGetYElement(pt->hcp, pt->trace, j));
                }
              CpDataSetXElement(pt->hcp, pt->trace, j - 1,
                                SAFEFLOAT(pt->recordX->value));
              CpDataSetYElement(pt->hcp, pt->trace, j - 1,
                                SAFEFLOAT(pt->recordY->value));
            }
        }
#if 0
      /* Use with cpCP_XYScalar.adl or delete these lines */
      printf("pointsUsed=%d count=%d x=%g y=%g\n",
             pointsUsed,count,
             pt->recordX->value,pt->recordY->value);
      printf("x1=%g x2=%g x3=%g\n",
             CpDataGetXElement(pt->hcp,pt->trace,0),
             CpDataGetXElement(pt->hcp,pt->trace,1),
             CpDataGetXElement(pt->hcp,pt->trace,2));
      printf("y1=%g y2=%g y3=%g\n\n",
             CpDataGetYElement(pt->hcp,pt->trace,0),
             CpDataGetYElement(pt->hcp,pt->trace,1),
             CpDataGetYElement(pt->hcp,pt->trace,2));
#endif
      break;

    case CP_XScalar:
      /* x channel scalar, up to count pairs */
      count = dlCartesianPlot->count;
#if USECOUNTPV
      /* Use count from the PV */
      if (pcp->countCh.recordX != 0)
        {
          int chCount = (int)(pcp->countCh.recordX->value + .5);
          if (chCount > 0)
            count = chCount;
          else
            count = 0;
        }
#endif
      if (count <= 0 || !updateLastPoint)
        {
          break;
        }
      pointsUsed = CpDataGetPointsUsed(pt->hcp, pt->trace);
      if (pointsUsed < count)
        {
          CpDataSetXElement(pt->hcp, pt->trace, pointsUsed,
                            SAFEFLOAT(pt->recordX->value));
          CpDataSetYElement(pt->hcp, pt->trace, pointsUsed, (float)pointsUsed);
          CpDataSetPointsUsed(w, pt->hcp, pt->trace, ++pointsUsed);
        }
      else
        {
          if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF)
            {
              /* All done, don't add any more points */
            }
          else if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON)
            {
              /* Shift everybody down one, add at end */
              int j;
              for (j = 1; j < count; j++)
                {
                  CpDataSetXElement(pt->hcp, pt->trace, j - 1,
                                    CpDataGetXElement(pt->hcp, pt->trace, j));
                  CpDataSetYElement(pt->hcp, pt->trace, j - 1, (float)(j - 1));
                }
              CpDataSetXElement(pt->hcp, pt->trace, j - 1,
                                SAFEFLOAT(pt->recordX->value));
              CpDataSetYElement(pt->hcp, pt->trace, j - 1, (float)(j - 1));
            }
        }
      break;

    case CP_XVector:
      /* x channel vector, ca_element_count(chid) elements */
      count = pt->recordX->elementCount;
#if USECOUNTPV
      /* Use count from the PV */
      if (pcp->countCh.recordX != 0)
        {
          int chCount = (int)(pcp->countCh.recordX->value + .5);

          /* Must be greater than 0 and less than the native count */
          if (chCount > 0 && chCount < count)
            count = chCount;
        }
#endif
      /* Use count from the recordX */
      if (dlCartesianPlot->count == 0)
        {
          count = pt->recordX->currentCount;
        }
      pointsUsed = MAX(count, 0);
      CpDataSetPointsUsed(w, pt->hcp, pt->trace, pointsUsed);
      switch (pt->recordX->dataType)
        {
        case DBF_STRING:
          {
            for (j = 0; j < count; j++)
              {
                CpDataSetXElement(pt->hcp, pt->trace, j, 0.0);
                CpDataSetYElement(pt->hcp, pt->trace, j, (float)j);
              }
            break;
          }
        case DBF_INT:
          {
            short *pShort = (short *)pt->recordX->array;
            for (j = 0; j < count; j++)
              {
                CpDataSetXElement(pt->hcp, pt->trace, j, (float)pShort[j]);
                CpDataSetYElement(pt->hcp, pt->trace, j, (float)j);
              }
            break;
          }
        case DBF_FLOAT:
          {
            float *pFloat = (float *)pt->recordX->array;
            for (j = 0; j < count; j++)
              {
                CpDataSetXElement(pt->hcp, pt->trace, j, SAFEFLOAT(pFloat[j]));
                CpDataSetYElement(pt->hcp, pt->trace, j, (float)j);
              }
            break;
          }
        case DBF_ENUM:
          {
            unsigned short *pUShort = (unsigned short *)pt->recordX->array;
            for (j = 0; j < count; j++)
              {
                CpDataSetXElement(pt->hcp, pt->trace, j, (float)pUShort[j]);
                CpDataSetYElement(pt->hcp, pt->trace, j, (float)j);
              }
            break;
          }
        case DBF_CHAR:
          {
            unsigned char *pUChar = (unsigned char *)pt->recordX->array;
            for (j = 0; j < count; j++)
              {
                CpDataSetXElement(pt->hcp, pt->trace, j, (float)pUChar[j]);
                CpDataSetYElement(pt->hcp, pt->trace, j, (float)j);
              }
            break;
          }
        case DBF_LONG:
          {
            dbr_long_t *pLong = (dbr_long_t *)pt->recordX->array;
            for (j = 0; j < count; j++)
              {
                CpDataSetXElement(pt->hcp, pt->trace, j, (float)pLong[j]);
                CpDataSetYElement(pt->hcp, pt->trace, j, (float)j);
              }
            break;
          }
        case DBF_DOUBLE:
          {
            double *pDouble = (double *)pt->recordX->array;
            for (j = 0; j < count; j++)
              {
                CpDataSetXElement(pt->hcp, pt->trace, j, SAFEFLOAT(pDouble[j]));
                CpDataSetYElement(pt->hcp, pt->trace, j, (float)j);
              }
            break;
          }
        }
      break;

    case CP_YScalar:
      /* y channel scalar, up to count pairs */
      count = dlCartesianPlot->count;
#if USECOUNTPV
      /* Use count from the PV */
      if (pcp->countCh.recordX != 0)
        {
          int chCount = (int)(pcp->countCh.recordX->value + .5);
          if (chCount > 0)
            count = chCount;
          else
            count = 0;
        }
#endif
      if (count <= 0 || !updateLastPoint)
        {
#if DEBUG_COUNT
          print("CP_YScalar [break]: count=%d updateLastPoint=%s\n",
                count, updateLastPoint ? "True" : "False");
#endif
          break;
        }
      pointsUsed = CpDataGetPointsUsed(pt->hcp, pt->trace);
#if DEBUG_COUNT
      print("CP_YScalar counts: CP: %d PV: %d Count: %d pointsUsed: %d\n",
            dlCartesianPlot->count,
            pcp->countCh.recordX != 0 ? (int)(pcp->countCh.recordX->value + .5) : -1,
            count, pointsUsed);
#endif
#if DEBUG_CARTESIAN_PLOT_UPDATE && defined(XRTGRAPH)
      printf("  pointsUsed=%d count=%d\n  XRT_HUGE_VAL=%f\n",
             pointsUsed,
             count,
             XRT_HUGE_VAL);
#endif
      if (pointsUsed < count)
        {
          if (pointsUsed == 0 && pcp->timeScale)
            {
              CpSetTimeBase(pcp->dlElement->widget,
                            timeOffset + (time_t)pt->recordY->time.secPastEpoch);
              pcp->startTime = pt->recordY->time;
            }
#if defined(SCIPLOT)
          if (pcp->timeScale)
            CpDataSetXElement(pt->hcp, pt->trace, pointsUsed, (double)(pt->recordY->time.secPastEpoch + 1e-9 * pt->recordY->time.nsec));
          else
            CpDataSetXElement(pt->hcp, pt->trace, pointsUsed, (float)pointsUsed);
#else
          CpDataSetXElement(pt->hcp, pt->trace, pointsUsed,
                            (pcp->timeScale) ? (float)(pt->recordY->time.secPastEpoch - pcp->startTime.secPastEpoch) : (float)pointsUsed);
#endif
          CpDataSetYElement(pt->hcp, pt->trace, pointsUsed,
                            (float)pt->recordY->value);
          pointsUsed++;
          CpDataSetPointsUsed(w, pt->hcp, pt->trace, pointsUsed);
#if DEBUG_COUNT
          print("  New pointsUsed: %d count: %d\n", pointsUsed, count);
#endif
        }
      else
        {
          if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_OFF)
            {
              /* All done, don't add any more points */
#if DEBUG_CARTESIAN_PLOT_UPDATE || DEBUG_COUNT
              printf("  Array full: count=%d pointsUsed=%d\n",
                     count, pointsUsed);
#endif
            }
          else if (dlCartesianPlot->erase_oldest == ERASE_OLDEST_ON)
            {
              /* Shift everybody down one, add at end */
              int j;

#if DEBUG_CARTESIAN_PLOT_UPDATE || DEBUG_COUNT
              printf("  Shifting: count=%d pointsUsed=%d\n",
                     count, pointsUsed);
#endif
              if (pcp->timeScale)
                {
                  for (j = 1; j < count; j++)
                    {
                      CpDataSetXElement(pt->hcp, pt->trace, j - 1,
                                        CpDataGetXElement(pt->hcp, pt->trace, j));
                      CpDataSetYElement(pt->hcp, pt->trace, j - 1,
                                        CpDataGetYElement(pt->hcp, pt->trace, j));
                    }
#if defined(SCIPLOT)
                  CpDataSetXElement(pt->hcp, pt->trace, j - 1,
                                    (double)(pt->recordY->time.secPastEpoch + 1e-9 * pt->recordY->time.nsec));
#else
                  CpDataSetXElement(pt->hcp, pt->trace, j - 1,
                                    (float)(pt->recordY->time.secPastEpoch -
                                            pcp->startTime.secPastEpoch));
#endif
                  CpDataSetYElement(pt->hcp, pt->trace, j - 1,
                                    (float)pt->recordY->value);
                }
              else
                {
                  for (j = 1; j < count; j++)
                    {
                      CpDataSetYElement(pt->hcp, pt->trace, j - 1,
                                        CpDataGetYElement(pt->hcp, pt->trace, j));
                    }
                  CpDataSetYElement(pt->hcp, pt->trace, j - 1,
                                    (float)pt->recordY->value);
                }
            }
        }
      break;

    case CP_YVector:
      /* plot first "count" elements of vector per dlCartesianPlot */
      count = pt->recordY->elementCount;
#if USECOUNTPV
      /* Use count from the PV */
      if (pcp->countCh.recordX != 0)
        {
          int chCount = (int)(pcp->countCh.recordX->value + .5);

          /* Must be greater than 0 and less than the native count */
          if (chCount > 0 && chCount < count)
            count = chCount;
        }
#endif

      /* Use count from the recordY */
      if (dlCartesianPlot->count == 0)
        {
          count = pt->recordY->currentCount;
        }
      pointsUsed = MAX(count, 0);
      CpDataSetPointsUsed(w, pt->hcp, pt->trace, pointsUsed);
      switch (pt->recordY->dataType)
        {
        case DBF_STRING:
          {
            for (j = 0; j < count; j++)
              {
                CpDataSetYElement(pt->hcp, pt->trace, j, 0.0);
                CpDataSetXElement(pt->hcp, pt->trace, j, (float)j);
              }
            break;
          }
        case DBF_INT:
          {
            short *pShort = (short *)pt->recordY->array;
            for (j = 0; j < count; j++)
              {
                CpDataSetYElement(pt->hcp, pt->trace, j, (float)pShort[j]);
                CpDataSetXElement(pt->hcp, pt->trace, j, (float)j);
              }
            break;
          }
        case DBF_FLOAT:
          {
            float *pFloat = (float *)pt->recordY->array;
            for (j = 0; j < count; j++)
              {
                CpDataSetYElement(pt->hcp, pt->trace, j, SAFEFLOAT(pFloat[j]));
                CpDataSetXElement(pt->hcp, pt->trace, j, (float)j);
              }
            break;
          }
        case DBF_ENUM:
          {
            unsigned short *pUShort = (unsigned short *)pt->recordY->array;
            for (j = 0; j < count; j++)
              {
                CpDataSetYElement(pt->hcp, pt->trace, j, (float)pUShort[j]);
                CpDataSetXElement(pt->hcp, pt->trace, j, (float)j);
              }
            break;
          }
        case DBF_CHAR:
          {
            unsigned char *pUChar = (unsigned char *)pt->recordY->array;
            for (j = 0; j < count; j++)
              {
                CpDataSetYElement(pt->hcp, pt->trace, j, (float)pUChar[j]);
                CpDataSetXElement(pt->hcp, pt->trace, j, (float)j);
              }
            break;
          }
        case DBF_LONG:
          {
            dbr_long_t *pLong = (dbr_long_t *)pt->recordY->array;
            for (j = 0; j < count; j++)
              {
                CpDataSetYElement(pt->hcp, pt->trace, j, (float)pLong[j]);
                CpDataSetXElement(pt->hcp, pt->trace, j, (float)j);
              }
            break;
          }
        case DBF_DOUBLE:
          {
            double *pDouble = (double *)pt->recordY->array;
            for (j = 0; j < count; j++)
              {
                CpDataSetYElement(pt->hcp, pt->trace, j, SAFEFLOAT(pDouble[j]));
                CpDataSetXElement(pt->hcp, pt->trace, j, (float)j);
              }
            break;
          }
        }
      break;

    case CP_XVectorYScalar:
      count = pt->recordX->elementCount;
#if USECOUNTPV
      /* Use count from the PV */
      if (pcp->countCh.recordX != 0)
        {
          int chCount = (int)(pcp->countCh.recordX->value + .5);

          /* Must be greater than 0 and less than the native count */
          if (chCount > 0 && chCount < count)
            count = chCount;
        }
#endif
      /* Use count from the recordX */
      if (dlCartesianPlot->count == 0)
        {
          count = pt->recordX->currentCount;
        }
      pointsUsed = MAX(count, 0);
      CpDataSetPointsUsed(w, pt->hcp, pt->trace, pointsUsed);
      if (pr == pt->recordX)
        {
          /* plot first "count" elements of vector per dlCartesianPlot */
          switch (pt->recordX->dataType)
            {
            case DBF_STRING:
              {
                for (j = 0; j < count; j++)
                  {
                    CpDataSetXElement(pt->hcp, pt->trace, j, 0.0);
                  }
                break;
              }
            case DBF_INT:
              {
                short *pShort = (short *)pt->recordX->array;
                for (j = 0; j < count; j++)
                  {
                    CpDataSetXElement(pt->hcp, pt->trace, j, (float)pShort[j]);
                  }
                break;
              }
            case DBF_FLOAT:
              {
                float *pFloat = (float *)pt->recordX->array;
                for (j = 0; j < count; j++)
                  {
                    CpDataSetXElement(pt->hcp, pt->trace, j, SAFEFLOAT(pFloat[j]));
                  }
                break;
              }
            case DBF_ENUM:
              {
                unsigned short *pUShort = (unsigned short *)pt->recordX->array;
                for (j = 0; j < count; j++)
                  {
                    CpDataSetXElement(pt->hcp, pt->trace, j, (float)pUShort[j]);
                  }
                break;
              }
            case DBF_CHAR:
              {
                unsigned char *pUChar = (unsigned char *)pt->recordX->array;
                for (j = 0; j < count; j++)
                  {
                    CpDataSetXElement(pt->hcp, pt->trace, j, (float)pUChar[j]);
                  }
                break;
              }
            case DBF_LONG:
              {
                dbr_long_t *pLong = (dbr_long_t *)pt->recordX->array;
                for (j = 0; j < count; j++)
                  {
                    CpDataSetXElement(pt->hcp, pt->trace, j, (float)pLong[j]);
                  }
                break;
              }
            case DBF_DOUBLE:
              {
                double *pDouble = (double *)pt->recordX->array;
                for (j = 0; j < count; j++)
                  {
                    CpDataSetXElement(pt->hcp, pt->trace, j,
                                      SAFEFLOAT(pDouble[j]));
                  }
                break;
              }
            }
        }
      else
        {
          if (pr == pt->recordY)
            {
              for (j = 0; j < count; j++)
                {
                  CpDataSetYElement(pt->hcp, pt->trace, j,
                                    SAFEFLOAT(pt->recordY->value));
                }
            }
        }
      break;

    case CP_YVectorXScalar:
      count = pt->recordY->elementCount;
#if USECOUNTPV
      /* Use count from the PV */
      if (pcp->countCh.recordX != 0)
        {
          int chCount = (int)(pcp->countCh.recordX->value + .5);

          /* Must be greater than 0 and less than the native count */
          if (chCount > 0 && chCount < count)
            count = chCount;
        }
#endif
      /* Use count from the recordY */
      if (dlCartesianPlot->count == 0)
        {
          count = pt->recordY->currentCount;
        }
      pointsUsed = MAX(count, 0);
      CpDataSetPointsUsed(w, pt->hcp, pt->trace, pointsUsed);
      if (pr == pt->recordY)
        {
          /* plot first "count" elements of vector per dlCartesianPlot */
          switch (pt->recordY->dataType)
            {
            case DBF_STRING:
              {
                for (j = 0; j < count; j++)
                  {
                    CpDataSetYElement(pt->hcp, pt->trace, j, 0.0);
                  }
                break;
              }
            case DBF_INT:
              {
                short *pShort = (short *)pt->recordY->array;
                for (j = 0; j < count; j++)
                  {
                    CpDataSetYElement(pt->hcp, pt->trace, j, (float)pShort[j]);
                  }
                break;
              }
            case DBF_FLOAT:
              {
                float *pFloat = (float *)pt->recordY->array;
                for (j = 0; j < count; j++)
                  {
                    CpDataSetYElement(pt->hcp, pt->trace, j,
                                      SAFEFLOAT(pFloat[j]));
                  }
                break;
              }
            case DBF_ENUM:
              {
                unsigned short *pUShort = (unsigned short *)pt->recordY->array;
                for (j = 0; j < count; j++)
                  {
                    CpDataSetYElement(pt->hcp, pt->trace, j, (float)pUShort[j]);
                  }
                break;
              }
            case DBF_CHAR:
              {
                unsigned char *pUChar = (unsigned char *)pt->recordY->array;
                for (j = 0; j < count; j++)
                  {
                    CpDataSetYElement(pt->hcp, pt->trace, j, (float)pUChar[j]);
                  }
                break;
              }
            case DBF_LONG:
              {
                dbr_long_t *pLong = (dbr_long_t *)pt->recordY->array;
                for (j = 0; j < count; j++)
                  {
                    CpDataSetYElement(pt->hcp, pt->trace, j, (float)pLong[j]);
                  }
                break;
              }
            case DBF_DOUBLE:
              {
                double *pDouble = (double *)pt->recordY->array;
                for (j = 0; j < count; j++)
                  {
                    CpDataSetYElement(pt->hcp, pt->trace, j,
                                      SAFEFLOAT(pDouble[j]));
                  }
                break;
              }
            }
        }
      else if (pr == pt->recordX)
        {
          for (j = 0; j < count; j++)
            {
              CpDataSetXElement(pt->hcp, pt->trace, j,
                                SAFEFLOAT(pt->recordX->value));
            }
        }
      break;

    case CP_XYVector:
      {
        int dox;

        count = MIN(pt->recordX->elementCount, pt->recordY->elementCount);
#if USECOUNTPV
        /* Use count from the PV */
        if (pcp->countCh.recordX != 0)
          {
            int chCount = (int)(pcp->countCh.recordX->value + .5);

            /* Must be greater than 0 and less than the native count */
            if (chCount > 0 && chCount < count)
              count = chCount;
          }
#endif
        /* Use count from recordX and recordY */
        if (dlCartesianPlot->count == 0)
          {
            count = MIN(pt->recordX->currentCount, pt->recordY->currentCount);
          }

        pointsUsed = MAX(count, 0);
        CpDataSetPointsUsed(w, pt->hcp, pt->trace, pointsUsed);

        if (pr == pt->recordX)
          {
            dox = 1;
          }
        else if (pr == pt->recordY)
          {
            dox = 0;
          }
        else
          {
            /* don't do anything */
            break;
          }
        if (pcp->timeScale && pr == pt->recordX) //dox has to equal 1
          {
            if (pcp->startTime.nsec)
              {
              }
            else
              {
                pcp->startTime.secPastEpoch = (int)pr->value;
                pcp->startTime.nsec = 1;
                CpSetTimeBase(pcp->dlElement->widget,
                              timeOffset + pcp->startTime.secPastEpoch);
#if DEBUG_TIME
                printf("pcp->startTime = %d\n", pcp->startTime.secPastEpoch);
#endif
              }
            switch (pr->dataType)
              {
              case DBF_LONG:
                {
                  dbr_long_t *pLong = (dbr_long_t *)pr->array;
#if !defined(SCIPLOT)
                  dbr_long_t offset = (dbr_long_t)pcp->startTime.secPastEpoch;
#endif
                  for (j = 0; j < count; j++)
                    {
#if defined(SCIPLOT)
                      if (dox)
                        CpDataSetXElement(pt->hcp, pt->trace, j,
                                          (double)(pLong[j]));
                      else
                        CpDataSetYElement(pt->hcp, pt->trace, j,
                                          (double)(pLong[j]));
#else
                      if (dox)
                        CpDataSetXElement(pt->hcp, pt->trace, j,
                                          (float)(pLong[j] - offset));
                      else
                        CpDataSetYElement(pt->hcp, pt->trace, j,
                                          (float)(pLong[j] - offset));
#endif
                    }
                  break;
                }
              case DBF_DOUBLE:
                {
                  double *pDouble = (double *)pr->array;
#if !defined(SCIPLOT)
                  double offset = (double)pcp->startTime.secPastEpoch;
#endif
                  for (j = 0; j < count; j++)
                    {
#if defined(SCIPLOT)
                      if (dox)
                        CpDataSetXElement(pt->hcp, pt->trace, j,
                                          pDouble[j]);
                      else
                        CpDataSetYElement(pt->hcp, pt->trace, j,
                                          pDouble[j]);
#else
                      if (dox)
                        CpDataSetXElement(pt->hcp, pt->trace, j,
                                          SAFEFLOAT(pDouble[j] - offset));
                      else
                        CpDataSetYElement(pt->hcp, pt->trace, j,
                                          SAFEFLOAT(pDouble[j] - offset));
#endif
                    }
                  break;
                }
              default:
                break;
              }
          }
        else
          {
            switch (pr->dataType)
              {
              case DBF_STRING:
                for (j = 0; j < count; j++)
                  {
                    if (dox)
                      CpDataSetXElement(pt->hcp, pt->trace, j, 0.0);
                    else
                      CpDataSetYElement(pt->hcp, pt->trace, j, 0.0);
                  }
                break;
              case DBF_INT:
                {
                  short *pShort = (short *)pr->array;
                  for (j = 0; j < count; j++)
                    {
                      if (dox)
                        CpDataSetXElement(pt->hcp, pt->trace, j, (float)pShort[j]);
                      else
                        CpDataSetYElement(pt->hcp, pt->trace, j, (float)pShort[j]);
                    }
                  break;
                }
              case DBF_FLOAT:
                {
                  float *pFloat = (float *)pr->array;
                  for (j = 0; j < count; j++)
                    {
                      if (dox)
                        CpDataSetXElement(pt->hcp, pt->trace, j,
                                          SAFEFLOAT(pFloat[j]));
                      else
                        CpDataSetYElement(pt->hcp, pt->trace, j,
                                          SAFEFLOAT(pFloat[j]));
                    }
                  break;
                }
              case DBF_ENUM:
                {
                  unsigned short *pUShort = (unsigned short *)pr->array;
                  for (j = 0; j < count; j++)
                    {
                      if (dox)
                        CpDataSetXElement(pt->hcp, pt->trace, j, (float)pUShort[j]);
                      else
                        CpDataSetYElement(pt->hcp, pt->trace, j, (float)pUShort[j]);
                    }
                  break;
                }
              case DBF_CHAR:
                {
                  unsigned char *pUChar = (unsigned char *)pr->array;
                  for (j = 0; j < count; j++)
                    {
                      if (dox)
                        CpDataSetXElement(pt->hcp, pt->trace, j, (float)pUChar[j]);
                      else
                        CpDataSetYElement(pt->hcp, pt->trace, j, (float)pUChar[j]);
                    }
                  break;
                }
              case DBF_LONG:
                {
                  dbr_long_t *pLong = (dbr_long_t *)pr->array;
                  for (j = 0; j < count; j++)
                    {
                      if (dox)
                        CpDataSetXElement(pt->hcp, pt->trace, j, (float)pLong[j]);
                      else
                        CpDataSetYElement(pt->hcp, pt->trace, j, (float)pLong[j]);
                    }
                  break;
                }
              case DBF_DOUBLE:
                {
                  double *pDouble = (double *)pr->array;
                  for (j = 0; j < count; j++)
                    {
                      if (dox)
                        CpDataSetXElement(pt->hcp, pt->trace, j,
                                          SAFEFLOAT(pDouble[j]));
                      else
                        CpDataSetYElement(pt->hcp, pt->trace, j,
                                          SAFEFLOAT(pDouble[j]));
                    }
                  break;
                }
              }
          }
        break;
      }
    default:
      medmPrintf(1, "\ncartesianPlotUpdateTrace: Unknown dataType\n");
    }
}

static void cartesianPlotUpdateScreenFirstTime(XtPointer cd)
{
  Record *pr = (Record *)cd;
  XYTrace *pt = (XYTrace *)pr->clientData;
  MedmCartesianPlot *pcp = pt->cartesianPlot;
  Widget w = pcp->dlElement->widget;
  int i;
  Boolean clearDataSet1 = True;
  Boolean clearDataSet2 = True;
  Boolean clearDataSet3 = True;
  Boolean clearDataSet4 = True;
  Boolean initialize = True;

#if DEBUG_CARTESIAN_PLOT_UPDATE || DEBUG_ACCESS
  printf("cartesianPlotUpdateScreenFirstTime: nTraces=%d\n",
         pcp->nTraces);
#endif
  /* Return until all channels get their graphical information */
  for (i = 0; i < pcp->nTraces; i++)
    {
      XYTrace *t = &(pcp->xyTrace[i]);
#if DEBUG_CARTESIAN_PLOT_UPDATE || DEBUG_ACCESS
      printf("  recordX=%p recordY=%p hcp=%p\n",
             (void *)t->recordX, (void *)t->recordY, (void *)t->hcp);
      if (t->recordX)
        {
          printf("  x: array=%p precision=%d\n",
                 (void *)t->recordX->array, t->recordX->precision);
        }
      if (t->recordY)
        {
          printf("  y: array=%p precision=%d\n",
                 (void *)t->recordY->array, t->recordY->precision);
        }
#endif
      if ((t->recordX) || (t->recordY))
        {
          if (t->hcp == NULL)
            {
              initialize = False;
              goto INITIALIZE;
            }
          if (t->recordX &&
              (!t->recordX->array || t->recordX->precision < 0))
            {
              initialize = False;
              goto INITIALIZE;
            }
          if (t->recordY &&
              (!t->recordY->array || t->recordY->precision < 0))
            {
              initialize = False;
              goto INITIALIZE;
            }
        }
    }
  if (pcp->triggerCh.recordX)
    {
      if (pcp->triggerCh.recordX->precision < 0)
        {
          initialize = False;
          goto INITIALIZE;
        }
    }
  if (pcp->eraseCh.recordX)
    {
      if (pcp->eraseCh.recordX->precision < 0)
        {
          initialize = False;
          goto INITIALIZE;
        }
    }
  if (pcp->countCh.recordX)
    {
      if (pcp->countCh.recordX->precision < 0)
        {
          initialize = False;
          goto INITIALIZE;
        }
    }

 INITIALIZE:
  if (!initialize)
    {
      /* Update the plot for access, etc. */
      updateTaskMarkUpdate(pcp->updateTask);
      return;
    }

  /* Draw all traces once */
  for (i = 0; i < pcp->nTraces; i++)
    {
      XYTrace *pt = &(pcp->xyTrace[i]);
      if (pt->recordX && pt->recordY)
        {
          cartesianPlotUpdateTrace((XtPointer)pt->recordX, True);
          cartesianPlotUpdateTrace((XtPointer)pt->recordY, True);
        }
      else if (pt->recordX)
        {
          cartesianPlotUpdateTrace((XtPointer)pt->recordX, True);
        }
      else if (pt->recordY)
        {
          cartesianPlotUpdateTrace((XtPointer)pt->recordY, True);
        }
      else
        {
          continue;
        }
      if (pt->hcp == pcp->hcp1)
        {
          CpSetData(w, CP_Y, pt->hcp);
        }
      else if (pt->hcp == pcp->hcp2)
        {
          CpSetData(w, CP_Y2, pt->hcp);
        }
      else if (pt->hcp == pcp->hcp3)
        {
          CpSetData(w, CP_Y3, pt->hcp);
        }
      else if (pt->hcp == pcp->hcp4)
        {
          CpSetData(w, CP_Y4, pt->hcp);
        }
    }

  /* Erase the plot */
  if (pcp->eraseCh.recordX)
    {
      Record *pr = pcp->eraseCh.recordX;
      if (((pr->value == 0.0) && (pcp->eraseMode == ERASE_IF_ZERO)) || ((pr->value != 0.0) && (pcp->eraseMode == ERASE_IF_NOT_ZERO)))
        {
          for (i = 0; i < pcp->nTraces; i++)
            {
              XYTrace *pt = &(pcp->xyTrace[i]);
              if (pt->recordX || pt->recordY)
                {
                  /* Erase by setting to uninitialized state */
                  if ((pt->hcp == pcp->hcp1) && (clearDataSet1))
                    {
                      CpEraseData(w, CP_Y, pt->hcp);
                      clearDataSet1 = False;
                    }
                  else if ((pt->hcp == pcp->hcp2) && (clearDataSet2))
                    {
                      CpEraseData(w, CP_Y2, pt->hcp);
                      clearDataSet2 = False;
                    }
                  else if ((pt->hcp == pcp->hcp3) && (clearDataSet3))
                    {
                      CpEraseData(w, CP_Y3, pt->hcp);
                      clearDataSet3 = False;
                    }
                  else if ((pt->hcp == pcp->hcp4) && (clearDataSet4))
                    {
                      CpEraseData(w, CP_Y4, pt->hcp);
                      clearDataSet4 = False;
                    }
                  CpDataSetPointsUsed(w, pt->hcp, pt->trace, 0);
                }
            }
        }
    }

  /* Switch to the regular update routine from now on */

  for (i = 0; i < pcp->nTraces; i++)
    {
      XYTrace *pt = &(pcp->xyTrace[i]);
      if (pt->recordX)
        {
          medmRecordAddUpdateValueCb(pt->recordX,
                                     cartesianPlotUpdateValueCb);
        }
      if (pt->recordY)
        {
          medmRecordAddUpdateValueCb(pt->recordY,
                                     cartesianPlotUpdateValueCb);
        }
    }
  if (pcp->eraseCh.recordX)
    {
      medmRecordAddUpdateValueCb(pcp->eraseCh.recordX,
                                 cartesianPlotUpdateValueCb);
    }
  if (pcp->triggerCh.recordX)
    {
      medmRecordAddUpdateValueCb(pcp->triggerCh.recordX,
                                 cartesianPlotUpdateValueCb);
    }
  if (pcp->countCh.recordX)
    {
      medmRecordAddUpdateValueCb(pcp->countCh.recordX,
                                 cartesianPlotUpdateValueCb);
    }

  CpUpdateWidget(w, CP_FULL);

  //This was added to update the plot every second.
  //This is desired in case the PVs are not updating.
  if (pcp->triggerCh.recordX == NULL)
    {
      updateTaskSetScanRate(pcp->updateTask, 1);
    }
}

static void cartesianPlotUpdateValueCb(XtPointer cd)
{
  Record *pr = (Record *)cd;
  XYTrace *pt = (XYTrace *)pr->clientData;
  MedmCartesianPlot *pcp = pt->cartesianPlot;
  Widget w = pcp->dlElement->widget;
  int i;

#if DEBUG_CARTESIAN_PLOT_UPDATE || DEBUG_ACCESS
  printf("cartesianPlotUpdateValueCb:\n");
#endif
  /* If this is an erase channel, erase screen */
  if (pr == pcp->eraseCh.recordX)
    {
      Boolean clearDataSet1 = True;
      Boolean clearDataSet2 = True;
      Boolean clearDataSet3 = True;
      Boolean clearDataSet4 = True;

      /* Not the right value, return */
      if (((pr->value == 0) && (pcp->eraseMode == ERASE_IF_NOT_ZERO)) || ((pr->value != 0) && (pcp->eraseMode == ERASE_IF_ZERO)))
        {
          return;
        }

      /* Erase */
      for (i = 0; i < pcp->nTraces; i++)
        {
          XYTrace *pt = &(pcp->xyTrace[i]);
          if (pt->recordX || pt->recordY)
            {
              /* Erase by setting to uninitialized state */
              if ((pt->hcp == pcp->hcp1) && (clearDataSet1))
                {
                  CpEraseData(w, CP_Y, pt->hcp);
                  clearDataSet1 = False;
                  pcp->dirty1 = False;
                }
              else if ((pt->hcp == pcp->hcp2) && (clearDataSet2))
                {
                  CpEraseData(w, CP_Y2, pt->hcp);
                  clearDataSet2 = False;
                  pcp->dirty2 = False;
                }
              else if ((pt->hcp == pcp->hcp3) && (clearDataSet3))
                {
                  CpEraseData(w, CP_Y3, pt->hcp);
                  clearDataSet3 = False;
                  pcp->dirty3 = False;
                }
              else if ((pt->hcp == pcp->hcp4) && (clearDataSet4))
                {
                  CpEraseData(w, CP_Y4, pt->hcp);
                  clearDataSet4 = False;
                  pcp->dirty4 = False;
                }
              CpDataSetPointsUsed(w, pt->hcp, pt->trace, 0);
            }
        }
      CpUpdateWidget(w, CP_FULL);
#if DEBUG_ERASE
      dumpCartesianPlotData("Erase Channel", pcp);
#endif
      updateTaskMarkUpdate(pcp->updateTask);
      return;
    }

  /* Check if there is a trigger channel */
  if (pcp->triggerCh.recordX)
    {
      if (pr != pcp->triggerCh.recordX)
        {
          /* There is a trigger channel but this is not it, return
           * and do not update */
          return;
        }
      else
        {
          /* This is the trigger channel, update appropriate plots */
          for (i = 0; i < pcp->nTraces; i++)
            {
              XYTrace *pt = &(pcp->xyTrace[i]);
              if ((pt->recordX == NULL) && (pt->recordY == NULL))
                continue;
              if ((pt->recordX) && (pt->hcp))
                {
                  cartesianPlotUpdateTrace((XtPointer)pt->recordX, True);
                  if (pt->type == CP_XYVector)
                    {
                      cartesianPlotUpdateTrace((XtPointer)pt->recordY, True);
                    }
                  if (pt->hcp == pcp->hcp1)
                    pcp->dirty1 = True;
                  else if (pt->hcp == pcp->hcp2)
                    pcp->dirty2 = True;
                  else if (pt->hcp == pcp->hcp3)
                    pcp->dirty3 = True;
                  else if (pt->hcp == pcp->hcp4)
                    pcp->dirty4 = True;
                }
              else if ((pt->recordY) && (pt->hcp))
                {
                  cartesianPlotUpdateTrace((XtPointer)pt->recordY, True);
                  if (pt->hcp == pcp->hcp1)
                    pcp->dirty1 = True;
                  else if (pt->hcp == pcp->hcp2)
                    pcp->dirty2 = True;
                  else if (pt->hcp == pcp->hcp3)
                    pcp->dirty3 = True;
                  else if (pt->hcp == pcp->hcp4)
                    pcp->dirty4 = True;
                }
            }
#if DEBUG_ERASE
          dumpCartesianPlotData("Trigger Channel", pcp);
#endif
          updateTaskMarkUpdate(pcp->updateTask);
          return;
        }
    }

  /* If this is the count channel, resize and update the plot */
  if (pcp->countCh.recordX && pr == pcp->countCh.recordX)
    {
      /* Resize */
      Boolean updateLastPoint = cartesianPlotResetPlot(pcp);
      /* Update without updating lastPoint */
      for (i = 0; i < pcp->nTraces; i++)
        {
          XYTrace *pt = &(pcp->xyTrace[i]);
          if ((pt->recordX == NULL) && (pt->recordY == NULL))
            continue;
          if ((pt->recordX) && (pt->hcp))
            {
              cartesianPlotUpdateTrace((XtPointer)pt->recordX,
                                       updateLastPoint);
              if (pt->type == CP_XYVector)
                {
                  cartesianPlotUpdateTrace((XtPointer)pt->recordY,
                                           updateLastPoint);
                }
              if (pt->hcp == pcp->hcp1)
                pcp->dirty1 = True;
              else if (pt->hcp == pcp->hcp2)
                pcp->dirty2 = True;
              else if (pt->hcp == pcp->hcp3)
                pcp->dirty3 = True;
              else if (pt->hcp == pcp->hcp4)
                pcp->dirty4 = True;
            }
          else if ((pt->recordY) && (pt->hcp))
            {
              cartesianPlotUpdateTrace((XtPointer)pt->recordY,
                                       updateLastPoint);
              if (pt->hcp == pcp->hcp1)
                pcp->dirty1 = True;
              else if (pt->hcp == pcp->hcp2)
                pcp->dirty2 = True;
              else if (pt->hcp == pcp->hcp3)
                pcp->dirty3 = True;
              else if (pt->hcp == pcp->hcp4)
                pcp->dirty4 = True;
            }
        }
#if DEBUG_ERASE
      dumpCartesianPlotData("Count Channel", pcp);
#endif
      updateTaskMarkUpdate(pcp->updateTask);
      return;
    }

  /* This is a trace, update it */
  cartesianPlotUpdateTrace((XtPointer)pr, True);
  if (pt->hcp == pcp->hcp1)
    {
      pcp->dirty1 = True;
    }
  else if (pt->hcp == pcp->hcp2)
    {
      pcp->dirty2 = True;
    }
  else if (pt->hcp == pcp->hcp3)
    {
      pcp->dirty3 = True;
    }
  else if (pt->hcp == pcp->hcp4)
    {
      pcp->dirty4 = True;
    }
  else
    {
      medmPrintf(1, "\ncartesianPlotUpdateValueCb: "
                 "Illegal cpDataSet specified\n");
    }
#if DEBUG_ERASE
  dumpCartesianPlotData("Normal Channel", pcp);
#endif
  updateTaskMarkUpdate(pcp->updateTask);
}

/* Set the plot initially and reset the plot when the number of
 * points changes. Returns if it was the first time or not */
static Boolean cartesianPlotResetPlot(MedmCartesianPlot *pcp)
{
  XColor xColors[MAX_TRACES];
  CpDataHandle hcp = NULL, hcpold = NULL, hcp1 = NULL, hcp2 = NULL, hcp3 = NULL, hcp4 = NULL, hcpold1 = NULL, hcpold2 = NULL, hcpold3 = NULL, hcpold4 = NULL;
  DlCartesianPlot *dlCartesianPlot = pcp->dlElement->structure.cartesianPlot;
  DisplayInfo *displayInfo = pcp->updateTask->displayInfo;
  Widget w = pcp->dlElement->widget;
  float minX, maxX, minY, maxY, minY2, maxY2, minY3, maxY3, minY4, maxY4;
  XcVType minXF, maxXF, minYF, maxYF, minY2F, maxY2F, minY3F, maxY3F, minY4F, maxY4F, tickF;
  char string[24];
  int firstTime = 1, count = 0, nPoints = 0, nPointsOld = 0;
  int iPrec, kk, pointSize;
  int i, j;
  int iY1, iY2, iY3, iY4;
  int Yaxis, iYaxis;

#if DEBUG_RESET
  print("cartesianPlotResetPlot: \n");
#endif

  if (!pcp)
    {
      medmPostMsg(1, "cartesianPlotResetPlot: Invalid plot");
      return False;
    }

  /* Check if this is the first time or not */
  if (pcp->hcp1)
    firstTime = 0;

  /* Calculate the number of points (for allocating data structures)
   * to be the largest of any of the Channel Access counts and the
   * specified count (from the DlCartesianPlot or the countCh if it
   * exists) */
  count = dlCartesianPlot->count;
#if USECOUNTPV
  if (pcp->countCh.recordX != 0)
    {
      int chCount = (int)(pcp->countCh.recordX->value + .5);
      if (chCount > 0)
        count = chCount;
      else
        count = 0;
    }
#endif
  nPoints = count;
  for (i = 0; i < pcp->nTraces; i++)
    {
      XYTrace *pt = &(pcp->xyTrace[i]);
      if (pt->recordX)
        nPoints = MAX(nPoints, (int)pt->recordX->elementCount);
      if (pt->recordY)
        nPoints = MAX(nPoints, (int)pt->recordY->elementCount);
    }

  if (!firstTime)
    {
      /* Check if nPoints has changed */
      if (nPoints == nPointsOld)
        {
          /* Nothing needs to be done */
          return firstTime;
        }
      else
        {
          /* Retain pointers to old values */
          nPointsOld = pcp->nPoints;
          hcpold1 = pcp->hcp1;
          hcpold2 = pcp->hcp2;
          hcpold3 = pcp->hcp3;
          hcpold4 = pcp->hcp4;
        }
    }

  /* Delete the current curves from the plot */
  CpDataDeleteCurves(w, hcpold1);
  CpDataDeleteCurves(w, hcpold2);
  CpDataDeleteCurves(w, hcpold3);
  CpDataDeleteCurves(w, hcpold4);

  /* Allocate the CP data structures with size nPoints.  Note that
   * nPoints should always be at least 1 because the elementCount is
   * always at least 1. */
  pcp->nPoints = nPoints;
  hcp1 = hcp2 = hcp3 = hcp4 = NULL;
  if (pcp->nTracesY1 > 0)
    hcp1 = CpDataCreate(w, CP_GENERAL, pcp->nTracesY1, nPoints, CP_Y);
  if (pcp->nTracesY2 > 0)
    hcp2 = CpDataCreate(w, CP_GENERAL, pcp->nTracesY2, nPoints, CP_Y2);
  if (pcp->nTracesY3 > 0)
    hcp3 = CpDataCreate(w, CP_GENERAL, pcp->nTracesY3, nPoints, CP_Y3);
  if (pcp->nTracesY4 > 0)
    hcp4 = CpDataCreate(w, CP_GENERAL, pcp->nTracesY4, nPoints, CP_Y4);

  /* Set the data pointers in the MedmCartesianPlot */
  pcp->hcp1 = hcp1;
  pcp->hcp2 = hcp2;
  pcp->hcp3 = hcp3;
  pcp->hcp4 = hcp4;

  /* Get the colors for all the possible traces.  This is done here
   * because it must be redone each time for SciPlot. */
  for (i = 0; i < pcp->nTraces; i++)
    xColors[i].pixel =
      displayInfo->colormap[dlCartesianPlot->trace[i].data_clr];
  XQueryColors(XtDisplay(w), cmap, xColors, pcp->nTraces);

  /* Loop over traces and set CpDataStyle array */
  pointSize = MAX(2, dlCartesianPlot->object.height / 70);
  iY1 = iY2 = iY3 = iY4 = 0;
  iYaxis = 1;
  for (i = 0; i < pcp->nTraces; i++)
    {
      Yaxis = dlCartesianPlot->trace[i].yaxis;
      if (Yaxis == 0)
        {
          hcp = hcp1;
          iY1++;
          iYaxis = iY1;
        }
      else if (Yaxis == 1)
        {
          hcp = hcp2;
          iY2++;
          iYaxis = iY2;
        }
      else if (Yaxis == 2)
        {
          hcp = hcp3;
          iY3++;
          iYaxis = iY3;
        }
      else if (Yaxis == 3)
        {
          hcp = hcp4;
          iY4++;
          iYaxis = iY4;
        }
#if defined(SCIPLOT)
      switch (dlCartesianPlot->style)
        {
        case POINT_PLOT:
          CpSetAxisStyle(w, hcp, i, CP_LINE_NONE, CP_LINE_NONE,
                         xColors[i], pointSize, Yaxis, iYaxis - 1, dlCartesianPlot->trace[i].yside);
          break;
        case LINE_PLOT:
          CpSetAxisStyle(w, hcp, i, CP_LINE_SOLID, CP_LINE_NONE,
                         xColors[i], pointSize, Yaxis, iYaxis - 1, dlCartesianPlot->trace[i].yside);
          break;
        case STEP_PLOT:
          CpSetAxisStyle(w, hcp, i, CP_LINE_STEP, CP_LINE_NONE,
                         xColors[i], pointSize, Yaxis, iYaxis - 1, dlCartesianPlot->trace[i].yside);
          break;
        case FILL_UNDER_PLOT:
          CpSetAxisStyle(w, hcp, i, CP_LINE_SOLID, CP_LINE_SOLID,
                         xColors[i], pointSize, Yaxis, iYaxis - 1, dlCartesianPlot->trace[i].yside);
          break;
        }
#else
      switch (dlCartesianPlot->style)
        {
        case POINT_PLOT:
          CpSetAxisStyle(w, hcp, i, CP_LINE_NONE, CP_LINE_NONE,
                         xColors[i], pointSize, Yaxis, iYaxis - 1);
          break;
        case LINE_PLOT:
          CpSetAxisStyle(w, hcp, i, CP_LINE_SOLID, CP_LINE_NONE,
                         xColors[i], pointSize, Yaxis, iYaxis - 1);
          break;
        case STEP_PLOT:
          CpSetAxisStyle(w, hcp, i, CP_LINE_SOLID, CP_LINE_NONE,
                         xColors[i], pointSize, Yaxis, iYaxis - 1);
          break;
        case FILL_UNDER_PLOT:
          CpSetAxisStyle(w, hcp, i, CP_LINE_SOLID, CP_LINE_SOLID,
                         xColors[i], pointSize, Yaxis, iYaxis - 1);
          break;
        }
#endif
    }

#if DEBUG_RESET
  print("  firstTime=%d hcp1=%p hcp2=%p hcpold1=%p hcpold2=%p\n",
        firstTime, hcp1, hcp2, hcpold1, hcpold2);
  print("  nPoints=%d nPointsOld=%d count=%d\n",
        nPoints, nPointsOld, count);
#endif

  if (firstTime)
    goto FIRSTTIME;

  /* Data have previously been allocated.  Loop over all the traces
   * and copy the old data */
  for (i = 0; i < pcp->nTraces; i++)
    {
      XYTrace *pt = &(pcp->xyTrace[i]);
      int pointsUsed;

      /* Decide which handle to use */
#if DEBUG_RESET
      print("  Before: Trace %d pt->hcp=%p pt->type=%d\n",
            i, pt->hcp, pt->type);
#endif
      if (pt->hcp == hcpold1)
        {
          hcp = pt->hcp = hcp1;
          hcpold = hcpold1;
        }
      else if (pt->hcp == hcpold2)
        {
          hcp = pt->hcp = hcp2;
          hcpold = hcpold2;
        }
      else if (pt->hcp == hcpold3)
        {
          hcp = pt->hcp = hcp3;
          hcpold = hcpold3;
        }
      else if (pt->hcp == hcpold4)
        {
          hcp = pt->hcp = hcp4;
          hcpold = hcpold4;
        }
      else
        {
          medmPrintf(1, "\ncartesianPlotResetPlot: "
                     "Invalid handle for trace %d\n",
                     pt->trace);
        }
      if (!hcp || !hcpold)
        {
          if (!hcp)
            {
              medmPrintf(1, "\ncartesianPlotResetPlot: "
                         "Null new handle for trace %d\n",
                         pt->trace);
            }
          if (!hcpold)
            {
              medmPrintf(1, "\ncartesianPlotResetPlot: "
                         "Null old handle for trace %d\n",
                         pt->trace);
            }
          continue;
        }

      /* Fix points used */
      pointsUsed = CpDataGetPointsUsed(hcpold, pt->trace);
      if (pointsUsed > count)
        pointsUsed = count;
      CpDataSetPointsUsed(w, hcp, pt->trace, pointsUsed);
#if DEBUG_RESET
      print("  After:  Trace %d pointsUsed=%d\n",
            i, pointsUsed);
#endif

      /* Copy the data */
      for (j = 0; j < nPoints; j++)
        {
          if (j < nPointsOld)
            {
              CpDataSetXElement(hcp, pt->trace, j,
                                CpDataGetXElement(hcpold, pt->trace, j));
              CpDataSetYElement(hcp, pt->trace, j,
                                CpDataGetYElement(hcpold, pt->trace, j));
            }
          else
            {
              CpDataSetXElement(hcp, pt->trace, j, 0.0);
              CpDataSetYElement(hcp, pt->trace, j, 0.0);
            }
        }

      /* Add additional index data if necessary */
      if (pt->type == CP_XVector || pt->type == CP_XScalar)
        {
          for (j = nPointsOld; j < nPoints; j++)
            {
              CpDataSetYElement(hcp, pt->trace, j, (float)j);
            }
        }
      else if (pt->type == CP_YVector || pt->type == CP_YScalar)
        {
          for (j = nPointsOld; j < nPoints; j++)
            {
              CpDataSetXElement(hcp, pt->trace, j, (float)j);
            }
        }
    } /* End loop over traces */

  /* Free old values */
  if (hcpold1)
    CpDataDestroy(hcpold1);
  if (hcpold2)
    CpDataDestroy(hcpold2);
  if (hcpold3)
    CpDataDestroy(hcpold3);
  if (hcpold4)
    CpDataDestroy(hcpold4);

  return firstTime;

 FIRSTTIME:

  /* This is the first time.  Loop over all the traces and initialize
   * them */
  minX = minY = minY2 = minY3 = minY4 = FLT_MAX;
  maxX = maxY = maxY2 = maxY3 = maxY4 = (float)(-0.99 * FLT_MAX);
  iY1 = iY2 = iY3 = iY4 = 0;
  for (i = 0; i < pcp->nTraces; i++)
    {
      XYTrace *pt = &(pcp->xyTrace[i]);
      Yaxis = dlCartesianPlot->trace[i].yaxis;
      /* Set the pointsUsed to 0 */
      if (Yaxis <= 0)
        {
          CpDataSetPointsUsed(w, hcp1, iY1, 0);
          iY1++;
        }
      else if (Yaxis == 1)
        {
          CpDataSetPointsUsed(w, hcp2, iY2, 0);
          iY2++;
        }
      else if (Yaxis == 2)
        {
          CpDataSetPointsUsed(w, hcp3, iY3, 0);
          iY3++;
        }
      else
        {
          CpDataSetPointsUsed(w, hcp4, iY4, 0);
          iY4++;
        }

      /* Determine data type (based on type (scalar or vector) of data) */
      if (pt->recordX && pt->recordY)
        {
          /* X and Y channels */
          if (pt->recordX->elementCount > 1 && pt->recordY->elementCount > 1)
            {
              pt->type = CP_XYVector;
            }
          else if (pt->recordX->elementCount > 1 &&
                   pt->recordY->elementCount <= 1)
            {
              pt->type = CP_XVectorYScalar;
            }
          else if (pt->recordY->elementCount > 1 &&
                   pt->recordX->elementCount <= 1)
            {
              pt->type = CP_YVectorXScalar;
            }
          else if (pt->recordX->elementCount <= 1 &&
                   pt->recordY->elementCount <= 1)
            {
              pt->type = CP_XYScalar;
            }

          /* Get bounds */
          if (Yaxis <= 0)
            {
              minY = (float)MIN(minY, pt->recordY->lopr);
              maxY = (float)MAX(maxY, pt->recordY->hopr);
            }
          else if (Yaxis == 1)
            {
              minY2 = (float)MIN(minY2, pt->recordY->lopr);
              maxY2 = (float)MAX(maxY2, pt->recordY->hopr);
            }
          else if (Yaxis == 2)
            {
              minY3 = (float)MIN(minY3, pt->recordY->lopr);
              maxY3 = (float)MAX(maxY3, pt->recordY->hopr);
            }
          else if (Yaxis == 3)
            {
              minY4 = (float)MIN(minY4, pt->recordY->lopr);
              maxY4 = (float)MAX(maxY4, pt->recordY->hopr);
            }
          minX = (float)MIN(minX, pt->recordX->lopr);
          maxX = (float)MAX(maxX, pt->recordX->hopr);
        }
      else if (pt->recordX)
        {
          /* X channel - supporting scalar or waveform */
          if (pt->recordX->elementCount > 1)
            {
              /* Vector/waveform */
              pt->type = CP_XVector;
              if (Yaxis <= 0)
                {
                  for (j = 0; j < (int)pt->recordX->elementCount; j++)
                    CpDataSetYElement(hcp1, iY1 - 1, j, (float)j);
                  minY = (float)MIN(minY, 0.);
                  maxY = (float)MAX(maxY,
                                    (float)((int)pt->recordX->elementCount - 1));
                }
              else if (Yaxis == 1)
                {
                  for (j = 0; j < (int)pt->recordX->elementCount; j++)
                    CpDataSetYElement(hcp2, iY2 - 1, j, (float)j);
                  minY2 = (float)MIN(minY2, 0.);
                  maxY2 = (float)MAX(maxY2, (float)pt->recordX->elementCount - 1);
                }
              else if (Yaxis == 2)
                {
                  for (j = 0; j < (int)pt->recordX->elementCount; j++)
                    CpDataSetYElement(hcp3, iY3 - 1, j, (float)j);
                  minY3 = (float)MIN(minY3, 0.);
                  maxY3 = (float)MAX(maxY3, (float)pt->recordX->elementCount - 1);
                }
              else if (Yaxis == 3)
                {
                  for (j = 0; j < (int)pt->recordX->elementCount; j++)
                    CpDataSetYElement(hcp4, iY4 - 1, j, (float)j);
                  minY4 = (float)MIN(minY4, 0.);
                  maxY4 = (float)MAX(maxY4, (float)pt->recordX->elementCount - 1);
                }
              minX = (float)MIN(minX, pt->recordX->lopr);
              maxX = (float)MAX(maxX, pt->recordX->hopr);
            }
          else
            {
              /* Scalar */
              pt->type = CP_XScalar;
              if (Yaxis <= 0)
                {
                  for (j = 0; j < count; j++)
                    CpDataSetYElement(hcp1, iY1 - 1, j, (float)j);
                  CpDataSetXElement(hcp1, iY1 - 1, 0, (float)pt->recordX->value);
                  minY = (float)MIN(minY, 0.);
                  maxY = (float)MAX(maxY, (float)count);
                }
              else if (Yaxis == 1)
                {
                  for (j = 0; j < count; j++)
                    CpDataSetYElement(hcp2, iY2 - 1, j, (float)j);
                  CpDataSetXElement(hcp2, iY2 - 1, 0, (float)pt->recordX->value);
                  minY2 = (float)MIN(minY2, 0.);
                  maxY2 = (float)MAX(maxY2, (float)count);
                }
              else if (Yaxis == 2)
                {
                  for (j = 0; j < count; j++)
                    CpDataSetYElement(hcp3, iY3 - 1, j, (float)j);
                  CpDataSetXElement(hcp3, iY3 - 1, 0, (float)pt->recordX->value);
                  minY3 = (float)MIN(minY3, 0.);
                  maxY3 = (float)MAX(maxY3, (float)count);
                }
              else if (Yaxis == 3)
                {
                  for (j = 0; j < count; j++)
                    CpDataSetYElement(hcp4, iY4 - 1, j, (float)j);
                  CpDataSetXElement(hcp4, iY4 - 1, 0, (float)pt->recordX->value);
                  minY4 = (float)MIN(minY4, 0.);
                  maxY4 = (float)MAX(maxY4, (float)count);
                }
              minX = (float)MIN(minX, pt->recordX->lopr);
              maxX = (float)MAX(maxX, pt->recordX->hopr);
            }
        }
      else if (pt->recordY)
        {
          /* Y channel - supporting scalar or waveform */
          if (pt->recordY->elementCount > 1)
            {
              /* Vector/waveform */
              pt->type = CP_YVector;
              if (Yaxis <= 0)
                {
                  for (j = 0; j < (int)pt->recordY->elementCount; j++)
                    {
                      CpDataSetXElement(hcp1, iY1 - 1, j, (float)j);
                    }
                  minY = (float)MIN(minY, pt->recordY->lopr);
                  maxY = (float)MAX(maxY, pt->recordY->hopr);
                }
              else if (Yaxis == 1)
                {
                  for (j = 0; j < pt->recordY->elementCount; j++)
                    CpDataSetXElement(hcp2, iY2 - 1, j, (float)j);
                  minY2 = (float)MIN(minY2, pt->recordY->lopr);
                  maxY2 = (float)MAX(maxY2, pt->recordY->hopr);
                }
              else if (Yaxis == 2)
                {
                  for (j = 0; j < pt->recordY->elementCount; j++)
                    CpDataSetXElement(hcp3, iY3 - 1, j, (float)j);
                  minY3 = (float)MIN(minY3, pt->recordY->lopr);
                  maxY3 = (float)MAX(maxY3, pt->recordY->hopr);
                }
              else if (Yaxis == 3)
                {
                  for (j = 0; j < pt->recordY->elementCount; j++)
                    CpDataSetXElement(hcp4, iY4 - 1, j, (float)j);
                  minY4 = (float)MIN(minY4, pt->recordY->lopr);
                  maxY4 = (float)MAX(maxY4, pt->recordY->hopr);
                }
              minX = (float)MIN(minX, 0.);
              maxX = (float)MAX(maxX, (float)(pt->recordY->elementCount - 1));
            }
          else
            {
              /* Scalar */
              pt->type = CP_YScalar;
              if (Yaxis <= 0)
                {
                  for (j = 0; j < count; j++)
                    CpDataSetXElement(hcp1, 0, j, (float)j);
                  CpDataSetYElement(hcp1, 0, 0, (float)pt->recordY->value);
                  minY = (float)MIN(minY, pt->recordY->lopr);
                  maxY = (float)MAX(maxY, pt->recordY->hopr);
                }
              else if (Yaxis == 1)
                {
                  for (j = 0; j < count; j++)
                    CpDataSetXElement(hcp2, 0, j, (float)j);
                  CpDataSetYElement(hcp2, 0, 0, (float)pt->recordY->value);
                  minY2 = (float)MIN(minY2, pt->recordY->lopr);
                  maxY2 = (float)MAX(maxY2, pt->recordY->hopr);
                }
              else if (Yaxis == 2)
                {
                  for (j = 0; j < count; j++)
                    CpDataSetXElement(hcp3, 0, j, (float)j);
                  CpDataSetYElement(hcp3, 0, 0, (float)pt->recordY->value);
                  minY3 = (float)MIN(minY3, pt->recordY->lopr);
                  maxY3 = (float)MAX(maxY3, pt->recordY->hopr);
                }
              else if (Yaxis == 3)
                {
                  for (j = 0; j < count; j++)
                    CpDataSetXElement(hcp4, 0, j, (float)j);
                  CpDataSetYElement(hcp4, 0, 0, (float)pt->recordY->value);
                  minY4 = (float)MIN(minY4, pt->recordY->lopr);
                  maxY4 = (float)MAX(maxY4, pt->recordY->hopr);
                }
              minX = (float)MIN(minX, 0.);
              maxX = (float)MAX(maxX, (float)count);
            }
        }
    } /* End for loop over traces */

  /* Record the trace number and set the data pointers in the XYTrace */
  iY1 = iY2 = iY3 = iY4 = 0;
  for (i = 0; i < pcp->nTraces; i++)
    {
      XYTrace *pt = &(pcp->xyTrace[i]);
      Yaxis = dlCartesianPlot->trace[i].yaxis;
      if (Yaxis <= 0)
        {
          pt->hcp = hcp1;
          pt->trace = iY1;
          iY1++;
        }
      else if (Yaxis == 1)
        {
          pt->hcp = hcp2;
          pt->trace = iY2;
          iY2++;
        }
      else if (Yaxis == 2)
        {
          pt->hcp = hcp3;
          pt->trace = iY3;
          iY3++;
        }
      else if (Yaxis == 3)
        {
          pt->hcp = hcp4;
          pt->trace = iY4;
          iY4++;
        }
    }

  /* Fill in connect-time channel-based range specifications
   * This is different than the min/max stored in the display element */
  pcp->axisRange[X_AXIS_ELEMENT].axisMin = minX;
  pcp->axisRange[X_AXIS_ELEMENT].axisMax = maxX;
  pcp->axisRange[Y1_AXIS_ELEMENT].axisMin = minY;
  pcp->axisRange[Y1_AXIS_ELEMENT].axisMax = maxY;
  pcp->axisRange[Y2_AXIS_ELEMENT].axisMin = minY2;
  pcp->axisRange[Y2_AXIS_ELEMENT].axisMax = maxY2;
  pcp->axisRange[Y3_AXIS_ELEMENT].axisMin = minY3;
  pcp->axisRange[Y3_AXIS_ELEMENT].axisMax = maxY3;
  pcp->axisRange[Y4_AXIS_ELEMENT].axisMin = minY4;
  pcp->axisRange[Y4_AXIS_ELEMENT].axisMax = maxY4;
  for (kk = X_AXIS_ELEMENT; kk <= Y4_AXIS_ELEMENT; kk++)
    {
      if (dlCartesianPlot->axis[kk].rangeStyle == CHANNEL_RANGE)
        {
          pcp->axisRange[kk].isCurrentlyFromChannel = True;
        }
      else
        {
          pcp->axisRange[kk].isCurrentlyFromChannel = False;
        }
    }
  minXF.fval = minX;
  maxXF.fval = maxX;
  minYF.fval = minY;
  maxYF.fval = maxY;
  minY2F.fval = minY2;
  maxY2F.fval = maxY2;
  minY3F.fval = minY3;
  maxY3F.fval = maxY3;
  minY4F.fval = minY4;
  maxY4F.fval = maxY4;
  if (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle == CHANNEL_RANGE)
    {
      CpSetAxisMaxMin(w, CP_X, maxXF, minXF);
    }
  else if (dlCartesianPlot->axis[X_AXIS_ELEMENT].rangeStyle == USER_SPECIFIED_RANGE)
    {
      int k;

      minXF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].minRange;
      maxXF.fval = dlCartesianPlot->axis[X_AXIS_ELEMENT].maxRange;
      tickF.fval = (float)((maxXF.fval - minXF.fval) / 4.0);
      sprintf(string, "%f", tickF.fval);
      k = strlen(string) - 1;
      while (string[k] == '0')
        k--; /* strip off trailing zeroes */
      iPrec = k;
      while (string[k] != '.' && k >= 0)
        k--;
      iPrec = iPrec - k;
      CpSetAxisAll(w, CP_X, maxXF, minXF, tickF,
                   tickF, iPrec);
    }

  /* Set axis parameters for Y1 */
  if (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle == CHANNEL_RANGE)
    {
      CpSetAxisMaxMin(w, CP_Y, maxYF, minYF);
    }
  else if (dlCartesianPlot->axis[Y1_AXIS_ELEMENT].rangeStyle == USER_SPECIFIED_RANGE)
    {
      int k;

      minYF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].minRange;
      maxYF.fval = dlCartesianPlot->axis[Y1_AXIS_ELEMENT].maxRange;
      tickF.fval = (float)((maxYF.fval - minYF.fval) / 4.0);
      sprintf(string, "%f", tickF.fval);
      k = strlen(string) - 1;
      while (string[k] == '0')
        k--; /* strip off trailing zeroes */
      iPrec = k;
      while (string[k] != '.' && k >= 0)
        k--;
      iPrec = iPrec - k;
      CpSetAxisAll(w, CP_Y, maxYF, minYF, tickF,
                   tickF, iPrec);
    }

  /* Set axis parameters for Y2 */
  if (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle == CHANNEL_RANGE)
    {
      CpSetAxisMaxMin(w, CP_Y2, maxY2F, minY2F);
    }
  else if (dlCartesianPlot->axis[Y2_AXIS_ELEMENT].rangeStyle == USER_SPECIFIED_RANGE)
    {
      int k;

      minY2F.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].minRange;
      maxY2F.fval = dlCartesianPlot->axis[Y2_AXIS_ELEMENT].maxRange;
      tickF.fval = (float)((maxY2F.fval - minY2F.fval) / 4.0);
      sprintf(string, "%f", tickF.fval);
      k = strlen(string) - 1;
      while (string[k] == '0')
        k--; /* strip off trailing zeroes */
      iPrec = k;
      while (string[k] != '.' && k >= 0)
        k--;
      iPrec = iPrec - k;
      CpSetAxisAll(w, CP_Y2, maxY2F, minY2F, tickF,
                   tickF, iPrec);
    }

  /* Set axis parameters for Y3 */
  if (dlCartesianPlot->axis[Y3_AXIS_ELEMENT].rangeStyle == CHANNEL_RANGE)
    {
      CpSetAxisMaxMin(w, CP_Y3, maxY3F, minY3F);
    }
  else if (dlCartesianPlot->axis[Y3_AXIS_ELEMENT].rangeStyle == USER_SPECIFIED_RANGE)
    {
      int k;

      minY3F.fval = dlCartesianPlot->axis[Y3_AXIS_ELEMENT].minRange;
      maxY3F.fval = dlCartesianPlot->axis[Y3_AXIS_ELEMENT].maxRange;
      tickF.fval = (float)((maxY3F.fval - minY3F.fval) / 4.0);
      sprintf(string, "%f", tickF.fval);
      k = strlen(string) - 1;
      while (string[k] == '0')
        k--; /* strip off trailing zeroes */
      iPrec = k;
      while (string[k] != '.' && k >= 0)
        k--;
      iPrec = iPrec - k;
      CpSetAxisAll(w, CP_Y3, maxY3F, minY3F, tickF,
                   tickF, iPrec);
    }

  /* Set axis parameters for Y4 */
  if (dlCartesianPlot->axis[Y4_AXIS_ELEMENT].rangeStyle == CHANNEL_RANGE)
    {
      CpSetAxisMaxMin(w, CP_Y4, maxY4F, minY4F);
    }
  else if (dlCartesianPlot->axis[Y4_AXIS_ELEMENT].rangeStyle == USER_SPECIFIED_RANGE)
    {
      int k;

      minY4F.fval = dlCartesianPlot->axis[Y4_AXIS_ELEMENT].minRange;
      maxY4F.fval = dlCartesianPlot->axis[Y4_AXIS_ELEMENT].maxRange;
      tickF.fval = (float)((maxY4F.fval - minY4F.fval) / 4.0);
      sprintf(string, "%f", tickF.fval);
      k = strlen(string) - 1;
      while (string[k] == '0')
        k--; /* strip off trailing zeroes */
      iPrec = k;
      while (string[k] != '.' && k >= 0)
        k--;
      iPrec = iPrec - k;
      CpSetAxisAll(w, CP_Y4, maxY4F, minY4F, tickF,
                   tickF, iPrec);
    }

  return firstTime;
}

static void cartesianPlotDestroyCb(XtPointer cd)
{
  MedmCartesianPlot *pcp = (MedmCartesianPlot *)cd;

  if (executeTimeCartesianPlotWidget &&
      executeTimeCartesianPlotWidget == pcp->dlElement->widget)
    {
      executeTimeCartesianPlotWidget = NULL;
      XtSetSensitive(cartesianPlotAxisS, False);
    }
  if (pcp)
    {
      int i;
      for (i = 0; i < pcp->nTraces; i++)
        {
          Record *pr;
          pr = pcp->xyTrace[i].recordX;
          if (pr)
            {
              medmDestroyRecord(pr);
            }
          pr = pcp->xyTrace[i].recordY;
          if (pr)
            {
              medmDestroyRecord(pr);
            }
        }
      if (pcp->eraseCh.recordX)
        medmDestroyRecord(pcp->eraseCh.recordX);
      if (pcp->triggerCh.recordX)
        medmDestroyRecord(pcp->triggerCh.recordX);
      if (pcp->countCh.recordX)
        medmDestroyRecord(pcp->countCh.recordX);
      if (pcp->hcp1)
        CpDataDestroy(pcp->hcp1);
      if (pcp->hcp2)
        CpDataDestroy(pcp->hcp2);
      if (pcp->hcp3)
        CpDataDestroy(pcp->hcp3);
      if (pcp->hcp4)
        CpDataDestroy(pcp->hcp4);
      if (pcp->dlElement)
        pcp->dlElement->data = NULL;
      free((char *)pcp);
    }
  return;
}

static void cartesianPlotDraw(XtPointer cd)
{
  MedmCartesianPlot *pcp = (MedmCartesianPlot *)cd;
  DlElement *dlElement = pcp->dlElement;
  Widget w = dlElement->widget;
  int i;
  Boolean connected = True;
  Boolean readAccess = True;
  Boolean validPrecision = True;
  double currentTime;

  /* Check if hidden */
  if (dlElement->hidden)
    {
      if (w && XtIsManaged(w))
        {
          XtUnmanageChild(w);
        }
      return;
    }

  /* Check for connection */
  for (i = 0; i < pcp->nTraces; i++)
    {
      Record *pr;
      pr = pcp->xyTrace[i].recordX;
      if (pr)
        {
          if (pr->connected == FALSE)
            {
              connected = False;
              break;
            }
          else if (!pr->readAccess)
            {
              readAccess = False;
            }
          if (pr->precision < 0)
            validPrecision = False;
        }
      pr = pcp->xyTrace[i].recordY;
      if (pr)
        {
          if (!pr->connected)
            {
              connected = False;
              break;
            }
          else if (!pr->readAccess)
            {
              readAccess = False;
            }
          if (pr->precision < 0)
            validPrecision = False;
        }
    }
  // This is code that is supposed to run every second to update the plot in case the PV has not updated
  currentTime = getTimerDouble();
  for (i = 0; i < pcp->nTraces; i++)
    {
      XYTrace *pt = &(pcp->xyTrace[i]);
      if (pt->recordX != NULL) //Do not update plot if X axis is not time or counts
        continue;
      //Do not update plot if the PV changed within the last second.
      //If it did, then the plot already updated.
      if ((pt->recordY) &&
          (pt->hcp) &&
          (pt->recordY->connected) &&
          ((currentTime - timeOffset - pt->recordY->time.secPastEpoch) > 1))
        {
          cartesianPlotUpdateTrace((XtPointer)pt->recordY, True);
          if (pt->hcp == pcp->hcp1)
            pcp->dirty1 = True;
          else if (pt->hcp == pcp->hcp2)
            pcp->dirty2 = True;
          else if (pt->hcp == pcp->hcp3)
            pcp->dirty3 = True;
          else if (pt->hcp == pcp->hcp4)
            pcp->dirty4 = True;
        }
    }

  if (pcp->eraseCh.recordX)
    {
      Record *pr = pcp->eraseCh.recordX;
      if (!pr->connected)
        {
          connected = False;
        }
      else if (!pr->readAccess)
        {
          readAccess = False;
        }
      if (pr->precision < 0)
        validPrecision = False;
    }
  if (pcp->triggerCh.recordX)
    {
      Record *pr = pcp->triggerCh.recordX;
      if (!pr->connected)
        {
          connected = False;
        }
      else if (!pr->readAccess)
        {
          readAccess = False;
        }
      if (pr->precision < 0)
        validPrecision = False;
    }
  if (pcp->countCh.recordX)
    {
      Record *pr = pcp->countCh.recordX;
      if (!pr->connected)
        {
          connected = False;
        }
      else if (!pr->readAccess)
        {
          readAccess = False;
        }
      if (pr->precision < 0)
        validPrecision = False;
    }
  if (connected)
    {
      if (readAccess)
        {
          if (w && validPrecision)
            {
              if (pcp->dirty1)
                {
                  pcp->dirty1 = False;
                  CpSetData(w, CP_Y, pcp->hcp1);
                }
              if (pcp->dirty2)
                {
                  pcp->dirty2 = False;
                  CpSetData(w, CP_Y2, pcp->hcp2);
                }
              if (pcp->dirty3)
                {
                  pcp->dirty3 = False;
                  CpSetData(w, CP_Y3, pcp->hcp3);
                }
              if (pcp->dirty4)
                {
                  pcp->dirty4 = False;
                  CpSetData(w, CP_Y4, pcp->hcp4);
                }
              addCommonHandlers(w, pcp->updateTask->displayInfo);
              XtManageChild(w);
            }
        }
      else
        {
          if (w && XtIsManaged(w))
            XtUnmanageChild(w);
          drawBlackRectangle(pcp->updateTask);
        }
    }
  else
    {
      if (w && XtIsManaged(w))
        XtUnmanageChild(w);
      drawWhiteRectangle(pcp->updateTask);
    }
  /* KE: Used to use CP_FAST here.  SciPlot autoscales to larger, but
     not to smaller then. */
  CpUpdateWidget(w, CP_FULL);
}

static void cartesianPlotGetRecord(XtPointer cd, Record **record, int *count)
{
  MedmCartesianPlot *pcp = (MedmCartesianPlot *)cd;
  int i, j;

  j = 0;
  for (i = 0; i < pcp->nTraces; i++)
    {
      XYTrace *pt = &(pcp->xyTrace[i]);
      record[j++] = pt->recordX;
      record[j++] = pt->recordY;
    }
  if (pcp->eraseCh.recordX)
    {
      record[j++] = pcp->eraseCh.recordX;
    }
  else
    { /* Count these even if NULL */
      record[j++] = NULL;
    }
  if (pcp->triggerCh.recordX)
    {
      record[j++] = pcp->triggerCh.recordX;
    }
  else
    { /* Count these even if NULL */
      record[j++] = NULL;
    }
  if (pcp->countCh.recordX)
    {
      record[j++] = pcp->countCh.recordX;
    }
  else
    { /* Count these even if NULL */
      record[j++] = NULL;
    }

  /* 200 means two columns */
  *count = j + 200;
}

DlElement *createDlCartesianPlot(DlElement *p)
{
  DlCartesianPlot *dlCartesianPlot;
  DlElement *dlElement;
  int traceNumber;

  dlCartesianPlot = (DlCartesianPlot *)malloc(sizeof(DlCartesianPlot));
  if (!dlCartesianPlot)
    return 0;
  if (p)
    {
      *dlCartesianPlot = *p->structure.cartesianPlot;
    }
  else
    {
      objectAttributeInit(&(dlCartesianPlot->object));
      plotcomAttributeInit(&(dlCartesianPlot->plotcom));
      dlCartesianPlot->count = 1;
      dlCartesianPlot->style = POINT_PLOT;
      dlCartesianPlot->erase_oldest = ERASE_OLDEST_OFF;
      for (traceNumber = 0; traceNumber < MAX_TRACES; traceNumber++)
        {
          traceAttributeInit(&(dlCartesianPlot->trace[traceNumber]));
          if (traceNumber == 0)
            {
              dlCartesianPlot->trace[traceNumber].yaxis = 0;
              dlCartesianPlot->trace[traceNumber].yside = 0;
            }
        }
      plotAxisDefinitionInit(&(dlCartesianPlot->axis[X_AXIS_ELEMENT]));
      plotAxisDefinitionInit(&(dlCartesianPlot->axis[Y1_AXIS_ELEMENT]));
      plotAxisDefinitionInit(&(dlCartesianPlot->axis[Y2_AXIS_ELEMENT]));
      plotAxisDefinitionInit(&(dlCartesianPlot->axis[Y3_AXIS_ELEMENT]));
      plotAxisDefinitionInit(&(dlCartesianPlot->axis[Y4_AXIS_ELEMENT]));
      dlCartesianPlot->trigger[0] = '\0';
      dlCartesianPlot->erase[0] = '\0';
      dlCartesianPlot->countPvName[0] = '\0';
      dlCartesianPlot->eraseMode = ERASE_IF_NOT_ZERO;
    }

  if (!(dlElement = createDlElement(DL_CartesianPlot,
                                    (XtPointer)dlCartesianPlot,
                                    &cartesianPlotDlDispatchTable)))
    {
      free(dlCartesianPlot);
    }

  return (dlElement);
}

DlElement *parseCartesianPlot(DisplayInfo *displayInfo)
{
  char token[MAX_TOKEN_LENGTH];
  TOKEN tokenType;
  int nestingLevel = 0;
  DlCartesianPlot *dlCartesianPlot;
  DlElement *dlElement = createDlCartesianPlot(NULL);
  int traceNumber;

  if (!dlElement)
    return 0;
  dlCartesianPlot = dlElement->structure.cartesianPlot;
  do
    {
      switch ((tokenType = getToken(displayInfo, token)))
        {
        case T_WORD:
          if (!strcmp(token, "object"))
            parseObject(displayInfo, &(dlCartesianPlot->object));
          else if (!strcmp(token, "plotcom"))
            parsePlotcom(displayInfo, &(dlCartesianPlot->plotcom));
          else if (!strcmp(token, "count"))
            {
              getToken(displayInfo, token);
              getToken(displayInfo, token);
              dlCartesianPlot->count = atoi(token);
              strcpy(dlCartesianPlot->countPvName, token);
            }
          else if (!strcmp(token, "style"))
            {
              getToken(displayInfo, token);
              getToken(displayInfo, token);
              if (!strcmp(token, "point plot"))
                dlCartesianPlot->style = POINT_PLOT;
              else if (!strcmp(token, "point"))
                dlCartesianPlot->style = POINT_PLOT;
              else if (!strcmp(token, "line plot"))
                dlCartesianPlot->style = LINE_PLOT;
              else if (!strcmp(token, "line"))
                dlCartesianPlot->style = LINE_PLOT;
              else if (!strcmp(token, "step"))
                dlCartesianPlot->style = STEP_PLOT;
              else if (!strcmp(token, "fill under"))
                dlCartesianPlot->style = FILL_UNDER_PLOT;
              else if (!strcmp(token, "fill-under"))
                dlCartesianPlot->style = FILL_UNDER_PLOT;
            }
          else if (!strcmp(token, "erase_oldest"))
            {
              getToken(displayInfo, token);
              getToken(displayInfo, token);
              if (!strcmp(token, "on"))
                dlCartesianPlot->erase_oldest = ERASE_OLDEST_ON;
              else if (!strcmp(token, "off"))
                dlCartesianPlot->erase_oldest = ERASE_OLDEST_OFF;
              else if (!strcmp(token, "plot last n pts"))
                dlCartesianPlot->erase_oldest = ERASE_OLDEST_ON;
              else if (!strcmp(token, "plot n pts & stop"))
                dlCartesianPlot->erase_oldest = ERASE_OLDEST_OFF;
            }
          else if (!strncmp(token, "trace", 5))
            {
              traceNumber = MIN(token[6] - '0', MAX_TRACES - 1);
              if (traceNumber == 0)
                {
                  dlCartesianPlot->trace[traceNumber].yaxis = 0;
                  dlCartesianPlot->trace[traceNumber].yside = 0;
                }
              parseTrace(displayInfo,
                         &(dlCartesianPlot->trace[traceNumber]));
              {
                int i;
                for (i = 0; i < traceNumber; i++)
                  {
                    if (dlCartesianPlot->trace[traceNumber].yaxis == dlCartesianPlot->trace[i].yaxis)
                      {
                        dlCartesianPlot->trace[traceNumber].yside = dlCartesianPlot->trace[i].yside;
                        break;
                      }
                  }
              }
            }
          else if (!strcmp(token, "x_axis"))
            {
              parsePlotAxisDefinition(displayInfo,
                                      &(dlCartesianPlot->axis[X_AXIS_ELEMENT]));
            }
          else if (!strcmp(token, "y1_axis"))
            {
              parsePlotAxisDefinition(displayInfo,
                                      &(dlCartesianPlot->axis[Y1_AXIS_ELEMENT]));
            }
          else if (!strcmp(token, "y2_axis"))
            {
              parsePlotAxisDefinition(displayInfo,
                                      &(dlCartesianPlot->axis[Y2_AXIS_ELEMENT]));
            }
          else if (!strcmp(token, "y3_axis"))
            {
              parsePlotAxisDefinition(displayInfo,
                                      &(dlCartesianPlot->axis[Y3_AXIS_ELEMENT]));
            }
          else if (!strcmp(token, "y4_axis"))
            {
              parsePlotAxisDefinition(displayInfo,
                                      &(dlCartesianPlot->axis[Y4_AXIS_ELEMENT]));
            }
          else if (!strcmp(token, "trigger"))
            {
              getToken(displayInfo, token);
              getToken(displayInfo, token);
              strcpy(dlCartesianPlot->trigger, token);
            }
          else if (!strcmp(token, "erase"))
            {
              getToken(displayInfo, token);
              getToken(displayInfo, token);
              strcpy(dlCartesianPlot->erase, token);
            }
          else if (!strcmp(token, "countPvName"))
            {
              getToken(displayInfo, token);
              getToken(displayInfo, token);
              dlCartesianPlot->count = atoi(token);
              strcpy(dlCartesianPlot->countPvName, token);
            }
          else if (!strcmp(token, "eraseMode"))
            {
              getToken(displayInfo, token);
              getToken(displayInfo, token);
              if (!strcmp(token, "if not zero"))
                dlCartesianPlot->eraseMode = ERASE_IF_NOT_ZERO;
              else if (!strcmp(token, "if zero"))
                dlCartesianPlot->eraseMode = ERASE_IF_ZERO;
            }
          break;
        case T_EQUAL:
          break;
        case T_LEFT_BRACE:
          nestingLevel++;
          break;
        case T_RIGHT_BRACE:
          nestingLevel--;
          break;
        default:
          break;
        }
    }
  while ((tokenType != T_RIGHT_BRACE) && (nestingLevel > 0) && (tokenType != T_EOF));

  return dlElement;
}

void writeDlCartesianPlot(FILE *stream, DlElement *dlElement, int level)
{
  int i;
  char indent[16];
  DlCartesianPlot *dlCartesianPlot = dlElement->structure.cartesianPlot;

  for (i = 0; i < level; i++)
    indent[i] = '\t';
  indent[i] = '\0';

  fprintf(stream, "\n%s\"cartesian plot\" {", indent);
  writeDlObject(stream, &(dlCartesianPlot->object), level + 1);
  writeDlPlotcom(stream, &(dlCartesianPlot->plotcom), level + 1);
#ifdef SUPPORT_0201XX_FILE_FORMAT
  if (MedmUseNewFileFormat)
    {
#endif
      if (dlCartesianPlot->style != POINT_PLOT)
        fprintf(stream, "\n%s\tstyle=\"%s\"", indent,
                stringValueTable[dlCartesianPlot->style]);
      if (dlCartesianPlot->erase_oldest != ERASE_OLDEST_OFF)
        fprintf(stream, "\n%s\terase_oldest=\"%s\"", indent,
                stringValueTable[dlCartesianPlot->erase_oldest]);
      if (dlCartesianPlot->count != 1)
        fprintf(stream, "\n%s\tcount=\"%s\"", indent, dlCartesianPlot->countPvName);
      for (i = 0; i < MAX_TRACES; i++)
        {
          writeDlTrace(stream, &(dlCartesianPlot->trace[i]), i, level + 1);
        }
      writeDlPlotAxisDefinition(stream,
                                &(dlCartesianPlot->axis[X_AXIS_ELEMENT]), X_AXIS_ELEMENT, level + 1);
      writeDlPlotAxisDefinition(stream,
                                &(dlCartesianPlot->axis[Y1_AXIS_ELEMENT]), Y1_AXIS_ELEMENT, level + 1);
      writeDlPlotAxisDefinition(stream,
                                &(dlCartesianPlot->axis[Y2_AXIS_ELEMENT]), Y2_AXIS_ELEMENT, level + 1);
      writeDlPlotAxisDefinition(stream,
                                &(dlCartesianPlot->axis[Y3_AXIS_ELEMENT]), Y3_AXIS_ELEMENT, level + 1);
      writeDlPlotAxisDefinition(stream,
                                &(dlCartesianPlot->axis[Y4_AXIS_ELEMENT]), Y4_AXIS_ELEMENT, level + 1);
      if (dlCartesianPlot->trigger[0] != '\0')
        fprintf(stream, "\n%s\ttrigger=\"%s\"", indent, dlCartesianPlot->trigger);
      if (dlCartesianPlot->erase[0] != '\0')
        fprintf(stream, "\n%s\terase=\"%s\"", indent, dlCartesianPlot->erase);
      if (dlCartesianPlot->countPvName[0] != '\0')
        fprintf(stream, "\n%s\tcountPvName=\"%s\"", indent, dlCartesianPlot->countPvName);
      if (dlCartesianPlot->eraseMode != ERASE_IF_NOT_ZERO)
        fprintf(stream, "\n%s\teraseMode=\"%s\"", indent,
                stringValueTable[dlCartesianPlot->eraseMode]);
#ifdef SUPPORT_0201XX_FILE_FORMAT
    }
  else
    {
      fprintf(stream, "\n%s\tstyle=\"%s\"", indent,
              stringValueTable[dlCartesianPlot->style]);
      fprintf(stream, "\n%s\terase_oldest=\"%s\"", indent,
              stringValueTable[dlCartesianPlot->erase_oldest]);
      fprintf(stream, "\n%s\tcount=\"%s\"", indent, dlCartesianPlot->countPvName);
      for (i = 0; i < MAX_TRACES; i++)
        {
          writeDlTrace(stream, &(dlCartesianPlot->trace[i]), i, level + 1);
        }
      writeDlPlotAxisDefinition(stream,
                                &(dlCartesianPlot->axis[X_AXIS_ELEMENT]), X_AXIS_ELEMENT, level + 1);
      writeDlPlotAxisDefinition(stream,
                                &(dlCartesianPlot->axis[Y1_AXIS_ELEMENT]), Y1_AXIS_ELEMENT, level + 1);
      writeDlPlotAxisDefinition(stream,
                                &(dlCartesianPlot->axis[Y2_AXIS_ELEMENT]), Y2_AXIS_ELEMENT, level + 1);
      writeDlPlotAxisDefinition(stream,
                                &(dlCartesianPlot->axis[Y3_AXIS_ELEMENT]), Y3_AXIS_ELEMENT, level + 1);
      writeDlPlotAxisDefinition(stream,
                                &(dlCartesianPlot->axis[Y4_AXIS_ELEMENT]), Y4_AXIS_ELEMENT, level + 1);
      fprintf(stream, "\n%s\ttrigger=\"%s\"", indent, dlCartesianPlot->trigger);
      fprintf(stream, "\n%s\terase=\"%s\"", indent, dlCartesianPlot->erase);
      fprintf(stream, "\n%s\tcountPvName=\"%s\"", indent, dlCartesianPlot->countPvName);
      fprintf(stream, "\n%s\teraseMode=\"%s\"", indent,
              stringValueTable[dlCartesianPlot->eraseMode]);
    }
#endif
  fprintf(stream, "\n%s}", indent);
}

static void cartesianPlotInheritValues(ResourceBundle *pRCB, DlElement *p)
{
  DlCartesianPlot *dlCartesianPlot = p->structure.cartesianPlot;
  medmGetValues(pRCB,
                CLR_RC, &(dlCartesianPlot->plotcom.clr),
                BCLR_RC, &(dlCartesianPlot->plotcom.bclr),
                -1);
}

static void cartesianPlotGetValues(ResourceBundle *pRCB, DlElement *p)
{
  DlCartesianPlot *dlCartesianPlot = p->structure.cartesianPlot;
  medmGetValues(pRCB,
                X_RC, &(dlCartesianPlot->object.x),
                Y_RC, &(dlCartesianPlot->object.y),
                WIDTH_RC, &(dlCartesianPlot->object.width),
                HEIGHT_RC, &(dlCartesianPlot->object.height),
                TITLE_RC, &(dlCartesianPlot->plotcom.title),
                XLABEL_RC, &(dlCartesianPlot->plotcom.xlabel),
                YLABEL_RC, &(dlCartesianPlot->plotcom.ylabel),
                Y2LABEL_RC, &(dlCartesianPlot->plotcom.y2label),
                Y3LABEL_RC, &(dlCartesianPlot->plotcom.y3label),
                Y4LABEL_RC, &(dlCartesianPlot->plotcom.y4label),
                CLR_RC, &(dlCartesianPlot->plotcom.clr),
                BCLR_RC, &(dlCartesianPlot->plotcom.bclr),
                COUNT_RC, &(dlCartesianPlot->countPvName),
                CSTYLE_RC, &(dlCartesianPlot->style),
                ERASE_OLDEST_RC, &(dlCartesianPlot->erase_oldest),
                CPDATA_RC, &(dlCartesianPlot->trace),
                CPAXIS_RC, &(dlCartesianPlot->axis),
                TRIGGER_RC, &(dlCartesianPlot->trigger),
                ERASE_RC, &(dlCartesianPlot->erase),
                ERASE_MODE_RC, &(dlCartesianPlot->eraseMode),
                -1);
}

static void cartesianPlotSetBackgroundColor(ResourceBundle *pRCB, DlElement *p)
{
  DlCartesianPlot *dlCartesianPlot = p->structure.cartesianPlot;
  medmGetValues(pRCB,
                BCLR_RC, &(dlCartesianPlot->plotcom.bclr),
                -1);
}

static void cartesianPlotSetForegroundColor(ResourceBundle *pRCB, DlElement *p)
{
  DlCartesianPlot *dlCartesianPlot = p->structure.cartesianPlot;
  medmGetValues(pRCB,
                CLR_RC, &(dlCartesianPlot->plotcom.clr),
                -1);
}

/*
 * Set Cartesian Plot Axis attributes
 * (complex - has to handle both EDIT and EXECUTE time interactions)
 */
static void cpAxisOptionMenuSimpleCallback(Widget w, XtPointer cd, XtPointer cbs)
{
  DisplayInfo *cdi = currentDisplayInfo;
  int buttonId = (intptr_t)cd;
  int k, rcType, iPrec;
  char string[24];
  char *stringMinValue, *stringMaxValue;
  XcVType minF, maxF, tickF;
  XtPointer userData;
  MedmCartesianPlot *pcp = NULL;
  //DlCartesianPlot *dlCartesianPlot = NULL;

  UNREFERENCED(cbs);

  /* Get current cartesian plot */
  if (globalDisplayListTraversalMode == DL_EXECUTE)
    {
      if (executeTimeCartesianPlotWidget)
        {
          XtVaGetValues(executeTimeCartesianPlotWidget,
                        XmNuserData, &userData, NULL);
          pcp = (MedmCartesianPlot *)userData;
          /*
            if(pcp)
            dlCartesianPlot = (DlCartesianPlot *)pcp->dlElement->structure.cartesianPlot;
          */
        }
      else
        {
          medmPostMsg(1, "cpAxisOptionMenuSimpleCallback: "
                      "Element is no longer valid\n");
          XtPopdown(cartesianPlotAxisS);
          return;
        }
    }

  /* rcType (and therefore which option menu...) is stored in userData */
  XtVaGetValues(XtParent(w), XmNuserData, &rcType, NULL);
  switch (rcType)
    {
    case CP_X_AXIS_STYLE:
    case CP_Y_AXIS_STYLE:
    case CP_Y2_AXIS_STYLE:
    case CP_Y3_AXIS_STYLE:
    case CP_Y4_AXIS_STYLE:
      {
        CartesianPlotAxisStyle style = (CartesianPlotAxisStyle)(FIRST_CARTESIAN_PLOT_AXIS_STYLE + buttonId);

        globalResourceBundle.axis[rcType - CP_X_AXIS_STYLE].axisStyle = style;
        switch (rcType)
          {
          case CP_X_AXIS_STYLE:
            switch (style)
              {
              case LINEAR_AXIS:
                if (globalDisplayListTraversalMode == DL_EXECUTE)
                  {
                    CpSetAxisLinear(executeTimeCartesianPlotWidget, CP_X);
                  }
                XtSetSensitive(axisTimeFormat, False);
                if (pcp)
                  pcp->timeScale = False;
                break;
              case LOG10_AXIS:
                if (globalDisplayListTraversalMode == DL_EXECUTE)
                  {
                    CpSetAxisLog(executeTimeCartesianPlotWidget, CP_X);
                  }
                XtSetSensitive(axisTimeFormat, False);
                if (pcp)
                  pcp->timeScale = False;
                break;
              case TIME_AXIS:
                if (globalDisplayListTraversalMode == DL_EXECUTE)
                  {
                    CpSetAxisTime(executeTimeCartesianPlotWidget, CP_X,
                                  time900101, cpTimeFormatString[(int)globalResourceBundle.axis[0].timeFormat - FIRST_CP_TIME_FORMAT]);
                  }
                XtSetSensitive(axisTimeFormat, True);
                if (pcp)
                  pcp->timeScale = True;
                break;
              }
            break;
          case CP_Y_AXIS_STYLE:
            if (globalDisplayListTraversalMode == DL_EXECUTE)
              {
                CpSetAxisLinear(executeTimeCartesianPlotWidget, CP_Y);
                (style == LOG10_AXIS) ? CpSetAxisLog(executeTimeCartesianPlotWidget, CP_Y) : CpSetAxisLinear(executeTimeCartesianPlotWidget, CP_Y);
              }
            break;
          case CP_Y2_AXIS_STYLE:
            if (globalDisplayListTraversalMode == DL_EXECUTE)
              {
                CpSetAxisLinear(executeTimeCartesianPlotWidget, CP_Y2);
                (style == LOG10_AXIS) ? CpSetAxisLog(executeTimeCartesianPlotWidget, CP_Y2) : CpSetAxisLinear(executeTimeCartesianPlotWidget, CP_Y2);
              }
            break;
          case CP_Y3_AXIS_STYLE:
            if (globalDisplayListTraversalMode == DL_EXECUTE)
              {
                CpSetAxisLinear(executeTimeCartesianPlotWidget, CP_Y3);
                (style == LOG10_AXIS) ? CpSetAxisLog(executeTimeCartesianPlotWidget, CP_Y3) : CpSetAxisLinear(executeTimeCartesianPlotWidget, CP_Y3);
              }
            break;
          case CP_Y4_AXIS_STYLE:
            if (globalDisplayListTraversalMode == DL_EXECUTE)
              {
                CpSetAxisLinear(executeTimeCartesianPlotWidget, CP_Y4);
                (style == LOG10_AXIS) ? CpSetAxisLog(executeTimeCartesianPlotWidget, CP_Y4) : CpSetAxisLinear(executeTimeCartesianPlotWidget, CP_Y4);
              }
            break;
          }
        break;
      }
    case CP_X_RANGE_STYLE:
    case CP_Y_RANGE_STYLE:
    case CP_Y2_RANGE_STYLE:
    case CP_Y3_RANGE_STYLE:
    case CP_Y4_RANGE_STYLE:
      globalResourceBundle.axis[rcType - CP_X_RANGE_STYLE].rangeStyle = (CartesianPlotRangeStyle)(FIRST_CARTESIAN_PLOT_RANGE_STYLE + buttonId);
      switch (globalResourceBundle.axis[rcType % 5].rangeStyle)
        {
        case USER_SPECIFIED_RANGE:
          XtSetSensitive(axisRangeMinRC[rcType % 5], True);
          XtSetSensitive(axisRangeMaxRC[rcType % 5], True);
          if (globalDisplayListTraversalMode == DL_EXECUTE)
            {
              /* Determine max and min values */
              stringMinValue = XmTextFieldGetString(axisRangeMin[rcType % 5]);
              stringMaxValue = XmTextFieldGetString(axisRangeMax[rcType % 5]);
              if (*stringMinValue && *stringMaxValue)
                {
                  /* Values have been entered */
                  minF.fval = (float)atof(stringMinValue);
                  maxF.fval = (float)atof(stringMaxValue);
                }
              else if (executeTimeCartesianPlotWidget)
                {
                  /* Get values from the widget */
                  CpGetAxisMaxMin(executeTimeCartesianPlotWidget,
                                  CP_X + rcType % 5, &maxF, &minF);
                }
              else
                {
                  /* Get values from the resource palette */
                  minF.fval = globalResourceBundle.axis[rcType % 5].minRange;
                  maxF.fval = globalResourceBundle.axis[rcType % 5].maxRange;
                }
              XtFree(stringMinValue);
              XtFree(stringMaxValue);

              /* Set values in the text fields */
              sprintf(string, "%f", minF.fval);
              XmTextFieldSetString(axisRangeMin[rcType % 5], string);
              sprintf(string, "%f", maxF.fval);
              XmTextFieldSetString(axisRangeMax[rcType % 5], string);

              /* Set the values in the widget */
              tickF.fval = (float)((maxF.fval - minF.fval) / 4.0);
              sprintf(string, "%f", tickF.fval);
              k = strlen(string) - 1;
              while (string[k] == '0')
                k--; /* strip off trailing zeroes */
              iPrec = k;
              while (string[k] != '.' && k >= 0)
                k--;
              iPrec = iPrec - k;
              switch (rcType % 5)
                {
                case X_AXIS_ELEMENT:
                  CpSetAxisAll(executeTimeCartesianPlotWidget, CP_X,
                               maxF, minF, tickF, tickF, iPrec);
                  break;
                case Y1_AXIS_ELEMENT:
                  CpSetAxisAll(executeTimeCartesianPlotWidget, CP_Y,
                               maxF, minF, tickF, tickF, iPrec);
                  break;
                case Y2_AXIS_ELEMENT:
                  CpSetAxisAll(executeTimeCartesianPlotWidget, CP_Y2,
                               maxF, minF, tickF, tickF, iPrec);
                  break;
                case Y3_AXIS_ELEMENT:
                  CpSetAxisAll(executeTimeCartesianPlotWidget, CP_Y3,
                               maxF, minF, tickF, tickF, iPrec);
                  break;
                case Y4_AXIS_ELEMENT:
                  CpSetAxisAll(executeTimeCartesianPlotWidget, CP_Y4,
                               maxF, minF, tickF, tickF, iPrec);
                  break;
                }
            }
          if (pcp)
            pcp->axisRange[rcType % 5].isCurrentlyFromChannel = False;
          break;
        case CHANNEL_RANGE:
          XtSetSensitive(axisRangeMinRC[rcType % 5], False);
          XtSetSensitive(axisRangeMaxRC[rcType % 5], False);
          if (globalDisplayListTraversalMode == DL_EXECUTE)
            {
              if (pcp)
                {
                  /* get channel-based range specifiers - NB: these are
                   *   different than the display element version of these
                   *   which are the user-specified values
                   */
                  minF.fval = pcp->axisRange[rcType % 5].axisMin;
                  maxF.fval = pcp->axisRange[rcType % 5].axisMax;
                }
              switch (rcType % 5)
                {
                case X_AXIS_ELEMENT:
                  CpSetAxisChannel(executeTimeCartesianPlotWidget, CP_X,
                                   maxF, minF);
                  break;
                case Y1_AXIS_ELEMENT:
                  CpSetAxisChannel(executeTimeCartesianPlotWidget, CP_Y,
                                   maxF, minF);
                  break;
                case Y2_AXIS_ELEMENT:
                  CpSetAxisChannel(executeTimeCartesianPlotWidget, CP_Y2,
                                   maxF, minF);
                  break;
                case Y3_AXIS_ELEMENT:
                  CpSetAxisChannel(executeTimeCartesianPlotWidget, CP_Y3,
                                   maxF, minF);
                  break;
                case Y4_AXIS_ELEMENT:
                  CpSetAxisChannel(executeTimeCartesianPlotWidget, CP_Y4,
                                   maxF, minF);
                  break;
                }
            }
          if (pcp)
            pcp->axisRange[rcType % 5].isCurrentlyFromChannel = True;
          break;
        case AUTO_SCALE_RANGE:
          XtSetSensitive(axisRangeMinRC[rcType % 5], False);
          XtSetSensitive(axisRangeMaxRC[rcType % 5], False);
          if (globalDisplayListTraversalMode == DL_EXECUTE)
            {
              switch (rcType % 5)
                {
                case X_AXIS_ELEMENT:
                  CpSetAxisAuto(executeTimeCartesianPlotWidget, CP_X);
                  break;
                case Y1_AXIS_ELEMENT:
                  CpSetAxisAuto(executeTimeCartesianPlotWidget, CP_Y);
                  break;
                case Y2_AXIS_ELEMENT:
                  CpSetAxisAuto(executeTimeCartesianPlotWidget, CP_Y2);
                  break;
                case Y3_AXIS_ELEMENT:
                  CpSetAxisAuto(executeTimeCartesianPlotWidget, CP_Y3);
                  break;
                case Y4_AXIS_ELEMENT:
                  CpSetAxisAuto(executeTimeCartesianPlotWidget, CP_Y4);
                  break;
                }
            }
          if (pcp)
            pcp->axisRange[rcType % 5].isCurrentlyFromChannel = False;
          break;
        default:
          break;
        }
      break;
    case CP_X_TIME_FORMAT:
      globalResourceBundle.axis[0].timeFormat =
        (CartesianPlotTimeFormat_t)(FIRST_CP_TIME_FORMAT + buttonId);
      if (globalDisplayListTraversalMode == DL_EXECUTE)
        {
          CpSetTimeFormat(executeTimeCartesianPlotWidget,
                          cpTimeFormatString[(int)globalResourceBundle.axis[0].timeFormat -
                                             FIRST_CP_TIME_FORMAT]);
        }
      break;
    default:
      medmPrintf(1, "\ncpAxisOptionMenuSimpleCallback: "
                 "Unknown rcType = %d\n",
                 rcType / 4);
      break;
    }

  /* Update from globalResourceBundle for EDIT mode
   *   EXECUTE mode already done */
  if (globalDisplayListTraversalMode == DL_EDIT)
    {
      if (cdi)
        {
          DlElement *dlElement = FirstDlElement(
                                                cdi->selectedDlElementList);
          unhighlightSelectedElements();
          while (dlElement)
            {
              updateElementFromGlobalResourceBundle(
                                                    dlElement->structure.element);
              dlElement = dlElement->next;
            }
          dmTraverseNonWidgetsInDisplayList(cdi);
          highlightSelectedElements();
        }
    }
}

void cpAxisTextFieldActivateCallback(Widget w, XtPointer cd, XtPointer cbs)
{
  DisplayInfo *cdi = currentDisplayInfo;
  int rcType = (intptr_t)cd;
  char *stringValue, string[24];
  int k, iPrec = -1;
  XcVType minF, maxF, tickF;
  int axis = -1, isMax = -1;
#if DEBUG_XRT
  XcVType valF;
#endif
  UNREFERENCED(cbs);

#if DEBUG_XRT
  print("\ncpAxisTextFieldActivateCallback: Entered\n");
#endif
  stringValue = XmTextFieldGetString(w);

  /* Determine axis max or min
   *   Note: For the strcpy() calls, note that the textField has a maxLength
   resource set such that the strcpy always succeeds) */
  switch (rcType)
    {
    case CP_X_RANGE_MIN:
    case CP_Y_RANGE_MIN:
    case CP_Y2_RANGE_MIN:
    case CP_Y3_RANGE_MIN:
    case CP_Y4_RANGE_MIN:
      globalResourceBundle.axis[rcType % 5].minRange = (float)atof(stringValue);
      if (globalDisplayListTraversalMode == DL_EXECUTE)
        {
#if DEBUG_XRT
          valF.fval = globalResourceBundle.axis[rcType % 5].minRange;
#endif
          switch (rcType % 5)
            {
            case X_AXIS_ELEMENT:
              axis = CP_X;
              isMax = 0;
              break;
            case Y1_AXIS_ELEMENT:
              axis = CP_Y;
              isMax = 0;
              break;
            case Y2_AXIS_ELEMENT:
              axis = CP_Y2;
              isMax = 0;
              break;
            case Y3_AXIS_ELEMENT:
              axis = CP_Y3;
              isMax = 0;
              break;
            case Y4_AXIS_ELEMENT:
              axis = CP_Y4;
              isMax = 0;
              break;
            default:
              medmPrintf(1, "\ncpAxisTextFieldActivateCallback (MIN): "
                         "Unknown rcType%%5 = %d\n",
                         rcType % 5);
              return;
            }
#if DEBUG_XRT
          print("\ncpAxisTextFieldActivateCallback [MIN]: "
                "valF.fval =%g valF.lval=%ld Converted: %d\n",
                valF.fval, valF.lval, XrtFloatToArgVal(valF.fval));
#endif
        }
      break;
    case CP_X_RANGE_MAX:
    case CP_Y_RANGE_MAX:
    case CP_Y2_RANGE_MAX:
    case CP_Y3_RANGE_MAX:
    case CP_Y4_RANGE_MAX:
      globalResourceBundle.axis[rcType % 5].maxRange = (float)atof(stringValue);
      if (globalDisplayListTraversalMode == DL_EXECUTE)
        {
#if DEBUG_XRT
          valF.fval = globalResourceBundle.axis[rcType % 5].maxRange;
#endif
          switch (rcType % 5)
            {
            case X_AXIS_ELEMENT:
              axis = CP_X;
              isMax = 1;
              break;
            case Y1_AXIS_ELEMENT:
              axis = CP_Y;
              isMax = 1;
              break;
            case Y2_AXIS_ELEMENT:
              axis = CP_Y2;
              isMax = 1;
              break;
            case Y3_AXIS_ELEMENT:
              axis = CP_Y3;
              isMax = 1;
              break;
            case Y4_AXIS_ELEMENT:
              axis = CP_Y4;
              isMax = 1;
              break;
            default:
              medmPrintf(1, "\ncpAxisTextFieldActivateCallback (MAX): "
                         "Unknown rcType%%5 = %d\n",
                         rcType % 5);
              return;
            }
#if DEBUG_XRT
          print("\ncpAxisTextFieldActivateCallback [MAX]: "
                "valF.fval =%g valF.lval=%ld Converted: %d\n",
                valF.fval, valF.lval, XrtFloatToArgVal(valF.fval));
#endif
        }
      break;
    default:
      medmPrintf(1, "\ncpAxisTextFieldActivateCallback: "
                 "Unknown rcType = %d\n",
                 rcType);
      return;
    }
  XtFree(stringValue);

  /* Recalculate ticks */
  if (globalDisplayListTraversalMode == DL_EXECUTE)
    {
      minF.fval = globalResourceBundle.axis[rcType % 5].minRange;
      maxF.fval = globalResourceBundle.axis[rcType % 5].maxRange;
      tickF.fval = (float)((maxF.fval - minF.fval) / 4.0);
#if DEBUG_XRT
      print("cpAxisTextFieldActivateCallback: "
            "minF.fval =%g minF.lval=%ld Converted: %d\n",
            minF.fval, minF.lval, XrtFloatToArgVal(minF.fval));
      print("cpAxisTextFieldActivateCallback: "
            "maxF.fval =%g maxF.lval=%ld Converted: %d\n",
            maxF.fval, maxF.lval, XrtFloatToArgVal(maxF.fval));
      print("cpAxisTextFieldActivateCallback: "
            "tickF.fval =%g tickF.lval=%ld Converted: %d\n",
            tickF.fval, tickF.lval, XrtFloatToArgVal(tickF.fval));
#endif
      sprintf(string, "%f", tickF.fval);
      k = strlen(string) - 1;
      while (string[k] == '0')
        k--; /* strip off trailing zeroes */
      iPrec = k;
      while (string[k] != '.' && k >= 0)
        k--;
      iPrec = iPrec - k;
    }

  /* Update for EDIT or EXECUTE mode  */
  switch (globalDisplayListTraversalMode)
    {
    case DL_EDIT:
      /*
       * update elements (this is overkill, but okay for now)
       *	-- not as efficient as it should be (don't update EVERYTHING if only
       *	   one item changed!)
       */
      if (cdi != NULL)
        {
          DlElement *dlElement = FirstDlElement(
                                                cdi->selectedDlElementList);
          unhighlightSelectedElements();
          while (dlElement)
            {
              updateElementFromGlobalResourceBundle(
                                                    dlElement->structure.element);
              dlElement = dlElement->next;
            }
          dmTraverseNonWidgetsInDisplayList(cdi);
          highlightSelectedElements();
        }
      break;

    case DL_EXECUTE:
      if (executeTimeCartesianPlotWidget != NULL)
        {
          if (isMax)
            {
              CpSetAxisMax(executeTimeCartesianPlotWidget, axis,
                           maxF, tickF, tickF, iPrec);
            }
          else
            {
              CpSetAxisMin(executeTimeCartesianPlotWidget, axis,
                           minF, tickF, tickF, iPrec);
            }
        }
      break;
    }
}

void cpAxisTextFieldLosingFocusCallback(Widget w, XtPointer cd, XtPointer cbs)
/* Note: Losing focus happens when cursor leaves cartesianPlotAxisS,
   too */
{
  int rcType = (intptr_t)cd;
  char string[MAX_TOKEN_LENGTH], *currentString;
  int tail;
  XcVType minF[4], maxF[4];

  UNREFERENCED(cbs);

#if DEBUG_LOSING_FOCUS
  print("\ncpAxisTextFieldLosingFocusCallback: Entered\n");
#endif
#if DEBUG_XRT
  print("\ncpAxisTextFieldLosingFocusCallback: Entered\n");
  print("  executeTimeCartesianPlotWidget: %d\n",
        executeTimeCartesianPlotWidget);
  print("  rcType: %d  rcType%%5: %d\n", rcType, rcType % 5);
  print("  axisRangeMin[rcType%%5]: %d  axisRangeMax[rcType%%5]: %d  "
        "w: %d\n",
        axisRangeMin[rcType % 5], axisRangeMax[rcType % 5], w);
#endif
  /* Losing focus - make sure that the text field remains accurate wrt
   *   values stored in widget (not necessarily what is in the
   *   globalResourceBundle) */
  if (executeTimeCartesianPlotWidget != NULL)
    {
      CpGetAxisMaxMin(executeTimeCartesianPlotWidget, CP_X,
                      &maxF[X_AXIS_ELEMENT], &minF[X_AXIS_ELEMENT]);
      CpGetAxisMaxMin(executeTimeCartesianPlotWidget, CP_Y,
                      &maxF[Y1_AXIS_ELEMENT], &minF[Y1_AXIS_ELEMENT]);
      CpGetAxisMaxMin(executeTimeCartesianPlotWidget, CP_Y2,
                      &maxF[Y2_AXIS_ELEMENT], &minF[Y2_AXIS_ELEMENT]);
      CpGetAxisMaxMin(executeTimeCartesianPlotWidget, CP_Y3,
                      &maxF[Y3_AXIS_ELEMENT], &minF[Y3_AXIS_ELEMENT]);
      CpGetAxisMaxMin(executeTimeCartesianPlotWidget, CP_Y4,
                      &maxF[Y4_AXIS_ELEMENT], &minF[Y4_AXIS_ELEMENT]);
    }
  else
    {
      return;
    }

#if DEBUG_LOSING_FOCUS
  print("  switch(rcType)\n");
#endif
  switch (rcType)
    {
    case CP_X_RANGE_MIN:
    case CP_Y_RANGE_MIN:
    case CP_Y2_RANGE_MIN:
    case CP_Y3_RANGE_MIN:
    case CP_Y4_RANGE_MIN:
      sprintf(string, "%f", minF[rcType % 5].fval);
      break;
    case CP_X_RANGE_MAX:
    case CP_Y_RANGE_MAX:
    case CP_Y2_RANGE_MAX:
    case CP_Y3_RANGE_MAX:
    case CP_Y4_RANGE_MAX:
      sprintf(string, "%f", maxF[rcType % 5].fval);
      break;
    default:
      medmPostMsg(1, "cpAxisTextFieldLosingFocusCallback: Unknown rcType = %d",
                  rcType / 4);
      return;
    }
#if DEBUG_LOSING_FOCUS
  print("  Strip trailing zeros\n");
#endif
  /* Strip trailing zeros */
  tail = strlen(string);
  while (string[--tail] == '0')
    string[tail] = '\0';
  /* Set the new value if it is different */
  currentString = XmTextFieldGetString(w);
  if (strcmp(string, currentString))
    {
      XmTextFieldSetString(w, string);
#if DEBUG_LOSING_FOCUS
      print("  XmTextFieldSetString\n");
#endif
    }
  XtFree(currentString);
}

/*
 * Menu entry support routine for the Cartesian Plot Axis Dialog...
 */
void createCartesianPlotAxisDialogMenuEntry(
                                            Widget parentRC,
                                            XmString axisLabelXmString,
                                            Widget *label,
                                            Widget *menu,
                                            XmString *menuLabelXmStrings,
                                            XmButtonType *buttonType,
                                            int numberOfLabels,
                                            XtPointer clientData)
{
  Arg args[10];
  int n = 0;
  Widget rowColumn;

  /* create rowColumn widget to hold the label and menu widgets */
  XtSetArg(args[n], XmNorientation, XmVERTICAL);
  n++;
  XtSetArg(args[n], XmNpacking, XmPACK_NONE);
  n++;
  rowColumn = XmCreateRowColumn(parentRC, "entryRC", args, n);

  /* create the label widget */
  n = 0;
  XtSetArg(args[n], XmNalignment, XmALIGNMENT_END);
  n++;
  XtSetArg(args[n], XmNlabelString, axisLabelXmString);
  n++;
  XtSetArg(args[n], XmNrecomputeSize, False);
  n++;
  *label = XmCreateLabel(rowColumn, "localLabel", args, n);

  /* create the text widget */
  n = 0;
  XtSetArg(args[n], XmNbuttonType, buttonType);
  n++;
  XtSetArg(args[n], XmNbuttons, menuLabelXmStrings);
  n++;
  XtSetArg(args[n], XmNbuttonCount, numberOfLabels);
  n++;
  XtSetArg(args[n], XmNsimpleCallback, cpAxisOptionMenuSimpleCallback);
  n++;
  XtSetArg(args[n], XmNuserData, clientData);
  n++;
  *menu = XmCreateSimpleOptionMenu(rowColumn, "localElement", args, n);
  XtUnmanageChild(XmOptionLabelGadget(*menu));
  XtManageChild(rowColumn);
}

/* Text entry support routine for the Cartesian Plot Axis Dialog...  */
void createCartesianPlotAxisDialogTextEntry(
                                            Widget parentRC,
                                            XmString axisLabelXmString,
                                            Widget *rowColumn,
                                            Widget *label,
                                            Widget *text,
                                            XtPointer clientData)
{
  Arg args[10];
  int n = 0;
  /* Create a row column widget to hold the label and text field widget */
  XtSetArg(args[n], XmNorientation, XmVERTICAL);
  n++;
  XtSetArg(args[n], XmNpacking, XmPACK_NONE);
  n++;
  *rowColumn = XmCreateRowColumn(parentRC, "entryRC", args, n);

  /* Create the label */
  n = 0;
  XtSetArg(args[n], XmNalignment, XmALIGNMENT_END);
  n++;
  XtSetArg(args[n], XmNlabelString, axisLabelXmString);
  n++;
  XtSetArg(args[n], XmNrecomputeSize, False);
  n++;
  *label = XmCreateLabel(*rowColumn, "localLabel", args, n);

  /* Create the text field */
  n = 0;
  XtSetArg(args[n], XmNmaxLength, MAX_TOKEN_LENGTH - 1);
  n++;
  *text = XmCreateTextField(*rowColumn, "localElement", args, n);
  XtAddCallback(*text, XmNactivateCallback,
                cpAxisTextFieldActivateCallback, clientData);
  XtAddCallback(*text, XmNlosingFocusCallback,
                cpAxisTextFieldLosingFocusCallback, clientData);
  XtAddCallback(*text, XmNmodifyVerifyCallback,
                textFieldFloatVerifyCallback, NULL);
  XtManageChild(*rowColumn);
}

/*
 * Create Cartesian Plot axis dialog box
 */
Widget createCartesianPlotAxisDialog(Widget parent)
{
  Widget shell, closeButton;
  Arg args[12];
  int counter;
  XmString xmString, axisStyleXmString, axisRangeXmString, axisMinXmString,
    axisMaxXmString, axisTimeFmtXmString, frameLabelXmString;
  int i, n;
  XmButtonType buttonType[MAX_CP_AXIS_BUTTONS];
  Widget entriesRC, frame, localLabel, parentRC;
  /* For keeping list of widgets around */
  Widget entryLabel[MAX_CP_AXIS_ELEMENTS], entryElement[MAX_CP_AXIS_ELEMENTS];
  Dimension width, height;
  /* Indexed like dlCartesianPlot->axis[]: X_ELEMENT_AXIS, Y1_ELEMENT_AXIS... */
  static char *frameLabelString[5] = {
    "X Axis",
    "Y1 Axis",
    "Y2 Axis",
    "Y3 Axis",
    "Y4 Axis",
  };

  UNREFERENCED(parent);

  /* Initialize XmString value tables (since this can be edit or execute time) */
  initializeXmStringValueTables();

  /* Set buttons to be push button */
  for (i = 0; i < MAX_CP_AXIS_BUTTONS; i++)
    buttonType[i] = XmPUSHBUTTON;

  /*
   * Create the interface
   *		     ...
   *		 OK     CANCEL
   */

  /* Shell */
  n = 0;
  XtSetArg(args[n], XmNdeleteResponse, XmDO_NOTHING);
  n++;
#if OMIT_RESIZE_HANDLES
  XtSetArg(args[n], XmNmwmDecorations, MWM_DECOR_ALL | MWM_DECOR_RESIZEH);
  n++;
  /* KE: The following is necessary for Exceed, which turns off the
     resize function with the handles.  It should not be necessary */
  XtSetArg(args[n], XmNmwmFunctions, MWM_FUNC_ALL);
  n++;
#endif
  XtSetArg(args[n], XmNautoUnmanage, False);
  n++;
  XtSetArg(args[n], XmNtitle, "Cartesian Plot Axis Data");
  n++;
  shell = XtCreatePopupShell("cartesianPlotAxisS",
                             topLevelShellWidgetClass, mainShell, args, n);
  XmAddWMProtocolCallback(shell, WM_DELETE_WINDOW,
                          cartesianPlotAxisActivate,
                          (XtPointer)CP_CLOSE_BTN);

  /* Form */
  n = 0;
  XtSetArg(args[n], XmNautoUnmanage, False);
  n++;
  XtSetArg(args[n], XmNmarginHeight, 8);
  n++;
  XtSetArg(args[n], XmNmarginWidth, 8);
  n++;
  XtSetArg(args[n], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL);
  n++;
  cpAxisForm = XmCreateForm(shell, "cartesianPlotAxisF", args, n);

  /* RowColumn */
  n = 0;
  XtSetArg(args[n], XmNnumColumns, 1);
  n++;
  XtSetArg(args[n], XmNorientation, XmVERTICAL);
  n++;
  XtSetArg(args[n], XmNpacking, XmPACK_COLUMN);
  n++;
  entriesRC = XmCreateRowColumn(cpAxisForm, "entriesRC", args, n);

  axisStyleXmString = XmStringCreateLocalized("Axis Style");
  axisRangeXmString = XmStringCreateLocalized("Axis Range");
  axisMinXmString = XmStringCreateLocalized("Minimum Value");
  axisMaxXmString = XmStringCreateLocalized("Maximum Value");
  axisTimeFmtXmString = XmStringCreateLocalized("Time format");

  /* Loop over major elements */
  counter = 0;
  for (i = X_AXIS_ELEMENT /* 0 */; i <= Y4_AXIS_ELEMENT /* 4 */; i++)
    {
      /* Frame */
      n = 0;
      XtSetArg(args[n], XmNshadowType, XmSHADOW_ETCHED_IN);
      n++;
      frame = XmCreateFrame(entriesRC, "frame", args, n);
      XtManageChild(frame);

      /* Label */
      n = 0;
      frameLabelXmString = XmStringCreateLocalized(frameLabelString[i]);
      XtSetArg(args[n], XmNlabelString, frameLabelXmString);
      n++;
      XtSetArg(args[n], XmNmarginWidth, 0);
      n++;
      XtSetArg(args[n], XmNmarginHeight, 0);
      n++;
      XtSetArg(args[n], XmNchildType, XmFRAME_TITLE_CHILD);
      n++;
      /* (Use font calculation for textField (which uses ~90% of height)) */
      XtSetArg(args[n], XmNfontList, fontListTable[textFieldFontListIndex(24)]);
      n++;
      localLabel = XmCreateLabel(frame, "label", args, n);
      XtManageChild(localLabel);
      XmStringFree(frameLabelXmString);

      /* RC within frame */
      n = 0;
      XtSetArg(args[n], XmNnumColumns, 1);
      n++;
      XtSetArg(args[n], XmNorientation, XmVERTICAL);
      n++;
      XtSetArg(args[n], XmNpacking, XmPACK_COLUMN);
      n++;
      parentRC = XmCreateRowColumn(frame, "parentRC", args, n);
      XtManageChild(parentRC);

      /* Create Axis Style Entry */
      createCartesianPlotAxisDialogMenuEntry(
                                             parentRC,
                                             axisStyleXmString,
                                             &(entryLabel[counter]),
                                             &(entryElement[counter]),
                                             &(xmStringValueTable[FIRST_CARTESIAN_PLOT_AXIS_STYLE]),
                                             buttonType,
                                             (!i) ? NUM_CARTESIAN_PLOT_AXIS_STYLES : NUM_CARTESIAN_PLOT_AXIS_STYLES - 1,
                                             (XtPointer)(uintptr_t)(CP_X_AXIS_STYLE + i));
      axisStyleMenu[i] = entryElement[counter];
      counter++;

      /* Create Range Style Entry */
      createCartesianPlotAxisDialogMenuEntry(parentRC,
                                             axisRangeXmString,
                                             &(entryLabel[counter]),
                                             &(entryElement[counter]),
                                             &(xmStringValueTable[FIRST_CARTESIAN_PLOT_RANGE_STYLE]),
                                             buttonType,
                                             NUM_CARTESIAN_PLOT_RANGE_STYLES,
                                             (XtPointer)(uintptr_t)(CP_X_RANGE_STYLE + i));
      axisRangeMenu[i] = entryElement[counter];
      counter++;

      /* Create Min text field entry */
      createCartesianPlotAxisDialogTextEntry(
                                             parentRC, axisMinXmString,
                                             &(axisRangeMinRC[i]), &(entryLabel[counter]),
                                             &(entryElement[counter]), (XtPointer)(uintptr_t)(CP_X_RANGE_MIN + i));
      axisRangeMin[i] = entryElement[counter];
      counter++;

      /* Create Max text field entry */
      createCartesianPlotAxisDialogTextEntry(
                                             parentRC, axisMaxXmString,
                                             &(axisRangeMaxRC[i]), &(entryLabel[counter]),
                                             &(entryElement[counter]), (XtPointer)(uintptr_t)(CP_X_RANGE_MAX + i));
      axisRangeMax[i] = entryElement[counter];
      counter++;

      /* Create time format menu entry for X axis only */
      if (i == X_AXIS_ELEMENT)
        {
          createCartesianPlotAxisDialogMenuEntry(
                                                 parentRC,
                                                 axisTimeFmtXmString,
                                                 &(entryLabel[counter]),
                                                 &(entryElement[counter]),
                                                 &(xmStringValueTable[FIRST_CP_TIME_FORMAT]),
                                                 buttonType,
                                                 NUM_CP_TIME_FORMAT,
                                                 (XtPointer)(CP_X_TIME_FORMAT));
          axisTimeFormat = entryElement[counter];
          counter++;
        }
    }

  /* Get max sizes and manage */
  for (i = 0; i < counter; i++)
    {
      XtVaGetValues(entryLabel[i], XmNwidth, &width, XmNheight, &height, NULL);
      maxLabelWidth = MAX(maxLabelWidth, width);
      maxLabelHeight = MAX(maxLabelHeight, height);
      XtVaGetValues(entryElement[i], XmNwidth, &width, XmNheight, &height, NULL);
      maxLabelWidth = MAX(maxLabelWidth, width);
      maxLabelHeight = MAX(maxLabelHeight, height);
#if 0
      /* Is done below */
      XtManageChild(entryLabel[i]);
      XtManageChild(entryElement[i]);
#endif
    }

  /* Resize the labels and elements (to max width) for uniform appearance */
  for (i = 0; i < counter; i++)
    {
      /* Set label */
      XtVaSetValues(entryLabel[i], XmNwidth, maxLabelWidth,
                    XmNheight, maxLabelHeight, XmNrecomputeSize, False,
                    XmNalignment, XmALIGNMENT_END, NULL);

      /* Set element */
      if (XtClass(entryElement[i]) == xmRowColumnWidgetClass)
        {
          /* must be option menu - unmanage label widget */
          XtVaSetValues(XmOptionButtonGadget(entryElement[i]),
                        XmNx, (Position)maxLabelWidth, XmNwidth, maxLabelWidth,
                        XmNheight, maxLabelHeight,
                        XmNrecomputeSize, False, XmNresizeWidth, True,
                        XmNmarginWidth, 0,
                        NULL);
        }
      XtVaSetValues(entryElement[i],
                    XmNx, (Position)maxLabelWidth, XmNwidth, maxLabelWidth,
                    XmNheight, maxLabelHeight,
                    XmNrecomputeSize, False, XmNresizeWidth, True,
                    XmNmarginWidth, 0,
                    NULL);
      XtManageChild(entryLabel[i]);
      XtManageChild(entryElement[i]);
    }

  /* Free strings */
  XmStringFree(axisStyleXmString);
  XmStringFree(axisRangeXmString);
  XmStringFree(axisMinXmString);
  XmStringFree(axisMaxXmString);
  XmStringFree(axisTimeFmtXmString);

  /* Set values for entriesRC (After resizing) */
  n = 0;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM);
  n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM);
  n++;
  XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM);
  n++;
  XtSetValues(entriesRC, args, n);

  /* Manage the RC */
  XtManageChild(entriesRC);

  /* Close button */
  n = 0;
  xmString = XmStringCreateLocalized("Close");
  XtSetArg(args[n], XmNlabelString, xmString);
  n++;
  XtSetArg(args[n], XmNtopAttachment, XmATTACH_WIDGET);
  n++;
  XtSetArg(args[n], XmNtopWidget, entriesRC);
  n++;
  XtSetArg(args[n], XmNtopOffset, 12);
  n++;
  XtSetArg(args[n], XmNleftAttachment, XmATTACH_POSITION);
  n++;
  XtSetArg(args[n], XmNbottomAttachment, XmATTACH_FORM);
  n++;
  XtSetArg(args[n], XmNbottomOffset, 12);
  n++;
  /* HACK - approximate centering button by putting at 43% of form width */
  XtSetArg(args[n], XmNleftPosition, (Position)43);
  n++;
  closeButton = XmCreatePushButton(cpAxisForm, "closeButton", args, n);
  XtAddCallback(closeButton, XmNactivateCallback,
                cartesianPlotAxisActivate, (XtPointer)CP_CLOSE_BTN);
  XtManageChild(closeButton);
  XmStringFree(xmString);

  /* Return the shell */
  return (shell);
}

/*
 * access function (for file-scoped globals, etc) to udpate the
 *	cartesian plot axis dialog with the values currently in
 *	globalResourceBundle
 */
void updateCartesianPlotAxisDialog()
{
  int i, tail;
  char string[MAX_TOKEN_LENGTH];

  for (i = X_AXIS_ELEMENT; i <= Y4_AXIS_ELEMENT; i++)
    {
      optionMenuSet(axisStyleMenu[i], globalResourceBundle.axis[i].axisStyle - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
      optionMenuSet(axisRangeMenu[i], globalResourceBundle.axis[i].rangeStyle - FIRST_CARTESIAN_PLOT_RANGE_STYLE);
      if (globalResourceBundle.axis[i].rangeStyle == USER_SPECIFIED_RANGE)
        {
          sprintf(string, "%f", globalResourceBundle.axis[i].minRange);
          /* strip trailing zeroes */
          tail = strlen(string);
          while (string[--tail] == '0')
            string[tail] = '\0';
          XmTextFieldSetString(axisRangeMin[i], string);
          sprintf(string, "%f", globalResourceBundle.axis[i].maxRange);
          /* strip trailing zeroes */
          tail = strlen(string);
          while (string[--tail] == '0')
            string[tail] = '\0';
          XmTextFieldSetString(axisRangeMax[i], string);
          XtSetSensitive(axisRangeMinRC[i], True);
          XtSetSensitive(axisRangeMaxRC[i], True);
        }
      else
        {
          XtSetSensitive(axisRangeMinRC[i], False);
          XtSetSensitive(axisRangeMaxRC[i], False);
        }
    }
  if (globalResourceBundle.axis[0].axisStyle == TIME_AXIS)
    {
      XtSetSensitive(axisTimeFormat, True);
      optionMenuSet(axisTimeFormat, globalResourceBundle.axis[0].timeFormat - FIRST_CP_TIME_FORMAT);
    }
  else
    {
      XtSetSensitive(axisTimeFormat, False);
    }
}

/*
 * access function (for file-scoped globals, etc) to udpate the
 *      cartesian plot axis dialog with the values currently in
 *      the subject cartesian plot
 */
void updateCartesianPlotAxisDialogFromWidget(Widget cp)
{
  int i, tail, buttonId;
  char string[MAX_TOKEN_LENGTH];
  MedmCartesianPlot *pcp;
  XtPointer userData;
  Boolean xAxisIsLog, y1AxisIsLog, y2AxisIsLog, y3AxisIsLog, y4AxisIsLog, xAxisIsTime,
    xAxisIsAuto, y1AxisIsAuto, y2AxisIsAuto, y3AxisIsAuto, y4AxisIsAuto,
    xIsCurrentlyFromChannel = False, y1IsCurrentlyFromChannel = False,
    y2IsCurrentlyFromChannel = False, y3IsCurrentlyFromChannel = False, y4IsCurrentlyFromChannel = False;
  XcVType xMinF, xMaxF, y1MinF, y1MaxF, y2MinF, y2MaxF, y3MinF, y3MaxF, y4MinF, y4MaxF;
  char *timeFormat = NULL;
  if (globalDisplayListTraversalMode != DL_EXECUTE)
    return;

  CpGetAxisInfo(cp, &userData, &xAxisIsTime, &timeFormat,
                &xAxisIsLog, &y1AxisIsLog, &y2AxisIsLog, &y3AxisIsLog, &y4AxisIsLog,
                &xAxisIsAuto, &y1AxisIsAuto, &y2AxisIsAuto, &y3AxisIsAuto, &y4AxisIsAuto,
                &xMaxF, &y1MaxF, &y2MaxF, &y3MaxF, &y4MaxF,
                &xMinF, &y1MinF, &y2MinF, &y3MinF, &y4MinF);

  /* Determine range by first checking if from channel using the
   *   MedmCartesianPlot.  If not from channel, determine whether
   *   auto or user from AxisIsAuto. */
  pcp = (MedmCartesianPlot *)userData;
  if (pcp)
    {
      xIsCurrentlyFromChannel =
        pcp->axisRange[X_AXIS_ELEMENT].isCurrentlyFromChannel;
      y1IsCurrentlyFromChannel =
        pcp->axisRange[Y1_AXIS_ELEMENT].isCurrentlyFromChannel;
      y2IsCurrentlyFromChannel =
        pcp->axisRange[Y2_AXIS_ELEMENT].isCurrentlyFromChannel;
      y3IsCurrentlyFromChannel =
        pcp->axisRange[Y3_AXIS_ELEMENT].isCurrentlyFromChannel;
      y4IsCurrentlyFromChannel =
        pcp->axisRange[Y4_AXIS_ELEMENT].isCurrentlyFromChannel;
    }

  /* X Axis Style */
  if (xAxisIsTime)
    {
      /* Style */
      optionMenuSet(axisStyleMenu[X_AXIS_ELEMENT],
                    TIME_AXIS - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
      /* Time format */
      buttonId = 0; /* Use for  default */
      for (i = 0; i < NUM_CP_TIME_FORMAT; i++)
        {
          if (timeFormat && !strcmp(cpTimeFormatString[i], timeFormat))
            {
              buttonId = i;
              break;
            }
        }
      optionMenuSet(axisTimeFormat, buttonId);
      XtSetSensitive(axisTimeFormat, True);
    }
  else
    {
      /* Style */
      optionMenuSet(axisStyleMenu[X_AXIS_ELEMENT],
                    (xAxisIsLog ? LOG10_AXIS : LINEAR_AXIS) - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
      XtSetSensitive(axisTimeFormat, False);
    }
  /* X Axis Range */
  buttonId = (xIsCurrentlyFromChannel ? CHANNEL_RANGE : (xAxisIsAuto ? AUTO_SCALE_RANGE : USER_SPECIFIED_RANGE)) - FIRST_CARTESIAN_PLOT_RANGE_STYLE;
  optionMenuSet(axisRangeMenu[X_AXIS_ELEMENT], buttonId);
  if (buttonId == USER_SPECIFIED_RANGE - FIRST_CARTESIAN_PLOT_RANGE_STYLE)
    {
      sprintf(string, "%f", xMinF.fval);
      /* strip trailing zeroes */
      tail = strlen(string);
      while (string[--tail] == '0')
        string[tail] = '\0';
      XmTextFieldSetString(axisRangeMin[X_AXIS_ELEMENT], string);
      sprintf(string, "%f", xMaxF.fval);
      /* strip trailing zeroes */
      tail = strlen(string);
      while (string[--tail] == '0')
        string[tail] = '\0';
      XmTextFieldSetString(axisRangeMax[X_AXIS_ELEMENT], string);
    }
  if (!xAxisIsAuto && !xIsCurrentlyFromChannel)
    {
      XtSetSensitive(axisRangeMinRC[X_AXIS_ELEMENT], True);
      XtSetSensitive(axisRangeMaxRC[X_AXIS_ELEMENT], True);
    }
  else
    {
      XtSetSensitive(axisRangeMinRC[X_AXIS_ELEMENT], False);
      XtSetSensitive(axisRangeMaxRC[X_AXIS_ELEMENT], False);
    }

  /* Y1 Axis both */
  optionMenuSet(axisStyleMenu[Y1_AXIS_ELEMENT],
                (y1AxisIsLog ? LOG10_AXIS : LINEAR_AXIS) - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
  buttonId = (y1IsCurrentlyFromChannel ? CHANNEL_RANGE : (y1AxisIsAuto ? AUTO_SCALE_RANGE : USER_SPECIFIED_RANGE)) - FIRST_CARTESIAN_PLOT_RANGE_STYLE;
  optionMenuSet(axisRangeMenu[Y1_AXIS_ELEMENT], buttonId);
  if (buttonId == USER_SPECIFIED_RANGE - FIRST_CARTESIAN_PLOT_RANGE_STYLE)
    {
      sprintf(string, "%f", y1MinF.fval);
      /* strip trailing zeroes */
      tail = strlen(string);
      while (string[--tail] == '0')
        string[tail] = '\0';
      XmTextFieldSetString(axisRangeMin[Y1_AXIS_ELEMENT], string);
      sprintf(string, "%f", y1MaxF.fval);
      /* strip trailing zeroes */
      tail = strlen(string);
      while (string[--tail] == '0')
        string[tail] = '\0';
      XmTextFieldSetString(axisRangeMax[Y1_AXIS_ELEMENT], string);
    }
  if (!y1AxisIsAuto && !y1IsCurrentlyFromChannel)
    {
      XtSetSensitive(axisRangeMinRC[Y1_AXIS_ELEMENT], True);
      XtSetSensitive(axisRangeMaxRC[Y1_AXIS_ELEMENT], True);
    }
  else
    {
      XtSetSensitive(axisRangeMinRC[Y1_AXIS_ELEMENT], False);
      XtSetSensitive(axisRangeMaxRC[Y1_AXIS_ELEMENT], False);
    }

  /* Y2 Axis both */
  optionMenuSet(axisStyleMenu[Y2_AXIS_ELEMENT],
                (y2AxisIsLog ? LOG10_AXIS : LINEAR_AXIS) - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
  buttonId = (y2IsCurrentlyFromChannel ? CHANNEL_RANGE : (y2AxisIsAuto ? AUTO_SCALE_RANGE : USER_SPECIFIED_RANGE)) - FIRST_CARTESIAN_PLOT_RANGE_STYLE;
  optionMenuSet(axisRangeMenu[Y2_AXIS_ELEMENT], buttonId);
  if (buttonId == USER_SPECIFIED_RANGE - FIRST_CARTESIAN_PLOT_RANGE_STYLE)
    {
      sprintf(string, "%f", y2MinF.fval);
      /* strip trailing zeroes */
      tail = strlen(string);
      while (string[--tail] == '0')
        string[tail] = '\0';
      XmTextFieldSetString(axisRangeMin[Y2_AXIS_ELEMENT], string);
      sprintf(string, "%f", y2MaxF.fval);
      /* strip trailing zeroes */
      tail = strlen(string);
      while (string[--tail] == '0')
        string[tail] = '\0';
      XmTextFieldSetString(axisRangeMax[Y2_AXIS_ELEMENT], string);
    }
  if (!y2AxisIsAuto && !y2IsCurrentlyFromChannel)
    {
      XtSetSensitive(axisRangeMinRC[Y2_AXIS_ELEMENT], True);
      XtSetSensitive(axisRangeMaxRC[Y2_AXIS_ELEMENT], True);
    }
  else
    {
      XtSetSensitive(axisRangeMinRC[Y2_AXIS_ELEMENT], False);
      XtSetSensitive(axisRangeMaxRC[Y2_AXIS_ELEMENT], False);
    }

  /* Y3 Axis both */
  optionMenuSet(axisStyleMenu[Y3_AXIS_ELEMENT],
                (y3AxisIsLog ? LOG10_AXIS : LINEAR_AXIS) - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
  buttonId = (y3IsCurrentlyFromChannel ? CHANNEL_RANGE : (y3AxisIsAuto ? AUTO_SCALE_RANGE : USER_SPECIFIED_RANGE)) - FIRST_CARTESIAN_PLOT_RANGE_STYLE;
  optionMenuSet(axisRangeMenu[Y3_AXIS_ELEMENT], buttonId);
  if (buttonId == USER_SPECIFIED_RANGE - FIRST_CARTESIAN_PLOT_RANGE_STYLE)
    {
      sprintf(string, "%f", y3MinF.fval);
      /* strip trailing zeroes */
      tail = strlen(string);
      while (string[--tail] == '0')
        string[tail] = '\0';
      XmTextFieldSetString(axisRangeMin[Y3_AXIS_ELEMENT], string);
      sprintf(string, "%f", y3MaxF.fval);
      /* strip trailing zeroes */
      tail = strlen(string);
      while (string[--tail] == '0')
        string[tail] = '\0';
      XmTextFieldSetString(axisRangeMax[Y3_AXIS_ELEMENT], string);
    }
  if (!y3AxisIsAuto && !y3IsCurrentlyFromChannel)
    {
      XtSetSensitive(axisRangeMinRC[Y3_AXIS_ELEMENT], True);
      XtSetSensitive(axisRangeMaxRC[Y3_AXIS_ELEMENT], True);
    }
  else
    {
      XtSetSensitive(axisRangeMinRC[Y3_AXIS_ELEMENT], False);
      XtSetSensitive(axisRangeMaxRC[Y3_AXIS_ELEMENT], False);
    }

  /* Y4 Axis both */
  optionMenuSet(axisStyleMenu[Y4_AXIS_ELEMENT],
                (y4AxisIsLog ? LOG10_AXIS : LINEAR_AXIS) - FIRST_CARTESIAN_PLOT_AXIS_STYLE);
  buttonId = (y4IsCurrentlyFromChannel ? CHANNEL_RANGE : (y4AxisIsAuto ? AUTO_SCALE_RANGE : USER_SPECIFIED_RANGE)) - FIRST_CARTESIAN_PLOT_RANGE_STYLE;
  optionMenuSet(axisRangeMenu[Y4_AXIS_ELEMENT], buttonId);
  if (buttonId == USER_SPECIFIED_RANGE - FIRST_CARTESIAN_PLOT_RANGE_STYLE)
    {
      sprintf(string, "%f", y4MinF.fval);
      /* strip trailing zeroes */
      tail = strlen(string);
      while (string[--tail] == '0')
        string[tail] = '\0';
      XmTextFieldSetString(axisRangeMin[Y4_AXIS_ELEMENT], string);
      sprintf(string, "%f", y4MaxF.fval);
      /* strip trailing zeroes */
      tail = strlen(string);
      while (string[--tail] == '0')
        string[tail] = '\0';
      XmTextFieldSetString(axisRangeMax[Y4_AXIS_ELEMENT], string);
    }
  if (!y4AxisIsAuto && !y4IsCurrentlyFromChannel)
    {
      XtSetSensitive(axisRangeMinRC[Y4_AXIS_ELEMENT], True);
      XtSetSensitive(axisRangeMaxRC[Y4_AXIS_ELEMENT], True);
    }
  else
    {
      XtSetSensitive(axisRangeMinRC[Y4_AXIS_ELEMENT], False);
      XtSetSensitive(axisRangeMaxRC[Y4_AXIS_ELEMENT], False);
    }
}

static void cartesianPlotAxisActivate(Widget w, XtPointer cd, XtPointer cbs)
{
  int buttonType = (intptr_t)cd;

  UNREFERENCED(w);
  UNREFERENCED(cbs);

  switch (buttonType)
    {
    case CP_CLOSE_BTN:
      XtPopdown(cartesianPlotAxisS);
      /* Since done with CP Axis dialog, reset the selected widget */
      executeTimeCartesianPlotWidget = NULL;
      break;
    }
}

/* Implements the Apply button.  Copies the tempPen to the
   globalResourceBundle
   Until then the user can cancel without affecting real data.  */
static void cartesianPlotDialogUpdateElement(void)
{
  DisplayInfo *cdi = currentDisplayInfo;
  int i;

  /* Read and store the text entries */
  cartesianPlotDialogStoreTextEntries();

  for (i = 0; i < MAX_TRACES; i++)
    {
      globalResourceBundle.cpData[i] = tempTrace[i];
    }
  /* and update the elements (since this level of "Apply" is analogous
   *        to changing text in a text field in the resource palette
   *        (don't need to traverse the display list since these changes
   *         aren't visible at the first level)
   */
  if (cdi)
    {
      DlElement *dlElement = FirstDlElement(
                                            cdi->selectedDlElementList);
      unhighlightSelectedElements();
      while (dlElement)
        {
          /* KE: This was =, changed to == */
          if (dlElement->structure.element->type == DL_CartesianPlot)
            updateElementFromGlobalResourceBundle(dlElement->structure.element);
          dlElement = dlElement->next;
        }
    }
  medmMarkDisplayBeingEdited(cdi);
}

/* Sets the values in the tempTrace when controls in the dialog are
   activated.  The row is stored in the high word of the cd and the
   type is stored in the low word.  If the type is less than 4, it is
   a button in the option menu, and the type is stored in the userData
   of the row column parent of the button.  Otherwise the type in the
   cd is valid. */
static void cartesianPlotDialogCb(Widget w, XtPointer cd, XtPointer cbs)
{
  /* The type is in the low word of the cd and the row is in the high word */
  int type = GETLOW((intptr_t)cd);
  int row = GETHIGH((intptr_t)cd);
  int button = 0;

#if DEBUG_CP
  print("\ncartesianPlotDialogCb: w=%x cd=%d type=%d row=%d\n",
        w, (int)cd, type, row);
#endif
  /* If the type is less than 2, the callback comes from the option menu button
   *   Find the real type from the userData of the RC parent of the button */
  if (type < 4)
    {
      XtPointer userData;

      button = type;
      XtVaGetValues(XtParent(w), XmNuserData, &userData, NULL);
      type = GETLOW((intptr_t)userData);
      row = GETHIGH((intptr_t)userData);
    }

  /* Check */
  if (row < 0 || row > MAX_TRACES)
    {
      medmPostMsg(1, "cartesianPlotDialogCb: Invalid row (%d)\n", row);
      return;
    }

  /* Put all the text values into tempTrace since we do not get a
     callback when they are entered */
  cartesianPlotDialogStoreTextEntries();

#if DEBUG_CP
  print("  Final values: type=%d row=%d pL=%x\n", type, row, pL);
#endif

  switch (type)
    {
    case CP_COLOR_BTN:
      setCurrentDisplayColorsInColorPalette(CPDATA_RC, row);
      XtPopup(colorS, XtGrabNone);
      break;
    case CP_YAXIS_BTN:
      tempTrace[row].yaxis = button;
#if defined(SCIPLOT)
      {
        int i;
        for (i = 0; i < MAX_TRACES; i++)
          {
            if (i == row)
              {
                continue;
              }
            if (tempTrace[i].yaxis == tempTrace[row].yaxis)
              {
                tempTrace[row].yside = tempTrace[i].yside;
                break;
              }
          }
      }
#endif
      break;
    case CP_YSIDE_BTN:
      tempTrace[row].yside = button; //0 = left, 1 = right
      {
        int i;
        for (i = 0; i < MAX_TRACES; i++)
          {
            if (i == row)
              {
                continue;
              }
            if (tempTrace[i].yaxis == tempTrace[row].yaxis)
              {
                tempTrace[i].yside = tempTrace[row].yside;
              }
          }
      }
      break;
    case CP_APPLY_BTN:
      cartesianPlotDialogUpdateElement();
#if 0
      XtPopdown(cartesianPlotS);
#endif
      break;
    case CP_CLOSE_BTN:
      if (colorS)
        XtPopdown(colorS);
      XtPopdown(cartesianPlotS);
      return;
      break;
    case CP_X_BTN:
      if (colorS)
        XtPopdown(colorS);
      XtPopdown(cartesianPlotS);
      cartesianPlotS = NULL;
      return;
    }

  /* Reset the dialog box */
  cartesianPlotDialogReset();
}

/* Reads the text entries and stores the values in the tempTrace.  There
   is no indication when a value is entered in a text entry, so we
   cannot do this when the value is changed.  (Perhaps we could, but
   we don't.)  */
static void cartesianPlotDialogStoreTextEntries(void)
{
  int i;
  char *string;

  /* Copy text values from dialog to tempTrace.  Other values
     should be up to date. */
  for (i = 0; i < MAX_TRACES; i++)
    {

      string = XmTextFieldGetString(table[i][0]);
      if (string)
        {
          strcpy(tempTrace[i].xdata, string);
          XtFree(string);
        }
      string = XmTextFieldGetString(table[i][1]);
      if (string)
        {
          strcpy(tempTrace[i].ydata, string);
          XtFree(string);
        }
    }
}

/* Udates the colors in the COLOR_COLUMN of the matrix */
void cpUpdateMatrixColors(int clr, int row)
{
  Widget w;
  Pixel pixel;
  Pixel *colormap = currentDisplayInfo->colormap;

  tempTrace[row].data_clr = clr;
  w = table[row][2];
  /* Use the specified color */
  pixel = colormap[tempTrace[row].data_clr];

#if DEBUG_COLOR
  print("cpUpdateMatrixColors: clr=%d row=%d pixel=%x w=%x\n",
        clr, row, pixel, w);
#endif
  if (w)
    XtVaSetValues(w, XmNbackground, pixel, NULL);
}

/*
 * create data dialog
 */
Widget createCartesianPlotDataDialog(Widget parent)
{
  Widget shell, w, wparent;
  //Widget columns[CP_COLS], labels[CP_COLS];
  Widget cpForm, cpActionAreaW, cpMatrixW;
#if defined(XRTGRAPH) || defined(SCIPLOT)
  XmString label, opt1, opt2, opt3, opt4;
#endif
  Arg args[12];
  int i, j, n;
  static Boolean first = True;

  /* Initialize file-scoped globals */
  if (first)
    {
      first = False;

      for (i = 0; i < MAX_TRACES; i++)
        {
          for (j = 0; j < CP_COLS; j++)
            {
              table[i][j] = NULL;
              /******************  CHECK NEXT LINE ******************/
              tempTrace[i].data_clr = CP_DEFAULT_CLR;
            }
        }
    }

  /*
   * Create the interface
   *
   *	       xdata | ydata |color | yaxis | yside
   *	       -----------------------------------
   *	    1 |  A      B      C      D       E
   *	    2 |
   *	    3 |
   *		     ...
   *		 OK     CANCEL
   */

  n = 0;
  XtSetArg(args[n], XmNautoUnmanage, False);
  n++;
  XtSetArg(args[n], XmNmarginHeight, 8);
  n++;
  XtSetArg(args[n], XmNmarginWidth, 8);
  n++;
  XtSetArg(args[n], XmNdialogStyle, XmDIALOG_PRIMARY_APPLICATION_MODAL);
  n++;
  cpForm = XmCreateFormDialog(parent, "cartesianPlotDataF", args, n);
  shell = XtParent(cpForm);
  n = 0;
  XtSetArg(args[n], XmNtitle, "Cartesian Plot Data");
  n++;
#if OMIT_RESIZE_HANDLES
  XtSetArg(args[n], XmNmwmDecorations, MWM_DECOR_ALL | MWM_DECOR_RESIZEH);
  n++;
  /* KE: The following is necessary for Exceed, which turns off the
     resize function with the handles.  It should not be necessary */
  XtSetArg(args[n], XmNmwmFunctions, MWM_FUNC_ALL);
  n++;
#endif
  XtSetValues(shell, args, n);
  XmAddWMProtocolCallback(shell, WM_DELETE_WINDOW,
                          cartesianPlotDialogCb, (XtPointer)CP_X_BTN);

  /* Create a row column for the matrix (cpMatrix) */
  cpMatrixW = XtVaCreateWidget("cpMatrix",
                               xmRowColumnWidgetClass, cpForm,
                               XmNpacking, XmPACK_TIGHT,
                               XmNorientation, XmHORIZONTAL,
                               XmNtopAttachment, XmATTACH_FORM,
                               XmNleftAttachment, XmATTACH_FORM,
                               XmNrightAttachment, XmATTACH_FORM,
                               NULL);

  for (j = 0; j < CP_COLS; j++)
    {
      /* Create a form for each column */
      w = XtVaCreateManagedWidget("columnF",
                                  xmFormWidgetClass, cpMatrixW,
                                  XmNfractionBase, MAX_TRACES + 1,
                                  NULL);
      //columns[j] = wparent = w;
      wparent = w;

      /* Create a column label */
      w = XtVaCreateManagedWidget(cpColumnLabels[j],
                                  xmLabelWidgetClass, wparent,
                                  XmNcolumns, cpColumnWidths[j],
                                  XmNalignment, XmALIGNMENT_CENTER,
                                  XmNtopAttachment, XmATTACH_POSITION,
                                  XmNbottomAttachment, XmATTACH_POSITION,
                                  XmNleftAttachment, XmATTACH_FORM,
                                  XmNrightAttachment, XmATTACH_FORM,
                                  XmNtopPosition, 0,
                                  XmNbottomPosition, 1,
                                  NULL);
      //labels[j] = w;

      for (i = 0; i < MAX_TRACES; i++)
        {
          switch (j)
            {
            case 0:
              w = XtVaCreateManagedWidget("xdata",
                                          xmTextFieldWidgetClass, wparent,
                                          XmNmaxLength, MAX_TOKEN_LENGTH - 1,
                                          XmNcolumns, cpColumnWidths[0],
                                          XmNtopAttachment, XmATTACH_POSITION,
                                          XmNbottomAttachment, XmATTACH_POSITION,
                                          XmNleftAttachment, XmATTACH_FORM,
                                          XmNrightAttachment, XmATTACH_FORM,
                                          XmNtopPosition, i + 1,
                                          XmNbottomPosition, i + 2,
                                          NULL);
              table[i][0] = w;
              break;
            case 1:
              w = XtVaCreateManagedWidget("ydata",
                                          xmTextFieldWidgetClass, wparent,
                                          XmNmaxLength, MAX_TOKEN_LENGTH - 1,
                                          XmNcolumns, cpColumnWidths[0],
                                          XmNtopAttachment, XmATTACH_POSITION,
                                          XmNbottomAttachment, XmATTACH_POSITION,
                                          XmNleftAttachment, XmATTACH_FORM,
                                          XmNrightAttachment, XmATTACH_FORM,
                                          XmNtopPosition, i + 1,
                                          XmNbottomPosition, i + 2,
                                          NULL);
              table[i][1] = w;
              break;
            case 2:
              w = XtVaCreateManagedWidget("color",
                                          xmDrawnButtonWidgetClass, wparent,
                                          XmNshadowType, XmSHADOW_IN,
                                          XmNtopAttachment, XmATTACH_POSITION,
                                          XmNbottomAttachment, XmATTACH_POSITION,
                                          XmNleftAttachment, XmATTACH_FORM,
                                          XmNrightAttachment, XmATTACH_FORM,
                                          XmNtopPosition, i + 1,
                                          XmNbottomPosition, i + 2,
                                          NULL);
              XtAddCallback(w, XmNactivateCallback, cartesianPlotDialogCb,
                            (XtPointer)(intptr_t)(CP_COLOR_BTN + SETHIGH(i)));
              table[i][2] = w;
              break;
#if defined(XRTGRAPH)
            case 3:
              label = XmStringCreateLocalized("Y axis");
              opt1 = XmStringCreateLocalized("Y");
              opt2 = XmStringCreateLocalized("Y2");
              w = XmVaCreateSimpleOptionMenu(wparent, "yaxis",
                                             label, '\0', 0, cartesianPlotDialogCb,
                                             XmVaPUSHBUTTON, opt1, '\0', NULL, NULL,
                                             XmVaPUSHBUTTON, opt2, '\0', NULL, NULL,
                                             XmNuserData, (XtPointer)(intptr_t)(CP_YAXIS_BTN + SETHIGH(i)),
                                             XmNtopAttachment, XmATTACH_POSITION,
                                             XmNbottomAttachment, XmATTACH_POSITION,
                                             XmNleftAttachment, XmATTACH_FORM,
                                             XmNrightAttachment, XmATTACH_FORM,
                                             XmNtopPosition, i + 1,
                                             XmNbottomPosition, i + 2,
                                             NULL);
              XtManageChild(w);
              optionMenuRemoveLabel(w);
              XmStringFree(label);
              XmStringFree(opt1);
              XmStringFree(opt2);
              table[i][3] = w;
              break;
#endif
#if defined(SCIPLOT)
            case 3:
              label = XmStringCreateLocalized("Y axis");
              opt1 = XmStringCreateLocalized("Y");
              opt2 = XmStringCreateLocalized("Y2");
              opt3 = XmStringCreateLocalized("Y3");
              opt4 = XmStringCreateLocalized("Y4");
              w = XmVaCreateSimpleOptionMenu(wparent, "yaxis",
                                             label, '\0', 0, cartesianPlotDialogCb,
                                             XmVaPUSHBUTTON, opt1, '\0', NULL, NULL,
                                             XmVaPUSHBUTTON, opt2, '\0', NULL, NULL,
                                             XmVaPUSHBUTTON, opt3, '\0', NULL, NULL,
                                             XmVaPUSHBUTTON, opt4, '\0', NULL, NULL,
                                             XmNuserData, (XtPointer)(intptr_t)(CP_YAXIS_BTN + SETHIGH(i)),
                                             XmNtopAttachment, XmATTACH_POSITION,
                                             XmNbottomAttachment, XmATTACH_POSITION,
                                             XmNleftAttachment, XmATTACH_FORM,
                                             XmNrightAttachment, XmATTACH_FORM,
                                             XmNtopPosition, i + 1,
                                             XmNbottomPosition, i + 2,
                                             NULL);
              XtManageChild(w);
              optionMenuRemoveLabel(w);
              XmStringFree(label);
              XmStringFree(opt1);
              XmStringFree(opt2);
              XmStringFree(opt3);
              XmStringFree(opt4);
              table[i][3] = w;
              break;
            case 4:
              label = XmStringCreateLocalized("Y Side");
              opt1 = XmStringCreateLocalized("Left");
              opt2 = XmStringCreateLocalized("Right");
              w = XmVaCreateSimpleOptionMenu(wparent, "yside",
                                             label, '\0', 0, cartesianPlotDialogCb,
                                             XmVaPUSHBUTTON, opt1, '\0', NULL, NULL,
                                             XmVaPUSHBUTTON, opt2, '\0', NULL, NULL,
                                             XmNuserData, (XtPointer)(intptr_t)(CP_YSIDE_BTN + SETHIGH(i)),
                                             XmNtopAttachment, XmATTACH_POSITION,
                                             XmNbottomAttachment, XmATTACH_POSITION,
                                             XmNleftAttachment, XmATTACH_FORM,
                                             XmNrightAttachment, XmATTACH_FORM,
                                             XmNtopPosition, i + 1,
                                             XmNbottomPosition, i + 2,
                                             NULL);
              XtManageChild(w);
              optionMenuRemoveLabel(w);
              XmStringFree(label);
              XmStringFree(opt1);
              XmStringFree(opt2);
              table[i][4] = w;
              break;
#endif
            }
        }
    }

  /* Create action area */
  w = XtVaCreateWidget("cpActionArea",
                       xmFormWidgetClass, cpForm,
                       XmNshadowThickness, 0,
                       XmNfractionBase, 5,
                       XmNtopAttachment, XmATTACH_WIDGET,
                       XmNtopWidget, cpMatrixW,
                       XmNleftAttachment, XmATTACH_FORM,
                       XmNrightAttachment, XmATTACH_FORM,
                       XmNbottomAttachment, XmATTACH_FORM,
                       NULL);
  cpActionAreaW = wparent = w;

  w = XtVaCreateManagedWidget("Apply",
                              xmPushButtonWidgetClass, wparent,
                              XmNtopAttachment, XmATTACH_FORM,
                              XmNtopOffset, 12,
                              XmNbottomAttachment, XmATTACH_FORM,
                              XmNbottomOffset, 12,
                              XmNleftAttachment, XmATTACH_POSITION,
                              XmNleftPosition, 1,
                              XmNrightAttachment, XmATTACH_POSITION,
                              XmNrightPosition, 2,
                              NULL);
  XtAddCallback(w, XmNactivateCallback,
                cartesianPlotDialogCb, (XtPointer)CP_APPLY_BTN);

  w = XtVaCreateManagedWidget("Close",
                              xmPushButtonWidgetClass, wparent,
                              XmNtopAttachment, XmATTACH_FORM,
                              XmNtopOffset, 12,
                              XmNbottomAttachment, XmATTACH_FORM,
                              XmNbottomOffset, 12,
                              XmNleftAttachment, XmATTACH_POSITION,
                              XmNleftPosition, 3,
                              XmNrightAttachment, XmATTACH_POSITION,
                              XmNrightPosition, 4,
                              NULL);
  XtAddCallback(w, XmNactivateCallback,
                cartesianPlotDialogCb, (XtPointer)CP_CLOSE_BTN);

  /* Fill in the tempTraces */
  for (i = 0; i < MAX_TRACES; i++)
    {
      tempTrace[i] = globalResourceBundle.cpData[i];
    }

  /* Fill in the values */
  cartesianPlotDialogReset();

  /* Manage the managers */
  XtManageChild(cpMatrixW);
  XtManageChild(cpActionAreaW);
  XtManageChild(cpForm);

  return shell;
}

/* Sets values into the dialog from the tempTrace */
static void cartesianPlotDialogReset()
{
  Widget w;
  Pixel pixel = 0;
  int i;
  DlTrace *pTrace;
  Pixel *colormap = currentDisplayInfo->colormap;

  /* Loop over rows */
  for (i = 0; i < MAX_TRACES; i++)
    {
      /*  Set the pointers */
      pTrace = &tempTrace[i];

      /* xdata */
      w = table[i][0];
      XmTextFieldSetString(table[i][0], pTrace->xdata);
      XtVaSetValues(w, XmNeditable, True, NULL);

      /* ydata */
      w = table[i][1];
      XmTextFieldSetString(table[i][1], pTrace->ydata);
      XtVaSetValues(w, XmNeditable, True, NULL);

      /* Color */
      w = table[i][2];
      if (*pTrace->xdata || *pTrace->ydata)
        {
          /* Use the specified color */
          pixel = colormap[pTrace->data_clr];
        }
      else
        {
          /* Use the background color of the xdata */
          XtVaGetValues(table[i][0], XmNbackground, &pixel, NULL);
        }
      if (w)
        XtVaSetValues(w, XmNbackground, pixel, NULL);

#if defined(XRTGRAPH) || defined(SCIPLOT)
      /* yaxis */
      w = table[i][3];
      XtSetSensitive(w, True);
      optionMenuSet(w, pTrace->yaxis);
#endif
#if defined(SCIPLOT)
      /* yside */
      w = table[i][4];
      XtSetSensitive(w, True);
      optionMenuSet(w, pTrace->yside);
#endif
    }
}

/*
 * access function (for file-scoped globals, etc) to udpate the
 *	cartesian plot data dialog with the values currently in
 *	globalResourceBundle
 */
void updateCartesianPlotDataDialog()
{
  int i;

  /* Fill in the tempTraces */
  for (i = 0; i < MAX_TRACES; i++)
    {
      tempTrace[i] = globalResourceBundle.cpData[i];
    }
  cartesianPlotDialogReset();
}

#if DEBUG_ERASE
static void dumpCartesianPlotData(const char *title,
                                  MedmCartesianPlot *pcp)
{
  int i, j;

  if (!pcp)
    return;

  /* Heading */
  if (title)
    {
      print("\nCartesian Plot Dump [%s]\n", title);
    }
  else
    {
      print("\nCartesian Plot Dump\n");
    }

  /* Loop over traces */
  for (i = 0; i < pcp->nTraces; i++)
    {
      XYTrace *pt = &(pcp->xyTrace[i]);
      if ((pt->recordX) || (pt->recordY))
        {
          int pointsUsed = CpDataGetPointsUsed(pt->hcp, pt->trace);

          print("Trace %d: pointsUsed=%d\n", i, pointsUsed);
          if (pt->recordX)
            {
              print("  x=[%x]%s\n",
                    pt->recordX, pt->recordX->name);
            }
          if (pt->recordX)
            {
              print("  y=[%x]%s\n",
                    pt->recordY, pt->recordY->name);
            }
          for (j = 0; j < pointsUsed; j++)
            {
              print("  %2d %15g %15g\n", j,
                    CpDataGetXElement(pt->hcp, pt->trace, j),
                    CpDataGetYElement(pt->hcp, pt->trace, j));
            }
        }
    }

  /* Erase Channel */
  if (pcp->eraseCh.recordX)
    {
      print("Erase Channel:   [%x] %34s %15g\n",
            pcp->eraseCh.recordX,
            pcp->eraseCh.recordX->name,
            pcp->eraseCh.recordX->value);
    }

  /* Trigger Channel */
  if (pcp->triggerCh.recordX)
    {
      print("Trigger Channel: [%x] %34s %15g\n",
            pcp->triggerCh.recordX,
            pcp->triggerCh.recordX->name,
            pcp->triggerCh.recordX->value);
    }

  /* Count Channel */
  if (pcp->countCh.recordX)
    {
      print("Count Channel: [%x] %34s %15g\n",
            pcp->countCh.recordX,
            pcp->countCh.recordX->name,
            pcp->countCh.recordX->value);
    }

  print("\n");
}
#endif
