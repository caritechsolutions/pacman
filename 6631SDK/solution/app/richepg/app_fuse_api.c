/*
    20240722
*/

#include "app.h"
#include "app_fuse_api.h"
#include "app_windows.h"

#if RICHEPG_SUPPORT
#include "app_eutel_common.h"
#include "richepg.h"
#include "richepg_file_splice.h"
#include "app_eutel_database.h"
#include "channel_common.h"
#include <common/gxpm.h>
#endif
#if PVR_RECORDING_POSTER_STYLE
#include "app_cover_pvr.h"
extern int app_pvr_file_info_update(const char *path, const char *name);
extern int app_pvr_file_info_save(void);
extern int app_cover_pvr_info_create_dialog(bool reverse_flag);
#endif


// freescan api
extern status_t app_pvr_media_update(void);
extern int  app_channel_info_show_video_info(PlayerVideoTrack video, GxBusPmDataProgDefinition definition);
extern void app_channel_info_update(void);
extern int  app_channel_info_clear_epg_info(void);
extern void app_create_epg_menu(void);

// eutel api
#if RICHEPG_SUPPORT

//待机醒来准备维护开始，不关闭视频输出，可以查看维护是否正常
//#define API_FUSE_WAKE_FROM_REBOOT_FLAG_VIDEO_OUT_DEBUG
#define API_FUSE_FREESCAN_TDT_CHECK_DEBUG

#define APP_FUSE_WAKE_FROM_REBOOT_FLAG  (3) //app_utility.h WakeState 扩展

static int s_low_power_wake_up_value = 0 ;

static event_list *eutel_play_parental_lock_timer = NULL;

extern void app_low_power(time_t sleep_time, uint32_t scan_code);
extern int  app_eutel_parental_lock_info_check(void);
extern int  app_eutel_core_switch_work_mode(int change_mode,int sat_id,int cur_mode,int cur_group,int cur_sat);
extern void app_eutel_epg_event_timer_set(bool bflag);
extern void app_eutel_epg2_event_timer_set(bool bflag, int book_id);
extern void app_eutel_epg2_ok_tip_refresh(void);
extern int  app_eutel_pvr_file_info_update(const char *path, const char *name);
extern int  app_eutel_pvr_file_info_save(void);
extern void app_create_pvr_media_fullsreeen(void);
extern void app_clean_fullscreen_tip(bool osd_trans_hide, bool end_pop_dlg, bool end_info_wnd);
extern void app_eutel_chinfo_show_video_info(PlayerVideoTrack video, GxBusPmDataProgDefinition definition);
extern void app_eutel_chinfo_update(void);
extern void app_eutel_chinfo_clear_epg_info(void);

//mode = 1 HD
int app_richepg_prog_check_callback(GxBusPmDataProg* prog,GxBusPmViewInfo* sys,int mode)
{
    if ( prog == NULL || sys == NULL )
        return 0;

    if (mode == 1)
    {
        if (app_eutel_sat_type_check(prog->sat_id))
            return -1 ;
        else
            return 0 ;
    }

    if (sys->group_mode == GROUP_MODE_SAT)
    {
        return 0 ;
    }

    if (app_eutel_sat_type_check(prog->sat_id))
        return -1 ;

    return 0 ;
}

static int32_t _eutel_play_parental_lock_func(void *usrdata)
{
	if ( app_fuse_spec_is_sattv_work_mode() )
	{
		app_eutel_parental_lock_info_check();
	}
	return 0 ;
}

static int _app_fuse_spec_usb_is_not_exist(void)
{
	if (richepg_get_profile_type(USB_ECL_TIMESTAMP) != RICHEPG_FULL_PROFILE)
		return 1;
	return 0 ;
}

static int _app_eutel_get_power_on_timer(void)
{
	int init_value = 0;
	GxBus_ConfigGetInt(RICHEPG_POWER_ON_TIME_KEY, &init_value, RICHEPG_POWER_ON_TIME_DEF);
	return init_value;
}

static void _app_eutel_set_power_on_timer(int init_value)
{
	int value = 0 ;
	value = _app_eutel_get_power_on_timer();
	if (value != init_value)
	{
		GxBus_ConfigSetInt(RICHEPG_POWER_ON_TIME_KEY, init_value);
		app_log_min("power on timer set=%d , old=%d \n",init_value,value);
	}
}

static int  _app_eutel_is_wake_from_reboot_flag(void)
{
	unsigned int sWakeFlag = 0;
	GxCore_GetWakeFlag(&sWakeFlag);
	app_log_min("app get wake flag = %d \n",sWakeFlag);
	if(sWakeFlag == APP_FUSE_WAKE_FROM_REBOOT_FLAG)
	{
		return 1 ;
	}
	return 0 ;
}

static int  _app_eutel_wake_from_reboot_close_video_out(void)
{
	if( _app_eutel_is_wake_from_reboot_flag() )
	{
		app_log_min("close video out1 \n");
		app_log_min("close video out2 \n");
		s_low_power_wake_up_value = 1 ;
#ifdef API_FUSE_WAKE_FROM_REBOOT_FLAG_VIDEO_OUT_DEBUG
		app_log_info("close video debug \n");
#else
		GxPlayer_SetVideoOutputPower(0);
#endif
	}
	return 0 ;
}

int  app_fuse_spec_get_wake_from_reboot_memory_flag_value(void)
{
	return s_low_power_wake_up_value ;
}
#endif

/*
 *   fuse spec api ===========================================================
 */
int app_fuse_spec_set_prog_check_callback(void)
{
#if RICHEPG_SUPPORT
    GxBus_PmSetPmProgCheckCallback(app_richepg_prog_check_callback);
#endif
    return 0 ;
}

int app_fuse_spec_del_prog_check_callback(void)
{
#if RICHEPG_SUPPORT
    GxBus_PmSetPmProgCheckCallback(NULL);
#endif
    return 0 ;
}

int  app_fuse_spec_play_parental_lock_start(void)
{
#if RICHEPG_SUPPORT
	app_eutel_parental_rating_set(0);
	if ( app_fuse_spec_is_sattv_work_mode() )
	{
		APP_TIMER_ADD(eutel_play_parental_lock_timer, _eutel_play_parental_lock_func,
				3000, TIMER_REPEAT);
	}
#endif
	return 0 ;
}

int  app_fuse_spec_play_parental_lock_stop(void)
{
#if RICHEPG_SUPPORT
	APP_TIMER_REMOVE(eutel_play_parental_lock_timer);
	app_eutel_parental_rating_set(0);
#endif
	return 0 ;
}

int  app_fuse_spec_play_set_lcn(void)
{
#if RICHEPG_SUPPORT
	if ( app_fuse_spec_is_sattv_work_mode() )
	{
		eutel_channel_prog_public_set_lcn(g_AppPlayOps.normal_play.play_count);
	}
#endif
	return 0 ;
}

int  app_fuse_spec_is_sattv_work_mode(void)
{
#if RICHEPG_SUPPORT
	if ((g_AppPlayOps.normal_play.view_info.group_mode == GROUP_MODE_SAT ) &&
		(app_eutel_sat_type_check(g_AppPlayOps.normal_play.view_info.sat_id)))
	{
		return 1 ;
	}
#endif
	return 0 ;
}

int  app_fuse_spec_is_sattv_view_mode(int cur_type,int check_type,int group,int sat_id)
{
#if RICHEPG_SUPPORT
	if (check_type == GXBUS_PM_PROG_RADIO || check_type == GXBUS_PM_PROG_TV )
	{
		if ( cur_type != check_type )
			return 0 ;
	}
	if ((group == GROUP_MODE_SAT ) && (app_eutel_sat_type_check(sat_id)))
	{
		return 1 ;
	}
#endif
	return 0 ;
}

int  app_fuse_spec_switch_work_mode(int mode)
{
#if RICHEPG_SUPPORT
	int sat_id = 0 ;
	int count = 0 ;

	if ( APP_FUSE_SATTV_WORK_MODE == mode )
	{
		if ( !app_richepg_mode_get() )
		{
			count = app_fuse_spec_add_group_id(NULL,0);
			if (count > 0 )
			{
				app_richepg_mode_set(1);
				sat_id = app_eutel_cur_sat_id_get();
				app_eutel_core_switch_work_data(1,sat_id,-1,-1,1);
			}
		}
	}
	else if (APP_FUSE_FREE_SCAN_WORK_MODE == mode)
	{
		if ( app_richepg_mode_get() )
		{
			app_richepg_mode_set(0);
			app_eutel_core_switch_work_data(0,sat_id,-1,-1,1);
		}
	}
#endif
	return 0 ;
}

int  app_fuse_spec_prepare_switch_work_env(int set_mode,int group,int sat_id)
{
#if RICHEPG_SUPPORT
	int change_mode = 0 ;
	if ((group == GROUP_MODE_SAT ) && (app_eutel_sat_type_check(sat_id)))
	{
		change_mode = 1 ;
		if (change_mode == set_mode )
		{
			app_richepg_mode_set(1);
		}
		return 0 ;
	}
	else
	{
		change_mode = 0 ;
		if (change_mode == set_mode )
		{
			app_richepg_mode_set(0);
		}
		return 1 ;
	}
#endif
	return 2 ;
}

int  app_fuse_spec_switch_work_env(int group,int sat_id)
{
#if RICHEPG_SUPPORT
	// 1 sat tv mode
	int cur_mode = 0 ;
	int change_mode = 0 ;
	int cur_group = 0;
	int cur_sat   = 0;

	cur_group = g_AppPlayOps.normal_play.view_info.group_mode ;
	cur_sat   = g_AppPlayOps.normal_play.view_info.sat_id ;

	if (( cur_group == GROUP_MODE_SAT ) && (app_eutel_sat_type_check(cur_sat)))
	{
		cur_mode = 1 ;
	}
	else
	{
		cur_mode = 0 ;
	}

	if ((group == GROUP_MODE_SAT ) && (app_eutel_sat_type_check(sat_id)))
	{
		change_mode = 1 ;
	}
	else
	{
		change_mode = 0 ;
	}

	if ( (cur_mode != change_mode) || ((cur_mode) && (g_AppPlayOps.normal_play.view_info.sat_id != sat_id)) )
	{
		return app_eutel_core_switch_work_mode(change_mode,sat_id,cur_mode,cur_group,cur_sat);
	}
#endif
	return 0 ;
}

int  app_fuse_spec_switch_work_edit_group_change(void)
{
#if RICHEPG_SUPPORT
	// 1 sat tv mode
	int change_mode = 0 ;
	int group = 0;
	int sat_id   = 0;
	bool cur_mode ;

	group  = g_AppPlayOps.normal_play.view_info.group_mode ;
	sat_id = g_AppPlayOps.normal_play.view_info.sat_id ;

	if (( group == GROUP_MODE_SAT ) && (app_eutel_sat_type_check(sat_id)))
	{
		change_mode = 1 ;
	}
	else
	{
		change_mode = 0 ;
	}
	cur_mode = app_richepg_mode_get();

	if ( change_mode == 1 )
	{
		if (!cur_mode)
		{
			app_richepg_mode_set(1);
		}

		if ( eutel_channel_prog_public_is_full_mode(sat_id) )
		{
			app_log_min("edit full mode \n");
			eutel_channel_ctrl_data_easy_save();
			if (cur_mode)
			{
				richepg_epg_build_enable_set(false, true);
				richepg_epg_ctrl_release();
			}
			return 0 ;
		}

#ifdef APP_EUTEL_CHANNEL_EASY_DATA_SUPPORT
		if ( app_eutel_channel_easy_data_is_ready(sat_id) )
		{
			app_log_min("edit easy mode \n");
			if (cur_mode)
			{
				richepg_epg_build_enable_set(false, true);
				richepg_epg_ctrl_release();
			}
			return 0 ;
		}
#endif
		if ( app_eutel_core_switch_work_data(change_mode,sat_id,120,234,0) < 0 )
		{
			app_richepg_mode_set(0);
			return -1 ;
		}
	}
	else
	{
		if (cur_mode)
		{
			app_richepg_mode_set(0);
			app_eutel_core_switch_work_data(change_mode,sat_id,120,234,0);
		}
	}

#endif
	return 0 ;
}

#if RICHEPG_SUPPORT
static void _fuse_spec_eutel_channel_easy_data_uninit(void)
{
#ifdef APP_EUTEL_CHANNEL_EASY_DATA_SUPPORT
#ifdef APP_EUTEL_CHANNEL_EASY_SAVE_ONLY_EDIT
	app_eutel_channel_easy_data_uninit();
#endif
#endif
}
#endif

int  app_fuse_spec_switch_work_edit_exit(void)
{
#if RICHEPG_SUPPORT
	// 1 sat tv mode
	int change_mode = 0 ;
	int group = 0;
	int sat_id   = 0;
	int ret = 0 ;

	group  = g_AppPlayOps.normal_play.view_info.group_mode ;
	sat_id = g_AppPlayOps.normal_play.view_info.sat_id ;

	if (( group == GROUP_MODE_SAT ) && (app_eutel_sat_type_check(sat_id)))
	{
		change_mode = 1 ;
	}
	else
	{
		change_mode = 0 ;
	}

	if ( change_mode == 1 )
	{
		if ( eutel_channel_prog_public_is_full_mode(sat_id) )
		{
			app_log_min("edit exit full mode \n");
			richepg_epg_build_enable_set(true, true);
			richepg_epg_build(true);

			_fuse_spec_eutel_channel_easy_data_uninit();
			return 0 ;
		}
		ret = app_eutel_core_switch_work_data(change_mode,sat_id,120,234,1);
		_fuse_spec_eutel_channel_easy_data_uninit();
		return ret ;
	}
	_fuse_spec_eutel_channel_easy_data_uninit();
#endif
	return 0 ;
}

int  app_fuse_spec_switch_work_group_change(void)
{
#if RICHEPG_SUPPORT
	// 1 sat tv mode
	int change_mode = 0 ;
	int group = 0;
	int sat_id   = 0;
	bool cur_mode ;
	PopDlg pop;

	group  = g_AppPlayOps.normal_play.view_info.group_mode ;
	sat_id = g_AppPlayOps.normal_play.view_info.sat_id ;

	if (( group == GROUP_MODE_SAT ) && (app_eutel_sat_type_check(sat_id)))
	{
		change_mode = 1 ;
	}
	else
	{
		change_mode = 0 ;
	}
	cur_mode = app_richepg_mode_get();

	if ( change_mode == 1 )
	{
		if (!cur_mode)
			app_richepg_mode_set(1);

		if (GUI_CheckDialog(WND_CHANNEL_LIST) == GXCORE_SUCCESS)
			GUI_EndDialog(WND_CHANNEL_LIST);
		if (GUI_CheckDialog(WND_EUTEL_CHLIST) == GXCORE_SUCCESS)
			GUI_EndDialog(WND_EUTEL_CHLIST);

		if ( eutel_channel_prog_public_is_full_mode(sat_id) )
		{
			g_AppPlayOps.play_list_create(&(g_AppPlayOps.normal_play.view_info));
			g_AppPlayOps.normal_play.play_count = g_AppChOps.get_cur_pos(g_AppPlayOps.normal_play.play_count);
			eutel_channel_prog_public_set_lcn(g_AppPlayOps.normal_play.play_count);
			app_eutel_chlist_create_dialog();
			app_play_current_prog(PLAY_MODE_POINT);
			richepg_epg_build_enable_set(true, true);
			richepg_epg_build(true);
			return 1 ;
		}

		if ( app_eutel_core_switch_work_data(change_mode,sat_id,500,100,1) < 0 )
		{
			if (!cur_mode)
			{
				app_richepg_mode_set(0);
				g_AppPlayOps.normal_play.view_info.group_mode = GROUP_MODE_ALL;
			}

			memset(&pop, 0, sizeof(PopDlg));
			pop.type = POP_TYPE_NO_BTN;
			pop.format = POP_FORMAT_DLG;
			pop.mode = POP_MODE_UNBLOCK;
			pop.pos.x = 500;
			pop.pos.y = 100;
			pop.str = "can not change,please check usb";
			pop.create_cb = NULL;
			pop.exit_cb = NULL;
			pop.timeout_sec = 3;

			return -1 ;
		}

		g_AppPlayOps.normal_play.play_count = g_AppChOps.get_cur_pos(g_AppPlayOps.normal_play.play_count);
		eutel_channel_prog_public_set_lcn(g_AppPlayOps.normal_play.play_count);
		app_eutel_chlist_create_dialog();
		app_play_current_prog(PLAY_MODE_POINT);
		//GUI_SetInterface("flush", NULL);
		return 1 ;
	}
	else
	{
		if (cur_mode)
		{
			app_richepg_mode_set(0);
			app_eutel_core_switch_work_data(change_mode,sat_id,500,100,1);
		}
	}
#endif
	return 0 ;
}

int  app_fuse_spec_add_group_id(int* p_satid,int size)
{
#if RICHEPG_SUPPORT
	int i ;
	int cur_id ;
	int count = 0 ;
	RichepgTsMgr *mgr = richepg_ts_mgr_get();
	if (mgr && mgr->sat_array)
	{
		for (i = 0; i < mgr->sat_num; i++)
		{
			if ( mgr->sat_array[i].lineup_id == 0 )
				continue ;
			cur_id = mgr->sat_array[i].sat_id ;

			if ( i != mgr->sat_sel )
			{
				if (  _app_fuse_spec_usb_is_not_exist() )
				{
					app_log_min("add group pass sat id = %d \n",cur_id);
					continue ;
				}
			}

			if ( (p_satid != NULL) && (count < size) )
			{
				p_satid[count] = cur_id ;
			}

			count++ ;
        }
	}
	return count ;
#endif
    return 0 ;
}

int  app_fuse_spec_tv_radio_num(int type)
{
#if RICHEPG_SUPPORT
	GxBusPmViewInfo view_info;
	if ( app_fuse_spec_add_group_id(NULL,0) )
	{
		app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, (void *)(&view_info));
		view_info.group_mode = GROUP_MODE_SAT;
		view_info.sat_id = app_eutel_cur_sat_id_get();
		view_info.stream_type = type;
		return GxBus_PmProgNumGetByViewInfo(&view_info);
	}
#endif
	return 0 ;
}

int  app_fuse_spec_sat_key_is_redefine(void)
{
#if RICHEPG_SUPPORT
    if (app_eutel_sat_list_create_dialog())
    {
		GUI_EndDialog(WND_CHANNEL_LIST);
		GUI_SetInterface("flush", NULL);
        return 1 ;
    }
#endif
    return 0;
}

int  app_fuse_spec_get_maint_wake_up_time(unsigned int wake_time)
{
#if RICHEPG_SUPPORT

    GxTime sys_time = {0};
    unsigned int cur_zone_sec ;
    unsigned int cur_hour ;
    unsigned int cur_seconds ;
    unsigned int durs = 0;
    unsigned int attach_durs = 0;
    unsigned int new_time = 0;
    unsigned int power_on_time = 0 ;
    int attach_maint_hour = -1;

    time_t last_update_sec = 0;
    last_update_sec = richepg_store_getint(RICHEPG_EPG_TIMESTAMP);
    if (0 == last_update_sec)
    {
        last_update_sec = richepg_store_getint(RICHEPG_ECL_TIMESTAMP);
        if (0 == last_update_sec)
        {
            return 0;
        }
    }

    GxCore_GetLocalTime(&sys_time);
    cur_zone_sec = sys_time.seconds + get_display_time_by_timezone(0);

    cur_hour    = cur_zone_sec % 86400 / 3600 ;
    cur_seconds = cur_zone_sec % 3600 ;
    app_log_min("sys time=%d,cur time=%d \n", sys_time.seconds,cur_zone_sec);

    //Off -1
    attach_maint_hour = app_richepg_update_attach_get();
    if ( (attach_maint_hour >= 0) && (attach_maint_hour < 24) )
    {
        if (cur_hour < attach_maint_hour)
        {
            attach_durs = (attach_maint_hour-cur_hour)*3600  - cur_seconds ;
        }
        else if (cur_hour == attach_maint_hour)
        {
            attach_durs = 86400 - cur_seconds;
        }
        else
        {
            attach_durs = (24-cur_hour+attach_maint_hour)*3600  - cur_seconds ;
        }
        if (attach_durs <= 0)
            attach_durs = 1 ;
    }
    if (cur_hour < AUTO_MAINTENANCE_HOUR)
    {
        durs = (AUTO_MAINTENANCE_HOUR-cur_hour)*3600  - cur_seconds ;
    }
    else if (cur_hour == AUTO_MAINTENANCE_HOUR)
    {
        durs = 86400 - cur_seconds;
    }
    else
    {
        durs = (24-cur_hour+AUTO_MAINTENANCE_HOUR)*3600  - cur_seconds ;
    }
    if (durs <= 0)
        durs = 1 ;

    app_log_min("attach durs =%d,default durs=%d \n", attach_durs,durs);
    if ( (attach_durs > 0) && (attach_durs < durs) )
    {
        durs = attach_durs ;
    }

    if ( (wake_time <= 0) || (wake_time >= durs) )
    {
        new_time = durs ;

        if (wake_time >= durs)
        {
            power_on_time = wake_time-durs ;
        }
        else
        {
            power_on_time = 0 ;
        }
    }
    else
    {
        new_time = 0 ;
        power_on_time = 0 ;
    }

    app_log_min("fuse time=%d,power on time=%d \n",durs,wake_time);
    _app_eutel_set_power_on_timer(power_on_time);
    app_log_min("get wake up time = %d \n", new_time);
    return new_time ;
#else
    return 0 ;
#endif
}

int  app_fuse_spec_wake_from_reboot_flag_keypress(int key_event)
{
#if RICHEPG_SUPPORT
#ifdef API_FUSE_FREESCAN_TDT_CHECK_DEBUG
	GxTime sys_time = {0};
	int i = 0 ;
	unsigned int cur_zone_sec ;
	unsigned int cur_hour ;
	int attach_maint_hour = -1;
#endif
	unsigned int wake_time = 0;
	unsigned int tick_time = 0;
	unsigned int sWakeFlag = 0;
	if (!s_low_power_wake_up_value)
	{
		return 0 ;
	}
	GxCore_GetWakeFlag(&sWakeFlag);
	if(sWakeFlag != APP_FUSE_WAKE_FROM_REBOOT_FLAG)
	{
		return 0 ;
	}

	if (key_event > 0)
	{
		app_log_min("key event \n");

		if (key_event != STBK_HALT)
		{
			app_log_min("need halt key \n");
			return 1 ;
		}
		app_log_min("open video out \n");
		_app_eutel_set_power_on_timer(0);
		s_low_power_wake_up_value = 0 ;
		//视频输出打开
#ifdef API_FUSE_WAKE_FROM_REBOOT_FLAG_VIDEO_OUT_DEBUG
		app_log_info("open video out debug \n");
#else
		GxPlayer_SetVideoOutputPower(1);
#endif
		return 1 ;
	}
	else
	{
		app_log_min("continue standby \n");

		wake_time = _app_eutel_get_power_on_timer();
		app_log_min("continue power on time = %d \n",wake_time);
		if (wake_time > 0 )
		{
			tick_time = app_tick_time_sec_get();
			if (wake_time <= tick_time )
			{
				wake_time = 1 ;
			}
			else
			{
				wake_time = wake_time - tick_time ;
			}
			app_log_min("continue wake time = %d,tick = %d\n",wake_time,tick_time);
		}

#ifdef API_FUSE_FREESCAN_TDT_CHECK_DEBUG
		if ( (key_event<0 ) && (!app_richepg_mode_get()) )
		{
			attach_maint_hour = app_richepg_update_attach_get();

			for(i=0;i<600;i++)
			{
				GxCore_GetLocalTime(&sys_time);
				cur_zone_sec = sys_time.seconds + get_display_time_by_timezone(0);
				cur_hour     = cur_zone_sec % 86400 / 3600 ;

				if ( (cur_hour != AUTO_MAINTENANCE_HOUR) && (cur_hour != attach_maint_hour))
				{
					app_log_min("tdt start %d,%d %d \n",cur_hour,attach_maint_hour,cur_zone_sec);
					GxCore_ThreadDelay(100);
					GUI_LoopEvent();
					app_log_min("tdt end \n");
				}
				else
				{
					app_log_min("tdt finish = %d \n",i);
					break;
				}
			}
		}
#endif
		s_low_power_wake_up_value = 0 ;

		app_low_power(wake_time,0);
	}
#endif
	return 0 ;
}

/*--------------- init ------------ */
int  app_fuse_spec_system_init(void)
{
#if RICHEPG_SUPPORT
	bool mode ;
	_app_eutel_wake_from_reboot_close_video_out();
	mode = app_richepg_mode_get();
	app_log_min("fuse init mode = %d \n",mode);
	app_fuse_spec_set_prog_check_callback();
	app_richepg_init_all();
	if (false == mode || 1 == richepg_store_getint(RICHEPG_ECL_STATE))
	{
		if(app_richepg_mode_get())
		{
			PopDlg pop;
			memset(&pop, 0, sizeof(PopDlg));
			pop.type = POP_TYPE_NO_BTN;
			pop.format = POP_FORMAT_DLG;
			pop.mode = POP_MODE_UNBLOCK;
			pop.str = "system is boot up, please wait...";
			pop.timeout_sec= 100;
			popdlg_create(&pop);
			GUI_SetInterface("flush", NULL);

			app_eutel_time_update();
			app_eutel_proglist_skip_set(false);
			eutel_channel_ctrl_build();
			eutel_channel_group_all_build();
			eutel_lineup_logo_key_add();
			eutel_channel_play_set_lcn_first();

			popdlg_destroy();

			return APP_FUSE_SATTV_WORK_MODE ;
		}
		else
		{
			app_log_min("fuse init free \n");
			app_eutel_proglist_skip_set(true);
		}
    }
	else
	{
		if(app_richepg_mode_get())
		{
			app_log_min("fuse init modify mode \n");//set mode is freescan
			app_richepg_mode_set(0);
		}
	}
#endif
	return 0;
}

int  app_fuse_spec_system_after_init(int mode)
{
#if RICHEPG_SUPPORT
	if ( APP_FUSE_SATTV_WORK_MODE == mode )
	{
		richepg_epg_build(true);
		app_eutel_maint_check_start();
	}
	else
	{
#ifdef APP_EUTEL_MAINT_CHECK_ANY_MODE
		app_eutel_maint_check_start();
#endif
	}
#endif
	return 0;
}

int  app_fuse_spec_system_reset(void)
{
#if RICHEPG_SUPPORT
    app_eutel_database_reset();
#endif
    return 0 ;
}

int  app_fuse_spec_gui_reverse(void)
{
#if RICHEPG_SUPPORT
    return app_richepg_gui_reverse();
#endif
    return 1;
}


int  app_fuse_spec_build_by_language_change(unsigned int lang)
{
#if RICHEPG_SUPPORT
    richepg_store_setstr(RICHEPG_LOCALE_LANGUAGE, app_eutel_porting_get_iso6392_code(lang));
    richepg_store_setint(RICHEPG_EPG_FORCE_UPDATE, 1);
    richepg_store_flush();
    if (app_richepg_mode_get())
    {
        // eutelsat要求多语言切换时也要切换节目等相关的多语言
        if (richepg_store_getint(RICHEPG_ECL_STATE))
        {
            eutel_channel_ctrl_build();
            eutel_channel_group_all_build();
        }
        if (richepg_store_getint(RICHEPG_EPG_STATE)
                && richepg_get_profile_type(USB_EPG_TIMESTAMP) == RICHEPG_FULL_PROFILE)
        {
            richepg_epg_build(true);
        }
    }
#endif
	return 0;
}

int  app_fuse_spec_epg_start_work(void)
{
#if RICHEPG_SUPPORT
	richepg_epg_build_enable_set(true, false);
#endif
	return 0;
}

int  app_fuse_spec_epg_stop_work(void)
{
#if RICHEPG_SUPPORT
    richepg_epg_build_enable_set(false, false);
    while (richepg_epg_build_working_check())
        GxCore_ThreadDelay(10);
#endif
	return 0;
}

int  app_fuse_spec_epg_set_timer_event(int type,bool bflag, int book_id)
{
#if RICHEPG_SUPPORT
	if ( GUI_CheckDialog(WND_EUTEL_EPG) == GXCORE_SUCCESS )
	{
		app_eutel_epg_event_timer_set(true);
	}
	if  ( GUI_CheckDialog(WND_EUTEL_EPG2) == GXCORE_SUCCESS )
	{
		app_eutel_epg2_event_timer_set(true, book_id);
	}
#endif
	return 0 ;
}

int  app_fuse_spec_epg_refresh_tip(void)
{
#if RICHEPG_SUPPORT
	app_eutel_epg2_ok_tip_refresh();
#endif
	return 0 ;
}

int  app_fuse_spec_get_pvr_statue(void)
{
#if RICHEPG_SUPPORT
    int richepg_flag = 0;
    richepg_flag = app_richepg_mode_get();
    return (richepg_flag);
#else
    return 0 ;
#endif

}

int  app_fuse_spec_pvr_file_info_update(const char *path, const char *name)
{
#if PVR_RECORDING_POSTER_STYLE
#if RICHEPG_SUPPORT
	return app_eutel_pvr_file_info_update(path,name);
#else
	return app_pvr_file_info_update(path,name);
#endif
#endif
    return 0 ;
}

int  app_fuse_spec_pvr_file_info_save(void)
{
#if PVR_RECORDING_POSTER_STYLE
#if RICHEPG_SUPPORT
	return app_eutel_pvr_file_info_save();
#else
	return app_pvr_file_info_save();
#endif
#endif
    return 0 ;
}

int  app_fuse_spec_pvr_record_create_fullsreeen(void)
{
#if RICHEPG_SUPPORT
	app_create_pvr_media_fullsreeen();
#endif
	return 0 ;
}

int  app_fuse_spec_full_screen_create_dialog(GUI_Event *event,int change_flag)
{
#if RICHEPG_SUPPORT
	app_clean_fullscreen_tip(false, true, true);
	if (event->key.sym == STBK_SAT)
	{
		if (g_AppPvrOps.state != PVR_DUMMY)
        {
            app_pvr_create_no_btn_popdlg(NULL);
            return 1;
        }
		app_eutel_sat_list_create_dialog();
		return 1 ;
	}
    if (app_richepg_mode_get())
    {
		app_eutel_chlist_create_dialog();
		if (change_flag)
		{
			GUI_SendEvent(WND_EUTEL_CHLIST, event);
		}
		return 1 ;
    }
    else
#endif
    {
        return 0 ;
    }
}

int  app_fuse_spec_full_screen_check_dialog(const char *wnd)
{
#if RICHEPG_SUPPORT
	if ( app_richepg_mode_get() )
	{
		 if ( (!( (0 == strcmp(wnd,WND_NUMBER)) && (0 == app_fuse_common_channel_list_check_dialog()) ) )
				&& (app_fuse_common_epg_check_dialog() != GXCORE_SUCCESS)
#if PVR_RECORDING_POSTER_STYLE
				&& (GUI_CheckDialog(WND_COVER_PVR) != GXCORE_SUCCESS)
#endif
        )
		{
			return 1 ;
		}
		else
			return 0 ;
	}
	else
	{
		return 1 ;
	}
#else
	return 1;
#endif
}

int  app_fuse_spec_proc_top_hotplug_in(char *hotplug_dev)
{
#if RICHEPG_SUPPORT
    if(GUI_CheckDialog(WND_FULL_SCREEN) != GXCORE_SUCCESS)
        return 0 ;
    if (app_richepg_mode_get())
    {
        app_eutel_epg2_bak_timer_stop();
        if (richepg_store_getint(RICHEPG_ECL_STATE)
              && richepg_get_profile_type(USB_ECL_TIMESTAMP) == RICHEPG_FULL_PROFILE)
        {
            richepg_files_splice_index_json_parse_all();
            eutel_channel_logo_key_refresh();
        }
        if (richepg_store_getint(RICHEPG_EPG_STATE)
              && richepg_get_profile_type(USB_EPG_TIMESTAMP) == RICHEPG_FULL_PROFILE)
        {
            app_eutel_epg_plug_in_out();
			richepg_epg_build(true);
        }
        app_fuse_common_channel_info_update_dialog();
    }
#endif
    return 0;
}

int  app_fuse_spec_proc_top_hotplug_out(char *hotplug_dev)
{
#if PVR_RECORDING_POSTER_STYLE
    if(GUI_CheckDialog(WND_COVER_PVR) == GXCORE_SUCCESS) // eutelsat record
    {
        app_cover_pvr_hotplug(hotplug_dev);
    }
#endif

#if RICHEPG_SUPPORT
    if (app_richepg_mode_get())
    {
        app_eutel_epg2_bak_timer_stop();
        if (richepg_store_getint(RICHEPG_ECL_STATE))
        {
            eutel_channel_logo_key_refresh();
        }
        if (richepg_store_getint(RICHEPG_EPG_STATE))
        {
            app_eutel_epg_plug_in_out();
            richepg_epg_build(true);
        }
     }
#endif
    return 0 ;
}

int  app_fuse_spec_proc_top_book_trigger(void)
{
#if RICHEPG_SUPPORT
	if (app_eutel_process_check_all_no_stop())
	{
		app_log_min("fuse can't stop eutelsat process, ignore book. \n");
		return 1 ;
	}
	if(GUI_CheckDialog(WND_EUTEL_COLLECT) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_EUTEL_COLLECT);
	}
#endif
	return 0 ;
}

int  app_fuse_spec_sat_type_check(int sat_id)
{
#if RICHEPG_SUPPORT
	return app_eutel_sat_type_check(sat_id);
#endif
	return 0;
}

int  app_fuse_spec_satellite_switch_check_dialog(void)
{
#if RICHEPG_SUPPORT
	if (GUI_CheckDialog(WND_EUTEL_SAT_LIST) == GXCORE_SUCCESS)
		return GXCORE_SUCCESS;
#endif
	return GXCORE_ERROR;
}

/* ---------------------------------------------------------------------------
 *  通用接口 common api
 */

char* app_fuse_common_get_pragram_num_str(unsigned int sel)
{
    int lcn = 0 ;
    static char lcn_str[8];
    if (app_fuse_spec_is_sattv_work_mode())
    {
#if RICHEPG_SUPPORT
        lcn = app_eutel_porting_channel_get_lcn(sel);
#endif
        memset(lcn_str,0,8);
        snprintf(lcn_str, 8, "%04d", lcn);
        return lcn_str;
    }
    else
    {
        //snprintf(lcn_str, sizeof(lcn_str), "%d", sel + 1);
        //return lcn_str;
        return app_channel_num_str_get(NULL, sel);
    }
}

int  app_fuse_common_channel_info_update_video_info(PlayerVideoTrack video, GxBusPmDataProgDefinition definition)
{
#if RICHEPG_SUPPORT
	if (app_richepg_mode_get())
	{
		app_eutel_chinfo_show_video_info(video, definition);
	}
	else
#endif
	{
		app_channel_info_show_video_info(video, definition);
	}
	return 0 ;
}

int  app_fuse_common_channel_info_check_dialog(void)
{
#if RICHEPG_SUPPORT
	if (app_richepg_mode_get())
	{
		if (GUI_CheckDialog(WND_EUTEL_CHINFO) == GXCORE_SUCCESS)
			return GXCORE_SUCCESS;
	}
	else
#endif
	{
		if (GUI_CheckDialog(WND_CHANNEL_INFO) == GXCORE_SUCCESS)
			return GXCORE_SUCCESS;
	}
	return GXCORE_ERROR;
}

int  app_fuse_common_channel_info_compare_dialog(const char *wnd)
{
	if ((strcasecmp(WND_CHANNEL_INFO, wnd) == 0)
#if RICHEPG_SUPPORT
			|| (strcasecmp(WND_EUTEL_CHINFO, wnd) == 0)
#endif
	   )
	{
		return 0;
	}
	return -1;
}

int  app_fuse_common_channel_info_create_dialog(void)
{
#if RICHEPG_SUPPORT
	if (app_richepg_mode_get())
	{
		app_eutel_chinfo_create_dialog();
	}
	else
#endif
	{
		if(GUI_CheckDialog(WND_CHANNEL_LIST) != GXCORE_SUCCESS)
			GUI_CreateDialog(WND_CHANNEL_INFO);
	}
	return 0 ;
}

int  app_fuse_common_channel_info_end_dialog(void)
{
#if RICHEPG_SUPPORT
	if (GUI_CheckDialog(WND_EUTEL_CHINFO) == GXCORE_SUCCESS)
		GUI_EndDialog(WND_EUTEL_CHINFO);
#endif
	if (GUI_CheckDialog(WND_CHANNEL_INFO) == GXCORE_SUCCESS)
		GUI_EndDialog(WND_CHANNEL_INFO);
	return 0 ;
}

int  app_fuse_common_channel_info_signal_check_dialog(void)
{
#if RICHEPG_SUPPORT
	if (app_richepg_mode_get())
	{
		if (GUI_CheckDialog(WND_EUTEL_CHMORE) == GXCORE_SUCCESS)
			return GXCORE_SUCCESS;
	}
	else
#endif
	{
		if (GUI_CheckDialog(WND_CHANNEL_INFO_SIGNAL) == GXCORE_SUCCESS)
			return GXCORE_SUCCESS;
	}
	return GXCORE_ERROR;
}

int app_fuse_common_channel_info_signal_compare_dialog(const char *wnd)
{
	if ((strcasecmp(WND_CHANNEL_INFO_SIGNAL, wnd) == 0)
#if RICHEPG_SUPPORT
			|| (strcasecmp(WND_EUTEL_CHMORE, wnd) == 0)
#endif
	   )
	{
		return 0;
	}
	return -1;
}

int app_fuse_common_channel_info_signal_end_dialog(void)
{
#if RICHEPG_SUPPORT
	if (GUI_CheckDialog(WND_EUTEL_CHMORE) == GXCORE_SUCCESS)
		GUI_EndDialog(WND_EUTEL_CHMORE);
#endif
	if (GUI_CheckDialog(WND_CHANNEL_INFO_SIGNAL) == GXCORE_SUCCESS)
		GUI_EndDialog(WND_CHANNEL_INFO_SIGNAL);
	return 0 ;
}

int  app_fuse_common_channel_info_update_dialog(void)
{
	if (app_fuse_common_channel_info_check_dialog() == GXCORE_SUCCESS)
	{
#if RICHEPG_SUPPORT
		if (app_richepg_mode_get())
		{
			app_eutel_chinfo_update();
		}
		else
#endif
		{
			app_channel_info_update();
		}
	}
	return 0 ;
}

int  app_fuse_common_channel_info_update_under_pop_dlg(void)
{
#if RICHEPG_SUPPORT
	if (app_richepg_mode_get())
	{
		app_update_pop_dlg(WND_EUTEL_CHINFO);
	}
	else
#endif
	{
		app_update_pop_dlg(WND_CHANNEL_INFO);
	}
	return 0;
}

int  app_fuse_common_channel_info_clear_epg_str(void)
{
#if RICHEPG_SUPPORT
	if (app_richepg_mode_get())
	{
		app_eutel_chinfo_clear_epg_info();
	}
	else
#endif
	{
		app_channel_info_clear_epg_info();
	}
	return 0 ;
}

int  app_fuse_common_channel_list_check_dialog(void)
{
#if RICHEPG_SUPPORT
	if (app_richepg_mode_get())
	{
		if (GUI_CheckDialog(WND_EUTEL_CHLIST) == GXCORE_SUCCESS)
			return GXCORE_SUCCESS;
	}
	else
#endif
	{
		if (GUI_CheckDialog(WND_CHANNEL_LIST) == GXCORE_SUCCESS)
			return GXCORE_SUCCESS;
	}
	return GXCORE_ERROR;
}

int  app_fuse_common_channel_list_compare_dialog(const char *wnd)
{
	if ((strcasecmp(WND_CHANNEL_LIST, wnd) == 0)
#if RICHEPG_SUPPORT
			|| (strcasecmp(WND_EUTEL_CHLIST, wnd) == 0)
#endif
	   )
	{
		return 0;
	}
	return -1;
}

int  app_fuse_common_epg_check_dialog(void)
{
#if RICHEPG_SUPPORT
	if (app_richepg_mode_get())
	{
		if (GUI_CheckDialog(WND_EUTEL_EPG) == GXCORE_SUCCESS
				|| GUI_CheckDialog(WND_EUTEL_EPG2) == GXCORE_SUCCESS)
			return GXCORE_SUCCESS;
	}
	else
#endif
	{
		if (GUI_CheckDialog(WND_EPG) == GXCORE_SUCCESS)
			return GXCORE_SUCCESS;
	}
	return GXCORE_ERROR;
}

int  app_fuse_common_epg_compare_dialog(const char *wnd)
{
	if ((strcasecmp(WND_EPG, wnd) == 0)
#if RICHEPG_SUPPORT
			|| (strcasecmp(WND_EUTEL_EPG, wnd) == 0)
			|| (strcasecmp(WND_EUTEL_EPG2, wnd) == 0)
#endif
	   )
	{
		return 0;
	}
	return -1;
}

int  app_fuse_common_epg_create_dialog(void)
{
#if RICHEPG_SUPPORT
    if (app_richepg_mode_get())
    {
        if (app_richepg_epg_view_style_get())
            app_eutel_epg_create_dialog();
        else
            app_eutel_epg2_create_dialog();
    }
    else
#endif
    {
        app_create_epg_menu();
    }
	return 0;
}

int  app_fuse_common_pvr_record_pre_create_dialog(int on_fullscreen)
{
#if PVR_RECORDING_LIST_STYLE
    if (app_pvr_media_update() == GXCORE_SUCCESS)
    {
        return 1 ;
    }
#else
    bool reverse_flag = false;
    const char *menu_logo = "cover_pvr_icon";
    #if RICHEPG_SUPPORT
    reverse_flag = app_richepg_gui_reverse();
    menu_logo = "richepg_company";
    #endif
    app_cover_pvr_manage_init(on_fullscreen, reverse_flag, menu_logo);
    if (app_cover_pvr_ctrl_build() == 0)
    {
        return 1 ;
    }
#endif
    return 0 ;
}

int  app_fuse_common_pvr_record_create_dialog(void)
{
#if PVR_RECORDING_LIST_STYLE
    GUI_CreateDialog(WND_PVR_MEIDA);
#else
    app_cover_pvr_create_dialog();
#endif
    return 0 ;
}

int app_fuse_common_pvr_info_create_dialog(void)
{
#if PVR_RECORDING_POSTER_STYLE
    bool reverse_flag = false;
    if (GXCORE_SUCCESS == GUI_CheckDialog(WND_COVER_PVR))
    {
        #if RICHEPG_SUPPORT
        reverse_flag = app_richepg_gui_reverse();
        #endif
        return app_cover_pvr_info_create_dialog(reverse_flag);
    }
#endif
    return -1;
}

