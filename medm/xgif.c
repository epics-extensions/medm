/*
*****************************************************************
                          COPYRIGHT NOTIFICATION
*****************************************************************

THE FOLLOWING IS A NOTICE OF COPYRIGHT, AVAILABILITY OF THE CODE,
AND DISCLAIMER WHICH MUST BE INCLUDED IN THE PROLOGUE OF THE CODE
AND IN ALL SOURCE LISTINGS OF THE CODE.

(C)  COPYRIGHT 1993 UNIVERSITY OF CHICAGO

Argonne National Laboratory (ANL), with facilities in the States of
Illinois and Idaho, is owned by the United States Government, and
operated by the University of Chicago under provision of a contract
with the Department of Energy.

Portions of this material resulted from work developed under a U.S.
Government contract and are subject to the following license:  For
a period of five years from March 30, 1993, the Government is
granted for itself and others acting on its behalf a paid-up,
nonexclusive, irrevocable worldwide license in this computer
software to reproduce, prepare derivative works, and perform
publicly and display publicly.  With the approval of DOE, this
period may be renewed for two additional five year periods.
Following the expiration of this period or periods, the Government
is granted for itself and others acting on its behalf, a paid-up,
nonexclusive, irrevocable worldwide license in this computer
software to reproduce, prepare derivative works, distribute copies
to the public, perform publicly and display publicly, and to permit
others to do so.

*****************************************************************
                                DISCLAIMER
*****************************************************************

NEITHER THE UNITED STATES GOVERNMENT NOR ANY AGENCY THEREOF, NOR
THE UNIVERSITY OF CHICAGO, NOR ANY OF THEIR EMPLOYEES OR OFFICERS,
MAKES ANY WARRANTY, EXPRESS OR IMPLIED, OR ASSUMES ANY LEGAL
LIABILITY OR RESPONSIBILITY FOR THE ACCURACY, COMPLETENESS, OR
USEFULNESS OF ANY INFORMATION, APPARATUS, PRODUCT, OR PROCESS
DISCLOSED, OR REPRESENTS THAT ITS USE WOULD NOT INFRINGE PRIVATELY
OWNED RIGHTS.

*****************************************************************
LICENSING INQUIRIES MAY BE DIRECTED TO THE INDUSTRIAL TECHNOLOGY
DEVELOPMENT CENTER AT ARGONNE NATIONAL LABORATORY (630-252-2000).
*/
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
static void dumpGIF(GIFData *gif);
static int getClientByteOrder();
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

/* Function to initialize for GIF processing */
Boolean initializeGIF(DisplayInfo *displayInfo, DlImage *dlImage)
{
    int x, y;
    unsigned int w, h;
    Boolean success;

    x=dlImage->object.x;
    y=dlImage->object.y;
    w=dlImage->object.width;
    h=dlImage->object.height;

  /* Free any existing GIF resources */
    freeGIF(dlImage);

    if (!(gif=(GIFData *)malloc(sizeof(GIFData)))) {
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
    if (gif->displayCells < 255) {
	medmPrintf(1,"\ninitializeGIF: At least an 8-plane display is required");
	freeGIF(dlImage);
	return(False);
    }

    gif->nFrames=0;
    gif->curFrame=0;
    strcpy(gif->imageName,dlImage->imageName);
    
  /* Open and read the file  */
    success=loadGIF(displayInfo,dlImage);
    if (!success) {
	return(False);
    }
    
    gif->iWIDE=LogicalScreenWidth;
    gif->iHIGH=LogicalScreenHeight;
    
    gif->eWIDE=gif->iWIDE;
    gif->eHIGH=gif->iHIGH;
    
    resizeGIF(dlImage);
    
  /* Don't automatically draw any more */
#if 0
    if (gif->frames) {
	XImage *image=CUREXPIMAGE(gif);
	
	XSetForeground(display,gif->theGC,gif->bcol);
	XFillRectangle(display,XtWindow(displayInfo->drawingArea),
	  gif->theGC,x,y,w,h);

#if 0
      /* Determine the order of the pixels */
	gif->expImage->byte_order=getClientByteOrder();
#endif	

	XPutImage(display,XtWindow(displayInfo->drawingArea),
	  gif->theGC,image,
	  0,0,x,y,w,h);
	XPutImage(display,displayInfo->drawingAreaPixmap,
	  gif->theGC,image,
	  0,0,x,y,w,h);
	XSetForeground(display,gif->theGC,gif->fcol);
    }
#endif
    
    return(True);

}

void drawGIF(DisplayInfo *displayInfo, DlImage *dlImage)
{
    GIFData *gif;
    int x, y;
    unsigned int w, h;

    gif=(GIFData *)dlImage->privateData;

    if (CUREXPIMAGE(gif) != NULL) {
	x=dlImage->object.x;
	y=dlImage->object.y;
	w=dlImage->object.width;
	h=dlImage->object.height;
	
      /* Determine the order of the pixels */
	CUREXPIMAGE(gif)->byte_order=getClientByteOrder();
	
      /* Copy to the drawing area */
	XPutImage(display,XtWindow(displayInfo->drawingArea),
	  gif->theGC,CUREXPIMAGE(gif),
	  0,0,x,y,w,h);
      /* Draw to pixmap */
	XPutImage(display,displayInfo->drawingAreaPixmap,
	  gif->theGC,CUREXPIMAGE(gif),
	  0,0,x,y,w,h);
    }
}

void resizeGIF(DlImage *dlImage)
{
    GIFData *gif;
    unsigned int w,h;
    int i;

    static char *rstr="Resizing Image.  Please wait...";

  /* warning:  this code'll only run machines where int=32-bits */

    gif=(GIFData *)dlImage->privateData;
    w=dlImage->object.width;
    h=dlImage->object.height;

  /* simply return if no GIF image attached */
    if (!gif->frames) return;

    gif->currentWidth=w;
    gif->currentHeight= h;

  /* Loop over frames */
    for(i=0; i < gif->nFrames; i++) {
	gif->curFrame=i;
	if ((int)w==gif->iWIDE && (int)h==gif->iHIGH) {
	  /* Very special case (size=actual GIF size) */
	    if (CUREXPIMAGE(gif) != CURIMAGE(gif)) {
		if (CUREXPIMAGE(gif) != NULL) {
		    free(CUREXPIMAGE(gif)->data);
		    CUREXPIMAGE(gif)->data=NULL;
		    XDestroyImage((XImage *)(CUREXPIMAGE(gif)));
		}
		CUREXPIMAGE(gif)=CURIMAGE(gif);
		gif->eWIDE=gif->iWIDE;  gif->eHIGH=gif->iHIGH;
	    }
	} else {				/* have to do some work */
	  /* if it's a big image, this'll take a while.  mention it */
	    if (w*h>(400*400)) {
	      /* (MDA) - could change cursor to stopwatch...*/
	    }
	    
	  /* first, kill the old CUREXPIMAGE(gif), if one exists */
	    if (CUREXPIMAGE(gif) && CUREXPIMAGE(gif) != CURIMAGE(gif)) {
		free(CUREXPIMAGE(gif)->data);
		CUREXPIMAGE(gif)->data=NULL;
		XDestroyImage((XImage *)(CUREXPIMAGE(gif)));
	    }
	    
	  /* create CUREXPIMAGE(gif) of the appropriate size */
	    gif->eWIDE=w;  gif->eHIGH=h;
	    
	    switch (DefaultDepth(display,screenNum)) {
	    case 8: {
		int  ix,iy,ex,ey;
		Byte *ximag,*ilptr,*ipptr,*elptr,*epptr;
		
		ximag=(Byte *)malloc(w*h);
		CUREXPIMAGE(gif)=XCreateImage(display,gif->theVisual,
		  DefaultDepth(display,screenNum),ZPixmap,
		  0,(char *)ximag,gif->eWIDE,gif->eHIGH,32,0);
		
		if (!ximag || !CUREXPIMAGE(gif)) {
		    medmPrintf(1,"\nresizeGIF: Unable to create a %dx%d image\n",
		      w,h);
		    return;
		}
		
		elptr=epptr=(Byte *)CUREXPIMAGE(gif)->data;
		
		for (ey=0;  ey<gif->eHIGH;  ey++, elptr+=gif->eWIDE) {
		    iy=(gif->iHIGH * ey) / gif->eHIGH;
		    epptr=elptr;
		    ilptr=(Byte *)CURIMAGE(gif)->data + (iy * gif->iWIDE);
		    for (ex=0;  ex<gif->eWIDE;  ex++,epptr++) {
			ix=(gif->iWIDE * ex) / gif->eWIDE;
			ipptr=ilptr + ix;
			*epptr=*ipptr;
		    }
		}
		break;
	    }
	    case 24: {
		int sx, sy, dx, dy;
		int sh, dh, sw, dw;
		Byte *ximag,*ilptr,*ipptr,*elptr,*epptr;
		int bytesPerPixel=CURIMAGE(gif)->bits_per_pixel/8;
		
		ximag=(Byte *)malloc(w*h*bytesPerPixel);
		CUREXPIMAGE(gif)=XCreateImage(display,gif->theVisual,
		  DefaultDepth(display,screenNum),ZPixmap,
		  0,(char *)ximag,gif->eWIDE,gif->eHIGH,32,0);
		
		if (!ximag || !CUREXPIMAGE(gif)) {
		    medmPrintf(1,"\nresizeGIF: Unable to create a %dx%d image\n",
		      w,h);
		    medmPrintf(0,"  24 bit: ximag=%08x, expImage=%08x\n",ximag,
		      CUREXPIMAGE(gif));
		    return;
		}
		
#if 1
		sw=CURIMAGE(gif)->width;
		sh=CURIMAGE(gif)->height;
		dw=CUREXPIMAGE(gif)->width;
		dh=CUREXPIMAGE(gif)->height;
		elptr=epptr=(Byte *)CUREXPIMAGE(gif)->data;
		for (dy=0;  dy<dh; dy++) {
		    sy=(sh * dy) / dh;
		    epptr=elptr;
		    ilptr=(Byte *)CURIMAGE(gif)->data + (sy * CURIMAGE(gif)->bytes_per_line);
		    for (dx=0;  dx <dw;  dx++) {
			sx=(sw * dx) / dw;
			ipptr=ilptr + sx*bytesPerPixel;
			*epptr++=*ipptr++;
			*epptr++=*ipptr++;
			*epptr++=*ipptr++;
			if (bytesPerPixel == 4) {
			    *epptr++=*ipptr++;
			}
		    }
		    elptr += CUREXPIMAGE(gif)->bytes_per_line;
		}
#endif
		break;
	    }
	    }
	}
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
    char fullPathName[2*MAX_TOKEN_LENGTH], dirName[2*MAX_TOKEN_LENGTH];
    char *dir;
    int startPos;
    int nFrames;
    Pixmap tempPixmap=NULL;
    int prevFrame;

    gif=dlImage->privateData;
    fname=dlImage->imageName;
    fp=NULL;
    RawGIF=NULL;
    Raster=NULL;
    error=True;

  /* Initialize some globals */
    ScreenDepth=DefaultDepth(display,screenNum);

    if (strcmp(fname,"-")==0) {
	fp=stdin;
	fname="<stdin>";
    } else {
#ifdef WIN32
      /* WIN32 opens files in text mode by default and then throws out CRLF */
	fp=fopen(fname,"rb");
#else
	fp=fopen(fname,"r");
#endif
    }

  /* Try to get a valid GIF file somewhere. If not in the current
   * directory, look in EPICS_DISPLAY_PATH directories */
    if (fp == NULL) {
	dir=getenv("EPICS_DISPLAY_PATH");
	if (dir != NULL) {
	    startPos=0;
	    while (fp == NULL &&
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
	    }
	}
    }
    if (fp == NULL) {
	medmPrintf(1,"\nloadGIF: Cannot open file:\n"
	  "  %s\n",fname);
	goto CLEANUP;
    }

  /* Find the size of the file */
    fseek(fp, 0L, SEEK_END);
    filesize=(int) ftell(fp);
    fseek(fp, 0L, SEEK_SET);

  /* Allocate memory for the raw data */
    if (!(ptr=RawGIF=(Byte *)malloc(filesize))) {
	medmPrintf(1,"\nloadGIF: Not enough memory to store GIF file:\n"
	  "  %s\n",fname);
	goto CLEANUP;
    }
    
  /* Allocate memory for the raster data */
    if (!(Raster=(Byte *)malloc(filesize))) {
	medmPrintf(1,"\nloadGIF: Not enough memory to store GIF file [2]:\n"
	  "  %s\n",fname);
	goto CLEANUP;
    }

  /* Read the file in one chunk */
    if (fread((char *)ptr, filesize, 1, fp) != 1) {
	char *errstring=strerror(ferror(fp));

	medmPrintf(1,"\nloadGIF: Cannot read file:\n"
	  "  %s\n  %s\n",fname,errstring);
	goto CLEANUP;
    }

  /* Parse the Header */
  /* Parse the Signature (3 bytes) and the Version (3 bytes) */
    if (!strncmp((char *)ptr, "GIF89a", 6)) {
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

#if DEBUG_GIF
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
    if (verbose) {
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
	for (i=0; i < ColorTableEntries; i++) {
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
        if (gif->nostrip) {
	  /* nostrip=True */
	  /* KE: Currently nostrip is False */
            j=0;
            lmask=lmasks[gif->strip];
            for (i=0; i<gif->numcols; i++) {
                gif->defs[i].red  =(Red[i]  &lmask)<<8;
                gif->defs[i].green=(Green[i]&lmask)<<8;
                gif->defs[i].blue =(Blue[i] &lmask)<<8;
                gif->defs[i].flags=DoRed | DoGreen | DoBlue;
                if (!XAllocColor(display,gif->theCmap,&gif->defs[i])) { 
                    j++;
		    gif->defs[i].pixel=0xffff;
		}
                gif->cols[i]=gif->defs[i].pixel;
	    }
	    
            if (j) {		/* Failed to pull it off */
                XColor ctab[256];
		
                medmPrintf(1,"\nloadGIF: Failed to allocate %d out of %d colors\n"
		  "  Trying extra hard\n",
		  j,gif->numcols);
                
	      /* Read in the color table */
                for (i=0; i<gif->numcols; i++) ctab[i].pixel=i;
                XQueryColors(display,gif->theCmap,ctab,gif->numcols);
                
                for (i=0; i<gif->numcols; i++) {
		    if (gif->cols[i] == 0xffff) { /* An unallocated pixel */
			int d, mdist, close;
			unsigned long r,g,b;
			
			mdist=100000;   close=-1;
			r= Red[i];
			g= Green[i];
			b= Blue[i];
			for (j=0; j<gif->numcols; j++) {
			    d=abs((int)(r - (ctab[j].red>>8))) +
			      abs((int)(g - (ctab[j].green>>8))) +
			      abs((int)(b - (ctab[j].blue>>8)));
			    if (d<mdist) { mdist=d; close=j; }
			}
			if (close<0) {
			    medmPrintf(1,"loadGIF: "
			     "Simply can't do it -- Sorry\n");
			}
			memcpy( (void*)(&gif->defs[i]),
			  (void*)(&gif->defs[close]),
			  sizeof(XColor));
			gif->cols[i]=ctab[close].pixel;
		    }
		}
	    }
	} else {
          /* nostrip=False, do the best auto-strip */
	  /* KE: Currently nostrip is False, and this is the algorithm used */
            j=0;
            while (gif->strip<8) {
                lmask=lmasks[gif->strip];
                for (i=0; i<gif->numcols; i++) {
                    gif->defs[i].red  =(Red[i]  &lmask)<<8;
                    gif->defs[i].green=(Green[i]&lmask)<<8;
                    gif->defs[i].blue =(Blue[i] &lmask)<<8;
                    gif->defs[i].flags=DoRed | DoGreen | DoBlue;
                    if (!XAllocColor(display,gif->theCmap,&gif->defs[i]))
		      break;
                    gif->cols[i]=gif->defs[i].pixel;
		}
                if (i<gif->numcols) {     /* Failed */
#if DEBUG_GIF
		    print("Auto strip %d (mask %2x) failed for color %d"
		      " [R=%hx G=%hx B=%hx]\n",
		      gif->strip,lmask,i,gif->defs[i].red,gif->defs[i].green,
		      gif->defs[i].blue);
#endif		    
                    gif->strip++;  j++;
                    XFreeColors(display,gif->theCmap,gif->cols,i,0L);
		} else {
		    break;
		}
	    }
	    
            if (j && gif->strip<8) {
		medmPrintf(0,"\nloadGIF: %s was masked to level %d"
		  " (mask 0x%2X)\n",
		  fname,gif->strip,lmask);
		if (verbose)
		  print("loadGIF: %s was masked to level %d (mask 0x%2X)\n",
		    fname,gif->strip,lmask);
	    }
	    
            if (gif->strip==8) {
                medmPrintf(1,"\nloadGIF: "
		  "Failed to allocate the desired colors\n");
                for (i=0; i<gif->numcols; i++) gif->cols[i]=i;
	    }
	}
    } else {
      /* No global colormap in GIF file */
        medmPrintf(0,"\nloadGIF:  No global colortable in %s."
	  "  Making one.\n",fname);
        if (!gif->numcols) gif->numcols=256;
        for (i=0; i < gif->numcols; i++) gif->cols[i]=(unsigned long)i;
    }
    
  /* Set the backgroundcolor index */
    gif->background=gif->cols[BackgroundColorIndex&(gif->numcols-1)];

  /* Determine the number of images */
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

#if DEBUG_GIF
    print("loadGIF: Enlarging images\n");
#endif	
  /* Make full-size images for each frame if necessary.  Make a pixmap
   *   and do the image manipulation with the pixmap rather than
   *   manipulating bytes.  This way is platform independent though it
   *   may be slower */
    for(i=0; i < nFrames; i++) {
#if DEBUG_GIF
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
      /* Don't do anything if the frame is full size */
	if(frames[i]->Height == LogicalScreenHeight &&
	  frames[i]->Width == LogicalScreenWidth) continue;

      /* Create a pixmap if not already created */
	if(!tempPixmap) {
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
	    case 1:     /* Leave in place */
	      /* Copy the previous image */
		XPutImage(display,tempPixmap,
		  gif->theGC,frames[prevFrame]->theImage,
		  0,0,0,0,LogicalScreenWidth,LogicalScreenHeight);
		break;
	    case 0:     /* No action */
	    case 2:     /* Restore to background */
	    case 3:     /* Restore to previous */
	    default:    /* To be defined */
	      /* Fill with display background */
		XSetForeground(display,gif->theGC,gif->bcol);
		XFillRectangle(display,tempPixmap,gif->theGC,
		  0,0,LogicalScreenWidth,LogicalScreenHeight);
		break;
	    }
	}

      /* Copy in the reduced image */
	if(frames[i]->theImage) {
	    XPutImage(display,tempPixmap,
	      gif->theGC,frames[i]->theImage,
	      0,0,frames[i]->LeftOffset,frames[i]->TopOffset,
	      frames[i]->Width,frames[i]->Height);
	  /* Destroy the current theImage */
	    XDestroyImage(frames[i]->theImage);
	}

      /* Define theImage from the temporary pixmap */
	frames[i]->theImage=XGetImage(display,tempPixmap,0,0,
	  LogicalScreenWidth,LogicalScreenHeight,
	  AllPlanes,ZPixmap);
#if DEBUG_GIF
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
	    while(DataBlockSize=NEXTBYTE) {
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
		while(DataBlockSize=NEXTBYTE) {
		    ptr+=DataBlockSize;
		}
		break;
	    case PLAINTEXT:
#if DEBUG_GIF > 1
		print("  PLAINTEXT [%2x]\n",ch);
#endif	    
		BlockSize=NEXTBYTE;     /* Should be 12 */
		ptr+=BlockSize;
		while(DataBlockSize=NEXTBYTE) {
		    ptr+=DataBlockSize;
		}
		break;
	    case APPLICATION:           /* Should be 11 */
#if DEBUG_GIF > 1
		print("  APPLICATION [%2x]\n",ch);
#endif	    
		BlockSize=NEXTBYTE;
		ptr+=BlockSize;
		while(DataBlockSize=NEXTBYTE) {
		    ptr+=DataBlockSize;
		}
		break;
	    case COMMENT:
#if DEBUG_GIF > 1
		print("  COMMENT [%2x]\n",ch);
#endif	    
	      /* There is no block before the data here */
		while(DataBlockSize=NEXTBYTE) {
		    ptr+=DataBlockSize;
		}
		break;
	    default:
#if DEBUG_GIF > 1
		print("  UNKNOWN [%2x]\n",ch);
#endif	    
		medmPrintf(0,"\ncountImages: Unknown extension [0x%x] for %s\n",
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
	    medmPrintf(0,"\ncountImages: Unknown data separator [0x%x] for %s\n",
	      ch,fname);
	    nFrames=-1;
	    done=True;
	    break;
	}
    }

  /* Return */
    if(!done) {
	medmPrintf(0,"\nloadGIF: Terminator not found for %s\n",fname);
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
	ptr+=3*LocalColorTableEntries;
    }

    if (verbose) {
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
    
  /* The GIF spec has it that the code size is the code size used to
   * compute the above values is the code size given in the file, but the
   * code size used in compression/decompression is the code size given in
   * the file plus one. (thus the ++).
   */
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
	while (ch--) *ptr1++=NEXTBYTE;
	if ((Raster - ptr1) > filesize){
	    medmPrintf(1,"\nparseGIFImage: "
	      "Trying to read past end of file for %s\n",fname);
	}
    } while(ch1);     /* Continue until block terminator */

    if (verbose)
      print("parseGIFImage: %s decompressing...\n",fname);

/* Allocate the X Image */
    switch (ScreenDepth) {
    case 8:
        BytesOffsetPerPixel=1;
	ImageDataSize=Width*Height;
        Image=(Byte *)malloc(ImageDataSize);
        if (!Image) {
	    medmPrintf(1,"\nparseGIFImage: Not enough memory for XImage"
	      " for %s\n",fname);
	    return(False);
        }
        CURIMAGE(gif)=XCreateImage(display,gif->theVisual,
	  ScreenDepth,ZPixmap,0,
	  (char*)Image,Width,Height,32,0);
        break;
    case 24:
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
        if (!Image) {
	    medmPrintf(1,"\nparseGIFImage: "
	      "Not enough memory for XImage for %s\n",
	      fname);
	    return(False);
        }
        CURIMAGE(gif)=XCreateImage(display,gif->theVisual,
	  ScreenDepth,ZPixmap,0,
	  (char*)Image,Width,Height,32,0);
        break;
    }
    if (!CURIMAGE(gif)) {
	medmPrintf(1,"\nparseGIFImage: Unable to create XImage for %s\n",fname);
	return(False);
    }
    BytesPerScanline=CURIMAGE(gif)->bytes_per_line;

  /* Decompress the file, continuing until we see the GIF EOF code or
   * we use up all the bytes in the data */
    Code=readCode();
    while (Code != EOFCode && BitOffset/8 < rasterBytes) {
      /* Clear code sets everything back to its initial value, then reads the
       * immediately subsequent code as uncompressed data.
       */
	if (Code == ClearCode) {
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
	    if (CurCode >= FreeCode) {
		CurCode=OldCode;
		OutCode[OutCount++]=FinChar;
	    }
	    
	  /* Unless this code is raw data, pursue the chain pointed to by CurCode
	   * through the hash table to its end; each code in the chain puts its
	   * associated output code on the output queue.
	   */
	    while (CurCode > BitMask) {
		if (OutCount > 1024){
		    medmPrintf(1,"\nparseGIFImage: Corrupt GIF file (OutCount)\n");
		    XDestroyImage(CURIMAGE(gif));
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
	    for (i=OutCount - 1; i >= 0; i--)
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
	    if (FreeCode >= MaxCode) {
		if (CodeSize < 12) {
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
	while(DataBlockSize=NEXTBYTE) {
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
	while(DataBlockSize=NEXTBYTE) {
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
	while(DataBlockSize=NEXTBYTE) {
	    ptr+=DataBlockSize;
	}
	break;
    case COMMENT:
#if DEBUG_GIF
	print("  COMMENT [%2x]\n",ch);
#endif	    
      /* Ignore (There is no block before the data here) */
	while(DataBlockSize=NEXTBYTE) {
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
	    XFreeColors(display,gif->theCmap,gif->cols,gif->numcols,(unsigned long)0);
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
			free(frame->expImage->data);
			frame->expImage->data=NULL;
			XDestroyImage((XImage *)(frame->expImage));
			frame->expImage=NULL;
		    }
		    if(frame->theImage) {
			free(frame->theImage->data);
			frame->theImage->data=NULL;
			XDestroyImage((XImage *)(frame->theImage));
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
	gif=NULL;
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

    gif1=(GIFData *)dlImage1->privateData;
    if(!gif1) {
	gif2=NULL;
	dlImage2->privateData=gif2;
	return;
    }
    if (!(gif2=(GIFData *)malloc(sizeof(GIFData)))) {
	medmPrintf(1,"\ncopyGIF: Memory allocation error:\n"
	  "  %s\n",gif1->imageName);
	dlImage2->privateData=gif2;
	return;
    }
    *gif2=*gif1;
    
  /* Reallocate the colors, in case they are freed elsewhere */
    for (i=0; i < gif2->numcols; i++) {
	if (!XAllocColor(display,gif2->theCmap,&gif2->defs[i])) { 
	    gif2->defs[i].pixel=0xffff;
	}
    }
    
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
		  0,(char *)ximag,gif1->iWIDE,gif1->iHIGH,32,0);
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
		  0,(char *)ximag,gif1->eWIDE,gif1->eHIGH,32,0);
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

/* Function to determine byte order on the client  */
static int getClientByteOrder()
{
    short i=1;
    
    return (*(char*)&i == 1) ? LSBFirst : MSBFirst;
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
    if (CodeSize >= 8)
      RawCode+=(0x10000*Raster[ByteOffset+2]);
#if 0
    print("readCode: XC=%d YC=%d BitOffset=%d ByteOffset=%d Rawcode=%x",
      XC,YC,BitOffset,ByteOffset,RawCode);
#endif    
    RawCode>>=(BitOffset%8);
#if 0
    print("->%x ReadMask=%x RawCode&ReadMask=%x\n",
      RawCode,ReadMask,RawCode&ReadMask);
#endif    
    BitOffset+=CodeSize;
    return(RawCode&ReadMask);
}

static void dumpGIF(GIFData *gif)
{
    int i;
    for (i=0; i<32; i++) {
	if (i && ((i>>4)<<4) == i) {
	    print("\n");
	}
	print("%02x ",(Byte)CURIMAGE(gif)->data[i]);
    }
    print("\n");
}

static void addToPixel(GIFData *gif, Byte Index)
{
    Pixel clr;
    int offset=YC*BytesPerScanline+XC;
	
  /* Check that we are in bounds and write to the XImage data */
    if(offset < ImageDataSize) {
	Byte *p=(Byte *)Image+offset;
	
	switch (ScreenDepth) {
	case 8:
	  /* Check for transparent color */
	    if(TransparentColorFlag && (Index&(gif->numcols-1)) ==
	      TransparentIndex) {
		*p=(Byte)gif->bcol;
	    } else {
		*p=(Byte)gif->cols[Index&(gif->numcols-1)];
	    }
	    break;
	case 24:
	  /* Check for transparent color */
	    if(TransparentColorFlag && (Index&(gif->numcols-1)) ==
	      TransparentIndex) {
		clr=gif->bcol;
	    } else {
		clr=gif->cols[Index&(gif->numcols-1)];
	    }
	    if (BytesOffsetPerPixel == 4) {
		*((Pixel *)p)=clr;
	    } else {
		*p++=(Byte)((clr & 0x00ff0000) >> 16);
		*p++=(Byte)((clr & 0x0000ff00) >> 8);
		*p=(Byte)((clr & 0x000000ff));
	    }
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
	if (!Interlace) {
	    YC++;
	} else {
	    switch (Pass) {
	    case 0:
		YC += 8;
		if (YC >= Height) {
		    Pass++;
		    YC=4;
		}
		break;
	    case 1:
		YC += 8;
		if (YC >= Height) {
		    Pass++;
		    YC=2;
		}
		break;
	    case 2:
		YC += 4;
		if (YC >= Height) {
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
