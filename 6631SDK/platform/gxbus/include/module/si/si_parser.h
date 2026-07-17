#ifndef __SI_PARSER_H__
#define __SI_PARSER_H__

#include <gxtype.h>
#include <stdint.h>
#include <ctype.h>
#include "gxcore.h"

__BEGIN_DECLS

typedef enum parser_status {
	PRIVATE_SECTION_OK,
	PRIVATE_SUBTABLE_OK
}private_parse_status;

/**
 * @brief	私有表的解析函数
 * @param  p_section_data[IN]   : section数据存储的地址
 *                len          : section数据的长度

 * @Return private_parse_status
 */
typedef private_parse_status (*private_table_parser)(uint8_t *p_section_data, uint32_t len);

typedef enum parser_mode
{
	PARSE_WITH_STANDARD,   // need free standard parse result space
	PARSE_PRIVATE_ONLY,    // only care about private parse
	PARSE_STANDARD_ONLY,
	PARSE_SECTION_ONLY      // get the original section data
}private_parser_mode;

typedef struct table_cfg
{
	private_parser_mode mode;
	private_table_parser table_parse_fun;
}private_table_cfg;




/* Exported Variables ----------------------------------------------------- */

/* Exported Macros -------------------------------------------------------- */
#define GET_TABLE_ID(parse_data) (*(uint8_t*)(parse_data))

#define SEC_PRIVATE_PARSE		((void*)NULL)
#define SEC_LEN_ERROR			((void*)0xfffffff1)
#define SEC_SYNTAX_INDICATOR	((void*)0xfffffff2)
#define SEC_NO_MEMORY			((void*)0xfffffff3)
#define SEC_SECTION_OK          ((void*)0xfffffff4)
#define SEC_SUBTABLE_OK         ((void*)0xfffffff5)


/* 配置si分配的解析空间数量*/

#define GXBUS_SI_SMALL_BUF_SIZE  "gxsi_small_buf_size"
#define GXBUS_SI_SMALL_BUF_COUNT "gxsi_small_buf_count"  ///< 除eit表之外的表使用，默认80个 耗费空间409600
#define GXBUS_SI_LARGE_BUF_COUNT "gxsi_large_buf_count"  ///< eit表专用，默认50个  耗费空间921600
/* Exported Messages ------------------------------------------------------ */

/* Exported Functions ----------------------------------------------------- */
/**
 * @brief	为存储解析后的si数据建立一块内存池
 * @param  void
 * @Return GXCORE_SUCCESS  建立内存池成功
 GXCORE_ERROR  建立内存池失败
 */
status_t GxBus_SiParserBufCreate(void);

/**
 * @brief	释放为存储解析后的si数据所建立的内存池
 * @param   void
 * @Return  void
 */
void GxBus_SiParserBufRelease(void);

/**
 * @brief	在获得到si服务解析后的数据，需要显示调用此接口来释放
 在内存池中申请到的内存块
 * @param   parsed_data   存储解析后数据的内存块起始地址
 * @Return  GXCORE_SUCCESS  释放内存块成功
 GXCORE_ERROR  释放内存块失败
 */
status_t GxBus_SiParserBufFree(uint8_t *parsed_data);

/**
 * @brief 所有表解析函数的入口
 * @param  table_parse   如果有私有表解析函数传入私有表解析函数，否则
 填NULL
 * @param section_data   section数据所在内存的起始地址
 * @param len  section数据的长度
 * @Return
 */
void* GxBus_SiParser(private_table_cfg *table_cfg,
		uint8_t *section_data,
		uint32_t len, int16_t si_subtable_id);
__END_DECLS

#endif /* __SI_PARSER_H__ */

