/*************************************************************************\
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

#include "medm.h"

#include "medmPix25.xpm"

static XImage arc25, bar25, byte25, bezierCurve25, cartesianPlot25, choiceButton25,
  image25, indicator25, line25, menu25, messageButton25, meter25,
  oval25, polygon25, polyline25, rectangle25, relatedDisplay25,
  select25, shellCommand25, wheelSwitch25, stripChart25, surfacePlot25, text25,
  textEntry25, textUpdate25, valuator25;


/*
 * initialize the image cache with the bitmap images used in the
 *	generation of pixmaps
 *	(should probably throw these away on exit too)
 */
void medmInitializeImageCache()
{
    arc25.width = arc25_width;
    arc25.height = arc25_height;
    arc25.data = (char *)arc25_bits;
    arc25.xoffset = 0;
    arc25.format = XYBitmap;
    arc25.byte_order = MSBFirst;
    arc25.bitmap_pad = 8;
    arc25.bitmap_bit_order = LSBFirst;
    arc25.bitmap_unit = 8;
    arc25.depth = 1;
    arc25.bytes_per_line = 4;
    arc25.obdata = NULL;
    XmInstallImage(&arc25,"arc25");

    bar25.width = bar25_width;
    bar25.height = bar25_height;
    bar25.data = (char *)bar25_bits;
    bar25.xoffset = 0;
    bar25.format = XYBitmap;
    bar25.byte_order = MSBFirst;
    bar25.bitmap_pad = 8;
    bar25.bitmap_bit_order = LSBFirst;
    bar25.bitmap_unit = 8;
    bar25.depth = 1;
    bar25.bytes_per_line = 4;
    bar25.obdata = NULL;
    XmInstallImage(&bar25,"bar25");

    byte25.width = byte25_width;
    byte25.height = byte25_height;
    byte25.data = (char *)byte25_bits;
    byte25.xoffset = 0;
    byte25.format = XYBitmap;
    byte25.byte_order = MSBFirst;
    byte25.bitmap_pad = 8;
    byte25.bitmap_bit_order = LSBFirst;
    byte25.bitmap_unit = 8;
    byte25.depth = 1;
    byte25.bytes_per_line = 4;
    byte25.obdata = NULL;
    XmInstallImage(&byte25,"byte25");

    bezierCurve25.width = bezierCurve25_width;
    bezierCurve25.height = bezierCurve25_height;
    bezierCurve25.data = (char *)bezierCurve25_bits;
    bezierCurve25.xoffset = 0;
    bezierCurve25.format = XYBitmap;
    bezierCurve25.byte_order = MSBFirst;
    bezierCurve25.bitmap_pad = 8;
    bezierCurve25.bitmap_bit_order = LSBFirst;
    bezierCurve25.bitmap_unit = 8;
    bezierCurve25.depth = 1;
    bezierCurve25.bytes_per_line = 4;
    bezierCurve25.obdata = NULL;
    XmInstallImage(&bezierCurve25,"bezierCurve25");

    choiceButton25.width = choiceButton25_width;
    choiceButton25.height = choiceButton25_height;
    choiceButton25.data = (char *)choiceButton25_bits;
    choiceButton25.xoffset = 0;
    choiceButton25.format = XYBitmap;
    choiceButton25.byte_order = MSBFirst;
    choiceButton25.bitmap_pad = 8;
    choiceButton25.bitmap_bit_order = LSBFirst;
    choiceButton25.bitmap_unit = 8;
    choiceButton25.depth = 1;
    choiceButton25.bytes_per_line = 4;
    choiceButton25.obdata = NULL;
    XmInstallImage(&choiceButton25,"choiceButton25");

    cartesianPlot25.width = cartesianPlot25_width;
    cartesianPlot25.height = cartesianPlot25_height;
    cartesianPlot25.data = (char *)cartesianPlot25_bits;
    cartesianPlot25.xoffset = 0;
    cartesianPlot25.format = XYBitmap;
    cartesianPlot25.byte_order = MSBFirst;
    cartesianPlot25.bitmap_pad = 8;
    cartesianPlot25.bitmap_bit_order = LSBFirst;
    cartesianPlot25.bitmap_unit = 8;
    cartesianPlot25.depth = 1;
    cartesianPlot25.bytes_per_line = 4;
    cartesianPlot25.obdata = NULL;
    XmInstallImage(&cartesianPlot25,"cartesianPlot25");

    image25.width = image25_width;
    image25.height = image25_height;
    image25.data = (char *)image25_bits;
    image25.xoffset = 0;
    image25.format = XYBitmap;
    image25.byte_order = MSBFirst;
    image25.bitmap_pad = 8;
    image25.bitmap_bit_order = LSBFirst;
    image25.bitmap_unit = 8;
    image25.depth = 1;
    image25.bytes_per_line = 4;
    image25.obdata = NULL;
    XmInstallImage(&image25,"image25");

    indicator25.width = indicator25_width;
    indicator25.height = indicator25_height;
    indicator25.data = (char *)indicator25_bits;
    indicator25.xoffset = 0;
    indicator25.format = XYBitmap;
    indicator25.byte_order = MSBFirst;
    indicator25.bitmap_pad = 8;
    indicator25.bitmap_bit_order = LSBFirst;
    indicator25.bitmap_unit = 8;
    indicator25.depth = 1;
    indicator25.bytes_per_line = 4;
    indicator25.obdata = NULL;
    XmInstallImage(&indicator25,"indicator25");

    line25.width = line25_width;
    line25.height = line25_height;
    line25.data = (char *)line25_bits;
    line25.xoffset = 0;
    line25.format = XYBitmap;
    line25.byte_order = MSBFirst;
    line25.bitmap_pad = 8;
    line25.bitmap_bit_order = LSBFirst;
    line25.bitmap_unit = 8;
    line25.depth = 1;
    line25.bytes_per_line = 4;
    line25.obdata = NULL;
    XmInstallImage(&line25,"line25");

    menu25.width = menu25_width;
    menu25.height = menu25_height;
    menu25.data = (char *)menu25_bits;
    menu25.xoffset = 0;
    menu25.format = XYBitmap;
    menu25.byte_order = MSBFirst;
    menu25.bitmap_pad = 8;
    menu25.bitmap_bit_order = LSBFirst;
    menu25.bitmap_unit = 8;
    menu25.depth = 1;
    menu25.bytes_per_line = 4;
    menu25.obdata = NULL;
    XmInstallImage(&menu25,"menu25");

    messageButton25.width = messageButton25_width;
    messageButton25.height = messageButton25_height;
    messageButton25.data = (char *)messageButton25_bits;
    messageButton25.xoffset = 0;
    messageButton25.format = XYBitmap;
    messageButton25.byte_order = MSBFirst;
    messageButton25.bitmap_pad = 8;
    messageButton25.bitmap_bit_order = LSBFirst;
    messageButton25.bitmap_unit = 8;
    messageButton25.depth = 1;
    messageButton25.bytes_per_line = 4;
    messageButton25.obdata = NULL;
    XmInstallImage(&messageButton25,"messageButton25");

    meter25.width = meter25_width;
    meter25.height = meter25_height;
    meter25.data = (char *)meter25_bits;
    meter25.xoffset = 0;
    meter25.format = XYBitmap;
    meter25.byte_order = MSBFirst;
    meter25.bitmap_pad = 8;
    meter25.bitmap_bit_order = LSBFirst;
    meter25.bitmap_unit = 8;
    meter25.depth = 1;
    meter25.bytes_per_line = 4;
    meter25.obdata = NULL;
    XmInstallImage(&meter25,"meter25");

    oval25.width = oval25_width;
    oval25.height = oval25_height;
    oval25.data = (char *)oval25_bits;
    oval25.xoffset = 0;
    oval25.format = XYBitmap;
    oval25.byte_order = MSBFirst;
    oval25.bitmap_pad = 8;
    oval25.bitmap_bit_order = LSBFirst;
    oval25.bitmap_unit = 8;
    oval25.depth = 1;
    oval25.bytes_per_line = 4;
    oval25.obdata = NULL;
    XmInstallImage(&oval25,"oval25");

    polygon25.width = polygon25_width;
    polygon25.height = polygon25_height;
    polygon25.data = (char *)polygon25_bits;
    polygon25.xoffset = 0;
    polygon25.format = XYBitmap;
    polygon25.byte_order = MSBFirst;
    polygon25.bitmap_pad = 8;
    polygon25.bitmap_bit_order = LSBFirst;
    polygon25.bitmap_unit = 8;
    polygon25.depth = 1;
    polygon25.bytes_per_line = 4;
    polygon25.obdata = NULL;
    XmInstallImage(&polygon25,"polygon25");

    polyline25.width = polyline25_width;
    polyline25.height = polyline25_height;
    polyline25.data = (char *)polyline25_bits;
    polyline25.xoffset = 0;
    polyline25.format = XYBitmap;
    polyline25.byte_order = MSBFirst;
    polyline25.bitmap_pad = 8;
    polyline25.bitmap_bit_order = LSBFirst;
    polyline25.bitmap_unit = 8;
    polyline25.depth = 1;
    polyline25.bytes_per_line = 4;
    polyline25.obdata = NULL;
    XmInstallImage(&polyline25,"polyline25");

    rectangle25.width = rectangle25_width;
    rectangle25.height = rectangle25_height;
    rectangle25.data = (char *)rectangle25_bits;
    rectangle25.xoffset = 0;
    rectangle25.format = XYBitmap;
    rectangle25.byte_order = MSBFirst;
    rectangle25.bitmap_pad = 8;
    rectangle25.bitmap_bit_order = LSBFirst;
    rectangle25.bitmap_unit = 8;
    rectangle25.depth = 1;
    rectangle25.bytes_per_line = 4;
    rectangle25.obdata = NULL;
    XmInstallImage(&rectangle25,"rectangle25");

    relatedDisplay25.width = relatedDisplay25_width;
    relatedDisplay25.height = relatedDisplay25_height;
    relatedDisplay25.data = (char *)relatedDisplay25_bits;
    relatedDisplay25.xoffset = 0;
    relatedDisplay25.format = XYBitmap;
    relatedDisplay25.byte_order = MSBFirst;
    relatedDisplay25.bitmap_pad = 8;
    relatedDisplay25.bitmap_bit_order = LSBFirst;
    relatedDisplay25.bitmap_unit = 8;
    relatedDisplay25.depth = 1;
    relatedDisplay25.bytes_per_line = 4;
    relatedDisplay25.obdata = NULL;
    XmInstallImage(&relatedDisplay25,"relatedDisplay25");

    select25.width = select25_width;
    select25.height = select25_height;
    select25.data = (char *)select25_bits;
    select25.xoffset = 0;
    select25.format = XYBitmap;
    select25.byte_order = MSBFirst;
    select25.bitmap_pad = 8;
    select25.bitmap_bit_order = LSBFirst;
    select25.bitmap_unit = 8;
    select25.depth = 1;
    select25.bytes_per_line = 4;
    select25.obdata = NULL;
    XmInstallImage(&select25,"select25");

    shellCommand25.width = shellCommand25_width;
    shellCommand25.height = shellCommand25_height;
    shellCommand25.data = (char *)shellCommand25_bits;
    shellCommand25.xoffset = 0;
    shellCommand25.format = XYBitmap;
    shellCommand25.byte_order = MSBFirst;
    shellCommand25.bitmap_pad = 8;
    shellCommand25.bitmap_bit_order = LSBFirst;
    shellCommand25.bitmap_unit = 8;
    shellCommand25.depth = 1;
    shellCommand25.bytes_per_line = 4;
    shellCommand25.obdata = NULL;
    XmInstallImage(&shellCommand25,"shellCommand25");

    wheelSwitch25.width = wheelSwitch25_width;
    wheelSwitch25.height = wheelSwitch25_height;
    wheelSwitch25.data = (char *)wheelSwitch25_bits;
    wheelSwitch25.xoffset = 0;
    wheelSwitch25.format = XYBitmap;
    wheelSwitch25.byte_order = MSBFirst;
    wheelSwitch25.bitmap_pad = 8;
    wheelSwitch25.bitmap_bit_order = LSBFirst;
    wheelSwitch25.bitmap_unit = 8;
    wheelSwitch25.depth = 1;
    wheelSwitch25.bytes_per_line = 4;
    wheelSwitch25.obdata = NULL;
    XmInstallImage(&wheelSwitch25,"wheelSwitch25");

    stripChart25.width = stripChart25_width;
    stripChart25.height = stripChart25_height;
    stripChart25.data = (char *)stripChart25_bits;
    stripChart25.xoffset = 0;
    stripChart25.format = XYBitmap;
    stripChart25.byte_order = MSBFirst;
    stripChart25.bitmap_pad = 8;
    stripChart25.bitmap_bit_order = LSBFirst;
    stripChart25.bitmap_unit = 8;
    stripChart25.depth = 1;
    stripChart25.bytes_per_line = 4;
    stripChart25.obdata = NULL;
    XmInstallImage(&stripChart25,"stripChart25");

    surfacePlot25.width = surfacePlot25_width;
    surfacePlot25.height = surfacePlot25_height;
    surfacePlot25.data = (char *)surfacePlot25_bits;
    surfacePlot25.xoffset = 0;
    surfacePlot25.format = XYBitmap;
    surfacePlot25.byte_order = MSBFirst;
    surfacePlot25.bitmap_pad = 8;
    surfacePlot25.bitmap_bit_order = LSBFirst;
    surfacePlot25.bitmap_unit = 8;
    surfacePlot25.depth = 1;
    surfacePlot25.bytes_per_line = 4;
    surfacePlot25.obdata = NULL;
    XmInstallImage(&surfacePlot25,"surfacePlot25");

    text25.width = text25_width;
    text25.height = text25_height;
    text25.data = (char *)text25_bits;
    text25.xoffset = 0;
    text25.format = XYBitmap;
    text25.byte_order = MSBFirst;
    text25.bitmap_pad = 8;
    text25.bitmap_bit_order = LSBFirst;
    text25.bitmap_unit = 8;
    text25.depth = 1;
    text25.bytes_per_line = 4;
    text25.obdata = NULL;
    XmInstallImage(&text25,"text25");

    textEntry25.width = textEntry25_width;
    textEntry25.height = textEntry25_height;
    textEntry25.data = (char *)textEntry25_bits;
    textEntry25.xoffset = 0;
    textEntry25.format = XYBitmap;
    textEntry25.byte_order = MSBFirst;
    textEntry25.bitmap_pad = 8;
    textEntry25.bitmap_bit_order = LSBFirst;
    textEntry25.bitmap_unit = 8;
    textEntry25.depth = 1;
    textEntry25.bytes_per_line = 4;
    textEntry25.obdata = NULL;
    XmInstallImage(&textEntry25,"textEntry25");

    textUpdate25.width = textUpdate25_width;
    textUpdate25.height = textUpdate25_height;
    textUpdate25.data = (char *)textUpdate25_bits;
    textUpdate25.xoffset = 0;
    textUpdate25.format = XYBitmap;
    textUpdate25.byte_order = MSBFirst;
    textUpdate25.bitmap_pad = 8;
    textUpdate25.bitmap_bit_order = LSBFirst;
    textUpdate25.bitmap_unit = 8;
    textUpdate25.depth = 1;
    textUpdate25.bytes_per_line = 4;
    textUpdate25.obdata = NULL;
    XmInstallImage(&textUpdate25,"textUpdate25");

    valuator25.width = valuator25_width;
    valuator25.height = valuator25_height;
    valuator25.data = (char *)valuator25_bits;
    valuator25.xoffset = 0;
    valuator25.format = XYBitmap;
    valuator25.byte_order = MSBFirst;
    valuator25.bitmap_pad = 8;
    valuator25.bitmap_bit_order = LSBFirst;
    valuator25.bitmap_unit = 8;
    valuator25.depth = 1;
    valuator25.bytes_per_line = 4;
    valuator25.obdata = NULL;
    XmInstallImage(&valuator25,"valuator25");
}

/*
 * clean out the image cache
 */
void medmClearImageCache()
{
    XmUninstallImage(&arc25);
    XmUninstallImage(&bar25);
    XmUninstallImage(&byte25);
    XmUninstallImage(&bezierCurve25);
    XmUninstallImage(&choiceButton25);
    XmUninstallImage(&cartesianPlot25);
    XmUninstallImage(&image25);
    XmUninstallImage(&indicator25);
    XmUninstallImage(&line25);
    XmUninstallImage(&menu25);
    XmUninstallImage(&messageButton25);
    XmUninstallImage(&meter25);
    XmUninstallImage(&oval25);
    XmUninstallImage(&polygon25);
    XmUninstallImage(&polyline25);
    XmUninstallImage(&rectangle25);
    XmUninstallImage(&relatedDisplay25);
    XmUninstallImage(&select25);
    XmUninstallImage(&shellCommand25);
    XmUninstallImage(&wheelSwitch25);
    XmUninstallImage(&stripChart25);
    XmUninstallImage(&surfacePlot25);
    XmUninstallImage(&text25);
    XmUninstallImage(&textEntry25);
    XmUninstallImage(&textUpdate25);
    XmUninstallImage(&valuator25);
}
