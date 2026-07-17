/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_eit.c
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
static void si_parse_lib_eit_release(GxSubsystemSiParseAtscEIT * eit);
static void si_parse_lib_eit_add_protect(void* p);
static uint32_t si_parse_lib_eit_remove_protect(void* p);
static int32_t si_parse_lib_eit_parse_loop_des(GxSubsystemSiParseAtscEITLoop * loop , uint8_t * section , uint8_t * section_end);
static int32_t si_parse_lib_eit_parse_loop(GxSubsystemSiParseAtscEIT * eit , uint8_t * section , int32_t size);
GxSubsystemSiParseAtscEIT * GxSubsystm_AtscSiParseLibEitParse(uint8_t * section,uint32_t len);
int32_t  GxSubsystm_AtscSiParseLibEitRelease(void * eit);



static void si_parse_lib_eit_release(GxSubsystemSiParseAtscEIT * eit)
{
    int32_t i = 0,j = 0,m = 0;

    if(eit)
    {
        if(eit->loop)
        {
            for(i = 0;i<eit->num_events_in_section;i++)
            {
                //multiple_string
                si_parse_lib_release_multiple_string(&eit->loop[i].title_text);
                //ac3
                if(eit->loop[i].ac3)
                {
                    GxCore_Free(eit->loop[i].ac3);
                }
                //eac3
                if(eit->loop[i].eac3)
                {
                    GxCore_Free(eit->loop[i].eac3);
                }
                //csd
                if(eit->loop[i].csd)
                {
                    for(j = 0;j<eit->loop[i].csd_count;j++)
                    {
                        if(eit->loop[i].csd[j].csdinfo)
                        {
                            GxCore_Free(eit->loop[i].csd[j].csdinfo);
                        }
                    }
                    GxCore_Free(eit->loop[i].csd);
                }
                //cad
                if(eit->loop[i].cad)
                {
                    for(m = 0;m<eit->loop[i].cad_count;m++)
                    {
                        if(eit->loop[i].cad[m].rating_region_info)
                        {
                            for(j = 0 ;j<eit->loop[i].cad[m].rating_region_count;j++)
                            {
                                if(eit->loop[i].cad[m].rating_region_info[j].dimensions)
                                {
                                    GxCore_Free(eit->loop[i].cad[m].rating_region_info[j].dimensions);
                                }
                                si_parse_lib_release_multiple_string(&eit->loop[i].cad[m].rating_region_info[j].rating_description_text);
                            }
                            GxCore_Free(eit->loop[i].cad[m].rating_region_info);
                        }
                    }
                    GxCore_Free(eit->loop[i].cad);
                }
                //rcd
                if(eit->loop[i].rcd)
                {
                    for(j = 0;j<eit->loop[i].rcd_count;j++)
                    {
                        if(eit->loop[i].rcd[j].attribute)
                        {
                            GxCore_Free(eit->loop[i].rcd[j].attribute);
                        }
                    }
                    GxCore_Free(eit->loop[i].rcd);
                }
                //gd
                if(eit->loop[i].gd)
                {
                    for(j = 0;j<eit->loop[i].gd_count;j++)
                    {
                        if(eit->loop[i].gd[j].attribute)
                        {
                            GxCore_Free(eit->loop[i].gd[j].attribute);
                        }
                    }
                    GxCore_Free(eit->loop[i].gd);
                }
            }
                GxCore_Free(eit->loop);
        }
        GxCore_Free(eit);
    }
}


static void si_parse_lib_eit_add_protect(void* p)
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

static uint32_t si_parse_lib_eit_remove_protect(void* p)
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


static int32_t si_parse_lib_eit_parse_loop_des(GxSubsystemSiParseAtscEITLoop * loop , uint8_t * section , uint8_t * section_end)
{
    uint8_t * p = section, * tmp_addr = NULL;
    int32_t i = 0,j = 0,ret = 0,tmp_size = 0,k = 0;
    int32_t mul_length = 0;
    int32_t descriptor_length = 0;
    int32_t size = loop->descriptor_length;
    int32_t ac3_count = 0,csd_count = 0,cad_count = 0,rcd_count = 0,gd_count = 0,eac3_count = 0;
    int32_t offset_count = 0,temp_count = 0;
    while(size > 0)
    {
        switch(p[0])
        {
            case SI_PARSE_LIB_AC3_DES:
                ac3_count++;
                loop->ac3 = GxCore_Realloc(loop->ac3,sizeof(GxSubsystemSiParseAC3Des)*ac3_count);
                memset(&loop->ac3[ac3_count - 1],0,sizeof(GxSubsystemSiParseAC3Des));
                if(loop->ac3 == NULL)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse eit no memery!!");
#endif  
                    goto err;
                }      
                loop->ac3[ac3_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop->ac3[ac3_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                if(loop->ac3[ac3_count - 1].descriptor_length >= loop->descriptor_length)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("PARSE AC3  length err\n");
#endif
                    goto err;
                }
                loop->ac3[ac3_count-1].AC3_type_flag = TODATA7BIT(p[2]); 
                loop->ac3[ac3_count-1].bsid_flag = TODATA6BIT(p[2]); 
                loop->ac3[ac3_count-1].mainid_flag = TODATA5BIT(p[2]); 
                loop->ac3[ac3_count-1].asvc_flag = TODATA4BIT(p[2]); 
                if(loop->ac3[ac3_count-1].AC3_type_flag ==1 )
                {
                    loop->ac3[ac3_count-1].AC3_type = TODATA4BIT(p[3]);
                }
                if(loop->ac3[ac3_count-1].bsid_flag ==1 )
                {
                    loop->ac3[ac3_count-1].bsid = TODATA4BIT(p[4]);
                }
                if(loop->ac3[ac3_count-1].mainid_flag ==1 )
                {
                    loop->ac3[ac3_count-1].mainid = TODATA4BIT(p[5]);
                }
                if(loop->ac3[ac3_count-1].asvc_flag ==1 )
                {
                    loop->ac3[ac3_count-1].asvc = TODATA4BIT(p[6]);
                }
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }
                p = si_parse_lib_poffset(p,section_end,loop->ac3[ac3_count-1].descriptor_length);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size , loop->ac3[ac3_count-1].descriptor_length);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }
                break;
            case SI_PARSE_LIB_EAC3_DES:
                eac3_count++;
                loop->eac3 = GxCore_Realloc(loop->ac3,sizeof(GxSubsystemSiParseEAC3Des)*eac3_count);
                memset(&loop->eac3[eac3_count - 1],0,sizeof(GxSubsystemSiParseEAC3Des));
                if(loop->eac3 == NULL)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse eit no memery!!");
#endif  
                    goto err;
                }      
                loop->eac3[eac3_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop->eac3[eac3_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                if(loop->eac3[eac3_count - 1].descriptor_length > loop->descriptor_length)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eac3 descriptor_length err\nn");
#endif
                    goto err;
                }
                loop->eac3[eac3_count-1].EAC3_type_flag = TODATA7BIT(p[2]); 
                loop->eac3[eac3_count-1].bsid_flag = TODATA6BIT(p[2]); 
                loop->eac3[eac3_count-1].mainid_flag = TODATA5BIT(p[2]); 
                loop->eac3[eac3_count-1].asvc_flag = TODATA4BIT(p[2]); 
                loop->eac3[eac3_count-1].mixinfoexists = TODATA4BIT(p[2]); 
                loop->eac3[eac3_count-1].substream1_flag = TODATA4BIT(p[2]); 
                loop->eac3[eac3_count-1].substream2_flag = TODATA4BIT(p[2]); 
                loop->eac3[eac3_count-1].substream3_flag = TODATA4BIT(p[2]); 
                if(loop->eac3[eac3_count-1].EAC3_type_flag ==1 )
                {
                    loop->eac3[eac3_count-1].EAC3_type = TODATA4BIT(p[3]);
                }
                if(loop->eac3[eac3_count-1].bsid_flag ==1 )
                {
                    loop->eac3[eac3_count-1].bsid = TODATA4BIT(p[4]);
                }
                if(loop->eac3[eac3_count-1].mainid_flag ==1 )
                {
                    loop->eac3[eac3_count-1].mainid = TODATA4BIT(p[5]);
                }
                if(loop->eac3[eac3_count-1].asvc_flag ==1 )
                {
                    loop->eac3[eac3_count-1].asvc = TODATA4BIT(p[6]);
                }
                if(loop->eac3[eac3_count-1].substream1_flag ==1 )
                {
                    loop->eac3[eac3_count-1].substream1 = TODATA4BIT(p[7]);
                }
                if(loop->eac3[eac3_count-1].substream2_flag ==1 )
                {
                    loop->eac3[eac3_count-1].substream2 = TODATA4BIT(p[8]);
                }
                if(loop->eac3[eac3_count-1].substream3_flag ==1 )
                {
                    loop->eac3[eac3_count-1].substream3 = TODATA4BIT(p[9]);
                }
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }

                p = si_parse_lib_poffset(p,section_end,loop->eac3[eac3_count-1].descriptor_length);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size , 2);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size , loop->eac3[eac3_count-1].descriptor_length);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }

                break;
            case SI_PARSE_LIB_CAPTION_SERVICE_DES:
                csd_count++;
                loop->csd = GxCore_Realloc(loop->csd,sizeof(GxSubsystemSiParseCaptionServiceDes)*csd_count);
                memset(&loop->csd[csd_count - 1],0,sizeof(GxSubsystemSiParseCaptionServiceDes));
                if(NULL == loop->csd)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse csd malloc failed");
#endif
                    goto err;
                }
                loop->csd[csd_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop->csd[csd_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
                if(loop->csd[csd_count - 1].descriptor_length > loop->descriptor_length)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("csd descriptor_length err\n");
#endif
                    goto err;
                }
                loop->csd[csd_count - 1].number_of_service = TODATA0_4BIT(p[2]);

                p  = si_parse_lib_poffset(p,section_end,3);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size,3);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }

                loop->csd[csd_count - 1].csdinfo =
                    GxCore_Mallocz(loop->csd[csd_count - 1].number_of_service * sizeof(GxSubsystemSiParseCSDinfo));
                if(NULL == loop->csd[csd_count -1].csdinfo)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("SI parse eit csdinfo malloc failed\n");
#endif
                    goto err;
                }
                for(i = 0;i<loop->csd[csd_count-1].number_of_service;i++)
                {
                    memcpy(loop->csd[csd_count - 1].csdinfo->language,p,3);
                    loop->csd[csd_count - 1].csdinfo->digital_cc = TODATA7BIT(p[3]);
                    if(ATSC_DEFAULT_VALUE_0 == loop->csd[csd_count - 1].csdinfo->digital_cc)
                    {
                        loop->csd[csd_count - 1].csdinfo->digital_cc_info.line32_filed_info.line32_filed = TODATA0BIT(p[3]);
                    }
                    else
                    {
                        loop->csd[csd_count - 1].csdinfo->digital_cc_info.caption_service_number_info.caption_service_number = TODATA0_5BIT(p[3]);
                    }
                    loop->csd[csd_count - 1].csdinfo->easy_reader = TODATA7BIT(p[4]);
                    loop->csd[csd_count - 1].csdinfo->wide_aspect_ratio = TODATA6BIT(p[4]);
                    p = si_parse_lib_poffset(p,section_end,6);
                    if(NULL == p)
                    {
#ifdef ATSC_SI_DEBUG
                        SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                        goto err;
                    }

                    size = si_parse_lib_sizeoffset(size , 6);
                    if(size < 0)
                    {
#ifdef ATSC_SI_DEBUG
                        SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                        goto err;
                    }

                }
                break;
            case SI_PARSE_LIB_CONTENT_ADVISORY_DES:
                cad_count++;
                loop->cad = GxCore_Realloc(loop->cad,sizeof(GxSubsystemSiParseContentAdvisoryDes)*cad_count);
                memset(&loop->cad[cad_count - 1],0,sizeof(GxSubsystemSiParseContentAdvisoryDes));
                if(NULL == loop->cad)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse cad malloc failed");
#endif
                    goto err;
                }
                //解析cad头
                tmp_addr = p;
                tmp_size = size;
                loop->cad[cad_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop->cad[cad_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
                if(loop->cad[cad_count - 1].descriptor_length >= loop->descriptor_length)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit cad decsriptor length err\n");
#endif
                    goto err;
                }
                loop->cad[cad_count - 1].rating_region_count = TODATA0_5BIT(p[2]);
                p = si_parse_lib_poffset(p,section_end,3);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size,3);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }

                loop->cad[cad_count - 1].rating_region_info = 
                    GxCore_Mallocz(sizeof(GxSubsystemSiParseRatingRegion)*loop->cad[cad_count - 1].rating_region_count);
                if(loop->cad[cad_count - 1].rating_region_info == NULL)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("cad rating region info err\n");
#endif
                    goto err;
                }

                temp_count = loop->cad[cad_count - 1].rating_region_count;
                for(i = 0;i<temp_count;i++)
                {
                    //解析rating_region头
                    loop->cad[cad_count - 1].rating_region_info[i].rating_region = TODATA0_7BIT(p[0]);   
                    loop->cad[cad_count - 1].rating_region_info[i].rated_dimensions = TODATA0_7BIT(p[1]);
                    p = si_parse_lib_poffset(p,section_end,2);
                    if(NULL == p)
                    {
#ifdef ATSC_SI_DEBUG
                        SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                        goto err;
                    }


                    size = si_parse_lib_sizeoffset(size, 2);
                    if(size < 0)
                    {
#ifdef ATSC_SI_DEBUG
                        SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                        goto err;
                    }



                    loop->cad[cad_count - 1].rating_region_info[i].dimensions = 
                        GxCore_Mallocz(loop->cad[cad_count - 1].rating_region_info[i].rated_dimensions *
                                sizeof(GxSubsystemSiParseRatedDimensions));
                    if(NULL == loop->cad[cad_count - 1].rating_region_info[i].dimensions)
                    {
#ifdef ATSC_SI_DEBUG
                        SI_PARSE_LIB_ERR_PRINTF("SI eit parse dimensions malloc failed\n");
#endif
                        goto err;
                    }

                    for(j = 0;j < loop->cad[cad_count - 1].rating_region_info[i].rated_dimensions;j++) 
                    {
                        loop->cad[cad_count - 1].rating_region_info[i].dimensions[j].rating_dimension_j = TODATA0_7BIT(p[0]);
                        loop->cad[cad_count - 1].rating_region_info[i].dimensions[j].rating_value = TODATA0_3BIT(p[1]);
                        p = si_parse_lib_poffset(p,section_end,2);
                        if(NULL == p)
                        {
#ifdef ATSC_SI_DEBUG
                            SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                            goto err;
                        }

                        size = si_parse_lib_sizeoffset(size,2);
                        if(size < 0)
                        {
#ifdef ATSC_SI_DEBUG
                            SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                            goto err;
                        }

                    }
                    loop->cad[cad_count - 1].rating_region_info[i].rating_description_length = TODATA0_7BIT(p[0]);
                    mul_length = loop->cad[cad_count - 1].rating_region_info[i].rating_description_length;

                    //根据实际情况中mul_length可能为0
                    if(mul_length)
                    {
                        ret = si_parse_lib_parse_multiple_string(&loop->cad[cad_count - 1].rating_region_info[i].rating_description_text,
                                p,section_end,mul_length);
                    }
                    else
                    {
                        ret = 0;
                    }
                    /* 
                     *当解析失败时，需要根据loop->descriptor_length来偏移
                     *
                     * */
                    if(ret < 0)
                    {
#ifdef ATSC_SI_DEBUG
                        SI_PARSE_LIB_ERR_PRINTF("SI parse eit multiple_string parse failed\n");
#endif
                        offset_count = loop->cad[cad_count - 1].descriptor_length - 1;//rating_region_count
                        //遍历当前cad_count,前面偏移过来的字节数
                        for(k = 0 ; k<i ; k++)
                        {
                            offset_count -= 2;//rating_region rated_dimensions
                            offset_count -= loop->cad[cad_count - 1].rating_region_info[k].rated_dimensions * 2;
                            offset_count -= loop->cad[cad_count - 1].rating_region_info[k].rating_description_length + 1;
                            if(loop->cad[cad_count - 1].rating_region_info[k].dimensions != NULL)
                            {
                                GxCore_Free(loop->cad[cad_count - 1].rating_region_info[k].dimensions);
                            }
                        }
                        offset_count -= 2;//rating_region rated_dimensions
                        offset_count -= loop->cad[cad_count - 1].rating_region_info[i].rated_dimensions * 2;
                        offset_count -= 1;
                         p = si_parse_lib_poffset(p,section_end,offset_count);
                        if(NULL == p)
                        {
#ifdef ATSC_SI_DEBUG
                            SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                            goto err;
                        }

                        size = si_parse_lib_sizeoffset(size,offset_count);
                        if(size < 0)
                        {
#ifdef ATSC_SI_DEBUG
                            SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                            goto err;
                        }

                        
                        GxCore_Free(loop->cad[cad_count - 1].rating_region_info);
                        cad_count --;
						size = 0;
						break;
                    }
                    else
                    {
                        p = si_parse_lib_poffset(p,section_end,mul_length);
                        if(NULL == p)
                        {
#ifdef ATSC_SI_DEBUG
                            SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                            goto err;
                        }

                        size = si_parse_lib_sizeoffset(size,mul_length);
                        if(size < 0)
                        {
#ifdef ATSC_SI_DEBUG
                            SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                            goto err;
                        }
                    }

                }
                //这个des嵌套多个循环所以指针的偏移和size变化放到for里面，需要验证是否有问题
                break;
            case SI_PARSE_LIB_REDISTRIBUTION_CONTRL_DES:
                rcd_count++;
                loop->rcd = GxCore_Realloc(loop->rcd,sizeof(GxSubsystemSiParseRedistributionControlDes)*rcd_count);
                memset(&loop->rcd[rcd_count - 1],0,sizeof(GxSubsystemSiParseRedistributionControlDes));
                if(NULL == loop->rcd)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse rcd malloc failed");
#endif
                    goto err;
                }
                loop->rcd[rcd_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop->rcd[rcd_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
                if(loop->rcd[rcd_count - 1].descriptor_length >= loop->descriptor_length)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("rcd descriptor_length err\n");
#endif
                    goto err;
                }
                loop->rcd[rcd_count - 1].attribute = GxCore_Mallocz(loop->rcd[rcd_count - 1].descriptor_length);
                if(NULL == loop->rcd[rcd_count - 1].attribute)
                    goto err;
                memcpy(loop->rcd[rcd_count - 1].attribute,&p[2],loop->rcd[rcd_count - 1].descriptor_length);
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }
                p = si_parse_lib_poffset(p,section_end,loop->rcd[rcd_count - 1].descriptor_length);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size , 2);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size , loop->rcd[rcd_count - 1].descriptor_length);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }

                break;
            case SI_PARSE_LIB_GENRE_DES:
                gd_count++;
                loop->gd = GxCore_Realloc(loop->gd,sizeof(GxSubsystemSiParseGenreDes)*gd_count);
                memset(&loop->gd[gd_count - 1],0,sizeof(GxSubsystemSiParseGenreDes));
                if(NULL == loop->gd)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse gdd malloc failed");
#endif
                    goto err;
                }
                loop->gd[gd_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop->gd[gd_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
                if(loop->gd[gd_count - 1].descriptor_length >= loop->descriptor_length)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("gd descriptor_length err\n");
#endif
                    goto err;
                }
                loop->gd[gd_count - 1].attribute_count = TODATA0_4BIT(p[2]); 
                loop->gd[gd_count - 1].attribute = GxCore_Mallocz(loop->gd[gd_count - 1].attribute_count);
                if(NULL == loop->gd[gd_count - 1].attribute)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("SI eit parse gd malloc failed\n");
#endif
                    goto err;
                }
                memcpy(loop->gd[gd_count - 1].attribute,&p[3],loop->gd[gd_count - 1].attribute_count);
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }

                p = si_parse_lib_poffset(p,section_end,loop->gd[gd_count - 1].descriptor_length);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size , 2);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size,loop->gd[gd_count - 1].descriptor_length);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    goto err;
                }

                break;
            default:
                descriptor_length = TODATA0_7BIT(p[1]); 
                p = si_parse_lib_poffset(p,section_end, 2 + descriptor_length);
                if(NULL == p)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
                    //goto err;
					size = 0;
					break;
                }

                size = si_parse_lib_sizeoffset(size , 2 + descriptor_length);
                if(size < 0)
                {
#ifdef ATSC_SI_DEBUG
                    SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					size = 0;
					break;
                    //goto err;
                }
        }
    }
    loop->ac3_count = ac3_count;
    loop->eac3_count = eac3_count;
    loop->csd_count = csd_count;
    loop->cad_count = cad_count;
    loop->rcd_count = rcd_count;
    loop->gd_count = gd_count;

    return size;
err:
    loop->ac3_count = ac3_count;
    loop->eac3_count = eac3_count;
    loop->csd_count = csd_count;
    loop->cad_count = cad_count;
    loop->rcd_count = rcd_count;
    loop->gd_count = gd_count;

    return -1;

}

static int32_t si_parse_lib_eit_parse_loop(GxSubsystemSiParseAtscEIT * eit , uint8_t * section , int32_t size)
{
    int32_t i = 0,count = 0,ret = 0;
    uint8_t * p = section;
    uint8_t * section_end;

    section_end = p + eit->section_length + 3 -10; 
    count = eit->num_events_in_section;
    eit->loop = GxCore_Mallocz(count * sizeof(GxSubsystemSiParseAtscEITLoop));
    if(NULL == eit->loop)
    {
        gxlogd("parse eit loop malloc failed\n");
        goto err;
    }   
    for(i = 0 ; i < count ; i++)
    {
        eit->loop[i].event_id = TODATA0_13BIT(p[0],p[1]);
        eit->loop[i].start_time = TODATA0_31BIT(p[2],p[3],p[4],p[5]);
        eit->loop[i].ETM_location = TODATA4_5BIT(p[6]);
        eit->loop[i].length_in_seconds = TODATA0_19BIT(p[6],p[7],p[8]); 
        eit->loop[i].title_length = TODATA0_7BIT(p[9]);
        p = si_parse_lib_poffset(p,section_end,10);
        if(NULL == p)
        {
            return -2;
        }
        size = si_parse_lib_sizeoffset(size,10);
        if(size < 0)
        {
            return -3;
        }

        ret = si_parse_lib_parse_multiple_string(&(eit->loop[i].title_text),p,section_end,size);
        if(ret < 0)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("eit multiple_string err\n");
#endif
            goto err;
        }

        p = si_parse_lib_poffset(p,section_end,eit->loop[i].title_length);
        if(NULL == p)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
            goto err;
        }

        size = si_parse_lib_sizeoffset(size , eit->loop[i].title_length);
        if(size < 0)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
            goto err;
        }

        eit->loop[i].descriptor_length = TODATA0_11BIT(p[0],p[1]);
        p = si_parse_lib_poffset(p,section_end,2);
        if(NULL == p)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
            goto err;
        }

        size = si_parse_lib_sizeoffset(size,2);
        if(size < 0)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
            goto err;
        }

        //解析des
        ret = si_parse_lib_eit_parse_loop_des(&eit->loop[i],p,section_end); 
        if(ret < 0)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("eit loop des parse err\n");
#endif
            goto err;
        }

        p = si_parse_lib_poffset(p,section_end,eit->loop[i].descriptor_length);
        if(NULL == p)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
            goto err;
        }

        size = si_parse_lib_sizeoffset(size , eit->loop[i].descriptor_length);
        if(size < 0)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
            goto err;
        }

    } 

    return size;
err:
    return -1;
}

/**
 *  @brief      把一段section数据解析成eit表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      eit数据
 *               NULL：发生错误
 */
GxSubsystemSiParseAtscEIT * GxSubsystm_AtscSiParseLibEitParse(uint8_t * section,uint32_t len) 
{
    GxSubsystemSiParseAtscEIT * eit = NULL;
    int32_t size = len;
    uint8_t * p = section;
    uint8_t * section_end;
    int32_t ret = 0;
    int32_t i = 0;


    if(len < 10 || NULL == section)
    {
        return NULL;
    }   
    /* 解析eit表头*/
    eit = GxCore_Mallocz(sizeof(GxSubsystemSiParseAtscEIT));
    if(NULL == eit)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("-------------------eit parse func malloc failed\n");
#endif
        goto err;
    }

    eit->table_id =  TODATA0_7BIT(p[0]);
    eit->section_syntax_indicator = TODATA7BIT(p[1]);
    if(ATSC_DEFAULT_VALUE_1 != eit->section_syntax_indicator)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the section_indicator is err!!");       
#endif
        goto err;
    }  
    eit->private_indicator = TODATA6BIT(p[1]);
    if(ATSC_DEFAULT_VALUE_1 != eit->private_indicator)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the private_indicator is err!!");       
#endif
        goto err;
    }

    eit->section_length = TODATA0_11BIT(p[1],p[2]);

    eit->source_id = TODATA0_15BIT(p[3],p[4]);
    eit->version_number = TODATA1_5BIT(p[5]);
    eit->current_next_indicator =TODATA0BIT(p[5]);
    if(ATSC_DEFAULT_VALUE_1 != eit->current_next_indicator)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("eit current_next_indictor err");
#endif
        goto err;
    }

    eit->section_number = TODATA0_7BIT(p[6]);
    /*EIT表中传输的section_number和last_section_number为可用值*/
    eit->last_section_number = TODATA0_7BIT(p[7]);
    eit->protocol_version = TODATA0_7BIT(p[8]);

    if(ATSC_DEFAULT_VALUE_0 != eit->protocol_version)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("eit protocol_version err");
#endif
    }
    eit->num_events_in_section = TODATA0_7BIT(p[9]);

    section_end = p + eit->section_length + 3;
    p = si_parse_lib_poffset(p,section_end,10);
    if(NULL == p)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
        goto err;
    }

    size = si_parse_lib_sizeoffset(size ,10);
    if(size < 0)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
        goto err;
    }

    size = si_parse_lib_sizeoffset(size , 4);
    if(size < 0)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
        goto err;
    }
    if(size != eit->section_length - 7 - 4)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("section_length is err");
#endif
        goto err;
    }


    //内循环解析
    if(eit->num_events_in_section > 0)
    {
        ret = si_parse_lib_eit_parse_loop(eit,p,size);
        if(ret < 0)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("eit loop parse err\n");
#endif
            goto err;
        }
        else
            size = ret;          
    }

    //跳过内循环
    for(i = 0 ;i<eit->num_events_in_section;i++)
    {
        p = si_parse_lib_poffset(p,section_end,eit->loop[i].title_length+12+eit->loop[i].descriptor_length);
        if(NULL == p)
        {
#ifdef ATSC_SI_DEBUG
            SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
            goto err;
        }    
    }
    /*是否要解析crc*/
    eit->CRC_32 = TODATA4_31BIT(p[0],p[1],p[2],p[3]);
    p = si_parse_lib_poffset(p,section_end,4);
    if(NULL == p)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
        goto err;
    }


    si_parse_lib_eit_add_protect((void*)eit);
    return eit;
err:
    si_parse_lib_eit_release(eit);
    return NULL;

}

/**
 *  @brief      释放eit的空间，应用应该在通过GxSubsystm_AtscSiParseLibEitParse获取
 *  解析好的eit数据之后某个时刻释放eit的空间。
 *   
 *  @param       GxSubsystemSiParseAtscEIT* eit:eit的存储空间，是通过GxSubsystm_AtscSiParseLibEitParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_AtscSiParseLibEitRelease(void * eit)
{
    uint32_t ret = 0;
    ret = si_parse_lib_eit_remove_protect(eit);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystem_ATSCSiParseLibeitparse* eit is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_eit_release((GxSubsystemSiParseAtscEIT *)eit);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

