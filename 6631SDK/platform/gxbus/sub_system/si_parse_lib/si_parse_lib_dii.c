/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_dii.c
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

static void si_parse_lib_dii_release(GxSubsystemSiParseDII* dii);
static void si_parse_lib_dii_add_protect(void* p);
static uint32_t si_parse_lib_dii_remove_protect(void* p);

static uint32_t si_parse_lib_dii_remove_protect(void* p)
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

static void si_parse_lib_dii_add_protect(void* p)
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
static void si_parse_lib_dii_release(GxSubsystemSiParseDII* dii)
{
    if(dii != NULL)
    {
        if(dii->adaptation_data != NULL)
        {
            GxCore_Free(dii->adaptation_data);
        }
		if(dii->module != NULL)
		{
			GxCore_Free(dii->module);
		}
        GxCore_Free(dii);
    }
	return;
}


/**
 *  @brief      把一段section数据解析成dii表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      dii数据
 *               NULL：发生错误
 */
GxSubsystemSiParseDII* GxSubsystm_SiParseLibDiiParse(uint8_t* section,uint32_t size)
{
    GxSubsystemSiParseDII* dii = NULL;
    int32_t len = (int32_t)size;
    uint8_t* p = section;
	int32_t i = 0;
	
	if(section == NULL || size < 8)//dii的头部8字节
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("section is NULL or size is err!!");
#endif
        goto err;
    }
    dii = GxCore_Malloc(sizeof(GxSubsystemSiParseDII));
    if(dii == NULL)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse dii no memery!!");
#endif
        goto err;
    }
    memset(dii,0,sizeof(GxSubsystemSiParseDII));
    dii->table_id = TODATA0_7BIT(p[0]);
    dii->section_syntax_indicator = TODATA7BIT(p[1]);
    dii->private_indicator = TODATA6BIT(p[1]);
    dii->section_length = TODATA0_11BIT(p[1],p[2]);
    dii->table_id_extension = TODATA0_15BIT(p[3],p[4]);
    dii->version_number = TODATA1_5BIT(p[5]);
    dii->current_next_indicator = TODATA0BIT(p[5]);
    dii->section_number = TODATA0_7BIT(p[6]);
    dii->last_section_number = TODATA0_7BIT(p[7]);
	
    p += 8;//跳过头部
    len -= 8;//头部解析完成
    len -= 4;//去掉crc

    dii->protocol_discriminator = TODATA0_7BIT(p[0]);
    dii->dsmcc_type = TODATA0_7BIT(p[1]);

    dii->message_id = TODATA0_15BIT(p[2],p[3]);
	if(dii->message_id != 0x1002)//dii 
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse dii err!!");
#endif
        goto err;
	}
    dii->transaction_id = TODATA0_31BIT(p[4],p[5],p[6],p[7]);
/*有一个字节为reserved*/
	dii->adaptation_length = TODATA0_7BIT(p[9]);
	dii->message_length = TODATA0_15BIT(p[10],p[11]);

	p += 12;//跳过dsmcc message header
	len -= 12;

	p += dii->adaptation_length;//跳过adaptation
	len -= dii->adaptation_length;

	if(len != dii->message_length-dii->adaptation_length)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse dii length err!!");
#endif
        goto err;
	}

	dii->download_id = TODATA0_31BIT(p[0],p[1],p[2],p[3]);
	dii->block_size = TODATA0_15BIT(p[4],p[5]);

	dii->window_size = TODATA0_7BIT(p[6]);//dc和oc都不使用 应该为0x00
	if(dii->window_size != 0x00)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse dii window_size err!!");
#endif
        goto err;
	}
	dii->ack_perid = TODATA0_7BIT(p[7]);//dc和oc都不使用 应该为0x00
	if(dii->ack_perid != 0x00)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse dii ack_perid err!!");
#endif
        goto err;
	}


	dii->tc_download_window = TODATA0_31BIT(p[8],p[9],p[10],p[11]);//dc和oc都不使用 应该为0x00000000
	if(dii->ack_perid != 0x00000000)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse dii tc_download_window err!!");
#endif
        goto err;
	}

	dii->tc_download_scenario = TODATA0_31BIT(p[12],p[13],p[14],p[15]);
	dii->compatibility_des = TODATA0_15BIT(p[16],p[17]);//应该为0x0000
	if(dii->compatibility_des != 0x0000)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse dii compatibility_des err!!");
#endif
        goto err;
	}
	dii->number_modules =TODATA0_15BIT(p[18],p[19]);
	if(dii->number_modules != 0)
	{
		dii->module = GxCore_Malloc(sizeof(GxSubsystemSiParseDiiModuleInfo)*dii->number_modules);
		if(dii->module == NULL)
		{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
			SI_PARSE_LIB_ERR_PRINTF("parse dii no memery!!");
#endif
			goto err;
		}
		p += 20;
		len -= 20;
		for(i=0; i<dii->number_modules; i++)
		{
			dii->module[i].module_id = TODATA0_15BIT(p[0],p[1]);

			dii->module[i].module_size = TODATA0_31BIT(p[2],p[3],p[4],p[5]);

			dii->module[i].module_version = TODATA0_7BIT(p[6]);
			dii->module[i].module_length = TODATA0_7BIT(p[7]);

			p += 8;
			len -= 8;
			p += dii->module[i].module_length;
			len -= dii->module[i].module_length;
			//GxSubsystemSiParseBiopModule* module;//貌似可以不管先
			//private数据 可以不管
		}
	}

    si_parse_lib_dii_add_protect((void*)dii);
    return dii;
err:
	si_parse_lib_dii_release(dii);
    return NULL;
}

/**
 *  @brief      释放dii的空间，应用应该在通过GxSubsystm_SiParseLibDiiParse获取
 *  解析好的dii数据之后某个时刻释放dii的空间。
 *   
 *  @param       GxSubsystemSiParseDII* dii:dii的存储空间，是通过GxSubsystm_SiParseLibDiiParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibDiiRelease(void *dii)
{
    uint32_t ret = 0;
    ret = si_parse_lib_dii_remove_protect(dii);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParseDII* dii is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_dii_release((GxSubsystemSiParseDII *)dii);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

