/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/* Header file for print utilities */

#ifndef __UTIL_PRINT_H__
#define __UTIL_PRINT_H__

#define PRINT_PORTRAIT  0
#define PRINT_LANDSCAPE 1
#define PRINT_A  0
#define PRINT_B  1
#define PRINT_A3 2
#define PRINT_A4 3
#define PRINT_TITLE_NONE       0
#define PRINT_TITLE_SHORT_NAME 1
#define PRINT_TITLE_LONG_NAME  2
#define PRINT_TITLE_SPECIFIED  3

#define PRINT_BUF_SIZE     256

#ifndef ALLOCATE_STORAGE
#define EXTERN extern
extern char *printerSizeTable[4];
extern char *printerOrientationTable[2];
extern char *printerTitleTable[4];
#else
#define EXTERN
char *printerSizeTable[4] = {
    "A (Letter)", "B", "A3", "A4"};
char *printerOrientationTable[2] = {
    "Portrait", "Landscape"};
char *printerTitleTable[4] = {
    "None", "Short Display Name", "Long Display Name", "Specified Title"};
#endif

EXTERN char printCommand[PRINT_BUF_SIZE];
EXTERN char printFile[PRINT_BUF_SIZE];
EXTERN char printTitleString[PRINT_BUF_SIZE];
EXTERN int printRemoveTempFiles;
EXTERN int printOrientation;
EXTERN int printSize;
EXTERN int printToFile;
EXTERN int printTitle;
EXTERN int printTime;
EXTERN int printDate;
EXTERN double printWidth, printHeight;

/* Function prototypes */
int utilPrint(Display *display, Widget w, char *xwdFileName, char *title);

/* This is declared in proto.h, but is needed here */
int xInfoMsg(Widget parent, const char *fmt, ...);

/* Global variables */

#endif  /* __UTIL_PRINT_H__ */
