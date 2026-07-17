/**
 *
 * @file        app_common_table_bat.h
 * @brief
 * @version     1.1.0
 * @date        04/30/2014
 * @author      zhouhuaming (zhouhm), zhuohm@nationalchip.com
 *
 */
#ifndef __APP_NIT_H__
#define __APP_NIT_H__

//#include "gxcore.h"

#if (DEMOD_DVB_C > 0)||(DEMOD_DTMB > 0)
void app_table_nit_search_filter_open(void);
void app_table_nit_search_filter_close(void);
void app_table_nit_get_search_filter_info(int32_t* pNitSubtId,uint32_t* pNitRequestId);
#endif
#if LCN_SUPPORT
private_parse_status app_table_nit_extend_descriptor_parse(uint8_t* p_section_data, size_t Size);
#endif

#endif


