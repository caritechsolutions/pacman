/*****************************************************************************
* 						   CONFIDENTIAL								
*        Hangzhou GuoXin Science and Technology Co., Ltd.             
*                      (C)2009, All right reserved
******************************************************************************

******************************************************************************
* File Name :	si_parse_lib_nit.c
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
static void si_parse_lib_nit_release(GxSubsystemSiParseNIT* nit);
static void si_parse_lib_nit_add_protect(void* p);
static uint32_t si_parse_lib_nit_remove_protect(void* p);
static int32_t si_parse_lib_nit_parse_loop1(GxSubsystemSiParseNIT* nit,uint8_t* src,uint32_t len); 
static int32_t si_parse_lib_nit_parse_loop2(GxSubsystemSiParseNIT* nit,uint8_t* src,uint8_t*section_end,uint32_t len); 
static int32_t si_parse_lib_nit_parse_loop2_des(GxSubsystemSiParseNitLoop2* loop2,uint8_t* src,uint8_t*section_end,uint32_t len);
static void si_parse_lib_nit_release_loop2(GxSubsystemSiParseNitLoop2* loop2);

static void si_parse_lib_nit_release_loop2(GxSubsystemSiParseNitLoop2* loop2)
{
    uint32_t i = 0;
    if(loop2)
    {
        if(loop2->service_list)
        {
            for(i=0; i<loop2->service_list_count; i++)
            {
                if(loop2->service_list[i].service_list)
                {
                    GxCore_Free(loop2->service_list[i].service_list);
                }
            }
            GxCore_Free(loop2->service_list);
        }
        if(loop2->satellite_delivery_system)
        {
            GxCore_Free(loop2->satellite_delivery_system);
        }
        if(loop2->cable_delivery_system)
        {
            GxCore_Free(loop2->cable_delivery_system);
        }
        if(loop2->dtmb_delivery_system)
        {
            GxCore_Free(loop2->dtmb_delivery_system);
        }
    }
    return;
}
/*释放nit的空间*/
static void si_parse_lib_nit_release(GxSubsystemSiParseNIT* nit)
{
    uint32_t i = 0;
    
    if(nit != NULL)
    {
        if(nit->loop1.network_name != NULL)
        {
            for(i=0; i<nit->loop1.network_name_count; i++)
            {
                if(nit->loop1.network_name[i].name != NULL)
                {
                    GxCore_Free(nit->loop1.network_name[i].name);
                }
            }
            GxCore_Free(nit->loop1.network_name);
        }
        if(nit->loop2)
        {
            for(i=0; i<nit->loop2_count; i++)
            {
                si_parse_lib_nit_release_loop2(&(nit->loop2[i]));
            }
            GxCore_Free(nit->loop2);
        }
        GxCore_Free(nit);
    }
    return;
}
static void si_parse_lib_nit_add_protect(void* p)
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
static uint32_t si_parse_lib_nit_remove_protect(void* p)
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
static int32_t si_parse_lib_nit_parse_loop1(GxSubsystemSiParseNIT* nit,uint8_t* src,uint32_t len) 
{
    int32_t size = len;
    uint32_t network_name_count = 0;
    uint8_t* p = src;
    uint8_t* section_end = p + nit->section_length + 3 - 10; 
    uint32_t descriptor_length = 0;

    while(size > 0)
    {
        switch(p[0])
        {
            case SI_PARSE_LIB_NETWORK_NAME_DES:
                network_name_count++;
                nit->loop1.network_name = GxCore_Realloc(nit->loop1.network_name,
                        sizeof(GxSubsystemSiParseNitDes)*network_name_count);
                if(nit->loop1.network_name == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse nit no memery!!");
#endif  
                    goto err;
                }      
                nit->loop1.network_name[network_name_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                nit->loop1.network_name[network_name_count-1].name_length = TODATA0_7BIT(p[1]);
                nit->loop1.network_name[network_name_count-1].name = NULL;
                if(nit->loop1.network_name[network_name_count-1].name_length != 0)
                {
                    nit->loop1.network_name[network_name_count-1].name = GxCore_Malloc(nit->loop1.network_name[network_name_count-1].name_length);
                    if(nit->loop1.network_name[network_name_count-1].name == NULL)
                    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                        SI_PARSE_LIB_ERR_PRINTF("parse nit no memery!!");
#endif  
                        goto err;
                    }
                    memcpy(nit->loop1.network_name[network_name_count-1].name,
                            &p[2],
                            nit->loop1.network_name[network_name_count-1].name_length);
                }
                //跳过头
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size ,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }
                //跳过该tag剩余字节
                p = si_parse_lib_poffset(p,section_end,nit->loop1.network_name[network_name_count-1].name_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size ,nit->loop1.network_name[network_name_count-1].name_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
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
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size ,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }

                //跳过该tag剩余字节
                p = si_parse_lib_poffset(p,section_end,descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size ,descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }
                break;
        }
    }
    nit->loop1.network_name_count = network_name_count;
    return size;

err:
    nit->loop1.network_name_count = network_name_count;
    return -1;
}
#if 0
static int32_t si_parse_lib_nit_parse_ttx(GxSubsystemSiParseTtxInfo* ttx_info,uint8_t* src,uint32_t len)
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

static int32_t si_parse_lib_nit_parse_vbi_ttx_data(GxSubsystemSiParseVbiDataDes* ttx_data_des,uint8_t* src,uint32_t len)
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
                    SI_PARSE_LIB_ERR_PRINTF("parse nit no memery!!");
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
            ttx_data_des->ttx_data[count-1].count = ttx_data_des->ttx_data[count-1].descriptor_length;
            ttx_data_des->ttx_data[count-1].info = GxCore_Malloc(sizeof(GxSubsystemSiParseTtxDataInfo)*ttx_data_des->ttx_data[count-1].count);
            if(ttx_data_des->ttx_data[count-1].info == NULL)
            {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse nit no memery!!");
#endif  
                    goto err;
            }
            for(i=0; i<ttx_data_des->ttx_data[count-1].count; i++)
            {
                ttx_data_des->ttx_data[count-1].info[i].field_parity = TODATA5BIT(p[i]);
            }
        }
        p += ttx_data_des->ttx_data[count-1].descriptor_length;//跳到下一个外循环
        size -= 2;
        size -= ttx_data_des->ttx_data[count-1].descriptor_length;
    }
    ttx_data_des->ttx_data_count = count;
    return count;
err:
    return -1;
}

#endif
static int32_t si_parse_lib_nit_parse_service_list(GxSubsystemSiParseServiceListInfo* service_list,
        uint8_t* src,uint8_t * section_end,
        uint32_t len)
{
    int32_t size = len;
    uint8_t* p = src;
    uint32_t i = 0;

    while(size > 0)
    {
        service_list[i].service_id = TODATA0_15BIT(p[0],p[1]);
        service_list[i].service_type = TODATA0_7BIT(p[2]);
        p = si_parse_lib_poffset(p,section_end,3);
        if(NULL == p)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
            goto err;
        }

        size = si_parse_lib_sizeoffset(size ,3);
        if(size < 0)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
            goto err;
        }
        i++;
    }
    return size;
err:
    return -1;
}
static int32_t si_parse_lib_nit_parse_loop2_des(GxSubsystemSiParseNitLoop2* loop2,uint8_t* src,uint8_t*section_end,uint32_t len)
{
    int32_t size = len;
    uint32_t service_list_count = 0;
    uint32_t satellite_delivery_system_count = 0;
    uint32_t cable_delivery_system_count = 0;
    uint32_t dtmb_delivery_system_count = 0;
    uint8_t* p = src;
    uint32_t descriptor_length = 0;
    int32_t ret = 0;

    while(size > 0)
    {
        switch(p[0])
        {
            case SI_PARSE_LIB_SERVICE_LIST_DES:
                service_list_count++;
                loop2->service_list = GxCore_Realloc(loop2->service_list,
                        sizeof(GxSubsystemSiParseServiceListDes)*service_list_count);
                if(loop2->service_list == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse nit no memery!!");
#endif  
                    goto err;
                }      
                loop2->service_list[service_list_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->service_list[service_list_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->service_list[service_list_count-1].descriptor_count = 
                    loop2->service_list[service_list_count-1].descriptor_length/3;
                loop2->service_list[service_list_count-1].service_list = NULL;
                if(loop2->service_list[service_list_count-1].descriptor_count != 0)
                {
                    loop2->service_list[service_list_count-1].service_list = 
                        GxCore_Malloc(sizeof(GxSubsystemSiParseServiceListInfo)*
                                loop2->service_list[service_list_count-1].descriptor_count);
                    if(loop2->service_list[service_list_count-1].service_list == NULL)
                    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                        SI_PARSE_LIB_ERR_PRINTF("parse nit no memery!!");
#endif  
                        goto err;
                    }
                    ret = si_parse_lib_nit_parse_service_list(loop2->service_list[service_list_count-1].service_list,
                            &p[2],section_end,
                            loop2->service_list[service_list_count-1].descriptor_length);
                    if(ret != 0)
                    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                        SI_PARSE_LIB_ERR_PRINTF("parse service list des err!!");
#endif  
                        goto err;
                    }
                }
                //跳过头
                p = si_parse_lib_poffset(p,section_end,2);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size ,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }
                //跳过该tag剩余字节
                p = si_parse_lib_poffset(p,section_end,loop2->service_list[service_list_count-1].descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size ,loop2->service_list[service_list_count-1].descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }
                break;

            case SI_PARSE_LIB_SAT_DELIVERY_SYSTEM_DES:
                satellite_delivery_system_count++;
                loop2->satellite_delivery_system = GxCore_Realloc(loop2->satellite_delivery_system,
                        sizeof(GxSubsystemSiParseSatelliteDeliveryDes)*satellite_delivery_system_count);
                if(loop2->satellite_delivery_system == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse nit no memery!!");
#endif  
                    goto err;
                }      
                loop2->satellite_delivery_system[satellite_delivery_system_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->satellite_delivery_system[satellite_delivery_system_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->satellite_delivery_system[satellite_delivery_system_count-1].frequency = TODATA0_31BIT(p[2],p[3],p[4],p[5]);
                loop2->satellite_delivery_system[satellite_delivery_system_count-1].orbital_position = TODATA0_15BIT(p[6],p[7]);
                loop2->satellite_delivery_system[satellite_delivery_system_count-1].west_east_flag = TODATA7BIT(p[8]);
                loop2->satellite_delivery_system[satellite_delivery_system_count-1].polarization = TODATA5_6BIT(p[8]);
                loop2->satellite_delivery_system[satellite_delivery_system_count-1].modulation = TODATA0_4BIT(p[8]);
                loop2->satellite_delivery_system[satellite_delivery_system_count-1].symbol_rate = TODATA4_31BIT(p[9],p[10],p[11],p[12]);
                loop2->satellite_delivery_system[satellite_delivery_system_count-1].FEC_inner = TODATA0_3BIT(p[12]);

                p = si_parse_lib_poffset(p,section_end,13);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size ,13);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }

                break;

            case SI_PARSE_LIB_CAB_DELIVERY_SYSTEM_DES:
                cable_delivery_system_count++;
                loop2->cable_delivery_system = GxCore_Realloc(loop2->cable_delivery_system,
                        sizeof(GxSubsystemSiParseCableDeliveryDes)*cable_delivery_system_count);
                if(loop2->cable_delivery_system == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse nit no memery!!");
#endif  
                    goto err;
                }      
                loop2->cable_delivery_system[cable_delivery_system_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->cable_delivery_system[cable_delivery_system_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->cable_delivery_system[cable_delivery_system_count-1].frequency = TODATA0_31BIT(p[2],p[3],p[4],p[5]);
                loop2->cable_delivery_system[cable_delivery_system_count-1].reserved_future_use = TODATA4_15BIT(p[6],p[7]);
                loop2->cable_delivery_system[cable_delivery_system_count-1].FEC_outer = TODATA0_3BIT(p[7]);
                loop2->cable_delivery_system[cable_delivery_system_count-1].modulation = TODATA0_7BIT(p[8]);
                loop2->cable_delivery_system[cable_delivery_system_count-1].symbol_rate = TODATA4_31BIT(p[9],p[10],p[11],p[12]);
                loop2->cable_delivery_system[cable_delivery_system_count-1].FEC_inner = TODATA0_3BIT(p[12]);


                p = si_parse_lib_poffset(p,section_end,13);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size ,13);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }
                break;

            case SI_PARSE_LIB_TER_DELIVERY_SYSTEM_DES:
                dtmb_delivery_system_count ++;
                loop2->dtmb_delivery_system = GxCore_Realloc(loop2->dtmb_delivery_system,
                        sizeof(GxSubsystemSiParseDtmbDeliveryDes)*dtmb_delivery_system_count);
                if(loop2->dtmb_delivery_system == NULL)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("parse nit no memery!!");
#endif  
                    goto err;
                }      

                loop2->dtmb_delivery_system[dtmb_delivery_system_count-1].descriptor_tag = TODATA0_7BIT(p[0]);
                loop2->dtmb_delivery_system[dtmb_delivery_system_count-1].descriptor_length = TODATA0_7BIT(p[1]);
                loop2->dtmb_delivery_system[dtmb_delivery_system_count-1].frequency = TODATA0_31BIT(p[2],p[3],p[4],p[5])/100;
                // change to KHz
                //dtmb_info->centr_freq = GET_DEC32(dtmb_info->centr_freq)/10;

                loop2->dtmb_delivery_system[dtmb_delivery_system_count-1].FEC = TODATA4_7BIT(p[6]);
                loop2->dtmb_delivery_system[dtmb_delivery_system_count-1].modulation = TODATA0_3BIT(p[6]);
                loop2->dtmb_delivery_system[dtmb_delivery_system_count-1].number_of_subcarrier = TODATA4_7BIT(p[7]);
                loop2->dtmb_delivery_system[dtmb_delivery_system_count-1].frame_header_mode = TODATA0_3BIT(p[7]);
                loop2->dtmb_delivery_system[dtmb_delivery_system_count-1].interleaving_mode = TODATA4_7BIT(p[8]);
                loop2->dtmb_delivery_system[dtmb_delivery_system_count-1].other_frequency_flag = TODATA3BIT(p[8]);
                loop2->dtmb_delivery_system[dtmb_delivery_system_count-1].sfn_mfn_flag = TODATA2BIT(p[8]);

	
                p = si_parse_lib_poffset(p,section_end,9);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size ,9);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
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
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size ,2);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }

                //跳过该tag剩余字节

                p = si_parse_lib_poffset(p,section_end,descriptor_length);
                if(NULL == p)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }

                size = si_parse_lib_sizeoffset(size ,descriptor_length);
                if(size < 0)
                {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
                    SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
                    goto err;
                }
                break;
        }
    }
    loop2->service_list_count = service_list_count;
    loop2->satellite_delivery_system_count = satellite_delivery_system_count;
    loop2->cable_delivery_system_count = cable_delivery_system_count;
    loop2->dtmb_delivery_system_count = dtmb_delivery_system_count;
    return size;

err:
    loop2->service_list_count = service_list_count;
    loop2->satellite_delivery_system_count = satellite_delivery_system_count;
    loop2->cable_delivery_system_count = cable_delivery_system_count;
    loop2->dtmb_delivery_system_count = dtmb_delivery_system_count;
    
    return -1;
}
static int32_t si_parse_lib_nit_parse_loop2(GxSubsystemSiParseNIT* nit,uint8_t* src,uint8_t* section_end,uint32_t len) 
{
    int32_t size = len;
    uint32_t loop2_count = 0;
    uint8_t* p = src;
    int32_t ret = 0;

    while(size > 0)
    {
        /*解析loop2头部*/
        loop2_count++;
        nit->loop2 = GxCore_Realloc(nit->loop2,sizeof(GxSubsystemSiParseNitLoop2)*loop2_count);
        memset(((void*)nit->loop2)+sizeof(GxSubsystemSiParseNitLoop2)*(loop2_count-1),0,sizeof(GxSubsystemSiParseNitLoop2));
        if(nit->loop2 == NULL)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("parse nit no memery!!");
#endif              
            goto err;
        }
        nit->loop2[loop2_count-1].transport_stream_id = TODATA0_15BIT(p[0],p[1]);
        nit->loop2[loop2_count-1].original_network_id = TODATA0_15BIT(p[2],p[3]);
        nit->loop2[loop2_count-1].reserved_future_use = TODATA4_7BIT(p[4]);
        nit->loop2[loop2_count-1].transport_descriptors_length = TODATA0_11BIT(p[4],p[5]);
        //跳过头部
        p = si_parse_lib_poffset(p,section_end,6);
        if(NULL == p)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
            goto err;
        }

        size = si_parse_lib_sizeoffset(size ,6);
        if(size < 0)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
            goto err;
        }
        ret = si_parse_lib_nit_parse_loop2_des(&nit->loop2[loop2_count-1],
                p,section_end,
                nit->loop2[loop2_count-1].transport_descriptors_length);
        if(ret < 0 )
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("parse loop2 des err!!\n");
#endif              
            goto err;
        }
        //跳过该tag
        p = si_parse_lib_poffset(p,section_end,nit->loop2[loop2_count-1].transport_descriptors_length);
        if(NULL == p)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
            goto err;
        }

        size = si_parse_lib_sizeoffset(size ,nit->loop2[loop2_count-1].transport_descriptors_length);
        if(size < 0)
        {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
            SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
            goto err;
        }

    }
    nit->loop2_count = loop2_count;
    return size;
err:
    nit->loop2_count = loop2_count;
    return -1;
}

/**
 *  @brief      把一段section数据解析成nit表的格式
 *   
 *  @param       uint8_t* section:section数据
 *                uint32_t size:section的大小
 *  
 *  @return      nit数据
 *               NULL：发生错误
 */
GxSubsystemSiParseNIT* GxSubsystm_SiParseLibDtmbnitParse(uint8_t* section,uint32_t size)
{
    GxSubsystemSiParseNIT* nit = NULL;
    int32_t len = (int32_t)size;
    uint8_t* p = section;
    uint8_t* section_end;
    int32_t ret = 0;

    if(section == NULL || size < 10)//nit表的头部有10字节
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("section is NULL or size is err!!");
#endif
        goto err;
    }
    nit = GxCore_Malloc(sizeof(GxSubsystemSiParseNIT));
    if(nit == NULL)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse nit no memery!!");
#endif
        goto err;
    }
    memset(nit,0,sizeof(GxSubsystemSiParseNIT));
    nit->table_id = TODATA0_7BIT(p[0]);
    nit->section_syntax_indicator = TODATA7BIT(p[1]);
    nit->reserved_future_use1 = TODATA6BIT(p[1]);
    nit->section_length = TODATA0_11BIT(p[1],p[2]);
    nit->network_id = TODATA0_15BIT(p[3],p[4]);
    nit->version_number =  TODATA1_5BIT(p[5]);
    nit->current_next_indicator = TODATA0BIT(p[5]);
    nit->section_number = TODATA0_7BIT(p[6]);
    nit->last_section_number = TODATA0_7BIT(p[7]);
    nit->reserved_future_use2 = TODATA4_7BIT(p[8]);
    nit->network_descriptors_length = TODATA0_11BIT(p[8],p[9]);

    section_end = p + nit->section_length + 3;
    //跳过头部
    p = si_parse_lib_poffset(p,section_end,10);
    if(NULL == p)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
        goto err;
    }

    len = si_parse_lib_sizeoffset(len ,10);
    if(len < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
        goto err;
    }
    //去掉crc
    len = si_parse_lib_sizeoffset(len ,4);
    if(len < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
        goto err;
    }
   

    if(len != nit->section_length-7-4)//解析出来的大小和表里面的大小不一致
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the section size is err!!");
#endif  
        goto err;
    }
    ret = si_parse_lib_nit_parse_loop1(nit,p,nit->network_descriptors_length);
    if(ret < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse loop1 err!!");
#endif
        goto err;
    }
    //跳过第一个循环
    p = si_parse_lib_poffset(p,section_end,nit->network_descriptors_length);
    if(NULL == p)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
        goto err;
    }

    len = si_parse_lib_sizeoffset(len ,nit->network_descriptors_length);
    if(len < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
        goto err;
    }

    nit->reserved_future_use3 = TODATA4_7BIT(p[0]);
    nit->transport_stream_loop_length= TODATA0_11BIT(p[0],p[1]);
    p = si_parse_lib_poffset(p,section_end,2);
    if(NULL == p)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
        goto err;
    }

    len = si_parse_lib_sizeoffset(len ,2);
    if(len < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("nit section size is err\n");
#endif
        goto err;
    }
    ret = si_parse_lib_nit_parse_loop2(nit,p,section_end,len);
    if(ret < 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("parse loop2 err!!");
#endif
        goto err;
    }
    si_parse_lib_nit_add_protect((void*)nit);
    return nit;
err:
    si_parse_lib_nit_release(nit);
    return NULL;
}
/**
 *  @brief      释放nit的空间，应用应该在通过GxSubsystm_SiParseLibNitParse获取
 *  解析好的nit数据之后某个时刻释放nit的空间。
 *   
 *  @param       GxSubsystemSiParseNIT* nit:nit的存储空间，是通过GxSubsystm_SiParseLibNitParse获取的
 *  
 *  @return      GX_SI_PARSE_LIB_PARAMETER_ERR：传入的参数是错误的
 *               GX_SI_PARSE_LIB_ERR：执行错误          
 *               GX_SI_PARSE_LIB_SUCCESS：执行成功
 */
int32_t  GxSubsystm_SiParseLibDtmbnitRelease(void* nit)
{
    uint32_t ret = 0;
    ret = si_parse_lib_nit_remove_protect(nit);
    if(ret == 0)
    {
#ifdef GX_SI_PARSE_LIB_ERR_DBUG
        SI_PARSE_LIB_ERR_PRINTF("the GxSubsystemSiParseNIT* nit is err!!");
#endif
        return GX_SI_PARSE_LIB_PARAMETER_ERR;
    }

    si_parse_lib_nit_release((GxSubsystemSiParseNIT*)nit);
    return GX_SI_PARSE_LIB_SUCCESS;
}
/* End of file -------------------------------------------------------------*/

