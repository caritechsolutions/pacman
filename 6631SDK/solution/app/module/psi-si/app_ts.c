#include "app.h"
#include "module/app_psi.h"
#include "module/app_dir_module.h"

#define APP_TS_LOG
#undef APP_TS_LOG

#define TS_PACK_LENGTH    (188)
#define TS_HEADER_LENGTH    (4)
#define TS_PAYLOAD_LENGTH    (184)
#define MAX_INVALID_TS_PACKAT    (2048)
#define TS_READ_CACHE       (188*23)

static char *s_section_data = NULL;
static char *s_section_ptr = NULL;
static int s_section_length = 0;
static char s_continuity_counter = 0xff;
static int s_ts_cnt = 0;

status_t app_ts_data_free(AppTsData *ts_data)
{
    int i = 0;
    if(ts_data == NULL)
        return GXCORE_ERROR;

    if(ts_data->pack_data != NULL)
    {
        for(i = 0; i < ts_data->pack_num; i++)
        {
            if(ts_data->pack_data[i] != NULL)
            {
                GxCore_Free(ts_data->pack_data[i]);
                ts_data->pack_data[i] = NULL;
            }
        }
        GxCore_Free(ts_data->pack_data);
        ts_data->pack_data = NULL;
    }
    ts_data->pack_num = 0;

    return GXCORE_SUCCESS;
}

status_t app_ts_pack(unsigned short pid, char *payload_data, int data_length, AppTsData *ts_data)
{
    bool empty_flag = false;
    int cp_len = 0;
    int pack_num = 1;
    int i = 0;
    int j = 0;
    char **pack_buff = NULL;
    char payload_unit_start = 0;

    if(ts_data == NULL)
        return GXCORE_ERROR;

    if((pid == 0x1fff) || (payload_data == NULL) || (data_length == 0))
    {
        empty_flag = true;
    }

    if(data_length > TS_PAYLOAD_LENGTH - 1)
        pack_num++;

    pack_num += (data_length - (TS_PAYLOAD_LENGTH - 1)) / TS_PAYLOAD_LENGTH;
    pack_buff = (char**)GxCore_Calloc(pack_num, sizeof(char*));
    if(pack_buff == NULL)
        return GXCORE_ERROR;

    ts_data->pack_num = pack_num;
    ts_data->pack_data = pack_buff;
    for(i = 0; i < pack_num; i++)
    {
        pack_buff[i] = (char*)GxCore_Calloc(TS_PACK_LENGTH, sizeof(char));
        if(pack_buff[i] == NULL)
            goto error_ret;

        memset(pack_buff[i], 0xff, TS_PACK_LENGTH);
        if((i == 0) && (empty_flag == false))
        {
            payload_unit_start = 1;
        }

        j = 0;
        pack_buff[i][j++] = 0x47; //syn_byte
        pack_buff[i][j] = (payload_unit_start << 6);
        pack_buff[i][j++] |= (pid >> 8) & 0x1f;
        pack_buff[i][j++] = (pid & 0xff);

        pack_buff[i][j] = 0x10; //adaption
        pack_buff[i][j++] |= (i % 16) & 0x0f; //continuity_counter

        if(payload_unit_start  == 1)
        {
            pack_buff[i][j++] = 0x00; //pointer_field
            (data_length > TS_PAYLOAD_LENGTH - 1) ? (cp_len = TS_PAYLOAD_LENGTH - 1) : (cp_len = data_length);
        }
        else
        {
            (data_length > TS_PAYLOAD_LENGTH) ? (cp_len = TS_PAYLOAD_LENGTH) : (cp_len = data_length);
        }

        if(cp_len > 0)
        {
            memmove(&pack_buff[i][j], payload_data, cp_len);
            data_length -= cp_len;
            payload_data += cp_len;
        }

#ifdef APP_TS_LOG
        int cnt = 0;
        app_log_debug("\n######### TS Pack %d #########\n", i);
        for(cnt = 0; cnt < 188; cnt++)
        {
            app_log_debug("%02x ", pack_buff[i][cnt]);
            if(cnt % 16 == 15)
                app_log_debug("\n");
        }
        app_log_debug("\n##############################\n");
#endif
    }

    return GXCORE_SUCCESS;

error_ret:
    app_ts_data_free(ts_data);

    return GXCORE_ERROR;
}

static void app_tsfile_parse_init(void)
{
    if(s_section_data != NULL)
    {
        GxCore_Free(s_section_data);
        s_section_data = NULL;
    }

    s_section_ptr = NULL;
    s_section_length = 0;
    s_continuity_counter = 0xff;
    s_ts_cnt = 0;
}

static TsParseRet app_ts_collect(char *ts_data, SectionParseCb *section_cb)
{
    char continuity_counter;
    char expected_counter;
    int payload_pos = 0;
    bool payload_unit_start = false;
    int payload_length;
    TsParseRet ret = TS_PARSE_CONTINUE;

    s_ts_cnt++;
    if(ts_data == NULL)
        return TS_PARSE_CONTINUE;

    if(ts_data[0] != 0x47)
        return TS_PARSE_CONTINUE;

    //Continuity check
    continuity_counter = ts_data[3] & 0xf;
    if(s_continuity_counter != 0xff) //not first packat
    {
        expected_counter = (s_continuity_counter + 1) & 0xf;
        if (expected_counter != continuity_counter)
        {
            //err counter
            return TS_PARSE_CONTINUE;
        }
    }
    s_continuity_counter = continuity_counter;

    //Return if no payload in the TS packet
    if (!(ts_data[3] & 0x10))
        return TS_PARSE_CONTINUE;

    //Skip the adaptation_field if present
    if (ts_data[3] & 0x20)
        payload_pos = ts_data[4] + 5;
    else
        payload_pos = 4;


    //Unit start -> skip the pointer_field and a new section begins
    if (ts_data[1] & 0x40)
    {
        payload_unit_start = true;
        payload_pos += 1;
    }

#ifdef APP_TS_LOG
    int cnt = 0;
    app_log_debug("\n######### TS Pack #########\n");
    for(cnt = 0; cnt < 188; cnt++)
    {
        app_log_debug("%02x ", ts_data[cnt]);
        if(cnt % 16 == 15)
            app_log_debug("\n");
    }
    app_log_debug("\n##############################\n");
#endif

    payload_length = 188 - payload_pos;
    while(payload_length > 0)
    {
        if (s_section_data == NULL)
        {
            if(payload_unit_start == true)
            {
                //new section
                s_section_length = ((unsigned short)(ts_data[payload_pos + 1] & 0xf)) << 8 | ts_data[payload_pos + 2];
                s_section_length += 3;
                s_section_data = (char*)GxCore_Calloc(s_section_length, 1);

                if (s_section_data == NULL)
                    return TS_PARSE_CONTINUE;

                s_section_ptr = s_section_data;
            }
            else
            {
                return TS_PARSE_CONTINUE;
            }
        }

        s_ts_cnt = 0;
        if (payload_length >= s_section_length)
        {

            //There are enough bytes in this packet to complete the header/section
            memmove(s_section_ptr, ts_data + payload_pos, s_section_length);
            payload_pos += s_section_length;
            s_section_ptr += s_section_length;
            payload_length -= s_section_length;
            ret = TS_PARSE_OVER;

#ifdef APP_TS_LOG
            int cnt = 0;
            s_section_length = ((unsigned short)(s_section_data[1] & 0xf)) << 8 | s_section_data[2];
            s_section_length += 3;
            app_log_debug("\n######### section %d #########\n", s_section_length);
            for(cnt = 0; cnt < s_section_length; cnt++)
            {
                app_log_debug("%02x ", s_section_data[cnt]);
                if(cnt % 16 == 15)
                    app_log_debug("\n");
            }
            s_section_length = 0;
            app_log_debug("\n##############################\n");
#endif
            if(section_cb->cb_proc != NULL)
            {
                ret = section_cb->cb_proc(s_section_data, section_cb->cb_data);
            }
            GxCore_Free(s_section_data);
            s_section_data = NULL;

            if(ret == TS_PARSE_OVER)
            {
       //         s_ts_cnt = 0;
                break;
            }
            else
            {
                return TS_PARSE_CONTINUE;
            }

            //left data
        }
        else
        {
            /* There aren't enough bytes in this packet to complete the
               header/section */
            memmove(s_section_ptr, ts_data + payload_pos, payload_length);
            s_section_ptr += payload_length;
            s_section_length -= payload_length;
            payload_length = 0;
            ret = TS_PARSE_CONTINUE;
        }
    }
    return ret;
}

status_t app_tsfile_section_parse(const char *ts_file, unsigned short pid, SectionParseCb *section_cb)
{
    int i  = 0;
    char ts_data[188] = {0};
    unsigned short temp_pid;
    status_t ret = GXCORE_ERROR;
    PlayerRecordConfig* config = NULL;
    if ((config = (PlayerRecordConfig*)GxCore_Calloc(1, sizeof(PlayerRecordConfig))) == NULL)
    {
        app_log_error("\n######### Calloc failed############\n");
        return ret;
    }
    if(GxPlayer_MediaGetRecordConfigByUrl(ts_file, config) == GXCORE_ERROR)
    {
        app_log_error("\n#########%s %d get record config failed############\n",__FILE__,__LINE__);
        GxCore_Free(config);
        return ret;
    }

    app_tsfile_parse_init();

    i = 0;
    while(i < config->ext_data.userdata_len)
    {
        if(config->ext_data.userdata[i] == 0x47)
        {
            memcpy(ts_data,&(config->ext_data.userdata[i]),sizeof(ts_data));
            temp_pid = ((unsigned short)(ts_data[1] & 0x1f) << 8) | ts_data[2];
            if(temp_pid == pid)
            {
                if(app_ts_collect(ts_data, section_cb) == TS_PARSE_OVER)
                {
                    ret = GXCORE_SUCCESS;
                    break;
                }
            }
        }
        i=i+188;
    }
    GxCore_Free(config);
    return ret;
}

void app_tsfile_test(void)
{
    SectionParseCb pat_parse_cb;
    PATInfo pat_info = {0};
    SectionParseCb pmt_parse_cb;
    PMTInfo pmt_info = {0};

    AudioLang_t audio_list = {0};
    ttx_subt *subt = NULL;
    int subt_num;
        pat_parse_cb.cb_data = (void*)&pat_info;
    pat_parse_cb.cb_proc = pat_section_proc;
    app_tsfile_section_parse("/dvb/0000.ts", 0, &pat_parse_cb);

    pmt_parse_cb.cb_data = (void*)&pmt_info;
    pmt_parse_cb.cb_proc = pmt_section_proc;
    app_tsfile_section_parse("/dvb/0000.ts", pat_info.prog_data[0].pmt_pid, &pmt_parse_cb);

    app_parse_pmt_audio(&pmt_info, &audio_list);
    app_audio_lang_free(&audio_list);

    app_parse_pmt_subt(&pmt_info, &subt, &subt_num);
    if(subt != NULL)
    {
        GxCore_Free(subt);
        subt = NULL;
    }

#if 0
    ttx_subt *ttx = NULL;
    int ttx_num;

    app_parse_pmt_ttx(&pmt_info, &ttx, &ttx_num);
    if(ttx != NULL)
    {
        GxCore_Free(ttx);
        ttx = NULL;
    }
#endif

    pat_data_release(&pat_info);
    pmt_data_release(&pmt_info);
}

