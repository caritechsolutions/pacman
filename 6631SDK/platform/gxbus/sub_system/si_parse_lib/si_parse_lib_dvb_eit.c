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
/* Functions --------------------------------------------------------------- */

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

static void si_parse_lib_eit_release(GxSubsystemSiParseDvbEIT * eit)
{
	int32_t i = 0,j = 0 , a = 0;
	GxSubsystemSiParseDvbEITLoop * loop = NULL;

	if(eit)
	{
		if(eit->loop)
		{
			for(a=0 ; a< eit->loop_count ; a++)
			{
				loop = &eit->loop[a];
				if(loop->sd)
				{
					for(i=0 ; i< loop->sd_count ; i++)
					{
						if(loop->sd[i].stuffing_byte)
						{
							GxCore_Free(loop->sd[i].stuffing_byte);
						}
					}
					GxCore_Free(loop->sd);
				}
				if(loop->ld)
				{
					for(i=0 ; i<loop->ld_count ; i++)
					{
						if(loop->ld[i].linkage_type != 0x08)
						{
							GxCore_Free(loop->ld[i].linkage_s.private_data_type);
						}
						else
						{
							if(loop->ld[i].linkage_s.mobileroaming_s.private_data_byte)
							{
								GxCore_Free(loop->ld[i].linkage_s.mobileroaming_s.private_data_byte);
							}
						}
					}
					GxCore_Free(loop->ld);
				}
				if(loop->sed)
				{
					for(i=0 ; i<loop->sed_count ; i++)
					{
						if(loop->sed[i].event_name)
						{
							GxCore_Free(loop->sed[i].event_name);
						}
						if(loop->sed[i].text)
						{
							GxCore_Free(loop->sed[i].text);
						}
					}
					GxCore_Free(loop->sed);
				}
				if(loop->eed)
				{
					for(i=0 ; i<loop->eed_count ; i++)
					{

						for(j=0 ; j<loop->eed[i].item_descriptor_count ; j++)
						{
							if(loop->eed[i].item_descriptor[j].item_descriptor)
							{
								GxCore_Free(loop->eed[i].item_descriptor[j].item_descriptor);
							}
							if(loop->eed[i].item_descriptor[j].item)
							{
								GxCore_Free(loop->eed[i].item_descriptor[j].item);
							}
						}
						if(loop->eed[i].text)
						{
							GxCore_Free(loop->eed[i].text);
						}
						GxCore_Free(loop->eed[i].item_descriptor);
					}
					GxCore_Free(loop->eed);
				}
				if(loop->tsed)
				{
					GxCore_Free(loop->tsed);
				}
				if(loop->componentdes)
				{
					for(i=0 ; i<loop->componentdes_count ; i++)
					{
						if(loop->componentdes[i].text)
						{
							GxCore_Free(loop->componentdes[i].text);
						}
					}
					GxCore_Free(loop->componentdes);
				}
				if(loop->cid)
				{
					for(i=0 ; i<loop->cid_count ; i++)
					{
						if(loop->cid[i].ca_system_id)
						{
							GxCore_Free(loop->cid[i].ca_system_id);
						}
					}
					GxCore_Free(loop->cid);
				}

				if(loop->contentdes)
				{
					for(i=0 ; i<loop->contentdes_count ; i++)
					{
						GxCore_Free(loop->contentdes[i].content);
					}
					GxCore_Free(loop->contentdes);
				}
				if(loop->prd)
				{
					for(i=0 ; i<loop->prd_count ; i++)
					{
						GxCore_Free(loop->prd[i].parental_rating);
					}
					GxCore_Free(loop->prd);
				}
				if(loop->td)
				{
					for(i=0 ; i<loop->td_count ; i++)
					{
						if(loop->td[i].country_prefix_char)
						{
							GxCore_Free(loop->td[i].country_prefix_char);
						}
						if(loop->td[i].international_area_code_char)
						{
							GxCore_Free(loop->td[i].international_area_code_char);
						}
						if(loop->td[i].operator_code_char)
						{
							GxCore_Free(loop->td[i].operator_code_char);
						}
						if(loop->td[i].national_area_code_char)
						{
							GxCore_Free(loop->td[i].national_area_code_char);
						}
						if(loop->td[i].core_number_char)
						{
							GxCore_Free(loop->td[i].core_number_char);
						}
					}
					GxCore_Free(loop->td);
				}
				if(loop->mcd)
				{
					for(i=0 ; i<loop->mcd_count ; i++)
					{
						for(j=0 ; j<loop->mcd[i].component_count;j++)
						{
							if(loop->mcd[i].component[j].text)
							{
								GxCore_Free(loop->mcd[i].component[j].text);
							}
						}
						GxCore_Free(loop->mcd[i].component);
					}
					GxCore_Free(loop->mcd);
				}
				if(loop->pd)
				{
					GxCore_Free(loop->pd);
				}
				if(loop->ssd)
				{
					for(i=0 ; i<loop->ssd_count ; i++)
					{
						if(loop->ssd[i].DVB_reserved)
						{
							GxCore_Free(loop->ssd[i].DVB_reserved);
						}
					}
					GxCore_Free(loop->ssd);
				}
				if(loop->dbd)
				{
					for(i=0 ; i<loop->dbd_count ; i++)
					{
						if(loop->dbd[i].selector_type)
						{
							GxCore_Free(loop->dbd[i].selector_type);
						}
						if(loop->dbd[i].text)
						{
							GxCore_Free(loop->dbd[i].text);
						}
					}
					GxCore_Free(loop->dbd);
				}
				if(loop->pdcd)
				{
					GxCore_Free(loop->pdcd);
				}
			}
			GxCore_Free(eit->loop);
		}
		GxCore_Free(eit);
	}
}

#if 0
static int32_t multilingual_component_descriptor_parse(GxSubsystemSiParseMulitiComponentDes * mcd,uint8_t*section,
		uint32_t len,uint8_t* section_end)
{
	int32_t size = len;
	uint8_t* p = section;
	uint32_t component_count = 0;
	while(size > 0)
	{
		component_count++;
		mcd->component = GxCore_Realloc(mcd->component,component_count * sizeof(GxSubsystemSiParseMulitiComponent));
		if(mcd->component == NULL)
		{
#ifdef ATSC_SI_DEBUG
			SI_PARSE_LIB_ERR_PRINTF("multilingual_component_des component malloc failed\n");
#endif
			goto err;
		}
		else
		{
			memset(&mcd->component[component_count - 1],0,sizeof(GxSubsystemSiParseMulitiComponent));
		}
		memcpy(mcd->component[component_count - 1].language,&p[0],3);
		mcd->component[component_count - 1].text_length = TODATA0_7BIT(p[3]);
		if(mcd->component[component_count - 1].text_length > 0)
		{
			mcd->component[component_count - 1].text = GxCore_Mallocz(mcd->component[component_count - 1].text_length +
					1);
			if(mcd->component[component_count - 1].text == NULL)
			{
#ifdef ATSC_SI_DEBUG
				SI_PARSE_LIB_ERR_PRINTF("component descriptors text malloc failed\n");
#endif
				goto err;
			}
			else
			{
				memcpy(mcd->component[component_count - 1].text,&p[4],mcd->component[component_count - 1].text_length );
			}
		}

		p = si_parse_lib_poffset(p,section_end,4 + mcd->component[component_count - 1].text_length);
		if(NULL == p)
		{
#ifdef ATSC_SI_DEBUG
			SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
			goto err;
		}

		size = si_parse_lib_sizeoffset(size,4 + mcd->component[component_count - 1].text_length);
		if(size < 0)
		{
#ifdef ATSC_SI_DEBUG
			SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
			goto err;
		}
	}
	mcd->component_count =  component_count;
	return size;
err:
	mcd->component_count =  component_count;
	return -1;
}
#endif

static int32_t parental_rating_descriptor_parse(GxSubsystemSiParseParentalRatingDes * prd,uint8_t* section,
		uint32_t len,uint8_t* section_end)
{
	int32_t size = len;
	int i = 0;
	uint8_t* p = section;
	uint32_t parental_rating_count = 0;
	parental_rating_count = len/4;
	prd->parental_rating =
		GxCore_Mallocz(parental_rating_count * sizeof(GxSubsystemSiParseParentalRating));
	if(prd->parental_rating == NULL)
	{
#ifdef ATSC_SI_DEBUG
		SI_PARSE_LIB_ERR_PRINTF("dvb eit parental_rating malloc failed\n");
#endif
		return -1;
	}
	for(i = 0;i<parental_rating_count;i++)
	{
		memcpy(prd->parental_rating[i].country,&p[0],3);
		prd->parental_rating[i].rating = TODATA0_7BIT(p[4]);

		p = si_parse_lib_poffset(p,section_end,4);
		if(NULL == p)
		{
#ifdef ATSC_SI_DEBUG
			SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
			goto err;
		}

		size = si_parse_lib_sizeoffset(size,4);
		if(size < 0)
		{
#ifdef ATSC_SI_DEBUG
			SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
			goto err;
		}
	}
	prd->parental_rating_count = parental_rating_count;
	return size;
err:
	prd->parental_rating_count = parental_rating_count;
	return -1;
}

#if 0
static int32_t content_descriptor_parse(GxSubsystemSiParseContentDes * contentdes,uint8_t* section,
		uint32_t len,uint8_t* section_end)
{
	int32_t size = len;
	uint8_t* p = section;
	uint32_t  content_count = 0;

	while(size > 0)
	{
		content_count++;
		contentdes->content = GxCore_Realloc(contentdes->content,content_count * sizeof(GxSubsystemSiParseContent));
		contentdes->content[content_count - 1].content_nibble_level_1 = TODATA4_7BIT(p[0]);
		contentdes->content[content_count - 1].content_nibble_level_2 = TODATA0_3BIT(p[0]);
		contentdes->content[content_count - 1].user_nibble1 = TODATA4_7BIT(p[1]);
		contentdes->content[content_count - 1].user_nibble2 = TODATA0_3BIT(p[1]);
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
	contentdes->content_count = content_count;
	return 0;
err:
	contentdes->content_count = content_count;
	return -1;
}
#endif

static int32_t item_descriptor_parse(GxSubsystemSiParseExtendedEventDes * eed,uint8_t * section,
		uint32_t len,uint8_t* section_end)
{
	uint32_t size = len;
	uint32_t item_descriptor_count = 0;
	uint8_t* p = section;
	while(size > 0)
	{
		item_descriptor_count ++;
		eed->item_descriptor =
			GxCore_Realloc(eed->item_descriptor,item_descriptor_count * sizeof(GxSubsystemSiParseExtendedEventItemDes));
		if(eed->item_descriptor == NULL)
		{
#ifdef ATSC_SI_DEBUG
			SI_PARSE_LIB_ERR_PRINTF("item_descriptor realloc failed\n");
#endif
			goto err;
		}
		else
		{
			eed->item_descriptor[item_descriptor_count - 1].item_descriptor_length =TODATA0_7BIT(p[0]);
			if(eed->item_descriptor[item_descriptor_count - 1].item_descriptor_length > 0)
			{
				eed->item_descriptor[item_descriptor_count - 1].item_descriptor =
					GxCore_Mallocz(eed->item_descriptor[item_descriptor_count - 1].item_descriptor_length + 1);
				if(eed->item_descriptor[item_descriptor_count - 1].item_descriptor == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("item_descriptor malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memcpy(eed->item_descriptor[item_descriptor_count - 1].item_descriptor,&p[1],
							eed->item_descriptor[item_descriptor_count - 1].item_descriptor_length);
				}
			}


			p = si_parse_lib_poffset(p,section_end,
					1 + eed->item_descriptor[item_descriptor_count - 1].item_descriptor_length);
			if(NULL == p)
			{
#ifdef ATSC_SI_DEBUG
				SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
				goto err;
			}

			size = si_parse_lib_sizeoffset(size,
					1 + eed->item_descriptor[item_descriptor_count - 1].item_descriptor_length);
			if(size < 0)
			{
#ifdef ATSC_SI_DEBUG
				SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
				goto err;
			}
			eed->item_descriptor[item_descriptor_count - 1].itrm_length = TODATA0_7BIT(p[0]);
			if(eed->item_descriptor[item_descriptor_count - 1].itrm_length > 0)
			{
				eed->item_descriptor[item_descriptor_count - 1].item =
					GxCore_Mallocz(eed->item_descriptor[item_descriptor_count - 1].itrm_length + 1);
				if(eed->item_descriptor[item_descriptor_count - 1].item == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("item malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memcpy(eed->item_descriptor[item_descriptor_count - 1].item,&p[1],
							eed->item_descriptor[item_descriptor_count - 1].itrm_length );
				}
			}
			p = si_parse_lib_poffset(p,section_end,
					1 + eed->item_descriptor[item_descriptor_count - 1].itrm_length);
			if(NULL == p)
			{
#ifdef ATSC_SI_DEBUG
				SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
				goto err;
			}

			size = si_parse_lib_sizeoffset(size,1 + eed->item_descriptor[item_descriptor_count - 1].itrm_length);
			if(size < 0)
			{
#ifdef ATSC_SI_DEBUG
				SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
				goto err;
			}
		}
	}
	eed->item_descriptor_count = item_descriptor_count;
	return size;
err:
	eed->item_descriptor_count = item_descriptor_count;
	return -1;
}

static int32_t si_parse_lib_eit_parse_loop_des(GxSubsystemSiParseDvbEITLoop* loop , uint8_t* section , uint8_t*
		section_end)
{
	int32_t size = loop->descriptors_loop_length;
	uint8_t* p = section;
	int32_t stuffing_count = 0,linkage_count = 0,short_event_count = 0,extended_event_count = 0;
	int32_t time_shift_event_count = 0,component_count = 0,CA_indentifier_count = 0,content_count = 0;
	int32_t parental_rating_count = 0,telephone_count = 0,multilingual_component_count = 0;
	int32_t private_data_specifier_count = 0,short_smoothing_buffer_count = 0,data_broadcase_count = 0;
	int32_t PDC_count = 0,des_len = 0;
	int32_t ret = 0;

	while(size > 0)
	{
		switch(p[0])
		{
#if 1

		case SI_PARSE_LIB_TIME_SHIFTED_EVENT_DES:
			{
				time_shift_event_count ++;
				loop->tsed =
					GxCore_Realloc(loop->sed,time_shift_event_count * sizeof(GxSubsystemSiParseTimeShiftEventDes));
				if(loop->tsed == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("dvb eit time_shift_event_malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memset(&loop->tsed[time_shift_event_count - 1],0,sizeof(GxSubsystemSiParseTimeShiftEventDes));
				}
				loop->tsed[time_shift_event_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
				loop->tsed[time_shift_event_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
				loop->tsed[time_shift_event_count - 1].reference_service_id = TODATA0_15BIT(p[2],p[3]);
				loop->tsed[time_shift_event_count - 1].reference_event_id = TODATA0_15BIT(p[4],p[5]);
				p = si_parse_lib_poffset(p,section_end,6);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size ,6);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}
				break;
			}
		case SI_PARSE_LIB_PARENTAL_RATING_DES:
			{
				parental_rating_count ++;
				loop->prd =
					GxCore_Realloc(loop->prd,parental_rating_count * sizeof(GxSubsystemSiParseParentalRatingDes));
				if(loop->prd == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("dvb eit parental_rating_des malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memset(&loop->prd[parental_rating_count - 1],0,sizeof(GxSubsystemSiParseParentalRatingDes));
				}
				loop->prd[parental_rating_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
				loop->prd[parental_rating_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
				if(loop->prd[parental_rating_count - 1].descriptor_length > 0)
				{
					ret = parental_rating_descriptor_parse(&loop->prd[parental_rating_count - 1],&p[2],
							loop->prd[parental_rating_count - 1].descriptor_length,section_end);
					if(ret < 0)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("dvb eit parental_rating_descriptor_parse err\n");
#endif
						goto err;
					}
				}
				p = si_parse_lib_poffset(p,section_end,2 + loop->prd[parental_rating_count - 1].descriptor_length);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size ,2 + loop->prd[parental_rating_count - 1].descriptor_length);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}
				break;
			}
		case SI_PARSE_LIB_EXTENDED_EVENT_DES:
			{
				extended_event_count ++;
				loop->eed = GxCore_Realloc(loop->eed,extended_event_count * sizeof(GxSubsystemSiParseExtendedEventDes));
				if(loop->eed == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("dvb eit extended_event_des malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memset(&loop->eed[extended_event_count - 1],0,sizeof(GxSubsystemSiParseExtendedEventDes));
				}

				loop->eed[extended_event_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
				loop->eed[extended_event_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
				loop->eed[extended_event_count - 1].descriptor_number = TODATA4_7BIT(p[2]);
				loop->eed[extended_event_count - 1].last_descriptor_number = TODATA0_3BIT(p[2]);
				memcpy(loop->eed[extended_event_count - 1].language ,&p[3],3);
				loop->eed[extended_event_count - 1].length_of_items = TODATA0_7BIT(p[6]);

				ret = item_descriptor_parse(&loop->eed[extended_event_count - 1],&p[7],
						loop->eed[extended_event_count - 1].length_of_items,section_end);
				if(ret < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("item descriptor parse err\n");
#endif
					goto err;
				}
				else
				{
					loop->eed[extended_event_count - 1].text_length =
						TODATA0_7BIT(p[7 + loop->eed[extended_event_count - 1].length_of_items + 1]);
					if(loop->eed[extended_event_count - 1].text_length > 0)
					{
						loop->eed[extended_event_count - 1].text =
							GxCore_Mallocz(loop->eed[extended_event_count - 1].text_length + 1);
						if(loop->eed[extended_event_count - 1].text == NULL)
						{
#ifdef ATSC_SI_DEBUG
							SI_PARSE_LIB_ERR_PRINTF("text malloc failed\n");
#endif
							goto err;
						}
						else
						{
							memcpy(loop->eed[extended_event_count - 1].text,
									&p[7 + loop->eed[extended_event_count - 1].length_of_items + 2],
									loop->eed[extended_event_count - 1].text_length);
						}
					}
				}

				p = si_parse_lib_poffset(p,section_end,2 + loop->eed[extended_event_count - 1].descriptor_length);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size ,2 + loop->eed[extended_event_count - 1].descriptor_length);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}
				break;
			}
		case SI_PARSE_LIB_SHORT_EVENT_DES:
			{
				short_event_count++;
				loop->sed = GxCore_Realloc(loop->sed,short_event_count * sizeof(GxSubsystemSiParseShortEventDes));
				if(loop->sed == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("dvb eit short_event_des malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memset(&loop->sed[short_event_count - 1],0,sizeof(GxSubsystemSiParseShortEventDes));
				}
				loop->sed[short_event_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
				loop->sed[short_event_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
				memcpy(&loop->sed[short_event_count - 1].language,&p[2],3);
				loop->sed[short_event_count - 1].event_name_length = TODATA0_7BIT(p[5]);
				if(loop->sed[short_event_count - 1].event_name_length > 0)
				{
					loop->sed[short_event_count - 1].event_name =
						GxCore_Mallocz(loop->sed[short_event_count - 1].event_name_length + 1);
					if(loop->sed[short_event_count - 1].event_name == NULL)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("dvb eit short_event_des malloc failed\n");
#endif
						goto err;
					}
					else
					{
						memcpy(loop->sed[short_event_count - 1].event_name,&p[6],
								loop->sed[short_event_count - 1].event_name_length);
					}
				}
				loop->sed[short_event_count - 1].text_length =
					TODATA0_7BIT(p[5 + 1 + loop->sed[short_event_count - 1].event_name_length]);
				if(loop->sed[short_event_count - 1].text_length > 0)
				{
					loop->sed[short_event_count - 1].text =
						GxCore_Mallocz(loop->sed[short_event_count - 1].text_length + 1);
					if(loop->sed[short_event_count - 1].text == NULL)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("dvb eit short event des text_char malloc failed\n");
#endif
						goto err;
					}
					else
					{
						memcpy(loop->sed[short_event_count - 1].text,
								&p[5 + 1 + 1 + loop->sed[short_event_count - 1].event_name_length],
								loop->sed[short_event_count - 1].text_length);
					}
				}

				p = si_parse_lib_poffset(p,section_end,2+loop->sed[short_event_count - 1].descriptor_length);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size ,2+loop->sed[short_event_count - 1].descriptor_length);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}
				break;
			}
#else
		case SI_PARSE_LIB_STUFFING_DES:
			{
				stuffing_count ++;
				loop->sd = GxCore_Realloc(loop->sd,stuffing_count*sizeof(GxSubsystemSiParseStuffingDes));
				if(loop->sd == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("dvb eit descriptors struffing malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memset(&loop->sd[stuffing_count - 1],0,sizeof(GxSubsystemSiParseStuffingDes));
				}
				loop->sd[stuffing_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
				loop->sd[stuffing_count - 1].descriptor_length = TODATA0_23BIT(p[1],p[2],p[3]);
				if(loop->sd[stuffing_count - 1].descriptor_length > 0)
				{
					loop->sd[stuffing_count - 1].stuffing_byte =
						GxCore_Mallocz(loop->sd[stuffing_count - 1].descriptor_length + 1);
					if(loop->sd[stuffing_count - 1].stuffing_byte == NULL)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("dvb eit stuffing_byte malloc failed\n");
#endif
						goto err;
					}
					else
					{
						memcpy(loop->sd[stuffing_count - 1].stuffing_byte , &p[4] ,
								loop->sd[stuffing_count - 1].descriptor_length );
					}
				}

				p = si_parse_lib_poffset(p,section_end,4+loop->sd[stuffing_count - 1].descriptor_length);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size ,4+loop->sd[stuffing_count - 1].descriptor_length);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}
				break;
			}
		case SI_PARSE_LIB_LINKAGE_DES:
			{
				linkage_count++;
				loop->ld = GxCore_Realloc(loop->ld,
						sizeof(GxSubsystemSiParseLinkageDes)*linkage_count);
				if(loop->ld == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("dvb eit linkage des malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memset(&loop->ld[linkage_count - 1],0,sizeof(GxSubsystemSiParseLinkageDes));
				}
				loop->ld[linkage_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
				loop->ld[linkage_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
				loop->ld[linkage_count - 1].transport_stream_id = TODATA0_15BIT(p[2],p[3]);
				loop->ld[linkage_count - 1].original_network_id = TODATA0_15BIT(p[4],p[5]);
				loop->ld[linkage_count - 1].service_id = TODATA0_15BIT(p[6],p[7]);
				loop->ld[linkage_count - 1].linkage_type = TODATA0_7BIT(p[8]);
				if(loop->ld[linkage_count - 1].linkage_type != 0x08)
				{
					loop->ld[linkage_count - 1].linkage_s.private_data_type =
						GxCore_Mallocz(loop->ld[linkage_count - 1].descriptor_length - 7 + 1);
					if(loop->ld[linkage_count - 1].linkage_s.private_data_type == NULL)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("dvb eit linkage des private_data_type malloc failed\n");
#endif
						goto err;
					}
					else
					{
						memcpy(loop->ld[linkage_count - 1].linkage_s.private_data_type,&p[9],
								loop->ld[linkage_count - 1].descriptor_length - 7);
					}
				}
				else
				{
					loop->ld[linkage_count - 1].linkage_s.mobileroaming_s.hand_over_type = TODATA4_7BIT(p[9]);
					loop->ld[linkage_count - 1].linkage_s.mobileroaming_s.reserved_future_use = TODATA1_3BIT(p[9]);
					loop->ld[linkage_count - 1].linkage_s.mobileroaming_s.original_type = TODATA0BIT(p[9]);
					if(loop->ld[linkage_count - 1].linkage_s.mobileroaming_s.original_type == 0x00)
					{
						loop->ld[linkage_count - 1].linkage_s.mobileroaming_s.type.network_id =
							TODATA0_15BIT(p[10],p[11]);
						flag = 1;
					}
					else if(loop->ld[linkage_count - 1].linkage_s.mobileroaming_s.original_type  == 0x01 ||
							loop->ld[linkage_count - 1].linkage_s.mobileroaming_s.original_type  == 0x02 ||
							loop->ld[linkage_count - 1].linkage_s.mobileroaming_s.original_type  == 0x03)
					{
						loop->ld[linkage_count - 1].linkage_s.mobileroaming_s.type.initial_service_id =
							TODATA0_15BIT(p[10],p[11]);
						flag = 1;
					}

					if(flag)
					{
						loop->ld[linkage_count - 1].linkage_s.mobileroaming_s.private_data_byte =
							GxCore_Mallocz(loop->ld[linkage_count - 1].descriptor_length - 7 - 1 - 2 + 1);
						if(loop->ld[linkage_count - 1].linkage_s.mobileroaming_s.private_data_byte == NULL)
						{
#ifdef ATSC_SI_DEBUG
							SI_PARSE_LIB_ERR_PRINTF("dvb eit linkage private data byte malloc failed\n");
#endif
							goto err;
						}
						else
						{
							memcpy(loop->ld[linkage_count - 1].linkage_s.mobileroaming_s.private_data_byte,&p[12],
									loop->ld[linkage_count - 1].descriptor_length - 7 - 1 - 2);
						}
					}
					else
					{
						loop->ld[linkage_count - 1].linkage_s.mobileroaming_s.private_data_byte =
							GxCore_Mallocz(loop->ld[linkage_count - 1].descriptor_length - 7 - 1 + 1);
						if(loop->ld[linkage_count - 1].linkage_s.mobileroaming_s.private_data_byte == NULL)
						{
#ifdef ATSC_SI_DEBUG
							SI_PARSE_LIB_ERR_PRINTF("dvb eit linkage private data byte malloc failed\n");
#endif
							goto err;
						}
						else
						{
							memcpy(loop->ld[linkage_count - 1].linkage_s.mobileroaming_s.private_data_byte,&p[10],
									loop->ld[linkage_count - 1].descriptor_length - 7 - 1);
						}
					}
				}

				p = si_parse_lib_poffset(p,section_end,2+loop->ld[linkage_count - 1].descriptor_length);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size ,2+loop->ld[linkage_count - 1].descriptor_length);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}
				break;
			}

		case SI_PARSE_LIB_COMPONENT_DES:
			{
				component_count ++;
				loop->componentdes =
					GxCore_Realloc(loop->componentdes,component_count * sizeof(GxSubsystemSiParseComponentDes));
				if(loop->componentdes == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("dvb eit componentdes malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memset(&loop->componentdes[component_count - 1],0,sizeof(GxSubsystemSiParseComponentDes));
				}
				loop->componentdes[component_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
				loop->componentdes[component_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
				loop->componentdes[component_count - 1].reserved_future_use = TODATA4_7BIT(p[2]);
				loop->componentdes[component_count - 1].stream_content = TODATA0_3BIT(p[2]);
				loop->componentdes[component_count - 1].stream_content = TODATA0_7BIT(p[3]);
				loop->componentdes[component_count - 1].component_tag =	TODATA0_7BIT(p[4]);
				memcpy(loop->componentdes[component_count - 1].language,&p[5],3);
				loop->componentdes[component_count - 1].text_length =
					loop->componentdes[component_count - 1].descriptor_length - 6;
				if(loop->componentdes[component_count - 1].text_length > 0)
				{
					loop->componentdes[component_count - 1].text =
						GxCore_Mallocz(loop->componentdes[component_count - 1].text_length + 1);
					if(loop->componentdes[component_count - 1].text == NULL)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("componentdes text_char malloc failed\n");
#endif
						goto err;
					}
					else
					{
						memcpy(loop->componentdes[component_count - 1].text,&p[5],
								loop->componentdes[component_count - 1].text_length);
					}
				}
				p = si_parse_lib_poffset(p,section_end,2 + loop->componentdes[component_count - 1].descriptor_length);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size ,2 + loop->componentdes[component_count - 1].descriptor_length);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}
				break;
			}
		case SI_PARSE_LIB_CA_IDENTIFIER_DES:
			{
				CA_indentifier_count ++;
				loop->cid = GxCore_Realloc(loop->cid,CA_indentifier_count * sizeof(GxSubsystemSiParseCaIdentifierDes));
				if(loop->cid == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("dvb eit CA_indentifier_des malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memset(&loop->cid[CA_indentifier_count - 1],0,sizeof(GxSubsystemSiParseCaIdentifierDes));
				}
				loop->cid[CA_indentifier_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
				loop->cid[CA_indentifier_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
				loop->cid[CA_indentifier_count - 1].ca_system_count =
					loop->cid[CA_indentifier_count - 1].descriptor_length / 2;
				if(loop->cid[CA_indentifier_count - 1].ca_system_count > 0)
				{
					loop->cid[CA_indentifier_count - 1].ca_system_id =
						GxCore_Mallocz(loop->cid[CA_indentifier_count - 1].descriptor_length);
					if(loop->cid[CA_indentifier_count - 1].ca_system_id == NULL)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("dvb eit CA_indentifier_des ca_system_id malloc failed\n");
#endif
						goto err;
					}
					else
					{
						memcpy(loop->cid[CA_indentifier_count - 1].ca_system_id,&p[2],
								loop->cid[CA_indentifier_count - 1].descriptor_length);
					}
				}
				p = si_parse_lib_poffset(p,section_end,2 + loop->cid[CA_indentifier_count - 1].descriptor_length);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size ,2 + loop->cid[CA_indentifier_count - 1].descriptor_length);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}
				break;
			}
		case SI_PARSE_LIB_CONTENT_DES:
			{
				content_count ++;
				loop->contentdes = GxCore_Realloc(loop->contentdes,content_count * sizeof(GxSubsystemSiParseContentDes));
				if(loop->contentdes == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("dvb eit contentdes malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memset(&loop->contentdes[content_count -1],0,sizeof(GxSubsystemSiParseContentDes));
				}
				loop->contentdes[content_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
				loop->contentdes[content_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
				if(loop->contentdes[content_count - 1].descriptor_length > 0)
				{
					ret = content_descriptor_parse(&loop->contentdes[content_count - 1],&p[2],
							loop->contentdes[content_count - 1].descriptor_length,section_end);
					if(ret < 0)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("dvb eit content parse failed\n");
#endif
						goto err;
					}
				}
				p = si_parse_lib_poffset(p,section_end,2 + loop->contentdes[content_count - 1].descriptor_length);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size ,2 + loop->contentdes[content_count - 1].descriptor_length);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}
				break;
			}

		case SI_PARSE_LIB_TELEPHONE_DES:
			{
				uint8_t * p_temp = p;
				telephone_count ++;
				loop->td = GxCore_Realloc(loop->td,telephone_count * sizeof(GxSubsystemSiParseTelephoneDes));
				if(loop->td == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("dvb eit telephone_des malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memset(&loop->td[telephone_count - 1],0,sizeof(GxSubsystemSiParseTelephoneDes));
				}
				loop->td[telephone_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
				loop->td[telephone_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
				loop->td[telephone_count - 1].reserved_future_use = TODATA6_7BIT(p[2]);
				loop->td[telephone_count - 1].foreign_availability = TODATA5BIT(p[2]);
				loop->td[telephone_count - 1].connection_type = TODATA0_4BIT(p[2]);
				loop->td[telephone_count - 1].reserved_future_use2 = TODATA7BIT(p[3]);
				loop->td[telephone_count - 1].country_prefix_length = TODATA5_6BIT(p[3]);
				loop->td[telephone_count - 1].international_area_code_length = TODATA2_4BIT(p[3]);
				loop->td[telephone_count - 1].operator_code_length = TODATA0_1BIT(p[3]);
				loop->td[telephone_count - 1].reserved_future_use3 = TODATA7BIT(p[4]);
				loop->td[telephone_count - 1].national_area_code_length = TODATA4_6BIT(p[4]);
				loop->td[telephone_count - 1].core_number_length = TODATA0_3BIT(p[4]);
				p_temp += 5;
				if(loop->td[telephone_count - 1].country_prefix_length > 0)
				{
					loop->td[telephone_count - 1].country_prefix_char =
						GxCore_Mallocz(loop->td[telephone_count - 1].country_prefix_length + 1);
					if(loop->td[telephone_count - 1].country_prefix_char == NULL)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("dvb eit telephone_des country_prefix_char malloc failed\n");
#endif
						goto err;
					}
					else
					{
						memcpy(loop->td[telephone_count - 1].country_prefix_char,&p_temp[0],
								loop->td[telephone_count - 1].country_prefix_length);
						p_temp += loop->td[telephone_count - 1].country_prefix_length;
					}
				}
				if(loop->td[telephone_count - 1].international_area_code_length > 0)
				{
					loop->td[telephone_count - 1].international_area_code_char =
						GxCore_Mallocz(loop->td[telephone_count - 1].international_area_code_length + 1);
					if(loop->td[telephone_count - 1].international_area_code_char == NULL)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("dvb eit telephone_des international_area_code_char malloc failed\n");
#endif
						goto err;
					}
					else
					{
						memcpy(loop->td[telephone_count - 1].international_area_code_char,&p_temp[0],
								loop->td[telephone_count - 1].international_area_code_length);
						p_temp += loop->td[telephone_count - 1].international_area_code_length;
					}
				}

				if(loop->td[telephone_count - 1].operator_code_length > 0)
				{
					loop->td[telephone_count - 1].operator_code_char =
						GxCore_Mallocz(loop->td[telephone_count - 1].operator_code_length + 1);
					if(loop->td[telephone_count - 1].operator_code_char == NULL)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("dvb eit telephone_des operator_code_char malloc failed\n");
#endif
						goto err;
					}
					else
					{
						memcpy(loop->td[telephone_count - 1].operator_code_char,&p_temp[0],
								loop->td[telephone_count - 1].operator_code_length);
						p_temp += loop->td[telephone_count - 1].operator_code_length;
					}
				}

				if(loop->td[telephone_count - 1].national_area_code_length > 0)
				{
					loop->td[telephone_count - 1].national_area_code_char =
						GxCore_Mallocz(loop->td[telephone_count - 1].national_area_code_length + 1);
					if(loop->td[telephone_count - 1].national_area_code_char == NULL)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("dvb eit telephone_des national_area_code_char malloc failed\n");
#endif
						goto err;
					}
					else
					{
						memcpy(loop->td[telephone_count - 1].national_area_code_char,&p_temp[0],
								loop->td[telephone_count - 1].national_area_code_length);
						p_temp += loop->td[telephone_count - 1].national_area_code_length;
					}
				}

				if(loop->td[telephone_count - 1].core_number_length > 0)
				{
					loop->td[telephone_count - 1].core_number_char =
						GxCore_Mallocz(loop->td[telephone_count - 1].core_number_length + 1);
					if(loop->td[telephone_count - 1].core_number_char == NULL)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("dvb eit telephone_des core_number_char malloc failed\n");
#endif
						goto err;
					}
					else
					{
						memcpy(loop->td[telephone_count - 1].core_number_char,&p_temp[0],
								loop->td[telephone_count - 1].core_number_length);
						p_temp += loop->td[telephone_count - 1].core_number_length;
					}
				}

				p = si_parse_lib_poffset(p,section_end,2 + loop->prd[telephone_count - 1].descriptor_length);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size ,2 + loop->prd[telephone_count - 1].descriptor_length);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}
				break;
			}
		case SI_PARSE_LIB_MULTILINGUAL_COMPONENT_DES:
			{
				multilingual_component_count ++;
				loop->mcd = GxCore_Realloc(loop->mcd,
						multilingual_component_count * sizeof(GxSubsystemSiParseMulitiComponentDes));
				if(loop->mcd == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("dvb eit multilingual_component_des malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memset(&loop->mcd[multilingual_component_count - 1],0,sizeof(GxSubsystemSiParseMulitiComponentDes));
				}
				loop->mcd[multilingual_component_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
				loop->mcd[multilingual_component_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
				loop->mcd[multilingual_component_count - 1].component_tag = TODATA0_7BIT(p[2]);
				if(loop->mcd[multilingual_component_count - 1].descriptor_length > 1)
				{
					ret = multilingual_component_descriptor_parse(&loop->mcd[multilingual_component_count - 1],&p[3],
							loop->mcd[multilingual_component_count - 1].descriptor_length - 1,section_end);
					if(ret < 0)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("multilingual_component_descriptor_parse parse err\n");
#endif
						goto err;
					}
				}

				p = si_parse_lib_poffset(p,section_end,
						2 + loop->mcd[multilingual_component_count - 1].descriptor_length);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size ,2 + loop->mcd[multilingual_component_count - 1].descriptor_length);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}
				break;
			}
		case SI_PARSE_LIB_PRIVATE_DATA_SPECIFIER_DES:
			{
				private_data_specifier_count++;
				loop->pd = GxCore_Realloc(loop->pd,private_data_specifier_count * sizeof(GxSubsystemSiParsePrivateDes));
				if(loop->pd == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("dvb eit private_data_specifier_des malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memset(&loop->pd[private_data_specifier_count - 1],0,sizeof(GxSubsystemSiParsePrivateDes));
				}
				loop->pd[private_data_specifier_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
				loop->pd[private_data_specifier_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
				loop->pd[private_data_specifier_count - 1].private_data_specifier = TODATA0_31BIT(p[2],p[3],p[4],p[5]);

				p = si_parse_lib_poffset(p,section_end,6);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size ,6);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}
				break;
			}
		case SI_PARSE_LIB_DATA_BROADCAST_DES:
			{
				data_broadcase_count++;
				loop->dbd = GxCore_Realloc(loop->dbd,data_broadcase_count * sizeof(GxSubsystemSiParseDataBroadcastDes));
				if(loop->dbd == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("dvb eit data_broadcast_des malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memset(&loop->dbd[data_broadcase_count - 1],0,sizeof(GxSubsystemSiParseDataBroadcastDes));
				}
				loop->dbd[data_broadcase_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
				loop->dbd[data_broadcase_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
				loop->dbd[data_broadcase_count - 1].data_broadcast_id = TODATA0_15BIT(p[2],p[3]);
				loop->dbd[data_broadcase_count - 1].component_tag = TODATA0_15BIT(p[4],p[5]);
				loop->dbd[data_broadcase_count - 1].selector_length = TODATA0_15BIT(p[6],p[7]);
				if(loop->dbd[data_broadcase_count - 1].selector_length > 0)
				{
					loop->dbd[data_broadcase_count - 1].selector_type =
						GxCore_Mallocz(loop->dbd[data_broadcase_count - 1].selector_length + 1);
					if(loop->dbd[data_broadcase_count - 1].selector_type == NULL)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("dvb eit data_broadcast_des selector_type malloc failed\n");
#endif
						goto err;
					}
					else
					{
						memcpy(loop->dbd[data_broadcase_count - 1].selector_type,&p[8],
								loop->dbd[data_broadcase_count - 1].selector_length);
					}
				}
				memcpy(loop->dbd[data_broadcase_count - 1].language,
						&p[7 + loop->dbd[data_broadcase_count - 1].selector_length + 1],3);
				loop->dbd[data_broadcase_count - 1].text_length =
					TODATA0_7BIT(p[7 + loop->dbd[data_broadcase_count - 1].selector_length + 2]);
				if(loop->dbd[data_broadcase_count - 1].text_length > 0)
				{
					loop->dbd[data_broadcase_count - 1].text =
						GxCore_Mallocz(loop->dbd[data_broadcase_count - 1].text_length + 1);
					if(loop->dbd[data_broadcase_count - 1].text == NULL)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("dvb eit data_broadcast_des text malloc failed\n");
#endif
						goto err;
					}
					else
					{
						memcpy(loop->dbd[data_broadcase_count - 1].text,
								&p[7 + loop->dbd[data_broadcase_count - 1].selector_length + 3],
								loop->dbd[data_broadcase_count - 1].text_length);
					}
				}

				p = si_parse_lib_poffset(p,section_end,2 + loop->dbd[data_broadcase_count - 1].descriptor_length);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size ,2 + loop->dbd[data_broadcase_count - 1].descriptor_length);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				break;
			}
		case SI_PARSE_LIB_SHORT_SMOOTHING_BUFFER_DES:
			{
				short_smoothing_buffer_count ++;
				loop->ssd =
					GxCore_Realloc(loop->ssd,short_smoothing_buffer_count* sizeof(GxSubsystemSiParseShortSmoothingDes));
				if(loop->ssd == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("dvb eit short_smoothing_buffer_des malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memset(&loop->ssd[short_smoothing_buffer_count - 1],0,sizeof(GxSubsystemSiParseShortSmoothingDes));
				}
				loop->ssd[short_smoothing_buffer_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
				loop->ssd[short_smoothing_buffer_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
				loop->ssd[short_smoothing_buffer_count - 1].sb_size = TODATA6_7BIT(p[2]);
				loop->ssd[short_smoothing_buffer_count - 1].sb_leak_rate = TODATA0_5BIT(p[2]);
				if(loop->ssd[short_smoothing_buffer_count - 1].descriptor_length > 3)
				{
					loop->ssd[short_smoothing_buffer_count - 1].DVB_reserved =
						GxCore_Mallocz(loop->ssd[short_smoothing_buffer_count - 1].descriptor_length - 3);
					if(loop->ssd[short_smoothing_buffer_count - 1].DVB_reserved == NULL)
					{
#ifdef ATSC_SI_DEBUG
						SI_PARSE_LIB_ERR_PRINTF("dvb eit short_smoothing_buffer_des DVB_reserved malloc failed\n");
#endif
						goto err;
					}
					else
					{
						memcpy(loop->ssd[short_smoothing_buffer_count - 1].DVB_reserved,&p[4],
								loop->ssd[short_smoothing_buffer_count - 1].descriptor_length - 3);
					}
				}

				p = si_parse_lib_poffset(p,section_end,
						2 + loop->ssd[short_smoothing_buffer_count - 1].descriptor_length);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size ,
						2 + loop->ssd[short_smoothing_buffer_count - 1].descriptor_length);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				break;
			}
		case SI_PARSE_LIB_PDC_DES:
			{
				PDC_count ++;
				loop->pdcd = GxCore_Realloc(loop->pdcd,PDC_count * sizeof(GxSubsystemSiParsePdcDes));
				if(loop->pdcd == NULL)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("dvb eit pdc_des malloc failed\n");
#endif
					goto err;
				}
				else
				{
					memset(&loop->pdcd[PDC_count - 1],0,sizeof(GxSubsystemSiParsePdcDes));
				}
				loop->pdcd[PDC_count - 1].descriptor_tag = TODATA0_7BIT(p[0]);
				loop->pdcd[PDC_count - 1].descriptor_length = TODATA0_7BIT(p[1]);
				loop->pdcd[PDC_count - 1].reserved_future_use = TODATA4_7BIT(p[2]);
				loop->pdcd[PDC_count - 1].programme_identification_label = TODATA0_19BIT(p[2],p[3],p[4]);

				p = si_parse_lib_poffset(p,section_end,5);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size , 5);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}
				break;
			}
#endif
		default:
			des_len = TODATA0_7BIT(p[1]);

			p = si_parse_lib_poffset(p,section_end,2 + des_len);
			if(NULL == p)
			{
#ifdef ATSC_SI_DEBUG
				SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
				goto err;
			}

			size = si_parse_lib_sizeoffset(size , 2 + des_len);
			if(size < 0)
			{
#ifdef ATSC_SI_DEBUG
				SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
				goto err;
			}
			break;
		}
	}
	loop->sd_count = stuffing_count;
	loop->ld_count = linkage_count;
	loop->sed_count = short_event_count;
	loop->eed_count = extended_event_count;
	loop->tsed_count = time_shift_event_count;
	loop->componentdes_count = component_count;
	loop->cid_count = CA_indentifier_count;
	loop->contentdes_count = content_count;
	loop->prd_count = private_data_specifier_count;
	loop->ssd_count = short_smoothing_buffer_count;
	loop->dbd_count = data_broadcase_count;
	loop->pdcd_count = PDC_count;
	loop->mcd_count = multilingual_component_count;
	loop->td_count = telephone_count;
	return size;
err:
	loop->sd_count = stuffing_count;
	loop->ld_count = linkage_count;
	loop->sed_count = short_event_count;
	loop->eed_count = extended_event_count;
	loop->tsed_count = time_shift_event_count;
	loop->componentdes_count = component_count;
	loop->cid_count = CA_indentifier_count;
	loop->contentdes_count = content_count;
	loop->prd_count = private_data_specifier_count;
	loop->ssd_count = short_smoothing_buffer_count;
	loop->dbd_count = data_broadcase_count;
	loop->pdcd_count = PDC_count;
	loop->mcd_count = multilingual_component_count;
	loop->td_count = telephone_count;
	return -1;
}

static int32_t si_parse_lib_eit_parse_loop(GxSubsystemSiParseDvbEIT* eit , uint8_t* section , int32_t len)
{
	int32_t size = len,eit_loop_count = 0,ret = 0;
    uint8_t* section_end;
	uint8_t* p = section;

	section_end = p + eit->section_length - 10;
	while(size > 0)
	{
		eit_loop_count++;
		eit->loop =GxCore_Realloc(eit->loop,eit_loop_count*sizeof(GxSubsystemSiParseDvbEITLoop));
		if(eit->loop == NULL)
		{
#ifdef ATSC_SI_DEBUG
			SI_PARSE_LIB_ERR_PRINTF("dvb eit loop realloc failed\n");
#endif
			goto err;
		}
		else
		{
			memset(&eit->loop[eit_loop_count - 1],0,sizeof(GxSubsystemSiParseDvbEITLoop));
		}
		eit->loop[eit_loop_count - 1].event_id = TODATA0_15BIT(p[0],p[1]);
		memcpy(eit->loop[eit_loop_count - 1].start_time,&p[2],5);
		eit->loop[eit_loop_count - 1].start_time[0] = p[2];
		eit->loop[eit_loop_count - 1].start_time[1] = p[3];
		eit->loop[eit_loop_count - 1].start_time[2] = p[4];
		eit->loop[eit_loop_count - 1].start_time[3] = p[5];
		eit->loop[eit_loop_count - 1].start_time[4] = p[6];
		eit->loop[eit_loop_count - 1].duration[0] = TODATA0_7BIT(p[7]);
		eit->loop[eit_loop_count - 1].duration[1] = TODATA0_7BIT(p[8]);
		eit->loop[eit_loop_count - 1].duration[2] = TODATA0_7BIT(p[9]);
		eit->loop[eit_loop_count - 1].running_status = TODATA5_7BIT(p[10]);
		eit->loop[eit_loop_count - 1].free_CA_mode = TODATA4BIT(p[10]);
		eit->loop[eit_loop_count - 1].descriptors_loop_length = TODATA0_11BIT(p[10],p[11]);

		p = si_parse_lib_poffset(p,section_end,12);
		if(NULL == p)
		{
#ifdef ATSC_SI_DEBUG
			SI_PARSE_LIB_ERR_PRINTF("dvb eit loop head size err\n");
#endif
			goto err;
		}

		size = si_parse_lib_sizeoffset(size ,12);
		if(size < 0)
		{
#ifdef ATSC_SI_DEBUG
			SI_PARSE_LIB_ERR_PRINTF("dvb eit loop head size err\n");
#endif
			goto err;
		}

		if((size > 0)&&(eit->loop[eit_loop_count - 1].descriptors_loop_length))
		{
			ret = si_parse_lib_eit_parse_loop_des(&eit->loop[eit_loop_count - 1],p,section_end);
			if(ret < 0)
			{
#ifdef ATSC_SI_DEBUG
				SI_PARSE_LIB_ERR_PRINTF("dvb eit loop descriptors parse err\n");
#endif
				goto err;
			}
			else
			{
				p = si_parse_lib_poffset(p,section_end,eit->loop[eit_loop_count - 1].descriptors_loop_length);
				if(NULL == p)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}

				size = si_parse_lib_sizeoffset(size ,eit->loop[eit_loop_count - 1].descriptors_loop_length);
				if(size < 0)
				{
#ifdef ATSC_SI_DEBUG
					SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
					goto err;
				}
			}
		}
	}
	eit->loop_count = eit_loop_count;
	return size;
err:
	eit->loop_count = eit_loop_count;
	return -1;
}

GxSubsystemSiParseDvbEIT* GxSubsystm_DvbSiParseLibEitParse(uint8_t* section,uint32_t len)
{
	GxSubsystemSiParseDvbEIT* eit = NULL;
	int32_t size = len;
	uint8_t* p = section;
	uint8_t* section_end;
	int32_t ret = 0;

	if(len < 14 || section == NULL)
	{
#ifdef ATSC_SI_DEBUG
		SI_PARSE_LIB_ERR_PRINTF("dvb eit len err || section is NULL\n");
#endif
		return NULL;
	}

	eit = GxCore_Mallocz(sizeof(GxSubsystemSiParseDvbEIT));
	if(eit == NULL)
	{
#ifdef ATSC_SI_DEBUG
		SI_PARSE_LIB_ERR_PRINTF("dvb eit malloc failed\n");
#endif
		return NULL;
	}

	eit->table_id = TODATA0_7BIT(p[0]);
	eit->section_syntax_indicator = TODATA7BIT(p[1]);
	if(eit->section_syntax_indicator != 1)
	{
#ifdef ATSC_SI_DEBUG
		SI_PARSE_LIB_ERR_PRINTF("dvb eit section_syntax_indicator value err\n");
#endif
		return NULL;
	}
	eit->reserved_future_use = TODATA6BIT(p[1]);
	eit->section_length = TODATA0_11BIT(p[1],p[2]);
	if(eit->section_length != size - 3)
	{
#ifdef ATSC_SI_DEBUG
		SI_PARSE_LIB_ERR_PRINTF("dvb eit section_length value err\n");
#endif
		return NULL;
	}
	eit->service_id = TODATA0_15BIT(p[3],p[4]);
	eit->version_number = TODATA1_5BIT(p[5]);
	eit->current_next_indicator = TODATA0BIT(p[5]);
	eit->section_number = TODATA0_7BIT(p[6]);
	eit->last_section_number = TODATA0_7BIT(p[7]);
	eit->transport_stream_id = TODATA0_15BIT(p[8],p[9]);
	eit->original_network_id = TODATA0_15BIT(p[10],p[11]);
	eit->segment_last_section_number = TODATA0_7BIT(p[12]);
	eit->last_table_id = TODATA0_7BIT(p[13]);

	section_end = p + size;

	p = si_parse_lib_poffset(p,section_end,14);
	if(NULL == p)
	{
#ifdef ATSC_SI_DEBUG
		SI_PARSE_LIB_ERR_PRINTF("eit size err");
#endif
		goto err;
	}

	size = si_parse_lib_sizeoffset(size ,14);
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
	if(size != eit->section_length - 11 - 4)
	{
#ifdef ATSC_SI_DEBUG
		SI_PARSE_LIB_ERR_PRINTF("section_length is err");
#endif
		goto err;
	}

	if(size > 12)//loop头为12字节
	{
		ret = si_parse_lib_eit_parse_loop(eit,p,size);
		if(ret < 0)
		{
#ifdef ATSC_SI_DEBUG
			SI_PARSE_LIB_ERR_PRINTF("dvb eit loop parse err\n");
#endif
			goto err;
		}
	}

    si_parse_lib_eit_add_protect((void*)eit);
	return eit;
err:
	si_parse_lib_eit_release(eit);
	eit = NULL;
	return NULL;
}

int32_t  GxSubsystm_DvbSiParseLibEitRelease(void * eit)
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

    si_parse_lib_eit_release((GxSubsystemSiParseDvbEIT *)eit);
    return GX_SI_PARSE_LIB_SUCCESS;
}
