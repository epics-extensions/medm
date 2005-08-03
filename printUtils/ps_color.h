/*************************************************************************\
* Copyright (c) 2002 The University of Chicago, as Operator of Argonne
* National Laboratory.
* Copyright (c) 2002 The Regents of the University of California, as
* Operator of Los Alamos National Laboratory.
* This file is distributed subject to a Software License Agreement found
* in the file LICENSE that is included with this distribution.
\*************************************************************************/
/*
 *** ps_color.h - the program to print a color image
 ***               on an old printer that does not
 ***               support color PostScript
 * $Header$
 */


char *ColorImage[] = {
    "%%Title: colorimage.ps",
    "% Written 11-4-88 by Bob Tatar",
    "% U.S. Mail: GE-CRD, PO Box 8, KW-C214, Schenectady, NY 12301",
    "%    E-Mail: tatar@crd.ge.com",
    "% colorimage procedure to be used on monochrome printers",
    "% or when the colorimage procedure is not available",
    "% NOTE: Only 1 color mode is supported: single proc. & RGB",
    "",
    "systemdict /colorimage known not {        % only create if not in systemdict",
    "  % Utility procedure for colorimage operator.  This procedure takes a",
    "  % string of rgb encoded values and creates a string 1/3 as long with",
    "  % monochrome values.  This procedure assumes 8 bits/color (i.e. ",
    "  % 1 character/color)",
    "  % storage format for input string:  (r1 g1 b1  r2 g2 b2  r3 g3 b3  ... )",
    "  % storage format for output string: (g1  g2  g3 ... )",
    "  ",
    "  /colortograyscale { %def                % (string)",
    "    dup /rgbdata exch store               % (string)",
    "    length 3 idiv                         % Ns/3 ",
    "    /npixls exch store                    % ; npixls => Ns/3",
    "    /indx 0 store                         % ; indx => 0",
    "    /pixls npixls string store            % ; pixls => (....)",
    "    0 1 npixls -1 add {                   % counter ",
    "      pixls exch                          % pixls counter",
    "      rgbdata indx get .3 mul             % pixls counter .3*rgbdata(ind)",
    "      rgbdata indx 1 add get .59 mul add  % pixls counter .3*rgbdata(ind) + ",
    "					  %          .59*rgbdata(ind+1)",
    "      rgbdata indx 2 add get .11 mul add  % pixls counter .3*rgbdata(ind) + .59",
    "					  %  *rgbdata(ind+1)+.11*rgbdata(ind+2)",
    "      cvi                                 % pixls counter <grayscale value>",
    "      put                                 %",
    "      /indx indx 3 add store              % ; /ind => ind+3",
    "    } for                                 % repeat for each rgb value",
    "    pixls                                 % (pixls)",
    "  } bind def                              % ; /colortograyscale -> dictionary",
    "  ",
    "  % Utility procedure for colorimage operator.  This procedure takes two",
    "  % procedures off the stack and merges them into a single procedure.",
    "  ",
    "  /mergeprocs { %def      % {proc1} {proc2}",
    "    dup length            % {proc1} {proc2} N2",
    "    3 -1 roll             % {proc2} N2 {proc1}",
    "    dup                   % {proc2} N2 {proc1} {proc1}",
    "    length                % {proc2} N2 {proc1} N1",
    "    dup                   % {proc2} N2 {proc1} N1 N1",
    "    5 1 roll              % N1 {proc2} N2 {proc1} N1",
    "    3 -1 roll             % N1 {proc2} {proc1} N1 N2",
    "    add                   % N1 {proc2} {proc1} N1+N2",
    "    array cvx             % N1 {proc2} {proc1} { ... }",
    "    dup                   % N1 {proc2} {proc1} { ... } { ... }",
    "    3 -1 roll             % N1 {proc2} { ... } { ... } {proc1}",
    "    0 exch                % N1 {proc2} { ... } { ... } 0 {proc1}",
    "    putinterval           % N1 {proc2} { <<{proc1}>> ... }",
    "    dup                   % N1 {proc2} { <<{proc1}>> ... } { <<{proc1}>> ... }",
    "    4 2 roll              % { <<{proc1}>> ... } { <<{proc1}>> ... } N1 {proc2}",
    "    putinterval           % { <<{proc1}>> <<{proc2}>> }",
    "  } bind def              % ; /mergeprocs => dictionary",
    "",
    "  /colorimage { %def               % {imageproc} multiproc ncolors",
    "     pop                           % {imageproc} multiproc ; assume 3 colors",
    "     pop                           % {imageproc}           ; assume false",
    "     {colortograyscale}            % {imageproc} {colortograyscale}",
    "     mergeprocs                    % {imageproc colortograyscale}",
    "     image                         % construct monochrome image",
    "  } bind def                       % ; /colorimage => dictionary",
    "} if                               % only create if it doesn't already exist",
    0
};
