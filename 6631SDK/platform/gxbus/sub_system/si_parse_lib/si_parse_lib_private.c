/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_private.c
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

/******************function******************************/
uint8_t * si_parse_lib_poffset(uint8_t * p,uint8_t * section_end,uint32_t offset)
{
    if((p + offset) > section_end)
        return NULL;
    else
        return (p + offset);
}
int32_t si_parse_lib_sizeoffset(uint32_t size,uint32_t offset)
{
    if((size - offset)>=0)
        return (size - offset);
    else
        return -1;
}
void si_parse_lib_release_multiple_string(GxSubsystemSiParseMSSstruct * mul)
{
    int32_t i,j;
    if(mul)
    {
       if(mul->string_info) 
       {
           for(i = 0;i<mul->number_string;i++)
           {
               if(mul->string_info[i].segments_info)
               {
                   for(j = 0;j<mul->string_info[i].number_segments;j++)
                   {
                       if(mul->string_info[i].segments_info[j].string_byte)
                       {
                           GxCore_Free(mul->string_info[i].segments_info[j].string_byte);
                       }
                   }
                   GxCore_Free(mul->string_info[i].segments_info);
               }
           }
           GxCore_Free(mul->string_info);
       }
       // GxCore_Free(mul);传进来的都是局部变量结构体，不是malloc出来的，不用free
    }
}
//多张表中用到该结构解析，所以放到private
int32_t   si_parse_lib_parse_multiple_string( GxSubsystemSiParseMSSstruct * mul,uint8_t * section , uint8_t * section_end ,int32_t size)
{
    int i = 0,j= 0,string_byte_count = 0,count = 0;
    uint8_t * p = section;
    int multiple_string_size = size;


   //multiple_string 头解析 
    mul->number_string = TODATA0_7BIT(p[0]);
    p = si_parse_lib_poffset(p,section_end,1);
    if(NULL == p)
    {
        goto err;
    }
    size = si_parse_lib_sizeoffset(size , 1);
    if(size < 0)
    {
        goto err;
    }
    multiple_string_size = size - 1;
    count = mul->number_string;


    mul->string_info = GxCore_Mallocz(count * sizeof(GxSubsystemSiParseMssnumberStrings));
    if(NULL == mul->string_info)
    {
        goto err;
    }

    for(i=0 ; i < count ; i++)
    {
        //ISO_639结构体头
        memcpy(mul->string_info[i].ISO_639_language_code,p,3);
        mul->string_info[i].number_segments = TODATA0_7BIT(p[3]);
        p = si_parse_lib_poffset(p,section_end,4);
        if(NULL == p)
        {
            goto err;
        }
        size = si_parse_lib_sizeoffset(size,4);
        if(size < 0)
        {
            goto err;
        }

        if(mul->string_info[i].number_segments >multiple_string_size / 4)
            goto err;

        mul->string_info[i].segments_info = GxCore_Mallocz(mul->string_info[i].number_segments * 
                sizeof(GxSubsystemSiParseMssnumberSegments));
        if(NULL == mul->string_info[i].segments_info)
        {
            goto err;
        }
        for(j = 0;j< mul->string_info[i].number_segments;j++)
        {
            //segments头信息
            mul->string_info[i].segments_info[j].compression_type = TODATA0_7BIT(p[0]);
            mul->string_info[i].segments_info[j].mode = TODATA0_7BIT(p[1]);
            mul->string_info[i].segments_info[j].number_bytes = TODATA0_7BIT(p[2]);
            string_byte_count = mul->string_info[i].segments_info[j].number_bytes;
            p = si_parse_lib_poffset(p,section_end,3);
            if(NULL == p)
            {
                goto err;
            }
            size = si_parse_lib_sizeoffset(size,3);
            if(size < 0)
            {
                goto err;
            }

            mul->string_info[i].segments_info[j].string_byte = GxCore_Mallocz(string_byte_count + 1);
            if(NULL == mul->string_info[i].segments_info[j].string_byte)
                goto err;
            else
            {
                memcpy(mul->string_info[i].segments_info[j].string_byte,p ,string_byte_count);
            }
            p = si_parse_lib_poffset(p,section_end,string_byte_count);
            if(NULL == p)
                goto err;
            size = si_parse_lib_sizeoffset(size,string_byte_count);
            if(size < 0)
                goto err;
        }
    }
    return 0;
err:
    if(mul->string_info != NULL)
    {
        for(i=0 ; i < count ; i++)
        {
            if(mul->string_info[i].segments_info != NULL) 
            {
                GxCore_Free(mul->string_info[i].segments_info);
            }
        }
        GxCore_Free(mul->string_info);
    }
    return -1;
}



void si_parse_lib_add_protect(si_parse_lib_protect* head, void* p)
{
    si_parse_lib_protect* temp = NULL;
    temp = head;
    while(1)
    {
        if((temp)->next == NULL)
        {
            (temp)->next = (void*)GxCore_Malloc(sizeof(si_parse_lib_protect));
            if((temp)->next == NULL)
            {
                return;
            }
            ((si_parse_lib_protect*)((temp)->next))->p = p;
            ((si_parse_lib_protect*)((temp)->next))->next = NULL;
            break;
        }
        else
        {
            temp = (si_parse_lib_protect*)((temp)->next);
        }
    }
    return;
}

uint32_t si_parse_lib_remove_protect(si_parse_lib_protect* head,void* p)
{
    si_parse_lib_protect* temp = NULL;
    si_parse_lib_protect* temp1 = NULL;
    temp = (si_parse_lib_protect*)(head->next);
    temp1 = head;
    if(p == NULL || head == NULL)
    {
        return 0;
    }
    while(temp != NULL)
    {
        if(temp->p == p)
        {
            temp1->next = temp->next;
            GxCore_Free(temp);
            return 1;//在保护记录表里找到了对应的指针
        }
        else
        {
            temp1 = temp;
            temp = (si_parse_lib_protect*)((temp)->next);
        }
    }
    return 0;//在保护记录表里没有找到对应的指针
}

/* End of file -------------------------------------------------------------*/

