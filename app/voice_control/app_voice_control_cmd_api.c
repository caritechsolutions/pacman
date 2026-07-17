#include "app.h"
#include "app_windows.h"

#if VOICE_CONTROL_SUPPORT
#include "app_volume_value.h"
#include "module/pm/gxpm_manage.h"
#include "module/channel_edit.h"
#include "app_wnd_system_setting_opt.h"
#include "module/app_bat.h"
#include "voice_control/include/app_voice_control_api.h"
// mute control
#include "full_screen.h"
#if TKGS_SUPPORT
extern bool app_tkgs_get_upgrade_finish_pop_status(void);
#endif

int app_check_unsupported_window(void)
{
    if((GXCORE_SUCCESS == GUI_CheckDialog(WND_KEYBOARD_LANGUAGE))
        ||(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_TIP))
        ||(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_SORT))
        ||(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_FAV))
        ||(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_BOOK))
        ||(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_DIALOG))
        ||(app_vc_osdttx_flag_get())
      )
    {
        return STATUS_TYPE_WINDOWN_UNSUPPORTED;
    }
    return STATUS_TYPE_CAN_SUPPORT;
}
// ret: 0-success, 1-repeat set, -1-not support
int app_vc_set_mute_quick_ex(char exec_flag, char is_mute)
{
    int ret = 0;
    FullArb *arb = &g_AppFullArb;
    char *img_name = NULL;
    if(((1 == is_mute) && (arb->state.mute == STATE_ON))
            ||(g_AppChOps.get_list_total() == 0)
            || ((1 != is_mute) && (arb->state.mute != STATE_ON))
      )
    {
        return STATUS_TYPE_NOT_SUPPORT;
    }

    if(exec_flag)
    {
        if(STATE_ON != is_mute)
        {
            GxPlayer_SetAudioMute(0);
        }
        else
        {
            GxPlayer_SetAudioMute(1);
        }

    }
    arb->state.mute = (is_mute? STATE_ON:STATE_OFF);
    // draw
    //
    if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MUSIC_VIEW))
    {
        img_name = "music_view_imag_mute";
    }
    else if(GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW))
    {
        img_name = "movie_view_imag_mute";
    }
    else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_EPG_PLAYBACK))
    {
        img_name = "img_epg_playback_mute";
    }
    else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_PLUGIN_LISTVIEW_PLAY))
    {
        img_name = "img_plugin_listview_play_mute";
    }
    else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_PLAY))
    {
        img_name = "img_netvideo_play_mute";
    }
    else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_DLNA_DMR))
    {
        img_name = "img_dlna_dmr_mute";
    }
#ifdef FM_SUPPORT
    else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_FM_PLAY))
    {
        img_name = "img_fm_play_mute";
    }
#endif
    else if((GXCORE_SUCCESS != GUI_CheckDialog(WND_MAIN_MENU))
                && (GXCORE_SUCCESS != GUI_CheckDialog(WND_CHANNEL_EDIT))
                && (GXCORE_SUCCESS != GUI_CheckDialog(WIN_MEDIA_CENTRE))
            )
    {// full screen
        img_name = "img_full_mute";
    }
    else
    {
        return STATUS_TYPE_CAN_SUPPORT;
    }

    if(img_name)
    {
        if (is_mute == STATE_ON)
        {
            GUI_SetProperty(img_name, "state", "show");
        }
        else
        {
            GUI_SetProperty(img_name, "state", "hide");
        }
        GUI_SetProperty(img_name, "draw_now", NULL);
    }
    return ret;
}


int app_vc_set_mute_quick(char is_mute)
{
    return app_vc_set_mute_quick_ex(0, is_mute);
}

// volume control
static int _volume_get(int *is_channel)
{
    int volume = 0;
    int flag = 0;
#if ( IPTV_LITE_SUPPORT > 0)
     extern int app_iptv_lite_check(void);
     if((app_volume_scope_status() && GUI_CheckDialog(WND_MAIN_MENU) == GXCORE_ERROR)
     &&(app_iptv_lite_check() == 0))
#else
    if(app_volume_scope_status() && GUI_CheckDialog(WND_MAIN_MENU) == GXCORE_ERROR)
#endif
    {
        GxMsgProperty_NodeByPosGet node = {0};

	    node.node_type = NODE_PROG;
	    node.pos = g_AppPlayOps.normal_play.play_count;
	    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
	    {
		    volume = node.prog_data.audio_volume;
	    }
        flag = 1;
	}
	else
	{
        volume = app_system_vol_get();
	}
     *is_channel = flag;
    return volume;
}

int app_vc_volume_quick_set_ex(int step, char direct_flag,  char play_flag, unsigned int *current_volume, unsigned int *max_volume)
{// step : (>0, volume+),(<0, volume-); save_flag: (0, need not play),(1, need play)
    FullArb *arb = &g_AppFullArb;
    int volume = 0;
    int flag = 0;
    int temp = 0;
    volume = _volume_get(&flag);
    *max_volume = MAX_DISPLAY_VOL;

    if(0 == direct_flag)
    {
        if((volume == (MAX_DISPLAY_VOL*VOLUME_NUM)) && (step > 0))
        {
            *current_volume = MAX_DISPLAY_VOL;
            return 1;
        }
        if((volume == 0) && (step < 0))
        {
            *current_volume = 0;
            return 2;
        }

        if(step > 0x10000000)
        {// 增加百分比音量，最小步进为1
            temp = ((step - 0x10000000)*volume/100);
            temp = ((temp > VOLUME_NUM)?temp:VOLUME_NUM);
            volume += temp;
        }
        else if(step < (int)0xF0000000)
        {// 减小百分比音量，最小步进为1
            temp = ((step + 0x10000000)*volume/100);
            temp = ((temp < (0-VOLUME_NUM))?temp:(0-VOLUME_NUM));
            volume += temp;
        }
        else
        {
            volume += (step*VOLUME_NUM);
        }
    }
    else
    {
        volume = (step*VOLUME_NUM);
    }
    if(volume > (MAX_DISPLAY_VOL*VOLUME_NUM))
    {
        volume = MAX_DISPLAY_VOL*VOLUME_NUM;
    }
    if(volume < 0)
    {
        volume = 0;
    }
    // set volume
    if(1 == play_flag)
        GxPlayer_SetAudioVolume(volume);

    // display save
    if(arb->state.mute == STATE_ON)
    {
        app_vc_set_mute_quick_ex(1, 0);
    }

    if(flag)// channel volume
    {
        GxMsgProperty_NodeByPosGet node = {0};
        GxMsgProperty_NodeModify node_modify = {0};
	    node.node_type = NODE_PROG;
	    node.pos = g_AppPlayOps.normal_play.play_count;
	    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
	    {
		    node_modify.node_type = NODE_PROG;
		    node_modify.prog_data = node.prog_data;
			node_modify.prog_data.audio_volume = volume;
		    app_send_msg_exec(GXMSG_PM_NODE_MODIFY, (void *)(&node_modify));
        }
    }
    else// global volume
    {
        app_system_vol_save(volume);
        app_system_vol_set_val(volume);
    }

    if(((volume/VOLUME_NUM == (MAX_DISPLAY_VOL - 1)) && (volume%VOLUME_NUM))
            || (volume/VOLUME_NUM == MAX_DISPLAY_VOL))
        *current_volume = MAX_DISPLAY_VOL;
    else
        *current_volume = volume/VOLUME_NUM;

    if(*current_volume == MAX_DISPLAY_VOL)
        return 1;
    else if(volume == 0)
        return 2;
    else
        return 0;
}

// ret: 0-success, 1-max, 2-min, -1-error
int app_vc_volume_quick_set(int step, char play_flag, unsigned int *current_volume, unsigned int *max_volume)
{// step : (>0, volume+),(<0, volume-); save_flag: (0, need not play),(1, need play)
    return app_vc_volume_quick_set_ex(step, 0, play_flag, current_volume, max_volume);
}

int app_vc_volume_quick_min(void)
{
    FullArb *arb = &g_AppFullArb;
    if(STATE_ON != arb->state.mute)
    {
        GxPlayer_SetAudioMute(1);
    }
    GxPlayer_SetAudioVolume(0);
    return 0;
}

int app_vc_volume_quick_resume(void)
{
    int volume = 0;
    int flag = 0;
    FullArb *arb = &g_AppFullArb;

    volume = _volume_get(&flag);
    GxPlayer_SetAudioVolume(volume);
    if(STATE_ON != arb->state.mute)
    {
        GxPlayer_SetAudioMute(0);
    }
    return 0;
}

static int _ui_sw_key_send(unsigned int stbk)
{
    return Gui_WriteKeyBuf(stbk);
}

static int _media_centre_play(int play_flag, int (*func)(void), int* stbk)
{
    int buf[3][3] = {{1,2,APPK_STOP}, {2,2,APPK_PAUSE}, {0,1,APPK_PLAY}};
    int flag = 0;

    if(-1 == func())
        return -1;
    flag = func();
    if((buf[play_flag][0] == flag)
            || (buf[play_flag][1] == flag))
    {
        *stbk = buf[play_flag][2];
    }
    else
    {
        return STATUS_TYPE_REPEAT;
    }
    return STATUS_TYPE_CAN_SUPPORT;
}

// play_flag: 0-stop; 1-pause; 2-play
// ret: 0-success; 1-not support; 2-repeat operator; -1=failed
int app_vc_set_play_quick(int play_flag)
{
    int ret = 0;
    int stbk = 0;
    char* focus_Window = (char*)GUI_GetFocusWindow();
    if((NULL == focus_Window) || (0 == strlen(focus_Window)) || (play_flag > 2) || (play_flag < 0))
        return -1;

    if(0 == strcmp(WIN_MUSIC_VIEW, focus_Window))
    {
        ret = _media_centre_play(play_flag, app_music_play_status_get, &stbk);
        if(ret)
            return ret;
    }
    else if((0 == strcmp(WIN_MOVIE_VIEW, focus_Window))
             || (0 == strcmp(WIN_MEDIA_BAR, focus_Window)))
    {
        ret = _media_centre_play(play_flag, app_movie_play_status_get, &stbk);
        if(ret)
            return ret;
    }
    else if(0 == strcmp(WIN_PIC_VIEW, focus_Window))
    {
        ret = _media_centre_play(play_flag, app_pic_play_status_get, &stbk);
        if(ret)
            return ret;
    }
    else if(0 == strcmp(WIN_FILE_BROWSER, focus_Window))
    {
        int buf[3] = {0,0,STBK_OK};
        stbk = buf[play_flag];
    }
    else if(0 == strcmp(WND_PVR_MEIDA, focus_Window))
    {
        int buf[3] = {0,0,STBK_OK};
        stbk = buf[play_flag];
    }
    else if(0 == strcmp(WND_COMMON_VIEW, focus_Window))
    {
        int buf[3] = {0,0,STBK_OK};
        stbk = buf[play_flag];
        if(stbk && (0 == app_vc_common_view_table_area_select()))
            stbk =0;
    }
    else if(0 == strcmp(WND_CHANNEL_EDIT, focus_Window))
    {// channel edit
        int buf[3] = {0,0,STBK_OK};
        if(0 == app_vc_channel_edit_mode_flag())
        {
            stbk = buf[play_flag];
        }
    }
    else if(0 == strcmp(WND_CHANNEL_LIST, focus_Window))
    {
        int buf[3] = {0,0,STBK_OK};
        ret = app_vc_channel_list_play_flag();
        if(0 == ret)
        {
            stbk = buf[play_flag];
        }
        else
        {
            return ret;
        }
    }
    else if((0 == strcmp(WND_FULL_SCREEN, focus_Window))
            || (0 == strcmp(WND_CHANNEL_INFO, focus_Window))
            || (0 == strcmp(WND_PVR_BAR, focus_Window))
            )
    {
        int buf[3][2] = {{-1,0},{0,STBK_PAUSE},{1,STBK_PAUSE}};
        if(buf[play_flag][0] < 0)
            return STATUS_TYPE_NOT_SUPPORT;
        ret = app_vc_fullscreen_reaction(buf[play_flag][0]);
        if(ret)
        {
            if(app_check_unsupported_window())
            {
                return STATUS_TYPE_WINDOWN_UNSUPPORTED;
            }
            else
            {
                return ret;
            }
        }
        stbk = buf[play_flag][1];
    }
    else if((0 == strcmp(WND_NETVIDEO_PLAY, focus_Window))
             || (0 == strcmp(WND_PLUGIN_PLAY_INFO, focus_Window)))
    {
        int buf[3] = {STBK_EXIT, APPK_PAUSE,APPK_PLAY};
        ret = app_vc_netvideo_play_flag(play_flag);
        if(0 == ret)
        {
            stbk = buf[play_flag];
            if(0 == strcmp(WND_PLUGIN_PLAY_INFO, focus_Window))
            {
                GUI_EndDialog(WND_PLUGIN_PLAY_INFO);
            }
        }
        else
        {
            return ret;
        }
    }
    else if(0 == strcmp(WND_COMMON_SELECT, focus_Window))
    {
        int buf[3] = {0,0,STBK_OK};
        stbk = buf[play_flag];

    }

    if(stbk != 0)
        _ui_sw_key_send(stbk);
    else
        return STATUS_TYPE_NOT_SUPPORT;// not support

    return ret;
}

// cmd: 0-prev channel, 1-next channel
// ret: 0-success; 1-not support; -1=failed
int app_vc_switch_channel_quick(int cmd)
{
    int stbk = 0;
    char* focus_Window = (char*)GUI_GetFocusWindow();
    if((NULL == focus_Window) || (0 == strlen(focus_Window)) || (cmd < 0) || (cmd > 1))
        return -1;

    if(0 == strcmp(WND_CHANNEL_LIST, focus_Window))
    {
        if(0 == app_vc_channel_list_switch_channel())
        {
            int buf[2] = {STBK_UP, STBK_DOWN};
            _ui_sw_key_send(buf[cmd]);
            stbk = STBK_OK;
        }
    }
    else if (0 == strcmp(WND_EPG, focus_Window))
    {
        int buf[2] = {STBK_UP, STBK_DOWN};
        focus_Window = (char*)GUI_GetFocusWidget();
        stbk = buf[cmd];
    }
    else if(0 == strcmp(WND_CHANNEL_EDIT, focus_Window))
    {
        if(0 == app_vc_channel_edit_switch_channel())
        {
            int buf[2] = {STBK_UP, STBK_DOWN};
            _ui_sw_key_send(buf[cmd]);
            stbk = STBK_OK;
        }
    }
    else if((0 == strcmp(WND_FULL_SCREEN, focus_Window))
            || (0 == strcmp(WND_CHANNEL_INFO, focus_Window))
            || (0 == strcmp(WND_PVR_BAR, focus_Window))
            )
    {
        int buf[2] = {STBK_DOWN, STBK_UP};
        stbk = buf[cmd];
    }
    else if(0 == strcmp(WIN_MUSIC_VIEW, focus_Window))
    {
        int buf[2] = {APPK_PREVIOUS,APPK_NEXT};
        stbk = buf[cmd];
    }
    else if((0 == strcmp(WIN_MOVIE_VIEW, focus_Window))
             || (0 == strcmp(WIN_MEDIA_BAR, focus_Window)))
    {
        int buf[2] = {APPK_PREVIOUS,APPK_NEXT};
        stbk = buf[cmd];
    }
    else if(0 == strcmp(WIN_PIC_VIEW, focus_Window))
    {
        int buf[2] = {APPK_PREVIOUS,APPK_NEXT};
        stbk = buf[cmd];
    }
    else if(0 == strcmp(WIN_FILE_BROWSER, focus_Window))
    {
        int buf[2] = {STBK_UP,STBK_DOWN};
        _ui_sw_key_send(buf[cmd]);
        stbk = STBK_OK;

    }
    else if(0 == strcmp(WND_PVR_MEIDA, focus_Window))
    {
        int buf[2] = {STBK_UP,STBK_DOWN};
        _ui_sw_key_send(buf[cmd]);
        stbk = STBK_OK;
    }
    else if(0 == strcmp(WND_COMMON_VIEW, focus_Window))
    {
        if((0 == strcmp("btn_common_view_item_page", GUI_GetFocusWidget()))
            || (0 == strcmp("listview_common_view_item", GUI_GetFocusWidget()))
          )
        {
            int buf[2] = {STBK_UP,STBK_DOWN};
            _ui_sw_key_send(buf[cmd]);
            stbk = STBK_OK;
        }
    }
    else if((0 == strcmp(WND_NETVIDEO_PLAY, focus_Window))
             || (0 == strcmp(WND_PLUGIN_PLAY_INFO, focus_Window)))
    {
        int buf[2] = {STBK_DOWN,STBK_UP};
        stbk = buf[cmd];
    }
    else if(0 == strcmp(WND_COMMON_SELECT, focus_Window))
    {
        int buf[2] = {STBK_UP,STBK_DOWN};
        _ui_sw_key_send(buf[cmd]);
        stbk = STBK_OK;
    }
    else if(0 == strcmp(WND_PASSWD, focus_Window))
    {
        if((GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_INFO))
           || (GXCORE_SUCCESS == GUI_CheckDialog(WND_FULL_SCREEN))
          )
        {
            int buf[2] = {STBK_DOWN,STBK_UP};
            stbk = buf[cmd];
        }
        else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_EPG))
        {
            int buf[2] = {STBK_UP,STBK_DOWN};
            stbk = buf[cmd];
        }
        else if((GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_LIST))
                || (GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT))
                )
        {
            int buf[2] = {STBK_UP,STBK_DOWN};
            _ui_sw_key_send(buf[cmd]);
            stbk = STBK_OK;
        }
        else if((GXCORE_SUCCESS == GUI_CheckDialog(WIN_MOVIE_VIEW))
                || (GXCORE_SUCCESS == GUI_CheckDialog(WIN_MEDIA_BAR)))
        {
            int buf[2] = {APPK_PREVIOUS,APPK_NEXT};
            stbk = buf[cmd];
            GUI_EndDialog(WND_PASSWD);
        }
    }


    if(stbk != 0)
        _ui_sw_key_send(stbk);
    else
        return STATUS_TYPE_NOT_SUPPORT;// not support

    return STATUS_TYPE_CAN_SUPPORT;
}
// cmd: 0-exit, 1-up,2-down,3-left,4-right,5-OK,6-cancel
// ret: 0-success; 1-not support; -1=failed
int app_vc_switch_basic_keypress(int key)
{
    int stbk = 0;
    char* focus_Window = (char*)GUI_GetFocusWindow();
    if ((NULL == focus_Window) || (0 == strlen(focus_Window)) || (key < 0) || (key > 6))
        return -1;

    // 如果在搜索窗口，只响应退出键
    if (GXCORE_SUCCESS == GUI_CheckDialog(WND_SEARCH)) {
        if (key == 0) {
            _ui_sw_key_send(STBK_EXIT);
            return STATUS_TYPE_CAN_SUPPORT;
        } else {
            return STATUS_TYPE_NOT_SUPPORT;
        }
    }

    // 如果在弹出对话框，只响应左、右和OK键
    if (GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_DIALOG)) {
        if (key == 3) {
            _ui_sw_key_send(STBK_LEFT);
            return STATUS_TYPE_CAN_SUPPORT;
        } else if (key == 4) {
            _ui_sw_key_send(STBK_RIGHT);
            return STATUS_TYPE_CAN_SUPPORT;
        } else if (key == 5) {
            _ui_sw_key_send(STBK_OK);
            return STATUS_TYPE_CAN_SUPPORT;
        } else {
            return STATUS_TYPE_NOT_SUPPORT;
        }
    }

    // 处理其他窗口的键响应
    switch (key) {
        case 0: // exit
        case 6:
            stbk = STBK_EXIT;
            break;
        case 1: // up
            stbk = STBK_UP;
            break;
        case 2: // down
            stbk = STBK_DOWN;
            break;
        case 3: // left
            stbk = STBK_LEFT;
            break;
        case 4: // right
            stbk = STBK_RIGHT;
            break;
        case 5: // OK
            stbk = STBK_OK;
            break;
        default:
            return STATUS_TYPE_NOT_SUPPORT;
    }

    if (stbk != 0) {
        _ui_sw_key_send(stbk);
        return STATUS_TYPE_CAN_SUPPORT;
    } else {
        return STATUS_TYPE_NOT_SUPPORT;
    }
}

// play
// recall
int app_vc_play_recall_quick(void)
{
    int stbk = 0;
    char* focus_Window = (char*)GUI_GetFocusWindow();
    if((NULL == focus_Window) || (0 == strlen(focus_Window)))
        return -1;

    if((0 == strcmp(WND_FULL_SCREEN, focus_Window))
            || (0 == strcmp(WND_CHANNEL_INFO, focus_Window))
            || (0 == strcmp(WND_PVR_BAR, focus_Window))
            )
    {
        stbk = STBK_RECALL;
    }
    if(stbk != 0)
        _ui_sw_key_send(stbk);
    else
        return STATUS_TYPE_NOT_SUPPORT;// not support
    return STATUS_TYPE_CAN_SUPPORT;
}

// play by num
int app_vc_play_by_num_quick(unsigned int num, char *name, unsigned int name_len)
{
    int ret = 0;
    unsigned int prog_pos = 0;
    char* focus_Window = (char*)GUI_GetFocusWindow();
    if((NULL == focus_Window) || (0 == strlen(focus_Window)))
        return -1;

    if((0 == strcmp(WND_FULL_SCREEN, focus_Window))
            || (0 == strcmp(WND_CHANNEL_INFO, focus_Window))
            || (0 == strcmp(WND_PVR_BAR, focus_Window))
            || (0 == strcmp(WND_NUMBER, focus_Window))
            )
    {
        ret = app_vc_number_switch(num, name, name_len);
    }
    else if(0 == strcmp(WND_CHANNEL_LIST, focus_Window))
    {
        prog_pos = app_find_get_value_by_index(num,MAX_FIND_NAME_COUNT);
        if(prog_pos != 0xffff)
        {
            if(prog_pos == 0){
                prog_pos = num;
            }
            ret = app_vc_number_switch(prog_pos, name, name_len);
             _ui_sw_key_send(STBK_EXIT);
        }
        else
        {
            ret = 1;
        }
    }
    else if((0 == strcmp(WND_NETVIDEO_PLAY, focus_Window))
            || (0 == strcmp(WND_PLUGIN_PLAY_INFO, focus_Window))
            || (0 == strcmp(WND_NETVIDEO_PLAY_NUMBER, focus_Window))
           )
    {
        ret = app_vc_net_number_switch(num, name, name_len);
    }
    else
    {
        return STATUS_TYPE_NOT_SUPPORT;// not support
    }
    if(ret){
        return STATUS_TYPE_NOT_SUPPORT;
    }
    return STATUS_TYPE_CAN_SUPPORT;
}

// parameters:
//     name[input]:
//     prog_id[output]: the ID of the matching channel
//     prog_pos[output]: the position of the matching channel
//     dbase_name[output]: the name of the matching channel
//     info[output]: the viewing mode of the matching channel
// ret: -1-error, -2-no matches found, 0-find in the current view mode, 1-find in other view mode
static int _name_match(char *name, unsigned int *prog_id, unsigned int *prog_pos, char* dbase_name, GxBusPmViewInfo* info)
{
#define NAME_LEN   (MAX_PROG_NAME + 1)
    GxBusPmViewInfo sys_info = {0};
    unsigned int *pIDs = NULL;
    char** pName = NULL;
    char *p = NULL;
    int ret = 0;
    int prog_num = 0;
    int i = 0;
    char flag = 1;
    GxMsgProperty_NodeNumGet node_num = {0};
    GxMsgProperty_NodeByPosGet prog_node = {0};
    unsigned int index_count = 0;
    int match_index[32] = {0};

    // search current view mode
    node_num.node_type = NODE_PROG;
    ret = app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num));
    if(GXCORE_ERROR == ret){
        app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
        return -1;
    }
    prog_num = node_num.node_num;
    pIDs = GxCore_Malloc(prog_num*sizeof(unsigned int));
    if(NULL == pIDs){
        app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
        return -1;
    }
    pName = GxCore_Malloc(prog_num*sizeof(char*) + prog_num*NAME_LEN*sizeof(char));
    if(NULL == pName){
        app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
        ret = -1;
        goto exit;
    }

    p = (char*)(pName + prog_num);
    for(i = 0; i < prog_num; i++){
        prog_node.node_type = NODE_PROG;
        prog_node.pos = i;
        if(GXCORE_ERROR == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&prog_node))){
            app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
            ret = -1;
            goto exit;
        }
        memcpy(p, prog_node.prog_data.prog_name, MAX_PROG_NAME);
        pName[i] = p;
        pIDs[i] = prog_node.prog_data.id;
        p = p + NAME_LEN;
    }

    index_count = MAX_FIND_NAME_COUNT;
    ret = app_fuzzy_strings_matching(name, pName, prog_num, match_index, &index_count,30);
    if(ret || (0 == index_count)){
        app_log_error("\nCurrent view mode can't match the name, %s, %d\n", __func__, __LINE__);
    }

    // find the name
    if(0 < index_count){
        int len = 0;
        if(1 == index_count){
            *prog_id = pIDs[match_index[0]];
            *prog_pos = match_index[0];
            len = strlen(pName[match_index[0]]);
            len = len >= MAX_PROG_NAME ? (MAX_PROG_NAME-1):len;
            memcpy(dbase_name, pName[match_index[0]], len);
            ret = 0;
        }else{
            find_name_change_list(pIDs,match_index,index_count);
            ret = 4;
        }
        goto exit;
    }

    // search other view mode
    // get current view info
    ret = GxBus_PmViewInfoGet(&sys_info);
    if(GXCORE_ERROR == ret){
        app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
        ret = -1;
        goto exit;
    }

    if(pIDs){
        GxCore_Free(pIDs);
        pIDs = NULL;
    }
    pIDs = GxCore_Malloc(SYS_MAX_PROG*sizeof(unsigned int));
    if(NULL == pIDs){
        app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
        ret = -1;
        goto exit;
    }

    if(pName){
        GxCore_Free(pName);
        pName = NULL;
    }
    pName = GxCore_Malloc(SYS_MAX_PROG*sizeof(char*) + SYS_MAX_PROG*NAME_LEN*sizeof(char));
    if(NULL == pName){
        app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
        ret = -1;
        goto exit;
    }

next:
    if(flag == 3){// last view mode
        // finish all view mode, can't match the name
        ret = -2;
        goto exit;
    }

    if (GXBUS_PM_PROG_TV == sys_info.stream_type){
        if(GROUP_MODE_ALL != sys_info.group_mode){
            sys_info.stream_type = GXBUS_PM_PROG_TV;
            flag = 2;// second view mode
        }
        else {
            sys_info.stream_type = GXBUS_PM_PROG_RADIO;
            flag = 3;// last view mode
        }
    }
    else{
        if(GROUP_MODE_ALL != sys_info.group_mode){
            sys_info.stream_type = GXBUS_PM_PROG_RADIO;
            flag = 2;// second view mode
        }
        else {
            sys_info.stream_type = GXBUS_PM_PROG_TV;
            flag = 3;// last view mode
        }
    }
    sys_info.group_mode = GROUP_MODE_ALL;
    sys_info.taxis_mode = TAXIS_MODE_NON;
    sys_info.pip_switch = 0;
    sys_info.pip_main_tp_id = 0;


    p = (char*)(pName + SYS_MAX_PROG);
    for(i = 0; i < SYS_MAX_PROG; i++){
        pName[i] = p;
        p = p + NAME_LEN;
    }

    ret = GxBus_PmProgInfoArryGetByViewInfo(&sys_info, SYS_MAX_PROG, pIDs, (int8_t**)pName);
    if(ret <= 0){
        app_log_error("\nERROR, %s, %d\n", __func__, __LINE__);
        ret = -1;
        goto exit;
    }

    prog_num = ret;
    index_count = MAX_FIND_NAME_COUNT;
    ret = app_fuzzy_strings_matching(name, pName, prog_num, match_index, &index_count, 30);
    if(ret || (0 == index_count)){
        app_log_error("\nCurrent view mode can't match the name, %s, %d\n", __func__, __LINE__);
    }

    // find the name
    if(index_count > 0){
        int len = 0;
        if(index_count == 1){
            *prog_id = pIDs[match_index[0]];
            *prog_pos = match_index[0];
            len = strlen(pName[match_index[0]]);
            len = len >= MAX_PROG_NAME ? (MAX_PROG_NAME-1):len;
            memcpy(dbase_name, pName[match_index[0]], len);

            memcpy(info, &sys_info, sizeof(GxBusPmViewInfo));
            ret = 1;
        }else{
            find_name_change_list(pIDs,match_index,index_count);
            ret = 6;
        }
    }
    else{// go next view mode
        memset(pName, 0, (SYS_MAX_PROG*sizeof(char*) + SYS_MAX_PROG*NAME_LEN*sizeof(char)));
        goto next;
    }

exit:
    if(pIDs){
        GxCore_Free(pIDs);
    }
    if(pName){
        GxCore_Free(pName);
    }
    return ret;
}

int app_vc_play_by_name_quick(char *name, char* real_channel_name, unsigned int real_name_len)
{
    int ret = 0;
    unsigned int channel_id = 0;
    unsigned int channel_pos = 0;
    char real_name[MAX_PROG_NAME] = {0};
    GxBusPmViewInfo info = {0};
    char* focus_Window = (char*)GUI_GetFocusWindow();
    if((NULL == name) || (0 == strlen(name)) || (NULL == real_channel_name) || (0 == real_name_len)
            || (NULL == focus_Window) || (0 == strlen(focus_Window)))
        return -1;


    // stage 1
    app_vc_goto_fullscreen();
    ret = _name_match(name, &channel_id, &channel_pos, real_name, &info);
    if(-1 == ret)
    {// error
        return -1;
    }
    else if(-2 == ret)
    {
        if(((0 == strcmp(WND_FULL_SCREEN, focus_Window))
            || (0 != strcmp(WND_CHANNEL_INFO, focus_Window)))
            && (strcmp(name, real_channel_name) != 0))
        {
            _ui_sw_key_send(STBK_OK);
        }
        else
        {
        // no matches found
        return STATUS_TYPE_NOT_SUPPORT;
        }
    }
    // stage 0：Duplicate judgment
    if(channel_id == app_play_id_get()){
        return STATUS_TYPE_REPEAT;
    }
    // stage 2
    if((1 == ret)||(6 == ret)){// change view mode
        g_AppPlayOps.play_list_create(&info);
        app_get_prog_pos_by_id_cur(channel_id, (int*)&channel_pos);
    }
    if((0 == ret)||(1 == ret)){
        ret = app_vc_number_switch((channel_pos + 1), NULL, 0);
        g_AppFullArb.state.tv = (info.stream_type == GXBUS_PM_PROG_TV) ? STATE_ON : STATE_OFF;
        g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
        g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);

        g_AppFullArb.state.pause = STATE_OFF;
        g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);

        real_name_len = strlen(real_name) > real_name_len?real_name_len:strlen(real_name);
        memcpy(real_channel_name, real_name, real_name_len);
    }
    return ret;
}

// main menu
// ret: 0-success; 1-not support; 2-repeat operator;
int app_vc_goto_mainmenu(void)
{
    char *focus_w = (char*)GUI_GetFocusWindow();
    if ((focus_w) && (0 == strcmp(WND_MAIN_MENU, focus_w)))
    {
        return STATUS_TYPE_REPEAT;
    }
    else if (GXCORE_SUCCESS == GUI_CheckDialog(WND_MAIN_MENU_ITEM))
    {
        GUI_EndDialog("after wnd_main_menu_item");
        GUI_SetProperty(WND_MAIN_MENU, "update_all", NULL);
        GUI_SetProperty(WND_MAIN_MENU_ITEM, "update_all", NULL);
        return STATUS_TYPE_CAN_SUPPORT;
    }
    else if(g_AppPvrOps.state == PVR_DUMMY)
    {
        GUI_EndDialog("after wnd_full_screen");
        _ui_sw_key_send(STBK_MENU);
        return STATUS_TYPE_CAN_SUPPORT;
    }
    return STATUS_TYPE_NOT_SUPPORT;
}

static void _exit_to_fullscreen(void)
{
    char *focus_wnd = NULL;
    if(GUI_CheckDialog(WND_POP_SORT) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_POP_SORT);
        GUI_SetInterface("flush", NULL);
    }
    if(GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
    {
        GUI_EndDialog(WND_PASSWD);
        GUI_SetInterface("flush", NULL);
    }
    if(GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS)
    {
        app_multi_lang_keyborad_menu_destroy();
    }
#if NETWORK_SUPPORT && WIFI_SUPPORT
    if(GUI_CheckDialog(WND_WIFI) == GXCORE_SUCCESS)
    {
        app_wifi_tip_lost_focus_hide();
    }
#endif
    focus_wnd = (char*)GUI_GetFocusWindow();
    if((focus_wnd) && (strcmp(focus_wnd, WND_DURATION_SET) == 0))
    {
        GUI_EndDialog(WND_DURATION_SET);
        GUI_EndDialog(WND_PVR_BAR);
    }
    g_AppPlayOps.normal_play.view_info.skip_view_switch = VIEW_INFO_SKIP_VANISH;
    g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
    if(g_AppChOps.get_list_total() != 0)
    {
#if DVB2IP_SERVER_SUPPORT
        if (app_dvb2ip_resume_normal_play())
#endif
        {
            g_AppPlayOps.normal_play.play_count = g_AppChOps.get_cur_pos(g_AppPlayOps.normal_play.play_count);
            app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
            g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);

            if (g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK)
                g_AppFullArb.tip = FULL_STATE_LOCKED;
            else
                g_AppFullArb.tip = FULL_STATE_DUMMY;

#if BOUQUET_SUPPORT
            g_AppBat.start(&g_AppBat);
#endif
            g_AppFullArb.state.tv = (g_AppPlayOps.normal_play.view_info.stream_type  == GXBUS_PM_PROG_TV) ? STATE_ON : STATE_OFF;
            g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
        }
    }
}

// full screen
// ret: 0-success; 1-not support; 2-repeat operator;
int app_vc_goto_fullscreen(void)
{
    char *focus_w = (char*)GUI_GetFocusWindow();
    if ((focus_w) && (0 == strcmp(WND_FULL_SCREEN, focus_w)))
    {
        return STATUS_TYPE_REPEAT;
    }
    app_trigger_end_popdlg();
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_EPG))
    {
        if((focus_w) && (strcmp(focus_w, WND_EPG) == 0))
        {
            _ui_sw_key_send(STBK_MENU);
        }
        else
        {
            _ui_sw_key_send(VK_BOOK_TRIGGER);
        }
        _exit_to_fullscreen();
    }
    else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT))
    {
        macro_ch_ed_stop_video();
        GUI_EndDialog("after wnd_full_screen");
        _exit_to_fullscreen();
    }
    else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_SYSTEM_SETTING))
    {
        app_system_trriger_func();
        _exit_to_fullscreen();
    }
    else if((GXCORE_SUCCESS == GUI_CheckDialog(WIN_MEDIA_CENTRE))
            || (GXCORE_SUCCESS == GUI_CheckDialog(WIN_FILE_BROWSER))
            || (GXCORE_SUCCESS == GUI_CheckDialog(WND_PVR_MEIDA))
            || (GXCORE_SUCCESS == GUI_CheckDialog(WND_ANTENNA_SETTING))
           )
    {
        GUI_EndDialog("after wnd_full_screen");
        _exit_to_fullscreen();
    }
    else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_MAIN_MENU))
    {
        //_ui_sw_key_send(VK_BOOK_TRIGGER);
        GUI_EndDialog("after wnd_full_screen");
        _exit_to_fullscreen();
    }
    else
    {
        GUI_EndDialog("after wnd_full_screen");
    }
    return STATUS_TYPE_CAN_SUPPORT;
}

// epg
// ret: 0-success; 1-not support; 2-repeat operator;
int app_vc_goto_epg(void)
{
    char *focus_w = (char*)GUI_GetFocusWindow();
    if ((focus_w) && (0 == strcmp(WND_EPG, focus_w)))
    {
        return STATUS_TYPE_REPEAT;
    }
    else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_EPG))
    {
        GUI_EndDialog("after wnd_epg");
        return STATUS_TYPE_CAN_SUPPORT;
    }
    else if(( g_AppPlayOps.normal_play.play_total != 0)
            && (g_AppPvrOps.state == PVR_DUMMY))
    {
        app_vc_goto_fullscreen();
        if(GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
        {
            GUI_EndDialog(WND_PASSWD);
            GUI_SetInterface("flush", NULL);
        }
        _ui_sw_key_send(STBK_EPG);
        return STATUS_TYPE_CAN_SUPPORT;
    }
    return STATUS_TYPE_NOT_SUPPORT;
}

// iptv
// ret: 0-success; 1-not support; 2-repeat operator;
int app_vc_goto_iptv(void)
{
    char *focus_w = (char*)GUI_GetFocusWindow();
    if ((focus_w) && (0 == strcmp(WND_COMMON_VIEW, focus_w)))
    {
        return STATUS_TYPE_REPEAT;
    }
    else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_COMMON_VIEW))
    {
        GUI_EndDialog("after wnd_common_view");
        return STATUS_TYPE_CAN_SUPPORT;
    }
    else if((g_AppPvrOps.state == PVR_DUMMY)
           &&((GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_INFO))
           || (GXCORE_SUCCESS == GUI_CheckDialog(WND_FULL_SCREEN)))
          )

    {
        app_vc_goto_fullscreen();
        if(GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
        {
            GUI_EndDialog(WND_PASSWD);
            GUI_SetInterface("flush", NULL);
        }
        //create iptv menu
#ifdef IPTV_SUPPORT
        app_create_gxiptv_menu();
#endif
        return STATUS_TYPE_CAN_SUPPORT;
    }
    return STATUS_TYPE_NOT_SUPPORT;
}

int app_vc_goto_xtream(void)
{
    char *focus_w = (char*)GUI_GetFocusWindow();
    if ((focus_w) && (0 == strcmp(WND_COMMON_VIEW, focus_w)))
    {
        return STATUS_TYPE_REPEAT;
    }
    else if((g_AppPvrOps.state == PVR_DUMMY)
            &&((GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_INFO))
           || (GXCORE_SUCCESS == GUI_CheckDialog(WND_FULL_SCREEN)))
           )
    {
        app_vc_goto_fullscreen();
        if(GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
        {
            GUI_EndDialog(WND_PASSWD);
            GUI_SetInterface("flush", NULL);
        }
#ifdef XTREAM_SUPPORT
        app_create_xtream_menu();
#endif
        return STATUS_TYPE_CAN_SUPPORT;
    }
    return STATUS_TYPE_NOT_SUPPORT;
}

int app_vc_goto_stalker(void)
{
    char *focus_w = (char*)GUI_GetFocusWindow();
    if ((focus_w) && (0 == strcmp(WND_COMMON_VIEW, focus_w)))
    {
        return STATUS_TYPE_REPEAT;
    }
    else if((g_AppPvrOps.state == PVR_DUMMY)
        &&((GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_INFO))
           || (GXCORE_SUCCESS == GUI_CheckDialog(WND_FULL_SCREEN)))
        )
    {
        app_vc_goto_fullscreen();
        if(GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
        {
            GUI_EndDialog(WND_PASSWD);
            GUI_SetInterface("flush", NULL);
        }
#ifdef STALKER_SUPPORT
        app_create_stalker_menu();
#endif
        return STATUS_TYPE_CAN_SUPPORT;
    }
    return STATUS_TYPE_NOT_SUPPORT;
}


// record
// is_stop: 0-record start; 1-record stop
// ret: 0-success; 1-not support; 2-repeat operator;
int app_vc_record_control(int is_stop)
{
    int ret = 0;
    char *focus_w = NULL;
    int stbk[2][2] = {{2, STBK_REC_START}, {3, STBK_REC_STOP}};

    if((is_stop < 0) || (is_stop > 1))
        return -1;

    if((GXCORE_SUCCESS == GUI_CheckDialog(WND_MAIN_MENU))
            || (GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT))
            || (GXCORE_SUCCESS == GUI_CheckDialog(WND_NETVIDEO_PLAY))
            || (GXCORE_SUCCESS == GUI_CheckDialog(WIN_MEDIA_CENTRE))
            || (GXCORE_SUCCESS == GUI_CheckDialog(WIN_FILE_BROWSER)))
        return STATUS_TYPE_NOT_SUPPORT;

    ret = app_vc_fullscreen_reaction(stbk[is_stop][0]);
    if(ret)
        return ret;
    focus_w = (char*)GUI_GetFocusWindow();
    if((focus_w) && (0 == strcmp(WND_CHANNEL_LIST,focus_w)))
    {
        if(0 != app_vc_channel_list_play_flag())
        {
            return STATUS_TYPE_NOT_SUPPORT;
        }
    }

    GUI_EndDialog("after wnd_full_screen");
    if(is_stop)
    {
        app_pvr_stop_cb(POP_VAL_OK);
    }
    else
    {
        _ui_sw_key_send(stbk[is_stop][1]);
    }
    return ret;
}

// switch file
// cmd: 0-prev file, 1-next file
// ret: 0-success; 1-not support; -1=failed
int app_vc_switch_file_quick(int cmd)
{
    int stbk = 0;
    char* focus_Window = (char*)GUI_GetFocusWindow();
    if((NULL == focus_Window) || (0 == strlen(focus_Window)) || (cmd < 0) || (cmd > 1))
        return -1;

    if(0 == strcmp(WIN_MUSIC_VIEW, focus_Window))
    {
        int buf[2] = {APPK_PREVIOUS,APPK_NEXT};
        stbk = buf[cmd];
    }
    else if((0 == strcmp(WIN_MOVIE_VIEW, focus_Window))
             || (0 == strcmp(WIN_MEDIA_BAR, focus_Window)))
    {
        int buf[2] = {APPK_PREVIOUS,APPK_NEXT};
        stbk = buf[cmd];
    }
    else if(0 == strcmp(WIN_PIC_VIEW, focus_Window))
    {
        int buf[2] = {APPK_PREVIOUS,APPK_NEXT};
        stbk = buf[cmd];
    }
    else if(0 == strcmp(WIN_FILE_BROWSER, focus_Window))
    {
        int buf[2] = {STBK_UP,STBK_DOWN};
        _ui_sw_key_send(buf[cmd]);
        stbk = STBK_OK;

    }
    else if(0 == strcmp(WND_PVR_MEIDA, focus_Window))
    {
        int buf[2] = {STBK_UP,STBK_DOWN};
        _ui_sw_key_send(buf[cmd]);
        stbk = STBK_OK;
    }
    else if(0 == strcmp(WND_COMMON_VIEW, focus_Window))
    {
        int buf[2] = {STBK_UP,STBK_DOWN};
        _ui_sw_key_send(buf[cmd]);
        stbk = STBK_OK;
    }

    if(stbk != 0)
        _ui_sw_key_send(stbk);
    else
        return STATUS_TYPE_NOT_SUPPORT;// not support

    return STATUS_TYPE_CAN_SUPPORT;
}

// standby
// cmd: 1-standby, 0-wakeup
// ret: 0-success; 1-not support; 2-repeat
int app_vc_standby_control(int is_standby)
{
    int ret = 0;

    if((is_standby && (app_simple_lowpower_status_get()))
            || ((0 == is_standby) && (0 == app_simple_lowpower_status_get()))
      )
        return STATUS_TYPE_REPEAT;

    if(((GUI_CheckDialog(WND_UPGRADE_PROCESS) == GXCORE_SUCCESS)
                || (GUI_CheckDialog(WND_NETUPG_PROCESS) == GXCORE_SUCCESS)
                || (GUI_CheckDialog(WND_SEARCH) == GXCORE_SUCCESS)
#if AUTO_UPDATE_SUPPORT
                || (app_get_auto_update_flag() != 0)
#endif
#if TKGS_SUPPORT
                || ((GUI_CheckDialog(WND_POP_TIP) == GXCORE_SUCCESS)&&(app_tkgs_get_upgrade_finish_pop_status() == true))
                || ((GUI_CheckDialog(WND_TKGS_UPGRADE_PROCESS) == GXCORE_SUCCESS)&&(app_simple_lowpower_status_get() == false))
#endif
       )
            && (is_standby)
      )
    {
        return STATUS_TYPE_NOT_SUPPORT;
    }

    if(g_AppPvrOps.state != PVR_DUMMY)
    {
        GxMsgProperty_NodeByPosGet node_prog = {0};
        unsigned int rec_prog_id = g_AppPvrOps.env.prog_id;
        book_popdlg_exec(POP_VAL_CANCEL);
        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_TIP))
        {
            popdlg_destroy();
        }
        if(GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_LIST))
        {
            GUI_EndDialog("after wnd_full_screen");
        }

        app_pvr_stop();
        if((g_AppPvrOps.state != PVR_RECORD) && (g_AppPvrOps.state != PVR_TP_RECORD))
        {
            node_prog.node_type = NODE_PROG;
            node_prog.pos = g_AppPlayOps.normal_play.play_count;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
            if(node_prog.prog_data.id == rec_prog_id)
            {
                g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
                app_play_current_prog(PLAY_MODE_POINT);
            }
            app_subt_resume();
        }
    }
    ret = top_stbk_halt(0);
    return ret;
}

int app_vc_stabdby_status_get(void)
{
    return app_simple_lowpower_status_get();
}

int app_vc_osdttx_flag_get(void)
{
    return ((g_AppFullArb.state.ttx == STATE_OFF)?0:1);
}

// string transform
char* app_vc_transform_strings(char *in_str)
{
    if(NULL == in_str)
        return "Error!!!";
    return (char*)GXGDI_I18N_String((const char*)in_str);
}

#endif

