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

/*
 * definition of image-related data types for GIF images
 */

#ifndef __XGIF_H__
#define __XGIF_H__

#define CURFRAME(gif) \
  (((gif)->curFrame >= 0 && (gif)->curFrame < (gif)->nFrames) ? \
  (gif)->curFrame : 0)
#define CURIMAGE(gif) \
  (gif->frames[CURFRAME(gif)]->theImage)
#define CUREXPIMAGE(gif) \
  (gif->frames[CURFRAME(gif)]->expImage)

typedef unsigned char Byte;

typedef struct {
    XImage        *theImage;
    XImage        *expImage;
    int           Height;
    int           Width;
    int           TopOffset;
    int           LeftOffset;
    Byte          TransparentColorFlag;
    Byte          TransparentIndex;
    int           DisposalMethod;
    int           DelayTime;
} FrameData;

typedef struct {
    char          imageName[MAX_TOKEN_LENGTH];
    int           displayCells;
    Colormap      theCmap;
    GC            theGC;
    Pixel         fcol,bcol;
    Pixel         background;
    Font          mfont;
    XFontStruct   *mfinfo;
    Visual        *theVisual;
    int           iWIDE;
    int           iHIGH;
    int           eWIDE;
    int           eHIGH;
    int           numcols;
    int           strip;
    int           nostrip;
    unsigned long cols[256];
    XColor        defs[256];
    unsigned int  currentWidth;
    unsigned int  currentHeight;
    int           nFrames;
    int           curFrame;
    FrameData     **frames;
} GIFData;

#endif  /* __XGIF_H__ */
