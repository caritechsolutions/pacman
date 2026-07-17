/*****************************************************************************
*                          CONFIDENTIAL                             
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2006,2007 All right reserved
******************************************************************************

******************************************************************************
* File Name :   gxdefs.h
* Author    :   liuyx
* Project   :   lowpower
* Type      :   csky
******************************************************************************
* Purpose   :   General data type definition
*****************************************************************************/

/* Define to prevent recursive inclusion */
#ifndef __GX_DEFS_H__
#define __GX_DEFS_H__

/* Includes ---------------------------------------------------------------- */

#ifdef __cplusplus
extern "C" {
#endif

/* Exported Types ---------------------------------------------------------- */

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

#ifndef DEFINED_U64
#define DEFINED_U64
typedef unsigned long long U64;
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

#ifndef DEFINED_S64
#define DEFINED_S64
typedef signed long long S64;
#endif

/* Boolean type (values should be among TRUE and FALSE constants only) */
#ifndef DEFINED_BOOL
#define DEFINED_BOOL
typedef unsigned char BOOL;
#endif

/* Register type */
#ifndef DEFINED_REG
#define DEFINED_REG
typedef U32  Reg_t;
#endif

/* Handle type */
#ifndef DEFINED_HANDLE
#define DEFINED_HANDLE
typedef U32  Handle_t;
#endif

/* General purpose string type */
typedef S8* GX_String_t;

/* Function return error code */
typedef U32 GX_ErrorCode_t;

/* Revision structure */
typedef const S8 * GX_Revision_t;

/* Device name type */
#define GX_MAX_DEVICE_NAME 16  /* 15 characters plus '\0' */
typedef U8 GX_DeviceName_t[GX_MAX_DEVICE_NAME];

/* Packet name type */
#define GX_MAX_PACKET_NAME 16  /* 15 characters plus '\0' */
typedef U8 GXETHMAC_PacketName_t[GX_MAX_PACKET_NAME];
/* Exported Constants ------------------------------------------------------ */

/* BOOL type constant values */
#ifndef TRUE
    #define TRUE    (1 == 1)
#endif
#ifndef FALSE
    #define FALSE   (!TRUE)
#endif

#ifndef true
    #define true    (1 == 1)
#endif
#ifndef false
    #define false   (!true)
#endif

/* General empty pointer type */
#ifndef NULL
#define NULL                (0)
#endif
    
/* Exported Variables ------------------------------------------------------ */


/* Exported Macros --------------------------------------------------------- */


/* Exported Functions ------------------------------------------------------ */

#ifdef __cplusplus
}
#endif

#endif  /* #ifndef __GX_DEFS_H__ */

/* End of gxdefs.h  ------------------------------------------------------- */


