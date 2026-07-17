/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_vct.c
* Author    : 	xiahuangshuai
* Project   :	GoXceed 
* Type      :	
******************************************************************************
* Purpose   :	
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2009.09.01	      	         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include <stdio.h>
#include <string.h>
#include "include/si_parse_lib_private.h"

static si_parse_lib_protect protect = {0};
static handle_t si_parse_lib_protect_mutex = 0;
/******************function******************************/
static void si_parse_lib_vct_add_protect(void* p);
static uint32_t si_parse_lib_vct_remove_protect(void* p);
static void si_parse_lib_vct_release(GxSubsystemSiParseVCT * vct);
static int32_t si_parse_lib_vct_parse_loop_des(GxSubsystemSiParseVCTloop * loop,uint8_t * section,uint8_t * section_end);
static int32_t si_parse_lib_cvct_parse_loop(GxSubsystemSiParseVCT * vct , uint8_t * section,uint32_t size);
GxSubsystemSiParseVCT * GxSubsystm_SiParseLibCvctParse(uint8_t * section,uint32_t len);
int32_t  GxSubsystm_SiParseLibCvctRelease(void * vct);





static void si_parse_lib_vct_add_protect(void* p)
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

static uint32_t si_parse_lib_vct_remove_protect(void* p)
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


static void si_parse_lib_vct_release(GxSubsystemSiParseVCT * vct)
{
    int32_t i,j;
    if(vct)
    {
        if(vct->loop)
        {
            for(i = 0;i<vct->num_channels_in_section;i++)
            {
                if(vct->loop[i].extendednamedes)
                {
                    for(j = 0;j<vct->loop[i].extendednamedes_count;j++)
                    {
                        si_parse_lib_release_multiple_string(&vct->loop[i].extendednamedes[j].long_channel_name_text);
                    }
                    GxCore_Free(vct->loop[i].extendednamedes);
                }

                if(vct->loop[i].sldes)
                {
                    for(j = 0;j<vct->loop[i].sld_count;j++)
                    {
                        if(vct->loop[i].sldes[j].elemets_info)
                        {
                            GxCore_Free(vct->loop[i].sldes[j].elemets_info);
                        }
                    }
                    GxCore_Free(vct->loop[i].sldes);
                }
                if(vct->loop[i].tssdes)
                {
                    for(j = 0;j<vct->loop[i].tssd_count;j++)
                    {
                        if(vct->loop[i].tssdes[j].service_info)
                        {
                            GxCore_Free(vct->loop[i].tssdes[j].service_info);
                        }
                    }
                    GxCore_Free(vct->loop[i].tssdes);
                }
            }
            GxCore_Free(vct->loop);
        }
        GxCore_Free(vct);
    }
}

static int32_t si_parse_lib_vct_parse_loop_des(GxSubsystemSiParseVCTloop * loop,uint8_t * section,uint8_t * section_end)
{
    uint8_t * p = section;
	uint8_t* temp_p = NULL;
    int32_t descriptor_length;
    int32_t extendednamedes_count = 0,sld_count = 0,tssd_count = 0;
    int32_t size = loop->descriptor_length;
    int32_t i,ret;
    while(size > 0)
    {
        switch(p[0])
        {
            case SI_PARSE_LIB_ATSC_EXTENDED_EVENT_DES:
                extendednamedes_count++;
                loop->extendednamedes = GxCore_Realloc(loop->extendednamedes,extendednamedes_count * sizeof(GxSubsystemSiParseExtendedNamedes));
                memset(&loop->extendednamedes[extendednamedes_count - 1],0,sizeof(GxSubsystemSiParseExtendedNamedes));
                if(NULL == loop->extendednamedes)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct loop malloc failed");
#endif
                    goto err;
                }
                loop->extendednamedes[extendednamedes_count - 1].descriptor_tag =TODATA0_7BIT(p[0]); 
                loop->extendednamedes[extendednamedes_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                    goto err;
                }

                ret = si_parse_lib_parse_multiple_string(&loop->extendednamedes[extendednamedes_count - 1].long_channel_name_text,
                        p,section_end,loop->extendednamedes[extendednamedes_count - 1].descriptor_length);
                if(ret < 0)
                    goto err;

                p = si_parse_lib_poffset(p,section_end,loop->extendednamedes[extendednamedes_count - 1].descriptor_length);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size , loop->extendednamedes[extendednamedes_count - 1].descriptor_length);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                    goto err;
                }

                break;
            case SI_PARSE_LIB_SERVICE_LOCATION_DES:
                sld_count ++ ;
                loop->sldes = GxCore_Realloc(loop->sldes,sld_count*sizeof(GxSubsystemSiParseSLDdes));
                memset(&loop->sldes[sld_count - 1],0,sizeof(GxSubsystemSiParseSLDdes));
                if(NULL == loop->sldes)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct loop malloc failed");
#endif
                    goto err;
                }
                loop->sldes[sld_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop->sldes[sld_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
                if (loop->sldes[sld_count - 1].descriptor_length >= loop->descriptor_length)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("atsc si sld descriptor_length >= loop->descriptor_length\n");
#endif
                    p = si_parse_lib_poffset(p,section_end,loop->descriptor_length);
                    if (NULL == p)
                    {
#ifdef ATSC_SI_DEBUG
                        SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                        goto err;
                    }
                    size = si_parse_lib_sizeoffset(size , loop->descriptor_length);
                    if (size < 0)
                    {
#ifdef ATSC_SI_DEBUG
                        SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                        goto err;
                    }
                    break;

#if 0
                    p = si_parse_lib_poffset(p,section_end,loop->descriptor_length);
                    if(NULL == p)
                    {
#ifdef ATSC_SI_DEBUG
                        SI_PARSE_LIB_ERR_PRINTF("atsc si sld loop->descriptor_length err\n");
#endif
                        goto err;
                    }
                    size = si_parse_lib_sizeoffset(size,loop->size);
                    if (size < 0)
                    {
#ifdef ATSC_SI_DEBUG
                        SI_PARSE_LIB_ERR_PRINTF("atsc si sld loop->descriptor_length err\n");
#endif
                        goto err;
                    }
#endif

                }
                loop->sldes[sld_count - 1].PCR_PID = TODATA0_12BIT(p[2],p[3]);
                loop->sldes[sld_count - 1].number_elements = TODATA0_7BIT(p[4]);
                p = si_parse_lib_poffset(p,section_end,5);
                if (NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size , 5);
                if (size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                    goto err;
                }

                loop->sldes[sld_count - 1].elemets_info = 
					GxCore_Mallocz(loop->sldes[sld_count - 1].number_elements * sizeof(Elements));
				if (NULL == loop->sldes[sld_count - 1].elemets_info)
					goto err;
				temp_p = p;
				for(i = 0 ; i < loop->sldes[sld_count - 1].number_elements;i++)
				{
					loop->sldes[sld_count - 1].elemets_info[i].stream_type = TODATA0_7BIT(temp_p[0]);
					loop->sldes[sld_count - 1].elemets_info[i].elementary_PID = TODATA0_12BIT(temp_p[1],temp_p[2]);
					memcpy(loop->sldes[sld_count - 1].elemets_info[i].ISO_639_language_code,&temp_p[3],3);
					temp_p += 6;
				}
				p = si_parse_lib_poffset(p,section_end,loop->sldes[sld_count - 1].descriptor_length - 3);
				if (NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
					goto err;
				}
				size = si_parse_lib_sizeoffset(size ,loop->sldes[sld_count - 1].descriptor_length - 3);
				if (size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
					goto err;
				}
				break;
			case SI_PARSE_LIB_ATSC_TIME_SHIFTED_SERVICE_DES:
				tssd_count ++ ;
				loop->tssdes = GxCore_Realloc(loop->tssdes,tssd_count*sizeof(GxSubsystemSiParseTSSDdes));
                memset(&loop->tssdes[tssd_count - 1],0,sizeof(GxSubsystemSiParseTSSDdes));
                if(NULL == loop->tssdes)
                    goto err;
                loop->tssdes[tssd_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop->tssdes[tssd_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
                loop->tssdes[tssd_count - 1].number_of_services = TODATA0_4BIT(p[2]);
                p = si_parse_lib_poffset(p,section_end,3);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,3);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                    goto err;
                }
				loop->tssdes[tssd_count - 1].service_info = 
					GxCore_Mallocz(loop->tssdes[tssd_count - 1].number_of_services * sizeof(TSSServices));
				if (NULL == loop->tssdes[tssd_count - 1].service_info )
					goto err;
				temp_p = p;
				for(i = 0;i < loop->tssdes[tssd_count - 1].number_of_services;i++)
				{
					loop->tssdes[tssd_count - 1].service_info[i].time_shift = TODATA0_9BIT(temp_p[0],temp_p[1]);    
					loop->tssdes[tssd_count - 1].service_info[i].major_channel_number = TODATA2_11BIT(temp_p[2],temp_p[3]);
					loop->tssdes[tssd_count - 1].service_info[i].minor_channel_number = TODATA0_9BIT(temp_p[3],temp_p[4]);
					temp_p += 5;
				}
				p = si_parse_lib_poffset(p,section_end,loop->tssdes[tssd_count - 1].descriptor_length - 1);
				if (NULL == p)
				{
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,loop->tssdes[tssd_count - 1].descriptor_length - 1); 
                if (size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                    goto err;
                }
                break;
            default :
                descriptor_length = TODATA0_7BIT(p[1]);
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                    goto err;
                }
                p = si_parse_lib_poffset(p,section_end,descriptor_length);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size ,2);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,descriptor_length);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
                    goto err;
                }
        }
    }
    loop->extendednamedes_count = extendednamedes_count;
    loop->sld_count = sld_count;
    loop->tssd_count = tssd_count;
    return size;
err:
    loop->extendednamedes_count = extendednamedes_count;
    loop->sld_count = sld_count;
    loop->tssd_count = tssd_count;
    
    return -1;
}

static int32_t si_parse_lib_cvct_parse_loop(GxSubsystemSiParseVCT * vct , uint8_t * section,uint32_t size)
{
    int32_t count = vct->num_channels_in_section;
    int32_t i,ret;
    uint8_t * p = section;
    uint8_t * section_end = section + vct->section_length - 7; 
    
    vct->loop = GxCore_Mallocz(count * sizeof(GxSubsystemSiParseVCTloop));
    if(NULL == vct->loop)
        goto err;
    for(i = 0;i<count;i++)
    {
        memcpy(vct->loop[i].short_name,p,14);
        p = si_parse_lib_poffset(p,section_end,14);
        if(NULL == p)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
            goto err;
        }
        size = si_parse_lib_sizeoffset(size,14);
        if(size < 0)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
            goto err;
        }

        vct->loop[i].major_channel_number = TODATA2_11BIT(p[0],p[1]); 
        vct->loop[i].minor_channel_number = TODATA0_9BIT(p[1],p[2]);
        vct->loop[i].modulation_mode = TODATA0_7BIT(p[3]);
        vct->loop[i].carrier_frequency = TODATA0_31BIT(p[4],p[5],p[6],p[7]);
        vct->loop[i].channel_TSID = TODATA0_15BIT(p[8],p[9]);
        vct->loop[i].program_number = TODATA0_15BIT(p[10],p[11]);

        vct->loop[i].ETM_location = TODATA6_7BIT(p[12]);
        vct->loop[i].access_controlled = TODATA5BIT(p[12]);
        vct->loop[i].hidden = TODATA4BIT(p[12]);
        vct->loop[i].info.cvctinfo.path_select = TODATA3BIT(p[12]); 
        vct->loop[i].info.cvctinfo.out_of_band = TODATA2BIT(p[12]);
        vct->loop[i].hide_guide = TODATA1BIT(p[12]); 

        vct->loop[i].service_type = TODATA0_5BIT(p[13]);
        vct->loop[i].source_id = TODATA0_15BIT(p[14],p[15]);
        vct->loop[i].descriptor_length = TODATA0_9BIT(p[16],p[17]);
        p = si_parse_lib_poffset(p,section_end,18);
        if(NULL == p)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
            goto err;
        }
        size = si_parse_lib_sizeoffset(size,18);
        if(size < 0)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
            goto err;
        }
        ret = si_parse_lib_vct_parse_loop_des(vct->loop,p,section_end);
        if(ret < 0)
            goto err;
        
        p = si_parse_lib_poffset(p,section_end,vct->loop[i].descriptor_length);
        if(NULL == p)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
            goto err;
        }
        size = si_parse_lib_sizeoffset(size,vct->loop[i].descriptor_length);
        if(size < 0)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
            goto err;
        }
    }
    return size;
err:
    return -1;
}

/**
 *  @brief      把一段section数据解析成cvct表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      cvct数据
 *               NULL：发生错误
 */
GxSubsystemSiParseVCT * GxSubsystm_SiParseLibCvctParse(uint8_t * section,uint32_t len)
{
    GxSubsystemSiParseVCT * vct = NULL;
    int32_t i;
    int32_t size = len;
    uint8_t * p = section;
    uint8_t * section_end;
    int32_t ret = 0;


    if(len < 10 || NULL == section)
    {
        return NULL;
    }   
    /* 解析vct表头*/
    vct = GxCore_Mallocz(sizeof(GxSubsystemSiParseVCT));
    if(NULL == vct)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("-------------------vct parse func malloc failed\n");
#endif
        goto err;
    }

    vct->table_id =  TODATA0_7BIT(p[0]);
    vct->section_syntax_indicator = TODATA7BIT(p[1]);
    if(ATSC_DEFAULT_VALUE_1 != vct->section_syntax_indicator)
    {
#ifdef ATSC_SI_DEBUG
         SI_PARSE_LIB_ERR_PRINTF("the section_indicator is err!!");       
#endif
         goto err;
    }  
    vct->private_indicator = TODATA6BIT(p[1]);
    if(ATSC_DEFAULT_VALUE_1 != vct->private_indicator)
    {
#ifdef ATSC_SI_DEBUG
         SI_PARSE_LIB_ERR_PRINTF("the private_indicator is err!!");       
#endif
         goto err;
    }
    
    vct->section_length = TODATA0_11BIT(p[1],p[2]);

    vct->transport_stream_id  =TODATA0_15BIT(p[3],p[4]);

    vct->version_number = TODATA1_5BIT(p[5]);

    vct->current_next_indicator = TODATA0BIT(p[5]);
    if(ATSC_DEFAULT_VALUE_1 != vct->current_next_indicator)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the current_next_indictor is err");
#endif
        goto err;
    }
    vct->section_number = TODATA0_7BIT(p[6]);
    if(ATSC_DEFAULT_VALUE_0 != vct->section_number)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the section_number is err");
#endif
        goto err;
    }
    vct->last_section_number = TODATA0_7BIT(p[7]);
    if(ATSC_DEFAULT_VALUE_0 != vct->last_section_number)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the last_section_number is err");
#endif
        goto err;
    }
    vct->protocol_version =TODATA0_7BIT(p[8]);
    if(ATSC_DEFAULT_VALUE_0 != vct->protocol_version)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the protocol_version is err");
#endif
        goto err;
    }

    vct->num_channels_in_section = TODATA0_7BIT(p[9]);

    section_end = p + vct->section_length + 3;
    p = si_parse_lib_poffset(p,section_end,10);
    if(NULL == p)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
        goto err;
    }
    size = si_parse_lib_sizeoffset(size ,10);
    if(size < 0)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
        goto err;
    }

    size = si_parse_lib_sizeoffset(size , 4);
    if(size < 0)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
        goto err;
    }

    if(size != vct->section_length - 7 - 4)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("section_length is err");
#endif
        goto err;
    }

    if(vct->num_channels_in_section > 0)
    {
        ret = si_parse_lib_cvct_parse_loop(vct,p,size);
        if(ret < 0)
            goto err;
        else
            size = ret;          
    }

    //跳过loop长度
    for(i = 0;i<vct->num_channels_in_section ; i++)
    {
        p = si_parse_lib_poffset(p,section_end,32+vct->loop[i].descriptor_length);
        if(NULL == p)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
            goto err;
        }
    }
    vct->additional_descriptors_length = TODATA0_9BIT(p[0],p[1]);
    p = si_parse_lib_poffset(p,section_end,2);
    if(NULL == p)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
        goto err;
    }

    size = si_parse_lib_sizeoffset(size , 2);
    if(size < 0)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
        goto err;
    }

    p = si_parse_lib_poffset(p,section_end,vct->additional_descriptors_length); 
    if(NULL == p)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
        goto err;
    }
    size = si_parse_lib_sizeoffset(size,vct->additional_descriptors_length);
    if(size < 0)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
        goto err;
    }


    /*是否要解析crc*/
    vct->CRC_32 = TODATA4_31BIT(p[0],p[1],p[2],p[3]);
    p = si_parse_lib_poffset(p,section_end,4);
    if(NULL == p)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("vct size err\n");
#endif
        goto err;
    }

    si_parse_lib_vct_add_protect((void*)vct);
    return vct;
err:
    si_parse_lib_vct_release(vct);
    return NULL;
}

/**
 *  @brief      释放cvct的空间，应用应该在通过GxSubsystm_SiParseLibCvctParse获取
 *  解析好的cvct数据之后某个时刻释放cvct的空间。
 *   
 *  @param       GxSubsystemSiParseVCT* vct:vct的存储空间，是通过GxSubsystm_SiParseLibCvctParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibCvctRelease(void * vct)
{
    uint32_t ret = 0;
    ret = si_parse_lib_vct_remove_protect(vct);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParsevct* vct is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_vct_release((GxSubsystemSiParseVCT *)vct);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

