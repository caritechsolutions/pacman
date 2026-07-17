/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber			IBM PC Ver 0.1,	Jun. 1989    *
******************************************************************************
* The kernel of the GIF Decoding process can be found here.		     *
******************************************************************************
* History:								     *
* 16 Jun 89 - Version 1.0 by Gershon Elber.				     *
*****************************************************************************/

//#include <sys/io.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
//#include <alloc.h>
#include <string.h>
#include <sys/stat.h>
#include "gui_core.h"
#include "gif_lib.h"
#include "gif_hash.h"

#define PROGRAM_NAME	"GIF_LIBRARY"
#define VERSION		"?Version 1.0, "

#define COMMENT_EXT_FUNC_CODE	'C'	/* Extension function code for comment */
#define GIF_STAMP	"GIF87a"	/* First chars in file - GIF stamp */
#define GIF_STAMP_LEN	sizeof(GIF_STAMP) - 1

#define LZ_MAX_CODE	4095	/* Biggest code possible in 12 bits. */
#define LZ_BITS		12

#define FILE_STATE_READ		0x01	/* 1 write, 0 read - EGIF_LIB compatible */

#define FLUSH_OUTPUT		4096	/* Impossible code, to signal flush */
#define FIRST_CODE		4097	/* Impossible code, to signal first */
#define NO_SUCH_CODE		4098	/* Impossible code, to signal empty */

#define IS_READABLE(Private)	(!(Private -> FileState & FILE_STATE_READ))

typedef struct GifFilePrivateType {
	int FileState, FileHandle,	/* Where all this data goes to! */
	 BitsPerPixel,		/* Bits per pixel (Codes uses at list this + 1) */
	 ClearCode,		/* The CLEAR LZ code */
	 EOFCode,		/* The EOF LZ code */
	 RunningCode,		/* The next code algorithm can generate */
	 RunningBits,		/* The number of bits required to represent RunningCode */
	 MaxCode1,		/* 1 bigger than maximum possible code, in RunningBits bits */
	 LastCode,		/* The code before the current code */
	 CrntCode,		/* Current algorithm code */
	 StackPtr,		/* For character stack (see below) */
	 CrntShiftState;	/* Number of bits in CrntShiftDWord */
	unsigned long CrntShiftDWord,	/* For bytes decomposition into codes */
	 PixelCount;		/* Number of pixels in image */
	FILE *File;		/* File as stream */
	char *data;
	ByteType Buf[256];	/* Compressed input is buffered here */
	ByteType Stack[LZ_MAX_CODE];	/* Decoded pixels are stacked here */
	ByteType Suffix[LZ_MAX_CODE + 1];	/* So we can trace the codes */
	unsigned int Prefix[LZ_MAX_CODE + 1];
} GifFilePrivateType;

extern int _GxGifError;

/*static char *VersionStr =
	PROGRAM_NAME
	"	IBMPC "
	VERSION
	"	Gershon Elber,	"
	__DATE__ ",   " __TIME__ "\n"
	"(C) Copyright 1989 Gershon Elber, Non commercial use only.\n";*/

static int GifGetWord(char * Data, int *Word);
static int GxDGifGetWord(FILE * File, int *Word);
static int GxDGifSetupDecompress(GifFileType * GifFile);
static int GifDecompressLine(GifFileType * GifFile, PixelType * Line, int LineLen);
static int GxDGifDecompressLine(GifFileType * GifFile, PixelType * Line, int LineLen);
static int GxDGifGetPrefixChar(unsigned int *Prefix, int Code, int ClearCode);
static int GifSetupDecompress(GifFileType * GifFile);
static int GifDecompressInput(GifFilePrivateType * Private, int *Code);
static int GxDGifDecompressInput(GifFilePrivateType * Private, int *Code);
static int GxDGifBufferedInput(FILE * File, ByteType * Buf, ByteType * NextByte);
static int GifBufferedInput(GifFilePrivateType * Private, ByteType * Buf, ByteType * NextByte);

/******************************************************************************
*   Open a new gif file for read, given by its name.			      *
*   Returns GifFileType pointer dynamically allocated which serves as the gif *
* info record. _GxGifError is cleared if succesfull.			      *
******************************************************************************/
GifFileType *GxDGifOpenFileName(char *FileName)
{
	int FileHandle;

	if ((FileHandle = open(FileName, O_RDONLY /* | O_BINARY */ )) == -1) {
		_GxGifError = D_GIF_ERR_OpenFailed;
		return NULL;
	}

	return GxDGifOpenFileHandle(FileHandle);
}


/******************************************************************************
*   Update a new gif file, given its file handle.			      *
*   Returns GifFileType pointer dynamically allocated which serves as the gif *
* info record. _GxGifError is cleared if succesfull.			      *
******************************************************************************/
GifFileType *GxDGifOpenFileHandle(int FileHandle)
{
	char Buf[GIF_STAMP_LEN + 1];
	GifFileType *GifFile;
	GifFilePrivateType *Private;
	FILE *f;

	//setmode(FileHandle, O_BINARY);       /* Make sure it is in binary mode */
	f = fdopen(FileHandle, "rb");	/* Make it into a stream: */
	setvbuf(f, NULL, _IOFBF, FILE_BUFFER_SIZE);	/* And increase stream buffer */

	if ((GifFile = (GifFileType *) GUI_MALLOC(sizeof(GifFileType))) == NULL) {
		_GxGifError = D_GIF_ERR_NotEnoughMem;
		return NULL;
	}

	if ((Private = (GifFilePrivateType *)GUI_MALLOC(sizeof(GifFilePrivateType)))
	    == NULL) {
		_GxGifError = D_GIF_ERR_NotEnoughMem;
		GUI_FREE(GifFile);
		return NULL;
	}
	GifFile->Private = (void *)Private;
	GifFile->SColorMap = GifFile->IColorMap = NULL;
	Private->FileHandle = FileHandle;
	Private->File = f;
	Private->FileState = 0;	/* Make sure bit 0 = 0 (File opened for read) */

	/* Lets see if this is GIF file: */
	if (fread(Buf, 1, GIF_STAMP_LEN, Private->File) != GIF_STAMP_LEN) {
		_GxGifError = D_GIF_ERR_ReadFailed;
		GUI_FREE(Private);
		GUI_FREE(GifFile);
		return NULL;
	}

	Buf[GIF_STAMP_LEN] = 0;
	if ((strcmp("GIF89a", Buf) != 0) && (strcmp("GIF87a", Buf) != 0)) {
		_GxGifError = D_GIF_ERR_NotGifFile;
		GUI_FREE(Private);
		GUI_FREE(GifFile);
		return NULL;
	}

	if (GxDGifGetScreenDesc(GifFile) == ERROR) {
		GUI_FREE(Private);
		GUI_FREE(GifFile);
		return NULL;
	}

	_GxGifError = 0;

	return GifFile;
}

int GifGetScreenDesc(GifFileType *GifFile)
{
	int Size, i;
	ByteType Buf[3];
	GifFilePrivateType *Private = (GifFilePrivateType *)(GifFile->Private);

	/* Put the screen descriptor into the file: */
	if(GifGetWord(Private->data, &GifFile->SWidth) == ERROR)
	{
		return (ERROR);
	}

	Private->data += 2;
	if(GifGetWord(Private->data, &GifFile->SHeight) == ERROR)
	{
		return (ERROR);
	}
	Private->data += 2;

	memcpy(Buf, Private->data, 3);
	GifFile->SColorResolution = (((Buf[0] & 0x70) + 1) >> 4) + 1;
	GifFile->SBitsPerPixel = (Buf[0] & 0x07) + 1;
	GifFile->SBackGroundColor = Buf[1];
	Private->data += 3;

	/* Do we have global color map? */
	if (Buf[0] & 0x80)
	{
		Size = (1 << GifFile->SBitsPerPixel);
		GifFile->SColorMap = (GifColorType *) GUI_MALLOC(sizeof(GifColorType) * Size);
		for (i=0; i<Size; i++)
		{
			memcpy(Buf, Private->data, 3);
			Private->data += 3;
			_GxGifError = D_GIF_ERR_ReadFailed;

			GifFile->SColorMap[i].Red = Buf[0];
			GifFile->SColorMap[i].Green = Buf[1];
			GifFile->SColorMap[i].Blue = Buf[2];
		}
	}

	return OK;

}

GifFileType *gif_header_parse(char *pData)
{
	char Buf[7] = {0};
	GifFileType *GifFile;
	GifFilePrivateType *Private;

	if ((GifFile = (GifFileType *) GUI_MALLOCZ(sizeof(GifFileType))) == NULL)
	{
		return (NULL);
	}

	if ((Private = (GifFilePrivateType *) GUI_MALLOCZ(sizeof(GifFilePrivateType))) == NULL)
	{
		return (NULL);
	}

	GifFile->Private = (void *)Private;
	GifFile->SColorMap = GifFile->IColorMap = NULL;
	Private->FileHandle = 0;
	Private -> File = 0;
	Private->FileState = 0; /* Make sure bit 0 = 0 (File opened for read) */

	/* Lets see if this is GIF file: */
	memcpy(Buf, pData, GIF_STAMP_LEN);

	Buf[GIF_STAMP_LEN] = '\0';

	if ((strcmp("GIF89a", Buf) != 0) && (strcmp("GIF87a", Buf) != 0))
	{
		gxlogd("The file is not GIF!\n");
		return (NULL);
	}

	Private->data = pData + strlen(Buf);
	if (GifGetScreenDesc(GifFile) == ERROR)
	{
		GUI_FREE(Private);
		GUI_FREE(GifFile);
		return NULL;
	}
	_GxGifError	 = 0;

	return (GifFile);
}

/******************************************************************************
*   This routine should be called before any other GxDGif calls. Note that      *
* this routine is called automatically from GxDGif file open routines.	      *
******************************************************************************/
int GxDGifGetScreenDesc(GifFileType * GifFile)
{
	int Size, i;
	ByteType Buf[3];
	GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

	if (!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		_GxGifError = D_GIF_ERR_NotReadable;
		return ERROR;
	}

	/* Put the screen descriptor into the file: */
	if (GxDGifGetWord(Private->File, &GifFile->SWidth) == ERROR ||
	    GxDGifGetWord(Private->File, &GifFile->SHeight) == ERROR)
		return ERROR;

	if (fread(Buf, 1, 3, Private->File) != 3) {
		_GxGifError = D_GIF_ERR_ReadFailed;
		return ERROR;
	}
	GifFile->SColorResolution = (((Buf[0] & 0x70) + 1) >> 4) + 1;
	GifFile->SBitsPerPixel = (Buf[0] & 0x07) + 1;
	GifFile->SBackGroundColor = Buf[1];
	if (Buf[0] & 0x80) {	/* Do we have global color map? */
		Size = (1 << GifFile->SBitsPerPixel);
		GifFile->SColorMap = (GifColorType *) GUI_MALLOC(sizeof(GifColorType) * Size);
		for (i = 0; i < Size; i++) {	/* Get the global color map: */
			if (fread(Buf, 1, 3, Private->File) != 3) {
				_GxGifError = D_GIF_ERR_ReadFailed;
				return ERROR;
			}
			GifFile->SColorMap[i].Red = Buf[0];
			GifFile->SColorMap[i].Green = Buf[1];
			GifFile->SColorMap[i].Blue = Buf[2];
		}
	}

	return OK;
}

int GifGetRecordType(GifFileType * GifFile, GifRecordType * Type)
{
	ByteType Buf;
	GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

	Buf = *Private->data;
	Private->data++;

	switch(Buf)
	{
		case ',':
			*Type = IMAGE_DESC_RECORD_TYPE;
		break;
		case '!':
			*Type = EXTENSION_RECORD_TYPE;
		break;
		case ';':
			*Type = TERMINATE_RECORD_TYPE;
		break;
		default:
			*Type = UNDEFINED_RECORD_TYPE;
			_GxGifError = D_GIF_ERR_WrongRecord;
		return ERROR;
	}

	return OK;
}


/******************************************************************************
*   This routine should be called before any attemp to read an image.         *
******************************************************************************/
int GxDGifGetRecordType(GifFileType * GifFile, GifRecordType * Type)
{
	ByteType Buf;
	GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

	if (!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		_GxGifError = D_GIF_ERR_NotReadable;
		return ERROR;
	}

	if (fread(&Buf, 1, 1, Private->File) != 1) {
		_GxGifError = D_GIF_ERR_ReadFailed;
		return ERROR;
	}

	switch (Buf) {
	case ',':
		*Type = IMAGE_DESC_RECORD_TYPE;
		break;
	case '!':
		*Type = EXTENSION_RECORD_TYPE;
		break;
	case ';':
		*Type = TERMINATE_RECORD_TYPE;
		break;
	default:
		*Type = UNDEFINED_RECORD_TYPE;
		_GxGifError = D_GIF_ERR_WrongRecord;
		return ERROR;
	}

	return OK;
}

int GifGetImageDesc(GifFileType * GifFile)
{
	int Size, i;
	ByteType Buf[3];
	GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

	if(ERROR == GifGetWord(Private->data, &GifFile->ILeft))
	{
		return (ERROR);
	}
	Private->data += 2;
	if(ERROR == GifGetWord(Private->data, &GifFile->ITop))
	{
		return (ERROR);
	}
	Private->data += 2;
	if(ERROR == GifGetWord(Private->data, &GifFile->IWidth))
	{
		return (ERROR);
	}
	Private->data += 2;
	if(ERROR == GifGetWord(Private->data, &GifFile->IHeight))
	{
		return (ERROR);
	}
	Private->data += 2;

	Buf[0] = *Private->data;
	Private->data++;

	GifFile->IBitsPerPixel = (Buf[0] & 0x07) + 1;
	GifFile->IInterlace = (Buf[0] & 0x40);
	/* Does this image have local color map? */
	if(Buf[0] & 0x80)
	{
		Size = (1 << GifFile->IBitsPerPixel);
		if(GifFile->IColorMap)
		{
			GUI_FREE(GifFile->IColorMap);
		}

		GifFile->IColorMap = (GifColorType *) GUI_MALLOC(sizeof(GifColorType) * Size);
		for(i = 0; i < Size; i++)
		{
			memcpy(Buf, Private->data, 3);
			Private->data += 3;

			GifFile->IColorMap[i].Red = Buf[0];
			GifFile->IColorMap[i].Green = Buf[1];
			GifFile->IColorMap[i].Blue = Buf[2];
		}
	} else {
		if(GifFile->IColorMap) {
			GUI_FREE(GifFile->IColorMap);
		}
	}

	Private->PixelCount = (long)GifFile->IWidth * (long)GifFile->IHeight;

	GifSetupDecompress(GifFile);	/* Reset decompress algorithm parameters */

	return (OK);
}

/******************************************************************************
*   This routine should be called before any attemp to read an image.         *
*   Note it is assumed the Image desc. header (',') has been read.	      *
******************************************************************************/
int GxDGifGetImageDesc(GifFileType * GifFile)
{
	int Size, i;
	ByteType Buf[3];
	GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

	if (!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		_GxGifError = D_GIF_ERR_NotReadable;
		return ERROR;
	}

	if (GxDGifGetWord(Private->File, &GifFile->ILeft) == ERROR ||
	    GxDGifGetWord(Private->File, &GifFile->ITop) == ERROR ||
	    GxDGifGetWord(Private->File, &GifFile->IWidth) == ERROR ||
	    GxDGifGetWord(Private->File, &GifFile->IHeight) == ERROR)
		return ERROR;
	if (fread(Buf, 1, 1, Private->File) != 1) {
		_GxGifError = D_GIF_ERR_ReadFailed;
		return ERROR;
	}
	GifFile->IBitsPerPixel = (Buf[0] & 0x07) + 1;
	GifFile->IInterlace = (Buf[0] & 0x40);
	if (Buf[0] & 0x80) {	/* Does this image have local color map? */
		Size = (1 << GifFile->IBitsPerPixel);
		if (GifFile->IColorMap)
			GUI_FREE(GifFile->IColorMap);
		GifFile->IColorMap = (GifColorType *) GUI_MALLOC(sizeof(GifColorType) * Size);
		for (i = 0; i < Size; i++) {	/* Get the image local color map: */
			if (fread(Buf, 1, 3, Private->File) != 3) {
				_GxGifError = D_GIF_ERR_ReadFailed;
				return ERROR;
			}
			GifFile->IColorMap[i].Red = Buf[0];
			GifFile->IColorMap[i].Green = Buf[1];
			GifFile->IColorMap[i].Blue = Buf[2];
		}
	}

	Private->PixelCount = (long)GifFile->IWidth * (long)GifFile->IHeight;

	GxDGifSetupDecompress(GifFile);	/* Reset decompress algorithm parameters */

	return OK;
}

int GifGetLine(GifFileType * GifFile, PixelType * Line, int LineLen)
{
	ByteType *Dummy;
	GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

	if(0 == LineLen)
	{
		LineLen = GifFile->IWidth;
	}
	if((Private->PixelCount -= LineLen) < 0)
	{
		return (ERROR);
	}

	if(OK == GifDecompressLine(GifFile, Line, LineLen))
	{
		if(Private->PixelCount == 0)
		{
			/* We probably would not be called any more, so lets clean       */
			/* everything before we return: need to flush out all rest of    */
			/* image until empty block (size 0) detected. We use GetCodeNext */
			do
			{
				if (GifGetCodeNext(GifFile, &Dummy) == ERROR)
				{
					return ERROR;
				}
			}while (Dummy != NULL) ;
		}
		return OK;
	}
	else
	{
		return ERROR;
	}
}

/******************************************************************************
*  Get one full scanned line (Line) of length LineLen from GIF file.	      *
******************************************************************************/
int GxDGifGetLine(GifFileType * GifFile, PixelType * Line, int LineLen)
{
	ByteType *Dummy;
	GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

	if (!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		_GxGifError = D_GIF_ERR_NotReadable;
		return ERROR;
	}

	if (!LineLen)
		LineLen = GifFile->IWidth;
	if ((Private->PixelCount -= LineLen) < 0) {
		_GxGifError = D_GIF_ERR_DataTooBig;
		return ERROR;
	}

	if (GxDGifDecompressLine(GifFile, Line, LineLen) == OK) {
		if (Private->PixelCount == 0) {
			/* We probably would not be called any more, so lets clean       */
			/* everything before we return: need to flush out all rest of    */
			/* image until empty block (size 0) detected. We use GetCodeNext */
			do
				if (GxDGifGetCodeNext(GifFile, &Dummy) == ERROR)
					return ERROR;
			while (Dummy != NULL) ;
		}
		return OK;
	} else
		return ERROR;
}

/******************************************************************************
* Put one pixel (Pixel) into GIF file.					      *
******************************************************************************/
int GxDGifGetPixel(GifFileType * GifFile, PixelType Pixel)
{
	ByteType *Dummy;
	GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

	if (!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		_GxGifError = D_GIF_ERR_NotReadable;
		return ERROR;
	}

	if (--Private->PixelCount < 0) {
		_GxGifError = D_GIF_ERR_DataTooBig;
		return ERROR;
	}

	if (GxDGifDecompressLine(GifFile, &Pixel, 1) == OK) {
		if (Private->PixelCount == 0) {
			/* We probably would not be called any more, so lets clean       */
			/* everything before we return: need to flush out all rest of    */
			/* image until empty block (size 0) detected. We use GetCodeNext */
			do
				if (GxDGifGetCodeNext(GifFile, &Dummy) == ERROR)
					return ERROR;
			while (Dummy != NULL) ;
		}
		return OK;
	} else
		return ERROR;
}

int GifGetExtension(GifFileType * GifFile, int *ExtCode, ByteType ** Extension)
{
	ByteType Buf;
	GifFilePrivateType *Private = NULL;

	if(NULL == GifFile || NULL == ExtCode || NULL == Extension)
	{
		return ERROR;
	}

	Private = (GifFilePrivateType *)GifFile->Private;
	Buf = *Private->data;
	Private->data++;
	*ExtCode = Buf;

	return (GifGetExtensionNext(GifFile, Extension));
}

/******************************************************************************
*   Get an extension block (see GIF manual) from gif file. This routine only  *
* returns the first data block, and GxDGifGetExtensionNext shouldbe called      *
* after this one until NULL extension is returned.			      *
*   The Extension should NOT be freed by the user (not dynamically allocated).*
*   Note it is assumed the Extension desc. header ('!') has been read.	      *
******************************************************************************/
int GxDGifGetExtension(GifFileType * GifFile, int *ExtCode, ByteType ** Extension)
{
	ByteType Buf;
	GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

	if (!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		_GxGifError = D_GIF_ERR_NotReadable;
		return ERROR;
	}

	if (fread(&Buf, 1, 1, Private->File) != 1) {
		_GxGifError = D_GIF_ERR_ReadFailed;
		return ERROR;
	}
	*ExtCode = Buf;

	return GxDGifGetExtensionNext(GifFile, Extension);
}

int GifGetExtensionNext(GifFileType * GifFile, ByteType ** Extension)
{
	ByteType Buf;
	GifFilePrivateType *Private = NULL;

	if(NULL == GifFile || NULL == Extension)
	{
		return (ERROR);
	}

	Private = (GifFilePrivateType *)(GifFile->Private);
	Buf = *Private->data;
	Private->data++;

	if(Buf > 0)
	{
		*Extension = Private->Buf;
		(*Extension)[0] = Buf;
		//(*Extension)[1] = *Private->data;
		memcpy(&((*Extension)[1]), Private->data, Buf);
		Private->data += Buf;
	}
	else
	{
		*Extension = NULL;
	}

	return (OK);
}

/******************************************************************************
*   Get a following extension block (see GIF manual) from gif file. This      *
* routine sould be called until NULL Extension is returned.		      *
*   The Extension should NOT be freed by the user (not dynamically allocated).*
******************************************************************************/
int GxDGifGetExtensionNext(GifFileType * GifFile, ByteType ** Extension)
{
	ByteType Buf;
	GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

	if (fread(&Buf, 1, 1, Private->File) != 1) {
		_GxGifError = D_GIF_ERR_ReadFailed;
		return ERROR;
	}
	if (Buf > 0) {
		*Extension = Private->Buf;	/* Use private unused buffer */
		(*Extension)[0] = Buf;	/* Pascal strings notation (pos. 0 is length) */
		if (fread(&((*Extension)[1]), 1, Buf, Private->File) != Buf) {
			_GxGifError = D_GIF_ERR_ReadFailed;
			return ERROR;
		}
	} else
		*Extension = NULL;

	return OK;
}

/******************************************************************************
*   This routine should be called last, to close GIF file.		      *
******************************************************************************/
int GxDGifCloseFile(GifFileType * GifFile)
{
	GifFilePrivateType *Private;
	FILE *File;

	if (GifFile == NULL)
		return ERROR;

	Private = (GifFilePrivateType *) GifFile->Private;

	if (!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		_GxGifError = D_GIF_ERR_NotReadable;
		return ERROR;
	}

	File = Private->File;

	if (GifFile->IColorMap)
		GUI_FREE(GifFile->IColorMap);
	if (GifFile->SColorMap)
		GUI_FREE(GifFile->SColorMap);
	if (Private)
		GUI_FREE(Private);
	GUI_FREE(GifFile);

	if ((File) && (fclose(File) != 0)) {
		_GxGifError = D_GIF_ERR_CloseFailed;
		return ERROR;
	}
	return OK;
}

/******************************************************************************
*   Get 2 bytes (word) from the given file:				      *
******************************************************************************/
static int GxDGifGetWord(FILE * File, int *Word)
{
	unsigned char c[2];

	if (fread(c, 1, 2, File) != 2) {
		_GxGifError = D_GIF_ERR_ReadFailed;
		return ERROR;
	}

	*Word = (((unsigned int)c[1]) << 8) + c[0];
	return OK;
}

/******************************************************************************
*   Get 2 bytes (word) from the given file:				      *
******************************************************************************/
static int GifGetWord(char * Data, int *Word)
{
	if (NULL == Data || NULL == Word) {
		_GxGifError = D_GIF_ERR_ReadFailed;
		return ERROR;
	}

	*Word = (((unsigned int)Data[1] & 0x000000FF) << 8) | ((unsigned int)Data[0] & 0x000000FF);
	return OK;
}

/******************************************************************************
*   Get the image code in compressed form. his routine can be called if the   *
* information needed to be piped out as is. Obviously this is much faster     *
* than decoding and encoding again. This routine should be followed by calls  *
* to GxDGifGetCodeNext, until NULL block is returned.			      *
*   The block should NOT be freed by the user (not dynamically allocated).    *
******************************************************************************/
int GxDGifGetCode(GifFileType * GifFile, int *CodeSize, ByteType ** CodeBlock)
{
	GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

	if (!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		_GxGifError = D_GIF_ERR_NotReadable;
		return ERROR;
	}

	*CodeSize = Private->BitsPerPixel;

	return GxDGifGetCodeNext(GifFile, CodeBlock);
}

int GifGetCodeNext(GifFileType * GifFile, ByteType ** CodeBlock)
{
	ByteType Buf;
	GifFilePrivateType *Private = (GifFilePrivateType *)GifFile->Private;

	Buf = *Private->data;
	Private->data++;

	if(Buf > 0)
	{
		*CodeBlock = Private->Buf;
		(*CodeBlock)[0] = Buf;
		memcpy(&((*CodeBlock)[1]), Private->data, Buf);
		Private->data += Buf;
	}
	else
	{
		*CodeBlock = NULL;
		Private->Buf[0] = 0;	/* Make sure the buffer is empty! */
		Private->PixelCount = 0;	/* And local info. indicate image read */
	}

	return OK;
}

/******************************************************************************
*   Continue to get the image code in compressed form. This routine should be *
* called until NULL block is returned.					      *
*   The block should NOT be freed by the user (not dynamically allocated).    *
******************************************************************************/
int GxDGifGetCodeNext(GifFileType * GifFile, ByteType ** CodeBlock)
{
	ByteType Buf;
	GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

	if (fread(&Buf, 1, 1, Private->File) != 1) {
		_GxGifError = D_GIF_ERR_ReadFailed;
		return ERROR;
	}

	if (Buf > 0) {
		*CodeBlock = Private->Buf;	/* Use private unused buffer */
		(*CodeBlock)[0] = Buf;	/* Pascal strings notation (pos. 0 is length) */
		if (fread(&((*CodeBlock)[1]), 1, Buf, Private->File) != Buf) {
			_GxGifError = D_GIF_ERR_ReadFailed;
			return ERROR;
		}
	} else {
		*CodeBlock = NULL;
		Private->Buf[0] = 0;	/* Make sure the buffer is empty! */
		Private->PixelCount = 0;	/* And local info. indicate image read */
	}

	return OK;
}

static int GifSetupDecompress(GifFileType * GifFile)
{
	int i, BitsPerPixel;
	ByteType CodeSize;
	unsigned int *Prefix;
	GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

	//fread(&CodeSize, 1, 1, Private->File);	/* Read Code size from file */
	CodeSize = *Private->data;
	Private->data++;
	BitsPerPixel = CodeSize;

	Private->Buf[0] = 0;	/* Input Buffer empty */
	Private->BitsPerPixel = BitsPerPixel;
	Private->ClearCode = (1 << BitsPerPixel);
	Private->EOFCode = Private->ClearCode + 1;
	Private->RunningCode = Private->EOFCode + 1;
	Private->RunningBits = BitsPerPixel + 1;	/* Number of bits per code */
	Private->MaxCode1 = 1 << Private->RunningBits;	/* Max. code + 1 */
	Private->StackPtr = 0;	/* No pixels on the pixel stack */
	Private->LastCode = NO_SUCH_CODE;
	Private->CrntShiftState = 0;	/* No information in CrntShiftDWord */
	Private->CrntShiftDWord = 0;

	Prefix = Private->Prefix;
	for (i = 0; i < LZ_MAX_CODE; i++)
		Prefix[i] = NO_SUCH_CODE;

	return OK;
}

/******************************************************************************
*   Setup the LZ decompression for this image:				      *
******************************************************************************/
static int GxDGifSetupDecompress(GifFileType * GifFile)
{
	int i, BitsPerPixel;
	ByteType CodeSize;
	unsigned int *Prefix;
	GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

	fread(&CodeSize, 1, 1, Private->File);	/* Read Code size from file */
	BitsPerPixel = CodeSize;

	Private->Buf[0] = 0;	/* Input Buffer empty */
	Private->BitsPerPixel = BitsPerPixel;
	Private->ClearCode = (1 << BitsPerPixel);
	Private->EOFCode = Private->ClearCode + 1;
	Private->RunningCode = Private->EOFCode + 1;
	Private->RunningBits = BitsPerPixel + 1;	/* Number of bits per code */
	Private->MaxCode1 = 1 << Private->RunningBits;	/* Max. code + 1 */
	Private->StackPtr = 0;	/* No pixels on the pixel stack */
	Private->LastCode = NO_SUCH_CODE;
	Private->CrntShiftState = 0;	/* No information in CrntShiftDWord */
	Private->CrntShiftDWord = 0;

	Prefix = Private->Prefix;
	for (i = 0; i < LZ_MAX_CODE; i++)
		Prefix[i] = NO_SUCH_CODE;

	return OK;
}

static int GifDecompressLine(GifFileType * GifFile, PixelType * Line, int LineLen)
{
	int i = 0;
	int j, CrntCode, EOFCode, ClearCode, CrntPrefix, LastCode, StackPtr;
	ByteType *Stack, *Suffix;
	unsigned int *Prefix;
	GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

	StackPtr = Private->StackPtr;
	Prefix = Private->Prefix;
	Suffix = Private->Suffix;
	Stack = Private->Stack;
	EOFCode = Private->EOFCode;
	ClearCode = Private->ClearCode;
	LastCode = Private->LastCode;

	if (StackPtr > LZ_MAX_CODE) {
		return ERROR;
	}

	if (StackPtr != 0) {
		/* Let pop the stack off before continueing to read the GIF file: */
		while (StackPtr != 0 && i < LineLen)
			Line[i++] = Stack[--StackPtr];
	}

	while (i < LineLen) {    /* Decode LineLen items. */
		if (GifDecompressInput(Private, &CrntCode) == ERROR)
			return ERROR;

		if (CrntCode == EOFCode) {
			/* Note however that usually we will not be here as we will stop
			 * decoding as soon as we got all the pixel, or EOF code will
			 * not be read at all, and DGifGetLine/Pixel clean everything.  */
			return ERROR;
		} else if (CrntCode == ClearCode) {
			/* We need to start over again: */
			for (j = 0; j <= LZ_MAX_CODE; j++)
				Prefix[j] = NO_SUCH_CODE;
			Private->RunningCode = Private->EOFCode + 1;
			Private->RunningBits = Private->BitsPerPixel + 1;
			Private->MaxCode1 = 1 << Private->RunningBits;
			LastCode = Private->LastCode = NO_SUCH_CODE;
		} else {
			/* Its regular code - if in pixel range simply add it to output
			 * stream, otherwise trace to codes linked list until the prefix
			 * is in pixel range: */
			if (CrntCode < ClearCode) {
				/* This is simple - its pixel scalar, so add it to output: */
				Line[i++] = CrntCode;
			} else {
				/* Its a code to needed to be traced: trace the linked list
				 * until the prefix is a pixel, while pushing the suffix
				 * pixels on our stack. If we done, pop the stack in reverse
				 * (thats what stack is good for!) order to output.  */
				if (Prefix[CrntCode] == NO_SUCH_CODE) {
					CrntPrefix = LastCode;

					/* Only allowed if CrntCode is exactly the running code:
					 * In that case CrntCode = XXXCode, CrntCode or the
					 * prefix code is last code and the suffix char is
					 * exactly the prefix of last code! */
					if (CrntCode == Private->RunningCode - 2) {
						Suffix[Private->RunningCode - 2] =
							Stack[StackPtr++] = GxDGifGetPrefixChar(Prefix,
									LastCode,
									ClearCode);
					} else {
						Suffix[Private->RunningCode - 2] =
							Stack[StackPtr++] = GxDGifGetPrefixChar(Prefix,
									CrntCode,
									ClearCode);
					}
				} else
					CrntPrefix = CrntCode;

				/* Now (if image is O.K.) we should not get a NO_SUCH_CODE
				 * during the trace. As we might loop forever, in case of
				 * defective image, we use StackPtr as loop counter and stop
				 * before overflowing Stack[]. */
				while (StackPtr < LZ_MAX_CODE &&
						CrntPrefix > ClearCode && CrntPrefix <= LZ_MAX_CODE) {
					Stack[StackPtr++] = Suffix[CrntPrefix];
					CrntPrefix = Prefix[CrntPrefix];
				}
				if (StackPtr >= LZ_MAX_CODE || CrntPrefix > LZ_MAX_CODE) {
					return ERROR;
				}
				/* Push the last character on stack: */
				Stack[StackPtr++] = CrntPrefix;

				/* Now lets pop all the stack into output: */
				while (StackPtr != 0 && i < LineLen)
					Line[i++] = Stack[--StackPtr];
			}
			if (LastCode != NO_SUCH_CODE && Private->RunningCode - 2 < (LZ_MAX_CODE+1) && Prefix[Private->RunningCode - 2] == NO_SUCH_CODE) {
				Prefix[Private->RunningCode - 2] = LastCode;

				if (CrntCode == Private->RunningCode - 2) {
					/* Only allowed if CrntCode is exactly the running code:
					 * In that case CrntCode = XXXCode, CrntCode or the
					 * prefix code is last code and the suffix char is
					 * exactly the prefix of last code! */
					Suffix[Private->RunningCode - 2] =
						GxDGifGetPrefixChar(Prefix, LastCode, ClearCode);
				} else {
					Suffix[Private->RunningCode - 2] =
						GxDGifGetPrefixChar(Prefix, CrntCode, ClearCode);
				}
			}
			LastCode = CrntCode;
		}
	}

	Private->LastCode = LastCode;
	Private->StackPtr = StackPtr;

	return OK;
}

/******************************************************************************
*   The LZ decompression routine:					      *
*   This version decompress the given gif file into Line of length LineLen.   *
*   This routine can be called few times (one per scan line, for example), in *
* order the complete the whole image.					      *
******************************************************************************/
static int GxDGifDecompressLine(GifFileType * GifFile, PixelType * Line, int LineLen)
{
	int i = 0, j, CrntCode, EOFCode, ClearCode, CrntPrefix, LastCode, StackPtr;
	ByteType *Stack, *Suffix;
	unsigned int *Prefix;
	GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

	StackPtr = Private->StackPtr;
	Prefix = Private->Prefix;
	Suffix = Private->Suffix;
	Stack = Private->Stack;
	EOFCode = Private->EOFCode;
	ClearCode = Private->ClearCode;
	LastCode = Private->LastCode;

	if (StackPtr != 0) {
		/* Let pop the stack off before continueing to read the gif file: */
		while (StackPtr != 0 && i < LineLen)
			Line[i++] = Stack[--StackPtr];
	}

	while (i < LineLen) {	/* Decode LineLen items. */
		if (GxDGifDecompressInput(Private, &CrntCode) == ERROR)
			return ERROR;

		if (CrntCode == EOFCode) {
			/* Note however that usually we will not be here as we will stop */
			/* decoding as soon as we got all the pixel, or EOF code will    */
			/* not be read at all, and GxDGifGetLine/Pixel clean everything.   */
			if (i != LineLen - 1 || Private->PixelCount != 0) {
				_GxGifError = D_GIF_ERR_EOFTooSoon;
				return ERROR;
			}
			i++;
		} else if (CrntCode == ClearCode) {
			/* We need to start over again: */
			for (j = 0; j < LZ_MAX_CODE; j++)
				Prefix[j] = NO_SUCH_CODE;
			Private->RunningCode = Private->EOFCode + 1;
			Private->RunningBits = Private->BitsPerPixel + 1;
			Private->MaxCode1 = 1 << Private->RunningBits;
			LastCode = Private->LastCode = NO_SUCH_CODE;
		} else {
			/* Its regular code - if in pixel range simply add it to output  */
			/* stream, otherwise trace to codes linked list until the prefix */
			/* is in pixel range:                                            */
			if (CrntCode < ClearCode) {
				/* This is simple - its pixel scalar, so add it to output:   */
				Line[i++] = CrntCode;
			} else {
				/* Its a code to needed to be traced: trace the linked list  */
				/* until the prefix is a pixel, while pushing the suffix     */
				/* pixels on our stack. If we done, pop the stack in reverse */
				/* (thats what stack is good for!) order to output.          */
				if (Prefix[CrntCode] == NO_SUCH_CODE) {
					/* Only allowed if CrntCode is exactly the running code: */
					/* In that case CrntCode = XXXCode, CrntCode or the      */
					/* prefix code is last code and the suffix char is       */
					/* exactly the prefix of last code!                      */
					if (CrntCode == Private->RunningCode - 2) {
						CrntPrefix = LastCode;
						Suffix[Private->RunningCode - 2] =
						    Stack[StackPtr++] = GxDGifGetPrefixChar(Prefix, LastCode, ClearCode);
					} else {
						_GxGifError = D_GIF_ERR_ImageDefect;
						return ERROR;
					}
				} else
					CrntPrefix = CrntCode;

				/* Now (if image is O.K.) we should not get and NO_SUCH_CODE */
				/* During the trace. As we might loop forever, in case of    */
				/* defective image, we count the number of loops we trace    */
				/* and stop if we got LZ_MAX_CODE. obviously we can not      */
				/* loop more than that.                                      */
				j = 0;
				while (j++ <= LZ_MAX_CODE && CrntPrefix > ClearCode && CrntPrefix <= LZ_MAX_CODE) {
					Stack[StackPtr++] = Suffix[CrntPrefix];
					CrntPrefix = Prefix[CrntPrefix];
				}
				if (j >= LZ_MAX_CODE || CrntPrefix > LZ_MAX_CODE) {
					_GxGifError = D_GIF_ERR_ImageDefect;
					return ERROR;
				}
				/* Push the last character on stack: */
				Stack[StackPtr++] = CrntPrefix;

				/* Now lets pop all the stack into output: */
				while (StackPtr != 0 && i < LineLen)
					Line[i++] = Stack[--StackPtr];
			}
			if (LastCode != NO_SUCH_CODE) {
				Prefix[Private->RunningCode - 2] = LastCode;

				if (CrntCode == Private->RunningCode - 2) {
					/* Only allowed if CrntCode is exactly the running code: */
					/* In that case CrntCode = XXXCode, CrntCode or the      */
					/* prefix code is last code and the suffix char is       */
					/* exactly the prefix of last code!                      */
					Suffix[Private->RunningCode - 2] =
					    GxDGifGetPrefixChar(Prefix, LastCode, ClearCode);
				} else {
					Suffix[Private->RunningCode - 2] =
					    GxDGifGetPrefixChar(Prefix, CrntCode, ClearCode);
				}
			}
			LastCode = CrntCode;
		}
	}

	Private->LastCode = LastCode;
	Private->StackPtr = StackPtr;

	return OK;
}

/******************************************************************************
* Routine to trace the Prefixes linked list until we get a prefix which is    *
* not code, but a pixel value (less than ClearCode). Returns that pixel value.*
* If image is defective, we might loop here forever, so we limit the loops to *
* the maximum possible if image O.k. - LZ_MAX_CODE times.		      *
******************************************************************************/
static int GxDGifGetPrefixChar(unsigned int *Prefix, int Code, int ClearCode)
{
	int i = 0;

	while (Code > ClearCode && i++ <= LZ_MAX_CODE)
		Code = Prefix[Code];
	return Code;
}

/******************************************************************************
*   Interface for accessing the LZ codes directly. Set Code to the real code  *
* (12bits), or to -1 if EOF code is returned.				      *
******************************************************************************/
int GxDGifGetLZCodes(GifFileType * GifFile, int *Code)
{
	ByteType *CodeBlock;
	GifFilePrivateType *Private = (GifFilePrivateType *) GifFile->Private;

	if (!IS_READABLE(Private)) {
		/* This file was NOT open for reading: */
		_GxGifError = D_GIF_ERR_NotReadable;
		return ERROR;
	}

	if (GxDGifDecompressInput(Private, Code) == ERROR)
		return ERROR;

	if (*Code == Private->EOFCode) {
		/* Skip rest of codes (hopefully only NULL terminating block): */
		do
			if (GxDGifGetCodeNext(GifFile, &CodeBlock) == ERROR)
				return ERROR;
		while (CodeBlock != NULL) ;

		*Code = -1;
	} else if (*Code == Private->ClearCode) {
		/* We need to start over again: */
		Private->RunningCode = Private->EOFCode + 1;
		Private->RunningBits = Private->BitsPerPixel + 1;
		Private->MaxCode1 = 1 << Private->RunningBits;
	}

	return OK;
}

static int GifDecompressInput(GifFilePrivateType * Private, int *Code)
{
	ByteType NextByte;
	static unsigned int CodeMasks[] = {
		0x0000, 0x0001, 0x0003, 0x0007,
		0x000f, 0x001f, 0x003f, 0x007f,
		0x00ff, 0x01ff, 0x03ff, 0x07ff,
		0x0fff
	};

	while (Private->CrntShiftState < Private->RunningBits) {
		/* Needs to get more bytes from input stream for next code: */
		if (GifBufferedInput(Private, Private->Buf, &NextByte)
		    == ERROR) {
			return ERROR;
		}
		Private->CrntShiftDWord |= ((unsigned long)NextByte) << Private->CrntShiftState;
		Private->CrntShiftState += 8;
	}
	*Code = Private->CrntShiftDWord & CodeMasks[Private->RunningBits];

	Private->CrntShiftDWord >>= Private->RunningBits;
	Private->CrntShiftState -= Private->RunningBits;

	/* If code cannt fit into RunningBits bits, must raise its size. Note */
	/* however that codes above 4095 are used for special signaling.      */
	if (++Private->RunningCode > Private->MaxCode1 && Private->RunningBits < LZ_BITS) {
		Private->MaxCode1 <<= 1;
		Private->RunningBits++;
	}
	return OK;
}

/******************************************************************************
*   The LZ decompression input routine:					      *
*   This routine is responsable for the decompression of the bit stream from  *
* 8 bits (bytes) packets, into the real codes.				      *
*   Returns OK if read succesfully.					      *
******************************************************************************/
static int GxDGifDecompressInput(GifFilePrivateType * Private, int *Code)
{
	ByteType NextByte;
	static unsigned int CodeMasks[] = {
		0x0000, 0x0001, 0x0003, 0x0007,
		0x000f, 0x001f, 0x003f, 0x007f,
		0x00ff, 0x01ff, 0x03ff, 0x07ff,
		0x0fff
	};

	while (Private->CrntShiftState < Private->RunningBits) {
		/* Needs to get more bytes from input stream for next code: */
		if (GxDGifBufferedInput(Private->File, Private->Buf, &NextByte)
		    == ERROR) {
			return ERROR;
		}
		Private->CrntShiftDWord |= ((unsigned long)NextByte) << Private->CrntShiftState;
		Private->CrntShiftState += 8;
	}
	*Code = Private->CrntShiftDWord & CodeMasks[Private->RunningBits];

	Private->CrntShiftDWord >>= Private->RunningBits;
	Private->CrntShiftState -= Private->RunningBits;

	/* If code cannt fit into RunningBits bits, must raise its size. Note */
	/* however that codes above 4095 are used for special signaling.      */
	if (++Private->RunningCode > Private->MaxCode1 && Private->RunningBits < LZ_BITS) {
		Private->MaxCode1 <<= 1;
		Private->RunningBits++;
	}
	return OK;
}

static int GifBufferedInput(GifFilePrivateType * Private, ByteType * Buf, ByteType * NextByte)
{
	if (Buf[0] == 0) {
		/* Needs to read the next buffer - this one is empty: */
		Buf[0] = *Private->data;
		Private->data++;

		if (Buf[0] == 0) {
			return ERROR;
		}

		memcpy(&Buf[1], Private->data, Buf[0]);
		Private->data += Buf[0];

		*NextByte = Buf[1];
		Buf[1] = 2;	/* We use now the second place as last char read! */
		Buf[0]--;
	} else {
		*NextByte = Buf[Buf[1]++];
		Buf[0]--;
	}

	return OK;
}

/******************************************************************************
*   This routines read one gif data block at a time and buffers it internally *
* so that the decompression routine could access it.			      *
*   The routine returns the next byte from its internal buffer (or read next  *
* block in if buffer empty) and returns OK if succesful.		      *
******************************************************************************/
static int GxDGifBufferedInput(FILE * File, ByteType * Buf, ByteType * NextByte)
{
	if (Buf[0] == 0) {
		/* Needs to read the next buffer - this one is empty: */
		if (fread(Buf, 1, 1, File) != 1) {
			_GxGifError = D_GIF_ERR_ReadFailed;
			return ERROR;
		}
		if (fread(&Buf[1], 1, Buf[0], File) != Buf[0]) {
			_GxGifError = D_GIF_ERR_ReadFailed;
			return ERROR;
		}
		*NextByte = Buf[1];
		Buf[1] = 2;	/* We use now the second place as last char read! */
		Buf[0]--;
	} else {
		*NextByte = Buf[Buf[1]++];
		Buf[0]--;
	}

	return OK;
}
