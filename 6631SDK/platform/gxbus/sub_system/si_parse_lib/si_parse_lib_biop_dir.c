/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_biop_dir.c
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

static void si_parse_lib_biop_dir_release(GxSubsystemSiParseBIOPDIR* biop_dir);
static void si_parse_lib_biop_dir_add_protect(void* p);
static uint32_t si_parse_lib_biop_dir_remove_protect(void* p);
static int32_t si_parse_lib_biop_dir_parse_iop_ior(GxSubsystemSiParseIORDes* iop_ior,
		uint8_t*src,
		uint32_t len);

static uint32_t si_parse_lib_biop_dir_remove_protect(void* p)
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

static void si_parse_lib_biop_dir_add_protect(void* p)
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
static void si_parse_lib_biop_dir_release(GxSubsystemSiParseBIOPDIR* biop_dir)
{
	int32_t i = 0;
    if(biop_dir != NULL)
    {
        if(biop_dir->object_key != NULL)
        {
            GxCore_Free(biop_dir->object_key);
        }
		if(biop_dir->object_kind != NULL)
		{
			GxCore_Free(biop_dir->object_kind);
		}
		if(biop_dir->object_info != NULL)
		{
			GxCore_Free(biop_dir->object_info);
		}
		if(biop_dir->service_context_list != NULL)
		{
			GxCore_Free(biop_dir->service_context_list);
		}
		if(biop_dir->binding != NULL)
		{
			for(i=0; i<biop_dir->bindings_count; i++)
			{
				if(biop_dir->binding[i].id != NULL)
				{
					GxCore_Free(biop_dir->binding[i].id);
				}
				if(biop_dir->binding[i].kind != NULL)
				{
					GxCore_Free(biop_dir->binding[i].kind);
				}
				if(biop_dir->binding[i].ior.type_id != NULL)
				{
					GxCore_Free(biop_dir->binding[i].ior.type_id);
				}
				if(biop_dir->binding[i].ior.bio_profile.obj_location.object_key != NULL)
				{
					GxCore_Free(biop_dir->binding[i].ior.bio_profile.obj_location.object_key);
				}
			}
			GxCore_Free(biop_dir->binding);
		}
        GxCore_Free(biop_dir);
    }
	return;
}

static int32_t si_parse_lib_biop_dir_parse_iop_ior(GxSubsystemSiParseIORDes* iop_ior,
		uint8_t*src,
		uint32_t len)
{
    int32_t size = len;
    uint8_t* p = src;
	uint8_t* p_temp = NULL;
	
	iop_ior->type_id_length = TODATA0_31BIT(p[0],p[1],p[2],p[3]);
	if(iop_ior->type_id_length == 0)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("dir parse ior err!!");
		return -1;
#endif
	}
	iop_ior->type_id = GxCore_Malloc(iop_ior->type_id_length+1);//加上\0
	if(iop_ior->type_id == NULL)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
		SI_PARSE_LIB_ERR_PRINTF("dir parse ior no memery!!");
		return -1;
#endif
	}

	memcpy(iop_ior->type_id,&p[4],iop_ior->type_id_length);
	iop_ior->type_id[iop_ior->type_id_length] = 0;

	p += 4;//跳过type_id_length
	size -= 4;
	
	p += iop_ior->type_id_length;//跳过tpype_id
	size -= iop_ior->type_id_length;

	iop_ior->tagged_profiles_count = TODATA0_31BIT(p[0],p[1],p[2],p[3]);//假设只有一个bio_profile 不然烦死了

	p += 4;//跳过tagged_profiles_count
	size -= 4;
/*解析profile*/
	p_temp = p;

	iop_ior->bio_profile.tag = TODATA0_31BIT(p_temp[0],p_temp[1],p_temp[2],p_temp[3]);
	if(iop_ior->bio_profile.tag != BIO_PROFILE_TAG)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
		SI_PARSE_LIB_ERR_PRINTF("dir parse ior bio profile err!!");
		return -1;
#endif
	}
	
	iop_ior->bio_profile.data_length = TODATA0_31BIT(p_temp[4],p_temp[5],p_temp[6],p_temp[7]);
	iop_ior->bio_profile.data_byte_order = TODATA0_7BIT(p_temp[8]);
	iop_ior->bio_profile.lite_component_count = TODATA0_7BIT(p_temp[9]);

	p_temp += 10;//跳过bio profile头部

	/*解析obj location*/

	iop_ior->bio_profile.obj_location.component_tag = TODATA0_31BIT(p_temp[0],p_temp[1],p_temp[2],p_temp[3]);
	if(iop_ior->bio_profile.obj_location.component_tag != BIO_OBJ_LOCATION_TAG)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
		SI_PARSE_LIB_ERR_PRINTF("dir parse ior bio profile bio location err!!");
		return -1;
#endif
	}
	iop_ior->bio_profile.obj_location.component_length = TODATA0_7BIT(p_temp[4]);
	iop_ior->bio_profile.obj_location.carousel_id = TODATA0_31BIT(p_temp[5],p_temp[6],p_temp[7],p_temp[8]);
	iop_ior->bio_profile.obj_location.module_id = TODATA0_15BIT(p_temp[9],p_temp[10]);
	iop_ior->bio_profile.obj_location.version_major = TODATA0_7BIT(p_temp[11]);
	iop_ior->bio_profile.obj_location.version_minor = TODATA0_7BIT(p_temp[12]);
	iop_ior->bio_profile.obj_location.object_key_length = TODATA0_7BIT(p_temp[13]);
	if(iop_ior->bio_profile.obj_location.object_key_length == 0 ||
			iop_ior->bio_profile.obj_location.object_key_length > 0x4)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
		SI_PARSE_LIB_ERR_PRINTF("dir parse ior bio profile bio location err!!");
		return -1;
#endif
	}
	iop_ior->bio_profile.obj_location.object_key = GxCore_Malloc( iop_ior->bio_profile.obj_location.object_key_length);
	if(iop_ior->bio_profile.obj_location.object_key == NULL)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
		SI_PARSE_LIB_ERR_PRINTF("dir parse ior no memery!!");
		return -1;
#endif
	}
	memcpy(iop_ior->bio_profile.obj_location.object_key,&p_temp[14],iop_ior->bio_profile.obj_location.object_key_length);

	p += 8+iop_ior->bio_profile.data_length; //跳过该profile
	size -= 8+iop_ior->bio_profile.data_length;

	return len - size;
}

/**
 *  @brief      把一段section数据解析成biop_dir表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      biop_dir数据
 *               NULL：发生错误
 */
GxSubsystemSiParseBIOPDIR* GxSubsystm_SiParseLibBiopDirParse(uint8_t* section,uint32_t size)
{
    GxSubsystemSiParseBIOPDIR* biop_dir = NULL;
    int32_t len = (int32_t)size;
    uint8_t* p = section;
	int32_t i = 0;
	int32_t ret = 0;
	
    biop_dir = GxCore_Malloc(sizeof(GxSubsystemSiParseBIOPDIR));
    if(biop_dir == NULL)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse biop_dir no memery!!");
#endif
        goto err;
    }
    memset(biop_dir,0,sizeof(GxSubsystemSiParseBIOPDIR));
    biop_dir->magic = TODATA0_31BIT(p[0],p[1],p[2],p[3]);
	if(biop_dir->magic != BIO__MAGIC)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse biop_dir magic err!!");
#endif
        goto err;
	}
	biop_dir->version_major = TODATA0_7BIT(p[4]);
	biop_dir->version_minor= TODATA0_7BIT(p[5]);
	biop_dir->byte_order = TODATA0_7BIT(p[6]);
	biop_dir->message_type = TODATA0_7BIT(p[7]);
	biop_dir->message_size = TODATA0_31BIT(p[8],p[9],p[10],p[11]);
	biop_dir->object_key_length = TODATA0_7BIT(p[12]);
	if(biop_dir->object_key_length == 0)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse biop_dir object_key len err !!");
#endif
        goto err;
	}
	biop_dir->object_key = GxCore_Malloc(biop_dir->object_key_length);
	if(biop_dir->object_key == NULL)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse biop_dir object_key no memory!!");
#endif
        goto err;
	}
	memcpy(biop_dir->object_key, &p[13], biop_dir->object_key_length);

	p += 13;//跳过头
	len -= 13;
	p += biop_dir->object_key_length;//跳过object_key
	len -= biop_dir->object_key_length;

	biop_dir->object_kind_length = TODATA0_31BIT(p[0],p[1],p[2],p[3]);
	if(biop_dir->object_kind_length>0x4&&
			biop_dir->object_kind_length == 0)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse biop_dir object kind len err!!");
#endif
        goto err;
	}

	biop_dir->object_kind = GxCore_Malloc(biop_dir->object_kind_length);
	if(biop_dir->object_kind == NULL)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse biop_dir object kind no memory!!");
#endif
        goto err;
	}
	
	memcpy(biop_dir->object_kind, &p[4], biop_dir->object_kind_length);
	if(strcmp((const char*)(biop_dir->object_kind),"dir") != 0 &&
			strcmp((const char*)(biop_dir->object_kind),"srg") != 0)
	{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse biop_dir object kind err!!");
#endif
        goto err;
	}
	
	p += 4;//跳过object_kind_length
	len -= 4;
	p += biop_dir->object_kind_length;//跳过object_kind
	len -= biop_dir->object_kind_length;

	biop_dir->object_info_length = TODATA0_15BIT(p[0],p[1]);
	
	p += 2;//跳过object_info_length
	len -= 2;
	p += biop_dir->object_info_length;//跳过object_info
	len -= biop_dir->object_info_length;

	biop_dir->service_context_list_count = TODATA0_7BIT(p[0]);

	p += 1;//跳过service_context_list_count
	len -= 1;
	p += biop_dir->service_context_list_count;//跳过service_context_list
	len -= biop_dir->service_context_list_count;
	
	biop_dir->message_body_length = TODATA0_31BIT(p[0],p[1],p[2],p[3]);
	biop_dir->bindings_count = TODATA0_15BIT(p[4],p[5]);
	p += 6;//跳过message_body_length bindings_count
	len -= 6;
	if(biop_dir->bindings_count != 0)
	{
		biop_dir->binding = GxCore_Mallocz(sizeof(GxSubsystemSiParseBIOPBINDING)*biop_dir->bindings_count);
		if(biop_dir->binding == NULL)
		{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
			SI_PARSE_LIB_ERR_PRINTF("parse biop_dir binding no memory!!");
#endif
			goto err;
		}
		for(i=0; i<biop_dir->bindings_count; i++)
		{
			biop_dir->binding[i].name_count = TODATA0_7BIT(p[0]);
			if(biop_dir->binding[i].name_count != 1)
			{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
				SI_PARSE_LIB_ERR_PRINTF("parse biop_dir name_count !=1 !!");
#endif
				goto err;
			}
			biop_dir->binding[i].id_length = TODATA0_7BIT(p[1]);
			if(biop_dir->binding[i].id_length != 0)
			{
				biop_dir->binding[i].id = GxCore_Mallocz(biop_dir->binding[i].id_length+1);//不上一个\0
				if(biop_dir->binding[i].id == NULL)
				{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
					SI_PARSE_LIB_ERR_PRINTF("parse biop_dir binding no memory!!");
#endif
					goto err;
				}
				memcpy(biop_dir->binding[i].id, &p[2], biop_dir->binding[i].id_length);
			}
			p += 2 + biop_dir->binding[i].id_length;
			len -= 2 + biop_dir->binding[i].id_length;

			biop_dir->binding[i].kind_length = TODATA0_7BIT(p[0]);
			if(biop_dir->binding[i].kind_length == 0)
			{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
					SI_PARSE_LIB_ERR_PRINTF("parse biop_dir kind_length!!");
#endif
					goto err;
			}
			biop_dir->binding[i].kind = GxCore_Mallocz( biop_dir->binding[i].kind_length);
			if(biop_dir->binding[i].kind == NULL)
			{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
					SI_PARSE_LIB_ERR_PRINTF("parse biop_dir binding no memory!!");
#endif
					goto err;
			}
			memcpy(biop_dir->binding[i].kind, &p[1], biop_dir->binding[i].kind_length);
			p += 1+biop_dir->binding[i].kind_length;
			len -= 1+biop_dir->binding[i].kind_length;

			biop_dir->binding[i].binding_type = TODATA0_7BIT(p[0]);

			p += 1;
			len -= 1;
			ret  = si_parse_lib_biop_dir_parse_iop_ior(&(biop_dir->binding[i].ior),p,len);
			if(ret < 0)
			{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
				SI_PARSE_LIB_ERR_PRINTF("parse biop_dir ior err!!");
#endif
				goto err;
			}
			p += ret;
			len -= ret;

			p += TODATA0_15BIT(p[0], p[1]) +1;//跳过obj info
			len -= TODATA0_15BIT(p[0], p[1]) +1;
		}
	}
    
	si_parse_lib_biop_dir_add_protect((void*)biop_dir);
    return biop_dir;
err:
	si_parse_lib_biop_dir_release(biop_dir);
    return NULL;
}

/**
 *  @brief      释放biop_dir的空间，应用应该在通过GxSubsystm_SiParseLibBiopDirParse获取
 *  解析好的biop_dir数据之后某个时刻释放biop_dir的空间。
 *   
 *  @param       GxSubsystemSiParseBIOPDIR* biop_dir:biop_dir的存储空间，是通过GxSubsystm_SiParseLibBiopDirParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibBiopDirRelease(GxSubsystemSiParseBIOPDIR* biop_dir)
{
    uint32_t ret = 0;
    ret = si_parse_lib_biop_dir_remove_protect((void*)biop_dir);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParseBIOPDIR* biop_dir is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_biop_dir_release(biop_dir);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

