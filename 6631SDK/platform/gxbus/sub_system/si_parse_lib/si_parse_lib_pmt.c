/*****************************************************************************
* 						   CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_pmt.c
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

static void si_parse_lib_pmt_release(GxSubsystemSiParsePMT* pmt);
static void si_parse_lib_pmt_add_protect(void* p);
static uint32_t si_parse_lib_pmt_remove_protect(void* p);
static int32_t si_parse_lib_pmt_parse_loop1(GxSubsystemSiParsePMT* pmt,uint8_t* src,uint32_t len);
static int32_t si_parse_lib_pmt_parse_loop2(GxSubsystemSiParsePMT* pmt,uint8_t* src,uint8_t * section_end,uint32_t len);
static int32_t si_parse_lib_pmt_parse_loop2_des(GxSubsystemSiParsePmtLoop2* loop2,uint8_t * section_end,uint8_t* src,uint32_t len);
static int32_t si_parse_lib_pmt_parse_ttx(GxSubsystemSiParseTtxInfo* ttx_info,uint8_t* src,uint32_t len);
static void si_parse_lib_pmt_release_loop2(GxSubsystemSiParsePmtLoop2* loop2);


static void si_parse_lib_pmt_release_loop2(GxSubsystemSiParsePmtLoop2* loop2)
{
    uint32_t i = 0;
    uint32_t j = 0;
    if(loop2)
    {
        if(loop2->vbi_ttx)
        {
            for(i=0; i<loop2->vbi_ttx_count; i++)
            {
                if(loop2->vbi_ttx[i].ttx_info)
                {
                    GxCore_Free(loop2->vbi_ttx[i].ttx_info);
                }
            }
            GxCore_Free(loop2->vbi_ttx);
        }
        if(loop2->vbi_ttx_data)
        {
            for(i=0; i<loop2->vbi_ttx_data_count; i++)
            {
                if(loop2->vbi_ttx_data[i].ttx_data)
                {
                    for(j=0; j<loop2->vbi_ttx_data[i].ttx_data_count; j++)
                    {
                        if(loop2->vbi_ttx_data[i].ttx_data[j].info)
                        {
                            GxCore_Free(loop2->vbi_ttx_data[i].ttx_data[j].info);
                        }
                    }
                    GxCore_Free(loop2->vbi_ttx_data[i].ttx_data);
                }
            }
            GxCore_Free(loop2->vbi_ttx_data);
        }
        if(loop2->stream_identifier)
        {
            GxCore_Free(loop2->stream_identifier);
        }
        if(loop2->ttx)
        {
            for(i=0; i<loop2->ttx_count; i++)
            {
                if(loop2->ttx[i].ttx_info)
                {
                    GxCore_Free(loop2->ttx[i].ttx_info);
                }
            }
            GxCore_Free(loop2->ttx);
        }
        if(loop2->subtitling)
        {
            for(i=0; i<loop2->subtitling_count; i++)
            {
                if(loop2->subtitling[i].subt_info)
                {
                    GxCore_Free(loop2->subtitling[i].subt_info);
                }
            }
            GxCore_Free(loop2->subtitling);
        }
        if(loop2->ac3)
        {
            GxCore_Free(loop2->ac3);
        }
        if(loop2->eac3)
        {
            GxCore_Free(loop2->eac3);
        }
        if(loop2->ancillary_data)
        {
            GxCore_Free(loop2->ancillary_data);
        }
        if(loop2->ca)
        {
            GxCore_Free(loop2->ca);
        }
        if(loop2->iso_639)
        {
            for(i=0; i<loop2->iso_639_count; i++)
            {
                if(loop2->iso_639[i].language)
                {
                    GxCore_Free(loop2->iso_639[i].language);
                }
            }
            GxCore_Free(loop2->iso_639);
        }
        if(loop2->data_broadcast_id)
        {
            for(i=0; i<loop2->data_broadcast_id_count; i++)
            {
                if(loop2->data_broadcast_id[i].id_selector_byte)
                {
                    GxCore_Free(loop2->data_broadcast_id[i].id_selector_byte);
                }
            }
            GxCore_Free(loop2->data_broadcast_id);
        }
        if(loop2->application_signalling)
        {
            for(i=0; i<loop2->application_signalling_count; i++)
            {
                if(loop2->application_signalling[i].ait_info)
                {
                    GxCore_Free(loop2->application_signalling[i].ait_info);
                }
            }
            GxCore_Free(loop2->application_signalling);
        }
    }
    return;
}

/*释放pmt的空间*/
static void si_parse_lib_pmt_release(GxSubsystemSiParsePMT* pmt)
{
    uint32_t i = 0;
    if(pmt != NULL)
    {
        if(pmt->loop1.ca != NULL)
        {
            GxCore_Free(pmt->loop1.ca);
        }
        if(pmt->loop2)
        {
            for(i=0; i<pmt->loop2_count; i++)
            {
                si_parse_lib_pmt_release_loop2(&(pmt->loop2[i]));
            }
            GxCore_Free(pmt->loop2);
        }
        GxCore_Free(pmt);
    }
    return;
}

static void si_parse_lib_pmt_add_protect(void* p)
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

static uint32_t si_parse_lib_pmt_remove_protect(void* p)
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

static int32_t si_parse_lib_pmt_parse_loop1(GxSubsystemSiParsePMT* pmt,uint8_t* src,uint32_t len)
{
    int32_t size = len;
    uint32_t ca_count = 0;
    uint8_t* p = src;
    uint8_t * section_end = p + pmt->section_length + 3 - 12;
    uint32_t descriptor_length = 0;

    while(size > 0)
    {
        switch(p[0])
        {
            case SI_PARSE_LIB_CA_DES:
                ca_count++;
                pmt->loop1.ca = GxCore_Realloc(pmt->loop1.ca,sizeof(GxSubsystemSiParseCaDes)*ca_count);
                if(pmt->loop1.ca == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                    goto err;
                }
                pmt->loop1.ca[ca_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                pmt->loop1.ca[ca_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                pmt->loop1.ca[ca_count-1].ca_system_id = TODATA0_15BIT(p[2],p[3]);
                pmt->loop1.ca[ca_count-1].ca_pid = TODATA0_12BIT(p[4],p[5]);
                pmt->loop1.ca[ca_count-1].private_data_length = pmt->loop1.ca[ca_count-1].descriptor_length - 4;
                pmt->loop1.ca[ca_count-1].private_data = NULL;
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("pmt loop1 offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("pmt loop1 size offset err\n");
#endif
                    goto err;
                }
                //跳过该tag剩余字节
                p = si_parse_lib_poffset(p,section_end,pmt->loop1.ca[ca_count-1].descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("pmt loop1 offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,pmt->loop1.ca[ca_count-1].descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("pmt loop1 size offset err\n");
#endif
                    goto err;
                }
                break;

            default:
                descriptor_length = TODATA0_7BIT(p[1]);
                //跳过头
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("pmt loop1 offset err\n");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("pmt loop1 size offset err\n");
#endif
                    goto err;
                }
                //跳过该tag剩余字节
                p = si_parse_lib_poffset(p,section_end,descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("pmt loop1 offset err\n");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size,descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("pmt loop1 size offset err\n");
#endif
                    goto err;
                }
                break;
        }
    }
    pmt->loop1.ca_count = ca_count;
    return size;

err:
    pmt->loop1.ca_count = ca_count;
    return -1;
}

static int32_t si_parse_lib_pmt_parse_ttx(GxSubsystemSiParseTtxInfo* ttx_info,uint8_t* src,uint32_t len)
{
    int32_t size = len;
    uint8_t* p = src;
    uint32_t i = 0;

    while(size > 0)
    {
        memcpy(ttx_info[i].language,p,3);
        ttx_info[i].teletex_type = TODATA3_7BIT(p[3]);
        ttx_info[i].magazine_number = TODATA0_2BIT(p[3]);
        ttx_info[i].page_number = TODATA0_7BIT(p[4]);
        p += 5;
        size -= 5;
        i++;
    }
    return size;
}

static int32_t si_parse_lib_pmt_parse_vbi_ttx_data(GxSubsystemSiParseVbiDataDes* ttx_data_des,uint8_t* src,uint32_t len)
{
    int32_t size = len;
    int32_t count = 0;
    uint8_t* p = src;
    uint32_t i = 0;

    while(size > 0)
    {
        count++;
        ttx_data_des->ttx_data = GxCore_Realloc(ttx_data_des->ttx_data,sizeof(GxSubsystemSiParseTtxData)*count);
        if(ttx_data_des->ttx_data == NULL)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                    goto err;
        }
        ttx_data_des->ttx_data[count-1].data_service_id = TODATA0_7BIT(p[0]);
        ttx_data_des->ttx_data[count-1].descriptor_length = TODATA0_7BIT(p[1]);
        p += 2;//跳到实际内容处
        if(ttx_data_des->ttx_data[count-1].data_service_id == 0x1 ||
                ttx_data_des->ttx_data[count-1].data_service_id == 0x2 ||
                ttx_data_des->ttx_data[count-1].data_service_id == 0x4 ||
                ttx_data_des->ttx_data[count-1].data_service_id == 0x5 ||
                ttx_data_des->ttx_data[count-1].data_service_id == 0x6 ||
                ttx_data_des->ttx_data[count-1].data_service_id == 0x7 )
        {
            ttx_data_des->ttx_data[count-1].count = ttx_data_des->ttx_data[count-1].descriptor_length/8;
			ttx_data_des->ttx_data[count-1].info = NULL;
			if(ttx_data_des->ttx_data[count-1].count != 0)
			{
				ttx_data_des->ttx_data[count-1].info = GxCore_Malloc(sizeof(GxSubsystemSiParseTtxDataInfo)*ttx_data_des->ttx_data[count-1].count);
				if(ttx_data_des->ttx_data[count-1].info == NULL)
				{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
					SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
					goto err;
				}
				for(i=0; i<ttx_data_des->ttx_data[count-1].count; i++)
				{
					ttx_data_des->ttx_data[count-1].info[i].field_parity = TODATA5BIT(p[i]);
				}
			}
        }
        p += ttx_data_des->ttx_data[count-1].descriptor_length;//跳到下一个外循环
        size -= 2;
        size -= ttx_data_des->ttx_data[count-1].descriptor_length;
    }
    ttx_data_des->ttx_data_count = count;
    return count;
err:
    ttx_data_des->ttx_data_count = count;
    return -1;
}

static int32_t si_parse_lib_pmt_parse_subt(GxSubsystemSiParseSubtInfo* subt_info,uint8_t* src,uint32_t len)
{
    int32_t size = len;
    uint8_t* p = src;
    uint32_t i = 0;

    while(size > 0)
    {
        memcpy(subt_info[i].language,p,3);
        subt_info[i].substitling_type = TODATA0_7BIT(p[3]);
        subt_info[i].composition_page_id = TODATA0_15BIT(p[4],p[5]);
        subt_info[i].ancillary_page_id = TODATA0_15BIT(p[6],p[7]);
        p += 8;
        size -= 8;
        i++;
    }
    return size;
}

static int32_t si_parse_lib_pmt_parse_cc(GxCcsystemSiParseCcInfo* cc_info,uint8_t* src,uint32_t len)
{
    int32_t size = len;
    uint8_t* p = src;
    uint32_t i = 0;

    while(size > 0)
    {
        cc_info[i].additional_data_component_info = p[0];
        p += 1;
        size -= 1;
        i++;
    }
    return size;
}


static int32_t si_parse_lib_pmt_parse_loop2_des(GxSubsystemSiParsePmtLoop2* loop2,uint8_t* src,uint8_t * section_end,uint32_t len)
{
    int32_t size = len;
    uint32_t vbi_ttx_count = 0;
    uint32_t vbi_ttx_data_count = 0;
    uint32_t stream_identifier_count = 0;
    uint32_t ttx_count = 0;
    uint32_t subtitling_count = 0;
    uint32_t ac3_count = 0;
    uint32_t eac3_count = 0;
    uint32_t ancillary_data_count = 0;
    uint32_t ca_count = 0;
    uint32_t aribcc_count = 0;
    uint32_t iso_639_count = 0;
    uint32_t data_broadcast_id_count = 0;
	uint32_t application_signalling_count = 0;
    uint8_t* p = src;
    uint32_t descriptor_length = 0;
    int32_t ret = 0;
	int32_t i = 0;

    while(size > 0)
    {
        switch(p[0])
        {
            case SI_PARSE_LIB_VBI_TTX_DES:
                vbi_ttx_count++;
                loop2->vbi_ttx = GxCore_Realloc(loop2->vbi_ttx,sizeof(GxSubsystemSiParseVbiDes)*vbi_ttx_count);
                memset(((void*)loop2->vbi_ttx)+sizeof(GxSubsystemSiParseVbiDes)*(vbi_ttx_count-1),0,sizeof(GxSubsystemSiParseVbiDes));
                if(loop2->vbi_ttx == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                    goto err;
                }
                loop2->vbi_ttx[vbi_ttx_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->vbi_ttx[vbi_ttx_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->vbi_ttx[vbi_ttx_count-1].txt_info_count = loop2->vbi_ttx[vbi_ttx_count-1].descriptor_length/40;
                loop2->vbi_ttx[vbi_ttx_count-1].ttx_info = NULL;
                if(loop2->vbi_ttx[vbi_ttx_count-1].txt_info_count > 0 )
                {
                    loop2->vbi_ttx[vbi_ttx_count-1].ttx_info = GxCore_Malloc(sizeof(GxSubsystemSiParseTtxInfo)*loop2->vbi_ttx[vbi_ttx_count-1].txt_info_count);
                    if(loop2->vbi_ttx[vbi_ttx_count-1].ttx_info == NULL)
                    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                        SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                        goto err;
                    }
                    ret = si_parse_lib_pmt_parse_ttx(loop2->vbi_ttx[vbi_ttx_count-1].ttx_info,
                            &p[2],
                            loop2->vbi_ttx[vbi_ttx_count-1].descriptor_length);
                    if(ret != 0)
                    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                        SI_PARSE_LIB_ERR_PRINTF("parse vbi ttx des err!!");
#endif
                        goto err;
                    }
                }
                //跳过头
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }
                //跳过该tag剩余字节
                p = si_parse_lib_poffset(p,section_end,loop2->vbi_ttx[vbi_ttx_count-1].descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size,loop2->vbi_ttx[vbi_ttx_count-1].descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }
                break;

            case SI_PARSE_LIB_VBI_TTX_DATA_DES:
                vbi_ttx_data_count++;
                loop2->vbi_ttx_data = GxCore_Realloc(loop2->vbi_ttx_data,sizeof(GxSubsystemSiParseVbiDataDes)*vbi_ttx_data_count);
                memset(((void*)loop2->vbi_ttx_data)+sizeof(GxSubsystemSiParseVbiDataDes)*(vbi_ttx_data_count-1),0,sizeof(GxSubsystemSiParseVbiDataDes));
                if(loop2->vbi_ttx_data == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                    goto err;
                }
                loop2->vbi_ttx_data[vbi_ttx_data_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->vbi_ttx_data[vbi_ttx_data_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                if(loop2->vbi_ttx_data[vbi_ttx_data_count-1].descriptor_length != 0)
                {
                    ret = si_parse_lib_pmt_parse_vbi_ttx_data(&loop2->vbi_ttx_data[vbi_ttx_data_count-1],
                            &p[2],
                            loop2->vbi_ttx_data[vbi_ttx_data_count-1].descriptor_length);
                    if(ret < 0)
                    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                        SI_PARSE_LIB_ERR_PRINTF("parse vbi ttx data des err!!");
#endif
                        goto err;
                    }
                }
                //跳过头
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }

                //跳过该tag剩余字节
                p = si_parse_lib_poffset(p,section_end,loop2->vbi_ttx_data[vbi_ttx_data_count-1].descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,loop2->vbi_ttx_data[vbi_ttx_data_count-1].descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }
                break;

            case SI_PARSE_LIB_STREAM_IDENTIFIER_DES:
                stream_identifier_count++;
                loop2->stream_identifier = GxCore_Realloc(loop2->stream_identifier,sizeof(GxSubsystemSiParseStreamIdentifierDes)*stream_identifier_count);
                if(loop2->stream_identifier == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                    goto err;
                }
                loop2->stream_identifier[stream_identifier_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->stream_identifier[stream_identifier_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->stream_identifier[stream_identifier_count-1].component_tag = TODATA0_7BIT(p[2]);
                //跳过头
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }

                //跳过该tag剩余字节
                p = si_parse_lib_poffset(p,section_end,loop2->stream_identifier[stream_identifier_count-1].descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,loop2->stream_identifier[stream_identifier_count-1].descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }

                break;

            case SI_PARSE_LIB_TELETEXT_DES:
                ttx_count++;
                loop2->ttx = GxCore_Realloc(loop2->ttx,sizeof(GxSubsystemSiParseTtxDes)*ttx_count);
                memset(((void*)loop2->ttx)+sizeof(GxSubsystemSiParseTtxDes)*(ttx_count-1),0,sizeof(GxSubsystemSiParseTtxDes));
                if(loop2->ttx == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                    goto err;
                }
                loop2->ttx[ttx_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->ttx[ttx_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->ttx[ttx_count-1].txt_info_count = loop2->ttx[ttx_count-1].descriptor_length/5;
                loop2->ttx[ttx_count-1].ttx_info = NULL;
                if(loop2->ttx[ttx_count-1].txt_info_count != 0)
                {
                    loop2->ttx[ttx_count-1].ttx_info = GxCore_Malloc(sizeof(GxSubsystemSiParseTtxInfo)*loop2->ttx[ttx_count-1].txt_info_count);
                    if(loop2->ttx[ttx_count-1].ttx_info == NULL)
                    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                        SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                        goto err;
                    }
                    ret = si_parse_lib_pmt_parse_ttx(loop2->ttx[ttx_count-1].ttx_info,
                            &p[2],
                            loop2->ttx[ttx_count-1].descriptor_length);
                    if(ret != 0)
                    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                        SI_PARSE_LIB_ERR_PRINTF("parse ttx des err!!");
#endif
                        goto err;
                    }
                }
                //跳过头
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }

                //跳过该tag剩余字节
                p = si_parse_lib_poffset(p,section_end,loop2->ttx[ttx_count-1].descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,loop2->ttx[ttx_count-1].descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }

                break;

            case SI_PARSE_LIB_SUBTITLING_DES:
                subtitling_count++;
                loop2->subtitling = GxCore_Realloc(loop2->subtitling,sizeof(GxSubsystemSiParseSubtDes)*subtitling_count);
                memset(((void*)loop2->subtitling)+sizeof(GxSubsystemSiParseSubtDes)*(subtitling_count-1),0,sizeof(GxSubsystemSiParseSubtDes));
                if(loop2->subtitling == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                    goto err;
                }
                loop2->subtitling[subtitling_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->subtitling[subtitling_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->subtitling[subtitling_count-1].subt_info_count = loop2->subtitling[subtitling_count-1].descriptor_length/8;
                loop2->subtitling[subtitling_count-1].subt_info = NULL;
                if(loop2->subtitling[subtitling_count-1].subt_info_count != 0)
                {
                    loop2->subtitling[subtitling_count-1].subt_info =
                        GxCore_Malloc(sizeof(GxSubsystemSiParseSubtInfo)*loop2->subtitling[subtitling_count-1].subt_info_count);
                    if(loop2->subtitling[subtitling_count-1].subt_info == NULL)
                    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                        SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                        goto err;
                    }
                    if(loop2->subtitling[subtitling_count-1].descriptor_length % 8 != 0)
                    {
                        ret = -1;
                    }
                    else
                    {
                        ret = si_parse_lib_pmt_parse_subt(loop2->subtitling[subtitling_count-1].subt_info,
                                &p[2],
                                loop2->subtitling[subtitling_count-1].descriptor_length);

                    }

                    if(ret != 0)
                    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                        SI_PARSE_LIB_ERR_PRINTF("parse subtitling des err!!");
#endif
                        goto err;
                    }
                }
                //跳过头
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }

                //跳过该tag剩余字节
                p = si_parse_lib_poffset(p,section_end,loop2->subtitling[subtitling_count-1].descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,loop2->subtitling[subtitling_count-1].descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }

                break;

			case SI_PARSE_LIB_ARIBCCING_DES:
				aribcc_count++;
				loop2->aribccing = GxCore_Realloc(loop2->aribccing,sizeof(GxCcsystemSiParseCcDes)*aribcc_count);
				memset(((void*)loop2->aribccing)+sizeof(GxCcsystemSiParseCcDes)*(aribcc_count-1),0,sizeof(GxCcsystemSiParseCcDes));
				if(loop2->aribccing == NULL)
				{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
					SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
					goto err;
				}
				loop2->aribccing[aribcc_count-1].descriptor_tag = p[0];
				loop2->aribccing[aribcc_count-1].descriptor_length = p[1];
				loop2->aribccing[aribcc_count-1].data_component_id = ((p[2]<<8)|p[3]);
				loop2->aribccing[aribcc_count-1].cc_info_count = loop2->aribccing[aribcc_count-1].descriptor_length/1;
				loop2->aribccing[aribcc_count-1].cc_info = NULL;
				if(loop2->aribccing[aribcc_count-1].cc_info_count != 0)
				{
					loop2->aribccing[aribcc_count-1].cc_info =
						GxCore_Malloc(sizeof(GxCcsystemSiParseCcInfo)*loop2->aribccing[aribcc_count-1].cc_info_count);
					if(loop2->aribccing[aribcc_count-1].cc_info == NULL)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
						goto err;
					}
					if(loop2->aribccing[aribcc_count-1].descriptor_length % 1 != 0)
					{
						ret = -1;
					}
					else
					{
						ret = si_parse_lib_pmt_parse_cc(loop2->aribccing[aribcc_count-1].cc_info,
								&p[4],
								loop2->aribccing[aribcc_count-1].descriptor_length);

					}

					if(ret != 0)
					{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
						SI_PARSE_LIB_ERR_PRINTF("parse subtitling des err!!");
#endif
						goto err;
					}
				}
				//跳过头
				p = si_parse_lib_poffset(p,section_end,4);
				if(NULL == p)
				{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
					SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
					goto err;
				}
				size = si_parse_lib_sizeoffset(size,4);
				if(size < 0)
				{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
					SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
					goto err;
				}

				//跳过该tag剩余字节
				p = si_parse_lib_poffset(p,section_end,loop2->aribccing[aribcc_count-1].descriptor_length);
				if(NULL == p)
				{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
					SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
					goto err;
				}
				size = si_parse_lib_sizeoffset(size,loop2->aribccing[aribcc_count-1].descriptor_length);
				if(size < 0)
				{
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
					SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
					goto err;
				}

				break;

            case SI_PARSE_LIB_AC3_DES:
                ac3_count++;
                loop2->ac3 = GxCore_Realloc(loop2->ac3,sizeof(GxSubsystemSiParseAC3Des)*ac3_count);
                if(loop2->ac3 == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                    goto err;
                }
                loop2->ac3[ac3_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->ac3[ac3_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->ac3[ac3_count-1].AC3_type_flag = TODATA7BIT(p[2]);
                loop2->ac3[ac3_count-1].bsid_flag = TODATA6BIT(p[2]);
                loop2->ac3[ac3_count-1].mainid_flag = TODATA5BIT(p[2]);
                loop2->ac3[ac3_count-1].asvc_flag = TODATA4BIT(p[2]);
                if(loop2->ac3[ac3_count-1].AC3_type_flag ==1 )
                {
                    loop2->ac3[ac3_count-1].AC3_type = TODATA4BIT(p[3]);
                }
                if(loop2->ac3[ac3_count-1].bsid_flag ==1 )
                {
                    loop2->ac3[ac3_count-1].bsid = TODATA4BIT(p[4]);
                }
                if(loop2->ac3[ac3_count-1].mainid_flag ==1 )
                {
                    loop2->ac3[ac3_count-1].mainid = TODATA4BIT(p[5]);
                }
                if(loop2->ac3[ac3_count-1].asvc_flag ==1 )
                {
                    loop2->ac3[ac3_count-1].asvc = TODATA4BIT(p[6]);
                }
                //跳过头
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }

                //跳过该tag剩余字节
                p = si_parse_lib_poffset(p,section_end,loop2->ac3[ac3_count-1].descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,loop2->ac3[ac3_count-1].descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }

                break;
            case SI_PARSE_LIB_EAC3_DES:
                eac3_count++;
                loop2->eac3 = GxCore_Realloc(loop2->ac3,sizeof(GxSubsystemSiParseEAC3Des)*eac3_count);
                if(loop2->eac3 == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                    goto err;
                }
                loop2->eac3[eac3_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->eac3[eac3_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->eac3[eac3_count-1].EAC3_type_flag = TODATA7BIT(p[2]);
                loop2->eac3[eac3_count-1].bsid_flag = TODATA6BIT(p[2]);
                loop2->eac3[eac3_count-1].mainid_flag = TODATA5BIT(p[2]);
                loop2->eac3[eac3_count-1].asvc_flag = TODATA4BIT(p[2]);
                loop2->eac3[eac3_count-1].mixinfoexists = TODATA4BIT(p[2]);
                loop2->eac3[eac3_count-1].substream1_flag = TODATA4BIT(p[2]);
                loop2->eac3[eac3_count-1].substream2_flag = TODATA4BIT(p[2]);
                loop2->eac3[eac3_count-1].substream3_flag = TODATA4BIT(p[2]);
                if(loop2->eac3[eac3_count-1].EAC3_type_flag ==1 )
                {
                    loop2->eac3[eac3_count-1].EAC3_type = TODATA4BIT(p[3]);
                }
                if(loop2->eac3[eac3_count-1].bsid_flag ==1 )
                {
                    loop2->eac3[eac3_count-1].bsid = TODATA4BIT(p[4]);
                }
                if(loop2->eac3[eac3_count-1].mainid_flag ==1 )
                {
                    loop2->eac3[eac3_count-1].mainid = TODATA4BIT(p[5]);
                }
                if(loop2->eac3[eac3_count-1].asvc_flag ==1 )
                {
                    loop2->eac3[eac3_count-1].asvc = TODATA4BIT(p[6]);
                }
                if(loop2->eac3[eac3_count-1].substream1_flag ==1 )
                {
                    loop2->eac3[eac3_count-1].substream1 = TODATA4BIT(p[7]);
                }
                if(loop2->eac3[eac3_count-1].substream2_flag ==1 )
                {
                    loop2->eac3[eac3_count-1].substream2 = TODATA4BIT(p[8]);
                }
                if(loop2->eac3[eac3_count-1].substream3_flag ==1 )
                {
                    loop2->eac3[eac3_count-1].substream3 = TODATA4BIT(p[9]);
                }
                //跳过头
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }

                //跳过该tag剩余字节
                p = si_parse_lib_poffset(p,section_end,loop2->eac3[eac3_count-1].descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,loop2->eac3[eac3_count-1].descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }
                break;

            case SI_PARSE_LIB_ANCILLARY_DATA_DES:
                ancillary_data_count++;
                loop2->ancillary_data = GxCore_Realloc(loop2->ancillary_data,
                        sizeof(GxSubsystemSiParseAncillaryDataDes)*ancillary_data_count);
                if(loop2->ancillary_data == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                    goto err;
                }
                loop2->ancillary_data[ancillary_data_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->ancillary_data[ancillary_data_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->ancillary_data[ancillary_data_count-1].ancillary_data_identifier = TODATA7BIT(p[2]);
                //跳过头
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }

                //跳过该tag剩余字节
                p = si_parse_lib_poffset(p,section_end,loop2->ancillary_data[ancillary_data_count-1].descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,loop2->ancillary_data[ancillary_data_count-1].descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }
                break;

            case SI_PARSE_LIB_CA_DES:
                ca_count++;
                loop2->ca = GxCore_Realloc(loop2->ca,sizeof(GxSubsystemSiParseCaDes)*ca_count);
                if(loop2->ca == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                    goto err;
                }
                loop2->ca[ca_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->ca[ca_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->ca[ca_count-1].ca_system_id = TODATA0_15BIT(p[2],p[3]);
                loop2->ca[ca_count-1].ca_pid = TODATA0_12BIT(p[4],p[5]);
                loop2->ca[ca_count-1].private_data_length = loop2->ca[ca_count-1].descriptor_length - 4;
                loop2->ca[ca_count-1].private_data = NULL;
                //跳过头
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }

                //跳过该tag剩余字节
                p = si_parse_lib_poffset(p,section_end,loop2->ca[ca_count-1].descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,loop2->ca[ca_count-1].descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }
                break;

            case SI_PARSE_LIB_ISO639_LANGUAGE_DES:
                iso_639_count++;
                loop2->iso_639 = GxCore_Realloc(loop2->iso_639,
                        sizeof(GxSubsystemSiParseIso639LanguageDes)*iso_639_count);
                if(loop2->iso_639 == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                    goto err;
                }
                loop2->iso_639[iso_639_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->iso_639[iso_639_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->iso_639[iso_639_count-1].language_count = (loop2->iso_639[iso_639_count-1].descriptor_length-1)/3;
                loop2->iso_639[iso_639_count-1].language = NULL;
                if(loop2->iso_639[iso_639_count-1].language_count != 0)
                {
                    loop2->iso_639[iso_639_count-1].language = GxCore_Malloc(3*loop2->iso_639[iso_639_count-1].language_count);
                    if(loop2->iso_639[iso_639_count-1].language == NULL)
                    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                        SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                        goto err;
                    }
                    memcpy(loop2->iso_639[iso_639_count-1].language,&p[2],3*loop2->iso_639[iso_639_count-1].language_count);
                }
                //跳过头
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }
                p = si_parse_lib_poffset(p,section_end,3*loop2->iso_639[iso_639_count-1].language_count);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                loop2->iso_639[iso_639_count-1].audio_type = p[0];
                p = si_parse_lib_poffset(p,section_end,1);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }

                //跳过该tag剩余字节

                size = si_parse_lib_sizeoffset(size,loop2->iso_639[iso_639_count-1].descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }
                break;

            case SI_PARSE_LIB_DATA_BROADCAST_ID_DES:
                data_broadcast_id_count++;
                loop2->data_broadcast_id = GxCore_Realloc(loop2->data_broadcast_id,
                        sizeof(GxSubsystemSiParseDataBroadcaseIdDes)*data_broadcast_id_count);
                if(loop2->data_broadcast_id == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                    goto err;
                }
                loop2->data_broadcast_id[data_broadcast_id_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->data_broadcast_id[data_broadcast_id_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->data_broadcast_id[data_broadcast_id_count-1].data_broadcast_id = TODATA0_15BIT(p[2],p[3]);
                loop2->data_broadcast_id[data_broadcast_id_count-1].id_selector_byte_count = (loop2->data_broadcast_id[data_broadcast_id_count-1].descriptor_length -2)/8;
                loop2->data_broadcast_id[data_broadcast_id_count-1].id_selector_byte = NULL;
                if(loop2->data_broadcast_id[data_broadcast_id_count-1].id_selector_byte_count != 0)
                {
                    loop2->data_broadcast_id[data_broadcast_id_count-1].id_selector_byte = GxCore_Malloc(8*loop2->data_broadcast_id[data_broadcast_id_count-1].id_selector_byte_count);
                    if(loop2->data_broadcast_id[data_broadcast_id_count-1].id_selector_byte == NULL)
                    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                        SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                        goto err;
                    }
                    memcpy(loop2->data_broadcast_id[data_broadcast_id_count-1].id_selector_byte,&p[4],8*loop2->data_broadcast_id[data_broadcast_id_count-1].id_selector_byte_count);
                }
                //跳过头
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }

                //跳过该tag剩余字节
                p = si_parse_lib_poffset(p,section_end,loop2->data_broadcast_id[data_broadcast_id_count-1].descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,loop2->data_broadcast_id[data_broadcast_id_count-1].descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }
                break;

            case SI_PARSE_LIB_APPLICATION_SIGNALLING_DES:
                application_signalling_count++;
                loop2->application_signalling = GxCore_Realloc(loop2->application_signalling,
                        sizeof(GxSubsystemSiParseApplicationSignallingDes)*application_signalling_count);
                if(loop2->application_signalling == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                    goto err;
                }
                loop2->application_signalling[application_signalling_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->application_signalling[application_signalling_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->application_signalling[application_signalling_count-1].ait_info_count = (loop2->application_signalling[application_signalling_count-1].descriptor_length)/24;
                loop2->application_signalling[application_signalling_count-1].ait_info = NULL;
                //跳过头
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }

                if(loop2->application_signalling[application_signalling_count-1].ait_info_count != 0)
                {
                    loop2->application_signalling[application_signalling_count-1].ait_info = GxCore_Malloc(sizeof(GxSubsystemSiParseAitInfo)*loop2->application_signalling[application_signalling_count-1].ait_info_count);
                    if(loop2->application_signalling[application_signalling_count-1].ait_info == NULL)
                    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                        SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
                        goto err;
                    }
                    for(i=0; i<loop2->application_signalling[application_signalling_count-1].ait_info_count; i++)
                    {
                        loop2->application_signalling[application_signalling_count-1].ait_info->application_type = TODATA0_15BIT(p[0],p[1]);
                        loop2->application_signalling[application_signalling_count-1].ait_info->AIT_version_number = TODATA0_4BIT(p[2]);
                        p = si_parse_lib_poffset(p,section_end,3);
                        if(NULL == p)
                        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                            SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                            goto err;
                        }
                        size = si_parse_lib_sizeoffset(size,3);
                        if(size < 0)
                        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                            SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                            goto err;
                        }
                    }
                }
                else
                {
                    p = si_parse_lib_poffset(p,section_end,
                            loop2->application_signalling[application_signalling_count-1].descriptor_length);
                    if(NULL == p)
                    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                        SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                        goto err;
                    }
                    size = si_parse_lib_sizeoffset(size,
                            loop2->application_signalling[application_signalling_count-1].descriptor_length);
                    if(size < 0)
                    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                        SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                        goto err;
                    }
                }
                break;

            default:
                descriptor_length = TODATA0_7BIT(p[1]);
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }
                p = si_parse_lib_poffset(p,section_end,descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des offset err\n");
#endif
                    goto err;
                }
                size = si_parse_lib_sizeoffset(size,descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("si_parse_lib_pmt_parse_loop2_des size offset err\n");
#endif
                    goto err;
                }
                break;
        }
    }
    loop2->vbi_ttx_count = vbi_ttx_count;
    loop2->vbi_ttx_data_count = vbi_ttx_data_count;
    loop2->stream_identifier_count = stream_identifier_count;
    loop2->ttx_count = ttx_count;
    loop2->subtitling_count = subtitling_count;
    loop2->ac3_count = ac3_count;
    loop2->ancillary_data_count = ancillary_data_count;
    loop2->ca_count = ca_count;
    loop2->iso_639_count = iso_639_count;
    loop2->data_broadcast_id_count = data_broadcast_id_count;
    loop2->application_signalling_count = application_signalling_count;
    return size;

err:
    loop2->vbi_ttx_count = vbi_ttx_count;
    loop2->vbi_ttx_data_count = vbi_ttx_data_count;
    loop2->stream_identifier_count = stream_identifier_count;
    loop2->ttx_count = ttx_count;
    loop2->subtitling_count = subtitling_count;
    loop2->ac3_count = ac3_count;
    loop2->ancillary_data_count = ancillary_data_count;
    loop2->ca_count = ca_count;
    loop2->iso_639_count = iso_639_count;
    loop2->data_broadcast_id_count = data_broadcast_id_count;
    loop2->application_signalling_count = application_signalling_count;
    return -1;
}

static int32_t si_parse_lib_pmt_parse_loop2(GxSubsystemSiParsePMT* pmt,uint8_t* src,uint8_t * section_end,uint32_t len)
{
    int32_t size = len;
    uint32_t loop2_count = 0;
    uint8_t* p = src;
    int32_t ret = 0;

    while(size > 0)
    {
        /*解析loop2头部*/
        loop2_count++;
        pmt->loop2 = GxCore_Realloc(pmt->loop2,sizeof(GxSubsystemSiParsePmtLoop2)*loop2_count);
		memset(((void*)pmt->loop2)+sizeof(GxSubsystemSiParsePmtLoop2)*(loop2_count-1),0,sizeof(GxSubsystemSiParsePmtLoop2));
        memset(&pmt->loop2[loop2_count - 1],0,sizeof(GxSubsystemSiParsePmtLoop2));
        if(pmt->loop2 == NULL)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
            goto err;
        }
        pmt->loop2[loop2_count-1].stream_type = TODATA0_7BIT(p[0]);
        pmt->loop2[loop2_count-1].elementary_pid = TODATA0_12BIT(p[1],p[2]);
        pmt->loop2[loop2_count-1].es_info_length = TODATA0_11BIT(p[3],p[4]);
        //跳过头部
        p = si_parse_lib_poffset(p,section_end,5);
        if(NULL == p)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("pmt loop2 offset err\n");
#endif
            goto err;
        }
        size = si_parse_lib_sizeoffset(size,5);
        if(size < 0)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("pmt loop2 size offset err\n");
#endif
            goto err;
        }
        ret = si_parse_lib_pmt_parse_loop2_des(&pmt->loop2[loop2_count-1],p,section_end,pmt->loop2[loop2_count-1].es_info_length);
        if(ret < 0 )
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("parse loop2 des err!!");
#endif
            goto err;
        }
        p += pmt->loop2[loop2_count-1].es_info_length;//跳过该tag
        size -= pmt->loop2[loop2_count-1].es_info_length;
    }
    pmt->loop2_count = loop2_count;
    return size;
err:
    pmt->loop2_count = loop2_count;
    return -1;
}

/**
 *  @brief      把一段section数据解析成pmt表的格式
 *
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *
 *  @return      pmt数据
 *               NULL：发生错误
 */
GxSubsystemSiParsePMT* GxSubsystm_SiParseLibPmtParse(uint8_t* section,uint32_t size)
{
    GxSubsystemSiParsePMT* pmt = NULL;
    int32_t len = (int32_t)size;
    int32_t i = 0;
    uint8_t* p = section;
    uint8_t * section_end;
    int32_t ret = 0;

    if(section == NULL || size < 12)//pmt表的头部有12字节
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("section is NULL or size is err!!");
#endif
        goto err;
    }
    pmt = GxCore_Malloc(sizeof(GxSubsystemSiParsePMT));
    if(pmt == NULL)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse pmt no memery!!");
#endif
        goto err;
    }
    memset(pmt,0,sizeof(GxSubsystemSiParsePMT));
    pmt->table_id = TODATA0_7BIT(p[0]);
    pmt->section_syntax_indicator = TODATA7BIT(p[1]);
    pmt->section_length = TODATA0_11BIT(p[1],p[2]);
    pmt->program_number = TODATA0_15BIT(p[3],p[4]);
    pmt->version_number =  TODATA1_5BIT(p[5]);
    pmt->current_next_indicator = TODATA0BIT(p[5]);
    pmt->section_number = TODATA0_7BIT(p[6]);
    pmt->last_section_number = TODATA0_7BIT(p[7]);
    pmt->PCR_PID = TODATA0_12BIT(p[8],p[9]);
    pmt->program_info_length = TODATA0_11BIT(p[10],p[11]);

    section_end = p + 3 +pmt->section_length;
    //跳过头部
    p = si_parse_lib_poffset(p,section_end,12);
    if(NULL == p)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse pmt offset err\n");
#endif
        goto err;
    }
    //头部解析完成
    len = si_parse_lib_sizeoffset(len,12);
    if(len < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse pmt head size err\n");
#endif
        goto err;
    }

    //去掉crc
    len = si_parse_lib_sizeoffset(len,4);
    if(len < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse pmt head size err\n");
#endif
        goto err;
    }

    if(len != pmt->section_length-9-4)//解析出来的大小和表里面的大小不一致
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the section size is err!!");
#endif
        goto err;
    }
    ret = si_parse_lib_pmt_parse_loop1(pmt,p,pmt->program_info_length);
    if(ret < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse loop1 err!!");
#endif
        goto err;
    }
    //跳过第一个循环
    p = si_parse_lib_poffset(p,section_end,pmt->program_info_length);
    if(NULL == p)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse pmt offset err\n");
#endif
        goto err;
    }
    len = si_parse_lib_sizeoffset(len,pmt->program_info_length);
    if(len < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse pmt head size err\n");
#endif
        goto err;
    }

    ret = si_parse_lib_pmt_parse_loop2(pmt,p,section_end,len);
    if(ret < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse loop2 err!!");
#endif
        goto err;
    }

    for(i = 0;i<pmt->loop2_count;i++)
    {
        p = si_parse_lib_poffset(p,section_end,5+pmt->loop2[i].es_info_length);
        if(NULL == p)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("parse pmt offset err\n");
#endif
            goto err;
        }

        len = si_parse_lib_sizeoffset(len,5+pmt->loop2[i].es_info_length);
        if(len < 0)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("parse pmt head size err\n");
#endif
            goto err;
        }
    }

    si_parse_lib_pmt_add_protect((void*)pmt);
    return pmt;
err:
    si_parse_lib_pmt_release(pmt);
    return NULL;
}

/**
 *  @brief      释放pmt的空间，应用应该在通过GxSubsystm_SiParseLibPmtParse获取
 *  解析好的pmt数据之后某个时刻释放pmt的空间。
 *
 *  @param       GxSubsystemSiParsePMT* pmt:pmt的存储空间，是通过GxSubsystm_SiParseLibPmtParse获取的
 *
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibPmtRelease(void* pmt)
{
    uint32_t ret = 0;
    ret = si_parse_lib_pmt_remove_protect(pmt);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParsePMT* pmt is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_pmt_release((GxSubsystemSiParsePMT*)pmt);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

