/* ps_utils.c - dump various PostScript structures to stdout

   Copyright (c) 1990 General Electric Company

   Permission to use, copy, modify, distribute, and sell this software
   and its documentation for any purpose is hereby granted without fee,
   provided that the above copyright notice appear in all copies and that
   both that copyright notice and this permission notice appear in
   supporting documentation, and that the name of General Electric
   Company not be used in advertising or publicity pertaining to
   distribution of the software without specific, written prior
   permission.  General Electric Company makes no representations about
   the suitability of this software for any purpose.  It is provided "as
   is" without express or implied warranty.

   General Electric Company DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS
   SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
   FITNESS, IN NO EVENT SHALL General Electric Company BE LIABLE FOR ANY
   SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
   RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
   CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
   CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdio.h>
#include <time.h>

#include "printUtils.h"
#include "my_logo.h"
#include "ps_color.h"

extern char progname[];

/*
 ** outputBorder() - put a border around the image
 */
void outputBorder(FILE *fo, Image the_image)
{
    fprintf(fo,"\nnewpath\n");
    fprintf(fo,"0 0 moveto\n");
    if(the_image.orientation == PORTRAIT) {
	fprintf(fo,"%f inch 0 rlineto\n", the_image.width);
	fprintf(fo,"0 %f inch rlineto\n", the_image.height);
	fprintf(fo,"-%f inch 0 rlineto\n", the_image.width);
    }
    else {
	fprintf(fo,"%f inch 0 rlineto\n", the_image.height);
	fprintf(fo,"0 %f inch rlineto\n", the_image.width);
	fprintf(fo,"-%f inch 0 rlineto\n", the_image.height);
    }
    fprintf(fo,"closepath\n");
    fprintf(fo,"stroke\n");
}

/*
 ** outputDate() - output the date
 */
void outputDate(FILE *fo, Image the_image)
{
    float xpos, ypos;
    char mytime[20];           /* character representation of current time */
    char mydate[40];           /* character representation of current date */

    get_time_and_date(mytime, mydate);

    fprintf(fo,"\nmatrix currentmatrix\n");
    fprintf(fo,"newpath\n");

    if(the_image.orientation == PORTRAIT) {
	xpos = 0;
	ypos = the_image.height;
	fprintf(fo,"%f inch %f inch moveto %% upper left of image\n", xpos, ypos);
    }
    else {
	xpos = 0;
	ypos = 0;
	fprintf(fo,"%f inch %f inch moveto  %% upper left of image\n", xpos, ypos);
	fprintf(fo,"90 rotate\n");
    }

    fprintf(fo,"/Times-Roman findfont\n");
    fprintf(fo,"10 scalefont\n");
    fprintf(fo,"setfont \n");
    fprintf(fo,"0 .12 inch rmoveto\n");
    fprintf(fo,"(%s) show\n",mydate);
    fprintf(fo,"stroke\n");
    fprintf(fo,"setmatrix\n\n");
}

/*
 ** outputSentence() - put a label in the diagram
 */
void outputTitle(FILE *fo, Image the_image, Options the_options)
{
    float xpos, ypos;

    fprintf(fo,"\nmatrix currentmatrix\n");
    fprintf(fo,"newpath\n");

    if(the_image.orientation == PORTRAIT) {
	xpos = the_image.width/2;
	ypos = the_image.height;
	fprintf(fo,"%f inch %f inch moveto %% upper center of image\n", xpos, ypos);
    }
    else {
	xpos = 0;
	ypos = the_image.width/2;
	fprintf(fo,"%f inch %f inch moveto  %% upper center of image\n", xpos, ypos);
	fprintf(fo,"90 rotate\n");
    }

    fprintf(fo,"/Times-Roman findfont\n");
    fprintf(fo,"%d scalefont\n",the_options.title.font_size);
    fprintf(fo,"setfont \n");
    fprintf(fo,"(%s) stringwidth\n",the_options.title.string);
    fprintf(fo,"pop -2 div .12 inch rmoveto\n");
    fprintf(fo,"(%s) show\n",the_options.title.string);
    fprintf(fo,"stroke\n");
    fprintf(fo,"setmatrix\n\n");
}


/*
 ** ouputTime() - put the time in the output
 */

void outputTime(FILE *fo, Image the_image)
{
    char mytime[20];           /* character representation of current time */
    char mydate[40];           /* character representation of current date */
    float x, y;

    get_time_and_date(mytime, mydate);

    fprintf(fo,"\nmatrix currentmatrix\n");
    fprintf(fo,"newpath\n");

    if(the_image.orientation == PORTRAIT) {
	x = the_image.width;
	y = the_image.height;
	fprintf(fo,"%f inch %f inch moveto %% upper right of image\n", x, y);
    }
    else {
	x = 0;
	y = the_image.width;
	fprintf(fo,"%f inch %f inch moveto  %% upper right of image\n", x, y);
	fprintf(fo,"90 rotate\n");
    }

    fprintf(fo,"/Times-Roman findfont\n");
    fprintf(fo,"10 scalefont\n");
    fprintf(fo,"setfont \n");
    fprintf(fo,"(%s) stringwidth\n",mytime);
    fprintf(fo,"pop -1 mul .12 inch rmoveto\n");
    fprintf(fo,"(%s) show\n",mytime);
    fprintf(fo,"stroke\n");
    fprintf(fo,"setmatrix\n\n");
}

/*
 ** outputColorImage() - output the code to support color
 **                      image printing on monochrome devices
 */
void outputColorImage(FILE *fo)
{
    printPS(fo, ColorImage);
}


/*
 ** outputLogo() - print out your logo
 */
void outputLogo(FILE *fo, Image the_image)
{
    float xpos, ypos;

  /* KE: Was missing first argument */
    printPS(fo,my_logo);  /* print out the logo */

  /*
   * set up the scaling factors
   */
    fprintf(fo,"\nmatrix currentmatrix\n");
    fprintf(fo,"newpath\n");
    fprintf(fo,"0 0 moveto\n");

    if(the_image.orientation == PORTRAIT) {
	xpos = 0;
	ypos = - LOGOHEIGHT;
	fprintf(fo,"%f inch %f inch translate\n", xpos,ypos);
    }
    else {
	xpos = the_image.height + (float)LOGOHEIGHT;
	ypos = 0;
	fprintf(fo,"%f inch %f inch translate\n", xpos,ypos);
	fprintf(fo,"90 rotate\n");
    }

  /*
   * output the company name
   */
    fprintf(fo,"/xlinepos 0 def\n");
    fprintf(fo,"/ylinepos 0 def\n");
    fprintf(fo,"/charheight .11 inch def\n");
    fprintf(fo,"/logoheight %f inch def\n",LOGOHEIGHT);
    fprintf(fo,"/xlogopos xlinepos def\n");
    fprintf(fo,"/ylogopos ylinepos logoheight 0.2 mul add def\n");
    fprintf(fo,"/logosize logoheight 0.6 mul def\n");
    fprintf(fo,"/xtextpos xlinepos logosize 2 charheight mul add add def\n");
    fprintf(fo,"/ytextpos ylogopos charheight add def\n");
    fprintf(fo,"/nextline {\n   /ytextpos ytextpos charheight sub store\n");
    fprintf(fo,"   xtextpos ytextpos moveto\n} def\n");
    fprintf(fo,"/Helvetica-BoldOblique findfont\n");
    fprintf(fo,"8 scalefont\n");
    fprintf(fo,"setfont \n");
    fprintf(fo,"xlogopos ylogopos logosize 1 0 %s \n", LOGONAME);
    fprintf(fo,"xtextpos ytextpos moveto  %% locate lower left corner of image\n");
    fprintf(fo,"(%s) show\n", COMPANYNAME1);
    fprintf(fo,"nextline\n");
    fprintf(fo,"xtextpos ytextpos moveto\n");
    fprintf(fo,"(%s) show\n", COMPANYNAME2);
    fprintf(fo,"1 0 0 setrgbcolor         %% draw laser line\n");
    fprintf(fo,"newpath\n");
    fprintf(fo,"xlogopos ylinepos moveto\n");
    fprintf(fo,"1.2 inch 0 rlineto\n");
    fprintf(fo,"stroke\n");
    fprintf(fo,"setmatrix\n\n");
}


/*
 * print_ps() - output the PostScript String
 *
 * This subroutine prints out PostScript code structures
 *
 * Written March 1990 by Craig A. McGowan
 */
void printPS(FILE *fo, char **p)
{
    while (*p)
      fprintf(fo,"%s\n",*p++);
}


/*
 *** printEPSF() - print out the Encapsulated PS header
 *
 */
void printEPSF(FILE *fo, Image image, Page  page, char  *file_name)
{
    time_t clock;

    fprintf(fo,"%%!PS-Adobe-2.0 EPSF-1.2\n");    /* standard PS header */
    if(image.orientation == PORTRAIT)
      fprintf(fo,"%%%%BoundingBox: %f %f %f %f\n",
	72*page.ximagepos - 1, 72*page.yimagepos - 1,
	72*(page.ximagepos+image.width) + 1,
	72*(page.yimagepos+image.height-page.yoffset) + 1);
    else
      fprintf(fo,"%%%%BoundingBox: %f %f %f %f\n",
	72*page.ximagepos - 1, 72*page.yimagepos - 1,
	72*(page.ximagepos+image.height) + 1,
	72*(page.yimagepos+image.width-page.yoffset) + 1);

    fprintf(fo,"%%%%Creator: %s\n", progname);
    fprintf(fo,"%%%%CreationDate: %s", (time (&clock), ctime (&clock)));
    fprintf(fo,"%%%%Title: %s\n", file_name);
    fprintf(fo,"%%%%EndComments\n");
}
