/**  A Set of utility algorithms for use by the graphX plot package
 **
 **  this set of algorithms come from various sources; each is more
 **  fully described in a paragram preceding the algorithm.....
 **
 **
 **/


#include <stdio.h>
#include <math.h>


/**
 **
 **
 ** Graphical plot scale calculation routines
 ** 
 ** from Communication of the ACM, October 1973, Volume 16 Number 10
 ** Original algorithms in FORTRAN
 **
 ** implemented in C by Mark Anderson
 **			Argonne National Laboratory
 **			13 Februrary 1990
 **
 **
 ** linear_scale == SCALE1 from that article
 ** log_scale == SCALE3
 **/


void linear_scale(xmin,xmax,n,xminp,xmaxp,dist)
    double xmin, xmax; int n;
    double *xminp, *xmaxp, *dist;
{

    static double vint[4] = { 1.0, 2.0, 5.0, 10.0, };
    static double sqr[3]  = { 1.414214, 3.162278, 7.071068, };
    double del = 0.0000002;		/* account for round-off errors */
    int nal, i, m1, m2;
    double fn, a, al, b, fm1, fm2;

  /* check whether proper input values were supplied */

    if ( ! (xmin <= xmax && n > 0)) {
	fprintf(stderr,"\nlinear_scale: improper input values supplied!");
	return;
    }

  /* provide 10% spread if graph is parallel to an axis */
    if (xmax == xmin) {
	xmax *= 1.05;
	xmin *= 0.95;
    }

    fn = n;

  /* find approximate interval size */

    a = (xmax - xmin)/ (double) fn;
    al = log10(a);
    nal = (int) al;

    if (a < 1.0) nal -= 1;

  /* a is scaled into variable named b betwen 1 and 10 */

    b = a / ((double) pow(10.0, (double) nal));
 
  /* the closest permissible value for b is found */

    for (i = 1; i < 4; i++) {
	if (b < sqr[i-1]) goto label30;
    }
    i = 4;

  /* the interval size is computed */

  label30:
    *dist = vint[i-1]*((double) pow(10.0, (double) nal));
    fm1 = xmin/(*dist);
    m1 = (int) fm1;
    if (fm1 < 0.0) m1 -= 1;
    if (fabs( ((double) m1) + 1.0 - ((double) fm1) ) < ((double) del)) 
      m1 += 1;

  /* the new min and max limits are found */

    *xminp = (*dist) * ((double) m1);
    fm2 = xmax/(*dist);
    m2 = ((int) fm2) + 1;
    if (fm2 < -1.0) m2 -= 1;
    if ( fabs(( ((double) fm2) + 1.0 - ((double) m2))) < del) 
      m2 -= 1;
    *xmaxp = (*dist) * ((double) m2);

  /* adjust limits to account for round-off if necessary */

    if ((double) (*xminp) > xmin) 
      *xminp = xmin;
    if ((double) (*xmaxp) < xmax) 
      *xmaxp = xmax;

}



void log_scale(xmin,xmax,n,xminp,xmaxp,dist)
    double xmin, xmax; int n;
    double *xminp, *xmaxp, *dist;
{

    static double vint[11] = 
    { 10.0, 9.0, 8.0, 7.0, 6.0, 5.0, 4.0, 3.0, 2.0, 1.0, 0.5,};

    double del = 0.0000002;		/* account for round-off errors */
    int nal, i, m1, m2, nx, np;
    double fn, xminl, xmaxl, distl, a, al, b, fm1, fm2;

  /* check whether proper input values were supplied */

    if ( ! (xmin <= xmax && n >= 1 && xmin > 0.0)) {
	fprintf(stderr,"\nlinear_scale: improper input values supplied!");
	return;
    }

  /* provide 10% spread if graph is parallel to an axis */
    if (xmax == xmin) {
	xmax *= 1.05;
	xmin *= 0.95;
    }

  /* translate values into logarithmic region */
    xminl = log10(xmin);
    xmaxl = log10(xmax);
    fn = n;

  /* find approximate interval size */

    a = (xmaxl - xminl)/ (double) fn;
    al = log10(a);
    nal = (int) al;

    if (a < 1.0) nal -= 1;

  /* a is scaled into variable named b betwen 1 and 10 */

    b = a / ((double) pow(10.0, (double) nal));
 
  /* the closest permissible value for b is found */

    for (i = 1; i < 9; i++) {
	if (b < (10.0/vint[i-1])+del) goto label30;
    }
    i = 10;

  /* the interval size is computed */

  label30:
    distl = (double) pow(10.0,(double) nal+1.0) / vint[i-1];
    fm1 = xminl/(distl);
    m1 = (int) fm1;
    if (fm1 < 0.0) m1 -= 1;
    if (fabs( ((double) m1) + 1.0 - ((double) fm1) ) < ((double) del)) 
      m1 += 1;

  /* the new min and max limits are found */

    *xminp = distl * ((double) m1);
    fm2 = xmaxl/distl;
    m2 = ((int) fm2) + 1;
    if (fm2 < -1.0) m2 -= 1;
    if ( fabs(( ((double) fm2) + 1.0 - ((double) m2))) < del) 
      m2 -= 1;
    *xmaxp = distl * ((double) m2);
    np = m2 - m1;

  /* check to see if another pass is necessary */
    if (np <= n) goto label40;
    i++;
    goto label30;

  label40:
    nx = (n-np)/2;
    *xminp = (*xminp) - (((double) nx) * distl);
    *xmaxp = (*xmaxp) + (((double) n) * distl);

  /* values are translated from the logarithmic into the linear region */
    *dist = (double) pow(10.0,(double)distl);
    *xminp = (double) pow(10.0,(double)*xminp);
    *xmaxp = (double) pow(10.0,(double)*xmaxp);


  /* adjust limits to account for round-off if necessary */

    if ((double) (*xminp) > xmin) 
      *xminp = xmin;
    if ((double) (*xmaxp) < xmax) 
      *xmaxp = xmax;

}









/*   rainbow(h, s, v, r, g, b)
     double h, s, v, *r, *g, *b;

 This routine computes colors suitable for use in color level plots.
 Typically s=v=1 and h varies from 0 (red) to 1 (blue) in
 equally spaced steps.  (h=.5 gives green; 1<h<1.5 gives magenta.)
 To convert for frame buffer, use   R = floor(255.999*pow(*r,1/gamma))  etc.
 To get tables calibrated for other devices or to report complaints,
 contact  Eric Grosse   research!ehg    201-582-5828.
*/

double huettab[] = {
    0.0000, 0.0062, 0.0130, 0.0202, 0.0280, 0.0365, 0.0457, 0.0559, 0.0671, 0.0796,
    0.0936, 0.1095, 0.1275, 0.1482, 0.1806, 0.2113, 0.2393, 0.2652, 0.2892, 0.3119,
    0.3333, 0.3556, 0.3815, 0.4129, 0.4526, 0.5060, 0.5296, 0.5501, 0.5679, 0.5834,
    0.5970, 0.6088, 0.6191, 0.6281, 0.6361, 0.6430, 0.6490, 0.6544, 0.6590, 0.6631,
    0.6667, 0.6713, 0.6763, 0.6815, 0.6873, 0.6937, 0.7009, 0.7092, 0.7190, 0.7308,
    0.7452, 0.7631, 0.7856, 0.8142, 0.8621, 0.9029, 0.9344, 0.9580, 0.9755, 0.9889,
    1.0000
};
/* computed from the FMC-1 color difference formula */
/* Barco monitor, max(r,g,b)=1, n=61 magenta,  2 Jan 1986 */

rainbow(h, s, v, r, g, b)
    double h, s, v, *r, *g, *b;
{
    int i;
    double modf(), trash;
    h = 60*modf(h/1.5,&trash);
    i = floor(h);
    h = huettab[i] + (huettab[i+1]-huettab[i])*(h-i);
    dhsv2rgb(h,s,v,r,g,b);
}


dhsv2rgb(h, s, v, r, g, b)    /*...hexcone model...*/
    double h, s, v, *r, *g, *b;    /* all variables in range [0,1[ */
  /* here, h=.667 gives blue, h=0 or 1 gives red. */
{  /* see Alvy Ray Smith, Color Gamut Transform Pairs, SIGGRAPH '78 */
    int i;
    double f, m, n, k;
    double modf(), trash;
    h = 6*modf(h,&trash);
    i = floor(h);
    f = h-i;
    m = (1-s);
    n = (1-s*f);
    k = (1-(s*(1-f)));
    switch(i){
    case 0: *r=1; *g=k; *b=m; break;
    case 1: *r=n; *g=1; *b=m; break;
    case 2: *r=m; *g=1; *b=k; break;
    case 3: *r=m; *g=n; *b=1; break;
    case 4: *r=k; *g=m; *b=1; break;
    case 5: *r=1; *g=m; *b=n; break;
    default: fprintf(stderr,"bad i: %f %d",h,i); exit(1);
    }
    f = *r;
    if( f < *g ) f = *g;
    if( f < *b ) f = *b;
    f = v / f;
    *r *= f;
    *g *= f;
    *b *= f;
}
