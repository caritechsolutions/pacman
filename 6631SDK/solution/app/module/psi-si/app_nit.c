#include "app_module.h"
#include "app_send_msg.h"
#include "app_default_params.h"
#include "app_wnd_search.h"

#if LCN_SUPPORT
typedef int32_t (*lcn_parse)(uint8_t *p_section_data, uint16_t len,
        uint16_t original_id, uint16_t network_id, uint16_t ts_id);

typedef struct _LCNTagParse
{
    uint32_t tag;
    lcn_parse parse;
}LCNTagParse;

extern int32_t app_lcn_tag82_83_parse(uint8_t *p_section_data, uint16_t len,
        uint16_t original_id, uint16_t network_id, uint16_t ts_id);

extern int32_t app_lcn_tag87_parse(uint8_t *p_section_data, uint16_t len,
        uint16_t original_id, uint16_t network_id, uint16_t ts_id);

extern int32_t app_lcn_tag88_parse(uint8_t *p_section_data, uint16_t len,
        uint16_t original_id, uint16_t network_id, uint16_t ts_id);

static LCNTagParse s_LcnTagParse[] =
{
    {0x82, app_lcn_tag82_83_parse},
    {0x83, app_lcn_tag82_83_parse},
    {0x87, app_lcn_tag87_parse},
    {0x88, app_lcn_tag88_parse},
};

static int32_t _lcn_tag_index_get(uint32_t tag)
{
    int32_t i = 0, size = 0;

    size = sizeof(s_LcnTagParse)/sizeof(LCNTagParse);

    for(i = 0; i < size; i++)
    {
        if(s_LcnTagParse[i].tag == tag)
            return i;
    }

    return -1;
}

private_parse_status app_table_nit_extend_descriptor_parse(uint8_t* p_section_data, size_t Size)
{
	uint8_t *data, *data1, *network_des;
	int16_t len1 = 0;
	int16_t len2 = 0;
	int16_t len3 = 0;
	int16_t len4 = 0;
	int16_t len_network_des = 0;
	int16_t i = 0;
	int32_t nitNetworkid = 0;
    uint16_t original_network_id = 0;
	uint32_t ret = PRIVATE_SECTION_OK;
	uint16_t ts_id = 0;
	data = p_section_data;
	static uint8_t* psNitSectionFlag = NULL;
    int32_t tag_index = -1;
    int32_t lcn_find = 0;

	if (NULL == p_section_data)
		return ret;

	if(NULL == psNitSectionFlag)
	{
		psNitSectionFlag = GxCore_Malloc(data[7]+1);
		if (NULL == psNitSectionFlag)
			return ret;
		memset(psNitSectionFlag,0,data[7]+1);
	}
	app_log_debug("app_table_nit_search_section_parse Size=%d len=%d\n",Size, ((data[1]&0x0f)<<8) + data[2] + 3 );
	app_log_debug("app_table_nit_search_section_parse data[7]=%d data[6]=%d\n",data[7],data[6]);
	if(data[0] == NIT_ACTUAL_NETWORK_TID)
	{
		if(data[6] > data[7])
		{
			app_log_debug("nit section number=%d big than last section number=%d \n",data[6],data[7]);
			data[6] = data[7];
		}
		psNitSectionFlag[data[6]] = 1;
		for(i = 0;i<=data[7];i++)
		{
			if(psNitSectionFlag[i]!=1)
			{
				break;
			}
		}
		if(i == (data[7]+1))
		{
			GxCore_Free(psNitSectionFlag);
			psNitSectionFlag = NULL;
			ret = PRIVATE_SUBTABLE_OK;
		}

		nitNetworkid = (data[3]<<8)|(data[4]);
		app_log_debug("MAIN_FREQ_NITVERSION=%d\n", (data[5] & 0x3E) >> 1);

		len1 = ((data[8]&0xf)<<8)+data[9];
        len_network_des = len1;
        network_des = data + (9+1);
 		len2 = ((data[9+len1+1]&0xf)<<8)+data[9+len1+2];
		data +=(9+1+len1+2);
		while(len2>0)
		{
            lcn_find = 0;
			ts_id = ((data[0]<<8)&0xff00)+data[1];
            original_network_id = ((data[2]<<8)&0xff00)+data[3];
			len4 = len3 = ((data[4]&0xf)<<8)+data[5];
			data1 = &data[6];
			while(len3>0)
			{
                tag_index = _lcn_tag_index_get(data1[0]);
                if(tag_index != -1)
                {
                    lcn_find = 1;
                    s_LcnTagParse[tag_index].parse(&(data1[0]), data1[1], original_network_id, nitNetworkid, ts_id);
                }

				len3 -= (data1[1]+2);
				data1 += (data1[1]+2);
			}

            if(0 == lcn_find) /*如果前面解不到lcn，就把network_descriptor解一下 */
            {
                uint8_t *temp_newtwork_des = network_des;
	            int16_t temp_len_network_des = len_network_des;

                while(temp_len_network_des>0)
                {
                    tag_index = _lcn_tag_index_get(temp_newtwork_des[0]);
                    if(tag_index != -1)
                        s_LcnTagParse[tag_index].parse(&(temp_newtwork_des[0]), temp_newtwork_des[1], original_network_id, nitNetworkid, ts_id);

                    temp_len_network_des -= (temp_newtwork_des[1]+2);
                    temp_newtwork_des += (temp_newtwork_des[1]+2);
                }
            }

			data +=(6+len4);
			len2 -=(6+len4);
		}
	}
	return ret;
}
#endif

#if (DEMOD_DVB_C > 0)||(DEMOD_DTMB > 0)

uint8_t* pNitSectionFlag = NULL;
static uint8_t app_nit_start_flag = 0;
static int32_t s_NitSubtId = -1;
static uint32_t s_NitRequestId = 0;


/*
* NIT搜索，分析NIT表
*/
private_parse_status app_table_nit_search_section_parse(uint8_t* p_section_data, size_t Size)
{
    uint8_t *data, *data1;
    int16_t len1 = 0;
    int16_t len2 = 0;
    int16_t len3 = 0;
    int16_t len4 = 0;
    int16_t i = 0;
	int16_t j = 0;
	uint32_t temp = 0;
    uint32_t		freq = 0;
    uint32_t		symbol = 0;
    uint32_t		modulation = 0;
    uint32_t ret = PRIVATE_SECTION_OK;
    uint16_t ts_id;

    data = p_section_data;

    if (NULL == p_section_data)
        return ret;
    if(NULL == pNitSectionFlag)
    {
        pNitSectionFlag = GxCore_Malloc(data[7]+1);
        if (NULL == pNitSectionFlag)
            return ret;
        memset(pNitSectionFlag,0,data[7]+1);
    }
    app_log_debug("app_table_nit_search_section_parse Size=%d len=%d\n",Size, ((data[1]&0x0f)<<8) + data[2] + 3 );
    app_log_debug("app_table_nit_search_section_parse data[7]=%d data[6]=%d\n",data[7],data[6]);
    if(data[0] == NIT_ACTUAL_NETWORK_TID)
    {
        pNitSectionFlag[data[6]] = 1;
        for(i = 0;i<=data[7];i++)
        {
            if(pNitSectionFlag[i]!=1)
            {
                break;
            }
        }
        if(i == (data[7]+1))
        {
        	if (NULL != pNitSectionFlag )
        	{
	        	GxCore_Free(pNitSectionFlag);
	            pNitSectionFlag = NULL;
        	}
            ret = PRIVATE_SUBTABLE_OK;
        }
        freq = searchFreList.app_fre_array[0];
        app_log_debug("MAIN_FREQ_NIT=%d\n", freq);
        app_log_debug("MAIN_FREQ_NITVERSION=%d\n", (data[5] & 0x3E) >> 1);
        freq = 0;

        len1 = ((data[8]&0xf)<<8)+data[9];
        len2 = ((data[9+len1+1]&0xf)<<8)+data[9+len1+2];
        data +=(9+1+len1+2);
        while(len2>0)
        {
            ts_id = ((data[0]<<8)&0xff00)+data[1];
            len4 = len3 = ((data[4]&0xf)<<8)+data[5];
            data1 = &data[6];
            while(len3>0)
            {
                switch(data1[0])
                {
                    case 0x44://CABLE_DELIVERY_SYSTEM_DESCRIPTOR:
                        { // -c
                            freq = ((data1[2] & 0xf0)>>4) * 10000000 + (data1[2] & 0xf) * 1000000
                                + ((data1[3] & 0xf0)>>4) * 100000 + (data1[3] & 0xf) * 10000
                                + ((data1[4] & 0xf0)>>4) * 1000 + (data1[4] & 0xf) * 100
                                + ((data1[5] & 0xf0) >> 4) * 10 + (data1[5] & 0xf);
                            symbol = ((data1[9] & 0xf0)>>4) * 1000000 + (data1[9] & 0xf) * 100000
                                + ((data1[10] & 0xf0)>>4) * 10000 + (data1[10] & 0xf) * 1000
                                + ((data1[11] & 0xf0)>>4) * 100 + (data1[11] & 0xf) * 10
                                + ((data1[12] & 0xf0) >> 4) ;
                            symbol = symbol /10;  // k
                            freq = freq/10; // khz
                       //     freq = freq/1000;// mhz
                            modulation = data1[8];
							app_log_debug("app_table_nit_search_section_parse cable freq=%d\n",freq);
                            app_log_debug("app_table_nit_search_section_parse cable modulation=%d\n",modulation);
                            for (i=0; i< searchFreList.num;i++)
                            {
                                if (freq == searchFreList.app_fre_array[i])
                                {
                                    searchFreList.app_fre_tsid[i] = ts_id;
                                    break;
                                }
                            }
                            if (i == searchFreList.num)
                            {
                                if (freq > 0)
                                {
                                    searchFreList.app_fre_array[searchFreList.num] = freq;
                                    searchFreList.app_symb_array[searchFreList.num] = symbol;
                                    searchFreList.app_qam_array[searchFreList.num] = modulation-1;
                                    searchFreList.app_fre_tsid[searchFreList.num] = ts_id;
                                    searchFreList.num++;
                                }
                            }
                        }
                        break;
                    default:
                        break;
                }
                len3 -= (data1[1]+2);
                data1 += (data1[1]+2);
            }
            data +=(6+len4);
            len2 -=(6+len4);
        }
    }

	/*
	* 频点排序，按频点大小顺序搜索
	*/
	if (PRIVATE_SUBTABLE_OK == ret)
	{
		for (i = 0; i< searchFreList.num-1;i++)
		{
			for (j=i+1;j<searchFreList.num;j++)
			{
				if (searchFreList.app_fre_array[j] < searchFreList.app_fre_array[i])
				{
					temp = searchFreList.app_fre_array[j];
					searchFreList.app_fre_array[j] = searchFreList.app_fre_array[i];
					searchFreList.app_fre_array[i] = temp;

					temp = searchFreList.app_symb_array[j];
					searchFreList.app_symb_array[j] = searchFreList.app_symb_array[i];
					searchFreList.app_symb_array[i] = temp;

					temp = searchFreList.app_qam_array[j];
					searchFreList.app_qam_array[j] = searchFreList.app_qam_array[i];
					searchFreList.app_qam_array[i] = temp;

					temp = searchFreList.app_fre_tsid[j];
					searchFreList.app_fre_tsid[j] = searchFreList.app_fre_tsid[i];
					searchFreList.app_fre_tsid[i] = temp;
				}
			}
			GxCore_ThreadDelay(50);
		}
	}
    return ret;
}

void app_table_nit_search_filter_open(void)
{
    static GxSubTableDetail subt_detail = {0};
    GxMsgProperty_SiCreate	params_create;
    GxMsgProperty_SiStart params_start;
    AppFrontend_Config cfg = {0};

    if (NULL != pNitSectionFlag )
    {
        GxCore_Free(pNitSectionFlag);
        pNitSectionFlag = NULL;
    }
    app_nit_start_flag = 1; /*NIT搜索FILTER标志置1*/

    if(s_NitSubtId == -1)
    {
        app_ioctl(app_tuner_cur_tuner_get(), FRONTEND_CONFIG_GET, &cfg);

        subt_detail.ts_src = cfg.ts_src; // multi-ts need this TS1-0 TS2-1 TS3-2
        subt_detail.demux_id = cfg.dmx_id;
        subt_detail.time_out = 5000;
        subt_detail.si_filter.pid = NIT_PID;
        subt_detail.si_filter.match_depth = 1/*5*/;
        subt_detail.si_filter.eq_or_neq = EQ_MATCH;
        subt_detail.si_filter.match[0] = NIT_ACTUAL_NETWORK_TID;
        subt_detail.si_filter.mask[0] = 0xff;
        params_create = &subt_detail;

        subt_detail.table_parse_cfg.mode = PARSE_PRIVATE_ONLY;
        subt_detail.table_parse_cfg.table_parse_fun = app_table_nit_search_section_parse;

        app_send_msg_exec(GXMSG_SI_SUBTABLE_CREATE, (void*)&params_create);

        s_NitSubtId = subt_detail.si_subtable_id;
        s_NitRequestId = subt_detail.request_id;

        // start si
        params_start = s_NitSubtId;
        app_send_msg_exec(GXMSG_SI_SUBTABLE_START, (void*)&params_start);
    }
}

void app_table_nit_search_filter_close(void)
{
    GxMsgProperty_SiRelease params_release;

    if (NULL != pNitSectionFlag )
    {
        GxCore_Free(pNitSectionFlag);
        pNitSectionFlag = NULL;
    }

    if (-1 != s_NitSubtId)
    {
        params_release = s_NitSubtId;
        app_send_msg_exec(GXMSG_SI_SUBTABLE_RELEASE, (void*)&params_release);
    }
    s_NitSubtId = -1;
    app_nit_start_flag = 0;/*NIT搜索FILTER标志清零*/
}

void app_table_nit_get_search_filter_info(int32_t* pNitSubtId,uint32_t* pNitRequestId)
{
    if ((NULL == pNitSubtId)||(NULL == pNitRequestId))
        return;

    *pNitSubtId = s_NitSubtId ;
    *pNitRequestId = s_NitRequestId;
    return;
}
#endif


