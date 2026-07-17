/******************************************************************************
* In order to make life a little bit easier when using the GIF file format,   *
* this library was written, and which does all the dirty work...	      *
*									      *
*					Written by Gershon Elber,  Jun. 1989  *
*******************************************************************************
* History:								      *
* 14 Jun 89 - Version 1.0 by Gershon Elber.				      *
******************************************************************************/

#ifndef GIF_LIB_H
#define GIF_LIB_H

#include "gxcore.h"

__BEGIN_DECLS

#ifndef ERROR
	#define	ERROR		0
#endif

#ifndef OK
	#define OK		1
#endif

#ifndef TRUE
#define TRUE		1
#define FALSE		0
#endif

#define FILE_BUFFER_SIZE 16384	     /* Files uses bigger buffers than usual */

typedef	int		BooleanType;
typedef	unsigned char	PixelType;
typedef unsigned char *	RowType;
typedef unsigned char	ByteType;

#define MESSAGE(s)	gxlogf (s)
#define EXIT(s)		{ MESSAGE(s); exit(-3); }

typedef struct GifColorType {
    ByteType Red, Green, Blue;
} GifColorType;

/* Note entries prefixed with S are of Screen information, while entries     */
/* prefixed with I are of the current defined Image.			     */
typedef struct GifFileType {
    int SWidth, SHeight,				/* Screen dimensions */
	SColorResolution, SBitsPerPixel, /* How many colors can we generate? */
	SBackGroundColor,		/* I hope you understand this one... */
	ILeft, ITop, IWidth, IHeight,		 /* Current image dimensions */
	IInterlace,			      /* Sequential/Interlaced lines */
	IBitsPerPixel;			  /* How many colors this image has? */
    GifColorType *SColorMap, *IColorMap;	       /* NULL if not exists */
    void *Private;	  /* The regular user should not mesh with this one! */
} GifFileType;

typedef enum {
    UNDEFINED_RECORD_TYPE,
    SCREEN_DESC_RECORD_TYPE,
    IMAGE_DESC_RECORD_TYPE,				   /* Begin with ',' */
    EXTENSION_RECORD_TYPE,				   /* Begin with '!' */
    TERMINATE_RECORD_TYPE				   /* Begin with ';' */
} GifRecordType;

/******************************************************************************
* O.k. here are the routines one can access in order to encode GIF file:      *
* (GIF_LIB file EGIF_LIB.C).						      *
******************************************************************************/

GifFileType *EGifOpenFileName(char *GifFileName, int GifTestExistance);
GifFileType *EGifOpenFileHandle(int GifFileHandle);
int EGifPutScreenDesc(GifFileType *GifFile,
	int GifWidth, int GifHeight, int GifColorRes, int GifBackGround,
	int GifBitsPerPixel, GifColorType *GifColorMap);
int EGifPutImageDesc(GifFileType *GifFile,
	int GifLeft, int GifTop, int Width, int GifHeight, int GifInterlace,
	int GifBitsPerPixel, GifColorType *GifColorMap);
int EGifPutLine(GifFileType *GifFile, PixelType *GifLine, int GifLineLen);
int EGifPutPixel(GifFileType *GifFile, PixelType GifPixel);
int EGifPutComment(GifFileType *GifFile, char *GifComment);
int EGifPutExtension(GifFileType *GifFile, int GifExtCode, int GifExtLen,
							void *GifExtension);
int EGifPutCode(GifFileType *GifFile, int GifCodeSize, ByteType *GifCodeBlock);
int EGifPutCodeNext(GifFileType *GifFile, ByteType *GifCodeBlock);
int EGifCloseFile(GifFileType *GifFile);

#define	E_GIF_ERR_OpenFailed	1		 /* And EGif possible errors */
#define	E_GIF_ERR_WriteFailed	2
#define E_GIF_ERR_HasScrnDscr	3
#define E_GIF_ERR_HasImagDscr	4
#define E_GIF_ERR_NoColorMap	5
#define E_GIF_ERR_DataTooBig	6
#define E_GIF_ERR_NotEnoughMem	7
#define E_GIF_ERR_DiskIsFull	8
#define E_GIF_ERR_CloseFailed	9
#define E_GIF_ERR_NotWriteable	10

/******************************************************************************
* O.k. here are the routines one can access in order to decode GIF file:      *
* (GIF_LIB file DGIF_LIB.C).						      *
******************************************************************************/

GifFileType *gif_header_parse(char *pData);
GifFileType *GxDGifOpenFileName(char *GifFileName);
GifFileType *GxDGifOpenFileHandle(int GifFileHandle);
int GxDGifGetScreenDesc(GifFileType *GifFile);
int GifGetRecordType(GifFileType * GifFile, GifRecordType * Type);
int GxDGifGetRecordType(GifFileType *GifFile, GifRecordType *GifType);
int GifGetImageDesc(GifFileType * GifFile);
int GxDGifGetImageDesc(GifFileType *GifFile);
int GifGetLine(GifFileType * GifFile, PixelType * Line, int LineLen);
int GxDGifGetLine(GifFileType *GifFile, PixelType *GifLine, int GifLineLen);
int GxDGifGetPixel(GifFileType *GifFile, PixelType GifPixel);
int GxDGifGetComment(GifFileType *GifFile, char *GifComment);
int GifGetExtension(GifFileType * GifFile, int *ExtCode, ByteType ** Extension);
int GxDGifGetExtension(GifFileType *GifFile, int *GifExtCode,
						ByteType **GifExtension);
int GifGetExtensionNext(GifFileType * GifFile, ByteType ** Extension);
int GxDGifGetExtensionNext(GifFileType *GifFile, ByteType **GifExtension);
int GxDGifGetCode(GifFileType *GifFile, int *GifCodeSize,
						ByteType **GifCodeBlock);
int GifGetCodeNext(GifFileType * GifFile, ByteType ** CodeBlock);
int GxDGifGetCodeNext(GifFileType *GifFile, ByteType **GifCodeBlock);
int GxDGifGetLZCodes(GifFileType *GifFile, int *GifCode);
int GxDGifCloseFile(GifFileType *GifFile);

#define	D_GIF_ERR_OpenFailed	101		 /* And GxDGif possible errors */
#define	D_GIF_ERR_ReadFailed	102
#define	D_GIF_ERR_NotGifFile	103
#define D_GIF_ERR_NoScrnDscr	104
#define D_GIF_ERR_NoImagDscr	105
#define D_GIF_ERR_NoColorMap	106
#define D_GIF_ERR_WrongRecord	107
#define D_GIF_ERR_DataTooBig	108
#define D_GIF_ERR_NotEnoughMem	109
#define D_GIF_ERR_CloseFailed	110
#define D_GIF_ERR_NotReadable	111
#define D_GIF_ERR_ImageDefect	112
#define D_GIF_ERR_EOFTooSoon	113

/******************************************************************************
* O.k. here are the routines from GIF_LIB file GIF_ERR.C.		      *
******************************************************************************/
void PrintGifError(void);
int GifLastError(void);

/******************************************************************************
* O.k. here are the routines from GIF_LIB file DEV2GIF.C.		      *
******************************************************************************/
int DumpScreen(char *FileName, int ReqGraphDriver, int ReqGraphMode);


__END_DECLS

#endif /*GIF_LIB_H*/



