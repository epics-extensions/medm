/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/* Header file for printUtils routines */

#define LPRINTF_SIZE 1024

#include "xwd2ps.h"

/* Function prototypes */

/* External */
void print(const char *fmt, ...);

/* xwd2ps.h */
int xwd2ps(int argc, char **argv, FILE *fo);

/* xwd.c */
int xwd(Display *display, Window window, char *file);

/* pUtils */
void get_time_and_date(char mytime[], char mydate[]);
void xwd2ps_swapshort(register char *bp, register long n);
void xwd2ps_swaplong(register char *bp, register long n);
void xwd2ps_usage(void);
float fMax(float a, float b);

/* ps_utils.c */
void outputBorder(FILE *fo, Image the_image);
void outputDate(FILE *fo, Image the_image);
void outputTitle(FILE *fo, Image the_image, Options the_options);
void outputTime(FILE *fo, Image the_image);
void outputColorImage(FILE *fo);
void outputLogo(FILE *fo, Image the_image);
void printPS(FILE *fo, char **p);
void printEPSF(FILE *fo, Image image, Page  page, char  *file_name);

/* utilPrint.c */
int errMsg(const char *fmt, ...);
