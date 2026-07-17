/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_tot.c
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
static void si_parse_lib_tot_release(GxSubsystemSiParseTOT* tot);
static void si_parse_lib_tot_add_protect(void* p);
static uint32_t si_parse_lib_tot_remove_protect(void* p);
static int32_t si_parse_lib_tot_parse_loop1(GxSubsystemSiParseTOT* tot,uint8_t* src,uint32_t len); 

/*释放tot的空间*/
static void si_parse_lib_tot_release(GxSubsystemSiParseTOT* tot)
{
    uint32_t i = 0; 
    if(tot != NULL)
    {
        if(tot->local_time_des != NULL)
        {
            for(i=0; i<tot->local_time_des_count; i++)
            {
                if(tot->local_time_des[i].time_offset != NULL)
                {
                    GxCore_Free(tot->local_time_des[i].time_offset);
                }
            }
            GxCore_Free(tot->local_time_des);
        }
        GxCore_Free(tot);
    }
    return;
}
static void si_parse_lib_tot_add_protect(void* p)
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
static uint32_t si_parse_lib_tot_remove_protect(void* p)
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
static int32_t si_parse_lib_tot_parse_loop1(GxSubsystemSiParseTOT* tot,uint8_t* src,uint32_t len) 
{
    int32_t size = len;
    uint32_t loop_count = 0;
    uint8_t* p = src;
    uint32_t descriptor_length = 0;
    uint32_t i = 0;

    while(size > 0)
    {
        switch(p[0])
        {
            //case SI_PARSE_LIB_NETWORK_NAME_DES:
            case SI_PARSE_LIB_LOCAL_TIME_OFFSET_DES:
                loop_count++;
                tot->local_time_des = GxCore_Realloc(tot->local_time_des,
                        sizeof(GxSubsystemSiParseLocalTimeOfffsetDes)*loop_count);
                if(tot->local_time_des == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse tot no memery!!");
#endif  
                    goto err;
                }      
                tot->local_time_des[loop_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                tot->local_time_des[loop_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                tot->local_time_des[loop_count-1].time_offset_count = TODATA0_7BIT(p[1])/13;
				tot->local_time_des[loop_count-1].time_offset = NULL;
				if(tot->local_time_des[loop_count-1].time_offset_count != 0)
				{
					tot->local_time_des[loop_count-1].time_offset = GxCore_Malloc(sizeof(GxSubsystemSiParseLocalTimeOfffset)
							*tot->local_time_des[loop_count-1].time_offset_count);
					if(tot->local_time_des[loop_count-1].time_offset == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse tot no memery!!");
#endif  
						goto err;
					}
					p += 2;//跳过头
					size -= 2;
					for(i=0; i<tot->local_time_des[loop_count-1].time_offset_count; i++)
					{
						memcpy(tot->local_time_des[loop_count-1].time_offset[i].country,p,3);
						tot->local_time_des[loop_count-1].time_offset[i].country_region_id = TODATA2_7BIT(p[3]);
						tot->local_time_des[loop_count-1].time_offset[i].local_time_offset_polarity = TODATA0BIT(p[3]);
						tot->local_time_des[loop_count-1].time_offset[i].local_time_offset = TODATA0_15BIT(p[4],p[5]);
						memcpy(tot->local_time_des[loop_count-1].time_offset[i].time_of_change,&p[6],5);
						tot->local_time_des[loop_count-1].time_offset[i].next_time_offset = TODATA0_15BIT(p[11],p[12]);
						p += 13;
						size -= 13;
					}
				}
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
    tot->local_time_des_count = loop_count;
    return size;

err:
    tot->local_time_des_count = loop_count;
    return -1;
}

/**
 *  @brief      把一段section数据解析成tot表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      tot数据
 *               NULL：发生错误
 */
GxSubsystemSiParseTOT* GxSubsystm_SiParseLibTotParse(uint8_t* section,uint32_t size)
{
    GxSubsystemSiParseTOT* tot = NULL;
    int32_t len = (int32_t)size;
    uint8_t* p = section;
    int32_t ret = 0;

    if(section == NULL || size < 10)//tot表的头部有10字节
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("section is NULL or size is err!!");
#endif
        goto err;
    }
    tot = GxCore_Malloc(sizeof(GxSubsystemSiParseTOT));
    if(tot == NULL)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse tot no memery!!");
#endif
        goto err;
    }
    memset(tot,0,sizeof(GxSubsystemSiParseTOT));
    tot->table_id = TODATA0_7BIT(p[0]);
    tot->section_syntax_indicator = TODATA7BIT(p[1]);
    tot->reserved_future_use = TODATA6BIT(p[1]);
    tot->section_length = TODATA0_11BIT(p[1],p[2]);
    memcpy(tot->UTC_time,&p[3],5);
    tot->descriptors_loop_length = TODATA0_11BIT(p[8],p[9]);
    p += 10;//跳过头部
    len -= 10;//头部解析完成
    len -= 4;//去掉crc

    if(len != tot->section_length-7-4)//解析出来的大小和表里面的大小不一致
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the section size is err!!");
#endif  
        goto err;
    }
    ret = si_parse_lib_tot_parse_loop1(tot,p,tot->descriptors_loop_length);
    if(ret < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse loop1 err!!");
#endif
        goto err;
    }
    si_parse_lib_tot_add_protect((void*)tot);
    return tot;
err:
    si_parse_lib_tot_release(tot);
    return NULL;
}

/**
 *  @brief      释放tot的空间，应用应该在通过GxSubsystm_SiParseLibTotParse获取
 *  解析好的tot数据之后某个时刻释放tot的空间。
 *   
 *  @param       GxSubsystemSiParseTOT* tot:tot的存储空间，是通过GxSubsystm_SiParseLibTotParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibTotRelease(void* tot)
{
    uint32_t ret = 0;
    ret = si_parse_lib_tot_remove_protect(tot);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParseTOT* tot is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_tot_release((GxSubsystemSiParseTOT*)tot);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

