/* Header file for print utilities */

#ifndef __UTIL_PRINT_H__
#define __UTIL_PRINT_H__

#define PRINT_PORTRAIT  0
#define PRINT_LANDSCAPE 1
#define PRINT_A         0
#define PRINT_B         1
#define PRINT_A3        2
#define PRINT_A4        3
#define PRINT_BUF_SIZE  256

#ifndef ALLOCATE_STORAGE
#define EXTERN extern
extern char *printerSizeTable[4];
extern char *printerOrientationTable[2];
#else
#define EXTERN
char *printerSizeTable[4] = { 
    "A [Letter]", "B", "A3", "A4"};
char *printerOrientationTable[2] = { 
    "Portrait", "Landscape"};
#endif

EXTERN char printCommand[PRINT_BUF_SIZE];
EXTERN char printFile[PRINT_BUF_SIZE];
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
