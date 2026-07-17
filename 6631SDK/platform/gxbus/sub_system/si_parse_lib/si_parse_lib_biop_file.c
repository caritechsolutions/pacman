/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_biop_file.c
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

static void si_parse_lib_biop_file_release(GxSubsystemSiParseBIOPFILE* biop_file);
static void si_parse_lib_biop_file_add_protect(void* p);
static uint32_t si_parse_lib_biop_file_remove_protect(void* p);

static uint32_t si_parse_lib_biop_file_remove_protect(void* p)
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

static void si_parse_lib_biop_file_add_protect(void* p)
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
static void si_parse_lib_biop_file_release(GxSubsystemSiParseBIOPFILE* biop_file)
{
    if(biop_file != NULL)
    {
        if(biop_file->object_key != NULL)
        {
            GxCore_Free(biop_file->object_key);
        }
		if(biop_file->object_kind != NULL)
		{
			GxCore_Free(biop_file->object_kind);
		}
		if(biop_file->object_info != NULL)
		{
			GxCore_Free(biop_file->object_info);
		}
		if(biop_file->service_context_list != NULL)
		{
			GxCore_Free(biop_file->service_context_list);
		}
		if(biop_file->content != NULL)
		{
			GxCore_Free(biop_file->content);
		}
        GxCore_Free(biop_file);
    }
	return;
}

/**
 *  @brief      把一段section数据解析成biop_file表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      biop_file数据
 *               NULL：发生错误
 */
GxSubsystemSiParseBIOPFILE* GxSubsystm_SiParseLibBiopFileParse(uint8_t* section,uint32_t size)
{
    GxSubsystemSiParseBIOPFILE* biop_file = NULL;
    int32_t len = (int32_t)size;
    uint8_t* p = section;
	
    biop_file = GxCore_Malloc(sizeof(GxSubsystemSiParseBIOPFILE));
    if(biop_file == NULL)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse biop_file no memery!!");
#endif
        goto err;
    }
    memset(biop_file,0,sizeof(GxSubsystemSiParseBIOPFILE));
    biop_file->magic = TODATA0_31BIT(p[0],p[1],p[2],p[3]);
	if(biop_file->magic != BIO__MAGIC)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse biop_file magic err!!");
#endif
        goto err;
	}
	biop_file->version_major = TODATA0_7BIT(p[4]);
	biop_file->version_minor= TODATA0_7BIT(p[5]);
	biop_file->byte_order = TODATA0_7BIT(p[6]);
	biop_file->message_type = TODATA0_7BIT(p[7]);
	biop_file->message_size = TODATA0_31BIT(p[8],p[9],p[10],p[11]);
	biop_file->object_key_length = TODATA0_7BIT(p[12]);
	if(biop_file->object_key_length == 0)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse biop_file object_key len err !!");
#endif
        goto err;
	}
	biop_file->object_key = GxCore_Malloc(biop_file->object_key_length);
	if(biop_file->object_key == NULL)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse biop_file object_key no memory!!");
#endif
        goto err;
	}
	memcpy(biop_file->object_key, &p[13], biop_file->object_key_length);

	p += 13;//跳过头
	len -= 13;
	p += biop_file->object_key_length;//跳过object_key
	len -= biop_file->object_key_length;

	biop_file->object_kind_length = TODATA0_31BIT(p[0],p[1],p[2],p[3]);
	if(biop_file->object_kind_length>0x4&&
			biop_file->object_kind_length == 0)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse biop_file object kind len err!!");
#endif
        goto err;
	}

	biop_file->object_kind = GxCore_Malloc(biop_file->object_kind_length);
	if(biop_file->object_kind == NULL)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse biop_file object kind no memory!!");
#endif
        goto err;
	}
	
	memcpy(biop_file->object_kind, &p[4], biop_file->object_kind_length);
	if(strcmp((const char*)(biop_file->object_kind),"fil") != 0)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse biop_file object kind err!!");
#endif
        goto err;
	}
	
	p += 4;//跳过object_kind_length
	len -= 4;
	p += biop_file->object_kind_length;//跳过object_kind
	len -= biop_file->object_kind_length;

	biop_file->object_info_length = TODATA0_15BIT(p[0],p[1]);
	
	p += 2;//跳过object_info_length
	len -= 2;
	p += biop_file->object_info_length;//跳过object_info
	len -= biop_file->object_info_length;

	biop_file->service_context_list_count = TODATA0_7BIT(p[0]);

	p += 1;//跳过service_context_list_count
	len -= 1;
	p += biop_file->service_context_list_count;//跳过service_context_list
	len -= biop_file->service_context_list_count;
	
	biop_file->message_body_length = TODATA0_31BIT(p[0],p[1],p[2],p[3]);
	biop_file->content_length = TODATA0_31BIT(p[4],p[5],p[6],p[7]);
	if(biop_file->content_length != 0)
	{
		biop_file->content = GxCore_Mallocz(biop_file->content_length);
		if(biop_file->content == NULL)
		{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
			SI_PARSE_LIB_ERR_PRINTF("parse biop_file  content no memory!!");
#endif
			goto err;
		}
		memcpy(biop_file->content, &p[8], biop_file->content_length);
	}
    
	si_parse_lib_biop_file_add_protect((void*)biop_file);
    return biop_file;
err:
	si_parse_lib_biop_file_release(biop_file);
    return NULL;
}

/**
 *  @brief      释放biop_file的空间，应用应该在通过GxSubsystm_SiParseLibBiopFileParse获取
 *  解析好的biop_file数据之后某个时刻释放biop_file的空间。
 *   
 *  @param       GxSubsystemSiParseBIOPFILE* biop_file:biop_file的存储空间，是通过GxSubsystm_SiParseLibBiopFileParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibBiopFileRelease(GxSubsystemSiParseBIOPFILE* biop_file)
{
    uint32_t ret = 0;
    ret = si_parse_lib_biop_file_remove_protect((void*)biop_file);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParseBIOPFILE* biop_file is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_biop_file_release(biop_file);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

