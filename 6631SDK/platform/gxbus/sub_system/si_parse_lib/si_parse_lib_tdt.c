/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_tdt.c
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

static void si_parse_lib_tdt_release(GxSubsystemSiParseTDT* tdt);
static void si_parse_lib_tdt_add_protect(void* p);
static uint32_t si_parse_lib_tdt_remove_protect(void* p);

/*释放tdt的空间*/
static void si_parse_lib_tdt_release(GxSubsystemSiParseTDT* tdt)
{
    if(tdt != NULL)
    {
        GxCore_Free(tdt);
    }
    return;
}

static void si_parse_lib_tdt_add_protect(void* p)
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

static uint32_t si_parse_lib_tdt_remove_protect(void* p)
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

/**
 *  @brief      把一段section数据解析成tdt表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      tdt数据
 *               NULL：发生错误
 */
GxSubsystemSiParseTDT* GxSubsystm_SiParseLibTdtParse(uint8_t* section,uint32_t size)
{
    GxSubsystemSiParseTDT* tdt = NULL;
    uint8_t* p = section;

    if(section == NULL || size !=8)//tdt的有8字节
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("section is NULL or size is err!!");
#endif
        goto err;
    }
    tdt = GxCore_Malloc(sizeof(GxSubsystemSiParseTDT));
    if(tdt == NULL)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse tdt no memery!!");
#endif
        goto err;
    }
    memset(tdt,0,sizeof(GxSubsystemSiParseTDT));
    tdt->table_id = TODATA0_7BIT(p[0]);
    tdt->section_syntax_indicator = TODATA7BIT(p[1]);
    tdt->section_length = TODATA0_11BIT(p[1],p[2]);
    if(tdt->section_length != 5)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the section size is err!!");
#endif  
        goto err;
    }
    memcpy((uint8_t*)(tdt->UTC_time),&p[3],5);

    si_parse_lib_tdt_add_protect((void*)tdt);
    return tdt;
err:
   si_parse_lib_tdt_release(tdt);
    return NULL;
}

/**
 *  @brief      释放tdt的空间，应用应该在通过GxSubsystm_SiParseLibTdtParse获取
 *  解析好的tdt数据之后某个时刻释放tdt的空间。
 *   
 *  @param       GxSubsystemSiParseTDT* tdt:tdt的存储空间，是通过GxSubsystm_SiParseLibTdtParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibTdtRelease(void* tdt)
{
    uint32_t ret = 0;
    ret = si_parse_lib_tdt_remove_protect(tdt);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParseTDT* tdt is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_tdt_release((GxSubsystemSiParseTDT*)tdt);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

