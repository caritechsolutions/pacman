/*****************************************************************************
*   "Gif-Lib" - Yet another gif library.				     *
*									     *
* Written by:  Gershon Elber			IBM PC Ver 0.1,	Jun. 1989    *
******************************************************************************
* Handle error reporting for the GIF library.				     *
******************************************************************************
* History:								     *
* 17 Jun 89 - Version 1.0 by Gershon Elber.				     *
*****************************************************************************/

#include <stdio.h>
#include "gif_lib.h"

#define PROGRAM_NAME	"GIF_LIBRARY"
#define VERSION		"?Version 1.0, "

int _GxGifError = 0;

/*static char *VersionStr =
	PROGRAM_NAME
	"	IBMPC "
	VERSION
	"	Gershon Elber,	"
	__DATE__ ",   " __TIME__ "\n"
	"(C) Copyright 1989 Gershon Elber, Non commercial use only.\n";*/

/*****************************************************************************
* Return the last GIF error (0 if none) and reset the error.		     *
*****************************************************************************/
int GifLastError(void)
{
	int i = _GxGifError;

	_GxGifError = 0;

	return i;
}

/*****************************************************************************
* Print the last GIF error to stderr.					     *
*****************************************************************************/
void PrintGifError(void)
{
	char *Err;

	switch (_GxGifError) {
	case E_GIF_ERR_OpenFailed:
		Err = "Failed to open given file";
		break;
	case E_GIF_ERR_WriteFailed:
		Err = "Failed to Write to given file";
		break;
	case E_GIF_ERR_HasScrnDscr:
		Err = "Screen Descriptor already been set";
		break;
	case E_GIF_ERR_HasImagDscr:
		Err = "Image Descriptor is still active";
		break;
	case E_GIF_ERR_NoColorMap:
		Err = "Neither Global Nor Local color map";
		break;
	case E_GIF_ERR_DataTooBig:
		Err = "#Pixels bigger than Width * Height";
		break;
	case E_GIF_ERR_NotEnoughMem:
		Err = "Fail to allocate required memory";
		break;
	case E_GIF_ERR_DiskIsFull:
		Err = "Write failed (disk full?)";
		break;
	case E_GIF_ERR_CloseFailed:
		Err = "Failed to close given file";
		break;
	case E_GIF_ERR_NotWriteable:
		Err = "Given file was not opened for write";
		break;
	case D_GIF_ERR_OpenFailed:
		Err = "Failed to open given file";
		break;
	case D_GIF_ERR_ReadFailed:
		Err = "Failed to Read from given file";
		break;
	case D_GIF_ERR_NotGifFile:
		Err = "Given file is NOT GIF file";
		break;
	case D_GIF_ERR_NoScrnDscr:
		Err = "No Screen Descriptor detected";
		break;
	case D_GIF_ERR_NoImagDscr:
		Err = "No Image Descriptor detected";
		break;
	case D_GIF_ERR_NoColorMap:
		Err = "Neither Global Nor Local color map";
		break;
	case D_GIF_ERR_WrongRecord:
		Err = "Wrong record type detected";
		break;
	case D_GIF_ERR_DataTooBig:
		Err = "#Pixels bigger than Width * Height";
		break;
	case D_GIF_ERR_NotEnoughMem:
		Err = "Fail to allocate required memory";
		break;
	case D_GIF_ERR_CloseFailed:
		Err = "Failed to close given file";
		break;
	case D_GIF_ERR_NotReadable:
		Err = "Given file was not opened for read";
		break;
	case D_GIF_ERR_ImageDefect:
		Err = "Image is defective, decoding aborted";
		break;
	case D_GIF_ERR_EOFTooSoon:
		Err = "Image EOF detected, before image complete";
		break;
	default:
		Err = NULL;
		break;
	}
	if (Err != NULL)
		gxloge ( "\nGIF-LIB error: %s\n", Err);
	else
		gxloge ( "\nGIF-LIB undefined error %d\n", _GxGifError);
}
