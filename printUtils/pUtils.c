/* utils.c - general system utility functions

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

   Portions of this program are also Copyright (c) 1989 Massachusetts
   Institute of Technology

   Permission to use, copy, modify, distribute, and sell this software
   and its documentation for any purpose is hereby granted without fee,
   provided that the above copyright notice appear in all copies and that
   both that copyright notice and this permission notice appear in
   supporting documentation, and that the name of M.I.T. not be
   used in advertising or publicity pertaining to distribution of the
   software without specific, written prior permission.  M.I.T. make no
   representations about the suitability of this software for any
   purpose.  It is provided "as is" without express or implied warranty.

   M.I.T. DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
   ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT
   SHALL M.I.T. BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL
   DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
   PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
   TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
   PERFORMANCE OF THIS SOFTWARE.
*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
/* WIN32 does not have unistd.h */
#include <io.h>         /* for read (usually in unistd.h) */
#else
/* Use unistd.h */
#include <unistd.h>
#endif

#if 0
/* KE: This is not used but kept around for imformation */
/* Function prototypes */
static void _swapbits(register unsigned char *b, register long n);

extern char progname[];
#endif

/*
 ** get_time_and_date() - return the time and date as strings
 *                          Written: 1-31-89 by Robert Tatar
 *                      Last update: 2-16-90 by R.C. Tatar
 * This routine returns the time and date as character strings in the
 * following example forms:
 *          mytime = "12:01:03"          (hh:mm:ss)
 *          mydate = "Jan 31, 1989"  (day mon dt, year)
 */

void get_time_and_date(char mytime[], char mydate[])
{
    time_t clock;
    char *ap;

    ap = (time (&clock), ctime (&clock));
    strncpy(mydate, ap+4, 6);
    mydate[6] = ',';
    strncpy(mydate+7, ap+19, 5);
    mydate[12] = '\0';

    strncpy(mytime, ap+11, 8);
    mytime[8] = '\0';
}

/*
 ** xwd2ps_swapshort() - swap the bytes in the next n shorts
 */
/* KE: This is a little different from the corresponding swap routine
   in xwd.c.  It isn't used but kept around in case. */
void xwd2ps_swapshort(register char *bp, register long n)
{
    register char c;
    register char *ep = bp + n;
    do {
	c = *bp;
	*bp = *(bp + 1);
	bp++;
	*bp = c;
	bp++;
    }  while (bp < ep);
}

/*
 ** xwd2ps_swaplong() - swap the bytes in the shorts, then swap the shorts
 */
/* KE: This is a little different from the corresponding swap routine
   in xwd.c */
void xwd2ps_swaplong(register char *bp, register long n)
{
    register char c;
    register char *ep = bp + n;
    register char *sp;
    do {
	sp = bp + 3;
	c = *sp;
	*sp = *bp;
	*bp++ = c;
	sp = bp + 1;
	c = *sp;
	*sp = *bp;
	*bp++ = c;
	bp += 2;
    } while (bp < ep);
}

/*
 ** xwd_usage() - print a help note
 * This subroutine
 * Written January 1989 by Robert Tatar.
 */

#if 0
/* KE: This is not used but kept around for imformation */
void xwd2ps_usage(void)
{
    fprintf(stderr,"\nusage: %s [options] XWD_raster_file_name\n", progname);
    fprintf(stderr,"    %s converts an XWD raster file to PostScript.  If\n",
      progname);
    fprintf(stderr,"    XWD_raster_file_name is absent, %s reads from standard input.\n", progname);
    fprintf(stderr,"    Spaces between options and arguments are ignored and can be left out.\n");
    fprintf(stderr,"\nThe following options are recognized:\n");
    fprintf(stderr,"OPTION ARGUMENT        EXPLANATION\n");
    fprintf(stderr,"  -P                   Portrait orientation (default if width <= height)\n");
    fprintf(stderr,"  -L                   Landscape orientation (default if width> height)\n");
    fprintf(stderr,"  -l                   GE logo just appended below lower left corner of image (see %s.doc file)\n"
      , progname);
    fprintf(stderr,"  -I                   Invert black to white (intended for black backgrounds)\n");
    fprintf(stderr,"  -m                   monochrome option, converts color into grayscale\n");
    fprintf(stderr,"  -s  string           title string to place at top of page\n");
    fprintf(stderr,"  -S  font_size        font size (points) of title string (default 16)\n");
    fprintf(stderr,"  -f  include_file     postscript command file to append to output file\n");
    fprintf(stderr,"  -c  copies           number of copies to print (default 1)\n");
    fprintf(stderr,"  -w  width            width of image in inches\n");
    fprintf(stderr,"  -h  height           height of image in inches\n");
    fprintf(stderr,"  -W  width_fraction   percent of horizontal raster used (between -1 & 1)\n");
    fprintf(stderr,"                       + values starts from left, - values starts from right\n");
    fprintf(stderr,"  -H  height_fraction  percent of vertical raster used (between -1 & 1)\n");
    fprintf(stderr,"                       + values starts from top, - values starts from bottom\n");
    fprintf(stderr,"  -d                   current date added to upper left of page\n");
    fprintf(stderr,"  -t                   current time added to upper right of page\n");
    fprintf(stderr,"  -b                   surrounds image with a thin, black border\n");
    fprintf(stderr,"  -p  paper_size       \"A\" or \"letter\" (default), \"B\", \"A3\" or \"A4\"\n");
    fprintf(stderr,"  -g  gamma            gamma factor; 0.5<gamma<1; 1=normal, 0. 5=brighter\n");
    fprintf(stderr,"\nusage examples:\n");
    fprintf(stderr,"  xwd | %s | lpr -Ppeacock\n\n", progname);
    fprintf(stderr,"  %s -lh8.0 viewgraph.xwd | lpr -Ppeacock\n\n", progname);
    fprintf(stderr,"  xwd | %s -ldts \"Gas Velocity\" -S 20 | lpr -Ppeacock\n\n", progname);
    fprintf(stderr,"  some_xwd_generator | %s -mw4 | lpr\n\n", progname);
    fprintf(stderr,"  %s -mbLw6 -W.445 -H.569 scrdump.xwd | lpr\n\n", progname);
    fprintf(stderr,"  %s -h6.2 -w4.5 -f border.ps brain.xwd > brain.eps\n\n", progname);
    exit(1);
}
#endif

#if 0
/* KE: This is not used but kept around in case */
static unsigned char _reverse_byte[0x100] = {
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};

static void _swapbits(register unsigned char *b, register long n)
{
    do {
	*b = _reverse_byte[*b];
	b++;
    } while (--n > 0);
}
#endif


/*
 *** fMax() - return the larger of two floating point numbers
 */
/* KE: This had no return type before and so was int by default.
   Changed it to float. */
float fMax(float a, float b)
{
    if(a >= b)
      return(a);
    else
      return(b);
}
