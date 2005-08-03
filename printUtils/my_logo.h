/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
#define LOGONAME "mylogo"
#define COMPANYNAME1 "F&B Widget"
#define COMPANYNAME2 "Corporation"

char *my_logo[] = {
    "%% Comments for your logo.",
    "/mylogo {",
    "%% This procedure needs five parameters to be passed to it:",
    "%% 1: Horizontal coordinate of lower left corner of logo",
    "%% 2: Vertical coordinate of lower left corner of logo",
    "%% 3: Diameter of logo",
    "%% 4: Color of \"GE\" characters against their background:",
    "%%       light (1) or dark (0)",
    "%% 5: Color of logo: black (0) or platinum (0.6)",

    " /logocolor exch def",
    " /charactercolor exch def",
    " /scalefactor exch def",
    " /Yposition exch def",
    " /Xposition exch def",

    "/Times-Italic findfont 6 scalefont setfont",
    "Xposition Yposition .2 inch sub moveto",
    "(Sorry. You have to write your own logo procedure.) show",
    "  } def  %% end of logo procedure",
    0
};
