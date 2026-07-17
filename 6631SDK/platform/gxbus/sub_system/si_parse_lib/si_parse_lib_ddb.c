/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_ddb.c
* Author    : 	zhangling
* Project   :	GoXceed 
* Type      :	
******************************************************************************
* Purpose   :	
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.09.01	      shenbin	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "include/si_parse_lib_private.h"

static si_parse_lib_protect protect = {0};
static handle_t si_parse_lib_protect_mutex = 0;
/******************function******************************/

static void si_parse_lib_ddb_release(GxSubsystemSiParseDDB* ddb);
static void si_parse_lib_ddb_add_protect(void* p);
static uint32_t si_parse_lib_ddb_remove_protect(void* p);

static uint32_t si_parse_lib_ddb_remove_protect(void* p)
{
    uint32_t ret = 0;
    if(si_parse_lib_protect_mutex == 0)
    {
	    GxCore_MutexCreate(&si_parse_lib_protect_mutex);
    }

    GxCore_MutexLock(si_parse_lib_protect_mutex);
    ret = si_parse_lib_remove_protect(&protect,p);
    GxCore_MutexUnlock(si_parse_lib_protect_mutex);

    return ret;
}

static void si_parse_lib_ddb_add_protect(void* p)
{
    if(si_parse_lib_protect_mutex == 0)
    {
	    GxCore_MutexCreate(&si_parse_lib_protect_mutex);
    }

    GxCore_MutexLock(si_parse_lib_protect_mutex);
    si_parse_lib_add_protect(&protect,p);
    GxCore_MutexUnlock(si_parse_lib_protect_mutex);

    return;
}
static void si_parse_lib_ddb_release(GxSubsystemSiParseDDB* ddb)
{
    if(ddb != NULL)
    {
        if(ddb->adaptation_data != NULL)
        {
            GxCore_Free(ddb->adaptation_data);
        }
		if(ddb->data != NULL)
		{
			GxCore_Free(ddb->data);
		}
        GxCore_Free(ddb);
    }
    return;
}


/**
 *  @brief      把一段section数据解析成ddb表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      ddb数据
 *               NULL：发生错误
 */
GxSubsystemSiParseDDB* GxSubsystm_SiParseLibDdbParse(uint8_t* section,uint32_t size)
{
    GxSubsystemSiParseDDB* ddb = NULL;
    int32_t len = (int32_t)size;
    uint8_t* p = section;
	
	if(section == NULL || size < 8)//ddb的头部8字节
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("section is NULL or size is err!!");
#endif
        goto err;
    }
    ddb = GxCore_Malloc(sizeof(GxSubsystemSiParseDDB));
    if(ddb == NULL)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse ddb no memery!!");
#endif
        goto err;
    }
    memset(ddb,0,sizeof(GxSubsystemSiParseDDB));
    ddb->table_id = TODATA0_7BIT(p[0]);
    ddb->section_syntax_indicator = TODATA7BIT(p[1]);
    ddb->private_indicator = TODATA6BIT(p[1]);
    ddb->section_length = TODATA0_11BIT(p[1],p[2]);
    ddb->table_id_extension = TODATA0_15BIT(p[3],p[4]);
    ddb->version_number = TODATA1_5BIT(p[5]);
    ddb->current_next_indicator = TODATA0BIT(p[5]);
    ddb->section_number = TODATA0_7BIT(p[6]);
    ddb->last_section_number = TODATA0_7BIT(p[7]);
	
    p += 8;//跳过头部
    len -= 8;//头部解析完成
    len -= 4;//去掉crc

    ddb->protocol_discriminator = TODATA0_7BIT(p[0]);
    ddb->dsmcc_type = TODATA0_7BIT(p[1]);

    ddb->message_id = TODATA0_15BIT(p[2],p[3]);
	if(ddb->message_id != 0x1003)//ddb 
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse ddb err!!");
#endif
        goto err;
	}
    ddb->download_id = TODATA0_31BIT(p[4],p[5],p[6],p[7]);
/*有一个字节为reserved*/
	ddb->adaptation_length = TODATA0_7BIT(p[9]);
	ddb->message_length = TODATA0_15BIT(p[10],p[11]);

	p += 12;//跳过dsmcc message header
	len -= 12;

	p += ddb->adaptation_length;//跳过adaptation
	len -= ddb->adaptation_length;

	if(len != ddb->message_length-ddb->adaptation_length)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse ddb length err!!");
#endif
        goto err;
	}
	
	ddb->module_id = TODATA0_15BIT(p[0],p[1]);
	ddb->module_version = TODATA0_7BIT(p[2]);
	ddb->block_number = TODATA0_15BIT(p[4],p[5]);

	ddb->data_len = ddb->message_length - ddb->adaptation_length - 6;
	if(ddb->data_len <= 0)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse ddb length err!!");
#endif
        goto err;
	}
	ddb->data = GxCore_Mallocz(ddb->data_len);
	if(ddb->data == NULL)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse ddb  no memroy!!");
#endif
        goto err;
	}
	
	memcpy(ddb->data,&p[6],ddb->data_len);

    si_parse_lib_ddb_add_protect((void*)ddb);
    return ddb;
err:
	si_parse_lib_ddb_release(ddb);
    return NULL;
}

/**
 *  @brief      释放ddb的空间，应用应该在通过GxSubsystm_SiParseLibDdbParse获取
 *  解析好的ddb数据之后某个时刻释放ddb的空间。
 *   
 *  @param       GxSubsystemSiParseDDB* ddb:ddb的存储空间，是通过GxSubsystm_SiParseLibDdbParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibDdbRelease(void* ddb)
{
    uint32_t ret = 0;
    ret = si_parse_lib_ddb_remove_protect(ddb);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParseDDB* ddb is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_ddb_release((GxSubsystemSiParseDDB*)ddb);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

