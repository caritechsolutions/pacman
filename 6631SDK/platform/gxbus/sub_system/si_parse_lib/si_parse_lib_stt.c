/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_stt.c
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
static void si_parse_lib_stt_add_protect(void* p);
static uint32_t si_parse_lib_stt_remove_protect(void* p);
static void si_parse_lib_stt_release(GxSubsystemSiParseSTT * stt);
GxSubsystemSiParseSTT * GxSubsystm_SiParseLibSttParse(uint8_t * section,uint32_t len);
int32_t  GxSubsystm_SiParseLibSttRelease(void* stt);

static void si_parse_lib_stt_add_protect(void* p)
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

static uint32_t si_parse_lib_stt_remove_protect(void* p)
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


static void si_parse_lib_stt_release(GxSubsystemSiParseSTT * stt)
{
    if(stt)
    {
        GxCore_Free(stt);
    }
}

/**
 *  @brief      把一段section数据解析成stt表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      stt数据
 *               NULL：发生错误
 */
GxSubsystemSiParseSTT * GxSubsystm_SiParseLibSttParse(uint8_t * section,uint32_t len)
{
    GxSubsystemSiParseSTT * stt = NULL;
    int32_t size = len;
    uint8_t * p = section;
    uint8_t * section_end;


    if(len < 13 || NULL == section)
    {
        return NULL;
    }   
    /* 解析stt表头*/
    stt = GxCore_Mallocz(sizeof(GxSubsystemSiParseSTT));
    if(NULL == stt)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("-------------------stt parse func malloc failed\n");
#endif
        goto err;
    }

    stt->table_id =  TODATA0_7BIT(p[0]);
    stt->section_syntax_indicator = TODATA7BIT(p[1]);
    if(ATSC_DEFAULT_VALUE_1 != stt->section_syntax_indicator)
    {
#ifdef ATSC_SI_DEBUG
         SI_PARSE_LIB_ERR_PRINTF("the section_indicator is err!!");       
#endif
         goto err;
    }  
    stt->private_indicator = TODATA6BIT(p[1]);
    if(ATSC_DEFAULT_VALUE_1 != stt->private_indicator)
    {
#ifdef ATSC_SI_DEBUG
         SI_PARSE_LIB_ERR_PRINTF("the private_indicator is err!!");       
#endif
         goto err;
    }
    
    stt->section_length = TODATA0_11BIT(p[1],p[2]);
    stt->table_id_extension =TODATA0_15BIT(p[3],p[4]);
    if(ATSC_DEFAULT_VALUE_0 != stt->table_id_extension)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the table_id_extension is err");
#endif
        goto err;
    }
    stt->version_number = TODATA1_5BIT(p[5]);

    stt->current_next_indicator = TODATA0BIT(p[5]);
    if(ATSC_DEFAULT_VALUE_1 != stt->current_next_indicator)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the current_next_indictor is err");
#endif
        goto err;
    }
    stt->section_number = TODATA0_7BIT(p[6]);
    if(ATSC_DEFAULT_VALUE_0 != stt->section_number)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the section_number is err");
#endif
        goto err;
    }
    stt->last_section_number = TODATA0_7BIT(p[7]);
    if(ATSC_DEFAULT_VALUE_0 != stt->last_section_number)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the last_section_number is err");
#endif
        goto err;
    }
    stt->protocol_version =TODATA0_7BIT(p[8]);
    if(ATSC_DEFAULT_VALUE_0 != stt->protocol_version)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the protocol_version is err");
#endif
        goto err;
    }
    stt->system_time = TODATA0_31BIT(p[9],p[10],p[11],p[12]);;
    stt->GPS_UTC_offset = TODATA0_7BIT(p[13]);
    stt->daylight_saving = TODATA0_15BIT(p[14],p[15]);
    section_end = p + stt->section_length + 3;
    p = si_parse_lib_poffset(p,section_end,16);
    if(NULL == p)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("stt size err\n");
#endif
        goto err;
    }
    size = si_parse_lib_sizeoffset(size ,16);
    if(size < 0)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("stt size err\n");
#endif
        goto err;
    }

    size = si_parse_lib_sizeoffset(size , 4);
    if(size < 0)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("stt size err\n");
#endif
        goto err;
    }

    stt->CRC_32 = TODATA0_31BIT(p[0],p[1],p[2],p[3]);

    if(size != stt->section_length - 10 - 4 - 3)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("section_length is err");
#endif
        goto err;
    }

si_parse_lib_stt_add_protect((void*)stt);
return stt;
err:
    return NULL;
    si_parse_lib_stt_release(stt);
}


/**
 *  @brief      释放stt的空间，应用应该在通过GxSubsystm_SiParseLibSttParse获取
 *  解析好的stt数据之后某个时刻释放stt的空间。
 *   
 *  @param       GxSubsystemSiParseSTT* stt:stt的存储空间，是通过GxSubsystm_SiParseLibSttParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibSttRelease(void* stt)
{
    uint32_t ret = 0;
    ret = si_parse_lib_stt_remove_protect(stt);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParsestt* stt is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_stt_release((GxSubsystemSiParseSTT*)stt);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

