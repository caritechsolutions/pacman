/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_mgt.c
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
static void si_parse_lib_mgt_add_protect(void* p);
static void si_parse_lib_mgt_release(GxSubsystemSiParseMGT * mgt);
static uint32_t si_parse_lib_mgt_remove_protect(void* p);
static int32_t si_parse_lib_mgt_loop(GxSubsystemSiParseMGT * mgt , uint8_t * section , int32_t size);
int32_t  GxSubsystm_SiParseLibMgtRelease(void* mgt);
GxSubsystemSiParseMGT * GxSubsystm_SiParseLibMgtParse(uint8_t * section,uint32_t len);




static void si_parse_lib_mgt_add_protect(void* p)
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

static uint32_t si_parse_lib_mgt_remove_protect(void* p)
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


static void si_parse_lib_mgt_release(GxSubsystemSiParseMGT * mgt)
{
    if(mgt)
    {
        if(mgt->loop)
        {
            GxCore_Free(mgt->loop);
        }
        GxCore_Free(mgt);
    }
}

static int32_t si_parse_lib_mgt_loop(GxSubsystemSiParseMGT * mgt , uint8_t * section , int32_t size)
{
    int i = 0,count = 0;
    uint8_t * p = section;
    uint8_t * section_end;

    section_end = p + mgt->section_length + 3 -11; 
    count = mgt->tables_defined;
    mgt->loop = GxCore_Mallocz(count * sizeof(GxSubsystemSiParseMGTLoop));
    if(NULL == mgt->loop)
    {
        SI_PARSE_LIB_ERR_PRINTF("parse mgt loop malloc failed\n");
        return -1;
    }   
    for(i = 0 ; i < count ; i++)
    {
        mgt->loop[i].table_type = TODATA0_15BIT(p[0],p[1]);
        mgt->loop[i].table_type_PID =TODATA0_12BIT(p[2],p[3]);
        mgt->loop[i].table_type_version_number =TODATA0_4BIT(p[4]);
        mgt->loop[i].number_bytes = TODATA0_31BIT(p[5],p[6],p[7],p[8]);
        mgt->loop[i].table_type_descriptors_length =TODATA0_11BIT(p[9],p[10]);
        p = si_parse_lib_poffset(p,section_end,11);
        if(NULL == p)
        {
            return -2;
        }
        p = si_parse_lib_poffset(p,section_end,mgt->loop->table_type_descriptors_length);
        if(NULL == p)
        {
            return -3;
        }
        size = si_parse_lib_sizeoffset(size,11);
        if(size < 0)
        {
            return -4;
        }
        size = si_parse_lib_sizeoffset(size,mgt->loop->table_type_descriptors_length);
        if(size < 0)
        {
            return -5;
        }
        
    } 

    return size;
}


/**
 *  @brief      把一段section数据解析成mgt表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      mgt数据
 *               NULL：发生错误
 */
GxSubsystemSiParseMGT * GxSubsystm_SiParseLibMgtParse(uint8_t * section,uint32_t len)
{
    GxSubsystemSiParseMGT * Mgt = NULL;
    int32_t i;
    int32_t size = len;
    uint8_t * p = section;
    uint8_t * section_end;
    int32_t ret = 0;


    if(len < 11 || NULL == section)
    {
        return NULL;
    }   
    /* 解析mgt表头*/
    Mgt = GxCore_Mallocz(sizeof(GxSubsystemSiParseMGT));
    if(NULL == Mgt)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("-------------------MGT parse func malloc failed\n");
#endif
        goto err;
    }

    Mgt->table_id =  TODATA0_7BIT(p[0]);
    Mgt->section_indicator = TODATA7BIT(p[1]);
    if(ATSC_DEFAULT_VALUE_1 != Mgt->section_indicator)
    {
#ifdef ATSC_SI_DEBUG
         SI_PARSE_LIB_ERR_PRINTF("the section_indicator is err!!");       
#endif
         goto err;
    }  
    Mgt->private_indicator = TODATA6BIT(p[1]);
    if(ATSC_DEFAULT_VALUE_1 != Mgt->section_indicator)
    {
#ifdef ATSC_SI_DEBUG
         SI_PARSE_LIB_ERR_PRINTF("the private_indicator is err!!");       
#endif
         goto err;
    }
    
    Mgt->section_length = TODATA0_11BIT(p[1],p[2]);
    Mgt->table_id_extension =TODATA0_15BIT(p[3],p[4]);
    if(ATSC_DEFAULT_VALUE_0 != Mgt->table_id_extension)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the table_id_extension is err");
#endif
        goto err;
    }
    Mgt->version_number = TODATA1_5BIT(p[5]);

    Mgt->current_next_indictor = TODATA0BIT(p[5]);
    if(ATSC_DEFAULT_VALUE_1 != Mgt->current_next_indictor)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the current_next_indictor is err");
#endif
        goto err;
    }
    Mgt->section_number = TODATA0_7BIT(p[6]);
    if(ATSC_DEFAULT_VALUE_0 != Mgt->section_number)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the section_number is err");
#endif
        goto err;
    }
    Mgt->last_section_number = TODATA0_7BIT(p[7]);
    if(ATSC_DEFAULT_VALUE_0 != Mgt->last_section_number)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the last_section_number is err");
#endif
        goto err;
    }
    Mgt->protocol_version =TODATA0_7BIT(p[8]);
    if(ATSC_DEFAULT_VALUE_0 != Mgt->protocol_version)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("the protocol_version is err");
#endif
        goto err;
    }
    Mgt->tables_defined = TODATA0_15BIT(p[9],p[10]);;


    section_end = p + Mgt->section_length + 3;
    p = si_parse_lib_poffset(p,section_end,11);
    if(NULL == p)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("mgt contein head only");
#endif
        goto err;
    }

    size = si_parse_lib_sizeoffset(size ,11);
    if(size < 0)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("mgt contein head only");
#endif
        goto err;
    }

    size = si_parse_lib_sizeoffset(size , 4);
    if(size < 0)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("mgt size err");
#endif
        goto err;
    }

    if(size != Mgt->section_length - 8 - 4)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("section_length is err");
#endif
        goto err;
    }

    if(Mgt->tables_defined > 0)
    {
        ret = si_parse_lib_mgt_loop(Mgt,p,size);
        if(ret < 0)
		{
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("mgt first loop err");
#endif
            goto err;
		}
        else
            size = ret;          
    }

    //跳过loop长度
    for(i = 0;i<Mgt->tables_defined;i++)
    {
        p = si_parse_lib_poffset(p,section_end,11);//loop中每个头的大小都是11
        if(NULL == p)
		{
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("mgt first loop err");
#endif
            goto err;
		}
        p = si_parse_lib_poffset(p,section_end,Mgt->loop[i].table_type_descriptors_length);//跳过每个loop成员的内容段
        if(NULL == p)
		{
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("mgt first loop err");
#endif
            goto err;
		}
    }
    
    Mgt->descriptors_length = TODATA0_11BIT(p[0],p[1]);
    p = si_parse_lib_poffset(p,section_end,2);
    if(NULL == p)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("mgt size err");
#endif
        goto err;
    }

    p = si_parse_lib_poffset(p,section_end ,Mgt->descriptors_length);
    if(NULL == p)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("mgt size err");
#endif
        goto err;
    }

    size = si_parse_lib_sizeoffset(size,2);
    if(size < 0)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("mgt size err");
#endif
        goto err;
    }
    size = si_parse_lib_sizeoffset(size , Mgt->descriptors_length);
    if(size < 0)
    {
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("mgt size err");
#endif
        goto err;
    }

    /*是否要解析crc*/
    Mgt->CRC_32 = TODATA4_31BIT(p[0],p[1],p[2],p[3]);
    p = si_parse_lib_poffset(p,section_end,4);
    if(p == NULL)
	{
#ifdef ATSC_SI_DEBUG
        SI_PARSE_LIB_ERR_PRINTF("mgt size err");
#endif
        goto err;
	}

    si_parse_lib_mgt_add_protect((void*)Mgt);
    return Mgt;
err:
    si_parse_lib_mgt_release(Mgt);
    return NULL;
} 

/**
 *  @brief      释放mgt的空间，应用应该在通过GxSubsystm_SiParseLibMgtParse获取
 *  解析好的mgt数据之后某个时刻释放mgt的空间。
 *   
 *  @param       GxSubsystemSiParseMGT* mgt:mgt的存储空间，是通过GxSubsystm_SiParseLibMgtParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibMgtRelease(void* mgt)
{
    uint32_t ret = 0;
    ret = si_parse_lib_mgt_remove_protect(mgt);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParseMGT* mgt is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_mgt_release((GxSubsystemSiParseMGT*)mgt);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

