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
/*****************************************************************************
 *
 * xgif.c - displays GIF pictures on an X11 display
 *
 *  Original Author: John Bradley, University of Pennsylvania
 *                   (bradley@cis.upenn.edu)
 *
 *	MDA - editorial comment:
 *		MAJOR MODIFICATIONS to make this sensible.
 *		be careful about running on non-32-bit machines
 *
 *****************************************************************************
*/

#define DEBUG_GIF 0
#define DEBUG_DISPOSAL 0
#define DEBUG_DISPOSAL_FILE 0
#define DEBUG_CODES 0
#define DEBUG_OPEN 0
#define DEBUG_EXODUS 0
#define DEBUG_BYTEORDER 0

/* include files */
#include <string.h>
#include "medm.h"
#include "xgif.h"
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define MAXEXPAND 16

/* Function prototypes */

/* KE: The following is the prototype for _XGetBitsPerPixel, which is a
 *   convenience function found in the MIT distribution in
 *   /opt/local/share/src/R5/mit/lib/X/XImUtil.c (at the APS)
 */
int _XGetBitsPerPixel(Display *dpy, int depth);

static int countImages(void);
static Boolean loadGIF(DisplayInfo *displayInfo, DlImage *dlImage);
static Boolean parseGIFImage(DisplayInfo *displayInfo, DlImage *dlImage);
static Boolean parseGIFExtension(void);
static void addToPixel(GIFData *gif, Byte Index);
#if 0
static void dumpGIF(GIFData *gif);
static int getClientByteOrder();
#endif
static int getCorrectedByteOrder();
static int readCode(void);

#define NEXTBYTE (*ptr++)
#define IMAGESEP 0x2c
#define TRAILER 0x3b
#define EXTENSION 0x21
#define GRAPHCONTROL 0xf9
#define PLAINTEXT 0x01
#define APPLICATION 0xff
#define COMMENT 0xfe
#define MASK0   0x01
#define MASK210 0x07
#define MASK432 0x1c
#define MASK3   0x08
#define MASK6   0x40
#define MASK7   0x80

FILE *fp;

int BitOffset;                    /* Bit Offset of next code */
int XC, YC;                       /* Output X and Y coords of current pixel */
int ImageDataSize;                /* The size of the XImage data */
int Pass;                         /* Used by output routine if interlaced pic */
int OutCount;                     /* Decompressor output 'stack count' */
int LogicalScreenWidth;           /* Logical screen width */
int LogicalScreenHeight;          /* Logical screen height */
Boolean GlobalColorTableFlag;     /* Flag indicating global color table follows */
Boolean GlobalColorTableSortFlag; /* Flag indicating global color table is sorted */
int SizeOfGlobalColorTable;       /* Size of Global Color Table from header */
int BitsPerPixel;                 /* Bits per pixel (SizeOfGlobalColorTable = 1) */
int Width, Height;                /* Image dimensions */
int LeftOffset, TopOffset;        /* Image offsets from logical screen */
int DisposalMethod;               /* Image disposal method */
int BytesPerScanline;             /* Bytes per scanline in output raster */
int ColorTableEntries;            /* Number of colors in global color table */
int BackgroundColorIndex;         /* BackgroundColorIndex color */
int PixelAspectRatio;             /* Pixel aspect ratio */
int CodeSize;                     /* Code size, read from GIF header */
int InitCodeSize;                 /* Starting code size, used during Clear */
int Code;                         /* Value returned by readCode */
int MaxCode;                      /* Limiting value for current code size */
int ClearCode;                    /* GIF clear code */
int EOFCode;                      /* GIF end-of-information code */
int CurCode, OldCode, InCode;     /* Decompressor variables */
int FirstFree;                    /* First free code, generated per GIF spec */
int FreeCode;                     /* Decompressor, next free slot in hash table */
int FinChar;                      /* Decompressor variable */
int BitMask;                      /* AND mask for data size */
int ReadMask;                     /* Code AND mask for current code size */
int BytesOffsetPerPixel;          /* Bytes offset per pixel */
int ScreenDepth;                  /* Bits per Pixel */
int LocalColorTableFlag;          /* Flag denoting a local color table */
int SizeOfLocalColorTable;        /* Size of a local color table */
int LocalColorTableEntries;       /* Number of colors in local color table */
Byte TransparentColorFlag;        /* Use Transparent color for the frame */
Byte TransparentIndex;            /* Transparent color index for the frame */
int DelayTime;                    /* Delay time in 1/100 sec for the frame */
Boolean Interlace;

#if DEBUG_GIF
Boolean verbose=True;
#else
Boolean verbose=False;
#endif

Byte *Image=NULL;   /* The result array */
Byte *RawGIF=NULL;  /* The heap array to hold it, raw */
Byte *Raster=NULL;  /* The raster data stream, unblocked */
Byte *ptr=NULL;
GIFData *gif=NULL;
char *fname=NULL;
int filesize=0;

/* The hash table used by the decompressor */
int Prefix[4096];
int Suffix[4096];

/* An output array used by the decompressor */
int OutCode[1025];

/* The color map, read from the GIF header */
Byte Red[256], Green[256], Blue[256];

/* The pixel for unallocated colors */
static Pixel blackPixel;
static XColor black;

/* Function to initialize for GIF processing */
Boolean initializeGIF(DisplayInfo *displayInfo, DlImage *dlImage)
{
    int x, y;
    unsigned int w, h;
    Boolean success;
    int first=1;

    if(first) {
      /* Define blackPixel for a fallback.  (It will lead to errors
         when it is freed) */
	blackPixel=BlackPixel(display,screenNum);
      /* Define a black color for when other colors can't be found */
	black.red=black.green=black.blue=0;
	black.flags=DoRed | DoGreen | DoBlue;
	first=0;
    }

    x=dlImage->object.x;
    y=dlImage->object.y;
    w=dlImage->object.width;
    h=dlImage->object.height;

  /* Free any existing GIF resources */
    freeGIF(dlImage);

  /* Check for a valid filename */
    if(!*dlImage->imageName) {
	medmPrintf(1,"\ninitializeGIF: Filename is empty\n");
	return(False);
    }

  /* Allocate the GIFData */
    if(!(gif=(GIFData *)malloc(sizeof(GIFData)))) {
	medmPrintf(1,"\ninitializeGIF: Memory allocation error\n");
	return(False);
    }
    dlImage->privateData=gif;

  /* (MDA) programmatically set nostrip to false - see what happens */
    gif->nostrip=False;
    gif->strip=0;

    gif->fcol=displayInfo->colormap[displayInfo->drawingAreaForegroundColor];
    gif->bcol=displayInfo->colormap[displayInfo->drawingAreaBackgroundColor];
#if DEBUG_GIF
    print("initializeGIF: fcol=%3d bcol=%3d\n",gif->fcol,gif->bcol);
#endif
    gif->mfont= 0;
    gif->mfinfo=NULL;
    gif->theCmap=cmap;	/* MDA - used to be DefaultColormap() */
    gif->theGC=DefaultGC(display,screenNum);
    gif->theVisual=DefaultVisual(display,screenNum);
    gif->numcols=0;
    gif->frames=NULL;
    *gif->imageName='\0';

    gif->displayCells=DisplayCells(display,screenNum);
#if DEBUG_EXODUS
    print("initializeGIF: DisplayCells=%d\n",gif->displayCells);
#endif

#if 0
    if(gif->displayCells < 255) {
	medmPrintf(1,"\ninitializeGIF: Need at least 256 color cells, got %d\n",
	  gif->displayCells);
	freeGIF(dlImage);
	return(False);
    }
#endif

    gif->nFrames=0;
    gif->curFrame=0;
    strcpy(gif->imageName,dlImage->imageName);

  /* Open and read the file  */
    success=loadGIF(displayInfo,dlImage);
    if(!success) {
	return(False);
    }

    gif->iWIDE=LogicalScreenWidth;
    gif->iHIGH=LogicalScreenHeight;

    gif->eWIDE=gif->iWIDE;
    gif->eHIGH=gif->iHIGH;

    resizeGIF(dlImage);

    return(True);

}

void drawGIF(DisplayInfo *displayInfo, DlImage *dlImage, Drawable drawable)
{
    GIFData *gif;
    int x, y;
    unsigned int w, h;

    gif=(GIFData *)dlImage->privateData;

#if DEBUG_DISPOSAL
    print("drawGIF: curFrame=%d expImage=%x %s\n",
      gif?gif->curFrame:0,
      gif?gif->frames[gif->curFrame]->expImage:0,
      gif?"":" (No gif)");
#endif

    if(gif->frames && gif->frames[CURFRAME(gif)] && CUREXPIMAGE(gif) != NULL) {
	x=dlImage->object.x;
	y=dlImage->object.y;
      /* KE: Should be the same as gif->eWIDE, gif->eHIGH */
	w=dlImage->object.width;
	h=dlImage->object.height;

#if DEBUG_DISPOSAL
	{
	    char title[80];
	    Pixmap debugPixmap;

	    if(gif->curFrame == 5 && CURIMAGE(gif)) {
		print("drawGIF: G theImage=%x iWIDE=%d iHIGH=%d\n",
		  CURIMAGE(gif),gif->iWIDE,gif->iHIGH);
		print("  bitmap_bit_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CURIMAGE(gif)->bitmap_bit_order,LSBFirst,MSBFirst);
		print("  bitmap_pad=%d\n",
		  CURIMAGE(gif)->bitmap_pad);
		print("  bitmap_unit=%d\n",
		  CURIMAGE(gif)->bitmap_unit);
		print("  byte_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CURIMAGE(gif)->byte_order,LSBFirst,MSBFirst);
		print("  bits_per_pixel=%d\n",
		  CURIMAGE(gif)->bits_per_pixel);
		debugPixmap=XCreatePixmap(display,RootWindow(display,screenNum),
		  gif->iWIDE,gif->iHIGH,XDefaultDepth(display,screenNum));
		XPutImage(display,debugPixmap,
		  gif->theGC,CURIMAGE(gif),
		  0,0,0,0,gif->iWIDE,gif->iHIGH);
		sprintf(title,"%dg Frame",gif->curFrame);
		dumpPixmap(debugPixmap,(Dimension)gif->iWIDE,
		  (Dimension)gif->iHIGH,title);
		XFreePixmap(display,debugPixmap);
	    }
	}
#endif
#if DEBUG_DISPOSAL
	{
	    char title[80];
	    Pixmap debugPixmap;

	    if(gif->curFrame == 5 && CUREXPIMAGE(gif)) {
#if DEBUG_DISPOSAL_FILE
		int w=gif->eWIDE;
		int h=gif->eHIGH;
		int i,j;
		Pixel pixel;
		FILE *file;

		file=fopen("dump.h.dat","w");
		fprintf(file,"Dumping H: expImage=%x width=%d height=%d\n",
		  CUREXPIMAGE(gif),w,h);

		fprintf(file,"  bitmap_bit_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CUREXPIMAGE(gif)->bitmap_bit_order,LSBFirst,MSBFirst);
		fprintf(file,"  bitmap_pad=%d\n",
		  CUREXPIMAGE(gif)->bitmap_pad);
		fprintf(file,"  bitmap_unit=%d\n",
		  CUREXPIMAGE(gif)->bitmap_unit);
		fprintf(file,"  byte_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CUREXPIMAGE(gif)->byte_order,LSBFirst,MSBFirst);
		fprintf(file,"  bits_per_pixel=%d\n",
		  CUREXPIMAGE(gif)->bits_per_pixel);

		for(i=0; i < w; i++) {
		    for(j=0; j < h; j++) {
			pixel=XGetPixel(CUREXPIMAGE(gif),i,j);
			fprintf(file,"%3d %3d %x\n",i,j,pixel);
		    }
		    fprintf(file,"\n");
		}
		fprintf(file,"\n");
		fclose(file);
		print("dump.h.dat finished\n");
#endif
		print("drawGIF: H expImage=%x eWIDE=%d eHIGH=%d\n",
		  CUREXPIMAGE(gif),gif->eWIDE,gif->eHIGH);
		print("  bitmap_bit_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CUREXPIMAGE(gif)->bitmap_bit_order,LSBFirst,MSBFirst);
		print("  bitmap_pad=%d\n",
		  CUREXPIMAGE(gif)->bitmap_pad);
		print("  bitmap_unit=%d\n",
		  CUREXPIMAGE(gif)->bitmap_unit);
		print("  byte_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CUREXPIMAGE(gif)->byte_order,LSBFirst,MSBFirst);
		print("  bits_per_pixel=%d\n",
		  CUREXPIMAGE(gif)->bits_per_pixel);
		debugPixmap=XCreatePixmap(display,RootWindow(display,screenNum),
		  gif->eWIDE,gif->eHIGH,XDefaultDepth(display,screenNum));
		XPutImage(display,debugPixmap,
		  gif->theGC,CUREXPIMAGE(gif),
		  0,0,0,0,gif->eWIDE,gif->eHIGH);
		sprintf(title,"%dh EXP Frame",gif->curFrame);
		dumpPixmap(debugPixmap,(Dimension)gif->eWIDE,
		  (Dimension)gif->eHIGH,title);
		XFreePixmap(display,debugPixmap);
	    }
	}
#endif
      /* Draw  */
#if 0
      /* Use internal GC */
	XPutImage(display,drawable,gif->theGC,CUREXPIMAGE(gif),
	  0,0,x,y,w,h);
#else
      /* Use the displayInfo GC so it will clip properly */
	XPutImage(display,drawable,displayInfo->gc,CUREXPIMAGE(gif),
	  0,0,x,y,w,h);
#endif
    }
}

void resizeGIF(DlImage *dlImage)
{
    GIFData *gif;
    unsigned int w,h;
    int i;

  /* warning:  this code'll only run machines where int=32-bits */

#if DEBUG_DISPOSAL
    print("resizeGIF\n");
#endif

    gif=(GIFData *)dlImage->privateData;
    w=dlImage->object.width;
    h=dlImage->object.height;

  /* simply return if no GIF image attached */
    if(!gif || !gif->frames) return;

    gif->currentWidth=w;
    gif->currentHeight= h;

  /* Loop over frames */
    for(i=0; i < gif->nFrames; i++) {
	gif->curFrame=i;
#if DEBUG_DISPOSAL
	{
	    char title[80];
	    Pixmap debugPixmap;

	    if(i == 5 && CURIMAGE(gif)) {
		print("resizeGIF: C theImage=%x\n",CURIMAGE(gif));
		print("  bitmap_bit_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CURIMAGE(gif)->bitmap_bit_order,LSBFirst,MSBFirst);
		print("  bitmap_pad=%d\n",
		  CURIMAGE(gif)->bitmap_pad);
		print("  bitmap_unit=%d\n",
		  CURIMAGE(gif)->bitmap_unit);
		print("  byte_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CURIMAGE(gif)->byte_order,LSBFirst,MSBFirst);
		print("  bits_per_pixel=%d\n",
		  CURIMAGE(gif)->bits_per_pixel);
		debugPixmap=XCreatePixmap(display,RootWindow(display,screenNum),
		  gif->iWIDE,gif->iHIGH,XDefaultDepth(display,screenNum));
		XPutImage(display,debugPixmap,
		  gif->theGC,CURIMAGE(gif),
		  0,0,0,0,gif->iWIDE,gif->iHIGH);
		sprintf(title,"%dc Frame",i);
		dumpPixmap(debugPixmap,(Dimension)LogicalScreenWidth,
		  (Dimension)LogicalScreenHeight,title);
		XFreePixmap(display,debugPixmap);
	    }
	}
#endif
#if DEBUG_DISPOSAL
	{
	    char title[80];
	    Pixmap debugPixmap;

	    if(i == 5 && CUREXPIMAGE(gif)) {
		print("resizeGIF: D expImage=%x\n",CUREXPIMAGE(gif));
		print("  bitmap_bit_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CUREXPIMAGE(gif)->bitmap_bit_order,LSBFirst,MSBFirst);
		print("  bitmap_pad=%d\n",
		  CUREXPIMAGE(gif)->bitmap_pad);
		print("  bitmap_unit=%d\n",
		  CUREXPIMAGE(gif)->bitmap_unit);
		print("  byte_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CUREXPIMAGE(gif)->byte_order,LSBFirst,MSBFirst);
		print("  bits_per_pixel=%d\n",
		  CUREXPIMAGE(gif)->bits_per_pixel);
		debugPixmap=XCreatePixmap(display,RootWindow(display,screenNum),
		  gif->eWIDE,gif->eHIGH,XDefaultDepth(display,screenNum));
		XPutImage(display,debugPixmap,
		  gif->theGC,CUREXPIMAGE(gif),
		  0,0,0,0,gif->eWIDE,gif->eHIGH);
		sprintf(title,"%dd EXP Frame",i);
		dumpPixmap(debugPixmap,(Dimension)LogicalScreenWidth,
		  (Dimension)LogicalScreenHeight,title);
		XFreePixmap(display,debugPixmap);
	    }
	}
#endif
	if((int)w == gif->iWIDE && (int)h == gif->iHIGH) {
	  /* Very special case (size=actual GIF size) */
	    if(CUREXPIMAGE(gif) != CURIMAGE(gif)) {
		if(CUREXPIMAGE(gif) != NULL) {
		  /* Free the memory we allocated to insure it is done
		   * with the same routines that allocated it.  Can be
		   * a problem on WIN32 if X frees it. */
		    XImage *image=(XImage *)(CUREXPIMAGE(gif));
		    if(image->data) free(image->data);
		    image->data=NULL;
		  /* Then destroy the image */
		    XDestroyImage(image);
		}
		CUREXPIMAGE(gif)=CURIMAGE(gif);
		gif->eWIDE=gif->iWIDE;
		gif->eHIGH=gif->iHIGH;
	    }
	} else {
	  /* Resize */
	    int sx, sy, dx, dy, count;
	    int sh, dh, sw, dw;
	    Byte *ximag,*ilptr,*ipptr,*elptr,*epptr;
	    int bytesPerPixel;

	  /* First, kill the old CUREXPIMAGE(gif), if one exists */
	    if(CUREXPIMAGE(gif) && CUREXPIMAGE(gif) != CURIMAGE(gif)) {
	      /* Free the memory we allocated to insure it is done
	       * with the same routines that allocated it.  Can be
	       * a problem on WIN32 if X frees it. */
		XImage *image=(XImage *)(CUREXPIMAGE(gif));
		if(image->data) free(image->data);
		image->data=NULL;
	      /* Then destroy the image */
		XDestroyImage(image);
	    }

	  /* Create CUREXPIMAGE(gif) of the appropriate size */
	    gif->eWIDE=w;  gif->eHIGH=h;
	    bytesPerPixel=CURIMAGE(gif)->bits_per_pixel/8;
	    ximag=(Byte *)malloc(w*h*bytesPerPixel);
	    CUREXPIMAGE(gif)=XCreateImage(display,gif->theVisual,
	      DefaultDepth(display,screenNum),ZPixmap,
	      0,(char *)ximag,gif->eWIDE,gif->eHIGH,8,0);

	    if(!ximag || !CUREXPIMAGE(gif)) {
		medmPrintf(1,"\nresizeGIF: Unable to create a %dx%d image\n",
		  w,h);
		medmPrintf(0,"  %d-bit: ximag=%08x, expImage=%08x\n",
		  DefaultDepth(display,screenNum),ximag,CUREXPIMAGE(gif));
		return;
	    }
	  /* Make the byte order be the same as that for theImage */
	    CUREXPIMAGE(gif)->byte_order=CURIMAGE(gif)->byte_order;

	  /* Copy and scale the bytes */
	    sw=CURIMAGE(gif)->width;
	    sh=CURIMAGE(gif)->height;
	    dw=CUREXPIMAGE(gif)->width;
	    dh=CUREXPIMAGE(gif)->height;
	    elptr=epptr=(Byte *)CUREXPIMAGE(gif)->data;
	    for(dy=0; dy < dh; dy++) {
		sy=(sh * dy)/dh;
		epptr=elptr;
		ilptr=(Byte *)CURIMAGE(gif)->data +
		  (sy * CURIMAGE(gif)->bytes_per_line);
		for(dx=0; dx < dw;  dx++) {
		    sx=(sw * dx)/dw;
		    ipptr=ilptr + sx*bytesPerPixel;
		  /* Copy bytesPerPixel bytes */
		    for(count=0; count < bytesPerPixel; count++) {
			*epptr++=*ipptr++;
		    }
		}
		elptr += CUREXPIMAGE(gif)->bytes_per_line;
	    }
	}
#if DEBUG_DISPOSAL
	{
	    char title[80];
	    Pixmap debugPixmap;

	    if(i == 5 && CURIMAGE(gif)) {
		print("resizeGIF: E theImage=%x\n",CURIMAGE(gif));
		print("  bitmap_bit_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CURIMAGE(gif)->bitmap_bit_order,LSBFirst,MSBFirst);
		print("  bitmap_pad=%d\n",
		  CURIMAGE(gif)->bitmap_pad);
		print("  bitmap_unit=%d\n",
		  CURIMAGE(gif)->bitmap_unit);
		print("  byte_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CURIMAGE(gif)->byte_order,LSBFirst,MSBFirst);
		print("  bits_per_pixel=%d\n",
		  CURIMAGE(gif)->bits_per_pixel);
		debugPixmap=XCreatePixmap(display,RootWindow(display,screenNum),
		  gif->iWIDE,gif->iHIGH,XDefaultDepth(display,screenNum));
		XPutImage(display,debugPixmap,
		  gif->theGC,CURIMAGE(gif),
		  0,0,0,0,gif->iWIDE,gif->iHIGH);
		sprintf(title,"%de Frame",i);
		dumpPixmap(debugPixmap,(Dimension)gif->iWIDE,
		  (Dimension)gif->iHIGH,title);
		XFreePixmap(display,debugPixmap);
	    }
	}
#endif
#if DEBUG_DISPOSAL
	{
char title[80];
	    Pixmap debugPixmap;

	    if(i == 5 && CUREXPIMAGE(gif)) {
#if DEBUG_DISPOSAL_FILE
		int w=gif->eWIDE;
		int h=gif->eHIGH;
		int i,j;
		Pixel pixel;
		FILE *file;

		file=fopen("dump.f.dat","w");
		fprintf(file,"Dumping F: expImage=%x\n",CUREXPIMAGE(gif));

		fprintf(file,"  bitmap_bit_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CUREXPIMAGE(gif)->bitmap_bit_order,LSBFirst,MSBFirst);
		fprintf(file,"  bitmap_pad=%d\n",
		  CUREXPIMAGE(gif)->bitmap_pad);
		fprintf(file,"  bitmap_unit=%d\n",
		  CUREXPIMAGE(gif)->bitmap_unit);
		fprintf(file,"  byte_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CUREXPIMAGE(gif)->byte_order,LSBFirst,MSBFirst);
		fprintf(file,"  bits_per_pixel=%d\n",
		  CUREXPIMAGE(gif)->bits_per_pixel);

		for(i=0; i < w; i++) {
		    for(j=0; j < h; j++) {
			pixel=XGetPixel(CUREXPIMAGE(gif),i,j);
			fprintf(file,"%3d %3d %x\n",i,j,pixel);
		    }
		    fprintf(file,"\n");
		}
		fprintf(file,"\n");
		fclose(file);
		print("dump.f.dat finished\n");
#endif
		print("resizeGIF: F expImage=%x eWIDE=%d eHIGH=%d\n",
		  CUREXPIMAGE(gif),w,h);
		print("  bitmap_bit_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CUREXPIMAGE(gif)->bitmap_bit_order,LSBFirst,MSBFirst);
		print("  bitmap_pad=%d\n",
		  CUREXPIMAGE(gif)->bitmap_pad);
		print("  bitmap_unit=%d\n",
		  CUREXPIMAGE(gif)->bitmap_unit);
		print("  byte_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CUREXPIMAGE(gif)->byte_order,LSBFirst,MSBFirst);
		print("  bits_per_pixel=%d\n",
		  CUREXPIMAGE(gif)->bits_per_pixel);
		debugPixmap=XCreatePixmap(display,RootWindow(display,screenNum),
		  gif->eWIDE,gif->eHIGH,XDefaultDepth(display,screenNum));
		XPutImage(display,debugPixmap,
		  gif->theGC,CUREXPIMAGE(gif),
		  0,0,0,0,gif->eWIDE,gif->eHIGH);
		sprintf(title,"%df EXP Frame",i);
		dumpPixmap(debugPixmap,(Dimension)gif->eWIDE,
		  (Dimension)gif->eHIGH,title);
		XFreePixmap(display,debugPixmap);
	    }
	}
#endif
#if DEBUG_DISPOSAL > 10
      /* KE: Repeats the block above */
	{
char title[80];
	    Pixmap debugPixmap;

	    if(i == 5 && CUREXPIMAGE(gif)) {
		print("resizeGIF: F1 expImage=%x eWIDE=%d eHIGH=%d\n",
		  CUREXPIMAGE(gif),w,h);
		print("  bitmap_bit_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CUREXPIMAGE(gif)->bitmap_bit_order,LSBFirst,MSBFirst);
		print("  bitmap_pad=%d\n",
		  CUREXPIMAGE(gif)->bitmap_pad);
		print("  bitmap_unit=%d\n",
		  CUREXPIMAGE(gif)->bitmap_unit);
		print("  byte_order=%d [LSBFirst=%d MSBFirst=%d]\n",
		  CUREXPIMAGE(gif)->byte_order,LSBFirst,MSBFirst);
		print("  bits_per_pixel=%d\n",
		  CUREXPIMAGE(gif)->bits_per_pixel);
		debugPixmap=XCreatePixmap(display,RootWindow(display,screenNum),
		  gif->eWIDE,gif->eHIGH,XDefaultDepth(display,screenNum));
		XPutImage(display,debugPixmap,
		  gif->theGC,CUREXPIMAGE(gif),
		  0,0,0,0,gif->eWIDE,gif->eHIGH);
		sprintf(title,"%df1 EXP Frame",i);
		dumpPixmap(debugPixmap,(Dimension)gif->eWIDE,
		  (Dimension)gif->eHIGH,title);
		XFreePixmap(display,debugPixmap);
	    }
	}
#endif
    }
}

static Boolean loadGIF(DisplayInfo *displayInfo, DlImage *dlImage)
{
    GIFData *gif;
    FrameData **frames;
    Boolean error, done;
    register Byte ch;
    Byte *eof, *pData;
    register int i,j;
    static Byte lmasks[8]={0xff, 0xfe, 0xfc, 0xf8, 0xf0, 0xe0, 0xc0, 0x80};
    Byte lmask;
    char fullPathName[PATH_MAX], dirName[PATH_MAX];
    char *dir;
    int startPos;
    int nFrames;
    Pixmap tempPixmap=(Pixmap)0;
    int prevFrame;

#if DEBUG_DISPOSAL
    print("loadGIF\n");
#endif

    gif=dlImage->privateData;
    fname=dlImage->imageName;
    fp=NULL;
    RawGIF=NULL;
    Raster=NULL;
    error=True;

  /* Initialize some globals */
    ScreenDepth=DefaultDepth(display,screenNum);

    if(strcmp(fname,"-") == 0) {
	fp=stdin;
	fname="<stdin>";
    } else {
#ifdef WIN32
      /* WIN32 opens files in text mode by default and then throws out CRLF */
	fp=fopen(fname,"rb");
#else
	fp=fopen(fname,"r");
#endif
#if DEBUG_OPEN
	print("loadGIF: fopen(1): fp=%x |%s|\n",fp,fname);
#endif
    }

  /* If not found and the name is a full path then can do no more */
    if(fp == NULL && isPath(fname)) {
	medmPrintf(1,"\nloadGIF: Cannot open file:\n"
	  "  %s\n",fname);
#if DEBUG_OPEN
	print("loadGIF: fopen(isPath): fp=%x |%s|\n",fp,fname);
#endif
	goto CLEANUP;
    }

  /* If not found yet, use path of ADL file */
    if(fp == NULL && displayInfo && displayInfo->dlFile &&
      displayInfo->dlFile->name) {
	char *stringPtr;

	strncpy(fullPathName, displayInfo->dlFile->name, PATH_MAX);
	fullPathName[PATH_MAX-1] = '\0';
	if(fullPathName && fullPathName[0]) {
	    stringPtr = strrchr(fullPathName, MEDM_DIR_DELIMITER_CHAR);
	    if(stringPtr) {
		*(++stringPtr) = '\0';
		strcat(fullPathName, fname);
#ifdef WIN32
	      /* WIN32 opens files in text mode by default and then throws out CRLF */
		fp=fopen(fullPathName,"rb");
#else
		fp=fopen(fullPathName,"r");
#endif
#if DEBUG_OPEN
		print("loadGIF: fopen(2): fp=%x |%s|\n",fp,fullPathName);
#endif
	    }
	}
    }

  /* If not found yet, look in EPICS_DISPLAY_PATH directories */
    if(fp == NULL) {
	dir=getenv("EPICS_DISPLAY_PATH");
	if(dir != NULL) {
	    startPos=0;
	    while(fp == NULL &&
	      extractStringBetweenColons(dir,dirName,startPos,&startPos)) {
		strcpy(fullPathName,dirName);
		strcat(fullPathName,MEDM_DIR_DELIMITER_STRING);
		strcat(fullPathName,fname);
#ifdef WIN32
	      /* WIN32 opens files in text mode by default and then throws out CRLF */
		fp=fopen(fullPathName,"rb");
#else
		fp=fopen(fullPathName,"r");
#endif
#if DEBUG_OPEN
	print("loadGIF: fopen(3): fp=%x |%s|\n",fp,fullPathName);
#endif
	    }
	}
    }
    if(fp == NULL) {
	medmPrintf(1,"\nloadGIF: Cannot open file:\n"
	  "  %s\n",fname);
	goto CLEANUP;
    }

  /* Find the size of the file */
    fseek(fp, 0L, SEEK_END);
    filesize=(int) ftell(fp);
    fseek(fp, 0L, SEEK_SET);

  /* Allocate memory for the raw data */
    if(!(ptr=RawGIF=(Byte *)malloc(filesize))) {
	medmPrintf(1,"\nloadGIF: Not enough memory to store GIF file:\n"
	  "  %s\n",fname);
	goto CLEANUP;
    }

  /* Allocate memory for the raster data */
    if(!(Raster=(Byte *)malloc(filesize))) {
	medmPrintf(1,"\nloadGIF: Not enough memory to store GIF file [2]:\n"
	  "  %s\n",fname);
	goto CLEANUP;
    }

  /* Read the file in one chunk */
    if(fread((char *)ptr, filesize, 1, fp) != 1) {
	char *errstring=strerror(ferror(fp));

	medmPrintf(1,"\nloadGIF: Cannot read file:\n"
	  "  %s\n  %s\n",fname,errstring);
	goto CLEANUP;
    }

  /* Parse the Header */
  /* Parse the Signature (3 bytes) and the Version (3 bytes) */
    if(!strncmp((char *)ptr, "GIF89a", 6)) {
    } else if(!strncmp((char *)ptr, "GIF87a", 6)) {
    } else {
	medmPrintf(1,"\nloadGIF: Not a valid GIF file\n  %s\n",fname);
	goto CLEANUP;
    }
    ptr += 6;

  /* Parse the Logical Screen Descriptor */
  /* Parse the required ScreenWidth, not used */
    ch=NEXTBYTE;
    LogicalScreenWidth=ch + 0x100 * NEXTBYTE;

  /* Parse the required ScreenHeight, not used */
    ch=NEXTBYTE;
    LogicalScreenHeight=ch + 0x100 * NEXTBYTE;

#if DEBUG_GIF || DEBUG_BYTEORDER
    {
	static int ifirst=1;

	if(ifirst) {
	    int depth=DefaultDepth(display,screenNum);
	    int bitmap_bit_order=BitmapBitOrder(display);
	    int bitmap_pad=BitmapPad(display);
	    int bitmap_unit=BitmapUnit(display);
	    int byte_order=ImageByteOrder(display);
	    int bits_per_pixel=_XGetBitsPerPixel(display,depth);

	    ifirst=0;
	    print("\nX Server Image Properties:\n");
	    print("  depth=%d [%d colors (0-%x)]\n",depth,1<<depth,
	      (int)(1<<depth)-1);
	    print("  bitmap_bit_order=%d [LSBFirst=%d MSBFirst=%d]\n",
	      bitmap_bit_order,LSBFirst,MSBFirst);
	    print("  bitmap_pad=%d\n",bitmap_pad);
	    print("  bitmap_unit=%d\n",bitmap_unit);
	    print("  byte_order=%d [LSBFirst=%d MSBFirst=%d]\n",
	      byte_order,LSBFirst,MSBFirst);
	    print("  bits_per_pixel=%d\n",bits_per_pixel);
	    print("\n  getClientByteOrder=%d [LSBFirst=%d MSBFirst=%d]",
	      getClientByteOrder(),LSBFirst,MSBFirst);
	    print("\n  getCorrectedByteOrder=%d"
	      " [LSBFirst=%d MSBFirst=%d]\n",
	      getCorrectedByteOrder(),LSBFirst,MSBFirst);
	}
    }
#endif

#if DEBUG_BYTEORDER
    {
	static int ifirst=1;

	if(ifirst) {
	    XImage *xImage;
	    Pixmap pixmap;
	    GC gc;
	    XColor color;

	    int depth;
	    int bitmap_bit_order;
	    int bitmap_pad;
	    int bitmap_unit;
	    int byte_order;
	    int bits_per_pixel;
	    unsigned long red_mask;
	    unsigned long green_mask;
	    unsigned long blue_mask;

	    ifirst=0;

	  /* Create a pixmap and draw a point */
	    pixmap=XCreatePixmap(display,RootWindow(display,screenNum),
	      1,1,XDefaultDepth(display,screenNum));

	  /* Create a GC */
	    gc=XCreateGC(display,pixmap,0,NULL);

	  /* Allocate a color */
	    color.red=0x1111;
	    color.green=0x2222;
	    color.blue=0x3333;
	    color.flags=DoRed|DoGreen|DoBlue;
	    XAllocColor(display,cmap,&color);

	  /* Draw a point */
	    XSetForeground(display,gc,color.pixel);
	    XDrawPoint(display,pixmap,gc,0,0);

	  /* Get a XImage of the pixmap */
	    xImage=XGetImage(display,pixmap,0,0,1,1,AllPlanes,ZPixmap);
	    depth=xImage->depth;
	    bitmap_bit_order=xImage->bitmap_bit_order;
	    bitmap_pad=xImage->bitmap_pad;
	    bitmap_unit=xImage->bitmap_unit;
	    byte_order=ImageByteOrder(display);
	    bits_per_pixel=xImage->bits_per_pixel;
	    red_mask=xImage->red_mask;
	    green_mask=xImage->green_mask;
	    blue_mask=xImage->blue_mask;


	    print("\nX Server Default XImage:\n");
	    print("  depth=%d [%d colors (0-%x)]\n",depth,1<<depth,
	      (int)(1<<depth)-1);
	    print("  bitmap_bit_order=%d [LSBFirst=%d MSBFirst=%d]\n",
	      bitmap_bit_order,LSBFirst,MSBFirst);
	    print("  bitmap_pad=%d\n",bitmap_pad);
	    print("  bitmap_unit=%d\n",bitmap_unit);
	    print("  byte_order=%d [LSBFirst=%d MSBFirst=%d]\n",
	      byte_order,LSBFirst,MSBFirst);
	    print("  bits_per_pixel=%d\n",bits_per_pixel);
	    print("  red_mask=%08x\n",red_mask);
	    print("  green_mask=%08x\n",green_mask);
	    print("  blue_mask=%08x\n",blue_mask);
	    print("  pixel=%08x\n",color.pixel);
	    print("  data[0][0]=%08x\n",*(Pixel *)xImage->data);

	  /* Cleanup */
	    if(gc) XFreeGC(display,gc);
	    if(pixmap) XFreePixmap(display,pixmap);
	    if(xImage) XDestroyImage(xImage);
	}
    }
#endif

  /* Parse the Packed byte
   * Bits 0-2: MASK210 Size of the Global Color Table
   * Bit 3:            Color Table Sort Flag (Ignored)
   * Bits 4-6:         Color Resolution (Ignored)
   * Bit 7:    MASK7   Global Color Table Flag
   */
    ch=NEXTBYTE;
    GlobalColorTableFlag=((ch & MASK7) ? True : False);
    GlobalColorTableSortFlag=((ch & MASK3) ? True : False);
    SizeOfGlobalColorTable=ch & MASK210;
    BitsPerPixel= SizeOfGlobalColorTable + 1;
    gif->numcols=ColorTableEntries=1 << BitsPerPixel;
    BitMask=ColorTableEntries - 1;

  /* Parse the BackgroundColorIndexColor */
    BackgroundColorIndex=NEXTBYTE;

  /* Parse the PixelAspectRatio, not used. */
    PixelAspectRatio=NEXTBYTE;

  /* End of header */
    if(verbose) {
	char version[7];

	strncpy(version,(char *)RawGIF,6);
	version[6]='\0';
	print("\nloadGIF: %s filesize=%d\n",fname,filesize);
	print("HEADER Version=%s\n",version);
	print("  LogicalScreenWidth=%d LogicalScreenHeight=%d\n",
	  LogicalScreenWidth,LogicalScreenHeight);
	print("  GlobalColorTableFlag=%s Sorted=%s\n",
	  GlobalColorTableFlag?"True":"False",
	  GlobalColorTableSortFlag?"True":"False");
	print("  SizeOfGlobalColorTable=%d (%d-bit, %d colors)\n",
	  SizeOfGlobalColorTable,
	  BitsPerPixel,ColorTableEntries);
	print("  BackgroundColorIndex=%d PixelAspectRatio=%d\n",
	  BackgroundColorIndex,PixelAspectRatio);
    }

  /* Parse the Global Color Table if present */
  /* KE: Note that if there is no global table, there may be local tables */
    if(GlobalColorTableFlag) {
	for(i=0; i < ColorTableEntries; i++) {
	    Red[i]=NEXTBYTE;
	    Green[i]=NEXTBYTE;
	    Blue[i]=NEXTBYTE;
#if DEBUG_GIF > 1
	    print("%3d %2x %2x %2x\n",i,Red[i],Green[i],Blue[i]);
#endif
	}

      /* Allocate the X colors for this picture.  Strip the least
       * significant bits of the colors to fit in the color table if
       * necessary.  There are two stripping algorithms, depending on
       * the value of nostrip */
        if(gif->nostrip) {
#if 0
	  /* nostrip=True */
	  /* KE: Currently nostrip is False and this branch is not
             used.  If it were used, it needs to be redone, keeping in
             mind that freeing colors that are calculated and not
             allocated will cause problems */
            j=0;
            lmask=lmasks[gif->strip];
            for(i=0; i<gif->numcols; i++) {
                gif->defs[i].red  =(Red[i]  &lmask)<<8;
                gif->defs[i].green=(Green[i]&lmask)<<8;
                gif->defs[i].blue =(Blue[i] &lmask)<<8;
                gif->defs[i].flags=DoRed | DoGreen | DoBlue;
                if(!XAllocColor(display,gif->theCmap,&gif->defs[i])) {
                    j++;
		    gif->defs[i].pixel=0xffff;
		}
                gif->cols[i]=gif->defs[i].pixel;
	    }

            if(j) {		/* Failed to pull it off */
                XColor ctab[256];

                medmPrintf(1,"\nloadGIF: Failed to allocate %d out of %d colors\n"
		  "  %s\n",j,gif->numcols,fname);

	      /* Read in the color table */
                for(i=0; i<gif->numcols; i++) ctab[i].pixel=i;
                XQueryColors(display,gif->theCmap,ctab,gif->numcols);

                for(i=0; i<gif->numcols; i++) {
		    if(gif->cols[i] == 0xffff) { /* An unallocated pixel */
			int d, mdist, close;
			unsigned long r,g,b;

			mdist=100000;   close=-1;
			r= Red[i];
			g= Green[i];
			b= Blue[i];
			for(j=0; j<gif->numcols; j++) {
			    d=abs((int)(r - (ctab[j].red>>8))) +
			      abs((int)(g - (ctab[j].green>>8))) +
			      abs((int)(b - (ctab[j].blue>>8)));
			    if(d<mdist) { mdist=d; close=j; }
			}
			if(close<0) {
			    medmPrintf(1,"loadGIF: "
			     "Simply can't do it -- Sorry\n  %s\n",fname);
			}
			memcpy( (void*)(&gif->defs[i]),
			  (void*)(&gif->defs[close]),
			  sizeof(XColor));
			gif->cols[i]=ctab[close].pixel;
		    }
		}
	    }
#endif
	} else {
          /* nostrip=False */
	  /* KE: Currently this is the only algorithm used */
            j=0;
            while(gif->strip<8) {
                lmask=lmasks[gif->strip];
                for(i=0; i<gif->numcols; i++) {
                    gif->defs[i].red  =(Red[i]  &lmask)<<8;
                    gif->defs[i].green=(Green[i]&lmask)<<8;
                    gif->defs[i].blue =(Blue[i] &lmask)<<8;
                    gif->defs[i].flags=DoRed | DoGreen | DoBlue;
                    if(!XAllocColor(display,gif->theCmap,&gif->defs[i])) {
			if(!XAllocColor(display,gif->theCmap,&black)) {
			    gif->cols[i]=blackPixel;
			} else {
			    gif->cols[i]=black.pixel;
			}
			break;
		    }
                    gif->cols[i]=gif->defs[i].pixel;
		}
                if(i<gif->numcols) {     /* Failed */
#if DEBUG_GIF
		    print("Auto strip %d (mask %2x) failed for color %d"
		      " [R=%hx G=%hx B=%hx]\n",
		      gif->strip,lmask,i,gif->defs[i].red,gif->defs[i].green,
		      gif->defs[i].blue);
#endif
                    gif->strip++;  j++;
                    XFreeColors(display,gif->theCmap,gif->cols,i,0UL);
		} else {
		  /* Success */
		    break;
		}
	    }

            if(j && gif->strip<8) {
		medmPrintf(0,"\nloadGIF: %s was masked to level %d"
		  " (mask 0x%2X)\n  %s\n",
		  fname,gif->strip,lmask,fname);
		if(verbose)
		  print("loadGIF: %s was masked to level %d (mask 0x%2X)\n",
		    fname,gif->strip,lmask);
	    }

            if(gif->strip==8) {
	      /* Algorithm not completely successful, Use the last try */
		j=0;
                lmask=lmasks[7];
                for(i=0; i<gif->numcols; i++) {
                    gif->defs[i].red  =(Red[i]  &lmask)<<8;
                    gif->defs[i].green=(Green[i]&lmask)<<8;
                    gif->defs[i].blue =(Blue[i] &lmask)<<8;
                    gif->defs[i].flags=DoRed | DoGreen | DoBlue;
                    if(XAllocColor(display,gif->theCmap,&gif->defs[i])) {
			gif->cols[i]=gif->defs[i].pixel;
		    } else {
			j++;
			if(!XAllocColor(display,gif->theCmap,&black)) {
			    gif->cols[i]=blackPixel;
			} else {
			    gif->cols[i]=black.pixel;
			}
		    }
		}
                medmPrintf(1,"\nloadGIF: "
		  "Strip algorithm failed to allocate %d out of %d colors\n"
		  "  %s\n",j,gif->numcols,fname);
	    }
	}
    } else {
      /* No global colormap in GIF file */
        medmPrintf(0,"\nloadGIF:  No global color table in %s."
	  "  Making one.\n",fname);
        if(!gif->numcols) gif->numcols=256;
        for(i=0; i < gif->numcols; i++) {
	    if(!XAllocColor(display,gif->theCmap,&black)) {
		gif->cols[i]=blackPixel;
	    } else {
		gif->cols[i]=black.pixel;
	    }
	}
    }

  /* Set the backgroundcolor index */
    gif->background=gif->cols[BackgroundColorIndex&(gif->numcols-1)];

  /* Set the number of images */
    pData=ptr;
    nFrames=countImages();
    if(nFrames <= 0) {
	gif->nFrames=0;
	goto CLEANUP;
    }
    if(verbose) {
	print("  %s has %d image(s)\n",fname,nFrames);
    }
  /* Allocate the GIFData array */
    gif->nFrames=nFrames;
    frames=(FrameData **)malloc(nFrames*sizeof(FrameData *));
    gif->frames=frames;
    if(!frames) {
	medmPrintf(1,"\nloadGIF: Not enough memory to store frame array:\n"
	  "  %s\n",fname);
	goto CLEANUP;
    }
    for(i=0; i < nFrames; i++) {
	frames[i]=(FrameData *)malloc(sizeof(FrameData));
	if(!frames[i]) {
	    medmPrintf(1,"\nloadGIF: Not enough memory to store frames:\n"
	      "  %s\n",fname);
	    goto CLEANUP;
	}
	frames[i]->theImage=NULL;
	frames[i]->expImage=NULL;
	frames[i]->Height=0;
	frames[i]->Width=0;
	frames[i]->TopOffset=0;
	frames[i]->LeftOffset=0;
	frames[i]->TransparentColorFlag=0;
	frames[i]->TransparentIndex=0;
	frames[i]->DisposalMethod=0;
	frames[i]->DelayTime=0;
    }

  /* Parse the data until the terminator is reached */
    ptr=pData;
    done=False;
    eof=RawGIF+filesize;     /* byte after the last character */
    gif->curFrame=-1;
    TransparentColorFlag=0;
    TransparentIndex=0;
    DelayTime=0;
    DisposalMethod=0;
    while(!done && ptr < eof) {
	ch=NEXTBYTE;
	switch(ch) {
	case IMAGESEP:
#if DEBUG_GIF
	    print("IMAGESEP [%2x]\n",ch);
#endif
	    BitOffset=0;
	    XC=0;
	    YC=0;
	    Pass=0;
	    OutCount=0;
	    gif->curFrame++;
	    if(!parseGIFImage(displayInfo,dlImage)) done=True;
	  /* Write the graph control params to the FrameData */
#if DEBUG_GIF
	    print("  TransparentColorFlag=%d TransparentIndex=%d\n"
	      "  DelayTime=%d DisposalMethod=%d\n",
	      TransparentColorFlag,TransparentIndex,
	      DelayTime,DisposalMethod);
#endif
	    gif->frames[CURFRAME(gif)]->Height=Height;
	    gif->frames[CURFRAME(gif)]->Width=Width;
	    gif->frames[CURFRAME(gif)]->TopOffset=TopOffset;
	    gif->frames[CURFRAME(gif)]->LeftOffset=LeftOffset;
	    gif->frames[CURFRAME(gif)]->TransparentColorFlag=
	      TransparentColorFlag;
	    gif->frames[CURFRAME(gif)]->TransparentIndex=TransparentIndex;
	    gif->frames[CURFRAME(gif)]->DisposalMethod=DisposalMethod;
	    gif->frames[CURFRAME(gif)]->DelayTime=DelayTime;
	  /* Restore graph control params (only have scope of one frame) */
	    TransparentColorFlag=0;
	    TransparentIndex=0;
	    DisposalMethod=0;
	    DelayTime=0;
	    break;
	case EXTENSION:
#if DEBUG_GIF
	    print("EXTENSION [%2x]\n",ch);
#endif
	    if(!parseGIFExtension()) done=True;
	    break;
	case TRAILER:
#if DEBUG_GIF
	    print("TRAILER [%2x]\n",ch);
#endif
	    done=True;
	    error=False;
	    break;
	default:
#if DEBUG_GIF
	    print("UNKNOWN [%2x]\n",ch);
#endif
	    done=True;
	    break;
	}
    }

  /* Check for error */
    if(error) {
	medmPrintf(1,"\nloadGIF: Error parsing GIF file for:\n  %s\n",fname);
	goto CLEANUP;
    }

#if DEBUG_GIF
    print("loadGIF: Enlarging images\n");
#endif
  /* Make full-size images for each frame if necessary.  Make a pixmap
   *   and do the image manipulation with the pixmap rather than
   *   manipulating bytes.  This way is platform independent though it
   *   may be slower */
    for(i=0; i < nFrames; i++) {
#if DEBUG_GIF || DEBUG_DISPOSAL > 1
	print("Frame %d: Width=%d Height=%d\n",
	  i,frames[i]->Width,frames[i]->Height);
	print("  LogicalScreenWidth=%d LogicalScreenHeight=%d\n",
	  LogicalScreenWidth,LogicalScreenHeight);
	print("  LeftOffset=%d TopOffset=%d\n",
	  frames[i]->LeftOffset,frames[i]->TopOffset);
	print("  TransparentColorFlag=%d TransparentColorIndex=%d\n",
	  frames[i]->TransparentColorFlag,frames[i]->TransparentIndex);
	print("  BackgroundColorIndex=%d DisposalMethod=%d\n",
	  BackgroundColorIndex,frames[i]->DisposalMethod);
	print("  theImage(1)=%x\n",frames[i]->theImage);
#endif
      /* Just set the byte_order if the frame is full size */
	if(frames[i]->Height == LogicalScreenHeight &&
	  frames[i]->Width == LogicalScreenWidth) {
	  /* Set the byte order since this came from our data */
	    frames[i]->theImage->byte_order=getCorrectedByteOrder();
	    continue;
	}

      /* Create a pixmap if not already created. Will happen for the
         first reduced-size frame. */
	if(!tempPixmap) {
#if DEBUG_GIF || DEBUG_DISPOSAL > 1
	    print("  Creating pixmap\n");
#endif
	    tempPixmap=XCreatePixmap(display,RootWindow(display,screenNum),
	      LogicalScreenWidth,LogicalScreenHeight,
	      XDefaultDepth(display,screenNum));
	    if(!tempPixmap) {
		medmPrintf(1,"\nloadGIF: Not enough memory for"
		  " temporary pixmap for:\n  %s\n",fname);
		goto CLEANUP;
	    }
	}

      /* Fill the pixmap */
	if(i == 0) {
	  /* Fill first frame with GIF background */
	    if(frames[i]->TransparentIndex == BackgroundColorIndex) {
	      /* Background is the transparent color, use display background */
		XSetForeground(display,gif->theGC,gif->bcol);
	    } else {
	      /* Use GIF background */
		XSetForeground(display,gif->theGC,gif->background);
	    }
	    XFillRectangle(display,tempPixmap,gif->theGC,
	      0,0,LogicalScreenWidth,LogicalScreenHeight);
	} else {
	  /* Fill based on the disposal method for the previous frame */
	    prevFrame=i-1;
	    switch(frames[prevFrame]->DisposalMethod) {
	    case 2:     /* Restore to background */
	    case 3:     /* Restore to previous */
	      /* Fill with display background */
		XSetForeground(display,gif->theGC,gif->bcol);
		XFillRectangle(display,tempPixmap,gif->theGC,
		  0,0,LogicalScreenWidth,LogicalScreenHeight);
		break;
	    case 0:     /* No action */
	    case 1:     /* Leave in place */
	    default:    /* To be defined */
	      /* Copy the previous image */
		XPutImage(display,tempPixmap,
		  gif->theGC,frames[prevFrame]->theImage,
		  0,0,0,0,LogicalScreenWidth,LogicalScreenHeight);
		break;
	    }
	}

      /* Copy in the reduced image */
	if(frames[i]->theImage) {
	    XImage *image;
	  /* Set the byte order since this came from our data */
	    frames[i]->theImage->byte_order=getCorrectedByteOrder();
	    XPutImage(display,tempPixmap,
	      gif->theGC,frames[i]->theImage,
	      0,0,frames[i]->LeftOffset,frames[i]->TopOffset,
	      frames[i]->Width,frames[i]->Height);
	  /* Destroy the current theImage */
	  /* Free the memory we allocated to insure it is done
	   * with the same routines that allocated it.  Can be a
	   * problem on WIN32 if X frees it. */
	    image=(XImage *)(frames[i]->theImage);
	    if(image->data) free(image->data);
	    image->data=NULL;
	  /* Then destroy the image */
	    XDestroyImage(image);
	}
#if DEBUG_DISPOSAL
	{
	    char title[80];

	    if(i == 5) {
		print("loadGIF: A tempPixmap theImage=%x\n",frames[i]->theImage);
		sprintf(title,"%da Frame",i);
		dumpPixmap(tempPixmap,(Dimension)LogicalScreenWidth,
		  (Dimension)LogicalScreenHeight,title);
	    }
	}
#endif

      /* Define theImage from the temporary pixmap.  Don't set the
         byte_order. This image is not from our data. XGetImage will
         get it right. */
	frames[i]->theImage=XGetImage(display,tempPixmap,0,0,
	  LogicalScreenWidth,LogicalScreenHeight,
	  AllPlanes,ZPixmap);

#if DEBUG_DISPOSAL
	{
	    char title[80];
	    Pixmap debugPixmap;

	    if(i == 5 && frames[i]->theImage) {
		print("loadGIF: B theImage=%x\n",frames[i]->theImage);
		debugPixmap=XCreatePixmap(display,RootWindow(display,screenNum),
		  LogicalScreenWidth,LogicalScreenHeight,
		  XDefaultDepth(display,screenNum));
		XPutImage(display,debugPixmap,
		  gif->theGC,frames[i]->theImage,
		  0,0,0,0,LogicalScreenWidth,LogicalScreenHeight);
		sprintf(title,"%db Frame",i);
		dumpPixmap(debugPixmap,(Dimension)LogicalScreenWidth,
		  (Dimension)LogicalScreenHeight,title);
		XFreePixmap(display,debugPixmap);
	    }
	}
#endif
#if DEBUG_DISPOSAL > 1
	print("  theImage(2)=%x\n",frames[i]->theImage);
#endif
}

  /* Clean up */
  CLEANUP:
    if(fp && fp != stdin) fclose(fp);
    if(Raster) {
	free((char *)Raster);
	Raster=NULL;
    }
    if(RawGIF) {
	free((char *)RawGIF);
	Raster=NULL;
    }
    if(tempPixmap) XFreePixmap(display,tempPixmap);
    if(error) freeGIF(dlImage);
    if(verbose) print("loadGIF: %s done (%s)\n",
      fname, error?"Failure":"Success");
    return(!error);
}

static int countImages(void)
{
    Boolean done;
    Byte BlockSize,DataBlockSize;
    Byte *eof;
    register Byte ch;
    int nFrames=0;

#if DEBUG_GIF > 1
    print("countImages: filename=%s filesize=%d\n",fname,filesize);
    fflush(stdout);
#endif
    done=False;
    eof=RawGIF+filesize;     /* byte after the last character */
    while(!done && ptr < eof) {
	ch=NEXTBYTE;
	switch(ch) {
	case IMAGESEP:
#if DEBUG_GIF > 1
	    print("IMAGESEP [%2x]\n",ch);
#endif
	    ptr+=8;
	  /* Check for local color table */
	    ch=NEXTBYTE;
	    LocalColorTableFlag=ch & MASK7;
	    SizeOfLocalColorTable=ch & MASK210;
	    LocalColorTableEntries= 1 << (SizeOfLocalColorTable + 1);
	    if(LocalColorTableFlag) ptr+=3*LocalColorTableEntries;
	  /* Skip LZW Minimum Code Size */
	    ptr++;
	    BlockSize=NEXTBYTE;
	    ptr+=BlockSize;
	  /* Note that double parentheses around a truth value keep
             GCC quiet */
	    while((DataBlockSize=NEXTBYTE)) {
		ptr+=DataBlockSize;
	    }
	    nFrames++;
	    break;
	case EXTENSION:
#if DEBUG_GIF > 1
	    print("EXTENSION [%2x]\n",ch);
#endif
	    ch=NEXTBYTE;
	    switch(ch) {
	    case GRAPHCONTROL:
#if DEBUG_GIF > 1
		print("  GRAPHCONTROL [%2x]\n",ch);
#endif
		BlockSize=NEXTBYTE;     /* Should be 4 */
		ptr+=BlockSize;
		while((DataBlockSize=NEXTBYTE)) {
		    ptr+=DataBlockSize;
		}
		break;
	    case PLAINTEXT:
#if DEBUG_GIF > 1
		print("  PLAINTEXT [%2x]\n",ch);
#endif
		BlockSize=NEXTBYTE;     /* Should be 12 */
		ptr+=BlockSize;
		while((DataBlockSize=NEXTBYTE)) {
		    ptr+=DataBlockSize;
		}
		break;
	    case APPLICATION:           /* Should be 11 */
#if DEBUG_GIF > 1
		print("  APPLICATION [%2x]\n",ch);
#endif
		BlockSize=NEXTBYTE;
		ptr+=BlockSize;
		while((DataBlockSize=NEXTBYTE)) {
		    ptr+=DataBlockSize;
		}
		break;
	    case COMMENT:
#if DEBUG_GIF > 1
		print("  COMMENT [%2x]\n",ch);
#endif
	      /* There is no block before the data here */
		while((DataBlockSize=NEXTBYTE)) {
		    ptr+=DataBlockSize;
		}
		break;
	    default:
#if DEBUG_GIF > 1
		print("  UNKNOWN [%2x]\n",ch);
#endif
		medmPrintf(0,"\ncountImages: Unknown extension [0x%x] for:\n  %s\n",
		  ch,fname);
		nFrames=-1;
		done=True;
	    }
	    break;
	case TRAILER:
#if DEBUG_GIF > 1
	    print("TRAILER [%2x]\n",ch);
#endif
	    done=True;
	    break;
	default:
#if DEBUG_GIF > 1
	    print("UNKNOWN [%2x]\n",ch);
#endif
	    medmPrintf(0,"\ncountImages: Unknown data separator [0x%x] for:\n  %s\n",
	      ch,fname);
	    nFrames=-1;
	    done=True;
	    break;
	}
    }

  /* Return */
    if(!done) {
	medmPrintf(0,"\nloadGIF: Terminator not found for:\n  %s\n",fname);
    }
    return nFrames;
}

static Boolean parseGIFImage(DisplayInfo *displayInfo, DlImage *dlImage)
{
    register Byte ch,ch1;
    register Byte *ptr1;
    int bits_per_pixel;
    int rasterBytes;
    int i;

  /* Now read in values from the image descriptor */
  /* Parse the Left word */
    ch=NEXTBYTE;
    LeftOffset=ch + 0x100 * NEXTBYTE;
  /* Parse the Top word */
    ch=NEXTBYTE;
    TopOffset=ch + 0x100 * NEXTBYTE;
  /* Parse the Width word */
    ch=NEXTBYTE;
    Width=ch + 0x100 * NEXTBYTE;
  /* Parse the Height word */
    ch=NEXTBYTE;
    Height=ch + 0x100 * NEXTBYTE;
  /* Parse the Packed byte
   * Bit 7:    MASK7   Local Color Table Flag
   * Bit 6:    MASK6   Interlace Flag
   * Bit 5:            Sort Flag (Ignored)
   * Bits 3-4:         Reserved
   * Bits 0-2: MASK210 Size of Local Color Table Entry
   */
    ch=NEXTBYTE;
    LocalColorTableFlag=ch & MASK7;
    Interlace=((ch & MASK6) ? True : False);
    SizeOfLocalColorTable=ch & MASK210;
    LocalColorTableEntries= 1 << (SizeOfLocalColorTable + 1);

    if(LocalColorTableFlag) {
	medmPrintf(1,"\nparseGIFImage: Cannot currently handle local color table."
	  "  Using global table.\n"
	  "  %s\n",fname);
      /* Skip over it */
	ptr+=3*LocalColorTableEntries;
    }

    if(verbose) {
        print("parseGIFImage: %s is %dx%d+%d+%d, %d colors, %s\n",
	  fname, Width,Height,LeftOffset,TopOffset,ColorTableEntries,
	  (Interlace) ? "Interlaced" : "Non-Interlaced");
	if(LocalColorTableFlag) {
	    print("  LocalColorTableEntries=%d\n",LocalColorTableEntries);
	}
    }

  /* Start reading the raster data. First we get the intial code size
   * and compute decompressor constant values, based on this code size.
   */
    CodeSize=NEXTBYTE;
    ClearCode=(1 << CodeSize);
    EOFCode=ClearCode + 1;
    FreeCode=FirstFree=ClearCode + 2;

  /* The GIF spec has it that the code size used to compute the above
   * values is the code size given in the file, but the code size used
   * in compression/decompression is the code size given in the file
   * plus one. (thus the ++).  */
    CodeSize++;
    InitCodeSize=CodeSize;
    MaxCode=(1 << CodeSize);
    ReadMask=MaxCode - 1;

  /* Read the raster data block by block until the block terminator
   * (block with BlockSize=0).  Just transpose it from the GIF array
   * to the Raster array, turning it from a series of blocks into one
   * long data stream, which makes life much easier for readCode().
   */
    ptr1=Raster;
    rasterBytes=0;
    do {
	ch=ch1=NEXTBYTE;     /* Number of bytes in block */
	rasterBytes+=ch;
#if DEBUG_GIF > 1
	print("parseGIFImage: rasterBytes=%d BlockSize=%d ch=%2x position=%d\n",
	  rasterBytes,ch1,*ptr,ptr-RawGIF+1);
#endif
	while(ch--) *ptr1++=NEXTBYTE;
	if((Raster - ptr1) > filesize){
	    medmPrintf(1,"\nparseGIFImage: "
	      "Trying to read past end of file for:\n  %s\n",fname);
	    return(False);
	}
    } while(ch1);     /* Continue until block terminator */

    if(verbose)
      print("parseGIFImage: %s decompressing...\n",fname);

/* Allocate the X Image.  The bitmap pad (next-to-last argument) may
  * be 8, 16, or 32.  The scanlines are padded out to an even number
  * of the bitmap_pad.  Use 8.  This should work with what we are
  * doing for depths of 8, 16, 24, 32, etc.  The bytes_per_line (last
  * argument) may be zero, indicating that X will calculate it for
  * you.  Otherwise it is probably width*bits_per_pixel/8. (Used to be
  * a switch statement here.) */
    bits_per_pixel=_XGetBitsPerPixel(display, ScreenDepth);
    BytesOffsetPerPixel=bits_per_pixel/8;
    ImageDataSize=BytesOffsetPerPixel*Width*Height;
#if DEBUG_GIF
    print("parseGIFImage: %s (24-bit display, %d bits per pixel)\n"
      "  BytesOffsetPerPixel=%d ImageSize=%d\n",
      fname, bits_per_pixel, BytesOffsetPerPixel,
      BytesOffsetPerPixel*Width*Height);
#endif
    Image=(Byte *)malloc(ImageDataSize);
    if(!Image) {
	medmPrintf(1,"\nparseGIFImage: "
	  "Not enough memory for XImage for:\n  %s\n",
	  fname);
	return(False);
    }
    CURIMAGE(gif)=XCreateImage(display,gif->theVisual,
      ScreenDepth,ZPixmap,
      0,(char*)Image,Width,Height,8,0);
    if(!CURIMAGE(gif)) {
	medmPrintf(1,"\nparseGIFImage: Unable to create XImage for:\n  %s\n",
	  fname);
	return(False);
    }
    BytesPerScanline=CURIMAGE(gif)->bytes_per_line;

  /* Decompress the file, continuing until we see the GIF EOF code or
   * we use up all the bytes in the data */
    Code=readCode();
    while(Code != EOFCode && BitOffset/8 < rasterBytes) {
      /* Clear code sets everything back to its initial value, then reads the
       * immediately subsequent code as uncompressed data.
       */
	if(Code == ClearCode) {
	    CodeSize=InitCodeSize;
	    MaxCode=(1 << CodeSize);
	    ReadMask=MaxCode - 1;
	    FreeCode=FirstFree;
	    CurCode=OldCode=Code=readCode();
	    FinChar=CurCode & BitMask;
	    addToPixel(gif,(Byte)FinChar);
	} else {
	  /* If not a clear code, then must be data: save same as CurCode and InCode */
	    CurCode=InCode=Code;

	  /* If greater or equal to FreeCode, not in the hash table yet;
	   * repeat the last character decoded
	   */
	    if(CurCode >= FreeCode) {
		CurCode=OldCode;
		OutCode[OutCount++]=FinChar;
	    }

	  /* Unless this code is raw data, pursue the chain pointed to by CurCode
	   * through the hash table to its end; each code in the chain puts its
	   * associated output code on the output queue.
	   */
	    while(CurCode > BitMask) {
		if(OutCount > 1024){
		    XImage *image;
		    medmPrintf(1,"\nparseGIFImage: Corrupt GIF file (OutCount)\n"
		      "  %s\n",fname);
		  /* Free the memory we allocated to insure it is done
		   * with the same routines that allocated it.  Can be a
		   * problem on WIN32 if X frees it. */
		    image=(XImage *)(CURIMAGE(gif));
		    if(image->data) free(image->data);
		    image->data=NULL;
		  /* Then destroy the image */
		    XDestroyImage(image);
		    return False;
		}
		OutCode[OutCount++]=Suffix[CurCode];
		CurCode=Prefix[CurCode];
	    }

	  /* The last code in the chain is treated as raw data. */
	    FinChar=CurCode & BitMask;
	    OutCode[OutCount++]=FinChar;

	  /* Now we put the data out to the Output routine.
	   * It's been stacked LIFO, so deal with it that way...
	   */
	    for(i=OutCount - 1; i >= 0; i--)
	      addToPixel(gif,(Byte)OutCode[i]);
	    OutCount=0;

	  /* Build the hash table on-the-fly. No table is stored in the file. */
	    Prefix[FreeCode]=OldCode;
	    Suffix[FreeCode]=FinChar;
	    OldCode=InCode;

	  /* Point to the next slot in the table.  If we exceed the current
	   * MaxCode value, increment the code size unless it's already 12.  If it
	   * is, do nothing: the next code decompressed better be CLEAR
	   */
	    FreeCode++;
	    if(FreeCode >= MaxCode) {
		if(CodeSize < 12) {
		    CodeSize++;
		    MaxCode *= 2;
		    ReadMask=(1 << CodeSize) - 1;
		}
	    }
	}
	Code=readCode();
    }

    return True;
}

static Boolean parseGIFExtension(void)
{
    Byte ch;
    Byte BlockSize,DataBlockSize;

    ch=NEXTBYTE;
    switch(ch) {
    case GRAPHCONTROL:
#if DEBUG_GIF
	print("  GRAPHCONTROL [%2x]\n",ch);
#endif
      /* Scope is the next image */
	BlockSize=NEXTBYTE;     /* Should be 4 */
      /* Parse the Packed byte
       * Bits 5-7:         Reserved (Ignored)
       * Bits 2-4:         Disposal Method
       * Bit 1:            User Input Flag (Ignored)
       * Bit 0:            Transparent Flag
       */
	ch=NEXTBYTE;
	TransparentColorFlag=ch & MASK0;
	DisposalMethod=(ch & MASK432)>>2;
      /* Parse the Delay Time (1/100 sec ) */
	ch=NEXTBYTE;
	DelayTime=ch + 0x100 * NEXTBYTE;
      /* Parse the TransparentIndex */
	TransparentIndex=NEXTBYTE;
      /* Should only be a block terminator here */
#if DEBUG_GIF
	print("    TransparentColorFlag=%d TransparentIndex=%d "
	  "DelayTime=%d\n",
	  TransparentColorFlag,TransparentIndex,DelayTime);
#endif
	while((DataBlockSize=NEXTBYTE)) {
	    ptr+=DataBlockSize;
	}
	break;
    case PLAINTEXT:
#if DEBUG_GIF
	print("  PLAINTEXT [%2x]\n",ch);
#endif
      /* Ignore */
	BlockSize=NEXTBYTE;     /* Should be 12 */
	ptr+=BlockSize;
	while((DataBlockSize=NEXTBYTE)) {
	    ptr+=DataBlockSize;
	}
	break;
    case APPLICATION:           /* Should be 11 */
#if DEBUG_GIF
	print("  APPLICATION [%2x]\n",ch);
#endif
      /* Ignore */
	BlockSize=NEXTBYTE;
#if DEBUG_GIF
	{
	    char ident[9];

	    strncpy(ident,(char *)ptr,8);
	    ident[8]='\0';
	    print("    ApplicationIdentifier=%s\n",ident);
	}
#endif
	ptr+=BlockSize;
	while((DataBlockSize=NEXTBYTE)) {
	    ptr+=DataBlockSize;
	}
	break;
    case COMMENT:
#if DEBUG_GIF
	print("  COMMENT [%2x]\n",ch);
#endif
      /* Ignore (There is no block before the data here) */
	while((DataBlockSize=NEXTBYTE)) {
	    ptr+=DataBlockSize;
	}
	break;
    default:
#if DEBUG_GIF
	print("  UNKNOWN [%2x]\n",ch);
#endif
	return False;
    }
    return True;
}

/* Function to free the X images and color cells in the default
 *   colormap (in anticipation of a new image) */
void freeGIF(DlImage *dlImage)
{
    GIFData *gif;
    FrameData **frames,*frame;
    int i,nFrames;

    gif=(GIFData *)dlImage->privateData;
  /* Free the GIFData */
    if(gif) {
      /* Free the colors */
	if(gif->numcols > 0) {
#if MANAGE_IMAGE_COLOR_ALLOCATION
#if 0
	  /* We need to free these one by one to check for the
             blackPixel, which will give a BadAccess error */
	    for(i=0; i < gif->numcols; i++) {
		if(gif->cols[i] != blackPixel) {
		    XFreeColors(display,gif->theCmap,&(gif->cols[i]),1,0UL);
		}
	    }
#else
	  /* This is more efficient.  blackPixel should not be used any more. */
	    XFreeColors(display,gif->theCmap,gif->cols,gif->numcols,0UL);
#endif
#endif
	    gif->numcols=0;
	}
      /* Free each image */
	nFrames=gif->nFrames;
	frames=gif->frames;
	if(frames) {
	    for(i=0; i < nFrames; i++) {
		frame=frames[i];
		if(frame) {
		  /* Kill the old images */
		    if(frame->expImage) {
		      /* Free the memory we allocated to insure it is done
		       * with the same routines that allocated it.  Can be a
		       * problem on WIN32 if X frees it. */
			XImage *image=(XImage *)(frame->expImage);
			if(image->data) free(image->data);
			image->data=NULL;
		      /* Then destroy the image */
			XDestroyImage(image);
		      /* Check if theImage is the same as expImage
                         (Resize special case) */
			if(frame->expImage == frame->theImage) {
			    frame->theImage=NULL;
			}
			frame->expImage=NULL;
		    }
		    if(frame->theImage) {
		      /* Free the memory we allocated to insure it is done
		       * with the same routines that allocated it.  Can be a
		       * problem on WIN32 if X frees it. */
			XImage *image=(XImage *)(frame->theImage);
			if(image->data) free(image->data);
			image->data=NULL;
		      /* Then destroy the image */
			XDestroyImage(image);
			frame->theImage=NULL;
		    }
		    free(frame);
		    frame=NULL;
		}
	    }
	    free(frames);
	    frames=NULL;
	}
      /* Free the GIFData */
	free((char *)gif);
	gif=dlImage->privateData=NULL;
    }
}

/* Function to make a new copy of the private data in dlImage1 and
   store it in dlImage2.  dlImage1 and dlImage2 may point to the same
   image. */
void copyGIF(DlImage *dlImage1, DlImage *dlImage2)
{
    GIFData *gif1,*gif2;
    FrameData **frames;
    int i,nFrames;
    Byte *ximag;
    int bytesPerPixel,imageDataSize;

#if DEBUG_DISPOSAL
    print("copyGIF: \n");
#endif

    gif1=(GIFData *)dlImage1->privateData;
    if(!gif1) {
	gif2=NULL;
	dlImage2->privateData=gif2;
	return;
    }
    if(!(gif2=(GIFData *)malloc(sizeof(GIFData)))) {
	medmPrintf(1,"\ncopyGIF: Memory allocation error:\n"
	  "  %s\n",gif1->imageName);
	dlImage2->privateData=gif2;
	return;
    }
    *gif2=*gif1;

#if MANAGE_IMAGE_COLOR_ALLOCATION
  /* Reallocate the colors to make the ref count increase so MEDM will
     retain ownership of the color cells.  This takes a long time. */
    for(i=0; i < gif2->numcols; i++) {
	if(!XAllocColor(display,gif2->theCmap,&gif2->defs[i])) {
	    if(!XAllocColor(display,gif->theCmap,&black)) {
		gif2->cols[i]=blackPixel;
	    } else {
		gif2->cols[i]=black.pixel;
	    }
	}
    }
#endif

  /* Allocate the new frames array */
    nFrames=gif2->nFrames;
    frames=(FrameData **)malloc(nFrames*sizeof(FrameData *));
    gif2->frames=frames;
    if(!frames) {
	medmPrintf(1,"\ncopyGIF: Not enough memory to store frame array:\n"
	  "  %s\n",gif2->imageName);
	dlImage2->privateData=gif2;
	return;
    }

  /* Allocate the frames */
    if(frames) {
    for(i=0; i < nFrames; i++) {
	frames[i]=(FrameData *)malloc(sizeof(FrameData));
	if(!frames[i]) {
	    medmPrintf(1,"\ncopyGIF: Not enough memory to store frames:\n"
	      "  %s\n",gif2->imageName);
	    dlImage2->privateData=gif2;
	    return;
	}
	gif1->curFrame=gif2->curFrame=i;
	if(gif1->frames[i] && frames[i]) {
	    *frames[i]=*gif1->frames[i];
	  /* Allocate and copy the images */
	    if(CURIMAGE(gif1)) {
		bytesPerPixel=CURIMAGE(gif1)->bits_per_pixel/8;
		imageDataSize=gif1->iWIDE*gif1->iHIGH*bytesPerPixel;
		ximag=(Byte *)malloc(imageDataSize);
		memcpy(ximag,CURIMAGE(gif1)->data,imageDataSize);
		CURIMAGE(gif2)=XCreateImage(display,gif1->theVisual,
		  DefaultDepth(display,screenNum),ZPixmap,
		  0,(char *)ximag,gif1->iWIDE,gif1->iHIGH,8,0);
	    } else {
		CURIMAGE(gif2)=NULL;
	    }
	    if(CUREXPIMAGE(gif1)) {
		bytesPerPixel=CUREXPIMAGE(gif1)->bits_per_pixel/8;
		imageDataSize=gif1->eWIDE*gif1->eHIGH*bytesPerPixel;
		ximag=(Byte *)malloc(imageDataSize);
		memcpy(ximag,CUREXPIMAGE(gif1)->data,imageDataSize);
		CUREXPIMAGE(gif2)=XCreateImage(display,gif1->theVisual,
		  DefaultDepth(display,screenNum),ZPixmap,
		  0,(char *)ximag,gif1->eWIDE,gif1->eHIGH,8,0);
	    } else {
		CUREXPIMAGE(gif2)=NULL;
	    }
	}
    }
    gif1->curFrame=gif2->curFrame=0;
    dlImage2->privateData=gif2;
    }
}

/* Static utility functions */

#if 0
/* Function to determine the byte order on the client  */
static int getClientByteOrder()
{
    short i=1;

    return (*(char*)&i == 1) ? LSBFirst : MSBFirst;
}
#endif

/* Function to determine the byte order to be used on the server */
static int getCorrectedByteOrder()
{
#if 1
  /* We wrote the bytes in MSBFirst, tell the server to use MSBFirst */
    return MSBFirst;
#else
#ifndef VMS
    int clientByteOrder=getClientByteOrder();

  /* The following empirically seems to work for Solaris, WIN32, and
     Linux.  Possible explanation: We are writing MSBFirst on a
     LSBFirst machine, so we need to reverse what the server would
     ordinarily do.  */
    if(clientByteOrder == MSBFirst) {
	return MSBFirst;
    } else {
	int byte_order=ImageByteOrder(display);
	return (byte_order == LSBFirst)?MSBFirst:LSBFirst;
    }
#else
  /* From ACM.  The above algorithm would probably work, but no way to
     check */
    return MSBFirst;
#endif
#endif
}

/* Fetch the next code from the raster data stream.  The codes can be
 * any length from 3 to 12 bits, packed into 8-bit Bytes, so we have to
 * maintain our location in the Raster array as a BIT Offset.  We compute
 * the Byte Offset into the raster array by dividing this by 8, pick up
 * three Bytes, compute the bit Offset into our 24-bit chunk, shift to
 * bring the desired code to the bottom, then mask it off and return it.
 */
static int readCode()
{
    int RawCode,ByteOffset;

    ByteOffset=BitOffset/8;
    RawCode=Raster[ByteOffset]+(0x100*Raster[ByteOffset+1]);
    if(CodeSize >= 8)
      RawCode+=(0x10000*Raster[ByteOffset+2]);
#if DEBUG_CODES
    print("readCode: XC=%d YC=%d BitOffset=%d ByteOffset=%d Rawcode=%x",
      XC,YC,BitOffset,ByteOffset,RawCode);
#endif
    RawCode>>=(BitOffset%8);
#if DEBUG_CODES
    print("->%x ReadMask=%x RawCode&ReadMask=%x\n",
      RawCode,ReadMask,RawCode&ReadMask);
#endif
    BitOffset+=CodeSize;
    return(RawCode&ReadMask);
}

#if 0
static void dumpGIF(GIFData *gif)
{
    int i;
    for(i=0; i<32; i++) {
	if(i && ((i>>4)<<4) == i) {
	    print("\n");
	}
	print("%02x ",(Byte)CURIMAGE(gif)->data[i]);
    }
    print("\n");
}
#endif

static void addToPixel(GIFData *gif, Byte Index)
{
    Pixel clr;
    int offset=YC*BytesPerScanline+XC;

  /* Check that we are in bounds and write to the XImage data */
    if(offset < ImageDataSize) {
	Byte *p=(Byte *)Image+offset;

      /* Check for transparent color */
	if(TransparentColorFlag && (Index&(gif->numcols-1)) ==
	  TransparentIndex) {
	    clr=gif->bcol;
	} else {
	    clr=gif->cols[Index&(gif->numcols-1)];
	}


      /* Copy the appropriate part of the pixel in MSBFirst order */
	switch(BytesOffsetPerPixel) {
	case 4:
	    *p++=(Byte)((clr & 0xff000000) >> 24);
	  /* Fall through */
	case 3:
	    *p++=(Byte)((clr & 0x00ff0000) >> 16);
	  /* Fall through */
	case 2:
	    *p++=(Byte)((clr & 0x0000ff00) >> 8);
	  /* Fall through */
	case 1:
	    *p=(Byte)((clr & 0x000000ff));
	    break;
	}
    }

  /* Update the X-coordinate, and if it overflows, update the Y-coordinate */
    XC=XC+BytesOffsetPerPixel;
    if(XC+BytesOffsetPerPixel > BytesPerScanline) {
      /* If a non-interlaced picture, just increment YC to the next scan line.
       * If it's interlaced, deal with the interlace as described in the GIF
       * spec.  Put the decoded scan line out to the screen if we haven't gone
       * past the bottom of it
       */
	XC=0;
	if(!Interlace) {
	    YC++;
	} else {
	    switch (Pass) {
	    case 0:
		YC += 8;
		if(YC >= Height) {
		    Pass++;
		    YC=4;
		}
		break;
	    case 1:
		YC += 8;
		if(YC >= Height) {
		    Pass++;
		    YC=2;
		}
		break;
	    case 2:
		YC += 4;
		if(YC >= Height) {
		    Pass++;
		    YC=1;
		}
		break;
	    case 3:
		YC += 2;
		break;
	    default:
		break;
	    }
	}
    }
}
