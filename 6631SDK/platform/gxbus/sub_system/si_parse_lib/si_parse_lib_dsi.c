/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_dsi.c
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

static void si_parse_lib_dsi_release(GxSubsystemSiParseDSI* dsi);
static void si_parse_lib_dsi_add_protect(void* p);
static uint32_t si_parse_lib_dsi_remove_protect(void* p);

static int32_t si_parse_lib_dsi_parse_service_gate_way(GxSubsystemSiParseDsiServiceGateWay* way,
		uint8_t*src,
		uint32_t len);

static uint32_t si_parse_lib_dsi_remove_protect(void* p)
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

static void si_parse_lib_dsi_add_protect(void* p)
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
static void si_parse_lib_dsi_release(GxSubsystemSiParseDSI* dsi)
{
    if(dsi != NULL)
    {
        if(dsi->adaptation_data != NULL)
        {
            GxCore_Free(dsi->adaptation_data);
        }
		if(dsi->service_gate_way.ior.type_id != NULL)
		{
			GxCore_Free(dsi->service_gate_way.ior.type_id);
		}
		if(dsi->service_gate_way.ior.bio_profile.obj_location.object_key != NULL)
		{
			GxCore_Free(dsi->service_gate_way.ior.bio_profile.obj_location.object_key);
		}
        GxCore_Free(dsi);
    }
    return;
}

static int32_t si_parse_lib_dsi_parse_service_gate_way(GxSubsystemSiParseDsiServiceGateWay* way,
		uint8_t*src,
		uint32_t len)
{
    int32_t size = len;
    uint8_t* p = src;	
	
	way->ior.type_id_length = TODATA0_31BIT(p[0],p[1],p[2],p[3]);
	if(way->ior.type_id_length == 0)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("dsi parse service gate way err!!");
		return -1;
#endif
	}
	way->ior.type_id = GxCore_Malloc(way->ior.type_id_length+1);//加上\0
	if(way->ior.type_id == NULL)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
		SI_PARSE_LIB_ERR_PRINTF("dsi parse service gate way no memery!!");
		return -1;
#endif
	}

	memcpy(way->ior.type_id,&p[4],way->ior.type_id_length);
	way->ior.type_id[way->ior.type_id_length] = 0;

	p += 4;//跳过type_id_length
	size -= 4;
	
	p += way->ior.type_id_length;//跳过tpype_id
	size -= way->ior.type_id_length;

	way->ior.tagged_profiles_count = TODATA0_31BIT(p[0],p[1],p[2],p[3]);

	p += 4;//跳过tagged_profiles_count
	size -= 4;
/*解析profile*/
	way->ior.bio_profile.tag = TODATA0_31BIT(p[0],p[1],p[2],p[3]);
	if(way->ior.bio_profile.tag != BIO_PROFILE_TAG)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
		SI_PARSE_LIB_ERR_PRINTF("dsi parse service gate way bio profile err!!");
		return -1;
#endif
	}
	
	way->ior.bio_profile.data_length = TODATA0_31BIT(p[4],p[5],p[6],p[7]);
	way->ior.bio_profile.data_byte_order = TODATA0_7BIT(p[8]);
	way->ior.bio_profile.lite_component_count = TODATA0_7BIT(p[9]);

	p += 10;//跳过bio profile头部
	size -= 10;

	/*解析obj location*/

	way->ior.bio_profile.obj_location.component_tag = TODATA0_31BIT(p[0],p[1],p[2],p[3]);
	if(way->ior.bio_profile.obj_location.component_tag != BIO_OBJ_LOCATION_TAG)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
		SI_PARSE_LIB_ERR_PRINTF("dsi parse service gate way bio profile bio location err!!");
		return -1;
#endif
	}
	way->ior.bio_profile.obj_location.component_length = TODATA0_7BIT(p[4]);
	way->ior.bio_profile.obj_location.carousel_id = TODATA0_31BIT(p[5],p[6],p[7],p[8]);
	way->ior.bio_profile.obj_location.module_id = TODATA0_15BIT(p[9],p[10]);
	way->ior.bio_profile.obj_location.version_major = TODATA0_7BIT(p[11]);
	way->ior.bio_profile.obj_location.version_minor = TODATA0_7BIT(p[12]);
	way->ior.bio_profile.obj_location.object_key_length = TODATA0_7BIT(p[13]);
	if(way->ior.bio_profile.obj_location.object_key_length == 0 ||
			way->ior.bio_profile.obj_location.object_key_length > 0x4)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
		SI_PARSE_LIB_ERR_PRINTF("dsi parse service gate way bio profile bio location err!!");
		return -1;
#endif
	}
	way->ior.bio_profile.obj_location.object_key = GxCore_Malloc( way->ior.bio_profile.obj_location.object_key_length);
	if(way->ior.bio_profile.obj_location.object_key == NULL)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
		SI_PARSE_LIB_ERR_PRINTF("dsi parse service gate way no memery!!");
		return -1;
#endif
	}
	memcpy(way->ior.bio_profile.obj_location.object_key,&p[14],way->ior.bio_profile.obj_location.object_key_length);

	return 0;
}

/**
 *  @brief      把一段section数据解析成dsi表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      dsi数据
 *               NULL：发生错误
 */
GxSubsystemSiParseDSI* GxSubsystm_SiParseLibDsiParse(uint8_t* section,uint32_t size)
{
    GxSubsystemSiParseDSI* dsi = NULL;
    int32_t len = (int32_t)size;
    uint8_t* p = section;
    int32_t ret = 0;
	
	if(section == NULL || size < 8)//dsi的头部8字节
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("section is NULL or size is err!!");
#endif
        goto err;
    }
    dsi = GxCore_Malloc(sizeof(GxSubsystemSiParseDSI));
    if(dsi == NULL)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse dsi no memery!!");
#endif
        goto err;
    }
    memset(dsi,0,sizeof(GxSubsystemSiParseDSI));
    dsi->table_id = TODATA0_7BIT(p[0]);
    dsi->section_syntax_indicator = TODATA7BIT(p[1]);
    dsi->private_indicator = TODATA6BIT(p[1]);
    dsi->section_length = TODATA0_11BIT(p[1],p[2]);
    dsi->table_id_extension = TODATA0_15BIT(p[3],p[4]);
    dsi->version_number = TODATA1_5BIT(p[5]);
    dsi->current_next_indicator = TODATA0BIT(p[5]);
    dsi->section_number = TODATA0_7BIT(p[6]);
    dsi->last_section_number = TODATA0_7BIT(p[7]);
	
    p += 8;//跳过头部
    len -= 8;//头部解析完成
    len -= 4;//去掉crc

    dsi->protocol_discriminator = TODATA0_7BIT(p[0]);
    dsi->dsmcc_type = TODATA0_7BIT(p[1]);

    dsi->message_id = TODATA0_15BIT(p[2],p[3]);
	if(dsi->message_id != 0x1006)//dsi 
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse dsi err!!");
#endif
        goto err;
	}
    dsi->transaction_id = TODATA0_31BIT(p[4],p[5],p[6],p[7]);
/*有一个字节为reserved*/
	dsi->adaptation_length = TODATA0_7BIT(p[9]);
	dsi->message_length = TODATA0_15BIT(p[10],p[11]);

	p += 12;//跳过dsmcc message header
	len -= 12;

	p += dsi->adaptation_length;//跳过adaptation
	len -= dsi->adaptation_length;

	if(len != dsi->message_length-dsi->adaptation_length)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse dsi length err!!");
#endif
        goto err;
	}

	dsi->server_id1 = TODATA0_31BIT(p[0],p[1],p[2],p[3]);
	dsi->server_id2 = TODATA0_31BIT(p[4],p[5],p[6],p[7]);
	dsi->server_id3 = TODATA0_31BIT(p[8],p[9],p[10],p[11]);
	dsi->server_id4 = TODATA0_31BIT(p[12],p[13],p[14],p[15]);
	dsi->server_id5 = TODATA0_31BIT(p[16],p[17],p[18],p[19]);

	if(dsi->server_id1 != 0xffffffff||
			dsi->server_id2 != 0xffffffff||
			dsi->server_id3 != 0xffffffff||
			dsi->server_id4 != 0xffffffff||
			dsi->server_id5 != 0xffffffff)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
		SI_PARSE_LIB_ERR_PRINTF("parse dsi err!!");
#endif
		goto err;
	}
	dsi->compatibility_des = TODATA0_15BIT(p[20],p[21]);
	if(dsi->compatibility_des != 0x0000)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
		SI_PARSE_LIB_ERR_PRINTF("parse dsi err!!");
#endif
		goto err;
	}
	dsi->private_data_length = TODATA0_15BIT(p[22],p[23]);

/*内蒙古dsmcc码流这个值为0*/
#if 0
	if(dsi->private_data_length == 0)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
		SI_PARSE_LIB_ERR_PRINTF("parse dsi err!!");
#endif
		goto err;
	}
#endif

	p += 24;//跳过sdi 头部
	len -= 24;

	if(dsi->private_data_length > len)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
		SI_PARSE_LIB_ERR_PRINTF("parse dsi err!!");
#endif
		goto err;
	}

    ret = si_parse_lib_dsi_parse_service_gate_way(&(dsi->service_gate_way),p,len);
    if(ret < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse service_gate_way err!!");
#endif
        goto err;
    }
    
    si_parse_lib_dsi_add_protect((void*)dsi);
    return dsi;
err:
	si_parse_lib_dsi_release(dsi);
    return NULL;
}

/**
 *  @brief      释放dsi的空间，应用应该在通过GxSubsystm_SiParseLibDsiParse获取
 *  解析好的dsi数据之后某个时刻释放dsi的空间。
 *   
 *  @param       GxSubsystemSiParseDSI* dsi:dsi的存储空间，是通过GxSubsystm_SiParseLibDsiParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibDsiRelease(void* dsi)
{
    uint32_t ret = 0;
    ret = si_parse_lib_dsi_remove_protect(dsi);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParseDSI* dsi is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_dsi_release((GxSubsystemSiParseDSI*)dsi);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

