/** @defgroup pm_module PM Module*/
/*@{*/
#ifndef __ARIBCC_DES_H__
#define __ARIBCC_DES_H__

#include "gxcore.h"

__BEGIN_DECLS



/*Closed Caption 0xFD 描述符*/
typedef struct
{
    uint8_t additional_data_component_info;
}GxCcsystemSiParseCcInfo;


typedef struct
{
    uint32_t descriptor_tag:8;
    uint32_t descriptor_length:8;
    uint32_t data_component_id:16;
    uint16_t cc_info_count;
    GxCcsystemSiParseCcInfo* cc_info;
}GxCcsystemSiParseCcDes;

__END_DECLS

#endif
/*@}*/

