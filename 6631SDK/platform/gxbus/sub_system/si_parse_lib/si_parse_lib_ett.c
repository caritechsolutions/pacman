/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_ett.c
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
static void si_parse_lib_ett_add_protect(void* p);
static uint32_t si_parse_lib_ett_remove_protect(void* p);
static void si_parse_lib_ett_release(GxSubsystemSiParseETT * ett);
GxSubsystemSiParseETT * GxSubsystm_SiParseLibEttParse(uint8_t * section,uint32_t len);
int32_t  GxSubsystm_SiParseLibEttRelease(void* ett);

static void si_parse_lib_ett_add_protect(void* p)
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

static uint32_t si_parse_lib_ett_remove_protect(void* p)
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


static void si_parse_lib_ett_release(GxSubsystemSiParseETT * ett)
{
    if(ett)
    {
        si_parse_lib_release_multiple_string(&ett->extended_text_message);
        GxCore_Free(ett);
    }
}

/**
 *  @brief      把一段section数据解析成ett表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      ett数据
 *               NULL：发生错误
 */
GxSubsystemSiParseETT * GxSubsystm_SiParseLibEttParse(uint8_t * section,uint32_t len)
{
    GxSubsystemSiParseETT * ett = NULL;
    int32_t size = len;
    uint8_t * p = section;
    uint8_t * section_end;
    int32_t ret;


    if(len < 13 || NULL == section)
    {
        return NULL;
    }   
    /* 解析ett表头*/
    ett = GxCore_Mallocz(sizeof(GxSubsystemSiParseETT));
    if(NULL == ett)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("-------------------ett parse func malloc failed\n");
#endif
        goto err;
    }

    ett->table_id =  TODATA0_7BIT(p[0]);
    ett->section_syntax_indicator = TODATA7BIT(p[1]);
    if(ATSC_DEFAULT_VALUE_1 != ett->section_syntax_indicator)
    {
#ifdef ATSC_SI_DEBUG
         SI_PARSE_LIB_ERR_PRINTF("the section_indicator is err!!");       
#endif
         goto err;
    }  
    ett->private_indicator = TODATA6BIT(p[1]);
    if(ATSC_DEFAULT_VALUE_1 != ett->private_indicator)
    {
#ifdef ATSC_SI_DEBUG
         SI_PARSE_LIB_ERR_PRINTF("the private_indicator is err!!");       
#endif
         goto err;
    }
    
    ett->section_length = TODATA0_11BIT(p[1],p[2]);
    ett->ETT_table_id_extension =TODATA0_15BIT(p[3],p[4]);
    ett->version_number = TODATA1_5BIT(p[5]);

    ett->current_next_indicator = TODATA0BIT(p[5]);
    if(ATSC_DEFAULT_VALUE_1 != ett->current_next_indicator)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the current_next_indictor is err");
#endif
        goto err;
    }
    ett->section_number = TODATA0_7BIT(p[6]);
    if(ATSC_DEFAULT_VALUE_0 != ett->section_number)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the section_number is err");
#endif
        goto err;
    }
    ett->last_section_number = TODATA0_7BIT(p[7]);
    if(ATSC_DEFAULT_VALUE_0 != ett->last_section_number)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the last_section_number is err");
#endif
        goto err;
    }
    ett->protocol_version =TODATA0_7BIT(p[8]);
    if(ATSC_DEFAULT_VALUE_0 != ett->protocol_version)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the protocol_version is err");
#endif
        goto err;
    }
    ett->ETM_id = TODATA0_31BIT(p[9],p[10],p[11],p[12]);;


    section_end = p + ett->section_length + 3;
    p = si_parse_lib_poffset(p,section_end,13);
    if(NULL == p)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("ett size err\n");
#endif
        goto err;
    }
    size = si_parse_lib_sizeoffset(size ,13);
    if(size < 0)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("ett size err\n");
#endif
        goto err;
    }

    size = si_parse_lib_sizeoffset(size , 4);
    if(size < 0)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("ett size err\n");
#endif
        goto err;
    }

    if(size != ett->section_length - 10 - 4)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("section_length is err");
#endif
        goto err;
    }
    //这里没有明确string的长度，所以直接传size进去
    ret = si_parse_lib_parse_multiple_string(&ett->extended_text_message,p,section_end,size);
    if(ret < 0)
    {
        goto err;
    }

    si_parse_lib_ett_add_protect((void*)ett);
    return ett;
err:
    si_parse_lib_ett_release(ett);
    return NULL;
}

/**
 *  @brief      释放ett的空间，应用应该在通过GxSubsystm_SiParseLibEttParse获取
 *  解析好的ett数据之后某个时刻释放ett的空间。
 *   
 *  @param       GxSubsystemSiParseETT* ett:ett的存储空间，是通过GxSubsystm_SiParseLibEttParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibEttRelease(void* ett)
{
    uint32_t ret = 0;
    ret = si_parse_lib_ett_remove_protect(ett);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParseett* ett is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_ett_release((GxSubsystemSiParseETT*)ett);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

