#include "app.h"
#if RICHEPG_SUPPORT
#include "richepg.h"
#include "app_cover_pvr.h"
#include "richepg_common.h"
#include "app_eutel_common.h"
#include "richepg_file_splice.h"
#include "app_cover_pvr_capture_poster.h"

/**** full_screen record will save epg information ****/

#define CAPTURE_WAIT_SEC    30

static PvrRecordInfo s_pvr_file_info;

static int _eutel_pvr_file_info_free(void)
{
    PvrRecordInfo *info = &s_pvr_file_info;

    APP_FREE(info->dir_path);
    APP_FREE(info->info_path);
    APP_FREE(info->logo_path);
    APP_FREE(info->chlogo_path);
    APP_FREE(info->index_path);
    APP_FREE(info->ori_logo_path);
    APP_FREE(info->ori_chlogo_path);
    app_cover_pvr_free_epg_record(&info->record);
    memset(info, 0, sizeof(PvrRecordInfo));

    return 0;
}

char *app_eutel_pvr_cur_event_name_get(void)
{
    PvrRecordInfo *info = &s_pvr_file_info;

    if (info->save_flag != PVR_RECORD_SAVE_ALL)
        return NULL;
    return info->record.name;
}

int app_eutel_pvr_file_info_save(void)
{
    extern void eutel_strlink(char *dst, char *src, int dst_size);
    PvrRecordInfo *info = &s_pvr_file_info;
    int duration = app_tick_time_sec_get() - info->start_sec;
    char genres_name[256] = {0};
    int i = 0;

    if (!info->save_flag)
        goto end;

    app_capture_vpp_stop();
    if (info->ori_logo_path)
    {
        if( richepg_file_splice_special_path_check(info->ori_logo_path) )
        {
            richepg_files_splice_copy_special_file_to_file(info->ori_logo_path,info->logo_path);
        }
        else
        {
            richepg_copy_file_to_file(info->ori_logo_path, info->logo_path, 8192);
        }
    }
    if (info->ori_chlogo_path)
    {
        if( richepg_file_splice_special_path_check(info->ori_chlogo_path) )
        {
            richepg_files_splice_copy_special_file_to_file(info->ori_chlogo_path,info->chlogo_path);
        }
        else
        {
             richepg_copy_file_to_file(info->ori_chlogo_path, info->chlogo_path, 8192);
        }
    }

    // index.json
    info->record.duration = duration;
    if (app_cover_pvr_save_epg_record_info(info->info_path, info->index_path, &info->record) < 0)
    {
        if (GxCore_FileExists(info->info_path) == GXCORE_FILE_EXIST)
            GxCore_FileDelete(info->info_path);
    }

    for (i = 0; i < info->record.contents_num; i++)
    {
        if (info->record.contents[i])
        {
            eutel_strlink(genres_name, info->record.contents[i], sizeof(genres_name));
        }
    }

    // collect info
    if (!info->record.from_freescan)
    {
        app_eutel_collect_add_item_rec_add(info->channel_id, duration,
                info->record.name != NULL ? info->record.name : info->record.chname, strlen(genres_name) > 0 ? genres_name : NULL);
    }
    app_eutel_collect_update_dialog();

    _eutel_pvr_file_info_free();
    return 0;
end:
    _eutel_pvr_file_info_free();
    return -1;
}

int app_eutel_pvr_file_info_update(const char *path, const char *name)
{
    EutelChannelCtrl *ch_ctrl = &s_eutel_ch_ctrl;
    EutelChannelArg *ch_arg = NULL;
    PvrRecordInfo *info = &s_pvr_file_info;
    RichepgEpgEvent *epg_info = NULL;
    char *content = NULL;
    char *ori_real_logo = NULL;
    char realpath[256] = {0};
    GxMsgProperty_NodeByPosGet node = {0};

    GxTime time = {0};
    int sel = 0;
    int i = 0;

    _eutel_pvr_file_info_free();
    if (!path || !name)
        return -1;
    if (app_richepg_mode_get())
    {
        sel = eutel_channel_play_use_sel_get();
        if (sel < 0)
            return -1;
        ch_arg = &ch_ctrl->arg_array[ch_ctrl->use_array[sel].pos];
    }
    else
    {
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        if(app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)) != GXCORE_SUCCESS)
            return -1;
    }

    // path
    if ((info->dir_path = app_cover_pvr_get_full_name(path, strlen(path), name, strlen(name)-PVR_FIX_SIFFIX_LEN)) == NULL)
        goto end;
    if ((info->info_path = app_cover_pvr_get_full_name2(info->dir_path, PVR_JSON_NAME)) == NULL)
        goto end;
    if ((info->logo_path = app_cover_pvr_get_full_name2(info->dir_path, PVR_LOGO_NAME)) == NULL)
        goto end;
    if ((info->chlogo_path = app_cover_pvr_get_full_name2(info->dir_path, PVR_CHLOGO_NAME)) == NULL)
        goto end;
    if ((info->index_path = app_cover_pvr_get_full_name2(path, PVR_INDEX_NAME)) == NULL)
        goto end;
    if (GxCore_FileExists(info->dir_path) != GXCORE_FILE_EXIST)
    {
        if (GxCore_Mkdir(info->dir_path) == GXCORE_ERROR)
            goto end;
    }

    //ch info
    GxCore_GetLocalTime(&time);
    info->start_sec = app_tick_time_sec_get();
    info->record.recstart = time.seconds;
    info->record.fname = GxCore_Strdup(name);
    if (app_richepg_mode_get())
    {
        info->channel_id = ch_arg->channel_id;
        info->record.chname = GxCore_Strdup(ch_arg->channel_name);
        ori_real_logo = richepg_get_logo_path(ch_arg->channel_logo, RICHEPG_LOGO_CHANNEL);
        if (!ori_real_logo)
        {
            if (richepg_store_getstr(RICHEPG_LINEUP_LOGO, realpath, sizeof(realpath)) != NULL)
            {
                if (access(realpath, F_OK) == 0)
                {
                    ori_real_logo = realpath;
                }
            }
        }
        if (ori_real_logo)
            info->ori_chlogo_path = GxCore_Strdup(ori_real_logo);
        info->save_flag = PVR_RECORD_SAVE_CH;
    }
    else
    {
        info->record.chname = GxCore_Strdup((char *)node.prog_data.prog_name);
        info->record.from_freescan = 1;

        info->record.contents = GxCore_Calloc(1, sizeof(char*));
        if ( info->record.contents != NULL )
        {
            info->record.contents[0] = GxCore_Strdup(STR_ID_FREE_SCAN_TAG);
            if ( info->record.contents[0] != NULL )
                info->record.contents_num = 1;
        }

        app_capture_vpp_start(info->logo_path, CAPTURE_WAIT_SEC);
        info->save_flag = PVR_RECORD_SAVE_CH;
        return 0;
    }

    // epg info
    if (((epg_info = app_eutel_epg_info_get()) == NULL)
            || (richepg_epg_event_info_get_by_time(time.seconds + 30, ch_arg->channel_id, epg_info, EUTEL_EPG_EVENT_SIZE, true) < 0))
    {
        app_capture_vpp_start(info->logo_path, CAPTURE_WAIT_SEC);
        return 0;
    }

    // handle epg info
    info->record.starttime = epg_info->start_time;
    info->record.finishtime = epg_info->finish_time;
    if (epg_info->genres_num > 0 &&
            (info->record.contents = GxCore_Calloc(epg_info->genres_num, sizeof(char*))) != NULL)
    {
        for (i = 0; i < epg_info->genres_num; i++)
        {
            content = eutel_genre_name_get(epg_info->genres_ids[i], EUTEL_CONTENT_GENRE);
            if (content && (info->record.contents[info->record.contents_num] = GxCore_Strdup(content)) != NULL)
                info->record.contents_num++;
        }
    }
    info->record.parental = epg_info->parent_rate;
    if (epg_info->event_title)
        info->record.name = GxCore_Strdup(epg_info->event_title);
    if (epg_info->event_desc)
        info->record.description = GxCore_Strdup(epg_info->event_desc);
    else
        info->record.description = app_richepg_translate_str(info->record.from_freescan ? STR_ID_FREE_SCAN_RECORD : STR_ID_NO_INFO);

    ori_real_logo = richepg_get_logo_path(epg_info->event_logo, RICHEPG_LOGO_EVENT);
    if (ori_real_logo)
        info->ori_logo_path = GxCore_Strdup(ori_real_logo);
    else
        app_capture_vpp_start(info->logo_path, CAPTURE_WAIT_SEC);

    info->save_flag = PVR_RECORD_SAVE_ALL;
    return 0;
end:
    _eutel_pvr_file_info_free();
    return -1;
}
#endif
