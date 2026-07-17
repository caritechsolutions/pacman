#include "app.h"
#include "module/app_psi.h"

#define PMT_FIXED_LENGTH    (16)
#define PMT_LOOP_FIXED_LENGTH    (5)

#define ISO639_DESCIPTOR_LENGTH    (6)
#define AC3_DESCIPTOR_LENGTH    (3)
#define EAC3_DESCIPTOR_LENGTH    (3)
#define DRA1_DESCIPTOR_LENGTH      (6)
#define SUBT_DESCIPTOR_LENGTH    (10)
#define TTX_DESCIPTOR_LENGTH    (7)

static unsigned short _pmt_stream_private_generate(char *buff, DescriptorInfo *descriptorinfo)
{
    unsigned short i = 0;
    unsigned short j = 0;

    buff[i++] = descriptorinfo->tag;
    buff[i++] = descriptorinfo->length;
    for(j = 0; j < descriptorinfo->length; j++)
    {
        buff[i++] = descriptorinfo->data[j];
    }

    return i;

}

static unsigned short _pmt_stream_video_generate(char *buff, unsigned short video_pid, GxBusPmDataProgVideoType video_type)
{
    unsigned short i = 0;
    unsigned char stream_type = 0;

    if(buff == NULL)
        return 0;

    if(video_type == GXBUS_PM_PROG_H264)
        stream_type = 0x1b;
    else if(video_type == GXBUS_PM_PROG_MPEG)
        stream_type = 0x01;
    else if(video_type == GXBUS_PM_PROG_H265)
        stream_type = 0x24;
    else if(video_type == GXBUS_PM_PROG_AVS)
        stream_type = 0x42;
    else
        stream_type = 0x02;

    buff[i++] = stream_type;
    buff[i++] = ((video_pid >> 8) & 0x1f);
    buff[i++] = video_pid & 0xff;
    buff[i++] = 0;
    buff[i++] = 0;

    return i;
}

static bool _check_audio_iso639(char *lang)
{
    if((lang == NULL) || (strlen(lang) == 0) || (strlen(lang) > 3))
        return false;

    return true;
}

static GxBusPmDataProgAudioType _get_pm_audio_type(AudioCodecType audio_type)
{
    GxBusPmDataProgAudioType type;

    switch(audio_type)
    {
        case AUDIO_CODEC_MPEG1:
            type = GXBUS_PM_AUDIO_MPEG1;
            break;

        case AUDIO_CODEC_MPEG2:
            type = GXBUS_PM_AUDIO_MPEG2;
            break;

        case AUDIO_CODEC_AC3:
            type = GXBUS_PM_AUDIO_AC3;
            break;

        case AUDIO_CODEC_EAC3:
            type = GXBUS_PM_AUDIO_EAC3;
            break;

        case AUDIO_CODEC_AAC_ADTS:
            type = GXBUS_PM_AUDIO_AAC_ADTS;
            break;

        case AUDIO_CODEC_AAC_LATM:
            type = GXBUS_PM_AUDIO_AAC_LATM;
            break;

		case AUDIO_CODEC_DRA1:
			type = GXBUS_PM_AUDIO_DRA;
			break;

        default:
            type = GXBUS_PM_AUDIO_MPEG2;
            break;
    }

    return type;
}

static unsigned short _pmt_stream_audio_generate(char *buff, unsigned short audio_pid, GxBusPmDataProgAudioType audio_type, char *iso_str)
{
    unsigned short i = 0;
    unsigned char stream_type = 0;
    unsigned short es_info_length = 0;
    bool iso_flag = false;
    bool ac3_flag = false;
    bool eac3_flag = false;

    if(buff == NULL)
        return 0;

    if(audio_type == GXBUS_PM_AUDIO_MPEG1)
    {
        stream_type = 0x03;
    }
    else if(audio_type == GXBUS_PM_AUDIO_MPEG2)
    {
        stream_type = 0x04;
    }
    else if(audio_type == GXBUS_PM_AUDIO_AAC_ADTS)
    {
        stream_type = 0x0f;
    }
    else if(audio_type == GXBUS_PM_AUDIO_AAC_LATM)
    {
        stream_type = 0x11;
    }
    else if(audio_type == GXBUS_PM_AUDIO_AC3)
    {
        stream_type = 0x06;
        es_info_length += AC3_DESCIPTOR_LENGTH;
        ac3_flag = true;
    }
    else if(audio_type == GXBUS_PM_AUDIO_EAC3)
    {
        stream_type = 0x06;
        es_info_length += EAC3_DESCIPTOR_LENGTH;
        eac3_flag = true;
    }
    else if(audio_type == GXBUS_PM_AUDIO_DRA)
    {
        stream_type = 0x06;
        es_info_length += DRA1_DESCIPTOR_LENGTH; //DRA DESCIPTOR LENGTH
    }
    else
    {
        stream_type = 0x04;
    }

    if(_check_audio_iso639(iso_str) == true)
    {
        es_info_length += ISO639_DESCIPTOR_LENGTH;
        iso_flag = true;
    }

    buff[i++] = stream_type;
    buff[i++] = ((audio_pid >> 8) & 0x1f);
    buff[i++] = audio_pid & 0xff;

    //es info lenth
    buff[i++] = ((es_info_length >> 8) & 0x0f);
    buff[i++] = (es_info_length & 0xff);

    if(iso_flag == true)
    {
        buff[i++] = 0x0a;
        buff[i++] = 4;
        buff[i++] = iso_str[0];
        buff[i++] = iso_str[1];
        buff[i++] = iso_str[2];
        buff[i++] = 1;
    }

    if(ac3_flag == true)
    {
        buff[i++] = 0x6a;
        buff[i++] = 1;
        buff[i++] = 0;
    }

    if(eac3_flag == true)
    {
        buff[i++] = 0x7a;
        buff[i++] = 1;
        buff[i++] = 0;
    }

    if(audio_type == GXBUS_PM_AUDIO_DRA)
    {
        buff[i++] = 0x05;
        buff[i++] = 4;
        buff[i++] = 'D';
        buff[i++] = 'R';
        buff[i++] = 'A';
        buff[i++] = '1';
    }

    return i;
}

#if 0
static unsigned short _pmt_stream_ttx_generate(char *buff, unsigned short ttx_pid, unsigned char type, char *iso_str, unsigned char magazine, unsigned char page)
{
    unsigned short i = 0;

    if(buff == NULL)
        return 0;

    if((iso_str == NULL) || (strlen(iso_str) == 0))
        return 0;

    //type
    buff[i++] = 0x06;
    //elem_pid
    buff[i++] = ((ttx_pid >> 8) & 0x1f);
    buff[i++] = (ttx_pid & 0xff);
    //es info length
    buff[i++] = 0;
    buff[i++] = TTX_DESCIPTOR_LENGTH;

    //descriptor tag
    buff[i++] = 0x56;
    //descriptor length
    buff[i++] = TTX_DESCIPTOR_LENGTH - 2;
    //iso_639
    buff[i++] = iso_str[0];
    buff[i++] = iso_str[1];
    buff[i++] = iso_str[2];
    //teletext type
    buff[i] = (type << 3);
    //magzine num
    buff[i++] |= (magazine & 0x07);
    //page num
    buff[i++] = page;

    return i;
}
#endif

static unsigned short _pmt_stream_subt_generate(char *buff, unsigned short subt_pid, unsigned char type, char *iso_str, unsigned short composite_page_id, unsigned short ancillary_page_id)
{
    unsigned short i = 0;

    if(buff == NULL)
        return 0;

    if((iso_str == NULL) || (strlen((char*)iso_str) == 0))
        return 0;

    //type
    buff[i++] = 0x06;
    //elem_pid
    buff[i++] = ((subt_pid >> 8) & 0x1f);
    buff[i++] = (subt_pid & 0xff);
    //es info length
    buff[i++] = 0;
    buff[i++] = SUBT_DESCIPTOR_LENGTH;

    //adescriptor tag
    buff[i++] = 0x59;
    //descriptor length
    buff[i++] = SUBT_DESCIPTOR_LENGTH - 2;
    //iso_639
    buff[i++] = iso_str[0];
    buff[i++] = iso_str[1];
    buff[i++] = iso_str[2];
    //teletext type
    buff[i++] = type;
    //composite_page_id
    buff[i++] = (composite_page_id >> 8) & 0xff;
    buff[i++] = composite_page_id & 0xff;
    //composite_page_id
    buff[i++] = (ancillary_page_id >> 8) & 0xff;
    buff[i++] = ancillary_page_id & 0xff;

    return i;
}

/////plz release result data if valid /////
char *app_pmt_generate(unsigned short porg_id, DescriptorInfo *descriptorinfo)
{
    char *pmt_data = NULL;
    char *temp_data = NULL;
    unsigned short i = 0;
    unsigned short cnt = 0;
    unsigned short length = 0;
    unsigned short temp_len = 0;
    unsigned int crc_value = 0;
    AudioLang_t *audio_list = NULL;
    GxBusPmDataProgAudioType audio_type;
    TtxSubtOps *p_ttx_subt = &g_AppTtxSubt;
    GxMsgProperty_NodeByIdGet node = {0};

    node.node_type = NODE_PROG;
    node.id = porg_id;
    if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node) == GXCORE_ERROR)
        return NULL;

    pmt_data = (char*)GxCore_Calloc(PMT_FIXED_LENGTH, 1);
    if(pmt_data != NULL)
    {
        length = PMT_FIXED_LENGTH;
        //table_id
        pmt_data[i++] = 0x02;
        //synctax / length
        pmt_data[i++] = 0x80;
        pmt_data[i++] = 0x00;
        //prog_num
        pmt_data[i++] = ((node.prog_data.service_id >> 8) & 0xff);
        pmt_data[i++] = (node.prog_data.service_id & 0xff);
        //version num / current_next_indicator
        pmt_data[i++] = 1;
        //sec_num
        pmt_data[i++] = 0x00;
        //last_sec_num
        pmt_data[i++] = 0x00;
        //pcr
        pmt_data[i++] = (node.prog_data.pcr_pid >> 8) & 0x1f;//0x1f;
        pmt_data[i++] = node.prog_data.pcr_pid & 0xff;//0xff;
        //prog_info_length
        pmt_data[i++] = 0;
        if(descriptorinfo && descriptorinfo->tag != 0 && descriptorinfo->length > 0)
        {
            pmt_data[i++] = descriptorinfo->length + 2;
            temp_data = (char*)GxCore_Realloc(pmt_data, length + descriptorinfo->length + 2);
            if(temp_data == NULL)
                goto error_ret;

            pmt_data = temp_data;
            temp_data = NULL;
            temp_len = _pmt_stream_private_generate(pmt_data + i, descriptorinfo);
            i += temp_len;
            length += temp_len;
        }
        else
        {
            pmt_data[i++] = 0;
        }
        if(node.prog_data.service_type == GXBUS_PM_PROG_TV)
        {
            // video
            temp_data = (char*)GxCore_Realloc(pmt_data, length + PMT_LOOP_FIXED_LENGTH);
            if(temp_data == NULL)
                goto error_ret;

            pmt_data = temp_data;
            temp_data = NULL;
            temp_len = _pmt_stream_video_generate(pmt_data + i, node.prog_data.video_pid, node.prog_data.video_type);
            i += temp_len;
            length += temp_len;
        }
        // audio
        audio_list = app_get_audio_lang();
        if((audio_list != NULL) && (audio_list->audioLangList != NULL) && (audio_list->audioLangCnt != 0))
        {
            ///current audio
            for(cnt = 0; cnt < audio_list->audioLangCnt; cnt++)
            {
                if(audio_list->audioLangList[cnt].id == node.prog_data.cur_audio_pid)
                {
                    temp_len = PMT_LOOP_FIXED_LENGTH;
                    if(_check_audio_iso639(audio_list->audioLangList[cnt].name) == true) //iso639
                    {
                        temp_len += ISO639_DESCIPTOR_LENGTH;
                    }

                    if(audio_list->audioLangList[cnt].audiotype == AUDIO_CODEC_AC3) //ac3 descriptor
                    {
                        temp_len += AC3_DESCIPTOR_LENGTH;
                    }

                    if(audio_list->audioLangList[cnt].audiotype == AUDIO_CODEC_EAC3) //eac3 descriptor
                    {
                        temp_len += EAC3_DESCIPTOR_LENGTH;
                    }

                    if(audio_list->audioLangList[cnt].audiotype == AUDIO_CODEC_DRA1) //DRA1 descriptor
                    {
                        temp_len += DRA1_DESCIPTOR_LENGTH;
                    }

                    temp_data = (char*)GxCore_Realloc(pmt_data, length + temp_len);
                    if(temp_data == NULL)
                        goto error_ret;

                    pmt_data = temp_data;
                    temp_data = NULL;
                    audio_type = _get_pm_audio_type(audio_list->audioLangList[cnt].audiotype);
                    temp_len = _pmt_stream_audio_generate(pmt_data + i, audio_list->audioLangList[cnt].id,
                            audio_type, audio_list->audioLangList[cnt].name);
                    i += temp_len;
                    length += temp_len;
                    break;
                }
            }

            ///other audio
            for(cnt = 0; cnt < audio_list->audioLangCnt; cnt++)
            {
                if(audio_list->audioLangList[cnt].id != node.prog_data.cur_audio_pid)
                {
                    temp_len = PMT_LOOP_FIXED_LENGTH;
                    if(_check_audio_iso639(audio_list->audioLangList[cnt].name) == true) //iso639
                    {
                        temp_len += ISO639_DESCIPTOR_LENGTH;
                    }

                    if(audio_list->audioLangList[cnt].audiotype == AUDIO_CODEC_AC3) //ac3 descriptor
                    {
                        temp_len += AC3_DESCIPTOR_LENGTH;
                    }

                    if(audio_list->audioLangList[cnt].audiotype == AUDIO_CODEC_EAC3) //eac3 descriptor
                    {
                        temp_len += EAC3_DESCIPTOR_LENGTH;
                    }

                    if(audio_list->audioLangList[cnt].audiotype == AUDIO_CODEC_DRA1) //DRA1 descriptor
                    {
                        temp_len += DRA1_DESCIPTOR_LENGTH;
                    }

                    temp_data = (char*)GxCore_Realloc(pmt_data, length + temp_len);
                    if(temp_data == NULL)
                        goto error_ret;

                    pmt_data = temp_data;
                    temp_data = NULL;
                    audio_type = _get_pm_audio_type(audio_list->audioLangList[cnt].audiotype);
                    temp_len = _pmt_stream_audio_generate(pmt_data + i, audio_list->audioLangList[cnt].id,
                            audio_type, audio_list->audioLangList[cnt].name);
                    i += temp_len;
                    length += temp_len;
                }
            }
        }
        else
        {
            temp_len = PMT_LOOP_FIXED_LENGTH;
            if(node.prog_data.cur_audio_type == GXBUS_PM_AUDIO_AC3) //ac3 descriptor
            {
                temp_len += AC3_DESCIPTOR_LENGTH;
            }

            if(node.prog_data.cur_audio_type == GXBUS_PM_AUDIO_EAC3) //eac3 descriptor
            {
                temp_len += EAC3_DESCIPTOR_LENGTH;
            }

            if(node.prog_data.cur_audio_type == GXBUS_PM_AUDIO_DRA) //dra descriptor
            {
                temp_len += DRA1_DESCIPTOR_LENGTH;
            }

            temp_data = (char*)GxCore_Realloc(pmt_data, length + temp_len);
            if(temp_data == NULL)
                goto error_ret;

            pmt_data = temp_data;
            temp_data = NULL;
            temp_len = _pmt_stream_audio_generate(pmt_data + i, node.prog_data.cur_audio_pid,
                            node.prog_data.cur_audio_type, NULL);
            i += temp_len;
            length += temp_len;
        }

        //dvb subt
        for(cnt = p_ttx_subt->ttx_num; cnt < p_ttx_subt->ttx_num + p_ttx_subt->subt_num; cnt++)
        {
            temp_len = PMT_LOOP_FIXED_LENGTH + SUBT_DESCIPTOR_LENGTH;
            temp_data = (char*)GxCore_Realloc(pmt_data, length + temp_len);
            if(temp_data == NULL)
                goto error_ret;
            pmt_data = temp_data;
            temp_data = NULL;

            temp_len = _pmt_stream_subt_generate(pmt_data + i, p_ttx_subt->content[cnt].elem_pid, p_ttx_subt->content[cnt].type,
                    (char*)p_ttx_subt->content[cnt].lang, p_ttx_subt->content[cnt].subt.composite_page_id, p_ttx_subt->content[cnt].subt.ancillary_page_id);
            i += temp_len;
            length += temp_len;
        }

        length -= 3;
        pmt_data[1] |= ((length >> 8) & 0x0f);
        pmt_data[2] = (length & 0xff);

        crc_value = dvb_crc32((unsigned char*)pmt_data, i);
        //crc
        pmt_data[i++] = (crc_value >> 24 & 0xff);
        pmt_data[i++] = (crc_value >> 16 & 0xff);
        pmt_data[i++] = (crc_value >> 8 & 0xff);
        pmt_data[i++] = (crc_value & 0xff);

#ifdef APP_PSI_LOG
        length = i;
        app_log_debug("\n######### PMT Length = %d #########\n", length);
        for(i = 0; i < length; i++)
        {
            app_log_debug("%02x ", pmt_data[i]);
            if(i % 16 == 15)
                app_log_debug("\n");
        }
        app_log_debug("\n###################################\n");
#endif
    }

    return pmt_data;

error_ret:

    if(pmt_data != NULL)
    {
        GxCore_Free(pmt_data);
        pmt_data = NULL;
    }
    return NULL;
}

static status_t pmt_add_prog_info(PMTInfo *pmt_info, DescriptorInfo *des_info)
{
    DescriptorInfo *temp_des = NULL;
    DescriptorInfo *temp = NULL;

    if((pmt_info == NULL) || (des_info == NULL))
        return GXCORE_ERROR;

    temp_des = (DescriptorInfo*)GxCore_Calloc(1, sizeof(DescriptorInfo));
    if(temp_des == NULL)
        return GXCORE_ERROR;
    memmove(temp_des, des_info, sizeof(DescriptorInfo));
    temp_des->next = NULL;

    if(pmt_info->prog_info == NULL)
    {
        pmt_info->prog_info = temp_des;
    }
    else
    {
        temp = pmt_info->prog_info;
        while(temp->next != NULL)
            temp = temp->next;
        temp->next = temp_des;
    }

    return GXCORE_SUCCESS;
}

static status_t pmt_stream_add_es_info(PmtStreamInfo *stream_info, DescriptorInfo *des_info)
{
    DescriptorInfo *temp_des = NULL;
    DescriptorInfo *temp = NULL;

    if((stream_info == NULL) || (des_info == NULL))
        return GXCORE_ERROR;

    temp_des = (DescriptorInfo*)GxCore_Calloc(1, sizeof(DescriptorInfo));
    if(temp_des == NULL)
        return GXCORE_ERROR;
    memmove(temp_des, des_info, sizeof(DescriptorInfo));
    temp_des->next = NULL;

    if(stream_info->es_info == NULL)
    {
        stream_info->es_info = temp_des;
    }
    else
    {
        temp = stream_info->es_info;
        while(temp->next != NULL)
            temp = temp->next;
        temp->next = temp_des;
    }

    return GXCORE_SUCCESS;
}

static status_t pmt_add_stream_info(PMTInfo *pmt_info, PmtStreamInfo *stream_info)
{
    PmtStreamInfo *temp_stream = NULL;
    PmtStreamInfo *temp = NULL;

    if((pmt_info == NULL) || (stream_info == NULL))
        return GXCORE_ERROR;

    temp_stream = (PmtStreamInfo*)GxCore_Calloc(1, sizeof(PmtStreamInfo));
    if(temp_stream == NULL)
        return GXCORE_ERROR;
    memmove(temp_stream, stream_info, sizeof(PmtStreamInfo));
    temp_stream->next = NULL;

    if(pmt_info->stream_info == NULL)
    {
        pmt_info->stream_info = temp_stream;
    }
    else
    {
        temp = pmt_info->stream_info;
        while(temp->next != NULL)
            temp = temp->next;

        temp->next = temp_stream;
    }

    return GXCORE_SUCCESS;
}

TsParseRet pmt_section_proc(char *section_data, void *ret_data)
{
    unsigned short service_id = 0xffff;
    int sec_length = 0;
    int prog_info_length = 0;
    int es_info_length = 0;
    int temp_length = 0;
    PMTInfo *pmt_info = (PMTInfo*)ret_data;
    char *ptr = NULL;
    PmtStreamInfo stream_info;
    DescriptorInfo des_info;
    char *des_ptr = NULL;

    if(section_data == NULL)
        return TS_PARSE_CONTINUE;

    if(ret_data == NULL)
        return TS_PARSE_OVER;

    if(section_data[0] != 0x02)//pmt table id
    {
        app_log_debug("[%s]%d %x not pmt table id !!! \n", __func__, __LINE__, section_data[0]);
        return TS_PARSE_CONTINUE;
    }

    service_id = ((unsigned short)section_data[3]) << 8 | section_data[4];
    if(service_id != pmt_info->service_id)
    {
        app_log_debug("[%s]%d prog num %d not match pmt %d !!! \n", __func__, __LINE__, pmt_info->service_id, service_id);
        return TS_PARSE_CONTINUE;
    }

    sec_length = ((unsigned short)(section_data[1] & 0xf)) << 8 | section_data[2];
    sec_length += 3;
    pmt_info->pcr_pid = ((unsigned short)(section_data[8] & 0x1f)) << 8 | section_data[9];
    prog_info_length = ((unsigned short)(section_data[10] & 0x0f)) << 8 | section_data[11];

    temp_length = sec_length - prog_info_length - 16;
    ptr = section_data + 12 + prog_info_length;

    if(prog_info_length > 0)
    {
        des_ptr = section_data + 12;
        while(prog_info_length > 2)
        {
            memset(&des_info, 0, sizeof(DescriptorInfo));
            des_info.tag = des_ptr[0];
            des_info.length = des_ptr[1];
            des_info.data = (char*)GxCore_Calloc(des_info.length, 1);
            if(des_info.data != NULL)
            {
                memmove(des_info.data, des_ptr + 2, des_info.length);
                pmt_add_prog_info(pmt_info, &des_info);
            }
            des_ptr += (des_info.length + 2);
            prog_info_length -= (des_info.length + 2);
        }
    }

    while(temp_length >= 5)
    {
        memset(&stream_info, 0, sizeof(PmtStreamInfo));
        stream_info.type = ptr[0];
        stream_info.pid = ((unsigned short)(ptr[1] & 0x1f)) << 8| ptr[2];
        es_info_length = ((unsigned short)(ptr[3] & 0x0f)) << 8 | ptr[4];

        des_ptr = ptr + 5;

        ptr += (es_info_length + 5);
        temp_length -= (es_info_length + 5);

        while(es_info_length > 2)
        {
            memset(&des_info, 0, sizeof(DescriptorInfo));
            des_info.tag = des_ptr[0];
            des_info.length = des_ptr[1];
            des_info.data = (char*)GxCore_Calloc(des_info.length, 1);
            if(des_info.data != NULL)
            {
                memmove(des_info.data, des_ptr + 2, des_info.length);
                pmt_stream_add_es_info(&stream_info, &des_info);
            }
            des_ptr += (des_info.length + 2);
            es_info_length -= (des_info.length + 2);
        }
        pmt_add_stream_info(pmt_info, &stream_info);
    }

    return TS_PARSE_OVER;
}

static void pmt_delete_descriptor(DescriptorInfo *des_info)
{
    DescriptorInfo *temp = NULL;

    while(des_info != NULL)
    {
        GxCore_Free(des_info->data);
        temp = des_info;
        des_info = des_info->next;
        GxCore_Free(temp);
    }
}

static void pmt_delete_stream_info(PmtStreamInfo *stream_info)
{
    PmtStreamInfo *temp = NULL;

    while(stream_info != NULL)
    {
        pmt_delete_descriptor(stream_info->es_info);
        temp = stream_info;
        stream_info = stream_info->next;
        GxCore_Free(temp);
    }
}

void pmt_data_release(PMTInfo *pmt_info)
{
    if(pmt_info == NULL)
        return;

    pmt_delete_descriptor(pmt_info->prog_info);
    pmt_delete_stream_info(pmt_info->stream_info);

    memset(pmt_info, 0 , sizeof(PMTInfo));
}

status_t app_parse_pmt_private(PMTInfo *pmt_info, void *private_data)
{
    DescriptorInfo *pmt_private = NULL;
    if(private_data == NULL || pmt_info == NULL)
        return GXCORE_ERROR;
    pmt_private = pmt_info->prog_info;
    while(pmt_private != NULL)
    {
        switch(pmt_private->tag)
        {
            case PMTINFO_PRIVATE_TAG:
                memcpy(private_data, pmt_private->data,pmt_private->length);
                break;
        }
        pmt_private = pmt_private->next;
    }
    return GXCORE_SUCCESS;
}

status_t app_parse_pmt_audio(PMTInfo *pmt_info, AudioLang_t *audio)
{
    PmtStreamInfo *pmt_stream = NULL;
    DescriptorInfo *es_info = NULL;
    unsigned int cnt = 0;
    AudioList_t *a_list = NULL;
    AudioList_t *a_temp = NULL;

    if((pmt_info == NULL) || (audio == NULL))
        return GXCORE_ERROR;

    pmt_stream = pmt_info->stream_info;
    while(pmt_stream != NULL)
    {
        a_temp = NULL;
        switch(pmt_stream->type)
        {
            case 0x03://MPEG2_AUDIO1:
            case 0x04://MPEG2_AUDIO2:
            case 0x0f://AAC_ADTS:
            case 0x11://AAC_LATM:
                a_temp = (AudioList_t*)GxCore_Realloc(a_list, (cnt + 1) * sizeof(AudioList_t));
                if(a_temp != NULL)
                {
                    a_list = a_temp;
                    a_temp = a_list + cnt;

                    if(pmt_stream->type == 0x03)
                        a_temp->audiotype = AUDIO_CODEC_MPEG1;
                    else if(pmt_stream->type == 0x04)
                        a_temp->audiotype = AUDIO_CODEC_MPEG2;
                    else if(pmt_stream->type == 0x0f)
                        a_temp->audiotype = AUDIO_CODEC_AAC_ADTS;
                    else if(pmt_stream->type == 0x11)
                        a_temp->audiotype = AUDIO_CODEC_AAC_LATM;

                    a_temp->id = pmt_stream->pid;
                    a_temp->name = NULL;

                    es_info = pmt_stream->es_info;
                    while(es_info != NULL)
                    {
                        if(es_info->tag == 0x0a) //iso639
                        {
                            a_temp->name = (char*)GxCore_Calloc(3, 1);
                            if(a_temp->name != NULL)
                            {
                                memmove(a_temp->name, es_info->data, 3);
                            }
                            break;
                        }
                        es_info = es_info->next;
                    }

                    cnt++;
                }
                break;

            case 0x06: //PES
            {
                bool IsDra = false;
                es_info = pmt_stream->es_info;
                while(es_info != NULL)
                {
                    //try to find dra audio type
                    if(es_info->tag == 0x05 && es_info->length == 4) //private descriptor
                    {
                        char *DraTag = NULL;
                        DraTag = (char*)GxCore_Malloc(sizeof(char) * (es_info->length+1));
                        memset(DraTag, 0, (es_info->length+1));
                        strncpy(DraTag, es_info->data, es_info->length);
                        if(strcmp(DraTag, "DRA1") == 0)
                            IsDra = true;
                        APP_FREE(DraTag);
                    }
                    if((es_info->tag == 0x6a) || (es_info->tag == 0x7a) || IsDra)
                    {
                        a_temp = (AudioList_t*)GxCore_Realloc(a_list, (cnt + 1) * sizeof(AudioList_t));
                        break;
                    }
                    es_info = es_info->next;
                }

                if(a_temp != NULL)
                {
                    a_list = a_temp;
                    a_temp = a_list + cnt;

                    if(es_info->tag == 0x6a)
                        a_temp->audiotype = AUDIO_CODEC_AC3;
                    else if(es_info->tag == 0x7a)
                        a_temp->audiotype = AUDIO_CODEC_EAC3;
                    else if(IsDra)
                        a_temp->audiotype = AUDIO_CODEC_DRA1;

                    a_temp->id = pmt_stream->pid;
                    a_temp->name = NULL;

                    es_info = pmt_stream->es_info;
                    while(es_info != NULL)
                    {
                        if(es_info->tag == 0x0a) //iso639
                        {
                            a_temp->name = (char*)GxCore_Calloc(3, 1);
                            if(a_temp->name != NULL)
                            {
                                memmove(a_temp->name, es_info->data, 3);
                            }
                            break;
                        }
                        es_info = es_info->next;
                    }

                    cnt++;
                }
            }
                break;

            default:
                break;
        }

        pmt_stream = pmt_stream->next;
    }

    audio->audioLangList = a_list;
    audio->audioLangCnt = cnt;

    return GXCORE_SUCCESS;
}

status_t app_parse_pmt_subt(PMTInfo *pmt_info, ttx_subt **subt, int *subt_num)
{
    PmtStreamInfo *pmt_stream = NULL;
    DescriptorInfo *es_info = NULL;
    unsigned int cnt = 0;
    ttx_subt *subt_list = NULL;
    ttx_subt *subt_temp = NULL;

    if((pmt_info == NULL) || (subt == NULL) || (subt_num == NULL))
        return GXCORE_ERROR;

    pmt_stream = pmt_info->stream_info;
    while(pmt_stream != NULL)
    {
        subt_temp = NULL;
        if(pmt_stream->type == 0x06)
        {
            es_info = pmt_stream->es_info;
            while(es_info != NULL)
            {
                if(es_info->tag == 0x59)
                {
                    subt_temp = (ttx_subt*)GxCore_Realloc(subt_list, (cnt + 1) * sizeof(ttx_subt));
                    if(subt_temp != NULL)
                    {
                        subt_list = subt_temp;
                        subt_temp = subt_list + cnt;
                        subt_temp->elem_pid = pmt_stream->pid;
                        memmove(subt_temp->lang, es_info->data, 3);
                        subt_temp->type = es_info->data[3];
                        subt_temp->subt.composite_page_id = (es_info->data[4] << 8) | es_info->data[5];
                        subt_temp->subt.ancillary_page_id = (es_info->data[6] << 8) | es_info->data[7];

                        cnt++;
                    }

                    break;
                }
                es_info = es_info->next;
            }
        }

        pmt_stream = pmt_stream->next;
    }

    *subt = subt_list;
    *subt_num = cnt;

    return GXCORE_SUCCESS;
}

#if 0
status_t app_parse_pmt_ttx(PMTInfo *pmt_info, ttx_subt **ttx, int *ttx_num)
{
    PmtStreamInfo *pmt_stream = NULL;
    DescriptorInfo *es_info = NULL;
    unsigned int cnt = 0;
    ttx_subt *ttx_list = NULL;
    ttx_subt *ttx_temp = NULL;

    if((pmt_info == NULL) || (ttx == NULL) || (ttx_num == NULL))
        return GXCORE_ERROR;

    pmt_stream = pmt_info->stream_info;
    while(pmt_stream != NULL)
    {
        ttx_temp = NULL;
        if(pmt_stream->type == 0x06)
        {
            es_info = pmt_stream->es_info;
            while(es_info != NULL)
            {
                if(es_info->tag == 0x56)
                {
                    ttx_temp = (ttx_subt*)GxCore_Realloc(ttx_list, (cnt + 1) * sizeof(ttx_subt));
                    if(ttx_temp != NULL)
                    {
                        ttx_list = ttx_temp;
                        ttx_temp = ttx_list + cnt;
                        ttx_temp->elem_pid = pmt_stream->pid;
                        memmove(ttx_temp->lang, es_info->data, 3);
                        ttx_temp->type = (es_info->data[3] >> 3) & 0x1f;
                        ttx_temp->ttx.magazine = es_info->data[3] & 0x07;
                        ttx_temp->ttx.page = es_info->data[4];

                        cnt++;
                    }

                    break;
                }
                es_info = es_info->next;
            }
        }

        pmt_stream = pmt_stream->next;
    }

    *ttx = ttx_list;
    *ttx_num = cnt;

    return GXCORE_SUCCESS;
}
#endif

status_t app_parse_pmt_video(PMTInfo *pmt_info, unsigned short *video_pid, VideoCodecType *video_type)
{
    PmtStreamInfo *pmt_stream = NULL;

    if((pmt_info == NULL) || (video_pid == NULL) || (video_type == NULL))
        return GXCORE_ERROR;

    pmt_stream = pmt_info->stream_info;
    while(pmt_stream != NULL)
    {
        if((pmt_stream->type == 0x01)
            || (pmt_stream->type == 0x02)
            || (pmt_stream->type == 0x10)
            || (pmt_stream->type == 0x1b)
            || (pmt_stream->type == 0x24)
            || (pmt_stream->type == 0x42))
        {
            *video_pid = pmt_stream->pid;
            if((pmt_stream->type == 0x01) || (pmt_stream->type == 0x02))
                *video_type = VIDEO_CODEC_MPEG12;
            if(pmt_stream->type == 0x10)
                *video_type = VIDEO_CODEC_MPEG4;
            if(pmt_stream->type == 0x1b)
                *video_type = VIDEO_CODEC_H264;
            if(pmt_stream->type == 0x24)
                *video_type = VIDEO_CODEC_H265;
            if(pmt_stream->type == 0x42)
                *video_type = VIDEO_CODEC_AVS;
            break;
        }

        pmt_stream = pmt_stream->next;
    }

    return GXCORE_SUCCESS;
}


