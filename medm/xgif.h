
/*
 * xgif.h
 *
 * definition of image-related data types for GIF images
 */

#ifndef __XGIF_H__
#define __XGIF_H__

typedef unsigned char Byte;

typedef struct {

	XImage	      *theImage;
	XImage	      *expImage;

	int           dispcells;
	Colormap      theCmap;
	GC            theGC;
	Pixel	      fcol,bcol;
	Font          mfont;
	XFontStruct   *mfinfo;
	Visual        *theVisual;

	int            iWIDE,iHIGH,eWIDE,eHIGH,numcols,strip,nostrip;
	unsigned long cols[256];
	XColor        defs[256];

	unsigned int	currentWidth;
	unsigned int	currentHeight;

} GIFData;



#endif  /* __XGIF_H__ */
