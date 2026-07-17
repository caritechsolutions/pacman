/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_sdt.c
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

static void si_parse_lib_sdt_release(GxSubsystemSiParseSDT* sdt);
static void si_parse_lib_sdt_info_release(GxSubsystemSiParseSdtInfo* sdt_info);
static void si_parse_lib_sdt_add_protect(void* p);
static uint32_t si_parse_lib_sdt_remove_protect(void* p);
static int32_t si_parse_lib_sdt_parse_loop(GxSubsystemSiParseSDT* sdt,uint8_t* src,uint32_t len); 
static int32_t si_parse_lib_sdt_parse_des(GxSubsystemSiParseSdtInfo* sdt_info,uint8_t* src,uint32_t len);
static int32_t si_parse_lib_sdt_nvod(GxSubsystemSiParseNvodDes* nvod,uint8_t* src,uint32_t len);
static int32_t si_parse_lib_sdt_muliti_service_name(GxSubsystemSiParseMulitiServiceDes* service_name,uint8_t* src,uint32_t len);

/*释放loop空间*/
static void si_parse_lib_sdt_info_release(GxSubsystemSiParseSdtInfo* sdt_info)
{
    uint32_t i = 0;
    uint32_t j = 0;
    if(sdt_info)
    {
        if(sdt_info->stuffing)
        {
            for(i=0; i<sdt_info->stuffing_count; i++)
            {
                if(sdt_info->stuffing[i].stuffing_byte)
                {
                    GxCore_Free(sdt_info->stuffing[i].stuffing_byte);
                }
            }
            GxCore_Free(sdt_info->stuffing);
        }
        if(sdt_info->bouquet_name)
        {
            for(i=0; i<sdt_info->bouquet_name_count; i++)
            {
                if(sdt_info->bouquet_name[i].name)
                {
                    GxCore_Free(sdt_info->bouquet_name[i].name);
                }
            }
            GxCore_Free(sdt_info->bouquet_name);
        }
        if(sdt_info->service)
        {
            for(i=0; i<sdt_info->service_count; i++)
            {
                if(sdt_info->service[i].provider_name)
                {
                    GxCore_Free(sdt_info->service[i].provider_name);
                }
                if(sdt_info->service[i].service_name)
                {
                    GxCore_Free(sdt_info->service[i].service_name);
                }
            }
            GxCore_Free(sdt_info->service);
        }
        if(sdt_info->country_availability)
        {
            for(i=0; i<sdt_info->country_availability_count; i++)
            {
                if(sdt_info->country_availability[i].country)
                {
                    GxCore_Free(sdt_info->country_availability[i].country);
                }
            }
            GxCore_Free(sdt_info->country_availability);
        }
        if(sdt_info->NVOD_reference)
        {
            for(i=0; i<sdt_info->NVOD_reference_count; i++)
            {
                if(sdt_info->NVOD_reference[i].descriptor)
                {
                    GxCore_Free(sdt_info->NVOD_reference[i].descriptor);
                }
            }
            GxCore_Free(sdt_info->NVOD_reference);
        }
        if(sdt_info->time_shifted_service)
        {
            GxCore_Free(sdt_info->time_shifted_service);
        }
        if(sdt_info->ca_identifier)
        {
            for(i=0 ;i<sdt_info->ca_identifier_count; i++)
            {
                if(sdt_info->ca_identifier[i].ca_system_id)
                {
                    GxCore_Free(sdt_info->ca_identifier[i].ca_system_id);
                }
            }
            GxCore_Free(sdt_info->ca_identifier);
        }
        if(sdt_info->muliti_service_name)
        {
            for(i=0 ;i<sdt_info->muliti_service_name_count; i++)
            {
                if(sdt_info->muliti_service_name[i].service_name)
                {
                    for(j=0; j<sdt_info->muliti_service_name[i].service_name_count; j++)
                    {
                        if(sdt_info->muliti_service_name[i].service_name[j].service_provider_name)
                        {
                            GxCore_Free(sdt_info->muliti_service_name[i].service_name[j].service_provider_name);
                        }
                        if(sdt_info->muliti_service_name[i].service_name[j].service_name)
                        {
                            GxCore_Free(sdt_info->muliti_service_name[i].service_name[j].service_name);
                        }
                    }
                    GxCore_Free(sdt_info->muliti_service_name->service_name);
                }
            }
            GxCore_Free(sdt_info->muliti_service_name);
        }
        if(sdt_info->data_broadcast)
        {
            for(i=0; i<sdt_info->data_broadcast_count; i++)
            {
                if(sdt_info->data_broadcast[i].selector_type)
                {
                    GxCore_Free(sdt_info->data_broadcast[i].selector_type);
                }
                if(sdt_info->data_broadcast[i].text)
                {
                    GxCore_Free(sdt_info->data_broadcast[i].text);
                }
            }
            GxCore_Free(sdt_info->data_broadcast);
        }
    }
    return;
}

/*释放sdt的空间*/
static void si_parse_lib_sdt_release(GxSubsystemSiParseSDT* sdt)
{
    uint32_t i = 0;
    if(sdt)
    {
        if(sdt->sdt_info)
        {
            for(i=0; i<sdt->sdt_info_count; i++)
            {
                si_parse_lib_sdt_info_release(&(sdt->sdt_info[i]));
            }
            GxCore_Free(sdt->sdt_info);
        }
        GxCore_Free(sdt);
    }
    return;
}

static void si_parse_lib_sdt_add_protect(void* p)
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

static uint32_t si_parse_lib_sdt_remove_protect(void* p)
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

static int32_t si_parse_lib_sdt_nvod(GxSubsystemSiParseNvodDes* nvod,uint8_t* src,uint32_t len)
{
    uint32_t descriptor_count = 0;
    uint8_t* p = src;
    int32_t size = len;
    while(size > 0)
    {
        descriptor_count++;
        nvod->descriptor = GxCore_Realloc(nvod->descriptor,sizeof(GxSubsystemSiParseNvodInfo)*descriptor_count);
        if(nvod->descriptor == NULL)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("parse nvod no memery!!");
#endif  
            goto err;
        }      
        nvod->descriptor[descriptor_count-1].transport_stream_id = TODATA0_15BIT(p[0],p[1]);
        nvod->descriptor[descriptor_count-1].original_network_id = TODATA0_15BIT(p[2],p[3]);
        nvod->descriptor[descriptor_count-1].service_id = TODATA0_15BIT(p[4],p[5]);
        p += 6;//跳过该描述
        size -= 6;
    }
    nvod->descriptor_count = descriptor_count;
    return size;
err:
    nvod->descriptor_count = descriptor_count;
    return -1;
}
static int32_t si_parse_lib_sdt_muliti_service_name(GxSubsystemSiParseMulitiServiceDes* service_name,uint8_t* src,uint32_t len)
{
    uint32_t descriptor_count = 0;
    uint8_t* p = src;
    int32_t size = len;
    while(size > 0)
    {
        descriptor_count++;
        service_name->service_name = GxCore_Realloc(service_name->service_name,
                sizeof(GxSubsystemSiParseMulitiService)*descriptor_count);
        if(service_name->service_name == NULL)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("parse service_name no memery!!");
#endif  
            goto err;
        }      
        memcpy(service_name->service_name[descriptor_count-1].language,p,3);
        service_name->service_name[descriptor_count-1].service_provider_name_length = TODATA0_7BIT(p[3]);
		service_name->service_name[descriptor_count-1].service_provider_name = NULL;
		if(service_name->service_name[descriptor_count-1].service_provider_name_length != 0)
		{
			service_name->service_name[descriptor_count-1].service_provider_name = GxCore_Malloc(service_name->service_name[descriptor_count-1].service_provider_name_length);
			if(service_name->service_name[descriptor_count-1].service_provider_name == NULL)
			{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
				SI_PARSE_LIB_ERR_PRINTF("parse service_name no memery!!");
#endif  
				goto err;
			}
			memcpy(service_name->service_name[descriptor_count-1].service_provider_name,
					&p[4],service_name->service_name[descriptor_count-1].service_provider_name_length);
		}
		p += 4;//跳过该描述
		size -= 4;
		p += service_name->service_name[descriptor_count-1].service_provider_name_length;//跳过该描述
		size -= service_name->service_name[descriptor_count-1].service_provider_name_length;
		service_name->service_name[descriptor_count-1].service_name_length = TODATA0_7BIT(p[0]);
		service_name->service_name[descriptor_count-1].service_name = NULL;
		if(service_name->service_name[descriptor_count-1].service_name_length != 0)
		{
			service_name->service_name[descriptor_count-1].service_name = GxCore_Malloc(service_name->service_name[descriptor_count-1].service_name_length);
			if(service_name->service_name[descriptor_count-1].service_name == NULL)
			{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
				SI_PARSE_LIB_ERR_PRINTF("parse service_name no memery!!");
#endif  
				goto err;
			}
			memcpy(service_name->service_name[descriptor_count-1].service_name,
					&p[1],service_name->service_name[descriptor_count-1].service_name_length);
		}
		p += 1;//跳过该描述
		size -= 1;
		p += service_name->service_name[descriptor_count-1].service_name_length;//跳过该描述
		size -= service_name->service_name[descriptor_count-1].service_name_length;
	}
    service_name->service_name_count = descriptor_count;
    return size;
err:
    service_name->service_name_count = descriptor_count;
    return -1;
}
static int32_t si_parse_lib_sdt_parse_des(GxSubsystemSiParseSdtInfo* sdt_info,uint8_t* src,uint32_t len)
{
    int32_t size = len;
    uint8_t* p = src;
    int32_t ret = 0;
    uint32_t descriptor_length = 0;
    uint32_t stuffing_count = 0;
    uint32_t bouquet_name_count = 0;
    uint32_t service_count = 0;
    uint32_t country_count = 0;
    uint32_t nvod_count = 0;
    uint32_t time_shifted_count = 0;
    uint32_t ca_identifier_count = 0;
    uint32_t muliti_service_name_count = 0;
    uint32_t data_broadcast_count = 0;
    
    while(size > 0)
    {
        switch(p[0])
        {
            case SI_PARSE_LIB_STUFFING_DES:
                stuffing_count++;
                sdt_info->stuffing = GxCore_Realloc(sdt_info->stuffing,sizeof(GxSubsystemSiParseStuffingDes)*stuffing_count);
                if(sdt_info->stuffing == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
                    goto err;
                }      
                sdt_info->stuffing[stuffing_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                sdt_info->stuffing[stuffing_count-1].descriptor_length = TODATA0_7BIT(p[1]);
				sdt_info->stuffing[stuffing_count-1].stuffing_byte = NULL;
				if(sdt_info->stuffing[stuffing_count-1].descriptor_length != 0)
				{
					sdt_info->stuffing[stuffing_count-1].stuffing_byte = GxCore_Malloc(sdt_info->stuffing[stuffing_count-1].descriptor_length);
					if(sdt_info->stuffing[stuffing_count-1].stuffing_byte == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
						goto err;
					}
					memcpy(sdt_info->stuffing[stuffing_count-1].stuffing_byte,
							&p[2],sdt_info->stuffing[stuffing_count-1].descriptor_length);
				}
				p += 2;//跳过头
				p += sdt_info->stuffing[stuffing_count-1].descriptor_length;//跳过该tag剩余字节
				size -= 2;
				size -= sdt_info->stuffing[stuffing_count-1].descriptor_length;
				break;

			case SI_PARSE_LIB_BOUQUET_NAME_DES:
				bouquet_name_count++;
				sdt_info->bouquet_name = GxCore_Realloc(sdt_info->bouquet_name,sizeof(GxSubsystemSiParseBouquetDes)*bouquet_name_count);
				if(sdt_info->bouquet_name == NULL)
				{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
                    goto err;
                }      
                sdt_info->bouquet_name[bouquet_name_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                sdt_info->bouquet_name[bouquet_name_count-1].name_length = TODATA0_7BIT(p[1]);
				sdt_info->bouquet_name[bouquet_name_count-1].name = NULL;
				if(sdt_info->bouquet_name[bouquet_name_count-1].name_length != 0)
				{
					sdt_info->bouquet_name[bouquet_name_count-1].name = GxCore_Malloc(sdt_info->bouquet_name[bouquet_name_count-1].name_length);
					if(sdt_info->bouquet_name[bouquet_name_count-1].name == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
						goto err;
					}
					memcpy(sdt_info->bouquet_name[bouquet_name_count-1].name,
							&p[2],sdt_info->bouquet_name[bouquet_name_count-1].name_length);
				}
				p += 2;//跳过头
				p += sdt_info->bouquet_name[bouquet_name_count-1].name_length;//跳过该tag剩余字节
				size -= 2;
				size -= sdt_info->bouquet_name[bouquet_name_count-1].name_length;
				break;

			case SI_PARSE_LIB_SERVICE_DES:
                service_count++;
                sdt_info->service = GxCore_Realloc(sdt_info->service,sizeof(GxSubsystemSiParseServiceDes)*service_count);
                if(sdt_info->service == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
                    goto err;
                }      
                sdt_info->service[service_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                sdt_info->service[service_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                sdt_info->service[service_count-1].service_type = TODATA0_7BIT(p[2]);
                sdt_info->service[service_count-1].service_provider_length = TODATA0_7BIT(p[3]);
				sdt_info->service[service_count-1].provider_name = NULL;
				if(sdt_info->service[service_count-1].service_provider_length != 0)
				{
					sdt_info->service[service_count-1].provider_name = GxCore_Malloc(sdt_info->service[service_count-1].service_provider_length);
					if(sdt_info->service[service_count-1].provider_name == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
						goto err;
					}
					memcpy(sdt_info->service[service_count-1].provider_name,
							&p[4],sdt_info->service[service_count-1].service_provider_length);
				}
				p += 4;//跳过头部
				size -= 4;
				p += sdt_info->service[service_count-1].service_provider_length;//跳过第一个描述
				size -= sdt_info->service[service_count-1].service_provider_length;
				sdt_info->service[service_count-1].service_name_length = TODATA0_7BIT(p[0]);
				sdt_info->service[service_count-1].service_name = NULL;
				if(sdt_info->service[service_count-1].service_name_length != 0)
				{
					sdt_info->service[service_count-1].service_name = 
                        GxCore_Malloc(sdt_info->service[service_count-1].service_name_length);
					if(sdt_info->service[service_count-1].service_name == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
						goto err;
					}
					memcpy(sdt_info->service[service_count-1].service_name,
							&p[1],sdt_info->service[service_count-1].service_name_length);
				}
				p += 1;//跳过第二个描述
				p += sdt_info->service[service_count-1].service_name_length;
				size -= 1;
				size -= sdt_info->service[service_count-1].service_name_length;
				break;

			case SI_PARSE_LIB_COUNTRY_AVAILABILITY_DES:
                country_count++;
                sdt_info->country_availability = GxCore_Realloc(sdt_info->country_availability,sizeof(GxSubsystemSiParseCountryDes)*country_count);
                if(sdt_info->country_availability == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
                    goto err;
                }      
                sdt_info->country_availability[country_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                sdt_info->country_availability[country_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                sdt_info->country_availability[country_count-1].country_availability_flag = TODATA7BIT(p[2]);
                sdt_info->country_availability[country_count-1].country_count = (sdt_info->country_availability[country_count-1].descriptor_length-1)/3;
				sdt_info->country_availability[country_count-1].country = NULL;
				if(sdt_info->country_availability[country_count-1].country_count != 0)
				{
					sdt_info->country_availability[country_count-1].country = GxCore_Malloc(sdt_info->country_availability[country_count-1].descriptor_length-1);
					if(sdt_info->country_availability[country_count-1].country == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
						goto err;
					}
					memcpy(sdt_info->country_availability[country_count-1].country,
							&p[3],sdt_info->country_availability[country_count-1].country_count*3);
				}
				p += 3;//跳过头
				p += sdt_info->country_availability[country_count-1].descriptor_length-1;//跳过该tag剩余字节
				size -= 3;
                size -= sdt_info->country_availability[country_count-1].descriptor_length-1;
                break;

            case SI_PARSE_LIB_NVOD_REFERENCE_DES:
                nvod_count++;
                sdt_info->NVOD_reference = GxCore_Realloc(sdt_info->NVOD_reference,sizeof(GxSubsystemSiParseNvodDes)*nvod_count);
                if(sdt_info->NVOD_reference == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
                    goto err;
                }      
				else
				{
					memset(&sdt_info->NVOD_reference[nvod_count - 1],0,sizeof(GxSubsystemSiParseNvodDes));
				}
                sdt_info->NVOD_reference[nvod_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                sdt_info->NVOD_reference[nvod_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                ret = si_parse_lib_sdt_nvod(&(sdt_info->NVOD_reference[nvod_count-1]),
                            &p[2],sdt_info->NVOD_reference[nvod_count-1].descriptor_length);
                if(ret < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse nvod err!!");
#endif  
                    goto err;
                }
                p += 2;//跳过头
                size -= 2;
                p += sdt_info->NVOD_reference[nvod_count-1].descriptor_length;//跳过该tag剩余字节
                size -= sdt_info->NVOD_reference[nvod_count-1].descriptor_length;
                break;

            case SI_PARSE_LIB_TIME_SHIFTED_SERVICE_DES:
                time_shifted_count++;
                sdt_info->time_shifted_service = GxCore_Realloc(sdt_info->time_shifted_service,
                        sizeof(GxSubsystemSiParseTimeShiftedDes)*time_shifted_count);
                if(sdt_info->time_shifted_service == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
                    goto err;
                }      
                sdt_info->time_shifted_service[time_shifted_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                sdt_info->time_shifted_service[time_shifted_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                sdt_info->time_shifted_service[time_shifted_count-1].reference_service_id = TODATA0_15BIT(p[2],p[3]);
                p += 2;//跳过头
                p += sdt_info->time_shifted_service[time_shifted_count-1].descriptor_length;//跳过该tag剩余字节
                size -= 2;
                size -= sdt_info->time_shifted_service[time_shifted_count-1].descriptor_length;
                break;

            case SI_PARSE_LIB_CA_IDENTIFIER_DES:
                ca_identifier_count++;
                sdt_info->ca_identifier = GxCore_Realloc(sdt_info->ca_identifier,
                        sizeof(GxSubsystemSiParseCaIdentifierDes)*ca_identifier_count);
                if(sdt_info->ca_identifier == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
                    goto err;
                }      
                sdt_info->ca_identifier[ca_identifier_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                sdt_info->ca_identifier[ca_identifier_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                sdt_info->ca_identifier[ca_identifier_count-1].ca_system_count = sdt_info->ca_identifier[ca_identifier_count-1].descriptor_length/2;
				sdt_info->ca_identifier[ca_identifier_count-1].ca_system_id = NULL;
				if(sdt_info->ca_identifier[ca_identifier_count-1].ca_system_count != 0)
				{
					sdt_info->ca_identifier[ca_identifier_count-1].ca_system_id = GxCore_Malloc(sdt_info->ca_identifier[ca_identifier_count-1].descriptor_length);
					if(sdt_info->ca_identifier[ca_identifier_count-1].ca_system_id == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
						goto err;
					}
					memcpy(sdt_info->ca_identifier[ca_identifier_count-1].ca_system_id,
							&p[2],sdt_info->ca_identifier[ca_identifier_count-1].descriptor_length);
				}
				p += 2;//跳过头
				p += sdt_info->ca_identifier[ca_identifier_count-1].descriptor_length;//跳过该tag剩余字节
                size -= 2;
                size -= sdt_info->ca_identifier[ca_identifier_count-1].descriptor_length;
                break;
                
            case SI_PARSE_LIB_MULTILINGUAL_SERVICE_NAME_DES:
                muliti_service_name_count++;
                sdt_info->muliti_service_name = GxCore_Realloc(sdt_info->muliti_service_name,
                        sizeof(GxSubsystemSiParseMulitiServiceDes)*muliti_service_name_count);
                if(sdt_info->muliti_service_name == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
                    goto err;
                }      
                sdt_info->muliti_service_name[muliti_service_name_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                sdt_info->muliti_service_name[muliti_service_name_count-1].descriptor_length = TODATA0_7BIT(p[1]);
				sdt_info->muliti_service_name[muliti_service_name_count-1].service_name = NULL;
                ret = si_parse_lib_sdt_muliti_service_name(&(sdt_info->muliti_service_name[muliti_service_name_count-1]),
                            &p[2],sdt_info->muliti_service_name[muliti_service_name_count-1].descriptor_length);
                if(ret < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse muliti_service_name err!!");
#endif  
                    goto err;
                }
                p += 2;//跳过头
                size -= 2;
                p += sdt_info->muliti_service_name[muliti_service_name_count-1].descriptor_length;//跳过该tag剩余字节
                size -= sdt_info->muliti_service_name[muliti_service_name_count-1].descriptor_length;
                break;

            case SI_PARSE_LIB_DATA_BROADCAST_DES:
                data_broadcast_count++;
                sdt_info->data_broadcast = GxCore_Realloc(sdt_info->data_broadcast,
                        sizeof(GxSubsystemSiParseDataBroadcastDes)*data_broadcast_count);
                if(sdt_info->data_broadcast == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
                    goto err;
                }      
                sdt_info->data_broadcast[data_broadcast_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                sdt_info->data_broadcast[data_broadcast_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                sdt_info->data_broadcast[data_broadcast_count-1].data_broadcast_id = TODATA0_15BIT(p[2],p[3]);
                sdt_info->data_broadcast[data_broadcast_count-1].component_tag = TODATA0_7BIT(p[4]);
                sdt_info->data_broadcast[data_broadcast_count-1].selector_length = TODATA0_7BIT(p[5]);
				sdt_info->data_broadcast[data_broadcast_count-1].selector_type = NULL;
				if(sdt_info->data_broadcast[data_broadcast_count-1].selector_length != 0)
				{
					sdt_info->data_broadcast[data_broadcast_count-1].selector_type = GxCore_Malloc(sdt_info->data_broadcast[data_broadcast_count-1].selector_length);
					if(sdt_info->data_broadcast[data_broadcast_count-1].selector_type == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
						goto err;
					}
					memcpy(sdt_info->data_broadcast[data_broadcast_count-1].selector_type,
							&p[6],sdt_info->data_broadcast[data_broadcast_count-1].selector_length);
				}
				p += 6;//跳过头部
				size -= 6;
				p += sdt_info->data_broadcast[data_broadcast_count-1].selector_length;//跳过第一个描述
				size -= sdt_info->data_broadcast[data_broadcast_count-1].selector_length;

				memcpy(sdt_info->data_broadcast[data_broadcast_count-1].language,p,3);
				sdt_info->data_broadcast[data_broadcast_count-1].text_length = TODATA0_7BIT(p[3]);
				sdt_info->data_broadcast[data_broadcast_count-1].text = NULL;
				if(sdt_info->data_broadcast[data_broadcast_count-1].text_length != 0)
				{
					sdt_info->data_broadcast[data_broadcast_count-1].text = GxCore_Malloc(sdt_info->data_broadcast[data_broadcast_count-1].text_length);
					if(sdt_info->data_broadcast[data_broadcast_count-1].text == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse sdt_info no memery!!");
#endif  
						goto err;
					}
					memcpy(sdt_info->data_broadcast[data_broadcast_count-1].text,
							&p[4],sdt_info->data_broadcast[data_broadcast_count-1].text_length);
				}
				p += 4;//跳过第二个描述
				p += sdt_info->data_broadcast[data_broadcast_count-1].text_length;
				size -= 4;
				size -= sdt_info->data_broadcast[data_broadcast_count-1].text_length;
				break;

			default:
				descriptor_length = TODATA0_7BIT(p[1]);
				p += 2;//跳过头
				p += descriptor_length;//跳过该tag剩余字节
                size -= 2;
                size -= descriptor_length;
                break;
        }
    }
    sdt_info->stuffing_count = stuffing_count;
    sdt_info->bouquet_name_count = bouquet_name_count;
    sdt_info->service_count = service_count;
    sdt_info->country_availability_count = country_count;
    sdt_info->NVOD_reference_count = nvod_count;
    sdt_info->time_shifted_service_count = time_shifted_count;
    sdt_info->ca_identifier_count = ca_identifier_count;
    sdt_info->muliti_service_name_count = muliti_service_name_count;
    sdt_info->data_broadcast_count = data_broadcast_count;
    return size;
err:
    sdt_info->stuffing_count = stuffing_count;
    sdt_info->bouquet_name_count = bouquet_name_count;
    sdt_info->service_count = service_count;
    sdt_info->country_availability_count = country_count;
    sdt_info->NVOD_reference_count = nvod_count;
    sdt_info->time_shifted_service_count = time_shifted_count;
    sdt_info->ca_identifier_count = ca_identifier_count;
    sdt_info->muliti_service_name_count = muliti_service_name_count;
    sdt_info->data_broadcast_count = data_broadcast_count;
    
    return -1;

}
static int32_t si_parse_lib_sdt_parse_loop(GxSubsystemSiParseSDT* sdt,uint8_t* src,uint32_t len) 
{
    int32_t size = len;
    uint32_t sdt_info_count = 0;
    uint8_t* p = src;
    int32_t ret = 0;

    while(size > 0)
    {
        sdt_info_count++;
        sdt->sdt_info = GxCore_Realloc(sdt->sdt_info,sizeof(GxSubsystemSiParseSdtInfo)*sdt_info_count);
		memset(((void*)sdt->sdt_info)+sizeof(GxSubsystemSiParseSdtInfo)*(sdt_info_count-1),0,sizeof(GxSubsystemSiParseSdtInfo));
        if(sdt->sdt_info == NULL)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("parse sdt no memery!!");
#endif  
            goto err;
        }      
        sdt->sdt_info[sdt_info_count-1].service_id = TODATA0_15BIT(p[0],p[1]);
        sdt->sdt_info[sdt_info_count-1].EIT_schedule_flag = TODATA1BIT(p[2]);
        sdt->sdt_info[sdt_info_count-1].EIT_present_following_flag = TODATA0BIT(p[2]);
        sdt->sdt_info[sdt_info_count-1].running_status = TODATA5_7BIT(p[3]);
        sdt->sdt_info[sdt_info_count-1].free_CA_mode = TODATA4BIT(p[3]);
        sdt->sdt_info[sdt_info_count-1].descriptors_loop_length = TODATA0_11BIT(p[3],p[4]);

        ret = si_parse_lib_sdt_parse_des(&(sdt->sdt_info[sdt_info_count-1]),
                &p[5],sdt->sdt_info[sdt_info_count-1].descriptors_loop_length);
        if(ret < 0)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("parse des err!!");
#endif
            goto err;
        }
        p += 5;//跳过头
        p += sdt->sdt_info[sdt_info_count-1].descriptors_loop_length;//跳过该tag剩余字节
        size -= 5;
        size -= sdt->sdt_info[sdt_info_count-1].descriptors_loop_length;
    }
    sdt->sdt_info_count = sdt_info_count;
    return size;

err:
    sdt->sdt_info_count = sdt_info_count;
    return -1;
}

/**
 *  @brief      把一段section数据解析成sdt表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      sdt数据
 *               NULL：发生错误
 */
GxSubsystemSiParseSDT* GxSubsystm_SiParseLibSdtParse(uint8_t* section,uint32_t size)
{
    GxSubsystemSiParseSDT* sdt = NULL;
    int32_t len = (int32_t)size;
    uint8_t* p = section;
    int32_t ret = 0;

    if(section == NULL || size < 11)//sdt表的头部有11字节
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("section is NULL or size is err!!");
#endif
        goto err;
    }
    sdt = GxCore_Malloc(sizeof(GxSubsystemSiParseSDT));
    if(sdt == NULL)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse sdt no memery!!");
#endif
        goto err;
    }
    memset(sdt,0,sizeof(GxSubsystemSiParseSDT));
    sdt->table_id = TODATA0_7BIT(p[0]);
    sdt->section_syntax_indicator = TODATA7BIT(p[1]);
    sdt->section_length = TODATA0_11BIT(p[1],p[2]);
    sdt->transport_stream_id = TODATA0_15BIT(p[3],p[4]);
    sdt->version_number =  TODATA1_5BIT(p[5]);
    sdt->current_next_indicator = TODATA0BIT(p[5]);
    sdt->section_number = TODATA0_7BIT(p[6]);
    sdt->last_section_number = TODATA0_7BIT(p[7]);
    sdt->original_network_id = TODATA0_15BIT(p[8],p[9]);

    p += 11;//跳过头部
    len -= 11;//头部解析完成
    len -= 4;//去掉crc

    if(len != sdt->section_length-8-4)//解析出来的大小和表里面的大小不一致
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the section size is err!!");
#endif  
        goto err;
    }
    ret = si_parse_lib_sdt_parse_loop(sdt,p,len);
    if(ret < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse loop err!!");
#endif
        goto err;
    }
    si_parse_lib_sdt_add_protect((void*)sdt);
    return sdt;
err:
    si_parse_lib_sdt_release(sdt);
    return NULL;
}

/**
 *  @brief      释放sdt的空间，应用应该在通过GxSubsystm_SiParseLibSdtParse获取
 *  解析好的sdt数据之后某个时刻释放sdt的空间。
 *   
 *  @param       GxSubsystemSiParseSDT* sdt:sdt的存储空间，是通过GxSubsystm_SiParseLibSdtParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibSdtRelease(void* sdt)
{
    uint32_t ret = 0;
    ret = si_parse_lib_sdt_remove_protect(sdt);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParseSDT* sdt is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_sdt_release((GxSubsystemSiParseSDT*)sdt);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

