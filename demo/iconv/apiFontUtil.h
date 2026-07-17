////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (¡§MStar Confidential Information¡¨) by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////
/// @file   apiFontUtil.h
/// @brief  Font API for character table conversion
/// @author MStar Semiconductor Inc.
/////////////////////////////////////////////////////////////////////////////////////

#ifndef MAPI_FONT_UTIL_H
#define MAPP_FONT_UTIL_H

#include "app_utility.h"

#ifdef __cplusplus
extern "C"
{
#endif


/********************************************************************************/
/*                           Macro                                              */
/********************************************************************************/

/********************************************************************************/
/*                           Enum                                               */
/********************************************************************************/
typedef enum
{
    ISO_6937,
    ISO_8859_01,
    ISO_8859_02,
    ISO_8859_03,
    ISO_8859_04,
    ISO_8859_05,
    ISO_8859_06,
    ISO_8859_07,
    ISO_8859_08,
    ISO_8859_09,
    ISO_8859_10,
    ISO_8859_11,
    ISO_8859_13,
    ISO_8859_14,
    ISO_8859_15,
    MAX_CH_TABLE,   //<RELEASE:1.06.1> 2007-8-15[hks]: add

} EN_CHARACTER_CODE_TABLE;

#define REMOVE_NONE             0x0000
#define REMOVE_00AD_SOFT_HYPHEN 0x0001

/********************************************************************************/
/*                           Function prototypes                                */
/********************************************************************************/
uint16_t MApi_CharTable_MappinUTF8ToUCS2( uint8_t *pu8Str, uint16_t *pu16Str, uint16_t srcByteLen, uint16_t dstWideCharLen );
uint16_t MApi_CharTable_MappingDVBTextToUCS2(uint8_t *pu8ISO3166, uint8_t *pu8Str, uint16_t *pu16Str, uint16_t srcByteLen, uint16_t dstWideCharLen, uint16_t filterCtrl );
uint16_t MApi_CharTable_MappingDVBTextToUCS2_Ext(uint8_t *pu8ISO639, uint8_t *pu8ISO3166, uint8_t *pu8Str, uint16_t *pu16Str, uint16_t srcByteLen, uint16_t dstWideCharLen, uint16_t filterCtrl );
uint16_t MApi_CharTable_MappingDVBTextToUTF8(uint8_t *pu8Str, uint8_t *pu8DestStr, uint16_t srcByteLen, uint16_t dstCharLen,uint8_t *pu8ISO639);

uint16_t MApi_CharTable_MappingIsoToUCS2( EN_CHARACTER_CODE_TABLE enTable, uint8_t *pu8Str, uint16_t *pu16Str, uint16_t u16SrcLen, uint16_t u16DestLen );
void MApi_CharTable_SetTblChkNotify(void (*pNotifyCB)(uint8_t *pu8ISO639, uint8_t *pu8ISO3166, EN_CHARACTER_CODE_TABLE *penTable));

#ifdef __cplusplus
}
#endif


#endif  /*MAPP_CHAR_TABLE_H*/

