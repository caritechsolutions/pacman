#include "channel_common.h"

GxBusPmDataProgVideoType channel_get_bus_video_type(AppProgVideoType video_type)
{
	GxBusPmDataProgVideoType type;
	switch(video_type)
	{
		case APP_PM_PROG_MPEG:
			type = GXBUS_PM_PROG_MPEG;
			break;
		case APP_PM_PROG_AVS:
			type = GXBUS_PM_PROG_AVS;
			break;
		case APP_PM_PROG_H264:
			type = GXBUS_PM_PROG_H264;
			break;
		case APP_PM_PROG_H265:
			type = GXBUS_PM_PROG_H265;
			break;
		case APP_PM_PROG_MPEG4:
			type = GXBUS_PM_PROG_MPEG4;
			break;
		default:
			type = GXBUS_PM_PROG_MPEG;
			break;
	}
	return type;
}

AppProgVideoType channel_get_app_video_type(GxBusPmDataProgVideoType video_type)
{
	AppProgVideoType type;
	switch(video_type)
	{
		case GXBUS_PM_PROG_MPEG:
			type = APP_PM_PROG_MPEG;
			break;
		case GXBUS_PM_PROG_AVS:
			type = APP_PM_PROG_AVS;
			break;
		case GXBUS_PM_PROG_H264:
			type = APP_PM_PROG_H264;
			break;
		case GXBUS_PM_PROG_H265:
			type = APP_PM_PROG_H265;
			break;
		case GXBUS_PM_PROG_MPEG4:
			type = APP_PM_PROG_MPEG4;
			break;
		default:
			type = APP_PM_PROG_MPEG;
			break;
	}
	return type;
}

void channel_get_video_type_str(char* video_str,AppProgVideoType video_type,int str_len)
{
	switch(video_type)
	{
		case APP_PM_PROG_MPEG:
			memcpy(video_str,"MPEG2",str_len);
			break;
		case APP_PM_PROG_AVS:
			memcpy(video_str,"AVS",str_len);
			break;
		case APP_PM_PROG_H264:
			memcpy(video_str,"H.264",str_len);
			break;
		case APP_PM_PROG_H265:
			memcpy(video_str,"H.265",str_len);
			break;
		case APP_PM_PROG_MPEG4:
			memcpy(video_str,"MPEG4",str_len);
			break;
		default:
			memset(video_str,0,str_len);
			break;
	}
}

GxBusPmDataProgAudioType channel_get_bus_audio_type(AppProgAudioType audio_type)
{
	GxBusPmDataProgAudioType type;
	switch(audio_type)
	{
		case APP_PM_AUDIO_MPEG1:
			type = GXBUS_PM_AUDIO_MPEG1;
			break;
		case APP_PM_AUDIO_MPEG2:
			type = GXBUS_PM_AUDIO_MPEG2;
			break;
		case APP_PM_AUDIO_AAC_LATM:
			type = GXBUS_PM_AUDIO_AAC_LATM;
			break;
		case  APP_PM_AUDIO_AC3:
			type = GXBUS_PM_AUDIO_AC3;
			break;
		case APP_PM_AUDIO_AAC_ADTS:
			type = GXBUS_PM_AUDIO_AAC_ADTS;
			break;
		case APP_PM_AUDIO_EAC3:
			type = GXBUS_PM_AUDIO_EAC3;
			break;
		case  APP_PM_AUDIO_LPCM:
			type = GXBUS_PM_AUDIO_LPCM;
			break;
		case APP_PM_AUDIO_DTS:
			type = GXBUS_PM_AUDIO_DTS;
			break;
		case APP_PM_AUDIO_DOLBY_TRUEHD:
			type = GXBUS_PM_AUDIO_DOLBY_TRUEHD;
			break;
		case APP_PM_AUDIO_AC3_PLUS:
			type = GXBUS_PM_AUDIO_AC3_PLUS;
			break;
		case APP_PM_AUDIO_DTS_HD:
			type = GXBUS_PM_AUDIO_DTS_HD;
			break;
		case APP_PM_AUDIO_DTS_MA:
			type = GXBUS_PM_AUDIO_DTS_MA;
			break;
		case APP_PM_AUDIO_AC3_PLUS_SEC:
			type = GXBUS_PM_AUDIO_AC3_PLUS_SEC;
			break;
		case APP_PM_AUDIO_DTS_HD_SEC:
			type = GXBUS_PM_AUDIO_DTS_HD_SEC;
			break;
		case APP_PM_AUDIO_DRA:
			type = GXBUS_PM_AUDIO_DRA;
			break;
		default:
			type = GXBUS_PM_AUDIO_MPEG2;
			break;
	}
	return type;
}

AppProgAudioType channel_get_app_audio_type(GxBusPmDataProgAudioType audio_type)
{
	AppProgAudioType type;
	switch(audio_type)
	{
		case GXBUS_PM_AUDIO_MPEG1:
			type = APP_PM_AUDIO_MPEG1;
			break;
		case GXBUS_PM_AUDIO_MPEG2:
			type = APP_PM_AUDIO_MPEG2;
			break;
		case GXBUS_PM_AUDIO_AAC_LATM:
			type = APP_PM_AUDIO_AAC_LATM;
			break;
		case  GXBUS_PM_AUDIO_AC3:
			type = APP_PM_AUDIO_AC3;
			break;
		case GXBUS_PM_AUDIO_AAC_ADTS:
			type = APP_PM_AUDIO_AAC_ADTS;
			break;
		case GXBUS_PM_AUDIO_EAC3:
			type = APP_PM_AUDIO_EAC3;
			break;
		case  GXBUS_PM_AUDIO_LPCM:
			type = APP_PM_AUDIO_LPCM;
			break;
		case GXBUS_PM_AUDIO_DTS:
			type = APP_PM_AUDIO_DTS;
			break;
		case GXBUS_PM_AUDIO_DOLBY_TRUEHD:
			type = APP_PM_AUDIO_DOLBY_TRUEHD;
			break;
		case GXBUS_PM_AUDIO_AC3_PLUS:
			type = APP_PM_AUDIO_AC3_PLUS;
			break;
		case  GXBUS_PM_AUDIO_DTS_HD:
			type = APP_PM_AUDIO_DTS_HD;
			break;
		case GXBUS_PM_AUDIO_DTS_MA:
			type = APP_PM_AUDIO_DTS_MA;
			break;
		case GXBUS_PM_AUDIO_AC3_PLUS_SEC:
			type = APP_PM_AUDIO_AC3_PLUS_SEC;
			break;
		case GXBUS_PM_AUDIO_DTS_HD_SEC:
			type = APP_PM_AUDIO_DTS_HD_SEC;
			break;
		case GXBUS_PM_AUDIO_DRA:
			type = APP_PM_AUDIO_DRA;
			break;
		default:
			type = APP_PM_AUDIO_MPEG2;
			break;
	}
	return type;
}

void channel_get_audio_type_str(char* audio_str,AppProgAudioType audio_type,int str_len)
{
	switch(audio_type)
	{
		case APP_PM_AUDIO_MPEG1:
			memcpy(audio_str,"MPEG-1",str_len);
			break;
		case APP_PM_AUDIO_MPEG2:
			memcpy(audio_str,"MPEG-2",str_len);
			break;
		case APP_PM_AUDIO_AAC_LATM:
			memcpy(audio_str,"AAC_L",str_len);
			break;
		case  APP_PM_AUDIO_AC3:
			memcpy(audio_str,"AC3",str_len);
			break;
		case APP_PM_AUDIO_AAC_ADTS:
			memcpy(audio_str,"AAC_A",str_len);
			break;
		case APP_PM_AUDIO_EAC3:
			 memcpy(audio_str,"EAC3",str_len);
			break;
		case  APP_PM_AUDIO_LPCM:
			memcpy(audio_str,"LPCM",str_len);
			break;
		case APP_PM_AUDIO_DTS:
			memcpy(audio_str,"DTS",str_len);
			break;
		case APP_PM_AUDIO_DOLBY_TRUEHD:
			memcpy(audio_str,"DOLBY",str_len);
			break;
		case APP_PM_AUDIO_AC3_PLUS:
			memcpy(audio_str,"AC3_P",str_len);
			break;
		case APP_PM_AUDIO_DTS_HD:
			 memcpy(audio_str,"DTS_HD",str_len);
			break;
		case APP_PM_AUDIO_DTS_MA:
			memcpy(audio_str,"DTS_MA",str_len);
			break;
		case APP_PM_AUDIO_AC3_PLUS_SEC:
			memcpy(audio_str,"AC3_S",str_len);
			break;
		case APP_PM_AUDIO_DTS_HD_SEC:
			memcpy(audio_str,"DTS_S",str_len);
			break;
		case APP_PM_AUDIO_DRA:
			 memcpy(audio_str,"DRA",str_len);
			break;
		default:
			memset(audio_str,0,str_len);
			break;
	}
}

int channel_fav_flag_to_id_add(AppIdOps *id_ops ,uint32_t flag)
{
	if (flag == 0 || id_ops == NULL)
		return GXCORE_ERROR;

	GxMsgProperty_NodeNumGet node_num = {0};
	node_num.node_type = NODE_FAV_GROUP;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &node_num);

	uint32_t i=0;
	GxMsgProperty_NodeByPosGet node_get = {0};
	for(i=0; i<node_num.node_num; i++)
	{
		if(0 != (flag&(1<<i)))
		{
			node_get.node_type = NODE_FAV_GROUP;
			node_get.pos = i;
			app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);
			id_ops->id_add(id_ops, 0, node_get.fav_item.id|FAV_MASK);
		}
	}

	return GXCORE_SUCCESS;
}

static int  _channel_get_sattv_valid_group_id(AppIdOps *cl_id, AppIdOps *sat_id)
{
	GxBusPmViewInfo view_info;
	int i = 0 ;
	int count = 0 ;
	int size = 0 ;
	int buff[EUTUL_SAT_MAX_NUM] ;

	size = EUTUL_SAT_MAX_NUM ;
	count = app_fuse_spec_add_group_id(buff,size);

	for(i=0;i<count;i++)
	{
		if ( g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO )
		{
			app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
			view_info.group_mode = GROUP_MODE_SAT;
			view_info.sat_id = buff[i];
			view_info.stream_type = GXBUS_PM_PROG_RADIO;
			if ( 0 >= GxBus_PmProgNumGetByViewInfo(&view_info) )
				continue ;
		}
		if(NULL != cl_id)
			cl_id->id_add(cl_id, 0, buff[i]);
		if(NULL != sat_id)
			sat_id->id_add(sat_id, 0, buff[i]);
	}
	return count ;
}

int channel_get_valid_group_id(AppIdOps *cl_id, AppIdOps *sat_id, AppIdOps *fav_id, AppIdOps *hd_id, GxBusPmViewInfoSkipView view_mode)
{
	status_t ret = GXCORE_SUCCESS;
    uint32_t hd_group_num = 0;

	GxBusPmViewInfo view_info_bak = {0};
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info_bak));

	GxBusPmViewInfo view_info = {0};
	memcpy(&view_info, &view_info_bak, sizeof(GxBusPmViewInfo));

	view_info.group_mode = GROUP_MODE_ALL; //all
	view_info.skip_view_switch = view_mode;
    view_info.stream_type=g_AppPlayOps.normal_play.view_info.stream_type;
    GxBus_PmViewInfoMemSet(&view_info); // Only memory, not flash

	// get total sat
	GxMsgProperty_NodeNumGet sat_num = {0};
	sat_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&sat_num));

	// get total fav
	GxMsgProperty_NodeNumGet fav_num = {0};
	fav_num.node_type = NODE_FAV_GROUP;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&fav_num));

	// get total prog
	GxMsgProperty_NodeNumGet prog_num = {0};
	prog_num.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&prog_num));

    //get total hd num
    GxMsgProperty_NodeNumGet hd_num = {0};
    hd_num.node_type = NODE_HD_GROUP;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&hd_num));
    if(hd_num.node_num > 0)
    {
        hd_group_num = 1;
    }

	// id manage
	if(NULL != cl_id)
	{
		cl_id->id_close(cl_id);//clear first
	#if ((DEMOD_DVB_COMBO == 1) || ((DEMOD_DVB_C != 1) && (DEMOD_DTMB != 1)))
		if (cl_id->id_open(cl_id, MANAGE_DIFF_LIST, sat_num.node_num+fav_num.node_num + hd_group_num + 1) != GXCORE_SUCCESS)
	#else
		if (cl_id->id_open(cl_id, MANAGE_DIFF_LIST, fav_num.node_num+hd_group_num+1) != GXCORE_SUCCESS)
	#endif
		{
			ret = GXCORE_ERROR;
			goto end;
		}
	}
	if(NULL != sat_id)
	{
		sat_id->id_close(sat_id);
		if (sat_id->id_open(sat_id, MANAGE_DIFF_LIST, sat_num.node_num+1) != GXCORE_SUCCESS)
		{
			ret = GXCORE_ERROR;
			goto end;
		}
	}
	if(NULL != fav_id)
	{
		fav_id->id_close(fav_id);
		if (fav_id->id_open(fav_id, MANAGE_DIFF_LIST, fav_num.node_num) != GXCORE_SUCCESS)
		{
			ret = GXCORE_ERROR;
			goto end;
		}
	}
    if(NULL != hd_id && hd_group_num > 0 && view_info.stream_type == GXBUS_PM_PROG_TV)
	{
		hd_id->id_close(hd_id);
		if (hd_id->id_open(hd_id, MANAGE_DIFF_LIST, hd_group_num) != GXCORE_SUCCESS)
		{
			ret = GXCORE_ERROR;
			goto end;
		}
	}

	//add sat id
	uint32_t i=0;
	uint32_t fav_flag=0;
	uint32_t hd_count=0;
	GxMsgProperty_NodeByPosGet node_get = {0};

	//增加sattv后，有可能只有1个卫星，所以第１个并不是ALL, 所以需要根据freescan节目个数判断。兼容以前
	if ( prog_num.node_num > 0 )
	{
		//add 0: all satellite
		if(NULL != cl_id)
			cl_id->id_add(cl_id, 0, ALL_SAT);
		if(NULL != sat_id)
			sat_id->id_add(sat_id, 0, ALL_SAT);
	}

	for(i=0; i<prog_num.node_num; i++)
	{
		memset(&node_get, 0, sizeof(GxMsgProperty_NodeByPosGet));
		node_get.node_type = NODE_PROG;
		node_get.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);
        extern bool app_check_prog_visible(uint32_t prog_id);

        if(app_check_prog_visible(node_get.prog_data.id) == false)
            continue;

		fav_flag |= node_get.prog_data.favorite_flag;
        if(node_get.prog_data.definition == GXBUS_PM_PROG_HD)
        {
            hd_count++;
        }
	#if ((DEMOD_DVB_COMBO == 1) || ((DEMOD_DVB_C != 1) && (DEMOD_DTMB != 1)))
		if(NULL != cl_id)
			cl_id->id_add(cl_id, 0, node_get.prog_data.sat_id);
		if(NULL != sat_id)
			sat_id->id_add(sat_id, 0, node_get.prog_data.sat_id);
	#endif
	}

	//增加sattv可用的卫星列表
	_channel_get_sattv_valid_group_id(cl_id,sat_id);

	//add fav id
	if(NULL != cl_id)
		channel_fav_flag_to_id_add(cl_id, fav_flag);
	if(NULL != fav_id)
		channel_fav_flag_to_id_add(fav_id, fav_flag);

    if(view_info.stream_type == GXBUS_PM_PROG_TV && hd_group_num > 0 && hd_count > 0)
    {
        if(NULL != cl_id)
		    cl_id->id_add(cl_id, 0, HD_MASK);
	    if(NULL != hd_id)
	        hd_id->id_add(hd_id, 0, HD_MASK);
    }

	//restore
end:
    GxBus_PmViewInfoMemClear();//recovery flash viewinfo
	return ret;
}

int channel_id_total_and_buffer_dump(AppIdOps *id_ops, uint32_t *id_total, uint32_t **id_buffer)
{
	if(id_ops == NULL)
	{
		app_log_error("----id_ops null error, %s:%d----\n",__func__,__LINE__);
		return GXCORE_ERROR;
	}

	*id_total = id_ops->id_total_get(id_ops);
	if(*id_total == 0)
	{
		app_log_error("----id total get error, %s:%d----\n",__func__,__LINE__);
		return GXCORE_ERROR;
	}

	*id_buffer = GxCore_Calloc(*id_total, sizeof(uint32_t));
	if(*id_buffer == NULL)
	{
		app_log_error("----malloc failed error, %s:%d----\n",__func__,__LINE__);
		return GXCORE_ERROR;
	}
	id_ops->id_get(id_ops, *id_buffer);

	return GXCORE_SUCCESS;
}

//根据当前view_info找到现在是第几个组
uint32_t channel_get_cur_group_num(AppIdOps *cl_id)
{
	uint32_t id_total = 0;
	uint32_t *id_buffer = NULL;
	if(channel_id_total_and_buffer_dump(cl_id, &id_total, &id_buffer) == GXCORE_ERROR)
	{
		app_log_error("----channel_id_total_and_buffer_dump error, %s:%d----\n",__func__,__LINE__);
		return -1;
	}

	uint32_t i = 0;
	uint32_t group_num = 0;
	if(GROUP_MODE_ALL == g_AppPlayOps.normal_play.view_info.group_mode)//all
	{
		group_num = 0;
	}
    else if(GROUP_MODE_HD == g_AppPlayOps.normal_play.view_info.group_mode)
    {
        for( i = 1; i < id_total; i++)
        {
            if(id_buffer[i] == HD_MASK)
            {
                group_num = i;
            }
        }
    }
	else if(GROUP_MODE_SAT == g_AppPlayOps.normal_play.view_info.group_mode)//sat
	{
		for(i=1; i<id_total; i++)
		{
			if((id_buffer[i] & FAV_MASK) == 0)//sat
			{
				if(g_AppPlayOps.normal_play.view_info.sat_id == id_buffer[i])
				{
					group_num = i;
					break;
				}
			}
		}
	}
	else if(GROUP_MODE_FAV == g_AppPlayOps.normal_play.view_info.group_mode)//fav
	{
		for(i=1; i<id_total; i++)
		{
			if((id_buffer[i] & FAV_MASK) != 0)//fav
			{
				if(g_AppPlayOps.normal_play.view_info.fav_id == (id_buffer[i] & (~FAV_MASK)))
				{
					group_num = i;
					break;
				}
			}
		}
	}

	GxCore_Free(id_buffer);
	return group_num;
}

char *app_get_sat_name(GxBusPmDataSat *sat)
{
    switch (sat->type)
    {
        case GXBUS_PM_SAT_S:        return (char *)sat->sat_s.sat_name;
        case GXBUS_PM_SAT_C:        return "DVB-C";
        case GXBUS_PM_SAT_T:        return "DVB-T";
        case GXBUS_PM_SAT_DTMB:     return "DVB-DTMB";
        case GXBUS_PM_SAT_DVBT2:    return "DVB-T/T2";
        case GXBUS_PM_SAT_ATSC_C:   return "DVB-ATSC-C";
        case GXBUS_PM_SAT_ATSC_T:   return "DVB-ATSC-T";
        case GXBUS_PM_SAT_NET :     return (char *)sat->sat_net.server_name;
        default:                    return NULL;
    }
}

int channel_cmb_group_content_dump(AppIdOps *cl_id, char **buffer_all)
{
#define BUFFER_LENGTH_GROUP  (30)

	uint32_t id_total = 0;
	uint32_t *id_buffer = NULL;
	if(channel_id_total_and_buffer_dump(cl_id, &id_total, &id_buffer) == GXCORE_ERROR)
	{
		app_log_error("----channel_id_total_and_buffer_dump error, %s:%d----\n",__func__,__LINE__);
		return GXCORE_ERROR;
	}

	*buffer_all = GxCore_Calloc(id_total, BUFFER_LENGTH_GROUP);
	if(*buffer_all == NULL)
	{
		app_log_error("----malloc failed error, %s:%d----\n",__func__,__LINE__);
		GxCore_Free(id_buffer);
		return GXCORE_ERROR;
	}

	uint32_t i = 0;
	GxMsgProperty_NodeByIdGet node;
	char buffer[BUFFER_LENGTH_GROUP] = {0};

	memset(buffer, 0, BUFFER_LENGTH_GROUP);

	//增加sattv后，有可能只有1个卫星，所以第１个需要判断。兼容以前
	if(id_buffer[0] == ALL_SAT)
		sprintf(buffer,"[%s",STR_ID_ALL); //all
	else if((id_buffer[0] & FAV_MASK) == 0)//sat
	{
		memset(&node, 0, sizeof(GxMsgProperty_NodeByIdGet));
		node.node_type = NODE_SAT;
		node.id = id_buffer[0];
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
		sprintf(buffer,"[%s",(char *)app_get_sat_name(&node.sat_data));
	}

	strcat(*buffer_all, buffer);

	for(i=1; i < id_total; i++)
	{
		memset(&node, 0, sizeof(GxMsgProperty_NodeByIdGet));
		if((id_buffer[i] & FAV_MASK) == 0)//sat
		{
			node.node_type = NODE_SAT;
			node.id = id_buffer[i];
			app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

			memset(buffer, 0, BUFFER_LENGTH_GROUP);
			sprintf(buffer,",%s",(char *)app_get_sat_name(&node.sat_data));
		}
        else if(id_buffer[i] == HD_MASK)
        {
	        memset(buffer, 0, BUFFER_LENGTH_GROUP);
            sprintf(buffer,",%s",STR_ID_HD);
        }
		else //fav
		{
			node.node_type = NODE_FAV_GROUP;
			node.id = (id_buffer[i] & (~FAV_MASK));
			app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);

			memset(buffer, 0, BUFFER_LENGTH_GROUP);
			sprintf(buffer,",%s",node.fav_item.fav_name);
		}
        strcat(*buffer_all, buffer);
	}

	strcat(*buffer_all, "]");
	GxCore_Free(id_buffer);

	return GXCORE_SUCCESS;
}

void channel_sat_tp_info_str_get(GxBusPmDataSat *sat_data, GxBusPmDataTP *tp_data, char *buffer, int len)
{
	if(sat_data == NULL || tp_data == NULL || buffer == NULL || len == 0)
		return;

	memset(buffer, 0 , len);

#if DEMOD_DVB_S
	if(GXBUS_PM_SAT_S == sat_data->type)
	{
		char* polar_str[2] = {"H", "V"};
		sprintf(buffer, "%d / %s / %d",
				tp_data->frequency,
				polar_str[tp_data->tp_s.polar],
				tp_data->tp_s.symbol_rate);
	}
#endif

#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
	if(GXBUS_PM_SAT_DVBT2 == sat_data->type)
	{
        char* bandwidth_str[BANDWIDTH_AUTO] = {"8M", "7M", "6M"};
        GxBusPmDataProg *prog_arry = NULL;
        prog_arry = GxCore_Malloc(1 * sizeof(GxBusPmDataProg));
        if(prog_arry == NULL)
        {
             app_log_error("Malloc prog_arry failed!!!!!!!!!!!\n");
             return;
        }
        memset(prog_arry,0 ,sizeof(GxBusPmDataProg));
        GxBus_PmProgGetByTpId(tp_data->id,1,prog_arry);
        if(tp_data->tp_dvbt2.bandwidth < BANDWIDTH_AUTO)
			sprintf(buffer, "%d.%d M/%s/%s",
					tp_data->frequency/1000,
					(tp_data->frequency/100)%10,
					bandwidth_str[tp_data->tp_dvbt2.bandwidth],prog_arry[0].common_status ==1?"DVBT":"DVBT2");
        GxCore_Free(prog_arry);
	}
#endif

#if DEMOD_DVB_C
	if(GXBUS_PM_SAT_C == sat_data->type)
	{
		char* modulation_str[5] = {"16QAM", "32QAM", "64QAM", "128QAM", "256QAM"};
        if((tp_data->tp_c.modulation >= DVBC_16QAM) && (tp_data->tp_c.modulation <= DVBC_256QAM))
            sprintf(buffer, "%.1fM/%dK/%s/DVBC",
                    tp_data->frequency/1000.0,
                    tp_data->tp_c.symbol_rate,
                    modulation_str[tp_data->tp_c.modulation-DVBC_16QAM]);
	}
#endif

#if DEMOD_DTMB
	if(GXBUS_PM_SAT_DTMB == sat_data->type)
	{
		if(GXBUS_PM_SAT_1501_DTMB == sat_data->sat_dtmb.work_mode)
		{
			char* bandwidth_str[BANDWIDTH_AUTO] = {"8M", "7M","6M"};
			if(tp_data->tp_dtmb.symbol_rate < BANDWIDTH_AUTO)
				sprintf(buffer, "%d.%d/%s/DTMB",
						tp_data->frequency/1000,
						(tp_data->frequency/100)%10,
						bandwidth_str[tp_data->tp_dtmb.bandwidth]);
		}
		else
		{// work in dvbc mode
			char* modulation_str[5] = {"16QAM", "32QAM","64QAM","128QAM","256QAM"};

			if((tp_data->tp_dtmb.modulation >= DVBC_16QAM) && (tp_data->tp_dtmb.modulation <= DVBC_256QAM))
				sprintf(buffer, "%dM/%dK/%s/DVBC",
						tp_data->frequency/1000,
                        tp_data->tp_dtmb.symbol_rate,
						modulation_str[tp_data->tp_dtmb.modulation-DVBC_16QAM]);
		}
	}
#endif

}

void channel_prog_tp_info_str_get(GxBusPmDataProg *prog_data, char* buffer, int len)
{
	if(prog_data == NULL || buffer == NULL || len == 0)
		return;

	//get sat node
	GxMsgProperty_NodeByIdGet node_sat = {0};
	node_sat.node_type = NODE_SAT;
	node_sat.id = prog_data->sat_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_sat);

	//get tp node
	GxMsgProperty_NodeByIdGet node_tp = {0};
	node_tp.node_type = NODE_TP;
	node_tp.id = prog_data->tp_id;
	app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp);

	channel_sat_tp_info_str_get(&node_sat.sat_data, &node_tp.tp_data, buffer, len);
}

int channel_set_group_fav_hd_type(GxMsgProperty_NodeByPosGet prog_node,int hd_type)
{

	GxMsgProperty_NodeModify node_modify = {0};


	if(hd_type)//hd program-//((info->video.height > 576) && (info->video.width > 720))
	{
		node_modify.node_type = NODE_PROG;
		memcpy(&node_modify.prog_data, &prog_node.prog_data, sizeof(GxBusPmDataProg));
		node_modify.prog_data.definition = GXBUS_PM_PROG_HD;
		if(memcmp(&prog_node.prog_data, &node_modify.prog_data, sizeof(GxBusPmDataProg)) != 0)
		{
			if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify))
			{
				app_log_error("\nmodify program error\n");
			}
		}
	}
	else
	{
		node_modify.node_type = NODE_PROG;
		memcpy(&node_modify.prog_data, &prog_node.prog_data, sizeof(GxBusPmDataProg));
		node_modify.prog_data.definition = GXBUS_PM_PROG_SD;
		if(memcmp(&prog_node.prog_data, &node_modify.prog_data, sizeof(GxBusPmDataProg)) != 0)
		{
			if(GXCORE_SUCCESS != app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify))
			{
				app_log_error("\nmodify program --error\n");
			}
		}
	}


	return GXCORE_SUCCESS;
}

int channel_get_playing_pos_in_group(uint32_t *ppos)
{
	GxBusPmDataProg prog_data;
	GxBusPmViewInfo sysinfo;
	uint32_t prog_num,i;
	uint32_t prog_id = 0;

	if (NULL == ppos)
		return FALSE;

	prog_num = GxBus_PmProgNumGet();
	if (0 == prog_num)
	{
	    //qm, 2015-01-28
		*ppos = 0;
		return FALSE;
	}
	GxBus_PmViewInfoGet(&sysinfo);

	if(sysinfo.stream_type == GXBUS_PM_PROG_TV)
	{
		prog_id = sysinfo.tv_prog_cur;
	}
	else if(sysinfo.stream_type == GXBUS_PM_PROG_RADIO)
	{
		prog_id = sysinfo.radio_prog_cur;
	}
	for(i=0;i<prog_num;i++)
	{
		GxBus_PmProgGetByPos(i,1,(void*)&prog_data);
		if(prog_id == prog_data.id)
		{
			*ppos = i;

			break;
		}
	}
	if(i == prog_num)
	{
		*ppos = 0;
		 return FALSE;
	}
	return TRUE;
}

#if WEBAPI_SUPPORT
int webapi_channel_get_group_id(AppIdOps *cl_id, GxBusPmDataProgType  stream_type)
{
	status_t ret = GXCORE_SUCCESS;
	uint32_t hd_group_num = 0;

	GxBusPmViewInfo view_info_bak = {0};
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info_bak));

	GxBusPmViewInfo view_info = {0};
	memcpy(&view_info, &view_info_bak, sizeof(GxBusPmViewInfo));

	view_info.group_mode = GROUP_MODE_ALL; //all
	view_info.skip_view_switch = VIEW_INFO_SKIP_CONTAIN;
	view_info.stream_type = stream_type;
    GxBus_PmViewInfoMemSet(&view_info);// Only memory, not flash

	// get total sat
	GxMsgProperty_NodeNumGet sat_num = {0};
	sat_num.node_type = NODE_SAT;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&sat_num));

	// get total fav
	GxMsgProperty_NodeNumGet fav_num = {0};
	fav_num.node_type = NODE_FAV_GROUP;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&fav_num));

	// get total prog
	GxMsgProperty_NodeNumGet prog_num = {0};
	prog_num.node_type = NODE_PROG;
	app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&prog_num));

	//get total hd num
	GxMsgProperty_NodeNumGet hd_num = {0};
	if((view_info.stream_type==GXBUS_PM_PROG_TV)||(view_info.stream_type==GXBUS_PM_PROG_ALL))
	{
		hd_num.node_type = NODE_HD_GROUP;
		app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&hd_num));
		if(hd_num.node_num>0)
		{
			hd_group_num = 1;
		}
	}

	// id manage
	if(NULL != cl_id)
	{
		cl_id->id_close(cl_id);//clear first
		if(cl_id->id_open(cl_id, MANAGE_DIFF_LIST, sat_num.node_num+fav_num.node_num+1) != GXCORE_SUCCESS)
		{
			ret = GXCORE_ERROR;
			goto end;
		}
	}

	//add sat id
	uint32_t i=0;
	uint32_t fav_flag=0;
	GxMsgProperty_NodeByPosGet node_get = {0};

	if ( prog_num.node_num > 0 )
	{
		//add 0: all satellite
		cl_id->id_add(cl_id, 0, ALL_SAT);
	}


	for(i=0; i<prog_num.node_num; i++)
	{
		memset(&node_get, 0, sizeof(GxMsgProperty_NodeByPosGet));
		node_get.node_type = NODE_PROG;
		node_get.pos = i;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_get);

		fav_flag |= node_get.prog_data.favorite_flag;
		cl_id->id_add(cl_id, 0, node_get.prog_data.sat_id);
	}

	//增加sattv可用的卫星列表
	_channel_get_sattv_valid_group_id(cl_id,NULL);

	//add fav id
	channel_fav_flag_to_id_add(cl_id, fav_flag);

	if((NULL != cl_id)&&(hd_group_num>0))
	{
		cl_id->id_add(cl_id, 0, HD_MASK);
	}

	//restore
	end:
    GxBus_PmViewInfoMemClear();////recovery flash viewinfo

	return ret;
}

#endif
