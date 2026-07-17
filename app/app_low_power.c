#include "app.h"
#include "app_config.h"
#include "app_book.h"
#include "module/app_panel.h"
#include "module/app_ioctl.h"
#include "full_screen.h"
#include <common/gxpm.h>
#include "app_windows.h"
#include "app_wnd_search.h"
#include "app_channel_info.h"
#include "module/app_time.h"
#if HDMI_CEC_SUPPORT
#include "av/hal/gxav_hal_vout.h"
#include "hdmi_cec/app_cec_link.h"
#include "hdmi_cec/app_cec_config.h"
#endif
#if NETWORK_SUPPORT && GPRS_SUPPORT
#include "module/app_gprs.h"
#endif
#define SIMPLE_LOWPOWER_SUPPORT 0

#if TKGS_SUPPORT
#include "module/tkgs/gxtkgs.h"
#include "app_tkgs_upgrade.h"
static void app_tkgs_power_on(void);
static void app_tkgs_power_off(time_t sleep_time);
extern APP_TKGS_UPGRADE_MODE app_tkgs_get_upgrade_mode(void);
extern GxTkgsOperatingMode app_tkgs_get_operatemode(void);
#endif
#if DVB_CLOUD_SUPPORT
#include <gx_dvbonline.h>
#endif

#if HDMI_CEC_SUPPORT
extern int app_cec_init(void);
#endif

extern FullArb g_AppFullArb;

static bool s_simple_lowpower = false;
static event_list* s_simple_wakeup_timer = NULL;
static event_list* s_panel_led_timer = NULL;
static bool s_reboot_flag = false;

void app_simple_power_on(void)
{
    unsigned int led_time = PANEL_END_TIME;
    uint32_t standby = 0;
    uint64_t lock_time_out = 0 ;

    APP_TIMER_REMOVE(s_simple_wakeup_timer);
    APP_TIMER_REMOVE(s_panel_led_timer);

    app_panel_ioctl_handle(PANEL_SHOW_STANDBY, &standby);
    app_panel_ioctl_handle(PANEL_SHOW_TIME, &led_time);
    // for unmute
    app_set_hardware_unmute();

#if NDEMOD_WITH_1TUNER
    app_only_cur_frontend_monitor_start(app_tuner_cur_tuner_get());
#else
    app_all_frontend_monitor_control(FRONTEND_MONITOR_ON, false);
#endif

    app_power_gpio_set_on();

    const char *full_focus = NULL;
    AppFrontend_LockState lock = FRONTEND_UNLOCK;
    full_focus = GUI_GetFocusWindow();
    if ( (full_focus && (0 == strcmp(WND_FULL_SCREEN, full_focus)))
            || ( GXCORE_SUCCESS == app_channel_info_check_dialog())
            || ( GXCORE_SUCCESS == app_channel_list_check_dialog())
            || ( GXCORE_SUCCESS == app_channel_epg_check_dialog())
            || ( GXCORE_SUCCESS == GUI_CheckDialog(WND_CHANNEL_EDIT)))
    {
        if(true == g_AppPlayOps.running) // wait lock after power gpio on
        {
            lock_time_out = GxCore_TickStart(5000); // 5000 ms > frontend monitor duration
            do
            {
                app_ioctl(g_AppPlayOps.normal_play.tuner, FRONTEND_LOCK_STATE_GET, &lock);
                if ( lock == FRONTEND_LOCKED )
                    break;
                GxCore_ThreadDelay(100);
            }while( !GxCore_TickEnd(lock_time_out));
        }
        else //replay after program stop
        {
            app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
        }
    }

    GxPlayer_SetVideoOutputPower(1);//1 open output

    if (g_AppFullArb.state.mute == STATE_OFF)
    {
        GxMsgProperty_PlayerAudioMute player_mute = 0;
        app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
    }

#if HDMI_CEC_SUPPORT
	app_cec_init();
#endif
}

static int _panel_show_time(void *userdata)
{
    time_t led_time = 0;
    struct tm *temp_tm = NULL;
    led_time = g_AppTime.utc_get(&g_AppTime);
    led_time = get_display_time_by_timezone(led_time);
    temp_tm = localtime(&led_time);
    led_time = temp_tm->tm_hour * 100 + temp_tm->tm_min;
    app_panel_ioctl_handle(PANEL_SHOW_TIME, &led_time);

    return 0;
}

#if PANEL_SETTING_SUPPORT
void app_panel_undisplay_time(void)
{
    unsigned int led_time = 0;

    APP_TIMER_REMOVE(s_panel_led_timer);
    app_panel_ioctl_handle(PANEL_SHOW_TIME, &led_time);

    return ;
}

void app_panel_display_time(void)
{
    int32_t led_display;

    GxBus_ConfigGetInt(PANEL_LED_DISPLAY_KEY, &led_display, PANEL_LED_DISPLAY);
    if (led_display == 1) /*0 show channel No 1 Show time*/
    {
        _panel_show_time(NULL);
        APP_TIMER_ADD(s_panel_led_timer, _panel_show_time, 1000, TIMER_REPEAT);
    }
    else
    {
        app_panel_undisplay_time();
    }
}
#endif

static int _simple_power_wakeup(void *userdata)
{
    s_simple_wakeup_timer = NULL;
    app_simple_power_on();
    s_simple_lowpower = false;
    return 0;
}

void app_simple_power_off(time_t sleep_time)
{
    unsigned int lock = 0;
    unsigned int standby = 1;

    _panel_show_time(NULL);
    APP_TIMER_ADD(s_panel_led_timer, _panel_show_time, 1000, TIMER_REPEAT);

    if (g_AppFullArb.state.mute == STATE_OFF)
    {
        GxMsgProperty_PlayerAudioMute player_mute = 1;
        app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
    }

    GxPlayer_SetVideoOutputPower(0); //0 close output

    app_all_frontend_monitor_control(FRONTEND_MONITOR_OFF, false);

    app_panel_ioctl_handle(PANEL_SHOW_LOCK, &lock);
    app_panel_ioctl_handle(PANEL_SHOW_STANDBY, &standby);
 // for mute
    app_set_hardware_mute();
    app_power_gpio_set_off();

    if(sleep_time > 0)
    {
        APP_TIMER_ADD(s_simple_wakeup_timer, _simple_power_wakeup, sleep_time * 1000, TIMER_ONCE);
	}
}

static void app_entry_lowpower(time_t wake_time)
{
#define MAX_CMD_LEN 1024
    //mcu mode cmdline="keys=0xxxxx,0xxxxx,0xxx"
    //cpu mode cmdline="keys=0xxxxx,0xxxxx powerkey=0xxx"
    unsigned int *val = NULL;
    int num = 0;
    int i = 0;
    int cmd_len = 0;
    char temp[32] = {0};
    char buf[64] = {0};
    unsigned int powerkey = 0;
    char* cmd = NULL;
    struct lowpower_info_s lowpower;
    unsigned int port_mask = 0;
    unsigned int port_data = 0;
    int init_value = 0;

    app_bsp_get_gpio_port(&port_mask,&port_data);
    lowpower.WakeTime = wake_time;
    lowpower.GpioMask = port_mask;
    lowpower.GpioData = port_data;
    lowpower.key = 0x00;
    num = app_get_keymap_data("GUIK_H", &val); //irr power key
    cmd_len = 0;
    cmd = GxCore_Calloc(1, MAX_CMD_LEN);
    if(cmd != NULL)
    {
        if(num > 0)
        {
            strcat(cmd, "keys=");
            cmd_len += 5;
            for(i = 0; i < num; i++)
            {
                if(num == 19)
                    break;
                memset(temp, 0, sizeof(temp));
                sprintf(temp, "0x%x,", val[i]);
                cmd_len += strlen(temp);
                if(cmd_len > MAX_CMD_LEN)
                {
                    goto cmd_overflow;
                }
                strcat(cmd, temp);
            }
            cmd[cmd_len - 1] = 0;
        }

        powerkey = app_get_panel_key(); //keyboard power key
        if(powerkey)
        {
            strcat(cmd,",");
            memset(temp, 0, sizeof(temp));
            sprintf(temp, "0x%x", powerkey);
            cmd_len += strlen(temp);
            if(cmd_len > MAX_CMD_LEN)
            {
                goto cmd_overflow;
            }
            strcat(cmd, temp);
        }
        memset(temp, 0, sizeof(temp));
        memset(buf, 0, sizeof(buf));
        if(GXCORE_SUCCESS == app_get_gpio_config(GPIO_POWERCUT,temp,sizeof(temp)))
        {
            sprintf(buf, " powercut=%s", temp);
            cmd_len += strlen(buf);
            if(cmd_len > MAX_CMD_LEN)
            {
                goto cmd_overflow;
            }
            strcat(cmd, buf);
        }
        memset(temp, 0, sizeof(temp));
        memset(buf, 0, sizeof(buf));
        if(GXCORE_SUCCESS == app_get_gpio_config(GPIO_PANEL,temp,sizeof(temp)))
        {
            sprintf(buf," panelio=%s",temp);
            cmd_len += strlen(buf);
            if(cmd_len > MAX_CMD_LEN)
            {
                goto cmd_overflow;
            }
            strcat(cmd, buf);
        }

        //Get Xtal clock by parse gpio.xml in solutions/project not by CHIP.
        memset(temp, 0, sizeof(temp));
        memset(buf, 0, sizeof(buf));
        if(GXCORE_SUCCESS == app_get_gpio_config(GPIO_XTAL,temp,sizeof(temp)))
        {
            sprintf(buf, " lowpower_clock_speed=%s",temp);
            cmd_len += strlen(buf);
            if(cmd_len > MAX_CMD_LEN)
            {
                goto cmd_overflow;
            }
            strcat(cmd, buf);
        }

#if STANDBY_SHOW_TIME_SHPPORT
        int zone, half_zone, summer;
        GxBus_ConfigGetInt(PANEL_LED_STANDBY_TIME_KEY, &init_value, PANEL_STANDBY_TIME);
        cmd_len += 15;
        if(cmd_len > MAX_CMD_LEN)
        {
            goto cmd_overflow;
        }
        if (init_value == 1)
        {
            strcat(cmd, " timeshowflag=1"); //panel time show flag

        }
        else
        {
            strcat(cmd, " timeshowflag=0"); //panel time show flag
        }
        app_time_zone_config_get(&zone, &half_zone, &summer);
        memset(temp, 0, sizeof(temp));
        sprintf(temp, " timesummer=%d", summer);
        cmd_len += strlen(temp);
        if(cmd_len > MAX_CMD_LEN)
        {
            goto cmd_overflow;
        }
        strcat(cmd, temp);
        memset(temp, 0, sizeof(temp));
        sprintf(temp, " timezone=%g", zone+0.5*half_zone);
        cmd_len += strlen(temp);
        if(cmd_len > MAX_CMD_LEN)
        {
            goto cmd_overflow;
        }
        strcat(cmd, temp);
#endif
#if HDMI_CEC_SUPPORT
        int cec_value = 0;
        GxBus_ConfigGetInt(CEC_CONF_KEY, &cec_value, CEC_CONFIG_VALUE);
        if(cec_value == 1)
        {
            if(app_book_sleep_type_get() == BOOK_PROGRAM_PVR)
            {
                strcat(cmd, " cecmode=2");
            }
			else if ((wake_time == 1)
			        ||(s_reboot_flag == true)
			        ||(!app_cec_get_tv_on_stb_activity_enable_flag())
			        ||(!app_cec_get_stb_standby_tv_ativity_enable_flag()))
			{
			    strcat(cmd, " cecmode=0");
			}
            else
            {
                strcat(cmd, " cecmode=1");
            }
        }
        else
        {
            strcat(cmd, " cecmode=0");
        }
#endif
        memset(temp, 0, sizeof(temp));
        app_panel_get_int(PANEL_LED_STANDBY, &init_value);
        sprintf(temp, " panel_dot_ctrl=0x%x", ((1<<(init_value+4))|(1<<init_value)));
        cmd_len += strlen(temp);
        if(cmd_len > MAX_CMD_LEN)
        {
            goto cmd_overflow;
        }
        strcat(cmd, temp);
#if PANEL_SETTING_SUPPORT
        memset(temp, 0, sizeof(temp));
        int panel_brightness = 0;
        GxBus_ConfigGetInt(PANEL_LED_BRIGHTNESS_KEY, &panel_brightness, PANEL_LED_BRIGHTNESS);
        if(panel_brightness == 0)
            sprintf(temp, " panel_brightness=1");
        else if(panel_brightness == 1)
            sprintf(temp, " panel_brightness=4");
        else
            sprintf(temp, " panel_brightness=8");
        cmd_len += strlen(temp);
        if(cmd_len > MAX_CMD_LEN)
        {
            goto cmd_overflow;
        }
        strcat(cmd, temp);
#endif

        /* 待机的时候，获取下一次维护的启动时间，同时和Power On时间wake_time对比。
         * Power On 待机时间小，则Power On待机，等醒来后，还没到维护时间。
         * 如果维护待机时间小，返回值大于0,同时把rebootflag=1传入低功耗。同时如果这个时候Power On的时间会记录下来，
         * 维护结束进行待机中，会再次比较时间。add 2024114
         */
        unsigned int fuse_time = 0;
        fuse_time = app_fuse_spec_get_maint_wake_up_time(wake_time);
        if ( fuse_time > 0 )
        {
           lowpower.WakeTime = fuse_time;
           strcat(cmd, " rebootflag=1");
        }
    }
    app_log_debug("####[%s]%d cmd: %s####\n", __func__, __LINE__, cmd);
    APP_FREE(val);

    lowpower.cmdline = (unsigned char*)cmd;
    GxCore_Halt(&lowpower);
    GxCore_Free(cmd);
    return;
cmd_overflow:
    app_log_error("cmd overflow!!!!!!!\n");
    GxCore_Free(cmd);

    return ;
}

#if TKGS_SUPPORT
extern uint8_t s_tkgs_force_stop ;
/*
* 退出到指定界面后，响应对应的遥控器或创建指定的窗体
*/
static void app_win_exist_to_win_widget(const char* widget_win_name)
{
	GUI_Event keyEvent = {0};
	int rtn = 1;

    if (NULL != widget_win_name)
    {
        char* focus_Window = (char*)GUI_GetFocusWindow();
        //app_log_debug("%s %s %d focus_Window=%s\n",__FILE__,__func__,__LINE__,focus_Window);
        if (NULL != focus_Window)
        {
            rtn = strcasecmp(widget_win_name, focus_Window);
            while(rtn != 0)
            {

                if (NULL != focus_Window)
                {
                    //app_log_debug("%s %s %d focus_Window=%s\n",__FILE__,__func__,__LINE__,focus_Window);

                    if ((0 == strcasecmp(WND_FULL_SCREEN ,widget_win_name))&&
                            ((0 == strcasecmp(WND_MAIN_MENU ,focus_Window))||(0 == strcasecmp(WND_MAIN_MENU_ITEM ,focus_Window))
                             ||(0 == strcasecmp(WND_TKGS_UPGRADE_PROCESS ,focus_Window))||(0 == strcasecmp(WND_POP_TIP ,focus_Window))
                             ||(0 == app_channel_info_compare_dialog(focus_Window))||(0 == strcasecmp(WND_COMMON_SELECT ,focus_Window))))
                    {
                        /*
                         * 系统设置、网络多媒体退出时重新播放节目，直接关闭窗体
                         * 避免退出到全屏前，播放上一节目
                         */
                        popdlg_destroy();
						GUI_EndDialog(focus_Window);
                    }
                    else if ((0 == strcasecmp(WIN_PIC_VIEW ,focus_Window))
                            ||(0 == strcasecmp(WIN_MUSIC_VIEW ,focus_Window))
#if APPLETS_SUPPORT
                            ||(0 == strcasecmp(WND_APPLETS ,focus_Window))
#endif
                            )
                    {
                        /*
                         * 对于会引起死锁界面，直接GUI_EndDialog
                         */
                        GUI_EndDialog(focus_Window);
                    }
                    else
                    {
                        keyEvent.type = GUI_KEYDOWN;
                        keyEvent.key.sym = STBK_EXIT;
                        GUI_SendEvent(focus_Window,&keyEvent);
                    }

                }
                GUI_LoopEvent();
                rtn = 1;
                focus_Window = (char*)GUI_GetFocusWindow();

                if (NULL != focus_Window)
                {
                    rtn = strcasecmp(widget_win_name, focus_Window);
                    if (0 == rtn)
                    {
                        //	app_log_debug("%s %s %d focus_Window=%s\n",__FILE__,__func__,__LINE__,focus_Window);
                    }
                }
            }
        }
    }

	//app_log_debug("%s %s %d  end\n",__FILE__,__func__,__LINE__);

	return ;
}

static int _tkgs_real_lowpower(void *userdata)
{
    s_simple_wakeup_timer = NULL;

	APP_TIMER_REMOVE(s_panel_led_timer);
    s_panel_led_timer = NULL;

	unsigned int lock = 0;
	unsigned int standby = 1;
	app_panel_ioctl_handle(PANEL_SHOW_LOCK, &lock);
	app_panel_ioctl_handle(PANEL_SHOW_STANDBY, &standby);

	app_bsp_panel_close();

#if defined(GX6605S) || defined(TAURUS)
	//========================luna add================//
	GxPlayer_SetAudioPowerMute(1);
#endif
	GxPlayer_SetVideoOutputPower(0); //0 close output

    app_entry_lowpower(0);
    return 0;
}

bool app_get_tkgs_power_status(void)
{
    return s_simple_lowpower;
}

void app_tkgs_power_on(void)
{
	timer_stop(s_simple_wakeup_timer);
    APP_TIMER_REMOVE(s_simple_wakeup_timer);
	s_simple_wakeup_timer = NULL;

	timer_stop(s_panel_led_timer);
	APP_TIMER_REMOVE(s_panel_led_timer);
    s_panel_led_timer = NULL;

    GxPlayer_SetVideoOutputPower(1);//1 open output
    app_set_hardware_unmute();

    g_AppFullArb.draw[EVENT_TV_RADIO](&g_AppFullArb);
    g_AppPlayOps.play_list_create(&g_AppPlayOps.normal_play.view_info);
    if (g_AppPlayOps.normal_play.play_total != 0)
    {
		app_play_current_prog(PLAY_TYPE_NORMAL|PLAY_MODE_POINT);
        g_AppFullArb.draw[EVENT_MUTE](&g_AppFullArb);

        if (g_AppPlayOps.normal_play.key == PLAY_KEY_LOCK)
        {
            g_AppFullArb.tip = FULL_STATE_LOCKED;
        }
        else
        {
            g_AppFullArb.tip = FULL_STATE_DUMMY;
        }
    }

    app_panel_show_prog_num(0);
	if (g_AppFullArb.state.mute == STATE_OFF)
    {
        GxMsgProperty_PlayerAudioMute player_mute = 0;
        app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
    }

#if HDMI_CEC_SUPPORT
	app_cec_init();
#endif
}

void app_tkgs_power_off(time_t sleep_time)
{
    GxTkgsLocation s_tkgsLocation;
    s_tkgsLocation.visible_num = 0;
    s_tkgsLocation.invisible_num = 0;

    app_send_msg_exec(GXMSG_TKGS_GET_LOCATION, &s_tkgsLocation);
    app_win_exist_to_win_widget(WND_FULL_SCREEN);
	_panel_show_time(NULL);
    APP_TIMER_ADD(s_panel_led_timer, _panel_show_time, 1000, TIMER_REPEAT);

	if (g_AppFullArb.state.mute == STATE_OFF)
    {
        GxMsgProperty_PlayerAudioMute player_mute = 1;
        app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
    }

    GxPlayer_SetVideoOutputPower(0); //0 close output

    // for mute
    app_set_hardware_mute();

    if((s_tkgsLocation.visible_num>0)
            ||(s_tkgsLocation.invisible_num>0))
    {
        if(app_tkgs_get_upgrade_mode() != TKGS_UPGRADE_MODE_STANDBY && s_tkgs_force_stop == 0)
        {
            app_tkgs_set_upgrade_mode(TKGS_UPGRADE_MODE_STANDBY);
            if(app_tkgs_get_operatemode() == GX_TKGS_OFF)
            {
                _tkgsupgradeprocess_upgrade_version_check(0);
            }
            else
            {
                app_tkgs_upgrade_process_menu_exec();
            }
        }
    }
    else
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.str = STR_ID_TKGS_SET_LOCATION;
        pop.mode = POP_MODE_UNBLOCK;
        pop.format = POP_FORMAT_DLG;
        popdlg_create(&pop);
    }

    if(sleep_time > 0)
    {
        APP_TIMER_ADD(s_simple_wakeup_timer, _tkgs_real_lowpower, sleep_time * 1000, TIMER_ONCE);
    }
}
#endif


void app_low_power(time_t sleep_time, uint32_t scan_code)
{
#if HDMI_CEC_SUPPORT
    if ((sleep_time != 1)||(s_reboot_flag == false))
    {
	    extern int app_cec_enter_standby_mode(void);
		if ((s_simple_lowpower  == false)&&(scan_code != OPCODE_SYSTEM_STANDBY))
		{
		    app_cec_enter_standby_mode();
		}
#if ((SIMPLE_LOWPOWER_SUPPORT == 0)&&(TKGS_SUPPORT == 0))
	    /* Resolve problem [#336144] */
	    GxVoutHAL_HdmiSetCecEnable(0);
#endif
    }
#endif

#ifdef SUPPORT_SCART
    app_gpio_set_scart_onoff(1);
    app_gpio_set_scart_tv(0);
#endif
#if (DVBT2_ANT_POWER > 0)
    app_gpio_ant_power_set(0);
#endif
#if PVR_STANDBY_SUPPORT
    if (sleep_time >= 10)
    {
        extern void app_set_pvr_standby_flag(bool bflag);
        app_set_pvr_standby_flag(true);
    }
#endif

#if SIMPLE_LOWPOWER_SUPPORT
    if(s_reboot_flag == false)
    {
        if(s_simple_lowpower == false)
        {
            s_simple_lowpower = true;
            app_simple_power_off(sleep_time);
        }
        else
        {
            s_simple_lowpower = false;
            app_simple_power_on();
        }
        return;
    }
#endif
#if TKGS_SUPPORT
    if(sleep_time == 0 && scan_code != 0)
    {
        if(s_reboot_flag == false)
        {
            if(s_simple_lowpower == false)
            {
                s_simple_lowpower = true;
                if(app_tkgs_get_operatemode() == GX_TKGS_OFF)
                {
                    app_tkgs_power_off(sleep_time);
                }
                else
                {
                    app_tkgs_power_off(60);
                }
            }
            else
            {
                s_simple_lowpower = false;
                app_tkgs_power_on();
            }
            return;
        }

    }
#endif

#if DVB_CLOUD_SUPPORT
	gx_dvbonline_pause();
#endif
    app_frontend_standby();
	app_send_msg_exec(GXMSG_PM_CLOSE, NULL);//new 20130608


#if NETWORK_SUPPORT && GPRS_SUPPORT
	if(g_AppGprs.mode == 1)
		g_AppGprs.switchtocmd();
#endif

// for panel
    unsigned int lock = 0;
    unsigned int standby = 1;
    app_panel_ioctl_handle(PANEL_SHOW_LOCK, &lock);
    app_panel_ioctl_handle(PANEL_SHOW_STANDBY, &standby);

    app_set_hardware_mute();
    app_bsp_panel_close();
    //========================luna add================//
    GxPlayer_SetAudioPowerMute(1);
    GxPlayer_SetVideoOutputPower(0); //0 close output

#ifdef LINUX_OS
#if NETWORK_SUPPORT
    app_send_msg_exec(GXMSG_NETWORK_STOP, NULL);
#endif
    system("prepare_lowpower");
#endif

    app_entry_lowpower(sleep_time);
    return;
}

bool app_simple_lowpower_status_get(void)
{
    return s_simple_lowpower;
}

void app_update_sync_data(void)
{
    s_reboot_flag = false;
	app_send_msg_exec(GXMSG_PM_CLOSE, NULL);//new 20130608
    app_bsp_panel_close();
	g_AppTime.stop(&g_AppTime);
	g_AppFullArb.timer_stop();

	return;
}

static void app_remount_rootfs(void)
{
#ifdef ECOS_OS
	extern int mount(const char *devname,
			const char *dir,
			const char *fsname,
			unsigned long mountflags,
			const void *data);
	extern int umount(const char *name);

	struct partition* flash_part = NULL;
	struct partition_info* partition = NULL;
	char *fs_type = NULL;
	static char dir_cur[32] = {0};

	flash_part = GxCore_PartitionFlashInit();
	if (flash_part)
	{
		partition = GxCore_PartitionGetByName(flash_part, "ROOT");
		if (partition)
		{
			if (partition->file_system_type < MINIFS)
			{
				fs_type = GxCore_PartitionGetType(partition);
				sprintf(dir_cur, "/dev/flash/0/0x%x,0x%x", partition->start_addr, partition->total_size);

				umount("/");
				mount(dir_cur, "/", fs_type, 0, 0);
				app_log_debug("\n\033[31m mount /, dir_cur=%s, type=%s\033[0m\n", dir_cur, fs_type);
			}
		}
	}
#endif
}

void app_reboot(void)
{
    s_reboot_flag = true;
    app_send_msg_exec(GXMSG_PM_CLOSE, NULL);
    app_remount_rootfs();

    app_set_hardware_mute();
    //========================luna add================//
    GxPlayer_SetAudioPowerMute(1);

    app_low_power(1, 0);

    return;
}
