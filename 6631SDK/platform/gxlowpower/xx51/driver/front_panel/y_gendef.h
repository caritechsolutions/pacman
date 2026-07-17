/****************************************************************************
** Notice:		Copyright (c)2006 TunerCom - All Rights Reserved
**
** File Name:	y_gendef.h
**
** Revision:	1.0
** Date:		2006.2.10
**
** Description: Header file of general definations.
**             
****************************************************************************/

#ifndef __Y_GENDEF_H__
#define __Y_GENDEF_H__

/****************************************************************************
** System wide definations
****************************************************************************/
#ifndef BOOL
typedef   unsigned char			BOOL;
#endif

#ifndef  INT8_T
#define INT8_T
typedef signed char			INT8;
#endif

#ifndef  UINT8_T
#define UINT8_T
typedef unsigned char 					UINT8;
#endif

#ifndef  INT16_T
#define INT16_T
typedef signed short				INT16;
#endif

#ifndef  UINT16_T
#define UINT16_T
typedef unsigned short 			UINT16;
#endif

#ifndef  INT32_T
#define INT32_T
typedef signed int			INT32;
#endif

#ifndef	 UINT32_T
#define UINT32_T
typedef unsigned int					UINT32;
#endif

#ifndef BYTE_T
#define BYTE_T
typedef unsigned char 					BYTE;
#endif

#ifndef WORD_T
#define WORD_T
typedef unsigned short 					WORD;
#endif

#ifndef DWORD_T
#define DWORD_T
typedef unsigned int					DWORD;
#endif

#ifndef UNICODE_T
#define UNICODE_T
typedef unsigned short				UNICODE;
#endif

#ifndef FALSE
#define FALSE				(0)
#endif

#ifndef TRUE
#define TRUE				(1)
#endif

#ifndef NULL
#define NULL				(0)
#endif


#ifndef Y_SUCCESS
#define Y_SUCCESS			(0)
#endif

#ifndef Y_FAILURE
#define Y_FAILURE			(-1)
#endif




/* Common unsigned types */
#ifndef DEFINED_U8
#define DEFINED_U8
typedef unsigned char  U8;
#endif

#ifndef DEFINED_U16
#define DEFINED_U16
typedef unsigned short U16;
#endif

#ifndef DEFINED_U32
#define DEFINED_U32
typedef unsigned int   U32;
#endif


/* Common signed types */
#ifndef DEFINED_S8
#define DEFINED_S8
typedef signed char  S8;
#endif

#ifndef DEFINED_S16
#define DEFINED_S16
typedef signed short S16;
#endif

#ifndef DEFINED_S32
#define DEFINED_S32
typedef signed int   S32;
#endif




#endif
