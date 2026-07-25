/*****************************************************************************
* 			  CONFIDENTIAL
*        Hangzhou GuoXin Science and Technology Co., Ltd.
*                      (C)2009-2010, All right reserved
******************************************************************************

******************************************************************************
* File Name :	app_play_control.c
* Author    : 	shenbin
* Project   :	GoXceed -S
******************************************************************************
* Purpose   :
******************************************************************************
* Release History:
  VERSION	Date			  AUTHOR         Description
   0.0  	2010.2.27	           shenbin         creation
*****************************************************************************/

/* Includes --------------------------------------------------------------- */
#include "app_module.h"
#include "channel_common.h"
#include "app_send_msg.h"
#include "app_msg.h"
#include "app_bsp.h"
#include "app_default_params.h"
#include "full_screen.h" /* need sync lock state, so embeded  */
#include "module/app_frontend.h"
#include "app_epg.h"
#include "app_volume_value.h"
#include "app_pop.h"
#include "app_windows.h"
#include <gxplayer_url.h>
#include "app_pdmx.h"
#include "app_channel_info.h"
#include "app_netaudio.h"
#include "app_fuse_ops.h"
#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
#include "tkgs/app_tkgs_background_upgrade.h"
extern status_t app_tkg_lcn_flush(void);
extern void app_tkgs_sort_channels_by_lcn(void);
extern bool app_full_screen_get_enter_standby_flag(void);

#endif
#if LCN_SUPPORT
#include "app_lcn.h"
#endif
#if CASCAM_SUPPORT
#include "app_cascam.h"
#endif
#if PRESET_PROG_INFO_CORRECT_SUPPORT
#include "app_preset_prog_info_correct.h"
#endif

#define PLAY_DELAY_MS (300)

typedef struct
{
    int32_t     play_count;
    uint32_t    key;
} PlayPwdCbData;

static PlayerWindow s_NormalRect = {0, 0, APP_XRES, APP_YRES};

static event_list *s_play_delay_timer = NULL;
static event_list *s_clear_paly_flag_timer = NULL;
static event_list *s_play_rec_timer = NULL;
static event_list* spApp_pmt_play_timer = NULL;
static uint32_t pmt_auto_update_count = 0;
static bool s_play_exec_flag = false;
static time_t s_rec_sec = 0;
static event_list *thiz_replay_timer = NULL;
static GxBusPmDataProg thiz_replay_prog_bak = {0};
static bool s_rec_restart = false;
static bool s_rec_flag = false;
static int s_next_prog_num = 0;
#if RECALL_LIST_SUPPORT
static int s_recall_view_num = 1;//20130419
#endif
static uint16_t prog_tp_id = 0;
static uint32_t prog_ts_id = 0;

extern bool app_full_check_scramble(uint32_t flag);
extern void app_set_avcodec_flag(bool state);
extern bool app_check_av_running(void);
extern void app_audio_free(void);
#if SMART_VOLUME_SUPPORT
extern void app_smart_volume_save_extinfo(int32_t prog_id);
extern void app_smart_volume_set_loudness(int32_t prog_id);
#endif
#if MULTIFEED_LIST_SUPPORT
extern void app_multifeed_clear(void);
#endif
extern void app_bad_signal_count_clean(void);
#if UNICABLE_SUPPORT
extern int app_unicable_param_check(GxBusPmDataSat sat);
#endif
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
extern int app_system_ad_volume_set(int32_t offset);
#endif
/* Private functions ------------------------------------------------------ */
#define PLAY_KEY_GET()      app_play_key_get()
#define PLAY_DISC_SET()
#define PID_VAILD(pid) (pid >= 0 && pid < 0x1FFF)
#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
static bool app_channel_blocking_check(uint32_t prog_id)
{
    GxTkgsBlockCheckParm block_check = {prog_id, 0};

    app_send_msg_exec(GXMSG_TKGS_BLOCKING_CHECK, &block_check);
    return (block_check.resault == 1);
}
#endif

#if RICHEPG_SUPPORT
extern void app_backstage_search_stop(void);
extern int32_t app_backstage_search_start(uint32_t ts_src, uint32_t dmx_id, uint32_t sat_id, uint32_t tp_id);
extern int app_backstage_search_status(void);
#endif

void app_play_pid_search_stop(void)
{
#if RICHEPG_SUPPORT
	return app_backstage_search_stop();
#endif
}

int app_play_pid_search_start(uint32_t ts_src, uint32_t dmx_id, uint32_t sat_id, uint32_t tp_id)
{
#if RICHEPG_SUPPORT
	return app_backstage_search_start(ts_src, dmx_id, sat_id, tp_id);
#else
	return 0;
#endif
}

int app_play_pid_search_status(void)
{
#if RICHEPG_SUPPORT
	return app_backstage_search_status();
#else
	return 0;
#endif
}

int app_panel_show_prog_num(uint32_t type)
{
    uint32_t panel_led = 0;

    if (type & PANEL_UNDISPLAY_TIME)
    {
#if PANEL_SETTING_SUPPORT
        extern void app_panel_undisplay_time(void);
        app_panel_undisplay_time();
#endif
        if (type & PANEL_CLEAN_PROG_NUM)
            app_panel_ioctl_handle(PANEL_SHOW_NUM, &panel_led);
        return 0;
    }
#if PANEL_SETTING_SUPPORT
    else if (type & PANEL_DISPLAY_TIME)
    {
        extern void app_panel_display_time(void);
        app_panel_display_time();
    }
    else
#endif
    {
        if (!(type & PANEL_CLEAN_PROG_NUM) && g_AppPlayOps.normal_play.play_total > 0)
        {
            GxMsgProperty_NodeByPosGet node = {0};
            node.node_type = NODE_PROG;
            node.pos = g_AppPlayOps.normal_play.play_count;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));
            panel_led = app_channel_panel_led_get(node.prog_data.id);
        }
        app_panel_ioctl_handle(PANEL_SHOW_NUM, &panel_led);
    }
    return 0;
}

void app_play_current_prog(uint32_t mode)
{
    if(g_AppChOps.get_list_total() > 0)
        g_AppPlayOps.program_play(mode, g_AppPlayOps.normal_play.play_count);
}

bool app_play_rec_timer_check(void)
{
    if (s_play_rec_timer)
        return true;
    return s_rec_restart;
}

int app_play_update_prog_tp_id(uint16_t tp_id)
{
	if(tp_id != prog_tp_id)
	{
		prog_tp_id = tp_id;
	}
	return 0;
}

int app_channel_info_get_prog_lock_flag(void)
{
    int ret = 0;
    GxMsgProperty_NodeByPosGet node_prog = {0};
    // prog
    node_prog.node_type = NODE_PROG;
    node_prog.pos = s_next_prog_num;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
    ret = node_prog.prog_data.lock_flag;
    return ret;
}

status_t app_get_prog_pos_by_id(GxBusPmViewInfo *pm_view, uint32_t prog_id, int32_t *prog_pos)
{
    GxMsgProperty_NodeNumGet node_num_get = {0};
    GxMsgProperty_NodeByPosGet node = {0};
    uint32_t i = 0;

    if (prog_id == 0)
    {
        return GXCORE_ERROR;
    }

    app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(pm_view));

    node_num_get.node_type = NODE_PROG;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));

    node.node_type = NODE_PROG;
    for (i = 0; i < node_num_get.node_num; i++)
    {
        node.pos = i;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

        if (node.prog_data.id == prog_id)
        {
            *prog_pos  = i;
            return GXCORE_SUCCESS;
        }
    }

    return GXCORE_ERROR;
}

int app_get_prog_pos_by_id_cur(int prog_id, int32_t *prog_pos)
{
    int pos;
    GxMsgProperty_NodeByPosGet node = {0};
    GxMsgProperty_NodeNumGet nodeNum = {0};

    nodeNum.node_type = NODE_PROG;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&nodeNum)))
    {
        for(pos = 0; pos < nodeNum.node_num; pos++)
        {
            node.node_type = NODE_PROG;
            node.pos = pos;
            if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
            {
                if(node.prog_data.id == prog_id)
                {
                    if(prog_pos != NULL)
                    {
                        *prog_pos = node.pos;
                    }
                    return 0;
                }
            }
        }
    }

    return -1;
}

int app_get_prog_pos_by_id_all(int prog_id, int32_t *prog_pos)
{
    int ret = 0;
    GxMsgProperty_NodeByIdGet prog_node;
    GxBusPmViewInfo viewinfo_use, viewinfo_bak;

    prog_node.node_type = NODE_PROG;
    prog_node.id = prog_id;
    if(GXCORE_ERROR == app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&prog_node)))
    {
        return -1;
    }

    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&viewinfo_use));
    memcpy(&viewinfo_bak, &viewinfo_use, sizeof(GxBusPmViewInfo));

    if((viewinfo_use.group_mode == GROUP_MODE_SAT && viewinfo_use.sat_id == prog_node.prog_data.sat_id)
            || (viewinfo_use.group_mode == GROUP_MODE_FAV && viewinfo_use.fav_id == prog_node.prog_data.favorite_flag))
    {
        if(viewinfo_use.stream_type != prog_node.prog_data.service_type)
        {
            viewinfo_use.stream_type = prog_node.prog_data.service_type;
            GxBus_PmViewInfoMemSet(&viewinfo_use);
        }
    }
    else
    {
        if ( !app_fuse_spec_is_sattv_view_mode(-1,-1,viewinfo_use.group_mode,viewinfo_use.sat_id) )
        {
            viewinfo_use.group_mode = GROUP_MODE_ALL;
            viewinfo_use.taxis_mode = TAXIS_MODE_NON;
            viewinfo_use.pip_switch = 0;
            viewinfo_use.pip_main_tp_id = 0;
            viewinfo_use.stream_type = (GXBUS_PM_PROG_TV == prog_node.prog_data.service_type) ? GXBUS_PM_PROG_TV : GXBUS_PM_PROG_RADIO;
            GxBus_PmViewInfoMemSet(&viewinfo_use);
        }
    }

    if(app_get_prog_pos_by_id_cur(prog_id, prog_pos) < 0)
    {
        app_log_error("\033[31m prog not find, prog_id = %d!\033[0m\n", prog_id);
        GxBus_PmViewInfoMemClear();
        return -1;
    }
    GxBus_PmViewInfoMemClear();
    if(memcmp(&viewinfo_bak, &viewinfo_use, sizeof(GxBusPmViewInfo)))
    {
        g_AppPlayOps.play_list_create(&viewinfo_use);
        ret = 1;
    }

    return ret;
}

uint32_t app_play_id_get(void)
{
#define ALL_GROUP (0)
#define SAT_GROUP (1)
#define FAV_GROUP (2)
#define HD_GROUP  (0x4)
    GxBusPmViewInfo view_info = {0};
    GxMsgProperty_NodeByIdGet node = {0};
    uint32_t id = 0;

    // get system info
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

    switch (view_info.group_mode)
    {
        case ALL_GROUP:
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                GxBus_ConfigGetInt(ALL_TV_KEY, (int32_t *)&id, ALL_TV_ID);
            }
            else
            {
                GxBus_ConfigGetInt(ALL_RADIO_KEY, (int32_t *)&id, ALL_RADIO_ID);
            }
            break;

        case SAT_GROUP:
            node.node_type = NODE_SAT;
            node.id = view_info.sat_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                id = node.sat_data.cur_tv;
            }
            else
            {
                id = node.sat_data.cur_radio;
            }
            break;

        case FAV_GROUP:
            node.node_type = NODE_FAV_GROUP;
            node.id = view_info.fav_id;
            app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                id = node.fav_item.cur_tv;
            }
            else
            {
                id = node.fav_item.cur_radio;
            }
            break;
        case HD_GROUP:
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                GxBus_ConfigGetInt(HD_TV_KEY, (int32_t *)&id, HD_TV_ID);
            }
            break;

        default:
            break;
    }

    return id;
}

uint32_t app_play_id_set(uint32_t id)
{
#define ALL_GROUP (0)
#define SAT_GROUP (1)
#define FAV_GROUP (2)
    GxBusPmViewInfo view_info = {0};
    GxMsgProperty_NodeByIdGet node_get = {0};
    GxMsgProperty_NodeModify node_modify = {0};

    // get system info
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));

    switch (view_info.group_mode)
    {
        case ALL_GROUP:
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                GxBus_ConfigSetInt(ALL_TV_KEY, id);
            }
            else
            {
                GxBus_ConfigSetInt(ALL_RADIO_KEY, id);
            }
            break;

        case SAT_GROUP:
            node_get.node_type = NODE_SAT;
            node_get.id = view_info.sat_id;
			if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_get) != GXCORE_SUCCESS)
			{
				break;
			}
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                node_get.sat_data.cur_tv = id;
            }
            else
            {
                node_get.sat_data.cur_radio = id;
            }

            node_modify.node_type = NODE_SAT;
            node_modify.sat_data = node_get.sat_data;
            app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
            break;

        case FAV_GROUP:
            node_get.node_type = NODE_FAV_GROUP;
            node_get.id = view_info.fav_id;
			if(app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_get) != GXCORE_SUCCESS)
			{
				break;
			}
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                node_get.fav_item.cur_tv = id;
            }
            else
            {
                node_get.fav_item.cur_radio = id;
            }

            node_modify.node_type = NODE_FAV_GROUP;
            node_modify.fav_item = node_get.fav_item;
            app_send_msg_exec(GXMSG_PM_NODE_MODIFY, &node_modify);
            break;
        case HD_GROUP:
            if (view_info.stream_type == GXBUS_PM_PROG_TV)
            {
                GxBus_ConfigSetInt(HD_TV_KEY, id);
            }
            break;

        default:
            break;
    }

    if (view_info.stream_type == GXBUS_PM_PROG_TV)
    {
        view_info.tv_prog_cur = id;
    }
    else
    {
        view_info.radio_prog_cur = id;
    }
    app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)(&view_info));

    return 0;
}

int32_t app_play_key_get(void)
{
    int32_t lock_state = 0;

    GxBus_ConfigGetInt(CHANNEL_LOCK_KEY, &lock_state, CHANNEL_LOCK);

    return lock_state;
}

bool app_check_play_exec_flag(void)
{
    return s_play_exec_flag;
}

bool app_check_play_rec_flag(void)
{
    return s_rec_flag;
}

static void viewinfo_init(GxBusPmViewInfo  *view_info)
{
#if TKGS_SUPPORT
    int status = 0;
#endif
    // get system info
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)view_info);

    if (view_info->status != VIEW_INFO_ENABLE)
    {
        view_info->type = VIEW_INFO_ID;
        view_info->tv_prog_cur = 0;
        view_info->radio_prog_cur = 0;
        view_info->group_mode = GROUP_MODE_ALL;
        view_info->taxis_mode = TAXIS_MODE_NON;
        view_info->stream_type = GXBUS_PM_PROG_TV;
        view_info->fav_id =  0; // GROUP_MODE_ALL needn't care it
        view_info->sat_id = 0; // GROUP_MODE_ALL needn't care it
        view_info->cas_id = 0; //TAXIS_MODE_NON needn't care it
        view_info->tp_id = 0; //TAXIS_MODE_NON needn't care it
        view_info->tuner_num = 0; //TAXIS_MODE_NON needn't care it
        view_info->pip_switch = 0; // full screen program
        view_info->pip_main_tp_id = 0; // cur tp id
        memset(view_info->letter, 0, MAX_CMP_NAME);  // TAXIS_MODE_NON needn't care it
        view_info->reserved1 = 0;
        view_info->status = VIEW_INFO_ENABLE;
#if TKGS_SUPPORT
        status = 1;
#endif
    }

    if (view_info->pip_main_tp_id != 0)
    {
        pvr_taxis_mode_clear();

        memset(view_info, 0, sizeof(GxBusPmViewInfo));
        app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(view_info));
    }
    else
    {
        view_info->taxis_mode = TAXIS_MODE_NON;
    }

    if(view_info->stream_type == GXBUS_PM_PROG_ALL)
    {
        view_info->stream_type = GXBUS_PM_PROG_TV;
    }

    if (view_info->skip_view_switch == VIEW_INFO_SKIP_CONTAIN)
    {
        view_info->skip_view_switch = VIEW_INFO_SKIP_VANISH;
    }
    // set system info
    app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *)view_info);

#if TKGS_SUPPORT
    if (status == 1)
    {
        app_tkg_lcn_flush();
        app_tkgs_sort_channels_by_lcn();//2014.09.25 add
        status = 0;
    }
#endif
}

/**
 * @brief when finish the list operation, rebuild the play list
 *         !!! will change g_AppPlayOps.normal_play.view_info !!!
 * @param play mode
 * @Return no program return GXCORE_ERROR
 *          else GXCORE_SUCCESS
 */
static status_t app_play_list_create(GxBusPmViewInfo *p_view_info)
{
    GxMsgProperty_NodeNumGet node_num_get = {0};
    GxMsgProperty_NodeNumGet node_num_get_temp = {0};
    GxBusPmViewInfo view_info_temp = {0};
    uint32_t prog_id;

    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info_temp));
    node_num_get_temp.node_type = NODE_PROG;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get_temp));

    if (p_view_info != &(g_AppPlayOps.normal_play.view_info))//该部分判断传入参数是否就是“g_AppPlayOps.normal_play.view_info”
    {
        memcpy(&(g_AppPlayOps.normal_play.view_info), p_view_info, sizeof(GxBusPmViewInfo));
    }
    // new list create，下述接口封装已经有数据相同判断，所以此处可以不用再做是否相同判断
    app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *) & (g_AppPlayOps.normal_play.view_info));

    node_num_get.node_type = NODE_PROG;
    app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
	g_AppChOps.create(node_num_get.node_num);

    if ((g_AppChOps.get_list_total() == 0)
            && (g_AppPlayOps.normal_play.view_info.group_mode != GROUP_MODE_ALL)
            && ((g_AppPlayOps.normal_play.view_info.skip_view_switch == view_info_temp.skip_view_switch)
			 || (node_num_get_temp.node_num != 0))
			)
    {
        if ( !app_fuse_spec_is_sattv_view_mode(g_AppPlayOps.normal_play.view_info.stream_type,GXBUS_PM_PROG_RADIO,
			g_AppPlayOps.normal_play.view_info.group_mode,g_AppPlayOps.normal_play.view_info.sat_id))
        {
            p_view_info->group_mode = GROUP_MODE_ALL;
            g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_ALL;
            app_send_msg_exec(GXMSG_PM_VIEWINFO_SET, (void *) & (g_AppPlayOps.normal_play.view_info));
            node_num_get.node_type = NODE_PROG;
            app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&node_num_get));
            g_AppChOps.create(node_num_get.node_num);
        }
    }

    if (node_num_get.node_num == 0)
    {
        g_AppPlayOps.normal_play.play_total = 0;
        g_AppPlayOps.normal_play.play_count = 0;
        app_panel_show_prog_num(PANEL_CLEAN_PROG_NUM);
        return GXCORE_ERROR;
    }

    g_AppPlayOps.normal_play.play_total = node_num_get.node_num;
	prog_id = app_play_id_get();
    if (app_get_prog_pos_by_id(&g_AppPlayOps.normal_play.view_info, prog_id, &g_AppPlayOps.normal_play.play_count) == GXCORE_ERROR)
    {
        g_AppPlayOps.normal_play.play_count = g_AppChOps.real_pos(0);
    }
    g_AppPlayOps.normal_play.play_count = g_AppChOps.get_cur_pos(g_AppPlayOps.normal_play.play_count);

    app_panel_show_prog_num(0);

    GxMsgProperty_NodeByPosGet node = {0};
    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)))
    {
        g_AppPlayOps.normal_play.tuner = node.prog_data.tuner;
    }
    return GXCORE_SUCCESS;
}

static void app_program_play_init(void)
{
    GxBusPmViewInfo view_info = {0};
    AppFrontend_Config cfg = {0};
    app_ioctl(0, FRONTEND_CONFIG_GET, &cfg);

    g_AppPlayOps.normal_play.ts_src = cfg.ts_src; // multi-ts need this TS1-0 TS2-1 TS3-2
#if START_CHANNEL_SUPPORT
{
    extern int32_t app_get_start_channel_number(int32_t pos);
    app_get_start_channel_number(g_AppPlayOps.normal_play.play_count);
}
#endif

    viewinfo_init(&view_info);
    app_play_list_create(&view_info);

    // set recall_play params
#if RECALL_LIST_SUPPORT
    g_AppPlayOps.recall_play[0] = g_AppPlayOps.normal_play;
#else
    g_AppPlayOps.recall_play = g_AppPlayOps.normal_play;
#endif
}

static void app_play_window_set(AppPlayMode play_mode, PlayerWindow *rect)
{
    GxMsgProperty_PlayerVideoWindow play_wnd;

    memcpy(&s_NormalRect, rect, sizeof(PlayerWindow));
    play_wnd.player = PLAYER_FOR_NORMAL;
    memcpy(&(play_wnd.rect), rect, sizeof(PlayerWindow));

    app_send_msg_exec(GXMSG_PLAYER_VIDEO_WINDOW, (void *)(&play_wnd));
}

void app_play_set_diseqc(uint32_t sat_id, uint16_t tp_id, bool ForceFlag)
{
    AppFrontend_DiseqcParameters diseqc10 = {0};
    AppFrontend_DiseqcParameters diseqc11 = {0};
    GxMsgProperty_NodeNumGet tp_num = {0};
    GxMsgProperty_NodeByIdGet node_sat = {0};
    GxMsgProperty_NodeByIdGet node_tp = {0};
    bool diseqc_force = ForceFlag;

    diseqc10.type = DiSEQC10;
    node_sat.node_type = NODE_SAT;
    node_sat.id = sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_sat);
    diseqc10.u.params_10.chDiseqc = node_sat.sat_data.sat_s.diseqc10;
    diseqc10.u.params_10.bPolar = app_show_to_lnb_polar(&node_sat.sat_data);

    node_tp.node_type = NODE_TP;
    node_tp.id = tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_tp);

    if (diseqc10.u.params_10.bPolar == SEC_VOLTAGE_ALL)
    {
        tp_num.node_type = NODE_TP;
        tp_num.sat_id = sat_id;
        app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, &tp_num);
        if (tp_num.node_num == 0)
        {
            diseqc10.u.params_10.bPolar = SEC_VOLTAGE_18;
        }
        else
        {
            diseqc10.u.params_10.bPolar = app_show_to_tp_polar(&node_tp.tp_data);
        }
    }
    diseqc10.u.params_10.b22k = app_show_to_sat_22k(node_sat.sat_data.sat_s.switch_22K, &node_tp.tp_data);

    diseqc11.u.params_11.chUncom = node_sat.sat_data.sat_s.diseqc11;

    diseqc11.type = DiSEQC11;
    diseqc11.u.params_11.chCom = diseqc10.u.params_10.chDiseqc ;
    diseqc11.u.params_11.bPolar = diseqc10.u.params_10.bPolar;
    diseqc11.u.params_11.b22k = diseqc10.u.params_10.b22k;

#if UNICABLE_SUPPORT
    if(!app_unicable_param_check(node_sat.sat_data))
    {
        // for polar
        AppFrontend_PolarState Polar = diseqc10.u.params_10.bPolar ;
        app_ioctl(node_sat.sat_data.tuner, FRONTEND_POLAR_SET, (void *)&Polar);
    }
#else
    // for polar
    AppFrontend_PolarState Polar = diseqc10.u.params_10.bPolar ;
    app_ioctl(node_sat.sat_data.tuner, FRONTEND_POLAR_SET, (void *)&Polar);
#endif

    // for 22k
    //GxFrontend_Set22K(sat_node.sat_data.tuner, sat22k);
    AppFrontend_22KState _22K = diseqc10.u.params_10.b22k;
    app_ioctl(node_sat.sat_data.tuner, FRONTEND_22K_SET, (void *)&_22K);

    app_set_diseqc_10(node_sat.sat_data.tuner, &diseqc10, diseqc_force);
    app_set_diseqc_11(node_sat.sat_data.tuner, &diseqc11, diseqc_force);

    int32_t motor_state = app_get_motor_state_in_fullscreen();
    MotorState tip = DISH_NONE;

    if ((node_sat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_1_2) == GXBUS_PM_SAT_DISEQC_1_2)
    {
        tip = app_diseqc12_goto_position(node_sat.sat_data.id, node_sat.sat_data.tuner,
                node_sat.sat_data.sat_s.diseqc12_pos, diseqc_force, motor_state);
        app_create_position_tip(tip, motor_state);
    }
    else if ((node_sat.sat_data.sat_s.diseqc_version & GXBUS_PM_SAT_DISEQC_USALS) == GXBUS_PM_SAT_DISEQC_USALS)
    {
        LongitudeInfo local_longitude = {0};
        local_longitude.longitude = node_sat.sat_data.sat_s.longitude;
        local_longitude.longitude_direct = node_sat.sat_data.sat_s.longitude_direct;
        tip = app_usals_goto_position(node_sat.sat_data.id, node_sat.sat_data.tuner,
                &local_longitude, NULL, diseqc_force, motor_state);
        app_create_position_tip(tip, motor_state);
    }
    app_diseqc_sat_id_bak_set(node_sat.sat_data.id);

    // POST SEM
    GxFrontendOccur(node_sat.sat_data.tuner);
}

void app_play_set_tp(GxMsgProperty_NodeByIdGet sat_get,
							GxMsgProperty_NodeByIdGet tp_get,
							GxMsgProperty_NodeByPosGet prog_get)
{
    AppFrontend_SetTp params = {0};

    switch (sat_get.sat_data.type)
    {
#if (DEMOD_DVB_S > 0)
        case GXBUS_PM_SAT_S:
        {
            params.type = FRONTEND_DVB_S;
            params.fre = app_show_to_tp_freq(&sat_get.sat_data, &tp_get.tp_data);
            params.symb = tp_get.tp_data.tp_s.symbol_rate;
			params.pls_n = tp_get.tp_data.tp_s.pls_n;
            params.stream_size = tp_get.tp_data.tp_s.stream_size;

            params.polar = app_show_to_lnb_polar(&sat_get.sat_data);
            if (params.polar == SEC_VOLTAGE_ALL)
            {
                params.polar = app_show_to_tp_polar(&tp_get.tp_data);
            }
            params.sat22k = app_show_to_sat_22k(sat_get.sat_data.sat_s.switch_22K, &tp_get.tp_data);
            params.tuner = sat_get.sat_data.tuner;

#if UNICABLE_SUPPORT
            {
                uint32_t centre_frequency = 0 ;
                uint32_t lnb_index = 0 ;
                uint32_t set_tp_req = 0;
                if(app_unicable_param_check(sat_get.sat_data))
                {
                    lnb_index = app_calculate_set_freq_and_initfreq(&sat_get.sat_data, &tp_get.tp_data, &set_tp_req, &centre_frequency);
                    params.fre  = set_tp_req;
                    params.lnb_index  = lnb_index;
                }
                else
                {
                    params.lnb_index  = 0x20;
                }
                params.centre_fre  = centre_frequency;
                params.if_channel_index = sat_get.sat_data.sat_s.unicable_para.if_channel;
                params.if_fre  = sat_get.sat_data.sat_s.unicable_para.centre_fre[params.if_channel_index];
            }
#else
            params.lnb_index = 0x20;
            params.centre_fre = 0;
            params.if_channel_index = 0;
            params.if_fre = 0;
#endif

#if MULTISTREAM_SUPPORT
				params.pls_chanage = 0xff;
				params.pls_code = prog_get.prog_data.muti_ts_code;
				params.pls_ts_id = prog_get.prog_data.muti_ts_id;
				params.pls_ts_status = prog_get.prog_data.muti_ts_exist;
#endif
            break;
        }
#endif
#if (DEMOD_DVB_C > 0)
        case GXBUS_PM_SAT_C:
        {
            params.fre = tp_get.tp_data.frequency;
            params.symb = tp_get.tp_data.tp_c.symbol_rate;
            params.qam = tp_get.tp_data.tp_c.modulation;
            params.type = FRONTEND_DVB_C;
            params.tune_mode = DVBC_TUNE_MODE;
            break;
        }
#endif
#if ((DEMOD_DVB_T > 0)||(DEMOD_ISDBT > 0))
        case GXBUS_PM_SAT_T:
        case GXBUS_PM_SAT_DVBT2:
        {
            params.fre = tp_get.tp_data.frequency;
            params.bandwidth = tp_get.tp_data.tp_dvbt2.bandwidth;
#if (DEMOD_ISDBT > 0)
		    params.type = FRONTEND_DVB_T;
#else
            params.type = FRONTEND_DVB_T2;
#endif
            params.type_1501 = prog_get.prog_data.common_status;
		    params.set_tp_mode = SET_MANUAL_PLP;
		    params.data_plp_id = prog_get.prog_data.data_plp_id;
		    params.common_plp_exist = prog_get.prog_data.common_plp_exist;
		    params.common_plp_id = prog_get.prog_data.common_plp_id;

            break;
        }
#endif
#if (DEMOD_DTMB > 0)
        case GXBUS_PM_SAT_DTMB:
            if (GXBUS_PM_SAT_1501_DTMB == sat_get.sat_data.sat_dtmb.work_mode)
            {
                params.fre = tp_get.tp_data.frequency;
                params.bandwidth = tp_get.tp_data.tp_dtmb.bandwidth;
                params.type = FRONTEND_DTMB;
                params.type_1501 = DTMB;
            }
            else
            {
                // DVBC mode
                params.fre = tp_get.tp_data.frequency;
                params.symb = tp_get.tp_data.tp_dtmb.symbol_rate;
                params.qam = tp_get.tp_data.tp_dtmb.modulation;
                params.type_1501 = DTMB_C;
            }
            break;
#endif

        default:
            break;
    }

    app_set_tp(sat_get.sat_data.tuner, &params, SET_TP_AUTO);
}

/*  @in     node  : program node
 *
 *  @return true  : program need predemux
 *          false : program don't need predemux
 */
bool app_program_need_predemux(GxMsgProperty_NodeByPosGet *node)
{
#if PDMX_SUPPORT
    GxMsgProperty_NodeByIdGet tp_get = {0};
    int pdmx_switch = GXBUS_FRONTEND_PDMX_OFF;

    GxBus_ConfigGetInt(GXBUS_FRONTEND_PDMX, &pdmx_switch, GXBUS_FRONTEND_PDMX_OFF);
    tp_get.node_type = NODE_TP;
    tp_get.id = node->prog_data.tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tp_get);

    if(/*(app_frontend_hard_cap(node->prog_data.tuner, node->prog_data.pdmx_flag))
            && */
            (GXBUS_FRONTEND_PDMX_OFF != pdmx_switch)
            && ((tp_get.tp_data.tp_s.stream_size > 188) || (node->prog_data.pdmx_flag > 0))) {
        return true;
    }
#endif
    return false;
}

status_t app_get_current_program(GxMsgProperty_NodeByPosGet *node)
{
    if (NULL == node || g_AppPlayOps.normal_play.play_total == 0) {
        return GXCORE_ERROR;
    }

    node->node_type = NODE_PROG;
    node->pos = g_AppPlayOps.normal_play.play_count;
    return app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(node));
}



void app_player_url_get(char *url, GxBusPmDataProg *prog, uint16_t ts_src, uint16_t dmx_id)
{
    // url len must equal or greater to PLAYER_URL_LONG
    char url_tmp[100] = {0};

#if UNICABLE_SUPPORT || MULTISTREAM_SUPPORT
    GxMsgProperty_NodeByIdGet tp_get = {0};
    tp_get.node_type = NODE_TP;
    tp_get.id = prog->tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tp_get);
#endif

#if UNICABLE_SUPPORT
    GxMsgProperty_NodeByIdGet sat_get = {0};
    sat_get.node_type = NODE_SAT;
    sat_get.id = prog->sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &sat_get);
#endif

    // get program url
    memset(url, 0, PLAYER_URL_LONG);
    {
        GxMsgProperty_GetUrl url_get;
        url_get.prog_info = *prog;
        url_get.url = (int8_t *)(url);
        url_get.size = PLAYER_URL_LONG;
        app_send_msg_exec(GXMSG_PM_NODE_GET_URL, (void *)(&url_get));

#if (QUICK_SWITCH_SUPPORT == 0)
        app_set_fre_zero(url);
#endif
        memset(url_tmp, 0, sizeof(url_tmp));
        sprintf(url_tmp, "&tsid:%d&dmxid:%d&progid:%d", ts_src, dmx_id, prog->id);
        strcat(url, url_tmp);
    }

#if CA_SUPPORT
    {
        int time = app_tsd_time_get();
        if((time > 0) && ((((prog->cas_id) & 0xf00) == 0x900) || (((prog->cas_id) & 0xf00) == 0xe00)))
        {
            memset(url_tmp, 0, sizeof(url_tmp));
            sprintf(url_tmp, "&tsbuff:%d&tsbuff_dmxid:%d", time,TS_2_DMX);
            strcat(url, url_tmp);
        }
    }
#endif

#if UNICABLE_SUPPORT
    {
        uint32_t centre_frequency = 0 ;
        uint32_t lnb_index = 0x20 ;
        uint32_t set_tp_req = 0;

        if(app_unicable_param_check(sat_get.sat_data))
        {
            lnb_index = app_calculate_set_freq_and_initfreq(&sat_get.sat_data, &tp_get.tp_data, &set_tp_req, &centre_frequency);
            memset(url_tmp, 0, sizeof(url_tmp));
            sprintf(url_tmp, "&ifindex:%d&lnbindex:%d&centrefre:%d&iffre:%d", sat_get.sat_data.sat_s.unicable_para.if_channel, \
                    lnb_index, centre_frequency, sat_get.sat_data.sat_s.unicable_para.centre_fre[sat_get.sat_data.sat_s.unicable_para.if_channel]);
            strcat(url, url_tmp);
        }
    }
#endif

#if MULTISTREAM_SUPPORT
    memset(url_tmp, 0, sizeof(url_tmp));
    sprintf(url_tmp, "&pls_num:%d",tp_get.tp_data.tp_s.pls_n);
    strcat(url, url_tmp);

    if(prog->muti_ts_exist == 1)
    {
        memset(url_tmp, 0, sizeof(url_tmp));
        sprintf(url_tmp, "&pls_ts_id:%d&pls_code:%d&pls_change:%d&pls_status:%d",
                prog->muti_ts_id,
                prog->muti_ts_code,
                0xff,
                prog->muti_ts_exist);
        strcat(url, url_tmp);
    }
#endif
#if NET_SEARCH_SUPPORT
	GxMsgProperty_NodeByIdGet serverdata = {0};
    serverdata.node_type = NODE_SAT;
    serverdata.id = prog->sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &serverdata);

	if(serverdata.sat_data.type == GXBUS_PM_SAT_NET)
	{
		char *sat_tp_url = NULL;
		sat_tp_url = (char*)GxCore_Calloc(PLAYER_URL_LONG, sizeof(char));
		if(sat_tp_url == NULL)
			return;
		GxBus_PmGetNetServerStreamUrlByTpId(prog->tp_id, sat_tp_url);
		memset(url_tmp, 0, sizeof(url_tmp));
		sprintf(url_tmp," -H ts_source:\"");
		strcat(url, url_tmp);
		strcat(url, sat_tp_url);
		strcat(url, "\"");
		APP_FREE(sat_tp_url);
	}
#endif

#if NET_AUDIO_SUPPORT
	 if (app_netaudio_check_flag())
	 {
         memset(url_tmp, 0, sizeof(url_tmp));
#if (defined GX3211 || defined SIRIUS || defined CANOPUS)
         sprintf(url_tmp, "&tsbuff:%d&tsbuff_dmxid:3", app_get_netaudio_tsbuff_time(app_get_netaudio_tsbuff_index()));

#else
         sprintf(url_tmp, "&tsbuff:%d&tsbuff_dmxid:1", app_get_netaudio_tsbuff_time(app_get_netaudio_tsbuff_index()));
#endif
         strcat(url, url_tmp);
	 }

	extern void app_netaudio_play_video(char *url);
	app_netaudio_play_video(url);
#endif
    /* url = dvbs://s_channellist_2fre:1342&polar:1&symbol:8800&22k:0&vpid:6496&apid:6499&pcrpid:6496
    &vcodec:1&acodec:0&tuner:0&scramble:0&pmt:5001&tsid:0&dmxid:0&progid:40 */
    app_log_info("\033[32m[Call_Player] prog=%s\033[0m\n", prog->prog_name);
    app_log_info("\033[32murl=%s\033[0m\n", url);
}

static void app_normal_play(GxMsgProperty_NodeByPosGet *prog_node)
{
    AppFrontend_Config config;
    GxMsgProperty_PlayerPlay* play = NULL;
#if MENCENT_FREEE_SPACE
    int used_spp_dlg = 0 ;
#endif
    play = GxCore_Mallocz(sizeof(GxMsgProperty_PlayerPlay));
    if(play == NULL)
    {
        app_log_error("%s:Malloc failed!!!!!!!\n",__func__);
        return ;
    }
#if PANEL_SETTING_SUPPORT
    app_panel_show_prog_num(PANEL_DISPLAY_TIME);
#endif
#if BLUETOOTH_SUPPORT
    extern int app_bluez_get_source_connect_statue(void);
    extern int app_bluez_source_pause(void);
    extern int app_bluez_source_play(void);
    if ( app_bluez_get_source_connect_statue() )
    {
        app_bluez_source_pause();
        app_player_close(PLAYER_FOR_NORMAL);
        app_bluez_source_play();
    }
#endif
#if SMART_VOLUME_SUPPORT
    app_smart_volume_set_loudness(prog_node->prog_data.id);
#endif
    app_ioctl(prog_node->prog_data.tuner, FRONTEND_CONFIG_GET, &config);
    g_AppPlayOps.normal_play.tuner = config.tuner;
    g_AppPlayOps.normal_play.dmx_id = config.dmx_id;
#if PDMX_SUPPORT
    if(true == app_program_need_predemux(prog_node)
            && false == app_frontend_hard_cap(prog_node->prog_data.tuner, prog_node->prog_data.pdmx_flag))
    {
        g_AppPlayOps.normal_play.ts_src = 3;
    }
    else
#endif
    {
        g_AppPlayOps.normal_play.ts_src = config.ts_src;
    }

#if CA_SUPPORT
    app_descrambler_close_by_control_type(CONTROL_NORMAL);
#endif

    play->player = PLAYER_FOR_NORMAL;
    app_player_url_get(play->url, &prog_node->prog_data,
            g_AppPlayOps.normal_play.ts_src, g_AppPlayOps.normal_play.dmx_id);
    memcpy(&play->window, &s_NormalRect, sizeof(PlayerWindow)); // record player window
#if MENCENT_FREEE_SPACE
    used_spp_dlg = 0 ;

    if (GUI_CheckDialog(WND_MAIN_MENU) == GXCORE_SUCCESS)
    {
        used_spp_dlg = 1;
    }

    if( APP_THEME_CLASSIC_BLUE ){  //THEME_CLASSIC_BLUE big memory use spp, other not use spp. play malloc bug 20230426
    if ( (GUI_CheckDialog(WND_CHANNEL_EDIT) == GXCORE_SUCCESS)  ||
	     (app_channel_epg_check_dialog() == GXCORE_SUCCESS) )
    {
        used_spp_dlg = 1 ;
    }
    }else{
    if ( (GUI_CheckDialog(WND_CHANNEL_EDIT) == GXCORE_SUCCESS)  ||
	     (app_channel_epg_check_dialog() == GXCORE_SUCCESS) )
    {
        used_spp_dlg = 0 ;
    }
    }

    if ((GXBUS_PM_PROG_TV == prog_node->prog_data.service_type) && ( used_spp_dlg == 0))
    {
        GUI_SetInterface("free_space", "fragment|spp");
        GxCore_HwCleanCache();
    }
    else
    {
        GUI_SetInterface("free_space", "fragment");
    }
#endif
    // send message to player
    //app_send_msg_exec(GXMSG_PLAYER_PLAY, (void *)(&play));
	//
#if QUICK_SWITCH_SUPPORT
    char url_tmp[100] = {0};
    snprintf(url_tmp, sizeof(url_tmp), "&tscache_ms:%d&tscache_mode:%d&tscache_dmxid:%d&tscache_bufsize:%d", 600, 2, g_AppPlayOps.normal_play.dmx_id + 1, 10240000);
    strcat(play->url, url_tmp);
#endif
    if (GUI_CheckDialog(WND_SYSTEM_SETTING) != GXCORE_SUCCESS)
    {
#if CASCAM_SUPPORT
        app_ca_stop_descramble();
#endif
        if(app_channel_epg_check_dialog() == GXCORE_SUCCESS)
        {
            app_send_msg_exec(GXMSG_PLAYER_PLAY, (void *)play);
        }
        else
        {
            GxPlayer_MediaPlay(play->player, play->url, 0, 0, NULL);//&play.window
        }
#if CASCAM_SUPPORT
		{
			CascamSwitchChannel switch_channel;
			switch_channel.pmt_pid = prog_node->prog_data.pmt_pid;
			switch_channel.service_id = prog_node->prog_data.service_id;
			switch_channel.audio_pid = prog_node->prog_data.cur_audio_pid;
			switch_channel.video_pid = prog_node->prog_data.video_pid;
			app_ca_switch_channel(switch_channel);
		}
#endif
        int32_t audio_vol = 0;
        if (app_volume_scope_status() == VOLUME_CHANNEL)
        {
            audio_vol = prog_node->prog_data.audio_volume;
            app_system_vol_set(audio_vol);
        }
        else
        {
            GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &audio_vol, AUDIO_VOLUME);
            app_system_vol_set(audio_vol);
        }

#if AUDIO_DESCRIPTOR_SUPPORT
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
        {
            int ad_vol_offset = 0;
            GxBus_ConfigGetInt(AD_VOL_OFFSET_KEY, &ad_vol_offset, AD_VOL_OFFSET_VALUE);
            app_system_ad_volume_set(ad_vol_offset);
        }
#endif
#endif
    }

    GXCORE_FREE(play);
    app_play_update_prog_tp_id(prog_node->prog_data.tp_id);
#if PDMX_SUPPORT
    {
        GxMsgProperty_NodeByIdGet tp_get = {0};

        tp_get.node_type = NODE_TP;
        tp_get.id = prog_node->prog_data.tp_id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tp_get);
        app_pdmx_play_start(g_AppPlayOps.normal_play.tuner, config.ts_src, config.dmx_id,
                tp_get.tp_data.tp_s.stream_size,  (void*)&(prog_node->prog_data));
    }
#endif

#if SAT2IP_SERVER_SUPPORT
    int app_sat2ip_play_change(GxBusPmDataProg *prog);
    app_sat2ip_play_change(&prog_node->prog_data);
#endif
#if DVB_CLOUD_SUPPORT
    void app_dvbonline_program_play(GxBusPmDataProg *prog);
    app_dvbonline_program_play(&prog_node->prog_data);
#endif

    /* dvb2ip: refresh the served program count whenever a channel plays. The
     * boot-time count (from network init) can run before the program DB is
     * ready, latching prog_num=0 and permanently blocking the HTTP server; here
     * the DB is loaded and the PM lock is free, so it counts correctly.
     * prog_num_update is a no-op when the count is unchanged, so this does not
     * disturb active client streams on every zap. */
#if DVB2IP_SERVER_SUPPORT
    {
        void app_dvb2ip_prog_num_update(void);
        app_dvb2ip_prog_num_update();
    }

    {
        /* Zap-driven dmx2 clear-TS -> UDP output. Non-blocking (arms a deferred
         * start timer); tears down the previous program's capture first. Does
         * NOT touch the screen decode above (PLAYER_FOR_NORMAL, 1155/1159) --
         * this adds a PARALLEL UDP output that follows channel changes, sourced
         * from the dmx2 clear-TS path (not the device-encrypted DVR). This is
         * where the old userspace-RIST capture hook lived; re-added here. */
        int app_rist_play_change(GxBusPmDataProg *prog);
        app_rist_play_change(&prog_node->prog_data);
    }
#endif

    app_fuse_spec_play_parental_lock_start();
}


#if NDEMOD_WITH_1TUNER
extern unsigned char app_check_change_tp(void);
#endif


static int32_t _replay_cb(void *usrdata)
{
    GxMsgProperty_NodeByPosGet node = {0};
    int flag = app_play_pid_search_status();

    if((thiz_replay_prog_bak.video_pid == 0 && thiz_replay_prog_bak.cur_audio_pid == 0)
            || (flag == 0))
    {
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));
        if(thiz_replay_prog_bak.video_pid != node.prog_data.video_pid
           || thiz_replay_prog_bak.cur_audio_pid != node.prog_data.cur_audio_pid)
        {
            app_log_min("\033[31mreplay prog_id: %d prog_name %s service_id: %d\033[0m\n", node.prog_data.id,
                    node.prog_data.prog_name, node.prog_data.service_id);
            if (s_rec_restart)
                app_play_current_prog(PLAY_TYPE_FORCE | PLAY_MODE_WITH_REC);
            else
                app_play_current_prog(PLAY_TYPE_FORCE);
            APP_TIMER_REMOVE(thiz_replay_timer);
        }
        if(flag == 0)
        {
            APP_TIMER_REMOVE(thiz_replay_timer);
        }
    }

    return 0;
}

int app_play_start_pid_empty_check(void)
{
    GxMsgProperty_NodeByPosGet node = {0};
    AppFrontend_Config config = {0};

    // get play node
    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

    if((0 == node.prog_data.video_pid) && (0 == node.prog_data.cur_audio_pid))
    {
        memcpy(&thiz_replay_prog_bak, &node.prog_data, sizeof(GxBusPmDataProg));
        app_play_pid_search_start(config.ts_src, TS_2_DMX, node.prog_data.sat_id, node.prog_data.tp_id);
        APP_TIMER_ADD(thiz_replay_timer, _replay_cb, 1000, TIMER_REPEAT);
    }
    return 0 ;
}

static void app_prepare_for_play(GxMsgProperty_NodeByPosGet *prog_node) /* TODO: some must do befor play */
{
    GxMsgProperty_NodeByIdGet SatNode = {0};
    GxMsgProperty_NodeByIdGet TpNode = {0};
    GxMsgProperty_NodeByIdGet last_tp_node = {0};
    GxMsgProperty_FrontendSetTp Param = {0};
    AppFrontend_DiseqcChange diseqc_change = {0};
    fe_sec_voltage_t polar = SEC_VOLTAGE_OFF;
    int32_t fre = 0;
    int32_t symb = 0;
    uint32_t sat22k = 0;
    bool change_flag = false;
    AppFrontend_DiseqcParameters diseqc10 = {0};
    AppFrontend_DiseqcParameters diseqc11 = {0};
    fe_modulation_t modulation = 0;
    unsigned char bandwidth = 0;
    int epg_info_clean_control = 0;
    int need_to_settp = 0;
    AppFrontend_Config config = {0};

    g_AppFullArb.scramble_set(1);

    SatNode.node_type = NODE_SAT;
    SatNode.id = prog_node->prog_data.sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &SatNode);

    TpNode.node_type = NODE_TP;
    TpNode.id = prog_node->prog_data.tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &TpNode);

    if(prog_tp_id != 0)
    {
        last_tp_node.node_type = NODE_TP;
        last_tp_node.id = prog_tp_id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &last_tp_node);
    }

    app_ioctl(prog_node->prog_data.tuner, FRONTEND_CONFIG_GET, &config);

#if NDEMOD_WITH_1TUNER
    app_only_cur_frontend_monitor_start(SatNode.sat_data.tuner);
#if DEMOD_DVB_S
    if (SatNode.sat_data.type == GXBUS_PM_SAT_DTMB)
    {
        // lnb polar off
        AppFrontend_PolarState polar = FRONTEND_POLAR_OFF;
        GxFrontend_SetVoltage(0, polar);
    }
#endif
#endif
    GxFrontend_GetCurFre(prog_node->prog_data.tuner, &Param);
    if(prog_node->prog_data.tp_id != prog_tp_id)
        change_flag = true;
    if(change_flag == false)
    {
        if (SatNode.sat_data.type == GXBUS_PM_SAT_S)
        {
            fre = app_show_to_tp_freq(&SatNode.sat_data, &TpNode.tp_data);
            symb = TpNode.tp_data.tp_s.symbol_rate;
            polar = app_show_to_lnb_polar(&SatNode.sat_data);
            sat22k = app_show_to_sat_22k(SatNode.sat_data.sat_s.switch_22K, &TpNode.tp_data);
            if (polar == SEC_VOLTAGE_ALL)
            {
                polar = app_show_to_tp_polar(&TpNode.tp_data);
            }
            if((Param.polar != polar) || (Param.symb != symb) || (Param.sat22k != sat22k))
                change_flag = true;
            diseqc10.type = DiSEQC10;
            diseqc10.u.params_10.chDiseqc = SatNode.sat_data.sat_s.diseqc10;
            diseqc10.u.params_10.bPolar = polar;
            diseqc10.u.params_10.b22k = sat22k;
            diseqc_change.diseqc = &diseqc10;
            app_ioctl(SatNode.sat_data.tuner, FRONTEND_DISEQC_CHANGE, &diseqc_change);
            if(FRONTEND_CHANGED == diseqc_change.state)
                change_flag = true;
            diseqc11.u.params_11.chUncom = SatNode.sat_data.sat_s.diseqc11;
            diseqc11.type = DiSEQC11;
            diseqc11.u.params_11.chCom = diseqc10.u.params_10.chDiseqc ;
            diseqc11.u.params_11.bPolar = diseqc10.u.params_10.bPolar;
            diseqc11.u.params_11.b22k = diseqc10.u.params_10.b22k;
            diseqc_change.diseqc = &diseqc11;
            app_ioctl(SatNode.sat_data.tuner, FRONTEND_DISEQC_CHANGE, &diseqc_change);
            if(FRONTEND_CHANGED == diseqc_change.state)
                change_flag = true;
#if UNICABLE_SUPPORT
            uint32_t centre_frequency = 0 ;
            uint32_t lnb_index = 0 ;
            uint32_t set_tp_req = 0;
            if(app_unicable_param_check(SatNode.sat_data))
            {
                lnb_index = app_calculate_set_freq_and_initfreq(&SatNode.sat_data, &TpNode.tp_data, &set_tp_req, &centre_frequency);
            }
            else
            {
                lnb_index  = 0x20;
            }
            if((Param.if_channel_index != SatNode.sat_data.sat_s.unicable_para.if_channel)
                    || (Param.lnb_index != lnb_index)
                    || (Param.centre_fre != centre_frequency)
                    || (Param.if_fre != SatNode.sat_data.sat_s.unicable_para.centre_fre[SatNode.sat_data.sat_s.unicable_para.if_channel]))
                change_flag = true;
#endif
            if(Param.fre != fre)
                change_flag = true;
        }
        else if (SatNode.sat_data.type == GXBUS_PM_SAT_DTMB)
        {
            fre = TpNode.tp_data.frequency;
            bandwidth = TpNode.tp_data.tp_dtmb.bandwidth;
            if((Param.bandwidth != bandwidth) || (Param.fre != fre))
                change_flag = true;
        }
        else if(SatNode.sat_data.type == GXBUS_PM_SAT_DVBT2)
        {
            fre = TpNode.tp_data.frequency;
            bandwidth = TpNode.tp_data.tp_dvbt2.bandwidth;
            if((Param.bandwidth != bandwidth) || (Param.type_1501 != prog_node->prog_data.common_status) || (Param.fre != fre))
                change_flag = true;
        }
        else if(SatNode.sat_data.type == GXBUS_PM_SAT_C)
        {
            fre = TpNode.tp_data.frequency;
            modulation = TpNode.tp_data.tp_c.modulation;
            symb = TpNode.tp_data.tp_c.symbol_rate;
            if((Param.qam != modulation)|| (Param.symb != symb) || (Param.fre != fre))
                change_flag = true;
        }
        app_log_debug("app---->fre %d=%d  polar  %d=%d symb %d=%d\n",Param.fre,fre,Param.polar,polar,Param.symb,symb);
    }
#if MULTISTREAM_SUPPORT
	if(prog_node->prog_data.muti_ts_exist == 1)
	{
		app_set_muti_ts_para(SatNode.sat_data.tuner,prog_node->prog_data.muti_ts_id, prog_node->prog_data.muti_ts_code);
	}
#endif

#if PDMX_SUPPORT
    app_pdmx_prepare(prog_node->prog_data.tuner, (void*)&prog_node->prog_data);
#endif


    GxBus_ConfigGetInt(EPG_INFO_CLEAN_KEY, &epg_info_clean_control, EPG_INFO_CLEAN_TP_CHANGE);
    if(EPG_INFO_CLEAN_TP_CHANGE == epg_info_clean_control)
    {
        if((change_flag == true)
            || (((SatNode.sat_data.type == GXBUS_PM_SAT_DVBT2)||(SatNode.sat_data.type == GXBUS_PM_SAT_S))
                &&(prog_node->prog_data.ts_id != prog_ts_id))
		)
        {
            app_epg_info_clean();
        }

    }
    else if(EPG_INFO_CLEAN_SAT_CHANGE == epg_info_clean_control)
    {
        if(last_tp_node.tp_data.sat_id != prog_node->prog_data.sat_id)
        {
            app_epg_info_clean();
        }
    }

    if((change_flag == true)
#if (NDEMOD_WITH_1TUNER > 0)
            || (SatNode.sat_data.tuner != app_tuner_cur_tuner_get())
#endif
       )
    {
#if DOUBLE_S2_SUPPORT
        int double_tuner = 1;
#else
        int double_tuner = 0;
#endif

        need_to_settp = 1;
        app_epg_filter_reset();

        if(EPG_INFO_CLEAN_TP_CHANGE == epg_info_clean_control)
        {
            app_epg_ts_id_filter_modify(prog_node->prog_data.ts_id);
        }
        g_AppPlayOps.normal_play.tuner = prog_node->prog_data.tuner;

        if(g_AppPvrOps.state == PVR_RECORD && double_tuner == 1)
        {
            int pvr_fre, pvr_polar, pvr_symb;

            GxUrl_GetItem(g_AppPvrOps.env.url, GX_URL_KEY_FRE, &pvr_fre);
            GxUrl_GetItem(g_AppPvrOps.env.url, GX_URL_KEY_SYM, &pvr_symb);
            GxUrl_GetItem(g_AppPvrOps.env.url, GX_URL_KEY_POLAR, &pvr_polar);

            if(Param.fre == pvr_fre && Param.polar == pvr_polar && Param.symb == pvr_symb)
                need_to_settp = 0;
        }

#if NDEMOD_WITH_1TUNER
        need_to_settp = 1;
#endif

        if (need_to_settp)
        {
#if AUTO_UPDATE_SUPPORT
            app_auto_update_stop();
#endif
            AppFrontend_SetTp params = {0};
            app_set_tp(prog_node->prog_data.tuner, &params, SAVE_TP_PARAM);

            GxFrontend_SetUnlock(prog_node->prog_data.tuner);// for TS disable output
            GxFrontend_CleanRecordPara(prog_node->prog_data.tuner);
            app_bad_signal_count_clean();
        }

#if TKGS_SUPPORT
        g_AppTkgsBackUpgrade.init(prog_node->prog_data.sat_id, prog_node->prog_data.tp_id, prog_node->prog_data.tuner);
#endif
        prog_ts_id = prog_node->prog_data.ts_id;

        if((0 == prog_node->prog_data.video_pid) && (0 == prog_node->prog_data.cur_audio_pid))
        {
            memcpy(&thiz_replay_prog_bak, &prog_node->prog_data, sizeof(GxBusPmDataProg));
            app_play_pid_search_start(config.ts_src, TS_2_DMX, prog_node->prog_data.sat_id, prog_node->prog_data.tp_id);
            APP_TIMER_ADD(thiz_replay_timer, _replay_cb, 1000, TIMER_REPEAT);
        }
    }
    else if  ((SatNode.sat_data.type == GXBUS_PM_SAT_S)&&(prog_node->prog_data.ts_id != prog_ts_id))
    {   //测试时，调制器先播放A码流，切换B码流，但是盒子没有删除A码流节目，导致节目的tsid不一样
        app_epg_ts_id_filter_modify(prog_node->prog_data.ts_id);
        prog_ts_id = prog_node->prog_data.ts_id;
        app_epg_filter_resume();
    }
#if (DEMOD_DVB_T > 0)
    else if ((SatNode.sat_data.type == GXBUS_PM_SAT_DVBT2)
               && (Param.common_plp_id != prog_node->prog_data.common_plp_id
                  || Param.common_plp_exist != prog_node->prog_data.common_plp_exist
                  || Param.data_plp_id != prog_node->prog_data.data_plp_id))
    {
        app_epg_filter_resume();
        app_play_set_tp(SatNode, TpNode, *prog_node);
    }
    else if  ((SatNode.sat_data.type == GXBUS_PM_SAT_DVBT2)
                &&(prog_node->prog_data.ts_id != prog_ts_id))/*add for switch multiplp epg */
    {
        app_epg_ts_id_filter_modify(prog_node->prog_data.ts_id);
        prog_ts_id = prog_node->prog_data.ts_id;
        app_epg_filter_resume();
    }
#endif
    else
    {
        //epg filter reset will restart filter
        //so resume can exec when tp doesn't channge
        app_epg_filter_resume();

        if(0 == prog_node->prog_data.video_pid && 0 == prog_node->prog_data.cur_audio_pid)
        {
            memcpy(&thiz_replay_prog_bak, &prog_node->prog_data, sizeof(GxBusPmDataProg));
            app_play_pid_search_start(config.ts_src, TS_2_DMX, prog_node->prog_data.sat_id, prog_node->prog_data.tp_id);
            APP_TIMER_ADD(thiz_replay_timer, _replay_cb, 1000, TIMER_REPEAT);
        }
    }

#if CA_SUPPORT
    if (prog_node->prog_data.scramble_flag)
    {
        //TODO:
        extern int app_send_prepare(ServiceClass *param);
        ServiceClass param = {0};
        unsigned int casid[1];

        casid[0] = prog_node->prog_data.cas_id;
        param.cas_id = casid[0];
        app_send_prepare(&param);

        if(((prog_node->prog_data.cas_id) & 0xf00) == 0x900)
        {
            app_ca_set_flag(0);
        }
        if((((prog_node->prog_data.cas_id) & 0xf00) == 0xe00))
        {
            app_ca_set_flag(1);
        }
        app_ca_set_flag(2);

    }
    else
    {
        app_ca_clear_flag();
    }

#if CMM_SUPPORT
	static int thiz_cache_size_status = 0;
	if((prog_node->prog_data.scramble_flag)
		&&(prog_node->prog_data.cas_id >=0x0e00)
		&&(prog_node->prog_data.cas_id <=0x0eff))
	{
		if(thiz_cache_size_status == 0)
		{
			GxBus_ConfigSetInt(PLAYER_CONFIG_DELAY_CACHE_HW_SIZE, 188*1024*2);
			thiz_cache_size_status = 0xff;
		}
	}
	else
	{
		if(thiz_cache_size_status == 0xff)
		{
			thiz_cache_size_status = 0;
			GxBus_ConfigSetInt(PLAYER_CONFIG_DELAY_CACHE_HW_SIZE, 188*1024*10);
		}
	}
#endif
#endif

    if (need_to_settp)
    {
#if DEMOD_DVB_S
        app_play_set_diseqc(prog_node->prog_data.sat_id, prog_node->prog_data.tp_id, false);
#endif
        //set tp if necessary
        app_play_set_tp(SatNode, TpNode, *prog_node);
#if PRESET_PROG_INFO_CORRECT_SUPPORT
        if(true == app_preset_prog_info_correct_running_check())
            app_preset_prog_info_correct_stop();
        app_preset_prog_info_correct_start(config.ts_src, prog_node->prog_data.tp_id);
#endif
    }
    else
    {
#if PRESET_PROG_INFO_CORRECT_SUPPORT
        if(false == app_preset_prog_info_correct_running_check())
            app_preset_prog_info_correct_start(config.ts_src, prog_node->prog_data.tp_id);
#endif
    }

    // Disable epg when TMS/Predemux is open
    if ((PVR_DUMMY == g_AppPvrOps.state || PVR_RECORD == g_AppPvrOps.state) && !app_program_need_predemux(prog_node))
        app_epg_enable(config.ts_src, TS_2_DMX);
    else
        app_epg_disable();

    {
        int track_sel = 0;
        if (prog_node->prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_MONO)
        {
            track_sel = 0;
        }
        else if (prog_node->prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_LEFT)
        {
            track_sel = 1;
        }
        else if (prog_node->prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_RIGHT)
        {
            track_sel = 2;
        }
        else
        {
            track_sel = 3;
        }
        app_send_msg_exec(GXMSG_PLAYER_AUDIO_TRACK, &track_sel);
    }
}

int app_check_running_by_play_frame_cnt(void)
{
    if(  app_check_av_running() == true )
    {
        PlayerProgInfo  *cur_info = app_player_prog_info_get();
        GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, cur_info); //solution check
        //printf("############## frame_cnt = %lld \n",cur_info->audio_stat.play_frame_cnt);

        if ( cur_info->audio_stat.play_frame_cnt > 0)
        {
            return 1 ;
        }
        else
        {
            return 0 ;
        }
    }
    return 0 ;
}

static int thiz_rec_delay_cnt = 0;
static int _play_rec_cb(void *usrdata)
{
    GxMsgProperty_NodeByPosGet node = {0};
    bool judge_ret_1;
    int judge_ret_2,judge_ret_3;
    int ret ;
    AppFrontend_LockState lock;

#define RECORD_TP_LOCK_COUNT   16 // time = 300ms*16=4800ms
#define RECORD_WAIT_COUNT      17 // time = 300ms*17=5100ms

    judge_ret_1 = app_check_av_running() ;
    judge_ret_2 = g_AppPlayOps.normal_play.parent_key;
    judge_ret_3 = g_AppPlayOps.normal_play.key ;

    if ( (g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK) || (g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO) )
    {
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

        if ( (g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK) && (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE) )
        {
            //printf("####### _play_rec_cb is lock & scramble \n");
            judge_ret_3 = PLAY_KEY_UNLOCK ;  //not judge
        }

        if ( (g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO) && (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE) )
        {
            ret = app_check_running_by_play_frame_cnt() ;
            //printf("####### _play_rec_cb is audio stream type & scramble , ret = %d \n",ret);
            if ( ret > 0 )
            {
                judge_ret_1 = true ;
            }
            else
            {
                judge_ret_1 = false ;
            }
        }
    }

    usb_check_state usb_state = g_AppPvrOps.usb_check(&g_AppPvrOps);

    //printf("####### _play_rec_cb judge = %d,%d ,%d , %d \n",judge_ret_1,judge_ret_3,usb_state,g_AppPmt.ver);

    /*
    if (((app_check_av_running() == true) && (0xff != g_AppPmt.ver) && (usb_state == USB_OK))
            || (g_AppPlayOps.normal_play.parent_key == PLAY_KEY_LOCK
            || g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK))
    */

    app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_LOCK_STATE_GET, &lock);

    if ((usb_state == USB_OK) && ((judge_ret_1 == true && 0xff != g_AppPmt.ver)
            || ((judge_ret_2 == PLAY_KEY_LOCK || judge_ret_3 == PLAY_KEY_LOCK) && FRONTEND_LOCKED == lock)))
    {

        if (s_play_rec_timer != NULL)
        {
            remove_timer(s_play_rec_timer);
            s_play_rec_timer = NULL;
        }
        thiz_rec_delay_cnt = 0;
        s_rec_flag = false;
        if (s_rec_sec == 0x0f)
        {
            g_AppFullArb.exec[EVENT_PAUSE](&g_AppFullArb);
            g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
        }
        else
        {
            app_pvr_rec_start(s_rec_sec);
        }

        if (app_channel_info_check_dialog() == GXCORE_SUCCESS)
        {
            app_channel_info_end_dialog();
        }

	    GUI_CreateDialog(WND_PVR_BAR);
    }
    else
    {
        ++thiz_rec_delay_cnt;
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

        if((thiz_rec_delay_cnt >= RECORD_TP_LOCK_COUNT && lock != FRONTEND_LOCKED)//若TP没锁定等待4.8s即可
                || (thiz_rec_delay_cnt >= RECORD_WAIT_COUNT)//若TP锁定了等待5s
                || (node.prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE))
        {

            if (s_play_rec_timer != NULL)
            {
                remove_timer(s_play_rec_timer);
                s_play_rec_timer = NULL;
            }
            thiz_rec_delay_cnt = 0;
            s_rec_flag = false;

            if ((app_channel_info_check_dialog() != GXCORE_SUCCESS)
                    && (node.prog_data.service_type == GXBUS_PM_PROG_RADIO))
            {
                app_channel_info_create_dialog();
            }

            PopDlg pop;
            memset(&pop, 0, sizeof(PopDlg));
            pop.type = POP_TYPE_NO_BTN;
            pop.format = POP_FORMAT_DLG;
            pop.mode = POP_MODE_UNBLOCK;
            pop.create_cb = NULL;
            pop.exit_cb = NULL;
            pop.timeout_sec = 3;
            if (usb_state != USB_OK)
            {
                if(usb_state == USB_ERROR) {
                    pop.str = STR_ID_INSERT_USB;
                } else if(usb_state == USB_NO_SPACE) {
                    pop.str = STR_ID_NO_SPACE;
                } else if(usb_state == USB_READ_ONLY) {
                    pop.str = STR_ID_NOWRITE_PERMISSIONS;
                }
            } else {
                pop.str = STR_ID_CANT_PVR;
            }
            popdlg_create(&pop);
        }
    }
    return 0;
}

static int app_rec_timer_start(GxBusPmDataProg *prog_data)
{
    if (prog_data->video_pid == 0 && prog_data->cur_audio_pid == 0)
    {
        APP_TIMER_REMOVE(s_play_rec_timer);
        s_rec_flag = false;
        s_rec_restart = true;
        return -1;
    }
    else
    {
        if (0 != reset_timer(s_play_rec_timer))
        {
            s_play_rec_timer = create_timer(_play_rec_cb, 300, NULL, TIMER_REPEAT);
        }
        return 0;
    }
}

static int _clear_play_flag_cb(void *usrdata)
{
    if (s_clear_paly_flag_timer != NULL)
    {
        remove_timer(s_clear_paly_flag_timer);
        s_clear_paly_flag_timer = NULL;
    }
    s_play_exec_flag = false;
    g_AppFullArb.timer_start();

    return 0;
}

static int _app_stop_subt(void)
{
    app_subt_pause();
    g_AppTtxSubt.ttx_num = 0;
    g_AppTtxSubt.subt_num = 0;
    g_AppTtxSubt.sync_flag = 0;
    return 0;
}

int app_update_recall_info(void)
{
#if RECALL_LIST_SUPPORT
    //g_AppPlayOps.current_play.play_count = g_AppPlayOps.normal_play.play_count;//---[2013-07-03]-
    g_AppPlayOps.add_recall_list(&(g_AppPlayOps.current_play));
#else
    bool play_common_prog = false;
    uint32_t i;
    uint32_t prog_id = 0;
    int recall_play_view_info_status_bak = g_AppPlayOps.recall_play.view_info.status;
    GxMsgProperty_NodeByPosGet ProgNode;
    GxMsgProperty_NodeNumGet prog_num = {0};
    prog_num.node_type = NODE_PROG;

    if(((g_AppPlayOps.normal_play.view_info.tv_prog_cur == g_AppPlayOps.current_play.view_info.tv_prog_cur
        &&g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV
        && g_AppPlayOps.current_play.view_info.stream_type == GXBUS_PM_PROG_TV)
        ||(g_AppPlayOps.normal_play.view_info.radio_prog_cur == g_AppPlayOps.current_play.view_info.radio_prog_cur
        &&g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO
        && g_AppPlayOps.current_play.view_info.stream_type == GXBUS_PM_PROG_RADIO))&&
        g_AppChOps.get_visible_flag(g_AppPlayOps.normal_play.play_count) == true)
    {
        play_common_prog = true;
    }
    if((!play_common_prog)&&((g_AppPlayOps.current_play.view_info.pip_main_tp_id == 0)
        || ((g_AppPlayOps.current_play.view_info.pip_main_tp_id != 0)
        && g_AppPvrOps.state  == PVR_RECORD)))
    {
        g_AppPlayOps.recall_play = g_AppPlayOps.current_play;
        g_AppPlayOps.recall_play.view_info.skip_view_switch = VIEW_INFO_SKIP_VANISH;
    }
    if(recall_play_view_info_status_bak == VIEW_INFO_DISABLE)
    {    //只要进入channel edit就可能会影响回看，需要进一步更新
        g_AppPlayOps.recall_play.view_info.skip_view_switch = VIEW_INFO_SKIP_VANISH;
        g_AppPlayOps.recall_play.view_info.status = VIEW_INFO_ENABLE;

        if(g_AppPlayOps.recall_play.view_info.stream_type == GXBUS_PM_PROG_TV)
        {
            prog_id = g_AppPlayOps.recall_play.view_info.tv_prog_cur;
        }
        else if(g_AppPlayOps.recall_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
        {
            prog_id = g_AppPlayOps.recall_play.view_info.radio_prog_cur;
        }

        GxBus_PmViewInfoMemSet(&g_AppPlayOps.recall_play.view_info);
        app_send_msg_exec(GXMSG_PM_NODE_NUM_GET, (void *)(&prog_num));
        for(i=0; i< prog_num.node_num; i++)
        {
            ProgNode.node_type = NODE_PROG;
            ProgNode.pos = i;
            app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &ProgNode);
            if(ProgNode.prog_data.id == prog_id)
            {
                    g_AppPlayOps.recall_play.play_count = i;
                break;
            }
        }
        if(i>=prog_num.node_num)//not find, prog del or skip
        {
            g_AppPlayOps.recall_play = g_AppPlayOps.normal_play;
        }
        GxBus_PmViewInfoMemClear();
    }
#endif
    return 0;
}

static bool recall_flag = false;
static int _play_cb(void *usrdata)
{
    GxMsgProperty_NodeByPosGet node = {0};

    g_AppFullArb.timer_stop();
    g_AppPmt.sync_flag=0;
    app_audio_free();
#if MULTIFEED_LIST_SUPPORT
    app_multifeed_clear();
#endif
    g_AppPmt.stop(&g_AppPmt);

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

#if AUTO_UPDATE_SUPPORT
    if((0x1fff == node.prog_data.video_pid && GXBUS_PM_PROG_TV == node.prog_data.service_type)
            || (0x1fff == node.prog_data.cur_audio_pid && GXBUS_PM_PROG_RADIO == node.prog_data.service_type))
    {
        GxPlayer_MediaStop(PLAYER_FOR_NORMAL);
        g_AppPmt.start(&g_AppPmt);
    }
#endif

    app_prepare_for_play(&node);//something must to do befor play

    if (s_play_delay_timer != NULL)
    {
        remove_timer(s_play_delay_timer);
        s_play_delay_timer = NULL;
    }
    g_AppPlayOps.normal_play.key = PLAY_KEY_DUMMY;
#if PARENTAL_LOCK_SUPPORT
    g_AppPlayOps.normal_play.parent_key = PLAY_KEY_DUMMY;
#endif
    g_AppPlayOps.normal_play.rec = PLAY_KEY_DUMMY;

    if ((PLAY_KEY_GET()
            && (node.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE))
       )
    {
        if((GXCORE_SUCCESS == GUI_CheckDialog(WND_NUMBER)))
        {
            GUI_EndDialog(WND_NUMBER);
        }

        //要播的节目与当前播放的是不同的节目或者相同节目但未播放
        if(((g_AppPlayOps.current_play.view_info.tv_prog_cur != node.prog_data.id && g_AppPlayOps.current_play.view_info.stream_type == GXBUS_PM_PROG_TV)
                || (g_AppPlayOps.current_play.view_info.radio_prog_cur != node.prog_data.id && g_AppPlayOps.current_play.view_info.stream_type == GXBUS_PM_PROG_RADIO))
                || false == app_check_av_running())
        {
            //在要播的与当前播放的是不同节目时如果存在密码框需要重新创建
            if(GXCORE_SUCCESS == GUI_CheckDialog(WND_PASSWD))
                GUI_EndDialog(WND_PASSWD);

            app_set_avcodec_flag(false);
            app_player_close(PLAYER_FOR_NORMAL);

            g_AppFullArb.tip = FULL_STATE_LOCKED; /* embeded, no way out */
            g_AppPlayOps.normal_play.key = PLAY_KEY_LOCK;
            g_AppPlayOps.normal_play.rec = PLAY_KEY_LOCK;
            g_AppPlayOps.poppwd_create(g_AppPlayOps.normal_play.play_count);
        }
    }

    if(GXCORE_SUCCESS==GUI_CheckDialog(WND_PASSWD))
    {
        if(GXCORE_SUCCESS==GUI_CheckDialog(WND_NUMBER))
        {
            GUI_EndDialog(WND_NUMBER);
            if(g_AppPlayOps.current_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
            {
                app_channel_info_end_dialog();
                app_channel_info_create_dialog();
            }
        }
    }
#if TKGS_SUPPORT
    else if ((app_channel_blocking_check(node.prog_data.id))
             && (node.prog_data.id != g_AppPvrOps.env.prog_id))
    {
        app_set_avcodec_flag(false);
        app_player_close(PLAYER_FOR_NORMAL);

        g_AppFullArb.tip = FULL_STATE_SCRAMBLE; /* embeded, no way out */
        g_AppPlayOps.normal_play.key = PLAY_KEY_LOCK;
        g_AppPlayOps.normal_play.rec = PLAY_KEY_LOCK;
    }
#endif
    else if (g_AppPlayOps.normal_play.rec == PLAY_KEY_LOCK)
    {
        g_AppFullArb.tip = FULL_STATE_LOCKED;
    }
    else if (node.prog_data.scramble_flag != GXBUS_PM_PROG_BOOL_ENABLE
             || g_AppFullArb.scramble_check() == FALSE)
    {
        g_AppFullArb.tip = FULL_STATE_DUMMY;
    }
    else
    {
        g_AppFullArb.tip = FULL_STATE_SCRAMBLE;
    }

    //g_AppFullArb.exec[EVENT_STATE](&g_AppFullArb);
    g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);

    if (g_AppPlayOps.normal_play.key == PLAY_KEY_DUMMY)
    {
        app_normal_play(&node);
    }
    if (s_rec_flag == true)
    {
        app_rec_timer_start(&node.prog_data);
    }

#if SDT_SUPPORT
    app_sdt_start(g_AppPlayOps.normal_play.dmx_id,node.prog_data.ts_id,node.prog_data.sdt_version);
#endif
#if PAT_SUPPORT
	app_pat_start(g_AppPlayOps.normal_play.dmx_id,node.prog_data.ts_id,node.prog_data.pat_version);
#endif
#if AUTO_UPDATE_SUPPORT
    if((GUI_CheckDialog(WND_CHANNEL_EDIT) != GXCORE_SUCCESS) && (g_AppPvrOps.state == PVR_DUMMY))
    {
        app_auto_update_start(&node);
    }
#endif
    g_AppTime.start(&g_AppTime);
    app_play_id_set(node.prog_data.id);
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&g_AppPlayOps.normal_play.view_info));
    if(recall_flag == true)
    {
        recall_flag = false;
    }
    else
    {
        app_update_recall_info();
    }
    if(g_AppChOps.get_visible_flag(g_AppPlayOps.normal_play.play_count) == false)
    {
        recall_flag = true;
    }
    g_AppPlayOps.current_play = g_AppPlayOps.normal_play;
	if (0 != reset_timer(s_clear_paly_flag_timer))
    {
        s_clear_paly_flag_timer = create_timer(_clear_play_flag_cb, 400, NULL, TIMER_REPEAT);
    }
    return 0;
}

static void app_program_play_exec(AppPlayMode play_mode, uint32_t play_count)
{
    GxMsgProperty_NodeByPosGet node = {0};

    if (0 >= g_AppPlayOps.normal_play.play_total)
    {
        _app_stop_subt();
        app_panel_show_prog_num(PANEL_CLEAN_PROG_NUM);
        return;
    }

    extern void app_play_run_exec_timer_rm(void);
    app_play_run_exec_timer_rm();

    APP_TIMER_REMOVE(thiz_replay_timer);
    app_fuse_spec_play_parental_lock_stop();

    if ((play_mode & PLAY_TYPE_FORCE) == PLAY_TYPE_FORCE)
    {
        s_play_exec_flag = false;
    }

    if ((play_mode & PLAY_MODE_NEXT) == PLAY_MODE_NEXT)
    {
        play_count = g_AppChOps.get_next_pos(g_AppPlayOps.normal_play.play_count);
        if(play_count == g_AppPlayOps.normal_play.play_count)
            return;
        s_next_prog_num = play_count;
    }
    else if ((play_mode & PLAY_MODE_PREV) == PLAY_MODE_PREV)
    {
        play_count = g_AppChOps.get_prev_pos(g_AppPlayOps.normal_play.play_count);
        if(play_count == g_AppPlayOps.normal_play.play_count)
            return;
        s_next_prog_num = play_count;
    }
    else if ((play_mode & PLAY_MODE_POINT) == PLAY_MODE_POINT)
    {
        if (play_count >= g_AppPlayOps.normal_play.play_total)
        {
            play_count = g_AppChOps.real_pos(0);
        }
    }

    node.node_type = NODE_PROG;
    node.pos = play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

    if (((play_mode & PLAY_MODE_WITH_REC) == PLAY_MODE_WITH_REC)
            || ((play_mode & PLAY_MODE_WITH_TMS) == PLAY_MODE_WITH_TMS))
    {
        s_rec_flag = true;

        if((app_check_av_running() == true)
                && ( g_AppPlayOps.current_play.view_info.tv_prog_cur == node.prog_data.id)
                && (g_AppPlayOps.current_play.view_info.stream_type == g_AppPlayOps.normal_play.view_info.stream_type)
                && ((play_mode & PLAY_TYPE_FORCE) != PLAY_TYPE_FORCE))
        {
            //same prog and it's running
            thiz_rec_delay_cnt = 0;
            app_rec_timer_start(&node.prog_data);
            return;
        }
        s_play_exec_flag = false;
    }
    else
    {
        s_rec_flag = false;
    }
    _app_stop_subt();
    g_AppPlayOps.normal_play.play_count = play_count;
    g_AppFullArb.state.rec = STATE_OFF;
    //	g_AppFullArb.exec[EVENT_STATE](&g_AppFullArb);
    //	g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
    app_panel_show_prog_num(0);
    APP_TIMER_REMOVE(s_clear_paly_flag_timer);

    if (s_play_exec_flag == false)
    {
        thiz_rec_delay_cnt = 0;
        if (s_play_rec_timer != NULL)
        {
            remove_timer(s_play_rec_timer);
            s_play_rec_timer = NULL;
        }
        s_play_exec_flag = true;
#if 0
        g_AppFullArb.timer_stop();
        app_subt_pause();
        g_AppTtxSubt.ttx_num = 0;
        g_AppTtxSubt.subt_num = 0;
        g_AppTtxSubt.sync_flag = 0;
        g_AppPmt.sync_flag=0;
        g_AppPmt.stop(&g_AppPmt);
#endif
        _play_cb(NULL);
    }
    else
    {
        //        g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
        if (0 != reset_timer(s_play_delay_timer))
        {
            s_play_delay_timer = create_timer(_play_cb, PLAY_DELAY_MS, NULL, TIMER_REPEAT);
        }
    }
    app_fuse_spec_play_set_lcn();
    const char *wnd = GUI_GetFocusWindow();

    if (strcmp(wnd, WND_FULL_SCREEN) == 0 &&  s_rec_flag != true)
    {
        app_channel_info_create_dialog();
    }
    else if (app_channel_info_compare_dialog(wnd) == 0 || strcmp(wnd,WND_PASSWD)==0)
    {
        app_channel_info_update_dialog();
    }
}

static void app_program_play(AppPlayMode play_mode, int32_t play_count)
{
    s_rec_restart = false;

#if TKGS_SUPPORT
    extern bool app_get_tkgs_power_status(void);
    if (app_get_tkgs_power_status() == true)
    {
        return;
    }
#endif
#if SMART_VOLUME_SUPPORT
    app_smart_volume_save_extinfo(g_AppPlayOps.normal_play.play_count);
#endif
    app_netaudio_netaudio_play_stop();
    g_AppPlayOps.running = true;
    if (g_AppPlayOps.normal_play.key == PLAY_KEY_UNLOCK)
    {
        _app_stop_subt();
        g_AppPmt.sync_flag=0;
        app_audio_free();
#if MULTIFEED_LIST_SUPPORT
        app_multifeed_clear();
#endif
        g_AppPmt.stop(&g_AppPmt);
        GxMsgProperty_NodeByPosGet node = {0};

        // for forbiden rec lock program
        g_AppPlayOps.normal_play.play_count = play_count;
        g_AppPlayOps.normal_play.rec = PLAY_KEY_UNLOCK;
        g_AppFullArb.tip = FULL_STATE_DUMMY; /* embeded, no way out */

        if (g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
        {
            node.id = g_AppPlayOps.normal_play.view_info.radio_prog_cur;
        }
        else
        {
            node.id = g_AppPlayOps.normal_play.view_info.tv_prog_cur;
        }
        node.node_type = NODE_PROG;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, (void *)(&node));
        app_normal_play(&node);
        if (s_rec_flag == true)
        {
            thiz_rec_delay_cnt = 0;
            app_rec_timer_start(&node.prog_data);
        }

        g_AppPlayOps.normal_play.key = PLAY_KEY_LOCK;
    }
    else
    {
        app_program_play_exec(play_mode, play_count);
    }
#if PARENTAL_LOCK_SUPPORT
    app_parental_rating_set(0);
#endif
}

static void app_program_stop(void)
{
    g_AppPlayOps.running = false;
    if (s_play_delay_timer != NULL)
    {
        remove_timer(s_play_delay_timer);
        s_play_delay_timer = NULL;
    }
    if (s_play_rec_timer != NULL)
    {
        remove_timer(s_play_rec_timer);
        s_play_rec_timer = NULL;
    }
    if (s_clear_paly_flag_timer != NULL)
    {
        remove_timer(s_clear_paly_flag_timer);
        s_clear_paly_flag_timer = NULL;
    }
    APP_TIMER_REMOVE(thiz_replay_timer);
    app_fuse_spec_play_parental_lock_stop();
    app_play_pid_search_stop();
    if(spApp_pmt_play_timer != NULL)
    {
        pmt_auto_update_count = 0;
        remove_timer(spApp_pmt_play_timer);
        spApp_pmt_play_timer=NULL;
    }
#if PDMX_SUPPORT
    app_pdmx_play_stop(g_AppPlayOps.normal_play.tuner);
#endif
    //memset(&g_AppPlayOps.current_play, 0, sizeof(PlayInfo));

    app_netaudio_netaudio_play_stop();

#if BLUETOOTH_SUPPORT
    extern int app_bluez_get_source_connect_statue(void);
    extern int app_bluez_source_pause(void);
    if ( app_bluez_get_source_connect_statue() )
    {
        app_bluez_source_pause();
    }
#endif

    s_rec_flag = false;
    app_set_avcodec_flag(false);
    app_audio_free();
#if MULTIFEED_LIST_SUPPORT
    app_multifeed_clear();
#endif
    g_AppPmt.stop(&g_AppPmt);
#if SDT_SUPPORT
    app_sdt_stop();
#endif
#if PAT_SUPPORT
    app_pat_stop();
#endif
#if AUTO_UPDATE_SUPPORT
    app_auto_update_stop();
#endif
    app_subt_pause();
    app_epg_filter_pause();
    app_player_close(PLAYER_FOR_NORMAL);
#if DVB_CLOUD_SUPPORT
    void app_dvbonline_program_stop(void);
    app_dvbonline_program_stop();
#endif
#if CASCAM_SUPPORT
	app_ca_stop_descramble();
#endif
#if PRESET_PROG_INFO_CORRECT_SUPPORT
    app_preset_prog_info_correct_stop();
#endif

}

static int app_play_pwd_cb(PopPwdRet *ret, void *usr_data)
{
	if(ret->result == PASSWD_OK)
	{
		GxMsgProperty_PlayerVideoWindow play_wnd;
		play_wnd.player = PLAYER_FOR_NORMAL;
		memcpy(&(play_wnd.rect), &s_NormalRect, sizeof(PlayerWindow));
		app_send_msg_exec(GXMSG_PLAYER_VIDEO_WINDOW, (void *)(&play_wnd));

		g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
#if PARENTAL_LOCK_SUPPORT
        g_AppPlayOps.normal_play.parent_key = PLAY_KEY_UNLOCK;
#endif
        app_play_current_prog(PLAY_MODE_POINT);

        if (GUI_CheckDialog(WND_EPG) == GXCORE_SUCCESS)
        {
            GUI_SetProperty("text_epg_show_detail_tip", "string", "Detail");
        }
        app_fuse_spec_epg_refresh_tip();
	}
    else if (ret->result == PASSWD_ERROR)
    {
        PasswdDlg passwd_dlg;
        memset(&passwd_dlg, 0, sizeof(PasswdDlg));
        passwd_dlg.str = STR_ID_ERR_PASSWD;
        passwd_dlg.usr_data = usr_data;
        passwd_dlg.passwd_exit_cb = app_play_pwd_cb;

        if (app_channel_epg_check_dialog() == GXCORE_SUCCESS)
        {
            passwd_dlg.pos.x = APP_WND_EPG_POP_POS_X;
            passwd_dlg.pos.y = APP_WND_EPG_POP_POS_Y;
        }
        if (GUI_CheckDialog(WND_CHANNEL_EDIT) == GXCORE_SUCCESS)
        {
            passwd_dlg.pos.x = APP_WND_CH_EDIT_POPDLG_POS_X;
            passwd_dlg.pos.y = APP_WND_CH_EDIT_POPDLG_POS_Y;
        }

        if ((app_channel_list_check_dialog() == GXCORE_SUCCESS)
#if BOUQUET_SUPPORT
			||(GUI_CheckDialog(WND_BOUQUET) == GXCORE_SUCCESS)
#endif
            )
        {
            passwd_dlg.pos.x = APP_WND_CH_LIST_PASSWD_POS_X;
            passwd_dlg.pos.y = APP_WND_CH_LIST_PASSWD_POS_Y;
        }
        passwd_dlg_create(&passwd_dlg);
    }
    else
    {

		 if (GUI_CheckDialog(WND_EPG) == GXCORE_SUCCESS)
		 {
   		 	if(g_AppPlayOps.normal_play.key==PLAY_KEY_LOCK)//lock
   		 	{
				GUI_SetProperty("text_epg_show_detail_tip", "string", "Unlock");
			}
		 }
         s_rec_flag = false;
    }

    return 0;
}

static void  app_play_poppwd_create(int32_t pos)
{
    PasswdDlg passwd_dlg;
    static PlayPwdCbData cb_data = {0};

	if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_BOOK))
	{
		return;
	}
    if ((GXCORE_SUCCESS != GUI_CheckDialog(WND_CHANNEL_EDIT))
            && (GXCORE_SUCCESS != app_channel_list_check_dialog())
#if BOUQUET_SUPPORT
            && (GXCORE_SUCCESS != GUI_CheckDialog(WND_BOUQUET))
#endif
            && (GXCORE_SUCCESS != app_channel_epg_check_dialog())
            && (GXCORE_SUCCESS != app_fuse_spec_satellite_switch_check_dialog()))
    {
        if(GUI_CheckDialog(WND_VOLUME) == GXCORE_SUCCESS)
        {
            GUI_EndDialog(WND_VOLUME);
        }
        if(app_channel_info_check_dialog() != GXCORE_SUCCESS)
        {
            app_channel_info_create_dialog();
        }
        app_clean_fullscreen_tip(true, false, false);
    }

    cb_data.play_count = pos;
    memset(&passwd_dlg, 0, sizeof(PasswdDlg));
    passwd_dlg.passwd_exit_cb = app_play_pwd_cb;
    passwd_dlg.usr_data = &cb_data;
#if PARENTAL_LOCK_SUPPORT
    if(g_AppFullArb.tip == FULL_STATE_PARENTAL_LOCK)
    {
        passwd_dlg.str = STR_ID_PARENT_GUID;
    }
#endif
    if (app_channel_epg_check_dialog() == GXCORE_SUCCESS)
    {
        passwd_dlg.pos.x = APP_WND_EPG_POP_POS_X;
        passwd_dlg.pos.y = APP_WND_EPG_POP_POS_Y;
    }
    if (GUI_CheckDialog(WND_CHANNEL_EDIT) == GXCORE_SUCCESS)
    {
        passwd_dlg.pos.x = APP_WND_CH_EDIT_POPDLG_POS_X;
        passwd_dlg.pos.y = APP_WND_CH_EDIT_POPDLG_POS_Y;
    }

    if ((app_channel_list_check_dialog() == GXCORE_SUCCESS)
#if BOUQUET_SUPPORT
		||(GUI_CheckDialog(WND_BOUQUET) == GXCORE_SUCCESS)
#endif
		||(app_fuse_spec_satellite_switch_check_dialog() == GXCORE_SUCCESS)
        )
    {
        passwd_dlg.pos.x = APP_WND_CH_LIST_PASSWD_POS_X;
        passwd_dlg.pos.y = APP_WND_CH_LIST_PASSWD_POS_Y;
    }
    if (GXCORE_SUCCESS == GUI_CheckDialog(WND_EPG))
    {
        GUI_SetProperty("img_epg_red_tip", "state", "hide");
        GUI_SetProperty("text_epg_book_tip", "state", "hide");
        GUI_SetProperty("img_epg_show_detail_tip", "state", "show");
        GUI_SetProperty("text_epg_show_detail_tip", "state", "show");
        GUI_SetProperty("text_epg_show_detail_tip", "string", "Unlock");
    }
	popdlg_destroy();

    if (GXCORE_SUCCESS == GUI_CheckDialog(WND_COMMON_SELECT))
    {
        GUI_EndDialog(WND_COMMON_SELECT);
        app_channel_info_create_dialog();
    }

    passwd_dlg_create(&passwd_dlg);
}

static void app_play_set_rec_time(uint32_t sec)
{
    s_rec_sec = sec;
}

#if RECALL_LIST_SUPPORT
static status_t app_play_set_recall_number(PlayInfo *p_play_info)
{
    static uint8_t recall_satus = 0;//标记是否是开机第一个节???	//uint16_t pre_sate_id = 0;//上一个卫星ID,用于diseqc
    int16_t j, index_callback = MAX_RECALL_NUM;
    //	app_log_debug("[recall]app_play_set_recall_number%d!!cur =%d\n",p_play_info->view_info.stream_type,
    //		p_play_info->view_info.stream_type?p_play_info->view_info.radio_prog_cur:p_play_info->view_info.tv_prog_cur);
    //	app_log_debug("[recall]play_count =%d\n",p_play_info->play_count);

    if (p_play_info == NULL)
    {
        return GXCORE_ERROR;
    }
    if (recall_satus == 1)
    {
        //pre_sate_id = g_AppPlayOps.recall_play[1].view_info.sat_id;
        for (j = MAX_RECALL_NUM - 1; j >= 0; j--)
        {
            if ((g_AppPlayOps.recall_play[j].view_info.tv_prog_cur == p_play_info->view_info.tv_prog_cur
                    && g_AppPlayOps.recall_play[j].view_info.stream_type == p_play_info->view_info.stream_type
                    && g_AppPlayOps.recall_play[j].view_info.stream_type == GXBUS_PM_PROG_TV)
                    || (g_AppPlayOps.recall_play[j].view_info.radio_prog_cur == p_play_info->view_info.radio_prog_cur
                        && g_AppPlayOps.recall_play[j].view_info.stream_type == p_play_info->view_info.stream_type
                        && g_AppPlayOps.recall_play[j].view_info.stream_type != GXBUS_PM_PROG_TV)
               )
            {
                //app_log_debug("[recall] same j=%d\n",j);
                index_callback = j;//遍历recall取得重复节目
                break;
            }
        }
        if (index_callback > MAX_RECALL_NUM - 1)
        {
            s_recall_view_num ++;
            index_callback = MAX_RECALL_NUM - 1;
        }
        if (s_recall_view_num > MAX_RECALL_NUM)
        {
            s_recall_view_num = MAX_RECALL_NUM;
        }

        for (j = index_callback; j > 0; j--)
        {
            //依次往后push,如果没有重复则丢弃最后一个历史节???
            memcpy(&(g_AppPlayOps.recall_play[j]), &(g_AppPlayOps.recall_play[j - 1]), sizeof(PlayInfo));
        }
    }
    else
    {
        // pre_sate_id = 0XFFFF;
    }

    //当前节目保存???位置

    memcpy(&(g_AppPlayOps.recall_play[!recall_satus]), p_play_info, sizeof(PlayInfo));
    //开机第一个节目历史为自己
    if (recall_satus == 0)
    {
        memcpy(&(g_AppPlayOps.recall_play[0]), &(g_AppPlayOps.recall_play[1]), sizeof(PlayInfo));
    }

    recall_satus = 1;

    return GXCORE_SUCCESS;
}

int app_play_get_recall_total_count(void)
{
    int16_t i = MAX_RECALL_NUM - 1, j;
    GxMsgProperty_NodeByPosGet node;
    status_t ret;
    node.node_type = NODE_PROG;
    uint32_t id;
    int recall_num = s_recall_view_num;
    for (i = recall_num - 1; i > 0; i--)
    {
        if (g_AppPlayOps.recall_play[i].view_info.stream_type == GXBUS_PM_PROG_RADIO)
        {
            id = g_AppPlayOps.recall_play[i].view_info.radio_prog_cur;
        }
        else
        {
            id = g_AppPlayOps.recall_play[i].view_info.tv_prog_cur;
        }
        node.id = id;
        ret = app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node);
        //throw skip prog
        if (ret != GXCORE_SUCCESS || node.id != id || node.prog_data.skip_flag == GXBUS_PM_PROG_BOOL_ENABLE)
        {
            //app_log_debug("i=%d s_recall_view_num = %d\n",i,s_recall_view_num);
            for (j = i; j < s_recall_view_num - 1; j++)
            {
                //丢弃无法获得的节???可能已经被删???
                memcpy(&(g_AppPlayOps.recall_play[j]), &(g_AppPlayOps.recall_play[j + 1]), sizeof(PlayInfo));
            }
            s_recall_view_num --;
        }
    }

    //app_log_debug("s_recall_view_num = %d\n",s_recall_view_num);

    return s_recall_view_num;
}
#endif

#if 1
int app_program_play_set_afd_info(void)
{
    int32_t ret = 0;
    PlayerWindow video_wnd = {0, 0, APP_THEME_XRES, APP_THEME_YRES};
    PlayerProgInfo *info = app_player_prog_info_get();

    ret = GxPlayer_MediaGetProgInfoByName(PLAYER_FOR_NORMAL, info); //solution check
    if ((ret == 0) && (info->video_stat.AFD.enable == 1))
    {
        g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL, &video_wnd);
    }

    return 0;
}
#endif

int app_play_program_user_info(GxMsgProperty_NodeModify node_modify)
{
    GxMsgProperty_NodeByPosGet node = {0};

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));
    app_normal_play(&node);
    return 0;
}

static  int play_pmt_timer(void *userdata)
{
	if (pmt_auto_update_count >0 )
	{
        GxMsgProperty_NodeByPosGet node = {0};
        node.node_type = NODE_PROG;
        node.pos = g_AppPlayOps.normal_play.play_count;
        app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));
        if((0x1fff == node.prog_data.video_pid && GXBUS_PM_PROG_TV == node.prog_data.service_type)
                || (0x1fff == node.prog_data.cur_audio_pid && GXBUS_PM_PROG_RADIO == node.prog_data.service_type))
        {
            g_AppPmt.start(&g_AppPmt);
        }
        else
        {
            if(GUI_CheckDialog(WND_CHANNEL_EDIT) == GXCORE_SUCCESS)
            {
#define VIDEO_TEXT "txt_channel_edit_for_pp"
                int x,y,w,h = 0;
                PlayerWindow video_wnd;
                sscanf((widget_get_property(gui_get_widget(NULL,VIDEO_TEXT),"rect")),"[%d,%d,%d,%d]",&x,&y,&w,&h);
                video_wnd.x = x;
                video_wnd.y = y;
                video_wnd.width = w;
                video_wnd.height = h;
                g_AppPlayOps.play_window_set(PLAY_TYPE_NORMAL,&video_wnd);
                g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
                g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
                g_AppPlayOps.normal_play.key = PLAY_KEY_DUMMY;
            }
            else
            {
                g_AppFullArb.state.pause = STATE_OFF;
                g_AppFullArb.draw[EVENT_PAUSE](&g_AppFullArb);
                g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
                g_AppPlayOps.program_play(PLAY_TYPE_NORMAL|PLAY_MODE_POINT, g_AppPlayOps.normal_play.play_count);
                g_AppPlayOps.normal_play.key = PLAY_KEY_DUMMY;
            }
        }
    }
	pmt_auto_update_count = 0;
	spApp_pmt_play_timer = NULL;
	return 0;
}

void app_play_replay_from_pmt(uint32_t duration)
{
	if(g_AppPvrOps.state != PVR_DUMMY)
	{
		if((g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV
			&& g_AppPlayOps.normal_play.view_info.tv_prog_cur == g_AppPvrOps.env.prog_id)
			||(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO
			&& g_AppPlayOps.normal_play.view_info.radio_prog_cur == g_AppPvrOps.env.prog_id))
		{
			if(GUI_CheckDialog(WND_PVR_BAR) == GXCORE_SUCCESS)
			{
				if(GUI_CheckDialog(WND_DURATION_SET) == GXCORE_SUCCESS)
					GUI_EndDialog(WND_DURATION_SET);
				GUI_EndDialog(WND_PVR_BAR);
			}
			extern void app_pvr_stop(void);
			app_pvr_stop();
		}
	}
	const char *wnd = GUI_GetFocusWindow();
	if(wnd)
	{
		if(0 == strcmp(wnd,WND_CHANNEL_LIST) && LIST_MODE_PROG == app_get_channel_list_mode())
		{
            int pos = g_AppChOps.real_select(g_AppPlayOps.normal_play.play_count);
			GUI_SetProperty("listview_channel_list", "update_all", NULL);
			GUI_SetProperty("listview_channel_list", "select", &pos);
			GUI_SetProperty("listview_channel_list", "active", &pos);
		}
		else if(0 == strcmp(wnd,WND_COMMON_SELECT))
		{
			GUI_EndDialog(WND_COMMON_SELECT);
		}
        app_channel_info_signal_end_dialog();
	}
    app_update_pop_dlg(WND_POP_BOOK);
	if(((GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS && g_AppFullArb.tip == FULL_STATE_LOCKED))
            || ((GUI_CheckDialog(WND_EPG) != GXCORE_SUCCESS)
                &&(GUI_CheckDialog(WND_CHANNEL_EDIT) != GXCORE_SUCCESS)
                &&(GUI_CheckDialog(WND_MAIN_MENU) == GXCORE_SUCCESS)))
	{
		return;
	}
	if(!spApp_pmt_play_timer)
	{
		pmt_auto_update_count = 1;
		if(duration != 0)
		{
			spApp_pmt_play_timer = create_timer(play_pmt_timer, 50, NULL,  TIMER_ONCE);
		}
	}
	else
	{
		pmt_auto_update_count++;
		if (pmt_auto_update_count >=2)
		{
			app_player_close(PLAYER_FOR_NORMAL);
		}
		reset_timer(spApp_pmt_play_timer);
	}
}

AppPlayOps g_AppPlayOps =
{
    .program_play_init = app_program_play_init,
    .play_list_create = app_play_list_create,
    .program_play = app_program_play,
    .program_stop = app_program_stop,
    .play_window_set = app_play_window_set,
    .poppwd_create = app_play_poppwd_create,
    .set_rec_time = app_play_set_rec_time,
#if RECALL_LIST_SUPPORT
    .add_recall_list = app_play_set_recall_number,
#endif
    .running = false,
};


// for fist boot
void app_play_frontend_config(GxMsgProperty_NodeByPosGet *prog_node)
{
    GxMsgProperty_NodeByIdGet SatNode = {0};
    GxMsgProperty_NodeByIdGet TpNode = {0};
    AppFrontend_SetTp params = {0};
    AppFrontend_Config config = {0};

    if(NULL == prog_node)
    {
        return ;
    }
    // SAT node
    SatNode.node_type = NODE_SAT;
    SatNode.id = prog_node->prog_data.sat_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &SatNode);
    // TP node
    TpNode.node_type = NODE_TP;
    TpNode.id = prog_node->prog_data.tp_id;
    app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &TpNode);

    // get frontend config
    app_ioctl(prog_node->prog_data.tuner, FRONTEND_CONFIG_GET, &config);

#if NDEMOD_WITH_1TUNER
    app_only_cur_frontend_monitor_start(SatNode.sat_data.tuner);
    if (SatNode.sat_data.type == GXBUS_PM_SAT_DTMB)
    {
        // lnb polar off
        AppFrontend_PolarState polar = FRONTEND_POLAR_OFF;
        GxFrontend_SetVoltage(prog_node->prog_data.tuner, polar);
    }
#endif

#if MULTISTREAM_SUPPORT
    if(prog_node->prog_data.muti_ts_exist == 1)
    {
        app_set_muti_ts_para(SatNode.sat_data.tuner,prog_node->prog_data.muti_ts_id, prog_node->prog_data.muti_ts_code);
    }
#endif

#if PDMX_SUPPORT
    app_pdmx_prepare(prog_node->prog_data.tuner, (void*)&prog_node->prog_data);
#endif

#if CA_SUPPORT
    if (prog_node->prog_data.scramble_flag)
    {
        //TODO:
        extern int app_send_prepare(ServiceClass *param);
        ServiceClass param = {0};
        unsigned int casid[1];

        casid[0] = prog_node->prog_data.cas_id;
        param.cas_id = casid[0];
        app_send_prepare(&param);

        if(((prog_node->prog_data.cas_id) & 0xf00) == 0x900)
        {
            app_ca_set_flag(0);
        }
        if((((prog_node->prog_data.cas_id) & 0xf00) == 0xe00))
        {
            app_ca_set_flag(1);
        }
        app_ca_set_flag(2);

    }
    else
    {
        app_ca_clear_flag();
    }

#if CMM_SUPPORT
    static int thiz_cache_size_status = 0;
    if((prog_node->prog_data.scramble_flag)
            &&(prog_node->prog_data.cas_id >=0x0e00)
            &&(prog_node->prog_data.cas_id <=0x0eff))
    {
        if(thiz_cache_size_status == 0)
        {
            GxBus_ConfigSetInt(PLAYER_CONFIG_DELAY_CACHE_HW_SIZE, 188*1024*2);
            thiz_cache_size_status = 0xff;
        }
    }
    else
    {
        if(thiz_cache_size_status == 0xff)
        {
            thiz_cache_size_status = 0;
            GxBus_ConfigSetInt(PLAYER_CONFIG_DELAY_CACHE_HW_SIZE, 188*1024*10);
        }
    }
#endif
#endif
    // save the params
    app_set_tp(prog_node->prog_data.tuner, &params, SAVE_TP_PARAM);
#if DEMOD_DVB_S
    app_play_set_diseqc(prog_node->prog_data.sat_id, prog_node->prog_data.tp_id, true);
#endif
    //set tp if necessary
    app_play_set_tp(SatNode, TpNode, *prog_node);
}

void app_first_play(GxMsgProperty_NodeByPosGet *prog_node)
{
    AppFrontend_Config config;

    // frontend control
    // app_play_frontend_config(prog_node);
    // check for play

    if(prog_node->prog_data.scramble_flag == GXBUS_PM_PROG_BOOL_ENABLE)
    {
        g_AppFullArb.scramble_set(1);
    }

    if ((PLAY_KEY_GET()
            && (prog_node->prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE))
#if TKGS_SUPPORT
       ||  (app_channel_blocking_check(prog_node->prog_data.id))
#endif
        )
        return ;

    GxMsgProperty_PlayerPlay* play = NULL;
    play = GxCore_Mallocz(sizeof(GxMsgProperty_PlayerPlay));
    if(play == NULL)
    {
        app_log_error("%s:Malloc failed!!!!!!!\n",__func__);
        return ;
    }
    g_AppPlayOps.running = true;//dvb2ip need

    app_ioctl(prog_node->prog_data.tuner, FRONTEND_CONFIG_GET, &config);
    g_AppPlayOps.normal_play.tuner = config.tuner;
    g_AppPlayOps.normal_play.dmx_id = config.dmx_id;
#if PDMX_SUPPORT
    if(true == app_program_need_predemux(prog_node)
            && false == app_frontend_hard_cap(prog_node->prog_data.tuner, prog_node->prog_data.pdmx_flag))
    {
        g_AppPlayOps.normal_play.ts_src = 3;
    }
    else
#endif
    {
        g_AppPlayOps.normal_play.ts_src = config.ts_src;
    }

#if CA_SUPPORT
    app_descrambler_close_by_control_type(CONTROL_NORMAL);
#endif
    // get play url
    app_player_url_get(play->url, &prog_node->prog_data,
            g_AppPlayOps.normal_play.ts_src, g_AppPlayOps.normal_play.dmx_id);
#if QUICK_SWITCH_SUPPORT
    char url_tmp[100] = {0};
    snprintf(url_tmp, sizeof(url_tmp), "&tscache_ms:%d&tscache_mode:%d&tscache_dmxid:%d&tscache_bufsize:%d", 600, 2, g_AppPlayOps.normal_play.dmx_id + 1, 10240000);
    strcat(play->url, url_tmp);
#endif
    play->player = PLAYER_FOR_NORMAL;
    // play
#if CASCAM_SUPPORT
	app_ca_stop_descramble();
#endif
    GxPlayer_MediaPlay(play->player, play->url, 0, 0, NULL);//&play.window
    GXCORE_FREE(play);
#if CASCAM_SUPPORT
    {
        CascamSwitchChannel switch_channel;
        switch_channel.pmt_pid = prog_node->prog_data.pmt_pid;
        switch_channel.service_id = prog_node->prog_data.service_id;
        switch_channel.audio_pid = prog_node->prog_data.cur_audio_pid;
        switch_channel.video_pid = prog_node->prog_data.video_pid;
        app_ca_switch_channel(switch_channel);
    }
#endif
#if PDMX_SUPPORT
    {
        GxMsgProperty_NodeByIdGet tp_get = {0};

        tp_get.node_type = NODE_TP;
        tp_get.id = prog_node->prog_data.tp_id;
        app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &tp_get);
        app_pdmx_play_start(g_AppPlayOps.normal_play.tuner, config.ts_src, config.dmx_id,
                tp_get.tp_data.tp_s.stream_size,  (void*)&(prog_node->prog_data));
    }
#endif
    // volume set
    int32_t audio_vol = 0;
    if (app_volume_scope_status() == VOLUME_CHANNEL)
    {
        audio_vol = prog_node->prog_data.audio_volume;
        app_system_vol_set(audio_vol);
    }
    else
    {
        GxBus_ConfigGetInt(AUDIO_VOLUME_KEY, &audio_vol, AUDIO_VOLUME);
        app_system_vol_set(audio_vol);
    }
#if AUDIO_DESCRIPTOR_SUPPORT
#if AUDIO_DESCRIPTOR_VOLUME_SET_SUPPORT
    {
        int ad_vol_offset = 0;
        GxBus_ConfigGetInt(AD_VOL_OFFSET_KEY, &ad_vol_offset, AD_VOL_OFFSET_VALUE);
        app_system_ad_volume_set(ad_vol_offset);
    }
#endif
#endif
// audio track set
    {
        int track_sel = 0;
        if (prog_node->prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_MONO)
        {
            track_sel = 0;
        }
        else if (prog_node->prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_LEFT)
        {
            track_sel = 1;
        }
        else if (prog_node->prog_data.audio_mode == GXBUS_PM_PROG_AUDIO_MODE_RIGHT)
        {
            track_sel = 2;
        }
        else
        {
            track_sel = 3;
        }
        app_send_msg_exec(GXMSG_PLAYER_AUDIO_TRACK, &track_sel);
    }

    app_play_update_prog_tp_id(prog_node->prog_data.tp_id);
}


void app_first_play_after(void)
{
    GxMsgProperty_NodeByPosGet node = {0};
    AppFrontend_Config config = {0};

    // get play node
    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

#if AUTO_UPDATE_SUPPORT
    if((0x1fff == node.prog_data.video_pid && GXBUS_PM_PROG_TV == node.prog_data.service_type)
            || (0x1fff == node.prog_data.cur_audio_pid && GXBUS_PM_PROG_RADIO == node.prog_data.service_type))
    {
        g_AppPmt.start(&g_AppPmt);
    }
#endif
    // get frontend config
    app_ioctl(node.prog_data.tuner, FRONTEND_CONFIG_GET, &config);

    {
        g_AppPlayOps.normal_play.tuner = node.prog_data.tuner;
#if TKGS_SUPPORT
        g_AppTkgsBackUpgrade.init(node.prog_data.sat_id, node.prog_data.tp_id, node.prog_data.tuner);
#endif
        prog_ts_id = node.prog_data.ts_id;
    }

        #if PRESET_PROG_INFO_CORRECT_SUPPORT
        if(true == app_preset_prog_info_correct_running_check())
            app_preset_prog_info_correct_stop();
        app_preset_prog_info_correct_start(config.ts_src, node.prog_data.tp_id);
#endif

    // Disable epg when TMS/Predemux is open
    if ((PVR_DUMMY == g_AppPvrOps.state || PVR_RECORD == g_AppPvrOps.state) && !app_program_need_predemux(&node))
        app_epg_enable(config.ts_src, TS_2_DMX);
    else
        app_epg_disable();

    //
    g_AppPlayOps.normal_play.key = PLAY_KEY_DUMMY;
#if PARENTAL_LOCK_SUPPORT
    g_AppPlayOps.normal_play.parent_key = PLAY_KEY_DUMMY;
#endif
    g_AppPlayOps.normal_play.rec = PLAY_KEY_DUMMY;
    // prog lock control
    if ((PLAY_KEY_GET()
            && (node.prog_data.lock_flag == GXBUS_PM_PROG_BOOL_ENABLE))
       )
    {
        g_AppFullArb.tip = FULL_STATE_LOCKED; /* embeded, no way out */
        g_AppPlayOps.normal_play.key = PLAY_KEY_LOCK;
        g_AppPlayOps.normal_play.rec = PLAY_KEY_LOCK;
        g_AppPlayOps.poppwd_create(g_AppPlayOps.normal_play.play_count);
    }

    if(GXCORE_SUCCESS==GUI_CheckDialog(WND_PASSWD))
    {
        ;
    }
#if TKGS_SUPPORT
    else if ((app_channel_blocking_check(node.prog_data.id))
             && (node.prog_data.id != g_AppPvrOps.env.prog_id))
    {
        app_set_avcodec_flag(false);
        g_AppFullArb.tip = FULL_STATE_SCRAMBLE; /* embeded, no way out */
        g_AppPlayOps.normal_play.key = PLAY_KEY_LOCK;
        g_AppPlayOps.normal_play.rec = PLAY_KEY_LOCK;
    }
#endif
    else if (node.prog_data.scramble_flag != GXBUS_PM_PROG_BOOL_ENABLE
             || g_AppFullArb.scramble_check() == FALSE)
    {
        g_AppFullArb.tip = FULL_STATE_DUMMY;
    }
    else
    {
        g_AppFullArb.tip = FULL_STATE_SCRAMBLE;
    }

    g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);

    if (g_AppPlayOps.normal_play.key == PLAY_KEY_DUMMY)
    {
       // app_play_stage1(&node);
#if PANEL_SETTING_SUPPORT
        app_panel_show_prog_num(PANEL_DISPLAY_TIME);
#endif
#if SAT2IP_SERVER_SUPPORT
        int app_sat2ip_play_change(GxBusPmDataProg *prog);
        app_sat2ip_play_change(&node.prog_data);
#endif
#if DVB_CLOUD_SUPPORT
        void app_dvbonline_program_play(GxBusPmDataProg *prog);
        app_dvbonline_program_play(&node.prog_data);
#endif
    }

#if SDT_SUPPORT
    app_sdt_start(g_AppPlayOps.normal_play.dmx_id,node.prog_data.ts_id,node.prog_data.sdt_version);
#endif
#if PAT_SUPPORT
	app_pat_start(g_AppPlayOps.normal_play.dmx_id,node.prog_data.ts_id,node.prog_data.pat_version);
#endif
#if AUTO_UPDATE_SUPPORT
    app_auto_update_start(&node);
#endif
    g_AppTime.start(&g_AppTime);
    app_play_id_set(node.prog_data.id);
	app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&g_AppPlayOps.normal_play.view_info));
    // recall control
    app_update_recall_info();
    if(g_AppChOps.get_visible_flag(g_AppPlayOps.normal_play.play_count) == false)
    {
        recall_flag = true;
    }
    g_AppPlayOps.current_play = g_AppPlayOps.normal_play;

    extern int app_play_start_pid_empty_check(void);
    app_play_start_pid_empty_check();
}

void app_play_control_module_sys_init(void)
{
	s_NormalRect.width  = APP_THEME_XRES;
	s_NormalRect.height = APP_THEME_YRES;
}

/* End of file -------------------------------------------------------------*/
