/****************************************************************************
*						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parser.c
* Author    :	shenbin
* Project   :	GoXceed
* Type      :
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0	2009.09.01	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <string.h>
#include "module/si/si_parser.h"
#include "module/si/si_public.h"
#include "module/si/si_public_parser.h"

#include "module/config/gxconfig.h"

#include <gxcore.h>

static uint32_t s_ProtectDisable = 0;
static int32_t s_small_buf_size = 0;

// 管理parsed data 与subtable id之间的联系
struct parsed_data_status
{
	uint8_t *parsed_data;
	int16_t subtable_id;
	int16_t reserved;
};

struct parsed_data_ctrl
{
	void (*init)(void);
	void (*add)(int16_t, uint8_t*); // subtable id    ,    parsed data
	status_t (*del_by_data)(uint8_t*);
	void (*del_by_id)(int16_t);
};

// mph  - mem pool handle
static uint32_t s_mph_onek;

static struct
{
	uint32_t lo_addr;
	uint32_t hi_addr;
}s_onek_range = {~0, 0};

//uint32_t alloc_record1 = 0;
//uint32_t alloc_record4 = 0;
/* Exported Variables ----------------------------------------------------- */

/* Private Macros --------------------------------------------------------- */
#define SMALL_BUF_SIZE	(SI_EIT_BUF_SIZE+ADDITIONAL_SIZE)// standard mode for EIT
#define SMALL_BUF_NUM	(80)

// parsed data 与subtable id管理的内存空间
static struct parsed_data_status s_parsed_buf[SMALL_BUF_NUM];

/* Private Functions Prototypes ------------------------------------------- */

/* Private Functions ------------------------------------------------------ */

/**
 * @brief	after get parsed, call this function to free the parsed data buf
 * @param	  parsed_data: parsed data buffer address
 * @Return  GXCORE_SUCCESS
 */
static status_t si_parser_buf_GxCore_Free(uint8_t *parsed_data)
{
	if (parsed_data == NULL)
	{
		gxlogd("[si parser]: mem pool addr is null!\n");
		return GXCORE_ERROR;
	}

	if ((uint32_t)parsed_data >= s_onek_range.lo_addr
		&& (uint32_t)parsed_data <= s_onek_range.hi_addr)
	{
		GxCore_MemPoolFree(s_mph_onek, parsed_data);
	}
	else
	{
		gxlogd("[si parser]: err mem pool addr!\n");
		return GXCORE_ERROR;
	}

	return GXCORE_SUCCESS;
}

static void parsed_data_init(void)
{
	int32_t i;

	//init parsed data ctrl
	for (i = 0; i < SMALL_BUF_NUM; i++)
	{
		s_parsed_buf[i].parsed_data = NULL;
		s_parsed_buf[i].subtable_id = -1;
		s_parsed_buf[i].reserved = 0;
	}
}

static void parsed_data_add(int16_t si_subtable_id, uint8_t* p_parsed_data)
{
	int32_t i;

	for (i = 0; i < SMALL_BUF_NUM; i++)
	{
		if (s_parsed_buf[i].subtable_id == -1)
		{
			s_parsed_buf[i].subtable_id = si_subtable_id;
			s_parsed_buf[i].parsed_data = p_parsed_data;

			break;
		}
	}
}

static status_t parsed_data_del_by_data(uint8_t* p_parsed_data)
{
	int32_t i;

	for (i = 0; i < SMALL_BUF_NUM; i++)
	{
		if (s_parsed_buf[i].parsed_data == p_parsed_data)
		{
			s_parsed_buf[i].subtable_id = -1;
			s_parsed_buf[i].parsed_data = NULL;

			return GXCORE_SUCCESS;
		}
	}

	return GXCORE_ERROR;
}

static void parsed_data_del_by_id(int16_t si_subtable_id)
{
	int32_t i;

	for (i = 0; i < SMALL_BUF_NUM; i++)
	{
		if (s_parsed_buf[i].subtable_id == si_subtable_id)
		{
			s_parsed_buf[i].subtable_id = -1;

			si_parser_buf_GxCore_Free(s_parsed_buf[i].parsed_data);

			s_parsed_buf[i].parsed_data = NULL;

			break;
		}
	}
}

struct parsed_data_ctrl s_parsed_ctrl =
{
	.init = parsed_data_init,
	.add = parsed_data_add,
	.del_by_data = parsed_data_del_by_data,
	.del_by_id = parsed_data_del_by_id
};


/* Exported Functions ----------------------------------------------------- */
// for test only
void GxTest_SiGetMemPoolHandle(uint32_t *onek_handle)
{
	*onek_handle = s_mph_onek;
}

void GxTest_SiSetMemRange(uint32_t oneklo, uint32_t onekhi)
{
	s_onek_range.lo_addr = oneklo;
	s_onek_range.hi_addr = onekhi;
}

/**
 * @brief	create the mempool for si parser, do it in "gxsi.c"
 * @param	  void
 * @Return  GXCORE_ERROR  GXCORE_SUCCESS
 */
status_t GxBus_SiParserBufCreate(void)
{
	int32_t small_buf_count = 0;

	GxBus_ConfigGetInt(GXBUS_SI_SMALL_BUF_SIZE, &s_small_buf_size, SMALL_BUF_SIZE);
	GxBus_ConfigGetInt(GXBUS_SI_SMALL_BUF_COUNT, &small_buf_count, SMALL_BUF_NUM);
	small_buf_count = small_buf_count >= SMALL_BUF_NUM ? SMALL_BUF_NUM : small_buf_count;

	s_mph_onek = GxCore_MemPoolCreate(s_small_buf_size, small_buf_count, 500);
	if (s_mph_onek == 0)
	{
		return GXCORE_ERROR;
	}
	else
	{
		// init mem pool range
		s_onek_range.lo_addr = (~0);
		s_onek_range.hi_addr = 0;
	}

	s_parsed_ctrl.init();

	return GXCORE_SUCCESS;
}

/**
 * @brief	release the mempool for si parser
 * @param	  void
 * @Return  void
 */
void GxBus_SiParserBufRelease(void)
{
	GxCore_MemPoolDestory(s_mph_onek);
}

/**
 * @brief	after get parsed, call this function to free the parsed data buf
 * @param	  parsed_data: parsed data buffer address
 * @Return  GXCORE_SUCCESS
 */
status_t GxBus_SiParserBufFree(uint8_t *parsed_data)
{
    if (parsed_data == NULL)
    {
        return GXCORE_ERROR;
    }

	if (GXCORE_SUCCESS == s_parsed_ctrl.del_by_data(parsed_data)) {
		si_parser_buf_GxCore_Free(parsed_data);
	} else {
		//already free in subtable release "del_by_id"
	}

	return GXCORE_SUCCESS;
}

// this function for si_filter, in subtable release to free all mem belong to the subtable id
void GxBus_SiParserMemFree(int16_t si_subtable_id)
{
	s_parsed_ctrl.del_by_id(si_subtable_id);
}

void GxBus_SiParserProtectEnable(void)
{
    s_ProtectDisable = 0;
}
void GxBus_SiParserProtectDisable(void)
{
    s_ProtectDisable = 1;
}

/**
 * @brief	call this function to parse all the table
 * @param table_parse: private table parse function, if this set "NULL", will call default table parse function
 * @param section_data: the section data from demux
 * @param len: section length
 * @Return	parsed_data
			NULL: parse error
 */
void* GxBus_SiParser(private_table_cfg *table_cfg,
						uint8_t *section_data,
						uint32_t len, int16_t si_subtable_id)
{
// include table_id (1) and section_len (2)
#define SECTION_ONLY_APPEND_LEN   (3)
	int16_t sec_len;
	StandardParserState ret;
	uint8_t *p_parsed_data = SEC_PRIVATE_PARSE;

	if((section_data == NULL) || (len == 0))
	{
		return NULL;
	}

	if (table_cfg->mode == PARSE_STANDARD_ONLY
			|| table_cfg->mode == PARSE_WITH_STANDARD
			|| table_cfg->mode == PARSE_SECTION_ONLY)
	{
		if (table_cfg->mode == PARSE_WITH_STANDARD
				&& table_cfg->table_parse_fun != NULL)
		{
			// private parse
			table_cfg->table_parse_fun(section_data, len);
		}

		//parse eit by epg service, don't use Si. just use SMALL_BUF_SIZE in here
		p_parsed_data = (uint8_t*)GxCore_MemPoolAllocZero(s_mph_onek);

		if (p_parsed_data == NULL)
		{
			gxlogd("[si_parser] GxCore_MemPoolAllocZero1 error!\n");
			return SEC_NO_MEMORY;
		}
		//		alloc_record1++;
		//		gxlogd("alloc alloc_record1 --- 0x%x\n", alloc_record1);
		if ((uint32_t)p_parsed_data < s_onek_range.lo_addr)
		{
			s_onek_range.lo_addr = (uint32_t)p_parsed_data;
		}
		if ((uint32_t)p_parsed_data > s_onek_range.hi_addr)
		{
			s_onek_range.hi_addr = (uint32_t)p_parsed_data;
		}

		memset(p_parsed_data, 0, s_small_buf_size);

		s_parsed_ctrl.add(si_subtable_id, p_parsed_data);

		// get section length
		sec_len = TODATA12(section_data[1], section_data[2]);

		// for parse section only
		if (table_cfg->mode == PARSE_SECTION_ONLY)
		{
			if (sec_len + SECTION_ONLY_APPEND_LEN > s_small_buf_size)
			{
				GxBus_SiParserBufFree(p_parsed_data);
				return SEC_LEN_ERROR;
			}

			memcpy(p_parsed_data, section_data, sec_len + SECTION_ONLY_APPEND_LEN);

			return p_parsed_data;
		}

		ret = GxBus_SiStandardParser(section_data,p_parsed_data,len,s_ProtectDisable);
		if(ret == STANDARD_PARSER_SYNTAX_ERROR)
		{
			GxBus_SiParserBufFree(p_parsed_data);
			return SEC_SYNTAX_INDICATOR;
		}
	    else if(ret == STANDARD_PARSER_LEN_ERROR)
		{
			GxBus_SiParserBufFree(p_parsed_data);
			return SEC_LEN_ERROR;
		}
	}
	else if (table_cfg->mode == PARSE_PRIVATE_ONLY)
	{
		int32_t private_status;

		// private parse
		if (table_cfg->table_parse_fun != NULL)
		{
			private_status = table_cfg->table_parse_fun(section_data, len);
			if (PRIVATE_SECTION_OK == private_status)
			{
				p_parsed_data = SEC_SECTION_OK;
			}
			else
			{
				p_parsed_data = SEC_SUBTABLE_OK;
			}
			// above p_parsed_data use to return private parse result
		}
	}
	else {}

	//	gxlogd("4  super tail - 0x%x\n", ((EitInfo *)(p_parsed_data))->tail);

	return p_parsed_data;
}
/* End of file -------------------------------------------------------------*/

