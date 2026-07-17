/*
 * :i
 * =====================================================================================
 *
 *       Filename:  app_pvr_bar.c
 *
 *    Description:
 *
 *        Version:  1.0
 *        Created:  2011年09月21日 11时16分35秒
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (),
 *        Company:  Hangzhou Nationalchip Science&Technology Co.Ltd.
 *
 * =====================================================================================
 */

#include "app_module.h"
#include "app_send_msg.h"
#include "app.h"
#include "app_pop.h"
#include "app_str_id.h"
#include "full_screen.h"
#include "app_default_params.h"
#include "app_epg.h"
#include "app_windows.h"
#include "app_book.h"
#include <common/gxpm.h>
#include "app_channel_info.h"
#include "app_cover_pvr.h"
#include "app_cover_pvr_capture_poster.h"

typedef enum
{
    PVR_SHOW,
    PVR_HIDE
}pvr_ui;

typedef enum
{
    PVR_PAUSE_ENABLE,
    PVR_PAUSE_DISABLE
}pvr_pause;

typedef enum
{
    PVR_TIMER_TICK,
    PVR_TIMER_SLIDER,
    PVR_TIMER_REC_END,
    PVR_TIMER_TOTAL
}pvr_timer_mode;

typedef struct _pvr_timer pvr_timer;
struct _pvr_timer
{
    event_list      *timer[PVR_TIMER_TOTAL];
    uint32_t        interval[PVR_TIMER_TOTAL];
    TIMER_FLAG      flag[PVR_TIMER_TOTAL];
    int             (*cb[PVR_TIMER_TOTAL])(void*);
    void    (*init)(pvr_timer*);
    void    (*release)(pvr_timer*);
    void    (*start)(pvr_timer*, pvr_timer_mode);
};

typedef struct
{
    pvr_ui          ui;
    uint32_t        slider_refresh; // 0-stop refresh  1-timer refresh
    bool              shift_end;
    uint32_t        tick_refresh_delay;
    void            (*taxis)(uint32_t);
    void            (*state_change)(PlayerStatus);// ff fb pause play dummy
    pvr_timer       timer;
    bool            shift_start;
#define TAXIS_NONE  (0)
#define TAXIS_TP    (1)
}AppPvrUi;

static void _get_pvr_duration_from_event(void);
static int pvr_timer_rec_duration(void *usrdata);
void app_pvr_patition_error_proc(char* str_tip);
extern void zoom_refresh(void);
extern int app_get_channel_list_mode(void);
extern AppPvrUi g_AppPvrUi;
static time_t pvr_duration = 0;
static event_list* sp_PvrBarTimer = NULL;
extern void book_popdlg_exec(PopDlgRet ret);
extern int app_channel_list_get_cur_list_sel(void);
extern int channel_list_get_cur_group_num(void);


static bool s_pvr_usbin_flag = false;
static bool s_pvr_seek_flag = false;

#define TXT_NAME        "txt_pvr_prog_name"
#define TXT_PERCENT     "txt_pvr_percent"
#define IMG_PERCENT     "img_percent_"
#define IMG_STATE       "img_pvr_state"
#define TXT_STATE       "txt_pvr_state"
#define TXT_CUR_TICK    "txt_pvr_cur_time"
#define TXT_SEEKMIN_TICK  "txt_pvr_seekmin_time"
#define TXT_TOTAL_TICK  "txt_pvr_total_time"
#define SLIDER_BAR      "slider_pvr_process"
#define SLIDER_BAR_SEEK      "slider_pvr_seek"
#define EDIT_DURATION      "edit_duration_set"

#define TICK_DELAY (1)

#define SPD_NUM (5)
pvr_speed spd_val[SPD_NUM+1] = {PVR_SPD_1,PVR_SPD_2,PVR_SPD_4,PVR_SPD_8,PVR_SPD_16,PVR_SPD_32};

#if PVR_STANDBY_SUPPORT
extern void app_low_power(time_t sleep_time, uint32_t scan_code);
void app_set_pvr_standby_flag(bool bflag);
static bool s_pvr_standby_flag = false;

#if PVR_STANDBY_TIP_SUPPORT
static time_t s_sleep_time = 0;
static int _pvr_standby_cb(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        app_low_power(s_sleep_time, 0);
    }
    return 0;
}
#endif

int app_pvr_standby(void)
{
	time_t timerDur;
	time_t time_now;
    time_t timerPvr;
    time_t timerPlay;

    time_now = g_AppTime.utc_get(&g_AppTime);
    timerDur = g_AppBook.get_sleep_time(time_now);
    timerPlay =  g_AppBook.get_pvr_time(time_now,BOOK_PROGRAM_PLAY);
    timerPvr = g_AppBook.get_pvr_time(time_now,BOOK_PROGRAM_PVR);

	if ((timerPlay != 0) && (timerDur != 0))
	{
		timerDur = timerDur > timerPlay ? timerPlay : timerDur;
	}
	if ((timerPlay != 0) && (timerDur != 0))
	{
   	 	timerDur = timerDur > timerPvr ? timerPvr : timerDur;
	}

    if(timerDur == timerPvr)
    {
        app_set_pvr_standby_flag(true);
    }
#if PVR_STANDBY_TIP_SUPPORT
    int value = 0;
    GxBus_ConfigGetInt(PVR_END_STANDBY_KEY, &value, PVR_END_STANDBY_VALUE);
    s_sleep_time = timerDur;
    if(1 == value)
    {
        PopDlg pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_YES_NO;
        pop.format = POP_FORMAT_DLG;
        pop.str = STR_ID_PVR_END_STANDBY;
        pop.mode = POP_MODE_UNBLOCK;
        pop.create_cb = NULL;
        pop.exit_cb = _pvr_standby_cb;
        pop.timeout_sec = 15;
        pop.show_time = true;
        popdlg_create(&pop);
    }
    else
    {
        app_low_power(timerDur, 0);
    }
#else
    app_low_power(timerDur, 0);
#endif
    return EVENT_TRANSFER_STOP;
}

void app_set_pvr_standby_flag(bool bflag)
{
    int standby_value = 0;
    if (bflag == true)
    {
        standby_value =1;
    }
    else
    {
        standby_value = 0;
    }
    GxBus_ConfigSetInt(PVR_STANDBY, standby_value);
}

bool app_get_pvr_standby_flag(void)
{
    return s_pvr_standby_flag;
}

void app_get_pvr_standby_set(bool flag)
{
    s_pvr_standby_flag = flag;
}

void app_load_pvr_standby_flag(void)
{
    int standby_value = 0;
    unsigned int sWakeFlag = 0;
    GxCore_GetWakeFlag(&sWakeFlag);
    if(sWakeFlag != WAKE_FROM_AUTO_STANDBY)
    {
        /*power on or wake up by manual  */
        standby_value = 0;
        s_pvr_standby_flag = false ;
        GxBus_ConfigSetInt(PVR_STANDBY, standby_value);
        return;
    }
    GxBus_ConfigGetInt(PVR_STANDBY,&standby_value, PVR_STANDYB_FLAG);

    if (standby_value ==1)
    {
        GxMsgProperty_PlayerAudioMute player_mute;
        player_mute = 1;
        app_send_msg_exec(GXMSG_PLAYER_AUDIO_MUTE, (void *)(&player_mute));
        app_get_pvr_standby_set(true);
    }
    else
    {
        app_get_pvr_standby_set(false);
    }
    standby_value = 0;
    GxBus_ConfigSetInt(PVR_STANDBY, standby_value);

}

#endif


pvr_speed app_get_pvr_speed(unsigned int speed)
{
    return spd_val[speed];
}

void app_set_pvr_usbin_flag(bool bflag)
{
	s_pvr_usbin_flag = bflag;
}

static bool app_check_pvr_usbin_flag(void)
{
	return s_pvr_usbin_flag;
}

void app_pvrseek_flag_set(void)
{
	s_pvr_seek_flag = false;
}

static int _pvr_bar_timeout(void* data)
{
    sp_PvrBarTimer =NULL;
	GUI_EndDialog(WND_PVR_BAR);
	if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
	{
		app_channel_info_create_dialog();
	}
    if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_TIP))
        GUI_SetFocusWidget(WND_POP_TIP);
    else if(GXCORE_SUCCESS == GUI_CheckDialog(WND_POP_BOOK))
        GUI_SetFocusWidget(WND_POP_BOOK);
	return 0;
}

static void sectohour(time_t time, struct tm *tm_time)
{

    if(time < 0)
        time = 0;
    tm_time->tm_hour = (uint32_t)(time/3600);
    tm_time->tm_min = (uint32_t)((time - tm_time->tm_hour*3600)/60);
    tm_time->tm_sec = (uint32_t)(time%60);
    return;
}

static void pvr_taxis_mode(uint32_t mode)
{
    GxBusPmViewInfo view_info;
    GxMsgProperty_NodeByPosGet node;

    // prog data get, tp id get
    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node));

    // view info get
    app_send_msg_exec(GXMSG_PM_VIEWINFO_GET, &view_info);

    // view info set
#if 0
    if (mode == TAXIS_NONE)
    {
        view_info.tp_id = 0;
        view_info.taxis_mode = TAXIS_MODE_NON;
    }
    else
    {
        view_info.tp_id = node.prog_data.tp_id;
        view_info.taxis_mode = TAXIS_MODE_TP;
    }
#endif
    if (mode == TAXIS_NONE)
    {
        view_info.pip_switch = 0;
        view_info.pip_main_tp_id = 0;
    }
    else
    {
#if INDEPEND_RECORD
        view_info.pip_switch = 0;
        view_info.pip_main_tp_id = 0;
#else
        uint16_t tp_id = 0;
#if NDEMOD_WITH_1TUNER
        tp_id = node.prog_data.tp_id|0x8000;
#else
        tp_id = node.prog_data.tp_id;
#endif
        view_info.pip_switch = 1;
        view_info.pip_main_tp_id = tp_id;
#endif
    }

    g_AppPlayOps.play_list_create(&view_info);
    if(g_AppPlayOps.current_play.view_info.pip_switch != g_AppPlayOps.normal_play.view_info.pip_switch)
    {
        g_AppPlayOps.current_play.view_info.pip_switch = g_AppPlayOps.normal_play.view_info.pip_switch;
        g_AppPlayOps.current_play.view_info.pip_main_tp_id = g_AppPlayOps.normal_play.view_info.pip_main_tp_id;
#if RECALL_LIST_SUPPORT
    int i=0;
        for(i=0; i< MAX_RECALL_NUM; i++)
        {
            g_AppPlayOps.recall_play[i].view_info.pip_switch = g_AppPlayOps.normal_play.view_info.pip_switch;
            g_AppPlayOps.recall_play[i].view_info.pip_main_tp_id = g_AppPlayOps.normal_play.view_info.pip_main_tp_id;
        }
#else
        g_AppPlayOps.recall_play.view_info.pip_switch = g_AppPlayOps.normal_play.view_info.pip_switch;
        g_AppPlayOps.recall_play.view_info.pip_main_tp_id = g_AppPlayOps.normal_play.view_info.pip_main_tp_id;

#endif
    }
}

static int pvr_cur_total_tick_get(void)
{
    GxMsgProperty_GetPlayTime time = {0};
    AppPvrOps *pvr = &g_AppPvrOps;
    FullArb *arb = &g_AppFullArb;

    if (pvr->state == PVR_DUMMY)
    {
        return -1;
    }
    else if((pvr->state == PVR_RECORD) || (pvr->state == PVR_TP_RECORD))
    {
        time.player_name = (int8_t*)PLAYER_FOR_REC;
    }
    else
    {
        // pvr || pvr&tms
        time.player_name = (int8_t*)PLAYER_FOR_TIMESHIFT;
    }

    if(GXCORE_SUCCESS
            == app_send_msg_exec(GXMSG_GET_MEDIA_TIME,(void *)&time))
    {
		pvr->time.seekmin_tick = (uint32_t)(time.seekmin_time/SEC_TO_MILLISEC);
		pvr->time.total_tick = (uint32_t)(time.total_time/SEC_TO_MILLISEC);
		if(arb->state.pause == STATE_OFF)
        {
            pvr->time.cur_tick = (uint32_t)(time.cur_time/SEC_TO_MILLISEC);
        }
    }
    else
    {
    	return -1;
    }

    if((pvr->time.total_tick > 0) && (pvr->time.total_tick > pvr->time.cur_tick))
    {
        pvr->time.remaining_tick = pvr->time.total_tick - pvr->time.cur_tick;
    }
    else
    {
        pvr->time.remaining_tick = 0;
    }
    return 0;
}

static void pvr_time_show(void)
{
    AppPvrOps *pvr = &g_AppPvrOps;
    AppPvrUi *pvr_ui = &g_AppPvrUi;
    char seekmin_tick[16] = {0};
    char cur_tick[16]= {0};
    char total_tick[16]= {0};
   struct tm tm = {0};
	uint32_t hour;

    if(pvr->state == PVR_DUMMY)
    {
        return;
    }

    if((pvr->state == PVR_RECORD) || (pvr->state == PVR_TP_RECORD))
    {
        sectohour((time_t)(pvr->time.total_tick), &tm);
    }
    else
    {
        sectohour((time_t)(pvr->time.seekmin_tick), &tm);
    }
    hour = 24*tm.tm_mday+tm.tm_hour;
    sprintf(seekmin_tick, "%02d:%02d:%02d", hour , tm.tm_min, tm.tm_sec);
    GUI_SetProperty(TXT_SEEKMIN_TICK, "string", seekmin_tick);

    if((pvr->state == PVR_RECORD) || (pvr->state == PVR_TP_RECORD))
    {
        GUI_SetProperty(TXT_CUR_TICK, "string", " ");
    }
    else
    {
        if(pvr->time.cur_tick >= pvr->time.seekmin_tick)
        {
            sectohour((time_t)(pvr->time.cur_tick), &tm);
        }
        else
        {
            sectohour((time_t)(pvr->time.seekmin_tick), &tm);
        }
        hour = 24*tm.tm_mday+tm.tm_hour;
        sprintf(cur_tick, "%02d:%02d:%02d", hour, tm.tm_min, tm.tm_sec);
        GUI_SetProperty(TXT_CUR_TICK, "string", cur_tick);
    }

    if((pvr->state == PVR_RECORD) || (pvr->state == PVR_TP_RECORD))
    {

        if(pvr->time.rec_duration >= pvr->time.total_tick && pvr_ui->timer.timer[PVR_TIMER_REC_END] != NULL)
        {
            sectohour((time_t)(pvr->time.rec_duration), &tm);
        }
		else
		{
			 sectohour((time_t)(pvr->time.total_tick), &tm);
		}
	}
    else
    {
        sectohour((time_t)(pvr->time.total_tick), &tm);
    }
	hour = 24*tm.tm_mday+tm.tm_hour;
	sprintf(total_tick, "%02d:%02d:%02d", hour, tm.tm_min, tm.tm_sec);
    GUI_SetProperty(TXT_TOTAL_TICK, "string", total_tick);
}

static void pvr_slider_show(char *slider)
{
	uint32_t bar_value = 0;
	AppPvrOps *pvr = &g_AppPvrOps;
    AppPvrUi *pvr_ui = &g_AppPvrUi;

	if ((pvr->time.cur_tick == 0) && (pvr->spd > 0))
		return;

	if (pvr->state == PVR_RECORD)
	{
        if (pvr_ui->timer.timer[PVR_TIMER_REC_END] != NULL)
        {
			if(pvr->time.rec_duration != 0)
				bar_value = (pvr->time.total_tick*100)/(pvr->time.rec_duration);
        }
        else
        {
            bar_value = 100;
        }
    }
	else if (pvr->state == PVR_TIMESHIFT
		|| pvr->state == PVR_TMS_AND_REC)
	{
			if(pvr->time.total_tick != 0)
			{
                bar_value = pvr->time.cur_tick > pvr->time.seekmin_tick ? (pvr->time.cur_tick - pvr->time.seekmin_tick):0;
				bar_value = (bar_value*100) / (pvr->time.total_tick - pvr->time.seekmin_tick);
			}
   }
	GUI_SetProperty(slider, "value", &bar_value);
}

static void pvr_percent_show(void)
{
#define PER_STR_LEN     8
#define PER_IMG_LEN     16
    uint32_t i;
    int32_t percent;
    char *str = NULL;
    char per_str[PER_STR_LEN] = {0};
    char per_img[PER_IMG_LEN];

    AppPvrOps *pvr = &g_AppPvrOps;
    percent = pvr->percent(pvr);
    if((percent < 0)||(percent>100))
        return;

	if(percent == 0)
	{
		percent = 1;
	}
    sprintf(per_str, "%d%%", percent);

    GUI_GetProperty(TXT_PERCENT, "string", &str);
    if(NULL == str || strcmp(str, per_str))
        GUI_SetProperty(TXT_PERCENT, "string", per_str);
    else
        return;

    // 5 dot to show the percent
    for (i=0; i<4; i++)
    {
        memset(per_img, 0, PER_IMG_LEN);
        sprintf(per_img, "%s%d", IMG_PERCENT, i);

        if (i*25 < percent)
        {
            if (i < 3)
            {
                GUI_SetProperty(per_img, "img", "s_pvr_progress_blue");
            }
            else if (i == 3)
            {
                GUI_SetProperty(per_img, "img", "s_pvr_progress_yellow");
            }
            else
            {
                GUI_SetProperty(per_img, "img", "s_pvr_progress_red");
            }
        }
        else
        {
            GUI_SetProperty(per_img, "img", "s_pvr_progress_grey");
        }
    }
}

static int pvr_timer_tick(void *usrdata)
{
    static int32_t percent_show = 0;
    AppPvrUi *pvr_ui = &g_AppPvrUi;

     // get tick
    if(pvr_cur_total_tick_get()<0)
  	 return 0;

    if (GUI_CheckDialog(WND_PVR_BAR) != GXCORE_SUCCESS)
    {
        return 0;
    }

    if(pvr_ui->tick_refresh_delay == 0)
    {
        pvr_time_show();
    }

    if(pvr_ui->tick_refresh_delay > 0)
    {
        pvr_ui->tick_refresh_delay--;
    }

    if(pvr_ui->slider_refresh <= 1)
    {
        pvr_slider_show(SLIDER_BAR);
        if(pvr_ui->slider_refresh == 1)
        {
            pvr_slider_show(SLIDER_BAR_SEEK);
        }
    }

    if(pvr_ui->slider_refresh>1)
    {
        pvr_ui->slider_refresh--;
    }

    // 10s , refresh usb percent
    if (percent_show++ == 10)
    {
        percent_show = 0;
        pvr_percent_show();
    }

    return 0;
}

static int pvr_timer_slider(void *usrdata)
{
	g_AppPvrUi.timer.timer[PVR_TIMER_SLIDER] = NULL;
	return 0;
}

static void pvr_timer_init(pvr_timer *timer)
{
    timer->cb[PVR_TIMER_TICK]         = pvr_timer_tick;
    timer->interval[PVR_TIMER_TICK]   = 1*SEC_TO_MILLISEC;
    timer->flag[PVR_TIMER_TICK]       = TIMER_REPEAT;

    timer->cb[PVR_TIMER_SLIDER]       = pvr_timer_slider;
    timer->interval[PVR_TIMER_SLIDER] = 1*SEC_TO_MILLISEC;
    timer->flag[PVR_TIMER_SLIDER]     = TIMER_ONCE;

}

static void pvr_timer_release(pvr_timer *timer)
{
    uint32_t i;

    for (i=0; i<PVR_TIMER_SLIDER; i++)
    {
        if (timer->timer[i] != NULL)
        {
            remove_timer(timer->timer[i]);
            timer->timer[i] = NULL;
        }
    }
}

static void pvr_timer_start(pvr_timer *timer, pvr_timer_mode mode)
{
    if(0 != reset_timer(timer->timer[mode]))
    {
        timer->timer[mode] = create_timer(timer->cb[mode],
        timer->interval[mode], NULL, timer->flag[mode]);
    }
}

static void pvr_name_show(void)
{
	char *prog_name = NULL;
	if(g_AppPvrOps.env.prog_id == 0)
	{
		GxMsgProperty_NodeByPosGet node_prog = {0};
		// prog
		node_prog.node_type = NODE_PROG;
		node_prog.pos = g_AppPlayOps.normal_play.play_count;
		app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
		prog_name = (char*)node_prog.prog_data.prog_name;
	}
	else
	{
		GxMsgProperty_NodeByIdGet node_prog = {0};
		// prog
		node_prog.node_type = NODE_PROG;
		node_prog.id= g_AppPvrOps.env.prog_id;
		app_send_msg_exec(GXMSG_PM_NODE_BY_ID_GET, &node_prog);
		prog_name = (char*)node_prog.prog_data.prog_name;
	}

	GUI_SetProperty(TXT_NAME, "string", prog_name);
}

// bk-0. use bakc   1. use force
static void pvr_slider_bk(uint32_t bk)
{
    char *bk_img[2][3] = {{"s_pvr_progress_l", "s_pvr_progress_m", "s_pvr_progress_r"},
                    {"s_pvr_progress_l2", "s_pvr_progress_m2", "s_pvr_progress_r2"}};

    GUI_SetProperty(SLIDER_BAR, "back_image_l", bk_img[bk][0]);
    GUI_SetProperty(SLIDER_BAR, "back_image_m", bk_img[bk][1]);
    GUI_SetProperty(SLIDER_BAR, "back_image_r", bk_img[bk][2]);
}

SIGNAL_HANDLER int app_pvr_bar_create(GuiWidget *widget, void *usrdata)
{
    AppPvrUi *pvr_ui = &g_AppPvrUi;
    AppPvrOps *pvr = &g_AppPvrOps;
    FullArb *arb = &g_AppFullArb;
    char *spd_str[] = {"X1","X2","X4","X8","X16","X32"};

    if(app_channel_info_check_dialog() == GXCORE_SUCCESS)
    {
        app_channel_info_end_dialog();
    }
	if(app_check_pvr_usbin_flag() == true)
	{
		app_set_pvr_usbin_flag(false);
#define PER_IMG_LEN     16
		uint32_t i;
		char per_img[PER_IMG_LEN];
		GUI_SetProperty(TXT_PERCENT, "string", "0%");

		for (i=0; i<4; i++)
		{
			memset(per_img, 0, PER_IMG_LEN);
			sprintf(per_img, "%s%d", IMG_PERCENT, i);
			GUI_SetProperty(per_img, "img", "s_pvr_progress_grey");
		}

	}
	pvr_name_show();
	pvr_percent_show();
	//tv/radio
    if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
    {
        GUI_SetProperty(IMG_STATE, "img", "s_bar_radio");
    }

    if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_TV)
    {
        GUI_SetProperty(IMG_STATE, "img", "s_bar_tv");
    }


    if((pvr->state == PVR_RECORD) || (pvr->state == PVR_DUMMY) || (pvr->state == PVR_TP_RECORD))
    {
		pvr_slider_bk(1);
        GUI_SetProperty(SLIDER_BAR_SEEK, "state", "hide");
        GUI_SetFocusWidget(SLIDER_BAR);
    }
    else
    {
		pvr_slider_bk(1);
        if (pvr->spd != 0)
        {
            if (arb->state.pause == STATE_ON)
            {
                GUI_SetProperty(SLIDER_BAR_SEEK, "state", "show");
                GUI_SetFocusWidget(SLIDER_BAR_SEEK);
            }
            else
            {
                GUI_SetFocusWidget(SLIDER_BAR);
            }
        }
        else
        {
            GUI_SetProperty(SLIDER_BAR_SEEK, "state", "show");
            GUI_SetFocusWidget(SLIDER_BAR_SEEK);
        }
    }

    pvr_cur_total_tick_get();
    pvr_time_show();
    pvr_slider_show(SLIDER_BAR);
    pvr_slider_show(SLIDER_BAR_SEEK);

    pvr_ui->timer.init(&(pvr_ui->timer));
    pvr_ui->timer.start(&(pvr_ui->timer), PVR_TIMER_TICK);
    pvr_ui->ui = PVR_SHOW;

    if(arb->state.pause == STATE_OFF)
    {
        if(pvr->spd > 0)
        {
            GUI_SetProperty(TXT_STATE, "string", spd_str[pvr->spd]);
        }
        else if(pvr->spd < 0)
        {
            GUI_SetProperty(TXT_STATE, "string", spd_str[pvr->spd*(-1)]);
        }
        else
        {
            GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
        }
    }
    else
    {
        GUI_SetProperty(TXT_STATE, "string", STR_ID_PAUSE);
    }

#if MENCENT_FREEE_SPACE
		GUI_SetInterface("flush", NULL);// ??????????,????Player???????????
#endif

	if ( pvr->spd == 0 && reset_timer(sp_PvrBarTimer) != 0 )
	{
        sp_PvrBarTimer = create_timer(_pvr_bar_timeout, PVRBAR_TIMEOUT*SEC_TO_MILLISEC, NULL, TIMER_ONCE);
	}

    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_bar_destroy(GuiWidget *widget, void *usrdata)
{
    AppPvrUi *pvr_ui = &g_AppPvrUi;

    if(GUI_CheckDialog(WND_DURATION_SET) == GXCORE_SUCCESS)
        GUI_EndDialog(WND_DURATION_SET);

    pvr_ui->timer.release(&(pvr_ui->timer));
    pvr_ui->ui = PVR_HIDE;
    pvr_ui->slider_refresh = 1;
    pvr_ui->tick_refresh_delay = 0;
	APP_TIMER_REMOVE(sp_PvrBarTimer);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_bar_got_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_KEEPON;
}

SIGNAL_HANDLER int app_pvr_bar_lost_focus(GuiWidget *widget, void *usrdata)
{
    return EVENT_TRANSFER_KEEPON;
}

void app_pvr_stop_refresh_ui(void)
{
    AppPvrOps *pvr = &g_AppPvrOps;
    FullArb *arb = &g_AppFullArb;

    if (pvr->state == PVR_TMS_AND_REC)
    {
        arb->state.rec = STATE_OFF;
        arb->state.tms = STATE_OFF;
        arb->draw[EVENT_STATE](arb);
    }
    return;
}

void app_pvr_rec_stop(void)
{
    AppPvrOps *pvr = &g_AppPvrOps;
    FullArb *arb = &g_AppFullArb;
    AppPvrUi *pvr_ui = &g_AppPvrUi;

    if((pvr->state != PVR_RECORD)
        && (pvr->state != PVR_TMS_AND_REC)
        && (pvr->state != PVR_TP_RECORD))
        return;
    arb->state.rec = STATE_OFF;
	arb->draw[EVENT_STATE](arb);
    pvr->rec_stop(pvr);
    pvr_ui->taxis(TAXIS_NONE);
}

void app_pvr_tms_stop(void)
{
    AppPvrOps *pvr = &g_AppPvrOps;
    FullArb *arb = &g_AppFullArb;
    AppPvrUi *pvr_ui = &g_AppPvrUi;
    pvr_timer_mode mode     = PVR_TIMER_REC_END;

    if((pvr->state != PVR_TIMESHIFT)
        && (pvr->state != PVR_TMS_AND_REC))
        return;

    //Rebuild Timer PVR_TIMER_REC_END
    if((pvr_cur_total_tick_get() >= 0)
	&&(pvr->state == PVR_TMS_AND_REC)
        &&(pvr->time.rec_duration > pvr->time.total_tick))
    {
        pvr_ui->timer.cb[mode]         = pvr_timer_rec_duration;
        pvr_ui->timer.interval[mode]   = (pvr->time.rec_duration - pvr->time.total_tick)*SEC_TO_MILLISEC;
        pvr_ui->timer.flag[mode]       = TIMER_ONCE;

        if(0 != reset_timer(pvr_ui->timer.timer[mode]))
        {
            pvr_ui->timer.timer[mode] = create_timer(pvr_ui->timer.cb[mode],
                    pvr_ui->timer.interval[mode], NULL, pvr_ui->timer.flag[mode]);
        }
    }
    //end

    pvr->spd = 0;
	arb->state.pause = STATE_OFF;
	arb->state.tms = STATE_OFF;//-#36605
	arb->draw[EVENT_STATE](arb);//-#36605
	arb->draw[EVENT_PAUSE](arb);
    arb->draw[EVENT_SPEED](arb);
    pvr->tms_stop(pvr);
}

static int _close_stop_pvr_pop(void)
{
    char *buffer_tip = NULL;
    if(GUI_CheckDialog(WND_POP_TIP) == GXCORE_SUCCESS)
    {
        GUI_GetProperty("txt_pop_tip", "string", &buffer_tip);

        if((buffer_tip != NULL)&&((strcmp(buffer_tip,STR_ID_STOP_REC_INFO) == 0)||
            (strcmp(buffer_tip,STR_ID_STOP_TMS_INFO) == 0)||
            (strcmp(buffer_tip,STR_ID_STOP_TMS_AND_REC_INFO) == 0)))
        {
            popdlg_destroy();
        }
    }
    return 0;
}

static int pvr_timer_rec_duration(void *usrdata)
{
    g_AppPvrUi.timer.timer[PVR_TIMER_REC_END] = NULL;
    uint32_t sel;
    uint32_t cmb_sel;
    uint32_t active_sel;
#if PVR_STANDBY_SUPPORT
    bool funcflag = false;
    if (app_get_pvr_standby_flag() == true)
    {
        funcflag = true;
    }
#endif
    app_pvr_stop();
    _close_stop_pvr_pop();
    GUI_EndDialog(WND_PVR_BAR);
    if (GUI_CheckDialog(WND_CHANNEL_LIST) == GXCORE_SUCCESS)
    {
        sel = app_channel_list_get_cur_list_sel();
        cmb_sel = channel_list_get_cur_group_num();
        GUI_SetProperty("cmb_channel_list_group", "select", &cmb_sel);
		GUI_SetProperty("listview_channel_list", "update_all", NULL);
		GUI_SetProperty("listview_channel_list", "select", &sel);
		active_sel = sel;
		GUI_SetProperty("listview_channel_list", "active", &active_sel);
        app_update_pop_dlg(WND_PASSWD);
        app_update_pop_dlg(WND_POP_TIP);
    }
    else if (app_channel_info_check_dialog() == GXCORE_SUCCESS)
    {
        app_update_pop_dlg(WND_PASSWD);
        app_update_pop_dlg(WND_POP_TIP);
    }
    else
    {
        if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
        {
            if(app_channel_info_check_dialog() != GXCORE_SUCCESS)
                app_channel_info_create_dialog();
        }
    }
#if PVR_STANDBY_SUPPORT
    if (funcflag)
    {
       app_pvr_standby();
    }
#endif
    return 0;
}

void app_pvr_stop(void)
{
	if(app_channel_info_check_dialog() == GXCORE_SUCCESS && (GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS))
	{
		static GUI_Event event;
		event.type = GUI_KEYDOWN;
		event.key.sym = STBK_F1;
		GUI_SendEvent(WND_KEYBOARD_LANGUAGE, &event);
        app_update_pop_dlg(WND_POP_OPTION_LIST);
	}
#if PVR_STANDBY_SUPPORT
    app_get_pvr_standby_set(false);
#endif
    AppPvrOps *pvr = &g_AppPvrOps;
    AppPvrUi *pvr_ui = &g_AppPvrUi;

     if(pvr->state == PVR_DUMMY)
        return;

     app_pvr_tms_stop();
     app_pvr_rec_stop();
     app_pvr_stop_refresh_ui();
    _close_stop_pvr_pop();
    if (pvr_ui->timer.timer[PVR_TIMER_REC_END] != NULL)
    {
        remove_timer(pvr_ui->timer.timer[PVR_TIMER_REC_END]);
        pvr_ui->timer.timer[PVR_TIMER_REC_END] = NULL;
    }

    g_AppFullArb.exec[EVENT_STATE](&g_AppFullArb);
    g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
	if(GUI_CheckDialog(WND_ZOOM) == GXCORE_SUCCESS)
	{
		zoom_refresh();
	}
}

// dura_sec : 0-use system setting time
int app_pvr_rec_start(uint32_t dura_sec)
{
#define HALF_HOUR_SEC       (60*30)
    AppPvrOps   *pvr        = &g_AppPvrOps;
    FullArb     *arb        = &g_AppFullArb;
    AppPvrUi    *pvr_ui     = &g_AppPvrUi;
    pvr_timer_mode mode     = PVR_TIMER_REC_END;
    pvr_state old_state     =  PVR_DUMMY;

    if (pvr->state == PVR_RECORD
            || pvr->state == PVR_TMS_AND_REC)
    {
        return 0;
    }
    old_state = pvr->state;

    _get_pvr_duration_from_event();

//    if (pvr->state == PVR_DUMMY)
//    {
//		pvr_slider_bk(0);
//    }


    pvr_ui->taxis(TAXIS_TP);

    if (dura_sec == 0)
    {
        pvr->time.rec_duration = pvr_duration;
    }
    else
    {
        pvr->time.rec_duration = dura_sec;
    }

    pvr->rec_start(pvr);//Pvr state change

    if(pvr->state == old_state)
    {
        return -1;
    }
    arb->state.rec = STATE_ON;
    arb->draw[EVENT_STATE](arb);
    pvr_percent_show();

    if(pvr->state == PVR_RECORD) //TMS->TMS_REC Don't need timer_rec_end; Just Record
    {
        // set timer
        pvr_ui->timer.cb[PVR_TIMER_REC_END]         = pvr_timer_rec_duration;
        //+3 sec, timer is not accuate, record more data is better than less, maybe...
        pvr_ui->timer.interval[PVR_TIMER_REC_END]   = pvr->time.rec_duration*SEC_TO_MILLISEC + 3;
        pvr_ui->timer.flag[PVR_TIMER_REC_END]       = TIMER_ONCE;

        if(0 != reset_timer(pvr_ui->timer.timer[mode]))
        {
            pvr_ui->timer.timer[mode] = create_timer(pvr_ui->timer.cb[mode],
                    pvr_ui->timer.interval[mode], NULL, pvr_ui->timer.flag[mode]);
        }
    }
    else if(pvr->state ==PVR_DUMMY)
    {
        pvr_ui->taxis(TAXIS_NONE);
    }
    return 0;
}

void app_tp_rec_start(uint32_t dura_sec)
{
#define HALF_HOUR_SEC       (60*30)
    AppPvrOps   *pvr        = &g_AppPvrOps;
    FullArb     *arb        = &g_AppFullArb;
    AppPvrUi    *pvr_ui     = &g_AppPvrUi;
//    pvr_timer_mode mode     = PVR_TIMER_REC_END;

    if (pvr->state != PVR_DUMMY)
    {
        return;
    }


    pvr_ui->taxis(TAXIS_TP);

#if 0
    if (dura_sec == 0)
    {
        pvr->time.rec_duration = pvr_duration;
    }
    else
    {
        pvr->time.rec_duration = dura_sec;
    }
#endif

    pvr->tp_rec_start(pvr);//Pvr state change
    arb->state.rec = STATE_ON;
    arb->draw[EVENT_STATE](arb);
    pvr_percent_show();

#if 0
    if(pvr->state == PVR_RECORD) //TMS->TMS_REC Don't need timer_rec_end; Just Record
    {
        // set timer
        pvr_ui->timer.cb[PVR_TIMER_REC_END]         = pvr_timer_rec_duration;
        //+3 sec, timer is not accuate, record more data is better than less, maybe...
        pvr_ui->timer.interval[PVR_TIMER_REC_END]   = pvr->time.rec_duration*SEC_TO_MILLISEC + 3;
        pvr_ui->timer.flag[PVR_TIMER_REC_END]       = TIMER_ONCE;

        if(0 != reset_timer(pvr_ui->timer.timer[mode]))
        {
            pvr_ui->timer.timer[mode] = create_timer(pvr_ui->timer.cb[mode],
                    pvr_ui->timer.interval[mode], NULL, pvr_ui->timer.flag[mode]);
        }
    }
#endif
}

void app_tplist_rec_start(void)
{
    AppPvrOps   *pvr        = &g_AppPvrOps;
    FullArb     *arb        = &g_AppFullArb;
    AppPvrUi    *pvr_ui     = &g_AppPvrUi;

    if (pvr->state != PVR_DUMMY)
        return;

    pvr_ui->taxis(TAXIS_TP);

    pvr->tplist_rec_start(pvr);//Pvr state change
    arb->state.rec = STATE_ON;

    arb->draw[EVENT_STATE](arb);
}

#if PVR_RECORDING_POSTER_STYLE

#define COVER_PVR_CAPTURE_WAIT_SEC (3)
static PvrRecordInfo s_pvr_file_info;

static int _pvr_file_info_free(void)
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

#if PVR_PREVIEW_SUPPORT
int app_pvr_file_info_update_preview_time(time_t time)
{
    PvrRecordInfo *pvr_info = &s_pvr_file_info;
    PreviewInfo info = {0};

    info.flag = PREVIEW_FROM_TIME;
    if (time - 1 > pvr_info->start_sec)
    {
        info.offset = time - pvr_info->start_sec - 1; //-1s for the first frame
    }
    else
    {
        time = 0;
    
    }
    return app_cover_pvr_preview_info_record(&pvr_info->record, &info);
}

int app_pvr_file_info_update_preview_path(char *path)
{
    PvrRecordInfo *pvr_info = &s_pvr_file_info;
    PreviewInfo info = {0};

    info.flag = PREVIEW_FROM_PATH;
    info.path = path;
    return app_cover_pvr_preview_info_record(&pvr_info->record, &info);
}
#endif

int app_pvr_file_info_update(const char *path, const char *name)
{
    PvrRecordInfo *info = &s_pvr_file_info;
    GxMsgProperty_NodeByPosGet node = {0};
    GxTime time = {0};
    // epg info
    GxEpgInfo *cur_epg_info = NULL;
    EpgProgInfo epg_prog_info;
    AppEpgOps ch_epg_ops;


    _pvr_file_info_free();

    if (!path || !name)
        return -1;

    node.node_type = NODE_PROG;
    node.pos = g_AppPlayOps.normal_play.play_count;
    if(app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void *)(&node)) != GXCORE_SUCCESS)
        return -1;

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

    info->record.chname = GxCore_Strdup((char *)node.prog_data.prog_name);
    info->record.from_freescan = 1;

	epg_prog_info.ts_id= node.prog_data.ts_id;
	epg_prog_info.orig_network_id= node.prog_data.original_id;
	epg_prog_info.service_id= node.prog_data.service_id;
	epg_prog_info.prog_id= node.prog_data.id;
	epg_prog_info.prog_pos = node.pos;
    epg_prog_info.tp_id = node.prog_data.tp_id;
    cur_epg_info = (GxEpgInfo *)GxCore_Malloc(EVENT_GET_NUM*EVENT_GET_MAX_SIZE);

	if(cur_epg_info == NULL)
	{
		app_log_error("_get_pvr_duration_from_event, malloc failed...%s,%d\n",__FILE__,__LINE__);
        goto end;
	}

    epg_ops_init(&ch_epg_ops);
    ch_epg_ops.prog_info_update(&ch_epg_ops, &epg_prog_info);
    if(ch_epg_ops.get_cur_event_cnt(&ch_epg_ops) > 0
        && ch_epg_ops.get_cur_event_info(&ch_epg_ops, cur_epg_info) == GXCORE_SUCCESS)
    {
        info->record.starttime = cur_epg_info->start_time;
        info->record.finishtime = cur_epg_info->start_time + cur_epg_info->duration;
        if (cur_epg_info->event_name_len > 0)
        {
            info->record.name = GxCore_Strdup((char *)cur_epg_info->event_name);
        }
        else
        {
            info->record.name = GxCore_Strdup(STR_ID_NO_INFO);
        }
        if (cur_epg_info->event_detail_len > 0)
        {
            info->record.description = GxCore_Strdup((char *)cur_epg_info->event_detail);
        }
        else
        {
            info->record.description = GxCore_Strdup(STR_ID_NO_INFO);
        }

    }
    else {
        info->record.description = GxCore_Strdup(STR_ID_NO_EPG_EVENT);
    }

    app_capture_vpp_start(info->logo_path, COVER_PVR_CAPTURE_WAIT_SEC);
    APP_FREE(cur_epg_info);
    info->save_flag = PVR_RECORD_SAVE_CH;
    return 0;
end:
    APP_FREE(cur_epg_info);
    _pvr_file_info_free();
    return -1;
}

int app_pvr_file_info_save(void)
{
    PvrRecordInfo *info = &s_pvr_file_info;
    unsigned int duration = app_tick_time_sec_get() - info->start_sec;

    if (!info->save_flag)
        goto end;

    app_capture_vpp_stop();
    //
    // index.json
    info->record.duration = duration;
    if (app_cover_pvr_save_epg_record_info(info->info_path, info->index_path, &info->record) < 0)
    {
        if (GxCore_FileExists(info->info_path) == GXCORE_FILE_EXIST)
            GxCore_FileDelete(info->info_path);
    }

    _pvr_file_info_free();
    return 0;
end:
    _pvr_file_info_free();
    return -1;
}
#endif

void pvr_taxis_mode_clear(void)
{
    g_AppPvrUi.taxis(TAXIS_NONE);
}

void tms_speed_change(float spd_speed)
{

    AppPvrOps *pvr = &g_AppPvrOps;
    FullArb *arb = &g_AppFullArb;
	float new_spd;
    if (pvr->state != PVR_TIMESHIFT
        && pvr->state != PVR_TMS_AND_REC)
    {
        return;
    }

    AppPvrUi  *pvr_ui = &g_AppPvrUi;

    if(spd_speed == PVR_SPD_1)
    {
        GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
        GUI_SetProperty(SLIDER_BAR_SEEK, "state", "show");
        GUI_SetFocusWidget(SLIDER_BAR_SEEK);
        pvr->spd = 0;
        if (GUI_CheckDialog(WND_PVR_BAR) == GXCORE_SUCCESS)
        {
            if (reset_timer(sp_PvrBarTimer) != 0)
            {
                sp_PvrBarTimer = create_timer(_pvr_bar_timeout, PVRBAR_TIMEOUT*SEC_TO_MILLISEC, NULL, TIMER_ONCE);
            }
        }
    }
    else
    {
        timer_stop(sp_PvrBarTimer);
		new_spd = 1;
        if(spd_speed < 0)
        {
            spd_speed *= -1;
			new_spd *= -1;
        }
		switch((int)spd_speed)//the value of pvr->spd depends on spd_val[]
		{
			case PVR_SPD_2:
				GUI_SetProperty(TXT_STATE, "string", "X2");
                pvr->spd = new_spd;
				break;

			case PVR_SPD_4:
				GUI_SetProperty(TXT_STATE, "string", "X4");
				new_spd *= 2;
				pvr->spd = new_spd;
				break;

			case PVR_SPD_8:
				GUI_SetProperty(TXT_STATE, "string", "X8");
				new_spd *= 3;
				pvr->spd = new_spd;
				break;

			case PVR_SPD_16:
				GUI_SetProperty(TXT_STATE, "string", "X16");
				new_spd *= 4;
				pvr->spd = new_spd;
				break;

			case PVR_SPD_32:
				GUI_SetProperty(TXT_STATE, "string", "X32");
				new_spd *= 5;
				pvr->spd = new_spd;
				break;

			default:
				GUI_SetProperty(TXT_STATE, "string", " ");
				break;
		}
		pvr_ui->slider_refresh = 1;
		pvr_ui->tick_refresh_delay = 0;
		GUI_SetProperty(SLIDER_BAR_SEEK, "state", "hide");
        GUI_SetInterface("flush",NULL);
		GUI_SetFocusWidget(SLIDER_BAR);
	}
    arb->draw[EVENT_SPEED](arb);
	if(GUI_CheckDialog(WND_ZOOM) == GXCORE_SUCCESS)
	{
		zoom_refresh();
	}

}

void record_state_change(PlayerStatus state)
{
    AppPvrOps *pvr = &g_AppPvrOps;
    if(pvr->state == PVR_DUMMY)
        return;

    switch(state)
    {
        case PLAYER_STATUS_RECORD_END:
            {
                /*book有可能是播放，既然录制已经结束，则播放是可以继续的，所以应该book弹框的优先级别比
                  pvr提示结束高，要不book事件就不执行。另外和时移tms_state_change保持一致。 20230725 kk */
                //book_popdlg_exec(POP_VAL_CANCEL);
                app_pvr_patition_error_proc(STR_ID_PVR_END);
            }
            break;
        case PLAYER_STATUS_RECORD_FULL:
            {
                book_popdlg_exec(POP_VAL_CANCEL);
                app_pvr_patition_error_proc(STR_ID_NO_SPACE);
            }
            break;
        case PLAYER_STATUS_ERROR:
            {
                app_pvr_patition_error_proc(STR_ID_CANT_PVR);
            }
            break;
        case PLAYER_STATUS_RECORD_RUNNING:
            {
#if CA_SUPPORT
                // TODO: 20160302
#if ECM_SUPPORT
#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT
                if(app_tsd_check())
                {
                    break;
                }
                if(0 == g_AppEcm2.ecm_count)
                {
                    memcpy(&g_AppEcm2,&g_AppEcm,sizeof(AppEcm));
                    //g_AppEcm2.DemuxId = 1;// TODO: 20120702
                    app_getvalue(g_AppPvrOps.env.url,"dmxid",&g_AppEcm2.DemuxId);
                    g_AppEcm2.CaManager.ProgId = g_AppPvrOps.env.prog_id;
                    g_AppEcm2.CaManager.Type = CONTROL_PVR;
                    g_AppEcm2.init(&g_AppEcm2);
                    g_AppEcm2.start(&g_AppEcm2);
                }
#endif //DUAL_CHANNEL_DESCRAMBLE_SUPPORT
#endif //ECM_SUPPORT
#endif //CA_SUPPORT
            }
            break;
        default:
            break;
    }
}

void tms_state_change(PlayerStatus state)
{
	AppPvrOps *pvr = &g_AppPvrOps;

    if (pvr->state != PVR_TIMESHIFT
        && pvr->state != PVR_TMS_AND_REC)
    {
        return;
    }

    AppPvrUi  *pvr_ui = &g_AppPvrUi;
	FullArb *arb = &g_AppFullArb;

    switch(state)
    {
        case PLAYER_STATUS_RECORD_FULL:
        {
            book_popdlg_exec(POP_VAL_CANCEL);
            app_pvr_patition_error_proc(STR_ID_NO_SPACE);
            break;
        }
        case PLAYER_STATUS_SHIFT_END:
        {
            app_pvr_patition_error_proc(STR_ID_PVR_END);
            break;
        }
        case PLAYER_STATUS_ERROR:
        {
            if(g_AppPvrOps.state == PVR_TIMESHIFT)
            {
                app_pvr_patition_error_proc(STR_ID_CANT_PVR);
            }
            break;
        }

        case PLAYER_STATUS_SHIFT_START:
        {//时移开始，解复用相关配置完成
            pvr_ui->shift_start = true;
            pvr->spd = 0;
            GUI_SetProperty(TXT_STATE, "string", STR_ID_PAUSE);
            GUI_SetProperty(SLIDER_BAR_SEEK, "state", "show");
GUI_SetFocusWidget(SLIDER_BAR_SEEK);
#if CA_SUPPORT
            // TODO: reset cmb ...
#if ECM_SUPPORT
#if DUAL_CHANNEL_DESCRAMBLE_SUPPORT
            {
                if(app_tsd_check())
                {
                    break;
                }

                if(0 == g_AppEcm2.ecm_count)
                {
                    int demux_id = 0;
                    memcpy(&g_AppEcm2,&g_AppEcm,sizeof(AppEcm));
                    app_getvalue(g_AppPvrOps.env.url,"dmxid",&demux_id);
                    g_AppEcm2.CaManager.ProgId = g_AppPvrOps.env.prog_id;
                    g_AppEcm2.CaManager.Type = CONTROL_PVR;
                    g_AppEcm2.init(&g_AppEcm2);
                    g_AppEcm2.start(&g_AppEcm2);
                }
            }
#endif
#endif
            app_descrambler_close_by_control_type(CONTROL_NORMAL);
#endif
            break;
        }

        case PLAYER_STATUS_SHIFT_RUNNING:
        {
            pvr_ui->shift_start = false;
            pvr_ui->slider_refresh = 1;
            pvr_ui->tick_refresh_delay = 0;

            if(pvr->spd == 0)
			{
				GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
				GUI_SetProperty(SLIDER_BAR_SEEK, "state", "show");
				GUI_SetFocusWidget(SLIDER_BAR_SEEK);
			}
#if CA_SUPPORT
            app_descrambler_close_by_control_type(CONTROL_NORMAL);
#endif
            break;
        }

        case PLAYER_STATUS_SHIFT_HOLD_RUNNING:
        {
            pvr_ui->shift_start = false;
            pvr_ui->slider_refresh = 1;
            pvr_ui->tick_refresh_delay = 0;
			if((pvr->time.total_tick > 0) && (pvr->time.total_tick > pvr->time.cur_tick))
			{
				pvr->time.cur_tick = pvr->time.total_tick;
				pvr->time.remaining_tick = 0;
			}
			if(pvr->spd >= 0)
			{
				pvr->spd = 0;
				arb->draw[EVENT_SPEED](arb);
				//GUI_SetProperty("img_full_pause", "state", "hide");
				//GUI_SetProperty("txt_full_speed", "state", "hide");
				GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
				GUI_SetProperty(SLIDER_BAR_SEEK, "state", "show");
				GUI_SetFocusWidget(SLIDER_BAR_SEEK);

				//	arb->state.pause = STATE_OFF;
				//	arb->draw[EVENT_PAUSE](arb);//错误 #39358
#if CA_SUPPORT

                GxMsgProperty_NodeByPosGet node;
                int demux_id = g_AppPlayOps.normal_play.dmx_id;

				node.node_type = NODE_PROG;
				node.pos = g_AppPlayOps.normal_play.play_count;
				if(GXCORE_SUCCESS == app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, (void*)&node))
                {
                    app_extend_send_service(pvr->env.prog_id, node.prog_data.sat_id, node.prog_data.tp_id, demux_id, CONTROL_NORMAL);
                }

#endif
			}
            break;
        }

        case PLAYER_STATUS_SHIFT_PAUSE:
        {
            GUI_SetProperty(TXT_STATE, "string", STR_ID_PAUSE);
            break;
        }

        default:
            break;
    }
}

//Duration Set Start!
static void _get_pvr_duration_from_event(void)
{
#define HALF_HOUR_SEC       (60*30)
    time_t system_sec = 0;
    int32_t     duration;
    bool cur_flag = false;
    bool next_flag = false;
    AppEpgOps ch_epg_ops;
    GxEpgInfo *cur_epg_info = NULL;
    GxEpgInfo *next_epg_info = NULL;
    EpgProgInfo epg_prog_info;
    GxMsgProperty_NodeByPosGet node_prog = {0};

    epg_ops_init(&ch_epg_ops);
    GxBus_ConfigGetInt(PVR_DURATION_KEY, &duration, PVR_DURATION_VALUE);
    pvr_duration = (duration+4)*HALF_HOUR_SEC;//2H

    node_prog.node_type = NODE_PROG;
    node_prog.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);

	epg_prog_info.ts_id= node_prog.prog_data.ts_id;
	epg_prog_info.orig_network_id= node_prog.prog_data.original_id;
	epg_prog_info.service_id= node_prog.prog_data.service_id;
	epg_prog_info.prog_id= node_prog.prog_data.id;
	epg_prog_info.prog_pos = node_prog.pos;
    epg_prog_info.tp_id = node_prog.prog_data.tp_id;

    cur_epg_info = (GxEpgInfo *)GxCore_Malloc(EVENT_GET_NUM*EVENT_GET_MAX_SIZE);
	if(cur_epg_info == NULL)
	{
		app_log_error("_get_pvr_duration_from_event, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return ;
	}

    next_epg_info = (GxEpgInfo *)GxCore_Malloc(EVENT_GET_NUM*EVENT_GET_MAX_SIZE);
	if(next_epg_info == NULL)
	{
        APP_FREE(cur_epg_info);
		app_log_error("_get_pvr_duration_from_event, malloc failed...%s,%d\n",__FILE__,__LINE__);
		return ;
	}

    ch_epg_ops.prog_info_update(&ch_epg_ops, &epg_prog_info);
    if(ch_epg_ops.get_cur_event_cnt(&ch_epg_ops) > 0)
    {
        if(cur_epg_info != NULL)
        {
            if(ch_epg_ops.get_cur_event_info(&ch_epg_ops, cur_epg_info) == GXCORE_SUCCESS)
            {
                if(cur_epg_info->event_name_len > 0)
                {
                    cur_flag = true;
                }
            }
        }

        if(next_epg_info != NULL)
        {
            if(ch_epg_ops.get_next_event_info(&ch_epg_ops, next_epg_info) == GXCORE_SUCCESS)
            {
                if(next_epg_info->event_name_len > 0)
                {
                    next_flag = true;
                }
            }
        }
    }

    if(cur_flag)
    {
        system_sec = g_AppTime.utc_get(&g_AppTime);
        if(system_sec > cur_epg_info->start_time && (system_sec < cur_epg_info->start_time + cur_epg_info->duration))
        {
            pvr_duration = cur_epg_info->duration - (system_sec - cur_epg_info->start_time);
            if(pvr_duration < 3*60)//3 MIN
            {
                if(next_flag)
                {
                    pvr_duration += next_epg_info->duration;
                    if(pvr_duration < 3*60)
                    {
                        pvr_duration = 3*60;
                    }
                }
                else
                {
                    pvr_duration = (duration+4)*HALF_HOUR_SEC;//2H
                }
            }
        }
    }

    APP_FREE(cur_epg_info);
    APP_FREE(next_epg_info);
}

SIGNAL_HANDLER int app_edit_duration_set_reach_end(GuiWidget *widget, void *usrdata)
{
    uint32_t sel = 0;
    uint32_t cur_sel = 0;
#define EDIT_DURATION_SET_LEN 5
    GUI_GetProperty("edit_duration_set", "select", &cur_sel);
    sel = (cur_sel == 0 ? EDIT_DURATION_SET_LEN - 1 : 0);
    GUI_SetProperty("edit_duration_set", "select", &sel);
    return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_duration_set_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
	time_t time = 0;
	char *buffer = NULL;
    AppPvrOps   *pvr        = &g_AppPvrOps;
    AppPvrUi    *pvr_ui     = &g_AppPvrUi;
    pvr_timer_mode mode     = PVR_TIMER_REC_END;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
                case STBK_EXIT:
                case STBK_MENU:
                    GUI_EndDialog(WND_DURATION_SET);
                    break;
                case STBK_OK:
                    GUI_GetProperty(EDIT_DURATION, "string", &buffer);
                    time = app_edit_str_to_hourmin_sec(buffer);

                    if(time > pvr->time.total_tick)
                    {
                        pvr->time.rec_duration = time;
                    }
                    else
                    {
                        PopDlg pop;
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_OK;
                        pop.format = POP_FORMAT_DLG;
                        pop.str = STR_ID_END_TIME_ERR;
                        pop.timeout_sec= 3;
                        pop.mode = POP_MODE_UNBLOCK;
                        popdlg_create(&pop);
                        break;
                    }

                    //remove old timer
                    if (pvr_ui->timer.timer[mode] != NULL)
                    {
                        remove_timer(pvr_ui->timer.timer[mode]);
                        pvr_ui->timer.timer[mode] = NULL;
                    }

                    //create new timer
                    pvr_ui->timer.cb[mode]         = pvr_timer_rec_duration;
                    pvr_ui->timer.interval[mode]   = (pvr->time.rec_duration - pvr->time.total_tick)*SEC_TO_MILLISEC;
                    pvr_ui->timer.flag[mode]       = TIMER_ONCE;

                    if(0 != reset_timer(pvr_ui->timer.timer[mode]))
                    {
                        pvr_ui->timer.timer[mode] = create_timer(pvr_ui->timer.cb[mode],
                                pvr_ui->timer.interval[mode], NULL, pvr_ui->timer.flag[mode]);
                    }

                    GUI_EndDialog(WND_DURATION_SET);
                    break;
                default:
                    break;
            }
    }

    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_duration_set_create(GuiWidget *widget, void *usrdata)
{
#define HM_STR_LEN 16
	char *string = NULL;
    AppPvrOps   *pvr        = &g_AppPvrOps;
	struct tm temp_tm = {0};

	string = (char *)GxCore_Malloc(HM_STR_LEN);
	if(string != NULL)
	{
		memset(string, 0, HM_STR_LEN);
        sectohour((time_t)(pvr->time.rec_duration), &temp_tm);
		sprintf(string, "%02d:%02d", temp_tm.tm_hour , temp_tm.tm_min);
	}
    //string = app_time_to_hourmin_edit_str(pvr->time.rec_duration);
    if(string != NULL)
    {
        GUI_SetProperty(EDIT_DURATION, "string", string);
        GxCore_Free(string);
        string = NULL;
    }

    return EVENT_TRANSFER_STOP;
}


SIGNAL_HANDLER int app_duration_set_destroy(GuiWidget *widget, void *usrdata)
{
	if (reset_timer(sp_PvrBarTimer) != 0)
	{
		sp_PvrBarTimer = create_timer(_pvr_bar_timeout, PVRBAR_TIMEOUT*SEC_TO_MILLISEC, NULL, TIMER_ONCE);
	}
    return EVENT_TRANSFER_STOP;
}
//Duration Set End!

int app_pvr_stop_cb(PopDlgRet ret)
{
    if(ret == POP_VAL_OK)
    {
        GxMsgProperty_NodeByPosGet node_prog = {0};
        uint32_t rec_prog_id = g_AppPvrOps.env.prog_id;
        GUI_SetInterface("flush",NULL);

        if((g_AppPvrOps.state != PVR_RECORD) && (g_AppPvrOps.state != PVR_TP_RECORD))
        {
            app_pvr_stop();
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
        else
        {
            app_pvr_stop();
        }

    }

	if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
	{
		if(app_channel_info_check_dialog() != GXCORE_SUCCESS)
			app_channel_info_create_dialog();
	}
    return 0;
}

SIGNAL_HANDLER int app_pvr_bar_keypress(GuiWidget *widget, void *usrdata)
{
    GUI_Event *event = NULL;
    AppPvrOps *pvr = &g_AppPvrOps;
    FullArb *arb = &g_AppFullArb;
    AppPvrUi  *pvr_ui = &g_AppPvrUi;
    int freeze = 0;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
                case STBK_EXIT:
                case STBK_MENU:
                case VK_BOOK_TRIGGER:
                    GUI_EndDialog(WND_PVR_BAR);
                    if(GXCORE_ERROR == app_channel_info_check_dialog())
                    {
                        app_channel_info_create_dialog();
                    }
                    break;

                case STBK_PAUSE:
                case STBK_PAUSE_STB:
                case STBK_TMS:
                {
#if INDEPEND_RECORD
                    {
                        GUI_SendEvent(WND_FULL_SCREEN, event);
                    }
                    break;
#endif
                    if(s_pvr_seek_flag == true && g_AppFullArb.state.tv == STATE_ON)
                    {
                        break;
                    }
                    reset_timer(sp_PvrBarTimer);
                    GxMsgProperty_NodeByPosGet node_prog = {0};
                    node_prog.node_type = NODE_PROG;
                    node_prog.pos = g_AppPlayOps.normal_play.play_count;
                    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);
                    if((pvr->state != PVR_TP_RECORD)
                        && ((pvr->state == PVR_DUMMY)
                            || (node_prog.prog_data.id == g_AppPvrOps.env.prog_id)))
                    {
                        if(arb->state.pause == STATE_OFF)
                        {
                            freeze = 1;
                            app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);

                            GUI_SetProperty(TXT_STATE, "string", STR_ID_PAUSE);
                            if(pvr->state == PVR_RECORD)
                            {
                                pvr->time.cur_tick = pvr->time.total_tick;
                                if (pvr_ui->timer.timer[PVR_TIMER_REC_END] != NULL)
                                {
	                                remove_timer(pvr_ui->timer.timer[PVR_TIMER_REC_END]);
	                                pvr_ui->timer.timer[PVR_TIMER_REC_END] = NULL;
                                }
                            }
                            pvr->pause(pvr);
#ifndef COLOR_16BIT_TTX_SUPPORT
                            extern void app_old_ttx_pause(void);
                            app_old_ttx_pause();
#endif
                            arb->state.pause = STATE_ON;
                            arb->draw[EVENT_PAUSE](arb);
                            arb->state.tms = STATE_ON;
                            pvr_percent_show();
                            arb->draw[EVENT_STATE](arb);
                            GUI_SetProperty(SLIDER_BAR_SEEK, "state", "show");
                            GUI_SetFocusWidget(SLIDER_BAR_SEEK);
                        }
                        else
                        {
                            GxBus_ConfigGetInt(FREEZE_SWITCH, &freeze, FREEZE_SWITCH_VALUE);
                            app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);

                            GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
                            if(pvr_ui->shift_start == true)
                            {
                                pvr_ui->tick_refresh_delay = TICK_DELAY * 2;
                                pvr_ui->slider_refresh = 4;
                            }
                            if (pvr->spd != 0)
                            {
#if 1//(TRICK_PLAY_SUPPORT > 0)
//#9886
//								pvr_slider_bk(1);
								float spd = 0;
								// if fb or ff, set normal speed
								if(pvr->spd < 0)
								{
									spd = ((float)spd_val[(pvr->spd*(-1))])*(-1);
								}
								else
								{
									spd = spd_val[pvr->spd];
								}
								//pvr->spd = 0;
//								pvr->speed(pvr,PVR_SPD_1);
								arb->state.pause = STATE_OFF;
								arb->draw[EVENT_PAUSE](arb);
								pvr->resume(pvr);
								tms_speed_change(spd);
#else
								pvr->spd = 0;
								arb->state.pause = STATE_OFF;
								arb->draw[EVENT_PAUSE](arb);
								pvr->speed(pvr,PVR_SPD_1);
#endif
							}
							else
							{
								arb->state.pause = STATE_OFF;
								arb->draw[EVENT_PAUSE](arb);
								pvr->resume(pvr);
							}
						}
					}
					else
                    {
                        GxMsgProperty_PlayerPause player_pause;
                        GxMsgProperty_PlayerResume player_resume;
                        if (arb->state.pause == STATE_ON)
                        {
                            GxBus_ConfigGetInt(FREEZE_SWITCH, &freeze, FREEZE_SWITCH_VALUE);
                            app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);
                            player_resume.player = PLAYER_FOR_NORMAL;
                            app_send_msg_exec(GXMSG_PLAYER_RESUME, (void *)(&player_resume));
                            arb->state.pause = STATE_OFF;
                        }
                        else
                        {
                            app_subt_pause();
                            freeze = 1;
                            app_send_msg_exec(GXMSG_PLAYER_FREEZE_FRAME_SWITCH, &freeze);
                            player_pause.player = PLAYER_FOR_NORMAL;
                            app_send_msg_exec(GXMSG_PLAYER_PAUSE, (void *)(&player_pause));
                            arb->state.pause = STATE_ON;
                        }
                        GUI_EndDialog(WND_PVR_BAR);
                        GUI_SetProperty(WND_PVR_BAR, "draw_now", NULL);
						arb->draw[EVENT_PAUSE](arb);
                    }
                    break;
                }

                case STBK_REC_START:
                    if (pvr->state == PVR_RECORD)
                    {
                        if(GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
                            GUI_EndDialog(WND_PASSWD);
                        GUI_CreateDialog(WND_DURATION_SET);
						g_AppFullArb.draw[EVENT_STATE](&g_AppFullArb);
                        timer_stop(sp_PvrBarTimer);
                    }
                    else if(pvr->state != PVR_TP_RECORD)
                    {
                        app_pvr_rec_start(0);
                    }
                    break;

                case STBK_REC_STOP:
                    {
                        PopDlg pop;
                        if(GUI_CheckDialog(WND_PVR_BAR) == GXCORE_SUCCESS)
                        {
                            GUI_EndDialog(WND_PVR_BAR);
                            GUI_SetProperty(WND_PVR_BAR, "draw_now", NULL);
                        }
                        memset(&pop, 0, sizeof(PopDlg));
                        pop.type = POP_TYPE_YES_NO;
                        pop.str = app_pvr_get_tips(pvr->state);
                        pop.mode = POP_MODE_UNBLOCK;
                        pop.exit_cb = app_pvr_stop_cb;
                        popdlg_create(&pop);

                    }
                    break;

                case STBK_MUTE:
                    reset_timer(sp_PvrBarTimer);
                    arb->exec[EVENT_MUTE](arb);
                    arb->draw[EVENT_MUTE](arb);
                    break;

			case STBK_CH_UP:
			case STBK_UP:
			case STBK_CH_DOWN:
			case STBK_DOWN:
				if(GUI_CheckDialog(WND_PVR_BAR) == GXCORE_SUCCESS)
				{
					GUI_EndDialog(WND_PVR_BAR);
                GUI_SetInterface("flush", NULL);
				}
				GUI_SendEvent(GUI_GetFocusWindow(), event);
				break;

			case STBK_OK:
            case STBK_EPG:
				if(GUI_CheckDialog(WND_PVR_BAR) == GXCORE_SUCCESS)
				{
					GUI_EndDialog(WND_PVR_BAR);
                GUI_SetInterface("flush", NULL);
				}
				GUI_SendEvent(GUI_GetFocusWindow(), event);
				break;

                case STBK_VOLDOWN:
                case STBK_VOLUP:
                    if(GUI_CheckDialog(WND_PVR_BAR) == GXCORE_SUCCESS)
                    {
                        GUI_EndDialog(WND_PVR_BAR);
                        GUI_SetInterface("flush", NULL);
                    }
                    GUI_CreateDialog(WND_VOLUME);
                    GUI_SendEvent(WND_VOLUME, event);
                    break;

           default:
              break;
        }
    }

	return EVENT_TRANSFER_STOP;
}

SIGNAL_HANDLER int app_pvr_sliderbar_keypress(GuiWidget *widget, void *usrdata)
{
    status_t ret = EVENT_TRANSFER_STOP;
    GUI_Event *event = (GUI_Event *)usrdata;
    AppPvrOps *pvr = &g_AppPvrOps;
    //pvr_speed spd_val[] = {PVR_SPD_1,PVR_SPD_2,PVR_SPD_4,PVR_SPD_8,PVR_SPD_16,PVR_SPD_32};
    FullArb *arb = &g_AppFullArb;


    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
            switch(event->key.sym)
            {
                case STBK_LEFT:
                case STBK_RIGHT:
                case STBK_1:
                case STBK_2:
                case STBK_3:
                case STBK_4:
                case STBK_5:
                case STBK_6:
                case STBK_7:
                case STBK_8:
                case STBK_9:
                case STBK_0:
                    if(GUI_CheckDialog(WND_PASSWD) == GXCORE_SUCCESS)
                    {
                        GUI_SendEvent(WND_PASSWD, event);
                    }
                    break;

                case STBK_FB:
                    if((g_AppPvrOps.state == PVR_TIMESHIFT)
                            ||(g_AppPvrOps.state == PVR_TMS_AND_REC))
                    {
                        arb->state.pause = STATE_OFF;
                        if (pvr->spd > 0)
                            pvr->spd = 0;

                        (pvr->spd<=-SPD_NUM)?(pvr->spd=0):(pvr->spd--);
                        if(pvr->spd != 0)
                        {
                            pvr->speed(pvr, ((float)spd_val[(pvr->spd*(-1))])*(-1));
                        }
                        else
                        {
                            pvr->speed(pvr, spd_val[pvr->spd]);
                        }
                    }
                    break;

                case STBK_FF:
                    if((g_AppPvrOps.state == PVR_TIMESHIFT)
                            ||(g_AppPvrOps.state == PVR_TMS_AND_REC))
                    {
                        arb->state.pause = STATE_OFF;
                        if (pvr->spd < 0)
                            pvr->spd = 0;

                        (pvr->spd>=SPD_NUM)?(pvr->spd=0):(pvr->spd++);
                        pvr->speed(pvr, spd_val[pvr->spd]);
                    }
                    break;

                case STBK_PLAY:
                    reset_timer(sp_PvrBarTimer);
                    if (pvr->spd != 0)
                    {
                        pvr->spd = 0;
                        pvr->speed(pvr, PVR_SPD_1);
                    }
                    else
                    {
                        GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
                        if(pvr->enter_shift == 0)
                            pvr->resume(pvr);
                    }
                    arb->state.pause = STATE_OFF;
                    //	arb->draw[EVENT_PAUSE](arb);
                    arb->draw[EVENT_SPEED](arb);

#ifdef COLOR_16BIT_TTX_SUPPORT
                    app_subt_resume();
#endif
                    break;

                default:
                    ret = EVENT_TRANSFER_KEEPON;
                    break;
            }
    }
    return ret;
}

SIGNAL_HANDLER int app_pvr_seek_slider_keypress(GuiWidget *widget, void *usrdata)
{
#define TMS_SEEK_DELAY (0) //ms
	status_t ret = EVENT_TRANSFER_STOP;
	uint32_t slider = 0;
	GUI_Event *event = NULL;
    AppPvrOps *pvr = &g_AppPvrOps;
    AppPvrUi  *pvr_ui = &g_AppPvrUi;
    FullArb *arb = &g_AppFullArb;
	int32_t spd_fb = 0;

    event = (GUI_Event *)usrdata;
    switch(event->type)
    {
        case GUI_SERVICE_MSG:
            break;

        case GUI_MOUSEBUTTONDOWN:
            break;

        case GUI_KEYDOWN:
    switch(event->key.sym)
    {
		case STBK_LEFT:
		case STBK_RIGHT:
            reset_timer(sp_PvrBarTimer);
            if((pvr->time.remaining_tick == 0) && (pvr_ui->slider_refresh != 0) && (event->key.sym == STBK_RIGHT))
                break;

            // fb or ff can't control slider bar
            if ((pvr->spd != 0) && (arb->state.pause == STATE_OFF))
                break;
            // rec can't control slider bar
            if ((pvr->state!=PVR_TIMESHIFT) && (pvr->state!=PVR_TMS_AND_REC))
                break;

            pvr_ui->slider_refresh = 0;
            ret = EVENT_TRANSFER_KEEPON;
            break;

            case STBK_OK:
                if(pvr_ui->slider_refresh == 0)
                {
                    reset_timer(sp_PvrBarTimer);

                    if ((pvr->state == PVR_TIMESHIFT) || (pvr->state == PVR_TMS_AND_REC))
                    {
						s_pvr_seek_flag = true;
						uint32_t seek_time = 0;
                        GUI_GetProperty(SLIDER_BAR_SEEK, "value", &slider);
                        seek_time = ((pvr->time.total_tick-pvr->time.seekmin_tick)*slider/100)+pvr->time.seekmin_tick;
						char cur_tick[16]= {0};
						struct tm tm = {0};
						sectohour(seek_time, &tm);
						sprintf(cur_tick, "%02d:%02d:%02d", tm.tm_hour, tm.tm_min, tm.tm_sec);
						GUI_SetProperty(TXT_CUR_TICK, "string", cur_tick);

                        if(pvr->time.total_tick - seek_time < 5)
                        {
                            pvr_ui->slider_refresh = 4;
                            pvr_ui->tick_refresh_delay = TICK_DELAY * 4;
                        }
                        else
                        {
                            pvr_ui->slider_refresh = 1;
                            pvr_ui->tick_refresh_delay = TICK_DELAY;
                        }
                        if(arb->state.pause == STATE_ON)
                        {
                             if (pvr->spd != 0)
                            {
                                // if fb or ff, set normal speed
                                pvr->spd = 0;
                                pvr->speed(pvr,PVR_SPD_1);
								arb->draw[EVENT_SPEED](arb);
                            }
                            else
                            {
                                GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
								if(pvr->enter_shift == 0)
                                pvr->resume(pvr);
                            }
							pvr->seek(pvr,seek_time);

							pvr->time.cur_tick = seek_time;
                            arb->state.pause = STATE_OFF;
                            arb->draw[EVENT_PAUSE](arb);

						}
						else
						{
							pvr->seek(pvr, seek_time);
							pvr->time.cur_tick = seek_time;
						}
						GUI_SetProperty(SLIDER_BAR, "value", &slider);
					}
                    else
                    {}
                }
                else
                {
                    ret = EVENT_TRANSFER_KEEPON;
                }
            break;

            case STBK_FB:

#if REDIO_RECODE_SUPPORT
                if(arb->state.tv == STATE_OFF)
                {
                    PopDlg pop;
                    memset(&pop, 0, sizeof(PopDlg));
                    pop.type = POP_TYPE_NO_BTN;
                    pop.format = POP_FORMAT_DLG;
                    pop.str = STR_ID_MODE_NOT_SUPPORT;
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.timeout_sec = 3;
                    popdlg_create(&pop);
                    break;
                }
#endif
                if ((pvr->state!=PVR_TIMESHIFT)&&(pvr->state!=PVR_TMS_AND_REC))
                    break;

                if(s_pvr_seek_flag == true)
                    break;

                if(pvr->time.total_tick <= TMS_SEEK_DELAY)
                    break;

                if(pvr->time.cur_tick == 0)
                    break;

                if(pvr_ui->shift_start == true)
                {
                    if(pvr->time.remaining_tick > TMS_SEEK_DELAY)
                    {
                        pvr->resume(pvr);
                    }
                    else
                    {
                        break;
                    }
                }

                arb->state.pause = STATE_OFF;
                arb->draw[EVENT_PAUSE](arb);
				if(pvr->spd > 0)
					pvr->spd = 0;

				spd_fb = pvr->spd;
				(spd_fb<=-SPD_NUM)?(pvr->spd=spd_fb=0):(spd_fb--);
				if(spd_fb != 0)
	            {
	                pvr->speed(pvr, ((float)spd_val[(spd_fb*(-1))])*(-1));
	            }
	            else
	            {
	                pvr->speed(pvr, spd_val[spd_fb]);
				}

                break;

            case STBK_FF:

#if REDIO_RECODE_SUPPORT
                if(arb->state.tv == STATE_OFF)
                {
                    PopDlg pop;
                    memset(&pop, 0, sizeof(PopDlg));
                    pop.type = POP_TYPE_NO_BTN;
                    pop.format = POP_FORMAT_DLG;
                    pop.str = STR_ID_MODE_NOT_SUPPORT;
                    pop.mode = POP_MODE_UNBLOCK;
                    pop.timeout_sec = 3;
                    popdlg_create(&pop);
                    break;
                }
#endif
                if ((pvr->state != PVR_TIMESHIFT)&&(pvr->state!= PVR_TMS_AND_REC))
                    break;

                if(s_pvr_seek_flag == true)
                    break;

                if(pvr->time.total_tick<= TMS_SEEK_DELAY)
                    break;

                if(pvr->time.remaining_tick <= TMS_SEEK_DELAY)
                    break;

                if(pvr_ui->shift_start == true)
                {
                    pvr->resume(pvr);
                }

                arb->state.pause = STATE_OFF;
                arb->draw[EVENT_PAUSE](arb);
				if(pvr->spd < 0)
					pvr->spd = 0;

				(pvr->spd>=SPD_NUM)?(pvr->spd=0):(pvr->spd++);

                pvr->speed(pvr, spd_val[pvr->spd]);

                break;

            case STBK_PLAY://pause resume
                reset_timer(sp_PvrBarTimer);
                if(arb->state.pause == STATE_ON)
                {
                    GUI_SetProperty(TXT_STATE, "string", STR_ID_PLAY);
                    if(pvr_ui->shift_start == true)
                    {
                        pvr_ui->tick_refresh_delay = TICK_DELAY * 2;
                        pvr_ui->slider_refresh = 4;
                    }
                    if (pvr->spd != 0)
                    {
                        // if fb or ff, set normal speed
                        pvr->spd = 0;
                        pvr->speed(pvr,PVR_SPD_1);
                    }
                    else
                    {
                        pvr->resume(pvr);
                    }
                    arb->state.pause = STATE_OFF;
                    arb->draw[EVENT_PAUSE](arb);
                    arb->draw[EVENT_SPEED](arb);

                }

            break;

            default:
                ret = EVENT_TRANSFER_KEEPON;
                break;
        }
    }
    return ret;
}

void app_update_channel_list_after_pvr_stop(void)
{
    int32_t sel;
    uint32_t cmb_sel;

    if(GUI_CheckDialog(WND_CHANNEL_LIST) == GXCORE_SUCCESS)
    {
        sel = app_channel_list_get_cur_list_sel();
        cmb_sel = channel_list_get_cur_group_num();
        GUI_SetProperty("cmb_channel_list_group", "select", &cmb_sel);
        GUI_SetProperty("listview_channel_list", "update_all", NULL);
        GUI_SetProperty(WND_CHANNEL_LIST, "update_all", NULL);
        GUI_SetProperty("listview_channel_list", "select", &sel);
        GUI_SetProperty("listview_channel_list", "active", &sel);
        app_update_pop_dlg(WND_PASSWD);
        app_update_pop_dlg(WND_POP_TIP);

    }
    else if (app_channel_info_check_dialog() == GXCORE_SUCCESS)
    {
        app_update_pop_dlg(WND_PASSWD);
        app_update_pop_dlg(WND_POP_TIP);
    }
    return;
}

void app_pvr_patition_error_proc(char* str_tip)
{
    pvr_state   state;

	if(GUI_CheckDialog(WND_POP_OPTION_LIST) == GXCORE_SUCCESS)
	{
		extern int app_pop_list_end_dialog(void);
		app_pop_list_end_dialog();
	}
	popdlg_destroy();
#if WEBLET_SUPPORT
	if(GUI_CheckDialog(WND_POP_QR) == GXCORE_SUCCESS)
	{
		GUI_EndDialog(WND_POP_QR);
		GUI_SetInterface("flush", NULL);
	}
#endif
    if(GUI_CheckDialog(WND_COMMON_SELECT) == GXCORE_SUCCESS)
        GUI_EndDialog(WND_COMMON_SELECT);

    if(app_channel_info_check_dialog() == GXCORE_SUCCESS && (GUI_CheckDialog(WND_KEYBOARD_LANGUAGE) == GXCORE_SUCCESS))
    {
        static GUI_Event event;
        event.type = GUI_KEYDOWN;
        event.key.sym = STBK_EXIT;
        GUI_SendEvent(WND_KEYBOARD_LANGUAGE, &event);
    }
    GxMsgProperty_NodeByPosGet node_prog = {0};
    uint32_t rec_prog_id = g_AppPvrOps.env.prog_id;

    GUI_SetProperty("img_full_mute", "img", "s_mute");
    GUI_SetProperty("img_full_pause", "img", "s_pause");

    state = g_AppPvrOps.state;
    GUI_EndDialog(WND_PVR_BAR);
    app_pvr_stop();
    GUI_EndDialog(WND_PASSWD);
    app_update_channel_list_after_pvr_stop();
    app_channel_info_signal_end_dialog();
    GUI_EndDialog(WND_VOLUME);

    node_prog.node_type = NODE_PROG;
    node_prog.pos = g_AppPlayOps.normal_play.play_count;
    app_send_msg_exec(GXMSG_PM_NODE_BY_POS_GET, &node_prog);

    if((state != PVR_RECORD) && (state != PVR_TP_RECORD))
    {
        if(node_prog.prog_data.id == rec_prog_id)
        {
            g_AppPlayOps.normal_play.key = PLAY_KEY_UNLOCK;
            app_play_current_prog(PLAY_MODE_POINT);
        }
    }

    if(g_AppPlayOps.normal_play.view_info.stream_type == GXBUS_PM_PROG_RADIO)
    {
        if((app_channel_info_check_dialog() != GXCORE_SUCCESS)
                && (app_channel_list_check_dialog() != GXCORE_SUCCESS))
            app_channel_info_create_dialog();
    }

    if(GUI_CheckDialog(WND_POP_BOOK) != GXCORE_SUCCESS)
    {
        PopDlg  pop;
        memset(&pop, 0, sizeof(PopDlg));
        pop.type = POP_TYPE_OK;
        pop.mode = POP_MODE_UNBLOCK;

        if(g_AppPvrOps.usb_check(&g_AppPvrOps) != USB_ERROR)
        {
            pop.str = str_tip;
        }
        else
        {
            pop.str = STR_ID_USB_OUT;
        }
        popdlg_create(&pop);
        app_channel_info_update_under_pop_dlg();
    }
    else
    {
        GUI_SetProperty(WND_POP_BOOK, "update", NULL);
    }
}

AppPvrUi g_AppPvrUi =
{
    .ui             = PVR_HIDE,
    .slider_refresh = 1,
    .taxis          = pvr_taxis_mode,
    .timer  = {{NULL,},{0,},{0,},{NULL,},pvr_timer_init,pvr_timer_release,pvr_timer_start},
};

