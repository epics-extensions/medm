/*
 * Copyright(c) 1992 Bell Communications Research, Inc. (Bellcore)
 *                        All rights reserved
 * Permission to use, copy, modify and distribute this material for
 * any purpose and without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies, and that the name of Bellcore not be used in advertising
 * or publicity pertaining to this material without the specific,
 * prior written permission of an authorized representative of
 * Bellcore.
 *
 * BELLCORE MAKES NO REPRESENTATIONS AND EXTENDS NO WARRANTIES, EX-
 * PRESS OR IMPLIED, WITH RESPECT TO THE SOFTWARE, INCLUDING, BUT
 * NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR ANY PARTICULAR PURPOSE, AND THE WARRANTY AGAINST IN-
 * FRINGEMENT OF PATENTS OR OTHER INTELLECTUAL PROPERTY RIGHTS.  THE
 * SOFTWARE IS PROVIDED "AS IS", AND IN NO EVENT SHALL BELLCORE OR
 * ANY OF ITS AFFILIATES BE LIABLE FOR ANY DAMAGES, INCLUDING ANY
 * LOST PROFITS OR OTHER INCIDENTAL OR CONSEQUENTIAL DAMAGES RELAT-
 * ING TO THE SOFTWARE.
 */

#include "Matrix.h"

/*
 * Simple example of loaded Matrix
 */

#define ROWS		6
#define COLUMNS		5

/* Function prototypes */
int main(int argc, char *argv[]);


static String rows[ROWS][COLUMNS] = {
    {"Orange", "12", "Rough", "Inches", "Large"},
    {"Blue", "323", "Smooth", "Feet", "Medium"},
    {"Yellow", "456", "Bristly", "Meters", "Large"},
    {"Green", "1", "Knobby", "Miles", "Small"},
    {"Pink", "33", "Hairy", "Quarts", "Small"},
    {"Black", "7", "Silky", "Gallons", "Small"},
};
String *cells[ROWS];

short widths[5] = { 6, 3, 10, 10, 10 };

int main(int argc, char *argv[])
{
    Widget toplevel, mw;
    XtAppContext app;
    int i;

    toplevel = XtVaAppInitialize(&app, "Simple",
      NULL, 0,
      &argc, argv,
      NULL,
      NULL);

    for (i = 0; i < ROWS; i++)
      cells[i] = &rows[i][0];

    mw = XtVaCreateManagedWidget("mw",
      xbaeMatrixWidgetClass, toplevel,
      XmNrows,		ROWS,
      XmNcolumns,		COLUMNS,
      XmNcolumnWidths,	widths,
      XmNcells,		cells,
      NULL);

    XtRealizeWidget(toplevel);
    XtAppMainLoop(app);

    return(0);
}
