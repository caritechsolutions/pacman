/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_pat.c
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

static void si_parse_lib_pat_release(GxSubsystemSiParsePAT* pat);
static void si_parse_lib_pat_add_protect(void* p);
static uint32_t si_parse_lib_pat_remove_protect(void* p);

/*释放pat的空间*/
static void si_parse_lib_pat_release(GxSubsystemSiParsePAT* pat)
{
    if(pat != NULL)
    {
        if(pat->prog_info != NULL)
        {
            GxCore_Free(pat->prog_info);
        }
        GxCore_Free(pat);
    }
    return;
}

static void si_parse_lib_pat_add_protect(void* p)
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

static uint32_t si_parse_lib_pat_remove_protect(void* p)
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
 *  @brief      把一段section数据解析成pat表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      pat数据
 *               NULL：发生错误
 */
GxSubsystemSiParsePAT* GxSubsystm_SiParseLibPatParse(uint8_t* section,uint32_t size)
{
    GxSubsystemSiParsePAT* pat = NULL;
    int32_t len = (int32_t)size;
    uint8_t* p = section;
    uint32_t prog_count = 0;

    if(section == NULL || size < 8)//pat的头部有8字节
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("section is NULL or size is err!!");
#endif
        goto err;
    }
    pat = GxCore_Malloc(sizeof(GxSubsystemSiParsePAT));
    if(pat == NULL)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse pat no memery!!");
#endif
        goto err;
    }
    memset(pat,0,sizeof(GxSubsystemSiParsePAT));
    pat->table_id = TODATA0_7BIT(p[0]);
    pat->section_syntax_indicator = TODATA7BIT(p[1]);
    pat->section_length = TODATA0_11BIT(p[1],p[2]);
    pat->transport_stream_id = TODATA0_15BIT(p[3],p[4]);
    pat->version_number = TODATA1_5BIT(p[5]);
    pat->current_next_indicator = TODATA0BIT(p[5]);
    pat->section_number = TODATA0_7BIT(p[6]);
    pat->last_section_number = TODATA0_7BIT(p[7]);

    p += 8;//跳过头部
    len -= 8;//头部解析完成
    len -= 4;//去掉crc

    if(len != pat->section_length-5-4)//解析出来的大小和表里面的大小不一致
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the section size is err!!");
#endif  
        goto err;
    }

    while(len > 0)
    {
        prog_count++;
        pat->prog_info = GxCore_Realloc(pat->prog_info,sizeof(GxSubsystemSiParseProgInfo)*prog_count);
        if(pat->prog_info == NULL)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("parse pat no memery!!");
#endif  
            goto err;
        }       
        memset(&pat->prog_info[prog_count - 1],0,sizeof(GxSubsystemSiParseProgInfo));
        pat->prog_info[prog_count-1].prog_num = TODATA0_15BIT(p[0],p[1]);
        if(pat->prog_info[prog_count-1].prog_num == 0)
        {
            pat->prog_info[prog_count-1].network_pid = TODATA0_12BIT(p[2],p[3]); 
        }
        else
        {
            pat->prog_info[prog_count-1].pmt_pid = TODATA0_12BIT(p[2],p[3]);
        }
        p += 4;
        len -= 4;
    }
    pat->prog_count = prog_count;
    si_parse_lib_pat_add_protect((void*)pat);
    return pat;
err:
    pat->prog_count = prog_count;
    si_parse_lib_pat_release(pat);
    return NULL;
}

/**
 *  @brief      释放pat的空间，应用应该在通过GxSubsystm_SiParseLibPatParse获取
 *  解析好的pat数据之后某个时刻释放pat的空间。
 *   
 *  @param       GxSubsystemSiParsePAT* pat:pat的存储空间，是通过GxSubsystm_SiParseLibPatParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibPatRelease(void* pat)
{
    uint32_t ret = 0;
    ret = si_parse_lib_pat_remove_protect(pat);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParsePAT* pat is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_pat_release((GxSubsystemSiParsePAT*)pat);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

